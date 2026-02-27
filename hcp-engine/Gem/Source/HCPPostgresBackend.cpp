#include "HCPDatabaseBackend.h"

#include <libpq-fe.h>
#include <cstdio>
#include <cstring>

namespace HCPEngine
{
    class PostgresBackend : public IDatabaseBackend
    {
    public:
        PostgresBackend() = default;

        ~PostgresBackend() override
        {
            Disconnect();
        }

        bool Connect(const char* connectionString) override
        {
            if (m_conn)
                Disconnect();

            const char* connStr = connectionString;
            if (!connStr || !connStr[0])
                connStr = "host=localhost dbname=hcp_fic_pbm user=hcp password=hcp_dev";

            m_conn = PQconnectdb(connStr);
            if (PQstatus(m_conn) != CONNECTION_OK)
            {
                fprintf(stderr, "[PostgresBackend] Connect failed: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
                PQfinish(m_conn);
                m_conn = nullptr;
                return false;
            }

            return true;
        }

        void Disconnect() override
        {
            if (m_conn)
            {
                PQfinish(m_conn);
                m_conn = nullptr;
            }
        }

        bool IsConnected() const override
        {
            return m_conn != nullptr && PQstatus(m_conn) == CONNECTION_OK;
        }

        QueryResult Query(const char* sql) override
        {
            QueryResult qr;
            if (!m_conn) return qr;

            PGresult* res = PQexec(m_conn, sql);
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                fprintf(stderr, "[PostgresBackend] Query error: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
                PQclear(res);
                return qr;
            }

            qr.success = true;
            qr.rowCount = PQntuples(res);
            qr.colCount = PQnfields(res);
            qr.rows.reserve(qr.rowCount);

            for (int r = 0; r < qr.rowCount; ++r)
            {
                AZStd::vector<AZStd::string> row;
                row.reserve(qr.colCount);
                for (int c = 0; c < qr.colCount; ++c)
                {
                    if (PQgetisnull(res, r, c))
                        row.push_back({});
                    else
                        row.push_back(PQgetvalue(res, r, c));
                }
                qr.rows.push_back(AZStd::move(row));
            }

            PQclear(res);
            return qr;
        }

        QueryResult QueryParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) override
        {
            QueryResult qr;
            if (!m_conn) return qr;

            // Build param arrays for PQexecParams
            AZStd::vector<const char*> paramValues(params.size());
            AZStd::vector<int> paramLengths(params.size());
            AZStd::vector<int> paramFormats(params.size(), 0); // all text

            for (size_t i = 0; i < params.size(); ++i)
            {
                paramValues[i] = params[i].c_str();
                paramLengths[i] = static_cast<int>(params[i].size());
            }

            PGresult* res = PQexecParams(m_conn, sql,
                static_cast<int>(params.size()),
                nullptr,
                paramValues.data(),
                paramLengths.data(),
                paramFormats.data(),
                0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                fprintf(stderr, "[PostgresBackend] QueryParams error: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
                PQclear(res);
                return qr;
            }

            qr.success = true;
            qr.rowCount = PQntuples(res);
            qr.colCount = PQnfields(res);
            qr.rows.reserve(qr.rowCount);

            for (int r = 0; r < qr.rowCount; ++r)
            {
                AZStd::vector<AZStd::string> row;
                row.reserve(qr.colCount);
                for (int c = 0; c < qr.colCount; ++c)
                {
                    if (PQgetisnull(res, r, c))
                        row.push_back({});
                    else
                        row.push_back(PQgetvalue(res, r, c));
                }
                qr.rows.push_back(AZStd::move(row));
            }

            PQclear(res);
            return qr;
        }

        bool Execute(const char* sql) override
        {
            if (!m_conn) return false;

            PGresult* res = PQexec(m_conn, sql);
            ExecStatusType status = PQresultStatus(res);
            bool ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);

            if (!ok)
            {
                fprintf(stderr, "[PostgresBackend] Execute error: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
            }

            PQclear(res);
            return ok;
        }

        bool ExecuteParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) override
        {
            if (!m_conn) return false;

            AZStd::vector<const char*> paramValues(params.size());
            AZStd::vector<int> paramLengths(params.size());
            AZStd::vector<int> paramFormats(params.size(), 0);

            for (size_t i = 0; i < params.size(); ++i)
            {
                paramValues[i] = params[i].c_str();
                paramLengths[i] = static_cast<int>(params[i].size());
            }

            PGresult* res = PQexecParams(m_conn, sql,
                static_cast<int>(params.size()),
                nullptr,
                paramValues.data(),
                paramLengths.data(),
                paramFormats.data(),
                0);

            ExecStatusType status = PQresultStatus(res);
            bool ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);

            if (!ok)
            {
                fprintf(stderr, "[PostgresBackend] ExecuteParams error: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
            }

            PQclear(res);
            return ok;
        }

        bool BeginTransaction() override
        {
            return Execute("BEGIN");
        }

        bool CommitTransaction() override
        {
            return Execute("COMMIT");
        }

        bool RollbackTransaction() override
        {
            return Execute("ROLLBACK");
        }

        const char* BackendName() const override { return "postgres"; }

        void* GetRawConnection() const override { return m_conn; }

    private:
        PGconn* m_conn = nullptr;
    };

    // Factory registration — called from CreateDatabaseBackend()
    AZStd::unique_ptr<IDatabaseBackend> CreatePostgresBackend()
    {
        return AZStd::make_unique<PostgresBackend>();
    }

} // namespace HCPEngine
