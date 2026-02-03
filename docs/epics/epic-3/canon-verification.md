# Canon Verification: Epic 3 -- Terrain System

**Verifier:** Systems Architect
**Date:** 2026-01-28
**Canon Version:** 0.5.0
**Tickets Verified:** 40

## Summary

- Total tickets: 40
- Canon-compliant: 37
- Conflicts found: 2
- Minor issues: 5

Overall, Epic 3 is well-aligned with canon v0.5.0. The tickets demonstrate thorough consideration of system boundaries, multiplayer constraints, bioluminescent art direction, configurable map sizes, and free camera support. Two conflicts require resolution before implementation. Five minor issues are non-blocking but should be addressed for consistency.

---

## Verification Checklist

### System Boundary Verification

**Source:** `systems.yaml` -- `phase_1.epic_3_terrain.systems.TerrainSystem`

| Boundary | Status | Notes |
|----------|--------|-------|
| Tile data (terrain type, elevation) | PASS | Tickets 3-001, 3-002 define TerrainComponent and TerrainGrid |
| Terrain generation (procedural, scales to map size) | PASS | Tickets 3-007 through 3-013 cover generation, ticket 3-011 covers map size scaling |
| Water body placement | PASS | Ticket 3-009 covers all three water types |
| Vegetation placement | **CONFLICT** | See Conflict C-001 below |
| Terrain modification (clearing, leveling) | PASS | Tickets 3-019 (purge), 3-020 (grade), 3-021 (terraform) |
| Sea level | PASS | Ticket 3-002 includes sea_level field; tickets note sea level is NOT player-adjustable |
| Does NOT own: Terrain rendering | PASS | All rendering tickets (3-023 through 3-035) assigned to RenderingSystem |
| Does NOT own: What can be built where (ZoneSystem validates) | PASS | Ticket 3-014 provides buildability queries but does not validate zone placement |
| Provides: Terrain queries | PASS | Ticket 3-014 ITerrainQueryable with comprehensive query methods |
| Provides: Buildability checks | PASS | Ticket 3-014 includes is_buildable(x, y) |
| Provides: Water proximity checks | PASS | Ticket 3-006 pre-computes water distance field, exposed via 3-014 |
| Provides: Terrain gameplay effects | PASS | Ticket 3-003 TerrainTypeInfo covers value_bonus, harmony_bonus, contamination |
| depends_on: [] (no dependencies) | PASS | TerrainSystem core has no dependencies; cross-system serialization tickets (3-036, 3-037) bridge to Epic 1 infrastructure without making TerrainSystem depend on it |

**Terrain types from systems.yaml vs. tickets:**

| systems.yaml Type | Ticket 3-001 Enum Value | Match |
|-------------------|------------------------|-------|
| flat_ground | Substrate(0) | PASS |
| hills | Ridge(1) | PASS |
| water_ocean | DeepVoid(2) | PASS |
| water_river | FlowChannel(3) | PASS |
| water_lake | StillBasin(4) | PASS |
| forest | BiolumeGrove(5) | PASS |
| crystal_fields | PrismaFields(6) | PASS |
| spore_plains | SporeFlats(7) | PASS |
| toxic_marshes | BlightMires(8) | PASS |
| volcanic_rock | EmberCrust(9) | PASS |

All 10 terrain types accounted for with correct canonical alien terminology.

---

### Interface Contract Verification

**Source:** `interfaces.yaml`

| Interface | Relevance to Epic 3 | Status | Notes |
|-----------|---------------------|--------|-------|
| IBuildable.can_build_at(position, terrain) | TerrainSystem provides TerrainData consumed by IBuildable | PASS | Ticket 3-014 exposes terrain data; IBuildable consumers (Epic 4+) will call can_build_at with terrain from ITerrainQueryable |
| IGridQueryable | Generic spatial query -- implemented by SpatialIndex, not TerrainSystem directly | PASS | Ticket 3-014 creates ITerrainQueryable as a specialized terrain query interface. IGridQueryable from interfaces.yaml is for entity queries (SpatialIndex). Different concerns, no overlap. |
| ISerializable | TerrainGrid must be serializable for network sync | PASS | Ticket 3-036 explicitly implements ISerializable for TerrainGrid with version field, fixed-size types, little-endian byte order. Matches ISerializable contract from interfaces.yaml. |
| IContaminationSource | Terrain tiles produce contamination | PASS | Ticket 3-018 exposes terrain contamination sources (BlightMires). Matches contamination.IContaminationSource methods (get_contamination_output, get_contamination_type). |
| IRenderStateProvider | For interpolation between ticks | N/A | Terrain is static between modifications -- no per-tick interpolation needed for terrain mesh. Applicable to moving entities only. |
| ISimulatable | TerrainSystem is NOT listed as ISimulatable implementer | PASS | Correct: TerrainSystem does not tick in the simulation loop. Terrain is modified on-demand, not per-tick. systems.yaml does not list TerrainSystem in ISimulatable.implemented_by. |

**New interfaces defined in Epic 3:**

| Interface | Ticket | Status | Notes |
|-----------|--------|--------|-------|
| ITerrainQueryable | 3-014 | PASS | Read-only query interface for downstream systems. Acceptance criteria include "Interface registered in interfaces.yaml." |
| ITerrainRenderData | 3-016 | PASS | Read-only rendering data interface. Acceptance criteria include "Interface registered in interfaces.yaml." |
| ITerrainModifier | 3-017 | PASS | Server-side modification interface with client-safe cost queries. Acceptance criteria include "Interface registered in interfaces.yaml." |

All three new interfaces require registration in `interfaces.yaml` as part of implementation. Verified this is captured in acceptance criteria.

---

### Pattern Compliance

**Source:** `patterns.yaml`

| Pattern | Status | Notes |
|---------|--------|-------|
| ECS: Components are pure data | **MINOR ISSUE** | See Minor Issue M-001. TerrainComponent is a 4-byte struct stored in a dense grid, not attached to entities. Justified by cache performance, but departs from "All game state lives in components" (CANON.md Core Principle 2). |
| ECS: Systems are stateless | PASS | TerrainSystem manages TerrainGrid as internal data structure. System logic in tickets 3-008 through 3-013 (generation) and 3-019 through 3-021 (modification) is stateless logic operating on grid data. |
| ECS: Entities are just IDs | PASS | Terrain tiles are NOT entities. Multi-tick modifications (3-020, 3-021) create temporary entities with TerrainModificationComponent -- this IS proper ECS usage for tracking in-progress operations. |
| Multiplayer: dedicated_server | PASS | Ticket 3-022 implements server-authoritative terrain modifications. Ticket 3-037 implements seed-based sync. |
| Multiplayer: server_authoritative for simulation | PASS | All modification tickets (3-019 through 3-022) are server-authoritative. Generation is deterministic for client-side reproduction. |
| Multiplayer: synchronization at 20Hz tick rate | PASS | Terrain modifications are on-change events (not per-tick), consistent with multiplayer.synchronization.what_syncs.on_change. |
| Pool model for resources | N/A | TerrainSystem is not a resource system. |
| Grid coordinate system: x=East, y=South, z=Elevation | PASS | Ticket 3-002 uses row-major (y * width + x). 3D mapping in ticket 3-025: grid_x -> world X, elevation -> world Y, grid_z -> world Z. Consistent with patterns.yaml and Epic 2 ticket 2-033. |
| Tile size: 1x1 unit in 3D space | PASS | Ticket 3-025 vertex positions: x = tile_x, z = tile_z (1:1 mapping). |
| Elevation: 32 levels (0-31) | PASS | Ticket 3-001 enforces elevation 0-31. Ticket 3-025 converts to world Y via elevation * 0.25 (ELEVATION_HEIGHT from Epic 2). |
| File naming: PascalCase | PASS | Ticket names use PascalCase (TerrainType, TerrainGrid, TerrainTypeInfo, etc.) |
| Component naming: {Concept}Component | PASS | TerrainComponent, TerrainModificationComponent |
| System naming: {Concept}System | PASS | TerrainSystem |
| Event naming: {Subject}{Action}Event | PASS | TerrainModifiedEvent |
| Error handling: return values for expected failures | PASS | Modification operations (3-019, 3-020, 3-021) return bool success/fail. |

**Art Direction patterns (patterns.yaml art_direction section):**

| Pattern | Status | Notes |
|---------|--------|-------|
| Bioluminescent palette: dark base + glow accents | PASS | Ticket 3-003 defines emissive_color and emissive_intensity per type. Ticket 3-040 specifies exact glow colors. |
| Toon/cell shading | PASS | Ticket 3-026 extends toon shader with terrain variant. |
| Emissive materials for glow | PASS | Tickets 3-003, 3-026, 3-029, 3-030, 3-039 all address emissive rendering. |
| Bloom post-processing | PASS | Ticket 3-026 outputs to emissive render target for bloom extraction. |
| Hard shadow edges | PASS | Ticket 3-024 computes normals for toon shader banding. |

**Map configuration patterns:**

| Pattern | Status | Notes |
|---------|--------|-------|
| Sizes: 128x128, 256x256, 512x512 | PASS | Ticket 3-002 supports all three. Ticket 3-011 scales generation parameters. |
| Size set at game creation, immutable | PASS | No ticket allows map size change after creation. |
| Same tile unit size across all sizes | PASS | All sizes use 1x1 unit per tile. |
| Larger maps require LOD/culling | PASS | Tickets 3-032 (LOD) and 3-034 (frustum culling) address this. |
| Default: medium (256x256) | PASS | Not explicitly stated in Epic 3 tickets but consistent with canon. |

**Terrain type patterns (patterns.yaml terrain_types section):**

| Terrain Type | Buildable | Gameplay Effect | Ticket Coverage | Status |
|-------------|-----------|----------------|-----------------|--------|
| flat_ground (Substrate) | true | None - baseline | 3-001, 3-003 | PASS |
| hills (Ridge) | true | Increased build cost with elevation | 3-001, 3-003 | PASS |
| water/ocean (DeepVoid) | false | Required for fluid extractors, boosts value | 3-001, 3-009 | PASS |
| water/river (FlowChannel) | false | Required for fluid extractors, boosts value | 3-001, 3-009 | PASS |
| water/lake (StillBasin) | false | Required for fluid extractors, boosts value | 3-001, 3-009 | PASS |
| forest (BiolumeGrove) | true (must clear) | Must clear before building; boosts nearby value | 3-001, 3-003, 3-019 | PASS |
| crystal_fields (PrismaFields) | true (must clear) | Bonus value; clearing yields one-time credits | 3-001, 3-003, 3-019 | PASS |
| spore_plains (SporeFlats) | true | Boosts harmony for nearby habitation | 3-001, 3-003, 3-010 | PASS |
| toxic_marshes (BlightMires) | false | Generates contamination, reduces value | 3-001, 3-003, 3-018 | PASS |
| volcanic_rock (EmberCrust) | true | Increased build cost, geothermal bonus | 3-001, 3-003, 3-010 | PASS |

All 10 terrain types match canon gameplay effects and buildability rules.

---

### Terminology Compliance

**Source:** `terminology.yaml`

**Terrain terms (all verified in ticket 3-001 and throughout):**

| Human Term | Canon Term | Used in Tickets | Status |
|------------|-----------|----------------|--------|
| flat_ground | substrate | 3-001: Substrate(0) | PASS |
| hills | ridges | 3-001: Ridge(1) | PASS |
| ocean | deep_void | 3-001: DeepVoid(2), 3-009 title | PASS |
| river | flow_channel | 3-001: FlowChannel(3), 3-009 title | PASS |
| lake | still_basin | 3-001: StillBasin(4), 3-009 title | PASS |
| forest | biolume_grove | 3-001: BiolumeGrove(5) | PASS |
| crystal_fields | prisma_fields | 3-001: PrismaFields(6) | PASS |
| spore_plains | spore_flats | 3-001: SporeFlats(7) | PASS |
| toxic_marshes | blight_mires | 3-001: BlightMires(8) | PASS |
| volcanic_rock | ember_crust | 3-001: EmberCrust(9) | PASS |

**Terrain action terms:**

| Human Term | Canon Term | Used in Tickets | Status |
|------------|-----------|----------------|--------|
| clear_terrain | purge_terrain | 3-019: "Purge Terrain Operation" | PASS |
| level_terrain | grade_terrain | 3-020: "Grade Terrain Operation" | PASS |
| terraform | (not defined) | 3-021: "Terraform Operation" | **MINOR ISSUE** -- See M-002 |

**Other terminology in tickets:**

| Term Used | Expected Canon Term | Status |
|-----------|-------------------|--------|
| "spawn point" (3-012) | Not defined in terminology.yaml | **MINOR ISSUE** -- See M-003 |
| "contamination" (3-018) | contamination (pollution -> contamination) | PASS |
| "sector value" (3-003: value_bonus) | sector_value (land_value -> sector_value) | PASS |
| "harmony" (3-003: harmony_bonus, 3-010) | harmony (happiness -> harmony) | PASS |
| "credits" (3-019: revenue) | credits (money -> credits) | PASS |
| "sea level" (3-002, 3-009) | No canon alias defined | PASS (geographic term, not gameplay term) |
| "chunk" (3-004, 3-025 etc.) | No canon alias needed | PASS (internal implementation term) |

---

### Multiplayer Compliance

**Source:** `patterns.yaml` multiplayer section, `CANON.md` Core Principle 1

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Deterministic generation | PASS | Ticket 3-007: seeded PRNG, cross-platform determinism, strict FP semantics, golden output tests. Tickets 3-008, 3-009, 3-010, 3-012: all require "Fully deterministic generation." |
| Server-authoritative modifications | PASS | Ticket 3-022: server validates ownership, cost, terrain type; broadcasts changes. Tickets 3-019, 3-020, 3-021: all note "Server-authoritative with validation." |
| State synchronization | PASS | Ticket 3-037: seed + modification list on join. TerrainModifiedEvent broadcast during gameplay. Full snapshot fallback. |
| ISerializable for network data | PASS | Ticket 3-036: TerrainGrid implements ISerializable with version field, fixed-size types, little-endian. |
| Player ownership enforcement | PASS | Ticket 3-019, 3-020, 3-021: all validate player_id authority. Ticket 3-022: server validates tile ownership. |
| No client-side prediction | PASS | No ticket implements client prediction for terrain. Modifications are request/confirm only. |
| Seeded RNG from server | PASS | Ticket 3-007: map_seed from server. Ticket 3-037: server sends seed on join. |
| On-change sync (not per-tick) | PASS | Terrain modifications broadcast as events, not per-tick delta. Consistent with multiplayer.synchronization.what_syncs.on_change. |
| Late join support | PASS | Ticket 3-037: join flow sends seed + modifications. Client regenerates deterministically. |
| Cross-platform determinism | PASS | Ticket 3-007: integer-based noise, strict FP flags, cross-platform verification test, golden output reference. HIGH RISK acknowledged. |

---

### Art Direction Compliance

**Source:** `patterns.yaml` art_direction section, `CANON.md` Visual Identity

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Dark base environment | PASS | Ticket 3-003: emissive colors on dark base tones. Ticket 3-026: toon lighting with dark palette. Ticket 3-040: dark tone hex values specified per type. |
| Glow accents (cyan, green, amber, magenta) | PASS | Ticket 3-040: color palette across all 10 types with distinct hues. Ticket 3-003: emissive_color (Vec3) per type. |
| Emissive hierarchy: terrain < buildings | PASS | Ticket 3-003: max terrain emissive 0.60 (PrismaFields). Canon specifies buildings 0.5-1.0. Ticket 3-040: intensity hierarchy verified. |
| Toon/cell shading | PASS | Ticket 3-026: terrain fragment shader extends toon shader with 3-4 bands. |
| Hard shadow edges | PASS | Ticket 3-024: normals computed for toon shader banding. |
| Bloom for glow bleed | PASS | Ticket 3-026: outputs to emissive render target for bloom extraction. |
| Per-terrain-type glow behaviors | PASS | Ticket 3-026: 7 distinct glow behaviors (static, slow pulse, subtle pulse, shimmer, rhythmic pulse, irregular bubble, slow throb). |
| Shoreline glow | PASS | Ticket 3-027: per-vertex shore_factor. Ticket 3-028: shoreline emissive glow with color matching water type. |
| Bioluminescent vegetation | PASS | Ticket 3-029: instance data includes emissive_color, emissive_intensity. Ticket 3-030: models have emissive material regions. |
| "Unpowered structures lose glow" | N/A for terrain | Terrain does not have powered/unpowered state. This rule applies to buildings (Epic 4+). |
| Color-impaired accessibility | PASS | Ticket 3-040: "Pattern differentiation for color-impaired players (veining vs pulsing vs flowing vs geometric)." |

---

### Map Size Compliance

**Source:** `patterns.yaml` map_configuration, `CANON.md` Tech Stack

| Requirement | Status | Evidence |
|-------------|--------|----------|
| 128x128 (small/compact_region) | PASS | Ticket 3-002: explicitly supports. Target metrics: <100ms generation. |
| 256x256 (medium/standard_region, default) | PASS | Ticket 3-002: explicitly supports. Target metrics: <500ms generation. |
| 512x512 (large/expanse_region) | PASS | Ticket 3-002: explicitly supports. Target metrics: <2s generation. |
| Generation scales to size | PASS | Ticket 3-011: dedicated to scaling parameters across sizes. |
| LOD for larger maps | PASS | Ticket 3-032: 3 LOD levels. LOD 2 may never trigger on 128x128. Thresholds tuned per map size. |
| Culling for larger maps | PASS | Ticket 3-034: 84-94% culled at typical zoom on 512x512. |
| Memory within budget | PASS | Ticket 3-002: 64KB/256KB/1MB for TerrainGrid. Ticket 3-005: additional 768KB at 512x512 for water data. Total CPU terrain memory: 1.26MB at 512x512 (within summary's stated budget). |
| GPU memory within budget | PASS | Summary states 17-66 MB terrain GPU memory (3-13% of min spec 512MB). |

---

### Free Camera Compliance

**Source:** `systems.yaml` CameraSystem, `patterns.yaml` grid.art_style, `CANON.md` Tech Stack

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Perspective projection (~35 deg FOV) | PASS | Ticket 3-025: world Y = elevation * 0.25 (consistent with perspective scaling). Ticket 3-035: edge detection tuned for perspective projection at terrain distances. |
| Free camera orbit/pan/zoom/tilt | PASS | Ticket 3-034: "Culling works correctly at all camera angles (preset and free)." Ticket 3-030: "NOT billboard sprites -- free camera (all viewing angles)." |
| Isometric preset snap views | PASS | Ticket 3-034: tests both preset and free angles. |
| Terrain readable at all angles | PASS | Ticket 3-035: edge detection tuned for terrain at multiple zoom levels and camera angles. Ticket 3-040: "Distance readability verified: types distinguishable by color at max zoom-out." |
| 3D models (not sprites) | PASS | Ticket 3-030: "NOT billboard sprites -- free camera (all viewing angles) makes billboards obvious. Low-poly 3D models with emissive materials." |
| Frustum culling at all angles | PASS | Ticket 3-034: "Culling works correctly at all camera angles (preset and free). No visible popping (conservative culling)." |

---

## Conflicts

### C-001: Vegetation Placement Ownership Boundary

**Ticket:** 3-029 (Vegetation Instance Placement Generator)
**Issue:** systems.yaml states that TerrainSystem owns "Vegetation placement." However, ticket 3-029 assigns per-tile vegetation instance placement (position jitter, rotation, scale) to RenderingSystem. The justification is sound: "RenderingSystem generates placement (not TerrainSystem) to avoid syncing per-tree positions over the network."
**Canon Reference:** `systems.yaml` line 174: `- Vegetation placement`
**Severity:** Moderate -- system boundary violation with valid technical justification.
**Resolution Options:**
1. **Update systems.yaml** to clarify: TerrainSystem owns "Vegetation tile designation (which tiles have vegetation)" while RenderingSystem owns "Vegetation visual instance placement (per-tile position jitter, rotation, scale)." This makes the split explicit.
2. **Move placement to TerrainSystem** and have it produce a deterministic instance list that RenderingSystem consumes. This increases network sync cost (vegetation data per tile).
3. **Accept the split as-is** and document that systems.yaml "Vegetation placement" means tile-level designation, not per-instance visual generation.

**Recommended:** Option 1 -- update systems.yaml to reflect the actual boundary. The split is architecturally correct. TerrainSystem designates tile types; RenderingSystem generates visual representations. This is consistent with the existing principle that TerrainSystem "does not own: Terrain rendering."

---

### C-002: Dense Grid vs. ECS Component Pattern

**Ticket:** 3-002 (TerrainGrid Dense Array Storage)
**Issue:** CANON.md Core Principle 2 states "All game state lives in components. All logic lives in systems." Ticket 3-002 explicitly says "This is NOT per-entity ECS storage; TerrainSystem manages this data structure internally." The TerrainGrid is a dense 2D array managed directly by TerrainSystem, not an ECS component attached to entities.
**Canon Reference:** `CANON.md` line 48: "ECS Everywhere"; `patterns.yaml` ecs.overview: "Entities: Just IDs... Components: Pure data structs... Systems: Stateless logic"
**Severity:** Moderate -- architectural pattern departure with valid performance justification.
**Resolution Options:**
1. **Update CANON.md** to acknowledge that terrain data uses a dense grid for performance, with a documented exception to "ECS Everywhere" for spatial grid data where per-entity overhead is prohibitive (4 bytes/tile in dense grid vs. ~24+ bytes/entity in ECS).
2. **Wrap the dense grid in an ECS-compatible interface** where terrain tiles are entities with TerrainComponent, but stored in a dense SoA (Structure of Arrays) backend. This preserves ECS semantics while achieving dense storage.
3. **Accept the departure** and document it as a pragmatic exception. TerrainComponent is still a "pure data struct" -- it is just stored in a dense array rather than an ECS registry.

**Recommended:** Option 3 with documentation. The 4-byte-per-tile dense grid is the correct engineering choice. The ECS pattern is about separation of concerns (data in components, logic in systems, entities as IDs), and the TerrainGrid achieves this -- the data is pure (TerrainComponent struct), the logic is in TerrainSystem, and tile coordinates serve as implicit entity IDs. Document this as a canonical exception for high-density spatial data.

---

## Minor Issues

### M-001: TerrainComponent Naming May Cause Confusion

**Ticket:** 3-001
**Issue:** The name "TerrainComponent" follows the `{Concept}Component` naming pattern, but the struct is stored in a dense grid, not in an ECS registry. This may confuse developers expecting it to be an ECS component queryable via registry.
**Recommendation:** Consider renaming to `TerrainTileData` or `TerrainCell` to distinguish from ECS components. Alternatively, document clearly that TerrainComponent is a "dense storage component" not registered in the ECS registry.

---

### M-002: "Terraform" Operation Lacks Canonical Alien Term

**Ticket:** 3-021
**Issue:** terminology.yaml defines alien terms for "clear_terrain" (purge_terrain) and "level_terrain" (grade_terrain) but does not define an alien term for "terraform." If this operation will be player-facing (UI button, tooltip), it needs a canonical term.
**Recommendation:** Add a canonical term to terminology.yaml. Suggestions: "reform_terrain," "transmute_terrain," or "reclaim_terrain" (given that blight mire reclamation is the primary use case, "reclaim" fits the narrative well).

---

### M-003: "Spawn Point" Lacks Canonical Alien Term

**Ticket:** 3-012
**Issue:** "Spawn point" is human gaming terminology. If player-facing (e.g., map overview showing spawn locations), it needs a canonical alien term per terminology.yaml.
**Recommendation:** Add to terminology.yaml. Suggestion: "emergence_site" or "landing_site" (fits the alien colonization theme -- beings "emerge" or "land" at the site).

---

### M-004: ITerrainQueryable get_water_distance Return Type Widening

**Ticket:** 3-014
**Issue:** Ticket 3-006 stores water distance as uint8 (capped at 255). Ticket 3-014 defines `get_water_distance(x, y) -> uint32`. The return type is wider than the storage type. While not incorrect (implicit widening is safe), it may create an expectation that distances > 255 are possible.
**Recommendation:** Document the uint8 storage cap (255 max) in the ITerrainQueryable interface documentation, or use uint8 as the return type for consistency. If uint32 is chosen for future-proofing, add a note explaining the current 255 cap.

---

### M-005: SporeFlats Clearable Status Inconsistency

**Ticket:** 3-019
**Issue:** Ticket 3-019 acceptance criteria states: "tile is clearable type (BiolumeGrove, PrismaFields, SporeFlats)." However, patterns.yaml terrain_types lists SporeFlats as `buildable: true` without mentioning "must clear first" (unlike BiolumeGrove and PrismaFields which are listed as `buildable: true # Must clear first`). If SporeFlats is directly buildable without clearing, then including it in the clearable list in ticket 3-019 may be an error, or it may be an intentional design choice to allow optional clearing.
**Recommendation:** Clarify in ticket 3-019 and patterns.yaml whether SporeFlats requires clearing before building or is directly buildable. If directly buildable, removing SporeFlats from the clearable list in 3-019 is correct. If clearing is optional (build directly OR clear first), document this explicitly.

---

## Cross-Epic Dependency Verification

All 17 Epic 2 ticket references in Epic 3 were verified against Epic 2's `tickets.md`:

| Epic 3 Ticket | References Epic 2 | Epic 2 Ticket Description | Match |
|---------------|--------------------|--------------------------|-------|
| 3-023 | 2-003 | GPU Resource Management Infrastructure | CORRECT |
| 3-023 | 2-010 | GPU Mesh Representation | CORRECT |
| 3-025 | 2-033 | Position-to-Transform Synchronization (ELEVATION_HEIGHT) | CORRECT |
| 3-025 | 2-050 | Spatial Partitioning for Large Maps (32x32 chunk alignment) | CORRECT |
| 3-026 | 2-004 | Shader Compilation Pipeline | CORRECT |
| 3-026 | 2-005 | Toon Shader Implementation (Stepped Lighting) | CORRECT |
| 3-026 | 2-007 | Graphics Pipeline Creation (MRT for emissive) | CORRECT |
| 3-026 | 2-037 | Emissive Material Support | CORRECT |
| 3-028 | 2-005 | Toon Shader Implementation | CORRECT |
| 3-028 | 2-016 | Transparent Object Handling | CORRECT |
| 3-028 | 2-034 | Render Layer Definitions (WATER layer) | CORRECT |
| 3-029 | 2-012 | GPU Instancing for Repeated Models (referenced in agent notes) | CORRECT |
| 3-030 | 2-009 | glTF Model Loading | CORRECT |
| 3-030 | 2-012 | GPU Instancing for Repeated Models | CORRECT |
| 3-032 | 2-049 | LOD System (Basic Framework) | CORRECT |
| 3-034 | 2-026 | Frustum Culling | CORRECT |
| 3-034 | 2-042 | Render Statistics Display | CORRECT |
| 3-034 | 2-050 | Spatial Partitioning for Large Maps | CORRECT |
| 3-035 | 2-006 | Screen-Space Edge Detection (Post-Process) | CORRECT |
| 3-036 | Epic 1 | ISerializable interface (from interfaces.yaml) | CORRECT |
| 3-037 | Epic 1 | SyncSystem for transport | CORRECT |
| 3-038 | 2-040 | Debug Grid Overlay | CORRECT |
| 3-039 | 2-008 | ToonShaderConfig Runtime Resource | CORRECT |

**Cross-Epic Dependencies Table (from Epic 3 tickets.md):**

All entries in the cross-epic dependency table at the bottom of tickets.md were verified. The table correctly maps:
- 2-001 through 2-007 for GPU infrastructure
- 2-005 for toon shader base
- 2-006 for edge detection
- 2-008 for ToonShaderConfig
- 2-009 for glTF loading
- 2-010 for GPU mesh
- 2-012 for GPU instancing
- 2-016 for transparent handling
- 2-026 for frustum culling
- 2-029, 2-030 for ray casting/tile picking (noted as soft circular dependency)
- 2-034 for render layers
- 2-037 for emissive support
- 2-038 for bloom
- 2-040 for debug grid
- 2-049 for LOD framework
- 2-050 for spatial partitioning

**Circular dependency note verified:** Epic 2 ticket 2-030 (Tile Picking) needs terrain elevation from Epic 3 for heightmap ray casting. The resolution is documented: Epic 2 implements flat-plane picking first; Epic 3 exposes get_elevation(); then picking is updated. This is a soft dependency, correctly handled.

**No mismatches or incorrect references found.**

---

## Recommendations

### R-001: Resolve Vegetation Ownership Boundary (Conflict C-001)

Update `systems.yaml` to split vegetation ownership: TerrainSystem owns tile-level vegetation designation (which tiles are BiolumeGrove type), RenderingSystem owns per-tile visual instance generation. This accurately describes the architecture documented in ticket 3-029 and prevents future confusion about system boundaries.

### R-002: Document Dense Grid Exception (Conflict C-002)

Add a note to CANON.md or patterns.yaml documenting that high-density spatial data (terrain grid) uses dense array storage rather than per-entity ECS, as a documented exception to "ECS Everywhere." The exception is justified by: (a) 4 bytes/tile dense vs. ~24+ bytes/entity in ECS, (b) cache performance for linear traversal, (c) terrain tiles are implicit entities identified by coordinate rather than EntityID.

### R-003: Add Missing Terminology (Minor Issues M-002, M-003)

Update terminology.yaml with:
- `terraform: reclaim_terrain` (or chosen alien term) for the terrain type conversion operation
- `spawn_point: emergence_site` (or chosen alien term) for multiplayer starting locations

### R-004: Clarify SporeFlats Clearable Behavior (Minor Issue M-005)

Determine whether SporeFlats requires clearing before building or is directly buildable. Update both patterns.yaml terrain_types and ticket 3-019 acceptance criteria to be consistent. This affects gameplay balance: if SporeFlats must be cleared, the harmony bonus is sacrificed; if directly buildable, players can build on SporeFlats while keeping the bonus.

### R-005: Consider Adding TerrainSystem to ISimulatable

Currently TerrainSystem has no simulation tick. However, ticket 3-020 (Grade) and 3-021 (Terraform) create multi-tick operations that need per-tick processing. These tickets handle this via temporary entities with TerrainModificationComponent. Consider whether TerrainSystem should implement ISimulatable with a tick() that processes in-progress modifications, or whether the current approach (temporary entities processed by a modification sub-system) is preferred. Either way, document the decision.

### R-006: Validate Ticket Count Against Summary Table

The tickets.md summary table at the bottom counts:
- Group A: 7 tickets (3-001 through 3-007) -- CORRECT
- Group B: 6 tickets (3-008 through 3-013) -- CORRECT
- Group C: 4 tickets (3-014 through 3-017) -- CORRECT
- Group D: 5 tickets (3-018 through 3-022) -- CORRECT
- Group E: 4 tickets (3-023 through 3-026) -- CORRECT
- Group F: 2 tickets (3-027, 3-028) -- CORRECT
- Group G: 2 tickets (3-029, 3-030) -- CORRECT
- Group H: 5 tickets (3-031 through 3-035) -- CORRECT
- Group I: 2 tickets (3-036, 3-037) -- CORRECT
- Group J: 3 tickets (3-038 through 3-040) -- CORRECT
- Total: 40 -- CORRECT

Size distribution verified: S=9, M=17, L=10, XL=4 (counted and confirmed against individual tickets).

---

## Verification Certification

All 40 tickets in Epic 3 have been individually verified against:
- `systems.yaml` v0.5.0 (system boundaries)
- `interfaces.yaml` v0.3.0 (interface contracts)
- `patterns.yaml` v0.5.0 (architectural patterns, art direction, map configuration, terrain types)
- `terminology.yaml` v0.5.0 (canonical alien terminology)
- `CANON.md` v0.5.0 (core principles)
- `docs/epics/epic-2/tickets.md` (cross-epic dependency references)

**Result: CONDITIONALLY APPROVED**

Epic 3 is approved for implementation pending resolution of:
1. Conflict C-001 (vegetation ownership boundary) -- recommend canon update
2. Conflict C-002 (dense grid vs. ECS) -- recommend documented exception

All 5 minor issues are non-blocking but should be addressed before or during implementation.

All cross-epic references to Epic 2 tickets are accurate and correctly described.
