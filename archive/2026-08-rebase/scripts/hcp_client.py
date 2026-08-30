#!/usr/bin/env python3
"""
HCP Engine client — length-framed protocol (4-byte big-endian prefix).

Usage:
    # Ingest a single file
    hcp_client.py ingest <file>

    # Ingest all .txt files in a directory
    hcp_client.py ingest <dir>

    # Resolve text inline (requires phys_resolve to be active)
    hcp_client.py resolve "<text>"

    # Delete a document by doc_id
    hcp_client.py delete <doc_id>

    # List all documents
    hcp_client.py list

    # Send a raw JSON action
    hcp_client.py raw '{"action":"ping"}'

Options:
    --host  Engine host (default: 127.0.0.1)
    --port  Engine port (default: 9720)
    --name  Document name for ingest (default: filename stem)
"""

import argparse
import json
import os
import socket
import struct
import sys
from pathlib import Path

HOST = "127.0.0.1"
PORT = 9720


def send_recv(host, port, payload: dict) -> dict:
    data = json.dumps(payload).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        # 4-byte big-endian length prefix
        s.sendall(struct.pack(">I", len(data)) + data)
        # Read response header
        header = b""
        while len(header) < 4:
            chunk = s.recv(4 - len(header))
            if not chunk:
                raise ConnectionError("Socket closed before header complete")
            header += chunk
        resp_len = struct.unpack(">I", header)[0]
        # Read response body
        body = b""
        while len(body) < resp_len:
            chunk = s.recv(min(65536, resp_len - len(body)))
            if not chunk:
                raise ConnectionError("Socket closed mid-response")
            body += chunk
    return json.loads(body.decode("utf-8"))


def do_ingest(host, port, path: Path, name: str | None = None):
    text = path.read_text(encoding="utf-8", errors="replace")
    doc_name = name or path.stem
    payload = {
        "action": "phys_ingest",
        "name": doc_name,
        "text": text,
    }
    print(f"  Ingesting '{doc_name}' ({len(text):,} chars)...", flush=True)
    resp = send_recv(host, port, payload)
    status = resp.get("status", "?")
    tokens = resp.get("unique_tokens", resp.get("tokens", resp.get("token_count", "?")))
    elapsed = resp.get("resolve_time_ms", resp.get("elapsed_ms", resp.get("time_ms", "?")))
    doc_id = resp.get("doc_id", "")
    print(f"  → status={status}  tokens={tokens}  elapsed={elapsed}ms  doc_id={doc_id}")
    if resp.get("error"):
        print(f"  ERROR: {resp['error']}", file=sys.stderr)
    return resp


def do_resolve(host, port, text: str):
    payload = {"action": "phys_resolve", "text": text}
    resp = send_recv(host, port, payload)
    print(json.dumps(resp, indent=2))
    return resp


def do_delete(host, port, doc_id: str):
    resp = send_recv(host, port, {"action": "delete_doc", "doc_id": doc_id})
    status = resp.get("status", "?")
    deleted = resp.get("deleted", "?")
    print(f"  delete_doc '{doc_id}' → status={status}  deleted={deleted}")
    if resp.get("message"):
        print(f"  {resp['message']}", file=sys.stderr)
    return resp


def do_list(host, port):
    resp = send_recv(host, port, {"action": "list"})
    docs = resp.get("documents", [])
    print(f"{'doc_id':<40} {'name':<40} {'starters':>8} {'bonds':>8}")
    print("-" * 100)
    for d in docs:
        print(f"{d.get('doc_id',''):<40} {d.get('name',''):<40} {d.get('starters',0):>8} {d.get('bonds',0):>8}")
    print(f"\n{len(docs)} document(s)")
    return resp


def do_raw(host, port, raw_json: str):
    payload = json.loads(raw_json)
    resp = send_recv(host, port, payload)
    print(json.dumps(resp, indent=2))
    return resp


def main():
    parser = argparse.ArgumentParser(description="HCP Engine client")
    parser.add_argument("command", choices=["ingest", "resolve", "delete", "list", "raw"])
    parser.add_argument("target", nargs="?", help="File, directory, text, doc_id, or raw JSON")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--name", default=None, help="Document name (ingest only)")
    args = parser.parse_args()

    if args.command == "ingest":
        if not args.target:
            print("ingest requires a file or directory argument", file=sys.stderr)
            sys.exit(1)
        p = Path(args.target)
        if p.is_dir():
            files = sorted(p.glob("*.txt"))
            if not files:
                print(f"No .txt files found in {p}", file=sys.stderr)
                sys.exit(1)
            print(f"Found {len(files)} .txt file(s) in {p}")
            ok = err = 0
            for f in files:
                try:
                    do_ingest(args.host, args.port, f)
                    ok += 1
                except Exception as e:
                    print(f"  FAILED: {e}", file=sys.stderr)
                    err += 1
            print(f"\nDone: {ok} ok, {err} failed")
        elif p.is_file():
            do_ingest(args.host, args.port, p, args.name)
        else:
            print(f"Not found: {p}", file=sys.stderr)
            sys.exit(1)

    elif args.command == "resolve":
        if not args.target:
            print("resolve requires a text argument", file=sys.stderr)
            sys.exit(1)
        do_resolve(args.host, args.port, args.target)

    elif args.command == "delete":
        if not args.target:
            print("delete requires a doc_id argument", file=sys.stderr)
            sys.exit(1)
        do_delete(args.host, args.port, args.target)

    elif args.command == "list":
        do_list(args.host, args.port)

    elif args.command == "raw":
        if not args.target:
            print("raw requires a JSON argument", file=sys.stderr)
            sys.exit(1)
        do_raw(args.host, args.port, args.target)


if __name__ == "__main__":
    main()
