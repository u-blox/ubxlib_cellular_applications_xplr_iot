# Testing
This test folder is for holding the test reports and log files for each release.

# MQTT Testing
All MQTT examples should be tested, apart from private mqtt connections which require username/passwords.

Testing should involve MQTT brokers and MQTT-SN gateways, Thingstream Services and Mosquitto test server.

# Cell Scanning
Should show the cell scanning is operational and publishes the results.
Cell scanning should be able to be cancelled. The LED should turn blue and blip light blue.

# Location
Location task should be able to run in the right conditions and publish the location data.

# Logging
Check the log file is appended to the old log file. <br>
Check the log file can be displayed and deleted.

# Shutdown
The shutdown (Button #1) should operate and not 'hang' - test with other tests

# Drive testing
Show the application can operate in an expected drive test, using the normal logging mode (eINFO). Output should be signal quality and location JSON data being published.

# Out-of-Coverage
The XPLR device should be put in a position which causes it to go Out-of-Coverage. It should be left there for 5 minutes and then taken out. Describe what has happened in the test report, how long does it take to recover?
