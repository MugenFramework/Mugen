#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tengu.h"

Parser parser_new(const uint8_t* data, size_t len) {
    Parser p;
    p.data = data;
    p.pos  = 0;
    p.len  = len;
    return p;
}

// Read 4-byte LE uint32.
uint32_t parser_u32le(Parser* p) {
    if (p->pos + 4 > p->len) return 0;
    uint32_t v = (uint32_t)p->data[p->pos]
               | ((uint32_t)p->data[p->pos+1] << 8)
               | ((uint32_t)p->data[p->pos+2] << 16)
               | ((uint32_t)p->data[p->pos+3] << 24);
    p->pos += 4;
    return v;
}

// Read LE length-prefixed string. Returns malloc'd, caller frees.
char* parser_str(Parser* p) {
    uint32_t len = parser_u32le(p);
    if (p->pos + len > p->len) return NULL;
    char* s = malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, p->data + p->pos, len);
    s[len] = '\0';
    // Skip null terminator if present in stream.
    if (len > 0 && s[len-1] == '\0')
        s[len-1] = '\0';
    p->pos += len;
    return s;
}

// Read LE length-prefixed bytes. Returns malloc'd, caller frees.
uint8_t* parser_bytes(Parser* p, size_t* out_len) {
    uint32_t len = parser_u32le(p);
    if (p->pos + len > p->len) { *out_len = 0; return NULL; }
    uint8_t* b = malloc(len);
    if (!b) { *out_len = 0; return NULL; }
    memcpy(b, p->data + p->pos, len);
    p->pos += len;
    *out_len = len;
    return b;
}
