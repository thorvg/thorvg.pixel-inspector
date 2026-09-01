#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESOURCE_DIR="$ROOTDIR/res/compliance"
OUTPUT_DIR="$ROOTDIR/output/compliance"
PIXEL_LOG="${PIXEL_LOG:-false}"
UNZIP=false
REFS=()

usage()
{
    echo "Usage: $0 [--unzip] <golden-ref> <test-ref>"
    echo "  --unzip  Extract every archive under res/compliance before testing."
    echo "Example: $0 --unzip v1.1.1 main"
}

# Parse options and collect the two ThorVG refs.
for argument in "$@"; do
    case "$argument" in
        --unzip) UNZIP=true ;;
        --help|-h)
            usage
            exit 0
            ;;
        --*)
            echo "Unknown option: $argument" >&2
            usage >&2
            exit 1
            ;;
        *) REFS+=("$argument") ;;
    esac
done

if [ "${#REFS[@]}" -ne 2 ]; then
    usage >&2
    exit 1
fi

GOLDEN_REF="${REFS[0]}"
TEST_REF="${REFS[1]}"

# Extract all compliance ZIP archives in place when requested.
if [ "$UNZIP" = true ]; then
    command -v unzip >/dev/null 2>&1 || {
        echo "unzip is required to extract compliance resources." >&2
        exit 1
    }

    ARCHIVE_COUNT=0
    while IFS= read -r -d '' archive; do
        unzip -oq "$archive" -d "$(dirname "$archive")"
        ARCHIVE_COUNT=$((ARCHIVE_COUNT + 1))
    done < <(find "$RESOURCE_DIR" -type f -name '*.zip' -print0)
    echo "Extracted $ARCHIVE_COUNT compliance archives."
fi

# Count all supported assets under the compliance resource directory.
ASSET_COUNT="$(find "$RESOURCE_DIR" -type f \( -name '*.svg' -o -name '*.json' \) | wc -l | tr -d ' ')"
if [ "$ASSET_COUNT" -eq 0 ]; then
    echo "No SVG or Lottie compliance resources found under $RESOURCE_DIR." >&2
    echo "Run $0 --unzip if the resources are archived." >&2
    exit 1
fi

echo "Compliance resources: $ASSET_COUNT files"

# Run the comparison from the project root.
cd "$ROOTDIR"
if [ "${PIXEL_SKIP_BUILD:-false}" = true ]; then
    UPDATE_ARGS=("$GOLDEN_REF" "$TEST_REF")
    if [ -n "${PIXEL_PR_NUMBER:-}" ]; then
        UPDATE_ARGS+=("$PIXEL_PR_NUMBER")
    fi

    PIXEL_PARALLEL_OUTPUT="${PIXEL_PARALLEL_OUTPUT:-$ROOTDIR/output/compliance}" \
    PIXEL_BACKENDS=cpu PIXEL_LOG="$PIXEL_LOG" PIXEL_SKIP_BUILD=true \
        bash ./update_and_evaluate_parallel.sh "${UPDATE_ARGS[@]}" \
        --resource "$RESOURCE_DIR" \
        --skip-examples
else
    THORVG_ENGINES=cpu PIXEL_BACKENDS=cpu PIXEL_LOG="$PIXEL_LOG" \
        bash ./update_and_evaluate.sh "$GOLDEN_REF" "$TEST_REF" \
        --backend cpu \
        --resource "$RESOURCE_DIR" \
        --output "$OUTPUT_DIR/cpu" \
        --skip-examples
fi
