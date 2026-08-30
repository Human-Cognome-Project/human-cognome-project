#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace HCPEngine
{
    /// Abstract database backend — thin interface over write kernel operations.
    /// Implementations: PostgresBackend (production), SqliteBackend (standalone).
    /// Selected at build time via CMake option, runtime config for connection details.
    class IDatabaseBackend
    {
    public:
        virtual ~IDatabaseBackend() = default;

        /// Connect to the database. Returns true on success.
        virtual bool Connect(const char* connectionString = nullptr) = 0;

        /// Disconnect and release resources.
        virtual void Disconnect() = 0;

        virtual bool IsConnected() const = 0;

        // ---- Query execution ----

        /// Result of a query — rows × columns of string values.
        struct QueryResult
        {
            bool success = false;
            int rowCount = 0;
            int colCount = 0;
            AZStd::vector<AZStd::vector<AZStd::string>> rows;

            AZStd::string GetValue(int row, int col) const
            {
                if (row < rowCount && col < colCount)
                    return rows[row][col];
                return {};
            }
        };

        /// Execute a query with no parameters. Returns results.
        virtual QueryResult Query(const char* sql) = 0;

        /// Execute a parameterized query. $1, $2, ... placeholders.
        virtual QueryResult QueryParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) = 0;

        /// Execute a command (INSERT/UPDATE/DELETE) with no result rows.
        /// Returns true on success.
        virtual bool Execute(const char* sql) = 0;

        /// Execute a parameterized command.
        virtual bool ExecuteParams(
            const char* sql,
            const AZStd::vector<AZStd::string>& params) = 0;

        // ---- Bulk operations ----

        /// Begin a batch insert transaction.
        virtual bool BeginTransaction() = 0;

        /// Commit a batch insert transaction.
        virtual bool CommitTransaction() = 0;

        /// Rollback a transaction.
        virtual bool RollbackTransaction() = 0;

        // ---- Backend identification ----

        /// Returns "postgres" or "sqlite".
        virtual const char* BackendName() const = 0;

        /// Get the raw connection handle for backend-specific operations.
        /// Postgres: returns PGconn*. SQLite: returns sqlite3*.
        virtual void* GetRawConnection() const = 0;
    };

    /// Create a database backend by name.
    /// @param backendName "postgres" or "sqlite"
    /// @param connectionString Backend-specific connection string
    AZStd::unique_ptr<IDatabaseBackend> CreateDatabaseBackend(
        const char* backendName,
        const char* connectionString = nullptr);

} // namespace HCPEngine
