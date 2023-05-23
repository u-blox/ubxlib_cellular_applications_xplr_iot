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
 * Configuration for the Cellular Tracking Application
 * Contains:
 *      MQTT credentials (in another #include)
 *
 *      Cellular APN
 *      Cellular MNO Profile, URAT
 *
 */

/* This is the configuration file for the MQTT credentials
 If this include file is included its data will be saved as
 a 'configuration file' to be used later.

 Compile with the mqtt_credentials.h included, and it will be
 automatically saved to the file system to be used again.
 You can then delete/remove/comment out this #include so
 that it is not saved to the repository :)
 */

#include "mqtt_credentials.h"

/* ----------------------------------------------------------------
 * DEBUG LEVEL SETTING - This can be changed remotely using
 *                       "SET_LOG_LEVEL" command via the MQTT topic
 * -------------------------------------------------------------- */
#define LOGGING_LEVEL eINFO            // logLevels_t

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */
#define APN "TSIOT"

// Use the RAT enum from the UBXLIB uCellNetRat_t list.
#define URAT U_CELL_NET_RAT_CATM1

// Standard Europe MNO profile
#define MNO_PROFILE 100

// This is set to NULL if the mqtt_credentials.h is not #included above
#ifndef _MQTT_CREDENTIALS_H_
#define MQTT_CREDENTIALS NULL
#endif