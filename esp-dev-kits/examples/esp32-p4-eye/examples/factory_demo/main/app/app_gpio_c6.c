/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_gpio_c6.h"
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_ai_detect.h"
#include "app_uart_lora.h"

static const char *TAG = "app_c6_uart";

// UART configuration for C6 communication
#define C6_UART_PORT_NUM      UART_NUM_2
#define C6_UART_BAUD_RATE     9600
#define C6_UART_TX_PIN        8
#define C6_UART_RX_PIN        10
#define C6_UART_BUF_SIZE      1024

static bool s_uart_initialized = false;
static bool s_c6_ready = false;
static c6_status_t s_last_status = C6_STATUS_IDLE;
static c6_status_callback_t s_callback = NULL;
static void *s_callback_user_data = NULL;
static TaskHandle_t s_rx_task_handle = NULL;
static bool s_last_event_was_granted = false;  // Track last event to send only once

// UART RX task - receives text data from C6
static void uart_c6_rx_task(void *arg)
{
    uint8_t rx_buf[256];

    ESP_LOGI(TAG, "C6 UART RX task started, waiting for messages from C6");

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(C6_UART_PORT_NUM, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            // Null-terminate the string
            rx_buf[len] = '\0';

            // Log received data - THIS IS WHAT YOU'LL SEE IN TERMINAL
            ESP_LOGI(TAG, "[C6 MESSAGE] %s", (char*)rx_buf);

            // Parse received message
            if (strstr((char*)rx_buf, "READY") != NULL) {
                if (!s_c6_ready) {
                    s_c6_ready = true;
                    s_last_status = C6_STATUS_CODE_OK;
                    ESP_LOGI(TAG, ">>> C6 is READY <<<");
                }
            } else if (strstr((char*)rx_buf, "GRANTED") != NULL) {
                // CODE BON (1) - Send every GRANTED event
                s_last_status = C6_STATUS_CODE_OK;
                int face_count = app_ai_get_current_face_count();

                // Send message to LoRaWAN format: 1;<nb_faces>\n
                char lora_msg[32];
                snprintf(lora_msg, sizeof(lora_msg), "1;%d\n", face_count);
                app_uart_lora_send_message(lora_msg);

                ESP_LOGI(TAG, ">>> CODE GRANTED (1;%d) - Sent to LoRaWAN <<<", face_count);
                s_last_event_was_granted = true;
            } else if (strstr((char*)rx_buf, "DENIED") != NULL) {
                // CODE MAUVAIS (0) - Send every DENIED event
                s_last_status = C6_STATUS_CODE_ERROR;
                int face_count = app_ai_get_current_face_count();

                // Send message to LoRaWAN format: 0;<nb_faces>\n
                char lora_msg[32];
                snprintf(lora_msg, sizeof(lora_msg), "0;%d\n", face_count);
                app_uart_lora_send_message(lora_msg);

                ESP_LOGI(TAG, ">>> CODE DENIED (0;%d) - Sent to LoRaWAN <<<", face_count);
                s_last_event_was_granted = false;
            }

            // Call user callback if registered
            if (s_callback) {
                s_callback(s_last_status, s_callback_user_data);
            }
        }
    }
}

esp_err_t app_gpio_c6_init(void)
{
    ESP_LOGI(TAG, "Initializing UART communication with ESP32-C6");
    ESP_LOGI(TAG, "UART config: TX=GPIO%d, RX=GPIO%d, Baud=%d",
             C6_UART_TX_PIN, C6_UART_RX_PIN, C6_UART_BAUD_RATE);

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = C6_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters first
    esp_err_t ret = uart_param_config(C6_UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set UART pins before installing driver
    ret = uart_set_pin(C6_UART_PORT_NUM, C6_UART_TX_PIN, C6_UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Install UART driver
    ret = uart_driver_install(C6_UART_PORT_NUM, C6_UART_BUF_SIZE, C6_UART_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    s_uart_initialized = true;
    ESP_LOGI(TAG, "UART C6 initialized successfully");

    // Start UART RX task to receive messages from C6
    xTaskCreatePinnedToCore(
        uart_c6_rx_task,
        "UART_C6_RX",
        4096,
        NULL,
        5,
        &s_rx_task_handle,
        0
    );

    return ESP_OK;
}

esp_err_t app_gpio_c6_send_command(uint32_t level)
{
    if (!s_uart_initialized) {
        ESP_LOGW(TAG, "UART C6 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Send as text message
    const char *msg = level ? "HIGH\n" : "LOW\n";
    int written = uart_write_bytes(C6_UART_PORT_NUM, msg, strlen(msg));

    if (written < 0) {
        ESP_LOGE(TAG, "Failed to write to UART");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent command to C6: %s", msg);
    uart_wait_tx_done(C6_UART_PORT_NUM, pdMS_TO_TICKS(100));

    return ESP_OK;
}

uint32_t app_gpio_c6_read_status(void)
{
    return (s_last_status == C6_STATUS_CODE_OK) ? 1 : 0;
}

esp_err_t app_gpio_c6_register_callback(c6_status_callback_t callback, void *user_data)
{
    if (!s_uart_initialized) {
        ESP_LOGW(TAG, "UART C6 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    s_callback = callback;
    s_callback_user_data = user_data;

    ESP_LOGI(TAG, "Callback registered for C6 messages");

    return ESP_OK;
}

bool app_gpio_c6_is_ready(void)
{
    return s_c6_ready;
}

c6_status_t app_gpio_c6_get_last_status(void)
{
    return s_last_status;
}
