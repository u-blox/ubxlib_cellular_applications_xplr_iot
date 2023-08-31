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
 *  Task control functions - how the application initialises and runs the various tasks
 */

#include "common.h"
#include "leds.h"
#include "taskControl.h"
#include "LEDTask.h"
#include "mqttTask.h"
#include "registrationTask.h"
#include "signalQualityTask.h"
#include "cellScanTask.h"
#include "locationTask.h"
#include "sensorTask.h"
#include "exampleTask.h"

static void setRedLED(void *param);

/* ----------------------------------------------------------------
 * Task Runner Definitions for each appTask. These task runners 
 * define the application. Here we specify what tasks are to run, 
 * and what configuration they are to use
 * -------------------------------------------------------------- */

taskRunner_t taskRunners[] = {
    // Registration - Looks after the cellular registration process
    {initNetworkRegistrationTask, startNetworkRegistrationTaskLoop, stopNetworkRegistrationTaskLoop, true,
            {NETWORK_REG_TASK, "Registration", 30, false, BLANK_TASK_HANDLES, setRedLED}},

    // CellScan - Performs the +COPS=? Query for seeing what cells are available and publishes the results
    {initCellScanTask, startCellScanTaskLoop, stopCellScanTask, false,
            {CELL_SCAN_TASK, "CellScan", -1, false, BLANK_TASK_HANDLES, NULL}},

    // MQTT - Handles the MQTT broker connection, publishing messages and handling downlink messages
    {initMQTTTask, startMQTTTaskLoop, stopMQTTTaskLoop, false,
            {MQTT_TASK, "MQTT", 30, false, BLANK_TASK_HANDLES, NULL}},

    // SignalQuality - Measures the Signal Quality and other network parameters and publishes the results
    {initSignalQualityTask, startSignalQualityTaskLoop, stopSignalQualityTaskLoop, false,
            {SIGNAL_QUALITY_TASK, "SignalQuality", 30, false, BLANK_TASK_HANDLES, NULL}},

    // LED - Handles the flashing of the LEDS depending on the AppStatus global variable
    {initLEDTask, startLEDTaskLoop, stopLEDTaskLoop, false,
            {LED_TASK, "LED", -1, false, BLANK_TASK_HANDLES, setRedLED}},

    // Example - Simple example task that does "nothing"
    {initExampleTask, startExampleTaskLoop, stopExampleTaskLoop, false,
            {EXAMPLE_TASK, "Example", 30, false, BLANK_TASK_HANDLES, NULL}},

    // Location - Periodically gets the GNSS location of the device and publishes the results
    {initLocationTask, startLocationTaskLoop, stopLocationTaskLoop, false,
            {LOCATION_TASK, "Location", 30, false, BLANK_TASK_HANDLES, NULL}},

    // Sensor - Measures the sensor parameters and publishes the results
    {initSensorTask, startSensorTaskLoop, stopSensorTaskLoop, false,
            {SENSOR_TASK, "Sensor", 30, false, BLANK_TASK_HANDLES, NULL}}
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static void setRedLED(void *param)
{
    SET_RED_LED;
}

static taskRunner_t *getTaskRunner(taskTypeId_t id)
{
    for(size_t i=0; i<NUM_ELEMENTS(taskRunners); i++) {
        if (taskRunners[i].config.id == id)
            return &taskRunners[i];
    }

    return NULL;
}

static taskConfig_t *getTaskConfig(taskTypeId_t id)
{
    taskRunner_t *runner = getTaskRunner(id);
    if (runner == NULL) return NULL;

    return &(runner->config);
}

/// @brief Blocking function while waiting for the task to finish
/// @param taskConfig The task Configuration to wait for
static bool waitForTaskToStop(taskTypeId_t id)
{
    taskRunner_t *taskRunner = getTaskRunner(id);
    if (taskRunner == NULL) {
        writeFatal("Failed to find task %d", id);
        return false;
    }

    while(isMutexLocked(taskRunner->config.handles.mutexHandle)) {
        writeInfo("Waiting for %s task to stop...", taskRunner->config.name);
        uPortTaskBlock(2000);
    }

    return true;

}

static bool stopTask(taskTypeId_t id)
{
    taskRunner_t *runner = getTaskRunner(id);
    if (runner == NULL) {
        writeFatal("Failed to find task %d", id);
        return false;
    }

    int32_t errorCode = runner->stopFunc(NULL);
    if (errorCode != 0) {
        writeFatal("Stopping task %s returned error: %d", runner->config.name, errorCode);
        return false;
    }

    return true;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Checks the "isTaskRunningxxxx()" functions and returns when the tasks have all stopped.
void waitForAllTasksToStop()
{
    bool stillWaiting;

    writeLog("Waiting for app tasks to stop... This can take sometime if waiting for AT commands to timeout...");
    do
    {
        stillWaiting = false;

        for(int i=0; i<NUM_ELEMENTS(taskRunners); i++) {
            taskRunner_t *taskRunner = &taskRunners[i];
            if (!taskRunner->explicit_stop && isMutexLocked(taskRunner->config.handles.mutexHandle)) {
                printDebug("...still waiting for %s task to finish", taskRunner->config.name);
                stillWaiting = true;
            }
        }

        uPortTaskBlock(100);
    } while (stillWaiting);

    writeLog("All tasks have now finished...");
}

void stopAndWait(taskTypeId_t id)
{
    if (stopTask(id))
        waitForTaskToStop(id);
}

int32_t initSingleTask(taskTypeId_t id)
{
    taskRunner_t *taskRunner = getTaskRunner(id);
    if (taskRunner == NULL) {
        printError("Task Runner is NULL!");
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    taskConfig_t *taskConfig = &taskRunner->config;

    if (!taskConfig->initialised) {
        int32_t errorCode = taskRunner->initFunc(taskConfig);
        if (errorCode < 0) {
            writeFatal("* Failed to initialise the %s task (%d)", taskConfig->name, errorCode);
            return errorCode;
        }

        taskConfig->initialised = true;
    } else {
        printDebug("%s task has already been initialised", taskConfig->name);
    }

    return U_ERROR_COMMON_SUCCESS;
}

int32_t initTasks()
{
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;
    taskRunner_t *runner = taskRunners;

    for(int i=0; i<NUM_ELEMENTS(taskRunners); i++) {
        errorCode = initSingleTask(runner->config.id);
        if (errorCode < 0)
            break;

        runner++;
    }

    return errorCode;
}

int32_t runTask(taskTypeId_t id, bool (*waitForFunc)(void))
{
    if (gExitApp) return U_ERROR_COMMON_CANCELLED;

    taskRunner_t *runner = getTaskRunner(id);
    if (runner == NULL) {
        printError("Failed to get task runner for task ID #%d, not running task", id);
        return U_ERROR_COMMON_UNKNOWN;
    }

    int32_t errorCode = runner->startFunc(NULL);
    if (errorCode < 0) {
        printError("Failed to start task %s, error: %d", runner->config.name, errorCode);
        return errorCode;
    }

    if (waitForFunc != NULL && !waitFor(waitForFunc))
        return U_ERROR_COMMON_UNKNOWN; 

    return U_ERROR_COMMON_SUCCESS;
}

/// @brief Waits for a period of time and exits if the task is requested to exit/stop
/// @param taskConfig The task configuration that holds the dwell time
/// @param exitFunc The function that checks if the task should exit/stop
void dwellTask(taskConfig_t *taskConfig, bool (*canDoDwell)(void))
{
    // Always do a task block to give other tasks a run
    uPortTaskBlock(100);

    writeDebug("%s dwelling for %d seconds...", taskConfig->name, taskConfig->taskLoopDwellTime);

    // multiply by 10 as the TaskBlock is 100ms
    int32_t count = taskConfig->taskLoopDwellTime * 10;
    int i = 0;
    do {
        uPortTaskBlock(100);
        i++;
    } while (canDoDwell() && i < count);
}

/// @brief Sends a task a message via its event queue
/// @param taskId The TaskId (based on the taskTypeId_t)
/// @param message pointer to the message to send
/// @param msgSize the size of the message to send
/// @return 0 on success, negative on failure
int32_t sendAppTaskMessage(int32_t taskId, void *pMessage, size_t msgSize)
{
    taskConfig_t *taskConfig = getTaskConfig(taskId);
    if (taskConfig == NULL) {
        printError("Failed to find task Id #%d", taskId);
        return U_ERROR_COMMON_NOT_FOUND;
    }

    // if the mutex or queue handle is not valid, don't queue a message
    if (!taskConfig->initialised) {
        printError("%s queue/task is not initialised, not queueing command", taskConfig->name);
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    int32_t errorCode = uPortEventQueueSendIrq(taskConfig->handles.eventQueueHandle, pMessage, msgSize);
    if (errorCode < 0) {
        // this is a debug message because this will only error if there is no room on the queue, but that
        // isn't an error, it's just what can happen.
        writeDebug("SendAppTaskMessage(): %s task event queue probably full", taskConfig->name);
    }

    return errorCode;
}