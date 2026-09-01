#!/usr/bin/env python3
"""build_raw_chains — R1 token-range chain generator (producer lane, spec §F5 OPEN-R1).

Emits the CHAINS over the verbatim Moby blob; the blob itself is NEVER rewritten
(P-STEER: dumb blobs, structure as added chains). The pour rig (silas, pour_raw.py)
owns the blob leg; this owns the derived views:

  token__lo / token__hi   maximal ASCII-letter runs [A-Za-z]+, byte offsets into
                          the raw blob (positions canonical). Case, punctuation,
                          digits, CRLF, BOM, UTF-8 continuation bytes remain given
                          matter — they are simply not inside token spans.
  line__start             offset of each line start (0 and every byte after 0x0a).
  token__type             type id per token; word-type = chain equivalence class
                          under ASCII case-fold (recurrence DERIVED, v1 stream law).
  type_spell__off/bytes   packed folded spellings, CSR (array-stored, compositional
                          addressing — no unicode dtypes).
  type__count             tokens per type.

Declared derived rules (chain choices, replaceable without touching matter):
  R-token: token = maximal run of bytes 0x41-0x5A / 0x61-0x7A. Apostrophes split
           ("whale's" -> "whale","s") — a declared chain choice, not a deletion.
  R-fold:  type equivalence = byte|0x20 on A-Z only (ASCII case-fold).
  R-line:  line starts at 0 and after every 0x0a (CRLF file: 0x0d stays in-line).

PROVENANCE GATE: refuses to emit unless source sha256 == the spec pin. Acceptance
checks run in-process; any failure -> no artifact. Includes a NEGATIVE control
(a corrupted span must FAIL the checker) so the checker is proven falsifiable.

Planner, 2026-08-31.
"""
import hashlib
import json
import sys
from pathlib import Path

import numpy as np

SRC_REPO_REL = "data/gutenberg/texts/02701_Moby Dick Or The Whale.txt"
_here = Path(__file__).resolve().parent
_repo_root = _here.parent.parent if (_here.parent.parent / "data").is_dir() else None
if _repo_root is not None:                      # running from engine/storage/ in the repo
    SRC, EXCH = _repo_root / SRC_REPO_REL, _here
else:                                           # Haven exchange seat: repo checkout beside us
    EXCH = Path.home() / "shared-brain" / "exchange"
    SRC = EXCH / "P_folder" / "project" / "repo" / SRC_REPO_REL
SHA_PIN = "7cc6f1aac955b38c5a59306f48fe2d752d917549e8019c0f6ea65fb52d1d21cc"
OUT = EXCH / "moby-raw-chains-v0.npz"
MANIFEST = EXCH / "moby-raw-chains-v0.manifest.json"

data = SRC.read_bytes()
sha = hashlib.sha256(data).hexdigest()
if sha != SHA_PIN:
    sys.exit(f"PROVENANCE GATE: sha {sha} != pin {SHA_PIN} — refusing to emit")
b = np.frombuffer(data, dtype=np.uint8)
N = len(b)

# --- token spans: maximal [A-Za-z]+ runs ---
is_letter = ((b >= 0x41) & (b <= 0x5A)) | ((b >= 0x61) & (b <= 0x7A))
edge = np.diff(is_letter.astype(np.int8))
lo = np.flatnonzero(edge == 1) + 1
hi = np.flatnonzero(edge == -1) + 1
if is_letter[0]:
    lo = np.concatenate([[0], lo])
if is_letter[-1]:
    hi = np.concatenate([hi, [N]])
lo = lo.astype(np.int64); hi = hi.astype(np.int64)
n_tok = len(lo)

# --- line-boundary chain ---
nl = np.flatnonzero(b == 0x0A).astype(np.int64)
line_start = np.concatenate([[np.int64(0)], nl + 1])

# --- type chain: ASCII case-fold equivalence over token bytes ---
fold = b.copy()
upper = (b >= 0x41) & (b <= 0x5A)
fold[upper] |= 0x20
types = {}
tok_type = np.empty(n_tok, dtype=np.int64)
for i in range(n_tok):
    s = fold[lo[i]:hi[i]].tobytes()
    t = types.get(s)
    if t is None:
        t = len(types)
        types[s] = t
    tok_type[i] = t
spellings = list(types.keys())
type_off = np.zeros(len(spellings) + 1, dtype=np.int64)
np.cumsum([len(s) for s in spellings], out=type_off[1:])
type_bytes = np.frombuffer(b"".join(spellings), dtype=np.uint8)
type_count = np.bincount(tok_type, minlength=len(spellings)).astype(np.int64)


def check_spans(lo_a, hi_a):
    """Acceptance: well-formed, maximal, letters-only, byte-exact against the blob."""
    if not (len(lo_a) == len(hi_a) and np.all(hi_a > lo_a)
            and np.all(lo_a[1:] >= hi_a[:-1]) and lo_a[0] >= 0 and hi_a[-1] <= N):
        return "spans malformed"
    covered = np.zeros(N, dtype=bool)
    for l, h in ((lo_a, hi_a),):
        idx = np.repeat(l, (h - l))
        covered[idx + _ranges(h - l)] = True
    if not np.array_equal(covered, is_letter):
        return "span union != letter mask (re-slice mismatch)"
    before_ok = np.all(~is_letter[lo_a[lo_a > 0] - 1])
    after_ok = np.all(~is_letter[hi_a[hi_a < N]])
    if not (before_ok and after_ok):
        return "spans not maximal"
    return None


def _ranges(lengths):
    """[3,2] -> [0,1,2,0,1] — per-span local offsets, vectorized."""
    ends = np.cumsum(lengths)
    out = np.arange(ends[-1], dtype=np.int64)
    out -= np.repeat(ends - lengths, lengths)
    return out


errs = []
e = check_spans(lo, hi)
if e:
    errs.append(f"token spans: {e}")
# negative control: a corrupted copy MUST fail (checker falsifiability)
if n_tok > 1:
    bad_hi = hi.copy(); bad_hi[0] = min(bad_hi[0] + 1, N)
    if check_spans(lo, bad_hi) is None:
        errs.append("NEGATIVE CONTROL PASSED: checker cannot fail — void")
# lines: every start-1 is 0x0a; count identity
if line_start[0] != 0 or not np.all(b[line_start[1:] - 1] == 0x0A) \
        or len(line_start) != len(nl) + 1:
    errs.append("line chain: start positions inconsistent with 0x0a census")
# types: table roundtrip on a sample (positive control against the packed table)
rng = np.random.default_rng(178)
for i in rng.integers(0, n_tok, 200):
    t = tok_type[i]
    if fold[lo[i]:hi[i]].tobytes() != type_bytes[type_off[t]:type_off[t + 1]].tobytes():
        errs.append(f"type table mismatch at token {i}")
        break
if int(type_count.sum()) != n_tok:
    errs.append("type counts do not sum to token count")

if errs:
    for x in errs:
        print(f"FAIL: {x}")
    sys.exit(1)

np.savez_compressed(OUT, token__lo=lo, token__hi=hi, line__start=line_start,
                    token__type=tok_type, type_spell__off=type_off,
                    type_spell__bytes=type_bytes, type__count=type_count)
manifest = {
    "artifact": "moby-raw-chains-v0",
    "source_repo_relpath": SRC_REPO_REL,
    "source_sha256": SHA_PIN,
    "source_bytes": N,
    "rules": {
        "R-token": "maximal [A-Za-z]+ byte runs; offsets into raw blob",
        "R-fold": "ASCII case-fold (byte|0x20 on A-Z) for type equivalence",
        "R-line": "line starts at 0 and after every 0x0a",
    },
    "counts": {"tokens": n_tok, "types": len(spellings),
               "lines": int(len(line_start)),
               "letter_bytes": int(is_letter.sum()),
               "nonletter_bytes": int(N - is_letter.sum())},
    "acceptance": "spans well-formed+maximal+byte-exact vs letter mask; negative "
                  "control corrupted-span FAILED as required; line starts == 0x0a "
                  "census; type table roundtrip 200-sample",
    "spec": "raw-ingest-realignment-spec-draft-v0-2026-08-31.md §F5 OPEN-R1",
    "author": "Planner",
    "date": "2026-08-31",
}
MANIFEST.write_text(json.dumps(manifest, indent=1))
print(f"OK  tokens={n_tok}  types={len(spellings)}  lines={len(line_start)}  "
      f"letters={int(is_letter.sum())}/{N}")
print(f"    v1-stream comparison (report-only): v1 kept 215,092 positions of a "
      f"normalized [a-z]+ stream; raw chain sees ALL letter-runs incl. case-split "
      f"and OOV — counts are expected to differ by design.")
print(f"    wrote {OUT.name} + {MANIFEST.name}")
