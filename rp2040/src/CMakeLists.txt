add_executable(${PROJECT_NAME}
    main.c
    bme280.c
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib

    hardware_i2c
    
    bme280_lib
    
    FreeRTOS-Kernel
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)