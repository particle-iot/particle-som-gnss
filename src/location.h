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

#include "location_options.h"
#include "location_point.h"

enum class LocationCommand {
    None,                   /**< Do nothing */
    Acquire,                /**< Perform GNSS acquisition */
    Exit,                   /**< Exit from thread */
};

/**
 * @brief SomLocation result response enumerations
 *
 */
enum class LocationResults {
    Unavailable,            /**< GNSS is not available, typically this indicates the modem is off */
    Unsupported,            /**< GNSS is not supported on this hardware */
    Idle,                   /**< No GNSS acquistions are pending or in progress */
    Acquiring,              /**< GNSS is aquiring a fix */
    Pending,                /**< A previous GNSS acquisition is in progress */
    Fixed,                  /**< GNSS position has been aquired and fixed */
    TimedOut,               /**< GNSS has not fix */
};

/**
 * @brief SomLocation class response callback prototype
 *
 */
using LocationDone = std::function<void(LocationResults)>;

struct LocationCommandContext {
    LocationCommand command {LocationCommand::None};
    bool sendResponse {false};
    LocationDone doneCallback {};
    bool publish {false};
    LocationPoint* point {nullptr};
};

enum class CME_Error {
    NONE                  = 0,
    FIX                   = 1,    /**< Fixed position */
    SESSION_IS_ONGOING    = 504,  /**< Session is ongoing */
    SESSION_NOT_ACTIVE    = 505,  /**< Session not active */
    OPERATION_TIMEOUT     = 506,  /**< Operational timeout */
    NO_FIX                = 516,  /**< No fix */
    GNSS_IS_WORKING       = 522,  /**< GNSS is working */
    UNKNOWN_ERROR         = 549,  /**< Unknown error */
    UNDEFINED             = 999,
};

/**
 * @brief SomLocation class to aquire GNSS location
 *
 */
class SomLocation {
public:
    /**
     * @brief Singleton class instance for SomLocation
     *
     * @return SomLocation&
     */
    static SomLocation& instance()
    {
        if(!_instance)
        {
            _instance = new SomLocation();
        }
        return *_instance;
    }

    /**
     * @brief Configure the SomLocation class
     *
     * @param configuration Structure containing configuration
     * @retval 0 Success
     */
    int begin(LocationConfiguration& configuration);

    /**
     * @brief Get GNSS position, synchronously
     *
     * @param point Location point with position
     * @param publish Publish location point after aquisition
     * @return LocationResults
     */
    LocationResults getLocation(LocationPoint& point, bool publish = false);

    /**
     * @brief Get GNSS position, asynchronously, with given callback
     *
     * @param point Location point with position
     * @param callback Callback function to call after aquisition completion
     * @param publish Publish location point after aquisition
     * @return LocationResults
     */
    LocationResults getLocation(LocationPoint& point, LocationDone callback, bool publish = false);

    /**
     * @brief Get the current acquistion state
     *
     * @retval LocationResults::Idle No current aquisition
     * @retval LocationResults::Acquiring Aquisition in progress
     */
    LocationResults getStatus() const {
        return (_acquiring.load()) ? LocationResults::Acquiring : LocationResults::Idle;
    }

private:
    enum class _ModemType {
        Unavailable,                    /**< Modem type has not been read yet likely because the modem is off */
        Unsupported,                    /**< Modem type is not supported for this GNSS library */
        BG95_M5,                        /**< BG95-M5 modem type */
        EG91,                           /**< EG91 modem type */
    };

    struct QlocContext {
        // QLOC parsed fields
        unsigned int tm_hour {};
        unsigned int tm_min {};
        unsigned int tm_sec {};
        unsigned int tm_day {};
        unsigned int tm_month {};
        unsigned int tm_year {};
        double latitude {};
        double longitude {};
        unsigned int fix {};
        float hdop {};
        float altitude {};
        unsigned int cogDegrees {};
        unsigned int cogMinutes {};
        float speedKmph {};
        float speedKnots {};
        unsigned int nsat {};

        // Time related
        std::tm timeinfo = {};
    };

    struct EpeContext {
        // EPE parsed fields
        float h_acc {};
        float v_acc {};
        float speed_acc {};
        float head_acc {};
    };

    SomLocation();

    bool modemNotDetected() const {
        return (_ModemType::Unavailable == _modemType);
    }

    bool detectModemType();
    void setAntennaPower();
    void clearAntennaPower();

    bool isModemOn() const {
        return Cellular.isOn();
    }

    bool isConnected() const {
        return Particle.connected();
    }

    int setConstellationBg95(LocationConstellation flags);

    LocationCommandContext waitOnCommandEvent(system_tick_t timeout);
    LocationResults waitOnResponseEvent(system_tick_t timeout);
    static void stripLfCr(char* str);
    static int glocCallback(int type, const char* buf, int len, char* locBuffer);
    static int epeCallback(int type, const char* buf, int len, char* epeBuffer);
    CME_Error parseCmeError(const char* buf);
    int parseQloc(const char* buf, QlocContext& context, LocationPoint& point);
    CME_Error parseQlocResponse(const char* buf, QlocContext& context, LocationPoint& point);
    void parseEpeResponse(const char* buf, EpeContext& context, LocationPoint& point);
    void threadLoop();
    size_t buildPublish(char* buffer, size_t len, LocationPoint& point, unsigned int seq);

    static SomLocation* _instance;
    os_queue_t _commandQueue;
    os_queue_t _responseQueue;
    Thread* _thread;
    std::atomic<bool> _acquiring{false};
    char _locBuffer[256];
    char _epeBuffer[256];
    QlocContext _qlocContext {};
    EpeContext _epeContext {};

    LocationConfiguration _conf;
    pin_t _antennaPowerPin {PIN_INVALID};
    _ModemType _modemType {_ModemType::Unavailable};

    char _publishBuffer[particle::protocol::MAX_EVENT_DATA_LENGTH];
    unsigned int _reqid {1};
};

#define Location SomLocation::instance()
