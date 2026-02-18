#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

// Forward declarations — full PhysX headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxFoundation;
    class PxScene;
    class PxPBDParticleSystem;
    class PxParticleMaterial;
    class PxCudaContextManager;
    class PxDefaultCpuDispatcher;
}

namespace HCPEngine
{
    class HCPVocabulary;

    //! A bond triple: (token_a, token_b, count)
    struct Bond
    {
        AZStd::string tokenA;
        AZStd::string tokenB;
        int count;
    };

    //! Result of PBM disassembly
    struct PBMData
    {
        AZStd::vector<Bond> bonds;
        AZStd::string firstFpbA;  // First forward pair bond A-side
        AZStd::string firstFpbB;  // First forward pair bond B-side
        size_t totalPairs = 0;
        size_t uniqueTokens = 0;
    };

    //! The particle pipeline: manages PhysX PBD particle system for
    //! disassembly (text -> bonds) and reassembly (bonds -> text).
    class HCPParticlePipeline
    {
    public:
        HCPParticlePipeline() = default;
        ~HCPParticlePipeline();

        //! Initialize PhysX PBD subsystem: CUDA context, GPU scene, particle system.
        bool Initialize(physx::PxPhysics* pxPhysics, physx::PxFoundation* pxFoundation);

        //! Shut down PBD subsystem and release resources
        void Shutdown();

        //! Run disassembly: token sequence -> pair bond map via PBD particle physics.
        PBMData Disassemble(const AZStd::vector<AZStd::string>& tokenIds);

        //! Run reassembly: bond map -> token sequence via PBD particle physics.
        AZStd::vector<AZStd::string> Reassemble(
            const PBMData& pbmData,
            const HCPVocabulary& vocab);

        bool IsInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;

        physx::PxPhysics* m_pxPhysics = nullptr;
        physx::PxCudaContextManager* m_cudaContextManager = nullptr;
        physx::PxScene* m_pxScene = nullptr;
        physx::PxPBDParticleSystem* m_particleSystem = nullptr;
        physx::PxParticleMaterial* m_particleMaterial = nullptr;
        physx::PxParticleMaterial* m_reassemblyMaterial = nullptr;
        physx::PxDefaultCpuDispatcher* m_cpuDispatcher = nullptr;

        // Callback data for disassembly bond counting
        AZStd::vector<AZStd::string> m_particleTokenIds;
        AZStd::unordered_map<AZStd::string, int> m_bondCounts;
    };

} // namespace HCPEngine
