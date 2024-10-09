add_library(bme280_lib STATIC
    bme280_lib.c bme280_lib.h
)
target_include_directories(bme280_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")