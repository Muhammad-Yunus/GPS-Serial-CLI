#include "nmea.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Validate NMEA checksum (after '$', before optional '*xx')
int nmea_checksum(const char* sentence) {
    if (!sentence || sentence[0] != '$') return -1;

    const char* star = strchr(sentence, '*');
    if (!star) return -1;

    unsigned char calc = 0;
    for (const char* p = sentence + 1; *p && *p != '*'; p++)
        calc ^= (unsigned char)*p;

    unsigned int given;
    if (sscanf(star + 1, "%2x", &given) != 1)
        return -1;

    return calc == given ? 0 : -1;
}

nmea_type_t nmea_type(const char* sentence) {
    if (!sentence || sentence[0] != '$') return NMEA_UNKNOWN;

    if (strncmp(sentence, "$GPGGA", 6) == 0) return NMEA_GPGGA;
    if (strncmp(sentence, "$GPRMC", 6) == 0) return NMEA_GPRMC;
    if (strncmp(sentence, "$GPGSV", 6) == 0) return NMEA_GPGSV;
    if (strncmp(sentence, "$GPGSA", 6) == 0) return NMEA_GPGSA;
    if (strncmp(sentence, "$GPVTG", 6) == 0) return NMEA_GPVTG;
    return NMEA_UNKNOWN;
}

// Split NMEA sentence into fields (destroys sentence in-place)
static int split_fields(char* s, const char** fields, int max_fields) {
    int n = 0;
    char* star = strchr(s, '*');
    if (star) *star = '\0';

    fields[n++] = s;
    for (char* p = s; *p && n < max_fields; p++) {
        if (*p == ',') {
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    return n;
}

static double parse_decimal(const char* s) {
    if (!s || !*s) return 0.0;
    return strtod(s, NULL);
}

static int parse_int(const char* s) {
    if (!s || !*s) return 0;
    return atoi(s);
}

// NMEA lat/lon format: [D]DMM.MMMM -> decimal degrees
// Minutes are always 2 digits before decimal, degrees are before that.
static double nmea_to_dd(const char* str, char hemi) {
    if (!str || !*str) return 0.0;
    const char* dot = strchr(str, '.');
    if (!dot || (size_t)(dot - str) < 2) return 0.0;

    size_t minute_len = 2;  // MM before decimal
    const char* min_start = dot - minute_len;
    size_t deg_len = min_start - str;

    char deg_str[16];
    memcpy(deg_str, str, deg_len);
    deg_str[deg_len] = '\0';

    double degrees = strtod(deg_str, NULL);
    double minutes = strtod(min_start, NULL);
    double dd = degrees + minutes / 60.0;
    if (hemi == 'S' || hemi == 'W') dd = -dd;
    return dd;
}

double nmea_lat_to_dd(const char* lat_str, char ns) {
    return nmea_to_dd(lat_str, ns);
}

double nmea_lon_to_dd(const char* lon_str, char ew) {
    return nmea_to_dd(lon_str, ew);
}

int nmea_parse_gga(const char* sentence, nmea_gga_t* out) {
    if (!sentence || !out) return -1;
    memset(out, 0, sizeof(*out));

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sentence);
    const char* fields[20];
    int n = split_fields(buf, fields, 20);
    if (n < 15) return -1;

    out->lat = nmea_lat_to_dd(fields[2], fields[3] ? fields[3][0] : 0);
    out->lon = nmea_lon_to_dd(fields[4], fields[5] ? fields[5][0] : 0);
    out->quality = parse_int(fields[6]);
    out->nsat = parse_int(fields[7]);
    out->hdop = parse_decimal(fields[8]);
    out->alt = parse_decimal(fields[9]);
    out->geoid_sep = parse_decimal(fields[11]);
    return 0;
}

int nmea_parse_rmc(const char* sentence, nmea_rmc_t* out) {
    if (!sentence || !out) return -1;
    memset(out, 0, sizeof(*out));

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sentence);
    const char* fields[15];
    int n = split_fields(buf, fields, 15);
    if (n < 12) return -1;

    out->valid = (fields[2] && fields[2][0] == 'A') ? 1 : 0;
    out->lat = nmea_lat_to_dd(fields[3], fields[4] ? fields[4][0] : 0);
    out->lon = nmea_lon_to_dd(fields[5], fields[6] ? fields[6][0] : 0);
    out->speed = parse_decimal(fields[7]);
    out->course = parse_decimal(fields[8]);

    // Date: ddmmyy
    if (fields[9] && strlen(fields[9]) >= 6) {
        out->day = (fields[9][0] - '0') * 10 + (fields[9][1] - '0');
        out->mon = (fields[9][2] - '0') * 10 + (fields[9][3] - '0');
        out->year = 2000 + (fields[9][4] - '0') * 10 + (fields[9][5] - '0');
    }

    // Time: hhmmss.ss
    if (fields[1] && strlen(fields[1]) >= 6) {
        out->hour = (fields[1][0] - '0') * 10 + (fields[1][1] - '0');
        out->min  = (fields[1][2] - '0') * 10 + (fields[1][3] - '0');
        out->sec  = (fields[1][4] - '0') * 10 + (fields[1][5] - '0');
    }
    return 0;
}

int nmea_parse_gsv(const char* sentence, nmea_gsv_t* out) {
    if (!sentence || !out) return -1;

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sentence);
    const char* fields[20];
    int n = split_fields(buf, fields, 20);
    if (n < 4) return -1;

    int total_msgs = parse_int(fields[1]);
    int msg_no = parse_int(fields[2]);
    out->total_sats = parse_int(fields[3]);

    int sats_in_msg = (n - 4) / 4;
    int start_idx = (msg_no - 1) * 4;

    for (int i = 0; i < sats_in_msg && (start_idx + i) < 32; i++) {
        int base = 4 + i * 4;
        out->sats[start_idx + i].prn = parse_int(fields[base]);
        out->sats[start_idx + i].elevation = parse_int(fields[base + 1]);
        out->sats[start_idx + i].azimuth = parse_int(fields[base + 2]);
        out->sats[start_idx + i].snr = parse_int(fields[base + 3]);
    }

    out->nsats = start_idx + sats_in_msg;
    if (out->nsats > 32) out->nsats = 32;
    return 0;
}

int nmea_parse_gsa(const char* sentence, nmea_gsa_t* out) {
    if (!sentence || !out) return -1;
    memset(out, 0, sizeof(*out));

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sentence);
    const char* fields[20];
    int n = split_fields(buf, fields, 20);
    if (n < 18) return -1;

    out->fix_mode = parse_int(fields[2]);

    out->nprns = 0;
    for (int i = 3; i < 15 && i < n; i++) {
        int prn = parse_int(fields[i]);
        if (prn > 0 && out->nprns < 12)
            out->prns[out->nprns++] = prn;
    }

    out->pdop = parse_decimal(fields[15]);
    out->hdop = parse_decimal(fields[16]);
    out->vdop = parse_decimal(fields[17]);
    return 0;
}

int nmea_parse_vtg(const char* sentence, nmea_vtg_t* out) {
    if (!sentence || !out) return -1;
    memset(out, 0, sizeof(*out));

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sentence);
    const char* fields[12];
    int n = split_fields(buf, fields, 12);
    if (n < 10) return -1;

    out->course_true = parse_decimal(fields[1]);
    out->speed_knots = parse_decimal(fields[5]);
    out->speed_kmh = parse_decimal(fields[7]);
    return 0;
}
