# Prepared backing / binding evidence

This retains the CPU proof for the backing and native-binding section accepted
in [#165](https://github.com/Human-Cognome-Project/human-cognome-project/pull/165),
under #162. [The implementation guide](../../docs/PREPARED-EXECUTION.md) states
the scope, ownership rules, reproduction commands and remaining work.

[`acceptance.json`](acceptance.json) pins the measured implementation/native
trees, source hashes, environment, test sequence and parsed profiles. The raw
[`evidence`](evidence) files retain both host paths for the 70/4,096 capacities
and the aggregate-letter, byte-only and mixed membership fixtures. Supplied
starts belong only to the test adapter. Each profile ends preparation empty,
then performs its first nonempty use and 32 ticks through existing kernels.

Raw profile rows use these columns:

| Row | Columns after row name |
|---|---|
| `profile=...` | Named logical limits, actual presence/relationships/focus/references, binding/scratch bytes, awake counts and body counts. |
| `ready_empty` / `after_32_ticks` | Native requested bytes, live-list chunk bytes, allocated native elements, pool calls, pool requested bytes, raw pool calls, raw reservation, aligned host cursor, Program tree count, issued SNode IDs, cumulative compilation seconds. |
| `numerical` | Family, rows, capacity stride, element bytes, retained host copies, backend copies, bytes per copy. Activity host workspace is separately charged in `workspace`. |
| `native_root` | Layout family and compiled root bytes, before allocator/pool padding. |
| `workspace` | Term, physical count, element bytes, retained copies, total bytes. Actual binding payload plus unused binding reserve completes the finite scratch reservation. |

The dense reference runs before each empty-ready baseline. Absolute Program and
pool counters can include that earlier work; the ready-to-final comparison
measures the prepared execution. Known live SNodes supply list/allocator queries;
the test does not query destroyed reference layouts. Pool/raw reservation and
aligned cursor consumption are distinct from payload sizes.

The allocation observer counts linked C++ `new/new[]`, including aligned forms
and worker threads. It does not measure every direct malloc, physical residency,
page fault or CUDA driver allocation. CPU backing stability and zero observed
C++ allocations do not qualify GPU execution or performance.

CUDA launcher/context syntax and `ARCH_cuda` runtime compilation are recorded
separately. Actual GPU execution, driver traces and pinning remain local under
#145. Coherent changes while ticking and production #156 integration remain
unbuilt; this evidence does not close #162 or #156.
