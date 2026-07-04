#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "obfstr.h"

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

#define QR(a, b, c, d) do { \
    (a) += (b); (d) ^= (a); (d) = rotl32((d), 16); \
    (c) += (d); (b) ^= (c); (b) = rotl32((b), 12); \
    (a) += (b); (d) ^= (a); (d) = rotl32((d),  8); \
    (c) += (d); (b) ^= (c); (b) = rotl32((b),  7); \
} while(0)

static void chacha20_block(uint32_t out[16], const uint32_t st[16]) {
    uint32_t x[16];
    int i;
    for (i = 0; i < 16; i++) x[i] = st[i];
    for (i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8],  x[12]);
        QR(x[1], x[5], x[9],  x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8],  x[13]);
        QR(x[3], x[4], x[9],  x[14]);
    }
    for (i = 0; i < 16; i++) out[i] = x[i] + st[i];
}

static uint32_t load_le32(const uint8_t* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

// RFC 8439 ChaCha20: key=32B, nonce=12B, counter=32-bit.
// Encrypts/decrypts buf in-place (XOR with keystream).
void chacha20_xcrypt(const uint8_t key[32], const uint8_t nonce[12],
                     uint32_t counter, uint8_t* buf, size_t len) {
    uint32_t st[16] = {
        0x61707865u, 0x3320646eu, 0x79622d32u, 0x6b206574u,
        load_le32(key),       load_le32(key +  4),
        load_le32(key +  8),  load_le32(key + 12),
        load_le32(key + 16),  load_le32(key + 20),
        load_le32(key + 24),  load_le32(key + 28),
        counter,
        load_le32(nonce),     load_le32(nonce + 4), load_le32(nonce + 8)
    };
    uint32_t block[16];
    size_t i = 0;

    while (i < len) {
        chacha20_block(block, st);
        st[12]++;
        const uint8_t* kb = (const uint8_t*)block;
        size_t j;
        for (j = 0; j < 64 && i < len; j++, i++)
            buf[i] ^= kb[j];
    }
}

// Fill out with len random bytes from /dev/urandom.
void tengu_getrandom(uint8_t* out, size_t len) {
    int fd = open(SXOR(SXOR_URANDOM), O_RDONLY);
    if (fd < 0) {
        // Fallback: deterministic garbage better than zeros.
        size_t i;
        for (i = 0; i < len; i++) out[i] = (uint8_t)(i * 0x53u ^ 0xA5u);
        return;
    }
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, out + got, len - got);
        if (n <= 0) break;
        got += (size_t)n;
    }
    close(fd);
    // Zero-fill in the (extremely unlikely) event of a short read.
    while (got < len) out[got++] = 0;
}
