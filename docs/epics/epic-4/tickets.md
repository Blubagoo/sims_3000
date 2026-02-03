# Epic 4: Zoning & Building System - Tickets

**Epic Goal:** Establish the colony growth loop: zone designation, structure materialization, building state machine, template-driven variety, growth pressure (demand), forward dependency stubs for infrastructure epics, and multiplayer-safe server-authoritative operations
**Created:** 2026-01-28
**Canon Version:** 0.13.0
**Planning Agents:** Systems Architect, Game Designer, Zone Engineer, Building Engineer

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-28 | v0.6.0 | Initial ticket creation from four-agent analysis and resolved discussion (CCR-001 through CCR-012) |
| 2026-01-29 | canon-verification (v0.6.0 → v0.13.0) | Minor terminology alignment needed (see note below) |

> **Verification Note (2026-01-29):** Verified forward dependency stubs against interfaces.yaml v0.13.0:
> - **IEnergyProvider**: ✓ Matches (is_powered, is_powered_at)
> - **IFluidProvider**: ✓ Updated to use "has_fluid" terminology to match interfaces.yaml
> - **ITransportProvider**: ✓ Updated to use is_road_accessible_at(x, y, max_distance)
> - **IDemandProvider**: ✓ Matches (get_demand)
> - **ILandValueProvider**: ℹ Not defined as separate interface in interfaces.yaml; LandValueSystem implements IGridOverlay for value queries — stub is valid for Epic 4
> - **ICreditProvider**: ℹ Not defined as separate interface; EconomySystem provides treasury queries via IEconomyQueryable — stub is valid for Epic 4

---

## Summary

Epic 4 is the most-depended-upon epic after Epic 3. It implements the core colony growth loop: overseers designate zones, the simulation responds by materializing structures, and a demand system (growth pressure) drives ongoing expansion. Seven downstream epics consume its interfaces.

**Key Design Decisions (Resolved via Discussion):**
- CCR-001: 5-state machine -- Materializing, Active, Abandoned, Derelict, Deconstructed (Building Engineer naming)
- CCR-002: ZoneComponent is 4 bytes -- zone_type, density, state (Designated/Occupied/Stalled), desirability
- CCR-003: BuildingComponent is 28 bytes -- Building Engineer's canonical struct
- CCR-004: ZoneGrid = sparse EntityID index; BuildingGrid = dense grid exception (like TerrainGrid)
- CCR-005: Density is player-chosen at designation; existing structures persist at old density on redesignation
- CCR-006: Six forward dependency stubs -- IEnergyProvider, IFluidProvider, ITransportProvider, ILandValueProvider, IDemandProvider, ICreditProvider
- CCR-007: Chebyshev distance for pathway proximity (3 sectors = 7x7 square), measured from nearest footprint edge
- CCR-008: ZoneSystem owns basic demand; IDemandProvider interface for handoff to DemandSystem (Epic 10)
- CCR-009: Target 30 templates minimum (5 per zone_type+density bucket)
- CCR-010: NO scale variation -- rotation + color accent only
- CCR-011: 4 construction phases (Foundation/Framework/Exterior/Finalization)
- CCR-012: De-zoning occupied sector uses DemolitionRequestEvent (decoupled)

**Target Metrics:**
- Structure spawn: max 3 per overseer per scan (every 20 ticks / 1 second)
- Materialization time: 40-200 ticks (2-10 seconds real time)
- BuildingSystem tick: <2ms at 10,000 structures
- ZoneComponent: 4 bytes; BuildingComponent: 28 bytes (32 aligned)
- ZoneGrid memory: 64KB (128x128) to 1MB (512x512)
- BuildingGrid memory: 64KB (128x128) to 1MB (512x512)

---

## Ticket Counts

| Category | Count |
|----------|-------|
| **By Type** | |
| Infrastructure | 16 |
| Feature | 27 |
| Content | 1 |
| Design | 4 |
| **By System** | |
| ZoneSystem | 16 |
| BuildingSystem | 25 |
| Shared | 7 |
| **By Scope** | |
| S | 15 |
| M | 19 |
| L | 12 |
| XL | 2 |
| **By Priority** | |
| P0 (critical path) | 18 |
| P1 (important) | 22 |
| P2 (enhancement) | 8 |
| **Total** | **48** |

---

## Ticket List

### Group A: Data Types and Enumerations

---

### 4-001: Zone Type Enumerations and ZoneComponent Structure

**Type:** infrastructure
**System:** ZoneSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the foundational zone data types. ZoneType enum with 3 canonical alien zone types (Habitation, Exchange, Fabrication). ZoneDensity enum (LowDensity, HighDensity). ZoneState enum (Designated, Occupied, Stalled). ZoneComponent struct at exactly 4 bytes: zone_type (ZoneType), density (ZoneDensity), state (ZoneState), desirability (uint8_t, 0-255 cached attractiveness score). Per CCR-002, this is the canonical field set. Also define supporting structs: ZoneDemandData, ZoneCounts, ZonePlacementRequest, ZonePlacementResult, DezoneResult.

**Acceptance Criteria:**
- [ ] ZoneType enum: Habitation(0), Exchange(1), Fabrication(2)
- [ ] ZoneDensity enum: LowDensity(0), HighDensity(1)
- [ ] ZoneState enum: Designated(0), Occupied(1), Stalled(2)
- [ ] ZoneComponent struct is exactly 4 bytes (verified with static_assert)
- [ ] ZoneDemandData struct with int8_t fields for habitation_demand, exchange_demand, fabrication_demand (range -100 to +100)
- [ ] ZoneCounts struct with per-type, per-density counters and designated/occupied/stalled totals
- [ ] ZonePlacementRequest/Result and DezoneResult structs defined
- [ ] All enum values use canonical alien terminology (habitation not residential, exchange not commercial, fabrication not industrial)
- [ ] Unit tests for component size assertion and enum value ranges

**Dependencies:**
- Blocked by: None (foundational)
- Blocks: 4-002, 4-003, 4-005, 4-006, 4-007, 4-008, 4-009, 4-010, 4-011, 4-012, 4-015, 4-016, 4-017

**Agent Notes:**
- Zone Engineer: Canonical field set. desirability caches aggregate attractiveness for BuildingSystem template selection. Demand is system-level, not per-sector.
- Systems Architect: demand_weight dropped per CCR-002 in favor of desirability field. 4 bytes enables tight ECS packing.
- Game Designer: Three zone types (habitation/exchange/fabrication) with two density levels provides sufficient strategic depth without analysis paralysis.

---

### 4-002: Building State and Type Enumerations

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the foundational structure data types. BuildingState enum with 5 states per CCR-001 (Materializing, Active, Abandoned, Derelict, Deconstructed). ZoneBuildingType enum matching zone types. DensityLevel enum. ConstructionPhase enum with 4 phases per CCR-011 (Foundation, Framework, Exterior, Finalization).

**Acceptance Criteria:**
- [ ] BuildingState enum: Materializing(0), Active(1), Abandoned(2), Derelict(3), Deconstructed(4)
- [ ] ZoneBuildingType enum: Habitation(0), Exchange(1), Fabrication(2)
- [ ] DensityLevel enum: Low(0), High(1)
- [ ] ConstructionPhase enum: Foundation(0), Framework(1), Exterior(2), Finalization(3)
- [ ] All enums use canonical alien terminology (materializing not under_construction, derelict not abandoned, deconstructed not demolished)
- [ ] Unit tests for enum values

**Dependencies:**
- Blocked by: None (foundational)
- Blocks: 4-003, 4-004, 4-019, 4-020, 4-021, 4-023, 4-024, 4-025, 4-026, 4-027

**Agent Notes:**
- Building Engineer: 5-state machine is the canonical lifecycle. Systems Architect's "Degraded" merged into Abandoned with a grace timer tracked within Active state.
- Game Designer: States map to visual treatments -- materializing = emergence glow, active = normal glow, abandoned = flickering, derelict = dark, deconstructed = debris.

---

### 4-003: BuildingComponent Structure

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the BuildingComponent struct per CCR-003 (28 bytes canonical). Fields: template_id (uint32_t), zone_type (ZoneBuildingType), density (DensityLevel), state (BuildingState), level (uint8_t), health (uint8_t), capacity (uint16_t), current_occupancy (uint16_t), footprint_w (uint8_t), footprint_h (uint8_t), state_changed_tick (uint32_t), abandon_timer (uint16_t), rotation (uint8_t, 0-3 for 0/90/180/270), color_accent_index (uint8_t). Per CCR-010, NO scale variation stored -- rotation and color accent only.

**Acceptance Criteria:**
- [ ] BuildingComponent struct defined with all fields from CCR-003
- [ ] Struct size is 28 bytes packed (verified with static_assert) or 32 bytes aligned
- [ ] rotation stores 0-3 (four 90-degree rotations)
- [ ] color_accent_index indexes into template's accent palette
- [ ] No scale fields (per CCR-010: no scale variation)
- [ ] Trivially copyable (verified with static_assert on is_trivially_copyable)
- [ ] Unit tests for default initialization and size assertion

**Dependencies:**
- Blocked by: 4-002 (BuildingState, ZoneBuildingType, DensityLevel enums)
- Blocks: 4-004, 4-019, 4-025, 4-026, 4-027, 4-034, 4-035, 4-038, 4-040, 4-042

**Agent Notes:**
- Building Engineer: 28-byte struct is canonical per CCR-003. template_id as uint32_t allows billions of templates. capacity/current_occupancy as uint16_t supports up to 65,535 occupants.
- Systems Architect: No pointers in component -- template_id is a lookup key. All fields are fixed-size for deterministic serialization.

---

### 4-004: ConstructionComponent Structure

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the ConstructionComponent struct -- a transient component attached during the Materializing state and removed on completion. Fields: ticks_total (uint16_t), ticks_elapsed (uint16_t), phase (ConstructionPhase), phase_progress (uint8_t, 0-255 within current phase for smooth interpolation), construction_cost (uint32_t), is_paused (bool). Per CCR-011, 4 construction phases map to 0-25%, 25-50%, 50-75%, 75-100% progress.

**Acceptance Criteria:**
- [ ] ConstructionComponent struct defined with all fields
- [ ] Struct size is 12 bytes (verified with static_assert)
- [ ] Phase derived from progress: Foundation(0-25%), Framework(25-50%), Exterior(50-75%), Finalization(75-100%)
- [ ] phase_progress (0-255) provides smooth interpolation within each phase for RenderingSystem
- [ ] is_paused prevents ticks_elapsed from incrementing
- [ ] Trivially copyable for serialization
- [ ] Unit tests for phase calculation from progress percentage

**Dependencies:**
- Blocked by: 4-002 (ConstructionPhase enum)
- Blocks: 4-025, 4-026, 4-042

**Agent Notes:**
- Building Engineer: Transient component pattern -- added at spawn, removed at completion. ECS handles add/remove efficiently via archetype migration per Q038 resolution.
- Systems Architect: RenderingSystem reads phase and phase_progress directly (no separate RenderingHint needed per Q043).
- Game Designer: 4-phase construction (Foundation/Framework/Exterior/Finalization) provides granular visual feedback during materializing.

---

### 4-005: DebrisComponent Structure

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Define the DebrisComponent struct for deconstructed structure debris. Fields: original_template_id (uint32_t), clear_timer (uint16_t, ticks until auto-clear), footprint_w (uint8_t), footprint_h (uint8_t). Debris occupies the same sectors as the original structure, blocks new construction, and auto-clears after a configurable timer (default 60 ticks / 3 seconds).

**Acceptance Criteria:**
- [ ] DebrisComponent struct defined with all fields
- [ ] Default clear_timer configurable (default: 60 ticks / 3 seconds)
- [ ] Debris blocks new structure construction on occupied sectors
- [ ] Auto-clear behavior documented (timer counts down each tick)
- [ ] Unit tests for timer initialization and default values

**Dependencies:**
- Blocked by: None (standalone data type)
- Blocks: 4-030, 4-031

**Agent Notes:**
- Building Engineer: Debris is the final visible state before sector returns to development eligibility. Short-lived entity with minimal data.

---

### Group B: Grid Storage and Spatial Indices

---

### 4-006: ZoneGrid Sparse Spatial Index

**Type:** infrastructure
**System:** ZoneSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the ZoneGrid class -- a flat array storing EntityID per cell for O(1) spatial lookups of zone data. Per CCR-004, ZoneGrid is a sparse index (most cells are INVALID/empty), NOT a dense grid exception. Stores EntityID (uint32_t) per cell; INVALID_ENTITY (0) means no zone. Same dimensions as TerrainGrid. Row-major layout.

**Acceptance Criteria:**
- [ ] ZoneGrid class with width, height, and flat vector<EntityID> storage
- [ ] place_zone(x, y, EntityID) sets cell and returns success/failure
- [ ] remove_zone(x, y) clears cell and returns success/failure
- [ ] get_zone_at(x, y) returns EntityID or INVALID_ENTITY
- [ ] has_zone_at(x, y) returns bool
- [ ] in_bounds(x, y) validation
- [ ] Row-major storage: index = y * width + x
- [ ] Supports 128x128, 256x256, 512x512 map sizes
- [ ] Memory: 4 bytes per cell (64KB / 256KB / 1MB for respective sizes)
- [ ] Unit tests for place/remove/query, bounds checking, and edge-of-map sectors

**Dependencies:**
- Blocked by: 4-001 (ZoneComponent types needed for entity composition)
- Blocks: 4-011, 4-012, 4-015, 4-016, 4-017, 4-038

**Agent Notes:**
- Zone Engineer: Sparse index -- only stores EntityIDs where zones exist. O(1) point queries, prevents zone overlaps via simple non-zero check.
- Systems Architect: ZoneGrid is a private implementation detail of ZoneSystem (per Q020). Consumers use purpose-specific query methods.

---

### 4-007: BuildingGrid Dense Occupancy Array

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the BuildingGrid class -- a dense 2D grid of EntityID per cell for O(1) building spatial lookups. Per CCR-004, BuildingGrid follows the dense_grid_exception pattern (same as TerrainGrid). Every sector potentially has a structure, spatial lookups must be O(1), and per-entity overhead is prohibitive at scale. Supports multi-sector footprint registration (a 2x2 structure marks all 4 cells with the same EntityID).

**Acceptance Criteria:**
- [ ] BuildingGrid class with dense flat vector<EntityID> storage
- [ ] get_building_at(x, y) returns EntityID or INVALID_ENTITY -- O(1)
- [ ] set_building_at(x, y, EntityID) registers a structure at a cell
- [ ] clear_building_at(x, y) removes registration
- [ ] is_tile_occupied(x, y) returns bool
- [ ] is_footprint_available(x, y, w, h) checks all cells in a rectangular footprint
- [ ] Multi-sector registration: set_footprint(x, y, w, h, EntityID) marks all cells
- [ ] clear_footprint(x, y, w, h) clears all cells
- [ ] Memory: 4 bytes per cell (64KB / 256KB / 1MB for respective sizes)
- [ ] Unit tests for single-sector and multi-sector (2x2) operations, overlaps, bounds checking

**Dependencies:**
- Blocked by: None (foundational data structure)
- Blocks: 4-024, 4-025, 4-030, 4-034

**Agent Notes:**
- Building Engineer: Dense grid pattern justified by O(1) lookups for every system querying structure presence. Canon update needed: add BuildingGrid to dense_grid_exception in patterns.yaml.
- Systems Architect: Same rationale as TerrainGrid. Per Q036, formally added to canon patterns.

---

### Group C: ZoneSystem Core

---

### 4-008: ZoneSystem Class Skeleton and ISimulatable

**Type:** infrastructure
**System:** ZoneSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define the ZoneSystem class implementing ISimulatable at priority 30. Constructor takes ITerrainQueryable* (Epic 3) and ITransportProvider* (stub). tick() method orchestrates demand update and desirability refresh. Declare all query methods (point, area, aggregate, demand, state mutation). Wire ZoneGrid internally. Initialize per-overseer ZoneCounts.

**Acceptance Criteria:**
- [ ] ZoneSystem class declaration with ISimulatable implementation
- [ ] get_priority() returns 30 (zones process before structures each tick)
- [ ] Constructor accepts ITerrainQueryable* and ITransportProvider* via dependency injection
- [ ] tick() method stub that will orchestrate demand and desirability updates
- [ ] All query method declarations from Zone Engineer analysis Section 7
- [ ] Internal ZoneGrid instance created on initialization
- [ ] Per-overseer ZoneCounts initialized
- [ ] Compiles and registers as a system (no logic yet)

**Dependencies:**
- Blocked by: 4-001 (zone types), 4-006 (ZoneGrid)
- Blocks: 4-011, 4-012, 4-013, 4-014, 4-015, 4-016, 4-017, 4-038
- External: Epic 3 ticket 3-014 (ITerrainQueryable interface)

**Agent Notes:**
- Zone Engineer: System skeleton enables parallel development of placement, demand, and query features.
- Systems Architect: Priority 30 ensures zones are settled before BuildingSystem (priority 40) runs.

---

### 4-009: Zone Event Definitions

**Type:** infrastructure
**System:** ZoneSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define all zone-related event structs. ZoneDesignatedEvent (zone entity, position, type, density, owner), ZoneUndesignatedEvent (zone entity, position, type, owner), ZoneStateChangedEvent (zone entity, position, old_state, new_state), ZoneDemandChangedEvent (overseer, demand data). Per CCR-012, also define DemolitionRequestEvent (position, requesting entity) used when de-zoning an occupied sector.

**Acceptance Criteria:**
- [ ] ZoneDesignatedEvent with fields: entity_id, grid_position, zone_type, density, owner_id
- [ ] ZoneUndesignatedEvent with fields: entity_id, grid_position, zone_type, owner_id
- [ ] ZoneStateChangedEvent with fields: entity_id, grid_position, old_state, new_state
- [ ] ZoneDemandChangedEvent with fields: player_id, ZoneDemandData
- [ ] DemolitionRequestEvent with fields: grid_position, requesting_entity_id (for CCR-012 de-zone flow)
- [ ] All events follow canon event pattern (suffix: Event)
- [ ] Unit tests verifying struct completeness

**Dependencies:**
- Blocked by: 4-001 (zone type enums for event fields)
- Blocks: 4-012, 4-013, 4-016

**Agent Notes:**
- Zone Engineer: DemolitionRequestEvent decouples ZoneSystem from BuildingSystem (per Q024 / CCR-012). ZoneSystem emits; BuildingSystem handles demolition.
- Systems Architect: Events are the primary notification mechanism. Direct method calls for state mutation (BuildingSystem -> ZoneSystem.set_zone_state).

---

### 4-010: Building Event Definitions

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define all structure-related event structs. BuildingConstructedEvent, BuildingAbandonedEvent, BuildingRestoredEvent, BuildingDerelictEvent, BuildingDeconstructedEvent, DebrisClearedEvent, BuildingUpgradedEvent, BuildingDowngradedEvent. These events are consumed by RenderingSystem, UISystem, AudioSystem, EconomySystem, and future epic systems.

**Acceptance Criteria:**
- [ ] BuildingConstructedEvent: entity_id, owner_id, zone_type, grid_position, template_id
- [ ] BuildingAbandonedEvent: entity_id, owner_id, grid_position
- [ ] BuildingRestoredEvent: entity_id, owner_id, grid_position
- [ ] BuildingDerelictEvent: entity_id, owner_id, grid_position
- [ ] BuildingDeconstructedEvent: entity_id, owner_id, grid_position, was_player_initiated (bool)
- [ ] DebrisClearedEvent: entity_id, grid_position
- [ ] BuildingUpgradedEvent: entity_id, old_level, new_level
- [ ] BuildingDowngradedEvent: entity_id, old_level, new_level
- [ ] All events follow canon event pattern
- [ ] Unit tests for struct completeness

**Dependencies:**
- Blocked by: 4-002 (building type enums for event fields)
- Blocks: 4-025, 4-026, 4-027, 4-028, 4-030, 4-031, 4-034

**Agent Notes:**
- Building Engineer: Events are the primary notification channel to downstream systems. BuildingConstructedEvent includes template_id per Q014 resolution.
- Systems Architect: Server broadcasts these events to all clients. No client-side derivation from RNG.

---

### Group D: Zone Placement and Validation

---

### 4-011: Zone Placement Validation Pipeline

**Type:** feature
**System:** ZoneSystem
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement the full zone placement validation pipeline. Validation order: (1) bounds check, (2) ownership check (overseer owns sector), (3) terrain check (ITerrainQueryable.is_buildable), (4) overlap check (ZoneGrid.has_zone_at is false), (5) building check (no non-zone structure at position). Pathway proximity does NOT gate designation (per CCR-007 / Q017 -- it gates structure development instead). Supports both single-sector and rectangular area validation for the drag-to-zone tool.

**Acceptance Criteria:**
- [ ] validate_zone_placement(x, y, player_id) returns success/failure with reason
- [ ] Bounds check rejects coordinates outside map
- [ ] Ownership check rejects sectors not owned by requesting overseer
- [ ] Terrain check calls ITerrainQueryable.is_buildable(x, y)
- [ ] Overlap check via ZoneGrid.has_zone_at(x, y)
- [ ] Building check confirms no existing non-zone structure
- [ ] Pathway proximity is NOT checked at designation time (gates development, not designation)
- [ ] validate_zone_area(GridRect, player_id) validates each sector in rectangle, returns per-sector results
- [ ] Failed sectors are silently skipped in area operations
- [ ] Unit tests for each validation step individually and combined

**Dependencies:**
- Blocked by: 4-006 (ZoneGrid), 4-008 (ZoneSystem skeleton)
- Blocks: 4-012, 4-015
- External: Epic 3 ticket 3-014 (ITerrainQueryable.is_buildable)

**Agent Notes:**
- Zone Engineer: Strict validation order prevents partial or invalid states. Pathway check intentionally excluded -- overseers should be able to plan zones ahead of pathways.
- Game Designer: Allowing designation before pathways preserves the "plan-ahead" workflow that feels natural in colony builders.

---

### 4-012: Zone Placement Execution

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement zone placement execution. After validation passes, create zone entities with ZoneComponent + PositionComponent + OwnershipComponent. Update ZoneGrid spatial index. Update per-overseer ZoneCounts. Emit ZoneDesignatedEvent. Handle multi-sector placement (iterate rectangle, skip failed sectors, report result). Deduct designation credits (low density: 2 credits/sector, high density: 5 credits/sector -- configurable).

**Acceptance Criteria:**
- [ ] place_zones(ZonePlacementRequest) creates entities for valid sectors
- [ ] Each zone entity has: ZoneComponent, PositionComponent, OwnershipComponent
- [ ] ZoneComponent initialized: zone_type from request, density from request, state = Designated, desirability = 0
- [ ] ZoneGrid updated for each placed zone
- [ ] ZoneCounts incremented for each placed zone
- [ ] ZoneDesignatedEvent emitted per zone (or batched)
- [ ] Multi-sector placement iterates rectangle, skips invalid, returns ZonePlacementResult with success/skip counts
- [ ] Credit cost deducted (configurable: default 2/5 credits per sector for low/high density)
- [ ] Server-authoritative: client sends request, server validates and applies
- [ ] Unit tests for single and multi-sector placement, cost calculation, result reporting

**Dependencies:**
- Blocked by: 4-011 (validation), 4-009 (events), 4-006 (ZoneGrid), 4-008 (ZoneSystem)
- Blocks: 4-015, 4-038

**Agent Notes:**
- Zone Engineer: Placement is atomic per request. Multi-sector operations are all-or-nothing per sector, with partial success reported in result.
- Game Designer: Small designation cost (2-5 credits) prevents spam zoning while maintaining low friction.

---

### 4-013: De-zoning Implementation

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement de-zoning (undesignation). Validate de-zone request (zone exists, overseer owns sector). If zone is Designated or Stalled (no structure), destroy zone entity immediately at no cost. If zone is Occupied (structure present), emit DemolitionRequestEvent per CCR-012 -- BuildingSystem handles demolition, then calls ZoneSystem.set_zone_state(Designated), and ZoneSystem destroys the zone entity. Support rectangular de-zone (drag-to-dezone). Update ZoneGrid and ZoneCounts. Emit ZoneUndesignatedEvent.

**Acceptance Criteria:**
- [ ] remove_zones(GridRect, player_id) validates and removes zones
- [ ] Empty zones (Designated/Stalled): immediate entity destruction, no cost
- [ ] Occupied zones: DemolitionRequestEvent emitted (per CCR-012)
- [ ] BuildingSystem handles demolition, calls back set_zone_state(Designated), ZoneSystem then destroys entity
- [ ] ZoneGrid cleared for each removed zone
- [ ] ZoneCounts decremented
- [ ] ZoneUndesignatedEvent emitted per removed zone
- [ ] No refund of original designation cost
- [ ] Rectangular de-zone iterates area, skips invalid sectors
- [ ] Unit tests for empty and occupied de-zoning, event emission

**Dependencies:**
- Blocked by: 4-009 (events including DemolitionRequestEvent), 4-008 (ZoneSystem)
- Blocks: 4-014

**Agent Notes:**
- Zone Engineer: De-zoning occupied sectors uses events to avoid circular dependency (ZoneSystem -> BuildingSystem -> ZoneSystem).
- Game Designer: De-zoning is the overseer deliberately removing a designation. Occupied sector demolition creates a meaningful cost.

---

### 4-014: Redesignation (Zone Type/Density Change)

**Type:** feature
**System:** ZoneSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement redesignation -- changing zone type or density on an existing zone. If zone is Designated or Stalled (empty), allow direct atomic change of zone_type and/or density in place. Deduct cost difference (per Q027/Q029). If zone is Occupied, de-zone first (triggers demolition per CCR-012) then re-zone. Per CCR-005, density change on an occupied zone updates ZoneComponent.density but existing structure retains its original density until naturally replaced.

**Acceptance Criteria:**
- [ ] redesignate_zone(x, y, new_type, new_density, player_id) handles all cases
- [ ] Empty zone (Designated/Stalled): direct field update, cost = difference between new and old designation cost
- [ ] Occupied zone with type change: triggers demolition flow (de-zone + re-zone)
- [ ] Occupied zone with density-only change: ZoneComponent.density updated in place, existing structure keeps its density (per CCR-005)
- [ ] Downgrade density (high -> low) on empty zone: allowed, no refund
- [ ] Upgrade density (low -> high) on empty zone: costs difference (5 - 2 = 3 credits)
- [ ] Unit tests for all redesignation scenarios

**Dependencies:**
- Blocked by: 4-013 (de-zoning for occupied case)
- Blocks: None

**Agent Notes:**
- Zone Engineer: Direct redesignation for empty zones avoids unnecessary de-zone/re-zone cycle (Q027).
- Building Engineer: Per CCR-005, existing structures are NOT auto-demolished on density change. They persist at old density until naturally replaced.

---

### Group E: Zone State and Demand

---

### 4-015: Zone State Management

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement ZoneState transition management. Designated -> Occupied (when BuildingSystem places a structure, it calls set_zone_state). Occupied -> Designated (when structure is demolished, BuildingSystem calls set_zone_state). Designated -> Stalled (when pathway access is lost -- requires ITransportProvider, stubbed as always-accessible in Epic 4). Stalled -> Designated (when pathway access restored). BuildingSystem uses direct method call for state mutation (per Q023 resolution).

**Acceptance Criteria:**
- [ ] set_zone_state(x, y, ZoneState) updates zone entity's state field
- [ ] Designated -> Occupied transition on structure placement
- [ ] Occupied -> Designated transition on structure demolition
- [ ] Designated -> Stalled transition when ITransportProvider reports no pathway access (stubbed: never triggers in Epic 4)
- [ ] Stalled -> Designated transition when pathway access restored
- [ ] ZoneStateChangedEvent emitted on each transition
- [ ] ZoneCounts updated on state transitions
- [ ] Stalled zones do NOT count toward supply in demand calculation
- [ ] Unit tests for all state transitions

**Dependencies:**
- Blocked by: 4-001 (ZoneState enum), 4-009 (events), 4-006 (ZoneGrid), 4-008 (ZoneSystem)
- Blocks: 4-038

**Agent Notes:**
- Zone Engineer: Stalled state provides UI feedback about zones that cannot develop. With Epic 4 stubs, no zones will be stalled (all pathway-accessible).
- Game Designer: Stalled state creates the "what's wrong?" feedback loop that prevents "Nothing Is Happening" Syndrome.

---

### 4-016: Basic Demand Calculation (Growth Pressure)

**Type:** feature
**System:** ZoneSystem
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement per-overseer, per-zone-type demand calculation (growth pressure). Per CCR-008, ZoneSystem owns basic demand for Epic 4. Formula: demand[type] = base_pressure + stub_factors - supply_saturation. Base pressure: Habitation=+10, Exchange=+5, Fabrication=+5. Stub factors: population(+20/+10/+10), employment(0), utility(+10), tribute(0). Supply saturation computed from occupied zone count. Range clamped to [-100, +100]. Soft cap at +80 (diminishing returns). Updated once per tick. Per-overseer demand is independent (no cross-overseer interaction in Epic 4).

**Acceptance Criteria:**
- [ ] Demand calculated per overseer, per zone type, each tick
- [ ] Base pressure: Habitation=+10, Exchange=+5, Fabrication=+5
- [ ] Stub factor values for population, employment, utility, tribute (all configurable)
- [ ] Supply saturation: ratio of occupied zones to target count, reducing demand as supply increases
- [ ] Hard clamp: [-100, +100]
- [ ] Soft cap: above +80, demand growth rate halved
- [ ] Negative demand can reach -100 (triggers structure abandonment in BuildingSystem)
- [ ] get_zone_demand(player_id) returns ZoneDemandData
- [ ] get_demand_for_type(ZoneType, player_id) returns int8_t
- [ ] Lightweight computation: uses cached ZoneCounts, no entity iteration
- [ ] All constants configurable for tuning
- [ ] Unit tests for demand at various zone count levels, clamping, soft cap

**Dependencies:**
- Blocked by: 4-001 (zone types), 4-009 (ZoneDemandChangedEvent), 4-008 (ZoneSystem tick)
- Blocks: 4-017, 4-025

**Agent Notes:**
- Zone Engineer: Basic demand is a temporary calculation replaced by DemandSystem (Epic 10) via IDemandProvider.
- Systems Architect: Linear saturation for Epic 4 simplicity. DemandSystem can introduce curves.
- Game Designer: Growth pressure is the steering mechanism. The H/E/F balance creates the core management tension.

---

### 4-017: Demand Factor Extension Points (IDemandProvider)

**Type:** infrastructure
**System:** Shared
**Scope:** S
**Priority:** P1 (important)

**Description:**
Define the IDemandProvider interface for pluggable demand calculation. ZoneSystem's basic demand is the default provider. When DemandSystem (Epic 10) is implemented, it replaces the basic calculation via dependency injection. Per CCR-008, this is the canonical handoff pattern.

**Acceptance Criteria:**
- [ ] IDemandProvider abstract interface with get_demand(ZoneType, PlayerID) method
- [ ] StubDemandProvider implementation that returns configurable positive demand (default: 1.0f)
- [ ] ZoneSystem can accept an IDemandProvider* via setter injection
- [ ] When no external provider set, ZoneSystem uses its own basic demand calculation
- [ ] When external provider set, ZoneSystem delegates to it
- [ ] Unit tests for both modes (internal and external provider)

**Dependencies:**
- Blocked by: 4-016 (basic demand as default implementation)
- Blocks: None (extension point for Epic 10)

**Agent Notes:**
- Zone Engineer: Pluggable provider avoids code duplication. DemandSystem (Epic 10) implements IDemandProvider and completely replaces ZoneSystem's basic demand.
- Systems Architect: Same setter injection pattern as other forward dependency stubs.

---

### 4-018: Zone Desirability Calculation

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Calculate per-zone-sector desirability score (0-255) and cache in ZoneComponent.desirability. Factors: terrain value bonus (from ITerrainQueryable.get_value_bonus), pathway proximity (stub: max score), contamination penalty (stub: 0), service coverage (stub: neutral). Update periodically (every 10 ticks, not every tick). Desirability is consumed by BuildingSystem for template selection and by the demand calculation.

**Acceptance Criteria:**
- [ ] Desirability calculated as weighted sum of factors, clamped to 0-255
- [ ] Terrain value bonus from ITerrainQueryable
- [ ] Pathway proximity factor (stub returns maximum in Epic 4)
- [ ] Contamination penalty (stub returns 0 in Epic 4)
- [ ] Service coverage factor (stub returns neutral in Epic 4)
- [ ] Update frequency: every 10 ticks (500ms), not every tick
- [ ] Result cached in ZoneComponent.desirability field
- [ ] update_desirability(x, y, value) method for external systems to contribute
- [ ] Performance: 50K zones at ~10 operations each = <0.5ms per update cycle
- [ ] Unit tests for desirability calculation with various factor combinations

**Dependencies:**
- Blocked by: 4-001 (ZoneComponent.desirability field), 4-008 (ZoneSystem), 4-006 (ZoneGrid)
- Blocks: None (consumed by 4-025 template selection)
- External: Epic 3 ticket 3-014 (ITerrainQueryable.get_value_bonus)

**Agent Notes:**
- Zone Engineer: Desirability is a cached aggregate score that avoids recalculating attractiveness on every query. Updated infrequently since factors change slowly.
- Building Engineer: Template selection uses desirability as a proxy for sector value when ILandValueProvider is stubbed.

---

### Group F: Forward Dependency Interfaces and Stubs

---

### 4-019: Forward Dependency Interface Definitions

**Type:** infrastructure
**System:** Shared
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define the six abstract interfaces for forward dependencies per CCR-006: IEnergyProvider, IFluidProvider, ITransportProvider, ILandValueProvider, IDemandProvider (also in 4-017), ICreditProvider. Each interface declares the minimal query methods BuildingSystem and ZoneSystem need. These interfaces are the contract that later epics implement. Place in shared interfaces directory.

**Acceptance Criteria:**
- [ ] IEnergyProvider: is_powered(EntityID), is_powered_at(x, y, PlayerID)
- [ ] IFluidProvider: has_fluid(EntityID), has_fluid_at(x, y, PlayerID) *(Note: uses "fluid" per interfaces.yaml, not "water")*
- [ ] ITransportProvider: is_road_accessible_at(x, y, max_distance), get_nearest_road_distance(x, y)
- [ ] ILandValueProvider: get_land_value(x, y) *(Note: LandValueSystem implements IGridOverlay; consider using that pattern)*
- [ ] IDemandProvider: get_demand(ZoneType, PlayerID) (shared with 4-017)
- [ ] ICreditProvider: deduct_credits(PlayerID, amount) -> bool, has_credits(PlayerID, amount) -> bool *(Note: consider event-based pattern or direct EconomySystem call)*
- [ ] All interfaces are pure abstract (virtual destructor, pure virtual methods)
- [ ] Interfaces registered in interfaces.yaml
- [ ] Unit tests compile against interfaces (verifying API)

**Dependencies:**
- Blocked by: 4-002 (building type enums used in interface signatures)
- Blocks: 4-020, 4-025, 4-026

**Agent Notes:**
- Building Engineer: Section 8 is the canonical implementation reference. Raw interface pointers with setter injection per Q037.
- Systems Architect: Six stubs total per CCR-006. These enable Epic 5/6/7/10/11 to begin in parallel once interfaces are locked.

---

### 4-020: Forward Dependency Stub Implementations

**Type:** infrastructure
**System:** Shared
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Implement stub classes for all six forward dependency interfaces. All stubs return permissive defaults: energy=always powered, fluid=always has water, transport=always accessible (distance 0), land value=50 (medium), demand=positive, credits=unlimited (always true). Include a debug toggle mode where stubs can return restrictive values for testing the abandonment pipeline.

**Acceptance Criteria:**
- [ ] StubEnergyProvider: is_powered() -> true, is_powered_at() -> true
- [ ] StubFluidProvider: has_fluid() -> true, has_fluid_at() -> true
- [ ] StubTransportProvider: is_road_accessible() -> true, get_nearest_road_distance() -> 0
- [ ] StubLandValueProvider: get_land_value() -> 50
- [ ] StubDemandProvider: get_demand() -> 1.0f (positive)
- [ ] StubCreditProvider: deduct_credits() -> true (always succeeds), has_credits() -> true
- [ ] Debug toggle mode: configurable to return false/restrictive values for state machine testing
- [ ] Unit tests verifying all stubs return expected defaults
- [ ] Unit tests for debug toggle behavior

**Dependencies:**
- Blocked by: 4-019 (interface definitions)
- Blocks: 4-025 (BuildingSystem uses stubs), 4-034 (BuildingSystem integration)

**Agent Notes:**
- Building Engineer: Stubs enable full BuildingSystem development without waiting for Epics 5/6/7/10/11.
- Systems Architect: Debug toggle is critical for testing the full state machine (Active -> Abandoned -> Derelict -> Deconstructed) before real utility systems exist.

---

### Group G: Template System

---

### 4-021: BuildingTemplate Data Structure and Registry

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define the BuildingTemplate struct and BuildingTemplateRegistry class. Template fields: template_id, name, zone_type, density, model_source (Procedural/Asset), model_path, footprint (w, h), construction_cost, construction_ticks, min_land_value, min_level, base_capacity, energy_required, fluid_required, contamination_output, color_accent_count, selection_weight. Registry organized by TemplatePoolKey (zone_type + density). Minimum 5 templates per pool (30 total per CCR-009).

**Acceptance Criteria:**
- [ ] BuildingTemplate struct with all fields defined
- [ ] TemplatePoolKey struct: zone_type + density with equality and hash
- [ ] BuildingTemplateRegistry class with:
  - load(path) for data file loading (or hardcoded initial set)
  - get_template(template_id) -> const BuildingTemplate&
  - get_templates_for_pool(zone_type, density) -> vector of template pointers
  - get_template_count() for validation
- [ ] Pool index: unordered_map<TemplatePoolKey, vector<uint32_t>>
- [ ] Template lookup: unordered_map<uint32_t, BuildingTemplate>
- [ ] Supports initial hardcoded template set for development
- [ ] Unit tests for registry lookup, pool filtering, edge cases (empty pool)

**Dependencies:**
- Blocked by: 4-002 (ZoneBuildingType, DensityLevel enums)
- Blocks: 4-022, 4-023, 4-025

**Agent Notes:**
- Building Engineer: Registry is loaded at startup and immutable during gameplay. Template pools organized by zone_type + density for fast selection.
- Systems Architect: Template data is shared across all overseers. No per-overseer template customization.

---

### 4-022: Template Selection Algorithm

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the weighted random template selection algorithm. Steps: (1) get candidate pool for zone_type + density, (2) filter by sector value (min_land_value <= current value), (3) filter by min_level <= 1 for initial spawn, (4) weight candidates -- base 1.0, land value bonus +0.5, duplicate penalty -0.7 for matching adjacent templates (orthogonal neighbors only), clamp minimum 0.1, (5) weighted random selection using server-side seeded PRNG (seed = hash(tile_x, tile_y, sim_tick) per Q013). Per CCR-010, apply rotation (0/90/180/270) and color_accent_index only -- NO scale variation.

**Acceptance Criteria:**
- [ ] select_template(zone_type, density, land_value, tile_x, tile_y, sim_tick, neighbors) returns template_id + variation
- [ ] Pool filtering by zone_type + density
- [ ] Land value filtering: min_land_value <= current_land_value
- [ ] Level filtering: min_level <= 1 for new spawns
- [ ] Weighting: base(1.0), land_value_bonus(+0.5 if high), duplicate_penalty(-0.7 per adjacent match), minimum(0.1)
- [ ] Neighbor check: 4 orthogonal adjacent sectors only (not diagonal)
- [ ] If all candidates match neighbors, allow duplicates (small pool edge case)
- [ ] Deterministic seeded PRNG: seed = hash(tile_x, tile_y, sim_tick)
- [ ] Variation output: rotation (0-3), color_accent_index (0 to template.color_accent_count-1)
- [ ] NO scale variation (per CCR-010)
- [ ] Unit tests for selection distribution, edge cases, determinism across runs

**Dependencies:**
- Blocked by: 4-021 (template registry), 4-007 (BuildingGrid for neighbor checks)
- Blocks: 4-025

**Agent Notes:**
- Building Engineer: Neighbor duplicate avoidance prevents rows of identical structures. Weight floor of 0.1 ensures no template is fully eliminated.
- Systems Architect: Seeded PRNG ensures server-authoritative deterministic selection. Clients never re-derive (per Q014).
- Game Designer: CCR-010 resolved NO scale variation -- structures must fit their sector footprint exactly.

---

### 4-023: Initial Template Definitions (Content)

**Type:** content
**System:** BuildingSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Author the initial set of 30 structure templates (minimum 5 per zone_type+density bucket per CCR-009). Define construction times, costs, capacities, sector value thresholds, energy/fluid requirements, contamination output, footprint sizes, color accent counts. Low-density structures are 1x1; high-density may be 1x1 or 2x2. Use procedural model source initially (proper assets added later). Capacity ranges per Q039: habitation low 4-12, high 40-200; exchange low 2-6, high 20-80; fabrication low 4-10, high 30-120.

**Acceptance Criteria:**
- [ ] 5 templates for low-density habitation (varied capacities, costs, sector value thresholds)
- [ ] 5 templates for high-density habitation (includes at least one 2x2 footprint)
- [ ] 5 templates for low-density exchange
- [ ] 5 templates for high-density exchange (includes at least one 2x2 footprint)
- [ ] 5 templates for low-density fabrication
- [ ] 5 templates for high-density fabrication (includes at least one 2x2 footprint)
- [ ] Construction times: low density 40-80 ticks (2-4s), high density 100-200 ticks (5-10s)
- [ ] Capacity ranges follow Q039 resolution values
- [ ] Fabrication templates include contamination_output > 0
- [ ] Each template has unique template_id and descriptive name
- [ ] All templates loadable by BuildingTemplateRegistry
- [ ] Templates use canonical alien terminology in names (dwelling, hab-spire, market pod, exchange tower, fabricator pod, fabrication complex)

**Dependencies:**
- Blocked by: 4-021 (template data structure)
- Blocks: None (content can be expanded independently)

**Agent Notes:**
- Building Engineer: 30-47 templates recommended. Start with 30 minimum viable pool. Expand based on visual variety assessment.
- Game Designer: 23-28 was the design recommendation, but CCR-009 resolved to 30 minimum for 5 per bucket.

---

### Group H: Building Spawning

---

### 4-024: Building Spawn Condition Checks

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the can_spawn_building(tile) function that checks ALL spawn preconditions. Conditions: (1) sector has ZoneComponent with valid zone_type, (2) zone demand is positive for this type, (3) sector has no existing structure (BuildingGrid check), (4) terrain is buildable (ITerrainQueryable), (5) pathway within 3 sectors using Chebyshev distance per CCR-007 (stub: always true), (6) utilities available -- energy and fluid (stubs: always true), (7) sector owned by valid overseer. For multi-sector footprints, ALL sectors must satisfy conditions.

**Acceptance Criteria:**
- [ ] can_spawn_building(x, y, player_id) returns bool
- [ ] Checks zone exists and is Designated state
- [ ] Checks demand > 0 for zone_type via ZoneSystem
- [ ] Checks BuildingGrid.is_tile_occupied returns false
- [ ] Checks ITerrainQueryable.is_buildable
- [ ] Checks ITransportProvider.is_road_accessible (stub: true)
- [ ] Checks IEnergyProvider.is_powered_at (stub: true)
- [ ] Checks IFluidProvider.has_fluid_at (stub: true)
- [ ] Checks OwnershipComponent.owner is valid overseer
- [ ] For multi-sector footprint: can_spawn_footprint(x, y, w, h, player_id) checks all sectors
- [ ] Chebyshev distance from nearest footprint edge per CCR-007
- [ ] Unit tests for each condition individually and combined

**Dependencies:**
- Blocked by: 4-007 (BuildingGrid), 4-019 (forward dependency interfaces), 4-020 (stubs)
- Blocks: 4-025
- External: Epic 3 ticket 3-014 (ITerrainQueryable)

**Agent Notes:**
- Zone Engineer: Pathway proximity gates development, not designation (Q017). Chebyshev distance of 3 = 7x7 square (CCR-007).
- Building Engineer: Multi-sector footprint check ensures ALL sectors are valid. Strict validation prevents partial placement.

---

### 4-025: Building Entity Creation

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement spawn_building(tile, template) function that creates a complete structure entity. Entity composition: PositionComponent, BuildingComponent (initialized from template + variation), ConstructionComponent (initialized with template construction_ticks), OwnershipComponent (inherited from zone sector owner). Register in BuildingGrid (all footprint sectors). Set ZoneComponent.state to Occupied via ZoneSystem.set_zone_state(). Per Q014, the server sends BuildingSpawnedMessage with template_id, rotation, and color_accent -- clients do NOT re-derive from RNG.

**Acceptance Criteria:**
- [ ] spawn_building creates entity with: PositionComponent, BuildingComponent, ConstructionComponent, OwnershipComponent
- [ ] BuildingComponent initialized: template_id, zone_type, density, state=Materializing, level=1, health=100, capacity from template, rotation and color_accent from selection
- [ ] ConstructionComponent initialized: ticks_total from template, ticks_elapsed=0, phase=Foundation, construction_cost from template
- [ ] OwnershipComponent.owner inherited from zone sector owner
- [ ] BuildingGrid.set_footprint(x, y, w, h, entity_id) for all footprint sectors
- [ ] ZoneSystem.set_zone_state(x, y, Occupied) called
- [ ] Entity ID returned for further processing
- [ ] Unit tests for entity creation, component initialization, grid registration

**Dependencies:**
- Blocked by: 4-003 (BuildingComponent), 4-004 (ConstructionComponent), 4-007 (BuildingGrid), 4-010 (events), 4-022 (template selection), 4-021 (template registry), 4-016 (demand for spawn gating)
- Blocks: 4-026, 4-034

**Agent Notes:**
- Building Engineer: Entity creation is the core output of the spawning pipeline. All components initialized atomically.
- Systems Architect: Server-authoritative -- clients receive the completed entity via SyncSystem, including template_id and variation parameters.

---

### 4-026: Building Spawning Loop

**Type:** feature
**System:** BuildingSystem
**Scope:** XL
**Priority:** P0 (critical path)

**Description:**
Implement the core per-tick spawning scan in BuildingSystem.tick(). Every SPAWN_SCAN_INTERVAL ticks (default: 20 = 1 second), scan designated zones per overseer. For each overseer: iterate their designated zones (shuffled for fairness), check spawn conditions, select template, create entity, cap at MAX_SPAWNS_PER_SCAN per overseer (default: 3). Stagger scans across overseers (offset by player_id * 5 ticks). Actual spawn count gated by demand strength per Q003 resolution.

**Acceptance Criteria:**
- [ ] Spawning scan runs every SPAWN_SCAN_INTERVAL ticks (configurable, default: 20)
- [ ] Per-overseer processing with player_id-based stagger
- [ ] Maximum MAX_SPAWNS_PER_SCAN per overseer per scan (configurable, default: 3)
- [ ] Actual spawn count scaled by demand magnitude (high demand = more spawns, low demand = fewer)
- [ ] Zone iteration order shuffled each scan for fairness
- [ ] Calls can_spawn_building, select_template, spawn_building pipeline
- [ ] Priority ordering: sectors closest to pathways spawned first (stub: all equal priority)
- [ ] BuildingSystem.tick() integrates spawning with other per-tick work
- [ ] Performance: scan of 10,000 zones < 2ms
- [ ] Integration test: zones with positive demand spawn structures over multiple ticks

**Dependencies:**
- Blocked by: 4-025 (entity creation), 4-024 (spawn conditions), 4-022 (template selection), 4-010 (events)
- Blocks: 4-034

**Agent Notes:**
- Building Engineer: Scan interval + per-overseer cap prevents explosion of simultaneous constructions. Stagger distributes server load.
- Game Designer: Target 15-20 structures in first 5 minutes. At 3/scan, demand-gating keeps early game fast and late game steady.
- Systems Architect: Demand-gated spawn count is the key tuning knob for colony growth pacing.

---

### Group I: Construction System

---

### 4-027: Construction Progress System

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Each tick, increment ConstructionComponent.ticks_elapsed for all entities in Materializing state. Update ConstructionPhase based on progress percentage (Foundation 0-25%, Framework 25-50%, Exterior 50-75%, Finalization 75-100%). Update phase_progress (0-255) within current phase for smooth rendering interpolation. Handle is_paused flag (no increment when paused). When ticks_elapsed >= ticks_total: remove ConstructionComponent, transition BuildingComponent.state to Active, emit BuildingConstructedEvent.

**Acceptance Criteria:**
- [ ] Each tick: ticks_elapsed++ for all non-paused entities with ConstructionComponent
- [ ] Phase derived from progress: (ticks_elapsed / ticks_total) mapped to 4 phases
- [ ] phase_progress calculated as 0-255 within current phase quarter
- [ ] is_paused prevents increment
- [ ] Completion detection: ticks_elapsed >= ticks_total
- [ ] On completion: ConstructionComponent removed, BuildingState = Active, BuildingConstructedEvent emitted
- [ ] state_changed_tick updated on completion
- [ ] Unit tests for progress increments, phase transitions, completion, paused behavior

**Dependencies:**
- Blocked by: 4-004 (ConstructionComponent), 4-003 (BuildingComponent), 4-010 (events)
- Blocks: 4-028, 4-034

**Agent Notes:**
- Building Engineer: Construction is the entry path for every structure. phase_progress enables RenderingSystem to smoothly interpolate the materializing animation.
- Game Designer: Materializing should take 2-10 seconds real time. Construction glow should be brighter than normal active glow to grab attention.

---

### Group J: Building State Machine

---

### 4-028: Building State Transition System

**Type:** feature
**System:** BuildingSystem
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement the full 5-state building lifecycle per CCR-001. Active -> Abandoned: when essential services lost for grace period ticks (default 100 ticks / 5 seconds). Abandoned -> Active: services restored before timer expires. Abandoned -> Derelict: abandon_timer reaches 0 (default 200 ticks / 10 seconds). Derelict -> Deconstructed: derelict timer expires (default 500 ticks / 25 seconds) OR overseer demolishes. Deconstructed -> Entity removed: debris auto-clears (60 ticks / 3 seconds). Per-tick evaluation for Active and Abandoned entities. All timers configurable.

**Acceptance Criteria:**
- [ ] Active -> Abandoned: triggered when is_powered/has_fluid/is_road_accessible returns false for grace_period ticks
- [ ] Grace period tracking per entity (not global timer)
- [ ] Abandoned -> Active: all services restored, timer reset, BuildingRestoredEvent emitted
- [ ] Abandoned -> Derelict: abandon_timer reaches 0, BuildingDerelictEvent emitted
- [ ] Derelict structures CANNOT transition back to Active (must be deconstructed)
- [ ] Derelict -> Deconstructed: after configurable timer (500 ticks), BuildingDeconstructedEvent emitted
- [ ] Deconstructed -> Entity removal: after debris clear timer (60 ticks), DebrisClearedEvent emitted
- [ ] All state transitions update BuildingComponent.state and state_changed_tick
- [ ] All timers are configurable constants
- [ ] Integration test: full lifecycle spawn -> construct -> active -> abandon -> derelict -> deconstructed -> cleared

**Dependencies:**
- Blocked by: 4-027 (construction completion triggers Active state), 4-010 (events), 4-019 (interface for utility queries)
- Blocks: 4-029, 4-034

**Agent Notes:**
- Building Engineer: State machine is the core lifecycle management. Grace period prevents flickering during brief outages.
- Game Designer: Decline should feel like a signal ("fix this!"), not punishment. Gradual visual degradation provides comfortable reaction window.
- Systems Architect: With stubs returning true, Active -> Abandoned never triggers in Epic 4. Debug toggle on stubs enables testing.

---

### 4-029: Abandon Grace Period Logic

**Type:** feature
**System:** BuildingSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Track per-structure utility loss duration. Only transition Active -> Abandoned after a configurable grace period of sustained utility loss (default: 100 ticks / 5 seconds). Reset grace counter immediately when utilities are restored. This prevents flickering between Active and Abandoned on brief outages (brownouts, temporary pathway disruption). Separate counters for energy, fluid, and pathway.

**Acceptance Criteria:**
- [ ] Per-entity grace counter tracking continuous ticks without each essential service
- [ ] Grace period configurable (default: 100 ticks / 5 seconds for energy/fluid, immediate for pathway loss)
- [ ] Counter increments each tick while service is unavailable
- [ ] Counter resets to 0 when service is restored
- [ ] Transition to Abandoned only when ANY counter exceeds threshold
- [ ] Pathway loss has no grace period (immediate abandon per Building Engineer analysis)
- [ ] Unit tests for grace period with intermittent outages (flickering power)
- [ ] Unit test for immediate pathway-loss abandonment

**Dependencies:**
- Blocked by: 4-028 (state transition system)
- Blocks: None

**Agent Notes:**
- Building Engineer: Grace period is critical UX. Brief brownout should not cascade into mass abandonment.
- Game Designer: Duration tuned so brief 2-second outage causes no harm, but sustained 5-second outage signals a real problem.

---

### Group K: Demolition and Debris

---

### 4-030: Overseer Demolition Handler

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Handle overseer-initiated structure demolition. Validate: entity exists, has BuildingComponent, overseer owns it, structure is not already Deconstructed. Calculate cost: base = template.construction_cost * 0.25, multiplied by state modifier (Materializing=0.5, Active=1.0, Abandoned=0.1, Derelict=0.0 free). Debit credits via ICreditProvider (stub: always succeeds). Transition to Deconstructed: clear BuildingGrid for all footprint sectors, create DebrisComponent. Emit BuildingDeconstructedEvent. Also handle DemolitionRequestEvent from ZoneSystem (per CCR-012 de-zone flow).

**Acceptance Criteria:**
- [ ] handle_demolish_request(entity_id, player_id) validates and executes
- [ ] Ownership validation: OwnershipComponent.owner == requesting player
- [ ] State validation: not already Deconstructed
- [ ] Cost calculation: base(25% of construction cost) * state modifier (Materializing=0.5, Active=1.0, Abandoned=0.1, Derelict=0.0)
- [ ] Credit deduction via ICreditProvider (stub: always succeeds)
- [ ] Reject if insufficient credits (when real EconomySystem exists)
- [ ] BuildingGrid cleared for all footprint sectors
- [ ] DebrisComponent added to entity with clear_timer
- [ ] BuildingComponent.state = Deconstructed
- [ ] BuildingDeconstructedEvent emitted with was_player_initiated = true
- [ ] DemolitionRequestEvent handler: processes demolition requests from ZoneSystem de-zone flow
- [ ] After demolition from de-zone: calls ZoneSystem.set_zone_state(Designated) then ZoneSystem destroys zone entity
- [ ] Unit tests for cost calculation, ownership validation, grid cleanup, event emission

**Dependencies:**
- Blocked by: 4-005 (DebrisComponent), 4-007 (BuildingGrid), 4-010 (events), 4-019 (ICreditProvider)
- Blocks: 4-031, 4-034

**Agent Notes:**
- Building Engineer: Demolition cost scales with structure value -- derelict structures are free to clear, incentivizing cleanup.
- Game Designer: Derelict structure clearance being free encourages overseers to maintain their colony rather than leaving dark structures.
- Zone Engineer: DemolitionRequestEvent from de-zone flow uses the same demolition handler, maintaining decoupled architecture per CCR-012.

---

### 4-031: Debris Auto-Clear System

**Type:** feature
**System:** BuildingSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Each tick, decrement DebrisComponent.clear_timer for all debris entities. When timer reaches 0: remove entity from ECS, clear any remaining BuildingGrid entries, emit DebrisClearedEvent. Also handle overseer-initiated immediate debris clearing (ClearDebrisMessage) at nominal cost (configurable, default: 10 credits). Sector returns to development eligibility after debris cleared.

**Acceptance Criteria:**
- [ ] Each tick: clear_timer-- for all entities with DebrisComponent
- [ ] On timer expiry: entity removed, BuildingGrid cleared, DebrisClearedEvent emitted
- [ ] Manual clear: handle_clear_debris(entity_id, player_id) with configurable cost (default: 10 credits)
- [ ] Sector returns to zone Designated state (eligible for new structure spawning)
- [ ] Unit tests for auto-clear timing, manual clear cost, grid cleanup

**Dependencies:**
- Blocked by: 4-005 (DebrisComponent), 4-010 (DebrisClearedEvent)
- Blocks: None

**Agent Notes:**
- Building Engineer: Short debris timer (3 seconds) prevents sectors from being blocked indefinitely. Manual clear provides impatient overseers an option.

---

### Group L: Building Upgrades and Downgrades

---

### 4-032: Building Upgrade System

**Type:** feature
**System:** BuildingSystem
**Scope:** L
**Priority:** P1 (important)

**Description:**
Implement structure level upgrade logic. Conditions: structure is Active, all utilities connected (stubs: true), sector value meets next-level template threshold, time at current level exceeds UPGRADE_COOLDOWN (default: 200 ticks / 10 seconds per Q041), demand is positive. On upgrade: level++, template may change (re-selection filtered by min_level), capacity scales (base_capacity * level_multiplier), brief upgrade animation via temporary ConstructionComponent (~20 ticks / 1 second per Q006). Occupants remain during upgrade. Emit BuildingUpgradedEvent.

**Acceptance Criteria:**
- [ ] Upgrade conditions checked periodically (not every tick -- every 10 ticks)
- [ ] All conditions must be true: Active state, utilities connected, sector value threshold, cooldown elapsed, demand positive
- [ ] Level incremented (capped at template max_level)
- [ ] Template re-selection for new level using min_level filter
- [ ] Capacity recalculated: template.base_capacity * level_multiplier[level]
- [ ] Level multipliers configurable (default: [1.0, 1.5, 2.0, 2.5, 3.0])
- [ ] Brief upgrade animation: temporary ConstructionComponent with ~20 ticks duration
- [ ] Structure remains Active during upgrade animation (occupants stay per Q006)
- [ ] Model swap covered by ConstructionComponent materializing effect (per Q044)
- [ ] BuildingUpgradedEvent emitted
- [ ] UPGRADE_COOLDOWN configurable (default: 200 ticks)
- [ ] Unit tests for upgrade conditions, level cap, capacity scaling

**Dependencies:**
- Blocked by: 4-028 (state machine -- must be Active), 4-021 (templates for level filtering), 4-010 (events)
- Blocks: 4-034

**Agent Notes:**
- Building Engineer: Upgrade animation uses temporary ConstructionComponent to cover model swap. This reuses the materializing visual pipeline.
- Game Designer: Upgrade cooldown prevents rapid oscillation. Consider shorter cooldown for first upgrade (per Q041 resolution).

---

### 4-033: Building Downgrade System

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P2 (enhancement)

**Description:**
Implement structure level downgrade logic. Triggers: sector value drops below current level threshold, sustained negative demand. On downgrade: level--, may revert to lower-level template, capacity decreases (excess occupants displaced by PopulationSystem in Epic 10). Brief downgrade animation (~20 ticks). No credit cost for downgrades. Emit BuildingDowngradedEvent.

**Acceptance Criteria:**
- [ ] Downgrade triggered when sector value drops below min_land_value for current level
- [ ] Downgrade triggered by sustained negative demand for DOWNGRADE_DELAY ticks
- [ ] Level decremented (minimum 1)
- [ ] Template may revert to lower-level variant
- [ ] Capacity recalculated using same formula as upgrade
- [ ] Brief animation via temporary ConstructionComponent
- [ ] No credit cost for downgrades
- [ ] BuildingDowngradedEvent emitted
- [ ] Unit tests for downgrade triggers, minimum level, capacity reduction

**Dependencies:**
- Blocked by: 4-032 (upgrade system provides the level infrastructure)
- Blocks: None

**Agent Notes:**
- Building Engineer: Downgrade is the inverse of upgrade. Minimal complexity addition since it reuses the same level/template infrastructure.
- Game Designer: Downgrades create visible feedback that conditions are deteriorating, motivating the overseer to act.

---

### Group M: BuildingSystem Integration

---

### 4-034: BuildingSystem Class (ISimulatable Integration)

**Type:** feature
**System:** BuildingSystem
**Scope:** XL
**Priority:** P0 (critical path)

**Description:**
Assemble the complete BuildingSystem class implementing ISimulatable at priority 40. Wire together: spawning loop (4-026), construction progress (4-027), state machine (4-028), upgrades (4-032), demolition (4-030), debris (4-031). Dependency injection for all six forward dependency providers (energy, fluid, transport, land value, demand, credits). tick() orchestrates all subsystems. Maintain per-state entity lists and type counters for O(1) aggregate queries.

**Acceptance Criteria:**
- [ ] BuildingSystem implements ISimulatable with get_priority() = 40
- [ ] Constructor accepts: IEnergyProvider*, IFluidProvider*, ITransportProvider*, ILandValueProvider*, IDemandProvider*, ICreditProvider*
- [ ] Setter injection for each provider: set_energy_provider(), etc.
- [ ] tick() calls spawning scan, construction progress, state evaluation, debris timer in correct order
- [ ] Maintains per-state entity lists (materializing, active, abandoned, derelict)
- [ ] Maintains per-overseer per-type counters for O(1) aggregate queries
- [ ] Per-state lists updated on every state transition
- [ ] Performance: tick() < 2ms at 10,000 structures
- [ ] Integration tests for full pipeline: zone -> spawn -> construct -> active

**Dependencies:**
- Blocked by: 4-003 (BuildingComponent), 4-007 (BuildingGrid), 4-010 (events), 4-020 (stubs), 4-025 (entity creation), 4-026 (spawning), 4-027 (construction), 4-028 (state machine), 4-030 (demolition)
- Blocks: 4-035, 4-040

**Agent Notes:**
- Building Engineer: This is the integration ticket that assembles all BuildingSystem subsystems. Priority 40 ensures it runs after ZoneSystem (30).
- Systems Architect: Per Q010, forward dependency interfaces should be locked early so Epic 5 can begin in parallel.

---

### Group N: Query APIs

---

### 4-035: IZoneQueryable Interface and Implementation

**Type:** infrastructure
**System:** ZoneSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Define and implement the IZoneQueryable interface -- the formal query contract ZoneSystem exposes to downstream systems. Methods: get_zone_type(x,y), get_zone_density(x,y), is_zoned(x,y), get_zone_count(player, type), get_zoned_tiles_without_buildings(player, type), get_demand(type, player). ZoneSystem implements this interface. Register in interfaces.yaml.

**Acceptance Criteria:**
- [ ] IZoneQueryable abstract interface defined with all methods
- [ ] get_zone_type(x, y) -> optional<ZoneType> (None if not zoned)
- [ ] get_zone_density(x, y) -> optional<ZoneDensity>
- [ ] is_zoned(x, y) -> bool
- [ ] get_zone_count(player_id, type) -> uint32_t (from cached ZoneCounts)
- [ ] get_zoned_tiles_without_buildings(player_id, type) -> vector<GridPosition> (Designated state zones)
- [ ] get_demand(type, player_id) -> float
- [ ] ZoneSystem implements IZoneQueryable
- [ ] Interface registered in interfaces.yaml
- [ ] Unit tests for all query methods with various zone configurations

**Dependencies:**
- Blocked by: 4-008 (ZoneSystem), 4-006 (ZoneGrid), 4-001 (types)
- Blocks: None (consumed by downstream epics)

**Agent Notes:**
- Systems Architect: IZoneQueryable is consumed by 6+ downstream systems. Interface stability is critical.
- Zone Engineer: Backed by ZoneGrid for spatial queries and ZoneCounts for aggregate queries. All O(1) or O(zone_count).

---

### 4-036: IBuildingQueryable Interface and Implementation

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** L
**Priority:** P1 (important)

**Description:**
Define and implement IBuildingQueryable -- the formal query contract BuildingSystem exposes to downstream systems. Spatial queries (get_building_at, is_tile_occupied, is_footprint_available, get_buildings_in_rect), ownership queries (get_buildings_by_owner, get_building_count_by_owner), state queries (get_building_state, get_buildings_by_state), type queries (get_building_count_by_type, get_building_count_by_type_and_owner), capacity queries (get_total_capacity, get_total_occupancy), statistics (get_building_stats). Register in interfaces.yaml.

**Acceptance Criteria:**
- [ ] IBuildingQueryable abstract interface defined with all methods from Building Engineer Section 9
- [ ] get_building_at(x, y) returns EntityID via O(1) BuildingGrid lookup
- [ ] is_tile_occupied(x, y) via BuildingGrid
- [ ] is_footprint_available(x, y, w, h) via BuildingGrid
- [ ] get_buildings_in_rect(rect) iterates BuildingGrid sub-rect
- [ ] get_buildings_by_owner(player_id) from per-overseer entity list
- [ ] get_building_count_by_type(type) from cached counters -- O(1)
- [ ] get_total_capacity(type, player_id) and get_total_occupancy(type, player_id)
- [ ] BuildingStats struct with totals per state
- [ ] BuildingSystem implements IBuildingQueryable
- [ ] Interface registered in interfaces.yaml
- [ ] Unit tests for all query methods

**Dependencies:**
- Blocked by: 4-034 (BuildingSystem with internal data structures), 4-007 (BuildingGrid), 4-003 (BuildingComponent)
- Blocks: None (consumed by 8+ downstream systems in Epics 5-13)

**Agent Notes:**
- Systems Architect: IBuildingQueryable is consumed by the most downstream systems of any Epic 4 interface. Stability is paramount.
- Building Engineer: Per-overseer entity lists and type counters maintained incrementally for O(1) aggregate queries.

---

### 4-037: IBuildingTemplateQuery Interface

**Type:** infrastructure
**System:** BuildingSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Define and implement IBuildingTemplateQuery -- allows downstream systems to query template metadata. Methods: get_template(template_id), get_templates_for_zone(type, density), get_energy_required(template_id), get_fluid_required(template_id), get_population_capacity(template_id), get_job_capacity(template_id). Backed by BuildingTemplateRegistry. Register in interfaces.yaml.

**Acceptance Criteria:**
- [ ] IBuildingTemplateQuery abstract interface defined
- [ ] get_template(template_id) returns const BuildingTemplate&
- [ ] get_templates_for_zone(type, density) returns template list
- [ ] Convenience methods for common queries (energy, fluid, capacity)
- [ ] BuildingSystem or BuildingTemplateRegistry implements the interface
- [ ] Interface registered in interfaces.yaml
- [ ] Unit tests for template queries

**Dependencies:**
- Blocked by: 4-021 (BuildingTemplateRegistry)
- Blocks: None (consumed by Epics 5, 6, 10, 11, 12)

**Agent Notes:**
- Systems Architect: Downstream systems (EnergySystem, FluidSystem, PopulationSystem) need template metadata to calculate consumption requirements.

---

### Group O: Network and Serialization

---

### 4-038: Zone Network Messages

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Define and implement zone-related network messages. Client -> Server: ZonePlacementRequestMsg (area, type, density), DezoneRequestMsg (area), RedesignateRequestMsg (position, new type, new density). Server validates via ZoneSystem placement/removal methods, broadcasts results via SyncSystem. Implement ISerializable for all messages. Zone entity creation/destruction syncs through standard ECS delta (Epic 1).

**Acceptance Criteria:**
- [ ] ZonePlacementRequestMsg: GridRect area, ZoneType, ZoneDensity (PlayerID inferred from connection)
- [ ] DezoneRequestMsg: GridRect area
- [ ] RedesignateRequestMsg: position(x,y), new ZoneType, new ZoneDensity
- [ ] All messages implement ISerializable (fixed-size, little-endian, versioned)
- [ ] Server-side handler dispatches to ZoneSystem methods
- [ ] Server validates per tick, broadcasts results via SyncSystem
- [ ] Zone component changes sync as standard ECS deltas
- [ ] ZoneDemandData synced periodically (every ~10 ticks)
- [ ] Unit tests for serialization round-trips

**Dependencies:**
- Blocked by: 4-012 (zone placement execution), 4-013 (de-zoning), 4-008 (ZoneSystem), 4-006 (ZoneGrid)
- Blocks: 4-039
- External: Epic 1 (ISerializable, SyncSystem)

**Agent Notes:**
- Zone Engineer: Network messages are thin request wrappers. All logic lives in ZoneSystem. SyncSystem handles state broadcast.
- Systems Architect: No client-side prediction for zoning. Client sends request, waits for server confirmation (~150ms acceptable for city builder).

---

### 4-039: Zone Server-Authoritative Validation Pipeline

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement the server-side message handler for zone operations. Server receives zone messages, validates ownership per-sector for multi-sector requests, applies changes via ZoneSystem, broadcasts results via SyncSystem. Concurrent request handling: requests from different overseers processed sequentially within a tick (no conflicts due to sector ownership). Atomic per-request: each ZonePlacementRequest processed atomically with partial failure support.

**Acceptance Criteria:**
- [ ] Server receives ZonePlacementRequestMsg, validates, calls ZoneSystem.place_zones()
- [ ] Server receives DezoneRequestMsg, validates, calls ZoneSystem.remove_zones()
- [ ] Server receives RedesignateRequestMsg, validates, calls ZoneSystem.redesignate_zone()
- [ ] Ownership validated per-sector for multi-sector operations
- [ ] Results broadcast to all clients via SyncSystem
- [ ] Rejection responses sent back to requesting client with reason
- [ ] Sequential processing within a tick prevents race conditions
- [ ] Unit tests for validation rejection cases

**Dependencies:**
- Blocked by: 4-038 (zone messages)
- Blocks: None
- External: Epic 1 (SyncSystem)

**Agent Notes:**
- Zone Engineer: Server processes messages before simulation tick. No concurrent access issues due to single-threaded tick processing.
- Systems Architect: Sector ownership prevents conflicts between overseers. Each overseer can only zone their own sectors.

---

### 4-040: Building Network Messages

**Type:** feature
**System:** BuildingSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Define and implement structure-related network messages. Client -> Server: DemolishRequestMessage (entity_id), ClearDebrisMessage (entity_id). Server -> Client: BuildingSpawnedMessage (entity_id, position, template_id, owner, rotation, color_accent per Q014), BuildingStateChanged, BuildingUpgraded, ConstructionProgress (every 10 ticks for animation interpolation), BuildingDemolished, DebrisCleared. All messages implement ISerializable.

**Acceptance Criteria:**
- [ ] DemolishRequestMessage: entity_id (PlayerID inferred from connection)
- [ ] ClearDebrisMessage: entity_id
- [ ] BuildingSpawnedMessage: entity_id, position, template_id, owner, rotation, color_accent_index
- [ ] BuildingStateChanged: entity_id, new_state
- [ ] BuildingUpgraded: entity_id, new_level, new_template_id
- [ ] ConstructionProgress: entity_id, ticks_elapsed, ticks_total (synced every 10 ticks)
- [ ] BuildingDemolished: entity_id
- [ ] DebrisCleared: entity_id, position
- [ ] All messages implement ISerializable (fixed-size, little-endian, versioned)
- [ ] Client-side interpolation of construction progress between server updates
- [ ] Unit tests for serialization round-trips

**Dependencies:**
- Blocked by: 4-034 (BuildingSystem integration), 4-003 (BuildingComponent)
- Blocks: None
- External: Epic 1 (ISerializable, SyncSystem)

**Agent Notes:**
- Building Engineer: Structure spawning is server-only -- no client request needed. Server decides when conditions are met and broadcasts.
- Systems Architect: Construction progress synced every 10 ticks (0.5s). Clients interpolate for smooth animation.

---

### 4-041: Zone Serialization (Save/Load)

**Type:** feature
**System:** ZoneSystem
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement ISerializable for ZoneComponent and ZoneGrid data. Fixed-size types, little-endian, versioned format. Includes all zone entity data, ZoneGrid spatial index state, ZoneCounts, demand data. For save/load (Epic 16) and full snapshot network transfer.

**Acceptance Criteria:**
- [ ] ZoneComponent implements ISerializable with version field
- [ ] ZoneGrid serialization: EntityID array + metadata (dimensions)
- [ ] ZoneCounts serialization for per-overseer state
- [ ] ZoneDemandData serialization
- [ ] Fixed-size types with explicit endianness (little-endian)
- [ ] Round-trip test: serialize -> deserialize -> compare original = result
- [ ] Format versioned for forward compatibility
- [ ] Unit tests for round-trip validation

**Dependencies:**
- Blocked by: 4-001 (ZoneComponent), 4-006 (ZoneGrid)
- Blocks: None
- External: Epic 1 (ISerializable interface)

**Agent Notes:**
- Zone Engineer: Serialization enables save/load and snapshot-based network join.

---

### 4-042: Building Serialization (Save/Load)

**Type:** feature
**System:** BuildingSystem
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement ISerializable for BuildingComponent, ConstructionComponent, and DebrisComponent. Fixed-size types, little-endian, versioned format with reserved bytes for future fields (health expansion for Epic 13, service flags for Epic 9). Round-trip testing required.

**Acceptance Criteria:**
- [ ] BuildingComponent implements ISerializable with version field
- [ ] ConstructionComponent implements ISerializable
- [ ] DebrisComponent implements ISerializable
- [ ] Fixed-size types, little-endian
- [ ] Reserved padding bytes for anticipated future fields
- [ ] New fields get default values when deserializing old formats
- [ ] Round-trip tests for all three components
- [ ] Unit tests for version compatibility

**Dependencies:**
- Blocked by: 4-003 (BuildingComponent), 4-004 (ConstructionComponent), 4-005 (DebrisComponent)
- Blocks: None
- External: Epic 1 (ISerializable interface)

**Agent Notes:**
- Building Engineer: Serialization versioning is critical for forward compatibility as BuildingComponent evolves with later epics.

---

### Group P: Design Deliverables

---

### 4-043: Zone Color Specification and Overlay Design

**Type:** design
**System:** Shared
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Define exact color values for all zone overlays, glow accents, and visual states in the bioluminescent palette. Zone overlay tints for designation preview and active zones. Three zone types are instantly distinguishable: habitation = cyan/teal, exchange = amber/gold, fabrication = magenta/pink. Stalled zones shown as dimmed/desaturated with prerequisite icons. Include hex/RGB values for each zone type at each state (empty, materializing, active, declining, derelict). This spec feeds into RenderingSystem and UISystem.

**Acceptance Criteria:**
- [ ] Color values (hex/RGB) for each zone type overlay tint: habitation (cyan), exchange (amber), fabrication (magenta)
- [ ] Stalled zone visual: dimmed/desaturated overlay + prerequisite icon description
- [ ] Glow accent colors per zone type per state (materializing, active, declining, derelict)
- [ ] Colors consistent across all contexts: zone overlays, structure glow, sector scan (minimap), UI icons
- [ ] Color language documented for RenderingSystem and UISystem implementers
- [ ] No two zone types share primary hue

**Dependencies:**
- Blocked by: None (design specification)
- Blocks: None (consumed by RenderingSystem/UISystem in Epics 2/12)

**Agent Notes:**
- Game Designer: Three-color system (cyan/amber/magenta) provides instant visual readability. Emotional register: habitation = inviting, exchange = prosperous, fabrication = powerful.

---

### 4-044: Materialization Animation Specification

**Type:** design
**System:** Shared
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Define timing, visual phases, and feel targets for structure materialization per CCR-011 (4 phases: Foundation/Framework/Exterior/Finalization). Describe the visual treatment per zone type. Foundation = glow point emergence; Framework = structural skeleton forms; Exterior = shell crystallizes; Finalization = glow settles to active level with completion pulse. Specify timing, color behavior, and emotional targets. This spec feeds into RenderingSystem shader work.

**Acceptance Criteria:**
- [ ] Visual description for each of 4 construction phases
- [ ] Per-zone-type variation (habitation = organic growth, exchange = unfurling, fabrication = angular assembly)
- [ ] Timing targets: low density 2-4 seconds, high density 5-10 seconds
- [ ] Construction glow intensity: brighter than active glow (attention-grabbing)
- [ ] Completion pulse: brief brightening that signals "now active"
- [ ] Phase-to-visual mapping for RenderingSystem implementation
- [ ] Feel targets documented: wonder, organic growth, colony is alive

**Dependencies:**
- Blocked by: None (design specification)
- Blocks: None (consumed by RenderingSystem)

**Agent Notes:**
- Game Designer: Materializing should be one of the most visually delightful moments. This is not human construction -- it is alien emergence.
- Building Engineer: 4-phase enum (Foundation/Framework/Exterior/Finalization) maps to 0-25%, 25-50%, 50-75%, 75-100%.

---

### 4-045: Demand Tuning Parameters

**Type:** design
**System:** ZoneSystem
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document initial values for all demand factors and tuning parameters. Base pressures per zone type, stub factor values, saturation formula constants, soft cap threshold, demand-to-spawn-rate mapping, upgrade cooldowns. These are starting points for playtesting -- all configurable. Include expected pacing targets: 15-20 structures in first 5 minutes, steady growth in mid-game, plateau in late-game.

**Acceptance Criteria:**
- [ ] Base pressure values documented: H=+10, E=+5, F=+5
- [ ] Stub factor values documented per zone type
- [ ] Supply saturation formula and constants
- [ ] Soft cap threshold (+80) and diminishing returns formula
- [ ] Spawn rate mapping: demand magnitude -> spawns per scan
- [ ] Pacing targets: structures per minute at various colony stages
- [ ] Upgrade cooldown values
- [ ] All values in a single tuning document for easy playtesting iteration
- [ ] Configurable constant names mapped to tuning values

**Dependencies:**
- Blocked by: None (design specification)
- Blocks: None (consumed by 4-016, 4-026, 4-032 implementations)

**Agent Notes:**
- Game Designer: These are starting points. Expect significant revision during playtesting.
- Zone Engineer: All constants exposed as configurable values for rapid tuning iteration.

---

### 4-046: Building Template Visual Briefs

**Type:** design
**System:** BuildingSystem
**Scope:** M
**Priority:** P2 (enhancement)

**Description:**
For each of the 30 templates, provide a visual brief describing silhouette, glow character, mood, and zone type identity. Briefs guide future 3D model creation and immediate procedural geometry generation. Include sector value tier distinctions (basic/standard/premium templates). Use canonical alien terminology (dwelling not house, hab-spire not apartment, market pod not shop, fabricator pod not factory).

**Acceptance Criteria:**
- [ ] 30 template briefs (5 per zone_type+density bucket)
- [ ] Each brief: name, silhouette description, glow character, mood, height range
- [ ] Sector value tier assignment per template (poor/moderate/wealthy)
- [ ] Zone type visual identity: habitation = organic/rounded, exchange = outward-facing/displaying, fabrication = angular/industrial
- [ ] Low vs. high density visual distinction documented
- [ ] Canonical alien terminology throughout
- [ ] Reference imagery or mood descriptions for art pipeline

**Dependencies:**
- Blocked by: None (design specification)
- Blocks: None (consumed by art pipeline and 4-023 template content)

**Agent Notes:**
- Game Designer: Each template should be visually distinct, not just color variants. The alien aesthetic means organic shapes, bioluminescent materials, asymmetric profiles.

---

### Group Q: Integration Testing

---

### 4-047: Integration Test: Zone-to-Building Pipeline

**Type:** feature
**System:** Shared
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
End-to-end integration test covering the complete zone-to-building pipeline. Test sequence: (1) designate zone sector, verify zone entity created with correct components, (2) verify demand is positive, (3) wait for building spawn scan, verify structure entity created with correct template and components, (4) verify construction progress increments, (5) verify state transition to Active on completion, (6) verify BuildingConstructedEvent emitted, (7) verify ZoneState transitions to Occupied. Also test: demolition flow, de-zone with occupied sector flow (CCR-012), and full lifecycle to entity removal.

**Acceptance Criteria:**
- [ ] Test: zone designation -> zone entity created with ZoneComponent + PositionComponent + OwnershipComponent
- [ ] Test: positive demand -> structure spawns on designated zone within scan interval
- [ ] Test: construction progress increments each tick, phases transition correctly
- [ ] Test: construction completes -> ConstructionComponent removed, state = Active, event emitted
- [ ] Test: ZoneState transitions from Designated to Occupied on structure placement
- [ ] Test: overseer demolition -> Deconstructed -> debris -> entity removed
- [ ] Test: de-zone occupied sector -> DemolitionRequestEvent -> demolition -> zone entity destroyed (CCR-012 flow)
- [ ] Test: full lifecycle: zone -> spawn -> construct -> active -> abandon (debug stub) -> derelict -> deconstructed -> cleared
- [ ] Test: multi-sector footprint structure placement and cleanup
- [ ] All tests use stub providers (permissive defaults)

**Dependencies:**
- Blocked by: 4-034 (BuildingSystem), 4-008 (ZoneSystem), 4-012 (zone placement), 4-026 (spawning), 4-028 (state machine)
- Blocks: None

**Agent Notes:**
- Systems Architect: This is the critical validation that the zone-to-building pipeline works end-to-end. Must pass before Epic 4 is considered complete.

---

### 4-048: ISimulatable Registration for ZoneSystem and BuildingSystem

**Type:** infrastructure
**System:** Shared
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Register both ZoneSystem (priority 30) and BuildingSystem (priority 40) as ISimulatable implementations in the simulation tick pipeline. Ensure correct execution order: zones process before structures each tick. Verify tick ordering with TerrainSystem (priority 5), EnergySystem stub (priority 10), FluidSystem stub (priority 20). Canon update needed: add ISimulatable priorities to systems.yaml.

**Acceptance Criteria:**
- [ ] ZoneSystem registered as ISimulatable with priority 30
- [ ] BuildingSystem registered as ISimulatable with priority 40
- [ ] Tick order verified: TerrainSystem(5) -> EnergyStub(10) -> FluidStub(20) -> ZoneSystem(30) -> BuildingSystem(40)
- [ ] Both systems' tick() methods called at correct frequency (every tick = 50ms at 20 ticks/sec)
- [ ] Integration test: ZoneSystem updates demand before BuildingSystem reads it in the same tick
- [ ] Canon update documented: systems.yaml priorities for ZoneSystem and BuildingSystem

**Dependencies:**
- Blocked by: 4-008 (ZoneSystem skeleton), 4-034 (BuildingSystem integration)
- Blocks: None

**Agent Notes:**
- Systems Architect: Tick ordering is critical for data consistency. Zones must be settled before structures evaluate spawn conditions.

---

## Dependency Graph

```
LEGEND:
  [4-NNN] = Epic 4 ticket
  {3-NNN} = Epic 3 dependency
  {1-NNN} = Epic 1 dependency
  --> = "blocks" (arrow points to dependent)

GROUP A: Data Types
  [4-001] ZoneComponent/Enums
    |--> [4-006] ZoneGrid
    |--> [4-008] ZoneSystem Skeleton
    |--> [4-009] Zone Events
    |--> [4-015] Zone State Mgmt
    |--> [4-016] Basic Demand
    |--> [4-035] IZoneQueryable
    |--> [4-041] Zone Serialization
    '--> [4-018] Desirability

  [4-002] BuildingState/Type Enums
    |--> [4-003] BuildingComponent
    |--> [4-004] ConstructionComponent
    |--> [4-019] Forward Dep Interfaces
    '--> [4-021] Template Registry

  [4-003] BuildingComponent
    |--> [4-025] Entity Creation
    |--> [4-034] BuildingSystem Integration
    |--> [4-036] IBuildingQueryable
    |--> [4-040] Building Network Msgs
    '--> [4-042] Building Serialization

  [4-004] ConstructionComponent
    |--> [4-025] Entity Creation
    |--> [4-027] Construction Progress
    '--> [4-042] Building Serialization

  [4-005] DebrisComponent
    |--> [4-030] Demolition Handler
    '--> [4-031] Debris System

GROUP B: Grid Storage
  [4-006] ZoneGrid
    |--> [4-008] ZoneSystem Skeleton
    |--> [4-011] Placement Validation
    |--> [4-012] Placement Execution
    |--> [4-015] Zone State
    |--> [4-035] IZoneQueryable
    '--> [4-038] Zone Network Msgs

  [4-007] BuildingGrid
    |--> [4-022] Template Selection (neighbor check)
    |--> [4-024] Spawn Conditions
    |--> [4-025] Entity Creation
    |--> [4-030] Demolition Handler
    '--> [4-034] BuildingSystem Integration

GROUP C-E: ZoneSystem Core
  [4-008] ZoneSystem Skeleton
    |--> [4-011] Placement Validation
    |--> [4-012] Placement Execution
    |--> [4-013] De-zoning
    |--> [4-015] Zone State
    |--> [4-016] Basic Demand
    |--> [4-018] Desirability
    '--> [4-035] IZoneQueryable

  [4-009] Zone Events
    |--> [4-012] Placement Execution
    |--> [4-013] De-zoning
    '--> [4-016] Basic Demand

  [4-011] Placement Validation
    '--> [4-012] Placement Execution

  [4-012] Placement Execution
    |--> [4-038] Zone Network Msgs

  [4-013] De-zoning
    '--> [4-014] Redesignation

  [4-016] Basic Demand
    '--> [4-017] IDemandProvider

GROUP F: Forward Dependencies
  [4-019] Interface Definitions
    '--> [4-020] Stub Implementations

  [4-020] Stubs
    |--> [4-024] Spawn Conditions
    |--> [4-025] Entity Creation
    '--> [4-034] BuildingSystem Integration

GROUP G: Templates
  [4-021] Template Registry
    |--> [4-022] Selection Algorithm
    |--> [4-023] Template Content
    '--> [4-037] IBuildingTemplateQuery

  [4-022] Selection Algorithm
    '--> [4-025] Entity Creation

GROUP H-I: Building Spawning + Construction
  [4-024] Spawn Conditions
    '--> [4-025] Entity Creation

  [4-025] Entity Creation
    '--> [4-026] Spawning Loop

  [4-026] Spawning Loop --> [4-034] Integration
  [4-027] Construction Progress --> [4-028] State Machine

GROUP J-K: State Machine + Demolition
  [4-028] State Machine
    |--> [4-029] Grace Period
    '--> [4-034] Integration

  [4-030] Demolition --> [4-031] Debris
  [4-030] Demolition --> [4-034] Integration

GROUP L: Upgrades
  [4-028] State Machine --> [4-032] Upgrades --> [4-033] Downgrades

GROUP M: Integration
  [4-034] BuildingSystem Integration
    |--> [4-036] IBuildingQueryable
    |--> [4-040] Building Network Msgs
    '--> [4-048] ISimulatable Registration

GROUP Q: Testing
  [4-034] + [4-008] + [4-012] + [4-026] + [4-028] --> [4-047] Integration Test
```

### Critical Path

The minimum path to "zones designate and structures materialize on screen":

**ZoneSystem track:**
1. **4-001** Zone enums/component (S)
2. **4-006** ZoneGrid (M)
3. **4-009** Zone events (S) [parallel with 4-006]
4. **4-008** ZoneSystem skeleton (M)
5. **4-011** Placement validation (L) [requires Epic 3: 3-014]
6. **4-012** Placement execution (M)
7. **4-016** Basic demand (L)
8. **4-015** Zone state management (M)

**BuildingSystem track (parallelizable with ZoneSystem after types defined):**
1. **4-002** Building enums (S)
2. **4-003** BuildingComponent (S)
3. **4-004** ConstructionComponent (S)
4. **4-007** BuildingGrid (M)
5. **4-019** Forward dep interfaces (M)
6. **4-020** Stubs (S)
7. **4-010** Building events (S) [parallel]
8. **4-021** Template registry (M)
9. **4-022** Template selection (M)
10. **4-024** Spawn conditions (M)
11. **4-025** Entity creation (M) -- **convergence point** (needs both Zone and Building tracks)
12. **4-026** Spawning loop (XL) -- **critical bottleneck**
13. **4-027** Construction progress (M)
14. **4-028** State machine (L)
15. **4-034** BuildingSystem integration (XL) -- **critical bottleneck**

**Final convergence:**
16. **4-048** ISimulatable registration (S)
17. **4-047** Integration test (L)

After the critical path, parallel tracks open:
- **Query track:** 4-035 (IZoneQueryable), 4-036 (IBuildingQueryable), 4-037 (IBuildingTemplateQuery)
- **Demolition track:** 4-030 -> 4-031
- **Upgrade track:** 4-032 -> 4-033
- **Network track:** 4-038 -> 4-039, 4-040
- **Serialization track:** 4-041, 4-042
- **Design track:** 4-043, 4-044, 4-045, 4-046

### Cross-Epic Dependencies

| External Epic | Epic 4 Consumer | Reason |
|--------------|----------------|--------|
| Epic 1 (Multiplayer) | 4-038, 4-039, 4-040, 4-041, 4-042 | ISerializable interface, SyncSystem for state broadcast |
| Epic 3 ticket 3-014 (ITerrainQueryable) | 4-011, 4-018, 4-024 | is_buildable(), get_value_bonus(), get_elevation() for zone/building placement |
| Epic 5 (EnergySystem) | 4-019, 4-020 | IEnergyProvider replaces StubEnergyProvider |
| Epic 6 (FluidSystem) | 4-019, 4-020 | IFluidProvider replaces StubFluidProvider |
| Epic 7 (TransportSystem) | 4-019, 4-020 | ITransportProvider replaces StubTransportProvider |
| Epic 10 (DemandSystem) | 4-017 | IDemandProvider replaces ZoneSystem basic demand |
| Epic 10 (LandValueSystem) | 4-019, 4-020 | ILandValueProvider replaces StubLandValueProvider |
| Epic 11 (EconomySystem) | 4-019, 4-020 | ICreditProvider replaces StubCreditProvider |

**Downstream consumers of Epic 4 (7 epics):**
- Epic 5 (Energy): IBuildingQueryable, BuildingConstructedEvent, building energy requirements
- Epic 6 (Fluid): IBuildingQueryable, BuildingConstructedEvent, building fluid requirements
- Epic 7 (Transport): IBuildingQueryable, building positions for traffic
- Epic 9 (Services): IBuildingQueryable, IBuildable/IDemolishable, service structures
- Epic 10 (Simulation): IZoneQueryable, IBuildingQueryable, zone counts, capacities, demand replacement
- Epic 11 (Financial): IBuildingQueryable, building counts/values, construction/demolition events
- Epic 13 (Disasters): IBuildingQueryable, BuildingComponent.health, building destruction

---

## Ticket Count Summary

| Group | Count | Description |
|-------|-------|-------------|
| A: Data Types | 5 | Enums, components (Zone, Building, Construction, Debris) |
| B: Grid Storage | 2 | ZoneGrid sparse index, BuildingGrid dense array |
| C: ZoneSystem Core | 2 | System skeleton, zone events |
| D: Zone Placement | 4 | Validation, execution, de-zoning, redesignation |
| E: Zone State/Demand | 4 | State management, demand calculation, demand extension, desirability |
| F: Forward Dependencies | 2 | Interface definitions, stub implementations |
| G: Template System | 3 | Data structure, selection algorithm, initial content |
| H: Building Spawning | 3 | Spawn conditions, entity creation, spawning loop |
| I: Construction | 1 | Construction progress system |
| J: State Machine | 2 | State transitions, grace period |
| K: Demolition/Debris | 2 | Demolition handler, debris auto-clear |
| L: Upgrades/Downgrades | 2 | Upgrade system, downgrade system |
| M: BuildingSystem Integration | 1 | Full system assembly |
| N: Query APIs | 3 | IZoneQueryable, IBuildingQueryable, IBuildingTemplateQuery |
| O: Network/Serialization | 5 | Zone messages, zone auth, building messages, zone serial, building serial |
| P: Design Deliverables | 4 | Color spec, animation spec, tuning params, template briefs |
| Q: Integration/Registration | 2 | End-to-end test, ISimulatable registration |
| **Total** | **48** | |

**Scope Distribution:**

| Scope | Count |
|-------|-------|
| S | 15 |
| M | 19 |
| L | 12 |
| XL | 2 |

**Priority Distribution:**

| Priority | Count |
|----------|-------|
| P0 (critical path) | 18 |
| P1 (important) | 22 |
| P2 (enhancement) | 8 |

---

## Notes

- Epic 4 is the second most-depended-upon epic in the project (after Epic 3). Seven downstream epics consume its interfaces. Interface stability (IZoneQueryable, IBuildingQueryable, IBuildingTemplateQuery) is critical -- lock signatures during Epic 4 planning.
- The forward dependency stub pattern (CCR-006) enables Epics 5/6/7 to begin in parallel once interfaces are defined. Deliver 4-019 and 4-020 early.
- With stubs returning permissive defaults, the Abandoned/Derelict/Deconstructed pipeline cannot be integration-tested until Epics 5/6/7. Use debug toggle on stubs (4-020) for state machine testing.
- The basic demand calculation (4-016) is intentionally simplified and will be completely replaced by DemandSystem (Epic 10) via IDemandProvider. Mark this code as temporary with a documented migration path.
- All overseer-facing text must use canonical alien terminology: habitation (not residential), exchange (not commercial), fabrication (not industrial), sector (not tile), pathway (not road), being (not citizen), overseer (not player/mayor), designation (not zoning), structure (not building), materializing (not under construction), derelict (not abandoned), deconstructed (not demolished), credits (not money), growth pressure (not RCI demand), colony (not city).
- Per CCR-010, NO scale variation on structures. Height variation is achieved through template selection (taller/shorter templates), not per-instance scaling.
- Per CCR-012, de-zoning an occupied sector uses DemolitionRequestEvent for decoupled architecture. ZoneSystem never directly calls BuildingSystem.
- BuildingGrid should be added to dense_grid_exception in patterns.yaml as a canon update.
- Audio cues for materialization, activation, decline, and deconstruction are deferred to the audio epic.
- Zone overlay rendering and growth pressure UI are deferred to Epic 12 (UI System). ZoneSystem and BuildingSystem expose the necessary query APIs.
