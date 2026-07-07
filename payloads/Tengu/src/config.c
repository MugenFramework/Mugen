#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "tengu.h"

uint32_t    g_agent_id = 0;
TenguConfig g_config   = {0};

// Read 4 bytes LE from buf at offset.
static uint32_t read_le32(const uint8_t* buf, size_t off) {
    return (uint32_t)buf[off]
         | ((uint32_t)buf[off+1] << 8)
         | ((uint32_t)buf[off+2] << 16)
         | ((uint32_t)buf[off+3] << 24);
}

// Read 8 bytes LE from buf at offset.
static int64_t read_le64(const uint8_t* buf, size_t off) {
    return (int64_t)buf[off]
         | ((int64_t)buf[off+1] << 8)
         | ((int64_t)buf[off+2] << 16)
         | ((int64_t)buf[off+3] << 24)
         | ((int64_t)buf[off+4] << 32)
         | ((int64_t)buf[off+5] << 40)
         | ((int64_t)buf[off+6] << 48)
         | ((int64_t)buf[off+7] << 56);
}

// Read a LE length-prefixed string. Advances *offset. Returns malloc'd copy.
static char* read_str(const uint8_t* buf, size_t len, size_t* off) {
    if (*off + 4 > len) return NULL;
    uint32_t slen = read_le32(buf, *off);
    *off += 4;
    if (*off + slen > len) return NULL;
    char* s = malloc(slen + 1);
    if (!s) return NULL;
    memcpy(s, buf + *off, slen);
    s[slen] = '\0';
    *off += slen;
    return s;
}

// Parse CONFIG_BYTES embedded at compile time.
// Layout (LE): [sleep:4][jitter:4][transport:4][transport-specific fields...]
// transport 0 (HTTP):  [host:str][port:4][uri_count:4][uri_0:str]...[uri_N:str][secure:4][user_agent:str][header_count:4][header_0:str]...[header_N:str]
// transport 1 (DNS):   [dns_host:str][dns_port:4][dns_domain:str]
// transport 2 (DoH):   [doh_host:str][doh_port:4][doh_path:str][doh_secure:4]
void config_parse(const uint8_t* bytes, size_t len) {
    size_t off = 0;

    if (off + 4 > len) return;
    g_config.sleep_delay = (int)read_le32(bytes, off); off += 4;

    if (off + 4 > len) return;
    g_config.sleep_jitter = (int)read_le32(bytes, off); off += 4;

    if (off + 8 <= len) {
        g_config.kill_date = read_le64(bytes, off); off += 8;
    }
    if (off + 4 <= len) {
        g_config.working_hours = (int32_t)read_le32(bytes, off); off += 4;
    }

    if (off + 4 > len) return;
    g_config.transport = (int)read_le32(bytes, off); off += 4;

    /* proxy config appended after all transport-specific fields */
    if (g_config.transport == 1) {
        g_config.dns_host   = read_str(bytes, len, &off);
        if (off + 4 > len) return;
        g_config.dns_port   = (int)read_le32(bytes, off); off += 4;
        g_config.dns_domain = read_str(bytes, len, &off);
    } else if (g_config.transport == 2) {
        g_config.dns_host  = read_str(bytes, len, &off);
        if (off + 4 > len) return;
        g_config.dns_port  = (int)read_le32(bytes, off); off += 4;
        g_config.doh_path  = read_str(bytes, len, &off);
        if (off + 4 > len) return;
        g_config.doh_secure = (int)read_le32(bytes, off); off += 4;
    } else if (g_config.transport == 3) {
        g_config.pivot_host = read_str(bytes, len, &off);
        if (off + 4 > len) return;
        g_config.pivot_port = (int)read_le32(bytes, off); off += 4;
    } else {
        // HTTP
        g_config.host = read_str(bytes, len, &off);
        if (off + 4 > len) return;
        g_config.port = (int)read_le32(bytes, off); off += 4;

        // URI list
        if (off + 4 > len) return;
        g_config.uri_count = (int)read_le32(bytes, off); off += 4;
        if (g_config.uri_count > 0) {
            g_config.uris = malloc(g_config.uri_count * sizeof(char*));
            for (int i = 0; i < g_config.uri_count; i++)
                g_config.uris[i] = read_str(bytes, len, &off);
        }

        if (off + 4 > len) return;
        g_config.secure = (int)read_le32(bytes, off); off += 4;
        g_config.user_agent = read_str(bytes, len, &off);
        if (!g_config.user_agent)
            g_config.user_agent = "Mozilla/5.0";

        // Custom headers
        if (off + 4 <= len) {
            g_config.http_header_count = (int)read_le32(bytes, off); off += 4;
            if (g_config.http_header_count > 0) {
                g_config.http_headers = malloc(g_config.http_header_count * sizeof(char*));
                for (int i = 0; i < g_config.http_header_count; i++)
                    g_config.http_headers[i] = read_str(bytes, len, &off);
            }
        }
    }

    /* proxy section (all transports) */
    if (off + 4 <= len) {
        int has_proxy = (int)read_le32(bytes, off); off += 4;
        if (has_proxy && off < len) {
            g_config.proxy_host = read_str(bytes, len, &off);
            if (off + 4 <= len) { g_config.proxy_port = (int)read_le32(bytes, off); off += 4; }
            if (off + 4 <= len) { g_config.proxy_type = (int)read_le32(bytes, off); off += 4; }
            if (off + 4 <= len) {
                int has_auth = (int)read_le32(bytes, off); off += 4;
                if (has_auth) {
                    g_config.proxy_user = read_str(bytes, len, &off);
                    g_config.proxy_pass = read_str(bytes, len, &off);
                }
            }
        }
    }
}
