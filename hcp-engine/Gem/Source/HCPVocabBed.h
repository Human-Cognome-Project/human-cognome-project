#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include "HCPResolutionChamber.h"  // TierAssembly, ChamberVocab, ResolutionManifest, etc.

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
    // ---- Constants (persistent vocab beds) ----

    static constexpr AZ::u32 VB_MAX_PARTICLES_PER_BUFFER = 60000;  // 5K below empirical OOM ceiling
    static constexpr AZ::u32 VB_PHASE_GROUP_STRIDE = 8;            // Phase group IDs per bed (inert + up to 7 tiers)
    static constexpr AZ::u32 VB_DEFAULT_SLOTS_PER_GROUP = 4;       // Run slots per first-char group (tunable)

    // ---- Per first-char group within a bed ----

    struct BedSlotGroup
    {
        char firstChar;
        AZ::u32 vocabEntryCount;       // Number of vocab entries for this (length, firstChar)
        AZ::u32 slotsPerGroup;         // How many concurrent stream runs this group can hold
        AZ::u32 vocabParticlesPerSlot; // vocabEntryCount * wordLength
        float xOffset;                 // X-axis start position for this group's region
        AZ::u32 vocabBufferStart;      // First particle index of this group's vocab in the buffer
        AZ::u32 dynamicBufferStart;    // First particle index of this group's dynamic region
        AZ::u32 nextFreeSlot;          // Next available slot (reset per batch)
    };

    // ---- VocabBed: one persistent PBD system per word length ----

    class VocabBed
    {
    public:
        VocabBed() = default;
        ~VocabBed();

        // Non-copyable (owns GPU resources)
        VocabBed(const VocabBed&) = delete;
        VocabBed& operator=(const VocabBed&) = delete;
        VocabBed(VocabBed&& other) noexcept;
        VocabBed& operator=(VocabBed&& other) noexcept;

        bool Initialize(
            physx::PxPhysics* physics,
            physx::PxScene* scene,
            physx::PxCudaContextManager* cuda,
            AZ::u32 wordLength,
            const TierAssembly& tierAssembly,
            AZ::u32 slotsPerGroup,
            AZ::u32 phaseGroupBase);

        // Load stream runs into dynamic slots. Returns number of overflow runs (couldn't fit).
        AZ::u32 LoadDynamicRuns(
            const AZStd::vector<CharRun>& runs,
            const AZStd::vector<AZ::u32>& runIndices);

        void CheckSettlement(AZ::u32 tierIndex);

        void FlipToTier(AZ::u32 nextTier);

        void ResetDynamics();

        void CollectResults(AZStd::vector<ResolutionResult>& out);

        bool HasUnresolved() const;

        AZ::u32 GetMaxTierCount() const { return m_maxTierCount; }
        AZ::u32 GetWordLength() const { return m_wordLength; }
        bool HasPendingRuns() const { return !m_streamSlots.empty(); }

        void Shutdown();

    private:
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxCudaContextManager* m_cuda = nullptr;
        physx::PxPBDParticleSystem* m_particleSystem = nullptr;
        physx::PxParticleBuffer* m_particleBuffer = nullptr;
        physx::PxPBDMaterial* m_material = nullptr;

        AZ::u32 m_wordLength = 0;
        AZ::u32 m_totalVocabParticles = 0;   // Static vocab region size
        AZ::u32 m_maxDynamicParticles = 0;    // Dynamic region capacity
        AZ::u32 m_activeDynamicCount = 0;     // Currently loaded dynamic particles
        AZ::u32 m_maxParticles = 0;           // Total buffer capacity

        AZStd::vector<BedSlotGroup> m_groups;
        AZStd::unordered_map<char, AZ::u32> m_charToGroupIndex;

        AZStd::vector<StreamRunSlot> m_streamSlots;
        AZ::u32 m_maxTierCount = 0;

        // Phase group IDs per tier
        AZStd::vector<AZ::u32> m_tierPhases;
        AZ::u32 m_inertPhase = 0;

        // Vocab entries per group (pointers into TierAssembly — valid for bed lifetime)
        struct GroupVocabRef
        {
            const ChamberVocab* vocab;  // Points into TierAssembly bucket
        };
        AZStd::vector<GroupVocabRef> m_groupVocabs;
    };

    // ---- BedManager: owns all VocabBeds, replaces ChamberManager ----

    class BedManager
    {
    public:
        bool Initialize(
            physx::PxPhysics* physics,
            physx::PxScene* scene,
            physx::PxCudaContextManager* cuda,
            const TierAssembly& tiers);

        ResolutionManifest Resolve(const AZStd::vector<CharRun>& runs);

        void Shutdown();

        bool IsInitialized() const { return m_initialized; }

    private:
        // Resolve a batch of runs across ALL beds simultaneously.
        // Loads dynamics into all relevant beds, runs one shared tier cascade,
        // collects results, resets. Overflow runs returned for re-processing.
        void ResolvePass(
            const AZStd::vector<CharRun>& runs,
            const AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>>& runsByLength,
            AZStd::vector<ResolutionResult>& results,
            AZStd::vector<AZ::u32>& overflowRuns);

        bool m_initialized = false;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxCudaContextManager* m_cuda = nullptr;
        const TierAssembly* m_tiers = nullptr;

        AZStd::vector<VocabBed> m_beds;
        AZStd::unordered_map<AZ::u32, AZ::u32> m_lengthToBedIndex;  // wordLength -> index into m_beds
    };

} // namespace HCPEngine
