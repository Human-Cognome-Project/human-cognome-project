# Adversary 2 · 02-1 — round two judgement on report-02-codegen.md and report-02b-codegen.md

Adversary 1 of 2, territory 02 (codegen), round two. The same adversary that
wrote `adversary-02-1.md`.

Method: every claim below was checked by opening the named file at the named
line in `/opt/project/taichi`. Where I correct a report or myself I give the
line I read. Where I found the reports right I say so and move on; where I
found them wrong I say what the source says instead.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Are both reports CORRECT? | **Report A (`report-02-codegen.md`): yes.** Every substantive claim I tested held. Two citation faults and one loose word remain, listed in §4. **Report B (`report-02b-codegen.md`): not fully.** One conclusion in §5.3 is contradicted by a fact B itself records two sentences earlier, and six citation faults survived its own correction pass. |
| Are both reports COMPLETE? | **No, for either.** Both carry the same wrong attribution of two of the three live translator call sites (§3.1). Both miss a second widening-invalidity site inside a function they both analyse (§3.2). B omits three findings A carries. |
| Is GENERAL CONSENSUS reached? | **Not yet.** The gap is narrow and mechanical. §6 scopes exactly what remains. |

**On my own round one objections:** eleven of my twelve were met on substance.
The twelfth, my correction of `bitmasked_activation`'s line numbers, was wrong;
both reports adjudicated against me and both are right (§1.3). I also handed
both agents the figure "six optional widths". That figure has no support in the
source. Both returned five and both flagged the discrepancy rather than adopting
it. That is the correct behaviour and it settles the count (§2.2).

---

## 1. Round one objections — addressed, or merely acknowledged

### 1.1 My objections (`adversary-02-1.md`)

| # | Objection | Status | Evidence |
|---|---|---|---|
| 1 | B's "checked at every use" is false | **Addressed.** Not acknowledged — withdrawn and rewritten from the source. B §0.1 and §4 re-derive the whole path; B's notes §19 records "I was wrong." | `report-02b` §4.3; `notes-02b` §19-§22 |
| 2 | A's "Metal sets Int64 unconditionally" is factually wrong | **Addressed.** A §3.3.7 now shows the gate. Verified: `metal_device.mm:1051` `if (feature_64_bit_integer_math)`, `:1038` `= family_apple3`, `:1035-1036` `supportsFamily:kMTLGPUFamilyApple3`, constant `:1025`. | both reports |
| 3 | The bypass covers eight accessors, not two | **Addressed by both**, with identical eight-row tables. Verified line by line (§2.2). | A §3.3.3; B §4.3 |
| 4 | The Int64-lacking target set is understated | **Addressed.** Both now carry Vulkan, OpenGL/GLES, Metal, DX11 and the C API OpenGL override. A additionally found imported Vulkan; **B does not carry it** (§3.4). | A §3.3.7; B §4.5 |
| 5 | The bound violation is a STORE over a range, not a lookup | **Addressed by both**, at `runtime.cpp:1003-1007`. | A §2.1; B §5.2 |
| 6 | Tree ids recycle, SNode ids do not | **Addressed by both.** Verified: `program.cpp:235` push, `:559-567` pop, `snode.cpp:220` `id = counter++`, no decrement anywhere. | A §2.1; B §5.3 |
| 7 | `GlobalTemporaryStmt` i32/i64 divergence has no live consequence | **Addressed by both**, both now citing the assertion at `offload.cpp:358` inside `allocate_global` (`:344`) against `taichi_global_tmp_buffer_size` (`constants.h:15`). Verified. | A §4.2.8; B §3.2 |
| 8 | `struct_llvm.cpp:174` produces `urem x, 0`, not a wrapped index | **Addressed by both.** Verified `CreateUDiv(CreateURem(l, prev), next)` at `:179` with `prev` from `:174-175`. One imprecision survives in both (§4.3). | A §4.1.2; B §3.1 |
| 9 | Nothing validates the emitted module | **Addressed in A, only half-addressed in B.** A §3.3.5 and §4.2.10 name `set_run_validator(false)` at `spirv_codegen.cpp:2710` and the `if constexpr (false)` at `:2758`. B §4.4 still says only that "the registered passes are optimisation, not validation" and never states the validator is explicitly off. B's reader would conclude the backstop is optional; it is absent. | §2.1 below |
| 10 | (My own correction of `bitmasked_activation` lines) | **Correctly rejected by both.** I was wrong; see §1.3. | A §3.4.3.1; B §7.1 |
| 11 | Citation slips I listed | **Addressed.** All nine I named are fixed in the reports that carried them. Verified individually. | §4 |

### 1.2 Adversary 02-2's objections

| # | Objection | Status |
|---|---|---|
| 1 | `Extension::data64` is declared, granted, never read | **Addressed by both.** Verified: declaration `taichi/inc/extensions.inc.h:6`; grants `taichi/program/extension.cpp:12`, `:16`, `:20`; every SPIR-V arch given `{}` or `extfunc` at `:23-27`; no call site passes it. Both reports also correct 02-2's supporting claim that `is_extension_supported` is called once — it is called at nine C++ sites. I re-grepped: `program.cpp:149`, `codegen_llvm.cpp:2726`, `compile_to_offloads.cpp:92,205,218,236,245,288`, `export_lang.cpp:1225`. Both reports are right and 02-2 was wrong. |
| 2 | `use_64bit_pointers` couples to two capabilities; `at_buffer` keys on `stype.dt` | **Addressed by both.** Verified `at_buffer` at `spirv_codegen.cpp:2194-2220`, branch on `ptr_val.stype.dt == PrimitiveType::u64` at `:2197`, PSB branch `:2198-2204`; `declare_primitive_type` sets `t.dt = dt` at `spirv_ir_builder.cpp:1536`; `OpCapability PhysicalStorageBufferAddresses` only at `:73-77`, addressing model only at `:113-122`. |
| 3 | The refine-coordinates ABI is the LLVM-side narrowing pair | **Addressed by both.** |
| 4 | `LinearizeStmt` strides are produced at `int` outside codegen | **Addressed by both**, at `scalar_pointer_lowerer.cpp:33` and `demote_dense_struct_fors.cpp:19`. |
| 5 | `all_dense` and `is_gc_able` gate the overflow | **Addressed in A, mishandled in B.** See §2.6 — this is the one place a round-two correction made a report internally inconsistent. |
| 6 | `TI_ASSERT` is unconditional | **Addressed by both.** Verified `logging.h:100-107`, no `NDEBUG` guard. |
| 7 | B's "only piece of SNode geometry 64-bit on both sides" is wrong | **Addressed.** B §3.1 withdraws it explicitly against `node_dense.h:10-12` and `runtime.cpp:318`. |
| 8 | B's nine citation faults | **Addressed.** I re-opened all nine. `spirv_ir_builder.cpp:311-314` (was `:310-313`) ✓; `vulkan_device_creator.cpp:821` (was `:823`) ✓; `runtime.cpp:310` (was `:307`) ✓; `codegen_llvm.cpp:2386` (was `:2385`) ✓; `constants.h:28` (was `:27`) ✓; `constants.h:13` for `kMaxNumSnodeTreesLlvm` (was `runtime.cpp:562-563`) ✓; `struct_llvm.cpp:153` (was `:152`) ✓; `llvm/kernel_compiler.cpp:32` (was `:31`) ✓; `spirv_ir_builder.cpp:591` (was the `:583-589` region) ✓. |

### 1.3 My own round one error, confirmed against me

I wrote that pass B's `bitmasked_activation` citation `:405, 411-412, 414`
should be `:404, :410, :411, :413`. **That correction was wrong.** Reading
`spirv_codegen.cpp:383-435`, the literal `ir_->u32_type()` calls before the op
branches are at `:405`, `:411`, `:412`, `:414`; `:404` is
`ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,` and
`:413` is a bare `bitmask_word_ptr =`. I cited statement starts while correcting
a citation of the calls. Both reports adjudicated this against me
(A §3.4.3.1, B §7.1) and both are right. Recorded so the planner does not carry
my amendment forward.

My round one placement of `node_allocators[snode_id]` at `runtime.cpp:1029` and
`ambient_elements[snode_id]` at `:1038` was right, and both reports adjudicated
for me over 02-2's `:1291-1296`/`:1298-1304`. Verified by exhaustive grep: the
only subscripts of those two arrays are `:1029`, `:1038` (writes) and `:1692`,
`:1723`, `:1740`, `:1784` (reads).

---

## 2. The six new claims, attacked

### 2.1 Nothing validates the generated SPIR-V module — **TRUE, and stronger than stated**

Verified at source:

- `spirv_opt_options_.set_run_validator(false);` at
  `taichi/codegen/spirv/spirv_codegen.cpp:2710`. Unconditional. It sits outside
  the `if (params.enable_spv_opt)` block, which closes at `:2709`.
- `spirv_opt_->Run(...)` at `:2746-2747` executes on every kernel regardless of
  `enable_spv_opt`; only the pass registrations at `:2685-2708` are gated.
- `spirv_tools_` is constructed at `:2712` and its only use is
  `Disassemble` at `:2763`, inside `if constexpr (false)` at `:2758`.

I then tested the wider claim rather than the narrow one. A tree-wide grep for
`Validate`, `spvValidate`, `set_run_validator` and `SpirvTools` over `taichi/`
and `c_api/` returns exactly four live hits, all in `spirv_codegen`
(`spirv_codegen.h:39`, `spirv_codegen.cpp:2710`, `:2712`) plus unrelated
comments in `frontend_ir.cpp:1044`, `dx12_api.cpp:20` and `ggui/renderable.cpp`.
**There is no SPIR-V validation anywhere in the tree**, not merely none in
codegen. Report A's §3.3.5 and §4.2.10 are correct and, if anything,
understated. Report B never makes the claim at all (§3.3 below).

**Weighing it against the type-translator bypass, as asked.** The two are not
alternatives; they compound, and the correct severity statement is narrower than
either report gives:

1. The bypass at `spirv_types.cpp:397-428` writes `vt.id` into
   `ir_node_2_spv_value` at `:418` and `:430` with no test. `SType::id` is
   `uint32_t id{0}` (`spirv_ir_builder.h:51`) and `declare_primitive_type`
   assigns `t.id = id_counter_++` at `spirv_ir_builder.cpp:1535`, so an SType
   whose declaration was skipped genuinely carries id 0.
2. That zero reaches `OpTypeStruct` through `visit_struct_type`'s operand list
   at `spirv_types.cpp:442-444`, and `OpMemberDecorate` at `:450-452`.
3. Nothing in Taichi looks at it. So there is **no Taichi-level diagnostic**.
4. But a 0 `<id>` operand is a hard parse failure for any conformant SPIR-V
   consumer. The failure is therefore **deferred to the driver at shader-module
   creation**, not absent. What the driver does is not determinable from this
   tree.

Neither report states point 4. Both write as though the module simply escapes.
The distinction changes what 6.2 must build: a **diagnostic** naming the type
and the capability, at a point where the type is still known, rather than a
correctness fix for a silent miscompile. Report A's heading "nothing downstream
catches it" (§3.3.5) is accurate scoped to the codegen and reads broader than it
is. This is the single place I would ask both reports to add a sentence.

### 2.2 Eight accessors, five capability gates — **number settled: FIVE, and "widths" is the wrong word**

Verified call site by call site at `taichi/codegen/spirv/spirv_types.cpp`:

| line | call | member |
|---|---|---|
| `:397` | `i8_type()` | `t_int8_` |
| `:399` | `i16_type()` | `t_int16_` |
| `:403` | `i64_type()` | `t_int64_` |
| `:409` | `u8_type()` | `t_uint8_` |
| `:411` | `u16_type()` | `t_uint16_` |
| `:415` | `u64_type()` | `t_uint64_` |
| `:424` | `f16_type()` | `t_fp16_` |
| `:428` | `f64_type()` | `t_fp64_` |

Eight, exactly as both reports say. The accessor declarations are
`spirv_ir_builder.h:529-531`, `:532-534`, `:535-537`, `:549-551`, `:552-554`,
`:555-557`, `:559-561`, `:562-564` — each a one-line `return t_*;`, no `caps_`.
Both reports cite all eight correctly.

The gates are five. `init_pre_defs` (`spirv_ir_builder.cpp:149-176`) declares
under `spirv_has_int8` at `:156-159`, `spirv_has_int16` at `:160-163`,
`spirv_has_int64` at `:166-169`, `spirv_has_float16` at `:171-173`,
`spirv_has_float64` at `:174-176`. The capability enumeration at
`taichi/inc/rhi_constants.inc.h:12-16` lists exactly those five as the optional
scalar-type capabilities. **Six has no support in the source and my round one
figure was an arithmetic slip.** Both agents were right to return five and right
to flag it rather than adopt it.

**But "five distinct optional bit widths" is not right either.** The distinct
bit widths are three: 8, 16 and 64. Five is the count of optional *types*
(int8, int16, int64, fp16, fp64). Report B says "eight optional types, five
capability flags" (§4.3) and is precise. Report A says "the number of distinct
optional bit-widths is also five: int8, int16, int64, fp16, fp64" (§3.3.3) and
then lists types, not widths. **B is right on the wording, A is loose.** Minor,
but the planner asked for the number settled, and a reader counting widths from
A's sentence would get the wrong shape of the problem.

### 2.3 `ti_set_runtime_capabilities_ext` — **TRUE, verbatim**

`c_api/src/taichi_core_impl.cpp:317-334`. Builds a fresh
`DeviceCapabilityConfig devcaps` at `:325`, fills it from the caller's
`TiCapabilityLevelInfo` array at `:326-330` with no device query of any kind,
and installs it with `runtime2->get().set_caps(std::move(devcaps));` at `:331`.
`Device::set_caps` is `taichi/rhi/public_device.h:855-857`, body
`caps_ = std::move(caps);` — whole-object replacement, not a merge. Any
capability, `spirv_has_int64` included, can be asserted from outside the library.

**Only report A carries this.** A §3.3.7 records it as fact and A escalation 6
raises it. A grep of `report-02b-codegen.md` for `set_runtime_capabilities`
returns nothing. B's escalation 12 covers only the OpenGL constructor override.
This is a completeness gap in B, and it is the more general of the two facts:
the OpenGL override is one hardcoded instance of what this entry point offers to
every caller.

### 2.4 `SNode::reset_counter()` is never called — **TRUE, and B has not taken it**

`taichi/ir/snode.h:348-350`:

```
  static void reset_counter() {
    counter = 0;
  }
```

Tree-wide grep for `reset_counter` over `taichi/`, `c_api/`, `tests/` and
`python/` returns three hits: this definition, `taichi/ir/ir.h:509`
(`Stmt::reset_counter`, a different class), and the single call
`Stmt::reset_counter();` at `taichi/program/program.cpp:347`. **No call to
`SNode::reset_counter()` exists.** The only reset of the SNode id space is
`SNode::counter = 0;` at `taichi/program/program.cpp:144`, inside the `Program`
constructor, guarded by `TI_ASSERT_INFO(num_instances_ == 0, ...)` at `:141`.

**Report A states this exactly** (§2.1, and escalation 2 notes one of the two
mechanisms in the source is dead).

**Report B does not.** B §5.1 reads: "It is reset only by `SNode::reset_counter()`
(`taichi/ir/snode.h:348-350`) and in `Program`'s constructor
(`taichi/program/program.cpp:144`)." As written that presents two live reset
avenues where there is one. B's escalation list does not correct it. This is a
substantive divergence between the two reports on a fact that bears directly on
escalation 2 in both — whether the counter should be reset per tree turns on
whether a reset mechanism already exists and is simply unused. **A is right.**

### 2.5 `spirv/kernel_utils.h` narrowing — **TRUE, both reports now carry it**

`taichi/codegen/spirv/kernel_utils.h:110-111`: `size_t begin{0}; size_t end{0};`
`:99`: `int advisory_total_num_threads{0};`
`taichi/codegen/spirv/spirv_codegen.cpp:1999`:
`const int num_elems = range_for_attribs.end - range_for_attribs.begin;`
`:2004`: `task_attribs_.advisory_total_num_threads = num_elems;`

All four verified. A carries it as new row S17 and closes its own sweep gap in
§3.4.4; B carries it in the §3.2 table. Both correct.

**One qualifier neither supplies.** In the `const_range()` branch that reaches
`:1999`, the two `size_t` fields were assigned at `:1989-1992` from
`stmt->begin_value` / `stmt->end_value`, which are `int32`
(`taichi/ir/statements.h:1415-1416`). So the narrowing is bounded today by the
IR field width, exactly as `list_element_size` is bounded by
`taichi_listgen_max_element_size` and `GlobalTemporaryStmt` by
`taichi_global_tmp_buffer_size` — and both reports supplied that "safe today
because" qualifier for those two and not for this one. The inconsistency matters
for prioritisation only, but it is the reports' own standard.

### 2.6 The `all_dense` gate is narrower than round one said — **TRUE, and B's conclusion now contradicts B's own evidence**

`taichi/runtime/llvm/llvm_runtime_executor.cpp:402`:
`bool all_dense = config_.demote_dense_struct_fors;`
then narrowed by the loop at `:403-410`, which can only ever clear it.
`runtime_initialize_snodes` returns early on it at `runtime.cpp:1000-1002`.
`demote_dense_struct_fors` is a public field (`compile_config.h:28`), defaults
true (`compile_config.cpp:18`), and is forced true only for SPIR-V archs
(`compile_config.cpp:72-74`), which do not use this runtime.

So with `demote_dense_struct_fors` false, `all_dense` is false unconditionally
and **the range write at `runtime.cpp:1003-1007` fires for every tree, dense or
sparse.** The gate is defeated by a config field, not by tree shape.

**Report A has this right.** §2.1: "with it false the early return never fires
**even for a fully dense tree**."

**Report B records the fact and then contradicts it.** B §5.3 point 1 says
`all_dense` "is true only when every SNode in the tree is `dense`, `place` or
`root`. It is also **seeded** from `config_.demote_dense_struct_fors` at `:402`."
Those two sentences cannot both hold — the seeding is precisely what makes the
first one false. B then concludes: "Both surviving routes therefore require
**sparse** SNodes." That conclusion is wrong, and B's §7.6 shows why it happened:
B added the seeding as a citation correction to an adversary's line range without
propagating it into the conclusion the paragraph draws.

This is not cosmetic. B's conclusion ties the defect to the brief's §4.1 sparsity
requirement. The true statement is stronger and simpler: **one of the two routes
to the out-of-bounds store requires no sparsity at all**, only a config field set
to false. B must correct §5.3.

---

## 3. Newly wrong, or still missing

### 3.1 Both reports mis-attribute two of the three live translator call sites

Both say the three `ir_translate_to_spirv` calls are `spirv_codegen.cpp:2386`
(args), `:2464` (rets), `:2511` (argpack). **The last two are swapped.**

Function boundaries, read directly:

| function | opens | closes |
|---|---|---|
| `compile_args_struct` | `spirv_codegen.cpp:2326` | `:2400` |
| `compile_argpack_struct` | `:2402` | `:2481` |
| `compile_ret_struct` | `:2483` | (before `get_buffer_binds` at `:2529`) |

So `:2386` is the args struct (correct in both), **`:2464` is the argpack
struct**, and **`:2511` is the return struct**. `:2464` sits between
`argpack_struct_type.id = ir2spirv_map[struct_type];` at `:2465` and
`argpack_types_[arg_id] = argpack_struct_type;` at `:2477`. `:2511` sits above
`ret_struct_type_.id = ir2spirv_map[struct_type];` at `:2512`.

**Report B's version is self-refuting on its face.** B §4.4 writes
"`spirv_codegen.cpp:2464` — `compile_ret_struct` (`:2483-...`)" and
"`spirv_codegen.cpp:2511` — `compile_argpack_struct` (`:2402-...`)". A line at
2464 cannot be inside a function that opens at 2483, and a line at 2511 cannot be
inside one that ends at 2481. B supplied the spans that disprove its own
attribution and did not notice.

**Origin.** This came from round one and both round one adversaries — including
me — repeated it. `notes-02-codegen.md` §25 shows report A's correction pass
re-opening `ir_translate_to_spirv` to fix its span (`:476-483`, correcting both
adversaries) while carrying the args/rets/argpack labels forward unchecked. So
the repair pass verified the callee and not the callers.

**Consequence.** Small but real. The set of three affected structs is right; only
the pairing is wrong. It matters to escalation 5/10 in the two reports, which
propose "check at the `translate_ti_type` call sites" — those five sites
(`:2344`, `:2358`, `:2420`, `:2436`, `:2494`, all confirmed by grep) partition
across the three functions the other way round, so anyone acting on the reports
would look in the wrong function first.

### 3.2 Both miss a second widening-invalidity site inside `bitmasked_activation`

Both reports identify `spirv_codegen.cpp:410-412` as the site where a widened
`make_pointer` produces an invalid `OpShiftRightLogical` — Result Type hardcoded
`u32_type()`, Base `bitmask_word_ptr` from `:409` carrying the pointer width.
Verified, and correct.

**There is a second one, twelve lines earlier, and neither report has it:**

```
398    auto bitmask_mask = ir_->make_value(spv::OpShiftLeftLogical, ptr_dt,
399                                        ir_->const_i32_one_, bitmask_bit_index);
```

`make_value(op, out_type, args...)` (`spirv_ir_builder.h:291`) takes the second
argument as Result Type and the third as the instruction's first operand, so
here Result Type is `ptr_dt` and Base is `const_i32_one_`, which is
`int_immediate_number(t_int32_, 1)` (`spirv_ir_builder.cpp:224`) — a 32-bit
value, permanently. SPIR-V requires the bit width of `OpShiftLeftLogical`'s
Result Type to match that of Base. Today `ptr_dt` is u32 and the widths match. If
`make_pointer` widened, Result Type would be 64-bit over a 32-bit Base:
**invalid, by the same rule and in the same function**, in the direction opposite
to `:410-412`.

This matters because both reports frame `bitmasked_activation` as "hardcodes u32
where the pointer widens". Half of it is the reverse: it hardcodes i32 operands
under a result type that follows the pointer. A fix that only replaces
`u32_type()` with the pointer type leaves `:398-399` broken.

### 3.3 Report B never states that the validator is disabled

B §4.4's failure-mode paragraph ends: "The only backstop is the optional
`spvtools` pass pipeline, and `enable_spv_opt` is overwritten from
`compile_config.external_optimization_level > 0` at
`taichi/codegen/spirv/kernel_compiler.cpp:38`; the registered passes are
optimisation, not validation."

That is true and it is not the fact. The fact is
`spirv_opt_options_.set_run_validator(false);` at `spirv_codegen.cpp:2710`,
unconditional and outside the `enable_spv_opt` block. A reader of B concludes the
backstop is conditional; it is absent. A has it (§3.3.5, §4.2.10). **B should
take A's finding.**

### 3.4 Three findings A carries that B does not

Recorded because the planner reads both and the union is what matters.

1. **`ti_set_runtime_capabilities_ext`** (`c_api/src/taichi_core_impl.cpp:317-334`).
   §2.3 above. In A only.
2. **Imported Vulkan sets no integer capability.**
   `c_api/src/taichi_vulkan_impl.cpp`, `DeviceCapabilityConfig caps{}` at `:35`,
   only `spirv_version` at `:37-43`, `set_caps` at `:53`, physical-storage-buffer
   set commented out at `:45-51`. Verified. In A's §3.3.7 table and escalation 7;
   absent from B's §4.5 table, which lists four setters plus DX11.
3. **`SNode::reset_counter()` is dead.** §2.4 above. In A only.

B carries one A does not, and it is load-bearing: the middle link in the "no
earlier check" argument. B §4.4 establishes that `get_buffer_value` does call
the guarded `get_primitive_type(dt)` at `spirv_codegen.cpp:2267`, but is called
with a hardcoded `PrimitiveType::i32` placeholder — verified at `:618`, with the
other four at `:728`, `:753`, `:788`, `:2293`. So the guarded function runs, on
the wrong type, and returns cleanly. A closes escalation 4 on the
front-of-pipeline absence (`Extension::data64`) and the back-of-pipeline absence
(the validator) without addressing the middle. **A should take B's finding.**
Note B's `:618` is right and both round one adversaries, including me, said
`:620`; `:620` is `buffer_val = ir_->make_access_chain(`.

---

## 4. Citation audit — does the repair hold?

Report A's correction pass claimed twenty faults found in itself, eleven missed
by both round one adversaries. I re-opened all eleven of those and a sample of
roughly forty further citations across both reports.

### 4.1 A's eleven self-found faults — all eleven repairs hold

| Claim | Verified |
|---|---|
| L9 muls `:1939`, `:1942`, add `:1944` | ✓ `:1938` is a continuation, `:1941` a comment, `:1943` a brace |
| L12 store `:2190-2191`, loads `:2216`, `:2228`, `:2253`, increment `:2273` | ✓ all five |
| S15 `const_i32_zero_`/`one_` at `spirv_ir_builder.cpp:223-224` | ✓ `:282-286` is `get_null_type` |
| i64 guard `:311-313`, return `:314`; `:310` is `return t_int32_;` | ✓ |
| S6 `bitmasked_activation` `:383-435` | ✓ |
| `make_pointer(...)` at `:406-408`, call on `:408` | ✓ |
| S16 pointer branch `spirv_types.cpp:490-498` | ✓ 64-bit `:492-493`, 32-bit `:495-496` |
| L20 ABI comment `codegen_llvm.cpp:284-292` | ✓ `:294` is the `functions` vector |
| L6 comment `:1750-1754`, assertion `:1755` | ✓ |
| `check_func_call_signature` `TI_ERROR` at `:141-142` | ✓ `:139-140` is `TI_INFO` about contexts |
| PSB framing `:821` test, `:825` `#if`, `:826` setter, `:827` `#endif` | ✓ `:822-824` are comments |

A's exhaustive subscript list for the three runtime arrays (§2.1 point 3) also
holds exactly. I grepped `element_lists[`, `node_allocators[`, `ambient_elements[`
over `runtime.cpp`: declarations `:567-569`; `element_lists` written `:1005`,
`:1016`, read `:1271`, `:1287`, `:1288`, `:1334`, `:1336`, `:1429`;
`node_allocators` written `:1029`, read `:1692`, `:1723`, `:1740`, `:1784`;
`ambient_elements` written `:1038`. Nothing else. A's correction that `:1335` is
`int num_parent_elements = parent_list->size();` and not a subscript is right.

A's file-sweep arithmetic in §3.4.4 and §6 also holds: `find` returns 45 `.h`/`.cpp`
files under `taichi/codegen/` and 51 total (six `CMakeLists.txt`). A's 24 swept
files plus the 21 cited in its tables account for all 45 exactly.

### 4.2 Faults that survived in report B

Six, all found by opening the line. None changes a conclusion; together they mean
B still cannot be transcribed without reopening the file, which is the standard B
itself applied to the round one reports.

| B site | Claimed | Actual |
|---|---|---|
| §3.1 | `codegen_llvm.cpp:2272` is `create_increment(loop_index, block_dim)` | `:2273`. `:2272` is `builder->SetInsertPoint(body_tail_bb);` |
| §3.1 | struct-for loop test `ICMP_SLT` at `:2213-2216` | `:2214-2216`. `:2213` is a `SetInsertPoint`. B's own §6 gives `:2214`, so B contradicts itself |
| §3.2 | struct-for "index variable" among `:2138-2141, 2146, ...` | the alloca is `:2149`; `:2146` is `spirv::Label loop_body = ir_->new_label();`, no `u32_type()` on it |
| §3.2 | "i32 phi induction variable and a signed `lt`" at `:2095, 2097` | phi at `:2093`, registered `:2094`; `lt` at `:2096`. `:2095` is `set_incoming`, `:2097` is `OpLoopMerge` |
| §3.2 | serial `RangeForStmt` step at `:1791, 1821, 1823` | `:1791` ✓, `:1823` ✓, but `:1821` is `spirv::Value next_value;` and the fourth use at `:1825` is omitted |
| §5.1 | array subscripts include `:1334-1336` | `:1334` and `:1336` are subscripts; `:1335` is not. This is the exact fault report A flagged and corrected in itself; B did not take it |

Two further internal inconsistencies in B: `translate_ti_type` is given as
`spirv_types.cpp:484-497` in §3.2 and `:484-514` in §4.4 — the function is
`:484-514`; and `check_func_call_signature` is given as `:103-144` in §6 where
the function is `:103-145`.

One overstatement in B §4.4: "A tree-wide grep for `data64` returns only those
four sites plus a commented-out OpenGL grant at `extension.cpp:29-30`." A
tree-wide grep returns those plus `python/taichi/lang/misc.py:186` and roughly
fifty `tests/python/` sites. The conclusion is unaffected — the Python front end
is out of scope per plan §1.2 — but the statement as written is false, and it is
the same species of error B correctly pinned on adversary 02-2's
`is_extension_supported` count.

### 4.3 Faults that survived in report A

Two, plus one loose word.

1. **The `:2464`/`:2511` swap** (§3.1 above). Shared with B.
2. **`VulkanRuntimeImported::Inner::Inner` at
   `c_api/src/taichi_vulkan_impl.cpp:18-56`** (§3.3.7). The class is
   `VulkanRuntimeImported::Workaround` (`taichi_vulkan_impl.h:27`), the
   definition opens at `:19`, and `:18` is blank. The line numbers inside
   (`:35`, `:37-43`, `:45-51`, `:53`) are all correct; only the name and the span
   start are wrong. This claim is **new in the correction pass**, so it is a fault
   the pass introduced rather than failed to remove.
3. **"five distinct optional bit-widths"** (§3.3.3). Three distinct widths; five
   optional types. §2.2 above.

Separately, A's §3.4.1 closes by listing **eight further span faults it
identified and deliberately did not fix in the body** — S8 `at_buffer`
`:2215-2218` (actual `:2214-2218`), S10 `ExternalPtrStmt` `:772-777` (actual
`:773-777`), `make_pointer` `:2317-2325` (`:2317-2324`), SPIR-V `LinearizeStmt`
`:531-540` (`:532-541`), `get_constant(T)` `:727-750` (`:727-749`),
`get_constant(DataType,T)` `:696-717` (`:695-718`), LLVM `SNodeLookupStmt`
`:1794-1828` (`:1792-1829`), SPIR-V `SNodeLookupStmt` `:503-507`. I spot-checked
three and A's "actual" values are right in each case. The disclosure is honest,
but the effect is that A's tables still contain eight line numbers A knows to be
wrong, with the corrections in a different section. For a document meant to be
read as an inventory, that is a defect of form. B corrected the equivalent spans
in its own body.

### 4.4 One imprecision shared by both

Both describe `snode->extractors[i].acc_shape * snode->extractors[i].shape`
(`struct_llvm.cpp:174-175`) as a product that "wraps". Both operands are `int`
(`taichi/ir/snode.h:41`, `:45`, `:49`), so signed overflow there is undefined
behaviour in C++ itself, not a wrap — the host compiler that builds the struct
compiler is entitled to assume it cannot happen. A's `urem x, 0` consequence at
`:179` is a real second failure mode and is correctly described; the first one is
mislabelled in both. Small, and worth one word each, because "wraps" invites a
reader to reason about the wrapped value.

---

## 5. Verdicts, argued

### 5.1 Correct?

**Report A: yes.** I attacked its three headline facts and all three held: the
`use_64bit_pointers` constant at `spirv_codegen.cpp:82` and its two capability
couplings; the eight-accessor bypass and the disabled validator; the
per-tree-versus-global-id chain with the range store at `runtime.cpp:1003-1007`.
Its eleven self-found citation repairs all hold. Its exhaustive claims — the
runtime subscript list, the four `taichi_max_num_snodes` sites, the two
`taichi_max_num_indices` sites, the 45-file sweep — I re-derived by grep and
`find` and each is exact. What remains is two citation faults and one loose word
(§4.3), none of which changes a conclusion.

**Report B: not fully.** One conclusion is wrong on the source: §5.3's "both
surviving routes therefore require sparse SNodes", contradicted by the
`all_dense` seeding B records in the same paragraph (§2.6). One statement of fact
is misleading: §5.1's presentation of `SNode::reset_counter()` as a live reset
(§2.4). One is false as written though its conclusion survives: the "tree-wide
grep for `data64`" claim (§4.2). And six citation faults survived its correction
pass, one of which is the exact fault A flagged and B did not take (§4.2).
Everything else in B that I tested held, including its complete rewrite of §4,
its withdrawal of the `max_num_elements` claim, its `at_buffer` correction, and
all nine of the citations round one had pinned on it.

The asymmetry is worth naming, because it is the reverse of round one. In round
one B was the more accurate on line numbers and wrong on one load-bearing claim.
After the correction pass A is the more accurate on line numbers and B is the one
carrying a wrong conclusion. Both passes improved; A improved more.

### 5.2 Complete?

**No, for either**, but the gap is now small and enumerable.

Shared:
1. The `:2464`/`:2511` attribution swap (§3.1).
2. The second widening-invalidity site at `spirv_codegen.cpp:398-399` (§3.2).
3. Neither says the invalid module's failure is deferred to the driver rather
   than absent (§2.1).
4. Neither notes that the `kernel_utils.h` narrowing is bounded today by the
   `int32` IR fields at `statements.h:1415-1416`, though both supply the
   equivalent qualifier for the other two narrowings (§2.5).

Report B additionally lacks: the disabled validator (§3.3),
`ti_set_runtime_capabilities_ext` (§2.3), imported Vulkan (§3.4), and the dead
`SNode::reset_counter()` (§2.4).

Report A additionally lacks: the `get_buffer_value` i32 placeholder that is the
middle link in its own escalation-4 closure (§3.4).

Coverage of `taichi/codegen/` is now demonstrably complete on A's side — the
45-file accounting closes it, and S17 closes the one real hole round one left. I
found no file in the territory that neither report opened. The remaining
incompleteness is depth at three specific lines, not breadth.

### 5.3 Is general consensus reached?

**No.** Six items, all mechanical, none requiring rework of either report's
structure or method. §6 lists them.

I want to be plain that this is not a finding of continuing disagreement between
the two reports. On every question round one put to them they now agree, and
where they adjudicated between the two adversaries they adjudicated correctly
both times — including against me. The residue is repair, not dispute.

---

## 6. What remains, scoped

Six items. Each is a specific edit to a specific line.

**Both reports:**

1. Correct the translator call-site attribution: `spirv_codegen.cpp:2386` is the
   args struct (`compile_args_struct`, `:2326-2400`); **`:2464` is the argpack
   struct** (`compile_argpack_struct`, `:2402-2481`); **`:2511` is the return
   struct** (`compile_ret_struct`, opens `:2483`). B must also drop the function
   spans it currently pairs with them, which contradict its own attribution.
2. Add `spirv_codegen.cpp:398-399` to the `bitmasked_activation` analysis: Result
   Type `ptr_dt`, Base `const_i32_one_` (i32, `spirv_ir_builder.cpp:224`), so a
   widened pointer breaks this instruction in the opposite direction from
   `:410-412`.
3. State that the zero-id module's failure is deferred to the driver at
   shader-module creation, not absent — there is no Taichi-level diagnostic, and
   no SPIR-V validation anywhere in the tree (grep over `taichi/` and `c_api/`).

**Report A only:**

4. `c_api/src/taichi_vulkan_impl.cpp` — the class is
   `VulkanRuntimeImported::Workaround` (`taichi_vulkan_impl.h:27`), definition
   `:19-56`, not `Inner::Inner` at `:18-56`.
5. Take B's `get_buffer_value` finding into the escalation-4 closure: the guarded
   `get_primitive_type` at `spirv_codegen.cpp:2267` does run, but on the hardcoded
   `PrimitiveType::i32` placeholder at `:618` (also `:728`, `:753`, `:788`,
   `:2293`), so it never sees the real argument type. Without it, "no earlier
   check" rests on `Extension::data64` alone.

**Report B only:**

6. Four corrections, all in §5 and §4:
   - §5.3: withdraw "both surviving routes therefore require sparse SNodes". The
     `element_lists` route requires no sparsity when
     `config_.demote_dense_struct_fors` is false, because `all_dense` is seeded
     from it at `llvm_runtime_executor.cpp:402` and the loop at `:403-410` can
     only clear it.
   - §5.1: `SNode::reset_counter()` (`snode.h:348-350`) is never called anywhere;
     the only reset is `program.cpp:144`.
   - §4.4: add `spirv_opt_options_.set_run_validator(false)` at
     `spirv_codegen.cpp:2710`, unconditional, and the `if constexpr (false)` at
     `:2758`.
   - The six citation faults in §4.2 above, plus `:1334-1336` in §5.1 and the
     `data64` grep overstatement in §4.4.

Optionally, and I do not press it: report A would be easier to transcribe if the
eight span faults it discloses in §3.4.1 were fixed in the tables rather than
listed separately.

---

## 7. Escalations

Unchanged in substance from round one, restated only where the source moved.
Nothing here is a proposal.

1. **What the SNode ceiling should bound.** Both reports now establish the
   mismatch, the range store (`runtime.cpp:1003-1007`), the monotonic global id
   (`snode.cpp:220`), the absent reset (`snode.h:348-350`), and that one of the
   two gates is a config field rather than tree shape (§2.6). What 6.1 means —
   raise the array size, re-point the assertion, or both — is the planner's, and
   interacts with `kMaxNumSnodeTreesLlvm = 512` (`constants.h:13`) and
   territory 03.

2. **Whether the deferred-to-driver framing changes 6.2's shape.** §2.1. If the
   zero-id module is reliably rejected by conformant drivers, 6.2's task at
   `spirv_types.cpp:397-428` is to add a diagnostic that names the type and the
   capability. If it is not, it is a correctness fix. The source cannot settle
   which, because nothing in this tree consumes the module.

3. **Whether `ti_set_runtime_capabilities_ext` is intended to be unchecked.**
   `c_api/src/taichi_core_impl.cpp:317-334` with
   `set_caps` at `public_device.h:855-857`. It is the general case of which the
   OpenGL constructor override (`c_api/src/taichi_opengl_impl.cpp:9-12`) is one
   instance. Outside `taichi/` and outside this territory.

4. **Whether Direct3D 11 and imported Vulkan are in scope.**
   `taichi/rhi/dx/dx_device.cpp:563-565` and
   `c_api/src/taichi_vulkan_impl.cpp:35-53` each set only `spirv_version`. The
   plan's §5.2 tiers are stated in NVIDIA generations and, per §5.2's own text,
   that is incidental — so the tier table does not settle which SPIR-V consumers
   the portable path must express. Under §2.2 the answer cannot be "the ones our
   test cards use".

5. **`use_64bit_pointers` and `spirv_has_physical_storage_buffer`.** Both reports
   now establish the two couplings and the `&& false` at
   `vulkan_device_creator.cpp:825`. Whether that guard is current or stale is not
   determinable from the source.

6. **Signedness as a decision separate from width.** Every loop comparison in
   both paths is signed except the SPIR-V struct-for
   (`spirv_codegen.cpp:2157`, `OpULessThan`). Unsettled in both reports and in
   both adversary passes.

7. **`create_bit_ptr`'s `isIntegerTy(32)`** (`codegen_llvm.cpp:1755`). A
   deliberate 32-bit contract on bit offsets within a physical type. Both reports
   decline to decide whether it widens; so do I.

---

## 8. Divergence with adversary2-02-2

At the time of writing,
`/opt/project/taichi/modernization/investigation/adversary2-02-2.md` does not
exist. I checked the directory after completing §0-§7; it contains
`adversary2-01-1.md`, `adversary2-01-2.md`, `adversary2-03-1.md`,
`adversary2-03-2.md`, `adversary2-04-1.md` and `adversary2-04-2.md`, and no
territory 02 round two file besides this one. I did not wait for it. No
comparison section can be recorded.
