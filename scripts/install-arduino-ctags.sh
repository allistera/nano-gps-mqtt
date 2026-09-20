#!/bin/sh
set -eu

install_dir=$(pwd -P)/build/tools/ctags

if [ -x "$install_dir/bin/ctags" ]; then
  exit 0
fi

mkdir -p build/tools
temp_dir=$(mktemp -d build/ctags-build.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT HUP INT TERM

git clone --depth 1 --branch 5.8-arduino11 \
  https://github.com/arduino/ctags.git "$temp_dir/source"

# Arduino ctags defines __unused__, which conflicts with modern macOS SDK headers.
# Rename only that internal macro while preserving the tagged parser behaviour.
perl -pi -e 's/__unused__/CTAGS_UNUSED/g' "$temp_dir"/source/*.[ch]

(
  cd "$temp_dir/source"
  ./configure --prefix="$install_dir"
  make -j4
  make install
)
