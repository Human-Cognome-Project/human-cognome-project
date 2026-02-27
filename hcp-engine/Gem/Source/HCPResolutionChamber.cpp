#include "HCPResolutionChamber.h"
#include "HCPBondCompiler.h"
#include "HCPVocabulary.h"

#include <AzCore/std/sort.h>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cmath>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>

namespace HCPEngine
{
    // ---- Bond scoring ----

    AZ::u32 ComputeWordBondCount(const AZStd::string& word, const HCPBondTable& bondTable)
    {
        if (word.size() < 2) return 0;

        AZ::u32 total = 0;
        for (size_t i = 0; i + 1 < word.size(); ++i)
        {
            AZStd::string a(1, word[i]);
            AZStd::string b(1, word[i + 1]);
            total += bondTable.GetBondStrength(a, b);
        }
        return total;
    }

    // ---- TierAssembly ----

    AZ::u32 TierAssembly::MakeBucketKey(AZ::u32 len, char firstChar)
    {
        return (len << 8) | static_cast<AZ::u8>(firstChar);
    }

    void TierAssembly::Build(const HCPBondTable& bondTable, const HCPVocabulary& vocab)
    {
        m_buckets.clear();
        m_totalWords = 0;

        const AZ::u32 tierLimits[] = { RC_TIER_0_MAX, RC_TIER_1_MAX, RC_TIER_2_MAX };
        constexpr AZ::u32 numTierLimits = 3;

        vocab.IterateWords([&](const AZStd::string& wordForm,
                               const AZStd::string& tokenId) -> bool
        {
            if (wordForm.empty()) return true;

            AZStd::string lower;
            lower.reserve(wordForm.size());
            for (size_t i = 0; i < wordForm.size(); ++i)
            {
                unsigned char uc = static_cast<unsigned char>(wordForm[i]);
                if (uc >= 128) return true;
                lower += static_cast<char>(std::tolower(uc));
            }

            if (lower.size() < 2) return true;

            AZ::u32 bondCount = ComputeWordBondCount(lower, bondTable);

            char firstChar = lower[0];
            AZ::u32 len = static_cast<AZ::u32>(lower.size());
            AZ::u32 key = MakeBucketKey(len, firstChar);

            auto& bucket = m_buckets[key];
            if (bucket.entries.empty())
            {
                bucket.bucketKey = key;
                bucket.wordLength = len;
                bucket.firstChar = firstChar;
                bucket.tierCount = 0;
            }

            TieredVocabEntry entry;
            entry.word = lower;
            entry.tokenId = tokenId;
            entry.bondCount = bondCount;
            entry.tierIndex = 0;

            bucket.entries.push_back(AZStd::move(entry));
            ++m_totalWords;

            return true;
        });

        size_t tieredWords = 0;
        size_t excludedWords = 0;

        for (auto& [key, bucket] : m_buckets)
        {
            AZStd::sort(bucket.entries.begin(), bucket.entries.end(),
                [](const TieredVocabEntry& a, const TieredVocabEntry& b)
                {
                    return a.bondCount > b.bondCount;
                });

            AZ::u32 totalCapacity = 0;
            for (AZ::u32 t = 0; t < numTierLimits; ++t)
                totalCapacity += tierLimits[t];

            if (bucket.entries.size() > totalCapacity)
            {
                excludedWords += bucket.entries.size() - totalCapacity;
                bucket.entries.resize(totalCapacity);
            }

            bucket.tierBoundaries.clear();
            bucket.tierBoundaries.push_back(0);

            AZ::u32 currentTier = 0;
            AZ::u32 tierStart = 0;

            for (AZ::u32 i = 0; i < static_cast<AZ::u32>(bucket.entries.size()); ++i)
            {
                AZ::u32 posInTier = i - tierStart;

                if (currentTier < numTierLimits && posInTier >= tierLimits[currentTier])
                {
                    ++currentTier;
                    tierStart = i;
                    bucket.tierBoundaries.push_back(i);
                }

                bucket.entries[i].tierIndex = currentTier;
            }

            bucket.tierCount = currentTier + 1;
            tieredWords += bucket.entries.size();
        }

        fprintf(stderr, "[TierAssembly] Built: %zu buckets, %zu tiered words, "
            "%zu excluded (var fallback)\n",
            m_buckets.size(), tieredWords, excludedWords);
        fflush(stderr);

        LogStats();
    }

    const ChamberVocab* TierAssembly::GetBucket(AZ::u32 wordLength, char firstChar) const
    {
        auto it = m_buckets.find(MakeBucketKey(wordLength, firstChar));
        if (it == m_buckets.end()) return nullptr;
        return &it->second;
    }

    void TierAssembly::LogStats() const
    {
        AZ::u32 tierCounts[RC_MAX_TIERS] = {};
        AZ::u64 tierBondSum[RC_MAX_TIERS] = {};

        for (const auto& [key, bucket] : m_buckets)
        {
            for (const auto& entry : bucket.entries)
            {
                if (entry.tierIndex < static_cast<AZ::u32>(RC_MAX_TIERS))
                {
                    tierCounts[entry.tierIndex]++;
                    tierBondSum[entry.tierIndex] += entry.bondCount;
                }
            }
        }

        for (int t = 0; t < RC_MAX_TIERS; ++t)
        {
            if (tierCounts[t] > 0)
            {
                float avgBond = static_cast<float>(tierBondSum[t]) / tierCounts[t];
                fprintf(stderr, "[TierAssembly]   Tier %d: %u words, avg bond count %.1f\n",
                    t, tierCounts[t], avgBond);
            }
        }

        AZStd::vector<const ChamberVocab*> sorted;
        sorted.reserve(m_buckets.size());
        for (const auto& [key, bucket] : m_buckets)
            sorted.push_back(&bucket);

        AZStd::sort(sorted.begin(), sorted.end(),
            [](const ChamberVocab* a, const ChamberVocab* b)
            {
                return a->entries.size() > b->entries.size();
            });

        fprintf(stderr, "[TierAssembly] Top 10 buckets:\n");
        for (size_t i = 0; i < 10 && i < sorted.size(); ++i)
        {
            const ChamberVocab* b = sorted[i];
            AZ::u32 topBond = b->entries.empty() ? 0 : b->entries[0].bondCount;
            AZ::u32 botBond = b->entries.empty() ? 0 : b->entries.back().bondCount;
            fprintf(stderr, "[TierAssembly]   len=%u first='%c': %zu words, "
                "%u tiers, bonds [%u..%u]\n",
                b->wordLength, b->firstChar,
                b->entries.size(), b->tierCount,
                botBond, topBond);
        }

        AZStd::vector<const TieredVocabEntry*> allEntries;
        for (const auto& [key, bucket] : m_buckets)
        {
            for (const auto& entry : bucket.entries)
                allEntries.push_back(&entry);
        }
        AZStd::sort(allEntries.begin(), allEntries.end(),
            [](const TieredVocabEntry* a, const TieredVocabEntry* b)
            {
                return a->bondCount > b->bondCount;
            });

        fprintf(stderr, "[TierAssembly] Top 5 words by bond count:\n");
        for (size_t i = 0; i < 5 && i < allEntries.size(); ++i)
        {
            const TieredVocabEntry* e = allEntries[i];
            fprintf(stderr, "[TierAssembly]   \"%s\" bond=%u tier=%u\n",
                e->word.c_str(), e->bondCount, e->tierIndex);
        }

        fflush(stderr);
    }

    // ========================================================================
    // ResolutionChamber — GPU-side physics resolution
    // ========================================================================

    ResolutionChamber::~ResolutionChamber()
    {
        Shutdown();
    }

    ResolutionChamber::ResolutionChamber(ResolutionChamber&& other) noexcept
        : m_physics(other.m_physics)
        , m_scene(other.m_scene)
        , m_cuda(other.m_cuda)
        , m_particleSystem(other.m_particleSystem)
        , m_particleBuffer(other.m_particleBuffer)
        , m_material(other.m_material)
        , m_vocab(other.m_vocab)
        , m_streamSlots(AZStd::move(other.m_streamSlots))
        , m_vocabSlots(AZStd::move(other.m_vocabSlots))
        , m_totalParticles(other.m_totalParticles)
        , m_vocabParticleCount(other.m_vocabParticleCount)
        , m_tierPhases(AZStd::move(other.m_tierPhases))
        , m_inertPhase(other.m_inertPhase)
    {
        other.m_physics = nullptr;
        other.m_scene = nullptr;
        other.m_cuda = nullptr;
        other.m_particleSystem = nullptr;
        other.m_particleBuffer = nullptr;
        other.m_material = nullptr;
        other.m_vocab = nullptr;
        other.m_totalParticles = 0;
        other.m_vocabParticleCount = 0;
    }

    ResolutionChamber& ResolutionChamber::operator=(ResolutionChamber&& other) noexcept
    {
        if (this != &other)
        {
            Shutdown();
            m_physics = other.m_physics;
            m_scene = other.m_scene;
            m_cuda = other.m_cuda;
            m_particleSystem = other.m_particleSystem;
            m_particleBuffer = other.m_particleBuffer;
            m_material = other.m_material;
            m_vocab = other.m_vocab;
            m_streamSlots = AZStd::move(other.m_streamSlots);
            m_vocabSlots = AZStd::move(other.m_vocabSlots);
            m_totalParticles = other.m_totalParticles;
            m_vocabParticleCount = other.m_vocabParticleCount;
            m_tierPhases = AZStd::move(other.m_tierPhases);
            m_inertPhase = other.m_inertPhase;

            other.m_physics = nullptr;
            other.m_scene = nullptr;
            other.m_cuda = nullptr;
            other.m_particleSystem = nullptr;
            other.m_particleBuffer = nullptr;
            other.m_material = nullptr;
            other.m_vocab = nullptr;
            other.m_totalParticles = 0;
            other.m_vocabParticleCount = 0;
        }
        return *this;
    }

    bool ResolutionChamber::Initialize(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const ChamberVocab& vocab)
    {
        if (!physics || !scene || !cuda) return false;

        m_physics = physics;
        m_scene = scene;
        m_cuda = cuda;
        m_vocab = &vocab;

        // Create PBD particle system
        m_particleSystem = physics->createPBDParticleSystem(*cuda, 96);
        if (!m_particleSystem) return false;

        m_particleSystem->setRestOffset(RC_REST_OFFSET);
        m_particleSystem->setContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setParticleContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setSolidRestOffset(RC_REST_OFFSET);
        m_particleSystem->setSolverIterationCounts(4, 1);
        scene->addActor(*m_particleSystem);

        // Create PBD material (same as Phase 1 trials)
        m_material = physics->createPBDMaterial(
            0.2f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!m_material) { Shutdown(); return false; }

        // Create phase groups: one per tier
        // Phase 0 = inert (no flags, no interactions)
        // createPhase auto-increments group IDs (0, 1, 2, ...)
        // Group ID + eSelfCollide → particles in same group interact
        m_inertPhase = 0;
        m_tierPhases.clear();
        for (AZ::u32 t = 0; t < vocab.tierCount; ++t)
        {
            physx::PxU32 phase = m_particleSystem->createPhase(
                m_material,
                physx::PxParticlePhaseFlags(
                    physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            m_tierPhases.push_back(phase);
        }

        return true;
    }

    void ResolutionChamber::LoadRuns(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& runIndices)
    {
        if (!m_particleSystem || !m_vocab || runIndices.empty()) return;

        AZ::u32 wordLen = m_vocab->wordLength;
        AZ::u32 numEntries = static_cast<AZ::u32>(m_vocab->entries.size());
        AZ::u32 numRuns = static_cast<AZ::u32>(runIndices.size());

        // Particle count: per run = (numEntries * wordLen) vocab + wordLen stream
        AZ::u32 particlesPerRun = numEntries * wordLen + wordLen;
        m_totalParticles = numRuns * particlesPerRun;
        m_vocabParticleCount = numRuns * numEntries * wordLen;

        if (m_totalParticles > RC_STANDARD_BUFFER_CAPACITY)
        {
            AZ::u32 maxRuns = RC_STANDARD_BUFFER_CAPACITY / particlesPerRun;
            if (maxRuns < 1) maxRuns = 1;
            if (numRuns > maxRuns)
            {
                fprintf(stderr, "[Chamber len=%u '%c'] Truncated %u -> %u runs (buffer limit %u)\n",
                    wordLen, m_vocab->firstChar, numRuns, maxRuns, RC_STANDARD_BUFFER_CAPACITY);
                fflush(stderr);
                numRuns = maxRuns;
            }
            m_totalParticles = numRuns * particlesPerRun;
            m_vocabParticleCount = numRuns * numEntries * wordLen;
        }

        // Create particle buffer
        m_particleBuffer = m_physics->createParticleBuffer(
            m_totalParticles, 1, m_cuda);
        if (!m_particleBuffer) return;

        m_streamSlots.clear();
        m_streamSlots.reserve(numRuns);
        m_vocabSlots.clear();

        // Fill buffer: vocab (static) then stream (dynamic) per run
        {
            physx::PxScopedCudaLock lock(*m_cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(m_totalParticles);

            AZ::u32 idx = 0;
            float nextXBase = 0.0f;

            for (AZ::u32 r = 0; r < numRuns; ++r)
            {
                AZ::u32 runIdx = runIndices[r];
                const CharRun& run = runs[runIdx];
                float xBase = nextXBase;

                // ---- Vocab particles (static, invMass=0) — all tiers at same spatial position ----
                for (AZ::u32 e = 0; e < numEntries; ++e)
                {
                    const TieredVocabEntry& entry = m_vocab->entries[e];
                    AZ::u32 phaseVal = (entry.tierIndex < m_tierPhases.size())
                        ? m_tierPhases[entry.tierIndex]
                        : m_inertPhase;

                    for (AZ::u32 c = 0; c < wordLen; ++c)
                    {
                        char ch = (c < entry.word.size()) ? entry.word[c] : '\0';
                        float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                        hostPos[idx] = physx::PxVec4(
                            xBase + static_cast<float>(c), 0.0f, z, 0.0f);  // invMass=0
                        hostVel[idx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        hostPhase[idx] = phaseVal;
                        ++idx;
                    }
                }

                // ---- Stream particles (dynamic, invMass=1) — start at tier 0 ----
                StreamRunSlot ss;
                ss.runIndex = runIdx;
                ss.bufferStart = idx;
                ss.charCount = wordLen;
                ss.runText = run.text;
                ss.resolved = false;

                AZ::u32 streamPhase = m_tierPhases.empty() ? m_inertPhase : m_tierPhases[0];

                for (AZ::u32 c = 0; c < wordLen; ++c)
                {
                    char ch = (c < run.text.size()) ? run.text[c] : '\0';
                    float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                    hostPos[idx] = physx::PxVec4(
                        xBase + static_cast<float>(c), RC_Y_OFFSET, z, 1.0f);  // invMass=1
                    hostVel[idx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                    hostPhase[idx] = streamPhase;
                    ++idx;
                }

                m_streamSlots.push_back(ss);
                nextXBase += static_cast<float>(wordLen) + RC_RUN_X_GAP;
            }

            // Upload to GPU
            m_cuda->copyHToD(devPos, hostPos, m_totalParticles);
            m_cuda->copyHToD(devVel, hostVel, m_totalParticles);
            m_cuda->copyHToD(devPhase, hostPhase, m_totalParticles);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->setNbActiveParticles(m_totalParticles);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        m_particleSystem->addParticleBuffer(m_particleBuffer);
    }

    bool ResolutionChamber::SimulateTier([[maybe_unused]] AZ::u32 tierIndex)
    {
        // For single-chamber testing: run simulation steps on the scene.
        // In multi-chamber mode, ChamberManager drives scene->simulate().
        if (!m_scene) return false;

        for (int step = 0; step < RC_SETTLE_STEPS; ++step)
        {
            m_scene->simulate(RC_DT);
            m_scene->fetchResults(true);
            m_scene->fetchResultsParticleSystem();
        }
        return true;
    }

    void ResolutionChamber::CheckSettlement(AZ::u32 tierIndex)
    {
        if (!m_particleBuffer || !m_cuda || !m_vocab) return;

        // D→H readback
        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            m_cuda->copyDToH(hostPos, devPos, m_totalParticles);
            m_cuda->copyDToH(hostVel, devVel, m_totalParticles);
        }

        for (auto& slot : m_streamSlots)
        {
            if (slot.resolved) continue;

            AZ::u32 settledCount = 0;
            for (AZ::u32 c = 0; c < slot.charCount; ++c)
            {
                AZ::u32 idx = slot.bufferStart + c;
                float y = hostPos[idx].y;
                float vy = hostVel[idx].y;
                if (fabsf(y) < RC_SETTLE_THRESHOLD && fabsf(vy) < RC_VELOCITY_THRESHOLD)
                    ++settledCount;
            }

            if (settledCount == slot.charCount)
            {
                slot.resolved = true;
                slot.tierResolved = tierIndex;

                // Find matching vocab word by string comparison
                for (const auto& entry : m_vocab->entries)
                {
                    if (entry.word == slot.runText)
                    {
                        slot.matchedWord = entry.word;
                        slot.matchedTokenId = entry.tokenId;
                        break;
                    }
                }
            }
        }

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
        }
    }

    void ResolutionChamber::FlipStreamToTier(AZ::u32 nextTier)
    {
        if (!m_particleBuffer || !m_cuda) return;
        if (nextTier >= m_tierPhases.size()) return;

        AZ::u32 newPhase = m_tierPhases[nextTier];

        // Read current state, update unresolved stream particles, re-upload
        {
            physx::PxScopedCudaLock lock(*m_cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_totalParticles);
            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(m_totalParticles);

            m_cuda->copyDToH(hostPos, devPos, m_totalParticles);
            m_cuda->copyDToH(hostVel, devVel, m_totalParticles);
            m_cuda->copyDToH(hostPhase, devPhase, m_totalParticles);

            for (const auto& slot : m_streamSlots)
            {
                if (slot.resolved)
                {
                    // Inert: zero flags, no interactions
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                    {
                        AZ::u32 idx = slot.bufferStart + c;
                        hostPhase[idx] = m_inertPhase;
                    }
                }
                else
                {
                    // Reset position to Y_OFFSET, zero velocity, new phase group
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                    {
                        AZ::u32 idx = slot.bufferStart + c;
                        hostPos[idx].y = RC_Y_OFFSET;
                        hostPos[idx].w = 1.0f;  // Keep invMass=1
                        hostVel[idx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        hostPhase[idx] = newPhase;
                    }
                }
            }

            m_cuda->copyHToD(devPos, hostPos, m_totalParticles);
            m_cuda->copyHToD(devVel, hostVel, m_totalParticles);
            m_cuda->copyHToD(devPhase, hostPhase, m_totalParticles);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
    }

    void ResolutionChamber::CollectResults(AZStd::vector<ResolutionResult>& out)
    {
        for (const auto& slot : m_streamSlots)
        {
            ResolutionResult r;
            r.runText = slot.runText;
            r.matchedWord = slot.matchedWord;
            r.matchedTokenId = slot.matchedTokenId;
            r.tierResolved = slot.tierResolved;
            r.resolved = slot.resolved;
            out.push_back(r);
        }
    }

    bool ResolutionChamber::HasUnresolved() const
    {
        for (const auto& slot : m_streamSlots)
        {
            if (!slot.resolved) return true;
        }
        return false;
    }

    void ResolutionChamber::Shutdown()
    {
        if (m_particleBuffer && m_particleSystem)
        {
            m_particleSystem->removeParticleBuffer(m_particleBuffer);
            m_particleBuffer->release();
            m_particleBuffer = nullptr;
        }

        if (m_material)
        {
            m_material->release();
            m_material = nullptr;
        }

        if (m_particleSystem && m_scene)
        {
            m_scene->removeActor(*m_particleSystem);
            m_particleSystem->release();
            m_particleSystem = nullptr;
        }

        m_streamSlots.clear();
        m_vocabSlots.clear();
        m_tierPhases.clear();
        m_totalParticles = 0;
        m_vocabParticleCount = 0;
    }

    // ========================================================================
    // ChamberManager — orchestrates all chambers
    // ========================================================================

    bool ChamberManager::Initialize(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const TierAssembly& tiers)
    {
        if (!physics || !scene || !cuda) return false;
        m_physics = physics;
        m_scene = scene;
        m_cuda = cuda;
        m_tiers = &tiers;
        return true;
    }

    ResolutionManifest ChamberManager::Resolve(const AZStd::vector<CharRun>& runs)
    {
        ResolutionManifest manifest;
        manifest.totalRuns = static_cast<AZ::u32>(runs.size());

        if (!m_tiers || runs.empty())
        {
            manifest.unresolvedRuns = manifest.totalRuns;
            return manifest;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        // ---- Group runs by (length, firstChar) bucket ----
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> runsByBucket;
        AZStd::vector<AZ::u32> noVocabRuns;

        for (AZ::u32 i = 0; i < static_cast<AZ::u32>(runs.size()); ++i)
        {
            const CharRun& run = runs[i];
            if (run.text.empty()) continue;

            AZ::u32 key = TierAssembly::MakeBucketKey(run.length, run.text[0]);
            const ChamberVocab* bucket = m_tiers->GetBucket(run.length, run.text[0]);

            if (bucket && !bucket->entries.empty())
            {
                runsByBucket[key].push_back(i);
            }
            else
            {
                noVocabRuns.push_back(i);
            }
        }

        // ---- Pre-compute particle count per bucket for batching ----
        struct BucketWork
        {
            AZ::u32 key;
            AZ::u32 particleCount;
        };

        AZStd::vector<BucketWork> allWork;
        allWork.reserve(runsByBucket.size());
        AZ::u32 grandTotalParticles = 0;

        for (const auto& [key, indices] : runsByBucket)
        {
            AZ::u32 len = key >> 8;
            char firstChar = static_cast<char>(key & 0xFF);
            const ChamberVocab* bucket = m_tiers->GetBucket(len, firstChar);
            if (!bucket) continue;

            AZ::u32 numRuns = static_cast<AZ::u32>(indices.size());
            AZ::u32 numEntries = static_cast<AZ::u32>(bucket->entries.size());
            AZ::u32 particlesPerRun = numEntries * bucket->wordLength + bucket->wordLength;
            AZ::u32 particles = numRuns * particlesPerRun;

            // Respect per-chamber buffer cap
            if (particles > RC_STANDARD_BUFFER_CAPACITY)
            {
                AZ::u32 maxRuns = RC_STANDARD_BUFFER_CAPACITY / particlesPerRun;
                if (maxRuns < 1) maxRuns = 1;
                particles = maxRuns * particlesPerRun;
            }

            allWork.push_back({ key, particles });
            grandTotalParticles += particles;
        }

        // Sort by particle count descending — pack large chambers first
        AZStd::sort(allWork.begin(), allWork.end(),
            [](const BucketWork& a, const BucketWork& b)
            { return a.particleCount > b.particleCount; });

        // ---- Batch chambers by particle budget ----
        AZStd::vector<AZStd::vector<AZ::u32>> batches;  // Each batch = list of indices into allWork
        AZStd::vector<AZ::u32> batchParticles;           // Particle count per batch

        for (AZ::u32 w = 0; w < static_cast<AZ::u32>(allWork.size()); ++w)
        {
            AZ::u32 pc = allWork[w].particleCount;

            // Find first batch with room
            bool placed = false;
            for (AZ::u32 b = 0; b < static_cast<AZ::u32>(batches.size()); ++b)
            {
                if (batchParticles[b] + pc <= RC_BATCH_PARTICLE_BUDGET)
                {
                    batches[b].push_back(w);
                    batchParticles[b] += pc;
                    placed = true;
                    break;
                }
            }

            if (!placed)
            {
                batches.push_back({ w });
                batchParticles.push_back(pc);
            }
        }

        fprintf(stderr, "[ChamberManager] %zu chambers, %u total particles -> %zu batches "
            "(budget %u/batch), %zu runs with vocab, %zu runs without\n",
            allWork.size(), grandTotalParticles, batches.size(),
            RC_BATCH_PARTICLE_BUDGET,
            runs.size() - noVocabRuns.size(), noVocabRuns.size());
        fflush(stderr);

        // ---- Process each batch ----
        for (AZ::u32 batchIdx = 0; batchIdx < static_cast<AZ::u32>(batches.size()); ++batchIdx)
        {
            const auto& batch = batches[batchIdx];

            struct ChamberEntry
            {
                AZ::u32 bucketKey;
                ResolutionChamber chamber;
            };

            AZStd::vector<ChamberEntry> chambers;
            chambers.reserve(batch.size());
            AZ::u32 batchPC = 0;

            for (AZ::u32 workIdx : batch)
            {
                AZ::u32 key = allWork[workIdx].key;
                AZ::u32 len = key >> 8;
                char firstChar = static_cast<char>(key & 0xFF);
                const ChamberVocab* bucket = m_tiers->GetBucket(len, firstChar);
                if (!bucket) continue;

                chambers.emplace_back();
                ChamberEntry& entry = chambers.back();
                entry.bucketKey = key;

                if (!entry.chamber.Initialize(m_physics, m_scene, m_cuda, *bucket))
                {
                    chambers.pop_back();
                    continue;
                }

                entry.chamber.LoadRuns(runs, runsByBucket[key]);
                batchPC += allWork[workIdx].particleCount;
            }

            fprintf(stderr, "[ChamberManager] Batch %u/%zu: %zu chambers, %u particles\n",
                batchIdx + 1, batches.size(), chambers.size(), batchPC);
            fflush(stderr);

            // ---- Tier cascade for this batch ----
            AZ::u32 maxTierCount = 0;
            for (const auto& entry : chambers)
            {
                AZ::u32 len = entry.bucketKey >> 8;
                char fc = static_cast<char>(entry.bucketKey & 0xFF);
                const ChamberVocab* bucket = m_tiers->GetBucket(len, fc);
                if (bucket && bucket->tierCount > maxTierCount)
                    maxTierCount = bucket->tierCount;
            }

            for (AZ::u32 tier = 0; tier < maxTierCount; ++tier)
            {
                // Simulate all chambers in this batch simultaneously
                for (int step = 0; step < RC_SETTLE_STEPS; ++step)
                {
                    m_scene->simulate(RC_DT);
                    m_scene->fetchResults(true);
                    m_scene->fetchResultsParticleSystem();
                }

                for (auto& entry : chambers)
                    entry.chamber.CheckSettlement(tier);

                bool anyUnresolved = false;
                for (const auto& entry : chambers)
                {
                    if (entry.chamber.HasUnresolved())
                    {
                        anyUnresolved = true;
                        break;
                    }
                }

                if (!anyUnresolved)
                    break;

                AZ::u32 nextTier = tier + 1;
                if (nextTier < maxTierCount)
                {
                    for (auto& entry : chambers)
                    {
                        if (entry.chamber.HasUnresolved())
                            entry.chamber.FlipStreamToTier(nextTier);
                    }
                }
            }

            // Collect results from this batch
            for (auto& entry : chambers)
                entry.chamber.CollectResults(manifest.results);

            // Shutdown batch chambers — free VRAM for next batch
            for (auto& entry : chambers)
                entry.chamber.Shutdown();
            chambers.clear();
        }

        // Add no-vocab runs as unresolved
        for (AZ::u32 idx : noVocabRuns)
        {
            ResolutionResult r;
            r.runText = runs[idx].text;
            r.resolved = false;
            manifest.results.push_back(r);
        }

        // Count resolved/unresolved
        for (const auto& r : manifest.results)
        {
            if (r.resolved)
                manifest.resolvedRuns++;
            else
                manifest.unresolvedRuns++;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        manifest.totalTimeMs = static_cast<float>(
            std::chrono::duration<double, std::milli>(t1 - t0).count());

        fprintf(stderr, "[ChamberManager] Complete: %u/%u resolved (%.1f%%) in %.1f ms, %zu batches\n",
            manifest.resolvedRuns, manifest.totalRuns,
            manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f,
            manifest.totalTimeMs, batches.size());
        fflush(stderr);

        return manifest;
    }

    void ChamberManager::Shutdown()
    {
        m_tiers = nullptr;
    }

} // namespace HCPEngine
