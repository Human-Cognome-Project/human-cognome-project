# Adversarial review — dispatch + seed + ingestion removal (final integration)

**Reviewer:** fresh, independent adversary (no stake in passing).
**Date:** 2026-09-18.
**Scope:** `dispatch/` (II.7 arraying executor + II.8 verb dispatcher), the
rewritten `seed/seed_0x.cpp`, and the removal of the old `ingestion/` source
(breadcrumb `ingestion/README.md` retained). Consumed modules
(`command/`, `controller/`, `declare/`, `read/`, `update/`) reviewed only for
"not changed semantically". **No file was modified during this review.**

**Method:** read PLAN.md (§I.A/I.F/I.G/I.H, §II.7/II.8, Part V) and NOTES.md
("Arraying is universal"; "Build-phase rulings — firmed 2026-09-18" incl.
"Request transport", DELETE gates); read every changed source and the consumed
headers; **independently rebuilt from scratch and ran every suite** against a
disposable local `hcp3_core` (peer auth, OS user `patrick`); grepped the tree
for removed surface and dangling references.

---

## Independently rebuilt + ran — every suite (verbatim)

Built from clean with `g++ -std=c++17 -O2 -Wall -Wextra` (each module's own
README recipe), each suite run serially against a live, self-reset `hcp3_core`.
**All 11 suites: BUILD-OK, 0 warnings surfaced as errors, exit 0.**

| Suite | Build | Verbatim summary | Exit |
|---|---|---|---|
| codec_test | OK | `PASS codec_test` | 0 |
| codec_advtest | OK | `PASS codec_advtest` | 0 |
| command_ir_test | OK | `PASS command_ir_test` | 0 |
| span_planner_test | OK | `PASS span_planner_test` | 0 |
| controller_test | OK | `PASS controller_test` | 0 |
| controller_advtest | OK | `PASS controller_advtest` | 0 |
| declare_core_test | OK | `PASS declare_core_test` | 0 |
| read_core_test | OK | `PASS read_core_test` | 0 |
| update_core_test | OK | `PASS update_core_test` | 0 |
| seed_0x | OK | `PASS seed_0x -- seed floor landed and read back clean` | 0 |
| dispatch_test | OK | `PASS dispatch_test` | 0 |

**No `FAIL` line anywhere in the combined output.**

`seed_0x` verbatim (key lines): `0x` minted mass 0; 16 hex atoms mass 1 at
`AA.AA.AA.AA.AB..AA.AA.AA.AA.AR`; `token_text == "AA.AA.AA.AA.AA"`;
`notation == "0x"`; `row counts: token=17 token_parent=0 token_child=0
members=0 member_of=0`; all 25 checks `ok`.

`dispatch_test` verbatim (32 checks, all `ok`): DECLARE/READ/MOVE/ADD_CONNECTION
each dispatch to the right `Verb::` and mutate the store; gated DELETE_RECORD
(match deletes / mismatch rejects + leaves token untouched) and DELETE_CONNECTION
(match deletes, reciprocal gone); RECONCILE/UPDATE_CACHE/REBASE_CACHE hit the
non-fatal stubs; 2-item ordered stream (DECLARE→MOVE of what it placed); **4-verb
chained stream DECLARE→READ→MOVE_RECORD→ADD_CONNECTION**, each item depending on
the prior, final store state reflects all four in order.

## Dead / removed-surface grep (whole `db_kernel` tree)

`grep -rnE 'add_group_membership|groups_of|token_sibling_group|token\.type'`
plus a `mint(`-with-type scan and a tree-wide `Ingestor|db_runtime|db_ingest`
scan. Result: **no live reference to removed surface.**

- `groups_of…` hits are all **local variable names** whose RHS is
  `ctl.member_of(...)` (the new door read) — not the removed `groups_of()` method.
- `token_sibling_group` / `token.type` hits are **negative assertions** in
  `schema/verify.sql` and `controller_test.cpp` ("no longer exists") and prose
  comments — no code path.
- `add_group_membership` appears once, in a `controller.h` comment
  ("replaces add_group_membership"). No call site.
- Every `mint(` call is the 4-arg `mint(id, notation, constituents[, mass])`
  form — no `type` argument anywhere (the parameter is gone).
- `Ingestor` / `db_runtime` / `db_ingest`: **no reference in the whole engine
  tree** except inside `ingestion/README.md` (the breadcrumb itself).

---

## Findings

No must-fix. All items below are informational; none blocks the stage.

**F-1 — INFO — II.7 arraying is realized split intra-core / cross-command
(`dispatch/dispatch.h`, `dispatch.cpp`, `README.md`).**
PLAN §II.7 lists the executor's job as "frame broadcast, co-index zip, ordered
execution, SEE collapse." The implementation does **not** re-perform
broadcast/co-index/SEE in the executor; those already live inside each single-op
core (DECLARE's `PARENTS` N-loop, MoveRecord's `sources`, AddConnection's
`elements`, all SEE-idempotent via the door — independently confirmed in the
consumed sources). `dispatch_stream()` adds only the cross-command ordered
serial piece (N distinct verb invocations, in order, one Result each). *Evidence:*
`dispatch.cpp:36-51`; README "What the arraying executor adds". *Spec:* PLAN §I.F,
§II.7; NOTES "Arraying is universal" (an array IS a compressed serial stream;
ordering is semantic). *Assessment:* faithful — see ruling below; noted so Patrick
can veto if he intended one monolithic executor object rather than this
allocation.

**F-2 — INFO — DELETE confirm mismatch is rejected by the update core, not by
`dispatch_one` (`dispatch.h:45-59`, `dispatch.cpp:18-25`).**
`DeleteRecordRequest`/`DeleteConnectionRequest` bundle `op`+`confirm` and pass
both to `update::delete_*`, which enforces the full-target gate; dispatch adds no
rejection path of its own. This is consistent with the stated discipline ("every
core re-validates its own IR defensively; this function performs no validation").
Test proves the gate fires (mismatched confirm → `!deleted`, token untouched).
Faithful; no action.

**F-3 — INFO — working tree was edited mid-review.** `dispatch/dispatch_test.cpp`
grew from 329 to 378 lines during the review (mtime 09:58), adding the 4-verb
chained-stream case. This review reflects the **current 378-line state**, which is
strictly stronger. Flagged only so the record is unambiguous about which snapshot
was judged. `update/update_core_test` binary sits in the tree but is
**git-ignored** (not tracked) — no leak.

---

## Scrutiny points (brief's five)

**1. Arraying executor (II.7) — RULING: FAITHFUL.**
`AdditiveCommand = variant<DeclareRecord, ReadRecord, MoveRecord, AddConnection>`
— DELETE and the cache stubs are **structurally excluded** (not variant
alternatives), so a `vector<AdditiveCommand>` *cannot represent* a DELETE: "DELETE
is the sole non-batchable op" is a type invariant, not a runtime check
(`dispatch.h:76-77`). Ordered execution is single-threaded, synchronous, in input
order (`dispatch.cpp:36-51`), proven with **genuinely dependent** cases: the
2-item DECLARE→MOVE-of-what-it-placed and the 4-item
DECLARE→READ→MOVE→ADD_CONNECTION chain, each item succeeding only because the
prior committed. The "intra-command arrays already in the cores, executor =
cross-command only" split (F-1) is a **faithful** realization of "arraying is
universal / all-but-DELETE batchable": all four additive verbs are batchable
through the stream, DELETE is structurally barred, and the broadcast/co-index/SEE
machinery the spec names demonstrably exists in the cores (re-doing it in the
executor would duplicate their loops). The allocation is legitimate code-structure
latitude explicitly granted by PLAN §I.A ("dispatch attachment, module layout is
the implementing agents' code-structure latitude"). Semantics match spec exactly.

**2. Dispatch surface (II.8) — faithful.** All PLAN §I.A verbs routed: DECLARE→
`declare::execute`, READ→`dbread::read`, MOVE→`update::move_record`,
ADD_CONNECTION→`update::add_connection`, DELETE_RECORD/DELETE_CONNECTION→the gated
`update::delete_*` (op+confirm bundled), RECONCILE/UPDATE_CACHE/REBASE_CACHE→
non-fatal `"<VERB>: not yet implemented"` stubs that never touch `ctl`. DELETE
honours the update-core full-target gate (mismatch rejects — tested). No external
wire/text parser: dispatch is over in-process `command::` IR only (G6 stays
deferred). One Result per verb (`dispatch_one`), one Result per stream item in
order (`dispatch_stream`). Result signatures match the consumed headers exactly.

**3. seed — faithful.** Mints `0x` (mass 0) and 16 hex atoms (mass 1) **only**
through `Controller::mint`'s optional-mass bootstrap channel (never
`declare::execute`/`dispatch::`); sequenced by `command::successor`
(`AA.AA.AA.AA.AB..AR`); resets and reapplies `schema.sql` once; reads back clean
against the rebased schema (`members`/`member_of` present and empty; 17 token
rows; 0 structure/membership rows). Verified live.

**4. ingestion removal — RULING: FAITHFUL.** The removed files were **uncommitted
pre-rebase** (`git status` = `AD`: staged-add, deleted-in-worktree — never in a
commit), so no committed work is lost. They called door surface the rebase removed
(`add_group_membership`, `mint`'s `type` param) and could not compile without
reintroducing it; reworking `db_runtime`'s one-request-per-line dispatch to the
firmed nested/arrayed grammar would require the **deferred G6 wire parser**
(PLAN §I.H; NOTES "Request transport — never one-request-per-line"). Every
capability is genuinely subsumed: intake by `command/` + `declare/`; verb routing
by `dispatch/`; seed by `seed/seed_0x.cpp`. The breadcrumb README is accurate
(replacement modules all exist; the one-shot CLI's absence is correctly stated as
G6-blocked). **Nothing needed-now is lost.** No dangling reference to the removed
files anywhere in the engine tree.

**5. Scope — clean.** `command/`, `controller/`, `declare/`, `read/`, `update/`
are **committed and unmodified** in the worktree (dispatch consumes them
unchanged). No OLD-MODEL-LEAK anywhere: no `TYPE` gating, no
`token_sibling_group`/`groups_of`/`add_group_membership`/`mint`-with-type on any
code path (see grep above).

---

## Verdict: **PASS**

Every suite rebuilds clean and passes against live Postgres (exit 0, zero FAIL
lines). The dispatcher routes the full §I.A verb surface with DELETE structurally
non-arrayable and gated; the arraying executor proves ordered cross-command
dependence over exactly the four additive verbs; the seed floor lands and reads
back clean via the sole declared-mass channel; the ingestion removal drops only
uncommitted, subsumed, removed-surface-dependent code and leaves an accurate
breadcrumb with no dangling references. Findings F-1/F-2/F-3 are informational.
