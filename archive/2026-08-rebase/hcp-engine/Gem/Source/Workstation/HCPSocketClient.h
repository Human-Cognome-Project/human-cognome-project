#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>
#include <QByteArray>
#include <functional>
#include <deque>
#endif

namespace HCPEngine
{
    /// Async socket client for the HCP engine daemon.
    /// Length-prefixed JSON protocol: 4-byte big-endian length + UTF-8 JSON payload.
    /// All responses arrive on the Qt event loop (UI thread) — callbacks are UI-safe.
    class HCPSocketClient : public QObject
    {
        Q_OBJECT

    public:
        using ResponseCallback = std::function<void(const QJsonObject&)>;

        explicit HCPSocketClient(const QString& host = "127.0.0.1",
                                  quint16 port = 9720,
                                  QObject* parent = nullptr);
        ~HCPSocketClient() override;

        void ConnectToEngine();
        void Disconnect();
        bool IsConnected() const;

        // ---- Typed convenience methods (one per socket action) ----

        void Health(ResponseCallback cb);
        void ListDocuments(ResponseCallback cb);
        void GetDocumentInfo(const QString& docId, ResponseCallback cb);
        void GetBonds(const QString& docId, const QString& tokenId, ResponseCallback cb);
        void RetrieveText(const QString& docId, ResponseCallback cb);
        void UpdateMeta(const QString& docId, const QJsonObject& setFields,
                        const QStringList& removeKeys, ResponseCallback cb);
        void Ingest(const QString& text, const QString& name,
                    const QString& metadata, ResponseCallback cb);
        void IngestFile(const QString& path, const QString& name,
                        const QString& metadata, ResponseCallback cb);
        void PhysResolve(const QString& text, int maxChars, ResponseCallback cb);

    signals:
        void connected();
        void disconnected();
        void connectionFailed(const QString& reason);

    private slots:
        void OnConnected();
        void OnDisconnected();
        void OnReadyRead();
        void OnSocketError(QAbstractSocket::SocketError error);
        void TryReconnect();

    private:
        void SendRequest(const QJsonObject& request, ResponseCallback cb);
        void ProcessRecvBuffer();

        QTcpSocket* m_socket = nullptr;
        QString m_host;
        quint16 m_port;

        // Partial read accumulation
        QByteArray m_recvBuffer;

        // FIFO callback queue — server processes one request at a time
        std::deque<ResponseCallback> m_pendingCallbacks;

        // Auto-reconnect
        QTimer* m_reconnectTimer = nullptr;
        int m_reconnectAttempts = 0;
        static constexpr int MaxReconnectDelay = 10000; // 10s max backoff
    };

} // namespace HCPEngine
