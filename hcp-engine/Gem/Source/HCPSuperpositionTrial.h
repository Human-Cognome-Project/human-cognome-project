#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

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

    //! A codepoint that collapsed onto its static particle — physics-confirmed character identity.
    struct CollapseResult
    {
        AZ::u32 streamPos;          // Position in codepoint stream (not byte stream)
        AZ::u32 codepoint;          // Unicode codepoint value
        AZ::u32 resolvedCodepoint;  // Codepoint this input resolved to
        float finalY;               // Final Y position (near 0 = settled on codepoint)
        bool settled;               // True if |Y| < threshold (confirmed match)
    };

    //! Result of the codepoint superposition trial.
    struct SuperpositionTrialResult
    {
        AZStd::vector<CollapseResult> collapses;   // Per-codepoint results
        AZ::u32 totalCodepoints = 0;               // Number of codepoints processed
        AZ::u32 totalBytes = 0;                    // Original UTF-8 byte count
        AZ::u32 settledCount = 0;                  // Codepoints that settled
        AZ::u32 unsettledCount = 0;                // Codepoints that didn't settle
        float simulationTimeMs = 0.0f;
        int simulationSteps = 0;
    };

    //! Decode a UTF-8 string into a vector of Unicode codepoints.
    //! Invalid sequences produce U+FFFD (replacement character).
    AZStd::vector<AZ::u32> DecodeUtf8ToCodepoints(const AZStd::string& input);

    //! Encode a single Unicode codepoint as UTF-8 bytes, appending to output.
    void AppendCodepointAsUtf8(AZStd::string& out, AZ::u32 cp);

    //! Run codepoint superposition trial (all-particle architecture).
    //!
    //! Input UTF-8 is first decoded to codepoints. For each codepoint,
    //! two PBD particles exist in the same system:
    //!   - A static codepoint particle (invMass=0) at (streamPos, 0, codepoint * Z_SCALE)
    //!   - A dynamic input particle (invMass=1) at (streamPos, Y_OFFSET, codepoint * Z_SCALE)
    //!
    //! Gravity pulls dynamic particles down. Each input particle contacts the
    //! static codepoint particle directly below it via PBD self-collision.
    //! The broadphase spatial hash discriminates on all 3 axes — particles at
    //! different stream positions (X) don't interact (spacing > 2*contactOffset).
    //!
    //! By operating on codepoints instead of raw bytes:
    //!   - Multi-byte UTF-8 sequences collapse to single particles
    //!   - Non-ASCII characters are fully supported
    //!   - Candidate space narrows (256 bytes → ~95 used ASCII codepoints for English)
    //!
    //! @param physics     PxPhysics instance
    //! @param scene       GPU-enabled PxScene
    //! @param cuda        CUDA context manager
    //! @param inputText   UTF-8 input text
    //! @param vocab       Vocabulary for validation comparison
    //! @param maxChars    Maximum codepoints to process
    //! @return Trial result with per-codepoint collapse data
    SuperpositionTrialResult RunSuperpositionTrial(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::string& inputText,
        const HCPVocabulary& vocab,
        AZ::u32 maxChars = 200);

} // namespace HCPEngine
