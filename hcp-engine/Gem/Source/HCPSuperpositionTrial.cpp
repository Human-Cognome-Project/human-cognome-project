#include "HCPSuperpositionTrial.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <chrono>
#include <cstdio>
#include <cmath>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>

namespace HCPEngine
{
    // ---- UTF-8 ↔ Codepoint conversion ----

    AZStd::vector<AZ::u32> DecodeUtf8ToCodepoints(const AZStd::string& input)
    {
        AZStd::vector<AZ::u32> codepoints;
        codepoints.reserve(input.size());

        const size_t len = input.size();
        size_t i = 0;
        while (i < len)
        {
            AZ::u32 cp;
            unsigned char b0 = static_cast<unsigned char>(input[i]);

            if (b0 < 0x80)
            {
                // 1-byte: 0xxxxxxx
                cp = b0;
                i += 1;
            }
            else if ((b0 & 0xE0) == 0xC0)
            {
                // 2-byte: 110xxxxx 10xxxxxx
                if (i + 1 < len &&
                    (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80)
                {
                    cp = (static_cast<AZ::u32>(b0 & 0x1F) << 6)
                       | (static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 1])) & 0x3F);
                    i += 2;
                }
                else
                {
                    cp = 0xFFFD;
                    i += 1;
                }
            }
            else if ((b0 & 0xF0) == 0xE0)
            {
                // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
                if (i + 2 < len &&
                    (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80)
                {
                    cp = (static_cast<AZ::u32>(b0 & 0x0F) << 12)
                       | ((static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 1])) & 0x3F) << 6)
                       | (static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 2])) & 0x3F);
                    i += 3;
                }
                else
                {
                    cp = 0xFFFD;
                    i += 1;
                }
            }
            else if ((b0 & 0xF8) == 0xF0)
            {
                // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                if (i + 3 < len &&
                    (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(input[i + 3]) & 0xC0) == 0x80)
                {
                    cp = (static_cast<AZ::u32>(b0 & 0x07) << 18)
                       | ((static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 1])) & 0x3F) << 12)
                       | ((static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 2])) & 0x3F) << 6)
                       | (static_cast<AZ::u32>(static_cast<unsigned char>(input[i + 3])) & 0x3F);
                    i += 4;
                }
                else
                {
                    cp = 0xFFFD;
                    i += 1;
                }
            }
            else
            {
                // Invalid leading byte (continuation byte or 0xFE/0xFF)
                cp = 0xFFFD;
                i += 1;
            }

            codepoints.push_back(cp);
        }

        return codepoints;
    }

    void AppendCodepointAsUtf8(AZStd::string& out, AZ::u32 cp)
    {
        if (cp < 0x80)
        {
            out += static_cast<char>(cp);
        }
        else if (cp < 0x800)
        {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp <= 0x10FFFF)
        {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    // ---- Layout parameters ----
    static constexpr float Z_SCALE = 10.0f;        // Codepoint value → Z position scaling
    static constexpr float Y_OFFSET = 1.5f;        // Initial Y height (dynamic particles fall from here)
    static constexpr float SETTLE_Y = 0.5f;         // |Y| below this = settled on codepoint particle
    static constexpr int MAX_STEPS = 60;            // Simulation steps
    static constexpr float DT = 1.0f / 60.0f;      // Time step

    // PBD contact parameters — particleContactOffset is per-particle radius.
    // Two particles interact when distance < 2 * contactOffset.
    // With integer X spacing (stream positions), contactOffset < 0.5 ensures
    // particles at adjacent stream positions don't interact (distance 1.0 > 2*0.4).
    static constexpr float PARTICLE_CONTACT_OFFSET = 0.4f;
    static constexpr float PARTICLE_REST_OFFSET = 0.1f;

    // Chunk size for batched processing.
    // PhysX PBD buffers have a ~65K particle limit. Each codepoint needs 2 particles
    // (static codepoint + dynamic input), so 16K codepoints = 32K particles per chunk.
    static constexpr AZ::u32 CHUNK_SIZE = 16384;

    // ---- Process a single chunk of codepoints through PBD ----
    // Returns true on success and populates collapses for indices [chunkStart, chunkStart+chunkLen).

    static bool ProcessChunk(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::vector<AZ::u32>& codepoints,
        AZ::u32 chunkStart,
        AZ::u32 chunkLen,
        AZStd::vector<CollapseResult>& collapses,
        AZ::u32& settledOut,
        AZ::u32& unsettledOut)
    {
        const physx::PxU32 N = chunkLen;
        const physx::PxU32 totalParticles = 2 * N;

        // ---- Create PBD system for this chunk ----
        physx::PxPBDParticleSystem* particleSystem =
            physics->createPBDParticleSystem(*cuda, 96);
        if (!particleSystem)
            return false;

        particleSystem->setRestOffset(PARTICLE_REST_OFFSET);
        particleSystem->setContactOffset(PARTICLE_CONTACT_OFFSET);
        particleSystem->setParticleContactOffset(PARTICLE_CONTACT_OFFSET);
        particleSystem->setSolidRestOffset(PARTICLE_REST_OFFSET);
        particleSystem->setSolverIterationCounts(4, 1);
        scene->addActor(*particleSystem);

        physx::PxPBDMaterial* pbdMaterial = physics->createPBDMaterial(
            0.2f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!pbdMaterial)
        {
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return false;
        }

        const physx::PxU32 phase = particleSystem->createPhase(
            pbdMaterial,
            physx::PxParticlePhaseFlags(physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));

        physx::PxParticleBuffer* particleBuffer = physics->createParticleBuffer(
            totalParticles, 1, cuda);
        if (!particleBuffer)
        {
            pbdMaterial->release();
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return false;
        }

        // ---- Initialize particles ----
        // X positions are chunk-local (0..N-1) — no cross-chunk interactions.
        {
            physx::PxScopedCudaLock lock(*cuda);

            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = particleBuffer->getVelocities();
            physx::PxU32* devPhase = particleBuffer->getPhases();

            physx::PxVec4* hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            physx::PxVec4* hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            physx::PxU32* hostPhase = cuda->allocPinnedHostBuffer<physx::PxU32>(totalParticles);

            for (physx::PxU32 i = 0; i < N; ++i)
            {
                AZ::u32 cp = codepoints[chunkStart + i];
                float x = static_cast<float>(i);
                float z = static_cast<float>(cp) * Z_SCALE;

                // Codepoint particle: static
                hostPos[i] = physx::PxVec4(x, 0.0f, z, 0.0f);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = phase;

                // Input particle: dynamic
                hostPos[N + i] = physx::PxVec4(x, Y_OFFSET, z, 1.0f);
                hostVel[N + i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[N + i] = phase;
            }

            cuda->copyHToD(devPos, hostPos, totalParticles);
            cuda->copyHToD(devVel, hostVel, totalParticles);
            cuda->copyHToD(devPhase, hostPhase, totalParticles);

            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
            cuda->freePinnedHostBuffer(hostPhase);
        }

        particleBuffer->setNbActiveParticles(totalParticles);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        particleSystem->addParticleBuffer(particleBuffer);

        // ---- Simulate ----
        for (int step = 0; step < MAX_STEPS; ++step)
        {
            scene->simulate(DT);
            scene->fetchResults(true);
            scene->fetchResultsParticleSystem();
        }

        // ---- Read back and classify ----
        physx::PxVec4* hostPos = nullptr;
        {
            physx::PxScopedCudaLock lock(*cuda);
            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            cuda->copyDToH(hostPos, devPos, totalParticles);
        }

        settledOut = 0;
        unsettledOut = 0;

        for (physx::PxU32 i = 0; i < N; ++i)
        {
            AZ::u32 cp = codepoints[chunkStart + i];

            CollapseResult& cr = collapses[chunkStart + i];
            cr.streamPos = chunkStart + i;   // Global codepoint position
            cr.codepoint = cp;
            cr.resolvedCodepoint = cp;
            cr.finalY = hostPos[N + i].y;
            cr.settled = (fabsf(hostPos[N + i].y) < SETTLE_Y);

            if (cr.settled)
                ++settledOut;
            else
                ++unsettledOut;
        }

        {
            physx::PxScopedCudaLock lock(*cuda);
            cuda->freePinnedHostBuffer(hostPos);
        }

        // ---- Cleanup ----
        particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        pbdMaterial->release();
        scene->removeActor(*particleSystem);
        particleSystem->release();

        return true;
    }

    // ---- Main trial function (chunked codepoint architecture) ----

    SuperpositionTrialResult RunSuperpositionTrial(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::string& inputText,
        const HCPVocabulary& vocab,
        AZ::u32 maxChars)
    {
        SuperpositionTrialResult result;

        if (!physics || !scene || !cuda || inputText.empty())
            return result;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Decode UTF-8 → codepoints
        AZStd::vector<AZ::u32> codepoints = DecodeUtf8ToCodepoints(inputText);
        result.totalBytes = static_cast<AZ::u32>(inputText.size());

        // Truncate at codepoint level
        if (codepoints.size() > maxChars)
            codepoints.resize(maxChars);

        const AZ::u32 N = static_cast<AZ::u32>(codepoints.size());
        result.totalCodepoints = N;
        result.collapses.resize(N);

        // Calculate chunks
        AZ::u32 numChunks = (N + CHUNK_SIZE - 1) / CHUNK_SIZE;

        fprintf(stderr, "[SuperpositionTrial] Input: %u bytes → %u codepoints, %u chunks of up to %u\n",
            result.totalBytes, N, numChunks, CHUNK_SIZE);
        fflush(stderr);

        // ---- Process each chunk ----
        for (AZ::u32 chunk = 0; chunk < numChunks; ++chunk)
        {
            AZ::u32 chunkStart = chunk * CHUNK_SIZE;
            AZ::u32 chunkLen = CHUNK_SIZE;
            if (chunkStart + chunkLen > N)
                chunkLen = N - chunkStart;

            AZ::u32 chunkSettled = 0, chunkUnsettled = 0;

            bool ok = ProcessChunk(
                physics, scene, cuda,
                codepoints, chunkStart, chunkLen,
                result.collapses,
                chunkSettled, chunkUnsettled);

            if (!ok)
            {
                fprintf(stderr, "[SuperpositionTrial] ERROR: Chunk %u/%u failed (codepoints %u..%u)\n",
                    chunk + 1, numChunks, chunkStart, chunkStart + chunkLen - 1);
                fflush(stderr);
                // Mark remaining codepoints as unsettled
                for (AZ::u32 i = chunkStart; i < chunkStart + chunkLen; ++i)
                {
                    result.collapses[i].streamPos = i;
                    result.collapses[i].codepoint = codepoints[i];
                    result.collapses[i].resolvedCodepoint = codepoints[i];
                    result.collapses[i].finalY = Y_OFFSET;
                    result.collapses[i].settled = false;
                    ++result.unsettledCount;
                }
                continue;
            }

            result.settledCount += chunkSettled;
            result.unsettledCount += chunkUnsettled;

            fprintf(stderr, "[SuperpositionTrial] Chunk %u/%u: %u/%u settled (codepoints %u..%u)\n",
                chunk + 1, numChunks, chunkSettled, chunkLen,
                chunkStart, chunkStart + chunkLen - 1);
            fflush(stderr);
        }

        result.simulationSteps = MAX_STEPS;

        auto endTime = std::chrono::high_resolution_clock::now();
        result.simulationTimeMs = static_cast<float>(
            std::chrono::duration<double, std::milli>(endTime - startTime).count());

        // ---- Report ----
        fprintf(stderr, "\n[SuperpositionTrial] ====== CODEPOINT SETTLEMENT (chunked, %u chunks) ======\n",
            numChunks);
        fprintf(stderr, "[SuperpositionTrial] Input: %u bytes → %u codepoints\n",
            result.totalBytes, result.totalCodepoints);
        fprintf(stderr, "[SuperpositionTrial] Settled: %u / %u (%.1f%%)\n",
            result.settledCount, result.totalCodepoints,
            result.totalCodepoints > 0 ? 100.0f * result.settledCount / result.totalCodepoints : 0.0f);
        fprintf(stderr, "[SuperpositionTrial] Unsettled: %u\n", result.unsettledCount);
        fprintf(stderr, "[SuperpositionTrial] Steps per chunk: %d | Total time: %.1f ms\n",
            result.simulationSteps, result.simulationTimeMs);

        // Show unsettled codepoints (if any, capped at 50)
        if (result.unsettledCount > 0)
        {
            fprintf(stderr, "\n[SuperpositionTrial] Unsettled codepoints (first 50):\n");
            AZ::u32 shown = 0;
            for (const auto& cr : result.collapses)
            {
                if (!cr.settled)
                {
                    char display = (cr.codepoint >= 32 && cr.codepoint < 127)
                        ? static_cast<char>(cr.codepoint) : '?';
                    fprintf(stderr, "  [%5u] U+%04X '%c'  Y=%.4f\n",
                        cr.streamPos, cr.codepoint, display, cr.finalY);
                    if (++shown >= 50) break;
                }
            }
            if (result.unsettledCount > 50)
                fprintf(stderr, "  ... and %u more\n", result.unsettledCount - 50);
        }

        // ---- Validation: compare against computational tokenizer ----
        AZStd::string text = inputText;
        if (text.size() > maxChars)
            text = text.substr(0, maxChars);
        TokenStream compStream = Tokenize(text, vocab);
        fprintf(stderr, "\n[SuperpositionTrial] Computational tokenizer: %zu tokens from same input\n",
            compStream.tokenIds.size());
        fprintf(stderr, "[SuperpositionTrial] ================================\n");
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
