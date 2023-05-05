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

/**
 * Initiate environment sensor and accelerometer
 */
void sensorsInit();

/// @brief Gets the X, Y, Z acceleration.
/// @return 0 on success, -1 on failure
int32_t getAccelerometer(float *x, float *y, float *z);

// void getPosition(float ax, float ay, float az, float *px, float *py, float *pz);

/// @brief Gets the temperature, pressure and humidity
/// @return 0 on success, -1 on failure
int32_t getTempSensor(float *temp, float *pressure, float *humidity);

/**
 * Get a string with environment sensor values
 */
const char *pollTempSensor();

/**
 * Get a string with accelerometer values
 */
const char *pollAccelerometer();

/**
 * Get a string with light sensor value
 */
const char *pollLightSensor();

/**
 * Get light sensor value in lux
 */
int32_t getLightSensor();