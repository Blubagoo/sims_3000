# Infrastructure Engineer Analysis: Epic 7 - Transportation Network

**Agent:** Infrastructure Engineer
**Epic:** 7 - Transportation Network (TransportSystem + RailSystem)
**Canon Version:** 0.8.0
**Date:** 2026-01-29

---

## Executive Summary

Epic 7 implements the transportation infrastructure that enables colony connectivity. This analysis covers the TransportSystem (pathways, junctions, flow simulation, decay) and RailSystem (rail lines, terminals, subterra networks). Transportation is unique in the project: pathways connect across ownership boundaries (unlike energy/fluid conduits), and the 3-tile proximity rule gates building development (ITransportConnectable interface). This document details component designs, network graph structures, pathfinding algorithms, flow simulation models, and cross-ownership connectivity patterns.

---

## Table of Contents

1. [Component Design](#1-component-design)
2. [Network Graph Structure](#2-network-graph-structure)
3. [Pathfinding](#3-pathfinding)
4. [Traffic Simulation Model](#4-traffic-simulation-model)
5. [Road Decay Mechanics](#5-road-decay-mechanics)
6. [Rail System Details](#6-rail-system-details)
7. [Proximity Query System](#7-proximity-query-system)
8. [Cross-Ownership Connectivity](#8-cross-ownership-connectivity)
9. [Performance Considerations](#9-performance-considerations)
10. [Key Work Items](#10-key-work-items)
11. [Questions for Other Agents](#11-questions-for-other-agents)
12. [Risks & Concerns](#12-risks--concerns)
13. [Dependencies](#13-dependencies)

---

## 1. Component Design

### 1.1 RoadComponent (Pathway Segment)

Attached to every pathway segment entity. Stores per-segment state including connectivity, decay, and type.

```cpp
enum class PathwayType : uint8_t {
    BasicPathway    = 0,   // Standard road, 2-lane equivalent
    TransitCorridor = 1,   // Canon: highway -> transit_corridor, high capacity
    Pedestrian      = 2,   // Walkways, low capacity, no vehicles
    Bridge          = 3,   // Over water/terrain, higher build cost
    Tunnel          = 4    // Through terrain, highest build cost
};

enum class PathwayDirection : uint8_t {
    Bidirectional = 0,  // Flow in both directions (default)
    OneWayNorth   = 1,  // Flow toward decreasing Y
    OneWaySouth   = 2,  // Flow toward increasing Y
    OneWayEast    = 3,  // Flow toward increasing X
    OneWayWest    = 4   // Flow toward decreasing X
};

struct RoadComponent {
    // Identity
    PathwayType type          = PathwayType::BasicPathway;
    PathwayDirection direction = PathwayDirection::Bidirectional;

    // Capacity
    uint16_t base_capacity    = 100;   // Max flow units per tick at full health
    uint16_t current_capacity = 100;   // Reduced by decay

    // Decay state
    uint8_t health            = 255;   // 0-255, 255 = pristine
    uint8_t decay_rate        = 1;     // Base decay per maintenance tick

    // Connectivity
    uint8_t connection_mask   = 0;     // Bit flags: N(1), S(2), E(4), W(8) neighbors
    bool is_junction          = false; // True if 3+ connections (intersection)

    // Network
    uint16_t network_id       = 0;     // Connected network identifier (for connectivity queries)

    // Maintenance
    uint32_t last_maintained_tick = 0; // Tick when last maintained
};
static_assert(sizeof(RoadComponent) == 16, "RoadComponent must be 16 bytes");
```

**Design Rationale:**

- **16 bytes**: Compact for dense pathway networks (thousands of segments)
- **connection_mask**: Bit flags enable O(1) neighbor lookup during graph traversal
- **is_junction**: Pre-computed flag avoids counting connections each tick
- **network_id**: Segments in the same connected network share an ID, enabling fast "is connected" queries
- **health (0-255)**: Granular decay state; capacity scales linearly with health

### 1.2 TrafficComponent (Flow State)

Attached to pathway segments that have active flow. This is a secondary component - not all pathways have it, only those with current traffic.

```cpp
struct TrafficComponent {
    // Current flow
    uint32_t flow_current     = 0;     // Flow units this tick
    uint32_t flow_previous    = 0;     // Flow units last tick (for smoothing)

    // Aggregated sources
    uint16_t flow_sources     = 0;     // Number of buildings contributing

    // Congestion state
    uint8_t congestion_level  = 0;     // 0-255: 0=free flow, 255=gridlock
    uint8_t flow_blockage_ticks = 0;   // Consecutive ticks at high congestion

    // Contamination contribution
    uint8_t contamination_rate = 0;    // Contamination per tick when congested
    uint8_t padding[3];
};
static_assert(sizeof(TrafficComponent) == 16, "TrafficComponent must be 16 bytes");
```

**Design Rationale:**

- **Separate from RoadComponent**: Not all pathways have traffic every tick; sparse attachment saves memory
- **flow_current/previous**: Two-frame tracking enables smoothed display and prevents flicker
- **congestion_level**: 0-255 scale maps to visual feedback (color gradient on overlay)
- **contamination_rate**: Per IContaminationSource interface - high-traffic pathways pollute

**Congestion Calculation:**

```cpp
uint8_t calculate_congestion(uint32_t flow, uint16_t capacity) {
    if (capacity == 0) return 255;  // Broken road = gridlock
    float ratio = static_cast<float>(flow) / capacity;
    if (ratio <= 0.5f) return 0;                              // Free flow
    if (ratio <= 0.75f) return static_cast<uint8_t>((ratio - 0.5f) * 4 * 85);   // 0-85
    if (ratio <= 1.0f) return static_cast<uint8_t>(85 + (ratio - 0.75f) * 4 * 85); // 85-170
    return static_cast<uint8_t>(std::min(255.0f, 170 + (ratio - 1.0f) * 170));  // 170-255
}
```

### 1.3 RailComponent (Rail Segment)

Attached to rail track segments. Rail differs from pathways: requires power, higher capacity, fixed routes.

```cpp
enum class RailType : uint8_t {
    SurfaceRail = 0,   // Standard above-ground rail
    ElevatedRail = 1,  // Raised rail (crosses over pathways)
    SubterraRail = 2   // Underground (see SubterraComponent)
};

struct RailComponent {
    // Identity
    RailType type             = RailType::SurfaceRail;

    // Capacity (beings per cycle, not flow units)
    uint16_t capacity         = 500;   // Beings transportable per cycle
    uint16_t current_load     = 0;     // Beings currently using

    // Connectivity
    uint8_t connection_mask   = 0;     // Bit flags: N, S, E, W
    bool is_terminal_adjacent = false; // True if next to a terminal

    // Power state
    bool is_powered           = false; // Must have power to operate
    bool is_active            = false; // Powered AND has terminal access

    // Network
    uint16_t rail_network_id  = 0;     // Rail networks are separate from road networks

    // Maintenance
    uint8_t health            = 255;
    uint8_t padding;
};
static_assert(sizeof(RailComponent) == 12, "RailComponent must be 12 bytes");
```

**Key Differences from RoadComponent:**

- **Capacity in beings**: Rail transports beings directly, not abstract flow units
- **Power dependency**: Rails require energy (IEnergyProvider integration)
- **Terminal adjacency**: Tracks are only useful if connected to terminals
- **Separate network_id space**: Rail and road networks are independent

### 1.4 SubterraComponent (Subway Specifics)

Additional component for subterra (subway) rail segments. Attached alongside RailComponent when `type == SubterraRail`.

```cpp
struct SubterraComponent {
    // Depth level (for multi-level subway systems)
    uint8_t depth_level       = 1;     // 1 = shallow, 2 = deep, 3 = very deep

    // Construction state
    bool is_excavated         = true;  // False during construction (tunnel boring)

    // Ventilation (affects nearby surface tiles)
    uint8_t ventilation_radius = 2;    // Surface tiles affected by vent shafts

    // Intersection with surface
    bool has_surface_access   = false; // True if connected to subterra terminal

    uint8_t padding[4];
};
static_assert(sizeof(SubterraComponent) == 8, "SubterraComponent must be 8 bytes");
```

**Subterra Layer Management:**

- Subterra exists on a separate "layer" from surface infrastructure
- Surface and subterra can overlap spatially but don't interact physically
- Subterra terminals are the interface points between surface and underground
- Depth levels allow future expansion for multi-level systems

### 1.5 Component Memory Budget

| Component | Size | Typical Count (256x256) | Memory |
|-----------|------|-------------------------|--------|
| RoadComponent | 16 bytes | ~5,000-15,000 segments | 80-240 KB |
| TrafficComponent | 16 bytes | ~3,000-8,000 (sparse) | 48-128 KB |
| RailComponent | 12 bytes | ~500-2,000 segments | 6-24 KB |
| SubterraComponent | 8 bytes | ~200-500 segments | 1.6-4 KB |
| PathwayGrid (dense) | 4 bytes/cell | 65,536 cells | 256 KB |
| **Total** | | | **~400-650 KB** |

---

## 2. Network Graph Structure

### 2.1 Graph vs Dense Grid Trade-offs

Transportation networks present a unique challenge: they are **sparse** (not every tile has a pathway) but require **connectivity queries** (is building A reachable from building B?).

**Option A: Pure Graph Representation**
```
Nodes = Junctions (3+ connections) + Endpoints
Edges = Pathway segments between nodes
```
- Pros: Memory efficient for sparse networks, natural for pathfinding
- Cons: Complex updates on pathway placement/removal, junction detection overhead

**Option B: Dense PathwayGrid + Overlay Graph**
```
PathwayGrid: EntityID per tile (0 = no pathway)
NetworkGraph: Lazy-built from grid when needed
```
- Pros: O(1) spatial queries, simple placement logic, follows TerrainGrid pattern
- Cons: Memory overhead for empty tiles, graph must sync with grid

**Recommendation: Hybrid Approach (Option B)**

Per the `dense_grid_exception` pattern established in Epics 3-6, use a dense grid for spatial queries combined with a cached graph for pathfinding.

### 2.2 PathwayGrid Implementation

```cpp
struct PathwayGridCell {
    EntityID entity_id;      // Pathway entity at this cell (0 = empty)
};

class PathwayGrid {
private:
    std::vector<PathwayGridCell> grid_;  // Dense 2D array
    uint32_t width_;
    uint32_t height_;
    bool network_dirty_ = true;          // True when graph needs rebuild

public:
    // O(1) spatial queries
    EntityID get_pathway_at(int32_t x, int32_t y) const;
    bool has_pathway(int32_t x, int32_t y) const;

    // Mutations (mark network dirty)
    void set_pathway(int32_t x, int32_t y, EntityID entity);
    void clear_pathway(int32_t x, int32_t y);

    // Distance queries (for ITransportConnectable)
    uint8_t get_nearest_pathway_distance(int32_t x, int32_t y) const;

    // Network state
    bool is_network_dirty() const { return network_dirty_; }
    void mark_network_clean() { network_dirty_ = false; }
};
```

**Memory: 4 bytes/cell = 256 KB for 256x256, 1 MB for 512x512**

### 2.3 NetworkGraph (Lazy-Built)

The graph is rebuilt when pathways change and a connectivity/pathfinding query is needed.

```cpp
struct NetworkNode {
    GridPosition position;
    std::vector<uint16_t> neighbor_indices;  // Indices into nodes array
    uint16_t network_id;                     // Connected component ID
};

class NetworkGraph {
private:
    std::vector<NetworkNode> nodes_;
    std::unordered_map<GridPosition, uint16_t> position_to_node_;
    uint16_t next_network_id_ = 1;

public:
    // Build from PathwayGrid
    void rebuild_from_grid(const PathwayGrid& grid);

    // Connectivity queries
    bool is_connected(GridPosition a, GridPosition b) const;
    uint16_t get_network_id(GridPosition pos) const;

    // Pathfinding
    std::vector<GridPosition> find_path(GridPosition start, GridPosition goal) const;
};
```

**Graph Rebuild Strategy:**

1. **Incremental vs Full**: For MVP, use full rebuild on any change. Incremental updates are complex and can be optimized later if profiling shows need.
2. **Rebuild Trigger**: When `network_dirty_` is true and a query is made.
3. **Rebuild Frequency**: Typically once per tick at most (pathways don't change every tick).

### 2.4 Network ID Assignment (Connected Components)

```cpp
void NetworkGraph::assign_network_ids() {
    // BFS/DFS to find connected components
    std::vector<bool> visited(nodes_.size(), false);

    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (visited[i]) continue;

        // BFS from this node
        std::queue<uint16_t> frontier;
        frontier.push(static_cast<uint16_t>(i));

        while (!frontier.empty()) {
            uint16_t current = frontier.front();
            frontier.pop();

            if (visited[current]) continue;
            visited[current] = true;
            nodes_[current].network_id = next_network_id_;

            // Also update the RoadComponent.network_id for O(1) queries
            update_road_component_network_id(nodes_[current].position, next_network_id_);

            for (uint16_t neighbor : nodes_[current].neighbor_indices) {
                if (!visited[neighbor]) {
                    frontier.push(neighbor);
                }
            }
        }

        ++next_network_id_;
    }
}
```

**Connectivity Query (O(1) after rebuild):**

```cpp
bool TransportSystem::is_connected(GridPosition a, GridPosition b) const {
    EntityID ea = pathway_grid_.get_pathway_at(a.x, a.y);
    EntityID eb = pathway_grid_.get_pathway_at(b.x, b.y);

    if (ea == INVALID_ENTITY || eb == INVALID_ENTITY) return false;

    auto& road_a = registry_.get<RoadComponent>(ea);
    auto& road_b = registry_.get<RoadComponent>(eb);

    return road_a.network_id == road_b.network_id && road_a.network_id != 0;
}
```

---

## 3. Pathfinding

### 3.1 When Pathfinding is Needed

1. **Building Traffic Contribution**: Determine which pathways a building's traffic flows through
2. **Emergency Response Routing**: Hazard response dispatching (Epic 9/13)
3. **Cosmetic Entity Movement**: Beings/vehicles following pathways (visual only)
4. **Query Tool**: Player asks "how do I get from A to B?"

**Note:** Most gameplay does NOT require per-tick pathfinding. Traffic simulation uses aggregate flow, not individual agents.

### 3.2 A* Implementation

Standard A* with Manhattan distance heuristic for grid-based pathways.

```cpp
struct PathfindingNode {
    GridPosition pos;
    uint16_t g_cost;      // Cost from start
    uint16_t h_cost;      // Heuristic to goal
    uint16_t parent_idx;  // Index of parent in closed set
};

std::vector<GridPosition> NetworkGraph::find_path(
    GridPosition start,
    GridPosition goal
) const {
    if (!is_connected(start, goal)) {
        return {};  // No path exists
    }

    // A* open set (min-heap by f_cost = g + h)
    auto compare = [](const PathfindingNode& a, const PathfindingNode& b) {
        return (a.g_cost + a.h_cost) > (b.g_cost + b.h_cost);
    };
    std::priority_queue<PathfindingNode, std::vector<PathfindingNode>, decltype(compare)> open(compare);

    // Closed set
    std::unordered_map<GridPosition, PathfindingNode> closed;

    // Initialize with start
    open.push({start, 0, manhattan_distance(start, goal), UINT16_MAX});

    while (!open.empty()) {
        PathfindingNode current = open.top();
        open.pop();

        if (current.pos == goal) {
            return reconstruct_path(closed, current);
        }

        if (closed.count(current.pos)) continue;
        closed[current.pos] = current;

        // Expand neighbors
        for (GridPosition neighbor : get_pathway_neighbors(current.pos)) {
            if (closed.count(neighbor)) continue;

            uint16_t g = current.g_cost + get_edge_cost(current.pos, neighbor);
            uint16_t h = manhattan_distance(neighbor, goal);

            open.push({neighbor, g, h, static_cast<uint16_t>(closed.size() - 1)});
        }
    }

    return {};  // No path found (shouldn't happen if is_connected passed)
}
```

### 3.3 Edge Cost Function

Edge cost incorporates pathway type and congestion:

```cpp
uint16_t get_edge_cost(GridPosition from, GridPosition to) const {
    EntityID entity = pathway_grid_.get_pathway_at(to.x, to.y);
    if (entity == INVALID_ENTITY) return UINT16_MAX;

    auto& road = registry_.get<RoadComponent>(entity);

    // Base cost by type
    uint16_t base_cost = 10;  // BasicPathway
    switch (road.type) {
        case PathwayType::TransitCorridor: base_cost = 5;  break;  // Faster
        case PathwayType::Pedestrian:      base_cost = 20; break;  // Slower
        case PathwayType::Bridge:          base_cost = 10; break;
        case PathwayType::Tunnel:          base_cost = 10; break;
    }

    // Congestion penalty (optional, for cosmetic routing)
    if (registry_.has<TrafficComponent>(entity)) {
        auto& traffic = registry_.get<TrafficComponent>(entity);
        base_cost += traffic.congestion_level / 25;  // 0-10 additional cost
    }

    // Decay penalty
    base_cost += (255 - road.health) / 50;  // 0-5 additional cost

    return base_cost;
}
```

### 3.4 Pathfinding Cache

For frequently-queried routes (emergency response, cosmetic beings), cache results:

```cpp
struct CachedPath {
    std::vector<GridPosition> path;
    uint32_t computed_tick;
    bool valid;
};

class PathCache {
private:
    std::unordered_map<std::pair<GridPosition, GridPosition>, CachedPath> cache_;
    uint32_t max_age_ = 100;  // Invalidate after 100 ticks (5 seconds)

public:
    std::vector<GridPosition>* get_cached_path(GridPosition start, GridPosition goal, uint32_t current_tick);
    void store_path(GridPosition start, GridPosition goal, std::vector<GridPosition> path, uint32_t current_tick);
    void invalidate_all();  // Called when network changes
};
```

### 3.5 Performance Targets

| Operation | Target | Notes |
|-----------|--------|-------|
| Single A* (256x256) | <1ms | Worst case diagonal traversal |
| Single A* (512x512) | <5ms | Worst case |
| Connectivity query | O(1) | After network rebuild |
| Network rebuild | <50ms | Full rebuild, 15,000 segments |
| Cached path lookup | O(1) | Hash map lookup |

---

## 4. Traffic Simulation Model

### 4.1 Aggregate Flow Model (Recommended)

Traffic simulation uses an **aggregate flow model**, not individual agent tracking. Each building contributes flow units to nearby pathways; flow aggregates and causes congestion.

**Why Aggregate, Not Agent-Based:**

1. **Performance**: Thousands of buildings, 20 ticks/second - agent-based is too expensive
2. **Determinism**: Aggregate model is easier to keep deterministic across clients
3. **Simplicity**: Fits the pool model pattern from energy/fluid systems
4. **Classic City Builder**: SimCity used aggregate traffic, not agents

### 4.2 Flow Contribution from Buildings

Each building contributes traffic based on its type, size, and occupancy:

```cpp
struct TrafficContribution {
    GridPosition origin;      // Building position
    uint32_t flow_amount;     // Flow units per tick
    uint8_t distribution_radius;  // How far traffic spreads
};

uint32_t calculate_building_traffic(EntityID building) const {
    auto& bldg = registry_.get<BuildingComponent>(building);

    if (bldg.state != BuildingState::Active) return 0;

    // Base traffic by zone type
    uint32_t base = 0;
    switch (bldg.zone_type) {
        case ZoneBuildingType::Habitation:  base = 2;  break;  // Beings leaving/arriving
        case ZoneBuildingType::Exchange:    base = 5;  break;  // More traffic (customers)
        case ZoneBuildingType::Fabrication: base = 3;  break;  // Goods transport
    }

    // Scale by occupancy
    float occupancy_ratio = static_cast<float>(bldg.current_occupancy) / bldg.capacity;
    base = static_cast<uint32_t>(base * occupancy_ratio);

    // Scale by level (larger buildings = more traffic)
    base *= bldg.level;

    return base;
}
```

### 4.3 Flow Distribution Algorithm

Traffic spreads from buildings to nearby pathways, then flows along the network toward high-connectivity areas (junctions attract traffic).

**Per-Tick Flow Calculation:**

```cpp
void TransportSystem::calculate_traffic(float delta_time) {
    // Phase 1: Clear previous flow (keep for smoothing)
    for (auto& [entity, traffic] : registry_.view<TrafficComponent>().each()) {
        traffic.flow_previous = traffic.flow_current;
        traffic.flow_current = 0;
        traffic.flow_sources = 0;
    }

    // Phase 2: Gather contributions from all active buildings
    for (auto& [entity, bldg, pos, owner] :
         registry_.view<BuildingComponent, PositionComponent, OwnershipComponent>().each()) {

        if (bldg.state != BuildingState::Active) continue;

        uint32_t flow = calculate_building_traffic(entity);
        if (flow == 0) continue;

        // Find nearest pathway
        GridPosition nearest_pathway;
        if (!find_nearest_pathway(pos, 3, nearest_pathway)) {
            continue;  // Building not road accessible (shouldn't happen for active buildings)
        }

        // Distribute flow to pathway and its neighbors
        distribute_flow_from_building(nearest_pathway, flow);
    }

    // Phase 3: Propagate flow along network (simplified diffusion)
    propagate_flow();

    // Phase 4: Calculate congestion levels
    for (auto& [entity, road, traffic] :
         registry_.view<RoadComponent, TrafficComponent>().each()) {

        traffic.congestion_level = calculate_congestion(traffic.flow_current, road.current_capacity);

        // Update flow blockage counter
        if (traffic.congestion_level > 200) {  // Heavily congested
            traffic.flow_blockage_ticks++;
        } else {
            traffic.flow_blockage_ticks = 0;
        }

        // Calculate contamination contribution
        if (traffic.congestion_level > 128) {  // Moderate+ congestion
            traffic.contamination_rate = (traffic.congestion_level - 128) / 8;  // 0-15
        } else {
            traffic.contamination_rate = 0;
        }
    }
}
```

### 4.4 Flow Propagation (Diffusion Model)

Flow spreads along pathways using a simple diffusion model:

```cpp
void TransportSystem::propagate_flow() {
    // Single-pass propagation (flow moves one step per tick)
    std::vector<std::pair<EntityID, uint32_t>> flow_deltas;

    for (auto& [entity, road, traffic, pos] :
         registry_.view<RoadComponent, TrafficComponent, PositionComponent>().each()) {

        if (traffic.flow_current == 0) continue;

        // Get connected neighbors
        std::vector<EntityID> neighbors = get_connected_neighbors(pos);
        if (neighbors.empty()) continue;

        // Flow distributes to neighbors based on their capacity ratio
        uint32_t total_neighbor_capacity = 0;
        for (EntityID neighbor : neighbors) {
            total_neighbor_capacity += registry_.get<RoadComponent>(neighbor).current_capacity;
        }

        if (total_neighbor_capacity == 0) continue;

        // Spread 20% of flow to neighbors each tick
        uint32_t spread_amount = traffic.flow_current / 5;

        for (EntityID neighbor : neighbors) {
            auto& neighbor_road = registry_.get<RoadComponent>(neighbor);
            uint32_t neighbor_share = (spread_amount * neighbor_road.current_capacity) / total_neighbor_capacity;
            flow_deltas.push_back({neighbor, neighbor_share});
        }
    }

    // Apply deltas
    for (auto& [entity, delta] : flow_deltas) {
        if (!registry_.has<TrafficComponent>(entity)) {
            registry_.emplace<TrafficComponent>(entity);
        }
        registry_.get<TrafficComponent>(entity).flow_current += delta;
    }
}
```

### 4.5 Congestion Effects

| Congestion Level | Description | Effects |
|-----------------|-------------|---------|
| 0-50 | Free flow | No negative effects |
| 51-100 | Light traffic | Minor visual indication |
| 101-150 | Moderate traffic | Slight sector value reduction nearby |
| 151-200 | Heavy traffic | Contamination begins, sector value drops |
| 201-255 | Flow blockage | Severe contamination, demand reduction |

### 4.6 Update Frequency

- **Flow calculation**: Every tick (20 Hz)
- **Congestion effects on buildings**: Every 10 ticks (2 Hz)
- **Contamination contribution**: Every 20 ticks (1 Hz), aggregated to ContaminationSystem

---

## 5. Road Decay Mechanics

### 5.1 Decay Factors

Pathway health decreases based on:

1. **Base Decay**: All pathways decay slowly over time
2. **Usage Decay**: Higher traffic = faster decay
3. **Weather**: Future hook for weather system (stub for now)
4. **Terrain**: Pathways on difficult terrain decay faster

```cpp
void TransportSystem::update_pathway_decay(float delta_time) {
    // Run every 100 ticks (5 seconds) to reduce per-tick overhead
    for (auto& [entity, road, traffic, pos] :
         registry_.view<RoadComponent, TrafficComponent, PositionComponent>().each()) {

        // Base decay (0.1 health per 5 seconds = 255 health lasts ~21 minutes)
        float decay = road.decay_rate * 0.1f;

        // Usage multiplier (high traffic = 3x decay)
        float usage_multiplier = 1.0f + 2.0f * (traffic.flow_current / static_cast<float>(road.base_capacity));
        decay *= usage_multiplier;

        // Terrain multiplier (stubs - future terrain integration)
        // float terrain_multiplier = get_terrain_decay_multiplier(pos);
        float terrain_multiplier = 1.0f;
        decay *= terrain_multiplier;

        // Apply decay
        road.health = static_cast<uint8_t>(std::max(0.0f, road.health - decay));

        // Update capacity based on health
        road.current_capacity = (road.base_capacity * road.health) / 255;

        // Emit event if health crosses threshold
        if (road.health < 51 && road.health + decay >= 51) {
            emit(PathwayDeterioratedEvent{entity, pos, road.health});
        }
    }
}
```

### 5.2 Maintenance

Pathways can be maintained to restore health. Maintenance is handled by EconomySystem (Epic 11), but TransportSystem provides the interface:

```cpp
// Called by EconomySystem when maintenance budget is allocated
void TransportSystem::apply_maintenance(EntityID pathway, uint8_t health_restored) {
    auto& road = registry_.get<RoadComponent>(pathway);
    road.health = std::min(255, road.health + health_restored);
    road.current_capacity = (road.base_capacity * road.health) / 255;
    road.last_maintained_tick = current_tick_;
}

// Query: How much maintenance is needed?
uint32_t TransportSystem::get_total_maintenance_cost(PlayerID owner) const {
    uint32_t total = 0;
    for (auto& [entity, road, ownership] :
         registry_.view<RoadComponent, OwnershipComponent>().each()) {

        if (ownership.owner != owner) continue;

        // Cost proportional to missing health
        uint32_t missing_health = 255 - road.health;
        uint32_t cost_per_health = get_pathway_type_maintenance_cost(road.type);
        total += (missing_health * cost_per_health) / 255;
    }
    return total;
}
```

### 5.3 Visual Feedback

Decay state maps to visual appearance:

| Health Range | Visual State | Description |
|--------------|--------------|-------------|
| 200-255 | Pristine | Full glow, clean texture |
| 150-199 | Good | Slight wear marks |
| 100-149 | Worn | Visible cracks, dimmer |
| 50-99 | Poor | Significant damage, minimal glow |
| 0-49 | Crumbling | Heavy damage, no glow, warning overlay |

---

## 6. Rail System Details

### 6.1 Rail vs Road Differences

| Aspect | Pathway (Road) | Rail |
|--------|---------------|------|
| Capacity Unit | Flow units | Beings per cycle |
| Power Required | No | Yes |
| Cross-Ownership | Yes | No (per-player networks) |
| Decay | Yes (traffic-based) | No (maintenance-based only) |
| Contamination | Yes (when congested) | No |
| Build Cost | Low-Medium | High |
| Coverage Model | Proximity (3 tiles) | Terminal-based (radius) |

### 6.2 Rail System Architecture

```cpp
class RailSystem : public ISimulatable {
private:
    // Per-player rail networks
    std::array<RailNetwork, MAX_PLAYERS> player_networks_;

    // Subterra layer tracking
    SubterraLayerManager subterra_manager_;

    // Interface dependencies
    IEnergyProvider* energy_provider_;
    ITerrainQueryable* terrain_queryable_;

public:
    void tick(float delta_time) override;

    // Rail operations
    bool validate_rail_placement(GridPosition pos, RailType type, PlayerID owner) const;
    void place_rail(GridPosition pos, RailType type, PlayerID owner);
    void remove_rail(GridPosition pos, PlayerID owner);

    // Terminal operations
    void place_terminal(GridPosition pos, TerminalType type, PlayerID owner);
    void remove_terminal(GridPosition pos, PlayerID owner);

    // Queries
    bool has_rail_access(GridPosition pos, PlayerID owner) const;
    uint32_t get_rail_capacity(PlayerID owner) const;
    uint32_t get_rail_load(PlayerID owner) const;
};
```

### 6.3 Terminal Types

```cpp
enum class TerminalType : uint8_t {
    SurfaceStation   = 0,  // Surface rail terminal
    SubterraStation  = 1,  // Subterra terminal (subway entrance)
    IntermodalHub    = 2   // Connects surface and subterra
};

struct TerminalComponent {
    TerminalType type      = TerminalType::SurfaceStation;
    uint16_t capacity      = 200;   // Beings per cycle throughput
    uint16_t current_usage = 0;
    uint8_t coverage_radius = 8;    // Tiles served by this terminal
    bool is_powered        = false;
    bool is_active         = false; // Powered AND connected to rail
    uint8_t padding[2];
};
static_assert(sizeof(TerminalComponent) == 10, "TerminalComponent must be 10 bytes");
```

### 6.4 Rail Coverage Model

Unlike pathways (3-tile proximity rule), rail uses a **terminal coverage radius** model:

- Buildings within `coverage_radius` tiles of an active terminal can use rail transit
- Rail transit reduces traffic contribution from those buildings by a percentage
- Multiple terminals in range don't stack - highest coverage wins

```cpp
float get_rail_traffic_reduction(GridPosition building_pos, PlayerID owner) const {
    // Find nearest active terminal owned by player
    EntityID nearest_terminal = INVALID_ENTITY;
    uint8_t min_distance = 255;

    for (auto& [entity, terminal, pos, ownership] :
         registry_.view<TerminalComponent, PositionComponent, OwnershipComponent>().each()) {

        if (ownership.owner != owner) continue;
        if (!terminal.is_active) continue;

        uint8_t dist = manhattan_distance(building_pos, pos);
        if (dist < min_distance && dist <= terminal.coverage_radius) {
            min_distance = dist;
            nearest_terminal = entity;
        }
    }

    if (nearest_terminal == INVALID_ENTITY) return 0.0f;

    auto& terminal = registry_.get<TerminalComponent>(nearest_terminal);

    // Linear falloff: 50% reduction at terminal, 0% at edge of radius
    float distance_ratio = static_cast<float>(min_distance) / terminal.coverage_radius;
    return 0.5f * (1.0f - distance_ratio);
}
```

### 6.5 Subterra Layer Management

Subterra (subway) operates on a separate layer from surface infrastructure:

```cpp
class SubterraLayerManager {
private:
    // Separate grid for subterra entities
    std::vector<EntityID> subterra_grid_;  // Same dimensions as surface
    uint32_t width_, height_;

public:
    // Subterra-specific queries
    EntityID get_subterra_at(int32_t x, int32_t y) const;
    bool has_subterra(int32_t x, int32_t y) const;

    // Layer interaction
    std::vector<GridPosition> get_ventilation_affected_tiles(GridPosition subterra_pos) const;
    bool can_build_subterra_at(int32_t x, int32_t y, const ITerrainQueryable& terrain) const;
};
```

**Subterra Building Rules:**

1. Cannot build subterra under water bodies
2. Cannot build subterra under very deep terrain (elevation < 5)
3. Subterra tunnels must connect to terminals to be useful
4. Subterra terminals create surface ventilation grates (cosmetic, minor land value impact)

### 6.6 Power Dependency

Rail requires power from EnergySystem:

```cpp
void RailSystem::update_power_states() {
    for (auto& [entity, rail, pos, ownership] :
         registry_.view<RailComponent, PositionComponent, OwnershipComponent>().each()) {

        rail.is_powered = energy_provider_->is_powered(entity);

        // Active = powered AND connected to network with terminals
        rail.is_active = rail.is_powered &&
                        is_connected_to_terminal(entity, ownership.owner);
    }

    for (auto& [entity, terminal, pos, ownership] :
         registry_.view<TerminalComponent, PositionComponent, OwnershipComponent>().each()) {

        terminal.is_powered = energy_provider_->is_powered(entity);

        // Active = powered AND connected to rail tracks
        terminal.is_active = terminal.is_powered &&
                            has_adjacent_rail(pos, ownership.owner);
    }
}
```

---

## 7. Proximity Query System

### 7.1 ITransportConnectable Implementation

Per canon `interfaces.yaml`, ITransportConnectable enforces the 3-tile road proximity rule for buildings:

```cpp
class ITransportConnectable {
public:
    virtual ~ITransportConnectable() = default;

    // Distance to nearest pathway (0 if adjacent)
    virtual uint32_t get_nearest_road_distance() const = 0;

    // True if within required road proximity (3 tiles)
    virtual bool is_road_accessible() const = 0;

    // Traffic contribution to nearby roads
    virtual uint32_t get_traffic_contribution() const = 0;
};
```

### 7.2 Distance Calculation

```cpp
uint8_t PathwayGrid::get_nearest_pathway_distance(int32_t x, int32_t y) const {
    // BFS from (x, y) until we find a pathway
    // Capped at 255 (max uint8_t)

    if (has_pathway(x, y)) return 0;

    std::queue<std::tuple<int32_t, int32_t, uint8_t>> frontier;
    std::set<std::pair<int32_t, int32_t>> visited;

    frontier.push({x, y, 0});
    visited.insert({x, y});

    static const int dx[] = {0, 0, 1, -1};
    static const int dy[] = {1, -1, 0, 0};

    while (!frontier.empty()) {
        auto [cx, cy, dist] = frontier.front();
        frontier.pop();

        if (dist >= 255) return 255;  // Cap reached

        for (int i = 0; i < 4; ++i) {
            int32_t nx = cx + dx[i];
            int32_t ny = cy + dy[i];

            if (!in_bounds(nx, ny)) continue;
            if (visited.count({nx, ny})) continue;
            visited.insert({nx, ny});

            if (has_pathway(nx, ny)) {
                return dist + 1;
            }

            frontier.push({nx, ny, static_cast<uint8_t>(dist + 1)});
        }
    }

    return 255;  // No pathway found
}
```

### 7.3 Proximity Cache

Distance queries are expensive (BFS). Cache results with dirty tracking:

```cpp
class ProximityCache {
private:
    std::vector<uint8_t> distance_cache_;  // One byte per tile
    bool dirty_ = true;
    uint32_t width_, height_;

public:
    uint8_t get_distance(int32_t x, int32_t y);
    void rebuild_if_dirty(const PathwayGrid& grid);
    void mark_dirty() { dirty_ = true; }
};
```

**Rebuild Strategy:**

Full BFS from all pathway tiles (multi-source BFS):

```cpp
void ProximityCache::rebuild(const PathwayGrid& grid) {
    std::fill(distance_cache_.begin(), distance_cache_.end(), 255);

    std::queue<std::tuple<int32_t, int32_t, uint8_t>> frontier;

    // Seed with all pathway tiles at distance 0
    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            if (grid.has_pathway(x, y)) {
                distance_cache_[y * width_ + x] = 0;
                frontier.push({x, y, 0});
            }
        }
    }

    // BFS to all other tiles
    while (!frontier.empty()) {
        auto [cx, cy, dist] = frontier.front();
        frontier.pop();

        static const int dx[] = {0, 0, 1, -1};
        static const int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int32_t nx = cx + dx[i];
            int32_t ny = cy + dy[i];

            if (nx < 0 || nx >= width_ || ny < 0 || ny >= height_) continue;

            uint8_t new_dist = std::min(255, dist + 1);
            size_t idx = ny * width_ + nx;

            if (distance_cache_[idx] > new_dist) {
                distance_cache_[idx] = new_dist;
                if (new_dist < 255) {
                    frontier.push({nx, ny, new_dist});
                }
            }
        }
    }

    dirty_ = false;
}
```

**Memory:** 1 byte per tile = 256 KB for 512x512

### 7.4 BuildingSystem Integration

BuildingSystem queries TransportSystem via ITransportProvider:

```cpp
class TransportSystem : public ISimulatable, public ITransportProvider {
public:
    // ITransportProvider implementation
    bool is_road_accessible(int32_t x, int32_t y) const override {
        return proximity_cache_.get_distance(x, y) <= 3;
    }

    uint32_t get_nearest_road_distance(int32_t x, int32_t y) const override {
        return proximity_cache_.get_distance(x, y);
    }
};
```

---

## 8. Cross-Ownership Connectivity

### 8.1 Canon Rule: Roads Connect Across Ownership

Per `patterns.yaml`:
```yaml
roads:
  connects_across_ownership: true
  description: "Roads connect physically regardless of tile ownership"
  effect: "Traffic flows across all connected roads"
```

This is unique to pathways - energy conduits and fluid conduits do NOT connect across ownership.

### 8.2 Implementation

```cpp
// When checking pathway connectivity, ignore ownership
bool PathwayGrid::has_pathway_connection(GridPosition a, GridPosition b) const {
    // Just check adjacency and existence - ownership doesn't matter
    if (!is_adjacent(a, b)) return false;
    return has_pathway(a.x, a.y) && has_pathway(b.x, b.y);
}

// When building the network graph, connect across ownership
void NetworkGraph::add_edge_if_connected(GridPosition a, GridPosition b) {
    // No ownership check here
    if (pathway_grid_.has_pathway(a.x, a.y) &&
        pathway_grid_.has_pathway(b.x, b.y) &&
        is_adjacent(a, b)) {
        add_bidirectional_edge(a, b);
    }
}
```

### 8.3 Ownership Tracking

Each pathway segment still has an owner (for maintenance costs, visual distinction):

```cpp
// RoadComponent doesn't store owner - use OwnershipComponent
// Each pathway entity has both RoadComponent and OwnershipComponent

PlayerID get_pathway_owner(GridPosition pos) const {
    EntityID entity = pathway_grid_.get_pathway_at(pos.x, pos.y);
    if (entity == INVALID_ENTITY) return GAME_MASTER;
    return registry_.get<OwnershipComponent>(entity).owner;
}
```

### 8.4 Visual Distinction at Boundaries

When rendering, show ownership boundaries on connected pathways:

```cpp
struct PathwayRenderData {
    GridPosition position;
    PathwayType type;
    uint8_t health;
    uint8_t congestion_level;
    PlayerID owner;
    uint8_t boundary_flags;  // Bit flags: N(1), S(2), E(4), W(8) = ownership boundary
};

void TransportSystem::generate_render_data(std::vector<PathwayRenderData>& out) const {
    for (auto& [entity, road, pos, ownership] :
         registry_.view<RoadComponent, PositionComponent, OwnershipComponent>().each()) {

        PathwayRenderData data;
        data.position = pos;
        data.type = road.type;
        data.health = road.health;
        data.owner = ownership.owner;

        // Congestion (if present)
        if (registry_.has<TrafficComponent>(entity)) {
            data.congestion_level = registry_.get<TrafficComponent>(entity).congestion_level;
        } else {
            data.congestion_level = 0;
        }

        // Boundary detection
        data.boundary_flags = 0;
        static const int dx[] = {0, 0, 1, -1};
        static const int dy[] = {-1, 1, 0, 0};  // N, S, E, W
        static const uint8_t flags[] = {1, 2, 4, 8};

        for (int i = 0; i < 4; ++i) {
            GridPosition neighbor = {pos.grid_x + dx[i], pos.grid_y + dy[i]};
            if (has_pathway(neighbor.x, neighbor.y)) {
                PlayerID neighbor_owner = get_pathway_owner(neighbor);
                if (neighbor_owner != ownership.owner) {
                    data.boundary_flags |= flags[i];
                }
            }
        }

        out.push_back(data);
    }
}
```

### 8.5 Flow Calculation Across Boundaries

Traffic flow crosses ownership boundaries without penalty:

```cpp
void TransportSystem::distribute_flow_from_building(GridPosition nearest_pathway, uint32_t flow) {
    // Distribute to all connected pathways regardless of owner
    std::queue<std::pair<GridPosition, uint32_t>> frontier;
    std::set<GridPosition> visited;

    frontier.push({nearest_pathway, flow});

    while (!frontier.empty() && flow > 0) {
        auto [pos, remaining_flow] = frontier.front();
        frontier.pop();

        if (visited.count(pos)) continue;
        visited.insert(pos);

        EntityID entity = pathway_grid_.get_pathway_at(pos.x, pos.y);
        if (entity == INVALID_ENTITY) continue;

        // Add flow to this pathway (regardless of owner)
        auto& traffic = ensure_traffic_component(entity);
        uint32_t absorbed = std::min(remaining_flow, static_cast<uint32_t>(10));
        traffic.flow_current += absorbed;
        remaining_flow -= absorbed;

        if (remaining_flow == 0) break;

        // Spread to neighbors (crossing ownership boundaries)
        for (GridPosition neighbor : get_connected_neighbors(pos)) {
            if (!visited.count(neighbor)) {
                frontier.push({neighbor, remaining_flow / 4});
            }
        }
    }
}
```

### 8.6 Maintenance Responsibility

Each player maintains only their own pathway segments:

```cpp
uint32_t TransportSystem::get_player_maintenance_cost(PlayerID owner) const {
    uint32_t total = 0;

    for (auto& [entity, road, ownership] :
         registry_.view<RoadComponent, OwnershipComponent>().each()) {

        // Only count segments owned by this player
        if (ownership.owner != owner) continue;

        uint32_t missing_health = 255 - road.health;
        uint32_t cost = (missing_health * get_maintenance_cost_per_health(road.type)) / 255;
        total += cost;
    }

    return total;
}
```

---

## 9. Performance Considerations

### 9.1 Pathway Count Estimates

| Map Size | Estimated Pathway Segments | Estimated Junctions |
|----------|---------------------------|---------------------|
| 128x128 | 1,000 - 3,000 | 200 - 600 |
| 256x256 | 5,000 - 15,000 | 1,000 - 3,000 |
| 512x512 | 15,000 - 50,000 | 3,000 - 10,000 |

### 9.2 Per-Tick Budget Allocation

Target: TransportSystem tick completes in <3ms on 256x256 map.

| Phase | Operation | Target Budget |
|-------|-----------|---------------|
| 1 | Network rebuild (if dirty) | <50ms (amortized: rare) |
| 2 | Proximity cache rebuild (if dirty) | <20ms (amortized: rare) |
| 3 | Traffic flow calculation | <1ms |
| 4 | Congestion calculation | <0.5ms |
| 5 | Decay update (every 100 ticks) | <0.5ms |
| 6 | Rail power state update | <0.2ms |
| **Total (steady state)** | | **<2ms** |

### 9.3 Pathfinding Performance

| Query Type | Frequency | Target |
|------------|-----------|--------|
| Connectivity check | High (BuildingSystem queries) | O(1) via network_id |
| Nearest pathway distance | High (proximity rule) | O(1) via cache |
| Full A* path | Low (cosmetic entities, query tool) | <5ms per path |

### 9.4 Memory Summary

| Data Structure | Size (256x256) | Size (512x512) |
|----------------|----------------|----------------|
| PathwayGrid | 256 KB | 1 MB |
| ProximityCache | 64 KB | 256 KB |
| NetworkGraph (lazy) | ~100 KB | ~400 KB |
| Components (typical) | ~400 KB | ~1.5 MB |
| Path Cache | ~50 KB | ~200 KB |
| **Total** | **~870 KB** | **~3.4 MB** |

### 9.5 Optimization Strategies

1. **Dirty Flag Everything**: Only rebuild/recalculate when pathways change
2. **Spatial Locality**: Process pathways in grid order for cache friendliness
3. **Sparse TrafficComponent**: Only attach when flow > 0
4. **Lazy Graph Rebuild**: Don't rebuild until a query needs it
5. **Proximity Cache**: Amortize BFS cost across many queries
6. **Batched Updates**: Decay every 100 ticks, not every tick

---

## 10. Key Work Items

### Infrastructure Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-001 | PathwayType, PathwayDirection enums | S | P0 | None |
| T-002 | RoadComponent struct (16 bytes) | S | P0 | T-001 |
| T-003 | TrafficComponent struct (16 bytes) | S | P0 | None |
| T-004 | RailType enum | S | P0 | None |
| T-005 | RailComponent struct (12 bytes) | S | P0 | T-004 |
| T-006 | SubterraComponent struct (8 bytes) | S | P0 | T-005 |
| T-007 | TerminalType enum, TerminalComponent struct | S | P0 | T-005 |
| T-008 | Transport event definitions | S | P0 | None |

### Spatial Data Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-009 | PathwayGrid dense 2D array class | M | P0 | T-002 |
| T-010 | ProximityCache for distance queries | M | P0 | T-009 |
| T-011 | Multi-source BFS for proximity rebuild | M | P0 | T-010 |
| T-012 | SubterraLayerManager | M | P1 | T-006 |

### Network Graph Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-013 | NetworkGraph node/edge structures | M | P0 | None |
| T-014 | Graph rebuild from PathwayGrid | L | P0 | T-009, T-013 |
| T-015 | Connected component (network_id) assignment | M | P0 | T-014 |
| T-016 | Connectivity query (O(1) via network_id) | S | P0 | T-015 |
| T-017 | PathCache for frequently-queried routes | M | P2 | T-013 |

### Pathfinding Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-018 | A* pathfinding implementation | L | P1 | T-014 |
| T-019 | Edge cost function (type, congestion, decay) | M | P1 | T-018 |
| T-020 | find_path() public API | S | P1 | T-018 |

### Traffic Simulation Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-021 | Building traffic contribution calculation | M | P0 | None |
| T-022 | Flow distribution from buildings to pathways | L | P0 | T-009, T-021 |
| T-023 | Flow propagation (diffusion model) | L | P0 | T-022 |
| T-024 | Congestion calculation | M | P0 | T-023 |
| T-025 | Contamination rate calculation (IContaminationSource) | M | P1 | T-024 |

### Decay Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-026 | Decay update system (every 100 ticks) | M | P1 | T-002 |
| T-027 | Capacity degradation from health | S | P1 | T-026 |
| T-028 | Maintenance application API | S | P1 | T-027 |
| T-029 | PathwayDeterioratedEvent emission | S | P2 | T-026 |

### ITransportProvider Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-030 | ITransportProvider interface definition | S | P0 | None |
| T-031 | is_road_accessible() implementation | S | P0 | T-010, T-030 |
| T-032 | get_nearest_road_distance() implementation | S | P0 | T-010, T-030 |
| T-033 | Replace BuildingSystem stub with real implementation | M | P0 | T-031, T-032 |

### Cross-Ownership Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-034 | Cross-ownership connectivity in NetworkGraph | M | P0 | T-014 |
| T-035 | Boundary flags for rendering | S | P1 | T-034 |
| T-036 | Per-owner maintenance cost calculation | S | P1 | T-026 |

### Rail System Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-037 | RailSystem class skeleton (ISimulatable) | M | P1 | T-005 |
| T-038 | Rail placement validation | M | P1 | T-037 |
| T-039 | Rail power state update (IEnergyProvider) | M | P1 | T-037 |
| T-040 | Terminal placement and activation | M | P1 | T-007, T-037 |
| T-041 | Rail coverage model (terminal radius) | M | P1 | T-040 |
| T-042 | Rail traffic reduction calculation | M | P2 | T-041 |
| T-043 | Subterra placement rules | M | P2 | T-012, T-037 |

### TransportSystem Integration Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-044 | TransportSystem class (ISimulatable, priority TBD) | L | P0 | All T-0XX |
| T-045 | Dirty flag coordination | M | P0 | T-044 |
| T-046 | Tick phase orchestration | M | P0 | T-044 |

### Network Sync Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-047 | RoadComponent serialization | M | P1 | T-002 |
| T-048 | TrafficComponent serialization | S | P1 | T-003 |
| T-049 | Network messages for pathway operations | M | P1 | T-044 |

### Content Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| T-050 | Define pathway type base stats | S | P0 | @game-designer |
| T-051 | Define decay rates and thresholds | S | P1 | @game-designer |
| T-052 | Define rail capacity and costs | S | P1 | @game-designer |
| T-053 | Balance traffic contribution values | S | P2 | @game-designer |

### Work Item Summary

| Category | Count | S | M | L |
|----------|-------|---|---|---|
| Infrastructure | 8 | 8 | 0 | 0 |
| Spatial Data | 4 | 0 | 4 | 0 |
| Network Graph | 5 | 2 | 2 | 1 |
| Pathfinding | 3 | 1 | 1 | 1 |
| Traffic Simulation | 5 | 0 | 3 | 2 |
| Decay | 4 | 2 | 2 | 0 |
| ITransportProvider | 4 | 3 | 1 | 0 |
| Cross-Ownership | 3 | 2 | 1 | 0 |
| Rail System | 7 | 0 | 7 | 0 |
| Integration | 3 | 0 | 2 | 1 |
| Network Sync | 3 | 1 | 2 | 0 |
| Content | 4 | 4 | 0 | 0 |
| **Total** | **53** | **23** | **25** | **5** |

---

## 11. Questions for Other Agents

### @systems-architect

1. **Tick Priority for TransportSystem:** EnergySystem is 10, FluidSystem is 20, ZoneSystem is 30, BuildingSystem is 40. Where should TransportSystem fit? I suggest **25** (after utilities, before zones) so buildings can query road accessibility during spawn checks.

2. **RailSystem as Separate System:** Canon defines TransportSystem and RailSystem as separate ECS systems. Should RailSystem be a separate class with its own tick, or a subsystem within TransportSystem? I recommend separate for cleaner code, but they need to share data.

3. **Dense Grid Pattern Confirmation:** PathwayGrid and ProximityCache follow the `dense_grid_exception` pattern from canon. Should this be formally documented in patterns.yaml alongside TerrainGrid, BuildingGrid, etc.?

4. **IContaminationSource for Traffic:** High-traffic pathways produce contamination per `interfaces.yaml`. Should TransportSystem implement IContaminationSource directly, or should ContaminationSystem query TrafficComponent?

### @game-designer

5. **3-Tile Rule Clarification:** Is the 3-tile road proximity rule measured by Manhattan distance or Chebyshev (including diagonals)? Current analysis assumes Manhattan.

6. **Traffic Contribution Values:** Proposed values:
   - Habitation: 2 flow/tick per occupant ratio
   - Exchange: 5 flow/tick per occupant ratio
   - Fabrication: 3 flow/tick per occupant ratio
   Are these reasonable starting points for balance?

7. **Decay Timeline:** Proposed decay: pristine pathway lasts ~20 minutes of game time with average traffic before reaching "Poor" state (health 50-99). Is this the right pace?

8. **Transit Corridor (Highway) Usage:** When should players unlock transit corridors? Population milestone? Technology progression? Build cost only?

9. **Rail Capacity Model:** Rail capacity is in "beings per cycle." What's a cycle in this context? One simulation tick? One game-time rotation (day)?

10. **Subterra Depth Levels:** Is multi-depth subterra (deep subway) needed for MVP, or can we ship with single-depth and add levels later?

### @building-engineer

11. **ITransportProvider Stub Replacement:** The stub in Epic 4 (`StubTransportProvider`) returns true for all `is_road_accessible()` calls. When Epic 7 goes live, how should we handle the transition? Immediate swap, or a migration period with warnings?

12. **Multi-Tile Building Road Access:** For multi-tile buildings (2x2 high-density), is road accessibility checked from ANY tile in the footprint, or from the anchor tile only? I assume any tile.

### @graphics-engineer

13. **Pathway Rendering Data:** What data does RenderingSystem need per pathway segment? I've proposed `PathwayRenderData` struct with position, type, health, congestion, owner, boundary flags. Is this sufficient?

14. **Congestion Visualization:** Congestion level 0-255 maps to visual feedback. Is this a color gradient overlay, or particle effects, or both? Need to know what TrafficComponent should expose.

15. **Ownership Boundary Rendering:** Cross-ownership pathway connections need visual distinction. Is a colored border/edge sufficient, or do we need different textures?

---

## 12. Risks & Concerns

### R1: Network Rebuild Performance (HIGH)

**Risk:** Full network graph rebuild on every pathway placement could cause frame hitches on large maps.

**Mitigation:**
- Dirty flag ensures rebuild only happens once per frame at most
- Rebuild is O(segments), not O(tiles)
- Profile early; consider incremental update for hot path
- Rebuild can be spread across multiple frames if needed

### R2: Pathfinding Cost Explosion (MEDIUM)

**Risk:** If pathfinding is called frequently (every cosmetic entity, every tick), CPU budget will be exceeded.

**Mitigation:**
- Cosmetic entities use pre-cached paths
- Invalidate cache on network change (infrequent)
- Limit path length (reject paths > 100 segments)
- Pathfinding is not needed for gameplay - only cosmetics and query tool

### R3: Cross-Ownership Complexity (MEDIUM)

**Risk:** Cross-ownership connectivity is unique to pathways. Edge cases around maintenance, visual boundaries, and flow calculation could create bugs.

**Mitigation:**
- Extensive unit tests for cross-boundary scenarios
- Clear ownership tracking (OwnershipComponent on each segment)
- Isolated rendering boundary calculation

### R4: Traffic Simulation Fidelity (LOW)

**Risk:** Aggregate flow model may feel "unrealistic" compared to agent-based traffic. Players may expect to see individual vehicles.

**Mitigation:**
- Cosmetic vehicles/beings follow pathways but don't affect simulation (visual only)
- Flow model is proven (SimCity 1-4 used similar)
- Clear documentation that traffic is aggregate, not per-vehicle

### R5: Proximity Cache Invalidation (MEDIUM)

**Risk:** If pathways change frequently, proximity cache rebuilds every tick, negating its benefit.

**Mitigation:**
- Batch pathway changes per player per tick
- Rebuild cache once per tick maximum
- Cache rebuild is O(tiles) but uses BFS which is cache-efficient

### R6: Rail/Road System Coupling (LOW)

**Risk:** RailSystem and TransportSystem need to share some data (e.g., traffic reduction from rail). Tight coupling could make maintenance difficult.

**Mitigation:**
- Clear interface boundaries
- Rail queries TransportSystem, not vice versa
- Shared types (GridPosition, PlayerID) but separate component sets

### R7: Subterra Layer Complexity (MEDIUM)

**Risk:** Subterra introduces a second spatial layer. UI, rendering, and placement logic all need to handle two layers.

**Mitigation:**
- Subterra is Phase 2 / stretch goal for Epic 7
- MVP can ship with surface rail only
- Clear SubterraLayerManager abstraction

---

## 13. Dependencies

### Incoming Dependencies (Systems We Depend On)

| System | Interface | Data Needed |
|--------|-----------|-------------|
| TerrainSystem (Epic 3) | ITerrainQueryable | is_buildable(), elevation for bridge/tunnel |
| BuildingSystem (Epic 4) | BuildingComponent queries | Building positions, occupancy, zone type for traffic |
| EnergySystem (Epic 5) | IEnergyProvider | Rail power state |
| SyncSystem (Epic 1) | ISerializable | Component sync |

### Outgoing Dependencies (Systems That Depend On Us)

| System | Interface | Data Provided |
|--------|-----------|---------------|
| BuildingSystem (Epic 4) | ITransportProvider | is_road_accessible(), get_nearest_road_distance() |
| ContaminationSystem (Epic 10) | IContaminationSource | Traffic contamination output |
| LandValueSystem (Epic 10) | TransportSystem queries | Traffic density for value calculation |
| ServicesSystem (Epic 9) | Network pathfinding | Emergency response routing |
| RenderingSystem (Epic 2) | PathwayRenderData | Pathway visuals |

### Canon Integration Points

| Canon File | Relevant Sections |
|------------|-------------------|
| systems.yaml | TransportSystem, RailSystem definitions |
| interfaces.yaml | ITransportConnectable |
| patterns.yaml | dense_grid_exception, infrastructure_connectivity |
| terminology.yaml | road->pathway, highway->transit_corridor, traffic->flow, congestion->flow_blockage, intersection->junction |

---

## Appendix A: Event Definitions

```cpp
// Pathway events
struct PathwayPlacedEvent {
    EntityID entity;
    GridPosition position;
    PathwayType type;
    PlayerID owner;
};

struct PathwayRemovedEvent {
    EntityID entity;
    GridPosition position;
    PlayerID owner;
};

struct PathwayDeterioratedEvent {
    EntityID entity;
    GridPosition position;
    uint8_t new_health;
};

struct PathwayRepairedEvent {
    EntityID entity;
    GridPosition position;
    uint8_t new_health;
};

// Network events
struct NetworkConnectedEvent {
    uint16_t network_id;
    std::vector<PlayerID> connected_players;  // Players whose roads now connect
};

struct NetworkDisconnectedEvent {
    uint16_t old_network_id;
    uint16_t new_network_id_a;
    uint16_t new_network_id_b;
};

// Rail events
struct RailPlacedEvent {
    EntityID entity;
    GridPosition position;
    RailType type;
    PlayerID owner;
};

struct RailRemovedEvent {
    EntityID entity;
    GridPosition position;
    PlayerID owner;
};

struct TerminalActivatedEvent {
    EntityID entity;
    GridPosition position;
    PlayerID owner;
};

struct TerminalDeactivatedEvent {
    EntityID entity;
    GridPosition position;
    PlayerID owner;
    bool due_to_power_loss;
};

// Flow events
struct FlowBlockageBeganEvent {
    EntityID pathway;
    GridPosition position;
    uint8_t congestion_level;
};

struct FlowBlockageEndedEvent {
    EntityID pathway;
    GridPosition position;
};
```

---

## Appendix B: Terminology Reference

| Human Term | Canonical Term | Code Identifier |
|------------|----------------|-----------------|
| Road | Pathway | PathwayType, RoadComponent |
| Highway | Transit Corridor | PathwayType::TransitCorridor |
| Traffic | Flow | flow_current, flow_previous |
| Congestion | Flow Blockage | flow_blockage_ticks |
| Intersection | Junction | is_junction |
| Railroad | Rail Line | RailComponent |
| Train | Rail Transport | (cosmetic only) |
| Train Station | Rail Terminal | TerminalComponent |
| Subway | Subterra Rail | SubterraComponent |
| Subway Station | Subterra Terminal | TerminalType::SubterraStation |

---

**Document Status:** Ready for review by Systems Architect and Game Designer

**Next Steps:**
1. @systems-architect to confirm tick priority and system separation
2. @game-designer to provide balance values for traffic, decay, and rail capacity
3. Finalize ITransportProvider interface before Epic 4 stub swap
4. Convert work items to formal tickets after questions resolved
