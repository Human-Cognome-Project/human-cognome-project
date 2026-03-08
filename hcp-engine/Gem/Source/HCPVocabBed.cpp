#include "HCPVocabBed.h"
#include "HCPVocabulary.h"

#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_set.h>
#include <lmdb.h>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cstring>

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
        if (!m_particleBuffer || !m_cuda) return 0;
        if (pack.totalVocabParticles == 0 || pack.totalVocabParticles > m_bufferCapacity) return 0;

        m_vocabParticleCount = pack.totalVocabParticles;
        m_currentWordLength = wordLength;

        // Compute stream capacity from remaining buffer
        AZ::u32 remainingCapacity = m_bufferCapacity - m_vocabParticleCount;
        m_maxStreamSlots = remainingCapacity / wordLength;
        if (m_maxStreamSlots < 1) m_maxStreamSlots = 1;

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

        // Only vocab active, no stream yet
        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

        m_streamSlots.clear();

        return m_maxStreamSlots;
    }

    AZ::u32 Workspace::LoadStreamRuns(
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<AZ::u32>& indices,
        AZ::u32 wordLength)
    {
        if (!m_particleBuffer || !m_cuda || indices.empty()) return 0;

        m_streamSlots.clear();
        AZ::u32 overflowCount = 0;
        AZ::u32 streamPhase = m_tierPhases.empty() ? m_inertPhase : m_tierPhases[0];
        AZ::u32 maxDynParticles = m_maxStreamSlots * wordLength;

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

        // Only activate the particles we actually loaded, not the full stream capacity.
        // slotIdx = number of runs loaded; each run = wordLength particles.
        AZ::u32 actualDynParticles = slotIdx * wordLength;
        m_particleBuffer->setNbActiveParticles(m_vocabParticleCount + actualDynParticles);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        m_particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

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
                }
            }
        }

        {
            physx::PxScopedCudaLock lock(*m_cuda);
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
        if (m_activeInScene || !m_particleSystem || !m_scene) return;
        m_scene->addActor(*m_particleSystem);
        m_activeInScene = true;
    }

    void Workspace::DeactivateFromScene()
    {
        if (!m_activeInScene || !m_particleSystem || !m_scene) return;
        m_scene->removeActor(*m_particleSystem);
        m_activeInScene = false;
    }

    void Workspace::BeginSimulate(int steps, float dt)
    {
        if (!m_scene) return;
        m_pendingSteps = steps;
        m_simDt = dt;

        // Kick off the first step — simulate() dispatches to GPU and returns
        if (m_pendingSteps > 0)
        {
            m_scene->simulate(m_simDt);
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
        if (!m_scene) return;

        // Complete the in-flight step
        m_scene->fetchResults(true);

        // Run remaining steps synchronously (each step must complete before next)
        while (m_pendingSteps > 0)
        {
            m_scene->simulate(m_simDt);
            m_scene->fetchResults(true);
            --m_pendingSteps;
        }

        m_scene->fetchResultsParticleSystem();
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
    // Each phase loads a small slice (RC_VOCAB_PER_PHASE entries) as static
    // particles, leaving maximum buffer space for stream runs. Phases cycle
    // until all runs resolve or vocab is exhausted. Early exit on full resolution.
    // ========================================================================

    VocabPack BedManager::BuildVocabSlice(AZ::u32 wordLength, AZ::u32 startEntry, AZ::u32 count) const
    {
        VocabPack pack;
        pack.wordLength = wordLength;
        pack.maxTierCount = 1;

        auto it = m_vocabByLength.find(wordLength);
        if (it == m_vocabByLength.end()) return pack;

        const auto& allEntries = it->second;
        if (startEntry >= static_cast<AZ::u32>(allEntries.size())) return pack;

        AZ::u32 endEntry = startEntry + count;
        if (endEntry > static_cast<AZ::u32>(allEntries.size()))
            endEntry = static_cast<AZ::u32>(allEntries.size());

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
            const auto& entry = allEntries[i];
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

    static InflectionStripResult TryInflectionStrip(
        const AZStd::string& word)
    {
        InflectionStripResult result;
        if (word.size() < 4) return result;  // Too short to strip meaningfully

        auto acceptBase = [&](const AZStd::string& base, AZ::u16 bits) -> bool
        {
            if (base.size() < 2) return false;
            result.baseWord = base;
            result.morphBits = bits;
            result.stripped = true;
            return true;
        };

        const size_t len = word.size();

        // ---- -ies → -y (e.g. "parties" → "party") ----
        if (len >= 4 && word.substr(len - 3) == "ies")
        {
            if (acceptBase(word.substr(0, len - 3) + "y", MorphBit::PLURAL | MorphBit::THIRD))
                return result;
        }

        // ---- -ves → -f / -fe (e.g. "wolves" → "wolf", "knives" → "knife") ----
        if (len >= 4 && word.substr(len - 3) == "ves")
        {
            if (acceptBase(word.substr(0, len - 3) + "f", MorphBit::PLURAL))
                return result;
            if (acceptBase(word.substr(0, len - 3) + "fe", MorphBit::PLURAL))
                return result;
        }

        // ---- -ied → -y (e.g. "carried" → "carry") ----
        if (len >= 4 && word.substr(len - 3) == "ied")
        {
            if (acceptBase(word.substr(0, len - 3) + "y", MorphBit::PAST))
                return result;
        }

        // ---- Doubled consonant + -ing (e.g. "running" → "run") ----
        if (len >= 6 && word.substr(len - 3) == "ing")
        {
            AZStd::string stem = word.substr(0, len - 3);
            if (stem.size() >= 3 && stem[stem.size()-1] == stem[stem.size()-2] && IsConsonant(stem.back()))
            {
                if (acceptBase(stem.substr(0, stem.size() - 1), MorphBit::PROG))
                    return result;
            }
        }

        // ---- Doubled consonant + -ed (e.g. "stopped" → "stop") ----
        if (len >= 5 && word.substr(len - 2) == "ed")
        {
            AZStd::string stem = word.substr(0, len - 2);
            if (stem.size() >= 3 && stem[stem.size()-1] == stem[stem.size()-2] && IsConsonant(stem.back()))
            {
                if (acceptBase(stem.substr(0, stem.size() - 1), MorphBit::PAST))
                    return result;
            }
        }

        // ---- -ing (try base, try base+e) ----
        if (len >= 5 && word.substr(len - 3) == "ing")
        {
            AZStd::string base = word.substr(0, len - 3);
            if (acceptBase(base, MorphBit::PROG))
                return result;
            if (acceptBase(base + "e", MorphBit::PROG))
                return result;
        }

        // ---- -ed (try base, try base+e) ----
        if (len >= 4 && word.substr(len - 2) == "ed")
        {
            AZStd::string base = word.substr(0, len - 2);
            if (acceptBase(base, MorphBit::PAST))
                return result;
            if (acceptBase(base + "e", MorphBit::PAST))
                return result;
        }

        // ---- -er (comparative: e.g. "bigger" → "big", "taller" → "tall") ----
        if (len >= 4 && word.substr(len - 2) == "er")
        {
            AZStd::string base = word.substr(0, len - 2);
            // Doubled consonant
            if (base.size() >= 3 && base[base.size()-1] == base[base.size()-2] && IsConsonant(base.back()))
            {
                if (acceptBase(base.substr(0, base.size() - 1), 0))
                    return result;
            }
            if (acceptBase(base, 0))
                return result;
            if (acceptBase(base + "e", 0))
                return result;
        }

        // ---- -est (superlative) ----
        if (len >= 5 && word.substr(len - 3) == "est")
        {
            AZStd::string base = word.substr(0, len - 3);
            if (base.size() >= 3 && base[base.size()-1] == base[base.size()-2] && IsConsonant(base.back()))
            {
                if (acceptBase(base.substr(0, base.size() - 1), 0))
                    return result;
            }
            if (acceptBase(base, 0))
                return result;
            if (acceptBase(base + "e", 0))
                return result;
        }

        // ---- -es (e.g. "boxes" → "box", "watches" → "watch") ----
        if (len >= 4 && word.substr(len - 2) == "es")
        {
            if (acceptBase(word.substr(0, len - 2), MorphBit::PLURAL | MorphBit::THIRD))
                return result;
            if (acceptBase(word.substr(0, len - 1), MorphBit::PLURAL | MorphBit::THIRD))  // "e" forms
                return result;
        }

        // ---- -s (plural / 3rd person) ----
        if (len >= 4 && word.back() == 's' && word[len-2] != 's')
        {
            if (acceptBase(word.substr(0, len - 1), MorphBit::PLURAL | MorphBit::THIRD))
                return result;
        }

        // ---- -ily → -y (adverb from y-stem: e.g. "daily" → "day", "happily" → "happy") ----
        if (len >= 5 && word.substr(len - 3) == "ily")
        {
            if (acceptBase(word.substr(0, len - 3) + "y", 0))
                return result;
        }

        // ---- -ly (adverb) ----
        if (len >= 5 && word.substr(len - 2) == "ly")
        {
            if (acceptBase(word.substr(0, len - 2), 0))
                return result;
        }

        // ---- -ness ----
        if (len >= 6 && word.substr(len - 4) == "ness")
        {
            if (acceptBase(word.substr(0, len - 4), 0))
                return result;
        }

        return result;  // No strip found
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

    bool BedManager::Initialize(
        physx::PxPhysics* physics,
        physx::PxCudaContextManager* cuda,
        MDB_env* lmdbEnv,
        HCPVocabulary* vocabulary)
    {
        if (!physics || !cuda || !lmdbEnv) return false;

        m_physics = physics;
        m_cuda = cuda;
        m_vocabulary = vocabulary;

        auto t0 = std::chrono::high_resolution_clock::now();

        // Open vbed sub-databases and read vocab entries.
        // Write txn to persist DBI handles, then read data in same txn.
        MDB_txn* txn;
        int rc = mdb_txn_begin(lmdbEnv, nullptr, 0, &txn);
        if (rc != 0)
        {
            fprintf(stderr, "[BedManager] mdb_txn_begin: %s\n", mdb_strerror(rc));
            return false;
        }

        AZ::u32 totalEntries = 0;
        AZ::u32 totalLabels = 0;

        for (int wlen = VBED_MIN_LEN; wlen <= VBED_MAX_LEN; ++wlen)
        {
            char dataName[16], metaName[24];
            snprintf(dataName, sizeof(dataName), "vbed_%02d", wlen);
            snprintf(metaName, sizeof(metaName), "vbed_%02d_meta", wlen);

            MDB_dbi dataDbi, metaDbi;
            rc = mdb_dbi_open(txn, dataName, MDB_CREATE, &dataDbi);
            if (rc != 0)
            {
                fprintf(stderr, "[BedManager] Skip %s: %s\n", dataName, mdb_strerror(rc));
                continue;
            }
            rc = mdb_dbi_open(txn, metaName, MDB_CREATE, &metaDbi);
            if (rc != 0) continue;

            // Read metadata
            MDB_val key, val;
            key.mv_data = const_cast<char*>("meta");
            key.mv_size = 4;
            rc = mdb_get(txn, metaDbi, &key, &val);
            if (rc != 0 || val.mv_size < sizeof(VBedMeta))
            {
                if (rc != MDB_NOTFOUND)
                    fprintf(stderr, "[BedManager] %s meta read: %s\n", metaName, mdb_strerror(rc));
                continue;
            }

            VBedMeta meta;
            memcpy(&meta, val.mv_data, sizeof(VBedMeta));

            if (meta.total_entries == 0) continue;

            // Read entry buffer (zero-copy pointer into mmap'd region)
            key.mv_data = const_cast<char*>("data");
            key.mv_size = 4;
            rc = mdb_get(txn, dataDbi, &key, &val);
            if (rc != 0)
            {
                if (rc != MDB_NOTFOUND)
                    fprintf(stderr, "[BedManager] %s data read: %s\n", dataName, mdb_strerror(rc));
                continue;
            }

            AZ::u32 entrySize = static_cast<AZ::u32>(wlen) + VBED_TOKEN_ID_WIDTH;
            AZ::u32 expectedBytes = meta.total_entries * entrySize;
            if (val.mv_size < expectedBytes)
            {
                fprintf(stderr, "[BedManager] %s: buffer too small (%zu < %u)\n",
                    dataName, val.mv_size, expectedBytes);
                continue;
            }

            const uint8_t* buf = static_cast<const uint8_t*>(val.mv_data);

            // Parse fixed-width entries directly into frequency-ordered list
            AZStd::vector<VocabPack::Entry> entries;
            entries.reserve(meta.total_entries);

            for (AZ::u32 i = 0; i < meta.total_entries; ++i)
            {
                AZ::u32 offset = i * entrySize;

                // Word: wlen bytes, null-padded
                const char* wordPtr = reinterpret_cast<const char*>(buf + offset);
                AZ::u32 actualWordLen = static_cast<AZ::u32>(wlen);
                while (actualWordLen > 0 && wordPtr[actualWordLen - 1] == '\0')
                    --actualWordLen;

                // Token ID: 14 bytes, null-padded
                const char* tidPtr = reinterpret_cast<const char*>(buf + offset + wlen);
                AZ::u32 tidLen = VBED_TOKEN_ID_WIDTH;
                while (tidLen > 0 && tidPtr[tidLen - 1] == '\0')
                    --tidLen;

                VocabPack::Entry entry;
                entry.word = AZStd::string(wordPtr, actualWordLen);
                entry.tokenId = AZStd::string(tidPtr, tidLen);
                entry.tierIndex = 0;  // Phase-based: position in list IS the priority
                entries.push_back(AZStd::move(entry));
            }

            totalEntries += meta.total_entries;
            totalLabels += meta.label_count;
            m_activeWordLengths.push_back(static_cast<AZ::u32>(wlen));
            m_labelCountByLength[static_cast<AZ::u32>(wlen)] = meta.label_count;
            m_vocabByLength[static_cast<AZ::u32>(wlen)] = AZStd::move(entries);

            fprintf(stderr, "[BedManager] vbed_%02d: %u entries (labels=%u, ranked=%u, unranked=%u)\n",
                wlen, meta.total_entries, meta.label_count,
                meta.tier1_end - meta.label_count,
                meta.total_entries - meta.tier1_end);
        }

        mdb_txn_commit(txn);

        // Sort lengths descending (resolve longest words first)
        AZStd::sort(m_activeWordLengths.begin(), m_activeWordLengths.end(),
            [](AZ::u32 a, AZ::u32 b) { return a > b; });

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

        fprintf(stderr, "[BedManager] LMDB initialized: %zu lengths, %u entries (%u labels), "
            "%u entries/phase, %u workspaces, %.1f ms\n",
            m_vocabByLength.size(), totalEntries, totalLabels,
            RC_VOCAB_PER_PHASE, createdCount, ms);
        fprintf(stderr, "[BedManager] Word lengths (descending):");
        for (AZ::u32 len : m_activeWordLengths)
            fprintf(stderr, " %u", len);
        fprintf(stderr, "\n");
        fflush(stderr);

        return true;
    }

    // ---- ResolvePhase: load one small vocab slice, simulate, collect ----
    //
    // Small vocab (RC_VOCAB_PER_PHASE entries) → maximum stream slots.
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
    // Phase 0 = entries [0..N), phase 1 = [N..2N), etc. N = RC_VOCAB_PER_PHASE.
    // Each phase: build tiny VocabPack on the fly, load all remaining runs,
    // simulate, collect resolved, pass unresolved to next phase.
    // Early exit when all runs resolved.

    // Build a VocabPack from a pre-filtered entry vector (not the full per-length list).
    // Used by ResolveLengthCycle after first-letter filtering.
    VocabPack BedManager::BuildVocabSliceFromEntries(
        AZ::u32 wordLength,
        const AZStd::vector<VocabPack::Entry>& entries,
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

    // Helper: filter vocab entries by first-letter set
    static AZStd::vector<VocabPack::Entry> FilterByFirstChar(
        const AZStd::vector<VocabPack::Entry>& entries,
        const AZStd::unordered_set<char>& neededChars)
    {
        AZStd::vector<VocabPack::Entry> filtered;
        filtered.reserve(entries.size() / 8);
        for (const auto& entry : entries)
        {
            if (!entry.word.empty() && neededChars.count(entry.word[0]))
                filtered.push_back(entry);
        }
        return filtered;
    }

    // Helper: run phase cascade over a filtered vocab list
    void BedManager::RunPhaseCascade(
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<VocabPack::Entry>& filteredVocab,
        AZStd::vector<AZ::u32>& currentIndices,
        AZStd::vector<ResolutionResult>& results,
        AZ::u32& phaseIndex)
    {
        AZ::u32 totalFiltered = static_cast<AZ::u32>(filteredVocab.size());

        for (AZ::u32 start = 0; start < totalFiltered && !currentIndices.empty(); start += RC_VOCAB_PER_PHASE)
        {
            VocabPack phasePack = BuildVocabSliceFromEntries(wordLength, filteredVocab, start, RC_VOCAB_PER_PHASE);
            if (phasePack.vocabEntryCount == 0) continue;

            AZStd::vector<AZ::u32> phaseUnresolved;
            ResolvePhase(wordLength, runs, currentIndices, phasePack, phaseIndex,
                         results, phaseUnresolved);

            currentIndices = AZStd::move(phaseUnresolved);
            ++phaseIndex;
        }
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
    //   - On drain: unresolved runs → new item at vocabStart += RC_VOCAB_PER_PHASE
    //   - On dispatch: leftover (overflow) → re-inserted at queue head (same phase)
    // VocabPack cache:  built once per vocabStart, never rebuilt.
    //   Pre-built during dispatch (after BeginSimulate, before FetchSimResults).

    void BedManager::RunPipelinedCascade(
        AZ::u32 wordLength,
        const AZStd::vector<CharRun>& runs,
        const AZStd::vector<VocabPack::Entry>& filteredVocab,
        AZStd::vector<AZ::u32>& currentIndices,
        AZStd::vector<ResolutionResult>& results,
        AZ::u32& phaseIndex)
    {
        AZ::u32 totalFiltered = static_cast<AZ::u32>(filteredVocab.size());
        if (totalFiltered == 0 || currentIndices.empty()) return;

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
        // Key: vocabStart (= phase * RC_VOCAB_PER_PHASE)
        // Pointers into this map remain stable across emplace (unordered_map guarantee).
        AZStd::unordered_map<AZ::u32, VocabPack> packCache;
        auto getOrBuildPack = [&](AZ::u32 start) -> const VocabPack*
        {
            auto it = packCache.find(start);
            if (it == packCache.end())
            {
                auto ins = packCache.emplace(start,
                    BuildVocabSliceFromEntries(wordLength, filteredVocab, start, RC_VOCAB_PER_PHASE));
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
                    AZ::u32 nextStart = slot.vocabStart + RC_VOCAB_PER_PHASE;
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
                    AZ::u32 nextStart = savedVocabStart + RC_VOCAB_PER_PHASE;
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

        auto it = m_vocabByLength.find(wordLength);
        if (it == m_vocabByLength.end())
        {
            for (AZ::u32 idx : runIndices)
                unresolvedIndices.push_back(idx);
            return;
        }

        const auto& allEntries = it->second;
        AZ::u32 labelCount = 0;
        {
            auto lIt = m_labelCountByLength.find(wordLength);
            if (lIt != m_labelCountByLength.end())
                labelCount = lIt->second;
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

        // ---- Pass 1: Label vocab (entries 0..labelCount) — capitalized runs ONLY ----
        if (!capRuns.empty() && labelCount > 0)
        {
            AZStd::vector<VocabPack::Entry> labelEntries(allEntries.begin(),
                allEntries.begin() + labelCount);

            AZStd::unordered_set<char> capChars;
            for (AZ::u32 idx : capRuns)
            {
                if (!runs[idx].text.empty())
                    capChars.insert(runs[idx].text[0]);
            }
            auto filteredLabels = FilterByFirstChar(labelEntries, capChars);

            if (!filteredLabels.empty())
            {
                fprintf(stderr, "[BedManager] Length %u Label pass: %zu cap runs, %u labels → %zu filtered\n",
                    wordLength, capRuns.size(), labelCount, filteredLabels.size());
                fflush(stderr);

                RunPipelinedCascade(wordLength, runs, filteredLabels, capRuns, results, phaseIndex);
            }

            // Unresolved cap runs fall through to common pass
        }

        // ---- Pass 2: Common vocab (entries labelCount..end) — ALL remaining runs ----
        // Merge unresolved cap runs back with plain runs
        AZStd::vector<AZ::u32> commonRuns;
        commonRuns.reserve(capRuns.size() + plainRuns.size());
        for (AZ::u32 idx : capRuns)
            commonRuns.push_back(idx);
        for (AZ::u32 idx : plainRuns)
            commonRuns.push_back(idx);

        if (!commonRuns.empty())
        {
            AZStd::vector<VocabPack::Entry> commonEntries(
                allEntries.begin() + labelCount, allEntries.end());

            AZStd::unordered_set<char> commonChars;
            for (AZ::u32 idx : commonRuns)
            {
                if (!runs[idx].text.empty())
                    commonChars.insert(runs[idx].text[0]);
            }
            auto filteredCommon = FilterByFirstChar(commonEntries, commonChars);

            fprintf(stderr, "[BedManager] Length %u common pass: %zu runs, %zu vocab → %zu filtered\n",
                wordLength, commonRuns.size(), commonEntries.size(), filteredCommon.size());
            fflush(stderr);

            if (!filteredCommon.empty())
                RunPipelinedCascade(wordLength, runs, filteredCommon, commonRuns, results, phaseIndex);
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
                ResolutionResult r;
                r.runText = run.text;
                r.resolved = true;
                r.matchedWord = run.text;
                r.matchedTokenId = run.preAssignedTokenId;
                r.tierResolved = 0;
                r.runIndex = i;
                r.firstCap = run.firstCap;
                r.allCaps = run.allCaps;
                manifest.results.push_back(r);
                ++preResolved;
            }
            else if (run.tag == RunTag::Numeric)
            {
                ResolutionResult r;
                r.runText = run.text;
                r.resolved = true;
                r.matchedWord = run.text;
                r.matchedTokenId = "NUM";
                r.tierResolved = 0;
                r.runIndex = i;
                manifest.results.push_back(r);
                ++preResolved;
            }
        }

        if (preResolved > 0)
        {
            fprintf(stderr, "[BedManager] Transform pre-resolved: %u runs (SingleChar/Numeric)\n", preResolved);
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
        AZStd::unordered_map<AZ::u32, bool> activeLenSet;
        for (const auto& [len, _] : m_vocabByLength)
            activeLenSet[len] = true;

        // Per-length text sets — O(1) "already queued" checks for interstitial stripping
        AZStd::unordered_map<AZ::u32, AZStd::unordered_set<AZStd::string>> queuedTextByLength;

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
                    queuedTextByLength[len].insert(run.text);
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

        // ---- Descending length resolve loop with interstitial inflection stripping ----
        //
        // Long-first order: "running" (7) processes before "run" (3).
        // After each length cycle, unresolved runs get stripped:
        //   - Strip suffix → base word + morph bits (delta)
        //   - O(1) check: is base already queued at shorter length?
        //     YES → piggyback (just record dependent, PBD resolves it naturally)
        //     NO  → inject synthetic CharRun into shorter-length queue
        //   - If base can't strip → var
        //
        // No LMDB lookups in the interstitial step — PBD IS the resolution
        // mechanism. We only record deltas and let the natural descending
        // flow resolve bases at their shorter length.
        //
        // Post-loop: scan manifest for resolved bases, propagate to dependents.
        // Unresolved bases → dependents become vars.

        struct InflectedDependent
        {
            AZ::u32 runIndex;       // Original inflected run index
            AZ::u16 morphBits;      // Inflection applied (delta)
        };
        AZStd::unordered_map<AZStd::string, AZStd::vector<InflectedDependent>> inflectedDependents;
        AZStd::vector<AZ::u32> allUnresolvedOriginal;
        AZ::u32 inflectionCount = 0;
        AZ::u32 syntheticInjections = 0;

        for (AZ::u32 len : m_activeWordLengths)
        {
            // Fetch indices for this length — may include synthetics injected by earlier (longer) cycles
            auto it = runsByLength.find(len);
            if (it == runsByLength.end() || it->second.empty()) continue;

            AZStd::vector<AZ::u32> indices = it->second;

            AZStd::vector<AZ::u32> unresolvedFromCycle;
            ResolveLengthCycle(len, runs, indices,
                               manifest.results, unresolvedFromCycle);

            // ---- Interstitial: strip unresolved, feed bases to shorter queues ----
            for (AZ::u32 idx : unresolvedFromCycle)
            {
                const CharRun& run = runs[idx];

                // Skip punctuation runs (handled by morpheme decomposition)
                if (run.text.find('\'') != AZStd::string::npos ||
                    run.text.find('-') != AZStd::string::npos)
                {
                    allUnresolvedOriginal.push_back(idx);
                    continue;
                }

                InflectionStripResult strip = TryInflectionStrip(run.text);
                if (!strip.stripped)
                {
                    allUnresolvedOriginal.push_back(idx);
                    continue;
                }

                AZ::u32 baseLen = static_cast<AZ::u32>(strip.baseWord.size());

                // Record the delta: inflected form depends on base
                inflectedDependents[strip.baseWord].push_back({idx, strip.morphBits});
                ++inflectionCount;

                // O(1) check: is base already queued at its shorter length?
                bool alreadyQueued = false;
                if (queuedTextByLength.count(baseLen))
                    alreadyQueued = queuedTextByLength[baseLen].count(strip.baseWord) > 0;

                if (!alreadyQueued && activeLenSet.count(baseLen))
                {
                    // Inject synthetic CharRun for the base into shorter-length PBD queue.
                    // No LMDB lookup — PBD at the base's length IS the existence check.
                    CharRun synth;
                    synth.text = strip.baseWord;
                    synth.startPos = 0;
                    synth.length = baseLen;
                    synth.tag = RunTag::Word;
                    synth.firstCap = false;
                    synth.allCaps = false;

                    AZ::u32 synthIdx = static_cast<AZ::u32>(runs.size());
                    runs.push_back(synth);
                    runsByLength[baseLen].push_back(synthIdx);
                    queuedTextByLength[baseLen].insert(strip.baseWord);
                    ++syntheticInjections;
                }
                // If already queued: base will resolve via PBD at its natural length.
                // Dependents picked up in post-loop scan.
            }

            fprintf(stderr, "[BedManager] Length %u: %zu runs, %zu unresolved\n",
                len, indices.size(), unresolvedFromCycle.size());
            fflush(stderr);
        }

        // ---- Resolve inflected dependents whose bases resolved (via PBD or synthetics) ----
        // Scan all manifest results for base words that have pending dependents.
        // Synthetic runs resolve via PBD → appear in manifest → dependents piggyback.
        // Failed bases collected for silent-e fallback (e.g. "resolv" → "resolve").
        AZStd::unordered_map<AZStd::string, AZStd::vector<InflectedDependent>*> silentECandidates;
        {
            AZStd::unordered_map<AZStd::string, const ResolutionResult*> resolvedBases;
            for (const auto& r : manifest.results)
            {
                if (r.resolved && inflectedDependents.count(r.matchedWord))
                    resolvedBases[r.matchedWord] = &r;
            }

            AZ::u32 depResolved = 0;
            for (auto& [baseWord, deps] : inflectedDependents)
            {
                auto bit = resolvedBases.find(baseWord);
                if (bit != resolvedBases.end())
                {
                    for (auto& dep : deps)
                    {
                        const CharRun& depRun = runs[dep.runIndex];
                        ResolutionResult r;
                        r.runText = depRun.text;
                        r.resolved = true;
                        r.matchedWord = bit->second->matchedWord;
                        r.matchedTokenId = bit->second->matchedTokenId;
                        r.morphBits = dep.morphBits;
                        r.tierResolved = bit->second->tierResolved;
                        r.runIndex = dep.runIndex;
                        r.firstCap = depRun.firstCap;
                        r.allCaps = depRun.allCaps;
                        manifest.results.push_back(r);
                        ++depResolved;
                    }
                }
                else
                {
                    // Base never resolved — collect for silent-e fallback
                    silentECandidates[baseWord] = &deps;
                }
            }

            if (depResolved > 0)
            {
                fprintf(stderr, "[BedManager] Inflected dependents resolved via base PBD: %u\n", depResolved);
                fflush(stderr);
            }
        }

        // ---- Silent-e fallback: failed bases from -ed/-es/-s get "e" appended ----
        // e.g. "resolv" failed → try "resolve", "issu" failed → try "issue"
        // Only fires for bases that actually failed PBD. One cleanup cycle per length.
        {
            // Collect base+e candidates grouped by length
            AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u32>> silentEByLength;
            AZStd::unordered_map<AZStd::string, AZStd::string> silentEOrigBase;  // base+e → original base
            AZ::u32 silentECount = 0;

            fprintf(stderr, "[BedManager] Silent-e candidates: %zu failed bases\n", silentECandidates.size());
            fflush(stderr);
            for (auto& [baseWord, depsPtr] : silentECandidates)
            {
                AZStd::string candidate = baseWord + "e";
                AZ::u32 candidateLen = static_cast<AZ::u32>(candidate.size());
                fprintf(stderr, "[BedManager]   '%s' -> try '%s' (len %u)\n",
                    baseWord.c_str(), candidate.c_str(), candidateLen);
                fflush(stderr);

                // Only inject if the length has a bed
                if (!activeLenSet.count(candidateLen))
                {
                    // No bed for this length — dependents are vars
                    for (auto& dep : *depsPtr)
                        allUnresolvedOriginal.push_back(dep.runIndex);
                    continue;
                }

                // Inject synthetic run for base+e
                CharRun synth;
                synth.text = candidate;
                synth.startPos = 0;
                synth.length = candidateLen;
                synth.tag = RunTag::Word;
                synth.firstCap = false;
                synth.allCaps = false;

                AZ::u32 synthIdx = static_cast<AZ::u32>(runs.size());
                runs.push_back(synth);
                silentEByLength[candidateLen].push_back(synthIdx);
                silentEOrigBase[candidate] = baseWord;
                ++silentECount;
            }

            if (silentECount > 0)
            {
                // Run cleanup PBD cycles for each length that has candidates
                AZ::u32 silentEResolved = 0;
                for (auto& [len, indices] : silentEByLength)
                {
                    AZStd::vector<AZ::u32> unresolvedFromCleanup;
                    ResolveLengthCycle(len, runs, indices,
                                       manifest.results, unresolvedFromCleanup);
                }

                // Scan results: find resolved base+e words
                // (collect first, then propagate — avoid modifying manifest during iteration)
                AZStd::unordered_map<AZStd::string, const ResolutionResult*> silentEResolutions;
                for (size_t ri = 0; ri < manifest.results.size(); ++ri)
                {
                    const auto& r = manifest.results[ri];
                    if (!r.resolved) continue;
                    if (silentEOrigBase.count(r.matchedWord))
                        silentEResolutions[r.matchedWord] = &manifest.results[ri];
                }

                // Propagate to original dependents
                AZStd::vector<AZStd::string> handledBases;
                for (auto& [candidate, baseResult] : silentEResolutions)
                {
                    auto origIt = silentEOrigBase.find(candidate);
                    if (origIt == silentEOrigBase.end()) continue;

                    auto candIt = silentECandidates.find(origIt->second);
                    if (candIt == silentECandidates.end()) continue;

                    for (auto& dep : *(candIt->second))
                    {
                        const CharRun& depRun = runs[dep.runIndex];
                        ResolutionResult dr;
                        dr.runText = depRun.text;
                        dr.resolved = true;
                        dr.matchedWord = baseResult->matchedWord;
                        dr.matchedTokenId = baseResult->matchedTokenId;
                        dr.morphBits = dep.morphBits;
                        dr.tierResolved = baseResult->tierResolved;
                        dr.runIndex = dep.runIndex;
                        dr.firstCap = depRun.firstCap;
                        dr.allCaps = depRun.allCaps;
                        manifest.results.push_back(dr);
                        ++silentEResolved;
                    }
                    handledBases.push_back(origIt->second);
                }
                for (const auto& b : handledBases)
                    silentECandidates.erase(b);

                // Remaining failed silent-e candidates → vars
                for (auto& [baseWord, depsPtr] : silentECandidates)
                {
                    for (auto& dep : *depsPtr)
                        allUnresolvedOriginal.push_back(dep.runIndex);
                }

                if (silentEResolved > 0)
                {
                    fprintf(stderr, "[BedManager] Silent-e fallback: %u dependents resolved (%u candidates tried)\n",
                        silentEResolved, silentECount);
                    fflush(stderr);
                }
            }
            else
            {
                // No silent-e candidates — push all remaining failed deps to unresolved
                for (auto& [baseWord, depsPtr] : silentECandidates)
                {
                    for (auto& dep : *depsPtr)
                        allUnresolvedOriginal.push_back(dep.runIndex);
                }
            }
        }

        if (inflectionCount > 0)
        {
            fprintf(stderr, "[BedManager] Inflection-stripped: %u runs (%u synthetic bases injected into PBD queues)\n",
                inflectionCount, syntheticInjections);
            fflush(stderr);
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
                    InflectionStripResult strip = TryInflectionStrip(vr.normalizedForm);
                    if (strip.stripped)
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
                        if (base.size() >= 2)
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
                        InflectionStripResult strip = TryInflectionStrip(vr.normalizedForm);
                        if (strip.stripped)
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
        AZStd::unordered_map<AZStd::string, const ResolutionResult*> resolvedLookup;
        for (const auto& r : manifest.results)
        {
            if (r.resolved)
                resolvedLookup[r.runText] = &r;
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
                    r.resolved = it->second->resolved;
                    r.matchedWord = it->second->matchedWord;
                    r.matchedTokenId = it->second->matchedTokenId;
                    r.tierResolved = it->second->tierResolved;
                    r.morphBits = it->second->morphBits;
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

            uniqueTokenSet.insert(tokenId);

            // Pack positional modifier: morphBits in upper 14 bits, cap flags in lower 2
            // Layout: [morphBits(14) | allCaps(1) | firstCap(1)]
            AZ::u32 modifier = 0;
            if (r.morphBits != 0 || r.firstCap || r.allCaps)
            {
                modifier = (static_cast<AZ::u32>(r.morphBits) << 2)
                         | (r.allCaps ? 2u : 0u)
                         | (r.firstCap ? 1u : 0u);
            }

            out.tokenIds.push_back(tokenId);
            out.positions.push_back(position);
            out.modifiers.push_back(modifier);
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

        fprintf(stderr, "[ScanManifest] %zu tokens, %zu unique, %zu bond types, %zu total pairs\n",
            out.tokenIds.size(), out.uniqueTokens, out.bonds.size(), out.totalPairs);
        fflush(stderr);

        return out;
    }

} // namespace HCPEngine
