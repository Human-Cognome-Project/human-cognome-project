#include "HCPDocVarQuery.h"

#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>

namespace HCPEngine
{
    AZStd::vector<HCPDocVarQuery::DocVar> HCPDocVarQuery::GetDocVars(int docPk)
    {
        AZStd::vector<DocVar> vars;
        PGconn* pg = m_conn.Get();
        if (!pg) return vars;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT var_id, surface FROM pbm_docvars WHERE doc_id = $1 ORDER BY var_id",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                DocVar v;
                v.varId = PQgetvalue(res, i, 0);
                v.surface = PQgetvalue(res, i, 1);
                vars.push_back(AZStd::move(v));
            }
        }
        PQclear(res);
        return vars;
    }

    AZStd::vector<HCPDocVarQuery::DocVarExtended> HCPDocVarQuery::GetDocVarsExtended(int docPk)
    {
        AZStd::vector<DocVarExtended> vars;
        PGconn* pg = m_conn.Get();
        if (!pg) return vars;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT v.var_id, v.surface, COALESCE(v.var_category, ''), "
            "       COALESCE(v.group_id, 0), COALESCE(g.suggested_id, ''), "
            "       COALESCE(g.status, '') "
            "FROM pbm_docvars v "
            "LEFT JOIN docvar_groups g ON g.id = v.group_id "
            "WHERE v.doc_id = $1 ORDER BY v.var_id",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                DocVarExtended v;
                v.varId = PQgetvalue(res, i, 0);
                v.surface = PQgetvalue(res, i, 1);
                v.category = PQgetvalue(res, i, 2);
                v.groupId = atoi(PQgetvalue(res, i, 3));
                v.suggestedId = PQgetvalue(res, i, 4);
                v.groupStatus = PQgetvalue(res, i, 5);
                vars.push_back(AZStd::move(v));
            }
        }
        PQclear(res);
        return vars;
    }

} // namespace HCPEngine
