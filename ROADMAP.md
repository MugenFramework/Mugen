# Mugen Roadmap

This document tracks planned features, improvements, and long-term goals for the Mugen framework. Items are grouped by component and roughly ordered by priority within each section. Nothing here is a commitment - it is a living document.

---

## v0.1 - *The Fragrant Flower Blooms With Dignity* (released)

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
- [x] **Loot Manager - Credentials** - credentials tab with SQLite persistence and Python API; Add/Edit/Remove buttons in a centered bottom bar; persistence fix (credentials reload correctly after server restart)
- [x] **Loot - Image Viewer** - inline viewer in the loot panel, zoom, fit, save, PNG auto-detect
- [x] **Session Notes & Tags** - free-form annotations, tags visible as column, SQLite persistence
- [x] **Python API** - `mugen` module, `RegisterTenguCommand`, `RegisterCommand(agent="Tengu")`, global `Packer`, `import havoc` compat
- [x] **`mugen.Tengu` class** - Python class for Tengu sessions: `ConsoleWrite` (INFO/ERROR/TASK) and `Command` (BOF dispatch via `TenguCmds`); prerequisite for Python modules targeting Tengu agents
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
- [x] **Session graph edge labels** - graph links display `ListenerName [TYPE]` (e.g. `http01 [HTTPS]`) for listener edges and `pivot` for SMB pivot edges; applies to both Demon and Tengu
- [x] **Tengu - Sleep obfuscation** - memory encryption during sleep, inspired by eclipse/pendulum (Ekko Linux port); encrypt rwx segments between check-ins
- [x] **Artifact strings cleanup** - remove/randomize identifiable strings (`demon.x64.dll`, visible exported function names, etc.)
- [x] **Tengu - String obfuscation** - XOR compile-time obfuscation of hardcoded strings in the Tengu binary
- [x] **Proxy support** - HTTP_PROXY / HTTPS_PROXY env awareness, NTLM proxy auth (Demon + Tengu)
- [x] **Tengu - Privilege escalation recon** - SUID/SGID scan, sudo -l, writable PATH, capabilities

---

## v0.2 - general improvements + operational capabilities

> v0.1 and v0.2 are general improvement releases. Goal: make the framework solid and operational before tackling the big items.

### Network evasion (high priority)

- [x] **Malleable C2 profile for Tengu** - configurable HTTP headers, URIs, user-agent per profile (like Demon)
- [ ] **JA3/JA3S fingerprint randomization** - rotate TLS client hello parameters to avoid static signatures. Tengu uses libcurl: this is a TLS-stack project, not a new implant command.
- [ ] **Domain fronting** - first-class `Host:` override in the profile UX. Tengu already sends custom HTTP headers; a `Host:` line in the profile is the agent-side knob. Remaining work is Demon + builder UX.

### Tengu (Linux agent)

The implant stays small: C2, filesystem, exec, pivot. New Linux tricks are ELF BOFs in **MugenFramework/Modules** (`TenguSA`), same layout as Demon's `SituationalAwareness`: Python command + `ObjectFiles/*.o`. Operators type `nsdetect`, not `bof nsdetect.x64.o`. Do not add persist / privesc / escape variants to `tengu.h`. Do not clone `whoami` / `ps` / `harvest` as BOFs.

**TenguSA (Modules)**

- [x] **TenguSA module** - `RegisterTenguCommand` + ELF BOFs. Already ships `sysinfo`, `who`, `lsmod`, `suidscan`, `capsdump`, `nsdetect`, `sshagent`.
- [x] **`ssh-list`** - inventory of `~/.ssh` (names, perms, `authorized_keys` comments, `known_hosts` hosts). Does not dump private keys (`harvest`) and does not scan agent sockets (`sshagent`).
- [x] **`cron-list`** - user crontab, `/etc/crontab`, `/etc/cron.d`, systemd user timers. Does not install persist (`persist cron`).
- [ ] **More TenguSA BOFs** as gaps show up (SSH spray, persist helpers, container escapes). Not clones of existing commands.

**Operator gaps (implant)**

- [ ] **PTY / interactive shell** - `shell` is `popen` today: no TTY, no `su` / `sudo` / `passwd`. Real PTY with stdin.
- [ ] **Long-running jobs** - a BOF or `portscan` must not freeze the beacon loop. Fork-and-run (or equivalent) so check-ins continue.
- [x] **Tengu as TCP pivot parent** - `pivot tcp listen <port>` already starts a Linux parent. Polish is docs / graph, not a new protocol.

**Lateral and persist (BOFs / first-class jump, not new `TENGU_*` persist commands)**

- [ ] **SSH jump** - use a harvested key (or agent) to drop a Tengu on another host and get a new session in the table. This is the Linux psexec. Spray stays a BOF, not a core command.
- [ ] **SSH spray BOF** - try keys/users against a list; prints hits. Does not register a session.
- [ ] **Persistence BOFs** - `LD_PRELOAD`, `/etc/profile.d`, XDG autostart. cron / systemd / bash stay in the implant (v0.1). Do not add more persist methods to `tengu.h`.
- [ ] **Container escape BOFs** - optional, separate from detection. Not in the implant (CVEs rot).

**Deploy**

- [ ] **Minimal ELF stager** - small HTTP(S) dropper that pulls the full Tengu. Today the operator drops the whole implant. (Cross-platform stager in v0.3 can reuse this.)

### Demon (Windows agent)

- [ ] **Crystal Palace loader integration** - modular evasion layer via [Crystal-Havoc](https://github.com/calv004/Crystal-Havoc) (Calvin Roth, MIT), decoupled from the Demon implant core; operators plug it in if they need it
- [ ] **Custom reflective loader** - replace the Havoc-inherited reflective loader with a clean Mugen implementation
- [ ] **PIC shellcode conversion** - full position-independent shellcode format for Demon (no import table, RIP-relative addressing); makes Demon compatible with any external loader, packer, or crypter
- [ ] **ChaCha20 application-layer encryption** - encrypt Demon frames before sending regardless of transport, matching what Tengu already has; key generated at payload build time
- [ ] **In-memory .NET assembly execution** - `execute-assembly` style, fork-and-run in a sacrificial process, output streamed back via named pipe
- [ ] **Token vault** - store multiple stolen tokens in-memory (named), switch between them without returning to the source process, list with privilege level; auto-impersonate on child spawn; `make_token` from credentials without touching LSASS

### Arsenal Kit

- [ ] **Artifact Kit** - customizable shellcode runner/loader templates (EXE, DLL, shellcode format) selectable at build time; today the builder always produces the same format
- [ ] **Sleep Mask Kit** - pluggable sleep obfuscation interface for Demon; swap the inherited Havoc implementation with a custom one per operator (Foliage, Cronos, or custom)

### Python API / Modules

- [x] **TenguSA** - Linux situational awareness in `MugenFramework/Modules` (Python + ELF BOFs), same pattern as Demon's SituationalAwareness. Further Linux tools land there, not as a second Python collection.
- [ ] **`mugen.AgentInfo(agent_id)`** - expose full session metadata to scripts
- [ ] **`mugen.OnTaskComplete(agent_id, callback)`** - callback triggered when a task completes
- [ ] **`mugen.AddContextMenu(label, handler)`** - add right-click entries from a script

### Resource Manager

- [x] **Server-side file storage** - upload executables, BOFs, and scripts to the teamserver once (`data/resources/`), list/delete from the client, reference by name in commands (`execute-assembly Rubeus`) instead of sending bytes inline every time
- [x] **Download** - retrieve any stored file from the teamserver to local disk via a save dialog; only the requesting operator receives the file
- [x] **Overwrite protection** - uploading a file whose name already exists prompts for confirmation before replacing
- [x] **Search bar** - real-time filter on name, kind, and date; useful when many files are stored
- [x] **Reference by name** - `bof`, `inline-execute`, `memfd`, and `execute-assembly` accept a bare filename; the teamserver resolves it to the full path automatically for both Tengu and Demon
- [x] **Uploader column** - tracks which operator uploaded each file; visible in the resource table
- [x] **SHA-256 integrity** - hash computed server-side at upload time, stored in DB, shown as tooltip on the filename
- [x] **Context menu** - right-click any row for Download / Delete actions without requiring a prior selection

### Client UI

- [x] **Split console view** - two agent consoles side by side
- [x] **Actions button** - quick-action menu next to the session table filter (Beacon Builder, Process List, File Explorer)
- [x] **Session table column customization** - hide/reorder columns, persist layout
- [x] **Payload builder UX** - live config preview, one-liner copy-paste generator
- [x] **Certificate pin viewer** - display teamserver TLS fingerprint in the connection dialog (the teamserver already logs `TLS SHA-256` at startup)
- [x] **Multi-select sessions + bulk actions** - select multiple agents and send a command to all (shell, sleep, kill)
- [x] **Bulk actions expansion** - extend multi-select beyond shell / sleep / kill: set tag, set alias, mark dead, sleep with jitter
- [x] **Persistent task history** - all commands and their output are stored in SQLite (TS_TaskHistory), survive restart/reconnect; dedicated History tab per agent with: output search, comment, delete
- [x] **Task status** - each task has a status (queued / sent / processing / completed / error) with timestamp and duration, visible in the agent console and as a per-agent list; operators can see whether a command is waiting on the next beacon or already done
- [x] **Tasks widget** - View → Ops → Tasks: live table of every task across all agents (status, agent, alias, operator, command, duration); filterable, double-click opens the agent console
- [x] **Ops hub** - View → Ops is a single tab with an internal navbar (Screenshots, Credentials, Downloads, Resources, Tasks, Networking); replaces the separate View entries
- [x] **Loot screenshots & downloads persistence** - stored under `data/loot/agents/`, restored to Ops when an operator connects, survive teamserver restart; timestamped folders from older runs are migrated
- [x] **Loot Download / Delete** - Ops → Screenshots and Downloads have Download (save to the operator machine) and Delete (remove from the teamserver); also in the row context menu
- [x] **File transfer progress** - progress bar and size estimate for uploads and downloads
- [x] **Agent aliasing** - assign a short human-readable name to an agent (e.g. `dc01-system`), shown in the session table alongside the ID; stored teamserver-side so every operator sees it, and it survives reconnects and server restarts
- [x] **Alias in console tab titles** - console tabs currently show `[TU-xxxx] user/host` and ignore the alias; when an alias is set, the tab should display it (e.g. `[dc01-system] user/host`)
- [x] **Notes & tags teamserver-side** - notes and tags are stored in the local client SQLite today, so other operators never see them; persist them on the teamserver like aliases, shared across operators and surviving reconnects
- [x] **Remove Map View** - drop the geolocation map (ip-api.com); unused in operations and leaks the operator IP to a third party
- [x] **Split console reuse** - split view reuses the existing session consoles so local command history, focus, and task state stay in sync
- [x] **Hide callback** - right-click Hide parks an agent out of the session table and graph without killing it; **View → Ops → Callbacks** lists every agent (including hidden) and can Show it again

### Networking (agent right-click)

- [x] **SOCKS5 proxy** - right-click an agent -> Networking -> SOCKS5 -> Start.../Stop, starts/stops a local SOCKS5 listener on the teamserver that tunnels traffic through the agent; persists across server restarts
- [x] **Port forwarding** - right-click -> Networking -> Port Forward -> Add.../Remove..., binds a local port and forwards connections to a remote host:port through the agent (reverse port forward); persists across server restarts
- [x] **SOCKS manager window** - Actions -> SOCKS Manager: global view of all active SOCKS5 tunnels across all agents (agent ID, local port), with a Stop button per entry
- [x] **Port forward manager window** - Actions -> Port Forward Manager: global view of all active port forwards across all agents (agent ID, local port, remote host:port), with a Remove button per entry

---

## v0.3 - Go API + start replacing Havoc code

> **Major goal**: begin progressively replacing the Havoc-inherited source code with 100% Mugen code. Drop Python in favor of Go for all API and modules.

### API - Python -> Go migration

- [ ] **Drop Python API** - full deprecation of the `mugen` Python module and `import havoc` compatibility
- [ ] **Go module API** - new Go API: plugins compiled as `.so` loaded dynamically by the teamserver, or native embedded modules
- [ ] **Port existing modules** - rewrite current Python modules in Go (SituationalAwareness, persistence, privesc)
- [ ] **Go plugin SDK** - documentation and stable interface for writing Mugen modules in Go

### Client UI refactor (exploratory)

- [ ] **Sidebar navigation** - replace the current mini-navbar with a persistent left sidebar for primary sections (sessions, callbacks, resource manager, loot, script manager, etc.); top navbar kept but stripped down to Mugen menu, theme toggle, and About only; improves discoverability and reduces navbar clutter - *idea to be refined*

### Client stability

- [ ] **WebSocket heartbeat** - 30-minute Qt timer that sends a ping to the teamserver to prevent idle WebSocket disconnects (reverse proxy / server-side timeout); inherited Havoc client has no keepalive mechanism
- [ ] **Null-safety in packet handler** - check that `Packager` is initialized before dispatching a received packet; a race between connect and the first server message crashes the client
- [ ] **Package memory management** - `DecodePackage` allocates with `new`; the result is never freed after `DispatchPackage` returns; fix the leak and add the delete in the handler
- [ ] **Exception wrappers in WebSocket handlers** - `binaryMessageReceived` and `connected` have no try/catch; a malformed packet or any exception in the dispatch chain crashes the client; wrap in try/catch and log instead

### Security hardening

- [ ] **Per-IP brute-force protection** - exponential backoff on failed auth attempts (per IP, not per username to avoid lockouts), auto-decay after inactivity, memory-only; closes the raw WebSocket bruteforce gap
- [ ] **Per-session unique key exchange** - replace the inherited static/fingerprinted Havoc KEX with a per-agent HMAC-based derivation + HKDF key expansion; eliminates the static network signature shared across all Demon agents

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
| v0.1 | *The Fragrant Flower Blooms With Dignity* | Released |
| v0.2 | *My Dress-Up Darling* | RC1 |
| v0.3 | TBD | Planned |
