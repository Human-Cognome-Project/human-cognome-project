#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <vector>          // std::vector for bulk vocab data (off AZ pool)
#include <unordered_map>   // std::unordered_map for m_vocabByLength (off AZ pool)
#include "HCPResolutionChamber.h"  // ResolutionManifest, ResolutionResult, StreamRunSlot, etc.
#include "HCPParticlePipeline.h"   // Bond, PBMData

// Forward declarations — full headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxCudaContextManager;
    class PxPBDParticleSystem;
    class PxParticleBuffer;
    class PxPBDMaterial;
}

struct MDB_env;  // LMDB environment (defined in lmdb.h)

namespace HCPEngine
{
    // ---- Constants ----

    //! Buffer capacity per workspace. Each workspace is a single PxPBDParticleSystem.
    //! Sized to fit larger vocab slices (RC_VOCAB_PER_PHASE × maxWordLength) + stream runs.
    static constexpr AZ::u32 WS_BUFFER_CAPACITY = 131072;

    //! Max word length handled by primary workspaces (X-width=10).
    static constexpr AZ::u32 WS_PRIMARY_MAX_LENGTH = 10;

    //! Number of primary workspaces (handle lengths 2-10, bulk of English).
    //! 3 enables triple-pipeline: one loading (CPU), one simulating (GPU), one draining (CPU).
    static constexpr AZ::u32 WS_PRIMARY_COUNT = 3;

    //! Number of extended workspaces (handle lengths 11-20+, sparse).
    static constexpr AZ::u32 WS_EXTENDED_COUNT = 2;

    //! Settlement threshold: particle is settled when velocity magnitude < this.
    static constexpr float WS_VELOCITY_SETTLE_THRESHOLD = 0.5f;

    // ---- VocabPack: CPU-side pre-built vocab data for one word length ----
    //
    // Combines ALL firstChar groups (a-z) into one pack per word length.
    // Host arrays are pre-computed at init time and memcpy'd into workspace
    // buffers at resolve time. Immutable after construction.

    struct VocabPack
    {
        AZ::u32 wordLength = 0;
        AZ::u32 totalVocabParticles = 0;   // Total static particles in this pack
        AZ::u32 vocabEntryCount = 0;        // Number of vocab words
        AZ::u32 maxTierCount = 0;           // Highest tier index + 1

        // Host-side pre-built particle arrays (positions, velocities, phases)
        // Positions: X=charIndex, Y=0, Z=ascii*Z_SCALE, W=0 (invMass=0, static)
        // Velocities: zero
        // Phases: logical tier index (remapped to actual phase group IDs at load time)
        // Note: std::vector (not AZStd) to keep bulk particle data on system heap,
        // avoiding exhaustion of the AZ allocator pool on large vocab phases.
        std::vector<float> positions;       // Flat: [x,y,z,w] * totalVocabParticles
        std::vector<AZ::u32> phases;        // Logical tier index per particle

        struct Entry
        {
            AZStd::string word;
            AZStd::string tokenId;
            AZ::u32 tierIndex;
        };
        AZStd::vector<Entry> entries;

        // O(1) settlement lookup per tier: tierLookup[tier][word] -> entry index
        AZStd::vector<AZStd::unordered_map<AZStd::string, AZ::u32>> tierLookup;
    };

    // ---- Workspace: one reusable GPU particle system with its own PxScene ----
    //
    // Created once at startup. Vocab data overwritten per cycle via CUDA memcpy.
    // Buffer layout: [vocab region (static, invMass=0)] [stream region (dynamic, invMass=1)]
    // Each workspace owns its own PxScene for pipelined GPU/CPU overlap:
    //   Scene A simulating (GPU) while Scene B is being read back (CPU)
    //   while Scene C is being loaded (CPU + LMDB prefetch).

    class Workspace
    {
    public:
        Workspace() = default;
        ~Workspace();

        // Non-copyable (owns GPU resources)
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;
        Workspace(Workspace&& other) noexcept;
        Workspace& operator=(Workspace&& other) noexcept;

        //! Create GPU resources: own PxScene + PxPBDParticleSystem + PxParticleBuffer.
        //! Each workspace creates its own scene on the shared CUDA context.
        //! maxTiers: number of tier phase groups to create (typically 3).
        bool Create(physx::PxPhysics* physics,
                    physx::PxCudaContextManager* cuda,
                    AZ::u32 bufferCapacity, AZ::u32 maxTiers);

        //! Overwrite vocab region with a VocabPack. Remaps logical tier→phase group IDs.
        //! Returns max stream slots available after vocab region.
        AZ::u32 LoadVocabPack(const VocabPack& pack, AZ::u32 wordLength);

        //! Load stream runs into dynamic region. Returns overflow count.
        AZ::u32 LoadStreamRuns(const AZStd::vector<CharRun>& runs,
                               const AZStd::vector<AZ::u32>& indices,
                               AZ::u32 wordLength);

        //! Check settlement against VocabPack's tier lookup.
        void CheckSettlement(AZ::u32 tierIndex, const VocabPack& pack);

        //! Phase-only flip for unresolved stream particles to next tier.
        void FlipToTier(AZ::u32 nextTier);

        //! Collect results from stream slots into output vector.
        void CollectResults(AZStd::vector<ResolutionResult>& out);

        //! True if any stream slot is unresolved.
        bool HasUnresolved() const;

        //! Clear dynamic region, ready for next cycle.
        void ResetDynamics();

        //! Add/remove particle system from own scene.
        void ActivateInScene();
        void DeactivateFromScene();
        bool IsActiveInScene() const { return m_activeInScene; }

        bool HasPendingRuns() const { return !m_streamSlots.empty(); }

        //! Kick off N simulation steps. Non-blocking — simulate() dispatches to GPU.
        //! Call FetchSimResults() to block until done, or IsSimDone() to poll.
        void BeginSimulate(int steps, float dt);

        //! Poll: has the most recent simulate() finished on this scene?
        bool IsSimDone() const;

        //! Block until simulation complete, then fetch particle system results.
        void FetchSimResults();

        //! Collect results, separating resolved and unresolved run indices.
        void CollectSplit(AZStd::vector<ResolutionResult>& resolved,
                          AZStd::vector<AZ::u32>& unresolvedRunIndices);

        //! Release all GPU resources including owned PxScene.
        void Shutdown();

    private:
        physx::PxPhysics* m_physics = nullptr;
        physx::PxScene* m_scene = nullptr;            // OWNED — one scene per workspace
        physx::PxCudaContextManager* m_cuda = nullptr;
        physx::PxPBDParticleSystem* m_particleSystem = nullptr;
        physx::PxParticleBuffer* m_particleBuffer = nullptr;
        physx::PxPBDMaterial* m_material = nullptr;
        bool m_ownsScene = false;                      // True when we created the scene

        AZ::u32 m_bufferCapacity = 0;       // Total buffer size
        AZ::u32 m_vocabParticleCount = 0;   // Current vocab region size (changes per cycle)
        AZ::u32 m_maxStreamSlots = 0;       // Stream capacity for current cycle
        AZ::u32 m_currentWordLength = 0;    // Word length of current loaded pack
        bool m_activeInScene = false;

        int m_pendingSteps = 0;              // Steps remaining in current BeginSimulate
        float m_simDt = 0.0f;               // dt for current simulation

        AZStd::vector<StreamRunSlot> m_streamSlots;

        // Phase group IDs per tier (persistent across cycles)
        AZStd::vector<AZ::u32> m_tierPhases;
        AZ::u32 m_inertPhase = 0;
        AZ::u32 m_maxTierCount = 0;
    };

    class HCPVocabulary;  // For punctuation lookups (declared in HCPVocabulary.h)

    // ---- Manifest scanner output ----
    //
    // The sorted manifest is the train manifest — each position carries its
    // payload (token_id + morph bits + cap flags). The scanner tallies bonds
    // between adjacent tokens as they pass, producing PBM bond data + positional
    // arrays in a single pass. No second traversal.

    struct ManifestScanResult
    {
        AZStd::vector<Bond> bonds;              // Aggregated adjacent-pair bonds
        AZStd::string firstFpbA;                // First forward pair bond A-side
        AZStd::string firstFpbB;                // First forward pair bond B-side
        size_t totalPairs = 0;
        size_t uniqueTokens = 0;

        // Positional data (parallel arrays, document order)
        AZStd::vector<AZStd::string> tokenIds;  // Token ID per position slot
        AZStd::vector<int> positions;            // Position number per slot
        AZStd::vector<AZ::u32> modifiers;        // Packed modifier per slot (morphBits<<2 | capFlags)
        AZStd::vector<AZStd::string> entityIds;  // Entity ID per slot (empty = not part of entity)
        int totalSlots = 0;                       // Total position slots in document
        size_t entityAnnotations = 0;             // Count of tokens with entity annotations
    };

    //! Scan a document-ordered manifest to produce PBM bond data + positions.
    //! Manifest MUST be sorted by runIndex before calling.
    //! Single pass: tallies bonds between adjacent tokens as they flow past.
    //! Unresolved runs become vars (VAR_PREFIX + surface text).
    ManifestScanResult ScanManifestToPBM(const ResolutionManifest& manifest);

    // ---- BedManager: orchestrates Workspace pool + phased vocab resolution ----
    //
    // Data source: pre-compiled LMDB (data/vocab.lmdb/) with per-length sub-databases.
    // Entries are frequency-ordered at compile time — Labels first, then freq-ranked,
    // then unranked. No Postgres at runtime, no sorting, no tier assignment.
    //
    // Internally: 2-4 reusable Workspaces, frequency-ordered vocab per length,
    // small phases (RC_VOCAB_PER_PHASE entries each), descending-length loop.
    // Each phase: tiny vocab + maximum stream slots → clean settlement.
    // Cycle through phases until all runs resolved or vocab exhausted.

    class BedManager
    {
    public:
        //! Initialize from pre-compiled LMDB vocab beds.
        //! Opens vbed_02..vbed_16 sub-databases, reads frequency-ordered entries,
        //! creates GPU workspaces (each with its own PxScene). No Postgres dependency.
        //! @param lmdbEnv Shared LMDB environment (from HCPVocabulary::GetLmdbEnv())
        //! @param vocabulary For punctuation word lookups at resolve time
        bool Initialize(
            physx::PxPhysics* physics,
            physx::PxCudaContextManager* cuda,
            MDB_env* lmdbEnv,
            HCPVocabulary* vocabulary);

        ResolutionManifest Resolve(const AZStd::vector<CharRun>& runs);

        //! Rebuild in-memory vocab index from LMDB w2t.
        //! Call after EnvelopeManager::ActivateEnvelope() to pick up the new working set.
        void RebuildVocab();

        void Shutdown();

        bool IsInitialized() const { return m_initialized; }

    private:
        //! Read filtered vocab entries directly from LMDB for one word length.
        //! Scans entries [startEntry, endEntry) and returns only those whose first char
        //! is in neededChars. Zero-copy LMDB read; caller owns the returned vector.
        std::vector<VocabPack::Entry> ReadFilteredVocabSlice(
            AZ::u32 wordLength,
            const AZStd::unordered_set<char>& neededChars,
            AZ::u32 startEntry,
            AZ::u32 endEntry) const;

        //! Build a VocabPack from a pre-filtered entry vector slice.
        VocabPack BuildVocabSliceFromEntries(AZ::u32 wordLength,
            const std::vector<VocabPack::Entry>& entries,
            AZ::u32 startEntry, AZ::u32 count) const;

        //! Get workspace pool for a given word length (primary or extended).
        AZStd::vector<Workspace*> GetWorkspacesForLength(AZ::u32 wordLength);

        //! Resolve a single phase's runs through workspace pool.
        void ResolvePhase(
            AZ::u32 wordLength,
            const AZStd::vector<CharRun>& runs,
            const AZStd::vector<AZ::u32>& runIndices,
            const VocabPack& phasePack,
            AZ::u32 phaseIndex,
            AZStd::vector<ResolutionResult>& results,
            AZStd::vector<AZ::u32>& unresolvedIndices);

        //! Pipelined phase cascade — overlaps GPU simulation of phase N with CPU
        //! preparation of phase N+1. filteredVocab is a pre-filtered slice from LMDB.
        //! Requires WS_PRIMARY_COUNT >= 3 for full triple-pipeline benefit.
        void RunPipelinedCascade(
            AZ::u32 wordLength,
            const AZStd::vector<CharRun>& runs,
            const std::vector<VocabPack::Entry>& filteredVocab,
            AZStd::vector<AZ::u32>& currentIndices,
            AZStd::vector<ResolutionResult>& results,
            AZ::u32& phaseIndex);

        //! Resolve runs of a single word length through Label + common phase cascade.
        //! Labels checked only against capitalized runs (firstCap/allCaps).
        //! Common vocab checked against all remaining unresolved runs.
        void ResolveLengthCycle(
            AZ::u32 wordLength,
            const AZStd::vector<CharRun>& runs,
            const AZStd::vector<AZ::u32>& runIndices,
            AZStd::vector<ResolutionResult>& results,
            AZStd::vector<AZ::u32>& unresolvedIndices);

        bool m_initialized = false;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxCudaContextManager* m_cuda = nullptr;
        HCPVocabulary* m_vocabulary = nullptr;  // For punctuation lookups

        // Workspace pools (created once at startup)
        AZStd::vector<Workspace> m_primaryWorkspaces;    // For lengths 2-10
        AZStd::vector<Workspace> m_extendedWorkspaces;   // For lengths 11-20+

        // LMDB environment + w2t handle.
        // w2t is populated by EnvelopeManager::ActivateEnvelope before resolve.
        // Call RebuildVocab() after each envelope activation to refresh in-memory index.
        MDB_env* m_lmdbEnv = nullptr;
        MDB_dbi m_vocabDbi = 0;
        bool m_vocabDbiOpen = false;

        // In-memory vocab index built from w2t on each RebuildVocab() call.
        // Grouped by word length, in insertion order (frequency-ordered by envelope query).
        std::unordered_map<AZ::u32, std::vector<VocabPack::Entry>> m_vocabByLength;

        // Label count per word length — zero until label tier is wired to envelope.
        AZStd::unordered_map<AZ::u32, AZ::u32> m_labelCountByLength;

        // Active word lengths (sorted descending)
        AZStd::vector<AZ::u32> m_activeWordLengths;
    };

} // namespace HCPEngine
