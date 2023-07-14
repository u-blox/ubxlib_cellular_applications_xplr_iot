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
 * Registration task to look after the network connection
 *
*/

#include "common.h"
#include "taskControl.h"
#include "config.h"
#include "registrationTask.h"
#include "NTPClient.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define BEGINNING_2023 1672531200

#define REG_TASK_STACK_SIZE 1024
#define REG_TASK_PRIORITY 5

#define REG_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define REG_QUEUE_PRIORITY 5
#define REG_QUEUE_SIZE 5

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * FUNCTION DECLARATIONS
 * -------------------------------------------------------------- */
static bool keepGoing(void *pParam);

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static const uNetworkCfgCell_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_CELL,
    .pApn = APN,
    .pKeepGoingCallback = keepGoing,
    .timeoutSeconds = 240  // Connection timeout in seconds
};

static const uNetworkType_t gNetworkType = U_NETWORK_TYPE_CELL;

// This counts the number of times the network gets registered correctly
// At the start of the application this is zero until the first successful
// network registration. Until then this network manager will keep calling
// the UBXLIB uNetworkInterfaceUp() function
static volatile int32_t networkUpCounter = 0;

// This is the list of 'internet restricted' APNs. Normal internet
// communication on these APNs is not exercised, like requesting
// the date/time from a NTP service.
// TSUDP:- Thingstream MQTT-Anywhere dedicated APN.
static const char *restrictredAPNs[] = {
    "TSUDP",
};

/* ----------------------------------------------------------------
 * PUBLIC VARIABLES
 * -------------------------------------------------------------- */

// This flag represents the network's registration status
bool gIsNetworkUp = false;

/// The unix network time, which is retrieved after first registration
extern int64_t unixNetworkTime;

char pOperatorName[OPERATOR_NAME_SIZE] = "Unknown";
int32_t operatorMcc = 0;
int32_t operatorMnc = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void clearOperatorInfo(void)
{
    strncpy(pOperatorName, "Unknown", OPERATOR_NAME_SIZE);
    operatorMcc = 0;
    operatorMnc = 0;
}

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    if (!gIsNetworkUp) {
        gAppStatus = REGISTRATION_UNKNOWN;
        clearOperatorInfo();
    }

    return !gExitApp && !exitTask;
}

// This is here as it needs to be defined before the network cfg cell just below
static bool keepGoing(void *pParam)
{
    bool keepGoing = isNotExiting();
    if (keepGoing)
        printDebug("Still trying to register on a network...");
    else
        printDebug("Network registration cancelled");

    return keepGoing;
}

static int32_t getNetworkInfo(void)
{
    // request the PLMN / network operator information
    int32_t errorCode = uCellNetGetOperatorStr(gDeviceHandle, pOperatorName, OPERATOR_NAME_SIZE);
    if (errorCode < 0) {
        writeWarn("Failed to get operator name: %d", errorCode);
    } else {
        errorCode = uCellNetGetMccMnc(gDeviceHandle, &operatorMcc, &operatorMnc);
        if (errorCode < 0) {
            writeWarn("Failed to get MCC/MNC: %d", errorCode);
        }
    }

    return errorCode;
}

static void networkStatusCallback(uDeviceHandle_t devHandle,
                             uNetworkType_t netType,
                             bool isUp,
                             uNetworkStatus_t *pStatus,
                             void *pParameter)
{
    // count the number of times the network 'goes up'
    if (!gIsNetworkUp && isUp) {
        networkUpCounter++;

        gAppStatus = REGISTERED;
        getNetworkInfo();        
    }

    writeLog("Network Status: %s", isUp ? "Registered" : "Unknown");
    gIsNetworkUp = isUp;

    if (!isUp) {
        gAppStatus = REGISTRATION_UNKNOWN;
        clearOperatorInfo();
    }
}

static bool usingRestrictedAPN(void)
{
    for(int i=0; i<NUM_ELEMENTS(restrictredAPNs); i++) {
        if (strncmp(APN, restrictredAPNs[i], strlen(APN)) == 0)
            return true;
    }

    return false;
}

static void getNetworkOrNTPTime(void)
{
    // try and get the network time from the cellular network
    int64_t time = uCellInfoGetTimeUtc(gDeviceHandle);

    // if the network time is less than 2023, assume it is wrong
    // and get the time from NTP service
    if (time < BEGINNING_2023) {
        if (usingRestrictedAPN()) {
            writeWarn("Can't get NTP Time as we are on a restricted APN");
            return;
        } else {
            printInfo("Requesting time from NTP Server...");
            time = getNTPTime();
        }
    }

    // if time is positive, it should now be a valid time
    if (time > 0) {
        // remove the current local tick time from this
        // to obtain a rough "boot" time value.
        unixNetworkTime = time - (uPortGetTickTimeMs() / 1000);
    }
}

static int32_t startNetworkRegistration(void)
{
    gAppStatus = REGISTERING;
    writeLog("Bringing up the cellular network...");
    int32_t errorCode = uNetworkInterfaceUp(gDeviceHandle, gNetworkType, &gNetworkCfg);
    if (errorCode != 0) {
        writeWarn("Failed to bring up the cellular network: %d", errorCode);
        return errorCode;
    }

    errorCode = uNetworkSetStatusCallback(gDeviceHandle, gNetworkType, networkStatusCallback, NULL);
    if (errorCode != 0) {
        writeError("Failed to set the network status callback: %d", errorCode);
        return errorCode;
    }

    gIsNetworkUp = true;
    gAppStatus = REGISTERED;
    networkUpCounter=1;

    getNetworkInfo();
    writeLog("Connected to Cellular Network: %s (%03d%02d)", pOperatorName, operatorMcc, operatorMnc);
    return 0;
}

static int32_t deRegisterFromNetwork(void)
{
    gAppStatus = REGISTERING;
    writeLog("Deregistering from the network...");
    int32_t errorCode = uNetworkInterfaceDown(gDeviceHandle, gNetworkType);
    if (errorCode != 0) {
        writeWarn("Failed to de-register from the cellular network: %d", errorCode);
    } else {
        writeLog("Deregistered from cellular network");
        gIsNetworkUp = false;
    }

    return errorCode;
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // does nothing... at the moment.
}

// Network registration manager task loop
static void taskLoop(void *pParameters)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);

    // We won't exit this task loop until we are specifically told to
    // as other tasks may need to close their cloud connections and when
    // this task finishes the device disconnects from the network.
    while(!exitTask) {
        // if the application is exiting, we don't need to try and
        // manage the network connection... just wait here until we are TOLD to exit
        if (!gExitApp) {
            // If we've never seen the network has been up before, start the reg process...
            if (networkUpCounter==0) {
                if (startNetworkRegistration() == 0) {
                    getNetworkOrNTPTime();
                }
            }

            // logging tick about the network reg status
            if (gIsNetworkUp) {
                writeDebug("Network is up and running");
            } else {
                gAppStatus = REGISTRATION_UNKNOWN;
                writeLog("Unknown network registration state");
                // What can we do here? not a lot.
            }

            dwellTask(taskConfig, isNotExiting);
        }

        // Just spin the task blocking here.
        // No need to use the dwellTask()
        uPortTaskBlock(50);
    }

    // we've been asked to exit the Network Manager, so go through
    // the de-registration process before finishing
    deRegisterFromNetwork();

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    FINALIZE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(registrationMsg_t),
                    REG_QUEUE_STACK_SIZE,
                    REG_QUEUE_PRIORITY,
                    REG_QUEUE_SIZE);

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

/// @brief Initialises the registration task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initNetworkRegistrationTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successful, a negative number otherwise
int32_t startNetworkRegistrationTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;
    START_TASK_LOOP(REG_TASK_STACK_SIZE, REG_TASK_PRIORITY);
}

int32_t stopNetworkRegistrationTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}