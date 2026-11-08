# Understand HCP

This path explains the model and component relationships. It does not require a build, the live PR queue or the detailed implementation stage. [Choose another path](README.md).

HCP models explicit objects and their relationships using field mechanics.
The engine calculates their relative motion; storage supplies reusable
definitions, and a harness controls execution. Analyst functions are conceptually
designed within the overall architecture; their implementation structure must be
engineered and calibrated using engine results on real data structures. See the
[analyst design/engineering distinction](../../engine/ARCHITECTURE.md#analyst-design-and-engineering-status). Read [current state](../../AGENTS.md#current-state) for what is built,
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


## Go deeper when useful

- Overall system: [NAPIER guide](../napier-system-guide.md), observing its built/planned labels.
- Engine architecture: [responsibilities and analyst design](../../engine/ARCHITECTURE.md).
- Objects and language: [data model](DATA-MODEL.md), then its owning contracts.
- Motivation and governance: [Manifesto](../../MANIFESTO.md), [Covenant](../../covenant.md) and [Charter](../../charter.md).
- Present capabilities: [current state](CURRENT-STATE.md), only when you want implementation status.
- How decisions developed: [historical path](HISTORY.md).

You can stop here if the model and component roles answer your question. Follow a deeper link for the particular relationship you want to investigate.
