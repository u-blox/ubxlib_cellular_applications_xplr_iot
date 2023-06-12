# Cellular Tracking Application
The Cellular tracking application purpose is to periodically monitor the signal quality of the cellular environment, its location and scan the visible base stations if the `Button #2` is pressed.

All collected information is stored in the device filesystem via the application log but also shared into the cloud via the SARA-R5 MQTT embedded client to an MQTT Broker.

Once turned ON, by default the application monitors the cellular signal quality. Once there is a GNSS fix, the location is also published to the cloud. If the `Button #2` is pressed, a base station scan is initialized.

If the `Button #1` is pressed the application shuts down and the log file is saved and closed. If you do not press `Button #1` and simply turn off the XPLR-IoT-1 device then the log file will not save the entire log.

## Building the application
Use the do build script with the -e argument: `do -e cellular_tracker build`

## Configuring the application
Using the [config.h](config/config.h) file in the [config](config/) folder you will find the cellular URAT and APN settings, plus the MQTT credentials which are not checked into this repository. There is an [mqtt_credentials.h](config/mqtt_credentials.h) for you to edit and #include in the config.h file for providing the MQTT settings/credentials.

# NOTES
## Thingstream SIMS
Thingstream SIMs can be used with two APNS; TSUDP or TSIOT.

TSUDP is ONLY for MQTT-Anywhere service (using MQTT-SN), and does not allow any other internet traffic. This means when using TSUDP the NTP date/time request is not performed. This 'TSUDP' APN is listed as a 'restricted' APN in the [tasks/registrationTask.h](../tasks/registrationTask.h) file.

TSIOT can be used for normal internet services and as such should be used when using other MQTT brokers, or even other MQTT-SN gateways.