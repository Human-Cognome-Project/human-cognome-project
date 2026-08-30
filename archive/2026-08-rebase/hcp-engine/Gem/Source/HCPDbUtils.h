#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    // Var request token prefix — must match HCPVocabulary.h VAR_REQUEST
    inline constexpr const char* VAR_PREFIX = "AA.AE.AF.AA.AC";
    inline constexpr size_t VAR_PREFIX_LEN = 14;  // strlen("AA.AE.AF.AA.AC")

    // ---- Base-50 encoding ----

    inline constexpr const char B50[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx";

    inline AZStd::string EncodePairStr(int value)
    {
        if (value < 0) value = 0;
        if (value >= 2500) value = 2499;
        char buf[3];
        buf[0] = B50[value / 50];
        buf[1] = B50[value % 50];
        buf[2] = '\0';
        return AZStd::string(buf);
    }

    // Position encoding: position → 4 chars (two pairs)
    // pair1 = position / 2500, pair2 = position % 2500
    // Max position: 2499 * 2500 + 2499 = 6,249,999
    inline void EncodePosition(int position, char out[4])
    {
        int pair1 = position / 2500;
        int pair2 = position % 2500;
        out[0] = B50[pair1 / 50];
        out[1] = B50[pair1 % 50];
        out[2] = B50[pair2 / 50];
        out[3] = B50[pair2 % 50];
    }

    inline int DecodeB50Char(char c)
    {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'x') return 26 + (c - 'a');
        return 0;
    }

    inline int DecodePosition(const char* p)
    {
        int pair1 = DecodeB50Char(p[0]) * 50 + DecodeB50Char(p[1]);
        int pair2 = DecodeB50Char(p[2]) * 50 + DecodeB50Char(p[3]);
        return pair1 * 2500 + pair2;
    }

    // ---- Token ID splitting ----

    // Split "AB.AB.CD.AH.xN" → parts[0..4]
    // Only splits on the first 4 dots — everything after the 4th dot goes into parts[4].
    inline void SplitTokenId(const AZStd::string& tokenId, AZStd::string parts[5])
    {
        int idx = 0;
        size_t start = 0;
        for (size_t i = 0; i < tokenId.size() && idx < 4; ++i)
        {
            if (tokenId[i] == '.')
            {
                parts[idx++] = AZStd::string(tokenId.data() + start, i - start);
                start = i + 1;
            }
        }
        if (start <= tokenId.size())
        {
            parts[idx] = AZStd::string(tokenId.data() + start, tokenId.size() - start);
        }
    }

    // ---- Var token helpers ----

    inline bool IsVarToken(const AZStd::string& token)
    {
        return token.size() > VAR_PREFIX_LEN + 1 &&
               token.starts_with(VAR_PREFIX) &&
               token[VAR_PREFIX_LEN] == ' ';
    }

    inline AZStd::string VarSurface(const AZStd::string& token)
    {
        return AZStd::string(token.data() + VAR_PREFIX_LEN + 1,
                             token.size() - VAR_PREFIX_LEN - 1);
    }

    // ---- Unicode-aware alpha check ----

    inline bool IsAlphaCodepoint(AZ::u32 cp)
    {
        // ASCII
        if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return true;
        // Latin Extended (accented: cafe, naive, resume, etc.)
        if (cp >= 0x00C0 && cp <= 0x024F) return true;
        // IPA Extensions
        if (cp >= 0x0250 && cp <= 0x02AF) return true;
        // Greek and Coptic
        if (cp >= 0x0370 && cp <= 0x03FF) return true;
        // Cyrillic
        if (cp >= 0x0400 && cp <= 0x052F) return true;
        // Armenian
        if (cp >= 0x0530 && cp <= 0x058F) return true;
        // Hebrew
        if (cp >= 0x0590 && cp <= 0x05FF) return true;
        // Arabic
        if (cp >= 0x0600 && cp <= 0x06FF) return true;
        // Devanagari, Bengali, other Indic
        if (cp >= 0x0900 && cp <= 0x0DFF) return true;
        // Thai
        if (cp >= 0x0E00 && cp <= 0x0E7F) return true;
        // CJK and beyond — treat as alpha
        if (cp >= 0x3000) return true;
        return false;
    }

    // Classify a docvar surface form into a category.
    // Detection order: uri_metadata → sic → proper → lingo (first match wins).
    // Unicode-aware: non-ASCII alpha characters (accented Latin, etc.) are NOT sic.
    inline const char* ClassifyVar(const AZStd::string& surface)
    {
        if (surface.empty()) return "lingo";

        // uri_metadata: starts with http://, https://, www., or contains ://
        if (surface.starts_with("http://") || surface.starts_with("https://") ||
            surface.starts_with("www.") || surface.find("://") != AZStd::string::npos)
        {
            return "uri_metadata";
        }

        // Decode UTF-8 to codepoints for Unicode-aware checks
        AZStd::vector<AZ::u32> codepoints;
        {
            const auto* p = reinterpret_cast<const unsigned char*>(surface.data());
            const auto* end = p + surface.size();
            while (p < end)
            {
                AZ::u32 cp = 0;
                if (*p < 0x80)
                {
                    cp = *p++;
                }
                else if ((*p & 0xE0) == 0xC0 && p + 1 < end)
                {
                    cp = (*p & 0x1F) << 6 | (*(p + 1) & 0x3F);
                    p += 2;
                }
                else if ((*p & 0xF0) == 0xE0 && p + 2 < end)
                {
                    cp = (*p & 0x0F) << 12 | (*(p + 1) & 0x3F) << 6 | (*(p + 2) & 0x3F);
                    p += 3;
                }
                else if ((*p & 0xF8) == 0xF0 && p + 3 < end)
                {
                    cp = (*p & 0x07) << 18 | (*(p + 1) & 0x3F) << 12 |
                         (*(p + 2) & 0x3F) << 6 | (*(p + 3) & 0x3F);
                    p += 4;
                }
                else
                {
                    ++p; // skip invalid
                    continue;
                }
                codepoints.push_back(cp);
            }
        }

        // number: contains any digit codepoint
        for (AZ::u32 cp : codepoints)
        {
            if (cp >= '0' && cp <= '9')
                return "number";
        }

        // sic: contains non-alpha/non-space/non-hyphen/non-apostrophe codepoints
        for (AZ::u32 cp : codepoints)
        {
            if (!(IsAlphaCodepoint(cp) || cp == ' ' || cp == '-' || cp == '\''))
                return "sic";
        }

        // proper: first codepoint is uppercase (ASCII uppercase or Latin Extended uppercase)
        if (!codepoints.empty())
        {
            AZ::u32 first = codepoints[0];
            if ((first >= 'A' && first <= 'Z') ||
                (first >= 0x00C0 && first <= 0x00D6) ||
                (first >= 0x00D8 && first <= 0x00DE) ||
                (first >= 0x0100 && first <= 0x024F && (first % 2 == 0)))
            {
                return "proper";
            }
        }

        return "lingo";
    }

    // ---- Sparse modifier encoding ----

    // Encode non-zero modifiers as sparse [position_b50(4) + modifier_b50(4)] pairs.
    // Returns empty string if no non-zero modifiers found for this token.
    inline AZStd::string EncodeSparseModifiers(
        const AZStd::vector<int>& posList,
        const AZStd::vector<AZ::u32>& allModifiers,
        const AZStd::vector<int>& allPositions,
        const AZStd::vector<AZStd::string>& allTokenIds,
        const AZStd::string& tokenId)
    {
        if (allModifiers.empty()) return {};

        AZStd::string encoded;
        for (int pos : posList)
        {
            for (size_t i = 0; i < allTokenIds.size(); ++i)
            {
                if (allTokenIds[i] == tokenId && allPositions[i] == pos)
                {
                    if (i < allModifiers.size() && allModifiers[i] != 0)
                    {
                        char posBuf[4];
                        EncodePosition(pos, posBuf);
                        char modBuf[4];
                        EncodePosition(static_cast<int>(allModifiers[i]), modBuf);
                        encoded.append(posBuf, 4);
                        encoded.append(modBuf, 4);
                    }
                    break;
                }
            }
        }
        return encoded;
    }

} // namespace HCPEngine
