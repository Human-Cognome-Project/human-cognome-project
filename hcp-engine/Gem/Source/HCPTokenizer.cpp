#include "HCPTokenizer.h"
#include "HCPVocabulary.h"

#include <AzCore/Console/ILogger.h>
#include <cctype>
#include <chrono>

namespace HCPEngine
{
    // ---- Phase transition: typesetting normalization ----
    // Normalize typesetting variants (curly quotes, BOM, TM) to DB forms.
    // Em/en-dashes are preserved — they're structural separators, not
    // typesetting variants. Handled by the dash split step.

    static AZStd::string NormalizeTypesetting(const AZStd::string& text)
    {
        AZStd::string out;
        out.reserve(text.size());

        const uint8_t* p = reinterpret_cast<const uint8_t*>(text.c_str());
        const uint8_t* end = p + text.size();

        while (p < end)
        {
            // 3-byte UTF-8 sequences: E2 xx xx
            if (p + 2 < end && p[0] == 0xE2)
            {
                if (p[1] == 0x80)
                {
                    switch (p[2])
                    {
                    case 0x98: // U+2018 LEFT SINGLE QUOTATION MARK
                    case 0x99: // U+2019 RIGHT SINGLE QUOTATION MARK
                        out += '\'';
                        p += 3;
                        continue;
                    case 0x9C: // U+201C LEFT DOUBLE QUOTATION MARK
                    case 0x9D: // U+201D RIGHT DOUBLE QUOTATION MARK
                        out += '"';
                        p += 3;
                        continue;
                    // Em/en-dashes preserved — structural, not typesetting
                    default:
                        break;
                    }
                }
            }

            // TM symbol: E2 84 A2 (U+2122) — strip (typesetting artifact)
            if (p + 2 < end && p[0] == 0xE2 && p[1] == 0x84 && p[2] == 0xA2)
            {
                p += 3;
                continue;
            }

            // BOM: EF BB BF (U+FEFF) — strip
            if (p + 2 < end && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
            {
                p += 3;
                continue;
            }

            out += static_cast<char>(*p);
            ++p;
        }

        return out;
    }

    // ---- Classification helpers ----

    static bool IsWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static bool IsPunctuation(char c)
    {
        // ASCII punctuation only. Multi-byte UTF-8 (em/en-dash etc.) excluded
        // so edge stripping doesn't corrupt UTF-8 sequences.
        unsigned char uc = static_cast<unsigned char>(c);
        return uc < 128 && !std::isalnum(uc) && !IsWhitespace(c);
    }

    static bool IsAlpha(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) != 0;
    }

    static AZStd::string ToLower(const AZStd::string& s)
    {
        AZStd::string lower(s);
        for (size_t i = 0; i < lower.size(); ++i)
        {
            unsigned char uc = static_cast<unsigned char>(lower[i]);
            if (std::isupper(uc))
                lower[i] = static_cast<char>(std::tolower(uc));
        }
        return lower;
    }

    // ---- Dash detection ----
    // Finds hyphen (-), em-dash (U+2014), or en-dash (U+2013) within a string.
    // Hyphen connects meaning. Em/en-dash is structural (replaces spacing).
    // All three are split points for the lookup stack.

    enum class DashType { None, Hyphen, EmDash, EnDash };

    struct DashSplit
    {
        size_t pos;    // byte offset of separator
        size_t len;    // byte length (1 for hyphen, 3 for em/en-dash)
        DashType type;
    };

    static DashSplit FindDash(const AZStd::string& s)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(s.c_str());
        size_t sz = s.size();

        for (size_t i = 0; i < sz; ++i)
        {
            // Em-dash: E2 80 94
            if (i + 2 < sz && p[i] == 0xE2 && p[i+1] == 0x80 && p[i+2] == 0x94)
                return { i, 3, DashType::EmDash };

            // En-dash: E2 80 93
            if (i + 2 < sz && p[i] == 0xE2 && p[i+1] == 0x80 && p[i+2] == 0x93)
                return { i, 3, DashType::EnDash };

            // Hyphen: only internal (not at edges — that's punct stripping)
            if (p[i] == 0x2D && i > 0 && i + 1 < sz)
                return { i, 1, DashType::Hyphen };
        }

        return { 0, 0, DashType::None };
    }

    static AZStd::string ResolveDashToken(
        DashType type,
        [[maybe_unused]] const AZStd::string& dashStr,
        const HCPVocabulary& vocab)
    {
        if (type == DashType::Hyphen)
            return vocab.LookupChar('-');

        // Em/en-dash: registered as "emdash"/"endash" in the word table
        if (type == DashType::EmDash)
            return vocab.LookupWord("emdash");
        else
            return vocab.LookupWord("endash");
    }

    // ---- Core resolution (steps 2-5) ----
    // Resolves an input string to one or more token IDs.
    // Handles its own edge punct stripping so recursive dash splits work.
    //
    // Step 2: core lowercase word lookup
    // Step 3: core exact-case word lookup
    // Step 4: possessive morpheme split ('s / s')
    // Step 5: recursive dash split

    static bool ResolveCore(
        const AZStd::string& input,
        const HCPVocabulary& vocab,
        AZStd::vector<AZStd::string>& ids,
        AZStd::unordered_map<AZStd::string, AZStd::string>* varCache = nullptr)
    {
        if (input.empty()) return false;

        // Strip ASCII edge punctuation
        size_t leadEnd = 0;
        while (leadEnd < input.size() && IsPunctuation(input[leadEnd]))
            ++leadEnd;

        size_t trailStart = input.size();
        while (trailStart > leadEnd && IsPunctuation(input[trailStart - 1]))
            --trailStart;

        // Resolve leading punct chars
        AZStd::vector<AZStd::string> leadIds;
        for (size_t j = 0; j < leadEnd; ++j)
        {
            AZStd::string cid = vocab.LookupChar(input[j]);
            if (cid.empty()) return false;
            leadIds.push_back(cid);
        }

        // Resolve trailing punct chars
        AZStd::vector<AZStd::string> trailIds;
        for (size_t j = trailStart; j < input.size(); ++j)
        {
            AZStd::string cid = vocab.LookupChar(input[j]);
            if (cid.empty()) return false;
            trailIds.push_back(cid);
        }

        // All punctuation, no core
        if (trailStart <= leadEnd)
        {
            for (const auto& id : leadIds)  ids.push_back(id);
            for (const auto& id : trailIds) ids.push_back(id);
            return !ids.empty();
        }

        AZStd::string core(input.data() + leadEnd, trailStart - leadEnd);

        // Check var cache — if this core was var'd before, reuse immediately
        AZStd::string coreLower = ToLower(core);
        if (varCache)
        {
            auto cacheIt = varCache->find(coreLower);
            if (cacheIt != varCache->end())
            {
                for (const auto& id : leadIds)  ids.push_back(id);
                ids.push_back(cacheIt->second);
                for (const auto& id : trailIds) ids.push_back(id);
                return true;
            }
        }

        // Step 2: core lowercase
        AZStd::string tid = vocab.LookupWord(coreLower);

        // Step 3: core exact case
        if (tid.empty() && coreLower != core)
            tid = vocab.LookupWord(core);

        // Single char fallback
        if (tid.empty() && core.size() == 1)
            tid = vocab.LookupChar(core[0]);

        if (!tid.empty())
        {
            for (const auto& id : leadIds)  ids.push_back(id);
            ids.push_back(tid);
            for (const auto& id : trailIds) ids.push_back(id);
            return true;
        }

        // Step 4: Morpheme decomposition (affix scan)
        // Try all registered suffixes then prefixes, longest first (greedy).
        // Subsumes possessive split ('s, s') and handles -ing, -ed, un-, re-, etc.
        //
        // Suffixes: core ends with stripped form → stem = core[0:N-sfxLen]
        // Prefixes: core starts with stripped form → stem = core[pfxLen:]

        // Suffixes — bucket lookup by word's last char, full resolver on stem
        {
            char lastChar = core.back();
            // Try lowercase key first (most affixes are lowercase)
            const auto* bucket = vocab.GetSuffixesForChar(
                static_cast<char>(std::tolower(static_cast<unsigned char>(lastChar))));
            if (!bucket)
                bucket = vocab.GetSuffixesForChar(lastChar);

            if (bucket)
            {
                for (const auto& sfx : *bucket)
                {
                    if (core.size() <= sfx.stripped.size()) continue;

                    size_t stemLen = core.size() - sfx.stripped.size();
                    if (core.compare(stemLen, sfx.stripped.size(), sfx.stripped) != 0)
                        continue;

                    AZStd::string stem(core.data(), stemLen);
                    AZStd::string stemLower = ToLower(stem);

                    // Check var cache for stem — a previously var'd word decomposes here
                    AZStd::string stemTid;
                    if (varCache)
                    {
                        auto cacheIt = varCache->find(stemLower);
                        if (cacheIt != varCache->end())
                            stemTid = cacheIt->second;
                    }
                    // Full LookupWord — triggers resolver/Postgres on LMDB miss.
                    // Cost is nominal (once per unique stem, then cached in LMDB).
                    if (stemTid.empty())
                        stemTid = vocab.LookupWord(stemLower);
                    if (stemTid.empty() && stemLower != stem)
                        stemTid = vocab.LookupWord(stem);

                    if (!stemTid.empty())
                    {
                        for (const auto& id : leadIds)  ids.push_back(id);
                        ids.push_back(stemTid);
                        ids.push_back(sfx.tokenId);
                        for (const auto& id : trailIds) ids.push_back(id);
                        return true;
                    }
                }
            }
        }

        // Prefixes — bucket lookup by word's first char, full resolver on stem
        {
            char firstChar = core[0];
            const auto* bucket = vocab.GetPrefixesForChar(
                static_cast<char>(std::tolower(static_cast<unsigned char>(firstChar))));
            if (!bucket)
                bucket = vocab.GetPrefixesForChar(firstChar);

            if (bucket)
            {
                for (const auto& pfx : *bucket)
                {
                    if (core.size() <= pfx.stripped.size()) continue;

                    if (core.compare(0, pfx.stripped.size(), pfx.stripped) != 0)
                        continue;

                    AZStd::string stem(core.data() + pfx.stripped.size(),
                                       core.size() - pfx.stripped.size());
                    AZStd::string stemLower = ToLower(stem);

                    // Check var cache for stem
                    AZStd::string stemTid;
                    if (varCache)
                    {
                        auto cacheIt = varCache->find(stemLower);
                        if (cacheIt != varCache->end())
                            stemTid = cacheIt->second;
                    }
                    if (stemTid.empty())
                        stemTid = vocab.LookupWord(stemLower);
                    if (stemTid.empty() && stemLower != stem)
                        stemTid = vocab.LookupWord(stem);

                    if (!stemTid.empty())
                    {
                        for (const auto& id : leadIds)  ids.push_back(id);
                        ids.push_back(pfx.tokenId);
                        ids.push_back(stemTid);
                        for (const auto& id : trailIds) ids.push_back(id);
                        return true;
                    }
                }
            }
        }

        // Step 5: dash/hyphen split on core — recursive on each part
        // Accept partial: resolved parts emit as tokens, unresolved parts
        // become individual var requests.
        DashSplit ds = FindDash(core);
        if (ds.type != DashType::None)
        {
            AZStd::string left(core.data(), ds.pos);
            AZStd::string dashStr(core.data() + ds.pos, ds.len);
            AZStd::string right(core.data() + ds.pos + ds.len,
                                core.size() - ds.pos - ds.len);

            AZStd::string dashTid = ResolveDashToken(ds.type, dashStr, vocab);
            if (dashTid.empty()) return false;

            AZStd::vector<AZStd::string> leftIds, rightIds;
            bool leftOk  = left.empty()  || ResolveCore(left, vocab, leftIds, varCache);
            bool rightOk = right.empty() || ResolveCore(right, vocab, rightIds, varCache);

            // Var any unresolved parts — and cache them
            if (!leftOk && !left.empty())
            {
                leftIds.clear();
                AZStd::string varReq = VAR_REQUEST;
                varReq += ' ';
                varReq += left;
                leftIds.push_back(varReq);
                if (varCache)
                    (*varCache)[ToLower(left)] = varReq;
            }
            if (!rightOk && !right.empty())
            {
                rightIds.clear();
                AZStd::string varReq = VAR_REQUEST;
                varReq += ' ';
                varReq += right;
                rightIds.push_back(varReq);
                if (varCache)
                    (*varCache)[ToLower(right)] = varReq;
            }

            for (const auto& id : leadIds)   ids.push_back(id);
            for (const auto& id : leftIds)   ids.push_back(id);
            ids.push_back(dashTid);
            for (const auto& id : rightIds)  ids.push_back(id);
            for (const auto& id : trailIds)  ids.push_back(id);
            return true;
        }

        return false;
    }

    // ---- Greedy word walk (missing space detection) ----
    // Try splitting an alphabetic sequence into known words.

    static bool TryGreedyWalk(
        const AZStd::string& chunk,
        const HCPVocabulary& vocab,
        TokenStream& stream,
        AZ::u32& slotPos)
    {
        // Only attempt on purely alphabetic chunks
        for (size_t i = 0; i < chunk.size(); ++i)
        {
            if (!IsAlpha(chunk[i])) return false;
        }

        if (chunk.size() < 2) return false;

        AZStd::string lower = ToLower(chunk);

        AZStd::vector<AZStd::string> foundIds;
        size_t pos = 0;

        while (pos < lower.size())
        {
            bool matched = false;

            for (size_t len = lower.size() - pos; len > 0; --len)
            {
                AZStd::string candidate(lower.data() + pos, len);
                AZStd::string tid = vocab.LookupWord(candidate);

                if (!tid.empty())
                {
                    size_t remaining = lower.size() - pos - len;
                    if (remaining == 0)
                    {
                        foundIds.push_back(tid);
                        pos += len;
                        matched = true;
                        break;
                    }

                    AZStd::string remainder(lower.data() + pos + len, remaining);
                    AZStd::string remTid = vocab.LookupWord(remainder);
                    if (!remTid.empty())
                    {
                        foundIds.push_back(tid);
                        foundIds.push_back(remTid);
                        pos = lower.size();
                        matched = true;
                        break;
                    }

                    foundIds.push_back(tid);
                    pos += len;
                    matched = true;
                    break;
                }
            }

            if (!matched)
                return false;
        }

        for (const auto& tid : foundIds)
        {
            stream.tokenIds.push_back(tid);
            stream.positions.push_back(slotPos++);
        }
        return true;
    }

    // Vector-output overload: returns IDs without emitting to stream.
    // Used when greedy walk is tried on a core after edge punct stripping.
    static bool TryGreedyWalk(
        const AZStd::string& chunk,
        const HCPVocabulary& vocab,
        AZStd::vector<AZStd::string>& outIds)
    {
        for (size_t i = 0; i < chunk.size(); ++i)
        {
            if (!IsAlpha(chunk[i])) return false;
        }
        if (chunk.size() < 2) return false;

        AZStd::string lower = ToLower(chunk);
        size_t pos = 0;

        while (pos < lower.size())
        {
            bool matched = false;
            for (size_t len = lower.size() - pos; len > 0; --len)
            {
                AZStd::string candidate(lower.data() + pos, len);
                AZStd::string tid = vocab.LookupWord(candidate);
                if (!tid.empty())
                {
                    size_t remaining = lower.size() - pos - len;
                    if (remaining == 0)
                    {
                        outIds.push_back(tid);
                        pos += len;
                        matched = true;
                        break;
                    }
                    AZStd::string remainder(lower.data() + pos + len, remaining);
                    AZStd::string remTid = vocab.LookupWord(remainder);
                    if (!remTid.empty())
                    {
                        outIds.push_back(tid);
                        outIds.push_back(remTid);
                        pos = lower.size();
                        matched = true;
                        break;
                    }
                    outIds.push_back(tid);
                    pos += len;
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }
        return true;
    }

    // ---- Var DB handoff ----

    static size_t s_varDebugCount = 0;

    static void HandoffToVarDb(
        const AZStd::string& chunk,
        [[maybe_unused]] const HCPVocabulary& vocab,
        TokenStream& stream,
        AZ::u32& slotPos,
        AZStd::unordered_map<AZStd::string, AZStd::string>* varCache = nullptr)
    {
        if (s_varDebugCount < 50)
        {
            fprintf(stderr, "[Tokenizer VAR] unresolved: \"%s\"\n", chunk.c_str());
            fflush(stderr);
            ++s_varDebugCount;
        }

        AZStd::string request = VAR_REQUEST;
        request += ' ';
        request += chunk;

        stream.tokenIds.push_back(request);
        stream.positions.push_back(slotPos++);

        // Cache so subsequent identical chunks resolve instantly
        if (varCache)
            (*varCache)[ToLower(chunk)] = request;
    }

    // ---- Main tokenizer ----
    //
    // Lookup stack per chunk:
    //   1. Lowercase space-to-space (fast path, most common)
    //   2. Strip edge punct, core lowercase
    //   3. Core exact case (I'm, proper nouns)
    //   4. Dash/hyphen split, check each part (recursive)
    //   5. Greedy word walk (missing spaces)
    //   6. Var DB handoff (unresolved)

    TokenStream Tokenize(const AZStd::string& text, const HCPVocabulary& vocab)
    {
        AZStd::string normalized = NormalizeTypesetting(text);

        TokenStream stream;
        stream.tokenIds.reserve(normalized.size() / 4);
        stream.positions.reserve(normalized.size() / 4);

        // Per-document var cache: lowercase form → VAR_REQUEST token string.
        // Once a chunk vars, every subsequent identical chunk resolves instantly.
        AZStd::unordered_map<AZStd::string, AZStd::string> varCache;

        AZ::u32 slotPos = 0;
        s_varDebugCount = 0;  // Reset per-tokenize debug counter

        size_t i = 0;
        while (i < normalized.size())
        {
            char c = normalized[i];

            if (c == ' ')
            {
                slotPos++;
                ++i;
                continue;
            }

            if (c == '\n')
            {
                AZStd::string nid = vocab.LookupLabel("newline");
                if (!nid.empty())
                {
                    stream.tokenIds.push_back(nid);
                    stream.positions.push_back(slotPos++);
                }
                else
                {
                    slotPos++;
                }
                ++i;
                continue;
            }

            if (c == '\r')
            {
                ++i;
                continue;
            }

            if (c == '\t')
            {
                slotPos++;
                ++i;
                continue;
            }

            // Collect chunk (everything until next whitespace)
            size_t chunkStart = i;
            while (i < normalized.size() && !IsWhitespace(normalized[i]))
                ++i;

            AZStd::string chunk(normalized.data() + chunkStart, i - chunkStart);

            // ==== FAST PATH: Dot-separated values → var as unit ====
            // Initialisms (U.S.), section numbers (1.E.8), URLs (www.gutenberg.org)
            // all contain internal periods. Skip the resolution stack entirely.
            {
                bool hasDotValue = false;
                for (size_t j = 1; j + 1 < chunk.size(); ++j)
                {
                    if (chunk[j] == '.' &&
                        std::isalnum(static_cast<unsigned char>(chunk[j-1])) &&
                        std::isalnum(static_cast<unsigned char>(chunk[j+1])))
                    {
                        hasDotValue = true;
                        break;
                    }
                }

                if (hasDotValue)
                {
                    // Strip edge punct, var the core, emit punct chars
                    size_t le = 0;
                    while (le < chunk.size() && IsPunctuation(chunk[le]) && chunk[le] != '.')
                        ++le;
                    size_t ts = chunk.size();
                    while (ts > le && IsPunctuation(chunk[ts - 1]) && chunk[ts - 1] != '.')
                        --ts;

                    // Emit leading punct
                    for (size_t j = 0; j < le; ++j)
                    {
                        AZStd::string cid = vocab.LookupChar(chunk[j]);
                        if (!cid.empty())
                        {
                            stream.tokenIds.push_back(cid);
                            stream.positions.push_back(slotPos++);
                        }
                    }

                    // Var the dot-separated core (check cache first)
                    AZStd::string core(chunk.data() + le, ts - le);
                    AZStd::string coreLower = ToLower(core);
                    auto dotCacheIt = varCache.find(coreLower);
                    AZStd::string varReq;
                    if (dotCacheIt != varCache.end())
                    {
                        varReq = dotCacheIt->second;
                    }
                    else
                    {
                        varReq = VAR_REQUEST;
                        varReq += ' ';
                        varReq += core;
                        varCache[coreLower] = varReq;
                        if (s_varDebugCount < 50)
                        {
                            fprintf(stderr, "[Tokenizer VAR] dot-value: \"%s\"\n", core.c_str());
                            fflush(stderr);
                            ++s_varDebugCount;
                        }
                    }
                    stream.tokenIds.push_back(varReq);
                    stream.positions.push_back(slotPos++);

                    // Emit trailing punct
                    for (size_t j = ts; j < chunk.size(); ++j)
                    {
                        AZStd::string cid = vocab.LookupChar(chunk[j]);
                        if (!cid.empty())
                        {
                            stream.tokenIds.push_back(cid);
                            stream.positions.push_back(slotPos++);
                        }
                    }
                    continue;
                }
            }

            AZStd::string lower = ToLower(chunk);

            // ==== VAR CACHE CHECK: previously var'd chunk resolves instantly ====
            {
                auto cacheIt = varCache.find(lower);
                if (cacheIt != varCache.end())
                {
                    stream.tokenIds.push_back(cacheIt->second);
                    stream.positions.push_back(slotPos++);
                    continue;
                }
            }

            // ==== STEP 1: Lowercase space-to-space ====
            AZStd::string tid = vocab.LookupWord(lower);
            if (tid.empty() && chunk.size() == 1)
                tid = vocab.LookupChar(chunk[0]);

            if (!tid.empty())
            {
                // Forward walk: boilerplate detection
                {
                    AZStd::string accumulated = chunk;
                    size_t peekPos = i;

                    bool walking = true;
                    while (walking && peekPos < normalized.size())
                    {
                        size_t nextStart = peekPos;
                        while (nextStart < normalized.size() && IsWhitespace(normalized[nextStart]))
                            ++nextStart;
                        if (nextStart >= normalized.size()) break;

                        size_t nextEnd = nextStart;
                        while (nextEnd < normalized.size() && !IsWhitespace(normalized[nextEnd]))
                            ++nextEnd;
                        AZStd::string nextChunk(normalized.data() + nextStart, nextEnd - nextStart);

                        ContinuationResult cr = vocab.CheckContinuation(accumulated, nextChunk);

                        if (cr.IsComplete())
                        {
                            stream.tokenIds.push_back(cr.sequenceId);
                            stream.positions.push_back(slotPos++);
                            i = nextEnd;
                            goto next_chunk;
                        }
                        else if (cr.IsContinue())
                        {
                            accumulated += ' ';
                            accumulated += nextChunk;
                            peekPos = nextEnd;
                        }
                        else
                        {
                            walking = false;
                        }
                    }
                }

                stream.tokenIds.push_back(tid);
                stream.positions.push_back(slotPos++);
                continue;
            }

            // ==== STEPS 2-5: Edge punct strip + core resolution ====
            {
                // Compute edge punct bounds (ASCII only)
                size_t leadEnd = 0;
                while (leadEnd < chunk.size() && IsPunctuation(chunk[leadEnd]))
                    ++leadEnd;

                size_t trailStart = chunk.size();
                while (trailStart > leadEnd && IsPunctuation(chunk[trailStart - 1]))
                    --trailStart;

                bool hasCore = trailStart > leadEnd;
                bool hasEdgePunct = (leadEnd > 0 || trailStart < chunk.size());

                if (hasCore)
                {
                    AZStd::string core(chunk.data() + leadEnd, trailStart - leadEnd);
                    AZStd::vector<AZStd::string> coreIds;
                    bool coreResolved = ResolveCore(core, vocab, coreIds, &varCache);

                    // If core didn't resolve via stack, try greedy walk on it
                    if (!coreResolved)
                        coreResolved = TryGreedyWalk(core, vocab, coreIds);

                    // If core still unresolved, var just the core and cache it
                    if (!coreResolved)
                    {
                        AZStd::string varReq = VAR_REQUEST;
                        varReq += ' ';
                        varReq += core;
                        coreIds.push_back(varReq);
                        varCache[ToLower(core)] = varReq;
                        if (s_varDebugCount < 50)
                        {
                            fprintf(stderr, "[Tokenizer VAR] unresolved: \"%s\"\n", core.c_str());
                            fflush(stderr);
                            ++s_varDebugCount;
                        }
                    }

                    // Emit leading punct chars
                    for (size_t j = 0; j < leadEnd; ++j)
                    {
                        AZStd::string cid = vocab.LookupChar(chunk[j]);
                        if (!cid.empty())
                        {
                            stream.tokenIds.push_back(cid);
                            stream.positions.push_back(slotPos++);
                        }
                    }

                    // Emit core tokens
                    for (const auto& id : coreIds)
                    {
                        stream.tokenIds.push_back(id);
                        stream.positions.push_back(slotPos++);
                    }

                    // Emit trailing punct chars
                    for (size_t j = trailStart; j < chunk.size(); ++j)
                    {
                        AZStd::string cid = vocab.LookupChar(chunk[j]);
                        if (!cid.empty())
                        {
                            stream.tokenIds.push_back(cid);
                            stream.positions.push_back(slotPos++);
                        }
                    }
                    continue;
                }
                else if (hasEdgePunct)
                {
                    // All ASCII punctuation, no word core — emit each char
                    bool allResolved = true;
                    for (size_t j = 0; j < chunk.size(); ++j)
                    {
                        AZStd::string cid = vocab.LookupChar(chunk[j]);
                        if (cid.empty()) { allResolved = false; break; }
                        stream.tokenIds.push_back(cid);
                        stream.positions.push_back(slotPos++);
                    }
                    if (allResolved)
                        continue;
                }
            }

            // ==== STEP 6: Greedy word walk (no edge punct case) ====
            if (TryGreedyWalk(chunk, vocab, stream, slotPos))
                continue;

            // ==== STEP 7: Var DB handoff ====
            HandoffToVarDb(chunk, vocab, stream, slotPos, &varCache);

            next_chunk:;
        }

        stream.totalSlots = slotPos;

        // Count actual var request tokens in stream (definitive count)
        size_t totalVars = 0;
        for (const auto& tid : stream.tokenIds)
        {
            if (tid.starts_with(VAR_REQUEST))
                ++totalVars;
        }

        fprintf(stderr, "[HCPTokenizer] %zu chars -> %zu tokens, %u slots, %zu var requests\n",
            normalized.size(), stream.tokenIds.size(), stream.totalSlots, totalVars);
        fflush(stderr);

        AZLOG_INFO("HCPTokenizer: %zu chars -> %zu tokens, %u slots, %zu vars",
            normalized.size(), stream.tokenIds.size(), stream.totalSlots, totalVars);
        return stream;
    }

} // namespace HCPEngine
