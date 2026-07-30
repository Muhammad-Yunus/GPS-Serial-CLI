#!/usr/bin/env python3
"""End-to-end test: simulate GPS device via PTY, run gps tool, validate output."""
import os, pty, sys, time, json, subprocess

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GPS_BIN = os.path.join(PROJECT_DIR, "build", "gps")
TEST_DATA = os.path.join(PROJECT_DIR, "test_data", "sample_nmea.txt")

passed = 0
failed = 0

def check(name, cond, detail=""):
    global passed, failed
    if cond:
        print(f"  PASS: {name}")
        passed += 1
    else:
        print(f"  FAIL: {name} {detail}")
        failed += 1

def run_gps_pty(args=None, feed_delay=0.3):
    master, slave = pty.openpty()
    slave_name = os.ttyname(slave)

    cmd = [GPS_BIN, "-d", slave_name, "-b", "9600"]
    if args:
        cmd += args

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           pass_fds=(slave,))
    time.sleep(feed_delay)

    with open(TEST_DATA, "r") as f:
        os.write(master, f.read().encode())

    try:
        stdout, stderr = proc.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, stderr = proc.communicate()
    finally:
        os.close(master)
        os.close(slave)

    return stdout.decode(), stderr.decode(), proc.returncode

def test_tabular():
    print("\nE2E: Testing tabular output...")
    stdout, stderr, rc = run_gps_pty()
    check("exit code 0", rc == 0, f"got {rc}")
    check("has VALID fix", "VALID" in stdout, stdout[:100])
    check("has latitude", "Latitude:" in stdout)
    check("has longitude", "Longitude:" in stdout)
    check("has altitude", "Altitude:" in stdout)
    check("has satellites", "Satellites" in stdout)

def test_json():
    print("\nE2E: Testing JSON output...")
    stdout, stderr, rc = run_gps_pty(["-j", "-c", "9"])
    check("exit code 0", rc == 0, f"got {rc}")
    check("has_fix true", '"has_fix": true' in stdout, stdout[:200])

    try:
        objects = []
        decoder = json.JSONDecoder()
        idx = 0
        while idx < len(stdout):
            obj, pos = decoder.raw_decode(stdout, idx)
            objects.append(obj)
            idx = pos + 1
            while idx < len(stdout) and stdout[idx] in ' \n\r\t':
                idx += 1
        check("valid JSON objects", len(objects) > 0)
        last = objects[-1]
        check("JSON has_fix", last.get("has_fix") == True)
        check("JSON latitude", "latitude" in last)
        check("JSON longitude", "longitude" in last)
        check("JSON satellites", "satellites" in last)
        check("JSON sat count > 0", len(last.get("satellites", [])) > 0)
    except (json.JSONDecodeError, ValueError) as e:
        check("valid JSON", False, str(e))

def test_watch():
    print("\nE2E: Testing watch mode (--watch -c 3)...")
    master, slave = pty.openpty()
    slave_name = os.ttyname(slave)

    cmd = [GPS_BIN, "-d", slave_name, "-b", "9600", "-w", "-c", "3"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           pass_fds=(slave,))
    time.sleep(0.5)

    with open(TEST_DATA, "r") as f:
        for line in f:
            os.write(master, line.encode())
            time.sleep(0.05)

    try:
        stdout, stderr = proc.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, stderr = proc.communicate()
    finally:
        os.close(master)
        os.close(slave)

    stdout = stdout.decode()
    rc = proc.returncode
    check("watch exit 0", rc == 0, f"got {rc}")
    if stdout:
        check("watch has fix", "VALID" in stdout)
        check("watch has HDOP", "HDOP:" in stdout)

def test_help():
    print("\nE2E: Testing --help...")
    result = subprocess.run([GPS_BIN, "--help"], capture_output=True, text=True, timeout=5)
    check("help exit 0", result.returncode == 0)
    check("help has usage", "Usage:" in (result.stdout + result.stderr))

def test_version():
    print("\nE2E: Testing --version...")
    result = subprocess.run([GPS_BIN, "--version"], capture_output=True, text=True, timeout=5)
    check("version exit 0", result.returncode == 0)
    check("version string", "version" in (result.stdout + result.stderr))

if __name__ == "__main__":
    if not os.path.exists(GPS_BIN):
        print("Building project first...")
        subprocess.run(["cmake", "-S", PROJECT_DIR,
                       "-B", os.path.join(PROJECT_DIR, "build")], check=True)
        subprocess.run(["cmake", "--build", os.path.join(PROJECT_DIR, "build")], check=True)

    test_help()
    test_version()
    test_tabular()
    test_json()
    test_watch()

    print(f"\nE2E: {passed} passed, {failed} failed")
    sys.exit(1 if failed > 0 else 0)
