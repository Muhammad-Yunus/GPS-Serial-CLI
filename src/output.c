#include "output.h"
#include <stdio.h>

void output_tabular(const gps_t* gps, FILE* fp) {
    fprintf(fp, "GPS Status\n");
    fprintf(fp, "  %-20s %s\n", "Fix:", gps->has_fix ? "VALID" : "NO FIX");
    if (gps->has_fix) {
        fprintf(fp, "  %-20s %.6f\n", "Latitude:", gps->lat);
        fprintf(fp, "  %-20s %.6f\n", "Longitude:", gps->lon);
        fprintf(fp, "  %-20s %.1f m\n", "Altitude:", gps->alt);
    }
    fprintf(fp, "  %-20s %d\n", "Satellites Used:", gps->nsat_used);
    fprintf(fp, "  %-20s %d\n", "Satellites in View:", gps->nsat_view);
    fprintf(fp, "  %-20s %s\n", "Fix Quality:",
            gps->fix_quality == 0 ? "Invalid" :
            gps->fix_quality == 1 ? "GPS" :
            gps->fix_quality == 2 ? "DGPS" : "Unknown");
    fprintf(fp, "  %-20s %s\n", "Fix Mode:",
            gps->fix_mode == 1 ? "No Fix" :
            gps->fix_mode == 2 ? "2D" :
            gps->fix_mode == 3 ? "3D" : "Unknown");
    fprintf(fp, "  %-20s %.1f\n", "HDOP:", gps->hdop);
    fprintf(fp, "  %-20s %.1f\n", "VDOP:", gps->vdop);
    fprintf(fp, "  %-20s %.1f\n", "PDOP:", gps->pdop);
    fprintf(fp, "  %-20s %.1f knots\n", "Speed:", gps->speed_knots);
    fprintf(fp, "  %-20s %.1f km/h\n", "Speed:", gps->speed_kmh);
    fprintf(fp, "  %-20s %.1f°\n", "Course:", gps->course);

    if (gps->year > 0) {
        fprintf(fp, "  %-20s %04d-%02d-%02d %02d:%02d:%02d\n",
                "UTC Time:",
                gps->year, gps->mon, gps->day,
                gps->hour, gps->min, gps->sec);
    }

    if (gps->nsats_tracked > 0) {
        fprintf(fp, "\n  Satellites:\n");
        fprintf(fp, "  %-4s %-10s %-5s %-5s %-5s\n", "PRN", "Elevation", "Azim", "SNR", "Used");
        for (int i = 0; i < gps->nsats_tracked; i++) {
            int used = 0;
            for (int j = 0; j < gps->nsat_used && j < 12; j++) {
                // approximate check — not perfect without GSA PRN matching
            }
            (void)used;
            fprintf(fp, "  %-4d %-10d %-5d %-5d %-5s\n",
                    gps->sats[i].prn,
                    gps->sats[i].elevation,
                    gps->sats[i].azimuth,
                    gps->sats[i].snr,
                    gps->sats[i].snr > 0 ? "yes" : "no");
        }
    }
}

void output_json(const gps_t* gps, FILE* fp) {
    fprintf(fp, "{\n");
    fprintf(fp, "  \"has_fix\": %s,\n", gps->has_fix ? "true" : "false");
    fprintf(fp, "  \"fix_quality\": %d,\n", gps->fix_quality);
    fprintf(fp, "  \"fix_mode\": %d,\n", gps->fix_mode);

    if (gps->has_fix) {
        fprintf(fp, "  \"latitude\": %.6f,\n", gps->lat);
        fprintf(fp, "  \"longitude\": %.6f,\n", gps->lon);
        fprintf(fp, "  \"altitude_m\": %.1f,\n", gps->alt);
    }

    fprintf(fp, "  \"satellites_used\": %d,\n", gps->nsat_used);
    fprintf(fp, "  \"satellites_view\": %d,\n", gps->nsat_view);
    fprintf(fp, "  \"hdop\": %.1f,\n", gps->hdop);
    fprintf(fp, "  \"vdop\": %.1f,\n", gps->vdop);
    fprintf(fp, "  \"pdop\": %.1f,\n", gps->pdop);
    fprintf(fp, "  \"speed_knots\": %.1f,\n", gps->speed_knots);
    fprintf(fp, "  \"speed_kmh\": %.1f,\n", gps->speed_kmh);
    fprintf(fp, "  \"course_deg\": %.1f", gps->course);

    if (gps->year > 0) {
        fprintf(fp, ",\n  \"utc_time\": \"%04d-%02d-%02dT%02d:%02d:%02dZ\"",
                gps->year, gps->mon, gps->day,
                gps->hour, gps->min, gps->sec);
    }

    if (gps->nsats_tracked > 0) {
        fprintf(fp, ",\n  \"satellites\": [\n");
        for (int i = 0; i < gps->nsats_tracked; i++) {
            fprintf(fp, "    {\"prn\":%d,\"elevation\":%d,\"azimuth\":%d,\"snr\":%d}%s\n",
                    gps->sats[i].prn,
                    gps->sats[i].elevation,
                    gps->sats[i].azimuth,
                    gps->sats[i].snr,
                    i < gps->nsats_tracked - 1 ? "," : "");
        }
        fprintf(fp, "  ]\n");
    } else {
        fprintf(fp, "\n");
    }

    fprintf(fp, "}\n");
}
