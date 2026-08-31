#!/usr/bin/env python3
"""v1 word-instance stream — a doc as a position-indexed array of word-type addresses.

P's calibration (Zulip 1216/1219, seam §BOND STAGE): a doc = a position-indexed array
of token addresses; positions are CANONICAL storage, pairs/recurrence/frequency are
DERIVED views (frequency is a shadow, never fed). Bond topology for corpus runs =
position adjacency, so a dropped token is a BREAK in given adjacency and must be
visible to the kernel (gap flag), never a silent splice.

Id space = snode-chains-v0 word ids (row index of the same ORDER BY word query).
Token -> LOWEST id whose label matches (homonym sense deliberately unresolved:
sense = sector tag, loose-until-audio; the physics can revisit).

Proof: every kept position's id decodes byte-exact through the SHIPPED
snode-chains-v0.npz word_char chain (memoized per unique id) — the stream is proven
against the artifact the consumer holds, not against this script's own dict.
"""
import hashlib, json, re, sys
import numpy as np
from build_snode_tree import load

SLICE = 2_000_000
V0 = "snode-chains-v0"

def strip_gutenberg(raw):
    m0 = re.search(r"\*\*\* START OF THE PROJECT GUTENBERG EBOOK.*?\*\*\*", raw)
    m1 = re.search(r"\*\*\* END OF THE PROJECT GUTENBERG EBOOK.*?\*\*\*", raw)
    if not (m0 and m1):
        sys.exit("FAIL: Gutenberg start/end markers not found — refusing unstripped text")
    return raw[m0.end():m1.start()]

def main(txt_path, source_name, out_base):
    raw = open(txt_path, encoding="utf-8").read()
    sha = hashlib.sha256(raw.encode()).hexdigest()
    body = strip_gutenberg(raw).lower()

    char_glyph, _, words = load(SLICE)
    labels = [w for w, _, _ in words]
    first_id = {}
    for i, lab in enumerate(labels):
        first_id.setdefault(lab, i)

    v0man = json.load(open(f"{V0}.manifest.json"))
    wc = next(c for c in v0man["chains"] if c["name"] == "word_char")
    if wc["n_groups"] != len(labels):
        sys.exit(f"FAIL: id-space mismatch — v0 artifact {wc['n_groups']} words, db load {len(labels)}")

    ids, gaps = [], []
    pending_gap, dropped = 0, {}
    for m in re.finditer(r"[a-z]+", body):
        tok = m.group()
        i = first_id.get(tok)
        if i is None:
            dropped[tok] = dropped.get(tok, 0) + 1
            pending_gap = 1
            continue
        ids.append(i)
        gaps.append(pending_gap)
        pending_gap = 0
    ids = np.array(ids, dtype=np.int32)
    gaps = np.array(gaps, dtype=np.uint8)
    n_drop = sum(dropped.values())

    # proof: decode every kept position through the SHIPPED artifact's word_char chain
    npz = np.load(f"{V0}.npz")
    off, mem = npz["word_char__off"], npz["word_char__members"]
    decode = {}
    for u in np.unique(ids):
        decode[u] = "".join(char_glyph[c] for c in mem[off[u]:off[u + 1]])
    tok_stream = [t for t in re.findall(r"[a-z]+", body) if t in first_id]
    assert len(tok_stream) == len(ids)
    bad = sum(1 for t, i in zip(tok_stream, ids) if decode[i] != t)
    assert bad == 0, f"FAIL: {bad} positions decode wrong through {V0}"
    assert ids.min() >= 0 and ids.max() < len(labels) and len(gaps) == len(ids)

    # derived-view demo: frequency as a SHADOW computed from positions (never stored as cause)
    counts = np.bincount(ids, minlength=len(labels))
    top = counts.argsort()[::-1][:10]

    np.savez_compressed(out_base, doc_word__ids=ids, doc_word__gap_before=gaps)
    manifest = {
        "kind": "word-instance-stream",
        "version": "v1",
        "source": {"name": source_name, "sha256": sha, "chars_stripped_lowered": len(body)},
        "id_space": f"{V0} word ids (row index of ORDER BY word query, {len(labels)} rows); "
                    "token -> lowest id whose label matches; homonym sense unresolved by design",
        "normalization": "gutenberg markers stripped; text lowercased; tokens = [a-z]+ runs",
        "adjacency": "given sequence = consecutive kept positions; gap_before[i]=1 means >=1 "
                     "dropped (OOV) token between kept i-1 and i (or before first kept) — a break "
                     "in given adjacency; punctuation/whitespace do NOT set gaps",
        "counts": {"positions": int(len(ids)), "unique_types": int(len(decode)),
                   "oov_dropped": int(n_drop), "oov_unique": len(dropped),
                   "gap_positions": int(gaps.sum()),
                   "coverage": round(len(ids) / (len(ids) + n_drop), 6)},
        "proofs": {"decode_via_v0_artifact": f"{len(ids)} positions byte-exact (all, memoized)",
                   "id_space_matched_v0_manifest": True},
        "oov_top": sorted(dropped.items(), key=lambda kv: -kv[1])[:20],
        "frequency_shadow_top10_derived_not_fed": [[decode[i], int(counts[i])] for i in top],
    }
    with open(f"{out_base}.manifest.json", "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"PASS: {len(ids)} positions, {len(decode)} types, {n_drop} OOV dropped "
          f"({manifest['counts']['coverage']:.2%} coverage), {int(gaps.sum())} gaps")
    print("top10 (derived shadow):", manifest["frequency_shadow_top10_derived_not_fed"])
    print(f"wrote {out_base}.npz + {out_base}.manifest.json")

if __name__ == "__main__":
    a = sys.argv
    main(a[1] if len(a) > 1 else "pg2701.txt",
         a[2] if len(a) > 2 else "gutenberg-2701-moby-dick",
         a[3] if len(a) > 3 else "word-instance-stream-v1")
