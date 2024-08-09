/*
 * Copyright (c) 2024 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <ctime>

/**
 * @brief Type of location fix.
 *
 */
enum LocationFix {
    LOCATION_FIX_NONE = 0,
    LOCATION_FIX_2D,
    LOCATION_FIX_3D,
};

/**
 * @brief Type of point coordinates of the given event.
 *
 */
struct LocationPoint {
    unsigned int fix;               /**< Indication of GNSS locked status */
    time_t epochTime;               /**< Epoch time from device sources */
    time32_t systemTime;            /**< System epoch time */
    double latitude;                /**< Point latitude in degrees */
    double longitude;               /**< Point longitude in degrees */
    float altitude;                 /**< Point altitude in meters */
    float speed;                    /**< Point speed in meters per second */
    float heading;                  /**< Point heading in degrees */
    float horizontalAccuracy;       /**< Point horizontal accuracy in meters */
    float horizontalDop;            /**< Point horizontal dilution of precision */
    float verticalAccuracy;         /**< Point vertical accuracy in meters */
    float verticalDop;              /**< Point vertical dilution of precision */
    float timeToFirstFix;           /**< Time-to-first-fix in seconds */
    unsigned int satsInUse;         /**< Point satellites in use */
};
