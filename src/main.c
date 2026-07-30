#include "cli.h"
#include "serial.h"
#include "nmea.h"
#include "gps.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_BUF 256

static const char* version = "1.0.0";

int main(int argc, char** argv) {
    cli_t opts;
    if (cli_parse(&opts, argc, argv) != 0) {
        cli_usage(argv[0]);
        return 1;
    }

    if (opts.help) {
        cli_usage(argv[0]);
        return 0;
    }

    if (opts.version) {
        printf("gps version %s\n", version);
        return 0;
    }

    serial_t* ser = serial_open(opts.device, opts.baud);
    if (!ser) {
        fprintf(stderr, "error: cannot open %s @ %d baud\n", opts.device, opts.baud);
        return 1;
    }

    gps_t gps;
    gps_init(&gps);

    char buf[LINE_BUF];
    int reads = 0;
    int exit_after = opts.count_set ? opts.count : (opts.watch ? 0 : 1);
    // 0 = infinite, only in watch mode without explicit -c

    while (1) {
        int n = serial_readline(ser, buf, sizeof(buf), opts.watch ? 1000 : 5000);
        if (n < 0) {
            perror("read error");
            break;
        }
        if (n == 0) {
            if (!opts.watch) {
                if (!gps.has_fix)
                    printf("No GPS data received. Check connection and satellite lock.\n");
                break;
            }
            continue;
        }

        if (nmea_checksum(buf) != 0)
            continue;

        gps_update(&gps, buf);
        reads++;

        if (opts.json)
            output_json(&gps, stdout);
        else
            output_tabular(&gps, stdout);

        fflush(stdout);

        if (exit_after > 0 && reads >= exit_after)
            break;
    }

    serial_close(ser);
    return 0;
}
