#include "HCPDatabaseBackend.h"

#include <sqlite3.h>
#include <cstdio>
#include <cstring>

namespace HCPEngine
{
    class SqliteBackend : public IDatabaseBackend
    {
    public:
        SqliteBackend() = default;

        ~SqliteBackend() override
        {
            Disconnect();
        }

        bool Connect(const char* connectionString) override
        {
            if (m_db)
                Disconnect();

            // Connection string is the file path for SQLite
            const char* path = connectionString;
            if (!path || !path[0])
                path = "hcp_workstation.db";

            int rc = sqlite3_open(path, &m_db);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "[SqliteBackend] Open failed: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                sqlite3_close(m_db);
                m_db = nullptr;
                return false;
            }

            // Enable WAL mode for concurrent reads
            sqlite3_exec(m_db, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
            // Enable foreign keys
            sqlite3_exec(m_db, "PRAGMA foreign_keys=ON", nullptr, nullptr, nullptr);
            // Reasonable busy timeout for concurrent access
            sqlite3_busy_timeout(m_db, 5000);

            return true;
        }

        void Disconnect() override
        {
            if (m_db)
            {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
        }

        bool IsConnected() const override
        {
            return m_db != nullptr;
        }

        QueryResult Query(const char* sql) override
        {
            QueryResult qr;
            if (!m_db) return qr;

            sqlite3_stmt* stmt;
            int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "[SqliteBackend] Prepare error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                return qr;
            }

            qr.colCount = sqlite3_column_count(stmt);
            qr.success = true;

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                AZStd::vector<AZStd::string> row;
                row.reserve(qr.colCount);
                for (int c = 0; c < qr.colCount; ++c)
                {
                    const char* val = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, c));
                    row.push_back(val ? AZStd::string(val) : AZStd::string());
                }
                qr.rows.push_back(AZStd::move(row));
                ++qr.rowCount;
            }

            if (rc != SQLITE_DONE)
            {
                fprintf(stderr, "[SqliteBackend] Step error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                qr.success = false;
            }

            sqlite3_finalize(stmt);
            return qr;
        }

        QueryResult QueryParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) override
        {
            QueryResult qr;
            if (!m_db) return qr;

            sqlite3_stmt* stmt;
            int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "[SqliteBackend] Prepare error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                return qr;
            }

            // Bind parameters — SQLite uses 1-based indexing
            for (size_t i = 0; i < params.size(); ++i)
            {
                sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                    params[i].c_str(), static_cast<int>(params[i].size()),
                    SQLITE_TRANSIENT);
            }

            qr.colCount = sqlite3_column_count(stmt);
            qr.success = true;

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                AZStd::vector<AZStd::string> row;
                row.reserve(qr.colCount);
                for (int c = 0; c < qr.colCount; ++c)
                {
                    const char* val = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, c));
                    row.push_back(val ? AZStd::string(val) : AZStd::string());
                }
                qr.rows.push_back(AZStd::move(row));
                ++qr.rowCount;
            }

            if (rc != SQLITE_DONE)
            {
                fprintf(stderr, "[SqliteBackend] Step error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                qr.success = false;
            }

            sqlite3_finalize(stmt);
            return qr;
        }

        bool Execute(const char* sql) override
        {
            if (!m_db) return false;

            char* errMsg = nullptr;
            int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "[SqliteBackend] Execute error: %s\n",
                    errMsg ? errMsg : "unknown");
                fflush(stderr);
                sqlite3_free(errMsg);
                return false;
            }

            return true;
        }

        bool ExecuteParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) override
        {
            if (!m_db) return false;

            sqlite3_stmt* stmt;
            int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "[SqliteBackend] Prepare error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
                return false;
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                    params[i].c_str(), static_cast<int>(params[i].size()),
                    SQLITE_TRANSIENT);
            }

            rc = sqlite3_step(stmt);
            bool ok = (rc == SQLITE_DONE || rc == SQLITE_ROW);
            if (!ok)
            {
                fprintf(stderr, "[SqliteBackend] ExecuteParams error: %s\n",
                    sqlite3_errmsg(m_db));
                fflush(stderr);
            }

            sqlite3_finalize(stmt);
            return ok;
        }

        bool BeginTransaction() override
        {
            return Execute("BEGIN TRANSACTION");
        }

        bool CommitTransaction() override
        {
            return Execute("COMMIT");
        }

        bool RollbackTransaction() override
        {
            return Execute("ROLLBACK");
        }

        const char* BackendName() const override { return "sqlite"; }

        void* GetRawConnection() const override { return m_db; }

        /// Initialize the SQLite database with HCP schema (parity with Postgres tables).
        bool InitializeSchema()
        {
            if (!m_db) return false;

            const char* schema = R"SQL(
                -- PBM Documents
                CREATE TABLE IF NOT EXISTS pbm_documents (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_id TEXT UNIQUE NOT NULL,
                    name TEXT NOT NULL,
                    century_code TEXT DEFAULT 'AS',
                    metadata TEXT DEFAULT '{}',
                    total_slots INTEGER DEFAULT 0,
                    created_at TEXT DEFAULT (datetime('now'))
                );

                -- PBM Starters
                CREATE TABLE IF NOT EXISTS pbm_starters (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_id INTEGER NOT NULL REFERENCES pbm_documents(id),
                    token_a TEXT NOT NULL,
                    token_b TEXT NOT NULL,
                    count INTEGER DEFAULT 1
                );
                CREATE INDEX IF NOT EXISTS idx_starters_doc ON pbm_starters(doc_id);
                CREATE INDEX IF NOT EXISTS idx_starters_token_a ON pbm_starters(token_a);

                -- PBM Bond subtables (flattened into single table with level column)
                CREATE TABLE IF NOT EXISTS pbm_bonds (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_id INTEGER NOT NULL REFERENCES pbm_documents(id),
                    level INTEGER NOT NULL DEFAULT 1,
                    token_a TEXT NOT NULL,
                    token_b TEXT NOT NULL,
                    count INTEGER DEFAULT 1
                );
                CREATE INDEX IF NOT EXISTS idx_bonds_doc ON pbm_bonds(doc_id);
                CREATE INDEX IF NOT EXISTS idx_bonds_token_a ON pbm_bonds(token_a);

                -- Positions
                CREATE TABLE IF NOT EXISTS pbm_positions (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_id INTEGER NOT NULL REFERENCES pbm_documents(id),
                    token_id TEXT NOT NULL,
                    positions TEXT NOT NULL
                );
                CREATE INDEX IF NOT EXISTS idx_positions_doc ON pbm_positions(doc_id);

                -- Document provenance
                CREATE TABLE IF NOT EXISTS document_provenance (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_id INTEGER NOT NULL REFERENCES pbm_documents(id),
                    source_type TEXT,
                    source_path TEXT,
                    source_format TEXT,
                    catalog TEXT,
                    catalog_id TEXT
                );

                -- Vocabulary tokens (for SQLite vocab cache)
                CREATE TABLE IF NOT EXISTS tokens (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL,
                    token_id TEXT NOT NULL,
                    layer TEXT DEFAULT 'word',
                    category TEXT DEFAULT '',
                    particle_key TEXT GENERATED ALWAYS AS (
                        substr(name, 1, 1) || length(name)
                    ) STORED
                );
                CREATE INDEX IF NOT EXISTS idx_tokens_name ON tokens(name);
                CREATE INDEX IF NOT EXISTS idx_tokens_token_id ON tokens(token_id);
                CREATE INDEX IF NOT EXISTS idx_tokens_particle_key ON tokens(particle_key);

                -- Var tokens
                CREATE TABLE IF NOT EXISTS var_tokens (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    var_id TEXT NOT NULL,
                    form TEXT NOT NULL,
                    status TEXT DEFAULT 'active',
                    category TEXT DEFAULT 'proper',
                    created_at TEXT DEFAULT (datetime('now'))
                );
                CREATE INDEX IF NOT EXISTS idx_var_form ON var_tokens(form);

                -- Var sources
                CREATE TABLE IF NOT EXISTS var_sources (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    var_id TEXT NOT NULL,
                    doc_id TEXT,
                    position INTEGER
                );

                -- Docvar staging
                CREATE TABLE IF NOT EXISTS docvar_staging (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_pk INTEGER NOT NULL,
                    var_id TEXT NOT NULL,
                    surface TEXT NOT NULL,
                    category TEXT DEFAULT 'proper',
                    group_id INTEGER DEFAULT 0,
                    suggested_entity_id TEXT DEFAULT '',
                    status TEXT DEFAULT 'pending'
                );
                CREATE INDEX IF NOT EXISTS idx_docvar_doc ON docvar_staging(doc_pk);

                -- Docvar groups
                CREATE TABLE IF NOT EXISTS docvar_groups (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    doc_pk INTEGER NOT NULL,
                    group_id INTEGER NOT NULL,
                    entity_id TEXT DEFAULT '',
                    status TEXT DEFAULT 'pending'
                );
            )SQL";

            return Execute(schema);
        }

    private:
        sqlite3* m_db = nullptr;
    };

    // Factory registration
    AZStd::unique_ptr<IDatabaseBackend> CreateSqliteBackend()
    {
        return AZStd::make_unique<SqliteBackend>();
    }

    // ---- Factory ----

    // Forward declarations from backend files
    AZStd::unique_ptr<IDatabaseBackend> CreatePostgresBackend();

    AZStd::unique_ptr<IDatabaseBackend> CreateDatabaseBackend(
        const char* backendName,
        const char* connectionString)
    {
        AZStd::unique_ptr<IDatabaseBackend> backend;

        if (strcmp(backendName, "sqlite") == 0)
        {
            backend = CreateSqliteBackend();
        }
        else
        {
            // Default to postgres
            backend = CreatePostgresBackend();
        }

        if (connectionString && connectionString[0])
        {
            backend->Connect(connectionString);
        }

        return backend;
    }

} // namespace HCPEngine
