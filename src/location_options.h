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

/**
 * @brief GNSS constellation types
 *
 */
enum LocationConstellation {
    LOCATION_CONST_GPS_ONLY         = 0,
    LOCATION_CONST_GPS_GLONASS      = (1 << 0),
    LOCATION_CONST_GPS_BEIDOU       = (1 << 1),
    LOCATION_CONST_GPS_GALILEO      = (1 << 2),
    LOCATION_CONST_GPS_QZSS         = (1 << 3),
};

constexpr LocationConstellation LocationConstellationDefault {LOCATION_CONST_GPS_GLONASS};
constexpr int LocationHdopDefault {100};
constexpr float LocationHaccDefault {50.0}; // Meters
constexpr unsigned int LocationFixTimeDefault {90}; // Seconds

/**
 * @brief LocationConfiguration class to configure Location class options
 *
 */
class LocationConfiguration {
public:
    /**
     * @brief Construct a new Location Configuration object
     *
     */
    LocationConfiguration() :
        _constellations(LOCATION_CONST_GPS_GLONASS),
        _antennaPin(PIN_INVALID),
        _hdop(LocationHdopDefault),
        _hacc(LocationHaccDefault),
        _maxFixSeconds(LocationFixTimeDefault) {
    }

    /**
     * @brief Construct a new Location Configuration object
     *
     */
    LocationConfiguration(LocationConfiguration&&) = default;

    /**
     * @brief Set GNSS constellations.
     *
     * @param constellations Bitmap of supported GNSS constellations
     * @return LocationConfiguration&
     */
    LocationConfiguration& constellations(LocationConstellation constellations) {
        _constellations = constellations;
        return *this;
    }

    /**
     * @brief Get GNSS constellations.
     *
     * @return LocationConstellation Bitmap of supported GNSS constellations
     */
    LocationConstellation constellations() const {
        return _constellations;
    }

    /**
     * @brief Set the pin assignment for GNSS antenna power
     *
     * @param pin Pin number to enable GNSS antenna power
     * @return LocationConfiguration&
     */
    LocationConfiguration& enableAntennaPower(pin_t pin) {
        _antennaPin = pin;
        return *this;
    }

    /**
     * @brief Get the pin assignment for GNSS antenna power
     *
     * @return pin_t Powered at initialization
     */
    pin_t enableAntennaPower() const {
        return _antennaPin;
    }

    /**
     * @brief Set the HDOP threshold for a stable position fix
     *
     * @param hdop Value to check for position stability, 0 to 100
     * @return LocationConfiguration&
     */
    LocationConfiguration& hdopThreshold(int hdop) {
        if (0 > hdop)
            hdop = 0;
        else if (100 < hdop)
            hdop = 100;
        _hdop = hdop;
        return *this;
    }

    /**
     * @brief Get the HDOP threshold for a stable position fix
     *
     * @return int Value to check for position stability
     */
    int hdopThreshold() const {
        return _hdop;
    }

    /**
     * @brief Set the horizontal accuracy threshold, in meters, for a stable position fix (if supported)
     *
     * @param hacc Value to check for position stability, 1.0 to 1000.0 meters
     * @return LocationConfiguration&
     */
    LocationConfiguration& haccThreshold(float hacc) {
        _hacc = hacc;
        return *this;
    }

    /**
     * @brief Get the horizontal accuracy threshold, in meters, for a stable position fix (if supported)
     *
     * @return float Value to check for position stability
     */
    float haccThreshold() const {
        return _hacc;
    }

    /**
     * @brief Set the maximum amount of time to wait for a position fix
     *
     * @param fixSeconds Number of seconds to allow for a position fix
     * @return TrackerConfiguration&
     */
    LocationConfiguration& maximumFixTime(unsigned int fixSeconds) {
        _maxFixSeconds = fixSeconds;
        return *this;
    }

    /**
     * @brief Get the maximum amount of time to wait for a position fix
     *
     * @return Number of seconds to allow for a position fix
     */
    unsigned int maximumFixTime() const {
        return _maxFixSeconds;
    }

    LocationConfiguration& operator=(const LocationConfiguration& rhs) {
        if (this == &rhs) {
            return *this;
        }
        this->_constellations = rhs._constellations;
        this->_antennaPin = rhs._antennaPin;
        this->_hdop = rhs._hdop;
        this->_maxFixSeconds = rhs._maxFixSeconds;

        return *this;
    }

private:
    LocationConstellation _constellations;
    pin_t _antennaPin;
    int _hdop;
    float _hacc;
    unsigned int _maxFixSeconds;
};
