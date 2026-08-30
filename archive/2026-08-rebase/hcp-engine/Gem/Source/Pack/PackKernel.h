#pragma once
// ============================================================================
// HCP Pack slice — the compact-ID packer (portable CPU reference / ORACLE).
//
// This is the core job of the entry-point staging kernel (graph claim 606):
// turn a bounded window of warm-set entries into the EXACT shape the GPU runs
// on, and nothing more.
//
// Doctrine this enforces (graph claims, not invented here):
//   * 56  see-it/mint-it: every surface form is its OWN token. No runtime
//         morphological reconstruction. "dog" and "dogs" are distinct tokens
//         with distinct canonical ids. Morphology lives in the definition,
//         NOT in this store.
//   * 444 everything is COMPRESSED TOKEN_IDS in arrayed, fixed-stride layouts
//         indexed by ID — no surface-form keys, no pointer/hash overhead.
//   * 91  the LMDB schema is engine-shaped, not Postgres-symmetric: caps are
//         DERIVED from the data, not stored; denormalized for batch reads.
//   * 606/613 identity is content-blind on the GPU side and reattached by
//         POSITION (slot) on readback. The large canonical ids live ONLY in a
//         CPU-side ledger — they never enter the GPU's store.
//
// The GPU matcher needs exactly two things: the CHARACTERS to match, and the
// (compact) ID to return when they settle. This kernel produces precisely that.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <utility>

namespace HCP {
namespace Pack {

// One entry as it arrives from the warm set into the bounded staging window:
// the large canonical token id (CPU-side identity) + the surface-form chars.
struct WindowEntry
{
    uint64_t    canonicalId;   // large id — lives only CPU-side, never in the GPU store
    std::string chars;         // the surface form: the ONLY thing the GPU matches
};

// A per-length contiguous fixed-stride char array (claim 444 arrayed addressing).
// blob holds count*length bytes; slot i (0..count-1) occupies [i*length, (i+1)*length).
// The GPU bed is laid out per word-length, so this is the engine-shaped unit.
struct LengthBucket
{
    uint32_t    length        = 0;   // fixed stride for this bucket
    uint32_t    baseCompactId = 0;   // global compact id of slot 0 here
    uint32_t    count         = 0;   // DERIVED from blob.size()/length; not a stored field
    std::string blob;                // contiguous chars, fixed stride = length
};

struct PackResult
{
    uint32_t                  count = 0;   // entries in the window
    std::vector<LengthBucket> buckets;     // ascending by length (= ascending compact-id ranges)
    std::vector<uint64_t>     ledger;      // compactId -> canonicalId; CPU-SIDE ONLY, not written to LMDB
};

// Assign dense, window-local compact ids in SLOT ORDER (length ascending, then
// arrival order within a length), build the per-length fixed-stride char blobs,
// and the CPU-side compact->canonical ledger that makes position the join key.
inline PackResult PackWindow(const std::vector<WindowEntry>& entries)
{
    // Group entry indices by surface-form length, preserving arrival order.
    // std::map keeps lengths ascending so the read side can reconstruct the
    // identical compact-id assignment with no stored manifest.
    std::map<uint32_t, std::vector<uint32_t>> byLen;
    for (uint32_t i = 0; i < entries.size(); ++i)
        byLen[static_cast<uint32_t>(entries[i].chars.size())].push_back(i);

    PackResult r;
    r.count = static_cast<uint32_t>(entries.size());
    r.ledger.resize(entries.size());

    uint32_t compactId = 0;
    for (auto& kv : byLen)
    {
        const uint32_t len = kv.first;
        const std::vector<uint32_t>& idxs = kv.second;

        LengthBucket b;
        b.length        = len;
        b.baseCompactId = compactId;
        b.count         = static_cast<uint32_t>(idxs.size());
        b.blob.reserve(static_cast<size_t>(len) * idxs.size());

        for (uint32_t srcIdx : idxs)
        {
            b.blob.append(entries[srcIdx].chars);          // exactly len bytes
            r.ledger[compactId] = entries[srcIdx].canonicalId;
            ++compactId;
        }
        r.buckets.push_back(std::move(b));
    }
    return r;
}

// Address the arrayed store: surface-form chars for a global compact id.
// Returns empty if the compact id is out of range.
inline std::string CharsForCompactId(const PackResult& r, uint32_t compactId)
{
    for (const auto& b : r.buckets)
    {
        if (compactId >= b.baseCompactId && compactId < b.baseCompactId + b.count)
        {
            const uint32_t slot = compactId - b.baseCompactId;
            return b.blob.substr(static_cast<size_t>(slot) * b.length, b.length);
        }
    }
    return std::string();
}

} // namespace Pack
} // namespace HCP
