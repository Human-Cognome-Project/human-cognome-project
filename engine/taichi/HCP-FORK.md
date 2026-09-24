# HCP Taichi fork recovery

This tree is the curated recovery of the Taichi fork used by the Human Cognome
Project native C++ engine.

## Provenance

- Upstream base: `taichi-dev/taichi@ba0e81dce559fb63a5958bf82feb1d00c55c02fe`
  (2025-07-30).
- The full local working folder was recovered through PR #65 on 2026-09-24.
- That upload contained 21,308 entries because it preserved a local Python
  environment, build trees, generated caches, compiled artifacts, and flattened
  submodule contents in addition to source.
- This curated tree reconstructs the source around the recorded upstream base
  and HCP changes. Exact upstream submodule gitlinks are restored from the base
  commit.

## HCP purpose

HCP retains Taichi's mathematical/particle execution core and backend compiler/
device machinery while bypassing the general-purpose Python frontend in the
runtime path. The HCP harness is native C++.

The fork exists to remove assumptions that were costly or limiting for HCP's
small-data field workload, including fixed metadata limits and heavyweight
allocation behaviour, while allowing newer/larger hardware to be used more
fully.

## Recorded modernization

The recovered modernization record documents, among other changes:

- configurable LLVM SNode and SNode-tree startup capacities replacing fixed
  metadata ceilings;
- paged/lazy ListManager metadata allocation;
- runtime/AOT ABI versioning for the changed layout;
- dense external-array flattened offsets widened to i64 in LLVM codegen;
- dense `Ndarray::nelement_` shape products widened to 64-bit;
- CPU/CUDA validation including a >2^31-element dense-array regression.

This is targeted modernization. It is not a claim that every Taichi backend,
sparse logical index, or frontend has been globally converted to 64-bit.

## Python status

The upstream Python frontend remains in this recovery source tree for now
because it is part of the original build/test distribution. HCP does not use it
as the engine interface.

Python may remain useful for build/bootstrap/tests. Removal of upstream Python
source should happen only after a clean native-only build proves it is
unnecessary. Machine-local `.venv`, caches, staged Python extension binaries,
and generated package runtime files are not retained.

## Build products

Local `build/`, `build-review/`, generated object/library files and staged
runtime bitcode are intentionally excluded. The source modernization record
states that the recovered build matched the modified source; the canonical
repository should reproduce that build rather than version a workstation build
directory.

The HCP wrapper under `../cmake/FindTaichiEngine.cmake` stages the native LLVM
runtime bitcode and CUDA libdevice into the HCP build directory after this fork
has been built.

See `modernization/IMPLEMENTATION-STATUS.md` and
`modernization/INSTALL-CONFIGURATION.md` for the recovered implementation and
validation record.
