#include "HCPWordSuperpositionTrial.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cctype>

#include <PxPhysicsAPI.h>
#include <PxParticleGpu.h>
#include <gpu/PxGpu.h>

namespace HCPEngine
{
    // ---- Phase 2 layout parameters ----
    static constexpr float P2_Z_SCALE = 10.0f;           // Char identity → Z position
    static constexpr float P2_Y_OFFSET = 1.5f;             // All dynamic particles start here
    static constexpr float P2_SETTLE_Y = 12.0f;            // |Y| below this = candidate for settled (generous for 30+ stacking)
    static constexpr float P2_VELOCITY_THRESHOLD = 3.0f;   // |Vy| below this = at rest (not mid-fall)
    static constexpr int P2_MAX_STEPS = 120;               // Simulation steps (all particles start at same Y, faster convergence)
    static constexpr float P2_DT = 1.0f / 60.0f;
    static constexpr float P2_RUN_X_GAP = 2.0f;           // Gap between runs on X axis

    // PBD contact — same as Phase 1
    static constexpr float P2_CONTACT_OFFSET = 0.4f;
    static constexpr float P2_REST_OFFSET = 0.1f;

    // ---- Text processing helpers ----

    static bool IsWhitespaceChar(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static bool IsPunctChar(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc < 128 && !std::isalnum(uc) && !IsWhitespaceChar(c);
    }

    // ---- Vocab index: (length, firstChar) → vector of (word, tokenId) ----

    struct VocabCandidate
    {
        AZStd::string word;
        AZStd::string tokenId;
    };

    struct VocabIndex
    {
        // Key: (length << 8) | firstCharLower
        AZStd::unordered_map<AZ::u32, AZStd::vector<VocabCandidate>> buckets;

        static AZ::u32 MakeKey(AZ::u32 len, char firstChar)
        {
            return (len << 8) | static_cast<AZ::u8>(firstChar);
        }

        const AZStd::vector<VocabCandidate>* Lookup(AZ::u32 len, char firstChar) const
        {
            auto it = buckets.find(MakeKey(len, firstChar));
            if (it == buckets.end()) return nullptr;
            return &it->second;
        }
    };

    static VocabIndex BuildVocabIndex(const HCPVocabulary& vocab)
    {
        VocabIndex index;
        vocab.IterateWords([&](const AZStd::string& word, const AZStd::string& tokenId) -> bool
        {
            if (word.empty()) return true;
            AZ::u32 len = static_cast<AZ::u32>(word.size());
            char first = static_cast<char>(std::tolower(static_cast<unsigned char>(word[0])));
            AZ::u32 key = VocabIndex::MakeKey(len, first);
            index.buckets[key].push_back({ word, tokenId });
            return true;
        });
        return index;
    }

    // ---- Run extraction ----

    AZStd::vector<CharRun> ExtractRuns(const AZStd::string& text, AZ::u32 maxChars)
    {
        AZStd::vector<CharRun> runs;
        AZ::u32 limit = static_cast<AZ::u32>(text.size());
        if (limit > maxChars) limit = maxChars;

        AZ::u32 i = 0;
        while (i < limit)
        {
            // Skip whitespace
            while (i < limit && IsWhitespaceChar(text[i]))
                ++i;
            if (i >= limit) break;

            // Collect chunk (to next whitespace)
            AZ::u32 chunkStart = i;
            while (i < limit && !IsWhitespaceChar(text[i]))
                ++i;

            // Strip edge punctuation
            AZ::u32 coreStart = chunkStart;
            while (coreStart < i && IsPunctChar(text[coreStart]))
                ++coreStart;
            AZ::u32 coreEnd = i;
            while (coreEnd > coreStart && IsPunctChar(text[coreEnd - 1]))
                --coreEnd;

            if (coreEnd <= coreStart) continue;

            // Track uppercase positions before lowercasing
            AZStd::vector<AZ::u32> upperPositions;
            for (AZ::u32 j = coreStart; j < coreEnd; ++j)
            {
                unsigned char uc = static_cast<unsigned char>(text[j]);
                if (std::isupper(uc))
                {
                    upperPositions.push_back(j - coreStart);
                }
            }

            // Lowercase the core
            AZStd::string core;
            core.reserve(coreEnd - coreStart);
            for (AZ::u32 j = coreStart; j < coreEnd; ++j)
            {
                unsigned char uc = static_cast<unsigned char>(text[j]);
                core += static_cast<char>(std::tolower(uc));
            }

            // Skip BOM bytes and non-ASCII for the trial
            bool allAscii = true;
            for (size_t j = 0; j < core.size(); ++j)
            {
                unsigned char uc = static_cast<unsigned char>(core[j]);
                if (uc >= 128) { allAscii = false; break; }
            }
            if (!allAscii) continue;

            CharRun run;
            run.text = core;
            run.startPos = coreStart;
            run.length = static_cast<AZ::u32>(core.size());

            // Populate normalization metadata
            if (upperPositions.size() == 1 && upperPositions[0] == 0)
            {
                // Label pattern: only first char uppercase
                run.firstCap = true;
                // capMask stays empty
            }
            else if (!upperPositions.empty())
            {
                // Unusual pattern (all-caps, camelCase, etc.)
                run.firstCap = false;
                run.capMask = AZStd::move(upperPositions);
            }
            // else: all lowercase — firstCap=false, capMask empty (defaults)

            runs.push_back(run);
        }

        return runs;
    }

    // ---- Extract runs from Phase 1 collapse results ----

    AZStd::vector<CharRun> ExtractRunsFromCollapses(const SuperpositionTrialResult& trialResult)
    {
        AZStd::vector<CharRun> runs;

        if (trialResult.collapses.empty())
            return runs;

        // Walk collapse results, accumulating settled alphanumeric chars into runs.
        // Boundaries: unsettled bytes, whitespace, end of stream.
        AZStd::string currentCore;
        AZStd::vector<AZ::u32> currentUpperPositions;
        AZ::u32 currentStart = 0;
        bool inRun = false;

        auto FlushRun = [&]()
        {
            if (!inRun || currentCore.empty())
            {
                inRun = false;
                currentCore.clear();
                currentUpperPositions.clear();
                return;
            }

            // Strip edge punctuation (same as ExtractRuns)
            AZ::u32 stripLeft = 0;
            while (stripLeft < currentCore.size() && IsPunctChar(currentCore[stripLeft]))
                ++stripLeft;
            AZ::u32 stripRight = static_cast<AZ::u32>(currentCore.size());
            while (stripRight > stripLeft && IsPunctChar(currentCore[stripRight - 1]))
                --stripRight;

            if (stripRight <= stripLeft)
            {
                inRun = false;
                currentCore.clear();
                currentUpperPositions.clear();
                return;
            }

            // Build stripped, lowercased core
            AZStd::string core;
            core.reserve(stripRight - stripLeft);
            AZStd::vector<AZ::u32> adjustedUpper;

            for (AZ::u32 j = stripLeft; j < stripRight; ++j)
            {
                unsigned char uc = static_cast<unsigned char>(currentCore[j]);
                if (std::isupper(uc))
                {
                    adjustedUpper.push_back(j - stripLeft);
                }
                core += static_cast<char>(std::tolower(uc));
            }

            // Skip non-ASCII
            bool allAscii = true;
            for (size_t j = 0; j < core.size(); ++j)
            {
                unsigned char uc = static_cast<unsigned char>(core[j]);
                if (uc >= 128) { allAscii = false; break; }
            }

            if (allAscii && !core.empty())
            {
                CharRun run;
                run.text = core;
                run.startPos = currentStart + stripLeft;
                run.length = static_cast<AZ::u32>(core.size());

                // Populate normalization metadata
                if (adjustedUpper.size() == 1 && adjustedUpper[0] == 0)
                {
                    run.firstCap = true;
                }
                else if (!adjustedUpper.empty())
                {
                    run.firstCap = false;
                    run.capMask = AZStd::move(adjustedUpper);
                }

                runs.push_back(run);
            }

            inRun = false;
            currentCore.clear();
            currentUpperPositions.clear();
        };

        for (const auto& collapse : trialResult.collapses)
        {
            // Unsettled byte → run boundary
            if (!collapse.settled)
            {
                FlushRun();
                continue;
            }

            char c = collapse.resolvedChar;

            // Settled whitespace → run boundary
            if (IsWhitespaceChar(c))
            {
                FlushRun();
                continue;
            }

            // Settled character → accumulate into run
            if (!inRun)
            {
                inRun = true;
                currentStart = collapse.streamPos;
                currentCore.clear();
                currentUpperPositions.clear();
            }

            currentCore += c;
        }

        // Flush final run
        FlushRun();

        return runs;
    }

    // ---- Main trial function ----

    WordTrialResult RunWordSuperpositionTrial(
        physx::PxPhysics* physics,
        physx::PxScene* scene,
        physx::PxCudaContextManager* cuda,
        const AZStd::string& inputText,
        const HCPVocabulary& vocab,
        AZ::u32 maxChars)
    {
        WordTrialResult result;

        if (!physics || !scene || !cuda || inputText.empty())
            return result;

        auto startTime = std::chrono::high_resolution_clock::now();

        // ---- Step 1: Extract runs from input ----
        AZStd::vector<CharRun> runs = ExtractRuns(inputText, maxChars);
        result.totalRuns = static_cast<AZ::u32>(runs.size());

        fprintf(stderr, "[WordTrial] Extracted %u runs from first %u bytes\n",
            result.totalRuns, maxChars);
        fflush(stderr);

        if (runs.empty()) return result;

        // ---- Step 2: Build vocab index and find candidates per run ----
        VocabIndex vocabIndex = BuildVocabIndex(vocab);

        // Per-run candidate lists and X base offsets
        struct RunLayout
        {
            AZ::u32 runIdx;
            float xBase;
            AZStd::vector<const VocabCandidate*> candidates;
        };

        AZStd::vector<RunLayout> layouts;
        layouts.reserve(runs.size());

        float nextXBase = 0.0f;
        AZ::u32 totalStaticParticles = 0;
        AZ::u32 totalDynamicParticles = 0;

        for (AZ::u32 r = 0; r < runs.size(); ++r)
        {
            const CharRun& run = runs[r];

            RunLayout layout;
            layout.runIdx = r;
            layout.xBase = nextXBase;

            // Look up candidates: same length + same first character
            const AZStd::vector<VocabCandidate>* bucket =
                vocabIndex.Lookup(run.length, run.text[0]);

            if (bucket)
            {
                for (const auto& cand : *bucket)
                    layout.candidates.push_back(&cand);
            }

            totalStaticParticles += run.length;
            totalDynamicParticles += static_cast<AZ::u32>(layout.candidates.size()) * run.length;

            result.totalCandidates += static_cast<AZ::u32>(layout.candidates.size());

            layouts.push_back(AZStd::move(layout));

            // Advance X: run length + gap
            nextXBase += static_cast<float>(run.length) + P2_RUN_X_GAP;
        }

        AZ::u32 totalParticles = totalStaticParticles + totalDynamicParticles;
        result.totalParticles = totalParticles;

        fprintf(stderr, "[WordTrial] %u runs, %u candidates, %u static + %u dynamic = %u particles\n",
            result.totalRuns, result.totalCandidates,
            totalStaticParticles, totalDynamicParticles, totalParticles);
        fflush(stderr);

        if (totalParticles == 0)
        {
            fprintf(stderr, "[WordTrial] No particles to simulate\n");
            fflush(stderr);
            return result;
        }

        // ---- Step 3: Create PBD system ----
        physx::PxPBDParticleSystem* particleSystem =
            physics->createPBDParticleSystem(*cuda, 96);
        if (!particleSystem)
        {
            fprintf(stderr, "[WordTrial] ERROR: Failed to create PBD particle system\n");
            fflush(stderr);
            return result;
        }

        particleSystem->setRestOffset(P2_REST_OFFSET);
        particleSystem->setContactOffset(P2_CONTACT_OFFSET);
        particleSystem->setParticleContactOffset(P2_CONTACT_OFFSET);
        particleSystem->setSolidRestOffset(P2_REST_OFFSET);
        particleSystem->setSolverIterationCounts(4, 1);
        scene->addActor(*particleSystem);

        physx::PxPBDMaterial* pbdMaterial = physics->createPBDMaterial(
            0.2f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (!pbdMaterial)
        {
            fprintf(stderr, "[WordTrial] ERROR: Failed to create PBD material\n");
            fflush(stderr);
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return result;
        }

        const physx::PxU32 phase = particleSystem->createPhase(
            pbdMaterial,
            physx::PxParticlePhaseFlags(physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide));

        physx::PxParticleBuffer* particleBuffer = physics->createParticleBuffer(
            totalParticles, 1, cuda);
        if (!particleBuffer)
        {
            fprintf(stderr, "[WordTrial] ERROR: Failed to create particle buffer\n");
            fflush(stderr);
            pbdMaterial->release();
            scene->removeActor(*particleSystem);
            particleSystem->release();
            return result;
        }

        // ---- Step 4: Initialize particles ----
        // Layout in buffer:
        //   [0 .. totalStatic-1]              = static run character particles (invMass=0)
        //   [totalStatic .. totalParticles-1]  = dynamic vocab word particles (invMass=1)
        //
        // We also need to track which dynamic particles belong to which word
        // for the host-side AND check.

        struct WordParticleRange
        {
            AZ::u32 runIdx;
            AZ::u32 candidateIdx;     // index into layout.candidates
            AZ::u32 bufferStart;      // first dynamic particle index
            AZ::u32 charCount;        // number of chars (= particles)
        };

        AZStd::vector<WordParticleRange> wordRanges;
        wordRanges.reserve(result.totalCandidates);

        {
            physx::PxScopedCudaLock lock(*cuda);

            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = particleBuffer->getVelocities();
            physx::PxU32* devPhase = particleBuffer->getPhases();

            physx::PxVec4* hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            physx::PxVec4* hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            physx::PxU32* hostPhase = cuda->allocPinnedHostBuffer<physx::PxU32>(totalParticles);

            // --- Static run character particles ---
            AZ::u32 staticIdx = 0;
            for (AZ::u32 r = 0; r < layouts.size(); ++r)
            {
                const CharRun& run = runs[layouts[r].runIdx];
                float xBase = layouts[r].xBase;

                for (AZ::u32 c = 0; c < run.length; ++c)
                {
                    float x = xBase + static_cast<float>(c);
                    float z = static_cast<float>(static_cast<unsigned char>(run.text[c])) * P2_Z_SCALE;

                    hostPos[staticIdx] = physx::PxVec4(x, 0.0f, z, 0.0f);   // invMass=0 (static)
                    hostVel[staticIdx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                    hostPhase[staticIdx] = phase;
                    ++staticIdx;
                }
            }

            // --- Dynamic vocab word particles ---
            AZ::u32 dynamicIdx = totalStaticParticles;
            for (AZ::u32 r = 0; r < layouts.size(); ++r)
            {
                const RunLayout& layout = layouts[r];
                const CharRun& run = runs[layout.runIdx];
                float xBase = layout.xBase;

                for (AZ::u32 w = 0; w < layout.candidates.size(); ++w)
                {
                    const VocabCandidate* cand = layout.candidates[w];

                    WordParticleRange range;
                    range.runIdx = layout.runIdx;
                    range.candidateIdx = w;
                    range.bufferStart = dynamicIdx;
                    range.charCount = run.length;

                    // All vocab words at same Y-offset — no Y-lanes.
                    // Particles at different Z don't interact (Z discrimination).
                    // Same-Z particles (shared character at same position) stack naturally.
                    for (AZ::u32 c = 0; c < run.length; ++c)
                    {
                        float x = xBase + static_cast<float>(c);
                        // Z = expected character identity from the vocab word
                        char expectedChar = (c < cand->word.size())
                            ? static_cast<char>(std::tolower(static_cast<unsigned char>(cand->word[c])))
                            : '\0';
                        float z = static_cast<float>(static_cast<unsigned char>(expectedChar)) * P2_Z_SCALE;

                        hostPos[dynamicIdx] = physx::PxVec4(x, P2_Y_OFFSET, z, 1.0f);  // invMass=1 (dynamic)
                        hostVel[dynamicIdx] = physx::PxVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        hostPhase[dynamicIdx] = phase;
                        ++dynamicIdx;
                    }

                    wordRanges.push_back(range);
                }
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

        // ---- Step 5: Simulate ----
        fprintf(stderr, "[WordTrial] Simulating %d steps (gravity-driven settlement)...\n", P2_MAX_STEPS);
        fflush(stderr);

        for (int step = 0; step < P2_MAX_STEPS; ++step)
        {
            scene->simulate(P2_DT);
            scene->fetchResults(true);
            scene->fetchResultsParticleSystem();

            if ((step + 1) % 20 == 0)
            {
                fprintf(stderr, "[WordTrial] Step %d/%d\n", step + 1, P2_MAX_STEPS);
                fflush(stderr);
            }
        }

        result.simulationSteps = P2_MAX_STEPS;

        // ---- Step 6: Read back positions AND velocities ----
        // Need both to distinguish settled (at rest near Y=0) from mid-fall (transient Y≈0).
        physx::PxVec4* hostPos = nullptr;
        physx::PxVec4* hostVel = nullptr;
        {
            physx::PxScopedCudaLock lock(*cuda);
            physx::PxVec4* devPos = particleBuffer->getPositionInvMasses();
            physx::PxVec4* devVel = particleBuffer->getVelocities();
            hostPos = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            hostVel = cuda->allocPinnedHostBuffer<physx::PxVec4>(totalParticles);
            cuda->copyDToH(hostPos, devPos, totalParticles);
            cuda->copyDToH(hostVel, devVel, totalParticles);
        }

        // ---- Step 7: Classify settlements per word ----
        // A character is "settled" when:
        //   1. |Y| < P2_SETTLE_Y (near the static reference at Y=0)
        //   2. |Vy| < P2_VELOCITY_THRESHOLD (at rest, not transiting through Y=0)
        // This distinguishes particles that contacted a Z-matching static and came
        // to rest from particles that are simply falling through Y≈0 at high speed
        // (their Z didn't match any static at this X, so nothing stopped them).

        result.runResults.resize(runs.size());
        for (AZ::u32 r = 0; r < runs.size(); ++r)
        {
            result.runResults[r].run = runs[r];
            result.runResults[r].candidateCount = 0;
            result.runResults[r].resolved = false;
        }

        for (const auto& wr : wordRanges)
        {
            const RunLayout& layout = layouts[wr.runIdx];
            const VocabCandidate* cand = layout.candidates[wr.candidateIdx];

            AZ::u32 settledCount = 0;
            for (AZ::u32 c = 0; c < wr.charCount; ++c)
            {
                float y = hostPos[wr.bufferStart + c].y;
                float vy = hostVel[wr.bufferStart + c].y;
                // Settled = near Y=0 AND at rest (low velocity)
                // Mid-fall = near Y=0 but high velocity (transiting through)
                // Fell through = far below Y=0
                // Still in transit = far above Y=0
                if (fabsf(y) < P2_SETTLE_Y && fabsf(vy) < P2_VELOCITY_THRESHOLD)
                    ++settledCount;
            }

            bool fullMatch = (settledCount == wr.charCount);

            result.runResults[wr.runIdx].candidateCount++;

            if (fullMatch)
            {
                // Record this as the matched word for this run
                WordCandidateResult wcr;
                wcr.word = cand->word;
                wcr.tokenId = cand->tokenId;
                wcr.settledChars = settledCount;
                wcr.totalChars = wr.charCount;
                wcr.fullMatch = true;

                result.runResults[wr.runIdx].matchedWord = wcr;
                result.runResults[wr.runIdx].resolved = true;
            }
        }

        // Count resolved/unresolved
        for (const auto& rr : result.runResults)
        {
            if (rr.resolved)
                result.resolvedRuns++;
            else
                result.unresolvedRuns++;
        }

        {
            physx::PxScopedCudaLock lock(*cuda);
            cuda->freePinnedHostBuffer(hostPos);
            cuda->freePinnedHostBuffer(hostVel);
        }

        // ---- Cleanup ----
        particleSystem->removeParticleBuffer(particleBuffer);
        particleBuffer->release();
        pbdMaterial->release();
        scene->removeActor(*particleSystem);
        particleSystem->release();

        auto endTime = std::chrono::high_resolution_clock::now();
        result.simulationTimeMs = static_cast<float>(
            std::chrono::duration<double, std::milli>(endTime - startTime).count());

        // ---- Report ----
        fprintf(stderr, "\n[WordTrial] ====== CHAR→WORD RESULTS (superposition zones) ======\n");
        fprintf(stderr, "[WordTrial] Runs: %u | Resolved: %u | Unresolved: %u\n",
            result.totalRuns, result.resolvedRuns, result.unresolvedRuns);
        fprintf(stderr, "[WordTrial] Candidates tested: %u | Total particles: %u\n",
            result.totalCandidates, result.totalParticles);
        fprintf(stderr, "[WordTrial] Steps: %d | Time: %.1f ms\n",
            result.simulationSteps, result.simulationTimeMs);

        fprintf(stderr, "\n[WordTrial] Per-run results:\n");
        for (AZ::u32 r = 0; r < result.runResults.size(); ++r)
        {
            const auto& rr = result.runResults[r];
            if (rr.resolved)
            {
                fprintf(stderr, "  [%2u] \"%s\" (%u chars, %u cands) -> MATCH: \"%s\" [%s]\n",
                    r, rr.run.text.c_str(), rr.run.length, rr.candidateCount,
                    rr.matchedWord.word.c_str(), rr.matchedWord.tokenId.c_str());
            }
            else
            {
                fprintf(stderr, "  [%2u] \"%s\" (%u chars, %u cands) -> UNRESOLVED\n",
                    r, rr.run.text.c_str(), rr.run.length, rr.candidateCount);
            }
        }

        // ---- Validation: compare against computational tokenizer ----
        AZStd::string truncated = inputText;
        if (truncated.size() > maxChars)
            truncated = truncated.substr(0, maxChars);
        TokenStream compStream = Tokenize(truncated, vocab);

        fprintf(stderr, "\n[WordTrial] Computational tokenizer: %zu tokens from same input\n",
            compStream.tokenIds.size());

        // Count how many resolved words match what the computational tokenizer found
        // (Simple comparison: check if resolved words appear in the comp token stream)
        AZ::u32 compMatches = 0;
        for (const auto& rr : result.runResults)
        {
            if (!rr.resolved) continue;
            for (const auto& tid : compStream.tokenIds)
            {
                if (tid == rr.matchedWord.tokenId)
                {
                    ++compMatches;
                    break;
                }
            }
        }
        fprintf(stderr, "[WordTrial] Physics matches also in computational output: %u / %u resolved\n",
            compMatches, result.resolvedRuns);

        fprintf(stderr, "[WordTrial] ================================\n");
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
