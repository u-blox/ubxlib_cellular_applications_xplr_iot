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
 * Logging header
 *
 */

#ifndef _LOGGING_H_
#define _LOGGING_H_

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */
#define printLog(log, ...) _writeLog(log, eINFO, false, ##__VA_ARGS__)
#define writeLog(log, ...) _writeLog(log, eINFO, true, ##__VA_ARGS__)

#define printLog2(log, level, ...) _writeLog(log, level, false, ##__VA_ARGS__)
#define writeLog2(log, level, ...) _writeLog(log, level, true, ##__VA_ARGS__)

#define printTrace(log, ...) _writeLog(log, eTRACE, false, ##__VA_ARGS__)
#define printDebug(log, ...) _writeLog(log, eDEBUG, false, ##__VA_ARGS__)
#define printInfo(log, ...) _writeLog(log, eINFO, false,  ##__VA_ARGS__)
#define printWarn(log, ...) _writeLog(log, eWARN, false, ##__VA_ARGS__)
#define printError(log, ...) _writeLog(log, eERROR, false, ##__VA_ARGS__)
#define printFatal(log, ...) _writeLog(log, eFATAL, false, ##__VA_ARGS__)
#define printAlways(log, ...) _writeLog(log, eNOFILTER, false, ##__VA_ARGS__)

#define writeTrace(log, ...) _writeLog(log, eTRACE, true, ##__VA_ARGS__)
#define writeDebug(log, ...) _writeLog(log, eDEBUG, true, ##__VA_ARGS__)
#define writeInfo(log, ...) _writeLog(log, eINFO, true,  ##__VA_ARGS__)
#define writeWarn(log, ...) _writeLog(log, eWARN, true, ##__VA_ARGS__)
#define writeError(log, ...) _writeLog(log, eERROR, true, ##__VA_ARGS__)
#define writeFatal(log, ...) _writeLog(log, eFATAL, true, ##__VA_ARGS__)
#define writeAlways(log, ...) _writeLog(log, eNOFILTER, true, ##__VA_ARGS__)

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */

/// @brief Logging level for the printLog() function
typedef enum {
    eTRACE,
    eDEBUG,
    eINFO,
    eWARN,
    eERROR,
    eFATAL,
    eMAXLOGLEVELS,
    eNOFILTER
} logLevels_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief Start logging to the specified file
/// @param pFilename The filename to log to
void startLogging(const char *pFilename);

/// @brief set the logging level of printLog and writeLog
void setLogLevel(logLevels_t level);

/// @brief Write a log entry to the log file and terminal
/// @param log The log format to write
/// @param level The level of terminal logging
/// @param writeToFile Set to false to not write to the file
/// @param ... The arguments to use in the log entry
void _writeLog(const char *log, logLevels_t level, bool writeToFile, ...);

/// @brief Display the entire log file to the terminal
void displayLogFile(void);

/// @brief Delete the specified file
/// @param pFilename The file to delete
void deleteFile(const char *pFilename);

/// @brief Close the log file
/// @param displayWarning Displays a warning message about waiting while closing the file
void closeLogFile(bool displayWarning);

/// @brief Display the free space and log file size
/// @param pFilename The log filename to check
void displayFileSpace(const char *pFilename);

#endif