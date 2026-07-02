<div align="center">
  <img src="assets/logomugen-white.png" width="180px" /><br /><br />
  <h1>無限 Mugen</h1>
  <p><i>Open-source post-exploitation C2 framework. Forked from Havoc GPL-3.0.</i></p>
  <br/>
  <a href="https://github.com/MugenFramework/Mugen/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-red" /></a>
  <a href="https://github.com/MugenFramework/Mugen/releases"><img src="https://img.shields.io/badge/version-v0.1--dev-orange" /></a>
  <img src="https://img.shields.io/badge/platform-Linux-blue" />
</div>

---

<div align="center">
  <img src="assets/Screenshots/SMB-Pivot.png" width="90%" />
  <br/><br/>
  <img src="assets/Screenshots/Multi-User.png" width="90%" />
</div>

---

## About

Mugen is a community-driven fork of [Havoc](https://github.com/HavocFramework/Havoc), the last GPL-3.0 release (December 2025). When Havoc transitioned to a proprietary commercial license in early 2026, we continued the open development under the same GPL-3.0 terms, which cannot be retroactively revoked.

**Mugen** means *infinite* (無限) in Japanese. No paywalls. No closed doors.

> Original copyright belongs to [@C5pider](https://twitter.com/C5pider) and Havoc contributors. See [CREDITS.md](CREDITS.md).

---

## Features

### Client

C++ and Qt5 desktop application.

- **Themes** - Mugen (sakura pink) and Havoc Classic (Dracula), switchable from the navbar and persisted
- **Dashboard** - live stat cards (active/dead/total sessions), last 8 credentials, last 8 downloads
- **Session table** - live health countdown with color-coded beacon status, space-separated query filter with named fields (`type:tg user:root health:live`)
- **Session notes & tags** - free-form annotations and tags visible directly in the session table, SQLite-persisted
- **Console search** - Ctrl+F with highlight, forward/backward navigation and match counter
- **Loot Manager** - credentials, screenshots (inline viewer with zoom and save), downloads in one panel
- **Map view** - world map with geolocated agents, live (pink) and dead (grey) dots
- **Desktop notifications** - tray icon + toast on every new agent check-in
- **Explorer UI** - interactive process list and file explorer for Tengu sessions

### Teamserver

Written in Go.

- HTTP/HTTPS, DNS, DNS-over-HTTPS (DoH), TCP listeners
- Payload generation (EXE, DLL, shellcode)
- Customizable C2 profiles (`.yaotl` format)
- External C2 and custom agent support via Service API WebSocket
- Python module auto-loader - all `.py` from `~/.mugen/modules/` loaded at startup

### Demon

Flagship Windows agent written in C and ASM.

- Sleep obfuscation - Ekko, Ziliean, FOLIAGE
- x64 return address spoofing
- Stack duplication during sleep
- Heap encryption
- Indirect syscalls for all Nt* APIs
- AMSI/ETW bypass via hardware breakpoints (no in-memory patching)
- Token vault with privilege filtering
- SMB pivot over named pipes
- Working hours + kill date (runtime configurable from session console)

### Tengu

Linux agent written in C. No external dependencies on the target.

**Transports:** HTTP/HTTPS, DNS (TXT record polling), DoH (POST `/dns-query`), TCP

**Application-layer encryption:** ChaCha20 on all transports post-INIT, key embedded at build time

**Commands:**
- Identity: `whoami`, `whoami /all` (uid/euid, groups, capabilities, sudo, TTY), `id`, `env`
- File system: `ls`, `cd`, `cat`, `download`, `upload`, `mkdir`, `rm -r`, `cp`, `chmod`
- Process: `ps`, `kill`
- Network recon: `netstat`, `arp`, `route`, `ifconfig` - parsed from `/proc`, no external binary
- Execution: `shell`, `inline-execute` (ELF BOF), `memfd` (in-memory ELF via `memfd_create`)
- Credential access: `harvest` (SSH keys, cloud tokens, git, docker, kube, shadow), `procdump` (scan `/proc/<pid>/mem` without ptrace), `keylog` (raw evdev + X11 fallback)
- Persistence: `persist cron`, `persist systemd`, `persist bash`
- Tunneling: `socks5`, `rportfwd`
- Screenshot: X11 (scrot), Wayland (grim), ImageMagick (import)
- Kill date + working hours - configured at payload build time

**ELF BOF loader:** execute x86_64 ELF relocatable objects in-process with full BeaconAPI (`BeaconPrintf`, `BeaconOutput`, `BeaconDataParse`, `BeaconDataExtract`, `BeaconFormatAlloc`, `BeaconIsAdmin`, ...)

### Python API

`import mugen` - full HavocFramework/Modules compatibility (`import havoc` also works from any path).

- `RegisterTenguCommand` - Python commands scoped to Tengu sessions
- `RegisterCommand(agent="Tengu")` - alternative syntax
- `Packer` available globally without import
- `mugen.AddCredential()` - log credentials from scripts into Loot Manager
- Module auto-loader from `~/.mugen/modules/`

---

## Quick Start

**Supported:** Arch Linux, Debian 10/11, Ubuntu 20.04/22.04, Kali Linux

### Dependencies

```bash
# Arch
sudo pacman -S git gcc base-devel cmake fontconfig glu gtest spdlog boost boost-libs \
    ncurses gdbm openssl readline libffi sqlite bzip2 mesa qt5-base qt5-websockets \
    python3 nasm mingw-w64-gcc

# Ubuntu / Kali
sudo apt install -y git build-essential cmake libfontconfig1 libglu1-mesa-dev libgtest-dev \
    libspdlog-dev libboost-all-dev libncurses5-dev libssl-dev libreadline-dev libffi-dev \
    libsqlite3-dev libbz2-dev mesa-common-dev qtbase5-dev qt5-qmake libqt5websockets5-dev \
    qtdeclarative5-dev golang-go python3-dev mingw-w64 nasm
```

### Build

```bash
git clone https://github.com/MugenFramework/Mugen
cd Mugen
make
```

This builds both the teamserver and the client.

```bash
# Run the teamserver
sudo ./mugen server --profile ./profiles/mugen.yaotl -v

# Run the client (separate terminal)
./mugen client
```

See [the documentation](https://mugenframework.github.io/getting-started/installation/) for the full installation guide.

---

## Documentation

- [Installation guide](https://mugenframework.github.io/getting-started/installation/)
- [Operator manual](https://mugenframework.github.io/operator/sessions/)
- [Tengu agent reference](https://mugenframework.github.io/agents/tengu/)
- [Python API](https://mugenframework.github.io/python-api/overview/)
- [Roadmap](ROADMAP.md)

---

## Contributing

Mugen is a community project. All contributions are welcome.

Please read [CONTRIBUTING.MD](CONTRIBUTING.MD) before submitting a pull request.

---

## License

GPL-3.0 - see [LICENSE](LICENSE).

Original work copyright (C) C5pider and Havoc contributors.
Mugen modifications copyright (C) Mugen contributors.

---

## Disclaimer

Mugen is intended for authorized security testing only. Users are responsible for complying with applicable laws and for obtaining proper authorization before use.
