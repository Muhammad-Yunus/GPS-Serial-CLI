#ifndef NMEA_H
#define NMEA_H

#include <stddef.h>

typedef enum {
    NMEA_UNKNOWN,
    NMEA_GPGGA,
    NMEA_GPRMC,
    NMEA_GPGSV,
    NMEA_GPGSA,
    NMEA_GPVTG,
} nmea_type_t;

typedef struct {
    double lat;         // decimal degrees
    double lon;         // decimal degrees
    int quality;        // 0=invalid, 1=GPS, 2=DGPS
    int nsat;           // number of satellites used
    double hdop;        // horizontal dilution of precision
    double alt;         // altitude in meters
    double geoid_sep;   // geoid separation in meters
} nmea_gga_t;

typedef struct {
    double lat;         // decimal degrees
    double lon;         // decimal degrees
    double speed;       // knots
    double course;      // degrees true
    int day, mon, year;
    int hour, min, sec;
    int valid;          // 1 if data valid
} nmea_rmc_t;

typedef struct {
    int prn;
    int elevation;
    int azimuth;
    int snr;
} nmea_sat_t;

typedef struct {
    int nsats;
    nmea_sat_t sats[32];
    int total_sats;     // total in view
} nmea_gsv_t;

typedef struct {
    int fix_mode;       // 1=no fix, 2=2D, 3=3D
    int prns[12];
    int nprns;
    double pdop, hdop, vdop;
} nmea_gsa_t;

typedef struct {
    double course_true;
    double speed_knots;
    double speed_kmh;
} nmea_vtg_t;

int nmea_checksum(const char* sentence);
nmea_type_t nmea_type(const char* sentence);

int nmea_parse_gga(const char* sentence, nmea_gga_t* out);
int nmea_parse_rmc(const char* sentence, nmea_rmc_t* out);
int nmea_parse_gsv(const char* sentence, nmea_gsv_t* out);
int nmea_parse_gsa(const char* sentence, nmea_gsa_t* out);
int nmea_parse_vtg(const char* sentence, nmea_vtg_t* out);

// Convert NMEA lat/lon format (DDDMM.MMMM) to decimal degrees
double nmea_lat_to_dd(const char* lat_str, char ns);
double nmea_lon_to_dd(const char* lon_str, char ew);

#endif
