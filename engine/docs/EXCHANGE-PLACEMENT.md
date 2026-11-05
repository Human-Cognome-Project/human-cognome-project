# Private native replacement staging

This D7 prerequisite connects the [outgoing-state reduction](EXCHANGE-STATE.md)
to certified placement and generated-row initialization. It is private scratch
staging, not a representation-exchange API or publication coordinator. The
[accepted D7 contract](../../docs/core-operation-readiness-plan.md#decision-status)
still requires logical occurrence/recipe validation, exact source contribution
mapping, supported momentum accounting, prospective hierarchy/contact validation
and one coherent replacement publication.

## Attempt and native data flow

`detail::Placement::stage_exchange` takes current-generation structural commands,
complete replacement intrinsics and a nonempty set of full outgoing body handles.
It immediately reduces resident positions, masses and velocities, including
sleepers. The outgoing set must match exactly the commands' retiring body set;
duplicate, missing or unrelated exclusions reject. Source projection validates
the complete retirement handles and incident references before occupancy capture.

A prepared strict Taichi kernel copies the derived centre and mass directly into
placement's focal scratch. Study focus is not the exchange target. The existing
certified nearest-free solver sees all prospective retained occupants, including
sleepers, and earlier canonical replacements. Outgoing bodies are excluded from
prospective occupancy, but remain live and claimed throughout. The solver keeps
its coordinate/radius domain, precision filtering, work/cell limits and distance
certificate. A valid reduction outside its supported coordinate domain rejects.
Current present-body geometry is still validated conservatively, including
outgoing geometry.

Native initialization remaps canonical rank to command index, copies certified
positions and intrinsic mass/radius, and transfers all three full-f64 derived
velocity components directly from outgoing scratch. It clears force/determiner
and unused command entries. No position or velocity is read back and re-uploaded
as authority. Diagnostic target/status readbacks do not initialize model state.
Ordinary and bootstrap staging explicitly disable motion transfer, retaining
their existing targets and zero initial motion even after an exchange attempt.

## Ownership, reservations and limits

This call creates no query token, claims no record and changes no live source,
numerical/history/activity/contact state, frame, generation or tick count. Prior
public observations remain valid. Failed scratch is not consumable; successful
scratch is only for an immediate future coordinator under exclusive ownership,
not a reusable candidate. Unexpected execution failures retain the established
unready-owner rule; ordinary rejection allows retry.

The old-plus-new particle/reference peaks are unchanged: retirement cannot fund
replacement claims, even if the final representation fits. Prepared accounting
adds one two-argument target-copy kernel and two initializer arguments, with no
new numerical array. Preparation compiles, binds and warms both paths. The
placement prefix additionally reserves `64*(P+1) + commands + 64*bodies + 128`
work for reduction, retirement matching and native transfer. Selection and all
placement/initialization backing are reused.

The following [native accounting increment](EXCHANGE-ACCOUNTING.md) adds prepared
report/source-mass arrays and a balance kernel. It reconciles scalar physical and
exact source masses within its supported integer domain, checks momentum within
explicit floating bounds, and reports actual centre and offset separately from
the target. This does not validate logical occurrence/alternative identity or
incoming source contribution mapping. Nearest-free placement does not promise an
unchanged combined centre. The coordinator must still publish retirement only
with the accepted replacement source/hierarchy/contact state. No public control
or database/recipe start path is added here.

## Executable scope

`exchange_placement` is included in the combined placement suite. At 52/8 body
fraction bits it exercises unequal-mass compression, aggregate expansion,
canonical command remapping, self-exclusion and blocking sleeping occupants,
with independent small-system certificate checks. It checks full-f64 transferred
velocity, zero force, invalid/stale/duplicate/mismatched selections, invalid
numerics, work rejection, ordinary scratch reuse, unchanged live state/history
and public observations, no observed recurring C++ allocation, and stable backing.
A final-fit/peak-overflow case retains the old-plus-new particle claim requirement.

The emitted-code audit includes the target-copy kernel, native outgoing bindings
for target/velocity consumption, strict arithmetic and owned scratch writers.
Its output-omission controls now cover all 19 kernels; the existing arithmetic and
publication controls remain. [Local evidence](../tests/admission/README.md)
records source pins and actual modes. No GPU execution or complete D7 exchange
is qualified by these CPU checks.
