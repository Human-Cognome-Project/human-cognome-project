
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/ILogger.h>

#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPStorage.h"
#include "HCPBondCompiler.h"

#include <AzCore/std/sort.h>
#include <fstream>
#include <chrono>
#include <cstdio>

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

        // Start socket server — API for ingestion and retrieval
        bool listenAll = static_cast<bool>(hcp_listen_all);
        m_socketServer.Start(this, HCPSocketServer::DEFAULT_PORT, listenAll);
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

        // Step 1: Tokenize
        TokenStream stream = Tokenize(text, m_vocabulary);
        if (stream.tokenIds.empty())
        {
            AZLOG_ERROR("HCPEngine: Tokenization produced no tokens");
            return {};
        }

        // Step 2: Derive PBM bonds
        PBMData pbmData = DerivePBM(stream);

        // Step 3: Store PBM via write kernel
        if (!m_writeKernel.IsConnected())
        {
            m_writeKernel.Connect();
        }
        if (!m_writeKernel.IsConnected())
        {
            AZLOG_ERROR("HCPEngine: Write kernel not connected — cannot store");
            return {};
        }

        AZStd::string docId = m_writeKernel.StorePBM(docName, centuryCode, pbmData);
        if (docId.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to store PBM");
            return {};
        }

        // Step 4: Store positions alongside bonds
        m_writeKernel.StorePositions(
            m_writeKernel.LastDocPk(),
            stream.tokenIds,
            stream.positions,
            stream.totalSlots);

        AZLOG_INFO("HCPEngine: Stored %s — %zu tokens, %zu bonds, %d slots",
            docId.c_str(), stream.tokenIds.size(), pbmData.bonds.size(), stream.totalSlots);

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

        if (!m_writeKernel.IsConnected())
        {
            m_writeKernel.Connect();
        }
        if (!m_writeKernel.IsConnected())
        {
            AZLOG_ERROR("HCPEngine: Write kernel not connected — cannot load");
            return {};
        }

        // Load positions — direct reconstruction from positional tree
        AZStd::vector<AZStd::string> tokenIds = m_writeKernel.LoadPositions(docId);
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

    void HCPEngineSystemComponent::SourceIngest(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 2)
        {
            fprintf(stderr, "[source_ingest] Usage: HCPEngineSystemComponent.SourceIngest <filepath> [century]\n");
            fflush(stderr);
            return;
        }

        AZStd::string filePath(arguments[1].data(), arguments[1].size());
        AZStd::string centuryCode = "AS";
        if (arguments.size() >= 3)
        {
            centuryCode = AZStd::string(arguments[2].data(), arguments[2].size());
        }

        // Read file
        std::ifstream ifs(filePath.c_str());
        if (!ifs.is_open())
        {
            fprintf(stderr, "[source_ingest] ERROR: Could not open '%s'\n", filePath.c_str());
            fflush(stderr);
            return;
        }
        std::string stdText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        AZStd::string text(stdText.c_str(), stdText.size());

        // Derive document name from filename
        AZStd::string docName = filePath;
        size_t lastSlash = docName.rfind('/');
        if (lastSlash != AZStd::string::npos) docName = docName.substr(lastSlash + 1);
        size_t lastDot = docName.rfind('.');
        if (lastDot != AZStd::string::npos) docName = docName.substr(0, lastDot);

        fprintf(stderr, "[source_ingest] %s (%zu bytes)\n", docName.c_str(), text.size());
        fflush(stderr);

        auto t0 = std::chrono::high_resolution_clock::now();

        TokenStream stream = Tokenize(text, m_vocabulary);
        if (stream.tokenIds.empty())
        {
            fprintf(stderr, "[source_ingest] ERROR: Tokenization produced no tokens\n");
            fflush(stderr);
            return;
        }

        PBMData pbmData = DerivePBM(stream);

        // Store PBM
        if (!m_writeKernel.IsConnected()) m_writeKernel.Connect();
        AZStd::string docId;
        if (m_writeKernel.IsConnected())
        {
            docId = m_writeKernel.StorePBM(docName, centuryCode, pbmData);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        fprintf(stderr, "[source_ingest] Encoded: %zu tokens\n",
            stream.tokenIds.size());
        fprintf(stderr, "[source_ingest] Bonds: %zu unique, %zu total pairs\n",
            pbmData.bonds.size(), pbmData.totalPairs);
        fprintf(stderr, "[source_ingest] Time: %.1f ms\n", ms);
        if (!docId.empty())
            fprintf(stderr, "[source_ingest] Stored -> %s\n", docId.c_str());
        else
            fprintf(stderr, "[source_ingest] WARNING: Not stored (DB unavailable)\n");
        fflush(stderr);
    }

    void HCPEngineSystemComponent::SourceDecode(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 2)
        {
            fprintf(stderr, "[source_decode] Usage: HCPEngineSystemComponent.SourceDecode <doc_id>\n");
            fflush(stderr);
            return;
        }

        AZStd::string docId(arguments[1].data(), arguments[1].size());

        if (!m_writeKernel.IsConnected()) m_writeKernel.Connect();
        if (!m_writeKernel.IsConnected())
        {
            fprintf(stderr, "[source_decode] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        AZStd::vector<AZStd::string> tokenIds = m_writeKernel.LoadPositions(docId);
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
        if (!m_writeKernel.IsConnected()) m_writeKernel.Connect();
        if (!m_writeKernel.IsConnected())
        {
            fprintf(stderr, "[source_list] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        auto docs = m_writeKernel.ListDocuments();
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
            m_writeKernel.IsConnected() ? "connected" : "disconnected");
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

        if (!m_writeKernel.IsConnected()) m_writeKernel.Connect();
        if (!m_writeKernel.IsConnected())
        {
            fprintf(stderr, "[source_stats] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        PBMData pbmData = m_writeKernel.LoadPBM(docId);
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

        if (!m_writeKernel.IsConnected()) m_writeKernel.Connect();
        if (!m_writeKernel.IsConnected())
        {
            fprintf(stderr, "[source_vars] ERROR: Database not available\n");
            fflush(stderr);
            return;
        }

        PBMData pbmData = m_writeKernel.LoadPBM(docId);
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
}
