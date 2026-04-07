#include "Robot_BLE.h"
#include "robot_wolfssl_server.h"

static RobotWolfSslServer g_tls_server;
static bool tls_ready = false;
static bool tls_handshake_done = false;

uint16_t robot_conn_id = 0;
esp_gatt_if_t robot_gatts_if = 0;
bool device_connected = false;
bool notify_enabled = false;


bool security_flag = false;

/* NEW: track whether BLE pairing/authentication completed successfully */
bool link_authenticated = false;

/* NEW: save peer address so security procedures can reference it if needed */
esp_bd_addr_t peer_bda = {0};


//TEST

uint16_t robot_handle_table[ROBOT_IDX_NB];
static const uint16_t GATTS_SERVICE_UUID           = 0x00FF;
static const uint16_t GATTS_ROBOT_UUID             = 0xFF01;
static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t robot_measurement_ccc[2]      = {0x00, 0x00};


static const uint32_t ROBOT_FIXED_PASSKEY = 123456;


typedef enum {
    WAITING          = 0x00, 
    START            = 0x01, 
    COLLECTING       = 0x02,
    FINISH           = 0x03,
} data_retrieval_t;

uint32_t spp_handle = 0;
uint8_t rx_buf[200]; 
int rx_idx = 0;
data_retrieval_t data_collection_mode = WAITING;
QueueHandle_t ble_recieve_queue = NULL;



static uint8_t service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// Advertising Data configuration
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,  // false = this is the main advertising packet
    .include_name        = true,   // include the DEVICE_NAME in advertising
    .include_txpower     = true,   // include TX power level in advertising
    .service_uuid_len    = sizeof(service_uuid), // length of service UUID(s)
    .p_service_uuid      = service_uuid,         // pointer to service UUID array
    .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), // advertising flags: general discoverable, no BR/EDR
};

// Advertising Parameter
esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,  // Connectable Discoverable Settings -- CHECK
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // Only allow if part of whitelist
    //.peer_addr and peer_addr_type to be advertise only to only MAC  to specfic address
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

// Stores the profiles instance only need one since I only have one profile
static struct gatts_profile_inst robot_profile_tab[ROBOT_PROFILE_NUM] = {
    [ROBOT_PROFILE_APP_IDX ] = {
        .gatts_cb = gatts_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },

};

static const esp_gatts_attr_db_t gatt_db[ROBOT_IDX_NB] =
{
    // Change ESP_GATT_AUTO_RSP to ESP_GATT_RSP_BY_APP to rsp manually
    // Service Declaration
    [ROBOT_IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID), (uint8_t *)&GATTS_SERVICE_UUID}},

    /* Characteristic Declaration */
    [ROBOT_IDX_CHAR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [ROBOT_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_ROBOT_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Client Characteristic Configuration Descriptor */
    [ROBOT_IDX_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(robot_measurement_ccc), (uint8_t *)robot_measurement_ccc}},
    
};

/*
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

    esp_ble_gatt_set_local_mtu(512); 

    esp_ble_gatts_register_callback(gatts_event_handler); // SET UP PROFILE and SERVICES on INITIALIZATION
    esp_ble_gap_register_callback(gap_event_handler); // STARTS ADVERTISING ON Initilization
    esp_ble_gatts_app_register(ESP_ROBOT_APP_ID);

}
*/

void robot_ble_init() {
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

    esp_ble_gatt_set_local_mtu(512);

    /* ------------------------------------------------------------------
     * NEW: BLE security configuration
     *
     * Matches the PMOD side intent:
     * - authenticated pairing
     * - MITM protection
     * - fixed passkey = 123456
     * - 16-byte key size
     * ------------------------------------------------------------------ */

    
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_MITM_BOND; /* removed SC requirement */
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IN;                  /* ESP32 enters passkey */


    uint8_t key_size = 16;

    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key  = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_AUTHEN_REQ_MODE,
        &auth_req,
        sizeof(auth_req)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_IOCAP_MODE,
        &iocap,
        sizeof(iocap)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_MAX_KEY_SIZE,
        &key_size,
        sizeof(key_size)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_SET_INIT_KEY,
        &init_key,
        sizeof(init_key)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_SET_RSP_KEY,
        &rsp_key,
        sizeof(rsp_key)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH,
        &auth_option,
        sizeof(auth_option)));

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(
        ESP_BLE_SM_SET_STATIC_PASSKEY,
        (void *)&ROBOT_FIXED_PASSKEY,
        sizeof(uint32_t)));

    link_authenticated = false;
    memset(peer_bda, 0, sizeof(peer_bda));

    /* Register callbacks / app */
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_ROBOT_APP_ID);

    if (robot_wolfssl_server_init(&g_tls_server) != 0) {
        ESP_LOGE(BLE_TAG, "wolfSSL server init failed");
    } else {
        tls_ready = true;
        tls_handshake_done = false;
    }
}



void send_bytes(uint8_t *packet, size_t len){
    esp_ble_gatts_send_indicate(robot_gatts_if, robot_conn_id, robot_handle_table[ROBOT_IDX_VAL],
                len, packet, false); 
}

void send_string(char *txt){
    esp_ble_gatts_send_indicate(robot_gatts_if, robot_conn_id, robot_handle_table[ROBOT_IDX_VAL],
                strlen(txt), (uint8_t *)txt, false); 
}

void send_cmd(uint8_t* pkt, int sec_lvl) {
    if(!sec_lvl){
        send_bytes(pkt, 8);
    }else{
        /*
        uint8_t cipher_text[PACKET_SIZE] = {0};
        
        if(aes_gcm_encrypt_packet((const char *)pkt, cipher_text) == 0){
            ESP_LOGI("SEND_CMD", "Secure packet sent (156 bytes)");
            send_bytes(cipher_text, sizeof(cipher_text));
        }else{
            ESP_LOGE("SEND_CMD", "Encryption FAILED");
        }
        */
    }
}



/*
// GAP Callback - handles advertising start and stop
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {

        // Advertising data configured → start advertising
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;

        // Advertising started
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAG, "Advertising start failed");
            } else {
                ESP_LOGI(BLE_TAG, "Advertising start successfully");
            }
            break;

        // Advertising stopped
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(BLE_TAG, "Advertising stopped successfully");
            }
            break;

        // Connection parameter update (optional but useful log)
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
*/


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

        /* -----------------------------------------------------------
         * NEW: security events
         * ----------------------------------------------------------- */

        case ESP_GAP_BLE_SEC_REQ_EVT:
            ESP_LOGI(BLE_TAG, "Security request received; accepting");
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;

        case ESP_GAP_BLE_PASSKEY_REQ_EVT:
            ESP_LOGI(BLE_TAG, "Passkey requested; replying with fixed key");
            esp_ble_passkey_reply(param->ble_security.ble_req.bd_addr, true, ROBOT_FIXED_PASSKEY);
            break;

        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
            ESP_LOGI(BLE_TAG, "Passkey notify: %06lu",
                     (unsigned long)param->ble_security.key_notif.passkey);
            break;

        case ESP_GAP_BLE_NC_REQ_EVT:
            ESP_LOGI(BLE_TAG, "Numeric comparison request: %lu",
                     (unsigned long)param->ble_security.key_notif.passkey);
            /* Accept numeric comparison */
            esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
            break;

        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            if (param->ble_security.auth_cmpl.success) {
                ESP_LOGI(BLE_TAG, "BLE authentication complete: SUCCESS");
                ESP_LOG_BUFFER_HEX(BLE_TAG, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
                link_authenticated = true;
            } else {
                ESP_LOGE(BLE_TAG, "BLE authentication failed, reason=0x%x",
                         param->ble_security.auth_cmpl.fail_reason);
                link_authenticated = false;
            }
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

        //NEW CONNECTION EVENT
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(BLE_TAG, "Device connected, conn_id=%d", param->connect.conn_id);

            /* NEW: clear stale bond to prevent reason=0x63 auth failure */
            esp_ble_remove_bond_device(param->connect.remote_bda);
            ESP_LOGI(BLE_TAG, "Cleared stale bond for peer");



            memset(rx_buf, 0, sizeof(rx_buf));
            rx_idx = 0;
            data_collection_mode = WAITING;

            esp_ble_gap_stop_advertising();

            robot_conn_id = param->connect.conn_id;
            robot_gatts_if = gatts_if;
            device_connected = true;
            notify_enabled = false;
            link_authenticated = false;

            memcpy(peer_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));

            esp_ble_gap_set_pkt_data_len(param->connect.remote_bda, 251);

            /* Request/update connection parameters */
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.min_int = 0x06;   // 7.5 ms
            conn_params.max_int = 0x08;   // 10 ms
            conn_params.latency = 0;
            conn_params.timeout = 1000;

            esp_ble_gap_update_conn_params(&conn_params);

            /* NEW: proactively request security/encryption on the robot side too */
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);

            tls_handshake_done = false;

            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(BLE_TAG, "Device disconnected");

            device_connected = false;
            notify_enabled = false;
            link_authenticated = false;

            memset(peer_bda, 0, sizeof(peer_bda));
            memset(rx_buf, 0, sizeof(rx_buf));
            rx_idx = 0;
            data_collection_mode = WAITING;

            esp_ble_gap_start_advertising(&adv_params);

            tls_handshake_done = false;

            break;

        case ESP_GATTS_WRITE_EVT:
        {
            if (!param->write.is_prep) {
                

                /*
                ESP_LOGI(BLE_TAG, "Write event, handle=%d len=%d", 
                                param->write.handle, param->write.len);

                ESP_LOG_BUFFER_HEX(BLE_TAG, param->write.value, param->write.len);
                */

                if (param->write.handle == robot_handle_table[ROBOT_IDX_CFG]){
                    uint16_t descr_value =
                        param->write.value[1] << 8 |
                        param->write.value[0];

                    if (descr_value == 0x0001) {
                        ESP_LOGI(BLE_TAG, "Notifications ENABLED");
                        notify_enabled = true;
                    }
                    else if (descr_value == 0x0000) {
                        ESP_LOGI(BLE_TAG, "Notifications DISABLED");
                        notify_enabled = false;
                    }
                }else if (param->write.handle == robot_handle_table[ROBOT_IDX_VAL]) {
                    
                    
                        /* NEW: if security mode is enabled, require BLE auth before accepting application data. */
                        if (security_flag && !link_authenticated) {
                            ESP_LOGW(BLE_TAG, "Dropping write: link not authenticated yet");

                            if (param->write.need_rsp) {
                                esp_ble_gatts_send_response(
                                    gatts_if,
                                    param->write.conn_id,
                                    param->write.trans_id,
                                    ESP_GATT_INSUF_AUTHENTICATION,
                                    NULL);
                            }
                            break;
                        }
                    
                    
                    
                    
                    
                    uint16_t incoming_len = param->write.len;
                    uint8_t *incoming_data = param->write.value;

                    if (rx_idx + incoming_len > sizeof(rx_buf)) {
                        ESP_LOGE(BLE_TAG, "Incoming data exceeds buffer, resetting index");
                        rx_idx = 0; 
                        return;
                    }

                    if (!security_flag){
                        memcpy(&rx_buf[rx_idx], incoming_data, incoming_len);
                        rx_idx += incoming_len;
                        if (rx_idx == 8) {
                            if (xQueueSend(ble_recieve_queue, (void *)rx_buf, (TickType_t)0) != pdPASS) {
                                ESP_LOGW(BLE_TAG, "BT Queue full, dropping packet");
                            }
                            rx_idx = 0;
                        }else if (rx_idx > 8){
                            ESP_LOGE(BLE_TAG, "Unsecure Mode - Packet Longer than 8 Bytes");
                            rx_idx = 0;
                        }
                    }else if (security_flag){
                        for (int i = 0; i < incoming_len; i++){
                                uint8_t current_byte = incoming_data[i];
                                
                                switch (data_collection_mode){
                                    case WAITING:
                                        if (current_byte == 0x0A) {
                                            data_collection_mode = START;
                                        }
                                    break;
                                    case START:
                                        if (current_byte == 0xD0) {
                                            data_collection_mode = COLLECTING;
                                        }else{
                                            data_collection_mode = WAITING;
                                        }
                                        rx_idx = 0;
                                    break;
                                    case COLLECTING:
                                        if (rx_idx < 156){
                                            rx_buf[rx_idx] = current_byte;
                                            rx_idx++;
                                        }else if (current_byte == 0xDA){
                                            rx_idx++;
                                            data_collection_mode = FINISH;
                                        }else{
                                            data_collection_mode = WAITING;
                                        }
                                    break;
                                    case FINISH:
                                        if (current_byte == 0x0D) {
                                            if (xQueueSend(ble_recieve_queue, (void *)rx_buf, (TickType_t)0) != pdPASS) {
                                                ESP_LOGW(BLE_TAG, "BT Queue full, dropping packet");
                                            }
                                            rx_idx =0;
                                        }
                                        data_collection_mode = WAITING;
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