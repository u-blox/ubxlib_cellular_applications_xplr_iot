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
#include "configUtils.h"
#include "log.h"

#include "kernel.h"

/* ----------------------------------------------------------------
 * MACORS for common task usage/access
 * -------------------------------------------------------------- */
#define IS_NETWORK_AVAILABLE        (gIsNetworkSignalValid && gIsNetworkUp)

#define NUM_ELEMENTS(x)             (sizeof(x) / sizeof(x[0]))

#define MAX_NUMBER_COMMAND_PARAMS   5

#define MAX_TOPIC_NAME_SIZE         50

#define QUEUE_STACK_SIZE(x)         MIN(U_PORT_EVENT_QUEUE_MIN_TASK_STACK_SIZE_BYTES, x)
#define QUEUE_STACK_SIZE_DEFAULT    U_PORT_EVENT_QUEUE_MIN_TASK_STACK_SIZE_BYTES

/** The maximum length of the Time Stamp string.
 * hh:mm:ss.mmm
 */
#define TIMESTAMP_MAX_LENTH_BYTES   13

#define OPERATOR_NAME_SIZE          20

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
    LOCATION_MEAS,
    MAX_STATUS
} applicationStates_t;

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

/* ----------------------------------------------------------------
 * EXTERNAL VARIABLES used in the application tasks
 * -------------------------------------------------------------- */

// serial number of the cellular module
extern char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

// This is the ubxlib deviceHandle for communicating with the celullar module
extern uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application's tasks should exit
extern bool gExitApp;

// This flag is for pausing the normal main loop activity
extern bool gPauseMainLoop;

// This flag represents the network's registration status
extern bool gIsNetworkUp;

// This flag represents the module can hear the network signaling (RSRP != 0)
extern bool gIsNetworkSignalValid;

// application status
extern applicationStates_t gAppStatus;

/// The unix network time, which is retrieved after first registration
extern int64_t unixNetworkTime;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
bool isMutexLocked(uPortMutexHandle_t mutex);
char *uStrDup(const char *src);
void *uMemDup(const void *data, size_t len);

int32_t sendAppTaskMessage(int32_t taskId, void *pMessage, size_t msgSize);

// Simple function to split message into command/params linked list
size_t getParams(char *message, commandParamsList_t **head);
void freeParams(commandParamsList_t *head);
int32_t getParamValue(commandParamsList_t *params, size_t index, int32_t minValue, int32_t maxValue, int32_t defValue);

void getTimeStamp(char *timeStamp);

void runTaskAndDelete(void *pParams);

#endif