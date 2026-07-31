cmake_minimum_required(VERSION 3.16)
project(gps VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gps
    src/main.c
    src/serial.c
    src/nmea.c
    src/gps.c
    src/output.c
    src/cli.c
)

# Install target: run `make install` or `cmake --install build`
install(TARGETS gps DESTINATION bin)

add_subdirectory(tests)
