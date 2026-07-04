#ifndef OBFSTR_H
#define OBFSTR_H

#include <stddef.h>

// SXOR_KEY and string defines (SXOR_PROC_MAPS, SXOR_URANDOM, ...) are
// generated per-build by the teamserver builder into obfstr_data.h,
// then included via gcc -include. If not set, obfuscation is a no-op.
#ifndef SXOR_KEY
#define SXOR_KEY 0x00
#endif

static inline char* _sxdec(char* s, size_t n) {
    for (size_t i = 0; i < n; i++) s[i] ^= (unsigned char)SXOR_KEY;
    return s;
}

// SXOR: decrypt a pre-XOR'd string literal on first use.
// The static local is decrypted in-place exactly once per call site.
// Uses GCC statement expressions and __COUNTER__ for unique statics.
#define _SXOR_I(cnt, ...) \
    __extension__({ \
        static char _sx##cnt[] = { __VA_ARGS__, 0 }; \
        static char _sd##cnt  = 0; \
        if (!_sd##cnt) { _sxdec(_sx##cnt, sizeof(_sx##cnt) - 1); _sd##cnt = 1; } \
        _sx##cnt; \
    })
#define SXOR(...) _SXOR_I(__COUNTER__, __VA_ARGS__)

#endif // OBFSTR_H
