#include "Robot_BLE.h"

typedef enum {
    WAITING          = 0x00,
    START            = 0x01,
    COLLECTING       = 0x02,
    FINISH           = 0x03,
} data_retrieval_t;

device_conn_t connected_devices[MAX_DEVICES];
int num_connected = 0;
volatile bool ble_congested = false;  // tracks BLE TX congestion state

uint16_t robot_handle_table[ROBOT_IDX_NB];

static const uint16_t GATTS_SERVICE_UUID           = 0x00FF;
static const uint16_t GATTS_ROBOT_TX_UUID          = 0xFF01;  // central writes here
static const uint16_t GATTS_ROBOT_RX_UUID          = 0xFF02;  // peripheral notifies here
static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_write    = ESP_GATT_CHAR_PROP_BIT_WRITE |
                                          ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_notify   = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t robot_measurement_ccc[2] = {0x00, 0x00};

uint32_t spp_handle = 0;
QueueHandle_t ble_recieve_queue = NULL;

static uint8_t service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static struct gatts_profile_inst robot_profile_tab[ROBOT_PROFILE_NUM] = {
    [ROBOT_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

static const esp_gatts_attr_db_t gatt_db[ROBOT_IDX_NB] =
{
    // Service Declaration
    [ROBOT_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID), (uint8_t *)&GATTS_SERVICE_UUID}},

    // TX Characteristic Declaration — central writes commands to robot
    [ROBOT_IDX_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

    // TX Characteristic Value (0xFF01) — WRITE only
    [ROBOT_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_ROBOT_TX_UUID, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    // RX Characteristic Declaration — robot notifies central with responses
    [ROBOT_IDX_RX_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_notify}},

    // RX Characteristic Value (0xFF02) — NOTIFY only
    [ROBOT_IDX_RX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_ROBOT_RX_UUID, ESP_GATT_PERM_READ,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    // CCCD — subscribe to notifications on the RX characteristic
    [ROBOT_IDX_CFG] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(robot_measurement_ccc), (uint8_t *)robot_measurement_ccc}},
};

static device_conn_t *find_device_by_conn_id(uint16_t conn_id) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (connected_devices[i].conn_id == conn_id) {
            return &connected_devices[i];
        }
    }
    return NULL;
}

void robot_ble_init(){
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_LOGI(BLE_TAG, "%s init bluetooth", __func__);

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ble_recieve_queue = xQueueCreate(10, PACKET_SIZE);

    if (ble_recieve_queue == NULL) {
        ESP_LOGE(BLE_TAG, "Queue creation failed!");
        return;
    }

    for (int i = 0; i < MAX_DEVICES; i++) {
        connected_devices[i].conn_id = CONN_ID_INVALID;
        connected_devices[i].gatts_if = ESP_GATT_IF_NONE;
        connected_devices[i].notify_enabled = false;
        memset(connected_devices[i].rx_buf, 0, sizeof(connected_devices[i].rx_buf));
        connected_devices[i].rx_idx = 0;
        connected_devices[i].data_mode = WAITING;
    }
    num_connected = 0;

    esp_ble_gatt_set_local_mtu(512);

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_ROBOT_APP_ID);
}

void send_bytes_to_all(uint8_t *packet, size_t len) {
    if (ble_congested) return;  // drop if BLE TX queue is backed up
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (connected_devices[i].conn_id != CONN_ID_INVALID && connected_devices[i].notify_enabled) {
            esp_ble_gatts_send_indicate(
                connected_devices[i].gatts_if,
                connected_devices[i].conn_id,
                robot_handle_table[ROBOT_IDX_RX_VAL],
                len,
                packet,
                false
            );
        }
    }
}

void send_bytes(uint8_t *packet, size_t len){
    send_bytes_to_all(packet, len);
}

void send_string(char *txt){
    send_bytes_to_all((uint8_t *)txt, strlen(txt));
}

void send_cmd(uint8_t* pkt, int sec_lvl) {
    if(!sec_lvl){
        char hex_str[17];
        for(int i = 0; i < 8; i++){
            sprintf(hex_str + (i * 2), "%02X", pkt[i]);
        }
        send_string(hex_str);
    }else{
        uint8_t cipher_text[PACKET_SIZE] = {0};

        if(aes_gcm_encrypt_packet((const char *)pkt, cipher_text) == 0){
            send_bytes_to_all(cipher_text, sizeof(cipher_text));
        }else{
            ESP_LOGE("SEND_CMD", "Encryption FAILED");
        }
    }
}

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
                ESP_LOGI(BLE_TAG, "Advertising start successfully");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(BLE_TAG, "Advertising stopped successfully");
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

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
        {
            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_gap_config_adv_data(&adv_data);
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, ROBOT_IDX_NB, SVC_INST_ID);
            break;
        }
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        {
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                memcpy(robot_handle_table, param->add_attr_tab.handles, sizeof(robot_handle_table));
                esp_ble_gatts_start_service(robot_handle_table[ROBOT_IDX_SVC]);
            }
            break;
        }

        case ESP_GATTS_CONNECT_EVT:
        {
            ESP_LOGI(BLE_TAG, "Device connected, conn_id=%d", param->connect.conn_id);

            int slot = -1;
            for (int i = 0; i < MAX_DEVICES; i++) {
                if (connected_devices[i].conn_id == CONN_ID_INVALID) {
                    slot = i;
                    break;
                }
            }

            if (slot == -1) {
                ESP_LOGW(BLE_TAG, "No free connection slots, rejecting device");
                esp_ble_gap_disconnect(param->connect.remote_bda);
                break;
            }

            connected_devices[slot].conn_id = param->connect.conn_id;
            connected_devices[slot].gatts_if = gatts_if;
            connected_devices[slot].notify_enabled = false;
            memset(connected_devices[slot].rx_buf, 0, sizeof(connected_devices[slot].rx_buf));
            connected_devices[slot].rx_idx = 0;
            connected_devices[slot].data_mode = WAITING;
            num_connected++;

            esp_ble_gap_set_pkt_data_len(param->connect.remote_bda, 251);

            if (num_connected < MAX_DEVICES) {
                ESP_LOGI(BLE_TAG, "Slot %d/%d used, restarting advertising", num_connected, MAX_DEVICES);
                esp_ble_gap_start_advertising(&adv_params);
            } else {
                ESP_LOGI(BLE_TAG, "All %d slots full, stopping advertising", MAX_DEVICES);
            }
            break;
        }

        case ESP_GATTS_DISCONNECT_EVT:
        {
            ESP_LOGI(BLE_TAG, "Device disconnected, conn_id=%d", param->disconnect.conn_id);
            device_conn_t *dev = find_device_by_conn_id(param->disconnect.conn_id);
            if (dev) {
                dev->conn_id = CONN_ID_INVALID;
                dev->gatts_if = ESP_GATT_IF_NONE;
                dev->notify_enabled = false;
                dev->rx_idx = 0;
                dev->data_mode = WAITING;
                if (num_connected > 0) num_connected--;
            }
            ble_congested = false;  // reset on disconnect
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }

        // Congestion event — BLE TX queue full/clear feedback from stack
        case ESP_GATTS_CONGEST_EVT:
        {
            ble_congested = param->congest.congested;
            ESP_LOGW(BLE_TAG, "BLE congestion: %s", ble_congested ? "CONGESTED" : "CLEAR");
            break;
        }

        case ESP_GATTS_WRITE_EVT:
        {
            if (!param->write.is_prep) {
                device_conn_t *dev = find_device_by_conn_id(param->write.conn_id);

                if (param->write.handle == robot_handle_table[ROBOT_IDX_CFG]) {
                    uint16_t descr_value =
                        param->write.value[1] << 8 |
                        param->write.value[0];

                    if (descr_value == 0x0001) {
                        ESP_LOGI(BLE_TAG, "Notifications ENABLED (conn_id=%d)", param->write.conn_id);
                        if (dev) dev->notify_enabled = true;
                    } else if (descr_value == 0x0000) {
                        ESP_LOGI(BLE_TAG, "Notifications DISABLED (conn_id=%d)", param->write.conn_id);
                        if (dev) dev->notify_enabled = false;
                    }
                } else if (param->write.handle == robot_handle_table[ROBOT_IDX_VAL]) {
                    if (!dev) {
                        ESP_LOGE(BLE_TAG, "Write from unknown conn_id=%d", param->write.conn_id);
                        break;
                    }

                    uint16_t incoming_len = param->write.len;
                    uint8_t *incoming_data = param->write.value;

                    if (dev->rx_idx + incoming_len > sizeof(dev->rx_buf)) {
                        ESP_LOGE(BLE_TAG, "Incoming data exceeds buffer, resetting index");
                        dev->rx_idx = 0;
                        return;
                    }

                    if (!security_flag) {
                        memcpy(&dev->rx_buf[dev->rx_idx], incoming_data, incoming_len);
                        dev->rx_idx += incoming_len;
                        if (dev->rx_idx == 8) {
                            if (xQueueSend(ble_recieve_queue, (void *)dev->rx_buf, (TickType_t)0) != pdPASS) {
                                ESP_LOGW(BLE_TAG, "BT Queue full, dropping packet");
                            }
                            dev->rx_idx = 0;
                        } else if (dev->rx_idx > 8) {
                            ESP_LOGE(BLE_TAG, "Unsecure Mode - Packet Longer than 8 Bytes");
                            dev->rx_idx = 0;
                        }
                    } else if (security_flag) {
                        for (int i = 0; i < incoming_len; i++) {
                            uint8_t current_byte = incoming_data[i];

                            switch (dev->data_mode) {
                                case WAITING:
                                    if (current_byte == 0x0A) {
                                        dev->data_mode = START;
                                    }
                                    break;
                                case START:
                                    if (current_byte == 0xD0) {
                                        dev->data_mode = COLLECTING;
                                    } else {
                                        dev->data_mode = WAITING;
                                    }
                                    dev->rx_idx = 0;
                                    break;
                                case COLLECTING:
                                    if (dev->rx_idx < 156) {
                                        dev->rx_buf[dev->rx_idx] = current_byte;
                                        dev->rx_idx++;
                                    } else if (current_byte == 0xDA) {
                                        dev->rx_idx++;
                                        dev->data_mode = FINISH;
                                    } else {
                                        dev->data_mode = WAITING;
                                    }
                                    break;
                                case FINISH:
                                    if (current_byte == 0x0D) {
                                        if (xQueueSend(ble_recieve_queue, (void *)dev->rx_buf, (TickType_t)0) != pdPASS) {
                                            ESP_LOGW(BLE_TAG, "BT Queue full, dropping packet");
                                        }
                                        dev->rx_idx = 0;
                                    }
                                    dev->data_mode = WAITING;
                                    break;
                            }
                        }
                    }
                }

                if (param->write.need_rsp) {
                    esp_ble_gatts_send_response(
                        gatts_if,
                        param->write.conn_id,
                        param->write.trans_id,
                        ESP_GATT_OK,
                        NULL);
                }
            }
            break;
        }

        default:
            break;
    }
}