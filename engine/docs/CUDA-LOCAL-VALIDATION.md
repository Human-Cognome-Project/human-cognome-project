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

Each test requests CUDA through `settings.cuda` (`runtime.cpp:40` passes it to
the Program as the requested arch). No backend check was recorded for them, so
these results do not assert the actual Program backend.

```text
PASS engine_smoke_test
PASS field_test
PASS native_activity_test        (--cuda)
PASS activity_assembly_test      (--cuda)
PASS active_transfer_experiment  (--cuda)
```

## What the `instance: arch` line shows

`engine_devices` prints that line from its own request, not from the running
Program: `engine_devices.cpp` chooses `use_cuda = cuda_available() && !cpu_only`,
sets `settings.cuda` from it, and prints the literal `"cuda"` or `"x64"` from
that same flag. Only the capacities and memory budget come from
`runtime.program().compile_config()`. The line therefore records the requested
architecture. This fork's `llvm_runtime_executor.cpp:44-62` falls back to the
host architecture when no CUDA driver or device is visible (maintainer finding
in #158), so a requested CUDA backend is not by itself proof of actual CUDA.

The five test results are in the same position: they request CUDA and no
backend check was recorded for them. The recorded device-binding lines show that
the engine bound device 0; they are not a backend check, and no memory
consumption recorded during the tests is cited as evidence. No fallback is
claimed or indicated for this run. Future evidence should check the actual
Program backend (`runtime.program().compile_config().arch`), including a
negative case with no visible device (for example `CUDA_VISIBLE_DEVICES=`
empty) that must not report CUDA.

## Scope

These five checks are bounded local functional evidence for the existing
fixture path. They do not validate engine-owned object insertion (#156, not
built). Functional execution on the GTX 1070 only. No transfer timings, pinning,
performance or GTX 750 Ti results are recorded or implied. Local GPU
transfer/pinning qualification remains open in #145.
