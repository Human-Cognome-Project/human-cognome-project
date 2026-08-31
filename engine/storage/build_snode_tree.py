#!/usr/bin/env python3
"""SNode English tree v0 — word -> char -> byte as array-stored compositional addressing.

WHY this exists: hcp_english addresses a word by entries.token_id = 'ns.p2.p3.p4.p5'
-- a dot-joined TEXT string ("delineated block"), hard-capped at 5 rungs. P's invariant:
ALL addressing must be ARRAY-STORED and COMPOSITIONAL -- an address is a COMPOSED INDEX
into contiguous array backing, reachable at any granularity by composing sub-indices,
NOT a pointer to a discrete block. This builds the first exercise (English, full
word->char->byte) that way, and backs the dense leaves with real Taichi dense SNodes so
the invariant holds BY CONSTRUCTION: you cannot address a dense SNode field except by
index-into-array. No 5-cap (depth is whatever the composition needs); no dot-string.

Ragged levels use CSR (offsets + one flat contiguous values array) -- the array-native
form of a ragged tree, and the shape a Taichi dense SNode wants:
    word --(spelling)--> char --(utf8)--> byte
Reads hcp_english READ-ONLY. Self-checks by lossless round-trip read back OUT of the
SNode fields (via to_numpy) -- a composition bug breaks reconstruction and exits 1.
"""
import sys, argparse
import numpy as np
import psycopg2
import taichi as ti

import os
DSN = os.environ.get("HCP_ENGLISH_DSN",
                     "host=localhost port=5435 dbname=hcp_english user=hcp")
# password comes from HCP_ENGLISH_DSN or ~/.pgpass — never from source


def load(slice_n):
    """Return (chars, words) from the live substrate. chars indexed by english_characters.seq."""
    cx = psycopg2.connect(DSN)
    cur = cx.cursor()

    # --- char level: seq is the alphabet index entries.spelling composes into (0=a,1=b,...) ---
    cur.execute("SELECT seq, character, codepoint FROM english_characters ORDER BY seq")
    rows = cur.fetchall()
    max_seq = max(r[0] for r in rows)
    char_glyph = [None] * (max_seq + 1)
    char_cp = np.full(max_seq + 1, -1, dtype=np.int32)
    for seq, glyph, cp in rows:
        char_glyph[seq] = glyph
        char_cp[seq] = cp

    # --- word slice: real, bounded, sorted so same-spelling homonyms cluster ---
    cur.execute(
        "SELECT word, pos, spelling FROM entries "
        "WHERE spelling IS NOT NULL AND word ~ '^[a-z]+$' "
        "ORDER BY word LIMIT %s", (slice_n,))
    words = cur.fetchall()          # (word, pos, spelling[])
    cx.close()
    return char_glyph, char_cp, words


def build(char_glyph, char_cp, words):
    ti.init(arch=ti.cpu, offline_cache=False)

    n_char = len(char_glyph)
    n_word = len(words)

    # ---- byte level: the 256 possible byte values, a dense contiguous array (identity).
    #      Now a CONTAINMENT rung, not the leaf (Silas O1) -- byte -> 2 nibbles below. ----
    byte_space = ti.field(ti.i32)
    ti.root.dense(ti.i, 256).place(byte_space)

    # ---- nibble level: 16 hex nibbles, dense identity. THE LEAF (Silas O1, 2026-08-30):
    #      the kernel's density state is per-nibble (base-16 element); a byte leaf would
    #      alias 2 nibble-particles onto one density cell. So storage matches kernel grain:
    #      ladder is word -> char -> byte -> nibble(leaf). ----
    nibble_space = ti.field(ti.i32)
    ti.root.dense(ti.i, 16).place(nibble_space)

    # ---- byte -> nibble, CSR: each byte value -> [hi, lo] nibble (indices into nibble_space) ----
    bn_off = [0]
    bn_val = []
    for b in range(256):
        bn_val.append(b >> 4)      # hi nibble
        bn_val.append(b & 0xF)     # lo nibble
        bn_off.append(len(bn_val))
    bn_off = np.array(bn_off, dtype=np.int32)     # len 257
    bn_val = np.array(bn_val, dtype=np.int32)     # len 512, values 0..15
    byte_nibble_off = ti.field(ti.i32); ti.root.dense(ti.i, 257).place(byte_nibble_off)
    byte_nibble_val = ti.field(ti.i32); ti.root.dense(ti.i, 512).place(byte_nibble_val)

    # ---- char -> byte, CSR into a flat byte-value array (indices into byte_space) ----
    # each char's UTF-8 bytes, ground-truth from the glyph.
    cb_off = [0]
    cb_val = []
    for seq in range(n_char):
        g = char_glyph[seq]
        b = g.encode("utf-8") if g is not None else b""
        cb_val.extend(b)
        cb_off.append(len(cb_val))
    cb_off = np.array(cb_off, dtype=np.int32)      # len n_char+1
    cb_val = np.array(cb_val, dtype=np.int32)      # flat

    char_byte_off = ti.field(ti.i32); ti.root.dense(ti.i, n_char + 1).place(char_byte_off)
    char_byte_val = ti.field(ti.i32); ti.root.dense(ti.i, max(len(cb_val), 1)).place(char_byte_val)
    char_codepoint = ti.field(ti.i32); ti.root.dense(ti.i, n_char).place(char_codepoint)

    # ---- word -> char, CSR into a flat char-seq array (the spelling; indices into chars) ----
    wc_off = [0]
    wc_seq = []
    pos_of = []
    glyph_of = []
    for word, pos, spelling in words:
        wc_seq.extend(int(s) for s in spelling)
        wc_off.append(len(wc_seq))
        pos_of.append(pos)
        glyph_of.append(word)
    wc_off = np.array(wc_off, dtype=np.int32)      # len n_word+1
    wc_seq = np.array(wc_seq, dtype=np.int32)      # flat spelling

    word_char_off = ti.field(ti.i32); ti.root.dense(ti.i, n_word + 1).place(word_char_off)
    word_char_seq = ti.field(ti.i32); ti.root.dense(ti.i, max(len(wc_seq), 1)).place(word_char_seq)

    # ---- materialize: push numpy into the SNode-backed fields ----
    byte_space.from_numpy(np.arange(256, dtype=np.int32))
    nibble_space.from_numpy(np.arange(16, dtype=np.int32))
    byte_nibble_off.from_numpy(bn_off)
    byte_nibble_val.from_numpy(bn_val)
    char_byte_off.from_numpy(cb_off)
    char_byte_val.from_numpy(cb_val)
    char_codepoint.from_numpy(char_cp)
    word_char_off.from_numpy(wc_off)
    word_char_seq.from_numpy(wc_seq)

    fields = dict(byte_space=byte_space, nibble_space=nibble_space,
                  byte_nibble_off=byte_nibble_off, byte_nibble_val=byte_nibble_val,
                  char_byte_off=char_byte_off, char_byte_val=char_byte_val,
                  char_codepoint=char_codepoint, word_char_off=word_char_off, word_char_seq=word_char_seq)
    meta = dict(n_char=n_char, n_word=n_word, glyph_of=glyph_of, pos_of=pos_of, char_glyph=char_glyph)
    return fields, meta


# ---------- LoD tier: the placement -> grouping-detection -> promote-to-tier loop ----------
def promote_tier(labels, min_group=2):
    """Generic loop: given a per-word label (a candidate common element), detect groupings
    of significance >= min_group, pull the common element, promote it to a coarser tier the
    group hangs under. Returns {label: [word_idx,...]} = the promoted tier (CSR-able:
    address a word as  tier_label INDEX-COMPOSED-WITH  member-position).  PoS is the
    no-brainer first promotion (P: PoS = tag it; sub-groupings wait for the map)."""
    tier = {}
    for w, lab in enumerate(labels):
        if lab is None:
            continue
        tier.setdefault(lab, []).append(w)
    return {lab: ws for lab, ws in tier.items() if len(ws) >= min_group}


def homonym_sectors(glyph_of):
    """Cross-cutting weighted-factor tag, NOT an LoD tier: words sharing a spelling are a
    mutual/reciprocal set (they 'list each other' by the shared key) -> one homonym sector.
    Association by shared tag-value, not a hierarchy rung. Loose-until-audio (text alone
    cannot resolve them: same spelling != same sound, and homophones differ in spelling)."""
    by_spelling = {}
    for w, g in enumerate(glyph_of):
        by_spelling.setdefault(g, []).append(w)
    return {g: ws for g, ws in by_spelling.items() if len(ws) >= 2}


# ---------- self-check: lossless round-trip READ BACK OUT OF THE SNODE FIELDS ----------
def selfcheck(fields, meta):
    """Compose word->char->byte purely by index-into-array, reading the SNode contents
    (to_numpy = the actual field storage), reconstruct UTF-8, assert == original word.
    A bug in any offset/composition breaks this. Runs the real slice; exits 1 on mismatch."""
    wc_off = fields["word_char_off"].to_numpy()
    wc_seq = fields["word_char_seq"].to_numpy()
    cb_off = fields["char_byte_off"].to_numpy()
    cb_val = fields["char_byte_val"].to_numpy()
    bn_off = fields["byte_nibble_off"].to_numpy()
    bn_val = fields["byte_nibble_val"].to_numpy()
    nibble_space = fields["nibble_space"].to_numpy()
    glyph_of = meta["glyph_of"]

    bad = []
    for w in range(meta["n_word"]):
        out = bytearray()
        for ci in range(wc_off[w], wc_off[w + 1]):          # compose: word -> char-seq
            seq = int(wc_seq[ci])
            for bi in range(cb_off[seq], cb_off[seq + 1]):  # compose: char -> byte
                b = int(cb_val[bi])                          # byte value / index (0..255)
                nibs = [int(nibble_space[int(bn_val[ni])])   # compose: byte -> nibble leaves
                        for ni in range(bn_off[b], bn_off[b + 1])]
                out.append((nibs[0] << 4) | nibs[1])         # recompose byte from its 2 nibble leaves
        try:
            recon = out.decode("utf-8")
        except UnicodeDecodeError:
            recon = None
        if recon != glyph_of[w]:
            bad.append((glyph_of[w], recon))
    return bad


def export_chains(fields, meta, tier, sectors, out_path, extra_chains=None):
    """Producer contract (seam: field-engine-interface-v0.md). Emit every chain as a tagged
    CSR pair (offsets + members + role) over the flat nibble leaf pool, for Silas's
    chain-agnostic kernel. All static (Silas O2: membership is lexically/relationally fixed;
    dynamic re-root is between-run = re-emit, not a per-tick buffer). -> .npz + JSON manifest."""
    import json
    npz = {}
    manifest = {"leaf_grain": "nibble", "nibble_space": 16, "byte_space": 256,
                "note": "CSR type-tree (dedup'd): members are indices into the child level's "
                        "alphabet, NOT instance ids. Instance->leaf expansion is the kernel's "
                        "seeding step (confirm mapping with producer if instance-level maps needed).",
                "chains": []}

    def add_chain(name, role, parent, child, off, members):
        off = np.asarray(off, dtype=np.int32)
        members = np.asarray(members, dtype=np.int32)
        npz[name + "__off"] = off
        npz[name + "__members"] = members
        manifest["chains"].append(dict(name=name, role=role, parent=parent, child=child,
                                       off_key=name + "__off", members_key=name + "__members",
                                       n_groups=int(len(off) - 1), n_members=int(len(members))))

    # containment (given-rungs, role=containment): word -> char -> byte -> nibble
    add_chain("word_char", "containment", "word", "char",
              fields["word_char_off"].to_numpy(), fields["word_char_seq"].to_numpy())
    add_chain("char_byte", "containment", "char", "byte",
              fields["char_byte_off"].to_numpy(), fields["char_byte_val"].to_numpy())
    add_chain("byte_nibble", "containment", "byte", "nibble",
              fields["byte_nibble_off"].to_numpy(), fields["byte_nibble_val"].to_numpy())

    # cross-cutting (role=cross-cutting, NOT LoD tiers): PoS tier, homonym sector -- CSR over words
    def dict_to_csr(groups):
        keys = sorted(groups)
        off = [0]; members = []
        for k in keys:
            members.extend(groups[k]); off.append(len(members))
        return keys, np.array(off, dtype=np.int32), np.array(members, dtype=np.int32)

    pos_keys, pos_off, pos_members = dict_to_csr(tier)
    add_chain("pos", "cross-cutting", "pos-tier", "word", pos_off, pos_members)
    manifest["chains"][-1]["group_labels"] = [str(k) for k in pos_keys]

    hom_keys, hom_off, hom_members = dict_to_csr(sectors)
    add_chain("homonym", "cross-cutting", "homonym-sector", "word", hom_off, hom_members)
    manifest["chains"][-1]["group_labels"] = [str(k) for k in hom_keys]

    # Chain C+ (generalized, msg-1198 offer): ANY named word-grouping joins as one
    # more tagged-CSR chain over the same leaves -- same schema, no new mechanism.
    for cname, (role, parent, groups) in (extra_chains or {}).items():
        c_keys, c_off, c_members = dict_to_csr(groups)
        add_chain(cname, role, parent, "word", c_off, c_members)
        manifest["chains"][-1]["group_labels"] = [str(k) for k in c_keys]

    npz["nibble_space"] = np.arange(16, dtype=np.int32)
    npz["byte_space"] = np.arange(256, dtype=np.int32)

    np.savez_compressed(out_path, **npz)
    with open(out_path + ".manifest.json", "w") as f:
        json.dump(manifest, f, indent=2)
    return manifest


def show(fields, meta, tier, sectors, sample=("bank", "stale", "cat", "read")):
    wc_off = fields["word_char_off"].to_numpy()
    wc_seq = fields["word_char_seq"].to_numpy()
    cb_off = fields["char_byte_off"].to_numpy()
    cb_val = fields["char_byte_val"].to_numpy()
    glyph_of, char_glyph = meta["glyph_of"], meta["char_glyph"]
    idx_of = {}
    for w, g in enumerate(glyph_of):
        idx_of.setdefault(g, w)
    # prefer the curated words if present; else fall back to real words in the slice
    # (a homonym-sector member + some 3-6 char words) so the proof always renders.
    sample = [w for w in sample if w in idx_of]
    if len(sample) < 3:
        extra = [g for g in sorted(sectors, key=lambda s: -len(sectors[s])) if 3 <= len(g) <= 6]
        extra += [g for g in glyph_of if 3 <= len(g) <= 6]
        for g in extra:
            if g not in sample:
                sample.append(g)
            if len(sample) >= 3:
                break

    print("\n================ PROOF: array-stored compositional address (word -> char -> byte) ================")
    for word in sample:
        if word not in idx_of:
            continue
        w = idx_of[word]
        seqs = [int(wc_seq[ci]) for ci in range(wc_off[w], wc_off[w + 1])]
        glyphs = [char_glyph[s] for s in seqs]
        byte_tuples = [[int(cb_val[bi]) for bi in range(cb_off[s], cb_off[s + 1])] for s in seqs]
        print(f"\n  word '{word}'  (word-node index {w})")
        print(f"    OLD (the mess) : token_id = 'ns.p2.p3.p4.p5'  -- dot-joined TEXT, capped at 5 rungs")
        print(f"    NEW word->char : compose  word_char_seq[word_char_off[{w}] .. ]  = {seqs}   (spelling as index-tuple into the char array; glyphs {glyphs})")
        print(f"    NEW char->byte : compose  char_byte_val[char_byte_off[seq] .. ]  = {byte_tuples}   (UTF-8 bytes as index-tuples into the 256 byte-space)")
        print(f"    depth reached  : {sum(len(bt) for bt in byte_tuples)} bytes over {len(seqs)} chars -- ARBITRARY, not 5; every hop is an index into a contiguous SNode array")

    # PoS LoD tier (promoted no-brainer)
    print("\n================ LoD tier (promoted): part-of-speech ================")
    top = sorted(tier.items(), key=lambda kv: -len(kv[1]))[:8]
    print("  promoted PoS tiers (tier ∘ member addressing):",
          ", ".join(f"{lab}×{len(ws)}" for lab, ws in top))

    # homonym sectors (cross-cutting tag, not a tier)
    print("\n================ Homonym sectors (cross-cutting tag, NOT a tier; loose-until-audio) ================")
    top_sec = sorted(sectors.items(), key=lambda kv: -len(kv[1]))[:8]
    for g, ws in top_sec:
        poss = [meta["pos_of"][x] for x in ws]
        print(f"  sector '{g}'  ×{len(ws)}  pos={poss}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", "--slice", type=int, default=20000, help="word slice size (real rows)")
    ap.add_argument("--export", metavar="PATH", default=None,
                    help="emit producer-contract chain-maps (.npz + .manifest.json) after a PASS")
    ap.add_argument("--demo-chain", metavar="PATH", default=None,
                    help="Chain-C demo: emit anagram-sector chain via the generalized emitter "
                         "(real relation, demo only; never the accepted exchange artifact)")
    a = ap.parse_args()

    char_glyph, char_cp, words = load(a.slice)
    print(f"loaded: {len(char_glyph)} chars, {len(words)} word-entries (slice={a.slice})")
    fields, meta = build(char_glyph, char_cp, words)

    tier = promote_tier(meta["pos_of"])
    sectors = homonym_sectors(meta["glyph_of"])

    bad = selfcheck(fields, meta)
    show(fields, meta, tier, sectors)

    print("\n================ self-check: lossless word->char->byte round-trip through the SNode fields ================")
    if bad:
        print(f"  FAIL: {len(bad)} of {meta['n_word']} words did not reconstruct. examples: {bad[:5]}")
        sys.exit(1)
    print(f"  PASS: all {meta['n_word']} words reconstructed byte-exact by composing indices into the dense SNode arrays.")
    print(f"        PoS tiers promoted: {len(tier)}   homonym sectors: {len(sectors)}")

    if a.export:
        man = export_chains(fields, meta, tier, sectors, a.export)
        print(f"\n================ producer-contract export (chain-maps for the kernel) ================")
        print(f"  wrote {a.export}.npz + {a.export}.manifest.json   (leaf grain: {man['leaf_grain']})")
        for c in man["chains"]:
            print(f"    chain {c['name']:<12} role={c['role']:<13} {c['parent']}->{c['child']:<8} "
                  f"groups={c['n_groups']:<6} members={c['n_members']}")

    if a.demo_chain:
        # anagram sectors: words sharing an identical char-multiset, >=2 DISTINCT
        # spellings (listen/silent) -- a real relation from the data, chosen as the
        # Chain-C demo. NOT a claimed LoD tier; not shipped to the exchange artifact.
        by_multiset = {}
        for w, g in enumerate(meta["glyph_of"]):
            by_multiset.setdefault(tuple(sorted(g)), []).append(w)
        ana = {k: ws for k, ws in by_multiset.items()
               if len({meta["glyph_of"][w] for w in ws}) >= 2}
        man = export_chains(fields, meta, tier, sectors, a.demo_chain,
                            extra_chains={"anagram_demo":
                                          ("cross-cutting", "anagram-sector", ana)})
        z = np.load(a.demo_chain + ".npz")
        off, mem = z["anagram_demo__off"], z["anagram_demo__members"]
        assert off[-1] == len(mem), "CSR invariant broken"
        for gi in (0, (len(off) - 1) // 2, len(off) - 2):
            ws = mem[off[gi]:off[gi + 1]]
            multisets = {tuple(sorted(meta["glyph_of"][w])) for w in ws}
            spellings = {meta["glyph_of"][w] for w in ws}
            assert len(multisets) == 1 and len(spellings) >= 2, (gi, multisets)
        ex = sorted({meta["glyph_of"][w] for w in mem[off[0]:off[1]]})[:6]
        print(f"\n================ Chain-C demo (generalized cross-cutting emitter) ================")
        print(f"  anagram_demo: {len(off)-1} sectors, {len(mem)} members  CSR-invariant OK, "
              f"multiset-membership verified on 3 sectors. sector[0] e.g. {ex}")


if __name__ == "__main__":
    main()
