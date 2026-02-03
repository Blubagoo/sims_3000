# Epic 5: Energy Infrastructure - Systems Architect Analysis

**Epic Goal:** Implement the energy matrix (power grid) using the canonical pool model: per-overseer energy pools fed by energy nexuses, with coverage zones defined by energy conduits enabling structures to draw from the pool.

**Author:** Systems Architect Agent
**Created:** 2026-01-28
**Canon Version:** 0.6.0
**Status:** Planning Phase

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Data Flow](#3-data-flow)
4. [Pool Model Details](#4-pool-model-details)
5. [Coverage Zone Algorithm](#5-coverage-zone-algorithm)
6. [Interface Implementation](#6-interface-implementation)
7. [Multiplayer Implications](#7-multiplayer-implications)
8. [Cross-Epic Dependencies](#8-cross-epic-dependencies)
9. [Key Work Items](#9-key-work-items)
10. [Questions for Other Agents](#10-questions-for-other-agents)
11. [Risks and Concerns](#11-risks-and-concerns)

---

## 1. System Boundaries

### What EnergySystem Owns

Per `systems.yaml`, EnergySystem (priority 10) owns:

| Responsibility | Description |
|----------------|-------------|
| Energy nexus simulation | Managing energy nexus (power plant) entities, their output, efficiency, and aging |
| Energy pool calculation | Computing per-overseer generation, consumption, and surplus each tick |
| Coverage zone calculation | Determining which sectors are within the energy matrix (connected via conduits) |
| Energy distribution | Marking structures as powered/unpowered based on pool surplus and coverage |
| Energy deficit/grid collapse detection | Detecting when consumption exceeds generation and applying priority-based rationing |
| Nexus efficiency and aging | Reducing output over time, handling maintenance effects |

### What EnergySystem Does NOT Own

| Responsibility | Owner | Notes |
|----------------|-------|-------|
| Structure energy consumption amounts | BuildingSystem / IEnergyConsumer | Structures define their own requirements |
| Energy UI | UISystem (Epic 12) | Overlays, energy deficit warnings, nexus info panels |
| Energy costs | EconomySystem (Epic 11) | Construction cost, maintenance cost, operational cost |
| Contamination from nexuses | ContaminationSystem (Epic 10) | EnergySystem implements IContaminationSource |
| Structure placement/destruction | BuildingSystem (Epic 4) | EnergySystem responds to BuildingConstructedEvent |
| Terrain validation for nexus placement | TerrainSystem (Epic 3) | ITerrainQueryable.is_buildable() |

### Relationship to Other Systems

```
                    +-----------------+
                    | BuildingSystem  |
                    |   (Epic 4)      |
                    +-------+---------+
                            |
        IEnergyConsumer     |     IEnergyProducer
        IEnergyProvider     |
                            v
                    +-----------------+
                    |  EnergySystem   |
                    |   (Epic 5)      |
                    +-------+---------+
                            |
        IContaminationSource|     ISimulatable (priority 10)
                            |
            +---------------+---------------+
            |               |               |
            v               v               v
    +---------------+ +-----------------+ +-----------------+
    |Contamination  | |   FluidSystem   | | TransportSystem |
    |System (E10)   | |    (Epic 6)     | |    (Epic 7)     |
    +---------------+ +-----------------+ +-----------------+
```

**Key System Interactions:**

1. **BuildingSystem (Epic 4):**
   - EnergySystem implements IEnergyProvider (replacing Epic 4's stub)
   - EnergySystem queries IEnergyConsumer and IEnergyProducer from structures
   - EnergySystem listens for BuildingConstructedEvent to register new consumers/producers

2. **ContaminationSystem (Epic 10):**
   - Certain energy nexuses (carbon nexus, petrochemical nexus) implement IContaminationSource
   - ContaminationSystem queries EnergySystem's nexuses for contamination output

3. **ZoneSystem (Epic 4):**
   - No direct interface; ZoneSystem uses EnergySystem indirectly via BuildingSystem's structure state

4. **FluidSystem (Epic 6):**
   - No direct dependency; both are parallel utility systems
   - Similar pool model pattern (potential shared abstraction)

---

## 2. Data Model

### 2.1 EnergyComponent

Attached to entities that consume energy (structures in zones, service structures, infrastructure that needs energy).

```cpp
struct EnergyComponent {
    uint32_t energy_required;     // Energy units needed per tick (0 = no requirement)
    uint32_t energy_received;     // Energy units actually received this tick
    bool     is_powered;          // True if structure has sufficient energy
    uint8_t  priority;            // Rationing priority (1=critical, 2=important, 3=normal)
    uint8_t  padding[2];          // Alignment padding
};
// Size: 12 bytes (verified with static_assert)
```

**Design Rationale:**
- `energy_required`: Defined by structure type/template, not EnergySystem
- `energy_received`: Set by EnergySystem each tick (may be < required during energy deficit)
- `is_powered`: True if `energy_received >= energy_required`
- `priority`: Used for rationing during energy deficit (lower value = higher priority)

**Priority Levels (per interfaces.yaml):**
- 1 = Critical (medical nexus, command nexus)
- 2 = Important (enforcer post, hazard response)
- 3 = Normal (all zone structures)

### 2.2 EnergyProducerComponent

Attached to energy nexuses (power plants) that generate energy.

```cpp
struct EnergyProducerComponent {
    uint32_t max_output;          // Maximum energy output per tick
    uint32_t current_output;      // Actual output this tick (after efficiency)
    float    efficiency;          // Current efficiency (0.0 - 1.0)
    float    age_factor;          // Degradation from aging (1.0 = new, decays over time)
    uint16_t ticks_since_built;   // Age in simulation ticks (capped at 65535)
    uint8_t  nexus_type;          // NexusType enum (for contamination, terrain bonuses)
    bool     is_online;           // True if operational (has fuel, workers, etc.)
};
// Size: 20 bytes
```

**Design Rationale:**
- `current_output = max_output * efficiency * age_factor`
- `efficiency`: Affected by worker availability, maintenance level, terrain bonuses
- `age_factor`: Starts at 1.0, decays over time (configurable curve)
- `nexus_type`: Enum for different nexus types (affects contamination, efficiency characteristics)

**NexusType Enum:**
```cpp
enum class NexusType : uint8_t {
    Carbon = 0,        // High output, high contamination
    Petrochemical = 1, // Medium output, medium contamination
    Gaseous = 2,       // Lower output, lower contamination
    Nuclear = 3,       // Very high output, risk factor, low contamination
    Wind = 4,          // Low output, no contamination, variable efficiency
    Hydro = 5,         // Medium output, no contamination, requires water proximity
    Solar = 6,         // Low-medium output, no contamination, variable efficiency
    Fusion = 7,        // Future tech: massive output, no contamination
    MicrowaveReceiver = 8  // Future tech: massive output, no contamination
};
```

### 2.3 EnergyConduitComponent

Attached to energy conduit (power line) entities that extend coverage zones.

```cpp
struct EnergyConduitComponent {
    uint8_t conduit_level;        // 1 = basic, 2 = upgraded (future)
    bool    is_connected;         // True if connected to a nexus or other conduit
    uint8_t coverage_radius;      // Tiles of coverage this conduit adds
    uint8_t padding;              // Alignment
};
// Size: 4 bytes
```

**Design Rationale:**
- `is_connected`: Set by coverage zone algorithm; false = conduit is isolated
- `coverage_radius`: Default 2-3 tiles; conduit extends coverage around itself
- `conduit_level`: Future expansion for upgraded conduits with larger radius

### 2.4 Supporting Data Structures

**PerPlayerEnergyPool:**
```cpp
struct PerPlayerEnergyPool {
    PlayerID owner;
    uint32_t total_generated;     // Sum of all nexus current_output
    uint32_t total_consumed;      // Sum of all consumer energy_required
    int32_t  surplus;             // generated - consumed (can be negative)
    uint32_t nexus_count;         // Number of active nexuses
    uint32_t consumer_count;      // Number of energy consumers
    bool     is_in_deficit;       // True if surplus < 0
    bool     is_in_collapse;      // True if surplus < -collapse_threshold
};
```

**EnergyPoolState Enum:**
```cpp
enum class EnergyPoolState : uint8_t {
    Healthy = 0,      // Surplus > threshold (e.g., > 10% buffer)
    Marginal = 1,     // 0 < surplus < threshold (warning zone)
    Deficit = 2,      // surplus <= 0 but > -collapse_threshold
    Collapse = 3      // surplus <= -collapse_threshold (grid collapse)
};
```

### 2.5 Size Optimization Summary

| Component | Size | Notes |
|-----------|------|-------|
| EnergyComponent | 12 bytes | Attached to all consumers |
| EnergyProducerComponent | 20 bytes | Attached to nexuses only |
| EnergyConduitComponent | 4 bytes | Attached to conduits only |

**Memory Budget Estimate (256x256 map, 30% developed):**
- ~20,000 structures with EnergyComponent: 240 KB
- ~50 energy nexuses: 1 KB
- ~2,000 conduit segments: 8 KB
- Coverage grid (256x256): 64 KB (1 byte per cell)
- Total: ~313 KB dedicated to energy system

---

## 3. Data Flow

### 3.1 Pool Model Data Flow

```
+---------------------+    +---------------------+    +---------------------+
|   Energy Nexuses    |    |   Per-Player Pool   |    |  Energy Consumers   |
|  (IEnergyProducer)  |--->|   (Calculation)     |--->|  (IEnergyConsumer)  |
+---------------------+    +---------------------+    +---------------------+
         ^                          |                          |
         |                          v                          v
    +----+----+              +------+------+           +-------+-------+
    | Aging,  |              | Surplus or  |           | is_powered    |
    | Mainten.|              | Deficit?    |           | set per tick  |
    +---------+              +------+------+           +---------------+
                                    |
                    +---------------+---------------+
                    |                               |
              Surplus >= 0                    Deficit < 0
                    |                               |
                    v                               v
         +------------------+            +---------------------+
         | All consumers in |            | Priority-based      |
         | coverage powered |            | rationing applied   |
         +------------------+            +---------------------+
```

### 3.2 Tick Processing Order

EnergySystem runs at priority 10 (early in tick), before most other systems.

```
tick(delta_time):
    1. Update all nexus outputs (efficiency * age_factor * max_output)
    2. Calculate per-overseer energy pools (sum generation, sum consumption)
    3. Determine pool state (healthy, marginal, deficit, collapse)
    4. If surplus >= 0:
         Mark all consumers in coverage as powered
    5. If deficit:
         Apply priority-based rationing
         Mark some consumers as unpowered
    6. Update is_powered flag on all EnergyComponents
    7. Emit EnergyStateChangedEvent for consumers that changed state
    8. Update aggregate stats for UI/queries
```

### 3.3 Event Flow

**Inbound Events (EnergySystem listens):**
- `BuildingConstructedEvent`: Register new consumer/producer
- `BuildingDeconstructedEvent`: Unregister consumer/producer
- `ConduitPlacedEvent`: Trigger coverage zone recalculation
- `ConduitRemovedEvent`: Trigger coverage zone recalculation

**Outbound Events (EnergySystem emits):**
- `EnergyStateChangedEvent`: Consumer's `is_powered` changed
- `EnergyDeficitBeganEvent`: Per-overseer, pool entered deficit
- `EnergyDeficitEndedEvent`: Per-overseer, pool exited deficit
- `GridCollapseBeganEvent`: Per-overseer, severe deficit threshold crossed
- `GridCollapseEndedEvent`: Per-overseer, recovered from collapse
- `NexusAgedEvent`: Nexus efficiency degraded past a threshold

---

## 4. Pool Model Details

### 4.1 Pool Calculation

Per `patterns.yaml`, the pool model is NOT a physical flow simulation. Energy doesn't "route" through conduits. Instead:

```
For each overseer:
    total_generated = SUM(nexus.current_output for nexus in overseer's nexuses)
    total_consumed = SUM(consumer.energy_required for consumer in overseer's covered consumers)
    surplus = total_generated - total_consumed
```

**Key Insight:** A structure is powered if BOTH conditions are true:
1. The structure is within the overseer's coverage zone
2. The overseer's pool has sufficient surplus (or structure is high-priority during rationing)

### 4.2 Pool States and Thresholds

| State | Condition | Effect |
|-------|-----------|--------|
| Healthy | surplus >= buffer_threshold | All covered consumers powered |
| Marginal | 0 <= surplus < buffer_threshold | All covered consumers powered, warning to overseer |
| Deficit (energy deficit) | -collapse_threshold < surplus < 0 | Priority-based rationing, some consumers lose energy |
| Collapse (grid collapse) | surplus <= -collapse_threshold | Severe rationing, possible total shutdown |

**Configurable Thresholds:**
- `buffer_threshold`: 10% of total_generated (default)
- `collapse_threshold`: 50% of total_consumed (default)

### 4.3 Priority-Based Rationing

When pool is in deficit, EnergySystem must decide which consumers to cut:

```
rationing_algorithm():
    available = total_generated

    // Sort consumers by priority (ascending = higher priority first)
    sorted_consumers = sort(consumers_in_coverage, by: priority)

    for consumer in sorted_consumers:
        if available >= consumer.energy_required:
            consumer.energy_received = consumer.energy_required
            consumer.is_powered = true
            available -= consumer.energy_required
        else:
            consumer.energy_received = 0
            consumer.is_powered = false

    // After rationing, emit events for state changes
```

**Rationing Priority Order:**
1. Priority 1 (Critical): Medical nexus, command nexus - powered first
2. Priority 2 (Important): Enforcer post, hazard response - powered second
3. Priority 3 (Normal): All zone structures - powered last

**Tie-breaking within same priority:** Entity ID (deterministic order for multiplayer)

### 4.4 Energy Deficit vs Grid Collapse

| Condition | Visual | Gameplay Effect |
|-----------|--------|----------------|
| Energy Deficit | Flickering lights, reduced glow | Some structures lose energy, reduced efficiency |
| Grid Collapse | Dark structures, emergency lighting | Most structures lose energy, critical services only |

**@game-designer:** What should the player experience be during energy deficit vs grid collapse? Should structures still function at reduced capacity, or go completely dark?

---

## 5. Coverage Zone Algorithm

### 5.1 Coverage Zone Concept

Energy conduits do NOT transport energy; they define the coverage zone. A structure can draw from the pool only if it's within the coverage zone.

**Coverage Zone Mechanics:**
1. Each energy nexus has a base coverage radius (e.g., 8 sectors)
2. Energy conduits extend the coverage zone by their own radius (e.g., 2-3 sectors)
3. Coverage propagates through connected conduits
4. Coverage does NOT cross overseer ownership boundaries

### 5.2 Algorithm: BFS/Flood-Fill

```
calculate_coverage_zone(overseer_id):
    coverage_grid = new Grid<bool>(map_width, map_height, false)
    queue = Queue<GridPosition>()

    // Step 1: Seed from nexuses
    for nexus in overseer's nexuses:
        pos = nexus.position
        mark_radius(coverage_grid, pos, nexus.coverage_radius, true)
        queue.enqueue(pos)

    // Step 2: Flood-fill through conduits
    while not queue.empty():
        current = queue.dequeue()
        for neighbor in get_neighbors(current):  // 4-directional or 8-directional
            if coverage_grid[neighbor] == true:
                continue
            if has_conduit_at(neighbor, overseer_id):
                conduit = get_conduit(neighbor)
                mark_radius(coverage_grid, neighbor, conduit.coverage_radius, true)
                queue.enqueue(neighbor)

    return coverage_grid
```

### 5.3 Coverage Grid vs Per-Entity Queries

**Option A: Dense Coverage Grid**
- Store a per-cell boolean/byte grid indicating coverage
- O(1) lookup: `is_in_coverage(x, y) = coverage_grid[y][x]`
- Memory: 1 byte per cell (64 KB for 256x256)
- Must recalculate on any conduit change

**Option B: Per-Entity Query**
- Calculate coverage on-demand for each entity
- O(N) where N = number of nexuses + conduits
- No dedicated memory
- No cache invalidation issues

**Recommendation:** Use dense coverage grid (Option A) with dirty tracking.

**Rationale:**
- Coverage queries happen every tick for every consumer
- At 10,000+ structures, per-entity queries become expensive
- Conduit changes are infrequent compared to per-tick queries

### 5.4 Dirty Tracking for Efficiency

```cpp
class EnergySystem {
private:
    Grid<uint8_t> coverage_grid_;          // 0 = not covered, overseer_id = covered by
    bool coverage_dirty_[MAX_PLAYERS];     // Per-overseer dirty flag

    void on_conduit_placed(ConduitPlacedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void on_conduit_removed(ConduitRemovedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void tick() {
        for (int i = 0; i < num_players; ++i) {
            if (coverage_dirty_[i]) {
                recalculate_coverage(i);
                coverage_dirty_[i] = false;
            }
        }
        // ... rest of tick
    }
};
```

### 5.5 Ownership Boundary Handling

Per `patterns.yaml`, energy conduits do NOT connect across ownership boundaries:

```
| Player A's tile | Player B's tile |
|    Conduit A    |    Conduit B    |

Conduit A only extends Player A's coverage zone.
Conduit B only extends Player B's coverage zone.
They are electrically isolated even if adjacent.
```

**Implementation:** Coverage grid stores `overseer_id`, not just boolean. When flooding, only continue to tiles owned by the same overseer.

---

## 6. Interface Implementation

### 6.1 ISimulatable (Epic 10 - SimulationCore)

```cpp
class EnergySystem : public ISimulatable {
public:
    void tick(float delta_time) override;
    int get_priority() const override { return 10; }  // Early priority
};
```

EnergySystem runs at priority 10, before:
- FluidSystem (priority 20)
- ZoneSystem (priority 30)
- BuildingSystem (priority 40)

**Rationale:** Energy state must be settled before other systems query `is_powered()`.

### 6.2 IEnergyProvider (Replaces Epic 4 Stub)

Epic 4 ticket 4-019 defines the IEnergyProvider stub:

```cpp
// Epic 4 stub definition:
class IEnergyProvider {
public:
    virtual ~IEnergyProvider() = default;
    virtual bool is_powered(EntityID entity) const = 0;
    virtual bool is_powered_at(int32_t x, int32_t y, PlayerID owner) const = 0;
};
```

**Epic 5 Real Implementation:**

```cpp
class EnergySystem : public ISimulatable, public IEnergyProvider {
public:
    // IEnergyProvider implementation
    bool is_powered(EntityID entity) const override {
        auto* energy = registry.get<EnergyComponent>(entity);
        return energy && energy->is_powered;
    }

    bool is_powered_at(int32_t x, int32_t y, PlayerID owner) const override {
        // Check if position is in coverage zone
        if (!is_in_coverage(x, y, owner)) {
            return false;
        }
        // Check if pool has surplus
        return get_pool(owner).surplus >= 0;
    }

    // Additional methods for energy-specific queries
    uint32_t get_energy_required(EntityID entity) const;
    uint32_t get_energy_received(EntityID entity) const;
    EnergyPoolState get_pool_state(PlayerID owner) const;
    const PerPlayerEnergyPool& get_pool(PlayerID owner) const;
};
```

### 6.3 IEnergyConsumer (Queried from Structures)

Per `interfaces.yaml`, IEnergyConsumer is implemented by structures:

```cpp
// Interface for energy consumers (implemented by building types)
class IEnergyConsumer {
public:
    virtual uint32_t get_energy_required() const = 0;
    virtual void on_energy_state_changed(bool has_power) = 0;
    virtual uint8_t get_energy_priority() const = 0;  // 1=critical, 2=important, 3=normal
};
```

**Note:** In pure ECS, this might be data-driven rather than virtual methods. The EnergyComponent stores the data, and BuildingSystem (or template system) populates it based on structure type.

### 6.4 IEnergyProducer (Implemented by Nexuses)

```cpp
// Interface for energy producers (implemented by nexus types)
class IEnergyProducer {
public:
    virtual uint32_t get_energy_output() const = 0;
    virtual uint32_t get_max_output() const = 0;
    virtual float get_efficiency() const = 0;
};
```

### 6.5 IContaminationSource (For Polluting Nexuses)

```cpp
// Implemented by carbon, petrochemical nexuses
class IContaminationSource {
public:
    virtual uint32_t get_contamination_output() const = 0;
    virtual ContaminationType get_contamination_type() const = 0;
};
```

**Nexuses implementing IContaminationSource:**
- Carbon nexus (coal): High contamination, type = Energy
- Petrochemical nexus (oil): Medium contamination, type = Energy
- Gaseous nexus (gas): Low contamination, type = Energy

**Clean nexuses (no IContaminationSource):**
- Wind, Solar, Hydro, Nuclear, Fusion, MicrowaveReceiver

---

## 7. Multiplayer Implications

### 7.1 Authority Model

| Aspect | Authority | Notes |
|--------|-----------|-------|
| Pool calculation | Server | Server computes all overseer pools |
| Coverage zone | Server | Server computes all coverage grids |
| is_powered state | Server | Server determines which structures are powered |
| Rationing order | Server | Deterministic by entity ID for consistency |

**Client receives:**
- `is_powered` state per structure (via component sync)
- Pool state per overseer (summary stats for UI)
- Coverage zone (optional: for UI overlay)

### 7.2 Per-Overseer Pools

Each overseer has an independent energy pool:

```
Overseer A: 500 generated, 300 consumed = 200 surplus (healthy)
Overseer B: 200 generated, 350 consumed = -150 deficit (energy deficit)
```

**Energy does NOT flow between overseers.** Each overseer must build sufficient nexuses for their own structures.

### 7.3 Network Sync Requirements

**Synced every tick:**
- `EnergyComponent.is_powered` (boolean per structure)
- `EnergyComponent.energy_received` (if partial power supported)

**Synced on change:**
- `PerPlayerEnergyPool` (summary stats)
- Pool state transitions (events)

**NOT synced (client-derived or server-only):**
- Coverage grid (client can reconstruct from conduit positions if needed for overlay)
- Rationing order (deterministic from entity IDs)

### 7.4 Event Broadcasting

| Event | Broadcast To |
|-------|--------------|
| EnergyStateChangedEvent | All clients (affects rendering) |
| EnergyDeficitBeganEvent | Affected overseer only (UI notification) |
| GridCollapseBeganEvent | All clients (affects rendering, major event) |
| NexusAgedEvent | Affected overseer only (UI notification) |

---

## 8. Cross-Epic Dependencies

### 8.1 Dependencies ON Epic 5 (What Epic 5 Requires)

| Epic | What Epic 5 Needs | Interface |
|------|-------------------|-----------|
| Epic 3 (Terrain) | Terrain validation for nexus placement | ITerrainQueryable.is_buildable() |
| Epic 3 (Terrain) | Water proximity for hydro nexus | ITerrainQueryable.get_water_distance() |
| Epic 3 (Terrain) | Volcanic terrain bonus for geothermal | ITerrainQueryable.get_terrain_type() |
| Epic 4 (Building) | Structure energy requirements | IEnergyConsumer interface |
| Epic 4 (Building) | Structure lifecycle events | BuildingConstructedEvent, BuildingDeconstructedEvent |
| Epic 4 (Building) | Nexus as a structure type | BuildingComponent, template system |

### 8.2 Dependencies FROM Epic 5 (What Epic 5 Provides)

| Epic | What Epic 5 Provides | Interface |
|------|---------------------|-----------|
| Epic 4 (Building) | Energy state queries | IEnergyProvider (replacing stub) |
| Epic 6 (Fluid) | Fluid extractor needs energy | IEnergyProvider.is_powered() |
| Epic 7 (Transport) | Rail terminal needs energy | IEnergyProvider.is_powered() |
| Epic 9 (Services) | Service structures need energy | IEnergyProvider.is_powered() |
| Epic 10 (Simulation) | Contamination from nexuses | IContaminationSource |
| Epic 10 (Population) | Energy affects habitability | IEnergyProvider pool state |
| Epic 12 (UI) | Energy overlay, pool status | IEnergyProvider queries, events |

### 8.3 Epic 6 (FluidSystem) Pattern Sharing

FluidSystem will follow a nearly identical pattern to EnergySystem:
- Per-overseer fluid pools
- Fluid extractors as producers
- Fluid conduits defining coverage
- Pool-based distribution

**Potential Shared Abstraction:**

```cpp
template<typename ProducerComponent, typename ConsumerComponent, typename ConduitComponent>
class UtilitySystem : public ISimulatable {
    // Shared pool calculation
    // Shared coverage algorithm
    // Shared rationing logic
};

class EnergySystem : public UtilitySystem<EnergyProducerComponent, EnergyComponent, EnergyConduitComponent>;
class FluidSystem : public UtilitySystem<FluidProducerComponent, FluidComponent, FluidConduitComponent>;
```

**@services-engineer:** Consider whether a shared UtilitySystem base class would reduce code duplication between Epic 5 and Epic 6.

---

## 9. Key Work Items

### 9.1 Infrastructure (Data Types and Storage)

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-001 | Energy type enumerations (NexusType, EnergyPoolState) | S | P0 |
| 5-002 | EnergyComponent structure (12 bytes) | S | P0 |
| 5-003 | EnergyProducerComponent structure (20 bytes) | S | P0 |
| 5-004 | EnergyConduitComponent structure (4 bytes) | S | P0 |
| 5-005 | PerPlayerEnergyPool structure | S | P0 |
| 5-006 | Coverage grid storage (dense grid, 1 byte/cell) | M | P0 |
| 5-007 | Energy event definitions | S | P0 |

### 9.2 EnergySystem Core

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-008 | EnergySystem class skeleton and ISimulatable | M | P0 |
| 5-009 | IEnergyProvider interface implementation (replaces Epic 4 stub) | M | P0 |
| 5-010 | Nexus registration and output calculation | M | P0 |
| 5-011 | Consumer registration and requirement aggregation | M | P0 |
| 5-012 | Pool calculation (generation - consumption = surplus) | M | P0 |
| 5-013 | Pool state machine (healthy, marginal, deficit, collapse) | M | P0 |

### 9.3 Coverage Zone

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-014 | Coverage zone algorithm (BFS flood-fill) | L | P0 |
| 5-015 | Dirty tracking for coverage recalculation | M | P1 |
| 5-016 | Ownership boundary enforcement in coverage | M | P0 |
| 5-017 | Coverage queries (is_in_coverage, get_coverage_at) | S | P0 |

### 9.4 Energy Distribution

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-018 | Energy distribution (powered = in coverage AND pool surplus) | M | P0 |
| 5-019 | Priority-based rationing during deficit | L | P0 |
| 5-020 | EnergyStateChangedEvent emission | S | P0 |
| 5-021 | Energy deficit/grid collapse event emission | S | P1 |

### 9.5 Nexus Mechanics

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-022 | Nexus aging and efficiency degradation | M | P1 |
| 5-023 | Nexus type definitions and base stats | M | P0 |
| 5-024 | Terrain bonuses (volcanic = geothermal, water = hydro) | M | P1 |
| 5-025 | IContaminationSource implementation for dirty nexuses | M | P1 |

### 9.6 Conduit Mechanics

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-026 | Energy conduit placement and validation | M | P0 |
| 5-027 | Conduit connection detection | M | P0 |
| 5-028 | Conduit removal and coverage update | M | P0 |

### 9.7 Integration

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-029 | BuildingConstructedEvent handler (register consumer/producer) | M | P0 |
| 5-030 | BuildingDeconstructedEvent handler (unregister) | M | P0 |
| 5-031 | Integration with Epic 4 BuildingSystem | L | P0 |
| 5-032 | Network serialization for EnergyComponent | M | P1 |
| 5-033 | Network serialization for pool state | S | P1 |

### 9.8 Content and Design

| Ticket | Description | Scope | Priority |
|--------|-------------|-------|----------|
| 5-034 | Define energy requirements per structure type/template | M | P0 |
| 5-035 | Define nexus output values and efficiency curves | M | P0 |
| 5-036 | Balance thresholds (deficit, collapse, rationing) | S | P2 |

### Work Item Summary

| Category | Count | S | M | L |
|----------|-------|---|---|---|
| Infrastructure | 7 | 5 | 2 | 0 |
| Core | 6 | 0 | 6 | 0 |
| Coverage | 4 | 1 | 2 | 1 |
| Distribution | 4 | 2 | 1 | 1 |
| Nexus | 4 | 0 | 4 | 0 |
| Conduit | 3 | 0 | 3 | 0 |
| Integration | 5 | 1 | 2 | 2 |
| Content | 3 | 1 | 2 | 0 |
| **Total** | **36** | **10** | **22** | **4** |

---

## 10. Questions for Other Agents

### @game-designer

1. **Energy deficit experience:** During energy deficit, should structures function at reduced capacity (e.g., fabrication at 50% output) or completely stop functioning? What visual treatment differentiates energy deficit from grid collapse?

2. **Rationing visibility:** Should overseers see which structures are being rationed and in what order? Should they be able to manually adjust priority levels, or are they fixed per structure type?

3. **Nexus variety:** How many energy nexus types should we target for MVP? The canon lists 9 types, but some are "future tech." Which subset should Epic 5 implement?

4. **Aging curve:** How steep should nexus aging be? Should a nexus ever reach 0% efficiency, or just asymptotically approach a minimum (e.g., 50%)? How does maintenance (spending credits) affect aging?

5. **Coverage visualization:** When the overseer is placing conduits, should they see a real-time preview of how the coverage zone will expand?

### @services-engineer

6. **Coverage calculation approach:** The analysis proposes a dense coverage grid with dirty tracking. Do you foresee any issues with this approach at 512x512 map scale with 4 overseers?

7. **Shared utility abstraction:** Given Epic 6 (FluidSystem) follows the same pattern, should we design a shared UtilitySystem base class now, or implement EnergySystem first and refactor later?

8. **Contamination integration:** For energy nexuses that implement IContaminationSource, should they emit contamination every tick, or only when actively generating? (Offline nexus = no contamination?)

### @building-engineer

9. **Structure energy requirements:** How should energy requirements be specified per structure template? Is it a field in BuildingComponent, or derived from template_id lookup?

10. **Nexus as structure:** Should energy nexuses use the same 5-state machine as zone structures (Materializing, Active, Abandoned, Derelict, Deconstructed)? Or do they have a different lifecycle?

---

## 11. Risks and Concerns

### 11.1 Performance Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Coverage calculation too slow on large maps | Medium | High | Dirty tracking, incremental updates, spatial partitioning |
| Per-tick consumer iteration at 10K+ entities | Medium | Medium | Entity lists per pool state, skip unchanged entities |
| Memory pressure from coverage grid | Low | Low | 1 byte per cell is minimal; 512x512 = 256KB |

**Benchmarking Target:** tick() < 1ms at 10,000 consumers on medium map (256x256).

### 11.2 Interface Stability Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| IEnergyProvider changes after Epic 4 integration | Medium | High | Lock interface before Epic 4 completes stubs |
| IContaminationSource changes in Epic 10 | Low | Medium | Define minimal interface early |

**Recommendation:** Finalize IEnergyProvider interface before Epic 4's 4-020 (stub implementation) is complete.

### 11.3 Forward Dependency Risks

| Risk | Description | Mitigation |
|------|-------------|------------|
| Epic 6 (Fluid) expects different pool model | FluidSystem might need nuances EnergySystem doesn't have (e.g., reservoir storage, pressure) | Define shared abstraction points, but allow specialization |
| Epic 7 (Transport) expects more from energy | Rail might need per-segment power tracking | Keep IEnergyProvider simple, let RailSystem query as needed |

### 11.4 Multiplayer Consistency Risks

| Risk | Description | Mitigation |
|------|-------------|------------|
| Rationing order differs between server/client | Clients should never compute rationing | Rationing is server-only; clients receive final is_powered state |
| Coverage grid desync | Coverage is derived, but if clients render overlay from local data... | Either sync coverage grid or have clients derive from synced conduit positions |

### 11.5 Design Ambiguity Risks

Several questions are unanswered and may affect implementation:

1. **Partial power:** Can structures receive partial energy (energy_received < energy_required)? Current design assumes binary (powered/unpowered).

2. **Nexus maintenance:** How does maintenance spending affect nexus efficiency? Not defined in current scope.

3. **Conduit damage:** Can conduits be damaged (disasters)? If so, coverage calculation needs health consideration.

4. **Energy trading:** patterns.yaml mentions energy trading between overseers. Is that in Epic 5 scope or a later epic?

**Recommendation:** Clarify these with @game-designer before ticket finalization.

---

## Appendix A: Canonical Terminology Reference

| Human Term | Canonical (Alien) Term |
|------------|------------------------|
| Power | Energy |
| Electricity | Energy |
| Power plant | Energy nexus |
| Power line | Energy conduit |
| Power grid | Energy matrix |
| Blackout | Grid collapse |
| Brownout | Energy deficit |
| Power on | Energized |
| Power off | De-energized |

---

## Appendix B: Interface Summary

```cpp
// Epic 5 implements these interfaces:
class EnergySystem :
    public ISimulatable,       // From SimulationCore
    public IEnergyProvider     // Replacing Epic 4 stub
{
    // ...
};

// Energy nexuses with contamination implement:
class CarbonNexus : public IContaminationSource { ... };
class PetrochemicalNexus : public IContaminationSource { ... };

// Epic 5 queries these interfaces (from Epic 4):
// - IEnergyConsumer (from building templates)
// - IEnergyProducer (from nexus types)

// Epic 5 queries these interfaces (from Epic 3):
// - ITerrainQueryable (for placement validation, terrain bonuses)
```

---

## Appendix C: Execution Order in Simulation Tick

```
Tick Order (by priority):

 5  TerrainSystem      (rarely changes, early to settle terrain state)
10  EnergySystem       <-- Epic 5
20  FluidSystem        (Epic 6)
30  ZoneSystem         (needs energy/fluid state)
40  BuildingSystem     (needs zone, energy, fluid state)
50  PopulationSystem   (needs building state)
60  EconomySystem      (needs population, building state)
70  DisorderSystem     (needs population, land value)
80  ContaminationSystem (needs building, nexus state)
```

EnergySystem runs early (priority 10) to ensure all downstream systems have accurate energy state for their calculations.

---

**Document Status:** Ready for review by Game Designer and other agents.

**Next Steps:**
1. @game-designer to answer experience questions (Section 10)
2. @services-engineer to review technical approach
3. Convert work items to formal tickets after questions resolved
