# Adversarial re-review #4 — `command/` grouping-node naming-literal rulings

**Reviewer:** fresh independent adversary (no stake in passing).
**Date:** 2026-09-17.
**Target (untouched working tree):** `/opt/project/repo/engine/db_kernel/command/`.
**Governing spec (recording the rulings):** `NOTES.md` "Build-phase rulings —
firmed 2026-09-17" → "Grouping-node naming literal (label DECLARE)" (700-721);
`PLAN.md` §I.B "Grouping-node naming literal" (125-139), §I.G (218-221), §II.3
(299-311); `NOTES.md` "Relationship model & type — firmed 2026-09-17".

Delta since re-review #3 (mtime + byte size): `command_ir.h` (14512→15628),
`command_ir.cpp` (13691→15922), `command_ir_test.cpp` (27937→32736), `README.md`
(10782→12755). `span_planner.h/.cpp` **byte-identical and unchanged** (mtime 09:51),
`span_planner_test.cpp` unchanged, `codec/*` untouched. Scope confined to
`command/`; the `NOTES.md`/`PLAN.md` edits are the team lead recording spec.

---

## 1. Test build + run — VERBATIM

```
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/cir ../codec/codec.cpp command_ir.cpp span_planner.cpp command_ir_test.cpp && /tmp/cir
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/sp  ../codec/codec.cpp command_ir.cpp span_planner.cpp span_planner_test.cpp && /tmp/sp
```

Both compiled with **zero warnings** under `-Wall -Wextra`, zero errors.
`command_ir_test` — **63/63 `ok`** (was 58; +5), `PASS command_ir_test`, exit 0.
`span_planner_test` — **31/31 `ok`**, `PASS span_planner_test`, exit 0.

## 2. Verification against the recorded spec

### #1 — Grouping-node forms → FAITHFUL
Recorded: `NOTES.md:700-712`, `PLAN.md:125-139` — ADDRESS **permitted** on a
grouping node; **Use-provided-ID** (existing ADDRESS, PARENTS absent →
reference); **Mint** (PARENTS present → mint from that composition, placed at
target ADDRESS if given else manager-placed); **neither PARENTS nor ADDRESS →
reject**; NOTATION optional/human-facing, never the handle.

Code (`command_ir.cpp:88-185`):
- Old "grouping node may not carry own ADDRESS" and "grouping-only requires
  non-blank NOTATION" rules are **removed** — grep confirms both phrases gone.
- `has_members && !has_parents && !node.address` → reject (`171-176`). ✓ the
  neither-PARENTS-nor-ADDRESS case.
- **Mint form** (`has_members && has_parents`): both the `171` and `181` rejects
  are skipped ⇒ ADDRESS optional. Present ⇒ validated through the normal
  cover-N/`plan()` path against N=1 (`220-252`); absent ⇒ no ADDRESS block ⇒
  manager-placed. ✓
- **Use-provided-ID** (`has_members && !has_parents && address`): `171` skipped;
  ADDRESS validated as form only via `validate_address_span_nested` + `plan(·,1)`
  — no store-existence check (that is execution-time). ✓
- **Plain structure node** (`has_parents && !has_members`): ADDRESS still required
  (`181-185`); N = `parents->size()` may be >1 (no ruling-#2 clamp). ✓ Unchanged.

### #2 — Mixed node N=1 → FAITHFUL
Recorded: `NOTES.md:718-719` — "N>1 for a mixed node (a naming-literal mint is a
single literal, N=1)", reject-until-ruled. Code `command_ir.cpp:116-127`:
`has_members && parents->size() != 1` → reject. A plain structure node
(`has_members` false) is not clamped, so N>1 remains legal there. ✓ (Test 180
uses a genuinely N=2 PARENTS whose 2-slot ADDRESS *matches* N=2, so the rejection
is attributable to ruling #2, not cover-N — a clean test.)

### #3 — nested-declare-in-ADDRESS collides with PARENTS → FAITHFUL, no leak
Recorded: `NOTES.md:719-721`, `PLAN.md:137-138` — an ADDRESS-span nested-declare
colliding with a slot's own PARENTS composition (would silently drop that slot's
constituents), reject-until-ruled. Code `command_ir.cpp:220-238`: when
`has_parents`, any `kNestedDeclare` ADDRESS segment → reject **before** the
validity recursion, so it fires "regardless of the nested declare's own validity"
(test 197 proves this with a *well-formed* nested declare). ✓
**No leak into MOVE:** the ruling-#3 loop lives only in `validate_declare`.
`validate_move_record` (`312-359`) routes to `validate_span_shape` (span_planner,
unchanged) for wildcard/range sources and `validate_address_span_nested` + `plan`
for all-explicit — neither rejects nested-declare segments. MOVE keeps the full
grammar (re-review #2's test at `command_ir_test.cpp` still asserts a nested-
declare + undeclared-hook MOVE destination is accepted). ✓

### #4 — NOTATION optional everywhere → FAITHFUL
No grouping-requires-NOTATION rule remains; the only NOTATION rule is the
co-index length==N when present, blanks legal (`command_ir.cpp:191`;
`NOTES.md:716-717`). Tests 137 (grouping blank NOTATION accepted), 218 (structure
blank accepted), 226 (length mismatch rejected). ✓

## 3. Ruling on the flagged interpretation — FAITHFUL (not too loose, not too strict)

**Mint-form ADDRESS routed through the SAME cover-N/`plan()` path as a plain
structure node's ADDRESS, against N=1.** This is faithful. The recorded spec calls
the mint-form ADDRESS the *placement target* of the minted naming literal
(`NOTES.md:707-711` "placed at the target address if given"; `PLAN.md:130,304`),
and ruling #2 fixes the mint at a **single** literal (N=1). `plan(address, 1)`
validates exactly "this target places one token": it admits every single-slot
span form (direct, pin, closed FROM..TO of length 1, elastic FROM filling 1, an
AFTER pending-seam) and rejects only a target that would place ≠1 token — which is
the correct constraint for placing one naming literal, not an over-requirement.
Looser (skip cover-N) would wrongly accept a multi-slot target for a single mint.
Routing it through the `has_parents` path also correctly subjects the mint target
to ruling #3 (a nested-declare target would silent-drop PARENTS[0]). Faithful.

## 4. No regression — CONFIRMED

- **MoveSource / wildcards / cover-N split / DELETE**: `validate_move_source`,
  `validate_move_record`, `validate_read`, `validate_add_connection`,
  `validate_delete_*` are unchanged from re-review #3 (verified by reading; the
  wildcard/terminal-only behaviour is codec-enforced and intact). Still correct.
- **inf1** (N=1 for the naming-literal): now **spec-backed** by `NOTES.md:718-719`
  (mint = single literal, N=1); pure-grouping use-provided-ID N=1 = one referenced
  naming literal. FAITHFUL. *(Note: the code comment at `command_ir.cpp:101-108`
  still calls N=1 "this module's bookkeeping choice, not a stated spec value —
  flagged in the handoff report", which is now stale — see OBS-1.)*
- **inf2** (grouping-only requires non-blank NOTATION): its premise is **removed
  by ruling #1** (NOTATION is never the handle; identity comes from ADDRESS or
  PARENTS). This is a legitimate spec **supersession**, not a regression — the
  code correctly no longer enforces it.
- **inf3** (`Reference` exactly one of {address, nested}): unchanged →
  CODE-STRUCTURE.
- **OLD-MODEL-LEAK**: CLEAN. Grep over `command_ir.h/.cpp` finds only the benign
  `command_ir.h:33` "sibling-in-this-statement" (a peer-address reference, not the
  dropped sibling-group edge). No TYPE gating, no label-CHILDREN, no token_child.

## 5. Observations (no action required)

- **OBS-1** — stale comment: `command_ir.cpp:101-108` still describes N=1 as an
  unspecified module choice "flagged in the handoff report", now superseded by the
  recorded ruling. Doc-only; harmless; worth tidying.
- **OBS-2** (carried) — `codec::is_valid_address({})` is true, so an empty address
  passes `is_explicit_address`; latent, codec-level, pre-existing.
- A nested-declare inside a *use-provided-ID* grouping node's ADDRESS (PARENTS
  absent) is permitted and recursively validated (ruling #3 is `has_parents`-scoped).
  Not covered by the three rulings and pre-existing span behaviour; noted for
  completeness, no action.

---

## Verdict — **PASS**

All three grouping-node rulings are implemented faithfully against the now-recorded
spec: (#1) the two-form grouping-node model with ADDRESS permitted, mint-form
ADDRESS optional, use-provided-ID form validated as form only, neither-PARENTS-nor-
ADDRESS rejected, and the old ADDRESS-forbidden/NOTATION-required rules removed;
(#2) mixed PARENTS+MEMBERS clamped to N=1 while plain structure nodes keep N>1;
(#3) a nested-declare ADDRESS segment rejected unconditionally when PARENTS is
present, with no leak into MOVE. NOTATION is optional everywhere (only length==N
co-index when present). The mint-form-ADDRESS interpretation (cover-N/`plan()`
against N=1) is faithful. Tests are meaningful and green (63/63, 31/31) with a
clean `-Wall -Wextra` compile; MoveSource/wildcards/cover-N split/DELETE,
inf1/inf3, and the OLD-MODEL-LEAK check show no regression (inf2 is legitimately
superseded by ruling #1); production changes are confined to `command/`. Ready.
