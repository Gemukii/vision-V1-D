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
#include "app_uart_c6.h"

static const char *TAG = "app_uart_c6";

// UART configuration for ESP32-C6 communication
// Note: Check your board schematic for the correct pins connecting P4 to C6
// Using UART_NUM_2 to avoid conflict with LoRa (UART_NUM_1)
#define UART_C6_PORT_NUM      UART_NUM_2
#define UART_C6_BAUD_RATE     115200
#define UART_C6_TX_PIN        52
#define UART_C6_RX_PIN        51
#define C6_RST_PIN            54
#define C6_BOOT_PIN           53
#define UART_C6_BUF_SIZE      1024

static bool s_uart_c6_initialized = false;
static bool s_c6_ready = false;
static TaskHandle_t s_c6_rx_task_handle = NULL;

// UART RX task - receives data from ESP32-C6
static void uart_c6_rx_task(void *arg)
{
    uint8_t rx_buf[128];

    ESP_LOGI(TAG, "UART C6 RX task started, waiting for READY from C6");

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_C6_PORT_NUM, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            // Null-terminate the string
            rx_buf[len] = '\0';

            // Log received data
            ESP_LOGI(TAG, "[C6 RX] %s", (char*)rx_buf);

            // Check if "READY" is in the received data
            if (strstr((char*)rx_buf, "READY") != NULL) {
                if (!s_c6_ready) {
                    s_c6_ready = true;
                    ESP_LOGI(TAG, "ESP32-C6 is READY - communication established");
                }
            }
        }
    }
}

static void c6_reset(void)
{
    ESP_LOGI(TAG, "Performing hardware reset of ESP32-C6...");

    // Initialize GPIOs for Reset and Boot control
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << C6_RST_PIN) | (1ULL << C6_BOOT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Set BOOT pin HIGH to ensure normal boot (not download mode)
    gpio_set_level(C6_BOOT_PIN, 1);

    // Toggle Reset pin
    gpio_set_level(C6_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(C6_RST_PIN, 1);

    // Wait for C6 to boot up
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t app_uart_c6_init(void)
{
    // Reset the C6 module first to ensure clean state
    c6_reset();

    ESP_LOGI(TAG, "Initializing UART for ESP32-C6 communication");
    ESP_LOGI(TAG, "UART config: TX=GPIO%d, RX=GPIO%d, Baud=%d", UART_C6_TX_PIN, UART_C6_RX_PIN, UART_C6_BAUD_RATE);

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_C6_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters first
    esp_err_t ret = uart_param_config(UART_C6_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set UART pins before installing driver
    ret = uart_set_pin(UART_C6_PORT_NUM, UART_C6_TX_PIN, UART_C6_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Install UART driver last
    ret = uart_driver_install(UART_C6_PORT_NUM, UART_C6_BUF_SIZE, UART_C6_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    s_uart_c6_initialized = true;
    ESP_LOGI(TAG, "UART C6 initialized successfully");

    // Start UART RX task
    xTaskCreatePinnedToCore(
        uart_c6_rx_task,
        "UART_C6_RX",
        4096,
        NULL,
        5,
        &s_c6_rx_task_handle,
        0
    );

    return ESP_OK;
}

esp_err_t app_uart_c6_send_message(const char *message)
{
    if (!s_uart_c6_initialized) {
        ESP_LOGW(TAG, "UART C6 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    size_t len = strlen(message);

    // Send data via UART
    int written = uart_write_bytes(UART_C6_PORT_NUM, message, len);

    if (written < 0) {
        ESP_LOGE(TAG, "Failed to write to UART C6");
        return ESP_FAIL;
    }

    if (written != len) {
        ESP_LOGW(TAG, "Only wrote %d of %d bytes", written, len);
        return ESP_ERR_TIMEOUT;
    }

    // Wait for TX to complete
    esp_err_t ret = uart_wait_tx_done(UART_C6_PORT_NUM, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for TX done: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

bool app_uart_c6_is_ready(void)
{
    return s_c6_ready;
}