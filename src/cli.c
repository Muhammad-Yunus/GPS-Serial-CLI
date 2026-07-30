#include "cli.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

static const char* default_device = "/dev/ttyAMA0";

int cli_parse(cli_t* opts, int argc, char** argv) {
    opts->device = default_device;
    opts->baud = 9600;
    opts->json = 0;
    opts->watch = 0;
    opts->count = 1;
    opts->count_set = 0;
    opts->help = 0;
    opts->version = 0;

    struct option long_opts[] = {
        {"device",  required_argument, 0, 'd'},
        {"baud",    required_argument, 0, 'b'},
        {"json",    no_argument,       0, 'j'},
        {"watch",   no_argument,       0, 'w'},
        {"count",   required_argument, 0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:b:jwc:hv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': opts->device = optarg; break;
            case 'b': opts->baud = atoi(optarg); break;
            case 'j': opts->json = 1; break;
            case 'w': opts->watch = 1; break;
            case 'c': opts->count = atoi(optarg); opts->count_set = 1; break;
            case 'h': opts->help = 1; return 0;
            case 'v': opts->version = 1; return 0;
            default: return -1;
        }
    }
    return 0;
}

void cli_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "\nRead and decode GPS data from a serial UART.\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -d, --device DEVICE  Serial device (default: %s)\n", default_device);
    fprintf(stderr, "  -b, --baud BAUD      Baud rate (default: 9600)\n");
    fprintf(stderr, "  -j, --json           Output in JSON format\n");
    fprintf(stderr, "  -w, --watch          Continuously watch and print updates\n");
    fprintf(stderr, "  -c, --count N        Exit after N reads (default: 1)\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
    fprintf(stderr, "  -v, --version        Show version\n");
}
