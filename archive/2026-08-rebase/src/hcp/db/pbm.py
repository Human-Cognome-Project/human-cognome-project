"""Generic PBM (Pair-Bond Map) build, store, read routines.

A PBM records forward pair-bonds (FPBs) and their recurrence counts (FBR).
Reconstruction uses molecular dynamics / energy minimization, not position data.
"""

from collections import Counter

from .postgres import insert_pbm_entry, insert_scope, get_pbm


def build_pbm(scope_id, token_sequence):
    """Build PBM data from an ordered list of token IDs.

    Returns a dict with:
        - entries: list of (token0, token1, fbr) tuples
        - tbd_tokens: set of TBD placeholder IDs (if any unknowns)
        - first_fpb: the first pair in the sequence (crystallization seed)
    """
    pair_counts = Counter()
    tbd_tokens = set()
    first_fpb = None

    for i in range(len(token_sequence) - 1):
        t0 = token_sequence[i]
        t1 = token_sequence[i + 1]

        if first_fpb is None:
            first_fpb = (t0, t1)

        # Track TBD tokens
        for t in (t0, t1):
            if t is not None and t.startswith("TBD"):
                tbd_tokens.add(t)

        pair_counts[(t0, t1)] += 1

    entries = [(t0, t1, fbr) for (t0, t1), fbr in pair_counts.items()]

    return {
        "entries": entries,
        "tbd_tokens": tbd_tokens,
        "first_fpb": first_fpb,
    }


def store_pbm(conn, scope_id, pbm_data, scope_name=None, scope_type="source_pbm",
              parent_id=None, metadata=None):
    """Write PBM entries to the database.

    If scope_name is provided, registers the scope first.
    Adds first_fpb and tbd_tokens to metadata automatically.
    """
    meta = dict(metadata or {})
    if pbm_data["tbd_tokens"]:
        meta["tbd_tokens"] = list(pbm_data["tbd_tokens"])
    if pbm_data["first_fpb"]:
        meta["first_fpb"] = list(pbm_data["first_fpb"])

    with conn.cursor() as cur:
        if scope_name:
            insert_scope(cur, scope_id, scope_name, scope_type,
                         parent_id=parent_id, metadata=meta if meta else None)

        for t0, t1, fbr in pbm_data["entries"]:
            insert_pbm_entry(cur, scope_id, t0, t1, fbr=fbr)

    conn.commit()


def read_pbm(conn, scope_id):
    """Retrieve a PBM's entries for a scope.

    Returns list of (token0_id, token1_id, fbr) tuples ordered by FBR desc.
    """
    return get_pbm(conn, scope_id)
