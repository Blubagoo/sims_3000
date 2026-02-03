# Systems Architect Analysis: Epic 7 - Transportation Network

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.8.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Network Model (vs Pool Model)](#3-network-model-vs-pool-model)
4. [ITransportProvider Interface Design](#4-itransportprovider-interface-design)
5. [Traffic Simulation Approach](#5-traffic-simulation-approach)
6. [Tick Priority Recommendation](#6-tick-priority-recommendation)
7. [Dense Grid Considerations](#7-dense-grid-considerations)
8. [Multiplayer Implications](#8-multiplayer-implications)
9. [Integration Points](#9-integration-points)
10. [Key Work Items](#10-key-work-items)
11. [Questions for Other Agents](#11-questions-for-other-agents)
12. [Risks and Concerns](#12-risks-and-concerns)
13. [Dependencies](#13-dependencies)

---

## 1. System Boundaries

### 1.1 TransportSystem

**Owns:**

- Pathway (road) placement and connectivity graph
- Pathway types: pathway (road), transit_corridor (highway)
- Junction (intersection) management and capacity
- Traffic (flow) simulation - aggregate flow model
- Flow_blockage (congestion) calculation per pathway segment
- Pathway decay from usage and underfunding
- Pathway connectivity queries (is building reachable?)
- Traffic contribution aggregation from buildings
- Path finding for connectivity queries

**Does NOT Own:**

- Rail networks (RailSystem owns)
- Pathway rendering (RenderingSystem owns)
- Pathway construction costs (EconomySystem owns)
- Traffic pollution calculation (ContaminationSystem owns - but queries TransportSystem)
- Cosmetic vehicle entities (RenderingSystem owns visual representation)
- Land value effects from traffic (LandValueSystem owns - queries TransportSystem)

**Provides:**

- `ITransportProvider` - Road proximity and accessibility queries (replaces Epic 4 stub)
- `ITransportConnectable` interface implementation
- Traffic density queries per pathway segment
- Connectivity graph queries (is point A connected to point B?)
- Aggregate flow data for ContaminationSystem

**Depends On:**

- TerrainSystem (valid placement via ITerrainQueryable)
- OwnershipComponent (pathway ownership for construction)
- BuildingSystem (building positions generate traffic)

### 1.2 RailSystem

**Owns:**

- Rail_line (railroad) track placement and connectivity
- Rail_terminal (train station) placement and operation
- Subterra_rail (subway) network - underground layer
- Subterra_terminal (subway station) placement
- Rail capacity calculation
- Rail decay from underfunding
- Transit coverage radius calculation

**Does NOT Own:**

- Surface roads/pathways (TransportSystem owns)
- Rail rendering (RenderingSystem owns)
- Rail construction costs (EconomySystem owns)
- Rail vehicle visuals (RenderingSystem owns)

**Provides:**

- Rail connectivity queries
- Transit coverage queries (radius around terminals)
- Traffic reduction effects (subway reduces road traffic)

**Depends On:**

- TerrainSystem (placement validation)
- EnergySystem (rail requires energy to operate)
- TransportSystem (rail terminals need pathway access)

### 1.3 Boundary Summary Table

| Concern | TransportSystem | RailSystem | Other System |
|---------|-----------------|------------|--------------|
| Surface road placement | **Owns** | - | - |
| Rail track placement | - | **Owns** | - |
| Subway network | - | **Owns** | - |
| Traffic simulation | **Owns** | - | - |
| Road rendering | - | - | RenderingSystem |
| Traffic pollution | Provides data | - | ContaminationSystem |
| Road costs | - | - | EconomySystem |
| Building road proximity | **Owns** (queries) | - | - |
| Cosmetic vehicles | - | - | RenderingSystem |

---

## 2. Data Model

### 2.1 PathwayComponent

```cpp
struct PathwayComponent {
    PathwayType pathway_type = PathwayType::Road;  // road, transit_corridor
    uint8_t capacity = 100;          // traffic capacity (vehicles/tick)
    uint8_t current_flow = 0;        // current traffic load
    uint8_t decay_level = 0;         // degradation (0-255, higher = more damaged)
    uint8_t upgrade_level = 0;       // 0 = base, 1+ = upgraded
    uint32_t network_id = 0;         // connected network identifier
    // Note: direction/connectivity stored in graph structure, not per-tile
};

enum class PathwayType : uint8_t {
    Road = 0,              // pathway - standard road
    TransitCorridor = 1,   // highway - higher capacity
    Bridge = 2,            // over water
    Tunnel = 3             // through terrain
};
```

### 2.2 JunctionComponent

```cpp
struct JunctionComponent {
    uint8_t connected_count = 0;     // number of connected pathways (1-4)
    uint8_t junction_type = 0;       // derived from connections (T, cross, etc.)
    uint8_t capacity_bonus = 0;      // traffic light/roundabout bonus
    bool has_traffic_control = false; // traffic light installed
};
```

### 2.3 TrafficComponent

```cpp
// Attached to buildings that generate traffic
struct TrafficComponent {
    uint32_t traffic_contribution = 0;  // traffic units generated per tick
    uint32_t nearest_pathway_id = 0;    // cached nearest pathway entity
    uint8_t pathway_distance = 255;      // tiles to nearest pathway (255 = no access)
    bool is_accessible = false;          // within 3-tile rule
};
```

### 2.4 RailComponent

```cpp
struct RailComponent {
    RailType rail_type = RailType::Surface;
    uint8_t capacity = 50;           // passenger capacity
    uint8_t current_load = 0;        // current utilization
    uint8_t decay_level = 0;         // degradation
    uint32_t line_id = 0;            // rail line identifier
    bool is_electrified = false;     // requires power
};

enum class RailType : uint8_t {
    Surface = 0,    // rail_line - above ground
    Subterra = 1,   // subterra_rail - underground
    Elevated = 2    // elevated rail (future)
};
```

### 2.5 RailTerminalComponent

```cpp
struct RailTerminalComponent {
    TerminalType terminal_type = TerminalType::Surface;
    uint8_t coverage_radius = 10;    // tiles affected by transit coverage
    uint32_t passenger_throughput = 0;
    uint8_t traffic_reduction_percent = 10;  // reduces road traffic in radius
    bool is_operational = false;     // has power + pathway access
};

enum class TerminalType : uint8_t {
    Surface = 0,      // rail_terminal
    Subterra = 1      // subterra_terminal
};
```

### 2.6 Pathway Network Graph Structure

Unlike energy/fluid (pool model), transport requires a **connectivity graph**:

```cpp
struct PathwayNetwork {
    uint32_t network_id;
    PlayerID owner;                          // or GAME_MASTER for shared
    std::vector<PathwaySegment> segments;    // all segments in network
    std::unordered_set<EntityID> junctions;  // junction entities
    bool is_connected_to_map_edge = false;   // external connection

    // Graph adjacency for pathfinding
    // Key: tile position, Value: connected tile positions
    std::unordered_map<GridPosition, std::vector<GridPosition>> adjacency;
};

struct PathwaySegment {
    GridPosition start;
    GridPosition end;
    PathwayType type;
    uint8_t capacity;
    uint8_t flow;        // aggregate flow on this segment
    float congestion;    // 0.0 = free, 1.0 = gridlock
};
```

### 2.7 Traffic Flow Aggregation

Traffic is NOT simulated per-vehicle. Instead, we aggregate:

```cpp
struct TrafficFlowData {
    // Per pathway segment, updated each tick
    uint32_t inbound_flow;     // traffic entering segment
    uint32_t outbound_flow;    // traffic leaving segment
    float congestion_factor;   // 0.0-1.0, affects travel time
    float pollution_factor;    // derived, used by ContaminationSystem
};
```

---

## 3. Network Model (vs Pool Model)

### 3.1 Critical Difference: Physical Connectivity

Energy and Fluid use a **pool model**:
- Producers add to a per-player pool
- Consumers draw from the pool
- Conduits define coverage zones, not transport routes
- No routing simulation

Transport is fundamentally different - it's a **network model**:
- Pathways form a physical connectivity graph
- Traffic flows along actual paths between points
- Congestion occurs at specific locations (not globally)
- Connectivity matters: building A can only access building B if a path exists

### 3.2 Why Pool Model Doesn't Work for Transport

| Pool Model (Energy/Fluid) | Network Model (Transport) |
|---------------------------|---------------------------|
| Coverage zone = binary (in/out) | Connectivity = graph traversal |
| Surplus is global (per player) | Congestion is local (per segment) |
| No routing needed | Path finding required |
| Conduits are passive infrastructure | Pathways carry active flow |
| Cross-boundary sharing: NO | Cross-boundary sharing: YES |

### 3.3 Cross-Ownership Connectivity

Per canon `patterns.yaml`:
```yaml
roads:
  connects_across_ownership: true
  description: "Roads connect physically regardless of tile ownership"
  effect: "Traffic flows across all connected roads"
```

This is unique to transport. Energy conduits and fluid pipes do NOT cross ownership boundaries. Roads DO. Implications:

1. **Shared Traffic:** Player A's traffic can flow through Player B's roads
2. **Shared Congestion:** Player B's roads can become congested from Player A's traffic
3. **Cooperative Infrastructure:** Players can build connected road networks
4. **Competitive Griefing Potential:** Player can build roads that route traffic through rival's area

### 3.4 Network vs Coverage Zone Calculation

**Energy/Fluid Coverage (BFS flood-fill):**
```
1. Seed from producers
2. BFS through conduits owned by same player
3. Mark tiles as "covered" within radius
4. Binary result: covered or not
```

**Transport Connectivity (Graph connectivity):**
```
1. Build adjacency graph from all pathways (regardless of owner)
2. Connectivity query: is there a path from A to B?
3. Path finding for traffic routing (aggregate, not per-vehicle)
4. Congestion calculated per-segment based on flow vs capacity
```

### 3.5 Data Flow Comparison

**Energy System Data Flow:**
```
EnergyProducer -> Pool (per player) -> Coverage Zone -> Consumers
               (additive)           (BFS from conduits)  (binary check)
```

**Transport System Data Flow:**
```
Buildings -> Traffic Contribution -> Path to Nearest Junction ->
          (aggregate)             (via connectivity graph)

-> Flow on Pathway Segments -> Congestion Calculation
   (distributed along paths)   (per segment: flow/capacity)
```

---

## 4. ITransportProvider Interface Design

### 4.1 Interface Purpose

ITransportProvider allows other systems (especially BuildingSystem) to query transport accessibility without direct coupling to TransportSystem. This mirrors the IEnergyProvider and IFluidProvider patterns.

### 4.2 Interface Definition

```cpp
class ITransportProvider {
public:
    virtual ~ITransportProvider() = default;

    // === Road Proximity Queries (for BuildingSystem 3-tile rule) ===

    // Check if entity is within 3 tiles of a pathway
    virtual bool is_road_accessible(EntityID entity) const = 0;

    // Check if position is within N tiles of a pathway
    virtual bool is_road_accessible_at(
        int32_t x,
        int32_t y,
        uint8_t max_distance = 3
    ) const = 0;

    // Get distance to nearest pathway (0 = adjacent, 255 = none within range)
    virtual uint8_t get_nearest_road_distance(int32_t x, int32_t y) const = 0;

    // === Connectivity Queries ===

    // Can a path be found from position to any map edge connection?
    virtual bool is_connected_to_network(int32_t x, int32_t y) const = 0;

    // Are two positions connected via the pathway network?
    virtual bool are_connected(
        int32_t x1, int32_t y1,
        int32_t x2, int32_t y2
    ) const = 0;

    // === Traffic Queries (for ContaminationSystem, LandValueSystem) ===

    // Get congestion level at position (0.0 = free, 1.0 = gridlock)
    virtual float get_congestion_at(int32_t x, int32_t y) const = 0;

    // Get traffic volume at position (for pollution calculation)
    virtual uint32_t get_traffic_volume_at(int32_t x, int32_t y) const = 0;

    // === Network State Queries ===

    // Get the network ID for a pathway at position (0 = no pathway)
    virtual uint32_t get_network_id_at(int32_t x, int32_t y) const = 0;
};
```

### 4.3 Stub Implementation (Epic 4 Forward Dependency)

Epic 4 defined a stub that returns "always accessible":

```cpp
class TransportProviderStub : public ITransportProvider {
public:
    bool is_road_accessible(EntityID) const override { return true; }
    bool is_road_accessible_at(int32_t, int32_t, uint8_t) const override { return true; }
    uint8_t get_nearest_road_distance(int32_t, int32_t) const override { return 0; }
    bool is_connected_to_network(int32_t, int32_t) const override { return true; }
    bool are_connected(int32_t, int32_t, int32_t, int32_t) const override { return true; }
    float get_congestion_at(int32_t, int32_t) const override { return 0.0f; }
    uint32_t get_traffic_volume_at(int32_t, int32_t) const override { return 0; }
    uint32_t get_network_id_at(int32_t, int32_t) const override { return 1; }
};
```

### 4.4 Stub Replacement Transition

When Epic 7 replaces the stub with real TransportSystem:

1. **Existing buildings without road access:** Buildings that were placed when stub returned "always accessible" may now fail the 3-tile rule
2. **Mitigation:** Either:
   - (A) Grace period: buildings keep is_accessible=true for N ticks, gradually enforce
   - (B) Grandfather clause: existing buildings are exempt, only new buildings checked
   - (C) Hard cutover: buildings immediately enter Degraded state if no road access

**Recommendation:** Option (A) - 500 tick grace period (25 seconds) with visual warning. This matches the energy/fluid grace period pattern, though longer because road infrastructure is more expensive to build quickly.

### 4.5 ITransportConnectable Interface

From canon interfaces.yaml, buildings implement this interface:

```cpp
class ITransportConnectable {
public:
    // Get distance to nearest pathway (cached in TrafficComponent)
    virtual uint32_t get_nearest_road_distance() const = 0;

    // True if within required road proximity (3 tiles)
    virtual bool is_road_accessible() const = 0;

    // Traffic this entity adds to nearby roads
    virtual uint32_t get_traffic_contribution() const = 0;
};
```

BuildingSystem implements this for buildings. TransportSystem queries buildings to aggregate traffic contribution.

---

## 5. Traffic Simulation Approach

### 5.1 Design Decision: Aggregate Flow, Not Individual Vehicles

**Recommendation: Aggregate flow simulation**

We should NOT simulate individual vehicles because:

1. **Scale:** 10,000+ buildings generating traffic = potentially millions of vehicle entities
2. **Performance:** Per-vehicle pathfinding every tick is prohibitively expensive
3. **Network sync:** Syncing vehicle positions would overwhelm bandwidth
4. **SimCity 2000 precedent:** Original game used aggregate traffic, not individual vehicles

**Aggregate flow model:**
- Each building contributes traffic units based on zone type/density
- Traffic flows through pathway network as aggregate values
- Congestion = sum(flow) / capacity per segment
- No individual vehicle entities in simulation

### 5.2 Traffic Flow Algorithm

```
Each tick:

1. AGGREGATE BUILDING TRAFFIC
   For each building with TrafficComponent:
     traffic_contribution = base_traffic * population_factor * employment_factor
     Add to nearest_pathway_segment.inbound_flow

2. DISTRIBUTE FLOW THROUGH NETWORK
   For each pathway segment (topologically sorted from sources):
     // Simple flow propagation - not full fluid dynamics
     outbound_flow = min(inbound_flow, capacity)
     excess_flow = inbound_flow - outbound_flow  // backs up, increases congestion

     // Distribute to connected segments
     For each connected segment:
       connected.inbound_flow += outbound_flow / connected_count

3. CALCULATE CONGESTION
   For each pathway segment:
     congestion = flow / capacity  // 0.0 to 1.0+
     // > 1.0 means flow exceeds capacity (gridlock)

4. CACHE RESULTS FOR QUERIES
   Update PathwayGrid with congestion values
   Update TrafficComponent.is_accessible for buildings
```

### 5.3 Traffic Generation Rates

| Zone Type | Density | Base Traffic |
|-----------|---------|--------------|
| Habitation | Low | 5 |
| Habitation | High | 20 |
| Exchange | Low | 15 |
| Exchange | High | 60 |
| Fabrication | Low | 10 |
| Fabrication | High | 40 |
| Service Building | - | 10-30 (varies) |

These values are design parameters to be tuned. Exchange generates most traffic (customers visiting). Fabrication generates truck traffic. Habitation generates commuter traffic.

### 5.4 Transit Coverage Effects

Rail terminals reduce road traffic in radius:

- Subterra_terminal: 10% traffic reduction in 10-tile radius
- Rail_terminal: 5% traffic reduction in 8-tile radius (less convenient than subway)
- Pod_hub (bus depot): 50% traffic reduction in small radius (per epic plan, but may be tuned)

Traffic reduction is multiplicative:
```
effective_traffic = base_traffic * (1.0 - coverage_reduction)
```

### 5.5 Cosmetic Vehicles (Separate Concern)

Per canon patterns.yaml:
```yaml
cosmetic_entities:
  description: "3D animated entities providing visual feedback for colony activity"
  behavior: "Automated, cosmetic - NOT player controlled"
  types:
    - "Vehicles on roads"
```

Cosmetic vehicles are:
- Rendered by RenderingSystem, NOT simulated by TransportSystem
- Spawned based on traffic density (denser traffic = more vehicles)
- Follow pre-computed paths or simple steering behaviors
- NOT synced across network (each client generates locally)
- Deterministic from traffic density + tile hash for visual consistency

TransportSystem provides:
- `get_traffic_density_at(x, y)` for RenderingSystem to decide how many vehicles to spawn

---

## 6. Tick Priority Recommendation

### 6.1 Current Tick Order (from interfaces.yaml)

```
Priority 5:  TerrainSystem
Priority 10: EnergySystem
Priority 20: FluidSystem
Priority 30: ZoneSystem
Priority 40: BuildingSystem
Priority 50: PopulationSystem
Priority 60: EconomySystem
Priority 70: DisorderSystem
Priority 80: ContaminationSystem
```

### 6.2 TransportSystem Placement Analysis

TransportSystem has these dependencies:
- **Depends on BuildingSystem:** Traffic generated from buildings
- **Used by ContaminationSystem:** Traffic pollution query
- **Used by LandValueSystem (Epic 10):** Traffic affects land value

Options:

| Priority | Pros | Cons |
|----------|------|------|
| 25 (before ZoneSystem) | N/A | Can't query building traffic yet |
| 35 (after ZoneSystem, before Building) | N/A | Can't query building traffic yet |
| **45 (after BuildingSystem)** | Buildings settled, can aggregate traffic | ContaminationSystem sees current-tick traffic |
| 55 (after PopulationSystem) | Population data available | Contamination uses stale traffic data |

### 6.3 Recommendation: Priority 45

```
Priority 40: BuildingSystem
Priority 45: TransportSystem  <-- NEW
Priority 50: PopulationSystem
...
Priority 80: ContaminationSystem  // can query current traffic
```

**Rationale:**
1. Buildings must exist before we aggregate their traffic
2. ContaminationSystem (priority 80) needs current-tick traffic data for pollution
3. PopulationSystem doesn't need transport data (commute calculation is Epic 10)

### 6.4 RailSystem Priority

RailSystem should run after TransportSystem to apply traffic reduction:

```
Priority 45: TransportSystem
Priority 47: RailSystem  <-- NEW
```

Or they could be combined into a single tick at priority 45 if the systems are tightly coupled.

---

## 7. Dense Grid Considerations

### 7.1 Current Dense Grid Exception Pattern

From canon patterns.yaml:
```yaml
dense_grid_exception:
  applies_to:
    - "TerrainGrid (Epic 3): 4 bytes/tile"
    - "BuildingGrid (Epic 4): 4 bytes/tile EntityID array"
    - "EnergyCoverageGrid (Epic 5): 1 byte/tile"
    - "FluidCoverageGrid (Epic 6): 1 byte/tile"
    - "Future: contamination grid, land value grid, pathfinding grid"
```

### 7.2 Pathway Grid Proposal

**Recommendation: Add PathwayGrid (1-2 bytes/tile)**

Structure:
```cpp
struct PathwayTileData {
    uint8_t pathway_type : 3;    // 0=none, 1=road, 2=highway, 3=bridge, 4=tunnel
    uint8_t flow_level : 4;      // 0-15 normalized traffic level (for rendering)
    uint8_t has_junction : 1;    // is this tile a junction
    uint8_t congestion_level;    // 0-255 (256 levels for overlay rendering)
};
// Total: 2 bytes/tile
```

**Justification:**
1. Pathways need O(1) lookup for proximity queries (3-tile rule)
2. Traffic density visualization needs per-tile congestion data
3. Sparse pathways but dense queries - grid is more efficient than scanning all pathway entities
4. Cache locality for flood-fill connectivity algorithms

### 7.3 Memory Budget

| Grid | Size per tile | 256x256 | 512x512 |
|------|---------------|---------|---------|
| TerrainGrid | 4 bytes | 256 KB | 1 MB |
| BuildingGrid | 4 bytes | 256 KB | 1 MB |
| EnergyCoverageGrid | 1 byte | 64 KB | 256 KB |
| FluidCoverageGrid | 1 byte | 64 KB | 256 KB |
| **PathwayGrid** | 2 bytes | 128 KB | 512 KB |
| **Total with PathwayGrid** | 12 bytes | 768 KB | 3 MB |

512 KB for PathwayGrid at largest map size is acceptable.

### 7.4 TrafficDensityGrid vs Inline in PathwayGrid

Alternative: Separate TrafficDensityGrid for congestion data.

**Recommendation: Inline in PathwayGrid** - Congestion is always paired with pathway existence. Separating them would require two lookups for every overlay render.

### 7.5 Canon Update Required

Add to `patterns.yaml` dense_grid_exception.applies_to:
```yaml
- "PathwayGrid (Epic 7): 2 bytes/tile for pathway type, junction flag, and traffic density"
```

---

## 8. Multiplayer Implications

### 8.1 Cross-Ownership Road Connectivity

This is the most significant multiplayer implication. Per canon:
- Roads connect physically regardless of tile ownership
- Traffic flows across all connected roads
- Congestion is shared across connected network segments

**Scenarios:**

**Scenario A: Cooperative road building**
```
Player 1 builds: A---B
Player 2 builds:     B---C
Result: Connected network A-B-C, traffic can flow end to end
```

**Scenario B: Traffic spillover**
```
Player 1's dense city generates 1000 traffic units
Player 2's road network is the only path to map edge
Result: Player 2's roads experience congestion from Player 1's traffic
```

**Scenario C: Infrastructure sabotage**
```
Player 1 demolishes a critical road segment
Result: Player 2's buildings may lose road connectivity
Mitigation: Players can only demolish roads on their own tiles
```

### 8.2 Authority Model

| Action | Authority | Notes |
|--------|-----------|-------|
| Pathway placement | Server | Validates terrain, ownership, cost |
| Pathway demolition | Server | Only owner can demolish |
| Traffic calculation | Server | Aggregate flow is authoritative |
| Congestion values | Server | Synced to all clients |
| Connectivity queries | Server + Client | Both compute (deterministic) |

### 8.3 Sync Requirements

| Data | Sync Method | Frequency |
|------|-------------|-----------|
| PathwayComponent creation | Delta | On placement |
| PathwayComponent removal | Delta | On demolition |
| Traffic flow per segment | Periodic | Every 10-20 ticks |
| Congestion values | Periodic | Every 10-20 ticks |
| Network connectivity | On change | When topology changes |

**Traffic sync optimization:** Don't sync every tick. Traffic changes gradually; 10-20 tick (0.5-1s) sync interval is sufficient. Interpolate client-side.

### 8.4 Visibility

All transport data is visible to all players:
- Road networks are visible (shared infrastructure)
- Traffic congestion is visible (helps planning)
- Other players' construction is visible (immediate)

### 8.5 Late Join / Reconnection

On join/reconnect, client receives:
1. Full PathwayGrid state (which tiles have roads)
2. Current traffic flow values
3. Network connectivity graph (or recomputes locally)

Client can recompute connectivity graph from PathwayGrid - no need to sync the graph itself.

---

## 9. Integration Points

### 9.1 BuildingSystem Integration (3-Tile Rule)

**Query:** BuildingSystem calls `ITransportProvider.is_road_accessible(entity)` every tick for each building.

**Optimization:** Cache result in TrafficComponent.is_accessible, only recompute when:
- Building is constructed
- Nearby pathway is placed/demolished (dirty flag)

**State Machine Impact:**
```
BuildingState transitions:
  Active -> Degraded: is_road_accessible becomes false, 500 tick grace period
  Degraded -> Abandoned: grace period expires
  Degraded -> Active: road built within grace period
```

### 9.2 ContaminationSystem Integration (Traffic Pollution)

**Query:** ContaminationSystem calls `ITransportProvider.get_traffic_volume_at(x, y)` to calculate traffic pollution.

**Pollution formula:**
```cpp
contamination_output = traffic_volume * TRAFFIC_POLLUTION_FACTOR;
// Only for tiles with pathways; traffic pollution is localized to roads
```

**Pollution type:** ContaminationType::Traffic (defined in canon interfaces.yaml)

### 9.3 LandValueSystem Integration (Epic 10)

**Queries:**
- `get_congestion_at(x, y)` - congestion reduces land value
- `get_nearest_road_distance(x, y)` - buildings too far from roads have lower value

**Land value effects:**
- Heavy traffic (congestion > 0.7): -20% land value
- No road access (distance > 3): -50% land value
- Adjacent to transit_corridor: -10% (noise) OR +5% (commercial access) - design decision needed

### 9.4 DemandSystem Integration (Epic 10)

**Query:** DemandSystem considers transport accessibility for zone demand:
- Zones without road access have reduced development pressure
- High congestion reduces commercial demand (customers avoid gridlock)

### 9.5 PortSystem Integration (Epic 8)

**Query:** PortSystem checks road connectivity to ports:
- Aero ports require road connection for passenger access
- Aqua ports require road connection for cargo transport
- Trade income affected by congestion on route to port

### 9.6 Data Flow Diagram

```
                              BuildingSystem (Epic 4)
                                    |
                    (TrafficComponent: traffic_contribution)
                                    |
                                    v
+---------------+           +------------------+
| TerrainSystem | --------> | TransportSystem  |
| (placement)   |           |                  |
+---------------+           | - Pathway graph  |
                            | - Flow aggregate |
                            | - Congestion     |
                            +--------+---------+
                                     |
        +-------------+--------------+--------------+
        |             |              |              |
        v             v              v              v
  BuildingSystem  ContaminationSys  LandValueSys  DemandSystem
  (3-tile rule)   (traffic poll.)  (congestion)  (accessibility)
```

---

## 10. Key Work Items

### 10.1 TransportSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| T-01 | PathwayComponent definition | S | Define PathwayType enum and PathwayComponent struct |
| T-02 | JunctionComponent definition | S | Define JunctionComponent for intersections |
| T-03 | TrafficComponent definition | S | Define TrafficComponent for buildings |
| T-04 | PathwayGrid implementation | M | Dense 2-byte/tile grid for pathway data |
| T-05 | Pathway placement validation | M | Check terrain, ownership, connectivity |
| T-06 | Pathway placement action | M | Create pathway entity, update PathwayGrid, recalc connectivity |
| T-07 | Pathway demolition action | M | Remove pathway, update grid, recalc connectivity |
| T-08 | Network connectivity graph | L | Build and maintain adjacency graph for connected pathways |
| T-09 | ITransportProvider implementation | L | Implement all queries (proximity, connectivity, traffic) |
| T-10 | Traffic aggregation algorithm | L | Aggregate building traffic, distribute through network |
| T-11 | Congestion calculation | M | Calculate flow/capacity per segment |
| T-12 | Transit corridor (highway) support | M | Higher capacity pathway type |
| T-13 | Bridge pathway support | M | Pathway over water tiles |
| T-14 | Tunnel pathway support | M | Pathway through terrain |
| T-15 | Pathway decay system | M | Degradation over time, affects capacity |
| T-16 | ISimulatable (tick priority 45) | S | Register TransportSystem with simulation |
| T-17 | Network messages | M | PathwayPlaceRequest, PathwayDemolishRequest, traffic sync |
| T-18 | PathwayComponent serialization | S | ISerializable implementation |

### 10.2 RailSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| R-01 | RailComponent definition | S | Define RailType enum and RailComponent |
| R-02 | RailTerminalComponent definition | S | Define terminal component with coverage |
| R-03 | Rail track placement | M | Surface rail track placement and validation |
| R-04 | Rail terminal placement | M | Terminal placement with coverage radius |
| R-05 | Subterra layer support | L | Underground rail network (separate layer) |
| R-06 | Subterra terminal placement | M | Subway station placement |
| R-07 | Transit coverage calculation | M | Calculate traffic reduction in radius |
| R-08 | Rail power dependency | M | Rails require energy to operate |
| R-09 | Rail decay system | M | Degradation from underfunding |
| R-10 | ISimulatable (tick priority 47) | S | Register RailSystem with simulation |
| R-11 | Rail network messages | M | Placement, demolition sync |
| R-12 | RailComponent serialization | S | ISerializable implementation |

### 10.3 Shared Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| S-01 | PathwayPlacedEvent, PathwayDemolishedEvent | S | Event definitions |
| S-02 | RailPlacedEvent, RailDemolishedEvent | S | Event definitions |
| S-03 | Integration test: building road proximity | L | End-to-end test of 3-tile rule |
| S-04 | Integration test: traffic pollution | M | ContaminationSystem traffic queries |
| S-05 | Stub replacement transition | M | Handle existing buildings without road access |

### 10.4 Work Item Size Summary

| Size | Count | Items |
|------|-------|-------|
| S | 9 | T-01, T-02, T-03, T-16, T-18, R-01, R-02, R-10, R-12 |
| M | 16 | T-04, T-05, T-06, T-07, T-11, T-12, T-13, T-14, T-15, T-17, R-03, R-04, R-06, R-07, R-08, R-09 |
| L | 5 | T-08, T-09, T-10, R-05, R-11 |
| XL | 0 | - |
| **Total** | **30** | Plus S-01 to S-05 = **35 work items** |

---

## 11. Questions for Other Agents

### @game-designer

1. **Traffic spillover policy:** When Player A's traffic uses Player B's roads, should there be any cost compensation, or is this just part of competitive multiplayer dynamics? Should players be able to "toll" roads on their tiles?

2. **Transit corridor land value effect:** Should properties adjacent to transit_corridors (highways) have reduced value (noise) or increased value (access)? Or zone-dependent (fabrication likes access, habitation dislikes noise)?

3. **Road decay rate:** How aggressively should roads decay from traffic and underfunding? Should high-traffic roads decay faster than low-traffic roads?

4. **Bus system inclusion:** The epic plan mentions bus depots with 50% traffic reduction. Should this be in Epic 7 or deferred? Buses seem simpler than rail but the 50% reduction is very powerful.

5. **Traffic visualization:** Should we show cosmetic vehicles immediately in Epic 7, or defer to Epic 17 (Polish)? Cosmetic vehicles are separate from traffic simulation but provide important visual feedback.

6. **Congestion threshold for effects:** At what congestion level should negative effects kick in? Proposed: 0.5 = minor slowdown, 0.7 = significant impact, 0.9 = gridlock.

### @infrastructure-engineer / @services-engineer

7. **Connectivity graph implementation:** Should we use a proper graph library (boost::graph) or implement a simple adjacency list? The graph operations needed are basic (BFS for connectivity, no Dijkstra needed for aggregate flow).

8. **PathwayGrid dirty tracking:** Should we use a dirty chunk system (like terrain) or a dirty entity list? Pathway changes are less frequent than terrain modifications.

9. **Traffic sync frequency:** Is 10-20 tick sync interval acceptable for multiplayer traffic display, or do we need faster updates for responsive overlay rendering?

10. **Subterra layer implementation:** Should the underground layer be a separate PathwayGrid, or can we pack surface + subterra into a single grid with layer flag?

### @graphics-engineer

11. **Traffic density visualization:** How should congestion be rendered on the pathway overlay? Color gradient (green-yellow-red)? Line thickness? Animated flow particles?

12. **Cosmetic vehicle system:** When we implement cosmetic vehicles, should they be instanced rendering, individual entities, or particle-like effects? This affects how TransportSystem exposes traffic density data.

---

## 12. Risks and Concerns

### 12.1 Architectural Risks

**RISK: Cross-Ownership Complexity (HIGH)**
Roads connecting across ownership is unique among systems. This creates shared state that doesn't fit the per-player pool model used elsewhere. Traffic from one player affects another player's infrastructure.

**Mitigation:**
- Clear documentation of cross-ownership behavior
- Careful network sync of shared road state
- Consider "road treaty" system for multiplayer (future feature)

**RISK: Stub Transition Stranding Buildings (MEDIUM)**
Buildings placed during Epic 4-6 (when stub returns "always accessible") may suddenly lose access when real TransportSystem activates.

**Mitigation:**
- 500-tick grace period with visual warning
- Or grandfather existing buildings
- Clear transition documentation

**RISK: Traffic Calculation Performance (MEDIUM)**
10,000+ buildings generating traffic, flowing through potentially complex road networks.

**Mitigation:**
- Aggregate flow, not per-vehicle simulation
- Update traffic less frequently than other systems (every 5-10 ticks)
- Cache connectivity results; only recompute on topology change
- Profile early with realistic building counts

### 12.2 Design Ambiguities

**AMBIGUITY: Traffic Distribution Algorithm**
How exactly does aggregate traffic flow through the network? Options:
- (A) Simple: traffic goes to nearest junction, spreads evenly
- (B) Gravity: traffic attracted to destinations (exchange zones, ports)
- (C) Shortest-path: aggregate paths computed between zone centroids

**Recommendation:** Start with (A) for MVP. Gravity model adds complexity without clear gameplay benefit. Shortest-path is computationally expensive.

**AMBIGUITY: Rail Traffic Interaction**
How does rail ridership interact with road traffic?
- Do we simulate beings choosing between road and rail?
- Or is rail coverage a simple percentage reduction?

**Recommendation:** Simple percentage reduction for MVP. Transit terminals reduce road traffic in radius by fixed percentage. No mode choice simulation.

**AMBIGUITY: Pathway Capacity Meaning**
What does pathway capacity represent?
- Vehicles per tick? Too abstract.
- Traffic units (same as building contribution)? Consistent with aggregate model.

**Recommendation:** Capacity is in same units as traffic contribution. A capacity-100 road can handle traffic from 20 low-density habitations (5 each).

### 12.3 Technical Debt

**DEBT: Hardcoded Traffic Values**
Initial traffic generation rates will be hardcoded. Should be data-driven for tuning.

**DEBT: No Mode Choice**
Beings don't choose between road and rail. Future improvement: beings near terminals prefer rail, reducing traffic more dynamically.

**DEBT: Simplified Congestion**
Congestion is flow/capacity without spillback or queuing dynamics. Good enough for city builder, not realistic traffic simulation.

---

## 13. Dependencies

### 13.1 What Epic 7 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 3 | ITerrainQueryable | Pathway placement validation |
| Epic 4 | BuildingSystem, TrafficComponent | Traffic generation from buildings |
| Epic 4 | ITransportProvider stub | Replace with real implementation |
| Epic 5 | IEnergyProvider | Rail requires power to operate |

### 13.2 What Later Epics Need from Epic 7

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 8 (Ports) | Road connectivity to ports | ITransportProvider.are_connected() |
| Epic 10 (Simulation) | Traffic for demand calculation | ITransportProvider queries |
| Epic 10 (LandValue) | Congestion values | ITransportProvider.get_congestion_at() |
| Epic 10 (Contamination) | Traffic volume for pollution | ITransportProvider.get_traffic_volume_at() |

### 13.3 Canon Updates Required

1. **interfaces.yaml:** Add ITransportProvider interface definition
2. **interfaces.yaml:** Update ISimulatable with TransportSystem priority 45, RailSystem priority 47
3. **patterns.yaml:** Add PathwayGrid to dense_grid_exception.applies_to
4. **systems.yaml:** Add tick_priority to TransportSystem and RailSystem entries

---

## Appendix A: Simulation Tick Order (Epic 7 Context)

```
Priority 5:  TerrainSystem        -- terrain data available
Priority 10: EnergySystem         -- power state calculated
Priority 20: FluidSystem          -- fluid state calculated
Priority 30: ZoneSystem           -- zones updated
Priority 40: BuildingSystem       -- buildings updated, TrafficComponent set
Priority 45: TransportSystem      -- traffic aggregated, congestion calculated  <-- NEW
Priority 47: RailSystem           -- transit coverage applied                   <-- NEW
Priority 50: PopulationSystem
Priority 60: EconomySystem
Priority 70: DisorderSystem
Priority 80: ContaminationSystem  -- queries traffic for pollution
```

---

## Appendix B: Entity Composition Examples

**Pathway segment:**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 75 }
    OwnershipComponent { owner: PLAYER_1 }
    PathwayComponent { pathway_type: Road, capacity: 100, current_flow: 45, network_id: 1 }
}
```

**Junction (4-way intersection):**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 76 }
    OwnershipComponent { owner: PLAYER_1 }
    PathwayComponent { pathway_type: Road, capacity: 150, current_flow: 120, network_id: 1 }
    JunctionComponent { connected_count: 4, has_traffic_control: true }
}
```

**Rail terminal:**
```
Entity {
    PositionComponent { grid_x: 60, grid_y: 80 }
    OwnershipComponent { owner: PLAYER_2 }
    RailTerminalComponent { terminal_type: Subterra, coverage_radius: 10,
                            traffic_reduction_percent: 10, is_operational: true }
    EnergyComponent { energy_required: 30, is_powered: true }
}
```

**Building with traffic:**
```
Entity {
    PositionComponent { grid_x: 52, grid_y: 77 }
    OwnershipComponent { owner: PLAYER_1 }
    BuildingComponent { ... }
    TrafficComponent { traffic_contribution: 15, nearest_pathway_id: 1234,
                       pathway_distance: 1, is_accessible: true }
}
```

---

## Appendix C: Forward Dependency Stub Replacement Checklist

| Stub | Introduced | Replaced By | Transition Notes |
|------|-----------|-------------|------------------|
| ITransportQuery (always accessible) | Epic 4, B-15 | Epic 7 (TransportSystem) | 500 tick grace period for existing buildings |

**Transition Steps:**
1. Implement ITransportProvider in TransportSystem
2. Register via dependency injection to replace stub
3. On first tick after activation, mark all buildings with road distance > 3 as "grace period"
4. After 500 ticks, buildings without road access transition to Degraded state
5. Visual indicator during grace period (flashing accessibility warning)
