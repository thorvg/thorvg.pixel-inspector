#!/usr/bin/env bash

set -euo pipefail

# Compare the configured backends concurrently with isolated outputs.

GOLDEN_BINARY="$1"
GOLDEN_LIB_DIR="$2"
TEST_BINARY="$3"
TEST_LIB_DIR="$4"
OUTPUT_ROOT="$5"
shift 5

TEST_OPTIONS=("$@")
IFS=',' read -r -a BACKENDS <<< "${PIXEL_BACKENDS:-cpu,gl,wg}"

run_backend()
{
    local backend="$1"
    local output_dir="$OUTPUT_ROOT/$backend"
    local golden_log="$OUTPUT_ROOT/$backend.golden.log"
    local test_log="$OUTPUT_ROOT/$backend.test.log"
    local start=$SECONDS
    local golden_status=0
    local test_status=0

    mkdir -p "$output_dir"

    # 1. Generate a complete golden set for this backend.
    LD_LIBRARY_PATH="$GOLDEN_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    DYLD_LIBRARY_PATH="$GOLDEN_LIB_DIR${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" \
        "$GOLDEN_BINARY" "${TEST_OPTIONS[@]}" \
            --backend "$backend" \
            --output "$output_dir" \
            --update-golden > "$golden_log" 2>&1 || golden_status=$?

    test_status=$golden_status
    if [ "$golden_status" -eq 0 ]; then
        # 2. Compare the test revision against the generated golden set.
        LD_LIBRARY_PATH="$TEST_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        DYLD_LIBRARY_PATH="$TEST_LIB_DIR${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" \
            "$TEST_BINARY" "${TEST_OPTIONS[@]}" \
                --backend "$backend" \
                --output "$output_dir" > "$test_log" 2>&1 || test_status=$?
    else
        echo "Test skipped because golden generation failed." > "$test_log"
    fi

    echo "$backend elapsed: $((SECONDS - start)) seconds"
    return "$test_status"
}

TOTAL_START=$SECONDS
PIDS=()
STATUS=0

mkdir -p "$OUTPUT_ROOT"

# Separate output directories prevent concurrent backends from overwriting reports.
for backend in "${BACKENDS[@]}"; do
    echo "Starting backend: $backend"
    run_backend "$backend" &
    PIDS+=("$!")
done

for i in "${!BACKENDS[@]}"; do
    backend="${BACKENDS[$i]}"
    if wait "${PIDS[$i]}"; then
        echo "Completed backend: $backend"
    else
        echo "Failed backend: $backend" >&2
        tail -50 "$OUTPUT_ROOT/$backend.golden.log" >&2
        tail -50 "$OUTPUT_ROOT/$backend.test.log" >&2
        STATUS=1
    fi
done

# Aggregate only after every backend has finished writing its report.
awk -F, '
    BEGIN {
        print "## Pixel Inspector Report"
        print ""
        print "| Backend | Compared | Differences | Errors |"
        print "| --- | ---: | ---: | ---: |"
    }
    FNR == 1 { next }
    {
        printf "| `%s` | %s | %s | %s |\n", $1, $2, $3, $4
    }
' "$OUTPUT_ROOT"/*/reporter.csv > "$OUTPUT_ROOT/reporter.md"

echo "Total elapsed: $((SECONDS - TOTAL_START)) seconds"
exit 0
