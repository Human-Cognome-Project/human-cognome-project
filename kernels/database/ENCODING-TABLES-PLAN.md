# UTF-8 encoding-tables build-out — PLAN

> **Status: PLAN, not built (rev. 2026-10-02).** This document specifies the next
> layer of the cold encoding floor — the UTF-8 character tables above the
> existing nibble/byte floor. **Execution is gated on Patrick accepting this
> plan.** Nothing here has been run against `hcp_core`; the schema trigger, the
> relabel, the population, and the snapshot re-dump all happen only after
> acceptance.

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
  Structural mass = sum of parents, so a 2-byte character is mass 4, a 3-byte is
  6, a 4-byte is 8 — set automatically by the **structural-mass trigger (§5)**,
  neither declared nor derived by `mint`.
- **1-byte characters are the existing byte codes `00`–`7F`.** ASCII is
  single-byte UTF-8, so those characters already exist as byte-code tokens; no
  separate character tokens are minted for them.

## 2. Relabel (notation only)

The three existing label rows at `00.00.01.00.0*` carry temporary/debug prose
`notation`. Correct them to the native terms with a **direct SQL `UPDATE`** on the
three `notation` cells. There is **no record-tier notation-edit verb** — the
`UPDATE_RECORD` surface is only MOVE_RECORD / ADD_CONNECTION / DELETE_RECORD /
DELETE_CONNECTION (`update/README.md`) — and `notation` is the temporary/debug
column. The edit is non-destructive and verified by re-query; token_ids,
addresses, mass, and all `members`/`member_of` edges are untouched:

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
4-byte, requiring sparse allocation below `00.00.*` across ~56 billion addresses
and relocating the temporary labels) was **rejected as overkill**.

**Reserve generously up front — re-addressing a populated block is NOT cheap.**
Addresses are the canonical `text[]` primary keys, referenced throughout
`token_parent`, `token_child`, `members`, and `member_of`. `Controller::rekey`
(`controller/controller.cpp:510-583`) relocates one token as INSERT-new +
repoint-8-FK-columns + DELETE-old in a transaction, so re-addressing a block of
millions would be a substantial coordinated rekey plus snapshot/reference update.
("Nothing holds addresses" is true for the *analyst*, which navigates by
connection — but the DB's own foreign keys do.) The generous reservations above
are sized so we never need that.

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

## 5. Structural mass — set by a trigger (removes it from followup work)

Structural mass is maintained by the database itself, as a standing invariant,
not supplied by the driver and not derived by `mint` (`Controller::mint` stores
its optional `mass` argument verbatim, writing SQL NULL when absent —
`controller/controller.cpp:404-414` — and `DECLARE` passes `nullopt`,
`declare/declare_core.cpp:222`).

**The trigger.** A single `AFTER INSERT` trigger on `token_parent`: when a
token's constituent rows are linked, set the owning token's
`mass = sum of its direct parents' token.mass`. It is **one level only** — each
parent's stored mass already aggregates its own subtree (the store-the-tax
funnel), so there is no recursion. Parentless base atoms never fire it and keep
their declared seed (nibble 1, `0x` 0). The trigger's formula yields the floor's
own values (byte codes → 2, multi-byte → 4 / 6 / 8); it fires only on new inserts,
not retroactively, so it never recomputes the existing seeded rows (a re-mint is a
SEE no-op, so no `token_parent` insert and no trigger). Guard: SQL `SUM` skips
NULL, so the trigger assumes each parent already carries a mass — true here, every
parent is a seeded atom or a mass-2 byte code.

**No cascade.** Structural mass is **invariant** — it is the structural value of
the composition, and composition *is* identity: change a token's parents and it
is a different token, not a mutated one, and the values ground in the fixed atom
seeds. MOVE/rekey re-addresses the *same* composition (same parents, same mass);
ADD_CONNECTION / DELETE_CONNECTION touch membership, not structure. So nothing
upstream ever changes under a fixed token, and there is nothing to propagate.

This makes structural mass a **record-tier schema change** (`schema/schema.sql`
trigger) plus model-anchored tests, and **removes structural mass from the
deferred aggregation / followup work entirely**. The driver therefore supplies
**no mass** (the trigger is the authority), which also dissolves the
`DECLARE`/mass contradiction.

### Structural vs centroid (do not conflate)

`token.mass` holds **structural mass** — the trigger's sum-of-parents, the fixed
**seed**. The **centroid** masses are a separate, **calculated** layer:

- the engine's per-tick **force / mass centroids** (the `m2` of `m1·m2/d²`,
  `OPERATIONAL-PLAN.md` §3.5); and
- the cache manager's **label / rollup centroids** (aggregate of members).

Structural values **seed** the centroid calculation; they are not the centroid.
Nothing writes a centroid into `token.mass`.

### Labels use both

A label draws on **both** values, for two jobs:

- its **own structural value** (from its naming literal's parents, in
  `token.mass`) drives **meta-organization** — how labels sit relative to one
  another; and
- its **centroid** (aggregate of members, calculated) drives
  **component-organization** — how it rolls up the things it groups.

So `token.mass` is now **uniformly structural** (literals and a label's naming
literal alike); the centroid is the calculated layer. This **refines** the
earlier note that "a label's stored mass = centroid of members": the *stored*
value is structural; the centroid is computed. The temporary labels have no
naming literal (no parents) yet, so the trigger never fires for them and their
placeholder `mass` stays until the prose→token_id swap builds the naming
literal; their centroid is computed from members whenever something composes
them.

### Structural values are gross alignment parameters

Within a group membership, structural values act as **gross alignment
parameters**: members whose structural values are close are coarsely drawn
togetherish, so structurally-kin things cluster at the coarse level (e.g. UTF-8
and UTF-16 would be structurally inclined to sit togetherish — illustrative;
UTF-16 is **not** added here, this build is UTF-8 only). The **centroid** values
then do the finer component organization within that gross arrangement — the
model's coarse→fine LoD, where more fields pin position more exactly.

This falls straight out of the trigger values here: structural mass **bands by
byte length** — every 2-byte character is mass 4, every 3-byte is 6, every
4-byte is 8 — so characters coarsely align by encoding width, with finer
separation within a band coming from the centroid layer.

## 6. Population scope

Populate **all valid characters now**: 1,920 + 61,440 + 1,048,576 =
**1,111,936** new character tokens (the 128 one-byte/ASCII scalars are not
re-minted — they are the existing byte codes — so this is below the 1,112,064
total Unicode scalar count).

With parent/child reciprocals and membership edges this is roughly **12.1M rows
across ~2.2M+ commits** — `mint` is one atomic transaction per token and
`add_membership` commits separately (`controller/controller.cpp`). This is **not
a performance guarantee**; Patrick accepts the runtime cost as the party paying
for the cycles. The load is naturally **resumable**: each mint is atomic and the
door is SEE-idempotent (`ON CONFLICT DO NOTHING`), so a re-run skips what is
already in and continues. **No batched bulk-insert path is added** — speed is
not the concern, and resumability comes for free from idempotency.

## 7. Driver

A **C++ bootstrap driver that mints directly through the `Controller`** — the
same bootstrap channel the seed floor used (`declare/declare_core.cpp:224`: "the
seed floor is minted directly through the controller, never through this core"),
**not** the `DECLARE` core (which force-blanks mass). Byte-count-parameterized so
the same code produces the 2-, 3-, and 4-byte tables. Per codepoint it:

1. computes the UTF-8 bytes → the ordered byte-code parents;
2. computes the sequential address in the category's reserved range;
3. `mint`s the character with those parents at that address and **no mass**
   (the trigger fills structural mass from the parents);
4. `add_membership` to the category label.

It reuses the built codec (address representation) and the controller's
reciprocal maintenance (`token_parent`/`token_child`, `members`/`member_of`) —
**no re-derivation of addressing or reciprocal logic in a script**, and Python
stays out of the address/reciprocal path entirely.

**ADDITIVE ONLY.** The driver inserts; it never resets, drops, or truncates. It
must not resemble the removed `seed_0x` reset utility (AGENTS.md: no
DB-resetting tooling as routine execution). This is the small runnable
record-tier driver the HANDOFF flagged as ready-now.

## 8. Construction instructions (ordered)

1. Add the structural-mass trigger to `schema/schema.sql` (§5) and test it
   against the existing floor (byte codes must read mass 2; atoms unchanged).
2. Relabel the three notations via direct SQL `UPDATE` (§2).
3. Create the three category labels under `Byte Code Groups` (§2).
4. Walk valid codepoints (§4); for each, mint directly (§7) with ordered
   byte-code parents, the sequential address in the category range, and no
   mass; `add_membership` to its category label. The trigger sets structural
   mass as the parents are linked.
5. Verify counts and masses (§9).
6. Re-dump the snapshot to `data/postgres/snapshots/` (new dated dir; Git LFS
   for the compressed dump) and update its manifest.

## 9. Verification plan

- **Floor unchanged:** adding the trigger does not touch the existing floor — it
  fires only on new `token_parent` inserts, not retroactively, so the 256 byte
  codes keep their seeded `mass = 2` and the 16 nibbles / `0x` keep their seed.
- **Trigger validated on a new mint:** the first newly-minted 2-byte character
  reads `mass = 4` (see spot-checks below) — that is the trigger firing; the floor
  is not.
- **Token total:** 276 (current) + 3 category labels + 1,111,936 characters =
  **1,112,215**.
- **`token_parent` rows added:** 1,920×2 + 61,440×3 + 1,048,576×4 =
  **4,382,464** (one row per ordinal slot). `token_child` is the reciprocal
  (one reverse per distinct byte-code→character pair; slightly fewer only where
  a byte repeats within a character).
- **`members` rows added:** 1,111,936 (each character in its category label) + 3
  (each category label in `Byte Code Groups`); `member_of` mirrors them.
- **Mass spot-checks (set by the trigger):** `é` → parents `C3 A9`, mass 4;
  `€` → `E2 82 AC`, mass 6; `😀` → `F0 9F 98 80`, mass 8; generally
  mass = 2 × (byte count).
- **Additive check:** the only pre-existing rows changed are the three
  relabelled notations; the byte codes' masses are unchanged from the seed (the
  trigger does not recompute existing rows). Nothing is removed, and the
  nibble/byte floor is otherwise unchanged.

## 10. Execution gating

This document is the plan. The schema trigger, the relabel, the ~1.1M-row
population, and the snapshot re-dump run **only after Patrick accepts it**. On
acceptance the trigger and driver are built under the coder + adversary
discipline, run additively against `hcp_core`, verified against §9, then
snapshot + PR.
