#!/bin/bash
# Sim addition. Builds the LVGL demo host (sim/fw) with the user's ESP-IDF 5.4.x install and writes one merged flash
# image sim/flash-<target>-<demo>.bin. TARGET=esp32c3|esp32s3 (default esp32c3), DEMO=widgets|music|benchmark.
# Build dirs and sdkconfig stay inside sim/fw (private to the clone).
set -e
cd "$(dirname "$0")/fw"
T=${TARGET:-esp32c3}; D=${DEMO:-widgets}
export IDF_PATH="${IDF_PATH:-$HOME/.espressif/v5.4.4/esp-idf}"; . "$IDF_PATH/export.sh" >/dev/null
python "$IDF_PATH/tools/idf.py" -B build-$T-$D -DIDF_TARGET=$T -DSDKCONFIG=build-$T-$D/sdkconfig -DDEMO=$D build 2>&1 | grep -E "error|Error|binary size"
(cd build-$T-$D && esptool.py --chip $T merge_bin -o ../../flash-$T-$D.bin --flash_mode dio --flash_size 4MB @flash_args >/dev/null)
ls -l ../flash-$T-$D.bin
