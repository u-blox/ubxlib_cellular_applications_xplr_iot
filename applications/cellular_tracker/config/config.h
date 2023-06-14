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

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* ----------------------------------------------------------------
 * DEBUG LEVEL SETTING - This can be changed remotely using
 *                       "SET_LOG_LEVEL" command via the
 *                        APP_CONTROL MQTT topic
 * -------------------------------------------------------------- */
#define LOGGING_LEVEL eINFO            // taken from logLevels_t

/* ----------------------------------------------------------------
 * APN SELECTION
 *
 * THINGSTREAM SIMS:-
 *  -   Must use 'TSUDP' for Thingstream MQTT-Anywhere.
 *  -   USE 'TSIOT' for 'normal' internet use.
 *
 * RESTRICTED APNS:-
 *  -   In the tasks/registrationTask.c file there is a list of APNs
 *      which are marked as 'restricted'. This means normal internet
 *      queries are not available, like the NTP service on TSUDP APN.
 *      Edit this list for other APNs which are restricted/limited.
 * -------------------------------------------------------------- */
#define APN "TSIOT"

/* ----------------------------------------------------------------
 * MQTT CREDENTIALS SELECTION
 *
 * Please select, using ONE #define below, which mqtt configuration
 * to use for this application.
 *
 * Each are described in the src/mqtt_credentials.c file
 * ----------------------------------------------------------------*/
// *** Thingstream MQTT Services
//#define MQTT_THINGSTREAM_ANYWHERE
//#define MQTT_THINGSTREAM_FLEX
//#define MQTT_THINGSTREAM_NOW_NoTLS_AUTH
//#define MQTT_THINGSTREAM_NOW_TLS_AUTH

// *** Mosquitto MQTT Test Service
#define MQTT_MOSQUITTO_NoTLS_NoAuth
//#define MQTT_MOSQUITTO_NoTLS_Auth
//#define MQTT_MOSQUITTO_TLS_Auth

/*  ----------------------------------------------------------------
 * RADIO ACCESS TECHNOLOGY SELECTION
 *
 * Please use the same RAT enum from the UBXLIB uCellNetRat_t list.
 *  ----------------------------------------------------------------*/
#define URAT U_CELL_NET_RAT_CATM1

/* ----------------------------------------------------------------
 * MNO PROFILE SELECTION
 *
 * Please use the MNO Profile number for the cellular module being
 * used. See the AT command manual appendix for list
 *
 * Standard Profiles:
 *    100 = European
 *    90  = Global
 * ----------------------------------------------------------------*/
#define MNO_PROFILE 100

#endif