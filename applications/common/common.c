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
 * Common utility functions
 *
 */
#include <time.h>

#include "common.h"

/* ----------------------------------------------------------------
 * DEFINITITIONS
 * -------------------------------------------------------------- */
#define PARAM_DELIMITERS " ,:"

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief Checks if the mutex is locked/running
/// @param mutex the mutex to check
/// @return false if it not locked/running, true if it is locked/running.
bool isMutexLocked(uPortMutexHandle_t mutex)
{
    if(mutex == NULL) {
        return false;
    }

    int32_t result = uPortMutexTryLock(mutex, 0);
    if (result != 0)
    {
        return true; // Can't get a lock, so it must be running
    }

    // we have locked the mutex, so release we need to release it
    if (uPortMutexUnlock(mutex) != 0) {
        printFatal("Failed to release mutex from lock check!!!");
        // now you've done it. You've tested the mutex lock
        // by locking it and now can't unlock it!!
        return true;
    }

    // As we could get a lock, the task is demeed "not running"
    return false;
}

/// @brief Duplicates a string via malloc - remember to free()!
/// @param src the string source
/// @returns pointer to the duplicated string, or NULL
char *uStrDup(const char *src)
{
    return (char *)uMemDup((void *)src, strlen(src)+1);
}

/// @brief Duplicates a block of data via malloc - remember to free()!
/// @param src the data source
/// @returns pointer to the duplicated data, or NULL
void *uMemDup(const void *src, size_t len)
{
    void *dst = pUPortMalloc(len);
    if(dst == NULL) {
        writeError("No memory for data duplication");
        return NULL;
    }

    memcpy(dst, src, len);
    return dst;
}

static commandParamsList_t *createParam(char *param)
{
    commandParamsList_t *newParam = (commandParamsList_t *)pUPortMalloc(sizeof(commandParamsList_t));
    if (newParam == NULL) {
        writeError("No memory for createParam()");
        return NULL;
    }

    newParam->parameter = uMemDup(param, strlen(param)+1);
    if (newParam->parameter == NULL) {
        writeError("No memory for createParam()");
        return NULL;
    }

    newParam->pNext = NULL;

    return newParam;
}

/// @brief Puts the command and param parts of a string message into a structure
/// @param message The string to parse into Command: param1, param2 etc
/// @param params The parameters for the command
/// @returns Number of parameters
size_t getParams(char *message, commandParamsList_t **head)
{
    char *token = strtok_r(message, PARAM_DELIMITERS, &message);
    if (token == NULL) {
        printWarn("Unable to parse message for command/params");
        return -1;
    }

    *head = createParam(token);
    if (head == NULL) {
        writeError("No memory for getParams()");
        return 0;
    }

    commandParamsList_t *current = *head;
    size_t count=1;

    while((token = strtok_r(NULL, PARAM_DELIMITERS, &message)) != NULL) {
        current->pNext = createParam(token);
        current = current->pNext;
        count++;
    }

    return count;
}

void freeParams(commandParamsList_t *item)
{
    if (item == NULL) {
        return;
    }

    freeParams(item->pNext);
    uPortFree(item->parameter);
    uPortFree(item);
}

int32_t getParamValue(commandParamsList_t *params, size_t index, int32_t minValue, int32_t maxValue, int32_t defValue)
{
    char *ptr;
    commandParamsList_t *param = params;
    for(int i=0; i<index && param != NULL; i++)
        param = params->pNext;

    if (param == NULL)
        return defValue;

    int32_t value = strtol(param->parameter, &ptr, 10);
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;

    return value;
}

/// @brief Notates the timestamp from the network time or boot tick time
/// @param timeStamp The string to write the timestamp to. Must be minimum size of TIMESTAMP_MAX_LENTH_BYTES
void getTimeStamp(char *timeStamp)
{
    // construct the log format, and include the ticks time
    int32_t ticks = uPortGetTickTimeMs();

    // if we have the network time set use this
    if (unixNetworkTime > 0) {
        time_t tmTime = unixNetworkTime + (ticks/1000);
        int32_t milliseconds = ticks % 1000;
        struct tm *time = gmtime(&tmTime);
        snprintf(timeStamp, TIMESTAMP_MAX_LENTH_BYTES, "%02d:%02d:%02d.%03d",
                                    time->tm_hour,
                                    time->tm_min,
                                    time->tm_sec,
                                    milliseconds);
    } else {
        snprintf(timeStamp, TIMESTAMP_MAX_LENTH_BYTES, "%d", ticks);
    }
}

void runTaskAndDelete(void *pParams)
{
    if (pParams != NULL) {
        void (*func)(void) = pParams;
        func();
    } else {
        writeWarn("No Task to run!");
    }

    uPortTaskDelete(NULL);
    uPortTaskBlock(2);
}