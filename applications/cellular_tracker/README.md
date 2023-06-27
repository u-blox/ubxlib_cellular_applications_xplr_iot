# Cellular Tracking Application
The Cellular tracking application purpose is to periodically monitor the signal quality of the cellular environment, its location and scan the visible base stations if the `Button #2` is pressed.

All collected information is stored in the device filesystem via the application log but also shared into the cloud via the SARA-R5 MQTT embedded client to an MQTT Broker.

Once turned ON, by default the application monitors the cellular signal quality. Once there is a GNSS fix, the location is also published to the cloud. If the `Button #2` is pressed, a base station scan is initialized.

If the `Button #1` is pressed the application shuts down and the log file is saved and closed. If you do not press `Button #1` and simply turn off the XPLR-IoT-1 device then the log file will not save the entire log.

## Building the application
Use the do build script with the -e argument: `do -e cellular_tracker build`

## Configuring the application
Using the [config.h](config/config.h) file in the [config](config/) folder you will find the cellular URAT and APN settings. 

For the MQTT connection, there is a #define of the MQTT credentials which to use for this application. Please see the [mqttCredentials.c](src/mqtt_credentials.c) file for examples of MQTT broker settings.

# Application remote commands

The application can be remotely controlled through various topics which are subscribed to by the application tasks. The listed topics and their commands are here:

## <IMEI\>AppControl

### SET_DWELL_TIME <milliseconds\>
Sets the period between the main loop performing the location and signal quality measurements. Default is 5 seconds.

### SET_LOG_LEVEL <log level\>
Sets the logging level of the application. Default is '2' for INFO log level.

    0: TRACE
    1: DEBUG
    2: INFO
    3: WARN
    4: ERROR
    5: FATAL

It could be possible to increase the logging of an application remotely by changing the logging value from '2' to '1'.

## <IMEI\>CellScanControl

### START_CELL_SCAN
Starts a cell scan process, just as if you had pressed Button #2

# NOTES
## Thingstream SIMS
Thingstream SIMs can be used with two APNS; TSUDP or TSIOT.

TSUDP is ONLY for MQTT-Anywhere service (using MQTT-SN), and does not allow any other internet traffic. This means when using TSUDP the NTP date/time request is not performed. This 'TSUDP' APN is listed as a 'restricted' APN in the [tasks/registrationTask.h](../tasks/registrationTask.h) file.

TSIOT can be used for normal internet services and as such should be used when using other MQTT brokers, or even other MQTT-SN gateways.