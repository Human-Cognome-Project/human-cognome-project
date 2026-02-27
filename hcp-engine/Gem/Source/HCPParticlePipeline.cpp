#include "HCPParticlePipeline.h"
#include "HCPVocabulary.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <cmath>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>
#include <extensions/PxDefaultSimulationFilterShader.h>

// O3DE PhysX system — for accessing the shared CPU dispatcher
#include <System/PhysXSystem.h>

namespace HCPEngine
{
    PBMData DerivePBM(const TokenStream& stream)
    {
        PBMData result;
        if (stream.tokenIds.size() < 2)
        {
            return result;
        }

        // Count adjacent pairs (consecutive tokens in the stream)
        AZStd::unordered_map<AZStd::string, int> bondCounts;
        for (size_t i = 0; i + 1 < stream.tokenIds.size(); ++i)
        {
            AZStd::string key = stream.tokenIds[i] + "|" + stream.tokenIds[i + 1];
            bondCounts[key]++;
        }

        // Build bond list
        result.bonds.reserve(bondCounts.size());
        for (const auto& [key, count] : bondCounts)
        {
            size_t sep = key.find('|');
            if (sep != AZStd::string::npos)
            {
                Bond bond;
                bond.tokenA = AZStd::string(key.data(), sep);
                bond.tokenB = AZStd::string(key.data() + sep + 1, key.size() - sep - 1);
                bond.count = count;
                result.bonds.push_back(bond);
            }
        }

        result.firstFpbA = stream.tokenIds[0];
        result.firstFpbB = stream.tokenIds[1];
        result.totalPairs = stream.tokenIds.size() - 1;
        {
            AZStd::unordered_map<AZStd::string, int> uniq;
            for (const auto& b : result.bonds) { uniq[b.tokenA] = 1; uniq[b.tokenB] = 1; }
            result.uniqueTokens = uniq.size();
        }

        return result;
    }

    // ---- Particle Pipeline ----

    HCPParticlePipeline::~HCPParticlePipeline()
    {
        Shutdown();
    }

    bool HCPParticlePipeline::Initialize(physx::PxPhysics* pxPhysics, physx::PxFoundation* pxFoundation)
    {
        if (m_initialized)
        {
            return true;
        }

        if (!pxPhysics || !pxFoundation)
        {
            AZLOG_ERROR("HCPParticlePipeline: PxPhysics or PxFoundation is null");
            return false;
        }

        m_pxPhysics = pxPhysics;

        // Register the PhysX foundation with our statically-linked PhysX code.
        // Without this, PxGetFoundation() returns null from our module's copy of
        // the global, causing crashes in PxCreateParticleClothPreProcessor etc.
        PxSetFoundationInstance(*pxFoundation);

        // Create CUDA context manager for GPU physics
        fprintf(stderr, "[HCPParticlePipeline] Creating CUDA context manager...\n");
        fflush(stderr);
        physx::PxCudaContextManagerDesc cudaDesc;
        cudaDesc.interopMode = physx::PxCudaInteropMode::NO_INTEROP;

        m_cudaContextManager = PxCreateCudaContextManager(*pxFoundation, cudaDesc);
        fprintf(stderr, "[HCPParticlePipeline] PxCreateCudaContextManager returned: %p\n",
            static_cast<void*>(m_cudaContextManager));
        fflush(stderr);
        if (!m_cudaContextManager || !m_cudaContextManager->contextIsValid())
        {
            fprintf(stderr, "[HCPParticlePipeline] CUDA context invalid or null\n");
            fflush(stderr);
            if (m_cudaContextManager)
            {
                m_cudaContextManager->release();
                m_cudaContextManager = nullptr;
            }
            return false;
        }

        fprintf(stderr, "[HCPParticlePipeline] CUDA context created on %s (%zu MB)\n",
            m_cudaContextManager->getDeviceName(),
            m_cudaContextManager->getDeviceTotalMemBytes() / (1024 * 1024));
        fflush(stderr);

        // Get CPU dispatcher from O3DE's PhysX system
        PhysX::PhysXSystem* physxSystem = PhysX::GetPhysXSystem();
        physx::PxCpuDispatcher* cpuDispatcher = physxSystem ? physxSystem->GetPxCpuDispathcher() : nullptr;
        if (!cpuDispatcher)
        {
            fprintf(stderr, "[HCPParticlePipeline] O3DE CPU dispatcher not available\n");
            fflush(stderr);
            Shutdown();
            return false;
        }
        fprintf(stderr, "[HCPParticlePipeline] Got O3DE CPU dispatcher\n");
        fflush(stderr);

        // Create a GPU-enabled PxScene specifically for PBD particle work.
        // This is separate from O3DE's game physics scene.
        fprintf(stderr, "[HCPParticlePipeline] Creating GPU-enabled PxScene...\n");
        fflush(stderr);
        physx::PxSceneDesc sceneDesc(pxPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = cpuDispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        sceneDesc.cudaContextManager = m_cudaContextManager;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
        sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;

        m_pxScene = pxPhysics->createScene(sceneDesc);
        fprintf(stderr, "[HCPParticlePipeline] createScene returned: %p\n",
            static_cast<void*>(m_pxScene));
        fflush(stderr);
        if (!m_pxScene)
        {
            fprintf(stderr, "[HCPParticlePipeline] Failed to create GPU-enabled PxScene\n");
            fflush(stderr);
            Shutdown();
            return false;
        }

        fprintf(stderr, "[HCPParticlePipeline] GPU-enabled PxScene created\n");
        fflush(stderr);

        // Create PBD particle material
        m_particleMaterial = pxPhysics->createPBDMaterial(
            0.2f,   // friction
            0.05f,  // damping
            0.0f,   // adhesion
            0.0f,   // viscosity
            0.5f,   // vorticityConfinement
            0.0f,   // surfaceTension
            1.0f,   // cohesion
            0.0f,   // lift
            0.0f    // drag
        );

        if (!m_particleMaterial)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create particle material");
            Shutdown();
            return false;
        }

        // Reassembly material: high cohesion for same-token clustering,
        // high damping for convergence, no adhesion/viscosity
        m_reassemblyMaterial = pxPhysics->createPBDMaterial(
            0.05f,  // friction — low, let particles slide
            0.8f,   // damping — high for fast convergence
            0.0f,   // adhesion
            0.0f,   // viscosity
            0.0f,   // vorticityConfinement
            0.0f,   // surfaceTension
            2.0f,   // cohesion — strong same-phase attraction
            0.0f,   // lift
            0.0f    // drag
        );

        if (!m_reassemblyMaterial)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create reassembly material");
            Shutdown();
            return false;
        }

        // PBD particle systems are created per-operation (Disassemble/Reassemble)
        // because GPU internal buffers are sized for the first buffer added and
        // cannot resize for different particle counts.

        m_initialized = true;
        AZLOG_INFO("HCPParticlePipeline: PBD pipeline initialized and ready");
        return true;
    }

    bool HCPParticlePipeline::CreateCharWordScene()
    {
        if (!m_initialized || !m_pxPhysics || !m_cudaContextManager)
        {
            fprintf(stderr, "[HCPParticlePipeline] CreateCharWordScene: pipeline not initialized\n");
            fflush(stderr);
            return false;
        }

        if (m_charWordScene)
        {
            return true;  // Already created
        }

        PhysX::PhysXSystem* physxSystem = PhysX::GetPhysXSystem();
        physx::PxCpuDispatcher* cpuDispatcher = physxSystem ? physxSystem->GetPxCpuDispathcher() : nullptr;
        if (!cpuDispatcher)
        {
            fprintf(stderr, "[HCPParticlePipeline] CreateCharWordScene: no CPU dispatcher\n");
            fflush(stderr);
            return false;
        }

        physx::PxSceneDesc sceneDesc(m_pxPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = cpuDispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        sceneDesc.cudaContextManager = m_cudaContextManager;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
        sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;

        m_charWordScene = m_pxPhysics->createScene(sceneDesc);
        if (!m_charWordScene)
        {
            fprintf(stderr, "[HCPParticlePipeline] CreateCharWordScene: createScene failed\n");
            fflush(stderr);
            return false;
        }

        fprintf(stderr, "[HCPParticlePipeline] Char->word PxScene created (dedicated Phase 2)\n");
        fflush(stderr);
        return true;
    }

    void HCPParticlePipeline::Shutdown()
    {
        if (m_reassemblyMaterial)
        {
            m_reassemblyMaterial->release();
            m_reassemblyMaterial = nullptr;
        }

        if (m_particleMaterial)
        {
            m_particleMaterial->release();
            m_particleMaterial = nullptr;
        }

        if (m_charWordScene)
        {
            m_charWordScene->release();
            m_charWordScene = nullptr;
        }

        if (m_pxScene)
        {
            m_pxScene->release();
            m_pxScene = nullptr;
        }

        // CPU dispatcher is owned by O3DE PhysXSystem — don't release it

        if (m_cudaContextManager)
        {
            m_cudaContextManager->release();
            m_cudaContextManager = nullptr;
        }

        m_initialized = false;
    }

    PBMData HCPParticlePipeline::Disassemble(const AZStd::vector<AZStd::string>& tokenIds)
    {
        PBMData result;

        if (!m_initialized || tokenIds.size() < 2)
        {
            return result;
        }

        const physx::PxU32 numParticles = static_cast<physx::PxU32>(tokenIds.size());
        fprintf(stderr, "[HCPParticlePipeline] Disassemble: %u particles\n", numParticles);
        fflush(stderr);

        // Create a fresh PBD particle system for this operation
        physx::PxPBDParticleSystem* particleSystem =
            m_pxPhysics->createPBDParticleSystem(*m_cudaContextManager, 96);
        if (!particleSystem)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create PBD particle system for disassembly");
            return result;
        }
        particleSystem->setRestOffset(0.3f);
        particleSystem->setContactOffset(0.4f);
        particleSystem->setParticleContactOffset(1.5f);
        particleSystem->setSolidRestOffset(0.3f);
        particleSystem->setSolverIterationCounts(4, 1);
        m_pxScene->addActor(*particleSystem);

        // Create a particle buffer: each token = one particle positioned in a 1D sequence
        physx::PxParticleBuffer* particleBuffer = m_pxPhysics->createParticleBuffer(
            numParticles, 1, m_cudaContextManager);
        if (!particleBuffer)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create particle buffer for %u particles", numParticles);
            m_pxScene->removeActor(*particleSystem);
            particleSystem->release();
            return result;
        }

        // Create a phase for our token particles
        const physx::PxU32 phase = particleSystem->createPhase(
            m_particleMaterial,
            physx::PxParticlePhaseFlags(physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));

        // Position each token at (i, 0, 0) in sequence.
        // particleContactOffset = 1.5 means particles at distance 1.0 apart ARE neighbors.
        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* positions = particleBuffer->getPositionInvMasses();
            physx::PxVec4* velocities = particleBuffer->getVelocities();
            physx::PxU32* phases = particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxVec4* hostVel = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxU32* hostPhase = m_cudaContextManager->allocPinnedHostBuffer<physx::PxU32>(numParticles);

            for (physx::PxU32 i = 0; i < numParticles; ++i)
            {
                hostPos[i] = physx::PxVec4(static_cast<float>(i), 0.0f, 0.0f, 1.0f);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = phase;
            }

            m_cudaContextManager->copyHToD(positions, hostPos, numParticles);
            m_cudaContextManager->copyHToD(velocities, hostVel, numParticles);
            m_cudaContextManager->copyHToD(phases, hostPhase, numParticles);

            m_cudaContextManager->freePinnedHostBuffer(hostPos);
            m_cudaContextManager->freePinnedHostBuffer(hostVel);
            m_cudaContextManager->freePinnedHostBuffer(hostPhase);
        }

        particleBuffer->setNbActiveParticles(numParticles);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);

        // Add buffer to particle system
        particleSystem->addParticleBuffer(particleBuffer);
        fprintf(stderr, "[HCPParticlePipeline] Particles uploaded, simulating...\n");
        fflush(stderr);

        // Simulate — PBD spatial hash processes all particles on GPU in parallel.
        // After simulation, read back positions to identify neighbor pairs.
        m_pxScene->simulate(1.0f / 60.0f);
        m_pxScene->fetchResults(true);
        m_pxScene->fetchResultsParticleSystem();
        fprintf(stderr, "[HCPParticlePipeline] Simulation complete, reading back positions...\n");
        fflush(stderr);

        // ---- Read back particle positions from GPU ----
        // After PBD simulation, particles have been processed by the spatial hash.
        // We read back positions and identify neighbors by proximity
        // (distance < particleContactOffset = 1.5).
        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* hostPos = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            m_cudaContextManager->copyDToH(hostPos, devPos, numParticles);

            // Particles are positioned at (i, 0, 0) so original index = round(x).
            // Build mapping from settled position to original sequence index,
            // then identify adjacent pairs via position proximity.
            struct ParticlePos
            {
                float x;
                physx::PxU32 origIndex;
            };
            AZStd::vector<ParticlePos> positions(numParticles);
            for (physx::PxU32 i = 0; i < numParticles; ++i)
            {
                positions[i].x = hostPos[i].x;
                positions[i].origIndex = i;  // Particles are unsorted — index IS sequence position
            }
            m_cudaContextManager->freePinnedHostBuffer(hostPos);

            // Sort by x position to find spatial neighbors
            AZStd::sort(positions.begin(), positions.end(),
                [](const ParticlePos& a, const ParticlePos& b) { return a.x < b.x; });

            // Count neighbor pairs: consecutive particles in sorted order
            // that are within particleContactOffset distance
            const float contactOffset = 1.5f;
            m_bondCounts.clear();
            AZStd::unordered_map<AZStd::string, int> uniqueTokenSet;

            for (physx::PxU32 i = 0; i + 1 < numParticles; ++i)
            {
                float dist = positions[i + 1].x - positions[i].x;
                if (dist < contactOffset)
                {
                    physx::PxU32 origA = positions[i].origIndex;
                    physx::PxU32 origB = positions[i + 1].origIndex;

                    // Ensure forward direction (lower sequence index first)
                    if (origA > origB)
                    {
                        physx::PxU32 tmp = origA;
                        origA = origB;
                        origB = tmp;
                    }

                    // Only count immediately adjacent pairs in the original sequence
                    if (origB == origA + 1)
                    {
                        const AZStd::string& tokA = tokenIds[origA];
                        const AZStd::string& tokB = tokenIds[origB];
                        AZStd::string key = tokA + "|" + tokB;
                        m_bondCounts[key]++;

                        uniqueTokenSet[tokA] = 1;
                        uniqueTokenSet[tokB] = 1;
                    }
                }
            }

            // Build bond list from counts
            for (const auto& [key, count] : m_bondCounts)
            {
                size_t sep = key.find('|');
                if (sep != AZStd::string::npos)
                {
                    Bond bond;
                    bond.tokenA = AZStd::string(key.data(), sep);
                    bond.tokenB = AZStd::string(key.data() + sep + 1, key.size() - sep - 1);
                    bond.count = count;
                    result.bonds.push_back(bond);
                }
            }
        }

        // First forward pair bond
        if (tokenIds.size() >= 2)
        {
            result.firstFpbA = tokenIds[0];
            result.firstFpbB = tokenIds[1];
        }

        result.totalPairs = tokenIds.size() - 1;
        {
            AZStd::unordered_map<AZStd::string, int> uniqueTokenSet;
            for (const auto& b : result.bonds) { uniqueTokenSet[b.tokenA] = 1; uniqueTokenSet[b.tokenB] = 1; }
            result.uniqueTokens = uniqueTokenSet.size();
        }

        fprintf(stderr, "[HCPParticlePipeline] Bonds extracted: %zu unique, %zu total pairs\n",
            result.bonds.size(), result.totalPairs);
        fflush(stderr);

        // Clean up — destroy per-operation particle system
        particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        m_pxScene->removeActor(*particleSystem);
        particleSystem->release();

        AZLOG_INFO("HCPParticlePipeline: Disassembled %zu tokens into %zu unique bonds (%zu total pairs)",
            tokenIds.size(), result.bonds.size(), result.totalPairs);

        return result;
    }

    AZStd::vector<AZStd::string> HCPParticlePipeline::Reassemble(
        const PBMData& pbmData,
        const HCPVocabulary& /*vocab*/)
    {
        // PLACEHOLDER — waiting on positional storage migration.
        // Reassembly will load position map from DB, sort by position,
        // and return ordered token IDs. Spaces filled at numeric gaps.
        //
        // Bond-only Euler path approach parked in HCPEulerReassembly_FUTURE.cpp
        // for revisit after conceptual modeling provides disambiguation constraints.

        AZStd::vector<AZStd::string> sequence;
        AZLOG_WARN("HCPParticlePipeline::Reassemble — not yet wired to positional storage");
        return sequence;
    }

} // namespace HCPEngine
