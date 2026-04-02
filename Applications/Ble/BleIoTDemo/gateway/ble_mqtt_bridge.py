#!/usr/bin/env python3
"""
ble_mqtt_bridge.py – QPG6200 BLE IoT Demo: BLE-to-MQTT Gateway Bridge
=======================================================================

Runs on the Raspberry Pi gateway (192.168.10.102).

Purpose
-------
This script bridges the QPG6200 BLE peripheral to the MQTT broker so that
Node-RED can display sensor data and control the device LED via a dashboard.

Data flow
---------
QPG6200 (BLE peripheral)  ←→  this script (BLE central)  ←→  MQTT broker
                                                           ←→  Node-RED

Inbound (QPG6200 → MQTT):
  BLE notification: Button State char  →  topic: qorvo/demo/button
  BLE notification: Heartbeat char     →  topic: qorvo/demo/heartbeat
  Connection event                     →  topic: qorvo/demo/status

Outbound (MQTT → QPG6200):
  topic: qorvo/demo/led  →  BLE write: LED Control char

MQTT payload formats
--------------------
  qorvo/demo/button    : JSON  {"state": "pressed"}  or  {"state": "released"}
  qorvo/demo/heartbeat : JSON  {"count": <0-255>, "timestamp": "<ISO-8601>"}
  qorvo/demo/status    : JSON  {"connected": true|false, "device": "<name>"}
  qorvo/demo/led       : JSON  {"led": "on"}  or  {"led": "off"}
                         (plain text "on" / "off" also accepted)

Dependencies
------------
  pip install bleak paho-mqtt

Usage
-----
  python3 ble_mqtt_bridge.py [--device <name>] [--broker <ip>] [--port <port>]

Requirements
------------
  - Python 3.8+
  - Bluetooth adapter visible to the OS (hciconfig shows hci0)
  - Mosquitto MQTT broker running on the Pi (sudo systemctl start mosquitto)
"""

import asyncio
import argparse
import json
import logging
import signal
import sys
from datetime import datetime, timezone

import paho.mqtt.client as mqtt
from bleak import BleakScanner, BleakClient
from bleak.backends.characteristic import BleakGATTCharacteristic

# ---------------------------------------------------------------------------
#  Logging
# ---------------------------------------------------------------------------

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
log = logging.getLogger("ble_mqtt_bridge")

# ---------------------------------------------------------------------------
#  Configuration defaults
# ---------------------------------------------------------------------------

DEFAULT_DEVICE_NAME = "QPG IoT Demo"
DEFAULT_MQTT_BROKER = "localhost"   # Runs on the Pi itself
DEFAULT_MQTT_PORT   = 1883

# Scan timeout (seconds) per scan attempt before retrying
SCAN_TIMEOUT_SEC = 10

# Delay (seconds) before attempting reconnect after a disconnect
RECONNECT_DELAY_SEC = 5

# ---------------------------------------------------------------------------
#  BLE UUIDs – must exactly match BleIoTDemo_Config.c in the firmware
#
#  Service  : A1B2C3D4-E5F6-7890-ABCD-EF0123456789  (IoT Demo Service)
#  Char UUIDs (last byte distinguishes them):
#    Button State  : …6790
#    LED Control   : …6791
#    Heartbeat     : …6792
# ---------------------------------------------------------------------------

DEMO_SERVICE_UUID    = "a1b2c3d4-e5f6-7890-abcd-ef0123456789"
BUTTON_STATE_UUID    = "a1b2c3d4-e5f6-7890-abcd-ef0123456790"
LED_CONTROL_UUID     = "a1b2c3d4-e5f6-7890-abcd-ef0123456791"
HEARTBEAT_UUID       = "a1b2c3d4-e5f6-7890-abcd-ef0123456792"

# ---------------------------------------------------------------------------
#  MQTT topic names
# ---------------------------------------------------------------------------

TOPIC_BUTTON    = "qorvo/demo/button"
TOPIC_HEARTBEAT = "qorvo/demo/heartbeat"
TOPIC_STATUS    = "qorvo/demo/status"
TOPIC_LED       = "qorvo/demo/led"

# ---------------------------------------------------------------------------
#  MQTT client setup
# ---------------------------------------------------------------------------

mqtt_client: mqtt.Client = None
ble_client_ref: BleakClient | None = None  # Updated when BLE connects/disconnects


def _now_iso() -> str:
    """Return the current UTC time as an ISO-8601 string."""
    return datetime.now(timezone.utc).isoformat()


def _publish(topic: str, payload: dict) -> None:
    """Serialise payload as JSON and publish to the MQTT broker."""
    if mqtt_client is None:
        log.warning("MQTT client not ready – dropping message on %s", topic)
        return
    msg = json.dumps(payload)
    result = mqtt_client.publish(topic, msg, qos=0, retain=False)
    if result.rc != mqtt.MQTT_ERR_SUCCESS:
        log.error("MQTT publish failed on %s (rc=%d)", topic, result.rc)
    else:
        log.debug("→ MQTT %s : %s", topic, msg)


def _publish_status(connected: bool, device_name: str = DEFAULT_DEVICE_NAME) -> None:
    """Publish a device connection status message."""
    _publish(TOPIC_STATUS, {
        "connected": connected,
        "device":    device_name,
        "timestamp": _now_iso(),
    })


def _setup_mqtt(broker: str, port: int) -> mqtt.Client:
    """Create, configure, and connect the MQTT client."""

    client = mqtt.Client(client_id="ble_mqtt_bridge", clean_session=True)
    client.enable_logger(logging.getLogger("mqtt"))

    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            log.info("MQTT connected to %s:%d", broker, port)
            # Subscribe to the LED control topic so we can forward to BLE
            client.subscribe(TOPIC_LED, qos=0)
            log.info("Subscribed to %s", TOPIC_LED)
        else:
            log.error("MQTT connection failed (rc=%d)", rc)

    def on_disconnect(client, userdata, rc):
        log.warning("MQTT disconnected (rc=%d) – will auto-reconnect", rc)

    def on_message(client, userdata, message: mqtt.MQTTMessage):
        """Forward LED commands from MQTT to the BLE LED Control characteristic."""
        payload_str = message.payload.decode("utf-8", errors="replace").strip()
        log.info("← MQTT %s : %s", message.topic, payload_str)

        if message.topic != TOPIC_LED:
            return

        # Accept both JSON {"led": "on"} and plain text "on" / "off"
        try:
            data = json.loads(payload_str)
            command = str(data.get("led", "")).lower()
        except (json.JSONDecodeError, AttributeError):
            command = payload_str.lower()

        led_value = bytes([0x01]) if command == "on" else bytes([0x00])

        # Schedule the BLE write on the asyncio event loop
        loop = asyncio.get_event_loop()
        loop.call_soon_threadsafe(
            asyncio.ensure_future,
            _write_led_characteristic(led_value)
        )

    client.on_connect    = on_connect
    client.on_disconnect = on_disconnect
    client.on_message    = on_message

    log.info("Connecting to MQTT broker %s:%d …", broker, port)
    client.connect_async(broker, port, keepalive=60)
    client.loop_start()

    return client


async def _write_led_characteristic(value: bytes) -> None:
    """Write a value to the LED Control BLE characteristic (fire-and-forget)."""
    global ble_client_ref
    if ble_client_ref is None or not ble_client_ref.is_connected:
        log.warning("BLE not connected – LED command dropped")
        return

    try:
        await ble_client_ref.write_gatt_char(
            LED_CONTROL_UUID,
            value,
            response=False  # Write Without Response (ATT_PROP_WRITE)
        )
        state = "ON" if value[0] else "OFF"
        log.info("BLE write: LED Control → %s", state)
    except Exception as exc:
        log.error("BLE write failed: %s", exc)


# ---------------------------------------------------------------------------
#  BLE notification callbacks
# ---------------------------------------------------------------------------

def _on_button_state(characteristic: BleakGATTCharacteristic, data: bytearray) -> None:
    """Called when the QPG6200 sends a Button State notification."""
    value = data[0] if data else 0
    state = "pressed" if value else "released"
    log.info("BLE notify: Button State = %s (0x%02X)", state, value)
    _publish(TOPIC_BUTTON, {
        "state":     state,
        "raw":       value,
        "timestamp": _now_iso(),
    })


def _on_heartbeat(characteristic: BleakGATTCharacteristic, data: bytearray) -> None:
    """Called when the QPG6200 sends a Heartbeat notification."""
    count = data[0] if data else 0
    log.info("BLE notify: Heartbeat count = %d", count)
    _publish(TOPIC_HEARTBEAT, {
        "count":     count,
        "timestamp": _now_iso(),
    })


# ---------------------------------------------------------------------------
#  BLE connection management
# ---------------------------------------------------------------------------

async def _scan_for_device(device_name: str) -> str | None:
    """
    Scan for the QPG6200 BLE peripheral by name.

    Returns the BLE address string if found, otherwise None.
    """
    log.info("Scanning for '%s' (timeout=%ds) …", device_name, SCAN_TIMEOUT_SEC)
    devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT_SEC)
    for device in devices:
        if device.name and device_name.lower() in device.name.lower():
            log.info("Found device: %s [%s]", device.name, device.address)
            return device.address
    log.warning("Device '%s' not found in scan", device_name)
    return None


async def _run_ble_connection(address: str, device_name: str) -> None:
    """
    Connect to the QPG6200, subscribe to notifications, and block until
    the connection is lost.
    """
    global ble_client_ref

    async with BleakClient(address) as client:
        ble_client_ref = client
        log.info("BLE connected to %s [%s]", device_name, address)
        _publish_status(connected=True, device_name=device_name)

        # Subscribe to Button State notifications
        await client.start_notify(BUTTON_STATE_UUID, _on_button_state)
        log.info("Subscribed to Button State notifications")

        # Subscribe to Heartbeat notifications
        await client.start_notify(HEARTBEAT_UUID, _on_heartbeat)
        log.info("Subscribed to Heartbeat notifications")

        log.info("Bridge active – forwarding BLE events to MQTT and vice-versa")
        log.info("  Button presses  → %s", TOPIC_BUTTON)
        log.info("  Heartbeat ticks → %s", TOPIC_HEARTBEAT)
        log.info("  LED commands    ← %s", TOPIC_LED)

        # Block until the BLE device disconnects
        while client.is_connected:
            await asyncio.sleep(1.0)

    ble_client_ref = None
    log.info("BLE connection to %s lost", device_name)
    _publish_status(connected=False, device_name=device_name)


async def _bridge_loop(device_name: str, broker: str, port: int) -> None:
    """
    Main bridge loop: scan → connect → forward → reconnect on disconnect.

    Runs indefinitely until a SIGINT/SIGTERM is received.
    """
    log.info("BLE-MQTT Bridge starting")
    log.info("  Target device : %s", device_name)
    log.info("  MQTT broker   : %s:%d", broker, port)
    log.info("  BLE service   : %s", DEMO_SERVICE_UUID)

    while True:
        try:
            address = await _scan_for_device(device_name)
            if address is None:
                log.info("Retrying scan in %ds …", RECONNECT_DELAY_SEC)
                await asyncio.sleep(RECONNECT_DELAY_SEC)
                continue

            await _run_ble_connection(address, device_name)

        except Exception as exc:
            log.error("BLE error: %s", exc)

        log.info("Reconnecting in %ds …", RECONNECT_DELAY_SEC)
        await asyncio.sleep(RECONNECT_DELAY_SEC)


# ---------------------------------------------------------------------------
#  Entry point
# ---------------------------------------------------------------------------

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="QPG6200 BLE IoT Demo – BLE to MQTT gateway bridge"
    )
    parser.add_argument(
        "--device", default=DEFAULT_DEVICE_NAME,
        help=f"BLE device name to connect to (default: '{DEFAULT_DEVICE_NAME}')"
    )
    parser.add_argument(
        "--broker", default=DEFAULT_MQTT_BROKER,
        help=f"MQTT broker hostname or IP (default: '{DEFAULT_MQTT_BROKER}')"
    )
    parser.add_argument(
        "--port", type=int, default=DEFAULT_MQTT_PORT,
        help=f"MQTT broker port (default: {DEFAULT_MQTT_PORT})"
    )
    parser.add_argument(
        "--debug", action="store_true",
        help="Enable verbose debug logging"
    )
    return parser.parse_args()


def main() -> None:
    global mqtt_client

    args = _parse_args()

    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)

    print("=" * 60)
    print("  QPG6200 BLE IoT Demo – Gateway Bridge")
    print("=" * 60)
    print(f"  BLE target  : {args.device}")
    print(f"  MQTT broker : {args.broker}:{args.port}")
    print(f"  Dashboard   : http://{args.broker}:1880/dashboard")
    print("=" * 60)
    print()

    # Set up MQTT first so status messages can be sent immediately
    mqtt_client = _setup_mqtt(args.broker, args.port)

    # Graceful shutdown on Ctrl-C / kill
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    def _shutdown(sig, frame):
        log.info("Shutdown signal received – stopping bridge")
        _publish_status(connected=False)
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
        for task in asyncio.all_tasks(loop):
            task.cancel()

    signal.signal(signal.SIGINT,  _shutdown)
    signal.signal(signal.SIGTERM, _shutdown)

    try:
        loop.run_until_complete(
            _bridge_loop(args.device, args.broker, args.port)
        )
    except asyncio.CancelledError:
        log.info("Bridge stopped")
    finally:
        loop.close()
        sys.exit(0)


if __name__ == "__main__":
    main()
