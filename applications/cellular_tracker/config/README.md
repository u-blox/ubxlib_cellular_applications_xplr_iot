# Application Configuration
Users need to modify the [mqtt_credentials.h](mqtt_credentials.h) file providing details of the MQTT broker to be used, and include this header file in the [config.h](config.h) file. It is commented out normally as this file should /not/ be shared or committed to the repository.
When this file is included the application will automatically save this file to the file system for later use. You can now delete/remove this file. This is so that a private configuration for the MQTT credentials can be kept private.

The `config.h` file contains the APN which shall be updated accordingly for the SIM card used. Thingstream SIMs should have `TSIOT` configured for the APN. Other items are for setting the UMNOPROFILE and URAT.

In summary, to configure this application:-

 1. Edit the `mqtt_credentials.h` file which describes the MQTT broken name or IP address and the username/password. 
 2. #include this `mqtt_credentials.h` file in the [config.h](config\config.h) file
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. Now delete the `mqtt_credentials.h` file, and comment out the #include for it. This is so it is not added  to the repository accidentially.
 5. Compile again and re-flash into the XPLR-IoT-1 device.
 6. The previously saved configuration will be loaded from the file system.