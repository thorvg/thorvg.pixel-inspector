#!/usr/bin/env bash

set -euo pipefail

# Build two ThorVG revisions once and compare their backends in parallel.

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THORVG_SOURCE_DIR="${THORVG_SOURCE_DIR:-$ROOTDIR/temp/thorvg}"
WORKDIR="${PIXEL_PARALLEL_WORKDIR:-$ROOTDIR/temp/parallel}"
OUTPUT_ROOT="${PIXEL_PARALLEL_OUTPUT:-$ROOTDIR/output}"
PIXEL_LOG="${PIXEL_LOG:-true}"
IFS=',' read -r -a BACKENDS <<< "${PIXEL_BACKENDS:-cpu,gl,wg}"

GOLDEN_REF="$1"
TEST_REF="$2"
PR_NUMBER=""
shift 2

if [[ "${1:-}" =~ ^[0-9]+$ ]]; then
    PR_NUMBER="$1"
    shift
fi

setup_build()
{
    local build_dir="$1"
    shift

    if [ -f "$build_dir/meson-private/coredata.dat" ]; then
        meson setup "$build_dir" --wipe "$@"
    else
        meson setup "$build_dir" "$@"
    fi
}

build_revision()
{
    local ref="$1"
    local name="$2"
    local prefix="$WORKDIR/install-$name"
    local thorvg_build="$WORKDIR/build-thorvg-$name"
    local inspector_build="$WORKDIR/build-inspector-$name"
    local binary="$WORKDIR/tvg-pixel-inspector-$name"

    # Separate prefixes keep each inspector linked to its matching revision.
    git -C "$THORVG_SOURCE_DIR" checkout "$ref"
    setup_build "$thorvg_build" "$THORVG_SOURCE_DIR" \
        --prefix "$prefix" \
        --libdir lib \
        -Dengines=all \
        -Dloaders=all \
        -Dextra=lottie_exp,openmp
    ninja -C "$thorvg_build"
    ninja -C "$thorvg_build" install

    PKG_CONFIG_PATH="$prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
        setup_build "$inspector_build" "$ROOTDIR" -Dbackends=all -Dlog="$PIXEL_LOG"
    ninja -C "$inspector_build"
    cp "$inspector_build/src/tvg-pixel-inspector" "$binary"
}

TOTAL_START=$SECONDS
mkdir -p "$WORKDIR"

if [ "${PIXEL_SKIP_BUILD:-false}" != true ]; then
    build_revision "$GOLDEN_REF" golden
    build_revision "$TEST_REF" test
fi

if [ "${PIXEL_BUILD_ONLY:-false}" = true ]; then
    echo "Build elapsed: $((SECONDS - TOTAL_START)) seconds"
    exit 0
fi

STATUS=0
# Reuse both builds while evaluate_parallel.sh runs each backend concurrently.
bash "$ROOTDIR/evaluate_parallel.sh" \
    "$WORKDIR/tvg-pixel-inspector-golden" \
    "$WORKDIR/install-golden/lib" \
    "$WORKDIR/tvg-pixel-inspector-test" \
    "$WORKDIR/install-test/lib" \
    "$OUTPUT_ROOT" \
    "$@" || STATUS=$?

# Add PR-specific names only when requested; otherwise keep the default names.
if [ -n "$PR_NUMBER" ]; then
    for backend in "${BACKENDS[@]}"; do
        report_dir="$OUTPUT_ROOT/$backend"
        for extension in html md; do
            report="$report_dir/reporter.$extension"
            if [ -f "$report" ]; then
                mv "$report" "$report_dir/reporter.PR-$PR_NUMBER.$extension"
            fi
        done
    done

    if [ -f "$OUTPUT_ROOT/reporter.md" ]; then
        cp "$OUTPUT_ROOT/reporter.md" "$OUTPUT_ROOT/reporter.PR-$PR_NUMBER.md"
    fi
fi

echo "Update and evaluation elapsed: $((SECONDS - TOTAL_START)) seconds"
exit "$STATUS"
