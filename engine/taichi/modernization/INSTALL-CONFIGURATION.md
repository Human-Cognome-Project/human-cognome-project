# LLVM SNode installation configuration

These settings are consumed when an instance starts. They apply to LLVM CPU,
CUDA and the shared LLVM runtime used by AMDGPU; AMDGPU is not qualified by
this implementation. They do not enable sparse SPIR-V backends or widen logical
indices. No live resizing or automatic hardware policy is introduced.

An installer/configuration loader can probe the system, obtain the user's
resource budget, and write environment settings for each application instance:

```sh
export TI_LLVM_SNODE_CAPACITY=16384
export TI_LLVM_SNODE_TREE_CAPACITY=2048
export TI_DEVICE_MEMORY_GB=0.5
```

These are examples, not automatically selected values for the GTX 1070. The first
two settings allocate runtime metadata tables at startup; the last is the
existing CUDA/AMDGPU sparse allocation pool budget. A shared-card installation
must budget all concurrent instances together with driver and other allocation
costs. CUDA device selection is independent of these capacities.

Python can supply the same settings explicitly:

```python
ti.init(arch=ti.cuda, llvm_snode_capacity=16384,
        llvm_snode_tree_capacity=2048, device_memory_GB=0.5)
```

Explicit ti.init arguments override environment values under the existing
Taichi configuration mechanism. Native C++ callers set the corresponding
CompileConfig members before runtime construction, or explicitly call
`CompileConfig::apply_llvm_runtime_environment()`. The native C API calls this
method when constructing an LLVM runtime and reads the two capacity variables.
`TI_DEVICE_MEMORY_GB` is handled by Python initialization; this change does not
add native C API parsing for that memory-budget variable. The runtime snapshots the
two capacities; changing configuration after startup does not resize its tables.
Both capacities must be positive representable integers. Defaults remain 1024
and 512 for compatibility; these are defaults, no longer fixed array sizes.

## What to budget

On 64-bit targets the SNode tables require 24 bytes per configured slot and the
tree tables require 16 bytes per configured slot, with page alignment. These
slots do not allocate particle fields or list managers. Root and scalar-place
SNodes consume IDs too. The program's implicit root also needs headroom.

Sparse containers need iteration lists; scalar-place leaves do not. Each list
manager now embeds a 2 KiB directory of page pointers rather than a 1 MiB flat
chunk-pointer array. A page of 512 chunk pointers is allocated on first use and
reused thereafter. Existing data-chunk sizing is unchanged, so small amounts of
active data can still reserve sizeable data chunks. Configured slot capacity is
not a guarantee that an arbitrary populated schema fits a selected memory pool.

SNode IDs remain monotonic for the lifetime of a Program. The SNode capacity
therefore bounds IDs created during that lifetime, including destroyed layouts;
it is not simply a maximum live-object count. Tree slots are recycled after
destruction. This change does not promise reclamation of all sparse metadata
on tree destruction. Reuse of existing layouts and activation/deactivation is
preferable for recurring work. Full reclamation/reusable SNode identities would
require a separate lifetime design accounting for compiled kernels.

The runtime reports a clear error if a tree or SNode ID exceeds the configured
capacity, including loading fields from AOT artifacts. Independently interleaved
layout creation initializes actual IDs rather than assuming every tree occupies
one contiguous interval.

## Compiled-code compatibility

The LLVM runtime layout changed. JIT cache keys include an explicit runtime ABI
version so existing cached kernels are not reused. Newly saved LLVM AOT modules
include a runtime_abi file; modules with a missing or incompatible marker are
rejected and must be regenerated. Keep this file with the module. This is a
runtime ABI boundary, not a claim of complete historical AOT compatibility.

## Validation tools

benchmarks/snode_patterns.py constructs many independent field layouts and
checks softened gravity plus centroid arithmetic against NumPy. It reports
construction, first execution, warm execution and readback separately. Optional
--memory-profile records existing runtime allocation counters (excluding
alignment); it adds synchronization and should not be used for clean latency
comparisons. Host RSS does not measure GPU memory use. See the adjacent benchmark
documentation for commands and interpretation.

Tests cover configured limits, actual interleaved IDs, larger node/tree counts,
reused tree slots, environment settings, and AOT ABI mismatches. The standalone
list-manager test covers concurrent first allocation, directory page boundaries,
out-of-order allocation, final directory slot, stable pointers and storage reuse.
