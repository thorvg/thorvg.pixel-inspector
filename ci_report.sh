#!/usr/bin/env bash

set -euo pipefail

ROOTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULT_DIR="${PIXEL_REPORT_RESULT_DIR:-$ROOTDIR/output}"
EXPORT_DIR="${PIXEL_REPORT_EXPORT_DIR:-$RESULT_DIR/ci-report}"
INPUT_DIR="${PIXEL_REPORT_INPUT_DIR:-$(dirname "$ROOTDIR")/pixel-reports}"

usage()
{
    echo "Usage:"
    echo "  $0 export-shard <pr-number> <shard>"
    echo "  $0 export-compliance <pr-number>"
    echo "  $0 aggregate <pr-number> <shard-count>"
}

print_pdf()
{
    local html="$1"
    local pdf="$2"
    local tab="$3"
    local chrome="${CHROME:-google-chrome}"

    if [ ! -f "$html" ]; then
        echo "Report HTML not found: $html"
        return
    fi
    if ! command -v "$chrome" >/dev/null 2>&1; then
        echo "Chrome not found: $chrome"
        return
    fi

    if "$chrome" \
        --headless \
        --disable-gpu \
        --no-sandbox \
        --print-to-pdf="$pdf" \
        "file://$html#$tab"; then
        echo "Wrote $pdf"
    else
        echo "Failed to print $pdf"
    fi
}

export_shard()
{
    local prNumber="$1"
    local shard="$2"

    mkdir -p "$EXPORT_DIR"

    for backend in cpu gl wg; do
        print_pdf \
            "$RESULT_DIR/$backend/reporter.PR-$prNumber.html" \
            "$EXPORT_DIR/reporter.$backend.shard-$shard.PR-$prNumber.pdf" \
            "$backend"

        if [ -f "$RESULT_DIR/$backend/reporter.csv" ]; then
            mkdir -p "$EXPORT_DIR/$backend"
            cp "$RESULT_DIR/$backend/reporter.csv" "$EXPORT_DIR/$backend/reporter.csv"
        fi
    done
}

export_compliance()
{
    local prNumber="$1"
    local reportCsv="$RESULT_DIR/compliance/cpu/reporter.csv"

    mkdir -p "$EXPORT_DIR/compliance"

    print_pdf \
        "$RESULT_DIR/compliance/cpu/reporter.PR-$prNumber.html" \
        "$EXPORT_DIR/reporter.compliance.PR-$prNumber.pdf" \
        cpu

    if [ -f "$reportCsv" ]; then
        sed 's/^cpu,/compliance,/' "$reportCsv" > "$EXPORT_DIR/compliance/reporter.csv"
    else
        echo "Report CSV not found: $reportCsv"
    fi
}

aggregate()
{
    local prNumber="$1"
    local shardCount="$2"

    mkdir -p "$EXPORT_DIR"
    shopt -s globstar nullglob

    local reports=("$INPUT_DIR"/**/reporter.csv)

    awk -F, -v shardCount="$shardCount" '
        BEGIN {
          print "## Pixel Inspector Report"
          print ""
          print "| Backend | Compared | Differences | Errors |"
          print "| --- | ---: | ---: | ---: |"
        }
        FNR == 1 { next }
        $0 ~ /^(cpu|gl|wg|compliance),[0-9]+,[0-9]+,[0-9]+$/ {
          compared[$1] += $2
          differences[$1] += $3
          errors[$1] += $4
          seen[$1]++
        }
        END {
          count = split("cpu gl wg", backends, " ")
          for (i = 1; i <= count; i++) {
            backend = backends[i]
            missing = shardCount - seen[backend]
            if (missing > 0) errors[backend] += missing
            printf "| `%s` | %d | %d | %d |\n", backend, compared[backend], differences[backend], errors[backend]
          }
          missing = 1 - seen["compliance"]
          if (missing > 0) errors["compliance"] += missing
          printf "| `compliance` | %d | %d | %d |\n", compared["compliance"], differences["compliance"], errors["compliance"]
        }
    ' "${reports[@]}" /dev/null > "$EXPORT_DIR/reporter.md"

    echo "$prNumber" > "$EXPORT_DIR/pr-number.txt"

    for backend in cpu gl wg compliance; do
        local pdfs=("$INPUT_DIR"/**/reporter."$backend"*.PR-"$prNumber".pdf)
        if [ "${#pdfs[@]}" -eq 1 ]; then
            cp "${pdfs[0]}" "$EXPORT_DIR/reporter.$backend.PR-$prNumber.pdf"
        elif [ "${#pdfs[@]}" -gt 1 ]; then
            pdfunite "${pdfs[@]}" "$EXPORT_DIR/reporter.$backend.PR-$prNumber.pdf"
        fi
    done
}

case "${1:-}" in
    export-shard)
        [ "$#" -eq 3 ] || { usage >&2; exit 1; }
        export_shard "$2" "$3"
        ;;
    export-compliance)
        [ "$#" -eq 2 ] || { usage >&2; exit 1; }
        export_compliance "$2"
        ;;
    aggregate)
        [ "$#" -eq 3 ] || { usage >&2; exit 1; }
        aggregate "$2" "$3"
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac
