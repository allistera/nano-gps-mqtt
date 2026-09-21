#!/bin/sh
set -eu

repo=${1:-.}
tracked=$(git -C "$repo" ls-files -- 'secrets.h' ':(glob)**/secrets.h')

if [ -n "$tracked" ]; then
  echo "error: secrets.h must never be tracked:" >&2
  printf '%s\n' "$tracked" >&2
  exit 1
fi
