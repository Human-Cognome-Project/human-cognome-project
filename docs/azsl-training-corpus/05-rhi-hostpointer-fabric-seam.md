# 05 — The RHI Host-Pointer / Staging Fabric Seam

**Audience:** the DI who will port the CPU↔GPU staging *fabric* (not the solver) from CUDA to O3DE's RHI.
**Scope:** RESEARCH ONLY. No port code here. This is the 90%-of-the-work part — the fabric.
**Doctrine carried in:** a GPU buffer address is a *scope-unique opaque slot*; **identity lives only on the CPU**. The GPU never knows what a row "is" — it knows position. The CPU holds the compact↔explicit map (the backtrace) and re-attaches identity by buffer **position** on readback.

What we are replacing (the CUDA fabric):

> pinned host memory (`cudaHostRegister` / `cudaHostAlloc`) → DMA stream the single-owner LMDB scratchpad bytes into VRAM → run compute → mirror results back → re-attach identity by buffer position via a CPU-held backtrace.

The question this doc answers: **how far does O3DE RHI take us, and exactly where do we drop to raw Vulkan?**

---

## TL;DR — the two findings that decide the architecture

1. **The RHI-native per-frame path is a *staging copy*, not zero-copy.** RHI's public buffer model gives you `Map → memcpy → Unmap` into a host-visible buffer, and for device-local compute buffers it transparently inserts a staging buffer + `vkCmdCopyBuffer` + barriers via a *resolver*. This is the idiomatic, portable, RHI-blessed way to get CPU bytes into a compute buffer per frame. It is a **copy**, full stop.

2. **O3DE's Vulkan backend *already contains* a complete `VK_EXT_external_memory_host` import path** — `BufferMemory::InitWithExternalHostMemory(...)`, which takes an arbitrary `void* allocatedHostMemory` and imports it as `VkDeviceMemory`. **But it is gated** behind an internal `usedForCrossDevice` flag and is **not exposed through any public RHI knob.** The machinery for our zero-copy LMDB dream exists; the *front door* to it does not. Reaching it for our purpose means either (a) a tiny engine patch to expose it, or (b) bypassing RHI's buffer init entirely and driving raw Vulkan through the documented native-handle escape hatch.

So the honest recommendation is **not** "RHI can't do it, drop to Vulkan." It's: **RHI's portable abstraction does a copy; the zero-copy import you want is sitting in the Vulkan backend behind a private door, and the realistic port is RHI-native staging copy first, with the host-pointer import as a Vulkan-specific fast path you unlock deliberately.** Evidence below.

---

## 1. The O3DE RHI buffer model (portable layer)

All paths from local headers in `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI/`.

### 1.1 Pools, heaps, and the host/device split

A buffer's memory character is fixed by its **pool descriptor**, not the buffer. `BufferPoolDescriptor` (`RHI.Reflect/BufferPoolDescriptor.h:40-51`):

- `HeapMemoryLevel m_heapMemoryLevel` — `Host` (CPU-local, mappable, slower GPU read) or `Device` (VRAM, needs a staging transfer). Default `Device`.
- `HostMemoryAccess m_hostMemoryAccess` — `Write` (CPU writes, GPU reads — *uploads*) or `Read` (GPU writes, CPU reads — *readback*). Device heap **must** be `Write`.

The enums themselves (`RHI.Reflect/MemoryEnums.h:22-38`):

```cpp
enum class HostMemoryAccess : uint32_t { Write = 0, Read };
enum class HeapMemoryLevel  : uint32_t { Host = 0, Device };
```

This is the whole knob set. There is **no** `m_importHostPointer`, **no** `m_externalMemory`, **no** way in the public descriptor to say "back this buffer with my LMDB mmap." That absence is the load-bearing fact of this document.

### 1.2 The map → memcpy → unmap path

`DeviceBufferPool` (`DeviceBufferPool.h`) is the platform-agnostic interface. Per-frame upload is:

- `InitBuffer(DeviceBufferInitRequest{buffer, descriptor, initialData})` — `:121`. Optional `m_initialData` does the first upload.
- `MapBuffer(DeviceBufferMapRequest{buffer, byteOffset, byteCount}, response)` → `response.m_data` is your CPU pointer — `:158`, response struct `:61-64`. You `memcpy` your bytes in.
- `UnmapBuffer(buffer)` — `:162`. Unblocks the GPU.
- `OrphanBuffer(buffer)` — `:144`. **Host pools only.** Allocates a *fresh* backing allocation so you can overwrite the whole buffer for a new frame without CPU/GPU hazard tracking — the N-buffering / round-robin escape (doc comment `:125-144`). This is the idiomatic "I'm rewriting the whole scratchpad each frame" move.

The multi-device public wrapper `BufferPool` / `BufferMapRequest` (`BufferPool.h`) layers a `MultiDevice::DeviceMask` on top; `BufferMapResponse.m_data` becomes a `unordered_map<int deviceIndex, void*>` (`BufferPool.h:20-24`). For a single-GPU compute engine you can stay on the `Device*`-suffixed API and ignore the device mask.

Map semantics worth memorizing (`DeviceBufferPool.h:146-158`):
- Read vs write access is dictated by the **pool type**, not the call. Host-Read pools read GPU-written data; everything else is write-only from the CPU.
- Map is **reference-counted** and **nestable for disjoint regions**; every `Map` needs an `Unmap`.
- Mapping **blocks the frame scheduler** from recording staging ops — unmap before the execute phase.

### 1.3 Staging upload, async copy queue, CopyItem

Two distinct upload mechanisms:

- **Synchronous / frame-scheduler staging** — for a `Device`-heap pool, `MapBuffer` doesn't hand you VRAM. It hands you a *staging buffer* and defers a copy (see §3, the resolver). The copy is recorded as a `DeviceCopyItem` into the frame graph.
- **Async streaming** — `StreamBuffer(DeviceBufferStreamRequest{...})` (`DeviceBufferPool.h:167`, request struct `:67-86`) is **decoupled from the frame scheduler**, runs on a dedicated async upload queue, and **signals a fence on completion** (`m_fenceToSignal`). The source data must stay alive until the fence fires. This is the closest RHI analogue to your CUDA "DMA stream and signal."

`CopyItem` (`CopyItem.h:168-247`) / `DeviceCopyItem` (`DeviceCopyItem.h:86-127`) is the GPU-timeline copy primitive. The relevant variants for the fabric:
- `CopyBufferDescriptor` — buffer→buffer (staging→device, or device→readback). `:18-40`.
- `CopyImageToBufferDescriptor` / `CopyBufferToImageDescriptor` — if results ever land in an image. `:71-139`.

**Idiomatic per-frame "CPU bytes → compute buffer" the RHI way:** create a `Device`-heap pool with `BufferBindFlags::ShaderReadWrite` (compute), `MapBuffer` the region → memcpy → `Unmap`; the frame scheduler's resolver stages it across with a `CopyItem` and inserts the barriers. For whole-buffer-per-frame rewrites, `OrphanBuffer` a Host-heap mirror or N-buffer. None of this is zero-copy: your LMDB bytes are touched at least once by the CPU into a staging buffer.

---

## 2. Does RHI expose host-pointer import / external memory? (the escape hatch)

**Public RHI: no.** There is no external-memory or host-pointer-import surface anywhere in `Code/Include/Atom/RHI` or `RHI.Reflect`. The descriptor knobs in §1.1 are the entire contract.

**But the Vulkan backend ships a documented native-handle escape hatch.** This distro ships only the *public/reflect* headers for the Vulkan gem (28 headers, **0 `.cpp`** under `Gems/Atom/RHI/Vulkan/Code` — the implementation lives in engine source). The one interface header that matters is `RHIVulkanInterface.h`:

`/opt/O3DE/25.10.2/Gems/Atom/RHI/Vulkan/Code/Include/Atom/RHI.Interface/Vulkan/RHIVulkanInterface.h`

```cpp
namespace AZ::Vulkan {
    VkDevice         GetDeviceNativeHandle(RHI::Device& device);          // :30
    VkPhysicalDevice GetPhysicalDeviceNativeHandle(const RHI::PhysicalDevice&); // :31
    VkBuffer         GetNativeBuffer(RHI::DeviceBuffer& buffer);          // :38
    VkDeviceMemory   GetBufferMemory(RHI::DeviceBuffer& buffer);          // :39
    size_t           GetBufferMemoryViewSize(RHI::DeviceBuffer& buffer);  // :40
    size_t           GetBufferAllocationSize(RHI::DeviceBuffer& buffer);  // :41
    size_t           GetBufferAllocationOffset(RHI::DeviceBuffer& buffer);// :42
}
```

The header's own preamble (`:22-27`) states the contract for using these: **you must synchronize with the renderer by waiting on a FrameGraph fence before executing, signal the FrameGraph to continue, leave the GPU in a valid state, and return resources in a valid state.** This is the sanctioned door from RHI down to the raw `VkDevice`. From `GetDeviceNativeHandle` you have a real `VkDevice` and can call `vkAllocateMemory` with an import chain yourself — i.e. you can build the zero-copy import *beside* RHI even though RHI's buffer init won't do it for you.

`VkAllocator.h` (`RHI.Reflect/VkAllocator.h`) confirms the backend routes Vulkan allocations through `VkSystemAllocator::Get()` callbacks — cosmetic for us, but tells you the backend owns its allocation conventions.

---

## 3. How the Vulkan backend *actually* moves bytes (from upstream o3de/o3de source)

The shipped distro has no `.cpp`; these are fetched from `github.com/o3de/o3de` (`development`), path `Gems/Atom/RHI/Vulkan/Code/Source/RHI/`. They are the real implementation behind the headers above.

### 3.1 The staging-copy path (this is what "RHI-native" costs you)

`BufferPool.cpp::MapBufferInternal` branches on heap level:

- **Host heap** (`BufferPool.cpp:177-193`): `buffer->GetBufferMemoryView()->Map(...)` returns the mapped pointer + `m_byteOffset` directly. Real zero-extra-copy *if* the data already lives in this host buffer. But this buffer's memory was allocated by **VMA** (`BufferMemory::Init`, see §3.3) — it is *not* your LMDB memory.
- **Device heap** (`BufferPool.cpp:194-206`): `GetResolver()->MapBuffer(...)` — hands back a **staging buffer's** mapped pointer, not the device buffer.

`BufferPoolResolver.cpp` is the staging engine:
- `MapBuffer` (`:25-48`): `AcquireStagingBuffer(byteCount, alignment)`, map it `Write`, queue a `BufferUploadPacket{attachmentBuffer, stagingBuffer, byteOffset, byteSize}`. Returns the **staging** address — *you memcpy into the staging buffer, never the device buffer*.
- `Compile` (`:50-94`): builds `VkBufferMemoryBarrier` prologue (→`TRANSFER_WRITE`) and epilogue (→shader access) barriers per packet.
- `Resolve` (`:96-116`): for each packet, `commandList.Submit(DeviceCopyItem(CopyBufferDescriptor{staging→dest}))` then releases the staging buffer.

So the device-heap per-frame path is, concretely: **your bytes → memcpy into VMA staging buffer → `vkCmdCopyBuffer` → device buffer**, fenced and barriered by the frame graph. One mandatory CPU-touch copy into staging, one GPU DMA copy across. This is the portable baseline and it is **correct, robust, and not zero-copy.**

### 3.2 The host-pointer import path — present, complete, and gated

`BufferMemory.cpp::InitWithExternalHostMemory` (two overloads, `:96-209`) is a textbook `VK_EXT_external_memory_host` import:

- `:140-145` — `GetMemoryHostPointerPropertiesEXT(device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, allocatedHostMemory, &hostMemoryProps)` to learn which memory types can wrap this pointer.
- `:150-165` — pick a **non-`DEVICE_LOCAL`**, host-visible memory type whose bit is set in `hostMemoryProps.memoryTypeBits` (prefers the *fewest* property flags).
- `:167-176` — `VkImportMemoryHostPointerInfoEXT{ handleType = ...HOST_ALLOCATION_BIT_EXT, pHostPointer = allocatedHostMemory }`, chained as `allocInfo.pNext`.
- `:179` — `vkAllocateMemory` — **imports your pointer as `VkDeviceMemory`. No copy.**
- `:183-196` — buffer created with `VkExternalMemoryBufferCreateInfo`, `:200` — `vkBindBufferMemory(buffer, memory, 0)`.

The first overload (`:96-120`) shows the alignment discipline you must honor:
```cpp
alignment = PhysicalDevice::GetExternalMemoryHostProperties().minImportedHostPointerAlignment;
m_allocatedHostMemorySize = RHI::AlignUp(descriptor.m_byteCount, alignment);
m_allocatedHostMemory.reset(AZ_OS_MALLOC(m_allocatedHostMemorySize, alignment));
```
i.e. **base pointer and size must both be multiples of `minImportedHostPointerAlignment`** (a power of two; typically 4 KiB, matching a page — which is *exactly* what an mmap region already satisfies).

**The gate.** `BufferPool.cpp::InitBufferInternal` (`:68-124`) only calls `InitWithExternalHostMemory` when `usedForCrossDevice == true` (`:92-110`); otherwise the normal `BufferMemory::Init` (VMA) runs (`:112-116`). And `usedForCrossDevice` is only ever set true by `InitBufferCrossDeviceInternal` (`:59-66`), which exists for **multi-GPU host-shared buffers** — its sole upstream user is `RPI CopyPass.cpp` (cross-device copy). The feature itself is gated on the extension: `Device.cpp:1578-1579` sets `m_features.m_crossDeviceHostMemory = IsOptionalDeviceExtensionSupported(ExternalMemoryHost)`. Extension registration: `PhysicalDevice.cpp:380` lists `VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME` as an **optional** device extension; `:494, :539-541` populate/append `VkPhysicalDeviceExternalMemoryHostPropertiesEXT`.

**Conclusion for §2/§3:** the exact `VkImportMemoryHostPointerInfoEXT` import we need is implemented, tested, and shipping in O3DE — but wired only to cross-device sharing, with the host memory **`AZ_OS_MALLOC`'d by the engine**, never an externally supplied LMDB pointer through a public API. There is no `BufferPoolDescriptor` flag and no `BufferInitRequest` field that reaches it.

### 3.3 Why the normal path can't be coerced into zero-copy

`BufferMemory::Init` (`:51-94`) allocates via **VMA**: `vmaCreateBufferWithAlignment(GetVmaAllocator(), createInfo, &allocInfo, ...)`. VMA owns the `VkDeviceMemory`, sub-allocates pages from large blocks, and there is no VMA entry point that wraps a *caller-supplied host pointer* as the backing store. Host-pointer import is fundamentally a `vkAllocateMemory`-with-`pNext` operation on a **whole, dedicated** `VkDeviceMemory` object (one import = one allocation; cannot be sub-allocated out of a VMA pool). That structural mismatch is exactly why O3DE's import path bypasses VMA and calls `vkAllocateMemory` directly (`BufferMemory.cpp:179`). You cannot get zero-copy by tweaking pool descriptors; it requires the dedicated-allocation import path.

---

## 4. `VK_EXT_external_memory_host` mechanics (Vulkan spec) — registering the LMDB mmap

Sources: Khronos registry + Vulkan Docs (URLs at end). Mechanics, matched to the code in §3.2:

1. **Enable** `VK_EXT_external_memory_host` as a device extension (O3DE already treats it as optional-on — `PhysicalDevice.cpp:380`).
2. **Query alignment** once: `VkPhysicalDeviceExternalMemoryHostPropertiesEXT::minImportedHostPointerAlignment` — power-of-two minimum alignment for **both base address and size** of an importable host pointer. An LMDB `mmap` region is page-aligned and page-sized, so it satisfies this natively (the dangerous case is a *sub-range* of the mmap that isn't page-aligned).
3. **Validate the pointer**: `vkGetMemoryHostPointerPropertiesEXT(device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, ptr, &props)` → `props.memoryTypeBits` tells you which memory types may wrap *this specific* pointer. Pick a host-visible, non-device-local type from that mask (O3DE's selection at `BufferMemory.cpp:150-165`).
4. **Import**: `vkAllocateMemory` with `pNext = &VkImportMemoryHostPointerInfoEXT{ handleType = ...HOST_ALLOCATION_BIT_EXT, pHostPointer = ptr }`, `allocationSize = page-aligned size`, `memoryTypeIndex = chosen`. Result: a `VkDeviceMemory` that *is* your LMDB pages. No staging, no DMA, no copy — the GPU reads host memory across the bus (BAR / PCIe), or on UMA/iGPU it is genuinely the same RAM.
5. **Bind & use**: create the `VkBuffer` with `VkExternalMemoryBufferCreateInfo{ handleTypes = ...HOST_ALLOCATION_BIT_EXT }`, `vkBindBufferMemory(buffer, memory, 0)`.

**Honest tradeoff: true zero-copy import vs. RHI staging copy.**

| Axis | RHI staging copy (§3.1) | Host-pointer import (§3.2/§4) |
|---|---|---|
| Copies per frame | ≥1 CPU memcpy + 1 GPU DMA | **0** |
| Portability | All RHI backends (DX12/Vulkan/Metal) | Vulkan-only; needs the extension; absent on many drivers/HW |
| GPU read speed | Full VRAM bandwidth after copy | **Slow** — reads cross PCIe/BAR; no VRAM locality (discrete GPU). On UMA/iGPU, near-parity |
| Coherency burden | RHI/frame-graph handles barriers | You own host/device coherency, flushes, `minImportedHostPointerAlignment`, sub-range alignment |
| LMDB fit | LMDB stays single-owner; you copy *out* of it | LMDB pages become GPU-visible directly — but LMDB's copy-on-write / page remaps under a writer can move pages out from under the import |
| Reachable via public RHI | **Yes** | **No** — gated (§3.2); needs engine patch or raw-Vulkan side path |

The decisive nuance: import wins **only** when the GPU's access pattern over the LMDB bytes is *streaming / low-reuse* (read each byte ~once) **or** the hardware is UMA. For a compute kernel that re-reads the scratchpad many times, the VRAM-resident staged copy is faster despite the copy, because import reads stay on the slow bus every access. Zero-copy is not free; it trades copy cost for per-access bus cost.

---

## 5. The backtrace / readback seam (the riskiest part, and our doctrine)

### 5.1 How results come back (RHI readback)

Readback is the mirror of upload, expressed through the **pool type**:

- Create a **Host heap, `HostMemoryAccess::Read`** pool (`BufferPoolDescriptor.h:44-46`: "Host 'Read' pools are written by the GPU and read by the CPU"). The compute pass writes results into a buffer the CPU can later map.
- Or keep results in a `Device` buffer and copy them out with a `CopyBufferDescriptor` (device→host-read buffer) — a `CopyItem` (`CopyItem.h:18-40`), the reverse of the resolver's upload.
- Then `MapBuffer` the Host-Read buffer (`BufferPool.cpp:177-193` Host branch returns the mapped pointer directly) → CPU reads. `DeviceBufferPool.h:146-149` confirms: "Host pools with host read access may read from the buffer—the contents of which are written by the GPU."
- Synchronize on the frame fence (or `StreamBuffer`'s completion fence) before reading, per the `RHIVulkanInterface.h:22-27` contract.

**If we use host-pointer import for readback too**, results land *in the LMDB pages directly* (GPU writes through the import) — no readback copy at all. Same tradeoff table applies, plus you must `vkInvalidateMappedMemoryRanges` / honor coherency before the CPU trusts the bytes.

### 5.2 Identity re-attachment by position — the doctrine, made concrete

Nothing in RHI or Vulkan carries identity. A `VkBuffer` + offset is, by our doctrine, a **scope-unique opaque slot**: the address means "slot N in this dispatch," nothing more. The GPU computes over positions; it has no notion of which concept/row/entity slot N holds.

Identity lives **only on the CPU**, as the compact↔explicit backtrace map: the CPU decides the packing order when it lays bytes into the staging/host/imported buffer (slot N ← entity E), and on readback re-reads slot N and looks up E. Buffer **position is the join key.** This means:

- The pack order on upload and the unpack order on readback must be the *same deterministic layout*; the map is the single source of truth and it never leaves the CPU.
- `OrphanBuffer` / N-buffering (`DeviceBufferPool.h:125-144`) is safe under this doctrine *only if* the backtrace is rebuilt/rebound to whatever layout the new frame uses — position identity is per-frame-layout, not stable across reallocation.
- GPU addresses must **never** be persisted as identity. They are scope-unique and reused; the moment a buffer is orphaned or a pool recycles a slot, any address-as-identity assumption is corrupted. The CPU map is the only durable identity.

This doctrine is *why* the staging-copy path is not a defeat: the CPU is already the marshaller that decides slot layout; doing the memcpy into staging is the same pass that establishes the position→identity binding. Zero-copy import removes the copy but **not** the marshalling — you still need the CPU to have laid the LMDB bytes out in the exact slot order the kernel and the backtrace agree on. If LMDB's on-disk/in-mmap order already equals the kernel's slot order, import is pure win; if it doesn't, you're copying-to-reorder anyway and import buys nothing.

---

## 6. Recommendation — RHI-native staging copy vs raw-Vulkan zero-copy import

**Stage 1 (baseline, do this first): RHI-native staging copy.** Build the fabric entirely on the portable RHI: `Device`-heap compute pool + `MapBuffer`/`memcpy`/`UnmapBuffer` (or `StreamBuffer` on the async queue for big uploads, fenced), `CopyItem` for device↔host-read readback, frame-graph barriers handled for you. It is correct, portable across DX12/Metal/Vulkan, and the resolver (§3.1) already does the barrier/fence dance. This proves the *fabric* — marshalling, position-identity backtrace, readback seam — independent of the zero-copy gamble. **The 90% (the fabric) is fully achievable on portable RHI with one mandatory copy.**

**Stage 2 (Vulkan fast path, unlock deliberately): host-pointer import.** Only if profiling shows the staging copy is the bottleneck *and* the access pattern is streaming or the target is UMA. Two ways in, in order of preference:

- **(a) Minimal engine patch** — expose the already-shipping `InitWithExternalHostMemory` through the public API: add an `m_importHostPointer`/`m_hostPointerSize` to `BufferInitRequest` (or a dedicated request) and let `InitBufferInternal` take the import branch without the `usedForCrossDevice` detour. This reuses O3DE's *tested* import path (alignment query, memory-type selection, `VkExternalMemoryBufferCreateInfo` binding) and keeps you inside RHI's lifetime/fence management. Lowest-risk route to zero-copy.
- **(b) Raw-Vulkan side path** — via `RHIVulkanInterface.h`: `GetDeviceNativeHandle` → do the `vkGetMemoryHostPointerPropertiesEXT` + `vkAllocateMemory(import)` + `vkBindBufferMemory` yourself, wrap the resulting `VkBuffer` for the kernel, and honor the `RHIVulkanInterface.h:22-27` fence/validity contract by hand. Maximum control, maximum coherency/lifetime burden, fully off the portable path. Use only if (a) is blocked.

**Do not** try to coerce VMA / pool descriptors into zero-copy — structurally impossible (§3.3).

---

## 7. The single most important open gap (the riskiest unknown of the whole port)

**Whether the LMDB mmap region can be safely imported as a *stable, page-aligned, non-relocating* `VkDeviceMemory` for the lifetime of a dispatch — and whether the GPU's access pattern over it actually beats a staging copy.** Three unresolved sub-risks stack here:

1. **LMDB page stability under import.** LMDB is mmap-backed with MVCC/copy-on-write. The import (`VkImportMemoryHostPointerInfoEXT`) pins a *virtual address + size*; if LMDB remaps, grows the map, or a writer triggers COW while the GPU is reading those pages, the import's backing can move or change underneath an in-flight dispatch. We have **not** verified that a read-only LMDB transaction holds the mmap region fixed for the dispatch duration, nor whether the single-owner scratchpad ever writes during a GPU read. This is a correctness landmine, not a perf detail.

2. **Alignment of the *sub-range* we actually want.** The whole mmap is page-aligned, but the scratchpad slice the kernel needs may start mid-page. `minImportedHostPointerAlignment` applies to base *and* size; a non-aligned slice forces either importing a superset (and offsetting in-shader) or copying-to-realign — which erases the zero-copy benefit.

3. **Does import actually win for *our* kernel?** Unmeasured. On a discrete GPU with a re-reading kernel, BAR/PCIe access can make import *slower* than staging-then-VRAM. We have no profile of the kernel's reuse factor over the scratchpad, and no confirmation of the deployment HW (UMA vs discrete). Until that exists, "zero-copy" is an assumption, not a result.

**Mitigation path:** build Stage 1 (staging copy) first so the fabric is provably correct and the backtrace/position-identity seam is validated independent of import; then prototype import behind a flag and *measure* (1)–(3) on the real corpus and real HW before committing the port to it.

---

## Sources

**Local O3DE 25.10.2 (file:line cited inline):**
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI/DeviceBufferPool.h` — Map/Unmap/Stream/Orphan, BufferMapRequest/Response
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI/BufferPool.h` — multi-device public wrapper
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI.Reflect/BufferPoolDescriptor.h` — heap/host-access knobs
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI.Reflect/MemoryEnums.h` — HeapMemoryLevel / HostMemoryAccess
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Code/Include/Atom/RHI/CopyItem.h`, `DeviceCopyItem.h` — copy primitives
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Vulkan/Code/Include/Atom/RHI.Interface/Vulkan/RHIVulkanInterface.h` — **the native-handle escape hatch** (GetDeviceNativeHandle / GetNativeBuffer / GetBufferMemory)
- `/opt/O3DE/25.10.2/Gems/Atom/RHI/Vulkan/Code/Include/Atom/RHI.Reflect/VkAllocator.h`

**Upstream o3de/o3de `development` (fetched via GitHub API; same files in engine source):**
- `Gems/Atom/RHI/Vulkan/Code/Source/RHI/BufferMemory.cpp` — **`InitWithExternalHostMemory` = the VK_EXT_external_memory_host import path** (lines 96-209); VMA path (51-94)
- `Gems/Atom/RHI/Vulkan/Code/Source/RHI/BufferPool.cpp` — the `usedForCrossDevice` gate (68-124), Map host/device branch (169-214)
- `Gems/Atom/RHI/Vulkan/Code/Source/RHI/BufferPoolResolver.cpp` — staging-copy engine (MapBuffer 25-48, Compile 50-94, Resolve 96-116)
- `Gems/Atom/RHI/Vulkan/Code/Source/RHI/PhysicalDevice.cpp` — extension registration & properties (380, 494, 539-541)
- `Gems/Atom/RHI/Vulkan/Code/Source/RHI/Device.cpp` — `m_crossDeviceHostMemory` feature gate (1578-1579)

**Vulkan spec / extension docs:**
- VK_EXT_external_memory_host — https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_external_memory_host.html
- VkImportMemoryHostPointerInfoEXT — https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkImportMemoryHostPointerInfoEXT.html
- vkGetMemoryHostPointerPropertiesEXT — https://khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetMemoryHostPointerPropertiesEXT.html
- VkPhysicalDeviceExternalMemoryHostPropertiesEXT (minImportedHostPointerAlignment) — https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceExternalMemoryHostPropertiesEXT.html
- Vulkan Memory Allocation (mapping constraints) — https://docs.vulkan.org/spec/latest/chapters/memory.html
- VMA memory mapping (single-mapping constraint) — https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
- O3DE Atom VMA API reference — https://docs.o3de.org/docs/api/gems/atom/usage_patterns.html
- IREE issue (real-world use of VK_EXT_external_memory_host for staging) — https://github.com/iree-org/iree/issues/7242
