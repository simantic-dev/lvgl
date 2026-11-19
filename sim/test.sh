#!/bin/sh
# Boots the committed LVGL widgets demo (ESP32-C3, ILI9341 + XPT2046 peers) for 14 virtual s and checks the
# firmware reached the demo, the panel was switched on and drawn to, and the picture changed (>3 distinct frames).
cd "$(dirname "$0")/.." || exit 1
d=$(mktemp -d)
mkdir "$d/raw"
ILI9341_SEQ_DIR=$d/raw ILI9341_SEQ_MS=100 ILI9341_STATS_OUT=$d/stats.json \
  ${SIM:-sim} run --scenario sim/scenario.yaml --timeout 14 --ascii >"$d/uart" 2>"$d/err"
rc=0
chk() { if eval "$2"; then echo "ok   $1"; else echo "FAIL $1"; rc=1; fi; }
chk "firmware reached the demo" "grep -qF 'LVGL demo started' $d/uart"
chk "panel display on" "grep -qF '\"display_on\": true' $d/stats.json"
chk "frame memory written (RAMWR >= 100)" "[ \$(grep -o '\"2C\": [0-9]*' $d/stats.json | grep -o '[0-9]*\$') -ge 100 ]"
n=$(for f in "$d"/raw/*.raw; do shasum "$f"; done | cut -d' ' -f1 | sort -u | wc -l)
chk "picture changed ($n distinct frames)" "[ $n -gt 3 ]"
chk "no panic or abort" "! grep -qiE 'guru meditation|abort\\(\\)|panic' $d/uart"
rm -rf "$d"
exit $rc
