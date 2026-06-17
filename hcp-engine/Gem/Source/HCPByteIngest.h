#pragma once

#include <AzCore/std/containers/vector.h>
#include <cstdint>
#include <cstddef>

#include "HCPWordSuperpositionTrial.h"   // CharRun
#include "HCPResolutionChamber.h"        // ResolutionManifest

namespace HCPEngine
{
    class BedManager;

    //! THE SEAM: ingest raw bytes through the byte-floor (bytes -> positioned characters),
    //! segment into CharRuns the chambers resolve to words. This replaces the PhysX Phase-1
    //! char routine — it is deterministic, handles every encoding, carries the byte-level
    //! positional map (CharRun::byteStart/byteLen), and does NOT drop non-ASCII (it carries
    //! it for the chambers to leave unresolved rather than silently dropping like ExtractRuns).
    AZStd::vector<CharRun> IngestBytes(const uint8_t* data, size_t len);

    //! Convenience: ingest bytes and resolve them to words in one call — the connected path,
    //! bytes -> characters -> words.
    ResolutionManifest ResolveBytes(BedManager& bed, const uint8_t* data, size_t len);
}
