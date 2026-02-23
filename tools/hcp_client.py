#!/usr/bin/env python3
"""
HCP Engine client — communicates with the HCP engine socket server.

Protocol: length-prefixed JSON messages.
  - 4 bytes: message length (big-endian)
  - N bytes: JSON payload (UTF-8)

Usage:
  # Health check
  python3 hcp_client.py health

  # Tokenize a file (analysis only, no DB)
  python3 hcp_client.py tokenize /path/to/text.txt

  # Ingest a file to the database
  python3 hcp_client.py ingest /path/to/text.txt "Document Name" [century_code]

  # Retrieve a document from the database
  python3 hcp_client.py retrieve vA.AB.AS.AA.AA

  # Batch ingest all Gutenberg texts
  python3 hcp_client.py batch /path/to/gutenberg/texts/
"""

import json
import socket
import struct
import sys
import os
import time
from pathlib import Path

HOST = "127.0.0.1"
PORT = 9720


def send_msg(sock, obj):
    """Send a length-prefixed JSON message."""
    payload = json.dumps(obj).encode("utf-8")
    sock.sendall(struct.pack("!I", len(payload)))
    sock.sendall(payload)


def recv_msg(sock):
    """Receive a length-prefixed JSON message."""
    header = b""
    while len(header) < 4:
        chunk = sock.recv(4 - len(header))
        if not chunk:
            raise ConnectionError("Connection closed")
        header += chunk

    length = struct.unpack("!I", header)[0]
    if length > 64 * 1024 * 1024:
        raise ValueError(f"Message too large: {length}")

    data = b""
    while len(data) < length:
        chunk = sock.recv(min(65536, length - len(data)))
        if not chunk:
            raise ConnectionError("Connection closed")
        data += chunk

    return json.loads(data.decode("utf-8"))


def connect():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    return sock


def cmd_health():
    sock = connect()
    send_msg(sock, {"action": "health"})
    resp = recv_msg(sock)
    sock.close()
    print(json.dumps(resp, indent=2))


def cmd_tokenize(filepath):
    text = Path(filepath).read_text(encoding="utf-8", errors="replace")
    sock = connect()
    send_msg(sock, {"action": "tokenize", "text": text})
    resp = recv_msg(sock)
    sock.close()

    if resp["status"] == "ok":
        print(f"File: {filepath}")
        print(f"  Original:  {resp['original_bytes']:,} bytes")
        print(f"  DB est:    {resp['db_bytes']:,} bytes")
        print(f"  Ratio:     {resp['ratio']:.3f}")
        print(f"  Tokens:    {resp['tokens']:,}")
        print(f"  Slots:     {resp['slots']:,}")
        print(f"  Unique:    {resp['unique']:,}")
        print(f"  Bonds:     {resp['bonds']:,}")
        print(f"  Time:      {resp['ms']:.1f} ms")
    else:
        print(f"Error: {resp.get('message', 'unknown')}")


def cmd_ingest(filepath, name=None, century="AS"):
    text = Path(filepath).read_text(encoding="utf-8", errors="replace")
    if name is None:
        name = Path(filepath).stem

    sock = connect()
    send_msg(sock, {
        "action": "ingest",
        "name": name,
        "century": century,
        "text": text
    })
    resp = recv_msg(sock)
    sock.close()

    if resp["status"] == "ok":
        print(f"Ingested: {name}")
        print(f"  Doc ID:    {resp['doc_id'] or '(DB unavailable)'}")
        print(f"  Tokens:    {resp['tokens']:,}")
        print(f"  Slots:     {resp['slots']:,}")
        print(f"  Unique:    {resp['unique']:,}")
        print(f"  Bonds:     {resp['bonds']:,}")
        print(f"  DB est:    {resp['db_bytes']:,} bytes")
        print(f"  Time:      {resp['ms']:.1f} ms")
    else:
        print(f"Error: {resp.get('message', 'unknown')}")


def cmd_retrieve(doc_id):
    sock = connect()
    send_msg(sock, {"action": "retrieve", "doc_id": doc_id})
    resp = recv_msg(sock)
    sock.close()

    if resp["status"] == "ok":
        print(f"Retrieved: {doc_id}")
        print(f"  Tokens:  {resp['tokens']:,}")
        print(f"  Slots:   {resp['slots']:,}")
        print(f"  Chars:   {len(resp['text']):,}")
        print(f"  Time:    {resp['ms']:.1f} ms")
        print(f"\n--- TEXT (first 500 chars) ---\n{resp['text'][:500]}")
    else:
        print(f"Error: {resp.get('message', 'unknown')}")


def cmd_batch(directory):
    """Batch tokenize all .txt files in a directory (analysis only, no DB)."""
    txt_files = sorted(Path(directory).glob("*.txt"))
    if not txt_files:
        print(f"No .txt files found in {directory}")
        return

    print(f"Found {len(txt_files)} files in {directory}\n")
    print(f"{'File':<60} {'Original':>10} {'DB Est':>10} {'Ratio':>7} {'Tokens':>8} {'Unique':>7} {'ms':>7}")
    print("-" * 115)

    totals = {"original": 0, "db": 0, "tokens": 0, "unique": 0, "ms": 0.0}
    sock = connect()

    for path in txt_files:
        text = path.read_text(encoding="utf-8", errors="replace")
        name = path.stem

        send_msg(sock, {"action": "tokenize", "text": text})
        resp = recv_msg(sock)

        if resp["status"] == "ok":
            orig = resp["original_bytes"]
            db = resp["db_bytes"]
            ratio = resp["ratio"]
            tokens = resp["tokens"]
            unique = resp["unique"]
            ms = resp["ms"]

            totals["original"] += orig
            totals["db"] += db
            totals["tokens"] += tokens
            totals["unique"] += unique
            totals["ms"] += ms

            short_name = name[:57] + "..." if len(name) > 60 else name
            print(f"{short_name:<60} {orig:>10,} {db:>10,} {ratio:>7.3f} {tokens:>8,} {unique:>7,} {ms:>7.1f}")
        else:
            print(f"{name:<60} ERROR: {resp.get('message', 'unknown')}")

    sock.close()

    print("-" * 115)
    total_ratio = totals["db"] / totals["original"] if totals["original"] > 0 else 0
    print(f"{'TOTAL':<60} {totals['original']:>10,} {totals['db']:>10,} {total_ratio:>7.3f} {totals['tokens']:>8,} {totals['unique']:>7,} {totals['ms']:>7.1f}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == "health":
        cmd_health()
    elif cmd == "tokenize" and len(sys.argv) >= 3:
        cmd_tokenize(sys.argv[2])
    elif cmd == "ingest" and len(sys.argv) >= 3:
        name = sys.argv[3] if len(sys.argv) > 3 else None
        century = sys.argv[4] if len(sys.argv) > 4 else "AS"
        cmd_ingest(sys.argv[2], name, century)
    elif cmd == "retrieve" and len(sys.argv) >= 3:
        cmd_retrieve(sys.argv[2])
    elif cmd == "batch" and len(sys.argv) >= 3:
        cmd_batch(sys.argv[2])
    else:
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
