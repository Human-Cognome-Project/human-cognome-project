# Adversary 3, pass 1 — territory 02, code generation

Round three. Charge: determine whether general consensus is now reached on
`report-02-codegen.md` (report A) and `report-02b-codegen.md` (report B), as two
separate judgements — correct, and complete.

Read in full before opening any source: both reports, `adversary2-02-1.md`,
`adversary2-02-2.md`, and the amendment entries at `notes-02-codegen.md:1163-1534`
(§§36-45) and `notes-02b-codegen.md:1094-1450` (§§32-41).

Every one of the seven claims put to me was treated as a claim and opened at
source. Nothing below is taken from either amendment agent's account. Where I
reproduce a line, I printed it from the file. Section 10 item 8 is applied to my
own numbers: every count here is derived from an enumeration printed in this
document or produced by a script whose method is stated.

**Verdicts up front.**

- **CORRECT: yes, for both reports.** All seven claims are upheld at source.
  Nothing substantive in either report is wrong on the evidence I opened.
- **COMPLETE: not yet, narrowly.** Four residual defects survive, all of them
  citation-level or accounting-level, none of them changing a conclusion. Three
  are in report A, one is in report B. They are listed in section 8 with lines.
  Consensus is one mechanical pass away, and that pass is smaller than round
  two's residue.

---

## 1. Claim 1 — the two struct-compiling functions were swapped, and are now corrected

**UPHELD. Both amendment agents are right, and the correction landed everywhere
the attribution appears in either report.**

I did not take either agent's grep. I extracted the function openings, closing
braces and every `ir_translate_to_spirv` and `translate_ti_type` call in
`spirv_codegen.cpp:2320-2530` in one pass:

```
2326:  void compile_args_struct() {
2344:              auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2358:      auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2386:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2387:    args_struct_type_.id = ir2spirv_map[struct_type];
2400:  }
2402:  spirv::Value compile_argpack_struct(const std::vector<int> &arg_id,
2420:              auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2436:      auto spirv_type = translate_ti_type(blk, type, has_buffer_ptr);
2464:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2465:    argpack_struct_type.id = ir2spirv_map[struct_type];
2477:    argpack_types_[arg_id] = argpack_struct_type;
2481:  }
2483:  void compile_ret_struct() {
2494:          translate_ti_type(blk, element.type, has_buffer_ptr));
2511:        ir_translate_to_spirv(reduced_blk.get(), layout_ctx, ir_.get());
2512:    ret_struct_type_.id = ir2spirv_map[struct_type];
2514:    rets_struct_types_.resize(element_types.size());
2527:  }
```

So `:2386` is args, `:2464` is **argpack**, `:2511` is **rets**. The three
functions are `:2326-2400`, `:2402-2481`, `:2483-2527`. The five
`translate_ti_type` sites partition args `:2344, :2358`, argpack `:2420, :2436`,
ret `:2494`.

**Did the correction land everywhere?** I grepped both reports for `2464`,
`2511`, `2386`, `argpack`, and each function name, and read every hit:

- Report A: `report-02-codegen.md:42-43` (header), `:581-583` (the boundary
  table), `:585-588` (the corroborating statements), `:597-598` (the
  `translate_ti_type` partition), `:1015` (adjudication row), `:1357-1359`
  (escalation 5), `:1481` (section 6 verification index). Seven places, all
  correct, no stale pairing anywhere.
- Report B: `report-02b-codegen.md:82-83` (header), `:500-505` (the three call
  sites with their enclosing functions), `:510-513` (the self-refutation
  recorded), `:522-523` (the partition), `:1281-1291` (adjudication §7.10),
  `:1609` (the ledger's non-citation-fault list). Six places, all correct.

No occurrence of the old pairing survives in either report. The claim is fully
discharged.

---

## 2. Claim 2 — the severity chain, end to end

**UPHELD at every link, and I verified two links neither amendment agent
traced.** The chain is real and it holds.

Both amendment agents describe the same chain and both are right. Because the
planner records this as reported three times and corrected twice, I opened every
file rather than checking their line numbers:

1. **The optimiser runs unconditionally.** `spirv_codegen.cpp:2740` copies the
   binary into `optimized_spv`; `:2745-2748` calls `spirv_opt_->Run(...)` inside
   a `TI_WARN_IF`. The `if (params.enable_spv_opt)` block opens at `:2683` and
   closes at `:2709`, and contains only `RegisterPass` calls. `Run` is outside
   it. **Confirmed.**
2. **The validator is off, unconditionally.** `:2710` is
   `spirv_opt_options_.set_run_validator(false);`, one line below that block's
   closing brace. **Confirmed.**
3. **`Optimizer::Run` skips the validator and then parses anyway.**
   `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598`. The guard is
   `if (opt_options->run_validator_ && !tools.Validate(...)) return false;` at
   `:590-594`, so with the flag false `Validate` is never called. `BuildModule`
   at `:596-597` is **unconditional**, and `:598` is
   `if (context == nullptr) return false;`. **Confirmed.**
4. **`BuildModule` parses and returns null on any non-success.**
   `build_module.cpp:56-75`; `spvBinaryParse` at `:68-69`;
   `return status == SPV_SUCCESS ? std::move(irContext) : nullptr;` at `:74`.
   **Confirmed.**
5. **The parser rejects a zero id.** `binary.cpp:449-450` "Error: Type Id is 0";
   `:455-456` "Error: Result Id is 0"; `:471-473`
   `case SPV_OPERAND_TYPE_ID: case SPV_OPERAND_TYPE_OPTIONAL_ID: if (!word) return diagnostic(SPV_ERROR_INVALID_ID) << "Id is 0";`
   — which is what an `OpTypeStruct` member operand is. **Confirmed.**
6. **The level stays ERROR.** `diagnostic.cpp:87-114`. `:89` is
   `auto level = SPV_MSG_ERROR;`. The switch `:90-108` lowers it only for
   `SPV_SUCCESS`/`SPV_REQUESTED_TERMINATION` (`:91-94`), `SPV_WARNING`
   (`:95-97`), three internal codes (`:98-102`) and `SPV_ERROR_OUT_OF_MEMORY`
   (`:103-104`). `SPV_ERROR_INVALID_ID` falls to `default: break;` at
   `:106-107`. `:112` calls the consumer. **Confirmed.**
7. **The enum order is the mechanism.**
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-95`: FATAL 0,
   INTERNAL_ERROR 1, ERROR 2, WARNING 3, INFO 4, DEBUG 5. **Confirmed.**
8. **The downgrade.** `spriv_message_consumer` is `spirv_codegen.cpp:2641-2659`,
   installed at `:2682`. `:2646` is `if (level <= SPV_MSG_FATAL)` → `TI_ERROR`;
   `:2649` is `else if (level <= SPV_MSG_WARNING)` → `TI_WARN`. Level 2 fails
   the first test and lands in the second. Both amendment agents are right that
   the **branch order**, not the severity assignment, is what downgrades it.
   **Confirmed.**
9. **The success flag is dead.** `success` declared `:2742`, set false `:2750`,
   read at exactly one place, `:2760`, inside `if constexpr (false)` opened at
   `:2758` and closed at `:2772`. `:2775` is
   `generated_spirv.push_back(std::move(optimized_spv));`, outside that block.
   **Confirmed.** Report A's note §39 point 9 additionally checks that
   `optimized_spv` still holds the unmodified copy on a failed `Run`, because
   `optimizer.cpp` reaches `optimized_binary->clear()` only at `:642`, after the
   `return false` at `:598`. I confirmed `:642` is downstream of `:598` in the
   same function.

**Two links neither amendment agent traced, which I opened because the chain
depends on them.** Both hold, so the chain is stronger than either agent showed:

- **The parser's diagnostic actually reaches Taichi's consumer.** The `diagnostic`
  helper in `binary.cpp:145-148` constructs
  `spvtools::DiagnosticStream({0, 0, _.instruction_count}, consumer_, "", error)`,
  so `consumer_` is the parser state's consumer, which `BuildModule` installs at
  `build_module.cpp:62` via `SetContextMessageConsumer(context, consumer)`.
- **That consumer is Taichi's.** `optimizer.cpp:596-597` passes `consumer()`,
  and `Optimizer::consumer()` at `optimizer.cpp:80-82` returns
  `impl_->pass_manager.consumer()`, which is what
  `spirv_opt_->SetMessageConsumer(spriv_message_consumer)` at
  `spirv_codegen.cpp:2682` set. Without these two links the chain would end at
  SPIRV-Tools' default consumer and produce no Taichi diagnostic at all.

**Adjudication against round two.** Adversary2 02-2 is right end to end.
Adversary2 02-1's §2.1 point 3, "Nothing in Taichi looks at it. So there is no
Taichi-level diagnostic", is false: two `TI_WARN`s fire, one from
`diagnostic.cpp:112` through the consumer at `spirv_codegen.cpp:2650` and one
from the `TI_WARN_IF` at `:2745-2748`. Its point 4, that the failure is
"deferred to the driver at shader-module creation", is false as a statement
about where the failure is first detected; it is detected in-process. Both
reports correctly declined to add that sentence — report A says so at
`report-02-codegen.md:1512-1518`, report B at `report-02b-codegen.md` §7 and in
its note §38.

**What survives of adversary2 02-1's position, and both reports keep it
correctly.** The module ships at `:2775` regardless, so a driver does eventually
see it; and neither report claims to know what a driver does with it. Report A
states that limit explicitly and escalates the rest.

**One consequence report A draws that I checked and agree with, and report B does
not draw.** The accidental net catches only what the **parser** rejects, which is
structural. A width mismatch of the S6a/S6b kind is a **validator** check —
`external/SPIRV-Tools/source/val/validate_bitwise.cpp:88-91` — and the validator
is exactly what is disabled. I printed `validate_bitwise.cpp:60-104` and confirm
the shift rule tests Base against Result Type for **bit width** (`:88-91`) and
dimension (`:83-86`), and tests the Shift operand only for type (`:93-97`) and
dimension (`:99-102`), never width. So report A's §4.2.10 is right that width
faults ship with no diagnostic at all, and its refusal to flag
`spirv_codegen.cpp:404-405` follows correctly from the same reading.

---

## 3. Claim 3 — the counts, settled by enumeration

**UPHELD. Report A's four figures are exact. Report B's are exact. The planner's
figure of six has no reading in the source, and adversary2 02-1's replacement
"five optional types" is also wrong.**

I enumerated rather than adjudicating between stated numbers, per section 10
item 8. Two sources printed in full.

**Capability gates — `spirv_ir_builder.cpp:149-176`:**

```
156:  if (caps_->get(cap::spirv_has_int8)) {
157:    t_int8_  = declare_primitive_type(get_data_type<int8>());
158:    t_uint8_ = declare_primitive_type(get_data_type<uint8>());
160:  if (caps_->get(cap::spirv_has_int16)) {
161:    t_int16_  = ...
162:    t_uint16_ = ...
166:  if (caps_->get(cap::spirv_has_int64)) {
167:    t_int64_  = ...
168:    t_uint64_ = ...
171:  if (caps_->get(cap::spirv_has_float16)) {
172:    t_fp16_ = declare_primitive_type(PrimitiveType::f16);
174:  if (caps_->get(cap::spirv_has_float64)) {
175:    t_fp64_ = declare_primitive_type(get_data_type<float64>());
```

**Five** gates: `:156`, `:160`, `:166`, `:171`, `:174`. They match
`taichi/inc/rhi_constants.inc.h:12-16` exactly, which I printed:
`spirv_has_int8`, `spirv_has_int16`, `spirv_has_int64`, `spirv_has_float16`,
`spirv_has_float64`. The `spirv_has_non_semantic_info` gate at `:151` is not a
scalar-type capability and is correctly excluded by both reports.

**Optional types declared under those gates:** `t_int8_`, `t_uint8_`, `t_int16_`,
`t_uint16_`, `t_int64_`, `t_uint64_`, `t_fp16_`, `t_fp64_` — **eight**, each with
its own `declare_primitive_type` call.

**Unconditional declarations, which is why the count is eight and not twelve:**
`t_bool_` `:155`, `t_int32_` `:164`, `t_uint32_` `:165`, `t_fp32_` `:170`.

**Bypassed accessor call sites — `spirv_types.cpp:393-431`:** `i8_type()` `:397`,
`i16_type()` `:399`, `i64_type()` `:403`, `u8_type()` `:409`, `u16_type()` `:411`,
`u64_type()` `:415`, `f16_type()` `:424`, `f64_type()` `:428` — **eight**. The
other four in those two visitors (`i32_type()` `:401`, `bool_type()` `:407`,
`u32_type()` `:413`, `f32_type()` `:426`) are bypassed identically and are safe
only because their members are declared unconditionally.

**Distinct bit widths across the eight optional types:** 8, 16, 64 — **three**.
(f16 is 16, f64 is 64; no fourth width appears.)

So: **8 accessors, 8 optional types, 5 capability gates, 3 distinct bit widths.**

- Report A states exactly that at `report-02-codegen.md:53-55`, `:539-540`,
  `:1017`. **Correct.**
- Report B states "Eight call sites, eight optional types, five capability
  flags" at `report-02b-codegen.md:482`. **Correct.** B states no bit-width
  figure, which is the right choice given that "widths" was the word that
  produced the error.
- Report A is right to refuse adversary2 02-1's supporting sentence, which
  called five "the count of optional *types*". Five is the count of
  **capability names**. Report A says so at `:543-550`.
- Both reports fully enumerate the eight accessors with their definition lines
  in the body — report A at `:508-517`, report B at `:473-479` — so the counts
  are reconcilable against a printed list in both, which is what item 8 asks.

I also checked the accessor definitions myself:
`spirv_ir_builder.h` defines twelve accessors at `:529` (i64), `:532` (u64),
`:535` (f64), `:539` (i32), `:542` (u32), `:545` (f32), `:549` (i16), `:552`
(u16), `:555` (f16), `:559` (i8), `:562` (u8), `:566` (bool). Eight optional,
four mandatory. Consistent with both reports.

---

## 4. Claim 4 — report B's citation ledger, and the resolution claim

### 4.1 The ledger arithmetic reconciles, and every row is real

Report B's ledger is `report-02b-codegen.md:1570-1622`. I counted the rows and
re-derived every figure:

| Figure | Derivation | Holds? |
|---|---|---|
| Twelve faults | Rows 1-12 of the table | Yes |
| By section: 4 + 6 + 1 + 1 = 12 | §3.1 rows 1-4; §3.2 rows 5-10; §5.1 row 11; §6 row 12 | Yes |
| Adversary2 02-1 found eight | Rows 1, 2, 5, 6, 7, 9, 11, 12 | Yes, eight rows |
| Adversary2 02-2 found six | Rows 1, 3, 6, 7, 9, 10 | Yes, six rows |
| Overlap four | Rows 1, 6, 7, 9 | Yes, and these are exactly the intersection |
| This pass alone two | Rows 4, 8 | Yes |
| 8 + 6 − 4 + 2 = 12 | Arithmetic | Yes |

Both totals reconcile against the rows beneath them, which is what section 10
item 8 requires and what round two found missing.

**I opened every corrected line rather than a sample, because the ledger is the
document that claims the audit happened.** All twelve corrections are exact:

- Row 1: `codegen_llvm.cpp:2272` is blank; `:2273` is
  `create_increment(loop_index, block_dim);`. **Correct.**
- Row 2: `:2213` is `builder->SetInsertPoint(loop_test_bb);`; the `CreateICmp`
  runs `:2214-2216`. **Correct.**
- Row 3: `:1834` opens the bit-pointer branch, `get_constant(bit_offset)` is at
  `:1838`, `:1839` is the `create_bit_ptr`; `:1840` is `} else {` and
  `:1841-1843` is the `call_struct_func` branch with no `get_constant`.
  **Correct.**
- Row 4: `:1820-1821` reads `element_num_bits`, `get_constant` is `:1822`, the
  multiply `:1823`, `create_bit_ptr` `:1824`, and `:1825` is `} else {`.
  **Correct.**
- Row 5: `:2146` is `spirv::Label loop_body = ir_->new_label();` — a label, no
  `u32_type()`; `:2149` is
  `auto loop_index_var = ir_->alloca_variable(ir_->u32_type());`. **Correct.**
- Row 6: `:2093` is the `make_phi`, `:2096` is `ir_->lt(loop_var, end_)`.
  **Correct.**
- Row 7: `:1791`, `:1823`, `:1825` carry `const_i32_one_`; `:1821` is
  `spirv::Value next_value;`. **Correct.**
- Row 8: `:2064` is blank, `:2065` a comment, and the `total_invocs` statement
  runs `:2066-2071`. **Correct.**
- Row 9: `translate_ti_type` is `:484-514`, pointer branch `:490-498` with the
  64-bit arm at `:492-493` and the 32-bit arm at `:495-496`. **Correct.**
- Row 10: `from_taichi_type` is `:334-353`, pointer branch `:337-342`.
  **Correct.**
- Row 11: `runtime.cpp:1335` is `int num_parent_elements = parent_list->size();`
  — not a subscript; `:1334` and `:1336` are the two `element_lists[...]`
  subscripts. **Correct.**
- Row 12: `check_func_call_signature` opens at `llvm_codegen_utils.cpp:103` and
  the closing brace is `:145`; `:141-142` is the `TI_ERROR`, `:138-140` the
  `TI_INFO`. **Correct**, and it also confirms report A's own §3.4 row.

### 4.2 The resolution claim — tested, and the figure in my brief is not the report's

**My brief asked me to test a claim of "342 file-qualified citations". No such
figure appears in report B.** The report claims **278**, at
`report-02b-codegen.md:41` and `:1577`. `grep -n "342"` over the report returns
nothing. I proceeded to test the claim the report actually makes.

**Method, stated so it can be repeated.** I extracted every token of the form
`<name>.<ext>:<line>` or `<name>.<ext>:<line>-<line>` from the report by regular
expression, resolved each basename against a full walk of the repository
preferring a path-suffix match, and compared the cited line against the target
file's length. Extensions covered: `cpp h mm cc hpp inc py txt md cmake`.

| Quantity | Value |
|---|---|
| Raw citation occurrences | 380 |
| Distinct (file, start, end) triples | 311 |
| Distinct (file, start-line) pairs | 298 |
| Distinct files named | 87 |
| **Citations whose file does not resolve** | **0** |
| **Citations whose line is out of range** | **0** |

The seven initial out-of-range hits were all `runtime.cpp`, and all were my
resolver preferring `taichi/runtime/gfx/runtime.cpp` (883 lines) over
`taichi/runtime/llvm/runtime_module/runtime.cpp` (1984 lines). The report's
surrounding prose disambiguates in every case, and I opened the disputed lines
in the LLVM runtime module by hand: `:1000-1002` is the `if (all_dense) return;`,
`:1003-1007` the `element_lists` range store, `:1029-1030` the `node_allocators`
write, `:1334-1336` the parent/child list pair. All exact.

**Verdict on the claim.** Report B's method statement is now true and testable,
which is precisely what round two found it was not. My count of 311 distinct
triples differs from the report's 278 because "distinct file-qualified citation"
is method-dependent — whether a range counts once or as its endpoints, whether
the ledger's deliberately-wrong "Was" column counts, whether a citation repeated
in two sections counts twice. The report does not define its counting rule, so
the two figures are not in conflict; they are two defensible countings of the
same set. **What matters is testable and passes: every file-qualified citation
in the report resolves to a real file at a real line.** The previous method
statement was false because it quantified over citations that had never been
opened. This one describes a sweep, and the sweep's product — the twelve-row
ledger — checks out row by row.

**One residual, recorded under section 8 below:** the sweep corrected two spans
in the sections the ledger names and left the superseded spans standing
elsewhere in the report, marked `[V]`.

---

## 5. Claim 5 — report B's withdrawal of the sparsity conclusion

**UPHELD. The withdrawal is correct and the replacement statement is correct.**

`llvm_runtime_executor.cpp:395-415` printed:

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

The loop contains exactly one assignment and it is `= false`. It can only clear.
So `all_dense == demote_dense_struct_fors && every-node-is-dense/place/root`,
and with the field false `all_dense` is false for **every** tree shape, the
early return at `runtime.cpp:1000-1002` never fires, and the range store at
`:1003-1007` executes on a fully dense tree. Report B's revision-2 conclusion
"both surviving routes therefore require sparse SNodes" is false and its
withdrawal at `report-02b-codegen.md:951-968` is correct.

The second filter is also as described: `is_gc_able` at
`taichi/ir/snode_types.cpp:21-23` is
`return (t == SNodeType::pointer || t == SNodeType::dynamic);`. So the
`node_allocators`/`ambient_elements` route does require sparsity and the
`element_lists` route does not.

**The store is genuinely out of bounds, which I checked rather than assumed.**
The three arrays are `runtime.cpp:567-569`, each `[taichi_max_num_snodes]`. The
loop at `:1003` runs `for (int i = root_id; i < root_id + num_snodes; i++)` and
writes `runtime->element_lists[i]`, where `root_id` derives from `SNode::id`,
assigned `id = counter++` at `taichi/ir/snode.cpp:220` from a process-global
counter. The only assertion in the tree, `struct_llvm.cpp:266`, is
`TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` — a **per-tree** count.
Different quantities. Both reports have this right and it matches what the
project plan already records at section 6.1.

**One route needs only a config flag — confirmed, with the reachability nuance
both reports state correctly.** I grepped `demote_dense_struct_fors` tree-wide:

```
taichi/program/compile_config.h:28            bool demote_dense_struct_fors;
taichi/program/compile_config.cpp:18          = true                (default)
taichi/program/compile_config.cpp:73          = true                (SPIR-V archs only, :72 gate)
taichi/python/export_lang.cpp:201-202         def_readwrite         (the only writer of false)
taichi/runtime/llvm/llvm_runtime_executor.cpp:402   the seed
taichi/transforms/compile_to_offloads.cpp:191 pass gate
taichi/transforms/offload.cpp:192             pass gate
taichi/analysis/offline_cache_util.cpp:46     cache key
tests/python/test_struct_for_intermediate.py:24, misc/benchmark_bls.py:9   set false
```

So the flag is defaulted true and the **only** in-tree writer of false is the
Python binding. Under plan section 1.2 that is out of scope, which is exactly the
grading territory 03 applied to `destroy_snode_tree` and which plan section 4.8
records as "reachable rather than routine". **Both reports state the Python
writer explicitly** — report B at `report-02b-codegen.md:945-947`, report A at
`report-02-codegen.md:286` and escalation 13 — and **both escalate the
configuration question rather than deciding it**, which is the correct handling
under section 10 item 2. I have nothing to add and no objection to manufacture.

The seed at `:402` is stated correctly in report A too, at `:248-253` and
`:266-269`, and report A's §2.1 now carries the corrected two-route statement at
`:276`. Adversary2 02-2 was right in round two that the correction was owed by
both reports; both have made it.

---

## 6. Claim 6 — the capability setter census

**UPHELD in every particular.** I re-derived it by grepping each of the five
capability names over `taichi/` and `c_api/`, excluding the `caps_->get(...)`
reads and the declaration file, then opened every site.

**Every setter of every one of the five, tree-wide, enumerated:**

| Capability | Sites | Count |
|---|---|---|
| `spirv_has_int8` | `metal_device.mm:1046`, `vulkan_device_creator.cpp:790` | 2 |
| `spirv_has_int16` | `metal_device.mm:1047`, `vulkan_device_creator.cpp:628`, `opengl_device.cpp:516`, `opengl_device.cpp:521` | 4 |
| `spirv_has_int64` | `metal_device.mm:1052`, `vulkan_device_creator.cpp:632`, `opengl_device.cpp:511`, `c_api/src/taichi_opengl_impl.cpp:9` | 4 |
| `spirv_has_float16` | `metal_device.mm:1048`, `vulkan_device_creator.cpp:787`, `opengl_device.cpp:517`, `opengl_device.cpp:525` | 4 |
| `spirv_has_float64` | `vulkan_device_creator.cpp:636`, `opengl_device.cpp:512`, `c_api/src/taichi_opengl_impl.cpp:10` | 3 |

**2 + 4 + 4 + 4 + 3 = 17**, derived from the rows above. This matches report B's
per-capability breakdown at `report-02b-codegen.md` §4.5.1 exactly (int8 2,
int16 4, int64 4, float16 4, float64 3), and matches report A's matrix at
`report-02-codegen.md:811-850` cell for cell. Excluded correctly by both:
`metal_device.mm:132` is a `caps.contains(...)` read feeding
`options.set_msl_version(2, 3, 0)` at `:134`, not a setter, and
`taichi/program/program.cpp:517` is a comment.

**Metal lacks a float64 setter entirely. Confirmed by the file-level negative:**
`grep -c float64 taichi/rhi/metal/metal_device.mm` returns **0**. The capability
block is `:1044-1053` — `spirv_version` `:1045`, int8 `:1046`, int16 `:1047`,
float16 `:1048`, subgroup_basic `:1049`, then int64 at `:1052` under
`if (feature_64_bit_integer_math)` at `:1051`. Float64 is never wired. Combined
with the bare `f64_type()` at `spirv_types.cpp:428`, an `f64` argument, return
value or argpack member on **any** Metal device emits an `OpTypeStruct` member
operand of 0. This is unconditional, not a legacy subset, and both reports are
right that it is a stronger instance than the Int64 case round one led with.

**DirectX 11 and imported Vulkan lack all five. Confirmed.**
`Dx11Device::Dx11Device` builds `DeviceCapabilityConfig caps{}` at
`dx_device.cpp:563`, sets **only** `spirv_version` at `:564`, and calls
`set_caps` at `:565`. `VulkanRuntimeImported::Workaround::Workaround` opens at
`c_api/src/taichi_vulkan_impl.cpp:19` (`:18` is blank), builds `caps{}` at `:35`,
sets only `spirv_version` across the three-way version branch `:37-43`, has the
physical-storage-buffer set commented out at `:45-51`, and calls `set_caps` at
`:53`. Neither names any of the five.

**Report A's correction of the fabricated identifier is right and is applied.**
`grep Inner c_api/src/taichi_vulkan_impl.{h,cpp}` returns no constructor named
`Inner`; the member is `inner_`. Report A now carries
`VulkanRuntimeImported::Workaround::Workaround` at `report-02-codegen.md:773`
and `:1508-1510`. No occurrence of `Inner::Inner` survives in either report.

**Two further findings in the same census, both correct:**

- **OpenGL never sets `spirv_has_int8` under any branch.** I printed
  `opengl_device.cpp:506-530` in full. The four `caps.set` blocks are the
  `!is_gles()` int64/float64 pair `:509-513`, the `GLAD_GL_NV_gpu_shader5`
  int16/float16 pair `:515-518`, the `GLAD_GL_AMD_gpu_shader_int16` int16 test
  `:520-522`, and the `GLAD_GL_AMD_gpu_shader_half_float` float16 test
  `:524-526`, then `spirv_version` `:528` and `set_caps` `:529`. No int8
  anywhere. So `i8`/`u8` arguments emit id 0 on all OpenGL, desktop included.
  Report A's three-probe span `:515-518`, `:520-522`, `:524-526` is exact and
  its correction from the round-two span `:515-522` — which omitted the
  half-float block — is right.
- **The C API OpenGL override is subtractive as well as additive.** I printed
  `c_api/src/taichi_opengl_impl.cpp:1-30`. `OpenglRuntime::OpenglRuntime` is
  `:4-13`; `device_()` is constructed at `:6`, so `GLDevice::GLDevice` and its
  `set_caps` at `opengl_device.cpp:529` run first; then `:8-11` builds a fresh
  config with int64, float64 and `spirv_version` only, and `:12` calls
  `get_gl().set_caps(std::move(caps))`. `Device::set_caps` at
  `taichi/rhi/public_device.h:855-857` is `caps_ = std::move(caps);` — a
  whole-object replacement. So any int16 or float16 the three probes granted is
  discarded. And `ti_import_opengl_runtime` at `:21-29` calls
  `set_gles_override(use_gles)` at `:26` before `ti_create_runtime` at `:28`,
  so on that path a GLES device reports Int64. Both reports have this; report A
  draws out the subtraction explicitly at `:790-798`.

**`ti_set_runtime_capabilities_ext` is the general case, and both reports carry
it.** `c_api/src/taichi_core_impl.cpp:317-334`: it builds a
`DeviceCapabilityConfig` from caller-supplied pairs at `:325-330` — casting an
untrusted `uint32_t` straight to `taichi::lang::DeviceCapability` at `:328` with
no enum validation — and installs it with `set_caps` at `:331`. No device query
anywhere. Report A records it at `:805-810`, report B as escalation 18.

---

## 7. Claim 7 — the sweep accounting

### 7.1 The territory, counted mechanically

`find taichi/codegen -type f` returns **51**. Restricted to `.h`/`.cpp` it
returns **45**. The difference is exactly six `CMakeLists.txt`, one per
directory: `amdgpu`, `cpu`, `cuda`, `dx12`, `llvm`, `spirv`. Both reports state
51 / 45 / 6 and both are right.

### 7.2 Report B's 21 + 24 partition — exact

I did not eyeball this. I transcribed report B's two lists into a script,
compared them against a directory walk, and checked for duplicates, omissions
and phantoms:

| Check | Result |
|---|---|
| Files on disk (`.h`/`.cpp`) | 45 |
| Names in §9.1 "the 21 cited" | 21 |
| Rows in §9.2 "the 24 remaining" | 24 |
| Union | 45 |
| Appearing in both lists | 0 |
| Named in the report but absent from disk | 0 |
| On disk but in neither list | 0 |

**The partition is exact.** 21 + 24 = 45 with no double-counting and no gap, and
each of the 24 carries a stated finding rather than a bare name. Report B's
`:1566-1568` reconciliation holds. This is the item that round two identified as
the real difference between the two reports, and it is now the stronger of the
two accountings.

I also spot-checked three of the 24 rows at source, choosing the three that
carry substantive findings rather than "no width content":

- `spirv/spirv_types.h:71-91` — `PhysicalPointerType` derives from
  `IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75`, with no capability
  gate. **Confirmed.**
- `spirv/snode_struct_compiler.cpp` — `construct` builds it at `:53`; the only
  call to `construct` is at `:18`, inside a `/* */` opened `:16` and closed
  `:19`. **Confirmed**, and there is a second commented block at `:22-28` that
  report A notes and report B does not.
- `spirv/kernel_utils.cpp:87-95` — the ret-attribs loop pushes one
  `RetAttributes` with `ra.dtype = PrimitiveTypeID::i32;` at `:93`, its own
  comment calling it a placeholder retained for
  `GfxRuntime::device_to_host::require_sync`. **Confirmed.**

### 7.3 Report A's accounting — the 24-file list is exact, the reconciling sentence is not

Report A's accounting is at `report-02-codegen.md:1560-1572`. It names a list of
files "with nothing width- or index-relevant" and then says: *"That, plus every
file cited in §3.1, §3.2 and §3.3, accounts for all 45 `.h`/`.cpp` files under
`taichi/codegen/`."*

I expanded the list, including its four-file shorthand "the four per-backend
`codegen_*.h` headers", and tested it the same way:

- The irrelevant list expands to **24** distinct files, no duplicates. Every one
  exists on disk.
- The remainder is therefore **21** files, each of which the sentence claims is
  cited in §3.1, §3.2 or §3.3 (lines 364-894).
- **Twenty of the twenty-one are.** The exception is
  `taichi/codegen/llvm/llvm_codegen_utils.cpp`, whose basename does not occur
  anywhere in lines 348-894.

`llvm_codegen_utils.cpp` **is** covered by report A — at `:923` in the §3.4
citation-audit table, at `:1093` in §4.1.9, and at `:1277` in the §4.3
divergence table — but §3.4 is not one of the three sections the sentence names,
and §4 certainly is not. So the union of the two halves the sentence adds
together is **44**, not 45.

This is a real defect of exactly the species section 10 item 8 governs: a total
stated alongside a list that a reader cannot reconcile against it. **Blast
radius: one sentence, `report-02-codegen.md:1569-1572`.** Nothing substantive
follows — the file is opened, its finding is real and is stated at `:1093`, and
report B independently reaches the same finding on the same lines. Report A's
coverage of the territory is complete; only its statement of how it is complete
does not close. Naming §4.1.9 alongside the other three would fix it.

---

## 8. What is newly wrong or still missing

Four residuals. All are citation-level or accounting-level. None changes a
conclusion in either report. I record them with lines because that is the only
way the next pass can close them mechanically.

### 8.1 Report A — the sweep accounting is off by one file

Stated in full at section 7.3 above. `report-02-codegen.md:1569-1572`.
**Architectural or environmental: neither — it is a defect in this report, not
in the codebase.** Blast radius: one sentence.

### 8.2 Report A — the `get_buffer_value` placeholder census omits two real call sites

This is the one substantive-looking find of my pass, and it turns out to
strengthen report A's conclusion rather than damage it. I record it because the
sentence quantifies, and section 10 item 6 says a verified citation cannot
certify a quantifier.

Report A says, at `:630-638` and again at `:1336-1340`:

> Every Args- and Rets-buffer call site passes a hardcoded placeholder:
> `PrimitiveType::i32` at `:609-610`, `:618`, `:651`, `:728`, `:753`, `:788` and
> `:2293`, and `PrimitiveType::u32` at `:401`, `:516` and `:2138`. (`:1901` is
> not a call site; it sits inside the comment block at `:1899-1903`.)

Its note §44 introduces that list with "Checked every call site to see what `dt`
it receives."

I grepped `get_buffer_value` over `spirv_codegen.cpp` and opened every hit. The
real call sites are:

```
 401  get_buffer_value(BufferInfo(BufferType::Root, root_id), PrimitiveType::u32)
 516  get_buffer_value(BufferType::GlobalTmps, PrimitiveType::u32)
 609  get_buffer_value({BufferType::ArgPack, indices_l}, PrimitiveType::i32)
 618  get_buffer_value(BufferType::Args, PrimitiveType::i32)
 651  get_buffer_value(BufferType::Rets, PrimitiveType::i32)
 728  get_buffer_value(BufferType::Args, PrimitiveType::i32)
 753  get_buffer_value(BufferType::Args, PrimitiveType::i32)
 788  get_buffer_value(BufferType::Args, PrimitiveType::i32)
2024  get_buffer_value(BufferType::GlobalTmps, PrimitiveType::i32)   <-- omitted
2039  get_buffer_value(BufferType::GlobalTmps, PrimitiveType::i32)   <-- omitted
2138  get_buffer_value(BufferType::ListGen, PrimitiveType::u32)
2212  get_buffer_value(ptr_to_buffers_.at(ptr), dt)                   <-- the only forwarder
2293  get_buffer_value(BufferType::Args, PrimitiveType::i32)
```

plus the definition at `:2266`, a commented reference at `:354`, and prose at
`:604-605`. **Thirteen real call sites: twelve hardcoded, one forwarding.**
Report A's list has ten hardcoded. `:2024` and `:2039` are missing; I printed
`spirv_codegen.cpp:2020-2042` and both are live calls inside
`generate_range_for_kernel`'s non-const begin and end branches, each passing
`PrimitiveType::i32`.

Report A correctly identifies `:1901` as a non-site — I printed `:1897-1905` and
it does sit inside a `/* */` opened `:1899` and closed `:1903`.

**Why this does not damage the conclusion.** Both omitted sites pass a hardcoded
type, so the load-bearing sentence — *"The only call that forwards a
caller-supplied `dt` is `at_buffer`'s at `:2212`"* — remains **true**, and the
escalation-5 closure at `:1330-1345` is **strengthened**, not weakened. What is
wrong is the census, not the finding.

**There is also an internal inconsistency in the label.** The sentence says
"Args- and Rets-buffer call site", but the list already includes `:401` (Root),
`:516` (GlobalTmps) and `:2138` (ListGen). Under the narrow reading those three
do not belong; under the broad reading `:2024` and `:2039` are missing. Either
way the list and its heading do not reconcile.

**Blast radius: two passages, `report-02-codegen.md:630-638` and `:1336-1340`,
plus `notes-02-codegen.md` §44.** Report B is not exposed here: its
corresponding sentence at `report-02b-codegen.md:534-540` writes "at `:618`
(also `:728`, `:753`, `:788`, `:2293`)", which is illustrative and makes no
exhaustiveness claim.

### 8.3 Report B — two withdrawn spans survive elsewhere in the report, marked `[V]`

Report B's ledger rows 9 and 10 correct `spirv_types.cpp:484-497` to `:484-514`
and `spirv_ir_builder.cpp:334-341` to `:334-353`. The ledger scopes each
correction to the sections it names — row 10 says "§3.2 and §4.6" — and those
sections are indeed corrected, at `:358` and `:778-779`, both with the
correction annotated in place.

But the superseded spans still stand in two other places:

- `report-02b-codegen.md:121` — executive summary item 2: "(`spirv_ir_builder.cpp:334-341`,
  `spirv_codegen.cpp:2194-2220`)".
- `report-02b-codegen.md:244-245` — section 2's territory-touchpoint list:
  "`spirv_ir_builder.cpp:334-341` `from_taichi_type` and `spirv_types.cpp:484-497`
  `translate_ti_type`". This bullet list is closed at `:248` with "All **[V]**."

So two citations the report's own ledger declares wrong are carried elsewhere
under a verification mark. The ledger is honest about where it applied the fix;
the report is not consistent with itself. **This is the same shape as the fault
claim 1 asked me to check for the `:2464`/`:2511` swap — a correction landing in
some places and not all — and it is why that check was worth making.**

**Blast radius: two lines, `report-02b-codegen.md:121` and `:244-245`.** The
pointer-representation conclusion those citations support is unaffected: I
opened both functions and the 32-bit branch is `spirv_ir_builder.cpp:341` and
`spirv_types.cpp:495-496` as the report says.

### 8.4 Report A — one open-ended span survives in a row labelled "Spans corrected"

`report-02-codegen.md:446` (row S16) reads:
"`translate_ti_type` (function `spirv_types.cpp:484-...`, pointer branch
`:490-498`, ...). **Spans corrected.**"

Report A's note §36 states that this span was "now closed", and the report gives
the closed form `:484-514` at `:595` and `:946`. The S16 row still carries the
ellipsis, in the row that announces the correction.

**Blast radius: one line.** The function is `:484-514`; I printed it.

A matching, smaller instance in report B: `report-02b-codegen.md:1553` gives
`construct` as `spirv/snode_struct_compiler.cpp:34-...` in the §9.2 table.
The function is `:34-63`; report A has it closed at `:735`.

### 8.5 Not defects — divergences between the two reports that need no arbitration

I record these so the planner is not asked to adjudicate them a fourth time.
Each is a convention difference, both sides land on the same code, and each
report is internally consistent.

- **`get_primitive_type` arm spans.** Report A starts each arm at the guard
  (`spirv_ir_builder.cpp:302-304`, `:306-308`, `:312-314`, `:316-318`,
  `:292-294`); report B includes the discriminating `else if` test
  (`:301-304`, `:305-308`, `:311-314`, `:315-318`, `:291-294`). I printed
  `:288-320`. Both are right about the same five arms.
- **`make_value`.** Report A cites `spirv_ir_builder.h:290-298`, the whole
  function; report B cites `:290-291`, the template head and signature. The
  function is `:290-298`; the signature is `:290-291`. Result Type is parameter
  two in both readings.
- **`PhysicalPointerType`.** Report A cites `spirv_types.h:71-77`, the class
  head through the constructor; report B cites `:71-91`, the whole class.
- **`visit(LinearizeStmt*)` on the LLVM path.** Report A's L5 gives
  `codegen_llvm.cpp:1736-1743`; report B gives `:1736-1744`. The function is
  `:1736-1744` — `:1743` is `llvm_val[stmt] = val;` and `:1744` is the closing
  brace. Report B matches report A's own signature-to-brace convention, stated
  in its note §41. This is worth one character in a future pass and nothing
  more.

### 8.6 What I looked for and did not find missing

Recorded so that "complete" means something checked rather than something
assumed.

- **Coverage of item 6.3 in this territory.** Both reports stop at the same
  seam with the same two facts and both say explicitly that 6.3 is not their
  assignment. `codegen.cpp:37-74` keys backend selection on
  `compile_config.arch` inside `#if defined(TI_WITH_CUDA)` / `TI_WITH_DX12` /
  `TI_WITH_AMDGPU`; `llvm/kernel_compiler.cpp` takes
  `const DeviceCapabilityConfig &device_caps` at `:32` and never uses it, while
  `spirv/kernel_compiler.cpp:37` assigns `params.caps = device_caps;`. Report A
  `:320-347`, report B `:250-266`. No gap.
- **The `PhysicalPointerType` ungated-64-bit finding**, which is directly on item
  6.2. I initially took this for a report B exclusive; it is not. Report A has it
  at `:733-737` with the dead construction site, and report A additionally names
  the second commented block at `snode_struct_compiler.cpp:22-28`, which report B
  does not.
- **The `Extension::data64` quantifier**, which report B's own ledger flags as a
  past overstatement. I tested it: `taichi/program/extension.cpp:9-28` grants
  `data64` to `Arch::x64` (`:12`), `Arch::arm64` (`:16`) and `Arch::cuda`
  (`:20`) only; `metal` `:23`, `opengl` `:24`, `gles` `:25`, `vulkan` `:26` and
  `dx11` `:27` are all empty or hold only `extfunc`, and the OpenGL grant is
  commented out at `:29-30`. **No SPIR-V-spine arch is granted it.** Report A's
  statement at `:625-626` holds.
- **The two constants' site counts.** `taichi_max_num_snodes` occurs five times
  tree-wide: the declaration `taichi/inc/constants.h:12`, exactly **one** in
  codegen (`struct_llvm.cpp:266`), and three runtime arrays
  (`runtime.cpp:567-569`). Report A's "exactly once in codegen" and "four sites"
  (uses, excluding the declaration) both reconcile.
  `taichi_max_num_indices` occurs **twice** in `taichi/codegen/`:
  `struct_llvm.cpp:171` live, `codegen_llvm.cpp:2363` inside a comment. Report A
  says "two in codegen" and discloses the comment separately at L16.
- **Escalation numbering.** Report A's section 5 runs 1-16 with no gap or
  repeat; report B's section 8 runs 1-20 with no gap or repeat. I cross-checked
  every escalation number referenced from the amendment notes against the
  numbered list in the corresponding report: report A's notes §§39, 42, 43, 45
  reference 13, 14, 15, 16 and each lands on the escalation the note describes;
  report B's notes §§38, 39, 41 reference 1, 2, 3, 11, 12, 13, 15, 16, 17, 18,
  19, 20 and each lands correctly. Report B's note §41 claims "Ten items. Seven
  of them carry an escalation of their own"; I counted the ten named items and
  the seven escalation numbers and both reconcile.
- **The withdrawn wording.** Report A's §4.2 point 5 is marked "WITHDRAWN AND
  REPLACED" in place rather than deleted, which is the right treatment. Report B
  has removed "silently" from its failure-mode statement; its four surviving
  uses of "silent" are all in correct contexts, three of them describing what is
  *not* silent.
- **Report A's classification of `spirv/kernel_utils.cpp`** as having nothing
  width- or index-relevant, against report B's recording of the hardcoded
  `ra.dtype = PrimitiveTypeID::i32;` at `:93`. I opened it. Report B itself says
  "It is not an index width". This is a judgement difference about what belongs
  in an inventory of index and address widths, not a factual gap, and report B
  states its reason. I am not calling it a defect in report A.

---

## 9. Escalations

Unresolved judgement, per the standing instructions. I am not deciding any of
these.

1. **Whether the four residuals in section 8 block consensus.** They are all
   citation-level or accounting-level, none changes a conclusion, and the two
   reports agree on every substantive matter I tested. Whether that constitutes
   "general consensus reached" under section 9.1 item 7, or requires one more
   mechanical pass, is the arbiter's call and not mine. My reading of the
   evidence is that the substance has converged and the residue is smaller than
   round two's; I state the residue precisely rather than grade it.

2. **The counting rule for "distinct file-qualified citation".** Report B claims
   278; my extraction gives 311 distinct (file, start, end) triples and 298
   distinct (file, start-line) pairs. The two are not in conflict because the
   report does not state its counting rule. Whether the rule should be stated —
   so that the number is reproducible by the next reader rather than only by its
   author — is a documentation question for the planner. **The testable part
   passes: zero unresolvable files, zero out-of-range lines.**

3. **The reachability grade of the `demote_dense_struct_fors` route.** The only
   in-tree writer of `false` is the Python binding
   (`taichi/python/export_lang.cpp:201-202`), which section 1.2 puts out of
   scope. This is structurally the same situation the plan resolved at section
   4.8 for `destroy_snode_tree`: reachable, not routine. Both reports escalate
   it rather than grading it, which I think is right, but the plan now has a
   precedent for how to grade it and applying that precedent is the planner's
   act, not an agent's.

4. **Whether `spirv/kernel_utils.cpp:87-95` belongs in a width inventory.** The
   two reports classify the same lines differently and both state their reason.
   Not adjudicable from the source; it depends on what the inventory is for.

---

## 10. Verdicts

**CORRECT — yes, for both reports.**

All seven claims put to me are upheld at source, each verified by opening the
named file rather than by reading either amendment agent's account:

1. The `:2464`/`:2511` swap is real, is corrected, and the correction landed in
   all seven places in report A and all six in report B. No stale pairing
   survives.
2. The severity chain holds at every link, including the two consumer-plumbing
   links neither amendment agent traced. Adversary2 02-2 is right; adversary2
   02-1's points 3 and 4 are false and both reports correctly declined to adopt
   them.
3. The counts are 8 accessors, 8 optional types, 5 capability gates, 3 distinct
   bit widths, derived from printed enumerations of
   `spirv_ir_builder.cpp:149-176` and `spirv_types.cpp:393-431`. Report A's four
   figures and report B's three are all exact. The planner's six has no reading;
   adversary2 02-1's replacement is a miscount of the same species it corrected.
4. Report B's ledger reconciles three ways against its own rows, and all twelve
   corrections are exact at source. Every file-qualified citation in the report
   resolves: 0 unresolvable files, 0 out-of-range lines, across 311 distinct
   citations covering 87 files.
5. The sparsity withdrawal is correct, the replacement statement is correct, and
   the config-flag route is real, with the Python-only writer stated by both
   reports and escalated rather than graded.
6. The census is 17 setters — 2, 4, 4, 4, 3 — Metal has zero occurrences of the
   string `float64`, and DirectX 11 and imported Vulkan set only
   `spirv_version`. Every cell of report A's seven-column matrix checks out.
7. The territory is 51 files, 45 `.h`/`.cpp`, 6 `CMakeLists.txt`. Report B's
   21 + 24 partition is exact, mechanically verified, with zero duplicates and
   zero omissions.

I found nothing substantive that is wrong in either report, and I have not
manufactured an objection to justify the round. Where an amendment agent
resolved against an adversary, I checked the source and the amendment agent was
right in every instance.

**COMPLETE — not yet, narrowly. Three items in report A, one in report B.**

- Report A `:1569-1572` — the 45-file accounting sentence adds two halves that
  union to 44. `llvm/llvm_codegen_utils.cpp` is covered at `:1093` but named by
  neither half.
- Report A `:630-638` and `:1336-1340` — the `get_buffer_value` census omits the
  live call sites at `spirv_codegen.cpp:2024` and `:2039`, and its heading does
  not reconcile with its own list. The conclusion it supports survives intact
  and is strengthened.
- Report A `:446` — an open-ended span `spirv_types.cpp:484-...` in the row
  labelled "Spans corrected"; the closed form `:484-514` appears at `:595` and
  `:946`.
- Report B `:121` and `:244-245` — two spans the report's own ledger withdrew,
  still standing, the second inside a bullet list closed with "All **[V]**".

None of these changes a finding, an escalation, or an answer to any item in
section 6 of the plan. All four are transcription residue of the same kind: a
correction that landed where it was raised and not where it was repeated. That
is the exact failure mode claim 1 was set to test, which is the argument for
one more mechanical pass and against calling it settled by assertion.

**What consensus now rests on.** The two reports agree on every substantive
matter I tested: the swap, the chain, the counts, the census, the sparsity
route, the sweep, the ungated `PhysicalPointerType`, the dead `success` flag,
the disabled validator, the unqueried `Extension::data64`, and the 6.3 seam.
Where they differ they differ in span convention, and each is internally
consistent in its own convention. The remaining work is four line-level edits
across two files, and no further source investigation.

---

## 11. Constraints

I modified no file except this one. No source file was touched. I decided
nothing unnecessary and proposed no removal, fix, cleanup or abstraction. Every
claim above carries a file path and a line number. Unresolved judgement is in
section 9 rather than settled here.

I built and ran nothing. The SPIRV-Tools chain in section 2 is read link by link
from the checked-out submodule at `external/SPIRV-Tools`. I have not observed a
driver reject an id-0 module and I make no claim about driver behaviour. The
citation-resolution figures in section 4.2 and the sweep partition in section
7.2 are script products; the method for each is stated in the section so it can
be repeated or falsified.

`adversary3-02-2.md` did not exist when I finished. Nothing appended.
