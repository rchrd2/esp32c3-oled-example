#include "helpers.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "sdkconfig.h"

static const char *TAG = "helpers";

esp_err_t set_cpu_frequency(int freq_mhz)
{
    #ifdef CONFIG_PM_ENABLE
    esp_pm_config_t pm_config = {
        .max_freq_mhz = freq_mhz,  /* Maximum CPU frequency */
        .min_freq_mhz = freq_mhz,  /* Minimum CPU frequency (set both equal for fixed freq) */
        .light_sleep_enable = false
    };

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to configure power management: %s", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "CPU frequency set to %d MHz", freq_mhz);
        return ESP_OK;
    }
    #else
    ESP_LOGW(TAG, "Power management not enabled. Set CONFIG_PM_ENABLE in sdkconfig to change CPU frequency dynamically.");
    ESP_LOGI(TAG, "Current CPU frequency: %d MHz (set via CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    return ESP_ERR_NOT_SUPPORTED;
    #endif
}

