#!/usr/bin/env python3
"""Isolated SNode scaling probe; run with the engine's Python environment.

Each pattern owns a position, force and centroid layout. This deliberately
measures distinct layouts, not a production gravity solver or an accuracy claim
for centroid approximations. JSON reports distinguish compilation, execution,
readback and structure construction. Every invocation runs in a child process
so an assertion/segfault at a baseline limit does not kill a comparison driver.
"""

import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import sys
import time


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--arch", choices=("cpu", "cuda"), default="cpu")
    result.add_argument("--dtype", choices=("f32", "f64"), default="f64")
    result.add_argument("--layout", choices=("one-tree", "many-trees", "churn"), default="one-tree")
    result.add_argument("--storage", choices=("dense", "pointer"), default="dense")
    result.add_argument("--patterns", type=int, default=8)
    result.add_argument("--particles", type=int, default=16)
    result.add_argument("--samples", type=int, default=4, help="Patterns checked and timed (0 checks all)")
    result.add_argument("--ticks", type=int, default=3)
    result.add_argument("--timeout", type=int, default=180)
    result.add_argument("--memory-profile", action="store_true", help="Collect runtime allocation counters (extra synchronization)")
    result.add_argument("--device-memory-gb", type=float, default=0.5)
    result.add_argument("--snode-capacity", type=int)
    result.add_argument("--tree-capacity", type=int)
    result.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    return result


def run_worker(args):
    import numpy as np
    import taichi as ti
    from taichi.lang.misc import is_arch_supported

    arch = getattr(ti, args.arch)
    if not is_arch_supported(arch):
        raise RuntimeError(f"Requested backend {args.arch} is unavailable; refusing fallback")
    options = dict(arch=arch, default_fp=getattr(ti, args.dtype), offline_cache=False,
                   fast_math=False, device_memory_GB=args.device_memory_gb)
    if args.snode_capacity is not None:
        options["llvm_snode_capacity"] = args.snode_capacity
    if args.tree_capacity is not None:
        options["llvm_snode_tree_capacity"] = args.tree_capacity
    started = time.perf_counter()
    ti.init(**options)
    initialization_s = time.perf_counter() - started
    dtype = getattr(ti, args.dtype)

    @ti.kernel
    def initialize(position: ti.template(), pattern: ti.i32):
        for i in range(args.particles):
            position[i] = ti.Vector([ti.cast(i + 1, dtype) / 16,
                                     ti.cast(pattern + 1, dtype) / 32, 0.25])

    @ti.kernel
    def interactions(position: ti.template(), force: ti.template(), centroid: ti.template()):
        # Four group sources; one force evaluation per member/source pair.
        # Fixed source data permits an independent NumPy oracle.
        for i in position:
            acceleration = ti.Vector.zero(dtype, 3)
            for source in range(4):
                displacement = ti.Vector([ti.cast(source + 1, dtype), 0.5, -0.25]) - position[i]
                distance2 = displacement.dot(displacement) + 0.125
                acceleration += ti.cast(source + 1, dtype) * displacement / (distance2 * ti.sqrt(distance2))
            force[i] = acceleration
        for axis in ti.static(range(3)):
            centroid[None][axis] = 0
        # Equal masses: centroid rollup through parallel atomic accumulation.
        for i in position:
            for axis in ti.static(range(3)):
                ti.atomic_add(centroid[None][axis], position[i][axis] / args.particles)

    count = args.patterns if args.samples == 0 else min(args.samples, args.patterns)
    sampled = set(np.linspace(0, args.patterns - 1, count, dtype=int).tolist())
    times = dict(construction_s=0.0, first_launch_s=0.0, execution_s=0.0, readback_s=0.0, destruction_s=0.0)
    max_force_error = 0.0
    max_centroid_error = 0.0
    node_ids = set()
    created_trees = 0

    def create_pattern(builder):
        position = ti.Vector.field(3, dtype)
        force = ti.Vector.field(3, dtype)
        centroid = ti.Vector.field(3, dtype)
        if args.storage == "dense":
            builder.dense(ti.i, args.particles).place(position, force)
        else:
            builder.pointer(ti.i, args.particles).place(position, force)
        builder.place(centroid)
        return position, force, centroid

    def collect_nodes(node):
        node_ids.add(node._id)
        for child in node._get_children():
            collect_nodes(child)

    def check_pattern(pattern, fields):
        nonlocal max_force_error, max_centroid_error
        position, force, centroid = fields
        started = time.perf_counter()
        initialize(position, pattern)
        interactions(position, force, centroid)
        ti.sync()
        times["first_launch_s"] += time.perf_counter() - started
        started = time.perf_counter()
        for _ in range(args.ticks):
            interactions(position, force, centroid)
        ti.sync()
        times["execution_s"] += time.perf_counter() - started
        started = time.perf_counter()
        actual_force = force.to_numpy()
        actual_centroid = np.array(centroid[None])
        times["readback_s"] += time.perf_counter() - started
        expected_position = np.column_stack((np.arange(1, args.particles + 1) / 16,
                                             np.full(args.particles, (pattern + 1) / 32),
                                             np.full(args.particles, 0.25)))
        sources = np.column_stack((np.arange(1, 5), np.full(4, 0.5), np.full(4, -0.25)))
        displacement = sources[None, :, :] - expected_position[:, None, :]
        distance2 = np.sum(displacement**2, axis=2) + 0.125
        expected_force = np.sum(np.arange(1, 5)[None, :, None] * displacement / distance2[:, :, None]**1.5, axis=1)
        expected_centroid = expected_position.mean(axis=0)
        max_force_error = max(max_force_error, float(np.max(np.abs(actual_force - expected_force))))
        max_centroid_error = max(max_centroid_error, float(np.max(np.abs(actual_centroid - expected_centroid))))
        tolerance = 5e-5 if args.dtype == "f32" else 1e-11
        np.testing.assert_allclose(actual_force, expected_force, rtol=tolerance, atol=tolerance)
        np.testing.assert_allclose(actual_centroid, expected_centroid, rtol=tolerance, atol=tolerance)

    retained = []
    shared = ti.FieldsBuilder() if args.layout == "one-tree" else None
    for pattern in range(args.patterns):
        started = time.perf_counter()
        builder = shared if shared is not None else ti.FieldsBuilder()
        fields = create_pattern(builder)
        if shared is not None:
            retained.append((pattern, fields))
        else:
            tree = builder.finalize()
            collect_nodes(builder.root)
            created_trees += 1
            times["construction_s"] += time.perf_counter() - started
            if args.layout == "churn":
                if pattern in sampled:
                    check_pattern(pattern, fields)
                started = time.perf_counter()
                tree.destroy()
                times["destruction_s"] += time.perf_counter() - started
                continue
            retained.append((pattern, fields, tree))
            continue
        times["construction_s"] += time.perf_counter() - started

    if shared is not None:
        started = time.perf_counter()
        shared_tree = shared.finalize()
        collect_nodes(shared.root)
        created_trees += 1
        times["construction_s"] += time.perf_counter() - started
    if args.memory_profile and args.layout != "churn":
        ti.profiler.print_memory_profiler_info()
    for entry in retained:
        if entry[0] in sampled:
            check_pattern(entry[0], entry[1])
    if args.memory_profile and args.layout != "churn":
        ti.profiler.print_memory_profiler_info()
    started = time.perf_counter()
    if shared is not None:
        shared_tree.destroy()
    else:
        for entry in retained:
            entry[2].destroy()
    ti.sync()
    times["destruction_s"] += time.perf_counter() - started
    result = dict(status="passed", arch=args.arch, dtype=args.dtype, layout=args.layout,
                  storage=args.storage, patterns=args.patterns, particles=args.particles,
                  checked_patterns=sorted(sampled), ticks=args.ticks, created_trees=created_trees,
                  observed_snode_ids=len(node_ids), max_snode_id=max(node_ids),
                  initialization_s=initialization_s, **times,
                  max_force_absolute_error=max_force_error, max_centroid_absolute_error=max_centroid_error,
                  taichi_version=str(ti.__version__), taichi_module=ti.__file__, python=sys.executable,
                  requested_snode_capacity=args.snode_capacity, requested_tree_capacity=args.tree_capacity,
                  requested_device_memory_gb=args.device_memory_gb, memory_profile=args.memory_profile,
                  cuda_visible_devices=os.environ.get("CUDA_VISIBLE_DEVICES"))
    try:
        import resource
        if sys.platform.startswith("linux"):
            result["peak_host_rss_kib"] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    except ImportError:
        pass
    print("SNODE_RESULT=" + json.dumps(result, sort_keys=True), flush=True)


def main():
    args = parser().parse_args()
    if min(args.patterns, args.particles, args.ticks, args.timeout) < 1 or args.samples < 0 or args.device_memory_gb <= 0:
        parser().error("Counts must be positive; samples may also be zero")
    if args.worker:
        run_worker(args)
        return 0
    command = [sys.executable, str(Path(__file__).resolve()), *sys.argv[1:], "--worker"]
    started = time.perf_counter()
    try:
        completed = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
    except subprocess.TimeoutExpired:
        print(json.dumps(dict(status="timeout", timeout_s=args.timeout)))
        return 1
    results = [line[len("SNODE_RESULT="):] for line in completed.stdout.splitlines() if line.startswith("SNODE_RESULT=")]
    if completed.returncode == 0 and results:
        result = json.loads(results[-1])
        result["wall_s"] = time.perf_counter() - started
        totals = re.findall(r"Total requested dynamic memory .*?: ([\d,]+) B", completed.stdout)
        if len(totals) == 2:
            result["dynamic_requested_after_construction_bytes"] = int(totals[0].replace(",", ""))
            result["dynamic_requested_after_execution_bytes"] = int(totals[1].replace(",", ""))
    else:
        result = dict(status="failed", returncode=completed.returncode,
                      stdout_tail=completed.stdout[-4000:], stderr_tail=completed.stderr[-4000:])
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
