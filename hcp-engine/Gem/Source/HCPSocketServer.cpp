#include "HCPSocketServer.h"
#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPStorage.h"

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

    bool HCPSocketServer::Start(HCPEngineSystemComponent* engine, int port)
    {
        if (m_running.load()) return true;

        m_engine = engine;
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
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // localhost only
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
        fprintf(stderr, "[HCPSocketServer] Listening on 127.0.0.1:%d\n", port);
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
        if (strcmp(action, "ingest") == 0)
        {
            if (!doc.HasMember("text") || !doc["text"].IsString())
            {
                return R"({"status":"error","message":"Missing 'text' field"})";
            }
            if (!doc.HasMember("name") || !doc["name"].IsString())
            {
                return R"({"status":"error","message":"Missing 'name' field"})";
            }

            const char* century = "AS";  // default
            if (doc.HasMember("century") && doc["century"].IsString())
            {
                century = doc["century"].GetString();
            }

            AZStd::string text(doc["text"].GetString(), doc["text"].GetStringLength());
            AZStd::string name(doc["name"].GetString(), doc["name"].GetStringLength());
            AZStd::string centuryCode(century);

            auto t0 = std::chrono::high_resolution_clock::now();

            // Tokenize
            TokenStream stream = Tokenize(text, m_engine->GetVocabulary());
            if (stream.tokenIds.empty())
            {
                return R"({"status":"error","message":"Tokenization produced no tokens"})";
            }

            // Position-based disassembly
            PositionMap posMap = DisassemblePositions(stream);

            // Derive PBM (for stats)
            PBMData pbmData = DerivePBM(stream);

            // Store via write kernel
            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected())
            {
                wk.Connect();
            }

            AZStd::string docId;
            if (wk.IsConnected())
            {
                docId = wk.StorePositionMap(name, centuryCode, posMap, stream);
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            // Estimate DB storage size
            size_t dbBytes = 0;
            for (const auto& entry : posMap.entries)
            {
                dbBytes += 6 + entry.positions.size() * 4;
            }

            fprintf(stderr, "[HCPSocketServer] Ingested '%s': %zu tokens, %u slots, %.1f ms%s\n",
                name.c_str(), stream.tokenIds.size(), stream.totalSlots, ms,
                docId.empty() ? " (DB unavailable)" : "");
            fflush(stderr);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("doc_id"); w.String(docId.c_str());
            w.Key("tokens"); w.Uint64(stream.tokenIds.size());
            w.Key("slots"); w.Uint(stream.totalSlots);
            w.Key("unique"); w.Uint64(posMap.uniqueTokens);
            w.Key("bonds"); w.Uint64(pbmData.bonds.size());
            w.Key("db_bytes"); w.Uint64(dbBytes);
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

            // Load from DB
            HCPWriteKernel& wk = m_engine->GetWriteKernel();
            if (!wk.IsConnected())
            {
                wk.Connect();
            }
            if (!wk.IsConnected())
            {
                return R"({"status":"error","message":"Database not available"})";
            }

            TokenStream loadedStream;
            PositionMap posMap = wk.LoadPositionMap(docId, loadedStream);
            if (posMap.entries.empty())
            {
                return R"({"status":"error","message":"Document not found"})";
            }

            // Reassemble positions → token stream
            TokenStream stream = ReassemblePositions(posMap);

            // Convert to text (gaps = spaces)
            AZStd::string text;
            text.reserve(stream.totalSlots);
            AZ::u32 cursor = 0;
            for (size_t i = 0; i < stream.tokenIds.size(); ++i)
            {
                AZ::u32 pos = stream.positions[i];
                const AZStd::string& tid = stream.tokenIds[i];

                while (cursor < pos)
                {
                    text += ' ';
                    ++cursor;
                }

                if (tid.starts_with("AB.AB."))
                {
                    AZStd::string word = m_engine->GetVocabulary().TokenToWord(tid);
                    if (!word.empty()) { text += word; ++cursor; continue; }
                }

                char c = m_engine->GetVocabulary().TokenToChar(tid);
                if (c != '\0') { text += c; ++cursor; continue; }

                AZStd::string nlLabel = m_engine->GetVocabulary().LookupLabel("newline");
                if (!nlLabel.empty() && tid == nlLabel)
                {
                    text += '\n';
                    ++cursor;
                    continue;
                }

                ++cursor;
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            fprintf(stderr, "[HCPSocketServer] Retrieved '%s': %zu tokens -> %zu chars, %.1f ms\n",
                docId.c_str(), stream.tokenIds.size(), text.size(), ms);
            fflush(stderr);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("text"); w.String(text.c_str(), static_cast<rapidjson::SizeType>(text.size()));
            w.Key("tokens"); w.Uint64(stream.tokenIds.size());
            w.Key("slots"); w.Uint(stream.totalSlots);
            w.Key("ms"); w.Double(ms);
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
            PositionMap posMap = DisassemblePositions(stream);
            PBMData pbmData = DerivePBM(stream);
            auto t1 = std::chrono::high_resolution_clock::now();

            size_t dbBytes = 0;
            for (const auto& entry : posMap.entries)
            {
                dbBytes += 6 + entry.positions.size() * 4;
            }

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("status"); w.String("ok");
            w.Key("tokens"); w.Uint64(stream.tokenIds.size());
            w.Key("slots"); w.Uint(stream.totalSlots);
            w.Key("unique"); w.Uint64(posMap.uniqueTokens);
            w.Key("bonds"); w.Uint64(pbmData.bonds.size());
            w.Key("db_bytes"); w.Uint64(dbBytes);
            w.Key("original_bytes"); w.Uint64(text.size());
            w.Key("ratio"); w.Double(text.size() > 0 ? static_cast<double>(dbBytes) / text.size() : 0.0);
            w.Key("ms"); w.Double(std::chrono::duration<double, std::milli>(t1 - t0).count());
            w.EndObject();
            return AZStd::string(sb.GetString(), sb.GetSize());
        }

        return R"({"status":"error","message":"Unknown action"})";
    }

} // namespace HCPEngine
