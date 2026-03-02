/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C6 GPIO status codes received on RX pin
 */
typedef enum {
    C6_STATUS_IDLE = 0,      /*!< No signal or idle state */
    C6_STATUS_CODE_OK = 1,   /*!< Code validation successful (GPIO HIGH) */
    C6_STATUS_CODE_ERROR = 2 /*!< Code validation error (GPIO LOW pulse) */
} c6_status_t;

/**
 * @brief Callback function type for C6 status changes
 *
 * @param status The new status received from C6
 * @param user_data User data passed during callback registration
 */
typedef void (*c6_status_callback_t)(c6_status_t status, void *user_data);

/**
 * @brief Initialize GPIO communication with ESP32-C6
 *
 * Configures GPIO pins for bidirectional communication:
 * - TX pin (GPIO 8): Output to send commands to C6
 * - RX pin (GPIO 10): Input to receive status from C6
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_gpio_c6_init(void);

/**
 * @brief Send a command to C6 via GPIO TX pin
 *
 * @param level GPIO level to set (0 or 1)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_gpio_c6_send_command(uint32_t level);

/**
 * @brief Read current status from C6 via GPIO RX pin
 *
 * @return Current GPIO level (0 or 1)
 */
uint32_t app_gpio_c6_read_status(void);

/**
 * @brief Register a callback for C6 status changes
 *
 * The callback will be called when the C6 RX pin state changes
 *
 * @param callback Callback function to register
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_gpio_c6_register_callback(c6_status_callback_t callback, void *user_data);

/**
 * @brief Check if C6 is ready (has sent ready signal)
 *
 * @return true if C6 is ready, false otherwise
 */
bool app_gpio_c6_is_ready(void);

/**
 * @brief Get last received status from C6
 *
 * @return Last status received from C6
 */
c6_status_t app_gpio_c6_get_last_status(void);

#ifdef __cplusplus
}
#endif
