CC = gcc
CFLAGS = -O2 -Wall -Wextra

CJSON_DIR = ./cJSON-master

INCLUDES = -I./includes/json_uds \
           -I./includes/bt2 \
           -I./includes/cmd_parser \
           -I./includes/cmd_structure \
           -I$(CJSON_DIR)

SRCS = gs_bridge2.c \
       includes/json_uds/json_uds.c \
       includes/bt2/bt2.c \
       includes/cmd_parser/cmd_parser.c \
       $(CJSON_DIR)/cJSON.c

TARGET = gs_bridge2

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(INCLUDES) -o $(TARGET)

bt2:
	$(CC) $(INCLUDES) -fsyntax-only includes/bt2/bt2.c

json_uds:
	$(CC) $(INCLUDES) -fsyntax-only includes/json_uds/json_uds.c

parser:
	$(CC) $(INCLUDES) -fsyntax-only includes/cmd_parser/cmd_parser.c

clean:
	rm -f $(TARGET)