# Systems Architect Analysis: Epic 8 - Ports & External Connections

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.13.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 8 implements **PortSystem** - the external connections infrastructure enabling trade, population migration, and demand bonuses. This epic introduces two port zone types (aero port, aqua port), neighbor connections at map edges, and inter-player trade mechanics for multiplayer.

Key architectural characteristics:
1. **Zone-based ports:** Ports are zoned areas (not placed buildings), following ZoneSystem patterns from Epic 4
2. **Demand modifiers:** Aero ports boost exchange (commercial) demand; aqua ports boost fabrication (industrial) demand
3. **Map edge connections:** Pathway/rail connections at map edges enable external trade and population migration
4. **Multiplayer trade:** Inter-player trade routes for resource/credit exchange
5. **Dependency integration:** Heavy integration with Epic 3 (terrain), Epic 4 (zones), Epic 7 (transport), Epic 10 (demand), Epic 11 (economy)

PortSystem is primarily a **demand modifier and economic enabler** - it creates zones that unlock global bonuses, generates trade income, and enables external connectivity. The actual zone mechanics remain in ZoneSystem.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Port Types and Requirements](#3-port-types-and-requirements)
4. [Map Edge Connections](#4-map-edge-connections)
5. [Trade and External Economy](#5-trade-and-external-economy)
6. [Demand Bonus Mechanics](#6-demand-bonus-mechanics)
7. [Multiplayer: Inter-Player Trade](#7-multiplayer-inter-player-trade)
8. [Tick Priority and Data Flow](#8-tick-priority-and-data-flow)
9. [Key Work Items](#10-key-work-items)
10. [Questions for Other Agents](#11-questions-for-other-agents)
11. [Risks and Concerns](#12-risks-and-concerns)
12. [Dependencies](#13-dependencies)
13. [Integration Points](#14-integration-points)
14. [Proposed Interfaces](#15-proposed-interfaces)

---

## 1. System Boundaries

### 1.1 PortSystem

**Type:** ECS System

**Owns:**
- Aero port (airport) zone designation and capacity tracking
- Aqua port (seaport) zone designation and capacity tracking
- External connection points (map edge connections)
- Trade route management (neighbor and inter-player)
- Trade income calculation
- Demand bonus calculation
- Port capacity calculation based on zone size and infrastructure

**Does NOT Own:**
- Zone designation mechanics (ZoneSystem owns - ports are zone types)
- Building spawning within port zones (BuildingSystem owns)
- Terrain validation (TerrainSystem owns - water proximity for aqua ports)
- Pathway connectivity to ports (TransportSystem owns)
- Rail connectivity to ports (RailSystem owns)
- Port zone rendering (RenderingSystem owns)
- Trade income depositing (EconomySystem owns treasury)
- Demand calculation integration (DemandSystem owns - queries PortSystem for bonuses)

**Provides:**
- `IPortProvider` interface for port state queries
- Port capacity queries (aero, aqua)
- Trade income data
- Demand bonus values
- External connection status
- Inter-player trade route information

**Depends On:**
- ZoneSystem (port zone registration)
- TerrainSystem (water proximity validation for aqua ports)
- TransportSystem (pathway connections for port access)
- RailSystem (optional rail connections for cargo)
- EconomySystem (trade income depositing)
- DemandSystem (bonus integration)

### 1.2 Boundary Summary Table

| Concern | PortSystem | ZoneSystem | TransportSystem | EconomySystem | DemandSystem |
|---------|------------|------------|-----------------|---------------|--------------|
| Port zone designation | Tracks | **Owns** | - | - | - |
| Port capacity calculation | **Owns** | - | Contributes | - | - |
| Trade income | **Owns** | - | - | Receives | - |
| Demand bonuses | **Owns** | - | - | - | Queries |
| External connections | **Owns** | - | Provides path | - | - |
| Port rendering | - | - | - | - | - (RenderingSystem) |

---

## 2. Data Model

### 2.1 PortComponent

```cpp
struct PortComponent {
    PortType port_type = PortType::Aero;    // aero_port or aqua_port
    uint32_t capacity = 0;                   // Current port capacity (passengers/cargo)
    uint32_t max_capacity = 0;               // Maximum based on zone size
    uint32_t utilization = 0;                // Current usage level (0-100%)
    bool is_operational = false;             // Has required infrastructure
    bool is_connected_to_edge = false;       // Connected to map edge (external trade)
    uint32_t demand_bonus_radius = 0;        // Radius of demand effect (tiles)
};

enum class PortType : uint8_t {
    Aero = 0,    // aero_port (airport)
    Aqua = 1     // aqua_port (seaport)
};
```

### 2.2 PortZoneComponent (Extends ZoneComponent)

```cpp
struct PortZoneComponent {
    PortType port_type = PortType::Aero;
    GridRect runway_area;                    // For aero: runway placement area
    GridRect dock_area;                      // For aqua: dock/pier placement area
    uint8_t zone_level = 0;                  // Development level (0-3)
    bool has_runway = false;                 // Aero: runway constructed
    bool has_dock = false;                   // Aqua: dock/pier constructed
    uint32_t zone_tiles = 0;                 // Total tiles in zone
};
```

### 2.3 ExternalConnectionComponent

```cpp
struct ExternalConnectionComponent {
    ConnectionType connection_type = ConnectionType::Pathway;
    MapEdge edge_side = MapEdge::North;      // Which map edge
    GridPosition edge_position;               // Position along edge
    bool is_active = false;                   // Connected to external world
    uint32_t trade_capacity = 0;              // Trade units per cycle
    uint32_t migration_capacity = 0;          // Beings per cycle
};

enum class ConnectionType : uint8_t {
    Pathway = 0,     // Road connection to edge
    Rail = 1,        // Rail connection to edge
    Energy = 2,      // Power connection (for future cross-map grids)
    Fluid = 3        // Water connection (for future cross-map sharing)
};

enum class MapEdge : uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3
};
```

### 2.4 TradeRouteComponent

```cpp
struct TradeRouteComponent {
    TradeRouteType route_type = TradeRouteType::External;
    PlayerID source_player = GAME_MASTER;    // Source colony
    PlayerID dest_player = GAME_MASTER;      // Destination (GAME_MASTER = external)
    TradeGoodType good_type = TradeGoodType::Goods;
    uint32_t volume_per_cycle = 0;           // Trade volume
    int64_t income_per_cycle = 0;            // Credits generated
    bool is_active = true;                   // Route is operational
};

enum class TradeRouteType : uint8_t {
    External = 0,    // Trade with virtual neighbors (NPC)
    InterPlayer = 1  // Trade between players
};

enum class TradeGoodType : uint8_t {
    Goods = 0,       // Generic goods (exchange zones)
    Materials = 1,   // Raw materials (fabrication zones)
    Passengers = 2,  // Beings (aero ports primarily)
    Cargo = 3        // Heavy cargo (aqua ports primarily)
};
```

### 2.5 PortCapacityData

```cpp
struct PortCapacityData {
    uint32_t aero_capacity = 0;              // Total aero port capacity
    uint32_t aqua_capacity = 0;              // Total aqua port capacity
    uint32_t external_connections = 0;        // Number of map edge connections
    float aero_utilization = 0.0f;           // Current aero usage (0.0-1.0+)
    float aqua_utilization = 0.0f;           // Current aqua usage (0.0-1.0+)
};
```

---

## 3. Port Types and Requirements

### 3.1 Aero Port (Airport)

**Canonical Term:** aero_port (from terminology.yaml)

**Zone Characteristics:**
- Minimum zone size: 6x6 tiles (36 tiles minimum)
- Optimal zone size: 9x12 tiles (108 tiles) for full runway
- Zone type: Special zone category (not habitation/exchange/fabrication)

**Requirements:**
- **Runway:** Linear clear area within zone (minimum 6 tiles long, 2 tiles wide)
- **Pathway Access:** Must connect to pathway network (ITransportProvider check)
- **Flat Terrain:** Runway area must be flat (single elevation level)

**Capacity Calculation:**
```
base_capacity = zone_tiles * 10
runway_bonus = has_runway ? 1.5 : 0.5
access_bonus = pathway_connected ? 1.0 : 0.0
aero_capacity = base_capacity * runway_bonus * access_bonus
```

**Demand Effects:**
- +15% exchange (commercial) demand globally (colony-wide)
- +5% habitation demand in 20-tile radius (airport jobs)
- +2% fabrication demand in 15-tile radius (cargo handling)

**Negative Effects:**
- Noise contamination in 10-tile radius (reduces sector value)
- High energy consumption

### 3.2 Aqua Port (Seaport)

**Canonical Term:** aqua_port (from terminology.yaml)

**Zone Characteristics:**
- Minimum zone size: 4x8 tiles (32 tiles minimum)
- Optimal zone size: 8x16 tiles (128 tiles) for full dock
- Zone type: Special zone category

**Requirements:**
- **Water Adjacency:** Zone must border water tiles (deep_void, still_basin, or flow_channel)
- **Dock Area:** Waterfront tiles must be designated dock (minimum 4 tiles)
- **Pathway Access:** Must connect to pathway network
- **Rail Access (Optional):** Rail connection increases cargo capacity by 50%

**Capacity Calculation:**
```
base_capacity = zone_tiles * 15
dock_bonus = dock_tiles * 0.2
water_access = adjacent_water_tiles >= 4 ? 1.0 : 0.5
rail_bonus = has_rail_connection ? 1.5 : 1.0
aqua_capacity = base_capacity * dock_bonus * water_access * rail_bonus
```

**Demand Effects:**
- +20% fabrication (industrial) demand globally (colony-wide)
- +10% exchange demand in 25-tile radius (trade hub effect)
- +5% habitation demand in 15-tile radius (port jobs)

**Negative Effects:**
- Industrial contamination in 8-tile radius (cargo operations)
- Moderate energy consumption

### 3.3 Port Development Levels

Both port types develop through levels based on capacity and utilization:

| Level | Name | Capacity Threshold | Visual |
|-------|------|-------------------|--------|
| 0 | Undeveloped | 0 | Empty zone |
| 1 | Basic | 100 | Small terminal |
| 2 | Standard | 500 | Medium terminal, partial infrastructure |
| 3 | Major | 2000 | Full terminal, complete infrastructure |
| 4 | International | 5000+ | Multiple terminals, advanced infrastructure |

---

## 4. Map Edge Connections

### 4.1 Edge Connection Points

External connections are made when infrastructure (pathways, rails) extends to the map edge:

**Detection Logic:**
```cpp
// Pathway reaches map edge
if (pathway_at(x, y) && is_map_edge(x, y)) {
    create_external_connection(ConnectionType::Pathway, get_edge(x, y), {x, y});
}

// Rail reaches map edge
if (rail_at(x, y) && is_map_edge(x, y)) {
    create_external_connection(ConnectionType::Rail, get_edge(x, y), {x, y});
}
```

### 4.2 Edge Types and Effects

| Edge | Typical Effect | Notes |
|------|---------------|-------|
| North | Trade with "northern neighbor" | Virtual NPC neighbor |
| East | Trade with "eastern neighbor" | Virtual NPC neighbor |
| South | Trade with "southern neighbor" | Virtual NPC neighbor |
| West | Trade with "western neighbor" | Virtual NPC neighbor |

**Virtual Neighbors:**
- For single-player and multiplayer, edge connections represent trade with NPC "neighbor colonies"
- Neighbor trade is abstract - no actual neighbor simulation
- Trade income scales with port capacity and connection quality

### 4.3 Connection Capacity

```cpp
struct EdgeConnectionCapacity {
    // Pathway connection
    uint32_t pathway_trade_capacity = 100;      // Base goods per cycle
    uint32_t pathway_migration_capacity = 50;   // Beings per cycle

    // Rail connection (bonus)
    uint32_t rail_trade_bonus = 200;            // Additional goods if rail connected
    uint32_t rail_migration_bonus = 25;         // Additional beings if rail connected
};
```

### 4.4 Migration Effects

External connections enable population migration:

**Inbound Migration:**
- When demand is high and connections exist, beings migrate in
- Rate: `migration_capacity * demand_factor * harmony_factor`
- Max per cycle: `10 + (external_connections * 5)`

**Outbound Migration:**
- When disorder is high or tribute is excessive, beings emigrate
- Rate: `migration_capacity * (disorder_index / 100) * tribute_penalty`
- Can cause population decline

---

## 5. Trade and External Economy

### 5.1 Trade Income Calculation

**Trade Income Sources:**
1. **Port Capacity Trade:** Based on port capacity and utilization
2. **External Connection Trade:** Based on map edge connections
3. **Inter-Player Trade:** Based on trade agreements (multiplayer)

**Port Trade Formula:**
```
aero_trade_income = aero_capacity * AERO_INCOME_PER_UNIT * utilization_factor
aqua_trade_income = aqua_capacity * AQUA_INCOME_PER_UNIT * utilization_factor

// Per canon, income goes to EconomySystem
// Proposed constants:
AERO_INCOME_PER_UNIT = 0.5 credits
AQUA_INCOME_PER_UNIT = 0.8 credits (cargo more valuable)
```

**Utilization Factor:**
```
utilization = min(demand_for_trade / capacity, 2.0)
// Can exceed 1.0 (over-utilized) up to 2.0 (congested)
// Over 1.0: bonus income but efficiency penalty
// Over 1.5: severe congestion, reduced capacity next cycle
```

### 5.2 Trade Income Integration with EconomySystem

PortSystem calculates trade income and reports to EconomySystem:

```cpp
// PortSystem provides
int64_t get_trade_income(PlayerID owner) const;
TradeIncomeBreakdown get_trade_income_breakdown(PlayerID owner) const;

struct TradeIncomeBreakdown {
    int64_t aero_income;
    int64_t aqua_income;
    int64_t external_trade_income;
    int64_t inter_player_trade_income;
    int64_t total;
};
```

**Canon Update Required:** Add `trade_income` to `IncomeBreakdown` in interfaces.yaml data_contracts.

### 5.3 Trade Goods Flow

```
+------------------+     +---------------+     +------------------+
| Fabrication Zone |---->| Aqua Port     |---->| External/Player  |
| (produces goods) |     | (cargo dock)  |     | (trade partner)  |
+------------------+     +---------------+     +------------------+

+------------------+     +---------------+     +------------------+
| Exchange Zone    |---->| Aero Port     |---->| External/Player  |
| (passengers)     |     | (terminal)    |     | (trade partner)  |
+------------------+     +---------------+     +------------------+
```

---

## 6. Demand Bonus Mechanics

### 6.1 Global Demand Modifiers

Ports provide colony-wide demand bonuses:

| Port Type | Zone Affected | Bonus | Rationale |
|-----------|--------------|-------|-----------|
| Aero Port | Exchange | +15% | Business travel, tourism |
| Aero Port | Habitation | +5% | Airport jobs |
| Aqua Port | Fabrication | +20% | Import/export access |
| Aqua Port | Exchange | +10% | Trade hub effect |

### 6.2 Local Demand Modifiers (Radius-Based)

| Port Type | Zone Affected | Radius | Bonus | Rationale |
|-----------|--------------|--------|-------|-----------|
| Aero Port | Habitation | 20 tiles | +5% | Airport employment |
| Aero Port | Fabrication | 15 tiles | +2% | Cargo handling |
| Aqua Port | Exchange | 25 tiles | +10% | Waterfront commerce |
| Aqua Port | Habitation | 15 tiles | +5% | Dock employment |

### 6.3 Integration with DemandSystem

DemandSystem queries PortSystem for bonuses:

```cpp
// IPortProvider interface method
DemandBonus get_demand_bonus(ZoneBuildingType zone_type, PlayerID owner) const;
DemandBonus get_local_demand_bonus(ZoneBuildingType zone_type, GridPosition pos, PlayerID owner) const;

struct DemandBonus {
    float global_modifier = 0.0f;    // Colony-wide bonus (0.0 to 0.5 typically)
    float local_modifier = 0.0f;     // Position-specific bonus
    float total() const { return global_modifier + local_modifier; }
};
```

**Integration Point:** DemandSystem (Epic 10) must query PortSystem when calculating zone demand.

---

## 7. Multiplayer: Inter-Player Trade

### 7.1 Trade Route Establishment

Players can establish trade routes with each other:

**Requirements:**
1. Both players have operational ports (either type)
2. Pathway or rail connection exists between their colonies
3. Trade agreement is mutually accepted

**Trade Route Data:**
```cpp
struct InterPlayerTradeRoute {
    PlayerID player_a;
    PlayerID player_b;
    PortType port_type_a;           // Which port of A is used
    PortType port_type_b;           // Which port of B is used
    TradeGoodType goods;            // What's being traded
    int64_t price_per_unit;         // Credits per unit
    uint32_t volume_per_cycle;      // Units traded per cycle
    TradeDirection direction;        // A->B, B->A, or bidirectional
};

enum class TradeDirection : uint8_t {
    AtoB = 0,
    BtoA = 1,
    Bidirectional = 2
};
```

### 7.2 Trade Benefits

**For Exporter:**
- Receives credits from importer
- Reduces fabrication surplus (if exporting materials)
- Increases port utilization

**For Importer:**
- Pays credits to exporter
- Receives demand bonus (goods availability)
- Increases port utilization

### 7.3 Trade Route Capacity

Trade capacity is limited by the smaller port's capacity:

```
route_capacity = min(port_a_capacity, port_b_capacity) * 0.5
```

### 7.4 Authority and Sync

| Aspect | Authority | Notes |
|--------|-----------|-------|
| Trade route creation | Server | Both players must accept |
| Trade route cancellation | Server | Either player can cancel |
| Trade volume calculation | Server | Server-authoritative |
| Trade income calculation | Server | Server-authoritative |
| Route capacity | Server | Calculated from port states |

### 7.5 Trade Route Network Messages

```cpp
// Client -> Server
struct TradeOfferRequest {
    PlayerID target_player;
    PortType offering_port;
    TradeGoodType goods;
    int64_t proposed_price;
    uint32_t proposed_volume;
    TradeDirection direction;
};

struct TradeOfferResponse {
    uint32_t offer_id;
    bool accepted;
};

struct TradeCancelRequest {
    uint32_t route_id;
};

// Server -> Client
struct TradeOfferNotification {
    uint32_t offer_id;
    PlayerID from_player;
    TradeOfferRequest details;
};

struct TradeRouteEstablished {
    uint32_t route_id;
    InterPlayerTradeRoute route;
};

struct TradeRouteCancelled {
    uint32_t route_id;
    PlayerID cancelled_by;
};
```

---

## 8. Tick Priority and Data Flow

### 8.1 Proposed Tick Priority

PortSystem should run after TransportSystem (needs connectivity) and before DemandSystem (provides demand bonuses):

```
Priority 45: TransportSystem        -- pathway connectivity calculated
Priority 47: RailSystem             -- rail connectivity calculated
Priority 48: PortSystem             -- NEW: port state, trade routes calculated
Priority 50: PopulationSystem       -- migration from ports affects population
Priority 52: DemandSystem           -- queries port demand bonuses
Priority 60: EconomySystem          -- receives trade income
```

**Alternative:** Priority 51 (after PopulationSystem, before DemandSystem) if migration is calculated by PopulationSystem using port data.

### 8.2 Data Flow Diagram

```
                        TerrainSystem (Epic 3)
                              |
                              | ITerrainQueryable (water for aqua ports)
                              v
TransportSystem (45) -----> PortSystem (48) <----- RailSystem (47)
       |                       |
       | ITransportProvider    | IPortProvider
       | (connectivity)        |
       v                       +------------------------+
                               |                        |
                               v                        v
                        DemandSystem (52)        EconomySystem (60)
                        (demand bonuses)         (trade income)
```

### 8.3 PortSystem Tick Logic

```
Each tick:

1. UPDATE PORT STATES
   For each port zone:
     Check terrain requirements (aqua: water adjacency)
     Check infrastructure requirements (runway/dock)
     Check connectivity (pathway access)
     Update is_operational flag
     Calculate current capacity

2. UPDATE EXTERNAL CONNECTIONS
   Scan map edges for pathway/rail connections
   Create/update ExternalConnectionComponent entities
   Calculate total external trade capacity

3. CALCULATE TRADE
   For each external connection:
     Calculate NPC trade income
   For each inter-player trade route:
     Calculate trade volume
     Calculate income for both parties

4. CALCULATE DEMAND BONUSES
   Aggregate port capacities by type
   Calculate global demand modifiers
   Cache for DemandSystem queries

5. REPORT TO ECONOMY
   Provide trade income to EconomySystem
   (EconomySystem deposits to treasury)
```

---

## 10. Key Work Items

### Phase 1: Core Infrastructure (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-001 | PortComponent definition | S | Define PortType enum and PortComponent struct |
| P8-002 | PortZoneComponent definition | S | Define port zone data (runway, dock, level) |
| P8-003 | ExternalConnectionComponent definition | S | Define map edge connection data |
| P8-004 | TradeRouteComponent definition | S | Define trade route data structure |
| P8-005 | PortSystem class skeleton | M | ISimulatable at tick_priority 48 |
| P8-006 | IPortProvider interface definition | M | Interface for port state queries |

### Phase 2: Port Zone Mechanics (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-007 | Aero port zone validation | M | Validate flat terrain, minimum size for aero zones |
| P8-008 | Aqua port zone validation | M | Validate water adjacency, dock requirements |
| P8-009 | Port capacity calculation | M | Calculate capacity from zone size and infrastructure |
| P8-010 | Port operational status | M | Check connectivity, infrastructure requirements |
| P8-011 | Port development levels | M | Track and update zone_level based on capacity |

### Phase 3: External Connections (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-012 | Map edge detection | M | Detect when pathways/rails reach map edge |
| P8-013 | External connection management | M | Create/update ExternalConnectionComponent |
| P8-014 | NPC trade calculation | M | Calculate trade with virtual neighbors |
| P8-015 | Migration effect calculation | M | Calculate population migration from connections |

### Phase 4: Demand Bonuses (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-016 | Global demand bonus calculation | M | Calculate colony-wide bonuses from ports |
| P8-017 | Local demand bonus calculation | M | Calculate radius-based bonuses |
| P8-018 | DemandSystem integration | M | Connect PortSystem to Epic 10 DemandSystem |

### Phase 5: Trade Income (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-019 | Trade income calculation | M | Calculate income from ports and connections |
| P8-020 | EconomySystem integration | M | Report trade income to Epic 11 EconomySystem |
| P8-021 | Trade income breakdown | S | Provide detailed breakdown for UI |

### Phase 6: Inter-Player Trade (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-022 | Trade offer system | L | Create, accept, reject trade offers |
| P8-023 | Trade route establishment | M | Establish routes on mutual acceptance |
| P8-024 | Trade route execution | M | Execute trades each cycle, transfer credits |
| P8-025 | Trade route cancellation | M | Allow either player to cancel |
| P8-026 | Trade network messages | M | Sync trade state across clients |
| P8-027 | Trade route capacity limits | M | Limit trades by port capacity |

### Phase 7: Rendering & Events (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-028 | Port zone rendering support | M | Provide data to RenderingSystem for port visuals |
| P8-029 | PortOperationalEvent definition | S | Event when port becomes operational/offline |
| P8-030 | TradeRouteEvent definitions | S | Events for trade route changes |

### Phase 8: Testing (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| P8-031 | Unit tests for port validation | L | Test zone requirements, capacity calculation |
| P8-032 | Integration test with Epic 10/11 | L | Test demand bonuses, trade income |
| P8-033 | Multiplayer trade test | L | Test inter-player trade route lifecycle |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 6 | P8-001, P8-002, P8-003, P8-004, P8-021, P8-029, P8-030 |
| M | 21 | P8-005 through P8-020, P8-022 through P8-028 |
| L | 4 | P8-022, P8-031, P8-032, P8-033 |
| **Total** | **33** | |

---

## 11. Questions for Other Agents

### @game-designer

1. **Aero port zone size requirements:** What should be the minimum zone size? Proposed 6x6 (36 tiles) for small airport, 9x12 (108 tiles) for full runway. Is this appropriate for gameplay balance?

2. **Aqua port water adjacency:** How many water tiles must the zone border? Proposed: minimum 4 water tiles for dock placement. Should we require contiguous water frontage?

3. **Demand bonus magnitudes:** Proposed +15% exchange from aero, +20% fabrication from aqua. Are these too strong/weak for game balance? SimCity 2000 had significant airport/seaport demand effects.

4. **Port noise contamination:** Should aero ports generate noise contamination (reducing sector value in radius)? This was a classic SimCity mechanic but adds complexity.

5. **Port zone vs. building placement:** Should ports be zone-only (auto-develop like R/C/I) or should players place specific port buildings within the zone? Proposed: Zone-based with auto-development for simplicity.

6. **Inter-player trade pricing:** Should players set trade prices freely, or should there be market-based pricing? Free pricing allows griefing (overcharging friends) but also enables creative cooperation.

7. **Migration effects from connections:** How significant should external connections be for population growth? Should a city without edge connections have migration penalties?

### @economy-engineer

8. **Trade income values:** Proposed 0.5 credits/aero capacity unit, 0.8 credits/aqua capacity unit. How does this scale with other income sources from Epic 11?

9. **Port operating costs:** Should ports have maintenance costs like other infrastructure? Proposed: Yes, but significant - ports are expensive to operate.

10. **Trade route credit transfer:** For inter-player trades, should credits transfer immediately each cycle, or accumulate for periodic settlement?

### @infrastructure-engineer

11. **Runway/dock detection:** How do we detect if a valid runway (6x2 flat tiles) or dock (4+ water-adjacent tiles) exists within a zone? Should this be explicit placement or auto-detected from terrain?

12. **Map edge connection detection:** Should edge connections be auto-detected when infrastructure reaches the edge, or should players explicitly "connect" to neighbors?

13. **Port connectivity requirements:** Should ports require DIRECT pathway adjacency, or is proximity (within 3 tiles like buildings) sufficient?

### @graphics-engineer

14. **Port zone visualization:** How should undeveloped vs. developed port zones be rendered? Should we show runway/dock outlines before they're "built"?

15. **Trade route visualization:** Should active trade routes be visible (e.g., animated cargo ships, planes)? This is cosmetic but adds visual feedback.

---

## 12. Risks and Concerns

### 12.1 Architectural Risks

**RISK: Zone System Coupling (MEDIUM)**
Ports are zone-based but have unique requirements (runway, water adjacency) not present in R/C/I zones. This may require significant ZoneSystem modifications.

**Mitigation:**
- Treat port zones as a separate zone category, not R/C/I density variants
- PortSystem validates requirements; ZoneSystem just tracks the zone
- Clear boundary: ZoneSystem = zone designation, PortSystem = port logic

**RISK: Demand System Complexity (MEDIUM)**
Adding global and local demand modifiers from ports increases DemandSystem complexity. Multiple sources (ports, services, taxes) all modify demand.

**Mitigation:**
- DemandSystem already handles multiple factors
- IPortProvider provides clean interface for queries
- Document modifier stacking clearly

**RISK: Multiplayer Trade Griefing (HIGH)**
Inter-player trade could enable griefing:
- Player A offers terrible deals to new player B
- Player A builds ports blocking player B's edge access
- Trade routes used to drain treasury

**Mitigation:**
- Trade requires mutual acceptance
- Market price reference for reasonable pricing
- Cannot block another player's map edge access (neutral shared edge)
- Trade volume limits prevent treasury draining

**RISK: Map Edge Ownership Conflicts (MEDIUM)**
Who owns map edges? Can one player block another from edge connections?

**Mitigation:**
- Map edges are GAME_MASTER owned (neutral)
- Any player can connect to edge on tiles they own adjacent to edge
- Edge connections are per-player, not exclusive

### 12.2 Design Ambiguities

**AMBIGUITY: Port Development Mechanism**
How do ports develop? Options:
- (A) Manual: Player places runway/dock buildings within zone
- (B) Automatic: Zone auto-develops like R/C/I based on conditions
- (C) Hybrid: Zone auto-develops but player can upgrade with placed buildings

**Recommendation:** Option (C) - Auto-development for basic functionality, optional placed buildings for enhanced capacity. This matches R/C/I pattern but allows strategic investment.

**AMBIGUITY: External Neighbor Behavior**
How do NPC neighbors behave?
- (A) Static: Fixed trade rates, unlimited capacity
- (B) Dynamic: Trade rates fluctuate based on global conditions
- (C) Responsive: Trade rates respond to player's trade volume

**Recommendation:** Option (A) for MVP - static NPC neighbors avoid complex simulation. Dynamic neighbors can be added later.

**AMBIGUITY: Multiple Ports**
Can a player have multiple aero ports and/or aqua ports?
- (A) One of each maximum
- (B) Unlimited, but diminishing returns
- (C) Unlimited with full benefits

**Recommendation:** Option (B) - Multiple ports allowed, but demand bonuses don't stack linearly. Second aero port = +50% of first's bonus. This encourages strategic port placement over port spam.

### 12.3 Technical Debt

**DEBT: Hardcoded Trade Values**
Trade income rates, demand bonuses, and capacity calculations will be hardcoded initially.

**DEBT: No Visual Trade Feedback**
Initial implementation won't show trade ships/planes. Visual feedback adds significant rendering work.

**DEBT: Simple Migration Model**
Migration is simplified - just capacity * factors. Real migration modeling would consider origin/destination conditions.

---

## 13. Dependencies

### 13.1 What Epic 8 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 3 | ITerrainQueryable | Water adjacency for aqua ports |
| Epic 4 | ZoneSystem | Port zone registration |
| Epic 4 | BuildingSystem | Building within port zones |
| Epic 7 | ITransportProvider | Pathway connectivity for port access |
| Epic 7 | RailSystem | Rail connectivity (optional bonus) |
| Epic 10 | DemandSystem | Demand bonus integration |
| Epic 11 | EconomySystem | Trade income depositing |

### 13.2 What Later Epics Need from Epic 8

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 10 (if not done) | Demand bonuses | IPortProvider.get_demand_bonus() |
| Epic 11 (if not done) | Trade income | IPortProvider.get_trade_income() |
| Epic 12 (UI) | Port info display | IPortProvider queries |
| Epic 14 (Progression) | Port milestones | Port capacity thresholds |

### 13.3 Canon Updates Required

1. **systems.yaml:** Add PortSystem entry with tick_priority and boundaries
2. **interfaces.yaml:** Add IPortProvider interface definition
3. **interfaces.yaml:** Add trade income to IncomeBreakdown data contract
4. **terminology.yaml:** Verify aero_port, aqua_port terms (already present)
5. **patterns.yaml:** Document port zone pattern (special zone category)

---

## 14. Integration Points

### 14.1 ZoneSystem Integration (Epic 4)

Port zones are a new zone category:

```cpp
enum class ZoneType : uint8_t {
    None = 0,
    Habitation = 1,
    Exchange = 2,
    Fabrication = 3,
    AeroPort = 4,      // NEW
    AquaPort = 5       // NEW
};
```

ZoneSystem handles designation; PortSystem handles logic.

### 14.2 TerrainSystem Integration (Epic 3)

Aqua port validation:

```cpp
bool validate_aqua_port_zone(const GridRect& zone, ITerrainQueryable& terrain) {
    int water_adjacent = 0;
    // Check zone perimeter for water tiles
    for (auto& edge_tile : get_perimeter(zone)) {
        for (auto& neighbor : get_neighbors(edge_tile)) {
            if (terrain.get_terrain_type(neighbor.x, neighbor.y) == TerrainType::Water) {
                water_adjacent++;
            }
        }
    }
    return water_adjacent >= 4;  // Minimum water adjacency
}
```

### 14.3 TransportSystem Integration (Epic 7)

Port connectivity check:

```cpp
bool is_port_accessible(EntityID port, ITransportProvider& transport) {
    auto pos = get_port_position(port);
    return transport.is_road_accessible_at(pos.x, pos.y, 3);  // 3-tile rule
}

bool is_port_connected_to_edge(EntityID port, ITransportProvider& transport) {
    auto pos = get_port_position(port);
    // Check if port is connected to any map edge via pathway network
    for (auto& edge_pos : get_map_edge_positions()) {
        if (transport.are_connected(pos.x, pos.y, edge_pos.x, edge_pos.y)) {
            return true;
        }
    }
    return false;
}
```

### 14.4 DemandSystem Integration (Epic 10)

DemandSystem queries port bonuses:

```cpp
// In DemandSystem::calculate_demand()
DemandBonus port_bonus = port_provider.get_demand_bonus(zone_type, owner);
zone_demand *= (1.0f + port_bonus.total());
```

### 14.5 EconomySystem Integration (Epic 11)

Trade income reporting:

```cpp
// Canon update: Add to IncomeBreakdown
struct IncomeBreakdown {
    Credits habitation_tribute;
    Credits exchange_tribute;
    Credits fabrication_tribute;
    Credits trade_income;          // NEW: from PortSystem
    Credits total;
};

// PortSystem provides trade income each cycle
TradeIncomeBreakdown port_income = port_system.get_trade_income_breakdown(owner);
income_breakdown.trade_income = port_income.total;
```

---

## 15. Proposed Interfaces

### 15.1 IPortProvider Interface

```cpp
class IPortProvider {
public:
    virtual ~IPortProvider() = default;

    // === Port State Queries ===

    // Get total port capacity by type for a player
    virtual uint32_t get_port_capacity(PortType type, PlayerID owner) const = 0;

    // Get port utilization (0.0-2.0, over 1.0 = congested)
    virtual float get_port_utilization(PortType type, PlayerID owner) const = 0;

    // Check if player has operational port of type
    virtual bool has_operational_port(PortType type, PlayerID owner) const = 0;

    // Get number of ports by type
    virtual uint32_t get_port_count(PortType type, PlayerID owner) const = 0;

    // === Demand Bonus Queries ===

    // Get global demand bonus for zone type (colony-wide)
    virtual float get_global_demand_bonus(ZoneBuildingType zone_type, PlayerID owner) const = 0;

    // Get local demand bonus at position (radius-based)
    virtual float get_local_demand_bonus(
        ZoneBuildingType zone_type,
        int32_t x, int32_t y,
        PlayerID owner
    ) const = 0;

    // Get combined demand bonus (convenience)
    virtual DemandBonus get_demand_bonus(
        ZoneBuildingType zone_type,
        GridPosition pos,
        PlayerID owner
    ) const = 0;

    // === External Connection Queries ===

    // Get number of external connections (map edge)
    virtual uint32_t get_external_connection_count(PlayerID owner) const = 0;

    // Check if connected to specific map edge
    virtual bool is_connected_to_edge(MapEdge edge, PlayerID owner) const = 0;

    // Get total external trade capacity
    virtual uint32_t get_external_trade_capacity(PlayerID owner) const = 0;

    // === Trade Income Queries ===

    // Get trade income for last cycle
    virtual int64_t get_trade_income(PlayerID owner) const = 0;

    // Get detailed trade income breakdown
    virtual TradeIncomeBreakdown get_trade_income_breakdown(PlayerID owner) const = 0;

    // === Inter-Player Trade Queries (Multiplayer) ===

    // Get active trade routes for player
    virtual std::vector<TradeRouteInfo> get_trade_routes(PlayerID owner) const = 0;

    // Check if trade route exists between two players
    virtual bool has_trade_route(PlayerID player_a, PlayerID player_b) const = 0;
};
```

### 15.2 Data Structures for Interface

```cpp
struct DemandBonus {
    float global_modifier = 0.0f;
    float local_modifier = 0.0f;
    float total() const { return global_modifier + local_modifier; }
};

struct TradeIncomeBreakdown {
    int64_t aero_income = 0;
    int64_t aqua_income = 0;
    int64_t external_trade_income = 0;
    int64_t inter_player_trade_income = 0;
    int64_t total = 0;
};

struct TradeRouteInfo {
    uint32_t route_id;
    PlayerID partner;
    PortType port_type;
    TradeGoodType goods;
    TradeDirection direction;
    int64_t income_per_cycle;
    bool is_active;
};
```

---

## Appendix A: Simulation Tick Order (Epic 8 Context)

```
Priority 5:  TerrainSystem        -- terrain data available
Priority 10: EnergySystem         -- power state calculated
Priority 20: FluidSystem          -- fluid state calculated
Priority 30: ZoneSystem           -- zones updated (including port zones)
Priority 40: BuildingSystem       -- buildings updated
Priority 45: TransportSystem      -- pathway connectivity calculated
Priority 47: RailSystem           -- rail connectivity calculated
Priority 48: PortSystem           -- NEW: port state, trade calculated
Priority 50: PopulationSystem     -- migration from ports affects population
Priority 52: DemandSystem         -- queries port demand bonuses
Priority 55: ServicesSystem       -- services processed
Priority 60: EconomySystem        -- receives trade income
Priority 70: DisorderSystem       -- disorder calculated
Priority 80: ContaminationSystem  -- port contamination included
Priority 85: LandValueSystem      -- port effects on sector value
```

---

## Appendix B: Entity Composition Examples

**Aero Port Zone:**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 50 }
    OwnershipComponent { owner: PLAYER_1 }
    ZoneComponent { zone_type: AeroPort, density: N/A }
    PortZoneComponent {
        port_type: Aero,
        runway_area: { 50, 55, 6, 2 },
        zone_level: 2,
        has_runway: true,
        zone_tiles: 72
    }
    PortComponent {
        port_type: Aero,
        capacity: 540,
        max_capacity: 720,
        utilization: 75,
        is_operational: true,
        is_connected_to_edge: true,
        demand_bonus_radius: 20
    }
}
```

**External Connection (Road at Map Edge):**
```
Entity {
    PositionComponent { grid_x: 0, grid_y: 128 }  // West edge
    OwnershipComponent { owner: GAME_MASTER }
    ExternalConnectionComponent {
        connection_type: Pathway,
        edge_side: West,
        edge_position: { 0, 128 },
        is_active: true,
        trade_capacity: 100,
        migration_capacity: 50
    }
}
```

**Inter-Player Trade Route:**
```
Entity {
    TradeRouteComponent {
        route_type: InterPlayer,
        source_player: PLAYER_1,
        dest_player: PLAYER_2,
        good_type: Goods,
        volume_per_cycle: 50,
        income_per_cycle: 250,
        is_active: true
    }
}
```

---

## Appendix C: Canon Update Proposals

### systems.yaml Addition

```yaml
phase_4:
  epic_8_ports:
    name: "Ports & External Connections"
    systems:
      PortSystem:
        type: ecs_system
        tick_priority: 48  # After RailSystem (47), before PopulationSystem (50)
        owns:
          - Aero port (airport) zones
          - Aqua port (seaport) zones
          - External connection points
          - Trade routes (external and inter-player)
          - Trade income calculation
          - Demand bonus calculation
        does_not_own:
          - Zone mechanics (ZoneSystem owns)
          - Port zone rendering (RenderingSystem owns)
          - Trade income depositing (EconomySystem owns)
        provides:
          - "IPortProvider: port state queries"
          - Port capacity
          - Trade income
          - Demand bonuses
        depends_on:
          - ZoneSystem
          - TerrainSystem (water for seaports)
          - TransportSystem (pathway connections)
          - RailSystem (rail connections)
        multiplayer:
          authority: server
          per_player: true  # Each player has own ports
        notes:
          - "Ports are zone-based, not placed buildings"
          - "Demand bonuses are global and local (radius)"
          - "Inter-player trade requires mutual acceptance"
```

### interfaces.yaml Addition

```yaml
ports:
  IPortProvider:
    description: "Provides port state queries for other systems"
    purpose: "Allows DemandSystem and EconomySystem to query port effects"

    methods:
      - name: get_port_capacity
        params:
          - name: type
            type: PortType
          - name: owner
            type: PlayerID
        returns: uint32_t
        description: "Total port capacity by type for player"

      - name: get_demand_bonus
        params:
          - name: zone_type
            type: ZoneBuildingType
          - name: pos
            type: GridPosition
          - name: owner
            type: PlayerID
        returns: DemandBonus
        description: "Combined global and local demand bonus"

      - name: get_trade_income
        params:
          - name: owner
            type: PlayerID
        returns: int64_t
        description: "Trade income for last cycle"

    implemented_by:
      - PortSystem (Epic 8)

    notes:
      - "Demand bonuses stack with other modifiers in DemandSystem"
      - "Trade income added to IncomeBreakdown"
```

---

**End of Systems Architect Analysis: Epic 8 - Ports & External Connections**
