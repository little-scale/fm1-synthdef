#!/usr/bin/env python3
"""Check the exact SDK packaging bytes before the upstream build (which only warns)."""
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "dependencies/ac79-sdk/cpu/wl82/tools"
expected = {
    "uboot.boot": "4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4",
    "cfg_tool.bin": "276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b",
    "cfg/eq_cfg_hw.bin": "41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d",
}
for name, digest in expected.items():
    actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
    if actual != digest:
        raise SystemExit(f"SDK mismatch: {name}: {actual}, expected {digest}")
print("SDK: all three packaging files match the pinned upstream hashes")
