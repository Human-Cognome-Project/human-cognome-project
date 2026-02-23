#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

// Forward declarations — full PhysX headers only in .cpp
namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxCudaContextManager;
}

namespace HCPEngine
{
    class HCPBondTable;

    //! A detected cluster of bytes that form a token (character, word, etc.)
    struct DetectedCluster
    {
        AZ::u32 startByte;          // First byte index in original stream
        AZ::u32 endByte;            // One past last byte index
        AZStd::string text;         // Decoded text of this cluster
    };

    //! Result of physics-based token detection.
    struct DetectionResult
    {
        AZStd::vector<DetectedCluster> clusters;
        AZ::u32 totalBytes = 0;
        int simulationSteps = 0;
        float simulationTimeMs = 0.0f;
    };

    //! Run physics-based token detection on a raw byte stream.
    //!
    //! Each byte enters as a PBD particle. PBM bond forces from the bond tables
    //! attract adjacent bytes/characters that commonly co-occur. Both bond tables
    //! (byte->char AND char->word) are active simultaneously — the physics engine
    //! naturally cascades: multi-byte UTF-8 sequences cluster first (strong byte->char
    //! bonds), then characters cluster into words (char->word bonds).
    //!
    //! This is a closed energy system (zero gravity). FEM finds the zero-loss
    //! configuration = correct match.
    //!
    //! @param physics        PxPhysics instance
    //! @param scene          GPU-enabled PxScene
    //! @param cuda           CUDA context manager
    //! @param bytes          Raw input byte stream
    //! @param len            Length of byte stream
    //! @param byteCharBonds  Byte->character PBM bond table
    //! @param charWordBonds  Character->word PBM bond table
    //! @return Detection result with identified clusters
    DetectionResult RunDetection(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const uint8_t* bytes, size_t len,
        const HCPBondTable& byteCharBonds,
        const HCPBondTable& charWordBonds);

} // namespace HCPEngine
