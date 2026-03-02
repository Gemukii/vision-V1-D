#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include "esp_err.h"

/**
 * Initialise le client BLE NimBLE.
 * Scanne, se connecte au serveur "Keypad-C6",
 * souscrit aux notifications et les affiche en console.
 */
esp_err_t ble_client_init(void);

#endif /* BLE_CLIENT_H */
