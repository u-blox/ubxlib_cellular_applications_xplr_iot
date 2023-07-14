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
 * Cell Scan Task to run the +COPS=? Query and publish the results
 *
 */

#include "common.h"
#include "taskControl.h"
#include "cellScanTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define NETWORK_SCAN_TOPIC "NetworkScan"

#define CELL_SCAN_TASK_STACK_SIZE (3*1024)
#define CELL_SCAN_TASK_PRIORITY 5

#define CELL_SCAN_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define CELL_SCAN_QUEUE_PRIORITY 5
#define CELL_SCAN_QUEUE_SIZE 2

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static applicationStates_t tempAppStatus;

static bool stopCellScan = false;

static char topicName[MAX_TOPIC_NAME_SIZE];

/// callback commands for incoming MQTT control messages
static callbackCommand_t callbacks[] = {
    {"START_CELL_SCAN", queueNetworkScan}
};

/* ----------------------------------------------------------------
 * EXTERNAL FUNCTIONS
 * -------------------------------------------------------------- */
extern void pauseMainLoop(bool state);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask && !stopCellScan;
}

static bool keepGoing(void *pParam)
{
    bool kg = isNotExiting();
    if (kg) {
        gAppStatus = COPS_QUERY;
        printDebug("Still scanning for networks...");
    } else {
        writeInfo("Scanning for networks cancelled");
    }

    return kg;
}

static void doCellScan(void *pParams)
{
    int32_t found = 0;
    int32_t count = 0;
    char internalBuffer[64];
    char payload[100];
    char mccMnc[U_CELL_NET_MCC_MNC_LENGTH_BYTES];
    uCellNetRat_t rat = U_CELL_NET_RAT_UNKNOWN_OR_NOT_USED;

    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    gAppStatus = COPS_QUERY;
    
    pauseMainLoop(true);

    char timestamp[TIMESTAMP_MAX_LENTH_BYTES];
    getTimeStamp(timestamp);

    char format[] = "{"                 \
            "\"Timestamp\":\"%s\", "    \
            "\"CellSCan\":{"            \
                "\"Name\":\"%s\", "     \
                "\"ubxlibRAT\":\"%d\", "     \
                "\"MCCMNC\":\"%s\"}"   \
        "}";

    writeLog("Scanning for networks...");
    for (count = uCellNetScanGetFirst(gDeviceHandle, internalBuffer,
                                            sizeof(internalBuffer), mccMnc, &rat,
                                            keepGoing);
            count > 0;
            count = uCellNetScanGetNext(gDeviceHandle, internalBuffer, sizeof(internalBuffer), mccMnc, &rat)) {

        found++;
        snprintf(payload, sizeof(payload), format, timestamp, internalBuffer, rat, mccMnc);
        writeAlways(payload);
        sendMQTTMessage(topicName, payload, U_MQTT_QOS_AT_MOST_ONCE, false);
    }

    if (!gExitApp) {
        if(count < 0 && count != U_CELL_ERROR_NOT_FOUND) {
            snprintf(payload, sizeof(payload), "Cell Scan Result: Error %d", count);
        } else {
            if (found == 0) {
                snprintf(payload, sizeof(payload), "Cell Scan Result: No network operators found.");
            } else {
                snprintf(payload, sizeof(payload), "Cell Scan Result: %d network(s) found in total.", found);
            }
        }
    } else {
        snprintf(payload, sizeof(payload), "Cell Scan Result: Cancelled.");
    }

    writeInfo(payload);

    // reset the stop cell scan indicator
    stopCellScan = false;

    pauseMainLoop(false);

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

static void startCellScan(void)
{
    RUN_FUNC(doCellScan, CELL_SCAN_TASK_STACK_SIZE, CELL_SCAN_TASK_PRIORITY);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    cellScanMsg_t *qMsg = (cellScanMsg_t *) pParam;

    switch(qMsg->msgType) {
        case START_CELL_SCAN:
            startCellScan();
            break;

        case STOP_CELL_SCAN:
            stopCellScan = true;
            break;

        case SHUTDOWN_CELL_SCAN_TASK:
            exitTask = true;
            break;

        default:
            writeWarn("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

static int32_t initMutex()
{
    INIT_MUTEX;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(cellScanMsg_t),
                    CELL_SCAN_QUEUE_STACK_SIZE,
                    CELL_SCAN_QUEUE_PRIORITY,
                    CELL_SCAN_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeFatal("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief Places a Start Network Scan message on the queue
/// @param params The parameters for this command
/// @return zero if successful, a negative value otherwise
int32_t queueNetworkScan(commandParamsList_t *params)
{
    cellScanMsg_t qMsg;
    if (isMutexLocked(TASK_MUTEX)) {
        writeLog("Cell Scan is already in progress, cancelling...");
        qMsg.msgType = STOP_CELL_SCAN;
    } else {
        writeLog("Starting cell scan...");
        qMsg.msgType = START_CELL_SCAN;
    }

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(cellScanMsg_t));
}

/// @brief Initialises the network scanning task(s)
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initCellScanTask(taskConfig_t *config)
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
int32_t startCellScanTaskLoop(commandParamsList_t *params)
{
    return U_ERROR_COMMON_NOT_IMPLEMENTED;
}

int32_t stopCellScanTask(commandParamsList_t *params)
{
    STOP_TASK;
}