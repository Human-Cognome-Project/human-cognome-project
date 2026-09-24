# Git realignment & cleanup plan — 2026-09-24

**Status:** VETTED (adversary FIX-THEN-EXECUTE; all 5 defects folded in) — awaiting Patrick's
approval of the §5 branch deletions. Nothing below has run.
**Branch:** `dbkernel-design-checkpoint` (current). **Author:** coordinator (Opus 4.8), for Patrick.
**Mandate (Patrick, 2026-09-24):** clean up/correct/update git and remote properly; structure
the repo like a proper git with documentation and experiment tracking; archive/remove deprecated
work. 5.5 experiment is out-of-repo and out of scope. Nuke planner-created work in field/ (and
ledger/ — but see FLAG-2).

Every factual claim below is reproducible with the command in its row. The adversary should
re-run and confirm each, and challenge the classifications and the deletion scope.

---

## 1. Verified git state (facts)

| Claim | Verify with |
| --- | --- |
| Remote = `git@github.com:Human-Cognome-Project/human-cognome-project.git`; fetch + push work | `git ls-remote --heads origin`; `git push --dry-run origin dbkernel-design-checkpoint` → "Everything up-to-date" |
| Current branch clean & fully pushed (db_kernel work through 09-23 safe) | `git status -sb` |
| Local `main` is 11 behind origin/main; the 11 are legit **2026-08-31** agent commits (Silas/Planner two-half field engine), NOT 5.5 | `git log --format='%h %an %ad %s' --date=short main..origin/main` |
| Stray worktree at `/home/patrick/.claude/jobs/1fb44dc1/tmp/wt-main`, branch `weld-main` = origin/main, clean, 0 ahead | `git worktree list`; `git -C <path> status -sb` |
| `origin/forces-slice-1` fully merged into main (26 behind, 0 ahead) | `git rev-list --left-right --count origin/main...origin/forces-slice-1` |
| `origin/worktree-agent-a9f49486` — 1 commit (2026-06-05, Patrick-authored), 63 behind | `git log main..origin/worktree-agent-a9f49486` |
| `origin/engine/v0-staging` — 27 ahead of main, real work (KEEP) | `git rev-list --left-right --count origin/main...origin/engine/v0-staging` |
| `planner/field-engine-config` (local+remote) — **3 unique commits** (2026-09-02), off-direction planner field-engine work | `git rev-list --left-right --count origin/main...origin/planner/field-engine-config` → "N  3" |
| `origin/azsl-settle-slice-1` (local+remote) — stale, fully merged (0 ahead, 29 behind) | `git rev-list --left-right --count origin/main...origin/azsl-settle-slice-1` → "29  0" |

## 2. Untracked working-tree classification (43 items)

| Group | Contents | Verdict |
| --- | --- | --- |
| Engine harness — source | `engine/{src,apps,tests,cmake,scripts}`, `engine/CMakeLists.txt`, `engine/.clang-format`, `engine/.gitignore`, `engine/README.md` | **COMMIT** — hosts `db_kernel/` (already tracked); host must be tracked too |
| Engine harness — docs | `engine/docs/*` (OPERATIONAL-PLAN, HARNESS-NOTES, VETTING-PLAN, …) | **COMMIT** — current engine operating discipline (referenced by coordinator memory) |
| Build artifacts | `engine/build/` (27 MB compiled: `*.a`, ninja, CMakeCache, binaries) | **GITIGNORE** — never commit |
| Working notes | `docs/*-notes.md` (bonding, particle-geometry, field-physics, parent-structure, storage-split) | **COMMIT** as documentation |
| Review pass | `review/pass-01..05`, `decisions.md`, `execution-plan-*`, `issue-ledger-*`, `next-step-readiness-*`, `ledger-working-set-map`, `fixtures/` | **COMMIT** as documentation |
| Field planner outputs | 23 untracked `field/*-mint.json` / `*-report.json` (+ already-ignored `*.npz`/`*.bak`/`__pycache__`) | **NUKE** (per mandate) |

## 3. Deletion scope — EXACT

**Intent (Patrick):** the planner's field-engine work does not belong in the repo — remove it.
The partition is planner-created field-engine files vs. Patrick's own field/ledger material.

**Evidence note (vet-corrected):** git-*author* for all 10 tracked `field/` files is Patrick — so
git-author does NOT discriminate here. The discriminator is **disk-owner `planner` + 2026-09-02
mtime + the `planner/field-engine-config` branch** the `.py` originate from. Removal is
**recoverable** (git history for the tracked `.py`; the `planner/field-engine-config` branch until
step 5 deletes it too). ledger/ is the one place both signals agree (all Patrick).

**NUKE — all planner-created field-engine work in `field/`:**
- Untracked outputs (working-tree `rm`): 23 `*-mint.json` / `*-report.json`
  (`git status --porcelain field`), plus ignored `*.npz`, `*.bak-*`, `__pycache__/`.
- Tracked prototype (`git rm`): the `field/field_engine*.py` family — `field_engine.py`,
  `field_engine_kernel_v0.py`, `field_engine_load_v0.py`, `field_engine_review.py`,
  `field_engine_v0.py` (planner's Python field-engine; AGENTS.md keeps Python out of the hot path).
  Removal is recoverable from history if ever needed.

**KEEP — Patrick's own field/ material:** `field_attraction.png`, `field_attraction_writeup.md`,
`field_model.py`, `planck_to_newton.md`, `RP_amount_ledger_rereading.md`.

**ledger/ — no action.** Every file is Patrick's (git-author + disk-owner); no planner work exists
there to remove. Nothing to do, no decision needed.

*(Adversary: verify this planner-vs-Patrick partition is correct and complete — that no planner
file is left behind and no Patrick file is caught in the nuke.)*

## 4. Execution order (nothing runs until approved — reordered per vet Defect-1)

1. **.gitignore hygiene** (non-destructive): add `**/build/` + compiled artifacts. (`engine/build/`
   is already covered by the untracked `engine/.gitignore`, which step 2 commits — verify the commit
   preserves that protection.)
2. **Commit harness + docs FIRST** (before any clean, so nothing uncommitted is at risk) on
   `dbkernel-design-checkpoint`: engine source, `engine/docs`, `docs/*-notes.md`, `review/*` (incl.
   this plan). Role-authored, project email. NB (vet): this commit does not touch `engine/build/`
   (ignored). Layout-collision with main's `engine/{kernel,storage,timestep}` is a later
   main-integration concern, not now (see §2 note).
3. **Nuke planner field-engine work in `field/`** (§3), now that step 2 has committed everything
   else: `git rm` the 5 tracked `field/field_engine*.py`; then `git clean -fx field/` for the
   untracked+ignored planner outputs (`*.json`, `*.npz`, `*.bak-*`, `__pycache__/`). **Scoped to
   `field/` and `-x` are mandatory** — a bare/wide clean is forbidden. Tracked KEEP files are
   untouched by `git clean` (they are tracked). Commit the `git rm`.
4. **Fast-forward local `main`** → origin/main (ff-only, safe; the 11 Aug commits).
5. **Branch/worktree cleanup** (all deletions below need Patrick's go — §5):
   - Prune stray worktree `wt-main` + branch `weld-main` (0 unique — safe).
   - Delete `forces-slice-1` (0 unique — safe) and `azsl-settle-slice-1` (0 unique — safe), local+remote.
   - Delete `origin/worktree-agent-a9f49486` (1 Patrick commit, 2026-06-05 — Patrick: no use).
   - Delete `planner/field-engine-config` local+remote (**3 unique commits** — off-direction planner
     field-engine work per mandate; removing it also drops the last recoverable copy of the `.py`).
6. **No experiment-tracking machinery** (Patrick): off-direction material removed, not restructured.
   Not a claim that experimental results are useless — there is simply no category to track. Dropped.

## 5. Branch-deletion approvals (Patrick's call — the only thing gating execution)

Resolved:
- `origin/worktree-agent-a9f49486`: **delete** (old, no use).
- Experiment-tracking: **none** (step 6).

New, surfaced by the vet — recommend delete, need your go:
- `azsl-settle-slice-1` (local+remote): fully merged, 0 unique — **safe delete**, recommend delete.
- `planner/field-engine-config` (local+remote): **3 unique commits**, but it IS the off-direction
  planner field-engine work you said to remove. Recommend **delete**; flagging because it holds
  unique commits (after this + the tracked `.py` removal, that work is gone save reflog). Say the
  word or say "archive it first" and I'll tag/bundle before deleting.

## 6. Vetting

- [x] Adversary vet (fresh, independent, read-only): **FIX-THEN-EXECUTE**; 5 defects, all folded in
  above (clean-ordering reordered; the two missed branches added; §3 evidence corrected; layout
  collision noted). No defect left unaddressed.
- [ ] Patrick approval of the §5 branch deletions — the only open gate.

