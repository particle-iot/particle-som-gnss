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

#include "Particle.h"
#include "location.h"

#if (PLATFORM_ID != PLATFORM_MSOM)
#error "This library can only be built for M-SOM devices"
#endif // PLATFORM_MSOM

#ifndef SYSTEM_VERSION_v582
#error "This library must be built with device OS version >= 5.8.2"
#endif // SYSTEM_VERSION_v582

constexpr system_tick_t LOCATION_PERIOD_SUCCESS_MS {1 * 1000};
constexpr system_tick_t LOCATION_INACTIVE_PERIOD_SUCCESS_MS {120 * 1000};
constexpr system_tick_t LOCATION_PERIOD_ACQUIRE_MS {1 * 1000};
constexpr system_tick_t ANTENNA_POWER_SETTLING_MS {100};
constexpr int LOCATION_REQUIRED_SETTLING_COUNT {2};  // Number of consecutive fixes

Logger locationLog("loc");

SomLocation *SomLocation::_instance = nullptr;

SomLocation::SomLocation() {
    os_queue_create(&_commandQueue, sizeof(LocationCommandContext), 1, nullptr);
    os_queue_create(&_responseQueue, sizeof(LocationResults), 1, nullptr);
    _thread = new Thread("gnss_cellular", [this]() {SomLocation::threadLoop();}, OS_THREAD_PRIORITY_DEFAULT);
}

bool SomLocation::detectModemType() {
    bool detected = false;

    if (modemNotDetected() && isModemOn()) {
        CellularDevice celldev = {};
        cellular_device_info(&celldev, nullptr);
        locationLog.trace("Modem ID is %d", celldev.dev);
        switch (celldev.dev) {
          case 0:
            locationLog.trace("Modem not cached yet");
            // Do not set modem type
            break;

          case DEV_QUECTEL_BG95_M5:
            _modemType = _ModemType::BG95_M5;
            locationLog.trace("BG95-M5 detected");
            detected = true;
            break;

        //   case DEV_QUECTEL_EG91_EX:
        //     _modemType = _ModemType::EG91;
        //     locationLog.trace("EG91-EX detected");
        //     detected = true;
        //     break;

          default:
            _modemType = _ModemType::Unsupported;
            locationLog.trace("Modem type %d not supported", celldev.dev);
            break;
        }
    }
    else if (!modemNotDetected() && (_ModemType::Unsupported != _modemType)) {
        detected = true;
    }

    return detected;
}

void SomLocation::setAntennaPower() {
    if (PIN_INVALID != _antennaPowerPin) {
        digitalWrite(_antennaPowerPin, HIGH);
        delay(ANTENNA_POWER_SETTLING_MS);
    }
}

void SomLocation::clearAntennaPower() {
    if (PIN_INVALID != _antennaPowerPin) {
        digitalWrite(_antennaPowerPin, LOW);
    }
}

int SomLocation::setConstellationBg95(LocationConstellation flags) {
    int configNumber = 1;
    if ((flags & LOCATION_CONST_GPS_ONLY) ||
        (flags & LOCATION_CONST_GPS_GLONASS)) {

        configNumber = 1; // GPS + GLONASS
    }
    else if (flags & LOCATION_CONST_GPS_BEIDOU) {
        configNumber = 2; // GPS + BeiDou
    }
    else if (flags & LOCATION_CONST_GPS_GALILEO) {
        configNumber = 3; // GPS + Galileo
    }
    else if (flags & LOCATION_CONST_GPS_QZSS) {
        configNumber = 4; // GPS + QZSS
    }

    char command[64] = {};
    sprintf(command, "AT+QGPSCFG=\"gnssconfig\",%d", configNumber);
    Cellular.command(command);
    return 0;
}

int SomLocation::begin(LocationConfiguration& configuration) {
    locationLog.info("Beginning location library");
    _conf = configuration;
    _antennaPowerPin = _conf.enableAntennaPower();
    if (PIN_INVALID != _antennaPowerPin) {
        locationLog.info("Configuring antenna pin");
        pinMode(_antennaPowerPin, OUTPUT);
    }

    if (isModemOn() && modemNotDetected()) {
        locationLog.info("Detecting modem type");
        detectModemType();

        if (_ModemType::BG95_M5 == _modemType) {
            setConstellationBg95(_conf.constellations());
        }
    }

    return 0;
}

LocationResults SomLocation::getLocation(LocationPoint& point, bool publish) {
    if (!isModemOn()) {
        locationLog.trace("Modem is not on");
        return LocationResults::Unavailable;
    }
    if (modemNotDetected()) {
        auto detected = detectModemType();
        if (!detected) {
            locationLog.trace("Modem is not supported");
            return LocationResults::Unsupported;
        }
    }

    // Check if already running
    if (_acquiring.load()) {
        locationLog.trace("Aquisition is already underway");
        return LocationResults::Pending;
    }
    locationLog.trace("Starting synchronous aquisition");
    LocationCommandContext event {};
    event.command = LocationCommand::Acquire;
    event.point = &point;
    event.sendResponse = true;
    os_queue_put(_commandQueue, &event, 0, nullptr);
    auto result = waitOnResponseEvent((system_tick_t)_conf.maximumFixTime() * 1000 + LOCATION_PERIOD_ACQUIRE_MS);
    if (publish && (LocationResults::Fixed == result) && isConnected()) {
        locationLog.info("Publishing loc event");
        buildPublish(_publishBuffer, sizeof(_publishBuffer), point, _reqid);
        auto published = Particle.publish("loc", _publishBuffer);
        if (published) {
            _reqid++;
        }
    }
    return result;
}

LocationResults SomLocation::getLocation(LocationPoint& point, LocationDone callback, bool publish) {
    if (!isModemOn()) {
        locationLog.trace("Modem is not on");
        return LocationResults::Unavailable;
    }
    if (modemNotDetected()) {
        auto detected = detectModemType();
        if (!detected) {
            locationLog.trace("Modem is not supported");
            return LocationResults::Unsupported;
        }
    }

    // Check if already running
    if (_acquiring.load()) {
        locationLog.trace("Aquisition is already underway");
        return LocationResults::Pending;
    }
    locationLog.trace("Starting asynchronous aquisition");
    LocationCommandContext event {};
    event.command = LocationCommand::Acquire;
    event.point = &point;
    event.doneCallback = callback;
    event.publish = publish;
    os_queue_put(_commandQueue, &event, 0, nullptr);
    return LocationResults::Acquiring;
}

LocationCommandContext SomLocation::waitOnCommandEvent(system_tick_t timeout) {
    LocationCommandContext event = {};
    auto ret = os_queue_take(_commandQueue, &event, timeout, nullptr);
    if (ret) {
        event.command = LocationCommand::None;
        event.point = nullptr;
    }

    return event;
}

LocationResults SomLocation::waitOnResponseEvent(system_tick_t timeout) {
    LocationResults event = {LocationResults::Idle};
    auto ret = os_queue_take(_responseQueue, &event, timeout, nullptr);
    if (ret) {
        event = LocationResults::Idle;
    }

    return event;
}

void SomLocation::stripLfCr(char* str) {
    if (!str) {
        return;
    }

    auto read = str;
    auto write = str;

    while ('\0' != *read) {
        if (('\n' != *read) && ('\r' != *read)) {
            *write++ = *read;
        }
        ++read;
    }

    // Null-terminate the resulting string
    *write = '\0';
}

int SomLocation::glocCallback(int type, const char* buf, int len, char* locBuffer) {
    switch (type) {
        case TYPE_PLUS:
            // fallthrough
        case TYPE_ERROR:
            strlcpy(locBuffer, buf, min((size_t)len, sizeof(SomLocation::_locBuffer)));
            stripLfCr(locBuffer);
            locationLog.trace("glocCallback: (%06x) %s", type, locBuffer);
            break;
    }

    return WAIT;
}

int SomLocation::epeCallback(int type, const char* buf, int len, char* epeBuffer) {
    switch (type) {
        case TYPE_PLUS:
            // fallthrough
        case TYPE_ERROR:
            strlcpy(epeBuffer, buf, min((size_t)len, sizeof(SomLocation::_epeBuffer)));
            stripLfCr(epeBuffer);
            break;
    }

    return WAIT;
}

CME_Error SomLocation::parseCmeError(const char* buf) {
    unsigned int error_code = 0;
    auto nargs = sscanf(buf," +CME ERROR: %u", &error_code);

    if (0 == nargs) {
        return CME_Error::NONE;
    }

    auto ret = CME_Error::UNDEFINED;

    switch (error_code) {
        case 504:
            // fallthrough
        case 505:
            // fallthrough
        case 506:
            // fallthrough
        case 516:
            // fallthrough
        case 522:
            // fallthrough
        case 549:
            ret = static_cast<CME_Error>(error_code);
            break;
    }

    return ret;
}

int SomLocation::parseQloc(const char* buf, QlocContext& context, LocationPoint& point) {
    // The general form of the AT command response is as follows
    // <UTC HHMMSS.hh>,<latitude (-)dd.ddddd>,<longitude (-)ddd.ddddd>,<HDOP>,<altitude>,<fix>,<COG ddd.mm>,<spkm>,<spkn>,<date DDmmyy>,<nsat>
    auto nargs = sscanf(buf, " +QGPSLOC: %02u%02u%02u.%*03u,%lf,%lf,%f,%f,%u,%03u.%02u,%f,%f,%02u%02u%02u,%u",
                        &context.tm_hour, &context.tm_min, &context.tm_sec,
                        &context.latitude, &context.longitude, &context.hdop, &context.altitude,
                        &context.fix, &context.cogDegrees, &context.cogMinutes, &context.speedKmph, &context.speedKnots,
                        &context.tm_day, &context.tm_month, &context.tm_year,
                        &context.nsat);

    if (0 == nargs) {
        return -1;
    }

    // Although there are several QLOC output options, we are taking the format that gives us the appropriate number of
    // significant digits for the supported accuracy.
    // QLOC=0 would give us ddmm.mmmmN/S, dddmm.mmmmE/W resulting in 8 significant digits for latitude and 9 in longitude
    // QLOC=1 would give us ddmm.mmmmmm,N/S, dddmm.mmmmmm,E/W resulting in 10 significant digits for latitude and 11 in longitude
    // QLOC=2 would give us (-)dd.ddddd, (-)ddd.ddddd resulting in 7 significant digits for latitude and 8 in longitude

    // Convert tm structure to time_t (epoch time)
    context.timeinfo.tm_year = context.tm_year + 2000 - 1900;  // GPRMC year from 2000 and then the difference from 1900
    context.timeinfo.tm_mon = context.tm_month - 1;     // The number of months since January (0-11)
    context.timeinfo.tm_mday = context.tm_day;
    context.timeinfo.tm_hour = context.tm_hour;
    context.timeinfo.tm_min = context.tm_min;
    context.timeinfo.tm_sec = context.tm_sec;
    point.epochTime = std::mktime(&context.timeinfo);

    point.fix = context.fix;
    point.latitude = context.latitude;
    point.longitude = context.longitude;
    point.altitude = context.altitude;
    point.speed = context.speedKmph * 1000.0;
    point.heading = (float)context.cogDegrees + (float)context.cogMinutes / 60.0;
    point.horizontalDop = context.hdop;
    point.satsInUse = context.nsat;

    return 0;
}

CME_Error SomLocation::parseQlocResponse(const char* buf, QlocContext& context, LocationPoint& point) {
    // Only expect the following CME error codes if present
    //   CME_Error::SESSION_IS_ONGOING - if GNSS is not enabled or ready
    //   CME_Error::SESSION_NOT_ACTIVE - if GNSS is not enabled or ready
    //   CME_Error::NO_FIX - if GNSS acquiring and not fixed
    auto result = parseCmeError(buf);

    if (CME_Error::NO_FIX == result) {
        point.fix = 0;
        return result; // module explicitly reported GNSS no fix
    }
    if (CME_Error::NONE != result) {
        return CME_Error::NONE;  // module just may have not been initialized
    }

    parseQloc(buf, context, point);
    return CME_Error::FIX;
}

void SomLocation::parseEpeResponse(const char* buf, EpeContext& context, LocationPoint& point) {
    // Only expect the following CME error codes
    //   CME_Error::SESSION_IS_ONGOING - if GNSS is not enabled or ready
    //   CME_Error::SESSION_NOT_ACTIVE - if GNSS is not enabled or ready
    //   CME_Error::NO_FIX - if GNSS acquiring and not fixed
    auto result = parseCmeError(buf);

    if (CME_Error::NONE != result) {
        return;  // module just may have not been initialized
    }

    auto nargs = sscanf(buf, " +QGPSCFG: \"estimation_error\",%f,%f,%f,%f",
                        &context.h_acc, &context.v_acc, &context.speed_acc, &context.head_acc);

    if (nargs) {
        point.horizontalAccuracy = context.h_acc;
        point.verticalAccuracy = context.v_acc;
    }

    return;
}

void SomLocation::threadLoop()
{
    auto loop = true;
    while (loop) {
        // Look for requests and provide a loop delay
        auto event = waitOnCommandEvent(LOCATION_PERIOD_SUCCESS_MS);

        switch (event.command) {
            case LocationCommand::None:
                // Do nothing
                break;

            case LocationCommand::Acquire: {
                _acquiring.store(true);
                SCOPE_GUARD({
                    _acquiring.store(false);
                    clearAntennaPower();
                });

                setAntennaPower();

                locationLog.trace("Started aquisition");
                Cellular.command(R"(AT+QGPS=1)");
                if (_ModemType::BG95_M5 == _modemType) {
                    Cellular.command(R"(AT+QGPSCFG="nmea_epe",1)");
                    setConstellationBg95(_conf.constellations());
                }
                auto maxTime = (uint64_t)_conf.maximumFixTime() * 1000;
                uint64_t firstFix = {};
                int fixCount = {};
                LocationResults response {LocationResults::TimedOut};
                bool power = false;
                auto start = System.millis();
                while ((power = isModemOn())) {
                    auto now = System.millis();
                    if ((now - start) >= maxTime)
                        break;
                    Cellular.command(glocCallback, _locBuffer, 1000, R"(AT+QGPSLOC=2)");
                    auto ret = parseQlocResponse(_locBuffer, _qlocContext, *event.point);
                    if (CME_Error::FIX == ret) {
                        fixCount++;
                        if (0 == firstFix) {
                            firstFix = System.millis();
                            event.point->systemTime = Time.now();
                        }
                    }
                    if (_ModemType::BG95_M5 == _modemType) {
                        Cellular.command(epeCallback, _epeBuffer, 1000, R"(AT+QGPSCFG="estimation_error")");
                        parseEpeResponse(_epeBuffer, _epeContext, *event.point);
                    }
                    if ((CME_Error::FIX == ret) && (LOCATION_REQUIRED_SETTLING_COUNT == fixCount) &&
                        (event.point->horizontalDop <= _conf.hdopThreshold()) &&
                        (event.point->horizontalAccuracy <= _conf.haccThreshold())) {

                        response = LocationResults::Fixed;
                        break;
                    }
                    delay(LOCATION_PERIOD_ACQUIRE_MS);
                }

                Cellular.command(R"(AT+QGPSEND)");

                if (!power && (LocationResults::Fixed != response)) {
                    response = LocationResults::Unavailable;
                }

                if (firstFix)
                    event.point->timeToFirstFix = (float)(firstFix - start) / 1000.0;

                if (event.sendResponse) {
                    locationLog.trace("Sending synchronous completion");
                    os_queue_put(_responseQueue, &response, 0, nullptr);
                }
                else if (event.doneCallback) {
                    if (event.publish && (LocationResults::Fixed == response) && isConnected()) {
                        locationLog.info("Publishing loc event");
                        buildPublish(_publishBuffer, sizeof(_publishBuffer), *event.point, _reqid);
                        auto published = Particle.publish("loc", _publishBuffer);
                        if (published) {
                            _reqid++;
                        }
                    }
                    locationLog.trace("Sending asynchronous completion");
                    event.doneCallback(response);
                }

                break;
            }

            case LocationCommand::Exit:
                // Get out of main loop and join
                loop = false;
                break;

            default:
                break;
        }
    }

    // Kill the thread if we get here
    _thread->cancel();
}

size_t SomLocation::buildPublish(char* buffer, size_t len, LocationPoint& point, unsigned int seq) {
    memset(buffer, 0, len);
    JSONBufferWriter writer(buffer, len);
    writer.beginObject();
        writer.name("cmd").value("loc");
        if (point.systemTime) {
            writer.name("time").value((unsigned int)point.systemTime);
        }
        writer.name("loc");
        writer.beginObject();
        if (0 == point.fix) {
            writer.name("lck").value(0);
        }
        else {
            writer.name("lck").value(1);
            writer.name("time").value((unsigned int)point.epochTime);
            writer.name("lat").value(point.latitude, 8);
            writer.name("lon").value(point.longitude, 8);
            writer.name("alt").value(point.altitude, 3);
            writer.name("hd").value(point.heading, 2);
            writer.name("spd").value(point.speed, 2);
            writer.name("hdop").value(point.horizontalDop, 1);
            if (0.0 < point.horizontalAccuracy) {
                writer.name("h_acc").value(point.horizontalAccuracy, 3);
            }
            if (0.0 < point.verticalAccuracy) {
                writer.name("v_acc").value(point.verticalAccuracy, 3);
            }
            writer.name("nsat").value(point.satsInUse);
            writer.name("ttff").value(point.timeToFirstFix, 1);
        }
        writer.endObject();
        writer.name("req_id").value(seq);
    writer.endObject();

    return writer.dataSize();
}
