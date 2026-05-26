#!/usr/bin/env bash

set -euo pipefail

REPO_URL="https://github.com/thorvg/thorvg.git"
REF="${1:-}"
ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$ROOTDIR/temp"
SRCDIR="$WORKDIR/thorvg"
BUILDDIR="$SRCDIR/builddir"

echo "Cloning ThorVG into: $SRCDIR"
if [[ ! -d "$SRCDIR" ]]; then
    mkdir -p "$WORKDIR"
    git clone "$REPO_URL" "$SRCDIR"

    cd "$SRCDIR"
else
    echo "Using existing ThorVG source: $SRCDIR"
    cd "$SRCDIR"
fi

if [[ -n "$REF" ]]; then
    echo "Checking out ThorVG ref: $REF"
    git fetch --all --tags
    git checkout "$REF"
fi

echo "Configuring ThorVG..."
meson setup "$BUILDDIR" --wipe \
    -Dengines=all \
    -Dloaders=all \
    -Dsavers=all \
    -Dextra=lottie_exp,openmp

echo "Building ThorVG..."
ninja -C "$BUILDDIR"

echo "Installing ThorVG..."
ninja -C "$BUILDDIR" install

echo "ThorVG installed successfully."
