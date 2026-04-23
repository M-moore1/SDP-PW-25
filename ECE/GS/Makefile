CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lwolfssl

CJSON_DIR = ./cJSON-master

INCLUDES = -I./includes/json_uds \
           -I./includes/ble \
           -I./includes/cmd_parser \
           -I./includes/cmd_structure \
           -I./includes/hardware_crypto \
           -I$(CJSON_DIR)

SRCS = gs_bridge2.c \
       includes/json_uds/json_uds.c \
       includes/cmd_parser/cmd_parser.c \
       includes/ble/pmod_esp32.c \
       includes/ble/uart_queue.c \
       includes/hardware_crypto/software_cryptography.c \
       includes/ble/ble_wolfssl_client.c \
       includes/ble/ble_tls_transport.c \
       $(CJSON_DIR)/cJSON.c

TARGET = gs_bridge2.o

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(INCLUDES) $(LDFLAGS) -o $(TARGET)

ble:
	$(CC) $(INCLUDES) -fsyntax-only includes/ble/ble.c

json_uds:
	$(CC) $(INCLUDES) -fsyntax-only includes/json_uds/json_uds.c

parser:
	$(CC) $(INCLUDES) -fsyntax-only includes/cmd_parser/cmd_parser.c

clean:
	rm -f $(TARGET)