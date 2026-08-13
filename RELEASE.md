# Release Notes

## Mugen

### v0.2 | *My Dress-Up Darling* (in progress)

**Resource Manager**

- Server-side file storage (`data/resources/`): upload executables, BOFs, and scripts to the teamserver once. Files persist across restarts and are broadcast to every operator on connect. **View → Ops → Resources** lists all stored files with Upload, Delete, and Download buttons. Supported kinds: `exe`, `bof`, `script`, `other` (auto-detected from extension).
- Download: retrieve any stored file from the teamserver to local disk via a save dialog; only the requesting operator receives the transfer.
- Overwrite protection: uploading a file whose name already exists prompts for confirmation before replacing it.
- Search bar: real-time filter on name, kind, or date.
- Reference by name: `bof`, `inline-execute`, `memfd`, and `execute-assembly` accept a bare filename (e.g. `bof who.x64.o`) - the teamserver resolves it to the full path automatically for both Tengu and Demon sessions.
- Uploader column: the resource table shows which operator uploaded each file.
- SHA-256 integrity: hash computed server-side at upload time and shown as a tooltip on the filename for out-of-band verification.
- Context menu: right-click any row for Download / Delete without requiring a prior selection.

**Teamserver**

- Networking tunnel persistence: active SOCKS5 proxies and port forward rules are saved to the SQLite database and automatically restored on server restart. Agents reconnecting after a restart will resume tunneling without manual intervention.
- Fixed TCP, DNS, and DoH listeners not surviving a server restart. TCP config was saved to the database but the restore loop had no case for it. DNS and DoH were neither saved nor restored. All three now persist correctly across restarts.
- Fixed `json.Marshal` panic in `ToMap()`: `TunnelSave` and `TunnelRemove` are function-type fields that `structs.Map()` includes in the serialization map. Functions are not JSON-serializable; they are now removed from the map before any marshal call, consistent with the other non-serializable fields (`Connection`, `SessionDir`, `JobQueue`, `Parent`).
- Task history now stores `Status`, `SentAt`, and `CompletedAt`. Existing rows migrate as `completed`. Status advances as the job leaves the queue (`sent`), when output starts (`processing`), and when a result or error comes back.
- Notes and tags are stored on the teamserver (`TS_Agents.Tags` / `Notes`), like aliases. Every operator sees the same values; they survive reconnects and server restarts. Empty save clears them. Tags are capped at 256 characters, notes at 4096.

**Client**

- Split console view: open two agent consoles side by side in a single tab, output mirrored in real time.
- Actions button: quick-action menu next to the session table filter bar. Opens Beacon Builder (floating dialog), Process List, or File Explorer for any live agent via a hover submenu.
- Session table column customization: right-click any column header to hide/show columns; drag headers to reorder; layout persists across restarts via QSettings.
- Beacon Builder one-liner: live command generator in the payload dialog, updates on every combo change, one-click copy to clipboard (Demon Exe/Dll/Shellcode + Tengu ELF).
- TLS fingerprint: SHA-256 digest of the teamserver certificate is logged to the Event Viewer on connect and stored in memory for reference.
- Multi-select and bulk actions: Ctrl+Click or Shift+Click to select multiple agents in the session table, then right-click for a bulk menu (Shell, Sleep, Kill). The Actions menu also has a Bulk Dispatch entry that opens a dialog listing all live agents with checkboxes, a free-form command input, and an Execute button.
- Networking right-click menu: right-click any live agent to open Networking -> SOCKS5 (Start.../Stop) or Networking -> Port Forward (Add.../Remove...). SOCKS5 starts a local proxy on the teamserver; port forward binds a local port and relays through the agent.
- SOCKS Manager and Port Forward Manager: Actions -> SOCKS Manager / Port Forward Manager open global views of all active tunnels across all agents, each with a Stop/Remove button per entry.
- Agent aliasing: right-click an agent -> Set Alias to give it a short human-readable name (e.g. `dc01-system`), shown in a dedicated ALIAS column next to the agent ID. Aliases are stored on the teamserver, so they are shared by every operator and survive client reconnects and server restarts. Saving an empty alias clears it. The session table filter accepts `alias:` as a search field.
- Console tab titles: when an alias is set, the agent console tab shows `[dc01-system] user/host` instead of the raw ID. Clearing the alias restores `[TU-xxxx] user/host`. The title updates live if the alias is changed while the tab is open.
- Map view removed: the geolocation map (ip-api.com) is gone from Session View. It was unused in operations and sent the operator IP to a third party.
- Task status: every command is tracked as queued / sent / processing / completed / error. The agent console shows a colour badge on the prompt (`[queued]`, `[sent]`, `[done]`, `[error]`). Duration is measured from queue time to completion.
- Tasks widget: **View → Ops → Tasks** is a live table of every task across all agents (status, agent, alias, type, operator, command, time, duration). Filter with `status:queued`, `agent:TU-`, `user:alice`, or the In progress / Completed / Error dropdown. Double-click a row to open that agent's console.
- Ops hub: **View → Ops** is a single bottom tab with an internal navbar (Screenshots, Credentials, Downloads, Resources, Tasks, Networking). The old separate View entries for Loot / Resources / Tasks / Networking are gone. Tengu `download` now shows up in Downloads like Demon.
- Notes and tags: right-click → Notes_Tags is now teamserver-side. Every operator sees the same tags (session table column) and notes; they survive reconnects and restarts. The session table filter accepts `tag:` and `notes:`.
- Session color: right-click → Color tints the row (left accent bar). The highlight is stored on the teamserver, shared by every operator, and survives reconnects and server restarts. Reset clears it.

---

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
