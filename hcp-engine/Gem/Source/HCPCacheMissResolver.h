#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

// Forward declare LMDB types (avoids lmdb.h in header for non-Private targets)
typedef struct MDB_env MDB_env;
typedef unsigned int MDB_dbi;

// Forward declare libpq connection type (avoids libpq-fe.h in header)
typedef struct pg_conn PGconn;

namespace HCPEngine
{
    // Base-50 alphabet — matches Python hcp.core.token_id.ALPHABET
    // A-N, P-Z, a-n, p-z (52 Latin letters minus O/o = 50)
    static constexpr const char BASE50_ALPHABET[] =
        "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz";
    static constexpr int BASE50 = 50;

    /// Encode an integer (0-2499) as a 2-char base-50 pair.
    inline AZStd::string EncodePairB50(int value)
    {
        char pair[3];
        pair[0] = BASE50_ALPHABET[value / BASE50];
        pair[1] = BASE50_ALPHABET[value % BASE50];
        pair[2] = '\0';
        return AZStd::string(pair, 2);
    }

    /// Context passed to handlers for document-level metadata.
    struct ResolveContext
    {
        const char* docId = nullptr;
        int position = -1;
    };

    /// Handler fills this on successful resolution.
    struct ResolveResult
    {
        AZStd::string value;   // The resolved value (returned to caller)

        struct LmdbWrite
        {
            AZStd::string subDbName;
            AZStd::string key;
            AZStd::string value;
        };
        AZStd::vector<LmdbWrite> writes;  // LMDB writes to execute
    };

    /// Interface for sub-database-specific cache miss handlers.
    class ICacheMissHandler
    {
    public:
        virtual ~ICacheMissHandler() = default;

        /// Name of the LMDB sub-database this handler serves.
        virtual const char* GetSubDbName() const = 0;

        /// Resolve a cache miss.
        /// @return true if resolved (result.value and result.writes populated)
        virtual bool Resolve(
            const char* key, size_t keyLen,
            const ResolveContext& context,
            ResolveResult& result) = 0;
    };

    /// Generic cache miss resolver — routes misses to registered handlers.
    ///
    /// Each sub-db registers its own handler that knows its Postgres query
    /// and value format. The resolver just routes and writes bytes.
    class CacheMissResolver
    {
    public:
        CacheMissResolver() = default;
        ~CacheMissResolver();

        void RegisterHandler(AZStd::unique_ptr<ICacheMissHandler> handler);

        /// Set LMDB environment and DBI handles (owned by HCPVocabulary).
        void SetLmdbEnv(MDB_env* env);
        void SetLmdbDbi(const char* subDbName, MDB_dbi dbi);

        /// Resolve a cache miss. Returns the value (empty = unresolvable).
        /// Writes resolved data to LMDB for future cache hits.
        AZStd::string HandleMiss(
            const char* subDbName,
            const char* key, size_t keyLen,
            const ResolveContext& context);

        /// Get or lazily open a Postgres connection by database name.
        PGconn* GetConnection(const char* dbname);

        void Shutdown();

    private:
        bool LmdbPut(const char* subDbName,
                      const void* key, size_t keyLen,
                      const void* value, size_t valueLen);

        AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<ICacheMissHandler>> m_handlers;
        AZStd::unordered_map<AZStd::string, MDB_dbi> m_dbis;
        AZStd::unordered_map<AZStd::string, PGconn*> m_pgConns;
        MDB_env* m_env = nullptr;
    };

    // ---- Vocabulary Handlers ----

    /// Resolves word form -> token_id from hcp_english.
    /// Tries lowercase first, then exact match for labels.
    /// Writes to w2t (primary) and t2w (reverse).
    class WordHandler : public ICacheMissHandler
    {
    public:
        WordHandler(CacheMissResolver* resolver) : m_resolver(resolver) {}
        const char* GetSubDbName() const override { return "w2t"; }
        bool Resolve(const char* key, size_t keyLen,
                     const ResolveContext& ctx, ResolveResult& result) override;
    private:
        CacheMissResolver* m_resolver;
    };

    /// Resolves a 4-byte Unicode codepoint -> deterministic token_id (no Postgres query).
    /// Codepoint -> AA.AA.AA.{p4}.{p5} where p4=cp/2500, p5=cp%2500 (base-50 pairs).
    /// For ASCII (cp < 256): p4="AA", so token_id matches legacy AA.AA.AA.AA.{p5}.
    /// Writes to c2t (primary) and t2c (reverse).
    class CharHandler : public ICacheMissHandler
    {
    public:
        const char* GetSubDbName() const override { return "c2t"; }
        bool Resolve(const char* key, size_t keyLen,
                     const ResolveContext& ctx, ResolveResult& result) override;
    };

    /// Resolves label name -> token_id from hcp_english.
    /// Labels are structural token names (e.g., "newline", "tab").
    /// Writes to l2t only (no reverse needed).
    class LabelHandler : public ICacheMissHandler
    {
    public:
        LabelHandler(CacheMissResolver* resolver) : m_resolver(resolver) {}
        const char* GetSubDbName() const override { return "l2t"; }
        bool Resolve(const char* key, size_t keyLen,
                     const ResolveContext& ctx, ResolveResult& result) override;
    private:
        CacheMissResolver* m_resolver;
    };

    /// Mints or returns var tokens from hcp_var.
    /// Triggered when HandleMiss key starts with VAR_REQUEST prefix.
    /// Writes to w2t (var tokens occupy word positions) and t2w (reverse).
    class VarHandler : public ICacheMissHandler
    {
    public:
        VarHandler(CacheMissResolver* resolver) : m_resolver(resolver) {}
        const char* GetSubDbName() const override { return "var"; }
        bool Resolve(const char* key, size_t keyLen,
                     const ResolveContext& ctx, ResolveResult& result) override;
    private:
        CacheMissResolver* m_resolver;
    };

} // namespace HCPEngine
