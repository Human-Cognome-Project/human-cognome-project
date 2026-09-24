# Adversary 2, round two, territory 05 — shelved 64-bit work

Adversary: adv2-05-2. Date: 2026-09-09. Fork at `ba0e81dce`, branch `master`.

Scope: settle the flat contradiction between the amended `report-05-shelved-64bit.md`
(hereafter **05a**) and `report-05b-shelved-64bit.md` (**05b**) on the `at_buffer`
pointer-width collision and the `ValueKind` tag; verify four further items; give
verdicts on correctness and completeness.

Method. Every claim below was re-derived from the source tree in this working
directory or from read-only `git`, before or independently of reading the
corresponding passage in either report. Every count is the output of an
enumeration reproduced in this file. Where I reproduce a report's number I say
so; where I reach a different one I give both.

Read in full for this pass: `PROJECT-PLAN.md`; `report-05-shelved-64bit.md`
§7.7 item F and §5/§3.2; `report-05b-shelved-64bit.md` §5.0, §13.4, §13.5, §13.6;
`adversary-05-1.md` §24.3 and §24.6; `adversary-05-2.md` §3, §14.1 and the
`snode.h` passage; `notes-05-shelved-64bit.md` and `notes-05b-shelved-64bit.md`
where the reports were unclear on provenance.

---

## 0. Summary of rulings

| # | Question | Ruling |
|---|---|---|
| 1 | Is the `ValueKind` tag read by `at_buffer`? | **No.** `at_buffer` reads `ptr_val.stype.dt`, never `ptr_val.flag`. 05b right. |
| 2 | Does the tag exist, get set, survive the name table, and drive load/store? | **Yes, all four.** 05a right. |
| 3 | Are 1 and 2 compatible? | **Yes.** The tag is on the output, not the input. Both reports state true facts. |
| 4 | Does tagging the production sites suffice, as 05a's remedy requires? | **No.** The tag is destroyed by `ir_->add` at `spirv_codegen.cpp:372` on every SNode path that reaches `at_buffer`. 05a's remedy is insufficient as stated. 05b right on the consequence. |
| 5 | Grade under PROJECT-PLAN §10 item 7 | **ARCHITECTURAL.** 05b's label is correct. 05a's withdrawal rests on a step the new definition forbids. |
| 6 | Blast radius | **Neither number is right.** 05a's 4 is the insufficient remedy. 05b's 147 is a regression surface presented as an edit surface. My derivation: 11 edit sites, 147 + 4 sites of behaviour change. §3.5. |
| 7 | The unguarded `u64_type()` defect | **Real, live, and understated.** 05a names three call sites; there are five live ones in two files, and the first to fire is not among the three 05a names. §4. |
| 8 | Sparse binding width, `snode.h:41/:45/:49` | **Verified,** with one enumeration correction to 05b. §5. |
| 9 | Module-wide addressing model withdrawn | **Verified,** and the shipped-release window is larger than either report states. §6. |
| 10 | Tag census | **Confirmed by enumeration.** 121 tags; 24 carry the file; 11 / 3 / 10. §7. |

---

## 1. The definition that governs, and what it changes

PROJECT-PLAN §10 item 7 now reads:

> *If every external thing were ideal today, would the obstacle still be there?*
> ENVIRONMENTAL if it exists because of something OUTSIDE this codebase...
> ARCHITECTURAL if it exists because of a decision INSIDE this codebase about how
> something is represented or structured.
>
> **Architectural does not mean hard, and environmental does not mean easy.**
> Grade the KIND first, then state the BLAST RADIUS separately.

This definition did not exist when 05a was amended, and 05a did not use it. 05a
graded against a definition it names in its own text:

> "it is not ARCHITECTURAL under the definition section 7 of this report uses —
> 'the approach was wrong or conflicts with something structural; does not
> expire' — because the replacement mechanism is present in the same file and
> already load-bearing elsewhere in it."
> — `report-05-shelved-64bit.md`, §7.7 item F

The clause after "because" is a **cost** argument. Under §10 item 7 the cost is
graded separately and may not enter the kind. So 05a's ground for withdrawal is
not available under the operative rule.

And 05a's own preceding sentence supplies the grading the new rule asks for:

> "It is not ENVIRONMENTAL: no driver, extension or hardware generation will fix
> it, because it is a decision internal to Taichi."

That sentence *is* the §10 item 7 test, answered, and its answer is
ARCHITECTURAL. **05a's facts, graded under the rule that now governs, give the
label 05a withdrew.** This is not a defect in 05a's investigation; it is a defect
in 05a's grading step, and it is exactly the incomparability the new definition
was written to end.

---

## 2. What is actually true at each point

Re-derived from source before reading either report's account of it.

### 2.1 The discriminator `at_buffer` reads

`taichi/codegen/spirv/spirv_codegen.cpp:2194-2220`:

```cpp
  spirv::Value at_buffer(const Stmt *ptr, DataType dt) {
    spirv::Value ptr_val = ir_->query_value(ptr->raw_name());

    if (ptr_val.stype.dt == PrimitiveType::u64) {           // :2197
      spirv::Value paddr_ptr = ir_->make_value(
          spv::OpConvertUToPtr,
          ir_->get_pointer_type(ir_->get_primitive_type(dt),
                                spv::StorageClassPhysicalStorageBuffer),
          ptr_val);
      paddr_ptr.flag = ValueKind::kPhysicalPtr;             // :2203
      return paddr_ptr;                                     // :2204
    }
    ...
```

The decision at `:2197` reads `ptr_val.stype.dt`. It does not read
`ptr_val.flag`. There is no reference to `ValueKind` anywhere in `at_buffer`
except the assignment at `:2203`, which is on the returned value.

The same sniff appears three times, enumerated:

```
$ grep -n "stype.dt == PrimitiveType::u64" taichi/codegen/spirv/spirv_codegen.cpp
2197:    if (ptr_val.stype.dt == PrimitiveType::u64) {      # at_buffer
2227:    if (ptr_val.stype.dt == PrimitiveType::u64) {      # load_buffer
2249:    if (ptr_val.stype.dt == PrimitiveType::u64) {      # store_buffer
```

Three sites, not one. Neither report gives this count. `:2227` and `:2249` use
the sniff for a different purpose — selecting `ti_buffer_type` — but they read
the same overloaded fact and would need the same replacement.

**05b is correct: `at_buffer` cannot be reading the tag.** Not because the tag is
uniformly absent, but because the tag is not consulted at all at the decision.

### 2.2 The tag exists, is set, survives, and is read

Every leg of 05a's account verified.

- **Exists.** `enum class ValueKind` at `taichi/codegen/spirv/spirv_ir_builder.h:69-79`,
  nine members, `kPhysicalPtr` at `:75`. `ValueKind flag{ValueKind::kNormal};` at
  `:88`. Both reports cite the enum as `:69-79` and both are right; I first read
  it as `:68-78` and that was my error, not theirs.
- **Is set.** `paddr_ptr.flag = ValueKind::kPhysicalPtr;` at `spirv_codegen.cpp:2203`.
  05b's bracketed correction to both adversaries — that the assignment is `:2203`
  and `:2204` is the `return` — is verified and correct.
- **Survives the name table.** `register_value` at `spirv_ir_builder.cpp:1299-1309`
  stores the whole `Value` by value into `value_name_tbl_` at `:1308`;
  `query_value` at `:1311-1317` returns `it->second` by value at `:1314`. The
  `flag` is a plain member of an aggregate, so it round-trips.
- **Is read, and drives load and store.** `load_variable` at `:1270-1285` asserts
  the flag is one of three pointer kinds at `:1271-1273` and branches on
  `kPhysicalPtr` at `:1275`, emitting `OpLoad` with `spv::MemoryAccessAlignedMask`
  at `:1277-1280`. `store_variable` at `:1286-1298` asserts at `:1287-1288` and
  branches at `:1290`.

**05a is correct on all four legs.** Its citation `:1271-1283` for `load_variable`
covers the assert and both arms; 05b's `:1270-1285` is the whole function. Both
accurate.

### 2.3 The two accounts are compatible

The prompt's hypothesis is right. A tag that exists and is set, but sits on the
output rather than the input, makes both accounts true. 05a states this itself,
and states it as the precision that 05b's predecessor missed:

> "The tag at `:2203` is set on the pointer `at_buffer` *produces*, after the
> branch is taken. It is not set on the address value `at_buffer` *consumes*."

That sentence is correct, and it is the reconciling fact. Nothing in 05b
contradicts it; 05b's §13.5 asserts the same thing in the same words.

**So there is no contradiction of fact between the two reports.** The
contradiction is entirely in what follows from the shared facts.

### 2.4 One imprecision in 05b, stated so the record reconciles

05b writes "both sides of the decision arrive untagged" and "the tag is `kNormal`
on both sides of the decision." The second is true for the two values that matter
and is the load-bearing claim. The first is loose: the values reaching `at_buffer`
are not untagged, they are tagged *non-discriminatingly*, and not all with the
same value. Enumerating the seven producers (§3.2) gives `kNormal` for five of
them and `kConstant` for `GlobalTemporaryStmt` at `spirv_codegen.cpp:708-710`,
whose value goes to `at_buffer` through the `ptr_to_buffers_` entry set at `:711`.

This does not disturb 05b's conclusion. `kConstant` no more distinguishes a
device address from a buffer offset than `kNormal` does. Recorded because "the
tag is `kNormal` on both sides" is exactly true of the two sides of *this*
decision, and "everything arrives untagged" is not exactly true of the input set.

---

## 3. The question that actually divides them, settled from source

Reduced to one proposition, this is where 05a and 05b are genuinely opposed:

> **Does a `ValueKind` tag applied at the production sites survive to `at_buffer`'s
> consumption point?**

05a's remedy requires yes: "To replace the sniff at `:2197` the tag would have to
be populated at the production sites — the four `make_pointer` call sites at
`:355`, `:371`, `:408`, `:506`, and the `ExternalPtrStmt` base-address path at
`:790-792`." 05b's requires no: "the obstacle is larger... changing `make_value`
at `spirv_ir_builder.h:291`."

### 3.1 The answer is no. Traced on the SNode path.

`make_value`, `taichi/codegen/spirv/spirv_ir_builder.h:290-298`:

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

The result's flag is a function of `out_type.flag` alone. **No operand's flag is
consulted.** An integer byte offset has `out_type.flag == TypeKind::kPrimitive`
(`SType::flag` defaults to `kPrimitive` at `spirv_ir_builder.h:59`), so the
result is `kNormal` whatever its operands carried.

Now the chain from a `make_pointer` production site to `at_buffer`:

| Step | Site | Operation | Resulting flag |
|---|---|---|---|
| 1 | `spirv_codegen.cpp:355` | `make_pointer(0)` → `uint_immediate_number` → `get_const` → `new_value(dtype, ValueKind::kConstant)` at `spirv_ir_builder.cpp:1502` | `kConstant` |
| 2 | `spirv_codegen.cpp:356` | `register_value` | `kConstant`, survives |
| 3 | `spirv_codegen.cpp:372` | `ir_->add(input_ptr_val, offset)` → `DEFINE_BUILDER_BINARY_USIGN_OP(add, Add)` at `spirv_ir_builder.cpp:1062` → `make_value(OpIAdd, a.stype, a, b)` | **`kNormal` — tag destroyed** |
| 4 | `spirv_codegen.cpp:373` | `register_value` | `kNormal` |
| 5 | `spirv_codegen.cpp:377` | `ptr_to_buffers_[stmt] = BufferInfo(BufferType::Root, root)` | (map entry, not a tag) |
| 6 | `spirv_codegen.cpp:2195` | `at_buffer` does `query_value` | `kNormal` |

Step 3 is the refutation, and it is not avoidable. `ptr_to_buffers_` is written
for an SNode only at `:377`, inside `visit(GetChStmt *)` and only when
`out_snode->is_place()`. So **every SNode pointer that ever reaches `at_buffer`
has passed through the `ir_->add` at `:372` at least once.** A tag set on
`make_pointer`'s output at `:355` or `:371` cannot reach `:2197` on any path.

`SNodeLookupStmt` at `:504-510` compounds it — `ir_->mul` at `:507` then
`ir_->add` at `:508`, two more erasures. `MatrixPtrStmt` at `:331` does
`ir_->add(origin_val, ir_->cast(origin_val.stype, offset_val))`, a third.

**05a's remedy, as written, does not work.** Tagging the four `make_pointer`
sites is necessary and insufficient. 05b is right that the remedy reaches
`make_value`.

### 3.2 The seven production sites, enumerated

05a says "four `make_pointer` call sites... and the `ExternalPtrStmt`
base-address path", i.e. five. Enumerating instead the `register_value` calls
whose value can be an `at_buffer` input gives seven:

```
$ grep -n "register_value(" taichi/codegen/spirv/spirv_codegen.cpp   # 33 hits total
```

Of those 33, the pointer-producing ones are:

| Line | Statement | Producer | Flag today |
|---|---|---|---|
| 334 | `MatrixPtrStmt` | `OpAccessChain` / `add` | `kVariablePtr` or `kNormal` |
| 356 | `GetRootStmt` | `make_pointer(0)` | `kConstant` |
| 373 | `GetChStmt` | `add` | `kNormal` |
| 510 | `SNodeLookupStmt` | `mul` + `add` | `kNormal` |
| 710 | `GlobalTemporaryStmt` | `int_immediate_number` | `kConstant` |
| 792 | `ExternalPtrStmt`, physical branch | `load_variable` + `add` | `kNormal` |
| 794 | `ExternalPtrStmt`, bound branch | shift/mul/add chain | `kNormal` |

Seven, not four and not five. `make_pointer`'s four call sites are `:355`,
`:371`, `:408` and `:506`; of these only `:355` is a production site in the sense
that matters, because `:371`, `:408` and `:506` produce *offsets* that are
immediately consumed by an `add` or `mul`.

### 3.3 The ndarray side, confirming 05b's leg 3

`visit(ExternalPtrStmt *)`, `spirv_codegen.cpp:783-795`:

```cpp
    if (caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)) {
      ...
      spirv::Value addr = ir_->load_variable(addr_ptr, ir_->u64_type());   // :789
      addr = ir_->add(addr, ir_->make_value(spv::OpSConvert, ir_->u64_type(),
                                            linear_offset));               // :790-791
      ir_->register_value(stmt->raw_name(), addr);                         // :792
```

`load_variable` returns `new_value(res_type, ValueKind::kNormal)` at
`spirv_ir_builder.cpp:1274`. `ir_->add` then routes through `make_value` with a
`kPrimitive` out type. So the genuine physical device address reaches `at_buffer`
as `kNormal`, identical to an SNode offset. **05b's leg 3 verified.**

### 3.4 05b's leg 4, on `ptr_to_buffers_`, verified but incomplete

05b rules out `ptr_to_buffers_` as an alternative discriminator on the ground
that `visit(ExternalPtrStmt *)` sets it at `:797-801`, outside the capability
branch, so under the capability an ndarray pointer carries both a `u64` value and
a map entry. **That is verified.** `:783-795` is the branch, `:797-801` is
outside it.

But it rules on the *presence* of an entry and not on its *value*, and the value
differs:

```
$ grep -n "ptr_to_buffers_" taichi/codegen/spirv/spirv_codegen.cpp
319:          ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];   # MatrixPtrStmt, shared alloca
326:        ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];     # MatrixPtrStmt, gtmp origin
332:      ptr_to_buffers_[stmt] = ptr_to_buffers_[stmt->origin];       # MatrixPtrStmt, byte offset
376:      TI_ASSERT(ptr_to_buffers_.count(stmt) == 0);
377:      ptr_to_buffers_[stmt] = BufferInfo(BufferType::Root, root);  # GetChStmt, place
711:    ptr_to_buffers_[stmt] = BufferType::GlobalTmps;                # GlobalTemporaryStmt
798:      ptr_to_buffers_[stmt] = {BufferType::ExtArr, arg_id};        # ExternalPtrStmt, array
800:      ptr_to_buffers_[stmt] = BufferType::Args;                    # ExternalPtrStmt, scalar
2212:    spirv::Value buffer = get_buffer_value(ptr_to_buffers_.at(ptr), dt);
2635:  std::unordered_map<const Stmt *, BufferInfo> ptr_to_buffers_;
```

Seven writes, one read, one declaration. An SNode place gets `BufferType::Root`;
an ndarray gets `BufferType::ExtArr` or `BufferType::Args`. The map is keyed on
`const Stmt *`, is propagated explicitly at `:319`, `:326` and `:332`, and
**arithmetic cannot erase it** — that is precisely the property the `Value` flag
lacks. The physical-address case and the SNode case therefore *are* separable by
`ptr_to_buffers_.at(ptr)`'s buffer type together with the compile-time
`caps_->get(...)`, without touching the value model.

I am not proposing this and PROJECT-PLAN §10 items 1 and 4 forbid me designing
it. I record it because **the blast radius both reports state is conditional on a
tag-based remedy that neither report established as the only one available**, and
because 05b's rejection of `ptr_to_buffers_` does not reach this. It goes to
Escalations.

### 3.5 The blast radius, derived

05b: "changing `make_value`... `grep -rn "make_value(" taichi/codegen/spirv/`
returns 148 occurrences, 147 of them call sites."

Reproduced exactly:

```
$ grep -rn "make_value(" taichi/codegen/spirv/ | wc -l
148
$ grep -rn "Value make_value(" taichi/codegen/spirv/
taichi/codegen/spirv/spirv_ir_builder.h:291:  Value make_value(spv::Op op, const SType &out_type, Args &&...args) {
```

148 hits, 1 definition, **147 call sites**. Split by file: `spirv_codegen.cpp` 83,
`spirv_ir_builder.cpp` 64, `spirv_ir_builder.h` 1 (the definition).

**But 147 is not the blast radius under §10 item 7, which asks for the change.**
`make_value` is one shared template. Changing its propagation rule edits one
site. The 147 are the sites whose *behaviour* changes — a regression surface, not
an edit surface, and the two are not interchangeable. PROJECT-PLAN §10 item 7
says "state the BLAST RADIUS separately and concretely, in files and call sites",
and 05b's presentation makes an architectural obstacle look 35 times more
expensive than the edit it requires. That is the same category of overweighting
the plan records itself committing three times.

My derivation, for the tag-based remedy:

| Set | Sites | Count |
|---|---|---|
| The discriminator read | `spirv_codegen.cpp:2197`, `:2227`, `:2249` | 3 |
| The propagation rule | `spirv_ir_builder.h:291` | 1 |
| Production sites needing an explicit tag | `spirv_codegen.cpp:334`, `356`, `373`, `510`, `710`, `792`, `794` | 7 |
| **Edit surface** | **3 files** | **11** |
| Ops on the pointer chain whose propagation must be specified | `add`/`mul` wrappers at `spirv_ir_builder.cpp:1062`, `:1064`; direct `make_value` at `spirv_codegen.cpp:771`, `:790`, `:2214` | 5 |
| Regression surface | all `make_value` call sites | 147 |

**11 edits across 3 files; 5 operations needing a stated propagation rule; 147
call sites of changed behaviour.** Neither report's number. 05a's 4 is the
insufficient remedy; 05b's 147 is the regression surface.

### 3.6 Ruling

- **On the facts: no contradiction.** Every enumerated factual claim in both
  accounts is true. The reconciling fact — the tag is on the output — is stated
  correctly in both.
- **On the consequence: 05b is right.** A tag set at production does not survive
  to consumption. `spirv_codegen.cpp:372` destroys it on every SNode path that
  reaches `at_buffer`. 05a's remedy is insufficient as written and its "bounded
  change to `at_buffer`, `load_buffer`, `store_buffer` and five production sites"
  does not hold.
- **On the label: ARCHITECTURAL, 05b right.** The overloading of pointer width as
  an addressing-scheme tag is a decision inside this codebase. No driver,
  toolchain, hardware generation or specification revision removes it. It does
  not expire. Under §10 item 7 that is the whole test, and 05a's "the replacement
  mechanism is present in the same file" is a cost argument the rule bars from
  the kind.
- **On the blast radius: neither.** 11 edits, 147 sites of changed behaviour, and
  a cheaper non-tag route at §3.4 that neither report priced.

The two orders of magnitude in the reported cost are not two orders of magnitude
in the work. They are the gap between an insufficient remedy and an inflated
count of a sufficient one.

---

## 4. Verification item 1 — the unguarded `u64_type()`

**05a's claim is true. It is understated in two ways and I found two more live
sites than it names.**

### 4.1 The accessor is unguarded; the type is conditional

`taichi/codegen/spirv/spirv_ir_builder.h:529-534`:

```cpp
  SType i64_type() const {
    return t_int64_;
  }
  SType u64_type() const {
    return t_uint64_;
  }
```

No capability check. The fields are assigned only under the capability,
`spirv_ir_builder.cpp:166-169`:

```cpp
  if (caps_->get(cap::spirv_has_int64)) {
    t_int64_ = declare_primitive_type(get_data_type<int64>());
    t_uint64_ = declare_primitive_type(get_data_type<uint64>());
  }
```

`SType::id` defaults to `0` at `spirv_ir_builder.h:51`. SPIR-V ids begin at 1, so
an undeclared `t_uint64_` yields a type reference of `%0`, which is not a valid
id.

The contrast is the point. `IRBuilder::get_primitive_type` at
`spirv_ir_builder.cpp:310-328` **is** guarded, for i64 at `:311-314` and u64 at
`:325-328`, each with `TI_ERROR("Type {} not supported.", ...)`. The same
codebase guards one route to the same field and leaves the other open.

### 4.2 The live call sites, enumerated

```
$ grep -rn "u64_type()\|i64_type()" --include=*.cpp --include=*.h taichi/ \
    | grep -v "SType u64_type\|SType i64_type"
taichi/codegen/spirv/spirv_types.cpp:403:        vt = spir_builder_->i64_type();
taichi/codegen/spirv/spirv_types.cpp:415:        vt = spir_builder_->u64_type();
taichi/codegen/spirv/spirv_codegen.cpp:787:          ir_->get_pointer_type(ir_->u64_type(), spv::StorageClassUniform),
taichi/codegen/spirv/spirv_codegen.cpp:789:      spirv::Value addr = ir_->load_variable(addr_ptr, ir_->u64_type());
taichi/codegen/spirv/spirv_codegen.cpp:790:      addr = ir_->add(addr, ir_->make_value(spv::OpSConvert, ir_->u64_type(),
taichi/codegen/spirv/spirv_codegen.cpp:2320:      return ir_->uint_immediate_number(ir_->u64_type(), offset);
```

**Six call sites; five live.** `:2320` sits inside `if (use_64bit_pointers)` and
`const bool use_64bit_pointers = false;` at `spirv_codegen.cpp:82`, so it is dead
today. 05a's amendment 5 does name `:2320` separately, correctly, as part of the
`make_pointer` finding.

**05a names `:787`, `:789` and `:790`. It does not name `spirv_types.cpp:403` or
`:415`, and `:415` fires first.**

### 4.3 Why `spirv_types.cpp:415` fires first, and on the same trigger

`translate_ti_type`, `taichi/codegen/spirv/spirv_types.cpp:484-513`:

```cpp
  if (t->is<PointerType>()) {
    if (has_buffer_ptr) {
      return ir_module.emplace_back<IntType>(/*num_bits=*/64,
                                             /*is_signed=*/false);   // :491-492
```

`has_buffer_ptr` is `caps_->get(DeviceCapability::spirv_has_physical_storage_buffer)`,
set at `spirv_codegen.cpp:2339-2340` in `compile_args_struct`, at `:2415-2416`,
and at `:2490-2491`. The resulting 64-bit unsigned `IntType` reaches
`Translate2Spirv::visit_int_type` at `spirv_types.cpp:393-419`, whose 64-bit
unsigned arm at `:414-415` calls the unguarded `u64_type()` and stores `vt.id`.

So the **argument struct's own member type** is `%0` before any kernel body is
emitted. `compile_args_struct` runs for any kernel with arguments; `:787-790`
runs only inside `visit(ExternalPtrStmt *)` (the function opens at
`spirv_codegen.cpp:734`), i.e. only for a kernel with an ndarray or external
array argument. The args-struct site has the wider trigger.

*Correction available to both reports:* `IRBuilder::from_taichi_type` at
`spirv_ir_builder.cpp:334-352` contains the same unguarded `return t_uint64_;` at
`:376` on its pointer arm. It is **dead code** — `grep -rn "from_taichi_type"` over
`taichi/`, `c_api/` and `python/` returns three hits, the definition at
`spirv_ir_builder.cpp:334`, its own recursive call at `:347`, and the declaration
at `spirv_ir_builder.h:343`. Zero external callers. Neither report cites it, and
neither should; recorded so a later pass does not add it as a fourth defect.

### 4.4 Reachability, and it is not through the out-of-scope front end

Two live routes set arbitrary capabilities with no consistency check.

**Route one, the C API.** `c_api/src/taichi_core_impl.cpp:317-334`:

```cpp
void ti_set_runtime_capabilities_ext(
    TiRuntime runtime, uint32_t capability_count,
    const TiCapabilityLevelInfo *capabilities) {
  ...
  taichi::lang::DeviceCapabilityConfig devcaps;
  for (uint32_t i = 0; i < capability_count; ++i) {
    const auto &cap_level_info = capabilities[i];
    devcaps.set((taichi::lang::DeviceCapability)cap_level_info.capability,
                cap_level_info.level);
  }
  runtime2->get().set_caps(std::move(devcaps));
```

No implication, no validation, and a C-style cast straight from a user integer to
the enum. This is C++, not the Python front end, so PROJECT-PLAN §1.2 does not
exempt it.

**Route two, the AOT builder.** `taichi/program/program.cpp:514-538`,
`translate_devcaps`, splits `name` or `name=value` strings and calls
`cfg.set(devcap, value)`. Its only defaulting is `spirv_version` at `:534-536`.
Reached from `Program::make_aot_module_builder` at `:540-556`, whose Python entry
is `python/taichi/aot/module.py:86-110`.

Passing `spirv_has_physical_storage_buffer` alone through either route gives
`spirv_has_int64 == 0`.

**What the detection path does instead.** `taichi/rhi/vulkan/vulkan_device_creator.cpp:630-633`
sets `spirv_has_int64` from `device_supported_features.shaderInt64`, and
`:821` requires the same `shaderInt64` before it would set
`spirv_has_physical_storage_buffer` at `:826`. So ordinary Vulkan device creation cannot
produce the inconsistent pair. The defect is confined to the two manual routes.

### 4.5 Validation is off, verified

`taichi/codegen/spirv/spirv_codegen.cpp:2710`:

```cpp
  spirv_opt_options_.set_run_validator(false);
```

And `enable_validation_layer{false}` at `taichi/rhi/vulkan/vulkan_device_creator.h:62`.
So an invalid module is optimised without a check and handed to the driver.

Two aggravating consequences neither report reaches:

1. The module also emits `OpCapability PhysicalStorageBufferAddresses`
   (`spirv_ir_builder.cpp:73-77`) and `OpMemoryModel AddressingModelPhysicalStorageBuffer64`
   (`:113-122`) while **omitting** `OpCapability Int64` (`:64-66`, gated). So the
   module is invalid twice over, and the second failure is a missing capability
   declaration rather than a bad id.
2. `IRBuilder::get_pointer_type` at `spirv_ir_builder.cpp:407-424` caches on
   `std::make_pair(value_type.id, storage_class)`. With `t_int64_.id == 0` and
   `t_uint64_.id == 0`, pointer types to *different* undeclared element types
   collide on the same cache key. So the failure is not merely a bad reference;
   it can alias two distinct pointer types.

### 4.6 Ruling on item 1

**Verified and true.** A user-reachable C API call produces a SPIR-V module
referencing an undeclared type, with no validation between codegen and the
driver. 05a is right to call it live and right that neither round-one adversary
stated it.

On "the most serious live finding in the territory" I record the properties
rather than the ranking, per PROJECT-PLAN §10 item 2. It is **live** — no source
change is needed to reach it. It is **silent** — no diagnostic on any of the five
live sites, against a guarded sibling at `spirv_ir_builder.cpp:310-328`. It is
**conditional** — it requires the manual capability route, and ordinary Vulkan
detection cannot produce it. And it becomes **more** exposed, not less, if the
project restores the capability, because restoring it multiplies the code paths
that assume the pair travel together. Whether that outranks the `at_buffer`
collision is the arbiter's call; the collision blocks the work, this one corrupts
output when the work is attempted incorrectly.

---

## 5. Verification item 2 — the sparse binding width

**Verified independently. Both reports are right. One enumeration correction to
05b.**

### 5.1 The three `int` fields

`taichi/ir/snode.h:37-54`, `struct AxisExtractor`:

| Field | Line | Type |
|---|---|---|
| `num_elements_from_root` | `taichi/ir/snode.h:41` | `int num_elements_from_root{1};` |
| `shape` | `taichi/ir/snode.h:45` | `int shape{1};` |
| `acc_shape` | `taichi/ir/snode.h:49` | `int acc_shape{1};` |

Against `int64 num_cells_per_container{1};` at `taichi/ir/snode.h:97` and
`int64 max_num_elements() const` at `:306-308`. **The 64-bit half is already
present; the extractors were left behind.** Confirmed.

### 5.2 The ordering: truncation precedes the warning

`taichi/ir/snode.cpp:89-101`:

```cpp
  int64 acc_shape = 1;                                                    // :89
  for (int i = taichi_max_num_indices - 1; i >= 0; i--) {                 // :90
    // casting to int32 in extractors.                                    // :91
    new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);       // :92
    acc_shape *= new_node.extractors[i].shape;                            // :93
  }                                                                       // :94
  if (acc_shape > std::numeric_limits<int>::max()) {                      // :95
    ErrorEmitter(
        TaichiIndexWarning(), &dbg_info,
        "SNode index might be out of int32 boundary but int64 indexing is not "
        "supported yet. Struct fors might not work either.");             // :96-99
  }
  new_node.num_cells_per_container = acc_shape;                           // :101
```

The narrowing store at `:92` runs on **every** iteration of the loop at
`:90-94`. The test at `:95` runs once, afterwards, on the untruncated local
`int64`. **The warning is a post-hoc detector for damage already done.** 05b's
"this report previously read it backwards" is correct, and 05a's §7.7 item I
states the same ordering.

It is a warning, not a throw: `TaichiIndexWarning` reaches `ErrorEmitter`, not
`TI_ERROR`. Execution continues with the truncated fields.

### 5.3 The assertion never fires on a sparse path

`taichi/transforms/demote_dense_struct_fors.cpp`:

- `TI_ASSERT(total_n <= std::numeric_limits<int>::max());` at `:29`, inside
  `convert_to_range_for` which opens at `:12`.
- Its only caller, `:114-119`:

```cpp
void maybe_convert(OffloadedStmt *stmt) {
  if ((stmt->task_type == TaskType::struct_for) &&
      stmt->snode->is_path_all_dense) {
    convert_to_range_for(stmt);
  }
}
```

- `bool is_path_all_dense{true};` at `taichi/ir/snode.h:114`, cleared at
  `taichi/ir/snode.cpp:20`:
  `new_ch->is_path_all_dense = (is_path_all_dense && !new_ch->need_activation());`
- `taichi/transforms/offload.cpp:192` reads the same flag for the same purpose.

PROJECT-PLAN §4.1 states "Sparsity is required." A `pointer`, `bitmasked` or
`dynamic` node anywhere on the path clears the flag for that node and everything
below it, so `convert_to_range_for` is never entered and `:29` never executes.
**Verified. Both reports correct.**

For completeness on the "runs by default" half, which neither report's ruling
depends on: `demote_dense_struct_fors = true;` at
`taichi/program/compile_config.cpp:18` and again at `:73` under
`if (arch_uses_spirv(arch))`, and the pass runs at
`taichi/transforms/compile_to_offloads.cpp:191-192`. On by default; irrelevant to
a sparse tree.

### 5.4 Which of the three binds, and one correction to 05b's enumeration

05b's refinement — that `num_elements_from_root` at `:41` is the field on the
SPIR-V path — is verified:

```
$ grep -rn "num_elements_from_root" --include=*.cpp --include=*.h taichi/
taichi/ir/snode.h:41              declaration
taichi/ir/snode.cpp:22-23         accumulated in insert_children
taichi/ir/snode.cpp:83            accumulated in create_node
taichi/ir/snode.cpp:176           returned by shape_along_axis, as int
taichi/analysis/offline_cache_util.cpp:112   serialised into the cache key
taichi/codegen/spirv/snode_struct_compiler.cpp:121   the SPIR-V consumer
```

`snode_struct_compiler.cpp:117-122` accumulates
`sn_desc.total_num_cells_from_root *= e.num_elements_from_root;` over all
extractors, for every SNode type including sparse ones. The destination is
`size_t`, so wide, but every factor was already truncated to `int`. And there is
**no** overflow detector on this field anywhere — no warning, no assert, unlike
`acc_shape` at `snode.cpp:95`. Verified.

**Correction.** 05b writes "`acc_shape` (`:49`) has two consumers:
`taichi/codegen/llvm/struct_llvm.cpp:174` and `:176`, and
`taichi/transforms/demote_dense_struct_fors.cpp:74` and `:76`." My enumeration
returns five reader lines in **three** files:

```
$ grep -rn "acc_shape" --include=*.cpp --include=*.h taichi/ | grep -v "^taichi/ir/snode"
taichi/analysis/offline_cache_util.cpp:114:    serializer(extractor.acc_shape);
taichi/codegen/llvm/struct_llvm.cpp:174
taichi/codegen/llvm/struct_llvm.cpp:176
taichi/transforms/demote_dense_struct_fors.cpp:74
taichi/transforms/demote_dense_struct_fors.cpp:76
```

`offline_cache_util.cpp:114` is a cache-key serialiser, not a width consumer, so
05b's substantive point stands untouched. Recorded because PROJECT-PLAN §10 item
8 requires a stated count to reconcile against its own enumeration, and "two
consumers" does not reconcile against five lines in three files.

### 5.5 One further truncation, confirming 05b

`taichi/transforms/demote_dense_struct_fors.cpp:19`:
`std::array<int, taichi_max_num_indices> total_shape;`, multiplied into at
`:23-25` with no check, while `int64 total_n` beside it at `:18` **is** checked at
`:29`. Verified. On the dense path only, so not this project's constraint, but
real.

---

## 6. Verification item 3 — the module-wide addressing model

**Withdrawal verified from source and from the tag. The shipped window is larger
than either report states.**

### 6.1 The mechanism

`taichi/codegen/spirv/spirv_ir_builder.cpp:113-127`: one `OpMemoryModel` per
module, `AddressingModelPhysicalStorageBuffer64` under the capability at
`:118-122`, `AddressingModelLogical` otherwise at `:124-126`. Module granularity,
no per-pointer commitment.

### 6.2 v1.1.3 settles it

```
$ git show v1.1.3:taichi/codegen/spirv/spirv_ir_builder.cpp | grep -n AddressingModel
114:        .add_seq(spv::AddressingModelPhysicalStorageBuffer64,
119:        .add_seq(spv::AddressingModelLogical, spv::MemoryModelGLSL450)

$ git show v1.1.3:taichi/codegen/spirv/spirv_codegen.cpp | grep -n use_64bit_pointers
74:  const bool use_64bit_pointers = false;
2063:    if (use_64bit_pointers) {

$ git show v1.1.3:taichi/rhi/vulkan/vulkan_device_creator.cpp | grep -n -B4 spirv_has_physical_storage_buffer
734-      if (CHECK_VERSION(1, 3) ||
735-          buffer_device_address_feature.bufferDeviceAddress) {
736-        if (device_supported_features.shaderInt64) {
737-          ti_device_->set_cap(
738:              DeviceCapability::spirv_has_physical_storage_buffer, true);
```

At v1.1.3 the capability was **live** — no `__APPLE__` guard, no `&& false` — the
module therefore declared `AddressingModelPhysicalStorageBuffer64`, and
`use_64bit_pointers` was `false`, so SNode addressing was 32-bit bound-buffer.
**The physical addressing model and 32-bit SNode addressing shipped together in a
tagged release.** The obstacle is refuted from a shipped artefact, not by
inference. Verified; both reports correct to withdraw it.

### 6.3 The window is 11 releases on all platforms, 14 on Linux and Windows

The reports say "eleven shipped releases". My enumeration (§7) gives **11** tags
enabled on all platforms and **3 more** enabled everywhere except macOS. On the
platforms this project runs on, the physical addressing model shipped in **14**
tagged releases, v0.9.0 through v1.2.2. "Eleven" is correct for the unguarded
window and understates the refutation by three tags. Neither report says which
window it means.

### 6.4 Note on the enabling gate at v1.1.3

`if (device_supported_features.shaderInt64)` at v1.1.3 `:736` is the same gate
present today at `vulkan_device_creator.cpp:821`. This is why §4's defect never
surfaced upstream: the detection path has always paired the two capabilities. The
defect lives only on the manual routes, which did not exist in the same form when
the feature was live.

---

## 7. Verification item 4 — the tag census, by enumeration

### 7.1 Total tags

```
$ git tag | wc -l
121
```

Cross-checked by family, summing to 121:

| Family | Count | | Family | Count |
|---|---|---|---|---|
| 0.3 | 1 | | 0.9 | 3 |
| 0.4 | 3 | | 1.0 | 5 |
| 0.5 | 9 | | 1.1 | 3 |
| 0.6 | 42 | | 1.2 | 3 |
| 0.7 | 30 | | 1.3 | 1 |
| 0.8 | 12 | | 1.4 | 2 |
| | | | 1.5 | 1 |
| | | | 1.6 | 1 |
| | | | 1.7 | 5 |

1+3+9+42+30+12+3+5+3+3+1+2+1+1+5 = **121**. 112 carry a `v` prefix, 9 do not.
`0.7.31` and `v0.7.31` are two tag names on the same version, so 121 names over
120 distinct versions.

**Confirmed: 121.** Adversary 1's original "122 release tags" (`adversary-05-1.md:90`)
is one over; both amendment agents put adversary 2 right and my count agrees with
adversary 2.

### 7.2 Tags carrying the capability, three-state

Mechanically, over every tag, locating `vulkan_device_creator.cpp` at whichever
path that tag uses, and classifying the eight lines above the `set_cap` call:

| Tags | State |
|---|---|
| v0.9.0, v0.9.1, v0.9.2, v1.0.0, v1.0.1, v1.0.2, v1.0.3, v1.0.4, v1.1.0, v1.1.2, v1.1.3 | **ENABLED, all platforms — 11** |
| v1.2.0, v1.2.1, v1.2.2 | enabled except macOS (`#if !defined(__APPLE__)`) — **3** |
| v1.3.0, v1.4.0, v1.4.1, v1.5.0, v1.6.0, v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4 | **DEAD** (`#if !defined(__APPLE__) && false`) — **10** |

11 + 3 + 10 = **24 tags carry the file with the capability in it.**

**This reproduces adversary 1's corrected census and adversary 2's census
exactly: 24 / 11 / 3 / 10.** Adversary 1's original 21 / 8 / 3 / 10 was three
short, and the three are v0.9.0, v0.9.1 and v0.9.2, exactly as adversary 2 states
at its §3. **The amendment agents' arbitration is confirmed by independent
enumeration.**

### 7.3 The two disabling steps, dated

```
$ git log -1 --format='%H %ad %an %s' --date=short d20f55dc8
d20f55dc8a48ebaf016205d8eaa1bd0a0681f55d 2022-10-30 PENGUINLIONG [aot] Disabled physical storage buffer temporarily (#6468)

$ git log -1 --format='%H %ad %s' --date=short 7705f688a
7705f688ac99ee6736dbd5bbda9e9c81b9d06c9b 2022-02-08 [vulkan] Add buffer device address (physical pointers) support & other improvements (#4221)
```

Introduced 2022-02-08; narrowed to non-macOS for issue 6295; killed with
`&& false` on 2022-10-30 by `d20f55dc8`, which is contained in exactly the 10
tags in the DEAD row (`git tag --contains d20f55dc8 | wc -l` → 10). The current
tree still carries `#if !defined(__APPLE__) && false` at
`vulkan_device_creator.cpp:825`, guarding the `set_cap` at `:826`, and the c_api
counterpart at `c_api/src/taichi_vulkan_impl.cpp:45-51` is commented out. **No
live site in this tree sets the capability from detection.**

---

## 8. Verdicts

### 8.1 Is 05a correct?

**Not entirely. One ruling is wrong and one remedy does not work.**

| Claim | Verdict |
|---|---|
| `ValueKind` exists, is set at `:2203`, survives `register_value`/`query_value`, drives `load_variable` | **CORRECT**, all four legs |
| The tag is on `at_buffer`'s output, not its input | **CORRECT**, and it is the reconciling fact |
| Adversary 1's "no third thing" sentence is factually wrong | **CORRECT** |
| The remedy is tagging the four `make_pointer` sites plus the `ExternalPtrStmt` path | **WRONG.** The tag is destroyed by `ir_->add` at `spirv_codegen.cpp:372` on every SNode path reaching `at_buffer`. §3.1 |
| Therefore not ARCHITECTURAL | **WRONG** under PROJECT-PLAN §10 item 7, and wrong on its own premise once §3.1 removes the "bounded" step |
| The unguarded `u64_type()` defect | **CORRECT**, and understated by two live sites. §4 |
| Sparse binding width at `snode.h:41/:45/:49` | **CORRECT.** §5 |
| Addressing model withdrawn | **CORRECT**, window understated by three tags. §6 |

The label is the material error. It is the one finding in this territory that
changes what the planner reads: 05a's closing sentence — "Neither of the two
obstacles this report previously called architectural survives that label...
That is a materially more favourable picture than the previous version of this
report handed the planner" — is the opposite of what §10 item 7 yields, and 05a
flags it itself as "the more agreeable finding". It was checked three ways for
its facts, which all hold, and not at all for its grading step, which does not.

### 8.2 Is 05b correct?

**Yes on every ruling. Two imprecisions, neither load-bearing.**

| Claim | Verdict |
|---|---|
| `make_value` erases the tag | **CORRECT**, with the refinement that it overwrites to `kVariablePtr` when `out_type.flag == kPtr` rather than always to `kNormal`; either way no operand flag is consulted |
| The physical address at `:789-791` arrives `kNormal` | **CORRECT** |
| Both sides of the decision arrive untagged | **CORRECT in substance, loose in wording.** `GlobalTemporaryStmt` arrives `kConstant`. §2.4 |
| `at_buffer` cannot be reading the tag | **CORRECT** |
| `ptr_to_buffers_` cannot serve as the discriminator | **VERIFIED as to presence of an entry; does not reach the entry's value.** §3.4 |
| ARCHITECTURAL | **CORRECT** under §10 item 7 |
| Blast radius 147 call sites | **OVERSTATED.** Regression surface, not edit surface. 11 edits. §3.5 |
| Sparse binding width and the assert gate | **CORRECT.** §5 |
| Addressing model withdrawn from v1.1.3 | **CORRECT.** §6 |

### 8.3 Are they complete?

**05a: no.** Three gaps.

1. The `u64_type()` defect it correctly raises is missing `spirv_types.cpp:403`
   and `:415`, and `:415` has the wider trigger than the three sites it names
   (§4.2, §4.3).
2. Its remedy is not traced past `spirv_codegen.cpp:372` (§3.1).
3. It grades against its own §7 definition and does not apply §10 item 7.

**05b: nearly.** Two gaps.

1. Its blast radius is a regression surface and does not separate the edit
   surface, which §10 item 7 requires be stated "concretely, in files and call
   sites" (§3.5).
2. Its rejection of `ptr_to_buffers_` rules on entry presence and not on entry
   value, leaving a candidate discriminator unpriced (§3.4).

**Shared, in both:** neither counts the three `u64` sniff sites, and neither
says whether "eleven releases" means the unguarded window or the whole live
window.

### 8.4 Is consensus reached?

**On facts, yes.** Every enumerated factual claim in both reports is true, and
the two accounts are compatible in the way the brief anticipated. There is no
surviving factual dispute in this territory that I can find.

**On the grading, no, and it will not close by itself.** 05a and 05b applied two
different definitions of "architectural". Under the definition that now governs,
05b's label is right and 05a's is wrong, and 05a would have to re-grade rather
than re-investigate. That is a one-paragraph change to 05a §7.7 item F and its
consequence paragraph, plus the withdrawal of the four-site remedy.

**On the blast radius, no.** Neither report's number survives, and the plan now
requires the number to be stated separately from the kind. Both would need §3.5's
derivation or their own.

What remains, exactly:

1. 05a must restore ARCHITECTURAL on the `at_buffer` collision and withdraw the
   four-site remedy (§3.1, §1).
2. Both must restate the blast radius as an edit surface, separated from the
   regression surface (§3.5).
3. 05a should extend the `u64_type()` finding to `spirv_types.cpp:403` and `:415`
   (§4.2).
4. Both should say which release window "eleven" means (§6.3).
5. The `ptr_to_buffers_` route needs pricing or an explicit refusal to price it
   (§3.4, Escalation 1).

None of these requires new investigation. All five are corrections to text
against facts now established.

---

## 9. Escalations

**E1. A third discriminator neither report priced.** `ptr_to_buffers_` is keyed
on `const Stmt *`, is explicitly propagated at `spirv_codegen.cpp:319`, `:326`
and `:332`, and carries a `BufferInfo` whose `type` already separates
`BufferType::Root` from `BufferType::ExtArr` and `BufferType::Args`. Arithmetic
cannot erase it, which is the exact property the `Value` flag lacks. 05b rejects
it on the ground that an ndarray under the capability has an entry — true, and it
does not reach the entry's value. **If this route works, the blast radius is the
three sniff sites and nothing else, and the grading of KIND is unchanged but the
cost falls by an order of magnitude.** I have not designed it; PROJECT-PLAN §10
items 1 and 4 bar me from doing so. The planner should decide whether it is worth
an assignment, because the two reports' cost figures are both conditional on a
remedy neither established as the only one.

**E2. "Blast radius" needs the same treatment "architectural" just received.**
§10 item 7 says "state the BLAST RADIUS separately and concretely, in files and
call sites". It does not say whether that means sites to edit or sites whose
behaviour changes. 05b's 147 and my 11 are both honest answers to different
readings of the same instruction, and they differ by a factor of thirteen. This
is the same failure mode that produced the definition in item 7, one level down.
Adversary 1's Escalation 13, passed on by 05b, asked for the arbiter's word on
"architectural"; it now has one, and the successor question is open.

**E3. Scope of the `u64_type()` defect.** The reachable routes are
`ti_set_runtime_capabilities_ext` in the C API and `translate_devcaps` reached
from `python/taichi/aot/module.py`. PROJECT-PLAN §1.2 puts the Python front end
out of scope. The C API route is C++ and is not exempt, so I have treated the
defect as in scope. If the planner reads §1.2 as covering the AOT surface as a
whole, the finding narrows to the C API alone and should be re-weighted. I have
not made that call.

**E4. `use_64bit_pointers` has been `false` since before v1.1.3.** `git show
v1.1.3:...spirv_codegen.cpp:74` gives `const bool use_64bit_pointers = false;`,
identical to `spirv_codegen.cpp:82` today, and `make_pointer`'s u64 arm carries
the comment "This is hacky, should check out how to encode uint64 values in
spirv". So the 64-bit SNode addressing arm has **never** been exercised in any
tagged release, on any platform, including the eleven where the capability was
live. Both reports treat the collision as a thing that would surface on a flag
flip. It is more than that: the u64 arm of `make_pointer` is code that has never
run. Whether that changes how the planner weighs "flipping the flag does not
produce a working build" is not mine to decide, but it is not recorded in either
report and it bears on how much of the shelved work is *shelved* versus *never
written*.

---

## 10. Peer divergence — `adversary2-05-1.md`

`adversary2-05-1.md` did not exist when I began; it landed while I was writing
and I have read it in full (717 lines). We worked independently and converged on
every ruling. This section records the four places we differ and which of us the
source supports, plus the places where the peer is better and I adopt it.

### 10.1 Where we agree, stated so the divergences are visible against it

Both of us, independently:

- Rule **05b right on the mechanism, 05a wrong on the consequence**, and find the
  two accounts partially compatible in exactly the way the brief anticipated.
- Rule the collision **ARCHITECTURAL** under plan §10 item 7, and find that
  **both** reports reached their label by a route the new definition forbids —
  05a withholding it because the fix looked small, 05b awarding it because the
  fix looked large. The peer states this symmetry more sharply than I did and it
  is correct: my §1 charged only 05a with the conflation. 05b does add the
  correct ground ("the collision does not expire"), but its narrative does read
  as if size decided it. **I accept the peer's correction against my §1.**
- Reproduce **148 `make_value` occurrences, 1 definition, 147 call sites**, and
  both reject 147 as an edit count.
- Independently find the **second entrance to the undeclared-`u64` defect**
  through `spirv_types.cpp:415` and `:403`, which no prior document in this
  territory names.
- Independently find the **`ptr_to_buffers_` route** and reach the same charge:
  05b's "no discriminator by any route" tests entry presence, not entry value,
  and rows `:377` (`Root`) against `:798` (`ExtArr`) are value-distinct.
- Reproduce **121 tags, 24 carrying the capability, 11 / 3 / 10**.
- Reach the same qualified severity on the `u64` defect: live and silent, but
  requiring a hand-written capability list, because
  `vulkan_device_creator.cpp:821` and `:630-632` lock the two capabilities
  together on the detection path.

Two adversaries reaching the same seven results from independent enumerations is
the strongest signal available in this territory, and it points the same way in
every case.

### 10.2 DIVERGENCE 1 — production sites: the peer says five, I say seven. Source supports seven.

The peer's §3.2: "the four `make_pointer` call sites... `:355`, `:371`, `:408`,
`:506` — plus the ndarray base at `:789`. Five. That is 05a's number and it is
correctly counted."

We are counting different sets, and both sets are real:

- The peer counts sites where a tag would **originate**.
- I count (my §3.2) the seven `register_value` sites whose value can be an
  `at_buffer` **input** and must therefore carry a correct tag at `:2197`.

The peer's number is correct for its own question. **For the question the remedy
turns on it is short by at least one, and the miss is not incidental.**

`visit(GlobalTemporaryStmt *)`, `spirv_codegen.cpp:707-712`:

```cpp
  void visit(GlobalTemporaryStmt *stmt) override {
    spirv::Value val = ir_->int_immediate_number(ir_->i32_type(), stmt->offset,
                                                 false);  // Named Constant
    ir_->register_value(stmt->raw_name(), val);           // :710
    ptr_to_buffers_[stmt] = BufferType::GlobalTmps;       // :711
  }
```

There is no `make_pointer` here and no arithmetic. `int_immediate_number` reaches
`get_const` and `new_value(dtype, ValueKind::kConstant)` at
`spirv_ir_builder.cpp:1502`, so the value is `kConstant`. The `ptr_to_buffers_`
entry at `:711` makes it a direct `at_buffer` input through the fall-through read
at `:2212`. A remedy that tags only the peer's five origination sites leaves
every global temporary arriving at `:2197` tagged `kConstant`, which is not a
valid answer to the predicate it would replace.

The peer's own §3.2 table does not cover this either: its five rows are all
arithmetic instructions, and `GlobalTemporaryStmt` performs none.

`MatrixPtrStmt` at `:334` is the seventh. The peer covers it as rows 1 and 2 of
its arithmetic table but not as a site needing a tag.

**Source supports my seven** on the "which sites must end up carrying a correct
tag" reading, and the peer's five on the "where would a tag originate" reading.
Both should be recorded; only the first sizes the remedy.

### 10.3 DIVERGENCE 2 — files touched: the peer says two, I say three, and the peer's own table says three

The peer's §3.3 summary: "two files, four functions edited, five production
sites". Its files are `spirv_ir_builder.h` and `spirv_codegen.cpp`.

But the last row of the same table reads: "Helper wrappers needing an
operand-propagation rule — 3 named plus 2 macros — `cast`, and the macros
`DEFINE_BUILDER_BINARY_USIGN_OP` at `spirv_ir_builder.cpp:1038` and
`DEFINE_BUILDER_BINARY_SIGN_OP` at `:1049`".

`spirv_ir_builder.cpp` is a third file, named in the peer's own row and absent
from its own summary. This is the §10 item 8 failure — a total that does not
reconcile against the list beneath it — and it is the fifth instance the project
has produced.

The substantive question underneath: **two files if `make_value` can propagate
generically from its operand flags, three if each op needs a stated rule.** The
peer's row 5 asserts the second and its summary counts the first. I say three
(my §3.5) and I hold it, on the peer's own evidence: the four instantiations at
`spirv_ir_builder.cpp:1062-1065` pass operands whose flags differ, and a generic
rule would have to decide what `kPhysicalPtr + kConstant` yields. That decision
has to be written somewhere.

Neither of us has established which. It belongs with Escalation E1 / the peer's
escalation 1, because it moves with the same unresolved question.

### 10.4 DIVERGENCE 3 — the peer's completeness charge against 05b is wrong

The peer's §10: "**05b.** Section 5.0's account of the dense path names the
assertion at `demote_dense_struct_fors.cpp:29` and not the unchecked `int`
accumulator at `:20` and `:24` in the same function."

**05b does name it.** `report-05b-shelved-64bit.md:748-752`:

> "One further unnamed truncation, VERIFIED, in the dense pass itself:
> `demote_dense_struct_fors.cpp:19` declares
> `std::array<int, taichi_max_num_indices> total_shape` and `:23-25` multiplies
> into it in a loop, with no check, while `total_n` beside it at `:18` is `int64`
> and **is** checked at `:29`."

I cite the same finding at my §5.5, independently, and reached it before reading
either document's treatment of it.

The peer's line numbers are also off. From source:

```
18:  int64 total_n = 1;
19:  std::array<int, taichi_max_num_indices> total_shape;
20:  total_shape.fill(1);
...
24:      total_shape[j] *= snode->extractors[j].shape;
```

`:19` is the declaration, `:20` is `total_shape.fill(1)`, and the accumulation is
`:24` inside the loop at `:23-25`. 05b's `:19` and `:23-25` are right; the peer's
`:20` is the fill, not the accumulator.

**Source supports 05b and me against the peer.** This charge should be struck
from the completeness verdict; 05b is complete on this point.

### 10.5 DIVERGENCE 4 — two citation corrections, one each way

- `translate_ti_type`. The peer gives it as "opening at
  `taichi/codegen/spirv/spirv_types.cpp:483`". `:483` is the closing brace of the
  preceding function; the signature begins at `:484`. **My §4.3 is right here.**
- `visit_int_type`. I first gave it as `spirv_types.cpp:393-418`; the closing
  brace is `:419`, so the peer's `:393-419` is right and mine was one short.
  Likewise the peer's `:414-415` for the unsigned 64-bit arm is tighter than my
  `:414-416`, which swept in the closing brace. **The peer is right here. Both
  are corrected at §4.3 and §11 above.**

Neither affects any conclusion. Both are recorded because §10 item 8 requires
citations to reconcile, and because I made an equivalent error earlier in this
document on the `ValueKind` enum, corrected at §2.2.

### 10.6 Where the peer is better, and I adopt it

Three places.

1. **The tag census.** The peer's §7 partitions all 121 tags into five classes —
   file absent 92, file present but name absent 5, unguarded 11, macOS-excepted
   3, dead 10 — summing to 121. Mine enumerated only the 24 that carry the
   capability and cross-checked the 121 by family. The peer's is the stronger
   form: it accounts for every tag rather than the interesting subset. It also
   diagnoses **why** adversary 1 missed v0.9.x more precisely than I did: the
   setter is spelled `ti_device_->set_cap(...)` at v0.9.x and `caps.set(...)`
   later, so a setter-matched sweep loses them where a name-matched sweep does
   not. My §7.2 attributes it to the path move alone, which the peer correctly
   notes would have hidden v1.0.0 to v1.0.3 as well. **I withdraw that
   attribution and adopt the peer's.**

2. **Re-grading 05a's whole verdict table.** The peer's §2.1 re-grades all nine
   rows of 05a §7.0 under plan §10 item 7 and changes two: F to ARCHITECTURAL,
   and C ("AOT devcap negotiation not ready") from ENVIRONMENTAL to
   ARCHITECTURAL. I graded only F. **I agree with the C regrade** and reached the
   same test independently in my §1: unwritten Taichi code is inside this
   codebase, an ideal driver and specification do not write it, and it expired
   because someone changed the tree — which §10 item 7 gives as how architectural
   obstacles go away, not environmental ones. Row B I did not examine; the peer
   examined it and left it standing, and I have no basis to disturb that.

3. **The `ptr_to_buffers_` qualifications.** We found the same route. The peer
   goes further and names the concrete unanswered questions I did not: row `:800`
   puts a non-array `ExternalPtrStmt` on `BufferType::Args` rather than `ExtArr`,
   and rows `:319`, `:326` and `:332` read `ptr_to_buffers_[stmt->origin]` with
   `operator[]`, which default-constructs a `BufferInfo` when the origin has no
   entry. Both are real hazards for any remedy built on the map, and both must be
   answered before anyone calls it cheap. **I adopt them into E1.** My §3.4
   understated the difficulty by omitting them.

### 10.7 Net

We diverge on two numbers and one completeness charge, and agree on every
ruling. On the numbers, source supports my seven production sites and my three
files; on the completeness charge, source supports 05b against the peer. On the
census, the re-grading breadth and the `ptr_to_buffers_` qualifications, the peer
is better and I have adopted it above.

Nothing in the peer's file changes any verdict in mine, and nothing in mine
should change any verdict in the peer's. The one question both of us leave open
is the same one: whether `ptr_to_buffers_` can carry the distinction, which is
worth two orders of magnitude and is closed by nobody.

---

## 11. Files and lines cited in this report

All paths relative to `/opt/project/taichi`.

`taichi/codegen/spirv/spirv_codegen.cpp` — 82, 319, 326, 332, 334, 355, 356,
371, 372, 373, 376, 377, 408, 504-510, 506, 708-711, 734, 771, 783-795, 787,
789, 790-791, 792, 794, 797-801, 2194-2220, 2197, 2203, 2204, 2212, 2214-2216,
2222-2242, 2227, 2244-2264, 2249, 2317-2324, 2320, 2339-2340, 2415-2416,
2490-2491, 2635, 2710.

`taichi/codegen/spirv/spirv_ir_builder.h` — 51, 59, 69-79, 88, 290-298, 291,
343, 520-526, 529-534, 617, 621.

`taichi/codegen/spirv/spirv_ir_builder.cpp` — 64-66, 73-77, 113-127, 166-169,
310-328, 334-352, 376, 407-424, 1038-1047, 1062-1065, 1270-1285, 1274, 1275,
1286-1298, 1299-1309, 1311-1317, 1502.

`taichi/codegen/spirv/spirv_types.cpp` — 393-419, 402-403, 403, 414-415, 484-513, 491-493.

`taichi/ir/snode.h` — 37-54, 41, 45, 49, 97, 114, 306-308, 316.
`taichi/ir/snode.cpp` — 20, 22-23, 83, 89-101, 92, 95, 174-177.

`taichi/transforms/demote_dense_struct_fors.cpp` — 12, 18, 19, 23-25, 29, 74, 76,
114-119.
`taichi/transforms/offload.cpp` — 192.
`taichi/transforms/compile_to_offloads.cpp` — 191-192.

`taichi/codegen/spirv/snode_struct_compiler.cpp` — 117-122, 121.
`taichi/codegen/llvm/struct_llvm.cpp` — 174, 176.
`taichi/analysis/offline_cache_util.cpp` — 112, 114.

`taichi/program/compile_config.cpp` — 18, 73.
`taichi/program/program.cpp` — 514-538, 540-556.

`taichi/rhi/vulkan/vulkan_device_creator.cpp` — 630-633, 819-829, 821, 825, 826.
`taichi/rhi/vulkan/vulkan_device_creator.h` — 62.

`c_api/src/taichi_core_impl.cpp` — 317-334.
`c_api/src/taichi_vulkan_impl.cpp` — 45-51.
`python/taichi/aot/module.py` — 86-110.

Commits: `7705f688a` (2022-02-08, #4221), `d20f55dc8` (2022-10-30, #6468).
Tags read: all 121; 24 carry the capability.
