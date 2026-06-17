// byte-floor — stage-1 resolution: raw byte stream -> (size, endian, table) by
// self-discrimination, then decode to codepoints with byte-granularity residue.
//
// Doctrine (HCP claims 569/570): NAPIER interprets from byte codes up, never trusting
// headers. Determine code-unit SIZE + ENDIANNESS + candidate TABLE(s) by self-
// discrimination (byte patterns mutually exclude interpretations; 0x00 is the key
// discriminator for size/endian; presence AND absence are evidence). Filter, don't
// decide unless sure — hold a candidate set when ambiguous. Undecodable bytes are not
// errors; they are residue at byte granularity (the granularity gradient), forwarded.
//
// Every discrimination here is a uniform predicate over a flat array (null histogram,
// UTF-8 validity scan, range counts) — i.e. a memory-bandwidth-bound reduction, the
// shape that maps directly to a GPU dispatch. This CPU reference computes the same
// result correctly; the passes are kept discrete so the parallel port is mechanical.
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace hcp::bytefloor {

enum class Size  : uint8_t { Unknown = 0, One = 1, Two = 2, Four = 4 };
enum class Endian: uint8_t { None, Little, Big };
enum class Table : uint8_t { Unknown, Ascii, Utf8, Latin1, Utf16, Utf32 };

const char* name(Size);
const char* name(Endian);
const char* name(Table);

// One resolved element: a decoded codepoint, or a single raw residue byte (a byte the
// chosen table could not decode — held at byte granularity, never dropped).
struct Elem {
    enum Kind : uint8_t { Codepoint, Residue } kind;
    uint32_t value;  // Unicode scalar, or the raw byte (0..255) when kind==Residue
};

struct Discrimination {
    Size   size   = Size::Unknown;
    Endian endian = Endian::None;
    Table  table  = Table::Unknown;
    // Candidate set when the evidence does not single one out (e.g. pure-ASCII is
    // consistent with Ascii/Utf8/Latin1 — decode-identical, so reported as superposition
    // rather than a forced pick). Empty unless a superposition was held.
    std::vector<Table> candidates;
    double confidence = 0.0;   // 0..1, how decisively the evidence settled
    size_t sampledBytes = 0;   // how many bytes the adaptive probe actually read to settle
    std::string evidence;      // human-readable trace of what the bytes argued
};

struct Result {
    Discrimination disc;
    std::vector<Elem> elems;
    size_t codepoints = 0;
    size_t residue    = 0;
};

// Resolve a raw byte buffer. Discrimination uses an ADAPTIVE sample: the probe grows
// (256, 1K, 4K, 16K, ... up to sampleLimit) only until the structure is decisively
// resolved — multibyte nulls, or a clean 1-byte table. Pure-ASCII (table superposition
// unresolved) and mixed / min-violation evidence are not yet confident, so the probe keeps
// growing; at sampleLimit it settles whatever it has (== a full-sample scan). The decode
// always covers the whole buffer. (HCP claim 617: narrow by structure on an adaptive sample.)
Result resolve(const uint8_t* data, size_t len, size_t sampleLimit = 65536);

} // namespace hcp::bytefloor
