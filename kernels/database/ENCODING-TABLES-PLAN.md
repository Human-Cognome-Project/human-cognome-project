# UTF-8 encoding-tables build-out — PLAN

> **Status: PLAN, not built (2026-10-01).** This document specifies the next
> layer of the cold encoding floor — the UTF-8 character tables above the
> existing nibble/byte floor. **Execution is gated on Patrick accepting this
> plan.** Nothing here has been run against `hcp_core`; the relabel, the
> population, and the snapshot re-dump all happen only after acceptance.

These are **numerical tables only** — the byte-sequence structure of UTF-8
characters. No semantics (meaning, notation, word associations) is assigned
here; that is later work.

## 1. Hierarchy

The floor builds upward by composition, each tier's parents drawn from the tier
below:

```
nibbles            (16)        — the hex digits 0–F, 4 bits each; mass 1 (seed)
  └─ byte codes    (256)       — 2 ordered nibble parents (high, low); mass 2
       └─ characters           — 2/3/4 ordered byte-code parents; mass 4/6/8
```

- A **byte code** has two nibble parents (high then low) and mass 2 (sum of
  parents) — BUILT (the 2026-09-28 floor).
- A **multi-byte character** has its UTF-8 bytes as **ordered byte-code
  parents** — 2 parents for a 2-byte character, 3 for a 3-byte, 4 for a 4-byte.
  Mass = sum of parents, so a 2-byte character is mass 4, a 3-byte is 6, a
  4-byte is 8. `mint` derives the mass from the parent list; it is not declared.
- **1-byte characters are the existing byte codes `00`–`7F`.** ASCII is
  single-byte UTF-8, so those characters already exist as byte-code tokens; no
  separate character tokens are minted for them.

## 2. Relabel (notation only)

The three existing label rows at `00.00.01.00.0*` carry temporary/debug prose
`notation`. Correct them to the native terms (notation-only `UPDATE`; token_ids,
addresses, mass, and all `members`/`member_of` edges are untouched):

| current notation | new notation |
|---|---|
| `Single Hex Code` | `Nibbles` |
| `Hex Couplets` | `Byte Codes` |
| `Hex Code Patterns` | `Byte Code Groups` |

**Three new category labels (PROPOSAL — awaiting Patrick's confirm on names):**
`Two-Byte Codes`, `Three-Byte Codes`, `Four-Byte Codes`, each a `member_of`
`Byte Code Groups` (alongside `Nibbles` and `Byte Codes`); every character is a
`member_of` its category label. As construction posts these carry prose
`notation` like the existing labels until real naming literals exist.

## 3. Addressing allocation (locked with Patrick)

All tiers sit under `00.00.00.*`, so the temporary labels at `00.00.01.*` never
move. One couplet block (`00.00.00.XX.*`, varying the 5th couplet) holds
`62² = 3,844` endpoints.

| category | reserved range | capacity | valid set | headroom |
|---|---|---|---|---|
| nibbles + byte codes | `00.00.00.00.*` | 3,844 | ~270 (current) | — |
| 2-byte | `00.00.00.01.*`–`02.*` | 7,688 | 1,920 | ~4× |
| 3-byte | `00.00.00.03.*`–`0z.*` | ~227k | 61,440 | ~3.7× |
| 4-byte | `00.00.00.10.*`–`zz.*` | ~14.5M | 1,048,576 | ~14× |

These are **reservations**. Each category is populated sequentially from the
start of its range; the unused tail is deliberate sparse headroom (the
*Per-kind trunk allocation* / *Division by sparsity* pattern in `NOTES.md`).

The full-combinatorial alternative (reserving all 2⁸ⁿ values — ~4.3 billion for
4-byte, requiring sparse allocation below `00.00.*` across ~56 billion
addresses and relocating the temporary labels) was **rejected as overkill**.
Because nothing holds addresses (connection is the invariant, the address is
manager-assigned and fluid), if a category ever outgrows its block,
re-addressing later is cheap.

## 4. Generation algorithm

The valid set is **generated, not sourced from an external table**:

1. Walk every Unicode scalar value **U+0080 through U+10FFFF**.
2. **Skip the surrogate range U+D800–U+DFFF** (not encodable in UTF-8).
3. UTF-8-encode each codepoint → its 2, 3, or 4 bytes.
4. Those bytes **are** the ordered byte-code parents (leader first, then
   continuations).
5. Place the character sequentially (by codepoint) in its category's reserved
   range; `member_of` its category label.

This walk **is** exactly the valid UTF-8 set — overlong forms, surrogate
encodings, and anything past U+10FFFF are never generated, so no validity
filter is needed. Counts: 1,920 two-byte (U+0080–U+07FF), 61,440 three-byte
(U+0800–U+FFFF minus 2,048 surrogates), 1,048,576 four-byte
(U+10000–U+10FFFF).

### Codepoint vs encoding (do not re-walk into this)

The byte-code parents are the **actual UTF-8 bytes**, never the codepoint
digits:

| character | codepoint | UTF-8 bytes (the parents) |
|---|---|---|
| é | U+00E9 | `C3 A9` |
| € | U+20AC | `E2 82 AC` |
| 😀 | U+1F600 | `F0 9F 98 80` |

`U+00E9` is **not** bytes `00 E9`; UTF-8 re-encodes the value (payload bits
packed into framed bytes). A padded `U+XXXX` codepoint table shows every value
as a uniform ≥2-byte form with leading `00`s that do not exist on the wire —
which is exactly why we generate from the encoding and never ingest such a
table.

## 5. Population scope

Populate **all valid characters now**: 1,920 + 61,440 + 1,048,576 =
**1,111,936** new character tokens (the 128 one-byte/ASCII scalars are not
re-minted — they are the existing byte codes — so this is below the 1,112,064
total Unicode scalar count). With their parent/child reciprocals and
membership edges this is a few million row-writes; accepted — pure DB inserts
are fast and the exact runtime does not matter.

## 6. Driver

A **C++ bootstrap driver over the record-tier** (`DECLARE` / `mint`),
byte-count-parameterized so the same code produces the 2-, 3-, and 4-byte
tables. It reuses the built codec (address placement), reciprocal
(`token_parent`/`token_child`, `members`/`member_of`), and mass-derivation
logic — **no re-derivation of addressing or reciprocal logic in a script**, and
Python stays out of the address/reciprocal path entirely (at most it could hand
the driver a plain input list, but the walk is trivial in C++ so even that is
unnecessary).

**ADDITIVE ONLY.** The driver inserts; it never resets, drops, or truncates. It
must not resemble the removed `seed_0x` reset utility (AGENTS.md:
no DB-resetting tooling as routine execution). This is the small runnable
record-tier driver the HANDOFF flagged as ready-now.

## 7. Construction instructions (ordered)

1. Relabel the three notations (§2).
2. Create the three category labels under `Byte Code Groups` (§2).
3. Walk valid codepoints (§4); for each, `DECLARE` the character with its
   ordered byte-code parents, its sequential address in the category range, and
   `member_of` its category label.
4. Verify counts (§8).
5. Re-dump the snapshot to `data/postgres/snapshots/` (new dated dir; Git LFS
   for the compressed dump) and update its manifest.

## 8. Verification plan

- **Token total:** 276 (current) + 3 category labels + 1,111,936 characters =
  **1,112,215**.
- **`token_parent` rows added:** 1,920×2 + 61,440×3 + 1,048,576×4 =
  **4,382,464** (one row per ordinal slot). `token_child` is the reciprocal
  (one reverse per distinct byte-code→character pair; slightly fewer only where
  a byte repeats within a character).
- **`members` rows added:** 1,111,936 (each character in its category label) + 3
  (each category label in `Byte Code Groups`); `member_of` mirrors them.
- **Spot-checks:** `é` → parents `C3 A9`; `€` → `E2 82 AC`; `😀` →
  `F0 9F 98 80`; each character's mass = 2 × (byte count).
- **Additive check:** the only pre-existing rows changed are the three relabelled
  notations; nothing is removed, and the nibble/byte floor is byte-identical
  otherwise.

## 9. Execution gating

This document is the plan. The relabel, the ~1.1M-row population, and the
snapshot re-dump run **only after Patrick accepts it**. On acceptance the driver
is built under the coder + adversary discipline, run additively against
`hcp_core`, verified against §8, then snapshot + PR.
