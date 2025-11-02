#ifndef HELPERS_H
#define HELPERS_H

#include "esp_err.h"

/**
 * @brief Configure CPU frequency using ESP-IDF power management API
 * @param freq_mhz Target CPU frequency in MHz (typically 80 or 160 for ESP32-C3)
 * @return ESP_OK on success, error code otherwise
 *
 * Note: Requires CONFIG_PM_ENABLE to be set in sdkconfig.
 * If power management is not enabled, this function will log a warning.
 */
esp_err_t set_cpu_frequency(int freq_mhz);

#endif // HELPERS_H

