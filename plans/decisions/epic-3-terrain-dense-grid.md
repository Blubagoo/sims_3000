# Decision: Dense Grid Storage for Terrain Data

**Date:** 2026-01-28
**Status:** accepted
**Epic:** 3
**Ticket:** 3-002
**Triggered By:** Canon verification conflict C-002

## Context

Epic 3 stores terrain data in a dense 2D array (TerrainGrid) rather than as per-entity ECS components. CANON.md Core Principle 2 states "All game state lives in components" (ECS Everywhere). This creates a pattern departure.

## Canon Conflict

**Current Canon:** CANON.md Core Principle 2 — "ECS Everywhere: All game state lives in components. All logic lives in systems."
**Proposed Change:** Document a canonical exception for high-density spatial grid data.

## Options

### A) Per-Entity ECS Storage
- Each tile is an entity with TerrainComponent attached via ECS registry
- Pros: Fully ECS-compliant
- Cons: ~24+ bytes per entity (EntityID + archetype overhead) vs 4 bytes dense; 512x512 = 262K entities polluting registry; cache-unfriendly for spatial traversal; breaks O(1) coordinate lookup

### B) Dense Grid with ECS Wrapper
- Dense array backend exposed through ECS-compatible query interface
- Tiles are "implicit entities" identified by coordinate
- Pros: ECS semantics preserved; dense storage performance
- Cons: Non-standard ECS pattern; adds abstraction layer

### C) Dense Grid as Documented Exception (Recommended)
- TerrainGrid is a dense 2D array managed by TerrainSystem
- TerrainComponent is a 4-byte pure data struct (still follows "components are pure data")
- TerrainSystem contains all logic (still follows "logic in systems")
- Grid coordinates serve as implicit entity IDs (still follows "entities are just IDs")
- Document as canonical exception for spatial grid data
- Pros: Optimal performance; pragmatic; preserves ECS spirit
- Cons: Canon needs exception documentation

## Decision

**Option C: Dense grid storage as a documented canonical exception.**

All three agents independently chose this architecture:
- Systems Architect: "Dense 2D grid array, NOT per-entity ECS storage" with detailed rationale
- Graphics Engineer: Assumed dense grid for mesh generation efficiency
- Game Designer: No opinion on storage (gameplay-neutral)

### Performance Justification

| Metric | Dense Grid | Per-Entity ECS |
|--------|-----------|----------------|
| Memory per tile | 4 bytes | ~24+ bytes |
| 512x512 total | 1 MB | ~6+ MB |
| Coordinate lookup | O(1) array index | O(log n) or hash |
| Cache behavior | Excellent (linear scan) | Poor (scattered) |
| Spatial query | Direct neighbor access | Registry query |

### ECS Spirit Compliance

The dense grid preserves the ECS separation of concerns:
- **Data** (TerrainComponent): Pure 4-byte struct with no logic
- **Logic** (TerrainSystem): Stateless operations on grid data
- **Identity** (grid coordinates): Implicit entity ID = (x, y)

## Consequences

- Canon files to update: Add exception note to CANON.md or patterns.yaml stating that high-density spatial grid data (terrain, potentially pathfinding grids) may use dense array storage rather than per-entity ECS when: (a) data covers every grid cell, (b) spatial locality is critical, (c) per-entity overhead is prohibitive
- Consider renaming TerrainComponent to TerrainTileData to avoid confusion with ECS-registered components (minor issue M-001)
- No ticket changes needed
- Future epics with similar grid data (e.g., contamination grid, land value grid) can reference this exception
