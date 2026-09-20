#!/bin/sh
set -eu

install_dir=$(pwd -P)/build/tools/dfu-util

if [ -x "$install_dir/bin/dfu-util" ]; then
  exit 0
fi

mkdir -p build/tools
temp_dir=$(mktemp -d build/dfu-build.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT HUP INT TERM

git clone https://github.com/bcmi-labs/dfu-util.git "$temp_dir/source"
git -C "$temp_dir/source" checkout 3a18dc1de4dccbd375db6562adb8c42564abfeeb

(
  cd "$temp_dir/source"
  ./autogen.sh
  ./configure --prefix="$install_dir"
  make -j4
  make install
)
