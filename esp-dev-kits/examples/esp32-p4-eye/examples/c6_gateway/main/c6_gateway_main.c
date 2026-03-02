/*
 * Firmware Passerelle ESP-NOW vers UART pour ESP32-C6 (Intégré au P4-EYE)
 * Ce code reçoit des données via ESP-NOW et les renvoie via UART au P4.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_mac.h"

static const char *TAG = "C6_GATEWAY";

// Configuration UART pour communiquer avec le P4
// Sur le module C6-Mini-1U, UART0 est sur GPIO16(TX) et GPIO17(RX)
// Ces pins sont connectées aux GPIO51(RX) et GPIO52(TX) du P4
#define UART_PORT_NUM      UART_NUM_0
#define UART_TX_PIN        16
#define UART_RX_PIN        17
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      1024

// Structure de données exemple (doit être identique sur le capteur émetteur)
typedef struct {
    int id;
    float temperature;
    float humidity;
} sensor_data_t;

// Initialisation de l'UART
static void init_uart(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));
}

// Callback de réception ESP-NOW
static void espnow_recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
    // Vérification basique de la taille
    if (data_len == sizeof(sensor_data_t)) {
        sensor_data_t *recv_data = (sensor_data_t *) data;
        
        // Formater le message pour le P4
        // Format: "SENSOR: <MAC> <ID> <TEMP> <HUM>"
        char buffer[128];
        int len = snprintf(buffer, sizeof(buffer), 
            "SENSOR: " MACSTR " ID:%d T:%.2f H:%.2f\n",
            MAC2STR(esp_now_info->src_addr),
            recv_data->id,
            recv_data->temperature,
            recv_data->humidity);

        // Envoyer au P4 via UART
        uart_write_bytes(UART_PORT_NUM, buffer, len);
    }
}

// Initialisation Wi-Fi (Requis pour ESP-NOW)
static void init_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Initialisation ESP-NOW
static void init_espnow(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
}

void app_main(void)
{
    // Initialisation NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialisation des périphériques
    init_uart();
    init_wifi();
    init_espnow();

    // Signaler au P4 que le C6 est prêt
    vTaskDelay(pdMS_TO_TICKS(1000));
    const char* ready_msg = "READY\n";
    uart_write_bytes(UART_PORT_NUM, ready_msg, strlen(ready_msg));

    // Boucle principale
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}