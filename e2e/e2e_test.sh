#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
GPS_BIN="${BUILD_DIR}/gps"
TEST_DATA="${PROJECT_DIR}/test_data/sample_nmea.txt"

if [ ! -f "$GPS_BIN" ]; then
    echo "E2E: Building project first..."
    mkdir -p "$BUILD_DIR"
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" >/dev/null
    cmake --build "$BUILD_DIR" >/dev/null
fi

cleanup() {
    kill "${SOCAT_PID:-}" 2>/dev/null || true
    rm -f "${PTY_SLAVE:-}" 2>/dev/null || true
}
trap cleanup EXIT

PTY_SLAVE=$(mktemp -u /tmp/gps_e2e_XXXX)

echo "E2E: Testing tabular output..."

socat PTY,link="$PTY_SLAVE",raw,echo=0 EXEC:"cat $TEST_DATA",nofork &
SOCAT_PID=$!
sleep 0.5

OUTPUT=$("$GPS_BIN" -d "$PTY_SLAVE" -b 9600 2>&1 || true)

if echo "$OUTPUT" | grep -q "NO FIX"; then
    echo "  FAIL: GPS shows NO FIX"
    echo "$OUTPUT"
    exit 1
fi

if echo "$OUTPUT" | grep -q "VALID"; then
    echo "  PASS: GPS shows VALID fix"
else
    echo "  FAIL: GPS doesn't show VALID fix"
    echo "$OUTPUT"
    exit 1
fi

if echo "$OUTPUT" | grep -q "Latitude:"; then
    echo "  PASS: Tabular contains latitude"
else
    echo "  FAIL: Tabular missing latitude"
    echo "$OUTPUT"
    exit 1
fi

cleanup
sleep 0.3

echo "E2E: Testing JSON output..."

socat PTY,link="$PTY_SLAVE",raw,echo=0 EXEC:"cat $TEST_DATA",nofork &
SOCAT_PID=$!
sleep 0.5

JSON_OUTPUT=$("$GPS_BIN" -d "$PTY_SLAVE" -b 9600 -j 2>&1 || true)

if echo "$JSON_OUTPUT" | grep -q '"has_fix": true'; then
    echo "  PASS: JSON has_fix true"
else
    echo "  FAIL: JSON missing has_fix"
    echo "$JSON_OUTPUT"
    exit 1
fi

if echo "$JSON_OUTPUT" | grep -q '"latitude"'; then
    echo "  PASS: JSON contains latitude"
else
    echo "  FAIL: JSON missing latitude"
    echo "$JSON_OUTPUT"
    exit 1
fi

if echo "$JSON_OUTPUT" | python3 -c "import sys,json; json.load(sys.stdin)" 2>/dev/null; then
    echo "  PASS: JSON is valid"
else
    echo "  FAIL: Invalid JSON"
    echo "$JSON_OUTPUT"
    exit 1
fi

echo ""
echo "E2E: All tests passed!"
exit 0
