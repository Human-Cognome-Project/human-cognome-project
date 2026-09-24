# Adversary 2-1, territory 05 (`shelved-64bit`), round two

Adversary 1 of 2, second adversarial round. Target documents:
`report-05-shelved-64bit.md` (hereafter **05a**) and
`report-05b-shelved-64bit.md` (hereafter **05b**), both amended 2026-09-09,
read against `adversary-05-1.md`, `adversary-05-2.md` (both with addenda),
`notes-05-shelved-64bit.md` and `notes-05b-shelved-64bit.md`.

Repository at `ba0e81dce`, branch `master`. Nothing was modified. Git was used
read-only. No `gh` call was made in this pass; every GitHub-derived claim I rely
on is carried forward from the documents under review and is marked as such
rather than re-verified, because the principal task is a source question.

**Grading authority.** Plan section 10 item 7, added after all four documents in
this territory were written. It replaces every earlier definition of
architectural and environmental used in this territory, including 05a's own
section 7 definition, which 05a explicitly cites as its ground for withholding a
label. The new test is applied below and it changes results.

Every count in this file is derived from an enumeration printed in the section
that states it.

---

## 0. Verdicts, up front

| Question | Verdict |
|---|---|
| The `at_buffer` / `ValueKind` contradiction | **05b is right on the mechanism. 05a is wrong.** Settled from source, section 1 |
| Is the collision ARCHITECTURAL under plan 10.7? | **Yes, unambiguously.** Both reports reach the label by reasoning the new definition forbids. Section 2 |
| Blast radius | **Neither report's number is the right shape.** My enumeration, section 3 |
| ALSO-VERIFY 1, the undeclared `u64` type | **CONFIRMED, and it is larger than 05a states.** Section 4 |
| ALSO-VERIFY 2, the sparse binding width | **CONFIRMED in every part.** Section 5 |
| ALSO-VERIFY 3, the addressing model withdrawal | **CONFIRMED from v1.1.3.** Section 6 |
| ALSO-VERIFY 4, the tag census | **CONFIRMED by enumeration. 121 tags, 11 / 3 / 10.** Section 7 |
| Is 05a CORRECT? | **No.** Three linked passages are wrong. Section 10 |
| Is 05b CORRECT? | **Yes on the principal question**, with two smaller defects. Section 10 |
| Are they COMPLETE? | **No, neither**, on one point they both assert categorically. Section 3.4 |

---

## 1. The principal task: what is actually true at each point

The lead's hypothesis is right. The two accounts are **partially compatible**.
Every individual fact 05a states is true. The inference 05a draws from those
facts is false, and it is false because of a fact 05a never traced.

### 1.1 The four points, each read from source

**Point 1. `ValueKind` exists and is a genuine third field.** TRUE, 05a is right.

`taichi/codegen/spirv/spirv_ir_builder.h:69-79`, `enum class ValueKind`, nine
members, enumerated from the source: `kNormal`, `kConstant`, `kVectorPtr`,
`kStructArrayPtr`, `kVariablePtr`, `kPhysicalPtr`, `kTexture`, `kFunction`,
`kExtInst`. Nine names, nine members. `struct Value` at `:82-93` carries
`ValueKind flag{ValueKind::kNormal}` at `:88`.

*Note on a citation both reports and both round-one adversaries share.* 05b says
both adversaries carry `:2204` for the tag assignment and that the correct line
is `:2203`. I confirm `:2203` independently: `grep -n "paddr_ptr.flag"` returns
one hit, at `:2203`. Both reports' `:69-79` for the enumeration is correct; I
checked it because I initially read it as off by one and it is not.

**Point 2. The tag is set at `spirv_codegen.cpp:2203`.** TRUE, 05a is right.
`paddr_ptr.flag = ValueKind::kPhysicalPtr;` sits between the `make_value` that
builds the `OpConvertUToPtr` result at `:2198-2202` and the `return` at `:2204`.

**Point 3. The tag already drives `load_variable`.** TRUE, 05a is right.
`taichi/codegen/spirv/spirv_ir_builder.cpp:1270-1285`. `TI_ASSERT` at
`:1271-1273` requires `kVariablePtr`, `kStructArrayPtr` or `kPhysicalPtr`;
`:1275` branches on `kPhysicalPtr` to emit `OpLoad` with
`spv::MemoryAccessAlignedMask` at `:1277-1280`. `store_variable` at `:1286-1298`
does the same at `:1290`.

**Point 4. The tag survives `register_value` / `query_value`.** TRUE, 05a is
right. `register_value` at `:1300-1309` stores the whole `Value` at `:1308`;
`query_value` at `:1311-1317` returns it by value at `:1314`.

**Point 5. `make_value` erases the tag.** TRUE, 05b is right, and 05a never
addresses it. `taichi/codegen/spirv/spirv_ir_builder.h:290-298`:

```cpp
  template <typename... Args>
  Value make_value(spv::Op op, const SType &out_type, Args &&...args) {
    Value val = new_value(out_type, ValueKind::kNormal);
    make_inst(op, out_type, val, std::forward<Args>(args)...);
    if (out_type.flag == TypeKind::kPtr) {
      val.flag = ValueKind::kVariablePtr;
    }
    return val;
  }
```

The result's tag is a function of the SPIR-V **result type** only. Operand tags
are not read. `struct SType` is at `spirv_ir_builder.h:49-67` and its
`TypeKind flag{TypeKind::kPrimitive}` at `:59`, so an integer byte offset of any
width comes out `kNormal`.

**Point 6, and it is the one that decides the question. At `spirv_codegen.cpp:2197`
both sides of the decision are `kNormal`.** TRUE, 05b is right.

The ndarray side, traced instruction by instruction with the capability on:

| Line | Instruction | Resulting tag |
|---|---|---|
| `spirv_codegen.cpp:786-788` | `make_access_chain(...)` | pointer kind, not the value under test |
| `spirv_codegen.cpp:789` | `ir_->load_variable(addr_ptr, ir_->u64_type())` | `kNormal`, set at `spirv_ir_builder.cpp:1274` |
| `spirv_codegen.cpp:790-791` | `ir_->add(addr, make_value(OpSConvert, u64, linear_offset))` | `kNormal`, via `DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038-1047` into `make_value` |
| `spirv_codegen.cpp:792` | `ir_->register_value(stmt->raw_name(), addr)` | `kNormal` stored |

Four rows. `load_variable` is unconditional on its return tag: I read
`spirv_ir_builder.cpp:1274`, `Value ret = new_value(res_type, ValueKind::kNormal);`,
which is above the `kPhysicalPtr` branch and applies to both arms.

The SNode side, for the same reason: `GetChStmt` at `spirv_codegen.cpp:371-373`
does `make_pointer(...)` then `ir_->add(input_ptr_val, offset)` then
`register_value`. `make_pointer` at `:2317-2324` returns
`ir_->uint_immediate_number(...)`, which reaches `new_value(dtype,
ValueKind::kConstant)` at `spirv_ir_builder.cpp:1502`; the `add` at `:372`
overwrites that with `kNormal`.

**So the genuine physical device address and a root-relative offset arrive at
`at_buffer` carrying identical tags.** `at_buffer` cannot be reading the tag,
and the dtype sniff at `:2197` is not a shortcut past an available mechanism.

### 1.2 Adjudication

- **05b's account is correct.** I re-derived every leg of it rather than
  accepting it, including the one leg it says decided the matter, and each holds.
- **05a's account is correct in each of its four assertions and wrong in the
  conclusion it draws.** The tag exists, is set, survives the value table, and
  drives load and store. None of that makes it available at `:2197`.
- **05a's stated precision is right and is the seed of its own error.** "The tag
  sits on `at_buffer`'s OUTPUT, not its input" is exactly true, and 05b says the
  same. 05a then infers that populating the input at the production sites closes
  the gap. That inference is refuted by point 5: a tag written at `make_pointer`
  is destroyed by the very next instruction on every path that reaches
  `at_buffer`.
- **05a ruled against the adversary who was right.** 05a section 7.7 item F
  says "Adversary 1's stated ground is factually wrong... Adversary 2 is right on
  the point of fact." Adversary 1's *sentence* was wrong and adversary 1 withdrew
  it at its section 24.6. Adversary 1's *ruling* was right, and adversary 2
  conceded in full at its section 14.1. 05a's amendment was written against the
  earlier state of both adversary files and did not pick up either movement;
  05b's section 13.5 says the files grew during the pass and re-derived the
  facts. `notes-05-shelved-64bit.md:839-889`, entry 29, confirms this: it
  enumerates `ValueKind`, `:2203`, `load_variable` and `register_value`, and
  never touches `make_value`.

### 1.3 What survives from 05a on this question

One thing, and it should carry forward. 05a's observation that the tag is on the
output is the precise statement of why the mechanism looks available and is not.
05b reaches the same place; 05a states it more cleanly. Nothing else in 05a
section 7.7 item F survives.

---

## 2. Grading under plan section 10 item 7

The test: *if every external thing were ideal today, would the obstacle still be
there?*

Apply it to the collision. The obstacle is that `spirv_codegen.cpp` represents
"absolute device address versus buffer-relative offset" by the scalar width of
the value, at `:2197`, `:2227` and `:2249`. That is a decision inside this
codebase about how something is represented. An ideal driver does not remove it.
An ideal SPIR-V specification does not remove it. A newer SPIRV-Tools pin does
not remove it. It goes away only by changing the representation.

**ARCHITECTURAL. Not contestable under this definition.**

Both reports arrive near this, and both arrive by reasoning the new definition
forbids:

- **05a withholds the label because the work is bounded.** Section 7.7 item F:
  "it is not ARCHITECTURAL under the definition section 7 of this report uses...
  because the replacement mechanism is present in the same file". That is a blast
  radius argument used as a kind argument. Plan 10.7 says in terms:
  "Architectural does not mean hard... Grade the KIND first, then state the BLAST
  RADIUS separately." 05a's stated reason for the label is exactly the conflation
  the new rule exists to stop.
- **05b awards the label because the work is large.** Section 13.5: "My objection
  to 'architectural' rested entirely on the obstacle being local. It is not."
  Same conflation, opposite sign. 05b then adds the correct ground — "the
  collision does not expire" — so 05b lands on the right answer, but its
  narrative reads as if the size decided it.

The two agents were graded against each other on a distinction that the size of
the fix cannot settle. Adversary 1's Escalation 13, carried into 05b section
13.5, asked the arbiter to fix exactly this. Plan 10.7 is that fix, and it
resolves the label without reference to either agent's sizing.

**Consequence: the label question and the cost question are now fully separable,
and the flat contradiction the lead describes collapses into one factual error
(05a on `make_value`) plus one grading rule that neither agent had.**

### 2.1 Two further rows change under the new definition, and neither report re-graded them

05a's verdict table at section 7.0 has nine rows. Re-graded under 10.7:

| Row | 05a's grade | Grade under plan 10.7 | Why |
|---|---|---|---|
| A, MoltenVK wrong results | ENVIRONMENTAL | ENVIRONMENTAL, unchanged | A driver defect, outside the tree |
| B, imported `VkDevice` cannot report enabled extensions | ENVIRONMENTAL | ENVIRONMENTAL, **defensible but contestable** | Vulkan offers no query for *enabled* device extensions, so an ideal external spec would remove it. The remedy available today is a field on `TiVulkanRuntimeInteropInfo`, `c_api/include/taichi/taichi_vulkan.h:26-47`, which has nine members and none for extensions or features. I checked; I do not overturn the grade |
| C, AOT devcap negotiation not ready | ENVIRONMENTAL | **ARCHITECTURAL** | Unwritten Taichi code. An ideal driver, spec and dependency set does not write it. It expired because someone changed the codebase, which is how 10.7 says architectural obstacles go away |
| D, argument ABI variant | ARCHITECTURAL | ARCHITECTURAL, unchanged | |
| E, module-wide addressing model | withdrawn | withdrawn, see section 6 | |
| F, `at_buffer` collision | "not ARCHITECTURAL" | **ARCHITECTURAL**, section 2 above | |
| G, hardware lacks the features | ENVIRONMENTAL | ENVIRONMENTAL, unchanged | Measured, expired on all three targets |
| H, SPIR-V toolchain vintage | ENVIRONMENTAL | ENVIRONMENTAL, unchanged | Submodule pins |
| I, 32-bit index width | ARCHITECTURAL | ARCHITECTURAL, unchanged | |

Nine rows in, nine rows out. Two grades change: F from "not architectural" to
ARCHITECTURAL, C from ENVIRONMENTAL to ARCHITECTURAL. One, B, I examined and left
standing. 05b's Part II table was not re-graded here row by row because 05b
reaches the same answers on F and E; its row for C carries the same defect as
05a's.

The C regrade is low-stakes, because both reports agree the obstacle is
discharged in substance. It matters only so the plan does not carry a wrong kind
next to a right conclusion.

---

## 3. Blast radius, derived here

Plan 10.7 requires the blast radius stated separately and concretely, in files
and call sites. Both reports state a number attached to a remedy, and the two
remedies are different, which is where the two orders of magnitude come from.
Neither number is wrong arithmetic; both are the wrong shape.

### 3.1 The obstacle's own footprint

Enumerated by `grep -n "ptr_val.stype.dt == PrimitiveType::u64" taichi/codegen/spirv/spirv_codegen.cpp`:

| # | Line | Function |
|---|---|---|
| 1 | `spirv_codegen.cpp:2197` | `at_buffer` |
| 2 | `spirv_codegen.cpp:2227` | `load_buffer` |
| 3 | `spirv_codegen.cpp:2249` | `store_buffer` |

Three rows, three predicates, three functions, **one file**. That is the whole of
the width-as-tag decision. Any remedy rewrites these three and no others.

### 3.2 What a tag at production would have to survive

Enumerated by reading every path from a `make_pointer` result or a device
address to a `query_value` that `at_buffer` consumes. The arithmetic instructions
that produce a registered pointer value:

| # | Line | Instruction | Visitor |
|---|---|---|---|
| 1 | `spirv_codegen.cpp:325` | `ir_->add(origin_val, offset_bytes)` | `MatrixPtrStmt`, global-temporary origin |
| 2 | `spirv_codegen.cpp:331` | `ir_->add(origin_val, ir_->cast(...))` | `MatrixPtrStmt`, byte-offset form |
| 3 | `spirv_codegen.cpp:372` | `ir_->add(input_ptr_val, offset)` | `GetChStmt` |
| 4 | `spirv_codegen.cpp:509` | `ir_->add(parent_val, offset)` | `SNodeLookupStmt` |
| 5 | `spirv_codegen.cpp:790-791` | `ir_->add(addr, OpSConvert(...))` | `ExternalPtrStmt`, capability branch |

Five rows, five instructions, four visitor methods. Every one routes through
`make_value` and every one returns `kNormal`. `bitmasked_activation` adds four
more `ptr_dt`-typed arithmetic instructions at `:398`, `:403-405`, `:406-408` and
`:409-412` which carry the width but do not reach `at_buffer`; they are counted
separately because they are the subject of a different defect, 05a section
2.3(b).

Production sites that would need to originate a tag: the four `make_pointer`
call sites, enumerated by `grep -n "make_pointer" taichi/codegen/spirv/spirv_codegen.cpp`
minus the definition — `:355`, `:371`, `:408`, `:506` — plus the ndarray base at
`:789`. Five. That is 05a's number and it is correctly counted; it is simply not
sufficient, because of the five rows above.

### 3.3 The `make_value` figure, checked and re-stated

`grep -rn "make_value(" taichi/codegen/spirv/` returns **148** occurrences. Per
file: `spirv_ir_builder.h` 1, `spirv_ir_builder.cpp` 64, `spirv_codegen.cpp` 83.
1 + 64 + 83 = 148. The single occurrence in the header is the definition at
`:291`. So **147 call sites**, 64 + 83, reconciling exactly. 05b's figure is
right and I derived it independently. The same grep tree-wide outside
`taichi/codegen/spirv/` and `external/` returns 0, so the function is confined to
this backend.

**But 147 is a count of affected call sites, not of edits.** The distinction is
the whole disagreement:

| Quantity | Number | Derivation |
|---|---|---|
| Functions whose body must change, minimum | **4** | `make_value` (1) plus the three predicates in `at_buffer`, `load_buffer`, `store_buffer` |
| Files touched, minimum | **2** | `spirv_ir_builder.h`, `spirv_codegen.cpp` |
| Production sites to tag | **5** | Section 3.2 |
| Call sites whose result tag changes meaning | **147** | Section 3.3 |
| Helper wrappers needing an operand-propagation rule | **3 named plus 2 macros** | `cast`, and the macros `DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038` and `DEFINE_BUILDER_BINARY_SIGN_OP` at `:1049`, whose four instantiations at `:1062-1065` are `add`, `sub`, `mul`, `div` |

**My statement of the blast radius:** two files, four functions edited, five
production sites, and a contract change on the SPIR-V backend's single SSA
constructor that reaches 147 call sites. It is a value-model change, not a
call-site sweep. 05b is right that this is not local and wrong to leave "147 call
sites" standing as if 147 edits were implied. 05a is wrong that five production
sites suffice.

### 3.4 A discriminator both reports rule out, and the ruling is not established

This is where I diverge from both, and it is the one place where I think the
territory is incomplete.

05b section 13.5 leg 4, taken verbatim from `adversary-05-2.md` section 14.1,
says `ptr_to_buffers_` cannot serve, because `visit(ExternalPtrStmt *)` sets it
at `:797-801` **outside** the capability branch, "so under the capability an
ndarray pointer carries both a u64 physical address and a `ptr_to_buffers_`
entry, exactly like an SNode pointer. **There is no existing discriminator at the
decision point by any route.**"

The premise is true. I verified it: the capability branch is `:783-795` and the
map assignment is `:797-801`, outside it. **The conclusion does not follow, and
the final sentence is not established.** The argument tests whether an entry is
*present* on both sides. It never looks at the entry's *value*.

`ptr_to_buffers_` is `std::unordered_map<const Stmt *, BufferInfo>` at
`spirv_codegen.cpp:2635`. `BufferInfo` carries `BufferType type`
(`taichi/codegen/spirv/kernel_utils.h:33-34`), and `BufferType` at `:23-31` has
seven members: `Root`, `GlobalTmps`, `Args`, `Rets`, `ListGen`, `ExtArr`,
`ArgPack`. Enumerating every writer, by `grep -n "ptr_to_buffers_" taichi/codegen/spirv/spirv_codegen.cpp`:

| # | Line | Value written | Path |
|---|---|---|---|
| 1 | `:319` | copied from `stmt->origin` | `MatrixPtrStmt`, shared alloca |
| 2 | `:326` | copied from `stmt->origin` | `MatrixPtrStmt`, global-temporary origin |
| 3 | `:332` | copied from `stmt->origin` | `MatrixPtrStmt`, byte-offset form |
| 4 | `:377` | `BufferInfo(BufferType::Root, root)` | `GetChStmt`, place node |
| 5 | `:711` | `BufferType::GlobalTmps` | `GlobalTemporaryStmt` |
| 6 | `:798` | `{BufferType::ExtArr, arg_id}` | `ExternalPtrStmt`, array argument |
| 7 | `:800` | `BufferType::Args` | `ExternalPtrStmt`, non-array argument |

Seven writers. Rows 4 and 6 are the two sides of the `:2197` decision, and they
hold **different values**: `Root` against `ExtArr`. The map is read at `:2212`
by the fall-through path, so it is already consulted inside `at_buffer`, one
statement below the predicate under dispute.

So a value-distinct, statement-keyed discriminator does sit at the decision
point today. Whether a correct remedy can be built on it I did not establish, and
I am not designing one: row 7 puts a non-array `ExternalPtrStmt` on `Args`, rows
1 to 3 inherit whatever the origin carried, and `BufferInfo` is default-
constructed by `operator[]` at rows 1 to 3 when the origin has no entry. Those
are real questions and they need answering before anyone claims a cheap fix.

**What I do claim, and it is bounded:** the sentence "there is no existing
discriminator at the decision point by any route" is refuted by rows 4 and 6, it
was reached by testing presence rather than value, and it now appears in
`adversary-05-2.md` section 14.1, in `report-05b` section 13.5, and in
`notes-05b-shelved-64bit.md`. It is load-bearing, because it is the sentence that
forces the remedy onto `make_value`. If the map can carry the distinction, the
blast radius is section 3.1's three predicates in one file, not section 3.3's
value model. That is the two-orders-of-magnitude question, still open, and closed
by nobody.

This does not change the KIND. Under plan 10.7 the collision is architectural
either way: representing addressing scheme by scalar width is an internal
representation decision, and it does not expire whichever internal replacement is
chosen. It changes only the cost, which 10.7 requires be stated separately.

---

## 4. ALSO-VERIFY 1: the undeclared `u64` type. CONFIRMED, and larger than 05a states

05a's claim: naming `spirv_has_physical_storage_buffer` alone leaves
`spirv_has_int64` false, `t_uint64_` is never declared, and
`spirv_codegen.cpp:787`, `:789`, `:790` call the unguarded `u64_type()`.

Every link verified from source:

1. **The caps list is taken verbatim.** `translate_devcaps`,
   `taichi/program/program.cpp:514-539`. `DeviceCapabilityConfig cfg{}` at `:517`
   is empty. The loop at `:518-532` sets only what the caller named. The only
   addition is a `spirv_version` default of `0x10300` at `:535-537`. Nothing
   implies `spirv_has_int64`.
2. **`t_uint64_` is declared only under the int64 capability.**
   `taichi/codegen/spirv/spirv_ir_builder.cpp:166-169`, inside
   `if (caps_->get(cap::spirv_has_int64))`. The `OpCapability Int64` declaration
   at `:64-66` is under the same guard.
3. **The accessor is unguarded.** `spirv_ir_builder.h:532-534`,
   `SType u64_type() const { return t_uint64_; }`. `t_uint64_` is declared at
   `:621` and is a plain `SType` member, so when never assigned it holds
   `SType{}`, and `SType::id` is `uint32_t id{0}` at `spirv_ir_builder.h:51`.
4. **The `TI_ERROR` guards are on a different function.**
   `spirv_ir_builder.cpp:312` and `:326` sit in `get_primitive_type(DataType)`.
   Neither the ndarray path nor `make_pointer` calls it.
5. **The call sites exist and are on the capability branch.** The branch opens at
   `spirv_codegen.cpp:783`. `:787` `ir_->get_pointer_type(ir_->u64_type(), ...)`,
   `:789` `ir_->load_variable(addr_ptr, ir_->u64_type())`, `:790`
   `ir_->make_value(spv::OpSConvert, ir_->u64_type(), ...)`. Three call sites,
   as 05a says.
6. **What is emitted.** `get_pointer_type` at `spirv_ir_builder.cpp:407-424`
   keys on `value_type.id`, which is 0, and emits `OpTypePointer <new id>
   Uniform 0` at `:419-421`. Id 0 is never a valid SPIR-V id. No diagnostic
   anywhere on the path.
7. **Nothing catches it.** `spirv_opt_options_.set_run_validator(false)` at
   `spirv_codegen.cpp:2710`. `grep -n "Validate"` over that file returns nothing;
   the only other SpirvTools call is a `Disassemble` at `:2763`. *(05a cites
   `:2709`; the line is `:2710`. Immaterial, recorded.)*

**CONFIRMED. And 05a understates the surface.** There is a second, independent
route to the same undeclared type, which no document in this territory names:

`translate_ti_type`, opening at `taichi/codegen/spirv/spirv_types.cpp:483`, maps
a Taichi `PointerType` to `IntType(64, is_signed=false)` at `:492-493` when
`has_buffer_ptr`, and to `IntType(32)` at `:495-496` otherwise. That tinyir type is lowered by
`Translate2Spirv::visit_int_type` at `spirv_types.cpp:393-419`, whose unsigned
64-bit arm at `:414-415` is `vt = spir_builder_->u64_type();` — the same
unguarded accessor — and which then stores `ir_node_2_spv_value[type] = vt.id` at
`:418`, that is, 0. `translate_ti_type` is called from `compile_args_struct`
(`spirv_codegen.cpp:2340` reads the capability), `compile_argpack_struct`
(`:2416`) and `compile_ret_struct` (`:2491`).

So a kernel taking an ndarray argument, compiled ahead-of-time with only the
physical-storage-buffer capability named, produces an undeclared 64-bit type in
**two** places: the argument-struct type, before any statement is visited, and
the `ExternalPtrStmt` body. The signed arm at `:402-403` reaches `i64_type()`
(`spirv_ir_builder.h:529-531`) with the same absence of a guard.

**Reachability.** The producer chain 05a and 05b both document is real and I
spot-checked its two ends: `Program::make_aot_module_builder(Arch, const
std::vector<std::string> &)` at `taichi/program/program.cpp:541-556`, and
`str2devcap` accepting the name because `taichi/inc/rhi_constants.inc.h:28`
declares it.

**One qualification neither report makes, and it bounds the exposure.** No
*device-derived* capability set can produce this pairing. In
`taichi/rhi/vulkan/vulkan_device_creator.cpp`, `spirv_has_int64` is set at `:632`
under `if (device_supported_features.shaderInt64)`, and the physical-storage-buffer
setter at `:826` sits inside the same `shaderInt64` test at `:821`. So on the
just-in-time path the two capabilities are locked together. The hazard requires a
caller-named list, which is `translate_devcaps` or
`ti_set_runtime_capabilities_ext`. That makes it a defect of the un-validated
caller-supplied path specifically, which is the same door section 1.10 of 05a and
section 1.1 of 05b already flag.

**My assessment of severity.** 05a calls it "the most serious live finding in the
territory". I agree that it is the most serious *silent* one, on the plan's own
section 2.2 reasoning: it produces a malformed module with no diagnostic, and
Taichi never validates. I record one limit on that: it is unreachable without a
deliberate, hand-written capability list that names one capability and omits its
dependency, so it is a trap for a caller rather than a fault that fires on
ordinary use. Both statements are true and the plan should carry both.

---

## 5. ALSO-VERIFY 2: the sparse binding width. CONFIRMED in every part

This is the constraint that actually applies to this project, so I verified each
element independently rather than checking the citations.

**The three fields.** `taichi/ir/snode.h:37-54`, `struct AxisExtractor`:

| Line | Field | Declared type |
|---|---|---|
| `:41` | `num_elements_from_root` | `int num_elements_from_root{1};` |
| `:45` | `shape` | `int shape{1};` |
| `:49` | `acc_shape` | `int acc_shape{1};` |

Three fields, all `int`. Read directly from the file. For contrast
`taichi/ir/snode.h:97` is `int64 num_cells_per_container{1};`.

**The truncation precedes the warning.** `taichi/ir/snode.cpp:88-101`:

```
 88    // infer extractors
 89    int64 acc_shape = 1;
 90    for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
 91      // casting to int32 in extractors.
 92      new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
 93      acc_shape *= new_node.extractors[i].shape;
 94    }
 95    if (acc_shape > std::numeric_limits<int>::max()) {
```

The store at `:92` runs on every iteration of the loop at `:90-94`. The check at
`:95` runs once, after the loop. So the diagnostic at `:95-100` reports damage
already written. `:101` assigns the untruncated `int64` to
`num_cells_per_container`. CONFIRMED.

**The assertion never fires on a sparse path.** CONFIRMED, four legs:

1. `taichi/transforms/demote_dense_struct_fors.cpp:29`,
   `TI_ASSERT(total_n <= std::numeric_limits<int>::max());`, inside
   `convert_to_range_for` which opens at `:13`.
2. `grep -rn "convert_to_range_for" taichi/` returns four hits: the definition at
   `demote_dense_struct_fors.cpp:13` and its single call at `:117`, plus a
   same-named function in `taichi/transforms/demote_no_access_mesh_fors.cpp` at
   `:12` and `:48`, which is a separate function in a separate anonymous
   namespace and contains no such assertion. So within this file the assertion
   has exactly one reachable caller.
3. That caller, `maybe_convert` at `:114-119`, requires
   `stmt->snode->is_path_all_dense`.
4. `is_path_all_dense` is declared at `taichi/ir/snode.h:114` and cleared at
   `taichi/ir/snode.cpp:20`:
   `new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation());`.
   `SNode::need_activation` at `snode.cpp:268-271` returns true for `pointer`,
   `hash`, `bitmasked` and `dynamic` — four types, enumerated from the source.

Plan section 4.1 requires sparsity. Any one of those four node types anywhere on
the path clears the flag, the pass does not run, and the assertion does not
execute. CONFIRMED.

**The assertion is unconditional where it does run.** `TI_ASSERT` at
`taichi/common/logging.h:100-107` expands to
`{ bool ___ret___ = static_cast<bool>(x); if (!___ret___) { TI_ERROR(...); } }`.
No `NDEBUG` gate. CONFIRMED.

**One addition neither report makes.** In the same function,
`demote_dense_struct_fors.cpp:20` declares
`std::array<int, taichi_max_num_indices> total_shape;` and `:24` accumulates
`total_shape[j] *= snode->extractors[j].shape;` with no check at all. That is a
fourth 32-bit accumulator in the dense path, and it overflows before the
assertion at `:29` is reached. It does not bear on the sparse case, and I record
it only so the dense-path picture is not left as "one assertion".

---

## 6. ALSO-VERIFY 3: the module-wide addressing model. CONFIRMED withdrawn

Settled from the v1.1.3 tag, read directly:

- `git show v1.1.3:taichi/codegen/spirv/spirv_ir_builder.cpp` emits
  `spv::AddressingModelPhysicalStorageBuffer64` with `MemoryModelGLSL450` under
  `if (device_->get_cap(cap::spirv_has_physical_storage_buffer))`, and
  `AddressingModelLogical` otherwise.
- `git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp` line 74 reads
  `const bool use_64bit_pointers = false;`, and `make_pointer` at `:2062-2069`
  returns `ir_->uint_immediate_number(ir_->u32_type(), uint32_t(offset))` on the
  live branch.
- The SNode path in that tag uses it at six call sites: `:275`, `:291`, `:328`,
  `:426`, `:1805`, `:1850`. Six, enumerated by grep. (Today there are four:
  `:355`, `:371`, `:408`, `:506`.)
- The capability was device-derived in that tag:
  `git show v1.1.3:taichi/rhi/vulkan/vulkan_device_creator.cpp:736` sets it under
  `if (device_supported_features.shaderInt64)`, with no preprocessor guard.

So v1.1.3 declares the physical addressing model while addressing the SNode root
through a descriptor-bound buffer at a 32-bit offset, and it shipped. Per section
7 below that configuration shipped in eleven tags. **The module-wide addressing
model is a declaration and not a constraint. Correctly withdrawn as an obstacle
by both reports. CONFIRMED.**

---

## 7. ALSO-VERIFY 4: the tag census. CONFIRMED by enumeration

`git tag | wc -l` returns **121**. `git tag | grep -c '^v1\.1\.1$'` returns 0:
there is no v1.1.1.

I enumerated all 121 mechanically, extracting
`taichi/rhi/vulkan/vulkan_device_creator.cpp` per tag with a fallback to
`taichi/backends/vulkan/vulkan_device_creator.cpp`, locating the first line
naming `spirv_has_physical_storage_buffer`, and classifying on the nearest `#if`
in the eight preceding lines. Matching the capability **name** rather than the
setter is necessary: v0.9.x writes `ti_device_->set_cap(...)` and later tags
write `caps.set(...)`.

| Class | Tags | Count |
|---|---|---|
| File absent | up to and including v0.8.6 | 92 |
| File present, name absent | v0.8.7, v0.8.8, v0.8.9, v0.8.10, v0.8.11 | 5 |
| Name present, no preprocessor guard | v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3 | 11 |
| `#if !defined(__APPLE__)` | v1.2.0, v1.2.1, v1.2.2 | 3 |
| `#if !defined(__APPLE__) && false` | v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | 10 |

92 + 5 + 11 + 3 + 10 = 121, the full tag list. The capability appears in
11 + 3 + 10 = 24 tags. The path move is visible in the rows: v0.9.0 through
v1.0.3 are found under `taichi/backends/vulkan/`, v1.0.4 onward under
`taichi/rhi/vulkan/`.

**Both amendment agents and adversary 2 are right. Adversary 1 is three tags
short, and the three it misses are v0.9.0, v0.9.1 and v0.9.2** — which is the
setter-name difference, not the path move, since the path move alone would have
hidden v1.0.0 to v1.0.3 as well. CONFIRMED.

"Unguarded" here means no preprocessor guard. All eleven sit under a runtime
device check: I read `v0.9.0:658-670` and `v1.1.3:730-742` and both nest the
setter inside `if (device_supported_features.shaderInt64)`.

---

## 8. Verdicts on 05a

**NOT CORRECT.** Three linked passages are wrong, all from the same root:

1. **Section 7.7 item F, the adjudication.** Rules for adversary 2 on the fact.
   The source rules for adversary 1. The deciding fact, `make_value` erasure at
   `spirv_ir_builder.h:291-297` and the `kNormal` tag on the live ndarray address
   at `spirv_codegen.cpp:789-791`, is absent from the report and from
   `notes-05-shelved-64bit.md` entry 29.
2. **Section 7.7 item F, the remedy.** "A bounded change to `at_buffer`,
   `load_buffer`, `store_buffer` and five production sites" is refuted by the
   five-row table in section 3.2 above: the tag is destroyed between production
   and consumption on every path.
3. **Section 7.0 row F and section 9 ledger row 5, the label.** "Not
   ARCHITECTURAL as section 7 defines the term" is superseded. Under plan 10.7
   the collision is ARCHITECTURAL, and 05a's stated reason for withholding the
   label is a blast-radius argument, which 10.7 forbids as a grading ground.

Consequences that follow and must move with it: section 7.8 item 3, "F is a
blocker but a bounded one. The obstacle count went from four permanent to two
permanent plus one bounded", and the closing paragraph of item F, "item 6.2 on
the SPIR-V path is gated... by bounded internal design work". Both rest on the
same error. 05a's own sentence there — "It is also the more agreeable finding,
which is why it was checked three ways" — is exact: the three ways it was checked
were the flag's existence, its propagation through the value table, and its
existing consumers. The fourth, whether it reaches the decision point, was not
checked, and it is the one that decides.

**Everything else in 05a that I tested holds.** Sections 1.10, 3.2, 7.1, 7.2,
7.5, 7.6, 7.7 items D, E and I, and the escalation list. The tag census in
section 1.3a matches mine exactly. One off-by-one recorded: the SPIR-V validator
is disabled at `spirv_codegen.cpp:2710`, not `:2709`.

## 9. Verdicts on 05b

**CORRECT on the principal question.** Section 13.5's ruling, its four legs, its
statement of the erasure, and its `make_value` count all reproduce under
independent derivation. Its self-criticism — that it had the deciding fact in
hand and mis-filed it as a sizing correction — is accurate and is the reason its
final answer is right.

Two defects, neither fatal:

1. **The categorical sentence in leg 4.** "There is no existing discriminator at
   the decision point by any route" is not established, and section 3.4 above
   shows why: the argument tests presence, not value, and `ptr_to_buffers_` holds
   `BufferType::Root` on one side and `BufferType::ExtArr` on the other. This is
   the sentence that forces the remedy onto the value model, so it carries the
   whole cost estimate.
2. **The grading narrative.** Section 13.5 reaches ARCHITECTURAL partly through
   the size of the remedy — "my objection rested entirely on the obstacle being
   local. It is not" — before giving the correct ground. Under plan 10.7 the size
   is irrelevant to the label. The conclusion is right; the route needs restating
   so the plan does not inherit the conflation.

Its Part II row for reason C carries the same misgrade as 05a's row C; see
section 2.1.

## 10. Verdicts on completeness

**Neither report is COMPLETE, on one shared point and one each.**

- **Shared.** Section 3.4. Both now assert, in identical terms inherited from
  `adversary-05-2.md`, that no discriminator exists at the decision point by any
  route. Seven writers of `ptr_to_buffers_`, enumerated above, produce
  value-distinct entries on the two sides of that decision. Until someone settles
  whether that map can carry the distinction, the blast radius is open by two
  orders of magnitude, which is exactly the quantity this round was convened to
  settle.
- **05a.** The undeclared-`u64` finding stops at three call sites. The
  argument-struct route through `spirv_types.cpp:415` and `:403` reaches the same
  unguarded accessors before any statement is visited. Section 4.
- **05b.** Section 5.0's account of the dense path names the assertion at
  `demote_dense_struct_fors.cpp:29` and not the unchecked `int` accumulator at
  `:20` and `:24` in the same function. Minor, and off this project's path.

**Consensus status.** On the principal question consensus is now reachable and
one document has to move: 05a's sections 7.0 row F, 7.7 item F, 7.8 item 3 and
ledger row 5. 05b needs the two corrections in section 9. What remains genuinely
unsettled after this pass is the blast radius, section 3.4, and it is unsettled
in both documents identically.

---

## 11. Escalations

1. **The `ptr_to_buffers_` question is open and it is the expensive one.**
   Section 3.4. If `BufferType` can discriminate at `spirv_codegen.cpp:2197`, the
   remedy is three predicates in one file. If it cannot, it is a contract change
   on `make_value` reaching 147 call sites. Both reports currently assert the
   second without testing the first. I did not design a fix and do not propose
   one; I report that the categorical claim is unsupported. This needs one agent
   with permission to reason about a remedy, or a compile.

2. **Plan section 10 item 7 changes two grades that no amendment applied.**
   Section 2.1. Row F becomes ARCHITECTURAL, which both reports approach from
   opposite directions on the wrong ground. Row C, "AOT devcap negotiation not
   ready", is ARCHITECTURAL and both reports call it environmental. Row B I
   examined and left as environmental, on the ground that Vulkan offers no query
   for enabled extensions on an imported device; that grade is defensible and
   contestable and I flag it rather than change it. The planner owns the plan
   edit.

3. **The undeclared-`u64` hazard has a second entrance.** Section 4. Beyond
   `spirv_codegen.cpp:787`, `:789` and `:790`, the argument, argpack and return
   struct translations reach `u64_type()` and `i64_type()` through
   `spirv_types.cpp:415` and `:403`. Whether to fix, guard or document is the
   planner's call. My one piece of scoping: no device-derived capability set can
   produce the pairing, because `vulkan_device_creator.cpp:632` and `:821-826`
   both sit under `shaderInt64`. It is a trap on the caller-named path only.

4. **The label and the cost were being traded against each other by both agents.**
   05a withheld ARCHITECTURAL because the fix looked small; 05b awarded it because
   the fix looked large. Plan 10.7 exists to stop that and it postdates both. I
   recommend the planner state, when this territory is folded in, that the F row's
   kind was settled by the rule and not by either agent's sizing, so the sizing
   question stays visibly open per escalation 1.

5. **I compiled nothing and ran nothing.** Every claim above is read from source
   or from `git show` on tagged trees. The claim that flipping
   `use_64bit_pointers` produces a broken module is INFERRED, as it is in all four
   prior documents. The tag census and the `make_value` count are mechanical and
   reproducible from the commands quoted.

6. **I did not re-verify the GitHub material.** Issue and PR content in both
   reports, including #6295, #6368, #6468, #6494, #5979, #8608 and #3177, is
   carried forward unchecked in this pass. The round-one adversaries verified it
   and I had no source-level reason to reopen it.

---

## 12. Cross-adversary

At the time of writing, `adversary2-05-2.md` does not exist. Checked by direct
`ls`. If it lands, this section is where the divergence goes.
