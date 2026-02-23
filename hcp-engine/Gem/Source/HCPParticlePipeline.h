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

    //! Result of PBM disassembly (used for aggregate inference, not per-doc storage)
    struct PBMData
    {
        AZStd::vector<Bond> bonds;
        AZStd::string firstFpbA;  // First forward pair bond A-side
        AZStd::string firstFpbB;  // First forward pair bond B-side
        size_t totalPairs = 0;
        size_t uniqueTokens = 0;
    };

    //! Per-token position entry: a token ID and all positions where it occurs.
    struct TokenPositions
    {
        AZStd::string tokenId;
        AZStd::vector<AZ::u32> positions;
    };

    //! Position-based document representation.
    //! Each unique token maps to the list of positions where it appears.
    //! Disassembly = record positions. Reassembly = place tokens at positions.
    //! PBM bond counts are derived at aggregate time, not stored per-document.
    struct PositionMap
    {
        AZStd::vector<TokenPositions> entries;
        AZ::u32 totalTokens = 0;     // total positions in the document
        size_t uniqueTokens = 0;      // number of distinct token IDs
    };

    //! Position-based disassembly: token stream -> position map.
    //! Positions include space gaps from the tokenizer. No physics needed.
    PositionMap DisassemblePositions(const TokenStream& stream);

    //! Position-based reassembly: position map -> token sequence with positions.
    //! Gaps in positions = spaces. No physics needed.
    TokenStream ReassemblePositions(const PositionMap& posMap);

    //! Derive PBM bond data from a token stream (for aggregate pipeline).
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

        //! Run reassembly: bond map -> token sequence via PBD particle physics.
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
