# Test results for V0.9 - PASS
MQTT examples all working. Only testing certificate and online test services.
Test run with DEBUG enabled to display the MQTT and Security Settings.

# MQTT examples
## MQTT-Anywhere (No security)
Uses "TSUDP" for the APN : [AT Log](MQTT_THINGSTREAM_ANYWHERE.txt)

## MQTT-Flex
Uses "TSIOT" for the APN : [AT Log](MQTT_THINGSTREAM_FLEX.txt)

# Mosquitto Test Service

## No TLS security, No Authentication
Uses "TSIOT" for the APN : [AT Log](MQTT_MOSQUITTO_NoTLS_NoAuth.txt)

## No TLS security, Username/Password Authentication
Uses "TSIOT" for the APN : [AT Log](MQTT_MOSQUITTO_NoTLS_Auth.txt)

## TLS security, Username/Password Authentication
Uses "TSIOT" for the APN : [AT Log](MQTT_MOSQUITTO_TLS_Auth.txt)

# Cell Scanning
Cell scanning finds a network and publishes it. The LED is the correct color and the operation can be cancelled by pressed Button #2 again.

# Location
Seen working, but not tested at this time.

# Logging
Log file is append <br/>
Log file can be displayed and deleted

# Shutdown
Shutdown is working on each test. 

# Drive test
Drive test is run in info logging mode, not debug.
