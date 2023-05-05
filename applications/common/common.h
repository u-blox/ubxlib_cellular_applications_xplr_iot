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
 * Application header
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stdint.h>

#include "ubxlib.h"
#include "config.h"
#include "log.h"

#include "kernel.h"

/* ----------------------------------------------------------------
 * MACORS for common task usage/access
 * -------------------------------------------------------------- */
#define SET_APP_STATUS(x) tempAppStatus = gAppStatus; gAppStatus = x
#define REVERT_APP_STATUS(x) gAppStatus = tempAppStatus

#define CHECK_SUCCESS(x) result = x(); if (result < 0) return result
#define TASK_MUTEX taskConfig->handles.mutexHandle
#define TASK_HANDLE taskConfig->handles.taskHandle
#define TASK_QUEUE taskConfig->handles.eventQueueHandle
#define TASK_NAME taskConfig->name
#define TASK_ID taskConfig->id
#define TASK_INITIALISED ((taskConfig != NULL) && taskConfig->initialised)

#define CREATE_TOPIC_NAME snprintf(topicName, MAX_TOPIC_NAME_SIZE, "/%s/%s", (const char *)gSerialNumber, TASK_NAME)

#define EXIT_IF_CANT_RUN_TASK   if (taskConfig == NULL || !TASK_INITIALISED) {                      \
                                    writeWarn("%s task is not initialised yet, not starting.",      \
                                            TASK_NAME);                                             \
                                    return U_ERROR_COMMON_NOT_INITIALISED;                          \
                                }                                                                   \
                                if (TASK_HANDLE != NULL) {                                          \
                                    writeWarn("%s task is already running, not starting again.",    \
                                            TASK_NAME);                                             \
                                    return U_ERROR_COMMON_SUCCESS;                                  \
                                }

#define EXIT_IF_CONFIG_NULL     if (config == NULL) {                                               \
                                    writeError("Cannot initialise task as configuration is NULL");  \
                                    return U_ERROR_COMMON_INVALID_PARAMETER;                        \
                                }

#define START_TASK              int32_t errorCode = startTask();                    \
                                if (errorCode == 0) {                               \
                                    writeLog("Started %s task loop", TASK_NAME);    \
                                }                                                   \
                                return errorCode;

#define STOP_TASK               if (taskConfig == NULL) {                                               \
                                    writeDebug("Stop %s task requested, but it is not initialised");    \
                                    return U_ERROR_COMMON_NOT_INITIALISED;                              \
                                }                                                                       \
                                exitTask = true;                                                        \
                                writeLog("Stop %s task requested...", taskConfig->name);                \
                                return U_ERROR_COMMON_SUCCESS;

#define INIT_MUTEX              int32_t errorCode = uPortMutexCreate(&TASK_MUTEX);      \
                                if (errorCode != 0) {                                   \
                                    writeFatal("Failed to create %s Mutex (%d).",       \
                                            TASK_NAME, errorCode);                      \
                                }                                                       \
                                return errorCode;

#define RUN_FUNC(func, stackSize, priority)                                                         \
                                uPortTaskHandle_t taskHandle;                                       \
                                int32_t errorCode = uPortTaskCreate(runTaskAndDelete, TASK_NAME,    \
                                            stackSize, func, priority, &taskHandle);                \
                                if (errorCode < 0) {                                                \
                                    writeError("Failed to start %s task function: %d",              \
                                            TASK_NAME, errorCode); }

#define START_TASK_LOOP(stackSize, priority)                                                        \
                                int32_t errorCode = uPortTaskCreate(runTaskAndDelete, TASK_NAME,    \
                                            stackSize, taskLoop, priority, &TASK_HANDLE);           \
                                if (errorCode != 0) {                                               \
                                    writeError("Failed to start the %s Task (%d).",                 \
                                            TASK_NAME, errorCode);                                  \
                                }                                                                   \
                                return errorCode;

#define FINALISE_TASK           writeDebug("%s task loop has stopped", TASK_NAME);                      \
                                if (taskConfig->taskStoppedCallback != NULL) {                          \
                                        writeDebug("Running %s task stopped callback...", TASK_NAME);   \
                                        taskConfig->taskStoppedCallback(NULL);                          \
                                }                                                                       \
                                TASK_HANDLE = NULL;

#define IS_NETWORK_AVAILABLE    (gIsNetworkSignalValid && gIsNetworkUp)

#define NUM_ELEMENTS(x) (sizeof(x) / sizeof((x)[0]))

#define MAX_NUMBER_COMMAND_PARAMS 5

#define MAX_TOPIC_NAME_SIZE 50

/** The maximum length of the Time Stamp string.
 * hh:mm:ss.mmm
 */
#define TIMESTAMP_MAX_LENTH_BYTES 13

// Lock the zephyr scheduler to allow atomic sections
#define Z_SECTION_LOCK      k_sched_lock(); {
#define Z_SECTION_UNLOCK    } k_sched_unlock();

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */
// Default set of application statuses
typedef enum {
    MANUAL,
    INIT_DEVICE,
    REGISTERING,
    MQTT_CONNECTING,
    COPS_QUERY,
    SEND_SIGNAL_QUALITY,
    REGISTRATION_UNKNOWN,
    REGISTERED,
    ERROR,
    SHUTDOWN,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    START_SIGNAL_QUALITY,
    REGISTRATION_DENIED,
    NO_NETWORKS_AVAILABLE,
    NO_COMPATIBLE_NETWORKS,
    MAX_STATUS
} applicationStates_t;

typedef struct TaskHandles {
    uPortTaskHandle_t taskHandle;
    uPortMutexHandle_t mutexHandle;
    int32_t eventQueueHandle;
} taskHandles_t;

typedef enum {
    NETWORK_REG_TASK = 0,
    CELL_SCAN_TASK = 1,
    MQTT_TASK = 2,
    SIGNAL_QUALITY_TASK = 3,
    LED_TASK = 4,
    EXAMPLE_TASK = 5,
    LOCATION_TASK = 6,
    SENSOR_TASK = 7,
    MAX_TASKS
} taskTypeId_t;

/// Callback for setting what happens after the task has stopped
typedef void (*taskStoppedCallback_t)(void *);

typedef struct TaskConfig {
    taskTypeId_t id;
    const char *name;
    int32_t taskLoopDwellTime;
    bool initialised;
    taskHandles_t handles;
    taskStoppedCallback_t taskStoppedCallback;
} taskConfig_t;

/// @brief command information
typedef struct commandParamsList {
    char *parameter;
    struct commandParamsList *pNext;
} commandParamsList_t;

/// @brief callback information
typedef struct {
    const char *command;
    int32_t (*callback)(commandParamsList_t *params);
} callbackCommand_t;

typedef int32_t (*taskInit_t)(taskConfig_t *taskConfig);
typedef int32_t (*taskStart_t)(commandParamsList_t *params);
typedef int32_t (*taskStop_t)(commandParamsList_t *params);

typedef struct TaskRunner {
    taskInit_t initFunc;
    taskStart_t startFunc;
    taskStop_t stopFunc;
    taskConfig_t config;
} taskRunner_t;

/* ----------------------------------------------------------------
 * EXTERNAL VARIABLES used in the application tasks
 * -------------------------------------------------------------- */

// serial number of the cellular module
extern char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

// This is the ubxlib deviceHandle for communicating with the celullar module
extern uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application's tasks should exit
extern bool gExitApp;

// This flag represents the network's registration status
extern bool gIsNetworkUp;

// This flag represents the module can hear the network signaling (RSRP != 0)
extern bool gIsNetworkSignalValid;

// application status
extern applicationStates_t gAppStatus;

// our framework tasks
extern taskRunner_t tasks[];

/// The unix network time, which is retrieved after first registration
extern int64_t unixNetworkTime;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
bool isMutexLocked(uPortMutexHandle_t mutex);
char *uStrDup(const char *src);

int32_t sendAppTaskMessage(int32_t taskId, void *pMessage, size_t msgSize);

// Simple function to split message into command/params linked list
size_t getParams(char *message, commandParamsList_t **head);
void freeParams(commandParamsList_t *head);
int32_t getParamValue(commandParamsList_t *params, size_t index, int32_t minValue, int32_t maxValue, int32_t defValue);

void getTimeStamp(char *timeStamp);

void dwellTask(taskConfig_t *taskConfig, bool (*exitFunc)(void));

void runTaskAndDelete(void *pParams);

#endif