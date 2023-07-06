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
#include "exampleTask.h"
#include "mqttTask.h"
// #include "otherheaders...h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
// not all tasks will have a task loop if it only uses a queue
#define EXAMPLE_TASK_STACK_SIZE (1 * 1024)
#define EXAMPLE_TASK_PRIORITY 5

#define EXAMPLE_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define EXAMPLE_QUEUE_PRIORITY 5
#define EXAMPLE_QUEUE_SIZE 1

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
    {"RUN_EXAMPLE", queueExampleCommand},
    {"START_TASK", startExampleTaskLoop},
    {"STOP_TASK", stopExampleTaskLoop}
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask;
}

static void doExampleThing(void *pParams)
{
    // example thing to run/process/publish
    printLog("Example Function !");
}

static void startExampleThing(void)
{
    RUN_FUNC(doExampleThing, EXAMPLE_TASK_STACK_SIZE, EXAMPLE_TASK_PRIORITY);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // cast the incoming pParam to the proper param structure
    exampleMsg_t *qMsg = (exampleMsg_t *) pParam;

    // do the message handling...
    switch(qMsg->msgType) {
        case RUN_EXAMPLE:
            startExampleThing();
            break;

        default:
            writeWarn("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

// Task loop where the activity is made and the dwell time is taken
static void taskLoop(void *pParameters)
{
    while(isNotExiting()) {
        doExampleThing(NULL);
        dwellTask(taskConfig, isNotExiting);
    }

    FINALIZE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(exampleMsg_t),
                    EXAMPLE_QUEUE_STACK_SIZE,
                    EXAMPLE_QUEUE_PRIORITY,
                    EXAMPLE_QUEUE_SIZE);

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
/// @brief Queue the getLocation operation
/// @param params The parameters for this command
/// @return returns the errorCode of sending the message on the eventQueue
int32_t queueExampleCommand(commandParamsList_t *params)
{
    exampleMsg_t qMsg;
    qMsg.msgType = RUN_EXAMPLE;

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(exampleMsg_t));
}

/// @brief Initialises the Signal Quality task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initExampleTask(taskConfig_t *config)
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
int32_t startExampleTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    if (params != NULL)
        taskConfig->taskLoopDwellTime = getParamValue(params, 1, 5, 60, 30);

    START_TASK_LOOP(EXAMPLE_TASK_STACK_SIZE, EXAMPLE_TASK_PRIORITY);
}

int32_t stopExampleTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}