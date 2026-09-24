#!/usr/bin/env python3
"""Public field-engine entry point.

Architecture:
    Taichi runtime
        ↓
    physics engine
        ↑
    engine harness
        ↑
    future analyst functions

The implementation is split across field_engine_physics.py (physics/internal
engine state), field_engine_harness.py (control surface/lifecycle), and
field_engine_validation.py (regression/equivalence checks).

This module remains the stable CLI/import facade and re-exports the names used by
existing review tooling.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from field_engine_physics import (  # noqa: E402,F401
    NV,
    Pool,
    Oracle,
    cells_of,
    occupied_cells,
    nbr,
    relax_channels,
    make_kernel,
)
from field_engine_harness import resolve, mint_readout, run  # noqa: E402,F401
from field_engine_validation import pdiff, check  # noqa: E402,F401

__all__ = [
    "NV", "Pool", "Oracle", "cells_of", "occupied_cells", "nbr",
    "relax_channels", "make_kernel", "resolve", "mint_readout", "run",
    "pdiff", "check", "main",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ticks", type=int, default=300)
    ap.add_argument("--grid", type=int, default=48, help="v0 discretisation")
    ap.add_argument("--relax-iters", type=int, default=40, help="v0 discretisation")
    ap.add_argument("--step", type=float, default=0.5, help="v0 discretisation")
    ap.add_argument("--box", type=float, default=100.0, help="v0 discretisation")
    ap.add_argument("--chunk", type=int, default=20)
    ap.add_argument("--grain", default=None,
                    help="endpoint|resolution; default = read from the declarations")
    ap.add_argument("--arch", default="cuda", help="cuda | cpu (taichi) | numpy (oracle)")
    ap.add_argument("--no-identity-pull", action="store_true")
    ap.add_argument("--no-exclusion", action="store_true")
    ap.add_argument("--no-absorb", action="store_true")
    ap.add_argument("--no-cap", action="store_true", help="drop the one-cell-per-tick bound (regression only)")
    ap.add_argument("--periodic", action="store_true",
                    help="v0's periodic demeaned box (regression only); default is OPEN space")
    ap.add_argument("--until-still", type=int, default=0,
                    help="stop after this many consecutive ticks with no condensation (0 = fixed ticks)")
    ap.add_argument("--state", default=os.path.join(HERE, "field-engine-state.npz"))
    ap.add_argument("--report", default=os.path.join(HERE, "field-engine-report.json"))
    ap.add_argument("--mint", default=os.path.join(HERE, "field-engine-mint.json"))
    ap.add_argument("--fresh", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--check-grid", type=int, default=32)
    ap.add_argument("--check-ticks", type=int, default=4)
    a = ap.parse_args()
    if a.check:
        return check(a)
    if a.fresh and os.path.exists(a.state):
        os.remove(a.state)
    return run(a)


if __name__ == "__main__":
    sys.exit(main())
