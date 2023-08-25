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
#include "config.h"
#include "cellInit.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define INFO_BUFFER_SIZE 50

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t checkReboot(void)
{
    if(uCellPwrRebootIsRequired(gDeviceHandle))
    {
        writeLog("Need to reboot the module as the settings have changed...");
        return uCellPwrReboot(gDeviceHandle, NULL);
    }

    return 0;
}

static int32_t configureMNOProfile(void)
{
    int32_t errorCode = uCellCfgGetMnoProfile(gDeviceHandle);
    if (errorCode == MNO_PROFILE) return 0;

    errorCode = uCellCfgSetMnoProfile(gDeviceHandle, MNO_PROFILE);
    if(errorCode != 0)
    {
        writeError("Failed to set MNO Profile %d", MNO_PROFILE);
        return errorCode;
    }

    return checkReboot();
}

static int32_t configureRAT(void)
{
    int32_t rat = uCellCfgGetRat(gDeviceHandle, 0);
    if (rat == URAT) return 0;

    int32_t errorCode = uCellCfgSetRat(gDeviceHandle, URAT);
    if (errorCode != 0)
    {
        writeError("Failed to set RAT %d", URAT);
        return errorCode;
    }

    return checkReboot();
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
int32_t configureCellularModule(void)
{
    writeInfo("Configuring the cellular module...");

    int32_t errorCode = configureMNOProfile();
    if (errorCode == 0)
        errorCode = configureRAT();

    return errorCode;
}

/// @brief Display the cellular module's information 
void displayCellularModuleInfo(void)
{
    char buffer[INFO_BUFFER_SIZE];
    
    // getIMxI functions return fixed chars, not a null terminated string (!)
    char imsi[U_CELL_INFO_IMSI_SIZE+1];
    char imei[U_CELL_INFO_IMEI_SIZE+1];
    imei[sizeof(imei)-1] = 0;
    imsi[sizeof(imsi)-1] = 0;

    int32_t count;
    int32_t errorCode;

    count = uCellInfoGetManufacturerStr(gDeviceHandle, buffer, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Manufacturer: %s", buffer);
    else
        writeWarn("Cellular Module Manufacturer: Failed to get");

    count = uCellInfoGetModelStr(gDeviceHandle, buffer, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Model: %s", buffer);
    else
        writeWarn("Cellular Module Model: Failed to get");

    count = uCellInfoGetFirmwareVersionStr(gDeviceHandle, buffer, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Firmware: %s", buffer);
    else
        writeWarn("Cellular Module Firmware: Failed to get");
    
    errorCode = uCellInfoGetImei(gDeviceHandle, imei);
    if (errorCode == 0)
        writeInfo("Cellular Module IMEI: %s", imei);
    else
        writeWarn("Cellular Module IMEI: Failed to get: %d", errorCode);
    
    errorCode = uCellInfoGetImsi(gDeviceHandle, imsi);
    if (errorCode == 0)
        writeInfo("Cellular Module IMSI: %s", imsi);
    else
        writeWarn("Cellular Module IMSI: Failed to get: %d", errorCode);
}