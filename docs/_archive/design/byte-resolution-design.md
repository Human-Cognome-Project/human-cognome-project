# Byte Resolution — Universal Encoding Design (2026-02-27)

## Core Insight

Normalize ALL encoding reference tables AND all incoming bytes to 4 bytes (zero-padded). This is not a hack — every codepoint fits in 4 bytes; variable-length encodings just omit leading zeros. Everything is always 4 bytes — input routine normalizes on ingest, DB stores 4-byte form.

## Why This Works

- Every byte on every encoding table has a deterministic prefix (`0x`) — it's how bytes are addressed
- UTF-8, UTF-16, UTF-32 are just different serializations of the same codepoint space
- 4-byte normalization makes all tables directly comparable in a single particle space
- ASCII range (0x00000000–0x0000007F) is identical across ALL encoding tables — one shared static particle set covers the universal common case

## 4-Byte Normalization

- **All reference tables in DB**: stored as 4-byte zero-padded codepoints (DB specialist task)
- **All incoming bytes**: normalized to 4 bytes at input time (input routine or processing loop — whichever is faster)
- No variable-length handling in the physics — everything is uniform 4-byte particles
- This is the canonical form for both storage and resolution

## Phased Resolution: Encoding Table Elimination

Phasing is NOT by byte length (everything is 4 bytes). Phasing is by encoding table candidacy.

### Phase 1: Universal shared codepoints
- ASCII range is identical across all encoding tables — resolves with one shared reference set
- Covers ~99% of English text input
- No table selection needed

### Phase 2: Conflict zone (0x80–0x9F) — table elimination by pattern
- Only ~32 codepoint values differ between major Western tables
- Deterministic elimination (not probabilistic):
  - ISO-8859-1 has NO printable characters in 0x80–0x9F → any content byte in that range eliminates ISO
  - Lone 0x80–0x9F byte surrounded by ASCII → Windows-1252 (in original stream, continuation bytes never appear standalone in UTF-8)
  - Valid continuation pattern in original stream → UTF-8
- Physics resolves remaining ambiguity — unsettled 4-byte particles tested against surviving candidate tables only

### Phase 3: Extended range (0xA0–0xFF) if needed
- ISO-8859-1 and Windows-1252 agree here (accented Latin)
- Other ISO-8859 variants (8859-2 Central European, 8859-5 Cyrillic) diverge
- Only runs if Phase 2 didn't fully resolve

## File-Type Priors

- File type determines which encoding tables AND which language tables to load as candidates
- `.cpp`, `.py`, `.js` → ASCII-dominant structure, language tables unnecessary
- `.doc`, legacy Windows `.txt` → Windows-1252 likely
- `.html` → may declare charset in headers
- Linux-originated `.txt` → UTF-8 almost certain
- **Key distinction**: file type informs LANGUAGE table selection (don't load CJK for a `.cpp`), not just encoding
- If everything settles → done. Unsettled bytes → expand to next most likely tables. Same cascade as Phase 2 tier resolution.

## Encoding Detection Is Free

- Whichever table's codepoints the ambiguous bytes settle against = the detected encoding
- No heuristic guessing — the physics IS the detection
- Persistent non-settlement = unknown/corrupt encoding (var-wrap equivalent for bytes)

## Particle Count Impact

- Uniform 4-byte particles everywhere — no variable-length special cases
- Shared ASCII reference covers all tables simultaneously
- Table-specific particles only needed for the ~32-value conflict zone
- Dramatic reduction vs. loading full per-table reference sets

## Hyphen Resolution Cascade (Phase 2)

Hyphenated sequences get a three-step cascade within word resolution:

1. **Full hyphenated form** — try `well-known` as-is against vocab. If it settles, done.
2. **Dehyphenated compound** — strip hyphen, try `firetruck` as one particle. Catches inputs that hyphenated a valid compound word (e.g. `Fire-Truck` → `firetruck`).
3. **Split at hyphens, resolve pieces independently** — each segment as its own run. Catches neologisms, idiosyncratic links, misplaced em/en dashes.

Each step only runs on what didn't settle in the previous step. No variation is always valid — all three forms can appear in real text.

## Relationship to Existing Architecture

- Same phased/tiered principle as Phase 2 word resolution
- Same persistent-bed model — shared ASCII bed is permanent, table-specific beds loaded on demand
- Same cascade: resolve common → escalate unsettled → var-wrap truly unknown
- 4-byte normalization parallels lowercase normalization — canonical form established at boundary, zero cost downstream
