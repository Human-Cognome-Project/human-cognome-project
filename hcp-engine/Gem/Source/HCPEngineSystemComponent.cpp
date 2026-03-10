
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/ILogger.h>

#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPBondCompiler.h"
#include "HCPSuperpositionTrial.h"
#include "HCPWordSuperpositionTrial.h"
#include "HCPResolutionChamber.h"
#include "HCPVocabBed.h"

#include <AzCore/std/sort.h>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <sys/resource.h>
#include <sys/stat.h>

#include <HCPEngine/HCPEngineTypeIds.h>

// PhysX access — we link against Gem::PhysX5.Static which exposes internal headers
#include <System/PhysXSystem.h>

// CVars — namespace scope (AZ_CVAR creates inline globals)
AZ_CVAR(bool, hcp_listen_all, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Listen on all interfaces (0.0.0.0) instead of localhost only");

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
        // File-based diagnostic — guaranteed visible
        FILE* diagFile = fopen("/tmp/hcp_editor_diag.txt", "a");
        if (diagFile)
        {
            fprintf(diagFile, "HCPEngineSystemComponent::Activate() called\n");
            fclose(diagFile);
        }

        AZ_TracePrintf("HCPEngine", "Activate() called\n");
        fprintf(stderr, "[HCPEngine] Activate() called\n");
        fflush(stderr);

        HCPEngineRequestBus::Handler::BusConnect();

        AZ_TracePrintf("HCPEngine", "Activating — loading vocabulary and initializing PBD particle system\n");
        AZLOG_INFO("HCPEngine: Activating — loading vocabulary and initializing PBD particle system");

        // Load vocabulary from LMDB (core tokens seeded, words populated by pipeline)
        fprintf(stderr, "[HCPEngine] Loading vocabulary from LMDB...\n");
        fflush(stderr);
        if (!m_vocabulary.Load())
        {
            AZ_TracePrintf("HCPEngine", "ERROR: Failed to load vocabulary from LMDB\n");
            { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"Vocab load FAILED\n");fclose(df);} }
            return;
        }
        { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"Vocab loaded: %zu words\n", m_vocabulary.WordCount());fclose(df);} }
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
            { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"FAILED: PhysX system not available\n");fclose(df);} }
            AZLOG_ERROR("HCPEngine: PhysX system not available");
            return;
        }

        physx::PxPhysics* pxPhysics = physxSystem->GetPxPhysics();
        if (!pxPhysics)
        {
            { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"FAILED: PxPhysics not available\n");fclose(df);} }
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
            { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"FAILED: PBD particle pipeline init\n");fclose(df);} }
            fprintf(stderr, "[HCPEngine] ERROR: Failed to initialize PBD particle pipeline\n");
            fflush(stderr);
            return;
        }
        fprintf(stderr, "[HCPEngine] PBD particle pipeline initialized\n");
        fflush(stderr);

        AZLOG_INFO("HCPEngine: Ready — vocab: %zu words, %zu labels, %zu chars; PBD particle system active",
            m_vocabulary.WordCount(), m_vocabulary.LabelCount(), m_vocabulary.CharCount());

        // Initialize persistent vocab beds from pre-compiled LMDB (Phase 2)
        // Vocab data is pre-ordered at compile time — no Postgres, no TierAssembly
        {
            auto bedStart = std::chrono::high_resolution_clock::now();

            if (!m_particlePipeline.GetCharWordScene())
            {
                m_particlePipeline.CreateCharWordScene();
            }

            if (m_particlePipeline.GetCuda() && m_vocabulary.GetLmdbEnv())
            {
                m_bedManager.Initialize(
                    pxPhysics,
                    m_particlePipeline.GetCuda(),
                    m_vocabulary.GetLmdbEnv(),
                    &m_vocabulary);

                auto bedEnd = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine] Persistent vocab beds initialized in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(bedEnd - bedStart).count());
                fflush(stderr);
            }
        }

        // Initialize envelope manager for LMDB cache lifecycle
        {
            const char* coreConnStr = "host=localhost dbname=hcp_core user=hcp password=hcp_dev";
            if (m_envelopeManager.Initialize(m_vocabulary.GetLmdbEnv(), coreConnStr))
            {
                fprintf(stderr, "[HCPEngine] Envelope manager initialized\n");
                fflush(stderr);
            }
        }

        // Initialize entity annotator for multi-word entity recognition
        if (m_vocabulary.GetLmdbEnv())
        {
            m_entityAnnotator.Initialize(m_vocabulary.GetLmdbEnv());
        }

        // Start socket server — API for ingestion and retrieval
        bool listenAll = static_cast<bool>(hcp_listen_all);
        m_socketServer.Start(this, HCPSocketServer::DEFAULT_PORT, listenAll);

        s_instance = this;
        { FILE* df = fopen("/tmp/hcp_editor_diag.txt","a"); if(df){fprintf(df,"Activate() COMPLETE — engine ready, s_instance set\n");fclose(df);} }

        // Daemon mode: block this thread on the socket server.
        // The socket server accept loop runs until m_stopRequested is set (via signal/Stop()).
        // This prevents the O3DE headless launcher from exiting after Activate() returns.
        fprintf(stderr, "[HCPEngine] Entering daemon mode — blocking on socket server\n");
        fflush(stderr);
        m_socketServer.WaitForShutdown();
        fprintf(stderr, "[HCPEngine] Socket server exited, daemon shutting down\n");
        fflush(stderr);
    }

    void HCPEngineSystemComponent::Deactivate()
    {
        s_instance = nullptr;
        AZLOG_INFO("HCPEngine: Deactivating — shutting down socket server and PBD pipeline");
        m_socketServer.Stop();
        m_envelopeManager.Shutdown();
        m_bedManager.Shutdown();
        m_resolver.Shutdown();
        m_dbConn.Disconnect();
        m_particlePipeline.Shutdown();
        HCPEngineRequestBus::Handler::BusDisconnect();
    }

    bool HCPEngineSystemComponent::IsReady() const
    {
        return m_vocabulary.IsLoaded() && m_particlePipeline.IsInitialized();
    }

    AZStd::string HCPEngineSystemComponent::ReassembleFromPBM(const AZStd::string& docId)
    {
        if (!IsReady())
        {
            AZLOG_ERROR("HCPEngine: Not ready");
            return {};
        }

        AZLOG_INFO("HCPEngine: Reassembling from %s", docId.c_str());

        if (!m_dbConn.IsConnected())
        {
            m_dbConn.Connect();
        }
        if (!m_dbConn.IsConnected())
        {
            AZLOG_ERROR("HCPEngine: DB not connected — cannot load");
            return {};
        }

        // Load positions — direct reconstruction from positional tree
        AZStd::vector<AZStd::string> tokenIds = m_pbmReader.LoadPositions(docId);
        if (tokenIds.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to load positions for %s", docId.c_str());
            return {};
        }

        // Convert token IDs to text with stickiness rules
        AZStd::string text = TokenIdsToText(tokenIds, m_vocabulary);

        AZLOG_INFO("HCPEngine: Reassembled %zu tokens → %zu chars",
            tokenIds.size(), text.size());
        return text;
    }

    // ---- Console commands ----

    void HCPEngineSystemComponent::SourceDecode(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 2)
        {
            fprintf(stderr, "[source_decode] Usage: HCPEngineSystemComponent.SourceDecode <doc_id>\n");
            fflush(stderr);
            return;
        }

        AZStd::string docId(arguments[1].data(), arguments[1].size());

        if (!m_dbConn.IsConnected()) m_dbConn.Connect();
        if (!m_dbConn.IsConnected())
        {
            fprintf(stderr, "[source_decode] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        AZStd::vector<AZStd::string> tokenIds = m_pbmReader.LoadPositions(docId);
        if (tokenIds.empty())
        {
            fprintf(stderr, "[source_decode] ERROR: Document not found or no positions: %s\n", docId.c_str());
            fflush(stderr);
            return;
        }

        AZStd::string text = TokenIdsToText(tokenIds, m_vocabulary);

        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        fprintf(stderr, "[source_decode] %s -> %zu tokens -> %zu chars (%.1f ms)\n",
            docId.c_str(), tokenIds.size(), text.size(), ms);
        fflush(stderr);

        // Output decoded text to stdout
        fwrite(text.c_str(), 1, text.size(), stdout);
        fflush(stdout);
    }

    void HCPEngineSystemComponent::SourceList(const AZ::ConsoleCommandContainer& /*arguments*/)
    {
        if (!m_dbConn.IsConnected()) m_dbConn.Connect();
        if (!m_dbConn.IsConnected())
        {
            fprintf(stderr, "[source_list] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        auto docs = m_docQuery.ListDocuments();
        fprintf(stderr, "[source_list] %zu documents stored\n", docs.size());
        for (const auto& doc : docs)
        {
            fprintf(stderr, "  %s  %s  starters=%d  bonds=%d\n",
                doc.docId.c_str(), doc.name.c_str(), doc.starters, doc.bonds);
        }
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourceHealth(const AZ::ConsoleCommandContainer& /*arguments*/)
    {
        fprintf(stderr, "[source_health] Engine ready: %s\n", IsReady() ? "yes" : "no");
        fprintf(stderr, "[source_health] Vocabulary: %zu words, %zu labels, %zu chars\n",
            m_vocabulary.WordCount(), m_vocabulary.LabelCount(), m_vocabulary.CharCount());
        fprintf(stderr, "[source_health] Affixes: %zu loaded\n", m_vocabulary.AffixCount());
        fprintf(stderr, "[source_health] Bond tables: char->word %zu pairs, byte->char %zu pairs\n",
            m_charWordBonds.PairCount(), m_byteCharBonds.PairCount());
        fprintf(stderr, "[source_health] Socket server: %s (port %d)\n",
            m_socketServer.IsRunning() ? "running" : "stopped", HCPSocketServer::DEFAULT_PORT);
        fprintf(stderr, "[source_health] DB: %s\n",
            m_dbConn.IsConnected() ? "connected" : "disconnected");
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourceStats(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 2)
        {
            fprintf(stderr, "[source_stats] Usage: HCPEngineSystemComponent.SourceStats <doc_id>\n");
            fflush(stderr);
            return;
        }

        AZStd::string docId(arguments[1].data(), arguments[1].size());

        if (!m_dbConn.IsConnected()) m_dbConn.Connect();
        if (!m_dbConn.IsConnected())
        {
            fprintf(stderr, "[source_stats] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        PBMData pbmData = m_pbmReader.LoadPBM(docId);
        if (pbmData.bonds.empty())
        {
            fprintf(stderr, "[source_stats] ERROR: Document not found: %s\n", docId.c_str());
            fflush(stderr);
            return;
        }

        fprintf(stderr, "[source_stats] %s\n", docId.c_str());
        fprintf(stderr, "  Bonds:        %zu unique\n", pbmData.bonds.size());
        fprintf(stderr, "  Pairs:        %zu total\n", pbmData.totalPairs);
        fprintf(stderr, "  Unique tokens: %zu\n", pbmData.uniqueTokens);
        fprintf(stderr, "  Starter:      %s | %s\n",
            pbmData.firstFpbA.c_str(), pbmData.firstFpbB.c_str());
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourceVars(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 2)
        {
            fprintf(stderr, "[source_vars] Usage: HCPEngineSystemComponent.SourceVars <doc_id>\n");
            fflush(stderr);
            return;
        }

        AZStd::string docId(arguments[1].data(), arguments[1].size());

        if (!m_dbConn.IsConnected()) m_dbConn.Connect();
        if (!m_dbConn.IsConnected())
        {
            fprintf(stderr, "[source_vars] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        PBMData pbmData = m_pbmReader.LoadPBM(docId);
        if (pbmData.bonds.empty())
        {
            fprintf(stderr, "[source_vars] ERROR: Document not found: %s\n", docId.c_str());
            fflush(stderr);
            return;
        }

        // Scan bonds for VAR_REQUEST tokens (AA.AE.AF.AA.AC prefix)
        AZStd::unordered_map<AZStd::string, int> varCounts;
        for (const auto& bond : pbmData.bonds)
        {
            if (bond.tokenA.starts_with(VAR_REQUEST))
            {
                varCounts[bond.tokenA] += bond.count;
            }
            if (bond.tokenB.starts_with(VAR_REQUEST))
            {
                varCounts[bond.tokenB] += bond.count;
            }
        }

        for (const auto& [tokenId, count] : varCounts)
        {
            AZStd::string form = tokenId;
            size_t spacePos = form.find(' ');
            if (spacePos != AZStd::string::npos)
            {
                form = form.substr(spacePos + 1);
            }
            fprintf(stderr, "  var: %s  (bond refs: %d)\n", form.c_str(), count);
        }
        fprintf(stderr, "[source_vars] %s: %zu unresolved vars\n", docId.c_str(), varCounts.size());
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourcePhysTokenize(const AZ::ConsoleCommandContainer& arguments)
    {
        // Debug: print all arguments
        fprintf(stderr, "[source_phys_tokenize] %zu arguments:\n", arguments.size());
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            fprintf(stderr, "  [%zu] '%.*s'\n", i,
                static_cast<int>(arguments[i].size()), arguments[i].data());
        }
        fflush(stderr);

        if (arguments.size() < 1)
        {
            fprintf(stderr, "[source_phys_tokenize] Usage: SourcePhysTokenize <filepath> [max_chars]\n");
            fflush(stderr);
            return;
        }

        // arguments[0] = filepath (command name is NOT in the container)
        AZStd::string filePath(arguments[0].data(), arguments[0].size());
        AZ::u32 maxChars = 200;
        if (arguments.size() >= 2)
        {
            AZStd::string maxStr(arguments[1].data(), arguments[1].size());
            maxChars = static_cast<AZ::u32>(atoi(maxStr.c_str()));
            if (maxChars == 0) maxChars = 200;
        }

        // Read file
        std::ifstream ifs(filePath.c_str());
        if (!ifs.is_open())
        {
            fprintf(stderr, "[source_phys_tokenize] ERROR: Could not open '%s'\n", filePath.c_str());
            fflush(stderr);
            return;
        }
        std::string stdText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        AZStd::string text(stdText.c_str(), stdText.size());

        fprintf(stderr, "[source_phys_tokenize] File: %s (%zu bytes), max_chars: %u\n",
            filePath.c_str(), text.size(), maxChars);
        fflush(stderr);

        if (!m_particlePipeline.IsInitialized())
        {
            fprintf(stderr, "[source_phys_tokenize] ERROR: Particle pipeline not initialized\n");
            fflush(stderr);
            return;
        }

        SuperpositionTrialResult result = RunSuperpositionTrial(
            m_particlePipeline.GetPhysics(),
            m_particlePipeline.GetScene(),
            m_particlePipeline.GetCuda(),
            text,
            m_vocabulary,
            maxChars);

        fprintf(stderr, "\n[source_phys_tokenize] Trial complete: %u/%u settled (%.1f%%) [%u bytes → %u codepoints]\n",
            result.settledCount, result.totalCodepoints,
            result.totalCodepoints > 0 ? 100.0f * result.settledCount / result.totalCodepoints : 0.0f,
            result.totalBytes, result.totalCodepoints);
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourcePhysWordTrial(const AZ::ConsoleCommandContainer& arguments)
    {
        fprintf(stderr, "[source_phys_word_trial] %zu arguments:\n", arguments.size());
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            fprintf(stderr, "  [%zu] '%.*s'\n", i,
                static_cast<int>(arguments[i].size()), arguments[i].data());
        }
        fflush(stderr);

        if (arguments.size() < 1)
        {
            fprintf(stderr, "[source_phys_word_trial] Usage: SourcePhysWordTrial <filepath> [max_chars]\n");
            fflush(stderr);
            return;
        }

        AZStd::string filePath(arguments[0].data(), arguments[0].size());
        AZ::u32 maxChars = 200;
        if (arguments.size() >= 2)
        {
            AZStd::string maxStr(arguments[1].data(), arguments[1].size());
            maxChars = static_cast<AZ::u32>(atoi(maxStr.c_str()));
            if (maxChars == 0) maxChars = 200;
        }

        std::ifstream ifs(filePath.c_str());
        if (!ifs.is_open())
        {
            fprintf(stderr, "[source_phys_word_trial] ERROR: Could not open '%s'\n", filePath.c_str());
            fflush(stderr);
            return;
        }
        std::string stdText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        AZStd::string text(stdText.c_str(), stdText.size());

        fprintf(stderr, "[source_phys_word_trial] File: %s (%zu bytes), max_chars: %u\n",
            filePath.c_str(), text.size(), maxChars);
        fflush(stderr);

        if (!m_particlePipeline.IsInitialized())
        {
            fprintf(stderr, "[source_phys_word_trial] ERROR: Particle pipeline not initialized\n");
            fflush(stderr);
            return;
        }

        WordTrialResult result = RunWordSuperpositionTrial(
            m_particlePipeline.GetPhysics(),
            m_particlePipeline.GetScene(),
            m_particlePipeline.GetCuda(),
            text,
            m_vocabulary,
            maxChars);

        fprintf(stderr, "\n[source_phys_word_trial] Trial complete: %u/%u runs resolved (%.1f%%)\n",
            result.resolvedRuns, result.totalRuns,
            result.totalRuns > 0 ? 100.0f * result.resolvedRuns / result.totalRuns : 0.0f);
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourcePhysWordResolve(const AZ::ConsoleCommandContainer& arguments)
    {
        fprintf(stderr, "[source_phys_word_resolve] %zu arguments:\n", arguments.size());
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            fprintf(stderr, "  [%zu] '%.*s'\n", i,
                static_cast<int>(arguments[i].size()), arguments[i].data());
        }
        fflush(stderr);

        if (arguments.size() < 1)
        {
            fprintf(stderr, "[source_phys_word_resolve] Usage: SourcePhysWordResolve <filepath> [max_chars]\n");
            fflush(stderr);
            return;
        }

        AZStd::string filePath(arguments[0].data(), arguments[0].size());
        AZ::u32 maxChars = 200;
        if (arguments.size() >= 2)
        {
            AZStd::string maxStr(arguments[1].data(), arguments[1].size());
            maxChars = static_cast<AZ::u32>(atoi(maxStr.c_str()));
            if (maxChars == 0) maxChars = 200;
        }

        // Read file
        std::ifstream ifs(filePath.c_str());
        if (!ifs.is_open())
        {
            fprintf(stderr, "[source_phys_word_resolve] ERROR: Could not open '%s'\n", filePath.c_str());
            fflush(stderr);
            return;
        }
        std::string stdText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        AZStd::string text(stdText.c_str(), stdText.size());

        fprintf(stderr, "[source_phys_word_resolve] File: %s (%zu bytes), max_chars: %u\n",
            filePath.c_str(), text.size(), maxChars);
        fflush(stderr);

        if (!m_particlePipeline.IsInitialized())
        {
            fprintf(stderr, "[source_phys_word_resolve] ERROR: Particle pipeline not initialized\n");
            fflush(stderr);
            return;
        }

        if (m_charWordBonds.PairCount() == 0)
        {
            fprintf(stderr, "[source_phys_word_resolve] ERROR: No char->word bond table loaded\n");
            fflush(stderr);
            return;
        }

        // Step 1: Run Phase 1 (byte→char settlement), then extract runs from settled chars
        SuperpositionTrialResult phase1 = RunSuperpositionTrial(
            m_particlePipeline.GetPhysics(),
            m_particlePipeline.GetScene(),
            m_particlePipeline.GetCuda(),
            text, m_vocabulary, maxChars);

        fprintf(stderr, "[source_phys_word_resolve] Phase 1: %u/%u settled (%.1f%%) in %.1f ms [%u bytes → %u codepoints]\n",
            phase1.settledCount, phase1.totalCodepoints,
            phase1.totalCodepoints > 0 ? 100.0f * phase1.settledCount / phase1.totalCodepoints : 0.0f,
            phase1.simulationTimeMs, phase1.totalBytes, phase1.totalCodepoints);
        fflush(stderr);

        AZStd::vector<CharRun> runs = ExtractRunsFromCollapses(phase1);
        fprintf(stderr, "[source_phys_word_resolve] Extracted %zu runs from Phase 1 output\n", runs.size());
        fflush(stderr);

        if (runs.empty())
        {
            fprintf(stderr, "[source_phys_word_resolve] No runs extracted\n");
            fflush(stderr);
            return;
        }

        // Step 2: Use persistent BedManager (initialized at Activate)
        if (!m_bedManager.IsInitialized())
        {
            fprintf(stderr, "[source_phys_word_resolve] ERROR: BedManager not initialized\n");
            fflush(stderr);
            return;
        }

        fprintf(stderr, "[source_phys_word_resolve] BedManager ready (LMDB vocab beds)\n");
        fflush(stderr);

        ResolutionManifest manifest = m_bedManager.Resolve(runs);

        // ---- Benchmark TSV output ----
        {
            // Get system resource usage
            struct rusage usage;
            getrusage(RUSAGE_SELF, &usage);
            long rssKb = usage.ru_maxrss;  // Peak RSS in KB (Linux)

            // Read GPU memory from /proc if available (nvidia-smi parsing is too slow)
            long vramUsedMb = 0;
            FILE* nvsmi = popen("nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null", "r");
            if (nvsmi)
            {
                char buf[64];
                if (fgets(buf, sizeof(buf), nvsmi))
                    vramUsedMb = atol(buf);
                pclose(nvsmi);
            }

            // Ensure benchmarks/ directory exists
            mkdir("benchmarks", 0755);

            // Timestamp for filename
            auto now = std::chrono::system_clock::now();
            auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();

            char tsvPath[256];
            snprintf(tsvPath, sizeof(tsvPath), "benchmarks/resolve_%lld.tsv", (long long)epochMs);

            FILE* tsv = fopen(tsvPath, "w");
            if (tsv)
            {
                // Header
                fprintf(tsv, "timestamp\tfile\tbytes\tphase1_codepoints\tphase1_settled\t"
                    "phase1_pct\tphase1_ms\ttotal_runs\tresolved\tunresolved\t"
                    "resolved_pct\tresolve_ms\trss_kb\tvram_mb\n");
                // Data row
                fprintf(tsv, "%lld\t%s\t%zu\t%u\t%u\t%.1f\t%.1f\t%u\t%u\t%u\t%.1f\t%.1f\t%ld\t%ld\n",
                    (long long)epochMs,
                    filePath.c_str(),
                    text.size(),
                    phase1.totalCodepoints,
                    phase1.settledCount,
                    phase1.totalCodepoints > 0 ? 100.0f * phase1.settledCount / phase1.totalCodepoints : 0.0f,
                    phase1.simulationTimeMs,
                    manifest.totalRuns,
                    manifest.resolvedRuns,
                    manifest.unresolvedRuns,
                    manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f,
                    manifest.totalTimeMs,
                    rssKb,
                    vramUsedMb);
                fclose(tsv);
                fprintf(stderr, "[source_phys_word_resolve] Benchmark written: %s\n", tsvPath);
                fflush(stderr);
            }
        }

        // Step 5: Report results
        fprintf(stderr, "\n[source_phys_word_resolve] === Resolution Manifest ===\n");
        fprintf(stderr, "  Total runs:      %u\n", manifest.totalRuns);
        fprintf(stderr, "  Resolved:        %u (%.1f%%)\n",
            manifest.resolvedRuns,
            manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f);
        fprintf(stderr, "  Unresolved:      %u\n", manifest.unresolvedRuns);
        fprintf(stderr, "  Time:            %.1f ms\n", manifest.totalTimeMs);
        fflush(stderr);

        // Per-run detail
        fprintf(stderr, "\n[source_phys_word_resolve] Per-run results:\n");
        AZ::u32 printLimit = 50;
        for (AZ::u32 i = 0; i < static_cast<AZ::u32>(manifest.results.size()) && i < printLimit; ++i)
        {
            const ResolutionResult& r = manifest.results[i];
            if (r.resolved)
            {
                fprintf(stderr, "  [%u] \"%s\" -> \"%s\" (tier %u, token %s)\n",
                    i, r.runText.c_str(), r.matchedWord.c_str(),
                    r.tierResolved, r.matchedTokenId.c_str());
            }
            else
            {
                fprintf(stderr, "  [%u] \"%s\" -> UNRESOLVED (var candidate)\n",
                    i, r.runText.c_str());
            }
        }
        if (manifest.results.size() > printLimit)
        {
            fprintf(stderr, "  ... (%zu more results)\n",
                manifest.results.size() - printLimit);
        }
        fflush(stderr);

        // Step 6: Validate against computational tokenizer
        fprintf(stderr, "\n[source_phys_word_resolve] === Validation vs Computational Tokenizer ===\n");
        AZ::u32 matchCount = 0;
        AZ::u32 mismatchCount = 0;
        AZ::u32 compResolvedCount = 0;

        for (const auto& r : manifest.results)
        {
            if (!r.resolved) continue;

            // Look up the run text in the vocabulary via computational path
            AZStd::string compTokenId = m_vocabulary.LookupWordLocal(r.runText);
            if (compTokenId.empty())
            {
                // Try with original case — vocabulary might store differently
                compTokenId = m_vocabulary.LookupWord(r.runText);
            }

            ++compResolvedCount;

            if (!compTokenId.empty() && compTokenId == r.matchedTokenId)
            {
                ++matchCount;
            }
            else
            {
                ++mismatchCount;
                fprintf(stderr, "  MISMATCH: \"%s\" physics=%s comp=%s\n",
                    r.runText.c_str(), r.matchedTokenId.c_str(),
                    compTokenId.empty() ? "(not found)" : compTokenId.c_str());
            }
        }

        fprintf(stderr, "  Physics resolved: %u, Validated: %u/%u (%.1f%%), Mismatches: %u\n",
            manifest.resolvedRuns, matchCount, compResolvedCount,
            compResolvedCount > 0 ? 100.0f * matchCount / compResolvedCount : 0.0f,
            mismatchCount);
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourceActivateEnvelope(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 1)
        {
            fprintf(stderr, "[source_activate_envelope] Usage: SourceActivateEnvelope <envelope_name>\n");
            fflush(stderr);
            return;
        }

        AZStd::string name(arguments[0].data(), arguments[0].size());

        fprintf(stderr, "[source_activate_envelope] Activating envelope '%s'...\n", name.c_str());
        fflush(stderr);

        EnvelopeActivation result = m_envelopeManager.ActivateEnvelope(name);

        fprintf(stderr, "[source_activate_envelope] Result: %d entries loaded, %d evicted, %.1f ms\n",
            result.entriesLoaded, result.evictedEntries, result.loadTimeMs);
        fflush(stderr);

        // Rebuild BedManager vocab index from the newly populated w2t
        if (result.entriesLoaded > 0)
            m_bedManager.RebuildVocab();
    }
}
