# Epic 5: Power Infrastructure (EnergySystem) - Services Engineer Analysis

**Author:** Services Engineer Agent
**Date:** 2026-01-28
**Epic:** 5 - Power Infrastructure
**Status:** Planning
**Canon Version:** 0.6.0

---

## Table of Contents

1. [Coverage System Design](#1-coverage-system-design)
2. [Pool Calculation Algorithm](#2-pool-calculation-algorithm)
3. [Component Designs](#3-component-designs)
4. [Energy Nexus Types](#4-energy-nexus-types)
5. [Contamination Integration](#5-contamination-integration)
6. [Priority-Based Rationing](#6-priority-based-rationing)
7. [Coverage Algorithm Implementation](#7-coverage-algorithm-implementation)
8. [Tick Order and Updates](#8-tick-order-and-updates)
9. [Network Sync](#9-network-sync)
10. [Key Work Items](#10-key-work-items)
11. [Questions for Other Agents](#11-questions-for-other-agents)
12. [Risks and Concerns](#12-risks-and-concerns)

---

## 1. Coverage System Design

### Overview

The energy matrix uses a **POOL model** (per canon `patterns.yaml`) where energy conduits define **coverage zones**, not transport routes. A structure is powered if:
1. It is within the overseer's coverage zone
2. The overseer's energy pool has sufficient surplus

### Coverage Zone Concept

Coverage zones represent the reach of the energy matrix from energy nexuses through connected energy conduits. Unlike physical simulation, energy does not "flow" through conduits - conduits simply extend the area where structures can draw from the pool.

```
+------------------+     +------------------+     +------------------+
|  Energy Nexus    |---->|  Coverage Zone   |---->|   Structures     |
|  (base radius)   |     |  (extended by    |     |   (in coverage)  |
+------------------+     |   conduits)      |     +------------------+
                         +------------------+
                                  |
                                  v
                         +------------------+
                         |  Energy Pool     |
                         |  (has surplus?)  |
                         +------------------+
                                  |
                                  v
                         +------------------+
                         |  is_powered      |
                         |  = in_coverage   |
                         |  AND has_surplus |
                         +------------------+
```

### Coverage Grid vs Per-Entity Tracking

**Decision: Use Dense 2D Coverage Grid**

Per the `dense_grid_exception` pattern in canon, high-density spatial data covering every cell should use dense array storage. The coverage grid fits this exception perfectly.

#### Dense Coverage Grid Approach

```cpp
// Coverage grid stores overseer ID (0 = uncovered)
class CoverageGrid {
    uint8_t* grid_;           // 1 byte per sector: 0=uncovered, 1-4=overseer_id
    uint32_t width_;
    uint32_t height_;

public:
    // O(1) coverage query
    bool is_in_coverage(int32_t x, int32_t y, PlayerID owner) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
        return grid_[y * width_ + x] == owner;
    }

    // O(1) any-coverage query (for rendering overlay)
    PlayerID get_coverage_owner(int32_t x, int32_t y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0;
        return grid_[y * width_ + x];
    }
};
```

**Memory Budget:**
| Map Size | Grid Memory |
|----------|-------------|
| 128x128 | 16 KB |
| 256x256 | 64 KB |
| 512x512 | 256 KB |

**Benefits:**
- O(1) coverage queries for all 10,000+ structures per tick
- Cache-friendly sequential access during iteration
- Simple implementation matching TerrainGrid pattern from Epic 3

**Trade-off:**
- Must recalculate on conduit placement/removal
- Addressed via dirty flag optimization (next section)

### Dirty Flag Optimization

Coverage recalculation is expensive (flood-fill across entire overseer's network). We use dirty flags to ensure recalculation only happens when the network changes.

```cpp
class EnergySystem {
private:
    CoverageGrid coverage_grid_;
    bool coverage_dirty_[MAX_PLAYERS];  // Per-overseer dirty flags

    void on_conduit_placed(const ConduitPlacedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void on_conduit_removed(const ConduitRemovedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void on_nexus_placed(const NexusPlacedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void on_nexus_removed(const NexusRemovedEvent& e) {
        coverage_dirty_[e.owner] = true;
    }

    void tick(float delta_time) {
        // Phase 1: Recalculate coverage for dirty overseers
        for (PlayerID i = 1; i <= num_players_; ++i) {
            if (coverage_dirty_[i]) {
                recalculate_coverage(i);
                coverage_dirty_[i] = false;
            }
        }
        // ... remaining tick phases
    }
};
```

**Dirty Flag Triggers:**
- Energy nexus placed
- Energy nexus removed/deconstructed
- Energy conduit placed
- Energy conduit removed/deconstructed
- Energy nexus goes online/offline (if this affects coverage)

### BuildingSystem O(1) Coverage Queries

BuildingSystem (and other systems) query coverage via IEnergyProvider:

```cpp
// IEnergyProvider interface (replaces Epic 4 stub)
class IEnergyProvider {
public:
    virtual ~IEnergyProvider() = default;

    // Entity-based query: is this structure powered?
    virtual bool is_powered(EntityID entity) const = 0;

    // Position-based query: would a structure at (x,y) be powered?
    virtual bool is_powered_at(int32_t x, int32_t y, PlayerID owner) const = 0;
};

// EnergySystem implementation
bool EnergySystem::is_powered_at(int32_t x, int32_t y, PlayerID owner) const {
    // O(1) coverage check
    if (!coverage_grid_.is_in_coverage(x, y, owner)) {
        return false;
    }
    // O(1) pool surplus check
    return pools_[owner].surplus >= 0;
}
```

---

## 2. Pool Calculation Algorithm

### Per-Overseer Energy Pool

Each overseer has an independent energy pool. Energy does NOT flow between overseers. This simplifies calculation and prevents griefing.

```cpp
struct PerPlayerEnergyPool {
    PlayerID owner;
    uint32_t total_generated;     // Sum of all nexus current_output
    uint32_t total_consumed;      // Sum of all consumer energy_required (in coverage)
    int32_t  surplus;             // generated - consumed (can be negative)
    uint32_t nexus_count;         // Active nexus count
    uint32_t consumer_count;      // Consumers in coverage count
    EnergyPoolState state;        // Healthy, Marginal, Deficit, Collapse
};
```

### Pool Calculation Steps

Each tick, the pool is recalculated for each overseer:

```
calculate_pool(overseer_id):
    pool = PerPlayerEnergyPool()
    pool.owner = overseer_id
    pool.total_generated = 0
    pool.total_consumed = 0
    pool.nexus_count = 0
    pool.consumer_count = 0

    // Step 1: Sum generation from all nexuses
    for nexus in get_nexuses_for_overseer(overseer_id):
        if nexus.is_online:
            pool.total_generated += nexus.current_output
            pool.nexus_count += 1

    // Step 2: Sum consumption from consumers IN COVERAGE
    for consumer in get_consumers_for_overseer(overseer_id):
        pos = get_position(consumer.entity)
        if coverage_grid_.is_in_coverage(pos.x, pos.y, overseer_id):
            pool.total_consumed += consumer.energy_required
            pool.consumer_count += 1

    // Step 3: Calculate surplus
    pool.surplus = pool.total_generated - pool.total_consumed

    // Step 4: Determine pool state
    pool.state = calculate_pool_state(pool)

    return pool
```

### Pool States

```cpp
enum class EnergyPoolState : uint8_t {
    Healthy = 0,    // surplus >= buffer_threshold (10% of generation)
    Marginal = 1,   // 0 <= surplus < buffer_threshold
    Deficit = 2,    // -collapse_threshold < surplus < 0
    Collapse = 3    // surplus <= -collapse_threshold (50% of consumption)
};
```

**State Calculation:**
```cpp
EnergyPoolState calculate_pool_state(const PerPlayerEnergyPool& pool) {
    int32_t buffer_threshold = pool.total_generated / 10;  // 10% buffer
    int32_t collapse_threshold = pool.total_consumed / 2;  // 50% deficit

    if (pool.surplus >= buffer_threshold) {
        return EnergyPoolState::Healthy;
    } else if (pool.surplus >= 0) {
        return EnergyPoolState::Marginal;
    } else if (pool.surplus > -collapse_threshold) {
        return EnergyPoolState::Deficit;
    } else {
        return EnergyPoolState::Collapse;
    }
}
```

### Energy Deficit Detection

When `pool.surplus < 0`, the overseer is in energy deficit (canonical term: "energy deficit"). When `pool.surplus <= -collapse_threshold`, the overseer is in grid collapse.

**@game-designer questions answered:**
- Energy deficit = some structures unpowered
- Grid collapse = most structures unpowered except critical priority

---

## 3. Component Designs

### 3.1 EnergyComponent (Consumers)

Attached to all structures that consume energy: zone structures, service structures, fluid extractors, rail terminals, etc.

```cpp
struct EnergyComponent {
    uint32_t energy_required;     // Energy units needed per tick (from template)
    uint32_t energy_received;     // Energy units actually received this tick
    bool     is_powered;          // True if energy_received >= energy_required
    uint8_t  priority;            // Rationing priority: 1=critical, 2=important, 3=normal, 4=low
    uint8_t  grid_id;             // Which energy grid this consumer is on (for future)
    uint8_t  padding;             // Alignment to 12 bytes
};
// Size: 12 bytes (verified with static_assert)
```

**Field Details:**

| Field | Size | Purpose |
|-------|------|---------|
| energy_required | 4 bytes | Set from structure template; defines consumption |
| energy_received | 4 bytes | Set by EnergySystem each tick; actual energy delivered |
| is_powered | 1 byte | True if sufficient energy; read by other systems |
| priority | 1 byte | Used during rationing; 1-4 priority levels |
| grid_id | 1 byte | Future: support for isolated grids |
| padding | 1 byte | Alignment |

**Priority Values:**
```cpp
constexpr uint8_t ENERGY_PRIORITY_CRITICAL = 1;   // Medical nexus, command nexus
constexpr uint8_t ENERGY_PRIORITY_IMPORTANT = 2;  // Enforcer post, hazard response
constexpr uint8_t ENERGY_PRIORITY_NORMAL = 3;     // Exchange, fabrication structures
constexpr uint8_t ENERGY_PRIORITY_LOW = 4;        // Habitation structures
```

### 3.2 EnergyProducerComponent (Nexuses)

Attached to energy nexuses that generate energy.

```cpp
struct EnergyProducerComponent {
    uint32_t base_output;         // Maximum output at 100% efficiency
    uint32_t current_output;      // Actual output this tick (after efficiency)
    float    efficiency;          // Current efficiency multiplier (0.0 - 1.0)
    float    age_factor;          // Aging degradation (1.0 = new, decays)
    uint16_t ticks_since_built;   // Age in ticks (capped at 65535)
    uint8_t  nexus_type;          // NexusType enum
    bool     is_online;           // True if operational
    uint32_t contamination_output; // Contamination units per tick (0 = clean)
};
// Size: 24 bytes
```

**Field Details:**

| Field | Size | Purpose |
|-------|------|---------|
| base_output | 4 bytes | Template-defined maximum output |
| current_output | 4 bytes | `base_output * efficiency * age_factor` |
| efficiency | 4 bytes | Affected by maintenance, terrain bonuses |
| age_factor | 4 bytes | Starts at 1.0, decays per aging curve |
| ticks_since_built | 2 bytes | Tracks age for degradation |
| nexus_type | 1 byte | Enum: Carbon, Petrochemical, Gaseous, Nuclear, Wind, Hydro, Solar, Geothermal |
| is_online | 1 byte | False = not contributing to pool |
| contamination_output | 4 bytes | For IContaminationSource |

**Output Calculation:**
```cpp
void update_nexus_output(EnergyProducerComponent& nexus) {
    if (!nexus.is_online) {
        nexus.current_output = 0;
        return;
    }
    nexus.current_output = static_cast<uint32_t>(
        nexus.base_output * nexus.efficiency * nexus.age_factor
    );
}
```

### 3.3 EnergyConduitComponent (Conduits)

Attached to energy conduit entities that extend coverage.

```cpp
struct EnergyConduitComponent {
    uint8_t  coverage_radius;     // Tiles of coverage this conduit adds (default: 2-3)
    bool     is_connected;        // True if connected to energy matrix
    bool     is_active;           // True if carrying energy (pool has generation)
    uint8_t  conduit_level;       // 1 = basic, 2 = upgraded (future expansion)
};
// Size: 4 bytes
```

**Field Details:**

| Field | Size | Purpose |
|-------|------|---------|
| coverage_radius | 1 byte | How many tiles around this conduit are covered |
| is_connected | 1 byte | Set by coverage algorithm; false = isolated segment |
| is_active | 1 byte | For rendering: glow if energy flowing |
| conduit_level | 1 byte | Future: upgraded conduits with larger radius |

### Size Summary

| Component | Size | Typical Count (256x256) | Memory |
|-----------|------|-------------------------|--------|
| EnergyComponent | 12 bytes | ~20,000 structures | 240 KB |
| EnergyProducerComponent | 24 bytes | ~50-100 nexuses | 2.4 KB |
| EnergyConduitComponent | 4 bytes | ~2,000-5,000 conduits | 20 KB |
| CoverageGrid | 1 byte/cell | 65,536 cells | 64 KB |
| **Total** | | | **~326 KB** |

---

## 4. Energy Nexus Types

### Nexus Type Enum

```cpp
enum class NexusType : uint8_t {
    Carbon = 0,           // Coal/plasma - high output, high contamination, cheap
    Petrochemical = 1,    // Oil/petroleum - medium output, medium contamination
    Gaseous = 2,          // Gas/methane - cleaner, moderate output
    Nuclear = 3,          // Very high output, no pollution, meltdown risk
    Wind = 4,             // Variable output (weather stub), no pollution
    Solar = 5,            // Variable output (day/night stub), no pollution
    Hydro = 6,            // Requires water proximity, stable, no pollution
    Geothermal = 7,       // Requires ember_crust terrain, stable, no pollution
    Fusion = 8,           // Future tech: massive output, no pollution
    MicrowaveReceiver = 9 // Future tech: massive output, no pollution
};
```

### Nexus Configuration Data

```cpp
struct NexusTypeConfig {
    NexusType type;
    uint32_t base_output;           // Energy units per tick
    uint32_t build_cost;            // Credits to construct
    uint32_t maintenance_cost;      // Credits per cycle
    uint32_t contamination_rate;    // Contamination units per tick
    uint8_t  contamination_radius;  // Tiles affected by contamination
    uint8_t  coverage_radius;       // Base coverage radius from nexus
    uint16_t lifespan_ticks;        // Ticks until efficiency degrades
    float    min_efficiency;        // Floor efficiency after max aging
    TerrainRequirement terrain_req; // Required terrain proximity
};

// Example configurations (to be finalized by @game-designer)
const NexusTypeConfig NEXUS_CONFIGS[] = {
    // type          base_out  build  maint  contam  c_rad  cov   life   min_eff  terrain
    { Carbon,            100,  5000,    50,    200,     8,    8,  6000,   0.60f,  None },
    { Petrochemical,     150,  8000,    75,    120,     6,    8,  8000,   0.65f,  None },
    { Gaseous,           120, 10000,    60,     40,     3,    8, 10000,   0.70f,  None },
    { Nuclear,           400, 50000,   200,      0,     0,   10, 20000,   0.75f,  None },
    { Wind,               30,  3000,    20,      0,     0,    4,  5000,   0.80f,  Ridges },
    { Solar,              50,  6000,    30,      0,     0,    6,  8000,   0.85f,  None },
    { Hydro,             200, 25000,    80,      0,     0,   10, 25000,   0.90f,  Water },
    { Geothermal,        250, 30000,    90,      0,     0,   10, 30000,   0.90f,  EmberCrust },
    { Fusion,            800,100000,   400,      0,     0,   12, 50000,   0.95f,  None },
    { MicrowaveReceiver, 600, 80000,   300,      0,     0,   12, 40000,   0.95f,  None },
};
```

### Variable Output (Stubs)

Wind and Solar nexuses have variable output based on conditions. For Epic 5 MVP, we implement stubs:

```cpp
float get_weather_factor(NexusType type) {
    if (type == NexusType::Wind) {
        // Stub: return random value between 0.5 and 1.0
        // Future: weather system integration
        return 0.75f;  // Average for now
    }
    return 1.0f;
}

float get_daynight_factor(NexusType type) {
    if (type == NexusType::Solar) {
        // Stub: return value based on day/night cycle
        // Future: day/night system integration
        return 0.80f;  // Average for now
    }
    return 1.0f;
}
```

### Terrain Requirements

Some nexuses require specific terrain proximity:

```cpp
enum class TerrainRequirement : uint8_t {
    None = 0,        // Can be placed anywhere buildable
    Water = 1,       // Must be within 3 tiles of water (hydro)
    EmberCrust = 2,  // Must be on or adjacent to ember_crust (geothermal)
    Ridges = 3       // Bonus efficiency on ridges/elevated terrain (wind)
};

bool validate_nexus_placement(NexusType type, int32_t x, int32_t y) {
    TerrainRequirement req = NEXUS_CONFIGS[type].terrain_req;

    switch (req) {
        case TerrainRequirement::Water:
            return terrain_->get_water_distance(x, y) <= 3;

        case TerrainRequirement::EmberCrust:
            return terrain_->get_terrain_type(x, y) == TerrainType::VolcanicRock;

        case TerrainRequirement::Ridges:
            // No requirement, just bonus
            return true;

        default:
            return terrain_->is_buildable(x, y);
    }
}
```

---

## 5. Contamination Integration

### Dirty Nexuses as Contamination Sources

Carbon, Petrochemical, and Gaseous nexuses implement `IContaminationSource` from `interfaces.yaml`. ContaminationSystem (Epic 10) queries these sources.

### EnergyProducerComponent.contamination_output

The `contamination_output` field in `EnergyProducerComponent` defines how much contamination this nexus produces per tick.

```cpp
// Set at nexus creation based on type
void initialize_nexus_contamination(EnergyProducerComponent& nexus) {
    const NexusTypeConfig& config = NEXUS_CONFIGS[nexus.nexus_type];
    nexus.contamination_output = config.contamination_rate;
}
```

### IContaminationSource Implementation

```cpp
// Per interfaces.yaml
class IContaminationSource {
public:
    virtual ~IContaminationSource() = default;
    virtual uint32_t get_contamination_output() const = 0;
    virtual ContaminationType get_contamination_type() const = 0;
};

enum class ContaminationType : uint8_t {
    Industrial = 0,  // From fabrication zones
    Traffic = 1,     // From congested roads
    Energy = 2,      // From polluting energy nexuses
    Terrain = 3      // From blight_mires (toxic_marshes)
};

// EnergySystem queries for contamination sources
std::vector<ContaminationSourceData> EnergySystem::get_contamination_sources() const {
    std::vector<ContaminationSourceData> sources;

    for (const auto& [entity, producer, pos] :
         registry_.view<EnergyProducerComponent, PositionComponent>().each()) {

        if (producer.contamination_output > 0 && producer.is_online) {
            const NexusTypeConfig& config = NEXUS_CONFIGS[producer.nexus_type];
            sources.push_back({
                .position = pos,
                .output = producer.contamination_output,
                .radius = config.contamination_radius,
                .type = ContaminationType::Energy
            });
        }
    }

    return sources;
}
```

### Contamination Only When Active

Per @systems-architect's question: contamination is only emitted when the nexus is online and generating.

```cpp
uint32_t EnergyProducerComponent::get_active_contamination() const {
    if (!is_online || current_output == 0) {
        return 0;  // Offline nexus = no contamination
    }
    return contamination_output;
}
```

---

## 6. Priority-Based Rationing

### When Rationing Occurs

Rationing occurs when the overseer's pool is in Deficit or Collapse state (`surplus < 0`).

### Priority Levels

From `interfaces.yaml` IEnergyConsumer notes:

| Priority | Level | Structure Types | Rationale |
|----------|-------|-----------------|-----------|
| 1 | Critical | Medical nexus, command nexus | Life support, essential services |
| 2 | Important | Enforcer post, hazard response | Safety services |
| 3 | Normal | Exchange, fabrication structures | Economy |
| 4 | Low | Habitation structures | Beings can survive short outages |

### Rationing Algorithm

```cpp
void apply_rationing(PlayerID owner) {
    PerPlayerEnergyPool& pool = pools_[owner];

    if (pool.surplus >= 0) {
        // No rationing needed - all consumers powered
        power_all_consumers_in_coverage(owner);
        return;
    }

    // Collect all consumers in coverage for this overseer
    std::vector<EntityID> consumers;
    for (const auto& [entity, energy, pos] :
         registry_.view<EnergyComponent, PositionComponent>().each()) {

        if (get_owner(entity) == owner &&
            coverage_grid_.is_in_coverage(pos.grid_x, pos.grid_y, owner)) {
            consumers.push_back(entity);
        }
    }

    // Sort by priority (ascending = higher priority first), then entity ID for determinism
    std::sort(consumers.begin(), consumers.end(),
        [this](EntityID a, EntityID b) {
            auto& ea = registry_.get<EnergyComponent>(a);
            auto& eb = registry_.get<EnergyComponent>(b);
            if (ea.priority != eb.priority) {
                return ea.priority < eb.priority;  // Lower number = higher priority
            }
            return a < b;  // Deterministic tie-breaker
        });

    // Distribute available energy by priority
    int32_t available = pool.total_generated;

    for (EntityID entity : consumers) {
        auto& energy = registry_.get<EnergyComponent>(entity);

        if (available >= static_cast<int32_t>(energy.energy_required)) {
            // Fully power this consumer
            energy.energy_received = energy.energy_required;
            energy.is_powered = true;
            available -= energy.energy_required;
        } else {
            // Cannot power this consumer (or partial - not implemented)
            energy.energy_received = 0;
            energy.is_powered = false;
        }
    }
}
```

### Deterministic Tie-Breaking

For multiplayer consistency, tie-breaking within the same priority uses entity ID (deterministic uint32_t ordering). This ensures server and all clients agree on rationing order.

### Binary vs Partial Power

Current design: **Binary power only**. Structures are either fully powered or unpowered. Partial power (energy_received < energy_required) could be a future enhancement but adds complexity.

---

## 7. Coverage Algorithm Implementation

### BFS Flood-Fill Algorithm

Coverage calculation uses Breadth-First Search (BFS) starting from nexuses and flooding through connected conduits.

```cpp
void recalculate_coverage(PlayerID owner) {
    // Clear coverage for this overseer
    for (uint32_t y = 0; y < height_; ++y) {
        for (uint32_t x = 0; x < width_; ++x) {
            if (coverage_grid_.get_coverage_owner(x, y) == owner) {
                coverage_grid_.clear(x, y);
            }
        }
    }

    // Queue for BFS
    std::queue<GridPosition> frontier;

    // Step 1: Seed from nexuses
    for (const auto& [entity, producer, pos] :
         registry_.view<EnergyProducerComponent, PositionComponent>().each()) {

        if (get_owner(entity) == owner && producer.is_online) {
            const NexusTypeConfig& config = NEXUS_CONFIGS[producer.nexus_type];

            // Mark nexus coverage radius
            mark_coverage_radius(pos.grid_x, pos.grid_y,
                                 config.coverage_radius, owner);

            // Add nexus position to frontier
            frontier.push({pos.grid_x, pos.grid_y});
        }
    }

    // Step 2: BFS through conduits
    std::set<GridPosition> visited;

    while (!frontier.empty()) {
        GridPosition current = frontier.front();
        frontier.pop();

        if (visited.count(current)) continue;
        visited.insert(current);

        // Check 4-directional neighbors
        static const int dx[] = {0, 0, 1, -1};
        static const int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int32_t nx = current.x + dx[i];
            int32_t ny = current.y + dy[i];

            if (!in_bounds(nx, ny)) continue;
            if (visited.count({nx, ny})) continue;

            // Check if neighbor has a conduit owned by this overseer
            EntityID conduit = get_conduit_at(nx, ny, owner);
            if (conduit != INVALID_ENTITY) {
                auto& comp = registry_.get<EnergyConduitComponent>(conduit);
                comp.is_connected = true;

                // Mark conduit coverage radius
                mark_coverage_radius(nx, ny, comp.coverage_radius, owner);

                // Add to frontier for continued BFS
                frontier.push({nx, ny});
            }
        }
    }
}

void mark_coverage_radius(int32_t cx, int32_t cy, uint8_t radius, PlayerID owner) {
    for (int32_t dy = -radius; dy <= radius; ++dy) {
        for (int32_t dx = -radius; dx <= radius; ++dx) {
            int32_t x = cx + dx;
            int32_t y = cy + dy;

            if (in_bounds(x, y)) {
                // Only mark if within ownership (conduits don't cross boundaries)
                if (get_tile_owner(x, y) == owner || get_tile_owner(x, y) == GAME_MASTER) {
                    coverage_grid_.set(x, y, owner);
                }
            }
        }
    }
}
```

### Coverage Distance Limit

Each conduit adds coverage within its `coverage_radius` (default 2-3 tiles). This is a square area, not circular, for simplicity.

### Ownership Boundary Enforcement

Per `patterns.yaml`, energy conduits do NOT connect across overseer boundaries:

```cpp
// When flooding through conduits, only continue if:
// 1. The tile is owned by the same overseer, OR
// 2. The tile is owned by GAME_MASTER (unclaimed)

bool can_extend_coverage_to(int32_t x, int32_t y, PlayerID owner) {
    PlayerID tile_owner = get_tile_owner(x, y);
    return (tile_owner == owner || tile_owner == GAME_MASTER);
}
```

---

## 8. Tick Order and Updates

### EnergySystem Tick Priority

Per `interfaces.yaml`, EnergySystem runs at **priority 10** - early in the tick order to ensure energy state is settled before downstream systems query it.

```
Tick Order:
 5  TerrainSystem      (rarely changes)
10  EnergySystem       <-- Epic 5 (this system)
20  FluidSystem        (Epic 6)
30  ZoneSystem         (needs energy state)
40  BuildingSystem     (needs energy state)
50  PopulationSystem   (needs building state)
60  EconomySystem      (needs population)
70  DisorderSystem     (needs population, land value)
80  ContaminationSystem (queries energy contamination sources)
```

### EnergySystem Tick Phases

```cpp
void EnergySystem::tick(float delta_time) {
    // Phase 1: Recalculate coverage (if dirty)
    for (PlayerID i = 1; i <= num_players_; ++i) {
        if (coverage_dirty_[i]) {
            recalculate_coverage(i);
            coverage_dirty_[i] = false;
        }
    }

    // Phase 2: Update all nexus outputs
    for (auto& [entity, producer] :
         registry_.view<EnergyProducerComponent>().each()) {
        update_nexus_aging(producer, delta_time);
        update_nexus_output(producer);
    }

    // Phase 3: Calculate per-overseer pools
    for (PlayerID i = 1; i <= num_players_; ++i) {
        calculate_pool(i);
    }

    // Phase 4: Detect state transitions (emit events)
    for (PlayerID i = 1; i <= num_players_; ++i) {
        detect_pool_state_transitions(i);
    }

    // Phase 5: Apply energy distribution (or rationing if deficit)
    for (PlayerID i = 1; i <= num_players_; ++i) {
        apply_rationing(i);  // Handles both surplus and deficit cases
    }

    // Phase 6: Emit EnergyStateChangedEvent for structures that changed
    emit_state_change_events();

    // Phase 7: Update conduit is_active flags for rendering
    update_conduit_active_states();
}
```

### Phase Details

**Phase 1: Recalculate coverage**
- Only runs if `coverage_dirty_[owner]` is true
- O(N) where N = number of conduits + nexuses for that owner
- Performance target: < 10ms on 512x512 map

**Phase 2: Update nexus outputs**
- Update aging (increment `ticks_since_built`, recalculate `age_factor`)
- Recalculate `current_output = base_output * efficiency * age_factor`
- Apply weather/day-night stubs for wind/solar

**Phase 3: Calculate pools**
- Sum `current_output` from all online nexuses
- Sum `energy_required` from all consumers in coverage
- Calculate surplus and pool state

**Phase 4: State transitions**
- Compare previous pool state to new state
- Emit `EnergyDeficitBeganEvent` or `EnergyDeficitEndedEvent`
- Emit `GridCollapseBeganEvent` or `GridCollapseEndedEvent`

**Phase 5: Apply rationing**
- If surplus >= 0: power all consumers in coverage
- If deficit: apply priority-based rationing

**Phase 6: State change events**
- Compare each consumer's `is_powered` to previous tick
- Emit `EnergyStateChangedEvent` for changes

**Phase 7: Conduit active states**
- Set `is_active = true` for conduits connected to online nexuses
- Used by RenderingSystem for glow effects

---

## 9. Network Sync

### What Syncs to Clients

**Synced Every Tick:**
| Data | Purpose | Size |
|------|---------|------|
| `EnergyComponent.is_powered` | Structure power state for rendering | 1 bit per entity |
| `EnergyComponent.energy_received` | Optional: for UI display | 4 bytes per entity |

**Synced On Change:**
| Data | Purpose | Trigger |
|------|---------|---------|
| `PerPlayerEnergyPool` summary | UI energy readout | Pool values change |
| Pool state transitions | UI notifications | State crosses threshold |
| `EnergyConduitComponent.is_active` | Conduit glow rendering | Nexus online/offline |

**Coverage Grid Sync:**

Two options:

**Option A: Sync full grid (simpler)**
- Send entire coverage grid on any change
- 256 KB for 512x512 (compressed: ~50KB with RLE)
- Simple client implementation

**Option B: Client reconstructs (no sync)**
- Client knows conduit positions from entity sync
- Client runs same BFS algorithm locally
- Zero additional bandwidth
- More complex client code

**Recommendation:** Option B (client reconstructs) for MVP. Coverage is deterministic from conduit positions.

### Pool Summary Message

```cpp
struct EnergyPoolSyncMessage {
    PlayerID owner;
    uint32_t total_generated;
    uint32_t total_consumed;
    int32_t  surplus;
    EnergyPoolState state;
};
// Size: 16 bytes per player, sent on change
```

### Energy Overlay Data for UI

The energy overlay displays:
1. Coverage zones (colored by owner)
2. Powered vs. unpowered structures
3. Conduit network

```cpp
// Client-side: reconstruct from synced entity positions
void render_energy_overlay() {
    // Draw coverage zones
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            PlayerID owner = local_coverage_grid_.get_coverage_owner(x, y);
            if (owner > 0) {
                draw_coverage_tile(x, y, get_player_color(owner));
            }
        }
    }

    // Draw structures with power state
    for (entity : structures_with_energy_component) {
        bool powered = entity.energy.is_powered;
        draw_structure_overlay(entity, powered ? COLOR_CYAN : COLOR_RED);
    }

    // Draw conduits with active state
    for (entity : conduits) {
        bool active = entity.conduit.is_active;
        draw_conduit(entity, active ? COLOR_CYAN : COLOR_DIM);
    }
}
```

---

## 10. Key Work Items

### Infrastructure Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-001 | `EnergyComponent` struct definition (12 bytes) | S | P0 | None |
| E-002 | `EnergyProducerComponent` struct definition (24 bytes) | S | P0 | None |
| E-003 | `EnergyConduitComponent` struct definition (4 bytes) | S | P0 | None |
| E-004 | `NexusType` enum and configuration data | S | P0 | None |
| E-005 | `EnergyPoolState` enum and `PerPlayerEnergyPool` struct | S | P0 | None |
| E-006 | Energy event definitions (`EnergyStateChangedEvent`, etc.) | S | P0 | None |

### Coverage System Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-007 | `CoverageGrid` dense 2D array class | M | P0 | None |
| E-008 | Coverage BFS flood-fill algorithm | L | P0 | E-007 |
| E-009 | Coverage dirty flag tracking | M | P1 | E-008 |
| E-010 | Ownership boundary enforcement in coverage | M | P0 | E-008 |
| E-011 | `is_in_coverage()` O(1) query | S | P0 | E-007 |

### Pool Calculation Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-012 | Per-player pool calculation (sum generation, consumption) | M | P0 | E-001, E-002 |
| E-013 | Pool state machine (Healthy, Marginal, Deficit, Collapse) | M | P0 | E-012 |
| E-014 | Pool state transition detection and events | M | P1 | E-013 |

### Energy Distribution Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-015 | Energy distribution (powered = in coverage AND surplus) | M | P0 | E-011, E-012 |
| E-016 | Priority-based rationing algorithm | L | P0 | E-015 |
| E-017 | Deterministic tie-breaking for multiplayer | S | P0 | E-016 |
| E-018 | `EnergyStateChangedEvent` emission | S | P1 | E-015 |

### EnergySystem Integration Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-019 | `EnergySystem` class skeleton and `ISimulatable` | M | P0 | All infra |
| E-020 | `IEnergyProvider` real implementation (replaces Epic 4 stub) | M | P0 | E-019 |
| E-021 | Event handlers for `BuildingConstructedEvent`/`DeconstructedEvent` | M | P0 | E-019 |
| E-022 | Event handlers for conduit placed/removed | M | P0 | E-019, E-009 |

### Nexus Mechanics Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-023 | Nexus output calculation (`base * efficiency * age_factor`) | M | P0 | E-002 |
| E-024 | Nexus aging and efficiency degradation | M | P1 | E-023 |
| E-025 | Weather/day-night stubs for Wind/Solar | S | P2 | E-023 |
| E-026 | Terrain requirement validation for placement | M | P1 | E-004 |
| E-027 | Terrain bonus efficiency (ridges for wind, etc.) | M | P2 | E-026 |

### Contamination Integration Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-028 | `IContaminationSource` implementation for dirty nexuses | M | P1 | E-002 |
| E-029 | Contamination output query API for ContaminationSystem | S | P1 | E-028 |
| E-030 | Contamination only when nexus is_online | S | P1 | E-028 |

### Conduit Mechanics Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-031 | Energy conduit placement validation | M | P0 | E-003 |
| E-032 | Conduit connection detection (for visual is_connected) | M | P0 | E-008 |
| E-033 | Conduit removal and coverage update | M | P0 | E-009 |

### Network Sync Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-034 | `EnergyComponent` serialization | M | P1 | E-001 |
| E-035 | `PerPlayerEnergyPool` sync message | S | P1 | E-005 |
| E-036 | Energy overlay data for client-side reconstruction | M | P2 | E-007 |

### Content/Design Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| E-037 | Define energy_required per structure template | M | P0 | @game-designer |
| E-038 | Define nexus output values and costs | M | P0 | @game-designer |
| E-039 | Balance deficit/collapse thresholds | S | P2 | E-013 |
| E-040 | Priority assignments per structure type | S | P0 | @game-designer |

### Work Item Summary

| Category | Count | S | M | L |
|----------|-------|---|---|---|
| Infrastructure | 6 | 6 | 0 | 0 |
| Coverage | 5 | 2 | 2 | 1 |
| Pool Calculation | 3 | 0 | 3 | 0 |
| Distribution | 4 | 2 | 1 | 1 |
| EnergySystem | 4 | 0 | 4 | 0 |
| Nexus Mechanics | 5 | 1 | 4 | 0 |
| Contamination | 3 | 2 | 1 | 0 |
| Conduit | 3 | 0 | 3 | 0 |
| Network | 3 | 1 | 2 | 0 |
| Content | 4 | 2 | 2 | 0 |
| **Total** | **40** | **16** | **22** | **2** |

---

## 11. Questions for Other Agents

### @systems-architect

1. **Coverage grid vs entity tracking finalized?** My analysis recommends dense 2D grid with dirty flags. Do you concur, or should we consider hybrid approaches (e.g., spatial hash for sparse conduit networks)?

2. **Dirty flag propagation scope:** When a conduit is placed, should we only dirty the owning overseer's coverage, or should we also dirty adjacent overseers (in case there's an edge case with boundary handling)?

3. **Shared UtilitySystem base class:** Given FluidSystem (Epic 6) follows the same pool model, should we extract a shared `UtilitySystemBase<TProducer, TConsumer, TConduit>` template now, or implement EnergySystem fully first and refactor?

4. **Coverage grid sync strategy:** I recommend client-side reconstruction (no sync). Does this align with your multiplayer architecture, or is there a reason to sync the grid directly?

### @game-designer

5. **Priority levels per structure type:** The canon mentions 3 priority levels (critical, important, normal). I've added a 4th (low) for habitation. Is this appropriate, or should habitation be priority 3 with something else at priority 4?

6. **Energy deficit duration before effects:** How many ticks should a structure be unpowered before negative effects apply? Immediate, or a grace period (e.g., 100 ticks / 5 seconds)?

7. **Nexus output values:** The configuration table in Section 4 has placeholder values. Can you provide canonical output values for each nexus type relative to typical structure consumption?

8. **Coverage visualization preview:** When placing conduits, should the coverage preview show:
   - Only the new conduit's radius?
   - The complete updated coverage zone?
   - The delta (what tiles will GAIN coverage)?

### @building-engineer

9. **Structure energy_required source:** Should `energy_required` be a field in `BuildingComponent`, or should EnergySystem look it up from the template system? The latter is more flexible but adds coupling.

10. **Nexus lifecycle:** Should energy nexuses use the same 5-state machine (Materializing, Active, Abandoned, Derelict, Deconstructed) as zone structures? If so, does a Materializing nexus contribute to the pool?

---

## 12. Risks and Concerns

### Performance Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Coverage recalculation too slow at 512x512 | Medium | High | Dirty tracking limits recalculation to changes; BFS is O(conduits), not O(grid) |
| Many producers/consumers per player | Medium | Medium | Pre-sorted entity lists per owner; O(N) iteration is acceptable at N < 50,000 |
| Per-tick consumer iteration | Low | Medium | Entity views are cache-friendly; skip unchanged entities |
| Conduit network complexity | Low | Medium | BFS naturally handles complex networks; visited set prevents revisits |

**Benchmarking Targets:**
- Coverage recalculation: < 10ms for 512x512 map with 5,000 conduits
- Pool calculation: < 1ms for 10,000 consumers
- Full tick: < 2ms for 256x256 map

### Memory Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Coverage grid memory at 512x512 | Low | Low | 256 KB is minimal; well within budget |
| EnergyComponent on all structures | Low | Low | 12 bytes per entity; ~240 KB for 20,000 structures |

### Interface Stability Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| IEnergyProvider changes after Epic 4 stub | Medium | High | Lock interface before Epic 4's 4-020 completes |
| IContaminationSource changes in Epic 10 | Low | Medium | Define minimal interface now; Epic 10 can extend |

**Recommendation:** Finalize IEnergyProvider interface immediately and communicate to Epic 4 team.

### Multiplayer Consistency Risks

| Risk | Description | Mitigation |
|------|-------------|------------|
| Rationing order differs server/client | Clients receive final is_powered state; no client-side rationing |
| Coverage grid desync | Clients reconstruct from synced conduit positions; deterministic algorithm |
| Pool calculation race | Server-authoritative; clients only display received state |

### Design Ambiguity Risks

| Ambiguity | Impact | Recommendation |
|-----------|--------|----------------|
| Partial power (energy_received < required) | Affects rationing, rendering | Start with binary (fully powered or not); partial power is future enhancement |
| Nexus maintenance spending | Affects efficiency curve | Defer to Economy Engineer (Epic 11); efficiency field already supports it |
| Conduit damage from disasters | Affects coverage calculation | EnergyConduitComponent has `is_connected`; disasters can set false to simulate damage |
| Energy trading between overseers | Out of scope for Epic 5 | Trading is abstract bank system (Epic 11), not infrastructure |

---

## Appendix A: Shared Utility System Consideration

Given FluidSystem (Epic 6) follows the same pattern:
- Per-overseer pools
- Producers add to pool
- Consumers draw from pool
- Conduits define coverage

A shared template could reduce code duplication:

```cpp
template<
    typename ProducerComp,  // EnergyProducerComponent or FluidProducerComponent
    typename ConsumerComp,  // EnergyComponent or FluidComponent
    typename ConduitComp    // EnergyConduitComponent or FluidConduitComponent
>
class UtilitySystem : public ISimulatable {
protected:
    CoverageGrid coverage_grid_;
    std::array<PerPlayerPool, MAX_PLAYERS> pools_;
    bool coverage_dirty_[MAX_PLAYERS];

    virtual void update_producer_output(ProducerComp& producer) = 0;
    virtual uint32_t get_consumer_requirement(const ConsumerComp& consumer) const = 0;

    void tick(float delta_time) override {
        recalculate_dirty_coverage();
        update_all_producers(delta_time);
        calculate_pools();
        apply_distribution();
        emit_state_changes();
    }
};

class EnergySystem : public UtilitySystem<
    EnergyProducerComponent,
    EnergyComponent,
    EnergyConduitComponent
> {
    // Energy-specific overrides
};

class FluidSystem : public UtilitySystem<
    FluidProducerComponent,
    FluidComponent,
    FluidConduitComponent
> {
    // Fluid-specific overrides (reservoir storage, pressure)
};
```

**Recommendation for @systems-architect:** Implement EnergySystem first, then factor out shared code when implementing FluidSystem. Premature abstraction risks over-engineering.

---

## Appendix B: Event Definitions

```cpp
// Energy state change for a specific entity
struct EnergyStateChangedEvent {
    EntityID entity;
    PlayerID owner;
    bool was_powered;
    bool is_powered;
};

// Pool transitioned to deficit
struct EnergyDeficitBeganEvent {
    PlayerID owner;
    int32_t deficit_amount;  // How much energy is short
    uint32_t affected_consumers;  // How many consumers unpowered
};

// Pool recovered from deficit
struct EnergyDeficitEndedEvent {
    PlayerID owner;
    int32_t surplus_amount;  // New surplus level
};

// Pool entered collapse state
struct GridCollapseBeganEvent {
    PlayerID owner;
    int32_t deficit_amount;
};

// Pool recovered from collapse
struct GridCollapseEndedEvent {
    PlayerID owner;
};

// Conduit network events
struct ConduitPlacedEvent {
    EntityID conduit;
    PlayerID owner;
    GridPosition position;
};

struct ConduitRemovedEvent {
    EntityID conduit;
    PlayerID owner;
    GridPosition position;
};

// Nexus events
struct NexusPlacedEvent {
    EntityID nexus;
    PlayerID owner;
    NexusType type;
    GridPosition position;
};

struct NexusRemovedEvent {
    EntityID nexus;
    PlayerID owner;
    GridPosition position;
};

struct NexusAgedEvent {
    EntityID nexus;
    PlayerID owner;
    float new_efficiency;  // Efficiency dropped to this level
};
```

---

## Appendix C: Canonical Terminology Reference

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
| Coal plant | Carbon nexus |
| Oil plant | Petrochemical nexus |
| Gas plant | Gaseous nexus |
| Wind turbine | Wind nexus |
| Solar panel | Solar collector |
| Hydroelectric | Hydro nexus |
| Geothermal | Geothermal nexus |
| Nuclear | Nuclear nexus |
| Fusion | Fusion reactor |

---

**Document Status:** Ready for review by Systems Architect and Game Designer.

**Next Steps:**
1. @systems-architect to review coverage approach and shared abstraction question
2. @game-designer to provide nexus output values and priority assignments
3. Convert work items to formal tickets after questions resolved
4. Lock IEnergyProvider interface and communicate to Epic 4 team
