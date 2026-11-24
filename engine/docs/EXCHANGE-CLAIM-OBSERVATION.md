# Native record-claim overlap observations

This is the measurement prerequisite for the whole-field LoD continuation in
[#428](https://github.com/Human-Cognome-Project/human-cognome-project/issues/428),
following [#427's provisional acceptance](https://github.com/Human-Cognome-Project/human-cognome-project/pull/427#issuecomment-7140102237).
It does not implement the whole-field operator. The private single-occurrence
exchange, public add/remove study and whole-field design gaps remain distinct.

Qualified source: `4267c1e1c092c4f07021f628fd64f58439b57a57`, tree
`8ef1ec109ccf25808786111b6501e290c1472299`, on #427 head `389cfdd3`.
Preserved local `083d0198` is tree-identical. Main remains `98cdbe03`; #427 is
provisionally accepted and unmerged, with authoritative native CI separate.

## What is measured

`UpdateResult::claims` records occupied particle/field counts from the existing
`PreparedRecordMap::size()` at three exclusive update barriers:

1. Before either family's incoming claims.
2. After both families have claimed, while all outgoing records still exist.
3. After both retirement batches finish.

All claims precede all retirement. The middle sample is therefore the actual
maximum simultaneous record occupancy of this successful transaction. It is not
reconstructed from final counts or a capacity formula. The getters read the
map's existing committed live-count metadata; they do not query allocator bytes,
launch kernels, synchronize, create backing or inspect model coordinates.

`sampled` becomes true only on successful native update completion. Preflight,
ordinary rejection, stale input and no-op requests have no observation. A zero
default value with `sampled=false` is not a measured zero peak. Unexpected failures
after claims continue to throw and mark execution unready; no successful receipt
is returned. Existing source/geometry/contact/hierarchy checks and their rejection
order remain unchanged.

These are **occupied records**, separate from potential addresses, prepared
payload capacity, references, active calculation, scratch, native allocator pool
bytes, compiler/ORC resources and RSS. No admission credit, capacity relaxation
or full memory census follows. Retiring old objects still cannot fund incoming
claims. The implementation adds six scalar getter reads and an inline return
value; it adds no retained owner member, array, kernel or reservation policy.

On the qualified host both compilers report `sizeof(UpdateClaimObservation)=56`
and `sizeof(UpdateResult)=192`. This result-layout change requires matching
native/client rebuilds. It is not a 56-byte retained store per potential object.
Existing recurring C++ new/new[] and sampled native backing/tree/compiler-count
guards remain in the controls.

## Real exchange and refusal evidence

The existing `exchange_fields` native fixture performs actual D7 expansion and
compression of one declared two-byte letter. It creates/retires the required
fields, keeps an inherited membership focus, commits one unticked generation,
resolves actual native claims and executes a following tick. This is native
execution through the existing private test adapter, not an operator LoD command.

| Direction | Particles before / peak / after | Fields before / peak / after |
| --- | --- | --- |
| Aggregate → two bytes | 1 / 3 / 2 | 3 / 9 / 7 |
| Two bytes → aggregate | 2 / 3 / 1 | 7 / 9 / 3 |

The fixture additionally configures seven field slots: the final seven-field
model fits, but its nine-claim peak does not. The unchanged native refusal names
Fields with required=9, leaves generation/history/maps/physical state healthy and
unchanged, and returns `sampled=false`. A focal-field retirement likewise refuses
without claims. Postclaim failure remains unready. Existing geometry/momentum,
contact, source-pin, no-new/backing and following-tick assertions stay active.

The following exact rows come from the clean Clang one-worker bulk run. They
record configured particle/field/participation/contact capacities and scratch,
actual occupied records and final active relationship/contact counts. Final
participation/contact counts are not peak byte measurements.

```text
exchange_native_claims,direction=parts,particle_capacity=70,field_capacity=70,participation_capacity=512,contact_capacity=70,scratch_bytes=273195972,particles_before=1,particles_peak=3,particles_after=2,fields_before=3,fields_peak=9,fields_after=7,active_participation=8,active_contacts=0,observation_bytes=56,result_bytes=192
exchange_native_claims,direction=aggregate,particle_capacity=70,field_capacity=70,participation_capacity=512,contact_capacity=70,scratch_bytes=273195972,particles_before=2,particles_peak=3,particles_after=1,fields_before=7,fields_peak=9,fields_after=3,active_participation=3,active_contacts=0,observation_bytes=56,result_bytes=192
```

## Operator observation

The existing `viewport_live_study` prints `native_claims` for each completed edit.
Sampled success contains `particles` and `fields`, each with `before`, `peak`,
`after`. Unsampled refusal/no-op prints only `{"sampled":false}`. It does not
manufacture zero-valued peak observations or alter the existing snapshot schema.
All values are observations; none are admission or placement inputs.

The [ten-record claim capture](../../tools/viewport/examples/live-study-claims-capture.jsonl)
uses the [existing correction commands](../../tools/viewport/examples/live-study-correction.commands):

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct \
  --commands tools/viewport/examples/live-study-correction.commands
```

It records removal, admission, intentional mismatched-template refusal, correct
restoration, no-op, removal and continued tick 2. Exit 1 is expected for the one
intentional refusal. The audit compares the complete completed observation across
refusal/no-op and verifies sampled occupancy against the live rows. This capture
is the ordinary-edit path; the real representation-exchange rows above are a
separate native witness.

## Qualification and next work

| Clean local check | Result |
| --- | --- |
| Existing quick CI | 3 CTests plus placement math/capacity controls |
| Clang 15, warm/viewport ON | 4 public-session/CLI CTests; 3 exchange/preflight controls |
| GCC 11, warm/viewport ON | 2 public-session CTests; 3 exchange/preflight controls |
| Clang 15, warm/viewport OFF | 1 core CTest; 2 exchange/preflight controls |

All pass on one clean source with unchanged HEAD/worktree. Full configurations
cover one-worker bulk and two-worker direct. Minimal real exchange/preflight
proves the observation does not require viewport or database integration. The
successful no-claim preflight control remains unsampled; public controls check
ordinary admission's observation and rejected template's absent observation.
These ten CTests, eight manual native controls and two quick placement controls
are scoped CPU evidence. Hosted full matrix and GPU results are not inferred.

Evidence: `/tmp/hcp-claim-4267c1e1-evidence.tar.gz`, SHA-256 `5bb82f906fa5bec42252879d5558d4d03ea251158a461c446e8d01c37330c3eb`.
Fresh extraction verifies the root and four nested manifests. The bundle retains
exact source/fork/toolchain/runtime/binary pins, commands/configuration, raw test
and CLI output, implementation patch/source bundle, build log and source checks.
It is machine-local; the exact native rows and bounded CLI capture are committed
for remote review. No native cache or persistent data was cleared.

The next implementation must stage all logical occurrences with separate native
D7 centres/velocities and one coherent map/exposure publication. Combining them
into the existing single outgoing set would mix motion; looping commits would
not preserve a whole-field refusal. The initial preset also needs prepared
primary alternatives and explicit compatible field/comparison/focus choices.
#428 asks whether to select a stable membership-focus preset first or separately
extend focus handover. That question does not block this measurement increment.
Keep whole-field LoD, subsequent confounding/structural trace and allocation-policy
decisions separate from what this observation proves. All merges are maintainer-only.
