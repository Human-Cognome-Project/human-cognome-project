#include "HCPByteIngest.h"
#include "HCPVocabBed.h"   // BedManager

#include "../../../tools/byte-floor/bytefloor.h"

#include <cctype>

namespace HCPEngine
{
    namespace
    {
        using Elem = hcp::bytefloor::Elem;

        bool IsWs(uint32_t c)    { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
        bool IsPunct(uint32_t c) { return c < 0x80 && std::ispunct(static_cast<int>(c)) != 0; }

        void AppendUtf8(uint32_t cp, AZStd::string& out)
        {
            if      (cp < 0x80)    out += char(cp);
            else if (cp < 0x800)   { out += char(0xC0|(cp>>6));  out += char(0x80|(cp&0x3F)); }
            else if (cp < 0x10000) { out += char(0xE0|(cp>>12)); out += char(0x80|((cp>>6)&0x3F)); out += char(0x80|(cp&0x3F)); }
            else                   { out += char(0xF0|(cp>>18)); out += char(0x80|((cp>>12)&0x3F)); out += char(0x80|((cp>>6)&0x3F)); out += char(0x80|(cp&0x3F)); }
        }

        bool IsCodepoint(const Elem& e) { return e.kind == Elem::Codepoint; }
    }

    AZStd::vector<CharRun> IngestBytes(const uint8_t* data, size_t len)
    {
        AZStd::vector<CharRun> runs;

        // Stage 1: bytes -> positioned characters (the byte-floor, replacing PhysX Phase-1).
        hcp::bytefloor::Result r = hcp::bytefloor::resolve(data, len);
        const auto& E = r.elems;
        const AZ::u32 n = static_cast<AZ::u32>(E.size());

        // Stage 2: segment the character stream into runs (whitespace-delimited, edge-punct
        // stripped, lowercased), carrying the byte span up from the byte-floor positional map.
        AZ::u32 i = 0;
        while (i < n)
        {
            while (i < n && IsCodepoint(E[i]) && IsWs(E[i].value)) ++i;          // skip whitespace
            if (i >= n) break;

            AZ::u32 chunkStart = i;
            while (i < n && !(IsCodepoint(E[i]) && IsWs(E[i].value))) ++i;        // collect to next whitespace

            AZ::u32 cs = chunkStart, ce = i;                                      // strip edge punctuation
            while (cs < ce && IsCodepoint(E[cs])   && IsPunct(E[cs].value))   ++cs;
            while (ce > cs && IsCodepoint(E[ce-1]) && IsPunct(E[ce-1].value)) --ce;
            if (ce <= cs) continue;

            CharRun run;
            run.startPos  = cs;                                                   // character (codepoint) position
            run.length    = ce - cs;
            run.byteStart = E[cs].srcOffset;                                      // positional map: source bytes
            run.byteLen   = E[ce-1].srcOffset + E[ce-1].srcLen - E[cs].srcOffset;

            AZStd::string core;
            core.reserve(ce - cs);
            for (AZ::u32 j = cs; j < ce; ++j)
            {
                const uint32_t v = E[j].value;
                if (IsCodepoint(E[j]) && v < 0x80)
                {
                    const unsigned char uc = static_cast<unsigned char>(v);
                    if (std::isupper(uc)) { run.capMask.push_back(j - cs); if (j == cs) run.firstCap = true; }
                    core += static_cast<char>(std::tolower(uc));
                }
                else if (IsCodepoint(E[j])) { AppendUtf8(v, core); }              // non-ASCII: carried, not dropped
                else                        { core += static_cast<char>(v & 0xFF); } // residue byte: carried
            }
            run.allCaps = (!run.capMask.empty() && run.capMask.size() == run.length);
            run.text    = AZStd::move(core);
            run.tag     = RunTag::Word;
            runs.push_back(AZStd::move(run));
        }

        return runs;
    }

    ResolutionManifest ResolveBytes(BedManager& bed, const uint8_t* data, size_t len)
    {
        // bytes -> characters -> words: the byte-floor and the chambers, talking.
        return bed.Resolve(IngestBytes(data, len));
    }
}
