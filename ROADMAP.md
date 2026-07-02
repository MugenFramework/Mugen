# Mugen Roadmap

This document tracks planned features, improvements, and long-term goals for the Mugen framework. Items are grouped by component and roughly ordered by priority within each section. Nothing here is a commitment - it is a living document.

---

## v0.1 - *The Fragrant Flower Blooms With Dignity* (in progress)

> Initial release. Establishes the framework foundation: full Havoc rebrand, new Linux agent, loot manager, Python API, Mugen UI.

- [x] **Full Havoc -> Mugen rebrand** - manga theme, name, Python module
- [x] **Themes** - Mugen theme (sakura pink) + Havoc Classic (Dracula), switchable from navbar, persisted via QSettings
- [x] **Tengu** - Linux agent in C, HTTP/HTTPS, base commands (shell, fs, ps, net, socks5)
- [x] **Tengu - Network recon** - `netstat` (/proc/net/tcp), `arp` (/proc/net/arp), `route` (/proc/net/route), no external binary
- [x] **Tengu - Screenshot** - `scrot` (X11) / `grim` (Wayland) / `import` (ImageMagick)
- [x] **Tengu - whoami** - uid/euid/gid, groups, capabilities, sudo -ln, TTY, shell
- [x] **Tengu - Keylogger** - raw `/dev/input/eventX` input, shift/caps/backspace handled
- [x] **Tengu - Port scanner** - TCP connect scan IP or CIDR, port list/range, 64 parallel conns, no external binary
- [x] **Tengu - Credential harvester** - SSH keys, history, AWS/GCP/Azure tokens, git, docker, kube, pgpass, /etc/shadow, env vars
- [x] **Tengu - Process memory scan** - `procdump`, scan /proc/pid/mem without ptrace, password/token/JWT/PEM/AKIA patterns, dedup
- [x] **Tengu - In-memory ELF execution** - `memfd`, memfd_create + execve /proc/self/fd, zero bytes written to disk
- [x] **Tengu - Persistence** - `persist cron` / `persist systemd` / `persist bash`
- [x] **Tengu - HTTP User-Agent** - configurable in payload generator, embedded at compile time
- [x] **Tengu - `info` + `help`** - local session metadata, tabular help with Python commands
- [x] **ELF BOF loader** - in-process execution of x86_64 `.o` files with full BeaconAPI
- [x] **Explorer UI for Tengu** - Process List + File Explorer in the Qt client
- [x] **Agent ID prefixes** - `DN-XXXXXXXX` for Demon, `TU-XXXXXXXX` for Tengu
- [x] **Loot Manager - Credentials** - credentials tab with SQLite persistence and Python API
- [x] **Loot - Image Viewer** - inline viewer in the loot panel, zoom, fit, save, PNG auto-detect
- [x] **Session Notes & Tags** - free-form annotations, tags visible as column, SQLite persistence
- [x] **Python API** - `mugen` module, `RegisterTenguCommand`, `RegisterCommand(agent="Tengu")`, global `Packer`, `import havoc` compat
- [x] **`make rebuild`** - recompile teamserver + client without wiping the cmake cache
- [x] **Map View** - world map, ip-api.com geolocation, live (pink) / dead (grey) dots
- [x] **Dashboard** - 4 stat cards (live/dead/total), last 8 credentials, last 8 downloads
- [x] **Session Health Countdown** - live countdown to next beacon, color-coded green/yellow/red
- [x] **Console Search** - Ctrl+F, highlight, navigation, n/total counter, Escape to close
- [x] **Session Table Query Filter** - space-separated tokens (implicit AND), named fields (type:/user:/ip:/etc.)
- [x] **Desktop Notifications** - tray icon + toast on every new agent check-in
- [x] **Tengu - Reverse port forward** - `rportfwd add/rm/list`, bidirectional internal relay without SOCKS5
- [x] **DNS C2 transport** - TXT record polling, frames base32-encoded in QNAME labels
- [x] **DoH C2 transport** - same DNS protocol over HTTPS POST `/dns-query`, Content-Type: application/dns-message
- [x] **TCP cross-platform** - Demon parent listens on TCP, child (Demon or Tengu) connects outbound; works Windows -> Linux
- [x] **ChaCha20 application-layer encryption** - encrypt Tengu frames before sending regardless of transport (HTTP, DNS, DoH, TCP); key generated at payload build time, negotiated at init with the teamserver
- [x] **Tengu - Kill date + working hours** - self-destruct after a configurable date, beacon only within a defined time window
- [x] **DEADBEEF magic bytes randomization** - replace hardcoded value at payload generation (`commands.go`, `Defines.h`)
- [x] **djb2 HASH_KEY randomization** - generate a different key per build instead of the fixed value (`Win32.h`)
- [x] **Module auto-loader** - load all `.py` from `~/.mugen/modules/` at startup
- [x] **Tengu - Live privilege escalation detection** - `whoami` updates `euid`-based username in session table and graph in real-time; triggers `UpdateSession` event on change
- [ ] **Tengu - Sleep obfuscation** - memory encryption during sleep, inspired by eclipse/pendulum (Ekko Linux port); encrypt rwx segments between check-ins
- [ ] **Artifact strings cleanup** - remove/randomize identifiable strings (`demon.x64.dll`, visible exported function names, etc.)
- [ ] **Tengu - String obfuscation** - XOR compile-time obfuscation of hardcoded strings in the Tengu binary
- [ ] **Proxy support** - HTTP_PROXY / HTTPS_PROXY env awareness, NTLM proxy auth (Demon + Tengu)
- [ ] **Hardware breakpoint AMSI/ETW bypass as default** - already in Demon, make it the default behavior instead of an opt-in
- [ ] **Tengu - Privilege escalation recon** - SUID/SGID scan, sudo -l, writable PATH, capabilities

---

## v0.2 - general improvements + network evasion

> v0.1 and v0.2 are general improvement releases. Goal: make the framework solid and operational before tackling the big items. Network evasion moves up in priority - it is a prerequisite for a serious red team tool in 2026.

### Network evasion (high priority)

- [ ] **Malleable C2 profile for Tengu** - configurable HTTP headers, URIs, user-agent per profile (like Demon)
- [ ] **JA3/JA3S fingerprint randomization** - rotate TLS client hello parameters to avoid static signatures
- [ ] **Domain fronting** - `Host:` header override per profile, for Demon and Tengu

### Tengu (Linux agent)

- [ ] **ELF BOF toolkit** - Linux equivalents of SituationalAwareness BOFs (advanced /proc enumeration, capability dump, who)
- [ ] **Advanced persistence** - `LD_PRELOAD`, `/etc/profile.d` (cron/systemd/bash already done in v0.1)
- [ ] **Lateral movement** - SSH key harvesting + spray, pivot over existing SSH session
- [ ] **Container awareness** - cgroup namespace detection, Docker socket exposure, escape primitives

### Demon (Windows agent)

- [ ] **Crystal Palace loader integration** - modular evasion layer via [Crystal-Havoc](https://github.com/calv004/Crystal-Havoc) (Calvin Roth, MIT): callstack spoofing + indirect syscalls as a swappable layer, decoupled from the Demon implant core
- [ ] **Module stomping** - overwrite the `.text` section of a legitimate DLL instead of allocating RWX memory
- [ ] **Stack spoofing** - synthetic call stack during sleep via fibers or CFG bypass
- [ ] **ETW provider unhooking** - disable per provider instead of a global patch
- [ ] **Heap encryption during sleep** - encrypt implant heap alongside stack duplication
- [ ] **Threadless injection** - overwrite return addresses or APC without `CreateRemoteThread`
- [ ] **Custom reflective loader** - replace the PE-Sieve-visible Reflective DLL with a custom loader
- [ ] **Improved PPID spoofing** - inherit handles and env in addition to parent process
- [ ] **Token vault improvements** - privilege-based filtering, auto-impersonate at spawn

### Python API / Modules

- [ ] **Linux module collection** - SituationalAwareness, persistence, privesc helpers in Python via Tengu BOF API
- [ ] **`mugen.AgentInfo(agent_id)`** - expose full session metadata to scripts
- [ ] **`mugen.OnTaskComplete(agent_id, callback)`** - callback triggered when a task completes
- [ ] **`mugen.AddContextMenu(label, handler)`** - add right-click entries from a script

### Teamserver

- [ ] **REST API** - authenticated HTTP API for external tooling (BloodHound, SIEM, custom dashboards)
- [ ] **Webhook notifications** - POST on check-in, task complete, error (Slack, Teams, custom)
- [ ] **Audit log** - structured persistent log of all operator commands and agent responses
- [ ] **Redirector support** - built-in HTTPS redirector config (auto-generate nginx/Apache rewrite rules)
- [ ] **Multi-profile listeners** - multiple HTTP/HTTPS listeners with different C2 profiles running simultaneously
- [ ] **Let's Encrypt integration** - automatic TLS certificate provisioning

### Client UI

- [ ] **Split console view** - two agent consoles side by side
- [ ] **Session table column customization** - hide/reorder columns, persist layout
- [ ] **Payload builder UX** - live config preview, one-liner copy-paste generator
- [ ] **Certificate pin viewer** - display teamserver TLS fingerprint in the connection dialog

---

## v0.3 - Go API + start replacing Havoc code

> **Major goal**: begin progressively replacing the Havoc-inherited source code with 100% Mugen code. Drop Python in favor of Go for all API and modules.

### API - Python -> Go migration

- [ ] **Drop Python API** - full deprecation of the `mugen` Python module and `import havoc` compatibility
- [ ] **Go module API** - new Go API: plugins compiled as `.so` loaded dynamically by the teamserver, or native embedded modules
- [ ] **Port existing modules** - rewrite current Python modules in Go (SituationalAwareness, persistence, privesc)
- [ ] **Go plugin SDK** - documentation and stable interface for writing Mugen modules in Go

### Progressive Havoc code replacement

- [ ] **Audit of inherited code** - identify and document all parts of the codebase that remain unmodified Havoc code
- [ ] **Teamserver refactoring** - replace inherited handlers and data structures with a 100% Mugen architecture
- [ ] **Demon agent protocol** - document and stabilize the Demon binary protocol to enable a clean reimplementation

### New agents

- [ ] **macOS agent** - Mach-O implant in C/ObjC, launchd persistence, Keychain access, TCC bypass primitives
- [ ] **Cross-platform shellcode stager** - minimal HTTP stager (~1KB) for Linux and macOS

### Infrastructure

- [ ] **Docker packaging** - `docker-compose up` to start a full teamserver + redirector stack
- [ ] **One-line install script** - `curl | bash` installer for Debian/Ubuntu/Arch/Kali
- [ ] **CI/CD pipeline** - GitHub Actions: build client, build teamserver, Go tests on every PR
- [ ] **Integration tests** - end-to-end automated: start teamserver, connect client, generate payload, verify check-in
- [ ] **Documentation site** - operator manual, Go API reference, module writing guide

### Extensibility

- [ ] **External agent SDK** - documented protocol + Go library for implementing a custom agent
- [ ] **gRPC transport** - alternative to WebSocket for client-server communication

---

## Ongoing / Maintenance

- [ ] Security audit of teamserver HTTP handler and authentication flow
- [ ] Performance: reduce client memory usage at high session count (> 500)
- [ ] Backward compatibility of `.yaotl` profiles across versions

---

## Release Names

Mugen release names are titles of romance manga.

| Version | Name | Status |
|---------|------|--------|
| v0.1 | *The Fragrant Flower Blooms With Dignity* | In progress |
| v0.2 | TBD | Planned |
| v0.3 | TBD | Planned |
