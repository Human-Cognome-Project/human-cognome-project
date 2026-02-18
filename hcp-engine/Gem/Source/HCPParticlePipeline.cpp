#include "HCPParticlePipeline.h"
#include "HCPVocabulary.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxDefaultSimulationFilterShader.h>

namespace HCPEngine
{
    // ---- Disassembly Callback (local to this translation unit) ----

    class DisassemblyCallback : public physx::PxParticleSystemCallback
    {
    public:
        DisassemblyCallback(
            AZStd::vector<AZStd::string>& particleTokenIds,
            AZStd::unordered_map<AZStd::string, int>& bondCounts)
            : m_particleTokenIds(particleTokenIds)
            , m_bondCounts(bondCounts)
        {}

        void onBegin(
            const physx::PxGpuMirroredPointer<physx::PxGpuParticleSystem>& /*gpuParticleSystem*/,
            CUstream /*stream*/) override
        {
        }

        void onAdvance(
            const physx::PxGpuMirroredPointer<physx::PxGpuParticleSystem>& /*gpuParticleSystem*/,
            CUstream /*stream*/) override
        {
        }

        void onPostSolve(
            const physx::PxGpuMirroredPointer<physx::PxGpuParticleSystem>& gpuParticleSystem,
            CUstream /*stream*/) override
        {
            auto* gpuPS = gpuParticleSystem.mHostPtr;
            if (!gpuPS)
            {
                return;
            }

            const physx::PxU32 numParticles = gpuPS->mCommonData.mNumParticles;

            // Use the PBD spatial hash neighbor structure to count token pair bonds.
            // Each particle represents a token positioned at (i, 0, 0).
            // The spatial hash with particleContactOffset identifies adjacent particles.
            m_bondCounts.clear();
            for (physx::PxU32 i = 0; i < numParticles; ++i)
            {
                physx::PxNeighborhoodIterator iter = gpuPS->getIterator(i);
                physx::PxU32 neighborIdx = iter.getNextIndex();

                while (neighborIdx != 0xFFFFFFFF && neighborIdx < numParticles)
                {
                    physx::PxU32 unsortedI = gpuPS->mSortedToUnsortedMapping[i];
                    physx::PxU32 unsortedN = gpuPS->mSortedToUnsortedMapping[neighborIdx];

                    // Only count forward direction to avoid double counting
                    if (unsortedI < unsortedN && unsortedN == unsortedI + 1)
                    {
                        if (unsortedI < m_particleTokenIds.size() && unsortedN < m_particleTokenIds.size())
                        {
                            AZStd::string key = m_particleTokenIds[unsortedI] + "|" + m_particleTokenIds[unsortedN];
                            m_bondCounts[key]++;
                        }
                    }
                    neighborIdx = iter.getNextIndex();
                }
            }

            m_complete = true;
        }

        bool m_complete = false;

    private:
        AZStd::vector<AZStd::string>& m_particleTokenIds;
        AZStd::unordered_map<AZStd::string, int>& m_bondCounts;
    };

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

        // Create CUDA context manager for GPU physics
        physx::PxCudaContextManagerDesc cudaDesc;
        cudaDesc.interopMode = physx::PxCudaInteropMode::NO_INTEROP;

        m_cudaContextManager = PxCreateCudaContextManager(*pxFoundation, cudaDesc);
        if (!m_cudaContextManager || !m_cudaContextManager->contextIsValid())
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create CUDA context manager");
            if (m_cudaContextManager)
            {
                m_cudaContextManager->release();
                m_cudaContextManager = nullptr;
            }
            return false;
        }

        AZLOG_INFO("HCPParticlePipeline: CUDA context created on %s (%zu MB)",
            m_cudaContextManager->getDeviceName(),
            m_cudaContextManager->getDeviceTotalMemBytes() / (1024 * 1024));

        // Create CPU dispatcher for scene simulation
        m_cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);

        // Create a GPU-enabled PxScene specifically for PBD particle work.
        // This is separate from O3DE's game physics scene.
        physx::PxSceneDesc sceneDesc(pxPhysics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -1.0f, 0.0f);
        sceneDesc.cpuDispatcher = m_cpuDispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        sceneDesc.cudaContextManager = m_cudaContextManager;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_PCM;
        sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;

        m_pxScene = pxPhysics->createScene(sceneDesc);
        if (!m_pxScene)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create GPU-enabled PxScene");
            Shutdown();
            return false;
        }

        AZLOG_INFO("HCPParticlePipeline: GPU-enabled PxScene created");

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

        // Create PBD particle system
        m_particleSystem = pxPhysics->createPBDParticleSystem(*m_cudaContextManager, 96);
        if (!m_particleSystem)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create PBD particle system");
            Shutdown();
            return false;
        }

        // Configure particle system parameters
        m_particleSystem->setRestOffset(0.3f);
        m_particleSystem->setContactOffset(0.4f);
        m_particleSystem->setParticleContactOffset(1.5f);
        m_particleSystem->setSolidRestOffset(0.3f);
        m_particleSystem->setSolverIterationCounts(4, 1);

        // Add particle system to the GPU scene
        m_pxScene->addActor(*m_particleSystem);

        m_initialized = true;
        AZLOG_INFO("HCPParticlePipeline: PBD particle system initialized and ready");
        return true;
    }

    void HCPParticlePipeline::Shutdown()
    {
        if (m_particleSystem && m_pxScene)
        {
            m_particleSystem->setParticleSystemCallback(nullptr);
            m_pxScene->removeActor(*m_particleSystem);
            m_particleSystem->release();
            m_particleSystem = nullptr;
        }

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

        if (m_cpuDispatcher)
        {
            m_cpuDispatcher->release();
            m_cpuDispatcher = nullptr;
        }

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

        // Set up callback data for this disassembly run
        m_particleTokenIds = tokenIds;
        m_bondCounts.clear();

        DisassemblyCallback callback(m_particleTokenIds, m_bondCounts);
        m_particleSystem->setParticleSystemCallback(&callback);

        // Create a particle buffer: each token = one particle positioned in a 1D sequence
        physx::PxParticleBuffer* particleBuffer = m_pxPhysics->createParticleBuffer(
            numParticles, 1, m_cudaContextManager);
        if (!particleBuffer)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create particle buffer for %u particles", numParticles);
            m_particleSystem->setParticleSystemCallback(nullptr);
            return result;
        }

        // Create a phase for our token particles
        const physx::PxU32 phase = m_particleSystem->createPhase(
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
        m_particleSystem->addParticleBuffer(particleBuffer);

        // Simulate — the PBD spatial hash identifies all neighbor pairs,
        // and the callback counts them as bonds
        m_pxScene->simulate(1.0f / 60.0f);
        m_pxScene->fetchResults(true);
        m_pxScene->fetchResultsParticleSystem();

        // Extract bonds from callback data
        AZStd::unordered_map<AZStd::string, int> uniqueTokenSet;
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

                uniqueTokenSet[bond.tokenA] = 1;
                uniqueTokenSet[bond.tokenB] = 1;
            }
        }

        // First forward pair bond
        if (tokenIds.size() >= 2)
        {
            result.firstFpbA = tokenIds[0];
            result.firstFpbB = tokenIds[1];
        }

        result.totalPairs = tokenIds.size() - 1;
        result.uniqueTokens = uniqueTokenSet.size();

        // Clean up
        m_particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        m_particleSystem->setParticleSystemCallback(nullptr);

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
        // Each dumbbell = 2 particles + 1 spring.
        physx::PxU32 totalSprings = 0;
        for (const auto& bond : pbmData.bonds)
        {
            totalSprings += static_cast<physx::PxU32>(bond.count);
        }
        const physx::PxU32 numParticles = totalSprings * 2;

        if (numParticles < 2)
        {
            return sequence;
        }

        AZLOG_INFO("HCPParticlePipeline: Reassembling %zu bonds (%u dumbbells, %u particles) via PBD physics",
            pbmData.bonds.size(), totalSprings, numParticles);

        // ---- Phase creation: one phase per unique token ----
        // Same token = same phase group = cohesion attraction.
        // The PBD solver uses phase groups to determine which particles
        // attract each other via the material's cohesion parameter.
        AZStd::unordered_map<AZStd::string, physx::PxU32> tokenPhases;
        for (const auto& bond : pbmData.bonds)
        {
            if (tokenPhases.find(bond.tokenA) == tokenPhases.end())
            {
                tokenPhases[bond.tokenA] = m_particleSystem->createPhase(
                    m_reassemblyMaterial,
                    physx::PxParticlePhaseFlags(
                        physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            }
            if (tokenPhases.find(bond.tokenB) == tokenPhases.end())
            {
                tokenPhases[bond.tokenB] = m_particleSystem->createPhase(
                    m_reassemblyMaterial,
                    physx::PxParticlePhaseFlags(
                        physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));
            }
        }

        AZLOG_INFO("HCPParticlePipeline: Created %zu unique token phases for cohesion clustering",
            tokenPhases.size());

        // ---- Build particle data arrays ----
        // Dumbbell d: particle[2d] = A-side, particle[2d+1] = B-side
        // Spring d: connects particle 2d <-> 2d+1 (bond constraint)
        AZStd::vector<AZStd::string> particleTokenIds(numParticles);
        AZStd::vector<physx::PxVec4> hostPositions(numParticles);
        AZStd::vector<physx::PxVec4> hostVelocities(numParticles);
        AZStd::vector<physx::PxU32>  hostPhases(numParticles);
        AZStd::vector<physx::PxVec4> hostRestPositions(numParticles);
        AZStd::vector<physx::PxParticleSpring> hostSprings(totalSprings);

        physx::PxU32 dIdx = 0;
        for (const auto& bond : pbmData.bonds)
        {
            for (int inst = 0; inst < bond.count; ++inst, ++dIdx)
            {
                const physx::PxU32 pA = dIdx * 2;
                const physx::PxU32 pB = dIdx * 2 + 1;

                particleTokenIds[pA] = bond.tokenA;
                particleTokenIds[pB] = bond.tokenB;

                // Spread dumbbells along x-axis with spacing
                const float x = static_cast<float>(dIdx) * 2.5f;

                // Stream_start anchor: invMass = 0 (immovable, pinned at origin)
                // This anchors the chain and gives it a fixed starting point.
                const float invMassA = (bond.tokenA == STREAM_START) ? 0.0f : 1.0f;
                const float invMassB = 1.0f;

                hostPositions[pA] = physx::PxVec4(x, 0.0f, 0.0f, invMassA);
                hostPositions[pB] = physx::PxVec4(x + 1.0f, 0.0f, 0.0f, invMassB);

                hostVelocities[pA] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostVelocities[pB] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);

                // Phase: same token = same group = cohesion
                hostPhases[pA] = tokenPhases[bond.tokenA];
                hostPhases[pB] = tokenPhases[bond.tokenB];

                // Rest positions for cloth solver
                hostRestPositions[pA] = physx::PxVec4(x, 0.0f, 0.0f, 0.0f);
                hostRestPositions[pB] = physx::PxVec4(x + 1.0f, 0.0f, 0.0f, 0.0f);

                // Spring: connects A <-> B within dumbbell (bond constraint)
                hostSprings[dIdx].ind0 = pA;
                hostSprings[dIdx].ind1 = pB;
                hostSprings[dIdx].length = 1.0f;
                hostSprings[dIdx].stiffness = 10.0f;
                hostSprings[dIdx].damping = 0.5f;
                hostSprings[dIdx].pad = 0.0f;
            }
        }

        // ---- Create PxParticleClothBuffer ----
        physx::PxParticleClothBuffer* clothBuffer = m_pxPhysics->createParticleClothBuffer(
            numParticles,      // maxParticles
            1,                 // maxNumVolumes
            1,                 // maxNumCloths
            0,                 // maxNumTriangles (no aerodynamics)
            totalSprings,      // maxNumSprings
            m_cudaContextManager);

        if (!clothBuffer)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create cloth buffer for %u particles", numParticles);
            return sequence;
        }

        // ---- Preprocess springs for GPU partitioning ----
        // The preprocessor partitions springs into groups that can be
        // resolved in parallel without conflicts on the GPU.
        physx::PxParticleCloth cloth;
        cloth.startVertexIndex = 0;
        cloth.numVertices = numParticles;
        cloth.clothBlendScale = 1.0f;
        cloth.restVolume = 0.0f;
        cloth.pressure = 0.0f;
        cloth.startTriangleIndex = 0;
        cloth.numTriangles = 0;

        physx::PxParticleClothDesc clothDesc;
        clothDesc.cloths = &cloth;
        clothDesc.nbCloths = 1;
        clothDesc.springs = hostSprings.data();
        clothDesc.nbSprings = totalSprings;
        clothDesc.restPositions = hostRestPositions.data();
        clothDesc.nbParticles = numParticles;
        clothDesc.triangles = nullptr;
        clothDesc.nbTriangles = 0;

        physx::PxParticleClothPreProcessor* preprocessor =
            PxCreateParticleClothPreProcessor(m_cudaContextManager);

        if (!preprocessor)
        {
            AZLOG_ERROR("HCPParticlePipeline: Failed to create cloth preprocessor");
            clothBuffer->release();
            return sequence;
        }

        physx::PxPartitionedParticleCloth partitioned;
        preprocessor->partitionSprings(clothDesc, partitioned);
        clothBuffer->setCloths(partitioned);

        // ---- Upload particle data to GPU ----
        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* devPos   = clothBuffer->getPositionInvMasses();
            physx::PxVec4* devVel   = clothBuffer->getVelocities();
            physx::PxU32*  devPhase = clothBuffer->getPhases();
            physx::PxVec4* devRest  = clothBuffer->getRestPositions();

            physx::PxVec4* pinnedPos   = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxVec4* pinnedVel   = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxU32*  pinnedPhase = m_cudaContextManager->allocPinnedHostBuffer<physx::PxU32>(numParticles);
            physx::PxVec4* pinnedRest  = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);

            for (physx::PxU32 i = 0; i < numParticles; ++i)
            {
                pinnedPos[i]   = hostPositions[i];
                pinnedVel[i]   = hostVelocities[i];
                pinnedPhase[i] = hostPhases[i];
                pinnedRest[i]  = hostRestPositions[i];
            }

            m_cudaContextManager->copyHToD(devPos,   pinnedPos,   numParticles);
            m_cudaContextManager->copyHToD(devVel,   pinnedVel,   numParticles);
            m_cudaContextManager->copyHToD(devPhase, pinnedPhase, numParticles);
            m_cudaContextManager->copyHToD(devRest,  pinnedRest,  numParticles);

            m_cudaContextManager->freePinnedHostBuffer(pinnedPos);
            m_cudaContextManager->freePinnedHostBuffer(pinnedVel);
            m_cudaContextManager->freePinnedHostBuffer(pinnedPhase);
            m_cudaContextManager->freePinnedHostBuffer(pinnedRest);
        }

        clothBuffer->setNbActiveParticles(numParticles);
        clothBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        clothBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        clothBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        clothBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_RESTPOSITION);
        clothBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_CLOTH);

        // ---- Configure solver for reassembly ----
        // High solver iterations: GPU resolves all spring + cohesion constraints in parallel.
        // This is where the GPU parallelism matters — every constraint is processed simultaneously.
        m_particleSystem->setSolverIterationCounts(64, 4);

        // Gentle +x gravity: provides directional ordering (sequence flow).
        // Combined with the pinned stream_start anchor at origin,
        // the chain extends rightward from the anchor point.
        const physx::PxVec3 origGravity = m_pxScene->getGravity();
        m_pxScene->setGravity(physx::PxVec3(0.05f, 0.0f, 0.0f));

        // Add cloth buffer to particle system
        m_particleSystem->addParticleBuffer(clothBuffer);

        // ---- Simulate: PBD solver resolves all constraints in parallel on GPU ----
        // Each step: GPU processes ALL springs + ALL cohesion + ALL collision simultaneously.
        // 64 solver iterations per step × 20 steps = 1280 total constraint resolution passes.
        const int numSteps = 20;
        const float dt = 1.0f / 60.0f;

        for (int step = 0; step < numSteps; ++step)
        {
            m_pxScene->simulate(dt);
            m_pxScene->fetchResults(true);
            m_pxScene->fetchResultsParticleSystem();
        }

        // ---- Read back final particle positions from GPU ----
        // After simulation, particles have settled into equilibrium:
        // - Same-token particles clustered via cohesion
        // - Dumbbell pairs connected via springs
        // - Chain ordered left-to-right via gravity + anchor
        struct DumbbellResult
        {
            float aX;
            physx::PxU32 index;
        };

        AZStd::vector<DumbbellResult> dumbbells(totalSprings);

        {
            physx::PxScopedCudaLock lock(*m_cudaContextManager);

            physx::PxVec4* devPos = clothBuffer->getPositionInvMasses();
            physx::PxVec4* pinnedPos = m_cudaContextManager->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            m_cudaContextManager->copyDToH(pinnedPos, devPos, numParticles);

            for (physx::PxU32 d = 0; d < totalSprings; ++d)
            {
                dumbbells[d].aX = pinnedPos[d * 2].x;
                dumbbells[d].index = d;
            }

            m_cudaContextManager->freePinnedHostBuffer(pinnedPos);
        }

        // ---- Sort dumbbells by A-side x position = sequence order ----
        // The physics determined the ordering. We just read it.
        AZStd::sort(dumbbells.begin(), dumbbells.end(),
            [](const DumbbellResult& a, const DumbbellResult& b) { return a.aX < b.aX; });

        // ---- Extract token sequence ----
        // Each sorted dumbbell contributes its A-side token.
        // The last dumbbell also contributes its B-side (final token in sequence).
        sequence.reserve(totalSprings + 1);
        for (physx::PxU32 d = 0; d < totalSprings; ++d)
        {
            const physx::PxU32 origIdx = dumbbells[d].index;
            sequence.push_back(particleTokenIds[origIdx * 2]);
        }
        if (!dumbbells.empty())
        {
            const physx::PxU32 lastIdx = dumbbells[totalSprings - 1].index;
            sequence.push_back(particleTokenIds[lastIdx * 2 + 1]);
        }

        // ---- Cleanup ----
        m_particleSystem->removeParticleBuffer(clothBuffer);
        clothBuffer->release();
        preprocessor->release();
        // partitioned cleaned up by PxPartitionedParticleCloth destructor

        // Restore original solver and gravity settings for disassembly
        m_particleSystem->setSolverIterationCounts(4, 1);
        m_pxScene->setGravity(origGravity);

        AZLOG_INFO("HCPParticlePipeline: Reassembled %zu bonds into %zu token sequence via PBD physics (%u particles, %d steps)",
            pbmData.bonds.size(), sequence.size(), numParticles, numSteps);

        return sequence;
    }

} // namespace HCPEngine
