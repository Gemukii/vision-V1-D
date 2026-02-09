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
 * @brief Initialize UART communication with LoRaWAN device
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_uart_lora_init(void);

/**
 * @brief Send a string message via UART to LoRaWAN device
 *
 * @param message The message string to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_uart_lora_send_message(const char *message);

/**
 * @brief Check if LoRaWAN device is ready (received "READY" message)
 *
 * @return true if LoRaWAN is ready, false otherwise
 */
bool app_uart_lora_is_ready(void);

#ifdef __cplusplus
}
#endif
