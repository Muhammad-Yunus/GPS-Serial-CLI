enable_testing()

function(add_gps_test name src)
    add_executable(${name} ${src}
        ${CMAKE_SOURCE_DIR}/src/nmea.c
        ${CMAKE_SOURCE_DIR}/src/gps.c
    )
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_gps_test(test_nmea test_nmea.c)
add_gps_test(test_gps  test_gps.c)
