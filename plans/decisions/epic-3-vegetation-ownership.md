# Decision: Vegetation Placement Ownership Split

**Date:** 2026-01-28
**Status:** accepted
**Epic:** 3
**Ticket:** 3-029
**Triggered By:** Canon verification conflict C-001

## Context

Epic 3 ticket 3-029 assigns per-tile vegetation visual instance placement (position jitter, rotation, scale) to RenderingSystem. However, `systems.yaml` states TerrainSystem owns "Vegetation placement." This creates a system boundary ambiguity.

## Canon Conflict

**Current Canon:** `systems.yaml` — TerrainSystem owns "Vegetation placement" (unqualified).
**Proposed Change:** Split ownership between tile-level designation (TerrainSystem) and visual instance generation (RenderingSystem).

## Options

### A) TerrainSystem Owns All Vegetation Placement
- TerrainSystem generates per-tile instance positions (deterministic from tile coords)
- RenderingSystem receives pre-computed instance data
- Cons: Increases network sync cost (per-tree positions), violates "TerrainSystem does not own rendering"

### B) Split Ownership (Recommended)
- TerrainSystem owns tile-level vegetation designation (which tiles are BiolumeGrove/PrismaFields/SporeFlats)
- RenderingSystem generates per-tile visual instances deterministically from tile coordinate hash
- Pros: No per-tree sync needed; consistent with "TerrainSystem does not own rendering"
- Cons: Canon requires clarification

### C) Keep Ambiguous
- Leave systems.yaml as-is, document intent in ticket only
- Cons: Future developers may misinterpret boundary

## Decision

**Option B: Split ownership with canon clarification.**

All three agents converged on this architecture during planning:
- Systems Architect: "RenderingSystem reads data and builds GPU resources"
- Graphics Engineer: "RenderingSystem deterministically generates instance placement using seeded PRNG"
- Game Designer: No objection — visual placement is not a gameplay concern

## Consequences

- Canon files to update: `systems.yaml` — clarify TerrainSystem "Vegetation placement" to "Vegetation tile designation (which tiles have vegetation type)"
- Add to RenderingSystem or a terrain render subsystem note: "Vegetation visual instance generation (per-tile position, rotation, scale from coordinate hash)"
- No ticket changes needed — ticket 3-029 already implements Option B
- No other epics affected
