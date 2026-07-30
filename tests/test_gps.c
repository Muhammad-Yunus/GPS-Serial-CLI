#include "gps.h"
#include <stdio.h>
#include <math.h>

static int failures = 0;
static int tests = 0;

#define ASSERT(cond, msg) do { \
    tests++; \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (%s)\n", msg, #cond); \
        failures++; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, eps, msg) do { \
    tests++; \
    if (fabs((a) - (b)) > (eps)) { \
        fprintf(stderr, "FAIL: %s — expected %f, got %f\n", msg, (b), (a)); \
        failures++; \
    } \
} while(0)

static void test_init(void) {
    gps_t gps;
    gps_init(&gps);
    ASSERT(gps.has_fix == 0, "init no fix");
    ASSERT(gps.nsat_used == 0, "init nsat");
    ASSERT(gps.fix_quality == 0, "init quality");
}

static void test_update_gga(void) {
    gps_t gps;
    gps_init(&gps);
    gps_update(&gps, "$GPGGA,092725.00,4717.11399,N,00833.91290,E,1,08,1.01,499.6,M,48.0,M,,*5C");

    ASSERT(gps.has_fix == 1, "GGA fix");
    ASSERT(gps.fix_quality == 1, "GGA quality");
    ASSERT(gps.nsat_used == 8, "GGA nsat");
    ASSERT_NEAR(gps.lat, 47.285233, 0.0001, "GGA lat");
    ASSERT_NEAR(gps.lon, 8.565215, 0.0001, "GGA lon");
    ASSERT_NEAR(gps.alt, 499.6, 0.1, "GGA alt");
    ASSERT_NEAR(gps.hdop, 1.01, 0.01, "GGA hdop");
}

static void test_update_rmc(void) {
    gps_t gps;
    gps_init(&gps);
    gps_update(&gps, "$GPRMC,083559.00,A,4717.11399,N,00833.91290,E,0.004,77.52,091202,,,A*5A");

    ASSERT(gps.has_fix == 1, "RMC fix");
    ASSERT_NEAR(gps.lat, 47.285233, 0.0001, "RMC lat");
    ASSERT_NEAR(gps.lon, 8.565215, 0.0001, "RMC lon");
    ASSERT_NEAR(gps.speed_knots, 0.004, 0.001, "RMC speed");
    ASSERT_NEAR(gps.course, 77.52, 0.01, "RMC course");
    ASSERT(gps.year == 2002, "RMC year");
    ASSERT(gps.mon == 12, "RMC month");
    ASSERT(gps.day == 9, "RMC day");
}

static void test_update_gsv(void) {
    gps_t gps;
    gps_init(&gps);
    gps_update(&gps, "$GPGSV,3,1,10,01,10,200,45,02,20,100,42,03,30,300,38,04,40,150,40*76");
    gps_update(&gps, "$GPGSV,3,2,10,05,50,200,35,06,60,100,30,07,70,300,25,08,80,150,20*7D");

    ASSERT(gps.nsat_view == 10, "GSV total");
    ASSERT(gps.nsats_tracked == 8, "GSV tracked");
    ASSERT(gps.sats[0].prn == 1, "GSV prn0");
    ASSERT(gps.sats[4].prn == 5, "GSV prn4");
}

static void test_update_gsa(void) {
    gps_t gps;
    gps_init(&gps);
    gps_update(&gps, "$GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,1.01,0.89,0.48*07");

    ASSERT(gps.fix_mode == 3, "GSA mode 3D");
    ASSERT_NEAR(gps.pdop, 1.01, 0.01, "GSA pdop");
    ASSERT_NEAR(gps.hdop, 0.89, 0.01, "GSA hdop");
    ASSERT_NEAR(gps.vdop, 0.48, 0.01, "GSA vdop");
}

static void test_update_vtg(void) {
    gps_t gps;
    gps_init(&gps);
    gps_update(&gps, "$GPVTG,77.52,T,,M,0.004,N,0.008,K,A*06");

    ASSERT_NEAR(gps.course, 77.52, 0.01, "VTG course");
    ASSERT_NEAR(gps.speed_knots, 0.004, 0.001, "VTG knots");
    ASSERT_NEAR(gps.speed_kmh, 0.008, 0.001, "VTG kmh");
}

static void test_multiple_updates(void) {
    gps_t gps;
    gps_init(&gps);

    gps_update(&gps, "$GPGGA,092725.00,4717.11399,N,00833.91290,E,1,08,1.01,499.6,M,48.0,M,,*5C");
    gps_update(&gps, "$GPRMC,083559.00,A,4717.11399,N,00833.91290,E,0.004,77.52,091202,,,A*5A");

    ASSERT(gps.has_fix == 1, "multi fix");
    ASSERT_NEAR(gps.alt, 499.6, 0.1, "multi alt from GGA");
    ASSERT_NEAR(gps.speed_knots, 0.004, 0.001, "multi speed from RMC");

    // GSA should override hdop
    gps_update(&gps, "$GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,2.50,1.50,2.00*0F");
    ASSERT_NEAR(gps.hdop, 1.50, 0.01, "multi hdop from GSA");
}

int main(void) {
    test_init();
    test_update_gga();
    test_update_rmc();
    test_update_gsv();
    test_update_gsa();
    test_update_vtg();
    test_multiple_updates();

    printf("tests: %d, failures: %d\n", tests, failures);
    return failures > 0 ? 1 : 0;
}
