#!/usr/bin/env bash

set -euo pipefail

REPO_URL="https://github.com/thorvg/thorvg.git"
REF="${1:-}"
ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$ROOTDIR/temp"
SRCDIR="$WORKDIR/thorvg"
BUILDDIR="$SRCDIR/builddir"
INSTALLDIR="$WORKDIR/thorvg-install"
ENGINES="${THORVG_ENGINES:-all}"

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
    git fetch origin --prune --tags
    if git show-ref --verify --quiet "refs/remotes/origin/$REF"; then
        git checkout -B "$REF" "origin/$REF"
    else
        git checkout --detach "$REF"
    fi
fi

echo "Configuring ThorVG..."
meson setup "$BUILDDIR" --wipe \
    --prefix "$INSTALLDIR" \
    --libdir lib \
    -Dengines="$ENGINES" \
    -Dloaders=all \
    -Dsavers=all \
    -Dextra=lottie_exp,openmp \
    -Dstatic=true

echo "Building ThorVG..."
ninja -C "$BUILDDIR"

echo "Installing ThorVG..."
ninja -C "$BUILDDIR" install

echo "ThorVG installed successfully."
echo "ThorVG prefix: $INSTALLDIR"
echo "Use this pkg-config path: $INSTALLDIR/lib/pkgconfig"
