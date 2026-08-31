#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THORVG_PREFIX="$ROOTDIR/temp/thorvg-install"
RESOURCE_DIR="$ROOTDIR/res/compliance/godot"
BUILD_DIR="$ROOTDIR/build-compliance"
OUTPUT_DIR="$ROOTDIR/output/compliance/godot"
UNZIP=false

usage()
{
    echo "Usage: $0 [--unzip]"
    echo "  --unzip  Extract the three Godot SVG compliance archives before testing."
}

for option in "$@"; do
    case "$option" in
        --unzip) UNZIP=true ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 1
            ;;
    esac
done

if [ "$UNZIP" = true ]; then
    command -v unzip >/dev/null 2>&1 || {
        echo "unzip is required to extract compliance resources." >&2
        exit 1
    }

    archives=(
        "ThorvgValidFiles.zip"
        "ThorvgNotValidFiles.zip"
        "AA_5.svg.zip"
    )
    for archive in "${archives[@]}"; do
        archive_path="$RESOURCE_DIR/$archive"
        if [ ! -f "$archive_path" ]; then
            echo "Missing compliance archive: $archive_path" >&2
            exit 1
        fi
        unzip -oq "$archive_path" -d "$RESOURCE_DIR"
    done
fi

SVG_COUNT="$(find "$RESOURCE_DIR" -type f -name '*.svg' | wc -l | tr -d ' ')"
if [ "$SVG_COUNT" -eq 0 ]; then
    echo "No extracted SVG compliance resources found." >&2
    echo "Run $0 --unzip first." >&2
    exit 1
fi

echo "Compliance target: godot ($SVG_COUNT SVG files)"

export PKG_CONFIG_PATH="$THORVG_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export DYLD_LIBRARY_PATH="$THORVG_PREFIX/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="$THORVG_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

meson setup "$BUILD_DIR" --wipe -Dbackends=cpu
ninja -C "$BUILD_DIR"

TEST_BINARY="$BUILD_DIR/src/tvg-pixel-inspector"
TEST_OPTIONS=(
    --backend cpu
    --resource "$RESOURCE_DIR"
    --output "$OUTPUT_DIR"
    --skip-examples
)

"$TEST_BINARY" "${TEST_OPTIONS[@]}" --update-golden
"$TEST_BINARY" "${TEST_OPTIONS[@]}"
