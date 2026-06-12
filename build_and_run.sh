#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THORVG_PREFIX="$ROOTDIR/temp/thorvg-install"

export PKG_CONFIG_PATH="$THORVG_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export DYLD_LIBRARY_PATH="$THORVG_PREFIX/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="$THORVG_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

meson setup build --wipe
ninja -C build
./build/src/tvg-pixel-inspector "$@"
