#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESOURCE_DIR="$ROOTDIR/res/compliance/webp"
SOURCE_URL="https://chromium.googlesource.com/webm/libwebp-test-data"
REVISION="06ddd96e276c2c638a72d39d3c0f340afd61978c"
EXPECTED_COUNT=131

mkdir -p "$RESOURCE_DIR"
ASSET_COUNT="$(find "$RESOURCE_DIR" -maxdepth 1 -type f -name '*.webp' | wc -l | tr -d ' ')"
if [ "$ASSET_COUNT" -eq "$EXPECTED_COUNT" ] && \
   [ "$(cat "$RESOURCE_DIR/.revision" 2>/dev/null || true)" = "$REVISION" ]; then
    echo "WebP compliance resources already downloaded ($ASSET_COUNT files)."
    exit 0
fi

command -v git >/dev/null 2>&1 || {
    echo "git is required to download WebP compliance resources." >&2
    exit 1
}

DOWNLOAD_DIR="$(mktemp -d)"
trap 'rm -rf "$DOWNLOAD_DIR"' EXIT

echo "Downloading WebP compliance resources from $SOURCE_URL at $REVISION"
git init -q "$DOWNLOAD_DIR"
git -C "$DOWNLOAD_DIR" fetch --depth 1 "$SOURCE_URL" "$REVISION"
git -C "$DOWNLOAD_DIR" checkout -q --detach FETCH_HEAD

ASSETS=("$DOWNLOAD_DIR"/*.webp)
if [ "${#ASSETS[@]}" -ne "$EXPECTED_COUNT" ]; then
    echo "Expected $EXPECTED_COUNT WebP files in upstream revision $REVISION." >&2
    exit 1
fi

cp "${ASSETS[@]}" "$RESOURCE_DIR/"
printf '%s\n' "$REVISION" > "$RESOURCE_DIR/.revision"
echo "Downloaded ${#ASSETS[@]} WebP files to $RESOURCE_DIR"
