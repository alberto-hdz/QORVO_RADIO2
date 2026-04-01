# How to Upload JSON file to Node-RED
1. Open Node-RED Editor
2. Click the bar button on the top-right corner
3. Click Import
4. Select JSON file to import
5. Click Import on the bottom


# Node-RED is a visual programming environment for connecting IoT devices and dashboards. After this installation, you can customize the dashboard for any purpose.
1. Run the Node-RED installer for Linux (official script):
bash <(curl -sL https://github.com/node-red/linux-installers/releases/latest/download/update-nodejs-and-nodered-deb)
This script installs Node.js, Node-RED, and sets up the environment correctly.
2. Enable Node-RED to start automatically on boot:
sudo systemctl enable nodered.service
3. Start Node-RED manually for the first time:
node-red-start
You should see the Node-RED console logs and a message like:
Server now running at http://127.0.0.1:1880/
4. Access the editor from a PC on the same network:
Open a browser and navigate to: http://(raspberry-pi-ip):1880
