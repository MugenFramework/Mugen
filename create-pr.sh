#!/bin/bash
set -e

git checkout -b feat/tengu-shell-pty

git add payloads/Tengu/src/commands.c \
  client/src/Mugen/Tengu/TenguConsoleInput.cc \
  RELEASE.md ROADMAP.md

git commit -m "$(cat <<'EOF'
feat(tengu): run shell on a PTY instead of popen

EOF
)"

git push -u origin feat/tengu-shell-pty

gh pr create \
    --base "develop/v0.2/my-dress-up-darling" \
    --title "feat(tengu): run shell on a PTY instead of popen" \
    --body "$(cat <<'EOF'
## Summary

- `shell` used `popen`: no TTY, so `sudo`/`su`/`passwd` and anything that checks `isatty` fail or look wrong. stderr was also easy to miss.
- It now `posix_openpt` + `sh -c`, with stderr on the same PTY. Hung commands are SIGKILL'd after 120s so the implant keeps beaconing.
- Interactive stdin across check-ins (`pty start` / send / stop) is the next step, with long-running jobs.

## Test plan

- [ ] Rebuild a Tengu payload and check in
- [ ] `shell id` / `shell ls -la` still return output
- [ ] `shell tty` prints a `/dev/pts/*` path (not "not a tty")
- [ ] `shell 'echo hi >&2'` shows `hi` (stderr captured)
- [ ] `shell sleep 2` returns after ~2s, not 120s
- [ ] A command that waits on a password is killed after 120s with `[pty] timed out after 120s` (don't wait the full two minutes unless you want to)

EOF
)"
