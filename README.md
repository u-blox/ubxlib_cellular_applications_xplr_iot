# ubxlib cellular applications for XPLR-IoT-1 development kit

The purpose of this repository is to provide example applications which run on the XPLR-IoT-1 development kit. This repository is forked from the [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) repository and is structured very much the same. A simple **do build** script is used to select the application to build and flash on to the XPLR-IoT-1 development kit.

The [ubxlib-examples-xplr-iot](https://github.com/u-blox/ubxlib_examples_xplr_iot) project contains small examples for individual features of the ubxlib API, whereas this `cellular applications` project is for complete applications based on the ubxlib API.

# NOTE #1: XPLR-IoT-1 SARA-R5 firmware version
PLEASE NOTE: The XPLR-IoT-1 device will most likely have an older version of the SARA-R5 cellular module's firmware. It is highly recommended to update the SARA-R5 module's firmware. This is explained in section 5.3 of the [XPLR-IoT-1 User guide](https://github.com/u-blox/xplr-iot-1-software). You **MUST** perform step 27, to change the +USIO setting to '2'. If you do not do this the application will not be able to communicate to the SARA-R510S module and the LED will continuously flash red.


# Applications

(Currently this repository has only one application).

* [Cellular Tracker](applications/cellular_tracker).
  Publishes cellular signal strength parameters and location. Can be controlled to publish Cell Query results (+COPS=?)

* [...]()

# Application framework

The design of these applications is based around the same design.
Each application has access to the following functions:

## Application tasks

Each application is built from a number of `appTasks` which are either run in their own loop thread or their event executed separately.

Each `appTask` is based on the same 'boiler plate' design. Each application task has a `taskMutex`, `taskEventQueue` and `taskHandler` which are provided by the UBXLIB API.

The application `main()` function simply runs the `appTask's` loop or commands an `appTaks` to run individually at certain timing, or possibly other events. This makes up the application.

See [here](applications/tasks) for further information of each `appTask` currently implemented.

## Application remote control

Each `appTask` has a `<IMEI>/<appTaskName>Control` topic where commands can be sent down to that `appTask`. Start, Stop task, measure 'now' etc.

A typical log output shows what the commands are:
> Subscribed to callback topic: 351457830026040/SensorControl
>
> With these commands:
>
> 1. MEASURE_NOW
> 2. START_TASK
> 3. STOP_TASK

## Application published data

Each appTask can publish their data/information to the MQTT broker to their own specific topic. This topic is well defined, being `<IMEI>/<appTaskName>`.

This data/information should normally be formatted as JSON.

## File Logging
The framework contains a file logging system which can be read through the UART, and deleted, at start-up through booting options...

## Booting options
For debuggability purposes, it is possible to extract the file log execution if, during the device boot, the `Button #1` is pressed within 5 seconds from power ON. The log file can also be erased if the `Button #2` is pressed within 5 seconds from power ON. The LED will change colour from blue to green to display the log file, or red to show the log file has been deleted.

NOTE: If you connect a terminal within the 3 seconds time of turning the device on (red LED), you will see some text remininding you about these button functions.

# Application main()
This is the starting point of the application. It will initialize the UBXLIB system and the device. It will also initialize the LEDS, FileSystem and check the log file.

Before running the application it will wait for either `Button #1` or `Button #2` to be pressed to display or delete the log file.

If present on the file system the application will load the MQTT credentials. If the MQTT credentials are not stored on the file system, it can be specified using a `#define` in the `config.h` file.

After the main system is initialized it will initialize the application tasks. Here each task will create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are initialized the Registration and MQTT application tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application will terminate if `Button #1` is pressed, closing the MQTT broker connection, deregistering from the network, and closing the log file. It is important to close down the application by this method as otherwise the log file might not have been saved.