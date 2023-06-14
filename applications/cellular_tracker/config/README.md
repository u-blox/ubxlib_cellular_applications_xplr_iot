# Application Configuration
## General settings
The `config.h` file contains the general settings for the cellular module's connection to the network.
The APN shall be configured accordingly for the SIM card used. Thingstream SIMs should use `TSIOT` for internet based MQTT brokers, and `TSUDP` for Thingstream MQTT-Anywhere service.

Other items are for the MNO PROFILE (+UMNOPROF) and Radio Access Technology (+URAT)

Finally, the MQTT broker needs to be selected from the list in the [mqtt_credentials.c](../src/mqtt_credentials.c) file.

## MQTT Credentials
Users might need to modify the [mqtt_credentials.c](../src/mqtt_credentials.c) file providing details of the MQTT broker to be used, and set the chosen configuration in the [config.h](config.h) file.

The application will automatically save the selected mqtt configuration to the file system for later use.
You can now delete/remove the specific credentials from the [mqtt_credentials.c](../src/mqtt_credentials.c) file. This is so that a private configuration for the MQTT credentials can be kept private.
Using the `MQTT_FILE_SYSTEM` definition will cause the application to load the mqtt credentials from the file system.

 1. Edit/Add to the `mqtt_credentials.c` file which describes the MQTT broken name or IP address and the username/password.
 2. Select this MQTT configuration using the #define in `config.h`
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. If required, delete the private credential information in `mqtt_credentials.c` and then select `MQTT_FILE_SYSTEM`.
    1. Compile again and re-flash into the XPLR-IoT-1 device.
    2. The previously saved configuration will be loaded from the file system.