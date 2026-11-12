# Shared native support

`reservation_allocator.h` contains the explicit allocation-domain primitives used
by owner preparation and allocator-aware supplied values. It has no engine,
database, network or runtime dependency. Domain selection and lifetime policy
remain with the calling owner; standalone values use detached allocators.

The engine's former include remains a forwarding header with namespace aliases.
This does not introduce another execution authority or a process-wide allocator.
Counters require exclusive synchronous access; actual requested bytes, cumulative
allocation work and retained-capacity censuses remain distinct.

## Domain bootstrap tracker (awaiting review)

`bootstrap_reservations.h` supplies the bounded lifetime prerequisite requested
in [#297 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/297#pullrequestreview-5878815959).
It is not yet wired into `ExchangeOwner::adopt`: that path still uses its existing
untracked `make_shared<ReservationDomain>`. Owner integration, trusted disjoint
borrowed accounting and combined admission remain follow-on work.

`BootstrapReservations::create` admits the outer tracker's own `sizeof(State)`
allocator request before allocation. Its intrusive handles occupy caller/allocator
objects, with no separately allocated reference-count block. Copies/moves preserve
the same identity; releasing the final handle frees the tracker. The tracker owns
no domain or selected object. All handle and counter access is exclusive and
synchronous, including destruction; this is not an atomic cross-thread handle.

`reserve_domain(tracker, selected_budget)` uses `allocate_shared` with a rebound
`BootstrapAllocator`. This observes the actual domain/control-block allocation
request rather than guessing a library-specific control-block size. The rebound
allocator retains the tracker until the last weak reference releases the block.
Selected allocators can retain the domain beyond the caller's handles without a
cycle. The `ReservationResult` returned by `create` is a snapshot, not a retained
reference to caller storage; `tracker.result()` obtains later snapshots.

Bootstrap bytes H consist of the tracker request plus every live attached
domain/control-block request. Domain-selected requests D remain separate, governed
by their own existing budget. H and D are disjoint request surfaces; neither is
physical residency, allocator-internal metadata, stack storage or borrowed native
backing. This helper does not sum them or implement a combined limit. Keeping an
external tracker handle deliberately keeps its own request live after domains die.

Both tracker creation and rebound allocation prepay cumulative request work.
Byte/work refusal precedes the underlying allocation; arithmetic overflow leaves
accepted live/work counters unchanged. Allocator/constructor failure releases
acquired storage without refunding prepaid work. `create` reports the local
snapshot even if the tracker allocation throws; later failures are observable
through the retained handle. Empty/moved-from handles cannot allocate. Failed
request status remains sticky under the existing domain contract; exact retry
controls use a fresh tracker.

The standalone `bootstrap_reservations_test` is registered in
`engine/tests/letter_projection`, which the hosted CPU workflow already builds
and runs independently of Taichi/PostgreSQL. It compares all observed C++ request
bytes against H/D, tests both boundaries' zero/one-short/exact budgets, weak-domain
and weak-selected lifetimes, each bootstrap allocation failure, constructor unwind,
alignment and arithmetic overflow. It does not qualify owner/native integration.
