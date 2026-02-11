# Build AES-GCM encrypt WASM module with Emscripten
# Requires: emcc (Emscripten SDK), mbedtls source in ./mbedtls/

CC       = emcc
MBEDTLS  = mbedtls
CFLAGS   = -O2 -Wall -I$(MBEDTLS)/include
LDFLAGS  = -L$(MBEDTLS)/library -lmbedcrypto -lmbedtls

# Emscripten export flags
EMFLAGS  = -s MODULARIZE=1 -s 'EXPORT_NAME="AesGcmEncrypt"' \
           -s EXPORTED_FUNCTIONS='["_encrypt_aes_gcm_json","_free_string","_malloc","_free"]' \
           -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString"]' \
           -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=0 \

OUT_DIR  = ../../controller-ui/public/wasm
OUT_JS   = $(OUT_DIR)/aes_gcm_encrypt.js
OUT_WASM = $(OUT_DIR)/aes_gcm_encrypt.wasm

.PHONY: all clean init-mbedtls

all: $(OUT_JS)

$(OUT_JS): aes_gcm_encrypt_wasm.c $(MBEDTLS)/library/libmbedcrypto.a
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(EMFLAGS) -o $(OUT_JS) aes_gcm_encrypt_wasm.c \
		$(MBEDTLS)/library/libmbedcrypto.a

$(MBEDTLS)/library/libmbedcrypto.a: init-mbedtls
	cd $(MBEDTLS) && emmake make no_test no_programs -j4
	$(MAKE) -C $(MBEDTLS) lib

init-mbedtls:
	@if [ ! -d $(MBEDTLS) ]; then \
		echo "Cloning mbedtls..."; \
		git clone --depth 1 https://github.com/Mbed-TLS/mbedtls.git $(MBEDTLS); \
		cd $(MBEDTLS) && cmake . -DUSE_SHARED_MBEDTLS_LIBRARY=Off -DENABLE_PROGRAMS=Off -DENABLE_TESTING=Off; \
		fi

clean:
	rm -f $(OUT_JS) $(OUT_WASM)
	rm -rf $(MBEDTLS)/library/*.a
	$(MAKE) -C $(MBEDTLS) clean 2>/dev/null || true
