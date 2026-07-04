#!/bin/bash
set -e

git checkout -b fix/builder-export-name-rand

git add teamserver/pkg/common/builder/builder.go

git commit -m "fix: use crypto/rand.Read instead of rand.Intn for export name generation"

git push -u origin fix/builder-export-name-rand

gh pr create \
  --title "fix: export name randomization compile error" \
  --body "## Summary

\`builder.go\` already imports \`crypto/rand\` as \`rand\`. The export name
randomization introduced in the artifact strings cleanup PR used \`rand.Intn\`
which belongs to \`math/rand\` and does not exist on \`crypto/rand\`, causing a
compile error.

## Fix

Replace \`rand.Intn(n)\` with \`rand.Read\` on a 7-byte buffer and use
\`int(b[i]) % len(charset)\` for index selection. Same entropy, no new import."
