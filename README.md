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

### Tabular (Valid Fix)

```
GPS Status
  Fix:                 VALID
  Latitude:            -6.150531
  Longitude:           106.896957
  Altitude:            52.6 m
  Satellites Used:     9
  Satellites in View:  13
  Fix Quality:         GPS
  Fix Mode:            3D
  HDOP:                0.9
  VDOP:                1.7
  PDOP:                1.9
  Speed:               0.1 knots
  Speed:               0.2 km/h
  Course:              0.0°
  UTC Time:            2026-07-31 08:54:26

  Satellites:
  PRN  Elevation  Azim  SNR   Used
  23   62          90   36    yes
  25   35          49   32    yes
  26   44         209    6    yes
  28   41         323   20    yes
```

### Tabular (No Fix Yet — Satellite Locking)

```
GPS Status
  Fix:                 NO FIX
  Latitude:            ---
  Longitude:           ---
  Altitude:            0.0 m
  Satellites Used:     0
  Satellites in View:  13
  Fix Quality:         Invalid
  Fix Mode:            Unknown
  HDOP:                0.0
  VDOP:                0.0
  PDOP:                0.0
  Speed:               0.0 knots
  Speed:               0.0 km/h
  Course:              0.0°

  Satellites:
  PRN  Elevation  Azim  SNR   Used
  10   55         12    17    yes
  12   1          42     8    yes
  16   16         210    0    no
  18   30         163   17    yes
```

### JSON (Single Read)

```json
{
  "has_fix": true,
  "fix_quality": 1,
  "fix_mode": 3,
  "latitude": -6.150531,
  "longitude": 106.896957,
  "altitude_m": 52.6,
  "satellites_used": 9,
  "satellites_view": 13,
  "hdop": 0.9,
  "vdop": 1.7,
  "pdop": 1.9,
  "speed_knots": 0.1,
  "speed_kmh": 0.2,
  "course_deg": 0.0,
  "utc_time": "2026-07-31T08:54:26Z",
  "satellites": [
    {"prn": 28, "elevation": 41, "azimuth": 323, "snr": 20},
    {"prn": 29, "elevation": 16, "azimuth": 120, "snr": 26},
    {"prn": 31, "elevation": 40, "azimuth": 281, "snr": 30},
    {"prn": 32, "elevation": 14, "azimuth": 353, "snr": 23}
  ]
}
```

### JSON Watch Mode (Multiple Objects, One Per Read)

```
{ "has_fix":true,"fix_quality":0,... }   /* first GPRMC, no fix yet */
{ "has_fix":true,"fix_quality":1,... }   /* first GPGGA, valid fix */
{ "has_fix":true,"fix_quality":1,... }   /* subsequent updates */
```

*(Each line is a separate JSON object; use `jq` or streaming parser to handle)*

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
