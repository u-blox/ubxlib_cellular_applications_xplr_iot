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
 * MQTT Credentials File
 *
 *
 */

#ifndef _MQTT_CREDENTIALS_H_
#define _MQTT_CREDENTIALS_H_

// Simple way to create a text file which is saved to the file system
// to configure the MQTT credentials
// Use a single space to separate the paramater name to the value.
//
// MQTT-TYPE is either "MQTT" or "MQTT-SN"
// You can use NULL for unused parameters, like no username or password
//
// If you set MQTT-CLIENTID to NULL in this configuration UBXLIB will automatically set
// the IMEI of the cellular device being used. So unless you MUST set the clientID for
// your own MQTT broker, you can leave this parameter NULL.
//
// For MQTT brokers that don't use security, set MQTT-SECURITY to NULL
// For MQTT brokers that require security, set the MQTT-SECURITY to the module's security profile number.
// NOTE: You will need to have already uploaded certificates and configured this security profile on the
// cellular device outside of this application.



// This is the Thingstream MQTT-Anywhere configuration. There is no need for username & password or clientID
// Thingstream MQTT-Anywhere uses the SIM security from the network operator
#define MQTT_CREDENTIALS    "MQTT-TYPE MQTT-SN\n" \
                            "MQTT_BROKER_NAME 10.7.0.55:2442\n" \
                            "MQTT_USERNAME NULL\n" \
                            "MQTT_PASSWORD NULL\n" \
                            "MQTT-CLIENTID NULL\n" \
                            "MQTT-SECURITY NULL\n"




// This is the mosquitto test server configuration. Port 1883 has no security.
/*
#define MQTT_CREDENTIALS    "MQTT-TYPE MQTT\n" \
                            "MQTT_BROKER_NAME test.mosquitto.org:1883\n" \
                            "MQTT_USERNAME NULL\n" \
                            "MQTT_PASSWORD NULL\n" \
                            "MQTT-CLIENTID NULL\n" \
                            "MQTT-SECURITY NULL\n"
*/

#endif