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
 * Example header for another task
 *
 */

#ifndef _SENSOR_TASK_H_
#define _SENSOR_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t initSensorTask(taskConfig_t *config);
int32_t startSensorTaskLoop(commandParamsList_t *params);
int32_t stopSensorTaskLoop(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * PUBLIC TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t queueGetSensors(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    GET_SENSORS_NOW,            // Get the sensor values now
    SHUTDOWN_SENSOR_TASK,       // shuts down the 'task' by ending the mutex, queue and task.
} sensorMsgType_t;

// Some message types are just a command, so they wont need a param/struct
typedef struct {
    sensorMsgType_t msgType;

    union {
        const char *topicName;      // topic name to publish to
    } msg;
} sensorMsg_t;

#endif