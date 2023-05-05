# ubxlib cellular applications for XPLR-IoT-1 development kit
The purpose of this repository is to provide example applications which run on the XPLR-IoT-1 development kit. This repository is forked from the [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) repository and is structured very much the same. A simple **do build** script is used to select the application to build and flash on to the XPLR-IoT-1 development kit.

The [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) project contains small examples for individual features of the ubxlib API, whereas this `cellular applications` project is for complete applications based on the ubxlib API.

Please see the [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) repository for further information about this project.

# Applications
Currently this repository has only one application, the cellular tracking application. This application is described [here](applications/cellular_tracker/README.md).

# Application framework
The design of these applications is based around the same design. Each application has access to the following functions:

## Application tasks
Each application is built from a number of `appTasks` which either run in their own loop thread or their event executed separately.

Each `appTask` is based on the same 'boiler plate' design.
Each application task has a `taskMutex`, `taskEventQueue` and `taskHandler` which are provided by the UBXLIB API.

The `main()` function simply runs the `appTask's` loop or commands an `appTaks` to run individually at certain timing, or possibly other events.

## Task Event Queue
Each `appTask` has an event queue for sending commands to it. The commands are listed in the `appTask's` .h file

## MQTT communication
Each `appTask` has a `/<IMEI>/<appTaskName>Control` topic where commands can be sent down to that `appTask`. Start, Stop task, measure 'now' etc.

Each appTask can publish their data/information to the MQTT broker to their own specific topic. This topic is well defined, being `/<IMEI>/<appTaskName>`.

## File Logging
The framework contains a file logging system which can be read through the UART, and deleted.

## Booting options
For debuggability purposes, it is possible to extract the file log execution if, during the device boot, the `Button #1` is pressed within 5 seconds from power ON. The log file can also be erased if the `Button #2` is pressed within 5 seconds from power ON. The LED will change colour from blue to green to display the log file, or red to show the log file has been deleted.

NOTE: If you connect a terminal within the 3 seconds time of turning the device on (red LED), you will see some text remininding you about these button functions.

# Application tasks
The structure of these applications is based around `appTasks`. These are tasks which are run in the background, measure and publish to their MQTT topic.

Please see [here](applications/tasks/README.md) for further information of each `appTask` currently implemented.

# Application Configuration
Users need to modify the `src\configFile.h` file providing details of the MQTT broker to be used, and include this header file in the `src\config.h` header file. It is commented out normally as this file should /not/ be shared or committed to the repository.
When this file is included the application will automatically save this file to the file system for later use. You can now delete/remove this file.

The `src\config.h` file also contains the APN which shall be updated accordingly for the SIM card used. Thingstream SIMs should have `TSIOT` configured for the APN.

In summary, to configure this application:-

 1. Edit the `exampleConfigFile.h` file which describes the MQTT broken name or IP address and the username/password. Rename it something like `myConfigFile.h`
 2. #include this `myConfigFile.h` file in the `\src\config.h` file
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. Now delete the `myConfigFile.h` file, and comment out the #include for it. This is so it is not added  to the repository accidentially.
 5. Compile again and re-flash into the XPLR-IoT-1 device.
 6. The saved configuration will be loaded from the file system.

# Application main()
This is the starting point of the application. It will initialize the UBXLIB system and the device. It will also initialize the LEDS, FileSystem and check the log file.

Before running the application it will wait for either `Button #1` or `Button #2` to be pressed to display or delete the log file.

It will then load the configuration file for the MQTT credentials, and if there is the special `\src\configFile.h` file present, save that first to the file system.

After the main system is initialized it will initialize the application tasks. Here they will each create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are initialized the Registration and MQTT application tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application loop will now simple request a SignalQuality measurement and a location update via their respected appTask eventQueues.

The main application will terminate if `Button #1` is pressed, closing the MQTT broker connection, deregistering from the network, and closing the log file. It is important to close down the application by this method as otherwise the log file might not have been saved.