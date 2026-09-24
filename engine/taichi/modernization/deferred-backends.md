# Deferred: backend-specific work

Parked by decision on 2026-09-08. Not part of the core data-model work.
Revisit once the data model lands, and only for targets the related project
actually needs.

Baseline observed at fork point (upstream ba0e81d, 2025-07-30).

## Portable path runs entirely through SPIR-V

One SPIR-V code generator feeds Vulkan natively, and reaches Metal, OpenGL and
DirectX 11 by translation through SPIRV-Cross. There is no separate Metal or
OpenGL compiler. This spine is the whole basis of hardware agnosticism.

## Backend state at fork point

| Backend | Build default | RHI lines | Notes |
|---|---|---|---|
| CUDA | ON | 1442 | most complete, vendor locked |
| Vulkan | OFF | 1432 | most complete portable path, ships disabled |
| Metal | ON | 875 | via SPIRV-Cross |
| AMD GPU | OFF | 622 | thin |
| OpenGL | ON | 386 | device layer has many TI_NOT_IMPLEMENTED |
| DirectX 11 | OFF | 385 | marked WIP in archs.inc.h |
| DirectX 12 | OFF | 20 | stub only |

Build toggles live in `cmake/TaichiCore.cmake`.

## How to read this file

**The table below measures the wrong thing.** It ranks backends by lines of
code without separating compute from graphics. Per plan section 2.2b, a purely
graphics gap is irrelevant here and a compute gap is left open rather than
closed or removed. The OpenGL device layer's unimplemented set was checked on
2026-09-09 and is entirely graphics: raster pipelines, render passes, draw
calls, surfaces, present, resize. On compute criteria that backend is not thin.

Nothing in this file is dropped. Incomplete compute paths stay open.

## Items

- [ ] `TI_WITH_VULKAN` defaults OFF. Wrong default for a vendor-agnostic target.
- [ ] OpenGL device layer is dotted with `TI_NOT_IMPLEMENTED`
      (`taichi/rhi/opengl/opengl_device.cpp`). Decide: complete, or drop for Vulkan.
- [ ] DirectX 12 is a 20-line stub. Decide whether it is in scope at all.
- [ ] AMD GPU backend is thin and defaults OFF. Needed for true vendor agnosticism.
- [ ] Confirm the SPIRV-Cross path to Metal actually works before relying on it.
