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
#include "cellScanTask.h"

// Application name and version number is in the config.h file

/* ----------------------------------------------------------------
 * Remote control callbacks for the main application
 * Add your application topic message callbacks here
 * -------------------------------------------------------------- */
#define APP_CONTROL_TOPIC "AppControl"
static callbackCommand_t callbacks[] = {
    {"SET_DWELL_TIME", setAppDwellTime},
    {"SET_LOG_LEVEL", setAppLogLevel}
};

/// @brief The application function(s) which are run every appDwellTime
/// @return A flag to indicate the application should continue (true)
bool appFunction(void)
{
    queueMeasureNow(NULL);
    queueLocationNow(NULL); 

    return true;
}

void buttonTwo(void)
{
    queueNetworkScan(NULL);
}

/* ----------------------------------------------------------------
 * Main startup function for the framework
 * -------------------------------------------------------------- */
void main(void)
{
    if (!startupFramework())
        return;

    // The Network registration task is used to connect to the cellular network
    // This will monitor the +CxREG URCs
    runTask(NETWORK_REG_TASK);

    // The MQTT task connects and reconnects to the MQTT broker selected in the 
    // config.h file. This needs to run for MQTT messages to be published and
    // for remote control messages to be handled
    runTask(MQTT_TASK);
    
    // Subscribe to the main AppControl topic for remote control the main application (this)
    subscribeToTopicAsync(APP_CONTROL_TOPIC, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    // Set button two to point to the queueCellScan function
    setButtonTwoFunction(buttonTwo);

    // Start the application loop with our app function
    runApplicationLoop(appFunction);

    // all done, close down and finalize    
    finalize(SHUTDOWN);
}
