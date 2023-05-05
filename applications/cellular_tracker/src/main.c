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
 * Cellular tracking application based on the XPLR-IoT-1 device
 * Connects to an MQTT broker and publishes:
 *      o cellular rsrp/rsrq measurements
 *      o cellular CellID
 *      o GNSS Location
 *
 */

#include "common.h"
#include "config.h"
#include "ext_fs.h"
#include "leds.h"
#include "buttons.h"

#include "taskControl.h"
#include "mqttTask.h"
#include "cellScanTask.h"
#include "signalQualityTask.h"
#include "locationTask.h"

/* ----------------------------------------------------------------
 * DEBUG LEVEL SETTING - This can be changed remotely using
 *                       "SET_LOG_LEVEL" command via the MQTT topic
 * -------------------------------------------------------------- */
#define LOGGING_LEVEL eINFO            // logLevels_t

/* ----------------------------------------------------------------
 * FUNCTION DECLARATIONS
 * -------------------------------------------------------------- */
static int32_t setAppDwellTime(commandParamsList_t *params);
static int32_t setAppLogLevel(commandParamsList_t *params);

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define STARTUP_DELAY 250        // 250 * 20ms => 5 seconds
#define LOG_FILENAME "log.csv"
#define CONFIG_FILENAME "config.txt"

// Dwell time of the main loop activity, pause period until the loop runs again
#define APP_DWELL_TIME_MS_MINIMUM 5000
#define APP_DWELL_TIME_MS_DEFAULT APP_DWELL_TIME_MS_MINIMUM;
#define APP_DWELL_TICK_MS 50

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    NO_BUTTON=-1,
    BUTTON_1=0,
    BUTTON_2=1
} buttonNumber_t;

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */

applicationStates_t gAppStatus = MANUAL;

// serial number of the module
char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

// deviceHandle is not static as this is shared between other modules.
uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application should close tasks and log files.
// This flag is set to true when Button #1 is pressed.
bool gExitApp = false;
bool gExitFast = false; // don't wait for closing app Tasks, just close the log file and exit.

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uDeviceType_t deviceType = U_DEVICE_TYPE_CELL;
static uDeviceCfg_t deviceCfg;

static bool buttonCommandEnabled = false;
static buttonNumber_t pressedButton = NO_BUTTON;

static int32_t appDwellTimeMS = 5000;

/* ----------------------------------------------------------------
 * Remote control callbacks for the main application
 * -------------------------------------------------------------- */
#define APP_CONTROL_TOPIC "AppControl"
static callbackCommand_t callbacks[] = {
    {"SET_DWELL_TIME", setAppDwellTime},
    {"SET_LOG_LEVEL", setAppLogLevel}
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t setAppDwellTime(commandParamsList_t *params)
{
    int32_t timeMS = getParamValue(params, 1, 5000, 60000, 30000);

    if (timeMS < APP_DWELL_TIME_MS_MINIMUM) {
        writeWarn("Failed to set App Dwell Time, %d is less than minimum (%d ms)", timeMS, APP_DWELL_TIME_MS_MINIMUM);
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    appDwellTimeMS = timeMS;
    writeLog("Setting App Dwell Time to: %d\n", timeMS);

    return U_ERROR_COMMON_SUCCESS;
}

static int32_t setAppLogLevel(commandParamsList_t *params)
{
    logLevels_t logLevel = (logLevels_t) getParamValue(params, 1, (int32_t) eTRACE, (int32_t) eMAXLOGLEVELS, (int32_t) eINFO);

    if (logLevel < eTRACE) {
        writeWarn("Failed to set App Log Level %d. Min: %d, Max: %d", logLevel, eTRACE, eMAXLOGLEVELS);
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    setLogLevel(logLevel);

    return U_ERROR_COMMON_SUCCESS;
}

/// @brief Handler for the buttons. On boot, Button 1: Display log, 2: Delete log.
static void button_pressed(int buttonNo, uint32_t holdTime)
{
    if (gExitApp)
        return;

    if (!holdTime) {
        pressedButton = buttonNo;
    } else {
        pressedButton = NO_BUTTON;
        if (!buttonCommandEnabled)
            return;

        switch (buttonNo) {
            case BUTTON_1:
                if (holdTime > 5000) {
                    writeLog("Fast exit requested - closing log now! Please wait for the RED LED to go out...");
                    gExitFast = true;
                } else {
                    writeLog("Exit button pressed, closing down... Please wait for the RED LED to go out...");
                }
                gExitApp = true;
                break;

            case BUTTON_2:
                queueNetworkScan(NULL);
                break;

            default:
                break;
            }
    }
}

/// @brief Function to check for a held button at start up.
/// @return The button that was held at start up.
static buttonNumber_t checkStartButton(void)
{
    SET_RED_LED;

    // just wait 3 seconds for any terminal to be connected
    uPortTaskBlock(3000);

    printLog("Button #1 = Display Log file");
    printLog("Button #2 = Delete  Log File");
    printLog("Waiting 5 seconds for selection before running application...");

    SET_BLUE_LED;
    // wait for 5 seconds, or if a button is pressed
    for(int i = 0; pressedButton == NO_BUTTON && i < STARTUP_DELAY; i++)
        uPortTaskBlock(20);

    SET_NO_LEDS;

    buttonNumber_t heldButton = pressedButton;
    if (heldButton == NO_BUTTON)
        return NO_BUTTON;

    // turn on the correct LED for the operation
    int led = 0;  // Delete log file = red
    if (heldButton == BUTTON_1)
        led = 1; // Display log file = green
    ledSet(led, true);

    // wait until the button is released
    while(pressedButton != NO_BUTTON)
        uPortTaskBlock(20);

    return heldButton;
}

/// @brief Reads the serial number from the module
/// @return The length of the serial number, or negative on error
static int32_t getSerialNumber(void)
{
    int32_t len = uSecurityGetSerialNumber(gDeviceHandle, gSerialNumber);
    if (len > 0) {
        if (gSerialNumber[0] == '"') {
            // Remove quotes
            memmove(gSerialNumber, gSerialNumber + 1, len);
            gSerialNumber[len - 2] = 0;
        }

        writeLog("Cellular Module Serial Number: %s", gSerialNumber);
    }

    return len;
}

/// @brief Initiate the UBXLIX API
static int32_t initCellularDevice(void) {
    int32_t errorCode;

    // turn off the UBXLIB printLog() logging
    uPortLogOff();

    writeLog("Initiating the UBXLIB Device API...");
    errorCode = uDeviceInit();
    if (errorCode != 0) {
        writeFatal("* Failed to initiate the UBXLIB device API: %d", errorCode);
        return errorCode;
    }

    uDeviceGetDefaults(deviceType, &deviceCfg);

    writeLog("Opening/Turning on the cellular module...");
    errorCode = uDeviceOpen(&deviceCfg, &gDeviceHandle);
    if (errorCode != 0) {
        writeFatal("* Failed to turn on the cellular module: %d", errorCode);
        return errorCode;
    }

    // get serial number
    errorCode = getSerialNumber();
    if (errorCode < 0) {
        writeFatal("* Failed to get the serial number of the module: %d", errorCode);
        return errorCode;
    }

    return 0;
}

/// @brief Sets the application status, waits for the tasks and closes the log
/// @param appState The application status to set for the shutdown
static void finalise(applicationStates_t appState)
{
    gAppStatus = appState;
    gExitApp = true;

    // stop the other tasks from flashing the LEDs
    stopAndWait(LED_TASK);

    // if we don't want a fast shutdown, be nice and close down
    // each appTask and wait for them to finish
    if (!gExitFast) {
        // Tell the common tasks to finish first
        taskTypeId_t checkTasks[] = {SIGNAL_QUALITY_TASK,
                                    MQTT_TASK,
                                    CELL_SCAN_TASK,
                                    EXAMPLE_TASK,
                                    LOCATION_TASK,
                                    SENSOR_TASK};

        waitForTasksToStop(checkTasks, NUM_ELEMENTS(checkTasks));

        // now stop the network registration task. Blue LED
        SET_BLUE_LED;
        stopAndWait(NETWORK_REG_TASK);
    }

    // stop the schedular, we're done!
    Z_SECTION_LOCK
        closeLog();

        uPortDeinit();

        SET_NO_LEDS;
        printLog("XPLR App has finished.");

        // application stays here forever
        while(1);
    Z_SECTION_UNLOCK
}


/// @brief Initialises the XPLR device LEDs, Buttons and file system and handles the startup button press
static bool initXplrDevice(void)
{
    if (uPortInit() != 0) {
        printFatal("* Failed to initiate UBXLIB - not running application!");
        return false;
    }
    if (!buttonsInit(button_pressed)) {
        printFatal("* Failed to initiate buttons - not running application!");
        return false;
    }
    if (!ledsInit()) {
        printFatal("* Failed to initiate leds - not running application!");
        return false;
    }
    if (!extFsInit()) {
        printFatal("* Failed to mounth File System - not running application!");
        return false;
    }

    // User has chance to hold down a button to delete or display the log
    buttonNumber_t button = checkStartButton();

    // deleting the log file is performed now before the start of the application
    if (button == BUTTON_2) {
        printLog("Deleting log file...");
        deleteFile(LOG_FILENAME);
    }

    setLogLevel(LOGGING_LEVEL);
    startLogging(LOG_FILENAME);

    // displaying the log file ends the application
    if (button == BUTTON_1) {
        displayLogFile();
        SET_NO_LEDS;
        printLog("Application finished");
        return false;
    }

    // Display the file system free size
    displayFileSpace(LOG_FILENAME);

    // Save the configuration file (if present)
    int32_t saveResult = saveConfigFile(CONFIG_FILENAME);
    if (saveResult != 0 && saveResult != U_ERROR_COMMON_NOT_FOUND) {
        printFatal("Aborting application as configuration file was not written");
        return false;
    }

    // Load the configuration file
    loadConfigFile(CONFIG_FILENAME);

    // now allow the buttons to run their commands
    buttonCommandEnabled = true;

    return true;
}

/// @brief Dwells for appDwellTimeMS time, and exits if this time changes
static void dwellAppLoop()
{
    int32_t tick = 0;
    int32_t dwellTimeMS = appDwellTimeMS;

    do
    {
        uPortTaskBlock(APP_DWELL_TICK_MS);
        tick = tick + APP_DWELL_TICK_MS;
    } while((tick < dwellTimeMS) &&
            (dwellTimeMS == appDwellTimeMS));
}



/// @brief Main entry to the application.
/// If Button 1 is held, the log file is displayed.
/// If Button 2 is held, the log file is deleted.
/// In both cases, the application starts as normal after.
void main(void)
{
    int32_t errorCode;

    // initialise our LEDs and start up button commmands
    if (!initXplrDevice())
        return;

    writeLog("Starting LED Task...");
    errorCode = initSingleTask(LED_TASK);
    if (errorCode < 0) {
        writeFatal("* Failed to initialise LED task - not running application!");
        return finalise(ERROR);
    }
    errorCode = runTask(LED_TASK);
    if (errorCode != 0) {
        writeFatal("* Failed to start LED task - not running application!");
        return finalise(ERROR);
    }

    // initialise the cellular module
    gAppStatus = INIT_DEVICE;
    errorCode = initCellularDevice();
    if (errorCode != 0) {
        writeFatal("* Failed to initialise the cellular module - not running application!");
        return finalise(ERROR);
    }

    // Initialise the task runners
    if (initTasks() != 0) {
        return finalise(ERROR);
    }

    // Run the two main tasks for this application, Network and MQTT tasks.
    runTask(NETWORK_REG_TASK);
    runTask(MQTT_TASK);

    subscribeToTopicAsync(APP_CONTROL_TOPIC, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    // Main application loop
    while(!gExitApp) {
        printDebug("*** Application Tick ***\n");

        dwellAppLoop();

        // exercise task operations
        if (gAppStatus != COPS_QUERY) {
            queueMeasureNow(NULL);
            queueLocationNow(NULL);
        }
    }

    writeLog("Application closing down...");
    finalise(SHUTDOWN);
}
