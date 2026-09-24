# Distinct SNode pattern probe

`snode_patterns.py` checks structural scaling and a small analytical workload.
Each pattern contains position and acceleration vectors plus a centroid vector.
A kernel computes softened acceleration from four fixed group sources and rolls
positions into an equal-mass centroid using atomic accumulation. NumPy evaluates
the numerical reference independently. This is a synthetic probe, not a model
of the user's interaction network or a validation of centroid approximation.

Run with a Python environment containing the engine you want to measure:

```bash
python benchmarks/snode_patterns.py --arch cpu
CUDA_VISIBLE_DEVICES=0 python benchmarks/snode_patterns.py --arch cuda
python benchmarks/snode_patterns.py --storage pointer --samples 0
python benchmarks/snode_patterns.py --layout many-trees --patterns 16
python benchmarks/snode_patterns.py --layout churn --patterns 16
```

The defaults are deliberately small: eight patterns, sixteen particles, four
checked patterns, and three warm ticks. `--samples 0` checks every pattern;
otherwise evenly spaced patterns include the first and last when there are at
least two samples. Unsampled patterns test construction and destruction only.
Each pattern introduces thirteen SNodes (six position/acceleration scalar leaves,
their container, and three centroid scalar leaves with three scalar containers),
plus the tree root. Eight patterns therefore produce 105 nodes in one tree. These are
schema nodes, not particle instances. Reported IDs are collected from the actual
tree rather than inferred from that formula.

`one-tree` puts all patterns in one tree; `many-trees` keeps independent trees
live together; `churn` creates and destroys one at a time to expose cumulative
identifier or allocation-lifecycle issues. All layouts destroy their trees after
checking output. Each invocation uses an isolated subprocess and returns one JSON
object even if the worker crashes. A failure is also a nonzero exit status.
The default timeout is 180 seconds; use `--timeout` to allow longer explicit runs.

For a baseline comparison, run the same command with the baseline and modified
engine environments, preserving the same hardware, dtype, pattern count, sample
count, and tick count. `PYTHONPATH=/path/to/checkout/python` can select an in-tree
build. The JSON includes the imported module path to verify that selection.
Offline caching and fast math are disabled. CUDA unavailability is a failure,
not a CPU fallback. Separate timings cover initialization, structure creation,
first launches (including compilation), warm synchronized execution, host output
readback, and destruction. Readback includes first-use compilation of generated
field-copy/accessor kernels; it is not a pure transfer-bandwidth measurement. Warm execution includes Python dispatch overhead and
is the total for all sampled patterns and ticks, not per-kernel device time.
Host RSS is a process high-water mark on Linux; it is not GPU memory usage.
`--memory-profile` records the runtime's cumulative requested dynamic bytes after
construction and after execution. This is an allocation-request counter, not
resident memory or current live allocations. Collection adds synchronization.
`--device-memory-gb` sets the CUDA runtime pool budget (default 0.5 GiB); preserve
it across comparisons. It is not a limit on all process or GPU allocations.

The modified runtime accepts `--snode-capacity` and `--tree-capacity`; omit these
for the baseline. For example, `--patterns 110 --snode-capacity 2048` crosses the
original 1024 metadata-slot boundary with one tree while storing only 1760
particles at the default particle count. Large baseline runs can consume much
more metadata memory than the field payload suggests; increase counts gradually.
Capacity options reserve metadata slots, not particle storage. There is no
automatic large sweep.
