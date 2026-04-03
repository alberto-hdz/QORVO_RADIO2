

**NODE-RED DOCUMENTATION**  
**IoT Gateway Dashboard System**  
QPG6200L IoT Mesh Network Project

| Document Version | 1.0 |
| :---- | :---- |
| **Date Created** | March 18, 2026 |
| **Classification** | Internal Technical Documentation |
| **Node-RED Version** | v4.1.7 |
| **Dashboard Package** | @flowfuse/node-red-dashboard |
| **Raspberry Pi IP** | 192.168.10.102 |
| **Dashboard URL** | http://192.168.10.102:1880/dashboard |
| **Editor URL** | http://192.168.10.102:1880 |

*This document explains how Node-RED is used in the QPG6200L IoT Mesh Network project, covering the editor interface, flow concepts, MQTT integration, Dashboard 2.0 configuration, and the complete doorbell and motion sensor flows currently deployed.*

# **Table of Contents**

1\.   What is Node-RED and Why We Use It

2\.   How Node-RED Fits Into the Project

3\.   The Node-RED Editor Interface

4\.   Core Concepts: Nodes, Flows, Wires, and Messages

5\.   MQTT Integration

6\.   Dashboard 2.0: Pages, Groups, and Widgets

7\.   The Doorbell Flow — Complete Walkthrough

8\.   The Motion Sensor Flow — Complete Walkthrough

9\.   Testing Flows Without Hardware

10\.  Adding New Devices in the Future

11\.  Troubleshooting Common Issues

12\.  Quick Reference

# **1\.  What is Node-RED and Why We Use It**

## **1.1  What is Node-RED?**

Node-RED is a visual flow-programming tool originally developed by IBM and now maintained as an open-source project under the OpenJS Foundation. It runs as a web application on the Raspberry Pi and is accessed through any browser on the same network. Instead of writing traditional code, you build programs by dragging and dropping nodes onto a canvas and connecting them with wires to define how data flows between them.

Node-RED is built on top of Node.js, which means it is fast, event-driven, and well-suited for IoT applications that need to react to incoming data in real time.

## **1.2  Why Node-RED for This Project?**

* **Built-in MQTT support:** Node-RED has native MQTT input and output nodes. Connecting to the Mosquitto broker running on the same Raspberry Pi requires only a server address and topic — no custom networking code needed.

* **Dashboard 2.0:** The @flowfuse/node-red-dashboard package adds drag-and-drop UI widgets (text displays, gauges, charts, buttons, notifications) that automatically update in real time when MQTT messages arrive.

* **Visual debugging:** You can see messages flowing through your nodes in real time using the debug panel. This makes testing and troubleshooting much faster than adding print statements to code.

* **No recompilation:** Changes to flows are deployed instantly with a single click. You do not need to recompile or reflash anything on the Pi to update the dashboard behavior.

* **Runs on the Pi:** Node-RED runs directly on the Raspberry Pi gateway, so the entire stack (OTBR, MQTT broker, Node-RED) is self-contained on one device.

## **1.3  Node-RED Version in This Project**

| Node-RED Version | v4.1.7 |
| :---- | :---- |
| **Node.js Version** | v20 (installed via Node-RED installer) |
| **Dashboard Package** | @flowfuse/node-red-dashboard |
| **Service Name** | nodered.service (systemd) |
| **Auto-start on boot** | Enabled |
| **Editor Port** | 1880 |
| **Flow Storage Location** | /home/pi/.node-red/flows.json |

# **2\.  How Node-RED Fits Into the Project**

## **2.1  Position in the Stack**

Node-RED is the final layer in the gateway stack. Data originates in the QPG6200L devices, travels over Thread to the Raspberry Pi, gets routed through MQTT, and arrives in Node-RED where it is processed and displayed.

|   QPG6200L Device  (e.g. Motion Sensor)   \- Reads sensor data   \- Publishes MQTT message to topic  qorvo/motion   \- Payload: {"distance": 42, "motion": true} |
| :---- |

                    ↓  Thread mesh  →  nRF52840 dongle  →  OTBR on Raspberry Pi  ↓

|   Mosquitto MQTT Broker  (port 1883, on Raspberry Pi)   \- Receives published message on topic  qorvo/motion   \- Forwards to all subscribers of that topic |
| :---- |

                    ↓  MQTT subscribe  ↓

|   Node-RED  (port 1880, on Raspberry Pi)   \- mqtt in node subscribed to  qorvo/motion  receives the message   \- Flow processes and transforms the data   \- Dashboard 2.0 widget updates in real time |
| :---- |

                    ↓  HTTP / WebSocket  ↓

|   Browser on any device   \- http://192.168.10.102:1880/dashboard   \- Shows live sensor data and doorbell alerts |
| :---- |

## **2.2  What Node-RED is Responsible For**

* **Subscribing to MQTT topics:** Each device has a dedicated MQTT topic. Node-RED listens to all of them simultaneously.

* **Processing incoming data:** Parsing JSON payloads, formatting values, and routing data to the correct dashboard widgets.

* **Displaying the dashboard:** Serving the web-based dashboard UI to the tablet browser over HTTP.

* **Triggering timed actions:** For example, the doorbell alert shows for 3 seconds then resets to Idle automatically.

## **2.3  What Node-RED is NOT Responsible For**

* **Thread networking:** That is handled by OTBR and the nRF52840 dongle.

* **MQTT brokering:** That is handled by Mosquitto. Node-RED only subscribes to topics, it does not run the broker.

* **Device firmware:** The QPG6200L devices run their own firmware. Node-RED has no direct connection to them.

# **3\.  The Node-RED Editor Interface**

## **3.1  Accessing the Editor**

The Node-RED editor runs on the Raspberry Pi and is accessed from any browser on the TRENDnet network. Make sure your laptop or tablet is connected to TRENDnet827\_2.4GHz\_QMGR, then navigate to:

http://192.168.10.102:1880

**WARNING: The editor has no password protection. Anyone on the same network can access and modify flows. Do not expose this network publicly.**

## **3.2  Editor Layout**

The editor is divided into four main areas:

| Area | Location | What It Does |
| :---- | :---- | :---- |
| Palette | Left side panel | Contains all available node types organized into categories. Drag nodes from here onto the canvas. |
| Canvas | Centre (main area) | Where you build your flows by placing and connecting nodes. Each tab at the top is a separate flow. |
| Right Panel | Right side panel | Context-sensitive panel showing node info, debug messages, Dashboard 2.0 configuration, and more. Switch between views using the dropdown arrow at the top. |
| Top Toolbar | Top of page | Contains the Deploy button (red), hamburger menu (three lines), and notification indicators. |

## **3.3  The Right Panel Tabs**

Click the dropdown arrow on the right side panel to switch between these views:

* **Information:** Shows documentation and details about the currently selected node.

* **Help:** Node-RED help and keyboard shortcuts.

* **Debug messages:** Shows output from debug nodes in real time. This is where you see msg.payload values when testing flows.

* **Configuration nodes:** Shows all shared configuration nodes such as MQTT server connections.

* **Context data:** Shows flow and global context variables if used.

* **Dashboard 2.0:** Shows and manages your dashboard pages, groups, and widgets. This is where you add new pages and groups.

## **3.4  The Deploy Button**

The red Deploy button in the top right corner publishes your changes. Until you click Deploy, any changes you make to the canvas exist only in the browser and are not running on the Pi.

When you click Deploy, Node-RED will ask you to confirm if there are any nodes with configuration warnings. You can safely confirm and deploy — warnings about unconfigured nodes do not prevent other correctly configured nodes from running.

TIP: Deploy often. If something is not working as expected after making a change, always check that you clicked Deploy first.

# **4\.  Core Concepts: Nodes, Flows, Wires, and Messages**

## **4.1  Nodes**

A node is the basic building block in Node-RED. Every node does one specific thing — receive a message, transform it, send it somewhere, display it, delay it, etc. Nodes appear as rectangular blocks on the canvas.

Every node has:

* **An input port:** The grey dot on the LEFT side of the node. This is where messages enter the node.

* **One or more output ports:** The grey dot(s) on the RIGHT side of the node. This is where messages leave after being processed.

* **A configuration panel:** Opened by double-clicking the node. This is where you set the node's behavior (topic, server, label, etc.).

Node status is shown by a small colored indicator below the node:

| Green dot / connected | Node is running and connected (e.g. MQTT node connected to broker) |
| :---- | :---- |
| **Red dot / error** | Node has an error or cannot connect |
| **Yellow dot / connecting** | Node is trying to connect |
| **Red triangle** | Node is not configured correctly — double click to fix |
| **No indicator** | Node is idle and waiting |

## **4.2  Flows**

A flow is a tab on the canvas. You can have multiple flows, each on its own tab. In this project there is currently one flow called Flow 1 which contains all the doorbell and motion sensor logic. You can rename a flow by double-clicking its tab name.

## **4.3  Wires**

A wire is a connection between the output port of one node and the input port of another. Wires define the path that messages travel. To create a wire, click and hold the grey dot on the right side of a node, drag to the grey dot on the left side of another node, and release.

One output port can connect to multiple input ports — just drag multiple wires from the same output dot. All connected nodes will receive a copy of the message.

TIP: To delete a wire, click on it to select it (it turns orange) then press the Delete key on your keyboard.

## **4.4  Messages**

Everything that flows through Node-RED is a message object, referred to in code as msg. A message is a JavaScript object that always has at least one property:

* **msg.payload:** The main data being carried. This is what most nodes read and write. For MQTT messages this is the raw content of the MQTT message.

* **msg.topic:** The MQTT topic the message came from (e.g. qorvo/doorbell).

* **msg.qos:** The MQTT Quality of Service level.

When a QPG6200L device publishes this MQTT message:

Topic:   qorvo/motion

Payload: {"distance": 42, "motion": true}

Node-RED receives it as:

msg.topic   \= "qorvo/motion"

msg.payload \= {"distance": 42, "motion": true}

After the JSON node parses it, you can access individual fields using:

msg.payload.distance  →  42

msg.payload.motion    →  true

## **4.5  Node Types Used in This Project**

| Node Name | Category | What It Does in This Project |
| :---- | :---- | :---- |
| mqtt in | network | Subscribes to an MQTT topic and outputs a message every time a new MQTT message arrives on that topic. |
| change | function | Modifies msg.payload. Used in the doorbell flow to set the payload to the string DOORBELL PRESSED. |
| trigger | function | Sends one message immediately and a second message after a time delay. Used to reset the doorbell display back to Idle after 3 seconds. |
| json | function | Converts the incoming payload string into a JavaScript object so individual fields like .distance and .motion can be accessed. |
| ui-text | dashboard 2 | Displays a text value on the dashboard. Used for doorbell status, motion status, and distance display. |
| ui-button | dashboard 2 | Displays a clickable button on the dashboard. Used during initial testing. |
| debug | common | Prints msg.payload to the debug panel. Useful for checking what data is arriving before wiring to a dashboard widget. |

# **5\.  MQTT Integration**

## **5.1  How Node-RED Connects to Mosquitto**

Node-RED connects to the Mosquitto MQTT broker running on the same Raspberry Pi. The connection is configured once as a shared server configuration node that all mqtt in and mqtt out nodes in the project reuse.

| Broker Host | 192.168.10.102  (the Raspberry Pi itself) |
| :---- | :---- |
| **Broker Port** | 1883 |
| **Protocol** | MQTT v3.1.1 |
| **Authentication** | None  (anonymous access enabled) |
| **Client ID** | Auto-generated by Node-RED |

*NOTE: Because Node-RED and Mosquitto are both running on the same Raspberry Pi, the broker host address is the Pi's own IP address 192.168.10.102. If you ever change the Pi's IP address you will need to update this in the MQTT server configuration node.*

## **5.2  MQTT Topics Used in This Project**

| Device | MQTT Topic | Payload Format |
| :---- | :---- | :---- |
| Doorbell | qorvo/doorbell | 1  (simple integer, sent when button pressed) |
| Motion Sensor | qorvo/motion | {"distance": 42, "motion": true}  (JSON object) |
| Speaker | qorvo/speaker | TBD — not yet implemented |
| Microphone | qorvo/mic | TBD — not yet implemented |

## **5.3  How to Update the MQTT Server Configuration**

If you ever need to change the broker address or port, you do not need to update every mqtt node individually. The server is stored as a shared configuration node. To update it:

1. In the Node-RED editor, click the dropdown arrow on the right panel and select Configuration nodes.

2. Find the MQTT broker entry showing 192.168.10.102:1883 and double click it.

3. Update the Host or Port field as needed.

4. Click Update, then Deploy.

All mqtt in and mqtt out nodes in the project will automatically use the updated server.

## **5.4  How to Add a New MQTT Topic**

When a new QPG6200L device is ready to send data, adding it to Node-RED is straightforward:

5. Drag a new mqtt in node onto the canvas.

6. Double click it. In the Server field select the existing 192.168.10.102 server.

7. Set the Topic field to the new device's topic (e.g. qorvo/speaker).

8. Wire it to processing nodes and dashboard widgets.

9. Click Deploy.

# **6\.  Dashboard 2.0: Pages, Groups, and Widgets**

## **6.1  Dashboard Structure**

Dashboard 2.0 organizes the UI into a hierarchy. You must create at least one Page and one Group before any widget will display. The structure is:

|   Dashboard   └── Page  (e.g. IoT Dashboard)  — a screen or tab in the dashboard       ├── Group  (e.g. Doorbell)  — a labelled section on the page       │   └── Widgets  (ui-text, ui-button, ui-gauge, etc.)       └── Group  (e.g. Motion Sensor)           └── Widgets |
| :---- |

In this project the current structure is:

| Level | Name | Contents |
| :---- | :---- | :---- |
| Page | IoT Dashboard | The main dashboard screen accessible at /dashboard |
| Group | Doorbell | Contains the doorbell status text widget |
| Group | Motion Sensor | Contains the motion status and distance text widgets |

## **6.2  How to Add a New Page**

10. Click the dropdown arrow on the right panel and select Dashboard 2.0.

11. Click the \+page button at the top of the panel.

12. Enter a name for the page and click Add.

13. The new page will appear as a separate tab in the dashboard.

## **6.3  How to Add a New Group**

14. In the Dashboard 2.0 panel, find the page you want to add a group to.

15. Click the \+group button under that page.

16. Enter a group name, confirm the page is correct, and click Add.

Alternatively, when configuring a widget node, click the \+ button next to the Group dropdown to create a new group on the fly.

## **6.4  Widget Types Available**

| Widget Node | What It Displays | Use Case in This Project |
| :---- | :---- | :---- |
| ui-text | A text label and value | Doorbell status, motion status, distance reading |
| ui-button | A clickable button | Testing flows (used during initial setup) |
| ui-gauge | A circular or arc gauge | Could display distance as a visual gauge |
| ui-chart | A line or bar chart over time | Could plot distance readings over time |
| ui-slider | A draggable slider | Could control speaker volume |
| ui-switch | An on/off toggle | Could enable/disable motion alerts |
| ui-notification | A popup notification banner | Alternative to text for doorbell alerts |
| ui-markdown | Formatted markdown text | Status summaries or labels |

## **6.5  Accessing the Dashboard**

The dashboard is a separate URL from the editor. Share this URL with anyone who needs to view the sensor data:

http://192.168.10.102:1880/dashboard

The dashboard updates in real time via WebSocket. When a new MQTT message arrives and flows through Node-RED, the widget on the dashboard updates within milliseconds without needing to refresh the browser.

# **7\.  The Doorbell Flow — Complete Walkthrough**

## **7.1  What the Doorbell Flow Does**

When the QPG6200L doorbell device detects a button press, it publishes an MQTT message to the topic qorvo/doorbell with payload 1\. The Node-RED doorbell flow receives this message, displays DOORBELL PRESSED on the dashboard in red, and automatically resets to Idle after 3 seconds.

## **7.2  Flow Diagram**

|   \[Doorbell Input\]        \[Format Doorbell\]      \[Reset After 3s\]       \[Doorbell Status\]    mqtt in node    ──►   change node     ──►   trigger node    ──►    ui-text node    topic:                  sets payload to        sends payload          displays on    qorvo/doorbell          DOORBELL PRESSED       then Idle after 3s     dashboard |
| :---- |

## **7.3  Node-by-Node Explanation**

### **Node 1 — Doorbell Input (mqtt in)**

This node is subscribed to the MQTT topic qorvo/doorbell. Every time the doorbell firmware publishes to that topic, this node receives the message and passes it to the next node in the flow. The msg.payload at this point contains the raw value 1\.

| Node type | mqtt in |
| :---- | :---- |
| **Server** | 192.168.10.102 |
| **Topic** | qorvo/doorbell |
| **QoS** | 0 |
| **Name** | Doorbell Input |

### **Node 2 — Format Doorbell (change)**

The change node replaces msg.payload with the string DOORBELL PRESSED. The raw payload value of 1 from the MQTT message is not meaningful for display purposes, so this node converts it into a readable string before passing it to the trigger node.

| Node type | change |
| :---- | :---- |
| **Rule** | Set msg.payload |
| **Set to** | DOORBELL PRESSED |
| **Value type** | string |
| **Name** | Format Doorbell |

### **Node 3 — Reset After 3s (trigger)**

The trigger node is the key to the doorbell's timed reset behavior. When it receives a message it immediately sends DOORBELL PRESSED downstream. Then it waits 3 seconds and automatically sends Idle downstream. This causes the dashboard to show the alert for exactly 3 seconds before reverting to the idle state — without any additional input from the device.

| Node type | trigger |
| :---- | :---- |
| **Send (first)** | DOORBELL PRESSED |
| **Wait** | 3 seconds |
| **Then send** | Idle |
| **Name** | Reset After 3s |

### **Node 4 — Doorbell Status (ui-text)**

This ui-text node displays the value of msg.payload on the IoT Dashboard page inside the Doorbell group. When it receives DOORBELL PRESSED it shows that text. When it receives Idle 3 seconds later it updates to show Idle.

| Node type | ui-text |
| :---- | :---- |
| **Group** | Doorbell |
| **Label** | Status |
| **Value** | {{msg.payload}} |
| **Name** | Doorbell Status |

## **7.4  Complete Message Trace**

Here is exactly what happens from button press to dashboard update:

17. QPG6200L doorbell firmware detects button press.

18. Firmware publishes MQTT message: topic \= qorvo/doorbell, payload \= 1\.

19. Mosquitto broker receives the message and forwards it to all subscribers.

20. Node-RED mqtt in node (Doorbell Input) receives the message. msg.payload \= 1\.

21. change node (Format Doorbell) sets msg.payload \= DOORBELL PRESSED.

22. trigger node (Reset After 3s) immediately sends msg.payload \= DOORBELL PRESSED downstream.

23. ui-text node (Doorbell Status) updates the dashboard to show DOORBELL PRESSED.

24. 3 seconds pass.

25. trigger node automatically sends msg.payload \= Idle downstream.

26. ui-text node (Doorbell Status) updates the dashboard to show Idle.

# **8\.  The Motion Sensor Flow — Complete Walkthrough**

## **8.1  What the Motion Sensor Flow Does**

When the QPG6200L motion sensor device takes a distance reading, it publishes a JSON MQTT message to the topic qorvo/motion containing the distance in centimeters and a boolean indicating whether motion was detected. Node-RED receives this, parses the JSON, and updates two widgets on the dashboard — one showing the motion status and one showing the distance value.

## **8.2  Flow Diagram**

|                                          ──►  \[Motion Status\]   ui-text   \[Motion Input\]      \[Parse JSON\]      /    displays motion true/false    mqtt in node  ──►  json node   ──►    topic:                              \\    qorvo/motion                         ──►  \[Distance Display\]  ui-text                                              displays distance in cm |
| :---- |

## **8.3  Node-by-Node Explanation**

### **Node 1 — Motion Input (mqtt in)**

This node subscribes to the MQTT topic qorvo/motion. When the motion sensor firmware publishes a reading, this node receives it. The msg.payload at this point is a raw JSON string such as {"distance": 42, "motion": true}. It is a string at this stage, not yet a JavaScript object, which is why the next node is needed.

| Node type | mqtt in |
| :---- | :---- |
| **Server** | 192.168.10.102 |
| **Topic** | qorvo/motion |
| **QoS** | 0 |
| **Name** | Motion Input |

### **Node 2 — Parse JSON (json)**

The json node converts the incoming payload string into a JavaScript object. This is a critical step. Without this node, when you try to access msg.payload.distance you get undefined because msg.payload is still a string and not an object with properties. After this node runs, msg.payload.distance returns 42 and msg.payload.motion returns true.

| Node type | json |
| :---- | :---- |
| **Action** | Always convert to JavaScript Object |
| **Name** | Parse JSON |

### **Node 3 — Motion Status (ui-text)**

This ui-text node displays whether motion was detected. The Value field uses a mustache template expression to access the motion property of the parsed JSON object. The dashboard shows the raw boolean value true or false. In the future this can be updated to show Motion Detected or No Motion using a function node or a switch node.

| Node type | ui-text |
| :---- | :---- |
| **Group** | Motion Sensor |
| **Label** | Status |
| **Value** | {{msg.payload.motion}} |
| **Name** | Motion Status |

### **Node 4 — Distance Display (ui-text)**

This ui-text node displays the numeric distance reading from the sensor. The Value field accesses msg.payload.distance and appends the unit cm. The Parse JSON node output connects to both this node and the Motion Status node simultaneously — both receive the same message.

| Node type | ui-text |
| :---- | :---- |
| **Group** | Motion Sensor |
| **Label** | Distance |
| **Value** | {{msg.payload.distance}} cm |
| **Name** | Distance Display |

## **8.4  Complete Message Trace**

27. QPG6200L motion sensor firmware takes a distance reading (e.g. 42 cm, motion \= true).

28. Firmware publishes MQTT message: topic \= qorvo/motion, payload \= {"distance": 42, "motion": true}.

29. Mosquitto broker receives and forwards the message.

30. Node-RED mqtt in node (Motion Input) receives it. msg.payload \= "{"distance": 42, "motion": true}"  (string).

31. json node (Parse JSON) converts msg.payload to a JavaScript object.

32. msg.payload.distance \= 42, msg.payload.motion \= true.

33. Both Motion Status and Distance Display ui-text nodes receive the message simultaneously.

34. Motion Status widget updates to show: Status: true

35. Distance Display widget updates to show: Distance: 42 cm

# **9\.  Testing Flows Without Hardware**

## **9.1  Using mosquitto\_pub to Simulate Devices**

You can simulate MQTT messages from any QPG6200L device using the mosquitto\_pub command line tool on the Raspberry Pi. This lets you test and verify Node-RED flows before the physical device firmware is ready.

SSH into the Pi from your laptop:

ssh pi@192.168.10.102

## **9.2  Test the Doorbell**

Run this command to simulate a doorbell press:

mosquitto\_pub \-h 192.168.10.102 \-t qorvo/doorbell \-m "1"

Expected result on dashboard: Status changes to DOORBELL PRESSED, then returns to Idle after 3 seconds.

## **9.3  Test the Motion Sensor**

Run this command to simulate a motion detection event with distance 45 cm:

mosquitto\_pub \-h 192.168.10.102 \-t qorvo/motion \-m '{"distance": 45, "motion": true}'

Expected result on dashboard: Status shows true, Distance shows 45 cm.

Run this command to simulate no motion:

mosquitto\_pub \-h 192.168.10.102 \-t qorvo/motion \-m '{"distance": 120, "motion": false}'

Expected result: Status shows false, Distance shows 120 cm.

## **9.4  Using the Debug Node**

If a widget is not updating as expected, add a debug node to check what data is actually flowing through your nodes:

36. Drag a debug node from the left palette (under common) onto the canvas.

37. Wire it to the output of any node you want to inspect.

38. Click Deploy.

39. Click the dropdown arrow on the right panel and select Debug messages.

40. Trigger the flow (e.g. run a mosquitto\_pub command).

41. The debug panel will show exactly what msg.payload contains at that point in the flow.

TIP: Debug nodes are the single most useful tool for troubleshooting. Always add one when something is not working as expected.

# **10\.  Adding New Devices in the Future**

## **10.1  Overview**

When the speaker and microphone QPG6200L devices are ready to send MQTT data, adding them to the Node-RED dashboard follows the same pattern as the existing flows. Here is the general process.

## **10.2  Step-by-Step: Adding a New Device**

42. Decide on the MQTT topic for the new device (e.g. qorvo/speaker or qorvo/mic).

43. Add that exact topic to the QPG6200L device firmware MQTT publish call.

44. In the Node-RED editor, drag a new mqtt in node onto the canvas.

45. Double click it, select the existing 192.168.10.102 server, set the topic, and click Done.

46. Decide what processing is needed (JSON parsing, value formatting, etc.) and add the appropriate nodes.

47. In the Dashboard 2.0 panel, add a new Group under the IoT Dashboard page for the new device.

48. Add appropriate ui-text, ui-gauge, or other widget nodes and assign them to the new group.

49. Wire all the nodes together and click Deploy.

50. Test using mosquitto\_pub before relying on the actual device.

## **10.3  Planned Future Devices**

| Device | MQTT Topic | Planned Dashboard Widgets |
| :---- | :---- | :---- |
| Speaker | qorvo/speaker | Playback status indicator, volume level display |
| Microphone | qorvo/mic | Audio level gauge, recording status indicator |

# **11\.  Troubleshooting Common Issues**

## **11.1  Dashboard Shows 'Cannot GET /dashboard'**

**Cause:** No pages or groups have been created in Dashboard 2.0, or no widgets have been deployed.

**Fix:** Open the editor at http://192.168.10.102:1880, click the Dashboard 2.0 panel on the right, create a Page and a Group, add at least one widget node, assign it to the group, and click Deploy.

## **11.2  Dashboard Shows \[object Object\]**

**Cause:** The MQTT payload is a JSON string but has not been parsed by a json node. The ui-text node is trying to display the entire object as text.

**Fix:** Add a json node between the mqtt in node and the ui-text node. Set its Action to Always convert to JavaScript Object.

## **11.3  MQTT Node Shows Red Dot / Disconnected**

**Cause:** Node-RED cannot connect to the Mosquitto broker.

**Fix:** SSH into the Pi and check that Mosquitto is running:

sudo systemctl status mosquitto

If it is not running restart it:

sudo systemctl restart mosquitto

Also verify the MQTT server configuration node in Node-RED shows host 192.168.10.102 and port 1883\.

## **11.4  Doorbell Alert Does Not Reset After 3 Seconds**

**Cause:** The trigger node is not configured correctly. The wait time may be set to milliseconds instead of seconds.

**Fix:** Double click the trigger node and confirm: wait for \= 3, unit dropdown \= Seconds, then send \= Idle (string).

## **11.5  Changes Not Appearing on Dashboard After Editing**

**Cause:** The Deploy button was not clicked after making changes.

**Fix:** Always click the red Deploy button after any change. Look for the green Successfully deployed notification at the top.

## **11.6  Node-RED Not Accessible**

**Cause:** The nodered service is not running on the Pi.

**Fix:** SSH into the Pi and run:

sudo systemctl status nodered

sudo systemctl restart nodered

# **12\.  Quick Reference**

## **12.1  Key URLs**

| Node-RED Editor | http://192.168.10.102:1880 |
| :---- | :---- |
| **Dashboard** | http://192.168.10.102:1880/dashboard |
| **SSH to Pi** | ssh pi@192.168.10.102 |

## **12.2  MQTT Topics**

| Device | Topic | Example Payload |
| :---- | :---- | :---- |
| Doorbell | qorvo/doorbell | 1 |
| Motion Sensor | qorvo/motion | {"distance": 42, "motion": true} |
| Speaker | qorvo/speaker | TBD |
| Microphone | qorvo/mic | TBD |

## **12.3  Test Commands**

mosquitto\_pub \-h 192.168.10.102 \-t qorvo/doorbell \-m "1"

mosquitto\_pub \-h 192.168.10.102 \-t qorvo/motion \-m '{"distance": 45, "motion": true}'

## **12.4  Service Management Commands**

sudo systemctl status nodered          \# Check Node-RED status

sudo systemctl restart nodered         \# Restart Node-RED

sudo systemctl status mosquitto        \# Check MQTT broker status

sudo systemctl restart mosquitto       \# Restart MQTT broker

## **12.5  Node Type Quick Reference**

| Node | Palette Section | Primary Use |
| :---- | :---- | :---- |
| mqtt in | network | Subscribe to MQTT topic, receive incoming device messages |
| mqtt out | network | Publish to MQTT topic, send commands to devices |
| json | function | Parse JSON string into JavaScript object |
| change | function | Set, change, or delete message properties |
| trigger | function | Send timed sequence of messages |
| debug | common | Print msg.payload to debug panel for inspection |
| ui-text | dashboard 2 | Display text value on dashboard |
| ui-gauge | dashboard 2 | Display numeric value as visual gauge |
| ui-chart | dashboard 2 | Display values as chart over time |
| ui-button | dashboard 2 | Add clickable button to dashboard |

*\--- End of Document \---*  
Node-RED Documentation v1.0  |  QPG6200L IoT Mesh Network Project  |  March 18, 2026