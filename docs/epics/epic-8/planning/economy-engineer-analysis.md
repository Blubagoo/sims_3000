# Economy Engineer Analysis: Epic 8 - Ports & External Connections

**Agent:** Economy Engineer
**Epic:** 8 - Ports & External Connections
**Canon Version:** 0.13.0
**Date:** 2026-01-29
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 8 introduces PortSystem, which manages aero ports (airports) and aqua ports (seaports), external map-edge connections, and trade mechanics. From an economy perspective, this epic introduces **new revenue streams** (trade income) and **demand modifiers** (zone demand bonuses) that integrate with the existing EconomySystem from Epic 11.

The key economic mechanics are:
1. **Trade Income**: Ports generate passive credits based on capacity and utilization
2. **Demand Bonuses**: Aero ports boost exchange (commercial) demand; aqua ports boost fabrication (industrial) demand
3. **Neighbor Trade**: AI-controlled neighbor connections for single-player and small-multiplayer scenarios
4. **Inter-Player Trade**: Direct trading between players in multiplayer sessions
5. **Resource Sharing**: Potential for energy/fluid trading through specialized port connections

**Primary Economic Risk:** Ports must be balanced to be valuable investments without making them "must-build" infrastructure that trivializes other growth strategies.

Estimated Economic Work Items: **8-10 items** (part of larger Epic 8 scope)

---

## Table of Contents

1. [Trade Income Mechanics](#1-trade-income-mechanics)
2. [Demand Bonus Mechanics](#2-demand-bonus-mechanics)
3. [Neighbor Trade System](#3-neighbor-trade-system)
4. [Inter-Player Trade](#4-inter-player-trade)
5. [Integration with Epic 11](#5-integration-with-epic-11)
6. [Data Structures](#6-data-structures)
7. [Questions for Other Agents](#7-questions-for-other-agents)
8. [Risks & Concerns](#8-risks--concerns)
9. [Economic Balance Recommendations](#9-economic-balance-recommendations)

---

## 1. Trade Income Mechanics

### 1.1 Income Model Overview

Ports generate **passive trade income** each budget cycle based on:
- Port capacity (infrastructure investment)
- Port utilization (connected demand)
- External demand factor (neighbor relationships)
- Transport connectivity (pathway/rail access)

```
Trade Income = Port Capacity * Utilization Rate * Trade Multiplier * External Demand Factor
```

### 1.2 Port Types and Base Income

| Port Type | Terminology | Base Capacity | Max Capacity | Income per Capacity Unit |
|-----------|-------------|---------------|--------------|--------------------------|
| Aero Port | aero_port | 500 | 2,500 | 0.8 credits/phase |
| Aqua Port | aqua_port | 1,000 | 5,000 | 0.6 credits/phase |

**Design Rationale:**
- Aqua ports have higher capacity (bulk goods) but lower per-unit income
- Aero ports have lower capacity (high-value goods) but higher per-unit income
- Both ports can be expanded through additional construction

### 1.3 Utilization Rate Calculation

Port utilization depends on how much the port's services are actually used:

```cpp
float calculate_port_utilization(PortComponent& port, PlayerID owner) {
    // Base: connected zone buildings that would use the port
    uint32_t potential_users = 0;
    uint32_t actual_capacity = port.capacity;

    if (port.type == PortType::AeroPort) {
        // Aero ports serve exchange zones (commercial)
        potential_users = count_exchange_buildings(owner) * AERO_USERS_PER_EXCHANGE;
    } else {
        // Aqua ports serve fabrication zones (industrial)
        potential_users = count_fabrication_buildings(owner) * AQUA_USERS_PER_FABRICATION;
    }

    // Utilization = min(1.0, potential_users / capacity)
    // Capped at 100% - can't over-utilize
    float utilization = static_cast<float>(potential_users) / static_cast<float>(actual_capacity);
    return std::min(1.0f, utilization);
}
```

**Constants:**
- `AERO_USERS_PER_EXCHANGE` = 5 (each exchange building generates 5 units of airport demand)
- `AQUA_USERS_PER_FABRICATION` = 10 (each fabrication building generates 10 units of seaport demand)

### 1.4 Trade Multiplier

A modifier based on port infrastructure quality:

| Port Infrastructure Level | Multiplier | Requirement |
|---------------------------|------------|-------------|
| Basic | 1.0x | Default |
| Upgraded Terminals | 1.2x | 50,000 cr investment |
| Advanced Logistics | 1.4x | 100,000 cr + rail connection |
| Premium Hub | 1.6x | 200,000 cr + full rail network |

### 1.5 External Demand Factor

See [Section 3: Neighbor Trade System](#3-neighbor-trade-system) for AI neighbor demand.
See [Section 4: Inter-Player Trade](#4-inter-player-trade) for player trade demand.

Base factor without trade deals: 0.5 (50% of potential income)
With trade deals: 0.8-1.2 depending on deal quality.

### 1.6 Example Trade Income Calculation

**Scenario: Medium colony with basic aero port**

```
Port Capacity: 500
Exchange Buildings: 80 (potential users = 80 * 5 = 400)
Utilization: 400/500 = 0.8 (80%)
Trade Multiplier: 1.0 (basic)
External Demand Factor: 0.7 (one neighbor deal)

Trade Income = 500 * 0.8 * 1.0 * 0.7 * 0.8 credits/capacity
             = 224 credits/phase
```

**Annualized (12 phases/cycle):** 2,688 credits/cycle

**ROI Analysis:**
- Basic aero port construction cost: ~10,000 credits
- Annual income: ~2,700 credits
- Payback period: ~4 cycles (reasonable investment)

---

## 2. Demand Bonus Mechanics

### 2.1 Demand Bonus Overview

Ports provide **zone demand bonuses** to specific zone types:
- Aero ports boost **exchange (commercial)** demand
- Aqua ports boost **fabrication (industrial)** demand

These bonuses are reported to DemandSystem (Epic 10) via an interface.

### 2.2 Demand Bonus Formula

```cpp
int8_t calculate_port_demand_bonus(PlayerID owner, ZoneBuildingType zone_type) {
    int8_t bonus = 0;

    auto& ports = get_player_ports(owner);

    for (const auto& port : ports) {
        if (port.type == PortType::AeroPort && zone_type == ZoneBuildingType::Exchange) {
            // Aero port boosts exchange demand
            bonus += calculate_single_port_bonus(port);
        }
        else if (port.type == PortType::AquaPort && zone_type == ZoneBuildingType::Fabrication) {
            // Aqua port boosts fabrication demand
            bonus += calculate_single_port_bonus(port);
        }
    }

    // Cap total port bonus at +30
    return std::min(static_cast<int8_t>(30), bonus);
}

int8_t calculate_single_port_bonus(const PortComponent& port) {
    // Base bonus per port based on capacity tier
    // Small port: +5 demand
    // Medium port: +10 demand
    // Large port: +15 demand

    if (port.capacity >= 2000) return 15;  // Large
    if (port.capacity >= 1000) return 10;  // Medium
    return 5;  // Small
}
```

### 2.3 Demand Bonus Tiers

| Port Size | Capacity Range | Demand Bonus | Player Perception |
|-----------|----------------|--------------|-------------------|
| Small | 0-999 | +5 demand | "Slight boost to zone growth" |
| Medium | 1,000-1,999 | +10 demand | "Noticeable zone attraction" |
| Large | 2,000+ | +15 demand | "Major zone driver" |

### 2.4 Demand Bonus Cap

**Maximum port demand bonus: +30**

This prevents players from building unlimited ports for infinite demand. After 2-3 large ports, additional ports provide income but no additional demand benefit.

**Design Intent:** Ports should be ONE of several ways to boost demand, not THE way.

### 2.5 Integration with DemandSystem

The DemandSystem (Epic 10, tick_priority 52) queries port bonuses during demand calculation:

```cpp
// In DemandSystem::calculate_zone_demand()
int8_t base_demand = calculate_base_demand(zone_type, owner);
int8_t port_bonus = port_system_->get_demand_bonus(owner, zone_type);  // NEW: Epic 8
int8_t tribute_modifier = calculate_tribute_modifier(zone_type, owner);
int8_t service_modifier = calculate_service_modifier(zone_type, owner);

int8_t final_demand = base_demand + port_bonus + tribute_modifier + service_modifier;
```

---

## 3. Neighbor Trade System

### 3.1 Concept Overview

"Neighbors" are AI-controlled external entities at map edges that:
- Provide trade demand (increasing port utilization)
- Accept trade deals (modifying income and demand)
- Simulate economic relationships beyond the map

**This is NOT multiplayer trading.** Neighbor trade provides economic depth for solo play and fills the gap when multiplayer sessions have few players.

### 3.2 Neighbor Connection Points

Neighbors connect at **map edge connection points**:

| Connection Type | Infrastructure Required | Neighbor Type |
|-----------------|------------------------|---------------|
| Pathway Connection | Pathway to map edge | Land neighbor (trade goods) |
| Rail Connection | Rail line to map edge | Rail neighbor (bulk/passengers) |
| Aero Route | Aero port with runway | Aerial neighbor (high-value) |
| Aqua Route | Aqua port on coast | Maritime neighbor (bulk goods) |

### 3.3 Neighbor Generation

At game start, 1-4 neighbors are generated based on map layout:

```cpp
struct NeighborData {
    uint8_t neighbor_id;              // 1-4
    NeighborType type;                // Land, Rail, Aerial, Maritime
    std::string neighbor_name;        // "Settlement Alpha", "Nexus Prime", etc.

    // Economic state
    float demand_factor;              // 0.5 - 1.5 (how much they want our goods)
    float supply_factor;              // 0.5 - 1.5 (how much they have to offer)

    // Relationship
    TradeRelationship relationship;   // None, Basic, Enhanced, Premium
    int32_t relationship_value;       // Accumulated trade history
};
```

### 3.4 Trade Deal Mechanics

Players can negotiate trade deals with neighbors:

| Deal Level | Requirement | Export Bonus | Import Benefit | Duration |
|------------|-------------|--------------|----------------|----------|
| None | Default | 0.5x income | No demand bonus | Permanent |
| Basic Deal | 1,000 cr/cycle | 0.8x income | +5 demand | 10 cycles |
| Enhanced Deal | 2,500 cr/cycle | 1.0x income | +10 demand | 20 cycles |
| Premium Deal | 5,000 cr/cycle | 1.2x income | +15 demand, -10% import costs | 30 cycles |

**Trade Deal Payment:**
- Paid from EconomySystem each budget cycle
- Counts as "Other Expenses" category
- Deal expires if payment missed (reverts to Basic or None)

### 3.5 Trade Deal Formula

```cpp
int64_t calculate_trade_deal_income(const NeighborData& neighbor, const PortComponent& port) {
    float base_income = port.capacity * port.utilization * port.trade_multiplier;

    // Apply neighbor demand factor
    base_income *= neighbor.demand_factor;

    // Apply deal level multiplier
    switch (neighbor.relationship) {
        case TradeRelationship::None:     return base_income * 0.5;
        case TradeRelationship::Basic:    return base_income * 0.8;
        case TradeRelationship::Enhanced: return base_income * 1.0;
        case TradeRelationship::Premium:  return base_income * 1.2;
    }
}
```

### 3.6 Neighbor Trade Questions

**Question for @game-designer:**
- Should neighbor demand fluctuate over time? (Economic cycles)
- Can neighbors have disasters that affect trade?
- Should there be negative events (trade wars, embargoes)?

---

## 4. Inter-Player Trade

### 4.1 Multiplayer Trade Overview

In multiplayer, players can trade directly with each other through ports. This creates economic interdependence between colonies.

**Important:** This is OPTIONAL cooperative trading, NOT competitive resource denial.

### 4.2 Inter-Player Trade Types

| Trade Type | Mechanism | Economic Effect |
|------------|-----------|-----------------|
| Credits Transfer | Direct gift/payment | Treasury to treasury |
| Trade Agreement | Mutual demand boost | Both players get +demand |
| Resource Sharing | Energy/Fluid through specialized infrastructure | Pool sharing |

### 4.3 Credits Transfer

Simple point-to-point credit transfer:

```cpp
bool transfer_credits(PlayerID from, PlayerID to, int64_t amount) {
    auto& from_treasury = get_treasury(from);
    auto& to_treasury = get_treasury(to);

    // Cannot transfer into debt
    if (from_treasury.balance < amount) {
        return false;
    }

    from_treasury.balance -= amount;
    to_treasury.balance += amount;

    emit(CreditsTransferredEvent{from, to, amount});
    return true;
}
```

**Constraints:**
- Cannot transfer more credits than treasury holds
- 0 credit minimum transfer
- No transfer fee (sandbox-friendly)
- Both players must have aero or aqua port (trade infrastructure requirement)

### 4.4 Trade Agreements

Mutual trade agreements between players:

```cpp
struct PlayerTradeAgreement {
    PlayerID player_a;
    PlayerID player_b;
    TradeAgreementType type;          // BasicTrade, AdvancedTrade, StrategicPartnership
    uint32_t cycles_remaining;

    // Benefits
    int8_t demand_bonus_a;            // Bonus player A receives
    int8_t demand_bonus_b;            // Bonus player B receives
    float income_multiplier_a;
    float income_multiplier_b;
};
```

| Agreement Type | Demand Bonus (each) | Income Bonus | Cost (each) | Duration |
|----------------|---------------------|--------------|-------------|----------|
| Basic Trade | +3 | +5% port income | 500 cr/cycle | 5 cycles |
| Advanced Trade | +6 | +10% port income | 1,000 cr/cycle | 10 cycles |
| Strategic Partnership | +10 | +15% port income | 2,000 cr/cycle | 20 cycles |

### 4.5 Resource Sharing (Energy/Fluid)

**Deferred to Phase 2 / Post-MVP**

The concept: Players with adjacent territories and connected infrastructure could share energy or fluid pools.

Technical challenges:
- Cross-ownership pool merging
- Fair distribution algorithms
- Disconnect handling

**Recommendation:** Design the PortSystem interface to SUPPORT resource sharing, but don't implement until Epic 17+ polish phase.

```cpp
// Future interface - document but don't implement
class IResourceSharing {
public:
    virtual bool can_share_resource(PlayerID from, PlayerID to, ResourceType type) const = 0;
    virtual int64_t get_shareable_amount(PlayerID owner, ResourceType type) const = 0;
    virtual bool initiate_share(PlayerID from, PlayerID to, ResourceType type, int64_t amount) = 0;
};
```

---

## 5. Integration with Epic 11

### 5.1 EconomySystem Integration Points

PortSystem integrates with EconomySystem at several points:

| Integration Point | Direction | Mechanism |
|-------------------|-----------|-----------|
| Trade Income | Port -> Economy | PortSystem reports income to EconomySystem |
| Trade Deal Costs | Economy -> Port | EconomySystem deducts deal payments |
| Construction Costs | Economy -> Port | Port construction deducted from treasury |
| Credits Transfer | Economy <-> Port | Player-to-player transfers through ports |

### 5.2 Income Reporting Interface

```cpp
// PortSystem provides this to EconomySystem
struct PortIncomeReport {
    PlayerID owner;
    int64_t aero_port_income;         // Total from all aero ports
    int64_t aqua_port_income;         // Total from all aqua ports
    int64_t trade_deal_bonuses;       // Additional income from deals
    int64_t total_port_income;        // Sum
};

// Called by EconomySystem during budget cycle
PortIncomeReport PortSystem::calculate_port_income(PlayerID owner) const;
```

### 5.3 Expense Integration

Trade deal costs are reported as "Other Expenses":

```cpp
// In EconomySystem::calculate_total_expenses()
int64_t trade_deal_costs = port_system_->get_trade_deal_costs(player);
treasury.other_expenses += trade_deal_costs;
```

### 5.4 IncomeBreakdown Extension

Extend the Epic 11 IncomeBreakdown data contract:

```cpp
struct IncomeBreakdown {
    // Existing (Epic 11)
    Credits habitation_tribute;
    Credits exchange_tribute;
    Credits fabrication_tribute;

    // NEW (Epic 8)
    Credits aero_port_income;         // Trade income from airports
    Credits aqua_port_income;         // Trade income from seaports
    Credits trade_bonus_income;       // Bonus income from trade deals

    Credits total;
};
```

### 5.5 Tick Priority Consideration

PortSystem should run BEFORE EconomySystem but AFTER TransportSystem:

| Priority | System | Rationale |
|----------|--------|-----------|
| 45 | TransportSystem | Pathway connectivity needed for port utilization |
| 47 | RailSystem | Rail connectivity for advanced ports |
| **48** | **PortSystem** | Calculates trade income and demand bonuses |
| 52 | DemandSystem | Queries port demand bonuses |
| 60 | EconomySystem | Incorporates port income into treasury |

**Recommendation:** PortSystem tick_priority = 48

### 5.6 IEconomyQueryable Extension

Epic 11's IEconomyQueryable should be extended (or a parallel IPortQueryable created):

```cpp
// Option A: Extend IEconomyQueryable
class IEconomyQueryable {
    // ... existing methods ...

    // NEW: Port economy queries
    virtual int64_t get_port_income(PlayerID owner) const = 0;
    virtual int64_t get_trade_deal_costs(PlayerID owner) const = 0;
};

// Option B: New interface (preferred for separation of concerns)
class IPortEconomy {
public:
    virtual int64_t get_aero_port_income(PlayerID owner) const = 0;
    virtual int64_t get_aqua_port_income(PlayerID owner) const = 0;
    virtual int8_t get_demand_bonus(PlayerID owner, ZoneBuildingType zone_type) const = 0;
    virtual int64_t get_trade_deal_costs(PlayerID owner) const = 0;
    virtual bool has_active_trade_agreement(PlayerID owner, PlayerID partner) const = 0;
};
```

**Recommendation:** Create IPortEconomy interface, have PortSystem implement it. EconomySystem queries via this interface.

---

## 6. Data Structures

### 6.1 PortComponent

```cpp
struct PortComponent {
    // Core data
    PortType type;                    // 1 byte: AeroPort or AquaPort
    uint16_t capacity;                // 2 bytes: Current capacity (max 5000)
    uint8_t infrastructure_level;     // 1 byte: 0-3 (Basic to Premium Hub)

    // Utilization tracking
    float utilization_rate;           // 4 bytes: 0.0-1.0 (calculated each tick)

    // Runway/dock requirements (position-specific)
    uint8_t runway_length;            // 1 byte: Tiles of runway (aero port)
    uint8_t dock_count;               // 1 byte: Number of docks (aqua port)

    // Connectivity flags
    uint8_t connection_flags;         // 1 byte: Bitfield for connections

    uint8_t padding[3];               // 3 bytes: Alignment

    // Total: 16 bytes
};

// Connection flags
constexpr uint8_t PORT_CONNECTED_PATHWAY = 0x01;
constexpr uint8_t PORT_CONNECTED_RAIL = 0x02;
constexpr uint8_t PORT_CONNECTED_ENERGY = 0x04;
constexpr uint8_t PORT_CONNECTED_FLUID = 0x08;
```

### 6.2 TradeAgreementComponent

```cpp
struct TradeAgreementComponent {
    // Parties
    PlayerID party_a;                 // 1 byte: First player (or GAME_MASTER for neighbor)
    PlayerID party_b;                 // 1 byte: Second player

    // Agreement type
    TradeAgreementType agreement_type; // 1 byte: None/Basic/Enhanced/Premium

    // Duration
    uint16_t cycles_remaining;        // 2 bytes: Cycles until expiration

    // Economic terms
    int8_t demand_bonus_a;            // 1 byte: Demand bonus for party A
    int8_t demand_bonus_b;            // 1 byte: Demand bonus for party B
    uint8_t income_bonus_percent;     // 1 byte: Income multiplier (100 = 1.0x)

    // Costs
    int32_t cost_per_cycle_a;         // 4 bytes: Payment from party A
    int32_t cost_per_cycle_b;         // 4 bytes: Payment from party B

    // Total: 16 bytes
};

enum class TradeAgreementType : uint8_t {
    None = 0,
    BasicTrade = 1,
    AdvancedTrade = 2,
    StrategicPartnership = 3
};
```

### 6.3 NeighborComponent

For AI-controlled map-edge neighbors:

```cpp
struct NeighborComponent {
    // Identity
    uint8_t neighbor_id;              // 1 byte: 0-3
    NeighborType neighbor_type;       // 1 byte: Land/Rail/Aerial/Maritime

    // Position (map edge)
    uint8_t edge;                     // 1 byte: N/E/S/W
    uint16_t edge_position;           // 2 bytes: Position along edge

    // Economic factors
    uint8_t demand_factor_percent;    // 1 byte: 50-150 (0.5-1.5x)
    uint8_t supply_factor_percent;    // 1 byte: 50-150 (0.5-1.5x)

    // Relationship
    TradeRelationship relationship;   // 1 byte
    int16_t relationship_value;       // 2 bytes: Accumulated trade history (-1000 to +1000)

    // Name (stored separately in string table)
    uint16_t name_id;                 // 2 bytes: Index into neighbor name table

    uint8_t padding[2];               // 2 bytes: Alignment

    // Total: 16 bytes
};

enum class NeighborType : uint8_t {
    Land = 0,
    Rail = 1,
    Aerial = 2,
    Maritime = 3
};

enum class TradeRelationship : uint8_t {
    None = 0,
    Basic = 1,
    Enhanced = 2,
    Premium = 3
};
```

### 6.4 Memory Budget

| Structure | Per-Entity | Typical Count | Total |
|-----------|------------|---------------|-------|
| PortComponent | 16 bytes | 2-8 ports/player | 128 bytes/player |
| TradeAgreementComponent | 16 bytes | 4-12 agreements/game | 192 bytes |
| NeighborComponent | 16 bytes | 1-4 neighbors | 64 bytes |
| **Total Economic Data** | | | ~400 bytes/player |

Negligible memory footprint.

---

## 7. Questions for Other Agents

### @systems-architect

1. **Tick Priority:** I propose PortSystem at tick_priority 48 (after TransportSystem 45, RailSystem 47, before DemandSystem 52, EconomySystem 60). Does this fit the system ordering?

2. **PortSystem Location:** Should PortSystem be a new system, or an extension of TransportSystem? I lean toward separate system for clear responsibility.

3. **Cross-Ownership Trade:** For inter-player trade agreements, how do we handle the two-player state? Is this a shared component, or each player has their own view of the agreement?

4. **IPortEconomy Interface:** Should port economy be a separate interface (IPortEconomy) or methods on IEconomyQueryable? I recommend separate for cleaner Epic 8/11 boundaries.

5. **Map Edge Detection:** How does PortSystem know where map edges are for neighbor connections? Does TerrainSystem provide this, or is there a MapBoundaryComponent?

### @game-designer

6. **Port Construction Cost:** What should the base construction costs be?
   - Small aero port: 10,000 cr?
   - Large aero port: 50,000 cr?
   - Small aqua port: 15,000 cr?
   - Large aqua port: 75,000 cr?

7. **Demand Bonus Cap:** Is +30 maximum port demand bonus appropriate? Higher cap favors port-heavy strategies.

8. **Trade Deal Flavoring:** Should trade deals have flavor text and unique names? ("The Spice Route Agreement", "Northern Transit Compact")

9. **Neighbor Personality:** Should AI neighbors have personalities that affect trade? (e.g., "aggressive negotiators" vs "friendly partners")

10. **Trade Failures:** In a sandbox game, should trade deals ever fail or have negative events? Or should all trade be reliably positive?

11. **Visual Trade Routes:** Should trade routes be visualized (ships, aircraft flying to/from ports)? Or is this pure economic simulation?

### @zone-engineer

12. **Port Zoning:** Are ports zoned (like habitation/exchange/fabrication) or placed as buildings? The epic plan says "zoned," which has implications for construction flow.

13. **Runway/Dock Requirements:** Do aero ports need specific runway tile patterns? Do aqua ports need specific water-adjacent configurations?

### @transport-engineer

14. **Pathway Access Bonus:** Should ports get income bonuses for pathway connectivity tier? (Basic pathway vs. transit corridor)

15. **Rail Integration:** How does rail connection affect port capacity/income? Proposed: +20% capacity bonus for rail-connected ports.

---

## 8. Risks & Concerns

### 8.1 Economic Balance Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Ports too powerful | Trivialize other growth strategies | Cap demand bonus at +30; balance income vs. construction cost |
| Ports too weak | Never worth building | Ensure 3-5 cycle ROI on basic ports |
| Trade deals too complex | Player confusion | Keep deal types simple (3-4 tiers max) |
| Inter-player trade exploitation | Dominant player funnels resources | Credits transfer limits; no resource sharing MVP |

### 8.2 Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Multiplayer desync | Trade agreements out of sync | Server-authoritative trade state |
| Performance overhead | Many ports slow simulation | Batch port calculations; cache utilization rates |
| Neighbor AI complexity | Unpredictable neighbor behavior | Simple, deterministic neighbor logic |

### 8.3 Scope Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Feature creep | Epic 8 expands beyond scope | Defer resource sharing to post-MVP |
| Tight Epic 11 coupling | Changes to economy break ports | Use interface abstraction (IPortEconomy) |
| UI complexity | Trade UI overwhelms players | Phase UI: basic budget window first, trade panel later |

### 8.4 Sandbox Philosophy Risk

**Concern:** Trade mechanics could introduce "fail states" if trade deals create dependencies that punish players when broken.

**Mitigation:**
- All trade is opt-in (no mandatory trade)
- Trade deal expiration is gradual (reduced benefits, not sudden penalty)
- No "trade war" negative events in MVP
- Player-to-player trade is always cooperative, never competitive resource denial

---

## 9. Economic Balance Recommendations

### 9.1 Port Construction Costs

| Port Type | Size | Construction Cost | Maintenance/Phase | Capacity |
|-----------|------|-------------------|-------------------|----------|
| Aero Port | Small | 10,000 cr | 200 cr | 500 |
| Aero Port | Medium | 30,000 cr | 500 cr | 1,500 |
| Aero Port | Large | 75,000 cr | 1,000 cr | 2,500 |
| Aqua Port | Small | 15,000 cr | 250 cr | 1,000 |
| Aqua Port | Medium | 45,000 cr | 600 cr | 3,000 |
| Aqua Port | Large | 100,000 cr | 1,200 cr | 5,000 |

### 9.2 Return on Investment Targets

| Port Size | Target Payback | Annual Income (at 80% util) |
|-----------|----------------|----------------------------|
| Small Aero | 4-5 cycles | 2,000-2,500 cr/cycle |
| Medium Aero | 5-6 cycles | 5,000-6,000 cr/cycle |
| Large Aero | 6-7 cycles | 11,000-12,000 cr/cycle |
| Small Aqua | 5-6 cycles | 2,500-3,000 cr/cycle |
| Medium Aqua | 6-7 cycles | 7,000-7,500 cr/cycle |
| Large Aqua | 7-8 cycles | 12,000-14,000 cr/cycle |

**Design Intent:** Ports are solid investments with reasonable ROI, but not so lucrative that they dominate the economy.

### 9.3 Trade Deal Costs vs. Benefits

| Deal Type | Cost/Cycle | Income Bonus | Demand Bonus | Net Benefit Threshold |
|-----------|------------|--------------|--------------|----------------------|
| Basic Neighbor | 1,000 cr | +30% port income | +5 | Break-even at 3,333 cr port income |
| Enhanced Neighbor | 2,500 cr | +50% port income | +10 | Break-even at 5,000 cr port income |
| Premium Neighbor | 5,000 cr | +70% port income | +15 | Break-even at 7,142 cr port income |
| Basic Player | 500 cr each | +5% each | +3 each | Always positive (mutual benefit) |
| Advanced Player | 1,000 cr each | +10% each | +6 each | Always positive |
| Strategic Player | 2,000 cr each | +15% each | +10 each | Always positive |

**Design Intent:** Player-to-player trade is always mutually beneficial. Neighbor trade requires sufficient port infrastructure to be worthwhile.

### 9.4 Demand Bonus Guidelines

| Source | Bonus Range | Notes |
|--------|-------------|-------|
| Small Port | +5 | Modest boost |
| Medium Port | +10 | Noticeable |
| Large Port | +15 | Significant |
| Basic Trade Deal | +5 | Stacks with port |
| Enhanced Trade Deal | +10 | Major boost |
| Premium Trade Deal | +15 | Cap at total +30 |
| **Maximum Total** | **+30** | Prevents infinite stacking |

### 9.5 Comparison to Other Income Sources

To ensure ports are balanced with Epic 11's tribute system:

| Income Source | Typical Medium Colony Income/Cycle |
|---------------|-----------------------------------|
| Habitation Tribute | 15,000-20,000 cr |
| Exchange Tribute | 15,000-20,000 cr |
| Fabrication Tribute | 5,000-10,000 cr |
| **Port Income** | 5,000-10,000 cr |
| **Total** | 45,000-60,000 cr/cycle |

**Port income should be ~10-20% of total income.** This makes ports valuable without dominating the economy.

---

## Appendix A: Canonical Terminology Reference

| Human Term | Canonical (Alien) Term |
|------------|------------------------|
| Airport | Aero port |
| Seaport | Aqua port |
| Trade | Exchange (also used for commercial) |
| Import/Export | Inbound/Outbound trade |
| Neighbor city | Adjacent settlement |
| Trade deal | Exchange compact |
| Trade route | Transit corridor (shared term) |

---

## Appendix B: Example Budget Cycle with Ports

**Scenario:** Player with medium colony, 1 aero port (medium), 1 aqua port (small), Basic trade deal with 1 neighbor.

```
=== BUDGET CYCLE: Cycle 15, Phase 3 ===

INCOME:
  Habitation Tribute:     18,000 cr
  Exchange Tribute:       16,000 cr
  Fabrication Tribute:     7,000 cr
  Aero Port Income:        4,500 cr  <-- NEW (Epic 8)
  Aqua Port Income:        2,200 cr  <-- NEW (Epic 8)
  Trade Deal Bonus:          800 cr  <-- NEW (Epic 8)
  --------------------------------
  TOTAL INCOME:           48,500 cr

EXPENSES:
  Infrastructure:          3,500 cr
  Services:                5,000 cr
  Energy Nexuses:          1,500 cr
  Port Maintenance:          750 cr  <-- NEW (Epic 8)
  Trade Deal Payment:      1,000 cr  <-- NEW (Epic 8)
  --------------------------------
  TOTAL EXPENSES:         11,750 cr

NET BUDGET:              +36,750 cr
```

---

## Appendix C: PortSystem Tick Flow

```cpp
void PortSystem::tick(float delta_time) {
    // Phase 1: Update port utilization for each player
    for (PlayerID player = 1; player <= num_players_; ++player) {
        update_port_utilization(player);
    }

    // Phase 2: Process trade agreements (check expiration, apply effects)
    process_trade_agreements();

    // Phase 3: Update neighbor relationships (AI neighbors only)
    update_neighbor_relationships();

    // Phase 4: Calculate demand bonuses (cached for DemandSystem queries)
    cache_demand_bonuses();

    // Phase 5: Calculate trade income (cached for EconomySystem queries)
    cache_trade_income();
}
```

---

**End of Economy Engineer Analysis: Epic 8 - Ports & External Connections**
