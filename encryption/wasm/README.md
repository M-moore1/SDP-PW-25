# AES-GCM Encrypt WebAssembly

Emscripten-compiled AES-256-GCM encryption, compatible with `aes_gcm_encrypt.c` / `aes_gcm_decrypt.c` wire format.

## Build

**Prerequisites:**
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- [CMake](https://cmake.org/) — `brew install cmake` on macOS

```bash
# From encryption/wasm/
./build.sh
```

Output: `controller-ui/public/wasm/aes_gcm_encrypt.js` and `aes_gcm_encrypt.wasm`

## Usage

1. Build the WASM module (see above)
2. Set `VITE_ENCRYPTION_KEY` in `.env` (64 hex chars = 32-byte key)
3. Messages are encrypted before sending over WebSocket

## Wire Format

Encrypted payload sent to robot:

```json
{
  "nonce": "24_hex_chars",
  "ct": "ciphertext_hex",
  "tag": "32_hex_chars"
}
```

The robot can decrypt with `aes_gcm_decrypt` using the same key, nonce, and tag.

## C Implementation

Uses mbedtls (not OpenSSL) for WASM portability—same AES-256-GCM algorithm and output format.
