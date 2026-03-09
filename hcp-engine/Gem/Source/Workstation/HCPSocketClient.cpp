#include "HCPSocketClient.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QtEndian>

#include <cstdio>

namespace HCPEngine
{
    HCPSocketClient::HCPSocketClient(const QString& host, quint16 port, QObject* parent)
        : QObject(parent)
        , m_host(host)
        , m_port(port)
    {
        m_socket = new QTcpSocket(this);

        connect(m_socket, &QTcpSocket::connected,
            this, &HCPSocketClient::OnConnected);
        connect(m_socket, &QTcpSocket::disconnected,
            this, &HCPSocketClient::OnDisconnected);
        connect(m_socket, &QTcpSocket::readyRead,
            this, &HCPSocketClient::OnReadyRead);
        connect(m_socket, &QTcpSocket::errorOccurred,
            this, &HCPSocketClient::OnSocketError);

        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout,
            this, &HCPSocketClient::TryReconnect);
    }

    HCPSocketClient::~HCPSocketClient()
    {
        m_reconnectTimer->stop();
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
            m_socket->abort();
    }

    void HCPSocketClient::ConnectToEngine()
    {
        m_reconnectAttempts = 0;
        m_recvBuffer.clear();
        m_pendingCallbacks.clear();
        m_socket->connectToHost(m_host, m_port);
    }

    void HCPSocketClient::Disconnect()
    {
        m_reconnectTimer->stop();
        m_socket->disconnectFromHost();
    }

    bool HCPSocketClient::IsConnected() const
    {
        return m_socket->state() == QAbstractSocket::ConnectedState;
    }

    // ---- Signal handlers ----

    void HCPSocketClient::OnConnected()
    {
        m_reconnectAttempts = 0;
        fprintf(stderr, "[HCP Client] Connected to %s:%d\n",
            m_host.toUtf8().constData(), m_port);
        fflush(stderr);
        emit connected();
    }

    void HCPSocketClient::OnDisconnected()
    {
        fprintf(stderr, "[HCP Client] Disconnected\n");
        fflush(stderr);

        // Fail all pending callbacks
        while (!m_pendingCallbacks.empty())
        {
            auto cb = std::move(m_pendingCallbacks.front());
            m_pendingCallbacks.pop_front();
            if (cb)
            {
                QJsonObject err;
                err["status"] = "error";
                err["message"] = "Connection lost";
                cb(err);
            }
        }
        m_recvBuffer.clear();

        emit disconnected();

        // Start auto-reconnect
        int delay = qMin(1000 * (1 << m_reconnectAttempts), MaxReconnectDelay);
        m_reconnectTimer->start(delay);
    }

    void HCPSocketClient::OnSocketError(QAbstractSocket::SocketError error)
    {
        if (error == QAbstractSocket::ConnectionRefusedError ||
            error == QAbstractSocket::HostNotFoundError ||
            error == QAbstractSocket::NetworkError)
        {
            QString reason = m_socket->errorString();
            fprintf(stderr, "[HCP Client] Connection error: %s\n",
                reason.toUtf8().constData());
            fflush(stderr);
            emit connectionFailed(reason);

            // Schedule reconnect if not already connected
            if (m_socket->state() == QAbstractSocket::UnconnectedState)
            {
                int delay = qMin(1000 * (1 << m_reconnectAttempts), MaxReconnectDelay);
                m_reconnectTimer->start(delay);
            }
        }
    }

    void HCPSocketClient::TryReconnect()
    {
        ++m_reconnectAttempts;
        fprintf(stderr, "[HCP Client] Reconnect attempt %d...\n", m_reconnectAttempts);
        fflush(stderr);
        m_socket->connectToHost(m_host, m_port);
    }

    // ---- Protocol: length-prefixed JSON ----

    void HCPSocketClient::SendRequest(const QJsonObject& request, ResponseCallback cb)
    {
        if (!IsConnected())
        {
            if (cb)
            {
                QJsonObject err;
                err["status"] = "error";
                err["message"] = "Not connected to engine";
                cb(err);
            }
            return;
        }

        QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact);

        // 4-byte big-endian length prefix
        quint32 len = static_cast<quint32>(payload.size());
        quint32 netLen = qToBigEndian(len);

        m_socket->write(reinterpret_cast<const char*>(&netLen), 4);
        m_socket->write(payload);
        m_socket->flush();

        m_pendingCallbacks.push_back(std::move(cb));
    }

    void HCPSocketClient::OnReadyRead()
    {
        m_recvBuffer.append(m_socket->readAll());
        ProcessRecvBuffer();
    }

    void HCPSocketClient::ProcessRecvBuffer()
    {
        while (true)
        {
            // Need at least 4 bytes for length prefix
            if (m_recvBuffer.size() < 4)
                return;

            quint32 netLen;
            memcpy(&netLen, m_recvBuffer.constData(), 4);
            quint32 payloadLen = qFromBigEndian(netLen);

            // Sanity check — 64MB max (matches server)
            if (payloadLen > 64 * 1024 * 1024)
            {
                fprintf(stderr, "[HCP Client] Protocol error: payload too large (%u bytes)\n",
                    payloadLen);
                fflush(stderr);
                m_socket->abort();
                return;
            }

            // Wait for full payload
            if (static_cast<quint32>(m_recvBuffer.size()) < 4 + payloadLen)
                return;

            // Extract payload
            QByteArray payload = m_recvBuffer.mid(4, payloadLen);
            m_recvBuffer.remove(0, 4 + payloadLen);

            // Parse JSON
            QJsonParseError parseErr;
            QJsonDocument jdoc = QJsonDocument::fromJson(payload, &parseErr);

            QJsonObject response;
            if (jdoc.isObject())
            {
                response = jdoc.object();
            }
            else
            {
                response["status"] = "error";
                response["message"] = QString("JSON parse error: %1").arg(parseErr.errorString());
            }

            // Dispatch to first pending callback
            if (!m_pendingCallbacks.empty())
            {
                auto cb = std::move(m_pendingCallbacks.front());
                m_pendingCallbacks.pop_front();
                if (cb)
                    cb(response);
            }
            else
            {
                fprintf(stderr, "[HCP Client] Warning: received response with no pending callback\n");
                fflush(stderr);
            }
        }
    }

    // ---- Typed convenience methods ----

    void HCPSocketClient::Health(ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "health";
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::ListDocuments(ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "list";
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::GetDocumentInfo(const QString& docId, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "info";
        req["doc_id"] = docId;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::GetBonds(const QString& docId, const QString& tokenId,
                                    ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "bonds";
        req["doc_id"] = docId;
        if (!tokenId.isEmpty())
            req["token"] = tokenId;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::RetrieveText(const QString& docId, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "retrieve";
        req["doc_id"] = docId;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::UpdateMeta(const QString& docId, const QJsonObject& setFields,
                                      const QStringList& removeKeys, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "update_meta";
        req["doc_id"] = docId;
        if (!setFields.isEmpty())
            req["set"] = setFields;
        if (!removeKeys.isEmpty())
        {
            QJsonArray arr;
            for (const auto& k : removeKeys)
                arr.append(k);
            req["remove"] = arr;
        }
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::Ingest(const QString& text, const QString& name,
                                  const QString& metadata, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "ingest";
        req["text"] = text;
        req["name"] = name;
        if (!metadata.isEmpty())
            req["metadata"] = metadata;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::IngestFile(const QString& path, const QString& name,
                                      const QString& metadata, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "ingest";
        req["file"] = path;
        req["name"] = name;
        if (!metadata.isEmpty())
            req["metadata"] = metadata;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::Tokenize(const QString& text, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "tokenize";
        req["text"] = text;
        SendRequest(req, std::move(cb));
    }

    void HCPSocketClient::PhysResolve(const QString& text, int maxChars, ResponseCallback cb)
    {
        QJsonObject req;
        req["action"] = "phys_resolve";
        req["text"] = text;
        if (maxChars > 0)
            req["max_chars"] = maxChars;
        SendRequest(req, std::move(cb));
    }

} // namespace HCPEngine
