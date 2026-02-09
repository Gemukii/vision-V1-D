/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB CDC interface
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_usb_cdc_init(void);

/**
 * @brief Send a string message via USB CDC
 *
 * @param message The message string to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_usb_cdc_send_message(const char *message);

/**
 * @brief Check if USB CDC is ready to send data
 *
 * @return true if ready, false otherwise
 */
bool app_usb_cdc_is_ready(void);

#ifdef __cplusplus
}
#endif
