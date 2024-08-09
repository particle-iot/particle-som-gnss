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

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup() {
    waitFor(Serial.isConnected, 10000);

    LocationConfiguration config;
    config.enableAntennaPower(GNSS_ANT_PWR);
    Location.begin(config);
}

LocationPoint point = {};
bool ready = true;

void getCb(LocationResults results) {
    Log.info("async callback returned %d", (int)results);
    if (results == LocationResults::Fixed)
        Log.info("async callback reporting fixed");
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();

        switch (c) {
            case 'g': {
                Location.getLocation(point, getCb, true);
                Log.info("GNSS acquistion started");
                break;
            }

            case 'p': {
                auto fixed = (point.fix) ? true : false;
                if (fixed) {
                    Log.info("Position fixed!");
                    Log.info("Lat %0.5lf, lon %0.5lf", point.latitude, point.longitude);
                    Log.info("Alt %0.1f m, speed %0.1f m/s, heading %0.1f deg", point.altitude, point.speed, point.heading);
                }
                else {
                    Log.info("Position not fixed. :(");
                }
                break;
            }
        }
    }
}
