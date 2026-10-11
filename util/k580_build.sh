#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Build redragon/k580 for the Evision OEM bootloader and make an app image based at 0x2800 (Esc check and
# interrupt table at 0x3000; see util/evision_oem_image.py).
#
#   util/k580_build.sh [KEYMAP]     (default: default)
#
# Steps: `qmk compile` (its final link fails by design in this fork), then assemble and link as compile.sh
# does, but linked at --code-loc 0x3027 instead of 0x0000, then util/evision_oem_image.py.
# Runs the toolchain in Docker (util/k580_build.Dockerfile; SDCC version = its ARG SDCC_VER).
# Output: redragon_k580_<keymap>_oem.bin (plus .ihx, .map, .mem) in the top directory of the tree.
set -euo pipefail
tree=$(cd "$(dirname "$0")/.." && pwd)
km=${1:-default}
kb=redragon_k580
code_loc=0x3027
obj=.build/obj_${kb}_${km}
sdcc_ver=$(sed -n 's/^ARG SDCC_VER=//p' "$tree/util/k580_build.Dockerfile")
image=vial-qmk-8051-k580:sdcc-${sdcc_ver:?}

run() { docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$tree":/qmk -w /qmk "$image" "$@"; }

if ! docker image inspect "$image" >/dev/null 2>&1; then
    tar -cf - -C "$tree/util" k580_build.Dockerfile -C "$tree" requirements.txt |
        docker build -t "$image" -f k580_build.Dockerfile -
fi
rm -rf "$tree/.build"
mkdir -p "$tree/.build"
run qmk compile -kb redragon/k580 -km "$km" -e EVISION_OEM_BOOTLOADER=yes >"$tree/.build/k580_qmk.log" 2>&1 || true
if ! ls "$tree/$obj"/quantum/main.asm >/dev/null 2>&1 || grep -qE ":[0-9]+: (syntax )?error|Internal error" "$tree/.build/k580_qmk.log"; then
    echo "qmk compile failed before the link step; see .build/k580_qmk.log" >&2; exit 1
fi

run bash -euc "
    ./util/del_bit_bank_area.py $obj/protocol/ch555/usb_device.asm
    for f in \$(find $obj -name '*.asm' -type f); do sdas8051 -plosgffw \"\${f%.asm}.rel\" \"\$f\"; done
    # Link order sets code placement, and find's order depends on the filesystem: sort it. The SDCC manual
    # requires the module with main() first.
    rels=\$(find $obj -name '*.rel' -type f ! -path $obj/quantum/main.rel | LC_ALL=C sort)
    sdcc -mmcs51 --model-large --iram-size 0x0100 --xram-size 0x0f00 --xram-loc 0x0100 \
         --code-loc $code_loc --code-size 0xefc0 --opt-code-speed \
         $obj/quantum/main.rel \$rels -o ${kb}_${km}_oem.ihx
" >"$tree/.build/k580_link.log" 2>&1 || { echo "link failed; see .build/k580_link.log" >&2; exit 1; }

python3 -I "$tree/util/evision_oem_image.py" sdcc "$tree/${kb}_${km}_oem.ihx" "$tree/$obj/quantum/main.asm" \
    "$tree/${kb}_${km}_oem.bin" --code-loc $code_loc
grep -E "ROM/EPROM/FLASH|EXTERNAL RAM|Stack starts" "$tree/${kb}_${km}_oem.mem"
python3 -I "$tree/util/evision_flash.py" check "$tree/${kb}_${km}_oem.bin"
