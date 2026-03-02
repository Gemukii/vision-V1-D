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
 * @brief Initialize UART interface for ESP32-C6 communication
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_uart_c6_init(void);

/**
 * @brief Send a string message to ESP32-C6 via UART
 *
 * @param message The message string to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_uart_c6_send_message(const char *message);

/**
 * @brief Check if ESP32-C6 is ready (received "READY" message)
 *
 * @return true if C6 is ready, false otherwise
 */
bool app_uart_c6_is_ready(void);

#ifdef __cplusplus
}
#endif