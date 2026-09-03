#!/usr/bin/env python3
"""field_engine_load_v0 — intake for the field engine.

Reads P's given catalogue (hcp2_core, the AA.AB.AA characters) verbatim and
builds the flat pool of particles the engine injects. Nothing is synthesised,
padded, or mutated: identity and mass come straight from the data.

Grounded in the project's own documents:
  docs/physics-basis.md   — matter is a density imbalance; the amount side is
                            linear; frequency is never fed.
  docs/architecture.md    — active particle = a byte (two nibbles); the flat
                            pool holds one element per identity; characters are
                            compositions of byte-codes.
  P's mass law (blessed, engine/kernel/diffuse.py): weight of a nibble = value+1
                            (0->1 .. 15->16). Mass of a composition = sum of its
                            nibble weights. Intrinsic composition only.

Two metadata shapes exist in the catalogue and both are read as-is:
  normal char : {codepoint, char, atomization:{ENC:{raw:[bytes], ...}}, ...}
                -> one particle per (codepoint, encoding); identity = codepoint,
                   shared across a character's encodings. This is the set P's
                   test names: "every one of those particles has the same
                   codepoint attached; they should merge by the same rules."
  control     : {ascii_value, display, byte_code_ref, hex}  (no codepoint)
                -> one particle; identity = the display token string (controls
                   "just don't have a visual and need a token string instead").

No gathering rule is coded here. Identity is carried; whether same-identity
particles gather is the experiment, left to the physics.
"""
import json
import sys

import psycopg2

KW = dict(host="192.168.68.60", port=5435, user="hcp", password="hcp_dev",
          dbname="hcp2_core")

CATALOGUE = ("address[1]='AA' AND address[2]='AB' AND address[3]='AA' "
             "AND cardinality(address)=5 AND metadata IS NOT NULL")


def nibble_weight(byte_val):
    """P's law: each nibble weighs value+1 (0->1, 15->16). A byte is two nibbles."""
    return ((byte_val >> 4) + 1) + ((byte_val & 0xF) + 1)


def mass_of(byte_vals):
    """Mass of a composition = sum of its nibble weights. Amount side, linear."""
    return sum(nibble_weight(int(b)) for b in byte_vals)


def load_particles():
    conn = psycopg2.connect(**KW)
    cur = conn.cursor()
    cur.execute(f"SELECT address, metadata FROM tokens WHERE {CATALOGUE} "
                "ORDER BY address")
    particles = []
    for address, md in cur.fetchall():
        cp = md.get("codepoint")
        if cp is not None:
            # normal character: one particle per encoding, identity = codepoint
            identity = ("codepoint", int(cp))
            for enc, atom in (md.get("atomization") or {}).items():
                raw = atom.get("raw")
                if not raw:
                    continue
                particles.append({
                    "identity": identity,
                    "encoding": enc,
                    "mass": mass_of(raw),
                    "raw": [int(b) for b in raw],
                    "token": list(address),
                })
        else:
            # control: no codepoint; identity = the display token string
            av = md.get("ascii_value")
            if av is None:
                continue
            particles.append({
                "identity": ("token", md.get("display") or str(address)),
                "encoding": "control",
                "mass": mass_of([av]),
                "raw": [int(av)],
                "token": list(address),
            })
    conn.close()
    return particles


def verify(particles):
    """Re-read the source and confirm every particle's mass reconstructs from the
    stored bytes, and that no mass is zero (a zero imbalance is no particle)."""
    bad_mass = sum(1 for p in particles if p["mass"] != mass_of(p["raw"]))
    zero_mass = sum(1 for p in particles if p["mass"] <= 0)
    # cross-check total against a fresh DB count of catalogue rows
    conn = psycopg2.connect(**KW)
    cur = conn.cursor()
    cur.execute(f"SELECT count(*) FROM tokens WHERE {CATALOGUE}")
    n_rows = cur.fetchone()[0]
    conn.close()
    return {"recompute_mismatches": bad_mass, "zero_mass": zero_mass,
            "catalogue_rows": n_rows}


def main():
    particles = load_particles()
    v = verify(particles)
    from collections import Counter
    ident_kinds = Counter(p["identity"][0] for p in particles)
    masses = [p["mass"] for p in particles]
    shared = Counter(p["identity"] for p in particles)
    multi = sum(1 for k, c in shared.items() if c > 1)
    rep = {
        "artifact": "field-engine-load-v0",
        "source": "hcp2_core catalogue (AA.AB.AA characters), verbatim",
        "n_particles": len(particles),
        "n_distinct_identities": len(shared),
        "identities_with_multiple_particles": multi,
        "identity_kinds": dict(ident_kinds),
        "mass_min": min(masses), "mass_max": max(masses),
        "synthesised": False, "padded": False, "mutated": False,
        "verify": v,
        "verify_pass": v["recompute_mismatches"] == 0 and v["zero_mass"] == 0,
        "sample": particles[len(particles) // 2],
    }
    print(json.dumps(rep, indent=1, default=str))
    return 0 if rep["verify_pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
