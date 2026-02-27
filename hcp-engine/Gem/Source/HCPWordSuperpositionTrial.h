#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include "HCPSuperpositionTrial.h"

// Forward declarations — full PhysX headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxCudaContextManager;
}

namespace HCPEngine
{
    class HCPVocabulary;

    //! A character run extracted from the input stream.
    //! Runs are whitespace-delimited, edge-punctuation-stripped, lowercased.
    struct CharRun
    {
        AZStd::string text;       // Lowercase core (no edge punct)
        AZ::u32 startPos;         // Position in original input
        AZ::u32 length;           // Character count

        // Normalization metadata — capitalization is positional, lowercase is canonical
        bool firstCap = false;                   // First char was uppercase (Label pattern)
        bool allCaps = false;                    // All chars were uppercase (e.g. "NASA")
        AZStd::vector<AZ::u32> capMask;         // Run-relative positions that were uppercase
        // Rules:
        //   Normal lowercase  → firstCap=false, allCaps=false, capMask empty
        //   Label ("The")     → firstCap=true,  allCaps=false, capMask empty
        //   All caps ("NASA") → firstCap=false, allCaps=true,  capMask={0,1,2,3}
        //   Unusual ("eBook") → firstCap=false, allCaps=false, capMask={1}
        // Sentence-initial caps (after . ? ! \n or at stream pos 0) are suppressed:
        //   firstCap and allCaps are cleared (positional, not intrinsic)
    };

    //! Result for a single vocabulary word candidate tested against a run.
    struct WordCandidateResult
    {
        AZStd::string word;       // Vocabulary word form
        AZStd::string tokenId;    // Token ID if matched
        AZ::u32 settledChars;     // Characters that settled (Z-matched)
        AZ::u32 totalChars;       // Word length
        bool fullMatch;           // All characters settled = confirmed match
    };

    //! Result for a single run's superposition resolution.
    struct RunResolutionResult
    {
        CharRun run;
        AZ::u32 candidateCount;          // Vocab words tested
        WordCandidateResult matchedWord;  // The winning candidate (if any)
        bool resolved;                    // True if exactly one word fully matched
    };

    //! Full result of the char→word superposition trial.
    struct WordTrialResult
    {
        AZStd::vector<RunResolutionResult> runResults;
        AZ::u32 totalRuns = 0;
        AZ::u32 resolvedRuns = 0;
        AZ::u32 unresolvedRuns = 0;
        AZ::u32 totalCandidates = 0;
        AZ::u32 totalParticles = 0;
        float simulationTimeMs = 0.0f;
        int simulationSteps = 0;
    };

    //! Extract character runs from input text.
    //! Runs are whitespace-delimited, edge-punctuation-stripped, lowercased, ASCII-only.
    AZStd::vector<CharRun> ExtractRuns(const AZStd::string& text, AZ::u32 maxChars);

    //! Extract character runs from Phase 1 collapse results.
    //! Walks settled CollapseResult entries: unsettled bytes and whitespace are run boundaries,
    //! edge punctuation is stripped, alphanumeric content is accumulated and lowercased.
    //! Populates firstCap/capMask normalization metadata.
    AZStd::vector<CharRun> ExtractRunsFromCollapses(const SuperpositionTrialResult& trialResult);

    //! Run char→word superposition trial.
    //!
    //! For each whitespace-delimited run in the input:
    //!   - Run characters become static PBD particles (invMass=0) at Y=0
    //!   - Matching vocabulary words (same length + first char) become
    //!     dynamic particles (invMass=1) at Y-offset lanes
    //!   - Gravity pulls dynamic particles down
    //!   - Z encodes character identity (ascii * Z_SCALE) — broadphase prunes
    //!   - Matched characters settle onto the static reference
    //!   - Host-side N-element AND confirms full word match
    //!
    //! All runs processed simultaneously in one PBD system.
    //! Runs separated on X axis (gap > 2*contactOffset).
    //! Vocab words separated by Y-lane per word.
    //!
    //! @param physics     PxPhysics instance
    //! @param scene       GPU-enabled PxScene
    //! @param cuda        CUDA context manager
    //! @param inputText   Raw input byte stream
    //! @param vocab       Vocabulary for candidate lookup and validation
    //! @param maxChars    Maximum bytes to process
    //! @return Trial result with per-run resolution data
    WordTrialResult RunWordSuperpositionTrial(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::string& inputText,
        const HCPVocabulary& vocab,
        AZ::u32 maxChars = 200);

} // namespace HCPEngine
