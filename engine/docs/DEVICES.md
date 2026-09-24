> **Recovered September 2026 record.** This document preserves the native-engine development/vetting context in which it was written. Absolute `/opt/project/...` paths, agent-routing instructions, and statements about branch/commit status are historical. Current paths/status are indexed in [README.md](README.md); current repository policy is in the root `AGENTS.md` and `CONTRIBUTING.md`.

# Devices on this host

Measured with `build/engine_devices` on 2026-09-12, not quoted from
anywhere. Rerun it after any driver or hardware change.

## Host

| | |
|---|---|
| CPU | AMD FX-6100, six cores, six threads |
| Memory | 15.5 GiB |
| Launch threads the engine picks | 6, from `std::thread::hardware_concurrency()` |
| NVIDIA driver | 535.309.01 |
| LLVM | 15.0.7 |

## GPUs

| Visible index | Device | Compute capability | Memory | Usable by the engine |
|---|---|---|---|---|
| 0 | GeForce GTX 1070 | 6.1 | 7.92 GiB total, 7.84 GiB free | yes |
| 1 | GeForce GTX 750 Ti | 5.0 | 1.95 GiB total, 1.92 GiB free | deferred, see below |

## Which device the engine uses

The engine binds visible ordinal zero and retains a primary context on it
(`taichi/rhi/cuda/cuda_context.cpp:22`). There is no setting for the device
index, and the context is a process-wide singleton, so the choice is made
before CUDA starts and cannot change afterwards.

Select by visibility, either at launch or in process before any CUDA call:

```sh
CUDA_VISIBLE_DEVICES=1 ./build/engine_smoke_test   # bind the 750 Ti
./build/engine_devices --visible 1                 # same, from the tool
```

`engine::set_visible_cuda_devices` does this in process and refuses, changing
nothing, once CUDA has been touched. `TI_VISIBLE_DEVICE` is read only by the
Vulkan loader (`taichi/rhi/vulkan/vulkan_loader.cpp:111`) and has no effect
on the CUDA path.

## The 750 Ti is deferred, not qualified

Binding it succeeds and an instance starts, but the first field allocation
fails:

```
CUDA Error CUDA_ERROR_NOT_SUPPORTED: operation not supported
while calling malloc_async_impl (cuMemAllocAsync)
```

The engine picks the asynchronous memory-pool allocation path from a device
attribute query (`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`,
`cuda_context.cpp:39`), and on this driver that query reports support for both
cards: `engine_devices` prints `memory pool path on` for the 750 Ti too. The
device then refuses the call.

Patrick's reading, 2026-09-12: the card reports the capability and the driver
states compliance, so something is disconnected or missing between them. It
may be addressable by a route that does not go through this CUDA call, or the
card may be usable only through the 32-bit implementation. Either way this is
optional for now and not being chased. The 1070 is the GPU to target, and the
CPU path works whichever card is visible.

## What an instance costs

Free device memory before and after construction, at the engine's default
capacities:

| Device | Reserved at startup |
|---|---|
| GTX 1070 | 0.029 GiB |
| GTX 750 Ti | 0.010 GiB |

Raising the capacities to 8192 SNodes and 2048 trees did not change the
reserved figure, so the metadata tables are small next to the allocation
granularity. `device_memory_GB` is a pool ceiling rather than an up-front
reservation: at its default of 1 GiB, startup took 29 MiB.

## What resident data costs

One dense array of `i32` cells on the 1070:

| Cells | Reserved | Per cell |
|---|---|---|
| 1,048,576 | 0.031 GiB | 32 bytes |
| 16,777,216 | 0.062 GiB | 4 bytes |

The second row is the real cost, four bytes for a four-byte cell. The first
row is allocation granularity: reservations move in 32 MiB steps, so a small
field still takes 32 MiB.

For sizing a resident set, Patrick's constraint is that a particle's data is
database rows, a few hundred at most, with no vertex or mesh payload. So the
device question is how many rows fit: rows multiplied by bytes per row,
against 7.84 GiB free on the 1070, in 32 MiB steps per field, with the pool
ceiling set by `device_memory_GB`. The bytes-per-row figure is not settled
yet, so no particle count is claimed here.

## The 1070 envelope

Measured with `engine_devices` on this host. The costs are split by when they
are paid, because a model that fixes its pool at instantiation pays the first
group once and the second group on every tick.

### Paid once, at instantiation

| Quantity | Measured |
|---|---|
| Instance startup | 0.029 GiB reserved, unchanged from default capacities up to 8192 SNodes and 2048 trees |
| Pre-allocating 256 MiB of cells | 17.0 ms |
| Pre-allocating 1 GiB of cells | 26.5 ms |
| First field in a process | about 15 ms, later fields about 2 ms, the difference being first-time runtime setup |
| One tree of eight leaves | about 0.8 ms, from 4,000 trees in 3.16 s |
| First launch of a kernel | 30 to 80 ms, which is its compilation |

### Paid on every access

| Quantity | Measured |
|---|---|
| Launch floor, synchronized | about 0.04 ms |
| 4.2M cells, 512 dependent multiply-adds, f32 | 1.59 ms |
| 4.2M cells, 512 dependent multiply-adds, f64 | 41.48 ms, 26.1 times f32 |
| 16.8M cells, 64 multiply-adds, f32 | 0.356 ms |
| 16.8M cells, 64 multiply-adds, f64 | 1.555 ms, 4.4 times f32 |

The two f64 ratios are the useful part. Where the kernel is arithmetic-bound
the card's double-precision rate dominates and f64 costs about 27 times f32.
Where the kernel is bandwidth-bound, which is the 16.8M-cell row, the penalty
falls to 4.4 times because the time goes on moving eight bytes per cell
instead of computing. A row-shaped working set with little arithmetic per row
sits nearer the second case.

Below about 0.05 ms a measurement is launch overhead rather than work, so size
a timing above that before reading anything into it.

### Memory

| Quantity | Measured |
|---|---|
| Free device memory | 7.84 GiB of 7.92 GiB |
| Field storage | exactly 4 bytes per `i32` cell at 16.8M, 67M and 268M cells |
| Reservation granularity | 32 MiB, so a small field still takes 32 MiB |
| Metadata | 4,000 live trees and 40,000 identifiers inside one 32 MiB reservation |

`device_memory_GB` is the pool ceiling, not an up-front reservation: at its
default of 1 GiB, startup took 29 MiB, and a 1 GiB field needed the ceiling
raised to fit.

One observation for a long-lived process: destroying an instance does not
return everything to the driver. After a run that reserved 1 GiB, free memory
came back to 7.70 GiB rather than 7.84 GiB. A model that fixes one pool at
instantiation and keeps it never meets this; repeatedly building and dropping
instances would.

For contrast, the same 4.2M-cell arithmetic on the six CPU threads takes
266 ms in f32, with f64 costing the same. The CPU is the reference path, not
the throughput path.

## Sizing a fixed pool

Patrick's constraints, 2026-09-12: a particle's data is database rows, a few
hundred at most, absolutely consistent in shape, and used only as variables in
calculations. So the payload is a fixed number of numeric slots per particle,
the pool can be reserved once at instantiation, and dense containers are
enough for it; the sparse path matters only if occupancy itself is sparse.
Consistent shape also means one compiled kernel serves the whole set, since
nothing about the layout varies per particle.

Capacity is then division. Storage measured out at exactly 4 bytes per
four-byte slot, so a reserved pool holds `pool bytes / (slots per particle x
4)` particles:

| Slots per particle | Particles per 1 GiB | Particles in 7.5 GiB |
|---|---|---|
| 64 | 4.19M | 31.4M |
| 256 | 1.05M | 7.9M |
| 1024 | 262k | 2.0M |

Reserving 1 GiB took 26.5 ms, once. Each separate field rounds up to a 32 MiB
reservation, so a few wide fields cost less overhead than many narrow ones.
Whether a slot should be four or eight bytes is a precision decision with the
measured cost in the access table above.
