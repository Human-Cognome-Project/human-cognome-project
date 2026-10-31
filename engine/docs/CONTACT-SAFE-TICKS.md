# Contact-safe candidate ticks — B1 integration

Status: implementation in progress under the accepted
[#199 plan](../../docs/native-admission-contact-transaction-plan.md), following
#200's accepted [native selection qualification](NATIVE-SELECTION.md). This
branch integrates the remaining B1 work. It is not qualified or accepted yet.
Placement, bootstrap/admission, enduring owners and CUDA remain separate.

A nonzero `Budget::TickWork` opts a prepared f64/x64 owner into
`Harness::tick_prepared`. GeometryWork is also required. Disabled owners retain
their existing numerical layouts and native signatures. Enabled owners refuse
the direct `tick64` entry, which would bypass contact validation.

The owner prepares private numerical/control surfaces and fixed bindings for
shared production field, Activity and installed C3 builders. It borrows presence,
source relationships and immutable hierarchy descriptors from the same Runtime.
Only the committed native type sets remain authoritative between attempts.
Runtime, capacity and lifetime preconditions of NativeSelectionWorkspace are
satisfied by the owning Harness; callers cannot supply arbitrary native sets.

The explicit contact ledger remains distinct from the physical union. Private
current and candidate geometry use the existing native collection, source-order,
discovery and CSR builders. Current coverage is checked before forces; candidate
coverage uses the actual published trial coordinates and includes sleepers.
Both boundaries share one GeometryWork allowance. The installed union and source
mapping are published with numerical/control state and native activity after all
checks complete. Changed coverage wakes affected present endpoints; discoveries
after motion select the following particle phase.

`TickResult` reports attempted work/demand and a named failing reservation.
Healthy capacity/domain rejection preserves committed values, membership,
source/union/CSR, parameters, generations, timings, prior report and observation
bindings. The last successful Demand remains available separately. A failed
backend operation or exception after publication begins makes the owner unready;
no host numerical replay or snapshot restore supplies rollback.

A changed installed union/mapping advances the prepared generation and installed
hierarchy source stamps at one completed boundary. Every accepted tick advances
the completed-tick observation. Unchanged coverage retains the generation.
Scratch and transport metadata are reserved independently of virtual address V;
setup charges measured native selection metadata in addition to the static
workspace/binding inventory. Reported work bounds are not FLOP measurements.

Qualification is in progress. Initial public-path smoke coverage checks zero dt,
unchanged TickWork rejection, retained query access after rejection, direct-entry
refusal and observed allocation-free dispatch. Moving contact overflow,
independent no-attempt retry comparisons, full-state preservation, dirty history,
source ordering/CSR, numerical parity, repeated use and emitted-code evidence
remain required before this integration is ready for acceptance.
