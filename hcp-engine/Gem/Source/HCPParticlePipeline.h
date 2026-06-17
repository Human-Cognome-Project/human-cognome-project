#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

// Pair-bond data types. These were once produced by a PhysX PBD "particle
// pipeline" (disassembly: text -> bonds); that PhysX path has been removed in
// the AZSL swap. The types remain as the stored form of a document, written and
// read by HCPPbmWriter / HCPPbmReader and produced by the manifest scanner.

namespace HCPEngine
{
    //! A bond triple: (token_a, token_b, count)
    struct Bond
    {
        AZStd::string tokenA;
        AZStd::string tokenB;
        int count;
    };

    //! Pair Bond Map — the stored form of a document.
    //! Each bond is (A, B, count) representing adjacent token pair occurrences.
    //! The set of bonds IS the document.
    struct PBMData
    {
        AZStd::vector<Bond> bonds;
        AZStd::string firstFpbA;  // First forward pair bond A-side
        AZStd::string firstFpbB;  // First forward pair bond B-side
        size_t totalPairs = 0;
        size_t uniqueTokens = 0;
    };

} // namespace HCPEngine
