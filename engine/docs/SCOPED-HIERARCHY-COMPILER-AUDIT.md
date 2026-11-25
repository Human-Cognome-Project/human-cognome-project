# Scoped hierarchy compiler audit

The maintainer provisionally accepted #438 at `39523794` for the fixed
`aá` LLVM CPU study and requested an audit of the scoped reduction and
inherited response. The same fixture now checks the Program-retained force,
poll and candidate-seed LLVM after bootstrapping the byte exposure.
[Maintainer direction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/438#issuecomment-7167787666) · [Endpoint run](REPEATED-ENDPOINT-ROLLUP.md)

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
