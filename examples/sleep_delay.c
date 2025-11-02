/* Goes into power save mode between frames
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "u8g2_esp32_hal.h"
#include "helpers.h"
static const char *TAG = "example";

u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

/* OLED contrast/brightness range (0-255, higher = brighter) */
#define OLED_CONTRAST_MIN 20   /* Dimmest brightness (for counter = 1) */
#define OLED_CONTRAST_MAX 255 /* Brightest brightness (for counter = 10) */

/* Target frames per second */
#define TARGET_FPS 5

/* Calculate frame delay in milliseconds from FPS */
#define FRAME_DELAY_MS (1000 / TARGET_FPS)

void app_main(void)
{
    ESP_LOGI(TAG, "Starting OLED example...");

    /* Set CPU frequency - typically 80 or 160 MHz for ESP32-C3 */
    /* Note: 10MHz is not supported - minimum is usually 80MHz */
    set_cpu_frequency(80);
    u8g2_esp32_hal.sda = 5;
    u8g2_esp32_hal.scl = 6;

    u8g2_esp32_hal_init(u8g2_esp32_hal);
    ESP_LOGI(TAG, "OLED HAL initialized");
    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_72x40_er_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    ESP_LOGI(TAG, "OLED display initialized");

    /* Pixel font options - change the font name below to try different styles:
     *   u8g2_font_bpixeldouble_tr    - Double-size block pixel (current, larger)
     *   u8g2_font_tallpixelextended_tf - Tall extended pixel font (larger)
     *   u8g2_font_pixelmordred_tf    - Pixel mordred style (medium)
     *   u8g2_font_Pixellari_tf       - Pixellari style (medium)
     *   u8g2_font_pxclassic_tf       - Classic pixel style (medium)
     *   u8g2_font_bpixel_tr          - Block pixel font (smaller)
     *   u8g2_font_micropixel_tf     - Small pixel font (small)
     *   u8g2_font_new3x9pixelfont_tf - 3x9 pixel font (small)
     *   u8g2_font_fivepx_tr          - Very small 5px font (very small)
     */
    u8g2_SetFont(&u8g2, u8g2_font_bpixeldouble_tr);
    ESP_LOGI(TAG, "Starting number cycle...");

    uint8_t counter = 1;
    char display_str[16];

    while (1)
    {
        /* Calculate brightness based on counter (1 = dimmest, 10 = brightest) */
        uint8_t contrast = OLED_CONTRAST_MIN + ((counter - 1) * (OLED_CONTRAST_MAX - OLED_CONTRAST_MIN)) / 9;
        u8g2_SetContrast(&u8g2, contrast);

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

        /* Draw a small moving dot from top-left to top-right */
        /* Counter 1 = top-left (0,0), Counter 10 = top-right */
        int16_t dot_x = ((counter - 1) * (display_width - 1)) / 9;
        int16_t dot_y = 0;  /* Stay at top edge */

        /* Draw a 2x2 pixel dot for visibility */
        u8g2_DrawBox(&u8g2, dot_x, dot_y, 2, 2);

        u8g2_SendBuffer(&u8g2);

        counter++;
        if (counter > 10) {
            counter = 1;
        }

        // Use light sleep instead of vTaskDelay for power saving
        // This reduces power consumption from ~40mA to ~0.8mA during sleep
        light_sleep_delay_ms(FRAME_DELAY_MS);
    }
}
// usb upload sometimes fails, not sure why, i did not do anything other than press upload again.