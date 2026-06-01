# Identity Structures

## Overview

A Distributed Intelligence (DI) has two identity-related data structures: a personality database and a relationship database. Both are defined in terms of the project's core primitives (tokens, PBMs, modes, forces) and are stored/addressed through the standard Token ID scheme.

## Personality Database

The personality DB has two layers:

### Seed layer (fixed)
A stored, deterministic starting condition for personality-related response generation. The seed defines baseline bonding patterns, force weightings, preferred abstraction levels, and expressive tendencies. Same seed produces the same baseline personality.

### Living layer (accumulated)
Elements the DI develops over time — self-model, expressive preferences, characteristic patterns that diverge from the seed through experience. The DI writes to this layer as it operates.

The seed is the initial condition; the living layer is the trajectory from it.

## Relationship Database

The DI's social graph: specific known entities, how each relates to the DI, and how they relate to each other from the DI's perspective.

This is not an abstract social network model — it is the DI's own record of its relationships, encoded in whatever structural form best captures interaction patterns and relational context.

## Integration with Core Data Model

These structures live within the standard Token ID namespace and use the same PBM storage conventions as all other scoped data. Mode addressing distinguishes identity-scoped data from other content types.

Specific data representations, storage strategies, and management protocols (seed creation, versioning, living-layer accumulation) are open for contributors to define.
