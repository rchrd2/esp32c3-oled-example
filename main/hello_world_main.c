/* Blink Example

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "u8g2_esp32_hal.h"
static const char *TAG = "example";

u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 8

static uint8_t s_led_state = 0;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    u8g2_esp32_hal.sda = 5;
    u8g2_esp32_hal.scl = 6;

    /* Configure the peripheral according to the LED type */
    configure_led();
    u8g2_esp32_hal_init(u8g2_esp32_hal);
    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_72x40_er_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_timR18_tf);

    uint8_t counter = 1;
    char display_str[16];

    while (1)
    {
        /* Update display with current counter */
        u8g2_ClearBuffer(&u8g2);
        snprintf(display_str, sizeof(display_str), "%d", counter);

        /* Center text horizontally and vertically */
        int16_t text_width = u8g2_GetStrWidth(&u8g2, display_str);
        int16_t display_width = u8g2_GetDisplayWidth(&u8g2);
        int16_t display_height = u8g2_GetDisplayHeight(&u8g2);
        int16_t font_ascent = u8g2_GetFontAscent(&u8g2);

        int16_t x = (display_width - text_width) / 2;
        int16_t y = (display_height + font_ascent) / 2;

        u8g2_DrawStr(&u8g2, x, y, display_str);
        u8g2_SendBuffer(&u8g2);

        ESP_LOGI(TAG, "Displaying number: %d", counter);

        /* Increment counter and wrap from 10 to 1 */
        counter++;
        if (counter > 10) {
            counter = 1;
        }

        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
// usb upload sometimes fails, not sure why, i did not do anything other than press upload again.