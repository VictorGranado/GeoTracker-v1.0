# GeoTracker v1.0 — Portable GPS Field Survey and Mapping Tool

GeoTracker v1.0 is a finished handheld embedded field-survey instrument built around an ESP32-WROOM-DA. It records GPS position, trip movement, waypoints, environmental conditions, compass heading, orientation data, and system status to a MicroSD card.

The goal of GeoTracker is to collect **location-tagged field data**: not just sensor readings, but sensor readings tied to **where**, **when**, and **under what movement/orientation conditions** the data was collected.

The project now has two complementary parts:

1. **GeoTracker v1.0** — the completed handheld embedded field-data acquisition device.  
2. **GeoTracker Studio** — the Python desktop visualization and analysis companion for importing SD card logs, viewing 2D/3D routes, analyzing sensor data, and preparing exports.

Unlike a simple environmental monitor or wireless scanner, GeoTracker focuses on adding the missing spatial layer: **where the data was collected and how the device moved through that space**.

---

## Final Build Status

**Status: Completed — GeoTracker v1.0 final hardware build**

The v1.0 build has completed:

- Hardware integration
- Wiring and assembly
- Multi-display UI
- GPS acquisition
- Compass/navigation display
- GPS status display
- BME280 environmental sensing
- MPU-6050 motion/orientation sensing
- BMM150 compass heading
- MicroSD CSV logging
- Keypad-based navigation and coordinate input
- Waypoint saving
- Tracking start/stop
- Return-to-start/navigation logic foundation

---

## Project Purpose

GeoTracker is designed to answer questions such as:

- Where am I?
- Where have I been?
- How far have I traveled?
- What direction is my target waypoint?
- Can I save a field point and navigate back to it?
- What were the local environmental conditions at this location?
- How did altitude, temperature, humidity, pressure, heading, or movement change along a path?
- Can I import the collected data into GeoTracker Studio for 2D/3D visualization and analysis?

The device is intended for outdoor testing, field experiments, mapping, trail logging, environmental surveys, embedded systems learning, and GPS/sensor visualization workflows through GeoTracker Studio.

---

## Project Line Context

GeoTracker is part of a broader line of embedded diagnostic tools:

| Project | Main Focus |
|---|---|
| **MSER v2.0** | Environmental sensing and data monitoring |
| **Signal Scanner** | Wireless, RF, EMF, and electrical signal diagnostics |
| **GeoTracker v1.0** | GPS-based field surveying, mapping, path tracking, and navigation |

GeoTracker adds a location and movement layer to the diagnostic tool family.

---

## Core Features

- Live GPS dashboard
- GPS fix, satellite count, HDOP, altitude, speed, and UTC time display
- Persistent compass/navigation OLED display
- Persistent GPS status OLED display with satellite animation
- TFT home screen with side menu
- Path tracking menu
- Start/stop CSV tracking
- Waypoint marking
- Manual coordinate input using keypad
- Navigate-to-coordinate mode
- Navigate-to-waypoint mode foundation
- Return-to-start mode foundation
- Distance traveled calculation
- GPS speed and altitude display
- Environmental readings using BME280
- Pressure altitude estimate
- Compass heading using BMM150 magnetometer
- Pitch/roll and motion data using MPU-6050
- MicroSD CSV logging
- Event logging
- Device status logging
- Portable assembled handheld form factor
- SD output designed for GeoTracker Studio import and visualization

---

## Final Hardware Scope

| Component | Purpose |
|---|---|
| **ESP32-WROOM-DA Dev Board** | Main microcontroller |
| **NEO-M8N GPS Module** | Latitude, longitude, GPS altitude, speed, UTC time, course, satellite data |
| **2.4 inch SPI TFT Display** | Main UI, home menu, path tracking, GPS, environment, logging, movement, and status screens |
| **0.96 inch I2C OLED Display #1** | Always-on compass/navigation display |
| **0.96 inch I2C OLED Display #2** | Always-on GPS status display with satellite animation |
| **MicroSD Card Module** | CSV logging for trips, waypoints, events, and status |
| **BME280 Sensor** | Temperature, humidity, pressure, and pressure-altitude estimate |
| **BMM150 Magnetometer** | Compass heading and navigation direction support |
| **MPU-6050** | Pitch, roll, acceleration, gyro, and movement/orientation data |
| **NULLLAB I2C Numberpad/Keypad** | Menu navigation, waypoint actions, and latitude/longitude input |
| **Power Circuit** | Portable battery-powered operation |

---

## Display Roles

GeoTracker uses a three-display layout:

| Display | Final Role |
|---|---|
| **2.4 inch SPI TFT** | Main user interface with home screen, side menu, path tracking, environment, logging, movement, and status pages |
| **OLED #1** | Always-on compass/navigation display with live heading animation and degree readout |
| **OLED #2** | Always-on GPS status display with lock state, satellite count, HDOP, UTC time, and satellite-orbit animation |

This layout makes the device feel more like a dedicated field instrument:

- The TFT handles interaction and deeper data views.
- OLED #1 always answers: **What direction am I facing?**
- OLED #2 always answers: **Is GPS healthy?**

---

## Final UI Structure

The main TFT interface is organized around a home screen and side menu.

```text
HOME
├── Path Tracking
│   ├── Live GPS
│   ├── Start / Stop Tracking
│   ├── Navigate To Coordinates
│   ├── Navigate to Waypoint
│   └── Return to Start
│
├── Environment
│   └── BME280 readings
│
├── Logging
│   ├── LOG.CSV
│   ├── WAYPTS.CSV
│   ├── EVENTS.CSV
│   ├── Distance
│   ├── Elevation gain/loss
│   └── Duration
│
├── Movement
│   ├── Pitch / Roll
│   ├── Acceleration magnitude
│   ├── Heading
│   ├── Magnetic field strength
│   └── Gyroscope data
│
└── Status
    ├── TFT status
    ├── OLED status
    ├── GPS serial status
    ├── Sensor status
    ├── SD status
    └── Keypad status
```

---

## Keypad Controls

GeoTracker uses the I2C keypad as the primary control system.

### Normal/Menu Mode

| Key | Action |
|---|---|
| **A** | Next item / next screen |
| **B** | Previous item / back |
| **C** | Enter / select / start-stop tracking depending on screen |
| **D** | Save current waypoint |
| **\*** | Jump to logging screen / decimal point in input mode |
| **#** | Home/menu shortcut / negative sign in input mode |

### Coordinate Input Mode

| Key | Action |
|---|---|
| **0–9** | Number input |
| **\*** | Decimal point |
| **#** | Negative sign |
| **A** | Confirm field / next field |
| **B** | Cancel / back |
| **C** | Confirm / save |
| **D** | Delete / backspace |

Example coordinate entry:

```text
Latitude:   43.826450
Longitude: -111.789120

Keypad input:
LAT: 43*826450
LON: #111*789120
```

---

## Main Operating Modes

### 1. Home

The home screen provides a quick overview:

- GPS fix state
- Satellite count
- Latitude and longitude
- Heading
- Temperature
- Tracking duration
- Distance traveled
- Side menu access

---

### 2. Path Tracking

This is the main mission area of the device.

Path Tracking includes:

- Live GPS
- Start/stop tracking
- Navigate to manual coordinates
- Navigate to saved waypoint ID
- Return to start

When tracking starts, GeoTracker stores the starting GPS coordinate and begins logging the full sensor/GPS state to CSV.

---

### 3. Live GPS

Displays:

- Fix type
- Satellite count
- Latitude
- Longitude
- GPS altitude
- Speed
- HDOP
- UTC time/date through the header/status displays

---

### 4. Waypoint Marker

The user can save the current GPS position as a waypoint.

Saved waypoint data includes:

- Waypoint ID
- Timestamp
- Name/source
- Latitude
- Longitude
- GPS altitude
- Heading
- Temperature
- Humidity
- Pressure
- Notes

---

### 5. Navigation

GeoTracker can guide the user toward a manually entered coordinate or saved waypoint using:

- Distance to target
- Bearing to target
- Current compass heading
- Turn left/right guidance
- Target name/source

This works like a field GPS compass, not road navigation. It gives direction and distance, but it does not know roads, sidewalks, buildings, fences, or obstacles.

---

### 6. Return to Start

When a tracking session begins, GeoTracker saves the starting point. Return-to-start mode uses that stored point as the navigation target.

Displayed values can include:

- Distance to start
- Bearing to start
- Current heading
- Turn left/right guidance
- Arrival state foundation

---

### 7. Environment

The Environment page displays BME280 data:

- Temperature
- Humidity
- Barometric pressure
- Pressure altitude estimate
- BME280 detected I2C address

This makes environmental data map-ready because each reading can be logged together with GPS position.

---

### 8. Movement

The Movement page combines MPU-6050 and BMM150 data:

- Pitch
- Roll
- Acceleration magnitude
- Heading
- Cardinal direction
- Magnetic field strength
- Gyroscope data

The BMM150 provides magnetic heading. The MPU-6050 provides orientation and movement data, but it does not replace the magnetometer because it does not detect magnetic north.

---

### 9. Logging

The Logging page shows file and trip information:

- SD card status
- Tracking state
- LOG.CSV size
- WAYPTS.CSV size
- EVENTS.CSV size
- Logged row count
- Distance traveled
- Elevation gain/loss
- Tracking duration

---

### 10. Device Status

The Status page shows system health:

- TFT
- OLED #1
- OLED #2
- BME280
- MPU6050
- BMM150
- SD
- GPS UART
- Keypad
- GPS lock
- I2C/SPI/UART pin summary
- GPS character count

---

## SD Card Output

GeoTracker writes to the `/GEOTRK` folder on the MicroSD card.

```text
/GEOTRK/LOG.CSV
/GEOTRK/WAYPTS.CSV
/GEOTRK/EVENTS.CSV
/GEOTRK/STATUS.TXT
```

### LOG.CSV

Main tracking log. Stores GPS, environmental, motion, compass, navigation, keypad, and UI state data.

Example categories:

- Row number
- Millis timestamp
- Date/time UTC
- Tracking state
- Elapsed time
- Distance
- Elevation gain/loss
- Target state/source/name
- Target distance/bearing/turn direction
- GPS fix, latitude, longitude, altitude, speed, course, satellites, HDOP
- BME280 temperature, humidity, pressure, pressure altitude
- MPU-6050 pitch, roll, acceleration, gyro
- BMM150 heading and magnetic field strength
- Last key pressed
- Current screen/page

### WAYPTS.CSV

Stores saved waypoints and tracking start points.

Example fields:

```csv
id,millis,date_utc,time_utc,name,source,lat,lon,alt_m,heading_deg,temp_c,humidity_percent,pressure_hpa,note
```

### EVENTS.CSV

Stores device and UI events.

Example events:

- BOOT
- TRACK_START
- TRACK_STOP
- WAYPOINT
- NAV_TARGET
- RETURN_START
- SELECT

### STATUS.TXT

Stores boot-time subsystem status and interface/pin information.

---

## Final Pin Mapping

### TFT Display — SPI

| TFT Pin | ESP32 Pin |
|---|---|
| DIN / MOSI | GPIO23 |
| CLK / SCK | GPIO18 |
| MISO | GPIO19 shared with SD, if available |
| CS | GPIO5 |
| DC | GPIO2 |
| RST | GPIO4 |
| BL | 3.3 V / always on |

### MicroSD — SPI

| SD Pin | ESP32 Pin |
|---|---|
| VCC | 5 V |
| GND | GND |
| SCK | GPIO18 |
| MOSI / DI | GPIO23 |
| MISO / DO | GPIO19 |
| CS | GPIO13 |

The SD module used in this build worked reliably from 5 V input.

### GPS — UART2

| GPS Pin | ESP32 Pin |
|---|---|
| GPS TX | GPIO16 / ESP32 RX2 |
| GPS RX | GPIO17 / ESP32 TX2 |
| Baud | 9600 |

### Main I2C Bus

| Signal | ESP32 Pin |
|---|---|
| SDA | GPIO21 |
| SCL | GPIO22 |

Devices on main I2C:

- OLED #1 at `0x3C`
- BME280 at `0x76` or `0x77`
- MPU-6050 at `0x68` or `0x69`
- BMM150 at `0x13`
- NULLLAB keypad at approximately `0x65`

### Second I2C Bus

| Signal | ESP32 Pin |
|---|---|
| SDA | GPIO32 |
| SCL | GPIO33 |

Device on second I2C:

- OLED #2 at `0x3C`

Using two ESP32 I2C buses allowed both OLEDs to use the same `0x3C` address without needing an I2C multiplexer.

---

## Sensor Roles

### NEO-M8N GPS Module

Used for:

- Latitude
- Longitude
- GPS altitude
- GPS speed
- GPS course while moving
- UTC time/date
- Satellite count
- Fix status
- HDOP

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
- Magnetic field strength reading

The magnetometer is important because GPS course is only reliable while moving. The BMM150 allows the device to estimate direction even when standing still.

---

### MPU-6050

Used for:

- Pitch
- Roll
- Acceleration
- Gyroscope data
- Movement/orientation display
- Future hold-level warnings or motion detection

The MPU-6050 does not detect magnetic north. It works together with the BMM150 to improve the handheld navigation experience.

---

## Navigation Calculations

### Distance to Target

GeoTracker uses a Haversine-style distance calculation.

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
diff > 0       -> turn right
diff < 0       -> turn left
diff near 0    -> go straight
```

---

## GeoTracker Studio Companion Software

GeoTracker Studio is the companion software layer for GeoTracker v1.0. The embedded device collects the field data, and GeoTracker Studio turns that data into readable maps, graphs, summaries, and exports.

```text
GeoTracker v1.0
Handheld ESP32 field-data collector
        ↓
MicroSD CSV files
        ↓
GeoTracker Studio
Python desktop visualization and analysis tool
```

This separation keeps the ESP32 firmware focused on reliable field operation while the desktop software handles heavier visualization, analysis, and export work.

### Companion Role

GeoTracker Studio is designed to import GeoTracker SD card files such as:

```text
LOG.CSV
WAYPTS.CSV
EVENTS.CSV
STATUS.TXT
```

Once imported, the software can reconstruct a session using GPS position, altitude, speed, heading, environmental readings, motion/orientation values, waypoints, and event markers.

### Visualization Levels

GeoTracker Studio follows a layered visualization model:

| Level | View | Purpose |
|---|---|---|
| **Level 1** | CSV/session summary | Quickly verify imported data and trip statistics |
| **Level 2** | Simple 2D route plot | Show the path, start/end points, waypoints, and events |
| **Level 3** | Interactive 2D map | Display route data over a map-style view using Folium/OpenStreetMap |
| **Level 4** | Interactive 3D route | Show route shape using local X/Y meters and altitude as Z |
| **Level 5** | Sensor overlays | Color or filter the path by speed, altitude, temperature, humidity, pressure, heading, GPS quality, or motion data |
| **Level 6** | Export layer | Generate KML/GPX/GeoJSON for Google Earth, Google My Maps, or other mapping tools |
| **Level 7** | Advanced Google mapping | Optional Google Maps API, Google Map Tiles, or Photorealistic 3D Tiles integration |

### Data Relationship

GeoTracker Studio depends on GeoTracker v1.0's core logging idea:

```text
position + time + sensor values + movement/orientation + event context
```

That means every logged field point can later be visualized as a spatial data sample instead of just a row of numbers.

### Current Companion Direction

GeoTracker Studio should be presented as a ready complementary project to GeoTracker v1.0. In documentation, portfolio descriptions, and project explanations, the pair can be described as:

```text
GeoTracker v1.0 is the embedded field-data acquisition device.
GeoTracker Studio is the Python visualization companion that imports and analyzes the captured data.
```

### Expansion Path

Future improvements to GeoTracker Studio can include:

- Session-based imports
- KML export
- GPX export
- GeoJSON export
- Interactive sensor overlays
- Synchronized route/sample selection
- 3D terrain or altitude views
- Google Earth / Google My Maps compatibility
- Optional Google Maps JavaScript API integration
- Optional Google Map Tiles / Photorealistic 3D Tiles advanced viewer

---

## Suggested Software Libraries

Arduino IDE libraries used or planned:

| Function | Library |
|---|---|
| GPS parsing | `TinyGPSPlus` |
| TFT display | `Adafruit_ILI9341` and `Adafruit_GFX` |
| OLED displays | `Adafruit_SSD1306` and `Adafruit_GFX` |
| MicroSD card | `SD` |
| BME280 | `Adafruit_BME280` |
| MPU-6050 | `Adafruit_MPU6050` |
| BMM150 | `DFRobot_BMM150` |
| I2C communication | `Wire` |
| SPI communication | `SPI` |

GeoTracker Studio Python libraries:

| Function | Library |
|---|---|
| CSV/data analysis | `pandas` |
| Simple 2D plots | `matplotlib` |
| Interactive 2D/3D plots | `plotly` |
| Interactive web maps | `folium` |
| KML/GPX export | `simplekml`, `gpxpy`, or custom exporters |
| GUI/dashboard | `PySide6`, `Tkinter`, or `Streamlit` |

---

## Testing and Validation

Completed validation includes:

- Individual TFT test
- Dual OLED test using two I2C buses
- BME280 test
- MPU-6050 test
- BMM150 test using DFRobot library
- NEO-M8N GPS test
- MicroSD read/write test
- I2C keypad test
- Full system integration test
- Final wiring and assembled build
- SD output verification
- Waypoint CSV verification
- Event CSV verification
- Status text verification

Final integration confirmed that all major peripherals can run together on the ESP32-WROOM-DA.

---

## Known Notes and Limitations

- GPS fix may be weak or unavailable indoors.
- Magnetometer accuracy depends on calibration and final mounting location.
- Nearby batteries, wires, speakers, magnets, or metal parts can affect the BMM150.
- GPS altitude can be noisy compared with pressure altitude.
- TFT and SD share the SPI bus, so chip-select handling must remain clean.
- The TFT refresh was improved in the smoother UI build, but future versions could use more advanced dirty-rectangle updates.
- Current navigation gives bearing/distance guidance, not road-aware routing.
- Google Maps / 3D Tiles integration belongs to GeoTracker Studio, not the embedded ESP32 device itself. The ESP32 records location-tagged field data; the companion software handles advanced map rendering.

---

## Future Improvements

Possible GeoTracker v1.1/v2.0 upgrades:

- Session-based SD folders
- Cleaner trip/session naming
- Larger TFT display
- Offline map tile support
- LoRa beacon mode
- Wi-Fi signal strength mapping
- UV exposure mapping
- Light intensity mapping
- Battery percentage estimation
- Recharge/charging status display
- Weather-resistant enclosure refinement
- Vibration motor for silent navigation feedback
- Compass calibration wizard
- Waypoint naming system
- Route summary screen
- More advanced path map on TFT
- Arrival buzzer alerts
- Improved TFT partial refresh / dirty rectangle rendering

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
- Learn embedded navigation, sensor integration, and data logging
- Import captured SD logs into GeoTracker Studio for 2D/3D visualization and analysis


---

## Project Status

**GeoTracker v1.0 is complete.**

The final build is assembled, wired, integrated, and operational. The embedded system successfully combines GPS, environmental sensing, compass heading, motion/orientation data, MicroSD logging, keypad input, and a three-display interface into a single handheld field survey device. GeoTracker Studio should be treated as the ready companion software component that extends the device into a full data visualization and analysis system.

---

## License

This project can be released under the MIT License or another open-source license of choice.

---

## Author

Created by Victor Granado as part of a personal embedded systems diagnostic-tool series.
