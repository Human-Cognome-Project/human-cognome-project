#include "HCPVocabBed.h"
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
    // ========================================================================
    // VocabBed — persistent PBD system for one word length
    // ========================================================================

    VocabBed::~VocabBed()
    {
        Shutdown();
    }

    VocabBed::VocabBed(VocabBed&& other) noexcept
        : m_physics(other.m_physics)
        , m_scene(other.m_scene)
        , m_cuda(other.m_cuda)
        , m_particleSystem(other.m_particleSystem)
        , m_particleBuffer(other.m_particleBuffer)
        , m_material(other.m_material)
        , m_wordLength(other.m_wordLength)
        , m_totalVocabParticles(other.m_totalVocabParticles)
        , m_maxDynamicParticles(other.m_maxDynamicParticles)
        , m_activeDynamicCount(other.m_activeDynamicCount)
        , m_maxParticles(other.m_maxParticles)
        , m_groups(AZStd::move(other.m_groups))
        , m_charToGroupIndex(AZStd::move(other.m_charToGroupIndex))
        , m_streamSlots(AZStd::move(other.m_streamSlots))
        , m_maxTierCount(other.m_maxTierCount)
        , m_tierPhases(AZStd::move(other.m_tierPhases))
        , m_inertPhase(other.m_inertPhase)
        , m_groupVocabs(AZStd::move(other.m_groupVocabs))
    {
        other.m_physics = nullptr;
        other.m_scene = nullptr;
        other.m_cuda = nullptr;
        other.m_particleSystem = nullptr;
        other.m_particleBuffer = nullptr;
        other.m_material = nullptr;
        other.m_wordLength = 0;
        other.m_totalVocabParticles = 0;
        other.m_maxDynamicParticles = 0;
        other.m_activeDynamicCount = 0;
        other.m_maxParticles = 0;
    }

    VocabBed& VocabBed::operator=(VocabBed&& other) noexcept
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
            m_wordLength = other.m_wordLength;
            m_totalVocabParticles = other.m_totalVocabParticles;
            m_maxDynamicParticles = other.m_maxDynamicParticles;
            m_activeDynamicCount = other.m_activeDynamicCount;
            m_maxParticles = other.m_maxParticles;
            m_groups = AZStd::move(other.m_groups);
            m_charToGroupIndex = AZStd::move(other.m_charToGroupIndex);
            m_streamSlots = AZStd::move(other.m_streamSlots);
            m_maxTierCount = other.m_maxTierCount;
            m_tierPhases = AZStd::move(other.m_tierPhases);
            m_inertPhase = other.m_inertPhase;
            m_groupVocabs = AZStd::move(other.m_groupVocabs);

            other.m_physics = nullptr;
            other.m_scene = nullptr;
            other.m_cuda = nullptr;
            other.m_particleSystem = nullptr;
            other.m_particleBuffer = nullptr;
            other.m_material = nullptr;
            other.m_wordLength = 0;
            other.m_totalVocabParticles = 0;
            other.m_maxDynamicParticles = 0;
            other.m_activeDynamicCount = 0;
            other.m_maxParticles = 0;
        }
        return *this;
    }

    bool VocabBed::Initialize(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        AZ::u32 wordLength,
        const TierAssembly& tierAssembly,
        AZ::u32 slotsPerGroup,
        AZ::u32 phaseGroupBase)
    {
        if (!physics || !scene || !cuda || wordLength < 2) return false;

        m_physics = physics;
        m_scene = scene;
        m_cuda = cuda;
        m_wordLength = wordLength;

        // Gather all (length, firstChar) buckets for this word length
        m_groups.clear();
        m_groupVocabs.clear();
        m_charToGroupIndex.clear();
        m_maxTierCount = 0;

        // Scan all 26 lowercase letters for buckets at this word length
        for (char c = 'a'; c <= 'z'; ++c)
        {
            const ChamberVocab* bucket = tierAssembly.GetBucket(wordLength, c);
            if (!bucket || bucket->entries.empty()) continue;

            BedSlotGroup group;
            group.firstChar = c;
            group.vocabEntryCount = static_cast<AZ::u32>(bucket->entries.size());
            group.slotsPerGroup = slotsPerGroup;
            group.vocabParticlesPerSlot = group.vocabEntryCount * wordLength;
            group.xOffset = 0.0f;  // Computed below
            group.vocabBufferStart = 0;
            group.dynamicBufferStart = 0;
            group.nextFreeSlot = 0;

            AZ::u32 idx = static_cast<AZ::u32>(m_groups.size());
            m_charToGroupIndex[c] = idx;
            m_groups.push_back(group);
            m_groupVocabs.push_back({ bucket });

            if (bucket->tierCount > m_maxTierCount)
                m_maxTierCount = bucket->tierCount;
        }

        if (m_groups.empty()) return false;

        // Auto-size slotsPerGroup to maximize buffer utilization.
        // Each additional slot costs: sum over groups of ((vocabEntryCount + 1) * wordLength)
        // because each slot replicates all vocab entries plus one stream particle per char position.
        AZ::u32 particlesPerSlotUnit = 0;
        for (const auto& grp : m_groups)
        {
            particlesPerSlotUnit += (grp.vocabEntryCount + 1) * wordLength;
        }

        if (particlesPerSlotUnit > 0)
        {
            slotsPerGroup = VB_MAX_PARTICLES_PER_BUFFER / particlesPerSlotUnit;
            if (slotsPerGroup < 1) slotsPerGroup = 1;
            // Cap at 256 to avoid degenerate allocation for tiny beds
            if (slotsPerGroup > 256) slotsPerGroup = 256;
        }

        // Compute particle counts and X offsets
        // Layout per group: [slot0_vocab | slot1_vocab | ... | slot0_stream | slot1_stream | ...]
        // Each slot contains: vocabEntryCount * wordLength vocab particles + wordLength stream particles
        // X-gap separates groups

        m_totalVocabParticles = 0;
        m_maxDynamicParticles = 0;
        float currentX = 0.0f;

        for (AZ::u32 g = 0; g < static_cast<AZ::u32>(m_groups.size()); ++g)
        {
            BedSlotGroup& grp = m_groups[g];
            grp.slotsPerGroup = slotsPerGroup;

            // Each run slot within a group occupies wordLength X positions.
            // Vocab is replicated per slot (same as current ResolutionChamber).
            // Slots are X-separated by wordLength + gap.
            grp.xOffset = currentX;
            grp.vocabBufferStart = m_totalVocabParticles;

            AZ::u32 vocabPerSlot = grp.vocabEntryCount * wordLength;
            AZ::u32 totalGroupVocab = vocabPerSlot * grp.slotsPerGroup;
            AZ::u32 totalGroupDynamic = wordLength * grp.slotsPerGroup;

            m_totalVocabParticles += totalGroupVocab;
            m_maxDynamicParticles += totalGroupDynamic;

            // Advance X: each slot = wordLength + gap, for slotsPerGroup slots
            currentX += static_cast<float>(grp.slotsPerGroup) *
                        (static_cast<float>(wordLength) + RC_RUN_X_GAP);
        }

        m_maxParticles = m_totalVocabParticles + m_maxDynamicParticles;

        if (m_maxParticles == 0) return false;

        // Create PBD particle system
        m_particleSystem = physics->createPBDParticleSystem(*cuda, 96);
        if (!m_particleSystem) return false;

        m_particleSystem->setRestOffset(RC_REST_OFFSET);
        m_particleSystem->setContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setParticleContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setSolidRestOffset(RC_REST_OFFSET);
        m_particleSystem->setSolverIterationCounts(4, 1);
        scene->addActor(*m_particleSystem);

        // Create PBD material
        m_material = physics->createPBDMaterial(
            0.2f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!m_material) { Shutdown(); return false; }

        // Create phase groups: inert + one per tier (offset by phaseGroupBase)
        m_inertPhase = 0;
        m_tierPhases.clear();
        for (AZ::u32 t = 0; t < m_maxTierCount; ++t)
        {
            physx::PxU32 phase = m_particleSystem->createPhase(
                m_material,
                physx::PxParticlePhaseFlags(
                    physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            m_tierPhases.push_back(phase);
        }

        // Create particle buffer
        m_particleBuffer = physics->createParticleBuffer(
            m_maxParticles, 1, cuda);
        if (!m_particleBuffer) { Shutdown(); return false; }

        // Write static vocab particles (once, at startup)
        {
            physx::PxScopedCudaLock lock(*cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(m_maxParticles);
            physx::PxVec4* hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(m_maxParticles);
            physx::PxU32* hostPhase = cuda->allocPinnedHostBuffer<physx::PxU32>(m_maxParticles);

            // Zero-init everything
            for (AZ::u32 i = 0; i < m_maxParticles; ++i)
            {
                hostPos[i] = physx::PxVec4(0.0f, -100.0f, 0.0f, 0.0f);  // Park unused far below
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = m_inertPhase;
            }

            // Write vocab particles per group, per slot
            AZ::u32 vocabIdx = 0;
            for (AZ::u32 g = 0; g < static_cast<AZ::u32>(m_groups.size()); ++g)
            {
                BedSlotGroup& grp = m_groups[g];
                grp.vocabBufferStart = vocabIdx;
                const ChamberVocab* vocab = m_groupVocabs[g].vocab;

                for (AZ::u32 s = 0; s < grp.slotsPerGroup; ++s)
                {
                    float slotXBase = grp.xOffset +
                        static_cast<float>(s) * (static_cast<float>(wordLength) + RC_RUN_X_GAP);

                    for (AZ::u32 e = 0; e < static_cast<AZ::u32>(vocab->entries.size()); ++e)
                    {
                        const TieredVocabEntry& entry = vocab->entries[e];
                        AZ::u32 phaseVal = (entry.tierIndex < m_tierPhases.size())
                            ? m_tierPhases[entry.tierIndex]
                            : m_inertPhase;

                        for (AZ::u32 c = 0; c < wordLength; ++c)
                        {
                            char ch = (c < entry.word.size()) ? entry.word[c] : '\0';
                            float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                            hostPos[vocabIdx] = physx::PxVec4(
                                slotXBase + static_cast<float>(c), 0.0f, z, 0.0f);  // invMass=0 (static)
                            hostVel[vocabIdx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                            hostPhase[vocabIdx] = phaseVal;
                            ++vocabIdx;
                        }
                    }
                }
            }

            // Dynamic region starts after all vocab
            AZ::u32 dynamicBase = m_totalVocabParticles;
            for (AZ::u32 g = 0; g < static_cast<AZ::u32>(m_groups.size()); ++g)
            {
                m_groups[g].dynamicBufferStart = dynamicBase;
                dynamicBase += m_groups[g].slotsPerGroup * wordLength;
            }

            // Upload to GPU
            cuda->copyHToD(devPos, hostPos, m_maxParticles);
            cuda->copyHToD(devVel, hostVel, m_maxParticles);
            cuda->copyHToD(devPhase, hostPhase, m_maxParticles);

            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
            cuda->freePinnedHostBuffer(hostPhase);
        }

        // Only vocab particles are active initially — no dynamics
        m_particleBuffer->setNbActiveParticles(m_totalVocabParticles);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        m_particleSystem->addParticleBuffer(m_particleBuffer);

        m_activeDynamicCount = 0;

        fprintf(stderr, "[VocabBed] len=%u: %zu groups, %u vocab particles, %u max dynamic, "
            "%u total capacity, %u slots/group, %u max tiers\n",
            wordLength, m_groups.size(), m_totalVocabParticles, m_maxDynamicParticles,
            m_maxParticles, slotsPerGroup, m_maxTierCount);
        fflush(stderr);

        return true;
    }

    AZ::u32 VocabBed::LoadDynamicRuns(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& runIndices)
    {
        if (!m_particleBuffer || !m_cuda || runIndices.empty()) return 0;

        m_streamSlots.clear();
        m_activeDynamicCount = 0;
        AZ::u32 overflowCount = 0;

        // Reset slot counters
        for (auto& grp : m_groups)
            grp.nextFreeSlot = 0;

        // Allocate pinned host buffer for dynamic region only
        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        physx::PxU32* hostPhase = nullptr;

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_maxDynamicParticles);
            hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_maxDynamicParticles);
            hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(m_maxDynamicParticles);
        }

        // Init dynamic region to parked state
        for (AZ::u32 i = 0; i < m_maxDynamicParticles; ++i)
        {
            hostPos[i] = physx::PxVec4(0.0f, -100.0f, 0.0f, 0.0f);
            hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
            hostPhase[i] = m_inertPhase;
        }

        AZ::u32 streamPhase = m_tierPhases.empty() ? m_inertPhase : m_tierPhases[0];

        for (AZ::u32 ri = 0; ri < static_cast<AZ::u32>(runIndices.size()); ++ri)
        {
            AZ::u32 runIdx = runIndices[ri];
            const CharRun& run = runs[runIdx];

            // Route to the correct first-char group
            char firstChar = run.text.empty() ? '\0' : run.text[0];
            auto it = m_charToGroupIndex.find(firstChar);
            if (it == m_charToGroupIndex.end())
            {
                ++overflowCount;  // No vocab group for this first char
                continue;
            }

            BedSlotGroup& grp = m_groups[it->second];

            if (grp.nextFreeSlot >= grp.slotsPerGroup)
            {
                ++overflowCount;  // Group full
                continue;
            }

            AZ::u32 slotIdx = grp.nextFreeSlot++;
            float slotXBase = grp.xOffset +
                static_cast<float>(slotIdx) * (static_cast<float>(m_wordLength) + RC_RUN_X_GAP);

            // Buffer offset within dynamic region
            AZ::u32 dynRegionOffset = (grp.dynamicBufferStart - m_totalVocabParticles) +
                                       slotIdx * m_wordLength;

            StreamRunSlot ss;
            ss.runIndex = runIdx;
            ss.bufferStart = m_totalVocabParticles + dynRegionOffset;  // Absolute buffer index
            ss.charCount = m_wordLength;
            ss.runText = run.text;
            ss.resolved = false;

            for (AZ::u32 c = 0; c < m_wordLength; ++c)
            {
                char ch = (c < run.text.size()) ? run.text[c] : '\0';
                float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                hostPos[dynRegionOffset + c] = physx::PxVec4(
                    slotXBase + static_cast<float>(c), RC_Y_OFFSET, z, 1.0f);  // invMass=1
                hostVel[dynRegionOffset + c] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[dynRegionOffset + c] = streamPhase;
            }

            m_streamSlots.push_back(ss);
            m_activeDynamicCount += m_wordLength;
        }

        // Upload dynamic region to GPU
        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            // Copy only the dynamic region (after vocab)
            m_cuda->copyHToD(
                devPos + m_totalVocabParticles,
                hostPos,
                m_maxDynamicParticles);
            m_cuda->copyHToD(
                devVel + m_totalVocabParticles,
                hostVel,
                m_maxDynamicParticles);
            m_cuda->copyHToD(
                devPhase + m_totalVocabParticles,
                hostPhase,
                m_maxDynamicParticles);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->setNbActiveParticles(m_totalVocabParticles + m_maxDynamicParticles);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

        return overflowCount;
    }

    void VocabBed::CheckSettlement(AZ::u32 tierIndex)
    {
        if (!m_particleBuffer || !m_cuda || m_streamSlots.empty()) return;

        AZ::u32 readbackCount = m_totalVocabParticles + m_maxDynamicParticles;

        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(readbackCount);
            hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(readbackCount);
            m_cuda->copyDToH(hostPos, devPos, readbackCount);
            m_cuda->copyDToH(hostVel, devVel, readbackCount);
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

                // Find matching vocab word — route to appropriate group
                char firstChar = slot.runText.empty() ? '\0' : slot.runText[0];
                auto it = m_charToGroupIndex.find(firstChar);
                if (it != m_charToGroupIndex.end())
                {
                    const ChamberVocab* vocab = m_groupVocabs[it->second].vocab;
                    for (const auto& entry : vocab->entries)
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
        }

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
        }
    }

    void VocabBed::FlipToTier(AZ::u32 nextTier)
    {
        if (!m_particleBuffer || !m_cuda) return;
        if (nextTier >= m_tierPhases.size()) return;

        AZ::u32 newPhase = m_tierPhases[nextTier];

        {
            physx::PxScopedCudaLock lock(*m_cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            // Only read/write the dynamic region
            AZ::u32 dynCount = m_maxDynamicParticles;
            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(dynCount);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(dynCount);
            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(dynCount);

            m_cuda->copyDToH(hostPos, devPos + m_totalVocabParticles, dynCount);
            m_cuda->copyDToH(hostVel, devVel + m_totalVocabParticles, dynCount);
            m_cuda->copyDToH(hostPhase, devPhase + m_totalVocabParticles, dynCount);

            for (const auto& slot : m_streamSlots)
            {
                AZ::u32 dynOffset = slot.bufferStart - m_totalVocabParticles;

                if (slot.resolved)
                {
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                    {
                        hostPhase[dynOffset + c] = m_inertPhase;
                    }
                }
                else
                {
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                    {
                        hostPos[dynOffset + c].y = RC_Y_OFFSET;
                        hostPos[dynOffset + c].w = 1.0f;
                        hostVel[dynOffset + c] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        hostPhase[dynOffset + c] = newPhase;
                    }
                }
            }

            m_cuda->copyHToD(devPos + m_totalVocabParticles, hostPos, dynCount);
            m_cuda->copyHToD(devVel + m_totalVocabParticles, hostVel, dynCount);
            m_cuda->copyHToD(devPhase + m_totalVocabParticles, hostPhase, dynCount);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
    }

    void VocabBed::ResetDynamics()
    {
        if (!m_particleBuffer) return;

        // Deactivate dynamic particles by setting count to vocab-only
        m_particleBuffer->setNbActiveParticles(m_totalVocabParticles);
        m_streamSlots.clear();
        m_activeDynamicCount = 0;

        for (auto& grp : m_groups)
            grp.nextFreeSlot = 0;
    }

    void VocabBed::CollectResults(AZStd::vector<ResolutionResult>& out)
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

    bool VocabBed::HasUnresolved() const
    {
        for (const auto& slot : m_streamSlots)
        {
            if (!slot.resolved) return true;
        }
        return false;
    }

    void VocabBed::Shutdown()
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
        m_groups.clear();
        m_groupVocabs.clear();
        m_charToGroupIndex.clear();
        m_tierPhases.clear();
        m_totalVocabParticles = 0;
        m_maxDynamicParticles = 0;
        m_activeDynamicCount = 0;
        m_maxParticles = 0;
    }

    // ========================================================================
    // BedManager — orchestrates all persistent vocab beds
    // ========================================================================

    bool BedManager::Initialize(
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

        // Determine which word lengths have vocab
        AZStd::unordered_map<AZ::u32, bool> lengthsWithVocab;
        for (char c = 'a'; c <= 'z'; ++c)
        {
            for (AZ::u32 len = 2; len <= 30; ++len)
            {
                const ChamberVocab* bucket = tiers.GetBucket(len, c);
                if (bucket && !bucket->entries.empty())
                {
                    lengthsWithVocab[len] = true;
                }
            }
        }

        // Sort word lengths for deterministic ordering
        AZStd::vector<AZ::u32> lengths;
        lengths.reserve(lengthsWithVocab.size());
        for (const auto& [len, _] : lengthsWithVocab)
            lengths.push_back(len);
        AZStd::sort(lengths.begin(), lengths.end());

        fprintf(stderr, "[BedManager] Initializing %zu beds for word lengths: ", lengths.size());
        for (AZ::u32 len : lengths)
            fprintf(stderr, "%u ", len);
        fprintf(stderr, "\n");
        fflush(stderr);

        m_beds.clear();
        m_beds.reserve(lengths.size());
        m_lengthToBedIndex.clear();

        AZ::u32 phaseGroupBase = 0;
        AZ::u32 bedCount = 0;

        for (AZ::u32 len : lengths)
        {
            m_beds.emplace_back();
            VocabBed& bed = m_beds.back();

            if (!bed.Initialize(physics, scene, cuda, len, tiers,
                                VB_DEFAULT_SLOTS_PER_GROUP, phaseGroupBase))
            {
                fprintf(stderr, "[BedManager] WARNING: Failed to init bed for len=%u, skipping\n", len);
                fflush(stderr);
                m_beds.pop_back();
                continue;
            }

            m_lengthToBedIndex[len] = static_cast<AZ::u32>(m_beds.size() - 1);
            phaseGroupBase += VB_PHASE_GROUP_STRIDE;
            ++bedCount;
        }

        m_initialized = true;

        fprintf(stderr, "[BedManager] Initialized: %u beds, %zu word lengths\n",
            bedCount, m_lengthToBedIndex.size());
        fflush(stderr);

        return true;
    }

    // Resolve a batch of runs across ALL beds simultaneously.
    // Groups by length, loads into all relevant beds at once,
    // runs one shared tier cascade (one simulate() steps all beds),
    // collects results, resets all beds. Returns overflow runs that
    // didn't fit in any slot.
    void BedManager::ResolvePass(
        const AZStd::vector<CharRun>& runs,
        const AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>>& runsByLength,
        AZStd::vector<ResolutionResult>& results,
        AZStd::vector<AZ::u32>& overflowRuns)
    {
        // Load dynamics into ALL beds simultaneously
        AZStd::vector<AZ::u32> activeBedIndices;
        AZ::u32 maxTierCount = 0;

        for (const auto& [len, indices] : runsByLength)
        {
            auto bedIt = m_lengthToBedIndex.find(len);
            if (bedIt == m_lengthToBedIndex.end()) continue;
            VocabBed& bed = m_beds[bedIt->second];

            AZ::u32 overflow = bed.LoadDynamicRuns(runs, indices);

            if (bed.HasPendingRuns())
            {
                activeBedIndices.push_back(bedIt->second);
                if (bed.GetMaxTierCount() > maxTierCount)
                    maxTierCount = bed.GetMaxTierCount();
            }

            // Track overflow: the last 'overflow' indices couldn't be loaded
            if (overflow > 0)
            {
                AZ::u32 loaded = static_cast<AZ::u32>(indices.size()) - overflow;
                for (AZ::u32 j = loaded; j < static_cast<AZ::u32>(indices.size()); ++j)
                    overflowRuns.push_back(indices[j]);
            }
        }

        if (activeBedIndices.empty()) return;

        fprintf(stderr, "[BedManager] ResolvePass: %zu active beds, max %u tiers\n",
            activeBedIndices.size(), maxTierCount);
        fflush(stderr);

        // One shared tier cascade — one simulate() steps ALL beds simultaneously
        for (AZ::u32 tier = 0; tier < maxTierCount; ++tier)
        {
            for (int step = 0; step < RC_SETTLE_STEPS; ++step)
            {
                m_scene->simulate(RC_DT);
                m_scene->fetchResults(true);
                m_scene->fetchResultsParticleSystem();
            }

            // Check settlement on all active beds
            for (AZ::u32 bi : activeBedIndices)
                m_beds[bi].CheckSettlement(tier);

            // Check if any bed still has unresolved
            bool anyUnresolved = false;
            for (AZ::u32 bi : activeBedIndices)
            {
                if (m_beds[bi].HasUnresolved())
                {
                    anyUnresolved = true;
                    break;
                }
            }

            if (!anyUnresolved) break;

            // Flip unresolved beds to next tier
            AZ::u32 nextTier = tier + 1;
            if (nextTier < maxTierCount)
            {
                for (AZ::u32 bi : activeBedIndices)
                {
                    if (m_beds[bi].HasUnresolved())
                        m_beds[bi].FlipToTier(nextTier);
                }
            }
        }

        // Collect results from all active beds, then reset
        for (AZ::u32 bi : activeBedIndices)
        {
            m_beds[bi].CollectResults(results);
            m_beds[bi].ResetDynamics();
        }
    }

    ResolutionManifest BedManager::Resolve(const AZStd::vector<CharRun>& runs)
    {
        ResolutionManifest manifest;
        manifest.totalRuns = static_cast<AZ::u32>(runs.size());

        if (!m_tiers || runs.empty() || !m_initialized)
        {
            manifest.unresolvedRuns = manifest.totalRuns;
            return manifest;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        // Group runs by word length
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> runsByLength;
        AZStd::vector<AZ::u32> noVocabRuns;

        for (AZ::u32 i = 0; i < static_cast<AZ::u32>(runs.size()); ++i)
        {
            const CharRun& run = runs[i];
            if (run.text.empty()) continue;

            AZ::u32 len = run.length;
            auto bedIt = m_lengthToBedIndex.find(len);

            if (bedIt != m_lengthToBedIndex.end())
            {
                char firstChar = run.text[0];
                const ChamberVocab* bucket = m_tiers->GetBucket(len, firstChar);
                if (bucket && !bucket->entries.empty())
                {
                    runsByLength[len].push_back(i);
                }
                else
                {
                    noVocabRuns.push_back(i);
                }
            }
            else
            {
                noVocabRuns.push_back(i);
            }
        }

        fprintf(stderr, "[BedManager] %zu runs with vocab across %zu lengths, %zu without vocab\n",
            runs.size() - noVocabRuns.size(), runsByLength.size(), noVocabRuns.size());
        fflush(stderr);

        // Resolve all beds simultaneously — overflow loops until all processed
        AZStd::vector<AZ::u32> overflowRuns;
        ResolvePass(runs, runsByLength, manifest.results, overflowRuns);

        // Handle overflow (runs that didn't fit in slots) with additional passes
        while (!overflowRuns.empty())
        {
            fprintf(stderr, "[BedManager] Overflow pass: %zu runs\n", overflowRuns.size());
            fflush(stderr);

            AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> overflowByLength;
            for (AZ::u32 idx : overflowRuns)
            {
                AZ::u32 len = runs[idx].length;
                overflowByLength[len].push_back(idx);
            }

            AZStd::vector<AZ::u32> nextOverflow;
            ResolvePass(runs, overflowByLength, manifest.results, nextOverflow);
            overflowRuns = AZStd::move(nextOverflow);
        }

        // Add no-vocab runs as unresolved
        for (AZ::u32 idx : noVocabRuns)
        {
            ResolutionResult r;
            r.runText = runs[idx].text;
            r.resolved = false;
            manifest.results.push_back(r);
        }

        // ---- Hyphen three-step cascade ----
        // Step 1 already done — full hyphenated forms were tried above.
        AZStd::vector<AZ::u32> hyphenUnresolved;
        for (AZ::u32 i = 0; i < static_cast<AZ::u32>(manifest.results.size()); ++i)
        {
            if (!manifest.results[i].resolved &&
                manifest.results[i].runText.find('-') != AZStd::string::npos)
            {
                hyphenUnresolved.push_back(i);
            }
        }

        if (!hyphenUnresolved.empty())
        {
            // Step 2: Strip hyphens → try as compound word
            AZStd::vector<CharRun> compoundRuns;
            AZStd::vector<AZ::u32> compoundToManifest;

            for (AZ::u32 mi : hyphenUnresolved)
            {
                const AZStd::string& text = manifest.results[mi].runText;
                AZStd::string compound;
                compound.reserve(text.size());
                for (char c : text)
                {
                    if (c != '-') compound += c;
                }

                if (compound.size() >= 2)
                {
                    CharRun cr;
                    cr.text = compound;
                    cr.startPos = 0;
                    cr.length = static_cast<AZ::u32>(compound.size());
                    compoundRuns.push_back(cr);
                    compoundToManifest.push_back(mi);
                }
            }

            if (!compoundRuns.empty())
            {
                AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> compoundsByLength;
                for (AZ::u32 i = 0; i < static_cast<AZ::u32>(compoundRuns.size()); ++i)
                {
                    AZ::u32 len = compoundRuns[i].length;
                    auto bedIt = m_lengthToBedIndex.find(len);
                    if (bedIt != m_lengthToBedIndex.end())
                    {
                        char fc = compoundRuns[i].text[0];
                        const ChamberVocab* bucket = m_tiers->GetBucket(len, fc);
                        if (bucket && !bucket->entries.empty())
                            compoundsByLength[len].push_back(i);
                    }
                }

                AZStd::vector<ResolutionResult> compoundResults;
                AZStd::vector<AZ::u32> compoundOverflow;
                ResolvePass(compoundRuns, compoundsByLength, compoundResults, compoundOverflow);

                // Map results back — compound runs come out in order per bed,
                // so we match by runText
                for (const auto& cr : compoundResults)
                {
                    if (!cr.resolved) continue;
                    // Find the manifest entry this compound maps to
                    for (AZ::u32 i = 0; i < static_cast<AZ::u32>(compoundRuns.size()); ++i)
                    {
                        if (compoundRuns[i].text == cr.runText)
                        {
                            AZ::u32 mi = compoundToManifest[i];
                            if (!manifest.results[mi].resolved)
                            {
                                manifest.results[mi].resolved = true;
                                manifest.results[mi].matchedWord = cr.matchedWord;
                                manifest.results[mi].matchedTokenId = cr.matchedTokenId;
                                manifest.results[mi].tierResolved = cr.tierResolved;
                            }
                        }
                    }
                }
            }

            // Step 3: Split at hyphens → resolve each segment independently
            AZStd::vector<AZ::u32> stillUnresolved;
            for (AZ::u32 mi : hyphenUnresolved)
            {
                if (!manifest.results[mi].resolved)
                    stillUnresolved.push_back(mi);
            }

            if (!stillUnresolved.empty())
            {
                struct SegmentMapping
                {
                    AZ::u32 manifestIndex;
                    AZ::u32 segmentCount;
                    AZ::u32 firstSegmentRun;
                };

                AZStd::vector<CharRun> segmentRuns;
                AZStd::vector<SegmentMapping> mappings;

                for (AZ::u32 mi : stillUnresolved)
                {
                    const AZStd::string& text = manifest.results[mi].runText;

                    SegmentMapping mapping;
                    mapping.manifestIndex = mi;
                    mapping.firstSegmentRun = static_cast<AZ::u32>(segmentRuns.size());
                    mapping.segmentCount = 0;

                    AZ::u32 segStart = 0;
                    for (AZ::u32 j = 0; j <= static_cast<AZ::u32>(text.size()); ++j)
                    {
                        if (j == static_cast<AZ::u32>(text.size()) || text[j] == '-')
                        {
                            if (j > segStart)
                            {
                                AZStd::string seg = text.substr(segStart, j - segStart);
                                if (seg.size() >= 2)
                                {
                                    CharRun cr;
                                    cr.text = seg;
                                    cr.startPos = 0;
                                    cr.length = static_cast<AZ::u32>(seg.size());
                                    segmentRuns.push_back(cr);
                                    ++mapping.segmentCount;
                                }
                            }
                            segStart = j + 1;
                        }
                    }

                    mappings.push_back(mapping);
                }

                if (!segmentRuns.empty())
                {
                    AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> segsByLength;
                    for (AZ::u32 i = 0; i < static_cast<AZ::u32>(segmentRuns.size()); ++i)
                    {
                        AZ::u32 len = segmentRuns[i].length;
                        auto bedIt = m_lengthToBedIndex.find(len);
                        if (bedIt != m_lengthToBedIndex.end())
                        {
                            char fc = segmentRuns[i].text[0];
                            const ChamberVocab* bucket = m_tiers->GetBucket(len, fc);
                            if (bucket && !bucket->entries.empty())
                                segsByLength[len].push_back(i);
                        }
                    }

                    AZStd::vector<ResolutionResult> segResults;
                    AZStd::vector<AZ::u32> segOverflow;
                    ResolvePass(segmentRuns, segsByLength, segResults, segOverflow);

                    // Build a lookup from segment run index to its result
                    AZStd::unordered_map<AZ::u32, const ResolutionResult*> segResultMap;
                    // segResults come out in bed order, not segment order.
                    // Match by runText against segmentRuns.
                    // Build resolved set indexed by segment run index.
                    AZStd::vector<bool> segResolved(segmentRuns.size(), false);
                    AZStd::vector<ResolutionResult> segResultsByIndex(segmentRuns.size());

                    for (const auto& sr : segResults)
                    {
                        // Find which segment run this matches
                        for (AZ::u32 i = 0; i < static_cast<AZ::u32>(segmentRuns.size()); ++i)
                        {
                            if (!segResolved[i] && segmentRuns[i].text == sr.runText)
                            {
                                segResolved[i] = sr.resolved;
                                segResultsByIndex[i] = sr;
                                break;
                            }
                        }
                    }

                    for (const auto& mapping : mappings)
                    {
                        if (mapping.segmentCount == 0) continue;

                        bool allResolved = true;
                        for (AZ::u32 s = 0; s < mapping.segmentCount; ++s)
                        {
                            AZ::u32 segIdx = mapping.firstSegmentRun + s;
                            if (segIdx >= segResolved.size() || !segResolved[segIdx])
                            {
                                allResolved = false;
                                break;
                            }
                        }

                        if (allResolved)
                        {
                            AZ::u32 mi = mapping.manifestIndex;
                            manifest.results[mi].resolved = true;
                            AZ::u32 firstSeg = mapping.firstSegmentRun;
                            manifest.results[mi].matchedWord = segResultsByIndex[firstSeg].matchedWord;
                            manifest.results[mi].matchedTokenId = segResultsByIndex[firstSeg].matchedTokenId;
                            manifest.results[mi].tierResolved = segResultsByIndex[firstSeg].tierResolved;
                        }
                    }
                }
            }
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

        fprintf(stderr, "[BedManager] Complete: %u/%u resolved (%.1f%%) in %.1f ms\n",
            manifest.resolvedRuns, manifest.totalRuns,
            manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f,
            manifest.totalTimeMs);
        fflush(stderr);

        return manifest;
    }

    void BedManager::Shutdown()
    {
        for (auto& bed : m_beds)
            bed.Shutdown();
        m_beds.clear();
        m_lengthToBedIndex.clear();
        m_initialized = false;
        m_tiers = nullptr;
    }

} // namespace HCPEngine
