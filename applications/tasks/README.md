# Application Tasks
This framework is based around a applications task which are responsible for certain requirements of the application. This could be measuring a sensor, registration management, cloud service communication, etc.

## OS features of each task
### Event Queue
Each `appTask` has an event queue for sending commands to it. The commands are listed in the `appTask's` .h file.

### Mutex
Each `appTask` has a mutex which is used to notify when a task is in operation, either a long running operation, or in its task loop.

### Thread
Each `appTask` has a task thread which is used for its loop function.

# Implemented application tasks
## LED Task
This task monitors the gAppStatus variable and changes the LEDs to show the current state. As this is a running task all three LEDS can be blinked, flashed, turned on/off etc.

The `gAppStatus` is based on an enumerator which inturn has it's own LED configuration for that status.

## Registration Task
This task monitors the registration status and calls the required `NetworkUp()` function if requried. The number of times the networks goes up is counted.

If the network is currently unknown, the other tasks can see this from the `gIsNetworkUp` variable. Generally if the network is not 'up' the other tasks should not send/publish any data, or expect any downlink data.

## Cell Scan Task
This task is run when the Button #2 is pressed. A message is sent to the CellScanEventQueue. The cell scan task performs a cell scan by using the `uCellNetScanGetFirst()` and `uCellNetScanGetNext()` UBXLIB functions.

The results of the network scan are published via MQTT to the defined broker/topic

## Signal Quality Task
This task runs a signal quality query using the `uCellInfoRefreshRadioParameters()` UBXLIB function. The RSRP and RSRQ results are published to the MQTT broker on the defined topic as a JSON formatted string.

This measurement request is performed via a request on its event queue.

## Location Task
This task configures the GNSS device and takes a location reading. If the GNSS has not aquired a fix yet, further requests for a location will be ignored.

The location is published to the MQTT broker on the defined topic as a JSON formatted string.

This location request is performed via a request on its event queue.

## Sensor Task
This task reads the XPLR-IoT-1 gyro sensors and publishes the values as a JSON formatted string.

This measurement request is performed via a request on its event queue.

## MQTT Task
This task waits for a message on it's MQTT event queue. The other tasks use the `sendMQTTMessage()` function to queue their message on the event queue, with the topic and message as parameters.

The event queue has a 10 message buffer. It will first check if `gIsNetworkUp` variable is set before it goes to publish the message using the `uMqttClientPublish()` UBXLIB function. If the network is not up, the message is not sent.

The MQTT task will also monitor the broker connection, and if it goes down, it will try and re-connect automatically.

The API for this TASK only requires a MQTT or MQTT-SN flag to be set in the mqtt_credentials configuration file found in the application's config folder. The "short names" found in MQTT-SN are automatically handled.

# Sending commands
Application tasks subscribe to a particular MQTT topic so they can listen to commands coming from the cloud. Each MQTT command topic starts with the \<IMEI> of the module and then "xxxControl" for that xxxTask.

## Topic : <IMEI>/AppControl
 - SET_DWELL_TIME \<dwell time ms> : Sets the time between the main application requests for signal quality measurement+location

## Topic : <IMEI>/SignalQualityControl
 - MEASURE_NOW : Request a signal quality measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : <IMEI>/SensorsControl
 - MEASURE_NOW : Request a sensor measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : <IMEI>/LocationControl
 - LOCATION_NOW : Request a location measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : <IMEI>/ExampleControl
 - RUN_EXAMPLE : Runs the example task's "event" (printLog)
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

## Topic : <IMEI>/SensorsControl
 - MEASURE_NOW : Request a sensor measurement to be made now and published to the cloud via MQTT
 - START_TASK \[dwell time seconds] : Starts the task loop with the specified dwell time, or uses the default if missing
 - STOP_TASK : Stops the task loop

# Application task diagram
![Basic appTask diagram](../../readme_images/AppTask.PNG)