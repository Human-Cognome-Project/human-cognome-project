#!/usr/bin/env python3
"""
HCP Engine benchmark client.

Sends phys_resolve for a test file via the socket API and saves both
the JSON response and a TSV summary. Monitors system resources.

Usage:
    python scripts/run_benchmark.py [--file PATH] [--host HOST] [--port PORT] [--label LABEL]

Defaults to the Sherlock Holmes test file at sources/data/sherlock.txt.
"""

import argparse
import json
import os
import socket
import struct
import sys
import time
from datetime import datetime
from pathlib import Path


def send_message(sock: socket.socket, data: bytes) -> None:
    sock.sendall(struct.pack(">I", len(data)) + data)


def recv_message(sock: socket.socket) -> bytes:
    header = b""
    while len(header) < 4:
        chunk = sock.recv(4 - len(header))
        if not chunk:
            raise ConnectionError("Server closed connection")
        header += chunk
    length = struct.unpack(">I", header)[0]
    parts = []
    remaining = length
    while remaining > 0:
        chunk = sock.recv(min(remaining, 65536))
        if not chunk:
            raise ConnectionError("Server closed connection")
        parts.append(chunk)
        remaining -= len(chunk)
    return b"".join(parts)


def get_system_stats() -> dict:
    """Collect host-side resource stats."""
    stats = {}

    # RSS from /proc/self/status
    try:
        with open("/proc/self/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    stats["client_rss_kb"] = int(line.split()[1])
                    break
    except (FileNotFoundError, ValueError):
        pass

    # GPU stats via nvidia-smi
    try:
        import subprocess
        result = subprocess.run(
            ["nvidia-smi", "--query-gpu=memory.used,memory.total,temperature.gpu,utilization.gpu",
             "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            parts = result.stdout.strip().split(", ")
            if len(parts) >= 4:
                stats["gpu_mem_used_mb"] = int(parts[0])
                stats["gpu_mem_total_mb"] = int(parts[1])
                stats["gpu_temp_c"] = int(parts[2])
                stats["gpu_util_pct"] = int(parts[3])
    except (FileNotFoundError, subprocess.TimeoutExpired, ValueError):
        pass

    return stats


def main():
    parser = argparse.ArgumentParser(description="HCP Engine benchmark client")
    parser.add_argument("--file", default="data/gutenberg/texts/01661_The Adventures of Sherlock Holmes.txt",
                        help="Path to test file (default: Sherlock Holmes)")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9720)
    parser.add_argument("--label", default="",
                        help="Optional label for this benchmark run")
    parser.add_argument("--max-chars", type=int, default=0,
                        help="Max chars to process (0 = unlimited)")
    args = parser.parse_args()

    if not os.path.exists(args.file):
        print(f"Error: file not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    file_size = os.path.getsize(args.file)
    print(f"Benchmark: {args.file} ({file_size:,} bytes)")

    # Pre-run system stats
    pre_stats = get_system_stats()
    if pre_stats.get("gpu_mem_used_mb"):
        print(f"  GPU: {pre_stats['gpu_mem_used_mb']}/{pre_stats['gpu_mem_total_mb']} MB, "
              f"{pre_stats.get('gpu_temp_c', '?')}C, {pre_stats.get('gpu_util_pct', '?')}% util")

    # Build request
    request = {
        "action": "phys_resolve",
        "file": os.path.abspath(args.file),
        "benchmark": True,
    }
    if args.max_chars > 0:
        request["max_chars"] = args.max_chars

    # Connect and send
    print(f"Connecting to {args.host}:{args.port}...")
    t0 = time.monotonic()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(300)  # 5 min timeout for large files
    sock.connect((args.host, args.port))

    send_message(sock, json.dumps(request).encode())
    response_bytes = recv_message(sock)
    sock.close()

    t1 = time.monotonic()
    wall_time_s = t1 - t0

    # Parse response
    response = json.loads(response_bytes)

    # Post-run system stats
    post_stats = get_system_stats()

    # Print summary
    print(f"\n{'='*60}")
    print(f"  Status:          {response.get('status')}")
    print(f"  Phase 1:         {response.get('phase1_settled')}/{response.get('phase1_total')} "
          f"settled ({response.get('phase1_total_bytes', 0):,} bytes)")
    print(f"  Phase 1 time:    {response.get('phase1_time_ms', 0):.1f} ms")
    print(f"  Runs:            {response.get('total_runs')} total")
    print(f"  Resolved:        {response.get('resolved')}/{response.get('total_runs')} "
          f"({100*response.get('resolved',0)/max(response.get('total_runs',1),1):.1f}%)")
    print(f"  Unresolved:      {response.get('unresolved')}")
    print(f"  Resolve time:    {response.get('time_ms', 0):.1f} ms")
    print(f"  Wall time:       {wall_time_s:.1f} s")
    print(f"  Engine RSS:      {response.get('rss_kb', 0):,} KB")
    if post_stats.get("gpu_mem_used_mb"):
        print(f"  GPU mem (post):  {post_stats['gpu_mem_used_mb']}/{post_stats['gpu_mem_total_mb']} MB")
        print(f"  GPU temp (post): {post_stats.get('gpu_temp_c', '?')}C")
    print(f"{'='*60}")

    # Save response JSON
    os.makedirs("benchmarks", exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    label_suffix = f"_{args.label}" if args.label else ""

    json_path = f"benchmarks/resolve_{ts}{label_suffix}.json"
    with open(json_path, "w") as f:
        # Strip per-result detail for summary file (can be large)
        summary = {k: v for k, v in response.items() if k != "results"}
        summary["wall_time_s"] = wall_time_s
        summary["file"] = args.file
        summary["file_bytes"] = file_size
        summary["label"] = args.label
        summary["pre_gpu"] = pre_stats
        summary["post_gpu"] = post_stats

        # Resolution rate by word length
        length_stats = {}
        for r in response.get("results", []):
            wlen = len(r.get("run", ""))
            if wlen not in length_stats:
                length_stats[wlen] = {"resolved": 0, "unresolved": 0}
            if r.get("resolved"):
                length_stats[wlen]["resolved"] += 1
            else:
                length_stats[wlen]["unresolved"] += 1
        summary["by_length"] = {str(k): v for k, v in sorted(length_stats.items())}

        # Morph bit counts
        morph_count = sum(1 for r in response.get("results", [])
                         if r.get("morph_bits", 0) != 0)
        cap_count = sum(1 for r in response.get("results", [])
                       if r.get("first_cap") or r.get("all_caps"))
        summary["morph_flagged"] = morph_count
        summary["cap_flagged"] = cap_count

        json.dump(summary, f, indent=2)

    print(f"\nSaved: {json_path}")

    # Save full results if needed for debugging
    full_path = f"benchmarks/resolve_{ts}{label_suffix}_full.json"
    with open(full_path, "w") as f:
        json.dump(response, f)
    print(f"Saved: {full_path}")


if __name__ == "__main__":
    main()
