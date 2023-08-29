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

#include "common.h"
#include "taskControl.h"
#include "cellInit.h"
#include "config.h"
#include "ext_fs.h"
#include "leds.h"
#include "buttons.h"

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    NO_BUTTON=-1,
    BUTTON_1=0,
    BUTTON_2=1
} buttonNumber_t;

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define STARTUP_DELAY 250       // 250 * 20ms => 5 seconds
#define LOG_FILENAME "log.csv"
#define MQTT_CREDENTIALS_FILENAME "mqttCredentials.txt"

// Dwell time of the main loop activity, pause period until the loop runs again
#define APP_DWELL_TIME_MS_MINIMUM 5000
#define APP_DWELL_TIME_MS_DEFAULT APP_DWELL_TIME_MS_MINIMUM;
#define APP_DWELL_TICK_MS 50

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uDeviceType_t deviceType = U_DEVICE_TYPE_CELL;
static uDeviceCfg_t deviceCfg;

static bool buttonCommandEnabled = false;
static buttonNumber_t pressedButton = NO_BUTTON;

static int32_t appDwellTimeMS = 5000;

// This flag will pause the main application loop
static bool pauseMainLoopIndicator = false;

static bool appFinalized = false;


/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */
// serial number of the module
char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

applicationStates_t gAppStatus = MANUAL;

// deviceHandle is not static as this is shared between other modules.
uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application should close tasks and log files.
// This flag is set to true when Button #1 is pressed.
bool gExitApp = false;

void (*buttonTwoFunc)(void) = NULL;

// reference to our mqtt credentials which are used for the application's publish/subscription
extern const char *mqttCredentials[];
extern int32_t mqttCredentialsSize;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Function to check for a held button at start up.
/// @return The button that was held at start up.
static buttonNumber_t checkStartButton(void)
{
    SET_RED_LED;

    // Wait 5 seconds for any terminal to be connected
    uPortTaskBlock(STARTUP_DELAY);

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

/// @brief Handler for the buttons. 
static void button_pressed(int buttonNo, uint32_t holdTime)
{
    // While exiting don't allow any buttons to be handled
    // Allow them to be handled once the application has finalized
    if (gExitApp && appFinalized == false) 
        return;
    
    // If the button is released, store the 'pressed' button
    if (!holdTime) {
        pressedButton = buttonNo;
    } else {
        pressedButton = NO_BUTTON; 
    
        // The application framework will enable the 'app' buttons,
        // exit handling the buttons if this is not enabled yet.
        if (!buttonCommandEnabled)
            return;

        switch (buttonNo) {
            // EXIT APPLICATION 
            case BUTTON_1:
                writeLog("Exit button pressed, closing down... Please wait for the RED LED to go out...");
                gExitApp = true;
                break;

            // BUTTON #2 action is set by the application via setButtonTwoFunction()
            case BUTTON_2:                
                if (buttonTwoFunc != NULL) {
                    writeLog("Button #2 pressed");
                    buttonTwoFunc();
                } else {
                    printDebug("No function defined for Button #2");
                }
                break;

            default:
                break;
        }
    }
}

static void displayLog(bool printHelp)
{
    startLogging(LOG_FILENAME);
    displayLogFile();
    closeLogFile(false);

    if (printHelp) {
        printf("\n\n\nYou can press Button #1 to display the log now still...");
    }
}

/// @brief Used at the end of the application to allow the displaying
/// of the log contents. This loop never exits - only use at end of app!
static void displayLogLoop(void)
{
    // make sure the button handler doesn't treat the button
    // presses as application presses.
    appFinalized = true;
    buttonCommandEnabled = false;

    // loop here, just in case Button #1 is pressed for
    // displaying the log after the device has been plugged
    // in to a computer.
    while(true) {
        uPortTaskBlock(50);

        if (pressedButton == BUTTON_1) {
            displayLog(true);
        }
    }
}

/// @brief Initiate the UBXLIX API
static int32_t initCellularDevice(void)
{
    int32_t errorCode;

    // turn off the UBXLIB printLog() logging as it is enabled by default (?!)
    #ifndef UBXLIB_LOGGING_ON
        uPortLogOff();
    #endif

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

    displayCellularModuleInfo();
    
    return configureCellularModule();
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

    SET_NO_LEDS;

    // deleting the log file is performed now before the start of the application
    if (button == BUTTON_2) {
        printLog("Deleting log file...");
        deleteFile(LOG_FILENAME);
    }

    // displaying the log file ends the application
    if (button == BUTTON_1) {
        displayLog(true);
        displayLogLoop();
    }

    setLogLevel(LOGGING_LEVEL);
    startLogging(LOG_FILENAME);

    // Display the file system free size
    displayFileSpace(LOG_FILENAME);

    return true;
}

static bool loadConfigFiles(void)
{
    // Save the mqtt credentials file (if present)
    if (mqttCredentialsSize > 0) {
        int32_t saveResult = saveConfigFile(MQTT_CREDENTIALS_FILENAME,
                                            mqttCredentials,
                                            mqttCredentialsSize);

        if (saveResult != 0 && saveResult != U_ERROR_COMMON_NOT_FOUND) {
            printFatal("Aborting application as configuration file was not written");
            return false;
        }
    } else {
        printDebug("No mqtt credentials to save to file system");
    }

    // Load the mqtt credentials config file
    printInfo("Loading MQTT Credentials...");
    loadConfigFile(MQTT_CREDENTIALS_FILENAME);

    // Note: More configuration files can be loaded.
    // Everything is appended to the config list array.

    // this will only print if logging is set to DEBUG or higher - security!
    printConfiguration();

    return true;
}

static void displayAppVersion()
{
    writeInfo("**************************************************");
    writeInfo("%s %s", APP_NAME, APP_VERSION);
    writeInfo("**************************************************\n");
}

/// @brief Dwells for appDwellTimeMS time, and exits if this time changes
static void dwellAppLoop(void)
{
    int32_t tick = 0;
    int32_t dwellTimeMS = appDwellTimeMS;

    do
    {
        uPortTaskBlock(APP_DWELL_TICK_MS);
        tick = tick + APP_DWELL_TICK_MS;
    } while((tick < dwellTimeMS) &&
            (dwellTimeMS == appDwellTimeMS));

    printDebug("*** Application Tick ***\n");
}

static void checkHeapInfo(void)
{
    uPortLogOn();
    printf("\n\n\nChecking for unfreed mallocs...\n");
    int32_t mallocs = uPortHeapDump(NULL);
    if (mallocs > 0) 
        printf("\n\n\nWARNING: Still have mallocs left!...\n");
    else
        printf("Mallocs are all freed.\n");

    uPortLogOff();
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Sets the time between each main loop execution
/// @param params The dwell time parameter for the dwell time
/// @return 0 if successful, or failure if invalid parameters
int32_t setAppDwellTime(commandParamsList_t *params)
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

/// @brief Sets the application logging level
/// @param params The log level parameter for the dwell time
/// @return 0 if successful, or failure if invalid parameters
int32_t setAppLogLevel(commandParamsList_t *params)
{
    logLevels_t logLevel = (logLevels_t) getParamValue(params, 1, (int32_t) eTRACE, (int32_t) eMAXLOGLEVELS, (int32_t) eINFO);

    if (logLevel < eTRACE) {
        writeWarn("Failed to set App Log Level %d. Min: %d, Max: %d", logLevel, eTRACE, eMAXLOGLEVELS);
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    setLogLevel(logLevel);

    return U_ERROR_COMMON_SUCCESS;
}

/// @brief Sets the function which handles the Button #2 function
/// @param func The function pointer for button #2 code
void setButtonTwoFunction(void (*func)(void))
{
    buttonTwoFunc = func;
}

/// @brief Method of pausing the running of the main loop. 
///        Useful for when there are other long running activities
///        which need to stop the main loop
/// @param state A flag to indicate if the main loop is paused or not
void pauseMainLoop(bool state)
{
    pauseMainLoopIndicator = state;
    printInfo("Main application loop %s", state ? "is paused" : "is unpaused");
}

/// @brief This is the main application loop which runs the appFunc which is 
///        defined in the application main.c module.
/// @param appFunc The function pointer of the app event code
void runApplicationLoop(bool (*appFunc)(void))
{
    // now allow the buttons to run their commands
    buttonCommandEnabled = true;
    printInfo("Buttons #1 and #2 are now enabled");

    while(!gExitApp) {
        dwellAppLoop();

        if (gExitApp) return;

        if (pauseMainLoopIndicator) {
            writeDebug("Application loop paused.");
            continue;
        }

        if (!appFunc()) {
            gExitApp = true;
            writeInfo("Application function stopped the app loop");
        }
    }
}

/// @brief Sets the application status, waits for the tasks and closes the log
/// @param appState The application status to set for the shutdown
void finalize(applicationStates_t appState)
{
    gAppStatus = appState;
    gExitApp = true;

    waitForAllTasksToStop();

    // now stop the network registration task. Blue LED
    SET_BLUE_LED;
    stopAndWait(NETWORK_REG_TASK);

    writeLog("Application Finished.");

    closeLogFile(true);

    uDeviceClose(gDeviceHandle, true);
    uDeviceDeinit();
    uPortDeinit();

    SET_NO_LEDS;

    #ifdef U_CFG_HEAP_MONITOR
    checkHeapInfo();
    #endif

    printf("\n\n\nXPLR App has finished. Press button #1 to display log...");

    displayLogLoop();
}

/// @brief Starts the application framework
/// @return true if successful, false otherwise
bool startupFramework(void)
{
    int32_t errorCode;

    // initialise our LEDs and start up button commands
    if (!initXplrDevice())
        return false;

    displayAppVersion();

    if (!loadConfigFiles())
        return false;

    errorCode = initSingleTask(LED_TASK);
    if (errorCode < 0) {
        writeFatal("* Failed to initialise LED task - not running application!");
        finalize(ERROR);
    }

    errorCode = runTask(LED_TASK);
    if (errorCode != 0) {
        writeFatal("* Failed to start LED task - not running application!");
        finalize(ERROR);
    }

    // initialise the cellular module
    gAppStatus = INIT_DEVICE;
    errorCode = initCellularDevice();
    if (errorCode != 0) {
        writeFatal("* Failed to initialise the cellular module - not running application!");
        finalize(ERROR);
    }

    // Initialise the task runners
    if (initTasks() != 0) {
        finalize(ERROR);
    }

    return true;
}