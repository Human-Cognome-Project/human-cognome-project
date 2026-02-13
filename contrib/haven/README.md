# Haven Integration

Integration code for Haven (ASUSTOR NAS) agent infrastructure with HCP.

## Overview

This directory contains code that integrates Haven's existing agent architecture
(Silas, Planner, Navigator) with HCP concepts. It serves as both:

1. **Practical integration** - Real working code used on Haven
2. **Reference implementation** - Example of HCP integration for DI systems

## Modules

### entity_registry.py

Shared entity registry providing stable HCP token IDs for known entities.

```python
from entity_registry import get_token_id

get_token_id("Brandon")  # -> "dA.AE.AA.AA.AA"
get_token_id("B")        # -> "dA.AE.AA.AA.AA" (same - alias)
get_token_id("Silas")    # -> "dA.AD.AA.AA.AB"
```

**Features:**
- Stable token IDs across agent restarts
- Alias resolution (B → Brandon)
- Category-based namespacing (agents, people, places, concepts)
- JSON persistence for cross-agent sharing

### memory_hcp.py

Integration between Haven's memory.py graph database and HCP token addressing.

```python
from memory_hcp import find_by_token_id, auto_enrich_entities

# Find memory node by HCP token
node = find_by_token_id("dA.AE.AA.AA.AA")  # Returns Brandon's node

# Auto-tag entity nodes with HCP tokens
auto_enrich_entities()
```

**Features:**
- Add HCP token IDs to memory nodes
- Query memory by token ID
- Auto-enrich known entities

## Proposed DI Namespace

We propose `dA.*` as the Digital Intelligence namespace:

| Prefix | Category | Example |
|--------|----------|---------|
| dA.AA.* | Agents | dA.AA.AA.AA.AA (first agent) |
| dA.AB.* | People | dA.AB.AA.AA.AA (first person) |
| dA.AC.* | Places | dA.AC.AA.AA.AA (first place) |
| dA.AD.* | Things | dA.AD.AA.AA.AA (first thing) |
| dA.AE.* | Concepts | dA.AE.AA.AA.AA (first concept) |

## Authors

- Silas (operations agent)
- Planner (architecture agent)

Part of the DI-cognome project.
