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
    // ---- Position-Based Document Representation ----

    PositionMap DisassemblePositions(const TokenStream& stream)
    {
        PositionMap result;
        result.totalTokens = stream.totalSlots;

        // Build map: token -> [positions] using the gap-encoded positions
        AZStd::unordered_map<AZStd::string, AZStd::vector<AZ::u32>> posMap;
        for (size_t i = 0; i < stream.tokenIds.size(); ++i)
        {
            posMap[stream.tokenIds[i]].push_back(stream.positions[i]);
        }

        // Convert to entries
        result.entries.reserve(posMap.size());
        for (auto& [tokenId, positions] : posMap)
        {
            TokenPositions tp;
            tp.tokenId = tokenId;
            tp.positions = AZStd::move(positions);
            result.entries.push_back(AZStd::move(tp));
        }
        result.uniqueTokens = result.entries.size();

        return result;
    }

    TokenStream ReassemblePositions(const PositionMap& posMap)
    {
        TokenStream stream;
        stream.totalSlots = posMap.totalTokens;

        // Count actual tokens (non-space slots)
        size_t tokenCount = 0;
        for (const auto& entry : posMap.entries)
        {
            tokenCount += entry.positions.size();
        }

        stream.tokenIds.reserve(tokenCount);
        stream.positions.reserve(tokenCount);

        // Collect all (position, tokenId) pairs, then sort by position
        struct PosToken
        {
            AZ::u32 pos;
            const AZStd::string* tokenId;
        };
        AZStd::vector<PosToken> all;
        all.reserve(tokenCount);
        for (const auto& entry : posMap.entries)
        {
            for (AZ::u32 pos : entry.positions)
            {
                all.push_back({pos, &entry.tokenId});
            }
        }
        AZStd::sort(all.begin(), all.end(),
            [](const PosToken& a, const PosToken& b) { return a.pos < b.pos; });

        for (const auto& pt : all)
        {
            stream.tokenIds.push_back(*pt.tokenId);
            stream.positions.push_back(pt.pos);
        }

        return stream;
    }

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
        sceneDesc.gravity = physx::PxVec3(0.0f, -1.0f, 0.0f);
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
        AZStd::vector<AZStd::string> sequence;

        if (!m_initialized || pbmData.bonds.empty())
        {
            return sequence;
        }

        // ---- Count total dumbbell instances and particles ----
        // Each bond (A, B, count) spawns count dumbbells.
        // Each dumbbell = 2 particles (A-side + B-side).
        physx::PxU32 totalDumbbells = 0;
        for (const auto& bond : pbmData.bonds)
        {
            totalDumbbells += static_cast<physx::PxU32>(bond.count);
        }
        const physx::PxU32 numParticles = totalDumbbells * 2;

        if (numParticles < 2)
        {
            return sequence;
        }

        fprintf(stderr, "[HCPParticlePipeline] Reassemble: %zu bonds, %u dumbbells, %u particles\n",
            pbmData.bonds.size(), totalDumbbells, numParticles);
        fflush(stderr);

        // ---- Create a fresh PBD particle system ----
        // Plain PxParticleBuffer (no cloth/springs) — uses PBD material cohesion
        // and proximity-based interactions for the attractive force between
        // same-token particles.
        physx::PxPBDParticleSystem* particleSystem =
            m_pxPhysics->createPBDParticleSystem(*m_cudaContextManager, 96);
        if (!particleSystem)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create PBD particle system for reassembly");
            return sequence;
        }
        particleSystem->setRestOffset(0.3f);
        particleSystem->setContactOffset(0.4f);
        // particleContactOffset: particles within this distance interact.
        // Dumbbell pair spacing = 0.5, so pair members are always in contact.
        // Inter-dumbbell spacing > 1.5, so separate dumbbells start independent.
        particleSystem->setParticleContactOffset(1.5f);
        particleSystem->setSolidRestOffset(0.3f);
        particleSystem->setSolverIterationCounts(4, 1);
        m_pxScene->addActor(*particleSystem);

        // ---- Phase creation: one phase per unique token ----
        // Same-token particles share a phase → PBD cohesion pulls them together.
        // This IS the attractive force — the engine's GPU solver handles the math.
        AZStd::unordered_map<AZStd::string, physx::PxU32> tokenPhases;
        for (const auto& bond : pbmData.bonds)
        {
            if (tokenPhases.find(bond.tokenA) == tokenPhases.end())
            {
                tokenPhases[bond.tokenA] = particleSystem->createPhase(
                    m_reassemblyMaterial,
                    physx::PxParticlePhaseFlags(
                        physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            }
            if (tokenPhases.find(bond.tokenB) == tokenPhases.end())
            {
                tokenPhases[bond.tokenB] = particleSystem->createPhase(
                    m_reassemblyMaterial,
                    physx::PxParticlePhaseFlags(
                        physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            }
        }

        fprintf(stderr, "[HCPParticlePipeline] Created %zu unique token phases\n", tokenPhases.size());
        fflush(stderr);

        // ---- Create plain particle buffer ----
        physx::PxParticleBuffer* particleBuffer = m_pxPhysics->createParticleBuffer(
            numParticles, 1, m_cudaContextManager);
        if (!particleBuffer)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create particle buffer for %u particles", numParticles);
            m_pxScene->removeActor(*particleSystem);
            particleSystem->release();
            return sequence;
        }

        // ---- Build and upload particle data ----
        // Dumbbell d: particle[2d] = A-side, particle[2d+1] = B-side
        // Pair members placed 0.5 apart (within contactOffset) = bonded.
        // Dumbbells spaced 3.0 apart in 3D cube (> contactOffset) = independent.
        AZStd::vector<AZStd::string> particleTokenIds(numParticles);

        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* positions = particleBuffer->getPositionInvMasses();
            physx::PxVec4* velocities = particleBuffer->getVelocities();
            physx::PxU32*  phases = particleBuffer->getPhases();

            physx::PxVec4* hostPos = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxVec4* hostVel = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxU32*  hostPhase = m_cudaContextManager->allocPinnedHostBuffer<physx::PxU32>(numParticles);

            // 3D cube layout for dumbbells
            const physx::PxU32 cubeEdge = static_cast<physx::PxU32>(
                std::ceil(std::cbrt(static_cast<double>(totalDumbbells)))) + 1;
            const float spacing = 3.0f;

            physx::PxU32 dIdx = 0;
            for (const auto& bond : pbmData.bonds)
            {
                for (int inst = 0; inst < bond.count; ++inst, ++dIdx)
                {
                    const physx::PxU32 pA = dIdx * 2;
                    const physx::PxU32 pB = dIdx * 2 + 1;

                    particleTokenIds[pA] = bond.tokenA;
                    particleTokenIds[pB] = bond.tokenB;

                    // 3D grid position
                    const physx::PxU32 ix = dIdx % cubeEdge;
                    const physx::PxU32 iy = (dIdx / cubeEdge) % cubeEdge;
                    const physx::PxU32 iz = dIdx / (cubeEdge * cubeEdge);
                    const float x = static_cast<float>(ix) * spacing;
                    const float y = static_cast<float>(iy) * spacing;
                    const float z = static_cast<float>(iz) * spacing;

                    // Stream_start anchor: invMass = 0 (pinned at origin)
                    const float invMassA = (bond.tokenA == STREAM_START) ? 0.0f : 1.0f;

                    hostPos[pA] = physx::PxVec4(x, y, z, invMassA);
                    hostPos[pB] = physx::PxVec4(x + 0.5f, y, z, 1.0f);

                    hostVel[pA] = physx::PxVec4(0.0f);
                    hostVel[pB] = physx::PxVec4(0.0f);

                    hostPhase[pA] = tokenPhases[bond.tokenA];
                    hostPhase[pB] = tokenPhases[bond.tokenB];
                }
            }

            fprintf(stderr, "[HCPParticlePipeline] 3D cube layout: %u^3, spacing %.1f\n",
                cubeEdge, spacing);
            fflush(stderr);

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

        // Gentle +x gravity: provides directional ordering (sequence flow).
        // Combined with the pinned stream_start anchor at origin,
        // the chain extends rightward.
        const physx::PxVec3 origGravity = m_pxScene->getGravity();
        m_pxScene->setGravity(physx::PxVec3(0.05f, 0.0f, 0.0f));

        fprintf(stderr, "[HCPParticlePipeline] Particles uploaded, simulating %u particles...\n",
            numParticles);
        fflush(stderr);

        // ---- Simulate: PBD solver processes all interactions in parallel on GPU ----
        const int numSteps = 20;
        const float dt = 1.0f / 60.0f;

        for (int step = 0; step < numSteps; ++step)
        {
            m_pxScene->simulate(dt);
            m_pxScene->fetchResults(true);
            m_pxScene->fetchResultsParticleSystem();
        }
        fprintf(stderr, "[HCPParticlePipeline] Simulation complete (%d steps)\n", numSteps);
        fflush(stderr);

        // ---- Read back particle positions from GPU ----
        struct DumbbellResult
        {
            float aX;
            physx::PxU32 index;
        };
        AZStd::vector<DumbbellResult> dumbbells(totalDumbbells);

        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* pinnedPos = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);

            if (!pinnedPos)
            {
                fprintf(stderr, "[HCPParticlePipeline] ERROR: allocPinnedHostBuffer returned NULL\n");
                fflush(stderr);
                // Fall through to cleanup
            }
            else
            {
                m_cudaContextManager->copyDToH(pinnedPos, devPos, numParticles);

                for (physx::PxU32 d = 0; d < totalDumbbells; ++d)
                {
                    dumbbells[d].aX = pinnedPos[d * 2].x;
                    dumbbells[d].index = d;
                }

                m_cudaContextManager->freePinnedHostBuffer(pinnedPos);
            }
        }

        // ---- Sort dumbbells by A-side x position = sequence order ----
        AZStd::sort(dumbbells.begin(), dumbbells.end(),
            [](const DumbbellResult& a, const DumbbellResult& b) { return a.aX < b.aX; });

        // ---- Extract token sequence ----
        sequence.reserve(totalDumbbells + 1);
        for (physx::PxU32 d = 0; d < totalDumbbells; ++d)
        {
            const physx::PxU32 origIdx = dumbbells[d].index;
            sequence.push_back(particleTokenIds[origIdx * 2]);
        }
        if (!dumbbells.empty())
        {
            const physx::PxU32 lastIdx = dumbbells[totalDumbbells - 1].index;
            sequence.push_back(particleTokenIds[lastIdx * 2 + 1]);
        }

        fprintf(stderr, "[HCPParticlePipeline] Sequence: %zu tokens\n", sequence.size());
        fflush(stderr);

        // ---- Cleanup ----
        particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        m_pxScene->removeActor(*particleSystem);
        particleSystem->release();
        m_pxScene->setGravity(origGravity);

        AZLOG_INFO("HCPParticlePipeline: Reassembled %zu tokens from %zu bonds",
            sequence.size(), pbmData.bonds.size());

        return sequence;
    }

} // namespace HCPEngine
