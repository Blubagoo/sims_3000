# Port System Specification

Epic 8: Ports & External Connections

## Overview

The Port System (`PortSystem`) manages all port facilities, external map-edge connections, trade agreements, and NPC neighbor interactions. It provides the `IPortProvider` interface for downstream systems (BuildingSystem, ZoneSystem, EconomySystem) to query port state, demand bonuses, and trade income.

## System Registration

| Property | Value |
|----------|-------|
| Class | `sims3000::port::PortSystem` |
| Header | `include/sims3000/port/PortSystem.h` |
| Source | `src/port/PortSystem.cpp` |
| Interface | `sims3000::building::IPortProvider` |
| tick_priority | **48** |
| Runs after | RailSystem (47) |
| Runs before | PopulationSystem (50) |
| Pattern | ISimulatable (duck-typed) |

## Tick Phases

Each simulation tick executes four phases in order:

1. **Update Port States** - Evaluate operational status of all port facilities
2. **Update External Connections** - Refresh active/inactive state of map-edge connections
3. **Calculate Trade Income** - Compute income from active trade agreements and port operations
4. **Cache Demand Bonuses** - Pre-compute demand bonus values for zone queries

## IPortProvider Interface

Defined in `include/sims3000/building/ForwardDependencyInterfaces.h`.

### Port State Queries

| Method | Return | Description |
|--------|--------|-------------|
| `get_port_capacity(port_type, owner)` | `uint32_t` | Total capacity across all ports of this type for owner |
| `get_port_utilization(port_type, owner)` | `float` | Utilization ratio (0.0 idle to 1.0 fully utilized) |
| `has_operational_port(port_type, owner)` | `bool` | Whether at least one operational port of this type exists |
| `get_port_count(port_type, owner)` | `uint32_t` | Number of ports of this type for owner |

### Demand Bonus Queries

| Method | Return | Description |
|--------|--------|-------------|
| `get_global_demand_bonus(zone_type, owner)` | `float` | Colony-wide demand bonus from all operational ports |
| `get_local_demand_bonus(zone_type, x, y, owner)` | `float` | Proximity-based demand bonus from nearby ports |

### External Connection Queries

| Method | Return | Description |
|--------|--------|-------------|
| `get_external_connection_count(owner)` | `uint32_t` | Number of active external connections |
| `is_connected_to_edge(edge, owner)` | `bool` | Whether connected to a specific map edge |

### Trade Income Queries

| Method | Return | Description |
|--------|--------|-------------|
| `get_trade_income(owner)` | `int64_t` | Total trade income in credits per cycle |

## Port Zone Pattern

Port zones are a **special zone category** that extends the standard zone system. Unlike habitation, exchange, and fabrication zones, port zones have type-specific infrastructure requirements.

### Port Types

| Port Type | Enum Value | Zone Boost | Terminology |
|-----------|------------|------------|-------------|
| Aero | `PortType::Aero (0)` | Exchange demand | aero_port (not airport) |
| Aqua | `PortType::Aqua (1)` | Fabrication demand | aqua_port (not seaport) |

Uses canonical alien terminology per `/docs/canon/terminology.yaml`.

### Port Zone Component (16 bytes)

```cpp
struct PortZoneComponent {
    PortType port_type;        // 1 byte - Aero or Aqua
    uint8_t zone_level;        // 1 byte - Development level (0-4)
    bool has_runway;           // 1 byte - Aero runway requirement
    bool has_dock;             // 1 byte - Aqua dock requirement
    uint8_t runway_length;     // 1 byte - Runway length in tiles
    uint8_t dock_count;        // 1 byte - Water-adjacent docks
    uint16_t zone_tiles;       // 2 bytes - Total zone tiles
    GridRect runway_area;      // 8 bytes - Runway bounding rect
};
```

### Port Component (16 bytes)

```cpp
struct PortComponent {
    PortType port_type;              // 1 byte
    uint16_t capacity;               // 2 bytes (max 5000)
    uint16_t max_capacity;           // 2 bytes
    uint8_t utilization;             // 1 byte (0-100%)
    uint8_t infrastructure_level;    // 1 byte (0-3)
    bool is_operational;             // 1 byte
    bool is_connected_to_edge;       // 1 byte
    uint8_t demand_bonus_radius;     // 1 byte
    uint8_t connection_flags;        // 1 byte (Pathway=1, Rail=2, Energy=4, Fluid=8)
    uint8_t padding[4];              // 4 bytes reserved
};
```

## Operational Status

A port is operational when ALL of these conditions are met:

1. **Zone validation** - Size and terrain requirements satisfied
2. **Infrastructure** - Runway (aero) or dock (aqua) requirement met
3. **Pathway connectivity** - Within 3 tiles of a pathway
4. **Non-zero capacity** - Has calculated capacity > 0

## Capacity Formulas

### Aero Port Capacity

```
base_capacity = zone_tiles * 10
runway_bonus  = has_runway ? 1.5 : 0.5
access_bonus  = pathway_connected ? 1.0 : 0.0
aero_capacity = base_capacity * runway_bonus * access_bonus
```

Maximum: **2,500**

### Aqua Port Capacity

```
base_capacity  = zone_tiles * 15
dock_bonus     = 1.0 + (dock_count * 0.2)
water_access   = adjacent_water >= 4 ? 1.0 : 0.5
rail_bonus     = has_rail ? 1.5 : 1.0
aqua_capacity  = base_capacity * dock_bonus * water_access * rail_bonus
```

Maximum: **5,000**

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `AERO_PORT_MAX_CAPACITY` | 2500 | Aero port capacity cap |
| `AQUA_PORT_MAX_CAPACITY` | 5000 | Aqua port capacity cap |
| `AERO_BASE_CAPACITY_PER_TILE` | 10 | Base capacity per zone tile (aero) |
| `AQUA_BASE_CAPACITY_PER_TILE` | 15 | Base capacity per zone tile (aqua) |
| `AERO_RUNWAY_BONUS` | 1.5 | Multiplier when runway present |
| `AERO_NO_RUNWAY_BONUS` | 0.5 | Multiplier when runway absent |
| `AERO_ACCESS_CONNECTED` | 1.0 | Multiplier when pathway connected |
| `AERO_ACCESS_DISCONNECTED` | 0.0 | Multiplier when pathway disconnected |
| `AQUA_DOCK_BONUS_BASE` | 1.0 | Base dock bonus |
| `AQUA_DOCK_BONUS_PER_DOCK` | 0.2 | Bonus increment per dock |
| `AQUA_WATER_ACCESS_FULL` | 1.0 | Full water access multiplier |
| `AQUA_WATER_ACCESS_PARTIAL` | 0.5 | Partial water access multiplier |
| `AQUA_MIN_ADJACENT_WATER` | 4 | Minimum tiles for full water access |
| `AQUA_RAIL_BONUS_CONNECTED` | 1.5 | Multiplier when rail connected |
| `AQUA_RAIL_BONUS_DISCONNECTED` | 1.0 | Multiplier when rail not connected |

## Development Levels

Port zones progress through development tiers based on capacity:

| Level | Name | Capacity Threshold |
|-------|------|--------------------|
| 0 | Undeveloped | 0 |
| 1 | Basic | 100 |
| 2 | Standard | 500 |
| 3 | Major | 2,000 |
| 4 | International | 5,000+ |

Level transitions emit `PortUpgradedEvent` for the rendering system.

## Port Upgrades

Players invest credits in infrastructure upgrades for trade multiplier bonuses:

| Upgrade Level | Cost | Trade Multiplier | Requires Rail |
|---------------|------|------------------|---------------|
| Basic | 0 (default) | 1.0x | No |
| Upgraded Terminals | 50,000 cr | 1.2x | No |
| Advanced Logistics | 100,000 cr | 1.4x | Yes |
| Premium Hub | 200,000 cr | 1.6x | Yes (full) |

## External Connections

### Connection Types

| Type | Trade Capacity/tile | Migration Capacity/tile |
|------|---------------------|-------------------------|
| Pathway | 100 | 50 |
| Rail | +200 bonus | +25 bonus |
| Energy | 0 (grid only) | 0 |
| Fluid | 0 (network only) | 0 |

### Map Edge Detection

External connections are detected at the four map edges (North, East, South, West). Each edge with at least one connection spawns an NPC neighbor.

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `PATHWAY_TRADE_CAPACITY_PER_TILE` | 100 | Pathway trade capacity |
| `PATHWAY_MIGRATION_CAPACITY_PER_TILE` | 50 | Pathway migration capacity |
| `RAIL_TRADE_CAPACITY_BONUS` | 200 | Rail trade bonus |
| `RAIL_MIGRATION_CAPACITY_BONUS` | 25 | Rail migration bonus |

## NPC Neighbors

Generated at game start based on map edge connections (max 4, one per edge).

### Generation

- Deterministic generation from seed (LCG-based RNG)
- Each neighbor assigned to one map edge
- Unique name from themed pool of 8 names
- Randomized demand_factor in [0.5, 1.5]
- Randomized supply_factor in [0.5, 1.5]
- Initial relationship: `TradeAgreementType::None`

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_NEIGHBORS` | 4 | Maximum NPC neighbors |
| `NEIGHBOR_NAME_POOL_SIZE` | 8 | Names available for selection |
| `NEIGHBOR_FACTOR_MIN` | 0.5 | Minimum demand/supply factor |
| `NEIGHBOR_FACTOR_MAX` | 1.5 | Maximum demand/supply factor |

## Neighbor Relationships (E8-034)

Trade history is tracked per NPC neighbor. Cumulative trade activity evolves the relationship, unlocking better trade deal tiers.

### Relationship Thresholds

| Status | Value Range | Max Tier Available |
|--------|-------------|-------------------|
| Hostile | < -50 | None |
| Cold | -50 to -1 | Basic |
| Neutral | 0 to 24 | Basic |
| Warm | 25 to 49 | Enhanced |
| Friendly | 50 to 79 | Enhanced |
| Allied | 80+ | Premium |

### Functions

| Function | Description |
|----------|-------------|
| `update_relationship(rel, points)` | Add points, clamp to [-100, +100], recalculate tier |
| `get_max_available_tier(value)` | Map relationship value to max TradeAgreementType |
| `get_relationship_status(value)` | Map relationship value to RelationshipStatus enum |
| `record_trade(rel, volume, points)` | Increment trade count/volume and update relationship |

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `RELATIONSHIP_MIN` | -100 | Minimum relationship value |
| `RELATIONSHIP_MAX` | 100 | Maximum relationship value |
| `RELATIONSHIP_HOSTILE_MAX` | -51 | Upper bound for Hostile |
| `RELATIONSHIP_COLD_MAX` | -1 | Upper bound for Cold |
| `RELATIONSHIP_NEUTRAL_MAX` | 24 | Upper bound for Neutral |
| `RELATIONSHIP_WARM_MAX` | 49 | Upper bound for Warm |
| `RELATIONSHIP_FRIENDLY_MAX` | 79 | Upper bound for Friendly |
| `RELATIONSHIP_ALLIED_MIN` | 80 | Lower bound for Allied |

## Trade Agreements

### Trade Agreement Component (16 bytes, packed)

```cpp
struct TradeAgreementComponent {
    PlayerID party_a;                    // 1 byte (0 = GAME_MASTER/NPC)
    PlayerID party_b;                    // 1 byte
    TradeAgreementType agreement_type;   // 1 byte
    uint8_t neighbor_id;                 // 1 byte
    uint16_t cycles_remaining;           // 2 bytes
    int8_t demand_bonus_a;               // 1 byte
    int8_t demand_bonus_b;               // 1 byte
    uint8_t income_bonus_percent;        // 1 byte (100 = 1.0x)
    uint8_t padding;                     // 1 byte
    int32_t cost_per_cycle_a;            // 4 bytes
    int16_t cost_per_cycle_b;            // 2 bytes
};
```

### Trade Deal Tiers (NPC Deals)

| Level | Cost/Cycle | Income Multiplier | Demand Bonus | Default Duration |
|-------|------------|-------------------|--------------|------------------|
| None | 0 | 0.5x (50%) | +0 | 0 |
| Basic | 1,000 cr | 0.8x (80%) | +5 | 500 |
| Enhanced | 2,500 cr | 1.0x (100%) | +10 | 1,000 |
| Premium | 5,000 cr | 1.2x (120%) | +15 | 1,500 |

### Inter-Player Trade Benefits (Symmetric)

| TradeAgreementType | Demand Bonus (each) | Income Bonus |
|--------------------|---------------------|--------------|
| None | +0 | +0% |
| Basic | +3 | +5% |
| Enhanced | +6 | +10% |
| Premium | +10 | +15% |

## Demand Bonuses

### Global Demand Bonus

Operational ports provide colony-wide demand bonuses based on capacity:

| Port Size | Capacity | Bonus |
|-----------|----------|-------|
| Small | < 500 | +5 |
| Medium | 500-1,999 | +10 |
| Large | >= 2,000 | +15 |

- Aero ports boost Exchange (zone_type 1) demand
- Aqua ports boost Fabrication (zone_type 2) demand
- Maximum total bonus capped at **+30**

### Local Demand Bonus

Proximity-based bonuses from nearby ports:

| Port Type | Zone Boosted | Radius | Bonus |
|-----------|-------------|--------|-------|
| Aero | Habitation (zone_type 0) | 20 tiles (Manhattan) | +5% |
| Aqua | Exchange (zone_type 1) | 25 tiles (Manhattan) | +10% |

### Diminishing Returns

Multiple ports of the same type apply diminishing returns:

| Port Index | Multiplier |
|------------|------------|
| 1st port | 100% |
| 2nd port | 50% |
| 3rd port | 25% |
| 4th+ port | 12.5% (floor) |

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `PORT_SIZE_SMALL_MAX` | 499 | Upper bound for Small |
| `PORT_SIZE_MEDIUM_MIN` | 500 | Lower bound for Medium |
| `PORT_SIZE_MEDIUM_MAX` | 1999 | Upper bound for Medium |
| `PORT_SIZE_LARGE_MIN` | 2000 | Lower bound for Large |
| `DEMAND_BONUS_SMALL` | 5.0 | Small port bonus |
| `DEMAND_BONUS_MEDIUM` | 10.0 | Medium port bonus |
| `DEMAND_BONUS_LARGE` | 15.0 | Large port bonus |
| `MAX_TOTAL_DEMAND_BONUS` | 30.0 | Maximum total bonus cap |
| `LOCAL_BONUS_AERO_RADIUS` | 20 | Aero local bonus radius |
| `LOCAL_BONUS_AQUA_RADIUS` | 25 | Aqua local bonus radius |
| `LOCAL_BONUS_AERO_HABITATION` | 5.0 | Aero habitation bonus |
| `LOCAL_BONUS_AQUA_EXCHANGE` | 10.0 | Aqua exchange bonus |
| `DIMINISHING_FIRST` | 1.0 | 1st port multiplier |
| `DIMINISHING_SECOND` | 0.5 | 2nd port multiplier |
| `DIMINISHING_THIRD` | 0.25 | 3rd port multiplier |
| `DIMINISHING_FLOOR` | 0.125 | 4th+ port multiplier floor |

## Trade Income

### Formula

```
port_trade_income = capacity * utilization * income_rate * trade_multiplier * external_demand_factor
```

### Income Rates

| Port Type | Income Rate (credits/phase) |
|-----------|---------------------------|
| Aero | 0.8 |
| Aqua | 0.6 |

### Utilization Estimates

| Port Size | Capacity | Utilization |
|-----------|----------|-------------|
| Small | < 500 | 0.5 |
| Medium | 500-1,999 | 0.7 |
| Large | >= 2,000 | 0.9 |

### Config Values

| Constant | Value | Description |
|----------|-------|-------------|
| `AERO_INCOME_PER_UNIT` | 0.8 | Aero income per utilized capacity unit |
| `AQUA_INCOME_PER_UNIT` | 0.6 | Aqua income per utilized capacity unit |
| `DEFAULT_EXTERNAL_DEMAND_FACTOR` | 1.0 | Default external demand factor |
| `DEFAULT_TRADE_MULTIPLIER` | 1.0 | Default trade multiplier (no agreements) |

## File Map

### Headers (`include/sims3000/port/`)

| File | Ticket | Description |
|------|--------|-------------|
| `PortTypes.h` | E8-001, E8-004 | Enums: PortType, MapEdge, ConnectionType, TradeAgreementType |
| `PortComponent.h` | E8-002 | Per-port-facility data (16 bytes) |
| `PortZoneComponent.h` | E8-003 | Per-port-zone data (16 bytes) |
| `ExternalConnectionComponent.h` | E8-004 | Per-connection data (16 bytes) |
| `TradeAgreementComponent.h` | E8-005 | Per-agreement data (16 bytes, packed) |
| `PortSystem.h` | E8-006 | Main orchestrator, IPortProvider impl |
| `PortCapacity.h` | E8-010 | Capacity calculation functions |
| `PortOperational.h` | E8-011 | Operational status checks |
| `PortDevelopment.h` | E8-012 | Development level calculation |
| `MapEdgeDetection.h` | E8-013 | Map edge connection detection |
| `ConnectionCapacity.h` | E8-014 | External connection capacity |
| `NeighborGeneration.h` | E8-015 | NPC neighbor generation |
| `DemandBonus.h` | E8-016 | Global/local demand bonus |
| `TradeIncome.h` | E8-019 | Trade income calculation |
| `PortIncomeUI.h` | E8-021 | Trade income UI data |
| `TradeDealNegotiation.h` | E8-022 | NPC trade deal negotiation |
| `TradeOfferManager.h` | E8-025 | Trade offer lifecycle management |
| `MigrationEffects.h` | E8-024 | Migration flow effects |
| `TradeDealExpiration.h` | E8-026 | Trade deal expiration logic |
| `TradeAgreementBenefits.h` | E8-027 | Inter-player trade benefits |
| `TradeEvents.h` | E8-029 | Trade event definitions |
| `PortEvents.h` | E8-029 | Port event definitions |
| `PortRenderData.h` | E8-030 | Port visual state for rendering |
| `PortZoneValidation.h` | E8-031 | Zone type extension validation |
| `PortUpgrades.h` | E8-032 | Infrastructure upgrade config |
| `PortContamination.h` | E8-033 | Noise/contamination effects |
| `NeighborRelationship.h` | E8-034 | Neighbor relationship evolution |
| `DiminishingReturns.h` | E8-035 | Multiple port diminishing returns |
| `PortSerialization.h` | E8-002 | Save/load serialization |
| `TradeNetworkMessages.h` | E8-028 | Network message definitions |

### Sources (`src/port/`)

| File | Description |
|------|-------------|
| `PortSystem.cpp` | Main orchestrator implementation |
| `PortCapacity.cpp` | Capacity calculation implementation |
| `PortOperational.cpp` | Operational status implementation |
| `MapEdgeDetection.cpp` | Map edge detection implementation |
| `NeighborGeneration.cpp` | Neighbor generation implementation |
| `DemandBonus.cpp` | Demand bonus implementation |
| `TradeDealNegotiation.cpp` | Trade deal negotiation implementation |
| `TradeOfferManager.cpp` | Trade offer management implementation |
| `MigrationEffects.cpp` | Migration effects implementation |
| `TradeDealExpiration.cpp` | Trade deal expiration implementation |
| `TradeNetworkMessages.cpp` | Network message implementation |
| `TradeIncome.cpp` | Trade income calculation implementation |
| `PortSerialization.cpp` | Serialization implementation |
| `PortZoneValidation.cpp` | Zone validation implementation |
| `NeighborRelationship.cpp` | Neighbor relationship implementation |
