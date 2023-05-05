# Cellular Signal Measuring Application
The Cellular Signal Measuring application is an example application that embeds several functionalities.

The main purpose of the application is to periodically monitor the signal quality of the cellular environment, its location and scan the visible base stations if the `Button #2` is pressed.

All collected information is stored in the device filesystem via the application log but also shared into the cloud via the SARA-R5 MQTT embedded client to an MQTT Broker.

Once turned ON, by default the application monitors the cellular signal quality. Once there is a GNSS fix, the location is also published to the cloud. If the `Button #2` is pressed, a base station scan is initialized.

If the `Button #1` is pressed the application shuts down and the log file is saved and closed. If you do not press `Button #1` and simply turn off the XPLR-IoT-1 device then the log file will not save the entire log.

## Booting options
For debuggability purposes, it is possible to extract the file log execution if, during the device boot, the `Button #1` is pressed within 5 seconds from power ON. The log file can also be erased if the `Button #2` is pressed within 5 seconds from power ON.

NOTE: If you connect a terminal within the 3 seconds time of turning the device on (red LED), you will see some text remininding you about these button functions.

# Application Configuration
Users need to modify the `src\configFile.h` file providing details of the MQTT broker to be used, and include this header file in the `src\config.h` header file. It is commented out normally as this file should /not/ be shared or committed to the repository.
When this file is included the application will automatically save this file to the file system for later use. You can now delete/remove this file.

The `src\config.h` file also contains the APN which shall be updated accordingly for the SIM card used. Thingstream SIMs should have `TSIOT` configured for the APN.

So in summary, to configure this application:-

 1. Edit the exampleConfigFile.h file which describes the MQTT broken name or IP aaddress and the username/password.
 2. #include this in the `\src\config.h` file
 3. Compile and flash. When the application runs it will save this configuration information to the file system.
 4. Now delete the `configFile.h` file, and comment out the #include for it. This is so it does not get added to the repository
 5. Compile again and re-flash into the XPLR-IoT-1 device.
 6. The saved configuration will be loaded from the file system.

# Application Tasks
This framework is based around an application task which is responsible for a certain requirement of the application. This could be measuring a sensor, registration management, cloud service communication, etc.

All application tasks, are based on the same 'boiler plate' design.
Each application task has a mixture of a `taskMutex`, `taskEventQueue` and `taskHandler` UBXLIB provided features.

The `taskMutex` is used to encompass the running of the task loop or the long running event of the task. There is a `isXXXXXTaskRunning()` function for each task that the main application can monitor.

The tasks can exit either by simply setting the `gExitApp` variable, or specifically calling the `stopXXX()` type function for that task.

This application has 6 `appTasks`:

- LED Task
- Registration Task
- Cell Scan Task
- Signal Quality Task
- Location Task
- MQTT Task

### LED Task
This task monitors the gAppStatus variable and changes the LEDs to show the current state. As this is a running task all three LEDS can be blinked, flashed, turned on/off etc.

The `gAppStatus` is based on an enumerator which inturn has it's own LED configuration for that status.

### Registration Task
This task monitors the registration status and calls the required `NetworkUp()` function if requried. The number of times the networks goes up is counted.

If the network is currently unknown, the other tasks can see this from the `gIsNetworkUp` variable. Generally if the network is not 'up' the other tasks should not send/publish any data, or expect any downlink data.

### Cell Scan Task
This task is run when the Button #2 is pressed. A message is sent to the CellScanEventQueue. The cell scan task performs a cell scan by using the `uCellNetScanGetFirst()` and `uCellNetScanGetNext()` UBXLIB functions.

The results of the network scan are published via MQTT to the defined broker/topic

### Signal Quality Task
This task runs a signal quality query using the `uCellInfoRefreshRadioParameters()` UBXLIB function. The RSRP and RSRQ results are published to the MQTT broker on the defined topic as a JSON formatted string.

This measurement request is performed via a request on its event queue.

### Location Task
This task configures the GNSS device and takes a location reading. If the GNSS has not aquired a fix yet, further requests for a location will be ignored.

The location is published to the MQTT broker on the defined topic as a JSON formatted string.

This location request is performed via a request on its event queue.

### MQTT Task
This task waits for a message on it's MQTT event queue. The other tasks use the `sendMQTTMessage()` function to queue their message on the event queue, with the topic and message as parameters.

The event queue has a 10 message buffer. It will first check if `gIsNetworkUp` variable is set before it goes to publish the message using the `uMqttClientPublish()` UBXLIB function. If the network is not up, the message is not sent.

The MQTT task will also monitor the broker connection, and if it goes down, it will try and re-connect automatically.

# Publishing information to the MQTT Broker
Each appTask can publish their data/information to the MQTT broker to their own specific topic. This topic is well defined, being /\<IMEI>/\<appTaskName>.

# main()
This is the starting point of the application. It will initialize the UBXLIB system and the device. It will also initialize the LEDS, FileSystem and check the log file.

Before running the application it will wait for either `Button #1` or `Button #2` to be pressed to display or delete the log file.

It will then load the configuration file for the MQTT credentials, and if there is the special `\src\configFile.h` file present, save that first to the file system.

After the main system is initialized it will initialize the application tasks. Here they will each create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are initialized the Registration and MQTT application tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application loop will now simple request a SignalQuality measurement and a location update via their respected appTask eventQueues.

The main application will terminate if `Button #1` is pressed, closing the MQTT broker connection, deregistering from the network, and closing the log file.

# Sending commands down to the XPLR-IoT-1 device
Application tasks have their own eventQueue. Commands can be sent to this eventQueue by other tasks. Application tasks also subscribe to a particular MQTT topic so they can listen to commands coming from the cloud. Each MQTT command topic starts with the \<IMEI> of the module and then "xxxControl" for that xxxTask.

## Topic : /\<IMEI>/AppControl
 - SET_DWELL_TIME \<dwell time ms> : Sets the time between the main application requests for signal quality measurement+location

## Topic : /\<IMEI>/SignalQualityControl
 - MEASURE_NOW : Request a signal quality measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : /\<IMEI>/SensorsControl
 - MEASURE_NOW : Request a sensor measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : /\<IMEI>/LocationControl
 - LOCATION_NOW : Request a location measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : /\<IMEI>/ExampleControl
 - RUN_EXAMPLE : Runs the example task's "event" (printLog)
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : /\<IMEI>/SensorsControl
 - LOCATION_NOW : Request a location measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

# Application task diagram
![Basic appTask diagram](src/AppTask.PNG)