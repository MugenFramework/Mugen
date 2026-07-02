# Release Notes

## Mugen

### v0.1 | *The Fragrant Flower Blooms With Dignity*

Initial Mugen release. Fork of Havoc (last public GPL-3.0 commit, December 2025).

---

**Core**

- Full rebrand from Havoc to Mugen
- Manga-inspired dark theme (sakura pink accent, ink black background)
- Theme switcher in the navbar - Mugen (default) and Havoc Classic (Dracula), persisted across sessions
- `make rebuild` target - recompile teamserver + client without wiping cmake cache
- Agent IDs prefixed by type: `dm-XXXXXXXX` (Demon) and `tg-XXXXXXXX` (Tengu)

**Client**

- Dashboard tab with live stat cards (Tengu Live, Demon Live, Dead, Total), last 8 credentials, last 8 downloads
- Map view - world map with geolocated agents, live (pink) and dead (grey) dots, hover for details
- Session table query filter - real-time, space-separated, named fields (`type:tg`, `user:root`, `health:live`, `ip:10.0`, ...)
- Session health countdown - live timer to next expected beacon, color-coded (green / red / yellow / grey)
- Session Notes & Tags - free-form annotations and comma-separated tags, visible in the session table, SQLite-persisted
- Loot Manager: Credentials tab (add/delete, full SQLite persistence), inline screenshot viewer with zoom and save
- Console search (Ctrl+F) - match highlighting, forward/backward navigation, match counter
- Desktop notifications - system tray icon + toast on every new agent check-in
- Explorer UI (Process List + File Explorer) available for both Demon and Tengu sessions

**Teamserver**

- Python module renamed from `havoc` to `mugen` - `import havoc` still works from any directory
- `Packer` class available globally in all scripts without import
- `mugen.RegisterTenguCommand()` - register Python commands scoped to Tengu sessions
- `mugen.RegisterCommand(..., agent="Tengu")` - alternative syntax
- `mugen.AddCredential()` - log credentials from scripts into the Loot Manager

**Tengu - new Linux C2 agent**

Lightweight Linux implant written in C, no external dependencies on the target.

Transports: HTTP/HTTPS, DNS (TXT record polling), DNS-over-HTTPS (POST `/dns-query`), TCP pivot

Commands:
- Identity: `whoami` (uid/euid, groups, capabilities, sudo, TTY), `id`, `env`
- File system: `ls`, `cd`, `cat`, `download`, `upload`, `mkdir`, `rm -r`, `cp`, `chmod`, `pwd`
- Process: `ps`, `kill`
- Network recon: `netstat`, `arp`, `route`, `ifconfig` - parsed from `/proc`, no external binary
- Execution: `shell`, `inline-execute` (ELF BOF), `memfd` (in-memory ELF via `memfd_create`)
- Credential access: `harvest` (SSH keys, cloud tokens, git, docker, kube, shadow), `procdump` (scan `/proc/<pid>/mem` without ptrace), `keylog` (raw evdev + X11 fallback)
- Persistence: `persist cron`, `persist systemd`, `persist bash`
- Tunneling: `socks5`, `rportfwd`
- Recon: `portscan` (TCP connect, single IP or CIDR, no binary)
- Screenshot: `screenshot` - X11 (scrot), Wayland (grim), ImageMagick (import)
- Session info: `info` (local, no round-trip), `help`

ELF BOF loader: execute x86_64 ELF relocatable objects in-process with full BeaconAPI (`BeaconPrintf`, `BeaconOutput`, `BeaconDataParse`, `BeaconFormatAlloc`, `BeaconIsAdmin`, ...)

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
