# Adversarial re-review #3 — `command/` against the now-RECORDED wildcard/MOVE rulings

**Reviewer:** fresh independent adversary (no stake in passing).
**Date:** 2026-09-17.
**Target (untouched working tree):** `/opt/project/repo/engine/db_kernel/command/`.
**Governing spec (now recording the rulings):** `NOTES.md` "Build-phase rulings —
firmed 2026-09-17" (lines 684-737) + "Relationship model & type — firmed
2026-09-17"; `PLAN.md` §I.C (141-156), §I.D (158-184), §II.5 (311-327);
`codec/codec.h` + `codec/codec.cpp`.

This review re-adjudicates re-review #2's G-1/G-2/G-3 against the recorded spec
(the fix was Patrick recording the rulings, not a code revert). build-2-ir's
delta since re-review #2: **`command_ir_test.cpp` (24160→27937) and `README.md`
(9389→10782) only** — the four production files (`command_ir.h/.cpp`,
`span_planner.h/.cpp`) are byte-identical and unchanged (mtime 09:51-09:56),
confirming NO production change (state already matched the rulings). `NOTES.md`/
`PLAN.md` are modified by the team lead (spec recording), not build-2-ir; nothing
else in `command/` moved. `codec/*` untouched.

---

## 1. Test build + run — VERBATIM

```
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/cir_test ../codec/codec.cpp command_ir.cpp span_planner.cpp command_ir_test.cpp && /tmp/cir_test
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/sp_test  ../codec/codec.cpp command_ir.cpp span_planner.cpp span_planner_test.cpp && /tmp/sp_test
```

Both compiled with **zero warnings** under `-Wall -Wextra`, zero errors.
`command_ir_test` — **58/58 `ok`** (was 50; +8), final `PASS command_ir_test`, exit 0.
`span_planner_test` — **31/31 `ok`**, final `PASS span_planner_test`, exit 0.
Independently reproduced.

## 2. Per-op wildcard re-adjudication against the RECORDED spec

All three prior findings now trace to recorded governing text → **FAITHFUL**.

### G-1 — MOVE terminal-wildcard (`kPrefix`) source → **FAITHFUL** (was NEEDS-PATRICK)
- **Recorded:** `NOTES.md:695-703` — "MOVE — 'from→to' is the relocation
  RELATIONSHIP, not a command form ... **source-selection and
  destination-placement are DISTINCT operations** ... Source: explicit token(s), a
  `FROM..TO` range, or a **terminal-wildcard prefix** selecting a whole
  trunk/branch (MOVE's primary use: bulk correction) ... **Only-follow — a
  prefix/range names a contiguous addressed region, *walked*, not
  predicate-searched.**" `PLAN.md:160-168` mirrors it ("MOVE is the one op whose
  source may be a wildcard/range").
- **Verdict:** the `kPrefix` source now has explicit spec backing, and my prior
  only-follow/never-search concern is directly answered by the recorded "walked,
  not predicate-searched." `MoveSource::Kind::kPrefix` traces to spec. FAITHFUL.

### G-2 — READ terminal-wildcard exclusions → **FAITHFUL** (was NEEDS-PATRICK)
- **Recorded:** `NOTES.md:708-714` — "READ — terminal wildcards permitted
  (nominal, tree-constrained). The anchor and **exclusions MAY be terminal
  wildcards** ... a **nominal** search — constrained to walk connected factors ...
  permitted, distinct from the forbidden arbitrary property-predicate. **This
  refines the earlier 'exclusions = specific token_ids only': specific token_ids
  OR a terminal-wildcard tree region; never a property predicate.**" `PLAN.md:150-152`.
- **Verdict:** the exact conflict I cited (`NOTES.md:662`) is now explicitly
  refined by the ruling — a terminal-wildcard exclusion is permitted; only a
  property predicate is forbidden. FAITHFUL.

### G-3 — READ anchor + ADD_CONNECTION endpoints terminal-wildcard → **FAITHFUL** (was NEEDS-PATRICK)
- **Recorded:** READ anchor — `NOTES.md:708-709`, `PLAN.md:141-143` "a single
  token_id, **or a terminal wildcard** to read a range/full construct (a nominal,
  tree-constrained read)." ADD_CONNECTION — `NOTES.md:716-723` "ADD_CONNECTION —
  terminal wildcards, either side ... **Endpoints still must pre-exist; the
  wildcard resolves to the definable address range at execution (UPDATE core),
  which enumerates the pairs.**" `PLAN.md:176-179`.
- **Verdict:** my pairwise/endpoints-must-pre-exist concern is answered — the
  wildcard is a bulk selector resolved to concrete pre-existing pairs at execution
  (Agent 5); the IR layer only validates it is a well-formed terminal wildcard.
  FAITHFUL.

### DELETE_RECORD / DELETE_CONNECTION explicit-only → **FAITHFUL** (unchanged)
- **Recorded:** `NOTES.md:725-729`, `PLAN.md:182-183` — "explicit only, no
  wildcards ... specific ids/pairs only (the destructive gate)." Enforced by
  `is_explicit_address` in `validate_delete_record`/`validate_delete_connection`.

## 3. Terminal-only enforcement — CORRECT, and tests are MEANINGFUL

The ruling's load-bearing constraint is **"Wildcards are TERMINAL ONLY — no
inline wildcards"** (`NOTES.md:689-693`; `PLAN.md:152`). The module enforces it
transitively through the codec: `codec::is_valid_address` (`codec/codec.cpp:92-103`)
returns false for any partial element that is not the address's last element. Every
validator gates on `is_valid_address` (READ/ADD directly; MOVE `kPrefix` via
`is_valid_address && is_wildcard`, `command_ir.cpp:265-270`), so an inline wildcard
is rejected everywhere while a terminal wildcard (single- or multi-element) passes.

`test_wildcards_are_terminal_only` (`command_ir_test.cpp:490-541`) exercises this
directly and non-vacuously — it constructs and asserts real accept/reject:
- multi-element terminal wildcard `{full,partial}` ("AA.B\*") → `is_valid_address`
  **true** (494);
- inline wildcard `{partial,full}` → `is_valid_address` **false** (500);
- inline wildcard **rejected** by READ anchor (506), READ exclusions (513),
  ADD group (519), ADD elements (525), MOVE `kPrefix` source (531);
- multi-element terminal wildcard **accepted** by MOVE `kPrefix` (538).

`test_read_and_add_connection_permit_terminal_wildcards` (452-482) asserts a
terminal wildcard is **accepted** (`kValid`) across READ anchor, READ exclusions,
ADD group, ADD elements. Together these cover exactly the matrix the task names
(inline rejected across READ anchor+exclusions, ADD group+elements, MOVE kPrefix;
terminal — incl. multi-element — accepted). Meaningful, not vacuous.

## 4. No regression — CONFIRMED

- **Cover-N split** (`command_ir.cpp:277-324`): all-`kExplicit` ⇒ N =
  `sources.size()` ⇒ `plan()` cover-N (kInvalid rejected); any non-explicit ⇒
  `validate_span_shape` only, cover-N deferred; mixed defers whole statement.
  Unchanged, correct, and now matches `NOTES.md:704-706` / `PLAN.md:167-168`
  verbatim.
- **MoveSource well-formedness** (`command_ir.cpp:243-273`): explicit/range
  concrete, range forward-same-depth (`span_length`), prefix must be a partial
  (terminal) address. Unchanged, correct.
- **DELETE explicit-only**: unchanged, correct.
- **inf1 / inf2 / inf3**: production unchanged → **FAITHFUL / FAITHFUL /
  CODE-STRUCTURE**, not regressed.
- **OLD-MODEL-LEAK**: production files byte-identical to re-review #2 → still
  **CLEAN** (no TYPE gating, no sibling-group, no label-CHILDREN, no token_child
  membership).
- **Scope**: build-2-ir changed only `command_ir_test.cpp` + `README.md` within
  `command/`; the `NOTES.md`/`PLAN.md` edits are the team lead recording spec.

## 5. Agent-5 (UPDATE/READ core) execution-time deferrals the module relies on

The IR layer validates *shape only*; these resolutions are correctly deferred and
now recorded in `PLAN.md §II.5`:
1. **Wildcard EXPANSION/RESOLUTION.** A terminal wildcard in READ (anchor/
   exclusions), ADD_CONNECTION (group/elements), and MOVE (`kPrefix` source)
   resolves to its concrete address range/token set against the live store at
   execution — ADD enumerates the concrete pairs (`PLAN.md:320-323`;
   `NOTES.md:722-723`), READ walks the tree region, endpoints must still pre-exist.
2. **MOVE wildcard/range source RESOLUTION + its cover-N.** For a range/prefix
   source the unit count N is store-resolved; the UPDATE core resolves the
   occupied token set then enforces destination-covers-resolved-N
   (`PLAN.md:315-316`; `NOTES.md:703-706`). The IR skips static cover-N in this
   case by design.
3. **Reject a mint-bearing MOVE destination** (prior G-5/F-1's deferred half).
   The IR `validate_span_shape` permits nested-declare / undeclared-hook
   destination segments; the UPDATE core **must reject** them (a relocation mints
   nothing) — now explicitly recorded as Agent 5's job (`PLAN.md:317-319`).

## 6. Observation carried forward (no action here)

- **Empty address** (prior G-4): `codec::is_valid_address({})` is true, so
  `is_explicit_address({})` is true — an empty token passes DELETE / MOVE-explicit.
  Latent, codec-level, pre-existing, unaffected by this work. Flag only.

---

## Verdict — **PASS**

The three re-review-#2 findings are resolved by rulings now recorded in the
governing spec, and the code traces to that text: MOVE terminal-wildcard source
(G-1), READ terminal-wildcard exclusions (G-2), and READ anchor + ADD_CONNECTION
terminal-wildcard endpoints (G-3) are all **FAITHFUL**; DELETE stays explicit-only
(**FAITHFUL**). The terminal-only constraint is enforced (via the codec) and
pinned by meaningful new tests (inline rejected across every field, terminal and
multi-element terminal accepted). Both suites are green (58/58, 31/31) with a clean
`-Wall -Wextra` compile; cover-N split, MoveSource well-formedness, DELETE
explicit-only, inf1/inf2/inf3, and the OLD-MODEL-LEAK check show no regression;
the production code was not changed (state already matched), and the Agent-5
execution-time deferrals the module leans on are all recorded in PLAN §II.5. The
module is spec-faithful and ready.
