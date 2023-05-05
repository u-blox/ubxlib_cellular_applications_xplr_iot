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

#ifndef _EXAMPLE_TASK_H_
#define _EXAMPLE_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t initExampleTask(taskConfig_t *config);
int32_t startExampleTaskLoop(commandParamsList_t *params);
int32_t stopExampleTaskLoop(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * PUBLIC TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t queueExampleCommand(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    RUN_EXAMPLE                // Run the example command
} exampleMsgType_t;

// Some message types are just a command, so they wont need a param/struct
typedef struct {
    exampleMsgType_t msgType;

    union {
        const char *topicName;      // topic name to publish to
    } msg;
} exampleMsg_t;

#endif