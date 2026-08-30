#include "HCPDbConnection.h"

#include <AzCore/Console/ILogger.h>
#include <libpq-fe.h>

namespace HCPEngine
{
namespace DbConn_Detail {
    constexpr const char* DEFAULT_CONNINFO =
        "dbname=hcp_fic_pbm user=hcp password=hcp_dev host=192.168.68.60 port=5435";
} // DbConn_Detail

    HCPDbConnection::~HCPDbConnection()
    {
        Disconnect();
    }

    bool HCPDbConnection::Connect(const char* connInfo)
    {
        if (m_conn)
        {
            Disconnect();
        }

        m_conn = PQconnectdb(connInfo ? connInfo : DbConn_Detail::DEFAULT_CONNINFO);
        if (PQstatus(m_conn) != CONNECTION_OK)
        {
            AZLOG_ERROR("HCPDbConnection: Connection failed: %s", PQerrorMessage(m_conn));
            PQfinish(m_conn);
            m_conn = nullptr;
            return false;
        }

        AZLOG_INFO("HCPDbConnection: Connected to %s", connInfo ? connInfo : DbConn_Detail::DEFAULT_CONNINFO);
        return true;
    }

    void HCPDbConnection::Disconnect()
    {
        if (m_conn)
        {
            PQfinish(m_conn);
            m_conn = nullptr;
        }
    }

    bool HCPDbConnection::IsConnected() const
    {
        return m_conn != nullptr && PQstatus(m_conn) == CONNECTION_OK;
    }

} // namespace HCPEngine
