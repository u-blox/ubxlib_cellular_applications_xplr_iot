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
 * Location Task header
 *
 */

#ifndef _LOCATION_TASK_H_
#define _LOCATION_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t initLocationTask(taskConfig_t *config);
int32_t startLocationTaskLoop(commandParamsList_t *params);
int32_t stopLocationTaskLoop(commandParamsList_t *params);
int32_t finalizeLocationTask(void);

/* ----------------------------------------------------------------
 * PUBLIC TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t queueLocationNow(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    GET_LOCATION_NOW,               // Get the position now
    STOP_LOCATON_ACQUISITION,       // Stops the current location acquisition
    SHUTDOWN_LOCATION_TASK,         // shuts down the 'task' by ending the mutex, queue and task.
} locationMsgType_t;

// Some message types are just a command, so they wont need a param/struct
typedef struct {
    locationMsgType_t msgType;

    union {
        const char *topicName;      // topic name to publish to
    } msg;
} locationMsg_t;

#endif