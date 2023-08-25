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
 * Logging functions
 *
 */

#include <stdarg.h>
#include <time.h>

#include "common.h"
#include "log.h"
#include "ext_fs.h"

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */

/// DO NOT PUT printLog() INSIDE this MUTEX LOCK!!!
#define MUTEX_LOCK if (pLogMutex != NULL) uPortMutexLock(pLogMutex); {
#define MUTEX_UNLOCK } if (pLogMutex != NULL) uPortMutexUnlock(pLogMutex);

#define LOGBUFF1SIZE 1024
#define LOGBUFF2SIZE 2048

#define FILE_READ_BUFFER 100

#define FLUSH_TIMER_SECONDS 60

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char buff1[LOGBUFF1SIZE];
static char buff2[LOGBUFF2SIZE];

static struct fs_file_t logFile;
static bool logFileOpen = false;

static uPortMutexHandle_t pLogMutex = NULL;
static uPortTimerHandle_t pFlushTimerHandle = NULL;

static logLevels_t gLogLevel = eINFO;

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */
/// The unix network time, which is retrieved after first registration
int64_t unixNetworkTime = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static void createTimeStampLog(const char *log)
{
    char timeStamp[TIMESTAMP_MAX_LENTH_BYTES];
    getTimeStamp(timeStamp);
    snprintf(buff1, LOGBUFF1SIZE, "%s: %s\n", timeStamp, log);
}

static bool printHeader(logLevels_t level, bool writeToFile)
{
    const char *header = NULL;
    switch(level) {
        case eWARN:
            header = "\n*** WARNING ************************************************\n";
            break;

        case eERROR:
            header = "\n************************************************************\n" \
                     "*** ERROR **************************************************\n";
            break;

        case eFATAL:
            header = "\n############################################################\n" \
                     "#### FATAL ** FATAL ** FATAL ** FATAL ** FATAL ** FATAL ####\n" \
                     "############################################################\n";
            break;

        default:
            // no header
            break;
    }

    if (header == NULL)
        return false;

    printf("%s", header);

    if (logFileOpen && writeToFile)
        fs_write(&logFile, header, strlen(header));

    return true;
}

static void flushTimerCallback(void *callbackHandle, void *param)
{
    MUTEX_LOCK
        printDebug("Flushing file system... ");
        fs_sync(&logFile);
        printDebug("Done\n");
    MUTEX_UNLOCK
}

static void startFlushTimer()
{
    int32_t errorCode = uPortTimerCreate(&pFlushTimerHandle, NULL, flushTimerCallback, NULL, FLUSH_TIMER_SECONDS * 1000, true);
    if (errorCode < 0) {
        printWarn("Failed to create the log file flushing timer: %d", errorCode);
    } else {
        errorCode = uPortTimerStart(pFlushTimerHandle);
        if (errorCode == 0)
            printInfo("Started log file flushing.... every %d seconds", FLUSH_TIMER_SECONDS);
        else
            printWarn("Failed to start the log file flushing: %d", errorCode);
    }
}

static bool openLogFile(const char *pFilename)
{
    fs_file_t_init(&logFile);
    const char *path = extFsPath(pFilename);
    int result = fs_open(&logFile, path, FS_O_APPEND | FS_O_CREATE | FS_O_RDWR);

    if (result == 0) {
        printLog("File logging enabled");
        logFileOpen = true;
    } else {
        printError("Failed to open log file: %d\n Logging to the log file will not be available.", result);
        logFileOpen = false;
    }

    return logFileOpen;
}

static int32_t createLogFileMutex(void)
{
    int32_t errorCode = uPortMutexCreate(&pLogMutex);
    if (errorCode < 0)
        printError("Failed to create the log mutex: %d\n Logging to the log file will not be available.", errorCode);

    return errorCode;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
void setLogLevel(logLevels_t logLevel)
{
    printInfo("Setting log level from %d to %d", gLogLevel, logLevel);
    gLogLevel = logLevel;
}

/// @brief Writes a log message to the terminal and the log file
/// @param log The log, which can contain string formating
/// @param  ... The variables for the string format
void _writeLog(const char *log, logLevels_t level, bool writeToFile, ...)
{
    // writeLog("The %s value is %d", "rssi", 1234)
    // will log "<time>: The rssi value is 1234"

    if (level < gLogLevel)
        return;

    MUTEX_LOCK

        // DO NOT PUT PRINTLOG OR WRITELOG MARCOS INSIDE
        // THIS MUTEX LOCK - *ONLY* USE PRINTF() !!!!!!

        createTimeStampLog(log);

        // now construct the application's arguments into the log string
        va_list arglist;

        #pragma GCC diagnostic ignored "-Wvarargs"
        va_start(arglist, buff1);
        vsnprintf(buff2, LOGBUFF2SIZE, buff1, arglist);
        va_end(arglist);

        bool header = printHeader(level, writeToFile);
        printf("%s", buff2);
        if (header)
            printf("\n");

        if (logFileOpen && writeToFile) {
            fs_write(&logFile, buff2, strlen(buff2));
            if (header)
                fs_write(&logFile, "\n", 1);
        }

        // DO NOT PUT PRINTLOG OR WRITELOG MARCOS INSIDE
        // THIS MUTEX LOCK - *ONLY* USE PRINTF() !!!!!!
    
    MUTEX_UNLOCK;
}

/// @brief Close the log file
void closeLogFile(bool displayWarning)
{
    if (!logFileOpen)
        return;

    if (displayWarning)
        printf("\nClosing log file... PLEASE WAIT!!!\n");
    
    MUTEX_LOCK
    
        fs_sync(&logFile);

        logFileOpen = false;
        fs_close(&logFile);

        if (pFlushTimerHandle != NULL)
            uPortTimerStop(pFlushTimerHandle);
    
    MUTEX_UNLOCK
    
    if (displayWarning)
        printf("Log file is now closed.\n");
}

void displayLogFile(void)
{
    char buffer[FILE_READ_BUFFER];
    int count;

    if (!logFileOpen) {
        printf("Opening log file failed, cannot display log.");
        return;
    }

    printf("\n********************************************************\n"
               "*** LOG START ******************************************\n"
               "********************************************************\n");

    while((count = fs_read(&logFile, buffer, FILE_READ_BUFFER)) > 0)
        printf("%.*s", count, buffer);

    printf("\n********************************************************\n"
               "*** LOG END ********************************************\n"
               "********************************************************\n");
}

void displayFileSpace(const char *pFilename)
{
    uint32_t freeSpace = extFsFree() * 1024;
    printLog("File system free space: %u bytes", freeSpace);

    size_t fileSize;
    const char *path = extFsPath(pFilename);
    if (extFsFileSize(path, &fileSize)) {
        printLog("Log file size: %u bytes", fileSize);
    }
}

void deleteFile(const char *pFilename)
{
    const char *path = extFsPath(pFilename);
    if (fs_unlink(path) == 0)
        printLog("Deleted file: %s", pFilename);
    else
        printLog("Failed to delete file: %s", pFilename);
}

void startLogging(const char *pFilename) {
    if (createLogFileMutex() < 0)
        return;

    if (openLogFile(pFilename))
        startFlushTimer();
}