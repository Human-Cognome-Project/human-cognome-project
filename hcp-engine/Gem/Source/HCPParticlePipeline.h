#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include "HCPTokenizer.h"

// Forward declarations — full PhysX headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxFoundation;
    class PxScene;
    class PxPBDParticleSystem;
    class PxParticleMaterial;
    class PxCudaContextManager;
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

    //! Pair Bond Map — the stored form of a document.
    //! Each bond is (A, B, count) representing adjacent token pair occurrences.
    //! The set of bonds IS the document. Physics reassembly recovers the
    //! unique ordering from the bond constraints.
    struct PBMData
    {
        AZStd::vector<Bond> bonds;
        AZStd::string firstFpbA;  // First forward pair bond A-side
        AZStd::string firstFpbB;  // First forward pair bond B-side
        size_t totalPairs = 0;
        size_t uniqueTokens = 0;
    };

    //! Derive PBM bond data from a token stream.
    //! Counts adjacent pairs — consecutive tokens in the stream form bonds.
    PBMData DerivePBM(const TokenStream& stream);

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

        //! Run reassembly: bond map -> ordered token sequence via PBD particle physics.
        //! Dumbbells dissolve when both ends find their mates.
        //! Whitespace is re-inserted at dissolution points unless
        //! stickiness rules suppress it (punctuation sticks to adjacent tokens).
        //! @return Ordered string sequence including whitespace
        AZStd::vector<AZStd::string> Reassemble(
            const PBMData& pbmData,
            const HCPVocabulary& vocab);

        bool IsInitialized() const { return m_initialized; }

        // Resource accessors — for detection scene and other physics operations
        physx::PxPhysics* GetPhysics() const { return m_pxPhysics; }
        physx::PxScene* GetScene() const { return m_pxScene; }
        physx::PxCudaContextManager* GetCuda() const { return m_cudaContextManager; }

    private:
        bool m_initialized = false;

        physx::PxPhysics* m_pxPhysics = nullptr;
        physx::PxCudaContextManager* m_cudaContextManager = nullptr;
        physx::PxScene* m_pxScene = nullptr;
        physx::PxParticleMaterial* m_particleMaterial = nullptr;
        physx::PxParticleMaterial* m_reassemblyMaterial = nullptr;

        // Working data for disassembly bond counting
        AZStd::unordered_map<AZStd::string, int> m_bondCounts;
    };

} // namespace HCPEngine
