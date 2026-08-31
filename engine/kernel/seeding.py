#!/usr/bin/env python3
"""Field-engine SEEDING — type-tree → instance expansion (consumer half, silas).

Seam: ~/shared-brain/exchange/field-engine-interface-v0.md
  §MAP GRAIN  — producer ships dedup'd TYPE-tree; consumer seeding expands to
                instances (mechanical walk word→char→byte→nibble).
  §Consumer O1 — leaf grain = NIBBLE; byte is a containment rung.
  §Init        — each leaf seeds amp one-hot on its nibble value; unseen = 1/16.

Input : snode-chains-v0.npz + .manifest.json (Planner's producer emit).
Output: instance-seed-v0.npz + .manifest.json in --outdir.

Instance-grain artifact (all CSR members IMPLICIT-CONTIGUOUS — the expansion
walk is depth-first in order, so every parent's instance children occupy a
contiguous index range; offsets alone define the chain, members = arange):
  leaf_val[N_leaf]            uint8  nibble value 0..15  (THE amp seed; amp is
                                     materialized one-hot in-kernel, not here)
  word_charinst_off[W+1]      int64  word w's char instances = [off[w], off[w+1])
  charinst_byteinst_off[C+1]  int64  char instance c's byte instances
  byteinst_leaf: implicit stride-2 (byte instance b's leaves = [2b, 2b+2), hi,lo)
  charinst_type[C]            int32  char-TYPE id per char instance
  byteinst_type[N_byte]       uint8  byte VALUE per byte instance (type id = value)
Cross-cutting chains (pos, homonym) pass through UNCHANGED: v0 lexicon has each
word type exactly once, so word-type id == word-instance id (seam §MAP GRAIN caveat).

Hard self-checks run on EVERY build (no --selftest flag to forget):
  CSR invariants, count identities, leaf-domain, byte↔nibble recomposition
  against the producer's own byte_nibble chain, and EXTERNAL-GROUND round-trip:
  random homonym sectors' member words must UTF-8-decode to the sector label.
Taichi ingest proof: --ti-proof materializes amp one-hot from leaf_val in a
real @ti.kernel and asserts per-particle sum==1 & argmax==leaf_val (sampled)
plus a full-pool reduction (kernel == numpy).
"""
import argparse, hashlib, json, os, sys
import numpy as np

EXCHANGE = os.path.expanduser("~/shared-brain/exchange")
SRC_NPZ = os.path.join(EXCHANGE, "snode-chains-v0.npz")
SRC_MANIFEST = os.path.join(EXCHANGE, "snode-chains-v0.manifest.json")


def die(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def check(cond, msg):
    if not cond:
        die(msg)
    print(f"  ok: {msg}")


def load_source():
    z = np.load(SRC_NPZ, allow_pickle=False)
    man = json.load(open(SRC_MANIFEST))
    chains = {c["name"]: c for c in man["chains"]}
    if man.get("leaf_grain") != "nibble":
        die(f"producer leaf_grain={man.get('leaf_grain')!r}, seeding assumes nibble")
    return z, man, chains


def decode_char_type(t, cb_off, cb_members):
    """char TYPE id -> the actual character, via its UTF-8 byte sequence."""
    bs = bytes(cb_members[cb_off[t]:cb_off[t + 1]].tolist())
    return bs.decode("utf-8")


def expand(z):
    """The mechanical walk. Returns the instance arrays."""
    wc_off = z["word_char__off"].astype(np.int64)
    wc_members = z["word_char__members"]          # char-type id per char INSTANCE, in spelling order
    cb_off = z["char_byte__off"].astype(np.int64)
    cb_members = z["char_byte__members"].astype(np.uint8)

    # word -> char-instance: expansion preserves order, so offsets ARE wc_off
    word_charinst_off = wc_off
    charinst_type = wc_members.astype(np.int32)
    n_char_inst = charinst_type.shape[0]

    # char-instance -> byte-instance
    clen = (cb_off[1:] - cb_off[:-1])             # bytes per char TYPE
    inst_blen = clen[charinst_type]               # bytes per char INSTANCE
    charinst_byteinst_off = np.zeros(n_char_inst + 1, dtype=np.int64)
    np.cumsum(inst_blen, out=charinst_byteinst_off[1:])
    n_byte_inst = int(charinst_byteinst_off[-1])

    # gather each instance's byte sequence (vectorized repeat-gather)
    rep_start = np.repeat(cb_off[charinst_type], inst_blen)      # type's byte-seq start, per emitted byte
    rep_base = np.repeat(charinst_byteinst_off[:-1], inst_blen)  # instance's own start, per emitted byte
    idx = np.arange(n_byte_inst, dtype=np.int64) - rep_base + rep_start
    byteinst_type = cb_members[idx]               # byte VALUE per byte instance

    # byte-instance -> leaves via the PRODUCER'S byte_nibble chain (its data is
    # the authority; shift/mask is only the independent cross-check, in checks())
    bn_off = z["byte_nibble__off"].astype(np.int64)
    bn_members = z["byte_nibble__members"].astype(np.uint8)
    if not np.all(bn_off[1:] - bn_off[:-1] == 2):
        die("byte_nibble chain is not uniformly 2 members per byte")
    nib_pairs = bn_members.reshape(256, 2)        # [byte value] -> (first, second) nibble
    leaf_val = nib_pairs[byteinst_type].reshape(-1)  # N_leaf = 2 * n_byte_inst

    return dict(
        leaf_val=leaf_val,
        word_charinst_off=word_charinst_off,
        charinst_byteinst_off=charinst_byteinst_off,
        charinst_type=charinst_type,
        byteinst_type=byteinst_type,
    )


def checks(z, chains, inst, n_sectors=200, seed=14):
    print("— self-checks —")
    wc_off = z["word_char__off"]
    cb_off = z["char_byte__off"].astype(np.int64)
    cb_members = z["char_byte__members"].astype(np.uint8)
    W = chains["word_char"]["n_groups"]

    # CSR invariants on emitted offsets
    for name in ("word_charinst_off", "charinst_byteinst_off"):
        off = inst[name]
        check(off[0] == 0 and np.all(off[1:] >= off[:-1]), f"{name} monotone from 0")
    check(inst["word_charinst_off"].shape[0] == W + 1, "word_charinst_off has W+1 entries")

    # count identities
    C = inst["charinst_type"].shape[0]
    B = inst["byteinst_type"].shape[0]
    L = inst["leaf_val"].shape[0]
    check(C == chains["word_char"]["n_members"], f"n_char_inst == producer n_members ({C})")
    check(int(inst["charinst_byteinst_off"][-1]) == B, f"byte-instance count closes CSR ({B})")
    check(L == 2 * B, f"n_leaf == 2 * n_byte_inst ({L})")

    # leaf domain + recomposition: producer's byte_nibble chain vs shift/mask (independent)
    lv = inst["leaf_val"]
    check(lv.min() >= 0 and lv.max() <= 15, "leaf_val ∈ [0,15]")
    hi, lo = lv[0::2].astype(np.uint16), lv[1::2].astype(np.uint16)
    recomposed = (hi << 4) | lo
    check(np.array_equal(recomposed, inst["byteinst_type"].astype(np.uint16)),
          "every byte recomposes from its 2 leaves (hi<<4|lo) == byteinst_type")

    # EXTERNAL GROUND: random homonym sectors — a member word must decode to the label
    hom = chains["homonym"]
    h_off = z["homonym__off"].astype(np.int64)
    h_members = z["homonym__members"]
    labels = hom["group_labels"]
    rng = np.random.default_rng(seed)
    sample = rng.choice(hom["n_groups"], size=min(n_sectors, hom["n_groups"]), replace=False)
    char_cache = {}
    hits = 0
    for g in sample:
        label = labels[g]
        found = False
        for w in h_members[h_off[g]:h_off[g + 1]]:
            s = []
            for t in z["word_char__members"][wc_off[w]:wc_off[w + 1]]:
                if t not in char_cache:
                    char_cache[t] = decode_char_type(int(t), cb_off, cb_members)
                s.append(char_cache[t])
            if "".join(s) == label:
                found = True
                break
        hits += found
    check(hits == len(sample),
          f"external ground: {hits}/{len(sample)} sampled homonym sectors contain a member "
          f"word that decodes (word→char→byte→utf8) exactly to the sector label")

    # instance-path == type-path decode for a few words (instance CSR walks agree)
    for w in rng.choice(W, size=20, replace=False):
        c0, c1 = inst["word_charinst_off"][w], inst["word_charinst_off"][w + 1]
        b0 = inst["charinst_byteinst_off"][c0]
        b1 = inst["charinst_byteinst_off"][c1]
        via_inst = bytes(inst["byteinst_type"][b0:b1].tolist()).decode("utf-8")
        via_type = "".join(
            decode_char_type(int(t), cb_off, cb_members)
            for t in z["word_char__members"][wc_off[w]:wc_off[w + 1]])
        if via_inst != via_type:
            die(f"word {w}: instance-path {via_inst!r} != type-path {via_type!r}")
    print("  ok: instance-path decode == type-path decode (20 random words)")


def synthetic_multibyte_check():
    """Positive control for the multi-byte path: v0 English uses only a-z (all
    1-byte), so real data never exercises clen>1 expansion. Feed expand() a
    synthetic tree containing 'é' (2B) and '中' (3B) and prove the walk."""
    words = ["café", "中a"]
    chars = sorted({c for w in words for c in w})           # char alphabet
    cid = {c: i for i, c in enumerate(chars)}
    wc_off, wc_members = [0], []
    for w in words:
        wc_members += [cid[c] for c in w]
        wc_off.append(len(wc_members))
    cb_off, cb_members = [0], []
    for c in chars:
        cb_members += list(c.encode("utf-8"))
        cb_off.append(len(cb_members))
    bn_off = np.arange(0, 514, 2, dtype=np.int32)
    bn_members = np.stack([np.arange(256) >> 4, np.arange(256) & 15], 1).reshape(-1)
    zs = {
        "word_char__off": np.array(wc_off, np.int32),
        "word_char__members": np.array(wc_members, np.int32),
        "char_byte__off": np.array(cb_off, np.int32),
        "char_byte__members": np.array(cb_members, np.int32),
        "byte_nibble__off": bn_off,
        "byte_nibble__members": bn_members.astype(np.int32),
    }
    inst = expand(zs)
    for w, word in enumerate(words):
        c0, c1 = inst["word_charinst_off"][w], inst["word_charinst_off"][w + 1]
        b0 = inst["charinst_byteinst_off"][c0]
        b1 = inst["charinst_byteinst_off"][c1]
        got = bytes(inst["byteinst_type"][b0:b1].tolist()).decode("utf-8")
        if got != word:
            die(f"synthetic multi-byte: word {word!r} expanded to {got!r}")
    lv = inst["leaf_val"]
    if not np.array_equal((lv[0::2].astype(np.uint16) << 4) | lv[1::2],
                          inst["byteinst_type"].astype(np.uint16)):
        die("synthetic multi-byte: leaf recomposition mismatch")
    check(True, "synthetic positive control: multi-byte chars ('é' 2B, '中' 3B) "
                "expand + decode exactly (real v0 data is a-z only, never hits this path)")


def ti_proof(inst, sample=100_000, seed=14):
    """Real @ti.kernel: materialize amp one-hot from leaf_val, assert seed law."""
    import taichi as ti
    ti.init(arch=ti.cpu, default_fp=ti.f32)
    lv = inst["leaf_val"]
    N = lv.shape[0]
    amp = ti.field(ti.f32, shape=(N, 16))
    lv_field = ti.field(ti.u8, shape=N)
    lv_field.from_numpy(lv)

    @ti.kernel
    def seed_amp():
        for i, k in amp:
            amp[i, k] = 1.0 if k == ti.cast(lv_field[i], ti.i32) else 0.0

    @ti.kernel
    def total() -> ti.f64:
        s = 0.0
        for i, k in amp:
            s += amp[i, k]
        return s

    seed_amp()
    t = total()
    if abs(t - N) > 0.5:
        die(f"ti amp total {t} != N {N} (each particle must sum to exactly 1)")
    rng = np.random.default_rng(seed)
    idx = rng.choice(N, size=min(sample, N), replace=False)
    sub = amp.to_numpy()[idx]  # sampled readback
    if not (np.array_equal(sub.argmax(1), lv[idx]) and np.allclose(sub.sum(1), 1.0)):
        die("ti amp sample: argmax/sum mismatch vs leaf_val")
    print(f"  ok: @ti.kernel seeded amp[{N:,},16] one-hot; total=={N:,}; "
          f"{len(idx):,}-sample argmax==leaf_val & rows sum to 1")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", default=os.path.expanduser("~/workspace/field-engine-kernel"))
    ap.add_argument("--ti-proof", action="store_true", help="also run the Taichi ingest proof")
    args = ap.parse_args()

    z, man, chains = load_source()
    print("expanding type-tree → instances …")
    inst = expand(z)
    checks(z, chains, inst)
    synthetic_multibyte_check()
    if args.ti_proof:
        print("— taichi ingest proof —")
        ti_proof(inst)

    out_npz = os.path.join(args.outdir, "instance-seed-v0.npz")
    np.savez_compressed(
        out_npz,
        leaf_val=inst["leaf_val"],
        word_charinst_off=inst["word_charinst_off"],
        charinst_byteinst_off=inst["charinst_byteinst_off"],
        charinst_type=inst["charinst_type"],
        byteinst_type=inst["byteinst_type"],
    )
    src_sha = hashlib.sha256(open(SRC_NPZ, "rb").read()).hexdigest()
    manifest = {
        "artifact": "instance-seed-v0",
        "leaf_grain": "nibble",
        "counts": {
            "words": int(inst["word_charinst_off"].shape[0] - 1),
            "char_instances": int(inst["charinst_type"].shape[0]),
            "byte_instances": int(inst["byteinst_type"].shape[0]),
            "leaves": int(inst["leaf_val"].shape[0]),
        },
        "csr_members": "implicit-contiguous (depth-first in-order expansion; a parent's "
                       "instance children are a contiguous range — offsets alone define "
                       "each chain, members == arange)",
        "byteinst_leaf": "implicit stride-2: byte instance b owns leaves [2b, 2b+1] = (hi, lo)",
        "amp_seed_law": "amp[i] = one-hot(leaf_val[i]); unseen/background particles = uniform 1/16 "
                        "(materialized in-kernel, never on disk)",
        "cross_cutting": "pos + homonym chains pass through from producer npz UNCHANGED — "
                         "word-type id == word-instance id in the v0 lexicon (seam §MAP GRAIN; "
                         "breaks when a running-text corpus repeats words → needs producer "
                         "word-instance stream, v1)",
        "provenance": {"source_npz": SRC_NPZ, "source_sha256": src_sha},
    }
    out_man = os.path.join(args.outdir, "instance-seed-v0.manifest.json")
    json.dump(manifest, open(out_man, "w"), indent=2)
    print(f"wrote {out_npz} ({os.path.getsize(out_npz)/1e6:.1f} MB) + manifest")
    print(json.dumps(manifest["counts"], indent=2))


if __name__ == "__main__":
    main()
