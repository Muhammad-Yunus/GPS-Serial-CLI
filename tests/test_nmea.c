#include "nmea.h"
#include <stdio.h>
#include <string.h>
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

static void test_checksum_valid(void) {
    const char* s = "$GPGGA,092725.00,4717.11399,N,00833.91290,E,1,08,1.01,499.6,M,48.0,M,,*5C";
    ASSERT(nmea_checksum(s) == 0, "valid checksum");
}

static void test_checksum_invalid(void) {
    const char* s = "$GPGGA,092725.00,4717.11399,N,00833.91290,E,1,08,1.01,499.6,M,48.0,M,,*00";
    ASSERT(nmea_checksum(s) != 0, "invalid checksum");
}

static void test_checksum_no_star(void) {
    const char* s = "$GPGGA,092725.00,4717.11399,N";
    ASSERT(nmea_checksum(s) != 0, "no checksum");
}

static void test_type_detection(void) {
    ASSERT(nmea_type("$GPGGA,...") == NMEA_GPGGA, "GPGGA");
    ASSERT(nmea_type("$GPRMC,...") == NMEA_GPRMC, "GPRMC");
    ASSERT(nmea_type("$GPGSV,...") == NMEA_GPGSV, "GPGSV");
    ASSERT(nmea_type("$GPGSA,...") == NMEA_GPGSA, "GPGSA");
    ASSERT(nmea_type("$GPVTG,...") == NMEA_GPVTG, "GPVTG");
    ASSERT(nmea_type("$GPXXX,...") == NMEA_UNKNOWN, "unknown");
    ASSERT(nmea_type("hello") == NMEA_UNKNOWN, "garbage");
}

static void test_parse_gga(void) {
    const char* s = "$GPGGA,092725.00,4717.11399,N,00833.91290,E,1,08,1.01,499.6,M,48.0,M,,*5C";
    nmea_gga_t gga;
    int ret = nmea_parse_gga(s, &gga);
    ASSERT(ret == 0, "parse GGA success");

    ASSERT_NEAR(gga.lat, 47.285233, 0.0001, "GGA latitude");
    ASSERT_NEAR(gga.lon, 8.565215, 0.0001, "GGA longitude");
    ASSERT(gga.quality == 1, "GGA quality");
    ASSERT(gga.nsat == 8, "GGA nsat");
    ASSERT_NEAR(gga.hdop, 1.01, 0.01, "GGA hdop");
    ASSERT_NEAR(gga.alt, 499.6, 0.1, "GGA altitude");
    ASSERT_NEAR(gga.geoid_sep, 48.0, 0.1, "GGA geoid sep");
}

static void test_parse_gga_no_fix(void) {
    const char* s = "$GPGGA,092725.00,,,,,0,00,99.99,,,,,,*48";
    nmea_gga_t gga;
    int ret = nmea_parse_gga(s, &gga);
    ASSERT(ret == 0, "parse GGA no-fix success");
    ASSERT(gga.quality == 0, "GGA quality 0");
    ASSERT_NEAR(gga.lat, 0.0, 0.001, "GGA lat 0");
}

static void test_parse_rmc(void) {
    const char* s = "$GPRMC,083559.00,A,4717.11399,N,00833.91290,E,0.004,77.52,091202,,,A*5A";
    nmea_rmc_t rmc;
    int ret = nmea_parse_rmc(s, &rmc);
    ASSERT(ret == 0, "parse RMC success");
    ASSERT(rmc.valid == 1, "RMC valid");
    ASSERT_NEAR(rmc.lat, 47.285233, 0.0001, "RMC lat");
    ASSERT_NEAR(rmc.lon, 8.565215, 0.0001, "RMC lon");
    ASSERT_NEAR(rmc.speed, 0.004, 0.001, "RMC speed");
    ASSERT_NEAR(rmc.course, 77.52, 0.01, "RMC course");
    ASSERT(rmc.year == 2002, "RMC year");
    ASSERT(rmc.mon == 12, "RMC month");
    ASSERT(rmc.day == 9, "RMC day");
    ASSERT(rmc.hour == 8, "RMC hour");
    ASSERT(rmc.min == 35, "RMC min");
    ASSERT(rmc.sec == 59, "RMC sec");
}

static void test_parse_gsv(void) {
    const char* s = "$GPGSV,3,1,10,01,10,200,45,02,20,100,42,03,30,300,38,04,40,150,40*76";
    nmea_gsv_t gsv;
    int ret = nmea_parse_gsv(s, &gsv);
    ASSERT(ret == 0, "parse GSV success");
    ASSERT(gsv.total_sats == 10, "GSV total sats");
    ASSERT(gsv.nsats == 4, "GSV sats in msg");
    ASSERT(gsv.sats[0].prn == 1, "GSV prn0");
    ASSERT(gsv.sats[0].snr == 45, "GSV snr0");
    ASSERT(gsv.sats[3].prn == 4, "GSV prn3");
}

static void test_parse_gsa(void) {
    const char* s = "$GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,1.01,0.89,0.48*07";
    nmea_gsa_t gsa;
    int ret = nmea_parse_gsa(s, &gsa);
    ASSERT(ret == 0, "parse GSA success");
    ASSERT(gsa.fix_mode == 3, "GSA fix mode 3");
    ASSERT(gsa.nprns == 8, "GSA nprns");
    ASSERT(gsa.prns[0] == 1, "GSA prn0");
    ASSERT_NEAR(gsa.pdop, 1.01, 0.01, "GSA pdop");
    ASSERT_NEAR(gsa.hdop, 0.89, 0.01, "GSA hdop");
    ASSERT_NEAR(gsa.vdop, 0.48, 0.01, "GSA vdop");
}

static void test_parse_vtg(void) {
    const char* s = "$GPVTG,77.52,T,,M,0.004,N,0.008,K,A*06";
    nmea_vtg_t vtg;
    int ret = nmea_parse_vtg(s, &vtg);
    ASSERT(ret == 0, "parse VTG success");
    ASSERT_NEAR(vtg.course_true, 77.52, 0.01, "VTG course");
    ASSERT_NEAR(vtg.speed_knots, 0.004, 0.001, "VTG knots");
    ASSERT_NEAR(vtg.speed_kmh, 0.008, 0.001, "VTG kmh");
}

static void test_lat_parse(void) {
    double lat = nmea_lat_to_dd("4717.11399", 'N');
    ASSERT_NEAR(lat, 47.285233, 0.0001, "lat N");
    ASSERT(nmea_lat_to_dd("", 'N') == 0.0, "lat empty");
    ASSERT(nmea_lat_to_dd("1234", 'N') == 0.0, "lat no dot");
    lat = nmea_lat_to_dd("4717.11399", 'S');
    ASSERT_NEAR(lat, -47.285233, 0.0001, "lat S");
}

static void test_lon_parse(void) {
    double lon = nmea_lon_to_dd("00833.91290", 'E');
    ASSERT_NEAR(lon, 8.565215, 0.0001, "lon E");
    lon = nmea_lon_to_dd("00833.91290", 'W');
    ASSERT_NEAR(lon, -8.565215, 0.0001, "lon W");
}

static void test_empty_sentence(void) {
    nmea_gga_t gga;
    ASSERT(nmea_parse_gga("", &gga) != 0, "empty GGA");
    ASSERT(nmea_type("") == NMEA_UNKNOWN, "empty type");
    ASSERT(nmea_checksum("") != 0, "empty checksum");
}

int main(void) {
    test_checksum_valid();
    test_checksum_invalid();
    test_checksum_no_star();
    test_type_detection();
    test_parse_gga();
    test_parse_gga_no_fix();
    test_parse_rmc();
    test_parse_gsv();
    test_parse_gsa();
    test_parse_vtg();
    test_lat_parse();
    test_lon_parse();
    test_empty_sentence();

    printf("tests: %d, failures: %d\n", tests, failures);
    return failures > 0 ? 1 : 0;
}
