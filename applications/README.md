# Applications

This page describes the various application this repository has available and how they generally operate.

<br />

## LED indicator
---
The XPLR-IoT-1 development platform has a RGB LED indicator on the front of the box. This application framework allows the application to set the LEDs independently, with various flashing and blinking modes. 

### Application startup LED indicator
The Boot, Registration, MQTT activity and cell scanning activity has the following LED statuses:

- Turn on: Solid RED 
- Option choice (Display/Delete log file): Solid Blue
- XPLR initialization: Flashing red
- Network Registration: Flashing Blue
- Connecting to MQTT: Flashing Green
- Operating: Green
- Shutdown: Solid Red (Button #1)
- Off: No LED (Safely turn off)

### Activity LED indicator

- Published MQTT message: Blip Green
- Cell Scan: Blue / Blip Blue


<br />

# Application List

(Currently this repository has only one application).

* [Cellular Tracker](cellular_tracker/).
  Publishes cellular signal strength parameters and location. Can be controlled to publish Cell Query results (+COPS=?)

* [...]()