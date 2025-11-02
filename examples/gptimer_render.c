/* Example using high-resolution timing.
   Written by Cursor AI. Not sure of correctness.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "u8g2_esp32_hal.h"
#include "helpers.h"
static const char *TAG = "example";

u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

/* OLED contrast/brightness range (0-255, higher = brighter) */
#define OLED_CONTRAST_MIN 20   /* Dimmest brightness (for counter = 1) */
#define OLED_CONTRAST_MAX 255 /* Brightest brightness (for counter = 10) */

/* Button configuration */
#define BUTTON_GPIO 9
#define BUTTON_PRESSED_LEVEL 0  // Button pulls GPIO low when pressed

/* FPS values to cycle through (1 to 30, scaled for 5 values) */
static const uint32_t fps_values[] = {1, 2, 5, 10, 30};
static size_t current_fps_index = 0;

/* GPTimer configuration */
#define TIMER_RESOLUTION_HZ 1000000  // 1MHz = 1 tick per microsecond
static gptimer_handle_t g_timer = NULL;
static SemaphoreHandle_t g_frame_update_sem = NULL;
static uint32_t current_fps = fps_values[0];  // Current FPS value

// GPTimer alarm callback (runs in ISR context)
// Per ESP-IDF GPTimer documentation:
// - Callbacks run under ISR environment
// - IRAM_ATTR ensures function is in IRAM for cache safety
// - Keep ISR short: only signal semaphore, do work in main task
// - Return true if higher priority task was woken (optimizes context switch)
static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // Signal that it's time to update the frame (keep ISR minimal)
    BaseType_t must_yield = pdFALSE;
    xSemaphoreGiveFromISR(g_frame_update_sem, &must_yield);
    return (must_yield == pdTRUE);  // Return true if context switch needed
}

// Update timer period when FPS changes
static void update_timer_period(uint32_t target_fps)
{
    if (target_fps == 0 || target_fps > 1000) {
        ESP_LOGW(TAG, "Invalid FPS value: %lu, using default", (unsigned long)target_fps);
        target_fps = 1;
    }

    uint64_t alarm_count_us = 1000000ULL / target_fps;
    if (alarm_count_us == 0) {
        ESP_LOGW(TAG, "Calculated alarm period too small: %llu, using minimum",
                 (unsigned long long)alarm_count_us);
        alarm_count_us = 1;
    }

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = alarm_count_us,
        .reload_count = 0,
        .flags = {
            .auto_reload_on_alarm = true,
        }
    };

    esp_err_t ret = gptimer_set_alarm_action(g_timer, &alarm_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update timer period: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "FPS changed to %lu (period: %llu microseconds)",
                 (unsigned long)target_fps, (unsigned long long)alarm_count_us);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting OLED example...");

    // Initialize OLED first (before button GPIO to avoid any conflicts)
    u8g2_esp32_hal.sda = 5;
    u8g2_esp32_hal.scl = 6;

    u8g2_esp32_hal_init(u8g2_esp32_hal);
    ESP_LOGI(TAG, "OLED HAL initialized - I2C pins: SDA=GPIO%d, SCL=GPIO%d",
             u8g2_esp32_hal.sda, u8g2_esp32_hal.scl);

    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_72x40_er_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);

    // SSD1306 I2C address: 0x78 (8-bit address) = 0x3C (7-bit address)
    // u8x8_SetI2CAddress expects the 8-bit address
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);
    ESP_LOGI(TAG, "Attempting to initialize OLED with I2C address 0x78 (0x3C 7-bit)");

    // Initialize display - this will fail if I2C communication doesn't work
    // Common causes: wrong wiring, missing pull-ups, wrong address, display not powered
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    ESP_LOGI(TAG, "OLED display initialized (if I2C failed, check wiring/pull-ups/address)");

    // Configure button GPIO after OLED initialization
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);  // Enable internal pull-up
    ESP_LOGI(TAG, "Button configured on GPIO %d (pressed = %d)", BUTTON_GPIO, BUTTON_PRESSED_LEVEL);

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

    // Create semaphore for frame updates (non-blocking synchronization)
    g_frame_update_sem = xSemaphoreCreateBinary();
    if (g_frame_update_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

    // Configure GPTimer for periodic alarm (non-blocking, interrupt-driven)
    // Following ESP-IDF GPTimer documentation best practices:
    // 1. Create timer with desired resolution
    // 2. Register callbacks BEFORE enabling (required by driver)
    // 3. Set alarm action
    // 4. Enable timer (registers interrupt service)
    // 5. Start timer (begins counting)
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,  // Default clock source (usually APB)
        .direction = GPTIMER_COUNT_UP,       // Count upward from 0
        .resolution_hz = TIMER_RESOLUTION_HZ, // 1MHz = 1 tick per microsecond
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &g_timer));

    // Register event callbacks BEFORE enabling (required by driver design)
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(g_timer, &cbs, NULL));

    // Configure timer alarm with initial FPS value
    update_timer_period(current_fps);

    // Enable timer (transitions from "init" to "enable" state, registers interrupt)
    // Per documentation: enables interrupt service if lazy installed in register_event_callbacks
    ESP_ERROR_CHECK(gptimer_enable(g_timer));

    // Start timer (transitions from "enable" to "run" state, begins counting)
    ESP_ERROR_CHECK(gptimer_start(g_timer));

    ESP_LOGI(TAG, "GPTimer started: %lu FPS", (unsigned long)current_fps);

    // Button state tracking
    int button_last_state = gpio_get_level(BUTTON_GPIO);
    bool button_press_handled = false;

    // Main loop: non-blocking to handle multiple events (timer updates + other inputs)
    // Note: while(1) is the standard FreeRTOS pattern for tasks that run forever.
    // Using a short timeout on semaphore allows us to check for other inputs (knob, buttons, etc.)
    while (1)
    {
        // Try to take semaphore with short timeout (non-blocking pattern)
        // Returns pdTRUE if timer fired, pdFALSE if timeout
        if (xSemaphoreTake(g_frame_update_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Timer fired - time to update display
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
        }

        // Check button state every loop iteration (not just when timer fires)
        // This ensures reliable detection even at high FPS
        int button_current_state = gpio_get_level(BUTTON_GPIO);

        // Detect button release (transition from pressed to not pressed)
        // Only change FPS on release, not while held down
        if (button_last_state == BUTTON_PRESSED_LEVEL &&
            button_current_state != BUTTON_PRESSED_LEVEL &&
            !button_press_handled) {
            // Button was pressed and now released - cycle to next FPS
            current_fps_index = (current_fps_index + 1) % (sizeof(fps_values) / sizeof(fps_values[0]));
            current_fps = fps_values[current_fps_index];
            update_timer_period(current_fps);
            button_press_handled = true;  // Mark as handled to prevent multiple triggers
        }

        // Reset handled flag when button is pressed again
        if (button_current_state == BUTTON_PRESSED_LEVEL && button_press_handled) {
            button_press_handled = false;
        }

        button_last_state = button_current_state;

        // This is where you'd handle knob rotation, etc.
    }
}
