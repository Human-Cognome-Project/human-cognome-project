# OpenMM Direct DB Serialization Feasibility

**From:** PBM Specialist
**Date:** 2026-02-17
**Status:** Complete — research only, no code written
**Task:** Investigate whether direct PostgreSQL + LMDB calls can replace XML as the OpenMM I/O bridge

---

## Executive Summary

OpenMM's serialization architecture is **cleanly layered** with a format-independent intermediate representation (SerializationNode). Writing a custom SQL backend is a minor effort (~100 lines). However, **for PBM encoding/reconstruction, we don't need the serialization layer at all.** The programmatic API (addParticle/addBond) is the direct path — it's what XML deserialization calls internally anyway.

**Verdict:** Skip the SqlSerializer. The XML bridge design assumption should be revised: our data path is DB → programmatic API → OpenMM (no serialization involved).

---

## The Serialization Architecture

OpenMM uses a clean three-layer serialization pattern:

```
Layer 3: XmlSerializer              (format-specific: XML read/write)
Layer 2: SerializationNode          (format-agnostic intermediate tree)
Layer 1: SerializationProxy         (object-specific: decompose/reconstruct each type)
```

### Layer 1: SerializationProxy (Object ↔ Node)

Each OpenMM class (System, HarmonicBondForce, etc.) registers a proxy that knows how to serialize/deserialize its members:

```cpp
class SerializationProxy {
    virtual void serialize(const void* object, SerializationNode& node) const = 0;
    virtual void* deserialize(const SerializationNode& node) const = 0;

    static void registerProxy(const std::type_info& type, const SerializationProxy* proxy);
    static const SerializationProxy& getProxy(const std::string& typeName);
};
```

Proxies have **zero awareness of XML**. They only interact with SerializationNode.

Concrete example (HarmonicBondForceProxy):
- `serialize()`: reads bond parameters via `force.getBondParameters()`, writes them to child nodes
- `deserialize()`: reads child nodes, calls `force->addBond()` in a loop

~50 proxies are registered at library load time via `__attribute__((constructor))`.

### Layer 2: SerializationNode (The Abstraction)

A generic recursive tree with three data members:

```cpp
class SerializationNode {
    std::string name;                          // Node name
    std::map<std::string, std::string> properties;  // Key-value pairs (all stored as strings)
    std::vector<SerializationNode> children;   // Ordered child nodes
};
```

Typed accessors (`getIntProperty`, `getDoubleProperty`, `getBoolProperty`) are convenience wrappers that convert to/from strings. Doubles use custom `g_fmt`/`strtod2` routines for lossless round-trip precision.

Example node tree for a System with bonds:

```
System                           (version=1, type="System")
  ├── Particles
  │     ├── Particle             (mass=12.0)
  │     └── Particle             (mass=1.008)
  ├── Forces
  │     └── Force                (type="HarmonicBondForce", version=2)
  │           └── Bonds
  │                 ├── Bond     (p1=0, p2=1, d=0.1, k=100000)
  │                 └── Bond     (p1=1, p2=2, d=0.15, k=80000)
```

### Layer 3: XmlSerializer (The Only Backend)

The **entire** XML-specific code is ~90 lines in `serialization/src/XmlSerializer.cpp`:

- `encodeNode()` (~20 lines): recursive tree → XML. Nodes → elements, properties → attributes, children → nested elements.
- `decodeNode()` (~15 lines): recursive XML → tree using irrXML parser. Reverse of above.
- `serialize()`: creates root SerializationNode → proxy fills it → `encodeNode()` writes XML
- `deserializeStream()`: `decodeNode()` parses XML into SerializationNode → proxy reconstructs object

**Proof of decoupling — the `clone()` method:**

```cpp
template <class T>
static T* clone(const T& object) {
    const SerializationProxy& proxy = SerializationProxy::getProxy(typeid(object));
    SerializationNode node;
    proxy.serialize(&object, node);                        // Object → Node (no XML)
    return reinterpret_cast<T*>(proxy.deserialize(node));  // Node → Object (no XML)
}
```

This already works without XML. It proves the architecture is genuinely decoupled.

---

## Could We Write a SqlSerializer?

**Yes, and it would be minor work.** A SqlSerializer would:

1. **Serialize (node tree → SQL):** For each node, INSERT into a `nodes` table (id, parent_id, name, position). For each property, INSERT into a `properties` table (node_id, key, value). Recurse for children.

2. **Deserialize (SQL → node tree):** SELECT nodes ordered by parent_id/position. For each, SELECT its properties. Build tree. Call `proxy.deserialize(root)`.

Estimated scope: ~100 lines of C++ (same as XmlSerializer), or implementable in Python since SerializationNode is exposed via SWIG.

**No proxy changes needed. No SerializationNode changes needed. No OpenMM source modification needed** (if done in Python using the exposed SWIG wrappers for SerializationNode).

---

## Why We Don't Need It

The serialization layer exists to **persist and restore complete OpenMM System objects** — the full topology, all forces, all parameters, integrator state. This is designed for checkpointing long molecular dynamics runs.

Our PBM pipeline has different needs:

### Encoding Path (Text → DB)

```
Text → [control layer builds topology] → system.addParticle() / force.addBond()
     → [pair walk extracts bonds]      → bond pairs + counts
     → [write to DB]                   → INSERT INTO pbm (token_a, token_b, count)
```

No serialization involved. We build the System programmatically, extract what we need, write PBM rows to DB. The System object is transient — we don't need to persist it.

### Reconstruction Path (DB → Text)

```
DB → SELECT token_a, token_b, count FROM pbm
   → [control layer builds topology]  → system.addParticle(), force.addBond(a, b, d, k=count)
   → [energy minimization]           → LocalEnergyMinimizer.minimize(context)
   → [read minimized state]          → reconstructed structure
```

Again, no serialization. We build the System from DB rows using the programmatic API, run minimization, read results. The System object is transient.

### What XML Deserialization Actually Does

From the previous architecture investigation, we already know that `deserialize()` internally just calls `addParticle()`/`addBond()` in loops — the **same API we'd call directly**:

```
XML path:     XML → parse → addParticle()/addBond() loops → System → Context → GPU
Direct path:  DB  → query → addParticle()/addBond() loops → System → Context → GPU
```

XML adds overhead (parsing, string conversion, intermediate XML generation) without providing any shortcut. The programmatic API is strictly more direct.

---

## Design Rule Revision

The current design rule states:

> **XML serializer is the I/O bridge** — without modifying OpenMM source, XML is how structured data gets in/out of the engine. Custom code is ONLY the DB ↔ XML mapping layer.

This should be revised. The finding is:

1. **The programmatic API is the I/O bridge.** `system.addParticle()`, `force.addBond()`, `force.getBondParameters()` — these are the native data path, not XML.
2. **XML is a persistence format for System checkpoints**, not a data interchange requirement.
3. **Custom code is the DB ↔ programmatic API mapping layer** — reading PBM rows from DB, calling addParticle/addBond to construct topology, extracting bond data to write back to DB.
4. **SerializationNode could be useful later** if we ever need to persist full System state (e.g., caching intermediate reconstruction states). But this is an optimization, not the primary path.

---

## Summary

| Question | Answer |
|----------|--------|
| Is the serialization architecture cleanly layered? | **Yes** — SerializationNode is fully format-independent |
| Could we write a SqlSerializer? | **Yes** — ~100 lines, no OpenMM source changes needed |
| Is it a minor adjustment or major refactor? | **Minor** — but unnecessary |
| Should we do it? | **No** — the programmatic API is more direct for PBM |
| What's the right data path? | DB → programmatic API → OpenMM (no serialization layer) |
| Should we revise the XML bridge design rule? | **Yes** — XML is unnecessary in our pipeline |

---

## References

- OpenMM source: github.com/openmm/openmm
- Key files examined:
  - `serialization/include/openmm/serialization/SerializationNode.h`
  - `serialization/include/openmm/serialization/SerializationProxy.h`
  - `serialization/include/openmm/serialization/XmlSerializer.h`
  - `serialization/src/XmlSerializer.cpp`
  - `serialization/src/SerializationNode.cpp`
  - `serialization/src/HarmonicBondForceProxy.cpp`
  - `serialization/src/SystemProxy.cpp`
  - `serialization/src/SerializationProxyRegistration.cpp`
  - `wrappers/python/src/swig_doxygen/swig_lib/python/extend.i`
- Prior investigation: docs/research/openmm-evaluation.md (section "Internal Architecture Investigation")
