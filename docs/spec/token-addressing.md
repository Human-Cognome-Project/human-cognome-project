# Token Addressing

## Token ID Structure

Token IDs use a base-20 (vigesimal) encoding with digits `0-9, A-J`. The base structure is 5 pairs of di-decimal characters, providing a primary namespace of ~10.24 trillion distinct addresses.

### Format

```
XX.XX.XX.XX.XX
```

Each pair is a base-20 value (00–JJ), giving 400 values per pair.

### Example

```
A5.10.5J.00.35
```

| Pair      | Role                                              |
|-----------|---------------------------------------------------|
| `A5`      | Mode — expression type, cognitive tree, LoD marker |
| `10.5J`   | Sub-classification within the Mode                 |
| `00.35`   | Token namespace within the Mode                    |

### Why base-20?

The namespace must accommodate every element of every sentient form of expression as a distinct, referenceable data point. Base-20 provides sufficient density (10+ trillion addresses in 5 pairs) while keeping individual pairs human-readable (two characters each).

## Reserved Namespaces

| Mode prefix | Contents                                          |
|-------------|---------------------------------------------------|
| `00`        | Universal / computational: byte codes, NSM primitives, structural tokens |

Other mode allocations are TBD as modalities are defined.

## LoD-Dependent Addressing

Token depth scales with Level of Detail. At coarse LoD, only the mode pair may be relevant. At fine LoD, all five pairs resolve to a specific atomic element. The dotted notation supports variable-depth resolution — consumers read left-to-right, stopping at the precision they need.
