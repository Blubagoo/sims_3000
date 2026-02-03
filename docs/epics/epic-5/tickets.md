# Epic 5: Energy Infrastructure (EnergySystem) - Tickets

**Epic Goal:** Implement the energy matrix (power grid) using the canonical pool model: per-overseer energy pools fed by energy nexuses, with coverage zones defined by energy conduits, enabling structures to draw from the pool when in coverage and pool has surplus
**Created:** 2026-01-28
**Canon Version:** 0.13.0
**Planning Agents:** Systems Architect, Game Designer, Services Engineer

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-28 | v0.6.0 | Initial ticket creation from three-agent analysis and resolved discussion (CCR-001 through CCR-010) |
| 2026-01-29 | canon-verification (v0.7.0 → v0.13.0) | No changes required |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. IEnergyProvider interface (ticket 5-009) matches interfaces.yaml. CoverageGrid in dense_grid_exception pattern. Energy overlay deferred to Epic 12 correctly. Priority 10 tick order confirmed in systems.yaml.

---

## Summary

Epic 5 implements the first major tension system in the game. Energy infrastructure transforms the peaceful act of zoning into a living colony that requires care and attention. The bioluminescent art direction and energy mechanics are perfectly aligned - energy literally powers the visual identity of the colony.

**Key Design Decisions (Resolved via Discussion):**
- CCR-001: Pool model confirmed -- per-overseer pools, no physical flow simulation. Conduits define coverage zones, not transport routes
- CCR-002: Binary power model -- structures are fully powered or completely unpowered, no partial power
- CCR-003: 4 priority levels -- Critical(1)=medical/command, Important(2)=enforcer/hazard, Normal(3)=exchange/fabrication, Low(4)=habitation
- CCR-004: 6 nexus types for MVP -- Carbon, Petrochemical, Gaseous, Nuclear, Wind, Solar (Hydro/Geothermal/Fusion/Microwave deferred)
- CCR-005: Dense coverage grid -- 1 byte per cell, BFS flood-fill, dirty flag per overseer, client reconstructs locally
- CCR-006: Asymptotic aging -- nexuses age toward type-specific floor (60-90%), never reach 0%
- CCR-007: Contamination only when online -- offline nexus (is_online=false or current_output=0) produces no contamination
- CCR-008: Energy state sync -- is_powered every tick, pool summary on change, all players see all states
- CCR-009: Grace period -- 100 ticks (5 seconds) before unpowered structure transitions to degraded state
- CCR-010: Coverage preview -- show delta (new coverage tiles) during conduit placement

**Target Metrics:**
- Coverage recalculation: <10ms for 512x512 map with 5,000 conduits
- Pool calculation: <1ms for 10,000 consumers
- Full tick: <2ms for 256x256 map
- EnergyComponent: 12 bytes; EnergyProducerComponent: 24 bytes; EnergyConduitComponent: 4 bytes
- Coverage grid memory: 64KB (256x256) to 256KB (512x512)

---

## Ticket Counts

| Category | Count |
|----------|-------|
| **By Type** | |
| Infrastructure | 15 |
| Feature | 28 |
| Content | 3 |
| Design | 2 |
| **By System** | |
| EnergySystem Core | 18 |
| Coverage System | 8 |
| Nexus Mechanics | 10 |
| Conduit Mechanics | 5 |
| Integration | 7 |
| **By Scope** | |
| S | 18 |
| M | 24 |
| L | 6 |
| **By Priority** | |
| P0 (critical path) | 22 |
| P1 (important) | 18 |
| P2 (enhancement) | 8 |
| **Total** | **48** |

---

## Ticket List

### Group A: Data Types and Enumerations

---

### 5-001: Energy Type Enumerations

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define foundational energy data type enumerations. NexusType enum with 6 MVP types per CCR-004 (Carbon, Petrochemical, Gaseous, Nuclear, Wind, Solar) plus 4 deferred types (Hydro, Geothermal, Fusion, MicrowaveReceiver). EnergyPoolState enum (Healthy, Marginal, Deficit, Collapse). TerrainRequirement enum for nexus placement validation.

**Acceptance Criteria:**
- [ ] NexusType enum: Carbon(0), Petrochemical(1), Gaseous(2), Nuclear(3), Wind(4), Solar(5), Hydro(6), Geothermal(7), Fusion(8), MicrowaveReceiver(9)
- [ ] EnergyPoolState enum: Healthy(0), Marginal(1), Deficit(2), Collapse(3)
- [ ] TerrainRequirement enum: None(0), Water(1), EmberCrust(2), Ridges(3)
- [ ] All enums use canonical alien terminology (nexus not plant, energy not power)
- [ ] Unit tests for enum value ranges

**Dependencies:**
- Blocked by: None (foundational)
- Blocks: 5-002, 5-003, 5-004, 5-005, 5-013, 5-023

**Agent Notes:**
- Systems Architect: NexusType includes deferred types for forward compatibility. Actual implementation gated by ticket scope.
- Game Designer: 6 MVP types provide sufficient variety for strategic choices without overwhelming players.

---

### 5-002: EnergyComponent Structure

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the EnergyComponent struct -- attached to all structures that consume energy (zone structures, service structures, fluid extractors, rail terminals). Per CCR-002 and CCR-003, 12 bytes canonical with 4 priority levels.

**Acceptance Criteria:**
- [ ] EnergyComponent struct defined:
  - energy_required (uint32_t): energy units needed per tick, from template
  - energy_received (uint32_t): energy units actually received this tick
  - is_powered (bool): true if energy_received >= energy_required
  - priority (uint8_t): rationing priority 1-4 per CCR-003
  - grid_id (uint8_t): future isolated grid support
  - padding (uint8_t): alignment
- [ ] Struct size is exactly 12 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] Priority constants: ENERGY_PRIORITY_CRITICAL=1, IMPORTANT=2, NORMAL=3, LOW=4
- [ ] Unit tests for default initialization and size assertion

**Dependencies:**
- Blocked by: 5-001 (may reference related enums)
- Blocks: 5-008, 5-018, 5-019, 5-029, 5-034

**Agent Notes:**
- Services Engineer: Binary power model per CCR-002. energy_received may equal 0 during rationing even if energy_required > 0.
- Systems Architect: 12-byte size enables tight ECS packing. grid_id reserved for future isolated grid scenarios.

---

### 5-003: EnergyProducerComponent Structure

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the EnergyProducerComponent struct -- attached to energy nexuses that generate energy. Includes output calculation fields, aging, and contamination output.

**Acceptance Criteria:**
- [ ] EnergyProducerComponent struct defined:
  - base_output (uint32_t): maximum output at 100% efficiency
  - current_output (uint32_t): actual output this tick = base_output * efficiency * age_factor
  - efficiency (float): current efficiency multiplier 0.0-1.0
  - age_factor (float): aging degradation, starts 1.0, decays per CCR-006
  - ticks_since_built (uint16_t): age in ticks, capped at 65535
  - nexus_type (uint8_t): NexusType enum value
  - is_online (bool): true if operational
  - contamination_output (uint32_t): contamination units per tick, 0 for clean nexuses
- [ ] Struct size is 24 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] Unit tests for default initialization and output calculation

**Dependencies:**
- Blocked by: 5-001 (NexusType enum)
- Blocks: 5-008, 5-010, 5-022, 5-023, 5-025

**Agent Notes:**
- Services Engineer: current_output = base_output * efficiency * age_factor. Contamination is per-nexus, not system-level.
- Systems Architect: age_factor uses asymptotic curve per CCR-006 (never reaches 0).

---

### 5-004: EnergyConduitComponent Structure

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the EnergyConduitComponent struct -- attached to energy conduit entities that extend coverage zones. Conduits do NOT transport energy; they define coverage area.

**Acceptance Criteria:**
- [ ] EnergyConduitComponent struct defined:
  - coverage_radius (uint8_t): tiles of coverage this conduit adds (default 2-3)
  - is_connected (bool): true if connected to energy matrix via BFS
  - is_active (bool): true if carrying energy (for rendering glow)
  - conduit_level (uint8_t): 1=basic, 2=upgraded (future expansion)
- [ ] Struct size is 4 bytes (verified with static_assert)
- [ ] Trivially copyable for serialization
- [ ] Unit tests for default initialization

**Dependencies:**
- Blocked by: None (standalone data type)
- Blocks: 5-014, 5-026, 5-027, 5-028

**Agent Notes:**
- Services Engineer: Conduits extend coverage, NOT transport energy. is_connected set by coverage algorithm.
- Game Designer: Conduits as "veins" of the colony -- visual satisfaction in placement.

---

### 5-005: PerPlayerEnergyPool Structure

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define the PerPlayerEnergyPool struct -- aggregated energy pool data per overseer. Includes generation, consumption, surplus, state, and counts.

**Acceptance Criteria:**
- [ ] PerPlayerEnergyPool struct defined:
  - owner (PlayerID): overseer who owns this pool
  - total_generated (uint32_t): sum of all nexus current_output
  - total_consumed (uint32_t): sum of all consumer energy_required in coverage
  - surplus (int32_t): generated - consumed (can be negative)
  - nexus_count (uint32_t): number of active nexuses
  - consumer_count (uint32_t): number of consumers in coverage
  - state (EnergyPoolState): Healthy/Marginal/Deficit/Collapse
  - previous_state (EnergyPoolState): for transition detection
- [ ] Struct supports negative surplus (deficit condition)
- [ ] Unit tests for surplus calculation and state determination

**Dependencies:**
- Blocked by: 5-001 (EnergyPoolState enum)
- Blocks: 5-012, 5-013, 5-033

**Agent Notes:**
- Systems Architect: Per-overseer pools per canon patterns.yaml. Energy does NOT flow between overseers.
- Services Engineer: surplus = total_generated - total_consumed. State thresholds configurable.

---

### 5-006: Energy Event Definitions

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define all energy-related event structs for cross-system communication and network sync.

**Acceptance Criteria:**
- [ ] EnergyStateChangedEvent: entity_id, owner_id, was_powered, is_powered
- [ ] EnergyDeficitBeganEvent: owner_id, deficit_amount, affected_consumers count
- [ ] EnergyDeficitEndedEvent: owner_id, surplus_amount
- [ ] GridCollapseBeganEvent: owner_id, deficit_amount
- [ ] GridCollapseEndedEvent: owner_id
- [ ] ConduitPlacedEvent: entity_id, owner_id, grid_position
- [ ] ConduitRemovedEvent: entity_id, owner_id, grid_position
- [ ] NexusPlacedEvent: entity_id, owner_id, nexus_type, grid_position
- [ ] NexusRemovedEvent: entity_id, owner_id, grid_position
- [ ] NexusAgedEvent: entity_id, owner_id, new_efficiency
- [ ] All events follow canon event pattern (suffix: Event)
- [ ] Unit tests verifying struct completeness

**Dependencies:**
- Blocked by: 5-001 (NexusType for event fields)
- Blocks: 5-020, 5-021, 5-022, 5-027, 5-028

**Agent Notes:**
- Systems Architect: Events are primary notification mechanism. Server broadcasts to all clients.
- Game Designer: EnergyStateChangedEvent drives visual feedback (glow on/off).

---

### Group B: Coverage System

---

### 5-007: CoverageGrid Dense 2D Array

**Type:** infrastructure
**System:** Coverage System
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the CoverageGrid class -- a dense 2D grid storing coverage owner per cell for O(1) coverage queries. Per CCR-005, follows dense_grid_exception pattern (like TerrainGrid). Stores uint8_t per cell: 0=uncovered, 1-4=overseer_id who has coverage.

**Acceptance Criteria:**
- [ ] CoverageGrid class with width, height, and flat vector<uint8_t> storage
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
- Blocks: 5-008, 5-014, 5-017

**Agent Notes:**
- Services Engineer: Dense grid justified by O(1) lookups for 10,000+ structures every tick. Matches canon dense_grid_exception pattern.
- Systems Architect: Canon update needed: add CoverageGrid to dense_grid_exception applies_to in patterns.yaml.

---

### 5-008: EnergySystem Class Skeleton and ISimulatable

**Type:** infrastructure
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define the EnergySystem class implementing ISimulatable at priority 10 (early in tick). Constructor takes ITerrainQueryable* (Epic 3). tick() method orchestrates coverage, pool calculation, distribution, and events. Declare all query methods and IEnergyProvider interface methods.

**Acceptance Criteria:**
- [ ] EnergySystem class declaration with ISimulatable implementation
- [ ] get_priority() returns 10 (energy runs before zones/buildings)
- [ ] Constructor accepts ITerrainQueryable* via dependency injection
- [ ] tick() method stub for full processing pipeline
- [ ] Internal CoverageGrid instance
- [ ] Per-overseer PerPlayerEnergyPool array
- [ ] Per-overseer dirty flags for coverage (bool array)
- [ ] All query method declarations
- [ ] Compiles and registers as a system

**Dependencies:**
- Blocked by: 5-002, 5-003, 5-004, 5-005, 5-007 (all component types and coverage grid)
- Blocks: 5-009, 5-010, 5-011, 5-012, 5-014, 5-018, 5-029
- External: Epic 3 (ITerrainQueryable interface)

**Agent Notes:**
- Systems Architect: Priority 10 ensures energy state is settled before ZoneSystem (30) and BuildingSystem (40).
- Services Engineer: System skeleton enables parallel development of subsystems.

---

### 5-009: IEnergyProvider Interface Implementation

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement the IEnergyProvider interface, replacing Epic 4's stub (ticket 4-019). EnergySystem provides real energy state queries.

**Acceptance Criteria:**
- [ ] IEnergyProvider interface implemented by EnergySystem:
  - is_powered(EntityID entity) const -> bool: check entity's EnergyComponent.is_powered
  - is_powered_at(int32_t x, int32_t y, PlayerID owner) const -> bool: coverage + pool check
- [ ] Epic 4's StubEnergyProvider can be swapped for EnergySystem via setter injection
- [ ] Additional methods for energy-specific queries:
  - get_energy_required(EntityID) const -> uint32_t
  - get_energy_received(EntityID) const -> uint32_t
  - get_pool_state(PlayerID owner) const -> EnergyPoolState
  - get_pool(PlayerID owner) const -> const PerPlayerEnergyPool&
- [ ] Unit tests verifying interface compliance
- [ ] Integration test with Epic 4's BuildingSystem

**Dependencies:**
- Blocked by: 5-008 (EnergySystem skeleton)
- Blocks: 5-031
- External: Epic 4 ticket 4-019 (IEnergyProvider stub definition)

**Agent Notes:**
- Systems Architect: This is the key integration point with Epic 4. Interface must match 4-019's definition exactly.
- Services Engineer: is_powered_at combines coverage check + pool surplus check.

---

### 5-014: Coverage Zone BFS Flood-Fill Algorithm

**Type:** feature
**System:** Coverage System
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement the BFS flood-fill algorithm for coverage zone calculation per CCR-005. Coverage propagates from nexuses through connected conduits. Each nexus and conduit has a coverage_radius.

**Acceptance Criteria:**
- [ ] recalculate_coverage(PlayerID owner) implements full BFS
- [ ] Step 1: Clear coverage for owner
- [ ] Step 2: Seed from all nexuses owned by player (mark radius around each, add to frontier)
- [ ] Step 3: BFS through 4-directional neighbors
- [ ] Step 4: If neighbor has conduit owned by same player, mark conduit's radius, add to frontier
- [ ] Step 5: Continue until frontier empty
- [ ] mark_coverage_radius(x, y, radius, owner) marks square area around position
- [ ] Ownership boundary enforcement: only extend to tiles owned by same player or GAME_MASTER
- [ ] visited set prevents infinite loops
- [ ] Performance target: <10ms for 512x512 with 5,000 conduits
- [ ] Unit tests for simple networks, complex networks, isolated segments

**Dependencies:**
- Blocked by: 5-007 (CoverageGrid), 5-004 (EnergyConduitComponent), 5-003 (EnergyProducerComponent)
- Blocks: 5-015, 5-016, 5-017, 5-032

**Agent Notes:**
- Services Engineer: BFS is O(conduits), not O(grid cells). visited set prevents revisits in complex networks.
- Systems Architect: This is NOT pathfinding. Coverage is binary: in-zone or not.

---

### 5-015: Coverage Dirty Flag Tracking

**Type:** feature
**System:** Coverage System
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement dirty flag tracking for coverage recalculation optimization. Coverage only recalculates when the network changes (conduit/nexus placed or removed).

**Acceptance Criteria:**
- [ ] bool coverage_dirty_[MAX_PLAYERS] array in EnergySystem
- [ ] on_conduit_placed(ConduitPlacedEvent) sets dirty flag for owner
- [ ] on_conduit_removed(ConduitRemovedEvent) sets dirty flag for owner
- [ ] on_nexus_placed(NexusPlacedEvent) sets dirty flag for owner
- [ ] on_nexus_removed(NexusRemovedEvent) sets dirty flag for owner
- [ ] tick() phase 1 iterates overseers, recalculates only if dirty, clears flag
- [ ] Per CCR-015: only dirty owning overseer, coverage doesn't cross boundaries
- [ ] Unit tests for dirty flag setting and clearing

**Dependencies:**
- Blocked by: 5-014 (coverage algorithm), 5-006 (events)
- Blocks: None (optimization)

**Agent Notes:**
- Services Engineer: Dirty tracking limits recalculation to actual changes. Critical for performance at scale.
- Systems Architect: Adjacent overseers are unaffected by another player's conduit changes.

---

### 5-016: Ownership Boundary Enforcement in Coverage

**Type:** feature
**System:** Coverage System
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement ownership boundary enforcement in coverage calculation. Per canon patterns.yaml, energy conduits do NOT connect across overseer boundaries.

**Acceptance Criteria:**
- [ ] can_extend_coverage_to(x, y, owner) returns bool
- [ ] Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
- [ ] Returns false if tile_owner belongs to different player
- [ ] Coverage BFS uses this check before extending to neighbor
- [ ] Conduits adjacent to boundary are marked is_connected but coverage stops at boundary
- [ ] Unit tests for boundary scenarios, adjacent overseer networks

**Dependencies:**
- Blocked by: 5-014 (coverage algorithm)
- Blocks: None

**Agent Notes:**
- Systems Architect: Explicit boundary check prevents accidental coverage sharing between overseers.
- Game Designer: Each overseer maintains their own energy matrix -- no griefing via energy denial.

---

### 5-017: Coverage Queries

**Type:** feature
**System:** Coverage System
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Implement public coverage query methods for other systems and UI overlay.

**Acceptance Criteria:**
- [ ] is_in_coverage(x, y, owner) const -> bool: O(1) query via CoverageGrid
- [ ] get_coverage_at(x, y) const -> PlayerID: returns covering owner or 0
- [ ] get_coverage_count(owner) const -> uint32_t: count of covered cells for owner
- [ ] All queries are const and thread-safe for UI reads
- [ ] Unit tests for all query methods

**Dependencies:**
- Blocked by: 5-007 (CoverageGrid), 5-014 (coverage algorithm)
- Blocks: 5-018, 5-036

**Agent Notes:**
- Services Engineer: O(1) queries essential for per-tick consumer iteration.
- Systems Architect: UI overlay will call get_coverage_at() for each visible cell.

---

### Group C: Pool Calculation and Distribution

---

### 5-010: Nexus Registration and Output Calculation

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement nexus registration (tracking all nexuses per overseer) and per-tick output calculation. Output = base_output * efficiency * age_factor.

**Acceptance Criteria:**
- [ ] Internal list/map of nexuses per overseer
- [ ] register_nexus(EntityID, PlayerID) called when nexus placed
- [ ] unregister_nexus(EntityID, PlayerID) called when nexus removed
- [ ] update_nexus_output(EnergyProducerComponent&) calculates current_output
- [ ] If !is_online, current_output = 0
- [ ] tick() phase 2 iterates all nexuses and updates current_output
- [ ] Supports weather/day-night stubs for Wind/Solar (returns average factor)
- [ ] Unit tests for output calculation with various efficiency/age combinations

**Dependencies:**
- Blocked by: 5-008 (EnergySystem skeleton), 5-003 (EnergyProducerComponent)
- Blocks: 5-012, 5-022

**Agent Notes:**
- Services Engineer: Output calculation is simple multiplication. Weather stubs return 0.75 (average) for MVP.
- Systems Architect: Per-overseer nexus tracking enables efficient pool calculation.

---

### 5-011: Consumer Registration and Requirement Aggregation

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement consumer registration (tracking all energy consumers per overseer) and per-tick requirement aggregation. Only consumers in coverage contribute to total_consumed.

**Acceptance Criteria:**
- [ ] Internal list/map of consumers per overseer
- [ ] register_consumer(EntityID, PlayerID) called when structure with EnergyComponent placed
- [ ] unregister_consumer(EntityID, PlayerID) called when structure removed
- [ ] aggregate_consumption(PlayerID) returns total energy_required for consumers in coverage
- [ ] Only consumers where is_in_coverage(position, owner) == true are counted
- [ ] tick() phase 3 uses this to populate pool.total_consumed
- [ ] Unit tests for aggregation with mixed coverage scenarios

**Dependencies:**
- Blocked by: 5-008 (EnergySystem skeleton), 5-002 (EnergyComponent), 5-017 (coverage queries)
- Blocks: 5-012

**Agent Notes:**
- Services Engineer: Consumers outside coverage don't contribute to pool consumption (they're not connected).
- Systems Architect: Registration happens via BuildingConstructedEvent handler.

---

### 5-012: Pool Calculation (Generation - Consumption = Surplus)

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement per-overseer pool calculation. Sum generation from all online nexuses, sum consumption from covered consumers, calculate surplus.

**Acceptance Criteria:**
- [ ] calculate_pool(PlayerID) populates PerPlayerEnergyPool for overseer
- [ ] total_generated = SUM(nexus.current_output for online nexuses)
- [ ] total_consumed = SUM(consumer.energy_required for consumers in coverage)
- [ ] surplus = total_generated - total_consumed (can be negative)
- [ ] nexus_count and consumer_count updated
- [ ] tick() phase 3 calls calculate_pool() for each overseer
- [ ] Unit tests for healthy, marginal, deficit, and collapse scenarios

**Dependencies:**
- Blocked by: 5-010 (nexus output), 5-011 (consumer aggregation), 5-005 (PerPlayerEnergyPool)
- Blocks: 5-013, 5-018, 5-019

**Agent Notes:**
- Systems Architect: Pool calculation is the heart of the system. Simple arithmetic, no simulation.
- Services Engineer: Negative surplus = deficit state. Triggers rationing logic.

---

### 5-013: Pool State Machine (Healthy/Marginal/Deficit/Collapse)

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement pool state calculation and transition detection. States based on surplus thresholds.

**Acceptance Criteria:**
- [ ] calculate_pool_state(PerPlayerEnergyPool) returns EnergyPoolState
- [ ] Healthy: surplus >= buffer_threshold (10% of total_generated)
- [ ] Marginal: 0 <= surplus < buffer_threshold
- [ ] Deficit: -collapse_threshold < surplus < 0
- [ ] Collapse: surplus <= -collapse_threshold (50% of total_consumed)
- [ ] Thresholds are configurable constants
- [ ] detect_pool_state_transitions(PlayerID) compares previous_state to new state
- [ ] Emits EnergyDeficitBeganEvent, EnergyDeficitEndedEvent, GridCollapseBeganEvent, GridCollapseEndedEvent as appropriate
- [ ] Unit tests for all state transitions and threshold edge cases

**Dependencies:**
- Blocked by: 5-012 (pool calculation), 5-006 (events)
- Blocks: 5-021

**Agent Notes:**
- Services Engineer: State transitions drive UI notifications and visual changes.
- Game Designer: Warning phase (Marginal) gives players time to react before crisis.

---

### 5-018: Energy Distribution (Powered = In Coverage AND Pool Surplus)

**Type:** feature
**System:** EnergySystem Core
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement energy distribution for healthy/marginal pools. When surplus >= 0, all consumers in coverage are powered.

**Acceptance Criteria:**
- [ ] distribute_energy(PlayerID) sets is_powered for all consumers
- [ ] If pool.surplus >= 0: mark all consumers in coverage as powered
- [ ] energy_received = energy_required for powered consumers
- [ ] is_powered = true for powered consumers
- [ ] Consumers outside coverage: is_powered = false, energy_received = 0
- [ ] tick() phase 5 calls distribution logic
- [ ] Unit tests for full distribution with healthy pool

**Dependencies:**
- Blocked by: 5-012 (pool calculation), 5-002 (EnergyComponent), 5-017 (coverage queries)
- Blocks: 5-019, 5-020

**Agent Notes:**
- Services Engineer: Simple case: surplus >= 0 means everyone in coverage gets power.
- Systems Architect: This is the happy path. Rationing (5-019) handles deficit case.

---

### 5-019: Priority-Based Rationing During Deficit

**Type:** feature
**System:** EnergySystem Core
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Implement priority-based rationing when pool is in deficit. Sort consumers by priority, power high-priority first until energy runs out.

**Acceptance Criteria:**
- [ ] apply_rationing(PlayerID) handles deficit distribution per CCR-003
- [ ] Collect all consumers in coverage for overseer
- [ ] Sort by priority (ascending = higher priority first), then EntityID for deterministic tie-breaking
- [ ] Iterate sorted list, distribute available energy:
  - If available >= consumer.energy_required: power consumer, deduct from available
  - Else: consumer unpowered (energy_received = 0, is_powered = false)
- [ ] Priority 1 (Critical) powered first, then 2, 3, 4
- [ ] Deterministic order for multiplayer consistency
- [ ] Unit tests for rationing scenarios with different deficit levels

**Dependencies:**
- Blocked by: 5-018 (distribution), 5-012 (pool calculation)
- Blocks: None

**Agent Notes:**
- Services Engineer: EntityID tie-breaking ensures server and clients agree on rationing order.
- Game Designer: Medical/command structures stay lit during deficit. Habitation goes dark first.

---

### 5-020: EnergyStateChangedEvent Emission

**Type:** feature
**System:** EnergySystem Core
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement EnergyStateChangedEvent emission when a consumer's is_powered state changes.

**Acceptance Criteria:**
- [ ] Track previous is_powered state per consumer (or detect change during distribution)
- [ ] emit_state_change_events() compares previous to current state
- [ ] Emit EnergyStateChangedEvent for each consumer that changed
- [ ] Event includes entity_id, owner_id, was_powered, is_powered
- [ ] tick() phase 6 calls emission logic
- [ ] Unit tests for event emission on power on/off transitions

**Dependencies:**
- Blocked by: 5-018 (distribution), 5-006 (events)
- Blocks: None

**Agent Notes:**
- Systems Architect: RenderingSystem listens for these events to update glow state.
- Game Designer: State changes drive visual feedback (glow on/off animation).

---

### 5-021: Pool State Transition Event Emission

**Type:** feature
**System:** EnergySystem Core
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement pool state transition event emission (deficit began/ended, collapse began/ended).

**Acceptance Criteria:**
- [ ] Detect transitions in detect_pool_state_transitions(PlayerID)
- [ ] Emit EnergyDeficitBeganEvent when state transitions to Deficit from Healthy/Marginal
- [ ] Emit EnergyDeficitEndedEvent when state transitions from Deficit to Healthy/Marginal
- [ ] Emit GridCollapseBeganEvent when state transitions to Collapse
- [ ] Emit GridCollapseEndedEvent when state transitions from Collapse
- [ ] Events include relevant data (deficit amount, affected consumer count)
- [ ] Unit tests for all transition combinations

**Dependencies:**
- Blocked by: 5-013 (pool state machine), 5-006 (events)
- Blocks: None

**Agent Notes:**
- Services Engineer: Pool state events trigger UI notifications and audio cues.
- Game Designer: "Grid collapse" is a major crisis moment -- needs dramatic notification.

---

### Group D: Nexus Mechanics

---

### 5-022: Nexus Aging and Efficiency Degradation

**Type:** feature
**System:** Nexus Mechanics
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement nexus aging mechanics per CCR-006. Nexuses degrade over time but never reach 0% efficiency -- asymptotic approach to type-specific floor.

**Acceptance Criteria:**
- [ ] update_nexus_aging(EnergyProducerComponent&, delta_time) called each tick
- [ ] ticks_since_built incremented (capped at 65535)
- [ ] age_factor calculated using asymptotic curve: age_factor = floor + (1.0 - floor) * decay_curve(ticks)
- [ ] Type-specific floors per CCR-006:
  - Carbon: 60%
  - Petrochemical: 65%
  - Gaseous: 70%
  - Nuclear: 75%
  - Wind: 80%
  - Solar: 85%
- [ ] Decay curve configurable (e.g., exponential decay)
- [ ] NexusAgedEvent emitted when efficiency drops past threshold (e.g., every 10% drop)
- [ ] Unit tests for aging progression and floor limits

**Dependencies:**
- Blocked by: 5-010 (nexus output), 5-003 (EnergyProducerComponent), 5-006 (NexusAgedEvent)
- Blocks: None

**Agent Notes:**
- Services Engineer: Asymptotic curve prevents nexuses from becoming useless. Maintenance mechanics deferred to Epic 11.
- Game Designer: Aging creates ongoing management without harsh punishment.

---

### 5-023: Nexus Type Definitions and Base Stats

**Type:** content
**System:** Nexus Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define configuration data for all 6 MVP nexus types per CCR-004. Includes output, costs, contamination, coverage radius, lifespan.

**Acceptance Criteria:**
- [ ] NexusTypeConfig struct defined with all configuration fields
- [ ] NEXUS_CONFIGS array with values for 6 MVP types:
  - Carbon: 100 output, 5000 build, 50 maint, 200 contamination, 8 cov radius
  - Petrochemical: 150 output, 8000 build, 75 maint, 120 contamination, 8 cov radius
  - Gaseous: 120 output, 10000 build, 60 maint, 40 contamination, 8 cov radius
  - Nuclear: 400 output, 50000 build, 200 maint, 0 contamination, 10 cov radius
  - Wind: 30 output (variable), 3000 build, 20 maint, 0 contamination, 4 cov radius
  - Solar: 50 output (variable), 6000 build, 30 maint, 0 contamination, 6 cov radius
- [ ] Values configurable (not hardcoded) for balancing
- [ ] Unit tests verifying configuration completeness

**Dependencies:**
- Blocked by: 5-001 (NexusType enum)
- Blocks: 5-010, 5-014, 5-025, 5-038

**Agent Notes:**
- Game Designer: Values are starting points for playtesting. Carbon nexus powers ~20 low-density habitation.
- Services Engineer: All values should be loaded from config file for easy balancing.

---

### 5-024: Terrain Bonus Efficiency (Ridges for Wind)

**Type:** feature
**System:** Nexus Mechanics
**Scope:** M
**Priority:** P2 (enhancement)

**Description:**
Implement terrain-based efficiency bonuses for certain nexus types. Wind nexuses get efficiency bonus on elevated terrain (ridges).

**Acceptance Criteria:**
- [ ] get_terrain_efficiency_bonus(NexusType, x, y) returns float multiplier
- [ ] Wind nexuses: +20% efficiency on elevated terrain
- [ ] Queries ITerrainQueryable for terrain type/elevation
- [ ] Bonus applied in update_nexus_output calculation
- [ ] Unit tests for terrain bonus scenarios

**Dependencies:**
- Blocked by: 5-010 (nexus output), 5-023 (nexus types)
- External: Epic 3 ITerrainQueryable

**Agent Notes:**
- Game Designer: Terrain bonuses reward strategic placement decisions.
- Services Engineer: Elevation queries may need interface extension.

---

### 5-025: IContaminationSource Implementation for Dirty Nexuses

**Type:** feature
**System:** Nexus Mechanics
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement IContaminationSource interface for polluting nexuses (Carbon, Petrochemical, Gaseous). Per CCR-007, contamination only emitted when online.

**Acceptance Criteria:**
- [ ] IContaminationSource interface defined:
  - get_contamination_output() const -> uint32_t
  - get_contamination_type() const -> ContaminationType
- [ ] EnergySystem provides query method: get_contamination_sources() -> vector<ContaminationSourceData>
- [ ] Only returns nexuses where is_online=true AND current_output>0
- [ ] ContaminationType::Energy for all energy nexuses
- [ ] ContaminationSourceData includes position, output, radius
- [ ] Unit tests for online/offline contamination behavior

**Dependencies:**
- Blocked by: 5-003 (EnergyProducerComponent), 5-023 (nexus types with contamination values)
- Blocks: None
- External: Epic 10 ContaminationSystem will consume this interface

**Agent Notes:**
- Services Engineer: Offline nexus = no combustion = no contamination. Makes gameplay sense.
- Systems Architect: Interface designed for Epic 10 consumption. Minimal coupling.

---

### 5-026: Nexus Placement Validation

**Type:** feature
**System:** Nexus Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement nexus placement validation including terrain requirements for specific types.

**Acceptance Criteria:**
- [ ] validate_nexus_placement(NexusType, x, y, owner) returns success/failure with reason
- [ ] Bounds check
- [ ] Ownership check (player owns tile)
- [ ] Terrain buildable check (ITerrainQueryable.is_buildable)
- [ ] No existing structure check
- [ ] Type-specific terrain requirements:
  - Hydro: within 3 tiles of water (stub: always valid for MVP)
  - Geothermal: on ember_crust terrain (stub: always valid for MVP)
  - Wind: ridges preferred but not required
- [ ] Unit tests for each validation step

**Dependencies:**
- Blocked by: 5-023 (nexus types with terrain requirements)
- External: Epic 3 ITerrainQueryable

**Agent Notes:**
- Services Engineer: Hydro/Geothermal validation stubbed for MVP (these types deferred).
- Game Designer: Nexus placement is a strategic decision. Clear feedback on valid locations.

---

### Group E: Conduit Mechanics

---

### 5-027: Energy Conduit Placement and Validation

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement energy conduit placement with validation. Conduits extend coverage zones.

**Acceptance Criteria:**
- [ ] validate_conduit_placement(x, y, owner) returns success/failure
- [ ] Bounds check
- [ ] Ownership check
- [ ] Terrain buildable check
- [ ] No existing structure check (conduit is infrastructure, not zone structure)
- [ ] place_conduit(x, y, owner) creates conduit entity with EnergyConduitComponent + PositionComponent + OwnershipComponent
- [ ] Emits ConduitPlacedEvent
- [ ] Sets coverage_dirty_[owner] = true
- [ ] Cost deducted (configurable, default 10 credits)
- [ ] Unit tests for placement validation and entity creation

**Dependencies:**
- Blocked by: 5-004 (EnergyConduitComponent), 5-006 (ConduitPlacedEvent), 5-015 (dirty tracking)
- Blocks: 5-028

**Agent Notes:**
- Services Engineer: Conduit placement triggers coverage recalculation via dirty flag.
- Game Designer: Low cost encourages building conduit networks. "Drawing glowing lines."

---

### 5-028: Conduit Connection Detection

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement conduit connection detection. A conduit's is_connected flag is set during coverage BFS if it's reachable from a nexus.

**Acceptance Criteria:**
- [ ] During coverage BFS (5-014), when visiting a conduit, set is_connected = true
- [ ] Before BFS, reset all conduits' is_connected to false for the owner
- [ ] Isolated conduits (not connected to any nexus network) remain is_connected = false
- [ ] is_connected used for visual feedback (dim vs bright conduit)
- [ ] Unit tests for connected and isolated conduit scenarios

**Dependencies:**
- Blocked by: 5-014 (coverage algorithm), 5-004 (EnergyConduitComponent)
- Blocks: None

**Agent Notes:**
- Services Engineer: Connection detection is a byproduct of BFS traversal.
- Game Designer: Isolated conduits appear dim -- visual feedback for "not working."

---

### 5-029: Conduit Removal and Coverage Update

**Type:** feature
**System:** Conduit Mechanics
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement conduit removal. Removing a conduit triggers coverage recalculation.

**Acceptance Criteria:**
- [ ] remove_conduit(EntityID) validates ownership and removes entity
- [ ] Emits ConduitRemovedEvent
- [ ] Sets coverage_dirty_[owner] = true
- [ ] No refund of placement cost
- [ ] Unit tests for removal and dirty flag setting

**Dependencies:**
- Blocked by: 5-027 (conduit placement), 5-006 (ConduitRemovedEvent)
- Blocks: None

**Agent Notes:**
- Services Engineer: Removal is straightforward. Dirty flag handles coverage update.
- Systems Architect: No cascade -- removing conduit doesn't affect structures, just coverage.

---

### 5-030: Conduit Active State for Rendering

**Type:** feature
**System:** Conduit Mechanics
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement conduit is_active state update for rendering. Conduits glow when actively carrying energy.

**Acceptance Criteria:**
- [ ] update_conduit_active_states() called in tick() phase 7
- [ ] Conduit is_active = true if is_connected AND owner's pool.total_generated > 0
- [ ] is_active = false if disconnected OR pool has no generation
- [ ] RenderingSystem reads is_active for glow effect
- [ ] Unit tests for active state based on pool and connection

**Dependencies:**
- Blocked by: 5-028 (connection detection), 5-012 (pool calculation)
- Blocks: None

**Agent Notes:**
- Services Engineer: Active state is visual only. Conduits glow when energy "flows."
- Game Designer: "Pulsing energy flow" visual reinforces the bioluminescent theme.

---

### 5-031: Conduit Placement Preview (Coverage Delta)

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
- Blocked by: 5-014 (coverage algorithm), 5-017 (coverage queries)
- Blocks: None

**Agent Notes:**
- Game Designer: Delta preview makes conduit placement feel like "painting coverage."
- Services Engineer: May need simplified preview algorithm for performance.

---

### Group F: Integration

---

### 5-032: BuildingConstructedEvent Handler (Register Consumer/Producer)

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement BuildingConstructedEvent handler to register new energy consumers and producers.

**Acceptance Criteria:**
- [ ] EnergySystem subscribes to BuildingConstructedEvent (from Epic 4)
- [ ] on_building_constructed(BuildingConstructedEvent&):
  - If entity has EnergyComponent: register_consumer(entity, owner)
  - If entity has EnergyProducerComponent: register_nexus(entity, owner), set coverage_dirty
- [ ] Consumers get is_powered set on next tick
- [ ] Unit tests for consumer and producer registration

**Dependencies:**
- Blocked by: 5-010 (nexus registration), 5-011 (consumer registration)
- External: Epic 4 BuildingConstructedEvent

**Agent Notes:**
- Systems Architect: Event-driven registration decouples EnergySystem from BuildingSystem.
- Services Engineer: New structures automatically participate in energy system.

---

### 5-033: BuildingDeconstructedEvent Handler (Unregister)

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Implement BuildingDeconstructedEvent handler to unregister consumers and producers.

**Acceptance Criteria:**
- [ ] EnergySystem subscribes to BuildingDeconstructedEvent (from Epic 4)
- [ ] on_building_deconstructed(BuildingDeconstructedEvent&):
  - If entity was consumer: unregister_consumer(entity, owner)
  - If entity was producer: unregister_nexus(entity, owner), set coverage_dirty
- [ ] Pool recalculated on next tick
- [ ] Unit tests for consumer and producer unregistration

**Dependencies:**
- Blocked by: 5-032 (registration handlers)
- External: Epic 4 BuildingDeconstructedEvent

**Agent Notes:**
- Services Engineer: Unregistration mirrors registration. Nexus removal triggers coverage recalc.
- Systems Architect: Destroyed nexuses stop contributing to pool immediately.

---

### 5-034: Network Serialization for EnergyComponent

**Type:** feature
**System:** Integration
**Scope:** M
**Priority:** P1 (important)

**Description:**
Implement network serialization for EnergyComponent. Per CCR-008, is_powered syncs every tick.

**Acceptance Criteria:**
- [ ] EnergyComponent serialization/deserialization methods
- [ ] is_powered synced every tick via standard component sync
- [ ] energy_received optionally synced (for UI display)
- [ ] energy_required only synced on entity creation (derived from template)
- [ ] Deterministic serialization for multiplayer consistency
- [ ] Unit tests for serialization round-trip

**Dependencies:**
- Blocked by: 5-002 (EnergyComponent)
- Blocks: None

**Agent Notes:**
- Systems Architect: is_powered is 1 bit per entity. Efficient for high-frequency sync.
- Services Engineer: energy_received optional -- UI can derive "powered percentage" if needed.

---

### 5-035: Network Serialization for Pool State

**Type:** feature
**System:** Integration
**Scope:** S
**Priority:** P1 (important)

**Description:**
Implement network sync for per-player pool state. Synced on change per CCR-008.

**Acceptance Criteria:**
- [ ] EnergyPoolSyncMessage struct: owner, total_generated, total_consumed, surplus, state
- [ ] Sent when pool values change (generation, consumption, state)
- [ ] Size: 16 bytes per player
- [ ] All players receive all pool states (rivals visible)
- [ ] Unit tests for message serialization

**Dependencies:**
- Blocked by: 5-005 (PerPlayerEnergyPool)
- Blocks: None

**Agent Notes:**
- Systems Architect: All players see all energy states for "schadenfreude" effect.
- Game Designer: Watching rival's colony go dark is part of the experience.

---

### 5-036: Integration with Epic 4 BuildingSystem (Replace Stub)

**Type:** feature
**System:** Integration
**Scope:** L
**Priority:** P0 (critical path)

**Description:**
Integrate EnergySystem with Epic 4's BuildingSystem by replacing the IEnergyProvider stub.

**Acceptance Criteria:**
- [ ] BuildingSystem accepts IEnergyProvider* via setter injection
- [ ] EnergySystem registered as the IEnergyProvider
- [ ] BuildingSystem queries is_powered() for structure state decisions
- [ ] Grace period (100 ticks per CCR-009) before building state change
- [ ] Building state machine responds to power changes
- [ ] Integration tests verifying full pipeline

**Dependencies:**
- Blocked by: 5-009 (IEnergyProvider implementation)
- External: Epic 4 BuildingSystem, ticket 4-019 stub

**Agent Notes:**
- Systems Architect: Key integration point. Epic 4 stub replaced by real implementation.
- Game Designer: 100 tick grace period prevents flickering building states.

---

### Group G: Content and Design

---

### 5-037: Define Energy Requirements Per Structure Template

**Type:** design
**System:** Content
**Scope:** M
**Priority:** P0 (critical path)

**Description:**
Define energy_required values for all structure types/templates. Per CCR-019 resolved values.

**Acceptance Criteria:**
- [ ] Energy requirements defined per zone type and density:
  - Low-density habitation: 5 units
  - High-density habitation: 20 units
  - Low-density exchange: 10 units
  - High-density exchange: 40 units
  - Low-density fabrication: 15 units
  - High-density fabrication: 60 units
- [ ] Service building requirements: 20-50 units depending on type
- [ ] Infrastructure energy requirements: conduits 0, nexuses 0 (they produce, not consume)
- [ ] Values stored in template system (configurable)
- [ ] Documentation of all values

**Dependencies:**
- Blocked by: None
- Blocks: 5-002 (EnergyComponent initialized from these values)
- External: Epic 4 template system

**Agent Notes:**
- Game Designer: Carbon nexus (100 units) powers ~20 low-density habitation or ~2 high-density.
- Services Engineer: Template system populates energy_required at structure creation.

---

### 5-038: Define Priority Assignments Per Structure Type

**Type:** design
**System:** Content
**Scope:** S
**Priority:** P0 (critical path)

**Description:**
Define energy priority per structure type per CCR-003.

**Acceptance Criteria:**
- [ ] Priority assignments documented:
  - Priority 1 (Critical): Medical nexus, command nexus
  - Priority 2 (Important): Enforcer post, hazard response post
  - Priority 3 (Normal): Exchange structures, fabrication structures, education, recreation
  - Priority 4 (Low): Habitation structures
- [ ] Priority values stored in template system
- [ ] Default priority (3) for any unspecified structure type
- [ ] Documentation of all assignments

**Dependencies:**
- Blocked by: None
- Blocks: 5-002 (EnergyComponent.priority initialized from these)

**Agent Notes:**
- Game Designer: Priority reflects survival importance. Medical stays on, habitation goes dark first.
- Services Engineer: Priority immutable per template. No player configuration per CCR-002 resolution.

---

### 5-039: Balance Deficit/Collapse Thresholds

**Type:** content
**System:** Content
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document and configure threshold values for pool state transitions.

**Acceptance Criteria:**
- [ ] Threshold values documented:
  - buffer_threshold: 10% of total_generated (Healthy threshold)
  - collapse_threshold: 50% of total_consumed (Collapse threshold)
- [ ] Values configurable for balancing
- [ ] Playtest notes documenting rationale for chosen values
- [ ] Unit tests verify threshold behavior

**Dependencies:**
- Blocked by: 5-013 (pool state machine)
- Blocks: None

**Agent Notes:**
- Game Designer: Thresholds create warning zone before crisis. 10% buffer is "running hot."
- Services Engineer: All values should be exposed for runtime tuning.

---

### 5-040: Energy Overlay UI Requirements

**Type:** design
**System:** Integration
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Document UI requirements for energy overlay (implementation in Epic 12).

**Acceptance Criteria:**
- [ ] Requirements documented for Epic 12:
  - Coverage zones colored by owner
  - Powered vs. unpowered structure indicators
  - Conduit glow states (active, inactive, disconnected)
  - Deficit/collapse warnings
  - Pool status readout
- [ ] Data queries needed from EnergySystem identified
- [ ] Performance requirements (overlay should not drop framerate)

**Dependencies:**
- Blocked by: 5-017 (coverage queries)
- Blocks: None (Epic 12 will implement)

**Agent Notes:**
- Game Designer: Energy overlay is essential, not optional. Players must always see energy state.
- Systems Architect: UI queries coverage via get_coverage_at() per visible cell.

---

### Group H: Testing and Validation

---

### 5-041: EnergySystem Integration Test Suite

**Type:** feature
**System:** Testing
**Scope:** L
**Priority:** P1 (important)

**Description:**
Comprehensive integration tests for EnergySystem verifying full pipeline.

**Acceptance Criteria:**
- [ ] Test: Place nexus, verify pool generation increases
- [ ] Test: Place structure with EnergyComponent, verify pool consumption increases
- [ ] Test: Structure in coverage, pool surplus -> is_powered = true
- [ ] Test: Structure outside coverage -> is_powered = false
- [ ] Test: Pool deficit -> priority rationing applied correctly
- [ ] Test: Conduit placement extends coverage
- [ ] Test: Conduit removal contracts coverage
- [ ] Test: Nexus offline -> no generation contribution
- [ ] Test: Nexus aging reduces output over time
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

### 5-042: Multiplayer Sync Verification Tests

**Type:** feature
**System:** Testing
**Scope:** M
**Priority:** P1 (important)

**Description:**
Tests verifying multiplayer sync correctness for energy system.

**Acceptance Criteria:**
- [ ] Test: is_powered state matches between server and clients
- [ ] Test: Rationing order is deterministic (same result on server and all clients)
- [ ] Test: Pool state transitions sync correctly
- [ ] Test: Coverage reconstruction on client matches server
- [ ] Test: Rival energy states visible to all players
- [ ] Test: Reconnection restores correct energy state

**Dependencies:**
- Blocked by: 5-034, 5-035 (network serialization)
- Blocks: None

**Agent Notes:**
- Systems Architect: Multiplayer consistency is critical. Deterministic rationing is key.
- Services Engineer: Client coverage reconstruction must match server calculation.

---

### 5-043: Performance Benchmark Suite

**Type:** feature
**System:** Testing
**Scope:** M
**Priority:** P1 (important)

**Description:**
Performance benchmarks for EnergySystem meeting target metrics.

**Acceptance Criteria:**
- [ ] Benchmark: Coverage recalculation at 512x512 with 5,000 conduits < 10ms
- [ ] Benchmark: Pool calculation with 10,000 consumers < 1ms
- [ ] Benchmark: Full tick at 256x256 map < 2ms
- [ ] Benchmark: Memory usage at 256x256 < 500KB total
- [ ] Benchmarks run in CI, fail if targets exceeded
- [ ] Historical tracking for regression detection

**Dependencies:**
- Blocked by: 5-014 (coverage algorithm), 5-012 (pool calculation)
- Blocks: None

**Agent Notes:**
- Services Engineer: Benchmarks ensure scalability as content grows.
- Systems Architect: CI integration prevents performance regressions.

---

### Group I: Documentation

---

### 5-044: EnergySystem Technical Documentation

**Type:** content
**System:** Documentation
**Scope:** M
**Priority:** P2 (enhancement)

**Description:**
Technical documentation for EnergySystem architecture and implementation.

**Acceptance Criteria:**
- [ ] Architecture overview with system diagram
- [ ] Component documentation (EnergyComponent, EnergyProducerComponent, EnergyConduitComponent)
- [ ] Pool model explanation with examples
- [ ] Coverage algorithm pseudocode and explanation
- [ ] Rationing algorithm explanation
- [ ] Interface documentation (IEnergyProvider, IContaminationSource)
- [ ] Integration points with other epics
- [ ] Configuration parameter reference

**Dependencies:**
- Blocked by: All feature tickets
- Blocks: None

**Agent Notes:**
- Systems Architect: Documentation enables future maintenance and epic handoffs.
- Services Engineer: Pseudocode helps verify implementation matches design.

---

### 5-045: Energy Balance Design Document

**Type:** content
**System:** Documentation
**Scope:** S
**Priority:** P2 (enhancement)

**Description:**
Game design documentation for energy balance and player experience.

**Acceptance Criteria:**
- [ ] Nexus type comparison table (output, cost, contamination, feel)
- [ ] Structure consumption reference table
- [ ] Expected early/mid/late game energy scenarios
- [ ] Crisis (deficit/collapse) experience guidelines
- [ ] Recovery gameplay flow description
- [ ] Multiplayer dynamics documentation
- [ ] Playtest feedback integration plan

**Dependencies:**
- Blocked by: 5-023, 5-037, 5-038 (content tickets)
- Blocks: None

**Agent Notes:**
- Game Designer: Balance doc is a living document updated from playtesting.
- Services Engineer: Cross-reference with technical doc for implementation alignment.

---

## Dependency Graph Summary

**Critical Path (P0 chain):**
1. 5-001 (enums) -> 5-002, 5-003, 5-004, 5-005 (components) -> 5-007 (coverage grid) -> 5-008 (system skeleton)
2. 5-008 -> 5-009 (IEnergyProvider) -> 5-036 (Epic 4 integration)
3. 5-008 -> 5-014 (coverage BFS) -> 5-017 (coverage queries) -> 5-018 (distribution)
4. 5-010, 5-011 -> 5-012 (pool calculation) -> 5-013 (state machine) -> 5-018, 5-019 (distribution/rationing)
5. 5-027 (conduit placement) -> 5-028, 5-029 (conduit mechanics)

**External Dependencies:**
- Epic 3: ITerrainQueryable (for nexus/conduit placement validation, terrain bonuses)
- Epic 4: BuildingConstructedEvent, BuildingDeconstructedEvent, IEnergyProvider stub (4-019)

**Downstream Dependents (future epics that will consume Epic 5):**
- Epic 6 (FluidSystem): May share pool model pattern, no direct dependency
- Epic 7 (TransportSystem): Rail terminals query IEnergyProvider.is_powered()
- Epic 9 (Services): Service structures query IEnergyProvider.is_powered()
- Epic 10 (ContaminationSystem): Queries IContaminationSource from dirty nexuses
- Epic 10 (PopulationSystem): Pool state affects habitability
- Epic 11 (EconomySystem): Nexus build/maintenance costs
- Epic 12 (UISystem): Energy overlay, pool status display

---

## Canon Update Flags (for Phase 6)

The following canon updates are flagged from the discussion document:

| File | Change | Rationale |
|------|--------|-----------|
| interfaces.yaml | Add IEnergyProvider interface (replacing Epic 4 stub definition) | Formal interface definition for energy queries |
| interfaces.yaml | Update IEnergyConsumer priority note to reflect 4 levels | Canon mentions 3 levels, we use 4 (added 'low' for habitation) |
| systems.yaml | Add EnergySystem ISimulatable priority (10) | Formal tick order documentation |
| terminology.yaml | Add energy nexus type terms: carbon_nexus, petrochemical_nexus, gaseous_nexus | Canonical names for dirty energy types |
| patterns.yaml | Add CoverageGrid to dense_grid_exception applies_to | Coverage grid follows same pattern as TerrainGrid |

---

**Document Status:** Ready for Canon Verification (Phase 5)

**Next Steps:**
1. Systems Architect reviews tickets against canon
2. Flag any canon conflicts as decisions
3. Apply canon updates via `/update-canon`
