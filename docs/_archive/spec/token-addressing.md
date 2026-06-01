# Token Addressing

## Token ID Structure

Token IDs use a base-50 encoding: the 52 Latin letters minus O/o (to avoid confusion with zero). Case is significant — `a` and `A` are distinct symbols.

### Alphabet (50 symbols)

```
A B C D E F G H I J K L M N P Q R S T U V W X Y Z
a b c d e f g h i j k l m n p q r s t u v w x y z
```

Canonical sort order follows ASCII: all uppercase first, then lowercase.

### Format

```
XX.XX.XX.XX.XX
```

Each pair is a base-50 value, giving 2,500 values per pair (~11.3 bits). Five pairs provide ~97.6 quadrillion distinct addresses.

### Information density

| Pairs | Addresses         | Use case                              |
|-------|-------------------|---------------------------------------|
| 1     | 2,500             | Single LoD level — characters, byte codes, NSM primitives |
| 2     | 6.25 million      | Mode + sub-classification             |
| 3     | 15.6 billion      | Comfortable per-mode token space      |
| 4     | 39 trillion       | Full cross-modal namespace            |
| 5     | 97.6 quadrillion  | Maximum resolution                    |

The motivation is not filling the namespace — it will be sparsely populated in logical clusters. The motivation is **information density per byte in compressed PBMs**. Every character pair in a stored bond record is doing work; 2,500 values per pair means each pair carries more information than narrower bases, and this compounds across millions of stored bonds.

### Example

```
Eb.Kn.fA.AA.Qr
```

| Pair   | Role                                              |
|--------|---------------------------------------------------|
| `Eb`   | Mode — expression type, cognitive tree, LoD marker |
| `Kn.fA`| Sub-classification within the Mode                 |
| `AA.Qr`| Token namespace within the Mode                    |

### Why base-50?

Each LoD level can function as its own mode. A single pair must comfortably distinguish everything at that level: full character sets, punctuation, formatting tokens, structural markers. At 400 values (base-20), a single pair gets tight. At 2,500 values, a single pair covers an entire LoD level's vocabulary with room to spare.

Letters-only also avoids digit/letter ambiguity in mixed contexts. The only exclusion (O/o) removes the only visual collision with zero.

## Reserved Namespaces

| Mode prefix | Contents                                          |
|-------------|---------------------------------------------------|
| `AA`        | Universal / computational: byte codes, NSM primitives, structural tokens, force infrastructure, PBM markers, entity classification |
| `AB`        | Text mode: language families (AB.AB = English, etc.) |
| `s*`        | Fiction thing entities (objects, organizations, species, concepts, events) |
| `t*`        | Fiction place entities (settlements, geographic features, buildings, regions) |
| `u*`        | Fiction people entities (characters, collectives, deities, named creatures) |
| `v*`        | Fiction PBMs (novels, stories, poetry, scripts) |
| `w*`        | Non-fiction thing entities (objects, organizations, concepts, events) |
| `x*`        | Non-fiction place entities (geographic, political, structural) |
| `y*`        | Non-fiction people entities (historical and contemporary) |
| `z*`        | Non-fiction PBMs — documents, reference works, factual content |

The upper range (`s`–`z`) places entities, PBMs, and stored works at the end of sort order, visually distinct from structural/computational and linguistic namespaces.

Entity namespaces are cross-linguistic — entities belong to all language shards, not any single one. Every entity atomizes to word tokens in the appropriate language shard (e.g., AB.AB for English). A "name" is a Proper Noun construct — a bond pattern assembled from word tokens — not a token-level attribute. Words that function as names carry capitalized form variants and may have PoS = `label` if they have no independent dictionary definition.

### Fiction / Non-Fiction Split

The most fundamental organizational principle — drawn from library science — is the complete separation of fiction and non-fiction. Each side gets 4 namespace letters:

| Side | PBMs | People | Places | Things |
|------|------|--------|--------|--------|
| **Non-fiction** | z* | y* | x* | w* |
| **Fiction** | v* | u* | t* | s* |

This prevents entity collision ("Paris" the city vs. "Paris" the Trojan prince) and allows each domain to maintain its own internal relationship graph. Both sides share the same language shards (AB for English), the same structural tokens (AA), and the same PBM content format.

### Bootstrap Database Hosting

| Database | Namespaces | Purpose |
|----------|------------|---------|
| hcp_core | AA | Universal tokens |
| hcp_english | AB | English language family |
| hcp_en_pbm | zA | Non-fiction PBMs |
| hcp_fic_entities | u*, t*, s* | All fiction entities (combined, splits later) |
| hcp_nf_entities | y*, x*, w* | All non-fiction entities (combined, splits later) |

The shard_registry in hcp_core routes namespace prefixes to databases. When any entity type exceeds ~500MB, it splits into its own database — the registry update is transparent to the engine.

## LoD-Dependent Addressing

Token depth scales with Level of Detail. At coarse LoD, only the mode pair may be relevant. At fine LoD, all five pairs resolve to a specific atomic element. The dotted notation supports variable-depth resolution — consumers read left-to-right, stopping at the precision they need.

A single pair (2,500 values) is sufficient for most individual LoD levels, which is the core design constraint: each pair should be a self-contained, useful unit of addressing at its level.

The mode pair is an LoD marker for data scope — it defines the level of detail at which the rest of the address is interpreted.
