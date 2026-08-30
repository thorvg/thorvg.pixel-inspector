#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <golden-ref> <test-ref> [test-options...]"
    echo "Example: $0 v1.0.5 main --backend cpu"
    exit 1
fi

GOLDEN_REF="$1"
TEST_REF="$2"
shift 2

TEST_OPTIONS=("$@")

TOTAL_START=$SECONDS
GOLDEN_START=$SECONDS
bash ./install_thorvg.sh "$GOLDEN_REF"
bash ./build_and_run.sh "${TEST_OPTIONS[@]}" --update-golden
echo "Golden elapsed: $((SECONDS - GOLDEN_START)) seconds"

TEST_START=$SECONDS
bash ./install_thorvg.sh "$TEST_REF"
set +e
bash ./build_and_run.sh "${TEST_OPTIONS[@]}"
TEST_STATUS="$?"
set -e
echo "Test elapsed: $((SECONDS - TEST_START)) seconds"
echo "Total elapsed: $((SECONDS - TOTAL_START)) seconds"

# Report: print each backend tab to a separate PDF.
REPORT_HTML="$(pwd)/output/reporter.html"
if [ -f "$REPORT_HTML" ]; then
    CHROME=""
    if command -v google-chrome >/dev/null 2>&1; then
        CHROME="$(command -v google-chrome)"
    elif command -v chromium >/dev/null 2>&1; then
        CHROME="$(command -v chromium)"
    elif [ -x "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" ]; then
        CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
    elif [ -x "/Applications/Chromium.app/Contents/MacOS/Chromium" ]; then
        CHROME="/Applications/Chromium.app/Contents/MacOS/Chromium"
    fi

    if [ -n "$CHROME" ]; then
        # Backend tab names present in the report (every data-tab value except "all").
        BACKENDS="$(grep -oE 'data-tab="[a-z]+"' "$REPORT_HTML" | sed -E 's/.*"(.*)".*/\1/' | grep -vx 'all' || true)"
        if [ -z "$BACKENDS" ]; then
            echo "No backend tabs found in $REPORT_HTML; skipping PDF export."
        fi
        for BACKEND in $BACKENDS; do
            REPORT_PDF="$(pwd)/output/reporter.$BACKEND.pdf"
            # The #<backend> hash makes the report preselect that backend before printing.
            "$CHROME" --headless --disable-gpu --no-sandbox --print-to-pdf="$REPORT_PDF" "file://$REPORT_HTML#$BACKEND" \
                && echo "Wrote $REPORT_PDF" || echo "Failed to print $REPORT_PDF"
        done
    else
        echo "Chrome or Chromium is required to print $REPORT_HTML to PDF."
    fi
fi

exit "$TEST_STATUS"
