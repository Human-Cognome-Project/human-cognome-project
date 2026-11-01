# Orientation

**New coding environment or agent:** start with the [fresh-instance onboarding/spec brief](AGENT-START.md).

HCP models explicit objects and their relationships using field mechanics.
The engine calculates their relative motion; storage supplies reusable
definitions, and a harness controls execution. Analyst reasoning remains future
work. Read [current state](../../AGENTS.md#current-state) for what is built,
accepted, held or still proposed.

## Objects, occurrences and fields

A definition describes reusable structure. A live occurrence is one copy in a
study, with its own execution identity and state. Two copies of the same letter
or assembled object remain two occurrences even when they share a definition.
Storage deduplication must not collapse them.

Ordered parent links describe construction, including repeated components.
Membership links describe participation in organizational fields. Multiple paths
to the same membership field contribute once per live occurrence; different
occurrences still contribute independently. Reverse child listings are traversal
indexes, not instructions to materialize every descendant as a live particle.

A letter is a linguistic endpoint with ordered core character parents and an
underlying byte construction. Language/alphabet membership supplies separate
organizational fields. Letters are the first bounded study objects; their
fixtures demonstrate mechanics, not completed linguistic analysis.

## Representation and calculation

Primary conceptual LoD changes the unit of distance. Secondary compression and
naming equivalence inherit their primary scale at 1:1; raw SNode nesting is not a
scale measurement. Exposure specifies which representation participates.
Mass must not be counted again merely because its components are exposed.

Distinguish three quantities:

| Quantity | Meaning |
|---|---|
| Potential capacity | Prepared space available for declarations; unused entries have no mass or fields. |
| Present objects | Live occurrences retaining state, mass and participation, including sleeping objects. |
| Awake work | Present elements selected by native binary activity for their type's calculation phase. |

Selected focal fields establish a relative origin through their combined
centroid. Present objects interact through the exposed fields. Their slot indices
are not physical coordinates. The production requirement is continuous,
engine-owned insertion near the current focus without resetting survivors or
allocating backing for each arrival. See [governing requirements](../../AGENTS.md#required-placement-and-allocation-behavior)
for that contract and its remaining implementation gap.

## Component boundaries

| Component | Responsibility |
|---|---|
| Cold stores | Retained definitions and structural relationships. |
| Warm PostgreSQL toolbox | Shared reusable definitions/assembly descriptions with scoped publication. |
| Analyst-owned hot workbench | Assembly of the study, including separately supplied private/local input. |
| HCP engine and harness | Field mathematics, execution lifecycle and its control interface. |
| Modified Taichi | Native storage/activity facilities, kernel compilation and backend execution. |
| Database, WAL and endpoint kernels | Separate cooperating infrastructure; local/remote transport is a topology concern. |

Current numerical state belongs to execution. Report copies support inspection.
The cache manager has no access to private DB codes or contents; privacy is
gated before lookup. The optional browser viewport reads captures and changes
only its own camera/display.

Native C++ is the operational path. Project Python is limited to early bulk
database bootstrap preparation/population/validation. Prefer the fork's native
facilities over a second allocator or scheduler.

## Reading paths

Start with this page and [current state](../../AGENTS.md#current-state).
Contributors also read the [Covenant](../../covenant.md),
[Charter](../../charter.md) and [workflow](../../CONTRIBUTING.md).
Then take the relevant path; there is no need to read all research first.

| Role | Next reads or action |
|---|---|
| First local run | [First run](FIRST-RUN.md): matched CPU build and one named smoke test. |
| Coding/review agent | Entire [AGENTS](../../AGENTS.md), then component README, accepted plan and tests. |
| Engine / geometry / LoD | [Architecture](../../engine/ARCHITECTURE.md) → [active field model](../../engine/docs/ACTIVE-FIELD-MODEL.md) → [realignment plan](../model-realignment-plan.md). For capacity/insertion, follow the active issue and accepted stages linked in AGENTS. |
| Data / linguistics | [Language component plan](../language-component-unicode-plan.md) → [object model](../first-stage-data-object-model.md) → [working set and ledger](../../kernels/database/WORKING-SET-AND-LEDGER.md) → component README/tests. |
| Reviewer | Current state → owning accepted plan/code → PR scope, latest findings and [evidence reading](../../CONTRIBUTING.md#reading-evidence-and-status). |
| System / repository | [NAPIER guide](../napier-system-guide.md), observing built/planned labels; [reorganization record](../../REORGANIZATION.md) for provenance. |

Research explains derivations and possibilities. Accepted implementation contracts
and executable evidence establish the scope of engineering work. A held proposal
does not become authorized because an introductory page mentions its topic.

