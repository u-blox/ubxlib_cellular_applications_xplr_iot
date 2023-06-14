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
 * MQTT Credentials examples
 * Select which one to use in the config.h file.
 *
 * Use a single space to separate the paramater name to the value.
 *
 * MQTT-TYPE is either "MQTT" or "MQTT-SN"
 * You can use NULL for unused parameters, like no username or password
 *
 * If you set MQTT_CLIENTID to NULL in this configuration UBXLIB will automatically set
 * the IMEI of the cellular device being used. So unless you MUST set the clientID for
 * your own MQTT broker, you can leave this parameter NULL.
 *
 * For MQTT brokers that don't use security, set MQTT-SECURITY to NULL
 * For MQTT brokers that require security, set the MQTT-SECURITY to the module's security profile number.
 * NOTE: You will need to have already uploaded certificates and configured this security profile on the
 * cellular device outside of this application.
 *
 * Missing entries will resolve to NULL if they are used in the application.
 *
 * Experiment with KeepAlive (TRUE/FALSE) and Timeout Values (seconds)
*/

#include "common.h"
#include "../config/config.h"

// This is the configuration for loading the MQTT configuration that is already on saved on the file system
#ifdef MQTT_FILE_SYSTEM
const char *mqtt_credentials[] = NULL       // NULL will cause the application to only load from file system
#endif

// This is the Thingstream MQTT-Anywhere configuration.
// There is no need for username & password or clientID
// Thingstream MQTT-Anywhere uses the SIM security from the network operator
#ifdef MQTT_THINGSTREAM_ANYWHERE
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT-SN",
    "MQTT_BROKER_NAME 10.7.0.55:2442",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE NULL",       // no security profile
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

// This is the Thingstream MQTT-FLEX service configuration.
// Port 2443 uses TLS security with client certificate and key
// Need to specify the client certificate and key files in security profile #1
#ifdef MQTT_THINGSTREAM_FLEX
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME mqtt.thingstream.io:2443",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE 1",       // security profile #1
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

// This is the Thingstream MQTT-NOW service configuration.
// Port 1883 doesn't use TLS security, just authentication
// Must provide username and password below...
#ifdef MQTT_THINGSTREAM_NOW_NoTLS_AUTH
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME mqtt.thingstream.io:1883",
    "MQTT_USERNAME YOUR-USER-NAME-HERE",
    "MQTT_PASSWORD YOUR-PASSWORD-HERE",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE NULL",       // no security profile
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

// This is the MQTT-NOW service configuration.
// Port 8883 uses TLS security and authentication
// Must provide username and password below...
#ifdef MQTT_THINGSTREAM_NOW_TLS_AUTH
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME mqtt.thingstream.io:8883",
    "MQTT_USERNAME YOUR-USER-NAME-HERE",
    "MQTT_PASSWORD YOUR-PASSWORD-HERE",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE 0",       // Security profile #0
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

// This is the mosquitto test server configuration.
// Port 1883 has no security or authentication
#ifdef MQTT_MOSQUITTO_NoTLS_NoAuth
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME test.mosquitto.org:1883",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE NULL",       // no security profile
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

// This is the mosquitto test server configuration.
// Port 1884 uses username/password authentication only
// "rw" and "readwrite" for the username/password
#ifdef MQTT_MOSQUITTO_NoTLS_AUTH
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME test.mosquitto.org:1884",
    "MQTT_USERNAME \"rw\"",
    "MQTT_PASSWORD \"readwrite\"",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE NULL",       // no security profile
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif
// This is the mosquitto test server configuration.
// Port 8885 uses TLS security and username/password authentication
// Security ProfileID 0 is assumed to be configured already on the SARA-R5 module
#ifdef MQTT_MOSQUITTO_TLS_AUTH
const char *mqttCredentials[] = {
    "MQTT_TYPE MQTT",
    "MQTT_BROKER_NAME test.mosquitto.org:8885",
    "MQTT_USERNAME \"rw\"",
    "MQTT_PASSWORD \"readwrite\"",
    "MQTT_CLIENTID NULL",
    "MQTT_SECURITY_PROFILE 0",       // Security profile #0
    "MQTT_KEEPALIVE NULL",
    "MQTT_TIMEOUT NULL",
};
#endif

const int32_t mqttCredentialsSize = NUM_ELEMENTS(mqttCredentials);