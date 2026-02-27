#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include "HCPWordSuperpositionTrial.h"  // CharRun

// Forward declarations — full PhysX headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxCudaContextManager;
    class PxPBDParticleSystem;
    class PxParticleBuffer;
    class PxPBDMaterial;
}

namespace HCPEngine
{
    class HCPVocabulary;
    class HCPBondTable;

    // ---- Constants (empirical, tunable) ----

    static constexpr float RC_Z_SCALE = 10.0f;
    static constexpr float RC_Y_OFFSET = 1.5f;
    static constexpr float RC_SETTLE_THRESHOLD = 0.5f;
    static constexpr float RC_VELOCITY_THRESHOLD = 3.0f;
    static constexpr float RC_CONTACT_OFFSET = 0.4f;
    static constexpr float RC_REST_OFFSET = 0.1f;
    static constexpr float RC_RUN_X_GAP = 2.0f;
    static constexpr float RC_DT = 1.0f / 60.0f;
    static constexpr int RC_SETTLE_STEPS = 60;
    static constexpr int RC_MAX_TIERS = 4;          // Tiers 0,1,2 + var fallback
    static constexpr AZ::u32 RC_TIER_0_MAX = 100;   // Words per tier (tunable)
    static constexpr AZ::u32 RC_TIER_1_MAX = 200;
    static constexpr AZ::u32 RC_TIER_2_MAX = 500;
    static constexpr AZ::u32 RC_STANDARD_BUFFER_CAPACITY = 8192;
    static constexpr AZ::u32 RC_BATCH_PARTICLE_BUDGET = 100000;  // Max particles per batch (VRAM safety)

    // ---- Tier Assembly (pure CPU) ----

    //! A vocabulary word scored and assigned to a frequency tier.
    struct TieredVocabEntry
    {
        AZStd::string word;       // Lowercase word form
        AZStd::string tokenId;    // Resolved token ID
        AZ::u32 bondCount;        // Aggregate PBM bond score (sum of adjacent char-pair bonds)
        AZ::u32 tierIndex;        // Assigned tier (0 = highest frequency)
    };

    //! Vocabulary for one (length, firstChar) chamber bucket.
    //! All words in this bucket share the same length and starting character.
    //! Entries are sorted by bondCount descending and assigned to tiers.
    struct ChamberVocab
    {
        AZ::u32 bucketKey;        // (length << 8) | firstCharLower
        AZ::u32 wordLength;
        char firstChar;
        AZStd::vector<TieredVocabEntry> entries;    // Sorted by bondCount desc
        AZStd::vector<AZ::u32> tierBoundaries;      // Index where each tier starts
        AZ::u32 tierCount;                           // Number of populated tiers
    };

    //! Builds tiered vocabulary from PBM bond data + vocabulary.
    //! Per (length, firstChar) bucket: sorts words by aggregate bond count,
    //! assigns to tiers (tier 0 = highest freq, tier 1, tier 2).
    //! Remaining words are excluded (var fallback path).
    class TierAssembly
    {
    public:
        //! Build tiered vocabulary from bond table and vocabulary.
        //! Bond count scoring: sum adjacent character pair bond strengths.
        //! "there" = GetBondStrength("t","h") + GetBondStrength("h","e") + ...
        void Build(const HCPBondTable& bondTable, const HCPVocabulary& vocab);

        //! Look up the vocabulary bucket for a given word length and first character.
        const ChamberVocab* GetBucket(AZ::u32 wordLength, char firstChar) const;

        size_t BucketCount() const { return m_buckets.size(); }
        size_t TotalWords() const { return m_totalWords; }

        void LogStats() const;

        static AZ::u32 MakeBucketKey(AZ::u32 len, char firstChar);

    private:

        AZStd::unordered_map<AZ::u32, ChamberVocab> m_buckets;
        size_t m_totalWords = 0;
    };

    //! Compute aggregate bond count for a word using the bond table.
    //! Sum of GetBondStrength(char[i], char[i+1]) for all adjacent pairs.
    AZ::u32 ComputeWordBondCount(const AZStd::string& word, const HCPBondTable& bondTable);

    // ---- Resolution Chamber (GPU, implemented in Steps 2-5) ----

    //! Tracking slot for a stream run loaded into a chamber buffer.
    struct StreamRunSlot
    {
        AZ::u32 runIndex;          // Index into the original runs array
        AZ::u32 bufferStart;       // First particle index in the buffer
        AZ::u32 charCount;         // Number of characters (= particles)
        AZStd::string runText;     // Lowercase run text (for match lookup)
        bool resolved = false;
        AZStd::string matchedWord;
        AZStd::string matchedTokenId;
        AZ::u32 tierResolved = 0xFF;  // Which tier resolved it (0xFF = unresolved)
    };

    //! Tracking slot for a vocab word in the chamber buffer.
    struct VocabWordSlot
    {
        AZ::u32 tierIndex;
        AZ::u32 entryIndex;        // Index into ChamberVocab::entries
        AZ::u32 bufferStart;       // First particle index in the buffer
        AZ::u32 charCount;
        AZ::u32 runSlotIndex;      // Which stream run this vocab copy serves
    };

    //! Result for a single run's resolution.
    struct ResolutionResult
    {
        AZStd::string runText;
        AZStd::string matchedWord;
        AZStd::string matchedTokenId;
        AZ::u32 tierResolved = 0xFF;  // 0xFF = unresolved
        bool resolved = false;
    };

    //! Full manifest from a resolution pass.
    struct ResolutionManifest
    {
        AZStd::vector<ResolutionResult> results;
        AZ::u32 totalRuns = 0;
        AZ::u32 resolvedRuns = 0;
        AZ::u32 unresolvedRuns = 0;
        float totalTimeMs = 0.0f;
    };

    //! One resolution chamber per (length, firstChar) group.
    //! Contains one PBD system + buffer with tiered vocab and stream runs.
    class ResolutionChamber
    {
    public:
        ResolutionChamber() = default;
        ~ResolutionChamber();

        // Non-copyable (owns GPU resources)
        ResolutionChamber(const ResolutionChamber&) = delete;
        ResolutionChamber& operator=(const ResolutionChamber&) = delete;
        ResolutionChamber(ResolutionChamber&& other) noexcept;
        ResolutionChamber& operator=(ResolutionChamber&& other) noexcept;

        bool Initialize(
            physx::PxPhysics* physics,
            physx::PxScene* scene,
            physx::PxCudaContextManager* cuda,
            const ChamberVocab& vocab);

        void LoadRuns(const AZStd::vector<CharRun>& runs,
                      const AZStd::vector<AZ::u32>& runIndices);

        bool SimulateTier(AZ::u32 tierIndex);

        void CheckSettlement(AZ::u32 tierIndex);

        void FlipStreamToTier(AZ::u32 nextTier);

        void CollectResults(AZStd::vector<ResolutionResult>& out);

        bool HasUnresolved() const;

        void Shutdown();

    private:
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxCudaContextManager* m_cuda = nullptr;
        physx::PxPBDParticleSystem* m_particleSystem = nullptr;
        physx::PxParticleBuffer* m_particleBuffer = nullptr;
        physx::PxPBDMaterial* m_material = nullptr;

        const ChamberVocab* m_vocab = nullptr;
        AZStd::vector<StreamRunSlot> m_streamSlots;
        AZStd::vector<VocabWordSlot> m_vocabSlots;
        AZ::u32 m_totalParticles = 0;
        AZ::u32 m_vocabParticleCount = 0;

        // Phase group IDs per tier (assigned by createPhase)
        AZStd::vector<AZ::u32> m_tierPhases;
        AZ::u32 m_inertPhase = 0;  // Phase group 0 = graveyard
    };

    //! Orchestrates all chambers in one PxScene.
    //! Groups runs by (length, firstChar), dispatches to chambers,
    //! runs the tier cascade, collects results.
    class ChamberManager
    {
    public:
        bool Initialize(
            physx::PxPhysics* physics,
            physx::PxScene* scene,
            physx::PxCudaContextManager* cuda,
            const TierAssembly& tiers);

        ResolutionManifest Resolve(const AZStd::vector<CharRun>& runs);

        void Shutdown();

    private:
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxCudaContextManager* m_cuda = nullptr;
        const TierAssembly* m_tiers = nullptr;
    };

} // namespace HCPEngine
