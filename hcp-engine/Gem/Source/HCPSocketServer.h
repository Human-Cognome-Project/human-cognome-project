#pragma once

#include <AzCore/std/string/string.h>
#include <thread>
#include <atomic>

namespace HCPEngine
{
    class HCPEngineSystemComponent;

    //! TCP socket server for the HCP engine API.
    //! Listens on a port and accepts JSON commands:
    //!
    //! Protocol: length-prefixed JSON messages.
    //!   - 4 bytes: message length (network byte order, big-endian)
    //!   - N bytes: JSON payload (UTF-8)
    //!
    //! Actions:
    //!   {"action": "health"}
    //!     → {"status": "ok", "words": N, "labels": N, "chars": N}
    //!
    //!   {"action": "ingest", "name": "Doc Name", "century": "AS", "text": "full text..."}
    //!     → {"status": "ok", "doc_id": "vA.AB.AS.AA.AA", "tokens": N, "slots": N, "unique": N}
    //!
    //!   {"action": "retrieve", "doc_id": "vA.AB.AS.AA.AA"}
    //!     → {"status": "ok", "text": "reassembled text...", "tokens": N, "slots": N}
    //!
    //!   {"action": "stats", "doc_id": "vA.AB.AS.AA.AA"}
    //!     → {"status": "ok", "tokens": N, "slots": N, "unique": N, "db_bytes": N}
    //!
    //!   On error: {"status": "error", "message": "description"}
    //!
    class HCPSocketServer
    {
    public:
        static constexpr int DEFAULT_PORT = 9720;

        HCPSocketServer() = default;
        ~HCPSocketServer();

        //! Start listening on the given port. Non-blocking — spawns a thread.
        bool Start(HCPEngineSystemComponent* engine, int port = DEFAULT_PORT);

        //! Stop the server and join the listener thread.
        void Stop();

        bool IsRunning() const { return m_running.load(); }

    private:
        void ListenerThread(int port);
        void HandleClient(int clientFd);
        AZStd::string ProcessRequest(const AZStd::string& json);

        HCPEngineSystemComponent* m_engine = nullptr;
        std::thread m_thread;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_stopRequested{false};
        int m_listenFd = -1;
    };

} // namespace HCPEngine
