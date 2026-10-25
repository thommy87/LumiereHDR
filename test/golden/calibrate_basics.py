#!/usr/bin/env python3
"""Measures how each operator's own parameters change brightness, global
contrast and local detail of the golden test scene.

The basic sliders of the tone mapping panel (brightness, contrast, detail)
are mapped onto one parameter per operator; this script shows which
parameter moves which quantity, in which direction and how strongly.

Usage:
  calibrate_basics.py CLI

Prints one row per parameter value, and writes the table to the job
summary when run in GitHub Actions.
"""
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_golden import write_scene  # noqa: E402

# operator, CLI option, values from low to high
SWEEPS = [
    ("mantiuk06", "tmoM06Contrast", [0.05, 0.1, 0.3, 0.6, 1.0]),
    ("mantiuk06", "tmoM06Detail", [1, 2, 5, 10, 30]),
    ("mantiuk08", "tmoM08ConstrastEnh", [0.5, 1, 2, 4, 8]),
    ("fattal", "tmoFatAlpha", [0.1, 0.5, 1.0, 1.5, 2.0]),
    ("fattal", "tmoFatBeta", [0.75, 0.8, 0.85, 0.9, 0.95]),
    ("ferradans", "tmoFerRho", [-10, -5, 0, 5, 10]),
    ("ferradans", "tmoFerInvAlpha", [1, 5, 10, 15, 20]),
    ("drago", "tmoDrgBias", [0.6, 0.7, 0.8, 0.9, 1.0]),
    ("durand", "tmoDurBase", [1, 3, 5, 7, 10]),
    ("durand", "tmoDurSigmaS", [1, 2, 4, 8, 16]),
    ("reinhard02", "tmoR02Key", [0.05, 0.1, 0.18, 0.3, 0.5]),
    ("reinhard02", "tmoR02Phi", [1, 4, 8, 16, 32]),
    ("reinhard05", "tmoR05Brightness", [-20, -10, 0, 10, 20]),
    ("reinhard05", "tmoR05Lightness", [0.0, 0.25, 0.5, 0.75, 1.0]),
    ("ashikhmin", "tmoAshLocal", [0.1, 0.3, 0.5, 0.7, 0.9]),
    ("pattanaik", "tmoPatMultiplier", [0.5, 1, 2, 5, 10]),
    ("ferwerda", "tmoFerwerdaMul", [0.2, 0.5, 1, 2, 5]),
    ("kimkautz", "tmoKimKautzC1", [0.5, 1, 2, 3, 5]),
    ("kimkautz", "tmoKimKautzC2", [0.5, 1, 1.5, 2, 3]),
    ("vanhateren", "tmoVanHaterenPupilArea", [1, 5, 10, 20, 50]),
    ("lischinski", "tmoLischinskiAlpha", [0.1, 0.3, 0.5, 0.7, 0.9]),
]


def measure(path):
    """mean and standard deviation of the luminance, and local detail:
    the mean difference to a slightly blurred copy (0-255 scale)."""
    grey = Image.open(path).convert("L")
    values = list(grey.getdata())
    n = len(values)
    mean = sum(values) / n
    std = (sum((v - mean) ** 2 for v in values) / n) ** 0.5
    blurred = list(grey.filter(ImageFilter.GaussianBlur(2)).getdata())
    detail = sum(abs(a - b) for a, b in zip(values, blurred)) / n
    return mean, std, detail


def main():
    cli = sys.argv[1]
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        hdr = os.path.join(tmp, "scene.hdr")
        write_scene(hdr)
        for op, option, values in SWEEPS:
            for value in values:
                out = os.path.join(tmp, "out.png")
                result = subprocess.run(
                    [cli, "-l", hdr, "--tmo", op, f"--{option}", str(value),
                     "-o", out],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if result.returncode != 0 or not os.path.exists(out):
                    rows.append((op, option, value, None))
                    continue
                rows.append((op, option, value, measure(out)))
                os.remove(out)

    lines = ["| operator | parameter | value | mean | contrast (std) | detail |",
             "|---|---|---|---|---|---|"]
    for op, option, value, m in rows:
        if m is None:
            lines.append(f"| {op} | {option} | {value} | failed | | |")
        else:
            lines.append(f"| {op} | {option} | {value} | {m[0]:.1f} | "
                         f"{m[1]:.1f} | {m[2]:.2f} |")
    table = "\n".join(lines)
    print(table)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write("### Basic sliders calibration\n" + table + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
