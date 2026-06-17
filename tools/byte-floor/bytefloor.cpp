#include "bytefloor.h"
#include <algorithm>
#include <cstdio>

namespace hcp::bytefloor {

const char* name(Size s){ switch(s){case Size::One:return "1-byte";case Size::Two:return "2-byte";case Size::Four:return "4-byte";default:return "unknown";} }
const char* name(Endian e){ switch(e){case Endian::Little:return "little";case Endian::Big:return "big";default:return "none";} }
const char* name(Table t){ switch(t){case Table::Ascii:return "ASCII";case Table::Utf8:return "UTF-8";case Table::Latin1:return "Latin-1";case Table::Utf16:return "UTF-16";case Table::Utf32:return "UTF-32";default:return "unknown";} }

// ---- Phase A: size + endianness from the null histogram --------------------------------
// Uniform reduction over the sample (count nulls + position parity / mod-4) = AZSL-port
// candidate. 0x00 is the key char: excluded from text content, so its presence and
// position are pure structural signal about the container.
namespace {
struct SizeEndian { Size size; Endian endian; double conf; std::string ev; };

SizeEndian phaseA(const uint8_t* d, size_t n) {
    if (n == 0) return { Size::One, Endian::None, 0.0, "empty" };
    size_t nulls = 0, nullEven = 0, nullOdd = 0, mod4[4] = {0,0,0,0};
    for (size_t i = 0; i < n; ++i) {
        if (d[i] == 0x00) { ++nulls; (i & 1) ? ++nullOdd : ++nullEven; }
        else ++mod4[i & 3];
    }
    const double nf = double(nulls) / double(n);
    char buf[256];
    if (nf < 0.10) {
        snprintf(buf, sizeof buf, "nulls %.1f%% (<10%%) => 1-byte units", nf*100);
        return { Size::One, Endian::None, 1.0 - nf, buf };
    }
    if (nf < 0.65) {
        // ~half null for ascii-range text => 2-byte; null = high byte, so its offset
        // parity gives endianness (even offset => BE, odd => LE).
        const Endian e = (nullEven >= nullOdd) ? Endian::Big : Endian::Little;
        const double skew = double(std::max(nullEven, nullOdd)) / double(nulls ? nulls : 1);
        snprintf(buf, sizeof buf, "nulls %.1f%%, even=%zu odd=%zu => 2-byte %s", nf*100, nullEven, nullOdd, name(e));
        return { Size::Two, e, skew, buf };
    }
    // mostly null => 4-byte; nonnull concentrates at one offset mod 4 (0 => LE, 3 => BE).
    int best = 0; for (int k = 1; k < 4; ++k) if (mod4[k] > mod4[best]) best = k;
    const Endian e = (best == 3) ? Endian::Big : Endian::Little;
    snprintf(buf, sizeof buf, "nulls %.1f%%, nonnull@mod4=[%zu,%zu,%zu,%zu] => 4-byte %s",
             nf*100, mod4[0], mod4[1], mod4[2], mod4[3], name(e));
    return { Size::Four, e, nf, buf };
}

// ---- Phase B: table discrimination by mutual exclusion (1-byte case) -------------------
// UTF-8 validity scan = uniform pass, AZSL-port candidate. Presence of valid multibyte
// sequences confirms UTF-8; high bytes that do NOT validate as UTF-8 preclude it and
// argue 8-bit (Latin-1). Pure-ASCII is decode-identical across the low range, held as a
// superposition rather than forced.
struct TablePick { Table table; std::vector<Table> cands; double conf; std::string ev; };

void utf8scan(const uint8_t* d, size_t n, size_t& validMB, size_t& invalidHigh, size_t& anyHigh) {
    validMB = invalidHigh = anyHigh = 0;
    size_t i = 0;
    while (i < n) {
        const uint8_t b = d[i];
        if (b < 0x80) { ++i; continue; }
        ++anyHigh;
        int need;
        if      ((b & 0xE0) == 0xC0) need = 1;
        else if ((b & 0xF0) == 0xE0) need = 2;
        else if ((b & 0xF8) == 0xF0) need = 3;
        else { ++invalidHigh; ++i; continue; }            // stray continuation / 0xF8+
        if (i + size_t(need) >= n) { ++invalidHigh; ++i; continue; }  // truncated
        bool ok = true;
        for (int k = 1; k <= need; ++k) if ((d[i+k] & 0xC0) != 0x80) { ok = false; break; }
        if (ok) { ++validMB; i += need + 1; } else { ++invalidHigh; ++i; }
    }
}

TablePick phaseB_1byte(const uint8_t* d, size_t n) {
    size_t validMB, invalidHigh, anyHigh;
    utf8scan(d, n, validMB, invalidHigh, anyHigh);
    char buf[256];
    if (anyHigh == 0) {
        snprintf(buf, sizeof buf, "all bytes <0x80 => ASCII (decode-identical to UTF-8/Latin-1)");
        return { Table::Ascii, {Table::Ascii, Table::Utf8, Table::Latin1}, 1.0, buf };
    }
    if (validMB > 0 && invalidHigh == 0) {
        snprintf(buf, sizeof buf, "%zu valid UTF-8 multibyte, 0 invalid => UTF-8", validMB);
        return { Table::Utf8, {}, 1.0, buf };
    }
    if (validMB > 0 && invalidHigh > 0) {
        const bool utf8 = validMB >= invalidHigh;   // min-violation settle
        snprintf(buf, sizeof buf, "%zu valid UTF-8 vs %zu invalid high => %s (min-violation)",
                 validMB, invalidHigh, utf8 ? "UTF-8" : "Latin-1");
        return { utf8 ? Table::Utf8 : Table::Latin1, {}, double(std::max(validMB,invalidHigh))/double(validMB+invalidHigh), buf };
    }
    snprintf(buf, sizeof buf, "%zu high bytes, 0 valid UTF-8 => Latin-1 (8-bit)", anyHigh);
    return { Table::Latin1, {}, 1.0, buf };
}

// ---- Phase C: decode to codepoints; undecodable => residue byte (resync & continue) ----
void decodeUtf8(const uint8_t* d, size_t n, Result& r) {
    size_t i = 0;
    while (i < n) {
        const uint8_t b = d[i];
        if (b < 0x80) { r.elems.push_back({Elem::Codepoint, b}); ++r.codepoints; ++i; continue; }
        int need; uint32_t cp;
        if      ((b & 0xE0) == 0xC0) { need = 1; cp = b & 0x1F; }
        else if ((b & 0xF0) == 0xE0) { need = 2; cp = b & 0x0F; }
        else if ((b & 0xF8) == 0xF0) { need = 3; cp = b & 0x07; }
        else { r.elems.push_back({Elem::Residue, b}); ++r.residue; ++i; continue; }
        if (i + size_t(need) >= n) { r.elems.push_back({Elem::Residue, b}); ++r.residue; ++i; continue; }
        bool ok = true;
        for (int k = 1; k <= need; ++k) if ((d[i+k] & 0xC0) != 0x80) { ok = false; break; }
        if (!ok) { r.elems.push_back({Elem::Residue, b}); ++r.residue; ++i; continue; }
        for (int k = 1; k <= need; ++k) cp = (cp << 6) | (d[i+k] & 0x3F);
        r.elems.push_back({Elem::Codepoint, cp}); ++r.codepoints; i += need + 1;
    }
}
void decodeLatin1(const uint8_t* d, size_t n, Result& r) {
    for (size_t i = 0; i < n; ++i) { r.elems.push_back({Elem::Codepoint, d[i]}); ++r.codepoints; }
}
void decodeUtf16(const uint8_t* d, size_t n, Endian e, Result& r) {
    auto rd = [&](size_t o) -> uint16_t {
        return e == Endian::Big ? (uint16_t(d[o]) << 8) | d[o+1] : (uint16_t(d[o+1]) << 8) | d[o];
    };
    size_t i = 0;
    while (i + 1 < n) {
        const uint16_t u = rd(i);
        if (u >= 0xD800 && u <= 0xDBFF) {                 // high surrogate
            if (i + 3 < n) {
                const uint16_t lo = rd(i+2);
                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                    const uint32_t cp = 0x10000 + ((uint32_t(u - 0xD800) << 10) | (lo - 0xDC00));
                    r.elems.push_back({Elem::Codepoint, cp}); ++r.codepoints; i += 4; continue;
                }
            }
            r.elems.push_back({Elem::Residue, d[i]}); ++r.residue; ++i; continue;   // lone surrogate
        }
        if (u >= 0xDC00 && u <= 0xDFFF) { r.elems.push_back({Elem::Residue, d[i]}); ++r.residue; ++i; continue; }
        r.elems.push_back({Elem::Codepoint, u}); ++r.codepoints; i += 2;
    }
    while (i < n) { r.elems.push_back({Elem::Residue, d[i]}); ++r.residue; ++i; }    // trailing odd byte
}
void decodeUtf32(const uint8_t* d, size_t n, Endian e, Result& r) {
    size_t i = 0;
    while (i + 3 < n) {
        const uint32_t cp = e == Endian::Big
            ? (uint32_t(d[i]) << 24) | (uint32_t(d[i+1]) << 16) | (uint32_t(d[i+2]) << 8) | d[i+3]
            : (uint32_t(d[i+3]) << 24) | (uint32_t(d[i+2]) << 16) | (uint32_t(d[i+1]) << 8) | d[i];
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) { r.elems.push_back({Elem::Residue, d[i]}); ++r.residue; ++i; }
        else { r.elems.push_back({Elem::Codepoint, cp}); ++r.codepoints; i += 4; }
    }
    while (i < n) { r.elems.push_back({Elem::Residue, d[i]}); ++r.residue; ++i; }
}
} // anonymous namespace

Result resolve(const uint8_t* data, size_t len, size_t sampleLimit) {
    Result r;
    const size_t cap = len < sampleLimit ? len : sampleLimit;

    // Adaptive sample (claim 617): grow the discrimination window only until the structure
    // is decisively resolved, then stop. The grow-vs-stop test is confidence:
    //   - multibyte (nulls present)        -> resolved, stop (nulls are a hard signal)
    //   - clean 1-byte table (UTF-8/Latin-1, no contradicting evidence) -> stop
    //   - pure-ASCII (table still a 3-way superposition) or mixed/min-violation -> NOT yet
    //     confident, keep growing; a representative prefix usually shows its hand fast, an
    //     uninformative ASCII prefix is grown past, and at the cap we settle == full scan.
    static const size_t kSteps[] = {256, 1024, 4096, 16384, 65536};
    SizeEndian se{ Size::One, Endian::None, 0.0, "empty" };
    TablePick  tp{ Table::Ascii, {}, 0.0, "" };
    size_t window = 0;
    for (size_t si = 0; ; ++si) {
        const size_t want = (si < sizeof(kSteps)/sizeof(kSteps[0])) ? kSteps[si] : cap;
        window = want < cap ? want : cap;
        se = phaseA(data, window);
        if (se.size != Size::One) break;                          // nulls decisive -> multibyte
        tp = phaseB_1byte(data, window);
        const bool cleanTable = tp.cands.empty() && tp.conf >= 0.9999;  // clean UTF-8/Latin-1
        if (cleanTable || window >= cap) break;                   // confident, or grown to cap
        // else: pure-ASCII superposition or mixed evidence -> grow the window
    }
    r.disc.sampledBytes = window;
    r.disc.size = se.size; r.disc.endian = se.endian;
    r.disc.confidence = se.conf; r.disc.evidence = se.ev;

    switch (se.size) {
    case Size::One: {
        r.disc.table = tp.table; r.disc.candidates = tp.cands;
        r.disc.evidence += " | " + tp.ev;
        r.disc.confidence = (r.disc.confidence + tp.conf) / 2.0;
        if (tp.table == Table::Latin1) decodeLatin1(data, len, r);
        else                           decodeUtf8(data, len, r);   // ASCII decodes via UTF-8 path
        break;
    }
    case Size::Two:  r.disc.table = Table::Utf16; decodeUtf16(data, len, se.endian, r); break;
    case Size::Four: r.disc.table = Table::Utf32; decodeUtf32(data, len, se.endian, r); break;
    default:         r.disc.size = Size::One; r.disc.table = Table::Utf8; decodeUtf8(data, len, r); break;
    }
    return r;
}

} // namespace hcp::bytefloor
