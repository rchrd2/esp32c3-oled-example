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
#include "esp_log.h"
#include "sdkconfig.h"

#include "u8g2_esp32_hal.h"
static const char *TAG = "example";

u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

/* OLED contrast/brightness range (0-255, higher = brighter) */
#define OLED_CONTRAST_MIN 20   /* Dimmest brightness (for counter = 1) */
#define OLED_CONTRAST_MAX 255 /* Brightest brightness (for counter = 10) */

/* Font scaling factor (1 = normal, 2 = 2x larger, 3 = 3x larger, etc.) */
/* Change this value to scale the font larger or smaller */
/* Note: u8g2 does not have built-in transform/scale functions like CSS */
/* This implementation draws to a temporary bitmap and scales using DrawBox for pixel-perfect scaling */
#define FONT_SCALE 4

/* Draw scaled text by replicating glyphs */
/* Note: u8g2 has no built-in transform functions, so we replicate glyphs */
/* This creates a larger bold appearance rather than crisp pixel-perfect scaling */
static void draw_scaled_str(u8g2_t *u8g2, int16_t x, int16_t y, const char *str)
{
    const char *s = str;
    int16_t font_ascent = u8g2_GetFontAscent(u8g2);
    int16_t display_width = u8g2_GetDisplayWidth(u8g2);
    int16_t display_height = u8g2_GetDisplayHeight(u8g2);

    char single_char[2] = {0, 0};

    while (*s != '\0') {
        single_char[0] = *s;
        int16_t char_width = u8g2_GetStrWidth(u8g2, single_char);

        /* Draw character FONT_SCALE x FONT_SCALE times with 1-pixel offsets */
        /* This creates a scaled-up, bold appearance */
        /* y is the baseline position, so we draw copies at y, y+1, y+2, etc. */
        for (int scale_y = 0; scale_y < FONT_SCALE; scale_y++) {
            for (int scale_x = 0; scale_x < FONT_SCALE; scale_x++) {
                int16_t offset_x = x + scale_x;
                int16_t offset_y = y + scale_y;  /* Baseline for this copy */

                /* Clamp coordinates to screen bounds */
                if (offset_x >= 0 && offset_x < display_width) {
                    /* DrawGlyph expects baseline y, so pass offset_y directly */
                    u8g2_DrawGlyph(u8g2, offset_x, offset_y, *s);
                }
            }
        }

        /* Advance position by scaled character width (char_width + overlap) */
        x += char_width + (FONT_SCALE - 1);

        /* Stop if we've gone off screen */
        if (x >= display_width) break;

        s++;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting OLED example...");
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
    /* Change font size: Just replace the font name on the line below.
     *
     * Standard fonts with numbered sizes (change the number: 08, 10, 12, 14, 18, 24 = larger):
     *   u8g2_SetFont(&u8g2, u8g2_font_timR08_tf);      // Size 8 (small)
     *   u8g2_SetFont(&u8g2, u8g2_font_timR10_tf);      // Size 10 (small-medium)
     *   u8g2_SetFont(&u8g2, u8g2_font_timR12_tf);      // Size 12 (medium)
     *   u8g2_SetFont(&u8g2, u8g2_font_timR14_tf);      // Size 14 (medium-large)
     *   u8g2_SetFont(&u8g2, u8g2_font_timR18_tf);      // Size 18 (large) <- good size
     *   u8g2_SetFont(&u8g2, u8g2_font_timR24_tf);      // Size 24 (very large)
     *
     * Bpixel font family (only 2 sizes available):
     *   u8g2_SetFont(&u8g2, u8g2_font_bpixel_tr);      // Regular size
     *   u8g2_SetFont(&u8g2, u8g2_font_bpixel_te);      // Regular size (extended charset)
     *   u8g2_SetFont(&u8g2, u8g2_font_bpixeldouble_tr); // Double size (current)
     *
     * Other pixel fonts:
     *   u8g2_SetFont(&u8g2, u8g2_font_Pixellari_tf);   // Pixel font (medium)
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

        /* Center text horizontally and vertically (scaled) */
        /* Calculate actual scaled dimensions */
        int16_t display_width = u8g2_GetDisplayWidth(&u8g2);
        int16_t display_height = u8g2_GetDisplayHeight(&u8g2);
        int16_t font_ascent = u8g2_GetFontAscent(&u8g2);
        int16_t font_descent = u8g2_GetFontDescent(&u8g2);

        /* Calculate text width and scaled dimensions */
        int16_t text_width = u8g2_GetStrWidth(&u8g2, display_str);
        /* Scaled width: with overlapping glyphs at x, x+1, ..., x+(FONT_SCALE-1) */
        /* Total width = text_width + (FONT_SCALE - 1) */
        int16_t scaled_text_width = text_width + (FONT_SCALE - 1);

        /* Scaled height: with overlapping glyphs at baseline y, y+1, ..., y+(FONT_SCALE-1) */
        /* Top is at (y - font_ascent), bottom is at (y + FONT_SCALE - 1 + font_descent) */
        /* So total height = font_ascent + font_descent + (FONT_SCALE - 1) */
        int16_t scaled_height = font_ascent + font_descent + (FONT_SCALE - 1);
        int16_t scaled_top_offset = font_ascent;  /* Distance from baseline to top */

        /* Center horizontally */
        int16_t x = (display_width - scaled_text_width) / 2;
        if (x < 0) x = 0;  /* Clamp to screen bounds */

        /* Center vertically */
        /* Text top is at (y - scaled_top_offset), text bottom is at (y - scaled_top_offset + scaled_height) */
        /* We want center at display_height / 2 */
        /* Center = (y - scaled_top_offset) + scaled_height / 2 = display_height / 2 */
        /* So: y = display_height / 2 - scaled_height / 2 + scaled_top_offset */
        /* Simplified: y = (display_height - scaled_height) / 2 + scaled_top_offset */
        int16_t y = (display_height - scaled_height) / 2 + scaled_top_offset;

        /* Ensure baseline is valid */
        if (y < scaled_top_offset) {
            y = scaled_top_offset;  /* Push down if too high */
        } else if (y - scaled_top_offset + scaled_height > display_height) {
            y = display_height - scaled_height + scaled_top_offset;  /* Push up if too low */
        }

        /* Draw scaled text - change FONT_SCALE define above to adjust scale factor */
        /* For debugging: if FONT_SCALE is 1, use normal drawing */
        if (FONT_SCALE == 1) {
            /* Normal drawing without scaling */
            u8g2_DrawStr(&u8g2, x, y, display_str);
        } else {
            /* Scaled drawing */
            draw_scaled_str(&u8g2, x, y, display_str);
        }
        u8g2_SendBuffer(&u8g2);

        counter++;
        if (counter > 10) {
            counter = 1;
        }

        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
// usb upload sometimes fails, not sure why, i did not do anything other than press upload again.