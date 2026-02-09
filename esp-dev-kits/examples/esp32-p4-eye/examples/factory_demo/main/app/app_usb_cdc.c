/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_usb_cdc.h"

static const char *TAG = "app_usb_cdc";
static bool s_cdc_initialized = false;
static bool s_lora_ready = false;
static TaskHandle_t s_rx_task_handle = NULL;

// Callback when CDC line state changes (DTR/RTS)
static void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    if (event->type == CDC_EVENT_LINE_STATE_CHANGED) {
        bool dtr = event->line_state_changed_data.dtr;
        bool rts = event->line_state_changed_data.rts;
        ESP_LOGI(TAG, "Line state changed: DTR=%d, RTS=%d", dtr, rts);
    }
}

// USB RX task - receives data from LoRaWAN device
static void usb_cdc_rx_task(void *arg)
{
    uint8_t rx_buf[128];

    ESP_LOGI(TAG, "USB CDC RX task started, waiting for READY from LoRaWAN device");

    while (1) {
        if (tud_cdc_connected() && tud_cdc_available()) {
            // Read available data
            uint32_t rx_size = tud_cdc_read(rx_buf, sizeof(rx_buf) - 1);

            if (rx_size > 0) {
                // Null-terminate the string
                rx_buf[rx_size] = '\0';

                // Log received data
                ESP_LOGI(TAG, "[USB 2.0 RX] %s", (char*)rx_buf);

                // Check if "READY" is in the received data
                if (strstr((char*)rx_buf, "READY") != NULL) {
                    if (!s_lora_ready) {
                        s_lora_ready = true;
                        ESP_LOGI(TAG, "LoRaWAN device is READY - face detection messages will now be sent");
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}

esp_err_t app_usb_cdc_init(void)
{
    ESP_LOGI(TAG, "Initializing USB CDC");

    // Configure TinyUSB CDC
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 512,
        .callback_rx = NULL,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = tinyusb_cdc_line_state_changed_callback,
        .callback_line_coding_changed = NULL
    };

    // Install CDC ACM (assumes tinyusb_driver_install was called previously)
    esp_err_t ret = tusb_cdc_acm_init(&acm_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CDC ACM: %s", esp_err_to_name(ret));
        return ret;
    }

    s_cdc_initialized = true;
    ESP_LOGI(TAG, "USB CDC initialized successfully");
    ESP_LOGI(TAG, "USB 2.0 port should now appear as a COM port on your computer");

    // Start USB RX task to receive "READY" from LoRaWAN device
    xTaskCreatePinnedToCore(
        usb_cdc_rx_task,
        "USB_CDC_RX",
        4096,
        NULL,
        5,
        &s_rx_task_handle,
        0
    );

    return ESP_OK;
}

bool app_usb_cdc_is_ready(void)
{
    if (!s_cdc_initialized) {
        return false;
    }

    // Check if USB is mounted and terminal is connected
    return tud_cdc_connected();
}

esp_err_t app_usb_cdc_send_message(const char *message)
{
    if (!s_cdc_initialized) {
        ESP_LOGW(TAG, "USB CDC not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!app_usb_cdc_is_ready()) {
        // Not connected yet, skip silently
        return ESP_OK;
    }

    size_t len = strlen(message);
    size_t written = 0;

    // Try to write data
    written = tud_cdc_write(message, len);

    if (written > 0) {
        tud_cdc_write_flush();
    }

    if (written != len) {
        ESP_LOGW(TAG, "Only wrote %d of %d bytes", written, len);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

bool app_usb_cdc_is_lora_ready(void)
{
    return s_lora_ready;
}
