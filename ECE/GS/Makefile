CC = gcc
CFLAGS = -O2 -Wall -Wextra
LIBS = -lcrypto

# ---- Include paths ----
INCLUDES_COMMON = -I./includes/json_uds \
                  -I./includes/cmd_structure \
                  -I./includes/crypto \
                  -I./newcJSON

INCLUDES_BT2 = -I./includes/bt2 \
               -I./includes/cmd_parser

INCLUDES_BLE = -I./includes/ble \
               -I./includes/cmd_parser_ble

# ---- Common sources (used by both) ----
SRCS_COMMON = includes/json_uds/json_uds.c \
              includes/crypto/aes_gcm_blob.c \
              includes/crypto/gs_key.c \
              ./newcJSON/cJSON.c

# ---- BT2 target ----
SRCS_BT2 = gs_bridge2.c \
           includes/bt2/bt2.c \
           includes/cmd_parser/cmd_parser.c

TARGET_BT2 = gs_bridge2

# ---- BLE target ----
SRCS_BLE = gs_bridgeble.c \
           includes/ble/ble.c \
           includes/cmd_parser_ble/cmd_parser_ble.c

TARGET_BLE = gs_bridgeble

.PHONY: all bt2 ble clean

all: $(TARGET_BT2) $(TARGET_BLE)

$(TARGET_BT2): $(SRCS_COMMON) $(SRCS_BT2)
	$(CC) $(CFLAGS) $(SRCS_COMMON) $(SRCS_BT2) $(INCLUDES_COMMON) $(INCLUDES_BT2) $(LIBS) -o $(TARGET_BT2)

$(TARGET_BLE): $(SRCS_COMMON) $(SRCS_BLE)
	$(CC) $(CFLAGS) $(SRCS_COMMON) $(SRCS_BLE) $(INCLUDES_COMMON) $(INCLUDES_BLE) $(LIBS) -o $(TARGET_BLE)

bt2: $(TARGET_BT2)
ble: $(TARGET_BLE)

clean:
	rm -f $(TARGET_BT2) $(TARGET_BLE)