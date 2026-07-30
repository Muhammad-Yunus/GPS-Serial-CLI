#include "serial.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

struct serial {
    int fd;
};

static int baud_to_rate(int baud) {
    switch (baud) {
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return B9600;
    }
}

serial_t* serial_open(const char* device, int baud) {
    serial_t* s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    s->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (s->fd < 0) {
        perror("open");
        free(s);
        return NULL;
    }

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    cfsetospeed(&tio, baud_to_rate(baud));
    cfsetispeed(&tio, baud_to_rate(baud));

    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 1;

    tcflush(s->fd, TCIFLUSH);
    tcsetattr(s->fd, TCSANOW, &tio);

    int flags = fcntl(s->fd, F_GETFL, 0);
    fcntl(s->fd, F_SETFL, flags & ~O_NONBLOCK);

    return s;
}

int serial_readline(serial_t* s, char* buf, size_t len, int timeout_ms) {
    if (!s || s->fd < 0) return -1;

    struct pollfd pfd = { .fd = s->fd, .events = POLLIN };
    size_t i = 0;

    while (i < len - 1) {
        int ret = poll(&pfd, 1, timeout_ms);
        if (ret < 0) return -1;
        if (ret == 0) break;

        char c;
        int n = read(s->fd, &c, 1);
        if (n <= 0) continue;

        if (c == '\r') continue;
        if (c == '\n') {
            buf[i] = '\0';
            return (int)i;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i > 0 ? (int)i : 0;
}

int serial_close(serial_t* s) {
    if (!s) return 0;
    if (s->fd >= 0) close(s->fd);
    free(s);
    return 0;
}
