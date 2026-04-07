#include "Robot_BLE.h"

// ─── Multi-Connection State ───────────────────────────────────────────────────

#define MAX_CONNECTIONS 3  // ESP32 supports up to 3 concurrent BLE connections by default
                           // Increase CONFIG_BT_ACL_CONNECTIONS in menuconfig (up to 9) for more

typedef enum {
    WAITING    = 0x00,
    START      = 0x01,
    COLLECTING = 0x02,
    FINISH     = 0x03,
} data_retrieval_t;

typedef struct {
    uint16_t         conn_id;
    esp_gatt_if_t    gatts_if;
    bool             active;
    bool             notify_enabled;
    uint8_t          rx_buf[200];
    int              rx_idx;
    data_retrieval_t data_collection_mode;
} robot_connection_t;

static robot_connection_t connections[MAX_CONNECTIONS];

// Legacy globals kept for compatibility — updated dynamically on connect
esp_gatt_if_t robot_gatts_if = 0;
uint16_t      robot_conn_id  = 0;
bool          device_connected = false;
bool          notify_enabled   = false;

// ─── Connection Helpers ───────────────────────────────────────────────────────

static robot_connection_t *find_connection(uint16_t conn_id)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active && connections[i].conn_id == conn_id)
            return &connections[i];
    }
    return NULL;
}

static robot_connection_t *find_free_slot(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!connections[i].active)
            return &connections[i];
    }
    return NULL;
}

static int active_connection_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active) count++;
    }
    return count;
}

// ─── GATT / GAP Setup ────────────────────────────────────────────────────────

uint16_t robot_handle_table[ROBOT_IDX_NB];

static const uint16_t GATTS_SERVICE_UUID           = 0x00FF;
static const uint16_t GATTS_ROBOT_UUID             = 0xFF01;
static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t  char_prop_read_write_notify  = ESP_GATT_CHAR_PROP_BIT_WRITE |
                                                      ESP_GATT_CHAR_PROP_BIT_READ  |
                                                      ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t  robot_measurement_ccc[2]     = {0x00, 0x00};

static uint8_t service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// Advertising Data
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp     = false,
    .include_name     = true,
    .include_txpower  = true,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid   = service_uuid,
    .flag             = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// Advertising Parameters
esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t    gatts_cb;
    uint16_t          gatts_if;
    uint16_t          app_id;
    uint16_t          conn_id;
    uint16_t          service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t          char_handle;
    esp_bt_uuid_t     char_uuid;
    esp_gatt_perm_t   perm;
    esp_gatt_char_prop_t property;
    uint16_t          descr_handle;
    esp_bt_uuid_t     descr_uuid;
};

static struct gatts_profile_inst robot_profile_tab[ROBOT_PROFILE_NUM] = {
    [ROBOT_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

static const esp_gatts_attr_db_t gatt_db[ROBOT_IDX_NB] =
{
    /* Service Declaration */
    [ROBOT_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID), (uint8_t *)&GATTS_SERVICE_UUID}},

    /* Characteristic Declaration */
    [ROBOT_IDX_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [ROBOT_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_ROBOT_UUID,
      ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Client Characteristic Configuration Descriptor */
    [ROBOT_IDX_CFG] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
      ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(robot_measurement_ccc), (uint8_t *)robot_measurement_ccc}},
};

// Receive queue — each entry is a fixed-size packet; conn_id prepended so the
// consumer can identify the sender.  See ble_queue_packet_t in the header.
QueueHandle_t ble_recieve_queue = NULL;

// ─── Initialisation ───────────────────────────────────────────────────────────

void robot_ble_init(void)
{
    // Zero-initialise all connection slots
    memset(connections, 0, sizeof(connections));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_LOGI(BLE_TAG, "%s init bluetooth", __func__);

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Queue items carry a conn_id prefix + PACKET_SIZE bytes of payload
    ble_recieve_queue = xQueueCreate(10 * MAX_CONNECTIONS, sizeof(ble_queue_packet_t));
    if (ble_recieve_queue == NULL) {
        ESP_LOGE(BLE_TAG, "Queue creation failed!");
        return;
    }

    esp_ble_gatt_set_local_mtu(512);

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_ROBOT_APP_ID);
}

// ─── Send Helpers ─────────────────────────────────────────────────────────────

/* Send raw bytes to one specific connection */
void send_bytes_to(uint16_t conn_id, uint8_t *packet, size_t len)
{
    robot_connection_t *conn = find_connection(conn_id);
    if (conn && conn->active && conn->notify_enabled) {
        esp_ble_gatts_send_indicate(conn->gatts_if, conn->conn_id,
                                    robot_handle_table[ROBOT_IDX_VAL],
                                    len, packet, false);
    }
}

/* Broadcast raw bytes to ALL connected and notify-enabled devices */
void send_bytes_all(uint8_t *packet, size_t len)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active && connections[i].notify_enabled) {
            esp_ble_gatts_send_indicate(connections[i].gatts_if,
                                        connections[i].conn_id,
                                        robot_handle_table[ROBOT_IDX_VAL],
                                        len, packet, false);
        }
    }
}

/* Legacy single-target wrappers — use the first active connection */
void send_bytes(uint8_t *packet, size_t len)
{
    send_bytes_to(robot_conn_id, packet, len);
}

void send_string(char *txt)
{
    send_bytes_to(robot_conn_id, (uint8_t *)txt, strlen(txt));
}

void send_cmd(uint8_t *pkt, int sec_lvl)
{
    if (!sec_lvl) {
        send_bytes(pkt, 8);
    } else {
        /*
        uint8_t cipher_text[PACKET_SIZE] = {0};
        if (aes_gcm_encrypt_packet((const char *)pkt, cipher_text) == 0) {
            ESP_LOGI("SEND_CMD", "Secure packet sent (156 bytes)");
            send_bytes(cipher_text, sizeof(cipher_text));
        } else {
            ESP_LOGE("SEND_CMD", "Encryption FAILED");
        }
        */
    }
}

// ─── GAP Callback ─────────────────────────────────────────────────────────────

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {

        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAG, "Advertising start failed");
            } else {
                ESP_LOGI(BLE_TAG, "Advertising started (%d/%d connections active)",
                         active_connection_count(), MAX_CONNECTIONS);
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(BLE_TAG, "Advertising stopped");
            }
            break;

        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(BLE_TAG,
                     "Conn params updated: status=%d, int=%d, latency=%d, timeout=%d",
                     param->update_conn_params.status,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;

        default:
            break;
    }
}

// ─── GATTS Callback ───────────────────────────────────────────────────────────

void gatts_event_handler(esp_gatts_cb_event_t event,
                         esp_gatt_if_t        gatts_if,
                         esp_ble_gatts_cb_param_t *param)
{
    switch (event) {

        // ── Registration ──────────────────────────────────────────────────────
        case ESP_GATTS_REG_EVT:
        {
            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_gap_config_adv_data(&adv_data);
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, ROBOT_IDX_NB, SVC_INST_ID);
            break;
        }

        // ── Attribute table created ───────────────────────────────────────────
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        {
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                memcpy(robot_handle_table, param->add_attr_tab.handles,
                       sizeof(robot_handle_table));
                esp_ble_gatts_start_service(robot_handle_table[ROBOT_IDX_SVC]);
            }
            break;
        }

        // ── Connection ────────────────────────────────────────────────────────
        case ESP_GATTS_CONNECT_EVT:
        {
            robot_connection_t *conn = find_free_slot();
            if (conn == NULL) {
                ESP_LOGW(BLE_TAG, "Max connections (%d) reached — rejecting conn_id=%d",
                         MAX_CONNECTIONS, param->connect.conn_id);
                esp_ble_gatts_close(gatts_if, param->connect.conn_id);
                break;
            }

            conn->conn_id            = param->connect.conn_id;
            conn->gatts_if           = gatts_if;
            conn->active             = true;
            conn->notify_enabled     = false;
            conn->rx_idx             = 0;
            conn->data_collection_mode = WAITING;
            memset(conn->rx_buf, 0, sizeof(conn->rx_buf));

            // Keep legacy globals pointing at the most-recently connected device
            robot_conn_id  = conn->conn_id;
            robot_gatts_if = conn->gatts_if;
            device_connected = true;

            ESP_LOGI(BLE_TAG, "Device connected — conn_id=%d  (%d/%d slots used)",
                     conn->conn_id, active_connection_count(), MAX_CONNECTIONS);

            esp_ble_gap_set_pkt_data_len(param->connect.remote_bda, 251);

            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.min_int = 0x06;   // 7.5 ms
            conn_params.max_int = 0x08;   // 10 ms
            conn_params.latency = 0;
            conn_params.timeout = 1000;
            esp_ble_gap_update_conn_params(&conn_params);

            // Keep advertising so additional devices can connect
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }

        // ── Disconnection ─────────────────────────────────────────────────────
        case ESP_GATTS_DISCONNECT_EVT:
        {
            robot_connection_t *conn = find_connection(param->disconnect.conn_id);
            if (conn) {
                ESP_LOGI(BLE_TAG, "Device disconnected — conn_id=%d", conn->conn_id);
                conn->active         = false;
                conn->notify_enabled = false;
                conn->rx_idx         = 0;
                memset(conn->rx_buf, 0, sizeof(conn->rx_buf));
            }

            device_connected = (active_connection_count() > 0);

            // Always restart advertising after a disconnection
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }

        // ── Write ─────────────────────────────────────────────────────────────
        case ESP_GATTS_WRITE_EVT:
        {
            if (param->write.is_prep) break;  // Ignore prepared writes

            robot_connection_t *conn = find_connection(param->write.conn_id);
            if (!conn) {
                ESP_LOGW(BLE_TAG, "Write from unknown conn_id=%d", param->write.conn_id);
                break;
            }

            uint16_t incoming_len  = param->write.len;
            uint8_t *incoming_data = param->write.value;

            // ── CCCD write: enable / disable notifications ─────────────────
            if (param->write.handle == robot_handle_table[ROBOT_IDX_CFG]) {
                uint16_t descr_value = (param->write.value[1] << 8) |
                                        param->write.value[0];
                if (descr_value == 0x0001) {
                    ESP_LOGI(BLE_TAG, "Notifications ENABLED  (conn_id=%d)", conn->conn_id);
                    conn->notify_enabled = true;
                } else if (descr_value == 0x0000) {
                    ESP_LOGI(BLE_TAG, "Notifications DISABLED (conn_id=%d)", conn->conn_id);
                    conn->notify_enabled = false;
                }

            // ── Characteristic value write: incoming command / data ────────
            } else if (param->write.handle == robot_handle_table[ROBOT_IDX_VAL]) {

                if (conn->rx_idx + incoming_len > sizeof(conn->rx_buf)) {
                    ESP_LOGE(BLE_TAG, "Buffer overflow on conn_id=%d — resetting",
                             conn->conn_id);
                    conn->rx_idx = 0;
                    goto send_response;
                }

                if (!security_flag) {
                    // ── Unsecure mode: expect fixed 8-byte packets ─────────
                    memcpy(&conn->rx_buf[conn->rx_idx], incoming_data, incoming_len);
                    conn->rx_idx += incoming_len;

                    if (conn->rx_idx == 8) {
                        ble_queue_packet_t qpkt;
                        qpkt.conn_id = conn->conn_id;
                        memcpy(qpkt.data, conn->rx_buf, PACKET_SIZE);

                        if (xQueueSend(ble_recieve_queue, &qpkt, 0) != pdPASS) {
                            ESP_LOGW(BLE_TAG, "BLE queue full — dropping packet from conn_id=%d",
                                     conn->conn_id);
                        }
                        conn->rx_idx = 0;

                    } else if (conn->rx_idx > 8) {
                        ESP_LOGE(BLE_TAG, "Unsecure mode — packet >8 bytes on conn_id=%d",
                                 conn->conn_id);
                        conn->rx_idx = 0;
                    }

                } else {
                    // ── Secure mode: framed 156-byte packets ──────────────
                    for (int i = 0; i < incoming_len; i++) {
                        uint8_t b = incoming_data[i];

                        switch (conn->data_collection_mode) {
                            case WAITING:
                                if (b == 0x0A)
                                    conn->data_collection_mode = START;
                                break;

                            case START:
                                conn->rx_idx = 0;
                                if (b == 0xD0)
                                    conn->data_collection_mode = COLLECTING;
                                else
                                    conn->data_collection_mode = WAITING;
                                break;

                            case COLLECTING:
                                if (conn->rx_idx < 156) {
                                    conn->rx_buf[conn->rx_idx++] = b;
                                } else if (b == 0xDA) {
                                    conn->rx_idx++;
                                    conn->data_collection_mode = FINISH;
                                } else {
                                    conn->data_collection_mode = WAITING;
                                }
                                break;

                            case FINISH:
                                ESP_LOGI(BLE_TAG, "Secure mode — checking final byte (conn_id=%d)",
                                         conn->conn_id);
                                if (b == 0x0D) {
                                    ble_queue_packet_t qpkt;
                                    qpkt.conn_id = conn->conn_id;
                                    memcpy(qpkt.data, conn->rx_buf, PACKET_SIZE);

                                    if (xQueueSend(ble_recieve_queue, &qpkt, 0) != pdPASS) {
                                        ESP_LOGW(BLE_TAG,
                                                 "BLE queue full — dropping secure packet from conn_id=%d",
                                                 conn->conn_id);
                                    }
                                    conn->rx_idx = 0;
                                }
                                conn->data_collection_mode = WAITING;
                                break;
                        }
                    }
                }
            }

            send_response:
            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if,
                                            param->write.conn_id,
                                            param->write.trans_id,
                                            ESP_GATT_OK,
                                            NULL);
            }
            break;
        }

        default:
            break;
    }
}