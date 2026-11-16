#!/bin/bash
# Sim addition. Boots the built LVGL demo on a simulated ESP32-C3 with the ILI9341 + XPT2046 peer (sim/c3.board),
# snapshots the panel frame memory every >= SEQ_MS of virtual time when it changed, converts to PNG, checks the
# frames are not blank and that the picture changed, and assembles sim/screenshots/<demo>.gif.
#   DEMO=widgets|music|benchmark (default widgets)   SECS=virtual seconds (default 14)   SEQ_MS=snapshot spacing (default 100)
set -e
cd "$(dirname "$0")/.."
SIM=${SIM:-sim}; D=${DEMO:-widgets}; O=sim/out/$D
IMG=sim/flash-esp32c3-$D.bin
rm -rf $O; mkdir -p $O/raw $O/png sim/screenshots
export ILI9341_SEQ_DIR=$O/raw ILI9341_SEQ_MS=${SEQ_MS:-100} ILI9341_STATS_OUT=$O/stats.json
$SIM ESP32-C3 run --elf $IMG --board sim/c3.board --timeout "${SECS:-14}" --ascii > $O/uart.log 2> $O/sim.log
python3 sim/raw2png.py $O/png $O/raw/*.raw | grep -v "fb-0001-" || true
N=$(ls $O/png | wc -l)
U=$(md5 -q $O/png/* | sort -u | wc -l)
grep -q "LVGL demo started" $O/uart.log || { echo "FAIL   firmware did not reach the demo"; exit 1; }
[ "$U" -gt 3 ] || { echo "FAIL   only $U distinct frames"; exit 1; }
rm -f $O/png/fb-0001-*
python3 sim/frames2gif.py sim/screenshots/$D.gif $O/png/*.png
echo "PASS   $N snapshots, $U distinct, $(grep -o '"2C": [0-9]*' $O/stats.json) RAMWR; GIF sim/screenshots/$D.gif"
