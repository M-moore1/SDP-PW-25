#include "pmod_esp32.h"
#include "uart_queue.h"
#include "ble_tls_transport.h"

volatile int BLE_CONNECTED = 0;


static BleTlsTransport *g_ble_tls_transport = NULL;

void pmod_attach_tls_transport(BleTlsTransport *t) {
    g_ble_tls_transport = t;
}

void pmod_on_ble_notification(const uint8_t *data, int len) {
    if (!g_ble_tls_transport || !data || len <= 0) return;
    ble_tls_transport_feed_rx(g_ble_tls_transport, data, (size_t)len);
}


long get_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000L) + (ts.tv_nsec / 1000000L);
}

int uart_open_config(const char *dev, speed_t baud) {
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK); // Open UART nonblocking
  if (fd < 0) {                                       // If open failed
    perror("open uart");                              // Print reason (errno)
    return -1;                                        // Error
  }

  struct termios tio;                                 // termios structure
  memset(&tio, 0, sizeof(tio));                        // Clear it

  if (tcgetattr(fd, &tio) != 0) {                      // Read current UART settings
    perror("tcgetattr");                               // Print error
    close(fd);                                         // Close fd
    return -1;                                         // Error
  }

  cfsetispeed(&tio, baud);                             // Set input baud
  cfsetospeed(&tio, baud);                             // Set output baud

  tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8;          // 8 data bits
  tio.c_cflag |= (CLOCAL | CREAD);                     // Local (ignore modem), enable receiver
  tio.c_cflag &= ~(PARENB | PARODD);                   // No parity
  tio.c_cflag &= ~CSTOPB;                              // 1 stop bit
  tio.c_cflag &= ~CRTSCTS;                             // No HW flow control

  tio.c_iflag = IGNPAR;                                // Ignore framing/parity errors (v1)
  tio.c_oflag = 0;                                     // Raw output
  tio.c_lflag = 0;                                     // Raw input (no canonical mode)

  tcflush(fd, TCIFLUSH);                               // Flush any stale input bytes
  if (tcsetattr(fd, TCSANOW, &tio) != 0) {              // Apply settings immediately
    perror("tcsetattr");                               // Print error
    close(fd);                                         // Close fd
    return -1;                                         // Error
  }

  return fd;                                           // Return UART fd
}

int gpio_export(int gpio_num) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);

    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 0;  
    }

    FILE *exp = fopen("/sys/class/gpio/export", "w");
    if (!exp) return -1;
    fprintf(exp, "%d", gpio_num);
    fclose(exp);

    usleep(200000);  

    f = fopen(path, "r");
    if (!f) return -1;
    fclose(f);
    return 0;
}

int gpio_set_direction(int gpio_num, const char *dir) {
    char path[64];

    if (gpio_export(gpio_num) != 0) return -1;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "%s", dir);
    fclose(f);
    return 0;
}

int gpio_write(int gpio_num, uint32_t value) {
    char path[64];
    char dir[8] = {0};

    // Check direction only write to outputs
    snprintf(path, sizeof(path),"/sys/class/gpio/gpio%d/direction", gpio_num);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    fscanf(f, "%7s", dir);
    fclose(f);
    if (strncmp(dir, "out", 3) != 0) return -1;

    // Write value
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
    f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "%u", value ? 1 : 0);
    fclose(f);
    return 0;
}

uint32_t gpio_read(int gpio_num) {
    char path[64];
    uint32_t val = 0xFFFFFFFF;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);

    FILE *f = fopen(path, "r");
    if (!f) return 0xFFFFFFFF;

    fscanf(f, "%u", &val);
    fclose(f);
    return val;
}

int send_at_cmd(int uart_fd, const char *cmd, const char *prefix, char *out_value, int timeout_ms) {
    char buffer[1024];
    int bytes_received = 0;
    long start_time = get_now_ms();
    
    memset(buffer, 0, sizeof(buffer));
    write(uart_fd, cmd, strlen(cmd));

    while (get_now_ms() - start_time < timeout_ms) {
        int n = uart_read_and_queue(uart_fd, buffer + bytes_received, sizeof(buffer) - bytes_received - 1);
        if (n > 0) {
            bytes_received += n;
            buffer[bytes_received] = '\0';

            if (prefix != NULL && out_value != NULL) {
                char *data_ptr = strstr(buffer, prefix);
                if (data_ptr) {
                    sscanf(data_ptr + strlen(prefix), "%s", out_value);
                }
            }

            if (strstr(buffer, "OK")) return 0;
            if (strstr(buffer, "ERROR")) {
                //printf("[AT ERROR] cmd: %s response: %s\n", cmd, buffer);
                return -1;
            }
        }
        usleep(5); 
    }
    return -2; 
}

int ble_write(int uart_fd, int srv, int chr, int desc, uint8_t *data, int len) {
    if (!BLE_CONNECTED) return -1;

    char cmd[64];
    if (desc >= 0) {
        snprintf(cmd, sizeof(cmd), "AT+BLEGATTCWR=0,%d,%d,%d,%d\r\n", srv, chr, desc, len);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+BLEGATTCWR=0,%d,%d,,%d\r\n", srv, chr, len);
    }

    char buffer[256];
    int bytes_received = 0;
    long start_time = get_now_ms();
    memset(buffer, 0, sizeof(buffer));

    write(uart_fd, cmd, strlen(cmd));
    while (get_now_ms() - start_time < 3000) {
        int n = uart_read_and_queue(uart_fd, buffer + bytes_received, sizeof(buffer) - bytes_received - 1);
        if (n > 0) {
            bytes_received += n;
            buffer[bytes_received] = '\0';
            if (strstr(buffer, ">")) break;
            if (strstr(buffer, "ERROR")) return -1;
        }
        usleep(5);
    }
    if (!strstr(buffer, ">")) return -2;

    write(uart_fd, data, len);
    return send_at_cmd(uart_fd, "", NULL, NULL, 3000);
}

int pmod_esp32_reset(int uart_fd) {
    char response[256];
    int timeout_ms = 3000; 
    int elapsed_ms = 0;
    int interval_ms = 10;

    if (gpio_write(PMOD_0_RST, 0) != 0) return -1;
    usleep(200000);
    if (gpio_write(PMOD_0_RST, 1) != 0) return -1;

    while (elapsed_ms < timeout_ms) {
        

        if (uart_read_and_queue(uart_fd, response, sizeof(response)) > 0) {
            if (strstr(response, "ready") != NULL) {
                BLE_CONNECTED = 0;
                return 0;
            }
        }

        usleep(interval_ms * 1000);
        elapsed_ms += interval_ms;
    }

    printf("[ESP32] Reset timeout - no ready response\n");
    return -1;
}


int pmod_esp32_init(int uart_fd) {
    int ret = 0;
    uart_queue_init(&uart_queue);

    // Set directions
    ret |= gpio_set_direction(PMOD_0_GPIO_0,  "out");
    ret |= gpio_set_direction(PMOD_0_GPIO_1,  "out");
    ret |= gpio_set_direction(PMOD_0_RST,     "out");
    ret |= gpio_set_direction(PMOD_0_MODE,  "out");

    if (ret != 0) return -1;

    
    gpio_write(PMOD_0_RST,    1);   
    gpio_write(PMOD_0_MODE, 0);   
    gpio_write(PMOD_0_GPIO_0, 0);
    gpio_write(PMOD_0_GPIO_1, 0);

    if (pmod_esp32_reset(uart_fd) < 0) return -1;

    return send_at_cmd(uart_fd, "ATE0\r\n", NULL, NULL, 50);

}


int pmod_name(int uart_fd, const char *set_name, char *out_name) {
    int ret;

    if (set_name != NULL) {
        char cmd_buffer[128];
        snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+BLENAME=\"%s\"\r\n", set_name);

        ret = send_at_cmd(uart_fd, cmd_buffer, NULL, NULL, 1000);
        if (ret != 0) {
            return ret; 
        }
    }

    if (out_name != NULL){
        return send_at_cmd(uart_fd, "AT+BLENAME?\r\n", "+BLENAME:", out_name, 1000);
    }

    return 0;
}


int set_pmod_name(int uart_fd, const char *dev_name){
    char cmd_buffer[128];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+BLENAME=\"%s\"\r\n", dev_name);
    return send_at_cmd(uart_fd, cmd_buffer, NULL, NULL, 1000);
}


int ble_discon(int uart_fd){
    BLE_CONNECTED = 0;

    return send_at_cmd(uart_fd, "AT+BLEDISCONN=0\r\n", NULL, NULL, 1000);
}

int ble_notification(int uart_fd, int enable) {
    if (BLE_CONNECTED == 0) return -1;

    uint8_t enable_cccd[2]  = {0x01, 0x00};
    uint8_t disable_cccd[2] = {0x00, 0x00};

    if (!enable) {
        return ble_write(uart_fd, 3, 1, 1, disable_cccd, 2);
    }
    return ble_write(uart_fd, 3, 1, 1, enable_cccd, 2);
}

int ble_connect(int uart_fd, const char *MAC) {
    if (MAC == NULL) {
        MAC = ESP32_MAC;
    }

    // Disconnect
    if (BLE_CONNECTED) ble_discon(uart_fd);

    char cmd_buffer[1024];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+BLECONN=%d,\"%s\"\r\n", CONN_IDX, MAC);
    int results = send_at_cmd(uart_fd, cmd_buffer, NULL, NULL, 10000);

    if (results < 0) {
        BLE_CONNECTED = 0;
        return -1;
    }

    
    usleep(500000); 
    BLE_CONNECTED = 1;
//------------------------------------------------------------------

    // Request authenticated encrypted link with MITM protection
    if (send_at_cmd(uart_fd, "AT+BLEENC=0,3\r\n", NULL, NULL, 3000) < 0) return -1;

    // Wait here for +BLEAUTHCMPL:0,0 before continuing
    if (wait_for_ble_auth_complete(uart_fd, 10000) < 0) {
        BLE_CONNECTED = 0;
        return -1;
    }

//------------------------------------------------------------------


    if (send_at_cmd(uart_fd, "AT+BLEDATALEN=0,251\r\n", NULL, NULL, 2000) < 0) return -1;  // Set Data Length
    if (send_at_cmd(uart_fd, "AT+BLECFGMTU=0,512\r\n", NULL, NULL, 2000) < 0) return -1;   // Set MTU
    if (send_at_cmd(uart_fd, "AT+BLEGATTCPRIMSRV=0\r\n", NULL, NULL, 5000) < 0) return -1; // Get BLE Connection Service and makes index
    if (send_at_cmd(uart_fd, "AT+BLEGATTCCHAR=0,3\r\n", NULL, NULL, 10000) < 0) return -1; // Gets the Robot custom Service 
    if (ble_notification(uart_fd, 1) < 0) return -1;                                       // Enable Robot Custom service notifications

    
    return 0;
}

int get_name(int uart_fd, char *name){
    if (!BLE_CONNECTED) return -1;
    return send_at_cmd(uart_fd, "AT+BLENAME?\r\n", "+BLENAME:", name, 1000);
}

int get_mac(int uart_fd, char *connected_mac) {
    if (!BLE_CONNECTED) return -1;
    return send_at_cmd(uart_fd, "AT+BLECONN?\r\n", "+BLECONN:0,", connected_mac, 1000);
}

int get_rssi(int uart_fd, char *rssi_out) {
    if (!BLE_CONNECTED) return -1;
    return send_at_cmd(uart_fd, "AT+BLERDRSSI=0\r\n", "+BLERDRSSI:0,", rssi_out, 1000);
}

int get_ble_conn_params(int uart_fd, char *params_out) {
    if (!BLE_CONNECTED) return -1;
    return send_at_cmd(uart_fd, "AT+BLECONNPARAM?\r\n", "+BLECONNPARAM:", params_out, 1000);
}


int ble_init(int uart_fd) {
    ble_discon(uart_fd);

    if (pmod_esp32_init(uart_fd) < 0) return -1;  
    usleep(500000);

    // Wifi off
    if (send_at_cmd(uart_fd, "AT+CWMODE=0\r\n", NULL, NULL, 1000) < 0) return -1;
    usleep(100000);

    // BLE init
    if (send_at_cmd(uart_fd, "AT+BLEINIT=1\r\n", NULL, NULL, 1000) < 0) return -1;
    usleep(100000);
//---------------------------------------------------------

    //Enforce a stronger BLE strategy
    /* CHANGE 13 to 5*/
    if (send_at_cmd(uart_fd, "AT+BLESECPARAM=5,4,16,3,3,1\r\n", NULL, NULL, 1000) < 0) return -1;
    if (send_at_cmd(uart_fd, "AT+BLESETKEY=123456\r\n", NULL, NULL, 1000) < 0) return -1;

//---------------------------------------------------------

    // Set Device Name
    if (pmod_name(uart_fd, PMOD_DEV_NAME, NULL) < 0) return -1;

    //send_at_cmd(uart_fd, "AT+GMR\r\n", NULL, NULL, 1000);

    return 0;
}



int ble_send_pkt(int uart_fd, uint8_t *data, int data_len) {
    if (data_len != PAYLOAD_BYTES) return -1;
    if (!BLE_CONNECTED) return -1;

    uint8_t packet[PACKET_BYTES];
    packet[0] = 0x0A;
    packet[1] = 0xD0;
    memcpy(packet + 2, data, PAYLOAD_BYTES);
    packet[158] = 0xDA;
    packet[159] = 0x0D;

    return ble_write(uart_fd, 3, 1, -1, packet, PACKET_BYTES);
}

int ble_send_instruction(int uart_fd, uint8_t instruction[8]) {
    return ble_write(uart_fd, 3, 1, -1, instruction, 8);
    /*
    uint8_t payload[PAYLOAD_BYTES];
    memset(payload, 0x00, PAYLOAD_BYTES);
    memcpy(payload, instruction, 8);
    return ble_send_pkt(uart_fd, payload, PAYLOAD_BYTES);
    */
}

//----------------------------------------------
//Helper Function to wait for auth
int wait_for_ble_auth_complete(int uart_fd, int timeout_ms) {
    char buffer[1024];
    int bytes_received = 0;
    long start_time = get_now_ms();

    memset(buffer, 0, sizeof(buffer));

    while (get_now_ms() - start_time < timeout_ms) {
        int n = uart_read_and_queue(uart_fd,
                                    buffer + bytes_received,
                                    sizeof(buffer) - bytes_received - 1);
        if (n > 0) {
            bytes_received += n;
            buffer[bytes_received] = '\0';

            // If peer requests pairing, accept it
            if (strstr(buffer, "+BLESECREQ:0")) {
                if (send_at_cmd(uart_fd, "AT+BLEENCRSP=0,1\r\n", NULL, NULL, 1000) < 0) {
                    return -1;
                }
                memset(buffer, 0, sizeof(buffer));
                bytes_received = 0;
                continue;
            }

            // If a passkey is requested/notified, reply with the same key
            if (strstr(buffer, "+BLESECNTFYKEY:0,")) {
                // For a static passkey example:
                if (send_at_cmd(uart_fd, "AT+BLEKEYREPLY=0,123456\r\n", NULL, NULL, 1000) < 0) {
                    return -1;
                }
                memset(buffer, 0, sizeof(buffer));
                bytes_received = 0;
                continue;
            }

            if (strstr(buffer, "+BLEAUTHCMPL:0,0")) return 0; // success
            if (strstr(buffer, "+BLEAUTHCMPL:0,1")) return -1; // failure
            if (strstr(buffer, "ERROR")) return -1;
        }

        usleep(5000);
    }

    return -2; // timeout
}
//-----------------------------------------------------------------------------