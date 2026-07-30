#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>

typedef struct serial serial_t;

serial_t* serial_open(const char* device, int baud);
int serial_readline(serial_t* s, char* buf, size_t len, int timeout_ms);
int serial_close(serial_t* s);

#endif
