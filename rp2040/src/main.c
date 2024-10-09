#include "pico/stdlib.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSStatic.h"

#include "utils.h"
#include "bme280.h"

void task_read_bme280(void *pvParameters);
#define task_read_bme280_stack_size (sizeof(uint8_t) + sizeof(int8_t) + sizeof(alarm_id_t) + sizeof(struct bme280_data) + sizeof(const struct bme280_data*)*2 + 256)
StackType_t task_read_bme280_stack[task_read_bme280_stack_size];
StaticTask_t task_read_bme280_buffer;
TaskHandle_t task_read_bme280_handle;

int main()
{
    stdio_init_all();
    sleep_ms(1000);
    printf("Begin\n");

    bme280_init();

    // read bme280
    task_read_bme280_handle = xTaskCreateStatic(
        task_read_bme280,
        "bme280",
        task_read_bme280_stack_size,
        NULL,
        1,
        task_read_bme280_stack,
        &task_read_bme280_buffer
    );

    INFO_PRINTLN("Starting scheduler");
    vTaskStartScheduler();
    CRITICAL_PRINTLN("Shouldn't have reached this section!");
}

void task_read_bme280(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(bme280_delay_ms() / portTICK_PERIOD_MS);
        const struct bme280_data *data = bme280_read();
        if (data)
            INFO_PRINTFLN("h: %f\%, t: %f*, p: %fhPa", data->humidity, data->temperature, data->pressure);
        else
            INFO_PRINTFLN("Data couldn't be read");
    }
}