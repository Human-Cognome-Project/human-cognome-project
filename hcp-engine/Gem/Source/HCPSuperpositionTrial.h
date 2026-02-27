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

    //! A byte that collapsed onto its codepoint particle — physics-confirmed character identity.
    struct CollapseResult
    {
        AZ::u32 streamPos;        // Position in input byte stream
        AZ::u8 byteValue;         // Original byte value
        char resolvedChar;         // Character this byte resolved to
        float finalY;              // Final Y position (near 0 = settled on codepoint)
        bool settled;              // True if |Y| < threshold (confirmed match)
    };

    //! Result of the byte→char superposition trial.
    struct SuperpositionTrialResult
    {
        AZStd::vector<CollapseResult> collapses;   // Per-byte results
        AZ::u32 totalBytes = 0;
        AZ::u32 settledCount = 0;                  // Bytes that found their codepoint
        AZ::u32 unsettledCount = 0;                // Bytes that didn't settle
        float simulationTimeMs = 0.0f;
        int simulationSteps = 0;
    };

    //! Run byte→char superposition trial (all-particle architecture).
    //!
    //! For each input byte, two PBD particles exist in the same system:
    //!   - A static codepoint particle (invMass=0) at (streamPos, 0, byteValue * Z_SCALE)
    //!   - A dynamic input particle (invMass=1) at (streamPos, Y_OFFSET, byteValue * Z_SCALE)
    //!
    //! Gravity pulls dynamic particles down. Each input particle contacts the
    //! static codepoint particle directly below it via PBD self-collision.
    //! The broadphase spatial hash discriminates on all 3 axes — particles at
    //! different stream positions (X) don't interact (spacing > 2*contactOffset).
    //!
    //! This is the all-particle foundation: codepoint particles carry phase
    //! groups encoding byte class. After settlement, resolved particles carry
    //! type information forward to char→word physics without serialization.
    //!
    //! @param physics     PxPhysics instance
    //! @param scene       GPU-enabled PxScene
    //! @param cuda        CUDA context manager
    //! @param inputText   Raw input byte stream
    //! @param vocab       Vocabulary for validation comparison
    //! @param maxChars    Maximum bytes to process
    //! @return Trial result with per-byte collapse data
    SuperpositionTrialResult RunSuperpositionTrial(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::string& inputText,
        const HCPVocabulary& vocab,
        AZ::u32 maxChars = 200);

} // namespace HCPEngine
