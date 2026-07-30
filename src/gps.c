#include "gps.h"
#include <string.h>
#include <stdio.h>

void gps_init(gps_t* gps) {
    memset(gps, 0, sizeof(*gps));
}

void gps_update_gsv(gps_t* gps, const nmea_gsv_t* gsv) {
    if (gsv->nsats > 0) {
        gps->nsats_tracked = gsv->nsats;
        gps->nsat_view = gsv->total_sats;
        for (int i = 0; i < gsv->nsats && i < 32; i++)
            gps->sats[i] = gsv->sats[i];
    }
}

void gps_update(gps_t* gps, const char* sentence) {
    if (!gps || !sentence) return;

    nmea_type_t type = nmea_type(sentence);

    switch (type) {
        case NMEA_GPGGA: {
            nmea_gga_t gga;
            if (nmea_parse_gga(sentence, &gga) == 0) {
                gps->lat = gga.lat;
                gps->lon = gga.lon;
                gps->alt = gga.alt;
                gps->hdop = gga.hdop;
                gps->fix_quality = gga.quality;
                gps->nsat_used = gga.nsat;
                gps->has_fix = (gga.quality > 0);
            }
            break;
        }
        case NMEA_GPRMC: {
            nmea_rmc_t rmc;
            if (nmea_parse_rmc(sentence, &rmc) == 0) {
                if (rmc.valid) {
                    gps->lat = rmc.lat;
                    gps->lon = rmc.lon;
                    gps->speed_knots = rmc.speed;
                    gps->course = rmc.course;
                    gps->year = rmc.year;
                    gps->mon  = rmc.mon;
                    gps->day  = rmc.day;
                    gps->hour = rmc.hour;
                    gps->min  = rmc.min;
                    gps->sec  = rmc.sec;
                }
                gps->has_fix = rmc.valid;
            }
            break;
        }
        case NMEA_GPGSV: {
            nmea_gsv_t gsv;
            if (nmea_parse_gsv(sentence, &gsv) == 0)
                gps_update_gsv(gps, &gsv);
            break;
        }
        case NMEA_GPGSA: {
            nmea_gsa_t gsa;
            if (nmea_parse_gsa(sentence, &gsa) == 0) {
                gps->fix_mode = gsa.fix_mode;
                gps->hdop = gsa.hdop;
                gps->vdop = gsa.vdop;
                gps->pdop = gsa.pdop;
            }
            break;
        }
        case NMEA_GPVTG: {
            nmea_vtg_t vtg;
            if (nmea_parse_vtg(sentence, &vtg) == 0) {
                gps->course = vtg.course_true;
                gps->speed_knots = vtg.speed_knots;
                gps->speed_kmh = vtg.speed_kmh;
            }
            break;
        }
        default:
            break;
    }
}
