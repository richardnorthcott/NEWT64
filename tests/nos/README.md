# Newton OS functions (NewtNOS.c)

`src/newt_core/NewtNOS.c` adds about 150 global functions from the Newton OS
2.1 API that NEWT/0 lacked. They keep that API's names, arguments and results.

- **Numbers:** Floor, Ceiling, Round, Trunc, Real, Abs, Signum, Max, Min,
  Random, BXor, Ticks, and the C maths functions (Sqrt, Sin, Pow, Atan2, Fmod,
  Rint, IsNaN and the rest).
- **Binary objects:** the Extract and Stuff functions for Byte, Word, Long,
  XLong, Char, UniChar, Bytes, CString and PString, plus BinEqual and
  BinaryMunger. Characters are converted to and from Mac Roman.
- **Strings:**
  - editing: StrMunger, Upcase, Downcase, Capitalize(Words), TrimString,
    StripDiacriticals, StrReverse, SubstituteChars, StrConcat;
  - searching and comparing: StrCompare, StrPosExact, CharPos, StrTokenize;
  - numbers: StringToNumber, NumberStr;
  - characters and symbols: the Is… character tests, SymbolName, MakeSymbol,
    GetFunctionArgCount.
- **Arrays and sets:** ArrayInsert, ArrayRemoveCount, ArrayMunger, ArrayPos,
  LSearch, Sort (also StableSort, InsertionSort, ShellSort, QuickSort),
  SetAdd, SetRemove, SetUnion, SetDifference, SetOverlaps, Map.
- **Dates:** Date and TotalMinutes, in minutes since 1 January 1904.

The maths, binary, character and string functions are C. The array, set,
sorting and date functions are NewtonScript, held in NewtNOS.c and compiled
once, the first time the VM starts.

## Testing

`probe.newt` calls the functions 227 ways and prints `label => value`.
`expected.out` holds the expected answers.

    newt64 tests/nos/probe.newt > out.txt
    python3 tests/nos/compare.py out.txt

## Behaviour worth knowing

- **Numbers:**
  - `Floor` and `Ceiling` return integers. A result outside 30 bits stays a
    real, as Newton integers are 30 bits.
  - `Round` and `Trunc` return reals.
  - `Max` and `Min` return one of their arguments unchanged.
  - `StringToNumber` always returns a real. It accepts `" 12 "`, `"-7"`,
    `".5"` and `"1,000"`, and rejects `"+3"`, `"1e3"` and `"12abc"`.
  - `NumberStr` groups thousands and gives 15 significant digits.
- **Strings:**
  - `StrCompare` and the `'|str<|` test ignore case and accents.
  - `Upcase`, `Downcase`, `Capitalize`, `CapitalizeWords`, `TrimString`,
    `StrMunger` and `StripDiacriticals` change their argument in place;
    `SubstituteChars` returns a copy.
- **Arrays and sets:**
  - `ArrayRemoveCount` returns nil.
  - `SetDifference` returns a copy without the first occurrence of each item.
  - `SetOverlaps` returns the index in its first argument.
  - `Map` calls its function for each slot and returns nil.
  - `LSearch` matches on equality only.
  - Without a test, `ArrayPos` compares with `=`, so two equal strings are
    different items.
  - `Sort` is stable and sorts in place. Its tests are `'|<|`, `'|>|`,
    `'|str<|`, `'|str>|` or a function; its key is nil, a slot, a path or a
    function.

## Limitations

- **Strings are UTF-8 and indexed by byte.** This holds here as in the rest
  of NEWT/0 (StrLen, SubStr, StrPos), so with non-ASCII text offsets and
  lengths count bytes. Case conversion and accent stripping cover ASCII and
  Latin-1. The character tests (IsAlpha and the others) are ASCII only.
- **Object errors**, such as ExtractLong of a value outside 30 bits, are
  thrown as `evt.ex.fr;type.ref.frame` with a data frame, not as
  `evt.ex.fr` with an `error` slot.
- **Date and TotalMinutes** use the Gregorian calendar and take no account
  of time zones.
- **Not included:** functions that need a host, such as deferred and delayed
  calls, user configuration, stores and soups, views and communications.
  TimeInSeconds is not included either.
