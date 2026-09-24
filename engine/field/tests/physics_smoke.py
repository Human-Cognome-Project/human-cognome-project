#!/usr/bin/env python3
"""No-DB smoke test for the active field-physics boundary."""
import pathlib
import sys

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
FIELD = HERE.parent
sys.path.insert(0, str(FIELD))

from field_engine_physics import Oracle, Pool  # noqa: E402


def main():
    particles = [{"config": [0]}, {"config": [0]}]
    pool = Pool(particles)
    pos = np.array([[1.25, 1.25, 1.25],
                    [1.25, 1.25, 1.25]], dtype=np.float64)

    engine = Oracle(
        pool,
        grid=4,
        box=4.0,
        relax_iters=1,
        step=0.0,
        pull=True,
        excl=False,
        absorb=True,
        periodic=False,
        cap=True,
    )
    out, condensed = engine.tick(pos)

    assert condensed == 1
    assert int(pool.alive.sum()) == 1
    assert int(pool.count.sum()) == 2
    assert pool.parent[1] == 0
    assert np.array_equal(out, pos)

    print("PASS field_engine_physics_smoke")


if __name__ == "__main__":
    main()
