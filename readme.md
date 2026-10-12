# QMK for CH555-family 8051 keyboards

A fork of [vial-qmk](https://github.com/vial-kb/vial-qmk), itself a fork of [QMK](https://github.com/qmk/qmk_firmware),
that builds QMK for 8051 microcontrollers in the WCH CH555 family with the SDCC compiler. The branch `k580-ch55x`
supports the Redragon K580 RGB (Evision VS11K33A), flashed through the keyboard's own Evision bootloader.

Despite the vial-qmk origin and the repository name, Vial does not work here. On the K580, Vial and the VIA
code it needs add about 7.8 KB of code and more internal RAM than the build has free, so the link fails.
Keymaps are changed in the source and flashed as a new build.

## Credits

- **vuhuycan**, [vuhuycan/qmk-8051](https://github.com/vuhuycan/qmk-8051): the port of vial-qmk to the 8051 and
  SDCC, including the MCS51 platform code and the CH555 USB stack.
- **RakeRaven**, [RakeRaven/qmk-8051](https://github.com/RakeRaven/qmk-8051): the Redragon K580 keyboard
  definition and fixes to the CH555 USB stack, NKRO and the lock LEDs.
- **Glitch752**, [Glitch752/qmk-8051-vs11k28a](https://github.com/Glitch752/qmk-8051-vs11k28a): reverse
  engineering of the Evision OEM bootloader's flash protocol.

## Status

Experimental. The firmware has been tested on **one** keyboard, a Redragon K580 RGB with an Evision VS11K33A MCU
and stock firmware 1.04, connected to Linux hosts only. Other keyboards, other K580 revisions, Windows and macOS
have not been tested.

| Keyboard | MCU | Flashed through | State on this branch |
|---|---|---|---|
| [Redragon K580 RGB](keyboards/redragon/k580/) | Evision VS11K33A | Evision OEM bootloader (`0c45:7687`) | Works; tested on one keyboard |
| [Reccarz KW75S](keyboards/reccarz/kw75s/) | HFD801KJC | WCH factory ISP bootloader | Does not build: its keymap lacks the `*_user` functions listed under [Toolchain limits](#toolchain-limits), and the link runs out of internal RAM |

### Redragon K580 RGB

Works:
- All keys of the main block, the navigation cluster and the numpad; NKRO.
- Fn layer: Fn+Esc enters the bootloader, Fn+F1–F12 send the media and application keys printed on the keycaps,
  Fn+Win toggles Win Lock.
- Media keys and the volume roller. The roller toggle key switches the roller between volume and key LED
  brightness.
- G1–G5 and Rec send F13–F18, for shortcuts set in the operating system.
- Num Lock, Caps Lock, Scroll Lock and Win Lock LEDs. They turn off while the host is suspended.
- Key LEDs (RGB; one colour on G1–G5, Rec and the media keys) and side lights, driven by the CH555 LED
  controller. Lighting effects with separate settings for the keys and the side lights, set on the Fn layer
  (see [the K580 readme](keyboards/redragon/k580/readme.md#default-keymap)). The LEDs turn off while the host
  is suspended.
- The lighting settings and the roller mode are saved in the CH555 data flash and kept at power-off.
- Holding Esc while plugging in the keyboard enters the bootloader.

Not implemented:
- USB serial number.
- QMK's EEPROM: the driver stores nothing, so QMK settings kept in EEPROM (eeconfig) are not saved. The
  keyboard's own settings use the data flash instead.
- Vial: it did not work in the original port, and this branch has no Vial keymap.

Other limits:
- About 15.3 KB of the application region (0x2800–0xEFBF, 51,136 bytes) is free.
- To save flash, the K580 build turns off tap and one-shot keys, `send_string`, Grave Escape and the magic-key
  swap settings; the list is at the end of
  [keyboards/redragon/k580/rules.mk](keyboards/redragon/k580/rules.mk).
- QMK enumerates as a USB hub (`3938:1195`) with three devices: boot keyboard, media keys and NKRO keyboard.
- The backup and flash tools (`util/k580_dump.py`, `util/evision_flash.py`) run on Linux only.
- Other K580 revisions may use a different MCU; one keyboard database lists the K580 with a Sonix SN32F248B.
  [keyboards/redragon/k580/readme.md](keyboards/redragon/k580/readme.md) describes how to check a keyboard
  before flashing.

## Building

The toolchain is SDCC 4.6.0 or 4.4.0 and the QMK CLI. SDCC 4.5.0 stops with an internal error on the CH555 USB
code (SDCC bug #3894). `qmk compile` fails at its final link step by design in this fork. A separate script
assembles and links the objects.

- Redragon K580: `util/k580_build.sh` runs the whole build in Docker and writes `redragon_k580_default_oem.bin`.
  Backup, flash and restore steps are in [keyboards/redragon/k580/readme.md](keyboards/redragon/k580/readme.md).
- `compile.sh` (from the original port) links an image at 0x0000 for flashing through the WCH factory ISP
  bootloader. No keyboard on this branch currently builds that way.

## Toolchain limits

These limits of SDCC and of this port affect keyboard and keymap code:

- Pins are named `PORT<port>_<pin>` (for example `PORT0_1`). `info.json` uses `"processor": "ch555"` and
  `"bootloader": "ch55xbl"`.
- SDCC has no weak symbols. The quantum stubs of `keyboard_post_init_user`, `process_record_user`,
  `pre_process_record_user`, `post_process_record_user`, `housekeeping_task_user`, `suspend_power_down_user` and
  `suspend_wakeup_init_user` are left out of SDCC builds, so every keymap must define all of them. The `*_kb`
  stubs are still compiled, so keyboard-level code cannot override them; board setup goes in the keymap's
  `*_user` functions.
- QMK's `rgb_matrix` does not compile with SDCC.
- SDCC compiles a copy of every `static inline` function of a header into each file that includes the header,
  used or not. Use a macro, or define the function once in a `.c` file.
- Some files do not include the keyboard's `config.h` (for example `tmk_core/protocol/ch555/usb_device.c`,
  `drivers/eeprom/eeprom_driver.c`, `quantum/keycode_config.c`). Defines they need, and defines that change
  shared types such as `NO_ACTION_TAPPING`, go in `OPT_DEFS` in `rules.mk`.
- `printf` and the console are disabled to save flash. A keymap-level `config.h` can enable `printf` again, and
  the K580 image then no longer fits.
