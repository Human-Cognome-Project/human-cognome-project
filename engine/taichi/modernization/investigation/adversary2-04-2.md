# Adversary 04-2, round two — backend, AOT and build architecture

Judging the revised `report-04-backend-build.md` (pass A) and
`report-04b-backend-build.md` (pass B) against my round-one objections in
`adversary-04-2.md`, against adversary 04-1's round-one objections, and against
source.

Every file:line below I opened or grepped myself in this round. Where I confirm
a report I say what I ran. Where I correct one, the correction carries its own
citation.

---

## 1. Verdicts up front

**CORRECT — yes, both.** This is a real change from round one. Every
materially false `[V]` claim in either report has been withdrawn and replaced
with something the source supports. I re-verified roughly sixty citations
across both revisions and found four residual defects (§5), none of which
inverts a finding.

**COMPLETE — the pair, no. One gap survives both, and it lands on the
escalation each report ranks first.** Neither report notices that the clang
command line both of them quote verbatim already carries a CMake-driven `-D`
that reaches the bitcode. See §4. Pass A alone is now close to complete; pass B
alone is not, by its own statement.

**CONSENSUS — not yet reached.** One outstanding item, §4, plus four small
corrections in §5. Both are a correction pass, not a re-run.

---

## 2. Were my round-one objections addressed or merely acknowledged?

### 2.1 Pass A — all nine addressed, one residue

| My objection | Disposition | Verified |
|---|---|---|
| §3.1 `Extension::data64` as a compile-time blocker | **Addressed.** Withdrawn at `report-04:405-407` and `:1007`; replaced by 4.1, which now says the table gates nothing in C++ | I re-grepped `is_extension_supported`: 8 call sites in `taichi/`, passing `assertion` (`program.cpp:149`), `bls` (`codegen_llvm.cpp:2726`), `mesh` (`compile_to_offloads.cpp:92, 205, 218, 236`), `quant` (`:245, 288`), plus the pybind export at `export_lang.cpp:1225`. No site passes `data64`. Table content re-read at `extension.cpp:8-33`: exact, including the abandoned comment at `:29-30` and `operator[]` at `:31` |
| §3.2 `ti_device_api_shared` | **Addressed.** `report-04:110-121`. The content half is now refuted on link semantics | `wc -c taichi/rhi/dummy.cpp` → 0 |
| §3.3 "exactly three things" | **Addressed.** `report-04:64-65` now says "usual", with four counter-examples at `:85-95` | `TaichiCore.cmake:104-105`, `:62-64`, `:278-283`, `CMakeLists.txt:155, 159, 163` all re-read |
| §3.4 `TI_WITH_LLVM` dropped twice | **Addressed.** Nine `-D` sites incl. `:94` and `:281` at `report-04:66-69`; seven `#else` arms incl. `:94` at `:281-284` | re-read |
| §3.5 no non-boolean cache variable | **Addressed.** `report-04:190-203` now lists `CUDA_VERSION` and `HOST_ARCH` | `cmake/TaichiCXXFlags.cmake:149`, `ARCH` set at `:139, 142, 145`. 24 `option()` calls confirmed by count |
| §3.6 self-contradiction on 64-bit hardware queries | **Addressed.** `report-04:463-468` now names two backends | `metal_device.mm:1027-1038, 1051-1053` re-read |
| §3.7 counts and ranges | **Addressed, all of them.** | `archs.inc.h` 12 entries at `:4-17`; `wc -l taichi/aot/*` → 7 files, 836 lines exactly; `taichi_core_impl.h:121-122`; `runtime_lib_dir` closes `:47`; `set_caps` 8 sites |
| §3.8 gaps A had, B filled | **Addressed** for chain 3 (`report-04:289-293`, arch ranges re-verified line by line against `taichi_core_impl.cpp:247-309`), `spirv_has_physical_storage_buffer` (4.3), the CUDA device-0 pin (3.3) | see §3.5 and §3.2 below |

**Residue.** `report-04:26` still carries "no dlopen of Taichi's own code" as a
`[V]` claim with no citation, anywhere in the report. I flagged the absence of
support in round one. The claim is true — I re-confirmed that the only dlopen
wrapper is `taichi/common/dynamic_loader.cpp` and every construction site loads
a vendor driver — but pass A asserts it uncited while pass B supports it at
`report-04b:553-556`. Standing instruction 5 asks for file paths and line
numbers. Minor, and the fact is not in doubt.

### 2.2 Pass B — six addressed, one declined

| My objection | Disposition |
|---|---|
| §4.1 Metal `set_caps`, and the "exhaustive" grep | **Addressed, and the root cause named.** `report-04b:173-187` states the cause was `--include=*.cpp --include=*.h` with no `--include=*.mm`, removes "exhaustive" from every grep claim, and reports re-running the others |
| §4.2 AMDGPU runtime source | **Addressed.** `report-04b:970-982` |
| §4.3 dead copy of `DynamicLoader` | **Addressed.** `report-04b:544-551` now anchors on `taichi/common/dynamic_loader.h` |
| §4.4 `spirv_has_atomic_int64` grep overstated | **Addressed but freshly imprecise.** See §5.3 |
| §4.5 smaller slips | **Addressed:** `taichi/rhi/CMakeLists.txt:28` at `:118-119`; eleven options incl. `USE_STDCPP` at `:99-103`; `ti_get_available_archs` `:171-205` at `:596`; `cuda_context.cpp:21-22` at `:287-288` |
| §4.6 the `Extension` system, `CompileConfig::fit()`, the DX12 stub | **Acknowledged, not addressed.** `report-04b:1079-1084` says so explicitly: "flagged by review as a genuine hole in my coverage ... I did not re-derive them here and I am not restating that agent's findings as my own" |

On the last: I do not treat that as evasion. Restating another agent's findings
would corrupt the divergence signal the method runs on (plan §9.1). But it does
mean pass B alone is not complete, which pass B says.

**I tested B's re-run claim rather than accepting it.** B asserts that
re-running the `get_required_caps`, `spirv_has_physical_storage_buffer`,
`taichi_max_num_snodes` and `DynamicLoader` censuses without the file-type
filter changed nothing, "since no `.mm` file contains those symbols". There are
four `.mm` files in scope: `taichi/rhi/metal/metal_device.mm`,
`c_api/src/taichi_metal_impl.mm`, `taichi/ui/ggui/gui_metal.mm`,
`taichi/ui/ggui/nswindow_adapter.mm`. I grepped each of the four symbols plus
`spirv_has_atomic_int64` and `kMaxNumSnodeTreesLlvm` across `--include=*.mm`:
zero hits for every one. **B's claim holds.** The `set_caps` census is the only
one the filter affected, and B corrected it.

### 2.3 The five items I said both reports must add

All five landed in both.

| Item | Pass A | Pass B |
|---|---|---|
| §6.1 umbrella header, `TI_WITH_CPU` | 3.5, `:348-374` | 4.4, `:610-652` |
| §6.2 one `required_caps` producer, generic struct dead | 5.1, `:640-647` | 3.3, `:429-438` |
| §6.3 renumbering also moves cache keys | 4.6, `:608-617` | 2.6, `:313-320` |
| §6.4 `.bc` already produced into the source tree | 2.5, `:180-186` | 5.3, `:766-773` |
| §6.5 the `-I` route is not a clean substitution | 2.5, `:172-178` | 5.3, `:756-764` |

Re-verified: `c_api/include/taichi/taichi.h` is 29 lines, gates at `:9, 13, 17,
21, 25`; `TI_WITH_CPU` outside `external/` and `build/` returns only `taichi.h:21`,
`:23` and `docs/lang/articles/deployment/tutorial.md:288` (pass A found the
docs hit, pass B's narrower source-file scope correctly excludes it).
`get_offline_cache_key_of_device_caps` at `offline_cache_util.cpp:88-95`
serialises `caps.devcaps` at `:92`, called `:185`, hashed `:190`;
`taichi_max_num_indices` loop at `:110`. `runtime_module/CMakeLists.txt:10`
`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`, install from
`${CMAKE_SOURCE_DIR}` at `:13`, TODO at `:9`. All exact.

---

## 3. The five places the explore agents ruled against round one

### 3.1 Capability ordering — pass A is right that neither adversary had it

**The source half, settled.**

*Fact 1, and pass A's reading of it is correct.*
`c_api/include/taichi/taichi_core.h:411-412` reads "It currently is not
guaranteed that a higher level value is compatible with a lower level value."
That declines to promise a relation. It does not assert the negation. Adversary
04-1's round-one position — "`!=` is the only relation the documented type
supports" — reads a prohibition into a non-guarantee and overreaches. Pass A's
adjudication at `report-04:554-560` is right.

My own round-one position was weaker but also not right: my escalation 5 said
"Both reports escalate it and both are right to", which accepted their framing
of it as a defect, and I had not seen `:411-412`. Pass A's ruling against me
holds on that point. In mitigation, I had already put it in Escalations rather
than in findings, which is where both reports now have it.

*Fact 2, pass B's verification of the provenance, which I confirm and which
neither adversary had.* `report-04b:480-485` says the sentence is authored
prose in `c_api/docs/taichi/taichi_core.h.md:281` rather than generated. I
traced the direction: `misc/taichi_json.py:260-266` defines
`class Documentation` reading `Path(f"c_api/docs/{name}")`, wired at `:353-354`,
and `misc/generate_c_api.py:54` emits `// {…}` comment lines from
`module.doc.api_refs`. The `.md` is the input; the header comment is the
generated artefact. B is right, and this matters: changing the contract means
editing a documentation file, not just a header.

*Fact 3, and pass A undercounts it.* Pass A claims "Nine ordered comparisons"
at `report-04:544-551`. **There are thirteen.** Pass A's nine are exact —
`spirv_ir_builder.cpp:556, 625, 671, 679, 741, 777`,
`vulkan_device_creator.cpp:577`, `spirv_codegen.cpp:1908`,
`spirv_ir_builder.h:424` — but it misses the whole block at
`spirv_codegen.cpp:2666-2679`, where `spirv_version` is read into a local at
`:2666` and four `>=` comparisons at `:2669, 2671, 2673, 2675` select
`spv_target_env` between `SPV_ENV_VULKAN_1_3`, `_1_2`,
`_1_1_SPIRV_1_4`, `_1_1` and `_1_0` for the optimizer built at `:2682`. That is
the single most consequential ordered use of the value in the tree, and it is
the one pass A does not have. Pass B cites `:2673` (`report-04b:493-494`) and
does have it.

Pass A's *substance* — "several deciding which constructs to emit" — is right.
I read two: `spirv_ir_builder.cpp:554-562` picks `StorageClassUniform` versus
`StorageClassStorageBuffer`, and `spirv_ir_builder.h:424-428` decides whether
global values join the `OpEntryPoint` interface list.

**The contract half, undecided and correctly escalated.** Whether `!=` becomes
`>=`, and whether one relation can serve both a packed version number and
booleans stored as levels, is a project-owner decision. Pass A escalation 3 and
pass B escalation 5 both state it that way. Correct.

**Pass B's supporting tally is right in conclusion and wrong in arithmetic.**
See §5.2.

One fact neither report states, which cuts in favour of `>=` for the AOT case
specifically: the SPIR-V in an AOT module is generated at build time against the
builder's declared caps, and `spirv_codegen.cpp:2666` reads `params.caps` — the
device's caps — only at generation time. At load time a device with a *higher*
`spirv_version` consumes already-generated lower-version SPIR-V forward
compatibly; a lower one does not. Recording, not proposing.

### 3.2 `spirv_has_physical_storage_buffer` — verified in full, and it changes the shape of 6.2 for one spine only

**Verified exactly as both reports now state it.** Grep across `taichi/` and
`c_api/`:

- **Two producers, both compiled out.** `vulkan_device_creator.cpp:826`, inside
  `#if !defined(__APPLE__) && false` opened `:825` and closed `:827`, with the
  macOS note and issue link at `:822-823`, the author's note at `:824`, and the
  enclosing `if (device_supported_features.shaderInt64)` at `:821`.
  `c_api/src/taichi_vulkan_impl.cpp:48-49`, inside `/* */` spanning `:46-51`,
  preceded at `:45` by "(penguinliong) Will bring it back after devcap."
- **Twelve live consumers**, and I counted them individually:
  `vulkan_device.cpp:1772, 1792, 2147, 2509` (4);
  `spirv_ir_builder.cpp:73, 113` (2);
  `spirv_codegen.cpp:783, 2340, 2416, 2491` (4);
  `runtime/gfx/runtime.cpp:96` (1); `gfx_program.h:81` (1). Twelve.
- `gfx_program.h:79-83` read directly: it returns `"1" + (has_buffer_ptr ? "b" : "-")`
  and therefore always `"1-"`.

A thirteenth textual hit exists at `c_api/include/taichi/cpp/taichi.hpp:1156`, a
setter in the header-only C++ wrapper. Neither report counts it as a consumer,
correctly.

**Does it change the shape of item 6.2?** Partly, and the reports are right for
the spine it covers, but neither states the limit.

It is confined to the SPIR-V spine. `arch_uses_spirv` is
`{opengl, gles, vulkan, dx11, metal}` (`arch.cpp:63-66`). On that spine 6.2 is
substantially "finish and enable machinery that already exists" rather than
"build 64-bit addressing", and pass A's framing at `report-04:526-529` is
accurate.

But the brief's three target tiers in §5.2 are all NVIDIA cards on the
CUDA/LLVM path, and that path never consults `DeviceCapability` at all — I
confirmed no `set_caps` call exists under `taichi/rhi/cuda/`,
`taichi/rhi/cpu/`, `taichi/rhi/llvm/` or `taichi/rhi/amdgpu/`. So on the spine
the brief actually targets, this finding is inert and 6.2 remains a
build-from-scratch question. Pass B reaches the same boundary from the other
side at `report-04b:198-200` ("This is the central gap for brief section 5.2")
but does not connect the two. Neither report says in one place that 6.2 splits
into two differently-shaped halves along `arch_uses_spirv`. I record it as a
framing gap, not an error.

### 3.3 `CUDA_VERSION` covers only half — B's fact is right, B's conclusion is too strong

Pass B, `report-04b:703-715` and escalation 1's third bullet at `:1107-1109`:
"`CUDA_VERSION` proves only the 'choose between prebuilt modules' half. Nothing
in the tree yet compiles a configure-time value *into* the bitcode, which is
what the ceiling requires."

**The narrow fact is true.** I confirmed `runtime.cpp`'s includes at `:16-27`:
`taichi/inc/constants.h` at `:25`, `cuda_kernel_utils.inc.h` at `:26`,
`taichi/math/arithmetic.h` at `:27`, and no `version.h`. The only includers of
`taichi/common/version.h` are the three B names. `CUDA_VERSION` never enters
the bitcode.

**The general claim is false, and the sentence after it does not follow.** See
§4 — a configure-time value is already compiled into the bitcode, on the
command line B quotes at `report-04b:733-734`.

**And the framing is over-narrow against the brief.** Plan §5 says the loader
configures the binary "either by custom compilation or by activating the
correct prebuilt binary, whichever proves easier". B's "it has to be compiled
into the bitcode, not consulted beside it" (`:712-715`) forecloses the second
of those. The ceiling must be fixed at *some* bitcode compile, but N pre-sized
variants selected at run time by filename is exactly what
`get_runtime_fn` (`llvm_context.cpp:209-211`) and the install at
`runtime_module/CMakeLists.txt:13` already do per arch.

The one thing that does constrain it, which neither report states: the host
side must agree with whichever bitcode is loaded. `runtime.cpp` is **not** in
the host source list — `cmake/TaichiCore.cmake:77-91` globs
`"taichi/runtime/*.h" "taichi/runtime/*.cpp"` non-recursively, so
`taichi/runtime/llvm/runtime_module/runtime.cpp` is never compiled into
`taichi_core`. The host's only knowledge of the array bound is the constant it
was itself compiled against, guarding the write at `struct_llvm.cpp:266`. So a
run-time choice among pre-sized bitcode variants is only coherent if the host
binary is fixed at the same moment — which is what plan §5's install-time
configuration does. The distinction B draws is real for a single binary adapting
at run time and dissolves under the stated deployment model.

This should be an escalation, not a stated constraint. It currently reads as a
finding.

### 3.4 `taichi/rhi/CMakeLists.txt:17` — verified, both reports exact

Read directly:

```
if (TI_WITH_OPENGL OR TI_WITH_VULKAN AND NOT ANDROID)
```

CMake evaluates `NOT` before `AND` before `OR`, so this is
`TI_WITH_OPENGL OR (TI_WITH_VULKAN AND (NOT ANDROID))` and the `NOT ANDROID`
guard does not reach the OpenGL arm. The guarded body: GLFW cache forcings
`:18-20`, the Apple case `:22-24`, `message` `:26`,
`target_compile_definitions` `:28`, `add_subdirectory` `:29`,
`target_link_libraries` `:30`, `target_include_directories` `:31-34`, `endif()`
`:35`. Pass A `:233-243` and pass B `:984-997` both cite this exactly, and both
correctly mark the build outcome as reasoning rather than executed. Nothing to
correct.

### 3.5 No handshake on AOT load — verified independently, not accepted on agreement

I ran the greps rather than trusting the convergence.

- **Capability.** `get_required_caps` across `taichi/` and `c_api/` returns
  exactly two lines: the default body at `taichi/aot/module_loader.h:97-100`
  (a function-static empty `DeviceCapabilityConfig`) and the call site at
  `c_api/src/taichi_gfx_impl.cpp:21`. No override. The loop at
  `taichi_gfx_impl.cpp:22-29` therefore always iterates an empty map. Read
  directly.
- **Version.** A grep for `->version()` and `.version()` across `taichi/` and
  `c_api/` returns **zero lines**. The declaration is at `module_loader.h:85`.
  Nothing calls it.
- **Arch.** A grep for `->arch()` and `.arch()` across `taichi/` and `c_api/`
  returns ten lines, and **every one is on a `CompiledKernelData`, not on an
  `aot::Module`**: `program.cpp:193`, `compiled_kernel_data.cpp:140`,
  `llvm/compiled_kernel_data.cpp:46`, `spirv/compiled_kernel_data.cpp:28`,
  `cpu/kernel_launcher.cpp:72`, `llvm_aot_module_builder.cpp:73`,
  `llvm/kernel_launcher.cpp:12`, `amdgpu/kernel_launcher.cpp:154`,
  `cuda/kernel_launcher.cpp:191`, `kernel_compilation_manager.cpp:225`. The
  declaration at `module_loader.h:84` has no caller. Both reports' claim that
  the pattern exists one layer down and was not lifted is exactly right, and I
  read `kernel_compilation_manager.cpp:218-230` to confirm `loaded` there is a
  `CompiledKernelData` from `load_ckd`.
- **The module records no arch.** `taichi/runtime/gfx/aot_utils.h:23`
  `TI_IO_DEF(kernels, fields, required_caps, root_buffer_size)`, mirrored at
  `taichi/aot/module_data.h:135`. Confirmed.

**Both reports are right, on my own evidence.** This is the strongest shared
finding in the territory and it survived independent re-derivation.

---

## 4. What still survives both reports

### The bitcode command line already carries a CMake-driven `-D`, and neither report notices

Both reports quote `taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`
verbatim — pass A at `report-04:157-158`, pass B at `report-04b:733-734`:

```
${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
  -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR}
```

I grepped both reports and both notes files. `ARCH_${rtm_arch}` appears in
exactly three places across all four documents, and all three are inside that
quoted block. **Neither pass discusses it anywhere.**

It is a complete, working instance of the mechanism 6.1 needs.

| Step | Site |
|---|---|
| Configure-time values | `HOST_ARCH` at `cmake/TaichiCXXFlags.cmake:149`; `CUDA_ARCH` `CMakeLists.txt:155`, `AMDGPU_ARCH` `:159`, `DX12_ARCH` `:163` |
| Iterated into a parameter | `runtime_module/CMakeLists.txt:29`, `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)` |
| **Compiled into the bitcode** | `:8`, `-D "ARCH_${rtm_arch}"` |
| Consumed inside the bitcode | 19 sites in `runtime.cpp` (`:51, 119, 155, 347, 771, 798, 801, 860, 1144, 1156, 1167, 1293, 1343, 1431, 1549, 1633, 1651, 1861, 1865`) plus `locked_task.h:7` and `node_pointer.h:15` |
| One artefact per parameter value | `runtime_${rtm_arch}.bc`, `:8` |
| Installed | `:13` |
| Selected at run time by filename | `llvm_context.cpp:209-211`, `get_runtime_fn(Arch)` |

Consequences, both load-bearing:

1. **It falsifies pass B's escalation 1, third bullet, verbatim.** "Nothing in
   the tree yet compiles a configure-time value *into* the bitcode" is false.
   That bullet is the surviving core of B's most consequential escalation.
2. **It narrows pass A's escalation 1.** `report-04:903-905` states "Only one
   route is sound: the source-tree header must stop being a header." Pass A
   reaches that by considering the `-I` list (`:172-178`) and the
   `CMAKE_CXX_FLAGS` route, and never the `-D` list on the same line. There is
   at least a second route, and it is the one already in use for a per-variant
   parameter.
3. **It is the closest precedent in the tree to items 6.1 and 6.3 together** —
   closer than `CUDA_VERSION`, which selects among vendor-supplied prebuilts,
   and closer than the AMDGPU ISA-version selection, which selects among
   vendor-supplied prebuilts by detected hardware. This one is a project-owned
   parameter that produces, installs and selects project-built modules.

One caveat I checked before offering it, and it belongs in the escalation
rather than as a finding: the existing form is `-D "ARCH_x64"`, a bare macro
name, whereas `constants.h:12` declares `constexpr int taichi_max_num_snodes = 1024;`.
A `-D taichi_max_num_snodes=4096` would macro-expand that declaration and fail
to compile, so reaching the constant this way needs a different macro name that
`constants.h` reads. That is a design decision, not mine.

I am recording the mechanism, not proposing a route. Both escalation 1s need
restating to include it.

---

## 5. Newly wrong, or freshly imprecise

None of these inverts a finding. All four are in the same family the method is
built to catch: a `[V]`-marked count that the stated grep does not produce.

### 5.1 Pass A: "Nine ordered comparisons" is thirteen

`report-04:544-551`, marked `[V]`. Covered in §3.1. The four missing are
`spirv_codegen.cpp:2669, 2671, 2673, 2675`, and the block they sit in
(`:2666-2679`) is the one that selects the SPIR-V target environment. Pass A's
adjudication survives; the count and the list do not.

### 5.2 Pass B: the capability-level tally does not match the tree

`report-04b:498-504`, marked `[V]`: "29 sites pass `true`, 10 pass `1`, and 13
pass a `spirv_version` value." That totals 52.

I ran the tally across `taichi/` and `c_api/src/`, including `.mm`, and count
**54** textual `caps.set(DeviceCapability::…)` sites: 30 passing `true` (21 in
`vulkan_device_creator.cpp`, 6 in `opengl_device.cpp`, 2 in
`taichi_opengl_impl.cpp`, 1 in `taichi_vulkan_impl.cpp`), 10 passing `1` (all
`metal_device.mm`), 14 passing a `spirv_version` value.

The two-site difference is most plausibly the two compiled-out
`spirv_has_physical_storage_buffer` producers, and `program.cpp:536`, which sets
a *build target* default rather than a device level. Any of those exclusions is
reasonable; B states none of them, in a revision whose stated remedy was to
state the scope actually searched.

**B's conclusion is unaffected and I confirm it independently:** every level
written anywhere in the tree today is either a boolean stored as `true`/`1` or
an ordered `spirv_version`, and `>=` mishandles neither kind.

### 5.3 Pass B: `spirv_has_atomic_int64` "four lines" is not what that grep returns

`report-04b:218-227`: "Grep over the tree excluding `external/` returns four
lines" and then lists `rhi_constants.inc.h:17`,
`c_api/include/taichi/taichi_core.h:388`, `cpp/taichi.hpp:1121-1122` and
`python/taichi/lang/enums.py:27`.

A grep for `spirv_has_atomic_int64` outside `external/`, `build/` and
`modernization/` returns **three** lines: `rhi_constants.inc.h:17`,
`cpp/taichi.hpp:1121`, `enums.py:27`. The fourth item is real but is a
different string — `taichi_core.h:388` reads
`TI_CAPABILITY_SPIRV_HAS_ATOMIC_INT64 = 7`, upper case. B's *list* is right and
its census is more complete than mine was in round one; its description of how
it was obtained is not.

### 5.4 Pass B: `TI_ARCH_METAL` case range off by one

`report-04b:67` gives "METAL 294-301". `c_api/src/taichi_core_impl.cpp:294` is
`#endif  // TI_WITH_LLVM`; the case runs `:295-301` under `#ifdef TI_WITH_METAL`
at `:295`. Pass A has `:295-301` and is right (`report-04:292`). Trivial.

---

## 6. What I tried to break and could not

Re-derived independently this round, all confirmed exact:

- `set_caps`: nine grep hits, one declaration at `public_device.h:855` and
  **eight** call sites, five RHI and three C API, matching both reports' tables
  row for row.
- The `spirv_has_physical_storage_buffer` census, §3.2.
- The three-handshake absence, §3.5.
- `taichi_max_num_snodes` at exactly three places (`constants.h:12`,
  `struct_llvm.cpp:266`, `runtime.cpp:567-569`) and `kMaxNumSnodeTreesLlvm` at
  two (`constants.h:13`, `runtime.cpp:562-563`). None in `taichi/rhi/`,
  `taichi/aot/`, `c_api/` or `cmake/`.
- The complete `CUDA_VERSION` chain: `TaichiCore.cmake:97-100` with the comment
  at `:97` ("This version var is only used to locate slim_libdevice.10.bc"),
  forced `"0.0"` at `CMakeLists.txt:149-152`, `version.h.in:5` (file is 5 lines),
  `configure_file` into `${CMAKE_SOURCE_DIR}` at `CMakeLists.txt:203-204`,
  `core.cpp:87-89`, `llvm_context.cpp:213-218`. And `.gitignore:63-64` listing
  `/taichi/common/version.h` and `/taichi/common/commit_hash.h`, which is pass
  B's point at `report-04b:695-701` and is correct.
- 25 `PER_DEVICE_CAPABILITY` entries at `rhi_constants.inc.h:10-34` against 25
  `TI_CAPABILITY_*` values 0-24 at `taichi_core.h:381-405`, with
  `spirv_has_int64` fifth (`:14`) and `TI_CAPABILITY_SPIRV_HAS_INT64 = 4`
  (`:385`). The positional derivation at `misc/taichi_json.py:115`.
- The header comment at `rhi_constants.inc.h:1-7` naming CUDA compute
  capability as something that "can be listed here". Pass B cites it in
  escalation 8; pass A does not have it. It is a good catch and it strengthens
  pass A's escalation 7 as well.
- `CompileConfig::fit()` at `compile_config.cpp:67-76`, containing only
  `:68-71`, `:72-74` and `:75`, exactly as pass A says, against the comment at
  `program.cpp:140`.
- The DX12 stub: `dx12_api.cpp` is 26 lines, `is_dx12_api_available()` at
  `:6-12` returning `true` on the define alone, `make_dx12_device()` at `:14-16`
  returning `nullptr`.
- The C API OpenGL capability override: `GLDevice::GLDevice`
  (`opengl_device.cpp:506-529`) with the `!is_gles()` guard at `:509-513` and
  the comment at `:510`, versus `OpenglRuntime::OpenglRuntime`
  (`taichi_opengl_impl.cpp:4-13`) setting `spirv_has_int64` unconditionally at
  `:9` and dropping the int16/float16 detection entirely. Pass A's scope caveat
  at `report-04:480-485` — that no `TI_ARCH_GLES` case exists in
  `ti_create_runtime` — is correct and correctly limits the claim.
- `ti_create_runtime` arch ranges and every `device_index != 0` rejection at
  `:270, 277, 283, 289, 297`, with Vulkan alone honouring it at `:254-255`.
- 24 `option()` calls, all boolean; `HOST_ARCH` the one project `CACHE INTERNAL`
  non-boolean; `CACHE PATH` at `c_api/cmake/FindTaichi.cmake:83`.
- `taichi/aot/*` globbed unconditionally at `cmake/TaichiCore.cmake:86`
  (pass B `:533-536`), which is why `module_loader.cpp:3-4` can include the
  backend loader headers unguarded.

---

## 7. Verdicts, stated separately

### Are both reports now CORRECT?

**Yes.** Subject to the four items in §5, each of which is a count or a grep
description rather than a conclusion, and none of which changes a design
decision. Every `[V]`-marked general statement I falsified in round one has been
withdrawn and replaced. Both reports now carry explicit correction sections
(`report-04:998-1065`, `report-04b:1017-1044`) naming what they withdrew and
why, and both re-verified before changing. Pass A's five withdrawals and pass
B's three are each supported by source I re-checked.

Pass A's `[V]`/`[I]` discipline, which round one found to be the failure — five
generalisations one step past what was read, all marked verified — is repaired.
`report-04:1085-1107` now separates eight specific inferences, including three
that a careless reader would take as verified.

### Are both reports now COMPLETE?

**Pass A: nearly.** It absorbed everything from both adversaries plus nine new
findings. What it lacks: the `-D` mechanism in §4, the four `spirv_version`
comparisons in §5.1, a citation for its own dlopen claim, and the
`rhi_constants.inc.h:1-7` header comment that pass B has.

**Pass B: no, and it says so.** `report-04b:1079-1084` declines the `Extension`
system, `CompileConfig::fit()` and the DX12 stub. That is a defensible refusal
under the method, but it leaves pass B unable to stand alone.

**The pair: no.** One gap survives both, §4, and it sits directly under the
first escalation of each report. That is the whole of what is outstanding.

---

## 8. What remains for consensus

1. **Both reports must absorb §4.** Pass B must withdraw "Nothing in the tree
   yet compiles a configure-time value *into* the bitcode" (`report-04b:1108-1109`)
   and restate the sentence at `:712-715` that turns it into a requirement.
   Pass A must withdraw "Only one route is sound" (`report-04:903-905`). Both
   escalation 1s then become a genuine choice among routes rather than a single
   forced one.
2. **Pass A: correct the ordered-comparison count** from nine to thirteen and
   add `spirv_codegen.cpp:2666-2679` (§5.1). The adjudication in 4.4 stands.
3. **Pass B: state the exclusions behind the tally** at `:498-504` (§5.2), and
   fix the `spirv_has_atomic_int64` grep description at `:218-227` (§5.3) and
   the METAL range at `:67` (§5.4).
4. **Pass A: cite the dlopen claim** at `:26` or drop the `[V]`.
5. **Optional, and I would take it:** neither report states in one place that
   6.2 divides along `arch_uses_spirv` (`arch.cpp:63-66`) into a
   finish-what-exists half and a build-from-nothing half, and that the brief's
   §5.2 tiers all sit on the second (§3.2).

None of this is a re-run. Items 2 through 4 are edits. Item 1 is one
verification each report can do in five minutes against a line it already
quotes.

---

## 9. Escalations

Unresolved. Recorded, not decided. I am not carrying forward the escalations
already raised by pass A (15) and pass B (16); those stand, except that
A's escalation 1 and B's escalation 1 both rest on a premise §4 corrects.

1. **How the SNode ceiling reaches the bitcode, given that a third route
   exists.** `runtime_module/CMakeLists.txt:8` already carries
   `-D "ARCH_${rtm_arch}"`, a configure-time parameter compiled into the
   bitcode and producing one installed variant per value. Whether 6.1 uses a
   generated `constants.h`, a `-D` on that line, or both, is now a choice
   among three routes rather than the single forced route both reports state.
   The `-D` route needs a macro name `constants.h` reads, because `-D` on the
   `constexpr` name itself would not compile (§4).

2. **Whether 6.1's answer is one configured binary or N prebuilt variants.**
   Plan §5 permits either. The `-D`/`get_runtime_fn` pair is a working
   instance of the second. The constraint on it, which neither report states,
   is that the host `taichi_core` must be fixed at the same moment: `runtime.cpp`
   is not in the host source list (`cmake/TaichiCore.cmake:77-91` globs
   `taichi/runtime/*.cpp` non-recursively), so the host's only knowledge of the
   array bound is the constant it was compiled against, guarding
   `struct_llvm.cpp:266`. Under install-time configuration that is satisfied.
   Under run-time adaptation it is not. Which model applies here is the
   project owner's call.

3. **Whether item 6.2 is one work item or two.** On the SPIR-V spine
   (`arch_uses_spirv` = opengl, gles, vulkan, dx11, metal) a large part of the
   machinery exists and is switched off at the capability layer (§3.2). On the
   LLVM/CUDA spine, where the brief's §5.2 tiers live, none of it applies
   because that path never populates `DeviceCapability` at all. Sizing 6.2 as
   one item conflates two very different amounts of work.

4. **Whether changing the capability-ordering contract means editing
   documentation.** `c_api/docs/taichi/taichi_core.h.md:281` is the source of
   the "not guaranteed" sentence; `c_api/include/taichi/taichi_core.h:411-412`
   is generated from it via `misc/taichi_json.py:260-266` and
   `misc/generate_c_api.py:54`. Changing `!=` to `>=` without changing the
   `.md` leaves the shipped documentation contradicting the shipped behaviour.

5. **Everything in my round-one §7 that is still open** — the scope of
   `Extension` against brief 1.2, what an installed Taichi tells its loader,
   `TI_WITH_CPU`, the offline-cache seam, and the `!=` relation. All five are
   now carried in both reports' escalations. Nothing to add.

---

## 10. Divergence from adversary2-04-1

`adversary2-04-1.md` did not exist when I finished. Checked by direct `ls` at
the point of writing; the directory contained no round-two adversary file but
this one. There was nothing to compare against, so no divergence section.
