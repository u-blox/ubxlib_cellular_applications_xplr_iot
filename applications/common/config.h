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
 * Configuration header.
 *  Contains:
 *
 *      MNO APN to use for cellular network registration
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

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
 * DEFINITIONS
 * -------------------------------------------------------------- */
#define APN "TSIOT"

// This is set to NULL if the configFile.h is not present, and
// therefore no 'configuration file' is saved to the file system
#ifndef _CONFIG_FILE_H_
#define CONFIG_FILE_CONTENTS NULL
#endif

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Loads a configuration file ready for indexing
/// @param filename The filename of the configuration file
/// @return 0 on success, negative on failure
int32_t loadConfigFile(const char *filename);

/// @brief Saves the CONFIG_FILE_CONTENTS defined above
/// @param filename The filename of the configuration file
/// @return 0 on success, negative on failure
int32_t saveConfigFile(const char *filename);

/// @brief returns the specified configuration value
/// @param key The configuration name to return the value of
/// @return The configuration value on succes, NULL on failure
char *getConfig(const char *key);

#endif