# Release Notes

## Mugen

### v0.1.4 | *The Fragrant Flower Blooms With Dignity*

Stability patch. Reintroduces proper per-build `HASH_KEY` randomization and fixes the full Tengu TCP pivot stack.

---

**Demon**

- Reintroduced per-build `HASH_KEY` randomization with a correct implementation: the builder now rewrites all `H_MODULE_*` and `H_FUNC_*` constants in `Defines.h` using the UTF-16LE djb2 algorithm that the agent actually uses at runtime. The v0.1.3 revert (fixed seed 5381) is superseded.

**Tengu**

- Fixed TCP pivot connect/disconnect loop: the child socket is non-blocking, but `ioctlsocket(FIONREAD)` can report the 4-byte length prefix available before the body has arrived. `TcpRecvAll` on the body then returned `WSAEWOULDBLOCK`, misinterpreted as a dead connection. Fix: use `MSG_PEEK` to read the length prefix without consuming it, re-check `FIONREAD >= 4 + FrameLen`, and only then consume and read the full frame.

**Teamserver**

- Fixed "Failed to parse Tengu registration" on TCP pivot: the `DEMON_PIVOT_TCP_CONNECT` handler was passing `AgentHdr.Data` directly to `ParseTenguRegisterRequest` without first consuming `Command` (4 bytes) and `RequestID` (4 bytes), unlike the HTTP path.
- Fixed `crypto/aes: invalid key size 0` on TCP and SMB pivot paths: `DecryptBuffer` was called unconditionally. A Tengu child has no AES key; both call sites now guard with `len(AESKey) > 0`.
- Fixed agent freezing after commands on TCP pivot: `TenguHandlePivotFrame` returned `nil` for task results. Tengu's `tcp_c2_post` always blocks on `recv()` after sending; with no response the agent stalled for 120 s (SO_RCVTIMEO). The handler now always returns a `COMMAND_NOJOB` frame so the agent unblocks immediately.

---

### v0.1.3 | *The Fragrant Flower Blooms With Dignity*

Stability patch.

---

**Demon**

- Fixed `HASH_KEY` randomization: the builder was generating a random seed per build but `Defines.h` hash constants were pre-computed with the fixed seed 5381. Every `Instance->Win32.*` pointer resolved to NULL, causing an immediate crash on any Win32 call. Reverted to fixed seed 5381 - proper per-build randomization re-introduced in v0.1.4.
- Fixed `DEMON_MAGIC_VALUE` per-run randomization: the teamserver regenerated a random magic value on every startup, making any previously-built agent unable to reconnect after a server restart. Replaced with the stable constant `0xDEADBEEF`.
- Fixed copy-paste bug in `Obf.c` line 215 (FoliageObf sleep obfuscation): the NtTestAlert return address was written to `RopBegin->Rsp` instead of `RopExitThd->Rsp`, corrupting the first APC frame and leaving the exit frame without a valid return address. FoliageObf was non-functional as a result.

---

### v0.1.2 | *The Fragrant Flower Blooms With Dignity*

- Fixed compilation failure in Demon: removed conflicting winsock headers (`winsock2.h`, `ws2tcpip.h`) from `PivotTcp.c`.

---

### v0.1 | *The Fragrant Flower Blooms With Dignity*

Initial Mugen release. Fork of Havoc (last public GPL-3.0 commit, December 2025).

---

**Core**

- Full rebrand from Havoc to Mugen
- Manga-inspired dark theme (sakura pink accent, ink black background)
- Theme switcher in the navbar - Mugen (default) and Havoc Classic (Dracula), persisted across sessions
- `make rebuild` target - recompile teamserver + client without wiping cmake cache
- Agent IDs prefixed by type: `DN-XXXXXXXX` (Demon) and `TU-XXXXXXXX` (Tengu)
- Per-build artifact randomization: `DEMON_MAGIC_VALUE`, `HASH_KEY` (djb2), and DLL export function name (`Start` -> random identifier) are all randomized at each build, making static signatures ineffective

**Client**

- Dashboard tab with live stat cards (Tengu Live, Demon Live, Dead, Total), last 8 credentials, last 8 downloads
- Map view - world map with geolocated agents, live (pink) and dead (grey) dots, hover for details
- Session table query filter - real-time, space-separated, named fields (`type:tg`, `user:root`, `health:live`, `ip:10.0`, ...)
- Session health countdown - live timer to next expected beacon, color-coded (green / red / yellow / grey)
- Session Notes & Tags - free-form annotations and comma-separated tags, visible in the session table, SQLite-persisted
- Loot Manager: Credentials tab with Add/Edit/Remove buttons in a centered bottom bar (replaces top-right Add button), inline screenshot viewer with zoom and save; credentials now persist correctly across server restarts
- Console search (Ctrl+F) - match highlighting, forward/backward navigation, match counter
- Desktop notifications - system tray icon + toast on every new agent check-in
- Explorer UI (Process List + File Explorer) available for both Demon and Tengu sessions
- Live privilege escalation detection - `whoami` on Tengu detects euid changes and updates the session table and graph in real-time
- Session graph edge labels - listener edges display `ListenerName [TYPE]` (e.g. `http01 [HTTPS]`, `beacon [DNS]`); pivot edges display `pivot`; applies to both Demon and Tengu

**Teamserver**

- Python module renamed from `havoc` to `mugen` - `import havoc` still works from any directory
- `Packer` class available globally in all scripts without import
- `mugen.RegisterTenguCommand()` - register Python commands scoped to Tengu sessions
- `mugen.RegisterCommand(..., agent="Tengu")` - alternative syntax
- `mugen.AddCredential()` - log credentials from scripts into the Loot Manager
- `mugen.Tengu(agent_id)` - Python class for Tengu sessions: `ConsoleWrite` (CONSOLE_INFO / CONSOLE_ERROR / CONSOLE_TASK) and `Command` (BOF dispatch via `TenguCmds`); fixes the crash that occurred when calling `Demon` on a Tengu session (`DemonCommands` is null for Tengu agents)

**Tengu - new Linux C2 agent**

Lightweight Linux implant written in C, no external dependencies on the target.

Transports: HTTP/HTTPS, DNS (TXT record polling), DNS-over-HTTPS (POST `/dns-query`), TCP pivot

Commands:
- Identity: `whoami` (uid/euid, groups, capabilities, sudo, TTY), `id`, `env`
- File system: `ls`, `cd`, `cat`, `download`, `upload`, `mkdir`, `rm -r`, `cp`, `chmod`, `pwd`
- Process: `ps`, `kill`
- Network recon: `netstat`, `arp`, `route`, `ifconfig` - parsed from `/proc`, no external binary
- Execution: `shell`, `inline-execute` / `bof` (ELF BOF, aliased), `memfd` (in-memory ELF via `memfd_create`)
- Credential access: `harvest` (SSH keys, cloud tokens, git, docker, kube, shadow), `procdump` (scan `/proc/<pid>/mem` without ptrace), `keylog` (raw evdev + X11 fallback)
- Persistence: `persist cron`, `persist systemd`, `persist bash`
- Tunneling: `socks5`, `rportfwd`
- Recon: `portscan` (TCP connect, single IP or CIDR, no binary), `privesc` (writable PATH dirs, `sudo -l -n`, SUID/SGID binaries in common dirs, processes with non-zero `CapEff`)
- Screenshot: `screenshot` - X11 (scrot), Wayland (grim), ImageMagick (import)
- Session info: `info` (local, no round-trip), `help`

ELF BOF loader: execute x86_64 ELF relocatable objects in-process with full BeaconAPI (`BeaconPrintf`, `BeaconOutput`, `BeaconDataParse`, `BeaconFormatAlloc`, `BeaconIsAdmin`, ...); trampoline support for PIE binaries - PLT32/PC32 relocations that exceed the 32-bit range are redirected through in-mapping stubs (`mov rax, abs64; jmp rax`), fixing SIGSEGV when BOFs call into the agent or libc symbols placed far in virtual memory

Proxy support: HTTP and SOCKS5 proxy configured at payload build time; falls back to `HTTP_PROXY` / `HTTPS_PROXY` env vars when not set; NTLM and Basic auth via libcurl `CURLAUTH_ANY`

Sleep obfuscation: XOR-encrypts the agent's own `r-x` code pages (`PROT_NONE`) during sleep intervals using a 32-byte random key per sleep; a helper thread holds pre-resolved libc pointers (`nanosleep`, `mprotect`, `sem_post`) and decrypts after the interval, making the beacon invisible to in-memory scanners between check-ins; falls back to plain `sleep()` on any setup failure

---

## Havoc - upstream history

The following is the upstream changelog from Havoc, preserved for historical reference.

### Version `0.6` | `Hierophant Green`

- Refactored/rewritten indirect syscalls (no more RX/RWX stubs)
- Proxy library loading
- Random order module and function resolving
- x86 demon implants
- Cross-process arch injection
- AMSI/ETW patching using hardware breakpoints
- Overall agent refactoring and bug fixes

### Version `0.5` | `Emperor`

- Upgraded socks4a to socks5
- Improved redirector support
- Health tab
- Working hours
- Refactored BOF loader
- Default BOFs
- Kill date
- Sleep jitter
- Kerberos native support
- Incognito find-tokens
- DLL reflective loader (Kayn)
- Refactored TS logs

### Version `0.4.1` | `The Fool`

- Socks4a proxy
- Bug fixes
- Service API vulnerability fix (found by hyperreality)

### Version `0.4` | `Silver Chariot`

- Chunked file downloading
- Threaded inline assembly execution
- Reverse port forwarding
- Discord webhooks
- SMB agent fixes
- Bug fixes

### Version `0.3` | `Hermit Purple`

- New session icons
- Lateral movement: jump-exec psexec / scshell
- Service executable payload
- Python API: `demon.ProcessCreate`

### Version `0.2` | `Magician's Red`

- shellcode execute command (self injection)
- UI/UX fixes
- Long-running job support

### Version `0.1` | `Star Platinum`

- Initial public release
