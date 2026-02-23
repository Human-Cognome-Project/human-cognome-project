
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/ILogger.h>

#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPStorage.h"
#include "HCPBondCompiler.h"
#include "HCPDetectionScene.h"

#include <AzCore/std/sort.h>
#include <fstream>
#include <chrono>
#include <cstdio>

#include <HCPEngine/HCPEngineTypeIds.h>

// PhysX access — we link against Gem::PhysX5.Static which exposes internal headers
#include <System/PhysXSystem.h>

namespace HCPEngine
{
    AZ_COMPONENT_IMPL(HCPEngineSystemComponent, "HCPEngineSystemComponent",
        HCPEngineSystemComponentTypeId);

    void HCPEngineSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HCPEngineSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void HCPEngineSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("HCPEngineService"));
    }

    void HCPEngineSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("HCPEngineService"));
    }

    void HCPEngineSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // NOTE: PhysXService dependency temporarily removed for headless testing.
        // PhysX is initialized manually in Activate() via GetPhysXSystem().
        // required.push_back(AZ_CRC_CE("PhysXService"));
    }

    void HCPEngineSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    HCPEngineSystemComponent::HCPEngineSystemComponent()
    {
        if (HCPEngineInterface::Get() == nullptr)
        {
            HCPEngineInterface::Register(this);
        }
    }

    HCPEngineSystemComponent::~HCPEngineSystemComponent()
    {
        if (HCPEngineInterface::Get() == this)
        {
            HCPEngineInterface::Unregister(this);
        }
    }

    void HCPEngineSystemComponent::Init()
    {
        fprintf(stderr, "[HCPEngine] Init() called\n");
        fflush(stderr);
    }

    void HCPEngineSystemComponent::Activate()
    {
        fprintf(stderr, "[HCPEngine] Activate() called\n");
        fflush(stderr);

        HCPEngineRequestBus::Handler::BusConnect();

        AZLOG_INFO("HCPEngine: Activating — loading vocabulary and initializing PBD particle system");

        // Load vocabulary from LMDB (core tokens seeded, words populated by pipeline)
        fprintf(stderr, "[HCPEngine] Loading vocabulary from LMDB...\n");
        fflush(stderr);
        if (!m_vocabulary.Load())
        {
            fprintf(stderr, "[HCPEngine] ERROR: Failed to load vocabulary\n");
            fflush(stderr);
            return;
        }
        fprintf(stderr, "[HCPEngine] Vocabulary loaded: %zu words\n", m_vocabulary.WordCount());
        fflush(stderr);

        // Initialize cache miss resolver — fills LMDB from Postgres on demand
        {
            m_resolver.SetLmdbEnv(m_vocabulary.GetLmdbEnv());

            // Register DBI handles for all sub-databases
            const char* dbNames[] = { "w2t", "c2t", "l2t", "t2w", "t2c", "forward" };
            for (const char* name : dbNames)
            {
                m_resolver.SetLmdbDbi(name, m_vocabulary.GetDbi(name));
            }

            // Register handlers
            m_resolver.RegisterHandler(AZStd::make_unique<WordHandler>(&m_resolver));
            m_resolver.RegisterHandler(AZStd::make_unique<CharHandler>());
            m_resolver.RegisterHandler(AZStd::make_unique<LabelHandler>(&m_resolver));
            m_resolver.RegisterHandler(AZStd::make_unique<VarHandler>(&m_resolver));

            // Wire resolver into vocabulary — lookups now auto-fill on miss
            m_vocabulary.SetResolver(&m_resolver);

            fprintf(stderr, "[HCPEngine] Cache miss resolver initialized (4 handlers)\n");
            fflush(stderr);

            // Bulk-load affix morpheme list from Postgres (suffixes/prefixes for tokenizer)
            PGconn* englishConn = m_resolver.GetConnection("hcp_english");
            if (englishConn)
            {
                m_vocabulary.LoadAffixes(englishConn);
            }
        }

        // Load sub-word PBM bond tables — try hcp_temp first, compile from source if empty
        {
            auto bondStart = std::chrono::high_resolution_clock::now();

            m_charWordBonds = LoadBondTable("char_word");
            auto t1 = std::chrono::high_resolution_clock::now();

            if (m_charWordBonds.PairCount() == 0)
            {
                fprintf(stderr, "[HCPEngine] No cached char->word bonds, compiling from hcp_english...\n");
                fflush(stderr);
                m_charWordBonds = CompileCharWordBondsFromPostgres(
                    "host=localhost dbname=hcp_english user=hcp password=hcp_dev");
                t1 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine] Char->word bonds compiled in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(t1 - bondStart).count());
                fflush(stderr);
                SaveBondTable(m_charWordBonds, "char_word");
            }
            else
            {
                fprintf(stderr, "[HCPEngine] Char->word bonds loaded from hcp_temp in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(t1 - bondStart).count());
                fflush(stderr);
            }

            auto t1b = std::chrono::high_resolution_clock::now();
            m_byteCharBonds = LoadBondTable("byte_char");
            auto t2 = std::chrono::high_resolution_clock::now();

            if (m_byteCharBonds.PairCount() == 0)
            {
                fprintf(stderr, "[HCPEngine] No cached byte->char bonds, compiling from hcp_core...\n");
                fflush(stderr);
                m_byteCharBonds = CompileByteCharBondsFromPostgres(
                    "host=localhost dbname=hcp_core user=hcp password=hcp_dev");
                t2 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine] Byte->char bonds compiled in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(t2 - t1b).count());
                fflush(stderr);
                SaveBondTable(m_byteCharBonds, "byte_char");
            }
            else
            {
                fprintf(stderr, "[HCPEngine] Byte->char bonds loaded from hcp_temp in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(t2 - t1b).count());
                fflush(stderr);
            }

            // Log top bond pairs for verification
            fprintf(stderr, "[HCPEngine] Top char->word bonds (by count):\n");
            fflush(stderr);
            AZStd::vector<AZStd::pair<AZStd::string, AZ::u32>> sortedBonds;
            for (const auto& [key, count] : m_charWordBonds.GetAllBonds())
            {
                sortedBonds.push_back({key, count});
            }
            AZStd::sort(sortedBonds.begin(), sortedBonds.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            for (size_t i = 0; i < 20 && i < sortedBonds.size(); ++i)
            {
                const auto& [key, count] = sortedBonds[i];
                size_t sep = key.find('|');
                if (sep != AZStd::string::npos)
                {
                    fprintf(stderr, "  %.*s -> %.*s : %u\n",
                        (int)sep, key.c_str(),
                        (int)(key.size() - sep - 1), key.c_str() + sep + 1,
                        count);
                }
            }
            fflush(stderr);
        }

        // Get PhysX physics and foundation from O3DE's PhysX system
        PhysX::PhysXSystem* physxSystem = PhysX::GetPhysXSystem();
        if (!physxSystem)
        {
            AZLOG_ERROR("HCPEngine: PhysX system not available");
            return;
        }

        physx::PxPhysics* pxPhysics = physxSystem->GetPxPhysics();
        if (!pxPhysics)
        {
            AZLOG_ERROR("HCPEngine: PxPhysics not available");
            return;
        }

        // Get foundation from physics tolerances (there's only one per process)
        physx::PxFoundation& foundation = pxPhysics->getFoundation();

        // Initialize PBD particle pipeline with CUDA + GPU scene
        fprintf(stderr, "[HCPEngine] Initializing PBD particle pipeline (CUDA + GPU)...\n");
        fflush(stderr);
        if (!m_particlePipeline.Initialize(pxPhysics, &foundation))
        {
            fprintf(stderr, "[HCPEngine] ERROR: Failed to initialize PBD particle pipeline\n");
            fflush(stderr);
            return;
        }
        fprintf(stderr, "[HCPEngine] PBD particle pipeline initialized\n");
        fflush(stderr);

        AZLOG_INFO("HCPEngine: Ready — vocab: %zu words, %zu labels, %zu chars; PBD particle system active",
            m_vocabulary.WordCount(), m_vocabulary.LabelCount(), m_vocabulary.CharCount());

        // ---- Self-test: process a Gutenberg text through the full pipeline ----
        {
            const char* testFile = "/opt/project/repo/data/gutenberg/texts/01952_The Yellow Wallpaper.txt";
            std::ifstream ifs(testFile);
            if (ifs.is_open())
            {
                std::string stdText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                ifs.close();
                AZStd::string text(stdText.c_str(), stdText.size());

                fprintf(stderr, "[HCPEngine TEST] Loaded '%s' (%zu bytes)\n", testFile, text.size());
                fflush(stderr);

                auto t0 = std::chrono::high_resolution_clock::now();

                // Step 1: Tokenize (with gap-encoded positions — gaps = spaces)
                TokenStream stream = Tokenize(text, m_vocabulary);
                auto t1 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine TEST] Tokenized -> %zu tokens, %u slots (%.1f ms)\n",
                    stream.tokenIds.size(), stream.totalSlots,
                    std::chrono::duration<double, std::milli>(t1 - t0).count());
                fflush(stderr);

                // Step 2: Position-based disassembly (per-document storage)
                PositionMap posMap = DisassemblePositions(stream);
                auto t2 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine TEST] Position disassembly -> %zu unique tokens, %u slots (%.3f ms)\n",
                    posMap.uniqueTokens, posMap.totalTokens,
                    std::chrono::duration<double, std::milli>(t2 - t1).count());
                fflush(stderr);

                // Step 3: Position-based reassembly (exact reconstruction)
                TokenStream reassembled = ReassemblePositions(posMap);
                auto t3 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine TEST] Position reassembly -> %zu tokens, %u slots (%.3f ms)\n",
                    reassembled.tokenIds.size(), reassembled.totalSlots,
                    std::chrono::duration<double, std::milli>(t3 - t2).count());
                fflush(stderr);

                // Step 4: Verify exact round-trip (tokens AND positions)
                bool match = (stream.tokenIds.size() == reassembled.tokenIds.size());
                if (match)
                {
                    for (size_t i = 0; i < stream.tokenIds.size(); ++i)
                    {
                        if (stream.tokenIds[i] != reassembled.tokenIds[i] ||
                            stream.positions[i] != reassembled.positions[i])
                        {
                            match = false;
                            AZLOG_INFO("HCPEngine TEST: Mismatch at index %zu: '%s'@%u vs '%s'@%u",
                                i,
                                stream.tokenIds[i].c_str(), stream.positions[i],
                                reassembled.tokenIds[i].c_str(), reassembled.positions[i]);
                            break;
                        }
                    }
                }
                else
                {
                    AZLOG_INFO("HCPEngine TEST: Length mismatch: original %zu vs reassembled %zu",
                        stream.tokenIds.size(), reassembled.tokenIds.size());
                }

                fprintf(stderr, "[HCPEngine TEST] Round-trip: %s\n",
                    match ? "EXACT MATCH" : "MISMATCH");
                fflush(stderr);

                // Step 5: Derive PBM from token stream (for aggregate pipeline)
                PBMData pbmData = DerivePBM(stream);
                auto t4 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine TEST] PBM derived -> %zu unique bonds, %zu total pairs (%.3f ms)\n",
                    pbmData.bonds.size(), pbmData.totalPairs,
                    std::chrono::duration<double, std::milli>(t4 - t3).count());
                fflush(stderr);

                // Store PBM into hcp_fic_pbm
                if (!m_writeKernel.IsConnected())
                {
                    m_writeKernel.Connect();
                }
                if (m_writeKernel.IsConnected())
                {
                    AZStd::string pbmDocId = m_writeKernel.StorePBM(
                        "The Yellow Wallpaper", "AS", pbmData);
                    if (!pbmDocId.empty())
                    {
                        fprintf(stderr, "[HCPEngine TEST] PBM stored -> %s\n", pbmDocId.c_str());
                    }
                    else
                    {
                        fprintf(stderr, "[HCPEngine TEST] PBM storage failed\n");
                    }
                    fflush(stderr);
                }
                else
                {
                    fprintf(stderr, "[HCPEngine TEST] Write kernel not connected, skipping PBM store\n");
                    fflush(stderr);
                }

                // Step 6: Base-50 position encode/decode round-trip
                {
                    size_t totalPositions = 0;
                    size_t encodedBytes = 0;
                    bool b50match = true;
                    for (const auto& entry : posMap.entries)
                    {
                        AZStd::string encoded = EncodePositions(entry.positions);
                        AZStd::vector<AZ::u32> decoded = DecodePositions(encoded);
                        encodedBytes += encoded.size();
                        totalPositions += entry.positions.size();
                        if (decoded.size() != entry.positions.size())
                        {
                            b50match = false;
                            break;
                        }
                        for (size_t j = 0; j < decoded.size(); ++j)
                        {
                            if (decoded[j] != entry.positions[j])
                            {
                                b50match = false;
                                fprintf(stderr, "[HCPEngine TEST] B50 mismatch: token '%s' pos[%zu] = %u, decoded = %u\n",
                                    entry.tokenId.c_str(), j, entry.positions[j], decoded[j]);
                                fflush(stderr);
                                break;
                            }
                        }
                        if (!b50match) break;
                    }
                    fprintf(stderr, "[HCPEngine TEST] Base-50 encode/decode: %s (%zu positions -> %zu bytes)\n",
                        b50match ? "EXACT MATCH" : "MISMATCH", totalPositions, encodedBytes);
                    fflush(stderr);
                }

                // Storage stats
                AZ::u32 spaceSlotsOmitted = stream.totalSlots -
                    static_cast<AZ::u32>(stream.tokenIds.size());

                // DB-format size estimate: base-50 positions + token segment overhead
                size_t dbEstBytes = 0;
                for (const auto& entry : posMap.entries)
                {
                    dbEstBytes += 6;  // ~6 bytes for token segments (p3.p4.p5 or similar)
                    dbEstBytes += entry.positions.size() * 4;  // 4 chars per position
                }

                fprintf(stderr, "[HCPEngine TEST] Storage: DB est %zu bytes, %u space slots implicit, PBM %zu bonds\n",
                    dbEstBytes, spaceSlotsOmitted, pbmData.bonds.size());
                fflush(stderr);

                auto tEnd = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine TEST] %s — total pipeline %.1f ms\n",
                    match ? "EXACT MATCH" : "MISMATCH",
                    std::chrono::duration<double, std::milli>(tEnd - t0).count());
                fflush(stderr);
            }
            else
            {
                AZLOG_INFO("HCPEngine TEST: Could not open test file '%s'", testFile);
            }
        }

        // ---- Physics detection test ----
        {
            const char* testStr = "the cat sat on the mat";
            size_t testLen = strlen(testStr);
            fprintf(stderr, "\n[HCPEngine TEST] Physics detection: \"%s\" (%zu bytes)\n", testStr, testLen);
            fflush(stderr);

            DetectionResult detResult = RunDetection(
                m_particlePipeline.GetPhysics(),
                m_particlePipeline.GetScene(),
                m_particlePipeline.GetCuda(),
                reinterpret_cast<const uint8_t*>(testStr), testLen,
                m_byteCharBonds, m_charWordBonds);

            fprintf(stderr, "[HCPEngine TEST] Detection: %zu clusters, %d steps, %.1f ms\n",
                detResult.clusters.size(), detResult.simulationSteps, detResult.simulationTimeMs);
            for (size_t i = 0; i < detResult.clusters.size(); ++i)
            {
                const auto& c = detResult.clusters[i];
                fprintf(stderr, "  Cluster %zu: [%u-%u] \"%s\"\n",
                    i, c.startByte, c.endByte, c.text.c_str());
            }
            fflush(stderr);
        }

        // Start socket server — API for ingestion and retrieval
        m_socketServer.Start(this, HCPSocketServer::DEFAULT_PORT);
    }

    void HCPEngineSystemComponent::Deactivate()
    {
        AZLOG_INFO("HCPEngine: Deactivating — shutting down socket server and PBD pipeline");
        m_socketServer.Stop();
        m_resolver.Shutdown();
        m_writeKernel.Disconnect();
        m_particlePipeline.Shutdown();
        HCPEngineRequestBus::Handler::BusDisconnect();
    }

    bool HCPEngineSystemComponent::IsReady() const
    {
        return m_vocabulary.IsLoaded() && m_particlePipeline.IsInitialized();
    }

    AZStd::string HCPEngineSystemComponent::ProcessText(
        const AZStd::string& text,
        const AZStd::string& docName,
        const AZStd::string& centuryCode)
    {
        if (!IsReady())
        {
            AZLOG_ERROR("HCPEngine: Not ready — call Activate first");
            return {};
        }

        AZLOG_INFO("HCPEngine: Processing '%s' (%zu chars)", docName.c_str(), text.size());

        // Step 1: Tokenize with gap-encoded positions
        TokenStream stream = Tokenize(text, m_vocabulary);
        if (stream.tokenIds.empty())
        {
            AZLOG_ERROR("HCPEngine: Tokenization produced no tokens");
            return {};
        }

        // Step 2: Position-based disassembly
        PositionMap posMap = DisassemblePositions(stream);

        // Step 3: Store to Postgres via write kernel
        if (!m_writeKernel.IsConnected())
        {
            m_writeKernel.Connect();
        }
        if (!m_writeKernel.IsConnected())
        {
            AZLOG_ERROR("HCPEngine: Write kernel not connected — cannot store");
            return {};
        }

        AZStd::string docId = m_writeKernel.StorePositionMap(docName, centuryCode, posMap, stream);
        if (docId.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to store position map");
            return {};
        }

        AZLOG_INFO("HCPEngine: Stored %s — %zu tokens, %zu unique, %u slots",
            docId.c_str(), stream.tokenIds.size(), posMap.uniqueTokens, stream.totalSlots);

        return docId;
    }

    AZStd::string HCPEngineSystemComponent::ReassembleFromPBM(const AZStd::string& docId)
    {
        if (!IsReady())
        {
            AZLOG_ERROR("HCPEngine: Not ready");
            return {};
        }

        AZLOG_INFO("HCPEngine: Reassembling from %s", docId.c_str());

        // Load position map from database
        if (!m_writeKernel.IsConnected())
        {
            m_writeKernel.Connect();
        }
        if (!m_writeKernel.IsConnected())
        {
            AZLOG_ERROR("HCPEngine: Write kernel not connected — cannot load");
            return {};
        }

        TokenStream loadedStream;
        PositionMap posMap = m_writeKernel.LoadPositionMap(docId, loadedStream);
        if (posMap.entries.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to load position map for %s", docId.c_str());
            return {};
        }

        // Reassemble: position map → token stream (with gap-encoded positions)
        TokenStream stream = ReassemblePositions(posMap);

        // Convert token stream to text — gaps in positions = spaces
        AZStd::string text;
        text.reserve(stream.totalSlots);

        AZ::u32 cursor = 0;
        for (size_t i = 0; i < stream.tokenIds.size(); ++i)
        {
            AZ::u32 pos = stream.positions[i];
            const AZStd::string& tid = stream.tokenIds[i];

            // Fill gaps with spaces
            while (cursor < pos)
            {
                text += ' ';
                ++cursor;
            }

            // Word token (AB.AB.*)
            if (tid.starts_with("AB.AB."))
            {
                AZStd::string word = m_vocabulary.TokenToWord(tid);
                if (!word.empty())
                {
                    text += word;
                    ++cursor;
                    continue;
                }
            }

            // Character/punctuation token (AA.*)
            char c = m_vocabulary.TokenToChar(tid);
            if (c != '\0')
            {
                text += c;
                ++cursor;
                continue;
            }

            // Label token — check for newline
            AZStd::string nlLabel = m_vocabulary.LookupLabel("newline");
            if (!nlLabel.empty() && tid == nlLabel)
            {
                text += '\n';
                ++cursor;
                continue;
            }

            // Unknown token — skip position
            ++cursor;
        }

        AZLOG_INFO("HCPEngine: Reassembled %zu tokens into %zu chars", stream.tokenIds.size(), text.size());
        return text;
    }
}
