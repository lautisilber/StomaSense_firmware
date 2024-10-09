#ifndef BME280_H
#define BME280_H

#include <string.h>
#include "hardware/i2c.h"
#include "bme280_lib.h"
#include "utils.h"

#include "FreeRTOS.h"
#include "task.h"

#define BME280_SDA_PIN 8
#define BME280_SCL_PIN 9
#define BME280_STOMASENSE_I2C_INTERFACE i2c0

#define BME280_BLOCKING_DELAY 0

bool bme280_wake_up();
bool bme280_sleep();
bool bme280_reset();

void bme280_init();

const struct bme280_data * bme280_read();

uint32_t bme280_delay_ms();

#endif /* BME280_H */