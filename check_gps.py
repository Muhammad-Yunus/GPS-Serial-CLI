#!/usr/bin/env python3
"""Quick GPS module detection - coba semua kemungkinan UART device."""
import serial, sys, time, os, errno

CANDIDATES = [
    "/dev/ttyAMA0", "/dev/ttyAMA1", "/dev/ttyAMA2",
    "/dev/ttyS0", "/dev/ttyUSB0", "/dev/ttyUSB1",
]
BAUD = 9600  # UBLOX 6M default

def try_device(path):
    try:
        fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)
        os.close(fd)
    except OSError as e:
        return ("not_found" if e.errno == errno.ENOENT else
                "no_permission" if e.errno == errno.EACCES else f"err:{e.errno}")

    try:
        s = serial.Serial(path, BAUD, timeout=2)
        s.reset_input_buffer()
        time.sleep(0.5)
        raw = s.read(200)
        s.close()
        if raw:
            printable = "".join(chr(b) if 32 <= b < 127 else "." for b in raw)
            return f"OK data={len(raw)}B sample={printable[:80]}"
        return "OK no_data"
    except serial.SerialException as e:
        return f"open_fail:{e}"

print(f"Scanning GPS on {', '.join(CANDIDATES)} @ {BAUD} baud...\n")
for dev in CANDIDATES:
    result = try_device(dev)
    print(f"  {dev:20s} -> {result}")

print("\nDone. If 'OK no_data', module might need power cycle.")
print("If 'OK data=...', GPS is talking (NMEA sentences visible).")
