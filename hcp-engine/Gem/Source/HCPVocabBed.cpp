#include "HCPVocabBed.h"
#include "HCPVocabulary.h"
#include "HCPEnvelopeManager.h"

#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_set.h>
#include <lmdb.h>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cstring>
#include <regex>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>
#include <System/PhysXSystem.h>

namespace HCPEngine
{
    // ========================================================================
    // Workspace — one reusable GPU particle system
    //
    // Created once at startup. Vocab overwritten per cycle via CUDA memcpy.
    // Buffer: [vocab region (static)] [stream region (dynamic)]
    // ========================================================================

    Workspace::~Workspace()
    {
        Shutdown();
    }

    Workspace::Workspace(Workspace&& other) noexcept
        : m_physics(other.m_physics)
        , m_scene(other.m_scene)
        , m_cuda(other.m_cuda)
        , m_particleSystem(other.m_particleSystem)
        , m_particleBuffer(other.m_particleBuffer)
        , m_material(other.m_material)
        , m_ownsScene(other.m_ownsScene)
        , m_bufferCapacity(other.m_bufferCapacity)
        , m_vocabParticleCount(other.m_vocabParticleCount)
        , m_maxStreamSlots(other.m_maxStreamSlots)
        , m_currentWordLength(other.m_currentWordLength)
        , m_activeInScene(other.m_activeInScene)
        , m_pendingSteps(other.m_pendingSteps)
        , m_simDt(other.m_simDt)
        , m_streamSlots(AZStd::move(other.m_streamSlots))
        , m_tierPhases(AZStd::move(other.m_tierPhases))
        , m_inertPhase(other.m_inertPhase)
        , m_maxTierCount(other.m_maxTierCount)
    {
        other.m_physics = nullptr;
        other.m_scene = nullptr;
        other.m_cuda = nullptr;
        other.m_particleSystem = nullptr;
        other.m_particleBuffer = nullptr;
        other.m_material = nullptr;
        other.m_ownsScene = false;
        other.m_bufferCapacity = 0;
        other.m_vocabParticleCount = 0;
        other.m_maxStreamSlots = 0;
        other.m_currentWordLength = 0;
        other.m_activeInScene = false;
        other.m_pendingSteps = 0;
        other.m_simDt = 0.0f;
        other.m_inertPhase = 0;
        other.m_maxTierCount = 0;
    }

    Workspace& Workspace::operator=(Workspace&& other) noexcept
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
            m_ownsScene = other.m_ownsScene;
            m_bufferCapacity = other.m_bufferCapacity;
            m_vocabParticleCount = other.m_vocabParticleCount;
            m_maxStreamSlots = other.m_maxStreamSlots;
            m_currentWordLength = other.m_currentWordLength;
            m_activeInScene = other.m_activeInScene;
            m_pendingSteps = other.m_pendingSteps;
            m_simDt = other.m_simDt;
            m_streamSlots = AZStd::move(other.m_streamSlots);
            m_tierPhases = AZStd::move(other.m_tierPhases);
            m_inertPhase = other.m_inertPhase;
            m_maxTierCount = other.m_maxTierCount;

            other.m_physics = nullptr;
            other.m_scene = nullptr;
            other.m_cuda = nullptr;
            other.m_particleSystem = nullptr;
            other.m_particleBuffer = nullptr;
            other.m_material = nullptr;
            other.m_ownsScene = false;
            other.m_bufferCapacity = 0;
            other.m_vocabParticleCount = 0;
            other.m_maxStreamSlots = 0;
            other.m_currentWordLength = 0;
            other.m_activeInScene = false;
            other.m_pendingSteps = 0;
            other.m_simDt = 0.0f;
            other.m_inertPhase = 0;
            other.m_maxTierCount = 0;
        }
        return *this;
    }

    bool Workspace::Create(
        physx::PxPhysics* physics,
        physx::PxCudaContextManager* cuda,
        AZ::u32 bufferCapacity,
        AZ::u32 maxTiers)
    {
        if (!physics || !cuda || bufferCapacity == 0) return false;

        m_physics = physics;
        m_cuda = cuda;
        m_bufferCapacity = bufferCapacity;
        m_maxTierCount = maxTiers;

        // Create dedicated PxScene for this workspace (pipelined: GPU simulates
        // one scene while CPU reads/loads others)
        {
            PhysX::PhysXSystem* physxSystem = PhysX::GetPhysXSystem();
            physx::PxCpuDispatcher* cpuDispatcher = physxSystem
                ? physxSystem->GetPxCpuDispathcher() : nullptr;
            if (!cpuDispatcher) return false;

            physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
            sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
            sceneDesc.cpuDispatcher = cpuDispatcher;
            sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
            sceneDesc.cudaContextManager = cuda;
            sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
            sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
            sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;

            m_scene = physics->createScene(sceneDesc);
            if (!m_scene) return false;
            m_ownsScene = true;
        }

        // Create PBD particle system
        m_particleSystem = physics->createPBDParticleSystem(*cuda, 96);
        if (!m_particleSystem) return false;

        m_particleSystem->setRestOffset(RC_REST_OFFSET);
        m_particleSystem->setContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setParticleContactOffset(RC_CONTACT_OFFSET);
        m_particleSystem->setSolidRestOffset(RC_REST_OFFSET);
        m_particleSystem->setSolverIterationCounts(4, 1);
        m_activeInScene = false;

        // Create PBD material
        m_material = physics->createPBDMaterial(
            0.2f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!m_material) { Shutdown(); return false; }

        // Phase groups: one per tier + inert (group 0)
        m_inertPhase = 0;
        m_tierPhases.clear();
        for (AZ::u32 t = 0; t < maxTiers; ++t)
        {
            physx::PxU32 phase = m_particleSystem->createPhase(
                m_material,
                physx::PxParticlePhaseFlags(
                    physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            m_tierPhases.push_back(phase);
        }

        // Create particle buffer
        m_particleBuffer = physics->createParticleBuffer(bufferCapacity, 1, cuda);
        if (!m_particleBuffer) { Shutdown(); return false; }

        // Park all particles initially
        {
            physx::PxScopedCudaLock lock(*cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(bufferCapacity);
            physx::PxVec4* hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(bufferCapacity);
            physx::PxU32* hostPhase = cuda->allocPinnedHostBuffer<physx::PxU32>(bufferCapacity);

            for (AZ::u32 i = 0; i < bufferCapacity; ++i)
            {
                hostPos[i] = physx::PxVec4(0.0f, -100.0f, 0.0f, 0.0f);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = m_inertPhase;
            }

            cuda->copyHToD(devPos, hostPos, bufferCapacity);
            cuda->copyHToD(devVel, hostVel, bufferCapacity);
            cuda->copyHToD(devPhase, hostPhase, bufferCapacity);

            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
            cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->setNbActiveParticles(0);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        m_particleSystem->addParticleBuffer(m_particleBuffer);

        m_vocabParticleCount = 0;
        m_maxStreamSlots = 0;
        m_currentWordLength = 0;

        return true;
    }

    AZ::u32 Workspace::LoadVocabPack(const VocabPack& pack, AZ::u32 wordLength)
    {
        fprintf(stderr, "[WS] LoadVocabPack: len=%u vocabParticles=%u bufCap=%u\n",
            wordLength, pack.totalVocabParticles, m_bufferCapacity);
        fflush(stderr);
        if (!m_particleBuffer || !m_cuda) return 0;
        if (pack.totalVocabParticles == 0 || pack.totalVocabParticles > m_bufferCapacity) return 0;

        m_vocabParticleCount = pack.totalVocabParticles;
        m_currentWordLength = wordLength;

        // Compute stream capacity from remaining buffer
        AZ::u32 remainingCapacity = m_bufferCapacity - m_vocabParticleCount;
        m_maxStreamSlots = remainingCapacity / wordLength;
        if (m_maxStreamSlots < 1) m_maxStreamSlots = 1;

        fprintf(stderr, "[WS] LoadVocabPack: maxStreamSlots=%u (remainingCap=%u)\n",
            m_maxStreamSlots, remainingCapacity);
        fflush(stderr);

        // Write vocab into buffer
        {
            physx::PxScopedCudaLock lock(*m_cuda);

            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_vocabParticleCount);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_vocabParticleCount);
            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(m_vocabParticleCount);

            // Copy from pre-built pack arrays
            for (AZ::u32 i = 0; i < m_vocabParticleCount; ++i)
            {
                AZ::u32 base = i * 4;
                hostPos[i] = physx::PxVec4(
                    pack.positions[base + 0],
                    pack.positions[base + 1],
                    pack.positions[base + 2],
                    pack.positions[base + 3]);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                // Remap logical tier index to actual phase group ID
                AZ::u32 logicalTier = pack.phases[i];
                hostPhase[i] = (logicalTier < m_tierPhases.size())
                    ? m_tierPhases[logicalTier]
                    : m_inertPhase;
            }

            m_cuda->copyHToD(devPos, hostPos, m_vocabParticleCount);
            m_cuda->copyHToD(devVel, hostVel, m_vocabParticleCount);
            m_cuda->copyHToD(devPhase, hostPhase, m_vocabParticleCount);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }
        fprintf(stderr, "[WS] LoadVocabPack: CUDA copy done\n"); fflush(stderr);

        // Only vocab active, no stream yet
        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

        m_streamSlots.clear();

        fprintf(stderr, "[WS] LoadVocabPack: done, maxStreamSlots=%u\n", m_maxStreamSlots);
        fflush(stderr);
        return m_maxStreamSlots;
    }

    AZ::u32 Workspace::LoadStreamRuns(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& indices,
        AZ::u32 wordLength)
    {
        fprintf(stderr, "[WS] LoadStreamRuns: %zu indices, wordLen=%u, vocabCount=%u\n",
            indices.size(), wordLength, m_vocabParticleCount);
        fflush(stderr);
        if (!m_particleBuffer || !m_cuda || indices.empty()) return 0;

        m_streamSlots.clear();
        AZ::u32 overflowCount = 0;
        AZ::u32 streamPhase = m_tierPhases.empty() ? m_inertPhase : m_tierPhases[0];
        AZ::u32 maxDynParticles = m_maxStreamSlots * wordLength;
        fprintf(stderr, "[WS] LoadStreamRuns: maxDynParticles=%u (slots=%u x len=%u)\n",
            maxDynParticles, m_maxStreamSlots, wordLength);
        fflush(stderr);

        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        physx::PxU32* hostPhase = nullptr;

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(maxDynParticles);
            hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(maxDynParticles);
            hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(maxDynParticles);
        }

        // Init dynamic region to parked state
        for (AZ::u32 i = 0; i < maxDynParticles; ++i)
        {
            hostPos[i] = physx::PxVec4(0.0f, -100.0f, 0.0f, 0.0f);
            hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
            hostPhase[i] = m_inertPhase;
        }

        AZ::u32 slotIdx = 0;
        for (AZ::u32 ri = 0; ri < static_cast<AZ::u32>(indices.size()); ++ri)
        {
            if (slotIdx >= m_maxStreamSlots)
            {
                overflowCount = static_cast<AZ::u32>(indices.size()) - ri;
                break;
            }

            AZ::u32 runIdx = indices[ri];
            const CharRun& run = runs[runIdx];
            AZ::u32 charCount = wordLength;

            AZ::u32 dynOffset = slotIdx * wordLength;

            StreamRunSlot ss;
            ss.runIndex = runIdx;
            ss.bufferStart = m_vocabParticleCount + dynOffset;
            ss.charCount = charCount;
            ss.runText = run.text;
            ss.resolved = false;
            ss.firstCap = run.firstCap;
            ss.allCaps = run.allCaps;

            for (AZ::u32 c = 0; c < charCount; ++c)
            {
                char ch = (c < run.text.size()) ? run.text[c] : '\0';
                float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                hostPos[dynOffset + c] = physx::PxVec4(
                    static_cast<float>(c), RC_Y_OFFSET, z, 1.0f);
                hostVel[dynOffset + c] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[dynOffset + c] = streamPhase;
            }

            m_streamSlots.push_back(ss);
            ++slotIdx;
        }

        // Upload dynamic region
        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            m_cuda->copyHToD(devPos + m_vocabParticleCount, hostPos, maxDynParticles);
            m_cuda->copyHToD(devVel + m_vocabParticleCount, hostVel, maxDynParticles);
            m_cuda->copyHToD(devPhase + m_vocabParticleCount, hostPhase, maxDynParticles);

            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }
        fprintf(stderr, "[WS] LoadStreamRuns: CUDA copy done (offset=%u+%u)\n",
            m_vocabParticleCount, maxDynParticles);
        fflush(stderr);

        // Only activate the particles we actually loaded, not the full stream capacity.
        // slotIdx = number of runs loaded; each run = wordLength particles.
        AZ::u32 actualDynParticles = slotIdx * wordLength;
        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount + actualDynParticles);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

        fprintf(stderr, "[WS] LoadStreamRuns: done, activeParticles=%u (vocab=%u+dyn=%u), overflow=%u\n",
            m_vocabParticleCount + actualDynParticles, m_vocabParticleCount, actualDynParticles, overflowCount);
        fflush(stderr);
        return overflowCount;
    }

    void Workspace::CheckSettlement(AZ::u32 tierIndex, const VocabPack& pack)
    {
        if (!m_particleBuffer || !m_cuda || m_streamSlots.empty()) return;

        // Only read back actually-active particles (vocab + loaded stream runs)
        AZ::u32 readbackCount = m_vocabParticleCount +
            static_cast<AZ::u32>(m_streamSlots.size()) * m_currentWordLength;
        if (readbackCount > m_bufferCapacity) readbackCount = m_bufferCapacity;

        physx::PxVec4* hostVel = nullptr;
        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(readbackCount);
            m_cuda->copyDToH(hostVel, devVel, readbackCount);
        }

        // Get tier lookup (O(1) hash)
        const AZStd::unordered_map<AZStd::string, AZ::u32>* lookup = nullptr;
        if (tierIndex < pack.tierLookup.size())
            lookup = &pack.tierLookup[tierIndex];

        for (auto& slot : m_streamSlots)
        {
            if (slot.resolved) continue;

            AZ::u32 settledCount = 0;
            for (AZ::u32 c = 0; c < slot.charCount; ++c)
            {
                AZ::u32 idx = slot.bufferStart + c;
                if (idx >= readbackCount) break;
                float vMag = fabsf(hostVel[idx].x) + fabsf(hostVel[idx].y) + fabsf(hostVel[idx].z);
                if (vMag < WS_VELOCITY_SETTLE_THRESHOLD)
                    ++settledCount;
            }

            if (settledCount == slot.charCount && lookup)
            {
                auto it = lookup->find(slot.runText);
                if (it != lookup->end())
                {
                    AZ::u32 entryIdx = it->second;
                    slot.resolved = true;
                    slot.tierResolved = tierIndex;
                    slot.matchedWord = pack.entries[entryIdx].word;
                    slot.matchedTokenId = pack.entries[entryIdx].tokenId;
                    slot.morphBits = pack.entries[entryIdx].morphBits;
                }
            }
        }

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            m_cuda->freePinnedHostBuffer(hostVel);
        }
    }

    // ---- LoadRulePack: load partial-match patterns into vocab region ----
    AZ::u32 Workspace::LoadRulePack(const RulePack& pack)
    {
        if (!m_particleBuffer || !m_cuda) return 0;
        if (pack.totalPatternParticles == 0 || pack.totalPatternParticles > m_bufferCapacity) return 0;

        m_vocabParticleCount = pack.totalPatternParticles;
        m_currentWordLength = pack.cellLength;

        AZ::u32 remainingCapacity = m_bufferCapacity - m_vocabParticleCount;
        m_maxStreamSlots = remainingCapacity / pack.cellLength;
        if (m_maxStreamSlots < 1) m_maxStreamSlots = 1;

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();
            physx::PxU32* devPhase = m_particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_vocabParticleCount);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(m_vocabParticleCount);
            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(m_vocabParticleCount);

            for (AZ::u32 i = 0; i < m_vocabParticleCount; ++i)
            {
                AZ::u32 base = i * 4;
                hostPos[i] = physx::PxVec4(pack.positions[base], pack.positions[base+1],
                                            pack.positions[base+2], pack.positions[base+3]);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = m_tierPhases.empty() ? m_inertPhase : m_tierPhases[0];
            }

            m_cuda->copyHToD(devPos, hostPos, m_vocabParticleCount);
            m_cuda->copyHToD(devVel, hostVel, m_vocabParticleCount);
            m_cuda->copyHToD(devPhase, hostPhase, m_vocabParticleCount);
            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
            m_cuda->freePinnedHostBuffer(hostPhase);
        }

        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        m_streamSlots.clear();
        return m_maxStreamSlots;
    }

    // ---- CheckPartialSettlement: only non-\0 pattern positions must settle ----
    void Workspace::CheckPartialSettlement(const RulePack& pack,
        AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>>& matches)
    {
        if (!m_particleBuffer || !m_cuda || m_streamSlots.empty()) return;

        AZ::u32 readbackCount = m_vocabParticleCount +
            static_cast<AZ::u32>(m_streamSlots.size()) * m_currentWordLength;
        if (readbackCount > m_bufferCapacity) readbackCount = m_bufferCapacity;

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

        for (AZ::u32 si = 0; si < static_cast<AZ::u32>(m_streamSlots.size()); ++si)
        {
            auto& slot = m_streamSlots[si];
            if (slot.resolved) continue;

            for (AZ::u32 pi = 0; pi < pack.patternCount; ++pi)
            {
                AZ::u32 activeCount = pack.activePositionCount[pi];
                if (activeCount == 0) continue;

                AZ::u32 activeSettled = 0;
                for (AZ::u32 c = 0; c < pack.cellLength; ++c)
                {
                    float patZ = pack.positions[(pi * pack.cellLength + c) * 4 + 2];
                    if (patZ == 0.0f) continue;  // Inert position

                    AZ::u32 streamIdx = slot.bufferStart + c;
                    if (streamIdx >= readbackCount) break;

                    float vMag = fabsf(hostVel[streamIdx].x) + fabsf(hostVel[streamIdx].y)
                               + fabsf(hostVel[streamIdx].z);
                    if (vMag < WS_VELOCITY_SETTLE_THRESHOLD)
                    {
                        float streamZ = hostPos[streamIdx].z;
                        if (fabsf(streamZ - patZ) < RC_Z_SCALE * 0.5f)
                            ++activeSettled;
                    }
                }

                if (activeSettled == activeCount)
                {
                    matches.push_back({si, pi});
                    slot.resolved = true;
                    break;
                }
            }
        }

        {
            physx::PxScopedCudaLock lock(*m_cuda);
            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
        }
    }

    void Workspace::FlipToTier(AZ::u32 nextTier)
    {
        if (!m_particleBuffer || !m_cuda) return;
        if (nextTier >= m_tierPhases.size()) return;

        AZ::u32 newPhase = m_tierPhases[nextTier];
        AZ::u32 dynCount = m_maxStreamSlots * m_currentWordLength;

        {
            physx::PxScopedCudaLock lock(*m_cuda);

            physx::PxU32* devPhase = m_particleBuffer->getPhases();
            physx::PxVec4* devPos = m_particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = m_particleBuffer->getVelocities();

            physx::PxU32* hostPhase = m_cuda->allocPinnedHostBuffer<physx::PxU32>(dynCount);
            physx::PxVec4* hostPos = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(dynCount);
            physx::PxVec4* hostVel = m_cuda->allocPinnedHostBuffer<physx::PxVec4>(dynCount);

            m_cuda->copyDToH(hostPhase, devPhase + m_vocabParticleCount, dynCount);
            m_cuda->copyDToH(hostPos, devPos + m_vocabParticleCount, dynCount);
            m_cuda->copyDToH(hostVel, devVel + m_vocabParticleCount, dynCount);

            for (const auto& slot : m_streamSlots)
            {
                AZ::u32 dynBase = slot.bufferStart - m_vocabParticleCount;

                if (slot.resolved)
                {
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                        hostPhase[dynBase + c] = m_inertPhase;
                }
                else
                {
                    for (AZ::u32 c = 0; c < slot.charCount; ++c)
                    {
                        hostPhase[dynBase + c] = newPhase;
                        hostPos[dynBase + c].y = RC_Y_OFFSET;
                        hostPos[dynBase + c].w = 1.0f;
                        hostVel[dynBase + c] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
            }

            m_cuda->copyHToD(devPhase + m_vocabParticleCount, hostPhase, dynCount);
            m_cuda->copyHToD(devPos + m_vocabParticleCount, hostPos, dynCount);
            m_cuda->copyHToD(devVel + m_vocabParticleCount, hostVel, dynCount);

            m_cuda->freePinnedHostBuffer(hostPhase);
            m_cuda->freePinnedHostBuffer(hostPos);
            m_cuda->freePinnedHostBuffer(hostVel);
        }

        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
    }

    void Workspace::CollectResults(AZStd::vector<ResolutionResult>& out)
    {
        for (const auto& slot : m_streamSlots)
        {
            ResolutionResult r;
            r.runText = slot.runText;
            r.matchedWord = slot.matchedWord;
            r.matchedTokenId = slot.matchedTokenId;
            r.tierResolved = slot.tierResolved;
            r.resolved = slot.resolved;
            r.runIndex = slot.runIndex;
            r.firstCap = slot.firstCap;
            r.allCaps = slot.allCaps;
            r.morphBits = slot.morphBits;
            out.push_back(r);
        }
    }

    bool Workspace::HasUnresolved() const
    {
        for (const auto& slot : m_streamSlots)
        {
            if (!slot.resolved) return true;
        }
        return false;
    }

    void Workspace::CollectSplit(
        AZStd::vector<ResolutionResult>& resolved,
        AZStd::vector<AZ::u32>& unresolvedRunIndices)
    {
        for (const auto& slot : m_streamSlots)
        {
            if (slot.resolved)
            {
                ResolutionResult r;
                r.runText = slot.runText;
                r.matchedWord = slot.matchedWord;
                r.matchedTokenId = slot.matchedTokenId;
                r.tierResolved = slot.tierResolved;
                r.resolved = true;
                r.runIndex = slot.runIndex;
                r.firstCap = slot.firstCap;
                r.allCaps = slot.allCaps;
                r.morphBits = slot.morphBits;
                resolved.push_back(r);
            }
            else
            {
                unresolvedRunIndices.push_back(slot.runIndex);
            }
        }
    }

    void Workspace::ResetDynamics()
    {
        if (!m_particleBuffer) return;
        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount);
        m_streamSlots.clear();
    }

    void Workspace::ActivateInScene()
    {
        fprintf(stderr, "[WS] ActivateInScene: activeInScene=%d\n", (int)m_activeInScene);
        fflush(stderr);
        if (m_activeInScene || !m_particleSystem || !m_scene) return;
        m_scene->addActor(*m_particleSystem);
        m_activeInScene = true;
        fprintf(stderr, "[WS] ActivateInScene: addActor done\n"); fflush(stderr);
    }

    void Workspace::DeactivateFromScene()
    {
        fprintf(stderr, "[WS] DeactivateFromScene: activeInScene=%d\n", (int)m_activeInScene);
        fflush(stderr);
        if (!m_activeInScene || !m_particleSystem || !m_scene) return;
        m_scene->removeActor(*m_particleSystem);
        m_activeInScene = false;
        fprintf(stderr, "[WS] DeactivateFromScene: removeActor done\n"); fflush(stderr);
    }

    void Workspace::BeginSimulate(int steps, float dt)
    {
        fprintf(stderr, "[WS] BeginSimulate: steps=%d dt=%.4f activeParticles=%u\n",
            steps, dt, m_vocabParticleCount);
        fflush(stderr);
        if (!m_scene) return;
        m_pendingSteps = steps;
        m_simDt = dt;

        // Kick off the first step — simulate() dispatches to GPU and returns
        if (m_pendingSteps > 0)
        {
            fprintf(stderr, "[WS] BeginSimulate: calling simulate()\n"); fflush(stderr);
            m_scene->simulate(m_simDt);
            fprintf(stderr, "[WS] BeginSimulate: simulate() returned\n"); fflush(stderr);
            --m_pendingSteps;
        }
    }

    bool Workspace::IsSimDone() const
    {
        if (!m_scene) return true;
        // Check if the in-flight GPU step (step 0) is done.
        // Remaining steps (m_pendingSteps) are run synchronously inside FetchSimResults.
        // The old `m_pendingSteps > 0` guard caused a deadlock: BeginSimulate dispatches
        // step 0 and leaves pendingSteps=59, so IsSimDone always returned false and
        // FetchSimResults was never called.
        return m_scene->checkResults(false);
    }

    void Workspace::FetchSimResults()
    {
        fprintf(stderr, "[WS] FetchSimResults: start, pendingSteps=%d\n", m_pendingSteps);
        fflush(stderr);
        if (!m_scene) return;

        // Complete the in-flight step
        fprintf(stderr, "[WS] FetchSimResults: fetchResults(step0)\n"); fflush(stderr);
        m_scene->fetchResults(true);
        fprintf(stderr, "[WS] FetchSimResults: step0 done, %d steps remain\n", m_pendingSteps);
        fflush(stderr);

        // Run remaining steps synchronously (each step must complete before next)
        while (m_pendingSteps > 0)
        {
            m_scene->simulate(m_simDt);
            m_scene->fetchResults(true);
            --m_pendingSteps;
        }

        fprintf(stderr, "[WS] FetchSimResults: all steps done, calling fetchResultsParticleSystem\n");
        fflush(stderr);
        m_scene->fetchResultsParticleSystem();
        fprintf(stderr, "[WS] FetchSimResults: done\n"); fflush(stderr);
    }

    void Workspace::Shutdown()
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
            if (m_activeInScene)
                m_scene->removeActor(*m_particleSystem);
            m_particleSystem->release();
            m_particleSystem = nullptr;
            m_activeInScene = false;
        }

        if (m_ownsScene && m_scene)
        {
            m_scene->release();
            m_scene = nullptr;
            m_ownsScene = false;
        }

        m_streamSlots.clear();
        m_tierPhases.clear();
        m_vocabParticleCount = 0;
        m_maxStreamSlots = 0;
        m_currentWordLength = 0;
        m_bufferCapacity = 0;
        m_maxTierCount = 0;
        m_pendingSteps = 0;
        m_simDt = 0.0f;
    }

    // ========================================================================
    // BedManager — LMDB-backed workspace pool + phased vocab resolution
    //
    // Reads pre-compiled vocab beds from LMDB (data/vocab.lmdb/).
    // Entries are frequency-ordered at compile time: Labels first, then freq-ranked,
    // then unranked. No Postgres at runtime, no sorting, no tier assignment.
    //
    // Each phase loads a vocab slice (sized dynamically from stream run count) as static
    // particles, leaving maximum buffer space for stream runs. Phases cycle
    // until all runs resolve or vocab is exhausted. Early exit on full resolution.
    // ========================================================================


    AZStd::vector<Workspace*> BedManager::GetWorkspacesForLength(AZ::u32 wordLength)
    {
        AZStd::vector<Workspace*> result;
        if (wordLength <= WS_PRIMARY_MAX_LENGTH)
        {
            for (auto& ws : m_primaryWorkspaces)
                result.push_back(&ws);
        }
        else
        {
            for (auto& ws : m_extendedWorkspaces)
                result.push_back(&ws);
        }
        return result;
    }

    // ---- LMDB vocab bed format ----
    // Sub-db "vbed_XX" (XX=02..16): single key "data" → packed entry buffer
    // Sub-db "vbed_XX_meta": single key "meta" → VBedMeta struct (16 bytes)
    // Entry format: word[wordLength] + tokenId[14], fixed-width per sub-db
    // Order: Labels first, then freq-ranked non-labels, then unranked

    static constexpr int VBED_MIN_LEN = 2;
    static constexpr int VBED_MAX_LEN = 16;
    static constexpr int VBED_TOKEN_ID_WIDTH = 14;

    struct VBedMeta
    {
        uint32_t total_entries;
        uint32_t label_count;     // Tier 0 boundary (Labels only)
        uint32_t tier1_end;       // End of freq-ranked non-labels
        uint32_t tier2_end;       // End of all entries (= total_entries)
    };

    // ========================================================================
    // Inflectional suffix stripping — host-side, before PBD
    //
    // Priority-ordered rules (longest suffix first). Each rule produces a
    // candidate base word + morph bits. No vocabulary lookup — the base gets
    // injected into the PBD queue at its length, and PBD against the vocab
    // bed IS the existence check. First matching rule wins.
    // ========================================================================

    struct InflectionStripResult
    {
        AZStd::string baseWord;
        AZ::u16 morphBits = 0;
        bool stripped = false;
    };

    static bool IsConsonant(char c)
    {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return c >= 'a' && c <= 'z' &&
            c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u';
    }

    // ---- CollectSingleStrip ------------------------------------------------
    // Applies inflection rules to `word` (no recursion).
    // If `rules` + `compiled` are provided (non-null, non-empty), uses DB rules.
    // Appends one InflectionStripResult per matched candidate to `results`.
    // `inheritedBits` are OR'd into every candidate's morphBits (for compounds).
    // `seen` deduplicates base words across the full two-level collection.
    // -----------------------------------------------------------------------
    static void CollectSingleStrip(
        const AZStd::string& word,
        AZ::u16 inheritedBits,
        AZStd::vector<InflectionStripResult>& results,
        AZStd::unordered_set<AZStd::string>& seen,
        const HCPVocabulary* vocab,
        const AZStd::vector<InflectionRule>* rules = nullptr,
        const std::vector<std::regex>* compiled = nullptr)
    {
        const size_t len = word.size();
        if (len < 3) return;

        auto appendBase = [&](const AZStd::string& base, AZ::u16 bits)
        {
            if (base.size() < 2 || seen.count(base)) return;
            // Validate base exists in LMDB before injecting as a synthetic.
            if (vocab && vocab->LookupWordLocal(base).empty()) return;
            seen.insert(base);
            results.push_back({base, static_cast<AZ::u16>(inheritedBits | bits), true});
        };

        // ---- DB rules path ----
        if (rules && compiled && !rules->empty())
        {
            const std::string wordStd(word.c_str(), word.size());
            for (size_t ri = 0; ri < rules->size(); ++ri)
            {
                const InflectionRule& r   = (*rules)[ri];
                const std::regex&    cond = (*compiled)[ri];
                const std::string&   add  = r.addSuffix.c_str();
                const bool doubling       = (r.stripSuffix == "__DOUBLING__");

                // Word must end with the inflected suffix
                if (add.empty() || len <= add.size()) continue;
                if (wordStd.substr(len - add.size()) != add) continue;

                // Compute candidate base by reversing the transformation
                std::string candidate = wordStd.substr(0, len - add.size());

                if (doubling)
                {
                    // CVC doubling: candidate ends with doubled consonant → strip one
                    size_t clen = candidate.size();
                    if (clen < 2 || candidate[clen-1] != candidate[clen-2]
                        || !IsConsonant(static_cast<char>(candidate[clen-1])))
                        continue;
                    candidate.pop_back();
                }
                else if (!r.stripSuffix.empty())
                {
                    // Restore the suffix that was removed from the base before inflecting
                    candidate += r.stripSuffix.c_str();
                }

                // Check condition regex against candidate base
                if (!std::regex_search(candidate, cond)) continue;

                appendBase(AZStd::string(candidate.c_str(), candidate.size()), r.morphBit);
            }
            return;
        }

        // ---- Hardcoded fallback (no rules loaded yet) ----

        // ---- -ies → -y ----
        if (len >= 4 && word.substr(len - 3) == "ies")
            appendBase(word.substr(0, len - 3) + "y", MorphBit::PLURAL | MorphBit::THIRD);

        // ---- -ied → -y ----
        if (len >= 4 && word.substr(len - 3) == "ied")
            appendBase(word.substr(0, len - 3) + "y", MorphBit::PAST);

        // ---- Doubled consonant + -ing ----
        if (len >= 6 && word.substr(len - 3) == "ing")
        {
            AZStd::string stem = word.substr(0, len - 3);
            if (stem.size() >= 3 && stem[stem.size()-1] == stem[stem.size()-2] && IsConsonant(stem.back()))
                appendBase(stem.substr(0, stem.size() - 1), MorphBit::PROG);
        }

        // ---- Doubled consonant + -ed ----
        if (len >= 5 && word.substr(len - 2) == "ed")
        {
            AZStd::string stem = word.substr(0, len - 2);
            if (stem.size() >= 3 && stem[stem.size()-1] == stem[stem.size()-2] && IsConsonant(stem.back()))
                appendBase(stem.substr(0, stem.size() - 1), MorphBit::PAST);
        }

        // ---- -ing (plain + silent-e) ----
        if (len >= 5 && word.substr(len - 3) == "ing")
        {
            appendBase(word.substr(0, len - 3),        MorphBit::PROG);
            appendBase(word.substr(0, len - 3) + "e",  MorphBit::PROG);
        }

        // ---- -ed (plain + silent-e) ----
        if (len >= 4 && word.substr(len - 2) == "ed")
        {
            appendBase(word.substr(0, len - 2),        MorphBit::PAST);
            appendBase(word.substr(0, len - 2) + "e",  MorphBit::PAST);
        }

        // ---- -er (doubled-consonant + plain + silent-e) ----
        if (len >= 4 && word.substr(len - 2) == "er")
        {
            AZStd::string base = word.substr(0, len - 2);
            if (base.size() >= 3 && base[base.size()-1] == base[base.size()-2] && IsConsonant(base.back()))
                appendBase(base.substr(0, base.size() - 1), 0);
            appendBase(base,       0);
            appendBase(base + "e", 0);
        }

        // ---- -est (doubled-consonant + plain + silent-e) ----
        if (len >= 5 && word.substr(len - 3) == "est")
        {
            AZStd::string base = word.substr(0, len - 3);
            if (base.size() >= 3 && base[base.size()-1] == base[base.size()-2] && IsConsonant(base.back()))
                appendBase(base.substr(0, base.size() - 1), 0);
            appendBase(base,       0);
            appendBase(base + "e", 0);
        }

        // ---- -es ----
        if (len >= 4 && word.substr(len - 2) == "es")
        {
            appendBase(word.substr(0, len - 2), MorphBit::PLURAL | MorphBit::THIRD);
            appendBase(word.substr(0, len - 1), MorphBit::PLURAL | MorphBit::THIRD);
        }

        // ---- -s ----
        if (len >= 4 && word.back() == 's' && word[len-2] != 's')
            appendBase(word.substr(0, len - 1), MorphBit::PLURAL | MorphBit::THIRD);

        // ---- -ily → -y ----
        if (len >= 5 && word.substr(len - 3) == "ily")
            appendBase(word.substr(0, len - 3) + "y", 0);

        // ---- -ly ----
        if (len >= 5 && word.substr(len - 2) == "ly")
            appendBase(word.substr(0, len - 2), 0);

        // ---- -ness ----
        if (len >= 6 && word.substr(len - 4) == "ness")
            appendBase(word.substr(0, len - 4), 0);
    }

    // ---- TryInflectionStrip ------------------------------------------------
    // Returns ordered candidates for `word`:
    //   Level 1 — all direct single-morpheme strips (priority order).
    //   Level 2 — compound strips of each level-1 base (appended after all
    //             level-1 entries so direct candidates are tried first).
    //
    // PBD against vocab beds is the existence check; no LMDB pre-screening.
    // The caller drives sequential fallback: try candidates[0], if PBD fails
    // try candidates[1], etc.  Level-2 entries handle cases like:
    //   "gardeners" → level1: "gardener" (-s) → level2: "garden" (-er)
    //   "runnings"  → level1: "running"  (-s) → level2: "run"    (-ing)
    // -----------------------------------------------------------------------
    static AZStd::vector<InflectionStripResult> TryInflectionStrip(
        const AZStd::string& word,
        const HCPVocabulary* vocab = nullptr,
        const AZStd::vector<InflectionRule>* rules = nullptr,
        const std::vector<std::regex>* compiled = nullptr)
    {
        AZStd::vector<InflectionStripResult> results;
        AZStd::unordered_set<AZStd::string> seen;

        // Level 1: direct single-morpheme strips
        CollectSingleStrip(word, 0, results, seen, vocab, rules, compiled);

        // Level 2: compound strips — checked after ALL level-1 candidates so a
        // compound fallback never beats a later direct candidate in priority order.
        // IMPORTANT: copy baseWord and morphBits by value before calling CollectSingleStrip.
        // CollectSingleStrip may push_back into `results`, which can reallocate the vector
        // and invalidate any reference into it — including results[i].baseWord itself.
        const size_t level1Count = results.size();
        for (size_t i = 0; i < level1Count; ++i)
        {
            AZStd::string baseCopy = results[i].baseWord;
            AZ::u16 bitsCopy = results[i].morphBits;
            CollectSingleStrip(baseCopy, bitsCopy, results, seen, vocab, rules, compiled);
        }

        return results;
    }

    // ========================================================================
    // Variant surface form normalization — host-side, Stage 2/4
    //
    // Runs AFTER TryInflectionStrip (only on runs still unresolved).
    // Rules implemented:
    //   V-1: -in'  (g-drop dialect)    → stem+"ing"  → VARIANT|VARIANT_DIALECT
    //   V-3: -eth  (archaic 3rd person) → base/base+e → VARIANT|VARIANT_ARCHAIC|THIRD
    //
    // Returns primary normalized form + optional alt form (base+e for -eth).
    // PBD is the existence guard — no pre-check needed.
    // ========================================================================

    struct VariantNormalResult
    {
        bool normalized = false;
        AZStd::string normalizedForm;   // Primary candidate (inject for direct PBD)
        AZStd::string altForm;          // Alt candidate (base+e for -eth), empty if none
        AZ::u16 variantBits = 0;        // VARIANT + subtype bits (no inflection bits)
    };

    static VariantNormalResult TryVariantNormalize(const AZStd::string& word)
    {
        VariantNormalResult result;
        const size_t len = word.size();

        // V-1: -in' g-drop (dialect)
        // Pattern: word ends with "in'" (min 4 chars: at least 1 stem char + "in'")
        // Transform: remove trailing ', append g  →  "darlin'" → "darling"
        if (len >= 4 && word[len-1] == '\'' && word[len-2] == 'n' && word[len-3] == 'i')
        {
            result.normalized = true;
            result.normalizedForm = word.substr(0, len - 1) + "g";  // strip ', add g
            result.variantBits = MorphBit::VARIANT | MorphBit::VARIANT_DIALECT;
            return result;
        }

        // V-3: -eth archaic 3rd person (min length 5: avoids "Seth", "Beth" as names)
        // Pattern: word ends with "eth"
        // Transform: strip -eth → base; also try base+e (silent-e fallback)
        // Examples: walketh→walk, maketh→mak/make, goeth→go
        if (len >= 5 && word[len-3] == 'e' && word[len-2] == 't' && word[len-1] == 'h')
        {
            AZStd::string base = word.substr(0, len - 3);
            if (base.size() >= 2)
            {
                result.normalized = true;
                result.normalizedForm = base;
                result.altForm = base + "e";   // silent-e fallback
                result.variantBits = MorphBit::VARIANT | MorphBit::VARIANT_ARCHAIC | MorphBit::THIRD;
                return result;
            }
        }

        return result;
    }

    // ---- DetectSignals: scan short-pass manifest for tense/register signals ----

    ShortPassSignal DetectSignals(const ResolutionManifest& manifest)
    {
        static const AZStd::unordered_set<AZStd::string> kPast   = {
            "was","were","had","did","got","went"};
        static const AZStd::unordered_set<AZStd::string> kFuture = {
            "will","shall","would","could","might"};
        static const AZStd::unordered_set<AZStd::string> kPresent = {
            "is","are","am","has","does"};
        static const AZStd::unordered_set<AZStd::string> kArchaic = {
            "hath","doth","thou","thee","hast","dost","wilt","wast"};

        ShortPassSignal sig;
        for (const auto& r : manifest.results)
        {
            if (!r.resolved) continue;
            ++sig.resolvedCount;
            if (kPast.count(r.runText))    sig.hasPast    = true;
            if (kFuture.count(r.runText))  sig.hasFuture  = true;
            if (kPresent.count(r.runText)) sig.hasPresent = true;
            if (kArchaic.count(r.runText)) sig.hasArchaic = true;
        }
        return sig;
    }

    bool BedManager::Initialize(
        physx::PxPhysics* physics,
        physx::PxCudaContextManager* cuda,
        MDB_env* lmdbEnv,
        HCPVocabulary* vocabulary,
        HCPEnvelopeManager* envelopeManager)
    {
        if (!physics || !cuda || !lmdbEnv) return false;

        m_physics = physics;
        m_cuda = cuda;
        m_vocabulary = vocabulary;
        m_envelopeManager = envelopeManager;
        m_lmdbEnv = lmdbEnv;

        auto t0 = std::chrono::high_resolution_clock::now();

        // Open w2t sub-db — populated by EnvelopeManager::ActivateEnvelope.
        // MDB_CREATE ensures the handle is valid even if the envelope hasn't fired yet;
        // RebuildVocab() will produce an empty index until it is populated.
        {
            MDB_txn* txn;
            int rc = mdb_txn_begin(lmdbEnv, nullptr, 0, &txn);
            if (rc != 0)
            {
                fprintf(stderr, "[BedManager] mdb_txn_begin: %s\n", mdb_strerror(rc));
                return false;
            }
            rc = mdb_dbi_open(txn, "w2t", MDB_CREATE, &m_vocabDbi);
            if (rc == 0)
                m_vocabDbiOpen = true;
            else
                fprintf(stderr, "[BedManager] w2t open: %s\n", mdb_strerror(rc));
            mdb_txn_commit(txn);
        }

        // Build in-memory vocab index from whatever is in w2t right now.
        RebuildVocab();

        // Sort lengths ascending (resolve shortest words first — function words establish context)
        AZStd::sort(m_activeWordLengths.begin(), m_activeWordLengths.end(),
            [](AZ::u32 a, AZ::u32 b) { return a < b; });

        // Create workspace pools — each workspace gets its own PxScene
        // for pipelined GPU/CPU overlap (simulate A while reading B, loading C)
        AZ::u32 maxPhaseGroups = 2;  // 1 active phase + inert
        m_primaryWorkspaces.resize(WS_PRIMARY_COUNT);
        m_extendedWorkspaces.resize(WS_EXTENDED_COUNT);

        AZ::u32 createdCount = 0;
        for (auto& ws : m_primaryWorkspaces)
        {
            if (ws.Create(physics, cuda, WS_BUFFER_CAPACITY, maxPhaseGroups))
                ++createdCount;
        }
        for (auto& ws : m_extendedWorkspaces)
        {
            if (ws.Create(physics, cuda, WS_BUFFER_CAPACITY, maxPhaseGroups))
                ++createdCount;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        float ms = static_cast<float>(std::chrono::duration<double, std::milli>(t1 - t0).count());

        m_initialized = true;

        AZ::u32 totalEntries = 0;
        for (auto& [len, entries] : m_vocabByLength)
            totalEntries += static_cast<AZ::u32>(entries.size());

        fprintf(stderr, "[BedManager] Initialized: %zu lengths, %u entries in w2t, "
            "phase floor=%u, %u workspaces, %.1f ms\n",
            m_activeWordLengths.size(), totalEntries,
            RC_VOCAB_PHASE_FLOOR, createdCount, ms);
        fflush(stderr);
        fprintf(stderr, "[BedManager] Word lengths (ascending, shortest-first):");
        for (AZ::u32 len : m_activeWordLengths)
            fprintf(stderr, " %u", len);
        fprintf(stderr, "\n");
        fflush(stderr);

        return true;
    }

    // ---- ResolvePhase: load one small vocab slice, simulate, collect ----
    //
    // Dynamic vocab slice (sized per stream run count) → maximum stream slots.
    // All runs that fit get checked in one simulate cycle.

    // Helper: load a workspace with vocab + stream runs, activate, return overflow indices.
    // Returns true if the workspace has pending runs to simulate.
    static bool LoadWorkspaceBatch(
        Workspace* ws,
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& remaining,
        AZ::u32& offset,
        const VocabPack& phasePack,
        AZStd::vector<AZ::u32>& overflow)
    {
        if (offset >= static_cast<AZ::u32>(remaining.size())) return false;

        AZ::u32 streamSlots = ws->LoadVocabPack(phasePack, wordLength);
        if (streamSlots == 0) return false;

        AZ::u32 endIdx = offset + streamSlots;
        if (endIdx > static_cast<AZ::u32>(remaining.size()))
            endIdx = static_cast<AZ::u32>(remaining.size());

        AZStd::vector<AZ::u32> wsIndices(remaining.begin() + offset, remaining.begin() + endIdx);
        AZ::u32 overflowCount = ws->LoadStreamRuns(runs, wsIndices, wordLength);

        offset = endIdx;

        if (overflowCount > 0)
        {
            AZ::u32 loaded = static_cast<AZ::u32>(wsIndices.size()) - overflowCount;
            for (AZ::u32 j = loaded; j < static_cast<AZ::u32>(wsIndices.size()); ++j)
                overflow.push_back(wsIndices[j]);
        }

        if (ws->HasPendingRuns())
        {
            ws->ActivateInScene();
            return true;
        }
        return false;
    }

    // Helper: finish simulation, check settlement, collect results, reset workspace.
    static void DrainWorkspace(
        Workspace* ws,
        const VocabPack& phasePack,
        AZ::u32 phaseIndex,
        AZStd::vector<ResolutionResult>& results,
        AZStd::vector<AZ::u32>& unresolvedIndices)
    {
        ws->FetchSimResults();
        ws->CheckSettlement(0, phasePack);

        AZStd::vector<ResolutionResult> wsResolved;
        AZStd::vector<AZ::u32> wsUnresolved;
        ws->CollectSplit(wsResolved, wsUnresolved);

        for (auto& r : wsResolved)
        {
            r.tierResolved = phaseIndex;
            results.push_back(AZStd::move(r));
        }
        for (AZ::u32 idx : wsUnresolved)
            unresolvedIndices.push_back(idx);

        ws->ResetDynamics();
        ws->DeactivateFromScene();
    }

    void BedManager::ResolvePhase(
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& runIndices,
        const VocabPack& phasePack,
        AZ::u32 phaseIndex,
        AZStd::vector<ResolutionResult>& results,
        AZStd::vector<AZ::u32>& unresolvedIndices)
    {
        if (runIndices.empty() || phasePack.vocabEntryCount == 0) return;

        AZStd::vector<Workspace*> workspaces = GetWorkspacesForLength(wordLength);
        if (workspaces.empty()) return;

        AZStd::vector<AZ::u32> remaining = runIndices;

        while (!remaining.empty())
        {
            AZStd::vector<AZ::u32> nextRemaining;
            AZ::u32 offset = 0;

            // Pipeline: load each workspace, kick off simulate, overlap with next load.
            // Each workspace owns its own PxScene — simulate() dispatches to GPU
            // and returns immediately, so we can load the next workspace on CPU
            // while the previous one's GPU work is in flight.

            AZStd::vector<Workspace*> simulating;

            for (auto* ws : workspaces)
            {
                if (offset >= static_cast<AZ::u32>(remaining.size())) break;

                if (LoadWorkspaceBatch(ws, wordLength, runs, remaining, offset,
                                       phasePack, nextRemaining))
                {
                    // Kick off simulation — GPU works while we load the next workspace
                    ws->BeginSimulate(RC_SETTLE_STEPS, RC_DT);
                    simulating.push_back(ws);
                }
            }

            // Anything that didn't fit into workspaces this round
            for (AZ::u32 j = offset; j < static_cast<AZ::u32>(remaining.size()); ++j)
                nextRemaining.push_back(remaining[j]);

            if (simulating.empty()) break;

            // Drain all simulating workspaces — fetch results as each finishes.
            // With per-workspace scenes, each fetchResults blocks only on its own scene.
            for (auto* ws : simulating)
                DrainWorkspace(ws, phasePack, phaseIndex, results, unresolvedIndices);

            remaining = AZStd::move(nextRemaining);
        }
    }

    // ---- ResolveLengthCycle: slice through freq-ordered vocab for one word length ----
    //
    // Phase 0 = entries [0..N), phase 1 = [N..2N), etc. N = phaseSize (dynamic).
    // Each phase: build tiny VocabPack on the fly, load all remaining runs,
    // simulate, collect resolved, pass unresolved to next phase.
    // Early exit when all runs resolved.

    VocabPack BedManager::BuildVocabSliceFromEntries(
        AZ::u32 wordLength,
        const std::vector<VocabPack::Entry>& entries,
        AZ::u32 startEntry,
        AZ::u32 count) const
    {
        VocabPack pack;
        pack.wordLength = wordLength;
        pack.maxTierCount = 1;

        if (startEntry >= static_cast<AZ::u32>(entries.size())) return pack;

        AZ::u32 endEntry = startEntry + count;
        if (endEntry > static_cast<AZ::u32>(entries.size()))
            endEntry = static_cast<AZ::u32>(entries.size());

        AZ::u32 sliceCount = endEntry - startEntry;
        pack.vocabEntryCount = sliceCount;
        pack.totalVocabParticles = sliceCount * wordLength;

        pack.positions.resize(pack.totalVocabParticles * 4, 0.0f);
        pack.phases.resize(pack.totalVocabParticles, 0);
        pack.entries.reserve(sliceCount);
        pack.tierLookup.resize(1);

        AZ::u32 particleIdx = 0;
        for (AZ::u32 i = startEntry; i < endEntry; ++i)
        {
            const auto& entry = entries[i];
            AZ::u32 entryIdx = i - startEntry;
            pack.entries.push_back(entry);
            pack.tierLookup[0][entry.word] = entryIdx;

            for (AZ::u32 c = 0; c < wordLength; ++c)
            {
                char ch = (c < entry.word.size()) ? entry.word[c] : '\0';
                float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                AZ::u32 base = particleIdx * 4;
                pack.positions[base + 0] = static_cast<float>(c);
                pack.positions[base + 1] = 0.0f;
                pack.positions[base + 2] = z;
                pack.positions[base + 3] = 0.0f;
                pack.phases[particleIdx] = 0;
                ++particleIdx;
            }
        }

        return pack;
    }


    // ---- BuildRulePack: construct partial-match patterns for one cell length ----
    //
    // Suffix rules: right-aligned (front-padded \0). Prefix rules: left-aligned (end-padded \0).
    // Contraction patterns included alongside suffix rules.
    // \0 positions inert (Z=0). CheckPartialSettlement skips them.

    RulePack BedManager::BuildRulePack(AZ::u32 cellLength) const
    {
        RulePack pack;
        pack.cellLength = cellLength;

        // Collect applicable rules for this cell length.
        // Suffix: add_suffix length must be < cellLength (need at least 1 char base).
        // Prefix: strip_prefix length must be < cellLength.
        AZStd::vector<RulePackEntry> entries;

        for (const auto& rule : m_inflectionRules)
        {
            if (rule.addSuffix.empty()) continue;
            AZ::u32 suffLen = static_cast<AZ::u32>(rule.addSuffix.size());
            if (suffLen >= cellLength) continue;  // Suffix must be shorter than word

            RulePackEntry e;
            e.pattern = rule.addSuffix;       // Suffix text on inflected form (match target)
            e.morpheme = rule.morpheme;
            e.morphBits = rule.morphBit;
            e.patternLen = suffLen;
            e.isSuffix = true;
            e.stripSuffix = rule.addSuffix;   // What to strip from inflected form = pattern text
            e.addBase = rule.stripSuffix;      // What to restore on stem to get base (e.g. "y" for -ies)
            entries.push_back(AZStd::move(e));
        }

        for (const auto& rule : m_prefixRules)
        {
            if (rule.stripPrefix.empty()) continue;
            AZ::u32 pfxLen = static_cast<AZ::u32>(rule.stripPrefix.size());
            if (pfxLen >= cellLength) continue;  // Prefix must be shorter than word

            RulePackEntry e;
            e.pattern = rule.stripPrefix;     // Prefix text on surface form (match target)
            e.morpheme = rule.morpheme;
            e.morphBits = 0;  // Prefix morphemes use name, not bit
            e.patternLen = pfxLen;
            e.isSuffix = false;
            e.stripPrefix = rule.stripPrefix; // Same as pattern — the prefix to remove
            entries.push_back(AZStd::move(e));
        }

        // Also add contraction suffix patterns (apostrophe-based)
        // These are compound word splits, not morphemes, but use the same
        // partial-matching machinery. Apostrophe is a valid character with
        // a non-zero Z value, so it participates in matching.
        static const struct { const char* suffix; const char* secondWord; } contractions[] = {
            {"n't", "not"},
            {"'re", "are"},
            {"'ve", "have"},
            {"'ll", "will"},
            {"'s",  nullptr},  // ambiguous: is/has/possessive — handled downstream
            {"'m",  "am"},
            {"'d",  nullptr},  // ambiguous: had/would — handled downstream
        };
        for (const auto& ct : contractions)
        {
            AZ::u32 suffLen = static_cast<AZ::u32>(strlen(ct.suffix));
            if (suffLen >= cellLength) continue;

            RulePackEntry e;
            e.pattern = ct.suffix;
            e.morpheme = "CONTRACTION";
            e.morphBits = 0;
            e.patternLen = suffLen;
            e.isSuffix = true;
            e.stripSuffix = ct.suffix;
            if (ct.secondWord) e.secondWord = ct.secondWord;  // "not", "are", "have", etc.
            entries.push_back(AZStd::move(e));
        }

        if (entries.empty()) return pack;

        // Deduplicate: same suffix text at this length only needs one pattern.
        // Keep first occurrence (suffix rules before prefix, priority order preserved).
        AZStd::unordered_set<AZStd::string> seen;
        AZStd::vector<RulePackEntry> deduped;
        for (auto& e : entries)
        {
            AZStd::string key = e.pattern + (e.isSuffix ? "_S" : "_P");
            if (seen.count(key)) continue;
            seen.insert(key);
            deduped.push_back(AZStd::move(e));
        }

        pack.patternCount = static_cast<AZ::u32>(deduped.size());
        pack.totalPatternParticles = pack.patternCount * cellLength;
        pack.positions.resize(pack.totalPatternParticles * 4, 0.0f);
        pack.phases.resize(pack.totalPatternParticles, 0);
        pack.activePositionCount.resize(pack.patternCount, 0);
        pack.rules = AZStd::move(deduped);

        // Build particle positions
        for (AZ::u32 pi = 0; pi < pack.patternCount; ++pi)
        {
            const auto& rule = pack.rules[pi];
            AZ::u32 activeCount = 0;

            for (AZ::u32 c = 0; c < cellLength; ++c)
            {
                AZ::u32 particleIdx = pi * cellLength + c;
                AZ::u32 base = particleIdx * 4;

                char ch = '\0';
                if (rule.isSuffix)
                {
                    // Right-aligned: pattern chars at end of cell
                    AZ::u32 offset = cellLength - rule.patternLen;
                    if (c >= offset)
                        ch = rule.pattern[c - offset];
                }
                else
                {
                    // Left-aligned: pattern chars at start of cell
                    if (c < rule.patternLen)
                        ch = rule.pattern[c];
                }

                float z = static_cast<float>(static_cast<unsigned char>(ch)) * RC_Z_SCALE;

                pack.positions[base + 0] = static_cast<float>(c);  // X = char position
                pack.positions[base + 1] = 0.0f;                    // Y = 0 (static)
                pack.positions[base + 2] = z;                        // Z = char identity
                pack.positions[base + 3] = 0.0f;                    // W = 0 (invMass=0, static)

                if (ch != '\0') ++activeCount;
            }

            pack.activePositionCount[pi] = activeCount;
        }

        return pack;
    }


    // ---- RunBroadphaseFilter: GPU partial-match broadphase ----
    //
    // Stage 1 of 3: GPU identifies which suffix/prefix patterns match which words.
    // Runs one PBD pass per word length with partial matching (~50 patterns, fast).
    // Returns (runIndex, patternIndex) pairs grouped by pattern.
    // Candidate expansion (Stage 2) and PBD resolution (Stage 3) follow on CPU.

    AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>> BedManager::RunBroadphaseFilter(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& candidateIndices)
    {
        AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>> allMatches;
        if (candidateIndices.empty() || !m_initialized) return allMatches;

        // Group candidates by word length
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> byLength;
        for (AZ::u32 idx : candidateIndices)
        {
            AZ::u32 len = static_cast<AZ::u32>(runs[idx].text.size());
            if (len >= 2 && len <= 20)
                byLength[len].push_back(idx);
        }

        fprintf(stderr, "[BedManager] Broadphase filter: %zu candidates across %zu lengths\n",
            candidateIndices.size(), byLength.size());
        fflush(stderr);

        // Process each word length: GPU partial-match against rule patterns
        for (auto& [len, indices] : byLength)
        {
            RulePack rulePack = BuildRulePack(len);
            if (rulePack.patternCount == 0) continue;

            auto workspaces = GetWorkspacesForLength(len);
            if (workspaces.empty()) continue;
            Workspace* ws = workspaces[0];

            // Process in batches (may overflow workspace capacity)
            AZ::u32 cursor = 0;
            while (cursor < static_cast<AZ::u32>(indices.size()))
            {
                ws->LoadRulePack(rulePack);

                AZ::u32 remaining = static_cast<AZ::u32>(indices.size()) - cursor;
                AZStd::vector<AZ::u32> batchIndices(
                    indices.begin() + cursor,
                    indices.begin() + cursor + remaining);

                AZ::u32 overflow = ws->LoadStreamRuns(runs, batchIndices, len);
                AZ::u32 loaded = remaining - overflow;

                // Simulate — fast pass, small pattern set settles quickly
                ws->ActivateInScene();
                ws->BeginSimulate(RC_SETTLE_STEPS, RC_DT);
                ws->FetchSimResults();

                // Check partial settlement — only non-\0 positions
                AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>> matches;
                ws->CheckPartialSettlement(rulePack, matches);

                ws->DeactivateFromScene();
                ws->ResetDynamics();

                // Translate slot indices to run indices
                for (const auto& [slotIdx, patternIdx] : matches)
                {
                    AZ::u32 runIdx = batchIndices[slotIdx];
                    allMatches.push_back({runIdx, patternIdx});
                }

                cursor += loaded;
                if (overflow == 0) break;
            }
        }

        fprintf(stderr, "[BedManager] Broadphase filter: %zu pattern matches\n", allMatches.size());
        fflush(stderr);

        return allMatches;
    }


    // ---- GenerateStripCandidates: CPU recursive expansion ----
    //
    // Stage 2 of 3: Takes GPU pattern matches, mechanically generates ALL possible
    // base words from each match — then recursively strips each base against all
    // ~50 patterns to handle chained morphemes (e.g. antidisestablishmentarianism
    // → anti- + dis- + establish + -ment + -arian + -ism, 5 levels deep).
    //
    // No existence check — ALL candidates injected. Ascending PBD filters to valid.
    // No depth limit — each strip shortens the word, so it terminates naturally.
    // Morph bits accumulate through the chain (position carries the full stack).
    //
    // GPU broadphase only runs on original surface forms. All recursive stripping
    // is pure CPU string suffix/prefix matching — fast for ~50 patterns.
    //
    // Contraction irregular: won't → will + not (special-cased).

    AZStd::vector<StripCandidate> BedManager::GenerateStripCandidates(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>>& gpuMatches,
        const AZStd::unordered_map<AZ::u32, RulePack>& rulePacksByLength) const
    {
        AZStd::vector<StripCandidate> candidates;
        if (gpuMatches.empty()) return candidates;

        // Collect all rule entries across all lengths for CPU recursive matching.
        // Indexed by suffix/prefix text for O(1) lookup during recursion.
        // Small set (~50) — vector scan is also fine, but map is cleaner.
        struct FlatRule
        {
            AZStd::string pattern;
            AZStd::string morpheme;
            AZ::u16 morphBits = 0;
            bool isSuffix = true;
            AZStd::string addBase;
            AZStd::string stripPrefix;
            AZStd::string secondWord;
        };
        AZStd::vector<FlatRule> allRules;
        AZStd::unordered_set<AZStd::string> ruleDedup;

        for (const auto& [len, pack] : rulePacksByLength)
        {
            for (const auto& rule : pack.rules)
            {
                AZStd::string key = rule.pattern + (rule.isSuffix ? "_S" : "_P");
                if (ruleDedup.count(key)) continue;
                ruleDedup.insert(key);

                FlatRule fr;
                fr.pattern = rule.pattern;
                fr.morpheme = rule.morpheme;
                fr.morphBits = rule.morphBits;
                fr.isSuffix = rule.isSuffix;
                fr.addBase = rule.addBase;
                fr.stripPrefix = rule.stripPrefix;
                fr.secondWord = rule.secondWord;
                allRules.push_back(AZStd::move(fr));
            }
        }

        // Apply one rule to a word, returning the stripped base (empty if rule doesn't apply).
        auto applyRule = [](const AZStd::string& word, const FlatRule& rule) -> AZStd::string
        {
            AZ::u32 wordLen = static_cast<AZ::u32>(word.size());
            AZ::u32 patLen = static_cast<AZ::u32>(rule.pattern.size());
            if (patLen >= wordLen) return {};  // Pattern must be shorter than word

            if (rule.morpheme == "CONTRACTION")
            {
                // Contractions only fire on the original surface form (have apostrophe).
                // Don't recurse contractions on stripped bases.
                return {};
            }

            if (rule.isSuffix)
            {
                // Check suffix match
                if (word.substr(wordLen - patLen) != rule.pattern) return {};

                AZStd::string stem = word.substr(0, wordLen - patLen);
                if (!rule.addBase.empty() && rule.addBase != "__DOUBLING__")
                {
                    return stem + rule.addBase;
                }
                else if (rule.addBase == "__DOUBLING__" && stem.size() >= 2
                         && stem[stem.size()-1] == stem[stem.size()-2])
                {
                    return stem.substr(0, stem.size() - 1);
                }
                return stem;
            }
            else
            {
                // Check prefix match
                if (word.substr(0, patLen) != rule.pattern) return {};
                return word.substr(patLen);
            }
        };

        // Process each GPU match: generate first-level candidate, then recurse.
        AZ::u32 level1Count = 0;
        AZ::u32 recursiveCount = 0;

        for (const auto& [runIdx, patternIdx] : gpuMatches)
        {
            const AZStd::string& text = runs[runIdx].text;
            AZ::u32 len = static_cast<AZ::u32>(text.size());

            auto packIt = rulePacksByLength.find(len);
            if (packIt == rulePacksByLength.end()) continue;
            const RulePack& pack = packIt->second;
            if (patternIdx >= pack.rules.size()) continue;

            const RulePackEntry& rule = pack.rules[patternIdx];

            StripCandidate cand;
            cand.sourceRunIndex = runIdx;
            cand.morpheme = rule.morpheme;
            cand.morphBits = rule.morphBits;

            if (rule.morpheme == "CONTRACTION")
            {
                cand.isContraction = true;
                AZ::u32 baseLen = len - rule.patternLen;
                cand.baseWord = text.substr(0, baseLen);
                cand.secondWord = rule.secondWord;

                if (cand.baseWord == "wo" && rule.pattern == "n't")
                    cand.baseWord = "will";
            }
            else if (rule.isSuffix)
            {
                AZ::u32 stemLen = len - rule.patternLen;
                AZStd::string stem = text.substr(0, stemLen);

                if (!rule.addBase.empty() && rule.addBase != "__DOUBLING__")
                    cand.baseWord = stem + rule.addBase;
                else if (rule.addBase == "__DOUBLING__" && stem.size() >= 2
                         && stem[stem.size()-1] == stem[stem.size()-2])
                    cand.baseWord = stem.substr(0, stem.size() - 1);
                else
                    cand.baseWord = stem;
            }
            else
            {
                cand.baseWord = text.substr(rule.patternLen);
            }

            if (cand.baseWord.empty()) continue;
            candidates.push_back(cand);
            ++level1Count;

            // ---- Recursive expansion: strip the base further ----
            // Feed each new base back through all ~50 patterns.
            // Each strip shortens the word → guaranteed termination.
            // Morph bits accumulate (OR'd) through the chain.
            // Dedup by (sourceRunIndex, baseWord) to avoid redundant candidates.

            AZStd::unordered_set<AZStd::string> seen;
            seen.insert(text);         // Original surface form — don't re-emit
            seen.insert(cand.baseWord);

            // Work queue: (baseWord, accumulatedMorphBits, morphemeChain)
            struct RecurseEntry
            {
                AZStd::string word;
                AZ::u16 morphBits;
            };
            AZStd::vector<RecurseEntry> queue;
            queue.push_back({cand.baseWord, cand.morphBits});

            while (!queue.empty())
            {
                RecurseEntry current = AZStd::move(queue.back());
                queue.pop_back();

                for (const auto& fr : allRules)
                {
                    AZStd::string deeper = applyRule(current.word, fr);
                    if (deeper.empty() || deeper.size() < 2) continue;  // Min 2 chars for a valid base
                    if (seen.count(deeper)) continue;
                    seen.insert(deeper);

                    AZ::u16 stackedBits = current.morphBits | fr.morphBits;

                    StripCandidate rc;
                    rc.sourceRunIndex = runIdx;
                    rc.baseWord = deeper;
                    rc.morpheme = fr.morpheme;  // Deepest morpheme name (for logging)
                    rc.morphBits = stackedBits; // Accumulated through chain
                    rc.isContraction = false;
                    candidates.push_back(rc);
                    ++recursiveCount;

                    // Queue for further stripping
                    queue.push_back({deeper, stackedBits});
                }
            }
        }

        fprintf(stderr, "[BedManager] GenerateStripCandidates: %u level-1 + %u recursive = %zu total candidates\n",
            level1Count, recursiveCount, candidates.size());
        fflush(stderr);

        return candidates;
    }


    // ---- SetInflectionRules: split by rule_type, compile conditions ----
    void BedManager::SetInflectionRules(AZStd::vector<InflectionRule> rules)
    {
        m_inflectionRules.clear();
        m_compiledConditions.clear();
        m_prefixRules.clear();
        m_compiledPrefixConditions.clear();

        for (auto& r : rules)
        {
            if (r.ruleType == "PREFIX")
            {
                m_compiledPrefixConditions.emplace_back(r.condition.c_str(),
                    std::regex::ECMAScript | std::regex::optimize);
                m_prefixRules.push_back(AZStd::move(r));
            }
            else
            {
                m_compiledConditions.emplace_back(r.condition.c_str(),
                    std::regex::ECMAScript | std::regex::optimize);
                m_inflectionRules.push_back(AZStd::move(r));
            }
        }

        fprintf(stderr, "[BedManager] SetInflectionRules: %zu suffix, %zu prefix rules\n",
            m_inflectionRules.size(), m_prefixRules.size());
        fflush(stderr);
    }

    // ---- TryPrefixStrip ----------------------------------------------------
    // Data-driven prefix stripping. Mirrors TryInflectionStrip but strips from
    // the front. Rules loaded from inflection_rules WHERE rule_type='PREFIX'.
    //
    // For each matching prefix rule:
    //   1. Word starts with rule.stripPrefix
    //   2. Remaining base satisfies condition regex
    //   3. Base exists in LMDB (LookupWordLocal) — the vocab is the filter
    //   4. Yield a strip result with the base and morpheme name
    //
    // Returns empty vector if no prefix matches or no base resolves.
    // Prefix class (PFX_NEG, PFX_ITER, etc.) is stored as morpheme string in
    // the result — goes into morphemePositions map, not a MorphBit (no bit reserved).
    // -----------------------------------------------------------------------
    struct PrefixStripResult
    {
        AZStd::string baseWord;
        AZStd::string morpheme;   // e.g. "PFX_NEG", "PFX_ITER"
        AZStd::string addPrefix;  // prefix to prepend for reconstruction
    };

    // NOTE: Currently unused — interstitial strip disabled (2026-03-18).
    // Retained for variant normalize path and future use.
    [[maybe_unused]]
    static AZStd::vector<PrefixStripResult> TryPrefixStrip(
        const AZStd::string& word,
        const HCPVocabulary* vocab,
        const AZStd::vector<InflectionRule>* rules,
        const std::vector<std::regex>* compiled)
    {
        AZStd::vector<PrefixStripResult> results;
        if (!rules || !compiled || rules->empty()) return results;
        if (!vocab) return results;

        const std::string wordStd(word.c_str(), word.size());

        for (size_t ri = 0; ri < rules->size(); ++ri)
        {
            const InflectionRule& r = (*rules)[ri];
            const std::string&    pfx = r.stripPrefix.c_str();
            if (pfx.empty() || wordStd.size() <= pfx.size()) continue;
            if (wordStd.substr(0, pfx.size()) != pfx) continue;

            std::string base = wordStd.substr(pfx.size());

            // Condition regex applied against the base
            if (!std::regex_search(base, (*compiled)[ri])) continue;

            // Base must exist in LMDB — vocab is the filter
            AZStd::string tokenId = vocab->LookupWordLocal(
                AZStd::string(base.c_str(), base.size()));
            if (tokenId.empty()) continue;

            PrefixStripResult res;
            res.baseWord  = AZStd::string(base.c_str(), base.size());
            res.morpheme  = r.morpheme;
            res.addPrefix = r.addPrefix;
            results.push_back(AZStd::move(res));
        }
        return results;
    }

    // ---- Envelope sliding window ----

    void BedManager::InitEnvelopeWindow(int envelopeId, int warmSetSize)
    {
        m_envelopeId  = envelopeId;
        m_sliceCursor = 0;
        m_warmSetSize = warmSetSize;
        fprintf(stderr, "[BedManager] Envelope window init: id=%d warm=%d slice=%d\n",
            envelopeId, warmSetSize, LMDB_SLICE_SIZE);
        fflush(stderr);
    }

    void BedManager::ResetWindowToStart()
    {
        // Reset per-length batch cursors so each new document starts from offset 0.
        // LMDB hot window is NOT modified — the initial high-priority entries loaded at
        // envelope activation remain valid for single-word lookups (TryInflectionStrip).
        // The multi-slice loop uses QueryWarmBatch (direct Postgres per-length) rather
        // than LMDB advances, so the LMDB window is stable across documents.
        m_lengthCursorByLen.clear();

        // Also reset m_vocabByLength to the LMDB-resident entries (initial hot window).
        // This ensures the first pass for each length uses what's already in LMDB,
        // only going to Postgres when those entries are exhausted.
        RebuildVocab();
    }

    bool BedManager::AdvanceEnvelopeLengthBatch(AZ::u32 wordLength)
    {
        if (!m_envelopeManager || m_envelopeId == 0) return false;

        int& cursor = m_lengthCursorByLen[wordLength];
        // On first call for this length, cursor is 0 — start from beginning.
        // On subsequent calls, advance past the batch we just processed.
        if (cursor > 0 || !m_vocabByLength[wordLength].empty())
            cursor += LMDB_SLICE_SIZE;

        auto batch = m_envelopeManager->QueryWarmBatch(
            m_envelopeId, static_cast<int>(wordLength), cursor, LMDB_SLICE_SIZE);

        if (batch.empty()) return false;  // exhausted for this length

        // Replace (not append) the in-memory vocab for this length.
        auto& vec = m_vocabByLength[wordLength];
        vec.clear();
        vec.reserve(batch.size());
        for (const auto& e : batch)
        {
            VocabPack::Entry entry;
            entry.word      = e.word;
            entry.tokenId   = e.tokenId;
            entry.tierIndex = 0;
            entry.morphBits = MorphemeNameToBit(e.morpheme.c_str());
            vec.push_back(AZStd::move(entry));
        }

        // Also write this batch to LMDB w2t so LookupWordLocal (used by inflection
        // stripping) can find these entries. LMDB fills incrementally as resolution
        // progresses — only the current batch, not the whole working set.
        if (m_lmdbEnv && m_vocabDbiOpen)
        {
            MDB_txn* txn = nullptr;
            if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) == 0)
            {
                for (const auto& e : batch)
                {
                    MDB_val mKey = { e.word.size(), const_cast<char*>(e.word.c_str()) };
                    std::string valStr(e.tokenId.c_str(), e.tokenId.size());
                    if (!e.morpheme.empty())
                    {
                        valStr += '\x00';
                        valStr.append(e.morpheme.c_str(), e.morpheme.size());
                    }
                    MDB_val mVal = { valStr.size(), const_cast<char*>(valStr.data()) };
                    mdb_put(txn, m_vocabDbi, &mKey, &mVal, 0);
                }
                mdb_txn_commit(txn);
            }
        }

        fprintf(stderr, "[BedManager] Length %u batch advance: offset=%d, %zu entries loaded\n",
            wordLength, cursor, batch.size());
        fflush(stderr);
        return true;
    }

    bool BedManager::AdvanceEnvelopeSlice()
    {
        if (!m_envelopeManager || m_envelopeId == 0 || m_warmSetSize == 0) return false;

        // The window covers [cursor, cursor + 3*SLICE). Advance by one slot.
        int nextFeedStart = m_sliceCursor + LMDB_SLICE_SIZE * 3;
        if (nextFeedStart >= m_warmSetSize) return false;  // window at end of warm set

        m_envelopeManager->EvictSlice(m_envelopeId, m_sliceCursor, LMDB_SLICE_SIZE);
        m_envelopeManager->FeedSlice(m_envelopeId, nextFeedStart, LMDB_SLICE_SIZE);
        m_sliceCursor += LMDB_SLICE_SIZE;

        RebuildVocab();
        return true;
    }

    // ---- RebuildVocab: scan w2t and build in-memory vocab index ----
    //
    // Reads whatever entries are currently in the LMDB hot window (w2t).
    // With the sliding window, this is at most 3 * LMDB_SLICE_SIZE entries.
    // t2w is maintained by EnvelopeManager::FeedSlice — not rebuilt here.
    void BedManager::RebuildVocab()
    {
        m_vocabByLength.clear();
        m_activeWordLengths.clear();
        m_labelCountByLength.clear();

        if (!m_vocabDbiOpen || !m_lmdbEnv) return;

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, MDB_RDONLY, &txn) != 0) return;

        MDB_cursor* cursor = nullptr;
        if (mdb_cursor_open(txn, m_vocabDbi, &cursor) != 0)
        {
            mdb_txn_abort(txn);
            return;
        }

        MDB_val key, val;
        AZ::u32 totalEntries = 0;
        while (mdb_cursor_get(cursor, &key, &val, MDB_NEXT) == 0)
        {
            AZ::u32 len = static_cast<AZ::u32>(key.mv_size);
            if (len < static_cast<AZ::u32>(VBED_MIN_LEN) ||
                len > static_cast<AZ::u32>(VBED_MAX_LEN)) continue;

            VocabPack::Entry e;
            e.word      = AZStd::string(static_cast<const char*>(key.mv_data), key.mv_size);
            e.tierIndex = 0;  // All common tier — label broadphase wired once envelope has tier data
            e.morphBits = 0;

            // Extended LMDB value format (migration 044):
            //   token_id (14 bytes) + '\x00' + morpheme_name (if variant has a morpheme)
            // Standard format: just token_id (14 bytes, backward compatible).
            const char* valData = static_cast<const char*>(val.mv_data);
            size_t valSize = val.mv_size;
            if (valSize > 14 && valData[14] == '\x00')
            {
                e.tokenId = AZStd::string(valData, 14);
                const char* morphStart = valData + 15;
                int morphLen = static_cast<int>(valSize) - 15;
                if (morphLen > 0)
                {
                    char morphBuf[32] = {};
                    int copyLen = morphLen < 31 ? morphLen : 31;
                    memcpy(morphBuf, morphStart, copyLen);
                    e.morphBits = MorphemeNameToBit(morphBuf);
                }
            }
            else
            {
                e.tokenId = AZStd::string(valData, valSize);
            }

            m_vocabByLength[len].push_back(AZStd::move(e));
            ++totalEntries;
        }

        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);

        // t2w is maintained by EnvelopeManager::FeedSlice (written with MDB_NOOVERWRITE,
        // accumulates across slices, survives EvictSlice). Do not rebuild t2w here —
        // that would wipe accumulated reverse-map entries from previous slices.

        for (auto& [len, entries] : m_vocabByLength)
        {
            m_activeWordLengths.push_back(len);
            m_labelCountByLength[len] = 0;
        }
        AZStd::sort(m_activeWordLengths.begin(), m_activeWordLengths.end(),
            [](AZ::u32 a, AZ::u32 b) { return a < b; });

        fprintf(stderr, "[BedManager] RebuildVocab: %u entries across %zu word lengths\n",
            totalEntries, m_activeWordLengths.size());
        fflush(stderr);
    }

    // ---- ReadFilteredVocabSlice: read from in-memory vocab index ----
    //
    // Returns entries [startEntry, endEntry) for the given word length whose
    // first char is in neededChars. Reads from m_vocabByLength — no LMDB I/O
    // at resolve time. The returned vector is on system heap (no AZ pool pressure).
    std::vector<VocabPack::Entry> BedManager::ReadFilteredVocabSlice(
        AZ::u32 wordLength,
        const AZStd::unordered_set<char>& neededChars,
        AZ::u32 startEntry,
        AZ::u32 endEntry) const
    {
        std::vector<VocabPack::Entry> filtered;

        auto it = m_vocabByLength.find(wordLength);
        if (it == m_vocabByLength.end())
            return filtered;

        const auto& allEntries = it->second;
        AZ::u32 maxEntries = static_cast<AZ::u32>(allEntries.size());
        if (endEntry > maxEntries) endEntry = maxEntries;
        if (startEntry >= endEntry)
            return filtered;

        filtered.reserve((endEntry - startEntry) / 4 + 1);
        for (AZ::u32 i = startEntry; i < endEntry; ++i)
        {
            const auto& e = allEntries[i];
            if (!e.word.empty() && (neededChars.empty() || neededChars.count(e.word[0])))
                filtered.push_back(e);
        }

        return filtered;
    }


    // ---- RunPipelinedCascade: work-queue state machine that overlaps GPU phases ----
    //
    // Replaces the sequential RunPhaseCascade loop. Key differences:
    //
    //   Sequential:   [LoadA][SimA][DrainA] | [BuildPack] [LoadA][SimA][DrainA] | ...
    //                                        ^--- GPU idle here ---^
    //
    //   Pipelined:    [Load A+B+C][Sim A+B+C] | [Drain A → reload A, Drain B → reload B, ...]
    //                 [BuildPack(N+1) during Sim(N)]  ← GPU idle gap eliminated
    //
    // With WS_PRIMARY_COUNT=3:
    //   - ws_A draining phase N-1 results (CPU)
    //   - ws_B simulating phase N (GPU)
    //   - ws_C loading phase N+1 vocab + runs (CPU)
    // → GPU transitions directly from phase N to N+1 without idle gap.
    //
    // Work queue:  each item = (vocabStart, absPhaseIdx, runIndices)
    //   - Initial item: vocabStart=0, runIndices=all input runs
    //   - On drain: unresolved runs → new item at vocabStart += phaseSize
    //   - On dispatch: leftover (overflow) → re-inserted at queue head (same phase)
    // VocabPack cache:  built once per vocabStart, never rebuilt.
    //   Pre-built during dispatch (after BeginSimulate, before FetchSimResults).

    void BedManager::RunPipelinedCascade(
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const std::vector<VocabPack::Entry>& filteredVocab,
        AZStd::vector<AZ::u32>& currentIndices,
        AZStd::vector<ResolutionResult>& results,
        AZ::u32& phaseIndex)
    {
        AZ::u32 totalFiltered = static_cast<AZ::u32>(filteredVocab.size());
        if (totalFiltered == 0 || currentIndices.empty()) return;

        // ---- Dynamic phase sizing ----
        // Allocate as much vocab as the buffer allows given the actual stream run count.
        // At long lengths with few runs, the entire vocab often fits in one phase.
        // At short lengths with many runs the stream side constrains the phase size.
        AZ::u32 phaseSize;
        {
            AZ::u32 streamParticles = static_cast<AZ::u32>(currentIndices.size()) * wordLength;
            AZ::u32 computed = RC_VOCAB_PHASE_FLOOR;
            if (streamParticles < WS_BUFFER_CAPACITY)
            {
                AZ::u32 vocabAvail = WS_BUFFER_CAPACITY - streamParticles;
                computed = AZStd::max(vocabAvail / wordLength, RC_VOCAB_PHASE_FLOOR);
            }
            phaseSize = AZStd::min(computed, totalFiltered);
        }
        fprintf(stderr, "[BedManager] Length %u: %zu stream runs → phaseSize=%u/%u\n",
            wordLength, currentIndices.size(), phaseSize, totalFiltered);
        fflush(stderr);

        AZStd::vector<Workspace*> workspaces = GetWorkspacesForLength(wordLength);
        if (workspaces.empty()) return;

        // --- Work item: a batch of runs to resolve against a specific vocab slice ---
        struct WorkItem
        {
            AZ::u32 vocabStart;               // start offset into filteredVocab
            AZ::u32 absPhaseIdx;              // for ResolutionResult::tierResolved
            AZStd::vector<AZ::u32> runIndices;
        };

        // --- Per-workspace slot state ---
        struct WsSlot
        {
            Workspace* ws          = nullptr;
            bool       simulating  = false;
            AZ::u32    absPhaseIdx = 0;
            AZ::u32    vocabStart  = 0;
        };

        AZStd::vector<WsSlot> slots;
        slots.reserve(workspaces.size());
        for (auto* ws : workspaces)
            slots.push_back({ws, false, 0, 0});

        // --- VocabPack cache: built once per vocabStart, reused across workspaces ---
        // Key: vocabStart (= phase * phaseSize)
        // Pointers into this map remain stable across emplace (unordered_map guarantee).
        AZStd::unordered_map<AZ::u32, VocabPack> packCache;
        auto getOrBuildPack = [&](AZ::u32 start) -> const VocabPack*
        {
            auto it = packCache.find(start);
            if (it == packCache.end())
            {
                auto ins = packCache.emplace(start,
                    BuildVocabSliceFromEntries(wordLength, filteredVocab, start, phaseSize));
                return &ins.first->second;
            }
            return &it->second;
        };

        // --- Work queue ---
        // Vector + head index — insert at head for leftover re-queue (same phase must
        // finish before advancing). Items are never erased; queueHead advances instead.
        AZStd::vector<WorkItem> workQueue;
        size_t queueHead = 0;

        workQueue.push_back({0, phaseIndex, AZStd::move(currentIndices)});
        currentIndices.clear();  // repopulated below with permanently unresolved runs

        AZ::u32 maxAbsPhase = phaseIndex;

        // Pre-build phase-0 pack immediately (no GPU work yet, CPU is free)
        getOrBuildPack(0);

        // --- Main pipeline loop ---
        for (;;)
        {
            // ===== Step 1: Drain any workspace that has finished simulating =====
            for (auto& slot : slots)
            {
                if (!slot.simulating) continue;
                if (!slot.ws->IsSimDone()) continue;

                const VocabPack& pack = packCache.at(slot.vocabStart);
                slot.ws->FetchSimResults();
                slot.ws->CheckSettlement(0, pack);

                AZStd::vector<ResolutionResult> wsResolved;
                AZStd::vector<AZ::u32> wsUnresolved;
                slot.ws->CollectSplit(wsResolved, wsUnresolved);

                for (auto& r : wsResolved)
                {
                    r.tierResolved = slot.absPhaseIdx;
                    results.push_back(AZStd::move(r));
                }

                if (!wsUnresolved.empty())
                {
                    AZ::u32 nextStart = slot.vocabStart + phaseSize;
                    if (nextStart < totalFiltered)
                    {
                        AZ::u32 nextPhaseIdx = slot.absPhaseIdx + 1;
                        if (nextPhaseIdx > maxAbsPhase) maxAbsPhase = nextPhaseIdx;
                        workQueue.push_back({nextStart, nextPhaseIdx, AZStd::move(wsUnresolved)});
                    }
                    else
                    {
                        // Vocab exhausted — permanently unresolved
                        for (AZ::u32 idx : wsUnresolved)
                            currentIndices.push_back(idx);
                    }
                }

                slot.ws->ResetDynamics();
                slot.ws->DeactivateFromScene();
                slot.simulating = false;
            }

            // ===== Step 2: Dispatch idle workspaces from work queue =====
            for (auto& slot : slots)
            {
                if (slot.simulating) continue;
                if (queueHead >= workQueue.size()) continue;

                WorkItem& item = workQueue[queueHead];
                if (item.runIndices.empty()) { ++queueHead; continue; }

                const VocabPack* pack = getOrBuildPack(item.vocabStart);
                if (pack->vocabEntryCount == 0) { ++queueHead; continue; }

                AZStd::vector<AZ::u32> overflow;
                AZ::u32 offset = 0;
                bool hasRuns = LoadWorkspaceBatch(slot.ws, wordLength, runs,
                                                  item.runIndices, offset, *pack, overflow);

                // Collect runs this workspace couldn't fit
                AZStd::vector<AZ::u32> leftover;
                for (AZ::u32 j = offset; j < static_cast<AZ::u32>(item.runIndices.size()); ++j)
                    leftover.push_back(item.runIndices[j]);
                leftover.insert(leftover.end(), overflow.begin(), overflow.end());

                AZ::u32 savedVocabStart = item.vocabStart;
                AZ::u32 savedPhaseIdx   = item.absPhaseIdx;
                ++queueHead;  // consume this item

                if (!leftover.empty())
                {
                    // Re-insert leftover at queue head: same phase must complete
                    // before advancing to the next phase's work items.
                    workQueue.insert(workQueue.begin() + static_cast<ptrdiff_t>(queueHead),
                        WorkItem{savedVocabStart, savedPhaseIdx, AZStd::move(leftover)});
                    // queueHead now points at the re-inserted item (next slot gets it)
                }

                if (hasRuns)
                {
                    slot.ws->BeginSimulate(RC_SETTLE_STEPS, RC_DT);
                    slot.simulating  = true;
                    slot.absPhaseIdx = savedPhaseIdx;
                    slot.vocabStart  = savedVocabStart;

                    // === KEY OPTIMIZATION: pre-build next phase pack while GPU runs ===
                    // BuildVocabSliceFromEntries is pure CPU/memory work. By building it
                    // here — after BeginSimulate, before the next FetchSimResults — the
                    // build time is hidden behind GPU simulation rather than adding to
                    // the critical path between phases.
                    AZ::u32 nextStart = savedVocabStart + phaseSize;
                    if (nextStart < totalFiltered)
                        getOrBuildPack(nextStart);
                }
            }

            // ===== Termination check =====
            bool anySim = false;
            for (const auto& slot : slots)
                if (slot.simulating) { anySim = true; break; }
            if (!anySim && queueHead >= workQueue.size()) break;
        }

        // Update phase counter for caller (label pass feeds into common pass)
        phaseIndex = maxAbsPhase + 1;
    }

    void BedManager::ResolveLengthCycle(
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& runIndices,
        AZStd::vector<ResolutionResult>& results,
        AZStd::vector<AZ::u32>& unresolvedIndices)
    {
        if (runIndices.empty()) return;

        // If no vocab loaded for this length, try fetching the first batch from Postgres.
        // This is the normal path when LMDB is not pre-populated — the resolution loop
        // drives loading on demand via per-length warm-set queries.
        auto vocabIt = m_vocabByLength.find(wordLength);
        if (vocabIt == m_vocabByLength.end() || vocabIt->second.empty())
        {
            if (!AdvanceEnvelopeLengthBatch(wordLength))
            {
                // No vocab available for this length at all
                for (AZ::u32 idx : runIndices)
                    unresolvedIndices.push_back(idx);
                return;
            }
            vocabIt = m_vocabByLength.find(wordLength);
            if (vocabIt == m_vocabByLength.end() || vocabIt->second.empty())
            {
                for (AZ::u32 idx : runIndices)
                    unresolvedIndices.push_back(idx);
                return;
            }
        }

        // ---- Split runs: capitalized (eligible for Label match) vs plain ----
        AZStd::vector<AZ::u32> capRuns;
        AZStd::vector<AZ::u32> plainRuns;
        for (AZ::u32 idx : runIndices)
        {
            const CharRun& run = runs[idx];
            if (run.firstCap || run.allCaps)
                capRuns.push_back(idx);
            else
                plainRuns.push_back(idx);
        }

        AZ::u32 phaseIndex = 0;

        // ---- Pass 1: Label lookup — capitalized runs ONLY ----
        // Labels are stored in original case (capitalized). CharRun text is lowercased.
        // For each cap run, reconstruct the capitalized forms and try direct lookup
        // in LMDB w2t. Order: all-caps → title-case → (skip lowercase,
        // that's handled in the common vocab pass below).
        // Ensure the batch for this length is loaded first so LMDB has the data.
        if (!capRuns.empty())
        {
            AZStd::vector<AZ::u32> resolvedCapRuns;
            for (AZ::u32 idx : capRuns)
            {
                const CharRun& run = runs[idx];
                if (run.text.empty()) continue;

                AZStd::string tokenId;

                // Try 1: All-caps form (IRS, NASA, etc.)
                if (run.allCaps)
                {
                    AZStd::string upper = run.text;
                    for (auto& c : upper) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
                    tokenId = m_vocabulary ? m_vocabulary->LookupWordLocal(upper) : "";
                }

                // Try 2: Title-case form (John, London, etc.)
                if (tokenId.empty())
                {
                    AZStd::string titled = run.text;
                    titled[0] = static_cast<char>(toupper(static_cast<unsigned char>(titled[0])));
                    tokenId = m_vocabulary ? m_vocabulary->LookupWordLocal(titled) : "";
                }

                if (!tokenId.empty())
                {
                    ResolutionResult r;
                    r.runText = run.text;
                    r.resolved = true;
                    r.matchedWord = run.text;
                    r.matchedTokenId = tokenId;
                    r.tierResolved = 0;  // Label tier
                    r.runIndex = idx;
                    r.firstCap = run.firstCap;
                    r.allCaps = run.allCaps;
                    results.push_back(r);
                    resolvedCapRuns.push_back(idx);
                }
            }

            // Remove resolved cap runs from the candidate lists
            if (!resolvedCapRuns.empty())
            {
                AZStd::unordered_set<AZ::u32> resolvedSet(resolvedCapRuns.begin(), resolvedCapRuns.end());
                AZStd::vector<AZ::u32> remainingCaps;
                for (AZ::u32 idx : capRuns)
                    if (resolvedSet.find(idx) == resolvedSet.end())
                        remainingCaps.push_back(idx);
                capRuns = AZStd::move(remainingCaps);

                fprintf(stderr, "[BedManager] Length %u Label direct: %zu resolved, %zu remaining caps\n",
                    wordLength, resolvedCapRuns.size(), capRuns.size());
                fflush(stderr);
            }
        }

        // ---- Pass 2: Common vocab — ALL remaining runs, loop through ALL warm-set slices ----
        //
        // Each iteration processes the current hot-cache window for this word length.
        // When unresolved runs remain, advance to the next slice and try again.
        // Stop only when all runs are resolved, or no more slices exist for this length.
        // Since warm-set rows are ordered (word_length, effective_priority), once the
        // sliding window moves past the last entry for this length the inner check breaks.
        AZStd::vector<AZ::u32> commonRuns;
        commonRuns.reserve(capRuns.size() + plainRuns.size());
        for (AZ::u32 idx : capRuns)   commonRuns.push_back(idx);
        for (AZ::u32 idx : plainRuns) commonRuns.push_back(idx);

        while (!commonRuns.empty())
        {
            auto curVocabIt = m_vocabByLength.find(wordLength);
            if (curVocabIt == m_vocabByLength.end() || curVocabIt->second.empty())
                break;  // no vocab for this length in the current hot window

            AZ::u32 curTotal  = static_cast<AZ::u32>(curVocabIt->second.size());
            AZStd::unordered_set<char> commonChars;
            for (AZ::u32 idx : commonRuns)
                if (!runs[idx].text.empty())
                    commonChars.insert(runs[idx].text[0]);

            // Start from 0 — Labels already handled by direct lookup above,
            // common vocab pass processes ALL entries including any remaining Labels
            auto filteredCommon = ReadFilteredVocabSlice(wordLength, commonChars, 0, curTotal);
            fprintf(stderr, "[BedManager] Length %u common pass (cursor=%d): %zu runs, %u vocab → %zu filtered\n",
                wordLength, m_sliceCursor, commonRuns.size(), curTotal, filteredCommon.size());
            fflush(stderr);

            if (!filteredCommon.empty())
                RunPipelinedCascade(wordLength, runs, filteredCommon, commonRuns, results, phaseIndex);

            if (commonRuns.empty()) break;                              // all runs resolved
            if (!AdvanceEnvelopeLengthBatch(wordLength)) break;        // exhausted for this length
        }

        for (AZ::u32 idx : commonRuns)
            unresolvedIndices.push_back(idx);
    }

    ResolutionManifest BedManager::Resolve(const AZStd::vector<CharRun>& inputRuns)
    {
        ResolutionManifest manifest;
        manifest.totalRuns = static_cast<AZ::u32>(inputRuns.size());

        if (inputRuns.empty() || !m_initialized)
        {
            manifest.unresolvedRuns = manifest.totalRuns;
            return manifest;
        }

        // Reset the LMDB hot window to offset 0 for each new document.
        // Each document needs the highest-priority vocab entries first.
        // Without this reset, the cursor would remain wherever the last document left it.
        ResetWindowToStart();

        // Mutable copy — synthetic base runs get appended during interstitial stripping.
        // Original runs at [0..inputRuns.size()), synthetics at [inputRuns.size()..N).
        AZStd::vector<CharRun> runs = inputRuns;
        const AZ::u32 originalRunCount = static_cast<AZ::u32>(inputRuns.size());

        auto t0 = std::chrono::high_resolution_clock::now();

        // ---- Transform layer: pre-resolve tagged runs ----
        AZ::u32 preResolved = 0;

        for (AZ::u32 i = 0; i < originalRunCount; ++i)
        {
            const CharRun& run = runs[i];
            if (run.tag == RunTag::SingleChar)
            {
                // Lookup: try run.text as-is first; if that fails and firstCap is set
                // (intrinsic cap like "I"), retry with first char uppercased.
                AZStd::string tokenId = run.preAssignedTokenId;
                if (tokenId.empty() && m_vocabulary)
                {
                    tokenId = m_vocabulary->LookupWordLocal(run.text);
                    if (tokenId.empty() && !run.text.empty())
                    {
                        // "I" is stored uppercase in LMDB — always try capitalised form
                        // for any single-char that fails the lowercase lookup.
                        AZStd::string cap = run.text;
                        cap[0] = static_cast<char>(toupper(static_cast<unsigned char>(cap[0])));
                        tokenId = m_vocabulary->LookupWordLocal(cap);
                    }
                    // Fallback: decode UTF-8 codepoint and query c2t sub-db.
                    // Punctuation is stored in c2t (char→token), not w2t (word→token).
                    if (tokenId.empty() && !run.text.empty())
                    {
                        const unsigned char b0 = static_cast<unsigned char>(run.text[0]);
                        AZ::u32 cp = b0;
                        if (b0 >= 0xE0 && run.text.size() >= 3)
                            cp = ((b0 & 0x0F) << 12)
                               | ((static_cast<unsigned char>(run.text[1]) & 0x3F) << 6)
                               | (static_cast<unsigned char>(run.text[2]) & 0x3F);
                        else if (b0 >= 0xC0 && run.text.size() >= 2)
                            cp = ((b0 & 0x1F) << 6) | (static_cast<unsigned char>(run.text[1]) & 0x3F);
                        tokenId = m_vocabulary->LookupCodepoint(cp);
                    }
                }
                ResolutionResult r;
                r.runText = run.text;
                r.resolved = !tokenId.empty();
                r.matchedWord = run.text;
                r.matchedTokenId = tokenId;
                r.tierResolved = 0;
                r.runIndex = i;
                r.firstCap = run.firstCap;
                r.allCaps = run.allCaps;
                manifest.results.push_back(r);
                ++preResolved;
            }
            else if (run.tag == RunTag::Numeric)
            {
                // Numbers are unresolved — ScanManifestToPBM assigns a per-number var token.
                ResolutionResult r;
                r.runText = run.text;
                r.resolved = false;
                r.matchedWord = run.text;
                r.tierResolved = 0;
                r.runIndex = i;
                manifest.results.push_back(r);
                ++preResolved;
            }
            else if (run.tag == RunTag::Newline)
            {
                // Paragraph break — resolve to newline char token
                AZStd::string tokenId = m_vocabulary ? m_vocabulary->LookupChar('\n') : "";
                if (!tokenId.empty())
                {
                    ResolutionResult r;
                    r.runText = "\n";
                    r.resolved = true;
                    r.matchedWord = "\n";
                    r.matchedTokenId = tokenId;
                    r.tierResolved = 0;
                    r.runIndex = i;
                    manifest.results.push_back(r);
                    ++preResolved;
                }
            }
        }

        if (preResolved > 0)
        {
            fprintf(stderr, "[BedManager] Transform pre-handled: %u runs (SingleChar/Numeric)\n", preResolved);
            fflush(stderr);
        }

        // ---- Classify Word runs + duplicate stacking ----
        AZStd::unordered_map<AZStd::string, AZ::u32> uniqueRunMap;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> runStacks;

        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> runsByLength;
        AZStd::vector<AZ::u32> apostropheRuns;
        AZStd::vector<AZ::u32> hyphenRuns;
        AZStd::vector<AZ::u32> noVocabRuns;

        // Build a set of active word lengths for O(1) lookup
        AZStd::unordered_set<AZ::u32> activeLenSet;
        for (AZ::u32 len : m_activeWordLengths)
            activeLenSet.insert(len);

        for (AZ::u32 i = 0; i < originalRunCount; ++i)
        {
            const CharRun& run = runs[i];
            if (run.text.empty() || run.tag != RunTag::Word) continue;

            auto [it, inserted] = uniqueRunMap.emplace(run.text, i);
            if (!inserted)
            {
                runStacks[it->second].push_back(i);
                continue;
            }

            bool hasApostrophe = run.text.find('\'') != AZStd::string::npos;
            bool hasHyphen = run.text.find('-') != AZStd::string::npos;

            if (hasApostrophe)
            {
                apostropheRuns.push_back(i);
            }
            else if (hasHyphen)
            {
                hyphenRuns.push_back(i);
            }
            else
            {
                AZ::u32 len = run.length;
                if (activeLenSet.count(len))
                {
                    runsByLength[len].push_back(i);
                }
                else
                    noVocabRuns.push_back(i);
            }
        }

        AZ::u32 uniqueWordRuns = static_cast<AZ::u32>(uniqueRunMap.size());
        AZ::u32 totalDuplicates = 0;
        for (const auto& [_, stack] : runStacks)
            totalDuplicates += static_cast<AZ::u32>(stack.size());

        fprintf(stderr, "[BedManager] Classification: %zu lengths with runs, %zu apostrophe, "
            "%zu hyphen, %zu no-vocab | %u unique words (%u duplicates stacked)\n",
            runsByLength.size(), apostropheRuns.size(), hyphenRuns.size(),
            noVocabRuns.size(), uniqueWordRuns, totalDuplicates);
        fflush(stderr);

        // ---- Apostrophe runs: vocabulary LMDB lookup ----
        AZStd::vector<AZ::u32> unresolvedApostrophe;
        for (AZ::u32 idx : apostropheRuns)
        {
            const CharRun& arun = runs[idx];
            AZStd::string tokenId = m_vocabulary ? m_vocabulary->LookupWordLocal(arun.text) : "";
            if (!tokenId.empty())
            {
                ResolutionResult r;
                r.runText = arun.text;
                r.resolved = true;
                r.matchedWord = arun.text;
                r.matchedTokenId = tokenId;
                r.tierResolved = 0;
                r.runIndex = idx;
                r.firstCap = arun.firstCap;
                r.allCaps = arun.allCaps;
                manifest.results.push_back(r);
            }
            else
            {
                unresolvedApostrophe.push_back(idx);
            }
        }

        // ---- Hyphen runs: vocabulary LMDB lookup ----
        AZStd::vector<AZ::u32> unresolvedHyphen;
        for (AZ::u32 idx : hyphenRuns)
        {
            const CharRun& hrun = runs[idx];
            AZStd::string tokenId = m_vocabulary ? m_vocabulary->LookupWordLocal(hrun.text) : "";
            if (!tokenId.empty())
            {
                ResolutionResult r;
                r.runText = hrun.text;
                r.resolved = true;
                r.matchedWord = hrun.text;
                r.matchedTokenId = tokenId;
                r.tierResolved = 0;
                r.runIndex = idx;
                r.firstCap = hrun.firstCap;
                r.allCaps = hrun.allCaps;
                manifest.results.push_back(r);
            }
            else
            {
                unresolvedHyphen.push_back(idx);
            }
        }

        if (!apostropheRuns.empty() || !hyphenRuns.empty())
        {
            fprintf(stderr, "[BedManager] Punctuation vocab-lookup: apo %zu/%zu resolved, hyp %zu/%zu resolved\n",
                apostropheRuns.size() - unresolvedApostrophe.size(), apostropheRuns.size(),
                hyphenRuns.size() - unresolvedHyphen.size(), hyphenRuns.size());
            fflush(stderr);
        }

        // ---- Broadphase strip pass: morphemes + contractions (3-stage pipeline) ----
        //
        // Stage 1: GPU partial-match — identifies which suffix/prefix/contraction patterns
        //   match which words across all lengths in parallel. Fast broadphase filter.
        // Stage 2: CPU candidate expansion — mechanically generates ALL possible base words
        //   from GPU matches. No existence check. Multiple candidates per run expected.
        // Stage 3: Ascending-length exact PBD (below) — bases injected into runsByLength.
        //   Shortest valid base wins naturally. Failed bases don't resolve = original var.

        AZStd::vector<StripCandidate> stripCandidates;
        fprintf(stderr, "[BedManager] Broadphase entry: %zu suffix rules, %zu prefix rules\n",
            m_inflectionRules.size(), m_prefixRules.size());
        fflush(stderr);
        if (!m_inflectionRules.empty() || !m_prefixRules.empty())
        {
            // Collect broadphase candidates: all word runs + unresolved apostrophe runs
            AZStd::vector<AZ::u32> broadphaseCandidates;

            // Plain word runs that could have suffix/prefix morphemes
            for (const auto& [len, indices] : runsByLength)
            {
                for (AZ::u32 idx : indices)
                    broadphaseCandidates.push_back(idx);
            }

            // Unresolved apostrophe runs (contraction candidates)
            for (AZ::u32 idx : unresolvedApostrophe)
                broadphaseCandidates.push_back(idx);

            fprintf(stderr, "[BedManager] Broadphase candidates: %zu word + %zu apostrophe = %zu total\n",
                broadphaseCandidates.size() - unresolvedApostrophe.size(),
                unresolvedApostrophe.size(), broadphaseCandidates.size());
            fflush(stderr);

            if (!broadphaseCandidates.empty())
            {
                // Stage 1: GPU broadphase filter — partial matching
                auto gpuMatches = RunBroadphaseFilter(runs, broadphaseCandidates);

                // Build rule packs by length for candidate expansion lookup
                AZStd::unordered_map<AZ::u32, RulePack> rulePacksByLength;
                for (AZ::u32 idx : broadphaseCandidates)
                {
                    AZ::u32 len = static_cast<AZ::u32>(runs[idx].text.size());
                    if (!rulePacksByLength.count(len))
                        rulePacksByLength[len] = BuildRulePack(len);
                }

                // Stage 2: CPU mechanical expansion — generate ALL possible bases
                stripCandidates = GenerateStripCandidates(runs, gpuMatches, rulePacksByLength);

                // Stage 3 setup: inject ALL base words into runsByLength for ascending PBD
                for (const auto& cand : stripCandidates)
                {
                    if (cand.baseWord.empty()) continue;

                    // Create synthetic CharRun for the base word
                    CharRun baseCr;
                    baseCr.text = cand.baseWord;
                    baseCr.startPos = 0;
                    baseCr.length = static_cast<AZ::u32>(cand.baseWord.size());
                    baseCr.tag = RunTag::Word;
                    AZ::u32 baseIdx = static_cast<AZ::u32>(runs.size());
                    runs.push_back(baseCr);

                    AZ::u32 baseLen = baseCr.length;
                    if (activeLenSet.count(baseLen))
                        runsByLength[baseLen].push_back(baseIdx);

                    // For contractions: also inject the second constituent word
                    if (cand.isContraction && !cand.secondWord.empty())
                    {
                        CharRun secondCr;
                        secondCr.text = cand.secondWord;
                        secondCr.startPos = 0;
                        secondCr.length = static_cast<AZ::u32>(cand.secondWord.size());
                        secondCr.tag = RunTag::Word;
                        AZ::u32 secondIdx = static_cast<AZ::u32>(runs.size());
                        runs.push_back(secondCr);

                        AZ::u32 secondLen = secondCr.length;
                        if (activeLenSet.count(secondLen))
                            runsByLength[secondLen].push_back(secondIdx);
                    }
                }

                if (!stripCandidates.empty())
                {
                    fprintf(stderr, "[BedManager] Broadphase injected %zu base words + contractions into resolution queue\n",
                        stripCandidates.size());
                    fflush(stderr);
                }
            }
        }

        // ---- Ascending length resolve loop with interstitial inflection stripping ----
        //
        // DEPRECATED (2026-03-17): Interstitial stripping will be replaced by a
        // broadphase PBD strip pass that runs BEFORE ascending-length resolution.
        // The broadphase pass handles both morpheme stripping (~39 rules) and
        // contraction decomposition (~7 patterns) in a single GPU pass, filtered
        // by apostrophe presence and known suffix/prefix endings.
        // Failed strips fall back to the ascending-length resolution below.
        // See memory/morpheme_strip_redesign.md for full design.
        //
        // Current (soon-deprecated) flow:
        // Shortest-first order: "run" (3) processes before "running" (7).
        // Function words and short signal words resolve first, establishing context.
        //
        // After each length cycle, unresolved runs get stripped:
        //   - Strip suffix → base word + morph bits (delta)
        //   - Since shorter lengths are ALREADY processed, the base is either:
        //     A) Already in manifest.results (appeared in text) → dep-resolution finds it
        //     B) Not in text → direct LMDB lookup, synthetic manifest entry added
        //   - If base can't strip or LMDB lookup fails → var
        //
        // No synthetic CharRuns injected into runsByLength (those queues are done).
        // CPU manifest check + LMDB direct lookup replace the GPU injection path.
        //
        // Post-loop: dep-resolution pass scans manifest for resolved bases,
        // propagates morph bits to dependents. Unresolved bases → vars.

        struct InflectionQueueEntry
        {
            AZ::u32 runIndex;
            AZStd::vector<InflectionStripResult> candidates;  // priority-ordered
        };
        AZStd::vector<InflectionQueueEntry> inflectionQueue;
        AZStd::vector<AZ::u32> allUnresolvedOriginal;
        AZ::u32 inflectionCount = 0;
        AZ::u32 syntheticInjections = 0;  // direct LMDB lookups that succeeded
        AZStd::unordered_set<AZStd::string> lookupDone;  // dedup: base words already looked up
        bool shortPassSignalFired = false;  // pre-fetch fires once, after first length >= 4

        for (AZ::u32 len : m_activeWordLengths)
        {
            // Fetch indices for this length — may include synthetics injected by earlier (longer) cycles
            auto it = runsByLength.find(len);
            if (it == runsByLength.end() || it->second.empty()) continue;

            AZStd::vector<AZ::u32> indices = it->second;

            AZStd::vector<AZ::u32> unresolvedFromCycle;
            ResolveLengthCycle(len, runs, indices,
                               manifest.results, unresolvedFromCycle);

            // ---- Interstitial strip: DISABLED (2026-03-18) ----
            // Broadphase strip pass (above) now handles morpheme/contraction
            // identification before the ascending loop. All unresolved runs
            // go to allUnresolvedOriginal for broadphase dep-resolution.
            // Original interstitial code preserved in git history for reference.
            for (AZ::u32 idx : unresolvedFromCycle)
                allUnresolvedOriginal.push_back(idx);

            fprintf(stderr, "[BedManager] Length %u: %zu runs, %zu unresolved\n",
                len, indices.size(), unresolvedFromCycle.size());
            fflush(stderr);

            // ---- Short-pass tense pre-fetch (fires once, after first length >= 4) ----
            //
            // After function-word lengths (2-4) resolve, detect tense/register signals
            // (past: was/were/had; archaic: hath/thou/etc.) and query the matching Postgres
            // envelope to inject inflected forms into m_vocabByLength for lengths 5+.
            // These entries are included by the next ResolveLengthCycle call via
            // ReadFilteredVocabSlice (which reads up to totalEntries = current vector size).
            if (!shortPassSignalFired && len >= 4 && m_envelopeManager)
            {
                shortPassSignalFired = true;
                ShortPassSignal sig = DetectSignals(manifest);

                // Pick dominant signal — priority: past > progressive > present
                AZStd::string targetEnvelope;
                if      (sig.hasPast)        targetEnvelope = "english_past_tense";
                else if (sig.hasProgressive) targetEnvelope = "english_progressive";
                else if (sig.hasPresent)     targetEnvelope = "english_plural";

                if (!targetEnvelope.empty())
                {
                    fprintf(stderr,
                        "[BedManager] Short-pass signal: past=%d prog=%d pres=%d archaic=%d"
                        " → pre-fetch '%s'\n",
                        sig.hasPast, sig.hasProgressive, sig.hasPresent, sig.hasArchaic,
                        targetEnvelope.c_str());
                    fflush(stderr);

                    auto entries = m_envelopeManager->QueryEnvelopeEntries(targetEnvelope);
                    AZ::u32 injected = 0;
                    for (const auto& e : entries)
                    {
                        AZ::u32 eLen = static_cast<AZ::u32>(e.word.size());
                        if (eLen < 5 || eLen > 16) continue;  // only unprocessed lengths
                        // FIXED (2026-03-17): AD namespace no longer exists — all tokens
                        // are now AB. Label detection should use PoS/cap metadata, not
                        // namespace prefix. For now, inject everything as tier 1.
                        // TODO: Wire proper Label detection from token_pos PoS data.
                        VocabPack::Entry entry;
                        entry.word      = e.word;
                        entry.tokenId   = e.tokenId;
                        entry.tierIndex = 1u;
                        entry.morphBits = MorphemeNameToBit(e.morpheme.c_str());
                        m_vocabByLength[eLen].push_back(AZStd::move(entry));
                        ++injected;
                    }

                    fprintf(stderr, "[BedManager] Pre-fetch injected %u entries from '%s'\n",
                        injected, targetEnvelope.c_str());
                    fflush(stderr);
                }
                else if (sig.hasArchaic)
                {
                    // Archaic envelope not yet defined — log for future wiring
                    fprintf(stderr, "[BedManager] Archaic signal detected"
                        " — no archaic envelope yet (future Task #16)\n");
                    fflush(stderr);
                }
            }
        }

        // ---- Resolve inflected dependents — priority-ordered candidate matching ----
        //
        // For each queued inflected run, check its candidates in priority order.
        // The first candidate whose base resolved in PBD wins; the rest are skipped.
        // resolvedRunIndices prevents duplicate manifest entries (a run can appear in
        // multiple candidate entries if it had several strip paths injected).
        //
        // Silent-e fallback is now folded in: base+"e" candidates were injected in the
        // main loop alongside all other candidates, so no separate pass is needed.
        {
            // Build resolved-base lookup from manifest — store copies (not pointers).
            // push_back() below can reallocate manifest.results, which would invalidate
            // any raw pointers into it (same fix applied to resolvedLookup in dupe propagation).
            AZStd::unordered_map<AZStd::string, ResolutionResult> resolvedBases;
            for (const auto& r : manifest.results)
                if (r.resolved) resolvedBases[r.matchedWord] = r;

            // Pre-populate resolvedRunIndices from already-resolved manifest entries
            AZStd::unordered_set<AZ::u32> resolvedRunIndices;
            for (const auto& r : manifest.results)
                resolvedRunIndices.insert(r.runIndex);

            AZ::u32 depResolved = 0;
            for (const auto& qe : inflectionQueue)
            {
                if (resolvedRunIndices.count(qe.runIndex)) continue;

                bool resolved = false;
                for (const auto& cand : qe.candidates)
                {
                    auto it = resolvedBases.find(cand.baseWord);
                    if (it == resolvedBases.end()) continue;

                    const CharRun& depRun = runs[qe.runIndex];
                    ResolutionResult r;
                    r.runText        = depRun.text;
                    r.resolved       = true;
                    r.matchedWord    = it->second.matchedWord;
                    r.matchedTokenId = it->second.matchedTokenId;
                    r.morphBits      = cand.morphBits;
                    r.tierResolved   = it->second.tierResolved;
                    r.runIndex       = qe.runIndex;
                    r.firstCap       = depRun.firstCap;
                    r.allCaps        = depRun.allCaps;
                    manifest.results.push_back(r);
                    resolvedRunIndices.insert(qe.runIndex);
                    ++depResolved;
                    resolved = true;
                    break;
                }

                if (!resolved)
                    allUnresolvedOriginal.push_back(qe.runIndex);
            }

            if (depResolved > 0)
            {
                fprintf(stderr, "[BedManager] Inflected dependents resolved: %u\n", depResolved);
                fflush(stderr);
            }
        }

        if (inflectionCount > 0)
        {
            fprintf(stderr, "[BedManager] Inflection-stripped: %u runs (%u direct LMDB lookups)\n",
                inflectionCount, syntheticInjections);
            fflush(stderr);
        }

        // ---- Broadphase dep-resolution: map resolved bases back to source runs ----
        //
        // The broadphase injected synthetic CharRuns for stripped bases into runsByLength.
        // Ascending PBD resolved the valid ones. Now scan stripCandidates: for each whose
        // baseWord resolved in the manifest, create a manifest entry for the ORIGINAL
        // source run (the inflected/contracted surface form) with accumulated morph bits.
        //
        // For contractions: the source run resolves to the base word's token_id + the
        // second word also resolves separately (already injected). Both get manifest entries.
        if (!stripCandidates.empty())
        {
            // Build resolved-base lookup from manifest (copies, not pointers — manifest grows)
            AZStd::unordered_map<AZStd::string, ResolutionResult> resolvedBases;
            for (const auto& r : manifest.results)
                if (r.resolved) resolvedBases[r.matchedWord] = r;

            AZStd::unordered_set<AZ::u32> resolvedRunIndices;
            for (const auto& r : manifest.results)
                resolvedRunIndices.insert(r.runIndex);

            AZ::u32 broadphaseResolved = 0;
            for (const auto& cand : stripCandidates)
            {
                if (resolvedRunIndices.count(cand.sourceRunIndex)) continue;

                auto it = resolvedBases.find(cand.baseWord);
                if (it == resolvedBases.end()) continue;

                const CharRun& srcRun = runs[cand.sourceRunIndex];
                ResolutionResult r;
                r.runText        = srcRun.text;
                r.resolved       = true;
                r.matchedWord    = it->second.matchedWord;
                r.matchedTokenId = it->second.matchedTokenId;
                r.morphBits      = cand.morphBits;
                r.tierResolved   = it->second.tierResolved;
                r.runIndex       = cand.sourceRunIndex;
                r.firstCap       = srcRun.firstCap;
                r.allCaps        = srcRun.allCaps;
                manifest.results.push_back(r);
                resolvedRunIndices.insert(cand.sourceRunIndex);
                ++broadphaseResolved;
            }

            // Remove broadphase-resolved indices from allUnresolvedOriginal
            if (broadphaseResolved > 0)
            {
                AZStd::vector<AZ::u32> stillUnresolved;
                for (AZ::u32 idx : allUnresolvedOriginal)
                {
                    if (!resolvedRunIndices.count(idx))
                        stillUnresolved.push_back(idx);
                }
                allUnresolvedOriginal = AZStd::move(stillUnresolved);

                fprintf(stderr, "[BedManager] Broadphase dep-resolution: %u runs resolved via stripped bases\n",
                    broadphaseResolved);
                fflush(stderr);
            }
        }

        // ---- Stage 2: TryVariantNormalize on still-unresolved runs ----
        //
        // Handles dialect g-drop (-in') and archaic -eth forms.
        // Runs AFTER TryInflectionStrip + silent-e fallback — only fires on what
        // those stages left unresolved.
        //
        // Three candidate paths per matching run (priority order):
        //   (a) Normalized form directly  — e.g., "darling" for "darlin'"
        //   (b) Alt form (base+e)         — e.g., "make" for "maketh"
        //   (c) Inflection-stripped base  — e.g., "run" for "runnin'" (via "running")
        // Whichever resolves in PBD cleanup wins; first listed takes priority.
        AZStd::unordered_set<AZ::u32> variantResolvedSet;  // run indices resolved here
        {
            struct VariantDep { AZ::u32 runIndex; AZ::u16 morphBits; };
            AZStd::unordered_map<AZStd::string, AZStd::vector<VariantDep>> vNorm;   // normalizedForm → deps
            AZStd::unordered_map<AZStd::string, AZStd::vector<VariantDep>> vAlt;    // altForm → deps
            AZStd::unordered_map<AZStd::string, AZStd::vector<VariantDep>> vStrip;  // strippedBase → deps

            auto enqueueVariant = [&](AZ::u32 idx)
            {
                VariantNormalResult vr = TryVariantNormalize(runs[idx].text);
                if (!vr.normalized) return;
                vNorm[vr.normalizedForm].push_back({idx, vr.variantBits});
                if (!vr.altForm.empty())
                    vAlt[vr.altForm].push_back({idx, vr.variantBits});
                // For dialect V-1: also inject inflection-stripped base of normalized form
                // e.g., "runnin'" → "running" → TryInflectionStrip → "run" + PROG
                if (vr.variantBits & MorphBit::VARIANT_DIALECT)
                {
                    for (const auto& strip : TryInflectionStrip(vr.normalizedForm, m_vocabulary,
                        m_inflectionRules.empty() ? nullptr : &m_inflectionRules,
                        m_compiledConditions.empty() ? nullptr : &m_compiledConditions))
                        vStrip[strip.baseWord].push_back({idx, static_cast<AZ::u16>(vr.variantBits | strip.morphBits)});
                }
            };

            // 2a: non-apostrophe runs from allUnresolvedOriginal (e.g., -eth forms)
            for (AZ::u32 idx : allUnresolvedOriginal)
            {
                if (idx >= originalRunCount) continue;
                if (runs[idx].text.find('\'') == AZStd::string::npos)
                    enqueueVariant(idx);
            }
            // 2b: apostrophe runs that failed LMDB lookup (e.g., -in' g-drop)
            for (AZ::u32 idx : unresolvedApostrophe)
                enqueueVariant(idx);

            if (!vNorm.empty() || !vAlt.empty() || !vStrip.empty())
            {
                AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> vByLen;
                AZStd::unordered_set<AZStd::string> vQueued;

                auto injectSynth = [&](const AZStd::string& word)
                {
                    if (vQueued.count(word)) return;
                    AZ::u32 wlen = static_cast<AZ::u32>(word.size());
                    if (!activeLenSet.count(wlen)) return;
                    CharRun s;
                    s.text = word; s.startPos = 0; s.length = wlen;
                    s.tag = RunTag::Word; s.firstCap = false; s.allCaps = false;
                    vByLen[wlen].push_back(static_cast<AZ::u32>(runs.size()));
                    runs.push_back(s);
                    vQueued.insert(word);
                };
                for (const auto& [w, _] : vNorm)  injectSynth(w);
                for (const auto& [w, _] : vAlt)   injectSynth(w);
                for (const auto& [w, _] : vStrip) injectSynth(w);

                AZStd::vector<ResolutionResult> vResults;
                for (const auto& [wlen, idxs] : vByLen)
                {
                    AZStd::vector<AZ::u32> unused;
                    ResolveLengthCycle(wlen, runs, idxs, vResults, unused);
                }

                // Build resolved lookup by value — safe against manifest.results realloc
                AZStd::unordered_map<AZStd::string, ResolutionResult> vResolved;
                for (const auto& vr : vResults)
                    if (vr.resolved && !vResolved.count(vr.matchedWord))
                        vResolved[vr.matchedWord] = vr;
                // Also check manifest for stripped bases already resolved in main loop
                for (const auto& mr : manifest.results)
                    if (mr.resolved && vStrip.count(mr.matchedWord) && !vResolved.count(mr.matchedWord))
                        vResolved[mr.matchedWord] = mr;

                AZ::u32 varCount = 0;
                auto propagateV = [&](const AZStd::unordered_map<AZStd::string, AZStd::vector<VariantDep>>& deps)
                {
                    for (const auto& [word, depList] : deps)
                    {
                        auto it = vResolved.find(word);
                        if (it == vResolved.end()) continue;
                        for (const auto& dep : depList)
                        {
                            if (variantResolvedSet.count(dep.runIndex)) continue;
                            const CharRun& depRun = runs[dep.runIndex];
                            ResolutionResult r;
                            r.runText = depRun.text;
                            r.resolved = true;
                            r.matchedWord = it->second.matchedWord;
                            r.matchedTokenId = it->second.matchedTokenId;
                            r.tierResolved = it->second.tierResolved;
                            r.morphBits = dep.morphBits;
                            r.runIndex = dep.runIndex;
                            r.firstCap = depRun.firstCap;
                            r.allCaps = depRun.allCaps;
                            manifest.results.push_back(r);
                            variantResolvedSet.insert(dep.runIndex);
                            ++varCount;
                        }
                    }
                };
                propagateV(vNorm);
                propagateV(vAlt);
                propagateV(vStrip);

                if (varCount > 0)
                {
                    fprintf(stderr, "[BedManager] Variant normalization (Stage 2): %u runs resolved\n", varCount);
                    fflush(stderr);
                }
            }
        }

        // ---- Morpheme decomposition for unresolved runs ----
        //
        // DEPRECATED (2026-03-17): Contractions are compound words, not morphemes.
        // "don't" = "do" + "not" (two tokens), NOT "do" + NEG morph bit.
        // MorphBit flags (NEG/WILL/HAVE/BE/AM/COND) are wrong for this purpose.
        // TODO: Replace with broadphase PBD strip pass — apostrophe as broadphase
        // signal, contraction decomposition into constituent word pairs.
        // See memory/morpheme_strip_redesign.md for full design.
        //
        struct MorphemeSuffix { const char* suffix; AZ::u32 len; AZ::u16 morphBits; };
        static const MorphemeSuffix suffixes[] = {
            {"n't", 3, MorphBit::NEG},
            {"'re", 3, MorphBit::BE},
            {"'ve", 3, MorphBit::HAVE},
            {"'ll", 3, MorphBit::WILL},
            {"'s",  2, MorphBit::POSS},
            {"'m",  2, MorphBit::AM},
            {"'d",  2, MorphBit::COND},
        };

        // Combine all unresolved: regular + punctuation that didn't match host lookup.
        // Filter out synthetic runs (index >= originalRunCount) — they're internal
        // base injections, not original input runs.
        AZStd::vector<AZ::u32> allUnresolved;
        for (AZ::u32 idx : allUnresolvedOriginal)
        {
            if (idx < originalRunCount)
                allUnresolved.push_back(idx);
        }
        for (AZ::u32 idx : unresolvedApostrophe)
            allUnresolved.push_back(idx);
        for (AZ::u32 idx : unresolvedHyphen)
            allUnresolved.push_back(idx);

        AZStd::vector<CharRun> decompRuns;
        struct DecompMapping
        {
            AZ::u32 originalRunIndex;
            AZ::u32 decompRunIndex;
            enum Type { ApostropheBase, HyphenCompound, HyphenSegment } type;
            AZ::u32 segmentCount;
            AZ::u32 firstSegmentRun;
            AZ::u16 morphBits = 0;  // Morph bits from contraction stripping
        };
        AZStd::vector<DecompMapping> decompMappings;

        for (AZ::u32 idx : allUnresolved)
        {
            const AZStd::string& text = runs[idx].text;

            if (text.find('\'') != AZStd::string::npos)
            {
                for (const auto& ms : suffixes)
                {
                    if (text.size() > ms.len && text.substr(text.size() - ms.len) == ms.suffix)
                    {
                        AZStd::string base = text.substr(0, text.size() - ms.len);
                        if (base.size() >= 1)
                        {
                            DecompMapping dm;
                            dm.originalRunIndex = idx;
                            dm.decompRunIndex = static_cast<AZ::u32>(decompRuns.size());
                            dm.type = DecompMapping::ApostropheBase;
                            dm.segmentCount = 0;
                            dm.firstSegmentRun = 0;
                            dm.morphBits = ms.morphBits;
                            decompMappings.push_back(dm);

                            CharRun cr;
                            cr.text = base;
                            cr.startPos = 0;
                            cr.length = static_cast<AZ::u32>(base.size());
                            cr.tag = RunTag::Word;
                            decompRuns.push_back(cr);
                        }
                        break;
                    }
                }
            }

            if (text.find('-') != AZStd::string::npos)
            {
                AZStd::string compound;
                compound.reserve(text.size());
                for (char ch : text)
                {
                    if (ch != '-') compound += ch;
                }
                if (compound.size() >= 2)
                {
                    DecompMapping dm;
                    dm.originalRunIndex = idx;
                    dm.decompRunIndex = static_cast<AZ::u32>(decompRuns.size());
                    dm.type = DecompMapping::HyphenCompound;
                    dm.segmentCount = 0;
                    dm.firstSegmentRun = 0;
                    decompMappings.push_back(dm);

                    CharRun cr;
                    cr.text = compound;
                    cr.startPos = 0;
                    cr.length = static_cast<AZ::u32>(compound.size());
                    cr.tag = RunTag::Word;
                    decompRuns.push_back(cr);
                }

                DecompMapping segDm;
                segDm.originalRunIndex = idx;
                segDm.decompRunIndex = 0;
                segDm.type = DecompMapping::HyphenSegment;
                segDm.firstSegmentRun = static_cast<AZ::u32>(decompRuns.size());
                segDm.segmentCount = 0;

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
                                cr.tag = RunTag::Word;
                                decompRuns.push_back(cr);
                                ++segDm.segmentCount;
                            }
                        }
                        segStart = j + 1;
                    }
                }
                if (segDm.segmentCount > 0)
                    decompMappings.push_back(segDm);
            }
        }

        // ---- Resolve decomposed runs through tier-cascaded length cycles ----
        if (!decompRuns.empty())
        {
            AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> decompByLen;
            for (AZ::u32 i = 0; i < static_cast<AZ::u32>(decompRuns.size()); ++i)
            {
                AZ::u32 len = decompRuns[i].length;
                if (decompRuns[i].text.empty()) continue;
                if (activeLenSet.count(len))
                    decompByLen[len].push_back(i);
            }

            AZStd::vector<ResolutionResult> decompResults;
            AZStd::vector<AZ::u32> allDecompUnresolved;
            for (const auto& [len, indices] : decompByLen)
            {
                AZStd::vector<AZ::u32> decompUnresolved;
                ResolveLengthCycle(len, decompRuns, indices,
                                   decompResults, decompUnresolved);
                for (AZ::u32 i : decompUnresolved)
                    allDecompUnresolved.push_back(i);
            }

            // ---- Length-1 bases: direct LMDB lookup (bypasses activeLenSet / beds) ----
            // Handles "i'll" → base "i", "i'm" → "i", "i've" → "i".
            // "I" is stored uppercase in LMDB (migration 017), so try lowercase first,
            // then capitalized fallback — same logic as SingleChar resolution above.
            if (m_vocabulary)
            {
                for (AZ::u32 i = 0; i < static_cast<AZ::u32>(decompRuns.size()); ++i)
                {
                    if (decompRuns[i].length != 1 || decompRuns[i].text.empty()) continue;
                    const AZStd::string& baseText = decompRuns[i].text;
                    AZStd::string tokenId = m_vocabulary->LookupWordLocal(baseText);
                    if (tokenId.empty())
                    {
                        AZStd::string cap = baseText;
                        cap[0] = static_cast<char>(toupper(static_cast<unsigned char>(cap[0])));
                        tokenId = m_vocabulary->LookupWordLocal(cap);
                    }
                    if (!tokenId.empty())
                    {
                        ResolutionResult r;
                        r.resolved = true;
                        r.runText = baseText;
                        r.matchedWord = baseText;
                        r.matchedTokenId = tokenId;
                        r.tierResolved = 0;
                        r.morphBits = 0;
                        r.runIndex = i;
                        r.firstCap = false;
                        r.allCaps = false;
                        decompResults.push_back(r);
                    }
                }
            }

            // ---- Stage 4: TryVariantNormalize on unresolved decomp bases ----
            // Handles e.g. "darlin's" → contraction strips 's → base "darlin'" unresolved
            // → Stage 4 applies V-1 → "darling".  Same logic as Stage 2, applied to
            // the decompRuns sub-array.  Results pushed into decompResults so the
            // existing decompResolved mapping loop picks them up.
            if (!allDecompUnresolved.empty())
            {
                struct DecompVDep { AZ::u32 decompRunIdx; AZ::u16 morphBits; };
                AZStd::unordered_map<AZStd::string, AZStd::vector<DecompVDep>> dvNorm;
                AZStd::unordered_map<AZStd::string, AZStd::vector<DecompVDep>> dvAlt;
                AZStd::unordered_map<AZStd::string, AZStd::vector<DecompVDep>> dvStrip;

                for (AZ::u32 di : allDecompUnresolved)
                {
                    VariantNormalResult vr = TryVariantNormalize(decompRuns[di].text);
                    if (!vr.normalized) continue;
                    dvNorm[vr.normalizedForm].push_back({di, vr.variantBits});
                    if (!vr.altForm.empty())
                        dvAlt[vr.altForm].push_back({di, vr.variantBits});
                    if (vr.variantBits & MorphBit::VARIANT_DIALECT)
                    {
                        for (const auto& strip : TryInflectionStrip(vr.normalizedForm, m_vocabulary))
                            dvStrip[strip.baseWord].push_back({di, static_cast<AZ::u16>(vr.variantBits | strip.morphBits)});
                    }
                }

                if (!dvNorm.empty() || !dvAlt.empty() || !dvStrip.empty())
                {
                    AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> dvByLen;
                    AZStd::unordered_set<AZStd::string> dvQueued;

                    auto injectDv = [&](const AZStd::string& word)
                    {
                        if (dvQueued.count(word)) return;
                        AZ::u32 wlen = static_cast<AZ::u32>(word.size());
                        if (!activeLenSet.count(wlen)) return;
                        CharRun s;
                        s.text = word; s.startPos = 0; s.length = wlen;
                        s.tag = RunTag::Word; s.firstCap = false; s.allCaps = false;
                        dvByLen[wlen].push_back(static_cast<AZ::u32>(runs.size()));
                        runs.push_back(s);
                        dvQueued.insert(word);
                    };
                    for (const auto& [w, _] : dvNorm)  injectDv(w);
                    for (const auto& [w, _] : dvAlt)   injectDv(w);
                    for (const auto& [w, _] : dvStrip) injectDv(w);

                    AZStd::vector<ResolutionResult> dvResults;
                    for (const auto& [wlen, idxs] : dvByLen)
                    {
                        AZStd::vector<AZ::u32> unused;
                        ResolveLengthCycle(wlen, runs, idxs, dvResults, unused);
                    }

                    AZStd::unordered_map<AZStd::string, ResolutionResult> dvResolved;
                    for (const auto& dr : dvResults)
                        if (dr.resolved && !dvResolved.count(dr.matchedWord))
                            dvResolved[dr.matchedWord] = dr;
                    for (const auto& dr : decompResults)
                        if (dr.resolved && dvStrip.count(dr.matchedWord) && !dvResolved.count(dr.matchedWord))
                            dvResolved[dr.matchedWord] = dr;

                    AZ::u32 dvCount = 0;
                    auto propagateDv = [&](const AZStd::unordered_map<AZStd::string, AZStd::vector<DecompVDep>>& deps)
                    {
                        for (const auto& [word, depList] : deps)
                        {
                            auto it = dvResolved.find(word);
                            if (it == dvResolved.end()) continue;
                            for (const auto& dep : depList)
                            {
                                // Skip if already resolved for this decompRun
                                bool done = false;
                                for (const auto& ex : decompResults)
                                    if (ex.runIndex == dep.decompRunIdx && ex.resolved)
                                    { done = true; break; }
                                if (done) continue;

                                ResolutionResult r = it->second;
                                r.runText = decompRuns[dep.decompRunIdx].text;
                                r.morphBits = dep.morphBits;
                                r.runIndex = dep.decompRunIdx;
                                r.firstCap = false;
                                r.allCaps = false;
                                decompResults.push_back(r);
                                ++dvCount;
                            }
                        }
                    };
                    propagateDv(dvNorm);
                    propagateDv(dvAlt);
                    propagateDv(dvStrip);

                    if (dvCount > 0)
                    {
                        fprintf(stderr, "[BedManager] Variant normalization (Stage 4): %u decomp runs resolved\n", dvCount);
                        fflush(stderr);
                    }
                }
            }

            // Map decomp results back to originals
            AZStd::vector<bool> decompResolved(decompRuns.size(), false);
            AZStd::vector<ResolutionResult> decompResultsByIndex(decompRuns.size());
            for (const auto& dr : decompResults)
            {
                for (AZ::u32 i = 0; i < static_cast<AZ::u32>(decompRuns.size()); ++i)
                {
                    if (!decompResolved[i] && decompRuns[i].text == dr.runText)
                    {
                        decompResolved[i] = dr.resolved;
                        decompResultsByIndex[i] = dr;
                        break;
                    }
                }
            }

            AZStd::unordered_map<AZ::u32, bool> originalResolvedViaDecomp;
            AZ::u32 decompMapped = 0;

            for (const auto& dm : decompMappings)
            {
                if (originalResolvedViaDecomp.count(dm.originalRunIndex)) continue;

                if (dm.type == DecompMapping::ApostropheBase ||
                    dm.type == DecompMapping::HyphenCompound)
                {
                    if (decompResolved[dm.decompRunIndex])
                    {
                        const auto& dr = decompResultsByIndex[dm.decompRunIndex];
                        const CharRun& origRun = runs[dm.originalRunIndex];
                        ResolutionResult r;
                        r.runText = origRun.text;
                        r.resolved = true;
                        r.matchedWord = dr.matchedWord;
                        r.matchedTokenId = dr.matchedTokenId;
                        r.tierResolved = dr.tierResolved;
                        r.morphBits = dr.morphBits | dm.morphBits;  // Base morph + contraction morph
                        r.runIndex = dm.originalRunIndex;
                        r.firstCap = origRun.firstCap;
                        r.allCaps = origRun.allCaps;
                        manifest.results.push_back(r);
                        originalResolvedViaDecomp[dm.originalRunIndex] = true;
                        ++decompMapped;
                    }
                }
                else if (dm.type == DecompMapping::HyphenSegment)
                {
                    bool allResolved = true;
                    for (AZ::u32 s = 0; s < dm.segmentCount; ++s)
                    {
                        AZ::u32 segIdx = dm.firstSegmentRun + s;
                        if (segIdx >= decompResolved.size() || !decompResolved[segIdx])
                        { allResolved = false; break; }
                    }
                    if (allResolved && dm.segmentCount > 0)
                    {
                        AZ::u32 firstSeg = dm.firstSegmentRun;
                        const CharRun& origRun = runs[dm.originalRunIndex];
                        ResolutionResult r;
                        r.runText = origRun.text;
                        r.resolved = true;
                        r.matchedWord = decompResultsByIndex[firstSeg].matchedWord;
                        r.matchedTokenId = decompResultsByIndex[firstSeg].matchedTokenId;
                        r.tierResolved = decompResultsByIndex[firstSeg].tierResolved;
                        r.runIndex = dm.originalRunIndex;
                        r.firstCap = origRun.firstCap;
                        r.allCaps = origRun.allCaps;
                        manifest.results.push_back(r);
                        originalResolvedViaDecomp[dm.originalRunIndex] = true;
                        ++decompMapped;
                    }
                }
            }

            for (AZ::u32 idx : allUnresolved)
            {
                if (!originalResolvedViaDecomp.count(idx) && !variantResolvedSet.count(idx))
                {
                    ResolutionResult r;
                    r.runText = runs[idx].text;
                    r.resolved = false;
                    r.runIndex = idx;
                    r.firstCap = runs[idx].firstCap;
                    r.allCaps = runs[idx].allCaps;
                    manifest.results.push_back(r);
                }
            }

            if (decompMapped > 0)
            {
                fprintf(stderr, "[BedManager] Decomposed bases resolved: %u / %zu mappings\n",
                    decompMapped, decompMappings.size());
                fflush(stderr);
            }
        }
        else
        {
            for (AZ::u32 idx : allUnresolved)
            {
                if (!variantResolvedSet.count(idx))
                {
                    ResolutionResult r;
                    r.runText = runs[idx].text;
                    r.resolved = false;
                    r.runIndex = idx;
                    r.firstCap = runs[idx].firstCap;
                    r.allCaps = runs[idx].allCaps;
                    manifest.results.push_back(r);
                }
            }
        }

        // ---- No-vocab runs as unresolved ----
        for (AZ::u32 idx : noVocabRuns)
        {
            ResolutionResult r;
            r.runText = runs[idx].text;
            r.resolved = false;
            r.runIndex = idx;
            r.firstCap = runs[idx].firstCap;
            r.allCaps = runs[idx].allCaps;
            manifest.results.push_back(r);
        }

        // ---- Propagate results to stacked duplicates ----
        // Store copies (not pointers) — push_back below can reallocate manifest.results,
        // which would invalidate any raw pointers into it.
        AZStd::unordered_map<AZStd::string, ResolutionResult> resolvedLookup;
        for (const auto& r : manifest.results)
        {
            if (r.resolved)
                resolvedLookup[r.runText] = r;
        }

        for (const auto& [firstIdx, dupes] : runStacks)
        {
            const AZStd::string& text = runs[firstIdx].text;
            auto it = resolvedLookup.find(text);
            for (AZ::u32 di = 0; di < static_cast<AZ::u32>(dupes.size()); ++di)
            {
                const CharRun& dupeRun = runs[dupes[di]];
                ResolutionResult r;
                r.runText = text;
                r.runIndex = dupes[di];
                r.firstCap = dupeRun.firstCap;
                r.allCaps = dupeRun.allCaps;
                if (it != resolvedLookup.end())
                {
                    r.resolved = it->second.resolved;
                    r.matchedWord = it->second.matchedWord;
                    r.matchedTokenId = it->second.matchedTokenId;
                    r.tierResolved = it->second.tierResolved;
                    r.morphBits = it->second.morphBits;
                }
                else
                {
                    r.resolved = false;
                }
                manifest.results.push_back(r);
            }
        }

        // ---- Sort by runIndex: restore document order (the train manifest) ----
        AZStd::sort(manifest.results.begin(), manifest.results.end(),
            [](const ResolutionResult& a, const ResolutionResult& b) {
                return a.runIndex < b.runIndex;
            });

        // ---- Count ----
        for (const auto& r : manifest.results)
        {
            if (r.resolved) manifest.resolvedRuns++;
            else manifest.unresolvedRuns++;
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
        for (auto& ws : m_primaryWorkspaces)
            ws.Shutdown();
        for (auto& ws : m_extendedWorkspaces)
            ws.Shutdown();
        m_primaryWorkspaces.clear();
        m_extendedWorkspaces.clear();
        m_vocabByLength.clear();
        m_vocabDbiOpen = false;
        m_activeWordLengths.clear();
        m_initialized = false;
        m_vocabulary = nullptr;
    }

    // ========================================================================
    // ScanManifestToPBM — the scanner
    //
    // The manifest (sorted by runIndex) is the train manifest. Each token
    // passes the scanner exactly once. As it passes:
    //   1. Record position + modifier (morph bits + cap flags)
    //   2. Pair with previous token → tally bond (A, B, count)
    // By the time the last token flows past, bonds + positions are complete.
    // Zero extra passes.
    // ========================================================================

    ManifestScanResult ScanManifestToPBM(const ResolutionManifest& manifest)
    {
        ManifestScanResult out;
        if (manifest.results.empty()) return out;

        // Var token prefix (unresolved tokens become vars)
        static constexpr const char* SCAN_VAR_PREFIX = "AA.AE.AF.AA.AC";

        // Bond accumulator: "tokenA|tokenB" → count
        AZStd::unordered_map<AZStd::string, int> bondCounts;
        AZStd::unordered_set<AZStd::string> uniqueTokenSet;

        AZStd::string prevTokenId;
        int position = 0;

        for (const auto& r : manifest.results)
        {
            // Determine token ID: resolved tokens use matchedTokenId,
            // unresolved become vars (VAR_PREFIX + surface text)
            AZStd::string tokenId;
            if (r.resolved)
            {
                tokenId = r.matchedTokenId;
            }
            else
            {
                tokenId = AZStd::string(SCAN_VAR_PREFIX) + " " + r.runText;
            }

            // Diagnostic: catch empty tokenIds before they route to '....' row
            if (tokenId.empty())
            {
                static int emptyWarnings = 0;
                if (emptyWarnings < 10)
                {
                    fprintf(stderr, "[ScanManifest] EMPTY tokenId: resolved=%d runText='%s' "
                        "matchedWord='%s' runIndex=%u\n",
                        (int)r.resolved, r.runText.c_str(),
                        r.matchedWord.c_str(), r.runIndex);
                    fflush(stderr);
                    ++emptyWarnings;
                }
            }

            uniqueTokenSet.insert(tokenId);

            // Record sparse morpheme/cap overlays — separate lists per morpheme.
            // Cap flags
            if (r.firstCap) out.morphemePositions["FIRST_CAP"].push_back(position);
            if (r.allCaps)  out.morphemePositions["ALL_CAPS"].push_back(position);
            // Morpheme bits — one entry per set bit
            if (r.morphBits != 0)
            {
                static const struct { AZ::u16 bit; const char* name; } kBitNames[] = {
                    { MorphBit::PLURAL,  "PLURAL"  },
                    { MorphBit::POSS,    "POSS"    },
                    { MorphBit::POSS_PL, "POSS_PL" },
                    { MorphBit::PAST,    "PAST"    },
                    { MorphBit::PROG,    "PROG"    },
                    { MorphBit::THIRD,   "3RD"     },
                    { MorphBit::NEG,     "NEG"     },
                    { MorphBit::COND,    "COND"    },
                    { MorphBit::WILL,    "WILL"    },
                    { MorphBit::HAVE,    "HAVE"    },
                    { MorphBit::BE,      "BE"      },
                    { MorphBit::AM,      "AM"      },
                };
                for (const auto& bn : kBitNames)
                    if (r.morphBits & bn.bit)
                        out.morphemePositions[bn.name].push_back(position);
            }

            out.tokenIds.push_back(tokenId);
            out.positions.push_back(position);
            out.entityIds.push_back(r.entityId);
            if (!r.entityId.empty()) ++out.entityAnnotations;

            // Bond: pair with previous token (scanner tallies as they pass)
            if (!prevTokenId.empty())
            {
                AZStd::string key = prevTokenId + "|" + tokenId;
                bondCounts[key]++;
                out.totalPairs++;
            }
            else
            {
                // First token = first FPB A-side
                out.firstFpbA = tokenId;
            }

            // Second token = first FPB B-side
            if (position == 1)
                out.firstFpbB = tokenId;

            prevTokenId = tokenId;
            ++position;
        }

        out.totalSlots = position;
        out.uniqueTokens = uniqueTokenSet.size();

        // Convert bond map to Bond vector
        out.bonds.reserve(bondCounts.size());
        for (auto& [key, count] : bondCounts)
        {
            size_t sep = key.find('|');
            Bond bond;
            bond.tokenA = AZStd::string(key.data(), sep);
            bond.tokenB = AZStd::string(key.data() + sep + 1, key.size() - sep - 1);
            bond.count = count;
            out.bonds.push_back(AZStd::move(bond));
        }

        size_t emptyTotal = 0;
        for (const auto& tid : out.tokenIds) if (tid.empty()) ++emptyTotal;
        fprintf(stderr, "[ScanManifest] %zu tokens (%zu empty), %zu unique, %zu bond types, %zu total pairs\n",
            out.tokenIds.size(), emptyTotal, out.uniqueTokens, out.bonds.size(), out.totalPairs);
        fflush(stderr);

        return out;
    }

} // namespace HCPEngine
