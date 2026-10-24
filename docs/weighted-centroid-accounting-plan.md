# Source counts and weighted centroid comparisons

Status: **accepted in #183, 2026-10-24; implementation pending**. Baseline: `main` at
`768493a28d1fcb5608179145c86346ba7489d9ed`, following accepted #182 and #181.
Owners: [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177)
and [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172);
operational integration remains under #156/#170.

## Accepted next component

Preserve exact source contribution counts at the prepared boundary, then extend
the accepted [upward candidate operation](../engine/docs/CENTROID-CASCADE.md) to
source-backed fractional contributions and compatible overlapping cohorts.
This supplies accounting needed by the later live ripple path. It does not
replace that path with an obligatory full query on every tick.

Acceptance releases the two implementation segments below in sequence.
Both remain candidate-only: no live centre replacement, forces, body motion,
wakes, new scheduler, database write or analyst matching policy. The first
segment preserves source metadata without changing the existing f32 physics.
The second uses it in explicitly requested f64 candidate comparisons. This is
not approval of the later precision/history/force conversion or proof of settling.

Whole-body/disjoint cohorts were #182's first-component limits, not restrictions
on the model. This extension supports overlaps whose source contributions
already form a consistent accounting partition. Arbitrary overlap allocation
and automatic comparison-context construction remain unbuilt.

## Observed source boundary

The existing [assembly preparation](../engine/src/assembly/study.cpp) retains
`PreparedParticle::exact_mass` and `Participation::exact_count` as uint64.
Ordered parent counts sum to the definition's exact structural mass. Separate
parent ordinals remain distinct; membership and sibling fields use whole mass.
The numerical share is currently narrowed from `parent.count / definition.mass`
to f32. The prepared [relationship header](../engine/src/field/prepared_storage.h)
retains handles, ordinal and that share, but no exact numerator/denominator.

For mass three and three count-one parents, the source accounts for exactly
three units. Three stored f32 approximations of one third need not sum to one.
An overlap validator must not mistake that rounding for physical excess, infer
missing counts by rounding a share, or silently normalize the stored fractions.

Exact structural mass is a **denominator**, not another live physical mass.
Current resident numerical mass remains the physical weight. This component
cannot recover precision already lost in its f32 storage.

## Segment A: retain exact source accounting

Carry optional exact structural mass per present occurrence and exact count per
ordered participation row through the prepared definition/update boundary.
Use integer metadata, alongside existing occurrence/field handles and ordinals.
Do not add source counts to coordinate/velocity payloads or obtain them from a
database lookup inside Harness.

The trusted source adapter supplies these values from the existing prepared
recipe/study data. Caller-owned query tables cannot assert new source counts.
The bounded test adapter can prepare a real `PreparedStudy`, bind its independent
occurrences to native handles and commit those exact values. This is not general
production admission or a replacement FixedStudy lifecycle.

Accepted rules:

- Exact accounting is either supplied or explicitly unavailable. Zero is not a
  valid supplied denominator/count. Existing commands without this metadata
  retain their legacy meaning; their rounded shares are not reverse-engineered.
- A supplied count is positive and no greater than its occurrence denominator.
  Validate its associated legacy f32 share against the source preparation's
  declared conversion. Preserve the count even when the ratio is not exactly
  representable. Do not change old physics to consume the new ratio in Segment A.
- Count and source-row edits commit with existing topology generation, ledger
  order and handle validation. Replacing a participation also replaces or
  explicitly clears its count provenance; never retain an old count by accident.
  A count-only change advances generation and invalidates bound candidates.
- Survivor denominators persist. Retirement clears their metadata; recycled
  records acquire only the new occurrence's supplied data. Do not mutate a
  survivor's structural denominator through a participation edit. Replacement
  representations use their own definition/lifecycle boundary.
- Supplied metadata is checked with prospective source topology before native
  publication. Rejection leaves physical arrays, source metadata, activity,
  history, generation and Demand unchanged. Unexpected backend/invariant failure
  retains the existing unready policy.

Store committed and prospective metadata in prepared storage. Inventory actual
`sizeof` changes to headers/commands, denominator tables, validation, inspection
and any native transfer buffers; do not assume only two extra integers in the
total reservation. Do not create a per-potential-address payload. Ordinary
updates must retain prepared backing and compile/bind counts.

## Segment B: select source rows and validate weighted coverage

Add an explicit weighted bind input while preserving #182's whole-body API and
its existing rejection rules. Each requested contribution identifies its full
occurrence handle and an ordered source row within the node's exact field and
committed generation. Check row identity, ordinal, endpoints and accounting
provenance. Row numbers alone cannot survive edits or recycling.

Coalesce repeated paths to the **same source row** within a comparison.
Different rows/parent ordinals remain separate even with equal endpoint handles.
Several live occurrences of one definition remain independent. Keep canonical
source-row order; children remain in canonical dependency order.

For occurrence i, let D_i be its source denominator, and let C(n,i) be the checked
integer sum of its selected counts in node n. For this bounded component:

```text
0 < C(n,i) <= D_i                     for each selected occurrence
C(parent,i) = sum C(child,i)          across the parent's child cohorts
```

An absent occurrence has count zero. Every parent must have exactly the coverage
its children represent. Use checked integer sums, with rejection before overflow;
no arbitrary floating tolerance, inferred missing count or automatic rescaling.
This is once-per-comparison accounting, not a limit on how many independent
membership fields can include an occurrence. Counts across unrelated comparison
contexts are not summed into a global cap.

Example: a, b and c have denominators 1, 2 and 1. Child A selects counts a=1,
b=1; child B selects b=1, c=1. Their parent selects a=1, b=2, c=1. The children
overlap on b but account for its full contribution exactly once in the parent.
Both children may retain their separately published numerical centres.

If instead each child represents b=2, their combined count four cannot satisfy
the parent's b=2. Reject this request. Scaling two already-published whole child
centres cannot generally repair arbitrary overlap: it would also rescale their
other members. A later matching plan may supply appropriately partitioned
comparison contexts; this component neither invents those contexts nor rewrites
existing source memberships to force a partition. It must not flatten the parent
to raw bodies or recover hidden child first moments as an overlap correction.

Distinct ordinals may sum to a full or partial contribution. For example, two
count-one ordinals from the same mass-three occurrence remain two rows with
coverage two, not one deduplicated endpoint. A request whose selected rows lack
exact provenance rejects as unsupported accounting; it does not fall back to
guessing from f32 shares. Existing whole-body queries continue independently.

## Numerical publication and native resources

At a finest selected node, compute each source-row contribution as:

```text
s = f64(exact_count) / f64(exact_structural_mass)
w = f64(current resident physical mass) * s
M = sum(w)
c = a + sum(w * (f64(current resident position) - a)) / M
```

Promote before products, subtraction and sums. Source integers preserve exact
coverage; their f64 conversion/division and the weighted reduction still have
ordinary numerical error, particularly above 2^53. Qualify native u64-to-f64
conversion and bound these errors independently. Do not claim exact arithmetic
or substitute structural count for current physical mass.

Parents consume published child centres and their untrimmed **participating**
masses. The matching coverage check permits that reduction without duplicate
physical contribution. It supplies no new parent distance multiplier and no
second application of the leaf share. Internal grouping has identity precision;
experimental Q_b is applied only at expressed comparison publication boundaries.
Discarded positional detail never returns through count metadata or a remainder.

The current opaque f32 source share remains available to legacy physics. The
weighted candidate uses the source count ratio explicitly; small differences
from the old f32-share reduction are expected and must be reported, not hidden.
It does not widen body storage, change whole-particle force absorption, change
universal physical mass or select a production rounding/LoD precision policy.

Bind validates coverage in reserved host tables and prepares count/denominator
descriptors. Query reads current native bodies, including sleepers. Validate
the pinned source endpoints/share representation and generation so modified
backend source data cannot silently contradict the prepared accounting.
Use existing Taichi u64/f64 loads, casts and ordered reduction facilities.

Charge source rows, unique occurrence/coverage entries, child links, contexts,
scratch and preparation/query work explicitly. Distinct ordinals count toward
reference capacity; deduplicated paths do not acquire physical mass. Coverage
entries are bounded by selected references. Include committed/prospective source
metadata, integer accumulation, host/native descriptors, candidate/inspection
buffers and binding/transfer bytes in the manifest. Use actual widths and copies,
and retain independent exhaustion tests. Do not add a native activity set,
per-level layout, host numerical authority, hot allocation or implicit expansion.

## CPU acceptance and release sequence

Segment A first, as a focused metadata/update PR. Verify exact counts from real
assembly preparation, thirds and repeated ordinals, independent copies, unknown
legacy metadata, count-only edits, retirement/recycling and atomic rejection.
The existing tick/force/centroid results must remain unchanged for equal legacy
inputs. Run the appropriate native regression matrix before beginning Segment B;
publish each implementation segment as its own reviewable PR.

Segment B then adds the weighted candidate operation and its evidence:

- Full and fractional direct comparisons, repeated ordinals and aliases, with
  separately checked logical integer coverage and numerical mass/centres.
- Two/three-level compatible overlaps, including the a/b/c example, partial
  parent coverage, skipped levels and same-LoD internal grouping. Compare against
  a hierarchical reference with identical expressed publication boundaries.
- Reject oversubscription, missing/unknown counts, incomplete parent coverage,
  stale/foreign/recycled rows, incompatible source share/count provenance,
  overflow, unsupported profiles and each independent resource limit. Preserve
  all physical/source/control/activity/history/Demand state on ordinary rejection.
- Use counts above 2^53, unequal current physical masses, cancellation and all
  legal experimental retained-bit counts. Demonstrate no f32-share narrowing in
  the new weighted calculation, no discarded-tail recovery, and no extra scale
  factor. Thirds must pass exact accounting without an epsilon.
- Both CPU source host paths, empty/first use, holes/final records, maximal
  demands, poisoned BulkCopy staging, sleepers and changed current values after
  real ticks. Retain setup/bind/query/inspection/reuse costs, source/backend/
  compiler pins, manifests, raw results and allocation/backing/compiler counters.

## Subsequent live ripple contract

This component establishes contribution inputs, not the live propagation order.
The following implementation still needs its own bounded release under #172/#177:

1. One authoritative numerical profile across body/field/focus/update/geometry
   and inspection interfaces, with useful f64 storage where selected. Separate
   accepted physical displacement and previous published history from discarded
   positional rounding residue; never clear the mixed old `group_delta` wholesale.
2. Initialize all exposed centroids, then trial ordinary native phases using
   current published contributions across ticks, as accepted in #181. One
   activity bit per particle type remains the scheduler. Specify safe read/
   publication cuts, native consume/reactivation behavior and reliable delivery
   of consequential pending changes before changing live mechanics.
3. Combine local and inherited whole-particle response once, integrate a body
   once, and retain contact/focal invalidation and #170's coherent candidate
   motion/contact publication. Exact downward response remains provisional.
4. Test several depths and perturbation/insertion recovery. Report the tested
   settling criterion, ticks, elapsed time, active work, oscillation and lost-wake
   checks. A quiet activity mask alone is not evidence of correct settling.

The full candidate evaluator can independently check a settled upward result.
It is not a required per-tick hierarchy solve, barrier loop, recursive substep
or second scheduler. General overlap matching, dynamic exposure, production
rounding, frame/rebase calibration and full operational/GPU qualification remain
open. This plan closes none of #172/#177/#156/#145.
