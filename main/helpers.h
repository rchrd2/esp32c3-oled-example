#ifndef HELPERS_H
#define HELPERS_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Configure CPU frequency using ESP-IDF power management API
 * @param freq_mhz Target CPU frequency in MHz (typically 80 or 160 for ESP32-C3)
 * @return ESP_OK on success, error code otherwise
 *
 * Note: Requires CONFIG_PM_ENABLE to be set in sdkconfig.
 * If power management is not enabled, this function will log a warning.
 */
esp_err_t set_cpu_frequency(int freq_mhz);

/**
 * @brief Sleep for specified milliseconds using light sleep mode
 * @param ms Milliseconds to sleep
 *
 * This function enters light sleep mode which significantly reduces power
 * consumption compared to vTaskDelay. The CPU is powered down during sleep
 * but RAM is preserved. The system wakes up automatically after the specified time.
 *
 * Benefits:
 * - Significantly lower power consumption (can reduce from ~40mA to ~0.8mA)
 * - Preserves all RAM state (no code/data loss)
 * - Automatic wake-up on timer
 *
 * Note: During light sleep, I2C and other peripherals are clock-gated.
 * The OLED display will stay ON showing the last frame (it's externally powered).
 * After wakeup, I2C should resume automatically.
 *
 * Use this instead of vTaskDelay for delays longer than ~10ms when power
 * consumption is a concern (battery-powered devices, etc.)
 */
void light_sleep_delay_ms(uint32_t ms);

#endif // HELPERS_H

