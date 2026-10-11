# Redragon K580 RGB

QMK for the Redragon K580 RGB with an Evision VS11K33A MCU (an 8051 in the WCH CH555 family). The firmware is
flashed through the keyboard's own Evision bootloader, which stays in place, so the stock firmware can be
restored from a backup.

Tested on one keyboard (stock firmware 1.04), with Linux hosts only. What works and what is missing:
[readme.md](../../../readme.md#redragon-k580-rgb) in the top directory.

## Check the keyboard first

- With the stock firmware, the keyboard enumerates as `320f:5000` (`lsusb`). The tested keyboard reports
  bcdDevice 0x0104 (firmware 1.04).
- The backup in step 1 prints whether the boot marker `AA 55 33 CC` is at 0x27C0. If it is not, the flash layout
  below does not apply: do not flash.

Other K580 revisions may use a different MCU. This firmware does not apply to them.

## Requirements

- Linux, Python 3 (standard library only), Docker (for the build).
- Access to the keyboard's hidraw nodes: install the udev rule and be a member of the `plugdev` group.
  ```
  sudo cp util/udev/70-redragon-k580.rules /etc/udev/rules.d/
  sudo udevadm control --reload && sudo udevadm trigger
  ```

Run the commands below from the top directory of this repository. `util/k580_dump.py` and
`util/evision_flash.py` send nothing to the keyboard unless `--send` is given; without it they print what they
would do.

## Flash layout and bootloader

| Range | Contents |
|---|---|
| 0x0000–0x27FF | Evision OEM bootloader (USB `0c45:7687`). The tools here never write it. |
| 0x27C0 | Boot marker `AA 55 33 CC`. The bootloader starts the application only while it is present. |
| 0x2800–0xEFBF | Application: stock firmware or QMK. The bootloader erases and programs this range. |
| 0xF400– | WCH factory ISP bootloader (`4348:55e0`). Not used: flashing through it overwrites the OEM bootloader. |

Entering the bootloader erases the boot marker. The keyboard then stays in the bootloader, also after a replug,
until a flash completes. If a flash fails or is interrupted, the keyboard stays in the bootloader and the flash
command can be run again. `util/evision_flash.py` writes only 0x2800–0xEFBF, and checks the image before it
erases anything.

`python3 -I util/evision_flash.py status` shows which mode the keyboard is in: stock firmware, bootloader or QMK.

## 1. Back up the stock firmware

QMK does not implement the read command that the backup uses. Make the backup before flashing QMK.

```
python3 -I util/k580_dump.py --out k580_stock_1.bin --send
python3 -I util/k580_dump.py --out k580_stock_2.bin --send
cmp k580_stock_1.bin k580_stock_2.bin
python3 -I util/evision_flash.py extract k580_stock_1.bin k580_stock_app.bin
```

`k580_stock_1.bin` is the full 64 KB code flash, including the OEM bootloader. `k580_stock_app.bin` is its
application region, the image that restores the stock firmware. Keep both.

## 2. Build

```
util/k580_build.sh            # keymap "default"
```

The output is `redragon_k580_default_oem.bin`. The image contains a check for Esc at plug-in at 0x2803 and the
interrupt table that the bootloader jumps to at 0x3000 (made by `util/evision_oem_image.py`).

## 3. Flash QMK

From the stock firmware:

```
python3 -I util/evision_flash.py enter --send
python3 -I util/evision_flash.py flash redragon_k580_default_oem.bin --expect 3938:1195 --send
```

To flash a new build over QMK, enter the bootloader with Fn+Esc, or hold Esc while plugging in the keyboard, then
run the `flash` command. The Esc check runs before QMK starts, so it also works if a QMK build does not start.
It has been tested on a modified stock image that contains the same code, not on a QMK build.

## Restore the stock firmware

Enter the bootloader (Fn+Esc in QMK), then:

```
python3 -I util/evision_flash.py flash k580_stock_app.bin --send
```

To compare the flash with the backup while the stock firmware runs:

```
python3 -I util/k580_dump.py --start 0x2800 --end 0xEFC0 --out readback.bin --send
python3 -I util/evision_flash.py verify readback.bin k580_stock_app.bin
```

On the tested keyboard, a restore of firmware 1.04 gave a full 64 KB read-back identical to the original backup.

## Default keymap

- Layer 0: the printed layout. Media keys: previous, play/pause, next. Roller: volume; roller press: Mute.
- Layer 1 (Fn): Fn+Esc enters the bootloader (`QK_BOOT`). Fn+Win toggles Win Lock, which blocks both Win keys and
  lights the Win Lock LED. Win Lock is off after power-up.
- G1–G5 and Rec: F13–F17 and F18, for shortcuts set in the operating system.
- LEDs: a fixed test pattern. The Esc and Caps Lock rows red, the number and Shift rows green, the Tab and Ctrl
  rows blue (by matrix row, so the numpad differs); G1–G5 and the side lights white; Rec and the media keys red.

## Hardware

- Key matrix: 7 rows × 18 columns. Rows P4.0–P4.6; columns P0.0–P0.7, P3.0–P3.7, P7.0–P7.1.
- Port 5: P5.5/P5.6 volume roller; P5.4 indicator LED enable (active low); P5.3, P5.2, P5.1, P5.0 Num Lock,
  Caps Lock, Scroll Lock, Win Lock LEDs (high = on).
- Key LEDs and side lights: the CH555 LED controller, with red, green and blue rows on P4, P2 and P1 and common
  lines on P0, P3 and P7, shared with the key matrix. LED row n on common line c is the LED of the key in matrix
  row n, column c (rows 0–6); LED row 7 on common lines 0–15 drives the side lights. The keys are read in the
  LED controller's interrupt, on each common line ([led_dma.c](led_dma.c)).
