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

#include "appInit.h"
#include "taskControl.h"

#include "mqttTask.h"
#include "signalQualityTask.h"
#include "locationTask.h"

/* ----------------------------------------------------------------
 * Remote control callbacks for the main application
 * -------------------------------------------------------------- */
#define APP_CONTROL_TOPIC "AppControl"
static callbackCommand_t callbacks[] = {
    {"SET_DWELL_TIME", setAppDwellTime},
    {"SET_LOG_LEVEL", setAppLogLevel}
};

/// @brief Main entry to the application.
/// If Button 1 is held, the log file is displayed.
/// If Button 2 is held, the log file is deleted.
void main(void)
{
    if (!startupFramework())
        return;

    // Run the two main tasks for this application, Network and MQTT tasks.
    runTask(NETWORK_REG_TASK);
    runTask(MQTT_TASK);

    subscribeToTopicAsync(APP_CONTROL_TOPIC, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    // Main application loop
    while(!gExitApp) {
        dwellAppLoop();

        // exercise task operations
        if (!gExitApp && gAppStatus != COPS_QUERY) {
            queueMeasureNow(NULL);
            queueLocationNow(NULL);
        }
    }

    writeLog("Application closing down...");
    finalise(SHUTDOWN);
}
