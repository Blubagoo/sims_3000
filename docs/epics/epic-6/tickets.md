# Epic 6: Water Infrastructure (FluidSystem) - Tickets

**Epic Goal:** Implement the fluid network (water system) using the canonical pool model: per-overseer fluid pools fed by fluid extractors near water bodies, with coverage zones defined by fluid conduits, enabling structures to draw from the pool when in coverage and pool has surplus. Reservoirs provide storage buffer for supply smoothing.
**Created:** 2026-01-28
**Canon Version:** 0.13.0
**Planning Agents:** Systems Architect, Game Designer, Services Engineer

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-28 | v0.7.0 | Initial ticket creation from three-agent analysis and resolved discussion (CCR-001 through CCR-010) |
| 2026-01-28 | v0.8.0 | Canon updated with IFluidProvider interface, FluidCoverageGrid in dense_grid_exception, FluidSystem tick_priority: 20 |
| 2026-01-29 | canon-verification (v0.8.0 → v0.13.0) | No changes required |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. IFluidProvider interface (ticket 6-038) matches interfaces.yaml with correct "has_fluid" terminology. FluidCoverageGrid in dense_grid_exception pattern. Priority 20 tick order confirmed in systems.yaml.

---

## Summary

Epic 6 implements the second major infrastructure system. Where energy makes your colony *function*, fluid makes it *alive*. FluidSystem follows the same pool model as EnergySystem but introduces key variations: water proximity requirements for extractors, power dependency (extractors need energy), and reservoir buffering for supply smoothing.

**Key Design Decisions (Resolved via Discussion):**
- CCR-001: Pool model confirmed -- per-overseer pools, conduits define coverage. Reservoirs provide storage buffer (energy has none)
- CCR-002: No priority rationing -- fluid uses all-or-nothing distribution. During deficit, ALL consumers lose fluid equally
- CCR-003: Water proximity model -- extractors require 0-8 tiles from water, efficiency falloff (100% adjacent, 30% at 8 tiles)
- CCR-004: Power dependency -- extractors require power via IEnergyProvider. Tick priority 20 (after EnergySystem at 10)
- CCR-005: Reservoir mechanics -- MVP one type, 1000 capacity, 50 fill rate, 100 drain rate (asymmetric). Proportional drain
- CCR-006: No grace period -- reservoirs serve as grace period. No built-in delay like energy's 100-tick grace
- CCR-007: Fluid requirements match energy -- identical values for all structure types
- CCR-008: Extractor energy priority -- extractors get ENERGY_PRIORITY_IMPORTANT (2) to stay powered during energy deficit
- CCR-009: Separate coverage grids -- EnergyCoverageGrid and FluidCoverageGrid are separate allocations
- CCR-010: Coverage delta preview -- same pattern as Epic 5 for conduit placement

**Target Metrics:**
- Coverage recalculation: <10ms for 512x512 map with 5,000 conduits
- Pool calculation: <1ms for 10,000 consumers
- Full tick: <2ms for 256x256 map
- FluidComponent: 12 bytes; FluidProducerComponent: 12 bytes; FluidReservoirComponent: 16 bytes; FluidConduitComponent: 4 bytes
- Coverage grid memory: 64KB (256x256) to 256KB (512x512)

---

## Ticket Counts

| Category | Count |
|----------|-------|
| **By Type** | |
| Infrastructure | 10 |
| Feature | 28 |
| Content | 4 |
| Design | 2 |
| **By System** | |
| FluidSystem Core | 12 |
| Coverage System | 6 |
| Pool & Distribution | 8 |
| Producer Mechanics | 8 |
| Conduit Mechanics | 5 |
| Integration | 5 |
| **By Scope** | |
| S | 16 |
| M | 24 |
| L | 4 |
| **By Priority** | |
| P0 (critical path) | 26 |
| P1 (important) | 12 |
| P2 (enhancement) | 6 |
| **Total** | **44** |

---

## Ticket List

### Group A: Data Types and Enumerations

---

### 6-001: Fluid Type Enumerations

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define foundational fluid data type enumerations. FluidPoolState enum (Healthy, Marginal, Deficit, Collapse) matching energy pattern. FluidProducerType enum to distinguish extractors from reservoirs.

**Acceptance Criteria:**
- [ ] FluidPoolState enum: Healthy(0), Marginal(1), Deficit(2), Collapse(3)
- [ ] FluidProducerType enum: Extractor(0), Reservoir(1)
- [ ] All enums use canonical alien terminology (fluid not water, extractor not pump)
- [ ] Unit tests for enum value ranges

**Dependencies:**
- Blocked by: None (foundational)
- Blocks: 6-002, 6-003, 6-004, 6-005, 6-006

**Agent Notes:**
- Systems Architect: FluidProducerType distinguishes production (extractors) from storage (reservoirs).
- Game Designer: Single extractor type for MVP. Reserve enum space for future types (deep well, desalination).

---

### 6-002: FluidComponent Structure

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the FluidComponent struct -- attached to all structures that consume fluid (zone structures, some service structures). Per CCR-002, NO priority field (unlike energy) as fluid uses all-or-nothing distribution.

**Acceptance Criteria:**
- [ ] FluidComponent struct defined:
  - fluid_required (uint32_t): fluid units needed per tick, from template
  - fluid_received (uint32_t): fluid units actually received this tick
  - has_fluid (bool): true if fluid_received >= fluid_required
  - padding (uint8_t[3]): alignment to 12 bytes
- [ ] Struct size is exactly 12 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] NO priority field (CCR-002: no rationing for fluid)
- [ ] Unit tests for default initialization and size assertion

**Dependencies:**
- Blocked by: None
- Blocks: 6-008, 6-019, 6-020, 6-034

**Agent Notes:**
- Services Engineer: No priority field unlike EnergyComponent. Fluid distribution is all-or-nothing.
- Game Designer: "Collective suffering" -- during deficit all structures lose fluid equally.

---

### 6-003: FluidProducerComponent Structure

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the FluidProducerComponent struct -- attached to fluid extractors that pump water. Simpler than EnergyProducerComponent: no aging, no contamination.

**Acceptance Criteria:**
- [ ] FluidProducerComponent struct defined:
  - base_output (uint32_t): maximum output at optimal conditions
  - current_output (uint32_t): actual output = base_output * water_factor * (powered ? 1 : 0)
  - max_water_distance (uint8_t): maximum tiles from water (typically 5 for operation)
  - current_water_distance (uint8_t): actual distance to water (from terrain query)
  - is_operational (bool): true if powered AND within water proximity
  - producer_type (uint8_t): FluidProducerType enum (Extractor for this component)
- [ ] Struct size is exactly 12 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] NO aging fields (unlike EnergyProducerComponent)
- [ ] NO contamination fields (fluid infrastructure is clean)
- [ ] Unit tests for default initialization and output calculation

**Dependencies:**
- Blocked by: 6-001 (FluidProducerType enum)
- Blocks: 6-008, 6-014, 6-025, 6-026

**Agent Notes:**
- Services Engineer: current_output = base_output * water_factor. Factor based on distance per CCR-003.
- Systems Architect: Simpler than energy (12 bytes vs 24 bytes) - no aging, no contamination.

---

### 6-004: FluidReservoirComponent Structure

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the FluidReservoirComponent struct -- attached to fluid reservoirs that store fluid buffer. Unique to Epic 6 (energy has no storage equivalent).

**Acceptance Criteria:**
- [ ] FluidReservoirComponent struct defined:
  - capacity (uint32_t): maximum storage (1000 units for MVP per CCR-005)
  - current_level (uint32_t): current stored amount
  - fill_rate (uint16_t): units per tick that can fill (50 per CCR-005)
  - drain_rate (uint16_t): units per tick that can drain (100 per CCR-005)
  - is_active (bool): true if connected to network
  - reservoir_type (uint8_t): reserved for future types
  - padding (uint8_t[2]): alignment
- [ ] Struct size is exactly 16 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] Asymmetric fill/drain rates per CCR-005 (drain faster than fill)
- [ ] Unit tests for default initialization and fill/drain calculations

**Dependencies:**
- Blocked by: None
- Blocks: 6-008, 6-015, 6-017

**Agent Notes:**
- Services Engineer: Asymmetric rates create tension - reservoirs empty quickly in crisis, recover slowly.
- Game Designer: "Reservoirs should feel like insurance" - visible level provides satisfaction/urgency.

---

### 6-005: FluidConduitComponent Structure

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the FluidConduitComponent struct -- attached to fluid conduit entities (pipes) that extend coverage zones. Identical structure to EnergyConduitComponent.

**Acceptance Criteria:**
- [ ] FluidConduitComponent struct defined:
  - coverage_radius (uint8_t): tiles of coverage this conduit adds (default 2-3)
  - is_connected (bool): true if connected to fluid network via BFS
  - is_active (bool): true if carrying fluid (for rendering flow effect)
  - conduit_level (uint8_t): 1=basic pipe, 2=upgraded (future expansion)
- [ ] Struct size is exactly 4 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] Identical to EnergyConduitComponent for consistency
- [ ] Unit tests for default initialization

**Dependencies:**
- Blocked by: None
- Blocks: 6-010, 6-029, 6-030, 6-031

**Agent Notes:**
- Services Engineer: Identical to EnergyConduitComponent - good pattern reuse.
- Game Designer: Conduits as "veins" carrying life through the colony - blue/cyan glow.

---

### 6-006: PerPlayerFluidPool Structure

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the PerPlayerFluidPool struct -- aggregated fluid pool data per overseer. Includes reservoir tracking fields unique to fluid.

**Acceptance Criteria:**
- [ ] PerPlayerFluidPool struct defined:
  - owner (PlayerID): overseer who owns this pool
  - total_generated (uint32_t): sum of operational extractor outputs
  - total_reservoir_stored (uint32_t): sum of all reservoir current levels
  - total_reservoir_capacity (uint32_t): sum of all reservoir max capacities
  - available (uint32_t): total_generated + total_reservoir_stored
  - total_consumed (uint32_t): sum of consumer fluid_required in coverage
  - surplus (int32_t): available - total_consumed (can be negative)
  - extractor_count (uint32_t): number of operational extractors
  - reservoir_count (uint32_t): number of reservoirs
  - consumer_count (uint32_t): number of consumers in coverage
  - state (FluidPoolState): Healthy/Marginal/Deficit/Collapse
  - previous_state (FluidPoolState): for transition detection
- [ ] Struct supports negative surplus (deficit condition)
- [ ] Includes reservoir tracking (unlike PerPlayerEnergyPool)
- [ ] Unit tests for surplus calculation and state determination

**Dependencies:**
- Blocked by: 6-001 (FluidPoolState enum)
- Blocks: 6-017, 6-018, 6-037

**Agent Notes:**
- Systems Architect: Key difference from energy: reservoir fields for buffer tracking.
- Services Engineer: available = generated + stored. Reservoirs contribute to available pool.

---

### 6-007: Fluid Event Definitions

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define all fluid-related event structs for cross-system communication and network sync.

**Acceptance Criteria:**
- [ ] FluidStateChangedEvent: entity_id, owner_id, had_fluid, has_fluid
- [ ] FluidDeficitBeganEvent: owner_id, deficit_amount, affected_consumers count
- [ ] FluidDeficitEndedEvent: owner_id, surplus_amount
- [ ] FluidCollapseBeganEvent: owner_id, deficit_amount
- [ ] FluidCollapseEndedEvent: owner_id
- [ ] FluidConduitPlacedEvent: entity_id, owner_id, grid_position
- [ ] FluidConduitRemovedEvent: entity_id, owner_id, grid_position
- [ ] ExtractorPlacedEvent: entity_id, owner_id, grid_position, water_distance
- [ ] ExtractorRemovedEvent: entity_id, owner_id, grid_position
- [ ] ReservoirPlacedEvent: entity_id, owner_id, grid_position
- [ ] ReservoirRemovedEvent: entity_id, owner_id, grid_position
- [ ] All events follow canon event pattern (suffix: Event)
- [ ] Unit tests verifying struct completeness

**Dependencies:**
- Blocked by: 6-001 (enums for event fields)
- Blocks: 6-021, 6-022, 6-029, 6-031

**Agent Notes:**
- Systems Architect: Events are primary notification mechanism. Server broadcasts to all clients.
- Game Designer: FluidStateChangedEvent drives visual feedback (hydrated glow on/off).

---

### Group B: Coverage System

---

### 6-008: FluidCoverageGrid Dense 2D Array

**Type:** infrastructure
**System:** Coverage System
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the FluidCoverageGrid class -- a dense 2D grid storing coverage owner per cell for O(1) coverage queries. Per CCR-009, separate from EnergyCoverageGrid. Follows dense_grid_exception pattern.

**Acceptance Criteria:**
- [ ] FluidCoverageGrid class with width, height, and flat vector<uint8_t> storage
- [ ] is_in_coverage(x, y, owner) returns bool -- O(1)
- [ ] get_coverage_owner(x, y) returns PlayerID (0 if uncovered) -- O(1)
- [ ] set(x, y, owner) marks cell as covered by owner
- [ ] clear(x, y) marks cell as uncovered
- [ ] clear_all_for_owner(owner) clears all cells for specific overseer
- [ ] Row-major storage: index = y * width + x
- [ ] Supports 128x128, 256x256, 512x512 map sizes
- [ ] Memory: 1 byte per cell (16KB / 64KB / 256KB for respective sizes)
- [ ] Unit tests for queries, bounds checking, ownership conflicts

**Dependencies:**
- Blocked by: None (foundational data structure)
- Blocks: 6-009, 6-010, 6-013

**Agent Notes:**
- Services Engineer: Separate from EnergyCoverageGrid per CCR-009. Simpler implementation, independent dirty tracking.
- Systems Architect: Canon update needed: add FluidCoverageGrid to dense_grid_exception applies_to.

---

### 6-009: FluidSystem Class Skeleton and ISimulatable

**Type:** infrastructure
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define the FluidSystem class implementing ISimulatable at priority 20 (after EnergySystem at 10). Constructor takes ITerrainQueryable* and IEnergyProvider*. tick() method orchestrates coverage, pool calculation, reservoir buffering, distribution, and events.

**Acceptance Criteria:**
- [ ] FluidSystem class declaration with ISimulatable implementation
- [ ] get_priority() returns 20 (fluid runs after energy, before zones/buildings)
- [ ] Constructor accepts ITerrainQueryable* and IEnergyProvider* via dependency injection
- [ ] tick() method stub for full processing pipeline
- [ ] Internal FluidCoverageGrid instance
- [ ] Per-overseer PerPlayerFluidPool array
- [ ] Per-overseer dirty flags for coverage (bool array)
- [ ] Per-overseer extractor, reservoir, consumer lists
- [ ] All query method declarations
- [ ] Compiles and registers as a system

**Dependencies:**
- Blocked by: 6-002, 6-003, 6-004, 6-005, 6-006, 6-008 (all component types and coverage grid)
- Blocks: 6-010, 6-011, 6-014, 6-015, 6-016, 6-019, 6-038
- External: Epic 3 (ITerrainQueryable), Epic 5 (IEnergyProvider)

**Agent Notes:**
- Systems Architect: Priority 20 ensures power state is settled before fluid calculations.
- Services Engineer: System skeleton enables parallel development of subsystems.

---

### 6-010: Coverage Zone BFS Flood-Fill Algorithm

**Type:** feature
**System:** Coverage System
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement the BFS flood-fill algorithm for coverage zone calculation. Coverage propagates from OPERATIONAL extractors and ALL reservoirs through connected conduits. Per CCR-004, extractors must be powered to seed coverage.

**Acceptance Criteria:**
- [ ] recalculate_coverage(PlayerID owner) implements full BFS
- [ ] Step 1: Clear coverage for owner
- [ ] Step 2: Seed from operational extractors (powered AND within water proximity)
- [ ] Step 3: Seed from ALL reservoirs (no power requirement - passive storage)
- [ ] Step 4: BFS through 4-directional neighbors
- [ ] Step 5: If neighbor has conduit owned by same player, mark conduit's radius, add to frontier
- [ ] Step 6: Continue until frontier empty
- [ ] mark_coverage_radius(x, y, radius, owner) marks square area around position
- [ ] Ownership boundary enforcement: only extend to tiles owned by same player or GAME_MASTER
- [ ] visited set prevents infinite loops
- [ ] Performance target: <10ms for 512x512 with 5,000 conduits
- [ ] Unit tests for simple networks, complex networks, isolated segments

**Dependencies:**
- Blocked by: 6-008 (FluidCoverageGrid), 6-005 (FluidConduitComponent), 6-003 (FluidProducerComponent)
- Blocks: 6-011, 6-012, 6-013, 6-030

**Agent Notes:**
- Services Engineer: Key difference from energy: extractors only seed if OPERATIONAL (powered + water proximity). Reservoirs always seed.
- Systems Architect: Coverage is binary: in-zone or not. Conduits extend coverage, not transport fluid.

---

### 6-011: Coverage Dirty Flag Tracking

**Type:** feature
**System:** Coverage System
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement dirty flag tracking for coverage recalculation optimization. Coverage only recalculates when the network changes (conduit/extractor/reservoir placed or removed).

**Acceptance Criteria:**
- [ ] bool coverage_dirty_[MAX_PLAYERS] array in FluidSystem
- [ ] on_conduit_placed(FluidConduitPlacedEvent) sets dirty flag for owner
- [ ] on_conduit_removed(FluidConduitRemovedEvent) sets dirty flag for owner
- [ ] on_extractor_placed(ExtractorPlacedEvent) sets dirty flag for owner
- [ ] on_extractor_removed(ExtractorRemovedEvent) sets dirty flag for owner
- [ ] on_reservoir_placed(ReservoirPlacedEvent) sets dirty flag for owner
- [ ] on_reservoir_removed(ReservoirRemovedEvent) sets dirty flag for owner
- [ ] tick() phase 1 iterates overseers, recalculates only if dirty, clears flag
- [ ] Unit tests for dirty flag setting and clearing

**Dependencies:**
- Blocked by: 6-010 (coverage algorithm), 6-007 (events)
- Blocks: None (optimization)

**Agent Notes:**
- Services Engineer: Dirty tracking limits recalculation to actual changes. Critical for performance.
- Systems Architect: More event types than energy (extractor + reservoir placements both trigger).

---

### 6-012: Ownership Boundary Enforcement in Coverage

**Type:** feature
**System:** Coverage System
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement ownership boundary enforcement in coverage calculation. Per canon patterns.yaml, fluid conduits do NOT connect across overseer boundaries.

**Acceptance Criteria:**
- [ ] can_extend_coverage_to(x, y, owner) returns bool
- [ ] Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
- [ ] Returns false if tile_owner belongs to different player
- [ ] Coverage BFS uses this check before extending to neighbor
- [ ] Conduits adjacent to boundary are marked is_connected but coverage stops at boundary
- [ ] Unit tests for boundary scenarios, adjacent overseer networks

**Dependencies:**
- Blocked by: 6-010 (coverage algorithm)
- Blocks: None

**Agent Notes:**
- Systems Architect: Explicit boundary check prevents accidental coverage sharing between overseers.
- Game Designer: Each overseer maintains their own fluid network -- no griefing via fluid denial.

---

### 6-013: Coverage Queries

**Type:** feature
**System:** Coverage System
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Implement public coverage query methods for other systems and UI overlay.

**Acceptance Criteria:**
- [ ] is_in_coverage(x, y, owner) const -> bool: O(1) query via FluidCoverageGrid
- [ ] get_coverage_at(x, y) const -> PlayerID: returns covering owner or 0
- [ ] get_coverage_count(owner) const -> uint32_t: count of covered cells for owner
- [ ] All queries are const and thread-safe for UI reads
- [ ] Unit tests for all query methods

**Dependencies:**
- Blocked by: 6-008 (FluidCoverageGrid), 6-010 (coverage algorithm)
- Blocks: 6-019, 6-039

**Agent Notes:**
- Services Engineer: O(1) queries essential for per-tick consumer iteration.
- Systems Architect: UI overlay will call get_coverage_at() for each visible cell.

---

### Group C: Pool Calculation and Distribution

---

### 6-014: Extractor Registration and Output Calculation

**Type:** feature
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement extractor registration (tracking all extractors per overseer) and per-tick output calculation. Output = base_output * water_factor * (powered ? 1 : 0).

**Acceptance Criteria:**
- [ ] Internal list/map of extractors per overseer
- [ ] register_extractor(EntityID, PlayerID) called when extractor placed
- [ ] unregister_extractor(EntityID, PlayerID) called when extractor removed
- [ ] update_extractor_output(FluidProducerComponent&, EntityID) calculates current_output
- [ ] Query IEnergyProvider.is_powered(entity) for power state
- [ ] Query ITerrainQueryable.get_water_distance(x, y) for water proximity
- [ ] calculate_water_factor(distance, max_distance) per CCR-003:
  - 0 tiles: 100%, 1-2: 90%, 3-4: 70%, 5-6: 50%, 7-8: 30%
- [ ] If !powered OR distance > max: current_output = 0, is_operational = false
- [ ] tick() phase 2 iterates all extractors and updates output
- [ ] Unit tests for output calculation with various distance/power combinations

**Dependencies:**
- Blocked by: 6-009 (FluidSystem skeleton), 6-003 (FluidProducerComponent)
- Blocks: 6-017
- External: Epic 5 IEnergyProvider, Epic 3 ITerrainQueryable

**Agent Notes:**
- Services Engineer: Water factor curve per CCR-003. Power check via IEnergyProvider.
- Game Designer: Extractors reward strategic waterfront placement - clear efficiency curve.

---

### 6-015: Reservoir Registration and Storage Management

**Type:** feature
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement reservoir registration (tracking all reservoirs per overseer) and storage level tracking. Reservoirs fill during surplus, drain during deficit.

**Acceptance Criteria:**
- [ ] Internal list/map of reservoirs per overseer
- [ ] register_reservoir(EntityID, PlayerID) called when reservoir placed
- [ ] unregister_reservoir(EntityID, PlayerID) called when reservoir removed
- [ ] calculate_total_reservoir_stored(PlayerID) sums all reservoir current_levels
- [ ] calculate_total_reservoir_capacity(PlayerID) sums all reservoir capacities
- [ ] tick() phase 3 updates reservoir totals in pool
- [ ] Unit tests for registration and total calculations

**Dependencies:**
- Blocked by: 6-009 (FluidSystem skeleton), 6-004 (FluidReservoirComponent)
- Blocks: 6-017, 6-018

**Agent Notes:**
- Services Engineer: Per-reservoir storage with pool-level aggregation per CCR-005.
- Game Designer: Visible reservoir levels create satisfaction/urgency feedback.

---

### 6-016: Consumer Registration and Requirement Aggregation

**Type:** feature
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement consumer registration (tracking all fluid consumers per overseer) and per-tick requirement aggregation. Only consumers in coverage contribute to total_consumed.

**Acceptance Criteria:**
- [ ] Internal list/map of consumers per overseer
- [ ] register_consumer(EntityID, PlayerID) called when structure with FluidComponent placed
- [ ] unregister_consumer(EntityID, PlayerID) called when structure removed
- [ ] aggregate_consumption(PlayerID) returns total fluid_required for consumers in coverage
- [ ] Only consumers where is_in_coverage(position, owner) == true are counted
- [ ] tick() phase 4 uses this to populate pool.total_consumed
- [ ] Unit tests for aggregation with mixed coverage scenarios

**Dependencies:**
- Blocked by: 6-009 (FluidSystem skeleton), 6-002 (FluidComponent), 6-013 (coverage queries)
- Blocks: 6-017

**Agent Notes:**
- Services Engineer: Consumers outside coverage don't contribute to pool consumption (not connected).
- Systems Architect: Registration happens via BuildingConstructedEvent handler.

---

### 6-017: Pool Calculation (Generation + Storage - Consumption = Surplus)

**Type:** feature
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement per-overseer pool calculation. Sum generation from operational extractors, add reservoir storage, subtract consumption from covered consumers.

**Acceptance Criteria:**
- [ ] calculate_pool(PlayerID) populates PerPlayerFluidPool for overseer
- [ ] total_generated = SUM(extractor.current_output for operational extractors)
- [ ] available = total_generated + total_reservoir_stored
- [ ] total_consumed = SUM(consumer.fluid_required for consumers in coverage)
- [ ] surplus = available - total_consumed (can be negative)
- [ ] extractor_count, reservoir_count, consumer_count updated
- [ ] tick() phase 5 calls calculate_pool() for each overseer
- [ ] Unit tests for healthy, marginal, deficit, and collapse scenarios

**Dependencies:**
- Blocked by: 6-014 (extractor output), 6-015 (reservoir storage), 6-016 (consumer aggregation), 6-006 (PerPlayerFluidPool)
- Blocks: 6-018, 6-019, 6-020

**Agent Notes:**
- Systems Architect: Key difference from energy: available includes reservoir storage.
- Services Engineer: Negative surplus = deficit state. Triggers reservoir drain.

---

### 6-018: Pool State Machine and Reservoir Buffering

**Type:** feature
**System:** FluidSystem Core
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement pool state calculation, reservoir fill/drain logic, and transition detection. States based on surplus thresholds.

**Acceptance Criteria:**
- [ ] calculate_pool_state(PerPlayerFluidPool) returns FluidPoolState
- [ ] Healthy: surplus >= buffer_threshold (10% of available)
- [ ] Marginal: 0 <= surplus < buffer_threshold
- [ ] Deficit: surplus < 0 AND reservoirs can cover
- [ ] Collapse: surplus < 0 AND reservoirs empty
- [ ] apply_reservoir_buffering(PlayerID) handles fill/drain per CCR-005:
  - Surplus > 0: fill reservoirs (capped by fill_rate, proportional to capacity)
  - Surplus < 0: drain reservoirs (capped by drain_rate, proportional to current level)
- [ ] Proportional drain across all reservoirs per discussion Q010
- [ ] detect_pool_state_transitions(PlayerID) compares previous_state to new state
- [ ] Emits FluidDeficitBeganEvent, FluidDeficitEndedEvent, FluidCollapseBeganEvent, FluidCollapseEndedEvent
- [ ] tick() phase 6 calls buffering logic, then state calculation
- [ ] Unit tests for all state transitions, fill/drain calculations, proportional distribution

**Dependencies:**
- Blocked by: 6-017 (pool calculation), 6-015 (reservoir registration), 6-007 (events)
- Blocks: 6-022

**Agent Notes:**
- Services Engineer: Reservoir buffering is unique to fluid. Proportional drain prevents "empty while others full."
- Game Designer: Asymmetric fill/drain (50/100) creates tension - empties fast, recovers slowly.

---

### 6-019: Fluid Distribution (All-or-Nothing)

**Type:** feature
**System:** FluidSystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement fluid distribution for healthy/marginal pools. Per CCR-002, NO priority rationing -- all consumers either have fluid or don't.

**Acceptance Criteria:**
- [ ] distribute_fluid(PlayerID) sets has_fluid for all consumers
- [ ] If pool.surplus >= 0 (after reservoir buffering): ALL consumers in coverage have fluid
  - fluid_received = fluid_required
  - has_fluid = true
- [ ] If pool.surplus < 0 (after reservoir drain exhausted): ALL consumers lose fluid
  - fluid_received = 0
  - has_fluid = false
- [ ] Consumers outside coverage: has_fluid = false, fluid_received = 0 (always)
- [ ] NO priority-based rationing (unlike energy per CCR-002)
- [ ] tick() phase 7 calls distribution logic
- [ ] Unit tests for full distribution with healthy pool, deficit pool

**Dependencies:**
- Blocked by: 6-017 (pool calculation), 6-018 (reservoir buffering), 6-002 (FluidComponent), 6-013 (coverage queries)
- Blocks: 6-021

**Agent Notes:**
- Services Engineer: Simpler than energy distribution - no sorting, no rationing.
- Game Designer: "Collective suffering" - during deficit, everyone loses water equally.

---

### 6-020: Priority-Based Rationing NOT Implemented (Design Note)

**Type:** design
**System:** FluidSystem Core
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document that fluid intentionally does NOT implement priority-based rationing per CCR-002. This ticket exists for documentation and future reference.

**Acceptance Criteria:**
- [ ] Document in FluidSystem code comments: "Fluid uses all-or-nothing distribution, no priority rationing"
- [ ] Reference CCR-002 from discussion document
- [ ] If future requirements change, this ticket provides starting point

**Dependencies:**
- Blocked by: 6-019 (distribution implementation)
- Blocks: None

**Agent Notes:**
- Game Designer: "Fluid affects habitability uniformly. You can't say these beings get water and these don't."
- Systems Architect: Explicit non-implementation. Energy has rationing (4 levels), fluid does not.

---

### 6-021: FluidStateChangedEvent Emission

**Type:** feature
**System:** FluidSystem Core
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement FluidStateChangedEvent emission when a consumer's has_fluid state changes.

**Acceptance Criteria:**
- [ ] Track previous has_fluid state per consumer (or detect change during distribution)
- [ ] emit_state_change_events() compares previous to current state
- [ ] Emit FluidStateChangedEvent for each consumer that changed
- [ ] Event includes entity_id, owner_id, had_fluid, has_fluid
- [ ] tick() phase 8 calls emission logic
- [ ] Unit tests for event emission on fluid gain/loss transitions

**Dependencies:**
- Blocked by: 6-019 (distribution), 6-007 (events)
- Blocks: None

**Agent Notes:**
- Systems Architect: RenderingSystem listens for these events to update hydration visual state.
- Game Designer: State changes drive visual feedback (hydrated blue glow on/off).

---

### 6-022: Pool State Transition Event Emission

**Type:** feature
**System:** FluidSystem Core
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement pool state transition event emission (deficit began/ended, collapse began/ended).

**Acceptance Criteria:**
- [ ] Detect transitions in detect_pool_state_transitions(PlayerID)
- [ ] Emit FluidDeficitBeganEvent when state transitions to Deficit from Healthy/Marginal
- [ ] Emit FluidDeficitEndedEvent when state transitions from Deficit to Healthy/Marginal
- [ ] Emit FluidCollapseBeganEvent when state transitions to Collapse
- [ ] Emit FluidCollapseEndedEvent when state transitions from Collapse
- [ ] Events include relevant data (deficit amount, affected consumer count)
- [ ] Unit tests for all transition combinations

**Dependencies:**
- Blocked by: 6-018 (pool state machine), 6-007 (events)
- Blocks: None

**Agent Notes:**
- Services Engineer: Pool state events trigger UI notifications and audio cues.
- Game Designer: "Fluid collapse" affects habitability - more personal than energy collapse.

---

### Group D: Producer Mechanics

---

### 6-023: Fluid Extractor Type Definition and Stats

**Type:** content
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define configuration data for MVP fluid extractor type. Single type for MVP, enum space reserved for future types.

**Acceptance Criteria:**
- [ ] FluidExtractorConfig struct defined with all configuration fields
- [ ] MVP extractor stats:
  - base_output: 100 fluid units/tick
  - build_cost: 3000 credits
  - maintenance_cost: 30 credits/cycle
  - energy_required: 20 units (for EnergyComponent)
  - energy_priority: ENERGY_PRIORITY_IMPORTANT (2) per CCR-008
  - max_placement_distance: 8 tiles
  - max_operational_distance: 5 tiles (full efficiency at 0-2)
  - coverage_radius: 8 tiles
- [ ] Values configurable (not hardcoded) for balancing
- [ ] Unit tests verifying configuration completeness

**Dependencies:**
- Blocked by: 6-001 (enums)
- Blocks: 6-014, 6-025, 6-026, 6-040

**Agent Notes:**
- Game Designer: Extractors reward waterfront placement. Energy priority keeps them running during energy deficit.
- Services Engineer: Single type for MVP simplicity. Future types reserved in enum.

---

### 6-024: Fluid Reservoir Type Definition and Stats

**Type:** content
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define configuration data for MVP fluid reservoir type. Per CCR-005 capacity and rates.

**Acceptance Criteria:**
- [ ] FluidReservoirConfig struct defined with all configuration fields
- [ ] MVP reservoir stats:
  - capacity: 1000 fluid units
  - fill_rate: 50 units/tick
  - drain_rate: 100 units/tick (asymmetric per CCR-005)
  - build_cost: 2000 credits
  - maintenance_cost: 20 credits/cycle
  - coverage_radius: 6 tiles
  - NO energy requirement (passive storage)
- [ ] Values configurable (not hardcoded) for balancing
- [ ] Unit tests verifying configuration completeness

**Dependencies:**
- Blocked by: 6-001 (enums)
- Blocks: 6-015, 6-028, 6-040

**Agent Notes:**
- Game Designer: "Reservoirs should feel like insurance" - 1000 capacity provides meaningful buffer.
- Services Engineer: Asymmetric rates per CCR-005 - drains faster (100) than fills (50).

---

### 6-025: Water Proximity Extraction Efficiency

**Type:** feature
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement water proximity efficiency calculation per CCR-003. Efficiency falls off with distance from water.

**Acceptance Criteria:**
- [ ] calculate_water_factor(uint8_t distance, uint8_t max_distance) returns float
- [ ] Efficiency curve per CCR-003:
  - 0 tiles (adjacent): 1.0 (100%)
  - 1-2 tiles: 0.9 (90%)
  - 3-4 tiles: 0.7 (70%)
  - 5-6 tiles: 0.5 (50%)
  - 7-8 tiles: 0.3 (30%)
  - 9+ tiles: 0.0 (cannot operate)
- [ ] Query ITerrainQueryable.get_water_distance(x, y) for distance
- [ ] current_output = base_output * water_factor * (powered ? 1 : 0)
- [ ] Unit tests for each distance tier

**Dependencies:**
- Blocked by: 6-014 (extractor output), 6-023 (extractor config)
- External: Epic 3 ITerrainQueryable

**Agent Notes:**
- Game Designer: Clear efficiency tiers reward strategic placement. Waterfront premium.
- Services Engineer: Linear-ish curve with discrete tiers. Easy to understand and balance.

---

### 6-026: Extractor Power Dependency

**Type:** feature
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement extractor power dependency via IEnergyProvider. Unpowered extractors produce nothing.

**Acceptance Criteria:**
- [ ] Query IEnergyProvider.is_powered(extractor_entity) each tick
- [ ] If !powered: current_output = 0, is_operational = false
- [ ] If powered AND within water proximity: calculate output normally
- [ ] Extractor has EnergyComponent with energy_required from config
- [ ] Extractor has energy_priority = ENERGY_PRIORITY_IMPORTANT (2) per CCR-008
- [ ] Power loss cascade: extractors offline -> pool generation drops -> potential deficit
- [ ] Unit tests for powered/unpowered state transitions

**Dependencies:**
- Blocked by: 6-014 (extractor output), 6-023 (extractor config)
- External: Epic 5 IEnergyProvider

**Agent Notes:**
- Systems Architect: FluidSystem tick_priority 20 ensures energy state is resolved first.
- Game Designer: Power loss -> water crisis cascade creates interesting tension.

---

### 6-027: Extractor Placement Validation

**Type:** feature
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement extractor placement validation including water proximity requirement unique to fluid.

**Acceptance Criteria:**
- [ ] validate_extractor_placement(x, y, owner) returns success/failure with reason
- [ ] Bounds check
- [ ] Ownership check (player owns tile)
- [ ] Terrain buildable check (ITerrainQueryable.is_buildable)
- [ ] No existing structure check
- [ ] Water proximity check (UNIQUE TO FLUID):
  - get_water_distance(x, y) <= MAX_PLACEMENT_DISTANCE (8)
- [ ] Cannot place ON water tiles (water is not buildable terrain)
- [ ] Return ExtractorPlacementPreview with: can_place, will_be_operational, water_distance, expected_efficiency
- [ ] Unit tests for each validation step

**Dependencies:**
- Blocked by: 6-023 (extractor config with distances)
- External: Epic 3 ITerrainQueryable

**Agent Notes:**
- Services Engineer: Unique validation for fluid - water proximity check. Energy nexuses have no such constraint.
- Game Designer: Clear feedback during placement - show efficiency at cursor position.

---

### 6-028: Reservoir Placement Validation

**Type:** feature
**System:** Producer Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement reservoir placement validation. Standard checks, NO water proximity requirement.

**Acceptance Criteria:**
- [ ] validate_reservoir_placement(x, y, owner) returns success/failure with reason
- [ ] Bounds check
- [ ] Ownership check (player owns tile)
- [ ] Terrain buildable check
- [ ] No existing structure check
- [ ] NO water proximity requirement (reservoirs can be placed anywhere buildable)
- [ ] Unit tests for each validation step

**Dependencies:**
- Blocked by: 6-024 (reservoir config)
- Blocks: None

**Agent Notes:**
- Services Engineer: Simpler than extractor - no water proximity needed.
- Game Designer: Strategic placement choice - reservoirs as buffer zones anywhere in network.

---

### Group E: Conduit Mechanics

---

### 6-029: Fluid Conduit Placement and Validation

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement fluid conduit placement with validation. Conduits extend coverage zones.

**Acceptance Criteria:**
- [ ] validate_conduit_placement(x, y, owner) returns success/failure
- [ ] Bounds check
- [ ] Ownership check
- [ ] Terrain buildable check
- [ ] No existing structure check (conduit is infrastructure, not zone structure)
- [ ] place_conduit(x, y, owner) creates conduit entity with FluidConduitComponent + PositionComponent + OwnershipComponent
- [ ] Emits FluidConduitPlacedEvent
- [ ] Sets coverage_dirty_[owner] = true
- [ ] Cost deducted (configurable, default 10 credits)
- [ ] Unit tests for placement validation and entity creation

**Dependencies:**
- Blocked by: 6-005 (FluidConduitComponent), 6-007 (FluidConduitPlacedEvent), 6-011 (dirty tracking)
- Blocks: 6-030

**Agent Notes:**
- Services Engineer: Conduit placement triggers coverage recalculation via dirty flag.
- Game Designer: Low cost encourages building conduit networks. "Drawing blue veins."

---

### 6-030: Conduit Connection Detection

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement conduit connection detection. A conduit's is_connected flag is set during coverage BFS if reachable from an operational producer.

**Acceptance Criteria:**
- [ ] During coverage BFS (6-010), when visiting a conduit, set is_connected = true
- [ ] Before BFS, reset all conduits' is_connected to false for the owner
- [ ] Isolated conduits (not connected to any producer network) remain is_connected = false
- [ ] is_connected used for visual feedback (dim vs flowing conduit)
- [ ] Unit tests for connected and isolated conduit scenarios

**Dependencies:**
- Blocked by: 6-010 (coverage algorithm), 6-005 (FluidConduitComponent)
- Blocks: None

**Agent Notes:**
- Services Engineer: Connection detection is a byproduct of BFS traversal.
- Game Designer: Isolated conduits appear dim -- visual feedback for "not working."

---

### 6-031: Conduit Removal and Coverage Update

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement conduit removal. Removing a conduit triggers coverage recalculation.

**Acceptance Criteria:**
- [ ] remove_conduit(EntityID) validates ownership and removes entity
- [ ] Emits FluidConduitRemovedEvent
- [ ] Sets coverage_dirty_[owner] = true
- [ ] No refund of placement cost
- [ ] Unit tests for removal and dirty flag setting

**Dependencies:**
- Blocked by: 6-029 (conduit placement), 6-007 (FluidConduitRemovedEvent)
- Blocks: None

**Agent Notes:**
- Services Engineer: Removal is straightforward. Dirty flag handles coverage update.
- Systems Architect: No cascade -- removing conduit doesn't affect structures, just coverage.

---

### 6-032: Conduit Active State for Rendering

**Type:** feature
**System:** Conduit Mechanics
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement conduit is_active state update for rendering. Conduits show flow effect when actively carrying fluid.

**Acceptance Criteria:**
- [ ] update_conduit_active_states() called in tick() phase 9
- [ ] Conduit is_active = true if is_connected AND owner's pool.total_generated > 0
- [ ] is_active = false if disconnected OR pool has no generation
- [ ] RenderingSystem reads is_active for flow pulse effect
- [ ] Unit tests for active state based on pool and connection

**Dependencies:**
- Blocked by: 6-030 (connection detection), 6-017 (pool calculation)
- Blocks: None

**Agent Notes:**
- Services Engineer: Active state is visual only. Conduits show flow when fluid "flows."
- Game Designer: "Soft blue pulse traveling along conduit path" - visual life in network.

---

### 6-033: Conduit Placement Preview (Coverage Delta)

**Type:** feature
**System:** Conduit Mechanics
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement coverage preview during conduit placement per CCR-010. Show what tiles will GAIN coverage.

**Acceptance Criteria:**
- [ ] preview_conduit_coverage(x, y, owner) returns set of tiles that would gain coverage
- [ ] Calculate hypothetical coverage with new conduit
- [ ] Compare to current coverage
- [ ] Return delta (new tiles only)
- [ ] Used by UI for placement preview overlay
- [ ] Performance: fast enough for real-time preview (<5ms)
- [ ] Unit tests for delta calculation

**Dependencies:**
- Blocked by: 6-010 (coverage algorithm), 6-013 (coverage queries)
- Blocks: None

**Agent Notes:**
- Game Designer: Delta preview makes conduit placement feel like "painting blue coverage."
- Services Engineer: May need simplified preview algorithm for performance.

---

### Group F: Integration

---

### 6-034: BuildingConstructedEvent Handler (Register Consumer/Producer)

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement BuildingConstructedEvent handler to register new fluid consumers and producers.

**Acceptance Criteria:**
- [ ] FluidSystem subscribes to BuildingConstructedEvent (from Epic 4)
- [ ] on_building_constructed(BuildingConstructedEvent&):
  - If entity has FluidComponent: register_consumer(entity, owner)
  - If entity has FluidProducerComponent: register_extractor(entity, owner), set coverage_dirty
  - If entity has FluidReservoirComponent: register_reservoir(entity, owner), set coverage_dirty
- [ ] Consumers get has_fluid set on next tick
- [ ] Unit tests for consumer and producer registration

**Dependencies:**
- Blocked by: 6-014 (extractor registration), 6-015 (reservoir registration), 6-016 (consumer registration)
- External: Epic 4 BuildingConstructedEvent

**Agent Notes:**
- Systems Architect: Event-driven registration decouples FluidSystem from BuildingSystem.
- Services Engineer: New structures automatically participate in fluid system.

---

### 6-035: BuildingDeconstructedEvent Handler (Unregister)

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement BuildingDeconstructedEvent handler to unregister consumers and producers.

**Acceptance Criteria:**
- [ ] FluidSystem subscribes to BuildingDeconstructedEvent (from Epic 4)
- [ ] on_building_deconstructed(BuildingDeconstructedEvent&):
  - If entity was consumer: unregister_consumer(entity, owner)
  - If entity was extractor: unregister_extractor(entity, owner), set coverage_dirty
  - If entity was reservoir: unregister_reservoir(entity, owner), set coverage_dirty
- [ ] Pool recalculated on next tick
- [ ] Unit tests for consumer and producer unregistration

**Dependencies:**
- Blocked by: 6-034 (registration handlers)
- External: Epic 4 BuildingDeconstructedEvent

**Agent Notes:**
- Services Engineer: Unregistration mirrors registration. Producer removal triggers coverage recalc.
- Systems Architect: Destroyed producers stop contributing to pool immediately.

---

### 6-036: Network Serialization for FluidComponent

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement network serialization for FluidComponent. has_fluid syncs every tick.

**Acceptance Criteria:**
- [ ] FluidComponent serialization/deserialization methods
- [ ] has_fluid synced every tick via standard component sync
- [ ] fluid_received optionally synced (for UI display)
- [ ] fluid_required only synced on entity creation (derived from template)
- [ ] Deterministic serialization for multiplayer consistency
- [ ] Unit tests for serialization round-trip

**Dependencies:**
- Blocked by: 6-002 (FluidComponent)
- Blocks: None

**Agent Notes:**
- Systems Architect: has_fluid is 1 bit per entity. Efficient for high-frequency sync.
- Services Engineer: Simpler than energy (no priority field to sync).

---

### 6-037: Network Serialization for Pool State

**Type:** feature
**System:** Integration
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement network sync for per-player pool state. Synced on change.

**Acceptance Criteria:**
- [ ] FluidPoolSyncMessage struct: owner, total_generated, total_consumed, surplus, reservoir_level, state
- [ ] Sent when pool values change (generation, consumption, state)
- [ ] Size: ~20 bytes per player (includes reservoir data)
- [ ] All players receive all pool states (rivals visible)
- [ ] Unit tests for message serialization

**Dependencies:**
- Blocked by: 6-006 (PerPlayerFluidPool)
- Blocks: None

**Agent Notes:**
- Systems Architect: All players see all fluid states for strategic visibility.
- Game Designer: Watching rival's reservoirs drain during crisis creates tension.

---

### 6-038: Integration with Epic 4 BuildingSystem (IFluidProvider)

**Type:** feature
**System:** Integration
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement IFluidProvider interface and integrate with Epic 4's BuildingSystem, replacing stub.

**Acceptance Criteria:**
- [ ] IFluidProvider interface implemented by FluidSystem:
  - has_fluid(EntityID entity) const -> bool
  - has_fluid_at(int32_t x, int32_t y, PlayerID owner) const -> bool
  - get_pool_state(PlayerID owner) const -> FluidPoolState
  - get_pool(PlayerID owner) const -> const PerPlayerFluidPool&
- [ ] BuildingSystem accepts IFluidProvider* via setter injection
- [ ] FluidSystem registered as the IFluidProvider
- [ ] BuildingSystem queries has_fluid() for structure state decisions
- [ ] NO grace period for fluid (per CCR-006) - reservoir buffer serves this purpose
- [ ] Building state machine responds to fluid changes (development blocked without fluid)
- [ ] Integration tests verifying full pipeline

**Dependencies:**
- Blocked by: 6-009 (FluidSystem skeleton with interface)
- External: Epic 4 BuildingSystem, IFluidProvider stub (ticket 4-020)

**Agent Notes:**
- Systems Architect: Key integration point. Epic 4 stub replaced by real implementation.
- Game Designer: "Buildings require fluid to develop" - core gameplay constraint.

---

### Group G: Content and Design

---

### 6-039: Define Fluid Requirements Per Structure Template

**Type:** design
**System:** Content
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define fluid_required values for all structure types/templates. Per CCR-007, values MATCH energy requirements exactly.

**Acceptance Criteria:**
- [ ] Fluid requirements defined per zone type and density (matching energy):
  - Low-density habitation: 5 units
  - High-density habitation: 20 units
  - Low-density exchange: 10 units
  - High-density exchange: 40 units
  - Low-density fabrication: 15 units
  - High-density fabrication: 60 units
- [ ] Service building requirements: 20-50 units depending on type (match energy)
- [ ] Infrastructure fluid requirements: conduits 0, reservoirs 0 (passive), extractors 0 (they produce)
- [ ] Values stored in template system (configurable)
- [ ] Documentation of all values

**Dependencies:**
- Blocked by: None
- Blocks: 6-002 (FluidComponent initialized from these values)
- External: Epic 4 template system

**Agent Notes:**
- Game Designer: Identical to energy - "building needs X energy AND X fluid" is intuitive.
- Services Engineer: Template system populates fluid_required at structure creation.

---

### 6-040: Balance Extractor/Reservoir Stats

**Type:** content
**System:** Content
**Scope:** S
**Priority:** P1 (important)

**Description:**
Document and configure extractor and reservoir balance values from CCR discussions.

**Acceptance Criteria:**
- [ ] Extractor values documented from 6-023:
  - base_output: 100 units (powers ~20 low-density habitation)
  - placement_distance: 8 tiles
  - operational_distance: 5 tiles
  - energy_required: 20 units
- [ ] Reservoir values documented from 6-024:
  - capacity: 1000 units (~10 ticks buffer at 100 consumption)
  - fill_rate: 50 units/tick
  - drain_rate: 100 units/tick
- [ ] Playtest notes documenting rationale for chosen values
- [ ] Unit tests verify balance parameters

**Dependencies:**
- Blocked by: 6-023, 6-024 (type definitions)
- Blocks: None

**Agent Notes:**
- Game Designer: Balance for "meaningful choices" - one extractor isn't enough, reservoirs matter.
- Services Engineer: All values exposed for runtime tuning.

---

### 6-041: Balance Deficit/Collapse Thresholds

**Type:** content
**System:** Content
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document and configure threshold values for pool state transitions.

**Acceptance Criteria:**
- [ ] Threshold values documented:
  - buffer_threshold: 10% of available (Healthy threshold)
  - collapse_threshold: reservoirs empty AND surplus < 0
- [ ] Values configurable for balancing
- [ ] Playtest notes documenting rationale for chosen values
- [ ] Unit tests verify threshold behavior

**Dependencies:**
- Blocked by: 6-018 (pool state machine)
- Blocks: None

**Agent Notes:**
- Game Designer: Collapse = reservoirs empty, not just deficit. Gives buffer time.
- Services Engineer: All values should be exposed for runtime tuning.

---

### 6-042: Fluid Overlay UI Requirements

**Type:** design
**System:** Integration
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document UI requirements for fluid overlay (implementation in Epic 12).

**Acceptance Criteria:**
- [ ] Requirements documented for Epic 12:
  - Coverage zones colored by owner (blue/cyan palette)
  - Hydrated vs dehydrated structure indicators
  - Conduit glow states (active flow, inactive, disconnected)
  - Reservoir fill level visualization
  - Deficit/collapse warnings
  - Pool status readout with reservoir buffer
- [ ] Data queries needed from FluidSystem identified
- [ ] Performance requirements (overlay should not drop framerate)
- [ ] Visual distinction from energy overlay (cool blues vs warm ambers)

**Dependencies:**
- Blocked by: 6-013 (coverage queries)
- Blocks: None (Epic 12 will implement)

**Agent Notes:**
- Game Designer: Fluid overlay uses cool colors (blue/cyan) vs energy's warm colors.
- Systems Architect: UI queries coverage via get_coverage_at() per visible cell.

---

### Group H: Testing and Validation

---

### 6-043: FluidSystem Integration Test Suite

**Type:** feature
**System:** Testing
**Scope:** L
**Priority:** P1 (important)

**Description:**
Comprehensive integration tests for FluidSystem verifying full pipeline.

**Acceptance Criteria:**
- [ ] Test: Place extractor near water, verify pool generation increases
- [ ] Test: Extractor far from water produces reduced output (efficiency curve)
- [ ] Test: Unpowered extractor produces nothing
- [ ] Test: Place structure with FluidComponent, verify pool consumption increases
- [ ] Test: Structure in coverage, pool surplus -> has_fluid = true
- [ ] Test: Structure outside coverage -> has_fluid = false
- [ ] Test: Pool deficit -> ALL structures lose fluid (no rationing)
- [ ] Test: Reservoir fills during surplus
- [ ] Test: Reservoir drains during deficit, delays collapse
- [ ] Test: Proportional drain across multiple reservoirs
- [ ] Test: Conduit placement extends coverage
- [ ] Test: Conduit removal contracts coverage
- [ ] Test: Coverage doesn't cross ownership boundaries
- [ ] Test: Event emission for all state changes
- [ ] Performance test: tick() < 2ms at 10,000 consumers

**Dependencies:**
- Blocked by: All feature tickets
- Blocks: None

**Agent Notes:**
- Systems Architect: Integration tests verify the full system works as designed.
- Services Engineer: Performance test ensures scalability targets are met.

---

### 6-044: Multiplayer Sync Verification Tests

**Type:** feature
**System:** Testing
**Scope:** M
**Priority:** P1 (important)

**Description:**
Tests verifying multiplayer sync correctness for fluid system.

**Acceptance Criteria:**
- [ ] Test: has_fluid state matches between server and clients
- [ ] Test: All-or-nothing distribution is consistent (no rationing differences)
- [ ] Test: Pool state transitions sync correctly
- [ ] Test: Reservoir levels sync correctly
- [ ] Test: Coverage reconstruction on client matches server
- [ ] Test: Rival fluid states visible to all players
- [ ] Test: Reconnection restores correct fluid state

**Dependencies:**
- Blocked by: 6-036, 6-037 (network serialization)
- Blocks: None

**Agent Notes:**
- Systems Architect: Multiplayer consistency is critical. No rationing simplifies sync.
- Services Engineer: Client coverage reconstruction must match server calculation.

---

## Dependency Graph Summary

**Critical Path (P0 chain):**
1. 6-001 (enums) -> 6-002, 6-003, 6-004, 6-005, 6-006 (components) -> 6-008 (coverage grid) -> 6-009 (system skeleton)
2. 6-009 -> 6-010 (coverage BFS) -> 6-013 (coverage queries) -> 6-019 (distribution)
3. 6-014, 6-015, 6-016 -> 6-017 (pool calculation) -> 6-018 (state machine + reservoir buffering) -> 6-019 (distribution)
4. 6-009 -> 6-038 (IFluidProvider integration with Epic 4)
5. 6-029 (conduit placement) -> 6-030, 6-031 (conduit mechanics)

**External Dependencies:**
- Epic 3: ITerrainQueryable (for water proximity queries, placement validation)
- Epic 4: BuildingConstructedEvent, BuildingDeconstructedEvent, IFluidProvider stub (4-020)
- Epic 5: IEnergyProvider (for extractor power state queries)

**Downstream Dependents (future epics that will consume Epic 6):**
- Epic 4 BuildingSystem: Uses IFluidProvider.has_fluid() for development gating
- Epic 9 (Services): Service structures query IFluidProvider.has_fluid()
- Epic 10 (PopulationSystem): Fluid state affects habitability and being vitality
- Epic 11 (EconomySystem): Extractor/reservoir build/maintenance costs
- Epic 12 (UISystem): Fluid overlay, pool status display, reservoir levels

---

## Canon Update Flags (for Phase 6)

The following canon updates are flagged from the discussion document:

| File | Change | Rationale |
|------|--------|-----------|
| interfaces.yaml | Add IFluidProvider interface | Mirror IEnergyProvider pattern for fluid state queries |
| systems.yaml | Add FluidSystem tick_priority: 20 | Document tick ordering (after EnergySystem at 10) |
| patterns.yaml | Add FluidCoverageGrid to dense_grid_exception applies_to | Follows same pattern as CoverageGrid |

---

**Document Status:** Ready for Canon Verification (Phase 5)

**Next Steps:**
1. Systems Architect reviews tickets against canon
2. Flag any canon conflicts as decisions
3. Apply canon updates via `/update-canon`
