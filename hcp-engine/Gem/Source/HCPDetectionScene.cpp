#include "HCPDetectionScene.h"
#include "HCPBondCompiler.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>

namespace HCPEngine
{
    // Convert byte to 2-char hex string for byte->char bond lookup
    static AZStd::string ByteToHex(uint8_t b)
    {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", b);
        return AZStd::string(hex, 2);
    }

    DetectionResult RunDetection(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const uint8_t* bytes, size_t len,
        const HCPBondTable& byteCharBonds,
        const HCPBondTable& charWordBonds)
    {
        DetectionResult result;
        result.totalBytes = static_cast<AZ::u32>(len);

        if (!physics || !scene || !cuda || !bytes || len == 0)
        {
            return result;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        const physx::PxU32 numParticles = static_cast<physx::PxU32>(len);
        fprintf(stderr, "[HCPDetection] Starting: %u bytes as particles\n", numParticles);
        fflush(stderr);

        // ---- Create PBD particle system ----
        // NOTE: PxParticleClothBuffer (native springs) tested but non-functional in PhysX 5.1.1.
        // Rest positions override spring forces — particles don't move.
        // Using manual D→H→H→D force injection until onAdvance + CUDA kernel path is available.
        // Requires: apt install nvidia-cuda-toolkit (nvcc) for custom GPU kernels.
        physx::PxPBDParticleSystem* particleSystem =
            physics->createPBDParticleSystem(*cuda, 96);
        if (!particleSystem)
        {
            fprintf(stderr, "[HCPDetection] ERROR: Failed to create PBD particle system\n");
            fflush(stderr);
            return result;
        }

        const float spacing = 1.0f;
        particleSystem->setRestOffset(0.3f);
        particleSystem->setContactOffset(0.4f);
        particleSystem->setParticleContactOffset(1.5f);
        particleSystem->setSolidRestOffset(0.3f);
        particleSystem->setSolverIterationCounts(4, 1);
        scene->addActor(*particleSystem);

        physx::PxPBDMaterial* material = physics->createPBDMaterial(
            0.1f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!material)
        {
            fprintf(stderr, "[HCPDetection] ERROR: Failed to create PBD material\n");
            fflush(stderr);
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return result;
        }

        const physx::PxU32 phase = particleSystem->createPhase(
            material,
            physx::PxParticlePhaseFlags(physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));

        // ---- Create and initialize particles ----
        physx::PxParticleBuffer* particleBuffer = physics->createParticleBuffer(
            numParticles, 1, cuda);
        if (!particleBuffer)
        {
            fprintf(stderr, "[HCPDetection] ERROR: Failed to create particle buffer (%u particles)\n", numParticles);
            fflush(stderr);
            material->release();
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return result;
        }

        {
            physx::PxScopedCudaLock lock(*cuda);

            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = particleBuffer->getVelocities();
            physx::PxU32* devPhase = particleBuffer->getPhases();

            physx::PxVec4* hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxVec4* hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            physx::PxU32* hostPhase = cuda->allocPinnedHostBuffer<physx::PxU32>(numParticles);

            for (physx::PxU32 i = 0; i < numParticles; ++i)
            {
                // Spaces and whitespace are force insulators — pinned in place (invMass = 0).
                bool isInsulator = (bytes[i] == ' ' || bytes[i] == '\t' ||
                                    bytes[i] == '\n' || bytes[i] == '\r');
                float invMass = isInsulator ? 0.0f : 1.0f;

                hostPos[i] = physx::PxVec4(static_cast<float>(i) * spacing, 0.0f, 0.0f, invMass);
                hostVel[i] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                hostPhase[i] = phase;
            }

            cuda->copyHToD(devPos, hostPos, numParticles);
            cuda->copyHToD(devVel, hostVel, numParticles);
            cuda->copyHToD(devPhase, hostPhase, numParticles);

            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
            cuda->freePinnedHostBuffer(hostPhase);
        }

        particleBuffer->setNbActiveParticles(numParticles);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_POSITION);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
        particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_PHASE);
        particleSystem->addParticleBuffer(particleBuffer);

        // ---- Precompute bond strengths for adjacent byte pairs ----
        AZStd::vector<float> adjacentStrength(len > 0 ? len - 1 : 0, 0.0f);
        float maxStrength = 1.0f;

        for (size_t i = 0; i + 1 < len; ++i)
        {
            uint8_t bI = bytes[i];
            uint8_t bJ = bytes[i + 1];
            AZ::u32 strength = 0;

            if (bI < 128 && bJ < 128 && bI >= 32 && bJ >= 32)
            {
                char cI = static_cast<char>(bI);
                char cJ = static_cast<char>(bJ);
                if (cI >= 'A' && cI <= 'Z') cI = cI - 'A' + 'a';
                if (cJ >= 'A' && cJ <= 'Z') cJ = cJ - 'A' + 'a';
                AZStd::string sI(1, cI);
                AZStd::string sJ(1, cJ);
                strength = charWordBonds.GetBondStrength(sI, sJ);
            }

            if (strength == 0)
            {
                strength = byteCharBonds.GetBondStrength(ByteToHex(bI), ByteToHex(bJ));
            }

            adjacentStrength[i] = static_cast<float>(strength);
            if (static_cast<float>(strength) > maxStrength)
            {
                maxStrength = static_cast<float>(strength);
            }
        }

        fprintf(stderr, "[HCPDetection] Precomputed %zu bond strengths, max = %.0f\n",
            adjacentStrength.size(), maxStrength);
        fflush(stderr);

        // ---- Simulation loop with force injection ----
        // Closed energy system — zero gravity. Forces come only from PBM bonds.
        const physx::PxVec3 origGravity = scene->getGravity();
        scene->setGravity(physx::PxVec3(0.0f, 0.0f, 0.0f));

        const int maxSteps = 60;
        const float dt = 1.0f / 60.0f;
        const float attractionScale = 5.0f;
        int stepsUsed = 0;

        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        {
            physx::PxScopedCudaLock lock(*cuda);
            hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
            hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(numParticles);
        }

        for (int step = 0; step < maxSteps; ++step)
        {
            scene->simulate(dt);
            scene->fetchResults(true);
            scene->fetchResultsParticleSystem();

            // ---- Force injection between steps ----
            {
                physx::PxScopedCudaLock lock(*cuda);

                physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
                physx::PxVec4* devVel = particleBuffer->getVelocities();

                cuda->copyDToH(hostPos, devPos, numParticles);
                cuda->copyDToH(hostVel, devVel, numParticles);

                struct SortEntry { float x; physx::PxU32 orig; };
                AZStd::vector<SortEntry> sorted(numParticles);
                for (physx::PxU32 i = 0; i < numParticles; ++i)
                {
                    sorted[i] = { hostPos[i].x, i };
                }
                AZStd::sort(sorted.begin(), sorted.end(),
                    [](const SortEntry& a, const SortEntry& b) { return a.x < b.x; });

                const float contactDist = spacing * 2.0f;

                for (physx::PxU32 si = 0; si + 1 < numParticles; ++si)
                {
                    for (physx::PxU32 sj = si + 1; sj < numParticles; ++sj)
                    {
                        float dx = sorted[sj].x - sorted[si].x;
                        if (dx > contactDist) break;

                        physx::PxU32 origI = sorted[si].orig;
                        physx::PxU32 origJ = sorted[sj].orig;
                        if (origI > origJ) { physx::PxU32 tmp = origI; origI = origJ; origJ = tmp; }
                        if (origJ != origI + 1) continue;

                        if (bytes[origI] == ' ' || bytes[origI] == '\t' ||
                            bytes[origI] == '\n' || bytes[origI] == '\r' ||
                            bytes[origJ] == ' ' || bytes[origJ] == '\t' ||
                            bytes[origJ] == '\n' || bytes[origJ] == '\r')
                            continue;

                        float bondStr = adjacentStrength[origI];
                        physx::PxU32 realI = sorted[si].orig;
                        physx::PxU32 realJ = sorted[sj].orig;
                        float dir = (hostPos[realJ].x > hostPos[realI].x) ? 1.0f : -1.0f;

                        if (bondStr > 0.0f)
                        {
                            float normalized = bondStr / maxStrength;
                            float impulse = normalized * attractionScale * dt;
                            hostVel[realI].x += dir * impulse;
                            hostVel[realJ].x -= dir * impulse;
                        }
                        else
                        {
                            float repulse = attractionScale * 0.3f * dt;
                            hostVel[realI].x -= dir * repulse;
                            hostVel[realJ].x += dir * repulse;
                        }
                    }
                }

                cuda->copyHToD(devVel, hostVel, numParticles);
                particleBuffer->raiseFlags(physx::PxParticleBufferFlag::eUPDATE_VELOCITY);
            }

            stepsUsed = step + 1;

            if ((step + 1) % 20 == 0)
            {
                fprintf(stderr, "[HCPDetection] Step %d/%d\n", step + 1, maxSteps);
                fflush(stderr);
            }
        }

        // ---- Read final positions ----
        {
            physx::PxScopedCudaLock lock(*cuda);
            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            cuda->copyDToH(hostPos, devPos, numParticles);
        }

        // ---- Identify clusters ----
        struct FinalEntry { float x; physx::PxU32 orig; };
        AZStd::vector<FinalEntry> finalOrder(numParticles);
        for (physx::PxU32 i = 0; i < numParticles; ++i)
        {
            finalOrder[i] = { hostPos[i].x, i };
        }
        AZStd::sort(finalOrder.begin(), finalOrder.end(),
            [](const FinalEntry& a, const FinalEntry& b) { return a.x < b.x; });

        const float gapThreshold = spacing * 0.7f;

        if (numParticles > 0)
        {
            AZStd::vector<AZStd::vector<AZ::u32>> clusterIndices;
            clusterIndices.push_back({finalOrder[0].orig});

            for (physx::PxU32 i = 1; i < numParticles; ++i)
            {
                float gap = finalOrder[i].x - finalOrder[i - 1].x;

                AZ::u32 prevOrig = finalOrder[i - 1].orig;
                AZ::u32 currOrig = finalOrder[i].orig;
                bool zeroBond = false;
                if (currOrig == prevOrig + 1 && prevOrig < adjacentStrength.size())
                {
                    zeroBond = (adjacentStrength[prevOrig] <= 0.0f);
                }
                else if (prevOrig == currOrig + 1 && currOrig < adjacentStrength.size())
                {
                    zeroBond = (adjacentStrength[currOrig] <= 0.0f);
                }

                if (gap > gapThreshold || zeroBond)
                {
                    clusterIndices.push_back({});
                }
                clusterIndices.back().push_back(finalOrder[i].orig);
            }

            for (auto& indices : clusterIndices)
            {
                AZStd::sort(indices.begin(), indices.end());

                DetectedCluster cluster;
                cluster.startByte = indices.front();
                cluster.endByte = indices.back() + 1;

                for (AZ::u32 idx : indices)
                {
                    cluster.text += static_cast<char>(bytes[idx]);
                }
                result.clusters.push_back(AZStd::move(cluster));
            }
        }

        // ---- Cleanup ----
        {
            physx::PxScopedCudaLock lock(*cuda);
            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
        }

        particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        material->release();
        scene->removeActor(*particleSystem);
        particleSystem->release();
        scene->setGravity(origGravity);

        result.simulationSteps = stepsUsed;
        auto endTime = std::chrono::high_resolution_clock::now();
        result.simulationTimeMs = static_cast<float>(
            std::chrono::duration<double, std::milli>(endTime - startTime).count());

        fprintf(stderr, "[HCPDetection] Complete: %zu clusters, %d steps, %.1f ms\n",
            result.clusters.size(), stepsUsed, result.simulationTimeMs);
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
