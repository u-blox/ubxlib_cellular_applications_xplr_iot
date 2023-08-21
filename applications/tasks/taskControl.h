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
 * Task Control header
 *
 */

#ifndef _TASK_CONTROL_H_
#define _TASK_CONTROL_H_

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */
#define BLANK_TASK_HANDLES {NULL, NULL, U_ERROR_COMMON_UNKNOWN}

#define EXIT_ON_FAILURE(x)      result = x(); if (result < 0) return result
#define CLEANUP_ON_ERROR(x)     if (errorCode == 0)     \
                                    errorCode = x();    \
                                else                    \
                                    goto cleanUp;

#define TASK_MUTEX  taskConfig->handles.mutexHandle
#define TASK_HANDLE taskConfig->handles.taskHandle
#define TASK_QUEUE  taskConfig->handles.eventQueueHandle
#define TASK_NAME   taskConfig->name
#define TASK_ID     taskConfig->id

#define TASK_INITIALISED        ((taskConfig != NULL) && taskConfig->initialised)

#define CREATE_TOPIC_NAME       snprintf(topicName, MAX_TOPIC_NAME_SIZE, "%s/%s", (const char *)gSerialNumber, TASK_NAME)

#define EXIT_IF_CANT_RUN_TASK   if (taskConfig == NULL || !TASK_INITIALISED) {                          \
                                    writeWarn("%s task is not initialised yet, not starting.",          \
                                            TASK_NAME);                                                 \
                                    return U_ERROR_COMMON_NOT_INITIALISED;                              \
                                }                                                                       \
                                if (TASK_HANDLE != NULL) {                                              \
                                    writeWarn("%s task is already running, not starting again.",        \
                                            TASK_NAME);                                                 \
                                    return U_ERROR_COMMON_SUCCESS;                                      \
                                }

#define EXIT_IF_CONFIG_NULL     if (config == NULL) {                                                   \
                                    writeError("Cannot initialise task as configuration is NULL");      \
                                    return U_ERROR_COMMON_INVALID_PARAMETER;                            \
                                }

#define START_TASK              int32_t errorCode = startTask();                                        \
                                if (errorCode == 0) {                                                   \
                                    writeLog("Started %s task loop", TASK_NAME);                        \
                                }                                                                       \
                                return errorCode;

#define STOP_TASK               if (taskConfig == NULL) {                                               \
                                    writeDebug("Stop %s task requested, but it is not initialised");    \
                                    return U_ERROR_COMMON_NOT_INITIALISED;                              \
                                }                                                                       \
                                exitTask = true;                                                        \
                                writeLog("Stop %s task requested...", taskConfig->name);                \
                                return U_ERROR_COMMON_SUCCESS;

#define INIT_MUTEX              int32_t errorCode = uPortMutexCreate(&TASK_MUTEX);                      \
                                if (errorCode != 0) {                                                   \
                                    writeFatal("Failed to create %s Mutex (%d).",                       \
                                            TASK_NAME, errorCode);                                      \
                                }                                                                       \
                                return errorCode;

#define RUN_FUNC(func, stackSize, priority)                                                             \
                                uPortTaskHandle_t taskHandle;                                           \
                                int32_t errorCode = uPortTaskCreate(runTaskAndDelete, TASK_NAME,        \
                                            stackSize, func, priority, &taskHandle);                    \
                                if (errorCode < 0) {                                                    \
                                    writeError("Failed to start %s task function: %d",                  \
                                            TASK_NAME, errorCode); }

#define START_TASK_LOOP(stackSize, priority)                                                            \
                                int32_t errorCode = uPortTaskCreate(runTaskAndDelete, TASK_NAME,        \
                                            stackSize, taskLoop, priority, &TASK_HANDLE);               \
                                if (errorCode != 0) {                                                   \
                                    writeError("Failed to start the %s Task (%d).",                     \
                                            TASK_NAME, errorCode);                                      \
                                }                                                                       \
                                return errorCode;

#define FINALIZE_TASK           writeDebug("%s task loop has stopped", TASK_NAME);                      \
                                if (taskConfig->taskStoppedCallback != NULL) {                          \
                                        writeDebug("Running %s task stopped callback...", TASK_NAME);   \
                                        taskConfig->taskStoppedCallback(NULL);                          \
                                }                                                                       \
                                TASK_HANDLE = NULL;

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */

typedef struct TaskHandles {
    uPortTaskHandle_t taskHandle;
    uPortMutexHandle_t mutexHandle;
    int32_t eventQueueHandle;
} taskHandles_t;

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

typedef int32_t (*taskInit_t)(taskConfig_t *taskConfig);
typedef int32_t (*taskStart_t)(commandParamsList_t *params);
typedef int32_t (*taskStop_t)(commandParamsList_t *params);

typedef struct TaskRunner {
    taskInit_t initFunc;
    taskStart_t startFunc;
    taskStop_t stopFunc;
    bool explicit_stop;
    taskConfig_t config;
} taskRunner_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
int32_t initTasks();
int32_t initSingleTask(taskTypeId_t id);

int32_t runTask(taskTypeId_t id);

void dwellTask(taskConfig_t *taskConfig, bool (*exitFunc)(void));

void stopAndWait(taskTypeId_t id);
void waitForAllTasksToStop(void);

#endif