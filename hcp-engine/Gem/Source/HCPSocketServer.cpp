#include "HCPSocketServer.h"
#include "HCPEngineSystemComponent.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"
#include "HCPParticlePipeline.h"
#include "HCPStorage.h"
#include "HCPJsonInterpreter.h"

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

        return R"({"status":"error","message":"Unknown action"})";
    }

} // namespace HCPEngine
