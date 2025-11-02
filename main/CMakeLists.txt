idf_component_register(SRCS "hello_world_main.c" "u8g2_esp32_hal.c" "helpers.c"
                    INCLUDE_DIRS "."
                    REQUIRES u8g2 driver esp_pm)
