#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <reference-ref> <test-ref>"
    echo "Example: $0 v1.0.5 main"
    exit 1
fi

REFERENCE_REF="$1"
TEST_REF="$2"

bash ./install_thorvg.sh "$REFERENCE_REF"
bash ./build_and_run.sh --evaluator pixel --update-reference

bash ./install_thorvg.sh "$TEST_REF"
bash ./build_and_run.sh --evaluator pixel

# Report
REPORT_HTML="$(pwd)/artifacts/report/reporter.html"
REPORT_PDF="$(pwd)/artifacts/report/reporter.pdf"
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
        "$CHROME" --headless --disable-gpu --no-sandbox --print-to-pdf="$REPORT_PDF" "file://$REPORT_HTML"
    else
        echo "Chrome or Chromium is required to print $REPORT_HTML to PDF."
    fi
fi
