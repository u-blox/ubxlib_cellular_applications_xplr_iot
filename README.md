# ubxlib cellular applications for XPLR-IoT-1 development kit
The purpose of this repository is to provide example applications which run on the XPLR-IoT-1 development kit. This repository is forked from the [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) repository and is structured very much the same. A simple **do build** script is used to select the application to build and flash on to the XPLR-IoT-1 development kit.

The [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) project contains small examples for individual features of the ubxlib API, whereas this `cellular applications` project is for complete applications based on the ubxlib API.

# Applications
(Currently this repository has only one application).

* [Cellular Tracker](applications/cellular_tracker).
  Publishes cellular signal strength parameters and location. Can be controlled to publish Cell Query results (+COPS=?)

* [...]()

# Application framework
The design of these applications is based around the same design. Each application has access to the following functions:

## Application tasks
Each application is built from a number of `appTasks` which are either run in their own loop thread or their event executed separately.

Each `appTask` is based on the same 'boiler plate' design. Each application task has a `taskMutex`, `taskEventQueue` and `taskHandler` which are provided by the UBXLIB API.

The application `main()` function simply runs the `appTask's` loop or commands an `appTaks` to run individually at certain timing, or possibly other events. This makes up the application.

See [here](applications/tasks) for further information of each `appTask` currently implemented.

## MQTT communication
Each `appTask` has a `/<IMEI>/<appTaskName>Control` topic where commands can be sent down to that `appTask`. Start, Stop task, measure 'now' etc.

Each appTask can publish their data/information to the MQTT broker to their own specific topic. This topic is well defined, being `/<IMEI>/<appTaskName>`.

## File Logging
The framework contains a file logging system which can be read through the UART, and deleted.

## Booting options
For debuggability purposes, it is possible to extract the file log execution if, during the device boot, the `Button #1` is pressed within 5 seconds from power ON. The log file can also be erased if the `Button #2` is pressed within 5 seconds from power ON. The LED will change colour from blue to green to display the log file, or red to show the log file has been deleted.

NOTE: If you connect a terminal within the 3 seconds time of turning the device on (red LED), you will see some text remininding you about these button functions.

# Application main()
This is the starting point of the application. It will initialize the UBXLIB system and the device. It will also initialize the LEDS, FileSystem and check the log file.

Before running the application it will wait for either `Button #1` or `Button #2` to be pressed to display or delete the log file.

It will then load the configuration file for the MQTT credentials, and if there is the special `\src\configFile.h` file present, save that first to the file system.

After the main system is initialized it will initialize the application tasks. Here they will each create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are initialized the Registration and MQTT application tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application loop will now simple request a SignalQuality measurement and a location update via their respected appTask eventQueues.

The main application will terminate if `Button #1` is pressed, closing the MQTT broker connection, deregistering from the network, and closing the log file. It is important to close down the application by this method as otherwise the log file might not have been saved.