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
 * Configuration file utility functions
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

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