# Native precision prerequisites

Status: **accepted in #178, 2026-10-24**, first slice of the
[#176 accepted plan](../../docs/native-precision-truncation-plan.md) under #172.
Baseline: `925d8c7ac8175ca6afe399c631fd35e710d4cfcc`, after #174/#175/#176.

This slice adds checked double packed views and a native experimental
significand-clearing helper. Calculation format, stored container and retained
fraction bits are independent. Existing field/tick/update/geometry behavior is
unchanged. No live numerical selector or LoD cascade is added; production
rounding and the #172/#177 integration remain separate work.

## Native interface and numerical boundary

[Precision](../src/engine/precision.h) specifies calculation and storage formats
(f32 or f64) and retained fraction bits b (0..23 or 0..52 respectively). The
normal implicit leading bit is not included in b. Unsupported enum/counts fail
at construction. There is no default calibrated profile.

`require_backend(runtime)` rejects requested-CUDA/actual-CPU fallback and absent
advertised data64 support when either format is f64. This is an advertisement
check, not hardware qualification. Call it during kernel preparation. The actual
CPU probes exercise both widths and both conversion directions. Local device
capability, accuracy, cost and allocation/transfer qualification remain #145.

Callers use explicitly typed native loads/constants/casts **before** their
selected arithmetic. `truncate_position(builder, calculated, precision)` cannot
recover detail lost earlier. It types the result boundary, validates the input,
converts to the selected stored container, validates the conversion, clears the
lowest m-b encoded fraction bits with native bitcast/bitwise IR and returns
`{value, valid}`. Compile its caller with `MathMode::Strict`.

Sign and exponent are preserved by the bit-clearing step. b=m is identity;
b=0 clears the entire encoded fraction. The resulting magnitude moves toward
zero; signed zero is preserved for supported values. Invalid input/conversion
returns valid=0 and a positive-zero placeholder. The caller must check validity
before publishing a candidate; that placeholder is not an accepted coordinate.
The helper returns no rounding remainder and writes no physical state.

Clearing two terminal bits, for example, still uses the full four/eight-byte
container. Within a quantization interval, small arithmetic variation can leave
stored position unchanged; changes across an interval boundary can still produce
motion. Truncation introduces origin bias. The tests retain those effects rather
than claiming universal jitter elimination or inventing a stability controller.
This precisely defined Q_b is an experimental candidate, not the selected live
rounding policy.

## Discovered native flush-to-zero boundary

The fork's [Program constructor](../taichi/taichi/program/program.cpp) enables
flush-to-zero (FTZ) on CPU; [CUDA lowering](../taichi/taichi/runtime/cuda/jit_cuda.cpp)
also specifies FTZ. Strict math disables fast-math transformations but does not
undo that mode. The initial known-value probe found f64 `2^-149` narrowing to
f32 zero instead of the representable smallest f32 subnormal. A host cast after
Program construction can flush too, so a host cast alone is an insufficient
conversion oracle.

This implementation leaves the global mode unchanged. For **f64 calculation →
f32 storage**, every nonzero input magnitude below f32's smallest normal
(`2^-126`) is explicitly unsupported: valid=0, value=0. This conservative range
also rejects values immediately below the normal boundary that might round up,
and values that an IEEE conversion would round to signed zero. Normal-range
conversion is checked against known ties-to-even encodings; overflowing/nonfinite
conversion remains invalid. Signed zero itself is supported.

Same-container bit clearing preserves encoded subnormals, and f32 subnormal
promotion to f64 is separately tested. This does **not** qualify arbitrary
subnormal arithmetic: operations before the helper still execute in the native
mode. Full gradual-underflow arithmetic/narrowing would require separately
reviewed native configuration or conversion work. No software floating-point
converter, global FTZ change or silently accepted flushed narrowing is hidden here.

## Typed transport, lifetime and real resources

[PackedView](../src/engine/packed_view.h) now accepts float/f32, double/f64 and
int/i32, retaining checked shape/product/byte bounds, generation, alignment and
borrowed lifetime. Its vector owner and data address must remain valid through
launch/transfer and synchronization. This is synchronous borrowing, not an
asynchronous lease. It does not make ordinary vectors pinned.

The existing `Param` dtype, `argument_float(double)` and native prepared scalar
cache already support f64. Existing ndarray transfers use actual element width.
The implementation reuses these facilities; it changes neither Program's default
floating type nor integer/flag/map widths. Adding double to PackedView alone
does not convert the live f32 Harness or recover source precision already lost.

The test-owned buffers report actual host/backend payload and binding bytes.
BulkCopy owns separate native ndarrays plus host buffers; CpuDirect borrows the
fixed host buffers and creates no probe payload ndarrays. For N boundary inputs
and B=m+1 retained-bit choices:

```text
input: N * sizeof(calculation scalar)
output: B*N * sizeof(stored scalar)
status plus idempotence: 2*B*N * sizeof(i32)
```

The arithmetic probe has N=4, input 4N calculation scalars, output 5N stored
scalars, status 4N i32 and an independent N-element f32 diagnostic. Its payload
is 224/368/304/288 bytes for 32→32 / 64→64 / 32→64 / 64→32 respectively.
Binding payload is inventoried separately in retained output. Fixed C++ object,
vector-capacity metadata, compiler/runtime storage and pool alignment are outside
these payload totals. No unused mirror or speculative live scratch bank is added.

## CPU qualification and reproduction

[Retained evidence](../tests/precision) records source/backend/compiler pins,
actual architecture, raw output, payloads, binding bytes and measured counters.
Two new modes cover BulkCopy and CpuDirect; each tests all four calculation/store
pairs and every legal retained-bit count. An independent long-double arithmetic
oracle checks the bit-clearing result; known conversion encodings separately
check normal ties and FTZ-range rejection.

All **54 CPU suites passed in 356.47 seconds**, including 99,792 boundary result
cells in the two new modes. This is the matched CPU result, not device execution.

Boundary cases include both signs, signed zero, exponent neighbors, minimum and
maximum normal/subnormal values, finite overflow, NaN/Inf, deterministic varied
bit patterns, idempotence and sign/exponent/error bounds. Arithmetic probes use
runtime inputs, including `(2^25+1)-2^25`, to expose lost f32 detail and late
widening. Prepared f64 scalar updates retain increments below an f32 ULP.
One kernel also computes an independent f32 diagnostic, demonstrating distinct
calculation purposes without claiming a speedup.

Combined-contribution tests distinguish final quantization from intermediate
filtering. Repeated publication probes discard subthreshold detail without
altering an independent velocity value. Boundary traces on both signs explicitly
exhibit crossing/oscillation. These small kernels do not prove live motion,
centroid accounting or a production activation rule.

Preparation compiles, binds and dispatches empty kernels. Measured first nonempty
and recurring dispatch/readback boundaries observe zero linked C++ new/new[]
calls. Native pool/requested/cursor, tree/SNode and compiler counters remain
unchanged through first use/reuse. The probes create no SNode trees; known-node
list counters are therefore empty, not a demonstration of new sparse machinery.
The observations exclude arbitrary direct malloc, physical page residency/faults
and driver internals.

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^precision_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

Use the [matched native build](PREPARED-EXECUTION.md#reproduction). A CPU-only
build refuses `precision_test --cuda`; this is negative selection evidence,
not GPU execution. No database migration/write or Python runtime is involved.

The [state/conversion audit](PRECISION-STATE-AUDIT.md) identifies the exact live
state that must be reconciled before integration. #177 retains local finest-LoD
centroids and upward compounding, provisional downward whole-object response,
and unresolved matching/phase ordering. This primitive neither supplies that
cascade nor selects a flat all-scale algorithm. #156/#170 retain operational
admission, candidate tick publication and their integrated CPU exit.
