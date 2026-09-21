#!/bin/sh
set -eu

repo_root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
guard="$repo_root/scripts/check-no-secrets.sh"
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT HUP INT TERM

git -C "$fixture" init -q
git -C "$fixture" config user.email test@example.invalid
git -C "$fixture" config user.name "Test User"

printf 'safe\n' > "$fixture/README.md"
git -C "$fixture" add README.md
"$guard" "$fixture"

mkdir -p "$fixture/nested"
printf 'credential\n' > "$fixture/nested/secrets.h"
git -C "$fixture" add -f nested/secrets.h

if "$guard" "$fixture" >/dev/null 2>&1; then
  echo "expected tracked secrets.h to be rejected" >&2
  exit 1
fi
