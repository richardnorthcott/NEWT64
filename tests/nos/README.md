# Newton OS functions: checked against the Newton ROM

`src/newt_core/NewtNOS.c` adds global functions that a Newton running Newton
OS 2.1 has and NEWT/0 lacked: maths, binary objects, Mac Roman characters,
strings, arrays, sets, sorting, StrTokenize, Date and TotalMinutes.

`probe.newt` calls each of them 227 ways and prints `label => value`.
`rom-717006.out` holds the answers that ROM 717006 (MessagePad 2x00, Newton OS
2.1) gave to the same calls. They were measured by running the probe in an
emulator, twice, with identical results.

    newt tests/nos/probe.newt > out.txt
    python3 tests/nos/compare.py out.txt

219 answers match. The other 8 are known and listed in `compare.py`:

- NEWT/0's strings are UTF-8 indexed by byte, where the ROM's are UTF-16.
- The ROM throws object errors as `evt.ex.fr` with an `error` slot.
- The ROM's calendar counts 2100 as a leap year.
- A symbol keeps the spelling it was first interned with.

The probe also documents ROM behaviour that is easy to get wrong:

- `Floor` returns an integer but `Round` a real.
- `StringToNumber` always returns a real.
- `StrPos` ignores case.
- `SetDifference` removes only the first match of each item.
- `ExtractChar` reads Mac Roman.
