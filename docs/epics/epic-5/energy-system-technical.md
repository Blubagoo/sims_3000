# EnergySystem Technical Documentation

**Ticket:** 5-044
**Status:** Complete
**Canon Version:** 0.13.0
**Related:** Epic 5 (Energy Infrastructure), tickets 5-001 through 5-043

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Components](#2-components)
3. [Tick Pipeline](#3-tick-pipeline)
4. [Coverage Algorithm](#4-coverage-algorithm)
5. [Rationing Algorithm](#5-rationing-algorithm)
6. [Interfaces](#6-interfaces)
7. [Integration Points](#7-integration-points)
8. [Configuration Reference](#8-configuration-reference)

---

## 1. Architecture Overview

The EnergySystem is the first major tension system in Sims 3000. It implements the canonical pool model where per-overseer energy pools are fed by energy nexuses, with coverage zones defined by energy conduits. Structures draw from the pool when they are within coverage and the pool has surplus.

EnergySystem implements `ISimulatable` at **priority 10**, running early in the tick order so that energy state is settled before ZoneSystem (30) and BuildingSystem (40) execute.

### System Diagram

```
+------------------------------------------------------------------+
|                        EnergySystem                               |
|  (ISimulatable, priority 10)                                      |
|                                                                   |
|  +-------------------+   +-------------------+   +--------------+ |
|  | Nexus Management  |   | Coverage Engine   |   | Pool Engine  | |
|  |                   |   |                   |   |              | |
|  | - Registration    |   | - CoverageGrid    |   | - Calculate  | |
|  | - Output calc     |   | - BFS flood-fill  |   | - State      | |
|  | - Aging           |   | - Dirty tracking  |   |   machine    | |
|  | - Contamination   |   | - Ownership       |   | - Distribute | |
|  +-------------------+   |   boundaries      |   | - Ration     | |
|                           +-------------------+   +--------------+ |
|                                                                   |
|  Interfaces provided:                                             |
|    IEnergyProvider  ->  BuildingSystem (Epic 4)                   |
|    IContaminationSource  ->  ContaminationSystem (Epic 10)        |
+------------------------------------------------------------------+
         |                    |                    |
         v                    v                    v
   EnergyProducer       EnergyConduit         EnergyComponent
   Component             Component             (on consumers)
   (on nexuses)          (on conduits)
```

### Ownership

EnergySystem owns:
- Energy nexus simulation (output, efficiency, aging)
- Energy pool calculation per overseer (generation, consumption, surplus)
- Coverage zone calculation (BFS through conduit network)
- Energy distribution to structures (powered/unpowered marking)
- Energy deficit and grid collapse detection
- Priority-based rationing during deficit

EnergySystem does NOT own:
- Structure energy consumption amounts (defined by templates via BuildingSystem)
- Energy UI overlays (Epic 12)
- Energy costs (EconomySystem, Epic 11)
- Contamination simulation (ContaminationSystem, Epic 10; EnergySystem only provides source data)

---

## 2. Components

### EnergyComponent (12 bytes)

Attached to all structures that consume energy: zone structures, service structures, fluid extractors, rail terminals.

```cpp
struct EnergyComponent {
    uint32_t energy_required;   // Energy units needed per tick (from template)
    uint32_t energy_received;   // Energy units actually received this tick
    bool     is_powered;        // true if energy_received >= energy_required
    uint8_t  priority;          // Rationing priority 1-4 (1 = highest)
    uint8_t  grid_id;           // Reserved for future isolated grid support
    uint8_t  padding;           // Alignment padding
};
static_assert(sizeof(EnergyComponent) == 12);
```

**Priority Levels (CCR-003):**

| Priority | Value | Structures |
|----------|-------|------------|
| Critical | 1 | Medical nexus, command nexus |
| Important | 2 | Enforcer post, hazard response post |
| Normal | 3 | Exchange structures, fabrication structures, education, recreation |
| Low | 4 | Habitation structures |

The binary power model (CCR-002) means structures are either fully powered or completely unpowered. There is no partial power state.

### EnergyProducerComponent (24 bytes)

Attached to energy nexuses that generate energy.

```cpp
struct EnergyProducerComponent {
    uint32_t base_output;            // Maximum output at 100% efficiency
    uint32_t current_output;         // Actual output this tick
    float    efficiency;             // Current efficiency multiplier [0.0, 1.0]
    float    age_factor;             // Aging degradation, starts at 1.0
    uint16_t ticks_since_built;      // Age in ticks, capped at 65535
    uint8_t  nexus_type;             // NexusType enum value
    bool     is_online;              // true if operational
    uint32_t contamination_output;   // Contamination units per tick (0 for clean)
};
static_assert(sizeof(EnergyProducerComponent) == 24);
```

**Output Calculation:**
```
current_output = base_output * efficiency * age_factor
```
If `is_online == false`, then `current_output = 0`.

### EnergyConduitComponent (4 bytes)

Attached to energy conduit entities that extend coverage zones. Conduits do NOT transport energy; they define the coverage area.

```cpp
struct EnergyConduitComponent {
    uint8_t coverage_radius;   // Tiles of coverage this conduit adds (default 2-3)
    bool    is_connected;      // true if connected to energy matrix via BFS
    bool    is_active;         // true if carrying energy (for rendering glow)
    uint8_t conduit_level;     // 1=basic, 2=upgraded (future expansion)
};
static_assert(sizeof(EnergyConduitComponent) == 4);
```

**Connection vs Active:**
- `is_connected`: Set during coverage BFS. True if this conduit is reachable from a nexus through the conduit network.
- `is_active`: True if `is_connected AND owner's pool has generation > 0`. Drives visual glow rendering.

### PerPlayerEnergyPool (24 bytes)

Aggregated energy pool data per overseer. One pool exists per player.

```cpp
struct PerPlayerEnergyPool {
    PlayerID         owner;           // Overseer who owns this pool
    uint32_t         total_generated; // Sum of all nexus current_output
    uint32_t         total_consumed;  // Sum of all consumer energy_required in coverage
    int32_t          surplus;         // generated - consumed (can be negative)
    uint32_t         nexus_count;     // Number of active nexuses
    uint32_t         consumer_count;  // Number of consumers in coverage
    EnergyPoolState  state;           // Healthy / Marginal / Deficit / Collapse
    EnergyPoolState  previous_state;  // For transition detection
};
```

**Pool States:**

| State | Condition | Meaning |
|-------|-----------|---------|
| Healthy | `surplus >= buffer_threshold` | Excess energy available |
| Marginal | `0 <= surplus < buffer_threshold` | Running near capacity |
| Deficit | `-collapse_threshold < surplus < 0` | Consumption exceeds generation; rationing active |
| Collapse | `surplus <= -collapse_threshold` | Severe deficit; mass outages |

Where:
- `buffer_threshold = total_generated * 0.10` (10% of generation)
- `collapse_threshold = total_consumed * 0.50` (50% of consumption)

### CoverageGrid

Dense 2D array for O(1) coverage queries. Follows the `dense_grid_exception` pattern (same as TerrainGrid from Epic 3).

```cpp
class CoverageGrid {
    std::vector<uint8_t> data_;   // Row-major: index = y * width + x
    uint32_t width_;
    uint32_t height_;

public:
    bool is_in_coverage(int32_t x, int32_t y, PlayerID owner) const;  // O(1)
    PlayerID get_coverage_owner(int32_t x, int32_t y) const;           // O(1)
    void set(int32_t x, int32_t y, PlayerID owner);
    void clear(int32_t x, int32_t y);
    void clear_all_for_owner(PlayerID owner);
};
```

**Storage:** 1 byte per cell. Value 0 = uncovered, 1-4 = overseer ID who has coverage.

**Memory Usage:**
| Map Size | Memory |
|----------|--------|
| 128x128 | 16 KB |
| 256x256 | 64 KB |
| 512x512 | 256 KB |

---

## 3. Tick Pipeline

The EnergySystem `tick()` method executes a multi-phase pipeline each simulation tick. The phases are ordered to ensure data dependencies are resolved before consumption.

```
Phase 0: Clear transition events
Phase 1: Age nexuses (asymptotic decay)
Phase 2: Update nexus outputs
Phase 3: Coverage BFS (if dirty)
Phase 4: Pool calculation (generation - consumption = surplus)
Phase 5: Pool state machine + transition detection
Phase 6: Snapshot power states + distribute energy (or ration)
Phase 7: Update conduit active states
Phase 8: Emit state change events
```

### Phase 0: Clear Transition Events

Reset per-tick event queues. Ensures stale events from the previous tick are not re-emitted.

### Phase 1: Age Nexuses

For each nexus entity:
- Increment `ticks_since_built` (cap at 65535)
- Recalculate `age_factor` using the asymptotic decay curve:

```
age_factor = floor + (1.0 - floor) * exp(-decay_rate * ticks_since_built)
```

The floor is type-specific per CCR-006, ensuring nexuses never degrade to zero output:

| Nexus Type | Aging Floor |
|------------|-------------|
| Carbon | 60% |
| Petrochemical | 65% |
| Gaseous | 70% |
| Nuclear | 75% |
| Wind | 80% |
| Solar | 85% |

Emit `NexusAgedEvent` when efficiency crosses a 10% threshold boundary.

### Phase 2: Update Nexus Outputs

For each nexus entity:
```
if (!is_online)
    current_output = 0
else
    current_output = base_output * efficiency * age_factor
```

Wind and Solar nexuses apply a weather/day-night factor (stub returns 0.75 average for MVP).

### Phase 3: Coverage BFS (If Dirty)

For each overseer, if `coverage_dirty_[owner]` is true:
1. Clear all coverage for this owner
2. Run BFS flood-fill from all nexuses owned by this player
3. Clear the dirty flag

Coverage recalculation is only triggered when the conduit/nexus network changes (placement or removal). See [Coverage Algorithm](#4-coverage-algorithm) for details.

### Phase 4: Pool Calculation

For each overseer:
```
pool.total_generated = SUM(nexus.current_output for all online nexuses)
pool.total_consumed  = SUM(consumer.energy_required for consumers in coverage)
pool.surplus         = pool.total_generated - pool.total_consumed
pool.nexus_count     = count of active nexuses
pool.consumer_count  = count of consumers in coverage
```

### Phase 5: Pool State Machine + Transition Detection

For each overseer:
1. Save `previous_state = pool.state`
2. Calculate new state based on surplus and thresholds
3. If state changed, queue appropriate transition events

### Phase 6: Snapshot Power States + Distribute Energy

1. Snapshot the current `is_powered` state for all consumers (for change detection in Phase 8)
2. If `pool.surplus >= 0`: Power all consumers in coverage
3. If `pool.surplus < 0`: Apply priority-based rationing (see [Rationing Algorithm](#5-rationing-algorithm))

### Phase 7: Update Conduit Active States

For each conduit owned by each overseer:
```
is_active = is_connected AND pool.total_generated > 0
```

This drives the visual glow rendering of conduits.

### Phase 8: Emit State Change Events

Compare snapshotted power states (from Phase 6 step 1) with current power states. For each consumer whose `is_powered` changed, emit `EnergyStateChangedEvent`.

Also emit pool state transition events queued in Phase 5:
- `EnergyDeficitBeganEvent`
- `EnergyDeficitEndedEvent`
- `GridCollapseBeganEvent`
- `GridCollapseEndedEvent`

---

## 4. Coverage Algorithm

Coverage uses a BFS (Breadth-First Search) flood-fill from nexuses through connected conduits. The algorithm runs only when the conduit/nexus network for a given overseer changes (dirty flag optimization).

### Pseudocode

```
function recalculate_coverage(owner: PlayerID):
    coverage_grid.clear_all_for_owner(owner)

    visited = empty set of (x, y)
    frontier = empty queue of (x, y)

    // Step 1: Seed from all nexuses owned by this player
    for each nexus owned by owner:
        pos = nexus.position
        mark_coverage_radius(pos.x, pos.y, nexus_config.coverage_radius, owner)
        visited.add(pos)
        frontier.enqueue(pos)

    // Step 2: BFS through conduit network
    while frontier is not empty:
        current = frontier.dequeue()

        for each neighbor in 4-directional neighbors of current:
            if neighbor not in visited:
                if can_extend_coverage_to(neighbor.x, neighbor.y, owner):
                    conduit = get_conduit_at(neighbor.x, neighbor.y, owner)
                    if conduit exists:
                        conduit.is_connected = true
                        mark_coverage_radius(neighbor.x, neighbor.y,
                                             conduit.coverage_radius, owner)
                        visited.add(neighbor)
                        frontier.enqueue(neighbor)

function mark_coverage_radius(cx, cy, radius, owner):
    for y in (cy - radius) to (cy + radius):
        for x in (cx - radius) to (cx + radius):
            if in_bounds(x, y):
                coverage_grid.set(x, y, owner)

function can_extend_coverage_to(x, y, owner):
    tile_owner = get_tile_owner(x, y)
    return tile_owner == owner OR tile_owner == GAME_MASTER
```

### Complexity

- **Time:** O(conduits) for the BFS traversal. The `mark_coverage_radius` step is O(radius^2) per conduit/nexus, but radius values are small (4-10).
- **Space:** O(conduits) for the visited set and frontier queue, plus the CoverageGrid itself.

### Dirty Flag Optimization

Coverage recalculates only when the network changes:
- `ConduitPlacedEvent` sets `coverage_dirty_[owner] = true`
- `ConduitRemovedEvent` sets `coverage_dirty_[owner] = true`
- `NexusPlacedEvent` sets `coverage_dirty_[owner] = true`
- `NexusRemovedEvent` sets `coverage_dirty_[owner] = true`

Per-overseer dirty flags ensure that one player's network changes do not trigger recalculation for other players.

### Ownership Boundaries

Coverage does NOT cross overseer boundaries. The `can_extend_coverage_to` check ensures that BFS only extends to tiles owned by the same player or unclaimed (GAME_MASTER) tiles.

### Performance Target

Coverage recalculation must complete in **< 10ms** for a 512x512 map with 5,000 conduits.

---

## 5. Rationing Algorithm

When the pool is in deficit (`surplus < 0`), priority-based rationing determines which consumers receive energy and which are cut off.

### Algorithm

```
function apply_rationing(owner: PlayerID):
    available_energy = pool.total_generated

    // Collect all consumers in coverage for this overseer
    consumers = get_consumers_in_coverage(owner)

    // Sort by priority (ascending = higher priority first),
    // then by EntityID for deterministic tie-breaking
    sort consumers by (priority ASC, entity_id ASC)

    // Distribute available energy
    for each consumer in sorted order:
        if available_energy >= consumer.energy_required:
            consumer.energy_received = consumer.energy_required
            consumer.is_powered = true
            available_energy -= consumer.energy_required
        else:
            consumer.energy_received = 0
            consumer.is_powered = false
```

### Priority Order

1. **Priority 1 (Critical):** Medical nexus, command nexus -- powered first
2. **Priority 2 (Important):** Enforcer post, hazard response post
3. **Priority 3 (Normal):** Exchange, fabrication, education, recreation structures
4. **Priority 4 (Low):** Habitation structures -- cut first

### Determinism

The EntityID tie-breaker ensures that rationing produces identical results on the server and all clients in multiplayer. This is critical for simulation consistency.

### Binary Power Model

Per CCR-002, there is no partial power. A consumer either receives its full `energy_required` or receives 0. This simplifies the algorithm and avoids fractional distribution complexity.

---

## 6. Interfaces

### IEnergyProvider

Provides energy state queries for other systems. Replaces the Epic 4 stub (ticket 4-019).

```cpp
class IEnergyProvider {
public:
    virtual ~IEnergyProvider() = default;

    // Check if a specific entity is currently powered
    virtual bool is_powered(EntityID entity) const = 0;

    // Check if a position has energy coverage and pool surplus
    virtual bool is_powered_at(int32_t x, int32_t y, PlayerID owner) const = 0;

    // Get energy requirement for an entity
    virtual uint32_t get_energy_required(EntityID entity) const = 0;

    // Get energy actually received by an entity
    virtual uint32_t get_energy_received(EntityID entity) const = 0;

    // Get the pool state for an overseer
    virtual EnergyPoolState get_pool_state(PlayerID owner) const = 0;

    // Get the full pool data for an overseer
    virtual const PerPlayerEnergyPool& get_pool(PlayerID owner) const = 0;
};
```

**Consumers:**
- BuildingSystem (Epic 4): Queries `is_powered()` for building state decisions
- TransportSystem (Epic 7): Rail terminals query `is_powered()`
- ServicesSystem (Epic 9): Service structures query `is_powered()`
- UISystem (Epic 12): Pool status display

### IContaminationSource

Provides contamination source data for polluting nexuses. Consumed by ContaminationSystem (Epic 10).

```cpp
class IContaminationSource {
public:
    virtual ~IContaminationSource() = default;

    // Get all active contamination sources
    virtual std::vector<ContaminationSourceData>
        get_contamination_sources() const = 0;
};

struct ContaminationSourceData {
    EntityID entity;
    int32_t  x;
    int32_t  y;
    uint32_t output;           // Contamination units per tick
    uint32_t radius;           // Contamination spread radius
    ContaminationType type;    // ContaminationType::Energy
};
```

Per CCR-007, contamination is only emitted when a nexus is online (`is_online == true` AND `current_output > 0`). An offline or zero-output nexus produces no contamination.

**Polluting Nexus Types:**
- Carbon: 200 contamination units
- Petrochemical: 120 contamination units
- Gaseous: 40 contamination units
- Nuclear, Wind, Solar: 0 contamination units

---

## 7. Integration Points

### Epic 3: Terrain System

**Interface:** `ITerrainQueryable`

- `is_buildable(x, y)`: Used for nexus and conduit placement validation
- Terrain elevation queries: Used for Wind nexus terrain bonus (+20% efficiency on ridges)
- Terrain type queries: Future use for Hydro (near water) and Geothermal (ember crust) nexus types

### Epic 4: Building System

**Inbound (EnergySystem listens):**
- `BuildingConstructedEvent`: Registers new consumers (entities with EnergyComponent) and producers (entities with EnergyProducerComponent). Nexus registration sets coverage dirty flag.
- `BuildingDeconstructedEvent`: Unregisters consumers and producers. Nexus unregistration sets coverage dirty flag.

**Outbound (EnergySystem provides):**
- `IEnergyProvider` interface: BuildingSystem queries `is_powered()` to determine building state.
- Grace period: 100 ticks (5 seconds at 20 ticks/sec) before an unpowered structure transitions to degraded state (CCR-009).

### Epic 10: Contamination System

**Outbound:**
- `IContaminationSource` interface: ContaminationSystem queries `get_contamination_sources()` each tick to calculate contamination spread from dirty nexuses.

### Epic 10: Population System

**Outbound:**
- Pool state affects habitability calculations. Population system reads pool state via `IEnergyProvider.get_pool_state()`.

### Epic 11: Economy System

**Inbound:**
- Nexus build costs deducted at placement time
- Nexus maintenance costs charged per tick by EconomySystem

### Epic 12: UI System

**Outbound:**
- Coverage queries: `get_coverage_at(x, y)` for overlay rendering
- Pool state: `get_pool(owner)` for HUD display
- Conduit preview: `preview_conduit_coverage(x, y, owner)` for placement preview delta

### Network Sync (CCR-008)

- `is_powered` state: Synced every tick via standard component sync (1 bit per entity)
- `energy_received`: Optionally synced for UI display
- Pool state: Synced on change (16 bytes per player per change)
- All players see all energy states (rivals visible for "schadenfreude" effect)

---

## 8. Configuration Reference

### NexusTypeConfig

Configuration data for the 6 MVP nexus types. Values are intended for balancing and should be loaded from configuration files.

```cpp
struct NexusTypeConfig {
    NexusType   type;
    uint32_t    base_output;          // Energy units per tick at 100%
    uint32_t    build_cost;           // Credits to construct
    uint32_t    maintenance_cost;     // Credits per tick
    uint32_t    contamination_output; // Contamination units per tick
    uint8_t     coverage_radius;      // Tiles of coverage around nexus
    float       aging_floor;          // Minimum efficiency from aging
    TerrainRequirement terrain_req;   // Terrain placement requirement
    bool        is_variable_output;   // true for Wind/Solar
};
```

| Type | Output | Build Cost | Maintenance | Contamination | Coverage Radius | Aging Floor | Variable |
|------|--------|-----------|-------------|---------------|----------------|-------------|----------|
| Carbon | 100 | 5,000 | 50 | 200 | 8 | 60% | No |
| Petrochemical | 150 | 8,000 | 75 | 120 | 8 | 65% | No |
| Gaseous | 120 | 10,000 | 60 | 40 | 8 | 70% | No |
| Nuclear | 400 | 50,000 | 200 | 0 | 10 | 75% | No |
| Wind | 30 | 3,000 | 20 | 0 | 4 | 80% | Yes |
| Solar | 50 | 6,000 | 30 | 0 | 6 | 85% | Yes |

### Pool State Thresholds

| Threshold | Value | Derivation |
|-----------|-------|------------|
| buffer_threshold | 10% | `total_generated * 0.10` |
| collapse_threshold | 50% | `total_consumed * 0.50` |

### System Constants

| Constant | Value | Description |
|----------|-------|-------------|
| Tick priority | 10 | ISimulatable execution order |
| Tick rate | 20 ticks/sec | 50ms fixed timestep |
| Grace period | 100 ticks | 5 seconds before unpowered degradation |
| Max players | 4 | Per-overseer pool/coverage array size |
| Conduit default cost | 10 credits | Build cost for basic conduit |
| Weather factor (MVP) | 0.75 | Average factor for Wind/Solar stubs |
| NexusAgedEvent threshold | 10% | Emit event every 10% efficiency drop |

### Enumerations

```cpp
enum class NexusType : uint8_t {
    Carbon = 0,
    Petrochemical = 1,
    Gaseous = 2,
    Nuclear = 3,
    Wind = 4,
    Solar = 5,
    // Deferred types (forward compatibility)
    Hydro = 6,
    Geothermal = 7,
    Fusion = 8,
    MicrowaveReceiver = 9
};

enum class EnergyPoolState : uint8_t {
    Healthy = 0,
    Marginal = 1,
    Deficit = 2,
    Collapse = 3
};

enum class TerrainRequirement : uint8_t {
    None = 0,
    Water = 1,
    EmberCrust = 2,
    Ridges = 3
};
```

### Energy Events

| Event | Fields | Trigger |
|-------|--------|---------|
| EnergyStateChangedEvent | entity_id, owner_id, was_powered, is_powered | Consumer power state change |
| EnergyDeficitBeganEvent | owner_id, deficit_amount, affected_consumers | Pool enters Deficit from Healthy/Marginal |
| EnergyDeficitEndedEvent | owner_id, surplus_amount | Pool exits Deficit to Healthy/Marginal |
| GridCollapseBeganEvent | owner_id, deficit_amount | Pool enters Collapse |
| GridCollapseEndedEvent | owner_id | Pool exits Collapse |
| ConduitPlacedEvent | entity_id, owner_id, grid_position | Conduit placed |
| ConduitRemovedEvent | entity_id, owner_id, grid_position | Conduit removed |
| NexusPlacedEvent | entity_id, owner_id, nexus_type, grid_position | Nexus placed |
| NexusRemovedEvent | entity_id, owner_id, grid_position | Nexus removed |
| NexusAgedEvent | entity_id, owner_id, new_efficiency | Nexus crosses 10% aging threshold |

### Performance Targets

| Metric | Target | Map Size |
|--------|--------|----------|
| Coverage recalculation | < 10ms | 512x512 with 5,000 conduits |
| Pool calculation | < 1ms | 10,000 consumers |
| Full tick | < 2ms | 256x256 map |
| EnergyComponent memory | 12 bytes | Per consumer entity |
| EnergyProducerComponent memory | 24 bytes | Per nexus entity |
| EnergyConduitComponent memory | 4 bytes | Per conduit entity |
| CoverageGrid memory | 64 KB | 256x256 map |
| CoverageGrid memory | 256 KB | 512x512 map |
