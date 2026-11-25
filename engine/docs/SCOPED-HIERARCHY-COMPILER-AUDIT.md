# Scoped hierarchy compiler audit

The maintainer provisionally accepted #438 at `39523794` for the fixed
`aá` LLVM CPU study and requested an audit of the scoped reduction and
inherited response. The same fixture now checks the Program-retained force,
poll and candidate-seed LLVM after bootstrapping the byte exposure.
[Maintainer direction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/438#issuecomment-7167787666) · [Endpoint run](REPEATED-ENDPOINT-ROLLUP.md)

The [live Unicode follow-up](LIVE-UNICODE-EXCHANGE.md#separate-scoped-compiler-coverage) qualifies the scoped seed and poll reduction for `ó` in both compiler modes.

## Checked operands

- The poll follows the sign of the current child link. A whole link acquires
  all four words of its own packet. A scoped link decodes `-1 - link`, reads
  the corresponding body and exact numerator and denominator, and supplies
  that body's XYZ and its ratio-weighted mass to the same published reduction.
  LLVM sinks the XYZ loads through pointer PHIs. The audit checks the
  predecessor addresses and verifies the resulting diagnostic LLVM module.
- In the seed kernel the whole path selects the matching candidate slot. The
  scoped path decodes and validates the body and uses the same exact row for
  the weighted mass feeding the accumulator. The fixture checks the seed
  packet values for both exposures.
- In force and both sides of the wake comparison, the scoped response uses
  the owning branch XYZ minus the selected recipient XYZ, branch mass,
  and the recipient mass times the ratio from the same influence row.
  Eligibility reads the owning branch's multiple-body flag and child count.
  Wake uses the old or pending publication words only for the changing node.
- Both compiler modes reject a shifted scoped poll source row and an omitted
  scoped X displacement. The controls are LLVM-verified clones of the actual
  modules and run only through the audit.

The ordinary packet audit continues to check sequential consistency,
complete selector-bound packet reads, and inactive-slot publication. The
scoped checks exercise the fixed repeated study and the emitted LLVM shapes.
Data bindings and numerical results are covered by the fixture's 291 assertions
per compiler mode. Live Unicode exchange and GPU qualification remain open.

## Local and hosted evidence

The tested local commit `6c82686510320849b2ae6cc6aa8f5c450d199a9f` and
GitHub source commit `7ece8447ab51d9b649746431a8315c0cf8a8222d` have identical
tree `70fd1ce88b6ccf9babc0e28a74d018f78af06ce1`. The default and no-unroll
runs used Clang/LLVM 15.0.7 on x64 CPU. The fork tree is
`a94a9eb350c94dfa2eb586bee49c53e0ee886648` and the runtime bitcode SHA256 is
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

Twelve selected CTests pass. They include the two-worker direct repeated
study in both compiler modes, the earlier semantic and admission controls,
viewport snapshot serialization and the CPU smoke and strict-math tests.
A one-worker bulk run also passed all 291 assertions and both controls.
The ordinary two-worker direct packet and refresh-transport audits pass
with both compiler settings. The scoped no-unroll profile is registered in
CTest for hosted native regression.

```sh
ctest --test-dir engine/build --output-on-failure -R '^semantic_repeated_rollup(_no_unroll)?$'
. engine/build/engine-env.sh
engine/build/live_hierarchy_test --threads 2 --direct --case refresh_transport
engine/build/live_hierarchy_test --threads 2 --direct --case refresh_transport --compiler-no-unroll
```

The machine-local archive `/tmp/hcp-scoped-compiler-6c826865-evidence.tar.gz`
has SHA256 `91ba52596fd3added0200d10040a3c91b062d075f46b99f98f9280f2af6af027`.
It contains the commands, source and runtime pins, JUnit logs, retained LLVM
from both modes, ordinary audit output and the two controls. Its manifest
passes after fresh extraction.

The parent #438 hosted database job received HTTP 502 from GitHub LFS while
downloading retained fixtures. The job stopped before database acceptance
ran. Retry that job when LFS can serve the fixtures. The audit follow-up
requires its own hosted CI and parent-first integration.
[Parent hosted database log](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45649473132/job/139334147579)

## Hosted default audit correction

GitHub's default `semantic_repeated_rollup` at #439 head `a8e0d8d0`
failed before the force audit reported a completed path. Its tested merge
was `4c74fcff561691d6291aa6df06baa8b7c9ca7784`. The no-unroll regression
and four ordinary compiler jobs passed.
[Default failure](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45657234407/job/139361937031) ·
[No-unroll result](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45657234407/job/139361937055) ·
[Maintainer finding](https://github.com/Human-Cognome-Project/human-cognome-project/pull/439#issuecomment-7169839218)

The same run's retained compiler artifact reports the `znver3` LLVM CPU
target. The local Alder Lake fixture passed with scalar XYZ operations.
A diagnostic executable requested `znver3` target analysis at the optimizer
call while executing on the local x64 host. The old checker failed on both
of those compiled force modules. This reproduces the hosted assertion at the force response matcher.

In the retained `znver3` force LLVM, the scalar kind-2 comparison and
eligibility gate are valid. Y is computed by a scalar subtraction. LLVM's
SLP pass puts the owning branch's X and Z words and the recipient's X and Z
positions into two lanes, subtracts the two vectors and selects the result
with the splatted kind-2 predicate. The old scan starts at a scalar X
subtraction and misses those two lanes. The corresponding Alder Lake
no-unroll module exposes three scalar selections. Loop unrolling and SLP
can vary independently. The local `znver3` no-unroll force also uses the
two-lane form.

The checker now scalarizes vector operations on its diagnostic LLVM copy,
then traces the same per-axis parent word, recipient position, eligibility,
branch mass and exact weight. The cloned LLVM passes verification. The
wrong-source and omitted-displacement controls both reject, including on
the normalized vector form. The Program's retained force, poll and seed
modules are written before the assertions when capture is enabled. Hosted
CI archives both compiler settings on the same two-worker direct runner.

The capture-only head `55e05253` was queued when the local evidence was
packaged. Its run will preserve the raw hosted default and no-unroll IR.
The comparison above uses the retained local modules and the hosted log
records for the original results.
[Capture run](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45670640893)

The lane-aware source is local `beeb8bdb29fc292431e1d4c52490cb30b921c182`
and GitHub `e5b64b78ab2a2c3090780933982925afdb4b6a21` with tree
`5b2781cf85c9a2d4ed3aa8c92b41ecb71f12aee5`. Four scoped native runs pass
291 assertions each and reject both controls. They use default and no-unroll
on the actual local CPU and the diagnostic `znver3` optimizer config.
The ordinary packet and refresh-transport profiles pass under both compiler
settings. The shared audit entry point also passes both registered repeated
rollup CTests at local `ef1a6360` and GitHub `fb131b35` with the identical
tree `265fa1fdef55b23cf162a555bc263695d062d10c`. Clang/LLVM is 15.0.7
and the fork and bitcode pins are listed in the previous section.

The machine-local archive `/tmp/hcp-439-lane-audit-evidence.tar.gz` has SHA256
`21d4a841cffe087f188a7898490e39c025b3668fa8b75d81b4cec3fd07c8517c`. It includes exact source and binary pins, original and
corrected LLVM, both sets of assertions, the explicit optimizer-target
override and the hosted evidence available at submission.
