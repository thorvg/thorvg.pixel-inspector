#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESOURCE_DIR="$ROOTDIR/res/compliance"
OUTPUT_DIR="$ROOTDIR/output/compliance"
PIXEL_LOG="${PIXEL_LOG:-false}"
UNZIP=false
TYPE=all
REFS=()

usage()
{
    echo "Usage: $0 [--type all|svg|webp] [--unzip] <golden-ref> <test-ref>"
    echo "  --type   Compliance set to run (default: all)."
    echo "  --unzip  Extract archives in the selected compliance set before testing."
    echo "Example: $0 --unzip v1.1.1 main"
}

# Parse options and collect the two ThorVG refs.
while [ "$#" -gt 0 ]; do
    argument="$1"
    case "$argument" in
        --type=*) TYPE="${argument#*=}" ;;
        --type)
            if [ "$#" -lt 2 ]; then
                echo "--type requires all, svg or webp." >&2
                exit 1
            fi
            TYPE="$2"
            shift
            ;;
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
    shift
done

case "$TYPE" in
    all) ;;
    svg) RESOURCE_DIR="$RESOURCE_DIR/godot" ;;
    webp) RESOURCE_DIR="$RESOURCE_DIR/webp" ;;
    *)
        echo "Invalid compliance type: $TYPE (expected all, svg or webp)." >&2
        exit 1
        ;;
esac
if [ "$TYPE" != all ]; then
    OUTPUT_DIR="$OUTPUT_DIR/$TYPE"
fi

if [ "${#REFS[@]}" -ne 2 ]; then
    usage >&2
    exit 1
fi

GOLDEN_REF="${REFS[0]}"
TEST_REF="${REFS[1]}"

if [ "$TYPE" = all ] || [ "$TYPE" = webp ]; then
    bash "$ROOTDIR/download_webp_compliance.sh"
fi

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
ASSET_COUNT="$(find "$RESOURCE_DIR" -type f \( -name '*.svg' -o -name '*.json' -o -name '*.webp' \) | wc -l | tr -d ' ')"
if [ "$ASSET_COUNT" -eq 0 ]; then
    echo "No SVG, Lottie or WebP compliance resources found under $RESOURCE_DIR." >&2
    echo "Run $0 --unzip if the resources are archived." >&2
    exit 1
fi

echo "Compliance resources ($TYPE): $ASSET_COUNT files"

# Run the comparison from the project root.
cd "$ROOTDIR"
if [ "${PIXEL_SKIP_BUILD:-false}" = true ]; then
    UPDATE_ARGS=("$GOLDEN_REF" "$TEST_REF")
    if [ -n "${PIXEL_PR_NUMBER:-}" ]; then
        UPDATE_ARGS+=("$PIXEL_PR_NUMBER")
    fi

    PIXEL_PARALLEL_OUTPUT="${PIXEL_PARALLEL_OUTPUT:-$OUTPUT_DIR}" \
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
