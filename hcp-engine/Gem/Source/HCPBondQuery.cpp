#include "HCPBondQuery.h"

#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>

namespace HCPEngine
{
    AZStd::vector<HCPBondQuery::BondEntry> HCPBondQuery::GetBondsForToken(
        int docPk, const AZStd::string& tokenId)
    {
        AZStd::vector<BondEntry> bonds;
        PGconn* pg = m_conn.Get();
        if (!pg) return bonds;

        AZStd::string pkStr = AZStd::to_string(docPk);

        if (tokenId.empty())
        {
            // Overview mode: top starters by total bond count
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT s.token_a_id, "
                "  COALESCE((SELECT SUM(wb.count) FROM pbm_word_bonds wb WHERE wb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(cb.count) FROM pbm_char_bonds cb WHERE cb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(mb.count) FROM pbm_marker_bonds mb WHERE mb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(vb.count) FROM pbm_var_bonds vb WHERE vb.starter_id = s.id), 0) AS total "
                "FROM pbm_starters s WHERE s.doc_id = $1 "
                "ORDER BY total DESC LIMIT 50",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    BondEntry be;
                    be.tokenB = PQgetvalue(res, i, 0);
                    be.count = atoi(PQgetvalue(res, i, 1));
                    bonds.push_back(AZStd::move(be));
                }
            }
            PQclear(res);
        }
        else
        {
            // Drill-down: bonds for a specific A-side token
            const char* params[] = { pkStr.c_str(), tokenId.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT s.id FROM pbm_starters s "
                "WHERE s.doc_id = $1 AND s.token_a_id = $2",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                PQclear(res);
                return bonds;
            }
            int starterId = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);

            AZStd::string sidStr = AZStd::to_string(starterId);

            // Word bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(pg,
                    "SELECT 'AB.AB.' || b_p3 || '.' || b_p4 || '.' || b_p5, count "
                    "FROM pbm_word_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Char bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(pg,
                    "SELECT 'AA.' || b_p2 || '.' || b_p3 || '.' || b_p4 || '.' || b_p5, count "
                    "FROM pbm_char_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Marker bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(pg,
                    "SELECT 'AA.AE.' || b_p3 || '.' || b_p4, count "
                    "FROM pbm_marker_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Var bonds
            {
                const char* p[] = { sidStr.c_str(), pkStr.c_str() };
                res = PQexecParams(pg,
                    "SELECT COALESCE(dv.surface, vb.b_var_id), vb.count "
                    "FROM pbm_var_bonds vb "
                    "LEFT JOIN pbm_docvars dv ON dv.doc_id = $2::integer AND dv.var_id = vb.b_var_id "
                    "WHERE vb.starter_id = $1::integer ORDER BY vb.count DESC",
                    2, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }
        }

        return bonds;
    }

    AZStd::vector<HCPBondQuery::BondEntry> HCPBondQuery::GetAllStarters(int docPk)
    {
        AZStd::vector<BondEntry> starters;
        PGconn* pg = m_conn.Get();
        if (!pg) return starters;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT s.token_a_id, "
            "  COALESCE((SELECT SUM(wb.count) FROM pbm_word_bonds wb WHERE wb.starter_id = s.id), 0) + "
            "  COALESCE((SELECT SUM(cb.count) FROM pbm_char_bonds cb WHERE cb.starter_id = s.id), 0) + "
            "  COALESCE((SELECT SUM(mb.count) FROM pbm_marker_bonds mb WHERE mb.starter_id = s.id), 0) + "
            "  COALESCE((SELECT SUM(vb.count) FROM pbm_var_bonds vb WHERE vb.starter_id = s.id), 0) AS total "
            "FROM pbm_starters s WHERE s.doc_id = $1 "
            "ORDER BY total DESC",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                BondEntry be;
                be.tokenB = PQgetvalue(res, i, 0);
                be.count = atoi(PQgetvalue(res, i, 1));
                starters.push_back(AZStd::move(be));
            }
        }
        PQclear(res);
        return starters;
    }

} // namespace HCPEngine
