#!/usr/bin/env python3
"""Compares NEWT/0's answers to the probe with ROM 717006's (rom-717006.out).

  newt tests/nos/probe.newt > out.txt && python3 tests/nos/compare.py out.txt

Reals are compared to five significant digits, symbols and slot names
ignoring case. Exits 1 if any answer differs other than the known ones.
"""
import pathlib
import re
import sys

KNOWN = {  # label: why NEWT/0 still differs
    "Upcase latin1": "strings are UTF-8 indexed by byte; the ROM's are UTF-16",
    "Upcase e-acute": "strings are UTF-8 indexed by byte",
    "Downcase E-acute": "strings are UTF-8 indexed by byte",
    "Upcase len": "strings are UTF-8 indexed by byte",
    "ExtractLong big": "the ROM throws object errors as evt.ex.fr with an error slot",
    "ExtractBytes class nil": "the ROM throws object errors as evt.ex.fr with an error slot",
    "Date 2100": "the ROM's calendar counts 2100 as a leap year",
    "SymbolName(case)": "a symbol keeps the spelling it was first interned with",
}


def answers(text):
    out = {}
    for line in text.splitlines():
        if " => " in line:
            k, v = line.split(" => ", 1)
            out[k] = v
    return out


def norm(v):
    v = re.sub(r"\br(-?\d+(?:\.\d*)?(?:e[-+]?\d+)?)", lambda m: "r%.5g" % float(m.group(1)), v)
    return v.lower()


rom = answers((pathlib.Path(__file__).parent / "rom-717006.out").read_text())
got = answers(pathlib.Path(sys.argv[1]).read_text())
same, failed = 0, 0
for label, value in rom.items():
    mine = got.get(label, "(missing)")
    if norm(mine) == norm(value):
        same += 1
    elif label in KNOWN:
        print(f"known   {label}: ROM {value}, NEWT {mine} ({KNOWN[label]})")
    else:
        failed += 1
        print(f"DIFFERS {label}: ROM {value}, NEWT {mine}")
print(f"{same} of {len(rom)} match, {len(rom) - same - failed} known differences, {failed} unexpected")
sys.exit(1 if failed else 0)
