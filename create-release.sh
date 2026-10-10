#!/bin/bash
set -euo pipefail

# Publish the v0.2-rc1 GitHub pre-release from develop.
# Run AFTER chore/v0.2-rc1 is merged. Does not touch main.

TAG="My-Dress-Up-Darling-v0.2-rc1"
TITLE="My Dress-Up Darling - v0.2-rc1"
TARGET="develop/v0.2/my-dress-up-darling"
ZIP="Mugen-v0.2-rc1.zip"

git checkout "$TARGET"
git pull https://github.com/MugenFramework/Mugen.git "$TARGET"

git archive --format=zip --prefix=Mugen/ -o "/tmp/${ZIP}" HEAD

gh release create "$TAG" \
    --repo MugenFramework/Mugen \
    --target "$TARGET" \
    --prerelease \
    --title "$TITLE" \
    --notes "$(cat <<'EOF'
# My Dress-Up Darling — v0.2-rc1

First pre-release of v0.2. This is a **release candidate**, not the final cut.

v0.1 asked whether Mugen could exist as more than an archived Havoc fork. v0.2 asks whether two operators can run an engagement without fighting the client.

**If operators need to share it, it lives on the teamserver. If it is clutter, it leaves the session table without dying.**

Rebuild **teamserver and client** together. An old client will not see the same working set (hidden agents, aliases, loot) as the operator next to you.

---

## Shared state

Aliases, notes, tags, session color, hidden flag, task history, resources, loot, SOCKS5 and port forwards are stored on the teamserver and broadcast to every operator. They survive reconnects and server restarts.

## Hide is not Dead, Dead is not Exit

- **Hide** — implant keeps beaconing, leaves the session table and graph. Opposite is **Show**.
- **Dead** — still in the table, greyed out.
- **Exit** — kill the implant.

**View → Ops** opens on **Callbacks**: the full inventory, including hidden rows. The session table stays the working set.

## Ops hub

One bottom tab: Callbacks · Screenshots · Credentials · Downloads · Resources · Tasks · Networking.

Resources are uploaded once (`data/resources/`) and called by name (`bof who.x64.o`). Loot lives under `data/loot/agents/<id>/`. Tasks are queued → sent → processing → completed/error, with a global table and per-console badges.

## Console

Split view (real consoles, not clones). Guttered stdout, aligned Tengu `ls`/`ps`, clickable directory `ls`, in-place transfer progress. Bulk actions: Shell, Sleep, Alias, Tag, Hide, Dead, Kill.

## Persistence that should have been boring

TCP / DNS / DoH listeners restore. Tunnels restore. Tengu ChaCha20 session keys restore (agents registered before this build need one new INIT). `data/` is created if missing; do not `rm -rf data/` to reset loot (that also deletes MinGW).

## Also in this RC

- Tengu HTTP URI rotation and custom request headers
- Map view removed (ip-api.com leaked the operator IP)
- Epic `MUGEN` splash on teamserver and client
- Client compile defaults to 4 jobs (`JOBS=$(nproc)` to go wide)
- Themed floating file dialogs

Full changelog: [`RELEASE.md`](https://github.com/MugenFramework/Mugen/blob/develop/v0.2/my-dress-up-darling/RELEASE.md)

---

## Pull requests

Every change since v0.1.4. Click a PR to see the diff.

- [#22](https://github.com/MugenFramework/Mugen/pull/22) chore(version): bump to v0.2 My Dress-Up Darling
- [#23](https://github.com/MugenFramework/Mugen/pull/23) feat(tengu): URI rotation and custom request headers for HTTP transport
- [#24](https://github.com/MugenFramework/Mugen/pull/24) feat(client): split console view
- [#25](https://github.com/MugenFramework/Mugen/pull/25) feat(client): Actions button in session table
- [#26](https://github.com/MugenFramework/Mugen/pull/26) feat(client): session table column customization
- [#27](https://github.com/MugenFramework/Mugen/pull/27) feat(client): payload builder one-liner generator
- [#28](https://github.com/MugenFramework/Mugen/pull/28) feat(client): TLS certificate fingerprint viewer
- [#29](https://github.com/MugenFramework/Mugen/pull/29) fix(teamserver): persist TCP, DNS and DoH listeners across restarts
- [#30](https://github.com/MugenFramework/Mugen/pull/30) feat(client): multi-select sessions + bulk actions
- [#31](https://github.com/MugenFramework/Mugen/pull/31) feat(networking): SOCKS5 proxy + port forwarding with persistence
- [#32](https://github.com/MugenFramework/Mugen/pull/32) fix(agent): remove func-type fields from ToMap serialization
- [#33](https://github.com/MugenFramework/Mugen/pull/33) feat(resources): Resource Manager
- [#34](https://github.com/MugenFramework/Mugen/pull/34) feat(resources): uploader column, SHA-256, context menu, detail panel
- [#35](https://github.com/MugenFramework/Mugen/pull/35) feat(history): persistent task history with per-task search, comment and delete
- [#36](https://github.com/MugenFramework/Mugen/pull/36) feat(ls,browser): permissions display and directory navigation
- [#37](https://github.com/MugenFramework/Mugen/pull/37) fix(tengu): sleep_obf SIGSEGV caused by GCC -Os function reordering
- [#38](https://github.com/MugenFramework/Mugen/pull/38) fix(webhook): skip empty username/avatar and log errors
- [#39](https://github.com/MugenFramework/Mugen/pull/39) feat(alias): assign a human-readable alias to an agent
- [#40](https://github.com/MugenFramework/Mugen/pull/40) feat(client): show agent alias in console tab titles
- [#41](https://github.com/MugenFramework/Mugen/pull/41) feat(client): remove geolocation map view
- [#42](https://github.com/MugenFramework/Mugen/pull/42) feat(tasks): track task status and add a live Tasks widget
- [#43](https://github.com/MugenFramework/Mugen/pull/43) feat(session): persist notes and tags on the teamserver
- [#44](https://github.com/MugenFramework/Mugen/pull/44) fix(client): apply Mugen theme to the Set Alias dialog
- [#45](https://github.com/MugenFramework/Mugen/pull/45) feat(session): persist session row color on the teamserver
- [#46](https://github.com/MugenFramework/Mugen/pull/46) feat(client): unify Loot, Resources, Tasks and Networking into an Ops hub
- [#47](https://github.com/MugenFramework/Mugen/pull/47) feat(loot): persist screenshots/downloads and let operators download or delete them
- [#48](https://github.com/MugenFramework/Mugen/pull/48) fix(loot): keep Downloads tab when selecting a file
- [#49](https://github.com/MugenFramework/Mugen/pull/49) feat(client): expand bulk session actions (alias, tag, mark dead, sleep jitter)
- [#50](https://github.com/MugenFramework/Mugen/pull/50) fix(tls): log certificate SHA-256 on the teamserver, not in Event Viewer
- [#51](https://github.com/MugenFramework/Mugen/pull/51) feat(console): aligned ls/ps tables with clickable directory ls
- [#52](https://github.com/MugenFramework/Mugen/pull/52) feat(console): in-place progress bar for uploads and downloads
- [#53](https://github.com/MugenFramework/Mugen/pull/53) fix(client): reuse session consoles in split view
- [#54](https://github.com/MugenFramework/Mugen/pull/54) feat: hide callbacks from the session table, recover them in Ops
- [#55](https://github.com/MugenFramework/Mugen/pull/55) fix(tengu): persist ChaCha20 session key across teamserver restart
- [#56](https://github.com/MugenFramework/Mugen/pull/56) fix(client): Mugen-themed floating save/open dialogs
- [#57](https://github.com/MugenFramework/Mugen/pull/57) feat: Epic FIGlet MUGEN banner on client and teamserver
- [#58](https://github.com/MugenFramework/Mugen/pull/58) chore: default client compile to 4 parallel jobs
- [#59](https://github.com/MugenFramework/Mugen/pull/59) chore: mark v0.2 My Dress-Up Darling as RC1

---

## Upgrade from v0.1.x

1. `git checkout develop/v0.2/my-dress-up-darling && make rebuild`
2. Start the new teamserver, then the new client.
3. Restart any Tengu implant that checked in before the ChaCha20 key was persisted, so INIT stores the key.
4. Notes and tags that lived in the old client-local SQLite are **not** migrated. Re-enter what you still care about.

EOF
)" \
    "/tmp/${ZIP}"

echo
echo "[+] pre-release: https://github.com/MugenFramework/Mugen/releases/tag/${TAG}"
