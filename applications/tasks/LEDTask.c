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
 * LED task to light up and flash the three LEDs.
 *
 */

#include "common.h"
#include "taskControl.h"
#include "LEDTask.h"
#include "leds.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define LED_TASK_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define LED_TASK_PRIORITY 5

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC VARIABLES
 * -------------------------------------------------------------- */

// Application Status is mapped to this LED configuration array
ledAppState_t ledAppStatus[] = {
    {MANUAL, {ledRedOff, ledGreenOff, ledBlueOff}},
    {INIT_DEVICE, {ledRedFastPulse, ledGreenOff, ledBlueOff}},
    {REGISTERING, {ledRedOff, ledGreenOff, ledBluePulse}},
    {MQTT_CONNECTING, {ledRedOff, ledGreenPulse, ledBlueOff}},
    {COPS_QUERY, {ledRedOff, ledGreenPulse, ledBlueOn}},
    {SEND_SIGNAL_QUALITY, {ledRedFlash, ledGreenFlash, ledBlueOff}},
    {REGISTRATION_UNKNOWN, {ledRedOff, ledGreenOff, ledBlueFlash}},
    {REGISTERED, {ledRedOff, ledGreenOff, ledBlueOn}},
    {ERROR, {ledRedOn, ledGreenOff, ledBlueOff}},
    {SHUTDOWN, {ledRedOn, ledGreenOn, ledBlueOn}},
    {MQTT_CONNECTED, {ledRedOff, ledGreenOn, ledBlueOff}},
    {MQTT_DISCONNECTED, {ledRedOff, ledGreenFlash, ledBlueOff}},
    {START_SIGNAL_QUALITY, {ledRedOff, ledGreenOff, ledBlueOn}},
    {LOCATION_MEAS, {ledRedOn, ledGreenOn, ledBlueOn}},
    {REGISTRATION_DENIED, {ledRedPulse, ledGreenOff, ledBluePulse}},
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // does nothing, at the moment.
}

ledCfg_t *getAppStatusLEDs()
{
    static applicationStates_t previousAppStatus = 0;
    static ledCfg_t *previousLedCfg = NULL;

    if (gAppStatus == previousAppStatus && previousLedCfg != NULL)
        return previousLedCfg;

    previousAppStatus = gAppStatus;

    for(int i=0; i<NUM_ELEMENTS(ledAppStatus); i++) {
        ledAppState_t *las = &ledAppStatus[i];
        if (las->appState == gAppStatus) {
            previousLedCfg = las->ledCfg;
            return las->ledCfg;
        }
    }

    writeWarn("Can't find LED setting for app Status %d", gAppStatus);
    return NULL;
}

// LED task loop for turning on and off the LEDs according to the gAppStatus
static void taskLoop(void *pParameters)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    while(!gExitApp && !exitTask) {
        int priorityLED = -1;

        // grab the leds for this app status
        ledCfg_t *leds = getAppStatusLEDs();
        if (leds == NULL) {
            writeDebug("Didn't find LEDS for this status: %d", gAppStatus);
            return;
        }

        // Calculate LED states and select priority LED
        for(int l = 0; l < MAX_LEDS; l++) {
            ledCfg_t *led = &leds[l];
            led->state = (led->timer < led->duty) != led->invert;
            led->timer = led->timer + ledTick_ms;
            if (led->timer >= led->period)
                led->timer = 0;
            if (priorityLED == -1 && led->priority && led->state)
                priorityLED = led->n;
        }

        // Set each LED state, priority LED overriding others.
        for(int l = 0; l < MAX_LEDS; l++) {
            ledCfg_t *led = &leds[l];
            if (priorityLED == -1)
                ledSet(led->n, led->state);
            else
                if (priorityLED == led->n)
                    ledSet(led->n, true);
                else
                    ledSet(led->n, false);
        }

        uPortTaskBlock(ledTick_ms);
    }

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    FINALIZE_TASK;
}


static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(LEDConfigMsg_t),
                    LED_TASK_STACK_SIZE,
                    1,
                    1);

    if (eventQueueHandle < 0) {
        writeLog("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
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

/// @brief Initialises the LED task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initLEDTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);

    return result;
}

/// @brief Starts the LED Task loop
/// @return zero if successful, a negative number otherwise
int32_t startLEDTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;
    START_TASK_LOOP(LED_TASK_STACK_SIZE, LED_TASK_PRIORITY);
}

/// @brief Stop the network manager and deregister from the cellular network
int32_t stopLEDTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}