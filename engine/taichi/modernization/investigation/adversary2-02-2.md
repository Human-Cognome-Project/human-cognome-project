# Adversary2 02-2 — round two, territory 02, code generation

Second pass on `report-02-codegen.md` (pass A) and `report-02b-codegen.md`
(pass B), against `adversary-02-1.md` and `adversary-02-2.md` (round one) and
the correction entries in `notes-02-codegen.md` §§23-35 and
`notes-02b-codegen.md` §§18-31.

I am the author of `adversary-02-2.md`. Two of my round-one claims were
adjudicated against me by both reports. Both adjudications are correct and I
record them as such in §3.

Every claim below was checked by opening the named file at the named line in
this pass. Where I found something new I give the line I read.

---

## 0. Verdicts, stated first

| Question | Verdict |
|---|---|
| Is pass A CORRECT? | **Yes on every substantive claim, with two residual defects**: one internal contradiction it introduced in the correction pass (§5.1) and one fabricated identifier (§5.2). Neither changes a conclusion. |
| Is pass B CORRECT? | **Yes on every substantive claim.** But its stated method — "Every **[V]** in this revision was re-opened at the named line during this pass" (`report-02b-codegen.md:16-17`) — is **false**, and four citation faults survive, one of them a total misattribution (§4.2). |
| Is pass A COMPLETE? | **Not quite.** Two gaps, both narrow, both shared with B: §6.1 and §6.2. |
| Is pass B COMPLETE? | **No.** The same two gaps, plus five of its own (§6.3), including one file it never sweeps in the report and one whole class of finding pass A carries and B does not. |
| Is GENERAL CONSENSUS reached? | **Not yet, but it is close.** Both reports are now correct and mutually consistent on every load-bearing fact. What remains is scoped in §7 and amounts to four additions and six corrections, none of them structural. |

The one thing in this analysis that changes an answer the planner has been
given: **the claim that an invalid module goes out unexamined is half right and
its second half is now settled.** Nothing *validates* the module, as both
reports say. But the SPIRV-Tools binary parser rejects id 0 outright, and the
optimiser calls that parser whether or not the validator is on. The invalid
module is caught, reported as a **warning**, and then shipped anyway because
the failure flag is dead code. Detail in §2.1. Neither report established this,
and it is exactly the severity question the arbitration posed.

---

## 1. Round one, objection by objection: addressed or merely acknowledged

### 1.1 My own objections (`adversary-02-2.md`)

| # | Objection | Against | Outcome |
|---|---|---|---|
| §1 | B's "checked at every use" is false; the tinyir path is unguarded and live | B | **Addressed.** B §4 rewritten in full, conclusion withdrawn at `report-02b:541-543`. A rebuilt §3.3 around it. |
| §1.5 | A overstated three of its four "assumed" 64-bit reads | A | **Addressed.** A §3.3.6 now heads them "Weaker than §3.3.3 but recorded" and gives the guard chain for each. |
| §2.2 | Reachability, the range write, the two gates, `TI_ASSERT` unconditional | both | **Addressed in both.** A §2.1, B §5.2-5.3. Both now carry the range write, both carry `all_dense` and `is_gc_able`, both carry `logging.h:100-107`. |
| §3.1 | B right on `bitmasked_activation` | A | **Addressed.** A's S6 rewritten; A now identifies `:410-412` as the load-bearing hardcode, which is more than B stated in round one. |
| §3.2 | `DecorationNoSignedWrap`, B only | A | **Addressed.** A §4.2.9, `spirv_codegen.cpp:778-781`. Verified: the shift is `:773-777`, the decoration block `:778-781`. |
| §3.3 | PSB guard framing wrong in both | both | **Addressed in both.** `:821` test, `:825` `#if`, `:826` setter, `:827` `#endif`. Verified line by line. |
| §3.4 | `use_64bit_pointers` coupled to two capabilities, not one | both | **Addressed in both.** A §4.2.2, B §6. |
| §3.6 | B's "only piece 64-bit on both sides" is contradicted by `node_dense.h:10-12` | B | **Addressed.** B §3.1 "Withdrawn from this list", `report-02b:266-275`. |
| §3.7 | LLVM `KernelCompiler` takes `device_caps` and drops it | A | **Addressed.** A §2.3 marked CORRECTED, `llvm/kernel_compiler.cpp:32`. |
| §3.9 | `spirv_codegen.cpp:1999` host-side narrowing | A | **Addressed.** A added it as S17 and traced it back to `kernel_utils.h:99,110-111`. |
| §3.10 | `LoopIndexStmt` span `:543-563` | A | **Addressed.** A escalation 8. |
| §3.11 | `constants.h:28`, not `:27` | B | **Addressed.** |
| §4 | Seven citation slips in A, ten in B | both | **Addressed, unevenly.** See §4. |
| §5.1 | Nothing in the C++ core rejects a 64-bit kernel argument | both | **Addressed in both**, and both correctly corrected my supporting claim. See §3.3. |
| §5.2 | Flipping `use_64bit_pointers` emits an undeclared capability | both | **Addressed in both.** |
| §5.3 | The refine ABI is the only LLVM-side narrowing of a 64-bit quantity | both | **Addressed.** A §4.1.11, B §6. |
| §5.4 | The stride producers are outside codegen and are `int` | both | **Addressed.** A §4.1.2, B §2/6.2. |
| §5.5 | The `all_dense` gate | both | **Addressed, and refined against me.** See §2.6. |
| §5.6 | `TI_ASSERT` is unconditional | both | **Addressed in both.** |

Nothing of mine was merely acknowledged. Every objection produced a substantive
edit, and in two places (§5.1's supporting count, §5.5's seed) the reports
corrected *me* while accepting the objection. That is the right outcome.

### 1.2 Adversary 02-1's objections (`adversary-02-1.md`)

| # | Objection | Outcome |
|---|---|---|
| §1.4 | The bypass covers eight accessors "across six optional widths" | **Addressed, and corrected against 02-1.** Eight accessors confirmed; six is wrong. See §2.2. |
| §2.1 | The write is a range store at `runtime.cpp:1003-1006` | **Addressed in both.** |
| §2.2 | Tree ids recycle, SNode ids do not | **Addressed in both.** Verified: `program.cpp:235`, `:559-567` (pop at `:563-564`), `program.h:336`, `snode.cpp:12`, `:220`. |
| §3 row 2 | `bitmasked_activation` is at `:404,:410,:411,:413` | **Correctly rejected by both reports.** 02-1 was wrong. See §3.1. |
| §3 row 9 | A's "Metal unconditional" is factually wrong | **Addressed.** A §3.3.7. |
| §3 row 11 | B right with `:772-776` for the shift | **Superseded.** The shift statement is `:773-777`; both reports now say so. 02-1's endorsement of `:772-776` was itself wrong and neither report repeats it. |
| §4.1 | DX11, C API OpenGL, Metal gating | **Addressed in both**, and A went further (imported Vulkan, `ti_set_runtime_capabilities_ext`). |
| §4.2 | `taichi_global_tmp_buffer_size` makes the i32 offset safe | **Addressed in both**, with the assertion at `taichi/transforms/offload.cpp:358`. |
| §4.3 | `urem` by zero at `struct_llvm.cpp:179` | **Addressed.** A §4.1.2 records both failure modes and says which is commoner; B §3.1 row `:179` and escalation 8. |
| §4.4 | "the default configuration runs nothing" | **Addressed and strengthened** — A found the validator is explicitly disabled, not merely unregistered. But see §2.1: the strengthened claim is itself incomplete. |

---

## 2. The new claims, attacked

### 2.1 "Nothing validates the generated SPIR-V module" — TRUE as stated, and the severity question it raises is now settled AGAINST both reports

The literal claim holds, exactly.

- `spirv_opt_options_.set_run_validator(false);` at
  `taichi/codegen/spirv/spirv_codegen.cpp:2710`. It sits **outside** the
  `if (params.enable_spv_opt)` block, which opens at `:2683` and closes at
  `:2709`. Unconditional. Confirmed.
- `spirv_tools_` is constructed at `:2712` and its only use is `Disassemble` at
  `:2763`, inside `if constexpr (false)` at `:2758`. Compiled out. Confirmed.
- A tree-wide grep for `run_validator`, `->Validate` and `spvValidate` over
  `taichi/` and `c_api/` returns **exactly one hit**: `spirv_codegen.cpp:2710`.
  So there is no validation anywhere else in the tree either. Neither report
  ran that grep; both assert the negative from the codegen alone. The negative
  is nonetheless true.

**But "no validator" is not the same as "not caught", and the arbitration asked
for the severity. Here it is, and neither report has it.**

The submodule is checked out at `external/SPIRV-Tools`, so this is readable
rather than assumed:

1. `Optimizer::Run` at `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598`
   skips `tools.Validate` when `run_validator_` is false (`:590-594`) — and
   then unconditionally calls `BuildModule(...)` at `:596-597` and returns
   false if it yields null at `:598`.
2. `BuildModule` at `external/SPIRV-Tools/source/opt/build_module.cpp:56-75`
   runs `spvBinaryParse` at `:68-69` and returns null unless the parse
   succeeds (`:74`).
3. `spvBinaryParse` rejects id 0 in three places:
   `external/SPIRV-Tools/source/binary.cpp:450` ("Error: Type Id is 0"),
   `:456` ("Error: Result Id is 0"), and `:473` ("Id is 0") for
   `SPV_OPERAND_TYPE_ID` — which is what an `OpTypeStruct` member operand is.

So the id-0 module produced by `spirv_types.cpp:418` **fails to parse**, and
the diagnostic is routed to Taichi's own consumer, which was installed at
`spirv_codegen.cpp:2682`.

4. That consumer is `spriv_message_consumer` at `spirv_codegen.cpp:2641-2659`.
   Its first branch is `if (level <= SPV_MSG_FATAL)` at `:2646`, and
   `SPV_MSG_FATAL` is **0** in the enum at
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-92`, with
   `SPV_MSG_ERROR` at 2. So a parse **error** falls through to
   `else if (level <= SPV_MSG_WARNING)` at `:2649` and is emitted as
   **`TI_WARN`**, not `TI_ERROR`.
5. `Run` returning false then fires `TI_WARN_IF(..., "SPIRV optimization
   failed")` at `spirv_codegen.cpp:2745-2748` and sets `success = false` at
   `:2750`.
6. **`success` is dead.** Its only read is at `:2760`, inside the
   `if constexpr (false)` block. Line `:2775` pushes
   `std::move(optimized_spv)` onto `generated_spirv` regardless, and on a
   failed `Run` that vector still holds the unmodified copy made at `:2740`.

**Net.** An id-0 module is caught by the optimiser's parser, downgraded to two
warnings, and then shipped unchanged to the pipeline layer. That is a third
severity, distinct from both alternatives the arbitration named: not silent,
not fatal, but a warning that the code path then discards.

Weighed against the type-translator bypass: the bypass is what *produces* the
invalid module (§2.2), and the absent validator is why nothing stops it at the
point of production. The optimiser's parse is an accident of the pipeline, not
a designed check — it only fires because the optimiser happens to run, and
`spirv_opt_->Run` executes whether or not `enable_spv_opt` registered any
passes (`:2746-2747`). So the bypass remains the defect and the missing
validator remains the missing net; what changes is that the failure is
**observable in the log today** rather than wholly silent. Both reports say
"silently" (A escalation 5, `report-02:955`; B §4.4 "Failure mode",
`report-02b:467-473`). That word should come out of both.

### 2.2 "Eight bare accessor calls at `spirv_types.cpp:397, 399, 403, 409, 411, 415, 424, 428`, five capability gates, five widths" — CONFIRMED. The number is FIVE, and six has no reading in the source.

Printed `spirv_types.cpp:393-431` numbered. The eight bare calls are on exactly
those eight lines:

| line | call | member | declared under |
|---|---|---|---|
| `:397` | `i8_type()` | `t_int8_` | `spirv_has_int8`, `spirv_ir_builder.cpp:156-159` |
| `:399` | `i16_type()` | `t_int16_` | `spirv_has_int16`, `:160-163` |
| `:403` | `i64_type()` | `t_int64_` | `spirv_has_int64`, `:166-169` |
| `:409` | `u8_type()` | `t_uint8_` | `spirv_has_int8`, `:156-159` |
| `:411` | `u16_type()` | `t_uint16_` | `spirv_has_int16`, `:160-163` |
| `:415` | `u64_type()` | `t_uint64_` | `spirv_has_int64`, `:166-169` |
| `:424` | `f16_type()` | `t_fp16_` | `spirv_has_float16`, `:171-173` |
| `:428` | `f64_type()` | `t_fp64_` | `spirv_has_float64`, `:174-176` |

Accessor declarations, all one-line `return t_...;` with no `caps_`
consultation: `spirv_ir_builder.h:529-531` (i64), `:532-534` (u64),
`:535-537` (f64), `:549-551` (i16), `:552-554` (u16), `:555-557` (f16),
`:559-561` (i8), `:562-564` (u8). Every one opened.

**The count.** Distinct capability flags: `spirv_has_int8`, `spirv_has_int16`,
`spirv_has_int64`, `spirv_has_float16`, `spirv_has_float64` — **five**, and the
`OpCapability` emissions at `spirv_ir_builder.cpp:58-72` enumerate the same
five. Distinct optional types: int8, int16, int64, fp16, fp64 — **five**.
Distinct optional bit-widths taken as bare numbers: 8, 16, 64 — three.

There is no arithmetic on this source that yields six. Six is the count of the
*integer* accessors alone (i8, i16, i64, u8, u16, u64), which is a real number
about a different quantity, and I take it that is where the figure came from.
Both agents returning five independently is the correct answer, and both
reports were right to flag the discrepancy rather than adopt the handed number
(A §3.3.3 "On the count", `report-02:446-451`; B §4.3, `report-02b:409-414`).
That is the behaviour the standing instructions ask for.

**One thing neither report says about the count**, and it matters more than the
number: the mandatory widths reached by the same two visitors — `i32_type()` at
`:401`, `u32_type()` at `:413`, `bool_type()` at `:407`, `f32_type()` at `:426`
— are safe *only because* `t_int32_`, `t_uint32_`, `t_bool_` and `t_fp32_` are
declared unconditionally at `spirv_ir_builder.cpp:155`, `:164-165`, `:170`.
A says this (`report-02:441-443`); B does not. The safety is a property of the
declaration site, not of the visitor, so any future capability gate on i32
silently widens this defect from five to six. Worth A's phrasing surviving into
whatever is merged.

### 2.3 "`ti_set_runtime_capabilities_ext` lets any caller assert any capability" — CONFIRMED, and pass B does not have it at all

`c_api/src/taichi_core_impl.cpp:317-334`. Verified:

- signature `:317-320`, `TI_CAPI_ARGUMENT_NULL(runtime)` at `:322`;
- fresh `DeviceCapabilityConfig devcaps;` at `:325`;
- the loop that copies caller-supplied `(capability, level)` pairs straight in,
  `:326-330`, with `devcaps.set(...)` at `:328-329`;
- `runtime2->get().set_caps(std::move(devcaps));` at `:331`.

No device query, no validation of the enum value, no merge.

`Device::set_caps` is `inline void set_caps(DeviceCapabilityConfig &&caps) {
caps_ = std::move(caps); }` at `taichi/rhi/public_device.h:855-857` — a
move-assign of the whole object. Confirmed. So the call does not add
capabilities, it **replaces** the config, which means it can also silently
*remove* capabilities the device layer had detected.

Pass A records this as fact at `report-02:587-591` and escalates it
(escalation 6). Its citations are exact.

**Pass B does not mention `ti_set_runtime_capabilities_ext` anywhere.** A grep
of `report-02b-codegen.md` returns zero hits. B has the OpenGL C API override
and stops there. This is a real gap: the OpenGL override is one hardcoded
runtime, whereas this entry point is a documented public API
(`c_api/include/taichi/taichi_core.h:939-943`, with a C++ wrapper at
`c_api/include/taichi/cpp/taichi.hpp:1260`) that lets *any* embedder assert
`spirv_has_int64` on *any* backend. For a project whose deliverable is
hardware agnosticism, that is the more consequential of the two.

### 2.4 "`SNode::reset_counter()` is never called" — CONFIRMED, exactly

`taichi/ir/snode.h:348-350`:

```
  static void reset_counter() {
    counter = 0;
  }
```

Tree-wide grep for `reset_counter` over `taichi/`, `c_api/`, `tests/` and
`python/` returns three hits and no more:

- `taichi/ir/snode.h:348` — the definition itself;
- `taichi/ir/ir.h:509` — `Stmt::reset_counter()`, a different class;
- `taichi/program/program.cpp:347` — `Stmt::reset_counter();`, which calls the
  `ir.h` one.

So `SNode::reset_counter()` has no caller, and the only reset of
`SNode::counter` is `SNode::counter = 0;` at `taichi/program/program.cpp:144`,
inside the `Program` constructor, guarded by
`TI_ASSERT_INFO(num_instances_ == 0, "Only one instance at a time")` at `:141`.
All four lines opened. Pass A's claim (`report-02:179-184`) is exact, including
its identification of `program.cpp:347` as the `Stmt` one.

**This correctly closes my own round-one escalation 2**, which listed
`snode.h:348-350` alongside `program.cpp:144` as if both were live reset
avenues. One is dead. A says so; **B does not** — `report-02b:589-591` still
reads "It is reset only by `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`)
and in `Program`'s constructor", which presents a dead function as a live
mechanism. That sentence is not false in isolation but it is misleading in
exactly the direction that matters for 6.1: it suggests a per-tree reset already
exists in the source and merely needs calling in a different place. B should
carry A's finding.

### 2.5 "`spirv/kernel_utils.h` holds a `size_t`-to-`int` narrowing neither report had swept" — CONFIRMED, and both reports now have it

- `int advisory_total_num_threads{0};` at `taichi/codegen/spirv/kernel_utils.h:99`
  and `int advisory_num_threads_per_group{0};` at `:100`.
- `size_t begin{0};` at `:110` and `size_t end{0};` at `:111`, inside
  `struct RangeForAttributes` opening at `:104`.
- `const int num_elems = range_for_attribs.end - range_for_attribs.begin;` at
  `taichi/codegen/spirv/spirv_codegen.cpp:1999`, inside the
  `if (range_for_attribs.const_range())` branch at `:1998`.
- `task_attribs_.advisory_total_num_threads = num_elems;` at `:2004`.

All five lines opened. A carries it as S17 (`report-02:369`) and closes its own
sweep gap with it (§3.4.4). B carries it as two rows in §3.2
(`report-02b:304-305`). Both correct.

A's framing is the better one: it names the file as the sweep gap and states
that thirteen of the fourteen omitted files were headers for covered code and
one was not. That is a falsifiable coverage claim, and I tested it in §6.3.

### 2.6 "The two gates are narrower than round one said, because `all_dense` is initialised from a settable config field" — CONFIRMED, and this is A's finding, not B's

`taichi/runtime/llvm/llvm_runtime_executor.cpp:402`:

```
402:  bool all_dense = config_.demote_dense_struct_fors;
403:  for (size_t i = 0; i < snode_metas.size(); i++) {
404:    if (snode_metas[i].type != SNodeType::dense &&
405:        snode_metas[i].type != SNodeType::place &&
406:        snode_metas[i].type != SNodeType::root) {
407:      all_dense = false;
408:      break;
409:    }
410:  }
```

The loop can only clear the flag; it never sets it. So `all_dense` is
`demote_dense_struct_fors && every-node-is-dense/place/root`.

`demote_dense_struct_fors` is a plain `bool` field
(`taichi/program/compile_config.h:28`), defaulted true at
`taichi/program/compile_config.cpp:18`, forced true for SPIR-V archs only at
`:72-74` (which is irrelevant on this LLVM path), and exposed as a writable
binding at `taichi/python/export_lang.cpp:201-202`. Verified all four.

**Consequence, correctly drawn by A and not by B.** With
`demote_dense_struct_fors` false, `all_dense` is false regardless of tree shape,
so the early return at `runtime.cpp:1000-1002` never fires and the range write
at `:1003-1007` executes **even for a fully dense tree**. Pass A states this at
`report-02:200-204`. Pass B records the seed (`report-02b:646`, and its
adjudication §7.6) but its §5.3 item 1 still reads "and is true only when every
SNode in the tree is `dense`, `place` or `root`", which is a necessary
condition presented as the definition. B never draws the consequence.

This matters for the plan, not just for tidiness: A's version says the
out-of-bounds store is reachable on an **all-dense** tree under a one-flag
configuration change, which is a materially larger reachable set than "sparse
configurations only". Both reports still conclude "both surviving routes involve
sparse SNodes" (A `report-02:210-212`, B `report-02b:651-654`) — **A's own
finding contradicts A's own conclusion two paragraphs earlier.** See §5.3.

---

## 3. The two adjudications, checked independently

### 3.1 `bitmasked_activation` line numbers — both reports are RIGHT, adversary 02-1 was wrong, and my round-one call holds

I printed `spirv_codegen.cpp:383-435` numbered and extracted every line in that
range containing `u32_type()`:

```
405 411 412 414 417 421 422 427 428 430 431 433
```

Before the op branches (`:416` onward) the calls are on **`:405`, `:411`,
`:412`, `:414`** — pass B's original citation and pass A's corrected S6.
Lines `:404`, `:410` and `:413` contain no such call:

- `:404` is `ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,`
- `:410` is `bitmask_word_ptr = ir_->make_value(`
- `:413` is `bitmask_word_ptr =`

Adversary 02-1 cited statement-opening lines while correcting a report that had
cited the calls. **Both reports adjudicated this correctly** (A §3.4.3 item 1,
B §7.1), and I confirm it a second time from the printed source.

I also confirm the substance both reports now carry, which is stronger than
either adversary's round-one framing: at `:409` `bitmask_word_ptr =
ir_->add(parent_ptr, bitmask_word_ptr);` carries `ptr_dt`, and `:410-412` builds
an `OpShiftRightLogical` whose Result Type is hardcoded `ir_->u32_type()`. A
widened pointer makes that an invalid instruction, not a truncation.

One refinement neither report makes, and it should be made because it changes
which of the four lines is a defect: **`:405` is not a bug.** It is the *Shift*
operand of an `OpShiftLeftLogical` whose Result Type is `ptr_dt` (`:404`), and
SPIR-V does not require the Shift operand to match Base's width. So of the four
hardcodes, `:411`/`:412`/`:414` are the ones that break under widening and
`:405` is benign. Both reports list all four undifferentiated. A gets closest
by naming `:410-412` as "the load-bearing one" (`report-02:358`).

### 3.2 The two runtime helper functions — both reports are RIGHT and I was wrong

Printed `runtime.cpp:1019-1045`:

- `runtime_NodeAllocator_initialize` is `:1026-1031`, and
  `runtime->node_allocators[snode_id] =` is on **`:1029`**.
- `runtime_allocate_ambient` is `:1033-1040`, and
  `runtime->ambient_elements[snode_id] =` is on **`:1038`**.

My round-one citations `:1291-1296` and `:1298-1304` land inside
`element_listgen_root`, which opens at `:1282`; `:1291` is
`auto child_get_num_elements = child->get_num_elements;`. Adversary 02-1's
numbers are right and mine were wrong by 265 and 260 lines respectively.
Pass A's §3.4.3 item 2 states the offset as "roughly 265 lines"; that is exact
for the first pair. **The adjudication is correct and I accept it.**

Both callers verified while I was there:
`llvm_runtime_executor.cpp:460-462` (`runtime_NodeAllocator_initialize`) and
`:465-466` (`runtime_allocate_ambient`), both inside
`if (is_gc_able(snode_metas[i].type))` at `:447`, with `snode_id =
snode_metas[i].id` at `:448`.

### 3.3 A third adjudication both reports made against me, which I also accept

Both reports correct my round-one §5.1 statement that `is_extension_supported`
"is called exactly once in the whole tree". A grep confirms **eight** C++ call
sites plus one pybind registration:

`taichi/program/program.cpp:149` (`assertion`);
`taichi/codegen/llvm/codegen_llvm.cpp:2726` (`bls`);
`taichi/transforms/compile_to_offloads.cpp:92`, `:205`, `:218`, `:236`
(`mesh`), `:245`, `:288` (`quant`); and the binding at
`taichi/python/export_lang.cpp:1225`.

Pass A calls that "nine C++ sites" (`report-02:698`), which counts the pybind
line as a site; pass B states it precisely as eight calls plus a binding
(`report-02b:833-836`). **B's phrasing is the accurate one.** The conclusion my
error was supporting — that nothing anywhere passes `Extension::data64` — is
unaffected: a tree-wide grep for `data64` returns exactly the declaration
(`taichi/inc/extensions.inc.h:6`), three grants
(`taichi/program/extension.cpp:12`, `:16`, `:20`), one commented-out grant
(`:29-30`) and one Python name list (`python/taichi/lang/misc.py:186`). Both
reports say so and both are right.

---

## 4. The citation repair: does it hold, and what remains

I opened roughly seventy distinct citations across the two reports in this
pass, weighted towards the ones the correction pass claims to have fixed and
towards the rows neither adversary had tested.

### 4.1 Pass A's self-audit is honest and the repair holds

Pass A's notes §31 declares twenty faults: nine caught by an adversary, eleven
found in its own pass. I re-derived all eleven of the self-found ones from the
source and **every one is correct**, including the three where it says every
cited line was wrong:

| A's round-one claim | Verified actual |
|---|---|
| L9 mul/add at `codegen_llvm.cpp:1938`, `:1941`, `:1943` | muls `:1939`, `:1942`; add `:1944`. `:1938` is a continuation of the `size_var` initialiser, `:1941` a comment, `:1943` a closing brace. **All three were wrong.** |
| L12 loop index at `:2213-2214`, `:2222`, `:2226`, `:2251` | store `:2190-2191`; loads `:2216`, `:2228`, `:2253`; increment `:2273`. `:2213` and `:2222` are `SetInsertPoint`, `:2226` blank, `:2251` a comment. **All four were wrong.** |
| S15 constants at `spirv_ir_builder.cpp:283-284` | `:223-224`. `:282-286` is `IRBuilder::get_null_type` — **a different function.** |

I also re-derived the other eight of the eleven and all hold: the i64 guard
`:311-313` with return `:314` (`:310` is `return t_int32_;`);
`bitmasked_activation` `:383-435`; the `make_pointer` call at `:408`;
the pointer branch `spirv_types.cpp:490-498`; the ABI comment
`codegen_llvm.cpp:284-292` (`:294` is the `functions` vector); the bit-pointer
comment `:1750-1754` with the assertion at `:1755`; the `TI_ERROR` at
`llvm_codegen_utils.cpp:141-142` (`:136-139` is the `TI_INFO` about differing
contexts, so A's "`:139-140`" was itself a slightly loose description of the
old error, but the target is right); and the PSB framing `:821`/`:825`/`:826`/
`:827`.

I sampled twenty-two entries from A's "checked and correct" list and broke
none: `struct_llvm.cpp:74-75`, `:111-113`, `:153`, `:171-188`, `:247`, `:266`;
`codegen_llvm.cpp:1857-1861`, `:1931`, `:2113-2119`, `:2117-2118`, `:2163-2164`,
`:2157`, `:2336`, `:2339`;
`spirv_codegen.cpp:355`, `:371-372`, `:503-508`, `:707-711`, `:321-325`,
`:2194-2220`, `:2197`; `spirv_ir_builder.cpp:64-72`, `:155-176`, `:288-332`,
`:565-608`, `:1536`; `spirv_ir_builder.h:51`, `:529-564`;
`runtime.cpp:562-563`, `:567-571`; `constants.h:5`, `:12`, `:13`, `:15`, `:28`;
`program.cpp:141`, `:144`, `:235`, `:559-567`; `snode.cpp:12`, `:220`;
`spirv_types.cpp:167-214` with i64 `:179-181` and u64 `:199-201`;
`opengl_device.cpp:506-530`; `metal_device.mm:1025`, `:1035-1038`, `:1051-1052`,
`:131-135`; `dx_device.cpp:557-568`; `vulkan_device_creator.cpp:630-632`,
`:821-827`.

**The repair holds.** Pass A's citations are now accurate at a rate I could not
break by sampling.

**One deliberate residual.** A's §3.4.1 closing paragraph lists eight span
edges that are off by one and says they are "recorded in the notes rather than
edited row by row". So the S-table and §3.0 still ship wrong spans that A knows
about: S2 `:2317-2325` (actual `:2317-2324`), S7 `:531-540` (actual `:532-541`,
verified — `:531` is blank and the function opens at `:532`), §3.0 `:727-750`
(actual `:727-749`, verified), S8 `:2215-2218` (actual `:2214-2218`), S5
`:503-507` (the `add` is at `:508`), L7 `:1794-1828` (actual `:1792-1829`),
S10's `:772-777` (actual `:773-777`), §3.0's `:696-717` (actual `:695-718`).
The tables are what a reader transcribes. Knowing a row is wrong and leaving it
wrong is not a repair; the fix is eight one-character edits.

### 4.2 Pass B's re-verification claim is FALSE, and four faults survive

`report-02b-codegen.md:16-17` states: "Every **[V]** in this revision was
re-opened at the named line during this pass, not carried forward on trust."
Its notes §28 lists what was re-opened. **Four rows of its §3 tables appear
in neither list, and all four are wrong.**

| B row | Claim | Verified actual | Severity |
|---|---|---|---|
| `report-02b:248` — `codegen_llvm.cpp:1841-1843` | "`GetChStmt` bit-struct branch: `get_constant(bit_offset)` i32" | The bit-struct branch is `:1834-1839`; `get_constant(bit_offset)` is on **`:1838`**. `:1841-1843` is the `else` branch, `call_struct_func`, which contains **no** `get_constant` at all. **Every cited line is wrong.** | misattributed to the wrong branch |
| `report-02b:309` — `spirv_codegen.cpp:2095, 2097` | "i32 phi induction variable and a signed `lt`" | The phi is `ir_->make_phi(begin_.stype, 2)` at **`:2093`**; the `lt` is `ir_->lt(loop_var, end_)` at **`:2096`**. `:2095` is `loop_var.set_incoming(0, begin_, init_label);` and `:2097` is `OpLoopMerge`. **Both cited lines are wrong.** | misattributed |
| `report-02b:310` — `spirv_codegen.cpp:1791, 1821, 1823` | "Serial `RangeForStmt` step uses `const_i32_one_`" | `:1791` and `:1823` do; **`:1821` is `spirv::Value next_value;`** and contains no such use. `:1825` does and is omitted. | one wrong, one omitted |
| `report-02b:219` — `codegen_llvm.cpp:2272` | "`create_increment(loop_index, block_dim)` on i32" | The call is on **`:2273`**; `:2272` is blank. Pass A caught and corrected this exact line in its own report (§3.4.1). | off by one |

Two further span faults in B, one of them a correction that replaced a wrong
span with another wrong span:

- `report-02b:286` and `:175` give `translate_ti_type` as
  `spirv_types.cpp:484-497`, marked "**Span corrected from `:486-495`**". The
  function is **`:484-514`** (closing brace at `:514`) and the pointer branch is
  **`:490-498`**. Neither reading gives `:484-497`. B's own §4.4 uses `:484-514`,
  so the report contradicts itself between §3.2 and §4.4. Pass A's S16 has the
  branch right at `:490-498`.
- `report-02b:285` and `:512` give `from_taichi_type` as
  `spirv_ir_builder.cpp:334-341`. The function is **`:334-353`**; `:341` is the
  middle of the pointer branch, which is `:337-342`. Pass A has `:337-342`.

I did also test twenty of B's other rows and broke none, including
`codegen_llvm.cpp:2308-2311`, `:2689-2692`, `:2405-2407`, `:1188`, `:1192`,
`:1755`, `:1750-1753`; `struct_llvm.cpp:179`, `:181-185`, `:258`, `:260-264`;
`spirv_ir_builder.cpp:960`, `:963-979`, `:981-997`, `:309-310`, `:323-324`,
`:576`, `:591`, `:605`; `spirv_ir_builder.h:149-159` with `ADD(uint32_t, v)` at
`:158`; and the whole `get_primitive_type` table at `:375-384`, which is exact
in all eight rows.

So B's citation quality is high in aggregate and its self-audit found twenty
real faults. What is not true is the blanket claim at `:16-17`. Six rows were
carried forward on trust and six rows are wrong.

---

## 5. Newly wrong

### 5.1 Pass A contradicts itself between §4.2.2 and §4.2.5

`report-02:815-825` (§4.2 point 2, third bullet) says a widened `make_pointer`
"routes **every** SNode access into the `OpConvertUToPtr` /
`spv::StorageClassPhysicalStorageBuffer` branch at `:2198-2204`".

`report-02:840-843` (§4.2 point 5) says "The SSBO index widens with the
pointer. `at_buffer` ... derives the array index from the pointer by shifting,
so a u64 pointer yields a u64 `OpAccessChain` index."

These cannot both be true. I opened `at_buffer` at `spirv_codegen.cpp:2194-2220`:
the u64 test is at `:2197` and its branch **returns** at `:2204`. The shift at
`:2214-2216` and the `struct_array_access` at `:2217-2218` are only reached when
the test is false. If point 2 is right, point 5's code is unreachable.

Point 5 is the round-one text; point 2 is new in the correction pass and was
added in front of it without reconciling. **Point 5 is the one that is wrong**,
and it is wrong in both sub-cases:

- On a device *with* Int64, `u64_type()` carries `dt == u64`
  (`declare_primitive_type` sets `t.dt = dt` at `spirv_ir_builder.cpp:1536`,
  verified), the `:2197` test is true, and the shift never runs.
- On a device *without* Int64, `u64_type()` returns a default-constructed
  `SType` whose `dt` is default-constructed. `DataType::DataType()` is
  `ptr_(PrimitiveType::unknown.ptr_)` at `taichi/ir/type.cpp:20` and
  `operator==` compares pointers (`taichi/ir/type.h:97-99`), so the `:2197` test
  is false — but then execution reaches
  `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at `:2207-2210`, and
  `is_integral` (`taichi/ir/type_utils.h:103-...`) returns false for `unknown`.
  **The error fires.** There is no u64 index either way.

Pass B does not have this contradiction: its §6 states the routing consequence
and never claims a widened SSBO index.

That last sub-case is also a small correction to a claim **both** reports make.
A `report-02:812-814` and B `report-02b:746-749` both say that on a device
without Int64, flipping `use_64bit_pointers` yields "type id 0" as the outcome.
That is true at the type-declaration site, but the first `at_buffer` use raises
a named `TI_ERROR` at `spirv_codegen.cpp:2207-2210`. So this particular sub-case
fails **loudly**, unlike the kernel-signature path of §2.2, which does not go
through `at_buffer` at all. The distinction is worth keeping because it is
exactly the distinction 6.2's stub note turns on: one 64-bit path has an
accidental guard and the other has none.

### 5.2 Pass A names a class that does not exist

`report-02:560-564`: "**Imported Vulkan is a fourth.**
`VulkanRuntimeImported::Inner::Inner` (`c_api/src/taichi_vulkan_impl.cpp:18-56`)".

There is no `Inner`. The constructor is
`VulkanRuntimeImported::Workaround::Workaround`, opening at
`c_api/src/taichi_vulkan_impl.cpp:19`, declared at
`c_api/src/taichi_vulkan_impl.h:27-29`. A grep for `Inner` across both files
returns nothing.

Every other line in the claim is exact and I verified each: `caps{}` at `:35`,
`spirv_version` only at `:37-43`, the commented-out physical-storage-buffer set
at `:45-51`, `set_caps` at `:53`. The finding itself is correct and is one that
neither adversary nor pass B has. But a fabricated identifier in an otherwise
exact citation is the worst kind, because a reader who greps for it concludes
the finding is invented.

### 5.3 Pass A's §2.1 conclusion contradicts its own §2.1 finding

`report-02:200-204` establishes that with `demote_dense_struct_fors` false the
`all_dense` early return "never fires **even for a fully dense tree**".
`report-02:210-212` then concludes "Both surviving routes involve sparse
SNodes." The first sentence describes a route that does not involve sparse
SNodes. Pass B has the same conclusion (`report-02b:651-654`) without the
contradicting finding, so B is internally consistent and incomplete where A is
complete and inconsistent.

The correct statement is: the `node_allocators` / `ambient_elements` route
requires `pointer` or `dynamic` SNodes; the `element_lists` route requires
either a non-dense SNode **or** `demote_dense_struct_fors == false`. Whichever
way the planner reads it, both reports currently overstate the sparsity
precondition.

### 5.4 Two words that should come out of both reports

"Silently", at `report-02:955` and `report-02b:467-473`, describing what
happens to the id-0 module. §2.1 shows it is not silent: two `TI_WARN`s are
emitted and the module is shipped anyway. The right word is "with two
warnings and no effect on the output".

---

## 6. Still missing

### 6.1 Both reports enumerate the target landscape for one of the five bypassed capabilities

This is the largest remaining gap and it follows directly from a claim both
reports make. Both establish that the `Translate2Spirv` bypass covers five
capabilities (§2.2). Both then enumerate the in-tree target set for
`spirv_has_int64` **only** — A §3.3.7, B §4.5, each a five-row table of Int64
setters. Neither asks the same question about the other four.

I ran it. Every setter of every one of the five, over `taichi/rhi/` and
`c_api/src/`:

| capability | setters |
|---|---|
| `spirv_has_int8` | `metal_device.mm:1046` (unconditional), `vulkan_device_creator.cpp:790` (queried) |
| `spirv_has_int16` | `metal_device.mm:1047` (unconditional), `vulkan_device_creator.cpp:628` (queried), `opengl_device.cpp:516`, `:521` (extension probes) |
| `spirv_has_int64` | `metal_device.mm:1052`, `vulkan_device_creator.cpp:632`, `opengl_device.cpp:511`, `c_api/src/taichi_opengl_impl.cpp:9` |
| `spirv_has_float16` | `metal_device.mm:1048` (unconditional), `vulkan_device_creator.cpp:787`, `opengl_device.cpp:517`, `:525` |
| `spirv_has_float64` | `vulkan_device_creator.cpp:636`, `opengl_device.cpp:512`, `c_api/src/taichi_opengl_impl.cpp:10` |

**`taichi/rhi/metal/metal_device.mm` contains no occurrence of the string
`float64` at all.** So **every Metal device, on every Apple GPU family
including the newest, reports no `spirv_has_float64`.** Combined with the bare
`f64_type()` at `spirv_types.cpp:428`, an `f64` kernel argument, return value or
argpack member compiled for Metal produces an `OpTypeStruct` referencing id 0 —
unconditionally, on all Metal hardware, not on a legacy subset.

That is a stronger and more concrete instance of the defect than the Int64 one
both reports lead with, and neither report contains it. It also changes the
shape of 6.2's stub requirement: the architecture is not being asked to express
"some old targets lack Int64", it is being asked to express a matrix of five
optional widths against five backends in which no backend has all five and one
backend is missing a capability on every device it will ever run on.

Two smaller consequences of the same table, also absent from both:

- **DX11 and imported Vulkan lack all five**, not just Int64.
  `dx_device.cpp:563-565` and `taichi_vulkan_impl.cpp:35-53` each set only
  `spirv_version`. Pass A gets close ("No integer or float capability of any
  kind", `report-02:557-558`) but does not connect it to the other four
  accessors; pass B says "sets no integer capability at all"
  (`report-02b:491`), which is narrower than the truth.
- **The C API OpenGL override removes capabilities as well as adding them.**
  `taichi_opengl_impl.cpp:8-12` sets exactly `spirv_has_int64`,
  `spirv_has_float64` and `spirv_version`, and `set_caps` replaces the whole
  object, so it discards any `spirv_has_int16` / `spirv_has_float16` that the
  `GLAD_GL_NV_gpu_shader5` / `GLAD_GL_AMD_gpu_shader_int16` /
  `GLAD_GL_AMD_gpu_shader_half_float` probes at `opengl_device.cpp:515-526`
  had granted. Pass A says the probes are discarded but cites only `:515-522`,
  omitting the `:524-526` half-float block. Pass B does not mention the probes
  at all.

### 6.2 Neither report establishes what happens to the invalid module

§2.1. Both stop at "nothing validates it". The chain that determines the actual
severity — `optimizer.cpp:596-598` → `build_module.cpp:68-74` →
`binary.cpp:450`/`:456`/`:473` → `spirv_codegen.cpp:2646-2651` (warning, not
error) → `:2750` (`success` set) → `:2760` (`success` read only inside
`if constexpr (false)`) → `:2775` (module pushed anyway) — is readable in this
tree, since `external/SPIRV-Tools` is checked out. Neither report opened it.

This is the difference between "6.2 must add a guard or the failure is
invisible" and "6.2 must add a guard, and separately the existing warning path
discards its own result at `spirv_codegen.cpp:2750-2775`". They are different
work items.

### 6.3 Gaps particular to pass B

1. **`ti_set_runtime_capabilities_ext` is absent entirely** (§2.3).
2. **`SNode::reset_counter()` is presented as a live reset avenue**
   (`report-02b:589-591`) when it has no caller (§2.4).
3. **No sweep list in the report.** Pass A closes its coverage with an explicit
   list and a countable claim: 45 `.h`/`.cpp` files under `taichi/codegen/`,
   plus six `CMakeLists.txt`, all accounted for. **I tested that claim.**
   `find taichi/codegen -type f \( -name '*.h' -o -name '*.cpp' \) | wc -l`
   gives 45, total 51, and every one of the 45 is named in A's report either as
   a finding or in the §6 sweep — including the seven that only appear via the
   `{h,cpp}` and "four per-backend `codegen_*.h`" shorthands. A's coverage
   claim is genuine. B claims "51 files" in its header line and then names
   **24** of the 45 anywhere in the report. Twenty-one are unmentioned,
   including `llvm/codegen_llvm_quant.cpp`, `spirv/snode_struct_compiler.cpp`,
   `spirv/lib_tiny_ir.h`, `spirv/kernel_utils.cpp`, `dx12_global_optimize_module.cpp`
   and `dx12_lower_runtime_context.cpp`. B's notes §10 does sweep four of those,
   so the work was done; the report does not carry it, and a report headed
   "Complete inventory" with no sweep list is not falsifiable. That is the
   difference between the two reports on completeness and it is not a small one.
4. **`spirv/snode_struct_compiler.cpp` and `PhysicalPointerType`.** B never
   mentions either. `spirv_types.h:71-77` derives `PhysicalPointerType` from
   `IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75` unconditionally, and
   its only construction is `snode_struct_compiler.cpp:53` inside a function
   whose only call is commented out at `:16-19`. Pass A has it (§3.3.6). It is a
   64-bit type in the SPIR-V type layer with no capability gate, which is
   directly on 6.2's question.
5. **`codegen_llvm_quant.cpp`.** Twenty-two i32 constant sites, covered by A
   with the reasoning that they are bit offsets within a physical type and
   therefore not a scaling limit. B's inventory omits the file. B's notes §10
   reaches the same conclusion, so this is a transcription gap rather than a
   research gap — but the planner reads reports.

### 6.4 One gap common to both, small

Neither report notes that the mandatory-width accessors reached by the same two
visitors (`i32_type()` at `spirv_types.cpp:401`, `u32_type()` at `:413`,
`bool_type()` at `:407`, `f32_type()` at `:426`) are safe only because their
members are declared unconditionally at `spirv_ir_builder.cpp:155`, `:164-165`,
`:170`. A states the fact but not that it is what makes the count five rather
than nine. See §2.2.

---

## 7. Verdicts, argued, and what remains for consensus

### 7.1 Are both reports CORRECT?

**Pass A: yes.** I attacked its three headline claims — the eight-accessor
bypass, the out-of-bounds range store, the disabled validator — and all three
survived at every link. Its citation repair is real: I re-derived all eleven of
its self-found faults and sampled twenty-two of its "checked and correct"
entries without breaking one. Its two defects are §5.1 (an internal
contradiction between §4.2.2 and §4.2.5, where the older text should have been
deleted) and §5.2 (`VulkanRuntimeImported::Inner`, a class that does not
exist), plus §5.3 (a conclusion contradicting its own finding two paragraphs
earlier). None of the three changes an answer the planner would act on.

**Pass B: yes on substance, no on its own method claim.** Every substantive
statement I tested held, including the whole of its rewritten §4, its §5, and
its four adjudications in §7. Its withdrawals are complete and correctly
argued. But `report-02b:16-17` claims every **[V]** was re-opened this pass and
that is demonstrably false for at least six rows, four of which are wrong and
two of which are misattributed to the wrong branch or the wrong function
(§4.2). A method claim that is false is a correctness defect in a report whose
whole value is that its citations can be transcribed.

### 7.2 Are both reports COMPLETE?

**Neither, but the gap is now narrow and mostly shared.** The territory itself
is covered: I found nothing in `taichi/codegen/` that pass A missed, and pass B
missed nothing that its notes missed. The remaining incompleteness is at the
same seam as in round one — `taichi/rhi/` and the SPIRV-Tools submodule — and
it is two things:

1. The capability landscape is enumerated for one of the five capabilities the
   reports themselves say are bypassed (§6.1). Metal reports no Float64 on any
   device; that is a live instance of the defect on shipping hardware and it is
   in neither report.
2. Neither report opened the SPIRV-Tools path that determines what actually
   happens to the invalid module (§6.2), which is the severity question the
   whole finding turns on.

Pass B additionally has five gaps of its own (§6.3), of which the missing sweep
list and the absent `ti_set_runtime_capabilities_ext` are the two that matter.

### 7.3 Is GENERAL CONSENSUS reached?

**Not yet.** The two reports now agree on every load-bearing fact, agree with
both round-one adversaries where the adversaries were right, and correctly
reject both adversaries where they were wrong. That is the substance of
consensus. What blocks declaring it is that both reports still contain
statements a reader would act on that the source does not support.

Scoped exactly, this is what remains. Nothing here is structural and nothing
requires re-investigation.

**Pass A — three corrections and two additions.**
1. Delete or rewrite §4.2 point 5 (`report-02:840-843`). It contradicts §4.2
   point 2 and the code at `spirv_codegen.cpp:2197`, `:2204`.
2. `VulkanRuntimeImported::Inner::Inner` → `VulkanRuntimeImported::Workaround::Workaround`,
   `c_api/src/taichi_vulkan_impl.cpp:19-56`, declared at `taichi_vulkan_impl.h:27-29`.
3. §2.1's "Both surviving routes involve sparse SNodes" (`report-02:210-212`)
   contradicts §2.1's own finding at `:200-204`. Restate as in §5.3 above.
4. Apply the eight span edges A's own §3.4.1 records but declines to make, in
   the tables where a reader will read them: S2, S5, S7, S8, S10, L7 and the two
   in §3.0.
5. Add §6.1 (the four other capabilities' target sets, and Metal's absent
   Float64) and §6.2 (what the optimiser's parser does with id 0).

**Pass B — four corrections and four additions.**
1. `codegen_llvm.cpp:1841-1843` → the bit-struct branch is `:1834-1839`,
   `get_constant(bit_offset)` at `:1838`.
2. `spirv_codegen.cpp:2095, 2097` → phi at `:2093`, `lt` at `:2096`.
3. `spirv_codegen.cpp:1791, 1821, 1823` → `:1791`, `:1823`, `:1825`.
4. `codegen_llvm.cpp:2272` → `:2273`; `translate_ti_type` `:484-497` →
   function `:484-514`, pointer branch `:490-498`; `from_taichi_type`
   `:334-341` → function `:334-353`, pointer branch `:337-342`. And withdraw or
   qualify the claim at `:16-17`.
5. Add `ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:317-334`,
   `set_caps` at `taichi/rhi/public_device.h:855-857`).
6. Correct `:589-591` to say `SNode::reset_counter()` has no caller.
7. Carry the notes §10 sweep into the report, and add
   `codegen_llvm_quant.cpp`, `snode_struct_compiler.cpp` and
   `PhysicalPointerType` (`spirv_types.h:71-77`, `snode_struct_compiler.cpp:53`)
   to the inventory or the sweep list.
8. Add §6.1 and §6.2 as for pass A.

**Both — one word.** "Silently" at `report-02:955` and `report-02b:467-473`.

On the plan's own test at §2.2 — hardware changes speed and granularity, never
capability — both reports now serve it correctly, and neither treats the NVIDIA
tiers in §5.2 as a vendor target. Pass A's escalation 7 and pass B's escalation
13 both put DX11 and imported Vulkan to the planner rather than deciding they
are out of scope, which is the right handling. The Metal-Float64 fact in §6.1
sharpens the same point: a portable path that produces an invalid module for an
`f64` argument on an entire backend is a gap in the portable path, per §5.2's
closing consequence, not a reason to prefer the vendor path.

---

## 8. Escalations

Unresolved. Each needs a decision I am not making.

1. **Whether `spirv_opt_options_.set_run_validator(false)`
   (`spirv_codegen.cpp:2710`) is deliberate.** It is a build-time-invariant
   choice with no comment and no config. Turning it on would catch the id-0
   modules of §2.2 at the point of production. Turning it on also costs
   validation time on every kernel compile, which is directly against 6.4's
   dispatch-throughput criterion. That trade is the planner's. I am not
   proposing it be flipped; I record only that nothing validates today and that
   §2.1 establishes the module is caught later and shipped anyway.

2. **Whether `success` at `spirv_codegen.cpp:2742-2750` is meant to gate
   anything.** It is computed, set on optimiser failure, and read only inside
   the `if constexpr (false)` block at `:2758-2772`. The module is pushed at
   `:2775` either way. Whether the intent was to fall back to
   `task_res.spirv_code`, or to fail the compile, or neither, is not
   determinable from the source, and the standing instructions forbid me
   proposing a change.

3. **Whether Metal's absent `spirv_has_float64` (§6.1) is a target constraint
   or an oversight.** `metal_device.mm` sets int8, int16 and float16
   unconditionally at `:1046-1048` and Int64 under a family test at `:1051-1052`,
   but never mentions float64. Whether Metal genuinely cannot do it, or whether
   the capability was simply never wired, decides whether 6.2's stub
   architecture must express a permanent Float64-less backend or a temporary
   one. Not determinable from this tree.

4. **Which quantity the SNode ceiling should bound.** Carried forward
   unchanged from round one and from both reports. `struct_llvm.cpp:266`
   bounds a per-tree count; `runtime.cpp:567-569` are indexed by a global id;
   `runtime.cpp:1003-1007` writes a range starting at that global id;
   `kMaxNumSnodeTreesLlvm = 512` (`constants.h:13`) bounds a different thing
   again. Item 8.1.2 reserves the value; the quantity is equally unreserved.

5. **Whether the reachable set for the out-of-bounds store includes all-dense
   trees.** §5.3. It does when `demote_dense_struct_fors` is false
   (`llvm_runtime_executor.cpp:402`), which is a one-flag change from a
   defaulted-true public field (`compile_config.cpp:18`,
   `compile_config.h:28`). Whether this project will ever set it false is a
   configuration decision nobody has made, and it changes how urgent 6.1's
   correctness half is.

6. **Whether `Extension::data64` is meant to be live.** Unchanged. Declared at
   `taichi/inc/extensions.inc.h:6`, granted at `taichi/program/extension.cpp:12`,
   `:16`, `:20`, queried by nothing.

7. **`create_bit_ptr`'s `isIntegerTy(32)` assertion**
   (`codegen_llvm.cpp:1755`). Unchanged. Both reports raise it and both decline
   to decide; I also decline.

8. **Signedness of a widened loop counter.** Unchanged. Every loop comparison
   in both paths is signed except the SPIR-V struct-for at
   `spirv_codegen.cpp:2157` (`OpULessThan`). Width and signedness are separate
   decisions and neither is settled.

---

## 9. Divergence with adversary2-02-1

At the time of writing,
`/opt/project/taichi/modernization/investigation/adversary2-02-1.md` **does not
exist**. I checked the directory after completing §0-§8; it contains
`adversary2-01-1.md`, `adversary2-01-2.md`, `adversary2-03-1.md`,
`adversary2-03-2.md`, `adversary2-04-1.md` and `adversary2-04-2.md`, but no
round-two file for territory 02 other than this one. I did not wait for it, per
my instructions. No comparison section can be recorded.

---

## 10. Divergence with adversary2-02-1 — appended after writing

`adversary2-02-1.md` did not exist when I finished §0-§9; it appeared before I
closed out. §9 records the state at the time of writing and is left standing.
This section is the comparison. I verified every claim of theirs I had not
already checked myself before crediting it, and I opened the source again on
the one place we disagree.

### 10.1 Where we agree, independently

We reached the same verdict on all six of the arbitration's new claims and on
both adjudications:

- Nothing validates the module: `spirv_codegen.cpp:2710` unconditional, `:2758`
  compiled out, and a tree-wide grep returning no validation anywhere else.
- Eight accessors, **five** capability gates. Six has no reading in the source.
  They confirm it was an arithmetic slip on their part; both reports were right
  to return five and flag it.
- `ti_set_runtime_capabilities_ext` at `c_api/src/taichi_core_impl.cpp:317-334`
  with the move-assign `set_caps` at `taichi/rhi/public_device.h:855-857`, and
  **absent from report B entirely**. We found this independently and reached the
  same conclusion, including that it is the general case of which B's OpenGL
  override is one instance.
- `SNode::reset_counter()` (`snode.h:348-350`) has no caller; the only reset is
  `program.cpp:144`; **report B still presents it as a live avenue** at
  `report-02b:589-591`. Same three grep hits, same conclusion.
- The `kernel_utils.h` narrowing, now carried by both reports.
- `all_dense` is seeded from `config_.demote_dense_struct_fors` at
  `llvm_runtime_executor.cpp:402` and the loop at `:403-410` can only clear it.
- `bitmasked_activation`: **both reports are right and adversary 02-1 was wrong**
  in round one. They record their own error plainly at their §1.3. I confirmed
  the same twelve `u32_type()` lines by extraction.
- The two runtime helpers: `runtime.cpp:1029` and `:1038`. **I was wrong in
  round one** and both reports adjudicated correctly against me. We both accept
  the adjudication that went against us.

Two adversaries arriving separately at the same five-not-six count, the same
two accepted self-corrections, and the same two gaps in report B should settle
those.

### 10.2 Where they found what I did not — checked, and they are right

**1. The `:2464` / `:2511` swap. This is the most consequential finding in
either round-two analysis, and it is theirs.**

I opened it rather than taking it. Extracting function openings, closings and
the three `ir_translate_to_spirv` calls from `spirv_codegen.cpp:2320-2530`:

```
2326:   void compile_args_struct() {
2386:         ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2387:     args_struct_type_.id = ir2spirv_map[struct_type];
2400:   }
2402:   spirv::Value compile_argpack_struct(const std::vector<int> &arg_id,
2464:         ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2465:     argpack_struct_type.id = ir2spirv_map[struct_type];
2481:   }
2483:   void compile_ret_struct() {
2511:         ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2512:     ret_struct_type_.id = ir2spirv_map[struct_type];
2527:   }
```

`:2464` assigns `argpack_struct_type.id` and sits inside
`compile_argpack_struct`, which opens at `:2402`. `:2511` assigns
`ret_struct_type_.id` and sits inside `compile_ret_struct`, which opens at
`:2483`. **Both reports have the last two labels swapped**, and so did both
round-one adversaries, and so did I in `adversary-02-2.md` §1.2. They are right.

Their observation that report B's own §4.4 is self-refuting is also right:
`report-02b:428-431` pairs `:2464` with "`compile_ret_struct` (`:2483-...`)"
and `:2511` with "`compile_argpack_struct` (`:2402-...`)", and a line at 2464
cannot lie inside a function opening at 2483. B supplied the spans that
disprove its own labels.

I accept their consequence too: the five `translate_ti_type` call sites that
both reports name as a candidate fix location (`:2344`, `:2358`, `:2420`,
`:2436`, `:2494`) partition across those three functions the other way round,
so anyone acting on either report's escalation 5/10 looks in the wrong function
first.

**2. The second widening-invalidity site at `spirv_codegen.cpp:398-399`.**
Confirmed:

```
398:    auto bitmask_mask = ir_->make_value(spv::OpShiftLeftLogical, ptr_dt,
399:                                        ir_->const_i32_one_, bitmask_bit_index);
```

Result Type is `ptr_dt`; Base is `const_i32_one_`, which is
`int_immediate_number(t_int32_, 1)` at `spirv_ir_builder.cpp:224` and is
therefore permanently 32-bit. Today `ptr_dt` is u32 and the widths agree. If
`make_pointer` widened, this becomes a 64-bit Result Type over a 32-bit Base —
invalid by the same SPIR-V rule as `:410-412` and in the **opposite**
direction. Both reports frame the function as "hardcodes u32 where the pointer
widens"; half of it hardcodes i32 under a result type that follows the pointer.
**They are right and I missed it.** My §3.1 differentiated `:405` from
`:411`/`:412`/`:414` but did not look at the `ptr_dt`-typed instructions above
for the mirror-image problem.

**3. `get_buffer_value(BufferType::Args, PrimitiveType::i32)` is at `:618`, not
`:620`.** Confirmed: `:618` is the call, `:620` is
`buffer_val = ir_->make_access_chain(`. **My round-one citation was wrong** and
report B's `:618` is right. Their point that report A should take B's
middle-link finding into its escalation-4 closure is well made: A closes "no
earlier check" on the front of the pipeline (`Extension::data64`) and the back
(the validator) without addressing the middle, where a guarded function does run
but on a placeholder type.

**4. Two further faults in report B I had not tested.** Both confirmed:
`spirv_codegen.cpp:2146` is `spirv::Label loop_body = ir_->new_label();` with no
`u32_type()` on it (the alloca is `:2149`); and `runtime.cpp:1335` is
`int num_parent_elements = parent_list->size();` and not a subscript, so B's
`:1334-1336` carries the exact fault report A found and corrected in itself.

**5. `struct_llvm.cpp:174-175` is undefined behaviour, not a wrap.** Correct C++.
Both operands are `int` (`taichi/ir/snode.h:41`, `:45`, `:49`), so signed
overflow is UB in the host compiler that builds the struct compiler. Both
reports say "wraps"/"overflow ... before LLVM sees it", which invites reasoning
about the wrapped value. Their wording is the right one.

**6. `rhi_constants.inc.h:12-16`** as the canonical enumeration of the five
optional scalar-type capabilities. Verified. A better citation for the count
than either report or I gave.

### 10.3 Where we disagree, and what the source says

**One substantive disagreement, and it is on the arbitration's own severity
question.**

Their §2.1 point 4 states: "a 0 `<id>` operand is a hard parse failure for any
conformant SPIR-V consumer. The failure is therefore **deferred to the driver at
shader-module creation**, not absent." Their §2.1 point 3 states: "Nothing in
Taichi looks at it. So there is **no Taichi-level diagnostic**." Their §6 item 3
asks *both* reports to add a sentence saying the failure is deferred to the
driver.

**That is wrong, and the sentence they propose adding is false.** The failure is
caught inside Taichi's own process, before the module ever reaches a driver.
`external/SPIRV-Tools` is checked out in this tree, so this is readable:

1. `spirv_opt_->Run(...)` at `spirv_codegen.cpp:2746-2747` runs on every kernel,
   which they and I both establish.
2. `Optimizer::Run` at `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598`
   skips `tools.Validate` when `run_validator_` is false (`:590-594`) — and then
   calls `BuildModule(impl_->target_env, consumer(), original_binary, ...)` at
   `:596-597` **unconditionally**, returning false at `:598` if it yields null.
3. `BuildModule` (`external/SPIRV-Tools/source/opt/build_module.cpp:56-75`) runs
   `spvBinaryParse` at `:68-69` and returns null unless it succeeds (`:74`).
4. `spvBinaryParse` rejects a zero `<id>` at
   `external/SPIRV-Tools/source/binary.cpp:473` for `SPV_OPERAND_TYPE_ID`, which
   is what an `OpTypeStruct` member operand is, and at `:450`/`:456` for type and
   result ids.
5. The diagnostic goes to the consumer Taichi installed at
   `spirv_codegen.cpp:2682`, namely `spriv_message_consumer` at `:2641-2659`.
   Its first branch is `if (level <= SPV_MSG_FATAL)` at `:2646`, and
   `SPV_MSG_FATAL` is **0** while `SPV_MSG_ERROR` is **2** in the enum at
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-92`. So the parse
   error falls through to `else if (level <= SPV_MSG_WARNING)` at `:2649` and is
   emitted as **`TI_WARN`**.
6. `Run` returning false then fires `TI_WARN_IF(..., "SPIRV optimization
   failed")` at `:2745-2748` and sets `success = false` at `:2750`.
7. `success` is read **only** at `:2760`, inside the `if constexpr (false)` block
   at `:2758-2772`. Line `:2775` pushes `std::move(optimized_spv)` regardless,
   and on a failed `Run` that vector still holds the unmodified copy made at
   `:2740`.

So there **is** a Taichi-level diagnostic — two of them — and the failure is
**not** deferred to the driver. It is detected in-process, downgraded from error
to warning by `spriv_message_consumer`'s branch order, and then discarded
because the flag that records it is dead code. That is a third severity, and it
is the one the arbitration was asking to have settled.

The practical difference is in what §6 asks the reports to add. Their edit would
have both reports assert a deferral that does not happen and would leave
`spirv_codegen.cpp:2750-2775` — where the detection is thrown away — unrecorded
in either report. My §7 asks instead for the chain above, which names a specific
dead-code defect the planner can act on. **On this one, the source supports me.**

I do not think this weakens their §2.1 otherwise: their four numbered points
about the bypass itself, and their finding that there is no validation anywhere
in the tree rather than merely none in codegen, are correct and match mine.

**Two smaller divergences.**

- **They credit report A on the sparsity conclusion; A has the same fault as B.**
  Their §2.6 says "Report A has this right", citing `report-02:200-204`. A does
  state the seeding correctly there. But eight lines later, at
  `report-02:210-212`, A concludes "Both surviving routes involve sparse SNodes"
  — the identical sentence they correctly pin on B at `report-02b:651-654`. A
  contradicts its own finding within the same subsection. So their §6 item 6,
  which asks only B to withdraw that conclusion, should ask both. My §5.3 has it.

- **They do not have report A's §4.2 internal contradiction.** `report-02:840-843`
  (§4.2 point 5) says a widened pointer "yields a u64 `OpAccessChain` index" via
  the shift at `at_buffer`, while `report-02:815-825` (point 2) says the same
  widening routes every access into the branch that **returns** at
  `spirv_codegen.cpp:2204` before reaching that shift. Point 5 is round-one text
  the correction pass left standing behind new text that contradicts it. My §5.1
  has it with the sub-case analysis, including that on a device without Int64 the
  `:2197` test is false (default `DataType` is `PrimitiveType::unknown`,
  `taichi/ir/type.cpp:20`, compared by pointer at `taichi/ir/type.h:97-99`) and
  execution reaches the `TI_ERROR_IF` at `:2207-2210`, so that sub-case fails
  loudly rather than producing id 0 as both reports claim.

### 10.4 Where I found what they did not

1. **Metal reports no Float64 on any device, and neither report enumerates the
   target set for four of the five bypassed capabilities** (my §6.1). Both
   reports establish the bypass is five capabilities wide and then build a
   five-row target table for `spirv_has_int64` alone. I ran the same enumeration
   for all five. `taichi/rhi/metal/metal_device.mm` contains no occurrence of the
   string `float64` — it sets int8, int16 and float16 unconditionally at
   `:1046-1048` and Int64 under `family_apple3` at `:1051-1052`, and never sets
   float64. So an `f64` kernel argument on **every** Metal device, current
   hardware included, takes the bare `f64_type()` at `spirv_types.cpp:428` and
   emits id 0. That is a live, unconditional instance of the defect on shipping
   hardware, and it is a stronger case than the Int64 one both reports lead with.
   The same sweep shows DX11 and imported Vulkan lack all five, not just Int64,
   and that the C API OpenGL override *removes* the int16/float16 that the
   `GLAD_GL_NV_gpu_shader5` / `GLAD_GL_AMD_*` probes at
   `opengl_device.cpp:515-526` may have granted.

2. **Report B's `codegen_llvm.cpp:1841-1843` is misattributed to the wrong
   branch** (my §4.2). `report-02b:248` claims it is the `GetChStmt` bit-struct
   branch with `get_constant(bit_offset)`. The bit-struct branch is `:1834-1839`
   with the `get_constant` at `:1838`; `:1841-1843` is the `else` branch,
   `call_struct_func`, containing no `get_constant` at all. Every cited line is
   wrong. Their §4.2 lists six surviving B faults and this is not among them.

3. **Report B's `from_taichi_type` span** (`report-02b:285`, `:512`) is
   `spirv_ir_builder.cpp:334-341`; the function is `:334-353` and the pointer
   branch is `:337-342`. Report A has `:337-342`.

4. **Report B has no sweep list in the report, and the coverage claim behind its
   header is unevidenced** (my §6.3 item 3). I checked every one of the 45
   `.h`/`.cpp` files under `taichi/codegen/` against both reports by basename.
   A names all 45, directly or through its `{h,cpp}` and "four per-backend
   `codegen_*.h`" shorthands — I confirm their arithmetic. **B names 24.** The 21
   unmentioned include `llvm/codegen_llvm_quant.cpp`,
   `spirv/snode_struct_compiler.cpp`, `spirv/lib_tiny_ir.h` and
   `spirv/kernel_utils.cpp`. B's notes §10 sweeps four of those, so the work was
   done and not transcribed. A report headed "Complete inventory" with no sweep
   list is not falsifiable, and that is the real completeness difference between
   the two reports.

5. **`PhysicalPointerType` is absent from report B.** `spirv_types.h:71-77`
   derives it from `IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75`
   unconditionally, with its only construction at
   `snode_struct_compiler.cpp:53` inside a function whose only call is commented
   out at `:16-19`. An ungated 64-bit type in the SPIR-V type layer is directly
   on 6.2's question. A has it (§3.3.6); their §3.4 lists three findings A
   carries that B does not and this is a fourth.

### 10.5 Net

On everything the arbitration asked to be settled, we agree: the count is five,
the validator is off, `ti_set_runtime_capabilities_ext` is unchecked and missing
from B, `SNode::reset_counter()` is dead and B has not taken it, the
`all_dense` gate is a config field, and both adjudications in the reports are
correct — including the two that went against each of us.

Where we differ, the source supports **them** on the `:2464`/`:2511` swap, on
the second invalidity site at `spirv_codegen.cpp:398-399`, on `:618`, and on
three further citation faults in report B; and it supports **me** on what
actually happens to the id-0 module — it is caught in-process by the optimiser's
own parser, warned about twice, and shipped anyway because `success` at
`spirv_codegen.cpp:2750` is never read outside a compiled-out block — on
report A's §4.2 self-contradiction, on report A sharing B's sparsity
overstatement, on Metal's absent Float64 and the unenumerated four capabilities,
and on report B's `:1841-1843` misattribution and missing sweep.

Neither analysis is a superset of the other. Both reach the same verdict:
**correct in substance, not yet complete, consensus not yet reached, and the
residue is mechanical repair rather than dispute.** Read together, our two §6/§7
lists are the full set of edits, with the one correction that item 3 of their §6
should be replaced by the in-process chain in my §10.3 rather than a deferral to
the driver.
