#ifndef CLI_H
#define CLI_H

typedef struct {
    const char* device;
    int baud;
    int json;
    int watch;
    int count;
    int count_set;  // 1 if -c was explicitly provided
    int help;
    int version;
} cli_t;

int cli_parse(cli_t* opts, int argc, char** argv);
void cli_usage(const char* prog);

#endif
