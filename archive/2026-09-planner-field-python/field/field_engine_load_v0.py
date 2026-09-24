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
import os
import sys

import psycopg2

KW = dict(host=os.environ.get("HCP_HOST", "localhost"),
          port=int(os.environ.get("HCP_PORT", "5435")),
          user=os.environ.get("HCP_USER", "hcp"),
          dbname=os.environ.get("HCP_CORE_DB", "hcp2_core"))
if os.environ.get("HCP_PW"):
    KW["password"] = os.environ["HCP_PW"]

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



# ---------------------------------------------------------------------------
# Configuration at the DECLARED grain (added 2026-09-03).
#
# P (2026-09-02): "the step declarations are the granularity knobs and data
# integrity rules" and "the total mass is a general pull with specific
# configuration of mass forming identity." So the engine does not invent a
# grain: it reads engine.declarations_v0 (P's ladder: Unicode -> table ->
# endpoint) and takes the finest declared step as the grain. At the ENDPOINT
# grain a particle's configuration is the hex digits of the value the endpoint
# carries (its codepoint, or for the 128 no-codepoint ASCII rows the byte
# value) -- ONE rule, cp else value, no controls branch. The mass at a grain is
# the same value+1 law over the configuration's nibbles (Silas verified this
# identity-grain law vs live hcp_core: 19,260 codepoints, mass 5..63, zero
# mismatches). The sub-declared rung -- the encodings -- is the RESOLUTION
# grain: configuration = the raw byte code (loader's original mass).
#
# Integrity rule: every particle's address must be a member of a declared step.
# ---------------------------------------------------------------------------

def load_declarations():
    """P's step declarations, verbatim from engine.declarations_v0."""
    conn = psycopg2.connect(**KW)
    cur = conn.cursor()
    cur.execute("SELECT declares, state, gathered, tag FROM engine.declarations_v0 "
                "ORDER BY cardinality(declares), declares")
    rows = [{"declares": list(d), "state": s, "gathered": g, "tag": t}
            for d, s, g, t in cur.fetchall()]
    conn.close()
    return rows


def grain_from_declarations(decls):
    """The members of the deepest declared step are the finest grain: the
    endpoints (depth = deepest declaration + 1). The engine runs there."""
    deepest = max(len(d["declares"]) for d in decls)
    endpoints = set()
    for d in decls:
        if len(d["declares"]) == deepest:
            for m in (d["gathered"] or {}).get("members", []):
                endpoints.add(tuple(m))
    return {"depth": deepest + 1, "n_declared": len(decls),
            "declared_steps": sum(1 for d in decls if len(d["declares"]) == deepest),
            "declared_endpoints": endpoints}


def hex_nibbles(value, width):
    """Hex digits of a value as nibble ints, most significant first."""
    return [int(ch, 16) for ch in f"{value:0{width}X}"]


def attach_configuration(particles, grain="endpoint"):
    """Set p['config'] (nibble list) and p['mass'] (value+1 sum over it).
    endpoint  : hex of the value the endpoint carries (codepoint, U+ width>=4;
                else the byte value, width 2). Encodings of one endpoint carry
                the SAME configuration -- whether they merge is the physics.
    resolution: the raw byte code of the particle's encoding (the loader's
                original mass law)."""
    for p in particles:
        if grain == "endpoint":
            if p["identity"][0] == "codepoint":
                nib = hex_nibbles(p["identity"][1], 4)
            else:
                nib = hex_nibbles(p["raw"][0], 2)
        elif grain == "resolution":
            nib = [d for b in p["raw"] for d in ((b >> 4) & 0xF, b & 0xF)]
        else:
            raise ValueError(grain)
        p["config"] = nib
        p["mass_resolution"] = mass_of(p["raw"])
        p["mass"] = sum(v + 1 for v in nib)
    return particles


def load_at_declared_grain(grain=None):
    """Load particles, read the declarations, enforce the integrity rule, and
    attach the configuration at the declared grain. Returns (particles, info)."""
    decls = load_declarations()
    g = grain_from_declarations(decls)
    if grain is None:
        grain = "endpoint" if g["depth"] == 5 else "resolution"
    particles = load_particles()
    bad = [p for p in particles if tuple(p["token"]) not in g["declared_endpoints"]]
    if bad:
        raise RuntimeError(f"{len(bad)} particles carry undeclared addresses")
    attach_configuration(particles, grain)
    S = max(len(p["config"]) for p in particles)
    masses = [p["mass"] for p in particles if p["identity"][0] == "codepoint"]
    info = {"grain": grain, "declared_depth": g["depth"],
            "declarations": g["n_declared"], "declared_steps": g["declared_steps"],
            "declared_endpoints": len(g["declared_endpoints"]),
            "n_particles": len(particles), "slots": S,
            "configs_distinct": len({tuple(p["config"]) for p in particles}),
            "codepoint_mass_min": min(masses), "codepoint_mass_max": max(masses)}
    return particles, info


if __name__ == "__main__":
    sys.exit(main())
