#include "HCPSocketServer.h"
#include "HCPEngineSystemComponent.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"
#include "HCPParticlePipeline.h"
#include "HCPStorage.h"
#include "HCPJsonInterpreter.h"
#include "HCPResolutionChamber.h"
#include "HCPVocabBed.h"
#include "HCPSuperpositionTrial.h"
#include "HCPWordSuperpositionTrial.h"
#include "HCPBondCompiler.h"

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
                if (m_stopRequested.load()) break;
                AZLOG_ERROR("HCPSocketServer: accept() failed: %s", strerror(errno));
                continue;
            }

            fprintf(stderr, "[HCPSocketServer] Client connected\n");
            fflush(stderr);

            HandleClient(clientFd);

            ::close(clientFd);
            fprintf(stderr, "[HCPSocketServer] Client disconnected\n");
            fflush(stderr);
        }

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

        // ---- ingest ----
        // Two modes:
        //   1. File path:  {"action":"ingest", "file":"/path/to/text.txt", ...}
        //   2. Inline text: {"action":"ingest", "text":"...", "name":"...", ...}
        // Optional: "metadata" (JSON string), "catalog" (e.g. "gutenberg"), "century"
        if (strcmp(action, "ingest") == 0)
        {
            AZStd::string text;
            AZStd::string name;

            if (doc.HasMember("file") && doc["file"].IsString())
            {
                // File path mode — read from disk, derive name from filename
                AZStd::string filePath(doc["file"].GetString(), doc["file"].GetStringLength());
                std::ifstream ifs(filePath.c_str());
                if (!ifs.is_open())
                {
                    rapidjson::StringBuffer sb;
                    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
                    w.StartObject();
                    w.Key("status"); w.String("error");
                    w.Key("message"); w.String("Could not open file");
                    w.Key("file"); w.String(filePath.c_str());
                    w.EndObject();
                    return AZStd::string(sb.GetString(), sb.GetSize());
                }
                std::string stdText((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
                ifs.close();
                text = AZStd::string(stdText.c_str(), stdText.size());

                // Derive name from filename (strip path and extension)
                name = filePath;
                size_t lastSlash = name.rfind('/');
                if (lastSlash != AZStd::string::npos) name = name.substr(lastSlash + 1);
                size_t lastDot = name.rfind('.');
                if (lastDot != AZStd::string::npos) name = name.substr(0, lastDot);

                // Override name if explicitly provided
                if (doc.HasMember("name") && doc["name"].IsString())
                {
                    name = AZStd::string(doc["name"].GetString(), doc["name"].GetStringLength());
                }
            }
            else if (doc.HasMember("text") && doc["text"].IsString())
            {
                // Inline text mode — text and name required
                if (!doc.HasMember("name") || !doc["name"].IsString())
                {
                    return R"({"status":"error","message":"Inline ingest requires 'name' field"})";
                }
                text = AZStd::string(doc["text"].GetString(), doc["text"].GetStringLength());
                name = AZStd::string(doc["name"].GetString(), doc["name"].GetStringLength());
            }
            else
            {
                return R"({"status":"error","message":"Ingest requires 'file' or 'text' field"})";
            }

            const char* century = "AS";
            if (doc.HasMember("century") && doc["century"].IsString())
            {
                century = doc["century"].GetString();
            }
            AZStd::string centuryCode(century);

            auto t0 = std::chrono::high_resolution_clock::now();

            // Tokenize
            TokenStream stream = Tokenize(text, m_engine->GetVocabulary());
            if (stream.tokenIds.empty())
            {
                return R"({"status":"error","message":"Tokenization produced no tokens"})";
            }

            // Derive PBM bonds
            PBMData pbmData = DerivePBM(stream);

            // Store PBM via write kernel
            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected())
            {
                wk.Connect();
            }

            AZStd::string docId;
            if (wk.IsConnected())
            {
                docId = wk.StorePBM(name, centuryCode, pbmData);

                // Store positions alongside bonds for exact reconstruction
                if (!docId.empty())
                {
                    wk.StorePositions(
                        wk.LastDocPk(),
                        stream.tokenIds,
                        stream.positions,
                        stream.totalSlots);
                }
            }

            // Process metadata if provided
            int metaKnown = 0, metaUnreviewed = 0;
            bool metaProvenance = false;
            if (!docId.empty() && wk.IsConnected() &&
                doc.HasMember("metadata") && doc["metadata"].IsString())
            {
                AZStd::string metaJson(doc["metadata"].GetString(),
                                        doc["metadata"].GetStringLength());
                AZStd::string catalog = "unknown";
                if (doc.HasMember("catalog") && doc["catalog"].IsString())
                {
                    catalog = AZStd::string(doc["catalog"].GetString(),
                                             doc["catalog"].GetStringLength());
                }

                JsonInterpretResult jResult = ProcessJsonMetadata(
                    metaJson, wk.LastDocPk(), catalog, wk, m_engine->GetVocabulary());

                metaKnown = jResult.knownFields;
                metaUnreviewed = jResult.unreviewedFields;
                metaProvenance = jResult.provenanceStored;
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            fprintf(stderr, "[HCPSocketServer] Ingested '%s': %zu tokens, %zu bonds, %.1f ms%s\n",
                name.c_str(), stream.tokenIds.size(),
                pbmData.bonds.size(), ms,
                docId.empty() ? " (DB unavailable)" : "");
            fflush(stderr);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("doc_id"); w.String(docId.c_str());
            w.Key("name"); w.String(name.c_str());
            w.Key("tokens"); w.Uint64(stream.tokenIds.size());
            w.Key("unique"); w.Uint64(pbmData.uniqueTokens);
            w.Key("bonds"); w.Uint64(pbmData.bonds.size());
            w.Key("total_pairs"); w.Uint64(pbmData.totalPairs);
            if (metaKnown > 0 || metaUnreviewed > 0)
            {
                w.Key("meta_known"); w.Int(metaKnown);
                w.Key("meta_unreviewed"); w.Int(metaUnreviewed);
                w.Key("meta_provenance"); w.Bool(metaProvenance);
            }
            w.Key("ms"); w.Double(ms);
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
            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected())
            {
                wk.Connect();
            }
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            AZStd::vector<AZStd::string> tokenIds = wk.LoadPositions(docId);
            if (tokenIds.empty())
            {
                return R"({"status":"error","message":"Document not found or has no positions"})";
            }

            auto tLoad = std::chrono::high_resolution_clock::now();

            // Convert token IDs to text with stickiness rules
            AZStd::string text = TokenIdsToText(tokenIds, m_engine->GetVocabulary());

            auto t1 = std::chrono::high_resolution_clock::now();
            double loadMs = std::chrono::duration<double, std::milli>(tLoad - t0).count();
            double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

            fprintf(stderr, "[HCPSocketServer] Retrieved '%s': %zu tokens -> %zu chars, %.1f ms\n",
                docId.c_str(), tokenIds.size(), text.size(), totalMs);
            fflush(stderr);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("text"); w.String(text.c_str(), static_cast<rapidjson::SizeType>(text.size()));
            w.Key("tokens"); w.Uint64(tokenIds.size());
            w.Key("load_ms"); w.Double(loadMs);
            w.Key("ms"); w.Double(totalMs);
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        // ---- list ----
        if (strcmp(action, "list") == 0)
        {
            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected())
            {
                wk.Connect();
            }
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            auto docs = wk.ListDocuments();

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

        // ---- tokenize (no DB, just analysis) ----
        if (strcmp(action, "tokenize") == 0)
        {
            if (!doc.HasMember("text") || !doc["text"].IsString())
            {
                return R"({"status":"error","message":"Missing 'text' field"})";
            }

            AZStd::string text(doc["text"].GetString(), doc["text"].GetStringLength());

            auto t0 = std::chrono::high_resolution_clock::now();
            TokenStream stream = Tokenize(text, m_engine->GetVocabulary());
            PBMData pbmData = DerivePBM(stream);
            auto t1 = std::chrono::high_resolution_clock::now();

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("tokens"); w.Uint64(stream.tokenIds.size());
            w.Key("unique"); w.Uint64(pbmData.uniqueTokens);
            w.Key("bonds"); w.Uint64(pbmData.bonds.size());
            w.Key("total_pairs"); w.Uint64(pbmData.totalPairs);
            w.Key("original_bytes"); w.Uint64(text.size());
            w.Key("ms"); w.Double(std::chrono::duration<double, std::milli>(t1 - t0).count());
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

            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected()) wk.Connect();
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            auto detail = wk.GetDocumentDetail(docId);
            if (detail.pk == 0)
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            auto prov = wk.GetProvenance(detail.pk);
            auto vars = wk.GetDocVars(detail.pk);

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

        // ---- update_meta ----
        if (strcmp(action, "update_meta") == 0)
        {
            if (!doc.HasMember("doc_id") || !doc["doc_id"].IsString())
            {
                return R"({"status":"error","message":"Missing 'doc_id' field"})";
            }

            AZStd::string docId(doc["doc_id"].GetString(), doc["doc_id"].GetStringLength());

            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected()) wk.Connect();
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            int docPk = wk.GetDocPk(docId);
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

            bool ok = wk.UpdateMetadata(docPk, setJson, removeKeys);

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

            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected()) wk.Connect();
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            int docPk = wk.GetDocPk(docId);
            if (docPk == 0)
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            AZStd::string tokenId;
            if (doc.HasMember("token") && doc["token"].IsString())
            {
                tokenId = AZStd::string(doc["token"].GetString(), doc["token"].GetStringLength());
            }

            auto bonds = wk.GetBondsForToken(docPk, tokenId);

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

        // ---- phys_resolve (Phase 2: char→word resolution chambers) ----
        if (strcmp(action, "phys_resolve") == 0)
        {
            AZStd::string text;
            AZ::u32 maxChars = 200;

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

            const HCPBondTable& charWordBonds = m_engine->GetCharWordBonds();
            if (charWordBonds.PairCount() == 0)
            {
                return R"({"status":"error","message":"No char->word bond table loaded"})";
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
            w.Key("buckets"); w.Uint64(m_engine->GetTierAssembly().BucketCount());
            w.Key("vocab_words"); w.Uint64(m_engine->GetTierAssembly().TotalWords());

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
                }
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

        return R"({"status":"error","message":"Unknown action"})";
    }

} // namespace HCPEngine
