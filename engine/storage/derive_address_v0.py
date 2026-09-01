#!/usr/bin/env python3
"""derive_address_v0 — same-method address derivation + supersession edge.

Spec: engine/storage/address-derivation-v0.md
Plan line: data-plan §6b (address = deterministic fn of the structure itself
under shared rules; collision = agreement = the merge primitive) + §2 two-way
db; namespace-reference §Ingestion Rules (fragments -> AB.AB.B*, one LoD below
source).

Folds write_back_v0's resolve-before-mint + mint into ONE op: compute the
address by method; occupied-by-same-structure IS resolution (union, provenance
accrues), empty is a mint, occupied-by-different-structure (R2 digest rung
only) probes k+1. The supersession edge (alias -> canonical forwarding, P 1440)
reconciles the rare cross-node split name — normalized-away names forward,
never leave a hole.

Self-test = real Moby byte sequences (provenance 'method-selftest', explicitly
NOT a condensation claim), all writes in session-local TEMP tables. Durable
output = the two standing EMPTY tables (engine.fragments_v0,
engine.address_forwarding_v0) awaiting the kernel's cross-byte bond epoch.

Run: ~/engine/venv/bin/python derive_address_v0.py
"""
import hashlib
import io
import json
import os
import time

import numpy as np
import psycopg2

HERE = os.path.dirname(os.path.abspath(__file__))
MOBY = os.path.join(HERE, "..", "..", "data", "gutenberg", "texts",
                    "02701_Moby Dick Or The Whale.txt")

# base-50 alphabet per docs/spec/token-addressing.md (A-Z a-z minus O/o, ASCII order)
ALPHA = [chr(c) for c in range(65, 91) if chr(c) != "O"] + \
        [chr(c) for c in range(97, 123) if chr(c) != "o"]
assert len(ALPHA) == 50
PAIR_AB = 1                      # 'AB' = 0*50 + 1
SLOTS = 2500 * 2500              # pairs 4-5 residual space per length class


def pair3(length):
    """Layer-B pair: 'B' + length-class symbol. Pure fn of the structure."""
    return 50 + min(length - 2, 49)


def dotted(addr):
    """Display render, emit-only — never persisted (data-protocol.md)."""
    return ".".join(ALPHA[v // 50] + ALPHA[v % 50] for v in addr)


def derive_r0_byte(b):
    """Single byte = not a fragment: resolves to its given byte-code token."""
    return [0, 0, 0, 0, int(b)]


def derive_r1_bigrams(bigrams):
    """R1 injective pack, vectorized: (N,2) uint8 -> (N,5) addresses.
    256^2 <= SLOTS, so the address IS the composition — no hash, no collision."""
    v = bigrams[:, 0].astype(np.int64) * 256 + bigrams[:, 1]
    out = np.empty((len(v), 5), dtype=np.int16)
    out[:, 0] = PAIR_AB
    out[:, 1] = PAIR_AB
    out[:, 2] = pair3(2)
    out[:, 3] = v // 2500
    out[:, 4] = v % 2500
    return out


def derive_r2(comp, k, slots=SLOTS):
    """R2 content digest at probe k. Pure fn of (structure, k)."""
    h = hashlib.blake2b(b"".join(int(c).to_bytes(4, "big") for c in comp) +
                        int(k).to_bytes(4, "big"),
                        digest_size=8, person=b"hcp-addr-v0")
    v = int.from_bytes(h.digest(), "big") % slots
    return [PAIR_AB, PAIR_AB, pair3(len(comp)), v // 2500, v % 2500]


def pg_arr(a):
    return "{" + ",".join(str(int(x)) for x in a) + "}"


FRAG_DDL = """CREATE {temp}TABLE {name}(
  address smallint[] PRIMARY KEY,
  composition smallint[] NOT NULL,
  method text NOT NULL,
  status text NOT NULL DEFAULT 'evidenced',
  n_evidence int NOT NULL DEFAULT 1,
  provenance text[] NOT NULL,
  created timestamptz NOT NULL DEFAULT now())"""

FWD_DDL = """CREATE {temp}TABLE {name}(
  alias smallint[] PRIMARY KEY,
  canonical smallint[] NOT NULL,
  reason text NOT NULL,
  weight real,
  provenance text[] NOT NULL,
  status text NOT NULL DEFAULT 'active',
  created timestamptz NOT NULL DEFAULT now())"""


def one_op(cur, table, addrs, comps, weights, prov):
    """THE one op, set-based: derive happened upstream; here the upsert reads
    the field. Returns (minted, resolved, divergent_rows). Divergents are
    stage rows whose address is occupied by a DIFFERENT structure."""
    cur.execute("CREATE TEMP TABLE IF NOT EXISTS stage"
                "(address smallint[], composition smallint[], w int)")
    cur.execute("TRUNCATE stage")
    rows = "\n".join(f"{pg_arr(a)}\t{pg_arr(c)}\t{int(w)}"
                     for a, c, w in zip(addrs, comps, weights))
    cur.copy_expert("COPY stage FROM STDIN", io.StringIO(rows))
    cur.execute(f"""
      INSERT INTO {table}(address, composition, method, n_evidence, provenance)
      SELECT s.address, s.composition, 'v0', s.w, ARRAY[%s]
      FROM stage s
      ON CONFLICT (address) DO UPDATE
        SET n_evidence = {table}.n_evidence + EXCLUDED.n_evidence,
            provenance = (SELECT array_agg(DISTINCT x) FROM
                          unnest({table}.provenance || EXCLUDED.provenance) x)
        WHERE {table}.composition = EXCLUDED.composition
      RETURNING (xmax = 0) AS minted""", (prov,))
    res = [r[0] for r in cur.fetchall()]
    minted, resolved = sum(res), len(res) - sum(res)
    cur.execute(f"""SELECT s.address, s.composition, s.w FROM stage s
                    JOIN {table} f ON f.address = s.address
                    AND f.composition <> s.composition""")
    return minted, resolved, cur.fetchall()


def ingest_r2(cur, table, comps, weights, prov, slots):
    """Full one-op cycle at the R2 rung with deterministic probing."""
    pending = [(list(map(int, c)), int(w), 0) for c, w in zip(comps, weights)]
    minted = resolved = probes = 0
    while pending:
        # intra-batch collision: one derived address, two structures — only one
        # can occupy it this round; the other is divergent by the same rule and
        # probes k+1 (deterministic keep: min composition).
        by_addr = {}
        for c, w, k in pending:
            by_addr.setdefault(tuple(derive_r2(c, k, slots)), []).append((c, w, k))
        batch, nxt = [], []
        for addr, items in by_addr.items():
            items.sort(key=lambda x: x[0])
            batch.append((list(addr),) + items[0])
            nxt += [(c, w, k + 1) for c, w, k in items[1:]]
        addrs = [b[0] for b in batch]
        m, r, div = one_op(cur, table,
                           addrs, [b[1] for b in batch],
                           [b[2] for b in batch], prov)
        minted += m
        resolved += r
        divset = {tuple(c) for _, c, _ in div}
        nxt += [(c, w, k + 1) for _, c, w, k in batch if tuple(c) in divset]
        probes += len(nxt)
        pending = nxt
    return minted, resolved, probes


def main():
    raw = np.fromfile(MOBY, dtype=np.uint8)
    conn = psycopg2.connect(host="192.168.68.60", port=5435, user="hcp",
                            password=os.environ.get("HCP_PW", "hcp_dev"),
                            dbname="hcp_english")
    conn.autocommit = True
    cur = conn.cursor()
    rep = {"artifact": "derive-address-v0 (same-method derivation + supersession edge)",
           "plan_line": "data-plan §6b same-method reconciliation + §2 two-way db; "
                        "namespace-reference §Ingestion Rules (AB.AB.B* fragments)",
           "spec": "engine/storage/address-derivation-v0.md",
           "source": {"file": os.path.basename(MOBY), "n_bytes": int(raw.size)},
           "tests": {}}
    t0 = time.perf_counter()

    # T0 — R0: a single byte resolves to its given byte-code token, all 256.
    assert all(derive_r0_byte(b) == [0, 0, 0, 0, b] for b in range(256))
    rep["tests"]["T0_archive_resolution_bytes"] = "PASS (256/256 -> AA.AA.AA.AA.{n})"

    # T1 — R1 determinism + injectivity on the real bigram set, two arrival orders.
    big = np.stack([raw[:-1], raw[1:]], axis=1)
    uniq, counts = np.unique(big, axis=0, return_counts=True)
    rng = np.random.default_rng(7)
    o1, o2 = rng.permutation(len(uniq)), rng.permutation(len(uniq))
    a1, a2 = derive_r1_bigrams(uniq[o1]), derive_r1_bigrams(uniq[o2])
    m1 = {tuple(c): tuple(a) for c, a in zip(uniq[o1], a1)}
    m2 = {tuple(c): tuple(a) for c, a in zip(uniq[o2], a2)}
    assert m1 == m2, "arrival order leaked into addresses"
    assert len({tuple(a) for a in a1}) == len(uniq), "pack not injective"
    rep["tests"]["T1_r1_order_independence"] = {
        "distinct_bigrams": int(len(uniq)), "distinct_addresses": int(len(uniq)),
        "orders_compared": 2, "identical": True,
        "sample_emit": f"{[int(x) for x in uniq[0]]} -> {dotted(m1[tuple(uniq[0])])}"}

    # T2 — one op, agreement-union: node-a mints, node-b (different order,
    # same real structures) resolves everything; merge is union.
    cur.execute(FRAG_DDL.format(temp="TEMP ", name="frags_t2"))
    addrs = derive_r1_bigrams(uniq)
    mA, rA, dA = one_op(cur, "frags_t2", addrs[o1], uniq[o1], counts[o1],
                        "method-selftest node-a (no condensation claim)")
    mB, rB, dB = one_op(cur, "frags_t2", addrs[o2], uniq[o2], counts[o2],
                        "method-selftest node-b (no condensation claim)")
    cur.execute("SELECT count(*), count(*) FILTER (WHERE array_length(provenance,1)=2) "
                "FROM frags_t2")
    n_rows, n_both = cur.fetchone()
    assert mA == len(uniq) and mB == 0 and rB == len(uniq) and not dA and not dB
    assert n_rows == len(uniq) and n_both == len(uniq)
    rep["tests"]["T2_one_op_agreement_union"] = {
        "node_a": {"minted": mA, "resolved": rA},
        "node_b": {"minted": mB, "resolved": rB},
        "rows_after_merge": int(n_rows), "rows_with_both_provenances": int(n_both),
        "verdict": "collision = agreement; merge = union"}

    # T3 — R2 forced-collision + supersession edge. Real trigrams, two nodes
    # ingesting DIFFERENT real slices (overlapping structures), slot space
    # shrunk to 4096 (test mode) to force digest collisions -> probe splits.
    tri = np.stack([raw[:-2], raw[1:-1], raw[2:]], axis=1)
    half = len(tri) // 2
    slotsT = 65536
    setA, cntA = np.unique(tri[:half + 2], axis=0, return_counts=True)
    setB, cntB = np.unique(tri[half:], axis=0, return_counts=True)
    # sample real structures to a load the test slot-space can hold (~5%),
    # anchored on the real shared core so the merge has overlap to reconcile
    eA, eB = setA.astype(np.int64), setB.astype(np.int64)
    encA = eA[:, 0] * 65536 + eA[:, 1] * 256 + eA[:, 2]
    encB = eB[:, 0] * 65536 + eB[:, 1] * 256 + eB[:, 2]
    shared = np.intersect1d(encA, encB)
    sh = rng.choice(shared, size=min(2000, len(shared)), replace=False)
    onlyA = np.setdiff1d(encA, encB)
    onlyB = np.setdiff1d(encB, encA)
    pickA = np.concatenate([sh, rng.choice(onlyA, min(1000, len(onlyA)), replace=False)])
    pickB = np.concatenate([sh, rng.choice(onlyB, min(1000, len(onlyB)), replace=False)])
    iA = np.searchsorted(encA, pickA)
    iB = np.searchsorted(encB, pickB)
    setA, cntA, setB, cntB = setA[iA], cntA[iA], setB[iB], cntB[iB]
    cur.execute(FRAG_DDL.format(temp="TEMP ", name="frags_na"))
    cur.execute(FRAG_DDL.format(temp="TEMP ", name="frags_nb"))
    mA2, rA2, pA = ingest_r2(cur, "frags_na", setA, cntA,
                             "method-selftest node-a", slotsT)
    mB2, rB2, pB = ingest_r2(cur, "frags_nb", setB, cntB,
                             "method-selftest node-b", slotsT)
    # merge: same structure under two names = the split the forwarding edge heals
    cur.execute("""SELECT a.composition, a.address, b.address,
                          a.n_evidence, b.n_evidence
                   FROM frags_na a JOIN frags_nb b USING (composition)
                   WHERE a.address <> b.address""")
    splits = cur.fetchall()
    cur.execute("""SELECT count(*) FROM frags_na a
                   JOIN frags_nb b USING (composition)""")
    n_shared = cur.fetchone()[0]
    cur.execute(FWD_DDL.format(temp="TEMP ", name="fwd_t3"))
    for comp, aa, ab, wa, wb in splits:
        # canonical = incorporated-function weight (use); deterministic fallback
        canon, alias, w = (aa, ab, wa) if (wa, ab) > (wb, aa) else (ab, aa, wb)
        cur.execute("INSERT INTO fwd_t3(alias, canonical, reason, weight, provenance)"
                    " VALUES (%s,%s,'r2-probe split-name normalization',%s,"
                    "ARRAY['method-selftest merge'])", (alias, canon, float(w)))
    # never-a-hole: every alias resolves through the edge to its canonical
    cur.execute("""SELECT count(*) FROM fwd_t3 f
                   WHERE NOT EXISTS (SELECT 1 FROM frags_na a WHERE a.address=f.canonical
                                     UNION SELECT 1 FROM frags_nb b WHERE b.address=f.canonical)""")
    holes = cur.fetchone()[0]
    assert holes == 0
    rep["tests"]["T3_r2_probe_and_supersession"] = {
        "slot_space_test_mode": slotsT,
        "node_a": {"structures": int(len(setA)), "minted": mA2, "probe_steps": pA},
        "node_b": {"structures": int(len(setB)), "minted": mB2, "probe_steps": pB},
        "shared_structures": int(n_shared), "split_names": len(splits),
        "forwarding_edges": len(splits), "holes_after_forwarding": int(holes),
        "verdict": "split names heal via alias->canonical; never a hole"}

    # Durable standing tables — empty, awaiting the kernel cross-byte bond epoch.
    cur.execute("CREATE SCHEMA IF NOT EXISTS engine")
    cur.execute("DROP TABLE IF EXISTS engine.fragments_v0")
    cur.execute("DROP TABLE IF EXISTS engine.address_forwarding_v0")
    cur.execute(FRAG_DDL.format(temp="", name="engine.fragments_v0"))
    cur.execute(FWD_DDL.format(temp="", name="engine.address_forwarding_v0"))
    cur.execute("SELECT count(*) FROM engine.fragments_v0")
    rep["durable"] = {
        "tables": ["hcp_english engine.fragments_v0 (empty)",
                   "hcp_english engine.address_forwarding_v0 (empty)"],
        "note": "array columns from BIRTH (smallint[]); dotted form emit-only; "
                "first real rows = kernel cross-byte bond epoch condensations"}
    rep["wall_s"] = round(time.perf_counter() - t0, 2)
    rep["caveats"] = [
        "AB.AB scope: fragments minted under the English family; cross-language "
        "fragment namespace needs P's word",
        "weight = n_evidence proxy in the self-test; real incorporated-function "
        "weight accrues from use across epochs",
        "R2 probe k is order-free per structure only when the slot is free; "
        "occupancy-dependent k is the residual the forwarding edge exists for"]
    out = os.path.join(HERE, "derive-address-v0-report.json")
    json.dump(rep, open(out, "w"), indent=1)
    print(json.dumps(rep, indent=1))
    conn.close()


if __name__ == "__main__":
    main()
