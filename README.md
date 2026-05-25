# GeoScope v1.0 — Portable GPS Field Survey and Mapping Tool

GeoScope v1.0 is a portable embedded field-survey instrument built around an ESP32-WROOM-DA. The device records GPS position, trip movement, waypoints, environmental conditions, compass heading, tilt/orientation data, and system status. The goal is to create a handheld tool that can collect **location-tagged field data** and provide basic navigation features without relying on a phone or internet connection.

Unlike a simple environmental monitor or signal scanner, GeoScope focuses on adding the missing context of **where** the data was collected.

---

## Project Purpose

GeoScope is designed to answer questions such as:

- Where am I?
- Where have I been?
- How far have I traveled?
- What direction is my target waypoint?
- What were the local environmental conditions at this location?
- How does altitude, temperature, humidity, pressure, or movement change across a path?
- Can I save field points and navigate back to them?

The device is intended for outdoor testing, field experiments, mapping, trail logging, environmental surveys, and embedded systems learning.

---

## Project Line Context

GeoScope is part of a broader line of embedded diagnostic tools:

| Project | Main Focus |
|---|---|
| **MSER v2.0** | Environmental sensing and data monitoring |
| **Signal Scanner** | Wireless, RF, EMF and electrical signal diagnostics |
| **GeoScope v1.0** | GPS-based field surveying, mapping, and navigation |

GeoScope adds a location and movement layer to the diagnostic tool family.

---

## Core Features

- Live GPS dashboard
- Live trail/path map on TFT display
- Waypoint marking
- Navigate-to-waypoint mode
- Return-to-start mode
- Trip recorder
- Distance traveled calculation
- GPS speed and altitude display
- Satellite/fix status display
- Compass heading using magnetometer
- Tilt/orientation display using MPU-6050
- Environmental readings using BME280
- Pressure altitude estimate
- CSV logging to MicroSD card
- Buzzer alerts for events
- Portable battery-powered operation
- Three-button menu navigation

---

## Locked Hardware Scope

| Component | Purpose |
|---|---|
| **ESP32-WROOM-DA Dev Board** | Main microcontroller |
| **NEO-M8N GPS Module** | Latitude, longitude, GPS altitude, speed, time, course, satellite data |
| **2.4 inch SPI TFT Display** | Main interface, menus, maps, trip data, navigation screens |
| **0.96 inch I2C OLED Display #1** | Compass and tilt/orientation display |
| **0.96 inch I2C OLED Display #2** | GPS satellite and fix-status display |
| **MicroSD Card Module** | CSV logging for trips, waypoints, environmental data, and events |
| **BME280 Sensor** | Temperature, humidity, pressure, and pressure-altitude estimate |
| **BMM150 Magnetometer** | Compass heading and navigation direction support |
| **MPU-6050** | Pitch, roll, motion detection, shake/impact detection |
| **3 Push Buttons** | Mode/menu, up, and down navigation |
| **Active Buzzer** | Alerts for waypoint save, arrival, GPS fix, SD error, and low battery |
| **Power Circuit** | Portable battery-powered operation |

![Alt text](20251119_133702.jpg)

---

## Display Roles

GeoScope uses one main display and two support displays.

| Display | Role |
|---|---|
| **2.4 inch SPI TFT** | Main UI, GPS dashboard, live map, trip screen, navigation screen |
| **OLED #1** | Compass heading, pitch, roll, tilt warnings |
| **OLED #2** | GPS fix, satellite count, HDOP, logging status |

This gives the project a multi-panel field-instrument layout. The TFT handles larger visual information, while the OLEDs provide always-visible status data.

---

## Main Operating Modes

### 1. GPS Dashboard

Displays live GPS information.

Shown values:

- GPS fix status
- Satellite count
- Latitude
- Longitude
- GPS altitude
- Speed
- Course over ground
- GPS time/date

Example display concept:

```text
GEOSCOPE GPS
Fix: 3D   Sat: 12
Lat: 43.826100
Lon:-111.789700
Alt: 1486m Spd:1.2m/s
```

---

### 2. Live Track Map

Draws a simple live GPS trail on the TFT display.

This is not a full street map. Instead, it plots the path using GPS coordinates converted into screen positions.

Shown elements:

- Current position
- Start point
- Path history
- Waypoint markers
- Target direction line
- Distance traveled
- Scale indicator

Example concept:

```text
+--------------------------+
| GEOSCOPE MAP   Fix: 3D   |
|                          |
|      WPT2                |
|       *                  |
|        \                 |
| START *--\-----> YOU     |
|                          |
| Dist: 0.84 km            |
+--------------------------+
```

---

### 3. Waypoint Marker

Allows the user to save the current GPS position as a waypoint.

Possible waypoint types:

- Start
- Sample point
- Observation point
- Hazard point
- Target point
- Custom point

Example:

```text
WAYPOINT SAVED
ID: WPT_004
Lat: 43.826100
Lon:-111.789700
```

---

### 4. Waypoint Navigation

Guides the user toward a selected waypoint or manually entered target coordinate.

Displayed values:

- Distance to target
- Bearing to target
- Current compass heading
- Turn direction
- Arrival status

Example:

```text
NAV TO TARGET
Dist: 248 m
Bearing: 126 deg
Heading: 101 deg
Turn: RIGHT 25 deg
```

This mode works like a field GPS compass rather than road navigation. It gives direction and distance, but it does not know roads, sidewalks, buildings, fences, or obstacles.

---

### 5. Return to Start

When a trip begins, GeoScope stores the starting GPS coordinate.

Return-to-start mode guides the user back to the starting point using:

- Distance to start
- Bearing to start
- Current heading
- Turn left/right guidance
- Arrival alert

This is one of the most practical field features of the device.

---

### 6. Trip Recorder

Tracks and logs movement over time.

Trip values:

- Total distance traveled
- Current speed
- Average speed
- Maximum speed
- Moving time
- Stopped time
- GPS altitude
- Estimated altitude gain/loss
- Waypoint count

Example:

```text
TRIP RECORDER
Dist: 1.42 km
Speed: 1.1 m/s
Alt: 1486 m
WPT: 004
```

---

### 7. Environmental Survey

Combines GPS data with BME280 readings.

Logged values:

- Latitude
- Longitude
- GPS altitude
- Temperature
- Humidity
- Pressure
- Pressure altitude
- Timestamp

This makes environmental data map-ready because every reading is tied to a specific location.

---

### 8. Compass and Tilt Display

OLED #1 provides a quick directional/orientation panel.

Data sources:

- BMM150 magnetometer for heading
- MPU-6050 for pitch and roll

Example:

```text
COMPASS
Head: 274 deg
Pitch: -4 deg
Roll: 12 deg
```

Potential warnings:

- Hold device level
- Magnetic interference detected
- Motion too unstable for accurate heading

---

### 9. Satellite Status Display

OLED #2 provides GPS status.

Example:

```text
GPS STATUS
Fix: 3D
Sat: 12
HDOP: 0.9
Log: ON
```

This allows the main TFT screen to remain focused on the active mode while GPS status remains visible.

---

## Button Controls

GeoScope uses three push buttons.

| Button | Short Press | Long Press |
|---|---|---|
| **MODE / MENU** | Change mode | Open or close menu |
| **UP** | Scroll up / zoom in | Save waypoint |
| **DOWN** | Scroll down / zoom out | Start or stop logging |

This layout keeps the hardware simple while still supporting menus, navigation, waypoint marking, and logging.

---

## Sensor Roles

### NEO-M8N GPS Module

Used for:

- Latitude
- Longitude
- GPS altitude
- GPS speed
- GPS course while moving
- GPS time/date
- Satellite count
- Fix status

---

### BME280

Used for:

- Temperature
- Humidity
- Barometric pressure
- Pressure altitude estimate

Pressure altitude formula:

```text
h = 44330 * (1 - (P / P0)^0.1903)
```

Where:

- `h` = altitude in meters
- `P` = measured pressure in hPa
- `P0` = sea-level reference pressure, commonly 1013.25 hPa

---

### BMM150 Magnetometer

Used for:

- Compass heading
- Stationary direction sensing
- Bearing comparison
- Navigation arrow support

The magnetometer is important because GPS course is only reliable while moving. The BMM150 allows the device to estimate direction even when standing still.

---

### MPU-6050

Used for:

- Pitch
- Roll
- Motion detection
- Shake detection
- Impact/shock events
- Orientation assist
- Hold-level warning

The MPU-6050 does not replace the magnetometer because it does not detect magnetic north. It works together with the BMM150 to improve the handheld navigation experience.

---

## Navigation Calculations

### Distance to Target

For accurate GPS distance calculations, GeoScope can use the Haversine formula.

```cpp
double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;

  double phi1 = radians(lat1);
  double phi2 = radians(lat2);
  double dPhi = radians(lat2 - lat1);
  double dLambda = radians(lon2 - lon1);

  double a = sin(dPhi / 2) * sin(dPhi / 2) +
             cos(phi1) * cos(phi2) *
             sin(dLambda / 2) * sin(dLambda / 2);

  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return R * c;
}
```

---

### Bearing to Target

```cpp
double bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  double phi1 = radians(lat1);
  double phi2 = radians(lat2);
  double dLambda = radians(lon2 - lon1);

  double y = sin(dLambda) * cos(phi2);
  double x = cos(phi1) * sin(phi2) -
             sin(phi1) * cos(phi2) * cos(dLambda);

  double theta = atan2(y, x);
  double bearing = degrees(theta);

  if (bearing < 0) bearing += 360.0;

  return bearing;
}
```

---

### Turn Direction

```cpp
double turnAngle(double targetBearing, double currentHeading) {
  double diff = targetBearing - currentHeading;

  while (diff > 180) diff -= 360;
  while (diff < -180) diff += 360;

  return diff;
}
```

Interpretation:

```text
diff > 0  -> turn right
diff < 0  -> turn left
diff near 0 -> go straight
```

---

## Live Map Concept

The TFT live map converts GPS position changes into screen coordinates.

Basic idea:

```text
latitude/longitude -> meters north/east -> TFT x/y pixels
```

Approximate local conversion:

```cpp
float north_m = (currentLat - startLat) * 111320.0;
float east_m  = (currentLon - startLon) * 111320.0 * cos(radians(startLat));

int x = centerX + east_m / metersPerPixel;
int y = centerY - north_m / metersPerPixel;
```

Then the display draws the path:

```cpp
tft.drawLine(oldX, oldY, x, y, pathColor);
tft.fillCircle(x, y, 3, currentPositionColor);
```

The first version should use a simple trail map instead of full offline street maps.

---

## Data Logging

GeoScope logs data to a MicroSD card using CSV files. CSV is easy to open in Excel, Google Sheets, Python, MATLAB, or mapping tools.

### Main Trip Log

Suggested filename:

```text
TRIP_001.csv
```

Suggested columns:

```csv
timestamp,lat,lon,gps_alt_m,speed_mps,course_deg,satellites,fix,temp_c,humidity_pct,pressure_hpa,pressure_alt_m,heading_deg,pitch_deg,roll_deg
```

Example:

```csv
2026-05-10 14:32:10,43.826100,-111.789700,1486.2,1.2,274,12,3D,21.4,38.0,846.3,1480.8,271.5,-4.0,12.0
```

---

### Waypoint Log

Suggested filename:

```text
WAYPOINTS.csv
```

Suggested columns:

```csv
waypoint_id,timestamp,type,lat,lon,gps_alt_m,note
```

Example:

```csv
WPT_004,2026-05-10 14:41:22,SAMPLE,43.826100,-111.789700,1486.2,Field sample point
```

---

### System Log

Suggested filename:

```text
SYSTEM.csv
```

Suggested columns:

```csv
timestamp,battery_v,gps_fix,satellites,sd_status,mode,event
```

Example:

```csv
2026-05-10 14:32:10,4.05,3D,12,OK,GPS_DASHBOARD,LOGGING_STARTED
```

---

## Suggested Software Libraries

Possible Arduino IDE libraries:

| Function | Suggested Library |
|---|---|
| GPS parsing | `TinyGPSPlus` |
| TFT display | `TFT_eSPI` or `Adafruit_ILI9341` |
| OLED displays | `Adafruit_SSD1306` and `Adafruit_GFX` |
| MicroSD card | `SD` or `SdFat` |
| BME280 | `Adafruit_BME280` |
| BMM150 | `DFRobot_BMM150` or compatible BMM150 library |
| MPU-6050 | `Adafruit_MPU6050` |
| I2C communication | `Wire` |
| SPI communication | `SPI` |

---

## Proposed Pin Mapping

> Final pin mapping may change depending on the exact TFT, SD module, GPS breakout, and available ESP32-WROOM-DA dev board pins.

| Component | Interface | Suggested ESP32 Pins |
|---|---|---|
| TFT Display SCK | SPI | GPIO18 |
| TFT Display MOSI | SPI | GPIO23 |
| TFT Display MISO | SPI | GPIO19, if needed |
| TFT Display CS | SPI | GPIO5 |
| TFT Display DC | GPIO | GPIO2 |
| TFT Display RST | GPIO | GPIO4 |
| MicroSD SCK | SPI shared | GPIO18 |
| MicroSD MOSI | SPI shared | GPIO23 |
| MicroSD MISO | SPI shared | GPIO19 |
| MicroSD CS | SPI | GPIO13 |
| GPS TX to ESP32 RX | UART | GPIO16 |
| GPS RX to ESP32 TX | UART | GPIO17 |
| I2C SDA | I2C | GPIO21 |
| I2C SCL | I2C | GPIO22 |
| Button MODE | GPIO input | GPIO25 |
| Button UP | GPIO input | GPIO26 |
| Button DOWN | GPIO input | GPIO27 |
| Active Buzzer | GPIO output | GPIO14 |
| Battery Monitor | ADC | GPIO32 |

---

## I2C Devices

The following modules share the I2C bus:

- OLED #1
- OLED #2
- BME280
- BMM150
- MPU-6050

Important note: two identical OLED displays often use the same default I2C address, commonly `0x3C`. If both OLEDs have the same address and cannot be changed, an I2C multiplexer such as the **TCA9548A** may be required.

Possible I2C address examples:

| Device | Common Address |
|---|---|
| OLED SSD1306 | `0x3C` or `0x3D` |
| BME280 | `0x76` or `0x77` |
| BMM150 | often `0x10` to `0x13`, depending on breakout/library |
| MPU-6050 | `0x68` or `0x69` |
| TCA9548A I2C Multiplexer | `0x70` |

---

## Power Notes

GeoScope is designed to be portable, so power stability is important.

Recommended power considerations:

- Use a stable 3.3 V supply for ESP32 and sensors.
- Verify whether the GPS, TFT, OLEDs, and SD module require 3.3 V or 5 V input.
- Use common ground for all modules.
- Add decoupling capacitors near display, GPS, and SD module power pins.
- Avoid powering high-current displays directly from weak regulator outputs.
- Monitor battery voltage through a voltage divider connected to an ESP32 ADC pin.
- Add low-battery warning through the buzzer and display.

---

## Development Plan

### Phase 1 — Core Bring-Up

- Test ESP32 board
- Test TFT display
- Test OLED #1
- Test OLED #2
- Test button input
- Test buzzer output

---

### Phase 2 — GPS System

- Read GPS data from NEO-M8N
- Display latitude and longitude
- Display satellite count and fix status
- Display GPS speed and altitude
- Show GPS time/date

---

### Phase 3 — Sensors

- Read BME280 temperature, humidity, and pressure
- Calculate pressure altitude
- Read BMM150 compass heading
- Read MPU-6050 pitch and roll
- Display compass and tilt on OLED #1
- Display GPS status on OLED #2

---

### Phase 4 — SD Logging

- Initialize MicroSD card
- Create trip CSV file
- Log GPS and sensor readings
- Create waypoint CSV file
- Create system event log

---

### Phase 5 — Navigation

- Save start position
- Save waypoint
- Calculate distance to waypoint
- Calculate bearing to waypoint
- Compare bearing with heading
- Display turn left/right guidance
- Trigger arrival buzzer alert

---

### Phase 6 — Live Track Map

- Convert GPS coordinates to local x/y map points
- Draw current position
- Draw path history
- Draw start point
- Draw waypoint markers
- Add zoom or scaling behavior

---

### Phase 7 — Polish

- Improve menu UI
- Add icons and status indicators
- Add startup animation
- Add error screens
- Add low battery warning
- Add enclosure and wiring cleanup
- Create final documentation and screenshots

---

## Expected Challenges

- Getting reliable GPS fix indoors may be difficult.
- Magnetometer calibration is required for accurate heading.
- The BMM150 may be affected by nearby metal, batteries, wires, speakers, and magnets.
- Two OLED displays may conflict if they share the same I2C address.
- SD card writing can fail if power is unstable.
- TFT and SD card sharing SPI requires careful chip-select handling.
- GPS altitude may be noisy compared with pressure altitude.
- MPU-6050 orientation can drift if used incorrectly.
- Outdoor testing is needed to validate navigation and mapping features.

---

## Future Improvements

Possible GeoScope v1.1 or v2.0 upgrades:

- GPX export
- KML export for Google Earth
- GeoJSON export
- Larger TFT display
- Offline map tile support
- LoRa beacon mode
- Wi-Fi signal strength mapping
- UV exposure mapping
- Light intensity mapping
- Battery percentage estimation
- Recharge/charging status display
- Enclosure with weather resistance
- Vibration motor for silent navigation feedback
- Rotary encoder for easier menu navigation
- TCA9548A I2C multiplexer for cleaner multi-OLED support
- Compass calibration wizard
- Waypoint naming system
- Route summary screen

---

## Example Use Cases

- Record a walking route across campus
- Mark environmental sample points
- Navigate back to the start of a route
- Compare GPS altitude with pressure altitude
- Create a temperature/humidity map of an area
- Save waypoints during field observations
- Log trail distance and movement data
- Test GPS and compass algorithms
- Build a handheld field-data collection device
- Learn embedded navigation, sensor fusion, and data logging

---

## Project Status

Current status: **Planning / hardware scope locked**

Locked hardware:

- ESP32-WROOM-DA Dev Board
- NEO-M8N GPS Module
- 2.4 inch SPI TFT Display
- Two 0.96 inch I2C OLED Displays
- MicroSD Card Module
- BME280 Sensor
- BMM150 Magnetometer
- MPU-6050
- Three Push Buttons
- Active Buzzer
- Power Circuit

---

## License

This project can be released under the MIT License or another open-source license of choice.

---

## Author

Created by Victor Granado as part of a personal embedded systems diagnostic-tool series.
