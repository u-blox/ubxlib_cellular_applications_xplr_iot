# Test results for V0.92 - PASS
Test run with DEBUG enabled to display the MQTT and Security Settings.

# MQTT examples
MQTT examples all working. Only testing certificate and online test services.

## MQTT_FILE_SYSTEM
Output log shows the config file being loaded only, and it has the correct MQTT credentials.

## MQTT-Anywhere (No security)
Uses "TSUDP" for the APN <br> 
[Log](MQTT_THINGSTREAM_ANYWHERE.txt)

## MQTT-Flex
Uses "TSIOT" for the APN <br>
[Log](MQTT_THINGSTREAM_FLEX.txt)

# Mosquitto Test Service

## No TLS security, No Authentication
Uses "TSIOT" for the APN <br>
[Log](MQTT_MOSQUITTO_NoTLS_NoAuth.txt)

## No TLS security, Username/Password Authentication
Uses "TSIOT" for the APN <br>
[Log](MQTT_MOSQUITTO_NoTLS_Auth.txt)

## TLS security, Username/Password Authentication
Uses "TSIOT" for the APN<br>
[Log](MQTT_MOSQUITTO_TLS_Auth.txt)

# Cell Scanning
Cell scanning finds a network and publishes it. The LED is the correct color and the operation can be cancelled by pressed Button #2 again.<br>
[Log](CellScanning.txt)

# Location
Drive test shows the location is being published.

# Logging
Log file is append. <br/>
Log file can be displayed and deleted.
Log file can be created after it has been deleted.

# Shutdown
Shutdown is working on each test. 

# Drive test
Drive test is run in info logging mode, not debug.
Drive test from Cambourne, to Cambridge, back to Cambourne was taken. <br>Results are [here](DriveTest.txt). 
Log shows the device went out of coverage, but then the application didn't see that the connect was back again. Not sure if this is a SARA-R5 issue, a ubxlib issue, or a Cellular-Tracker app issue as of yet.
