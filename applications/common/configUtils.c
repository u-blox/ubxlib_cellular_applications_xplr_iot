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
 * Utility functions to help load 'config' files in
 *
 */

#include "common.h"
#include "config.h"
#include "ext_fs.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define FILE_READ_BUFFER 50

// delimiters are ' ' (space) and '\n' (newline)
#define CONFIG_DELIMITERS " \n"

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef struct APP_CONFIG_LIST {
    char *key;
    char *value;
    struct APP_CONFIG_LIST *pNext;
} appConfigList_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char *configText;
static struct fs_file_t configFile;
static appConfigList_t *configList;

/* ----------------------------------------------------------------
 * STATIC PRIVATE FUNCTIONS
 * -------------------------------------------------------------- */
static appConfigList_t *createConfigKVP(char *key, char *value) {
    appConfigList_t *newKVP = (appConfigList_t *)pUPortMalloc(sizeof(appConfigList_t));
    newKVP->key = key;
    newKVP->value = value;
    newKVP->pNext = NULL;

    return newKVP;
}

static size_t parseConfiguration(appConfigList_t **head)
{
    size_t count = 0;
    appConfigList_t *current;

    do {
        char *key = strtok_r(configText, CONFIG_DELIMITERS, &configText);
        char *value = strtok_r(NULL, CONFIG_DELIMITERS, &configText);
        if(key == NULL || value == NULL) {
            break;
        }

        if(count == 0) {
            *head = createConfigKVP(key, value);
            current = *head;
        } else {
            current->pNext = createConfigKVP(key, value);
            current = current->pNext;
        }

        count++;
    } while(true);

    return count;
}

static void printConfiguration(void)
{
    size_t count=1;
    printLog("\n--- MQTT Credentials ------------------------------------------");
    appConfigList_t *kvp = configList;
    char *value;

    while(kvp != NULL) {
        value = getConfig(kvp->key);
        if (value==NULL) value = "N/A";
        printLog("   Key #%d: %s = %s",
                count, kvp->key, value);

        kvp = kvp->pNext;
        count++;
    }

    printLog("");
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Saves the configuration file defined in the header file
/// @param filename The filename of the configuration file
/// @return 0 on success, negative on failure
int32_t saveConfigFile(const char *filename)
{
    int32_t success;
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    if (MQTT_CREDENTIALS == NULL) {
        writeDebug("No configuration file to save to file system");
        return U_ERROR_COMMON_NOT_FOUND;
    }

    const char *path = extFsPath(filename);

    if (extFsFileExists(path)) {
        printDebug("Found old config file, deleting...");
        if (fs_unlink(path) != 0) {
            writeError("Failed to delete old configuration file: %s", filename);
            return U_ERROR_COMMON_DEVICE_ERROR;
        }
    }

    fs_file_t_init(&configFile);
    success = fs_open(&configFile, path, FS_O_CREATE | FS_O_WRITE);
    if (success < 0) {
        writeError("Failed to open configuration file: %d", success);
        return U_ERROR_COMMON_DEVICE_ERROR;
    }

    size_t configFileSize = sizeof(MQTT_CREDENTIALS);
    int32_t count = fs_write(&configFile, MQTT_CREDENTIALS, configFileSize);
    if (count != configFileSize) {
        if (count < 0)
            writeError("Failed to write configuration file. Error: %d", count);
        else
            writeError("Failed to write configuration file. Config size: %d, written size: %d", configFileSize, count);

        errorCode = U_ERROR_COMMON_DEVICE_ERROR;
    }

    fs_close(&configFile);

    return errorCode;
}

/// @brief Loads a configuration file ready for indexing
/// @param filename The filename of the configuration file
/// @return 0 on success, negative on failure
int32_t loadConfigFile(const char *filename)
{
    int32_t success;
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    const char *path = extFsPath(filename);
    if (!extFsFileExists(path)) {
        writeError("Cconfiguration file not found on file system");
        errorCode = U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    size_t fileSize;
    success = extFsFileSize(path, &fileSize);
    if (!success) {
        writeError("Failed to get filesize of configuration file '%s': %d", filename, success);
        errorCode =  U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    configText = (char *)pUPortMalloc(fileSize);
    if (configText == NULL) {
        writeError("Failed to allocate memory for loading in configuration file, size: %d", fileSize);
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanUp;
    }

    fs_file_t_init(&configFile);
    success = fs_open(&configFile, path, FS_O_READ);
    if (success < 0) {
        writeError("Failed to open configuration file: %d", success);
        errorCode = U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    size_t count;
    char *buffer = configText;
    while((count = fs_read(&configFile, buffer, FILE_READ_BUFFER)) > 0) {
        buffer = buffer + count;
    }

    parseConfiguration(&configList);
    printConfiguration();

cleanUp:
    if (errorCode != 0) {
        uPortFree(configText);
    }

    fs_close(&configFile);

    return errorCode;
}

/// @brief Returns the specified configuration value
/// @param key The configuration name to return the value of
/// @return The configuration value on succes, NULL on failure
char *getConfig(const char *key)
{
    for(appConfigList_t *kvp = configList; kvp != NULL; kvp=kvp->pNext) {
        if (strcmp(kvp->key, key) == 0) {
            if (strcmp(kvp->value, "NULL") == 0)
                return NULL;
            else
                return kvp->value;
        }
    }

    printWarn("Failed to find '%s' key", key);
    return NULL;
}
