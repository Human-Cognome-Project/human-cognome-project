# September 2026 planner Python field prototype

This directory preserves the Python field-engine line that was temporarily
promoted into `engine/field/` during repository reconciliation.

It is **not the canonical HCP engine implementation**.

The prototype was created while the native C++ engine workspace and the
modified Taichi fork existed only in a local working tree and had not been
properly pushed. In that absence, the Python line looked like the newest
complete field implementation and was mistakenly treated as canonical.

The recovered native workspace explicitly records the governing precept:

> Python is front-end feed only, never engine/data.

HCP uses a native C++ harness directly against the modified Taichi
runtime/compiler machinery. Taichi's own Python frontend exists for the
general-purpose abstraction surface HCP deliberately does not need.

The files here are retained as historical/experimental evidence only. They may
contain useful comparison fixtures or ideas, but new engine work must not build
on them or reintroduce Python into the runtime path.

The native recovery is under `/engine/src/`, with the modified Taichi fork
expected under `/engine/taichi/`.
