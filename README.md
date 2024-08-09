# Particle SOM GNSS Library

This standalone GNSS (Global Navigation Satellite System) library is designed specifically for Particle cellular modems with built-in GNSS support, including the M-SOM platform.  It provides a simple and efficient interface to access and utilize GNSS data for location-based applications on Particle devices.

## Note
This library is targeted for the M-SOM platform and can only be built with device OS version 5.8.2 and greater.

## Key Features
- Easy Integration: Seamlessly integrates with Particle devices equipped with GNSS functionality, allowing developers to quickly add location services to their IoT projects.
- Comprehensive GNSS Data: Access essential GNSS data, including latitude, longitude, altitude, speed, and heading, directly from your Particle device.
- Lightweight & Standalone: A lightweight library that doesn't require additional dependencies, making it ideal for resource-constrained environments.
- User-Friendly API: Provides a straightforward API to configure, control, retrieve GNSS data, and publish to console maps interfaces with location service.
---
## Installation
You can install the library via Particle's library manager on the command line, VS Code (Workbench), or by directly cloning this repository.
```bash
particle library add particle-som-gnss
```

## API
### Configuration
`int begin(LocationConfiguration& configuration)`

The begin function initializes and configures the Location class using the provided configuration structure.

Parameters
- configuration: A LocationConfiguration structure that contains the configuration settings for the GNSS.

Returns
- int: Returns 0 on success. Any other value indicates a failure during the initialization process.

Defaults
- Passive GNSS antenna - no power applied through GNSS_ANT pin
- GPS+GLONASS constellations
- HDOP under 100 qualifies a fix
- Horizontal accuracy under 50 meters qualifies a fix
- Maximum time for fix is 90 seconds

### Acquisition
`LocationResults getLocation(LocationPoint& point, bool publish = false)`

The getLocation function retrieves the GNSS position synchronously (blocking). It blocks until the location is acquired.

Parameters
- point: A LocationPoint object that will be populated with the GNSS position data.
- publish: (Optional) If set to true, the location data will be published to the cloud after acquisition. The default value is false.

Returns
- LocationResults: An object containing the results of the location acquisition process.


`LocationResults getLocation(LocationPoint& point, LocationDone callback, bool publish = false)`

The getLocation function retrieves the GNSS position asynchronously. It returns immediately and executes the specified callback function upon completion of the acquisition.

Parameters
- point: A LocationPoint object that will be populated with the GNSS position data.
- callback: A LocationDone callback function that is invoked once the location acquisition is complete.
- publish: (Optional) If set to true, the location data will be published to the cloud after acquisition. The default value is false.

Returns
- LocationResults: An object containing the initial result of the location acquisition process.

## Example

See [examples](examples/) for more examples.

```cpp
void setup() {
    LocationConfiguration config;

    // Identify and enable active GNSS antenna power
    config.enableAntennaPower(GNSS_ANT_PWR);

    // Assign buffer to encoder.
    Location.begin(config);
}

void loop() {
    LocationPoint point = {};
    // Get the current GNSS location and publish 'loc' event
    auto results = Location.getLocation(point, true);
    if (results == LocationResults::Fixed) {
        Log.info("Position fixed");
    }
```

---

### LICENSE

Unless stated elsewhere, file headers or otherwise, all files herein are licensed under The MIT License (MIT). For more information, please read the LICENSE file.

If you have questions about software licensing, please contact Particle [support](https://support.particle.io/).

---
### LICENSE

Unless stated elsewhere, file headers or otherwise, all files herein are licensed under an Apache License, Version 2.0. For more information, please read the LICENSE file.

If you have questions about software licensing, please contact Particle [support](https://support.particle.io/).

### LICENSE FAQ

**This firmware is released under Apache License, Version 2.0, what does that mean for you?**

 * You may use this commercially to build applications for your devices!  You **DO NOT** need to distribute your object files or the source code of your application under Apache License.  Your source can be released under a non-Apache license.  Your source code belongs to you when you build an application using this reference firmware.

**When am I required to share my code?**

 * You are **NOT required** to share your application firmware binaries, source, or object files when linking against libraries or System Firmware licensed under LGPL.

**Why?**

 * This license allows businesses to confidently build firmware and make devices without risk to their intellectual property, while at the same time helping the community benefit from non-proprietary contributions to the shared reference firmware.

**Questions / Concerns?**

 * Particle intends for this firmware to be commercially useful and safe for our community of makers and enterprises.  Please [Contact Us](https://support.particle.io/) if you have any questions or concerns, or if you require special licensing.

_(Note!  This FAQ isn't meant to be legal advice, if you're unsure, please consult an attorney)_

---

### CONTRIBUTE

Want to contribute to this library project? Follow [this link](CONTRIBUTING.md) to find out how.

---

### CONNECT

Having problems or have awesome suggestions? Connect with us [here.](https://community.particle.io/)

---

### Revision History

#### 1.0.0
* Initial file commit for M-SOM M404