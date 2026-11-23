#!/usr/bin/env python3
"""Compares a build's answers to tests/nos/probe.newt with expected.out.

  newt64 tests/nos/probe.newt > out.txt && python3 tests/nos/compare.py out.txt

Reals are compared to five significant digits, and symbols and slot names
ignoring case. Exits 1 if any answer differs.
"""
import pathlib
import re
import sys


def answers(text):
    out = {}
    for line in text.splitlines():
        if " => " in line:
            k, v = line.split(" => ", 1)
            out[k] = v.rstrip()
    return out


def norm(v):
    v = re.sub(r"\br(-?\d+(?:\.\d*)?(?:e[-+]?\d+)?)", lambda m: "r%.5g" % float(m.group(1)), v)
    return v.lower()


expected = answers((pathlib.Path(__file__).parent / "expected.out").read_text())
got = answers(pathlib.Path(sys.argv[1]).read_text())
differ = 0
for label, value in expected.items():
    mine = got.get(label, "(missing)")
    if norm(mine) != norm(value):
        differ += 1
        print(f"DIFFERS {label}: expected {value}, got {mine}")
print(f"{len(expected) - differ} of {len(expected)} answers as expected")
sys.exit(1 if differ else 0)
