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

#include "LEDTask.h"
#include "mqttTask.h"
#include "registrationTask.h"
#include "signalQualityTask.h"
#include "cellScanTask.h"
#include "locationTask.h"
#include "sensorTask.h"
#include "exampleTask.h"

static void setRedLED(void *param);

// These task runners define the application.
// Here we specify what tasks are to run, and what configuration they are to use
taskRunner_t taskRunners[] = {
    // Registration - Looks after the cellular registration process
    {initNetworkRegistrationTask, startNetworkRegistrationTaskLoop, stopNetworkRegistrationTaskLoop,
            {NETWORK_REG_TASK, "Registration", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, setRedLED}},

    // CellScan - Performs the +COPS=? Query for seeing what cells are available and publishes the results
    {initCellScanTask, startCellScanTaskLoop, stopCellScanTask,
            {CELL_SCAN_TASK, "CellScan", -1, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}},

    // MQTT - Handles the MQTT broker connection, publishing messages and handling downlink messages
    {initMQTTTask, startMQTTTaskLoop, stopMQTTTaskLoop,
            {MQTT_TASK, "MQTT", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}},

    // SignalQuality - Measures the Signal Quality and other network parameters and publishes the results
    {initSignalQualityTask, startSignalQualityTaskLoop, stopSignalQualityTaskLoop,
            {SIGNAL_QUALITY_TASK, "SignalQuality", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}},

    // LED - Handles the flashing of the LEDS depending on the AppStatus global variable
    {initLEDTask, startLEDTaskLoop, stopLEDTaskLoop,
            {LED_TASK, "LED", -1, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, setRedLED}},

    // Example - Simple example task that does "nothing"
    {initExampleTask, startExampleTaskLoop, stopExampleTaskLoop,
            {EXAMPLE_TASK, "Example", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}},

    // Location - Periodically gets the GNSS location of the device and publishes the results
    {initLocationTask, startLocationTaskLoop, stopLocationTaskLoop,
            {LOCATION_TASK, "Location", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}},

    // Sensor - Measures the sensor parameters and publishes the results
    {initSensorTask, startSensorTaskLoop, stopSensorTaskLoop,
            {SENSOR_TASK, "Sensor", 30, false, {NULL, NULL, U_ERROR_COMMON_UNKNOWN}, NULL}}
};

static void setRedLED(void *param)
{
    SET_RED_LED;
}

static taskRunner_t *getTaskRunner(taskTypeId_t id)
{
    for(size_t i=0; i<MAX_TASKS; i++) {
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

/// @brief Checks the "isTaskRunningxxxx()" functions and returns when the tasks have all stopped.
void waitForTasksToStop(taskTypeId_t *taskIds, size_t numTasks)
{
    bool stillWaiting;

    writeLog("Waiting for app tasks to stop...");
    do
    {
        stillWaiting = false;

        for(int i=0; i<numTasks; i++) {
            taskRunner_t *taskRunner = getTaskRunner(taskIds[i]);
            if (taskRunner == NULL) {
                printLog("task runner was null?");
                continue;
            }

            if (isMutexLocked(taskRunner->config.handles.mutexHandle)) {
                writeLog("...still waiting for %s task to finish", taskRunner->config.name);
                stillWaiting = true;
            }

            // give some time for the other tasks to print out
            // any debug information while they exit/unlock the mutex
            uPortTaskBlock(50);
        }

        uPortTaskBlock(2000);
    } while (stillWaiting);

    writeLog("All tasks are now finished...");
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

void stopAndWait(taskTypeId_t id)
{
    if (stopTask(id))
        waitForTaskToStop(id);
}

int32_t initSingleTask(taskTypeId_t id)
{
    int32_t errorCode;

    taskRunner_t *taskRunner = getTaskRunner(id);
    if (taskRunner == NULL) {
        printError("Task Runner is NULL!");
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    taskConfig_t *taskConfig = &taskRunner->config;

    if (!taskConfig->initialised) {
        errorCode = taskRunner->initFunc(taskConfig);
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

int32_t runTask(taskTypeId_t id)
{
    if (gExitApp) return -1;

    taskRunner_t *runner = getTaskRunner(id);
    if (runner == NULL) {
        printError("Failed to get task runner for task ID #%d, not running task", id);
        gExitApp = true;
        return -1;
    }

    int32_t errorCode = runner->startFunc(NULL);
    if (errorCode < 0) {
        printError("Failed to start task %s, error: %d", runner->config.name, errorCode);
    }

    return errorCode;
}

/// @brief Sends a task a message via its event queue
/// @param taskId The TaskId (based on the taskTypeId_t)
/// @param message pointer to the message to send
/// @param msgSize the size of the message to send
/// @return 0 on success, negative on failure
int32_t sendAppTaskMessage(int32_t taskId, void *pMessage, size_t msgSize)
{
    if (taskId < 0 || taskId >= MAX_TASKS) {
        printError("Send App Task [%d] Message Error: Invalid Task Id", taskId);
        return -1;
    }

    taskConfig_t *taskConfig = getTaskConfig(taskId);
    if (taskConfig == NULL) {
        printError("Failed to find task Id #%d", taskId);
        return -1;
    }

    // if the mutex or queue handle is not valid, don't queue a message
    if (!taskConfig->initialised) {
        printError("%s queue/task is not initialised, not queueing command", taskConfig->name);
        return -1;
    }

    int32_t errorCode = uPortEventQueueSendIrq(taskConfig->handles.eventQueueHandle, pMessage, msgSize);
    if (errorCode < 0) {
        // this is a debug message because this will only error if there is no room on the queue, but that
        // isn't an error, it's just what can happen.
        writeDebug("SendAppTaskMessage(): Message Error: Can't send message to %s task event queue: %d", taskConfig->name, errorCode);
    }

    return errorCode;
}