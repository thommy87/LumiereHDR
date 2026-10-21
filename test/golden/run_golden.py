#!/usr/bin/env python3
"""Golden-image regression check for every tone-mapping operator.

Renders one synthetic HDR scene with luminance-hdr-cli for each operator
and compares simple image statistics (per-channel mean, standard
deviation and 5/50/95th percentiles, on a 0-255 scale) with the
reference values in reference.json. Statistics rather than pixels keep
the check stable across compilers while still catching any change in
how an operator renders.

Usage:
  run_golden.py CLI [--update]

With --update (or when reference.json is missing) the current values are
written to reference.json and printed, and the check passes.
"""
import json
import math
import os
import struct
import subprocess
import sys
import tempfile

from PIL import Image

OPERATORS = [
    "ashikhmin", "drago", "durand", "fattal", "ferradans", "ferwerda",
    "kimkautz", "lischinski", "mai", "mantiuk06", "mantiuk08", "pattanaik",
    "reinhard02", "reinhard05", "vanhateren",
]
WIDTH, HEIGHT = 192, 128
# allowed absolute difference on the 0-255 scale
TOLERANCE = 2.0
HERE = os.path.dirname(os.path.abspath(__file__))
REFERENCE = os.path.join(HERE, "reference.json")


def float_to_rgbe(r, g, b):
    v = max(r, g, b)
    if v < 1e-32:
        return bytes((0, 0, 0, 0))
    mantissa, exponent = math.frexp(v)
    scale = mantissa * 256.0 / v
    return bytes((int(r * scale), int(g * scale), int(b * scale), exponent + 128))


def scene(x, y):
    """A sky gradient, a dark foreground with texture and two hot spots:
    about 16 stops of dynamic range, fully deterministic."""
    u, v = x / (WIDTH - 1), y / (HEIGHT - 1)
    if v < 0.55:
        base = 2.0 + 30.0 * (1.0 - v)
        r, g, b = 0.6 * base, 0.8 * base, 1.2 * base
    else:
        texture = 1.0 + 0.5 * math.sin(40 * u) * math.sin(25 * v)
        base = 0.02 + 0.3 * texture * (1.0 - u)
        r, g, b = 0.9 * base, 0.7 * base, 0.4 * base
    for cx, cy, peak in ((0.75, 0.2, 2000.0), (0.2, 0.7, 150.0)):
        d2 = (u - cx) ** 2 + ((v - cy) * HEIGHT / WIDTH) ** 2
        glow = peak * math.exp(-d2 / 0.0015)
        r, g, b = r + glow, g + 0.9 * glow, b + 0.7 * glow
    return r, g, b


def write_scene(path):
    with open(path, "wb") as f:
        f.write(b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n")
        f.write(f"-Y {HEIGHT} +X {WIDTH}\n".encode())
        for y in range(HEIGHT):
            # flat (uncompressed) scanlines; first pixel never starts 2,2
            f.write(b"".join(float_to_rgbe(*scene(x, y)) for x in range(WIDTH)))


def percentile(sorted_values, p):
    k = (len(sorted_values) - 1) * p
    lo, hi = math.floor(k), math.ceil(k)
    return sorted_values[lo] + (sorted_values[hi] - sorted_values[lo]) * (k - lo)


def stats(path):
    image = Image.open(path).convert("RGB")
    result = {}
    for name, channel in zip("RGB", image.split()):
        values = list(channel.getdata())
        n = len(values)
        mean = sum(values) / n
        std = math.sqrt(sum((v - mean) ** 2 for v in values) / n)
        ordered = sorted(values)
        result[name] = [round(mean, 2), round(std, 2)] + [
            round(percentile(ordered, p), 2) for p in (0.05, 0.5, 0.95)]
    return result


def main():
    cli = sys.argv[1]
    update = "--update" in sys.argv or not os.path.exists(REFERENCE)
    current = {}
    with tempfile.TemporaryDirectory() as tmp:
        hdr = os.path.join(tmp, "scene.hdr")
        write_scene(hdr)
        for op in OPERATORS:
            out = os.path.join(tmp, f"{op}.png")
            subprocess.run([cli, "-l", hdr, "--tmo", op, "-o", out],
                           check=True, stdout=subprocess.DEVNULL)
            current[op] = stats(out)

    if update:
        with open(REFERENCE, "w") as f:
            json.dump(current, f, indent=1, sort_keys=True)
            f.write("\n")
        print("GOLDEN-REFERENCE-BEGIN")
        print(json.dumps(current, sort_keys=True))
        print("GOLDEN-REFERENCE-END")
        return 0

    with open(REFERENCE) as f:
        reference = json.load(f)
    failures = []
    for op in OPERATORS:
        for channel in "RGB":
            for label, got, want in zip(("mean", "std", "p5", "p50", "p95"),
                                        current[op][channel],
                                        reference[op][channel]):
                if abs(got - want) > TOLERANCE:
                    failures.append(f"{op} {channel} {label}: {got} (reference {want})")
    for line in failures:
        print("CHANGED", line)
    print(f"{len(OPERATORS)} operators checked, {len(failures)} values changed")
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary and failures:
        # make an intended change easy to review and to record
        with open(summary, "a") as f:
            fence = "```"
            f.write(f"### Golden-image changes\n{fence}\n")
            f.write("\n".join(failures))
            f.write(f"\n{fence}\nCurrent values (paste into reference.json "
                    f"if intended):\n{fence}json\n")
            f.write(json.dumps(current, sort_keys=True))
            f.write(f"\n{fence}\n")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
