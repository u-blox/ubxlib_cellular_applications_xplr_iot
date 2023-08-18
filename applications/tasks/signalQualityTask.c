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
 * Signal Quality Task to monitor the signal quality of the network connection
 *
 */

#include "common.h"
#include "taskControl.h"
#include "signalQualityTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define SIGNAL_QUALITY_TASK_STACK_SIZE 1024
#define SIGNAL_QUALITY_TASK_PRIORITY 5

#define SIGNAL_QUALITY_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define SIGNAL_QUALITY_QUEUE_PRIORITY 5
#define SIGNAL_QUALITY_QUEUE_SIZE 5

#define JSON_STRING_LENGTH 300

/* ----------------------------------------------------------------
 * PUBLIC VARIABLES
 * -------------------------------------------------------------- */

// This flag represents the module can hear the network signaling
bool gIsNetworkSignalValid = false;

extern char pOperatorName[OPERATOR_NAME_SIZE];
extern int32_t operatorMcc, operatorMnc;

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char topicName[MAX_TOPIC_NAME_SIZE];

/// callback commands for incoming MQTT control messages
static callbackCommand_t callbacks[] = {
    {"MEASURE_NOW", queueMeasureNow},
    {"START_TASK", startSignalQualityTaskLoop},
    {"STOP_TASK", stopSignalQualityTaskLoop}
};

/// @brief buffer for the cell signal quality MQTT JSON message
static char jsonBuffer[JSON_STRING_LENGTH];

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask;
}

static void measureSignalQuality(void)
{
    int32_t errorCode;

    if (!gIsNetworkUp) {
        printDebug("measureSignalQuality(): Network is not attached.");
        return;
    }

    U_PORT_MUTEX_LOCK(TASK_MUTEX);

    printDebug("Fetching signal quality measurements...");
    gAppStatus = START_SIGNAL_QUALITY;

    char timestamp[TIMESTAMP_MAX_LENTH_BYTES];
    getTimeStamp(timestamp);
    errorCode = uCellInfoRefreshRadioParameters(gDeviceHandle);

    if (errorCode == 0) {
        int32_t rsrp = uCellInfoGetRsrpDbm(gDeviceHandle);
        int32_t rsrq = uCellInfoGetRsrqDb(gDeviceHandle);
        int32_t rssi = uCellInfoGetRssiDbm(gDeviceHandle);
        int32_t rxqual = uCellInfoGetRxQual(gDeviceHandle);
        int32_t snr;
        uCellInfoGetSnrDb(gDeviceHandle, &snr);
        int32_t cellId = uCellInfoGetCellId(gDeviceHandle);
        int32_t earfcn = uCellInfoGetEarfcn(gDeviceHandle);

        char format[] = "{" \
            "\"Timestamp\":%" PRId64 ", "   \
            "\"CellQuality\":{"             \
                "\"RSRP\":%d, "             \
                "\"RSRQ\":%d, "             \
                "\"RSSI\":%d, "             \
                "\"SNR\":%d}, "             \
            "\"CellInfo\":{"                \
                "\"RxQual\":%d, "           \
                "\"CellID\":\"%d\", "       \
                "\"EARFCN\":\"%d\", "       \
                "\"PLMN\":\"%03d%02d\", "   \
                "\"Operator\":\"%s\"}"      \
        "}";

        // Checking if some radio parameters are not zero is a good way
        // to determine if the network is visible and useable.
        // See macro "IS_NETWORK_AVAILABLE"
        gIsNetworkSignalValid = (rsrp != 0) && (rsrq != 2147483647);

        snprintf(jsonBuffer, JSON_STRING_LENGTH, format, (unixNetworkTime + (uPortGetTickTimeMs() / 1000)), 
                                rsrp, rsrq, rssi, snr, rxqual, 
                                cellId, earfcn, operatorMcc, operatorMnc, pOperatorName);
        sendMQTTMessage(topicName, jsonBuffer, U_MQTT_QOS_AT_MOST_ONCE, false);
        writeAlways(jsonBuffer);
    } else {
        if (errorCode == U_CELL_ERROR_NOT_REGISTERED) {
            writeDebug("SignalQualityTask: Not registered");
        } else {
            writeWarn("Failed to read Radio Parameters %d", errorCode);
        }
    }

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    signalQualityMsg_t *qMsg = (signalQualityMsg_t *) pParam;

    switch(qMsg->msgType) {
        case MEASURE_SIGNAL_QUALTY_NOW:
            measureSignalQuality();
            break;

        case SHUTDOWN_SIGNAL_QAULITY_TASK:
            stopSignalQualityTaskLoop(NULL);
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
        measureSignalQuality();
        dwellTask(taskConfig, isNotExiting);
    }

    FINALIZE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(signalQualityMsg_t),
                    SIGNAL_QUALITY_QUEUE_STACK_SIZE,
                    SIGNAL_QUALITY_QUEUE_PRIORITY,
                    SIGNAL_QUALITY_QUEUE_SIZE);

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
/// @brief Queue the get cell quality measurements command
/// @param params The parameters for this command
/// @return returns the errorCode of sending the message on the eventQueue
int32_t queueMeasureNow(commandParamsList_t *params)
{
    signalQualityMsg_t qMsg;
    qMsg.msgType = MEASURE_SIGNAL_QUALTY_NOW;

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(signalQualityMsg_t));
}

/// @brief Initialises the Signal Quality task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initSignalQualityTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    CREATE_TOPIC_NAME;

    writeLog("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);

    char tp[MAX_TOPIC_NAME_SIZE];
    snprintf(tp, MAX_TOPIC_NAME_SIZE, "%sControl", TASK_NAME);
    subscribeToTopicAsync(tp, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successful, a negative number otherwise
int32_t startSignalQualityTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    if (params != NULL)
        taskConfig->taskLoopDwellTime = getParamValue(params, 1, 5, 60, 30);

    START_TASK_LOOP(SIGNAL_QUALITY_TASK_STACK_SIZE, SIGNAL_QUALITY_TASK_PRIORITY);
}

int32_t stopSignalQualityTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}