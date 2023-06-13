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
 * If you set MQTT-CLIENTID to NULL in this configuration UBXLIB will automatically set
 * the IMEI of the cellular device being used. So unless you MUST set the clientID for
 * your own MQTT broker, you can leave this parameter NULL.
 *
 * For MQTT brokers that don't use security, set MQTT-SECURITY to NULL
 * For MQTT brokers that require security, set the MQTT-SECURITY to the module's security profile number.
 * NOTE: You will need to have already uploaded certificates and configured this security profile on the
 * cellular device outside of this application.
*/

#include "common.h"
#include "../config/config.h"

// This is the configuration for loading the MQTT configuration that is already on saved on the file system
#ifdef MQTT_FILE_SYSTEM
const char *mqtt_credentials[] = NULL       // NULL will cause the application to only load from file system
#endif

// This is the Thingstream MQTT-Anywhere configuration. There is no need for username & password or clientID
// Thingstream MQTT-Anywhere uses the SIM security from the network operator
#ifdef MQTT_THINGSTREAM_ANYWHERE
const char *mqttCredentials[] = {
    "MQTT-TYPE MQTT-SN",
    "MQTT_BROKER_NAME 10.7.0.55:2442",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT-CLIENTID NULL",
    "MQTT-SECURITY NULL",
};
#endif

// This is the mosquitto test server configuration. Port 1883 has no security.
#ifdef MQTT_MOSQUITTO
const char *mqttCredentials[] = {
    "MQTT-TYPE MQTT",
    "MQTT_BROKER_NAME test.mosquitto.org:1883",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT-CLIENTID NULL",
    "MQTT-SECURITY NULL",
};
#endif

// This is the MQTT-NOW service configuration.
#ifdef MQTT_THINGSTREAM_NOW
const char *mqttCredentials[] = {
    "MQTT-TYPE MQTT",
    "MQTT_BROKER_NAME NULL",
    "MQTT_USERNAME NULL",
    "MQTT_PASSWORD NULL",
    "MQTT-CLIENTID NULL",
    "MQTT-SECURITY NULL",
};
#endif

const int32_t mqttCredentialsSize = NUM_ELEMENTS(mqttCredentials);