#!/usr/bin/env python3
"""Sim addition. Assemble out/png/fb-NNNN-<virtual ms>.png into an animated GIF; each frame is shown for its real
virtual-time gap to the next (clamped 50..1000 ms). Needs Pillow. Usage: frames2gif.py OUT.gif PNG..."""
import sys
from PIL import Image
out, files = sys.argv[1], sorted(sys.argv[2:])
ts = [int(f.rsplit("-", 1)[1][:-4]) for f in files]
dur = [max(50, min(1000, b - a)) for a, b in zip(ts, ts[1:])] + [1000]
ims = [Image.open(f).convert("RGB").quantize(128, dither=Image.Dither.NONE) for f in files]
ims[0].save(out, save_all=True, append_images=ims[1:], duration=dur, loop=0, optimize=True)
print(out, len(ims), "frames", sum(dur) / 1000, "s")
