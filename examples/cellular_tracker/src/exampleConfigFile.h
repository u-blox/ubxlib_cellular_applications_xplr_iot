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
 * Configuration File - for saving to the ConfigFile.txt file
 *
 * THIS MUST NOT BE CHECKED IN TO SOURCE CONTROL!!!
 *
 */

#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

// Simple way to create a text file which is saved to the file system
#define CONFIG_FILE_CONTENTS "MQTT_BROKER_NAME:xxx.xxx.xxx.xxx\n" \
                             "MQTT_BROKER_PORT:xxxx" \
                             "MQTT_USERNAME:username\n" \
                             "MQTT_PASSWORD:password\n" \
                             "MQTT-TYPE:MQTT\n"

#endif