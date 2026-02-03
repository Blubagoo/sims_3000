# Epic 8: Ports & External Connections - Tickets

**Epic:** 8 - Ports & External Connections
**Canon Version:** 0.13.0
**Date:** 2026-01-29
**Status:** PLANNING COMPLETE

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.13.0 | Initial ticket creation |

> **Planning Note (2026-01-29):** Created from systems-architect, game-designer, and economy-engineer analyses. PortSystem at tick_priority 48. IPortProvider interface for DemandSystem/EconomySystem integration.

---

## Overview

Epic 8 implements **PortSystem** - external connections infrastructure enabling trade, population migration, and demand bonuses. Introduces aero ports (airports) boosting exchange demand, aqua ports (seaports) boosting fabrication demand, map edge neighbor connections, and inter-player trade mechanics.

**Key Architectural Decisions (from planning):**

| CCR ID | Decision |
|--------|----------|
| CCR-001 | Ports are zone-based (not placed buildings), using special zone types |
| CCR-002 | PortSystem at tick_priority 48 (after RailSystem 47, before PopulationSystem 50) |
| CCR-003 | IPortProvider interface for demand bonus and trade income queries |
| CCR-004 | Pool model for port capacity (similar to energy/fluid pattern) |
| CCR-005 | Demand bonus cap at +30 to prevent infinite stacking |
| CCR-006 | Neighbor trade uses AI-controlled virtual neighbors (NPC) |
| CCR-007 | Inter-player trade requires mutual acceptance (server-authoritative) |
| CCR-008 | Trade income added to IncomeBreakdown data contract |
| CCR-009 | Map edges owned by GAME_MASTER (neutral access) |
| CCR-010 | Resource sharing (energy/fluid) deferred to post-MVP |

---

## Ticket Summary

| Priority | Count | Description |
|----------|-------|-------------|
| P0 | 18 | Core infrastructure, must have for MVP |
| P1 | 14 | Important features, should have |
| P2 | 8 | Enhancements, later phases |
| **Total** | **40** | |

---

## P0 - Critical (Must Have)

### Core Infrastructure

#### E8-001: PortType Enum and Port Terminology
**Size:** S | **Component:** PortSystem

Define port type enumeration using canonical terminology.

```cpp
enum class PortType : uint8_t {
    Aero = 0,    // aero_port (airport) - boosts exchange demand
    Aqua = 1     // aqua_port (seaport) - boosts fabrication demand
};
```

**Acceptance Criteria:**
- [ ] Enum defined in port types header
- [ ] Uses canonical terminology (aero_port, aqua_port per terminology.yaml)
- [ ] Comments reference zone demand effects

---

#### E8-002: PortComponent Definition (16 bytes)
**Size:** S | **Component:** PortSystem | **Depends:** E8-001

Define PortComponent struct for port entities.

```cpp
struct PortComponent {
    PortType port_type = PortType::Aero;     // 1 byte
    uint16_t capacity = 0;                    // 2 bytes: max 5000
    uint16_t max_capacity = 0;                // 2 bytes
    uint8_t utilization = 0;                  // 1 byte: 0-100%
    uint8_t infrastructure_level = 0;         // 1 byte: 0-3
    bool is_operational = false;              // 1 byte
    bool is_connected_to_edge = false;        // 1 byte
    uint8_t demand_bonus_radius = 0;          // 1 byte
    uint8_t connection_flags = 0;             // 1 byte: pathway/rail/etc
    uint8_t padding[4];                       // 4 bytes
};
static_assert(sizeof(PortComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Size assertion passes (16 bytes)
- [ ] Serialization implemented for network sync

---

#### E8-003: PortZoneComponent Definition (16 bytes)
**Size:** S | **Component:** PortSystem | **Depends:** E8-001

Define port zone metadata component.

```cpp
struct PortZoneComponent {
    PortType port_type = PortType::Aero;      // 1 byte
    uint8_t zone_level = 0;                   // 1 byte: 0-4 development
    bool has_runway = false;                  // 1 byte: aero requirement
    bool has_dock = false;                    // 1 byte: aqua requirement
    uint8_t runway_length = 0;                // 1 byte: tiles (aero)
    uint8_t dock_count = 0;                   // 1 byte: water-adjacent (aqua)
    uint16_t zone_tiles = 0;                  // 2 bytes: total tiles in zone
    GridRect runway_area;                     // 8 bytes: x,y,w,h as int16
};
static_assert(sizeof(PortZoneComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Tracks runway (aero) and dock (aqua) requirements separately
- [ ] Zone level 0-4 for development progression

---

#### E8-004: ExternalConnectionComponent Definition (16 bytes)
**Size:** S | **Component:** PortSystem

Define map edge connection component.

```cpp
enum class MapEdge : uint8_t {
    North = 0, East = 1, South = 2, West = 3
};

enum class ConnectionType : uint8_t {
    Pathway = 0,  // Road to edge
    Rail = 1,     // Rail to edge
    Energy = 2,   // Power (future)
    Fluid = 3     // Water (future)
};

struct ExternalConnectionComponent {
    ConnectionType connection_type = ConnectionType::Pathway;  // 1 byte
    MapEdge edge_side = MapEdge::North;                        // 1 byte
    uint16_t edge_position = 0;                                // 2 bytes
    bool is_active = false;                                    // 1 byte
    uint8_t padding1 = 0;                                      // 1 byte
    uint16_t trade_capacity = 0;                               // 2 bytes
    uint16_t migration_capacity = 0;                           // 2 bytes
    uint16_t padding2 = 0;                                     // 2 bytes
    GridPosition position;                                     // 4 bytes
};
static_assert(sizeof(ExternalConnectionComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Supports all four map edges
- [ ] Tracks trade and migration capacity

---

#### E8-005: TradeAgreementComponent Definition (16 bytes)
**Size:** S | **Component:** PortSystem

Define trade agreement component for neighbor and inter-player trade.

```cpp
enum class TradeAgreementType : uint8_t {
    None = 0,
    Basic = 1,
    Enhanced = 2,
    Premium = 3
};

struct TradeAgreementComponent {
    PlayerID party_a = 0;                     // 1 byte: first player or GAME_MASTER
    PlayerID party_b = 0;                     // 1 byte: second player
    TradeAgreementType agreement_type = TradeAgreementType::None;  // 1 byte
    uint8_t neighbor_id = 0;                  // 1 byte: if NPC neighbor
    uint16_t cycles_remaining = 0;            // 2 bytes
    int8_t demand_bonus_a = 0;                // 1 byte
    int8_t demand_bonus_b = 0;                // 1 byte
    uint8_t income_bonus_percent = 100;       // 1 byte: 100 = 1.0x
    uint8_t padding = 0;                      // 1 byte
    int32_t cost_per_cycle_a = 0;             // 4 bytes
    int16_t cost_per_cycle_b = 0;             // 2 bytes (smaller for balance)
};
static_assert(sizeof(TradeAgreementComponent) == 16);
```

**Acceptance Criteria:**
- [ ] Component registered with ECS
- [ ] Supports both NPC neighbors (party_a = GAME_MASTER) and inter-player trade
- [ ] Duration tracking for deal expiration

---

#### E8-006: PortSystem Class Skeleton
**Size:** M | **Component:** PortSystem | **Depends:** E8-002, E8-003, E8-004, E8-005

Implement PortSystem as ISimulatable at tick_priority 48.

```cpp
class PortSystem : public ISimulatable, public IPortProvider {
public:
    void tick(float delta_time) override;
    int get_priority() const override { return 48; }

    // IPortProvider implementation (stubs initially)
    uint32_t get_port_capacity(PortType type, PlayerID owner) const override;
    float get_port_utilization(PortType type, PlayerID owner) const override;
    // ... other interface methods

private:
    void update_port_states();
    void update_external_connections();
    void calculate_trade_income();
    void cache_demand_bonuses();
};
```

**Acceptance Criteria:**
- [ ] System registered with SimulationCore at priority 48
- [ ] Implements ISimulatable::tick()
- [ ] Implements IPortProvider interface (stub returns initially)
- [ ] Runs after RailSystem (47), before PopulationSystem (50)

---

#### E8-007: IPortProvider Interface Definition
**Size:** M | **Component:** PortSystem | **Depends:** E8-006

Define IPortProvider interface for DemandSystem and EconomySystem integration.

```cpp
class IPortProvider {
public:
    virtual ~IPortProvider() = default;

    // Port state queries
    virtual uint32_t get_port_capacity(PortType type, PlayerID owner) const = 0;
    virtual float get_port_utilization(PortType type, PlayerID owner) const = 0;
    virtual bool has_operational_port(PortType type, PlayerID owner) const = 0;
    virtual uint32_t get_port_count(PortType type, PlayerID owner) const = 0;

    // Demand bonus queries
    virtual float get_global_demand_bonus(ZoneBuildingType zone_type, PlayerID owner) const = 0;
    virtual float get_local_demand_bonus(ZoneBuildingType zone_type, int32_t x, int32_t y, PlayerID owner) const = 0;

    // External connection queries
    virtual uint32_t get_external_connection_count(PlayerID owner) const = 0;
    virtual bool is_connected_to_edge(MapEdge edge, PlayerID owner) const = 0;

    // Trade income queries
    virtual int64_t get_trade_income(PlayerID owner) const = 0;
    virtual TradeIncomeBreakdown get_trade_income_breakdown(PlayerID owner) const = 0;
};
```

**Acceptance Criteria:**
- [ ] Interface defined in interfaces header
- [ ] Canon update: Add IPortProvider to interfaces.yaml
- [ ] Methods match analysis specification

---

### Port Zone Mechanics

#### E8-008: Aero Port Zone Validation
**Size:** M | **Component:** PortSystem | **Depends:** E8-003

Implement aero port zone validation (flat terrain, minimum size, runway detection).

Requirements:
- Minimum zone size: 36 tiles (6x6)
- Runway: Linear clear area minimum 6 tiles long, 2 tiles wide
- Flat terrain: Runway area must be single elevation level
- Pathway access required

```cpp
bool validate_aero_port_zone(const GridRect& zone,
                              ITerrainQueryable& terrain,
                              ITransportProvider& transport);
```

**Acceptance Criteria:**
- [ ] Validates minimum zone size (36 tiles)
- [ ] Detects valid runway area within zone
- [ ] Checks terrain flatness for runway
- [ ] Checks pathway accessibility (3-tile rule)

---

#### E8-009: Aqua Port Zone Validation
**Size:** M | **Component:** PortSystem | **Depends:** E8-003

Implement aqua port zone validation (water adjacency, dock requirements).

Requirements:
- Minimum zone size: 32 tiles (4x8)
- Water adjacency: Zone perimeter must border water tiles (deep_void, still_basin, flow_channel)
- Minimum 4 water-adjacent tiles for dock placement
- Pathway access required

```cpp
bool validate_aqua_port_zone(const GridRect& zone,
                              ITerrainQueryable& terrain,
                              ITransportProvider& transport);
```

**Acceptance Criteria:**
- [ ] Validates minimum zone size (32 tiles)
- [ ] Uses ITerrainQueryable to check water adjacency
- [ ] Requires minimum 4 water-adjacent perimeter tiles
- [ ] Checks pathway accessibility

---

#### E8-010: Port Capacity Calculation
**Size:** M | **Component:** PortSystem | **Depends:** E8-008, E8-009

Implement port capacity calculation from zone infrastructure.

**Aero Port Capacity:**
```
base_capacity = zone_tiles * 10
runway_bonus = has_runway ? 1.5 : 0.5
access_bonus = pathway_connected ? 1.0 : 0.0
aero_capacity = base_capacity * runway_bonus * access_bonus
```

**Aqua Port Capacity:**
```
base_capacity = zone_tiles * 15
dock_bonus = 1.0 + (dock_count * 0.2)
water_access = adjacent_water >= 4 ? 1.0 : 0.5
rail_bonus = has_rail_connection ? 1.5 : 1.0
aqua_capacity = base_capacity * dock_bonus * water_access * rail_bonus
```

**Acceptance Criteria:**
- [ ] Aero capacity scales with zone size and runway
- [ ] Aqua capacity scales with zone size, docks, and rail
- [ ] Maximum capacity capped per port type (2500 aero, 5000 aqua)

---

#### E8-011: Port Operational Status Check
**Size:** M | **Component:** PortSystem | **Depends:** E8-008, E8-009, E8-010

Implement port operational status check (determines if port functions).

A port is operational when:
- Zone validation passes (size, terrain requirements)
- Infrastructure requirements met (runway or dock)
- Pathway connected (within 3 tiles)
- Has non-zero capacity

```cpp
bool check_port_operational(EntityID port_entity,
                            ITerrainQueryable& terrain,
                            ITransportProvider& transport);
```

**Acceptance Criteria:**
- [ ] Updates PortComponent::is_operational flag
- [ ] Emits PortOperationalEvent on state change
- [ ] Non-operational ports provide no demand bonus or trade income

---

#### E8-012: Port Development Levels
**Size:** M | **Component:** PortSystem | **Depends:** E8-010

Track and update port zone development levels (0-4).

| Level | Name | Capacity Threshold | Visual |
|-------|------|-------------------|--------|
| 0 | Undeveloped | 0 | Empty zone |
| 1 | Basic | 100 | Small terminal |
| 2 | Standard | 500 | Medium terminal |
| 3 | Major | 2000 | Full terminal |
| 4 | International | 5000+ | Multiple terminals |

**Acceptance Criteria:**
- [ ] PortZoneComponent::zone_level updated based on capacity
- [ ] Level transitions emit PortUpgradedEvent
- [ ] Level affects visual representation (for RenderingSystem)

---

### External Connections

#### E8-013: Map Edge Detection
**Size:** M | **Component:** PortSystem | **Depends:** E8-004

Detect when pathways/rails reach map edge and create ExternalConnectionComponent.

```cpp
void scan_map_edges_for_connections();
bool is_map_edge(int32_t x, int32_t y) const;
MapEdge get_edge(int32_t x, int32_t y) const;
```

**Acceptance Criteria:**
- [ ] Detects pathway tiles at map edge
- [ ] Detects rail tiles at map edge
- [ ] Creates ExternalConnectionComponent entities
- [ ] Updates when infrastructure changes

---

#### E8-014: External Connection Capacity
**Size:** S | **Component:** PortSystem | **Depends:** E8-013

Calculate external connection trade and migration capacity.

| Connection Type | Trade Capacity/tile | Migration Capacity/tile |
|-----------------|---------------------|------------------------|
| Pathway | 100 | 50 |
| Rail | +200 bonus | +25 bonus |

**Acceptance Criteria:**
- [ ] Trade capacity calculated per connection
- [ ] Migration capacity calculated per connection
- [ ] Rail connections provide bonus to adjacent pathway connections

---

#### E8-015: NPC Neighbor Generation
**Size:** M | **Component:** PortSystem | **Depends:** E8-013

Generate AI-controlled neighbors at game start (1-4 based on map layout).

```cpp
struct NeighborData {
    uint8_t neighbor_id;
    MapEdge edge;
    std::string name;          // "Settlement Alpha", "Nexus Prime"
    float demand_factor;       // 0.5-1.5
    float supply_factor;       // 0.5-1.5
    TradeAgreementType relationship;
};
```

**Acceptance Criteria:**
- [ ] Generates 1-4 neighbors based on map edges
- [ ] Each neighbor has unique name and economic factors
- [ ] Neighbor data persists for save/load

---

### Demand Bonuses

#### E8-016: Global Demand Bonus Calculation
**Size:** M | **Component:** PortSystem | **Depends:** E8-010, E8-011

Calculate colony-wide demand bonuses from operational ports.

| Port Type | Zone Affected | Bonus per Port Size |
|-----------|---------------|---------------------|
| Aero Port | Exchange | Small +5, Medium +10, Large +15 |
| Aqua Port | Fabrication | Small +5, Medium +10, Large +15 |

**Maximum total port demand bonus: +30** (prevents infinite stacking)

**Acceptance Criteria:**
- [ ] Calculates bonus based on operational port capacity
- [ ] Aero boosts exchange demand
- [ ] Aqua boosts fabrication demand
- [ ] Total capped at +30

---

#### E8-017: Local Demand Bonus Calculation
**Size:** M | **Component:** PortSystem | **Depends:** E8-016

Calculate radius-based local demand bonuses near ports.

| Port Type | Zone Affected | Radius | Bonus |
|-----------|---------------|--------|-------|
| Aero Port | Habitation | 20 tiles | +5% |
| Aqua Port | Exchange | 25 tiles | +10% |

**Acceptance Criteria:**
- [ ] Position-specific bonus query
- [ ] Uses Manhattan distance for radius
- [ ] Stacks with global bonus (but total capped)

---

#### E8-018: DemandSystem Integration
**Size:** M | **Component:** PortSystem, DemandSystem | **Depends:** E8-007, E8-016, E8-017

Connect PortSystem demand bonuses to DemandSystem (Epic 10).

```cpp
// In DemandSystem::calculate_zone_demand()
float port_bonus = port_provider_->get_global_demand_bonus(zone_type, owner);
float local_port_bonus = port_provider_->get_local_demand_bonus(zone_type, x, y, owner);
```

**Acceptance Criteria:**
- [ ] DemandSystem queries IPortProvider for bonuses
- [ ] Demand calculation includes port modifiers
- [ ] Works with stub provider if PortSystem not yet implemented

---

## P1 - Important (Should Have)

### Trade Income

#### E8-019: Trade Income Calculation
**Size:** M | **Component:** PortSystem | **Depends:** E8-010, E8-015

Calculate trade income from ports and external connections.

```
port_trade_income = capacity * utilization * trade_multiplier * external_demand_factor

aero_income_per_unit = 0.8 credits/phase
aqua_income_per_unit = 0.6 credits/phase
```

**Acceptance Criteria:**
- [ ] Calculates income per port type
- [ ] Utilization affects income (underused ports earn less)
- [ ] Trade agreements affect multiplier

---

#### E8-020: EconomySystem Integration
**Size:** M | **Component:** PortSystem, EconomySystem | **Depends:** E8-007, E8-019

Report trade income to EconomySystem (Epic 11).

```cpp
struct TradeIncomeBreakdown {
    int64_t aero_income;
    int64_t aqua_income;
    int64_t trade_deal_bonuses;
    int64_t total;
};
```

**Canon Update Required:** Add trade_income to IncomeBreakdown data contract in interfaces.yaml.

**Acceptance Criteria:**
- [ ] EconomySystem queries port income each budget cycle
- [ ] Income breakdown available for UI display
- [ ] Trade deal costs reported as expenses

---

#### E8-021: Trade Income Breakdown UI Data
**Size:** S | **Component:** PortSystem | **Depends:** E8-020

Provide detailed trade income breakdown for UI (Epic 12).

**Acceptance Criteria:**
- [ ] Per-port income available
- [ ] Per-trade-deal income available
- [ ] Historical income tracking (last 12 phases)

---

### Neighbor Trade

#### E8-022: Trade Deal Negotiation
**Size:** M | **Component:** PortSystem | **Depends:** E8-005, E8-015

Implement trade deal creation with NPC neighbors.

| Deal Level | Cost/Cycle | Income Multiplier | Demand Bonus |
|------------|------------|-------------------|--------------|
| None | 0 | 0.5x | +0 |
| Basic | 1,000 cr | 0.8x | +5 |
| Enhanced | 2,500 cr | 1.0x | +10 |
| Premium | 5,000 cr | 1.2x | +15 |

**Acceptance Criteria:**
- [ ] Player can initiate trade deal via UI action
- [ ] Deal cost deducted from treasury each cycle
- [ ] Deal expires after specified duration
- [ ] Deal reverts to lower tier if payment fails

---

#### E8-023: Trade Deal Expiration
**Size:** S | **Component:** PortSystem | **Depends:** E8-022

Process trade agreement expiration and renewal.

**Acceptance Criteria:**
- [ ] Cycles remaining decremented each cycle
- [ ] Deal expires when cycles_remaining = 0
- [ ] Notification sent before expiration (5 cycles warning)
- [ ] Player can renew before expiration

---

#### E8-024: Migration Effects
**Size:** M | **Component:** PortSystem, PopulationSystem | **Depends:** E8-014

Calculate population migration from external connections.

**Inbound Migration:**
```
immigration_rate = migration_capacity * demand_factor * harmony_factor
max_per_cycle = 10 + (external_connections * 5)
```

**Outbound Migration:**
```
emigration_rate = migration_capacity * (disorder_index / 100) * tribute_penalty
```

**Acceptance Criteria:**
- [ ] PopulationSystem queries port migration data
- [ ] High harmony attracts immigration
- [ ] High disorder causes emigration
- [ ] External connections amplify migration effects

---

### Inter-Player Trade

#### E8-025: Trade Offer System
**Size:** L | **Component:** PortSystem | **Depends:** E8-005

Implement inter-player trade offer creation and acceptance.

```cpp
struct TradeOfferRequest {
    PlayerID target_player;
    TradeAgreementType proposed_type;
};

// Server validates both players have ports
// Server sends offer to target
// Target accepts or rejects
// On accept, create TradeAgreementComponent
```

**Acceptance Criteria:**
- [ ] Player can send trade offer to another player
- [ ] Target player receives notification
- [ ] Target can accept or reject
- [ ] Server-authoritative acceptance

---

#### E8-026: Trade Route Network Messages
**Size:** M | **Component:** PortSystem | **Depends:** E8-025

Define network messages for inter-player trade.

```cpp
// Client -> Server
TradeOfferRequest { target, proposed_type }
TradeOfferResponse { offer_id, accepted }
TradeCancelRequest { route_id }

// Server -> Client
TradeOfferNotification { offer_id, from_player, details }
TradeRouteEstablished { route_id, agreement }
TradeRouteCancelled { route_id, cancelled_by }
```

**Acceptance Criteria:**
- [ ] All message types defined and serializable
- [ ] Server broadcasts trade state changes to relevant clients
- [ ] Handles disconnection gracefully

---

#### E8-027: Trade Agreement Benefits
**Size:** M | **Component:** PortSystem | **Depends:** E8-025

Implement inter-player trade agreement benefits.

| Agreement Type | Demand Bonus (each) | Income Bonus |
|----------------|---------------------|--------------|
| Basic Trade | +3 | +5% port income |
| Advanced Trade | +6 | +10% port income |
| Strategic Partnership | +10 | +15% port income |

**Acceptance Criteria:**
- [ ] Both players receive symmetric benefits
- [ ] Demand bonus applies to relevant zones
- [ ] Income bonus applies to port trade income

---

### Events and Rendering

#### E8-028: Port Event Definitions
**Size:** S | **Component:** PortSystem

Define port-related events.

```cpp
struct PortOperationalEvent { EntityID port; bool is_operational; PlayerID owner; };
struct PortUpgradedEvent { EntityID port; uint8_t old_level; uint8_t new_level; };
struct PortCapacityChangedEvent { EntityID port; uint32_t old_capacity; uint32_t new_capacity; };
struct ExternalConnectionCreatedEvent { EntityID connection; MapEdge edge; ConnectionType type; };
struct ExternalConnectionRemovedEvent { EntityID connection; MapEdge edge; };
```

**Acceptance Criteria:**
- [ ] All events defined with proper fields
- [ ] Events registered with event system
- [ ] UISystem can subscribe for notifications

---

#### E8-029: Trade Agreement Event Definitions
**Size:** S | **Component:** PortSystem

Define trade-related events.

```cpp
struct TradeAgreementCreatedEvent { EntityID agreement; PlayerID party_a; PlayerID party_b; TradeAgreementType type; };
struct TradeAgreementExpiredEvent { EntityID agreement; PlayerID party_a; PlayerID party_b; };
struct TradeAgreementUpgradedEvent { EntityID agreement; TradeAgreementType old_type; TradeAgreementType new_type; };
struct TradeDealOfferReceivedEvent { uint32_t offer_id; PlayerID from; TradeAgreementType proposed; };
```

**Acceptance Criteria:**
- [ ] All events defined with proper fields
- [ ] UISystem displays trade notifications
- [ ] Events logged for replay/debug

---

#### E8-030: Port Zone Rendering Support
**Size:** M | **Component:** PortSystem, RenderingSystem

Provide data to RenderingSystem for port visualization.

**Required data:**
- Port zone boundaries
- Runway/dock positions
- Development level for model selection
- Operational status (glow vs no glow)

**Acceptance Criteria:**
- [ ] RenderingSystem can query port visual state
- [ ] Different visuals for development levels 0-4
- [ ] Aero ports show runway outline
- [ ] Aqua ports show dock structures

---

#### E8-031: Zone Type Extension for Ports
**Size:** M | **Component:** ZoneSystem, PortSystem | **Depends:** E8-001

Extend ZoneType enum to include port zones.

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

**Acceptance Criteria:**
- [ ] ZoneSystem accepts port zone types
- [ ] Zone designation UI includes port zones
- [ ] Port zones have distinct color/icon in overlays

---

## P2 - Enhancements (Later Phases)

### Advanced Features

#### E8-032: Port Infrastructure Upgrades
**Size:** M | **Component:** PortSystem | **Depends:** E8-012

Allow players to invest credits in port infrastructure upgrades.

| Upgrade Level | Cost | Effect |
|---------------|------|--------|
| Basic | Default | 1.0x trade multiplier |
| Upgraded Terminals | 50,000 cr | 1.2x trade multiplier |
| Advanced Logistics | 100,000 cr + rail | 1.4x trade multiplier |
| Premium Hub | 200,000 cr + full rail | 1.6x trade multiplier |

**Acceptance Criteria:**
- [ ] Players can purchase upgrades via UI
- [ ] Upgrades persist and affect trade multiplier
- [ ] Upgrade requirements (rail, etc.) validated

---

#### E8-033: Port Noise/Contamination Effects
**Size:** M | **Component:** PortSystem, ContaminationSystem

Implement negative effects of ports on surroundings.

**Aero Port:**
- Noise contamination in 10-tile radius
- Reduces sector_value

**Aqua Port:**
- Industrial contamination in 8-tile radius
- From cargo operations

**Acceptance Criteria:**
- [ ] ContaminationSystem queries port contamination sources
- [ ] Contamination affects sector_value
- [ ] Visual overlay shows port contamination

---

#### E8-034: Neighbor Relationship Evolution
**Size:** M | **Component:** PortSystem | **Depends:** E8-015

Allow neighbor relationships to evolve based on trade history.

```cpp
// Positive trades increase relationship_value
// High relationship unlocks better deal tiers
// Low relationship may trigger negative events (future)
```

**Acceptance Criteria:**
- [ ] Trade history tracked per neighbor
- [ ] Relationship affects available deal tiers
- [ ] UI shows relationship status

---

#### E8-035: Multiple Port Diminishing Returns
**Size:** S | **Component:** PortSystem | **Depends:** E8-016

Implement diminishing returns for multiple ports of same type.

```
second_port_bonus = first_port_bonus * 0.5
third_port_bonus = first_port_bonus * 0.25
```

**Acceptance Criteria:**
- [ ] Second same-type port gives 50% bonus
- [ ] Third same-type port gives 25% bonus
- [ ] Encourages diverse port strategy

---

### Testing

#### E8-036: Unit Tests - Port Validation
**Size:** L | **Component:** PortSystem

Comprehensive unit tests for port zone validation.

- Aero port size validation
- Aero port runway detection
- Aqua port water adjacency
- Aqua port dock requirements
- Capacity calculation accuracy

**Acceptance Criteria:**
- [ ] 80%+ code coverage on validation functions
- [ ] Edge cases tested (minimum size, no water, etc.)
- [ ] Regression tests for future changes

---

#### E8-037: Unit Tests - Trade Calculation
**Size:** L | **Component:** PortSystem

Unit tests for trade income and demand bonus calculation.

**Acceptance Criteria:**
- [ ] Trade income formula verified
- [ ] Demand bonus cap tested
- [ ] Trade agreement effects verified

---

#### E8-038: Integration Tests - Epic 10/11
**Size:** L | **Component:** PortSystem, DemandSystem, EconomySystem

Integration tests for DemandSystem and EconomySystem.

**Acceptance Criteria:**
- [ ] DemandSystem correctly queries port bonuses
- [ ] EconomySystem correctly incorporates port income
- [ ] Budget cycle includes port income/expenses

---

#### E8-039: Multiplayer Tests - Trade Routes
**Size:** L | **Component:** PortSystem

Multiplayer integration tests for inter-player trade.

**Acceptance Criteria:**
- [ ] Trade offers sync across clients
- [ ] Trade acceptance creates agreement on server
- [ ] Trade cancellation handled correctly
- [ ] Disconnection during trade handled

---

#### E8-040: Canon Update - PortSystem Documentation
**Size:** M | **Component:** Canon

Update canon files with Epic 8 additions.

**systems.yaml:**
- Add PortSystem with tick_priority 48
- Document owns/does_not_own/provides

**interfaces.yaml:**
- Add IPortProvider interface
- Add trade_income to IncomeBreakdown

**patterns.yaml:**
- Document port zone pattern (special zone category)

**Acceptance Criteria:**
- [ ] systems.yaml updated with PortSystem
- [ ] interfaces.yaml updated with IPortProvider
- [ ] patterns.yaml documents port zone pattern
- [ ] All canon files at v0.14.0 after update

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 10 | E8-001, E8-002, E8-003, E8-004, E8-005, E8-014, E8-021, E8-023, E8-028, E8-029, E8-035 |
| M | 22 | E8-006 through E8-020, E8-022, E8-024, E8-030, E8-031, E8-032, E8-033, E8-034, E8-040 |
| L | 8 | E8-025, E8-036, E8-037, E8-038, E8-039 |
| **Total** | **40** | |

---

## Dependency Graph

```
E8-001 (PortType)
    |
    +---> E8-002 (PortComponent)
    |         |
    +---> E8-003 (PortZoneComponent)
    |         |
    |         +---> E8-008 (Aero Validation)
    |         |         |
    |         +---> E8-009 (Aqua Validation)
    |                   |
    |                   +---> E8-010 (Capacity Calc)
    |                            |
    |                            +---> E8-011 (Operational Status)
    |                            |         |
    |                            +---> E8-012 (Development Levels)
    |                                      |
    |                                      +---> E8-016 (Global Demand)
    |                                      |         |
    |                                      +---> E8-017 (Local Demand)
    |                                                |
    |                                                +---> E8-018 (DemandSystem Integration)
    |
    +---> E8-004 (ExternalConnection)
              |
              +---> E8-013 (Map Edge Detection)
                        |
                        +---> E8-014 (Connection Capacity)
                        |
                        +---> E8-015 (NPC Neighbors)
                                  |
                                  +---> E8-022 (Trade Deals)
                                            |
                                            +---> E8-019 (Trade Income)
                                                      |
                                                      +---> E8-020 (EconomySystem Integration)

E8-005 (TradeAgreement)
    |
    +---> E8-025 (Inter-Player Trade)
              |
              +---> E8-026 (Network Messages)
              |
              +---> E8-027 (Trade Benefits)

E8-006 (PortSystem Skeleton)
    |
    +---> E8-007 (IPortProvider Interface)
              |
              +---> E8-018 (DemandSystem Integration)
              |
              +---> E8-020 (EconomySystem Integration)
```

---

## Canon Updates Required

After Epic 8 implementation, update canon to v0.14.0:

### systems.yaml Addition

```yaml
phase_4:
  epic_8_ports:
    name: "Ports & External Connections"
    systems:
      PortSystem:
        type: ecs_system
        tick_priority: 48
        owns:
          - Aero port (aero_port) zones
          - Aqua port (aqua_port) zones
          - External connection points
          - Trade routes (neighbor and inter-player)
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
          - TerrainSystem (water for aqua ports)
          - TransportSystem (pathway connections)
          - RailSystem (rail connections)
        multiplayer:
          authority: server
          per_player: true
```

### interfaces.yaml Addition

```yaml
ports:
  IPortProvider:
    description: "Provides port state queries for other systems"
    purpose: "Allows DemandSystem and EconomySystem to query port effects"
    methods:
      - name: get_port_capacity
      - name: get_demand_bonus
      - name: get_trade_income
    implemented_by:
      - PortSystem (Epic 8)
```

### data_contracts Update

```yaml
TradeIncomeBreakdown:
  fields:
    - aero_income: Credits
    - aqua_income: Credits
    - trade_deal_bonuses: Credits
    - total: Credits
```

---

**End of Epic 8 Tickets**
