# Local CUDA functional run, 2026-10-20

Partial evidence toward Gate 2 in [BUILD-VETTING.md](BUILD-VETTING.md); the
build recipe is in [engine/README.md](../README.md#local-cuda-build). It was
observed by the coordinating agent, not recorded by the maintainer, on repository
`main` at `8663215` with the in-tree Taichi fork at the same commit.

## Machine

Ubuntu 24.04, kernel 6.17.0-40, NVIDIA driver 535.309.01, LLVM/clang 15, no CUDA
toolkit installed.

## `engine_devices`

```text
host: 6 cpu threads, 15.5 GiB memory
cuda device 0: NVIDIA GeForce GTX 1070, compute capability 6.1  <- the engine binds this one
cuda device 1: NVIDIA GeForce GTX 750 Ti, compute capability 5.0
bound: NVIDIA GeForce GTX 1070, 7.92 GiB total, 7.84 GiB free, memory pool path on
instance: arch cuda, snode capacity 1024, tree capacity 512, device memory budget 1.00 GiB, 6 launch threads
instance device memory: 0.029 GiB reserved at startup
after release: 7.83 GiB free
```

## Functional tests

```text
PASS engine_smoke_test
PASS field_test
PASS native_activity_test        (--cuda)
PASS activity_assembly_test      (--cuda)
PASS active_transfer_experiment  (--cuda)
```

## Scope

Functional execution on the GTX 1070 only. No transfer timings, pinning,
performance or GTX 750 Ti results are recorded or implied. Local GPU
transfer/pinning qualification remains open in #145.
