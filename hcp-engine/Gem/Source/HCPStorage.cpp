#include "HCPStorage.h"

#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    // ---- Entity cross-reference free functions ----

    AZStd::vector<EntityInfo> GetFictionEntitiesForDocument(
        PGconn* ficEntConn, PGconn* pbmConn, int docPk)
    {
        AZStd::vector<EntityInfo> entities;
        if (!ficEntConn || !pbmConn) return entities;

        // Step 1: Get all starter token_a_id values from the document
        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* pbmParams[] = { pkStr.c_str() };
        PGresult* starterRes = PQexecParams(pbmConn,
            "SELECT DISTINCT token_a_id FROM pbm_starters WHERE doc_id = $1",
            1, nullptr, pbmParams, nullptr, nullptr, 0);
        if (PQresultStatus(starterRes) != PGRES_TUPLES_OK || PQntuples(starterRes) == 0)
        {
            PQclear(starterRes);
            return entities;
        }

        // Build IN-clause token list
        AZStd::string inClause = "(";
        for (int i = 0; i < PQntuples(starterRes); ++i)
        {
            if (i > 0) inClause += ",";
            inClause += "'";
            // Escape single quotes in token IDs (shouldn't happen but be safe)
            const char* tok = PQgetvalue(starterRes, i, 0);
            for (const char* p = tok; *p; ++p)
            {
                if (*p == '\'') inClause += "''";
                else inClause += *p;
            }
            inClause += "'";
        }
        inClause += ")";
        PQclear(starterRes);

        // Step 2: Cross-reference against entity_names in fiction entities DB
        AZStd::string query =
            "SELECT DISTINCT en.entity_id, t.name, t.category "
            "FROM entity_names en "
            "JOIN tokens t ON t.token_id = en.entity_id "
            "WHERE en.token_id IN " + inClause;

        PGresult* entRes = PQexec(ficEntConn, query.c_str());
        if (PQresultStatus(entRes) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[HCPStorage] Entity cross-ref query failed: %s\n",
                PQerrorMessage(ficEntConn));
            fflush(stderr);
            PQclear(entRes);
            return entities;
        }

        // Step 3: For each matched entity, fetch properties
        for (int i = 0; i < PQntuples(entRes); ++i)
        {
            EntityInfo info;
            info.entityId = PQgetvalue(entRes, i, 0);
            info.name = PQgetvalue(entRes, i, 1);
            info.category = PQgetvalue(entRes, i, 2);

            const char* propParams[] = { info.entityId.c_str() };
            PGresult* propRes = PQexecParams(ficEntConn,
                "SELECT key, value FROM entity_properties WHERE entity_id = $1",
                1, nullptr, propParams, nullptr, nullptr, 0);
            if (PQresultStatus(propRes) == PGRES_TUPLES_OK)
            {
                for (int j = 0; j < PQntuples(propRes); ++j)
                {
                    info.properties.push_back({
                        AZStd::string(PQgetvalue(propRes, j, 0)),
                        AZStd::string(PQgetvalue(propRes, j, 1))
                    });
                }
            }
            PQclear(propRes);

            entities.push_back(AZStd::move(info));
        }
        PQclear(entRes);

        return entities;
    }

    EntityInfo GetNfAuthorEntity(PGconn* nfEntConn, const AZStd::string& authorName)
    {
        EntityInfo info;
        if (!nfEntConn || authorName.empty()) return info;

        // Build lowercase search term from the last word of the author name (surname)
        AZStd::string searchName(authorName);
        // Lowercase the whole name and replace spaces with underscores for token name match
        for (auto& c : searchName)
        {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isupper(uc)) c = static_cast<char>(std::tolower(uc));
            if (c == ' ') c = '_';
        }

        // Search by name substring match
        AZStd::string pattern = "%" + searchName + "%";
        const char* params[] = { pattern.c_str() };
        PGresult* res = PQexecParams(nfEntConn,
            "SELECT token_id, name, category FROM tokens "
            "WHERE name LIKE $1 LIMIT 1",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
        {
            PQclear(res);
            return info;
        }

        info.entityId = PQgetvalue(res, 0, 0);
        info.name = PQgetvalue(res, 0, 1);
        info.category = PQgetvalue(res, 0, 2);
        PQclear(res);

        // Fetch properties
        const char* propParams[] = { info.entityId.c_str() };
        PGresult* propRes = PQexecParams(nfEntConn,
            "SELECT key, value FROM entity_properties WHERE entity_id = $1",
            1, nullptr, propParams, nullptr, nullptr, 0);
        if (PQresultStatus(propRes) == PGRES_TUPLES_OK)
        {
            for (int j = 0; j < PQntuples(propRes); ++j)
            {
                info.properties.push_back({
                    AZStd::string(PQgetvalue(propRes, j, 0)),
                    AZStd::string(PQgetvalue(propRes, j, 1))
                });
            }
        }
        PQclear(propRes);

        return info;
    }

} // namespace HCPEngine
