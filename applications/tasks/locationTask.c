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
 * Location Task to publish the position of the XPLR device to the cloud
 *
 */

#include <time.h>

#include "common.h"
#include "taskControl.h"
#include "locationTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define LOCATION_TASK_STACK_SIZE    (3 * 1024)
#define LOCATION_TASK_PRIORITY      5

#define LOCATION_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define LOCATION_QUEUE_PRIORITY 5
#define LOCATION_QUEUE_SIZE     5

#define JSON_STRING_LENGTH      300

#define TEN_MILLIONTH           10000000

#define FRACTION_FORMAT(v, d)   fractionConvert(v,&whole, &fraction, d), whole, fraction

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
uNetworkCfgGnss_t gNetworkGNSSCfg = {
    .type = U_NETWORK_TYPE_GNSS
};

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uDeviceCfg_t gDeviceGNSSCfg;
static uDeviceHandle_t gnssHandle;

static bool stopLocation = false;
static bool gettingLocation = false;

static char topicName[MAX_TOPIC_NAME_SIZE];

/// callback commands for incoming MQTT control messages
static callbackCommand_t callbacks[] = {
    {"LOCATION_NOW", queueLocationNow},
    {"START_TASK", startLocationTaskLoop},
    {"STOP_TASK", stopLocationTaskLoop}
};

/// @brief buffer for the location MQTT JSON message
static char jsonBuffer[JSON_STRING_LENGTH];

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask && !stopLocation;
}

static bool keepGoing(void *pParam)
{
    bool keepGoing = isNotExiting();
    if (keepGoing) {
        printDebug("Waiting for GNSS location...");
    } else {
        printDebug("GNSS location cancelled");
    }

    return keepGoing;
}

static char fractionConvert(int32_t x1e7,
                          int32_t *pWhole,
                          int32_t *pFraction,
                          int32_t divider)
{
    char prefix = '+';

    // Deal with the sign
    if (x1e7 < 0) {
        x1e7 = -x1e7;
        prefix = '-';
    }
    *pWhole = x1e7 / divider;
    *pFraction = x1e7 % divider;

    return prefix;
}

static void publishLocation(uLocation_t location)
{
    int32_t whole;
    int32_t fraction;

    if (!IS_NETWORK_AVAILABLE) {
        printDebug("publishLocation(): Network is not attached.");
        return;
    }

    gAppStatus = LOCATION_MEAS;

    char timestamp[TIMESTAMP_MAX_LENTH_BYTES];
    getTimeStamp(timestamp);

    char format[] = "{"                                         \
            "\"Timestamp\":%" PRId64 ", "                       \
            "\"Location\":{"                                    \
                "\"Altitude\":\"%d\", "                  \
                "\"Latitude\":\"%c%d.%07d\", "                  \
                "\"Longitude\":\"%c%d.%07d\", "                 \
                "\"Accuracy\":\"%d\", "                  \
                "\"Speed\":\"%d\", "                     \
                "\"Time\":\"%4d-%02d-%02d %02d:%02d:%02d\"}"    \
        "}";

    struct tm *t = gmtime(&location.timeUtc);

    snprintf(jsonBuffer, JSON_STRING_LENGTH, format, (unixNetworkTime + (uPortGetTickTimeMs() / 1000)),
            location.altitudeMillimetres,
            FRACTION_FORMAT(location.latitudeX1e7,  TEN_MILLIONTH),
            FRACTION_FORMAT(location.longitudeX1e7, TEN_MILLIONTH),
            location.radiusMillimetres,
            location.speedMillimetresPerSecond,
            t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    sendMQTTMessage(topicName, jsonBuffer, U_MQTT_QOS_AT_MOST_ONCE, false);
    writeAlways(jsonBuffer);
}

static void getLocation(void *pParams)
{
    if (gettingLocation) {
        printDebug("getLocation(): Already trying to get location...");
        return;
    }

    U_PORT_MUTEX_LOCK(TASK_MUTEX);

    uLocation_t location;   
    gettingLocation = true;

    printDebug("Requesting location information...");
    int32_t errorCode = uLocationGet(gnssHandle, U_LOCATION_TYPE_GNSS,
                                         NULL, NULL, &location, keepGoing);
    if (errorCode == 0) {
        publishLocation(location);
    } else {
        if (errorCode == U_ERROR_COMMON_TIMEOUT)
            writeDebug("Timed out getting GNSS location");
        else
            writeError("Failed to get GNSS location: %d", errorCode);
    }

    gettingLocation = false;

    // reset the stop location indicator
    stopLocation = false;

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

static void startGetLocation(void)
{
    RUN_FUNC(getLocation, LOCATION_TASK_STACK_SIZE, LOCATION_TASK_PRIORITY);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    locationMsg_t *qMsg = (locationMsg_t *) pParam;

    switch(qMsg->msgType) {
        case GET_LOCATION_NOW:
            startGetLocation();
            break;

        case STOP_LOCATON_ACQUISITION:
            stopLocation = true;
            break;

        case SHUTDOWN_LOCATION_TASK:
            stopLocationTaskLoop(NULL);
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

// Signal Quality task loop for reading the RSRP and RSRQ values
// and sending these values to the MQTT topic
static void taskLoop(void *pParameters)
{
    while(isNotExiting()) {
        getLocation(NULL);
        dwellTask(taskConfig, isNotExiting);
    }

    FINALIZE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(locationMsg_t),
                    LOCATION_QUEUE_STACK_SIZE,
                    LOCATION_QUEUE_PRIORITY,
                    LOCATION_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeFatal("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

static int32_t startGNSS(void)
{
    int32_t errorCode;

    uDeviceGetDefaults(U_DEVICE_TYPE_GNSS, &gDeviceGNSSCfg);
    errorCode = uDeviceOpen(&gDeviceGNSSCfg, &gnssHandle);
    if (errorCode != 0) {
        writeFatal("Failed to open the GNSS device: %d", errorCode);
        return errorCode;
    }

    errorCode = uNetworkInterfaceUp(gnssHandle, U_NETWORK_TYPE_GNSS, &gNetworkGNSSCfg);
    if (errorCode != 0) {
        writeFatal("Failed to bring up the GNSS device: %d", errorCode);
        return errorCode;
    }

    return U_ERROR_COMMON_SUCCESS;
}

static int32_t initMutex()
{
    INIT_MUTEX;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Queue the getLocation operation
/// @param params The parameters for this command
/// @return returns the errorCode of sending the message on the eventQueue
int32_t queueLocationNow(commandParamsList_t *params)
{
    locationMsg_t qMsg;
    qMsg.msgType = GET_LOCATION_NOW;

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(locationMsg_t));
}

/// @brief Initialises the Signal Quality task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initLocationTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    CREATE_TOPIC_NAME;

    writeLog("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);

    result = startGNSS();
    if (result < 0) {
        writeFatal("Failed to start the GNSS system");
        return result;
    }

    char tp[MAX_TOPIC_NAME_SIZE];
    snprintf(tp, MAX_TOPIC_NAME_SIZE, "%sControl", TASK_NAME);
    subscribeToTopicAsync(tp, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successful, a negative number otherwise
int32_t startLocationTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    if (params != NULL)
        taskConfig->taskLoopDwellTime = getParamValue(params, 1, 5, 60, 30);

    START_TASK_LOOP(LOCATION_TASK_STACK_SIZE, LOCATION_TASK_PRIORITY);
}

int32_t stopLocationTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}