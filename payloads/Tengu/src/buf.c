#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tengu.h"

Buf* buf_new(void) {
    Buf* b = calloc(1, sizeof(Buf));
    if (!b) return NULL;
    b->cap  = 256;
    b->data = malloc(b->cap);
    b->size = 0;
    return b;
}

static void buf_grow(Buf* b, size_t need) {
    while (b->cap < b->size + need)
        b->cap *= 2;
    b->data = realloc(b->data, b->cap);
}

void buf_push_bytes(Buf* b, const uint8_t* data, size_t len) {
    buf_grow(b, len);
    memcpy(b->data + b->size, data, len);
    b->size += len;
}

void buf_push_u32be(Buf* b, uint32_t v) {
    uint8_t tmp[4] = {
        (v >> 24) & 0xFF,
        (v >> 16) & 0xFF,
        (v >>  8) & 0xFF,
        (v      ) & 0xFF,
    };
    buf_push_bytes(b, tmp, 4);
}

void buf_push_u32le(Buf* b, uint32_t v) {
    uint8_t tmp[4] = {
        (v      ) & 0xFF,
        (v >>  8) & 0xFF,
        (v >> 16) & 0xFF,
        (v >> 24) & 0xFF,
    };
    buf_push_bytes(b, tmp, 4);
}

// Push a BE length-prefixed string (for the INIT packet header sent to server).
void buf_push_str_be(Buf* b, const char* str) {
    size_t len = str ? strlen(str) : 0;
    buf_push_u32be(b, (uint32_t)len);
    if (len > 0)
        buf_push_bytes(b, (const uint8_t*)str, len);
}

void buf_free(Buf* b) {
    if (!b) return;
    free(b->data);
    free(b);
}
