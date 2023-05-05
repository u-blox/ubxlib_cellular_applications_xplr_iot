# Cellular Tracking Application
The Cellular tracking application purpose is to periodically monitor the signal quality of the cellular environment, its location and scan the visible base stations if the `Button #2` is pressed.

All collected information is stored in the device filesystem via the application log but also shared into the cloud via the SARA-R5 MQTT embedded client to an MQTT Broker.

Once turned ON, by default the application monitors the cellular signal quality. Once there is a GNSS fix, the location is also published to the cloud. If the `Button #2` is pressed, a base station scan is initialized.

If the `Button #1` is pressed the application shuts down and the log file is saved and closed. If you do not press `Button #1` and simply turn off the XPLR-IoT-1 device then the log file will not save the entire log.