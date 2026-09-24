# Local field recovery snapshot — September 2026

This directory preserves the exact `field/` tree recovered from the local
`dbkernel-design-checkpoint` working state through PR #65.

It is a provenance snapshot, not current runtime code.

The tree contains:
- planner-era Python field-engine files;
- generated mint/report JSON outputs from control/probe runs;
- Patrick-authored field research material that also exists in the current
  research tree.

The canonical HCP engine is native C++ under `/engine/`. The later reconciled
planner Python line is separately preserved under
`/archive/2026-09-planner-field-python/`.

This snapshot is retained intact because the generated outputs and exact local
file combination may be useful when reconstructing how the native engine work
diverged from the planner prototype.
