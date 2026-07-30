# GPS — NMEA GPS Reader for Raspberry Pi

![C](https://img.shields.io/badge/language-C-blue)
![CMake](https://img.shields.io/badge/build-CMake-green)
![tests](https://img.shields.io/badge/tests-92%20unit%20%7C%2021%20e2e-brightgreen)
![license](https://img.shields.io/badge/license-MIT-lightgrey)

CLI tool to read and decode GPS data from a UBLOX module (NMEA protocol) via UART serial on Raspberry Pi. Output in **tabular** or **JSON** format.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         main.c                              │
│                  loop: read → parse → output                │
├─────────┬──────────┬───────────┬────────────┬───────────────┤
│ cli.c   │ serial.c │ nmea.c    │ gps.c      │ output.c      │
│ arg     │ UART     │ NMEA      │ aggregated  │ formatter     │
│ parsing │ termios  │ sentence  │ GPS state  │ tabular/JSON  │
└─────────┴──────────┴───────────┴────────────┴───────────────┘
```

**Layers:**

| Layer | File | Responsibility |
|-------|------|----------------|
| **CLI** | `cli.c/h` | Parse arguments (`-d`, `-b`, `-j`, `-w`, `-c`) |
| **Serial** | `serial.c/h` | Open/configure UART via termios, read lines |
| **NMEA** | `nmea.c/h` | Parse 5 sentence types: GGA, RMC, GSV, GSA, VTG — checksum validation |
| **GPS** | `gps.c/h` | Aggregate state from multiple NMEA sentences |
| **Output** | `output.c/h` | Format output as tabular or JSON |
| **Main** | `main.c` | Main loop: read → checksum → update → output |

## Project Structure

```
GPS/
├── CMakeLists.txt           # Build system
├── src/
│   ├── main.c               # Entry point
│   ├── cli.c / cli.h        # CLI argument parser
│   ├── serial.c / serial.h  # UART serial interface
│   ├── nmea.c / nmea.h      # NMEA sentence parser
│   ├── gps.c / gps.h        # GPS data model
│   └── output.c / output.h  # Output formatters
├── tests/
│   ├── CMakeLists.txt
│   ├── test_nmea.c          # 59 unit tests for NMEA parser
│   └── test_gps.c           # 33 unit tests for GPS model
├── e2e/
│   └── e2e_test.py          # 21 end-to-end tests via PTY
├── test_data/
│   └── sample_nmea.txt      # Sample NMEA sentences
├── check_gps.py             # Quick GPS validation script
└── README.md
```

## Supported NMEA Sentences

| Sentence | Data |
|----------|------|
| `$GPGGA` | Position, altitude, HDOP, fix quality |
| `$GPRMC` | Date, time, speed, course |
| `$GPGSV` | Satellites in view (PRN, elevation, azimuth, SNR) |
| `$GPGSA` | Fix mode (2D/3D), PDOP, HDOP, VDOP |
| `$GPVTG` | Course over ground, speed (knots & km/h) |

## Build

```bash
# Prerequisites (Raspberry Pi OS)
sudo apt install cmake gcc socat python3-serial

# Build
cmake -S . -B build
cmake --build build
```

## Usage

```bash
./build/gps [options]
```

### Basic

```bash
# Single read, tabular output (default)
./build/gps

# Single read, JSON output
./build/gps -j

# Custom device and baud rate
./build/gps -d /dev/ttyAMA0 -b 9600
```

### Watch Mode

```bash
# Continuous monitoring
./build/gps -w

# Watch 5 sentences then exit
./build/gps -w -c 5

# Watch with JSON output
./build/gps -w -j
```

### Options

| Flag | Long | Description | Default |
|------|------|-------------|---------|
| `-d` | `--device` | Serial device path | `/dev/ttyAMA0` |
| `-b` | `--baud` | Baud rate | `9600` |
| `-j` | `--json` | JSON output | tabular |
| `-w` | `--watch` | Continuous monitoring | off |
| `-c` | `--count` | Exit after N reads | `1` |
| `-h` | `--help` | Show help | |
| `-v` | `--version` | Show version | |

## Output Examples

### Tabular

```
GPS Status
  Fix:                 VALID
  Latitude:            -6.175392
  Longitude:           106.827153
  Altitude:            45.6 m
  Satellites Used:     10
  Satellites in View:  14
  Fix Quality:         GPS
  Fix Mode:            3D
  HDOP:                0.9
  VDOP:                1.2
  PDOP:                1.5
  Speed:               12.3 knots
  Speed:               22.8 km/h
  Course:              180.5°
  UTC Time:            2024-01-15 07:30:45

  Satellites:
  PRN  Elevation  Azim  SNR   Used
  2    45         180   42    yes
  5    30         90    38    yes
  ...
```

### JSON

```json
{
  "has_fix": true,
  "fix_quality": 1,
  "fix_mode": 3,
  "latitude": -6.175392,
  "longitude": 106.827153,
  "altitude_m": 45.6,
  "satellites_used": 10,
  "satellites_view": 14,
  "hdop": 0.9,
  "vdop": 1.2,
  "pdop": 1.5,
  "speed_knots": 12.3,
  "speed_kmh": 22.8,
  "course_deg": 180.5,
  "utc_time": "2024-01-15T07:30:45Z",
  "satellites": [
    {"prn": 2, "elevation": 45, "azimuth": 180, "snr": 42}
  ]
}
```

## Running on Raspberry Pi

### 1. Enable UART on GPIO 14/15 (RPi5)

Edit `/boot/firmware/config.txt` and add:

```ini
dtoverlay=uart0-pi5
```

Then reboot:

```bash
sudo reboot
```

### 2. Verify device

```bash
python3 check_gps.py
```

### 3. Run

```bash
./build/gps
```

> **Note:** When the GPS module has not locked onto satellites (blue LED not blinking), the tool will display "NO FIX" and position data will not be available.

## Tests

### Unit Tests

```bash
cmake --build build
ctest --test-dir build -V
```

Or run directly:

```bash
./build/tests/test_nmea
./build/tests/test_gps
```

### E2E Tests

Simulates a GPS device via a virtual PTY:

```bash
python3 e2e/e2e_test.py
```

Output:

```
E2E: Testing --help...
  PASS: help exit 0
  PASS: help has usage
E2E: Testing --version...
  PASS: version exit 0
  PASS: version string
E2E: Testing tabular output...
  PASS: exit code 0
  PASS: has VALID fix
  ...
E2E: 21 passed, 0 failed
```

## License

MIT
