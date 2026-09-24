import pytest

import taichi as ti
from tests import test_utils


@pytest.mark.parametrize("option", ["llvm_snode_capacity", "llvm_snode_tree_capacity"])
@pytest.mark.parametrize("value", [0, -1])
def test_invalid_llvm_capacity(option, value):
    with pytest.raises(RuntimeError, match="capacities must be positive"):
        ti.init(arch=ti.cpu, **{option: value})
    ti.reset()


@test_utils.test(arch=[ti.cpu, ti.cuda], llvm_snode_capacity=2048)
def test_sparse_tree_above_old_snode_capacity():
    builder = ti.FieldsBuilder()
    fields = [ti.field(ti.i32) for _ in range(1025)]
    builder.pointer(ti.i, 2).place(*fields)
    builder.finalize()
    assert fields[-1].snode._id >= 1024

    @ti.kernel
    def exercise() -> ti.i32:
        fields[0][0] = 7
        fields[1024][1] = 11
        result = 0
        for i in fields[1024]:
            result += fields[0][i] + fields[1024][i]
        return result

    assert exercise() == 18


@test_utils.test(arch=[ti.cpu, ti.cuda], llvm_snode_capacity=12)
def test_global_snode_capacity_rejected_before_runtime_access():
    # Each individual tree is small, but IDs accumulate across trees.
    with pytest.raises(RuntimeError, match="exceeds llvm_snode_capacity"):
        for _ in range(12):
            builder = ti.FieldsBuilder()
            field = ti.field(ti.i32)
            builder.pointer(ti.i, 2).place(field)
            builder.finalize()


@test_utils.test(arch=[ti.cpu, ti.cuda], llvm_snode_tree_capacity=2)
def test_tree_capacity_rejected_before_runtime_access():
    with pytest.raises(RuntimeError, match="exceeds llvm_snode_tree_capacity"):
        for _ in range(3):
            builder = ti.FieldsBuilder()
            builder.dense(ti.i, 1).place(ti.field(ti.i32))
            builder.finalize()


@test_utils.test(arch=[ti.cpu, ti.cuda])
def test_interleaved_sparse_snode_ids():
    first, second = ti.FieldsBuilder(), ti.FieldsBuilder()
    x, y = ti.field(ti.i32), ti.field(ti.i32)
    xp = first.pointer(ti.i, 2).dense(ti.i, 2)
    yp = second.pointer(ti.i, 2).dense(ti.i, 2)
    xp.place(x)
    yp.place(y)
    first.finalize()
    second.finalize()

    @ti.kernel
    def exercise() -> ti.i32:
        x[1] = 3
        y[2] = 5
        result = 0
        for i in x:
            result += x[i]
        for i in y:
            result += y[i]
        return result

    assert exercise() == 8


@test_utils.test(arch=[ti.cpu, ti.cuda], llvm_snode_capacity=2048, llvm_snode_tree_capacity=520)
def test_more_than_512_live_trees():
    trees = []
    fields = []
    for _ in range(513):
        builder = ti.FieldsBuilder()
        field = ti.field(ti.i32)
        builder.place(field)
        trees.append(builder.finalize())
        fields.append(field)
    fields[-1][None] = 37
    assert fields[-1][None] == 37
    for tree in trees:
        tree.destroy()


@test_utils.test(arch=[ti.cpu, ti.cuda], llvm_snode_capacity=128, llvm_snode_tree_capacity=2)
def test_tree_slots_reused_after_destroy():
    # SNode IDs remain monotonic; tree slots are reusable.
    for i in range(16):
        builder = ti.FieldsBuilder()
        field = ti.field(ti.i32)
        builder.place(field)
        tree = builder.finalize()
        field[None] = i
        assert field[None] == i
        tree.destroy()


def test_install_environment_capacities(monkeypatch):
    monkeypatch.setenv("TI_LLVM_SNODE_CAPACITY", "4096")
    monkeypatch.setenv("TI_LLVM_SNODE_TREE_CAPACITY", "768")
    try:
        ti.init(arch=ti.cpu)
        assert ti.lang.impl.current_cfg().llvm_snode_capacity == 4096
        assert ti.lang.impl.current_cfg().llvm_snode_tree_capacity == 768
    finally:
        ti.reset()
