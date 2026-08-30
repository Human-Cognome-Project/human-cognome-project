#!/usr/bin/env python3
"""HCP Asset Manager — CLI for the HCP engine.

Browse documents, inspect structure, review/update metadata, ingest files,
and retrieve content in token or text form.

Usage:
    python3 tools/hcp.py <command> [args...]

Commands:
    health              Engine status and vocabulary counts
    ingest <files...>   Ingest text files into the engine
    list                List all stored documents
    info <doc_id>       Full document detail (metadata, provenance, stats, vars)
    retrieve <doc_id>   Reconstruct and output original text
    tokens <doc_id>     Output token ID sequence (one per line)
    bonds <doc_id>      Bond overview or drill into a specific token
    meta <doc_id>       Show or update document metadata
    tokenize <text>     Analyze text without storing (no DB write)

Environment:
    HCP_ENGINE_HOST     Engine host (default: 127.0.0.1)
    HCP_ENGINE_PORT     Engine port (default: 9720)
"""

import argparse
import glob
import json
import os
import socket
import struct
import sys
import time


# ---- Socket protocol (length-prefixed JSON) ----

DEFAULT_HOST = os.environ.get("HCP_ENGINE_HOST", "127.0.0.1")
DEFAULT_PORT = int(os.environ.get("HCP_ENGINE_PORT", "9720"))


def connect(host, port):
    """Connect to the engine socket. Exits on failure."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((host, port))
    except ConnectionRefusedError:
        print(f"ERROR: Cannot connect to engine at {host}:{port}", file=sys.stderr)
        print("Is the engine running?", file=sys.stderr)
        sys.exit(1)
    return sock


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


def request(sock, payload: dict) -> dict:
    """Send a JSON request and return the parsed response."""
    send_message(sock, json.dumps(payload).encode("utf-8"))
    return json.loads(recv_message(sock))


def check_error(result):
    """Print error and exit if the result is an error."""
    if result.get("status") == "error":
        print(f"ERROR: {result.get('message', 'unknown error')}", file=sys.stderr)
        sys.exit(1)


# ---- Output formatting ----

def print_table(rows, headers):
    """Print aligned columns."""
    if not rows:
        print("(no results)")
        return
    widths = [len(h) for h in headers]
    for row in rows:
        for i, val in enumerate(row):
            widths[i] = max(widths[i], len(str(val)))
    fmt = "  ".join(f"{{:<{w}}}" for w in widths)
    print(fmt.format(*headers))
    print(fmt.format(*["-" * w for w in widths]))
    for row in rows:
        print(fmt.format(*[str(v) for v in row]))


def print_section(title, content):
    """Print a labeled section."""
    print(f"\n--- {title} ---")
    if isinstance(content, dict):
        for k, v in content.items():
            if isinstance(v, (list, dict)):
                print(f"  {k}: {json.dumps(v, indent=4, ensure_ascii=False)}")
            else:
                print(f"  {k}: {v}")
    elif isinstance(content, list):
        for item in content:
            if isinstance(item, dict):
                parts = [f"{k}={v}" for k, v in item.items()]
                print(f"  {', '.join(parts)}")
            else:
                print(f"  {item}")
    else:
        print(f"  {content}")


# ---- Commands ----

def cmd_health(args):
    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "health"})
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)
    print(f"Status:  {result.get('status', '?')}")
    print(f"Ready:   {result.get('ready', '?')}")
    print(f"Words:   {result.get('words', 0):,}")
    print(f"Labels:  {result.get('labels', 0):,}")
    print(f"Chars:   {result.get('chars', 0):,}")


def cmd_ingest(args):
    files = []
    for pattern in args.files:
        files.extend(sorted(glob.glob(pattern)))

    if not files:
        print("No files found.", file=sys.stderr)
        sys.exit(1)

    # Load metadata file if provided
    metadata_json = None
    if args.metadata:
        with open(args.metadata, "r", encoding="utf-8") as f:
            metadata_json = f.read()

    print(f"Ingesting {len(files)} file(s) via {args.host}:{args.port}")
    print("-" * 72)

    sock = connect(args.host, args.port)
    total_tokens = 0
    total_bytes = 0

    try:
        for filepath in files:
            name = os.path.splitext(os.path.basename(filepath))[0]

            payload = {
                "action": "ingest",
                "file": os.path.abspath(filepath),
                "name": name,
            }
            if args.century:
                payload["century"] = args.century
            if args.catalog:
                payload["catalog"] = args.catalog
            if metadata_json:
                payload["metadata"] = metadata_json

            t0 = time.time()
            result = request(sock, payload)
            wall_ms = (time.time() - t0) * 1000

            file_size = os.path.getsize(filepath)

            if args.json:
                result["_file"] = name
                result["_size"] = file_size
                result["_wall_ms"] = round(wall_ms, 1)
                print(json.dumps(result))
                continue

            status = result.get("status", "?")
            if status == "ok":
                tokens = result.get("tokens", 0)
                unique = result.get("unique", 0)
                doc_id = result.get("doc_id", "")
                ms = result.get("ms", 0)

                total_tokens += tokens
                total_bytes += file_size

                print(f"OK  {name}")
                print(f"    {file_size:,} bytes -> {tokens:,} tokens ({unique:,} unique)")
                print(f"    doc_id: {doc_id}")
                print(f"    engine: {ms:.1f}ms, wall: {wall_ms:.1f}ms")

                meta_known = result.get("meta_known", 0)
                meta_unreviewed = result.get("meta_unreviewed", 0)
                if meta_known or meta_unreviewed:
                    print(f"    metadata: {meta_known} known, {meta_unreviewed} unreviewed")
            else:
                msg = result.get("message", "unknown error")
                print(f"ERR {name}: {msg}")

            print()
    finally:
        sock.close()

    if not args.json:
        print("-" * 72)
        print(f"Total: {total_bytes:,} bytes, {total_tokens:,} tokens across {len(files)} files")


def cmd_list(args):
    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "list"})
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)
    docs = result.get("documents", [])
    print(f"{len(docs)} document(s)\n")

    rows = []
    for d in docs:
        rows.append([
            d.get("doc_id", ""),
            d.get("name", ""),
            f"{d.get('starters', 0):,}",
            f"{d.get('bonds', 0):,}",
        ])
    print_table(rows, ["DOC_ID", "NAME", "STARTERS", "BONDS"])


def cmd_info(args):
    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "info", "doc_id": args.doc_id})
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)

    print_section("Document", {
        "doc_id": result.get("doc_id"),
        "name": result.get("name"),
    })
    print_section("Stats", {
        "total_slots": f"{result.get('total_slots', 0):,}",
        "unique_tokens": f"{result.get('unique', 0):,}",
        "starters": f"{result.get('starters', 0):,}",
        "bonds": f"{result.get('bonds', 0):,}",
    })

    metadata = result.get("metadata", {})
    if metadata:
        print_section("Metadata", metadata)

    provenance = result.get("provenance")
    if provenance:
        print_section("Provenance", provenance)

    vars_list = result.get("vars", [])
    if vars_list:
        print_section("Variables", vars_list)


def cmd_retrieve(args):
    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "retrieve", "doc_id": args.doc_id})
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)

    text = result.get("text", "")
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(text)
        tokens = result.get("tokens", 0)
        load_ms = result.get("load_ms", 0)
        ms = result.get("ms", 0)
        print(f"Written {len(text):,} chars ({tokens:,} tokens) to {args.output}")
        print(f"Load: {load_ms:.1f}ms, Total: {ms:.1f}ms")
    else:
        sys.stdout.write(text)
        if text and not text.endswith("\n"):
            sys.stdout.write("\n")


def cmd_tokens(args):
    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "retrieve", "doc_id": args.doc_id})
    finally:
        sock.close()

    check_error(result)

    # We need positions with token IDs — retrieve gives us text.
    # For token view, use info to get stats, then explain limitation.
    # Actually, the engine's retrieve action returns text, not token IDs.
    # We need a way to get raw tokens. For now, re-tokenize the retrieved text.
    text = result.get("text", "")
    if not text:
        print("(empty document)", file=sys.stderr)
        return

    # Re-tokenize to get token IDs
    sock2 = connect(args.host, args.port)
    try:
        tok_result = request(sock2, {"action": "tokenize", "text": text})
    finally:
        sock2.close()

    if args.json:
        print(json.dumps(tok_result, indent=2))
        return

    check_error(tok_result)
    print(f"Tokens: {tok_result.get('tokens', 0):,}")
    print(f"Unique: {tok_result.get('unique', 0):,}")
    print(f"Bonds:  {tok_result.get('bonds', 0):,}")


def cmd_bonds(args):
    payload = {"action": "bonds", "doc_id": args.doc_id}
    if args.token:
        payload["token"] = args.token

    sock = connect(args.host, args.port)
    try:
        result = request(sock, payload)
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)

    if args.token:
        token = result.get("token", args.token)
        surface = result.get("surface", "")
        label = f"{token} ({surface})" if surface else token
        print(f"Bonds for: {label}\n")
    else:
        print(f"Top starters for {args.doc_id}\n")

    bonds = result.get("bonds", [])
    rows = []
    for b in bonds:
        token = b.get("token", "")
        surface = b.get("surface", "")
        count = b.get("count", 0)
        rows.append([token, surface, f"{count:,}"])

    header = ["TOKEN", "SURFACE", "COUNT"]
    print_table(rows, header)


def cmd_meta(args):
    # If --set or --remove specified, update; otherwise show
    if args.set or args.remove:
        # Build update payload
        payload = {"action": "update_meta", "doc_id": args.doc_id}

        if args.set:
            set_obj = {}
            for kv in args.set:
                if "=" not in kv:
                    print(f"ERROR: --set values must be key=value, got: {kv}", file=sys.stderr)
                    sys.exit(1)
                key, _, value = kv.partition("=")
                # Try to parse as JSON value (numbers, booleans, etc.)
                try:
                    set_obj[key] = json.loads(value)
                except json.JSONDecodeError:
                    set_obj[key] = value  # plain string
            payload["set"] = set_obj

        if args.remove:
            payload["remove"] = args.remove

        sock = connect(args.host, args.port)
        try:
            result = request(sock, payload)
        finally:
            sock.close()

        if args.json:
            print(json.dumps(result, indent=2))
            return

        check_error(result)
        print(f"Updated {args.doc_id}:")
        print(f"  Fields set:     {result.get('fields_set', 0)}")
        print(f"  Fields removed: {result.get('fields_removed', 0)}")
    else:
        # Show metadata via info action
        sock = connect(args.host, args.port)
        try:
            result = request(sock, {"action": "info", "doc_id": args.doc_id})
        finally:
            sock.close()

        if args.json:
            # Just the metadata portion
            check_error(result)
            print(json.dumps(result.get("metadata", {}), indent=2, ensure_ascii=False))
            return

        check_error(result)
        print(f"Metadata for {result.get('doc_id', args.doc_id)} ({result.get('name', '')})\n")
        metadata = result.get("metadata", {})
        if metadata:
            for k, v in metadata.items():
                if isinstance(v, (list, dict)):
                    print(f"  {k}: {json.dumps(v, indent=4, ensure_ascii=False)}")
                else:
                    print(f"  {k}: {v}")
        else:
            print("  (no metadata)")


def cmd_tokenize(args):
    text = args.text
    if text == "-":
        text = sys.stdin.read()

    sock = connect(args.host, args.port)
    try:
        result = request(sock, {"action": "tokenize", "text": text})
    finally:
        sock.close()

    if args.json:
        print(json.dumps(result, indent=2))
        return

    check_error(result)
    print(f"Tokens:   {result.get('tokens', 0):,}")
    print(f"Unique:   {result.get('unique', 0):,}")
    print(f"Bonds:    {result.get('bonds', 0):,}")
    print(f"Pairs:    {result.get('total_pairs', 0):,}")
    print(f"Original: {result.get('original_bytes', 0):,} bytes")
    print(f"Time:     {result.get('ms', 0):.1f}ms")


# ---- CLI setup ----

def main():
    parser = argparse.ArgumentParser(
        prog="hcp",
        description="HCP Asset Manager — manage documents in the HCP engine",
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help="Engine host")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Engine port")
    parser.add_argument("--json", action="store_true", help="Machine-readable JSON output")

    sub = parser.add_subparsers(dest="command", required=True)

    # health
    sub.add_parser("health", help="Engine status and vocabulary counts")

    # ingest
    p_ingest = sub.add_parser("ingest", help="Ingest text files")
    p_ingest.add_argument("files", nargs="+", help="File paths or glob patterns")
    p_ingest.add_argument("--century", default="AS", help="Century code (default: AS)")
    p_ingest.add_argument("--catalog", help="Source catalog name")
    p_ingest.add_argument("--metadata", help="Path to JSON metadata file")

    # list
    sub.add_parser("list", help="List all stored documents")

    # info
    p_info = sub.add_parser("info", help="Full document detail")
    p_info.add_argument("doc_id", help="Document ID (e.g., vA.AB.AS.AA.AA)")

    # retrieve
    p_retrieve = sub.add_parser("retrieve", help="Reconstruct original text")
    p_retrieve.add_argument("doc_id", help="Document ID")
    p_retrieve.add_argument("-o", "--output", help="Write to file instead of stdout")

    # tokens
    p_tokens = sub.add_parser("tokens", help="Token analysis for a document")
    p_tokens.add_argument("doc_id", help="Document ID")

    # bonds
    p_bonds = sub.add_parser("bonds", help="Bond overview or drill-down")
    p_bonds.add_argument("doc_id", help="Document ID")
    p_bonds.add_argument("token", nargs="?", help="Token ID to drill into")

    # meta
    p_meta = sub.add_parser("meta", help="Show or update metadata")
    p_meta.add_argument("doc_id", help="Document ID")
    p_meta.add_argument("--set", nargs="+", metavar="KEY=VALUE",
                         help="Set metadata fields (e.g., --set reviewed=true genre=fiction)")
    p_meta.add_argument("--remove", nargs="+", metavar="KEY",
                         help="Remove metadata fields (e.g., --remove download_count)")

    # tokenize
    p_tokenize = sub.add_parser("tokenize", help="Analyze text (no DB write)")
    p_tokenize.add_argument("text", help="Text to analyze (use '-' for stdin)")

    args = parser.parse_args()

    commands = {
        "health": cmd_health,
        "ingest": cmd_ingest,
        "list": cmd_list,
        "info": cmd_info,
        "retrieve": cmd_retrieve,
        "tokens": cmd_tokens,
        "bonds": cmd_bonds,
        "meta": cmd_meta,
        "tokenize": cmd_tokenize,
    }

    commands[args.command](args)


if __name__ == "__main__":
    main()
