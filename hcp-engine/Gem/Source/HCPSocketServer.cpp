#include "HCPSocketServer.h"
#include "HCPEngineSystemComponent.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"
#include "HCPParticlePipeline.h"
#include "HCPResolutionChamber.h"
#include "HCPVocabBed.h"
#include "HCPSuperpositionTrial.h"
#include "HCPWordSuperpositionTrial.h"
#include "HCPBondCompiler.h"
#include "HCPEnvelopeManager.h"
#include "HCPPhysIngest.h"
#include "HCPGutenbergRunner.h"

#include <AzCore/Console/ILogger.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sys/resource.h>
#include <sys/stat.h>

namespace HCPEngine
{
    // Read exactly N bytes from a socket. Returns false on error/disconnect.
    static bool ReadExact(int fd, void* buf, size_t len)
    {
        auto* p = static_cast<uint8_t*>(buf);
        size_t remaining = len;
        while (remaining > 0)
        {
            ssize_t n = ::recv(fd, p, remaining, 0);
            if (n <= 0) return false;
            p += n;
            remaining -= static_cast<size_t>(n);
        }
        return true;
    }

    // Write exactly N bytes to a socket. Returns false on error.
    static bool WriteExact(int fd, const void* buf, size_t len)
    {
        auto* p = static_cast<const uint8_t*>(buf);
        size_t remaining = len;
        while (remaining > 0)
        {
            ssize_t n = ::send(fd, p, remaining, MSG_NOSIGNAL);
            if (n <= 0) return false;
            p += n;
            remaining -= static_cast<size_t>(n);
        }
        return true;
    }

    // Read a length-prefixed message: 4 bytes big-endian length + payload
    static bool ReadMessage(int fd, AZStd::string& out)
    {
        uint32_t lenNet;
        if (!ReadExact(fd, &lenNet, 4)) return false;
        uint32_t len = ntohl(lenNet);

        // Sanity: max 64 MB per message
        if (len > 64 * 1024 * 1024) return false;

        out.resize(len);
        return ReadExact(fd, out.data(), len);
    }

    // Write a length-prefixed message
    static bool WriteMessage(int fd, const AZStd::string& msg)
    {
        uint32_t lenNet = htonl(static_cast<uint32_t>(msg.size()));
        if (!WriteExact(fd, &lenNet, 4)) return false;
        return WriteExact(fd, msg.data(), msg.size());
    }

    HCPSocketServer::~HCPSocketServer()
    {
        Stop();
    }

    bool HCPSocketServer::Start(HCPEngineSystemComponent* engine, int port, bool listenAll)
    {
        if (m_running.load()) return true;

        m_engine = engine;
        m_listenAll = listenAll;
        m_stopRequested.store(false);
        m_thread = std::thread(&HCPSocketServer::ListenerThread, this, port);
        return true;
    }

    void HCPSocketServer::Stop()
    {
        m_stopRequested.store(true);
        if (m_listenFd >= 0)
        {
            ::shutdown(m_listenFd, SHUT_RDWR);
            ::close(m_listenFd);
            m_listenFd = -1;
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        m_running.store(false);
    }

    void HCPSocketServer::WaitForShutdown()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void HCPSocketServer::ListenerThread(int port)
    {
        m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenFd < 0)
        {
            AZLOG_ERROR("HCPSocketServer: socket() failed: %s", strerror(errno));
            return;
        }

        int opt = 1;
        ::setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = m_listenAll ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(port));

        if (::bind(m_listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            AZLOG_ERROR("HCPSocketServer: bind() failed on port %d: %s", port, strerror(errno));
            ::close(m_listenFd);
            m_listenFd = -1;
            return;
        }

        if (::listen(m_listenFd, 4) < 0)
        {
            AZLOG_ERROR("HCPSocketServer: listen() failed: %s", strerror(errno));
            ::close(m_listenFd);
            m_listenFd = -1;
            return;
        }

        m_running.store(true);
        fprintf(stderr, "[HCPSocketServer] Listening on %s:%d\n",
            m_listenAll ? "0.0.0.0" : "127.0.0.1", port);
        fflush(stderr);

        while (!m_stopRequested.load())
        {
            struct sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = ::accept(m_listenFd,
                reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);

            if (clientFd < 0)
            {
                fprintf(stderr, "[HCPSocketServer] accept() failed: %s (errno=%d, stopRequested=%d)\n",
                    strerror(errno), errno, m_stopRequested.load() ? 1 : 0);
                fflush(stderr);
                if (m_stopRequested.load()) break;
                if (errno == EBADF || errno == EINVAL)
                {
                    // Listen fd was closed or is invalid — exit loop
                    fprintf(stderr, "[HCPSocketServer] Listen fd invalid — exiting accept loop\n");
                    fflush(stderr);
                    break;
                }
                continue;
            }

            fprintf(stderr, "[HCPSocketServer] Client connected\n");
            fflush(stderr);

            HandleClient(clientFd);

            ::close(clientFd);
            fprintf(stderr, "[HCPSocketServer] Client disconnected\n");
            fflush(stderr);
        }

        fprintf(stderr, "[HCPSocketServer] Accept loop exited (stopRequested=%d, listenFd=%d)\n",
            m_stopRequested.load() ? 1 : 0, m_listenFd);
        fflush(stderr);
        m_running.store(false);
    }

    void HCPSocketServer::HandleClient(int clientFd)
    {
        while (!m_stopRequested.load())
        {
            AZStd::string request;
            if (!ReadMessage(clientFd, request))
            {
                break;  // Client disconnected or error
            }

            AZStd::string response = ProcessRequest(request);
            if (!WriteMessage(clientFd, response))
            {
                break;  // Write error
            }
        }
    }

    AZStd::string HCPSocketServer::ProcessRequest(const AZStd::string& json)
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());

        if (doc.HasParseError() || !doc.IsObject())
        {
            return R"({"status":"error","message":"Invalid JSON"})";
        }

        if (!doc.HasMember("action") || !doc["action"].IsString())
        {
            return R"({"status":"error","message":"Missing 'action' field"})";
        }

        const char* action = doc["action"].GetString();

        // ---- health ----
        if (strcmp(action, "health") == 0)
        {
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("ready"); w.Bool(m_engine->IsEngineReady());
            w.Key("words"); w.Uint64(m_engine->GetVocabulary().WordCount());
            w.Key("labels"); w.Uint64(m_engine->GetVocabulary().LabelCount());
            w.Key("chars"); w.Uint64(m_engine->GetVocabulary().CharCount());
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- retrieve ----
        if (strcmp(action, "retrieve") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
            {
                return R"({"status":"error","message":"Missing 'doc_id' field"})";
            }

            AZStd::string docId(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());

            auto t0 = std::chrono::high_resolution_clock::now();

            // Load positions from DB — direct reconstruction
            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected())
            {
                db.Connect();
            }
            if (!db.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            AZStd::vector<AZStd::string> words;
            AZStd::vector<AZ::u32> posModifiers;
            if (!m_engine->GetPbmReader().LoadPositionsWithModifiers(docId, m_engine->GetVocabulary(), words, posModifiers)
                || words.empty())
            {
                return R"({"status":"error","message":"Document not found or has no positions"})";
            }

            auto tLoad = std::chrono::high_resolution_clock::now();

            // Reconstruct text from pre-resolved words with stickiness rules + stored cap/morph modifiers
            AZStd::string text = TokenIdsToText(words, &posModifiers);

            auto t1 = std::chrono::high_resolution_clock::now();
            double loadMs = std::chrono::duration<double, std::milli>(tLoad - t0).count();
            double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

            fprintf(stderr, "[HCPSocketServer] Retrieved '%s': %zu words -> %zu chars, %.1f ms\n",
                docId.c_str(), words.size(), text.size(), totalMs);
            fflush(stderr);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("text"); w.String(text.c_str(), static_cast<rapidjson::SizeType>(text.size()));
            w.Key("tokens"); w.Uint64(words.size());
            w.Key("load_ms"); w.Double(loadMs);
            w.Key("ms"); w.Double(totalMs);
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- list ----
        if (strcmp(action, "list") == 0)
        {
            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected())
            {
                db.Connect();
            }
            if (!db.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            auto docs = m_engine->GetDocumentQuery().ListDocuments();

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("count"); w.Uint64(docs.size());
            w.Key("documents");
            w.StartArray();
            for (const auto& d : docs)
            {
                w.StartObject();
                w.Key("doc_id"); w.String(d.docId.c_str());
                w.Key("name"); w.String(d.name.c_str());
                w.Key("starters"); w.Int(d.starters);
                w.Key("bonds"); w.Int(d.bonds);
                w.EndObject();
            }
            w.EndArray();
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- info (full document detail) ----
        if (strcmp(action, "info") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
            {
                return R"({"status":"error","message":"Missing 'doc_id' field"})";
            }

            AZStd::string docId(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());

            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected()) db.Connect();
            if (!db.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            HCPDocumentQuery& docQuery = m_engine->GetDocumentQuery();
            auto detail = docQuery.GetDocumentDetail(docId);
            if (detail.pk == 0)
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            auto prov = docQuery.GetProvenance(detail.pk);
            auto vars = m_engine->GetDocVarQuery().GetDocVars(detail.pk);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("doc_id"); w.String(detail.docId.c_str());
            w.Key("name"); w.String(detail.name.c_str());
            w.Key("total_slots"); w.Int(detail.totalSlots);
            w.Key("unique"); w.Int(detail.uniqueTokens);
            w.Key("starters"); w.Int(detail.starters);
            w.Key("bonds"); w.Int(detail.bonds);

            // Metadata — emit as raw JSON object
            w.Key("metadata");
            w.RawValue(detail.metadataJson.c_str(),
                       static_cast<rapidjson::SizeType>(detail.metadataJson.size()),
                       rapidjson::kObjectType);

            // Provenance
            if (prov.found)
            {
                w.Key("provenance");
                w.StartObject();
                w.Key("source_type"); w.String(prov.sourceType.c_str());
                w.Key("source_path"); w.String(prov.sourcePath.c_str());
                w.Key("source_format"); w.String(prov.sourceFormat.c_str());
                w.Key("catalog"); w.String(prov.catalog.c_str());
                w.Key("catalog_id"); w.String(prov.catalogId.c_str());
                w.EndObject();
            }

            // Vars
            if (!vars.empty())
            {
                w.Key("vars");
                w.StartArray();
                for (const auto& v : vars)
                {
                    w.StartObject();
                    w.Key("var_id"); w.String(v.varId.c_str());
                    w.Key("surface"); w.String(v.surface.c_str());
                    w.EndObject();
                }
                w.EndArray();
            }

            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- delete_doc ----
        if (strcmp(action, "delete_doc") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
                return R"({"status":"error","message":"Missing 'doc_id' field"})";

            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected()) db.Connect();
            if (!db.IsConnected())
                return R"({"status":"error","message":"Database not available"})";

            AZStd::string docIdStr(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());
            int docPk = m_engine->GetDocumentQuery().GetDocPk(docIdStr);
            if (docPk == 0)
            {
                rapidjson::StringBuffer sb;
                rapidjson::Writer<rapidjson::StringBuffer> w(sb);
                w.StartObject();
                w.Key("status"); w.String("error");
                w.Key("message"); w.String("Document not found");
                w.Key("doc_id"); w.String(docIdStr.c_str());
                w.EndObject();
                return AZStd::string(sb.GetString(), sb.GetSize());
            }

            int deleted = m_engine->GetDocumentQuery().DeleteDocument(docPk);
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String(deleted > 0 ? "ok" : "error");
            w.Key("doc_id"); w.String(docIdStr.c_str());
            w.Key("deleted"); w.Int(deleted);
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- update_meta ----
        if (strcmp(action, "update_meta") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
            {
                return R"({"status":"error","message":"Missing 'doc_id' field"})";
            }

            AZStd::string docId(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());

            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected()) db.Connect();
            if (!db.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            HCPDocumentQuery& docQuery = m_engine->GetDocumentQuery();
            int docPk = docQuery.GetDocPk(docId);
            if (docPk == 0)
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            // Build setJson from "set" object
            AZStd::string setJson = "{}";
            int fieldsSet = 0;
            if (doc.HasMember("set") && doc["set"].IsObject())
            {
                rapidjson::StringBuffer setSb;
                rapidjson::Writer<rapidjson::StringBuffer> setW(setSb);
                doc["set"].Accept(setW);
                setJson = AZStd::string(setSb.GetString(), setSb.GetSize());
                fieldsSet = static_cast<int>(doc["set"].MemberCount());
            }

            // Build removeKeys from "remove" array
            AZStd::vector<AZStd::string> removeKeys;
            if (doc.HasMember("remove") && doc["remove"].IsArray())
            {
                for (auto& v : doc["remove"].GetArray())
                {
                    if (v.IsString())
                    {
                        removeKeys.push_back(AZStd::string(v.GetString(), v.GetStringLength()));
                    }
                }
            }

            bool ok = docQuery.UpdateMetadata(docPk, setJson, removeKeys);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String(ok ? "ok" : "error");
            w.Key("doc_id"); w.String(docId.c_str());
            w.Key("fields_set"); w.Int(fieldsSet);
            w.Key("fields_removed"); w.Int(static_cast<int>(removeKeys.size()));
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- bonds (drill-down) ----
        if (strcmp(action, "bonds") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
            {
                return R"({"status":"error","message":"Missing 'doc_id' field"})";
            }

            AZStd::string docId(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());

            HCPDbConnection& db = m_engine->GetDbConnection();
            if (!db.IsConnected()) db.Connect();
            if (!db.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            int docPk = m_engine->GetDocumentQuery().GetDocPk(docId);
            if (docPk == 0)
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            AZStd::string tokenId;
            if (doc.HasMember("token") && doc["token"].IsString())
            {
                tokenId = AZStd::string(doc["token"].GetString(), doc["token"].GetStringLength());
            }

            auto bonds = m_engine->GetBondQuery().GetBondsForToken(docPk, tokenId);

            // Resolve surface forms via vocabulary lookup
            const auto& vocab = m_engine->GetVocabulary();

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("doc_id"); w.String(docId.c_str());

            if (!tokenId.empty())
            {
                w.Key("token"); w.String(tokenId.c_str());
                AZStd::string surface = vocab.TokenToWord(tokenId);
                if (!surface.empty())
                {
                    w.Key("surface"); w.String(surface.c_str());
                }
            }

            w.Key("bonds");
            w.StartArray();
            for (const auto& be : bonds)
            {
                w.StartObject();
                w.Key("token"); w.String(be.tokenB.c_str());
                // Try to resolve surface form
                AZStd::string bSurface = vocab.TokenToWord(be.tokenB);
                if (!bSurface.empty())
                {
                    w.Key("surface"); w.String(bSurface.c_str());
                }
                w.Key("count"); w.Int(be.count);
                w.EndObject();
            }
            w.EndArray();

            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- phys_resolve — DEPRECATED: use phys_ingest ----
        if (strcmp(action, "phys_resolve") == 0)
        {
            return R"({"status":"error","message":"phys_resolve is deprecated — use phys_ingest"})";
            // Dead code below retained for reference only
            AZStd::string text;
            AZ::u32 maxChars = 0;  // 0 = process entire text

            if (doc.HasMember("file") && doc["file"].IsString())
            {
                AZStd::string filePath(doc["file"].GetString(), doc["file"].GetStringLength());
                std::ifstream ifs(filePath.c_str());
                if (!ifs.is_open())
                {
                    return R"({"status":"error","message":"Could not open file"})";
                }
                std::string stdText((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
                ifs.close();
                text = AZStd::string(stdText.c_str(), stdText.size());
            }
            else if (doc.HasMember("text") && doc["text"].IsString())
            {
                text = AZStd::string(doc["text"].GetString(), doc["text"].GetStringLength());
            }
            else
            {
                return R"({"status":"error","message":"phys_resolve requires 'file' or 'text'"})";
            }

            if (doc.HasMember("max_chars") && doc["max_chars"].IsUint())
            {
                maxChars = doc["max_chars"].GetUint();
            }

            HCPParticlePipeline& pipeline = m_engine->GetParticlePipeline();
            if (!pipeline.IsInitialized())
            {
                return R"({"status":"error","message":"Particle pipeline not initialized"})";
            }

            // Phase 1: byte→char settlement
            SuperpositionTrialResult phase1 = RunSuperpositionTrial(
                pipeline.GetPhysics(),
                pipeline.GetScene(),
                pipeline.GetCuda(),
                text, m_engine->GetVocabulary(), maxChars);

            fprintf(stderr, "[phys_resolve] Phase 1: %u/%u settled (%.1f%%) in %.1f ms [%u bytes → %u codepoints]\n",
                phase1.settledCount, phase1.totalCodepoints,
                phase1.totalCodepoints > 0 ? 100.0f * phase1.settledCount / phase1.totalCodepoints : 0.0f,
                phase1.simulationTimeMs, phase1.totalBytes, phase1.totalCodepoints);
            fflush(stderr);

            // Extract character runs from Phase 1 output
            AZStd::vector<CharRun> runs = ExtractRunsFromCollapses(phase1);
            if (runs.empty())
            {
                return R"({"status":"error","message":"No runs extracted from Phase 1 output"})";
            }

            fprintf(stderr, "[phys_resolve] Extracted %zu runs from Phase 1 output (max %u bytes)\n",
                runs.size(), maxChars);
            fflush(stderr);

            // Use persistent BedManager (initialized at Activate)
            BedManager& bedManager = m_engine->GetBedManager();
            if (!bedManager.IsInitialized())
            {
                return R"({"status":"error","message":"BedManager not initialized"})";
            }

            ResolutionManifest manifest = bedManager.Resolve(runs);

            // ---- Optional benchmark TSV output ----
            bool writeBenchmark = doc.HasMember("benchmark") &&
                doc["benchmark"].IsBool() && doc["benchmark"].GetBool();
            if (writeBenchmark)
            {
                struct rusage usage;
                getrusage(RUSAGE_SELF, &usage);
                long rssKb = usage.ru_maxrss;

                long vramUsedMb = 0;
                FILE* nvsmi = popen("nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null", "r");
                if (nvsmi)
                {
                    char buf[64];
                    if (fgets(buf, sizeof(buf), nvsmi))
                        vramUsedMb = atol(buf);
                    pclose(nvsmi);
                }

                mkdir("benchmarks", 0755);

                auto now = std::chrono::system_clock::now();
                auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();

                char tsvPath[256];
                snprintf(tsvPath, sizeof(tsvPath), "benchmarks/resolve_%lld.tsv", (long long)epochMs);

                FILE* tsv = fopen(tsvPath, "w");
                if (tsv)
                {
                    fprintf(tsv, "timestamp\tbytes\tphase1_codepoints\tphase1_settled\t"
                        "phase1_pct\tphase1_ms\ttotal_runs\tresolved\tunresolved\t"
                        "resolved_pct\tresolve_ms\trss_kb\tvram_mb\n");
                    fprintf(tsv, "%lld\t%zu\t%u\t%u\t%.1f\t%.1f\t%u\t%u\t%u\t%.1f\t%.1f\t%ld\t%ld\n",
                        (long long)epochMs,
                        text.size(),
                        phase1.totalCodepoints,
                        phase1.settledCount,
                        phase1.totalCodepoints > 0 ? 100.0f * phase1.settledCount / phase1.totalCodepoints : 0.0f,
                        static_cast<double>(phase1.simulationTimeMs),
                        manifest.totalRuns,
                        manifest.resolvedRuns,
                        manifest.unresolvedRuns,
                        manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f,
                        static_cast<double>(manifest.totalTimeMs),
                        rssKb,
                        vramUsedMb);
                    fclose(tsv);
                    fprintf(stderr, "[phys_resolve] Benchmark written: %s\n", tsvPath);
                    fflush(stderr);
                }
            }

            // Build JSON response
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("phase1_settled"); w.Uint(phase1.settledCount);
            w.Key("phase1_total"); w.Uint(phase1.totalCodepoints);
            w.Key("phase1_total_bytes"); w.Uint(phase1.totalBytes);
            w.Key("phase1_time_ms"); w.Double(static_cast<double>(phase1.simulationTimeMs));
            w.Key("total_runs"); w.Uint(manifest.totalRuns);
            w.Key("resolved"); w.Uint(manifest.resolvedRuns);
            w.Key("unresolved"); w.Uint(manifest.unresolvedRuns);
            w.Key("time_ms"); w.Double(static_cast<double>(manifest.totalTimeMs));
            w.Key("bed_initialized"); w.Bool(m_engine->GetBedManager().IsInitialized());

            // System resource snapshot
            {
                struct rusage usage;
                getrusage(RUSAGE_SELF, &usage);
                w.Key("rss_kb"); w.Int64(usage.ru_maxrss);
            }

            w.Key("results");
            w.StartArray();
            for (const auto& r : manifest.results)
            {
                w.StartObject();
                w.Key("run"); w.String(r.runText.c_str());
                w.Key("resolved"); w.Bool(r.resolved);
                if (r.resolved)
                {
                    w.Key("word"); w.String(r.matchedWord.c_str());
                    w.Key("token_id"); w.String(r.matchedTokenId.c_str());
                    w.Key("tier"); w.Uint(r.tierResolved);
                    if (r.morphBits != 0)
                        { w.Key("morph_bits"); w.Uint(r.morphBits); }
                }
                if (r.firstCap)
                    { w.Key("first_cap"); w.Bool(true); }
                if (r.allCaps)
                    { w.Key("all_caps"); w.Bool(true); }
                w.EndObject();
            }
            w.EndArray();

            w.EndObject();

            fprintf(stderr, "[phys_resolve] Complete: %u/%u resolved (%.1f%%) in %.1f ms\n",
                manifest.resolvedRuns, manifest.totalRuns,
                manifest.totalRuns > 0 ? 100.0f * manifest.resolvedRuns / manifest.totalRuns : 0.0f,
                manifest.totalTimeMs);
            fflush(stderr);

            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- phys_ingest (Phase 1 + Phase 2 + scanner → PBM to Postgres) ----
        if (strcmp(action, "phys_ingest") == 0)
        {
            AZStd::string text;
            AZStd::string docName;
            AZStd::string centuryCode = "AB";
            bool fictionFirst = true;
            bool isWorkingDoc = false;

            if (doc.HasMember("corpus") && doc["corpus"].IsString())
            {
                AZStd::string corpus(doc["corpus"].GetString(), doc["corpus"].GetStringLength());
                if (corpus == "nonfiction" || corpus == "nf")
                    fictionFirst = false;
            }

            if (doc.HasMember("file") && doc["file"].IsString())
            {
                AZStd::string filePath(doc["file"].GetString(), doc["file"].GetStringLength());
                std::ifstream ifs(filePath.c_str());
                if (!ifs.is_open())
                    return R"({"status":"error","message":"Could not open file"})";
                std::string stdText((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
                ifs.close();
                text = AZStd::string(stdText.c_str(), stdText.size());

                size_t lastSlash = filePath.rfind('/');
                size_t lastDot   = filePath.rfind('.');
                if (lastSlash == AZStd::string::npos) lastSlash = 0; else ++lastSlash;
                if (lastDot == AZStd::string::npos || lastDot <= lastSlash) lastDot = filePath.size();
                docName = filePath.substr(lastSlash, lastDot - lastSlash);
                isWorkingDoc = (lastDot < filePath.size() && filePath.substr(lastDot) == ".json");
            }
            else if (doc.HasMember("text") && doc["text"].IsString())
            {
                text = AZStd::string(doc["text"].GetString(), doc["text"].GetStringLength());
            }
            else
            {
                return R"({"status":"error","message":"phys_ingest requires 'file' or 'text'"})";
            }

            if (doc.HasMember("name") && doc["name"].IsString())
                docName = AZStd::string(doc["name"].GetString(), doc["name"].GetStringLength());
            if (doc.HasMember("century") && doc["century"].IsString())
                centuryCode = AZStd::string(doc["century"].GetString(), doc["century"].GetStringLength());
            if (docName.empty()) docName = "untitled";

            AZStd::string ingestCatalog;
            AZStd::string ingestCatalogId;
            if (doc.HasMember("catalog") && doc["catalog"].IsString())
                ingestCatalog = AZStd::string(doc["catalog"].GetString(), doc["catalog"].GetStringLength());
            if (doc.HasMember("catalog_id") && doc["catalog_id"].IsString())
                ingestCatalogId = AZStd::string(doc["catalog_id"].GetString(), doc["catalog_id"].GetStringLength());

            PhysIngestResult pir = PhysIngestText(
                text, docName, centuryCode,
                ingestCatalog, ingestCatalogId,
                fictionFirst, isWorkingDoc, m_engine);

            if (!pir.errorMessage.empty())
                return R"({"status":"error","message":")" + pir.errorMessage + R"("})";

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("phase1_settled"); w.Uint(pir.phase1Settled);
            w.Key("phase1_total");   w.Uint(pir.phase1Total);
            w.Key("phase1_time_ms"); w.Double(static_cast<double>(pir.phase1TimeMs));
            w.Key("total_runs");     w.Uint(pir.totalRuns);
            w.Key("resolved");       w.Uint(pir.resolved);
            w.Key("unresolved");     w.Uint(pir.unresolved);
            if (pir.isWorkingDoc)
            {
                w.Key("mode");           w.String("working_doc");
                w.Key("working_doc_id"); w.Int(pir.workingDocId);
                w.Key("doc_name");       w.String(docName.c_str());
            }
            else
            {
                w.Key("doc_id");          w.String(pir.docId.c_str());
                w.Key("doc_pk");          w.Int(pir.docPk);
                w.Key("doc_name");        w.String(docName.c_str());
                if (pir.usedStub) { w.Key("stub_filled"); w.Bool(true); }
                w.Key("resolve_time_ms"); w.Double(static_cast<double>(pir.resolveTimeMs));
                w.Key("bond_types");      w.Uint64(pir.bondTypes);
                w.Key("total_pairs");     w.Uint64(pir.totalPairs);
                w.Key("unique_tokens");   w.Uint64(pir.uniqueTokens);
                w.Key("total_slots");     w.Int(pir.totalSlots);
                w.Key("positions_stored"); w.Bool(pir.positionsStored);
                w.Key("entity_annotations"); w.Uint64(pir.entityAnnotations);
            }
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- activate_envelope ----
        if (strcmp(action, "activate_envelope") == 0)
        {
            // Accept both "name" and "envelope" as the key for the envelope name.
            const char* nameKey = doc.HasMember("name") ? "name" : "envelope";
            if (!doc.HasMember(nameKey) || !doc[nameKey].IsString())
            {
                return R"({"status":"error","message":"Missing 'name' or 'envelope' field"})";
            }

            AZStd::string name(doc[nameKey].GetString(), doc[nameKey].GetStringLength());

            HCPEnvelopeManager& envMgr = m_engine->GetEnvelopeManager();
            EnvelopeActivation result = envMgr.ActivateEnvelope(name);
            int warmSize = envMgr.GetWorkingSetSize(result.envelopeId);
            m_engine->GetBedManager().InitEnvelopeWindow(result.envelopeId, warmSize);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("envelope"); w.String(name.c_str());
            w.Key("entries_loaded"); w.Int(result.entriesLoaded);
            w.Key("evicted_entries"); w.Int(result.evictedEntries);
            w.Key("load_time_ms"); w.Double(result.loadTimeMs);
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- deactivate_envelope ----
        if (strcmp(action, "deactivate_envelope") == 0)
        {
            if (!doc.HasMember("name") || !doc["name"].IsString())
            {
                return R"({"status":"error","message":"Missing 'name' field"})";
            }

            AZStd::string name(doc["name"].GetString(), doc["name"].GetStringLength());
            m_engine->GetEnvelopeManager().DeactivateEnvelope(name);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("envelope"); w.String(name.c_str());
            w.Key("deactivated"); w.Bool(true);
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- run_gutenberg ----
        if (strcmp(action, "run_gutenberg") == 0)
        {
            if (!doc.HasMember("working_doc_id") || !doc["working_doc_id"].IsInt())
                return R"({"status":"error","message":"Missing 'working_doc_id'"})";
            if (!doc.HasMember("texts_dir") || !doc["texts_dir"].IsString())
                return R"({"status":"error","message":"Missing 'texts_dir'"})";

            int workingDocId = doc["working_doc_id"].GetInt();
            AZStd::string textsDir(doc["texts_dir"].GetString(), doc["texts_dir"].GetStringLength());
            AZStd::string centuryCode = "AB";
            bool fictionFirst = true;

            if (doc.HasMember("century") && doc["century"].IsString())
                centuryCode = AZStd::string(doc["century"].GetString(), doc["century"].GetStringLength());
            if (doc.HasMember("corpus") && doc["corpus"].IsString())
            {
                AZStd::string corpus(doc["corpus"].GetString(), doc["corpus"].GetStringLength());
                if (corpus == "nonfiction" || corpus == "nf") fictionFirst = false;
            }

            GutenbergRunResult runResult = RunGutenbergTexts(
                workingDocId, textsDir, centuryCode, fictionFirst,
                m_engine, m_engine->GetVarConn());

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status");    w.String(runResult.failed > 0 && runResult.succeeded == 0 ? "error" : "ok");
            w.Key("processed"); w.Int(runResult.processed);
            w.Key("succeeded"); w.Int(runResult.succeeded);
            w.Key("failed");    w.Int(runResult.failed);
            w.Key("skipped");   w.Int(runResult.skipped);
            if (!runResult.errors.empty())
            {
                w.Key("errors");
                w.StartArray();
                for (const auto& e : runResult.errors) w.String(e.c_str());
                w.EndArray();
            }
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        return R"({"status":"error","message":"Unknown action"})";
    }

} // namespace HCPEngine
