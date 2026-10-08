# Character / endpoint tier — PLAN

> **Purpose and scope.** This endpoint tier establishes the level needed to begin ingesting language
> dictionaries and assembling SNode objects for each language's base structures and recorded combinations,
> ahead of assembling the English database. Cross-encoding completeness is deferred and is **not** a
> prerequisite for this step: this build is one parent and one membership per endpoint (the `UTF-8` mapping).

> **Status: PLAN, not built, not executed (drafted 2026-10-06, finalized after adversarial review).** A drafting-agent document; the
> adversary's confirmed corrections are applied and the lead's decisions are in §0A. It is not accepted by Patrick until the PR is reviewed. No
> database write, no commit, and no code was produced in drafting. Every status/structure claim
> below was checked read-only against live `hcp_core` (queries 2026-10-06, session
> `default_transaction_read_only=on`) and the code on `exec/encoding-floor` at `5120ee6`; where
> that differs from HANDOFF/NOTES prose it is in §0.
>
> **Intended home:** `kernels/database/ENCODING-ENDPOINT-TIER-PLAN.md`, a sibling of
> `ENCODING-TABLES-PLAN.md` (which lives on `docs/encoding-tables-plan`, PR #108, not on `main`).
> It extends that plan family; it does not edit it.

## 0. Verified state and divergences from the prose

Live `hcp_core` (2026-10-06): token 1,112,215 · token_parent 4,382,976 · token_child 4,333,104 ·
members 1,112,213 · member_of 1,112,213 — **all match HANDOFF and the 2026-10-03 MANIFEST.** No
NULL `token.mass` anywhere; the five PK indexes are the only indexes; the one trigger is
`token_parent_structural_mass`; address columns are `COLLATE "C"`; DB 3,737 MB; 189 GB free.
Nothing exists at or above third couplet `02` (0 rows), nor at label slot `00.00.01.00.06` (0 rows).

Byte-couplet tier, as built: 1,111,936 tokens, 5-couplet addresses `00.00.00.<4th>.<5th>`,
every one with exactly 2/3/4 ordered byte-code parents (0 exceptions), mass 4/6/8 (0 exceptions),
no duplicate notation, every `token_text` equal to its dot-joined address. Member counts of the
labels: Two 1,920 · Three 61,440 · Four 1,048,576; the three byte-width labels and `Nibbles`/`Byte
Codes` are each `member_of` `Byte Code Groups` (`00.00.01.00.02`).

**Divergences / things the prose does not say:**

1. **No driver code for this tier exists.** The working tree is clean apart from this document; an
   earlier address function mirroring the byte-couplet tables' own layout (stepping the combination's third couplet `00`→`02`) was
   considered and rejected in favour of the encoding-agnostic function in §1.2.
2. **DECLARE cannot express this token.** `command_ir.cpp:150` enforces an anti-alias floor of
   >= 2 constituents per PARENTS list; an endpoint has exactly one parent. So the path is the
   same **direct `Controller::mint` bootstrap channel** the byte tiers used
   (`declare_core.cpp:224`: "the seed floor is minted directly through the controller"), not
   DECLARE.
3. **#109 on the direct-mint path is the trigger.** `structural_mass_trigger.sql` states "Guard (#109,
   direct-mint path)": `parent mass IS NULL → RAISE`. That arm is on this path. Only the
   `declare_core.cpp` arm (`unknown_mass_parent`) is not used, since the driver does not call
   DECLARE. The driver's pre-check (§2) is redundant belt-and-braces. #110 (`with_transaction`)
   is also used.
4. **The `Controller` has no way to add a parent to an existing token.** `mint` is SEE-then-no-op
   (`controller.cpp` returns at the SEE probe), and its header says "only mint writes structure".
   The forward requirement (§7) therefore needs a new, small primitive that does not exist.
5. **Re-run idempotency must not cap an endpoint at one parent.** The check in `populate_endpoint`
   is `combination ∈ parents_of(endpoint)` plus whole-integrity (mass over all parents, reciprocal child edge,
   both membership directions by point-probe; §5), never `size() == 1`, so a later additional parent does not break a
   re-run, while an invalid whole is rejected rather than healed.
6. **Terminology collision.** HANDOFF/README/driver call the byte-couplet tokens "characters"
   (`populate_encoding … sample|full`, `populate_character`). The pinned model calls the new
   tier the character/endpoint tier. This plan says *byte-couplet token* and *endpoint*
   throughout and recommends new driver symbols/modes use `endpoint` (§5).
7. **`token.notation` is documented TEMPORARY, "WILL BE DROPPED"** (`schema.sql`). The pinned model
   makes the endpoint's character live in `notation`. See §8 (deferred).
8. Stale prose already acknowledged by the tables plan (§5 of that plan): `schema.sql:135`
   "label's mass is the centroid" comment. Not touched here.

9. **Provenance caveat.** The byte couplet tables are the shared, encoding-agnostic substrate and are
   left exactly as built; no encoding is a property of them. An encoding's mapping onto them is recorded
   only at the character level (the endpoint's `member_of` label). Only one such mapping exists today, so
   the "shared across encodings" claim is not yet exercised and current coverage is UTF-8-derived (a later
   encoding such as UTF-16BE would first have to mint byte-couplet tokens that were never populated, e.g. `00 E9`), and `combo_address(cp)` (§3) is necessarily
   the substrate's own layout function.

## 1. Addressing and placement

### 1.1 How the existing layout actually addresses things (verified)

- Root `00.00.00.00.00`; every address is `text[]` of two-character Base62 couplets, alphabet
  `0-9A-Za-z` (byte order = `COLLATE "C"`), couplet code = `62*d1 + d2`, space 3,844 (`codec.h`).
- Third couplet is the **kind/trunk at that ring**: `00` = the byte floor and byte-couplet
  tokens (4th couplet selects the block), `01` = labels (`00.00.01.00.*`), and `02` and up is empty.
  NOTES "Per-kind trunk allocation": a kind takes whole trunks and "the next kind starts at the next
  unoccupied trunk". The next unoccupied trunk at this ring is **`00.00.02`**.
- Natural-id principle (NOTES, 2026-09-28): the address character equals the value where it can
  (hex `0-F` at `…00–0F`).
- Byte-couplet addresses are sequential slots within byte-width bands (2/3/4-byte ranges).

### 1.2 Recommended derivation: the codepoint, written as four Base62 digits

```
endpoint_address(cp) = 00 . 00 . 02 . couplet(cp / 3844) . couplet(cp mod 3844)
couplet(n)           = ALPHABET[n / 62] ALPHABET[n mod 62]
```

That is `cp` as a four-digit Base62 number grouped into two couplets. It uses nothing but the
codepoint: no byte width, no surrogate compaction, no hex encoding, no `U+0080` floor. It is the
natural-id principle applied to the codepoint. Worked values (computed, and decoded back in SQL
against live `hcp_core`'s alphabet):

| cp | derivation | endpoint address |
|---|---|---|
| U+0041 (not minted here) | 65 = 0·3844 + 65 → `00`,`13` | `00.00.02.00.13` |
| U+0080 (first minted) | 128 → `00`,`24` | `00.00.02.00.24` |
| U+00E9 é | 233 → `00`,`3l` | `00.00.02.00.3l` |
| U+4E2D 中 | 20013 = 5·3844 + 793 → `05`,`Cn` | `00.00.02.05.Cn` |
| U+20AC € | 8364 = 2·3844 + 676 → `02`,`Au` | `00.00.02.02.Au` |
| U+1F600 😀 | 128512 = 33·3844 + 1660 → `0X`,`Qm` | `00.00.02.0X.Qm` |
| U+10FFFF (last) | 1114111 = 289·3844 + 3195 → `4f`,`pX` | `00.00.02.4f.pX` |

Properties (each is a checkable predicate):
- **Codepoint-ordered.** Fixed-width couplets + `COLLATE "C"` ⇒ PK order = codepoint order; a
  codepoint range is a contiguous PK range (gather-compatible, "sweep a trunk as one spread").
- **Simple math, no special cases.** No branch in the function. The surrogate range is simply
  never minted (no byte-couplet token exists for them); it leaves a sparse gap (`00.00.02.0E.Ns`…`0E.ut` region),
  which is the project's own "division by sparsity" — the gap needs no code.
- **Fits with headroom.** Max 4th couplet `4f` (code 289) of 3,843 possible: room for 14.7 M code
  points; Unicode needs 1.1 M. Trunk `00.00.02` is wholly the endpoint kind; the next kind starts
  at `00.00.03`.
- **No collision.** `00.00.02.*` is empty (verified); it is disjoint from `00.00.00.*` and
  `00.00.01.*`. Domain of the function: U+0000–U+10FFFF (a second table may need U+0000–U+007F;
  this build mints only U+0080+ per the pin, §4).

**Rejected alternative.** A function mirroring the byte-couplet tables' own layout (keep the
couplet token's 4th/5th couplets, step the third to `02`; U+0080 → `00.00.02.01.00`) was considered and
rejected: its slots are byte-width-band slots, so an endpoint's identity would depend on the substrate's
layout (band starts, the surrogate compaction shifting every cp ≥ U+E000 by 0x800), it has no address for
U+0000–U+007F or surrogates, and it needs three cases where §1.2 needs none. The pinned model
(codepoint-alone, encoding-agnostic) already decides this.

### 1.3 The `UTF-8` label (character-level membership)

`00.00.01.00.06`: the next free slot of the label trunk after `…03–05` (verified empty), notation
`UTF-8`, temporary placeholder `mass` 10 (the existing labels' convention; it is never a parent so
#109 is moot), no parents. Created before any endpoint (membership FK). It is not made a member of
anything (no "Encodings" parent label is invented — §0A).

## 2. Mint path and fields; one atomic unit; mass

**Path:** `Controller::mint` + `Controller::add_membership` inside `Controller::with_transaction`
(#110), exactly the byte-tier driver's channel. Fields of the per-endpoint command:

| field | value |
|---|---|
| token_id | `endpoint_address(cp)` |
| token_text | dot-joined address (derived by `mint`) |
| notation | the character as text (`chr(cp)`) — not hex |
| mass | **none passed** (`nullopt` → NULL; the trigger fills it) |
| constituents | exactly one: `{combo_address(cp), nullopt}` → `token_parent` ordinal 0, `token_child` wired |
| membership | `add_membership(endpoint, 00.00.01.00.06)` → `member_of` + `members` |

**Atomic unit:** `with_transaction { SEE/MINT/LINK/WIRE (mint) ; AFTER-INSERT trigger fires inside
it ; add_membership }`. FK failure (missing combination), the trigger's NULL-parent RAISE, or a
membership failure rolls back the token, parent, child and membership rows together — no minted-
but-unmembered or unparented endpoint can persist (#110's contract; the byte tier's existing test
covers rollback on forced membership failure). **#109 on this path:** the trigger's `NULL parent → RAISE` is #109's direct-mint arm and is
already in force. As belt-and-braces the driver pre-checks `attributes_of(combo)` (exists, `mass`
non-NULL) before the transaction and refuses otherwise. (DECLARE's `unknown_mass_parent` arm is not
on this path.)

**Mass (trigger, `structural_mass_trigger.sql`):** after the single `token_parent` insert, the
trigger reads the parent's mass (NULL ⇒ RAISE) and sets the endpoint's
`mass = SUM(parent masses)` — one parent, so **mass = the combination's mass**:

| case | combination parent mass | endpoint mass |
|---|---|---|
| 2-byte, U+00E9 (C3 A9) | 4 | **4** |
| 3-byte, U+4E2D (E4 B8 AD) | 6 | **6** |
| 4-byte, U+1F600 (F0 9F 98 80) | 8 | **8** |

Only the endpoint's own row is updated; no byte-couplet or byte-code row is touched (no cascade).
**Deferred consequence (§8):** the rule is a sum over *all* parents, so when a second
table later attaches a second parent the endpoint's mass becomes the sum of both (e.g. U+00E9
4 + 4 = 8). That follows the settled rule but makes an endpoint's mass depend on how many tables
cover it.

## 3. Resolving the byte-couplet token (only-follow)

The UTF-8 mapping's link-in is `combo_address(cp)` = the **existing** `character_address(cp)` (the
pure layout function that placed the byte tier; verified against live for U+00E9 → `00.00.00.01.1h`,
U+4E2D → `…07.fl`, U+20AC → `…04.ds`, U+1F600 → `…1G.Nk`, U+10FFFF → `…5O.mV`). Resolution is a
direct PK read of that address — no scan, no reverse index, nothing searched. Guards before the
command: the row exists; `mass` is non-NULL; and its `notation` equals the uppercase hex of
`utf8_encode(cp)` (the byte sequence the `UTF-8` membership maps the codepoint to). The **address is the identity**;
`notation` is a temporary column, so it is a cross-check, never the key (§0.7, §8). A mismatch
throws and writes nothing.

The endpoint address function (§1.2) never calls any byte-couplet-layout or encoding helper; `combo_address` is called only
by the UTF-8 mapping's link-in code. Keep them in separate functions so a later encoding cannot
inherit the substrate's layout by accident.

## 4. Surrogates and edge cases

- **Surrogates U+D800–U+DFFF:** never minted (not scalar values; the byte tier skips them and has
  no combination to parent them). No special case in the address function; the gap is sparse.
- **Noncharacters (U+FDD0–U+FDEF, U+nFFFE/U+nFFFF), unassigned, private use, C1 controls
  U+0080–U+009F:** **included.** Verified: the byte tier minted all 1,111,936 scalar values with no
  noncharacter exclusion (e.g. U+FDD0 `EFB790` at `00.00.00.0I.pw`, U+FFFE `EFBFBE`, U+1FFFE
  `F09FBFBE`, U+10FFFF `F48FBFBF` all exist). Each endpoint needs exactly its combination, so
  excluding any would leave an orphan combination; they are valid scalar values, legal for
  internal interchange, and the pin says "each valid codepoint". Postgres `text` stores all of
  them in a UTF8 database (only U+0000 is unstorable and is out of range).
- **U+0000–U+007F:** out of this build (the pin starts at U+0080; those are byte codes `00–7F`,
  not combinations). Their endpoints would take a byte code as parent (§8).
- **Display:** many notations are invisible/control/combining; all verification compares hex
  (`encode(convert_to(notation,'UTF8'),'hex')`), never display.
- **Client encoding:** `Controller` takes libpq defaults; live default is `UTF8`. Run with
  `PGCLIENTENCODING=UTF8` explicitly so the character bytes are never reinterpreted.
- **Normalization:** none; the stored text is `chr(cp)` verbatim (NFC/NFD never applied).

## 5. Driver and Controller changes (`kernels/database/bootstrap/`, `kernels/database/controller/`)

**Component scope.** This step changes two components: the `bootstrap/` driver (below) **and** `controller/`,
which gains one new read method, `membership_present(member, group)`, plus its own unit test (in
`controller/`, in the style of `controller_txn_test.cpp`, against a throwaway DB). The Controller change is
purely additive (a new const-style read; no existing signature or behaviour changes), so existing
`Controller` consumers are unaffected (AGENTS.md: preserve working interfaces). It is a separate
commit from the driver work.

Extend in the existing style (free functions in `encoding_populate.{h,cpp}`, one driver `main`,
tests in `encoding_populate_test.cpp` against a throwaway `hcp_test_<pid>` DB). Existing
`sample`/`full` modes, `populate_character`, `create_category_labels` stay untouched (AGENTS.md:
preserve working interfaces).

New symbols (names use *endpoint*; §0A):
- `endpoint_address(cp)` — §1.2; no encoding or byte-couplet-layout dependency; throws outside U+0000..U+10FFFF/surrogates.
- `utf8_label_address()`, `create_utf8_label(ctl)` — one command, idempotent via SEE.
- `resolve_combination(ctl, cp)` — §3 pre-check, returns the combination address.
- `populate_endpoint(ctl, cp)` — the §2 command. Re-run semantics (SEE integrity): an existing
  endpoint is accepted only if the **whole** validates, and an invalid whole is **rejected (throw, no
  write)** — the settled command rule; nothing partial is retained or healed. Checks, each a PK
  point or PK-prefix follow of one endpoint's own rows:
  (i) `combination ∈ parents_of(endpoint)` (never `size()==1`, so later additional parents stay allowed);
  (ii) the endpoint's `mass` equals the sum of `attributes_of(parent).mass` over **all** its current
  parents (the parent tokens' own `token.mass`). Do **not** use `parents_of(...).mass`: that is the
  per-element `token_parent.mass`, which is NULL for endpoints, so the sum would be wrong;
  (iii) the reciprocal `token_child` edge (combination → endpoint) exists, read from
  `children_of(combination)` (a byte-couplet token has one endpoint child in this step, so this read is small);
  (iv) **both** membership directions exist, by a **direct point-probe on the full membership PK pair in
  each direction**: `member_of(endpoint, UTF-8)` **and** `members(UTF-8, endpoint)`. It must **not** be
  `members_of(label)`, which returns every member of the label (up to ~1.1 M rows) and would make a
  resume ~10^12 reads. The current Controller has no pair probe, so this step names a **minimal required
  read primitive**, `membership_present(member, group)` → `{in_member_of, in_members}` (two full-key
  `SELECT 1` point lookups; a follow, not a search), added to `Controller` the same way §7 names
  `link_parent`; it is the one Controller addition this step needs;
  (v) `notation == chr(cp)` (from `attributes_of`).
  **A one-sided membership is rejected, never healed.** The re-run/resume path must **not** call
  `add_membership` on an already-existing endpoint: `add_membership` is `ON CONFLICT DO NOTHING` on each
  side and would silently fill in a missing side, masking the invalid whole this check exists to reject.
  Only a **freshly minted** endpoint runs `add_membership` (inside its §2 transaction). The byte-tier
  `populate_character` calls `add_membership` unconditionally; that pattern must **not** be copied into
  the integrity-rejecting re-run path.
- Driver modes `endpoints-sample` (U+00E9, U+20AC, U+1F600 — `sample_codepoints()`) and
  `endpoints` (U+0080..U+10FFFF, `is_populated_codepoint`), after `verify_floor` **plus** a new
  `verify_combinations` that reads only the three spot combinations in sample mode and, in full
  mode, relies on the per-endpoint pre-check. Progress sparsely to stderr; run redirects stderr to
  a log file (repo rule: big output to files).
- Tests (model-anchored, throwaway DB, never `hcp_core`): the §1.2 table above; strict monotonicity
  of `endpoint_address` over a stride of cp and over every width boundary
  (0x7FF/0x800, 0xFFFF/0x10000, 0xD7FF/0xE000, 0x10FFFF); no overlap with the combination/label
  trunks; mass 4/6/8; one parent / one membership; rollback leaves nothing when the combination is
  absent or NULL-mass; re-run no-op; **schema-permits-N proof** (in the throwaway DB, raw SQL: a
  second `token_parent` ordinal and a second `member_of` group on an endpoint insert cleanly and
  the trigger sums both) — proves nothing caps at one without building the attach primitive.
- **Required negative (rejection) tests for the re-run integrity check.** An existing endpoint is accepted
  only when the whole validates; each of the following invalid wholes, constructed in the throwaway DB by
  raw SQL, must make `populate_endpoint` **throw and write nothing** (row counts and a fingerprint of the
  endpoint's rows unchanged afterwards): (1) missing reciprocal `token_child` edge; (2) one-sided membership,
  `member_of` row present but `members` row absent; (3) one-sided membership, `members` row present but
  `member_of` row absent; (4) `mass` not equal to the sum of its parents' masses; (5) `notation != chr(cp)`.
  Plus the accepting case: a complete endpoint re-run is a no-op, and one with an additional second parent
  (raw SQL) whose mass equals the sum over both is still accepted.
- **Required `membership_present` unit test (`controller/`).** For a pair present in both directions it
  returns `{true, true}`; for an absent pair `{false, false}`; and for a one-sided pair created by raw SQL
  it returns `{true, false}` and `{false, true}` respectively — so each direction is probed independently.

Not in this build: the attach primitive for a second parent (§7).

## 6. Execution on the actual `hcp_core`

All steps additive: no reset, drop, truncate, update or delete against `hcp_core`. `hcp_core` is
the working store; the snapshot is the public-sharing export produced at the end (§6.6). Gated on
Patrick's acceptance of this plan and on the unit coder/adversary protocol of the tables plan §10.

**Execution dependency on PR #111.** Execution requires PR #111's driver, structural-mass trigger and
2026-10-03 snapshot (`exec/encoding-floor` at `5120ee6`) to be reviewed and available on the target.
This PR targets `main` while that implementation is still on the open `exec/encoding-floor` branch;
the plan cannot be executed until #111 is resolved. The open MANIFEST review thread on #111 also
bears on §6.1 and §6.6.

**6.0 Preflight (read-only).** Row totals equal §0; `00.00.02.*` and `00.00.01.00.06` empty; no NULL
mass; combination counts per label 1,920/61,440/1,048,576; trigger present; free disk >= 10 GB.
Record the **pre-fingerprint** of the pre-existing region (§6.4).

**6.0b Private safety dump (non-git).** `pg_dump` of `hcp_core` to a private location outside the repo
(never committed; distinct from the public sharing snapshot of §6.6). A forensic copy for comparison,
never a restore target. Taken before 6.4.

**6.1 Rehearsal in a disposable DB (not `hcp_core`).** `createdb hcp_rehearsal_<pid>`; restore
`data/postgres/snapshots/2026-10-03-encoding-tables/hcp_core.sql.gz` by a **normal** restore
(`zcat … | psql -v ON_ERROR_STOP=1`; no fast-load shortcut — `pg_dump` creates the trigger in the
post-data section, after `token_parent` is loaded, so it does not fire over the dumped rows) and validate
the restored totals first; run the label, sample, full,
the §6.5 queries and the §6.4 fingerprint; record time; drop **only** that DB (name prefix
checked, as `run_populate_test.sh` does). Because endpoints cannot be deleted from `hcp_core` under
the no-delete rule, this rehearsal is what de-risks the real run.

**6.2 Label** on `hcp_core`: `create_utf8_label` — 1 command, own transaction.

**6.3 Sample then STOP.** `PGCLIENTENCODING=UTF8 populate_encoding "dbname=hcp_core" endpoints-sample`
— three endpoints in three commands (one each of 2/3/4-byte). Run the spot-checks (§6.5). **Stop
for review** before the full walk (mirrors U7).

**6.4 Full.** `endpoints` mode (the sample's three become "already present", verified);
one `with_transaction` command per endpoint, 1,111,936 commits, resumable (a re-run SEEs and
verifies present ones). Optional bootstrap-only `PGOPTIONS="-c synchronous_commit=off"` as the
bootstrap README allows (a crash can lose only the last commits; re-run completes them). Not
a performance claim.

**Verifying the byte tables are untouched (before/after fingerprint).** Define OLD =
`token_id < '{00,00,02}'` excluding the new label. Compute identical fingerprints before 6.2 and
after 6.4 and compare; counts and md5 must be equal:
```sql
SELECT 'token', count(*), md5(string_agg(concat_ws('|',array_to_string(token_id,'.'),token_text,notation,mass), E'\n' ORDER BY token_id))
 FROM token WHERE token_id < ARRAY['00','00','02'] AND token_id <> ARRAY['00','00','01','00','06'];
SELECT 'token_parent', count(*), md5(string_agg(concat_ws('|',array_to_string(token_id,'.'),ordinal,array_to_string(parent_token_id,'.'),mass), E'\n' ORDER BY token_id,ordinal))
 FROM token_parent WHERE token_id < ARRAY['00','00','02'];
SELECT 'token_child(old children)', count(*), md5(string_agg(array_to_string(token_id,'.')||'>'||array_to_string(child_token_id,'.'), E'\n' ORDER BY token_id, child_token_id))
 FROM token_child WHERE child_token_id < ARRAY['00','00','02'];
SELECT 'members(old members)', count(*), md5(string_agg(array_to_string(token_id,'.')||'>'||array_to_string(member_token_id,'.'), E'\n' ORDER BY token_id, member_token_id))
 FROM members WHERE member_token_id < ARRAY['00','00','02'];
SELECT 'member_of(old)', count(*), md5(string_agg(array_to_string(token_id,'.')||'>'||array_to_string(group_token_id,'.'), E'\n' ORDER BY token_id, group_token_id))
 FROM member_of WHERE token_id < ARRAY['00','00','02'];
```
(The `token` fingerprint was run read-only on 2026-10-06: 1,112,215 rows, md5
`7665b8a28e232ba3c36b80ecb51a06df` — recompute at 6.0; do not trust this recorded value.)
Filtering token_child/members by the *old side* is deliberate: the combinations legitimately gain
one new `token_child` row each (their endpoint) — additive, listed below, not a change to old rows. Byte-tier row **content** is untouched; the combination → endpoint
`token_child` edge is the expected reciprocal of building a tier above (INSERT-only), and is itself the
additive expansion.

**Expected deltas** (from §0 baseline):

| table | before | added | after |
|---|---|---|---|
| token | 1,112,215 | 1 label + 1,111,936 endpoints = 1,111,937 | 2,224,152 |
| token_parent | 4,382,976 | 1,111,936 (one per endpoint, ordinal 0) | 5,494,912 |
| token_child | 4,333,104 | 1,111,936 (one per combination→endpoint) | 5,445,040 |
| members | 1,112,213 | 1,111,936 (all under the UTF-8 label) | 2,224,149 |
| member_of | 1,112,213 | 1,111,936 | 2,224,149 |

Total 5,559,681 rows (1,111,937 + 4 × 1,111,936). Expected growth ~1.5–2 GB of the current 3.7 GB (estimate, not measured —
the rehearsal gives the number).

**6.5 Post-build read-only validation.** (Run each against the rehearsal DB, then `hcp_core`.)
```sql
-- A. totals: the §6.4 table, plus label 06 has 0 parents and 0 member_of, 1,111,936 members.
-- B. endpoint decode + character + parent identity (all four must return 0; every comparison is NULL-safe):
WITH al AS (SELECT '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'::text s),
e AS (SELECT t.token_id, t.notation, t.mass,
        (((strpos(al.s,substr(t.token_id[4],1,1))-1)*62 + strpos(al.s,substr(t.token_id[4],2,1))-1)*3844
         + (strpos(al.s,substr(t.token_id[5],1,1))-1)*62 + strpos(al.s,substr(t.token_id[5],2,1))-1) AS cp
      FROM token t, al
      WHERE t.token_id >= ARRAY['00','00','02'] AND t.token_id < ARRAY['00','00','03'])
SELECT
 (SELECT count(*) FROM e WHERE e.cp IS NULL OR e.cp BETWEEN 55296 AND 57343 OR e.cp < 128 OR e.cp > 1114111),            -- 0
 (SELECT count(*) FROM e WHERE e.notation IS DISTINCT FROM chr(e.cp)),                                    -- 0
 (SELECT count(*) FROM e JOIN token_parent p ON p.token_id=e.token_id
    JOIN token c ON c.token_id=p.parent_token_id
   WHERE p.ordinal IS DISTINCT FROM 0 OR c.token_id[3] IS DISTINCT FROM '00' OR c.notation IS DISTINCT FROM upper(encode(convert_to(e.notation,'UTF8'),'hex'))
      OR e.mass IS DISTINCT FROM c.mass),                                                                 -- 0
 (SELECT count(*) FROM e WHERE (SELECT count(*) FROM token_parent p WHERE p.token_id=e.token_id) IS DISTINCT FROM 1
      OR (SELECT count(*) FROM member_of m WHERE m.token_id=e.token_id) IS DISTINCT FROM 1
      OR NOT EXISTS (SELECT 1 FROM member_of m WHERE m.token_id=e.token_id AND m.group_token_id=ARRAY['00','00','01','00','06'])
      OR NOT EXISTS (SELECT 1 FROM members g WHERE g.token_id=ARRAY['00','00','01','00','06'] AND g.member_token_id=e.token_id)
      OR e.mass IS DISTINCT FROM (SELECT sum(c.mass) FROM token_parent p JOIN token c ON c.token_id=p.parent_token_id WHERE p.token_id=e.token_id)); -- 0
-- C. reciprocal identity, both directions (all must return 0).
--  C1. every endpoint's parent lists that endpoint as its child (token_parent -> token_child):
SELECT count(*) FROM token e JOIN token_parent p ON p.token_id=e.token_id
 WHERE e.token_id >= ARRAY['00','00','02'] AND e.token_id < ARRAY['00','00','03']
   AND NOT EXISTS (SELECT 1 FROM token_child k WHERE k.token_id=p.parent_token_id AND k.child_token_id=e.token_id);       -- 0
--  C2. every endpoint-side child edge of a byte-couplet token is exactly an endpoint whose token_parent points back
--      to it (token_child -> token_parent), so reciprocal links swapped among couplet tokens cannot pass:
SELECT count(*) FROM token_child k
 WHERE k.child_token_id >= ARRAY['00','00','02'] AND k.child_token_id < ARRAY['00','00','03']
   AND NOT EXISTS (SELECT 1 FROM token_parent p WHERE p.token_id=k.child_token_id AND p.parent_token_id=k.token_id);       -- 0
--  C3. each byte-couplet token has exactly one endpoint child and still exactly its one byte-width membership.
--      Expected 0 POST-build; 1,111,936 PRE-build (no couplet token has an endpoint child yet):
SELECT count(*) FROM token c WHERE c.token_id >= ARRAY['00','00','00','01'] AND c.token_id < ARRAY['00','00','01']
 AND ((SELECT count(*) FROM token_child k WHERE k.token_id=c.token_id AND k.child_token_id >= ARRAY['00','00','02']) IS DISTINCT FROM 1
   OR (SELECT count(*) FROM member_of m WHERE m.token_id=c.token_id) IS DISTINCT FROM 1);                                  -- 0
-- D. mass distribution of endpoints: 4 -> 1,920 ; 6 -> 61,440 ; 8 -> 1,048,576.
SELECT mass, count(*) FROM token WHERE token_id >= ARRAY['00','00','02'] AND token_id < ARRAY['00','00','03'] GROUP BY 1 ORDER BY 1;
-- E. fingerprints (§6.4) equal pre/post; and the byte-code/combination masses are still 2 / 4,6,8.
```
(Query C scans by PK range; these are offline checks — the only-follow rule governs runtime reads.)

**Spot-checks (exact expected values):**

| cp | endpoint | notation | mass | parent (combination, notation) | member_of |
|---|---|---|---|---|---|
| U+00E9 é | `00.00.02.00.3l` | `é` (`c3a9` as UTF-8) | 4 | `00.00.00.01.1h`, `C3A9` | `00.00.01.00.06` |
| U+4E2D 中 (3-byte CJK) | `00.00.02.05.Cn` | `中` (`e4b8ad`) | 6 | `00.00.00.07.fl`, `E4B8AD` | `00.00.01.00.06` |
| U+1F600 😀 (4-byte) | `00.00.02.0X.Qm` | `😀` (`f09f9880`) | 8 | `00.00.00.1G.Nk`, `F09F9880` | `00.00.01.00.06` |

and for each: `children_of(combination)` contains the endpoint; the combination is still `member_of` its
byte-width label only; `token_child`/`members` forward-follows reach the endpoint without a scan.

**6.6 Re-export the sharing snapshot.** New dated dir
`data/postgres/snapshots/<date>-endpoint-tier/`: `pg_dump` (v16, plain, `--no-owner --no-privileges`)
`| gzip -9`, `.sha256`, `MANIFEST.md` (new totals; restore instructions describe a normal restore — the earlier
MANIFEST's "trigger re-fires on restore" note is wrong and is being reviewed on #111; if suppressing
referential checks is ever wanted that is `session_replication_role = replica`, not a trigger-ordering
effect), restore-verified into a throwaway DB reproducing
the §6.4 totals; Git LFS for the `.gz`. It supersedes, and does not delete, the 2026-10-03 snapshot.
Commit via the agent-role author convention; separate commits per AGENTS.md (driver / docs / data).

**Failure handling.** Each endpoint is atomic, so a crash or abort leaves only complete endpoints;
the run resumes. A defect found *after* endpoints are written cannot be undone by this plan (no
delete). Hence the rehearsal, the three-endpoint stop-gate, and the private dump (6.0b).

## 7. Forward requirement: a shared endpoint (DEFERRED; open model decision)

Nothing in §7 is built or claimed to work. The schema and addressing do not *cap* an endpoint at one
parent or one membership, and that is all this section establishes. This step uses exactly one parent
(the byte-couplet token) and one membership (`UTF-8`), which is unambiguous now.

**OPEN MODEL DECISION for Patrick (deferred).** `member_of(endpoint, encoding)` and
`token_parent(endpoint, byte-couplet token)` are **independent binary relations**. Once an endpoint has
several parents and several encoding labels, the membership label alone does **not** record *which*
byte-couplet parent belongs to *which* encoding. That association is unresolved. It must be decided
before any second encoding is attached; this plan does not specify it.

**Substrate qualification.** The stored byte-couplet identities are encoding-agnostic and can be shared,
but **current coverage is UTF-8-derived**: only the byte sequences that valid UTF-8 produces were minted.

Illustration of what a later second encoding would face (not a design):

1. *Missing byte-couplet token.* A UTF-16BE table maps U+00E9 to bytes `00 E9`. That 2-byte sequence was
   **never populated** (the built 2-byte tokens all start `C2`–`DF`; live check: no token with notation
   `00E9`), so such a table would first have to **mint** the missing byte-couplet token from the
   byte-code atoms and specify its placement. Only then could it link to the existing endpoint
   `00.00.02.00.3l` (address is identity, so the SEE probe finds it) — by a parent-attach primitive that
   does not exist (below) and a membership in its own label, with the unresolved association above.
2. *Codepoint no endpoint covers.* U+0041 for ISO-8859-1 (parent = byte code `41`, `00.00.00.00.23`):
   `endpoint_address` is defined (`00.00.02.00.13`) and nothing is there, so SEE misses and that encoding
   would mint it with its own parent and label.
3. *Why the address function is encoding-agnostic.* A function mirroring the substrate's layout has no
   slot for U+0041 or surrogates, so a second encoding would have to extend it, changing what "address is
   identity" means.

**What caps one parent / one membership today (checked), and what to build later:**

| layer | caps? |
|---|---|
| schema: `token_parent` PK `(token_id, ordinal)`, `token_child` PK `(parent, child)`, `member_of` PK `(token, group)` | **No.** Only PKs; no unique on notation, no cardinality check |
| trigger | **No.** Sums over all parents (mass consequence, §8) |
| `Controller::add_membership` | **No.** Idempotent, any number of groups |
| `Controller::mint` | **Yes, in effect:** SEE returns before linking; there is no add-parent operation. A new one-level primitive `link_parent(token, parent)` (SEE the pair; insert `token_parent` at next free ordinal + `token_child`; inside the caller's `with_transaction`; trigger recomputes mass) is needed when a second table is built. **Not built here; named only.** (The parent/label association above is a prerequisite decision.) |
| `populate_endpoint` re-run check | **No**, by design: `combination ∈ parents_of(endpoint)` plus whole-integrity (§5), never `size()==1` |
| addressing | **No** once §1.2 is used |

## 0A. Decisions taken (this step)

Resolved by the lead; each is vetoable by the reviewer.

- **Trunk `00.00.02` accepted** — the next free third couplet, "one step up" as delegated.
- **Vocabulary (two tiers, plus a membership).** *Byte couplet tables* = the encoding-agnostic lower
  tier: the byte codes and their 2-/3-/4-byte combinations; the individual tokens are *byte-couplet
  tokens* (HANDOFF/driver call the combinations "characters" and keep those names). *Endpoint* = the new
  tier above (actual-character tokens), each with one byte-couplet token as its structural parent.
  *`UTF-8`* exists only as a **character-level membership label**: an endpoint `member_of` `UTF-8`
  records that its byte-couplet parent is its UTF-8 encoding. It is not a property of the byte tier. New
  driver symbols/modes use `endpoint` (`endpoints`, `endpoints-sample`).
- **`UTF-8` label** (character-level encoding membership) standalone at `00.00.01.00.06`, placeholder mass 10, `member_of` nothing for now —
  the existing label convention; no "encodings" super-label is invented; future encoding labels are siblings and may later be grouped
  under one (deferred).
- **Private non-git `pg_dump`** of `hcp_core` before the full run (§6.0b); kept out of git per the
  private-data rule and distinct from the public snapshot.
- **Upward `token_child` edge** each byte-couplet gains is the additive expansion itself:
  INSERT-only, no byte-tier row content changed (§6.4).
- **Address function** is encoding-agnostic codepoint-as-Base62 (§1.2) — already decided by the pinned
  model; former Q2 is closed.

## 8. Deferred (second-table / future — not this step) and risks

- **Mass doubling** under sum-of-parents if a later table attaches a second parent (é 4→8). Revisit
  before any second table. Structural mass is the SEED only; true mass is the separate
  calculated/centroid layer.
- **Ordinal on an endpoint** = arrival order, not composition position; folds into the already-open
  connection-mechanics item.
- **ASCII U+0000–U+007F endpoints** (parent = the byte code directly); the address function already
  reserves their slots.
- **`notation` column future:** the character in `notation` is a temporary construction post, later
  replaced by the naming-literal reference.
- **`link_parent` attach primitive** (§7) — named, not built.
- **Risk R1** Per-endpoint commits are 1.1 M; fsync dominates (synchronous_commit off is optional).
- **Risk R2** Rehearsal needs a ~4 GB disposable DB; named and dropped only by its `hcp_rehearsal_` prefix.
- **Risk R3** HANDOFF/NOTES/README, `API.md` §9 and the snapshot MANIFEST need updating on execution
  (label 06, tier, totals, vocabulary); none of that is done by this document.
