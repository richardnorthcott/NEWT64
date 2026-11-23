// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file	NewtNOS.c
 * @brief	Newton OS 2.1 global functions that NEWT/0 lacks
 *
 * Numbers and maths, binary objects, Mac Roman characters, strings, arrays,
 * sets, searching and sorting, StrTokenize, Date and TotalMinutes, with the
 * names, arguments and results of the Newton OS 2.1 API. tests/nos/probe.newt
 * (NEWT64) calls each of them, and compare.py checks a build's answers.
 *
 * Strings: NEWT/0 keeps strings as UTF-8 and indexes them by byte (StrLen,
 * SubStr, StrPos). These functions do the same, so offsets agree with the
 * built-ins; for ASCII text a byte offset is the character index. Case and
 * accent handling covers ASCII and Latin-1 only.
 *
 * The maths, binary, character and string functions are C. The array, set,
 * sorting and date functions are NewtonScript (kNOSLibrary below), compiled
 * when the VM starts. Functions that need a host (deferred calls, user
 * configuration, communications) are left to the program that embeds NEWT/0.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "NewtCore.h"
#include "NewtVM.h"
#include "NewtStr.h"
#include "NewtFns.h"
#include "NewtParser.h"
#include "NewtBC.h"
#include "NewtNOS.h"

#define NIL kNewtRefNIL
#define TRUEREF kNewtRefTRUE
#define BOOLREF(b) ((b) ? kNewtRefTRUE : kNewtRefNIL)

// Newton integers are 30 bits; results outside that range stay reals, as Floor(1e10) does.
static const int64_t kMinInt30 = -536870912, kMaxInt30 = 536870911;

// MARK: - Numbers

static int NOSNumber(newtRefArg r, double * out)
{
    if (NewtRefIsInteger(r)) { *out = (double)NewtRefToInteger(r); return 1; }
    if (NewtRefIsReal(r)) { *out = NewtRefToReal(r); return 1; }
    return 0;
}

#define NUMBER_ARG(r, x) double x; if (!NOSNumber(r, &x)) return NewtThrow(kNErrNotANumber, r)

static newtRef NOSIntegralResult(double f)
{
    if (f >= kMinInt30 && f <= kMaxInt30) return NewtMakeInteger((int64_t)f);
    return NewtMakeReal(f);
}

static newtRef NsFloor(newtRefArg rcvr, newtRefArg r)
{
    if (NewtRefIsInteger(r)) return r;
    NUMBER_ARG(r, x);
    return NOSIntegralResult(floor(x));
}

static newtRef NsCeiling(newtRefArg rcvr, newtRefArg r)
{
    if (NewtRefIsInteger(r)) return r;
    NUMBER_ARG(r, x);
    return NOSIntegralResult(ceil(x));
}

// Round and Trunc return reals, even for an integer argument.
static newtRef NsRound(newtRefArg rcvr, newtRefArg r) { NUMBER_ARG(r, x); return NewtMakeReal(round(x)); }
static newtRef NsTrunc(newtRefArg rcvr, newtRefArg r) { NUMBER_ARG(r, x); return NewtMakeReal(trunc(x)); }
static newtRef NsReal(newtRefArg rcvr, newtRefArg r) { NUMBER_ARG(r, x); return NewtMakeReal(x); }

static newtRef NsAbs(newtRefArg rcvr, newtRefArg r)
{
    if (NewtRefIsInteger(r)) { int64_t i = NewtRefToInteger(r); return NewtMakeInteger(i < 0 ? -i : i); }
    NUMBER_ARG(r, x);
    return NewtMakeReal(fabs(x));
}

static newtRef NsSignum(newtRefArg rcvr, newtRefArg r)
{
    if (NewtRefIsInteger(r)) { int64_t i = NewtRefToInteger(r); return NewtMakeInteger((i > 0) - (i < 0)); }
    NUMBER_ARG(r, x);
    return NewtMakeReal((double)((x > 0) - (x < 0)));
}

// Max and Min return one of their arguments unchanged: Min(2, 2.5) is the integer 2.
static newtRef NOSPick(newtRefArg a, newtRefArg b, int wantMax)
{
    double x, y;
    if (NOSNumber(a, &x) && NOSNumber(b, &y))
        return (wantMax ? y > x : y < x) ? b : a;
    newtRefVar greater = NcGreaterThan(b, a);
    newtRefVar less = NcLessThan(b, a);
    return NewtRefIsNotNIL(wantMax ? greater : less) ? b : a;
}

static newtRef NsMax(newtRefArg rcvr, newtRefArg a, newtRefArg b) { return NOSPick(a, b, 1); }
static newtRef NsMin(newtRefArg rcvr, newtRefArg a, newtRefArg b) { return NOSPick(a, b, 0); }

#define MATH1(name, expr) \
    static newtRef Ns##name(newtRefArg rcvr, newtRefArg r) { NUMBER_ARG(r, x); return NewtMakeReal(expr); }
#define MATH2(name, expr) \
    static newtRef Ns##name(newtRefArg rcvr, newtRefArg a, newtRefArg b) { NUMBER_ARG(a, x); NUMBER_ARG(b, y); return NewtMakeReal(expr); }
#define TEST1(name, expr) \
    static newtRef Ns##name(newtRefArg rcvr, newtRefArg r) { NUMBER_ARG(r, x); return BOOLREF(expr); }

MATH1(Sqrt, sqrt(x))
MATH1(Sin, sin(x))
MATH1(Cos, cos(x))
MATH1(Tan, tan(x))
MATH1(Asin, asin(x))
MATH1(Acos, acos(x))
MATH1(Atan, atan(x))
MATH1(Sinh, sinh(x))
MATH1(Cosh, cosh(x))
MATH1(Tanh, tanh(x))
MATH1(Asinh, asinh(x))
MATH1(Acosh, acosh(x))
MATH1(Atanh, atanh(x))
MATH1(Exp, exp(x))
MATH1(Expm1, expm1(x))
MATH1(Log, log(x))
MATH1(Log1p, log1p(x))
MATH1(Log10, log10(x))
MATH1(Logb, logb(x))
MATH1(Fabs, fabs(x))
MATH1(Rint, rint(x))
MATH1(NearbyInt, nearbyint(x))
MATH1(Erf, erf(x))
MATH1(Erfc, erfc(x))
MATH1(LGamma, lgamma(x))
MATH2(Atan2, atan2(x, y))
MATH2(Pow, pow(x, y))
MATH2(Fmod, fmod(x, y))
MATH2(Hypot, hypot(x, y))
MATH2(CopySign, copysign(x, y))
MATH2(FMax, fmax(x, y))
MATH2(FMin, fmin(x, y))
MATH2(FDim, fdim(x, y))
MATH2(Remainder, remainder(x, y))
MATH2(NextAfterD, nextafter(x, y))
TEST1(IsNaN, isnan(x))
TEST1(IsFinite, isfinite(x))
TEST1(IsNormal, isnormal(x))

static newtRef NsRandom(newtRefArg rcvr, newtRefArg low, newtRefArg high)
{
    if (!NewtRefIsInteger(low)) return NewtThrow(kNErrNotAnInteger, low);
    if (!NewtRefIsInteger(high)) return NewtThrow(kNErrNotAnInteger, high);
    int64_t lo = NewtRefToInteger(low), hi = NewtRefToInteger(high);
    if (hi <= lo) return low;
    return NewtMakeInteger(lo + (int64_t)arc4random_uniform((uint32_t)(hi - lo + 1)));
}

static newtRef NsBXor(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
    if (!NewtRefIsInteger(a)) return NewtThrow(kNErrNotAnInteger, a);
    if (!NewtRefIsInteger(b)) return NewtThrow(kNErrNotAnInteger, b);
    return NewtMakeInteger(NewtRefToInteger(a) ^ NewtRefToInteger(b));
}

// Sixtieths of a second since the interpreter started, as Newton's Ticks().
static newtRef NsTicks(newtRefArg rcvr)
{
    static struct timespec start;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (!start.tv_sec && !start.tv_nsec) start = now;
    int64_t t = (int64_t)(now.tv_sec - start.tv_sec) * 60 + (now.tv_nsec - start.tv_nsec) / 16666667;
    return NewtMakeInteger(t & kMaxInt30);
}

// MARK: - Mac Roman (ExtractChar, StuffChar and the C and Pascal strings)

static const uint16_t kMacRoman[128] = {
    0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1, 0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
    0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3, 0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
    0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF, 0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
    0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211, 0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
    0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB, 0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
    0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA, 0x00FF, 0x0178, 0x2044, 0x00A4, 0x2039, 0x203A, 0xFB01, 0xFB02,
    0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
    0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC, 0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7,
};

static uint32_t NOSFromMacRoman(uint8_t b) { return b < 0x80 ? b : kMacRoman[b - 0x80]; }

static uint8_t NOSToMacRoman(uint32_t c)
{
    if (c < 0x80) return (uint8_t)c;
    for (int i = 0; i < 128; i++) if (kMacRoman[i] == c) return (uint8_t)(0x80 + i);
    if (c == 0x20AC) return 0xDB;   // the euro took the currency sign's place
    return '?';
}

// MARK: - UTF-8

static size_t NOSDecode(const uint8_t * s, size_t n, size_t i, uint32_t * c)
{
    uint8_t b = s[i];
    if (b < 0x80 || i + 1 >= n) { *c = b; return 1; }
    if ((b & 0xE0) == 0xC0) { *c = ((b & 0x1F) << 6) | (s[i + 1] & 0x3F); return 2; }
    if ((b & 0xF0) == 0xE0 && i + 2 < n) { *c = ((b & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); return 3; }
    if ((b & 0xF8) == 0xF0 && i + 3 < n) { *c = ((b & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F); return 4; }
    *c = b; return 1;
}

static size_t NOSEncode(uint32_t c, char * out)
{
    if (c < 0x80) { out[0] = (char)c; return 1; }
    if (c < 0x800) { out[0] = (char)(0xC0 | (c >> 6)); out[1] = (char)(0x80 | (c & 0x3F)); return 2; }
    if (c < 0x10000) { out[0] = (char)(0xE0 | (c >> 12)); out[1] = (char)(0x80 | ((c >> 6) & 0x3F)); out[2] = (char)(0x80 | (c & 0x3F)); return 3; }
    out[0] = (char)(0xF0 | (c >> 18)); out[1] = (char)(0x80 | ((c >> 12) & 0x3F)); out[2] = (char)(0x80 | ((c >> 6) & 0x3F)); out[3] = (char)(0x80 | (c & 0x3F));
    return 4;
}

// Latin-1 letters change case by 0x20, except × and ÷.
static uint32_t NOSUpper(uint32_t c)
{
    if (c >= 'a' && c <= 'z') return c - 0x20;
    if (c >= 0xE0 && c <= 0xFE && c != 0xF7) return c - 0x20;
    return c;
}

static uint32_t NOSLower(uint32_t c)
{
    if (c >= 'A' && c <= 'Z') return c + 0x20;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 0x20;
    return c;
}

// The base letter of an accented Latin-1 letter, or the character itself.
static uint32_t NOSBaseLetter(uint32_t c)
{
    static const char kBase[64] = "AAAAAA\0CEEEEIIII\0NOOOOO\0OUUUUY\0\0aaaaaa\0ceeeeiiii\0nooooo\0ouuuuy\0y";
    if (c < 0xC0 || c > 0xFF) return c;
    char b = kBase[c - 0xC0];
    return b ? (uint32_t)(uint8_t)b : c;
}

static int NOSIsLetter(uint32_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xFF && c != 0xD7 && c != 0xF7);
}

static int NOSIsSpace(uint32_t c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

static uint32_t NOSFold(uint32_t c) { return NOSLower(NOSBaseLetter(c)); }

// MARK: - Argument helpers

static int NOSIndex(newtRefArg r, int64_t * out)
{
    if (!NewtRefIsInteger(r)) return 0;
    *out = NewtRefToInteger(r);
    return 1;
}

static newtRef NOSWritableString(newtRefArg r)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    if (NewtRefIsReadonly(r)) return NewtThrow(kNErrObjectReadOnly, r);
    return kNewtRefUnbind;
}

#define WRITABLE_STRING(r) do { newtRefVar e_ = NOSWritableString(r); if (e_ != kNewtRefUnbind) return e_; } while (0)

// Binary objects other than strings; strings are a separate type in NEWT/0.
static uint8_t * NOSBinary(newtRefArg r, uint32_t * length, int writable, newtRefVar * error)
{
    if (!NewtRefIsBinary(r) || NewtRefIsString(r)) { *error = NewtThrow(kNErrNotABinaryObject, r); return NULL; }
    if (writable && NewtRefIsReadonly(r)) { *error = NewtThrow(kNErrObjectReadOnly, r); return NULL; }
    *length = NewtBinaryLength(r);
    return NewtRefToBinary(r);
}

#define BINARY_RANGE(r, off, count, writable) \
    uint32_t len_; newtRefVar err_ = NIL; int64_t at_; \
    uint8_t * p = NOSBinary(r, &len_, writable, &err_); \
    if (!p) return err_; \
    if (!NOSIndex(off, &at_)) return NewtThrow(kNErrNotAnInteger, off); \
    if (at_ < 0 || at_ + (int64_t)(count) > (int64_t)len_) return NewtErrOutOfBounds(r, (int32_t)at_); \
    p += at_

static uint32_t NOSCharArg(newtRefArg r, int * ok)
{
    *ok = 1;
    if (NewtRefIsCharacter(r)) return NewtRefToCharacter(r);
    if (NewtRefIsInteger(r)) return (uint32_t)NewtRefToInteger(r);
    *ok = 0;
    return 0;
}

// MARK: - Binary objects

static newtRef NsExtractChar(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 1, 0);
    return NewtMakeCharacter(NOSFromMacRoman(p[0]));
}

static newtRef NsExtractUniChar(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 2, 0);
    return NewtMakeCharacter((uint32_t)(p[0] << 8 | p[1]));
}

static newtRef NsExtractLong(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 4, 0);
    int32_t v = (int32_t)((uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]);
    if (v < kMinInt30 || v > kMaxInt30) return NewtThrow(kNErrLongOutOfRange, r);
    return NewtMakeInteger(v);
}

// ExtractXLong shifts the unsigned long right by three: ExtractXLong of 0x100 is 32.
static newtRef NsExtractXLong(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 4, 0);
    uint32_t v = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
    return NewtMakeInteger(v >> 3);
}

static newtRef NsExtractBytes(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg count, newtRefArg klass)
{
    int64_t n;
    if (!NOSIndex(count, &n) || n < 0) return NewtThrow(kNErrNotAnInteger, count);
    if (!NewtRefIsSymbol(klass)) return NewtThrow(kNErrNotASymbol, klass);
    BINARY_RANGE(r, off, n, 0);
    return NewtMakeBinary(klass, p, (uint32_t)n, false);
}

static newtRef NOSMacRomanString(const uint8_t * p, size_t n)
{
    char * buffer = malloc(n * 3 + 1);
    if (!buffer) return NewtThrow0(kNErrOutOfObjectMemory);
    size_t k = 0;
    for (size_t i = 0; i < n; i++) k += NOSEncode(NOSFromMacRoman(p[i]), buffer + k);
    newtRefVar s = NewtMakeString2(buffer, (uint32_t)k, false);
    free(buffer);
    return s;
}

static newtRef NsExtractCString(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 0, 0);
    size_t n = 0, room = len_ - (size_t)at_;
    while (n < room && p[n]) n++;
    return NOSMacRomanString(p, n);
}

static newtRef NsExtractPString(newtRefArg rcvr, newtRefArg r, newtRefArg off)
{
    BINARY_RANGE(r, off, 1, 0);
    size_t n = p[0];
    if ((size_t)at_ + 1 + n > len_) return NewtErrOutOfBounds(r, (int32_t)(at_ + 1 + n));
    return NOSMacRomanString(p + 1, n);
}

static newtRef NsStuffByte(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg v)
{
    if (!NewtRefIsInteger(v)) return NewtThrow(kNErrNotAnInteger, v);
    BINARY_RANGE(r, off, 1, 1);
    p[0] = (uint8_t)NewtRefToInteger(v);
    return NIL;
}

static newtRef NsStuffWord(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg v)
{
    if (!NewtRefIsInteger(v)) return NewtThrow(kNErrNotAnInteger, v);
    BINARY_RANGE(r, off, 2, 1);
    uint32_t w = (uint32_t)NewtRefToInteger(v);
    p[0] = (uint8_t)(w >> 8); p[1] = (uint8_t)w;
    return NIL;
}

static newtRef NsStuffLong(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg v)
{
    if (!NewtRefIsInteger(v)) return NewtThrow(kNErrNotAnInteger, v);
    BINARY_RANGE(r, off, 4, 1);
    uint32_t w = (uint32_t)NewtRefToInteger(v);
    p[0] = (uint8_t)(w >> 24); p[1] = (uint8_t)(w >> 16); p[2] = (uint8_t)(w >> 8); p[3] = (uint8_t)w;
    return NIL;
}

static newtRef NsStuffChar(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg v)
{
    int ok;
    uint32_t c = NOSCharArg(v, &ok);
    if (!ok) return NewtThrow(kNErrNotACharacter, v);
    BINARY_RANGE(r, off, 1, 1);
    p[0] = NewtRefIsInteger(v) ? (uint8_t)c : NOSToMacRoman(c);
    return NIL;
}

static newtRef NsStuffUniChar(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg v)
{
    int ok;
    uint32_t c = NOSCharArg(v, &ok);
    if (!ok) return NewtThrow(kNErrNotACharacter, v);
    BINARY_RANGE(r, off, 2, 1);
    p[0] = (uint8_t)(c >> 8); p[1] = (uint8_t)c;
    return NIL;
}

// The Mac Roman bytes of a NEWT string; the caller frees them.
static uint8_t * NOSMacRomanBytes(newtRefArg s, size_t * n)
{
    const uint8_t * u = (const uint8_t *)NewtRefToString(s);
    size_t len = NewtStringLength(s), k = 0;
    uint8_t * out = malloc(len + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; ) { uint32_t c; i += NOSDecode(u, len, i, &c); out[k++] = NOSToMacRoman(c); }
    *n = k;
    return out;
}

static newtRef NOSStuffStringChecked(newtRefArg r, newtRefArg off, newtRefArg s, int pascal);
static newtRef NsStuffCString(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg s) { return NOSStuffStringChecked(r, off, s, 0); }
static newtRef NsStuffPString(newtRefArg rcvr, newtRefArg r, newtRefArg off, newtRefArg s) { return NOSStuffStringChecked(r, off, s, 1); }

static newtRef NOSStuffStringChecked(newtRefArg r, newtRefArg off, newtRefArg s, int pascal)
{
    if (!NewtRefIsString(s)) return NewtThrow(kNErrNotAString, s);
    uint32_t length; newtRefVar error = NIL; int64_t at;
    if (!NOSBinary(r, &length, 1, &error)) return error;
    if (!NOSIndex(off, &at)) return NewtThrow(kNErrNotAnInteger, off);
    size_t n;
    uint8_t * bytes = NOSMacRomanBytes(s, &n);
    if (!bytes) return NewtThrow0(kNErrOutOfObjectMemory);
    if (pascal && n > 255) n = 255;
    if (at < 0 || at + (int64_t)n + 1 > (int64_t)length) { free(bytes); return NewtErrOutOfBounds(r, (int32_t)at); }
    uint8_t * p = NewtRefToBinary(r) + at;
    if (pascal) { p[0] = (uint8_t)n; memcpy(p + 1, bytes, n); }
    else { memcpy(p, bytes, n); p[n] = 0; }
    free(bytes);
    return NIL;
}

static const uint8_t * NOSBytes(newtRefArg r, size_t * n)
{
    if (NewtRefIsString(r)) { *n = NewtStringLength(r); return (const uint8_t *)NewtRefToString(r); }
    if (NewtRefIsBinary(r)) { *n = NewtBinaryLength(r); return NewtRefToBinary(r); }
    return NULL;
}

static newtRef NsBinEqual(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
    size_t na, nb;
    const uint8_t * pa = NOSBytes(a, &na), * pb = NOSBytes(b, &nb);
    if (!pa) return NewtThrow(kNErrNotABinaryObject, a);
    if (!pb) return NewtThrow(kNErrNotABinaryObject, b);
    return BOOLREF(na == nb && memcmp(pa, pb, na) == 0);
}

// Replaces dst[dstStart, dstCount) with src[srcStart, srcCount); nil counts run
// to the end, and a nil source deletes. Shared by BinaryMunger and StrMunger.
static newtRef NOSMunge(newtRefArg dst, newtRefArg dstStart, newtRefArg dstCount,
                        newtRefArg src, newtRefArg srcStart, newtRefArg srcCount, int isString)
{
    size_t length;
    const uint8_t * d = NOSBytes(dst, &length);
    int64_t ds, dc, ss = 0, sc = 0;
    if (!NOSIndex(dstStart, &ds)) return NewtThrow(kNErrNotAnInteger, dstStart);
    if (NewtRefIsNIL(dstCount)) dc = (int64_t)length - ds;
    else if (!NOSIndex(dstCount, &dc)) return NewtThrow(kNErrNotAnInteger, dstCount);
    if (ds < 0 || dc < 0 || ds > (int64_t)length) return NewtErrOutOfBounds(dst, (int32_t)ds);
    if (ds + dc > (int64_t)length) dc = (int64_t)length - ds;
    uint8_t * insert = NULL;
    if (NewtRefIsNotNIL(src)) {
        size_t sn;
        const uint8_t * s = NOSBytes(src, &sn);
        if (!s || (isString && !NewtRefIsString(src))) return NewtThrow(isString ? kNErrNotAString : kNErrNotABinaryObject, src);
        if (NewtRefIsNIL(srcStart)) ss = 0;
        else if (!NOSIndex(srcStart, &ss)) return NewtThrow(kNErrNotAnInteger, srcStart);
        if (NewtRefIsNIL(srcCount)) sc = (int64_t)sn - ss;
        else if (!NOSIndex(srcCount, &sc)) return NewtThrow(kNErrNotAnInteger, srcCount);
        if (ss < 0 || sc < 0 || ss + sc > (int64_t)sn) return NewtErrOutOfBounds(src, (int32_t)ss);
        // Copy first: the source may be the destination.
        insert = malloc((size_t)sc + 1);
        if (!insert) return NewtThrow0(kNErrOutOfObjectMemory);
        memcpy(insert, s + ss, (size_t)sc);
    }
    size_t tail = length - (size_t)(ds + dc), newLength = (size_t)(ds + sc) + tail;
    if (newLength > length) {
        if (isString) NewtStringSetLength(dst, (uint32_t)newLength); else NewtBinarySetLength(dst, (uint32_t)newLength);
    }
    uint8_t * p = isString ? (uint8_t *)NewtRefToString(dst) : NewtRefToBinary(dst);
    (void)d;
    memmove(p + ds + sc, p + ds + dc, tail);
    if (sc) memcpy(p + ds, insert, (size_t)sc);
    free(insert);
    if (newLength < length) {
        if (isString) NewtStringSetLength(dst, (uint32_t)newLength); else NewtBinarySetLength(dst, (uint32_t)newLength);
    }
    if (isString) ((char *)NewtRefToString(dst))[newLength] = 0;
    return dst;
}

static newtRef NsBinaryMunger(newtRefArg rcvr, newtRefArg dst, newtRefArg ds, newtRefArg dc,
                              newtRefArg src, newtRefArg ss, newtRefArg sc)
{
    uint32_t length; newtRefVar error = NIL;
    if (!NOSBinary(dst, &length, 1, &error)) return error;
    return NOSMunge(dst, ds, dc, src, ss, sc, 0);
}

// MARK: - Strings

static newtRef NsStrMunger(newtRefArg rcvr, newtRefArg dst, newtRefArg ds, newtRefArg dc,
                           newtRefArg src, newtRefArg ss, newtRefArg sc)
{
    WRITABLE_STRING(dst);
    return NOSMunge(dst, ds, dc, src, ss, sc, 1);
}

// Rewrites a string in place, one character at a time, where the change keeps
// the UTF-8 length (ASCII and Latin-1 case changes do).
static newtRef NOSMapInPlace(newtRefArg r, uint32_t (*map)(uint32_t, uint32_t))
{
    WRITABLE_STRING(r);
    uint8_t * s = (uint8_t *)NewtRefToString(r);
    size_t n = NewtStringLength(r);
    uint32_t previous = 0;
    for (size_t i = 0; i < n; ) {
        uint32_t c, m;
        size_t k = NOSDecode(s, n, i, &c);
        m = map(c, previous);
        char buffer[4];
        if (m != c && NOSEncode(m, buffer) == k) memcpy(s + i, buffer, k);
        previous = c ? c : 1;
        i += k;
    }
    return r;
}

static uint32_t NOSMapUpper(uint32_t c, uint32_t previous) { return NOSUpper(c); }
static uint32_t NOSMapLower(uint32_t c, uint32_t previous) { return NOSLower(c); }
static uint32_t NOSMapFirst(uint32_t c, uint32_t previous) { return previous ? c : NOSUpper(c); }
// "o'neil-smith x.y" becomes "O'Neil-Smith X.Y": a letter after anything but a letter or digit.
static uint32_t NOSMapWords(uint32_t c, uint32_t previous)
{
    int wordChar = previous && (NOSIsLetter(previous) || (previous >= '0' && previous <= '9'));
    return wordChar ? c : NOSUpper(c);
}

static newtRef NsUpcase(newtRefArg rcvr, newtRefArg r) { return NOSMapInPlace(r, NOSMapUpper); }
static newtRef NsDowncase(newtRefArg rcvr, newtRefArg r) { return NOSMapInPlace(r, NOSMapLower); }
static newtRef NsCapitalize(newtRefArg rcvr, newtRefArg r) { return NOSMapInPlace(r, NOSMapFirst); }
static newtRef NsCapitalizeWords(newtRefArg rcvr, newtRefArg r) { return NOSMapInPlace(r, NOSMapWords); }

static newtRef NOSSetStringBytes(newtRefArg r, const char * bytes, size_t n)
{
    char * copy = malloc(n + 1);
    if (!copy) return NewtThrow0(kNErrOutOfObjectMemory);
    memcpy(copy, bytes, n);
    NewtStringSetLength(r, (uint32_t)n);
    char * s = NewtRefToString(r);
    memcpy(s, copy, n);
    s[n] = 0;
    free(copy);
    return r;
}

static newtRef NsTrimString(newtRefArg rcvr, newtRefArg r)
{
    WRITABLE_STRING(r);
    const char * s = NewtRefToString(r);
    size_t n = NewtStringLength(r), a = 0, b = n;
    while (a < b && NOSIsSpace((uint8_t)s[a])) a++;
    while (b > a && NOSIsSpace((uint8_t)s[b - 1])) b--;
    return NOSSetStringBytes(r, s + a, b - a);
}

static newtRef NsStripDiacriticals(newtRefArg rcvr, newtRefArg r)
{
    WRITABLE_STRING(r);
    const uint8_t * s = (const uint8_t *)NewtRefToString(r);
    size_t n = NewtStringLength(r), k = 0;
    char * out = malloc(n + 1);
    if (!out) return NewtThrow0(kNErrOutOfObjectMemory);
    for (size_t i = 0; i < n; ) { uint32_t c; i += NOSDecode(s, n, i, &c); k += NOSEncode(NOSBaseLetter(c), out + k); }
    newtRefVar result = NOSSetStringBytes(r, out, k);
    free(out);
    return result;
}

static newtRef NsStrReverse(newtRefArg rcvr, newtRefArg r)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    const uint8_t * s = (const uint8_t *)NewtRefToString(r);
    size_t n = NewtStringLength(r);
    char * out = malloc(n + 1);
    if (!out) return NewtThrow0(kNErrOutOfObjectMemory);
    size_t end = n;
    for (size_t i = 0; i < n; ) { uint32_t c; size_t k = NOSDecode(s, n, i, &c); end -= k; memcpy(out + end, s + i, k); i += k; }
    newtRefVar result = NewtMakeString2(out, (uint32_t)n, false);
    free(out);
    return result;
}

// The sort order ignores case and accents: StrCompare("abc", "ABC") is 0.
static newtRef NsStrCompare(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
    if (!NewtRefIsString(a)) return NewtThrow(kNErrNotAString, a);
    if (!NewtRefIsString(b)) return NewtThrow(kNErrNotAString, b);
    const uint8_t * s = (const uint8_t *)NewtRefToString(a), * t = (const uint8_t *)NewtRefToString(b);
    size_t n = NewtStringLength(a), m = NewtStringLength(b), i = 0, j = 0;
    while (i < n && j < m) {
        uint32_t c, d;
        i += NOSDecode(s, n, i, &c);
        j += NOSDecode(t, m, j, &d);
        c = NOSFold(c); d = NOSFold(d);
        if (c != d) return NewtMakeInteger(c < d ? -1 : 1);
    }
    return NewtMakeInteger(i < n ? 1 : j < m ? -1 : 0);
}

static newtRef NOSFind(newtRefArg r, const char * needle, size_t needleLength, newtRefArg start)
{
    int64_t from = 0;
    if (NewtRefIsNotNIL(start) && !NOSIndex(start, &from)) return NewtThrow(kNErrNotAnInteger, start);
    const char * s = NewtRefToString(r);
    size_t n = NewtStringLength(r);
    if (from < 0) from = 0;
    if (needleLength == 0 || (size_t)from + needleLength > n) return NIL;
    for (size_t i = (size_t)from; i + needleLength <= n; i++)
        if (memcmp(s + i, needle, needleLength) == 0) return NewtMakeInteger((int64_t)i);
    return NIL;
}

static newtRef NsStrPosExact(newtRefArg rcvr, newtRefArg r, newtRefArg sub, newtRefArg start)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    if (!NewtRefIsString(sub)) return NewtThrow(kNErrNotAString, sub);
    return NOSFind(r, NewtRefToString(sub), NewtStringLength(sub), start);
}

static newtRef NsCharPos(newtRefArg rcvr, newtRefArg r, newtRefArg ch, newtRefArg start)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    if (!NewtRefIsCharacter(ch)) return NewtThrow(kNErrNotACharacter, ch);
    char buffer[4];
    size_t k = NOSEncode(NewtRefToCharacter(ch), buffer);
    return NOSFind(r, buffer, k, start);
}

// SubstituteChars("abcabc", "ab", "xy") is "xycxyc"; the original is left alone.
static newtRef NsSubstituteChars(newtRefArg rcvr, newtRefArg r, newtRefArg from, newtRefArg to)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    if (!NewtRefIsString(from)) return NewtThrow(kNErrNotAString, from);
    if (!NewtRefIsString(to)) return NewtThrow(kNErrNotAString, to);
    uint32_t source[256], target[256];
    size_t ns = 0, nt = 0;
    const uint8_t * f = (const uint8_t *)NewtRefToString(from), * t = (const uint8_t *)NewtRefToString(to);
    size_t fn = NewtStringLength(from), tn = NewtStringLength(to);
    for (size_t i = 0; i < fn && ns < 256; ) i += NOSDecode(f, fn, i, &source[ns++]);
    for (size_t i = 0; i < tn && nt < 256; ) i += NOSDecode(t, tn, i, &target[nt++]);
    const uint8_t * s = (const uint8_t *)NewtRefToString(r);
    size_t n = NewtStringLength(r), k = 0;
    char * out = malloc(n * 2 + 4);
    if (!out) return NewtThrow0(kNErrOutOfObjectMemory);
    for (size_t i = 0; i < n; ) {
        uint32_t c;
        i += NOSDecode(s, n, i, &c);
        for (size_t j = 0; j < ns; j++) if (source[j] == c) { if (j < nt) c = target[j]; break; }
        k += NOSEncode(c, out + k);
    }
    newtRefVar result = NewtMakeString2(out, (uint32_t)k, false);
    free(out);
    return result;
}

static newtRef NsStrConcat(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
    if (!NewtRefIsString(a)) return NewtThrow(kNErrNotAString, a);
    if (!NewtRefIsString(b)) return NewtThrow(kNErrNotAString, b);
    size_t n = NewtStringLength(a), m = NewtStringLength(b);
    char * out = malloc(n + m + 1);
    if (!out) return NewtThrow0(kNErrOutOfObjectMemory);
    memcpy(out, NewtRefToString(a), n);
    memcpy(out + n, NewtRefToString(b), m);
    newtRefVar result = NewtMakeString2(out, (uint32_t)(n + m), false);
    free(out);
    return result;
}

// "12", " 12 ", "-7", "3.5", ".5" and "1,000" are numbers; "+3", "1e3" and
// "12abc" are not. The result is always a real.
static newtRef NsStringToNumber(newtRefArg rcvr, newtRefArg r)
{
    if (!NewtRefIsString(r)) return NewtThrow(kNErrNotAString, r);
    const char * s = NewtRefToString(r);
    size_t n = NewtStringLength(r), i = 0;
    char buffer[64];
    size_t k = 0;
    int digits = 0;
    while (i < n && NOSIsSpace((uint8_t)s[i])) i++;
    if (i < n && s[i] == '-') buffer[k++] = s[i++];
    while (i < n && k < sizeof buffer - 2 && ((s[i] >= '0' && s[i] <= '9') || (s[i] == ',' && digits))) {
        if (s[i] != ',') { buffer[k++] = s[i]; digits++; }
        i++;
    }
    if (i < n && s[i] == '.') {
        buffer[k++] = s[i++];
        while (i < n && k < sizeof buffer - 1 && s[i] >= '0' && s[i] <= '9') { buffer[k++] = s[i++]; digits++; }
    }
    while (i < n && NOSIsSpace((uint8_t)s[i])) i++;
    if (!digits || i != n) return NIL;
    buffer[k] = 0;
    return NewtMakeReal(strtod(buffer, NULL));
}

// NumberStr(1234567.0) is "1,234,567"; NumberStr(1/3) is "0.333333333333333".
static newtRef NsNumberStr(newtRefArg rcvr, newtRefArg r)
{
    char raw[64], out[96];
    if (NewtRefIsInteger(r)) snprintf(raw, sizeof raw, "%lld", (long long)NewtRefToInteger(r));
    else if (NewtRefIsReal(r)) snprintf(raw, sizeof raw, "%.15g", NewtRefToReal(r));
    else return NewtThrow(kNErrNotANumber, r);
    if (strchr(raw, 'e') || strchr(raw, 'n') || strchr(raw, 'i')) return NewtMakeString(raw, false);
    const char * digits = raw[0] == '-' ? raw + 1 : raw;
    size_t whole = strcspn(digits, "."), k = 0;
    if (raw[0] == '-') out[k++] = '-';
    for (size_t i = 0; i < whole; i++) {
        if (i && (whole - i) % 3 == 0) out[k++] = ',';
        out[k++] = digits[i];
    }
    strcpy(out + k, digits + whole);
    return NewtMakeString(out, false);
}

// MARK: - Characters (ASCII only: IsAlpha($é) is nil)

#define CHAR_TEST(name, expr) \
    static newtRef Ns##name(newtRefArg rcvr, newtRefArg r) { \
        if (!NewtRefIsCharacter(r)) return NewtThrow(kNErrNotACharacter, r); \
        uint32_t c = NewtRefToCharacter(r); return BOOLREF(expr); }

CHAR_TEST(IsAlpha, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
CHAR_TEST(IsDigit, c >= '0' && c <= '9')
CHAR_TEST(IsUpper, c >= 'A' && c <= 'Z')
CHAR_TEST(IsLower, c >= 'a' && c <= 'z')
CHAR_TEST(IsAlphaNumeric, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
CHAR_TEST(IsWhiteSpace, NOSIsSpace(c))

// MARK: - Symbols and functions

static newtRef NsSymbolName(newtRefArg rcvr, newtRefArg r)
{
    if (!NewtRefIsSymbol(r)) return NewtThrow(kNErrNotASymbol, r);
    return NewtMakeString(NewtSymbolGetName(r), false);
}

static newtRef NsGetFunctionArgCount(newtRefArg rcvr, newtRefArg fn)
{
    if (!NewtRefIsFunction(fn)) return NewtThrow(kNErrNotAFunction, fn);
    if (NewtRefIsCodeBlock(fn)) {
        newtRefVar n = NcGetSlot(fn, NewtMakeSymbol("numArgs"));
        if (NewtRefIsInteger(n)) return NewtMakeInteger(NewtRefToInteger(n) & 0xFFFF);
    }
    return NIL;
}

// MARK: - Registration

// MARK: - Functions written in NewtonScript

static const char * kNOSLibrary =
    "// A type error names the value it was given.\n"
    "DefGlobalFn('|NOS:TypeError|, func(code, value) Throw('|evt.ex.fr.type;type.ref.frame|, {errorCode: code, value: value}));\n"
    "DefGlobalFn('|NOS:RangeError|, func(value) Throw('|evt.ex.fr;type.ref.frame|, {errorCode: -48205, value: value}));\n"
    "\n"
    "// MARK: Arrays\n"
    "\n"
    "DefGlobalFn('ArrayInsert, func(array, element, position) begin\n"
    "    local n := Length(array);\n"
    "    if not IsInteger(position) or position < 0 or position > n then |NOS:RangeError|(position);\n"
    "    AddArraySlot(array, nil);\n"
    "    for i := n to position + 1 by -1 do array[i] := array[i - 1];\n"
    "    array[position] := element;\n"
    "    array;\n"
    "end);\n"
    "\n"
    "DefGlobalFn('ArrayRemoveCount, func(array, start, count) begin\n"
    "    local n := Length(array);\n"
    "    if start + count > n then count := n - start;\n"
    "    if count > 0 then begin\n"
    "        for i := start to n - count - 1 do array[i] := array[i + count];\n"
    "        SetLength(array, n - count);\n"
    "    end;\n"
    "    nil;\n"
    "end);\n"
    "\n"
    "DefGlobalFn('ArrayMunger, func(a, aStart, aCount, b, bStart, bCount) begin\n"
    "    local n := Length(a);\n"
    "    if aCount = nil or aStart + aCount > n then aCount := n - aStart;\n"
    "    local insert := [];\n"
    "    if b then begin\n"
    "        if bStart = nil then bStart := 0;\n"
    "        if bCount = nil then bCount := Length(b) - bStart;\n"
    "        for i := bStart to bStart + bCount - 1 do AddArraySlot(insert, b[i]);\n"
    "    end;\n"
    "    local tail := [];\n"
    "    for i := aStart + aCount to n - 1 do AddArraySlot(tail, a[i]);\n"
    "    SetLength(a, aStart);\n"
    "    foreach x in insert do AddArraySlot(a, x);\n"
    "    foreach x in tail do AddArraySlot(a, x);\n"
    "    a;\n"
    "end);\n"
    "\n"
    "// With no test, items match by =, so two equal strings are different items.\n"
    "DefGlobalFn('ArrayPos, func(array, item, start, test) begin\n"
    "    if start = nil then start := 0;\n"
    "    for i := start to Length(array) - 1 do\n"
    "        if (if test then call test with (item, array[i]) else item = array[i]) then return i;\n"
    "    nil;\n"
    "end);\n"
    "\n"
    "// MARK: Tests and keys (Sort, LSearch)\n"
    "\n"
    "DefGlobalFn('|NOS:Key|, func(element, key)\n"
    "    if key = nil then element else if IsFunction(key) then call key with (element) else element.(key));\n"
    "\n"
    "// A comparison as an integer: below zero, zero or above zero. '|<| and '|>|\n"
    "// take numbers; '|str<|, '|str>| and '|str=| ignore case and accents.\n"
    "DefGlobalFn('|NOS:Compare|, func(test, a, b) begin\n"
    "    if IsFunction(test) then return call test with (a, b);\n"
    "    if test = '|<| or test = '|>| then begin\n"
    "        if not IsNumber(a) then |NOS:TypeError|(-48404, a);\n"
    "        if not IsNumber(b) then |NOS:TypeError|(-48404, b);\n"
    "        local r := if a < b then -1 else if a > b then 1 else 0;\n"
    "        return if test = '|>| then -r else r;\n"
    "    end;\n"
    "    if test = '|str<| or test = '|str=| then return StrCompare(a, b);\n"
    "    if test = '|str>| then return -StrCompare(a, b);\n"
    "    if test = '|=| then return if a = b then 0 else 1;\n"
    "    |NOS:TypeError|(-48411, test);\n"
    "end);\n"
    "\n"
    "// The first element whose key compares equal to item; '|<| and '|>| find\n"
    "// only equal numbers.\n"
    "DefGlobalFn('LSearch, func(array, item, start, test, key) begin\n"
    "    if start = nil then start := 0;\n"
    "    for i := start to Length(array) - 1 do\n"
    "        if |NOS:Compare|(test, item, |NOS:Key|(array[i], key)) = 0 then return i;\n"
    "    nil;\n"
    "end);\n"
    "\n"
    "// A stable merge sort, in place; returns the array.\n"
    "DefGlobalFn('Sort, func(array, test, key) begin\n"
    "    local n := Length(array);\n"
    "    if n < 2 then return array;\n"
    "    local keys := Array(n, nil);\n"
    "    local src := Array(n, nil);\n"
    "    for i := 0 to n - 1 do begin keys[i] := |NOS:Key|(array[i], key); src[i] := i end;\n"
    "    local dst := Array(n, nil);\n"
    "    local width := 1;\n"
    "    while width < n do begin\n"
    "        for low := 0 to n - 1 by 2 * width do begin\n"
    "            local mid := Min(low + width, n);\n"
    "            local high := Min(low + 2 * width, n);\n"
    "            local i := low;\n"
    "            local j := mid;\n"
    "            for k := low to high - 1 do begin\n"
    "                local left := nil;\n"
    "                if i < mid then\n"
    "                    if j >= high then left := true\n"
    "                    else if |NOS:Compare|(test, keys[src[j]], keys[src[i]]) >= 0 then left := true;\n"
    "                if left then begin dst[k] := src[i]; i := i + 1 end\n"
    "                else begin dst[k] := src[j]; j := j + 1 end;\n"
    "            end;\n"
    "        end;\n"
    "        local swap := src;\n"
    "        src := dst;\n"
    "        dst := swap;\n"
    "        width := 2 * width;\n"
    "    end;\n"
    "    local values := Clone(array);\n"
    "    for i := 0 to n - 1 do array[i] := values[src[i]];\n"
    "    array;\n"
    "end);\n"
    "DefGlobalFn('StableSort, func(array, test, key) Sort(array, test, key));\n"
    "DefGlobalFn('InsertionSort, func(array, test, key) Sort(array, test, key));\n"
    "DefGlobalFn('ShellSort, func(array, test, key) Sort(array, test, key));\n"
    "DefGlobalFn('QuickSort, func(array, test, key) Sort(array, test, key));\n"
    "\n"
    "// MARK: Sets\n"
    "\n"
    "DefGlobalFn('SetAdd, func(array, value, uniqueOnly) begin\n"
    "    if uniqueOnly and ArrayPos(array, value, 0, nil) then return nil;\n"
    "    AddArraySlot(array, value);\n"
    "    array;\n"
    "end);\n"
    "\n"
    "// Removes the first occurrence.\n"
    "DefGlobalFn('SetRemove, func(array, value) begin\n"
    "    local i := ArrayPos(array, value, 0, nil);\n"
    "    if i then ArrayRemoveCount(array, i, 1);\n"
    "    array;\n"
    "end);\n"
    "\n"
    "DefGlobalFn('SetUnion, func(a, b, uniqueOnly) begin\n"
    "    local result := [];\n"
    "    foreach x in a do if not uniqueOnly or not ArrayPos(result, x, 0, nil) then AddArraySlot(result, x);\n"
    "    foreach x in b do if not uniqueOnly or not ArrayPos(result, x, 0, nil) then AddArraySlot(result, x);\n"
    "    result;\n"
    "end);\n"
    "\n"
    "// A copy of a without the first occurrence of each element of b:\n"
    "// SetDifference([1, 2, 3, 2], [2]) is [1, 3, 2].\n"
    "DefGlobalFn('SetDifference, func(a, b) begin\n"
    "    local result := Clone(a);\n"
    "    foreach x in b do SetRemove(result, x);\n"
    "    result;\n"
    "end);\n"
    "\n"
    "// The index in a of its first element that is also in b.\n"
    "DefGlobalFn('SetOverlaps, func(a, b) begin\n"
    "    foreach i, x in a do if ArrayPos(b, x, 0, nil) then return i;\n"
    "    nil;\n"
    "end);\n"
    "\n"
    "// Calls fn(slot, value) for each slot; returns nil and leaves obj alone.\n"
    "DefGlobalFn('Map, func(obj, fn) begin\n"
    "    foreach k, v in obj do call fn with (k, v);\n"
    "    nil;\n"
    "end);\n"
    "\n"
    "// MARK: Strings\n"
    "\n"
    "// A function that returns the next token on each call, then nil. Empty\n"
    "// tokens are skipped; delimiters is a character or a string of them.\n"
    "DefGlobalFn('StrTokenize, func(str, delimiters) begin\n"
    "    if not IsString(str) then |NOS:TypeError|(-48402, str);\n"
    "    local delims := [];\n"
    "    if IsCharacter(delimiters) then AddArraySlot(delims, delimiters)\n"
    "    else if IsString(delimiters) then for i := 0 to StrLen(delimiters) - 1 do AddArraySlot(delims, delimiters[i])\n"
    "    else |NOS:TypeError|(-48402, delimiters);\n"
    "    local pos := 0;\n"
    "    local n := StrLen(str);\n"
    "    func() begin\n"
    "        while pos < n and SetContains(delims, str[pos]) do pos := pos + 1;\n"
    "        if pos >= n then return nil;\n"
    "        local start := pos;\n"
    "        while pos < n and not SetContains(delims, str[pos]) do pos := pos + 1;\n"
    "        SubStr(str, start, pos - start);\n"
    "    end;\n"
    "end);\n"
    "\n"
    "// MARK: Dates (minutes since 1 January 1904, local time)\n"
    "\n"
    "DefGlobalFn('|NOS:DaysFromCivil|, func(y, m, d) begin\n"
    "    if m <= 2 then y := y - 1;\n"
    "    local era := y div 400;\n"
    "    local yoe := y - era * 400;\n"
    "    local doy := (153 * (if m > 2 then m - 3 else m + 9) + 2) div 5 + d - 1;\n"
    "    local doe := yoe * 365 + yoe div 4 - yoe div 100 + doy;\n"
    "    era * 146097 + doe - 695361;\n"
    "end);\n"
    "\n"
    "DefGlobalFn('|NOS:DaysInMonth|, func(y, m)\n"
    "    if m = 2 then (if (y mod 4 = 0 and y mod 100 <> 0) or y mod 400 = 0 then 29 else 28)\n"
    "    else if m = 4 or m = 6 or m = 9 or m = 11 then 30 else 31);\n"
    "\n"
    "DefGlobalFn('Date, func(time) begin\n"
    "    local days := time div 1440;\n"
    "    local rest := time - days * 1440;\n"
    "    local z := days + 695361;\n"
    "    local era := z div 146097;\n"
    "    local doe := z - era * 146097;\n"
    "    local yoe := (doe - doe div 1460 + doe div 36524 - doe div 146096) div 365;\n"
    "    local doy := doe - (365 * yoe + yoe div 4 - yoe div 100);\n"
    "    local mp := (5 * doy + 2) div 153;\n"
    "    local d := doy - (153 * mp + 2) div 5 + 1;\n"
    "    local m := if mp < 10 then mp + 3 else mp - 9;\n"
    "    local y := yoe + era * 400 + (if m <= 2 then 1 else 0);\n"
    "    {year: y, month: m, date: d, dayOfWeek: (days + 5) mod 7,\n"
    "     hour: rest div 60, minute: rest mod 60, second: 0,\n"
    "     daysInMonth: |NOS:DaysInMonth|(y, m)};\n"
    "end);\n"
    "\n"
    "DefGlobalFn('TotalMinutes, func(date) begin\n"
    "    local d := |NOS:DaysFromCivil|(date.year, if date.month then date.month else 1, if date.date then date.date else 1);\n"
    "    d * 1440 + (if date.hour then date.hour * 60 else 0) + (if date.minute then date.minute else 0);\n"
    "end);\n";

// MARK: - Registration

#define DEF(name, fn, n) NewtDefGlobalFunc(NewtMakeSymbol(name), (void *)fn, n, name)

void NewtNOSInstall(void)
{
    DEF("Floor", NsFloor, 1);           DEF("Ceiling", NsCeiling, 1);
    DEF("Round", NsRound, 1);           DEF("Trunc", NsTrunc, 1);
    DEF("Real", NsReal, 1);             DEF("Abs", NsAbs, 1);
    DEF("Signum", NsSignum, 1);         DEF("Max", NsMax, 2);
    DEF("Min", NsMin, 2);               DEF("Sqrt", NsSqrt, 1);
    DEF("Sin", NsSin, 1);               DEF("Cos", NsCos, 1);
    DEF("Tan", NsTan, 1);               DEF("Asin", NsAsin, 1);
    DEF("Acos", NsAcos, 1);             DEF("Atan", NsAtan, 1);
    DEF("Sinh", NsSinh, 1);             DEF("Cosh", NsCosh, 1);
    DEF("Tanh", NsTanh, 1);             DEF("Asinh", NsAsinh, 1);
    DEF("Acosh", NsAcosh, 1);           DEF("Atanh", NsAtanh, 1);
    DEF("Exp", NsExp, 1);               DEF("Expm1", NsExpm1, 1);
    DEF("Log", NsLog, 1);               DEF("Log1p", NsLog1p, 1);
    DEF("Log10", NsLog10, 1);           DEF("Logb", NsLogb, 1);
    DEF("Fabs", NsFabs, 1);             DEF("Rint", NsRint, 1);
    DEF("NearbyInt", NsNearbyInt, 1);   DEF("Erf", NsErf, 1);
    DEF("Erfc", NsErfc, 1);             DEF("LGamma", NsLGamma, 1);
    DEF("Atan2", NsAtan2, 2);           DEF("Pow", NsPow, 2);
    DEF("Fmod", NsFmod, 2);             DEF("Hypot", NsHypot, 2);
    DEF("CopySign", NsCopySign, 2);     DEF("FMax", NsFMax, 2);
    DEF("FMin", NsFMin, 2);             DEF("FDim", NsFDim, 2);
    DEF("Remainder", NsRemainder, 2);   DEF("NextAfterD", NsNextAfterD, 2);
    DEF("IsNaN", NsIsNaN, 1);           DEF("IsFinite", NsIsFinite, 1);
    DEF("IsNormal", NsIsNormal, 1);     DEF("Random", NsRandom, 2);
    DEF("BXor", NsBXor, 2);             DEF("Ticks", NsTicks, 0);

    DEF("ExtractChar", NsExtractChar, 2);       DEF("ExtractUniChar", NsExtractUniChar, 2);
    DEF("ExtractLong", NsExtractLong, 2);       DEF("ExtractXLong", NsExtractXLong, 2);
    DEF("ExtractBytes", NsExtractBytes, 4);     DEF("ExtractCString", NsExtractCString, 2);
    DEF("ExtractPString", NsExtractPString, 2); DEF("StuffByte", NsStuffByte, 3);
    DEF("StuffWord", NsStuffWord, 3);           DEF("StuffLong", NsStuffLong, 3);
    DEF("StuffChar", NsStuffChar, 3);           DEF("StuffUniChar", NsStuffUniChar, 3);
    DEF("StuffCString", NsStuffCString, 3);     DEF("StuffPString", NsStuffPString, 3);
    DEF("BinEqual", NsBinEqual, 2);             DEF("BinaryMunger", NsBinaryMunger, 6);

    DEF("StrMunger", NsStrMunger, 6);           DEF("Upcase", NsUpcase, 1);
    DEF("Downcase", NsDowncase, 1);             DEF("Capitalize", NsCapitalize, 1);
    DEF("CapitalizeWords", NsCapitalizeWords, 1); DEF("TrimString", NsTrimString, 1);
    DEF("StripDiacriticals", NsStripDiacriticals, 1); DEF("StrReverse", NsStrReverse, 1);
    DEF("StrCompare", NsStrCompare, 2);         DEF("StrPosExact", NsStrPosExact, 3);
    DEF("CharPos", NsCharPos, 3);               DEF("SubstituteChars", NsSubstituteChars, 3);
    DEF("StrConcat", NsStrConcat, 2);           DEF("StringToNumber", NsStringToNumber, 1);
    DEF("NumberStr", NsNumberStr, 1);

    DEF("IsAlpha", NsIsAlpha, 1);               DEF("IsDigit", NsIsDigit, 1);
    DEF("IsUpper", NsIsUpper, 1);               DEF("IsLower", NsIsLower, 1);
    DEF("IsAlphaNumeric", NsIsAlphaNumeric, 1); DEF("IsWhiteSpace", NsIsWhiteSpace, 1);

    DEF("SymbolName", NsSymbolName, 1);         DEF("MakeSymbol", NsMakeSymbol, 1);
    DEF("GetFunctionArgCount", NsGetFunctionArgCount, 1);

}

newtRef NewtNOSLibraryFunction(void)
{
    nps_syntax_node_t *	stree = NULL;
    uint32_t	numStree = 0;
    newtRefVar	fn = kNewtRefNIL;

    if (NPSParseStr(kNOSLibrary, &stree, &numStree) == kNErrNone && stree != NULL)
    {
        NBCInit();
        fn = NBCGenBC(stree, numStree, true);
        NBCCleanup();
        NPSCleanup();
    }

    return fn;
}
