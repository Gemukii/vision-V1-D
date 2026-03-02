#include "ble_client.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>
#include <inttypes.h>

#define TAG          "BLE_CLIENT"
#define TARGET_NAME  "Keypad-C6"

/* ------------------------------------------------------------------ */
/*  UUIDs identiques au serveur                                       */
/* ------------------------------------------------------------------ */
static const ble_uuid128_t svc_uuid = BLE_UUID128_INIT(
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
    0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t chr_uuid = BLE_UUID128_INIT(
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
    0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

/* ------------------------------------------------------------------ */
/*  Variables d'etat                                                  */
/* ------------------------------------------------------------------ */
static uint16_t g_conn_handle      = BLE_HS_CONN_HANDLE_NONE;
static uint16_t g_chr_val_handle   = 0;
static uint16_t g_svc_start_handle = 0;
static uint16_t g_svc_end_handle   = 0;

/* Declarations anticipees */
static void start_scan(void);
static int  gap_event_cb(struct ble_gap_event *event, void *arg);

/* ------------------------------------------------------------------ */
/*  Decouverte GATT : descripteurs (CCCD)                             */
/* ------------------------------------------------------------------ */
static int on_dsc_discovered(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             uint16_t chr_val_handle,
                             const struct ble_gatt_dsc *dsc,
                             void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        return 0;
    }
    if (error->status != 0) {
        ESP_LOGE(TAG, "Erreur decouverte descripteurs: %d", error->status);
        return 0;
    }

    /* CCCD = UUID 0x2902 */
    if (ble_uuid_cmp(&dsc->uuid.u, BLE_UUID16_DECLARE(0x2902)) == 0) {
        ESP_LOGI(TAG, "CCCD trouve (handle=%d), activation notifications...",
                 dsc->handle);
        uint8_t val[2] = {0x01, 0x00}; /* enable notifications */
        int rc = ble_gattc_write_flat(conn_handle, dsc->handle,
                                      val, sizeof(val), NULL, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Erreur ecriture CCCD: %d", rc);
        } else {
            ESP_LOGI(TAG, "Notifications activees !");
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Decouverte GATT : caracteristiques                                */
/* ------------------------------------------------------------------ */
static int on_chr_discovered(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             const struct ble_gatt_chr *chr,
                             void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        if (g_chr_val_handle != 0) {
            ESP_LOGI(TAG, "Recherche des descripteurs...");
            ble_gattc_disc_all_dscs(conn_handle,
                                    g_chr_val_handle,
                                    g_svc_end_handle,
                                    on_dsc_discovered, NULL);
        } else {
            ESP_LOGW(TAG, "Caracteristique TX non trouvee");
        }
        return 0;
    }
    if (error->status != 0) {
        ESP_LOGE(TAG, "Erreur decouverte caracteristiques: %d", error->status);
        return 0;
    }

    if (ble_uuid_cmp(&chr->uuid.u, &chr_uuid.u) == 0) {
        g_chr_val_handle = chr->val_handle;
        ESP_LOGI(TAG, "Caracteristique TX trouvee (val_handle=%d)",
                 g_chr_val_handle);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Decouverte GATT : services                                        */
/* ------------------------------------------------------------------ */
static int on_svc_discovered(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             const struct ble_gatt_svc *svc,
                             void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        if (g_svc_start_handle != 0) {
            ESP_LOGI(TAG, "Service trouve, recherche caracteristiques...");
            ble_gattc_disc_chrs_by_uuid(conn_handle,
                                        g_svc_start_handle,
                                        g_svc_end_handle,
                                        &chr_uuid.u,
                                        on_chr_discovered, NULL);
        } else {
            ESP_LOGW(TAG, "Service cible non trouve sur le serveur");
        }
        return 0;
    }
    if (error->status != 0) {
        ESP_LOGE(TAG, "Erreur decouverte services: %d", error->status);
        return 0;
    }

    if (ble_uuid_cmp(&svc->uuid.u, &svc_uuid.u) == 0) {
        g_svc_start_handle = svc->start_handle;
        g_svc_end_handle   = svc->end_handle;
        ESP_LOGI(TAG, "Service UART trouve (handles %d-%d)",
                 g_svc_start_handle, g_svc_end_handle);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Scan BLE                                                          */
/* ------------------------------------------------------------------ */
static void start_scan(void)
{
    struct ble_gap_disc_params disc_params = {0};
    disc_params.passive           = 0;  /* scan actif */
    disc_params.filter_duplicates = 1;
    disc_params.itvl              = 0;  /* defaut */
    disc_params.window            = 0;  /* defaut */

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 10000 /* 10s */,
                          &disc_params, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erreur demarrage scan: %d", rc);
    } else {
        ESP_LOGI(TAG, "Scan BLE demarre, recherche de '%s'...", TARGET_NAME);
    }
}

/* ------------------------------------------------------------------ */
/*  Gestionnaire d'evenements GAP                                     */
/* ------------------------------------------------------------------ */
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {

    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields,
                                         event->disc.data,
                                         event->disc.length_data);
        if (rc != 0) break;

        if (fields.name != NULL && fields.name_len > 0 &&
            fields.name_len == strlen(TARGET_NAME) &&
            memcmp(fields.name, TARGET_NAME, fields.name_len) == 0)
        {
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Appareil '%s' detecte !", TARGET_NAME);
            ESP_LOGI(TAG, "  Adresse: %02x:%02x:%02x:%02x:%02x:%02x",
                     event->disc.addr.val[5], event->disc.addr.val[4],
                     event->disc.addr.val[3], event->disc.addr.val[2],
                     event->disc.addr.val[1], event->disc.addr.val[0]);
            ESP_LOGI(TAG, "  RSSI: %d dBm", event->disc.rssi);
            ESP_LOGI(TAG, "  Tentative de connexion...");
            ESP_LOGI(TAG, "========================================");

            ble_gap_disc_cancel();

            rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC,
                                 &event->disc.addr,
                                 5000,   /* timeout 5s */
                                 NULL,   /* params par defaut */
                                 gap_event_cb, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "Erreur connexion: %d", rc);
                start_scan();
            }
        }
        break;
    }

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "CONNECTE a '%s' (handle=%d)", TARGET_NAME, g_conn_handle);
            ESP_LOGI(TAG, "  Decouverte des services GATT...");
            ESP_LOGI(TAG, "========================================");

            /* Reinitialiser et lancer la decouverte de services */
            g_svc_start_handle = 0;
            g_svc_end_handle   = 0;
            g_chr_val_handle   = 0;

            ble_gattc_disc_svc_by_uuid(g_conn_handle, &svc_uuid.u,
                                       on_svc_discovered, NULL);
        } else {
            ESP_LOGW(TAG, "Connexion echouee (status=%d), relance scan",
                     event->connect.status);
            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            start_scan();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "DECONNECTE de '%s' (raison=%d)", TARGET_NAME,
                 event->disconnect.reason);
        ESP_LOGW(TAG, "  Relance du scan...");
        ESP_LOGW(TAG, "========================================");
        g_conn_handle    = BLE_HS_CONN_HANDLE_NONE;
        g_chr_val_handle = 0;
        start_scan();
        break;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "Scan termine sans resultat, relance...");
        if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
            start_scan();
        }
        break;

    case BLE_GAP_EVENT_NOTIFY_RX: {
        /* === RECEPTION DES NOTIFICATIONS === */
        uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);
        char buf[256];
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1;
        }
        os_mbuf_copydata(event->notify_rx.om, 0, len, buf);
        buf[len] = '\0';

        ESP_LOGI(TAG, "----------------------------------------");
        ESP_LOGI(TAG, "BLE RX [%d octets] handle=%" PRIu16 ": %s",
                 len, event->notify_rx.attr_handle, buf);
        ESP_LOGI(TAG, "----------------------------------------");
        break;
    }

    default:
        break;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Callbacks NimBLE                                                  */
/* ------------------------------------------------------------------ */
static void ble_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erreur adresse BLE: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "Stack BLE prete");
    start_scan();
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "Stack BLE reset (raison=%d)", reason);
    g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
}

static void ble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ------------------------------------------------------------------ */
/*  Init publique                                                     */
/* ------------------------------------------------------------------ */
esp_err_t ble_client_init(void)
{
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init() echoue: %d", ret);
        return ESP_FAIL;
    }

    ble_hs_cfg.sync_cb  = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    ble_svc_gap_init();

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE client initialise, recherche de '%s'...", TARGET_NAME);
    return ESP_OK;
}
