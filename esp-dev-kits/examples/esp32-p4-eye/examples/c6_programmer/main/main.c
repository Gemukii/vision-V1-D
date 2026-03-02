/*
 * ESP32-C6 UART Programmer
 *
 * Ce firmware transforme un ESP32-C6 en programmeur UART pour flasher
 * le C6 intégré sur la carte ESP32-P4-Eye.
 *
 * Connexions :
 *   C6 externe GPIO20 (UART1 TX) -> P4 GPIO9  (UART3 RX)
 *   C6 externe GPIO21 (UART1 RX) <- P4 GPIO8  (UART3 TX)
 *   GND commun
 *
 * Utilisation :
 *   1. Flasher ce firmware sur le C6 externe
 *   2. Connecter le C6 externe au P4 via GPIO20/21
 *   3. Le P4 doit avoir le firmware "c6_uart_bridge" modifié avec UART3
 *   4. Lancer : esptool.py --port COMx --chip esp32c6 write_flash ...
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_TO_PC      UART_NUM_0  /* USB-Serial-JTAG du C6 */
#define UART_TO_P4      UART_NUM_1  /* Vers P4 UART3 */
#define P4_TX_PIN       20          /* C6 GPIO20 -> P4 GPIO9 */
#define P4_RX_PIN       21          /* C6 GPIO21 <- P4 GPIO8 */
#define BUF_SIZE        2048

/* Tâche : PC (USB) -> P4 (UART1) */
static void usb_to_p4_task(void *arg)
{
    uint8_t buf[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_TO_PC, buf, BUF_SIZE, pdMS_TO_TICKS(10));
        if (len > 0) {
            uart_write_bytes(UART_TO_P4, buf, len);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

/* Tâche : P4 (UART1) -> PC (USB) */
static void p4_to_usb_task(void *arg)
{
    uint8_t buf[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_TO_P4, buf, BUF_SIZE, pdMS_TO_TICKS(10));
        if (len > 0) {
            uart_write_bytes(UART_TO_PC, buf, len);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void app_main(void)
{
    /* 1. Config UART0 (USB) - déjà initialisé par défaut, on reconfigure juste */
    uart_config_t uart0_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_TO_PC, &uart0_cfg);
    uart_driver_install(UART_TO_PC, BUF_SIZE, BUF_SIZE, 0, NULL, 0);

    /* 2. Config UART1 vers P4 */
    uart_config_t uart1_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_TO_P4, &uart1_cfg);
    uart_set_pin(UART_TO_P4, P4_TX_PIN, P4_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_TO_P4, BUF_SIZE, BUF_SIZE, 0, NULL, 0);

    /* 3. Attendre que le P4 soit prêt */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* 4. Envoyer le signal '!' pour dire au P4 de mettre le C6 en download mode */
    const char reset_cmd = '!';
    uart_write_bytes(UART_TO_P4, &reset_cmd, 1);
    uart_wait_tx_done(UART_TO_P4, pdMS_TO_TICKS(100));

    /* 5. Attendre que le C6 intégré entre en download mode */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* 6. Démarrer le bridge transparent */
    xTaskCreate(usb_to_p4_task, "usb2p4", 4096, NULL, 10, NULL);
    xTaskCreate(p4_to_usb_task, "p42usb", 4096, NULL, 10, NULL);

    /* Le bridge est maintenant actif - esptool peut communiquer */
}
