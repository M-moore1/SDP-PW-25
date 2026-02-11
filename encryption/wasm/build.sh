#!/bin/bash
# Build AES-GCM encrypt WASM module
# Requires: Emscripten SDK (emcc, emcmake, emmake) in PATH

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
OUT_DIR="../../controller-ui/public/wasm"
MBEDTLS_DIR="mbedtls"

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Check for cmake (required for mbedtls build)
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found. Install with: brew install cmake"
    exit 1
fi

# Clone mbedtls 2.28 (simpler build, no tf-psa-crypto submodule)
# If you previously cloned mbedtls 3.x and build failed, run: rm -rf mbedtls
MBEDTLS_TAG="v2.28.10"
if [ ! -d "$MBEDTLS_DIR" ]; then
    echo "Cloning mbedtls $MBEDTLS_TAG..."
    git clone --depth 1 --branch "$MBEDTLS_TAG" https://github.com/Mbed-TLS/mbedtls.git "$MBEDTLS_DIR"
fi

# Build mbedtls with Emscripten
cd "$MBEDTLS_DIR"
MBEDTLS_BUILD="build"
if [ ! -f "$MBEDTLS_BUILD/library/libmbedcrypto.a" ]; then
    echo "Building mbedtls..."
    rm -rf "$MBEDTLS_BUILD" && mkdir "$MBEDTLS_BUILD" && cd "$MBEDTLS_BUILD"
    emcmake cmake .. \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DUSE_SHARED_MBEDTLS_LIBRARY=Off \
        -DENABLE_PROGRAMS=Off \
        -DENABLE_TESTING=Off
    emmake make -j4
    cd ..
fi
cd "$SCRIPT_DIR"

MBA="$(find "$MBEDTLS_DIR" -name "libmbedcrypto.a" 2>/dev/null | head -1)"
if [ -z "$MBA" ]; then
    echo "Error: mbedtls build failed - libmbedcrypto.a not found"
    exit 1
fi

# Build our WASM module
mkdir -p "$OUT_DIR"
echo "Building aes_gcm_encrypt wasm..."
emcc -O2 -Wall -I"$MBEDTLS_DIR/include" \
    -s MODULARIZE=1 -s 'EXPORT_NAME="AesGcmEncrypt"' \
    -s EXPORTED_FUNCTIONS='["_encrypt_aes_gcm_json","_free_string","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString"]' \
    -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=0 \
    -o "$OUT_DIR/aes_gcm_encrypt.js" \
    aes_gcm_encrypt_wasm.c \
    "$MBA"

echo "Done. Output: $OUT_DIR/aes_gcm_encrypt.js and aes_gcm_encrypt.wasm"
