#!/usr/bin/env python3
"""Sim addition. Convert peers/ili9341.py frame-memory dumps (raw u16 RGB565, 240 cols x 320 rows in panel memory
order) to PNG with the stdlib. After MADCTL MV (landscape) the picture is the transpose: x = memory row, y = memory
column. Usage: raw2png.py [--scale N] OUT_DIR FILE.raw ..."""
import struct, sys, zlib
from array import array

def png(path, w, h, rgb):
    raw = b"".join(b"\0" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))

def convert(src, dst, scale=1):
    a = array("H"); a.frombytes(open(src, "rb").read())      # little-endian host assumed
    W, H = 320, 240
    out = bytearray()
    for y in range(H):
        row = bytearray()
        for x in range(W):
            p = a[x * 240 + y]
            r, g, b = (p >> 11) & 31, (p >> 5) & 63, p & 31
            row += bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))) * scale
        out += bytes(row) * scale
    png(dst, W * scale, H * scale, bytes(out))
    return any(v for v in a)

if __name__ == "__main__":
    args = sys.argv[1:]
    s = 1
    if args[0] == "--scale":
        s = int(args[1]); args = args[2:]
    for f in args[1:]:
        ok = convert(f, "%s/%s.png" % (args[0], f.rsplit("/", 1)[-1][:-4]), s)
        if not ok: print("BLANK", f)
