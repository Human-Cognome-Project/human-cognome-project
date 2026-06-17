
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/ILogger.h>

#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPBondCompiler.h"
#include "HCPResolutionChamber.h"
#include "HCPVocabBed.h"
#include "HCPByteIngest.h"
#include <cstdlib>

#include <AzCore/std/sort.h>
#include <libpq-fe.h>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <sys/resource.h>
#include <sys/stat.h>

#include <HCPEngine/HCPEngineTypeIds.h>

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
        // No PhysX dependency — the engine is AZSL/host-resident (PhysX swap complete).
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
                    "host=192.168.68.60 port=5435 dbname=hcp_english user=hcp password=hcp_dev");
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
                    "host=192.168.68.60 port=5435 dbname=hcp_core user=hcp password=hcp_dev");
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

        AZLOG_INFO("HCPEngine: Ready — vocab: %zu words, %zu labels, %zu chars (AZSL engine, no PhysX)",
            m_vocabulary.WordCount(), m_vocabulary.LabelCount(), m_vocabulary.CharCount());

        // Initialize persistent vocab beds from pre-compiled LMDB (Phase 2)
        // Vocab data is pre-ordered at compile time — no Postgres, no TierAssembly
        {
            auto bedStart = std::chrono::high_resolution_clock::now();

            if (m_vocabulary.GetLmdbEnv())
            {
                m_bedManager.Initialize(
                    m_vocabulary.GetLmdbEnv(),
                    &m_vocabulary,
                    &m_envelopeManager);

                auto bedEnd = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine] Persistent vocab beds initialized in %.1f ms\n",
                    std::chrono::duration<double, std::milli>(bedEnd - bedStart).count());
                fflush(stderr);
            }
        }

        // --- raw-bytes test hook: HCP_RESOLVE_FILE=<file> -> bytes straight into ResolveBytes -> word tokens ---
        if (const char* rf = std::getenv("HCP_RESOLVE_FILE"))
        {
            std::ifstream rfs(rf, std::ios::binary);
            std::string rbytes((std::istreambuf_iterator<char>(rfs)), std::istreambuf_iterator<char>());
            if (m_bedManager.IsInitialized() && !rbytes.empty())
            {
                ResolutionManifest rm = ResolveBytes(m_bedManager,
                    reinterpret_cast<const uint8_t*>(rbytes.data()), rbytes.size());
                fprintf(stderr, "[byte_resolve] %zu bytes -> %u runs, %u resolved\n",
                    rbytes.size(), rm.totalRuns, rm.resolvedRuns);
                for (const auto& rr : rm.results)
                    fprintf(stderr, "  %-24s -> %s %s\n", rr.runText.c_str(),
                        rr.resolved ? rr.matchedTokenId.c_str() : "(unresolved)",
                        rr.resolved ? rr.matchedWord.c_str() : "");
                fflush(stderr);
            }
        }

        // Initialize envelope manager for LMDB cache lifecycle
        {
            const char* coreConnStr = "host=192.168.68.60 port=5435 dbname=hcp_envelope user=hcp password=hcp_dev";
            if (m_envelopeManager.Initialize(m_vocabulary.GetLmdbEnv(), coreConnStr))
            {
                fprintf(stderr, "[HCPEngine] Envelope manager initialized\n");
                fflush(stderr);

                // Load inflection rules from hcp_english and wire into BedManager.
                // dbName is flaggable — defaults to "hcp_english" for English shard.
                auto rules = m_envelopeManager.LoadInflectionRules("hcp_english");
                if (!rules.empty())
                    m_bedManager.SetInflectionRules(AZStd::move(rules));

                // Activate the English resolution envelope: loads the query plan ONLY.
                // No cold→warm assembly at startup — assembly is demand-driven per
                // word length when the resolution loop first requests candidates.
                // Existing working set rows from previous sessions are reused.
                // LMDB is cleared (ephemeral hot cache) and rebuilt on demand.
                auto t0 = std::chrono::high_resolution_clock::now();
                fprintf(stderr, "[HCPEngine] Activating envelope 'english_resolve'...\n");
                fflush(stderr);
                EnvelopeActivation act = m_envelopeManager.ActivateEnvelope("english_resolve");
                int warmSize = m_envelopeManager.GetWorkingSetSize(act.envelopeId);
                m_bedManager.InitEnvelopeWindow(act.envelopeId, warmSize);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                fprintf(stderr, "[HCPEngine] Envelope ready: %d existing warm rows, %.1f ms\n",
                    warmSize, ms);
                fflush(stderr);
            }
        }

        // Initialize entity annotator for multi-word entity recognition
        if (m_vocabulary.GetLmdbEnv())
        {
            m_entityAnnotator.Initialize(m_vocabulary.GetLmdbEnv());
        }

        // Connect to hcp_var — personal vars / working document store
        m_varConn = PQconnectdb("host=localhost dbname=hcp_var user=hcp password=hcp_dev");
        if (PQstatus(m_varConn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPEngine] WARNING: hcp_var connection failed: %s\n", PQerrorMessage(m_varConn));
            fflush(stderr);
            PQfinish(m_varConn);
            m_varConn = nullptr;
        }
        else
        {
            fprintf(stderr, "[HCPEngine] hcp_var connected\n");
            fflush(stderr);
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
        if (m_varConn) { PQfinish(m_varConn); m_varConn = nullptr; }
        HCPEngineRequestBus::Handler::BusDisconnect();
    }

    int HCPEngineSystemComponent::StoreWorkingDoc(
        const AZStd::string& name,
        const AZStd::string& rawJson,
        const AZStd::string& resolvedJson)
    {
        if (!m_varConn || PQstatus(m_varConn) != CONNECTION_OK) return 0;

        const char* params[3] = { name.c_str(), rawJson.c_str(), resolvedJson.c_str() };
        PGresult* res = PQexecParams(m_varConn,
            "INSERT INTO working_docs (name, raw_content, resolved) "
            "VALUES ($1, $2::jsonb, $3::jsonb) RETURNING id",
            3, nullptr, params, nullptr, nullptr, 0);

        int id = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            id = atoi(PQgetvalue(res, 0, 0));
        else
            fprintf(stderr, "[HCPEngine] StoreWorkingDoc failed: %s\n", PQerrorMessage(m_varConn));

        PQclear(res);
        return id;
    }

    bool HCPEngineSystemComponent::IsReady() const
    {
        return m_vocabulary.IsLoaded() && m_bedManager.IsInitialized();
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
        AZStd::vector<AZStd::string> words;
        AZStd::vector<AZ::u32> modifiers;
        if (!m_pbmReader.LoadPositionsWithModifiers(docId, m_vocabulary, words, modifiers) || words.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to load positions for %s", docId.c_str());
            return {};
        }

        // Reconstruct text from pre-resolved words
        AZStd::string text = TokenIdsToText(words, &modifiers);

        AZLOG_INFO("HCPEngine: Reassembled %zu words → %zu chars",
            words.size(), text.size());
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

        AZStd::vector<AZStd::string> words;
        AZStd::vector<AZ::u32> modifiers;
        if (!m_pbmReader.LoadPositionsWithModifiers(docId, m_vocabulary, words, modifiers) || words.empty())
        {
            fprintf(stderr, "[source_decode] ERROR: Document not found or no positions: %s\n", docId.c_str());
            fflush(stderr);
            return;
        }

        AZStd::string text = TokenIdsToText(words, &modifiers);

        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        fprintf(stderr, "[source_decode] %s -> %zu words -> %zu chars (%.1f ms)\n",
            docId.c_str(), words.size(), text.size(), ms);
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
        int warmSize = m_envelopeManager.GetWorkingSetSize(result.envelopeId);
        m_bedManager.InitEnvelopeWindow(result.envelopeId, warmSize);

        fprintf(stderr, "[source_activate_envelope] Result: %d warm, %d evicted, %.1f ms\n",
            warmSize, result.evictedEntries, result.loadTimeMs);
        fflush(stderr);
    }
}
