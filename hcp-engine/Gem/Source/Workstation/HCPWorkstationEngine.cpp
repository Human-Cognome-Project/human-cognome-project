/// HCP Workstation Engine — lightweight kernel host implementation.

#include "HCPWorkstationEngine.h"

#include <libpq-fe.h>
#include <cstdio>

namespace HCPEngine
{
    static constexpr const char* DEFAULT_PBM_CONNINFO =
        "dbname=hcp_fic_pbm user=hcp password=hcp_dev host=192.168.68.60 port=5435";

    static constexpr const char* DEFAULT_FIC_ENT_CONNINFO =
        "dbname=hcp_fic_entities user=hcp password=hcp_dev host=192.168.68.60 port=5435";

    static constexpr const char* DEFAULT_NF_ENT_CONNINFO =
        "dbname=hcp_nf_entities user=hcp password=hcp_dev host=192.168.68.60 port=5435";

    static constexpr const char* DEFAULT_VOCAB_PATH =
        "/opt/project/repo/data/vocab.lmdb";


    HCPWorkstationEngine::~HCPWorkstationEngine()
    {
        Shutdown();
    }

    bool HCPWorkstationEngine::Initialize(
        const char* pbmConnInfo,
        const char* vocabPath,
        const char* ficEntConnInfo,
        const char* nfEntConnInfo)
    {
        // 1. Primary DB connection (required)
        bool dbOk = m_dbConn.Connect(pbmConnInfo ? pbmConnInfo : DEFAULT_PBM_CONNINFO);
        if (!dbOk)
        {
            fprintf(stderr, "[WorkstationEngine] WARNING: Primary DB connection failed. "
                "Offline features requiring DB will be unavailable.\n");
        }

        // 2. Vocabulary LMDB (required for surface resolution)
        bool vocabOk = m_vocabulary.Load(vocabPath ? vocabPath : DEFAULT_VOCAB_PATH);
        if (!vocabOk)
        {
            fprintf(stderr, "[WorkstationEngine] WARNING: Vocabulary LMDB failed to load from %s\n",
                vocabPath ? vocabPath : DEFAULT_VOCAB_PATH);
        }

        // 3. Entity cross-ref connections (optional)
        const char* ficStr = ficEntConnInfo ? ficEntConnInfo : DEFAULT_FIC_ENT_CONNINFO;
        m_ficEntConn = PQconnectdb(ficStr);
        if (PQstatus(m_ficEntConn) != CONNECTION_OK)
        {
            fprintf(stderr, "[WorkstationEngine] Entity DB (fic_entities) not available: %s\n",
                PQerrorMessage(m_ficEntConn));
            PQfinish(m_ficEntConn);
            m_ficEntConn = nullptr;
        }

        const char* nfStr = nfEntConnInfo ? nfEntConnInfo : DEFAULT_NF_ENT_CONNINFO;
        m_nfEntConn = PQconnectdb(nfStr);
        if (PQstatus(m_nfEntConn) != CONNECTION_OK)
        {
            fprintf(stderr, "[WorkstationEngine] Entity DB (nf_entities) not available: %s\n",
                PQerrorMessage(m_nfEntConn));
            PQfinish(m_nfEntConn);
            m_nfEntConn = nullptr;
        }

        fprintf(stderr, "[WorkstationEngine] Initialized: DB=%s, Vocab=%s (%zu words), "
            "FicEnt=%s, NfEnt=%s\n",
            dbOk ? "OK" : "FAIL",
            vocabOk ? "OK" : "FAIL",
            m_vocabulary.WordCount(),
            m_ficEntConn ? "OK" : "N/A",
            m_nfEntConn ? "OK" : "N/A");

        return dbOk || vocabOk;  // Useful if at least one data source loaded
    }

    void HCPWorkstationEngine::Shutdown()
    {
        if (m_ficEntConn)
        {
            PQfinish(m_ficEntConn);
            m_ficEntConn = nullptr;
        }
        if (m_nfEntConn)
        {
            PQfinish(m_nfEntConn);
            m_nfEntConn = nullptr;
        }
        m_dbConn.Disconnect();
    }

    AZStd::vector<EntityInfo> HCPWorkstationEngine::GetFictionEntities(int docPk)
    {
        if (!m_ficEntConn || !m_dbConn.IsConnected())
            return {};

        return GetFictionEntitiesForDocument(m_ficEntConn, m_dbConn.Get(), docPk);
    }

    EntityInfo HCPWorkstationEngine::GetNfAuthor(const AZStd::string& authorName)
    {
        if (!m_nfEntConn)
            return {};

        return GetNfAuthorEntity(m_nfEntConn, authorName);
    }

} // namespace HCPEngine
