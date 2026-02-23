#!/usr/bin/env python3
"""Feed text files into the HCP engine via the socket API (ingest action).

Full pipeline: tokenize → disassemble → store to Postgres.

Usage:
    python3 scripts/ingest_texts.py [file_or_glob ...]

    # Ingest specific files
    python3 scripts/ingest_texts.py data/gutenberg/texts/00043*.txt

    # Ingest first 5 Gutenberg texts
    python3 scripts/ingest_texts.py --first 5

    # Ingest all Gutenberg texts
    python3 scripts/ingest_texts.py --all
"""

import argparse
import glob
import json
import os
import socket
import struct
import sys
import time

ENGINE_HOST = "127.0.0.1"
ENGINE_PORT = 9720
GUTENBERG_DIR = os.path.join(os.path.dirname(__file__), "..", "data", "gutenberg", "texts")


def send_message(sock, data: bytes):
    sock.sendall(struct.pack("!I", len(data)) + data)


def recv_message(sock) -> bytes:
    header = b""
    while len(header) < 4:
        chunk = sock.recv(4 - len(header))
        if not chunk:
            raise ConnectionError("Connection closed")
        header += chunk
    length = struct.unpack("!I", header)[0]
    buf = b""
    while len(buf) < length:
        chunk = sock.recv(min(length - len(buf), 65536))
        if not chunk:
            raise ConnectionError("Connection closed during read")
        buf += chunk
    return buf


def ingest_file(sock, filepath: str, century: str = "AS") -> dict:
    name = os.path.splitext(os.path.basename(filepath))[0]
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    request = json.dumps({
        "action": "ingest",
        "name": name,
        "text": text,
        "century": century,
    })

    t0 = time.time()
    send_message(sock, request.encode("utf-8"))
    response = recv_message(sock)
    elapsed = time.time() - t0

    result = json.loads(response)
    result["_file"] = name
    result["_size"] = len(text)
    result["_wall_ms"] = round(elapsed * 1000, 1)
    return result


def main():
    parser = argparse.ArgumentParser(description="Ingest texts into HCP engine")
    parser.add_argument("files", nargs="*", help="Files to ingest")
    parser.add_argument("--first", type=int, help="Ingest first N Gutenberg texts")
    parser.add_argument("--all", action="store_true", help="Ingest all Gutenberg texts")
    parser.add_argument("--century", default="AS", help="Century code (default: AS)")
    parser.add_argument("--host", default=ENGINE_HOST)
    parser.add_argument("--port", type=int, default=ENGINE_PORT)
    args = parser.parse_args()

    # Resolve file list
    files = []
    if args.files:
        for pattern in args.files:
            files.extend(sorted(glob.glob(pattern)))
    elif args.first or args.all:
        gutenberg = os.path.abspath(GUTENBERG_DIR)
        all_files = sorted(glob.glob(os.path.join(gutenberg, "*.txt")))
        files = all_files if args.all else all_files[:args.first]
    else:
        parser.print_help()
        sys.exit(1)

    if not files:
        print("No files found.")
        sys.exit(1)

    print(f"Ingesting {len(files)} file(s) via {args.host}:{args.port}")
    print("-" * 80)

    # Connect once, send all files
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((args.host, args.port))
    except ConnectionRefusedError:
        print(f"ERROR: Cannot connect to engine at {args.host}:{args.port}")
        print("Is the engine running?")
        sys.exit(1)

    total_tokens = 0
    total_bytes = 0
    var_requests = 0

    try:
        for filepath in files:
            result = ingest_file(sock, filepath, args.century)

            status = result.get("status", "?")
            name = result["_file"]
            size = result["_size"]

            if status == "ok":
                tokens = result.get("tokens", 0)
                slots = result.get("slots", 0)
                unique = result.get("unique", 0)
                doc_id = result.get("doc_id", "")
                ms = result.get("ms", 0)
                wall = result["_wall_ms"]

                total_tokens += tokens
                total_bytes += size

                print(f"OK  {name}")
                print(f"    {size:,} bytes -> {tokens:,} tokens ({unique:,} unique), "
                      f"{slots:,} slots")
                print(f"    doc_id: {doc_id}")
                print(f"    engine: {ms:.1f}ms, wall: {wall:.1f}ms")
            else:
                msg = result.get("message", "unknown error")
                print(f"ERR {name}: {msg}")

            print()
    finally:
        sock.close()

    print("-" * 80)
    print(f"Total: {total_bytes:,} bytes, {total_tokens:,} tokens across {len(files)} files")


if __name__ == "__main__":
    main()
