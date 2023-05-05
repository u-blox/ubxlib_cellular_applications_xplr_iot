# Application Configuration
Users need to modify the [configFile.h](exampleConfigFile.h) file providing details of the MQTT broker to be used, and include this header file in the [config.h](config.h) file. It is commented out normally as this file should /not/ be shared or committed to the repository.
When this file is included the application will automatically save this file to the file system for later use. You can now delete/remove this file.

The `config.h` file also contains the APN which shall be updated accordingly for the SIM card used. Thingstream SIMs should have `TSIOT` configured for the APN.

In summary, to configure this application:-

 1. Edit the `exampleConfigFile.h` file which describes the MQTT broken name or IP address and the username/password. Rename it something like `myConfigFile.h`
 2. #include this `myConfigFile.h` file in the [config.h](config\config.h) file
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. Now delete the `myConfigFile.h` file, and comment out the #include for it. This is so it is not added  to the repository accidentially.
 5. Compile again and re-flash into the XPLR-IoT-1 device.
 6. The saved configuration will be loaded from the file system.