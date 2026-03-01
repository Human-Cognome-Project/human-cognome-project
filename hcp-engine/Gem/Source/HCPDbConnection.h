#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

struct pg_conn;
typedef pg_conn PGconn;

namespace HCPEngine
{
    //! Thin PGconn* wrapper — shared by all DB kernels.
    //! One connection per database. Kernels take a reference to this.
    class HCPDbConnection
    {
    public:
        HCPDbConnection() = default;
        ~HCPDbConnection();

        HCPDbConnection(const HCPDbConnection&) = delete;
        HCPDbConnection& operator=(const HCPDbConnection&) = delete;

        //! Connect to a Postgres database. Disconnects existing connection first.
        //! @param connInfo libpq connection string. If nullptr, uses default (hcp_fic_pbm).
        bool Connect(const char* connInfo = nullptr);

        //! Disconnect and release the connection.
        void Disconnect();

        //! True if connected and connection is alive.
        bool IsConnected() const;

        //! Raw PGconn* for query execution. All kernels use this.
        PGconn* Get() const { return m_conn; }

    private:
        PGconn* m_conn = nullptr;
    };

} // namespace HCPEngine
