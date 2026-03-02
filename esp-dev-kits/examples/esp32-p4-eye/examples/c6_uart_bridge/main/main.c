/*
 * ESP32-P4 UART Bridge pour flasher le C6 integre au P4-Eye
 *
 * IMPORTANT: Le bridge utilise le driver USB-Serial-JTAG.
 * Le secondary console est desactive dans sdkconfig.defaults
 * pour eviter que les logs polluent le USB-Serial-JTAG.
 *
 * Procedure :
 *   1. idf.py -p COM7 flash
 *   2. Attendre 10s, ouvrir un terminal serie sur COM7 a 115200
 *      -> Vous devez voir "BRIDGE_READY"
 *   3. Fermer le terminal, puis lancer esptool
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"

#define TAG "C6_BRIDGE"

/* Connexions P4 -> C6 intégré sur le P4-Eye */
#define UART_C6_PORT      UART_NUM_2
#define UART_C6_TX        52    /* P4 TX -> C6 GPIO17 (RX) */
#define UART_C6_RX        51    /* P4 RX -> C6 GPIO16 (TX) */
#define C6_RST_PIN        54    /* P4 GPIO54 -> C6 RST */
#define C6_BOOT_PIN       53    /* P4 GPIO53 -> C6 GPIO9 (BOOT) */

/* Connexions P4 -> C6 externe (programmeur) */
#define UART_PROG_PORT    UART_NUM_3
#define UART_PROG_TX      8     /* P4 TX -> C6 externe GPIO21 (RX) */
#define UART_PROG_RX      9     /* P4 RX -> C6 externe GPIO20 (TX) */

#define BUF_SIZE          2048

/* ------------------------------------------------------------------ */
/*  Met le C6 en mode download UART                                   */
/* ------------------------------------------------------------------ */
static void c6_enter_download_mode(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << C6_RST_PIN) | (1ULL << C6_BOOT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* BOOT LOW = download mode (strapping GPIO9 du C6) */
    gpio_set_level(C6_BOOT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Reset pulse */
    gpio_set_level(C6_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(C6_RST_PIN, 1);

    /* Attendre que le ROM bootloader du C6 demarre */
    vTaskDelay(pdMS_TO_TICKS(500));
}

/* ------------------------------------------------------------------ */
/*  Tache : USB (PC) -> UART (C6)                                    */
/* ------------------------------------------------------------------ */
static void usb_to_uart_task(void *arg)
{
    uint8_t buf[BUF_SIZE];
    while (1) {
        int len = usb_serial_jtag_read_bytes(buf, BUF_SIZE, pdMS_TO_TICKS(10));
        if (len > 0) {
            uart_write_bytes(UART_C6_PORT, buf, len);
        } else {
            /* Pas de donnees - yield au scheduler pour eviter watchdog */
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Tache : UART (C6) -> USB (PC)                                    */
/* ------------------------------------------------------------------ */
static void uart_to_usb_task(void *arg)
{
    uint8_t buf[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_C6_PORT, buf, BUF_SIZE, pdMS_TO_TICKS(10));
        if (len > 0) {
            usb_serial_jtag_write_bytes(buf, len, pdMS_TO_TICKS(1000));
        } else {
            /* Pas de donnees - yield au scheduler pour eviter watchdog */
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    /* Les ESP_LOGI vont sur UART0 (invisible sur COM7) */
    ESP_LOGI(TAG, "=========================================");
    ESP_LOGI(TAG, "  P4-Eye UART Bridge -> C6 Flash Tool");
    ESP_LOGI(TAG, "=========================================");

    /* 1. Attendre que l'USB se stabilise apres le boot */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* 2. Init UART2 vers C6 */
    uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_C6_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_C6_PORT, UART_C6_TX, UART_C6_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_C6_PORT, BUF_SIZE, BUF_SIZE,
                                        0, NULL, 0));
    ESP_LOGI(TAG, "UART2 init OK");

    /* 3. Mettre le C6 en download mode */
    ESP_LOGI(TAG, "Mise du C6 en mode download...");
    c6_enter_download_mode();

    /* 4. Lire ce que le C6 envoie (debug - ROM bootloader output) */
    {
        uint8_t rx[256];
        int total = 0;
        for (int i = 0; i < 10; i++) {
            int len = uart_read_bytes(UART_C6_PORT, rx, sizeof(rx) - 1,
                                      pdMS_TO_TICKS(100));
            if (len > 0) {
                rx[len] = '\0';
                ESP_LOGI(TAG, "C6 ROM output: %s", (char *)rx);
                total += len;
            }
        }
        if (total == 0) {
            ESP_LOGW(TAG, "Aucune reponse du C6 - verifier les connexions");
        } else {
            ESP_LOGI(TAG, "C6 a envoye %d octets", total);
        }
    }

    /* Vider les buffers UART */
    uart_flush_input(UART_C6_PORT);

    /* 5. Installer le driver USB-Serial-JTAG */
    ESP_LOGI(TAG, "Installation driver USB-Serial-JTAG...");
    usb_serial_jtag_driver_config_t usb_cfg = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };
    esp_err_t ret = usb_serial_jtag_driver_install(&usb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ECHEC usb_serial_jtag_driver_install: %s",
                 esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Driver USB-Serial-JTAG OK");

    /* 6. Vider les residus USB RX */
    {
        uint8_t trash[256];
        while (usb_serial_jtag_read_bytes(trash, sizeof(trash),
                                           pdMS_TO_TICKS(200)) > 0) {
            /* discard */
        }
    }

    /* 7. Envoyer un message de confirmation sur USB */
    const char *ready = "BRIDGE_READY\r\n";
    usb_serial_jtag_write_bytes((const uint8_t *)ready, strlen(ready),
                                pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Bridge pret - lancement des taches");

    /* 8. Demarrer le bridge */
    xTaskCreate(usb_to_uart_task, "usb2uart", 4096, NULL,
                configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(uart_to_usb_task, "uart2usb", 4096, NULL,
                configMAX_PRIORITIES - 1, NULL);

    ESP_LOGI(TAG, "Bridge actif !");
}
