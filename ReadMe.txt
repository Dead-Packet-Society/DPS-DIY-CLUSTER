DEAD PACKET SOCIETY
GENERIC ESP32-C5 CONTROLLER + SCANNER NODE
SETUP AND FLASHING GUIDE

Coded by Nightshade — Dead Packet Society

============================================================

1. WHAT THIS PROJECT IS
   ============================================================

The Dead Packet Society (DPS) field system uses:

```
1 x ESP32-C5 CONTROLLER
1-4 x ESP32-C5 SCANNER NODES
Android DPS Field Controller app
```

The communications path is:

```
Android App
     |
     | BLE
     v
ESP32-C5 CONTROLLER
     |
     | Wi-Fi TCP
     v
ESP32-C5 SCANNER NODES
```

The controller creates the WD-Hub Wi-Fi network.

The scanner nodes connect to WD-Hub and send scan observations
to the controller over TCP.

The Android application communicates with the controller over
Bluetooth Low Energy (BLE).

============================================================
2. IMPORTANT
============

This firmware is designed to be GENERIC ESP32-C5 firmware.

The controller does not require:

```
- LCD
- Display
- RGB LED
- Buttons
- XIAO-specific hardware
- Waveshare-specific hardware
- Heltec-specific hardware
```

The node firmware does not require:

```
- XIAO-specific GPIOs
- LCD
- Display
- RGB LED
- Buttons
- Other board-specific peripherals
```

Any ESP32-C5 board properly supported by the Arduino ESP32
core can be used, provided the board supports the Wi-Fi and
Bluetooth hardware and APIs required by this firmware.

============================================================
3. SOFTWARE REQUIRED
====================

Install the current Arduino IDE from the official Arduino
website:

https://www.arduino.cc/en/software/

============================================================
4. INSTALL THE ESP32 BOARD PACKAGE
==================================

Open Arduino IDE.

Go to:

```
File
  >
Preferences
```

Find:

```
Additional Boards Manager URLs
```

Add:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

If you already have other URLs installed, leave them in place
and separate multiple URLs as required by Arduino IDE.

Click:

```
OK
```

============================================================
5. INSTALL ESP32 BOARDS
=======================

Go to:

```
Tools
  >
Board
  >
Boards Manager
```

Search for:

```
esp32
```

Install:

```
esp32 by Espressif Systems
```

Use a current version that supports ESP32-C5.

============================================================
6. LIBRARIES REQUIRED
=====================

The DPS firmware uses:

```
WiFi
ArduinoJson
NimBLE
```

WiFi support is included with the Espressif ESP32 board package.

NimBLE support is included with the ESP32 Arduino environment.

ArduinoJson must be installed through the Arduino IDE Library
Manager.

Go to:

```
Sketch
  >
Include Library
  >
Manage Libraries
```

Search for:

```
ArduinoJson
```

Install:

```
ArduinoJson by Benoit Blanchon
```

Use a current ArduinoJson 7 release.

============================================================
7. DO NOT INSTALL DISPLAY LIBRARIES
===================================

The generic controller does NOT require:

```
Arduino_GFX_Library
Adafruit GFX
Adafruit ST7735
Adafruit ST7789
Adafruit ST77xx
U8g2
```

The generic node does NOT require display libraries.

These generic versions contain no LCD or display code.

============================================================
8. CONTROLLER FIRMWARE
======================

The controller firmware is:

```
Generic ESP32-C5 Controller
```

The controller is responsible for:

```
- Creating the WD-Hub Wi-Fi access point
- Accepting TCP connections from scanner nodes
- Managing up to four scanner nodes
- Receiving Wi-Fi observations
- Receiving BLE observations
- Forwarding observations to the Android application
- Accepting commands from the Android application
- Forwarding commands to scanner nodes
- Reporting node status
- Reporting node Wi-Fi MAC addresses
- Reporting node Bluetooth MAC addresses
- Providing BLE communication with the Android application
```

============================================================
9. CONTROLLER NETWORK SETTINGS
==============================

The default controller configuration is:

```
Wi-Fi SSID:
    WD-Hub

Wi-Fi Password:
    wardrive123

Controller IP:
    192.168.4.1

TCP Port:
    5000
```

Do not change the SSID or password unless you also change the
corresponding settings in the node firmware.

The controller IP address and TCP port are used by the scanner
nodes when connecting to the controller.

============================================================
10. CONTROLLER CONFIGURATION
============================

The controller's Wi-Fi name and password are configured in the
controller firmware before the controller is flashed.

The Android DPS Field Controller application also contains a
CONTROLLER SETTINGS section in the Config page.

This allows the Android application to store the controller
name and password for reference.

The available Android settings are:

```
Controller Name

Controller Password
```

IMPORTANT:

The Android Config page does NOT change the controller firmware.

The Android values are stored locally on the Android device.

They are intended to document the configuration that was used
when the controller was flashed.

The actual controller configuration is determined by the
firmware installed on the ESP32-C5 controller.

============================================================
10.1 CHANGING THE CONTROLLER NAME
=================================

Open the generic ESP32-C5 controller firmware in Arduino IDE.

Locate the controller name configuration.

The default controller name is:

```
WD-Hub
```

If you want to use a different controller name, change the
appropriate value in the controller firmware.

For example:

```
WD-Hub-01
```

After changing the controller name:

```
1. Save the controller firmware.

2. Select the correct ESP32-C5 board.

3. Select the correct COM port.

4. Upload the firmware to the controller.

5. Restart the controller.

6. Open Serial Monitor at 115200 baud.

7. Verify that the controller starts with the new
   configuration.
```

IMPORTANT:

If the controller's BLE advertised name is changed, the Android
application must use the corresponding BLE name when connecting
to the controller.

The default DPS controller BLE name is:

```
WD-Hub
```

============================================================
10.2 CHANGING THE CONTROLLER PASSWORD
=====================================

The controller creates a Wi-Fi access point for the scanner
nodes.

The default password is:

```
wardrive123
```

To change the password, modify the controller firmware before
flashing the controller.

After changing the password:

```
1. Save the controller firmware.

2. Upload the firmware to the controller.

3. Restart the controller.

4. Verify that the controller creates the Wi-Fi network.

5. Update the scanner node firmware with the same Wi-Fi
   password.

6. Reflash the affected scanner nodes.
```

IMPORTANT:

The scanner nodes must use the same Wi-Fi SSID and password
as the controller.

Changing the controller password without changing the node
firmware will prevent the nodes from connecting to WD-Hub.

============================================================
10.3 SAVING CONTROLLER CONFIGURATION IN THE ANDROID APP
=======================================================

Open the Android DPS Field Controller application.

Go to:

```
CONFIG
```

Find:

```
CONTROLLER SETTINGS
```

Enter:

```
Controller Name

Controller Password
```

The Android application saves these values locally.

The values remain saved when the application is closed and
reopened.

The controller password is masked while it is being entered.

IMPORTANT:

Saving these values in the Android application does NOT send
them to the controller.

It does NOT reconfigure the ESP32-C5.

It does NOT reflash the controller.

It is simply the Android application's stored record of the
controller configuration.

============================================================
10.4 CONTROLLER AND NODE CONFIGURATION MUST MATCH
=================================================

The controller and scanner nodes must use matching Wi-Fi
settings.

Example controller:

```
SSID:
    WD-Hub

Password:
    wardrive123
```

The nodes must use:

```
SSID:
    WD-Hub

Password:
    wardrive123
```

If you change either value:

```
1. Change the controller firmware.

2. Flash the controller.

3. Change the corresponding node firmware.

4. Reflash the affected nodes.

5. Verify that the nodes reconnect to the controller.
```

============================================================
10.5 RECOMMENDED CONTROLLER CONFIGURATION WORKFLOW
==================================================

When setting up a new DPS controller:

```
1. Open the generic controller firmware.

2. Set the desired controller name.

3. Set the desired controller Wi-Fi password.

4. Save the firmware.

5. Select the correct ESP32-C5 board.

6. Select the correct COM port.

7. Flash the controller.

8. Verify controller startup using Serial Monitor.

9. Update the scanner node firmware with the matching
   Wi-Fi configuration.

10. Flash the scanner nodes.

11. Open the Android DPS Field Controller application.

12. Open CONFIG.

13. Enter the controller name and password that were
    actually used when flashing the controller.
```

The Android application will save these values locally for
future reference.

============================================================
11. PREPARE THE CONTROLLER
==========================

Open the generic controller .ino file in Arduino IDE.

Go to:

```
Tools
  >
Board
```

Select the ESP32-C5 board that matches your controller.

The exact board name depends on the manufacturer and the
installed Espressif ESP32 board package.

If your specific board is not listed, select the appropriate
ESP32-C5 generic/dev-module option available for your hardware.

============================================================
12. CONTROLLER USB SETTINGS
===========================

The exact Arduino IDE Tools settings depend on the ESP32-C5
board being used.

Common options may include:

```
USB CDC On Boot
Flash Mode
Flash Size
Partition Scheme
Upload Mode
Port
```

Use the manufacturer's recommended settings for your specific
ESP32-C5 board.

If the board uses native USB CDC, enable:

```
USB CDC On Boot
```

when required.

Select the COM port belonging to the controller.

============================================================
13. UPLOAD THE CONTROLLER
=========================

Connect the ESP32-C5 controller to the computer with USB.

Select:

```
Tools
  >
Board
```

Choose the correct ESP32-C5 board.

Then select:

```
Tools
  >
Port
```

Choose the correct COM port.

Click:

```
Upload
```

Arduino IDE will compile and upload the controller firmware.

Some ESP32-C5 boards require the BOOT button to be held while
the board enters download mode.

If your board requires this, follow the manufacturer's procedure.

After the upload completes, open:

```
Tools
  >
Serial Monitor
```

Set the baud rate to:

```
115200
```

============================================================
14. CONTROLLER STARTUP
======================

A successful controller startup should show information similar
to:

```
DEAD PACKET SOCIETY
GENERIC ESP32-C5 CONTROLLER

[WiFi] SSID: WD-Hub
[WiFi] IP: 192.168.4.1
[WiFi] BSSID: xx:xx:xx:xx:xx:xx

[TCP] Server started on port 5000

[BLE] WD-Hub advertising

SYSTEM ONLINE
CONTROLLER: ESP32-C5
```

The MAC address will be different for every board.

============================================================
15. NODE FIRMWARE
=================

The node firmware is:

```
Generic ESP32-C5 Scanner Node
```

Each physical scanner must be assigned a unique NODE_ID before
the firmware is uploaded.

============================================================
16. NODE IDs
============

At the top of the node firmware you will find:

```
#define NODE_ID 4
```

Set this number according to the node being flashed.

For DPS-01:

```
#define NODE_ID 1
```

For DPS-02:

```
#define NODE_ID 2
```

For DPS-03:

```
#define NODE_ID 3
```

For DPS-04:

```
#define NODE_ID 4
```

IMPORTANT:

Never operate two nodes with the same NODE_ID.

Each node must have a unique ID.

============================================================
17. DEFAULT NODE ROLES
======================

The default configuration assigns a role based on NODE_ID.

DPS-01:

```
2.4 GHz Wi-Fi
Channels 1-7
```

DPS-02:

```
2.4 GHz Wi-Fi
Channels 8-13
```

DPS-03:

```
5 GHz Wi-Fi
Channels 36-165
```

DPS-04:

```
BLE scanning
```

The controller can also send configuration commands to change
a node's role and Wi-Fi channels.

============================================================
18. PREPARE A NODE
==================

Open the generic node .ino file in Arduino IDE.

Find:

```
#define NODE_ID 4
```

Change it to the desired node number.

Example:

```
#define NODE_ID 1
```

This creates:

```
DPS-01
```

Select the Arduino board that matches the ESP32-C5 hardware
being used.

The node does NOT need to be the same physical board as the
controller.

You can use different ESP32-C5 boards for different nodes.

============================================================
19. NODE WIFI SETTINGS
======================

The node firmware is configured for:

```
Wi-Fi SSID:
    WD-Hub

Wi-Fi Password:
    wardrive123

Controller IP:
    192.168.4.1

TCP Port:
    5000
```

These values must match the controller.

The node connects to the controller's Wi-Fi access point as a
Wi-Fi station/client.

============================================================
20. UPLOAD A NODE
=================

Connect the ESP32-C5 node to the computer with USB.

Select:

```
Tools
  >
Board
```

Choose the appropriate ESP32-C5 board.

Then select:

```
Tools
  >
Port
```

Choose the correct COM port.

Click:

```
Upload
```

If the board requires manual bootloader/download mode, use the
manufacturer's procedure.

After uploading, open:

```
Tools
  >
Serial Monitor
```

Set:

```
115200 baud
```

============================================================
21. NODE STARTUP
================

A successful node startup should show information similar to:

```
DEAD PACKET SOCIETY
GENERIC ESP32-C5 SCANNER NODE

NODE ID: DPS-01

[WiFi] Connecting to WD-Hub...
[WiFi] Connected
[WiFi] IP: 192.168.4.x

[TCP] Connecting to C5 controller...
[TCP] Connected to C5 controller

NODE ONLINE
```

The exact IP and MAC addresses will vary.

============================================================
22. FLASHING MULTIPLE NODES
===========================

To create four scanner nodes:

```
1. Open the generic node firmware.

2. Set:
       #define NODE_ID 1

3. Upload the firmware.

4. Disconnect the board.

5. Set:
       #define NODE_ID 2

6. Upload the firmware.

7. Disconnect the board.

8. Set:
       #define NODE_ID 3

9. Upload the firmware.

10. Disconnect the board.

11. Set:
       #define NODE_ID 4

12. Upload the firmware.
```

The resulting nodes are:

```
DPS-01
DPS-02
DPS-03
DPS-04
```

Each physical ESP32-C5 board retains its own hardware Wi-Fi
and Bluetooth MAC addresses.

============================================================
23. BRINGING UP THE COMPLETE SYSTEM
===================================

Recommended startup sequence:

```
1. Power on the ESP32-C5 controller.

2. Wait for:

       SYSTEM ONLINE

3. Power on the scanner nodes.

4. Each node connects to:

       WD-Hub

5. Each node establishes a TCP connection to:

       192.168.4.1:5000

6. The controller identifies each node.

7. Start the Android DPS Field Controller application.

8. Connect the Android application to the controller over
   BLE.

9. Request controller/node status.

10. Start scanning from the Android application.
```

============================================================
24. SERIAL MONITOR
==================

Both controller and node firmware provide diagnostic output.

Use:

```
115200 baud
```

Controller messages include:

```
[WiFi]
[TCP]
[BLE]
[CMD]
[NODE]
```

Node messages include:

```
[WiFi]
[TCP]
[CMD]
[CONFIG]
[SCAN]
```

============================================================
25. CONTROLLER BLE INFORMATION
==============================

The generic controller uses:

```
BLE Name:

    WD-Hub

Service UUID:

    12345678-1234-1234-1234-1234567890AB

Command UUID:

    12345678-1234-1234-1234-1234567890AC

Status UUID:

    12345678-1234-1234-1234-1234567890AD
```

The Android application must use these same UUIDs.

============================================================
26. CONTROLLER COMMANDS
=======================

The controller accepts commands from the Android application
including:

```
STATUS

NODES

START_ALL

START_DEF

STOP

NODE <node-id> <command>
```

============================================================
27. NODE COMMANDS
=================

Nodes accept JSON commands from the controller.

Supported commands include:

```
start

stop

start_def

status

set_node_config
```

============================================================
28. TROUBLESHOOTING — CONTROLLER
================================

CONTROLLER DOES NOT APPEAR OVER BLE

Check:

```
- Controller is powered.
- Serial Monitor shows BLE initialization.
- Serial Monitor shows advertising started.
- Android Bluetooth is enabled.
- Android has the required Bluetooth permissions.
```

The advertised device name is:

```
WD-Hub
```

CONTROLLER DOES NOT CREATE WD-HUB

Check:

```
- Controller firmware uploaded successfully.
- Serial Monitor is set to 115200.
- The board is actually running the new firmware.
- The selected board definition matches the hardware.
```

ANDROID CONNECTS BUT NO NODE DATA APPEARS

Check:

```
- Nodes are powered.
- Nodes connect to WD-Hub.
- Nodes receive a 192.168.4.x address.
- TCP connection to port 5000 succeeds.
- Node IDs are unique.
```

============================================================
29. TROUBLESHOOTING — NODES
===========================

NODE CANNOT CONNECT TO WD-HUB

Verify:

```
SSID:
    WD-Hub

Password:
    wardrive123
```

Also verify the controller is powered and its access point
is active.

NODE CONNECTS TO WIFI BUT NOT TCP

Verify:

```
Controller IP:
    192.168.4.1

TCP Port:
    5000
```

Check the controller Serial Monitor for incoming connections.

NODE APPEARS WITH THE WRONG ID

Check:

```
#define NODE_ID
```

Make sure the firmware was compiled and uploaded after changing
the value.

TWO NODES HAVE THE SAME ID

Disconnect both nodes.

Assign unique NODE_ID values.

Reflash the affected node.

Valid IDs:

```
1
2
3
4
```

============================================================
30. CHANGING NODE ROLES
=======================

The default roles are automatically assigned from NODE_ID.

Available roles are:

```
wifi24
wifi5
ble
```

The controller can provide channel lists when configuring a
Wi-Fi node.

============================================================
31. HARDWARE COMPATIBILITY
==========================

ESP32-C5 boards are not all physically identical.

Different manufacturers may use different:

```
- USB implementations
- Boot buttons
- Flash configurations
- Power circuits
- Pin assignments
- Antenna arrangements
- Board definitions
```

This generic DPS firmware intentionally avoids depending on
board-specific GPIO hardware.

If a particular ESP32-C5 board requires a special Arduino IDE
board setting to upload firmware, use that board manufacturer's
documentation for the appropriate setting.

============================================================
32. PROJECT ARCHITECTURE
========================

The DPS firmware is intentionally separated into two roles.

CONTROLLER

The controller handles:

```
- BLE communication with Android
- Wi-Fi access point
- TCP communications
- Node management
- Command routing
- Observation aggregation
- Node status
```

NODE

Each node handles:

```
- Wi-Fi scanning
- BLE scanning
- Observation reporting
- Heartbeats
- Node identification
- TCP communication with the controller
```

This allows scanner hardware to be replaced without redesigning
the controller architecture.

The controller does not need to know whether a node is a XIAO,
Waveshare, or another compatible ESP32-C5 board.

============================================================
33. QUICK START CHECKLIST
=========================

CONTROLLER:

```
[ ] Install Arduino IDE
[ ] Install Espressif ESP32 board package
[ ] Install ArduinoJson
[ ] Open generic controller firmware
[ ] Configure controller name/password if desired
[ ] Select ESP32-C5 board
[ ] Select correct COM port
[ ] Upload
[ ] Open Serial Monitor at 115200
[ ] Confirm WD-Hub appears
[ ] Confirm SYSTEM ONLINE
```

NODE:

```
[ ] Install Arduino IDE
[ ] Install Espressif ESP32 board package
[ ] Install ArduinoJson
[ ] Open generic node firmware
[ ] Set unique NODE_ID
[ ] Verify matching Wi-Fi settings
[ ] Select ESP32-C5 board
[ ] Select correct COM port
[ ] Upload
[ ] Open Serial Monitor at 115200
[ ] Confirm node connects to WD-Hub
[ ] Confirm TCP connection
[ ] Confirm NODE ONLINE
```

ANDROID APP:

```
[ ] Install DPS Field Controller
[ ] Connect to controller over BLE
[ ] Open CONFIG
[ ] Enter Controller Name
[ ] Enter Controller Password
[ ] Confirm values are saved
[ ] Verify controller/node status
[ ] Start scanning
```

============================================================
34. DEAD PACKET SOCIETY
=======================

Coded by Nightshade.

Dead Packet Society.

Respect the signal.
Hunt the packet.
Join the Society.

SYSTEM ONLINE
