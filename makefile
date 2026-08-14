ifndef VERBOSE
.SILENT:
endif

COMMIT := $(shell git rev-parse HEAD 2>/dev/null || echo "unknown")
# Keep the desktop usable while compiling the Qt client (Havoc/VibeHavoc use 4).
# Faster: make client-compile JOBS=$(nproc)
JOBS   ?= 4

# ── main targets ──────────────────────────────────────────────────────────────

# full build (first time, or after wiping data/): downloads MinGW into data/
all: ts-build client-build

# recompile after code changes. does not restore data/ (MinGW, loot, DB)
rebuild: ts-compile client-compile

# ── teamserver ────────────────────────────────────────────────────────────────

ts-build:
	@ echo "[*] building teamserver"
	@ ./teamserver/Install.sh
	@ cd teamserver && GO111MODULE="on" go build \
		-ldflags="-s -w -X cmd.VersionCommit=$(COMMIT)" \
		-o ../mugen main.go
	@ sudo setcap 'cap_net_bind_service=+ep' mugen

ts-compile:
	@ echo "[*] compiling teamserver"
	@ cd teamserver && GO111MODULE="on" go build \
		-ldflags="-s -w -X cmd.VersionCommit=$(COMMIT)" \
		-o ../mugen main.go
	@ echo "[+] teamserver: mugen"

# legacy alias kept for compatibility
dev-ts-compile: ts-compile

ts-cleanup:
	@ echo "[*] teamserver cleanup"
	@ rm -rf ./teamserver/bin
	@ rm -rf ./data/loot
	@ rm -rf ./data/x86_64-w64-mingw32-cross
	@ rm -rf ./data/mugen.db
	@ rm -rf ./data/server.*
	@ rm -rf ./teamserver/.idea
	@ rm -rf ./mugen

# ── client ────────────────────────────────────────────────────────────────────

client-build:
	@ echo "[*] building client"
	@ git submodule update --init --recursive
	@ mkdir -p client/Build && cd client/Build && cmake .. -DCMAKE_BUILD_TYPE=Release
	@ if [ -d "client/Modules" ]; then echo "[*] Modules found"; else echo "[!] Modules not found - clone MugenFramework/Modules into client/Modules"; fi
	@ cmake --build client/Build -- -j$(JOBS)
	@ echo "[+] client: client/Bin/Mugen"

# recompile only changed sources (re-runs cmake if CMakeLists.txt changed, no submodule update)
client-compile:
	@ echo "[*] compiling client"
	@ mkdir -p client/Build && cd client/Build && cmake .. -DCMAKE_BUILD_TYPE=Release -Wno-dev
	@ cmake --build client/Build -- -j$(JOBS)
	@ echo "[+] client: client/Bin/Mugen"

client-cleanup:
	@ echo "[*] client cleanup"
	@ rm -rf ./client/Build
	@ rm -rf ./client/Bin/*
	@ rm -rf ./client/Data/database.db
	@ rm -rf ./client/.idea
	@ rm -rf ./client/cmake-build-debug
	@ rm -rf ./client/Mugen
	@ rm -rf ./client/Modules

# ── global cleanup ────────────────────────────────────────────────────────────

clean: ts-cleanup client-cleanup
	@ rm -rf ./data/*.db
	@ rm -rf payloads/Demon/.idea
