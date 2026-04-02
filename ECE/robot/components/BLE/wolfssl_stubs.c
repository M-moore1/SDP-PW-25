// components/BLE/wolfssl_stubs.c

#include <stdint.h>

/*
 
Stub implementations of wolfSSL session cache lock functions.
These are missing because NO_SESSION_CACHE is not being picked up
by the pre-built wolfSSL library. Since we don't use session resumption
over BLE, these stubs are safe to use.*/

int TlsSessionCacheGetAndRdLock(uint16_t row, 
                                 const void **sess,
                                 uint32_t *idx,
                                 uint8_t type) {
    (void)row;
    (void)sess;
    (void)idx;
    (void)type;
    return -1; /* always return not found */
}

int TlsSessionCacheUnlockRow(uint32_t idx) {
    (void)idx;
    return 0;
}