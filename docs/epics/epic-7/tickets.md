# Epic 7: Transportation Network - Tickets

**Epic:** 7 - Transportation Network
**Canon Version:** 0.13.0
**Date:** 2026-01-29
**Status:** PLANNING COMPLETE

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.8.0 | Initial ticket creation |
| 2026-01-29 | canon-verification (v0.8.0 → v0.13.0) | No changes required |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. ITransportProvider interface (ticket E7-016) matches interfaces.yaml with is_road_accessible_at(x, y, max_distance). PathwayGrid + ProximityCache in dense_grid_exception pattern. Tick priorities 45/47 confirmed in systems.yaml.

---

## Overview

Epic 7 implements the transportation infrastructure that enables colony connectivity through **TransportSystem** (pathways, junctions, flow simulation, decay) and **RailSystem** (rail lines, terminals, subterra networks).

**Key Architectural Decisions (from discussion):**

| CCR ID | Decision |
|--------|----------|
| CCR-001 | Network model (not pool model) for transport |
| CCR-002 | Cross-ownership pathway connectivity |
| CCR-003 | ITransportProvider interface replaces Epic 4 stub |
| CCR-004 | Tick priorities: TransportSystem=45, RailSystem=47 |
| CCR-005 | PathwayGrid + ProximityCache in dense_grid_exception |
| CCR-006 | Aggregate flow model (not per-vehicle agents) |
| CCR-007 | 3-tile Manhattan distance rule for pathway access |
| CCR-008 | Flow_blockage is penalty (sector_value), not failure |
| CCR-009 | Rail supplements pathways, doesn't replace |
| CCR-010 | Transport pods deferred to Epic 17+ |

---

## Ticket Summary

| Priority | Count | Description |
|----------|-------|-------------|
| P0 | 22 | Core infrastructure, must have for MVP |
| P1 | 18 | Important features, should have |
| P2 | 8 | Enhancements, later phases |
| **Total** | **48** | |

---

## P0 - Critical (Must Have)

### Infrastructure Layer

#### E7-001: PathwayType and PathwayDirection Enums
**Size:** S | **Component:** TransportSystem

Define pathway type enumeration and direction enumeration per infrastructure engineer design.

```cpp
enum class PathwayType : uint8_t {
    BasicPathway    = 0,   // Standard road, 2-lane equivalent
    TransitCorridor = 1,   // High capacity (highway)
    Pedestrian      = 2,   // Walkways, low capacity
    Bridge          = 3,   // Over water/terrain
    Tunnel          = 4    // Through terrain
};

enum class PathwayDirection : uint8_t {
    Bidirectional = 0,
    OneWayNorth   = 1,
    OneWaySouth   = 2,
    OneWayEast    = 3,
    OneWayWest    = 4
};
```

**Acceptance Criteria:**
- [ ] Enums defined in transport types header
- [ ] Matches alien terminology (pathway, transit_corridor, not road/highway)

---

#### E7-002: RoadComponent Definition (16 bytes)
**Size:** S | **Component:** TransportSystem | **Depends:** E7-001

Define RoadComponent struct for pathway segments per infrastructure engineer spec.

```cpp
struct RoadComponent {
    PathwayType type          = PathwayType::BasicPathway;
    PathwayDirection direction = PathwayDirection::Bidirectional;
    uint16_t base_capacity    = 100;
    uint16_t current_capacity = 100;
    uint8_t health            = 255;   // 0-255, 255 = pristine
    uint8_t decay_rate        = 1;
    uint8_t connection_mask   = 0;     // N(1), S(2), E(4), W(8)
    bool is_junction          = false;
    uint16_t network_id       = 0;
    uint32_t last_maintained_tick = 0;
};
static_assert(sizeof(RoadComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Size assertion passes (16 bytes)
- [ ] Serialization implemented

---

#### E7-003: TrafficComponent Definition (16 bytes)
**Size:** S | **Component:** TransportSystem

Define TrafficComponent for pathway flow state (sparse attachment).

```cpp
struct TrafficComponent {
    uint32_t flow_current     = 0;
    uint32_t flow_previous    = 0;
    uint16_t flow_sources     = 0;
    uint8_t congestion_level  = 0;     // 0-255
    uint8_t flow_blockage_ticks = 0;
    uint8_t contamination_rate = 0;
    uint8_t padding[3];
};
static_assert(sizeof(TrafficComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Size assertion passes (16 bytes)
- [ ] Sparse attachment pattern (only pathways with flow)

---

#### E7-004: Transport Event Definitions
**Size:** S | **Component:** TransportSystem

Define transport system events per infrastructure engineer spec.

Events required:
- `PathwayPlacedEvent { entity, position, type, owner }`
- `PathwayRemovedEvent { entity, position, owner }`
- `PathwayDeterioratedEvent { entity, position, new_health }`
- `PathwayRepairedEvent { entity, position, new_health }`
- `NetworkConnectedEvent { network_id, connected_players[] }`
- `NetworkDisconnectedEvent { old_id, new_id_a, new_id_b }`
- `FlowBlockageBeganEvent { pathway, position, congestion_level }`
- `FlowBlockageEndedEvent { pathway, position }`

**Acceptance Criteria:**
- [ ] All events defined with proper fields
- [ ] Events registered with event system

---

### Spatial Data Layer

#### E7-005: PathwayGrid Dense 2D Array
**Size:** M | **Component:** TransportSystem | **Depends:** E7-002

Implement PathwayGrid following dense_grid_exception pattern.

```cpp
struct PathwayGridCell {
    EntityID entity_id;      // Pathway entity at this cell (0 = empty)
};

class PathwayGrid {
    std::vector<PathwayGridCell> grid_;
    uint32_t width_, height_;
    bool network_dirty_ = true;

public:
    EntityID get_pathway_at(int32_t x, int32_t y) const;
    bool has_pathway(int32_t x, int32_t y) const;
    void set_pathway(int32_t x, int32_t y, EntityID entity);
    void clear_pathway(int32_t x, int32_t y);
    bool is_network_dirty() const;
    void mark_network_clean();
};
```

**Memory:** 4 bytes/cell = 256 KB for 256x256, 1 MB for 512x512

**Acceptance Criteria:**
- [ ] O(1) spatial queries
- [ ] Dirty flag for network rebuild
- [ ] Memory budget within spec

---

#### E7-006: ProximityCache for Distance Queries
**Size:** M | **Component:** TransportSystem | **Depends:** E7-005

Implement ProximityCache for O(1) 3-tile rule queries after rebuild.

```cpp
class ProximityCache {
    std::vector<uint8_t> distance_cache_;  // 1 byte per tile
    bool dirty_ = true;
    uint32_t width_, height_;

public:
    uint8_t get_distance(int32_t x, int32_t y);
    void rebuild_if_dirty(const PathwayGrid& grid);
    void mark_dirty();
};
```

**Memory:** 1 byte/tile = 64 KB for 256x256, 256 KB for 512x512

**Acceptance Criteria:**
- [ ] O(1) distance lookups after rebuild
- [ ] Dirty tracking for efficient updates
- [ ] Manhattan distance (4-directional) per CCR-007

---

#### E7-007: Multi-Source BFS Proximity Rebuild
**Size:** M | **Component:** TransportSystem | **Depends:** E7-006

Implement efficient multi-source BFS for ProximityCache rebuild.

Algorithm:
1. Seed queue with all pathway tiles at distance 0
2. BFS expand to all tiles, tracking minimum distance
3. Cap at 255 (uint8_t max)

**Performance Target:** <20ms rebuild on 256x256

**Acceptance Criteria:**
- [ ] Correct Manhattan distances
- [ ] Performance within budget
- [ ] Cache-friendly memory access

---

### Network Graph Layer

#### E7-008: NetworkGraph Node/Edge Structures
**Size:** M | **Component:** TransportSystem

Define network graph structures for connectivity queries.

```cpp
struct NetworkNode {
    GridPosition position;
    std::vector<uint16_t> neighbor_indices;
    uint16_t network_id;
};

class NetworkGraph {
    std::vector<NetworkNode> nodes_;
    std::unordered_map<GridPosition, uint16_t> position_to_node_;
    uint16_t next_network_id_ = 1;

public:
    void rebuild_from_grid(const PathwayGrid& grid);
    bool is_connected(GridPosition a, GridPosition b) const;
    uint16_t get_network_id(GridPosition pos) const;
};
```

**Acceptance Criteria:**
- [ ] Node storage with neighbor indices
- [ ] Position-to-node lookup
- [ ] Network ID assignment

---

#### E7-009: Graph Rebuild from PathwayGrid
**Size:** L | **Component:** TransportSystem | **Depends:** E7-005, E7-008

Implement full graph rebuild from PathwayGrid.

Algorithm:
1. Scan PathwayGrid for pathway tiles
2. Create nodes for each pathway tile
3. Connect adjacent pathway tiles (cross-ownership per CCR-002)
4. Assign network IDs via connected component BFS

**Performance Target:** <50ms rebuild on 256x256 with 15,000 segments

**Acceptance Criteria:**
- [ ] Cross-ownership connectivity (no owner check)
- [ ] Network ID assigned to connected components
- [ ] Dirty flag cleared after rebuild

---

#### E7-010: Connected Component (network_id) Assignment
**Size:** M | **Component:** TransportSystem | **Depends:** E7-009

Implement connected component labeling with network_id propagation.

Also update RoadComponent.network_id for each pathway entity to enable O(1) connectivity queries.

**Acceptance Criteria:**
- [ ] Unique network_id per connected component
- [ ] RoadComponent.network_id updated
- [ ] O(1) connectivity check via network_id comparison

---

#### E7-011: Connectivity Query (O(1))
**Size:** S | **Component:** TransportSystem | **Depends:** E7-010

Implement fast connectivity query using network_id.

```cpp
bool TransportSystem::is_connected(GridPosition a, GridPosition b) const {
    EntityID ea = pathway_grid_.get_pathway_at(a.x, a.y);
    EntityID eb = pathway_grid_.get_pathway_at(b.x, b.y);
    if (ea == INVALID || eb == INVALID) return false;

    auto& road_a = registry_.get<RoadComponent>(ea);
    auto& road_b = registry_.get<RoadComponent>(eb);
    return road_a.network_id == road_b.network_id && road_a.network_id != 0;
}
```

**Acceptance Criteria:**
- [ ] O(1) query after graph rebuild
- [ ] Handles missing pathways correctly

---

### Traffic Simulation Layer

#### E7-012: Building Traffic Contribution Calculation
**Size:** M | **Component:** TransportSystem

Implement traffic contribution calculation from buildings.

Base values (configurable):
- Habitation: 2 flow/tick per occupant ratio
- Exchange: 5 flow/tick per occupant ratio
- Fabrication: 3 flow/tick per occupant ratio

Scale by building level (level 2 = 2x, level 3 = 3x).

**Acceptance Criteria:**
- [ ] Query BuildingComponent for zone type/occupancy
- [ ] Configurable base values
- [ ] Level scaling applied

---

#### E7-013: Flow Distribution from Buildings to Pathways
**Size:** L | **Component:** TransportSystem | **Depends:** E7-005, E7-012

Implement flow distribution from buildings to nearest pathways.

Algorithm:
1. For each active building, calculate traffic contribution
2. Find nearest pathway within 3 tiles
3. Distribute flow to pathway and neighbors

**Acceptance Criteria:**
- [ ] Buildings without pathway access skipped
- [ ] Flow distributed to connected pathways
- [ ] Crosses ownership boundaries (CCR-002)

---

#### E7-014: Flow Propagation (Diffusion Model)
**Size:** L | **Component:** TransportSystem | **Depends:** E7-013

Implement flow propagation via diffusion model per CCR-006.

Algorithm:
- Flow spreads 20% per tick to neighbors
- Distribution proportional to neighbor capacity
- Single-pass propagation per tick

**Acceptance Criteria:**
- [ ] Flow spreads along connected pathways
- [ ] Capacity-weighted distribution
- [ ] No per-vehicle simulation

---

#### E7-015: Congestion Calculation
**Size:** M | **Component:** TransportSystem | **Depends:** E7-014

Implement per-segment congestion calculation.

Thresholds (0-255 scale):
- 0-50: Free flow
- 51-100: Light traffic (-5% sector_value)
- 101-150: Moderate traffic (-10% sector_value)
- 151-200: Heavy traffic (-15% sector_value, contamination)
- 201-255: Flow_blockage (severe penalties)

**Acceptance Criteria:**
- [ ] Congestion level 0-255 per segment
- [ ] flow_blockage_ticks counter for sustained congestion
- [ ] Contamination rate calculated for ContaminationSystem

---

### ITransportProvider Interface

#### E7-016: ITransportProvider Interface Definition
**Size:** S | **Component:** TransportSystem

Define ITransportProvider interface per CCR-003.

```cpp
class ITransportProvider {
public:
    virtual bool is_road_accessible(EntityID entity) const = 0;
    virtual bool is_road_accessible_at(int32_t x, int32_t y, uint8_t max_distance = 3) const = 0;
    virtual uint8_t get_nearest_road_distance(int32_t x, int32_t y) const = 0;
    virtual bool is_connected_to_network(int32_t x, int32_t y) const = 0;
    virtual bool are_connected(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const = 0;
    virtual float get_congestion_at(int32_t x, int32_t y) const = 0;
    virtual uint32_t get_traffic_volume_at(int32_t x, int32_t y) const = 0;
    virtual uint32_t get_network_id_at(int32_t x, int32_t y) const = 0;
};
```

**Acceptance Criteria:**
- [ ] Interface defined in interfaces header
- [ ] Added to canon interfaces.yaml

---

#### E7-017: is_road_accessible Implementation
**Size:** S | **Component:** TransportSystem | **Depends:** E7-006, E7-016

Implement is_road_accessible using ProximityCache.

```cpp
bool is_road_accessible_at(int32_t x, int32_t y, uint8_t max_distance) const {
    return proximity_cache_.get_distance(x, y) <= max_distance;
}
```

**Acceptance Criteria:**
- [ ] O(1) query via cache
- [ ] 3-tile default per canon

---

#### E7-018: get_nearest_road_distance Implementation
**Size:** S | **Component:** TransportSystem | **Depends:** E7-006, E7-016

Implement distance query using ProximityCache.

**Acceptance Criteria:**
- [ ] O(1) query via cache
- [ ] Returns 255 if no pathway in range

---

#### E7-019: Replace BuildingSystem Stub with Real Implementation
**Size:** M | **Component:** TransportSystem, BuildingSystem | **Depends:** E7-017, E7-018

Replace Epic 4 stub (StubTransportProvider) with real TransportSystem implementation.

Transition strategy (per systems architect):
- 500 tick grace period for existing buildings
- Visual warning during grace period
- Buildings without access enter Degraded state after grace

**Acceptance Criteria:**
- [ ] Stub replaced via dependency injection
- [ ] Grace period implemented
- [ ] Visual warning for buildings losing access

---

### Cross-Ownership Layer

#### E7-020: Cross-Ownership Connectivity in NetworkGraph
**Size:** M | **Component:** TransportSystem | **Depends:** E7-009

Ensure NetworkGraph connects pathways regardless of ownership per CCR-002.

Key implementation points:
- No owner check in adjacency building
- Single unified network for all connected pathways
- Each segment still tracks owner via OwnershipComponent

**Acceptance Criteria:**
- [ ] Player A's pathway connects to Player B's if adjacent
- [ ] Single network ID for connected cross-ownership paths
- [ ] Ownership preserved for maintenance/rendering

---

#### E7-021: Per-Owner Maintenance Cost Calculation
**Size:** S | **Component:** TransportSystem | **Depends:** E7-002

Implement per-owner maintenance cost query for EconomySystem.

```cpp
uint32_t get_player_maintenance_cost(PlayerID owner) const {
    uint32_t total = 0;
    for (auto& [entity, road, ownership] : registry_.view<RoadComponent, OwnershipComponent>()) {
        if (ownership.owner != owner) continue;
        uint32_t missing_health = 255 - road.health;
        total += (missing_health * cost_per_health(road.type)) / 255;
    }
    return total;
}
```

**Acceptance Criteria:**
- [ ] Only counts owner's pathways
- [ ] Cost scales with damage

---

### TransportSystem Integration

#### E7-022: TransportSystem Class (ISimulatable, Priority 45)
**Size:** L | **Component:** TransportSystem | **Depends:** All E7-001 through E7-021

Implement TransportSystem as ISimulatable with tick priority 45 per CCR-004.

Tick phases:
1. Rebuild network if dirty
2. Rebuild proximity cache if dirty
3. Calculate building traffic contributions
4. Distribute and propagate flow
5. Calculate congestion
6. Update decay (every 100 ticks)

**Acceptance Criteria:**
- [ ] Registered with simulation at priority 45
- [ ] All tick phases implemented
- [ ] Performance within 3ms on 256x256 steady-state

---

## P1 - Important (Should Have)

### Pathfinding Layer

#### E7-023: A* Pathfinding Implementation
**Size:** L | **Component:** TransportSystem | **Depends:** E7-009

Implement A* pathfinding for connectivity queries and cosmetic entities.

Uses Manhattan distance heuristic. Edge cost considers pathway type, congestion, and decay.

**Performance Target:** <5ms per path on 512x512

**Acceptance Criteria:**
- [ ] A* with Manhattan heuristic
- [ ] Edge cost function considering type/congestion/decay
- [ ] Early exit if not connected (via network_id)

---

#### E7-024: Edge Cost Function
**Size:** M | **Component:** TransportSystem | **Depends:** E7-023

Implement edge cost considering pathway characteristics.

| Pathway Type | Base Cost |
|--------------|-----------|
| BasicPathway | 15 |
| StandardPathway | 10 |
| TransitCorridor | 5 |
| Pedestrian | 20 |
| Bridge/Tunnel | 10 |

Congestion penalty: +0-10 based on congestion_level
Decay penalty: +0-5 based on health

**Acceptance Criteria:**
- [ ] Type-based costs
- [ ] Congestion penalty applied
- [ ] Decay penalty applied

---

### Decay Layer

#### E7-025: Pathway Decay System
**Size:** M | **Component:** TransportSystem | **Depends:** E7-002

Implement pathway decay over time with traffic multiplier.

Decay formula (every 100 ticks):
- Base decay: 0.1 health per 5 seconds
- Traffic multiplier: 1.0 + 2.0 * (flow/capacity) = up to 3x

Timeline per game designer:
- Basic pathway: ~15 minutes to Poor
- Standard pathway: ~20 minutes to Poor
- Transit corridor: ~30 minutes to Poor

**Acceptance Criteria:**
- [ ] Decay runs every 100 ticks
- [ ] Traffic multiplier applied
- [ ] PathwayDeterioratedEvent emitted at thresholds

---

#### E7-026: Capacity Degradation from Health
**Size:** S | **Component:** TransportSystem | **Depends:** E7-025

Update current_capacity based on health degradation.

```cpp
road.current_capacity = (road.base_capacity * road.health) / 255;
```

**Acceptance Criteria:**
- [ ] Capacity scales linearly with health
- [ ] Zero capacity at zero health

---

#### E7-027: Maintenance Application API
**Size:** S | **Component:** TransportSystem | **Depends:** E7-025

Implement maintenance API for EconomySystem integration.

```cpp
void apply_maintenance(EntityID pathway, uint8_t health_restored);
```

**Acceptance Criteria:**
- [ ] Health restoration capped at 255
- [ ] Capacity recalculated
- [ ] last_maintained_tick updated

---

### Rendering Integration

#### E7-028: Boundary Flags for Rendering
**Size:** S | **Component:** TransportSystem | **Depends:** E7-020

Generate boundary flags for ownership boundary rendering.

```cpp
struct PathwayRenderData {
    GridPosition position;
    PathwayType type;
    uint8_t health;
    uint8_t congestion_level;
    PlayerID owner;
    uint8_t boundary_flags;  // N(1), S(2), E(4), W(8)
};
```

**Acceptance Criteria:**
- [ ] Boundary flags computed for each edge
- [ ] Flags indicate ownership change direction

---

#### E7-029: Contamination Rate for ContaminationSystem
**Size:** M | **Component:** TransportSystem | **Depends:** E7-015

Calculate contamination_rate in TrafficComponent for ContaminationSystem queries.

Only congested pathways (level > 128) contribute contamination.

```cpp
if (congestion_level > 128) {
    contamination_rate = (congestion_level - 128) / 8;  // 0-15
}
```

ContaminationSystem queries via get_traffic_volume_at().

**Acceptance Criteria:**
- [ ] Contamination rate in TrafficComponent
- [ ] Only congested pathways contribute
- [ ] Queryable via ITransportProvider

---

### Rail System Layer

#### E7-030: RailType Enum and RailComponent Definition (12 bytes)
**Size:** S | **Component:** RailSystem

Define rail type enumeration and RailComponent struct.

```cpp
enum class RailType : uint8_t {
    SurfaceRail = 0,
    ElevatedRail = 1,
    SubterraRail = 2
};

struct RailComponent {
    RailType type             = RailType::SurfaceRail;
    uint16_t capacity         = 500;   // Beings per cycle
    uint16_t current_load     = 0;
    uint8_t connection_mask   = 0;
    bool is_terminal_adjacent = false;
    bool is_powered           = false;
    bool is_active            = false;
    uint16_t rail_network_id  = 0;
    uint8_t health            = 255;
    uint8_t padding;
};
static_assert(sizeof(RailComponent) == 12);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Size assertion passes

---

#### E7-031: TerminalType Enum and TerminalComponent Definition (10 bytes)
**Size:** S | **Component:** RailSystem

Define terminal types and TerminalComponent.

```cpp
enum class TerminalType : uint8_t {
    SurfaceStation   = 0,
    SubterraStation  = 1,
    IntermodalHub    = 2
};

struct TerminalComponent {
    TerminalType type      = TerminalType::SurfaceStation;
    uint16_t capacity      = 200;
    uint16_t current_usage = 0;
    uint8_t coverage_radius = 8;
    bool is_powered        = false;
    bool is_active         = false;
    uint8_t padding[2];
};
static_assert(sizeof(TerminalComponent) == 10);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Size assertion passes

---

#### E7-032: RailSystem Class (ISimulatable, Priority 47)
**Size:** M | **Component:** RailSystem | **Depends:** E7-030, E7-031

Implement RailSystem as ISimulatable with tick priority 47 per CCR-004.

Tick phases:
1. Update power states from IEnergyProvider
2. Update active states (powered + terminal connection)
3. Calculate terminal coverage effects

**Acceptance Criteria:**
- [ ] Registered at priority 47
- [ ] Runs after TransportSystem (45)
- [ ] Power dependency via IEnergyProvider

---

#### E7-033: Rail Power State Update
**Size:** M | **Component:** RailSystem | **Depends:** E7-032

Implement power dependency for rail segments and terminals.

Rails require power from EnergySystem to operate (per canon).

**Acceptance Criteria:**
- [ ] Query IEnergyProvider.is_powered()
- [ ] is_active requires both power and terminal connection

---

#### E7-034: Terminal Placement and Activation
**Size:** M | **Component:** RailSystem | **Depends:** E7-031

Implement rail terminal placement with coverage radius.

Terminals require:
- Power from EnergySystem
- Adjacent rail tracks
- Pathway access (query ITransportProvider)

**Acceptance Criteria:**
- [ ] Placement validation
- [ ] Power and rail connectivity check
- [ ] Pathway access required

---

#### E7-035: Rail Coverage Model (Terminal Radius)
**Size:** M | **Component:** RailSystem | **Depends:** E7-034

Implement terminal coverage radius for traffic reduction.

Buildings within coverage_radius of active terminal have reduced traffic contribution.

Rail cycle = 100 ticks = 5 seconds (per discussion Q015).

**Acceptance Criteria:**
- [ ] Coverage radius query
- [ ] Traffic reduction percentage

---

### Network Sync Layer

#### E7-036: RoadComponent Serialization
**Size:** M | **Component:** TransportSystem | **Depends:** E7-002

Implement ISerializable for RoadComponent.

**Acceptance Criteria:**
- [ ] Serialize all fields
- [ ] Delta sync on placement/demolition

---

#### E7-037: TrafficComponent Serialization
**Size:** S | **Component:** TransportSystem | **Depends:** E7-003

Implement ISerializable for TrafficComponent.

Sync frequency: Every 10-20 ticks (not every tick).

**Acceptance Criteria:**
- [ ] Periodic sync (configurable frequency)
- [ ] Interpolation-friendly fields

---

#### E7-038: Network Messages for Pathway Operations
**Size:** M | **Component:** TransportSystem | **Depends:** E7-022

Implement network messages for pathway placement/demolition.

Messages:
- PathwayPlaceRequest
- PathwayPlaceResponse
- PathwayDemolishRequest
- PathwayDemolishResponse

**Acceptance Criteria:**
- [ ] Server validates ownership and terrain
- [ ] Client receives confirmation
- [ ] PathwayGrid synced

---

#### E7-039: Rail Network Messages
**Size:** M | **Component:** RailSystem | **Depends:** E7-032

Implement network messages for rail operations.

Messages:
- RailPlaceRequest/Response
- RailDemolishRequest/Response
- TerminalPlaceRequest/Response
- TerminalDemolishRequest/Response

**Acceptance Criteria:**
- [ ] Server authority for placement
- [ ] Energy requirement validated server-side

---

### Content Layer

#### E7-040: Pathway Type Base Stats Definition
**Size:** S | **Component:** TransportSystem | **Owner:** @game-designer

Define configurable base stats for each pathway type.

| Type | Base Capacity | Build Cost | Decay Rate |
|------|---------------|------------|------------|
| BasicPathway | 100 | Low | 1.0x |
| StandardPathway | 200 | Medium | 0.75x |
| TransitCorridor | 500 | High (4x) | 0.5x |
| Bridge | 200 | Very High | 0.75x |
| Tunnel | 200 | Very High | 0.6x |

**Acceptance Criteria:**
- [ ] Stats in configuration file
- [ ] Easily tunable

---

## P2 - Enhancement (Later Phases)

#### E7-041: PathCache for Frequently-Queried Routes
**Size:** M | **Component:** TransportSystem | **Depends:** E7-023

Implement path caching with invalidation.

Cache invalidates when network changes. Max age: 100 ticks.

**Acceptance Criteria:**
- [ ] Cache hit for repeated queries
- [ ] Invalidation on network change

---

#### E7-042: SubterraLayerManager
**Size:** M | **Component:** RailSystem | **Depends:** E7-030

Implement subterra (underground) layer management.

Single-depth for MVP per discussion Q016. Multi-depth deferred.

```cpp
class SubterraLayerManager {
    std::vector<EntityID> subterra_grid_;
public:
    EntityID get_subterra_at(int32_t x, int32_t y) const;
    bool can_build_subterra_at(int32_t x, int32_t y, const ITerrainQueryable& terrain) const;
};
```

**Acceptance Criteria:**
- [ ] Separate grid for subterra
- [ ] Placement validation (no water, elevation check)

---

#### E7-043: SubterraComponent Definition (8 bytes)
**Size:** S | **Component:** RailSystem

Define SubterraComponent for underground rail specifics.

```cpp
struct SubterraComponent {
    uint8_t depth_level       = 1;     // 1 for MVP
    bool is_excavated         = true;
    uint8_t ventilation_radius = 2;
    bool has_surface_access   = false;
    uint8_t padding[4];
};
static_assert(sizeof(SubterraComponent) == 8);
```

**Acceptance Criteria:**
- [ ] Component registered
- [ ] depth_level = 1 for MVP

---

#### E7-044: Subterra Placement Rules
**Size:** M | **Component:** RailSystem | **Depends:** E7-042, E7-043

Implement subterra placement validation.

Rules:
- Cannot build under water
- Cannot build under low elevation
- Must connect to subterra terminals

**Acceptance Criteria:**
- [ ] Terrain queries for validation
- [ ] Terminal connection required

---

#### E7-045: Rail Traffic Reduction Calculation
**Size:** M | **Component:** RailSystem | **Depends:** E7-035

Implement traffic reduction for buildings in terminal coverage.

Reduction formula: 50% at terminal, linear falloff to 0% at radius edge.

**Acceptance Criteria:**
- [ ] Distance-based falloff
- [ ] Applied to building traffic contribution

---

#### E7-046: Decay Rates and Thresholds Configuration
**Size:** S | **Component:** TransportSystem | **Owner:** @game-designer

Define decay thresholds and visual state mapping.

| Health Range | Visual State |
|--------------|--------------|
| 200-255 | Pristine |
| 150-199 | Good |
| 100-149 | Worn |
| 50-99 | Poor |
| 0-49 | Crumbling |

**Acceptance Criteria:**
- [ ] Thresholds configurable
- [ ] Event emission at state transitions

---

#### E7-047: Rail Capacity and Costs Configuration
**Size:** S | **Component:** RailSystem | **Owner:** @game-designer

Define rail system configuration values.

| Type | Capacity | Build Cost | Power Required |
|------|----------|------------|----------------|
| SurfaceRail | 500/cycle | High | Yes |
| ElevatedRail | 500/cycle | Very High | Yes |
| SubterraRail | 750/cycle | Maximum | Yes |
| SurfaceStation | 200/cycle | High | Yes |
| SubterraStation | 400/cycle | Very High | Yes |

**Acceptance Criteria:**
- [ ] Values in configuration file
- [ ] Power costs specified

---

#### E7-048: Balance Traffic Contribution Values
**Size:** S | **Component:** TransportSystem | **Owner:** @game-designer

Tune traffic contribution based on playtesting.

Starting values:
- Habitation: 2 flow/tick
- Exchange: 5 flow/tick
- Fabrication: 3 flow/tick

Level multiplier: level * base

**Acceptance Criteria:**
- [ ] Values tunable without code change
- [ ] Playtesting feedback incorporated

---

## Canon Updates Required

The following canon updates must be applied in Phase 6:

### interfaces.yaml

1. **Add ITransportProvider interface** with all methods from E7-016
2. **Update ISimulatable priorities:**
   - TransportSystem: priority 45
   - RailSystem: priority 47

### patterns.yaml

Add to `dense_grid_exception.applies_to`:
- "PathwayGrid (Epic 7): 4 bytes/tile EntityID array for pathway spatial queries"
- "ProximityCache (Epic 7): 1 byte/tile distance-to-nearest-pathway cache"

### systems.yaml

Add `tick_priority` to TransportSystem and RailSystem entries:
- TransportSystem: tick_priority: 45
- RailSystem: tick_priority: 47

---

## Dependencies

### Incoming (Systems We Depend On)

| System | Interface | Usage |
|--------|-----------|-------|
| TerrainSystem (Epic 3) | ITerrainQueryable | Placement validation |
| BuildingSystem (Epic 4) | BuildingComponent | Traffic contribution |
| EnergySystem (Epic 5) | IEnergyProvider | Rail power state |
| SyncSystem (Epic 1) | ISerializable | Network sync |

### Outgoing (Systems That Depend On Us)

| System | Interface | Usage |
|--------|-----------|-------|
| BuildingSystem (Epic 4) | ITransportProvider | 3-tile rule, replaces stub |
| ContaminationSystem (Epic 10) | ITransportProvider | Traffic pollution |
| LandValueSystem (Epic 10) | ITransportProvider | Congestion effects |
| ServicesSystem (Epic 9) | NetworkGraph | Emergency routing |
| RenderingSystem (Epic 2) | PathwayRenderData | Pathway visuals |

---

## Ticket ID Reference

| ID | Title | Priority |
|----|-------|----------|
| E7-001 | PathwayType and PathwayDirection Enums | P0 |
| E7-002 | RoadComponent Definition | P0 |
| E7-003 | TrafficComponent Definition | P0 |
| E7-004 | Transport Event Definitions | P0 |
| E7-005 | PathwayGrid Dense 2D Array | P0 |
| E7-006 | ProximityCache for Distance Queries | P0 |
| E7-007 | Multi-Source BFS Proximity Rebuild | P0 |
| E7-008 | NetworkGraph Node/Edge Structures | P0 |
| E7-009 | Graph Rebuild from PathwayGrid | P0 |
| E7-010 | Connected Component Assignment | P0 |
| E7-011 | Connectivity Query (O(1)) | P0 |
| E7-012 | Building Traffic Contribution | P0 |
| E7-013 | Flow Distribution to Pathways | P0 |
| E7-014 | Flow Propagation (Diffusion) | P0 |
| E7-015 | Congestion Calculation | P0 |
| E7-016 | ITransportProvider Interface | P0 |
| E7-017 | is_road_accessible Implementation | P0 |
| E7-018 | get_nearest_road_distance Implementation | P0 |
| E7-019 | Replace BuildingSystem Stub | P0 |
| E7-020 | Cross-Ownership Connectivity | P0 |
| E7-021 | Per-Owner Maintenance Cost | P0 |
| E7-022 | TransportSystem Class | P0 |
| E7-023 | A* Pathfinding | P1 |
| E7-024 | Edge Cost Function | P1 |
| E7-025 | Pathway Decay System | P1 |
| E7-026 | Capacity Degradation | P1 |
| E7-027 | Maintenance Application API | P1 |
| E7-028 | Boundary Flags for Rendering | P1 |
| E7-029 | Contamination Rate | P1 |
| E7-030 | RailComponent Definition | P1 |
| E7-031 | TerminalComponent Definition | P1 |
| E7-032 | RailSystem Class | P1 |
| E7-033 | Rail Power State Update | P1 |
| E7-034 | Terminal Placement | P1 |
| E7-035 | Rail Coverage Model | P1 |
| E7-036 | RoadComponent Serialization | P1 |
| E7-037 | TrafficComponent Serialization | P1 |
| E7-038 | Pathway Network Messages | P1 |
| E7-039 | Rail Network Messages | P1 |
| E7-040 | Pathway Type Base Stats | P1 |
| E7-041 | PathCache | P2 |
| E7-042 | SubterraLayerManager | P2 |
| E7-043 | SubterraComponent Definition | P2 |
| E7-044 | Subterra Placement Rules | P2 |
| E7-045 | Rail Traffic Reduction | P2 |
| E7-046 | Decay Rates Configuration | P2 |
| E7-047 | Rail Capacity Configuration | P2 |
| E7-048 | Balance Traffic Values | P2 |
