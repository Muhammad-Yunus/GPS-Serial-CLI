#ifndef GPS_H
#define GPS_H

#include "nmea.h"

typedef struct {
    int has_fix;          // 1 if valid position available
    double lat, lon;      // decimal degrees
    double alt;           // altitude (meters)
    double speed_knots;   // speed over ground (knots)
    double speed_kmh;     // speed over ground (km/h)
    double course;        // course over ground (degrees)
    double hdop;          // horizontal dilution of precision
    double vdop;          // vertical dilution of precision
    double pdop;          // position dilution of precision

    int fix_quality;      // 0=invalid, 1=GPS, 2=DGPS
    int fix_mode;         // 1=none, 2=2D, 3=3D
    int nsat_used;        // satellites used in fix
    int nsat_view;        // satellites in view
    nmea_sat_t sats[32];  // satellite details
    int nsats_tracked;    // number of tracked satellites

    int year, mon, day;
    int hour, min, sec;
} gps_t;

void gps_init(gps_t* gps);
void gps_update(gps_t* gps, const char* sentence);
void gps_update_gsv(gps_t* gps, const nmea_gsv_t* gsv);

#endif
