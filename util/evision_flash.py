#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Evision OEM bootloader (0c45:7687) flasher for the Redragon K580 (VS11K33A, CH555-family 8051).

The protocol and every guard below come from a disassembly of the OEM bootloader (0x0000-0x27FF of a stock
1.04 flash dump) and match Glitch752's USB capture (https://github.com/Glitch752/qmk-8051-vs11k28a).
Linux only: talks to the keyboard through hidraw. Python standard library only.

How the board boots: the bootloader jumps to the app (0x2800) only if code[0x27C0..0x27C3] == AA 55 33 CC.
The app's "enter bootloader" report (04 EE EE 00) erases the 64-byte page at 0x27C0 and resets, so the board
then stays in the bootloader across replugs until a flash completes. The bootloader writes the marker back
only after the checksum matches. So any abort, crash or unplug mid-flash leaves the board safely in the
bootloader, ready to retry.

Bootloader commands (SET_FEATURE, report ID 0, 64 B, on interface 1; GET_FEATURE returns
[cmd aa 55 xx][status x4][00 11 22 33], status FA FA FA FA = OK):
  01 aa 55                          hello
  02 aa 55                          erase marker page 0x27C0, then 0x2800-0xEFBF (64 B pages), blank-check
  03 aa 55 00 lo hi 00 00 nlo nhi   begin at addr (must be 0x2800), n 64-byte packets
  <n raw 64-byte packets>           programmed at 0x2800, 0x2840, ... (no upper bound in the bootloader!)
  04 aa 55 00 00 00 00 00 s0 s1 s2 s3   sum of bytes written; if it matches, writes the marker
  05 aa 55                          software reset (runs the app if the marker is present)

SAFETY BY CONSTRUCTION:
  * Never addresses anything below 0x2800 (the bootloader enforces this too) and caps the image at
    0x2800-0xEFBF (799 packets), the range the bootloader erases. Past that it would program unerased data
    flash or the WCH factory bootloader, or wrap around to 0x0000.
  * Image checks: LJMP at 0x2800 and a 13-entry LJMP interrupt table at 0x3000 (the bootloader forwards
    every interrupt there), all targets inside the app range.
  * Checks device identity (VID:PID on USB interface 1) and the bootloader's hello fingerprint before
    erasing. Checks status after every command and never sends reset (05) unless the checksum is OK.
  * DRY-RUN by default. Nothing is sent without --send.

Usage:
  python3 -I util/evision_flash.py extract FULL64K.bin OUT.bin   # cut the app region out of a full dump
  python3 -I util/evision_flash.py check IMAGE.bin               # validate an app image (no device access)
  python3 -I util/evision_flash.py status                        # which mode is the board in?
  python3 -I util/evision_flash.py enter [--send]                # app -> bootloader (commits to a reflash)
  python3 -I util/evision_flash.py flash IMAGE.bin [--send]      # bootloader: erase, write, verify, reset
  python3 -I util/evision_flash.py verify DUMP.bin IMAGE.bin     # compare a read-back dump to an image
"""
import sys, os, glob, argparse, array, fcntl, hashlib, time

def _ioc(d, t, nr, size): return (d << 30) | (size << 16) | (t << 8) | nr
def HIDIOCSFEATURE(n): return _ioc(3, ord('H'), 0x06, n)
def HIDIOCGFEATURE(n): return _ioc(3, ord('H'), 0x07, n)

APP_VID, APP_PID = 0x320f, 0x5000
BOOT_VID, BOOT_PID = 0x0c45, 0x7687
QMK_VID, QMK_PID = 0x3938, 0x1195        # QMK redragon/k580 (no Evision vendor interface)
APP_BASE = 0x2800
ERASE_END = 0xEFC0                       # bootloader erases pages 0xA0..0x3BE -> 0x2800..0xEFBF
PKT = 64
MAX_PACKETS = (ERASE_END - APP_BASE) // PKT   # 799
IVT_BASE, IVT_ENTRIES = 0x3000, 13       # bootloader ISRs at 0x03+8k forward to LJMP table at 0x3000+3k
STATUS_OK = b"\xfa\xfa\xfa\xfa"
HELLO_TAIL = b"\x00\x11\x22\x33"         # bootloader fingerprint in every GET_FEATURE reply
ENTER_REPORT = bytes([0x04, 0xee, 0xee]) + bytes(61)   # app: erase marker page 0x27C0 + reset

KNOWN = {   # sha256 of the raw (unpadded) app image -> label
    "75eb9250cb7842b82220cb1ffb7374042d49a7461b076ee79e0ea5fdaabdaea4":
        "stock 1.04 app region, as read from one K580",
    "4f7f1f17694774b452c46418e98981d30aa6274631da73428bcde1454a554ff5":
        "vendor 1.02 app region, from the Redragon updater",
}

def known_label(sha):
    return KNOWN.get(sha)

# --- image handling -------------------------------------------------------------------------------------
def ljmp_target(img, off):
    if off + 3 > len(img) or img[off] != 0x02:
        return None
    return (img[off + 1] << 8) | img[off + 2]

def load_image(path):
    """Load a raw app image based at 0x2800, validate it, return (padded bytes, info dict)."""
    raw = open(path, "rb").read()
    if len(raw) == 0x10000:
        raise SystemExit("%s is a full 64 KB image; cut the app region first with 'extract'" % path)
    errs = []
    if not raw:
        errs.append("empty image")
    n = (len(raw) + PKT - 1) // PKT
    if n > MAX_PACKETS:
        errs.append("image is %d bytes (%d packets); max is %d packets (0x%04x-0x%04x)"
                    % (len(raw), n, MAX_PACKETS, APP_BASE, ERASE_END - 1))
    end = APP_BASE + n * PKT
    t = ljmp_target(raw, 0)
    if t is None or not (APP_BASE <= t < end):
        errs.append("no LJMP into the app at 0x2800 (got %s)" % raw[:3].hex(" "))
    for k in range(IVT_ENTRIES):
        a = IVT_BASE + 3 * k
        t = ljmp_target(raw, a - APP_BASE)
        if t is None or not (APP_BASE <= t < end):
            errs.append("interrupt table entry %d at 0x%04x is not an LJMP into the app (%s)"
                        % (k, a, raw[a - APP_BASE:a - APP_BASE + 3].hex(" ")))
    img = raw + bytes(n * PKT - len(raw))     # pad with 0x00 (the erased value on this chip)
    cks = sum(img)
    if cks >= 1 << 24:
        errs.append("checksum 0x%x does not fit 24 bits" % cks)
    if errs:
        raise SystemExit("image rejected:\n  " + "\n  ".join(errs))
    sha = hashlib.sha256(raw).hexdigest()
    return img, {"path": path, "size": len(raw), "packets": n, "end": end - 1, "checksum": cks,
                 "sha256": sha, "label": known_label(sha)}

def print_image(info):
    print("image   : %s" % info["path"])
    print("sha256  : %s  (%s)" % (info["sha256"], info["label"] or "NOT a known image"))
    print("range   : 0x%04x-0x%04x  (%d bytes, %d x 64 B packets)" % (APP_BASE, info["end"], info["size"],
                                                                    info["packets"]))
    print("checksum: 0x%06x" % info["checksum"])

# --- device discovery (sysfs only) ----------------------------------------------------------------------
def find_nodes(vid, pid):
    """Return [(node, iface1, devpath)] for hidraw nodes of vid:pid."""
    hits = []
    for ue in glob.glob("/sys/class/hidraw/hidraw*/device/uevent"):
        txt = open(ue).read().upper()
        if ("HID_ID=0003:0000%04X:0000%04X" % (vid, pid)) not in txt:
            continue
        devpath = os.path.realpath(os.path.dirname(ue))
        hits.append(("/dev/" + ue.split("/")[4], ":1.1/" in devpath, devpath))
    return sorted(hits)

def iface1_node(vid, pid):
    hits = find_nodes(vid, pid)
    for node, i1, _ in hits:
        if i1:
            return node
    return None

def usb_devices(vid, pid):
    """Return the sysfs dirs of USB devices vid:pid (any firmware; no hidraw access needed)."""
    hits = []
    for d in glob.glob("/sys/bus/usb/devices/*"):
        try:
            if (int(open(d + "/idVendor").read(), 16), int(open(d + "/idProduct").read(), 16)) == (vid, pid):
                hits.append(d)
        except (OSError, ValueError):
            pass
    return sorted(hits)

def usb_attrs(d):
    out = {}
    for k in ("idVendor", "idProduct", "bcdDevice", "manufacturer", "product"):
        try:
            out[k] = open(os.path.join(d, k)).read().strip()
        except OSError:
            out[k] = None
    return out

def usb_strings(devpath):
    # devpath = .../1-2.2/1-2.2:1.1/0003:VVVV:PPPP.NNNN -> USB device dir is two levels up
    return usb_attrs(os.path.dirname(os.path.dirname(devpath)))

def mode():
    if find_nodes(BOOT_VID, BOOT_PID):
        return "bootloader"
    if find_nodes(APP_VID, APP_PID):
        return "app"
    if usb_devices(QMK_VID, QMK_PID):
        return "QMK"
    return None

def wait_for(vid, pid, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        node = iface1_node(vid, pid)
        if node:
            return node
        time.sleep(0.25)
    return None

def wait_usb(vid, pid, timeout):
    """Wait for any USB device vid:pid in sysfs (works for non-Evision firmware too)."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        hits = usb_devices(vid, pid)
        if hits:
            return hits[0]
        time.sleep(0.25)
    return None

# --- bootloader transport -------------------------------------------------------------------------------
def cmd_report(cmd, extra=b""):
    r = bytearray(PKT)
    r[0], r[1], r[2] = cmd, 0xaa, 0x55
    r[3:3 + len(extra)] = extra
    return bytes(r)

def set_feature(fd, data):
    buf = array.array("B", b"\x00" + data)      # report ID 0 (kernel strips it): 64-byte SET_REPORT
    fcntl.ioctl(fd, HIDIOCSFEATURE(len(buf)), buf, True)

def get_feature(fd):
    buf = array.array("B", bytes(PKT + 1))       # 65: report ID 0 + 64 data bytes
    fcntl.ioctl(fd, HIDIOCGFEATURE(len(buf)), buf, True)
    return bytes(buf[1:])

def expect_ok(fd, cmd, timeout, what, blind=False):
    """Poll GET_FEATURE until it echoes cmd with status OK. Exit (board stays in bootloader) otherwise.
    blind: skip the status read (fallback if GET_FEATURE doesn't work on this host); wait instead. Still safe:
    without a checksum match the bootloader never writes the boot marker, so reset returns to the bootloader."""
    if blind:
        time.sleep(2.5 if cmd == 0x02 else 0.2)
        print("  %-8s sent (no status check)" % what, flush=True)
        return None
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        try:
            r = get_feature(fd)
        except OSError as e:                      # busy (e.g. erasing): control transfer NAKs/times out
            last = "GET_FEATURE error: %s" % e
            time.sleep(0.5)
            continue
        last = r[:12].hex(" ")
        if r[0] == cmd and r[1:3] == b"\xaa\x55" and r[8:12] == HELLO_TAIL:
            if r[4:8] == STATUS_OK:
                print("  %-8s ok   (%s)" % (what, last), flush=True)
                return r
            raise SystemExit("ABORT: %s returned status %s (reply %s). Board stays in bootloader; safe to retry."
                             % (what, r[4:8].hex(" "), last))
        time.sleep(0.2)
    raise SystemExit("ABORT: no OK status for %s within %ds (last: %s). Board stays in bootloader; safe to retry."
                     % (what, timeout, last))

# --- subcommands ----------------------------------------------------------------------------------------
def do_extract(a):
    d = open(a.full, "rb").read()
    if len(d) != 0x10000:
        raise SystemExit("expected a full 64 KB image")
    region = d[APP_BASE:ERASE_END]
    last = max((i for i, b in enumerate(region) if b), default=-1)
    n = (last + PKT) // PKT
    out = region[:n * PKT]
    open(a.out, "wb").write(out)
    print("wrote %s: 0x%04x-0x%04x (%d bytes); 0x%04x-0x%04x is all 0x00 (erased) in the source"
          % (a.out, APP_BASE, APP_BASE + len(out) - 1, len(out), APP_BASE + len(out), ERASE_END - 1))
    _, info = load_image(a.out)
    print_image(info)

def do_check(a):
    _, info = load_image(a.image)
    print_image(info)
    print("OK: passes all image checks.")
    if not info["label"]:
        print("NOTE: unknown image. Make sure it can re-enter this bootloader (erase page 0x27C0 + reset);\n"
              "      otherwise the only way back is the WCH factory ISP.")

def do_status(a):
    for vid, pid, label in ((APP_VID, APP_PID, "app"), (BOOT_VID, BOOT_PID, "OEM bootloader")):
        for node, i1, devpath in find_nodes(vid, pid):
            s = usb_strings(devpath)
            print("%-15s %04x:%04x  %s  iface1=%s  bcdDevice=%s  %r / %r"
                  % (label, vid, pid, node, i1, s["bcdDevice"], s["manufacturer"], s["product"]))
    for d in usb_devices(QMK_VID, QMK_PID):     # QMK: a hub plus devices D0-D2, all with this VID:PID
        s = usb_attrs(d)
        print("%-15s %04x:%04x  %s  bcdDevice=%s  %r / %r"
              % ("QMK", QMK_VID, QMK_PID, os.path.basename(d), s["bcdDevice"], s["manufacturer"], s["product"]))
    m = mode()
    print("mode: %s" % (m or "keyboard not found"))

def do_enter(a):
    node = iface1_node(APP_VID, APP_PID)
    if not node:
        raise SystemExit("app-mode keyboard %04x:%04x (interface 1) not found; mode=%s" % (APP_VID, APP_PID, mode()))
    print("device : %04x:%04x  node %s (interface 1)" % (APP_VID, APP_PID, node))
    print("report : %s ... (64 B output report)" % ENTER_REPORT[:4].hex(" "))
    print("effect : app erases the boot marker page 0x27C0 and resets into the OEM bootloader (%04x:%04x).\n"
          "         The keyboard will NOT type again, even after a replug, until a flash completes."
          % (BOOT_VID, BOOT_PID))
    if not a.send:
        print("\nDRY-RUN. Nothing sent. Re-run with --send to transmit."); return
    fd = os.open(node, os.O_RDWR)
    try:
        os.write(fd, ENTER_REPORT)
    finally:
        os.close(fd)
    print("sent; waiting for bootloader ...", flush=True)
    bnode = wait_for(BOOT_VID, BOOT_PID, 15)
    if not bnode:
        raise SystemExit("bootloader did not appear within 15 s; run 'status'")
    do_status(a)

def do_flash(a):
    img, info = load_image(a.image)
    print_image(info)
    node = iface1_node(BOOT_VID, BOOT_PID)
    print("device  : %s" % ("%04x:%04x  node %s (interface 1)" % (BOOT_VID, BOOT_PID, node) if node
                            else "bootloader NOT present (mode=%s); run 'enter' (stock), or press Fn+Esc or hold "
                                 "Esc at plug-in (QMK) first" % mode()))
    begin = cmd_report(0x03, bytes([0x00, APP_BASE & 0xff, APP_BASE >> 8, 0x00, 0x00,
                                    info["packets"] & 0xff, info["packets"] >> 8]))
    finish = cmd_report(0x04, bytes(5) + info["checksum"].to_bytes(4, "little"))
    print("plan    : 01 hello -> 02 erase 0x2800-0xEFBF (~7 s) -> %s"
          % begin[:10].hex(" "))
    print("          -> %d data packets (%.0f ms apart) -> %s -> 05 reset (only if status OK)"
          % (info["packets"], a.gap * 1000, finish[:12].hex(" ")))
    if not info["label"]:
        print("WARNING : unknown image. If it can't re-enter this bootloader, only the WCH ISP can recover.")
    if not a.send:
        print("\nDRY-RUN. Nothing sent. Re-run with --send to flash."); return
    if not node:
        raise SystemExit("bootloader not present")
    fd = os.open(node, os.O_RDWR)
    try:
        set_feature(fd, cmd_report(0x01)); time.sleep(0.1)
        expect_ok(fd, 0x01, 5, "hello", a.no_status)
        set_feature(fd, cmd_report(0x02)); time.sleep(7.5)
        expect_ok(fd, 0x02, 30, "erase", a.no_status)
        set_feature(fd, begin); time.sleep(0.1)
        expect_ok(fd, 0x03, 5, "begin", a.no_status)
        for i in range(info["packets"]):
            set_feature(fd, img[i * PKT:(i + 1) * PKT])
            time.sleep(a.gap)
            if (i + 1) % 100 == 0 or i + 1 == info["packets"]:
                print("  data     %d/%d" % (i + 1, info["packets"]), flush=True)
        time.sleep(0.5)
        set_feature(fd, finish); time.sleep(1.0)
        expect_ok(fd, 0x04, 10, "checksum", a.no_status)
        set_feature(fd, cmd_report(0x05))
    finally:
        os.close(fd)
    vid, pid = (int(x, 16) for x in a.expect.split(":"))
    print("reset sent; waiting for %04x:%04x ..." % (vid, pid), flush=True)
    if not wait_usb(vid, pid, 30):
        raise SystemExit("%04x:%04x did not appear within 30 s; run 'status'" % (vid, pid))
    print("%04x:%04x enumerated" % (vid, pid))
    do_status(a)

def do_verify(a):
    dump = open(a.dump, "rb").read()
    img, info = load_image(a.image)
    base = 0 if len(dump) == 0x10000 else APP_BASE
    lo = APP_BASE - base
    got = dump[lo:lo + (ERASE_END - APP_BASE)]
    want = img + bytes((ERASE_END - APP_BASE) - len(img))
    if len(got) != len(want):
        raise SystemExit("dump too short: need 0x%04x-0x%04x" % (APP_BASE, ERASE_END - 1))
    bad = [i for i in range(len(want)) if got[i] != want[i]]
    if bad:
        raise SystemExit("MISMATCH: %d bytes differ, first at 0x%04x" % (len(bad), APP_BASE + bad[0]))
    print("MATCH: 0x%04x-0x%04x identical to %s" % (APP_BASE, ERASE_END - 1, a.image))

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("extract"); p.add_argument("full"); p.add_argument("out"); p.set_defaults(fn=do_extract)
    p = sub.add_parser("check"); p.add_argument("image"); p.set_defaults(fn=do_check)
    p = sub.add_parser("status"); p.set_defaults(fn=do_status)
    p = sub.add_parser("enter"); p.add_argument("--send", action="store_true"); p.set_defaults(fn=do_enter)
    p = sub.add_parser("flash"); p.add_argument("image"); p.add_argument("--send", action="store_true")
    p.add_argument("--gap", type=float, default=0.03, help="seconds between data packets (vendor tool ~0.031)")
    p.add_argument("--expect", default="%04x:%04x" % (APP_VID, APP_PID),
                   help="VID:PID the flashed firmware enumerates as (QMK K580: %04x:%04x)" % (QMK_VID, QMK_PID))
    p.add_argument("--no-status", action="store_true",
                   help="don't read GET_FEATURE status (fallback if it fails on this host); success = app reappears")
    p.set_defaults(fn=do_flash)
    p = sub.add_parser("verify"); p.add_argument("dump"); p.add_argument("image"); p.set_defaults(fn=do_verify)
    a = ap.parse_args()
    a.fn(a)

if __name__ == "__main__":
    main()
