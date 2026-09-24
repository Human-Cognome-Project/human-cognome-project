# Adversary 3, pass 2 — territory 02, code generation

Round three. Both reports and both round-two adversary files read in full, then
both notes files' amendment entries (`notes-02-codegen.md` §§36-45,
`notes-02b-codegen.md` §§32-41). Every claim below was opened at source before
being written. Where a figure is stated it is derived from an enumeration
printed in this file or from a command whose output is reproduced.

`external/SPIRV-Tools` and `external/SPIRV-Headers` are live checkouts in this
tree, so every SPIRV-Tools link is read, not quoted.

---

## VERDICTS

**CORRECT: NO.** One substantive fault survives, in `report-02b-codegen.md`,
repeated at four places. It is a claim about which instructions become invalid
under a widened pointer, and the report's own stated rule refutes two thirds of
it. Everything else in the seven verification items checks out at source.

**COMPLETE: NO.** `bitmasked_activation` holds **four** widening-invalidity
sites on one of its two call paths, not the two both reports now state. The two
neither report has are `spirv_codegen.cpp:392-394` and `:395-397`, and they
arise from a second mechanism — an uncast caller index — that neither report
names. Separately, `report-02-codegen.md`'s `get_buffer_value` placeholder
inventory is two rows short.

Both faults are in the same paragraph of each report's answer to item 6.2, and
both would misdirect a repair. Neither is a re-run of a previously reported
issue.

---

## 0. Correction to the dispatch brief

Item 4 of my brief says report B claims "342 file-qualified citations now
resolve." **Report B claims 278, not 342.** The string `342` occurs three times
in `report-02b-codegen.md` — at `:358`, `:779` and `:1593` — and every one is
the tail of the line range `spirv_ir_builder.cpp:337-342`, the
`from_taichi_type` pointer branch. The method statement is at
`report-02b-codegen.md:41`: "**278 distinct file-qualified citations** were
re-opened that way", repeated at `:1577`.

I tested the claim as written rather than the number in the brief. Result in §4.

---

## 1. The struct-compiling swap — CORRECTION LANDED, VERIFIED EVERYWHERE

**Source, opened directly.** Function boundaries and the three
`ir_translate_to_spirv` calls in `taichi/codegen/spirv/spirv_codegen.cpp`:

| Function | Opens | Closes | `ir_translate_to_spirv` |
|---|---|---|---|
| `compile_args_struct` | `:2326` | `:2400` | `:2386` |
| `compile_argpack_struct` | `:2402` | `:2481` | `:2464` |
| `compile_ret_struct` | `:2483` | `:2527` | `:2511` |

Confirmed by the line following each call: `:2387`
`args_struct_type_.id = ...`; `:2465` `argpack_struct_type.id = ...`; `:2512`
`ret_struct_type_.id = ...`. So `:2464` is **argpack** and `:2511` is
**return**. Both amendment agents are right.

**The correction landed at every occurrence.** I grepped both reports for all
five numbers and for the three function names.

- `report-02-codegen.md` — table at `:581-583` (correct), supporting evidence
  `:585-588`, the `translate_ti_type` partition at `:596-598`, the adjudication
  row at `:1015`, escalation 5's partition at `:1357-1359`, and the index at
  `:1481`. Six places, all consistent.
- `report-02b-codegen.md` — header note `:82-83`, §4.4 at `:500-513`, the
  `translate_ti_type` partition at `:521-523`, escalation 10 at `:1406`, §7.10
  at `:1281-1291`, ledger note at `:1609`. Six places, all consistent.

**The downstream partition is right too.** The five `translate_ti_type` call
sites are `:2344`, `:2358`, `:2420`, `:2436`, `:2494` (grep reproduced above),
and against the boundaries in the table they partition args `:2344, :2358`,
argpack `:2420, :2436`, ret `:2494`. That is what both reports now say.

**No residue.** Nothing in either report still pairs `:2464` with a return
struct or `:2511` with an argpack.

---

## 2. The severity chain — VERIFIED END TO END, EVERY LINK

Traced independently against the checked-out submodule. Every link holds.

| # | Claim | Source, read |
|---|---|---|
| 1 | Validator off, unconditionally | `spirv_codegen.cpp:2710` `spirv_opt_options_.set_run_validator(false);` — outside the `if (params.enable_spv_opt)` block that opens `:2683` and closes `:2709` |
| 2 | Optimiser runs on every kernel | copy at `:2740`, `spirv_opt_->Run(...)` at `:2746-2747`; only the `RegisterPass` chain `:2685-2708` is gated |
| 3 | `Validate` skipped, `BuildModule` unconditional | `external/SPIRV-Tools/source/opt/optimizer.cpp:590-594` guards `tools.Validate` on `opt_options->run_validator_`; `:596-597` calls `BuildModule` with no guard; `:598` `if (context == nullptr) return false;` |
| 4 | `BuildModule` returns null on parse failure | `build_module.cpp:68-69` `spvBinaryParse`, `:74` `return status == SPV_SUCCESS ? std::move(irContext) : nullptr;` |
| 5 | Parser rejects a zero `<id>` | `binary.cpp:450` type id, `:456` result id, `:473` `if (!word) return diagnostic(SPV_ERROR_INVALID_ID) << "Id is 0";` |
| 6 | Level stays `SPV_MSG_ERROR` | `diagnostic.cpp:89` `auto level = SPV_MSG_ERROR;`; the switch `:90-108` has no arm for `SPV_ERROR_INVALID_ID`, which reaches `default:` `:106-107`; consumer called `:112` |
| 7 | FATAL 0, ERROR 2 | `include/spirv-tools/libspirv.h:84` FATAL, `:87` INTERNAL_ERROR, `:91` ERROR, `:92` WARNING |
| 8 | Branch order downgrades | `spirv_codegen.cpp:2646` `if (level <= SPV_MSG_FATAL)` fails for 2; `:2649` `else if (level <= SPV_MSG_WARNING)` catches it, emitting `TI_WARN` at `:2650-2651`. Consumer installed `:2682` |
| 9 | Second warning, flag cleared | `TI_WARN_IF(..., "SPIRV optimization failed")` `:2745-2748`; `success = false` `:2750` |
| 10 | Flag dead | `success` declared `:2742`, read only at `:2760`, inside `if constexpr (false)` opening `:2758` and closing `:2772` |
| 11 | Invalid module shipped unchanged | `:2775` `generated_spirv.push_back(std::move(optimized_spv));`. `Optimizer::Run` reaches `optimized_binary->clear()` only at `optimizer.cpp:642`, past the `return false` at `:598`, so the vector still holds the `:2740` copy |

**The chain holds.** Report A states it at `:815-860` (§3.3.5a) and report B at
§4.4 and notes §38; both are accurate at every line I opened.

**One imprecision, in `notes-02-codegen.md` §39 point 5 only.** It says the
switch at `diagnostic.cpp:90-108` "only *lowers* it for four specific codes."
The switch has four arms covering **seven** codes, and two of those arms
**raise** severity rather than lower it: `SPV_UNSUPPORTED` /
`SPV_ERROR_INTERNAL` / `SPV_ERROR_INVALID_TABLE` to `SPV_MSG_INTERNAL_ERROR`
(`:98-102`) and `SPV_ERROR_OUT_OF_MEMORY` to `SPV_MSG_FATAL` (`:103-105`). The
conclusion is unaffected — `SPV_ERROR_INVALID_ID` reaches `default:` either way
— and **neither report repeats it**; report A's §3.3.5a link 8 states only that
the code hits the `default:` arm, which is exact. Notes-only, no action.

**One label off, in both reports and both notes files.** Both say an
`OpTypeStruct` member operand is `SPV_OPERAND_TYPE_ID`. Grammatically it is
`IdRef` with quantifier `*`
(`external/SPIRV-Headers/include/spirv/unified1/spirv.core.grammar.json:391-399`),
which the parser expands to `SPV_OPERAND_TYPE_VARIABLE_ID` +
`SPV_OPERAND_TYPE_OPTIONAL_ID` (`SPIRV-Tools/source/operand.cpp:433-436`, pushed
at `binary.cpp:513`). `SPV_OPERAND_TYPE_OPTIONAL_ID` is the label at
`binary.cpp:472`, sharing the same case body as `SPV_OPERAND_TYPE_ID` at `:471`
and hitting the identical `"Id is 0"` at `:473`. **The cited line, the error
code and the conclusion are all correct**; only the operand-type name is one
label off. I record it because standing instruction §10.6 draws exactly this
distinction, not because it changes anything.

---

## 3. The counts — SETTLED BY ENUMERATION

Enumerated from `spirv_ir_builder.cpp:149-176`, `spirv_types.cpp:393-431` and
`taichi/inc/rhi_constants.inc.h:12-16`, all printed.

| Quantity | Count | Rows |
|---|---|---|
| Bare accessor call sites in the visitor | **8** | `spirv_types.cpp:397` `:399` `:403` `:409` `:411` `:415` `:424` `:428` |
| Distinct optional SPIR-V types returned | **8** | `t_int8_ t_uint8_ t_int16_ t_uint16_ t_int64_ t_uint64_ t_fp16_ t_fp64_`, declared at `spirv_ir_builder.cpp:157 :158 :161 :162 :167 :168 :172 :175` |
| Capability gates | **5** | `spirv_ir_builder.cpp:156 :160 :166 :171 :174`, matching `rhi_constants.inc.h:12 :13 :14 :15 :16` |
| Distinct bit widths among those 8 | **3** | 8 (`i8 u8`), 16 (`i16 u16 f16`), 64 (`i64 u64 f64`) |

**Report A's figures — eight accessors, eight optional types, five capability
gates, three distinct bit widths (`report-02-codegen.md:537-543`) — are all
correct.** So is its ruling at `:546-552` that adversary 02-1's supporting sentence
miscounted: those five names are the capability names, not type names.

**Report B's independent figures are correct too.** Its table at
`report-02b-codegen.md:472-479` lists all eight call sites against their
accessors, guarded equivalents and declarations, and its total at `:482-484` —
"Eight call sites, eight optional types, five capability flags" — reconciles
against the rows. I re-derived every row: `i8_type()` at `spirv_ir_builder.h:559-561`
guarded at `.cpp:301-304`; `i16_type()` `:549-551` / `:305-308`; `i64_type()`
`:529-531` / `:311-314`; `u8_type()` `:562-564` / `:315-318`; `u16_type()`
`:552-554` / `:319-322`; `u64_type()` `:532-534` / `:325-328`; `f16_type()`
`:555-557` / `:291-294`; `f64_type()` `:535-537` / `:297-300`. All exact.

Report B does not state a bit-width figure at all, so it cannot contradict
report A's three. The two reports agree on every number they both state.

**Six has no reading in the source.** Neither eight, nor five, nor three is six.
The planner's figure and adversary 02-1's replacement are both refuted by the
enumeration above.

**Why eight and not twelve.** `i32_type()` `:401`, `u32_type()` `:413`,
`bool_type()` `:407` and `f32_type()` `:426` are bypassed identically but are
safe, because `t_bool_` `:155`, `t_int32_` / `t_uint32_` `:164-165` and
`t_fp32_` `:170` are declared outside any `caps_` test. Both reports have this.

---

## 4. Report B's citation ledger and its resolve claim

### 4.1 The ledger arithmetic reconciles, and every row is real

I opened all twelve corrected sites. Every one is exact:

| # | Corrected to | Verified at source |
|---|---|---|
| 1 | `codegen_llvm.cpp:2273` | `:2272` blank, `:2273` `create_increment(loop_index, block_dim);` |
| 2 | `codegen_llvm.cpp:2214-2216` | `:2213` `builder->SetInsertPoint(loop_test_bb);`, `CreateICmp` `:2214-2216` |
| 3 | `codegen_llvm.cpp:1834-1839`, const at `:1838` | `:1838` `auto offset = tlctx->get_constant(bit_offset);`; `:1841-1843` is the `call_struct_func` branch, no `get_constant` |
| 4 | `codegen_llvm.cpp:1820-1824`, const at `:1822` | `:1822` `get_constant(element_num_bits)`, `:1823` `CreateMul`, `:1824` `create_bit_ptr`, `:1825` `} else {` |
| 5 | `spirv_codegen.cpp:2149` | `:2146` `spirv::Label loop_body = ir_->new_label();`; `:2149` `alloca_variable(ir_->u32_type())` |
| 6 | `spirv_codegen.cpp:2093, 2096` | `:2093` `make_phi`, `:2096` `ir_->lt`; `:2095` `set_incoming`, `:2097` `OpLoopMerge` |
| 7 | `spirv_codegen.cpp:1791, 1823, 1825` | `:1791` `sub(end_, const_i32_one_)`, `:1823` `add`, `:1825` `sub`; `:1821` `spirv::Value next_value;` |
| 8 | `spirv_codegen.cpp:2066-2071` | `:2064` blank, statement runs `:2066-2071` |
| 9 | `spirv_types.cpp:484-514`, pointer branch `:490-498` | function opens `:484`, closes `:514`; branch `:490-498` |
| 10 | `spirv_ir_builder.cpp:334-353`, branch `:337-342` | function opens `:334`, closes `:353`; branch `:337-342` |
| 11 | `runtime.cpp:1334` and `:1336` | `:1335` is `int num_parent_elements = parent_list->size();` — not a subscript |
| 12 | `llvm_codegen_utils.cpp:103-145` | opens `:103`, closes `:145` |

**The arithmetic reconciles against the rows, which is what standing instruction
§10.8 asks.** By finder, from the last column: adversary2 02-1 rows 1, 2, 5, 6,
7, 9, 11, 12 = **8**; adversary2 02-2 rows 1, 3, 6, 7, 9, 10 = **6**; overlap
rows 1, 6, 7, 9 = **4**; this-pass-only rows 4, 8 = **2**. 8 + 6 − 4 + 2 = 12,
and the table has twelve rows. By section: `§3.1` rows 1-4 = 4, `§3.2` rows 5-10
= 6, `§5.1` row 11 = 1, `§6` row 12 = 1; 4 + 6 + 1 + 1 = 12. Both derivations
land on twelve. **Correct.**

### 4.2 The resolve claim, tested mechanically

Report B's previous method statement was false and was struck, so I did not
accept the replacement on its word. I extracted every `path.ext:NNN[-NNN]`
token from `report-02b-codegen.md` with a script, resolved each basename to a
real path in the tree, and checked the named lines against that file's length.

```
distinct (file, start, end) triples: 311
resolved to a real file, lines in range: 311
failed: 0
```

Normalising to basename (the report writes the same file both as
`spirv_codegen.cpp` and as `spirv/spirv_codegen.cpp`) gives **282** distinct
`(basename, start, end)` triples and **263** distinct `(basename, start)` pairs.
Report B's figure of 278 sits between those and within four of the 282 that my
extraction rule produces; the residual is a counting-rule difference, not a
discrepancy I can call a fault.

**The claim as stated holds: every file-qualified citation in report B resolves
and every named line exists.** I ran the same test on `report-02-codegen.md`:
**290 distinct citations, 290 in range, 0 failures.**

**What this does not certify, per standing instruction §10.6.** Resolvability is
not attribution. I therefore also opened, by hand and against the sentence each
supports, thirty-eight citations across both reports that no adversary and no
amendment agent had audited — report A's L1-L6, L8-L20, S3, S4, S11, S15 and
§3.3.7a's whole matrix; report B's `:708`, `:727-729`, `:1999`, `:2004`,
`:2054-2055`, `:2157`, `spirv_ir_builder.cpp:565`, `:576`, `:591`, `:596-602`,
`:605`, `:747`, `:960`, `spirv_ir_builder.h:149-159`,
`snode_struct_compiler.h:16,19,28,31,42`, `offload.cpp:358`. **All thirty-eight
are exact.** The one attribution fault I found is in §7, and it is not a wrong
line number.

---

## 5. The sparsity withdrawal, and the config-flag route

**VERIFIED, and both reports now state it correctly.**

`taichi/runtime/llvm/llvm_runtime_executor.cpp:402` is
`bool all_dense = config_.demote_dense_struct_fors;`. The loop `:403-410`
contains exactly one assignment, `all_dense = false;` at `:407`, inside the
`if` at `:404-406` that tests for a non-`dense`/`place`/`root` node, followed by
`break` at `:408`. **It can only clear.** So with the field false, `all_dense`
is false for every tree shape, the early return at
`runtime/llvm/runtime_module/runtime.cpp:1000-1002` never fires, and the range
store at `:1003-1007` executes on a fully dense tree.

The flag: declared `bool` at `taichi/program/compile_config.h:28`, set true at
`compile_config.cpp:18`, forced true for SPIR-V archs only at
`compile_config.cpp:72-74` inside `CompileConfig::fit()` — which does not reach
this LLVM-only runtime — and writable at `taichi/python/export_lang.cpp:201-202`.

**I grepped every writer tree-wide.** Outside `compile_config.{h,cpp}` the only
assignments are the Python `def_readwrite` at `export_lang.cpp:201-202` and two
test invocations at `tests/python/test_struct_for_intermediate.py:24` and `:29`.
Every reader is a read: `offload.cpp:192`, `compile_to_offloads.cpp:191`,
`offline_cache_util.cpp:46`, `llvm_runtime_executor.cpp:402`. So the flag is a
public field with no in-tree C++ writer.

- **Report B withdraws correctly** at `:1554-1565` ("Revision 2's conclusion
  here was wrong ... the range store at `:1003-1007` executes for EVERY tree"),
  and states the two routes' actual preconditions at `:1569-1575`.
- **Report A corrects correctly** in §2.1, marking the withdrawal explicitly and
  restating that the `element_lists` route "requires **either** a
  non-`dense`/`place`/`root` SNode **or** `demote_dense_struct_fors == false`",
  with the second disjunct needing "no sparsity at all."

**The supporting reachability chain is right in both.** I opened every link:
`free_snode_tree_ids_.push(...)` at `program.cpp:235`,
`allocate_snode_tree_id` at `:559-567` popping at `:563-564`,
`std::atomic<int> SNode::counter{0}` at `snode.cpp:12`, `id = counter++` at
`snode.cpp:220`, the sole reset `SNode::counter = 0;` at `program.cpp:144`,
`TI_ASSERT_INFO(num_instances_ == 0, ...)` at `program.cpp:141`,
`is_gc_able` = `pointer || dynamic` at `snode_types.cpp:21-23`,
`is_gc_able` filter at `llvm_runtime_executor.cpp:447`, the call passing
`root_id` and `(int)snode_metas.size()` at `:442-444`, `root_id` from
`tree->root()->id` at
`taichi/runtime/program_impls/llvm/llvm_program.cpp:61`, the assertion
`TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` at
`taichi/codegen/llvm/struct_llvm.cpp:266`, and `TI_ASSERT` expanding to a plain
`if` plus `TI_ERROR` with no `NDEBUG` guard at `taichi/common/logging.h:100-107`.

**`SNode::reset_counter()` really has no caller.** Grep over `taichi/`,
`c_api/`, `tests/` and `python/` returns three hits total: the definition at
`taichi/ir/snode.h:348`, the unrelated `Stmt::reset_counter` definition at
`taichi/ir/ir.h:509`, and the single call `Stmt::reset_counter();` at
`taichi/program/program.cpp:347`. Both reports have this right.

**`taichi_max_num_snodes` really appears in codegen exactly once.** Tree-wide
grep gives four sites: `taichi/codegen/llvm/struct_llvm.cpp:266` and the three
array declarations at `runtime.cpp:567-569`. Report A's summary claim holds.
`kMaxNumSnodeTreesLlvm` appears only at `constants.h:13` and `runtime.cpp:562-563`,
both indexed by the recycled tree id, so neither report's statement that it does
not bound the SNode-id overflow is contradicted.

---

## 6. The capability setter census — VERIFIED BY INDEPENDENT ENUMERATION

I grepped the five names over `taichi/` and `c_api/` and excluded reads.

| Capability | Setters | Sites |
|---|---|---|
| `spirv_has_int8` | 2 | `metal_device.mm:1046`, `vulkan_device_creator.cpp:790` |
| `spirv_has_int16` | 4 | `metal_device.mm:1047`, `vulkan_device_creator.cpp:628`, `opengl_device.cpp:516`, `:521` |
| `spirv_has_int64` | 4 | `metal_device.mm:1052`, `vulkan_device_creator.cpp:632`, `opengl_device.cpp:511`, `taichi_opengl_impl.cpp:9` |
| `spirv_has_float16` | 4 | `metal_device.mm:1048`, `vulkan_device_creator.cpp:787`, `opengl_device.cpp:517`, `:525` |
| `spirv_has_float64` | 3 | `vulkan_device_creator.cpp:636`, `opengl_device.cpp:512`, `taichi_opengl_impl.cpp:10` |

2 + 4 + 4 + 4 + 3 = **17**, and the rows list seventeen sites. **Report B's
count of 17 and its per-capability split (int8 2, int16 4, int64 4, float16 4,
float64 3) are both correct.** Report A's five-by-seven matrix at
`report-02-codegen.md:825-831` carries the same seventeen sites with the same
gates, and I verified each gate: `shaderInt16` at
`vulkan_device_creator.cpp:626`, `shaderInt64` at `:630`, `shaderFloat64` at
`:634`, `shaderFloat16` at `:786`, `shaderInt8` at `:789`; `!is_gles()` at
`opengl_device.cpp:509`; `GLAD_GL_NV_gpu_shader5` `:515`,
`GLAD_GL_AMD_gpu_shader_int16` `:520`, `GLAD_GL_AMD_gpu_shader_half_float`
`:524`.

**Metal lacks a Float64 setter entirely — confirmed at file level.**
`grep -c float64 taichi/rhi/metal/metal_device.mm` returns **0**. The capability
block is `:1044-1053`: `spirv_version` `:1045`, int8 `:1046`, int16 `:1047`,
float16 `:1048`, subgroup_basic `:1049`, int64 `:1052` under
`if (feature_64_bit_integer_math)` at `:1051`. So an `f64` argument on any Metal
device takes the bare `f64_type()` at `spirv_types.cpp:428` and emits id 0.
Unconditional. **Both reports have this and it is right.**

**DX11 and imported Vulkan lack all five.** `Dx11Device::Dx11Device` sets only
`spirv_version` — `dx_device.cpp:563` `DeviceCapabilityConfig caps{};`, `:564`
`caps.set(DeviceCapability::spirv_version, 0x10300);`, `:565`
`set_caps(std::move(caps));`. Imported Vulkan likewise:
`taichi_vulkan_impl.cpp:35` `caps{}`, `:37-43` the three `spirv_version`
branches, `:46-51` the commented-out physical-storage-buffer set, `:53`
`vk_device.set_caps(std::move(caps));`. **Verified.**

**The subtractive override is real.** `set_caps` is a whole-object move-assign
at `taichi/rhi/public_device.h:855-857`, and
`OpenglRuntime::OpenglRuntime` (`c_api/src/taichi_opengl_impl.cpp:4-13`) builds
a fresh `caps{}` at `:8`, sets int64 `:9`, float64 `:10`, spirv_version `:11`,
and calls `get_gl().set_caps(...)` at `:12`. So it discards whatever
`opengl_device.cpp:515-526` granted. **Both reports have this.**

**`VulkanRuntimeImported::Workaround::Workaround` exists and `Inner` does not.**
Constructor opens at `taichi_vulkan_impl.cpp:19`; `:18` is blank. Report A's
amendment correction is right.

**One attribution shortcut, minor.** Report A's matrix cites Metal's int64 as
"gated on `family_apple3` `:1051`". Line `:1051` is
`if (feature_64_bit_integer_math) {`; the assignment
`bool feature_64_bit_integer_math = family_apple3;` is at `:1038` and is not
cited. The substance is right; a reader opening `:1051` sees the gate but not
the predicate.

---

## 7. NEWLY WRONG — report B's widening enumeration

**This is the one substantive fault surviving into round three.**

`report-02b-codegen.md` states, at `:385` (§3.2 table), `:1096-1099` (§6),
`:1330` (§7.12) and by reference at `:237` and `:1315`:

> "of the four `u32_type()` hardcodes, `:411`, `:412` and `:414` break under
> widening and `:405` is benign."

**Two of those three do not break, and report B's own stated rule is what
refutes them.**

The instruction in question is one `OpShiftRightLogical`:

```
410:    bitmask_word_ptr = ir_->make_value(
411:        spv::OpShiftRightLogical, ir_->u32_type(), bitmask_word_ptr,
412:        ir_->uint_immediate_number(ir_->u32_type(), 2));
```

Against `make_value(spv::Op op, const SType &out_type, Args &&...args)`
(`spirv_ir_builder.h:290-291`): Result Type is `u32_type()` (`:411`), Base is
`bitmask_word_ptr` (`:411`), **Shift is the immediate at `:412`**.

- **`:412` does not break.** It is the Shift operand, structurally identical to
  `:405`, which report B itself exempts. The validator checks the Shift operand
  only for integer type and dimension —
  `external/SPIRV-Tools/source/val/validate_bitwise.cpp:93-97` (type) and
  `:99-102` (dimension) — and **never** for width. The width test at `:88-91`
  applies to Base alone. Report B applies this exemption to `:405` at
  `report-02b-codegen.md:1097-1099` and then withholds it from `:412` in the
  same sentence.
- **`:414` does not break, and must not be widened.**
  `ir_->struct_array_access(ir_->u32_type(), buffer, bitmask_word_ptr)`. In
  `IRBuilder::struct_array_access` (`spirv_ir_builder.cpp:770-790`) the first
  parameter is `res_type`, asserted `TypeKind::kPrimitive`, used to build
  `ptr_type = get_pointer_type(res_type, storage_class)` and emitted as the
  **pointee type** of an `OpAccessChain`. It is the element type of the buffer
  being indexed — and that buffer is
  `get_buffer_value(BufferInfo(BufferType::Root, root_id), PrimitiveType::u32)`
  from `spirv_codegen.cpp:401-402`, a genuine array of 32-bit bitmask words. It
  is not an index width. The index operand is `bitmask_word_ptr`, and
  `OpAccessChain` places **no width constraint** on its indexes. In
  `ValidateAccessChain` (`external/SPIRV-Tools/source/val/validate_memory.cpp:1264`)
  the per-index test is `:1342-1348`: the comment at `:1342` reads "The index
  must be a scalar integer type", and `:1344` rejects only
  `spv::Op::OpTypeInt != index_type->opcode()`, diagnosing "Indexes passed to
  ... must be of type integer" at `:1345-1347`. Width is never consulted. So
  `:414` is correct as written under any pointer width, and a repair that
  replaced its `u32_type()` with a widened pointer type would be a regression.

**Only `:411` breaks**, as the Result Type of the shift against a Base that
carries the widened pointer from `ir_->add(parent_ptr, bitmask_word_ptr)` at
`:409`. Report B's own §6 prose describes exactly that and then over-extends the
line list around it.

**`report-02-codegen.md` does not share this fault.** Its S6a row at `:436`
scopes the site to `:410-412` and names the Result Type at `:411` as the
narrow half, and its §4.2.2 at `:1157-1158` says a widened `make_pointer`
"yields **two invalid instructions**", `:410-412` and `:398-399`. That framing
is right at instruction granularity. Report A's S6 row does list
`:405, :411, :412, :414` as hardcoded `u32_type()` sites, which is true as an
inventory of call sites and is not a claim about invalidity.

**Blast radius.** Report B's escalation 10 and its section 6 are the text a
reader would work from when scoping the SPIR-V half of item 6.2. Acting on the
sentence as written produces three edits where one is needed and one of the
three is actively wrong.

**Grade, per standing instruction §10.7.** Not applicable — this is a reporting
fault, not an obstacle in the codebase.

---

## 8. STILL MISSING — `bitmasked_activation` holds four widening sites, not two

**Neither report has this, neither adversary raised it, and neither amendment
pass looked for it.** It is the same class as S6b, from a different mechanism.

### 8.1 The two sites

```
392:    auto bitmask_word_index =
393:        ir_->make_value(spv::OpShiftRightLogical, ptr_dt, input_index,
394:                        ir_->uint_immediate_number(ptr_dt, 5));
395:    auto bitmask_bit_index =
396:        ir_->make_value(spv::OpBitwiseAnd, ptr_dt, input_index,
397:                        ir_->uint_immediate_number(ptr_dt, 31));
```

`ptr_dt` is `parent_ptr.stype` (`spirv_codegen.cpp:388`), so both Result Types
follow the pointer width. Both immediates follow it too. **The remaining operand
in each, `input_index`, does not.**

- `:392-394` — `OpShiftRightLogical` with Base `input_index`. The Base/Result
  bit-width test is `validate_bitwise.cpp:88-91`.
- `:395-397` — `OpBitwiseAnd`, whose validation arm is
  `validate_bitwise.cpp:106-141` and which requires **every** operand to match
  Result Type's bit width: `:134-138`,
  `if (_.GetBitWidth(type_id) != result_bit_width) ... "Expected operands to
  have the same bit width as Result Type"`. The loop at `:118-119` covers all
  operands from index 2.

### 8.2 Why `input_index` is pinned at 32 bits — the second mechanism

`bitmasked_activation` has two call paths, and **they differ**:

- `visit(SNodeOpStmt*)` (`spirv_codegen.cpp:437-466`) casts first:
  `spirv::Value input_index_val = ir_->cast(parent_val.stype, ir_->query_value(stmt->val->raw_name()));`
  at `:443-444`, then calls at `:448-449`, `:455-456`, `:458-459`. On this path
  the index follows the pointer and `:392-397` stay valid.
- `visit(SNodeLookupStmt*)` (`:468-511`) does **not**:
  `spirv::Value input_index_val = ir_->query_value(stmt->input_index->raw_name());`
  at `:488-489`, called at `:490-491` — the raw i32 linear index, uncast. The
  same function casts the very same value at `:504-505` for the dense-offset
  branch, so the omission at `:488-489` is visible against its own neighbour
  eighteen lines below.

That index is i32 because it comes from `LinearizeStmt`, which both reports
document as i32 (`report-02-codegen.md` S7, `spirv_codegen.cpp:532-541`;
`report-02b-codegen.md:533-536`). **Today ptr_dt is u32 and i32 is the same
width, so the validator's width test passes** — the identical reason S6b is
legal today. Widen `make_pointer` and both become 64-bit Result Types over a
32-bit operand.

### 8.3 What this changes

On the `SNodeLookupStmt` path the function holds **four** widening-invalidity
sites — `:392-394`, `:395-397`, `:398-399`, `:410-412` — of which three are the
S6b direction and one is S6a. On the `SNodeOpStmt` path it holds **two**.
Both reports state two unconditionally:

- `report-02-codegen.md:1157-1161` — "a widened `make_pointer` yields **two
  invalid instructions**", and `:1160-1161` — "A repair that only replaces
  `u32_type()` with the pointer type fixes S6a and leaves S6b broken." The
  remedy sentence is right in kind and short by two.
- `report-02b-codegen.md:1094` — "breaks under widening **in two directions**",
  with the two directions enumerated as `:410-412` and `:398-399`.

Both reports would send a reader to fix two instructions in a function that has
four broken ones on its live SNode-access path. And the second mechanism —
a caller handing in an uncast index — is not addressable by editing
`bitmasked_activation` at all; it is a one-line cast at `:488-489`, or a
decision that the parameter should be typed.

**Grade, per standing instruction §10.7. ARCHITECTURAL.** If every driver,
hardware generation and specification were ideal today, `bitmasked_activation`
would still mix a pointer-width Result Type with 32-bit operands, because that
is a decision inside this codebase about how the bitmask index is represented.
It does not expire.

**Blast radius, stated separately and concretely.** One function,
`taichi/codegen/spirv/spirv_codegen.cpp:383-435`, holding four instructions;
plus one call site, `:488-491`, which is where the uncast index enters. Five
places in one file. That is small — which is the point of grading kind and
radius separately: architectural here is cheap, and the cost of *not* recording
it is that a repair scoped from either report's current text lands on two of
the five.

I am **not** proposing the repair, deciding which of the four a fix should
address, or deciding whether the parameter or the call site is the right place
to cast. Escalated below.

---

## 9. STILL MISSING — report A's `get_buffer_value` inventory is two rows short

`report-02-codegen.md:630-638` (§3.3.5) and `:1337-1341` (escalation 5) both
enumerate the hardcoded-placeholder call sites of `get_buffer_value`:

> `PrimitiveType::i32` at `:609-610`, `:618`, `:651`, `:728`, `:753`, `:788`
> and `:2293`, and `PrimitiveType::u32` at `:401`, `:516` and `:2138`.
> (`:1901` is not a call site; it sits inside the comment block at `:1899-1903`.)

Ten sites. **Grep gives twelve hardcoded ones.** The two omitted are
`spirv_codegen.cpp:2024` and `:2039`, both
`get_buffer_value(BufferType::GlobalTmps, PrimitiveType::i32)`, reading the
non-const range-for bounds out of the global-temporaries buffer.

Full enumeration, from `grep -n get_buffer_value spirv_codegen.cpp`:

| Kind | Sites |
|---|---|
| Hardcoded `PrimitiveType::i32` | `:609-610`, `:618`, `:651`, `:728`, `:753`, `:788`, **`:2024`**, **`:2039`**, `:2293` — 9 |
| Hardcoded `PrimitiveType::u32` | `:401`, `:516`, `:2138` — 3 |
| Forwards a caller-supplied `dt` | `:2212` (`at_buffer`) — 1 |
| Comments, not call sites | `:354`, `:1901` |
| The definition | `:2266` |

**The conclusion survives intact** — `:2024` and `:2039` are hardcoded like the
rest, so "the guard executes, on a type that is always mandatory, and returns
cleanly" still holds, and "the only call that forwards a caller-supplied `dt`
is `at_buffer`'s at `:2212`" is exactly right. But the sentence is presented as
an inventory and it is two rows short of the set it inventories, which is
standing instruction §10.8's failure mode. `:1901` is correctly excluded — I
opened `:1898-1903` and it is inside `/* */`.

Report B does not make the same error, because its claim at `:534-538` is
scoped ("the `dt` supplied by `visit(ArgLoadStmt*)`") rather than exhaustive.
Its `:2276` citation is right: `compile_args_struct();` is at `:2276`, inside
`get_buffer_value`'s `BufferType::Args` branch at `:2275-2281`.

I also confirmed the source itself calls these placeholders: `:650` reads
`// The PrimitiveType::i32 in this function call is a placeholder.`

---

## 10. The 45-file sweeps — BOTH RECONCILE, MECHANICALLY CHECKED

```
find taichi/codegen -type f                                  -> 51
find taichi/codegen -type f \( -name '*.h' -o -name '*.cpp' \) -> 45
the other 6: amdgpu/ cpu/ cuda/ dx12/ llvm/ spirv/ CMakeLists.txt
```

**Report B's 21 + 24.** I transcribed both of its lists —
`report-02b-codegen.md:1511-1518` (21 cited) and the table at `:1529-1554`
(24 opened) — into a file and diffed the union against the `find` output.
**Exact match, 45 = 45, no file in one and not the other, no duplicates.**
Report B's reconciliation is correct and is falsifiable as written, which is
what the round-two objection asked for.

**Report A's accounting — reconciles to 45 on coverage, but the reconciling
sentence does not close.** Its list of files "found irrelevant"
(`report-02-codegen.md:1559-1571`) expands to 24 entries once
`compiled_kernel_data.{h,cpp}` is split and "the four per-backend `codegen_*.h`
headers" is expanded. All 24 are real files. The remainder is therefore 21, and
the sentence at `:1569-1572` claims those are covered by "§3.1, §3.2 and §3.3".

**My first test of that claim was scoped wrong and gave a false pass, and
`adversary3-02-1.md` §7.3 has it right.** I searched lines 347-1034, which
includes §3.4; the three sections the sentence actually names run **:364-894**
(`### 3.1` at `:364`, `### 3.4` at `:895`). Re-run against the correct window,
**twenty of the twenty-one appear; `llvm/llvm_codegen_utils.cpp` does not.** Its
first occurrence in report A is `:923`, inside §3.4's citation-audit table, then
`:1093` in §4.1.9 and `:1277` in §4.3. So the union of the sentence's two halves
is 44, not 45.

The file **is** covered and its finding is real (`check_func_call_signature`,
`llvm_codegen_utils.cpp:103-145`, which I opened and confirmed as ledger row 12
in §4.1 above). What fails is the statement of how the coverage closes — the
§10.8 failure mode exactly. Naming §4.1.9 alongside the other three sections
fixes it. Blast radius: one sentence, `report-02-codegen.md:1569-1572`.

**One observation, not a fault.** Both reports land on a 21/24 split, but the
two partitions are **different sets**. They disagree on eight files, four each
way: report A puts `codegen_utils.h`, the top-level
`compiled_kernel_data.{h,cpp}` and `llvm/llvm_codegen_utils.h` in its
"irrelevant" 24, while report B counts them among its cited 21; report B puts
`llvm/codegen_llvm_quant.cpp`, `spirv/snode_struct_compiler.cpp`,
`spirv/spirv_codegen.h` and `spirv/spirv_types.h` in its "opened" 24, while
report A cites them in section 3. The criteria differ — A's split is
"irrelevant to width/index" versus "cited in §3", B's is "cited as a finding"
versus "opened and reported" — so the two are not in conflict, and neither
claims agreement with the other. Recorded so the planner does not read the
matching totals as convergence.

**Report B's three sweep findings, all opened.**
`spirv_types.h:71-91` — `PhysicalPointerType` derives from
`IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75` with no gate;
`snode_struct_compiler.cpp:34-63` — `construct` builds it at `:53`, and
`compute_snode_size` is `:65-...`; `lib_tiny_ir.h` — the layout interface is
`size_t` throughout. All three are as reported.

**One imprecision in report B's dead-code claim.** `report-02b-codegen.md:1551`
says of `construct`: "Its only call is at `:18`, inside a `/* */` block opened
at `:16` and closed at `:19`." There is a **second** call, the recursive
`construct(ir_module, ch.get())` at `snode_struct_compiler.cpp:44`. It is
reachable only from `:18`, so the conclusion — unreachable in the current tree —
is right; the word "only" is not. Report A avoids this by citing the
construction site of `PhysicalPointerType` (`:53`) rather than the call graph
of `construct`. Report A's own span for the class, `spirv_types.h:71-77`
(`report-02-codegen.md:733`), stops at the constructor body; the class runs
`:71-91`. Its derivation citation `:75` is exact.

---

## 11. Items I checked and found nothing wrong with

Recorded so the planner can see the ground that was covered and did not produce
an objection. Manufacturing one here would be the failure the brief names.

- **Report A's LLVM inventory L1-L24.** Opened L1 (`struct_llvm.cpp:153`), L2
  (`:171-188`, including `acc_shape * shape` as a host `int` multiply at
  `:174-175` feeding `CreateURem` at `:179`, and `snode.h:41/45/49` all `int`),
  L3 (`:111-113`), L4 (`:74-75`), L5 (`codegen_llvm.cpp:1736-1743`), L6
  (`:1755` with the struct comment `:1750-1754`), L7 (`:1792-1829`), L8
  (`:1927`), L9 (`:1931`, `:1939`, `:1942`, `:1944`), L10 (`:1988`), L11
  (`:1857-1861`), L12 (`:2157-2158`), L13 (`:2113-2119`, args at `:2117-2118`),
  L14 (`:2290-2291`, emitted at `:2309`, bounded by `constants.h:28`), L15
  (`:2336`, `:2339`), L16 (`:2360-2366`, confirmed at `runtime.cpp:288-290`),
  L19 (`:278`, matching `runtime.cpp:308`), L20 (`:284-292`, confirmed live at
  `runtime.cpp:312-322` and `node_dense.h:10,18,22`). **Every one exact.**
- **Report A's `get_constant` spans**, both corrected in the amendment pass:
  `llvm_context.cpp:727-749` (template at `:727`, close at `:749`, `:750`
  blank) and `:695-718`. Exact.
- **Report A's SPIR-V spans S1-S17.** S1 `:82`, S2 `:2317-2324`, S3 `:355`, S4
  `:371-372`, S5 `:504-508`, S7 `:532-541`, S8 `:2214-2218`, S9 `:707-711`, S10
  `:773-777`, S11 `:321-325`, S14 `:565-608`, S15 `:164-165` and `:223-224`
  (and its correction that round one's `:283-284` is inside `get_null_type`
  `:282-286` — confirmed), S16 `:337-342` and `:490-498`, S17
  `kernel_utils.h:99`, `:110-111` against `spirv_codegen.cpp:1999`, `:2004`.
  **Every one exact.**
- **Report A's §4.2 point 5 withdrawal.** `at_buffer` is `:2194-2220`, the `u64`
  test `:2197`, the branch **returns at `:2204`**, the shift `:2214-2216` is
  below it. Withdrawing point 5 is correct. The without-Int64 sub-case is right
  too: `TI_ERROR_IF(!is_integral(ptr_val.stype.dt), ...)` at `:2207-2210` fires,
  and `is_integral` (`taichi/ir/type_utils.h`) has no arm for `unknown`.
- **Report A's escalation 8.** `visit(LoopIndexStmt*)` is `:543-563` with
  `TI_NOT_IMPLEMENTED` at `:553`; `"ii"` is registered for struct-for at
  `:2171`; `demote_dense_struct_fors` is forced true for SPIR-V archs at
  `compile_config.cpp:72-74` and the pass applied at
  `compile_to_offloads.cpp:191-192`. All exact.
- **Report B's SPIR-V table rows** listed in §4.2 above, plus
  `spirv_ir_builder.cpp:576` (array length as u32 immediate), `:586` (`uint32_t
  nbytes`), `:591` (assigned from `size_t container_stride` — implicit
  narrowing), `:596-602` (the only diagnostic, `nbytes == 0`), `:605`
  (`DecorationArrayStride`), `:747` (`get_struct_array_type(value_type, 0)`),
  `:581-583` (`OpTypeRuntimeArray`), `:960` (`t_uint32_` return),
  `spirv_ir_builder.h:149-159` (the single `ADD(uint32_t, v)` at `:158`),
  `snode_struct_compiler.h:16,19,28,31,42` (all `size_t`),
  `offload.cpp:358` (the `taichi_global_tmp_buffer_size` assertion). **Every one
  exact.**
- **`ti_set_runtime_capabilities_ext`**, which both reports now carry:
  `c_api/src/taichi_core_impl.cpp:317-334`, loop at `:326-330`, no enum
  validation and no device query, `set_caps` at `:331`. Both citations right.
- **`spirv_has_physical_storage_buffer`.** One setter tree-wide at
  `vulkan_device_creator.cpp:826`, under `if (device_supported_features.shaderInt64)`
  at `:821`, behind `#if !defined(__APPLE__) && false` at `:825`, `#endif` at
  `:827`; `:822-824` are comments. Report A's corrected framing is exact.
- **Report B's ten "did not decide unnecessary" items and its 7 / 3 escalation
  split** (`notes-02b-codegen.md` §41). I counted the items: ten. I checked the
  seven named escalations against report B's own list — 3 `use_64bit_pointers`,
  2 `&& false`, 11 `Extension::data64`, 12 C API OpenGL override, 18
  `ti_set_runtime_capabilities_ext`, 15 validator, 16 `success` — and the three
  without one. Reconciles.

---

## 12. Escalations

Unresolved judgement, per the brief. I decide none of these.

1. **The `bitmasked_activation` widening count and its remedy (§8).** Four
   invalid instructions on the `SNodeLookupStmt` path, two on the
   `SNodeOpStmt` path. Whether the fix belongs in the function (typing the
   `input_index` parameter) or at the call site (`spirv_codegen.cpp:488-489`,
   adding the cast its own neighbour at `:504-505` already performs) is a
   design decision about that backend's value model, and section 8.2 item 0 of
   the plan already records that the SPIR-V value model is where the
   pointer-width collision lives. I am not choosing.

2. **Whether `:414` should be left alone (§7).** My reading is that
   `struct_array_access`'s first argument is the buffer's element type and must
   stay `u32` because the bitmask buffer holds 32-bit words. If the planner
   disagrees, the point to test is
   `IRBuilder::struct_array_access` at `spirv_ir_builder.cpp:770-790`, where
   `res_type` becomes the pointee of the emitted `OpAccessChain`. Both reports
   should say the same thing about it and currently do not.

3. **Whether report B's `:412` / `:414` sentence needs a fourth revision pass,
   or whether the planner records the correction here and closes.** The fault is
   one sentence repeated at four places (`report-02b-codegen.md:385`, `:1099`,
   `:1330`, and by reference at `:237` and `:1315`). It is not mine to amend.

4. **Whether the two reports' differing 21/24 partitions (§10) matter.** They
   reconcile to the same 45 by different criteria. If the planner wants the two
   sweeps comparable rather than merely both complete, the criteria have to be
   made the same, and that is a instruction to the reports, not a finding.

---

## 13. What I did not do

- Modified no file but this one. No source file, no report, no notes file, not
  the project plan, not either adversary file.
- Decided nothing unnecessary. Proposed no removal, no fix, no abstraction, and
  no capability wiring. The four `bitmasked_activation` sites in §8 are recorded
  as facts about what the validator in this tree would reject under a widened
  pointer; I do not say what should change.
- Built and ran nothing. Every SPIRV-Tools link in §2 and every validator rule
  in §7 and §8 is read from the checked-out submodule, line by line. I have not
  observed a driver reject an id-0 module and I make no claim about one.
- Did not resolve which of `:392-394`, `:395-397`, `:398-399` and `:410-412`
  a widening repair should address, whether report B's `:412`/`:414` sentence
  needs a revision round, or whether Metal's absent Float64, the disabled
  validator or the dead `success` flag are defects. Those are escalations 1-3
  here and escalations 13-16 (report A) / 15-17 (report B) there.

---

*Written after `adversary3-02-1.md` was checked for and found absent at the
time of writing. If it lands, the divergence section belongs below this line.*

---

# 14. Divergence from `adversary3-02-1.md`

`adversary3-02-1.md` landed after this file was written. Read in full. We agree
on all seven verification items and on the source behind them; every line it
reproduces that I also opened matches. We diverge on **one verdict**, on **one
finding I got wrong**, and on **three findings held by only one of us**.

## 14.1 The verdicts

| | 3-1 | 3-2 (this file) |
|---|---|---|
| Correct | **yes**, both reports | **no**, one substantive fault in report B |
| Complete | **not yet, narrowly** — four residuals, all citation- or accounting-level | **no** — one missing pair of source sites plus one short inventory |

**The source supports me on `correct`, and 3-1 did not test the point.**
3-1's section 8 has no entry on widening validity at all; the topic does not
appear in its file. Its "correct: yes" therefore rests on the seven briefed
claims, all of which I also uphold. It is not a contrary finding — it is an
untested area, and my §7 is inside it.

The evidence, which is decidable from this tree:
`report-02b-codegen.md` says at `:385`, `:1099` and `:1330` that of the four
`u32_type()` hardcodes in `bitmasked_activation`, "`:411`, `:412` and `:414`
break under widening and `:405` is benign."

- `:412` is the **Shift** operand of the `OpShiftRightLogical` whose Result Type
  is at `:411`. `external/SPIRV-Tools/source/val/validate_bitwise.cpp` tests the
  Shift operand only for integer type (`:93-97`) and dimension (`:99-102`); the
  bit-width test at `:88-91` applies to **Base**. This is the identical
  exemption report B itself grants `:405` at `:1097-1099`.
- `:414` is `struct_array_access`'s `res_type`, which
  `spirv_ir_builder.cpp:770-790` turns into the **pointee** type of an
  `OpAccessChain` — the element type of a buffer that genuinely holds 32-bit
  bitmask words (`spirv_codegen.cpp:401-402` requests it as
  `PrimitiveType::u32`). `ValidateAccessChain`
  (`validate_memory.cpp:1264`) tests each index only for
  `spv::Op::OpTypeInt` at `:1344`, never for width.

Only `:411` breaks. Two of the three named do not, and one of them must not be
changed. That is a claim about the code, repeated at three places, and it is
wrong — so `correct` cannot be **yes** for report B on my reading. Whether that
rises to blocking consensus is escalation 3 in my §12 and escalation 1 in 3-1's
§9; neither of us decides it.

## 14.2 Where 3-1 is right and I was wrong

**3-1 §7.3, report A's sweep-reconciling sentence.** 3-1 found that the
sentence at `report-02-codegen.md:1569-1572` names §3.1, §3.2 and §3.3, and that
`llvm/llvm_codegen_utils.cpp` — one of the 21 remainder files — appears in none
of them, first occurring at `:923` in §3.4. **I ran the same test with the wrong
window** (lines 347-1034, which swallows §3.4) and reported a false pass. I
re-ran it against `:364-894`, the correct span, and 3-1 is right: twenty of
twenty-one. My §10 is corrected in place and now credits 3-1.

**The source supports 3-1.** `### 3.1` opens at `report-02-codegen.md:364` and
`### 3.4` at `:895`; `grep -n llvm_codegen_utils.cpp report-02-codegen.md`
returns `:923`, `:1093`, `:1277` and nothing below `:895`.

## 14.3 Findings only 3-1 has, which I confirm

Both opened at source after reading its file.

- **3-1 §8.3 — report B carries two superseded spans under a `[V]` mark.**
  Confirmed. `report-02b-codegen.md:121` cites `spirv_ir_builder.cpp:334-341`
  and `:244-245` cites both `:334-341` and `spirv_types.cpp:484-497`; the block
  is closed "All **[V]**." at `:248`. Report B's own ledger rows 9 and 10
  (`:1592-1593`) correct those two spans to `:334-353` and `:484-514`, and its
  §3.2 and §4.6 carry the correction while these two do not. **I missed this,
  and it is the same shape as the `:2464`/`:2511` fault claim 1 was about — a
  correction landing in some places and not all.** The conclusion those two
  citations support is unaffected: I opened both functions in my §4.1 and the
  32-bit branches are `spirv_ir_builder.cpp:341` and `spirv_types.cpp:495-496`.
- **3-1 §8.4 — two open-ended spans survive.** Confirmed. Report A's S16 row at
  `report-02-codegen.md:446` still writes "function `spirv_types.cpp:484-...`"
  in a row labelled "**Spans corrected.**", while the same report gives the
  closed `:484-514` at `:595`. Report B's §9.2 row at `:1553` writes
  `construct` as `snode_struct_compiler.cpp:34-...`; the function is `:34-63`,
  which report A closes at `:736`. I flagged report B's `:1553` under a
  different heading in my §10 (its "only call is at `:18`" wording) and did not
  flag the ellipsis; 3-1's framing is the better one and both belong.

## 14.4 Findings only I have, which 3-1 did not reach

- **§8, the two further widening-invalidity sites.**
  `spirv_codegen.cpp:392-394` (`OpShiftRightLogical`) and `:395-397`
  (`OpBitwiseAnd`), both with Result Type `ptr_dt` over an `input_index` that is
  the raw i32 linear index on the `SNodeLookupStmt` call path (`:488-489`,
  uncast, against the same function's own cast of the same value at `:504-505`).
  Governed by `validate_bitwise.cpp:88-91` and `:134-138` respectively. 3-1's
  file does not address widening validity anywhere, so this is untested rather
  than disputed. **Graded ARCHITECTURAL with a five-site blast radius in my §8.**
- **§7, the `:412` / `:414` fault**, per §14.1.
- **§0, the brief's figure of 342.** Report B claims 278. 3-1's §4 also tests
  the resolution claim and reports its own extraction count; neither of us finds
  342 anywhere in report B as a citation total.

## 14.5 Where we independently converge

Recorded because independent convergence is worth more than either finding
alone.

- **Report A's `get_buffer_value` census omits `:2024` and `:2039`.** 3-1 §8.2,
  this file §9. We reached it separately, printed the same grep, and drew the
  same conclusion: twelve hardcoded sites plus one forwarder at `:2212`, against
  the report's ten; the load-bearing sentence survives, the census does not.
  3-1 additionally notes the heading/list mismatch ("Args- and Rets-buffer call
  site" against a list already containing Root, GlobalTmps and ListGen sites),
  which I record too.
- **Report B's 21 + 24 partition is exact**, tested mechanically by both of us
  against a directory walk, with the same result: union 45, intersection 0, no
  phantoms, no gaps.
- **The severity chain holds at every link**, and both of us traced it against
  the checked-out submodule rather than either amendment agent's account.
- **The counts are 8 / 8 / 5 / 3**, both derived by enumeration, both rejecting
  six.
- **The setter census is 17**, both derived by grep and per-capability split.
- **`SNode::reset_counter()` has no caller**, and **`taichi_max_num_snodes`
  appears in codegen exactly once.**

## 14.6 The combined residue, for the planner

Taking both files together, six items survive. I do not decide which block
consensus.

| # | Where | What | Found by |
|---|---|---|---|
| 1 | `report-02b-codegen.md:385`, `:1099`, `:1330` | `:412` and `:414` do not break under widening; only `:411` does, and `:414` must not be changed | 3-2 |
| 2 | `spirv_codegen.cpp:392-397` + call site `:488-489` | Two further widening-invalidity sites, absent from both reports | 3-2 |
| 3 | `report-02-codegen.md:630-638`, `:1337-1341` | `get_buffer_value` census two rows short | both |
| 4 | `report-02-codegen.md:1569-1572` | Reconciling sentence names three sections that cover 20 of 21 | 3-1 |
| 5 | `report-02b-codegen.md:121`, `:244-245` | Two ledger-superseded spans carried under `[V]` | 3-1 |
| 6 | `report-02-codegen.md:446`, `report-02b-codegen.md:1553` | Open-ended spans in rows announcing corrections | 3-1 |

Items 1 and 2 are claims about the code and change what a repair would touch.
Items 3-6 are citation- and accounting-level and change no conclusion.
