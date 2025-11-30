# Makefile for:
#   json_serialize  (cJSON)
#   aes_gcm_encrypt (OpenSSL AES-GCM)
#   aes_gcm_decrypt (OpenSSL AES-GCM)
#   test_aes_gcm    (tests for AES-GCM CLI tools)

# Tools
CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra -pedantic
LDFLAGS ?=

# ----- cJSON flags (prefer pkg-config; fallback) -----
CJSON_PC_CFLAGS := $(shell pkg-config --cflags libcjson 2>/dev/null)
CJSON_PC_LIBS   := $(shell pkg-config --libs   libcjson 2>/dev/null)

ifeq ($(strip $(CJSON_PC_CFLAGS)),)
  CJSON_PREFIX   := $(shell brew --prefix cjson 2>/dev/null)
  CJSON_CFLAGS   := -I$(CJSON_PREFIX)/include
  CJSON_LIBS     := -L$(CJSON_PREFIX)/lib -lcjson
else
  CJSON_CFLAGS   := $(CJSON_PC_CFLAGS)
  CJSON_LIBS     := $(CJSON_PC_LIBS)
endif

# ----- OpenSSL flags (prefer pkg-config; fallback) -----
OSSL_PC_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
OSSL_PC_LIBS   := $(shell pkg-config --libs   openssl 2>/dev/null)

ifeq ($(strip $(OSSL_PC_CFLAGS)),)
  OSSL_PREFIX    := $(shell brew --prefix openssl@3 2>/dev/null)
  OSSL_CFLAGS    := -I$(OSSL_PREFIX)/include
  OSSL_LIBS      := -L$(OSSL_PREFIX)/lib -lcrypto
else
  OSSL_CFLAGS    := $(OSSL_PC_CFLAGS)
  OSSL_LIBS      := $(OSSL_PC_LIBS)
endif

# ----- Targets -----
.PHONY: all clean test

all: json_serialize aes_gcm_encrypt aes_gcm_decrypt

json_serialize: json_serialize.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CJSON_CFLAGS) $< -o $@ $(LDFLAGS) $(CJSON_LIBS)

aes_gcm_encrypt: aes_gcm_encrypt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OSSL_CFLAGS) $< -o $@ $(LDFLAGS) $(OSSL_LIBS)

aes_gcm_decrypt: aes_gcm_decrypt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OSSL_CFLAGS) $< -o $@ $(LDFLAGS) $(OSSL_LIBS)

# ---- Test binary for AES-GCM CLI tools ----
test_aes_gcm: test_aes_gcm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS)

# Run full test suite
test: all test_aes_gcm
	./test_aes_gcm

clean:
	rm -f json_serialize aes_gcm_encrypt aes_gcm_decrypt test_aes_gcm \
	      pt.txt enc_out.txt ct.txt dec_out.txt dec_tampered.txt
