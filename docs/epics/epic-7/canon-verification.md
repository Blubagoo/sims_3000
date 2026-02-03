# Epic 7: Canon Verification

**Epic:** 7 - Transportation Network
**Canon Version:** 0.8.0
**Date:** 2026-01-29
**Status:** VERIFIED - Canon updates required

---

## Verification Summary

| Category | Status | Notes |
|----------|--------|-------|
| Terminology | PASS | Uses pathway, flow, flow_blockage, junction, transit_corridor |
| Interfaces | UPDATE REQUIRED | ITransportProvider must be added |
| Systems | UPDATE REQUIRED | tick_priority missing for Transport/Rail |
| Patterns | UPDATE REQUIRED | PathwayGrid/ProximityCache for dense_grid_exception |
| Component Design | PASS | Follows ECS patterns, components are pure data |
| Multiplayer | PASS | Cross-ownership per canon roads pattern |

---

## Terminology Verification

### Verified Canonical Terms Used

| Human Term | Canonical Term | Status |
|------------|----------------|--------|
| Road | Pathway | PASS - Used in all tickets |
| Highway | Transit Corridor | PASS - PathwayType::TransitCorridor |
| Traffic | Flow | PASS - flow_current, flow_previous |
| Congestion | Flow Blockage | PASS - flow_blockage_ticks |
| Intersection | Junction | PASS - is_junction flag |
| Railroad | Rail Line | PASS - RailType, RailComponent |
| Train | Rail Transport | PASS (cosmetic only) |
| Train Station | Rail Terminal | PASS - TerminalComponent |
| Subway | Subterra Rail | PASS - RailType::SubterraRail |
| Subway Station | Subterra Terminal | PASS - TerminalType::SubterraStation |

All tickets use canonical alien terminology. No human terms leaked.

---

## Interface Verification

### ITransportConnectable (Canon: interfaces.yaml)

**Canon Definition:**
```yaml
ITransportConnectable:
  methods:
    - get_nearest_road_distance() -> uint32
    - is_road_accessible() -> bool
    - get_traffic_contribution() -> uint32
  implemented_by:
    - All zone buildings
    - Service buildings
    - Energy nexuses
```

**Tickets Coverage:**
- E7-016: ITransportProvider interface includes these methods
- E7-017, E7-018: Implementation of accessibility methods
- E7-019: BuildingSystem stub replacement

**Status:** PASS - Interface implemented, stub replacement planned

### ITransportProvider (NEW - Not in canon)

**Proposed Interface (from CCR-003):**
```yaml
ITransportProvider:
  description: "Provides transport state queries for other systems"
  purpose: "Allows BuildingSystem and other consumers to query transport state without direct TransportSystem coupling"
  methods:
    - is_road_accessible(entity) -> bool
    - is_road_accessible_at(x, y, max_distance) -> bool
    - get_nearest_road_distance(x, y) -> uint8
    - is_connected_to_network(x, y) -> bool
    - are_connected(x1, y1, x2, y2) -> bool
    - get_congestion_at(x, y) -> float
    - get_traffic_volume_at(x, y) -> uint32
    - get_network_id_at(x, y) -> uint32
  implemented_by:
    - TransportSystem (Epic 7)
```

**Status:** UPDATE REQUIRED - Add ITransportProvider to interfaces.yaml

### ISimulatable (Canon: interfaces.yaml)

**Current Canon:**
```yaml
implemented_by:
  - TerrainSystem (priority: 5)
  - EnergySystem (priority: 10)
  - FluidSystem (priority: 20)
  - ZoneSystem (priority: 30)
  - BuildingSystem (priority: 40)
  - PopulationSystem (priority: 50)
  - EconomySystem (priority: 60)
  - DisorderSystem (priority: 70)
  - ContaminationSystem (priority: 80)
```

**Missing (from CCR-004):**
- TransportSystem (priority: 45)
- RailSystem (priority: 47)

**Status:** UPDATE REQUIRED - Add TransportSystem and RailSystem to ISimulatable implementers

---

## Systems Verification

### TransportSystem (Canon: systems.yaml)

**Current Canon Definition:**
```yaml
TransportSystem:
  type: ecs_system
  owns:
    - Road/pathway placement and connectivity
    - Traffic simulation
    - Congestion calculation
    - Road decay
  does_not_own:
    - Rail (RailSystem owns)
    - Road rendering (RenderingSystem owns)
    - Road costs (EconomySystem owns)
  provides:
    - Connectivity queries
    - Traffic density queries
    - Path finding
  depends_on:
    - TerrainSystem (valid placement)
  components:
    - RoadComponent
    - TrafficComponent
```

**Missing from Canon:**
- `tick_priority: 45` field
- `multiplayer` section (should mirror Energy/Fluid pattern)

**Tickets Coverage:**
- E7-022: TransportSystem class with priority 45
- E7-002: RoadComponent
- E7-003: TrafficComponent
- All `owns` items covered

**Status:** UPDATE REQUIRED - Add tick_priority: 45 to systems.yaml

### RailSystem (Canon: systems.yaml)

**Current Canon Definition:**
```yaml
RailSystem:
  type: ecs_system
  owns:
    - Rail track placement
    - Rail terminals
    - Subterra (subway) network
    - Rail capacity
  does_not_own:
    - Roads (TransportSystem owns)
  provides:
    - Rail connectivity
    - Transit coverage
  depends_on:
    - TerrainSystem
    - EnergySystem (rails need power)
  components:
    - RailComponent
    - SubterraComponent
```

**Missing from Canon:**
- `tick_priority: 47` field
- `TerminalComponent` in components list

**Tickets Coverage:**
- E7-032: RailSystem class with priority 47
- E7-030: RailComponent
- E7-031: TerminalComponent
- E7-043: SubterraComponent (P2)

**Status:** UPDATE REQUIRED - Add tick_priority: 47 and TerminalComponent to systems.yaml

---

## Patterns Verification

### Dense Grid Exception (Canon: patterns.yaml)

**Current Canon:**
```yaml
dense_grid_exception:
  applies_to:
    - "TerrainGrid (Epic 3): 4 bytes/tile"
    - "BuildingGrid (Epic 4): 4 bytes/tile EntityID array"
    - "EnergyCoverageGrid (Epic 5): 1 byte/tile"
    - "FluidCoverageGrid (Epic 6): 1 byte/tile"
    - "Future: contamination grid, land value grid, pathfinding grid"
```

**Missing (from CCR-005):**
- PathwayGrid (Epic 7): 4 bytes/tile EntityID array
- ProximityCache (Epic 7): 1 byte/tile distance cache

**Tickets Coverage:**
- E7-005: PathwayGrid (4 bytes/tile)
- E7-006: ProximityCache (1 byte/tile)

**Status:** UPDATE REQUIRED - Add PathwayGrid and ProximityCache to dense_grid_exception.applies_to

### Infrastructure Connectivity (Canon: patterns.yaml)

**Canon Definition:**
```yaml
infrastructure_connectivity:
  roads:
    connects_across_ownership: true
    description: "Roads connect physically regardless of tile ownership"
    effect: "Traffic flows across all connected roads"
    cosmetic_entities: "Cosmetic beings/vehicles move along roads"
```

**Tickets Coverage:**
- E7-020: Cross-ownership connectivity in NetworkGraph
- CCR-002: Cross-ownership resolution documented

**Status:** PASS - Tickets align with canon pattern

### Pool Model vs Network Model

**Canon Definition (patterns.yaml resources.pool_model):**
```yaml
pool_model:
  description: "Each player has resource pools. Generators add to pool, consumers draw from pool."
```

**Epic 7 Decision (CCR-001):**
Transport uses NETWORK model, NOT pool model. This is correct per canon - the pool model is explicitly for electricity and water, while roads use physical connectivity.

**Status:** PASS - Network model is correct for transport

---

## Component Design Verification

### RoadComponent (E7-002)

**ECS Compliance Check:**

| Rule | Status |
|------|--------|
| Pure data, no logic | PASS |
| Trivially copyable | PASS |
| No pointers | PASS |
| Uses entity IDs for refs | PASS (network_id) |
| Field names snake_case | PASS |

**Size:** 16 bytes (within reasonable bounds)

### TrafficComponent (E7-003)

**ECS Compliance Check:**

| Rule | Status |
|------|--------|
| Pure data, no logic | PASS |
| Trivially copyable | PASS |
| No pointers | PASS |
| Sparse attachment pattern | PASS (only for pathways with flow) |

**Size:** 16 bytes

### RailComponent (E7-030)

**ECS Compliance Check:**

| Rule | Status |
|------|--------|
| Pure data, no logic | PASS |
| Trivially copyable | PASS |
| No pointers | PASS |
| is_powered/is_active flags | PASS (EnergySystem integration) |

**Size:** 12 bytes

---

## Multiplayer Verification

### Cross-Ownership Roads (Canon Pattern)

**Canon:** `roads.connects_across_ownership: true`

**Tickets:**
- E7-020: Cross-ownership connectivity in NetworkGraph
- E7-013: Flow distribution crosses ownership boundaries
- E7-021: Per-owner maintenance cost (each player pays for own segments)

**Status:** PASS - Fully aligned

### Server Authority

**Canon:** "All simulation logic (ECS systems)" is server-authoritative

**Tickets:**
- E7-038: Network messages for pathway operations (server validates)
- E7-039: Rail network messages (server authority)
- E7-036, E7-037: Component serialization for sync

**Status:** PASS - Server-authoritative pattern followed

---

## Canon Updates Required

### 1. interfaces.yaml

**Add ITransportProvider interface:**

```yaml
transport:
  ITransportProvider:
    description: "Provides transport state queries for other systems"
    purpose: "Allows BuildingSystem and other consumers to query transport state without direct TransportSystem coupling"

    methods:
      - name: is_road_accessible
        params:
          - name: entity
            type: EntityID
        returns: bool
        description: "Whether entity is within 3 tiles of a pathway"

      - name: is_road_accessible_at
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
          - name: max_distance
            type: uint8_t
            description: "Maximum distance (default 3)"
        returns: bool
        description: "Whether position is within max_distance tiles of a pathway"

      - name: get_nearest_road_distance
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: uint8_t
        description: "Distance to nearest pathway (0 if adjacent, 255 if none)"

      - name: is_connected_to_network
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: bool
        description: "Whether pathway at position is part of a connected network"

      - name: are_connected
        params:
          - name: x1
            type: int32_t
          - name: y1
            type: int32_t
          - name: x2
            type: int32_t
          - name: y2
            type: int32_t
        returns: bool
        description: "Whether two positions are connected via the pathway network"

      - name: get_congestion_at
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: float
        description: "Congestion level at position (0.0 = free, 1.0 = gridlock)"

      - name: get_traffic_volume_at
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: uint32_t
        description: "Traffic volume at position (for pollution calculation)"

      - name: get_network_id_at
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: uint32_t
        description: "Network ID for pathway at position (0 = no pathway)"

    implemented_by:
      - TransportSystem (Epic 7)

    notes:
      - "Replaces Epic 4 stub"
      - "Mirrors IEnergyProvider/IFluidProvider pattern for consistency"
```

**Update ISimulatable implementers:**

```yaml
implemented_by:
  - TerrainSystem (priority: 5)
  - EnergySystem (priority: 10)
  - FluidSystem (priority: 20)
  - ZoneSystem (priority: 30)
  - BuildingSystem (priority: 40)
  - TransportSystem (priority: 45)   # NEW
  - RailSystem (priority: 47)        # NEW
  - PopulationSystem (priority: 50)
  - EconomySystem (priority: 60)
  - DisorderSystem (priority: 70)
  - ContaminationSystem (priority: 80)
```

### 2. systems.yaml

**Update TransportSystem:**

```yaml
TransportSystem:
  type: ecs_system
  tick_priority: 45  # NEW - after BuildingSystem (40)
  owns:
    - Road/pathway placement and connectivity
    - Traffic simulation
    - Congestion calculation
    - Road decay
    - PathwayGrid dense spatial data
    - ProximityCache for 3-tile rule
  ...
```

**Update RailSystem:**

```yaml
RailSystem:
  type: ecs_system
  tick_priority: 47  # NEW - after TransportSystem (45)
  owns:
    - Rail track placement
    - Rail terminals
    - Subterra (subway) network
    - Rail capacity
  ...
  components:
    - RailComponent
    - SubterraComponent
    - TerminalComponent  # NEW
```

### 3. patterns.yaml

**Update dense_grid_exception.applies_to:**

```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile"
  - "BuildingGrid (Epic 4): 4 bytes/tile EntityID array"
  - "EnergyCoverageGrid (Epic 5): 1 byte/tile"
  - "FluidCoverageGrid (Epic 6): 1 byte/tile"
  - "PathwayGrid (Epic 7): 4 bytes/tile EntityID array for pathway spatial queries"  # NEW
  - "ProximityCache (Epic 7): 1 byte/tile distance-to-nearest-pathway cache"  # NEW
  - "Future: contamination grid, land value grid"
```

---

## Version Bump

After applying updates:
- interfaces.yaml: 0.8.0 -> 0.9.0
- systems.yaml: 0.8.0 -> 0.9.0
- patterns.yaml: 0.8.0 -> 0.9.0
- CANON.md: 0.8.0 -> 0.9.0

**Changelog Entry:**
```
| 0.9.0 | 2026-01-29 | Epic 7 canon integration: ITransportProvider interface added, TransportSystem tick_priority: 45, RailSystem tick_priority: 47, PathwayGrid and ProximityCache added to dense_grid_exception, TerminalComponent added to RailSystem |
```

---

## Verification Complete

All 48 tickets have been verified against canon v0.8.0. The tickets are compliant with existing canon. Three canon files require updates to document the new interfaces, tick priorities, and dense grid patterns introduced by Epic 7.

**Next Step:** Apply canon updates (Phase 6).
