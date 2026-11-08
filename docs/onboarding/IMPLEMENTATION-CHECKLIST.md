# Cross-component implementation checklist

Use when a change spans numerical, data and operation boundaries, or as a review cross-check. For a bounded task, start with its specific contracts instead. This preserves the earlier onboarding specification without making every contributor load it.

## Non-negotiable implementation specification

- **One physical model:** native C++ over the curated Taichi fork. Reuse shared
  native builders and binary activity facilities. Do not add host physics replay,
  a custom scheduler, potential-capacity scans or a second persistent solver.
  Project Python is limited to early bulk-data bootstrap preparation/population/
  validation; upstream Taichi tooling is a separate boundary.
- **One auditable publication gate per destination:** accumulate applicable
  contributions at calculation precision, filter the net once at the expressed
  positional publication boundary, and compare with the stored position. Do not
  discard each small contribution independently, recover discarded child tails,
  or carry an unpublished positional remainder across ticks. Preserve pending
  input, source/mass invalidation and required published history.
- **Accepted wide motion:** positive dt retains accepted displacement/dt;
  zero dt retains incoming velocity. Quiet positive-dt publication retains zero
  velocity. This supersedes older untrimmed-velocity intent for that profile;
  legacy arithmetic remains separately supported.
- **Minimal universal numerical controls:** precision is calibrated, not a fixed
  f60 rule. Density-based f32/f64 selection is an optional future experiment,
  not an instruction to add regional switches now. No arbitrary physical epsilon
  or new damping rule to make small fixtures stop.
- **Distinct identity and mass:** a reusable definition is not a live occurrence.
  Copies have independent state and mass; deduplicate membership paths per
  occurrence and field. Primary conceptual LoD changes scale; secondary naming/
  compression stays 1:1. Storage depth and slot indices are not physical scale
  or coordinates. Potential, present and awake are distinct; sleepers remain
  occupants and contributors.
- **Admission:** use actual extents and sleeping occupants, canonical batch
  order, current combined focus and certified nearest-free placement. Tangency
  is allowed; missing geometry is rejected. New bodies start awake with zero
  motion. Validate quantized stored coordinates and the derived distance bound;
  exhaustion rejects without claims. D7 aggregate/parts exchange targets the
  outgoing logical centre, not study focus. Read D1–D7 rather than inferring
  bootstrap, restart or rebase behavior from this summary.
- **Transactions:** prepare old-plus-new peak capacity. Preserve survivor state,
  identity, dirty history, pending work and generation semantics. Ordinary
  rejection leaves the committed execution and last successful report unchanged;
  unexpected failure after live writes makes the execution unready. Do not
  uninstall/reseed the hierarchy or reset an owner to simulate an edit.
- **Data boundaries:** cold definitions → shared warm PostgreSQL toolbox →
  owner-local hot workbench → native execution. The cache manager rejects private
  namespaces before lookup; private inputs enter only the active owner boundary.
  Never reset/drop persistent stores or repopulate retained seeds as setup.
- **Observation:** snapshots read the actual active calculation stream, including
  GPU execution. A separate CPU replay is not a GPU snapshot. Keep captures,
  source/generation and completed tick identity coherent.


[Choose another path](README.md)
