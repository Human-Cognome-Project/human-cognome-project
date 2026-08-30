#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/base.h>

namespace HCPEngine
{
    class HCPVocabulary;

    //! Compiled PBM bond table for a single LoD level.
    //! Maps directional pairs (a, b) to bond counts.
    //! Used as force constants in the physics detection scene.
    class HCPBondTable
    {
    public:
        //! Look up bond strength between two adjacent elements.
        //! Returns 0 if no bond exists.
        AZ::u32 GetBondStrength(const AZStd::string& a, const AZStd::string& b) const;

        //! Increment a directional bond count.
        void AddBond(const AZStd::string& a, const AZStd::string& b, AZ::u32 count = 1);

        //! Number of unique directional pairs with non-zero counts.
        size_t PairCount() const { return m_bonds.size(); }

        //! Sum of all bond counts.
        size_t TotalCount() const { return m_totalCount; }

        //! Maximum bond count across all pairs (useful for normalization).
        AZ::u32 MaxCount() const { return m_maxCount; }

        //! Direct access for persistence or debugging.
        const AZStd::unordered_map<AZStd::string, AZ::u32>& GetAllBonds() const { return m_bonds; }

        //! Log summary stats.
        void LogStats(const char* label) const;

    private:
        static AZStd::string MakeKey(const AZStd::string& a, const AZStd::string& b);

        // Key = "a|b", value = count
        AZStd::unordered_map<AZStd::string, AZ::u32> m_bonds;
        size_t m_totalCount = 0;
        AZ::u32 m_maxCount = 0;
    };

    //! Compile char→word PBM bonds from vocabulary.
    //! Iterates all word forms, extracts adjacent character pairs, counts.
    //! Result: the spelling model — force constants for char→word assembly.
    HCPBondTable CompileCharWordBonds(const HCPVocabulary& vocab);

    //! Compile byte→char PBM bonds from vocabulary.
    //! Iterates all characters, extracts adjacent byte pairs in their UTF-8 encoding.
    //! Result: encoding model — force constants for byte→char assembly.
    //! Mostly trivial for ASCII; important for multi-byte encodings.
    HCPBondTable CompileByteCharBonds(const HCPVocabulary& vocab);

    //! Compile char→word PBM bonds directly from Postgres (source of truth).
    //! Use when LMDB cache is empty or incomplete.
    //! @param connInfo libpq connection string (e.g., "host=localhost dbname=hcp_english user=hcp password=hcp_dev")
    HCPBondTable CompileCharWordBondsFromPostgres(const char* connInfo);

    //! Compile byte→char PBM bonds directly from Postgres.
    //! @param connInfo libpq connection string for hcp_core
    HCPBondTable CompileByteCharBondsFromPostgres(const char* connInfo);

    // ---- Temp Postgres persistence (hcp_temp — DB specialist assigns permanent home) ----

    static constexpr const char* TEMP_CONNINFO =
        "host=localhost dbname=hcp_temp user=hcp password=hcp_dev";

    //! Save a compiled bond table to hcp_temp.bond_aggregates.
    //! @param level "byte_char" or "char_word"
    bool SaveBondTable(const HCPBondTable& table, const char* level,
                       const char* connInfo = TEMP_CONNINFO);

    //! Load a bond table from hcp_temp.bond_aggregates. Returns empty table if not found.
    //! @param level "byte_char" or "char_word"
    HCPBondTable LoadBondTable(const char* level,
                               const char* connInfo = TEMP_CONNINFO);

    // Forward declare — defined in HCPParticlePipeline.h
    struct PBMData;

    //! Save a document's derived PBM to hcp_temp.doc_pbm.
    //! Replaces any existing PBM for this doc_name.
    bool SaveDocPBM(const AZStd::string& docName, const PBMData& pbm,
                    const char* connInfo = TEMP_CONNINFO);

} // namespace HCPEngine
