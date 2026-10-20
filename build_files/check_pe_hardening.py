#!/usr/bin/env python3
"""Fail if a Windows PE file was linked without ASLR, high-entropy ASLR or DEP.

Usage: check_pe_hardening.py FILE [FILE ...]
"""
import struct
import sys

REQUIRED = {
    0x0020: "HIGH_ENTROPY_VA",
    0x0040: "DYNAMIC_BASE (ASLR)",
    0x0100: "NX_COMPAT (DEP)",
}
OPTIONAL = {0x4000: "GUARD_CF (Control Flow Guard)"}


def dll_characteristics(path):
    with open(path, "rb") as f:
        data = f.read(4096)
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE file")
    optional_header = pe + 24
    # same offset for PE32 and PE32+
    return struct.unpack_from("<H", data, optional_header + 70)[0]


def main(paths):
    failed = False
    for path in paths:
        flags = dll_characteristics(path)
        missing = [name for bit, name in REQUIRED.items() if not flags & bit]
        extra = [name for bit, name in OPTIONAL.items() if flags & bit]
        status = "FAIL" if missing else "ok"
        print(f"{status:4} {path}: 0x{flags:04x}"
              + (f" missing {', '.join(missing)}" if missing else "")
              + (f" (+{', '.join(extra)})" if extra else ""))
        failed |= bool(missing)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
