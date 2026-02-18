#include "HCPVocabulary.h"

#include <AzCore/Console/ILogger.h>
#include <libpq-fe.h>

namespace HCPEngine
{
    static constexpr const char* DB_CONNINFO = "dbname=hcp_english user=hcp password=hcp_dev host=localhost port=5432";

    bool HCPVocabulary::Load()
    {
        PGconn* conn = PQconnectdb(DB_CONNINFO);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            AZLOG_ERROR("HCPVocabulary: Failed to connect to hcp_english: %s", PQerrorMessage(conn));
            PQfinish(conn);
            return false;
        }

        // Load word tokens: word_form -> token_id
        // Words live in namespace AB.AB (English words)
        {
            PGresult* res = PQexec(conn,
                "SELECT word_form, token_id FROM english_words");
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                AZLOG_ERROR("HCPVocabulary: Failed to query english_words: %s", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return false;
            }

            int rows = PQntuples(res);
            m_wordToToken.reserve(rows);
            m_tokenToWord.reserve(rows);

            for (int i = 0; i < rows; ++i)
            {
                AZStd::string form(PQgetvalue(res, i, 0));
                AZStd::string tokenId(PQgetvalue(res, i, 1));
                m_wordToToken[form] = tokenId;
                m_tokenToWord[tokenId] = form;
            }
            PQclear(res);
            AZLOG_INFO("HCPVocabulary: Loaded %zu words", m_wordToToken.size());
        }

        // Load label tokens: label -> token_id
        // Labels are word forms associated with token IDs
        {
            PGresult* res = PQexec(conn,
                "SELECT label, token_id FROM english_labels");
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                AZLOG_ERROR("HCPVocabulary: Failed to query english_labels: %s", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return false;
            }

            int rows = PQntuples(res);
            m_labelToToken.reserve(rows);

            for (int i = 0; i < rows; ++i)
            {
                AZStd::string label(PQgetvalue(res, i, 0));
                AZStd::string tokenId(PQgetvalue(res, i, 1));
                m_labelToToken[label] = tokenId;
            }
            PQclear(res);
            AZLOG_INFO("HCPVocabulary: Loaded %zu labels", m_labelToToken.size());
        }

        // Load character tokens: single chars from the character token table
        {
            PGresult* res = PQexec(conn,
                "SELECT label, token_id FROM core_tokens "
                "WHERE namespace = 'AA' AND length(label) = 1");
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                AZLOG_ERROR("HCPVocabulary: Failed to query character tokens: %s", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return false;
            }

            int rows = PQntuples(res);
            for (int i = 0; i < rows; ++i)
            {
                const char* label = PQgetvalue(res, i, 0);
                AZStd::string tokenId(PQgetvalue(res, i, 1));
                if (label && label[0])
                {
                    m_charToToken[label[0]] = tokenId;
                    m_tokenToChar[tokenId] = label[0];
                }
            }
            PQclear(res);
            AZLOG_INFO("HCPVocabulary: Loaded %zu character tokens", m_charToToken.size());
        }

        PQfinish(conn);
        m_loaded = true;

        AZLOG_INFO("HCPVocabulary: Vocabulary loaded — %zu words, %zu labels, %zu chars",
            m_wordToToken.size(), m_labelToToken.size(), m_charToToken.size());
        return true;
    }

    AZStd::string HCPVocabulary::LookupWord(const AZStd::string& wordForm) const
    {
        auto it = m_wordToToken.find(wordForm);
        if (it != m_wordToToken.end())
        {
            return it->second;
        }
        return {};
    }

    AZStd::string HCPVocabulary::LookupChar(char c) const
    {
        auto it = m_charToToken.find(c);
        if (it != m_charToToken.end())
        {
            return it->second;
        }
        return {};
    }

    AZStd::string HCPVocabulary::LookupLabel(const AZStd::string& label) const
    {
        auto it = m_labelToToken.find(label);
        if (it != m_labelToToken.end())
        {
            return it->second;
        }
        return {};
    }

    AZStd::string HCPVocabulary::TokenToWord(const AZStd::string& tokenId) const
    {
        auto it = m_tokenToWord.find(tokenId);
        if (it != m_tokenToWord.end())
        {
            return it->second;
        }
        return {};
    }

    char HCPVocabulary::TokenToChar(const AZStd::string& tokenId) const
    {
        auto it = m_tokenToChar.find(tokenId);
        if (it != m_tokenToChar.end())
        {
            return it->second;
        }
        return '\0';
    }

} // namespace HCPEngine
