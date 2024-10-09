#include "bme280.h"
// #include "pico/binary_info.h" // for picotool


static void print_msg(int8_t res)
{
    switch (res)
    {
    case BME280_OK:
        INFO_PRINTLN("bme280 OK");
        break;
    case BME280_E_NULL_PTR:
        ERROR_PRINTLN("bme280 ERROR: null ptr");
        break;
    case BME280_E_COMM_FAIL:
        ERROR_PRINTLN("bme280 ERROR: comm fail");
        break;
    case BME280_E_INVALID_LEN:
        ERROR_PRINTLN("bme280 ERROR: invalid len");
        break;
    case BME280_E_DEV_NOT_FOUND:
        ERROR_PRINTLN("bme280 ERROR: dev not found");
        break;
    case BME280_E_SLEEP_MODE_FAIL:
        ERROR_PRINTLN("bme280 ERROR: sleep mode fail");
        break;
    case BME280_E_NVM_COPY_FAILED:
        ERROR_PRINTLN("bme280 ERROR: nvm copy failed");
        break;
    case BME280_W_INVALID_OSR_MACRO:
        WARN_PRINTLN("bme280 WARNING: invalid osr macro");
        break;
    default:
        ERROR_PRINTFLN("bme280 ERROR: unknown with exit code %i", res);
        break;
    }
}

// i2c callbacks ////////
// typedef BME280_INTF_RET_TYPE (*bme280_read_fptr_t)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE bme280_read_i2c(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // int i2c_read_blocking_until (i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, absolute_time_t until)
    const bool nonstop = false;               // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s

    // TODO: check if when FreeRTOS this line must be critical
    int rslt = i2c_read_timeout_us(BME280_STOMASENSE_I2C_INTERFACE, reg_addr, reg_data, len, nonstop, timeout_us);

    if (rslt == PICO_ERROR_TIMEOUT)
        return -1;
    else if (rslt == PICO_ERROR_GENERIC)
        return -2;
    else if (rslt != len)
        return -3;
    return BME280_INTF_RET_SUCCESS;
}

// typedef BME280_INTF_RET_TYPE (*bme280_write_fptr_t)(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE bme280_write_i2c(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // int i2c_write_blocking_until (i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, absolute_time_t until)
    const bool nonstop = false;               // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s

    // TODO: check if when FreeRTOS this line must be critical
    int rslt = i2c_write_timeout_us(BME280_STOMASENSE_I2C_INTERFACE, reg_addr, reg_data, len, nonstop, timeout_us);
    
    if (rslt == PICO_ERROR_TIMEOUT)
        return -1;
    else if (rslt == PICO_ERROR_GENERIC)
        return -2;
    else if (rslt != len)
        return -3;
    return BME280_INTF_RET_SUCCESS;
}

// typedef void (*bme280_delay_us_fptr_t)(uint32_t period, void *intf_ptr);
static void bme280_delay_us_i2c(uint32_t period_us, void *intf_ptr)
{
    sleep_us(period_us);
}
///////////////////////////////

static struct bme280_dev dev = {
    .chip_id     = 0,
    .intf        = BME280_I2C_INTF,
    .intf_ptr    = NULL,
    .read        = bme280_read_i2c,
    .write       = bme280_write_i2c,
    .delay_us    = bme280_delay_us_i2c
};
static const struct bme280_settings settings = {
    .osr_p        = BME280_OVERSAMPLING_1X,
    .osr_t        = BME280_OVERSAMPLING_1X,
    .osr_h        = BME280_OVERSAMPLING_1X,
    .filter       = BME280_FILTER_COEFF_OFF,
    .standby_time = BME280_STANDBY_TIME_500_MS, // I guess this is fine
};
static struct bme280_data last_data = {0};
static uint32_t meas_delay_ms = 5000;
volatile static bool flag_readable = true;

bool bme280_wake_up()
{
    int8_t res = bme280_lib_set_sensor_mode(BME280_POWERMODE_FORCED, &dev);
    print_msg(res);
    return res == BME280_OK;
}

bool bme280_sleep()
{
    int8_t res = bme280_lib_set_sensor_mode(BME280_POWERMODE_SLEEP, &dev);
    print_msg(res);
    return res == BME280_OK;
}

bool bme280_reset()
{
    int8_t res = bme280_lib_soft_reset(&dev);
    print_msg(res);
    return res == BME280_OK;
}

void bme280_init()
{
    // bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C)); // for picotool
    gpio_set_function(BME280_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BME280_SCL_PIN, GPIO_FUNC_I2C);
    i2c_init(BME280_STOMASENSE_I2C_INTERFACE, 100000); // 100 kHz
    DEBUG_PRINTFLN("I2C set up");

    int8_t res = bme280_lib_init(&dev);
    print_msg(res);
    if (res != BME280_OK)
    {
        CRITICAL_PRINTLN("Resetting...");
        sleep_ms(5000);
        USER_RESET();
    }
    DEBUG_PRINTFLN("bme280 lib init");

    res = bme280_lib_set_sensor_settings(
        BME280_SEL_OSR_PRESS | BME280_SEL_OSR_TEMP | BME280_SEL_OSR_HUM | BME280_SEL_FILTER | BME280_SEL_STANDBY,
        &settings,
        &dev
    );
    print_msg(res);
    if (res != BME280_OK)
    {
        CRITICAL_PRINTLN("Resetting...");
        sleep_ms(5000);
        USER_RESET();
    }
    DEBUG_PRINTFLN("bme280 lib set sensor settings");

    res = bme280_lib_cal_meas_delay(&meas_delay_ms, &settings);
    print_msg(res);
    if (res != BME280_OK)
    {
        CRITICAL_PRINTLN("Resetting...");
        sleep_ms(5000);
        USER_RESET();
    }
    DEBUG_PRINTFLN("bme280 got cal meas delay");

    if (!bme280_wake_up())
    {
        CRITICAL_PRINTLN("Resetting...");
        sleep_ms(5000);
        USER_RESET();
    }
    DEBUG_PRINTFLN("bme280 woken up");
    DEBUG_PRINTFLN("bme280 init");
}

static int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    flag_readable = true;
    // <0 to reschedule the same alarm this many us from the time the alarm was previously scheduled to fire
    // >0 to reschedule the same alarm this many us from the time this method returns
    // 0 to not reschedule the alarm
    return 0;
}

const struct bme280_data * bme280_read()
{
    // if it returns NULL, new data couldn't be read
    if (!flag_readable) // still can't read bme280
    {
        return NULL;
    }

    uint8_t status_reg;
    int8_t res = bme280_get_regs(BME280_REG_STATUS, &status_reg, 1, &dev);
    print_msg(res);
    if (res != BME280_OK)
        return NULL;
    if (!(status_reg & BME280_STATUS_MEAS_DONE))
        return NULL;

    #if BME280_BLOCKING_DELAY
    dev.delay_us(meas_delay_ms, dev.intf_ptr);
    #else
    vTaskDelay(MIN(meas_delay_ms / portTICK_PERIOD_MS, 1));
    #endif
    struct bme280_data data_temp;
    res = bme280_lib_get_sensor_data(BME280_ALL, &data_temp, &dev);
    print_msg(res);
    if (res != BME280_OK)
        return NULL;

    alarm_id_t alarm_id = add_alarm_in_ms(meas_delay_ms, alarm_callback, NULL, true);
    if (alarm_id > 0)
        flag_readable = false;
    else // couldn't set alarm
        flag_readable = true; // failsafe, this theoretically should never be reached

    taskENTER_CRITICAL();
    memcpy(&last_data, &data_temp, sizeof(struct bme280_data));
    #ifndef BME280_DOUBLE_ENABLE
    last_data.humidity = last_data.humidity / 1000;
    last_data.temperature = last_data.temperature / 1000;
    last_data.pressure = last_data.pressure / 1000;
    #endif
    taskEXIT_CRITICAL();


    return (const struct bme280_data *)&last_data;
}

uint32_t bme280_delay_ms()
{
    return meas_delay_ms;
}