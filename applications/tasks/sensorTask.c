/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * Example task for duplicating for your own tasks to be added to
 * this cellular application framework.
 *
 */

#include "common.h"
#include "taskControl.h"
#include "sensorTask.h"
#include "mqttTask.h"
#include "sensors.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define SENSOR_TOPIC "Sensors"
#define SENSOR_DWELL_SECONDS 30

#define SENSOR_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define SENSOR_QUEUE_PRIORITY 5
#define SENSOR_QUEUE_SIZE 1

// not all tasks will have a task loop if it only uses a queue
#define SENSOR_TASK_STACK_SIZE (2 * 1024)
#define SENSOR_TASK_PRIORITY 5

#define MQTT_MESSAGE_MAX_SIZE 200

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char topicName[MAX_TOPIC_NAME_SIZE];

static char buffer[MQTT_MESSAGE_MAX_SIZE];

/// callback commands for incoming MQTT control messages
static callbackCommand_t callbacks[] = {
    {"MEASURE_NOW", queueGetSensors},
    {"START_TASK", startSensorTaskLoop},
    {"STOP_TASK", stopSensorTaskLoop}
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask;
}

static void publishAccel(void)
{
    float x,y,z;
    getAccelerometer(&x, &y, &z);
    snprintf(buffer, MQTT_MESSAGE_MAX_SIZE, "{\"Accellerometer\": {\"X\":\"%.2f\", \"Y\":\"%.2f\", \"Z\":\"%.2f\"}}", x, y, z);
    sendMQTTMessage(topicName, buffer, U_MQTT_QOS_AT_MOST_ONCE, false);
    writeAlways(buffer);

//    float px, py, pz;
//    getPosition(x, y, z, &px, &py, &pz);
//    snprintf(buffer, MQTT_MESSAGE_MAX_SIZE, "{\"Position\": {\"X\":\"%.2f\", \"Y\":\"%.2f\", \"Z\":\"%.2f\"}}", px, py, pz);
//    sendMQTTMessage(topicName, buffer, U_MQTT_QOS_AT_MOST_ONCE, false);
//    writeLog(buffer);
}

static void publishTemp(void)
{
    float temp, pressure, humidity;
    getTempSensor(&temp, &pressure, &humidity);
    snprintf(buffer, MQTT_MESSAGE_MAX_SIZE,
                "{\"Temperature\": {\"Temperature\":\"%.2f\", \"Pressure\":\"%.2f\", \"Humidity\":\"%.2f\"}}",
                temp, pressure, humidity);

    sendMQTTMessage(topicName, buffer, U_MQTT_QOS_AT_MOST_ONCE, false);
    writeAlways(buffer);
}

static void publishLight(void)
{
    int32_t lux = getLightSensor();
    snprintf(buffer, MQTT_MESSAGE_MAX_SIZE, "{\"Light\": {\"Lux\":\"%d\"}}", lux);

    sendMQTTMessage(topicName, buffer, U_MQTT_QOS_AT_MOST_ONCE, false);
    writeAlways(buffer);
}

static void publishSensors(void)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    publishAccel();
    publishTemp();
    publishLight();
    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // cast the incoming pParam to the proper param structure
    sensorMsg_t *qMsg = (sensorMsg_t *) pParam;

    switch(qMsg->msgType) {
        case GET_SENSORS_NOW:
            publishSensors();
            break;

        case SHUTDOWN_SENSOR_TASK:
            stopSensorTaskLoop(NULL);
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

// Task loop where the activity is made and the dwell time is taken
static void taskLoop(void *pParameters)
{
    while(isNotExiting()) {
        publishSensors();
        dwellTask(taskConfig, isNotExiting);
    }

    FINALISE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(sensorMsg_t),
                    SENSOR_QUEUE_STACK_SIZE,
                    SENSOR_QUEUE_PRIORITY,
                    SENSOR_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeFatal("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

static int32_t initMutex()
{
    INIT_MUTEX;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief Queue the Get Sensors command
/// @param params The parameters for this command
/// @return returns the errorCode of sending the message on the eventQueue
int32_t queueGetSensors(commandParamsList_t *params)
{
    sensorMsg_t qMsg;
    qMsg.msgType = GET_SENSORS_NOW;

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(sensorMsg_t));
}


/// @brief Initialises the Signal Quality task
/// @param config The task configuration structure
/// @return zero if successfull, a negative number otherwise
int32_t initSensorTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    CREATE_TOPIC_NAME;

    writeLog("Initializing the %s task...", TASK_NAME);
    CHECK_SUCCESS(initMutex);
    CHECK_SUCCESS(initQueue);

    char tp[MAX_TOPIC_NAME_SIZE];
    snprintf(tp, MAX_TOPIC_NAME_SIZE, "%sControl", TASK_NAME);
    subscribeToTopicAsync(tp, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    return result;
}

/// @brief Starts the Sensor task loop
/// @return zero if successfull, a negative number otherwise
int32_t startSensorTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    if (params != NULL)
        taskConfig->taskLoopDwellTime = getParamValue(params, 1, 5, 60, 30);

    sensorsInit();
    START_TASK_LOOP(SENSOR_TASK_STACK_SIZE, SENSOR_TASK_PRIORITY);
}

int32_t stopSensorTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}