#pragma once

/// HCP Workstation Engine — lightweight kernel host for standalone operation.
///
/// Embeds the read/write DB kernels + LMDB vocabulary so the workstation
/// can browse and edit data without a running daemon. Resolution operations
/// (tokenize, ingest, byte resolve) still require the daemon
/// via HCPSocketClient.
///
/// Kernel composition mirrors HCPEngineSystemComponent but excludes:
///   - Resolution pipeline (VocabBed, byte-floor, AZSL settle)
///   - SocketServer (we're a client, not a server)
///   - EnvelopeManager, EntityAnnotator (daemon-side lifecycle)
///
/// Pattern: each kernel takes HCPDbConnection& — does not own the connection.

#include "../HCPDbConnection.h"
#include "../HCPPbmWriter.h"
#include "../HCPPbmReader.h"
#include "../HCPDocumentQuery.h"
#include "../HCPDocVarQuery.h"
#include "../HCPBondQuery.h"
#include "../HCPVocabulary.h"
#include "../HCPStorage.h"       // Entity cross-ref free functions

#include <AzCore/std/string/string.h>

struct pg_conn;
typedef pg_conn PGconn;

namespace HCPEngine
{
    class HCPWorkstationEngine
    {
    public:
        HCPWorkstationEngine() = default;
        ~HCPWorkstationEngine();

        HCPWorkstationEngine(const HCPWorkstationEngine&) = delete;
        HCPWorkstationEngine& operator=(const HCPWorkstationEngine&) = delete;

        /// Initialize the engine with database and vocabulary.
        /// @param pbmConnInfo libpq connection string for hcp_fic_pbm (primary data)
        /// @param vocabPath LMDB directory path (default: data/vocab.lmdb)
        /// @param ficEntConnInfo optional connection for hcp_fic_entities (entity cross-ref)
        /// @param nfEntConnInfo optional connection for hcp_nf_entities (author lookup)
        /// @return true if at least the primary DB + vocab loaded successfully
        bool Initialize(
            const char* pbmConnInfo = nullptr,
            const char* vocabPath = nullptr,
            const char* ficEntConnInfo = nullptr,
            const char* nfEntConnInfo = nullptr);

        /// Shut down connections and release resources.
        void Shutdown();

        // ---- State ----

        bool IsDbConnected() const { return m_dbConn.IsConnected(); }
        bool IsVocabLoaded() const { return m_vocabulary.IsLoaded(); }
        bool IsReady() const { return m_dbConn.IsConnected() && m_vocabulary.IsLoaded(); }

        // ---- Kernel accessors (same pattern as HCPEngineSystemComponent) ----

        HCPDbConnection& GetDbConnection() { return m_dbConn; }
        HCPPbmWriter& GetPbmWriter() { return m_pbmWriter; }
        HCPPbmReader& GetPbmReader() { return m_pbmReader; }
        HCPDocumentQuery& GetDocumentQuery() { return m_docQuery; }
        HCPDocVarQuery& GetDocVarQuery() { return m_docVarQuery; }
        HCPBondQuery& GetBondQuery() { return m_bondQuery; }
        const HCPVocabulary& GetVocabulary() const { return m_vocabulary; }
        HCPVocabulary& GetVocabularyMut() { return m_vocabulary; }

        // ---- Entity cross-reference (separate DB connections) ----

        /// Get fiction entities for a document. Returns empty if fic_entities DB not connected.
        AZStd::vector<EntityInfo> GetFictionEntities(int docPk);

        /// Get non-fiction author entity by name. Returns empty entity if nf_entities DB not connected.
        EntityInfo GetNfAuthor(const AZStd::string& authorName);

        bool HasEntityConnections() const { return m_ficEntConn != nullptr || m_nfEntConn != nullptr; }

    private:
        // Primary data connection (hcp_fic_pbm)
        HCPDbConnection m_dbConn;

        // Kernels — brace-init with shared connection reference
        HCPPbmWriter m_pbmWriter{m_dbConn};
        HCPPbmReader m_pbmReader{m_dbConn};
        HCPDocumentQuery m_docQuery{m_dbConn};
        HCPDocVarQuery m_docVarQuery{m_dbConn};
        HCPBondQuery m_bondQuery{m_dbConn};

        // Vocabulary — LMDB, standalone (no Postgres at runtime)
        HCPVocabulary m_vocabulary;

        // Entity cross-ref connections (optional, separate DBs)
        PGconn* m_ficEntConn = nullptr;   // hcp_fic_entities
        PGconn* m_nfEntConn = nullptr;    // hcp_nf_entities
    };

} // namespace HCPEngine
