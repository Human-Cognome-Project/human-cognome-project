#pragma once

#include "HCPDbConnection.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    //! Bond query kernel.
    //! Drill-down queries into PBM bond subtables for a specific document.
    class HCPBondQuery
    {
    public:
        explicit HCPBondQuery(HCPDbConnection& conn) : m_conn(conn) {}

        struct BondEntry
        {
            AZStd::string tokenB;
            int count = 0;
        };

        //! Get bonds for a specific token in a document.
        //! If tokenId is empty, returns top starters with their total bond counts.
        AZStd::vector<BondEntry> GetBondsForToken(int docPk, const AZStd::string& tokenId);

        //! Get ALL starters for a document (no LIMIT). For bond search.
        AZStd::vector<BondEntry> GetAllStarters(int docPk);

    private:
        HCPDbConnection& m_conn;
    };

} // namespace HCPEngine
