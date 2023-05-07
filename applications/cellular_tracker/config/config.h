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
 *      Cellular URAT
 *
 */

/* This is the configuration file for the MQTT credentials
 If this include file is included this data will be saved as
 a 'configuration file' to be used later.

 Compile with the configFile.h included, run and it will be
 automatically saved to the file system to be used again.
 You can now delete/remove/comment out the configFile.h so
 that it is not saved to the repository.
 */

//#include "exampleConfigFile.h"

/* ----------------------------------------------------------------
 * DEBUG LEVEL SETTING - This can be changed remotely using
 *                       "SET_LOG_LEVEL" command via the MQTT topic
 * -------------------------------------------------------------- */
#define LOGGING_LEVEL eINFO            // logLevels_t

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */
#define APN "TSIOT"

// This is set to NULL if the configFile.h is not present, and
// therefore no 'configuration file' is saved to the file system
#ifndef _CONFIG_FILE_H_
#define CONFIG_FILE_CONTENTS NULL
#endif