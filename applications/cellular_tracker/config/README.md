# Application Configuration
Users need to modify the [mqtt_credentials.c](../src/mqtt_credentials.c) file providing details of the MQTT broker to be used, and set the chosen configuration in the [config.h](config.h) file. 

When a mqtt configuration is selected the application will automatically save this file to the file system for later use. You can now delete/remove the specific credentials. This is so that a private configuration for the MQTT credentials can be kept private. You can now select the MQTT_FILE_SYSTEM #define. 

The `config.h` file contains the APN which shall be updated accordingly for the SIM card used. Thingstream SIMs should have `TSIOT` configured for the APN. Other items are for setting the UMNOPROFILE and URAT.

In summary, to configure this application:-

 1. Edit/Add to the `mqtt_credentials.c` file which describes the MQTT broken name or IP address and the username/password.
 2. Select this MQTT configuration using the #define in `config.h`
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. If required delete the private credential information in `mqtt_credentials.c` and set `MQTT_FILE_SYSTEM`.
 5. Compile again and re-flash into the XPLR-IoT-1 device.
 6. The previously saved configuration will be loaded from the file system.