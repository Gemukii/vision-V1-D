/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_uart_lora.h"

static const char *TAG = "app_uart_lora";

// UART configuration
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     9600
#define UART_TX_PIN        6
#define UART_RX_PIN        7
#define UART_BUF_SIZE      1024

static bool s_uart_initialized = false;
static bool s_lora_ready = false;
static TaskHandle_t s_rx_task_handle = NULL;

// UART RX task - receives data from LoRaWAN device
static void uart_lora_rx_task(void *arg)
{
    uint8_t rx_buf[128];

    ESP_LOGI(TAG, "UART RX task started, waiting for READY from LoRaWAN device");

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_PORT_NUM, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            // Null-terminate the string
            rx_buf[len] = '\0';

            // Log received data
            ESP_LOGI(TAG, "[UART RX] %s", (char*)rx_buf);

            // Check if "READY" is in the received data
            if (strstr((char*)rx_buf, "READY") != NULL) {
                if (!s_lora_ready) {
                    s_lora_ready = true;
                    ESP_LOGI(TAG, "LoRaWAN device is READY - face detection messages will now be sent");
                }
            }
        }
    }
}

esp_err_t app_uart_lora_init(void)
{
    ESP_LOGI(TAG, "Initializing UART for LoRaWAN communication");
    ESP_LOGI(TAG, "UART config: TX=GPIO%d, RX=GPIO%d, Baud=%d", UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters first
    esp_err_t ret = uart_param_config(UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set UART pins before installing driver
    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Install UART driver last
    ret = uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    s_uart_initialized = true;
    ESP_LOGI(TAG, "UART initialized successfully");

    // Start UART RX task to receive "READY" from LoRaWAN device
    xTaskCreatePinnedToCore(
        uart_lora_rx_task,
        "UART_LORA_RX",
        4096,
        NULL,
        5,
        &s_rx_task_handle,
        0
    );

    return ESP_OK;
}

esp_err_t app_uart_lora_send_message(const char *message)
{
    if (!s_uart_initialized) {
        ESP_LOGW(TAG, "UART not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    size_t len = strlen(message);

    // Send data via UART
    int written = uart_write_bytes(UART_PORT_NUM, message, len);

    if (written < 0) {
        ESP_LOGE(TAG, "Failed to write to UART");
        return ESP_FAIL;
    }

    if (written != len) {
        ESP_LOGW(TAG, "Only wrote %d of %d bytes", written, len);
        return ESP_ERR_TIMEOUT;
    }

    // Wait for TX to complete - ensures data is actually transmitted on GPIO50
    esp_err_t ret = uart_wait_tx_done(UART_PORT_NUM, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for TX done: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

bool app_uart_lora_is_ready(void)
{
    return s_lora_ready;
}
