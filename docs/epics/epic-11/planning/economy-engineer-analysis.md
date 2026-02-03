# Economy Engineer Analysis: Epic 11 - Financial System

**Agent:** Economy Engineer
**Epic:** 11 - Financial System
**Canon Version:** 0.11.0
**Date:** 2026-01-29
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 11 implements the EconomySystem that manages colony finances: treasury, tribute (tax) collection, maintenance expenses, budget allocation, credit advances (bonds), and ordinances. This epic replaces the Epic 10 `IEconomyQueryable` stub with a full implementation.

The EconomySystem runs at **tick_priority 60** (after ServicesSystem 55, before DisorderSystem 70) and calculates per-player financial state each budget cycle. The key technical challenges are:

1. **Tribute Collection Formula**: Balancing tribute rates with zone demand impact
2. **Maintenance Cost Aggregation**: Summing infrastructure and service costs
3. **Budget Cycle Timing**: Determining collection/payment frequency (per-phase vs per-tick)
4. **Credit Advance System**: Interest calculation and repayment mechanics

Estimated scope: **22 work items** (within 15-25 target)

---

## Table of Contents

1. [Technical Work Items](#1-technical-work-items)
2. [Treasury Formulas](#2-treasury-formulas)
3. [Budget Cycle Design](#3-budget-cycle-design)
4. [Data Structures](#4-data-structures)
5. [Credit Advance System](#5-credit-advance-system)
6. [Ordinances](#6-ordinances)
7. [Questions for Other Agents](#7-questions-for-other-agents)
8. [Testing Strategy](#8-testing-strategy)

---

## 1. Technical Work Items

### Phase 1: Components and Interfaces (5 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-001 | TreasuryComponent definition | Per-player treasury state (balance, income, expenses) | S |
| ECO-002 | TaxableComponent definition | Per-building tribute generation data | S |
| ECO-003 | MaintenanceCostComponent definition | Per-entity maintenance cost tracking | S |
| ECO-004 | IEconomyQueryable implementation | Replace Epic 10 stub with real implementation | M |
| ECO-005 | BudgetCategory enum | Categories for income/expense breakdown | S |

### Phase 2: EconomySystem Core (5 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-006 | EconomySystem class skeleton | ISimulatable at priority 60, tick structure | M |
| ECO-007 | Per-player treasury tracking | Multiple treasuries, starting balance | S |
| ECO-008 | Budget cycle timing | Phase-based collection (every ~500 ticks) | M |
| ECO-009 | Income aggregation | Sum tribute from all sources | M |
| ECO-010 | Expense aggregation | Sum maintenance from all sources | M |

### Phase 3: Tribute Collection (4 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-011 | Per-building tribute calculation | Base value * tribute_rate * occupancy * sector_value | L |
| ECO-012 | Zone-specific tribute rates | Separate rates for habitation/exchange/fabrication | M |
| ECO-013 | Tribute rate effects on demand | High tribute reduces zone demand (DemandSystem integration) | M |
| ECO-014 | Tribute collection events | TributeCollectedEvent per phase | S |

### Phase 4: Maintenance Costs (3 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-015 | Infrastructure maintenance | Roads, conduits, pipes cost per-tile | M |
| ECO-016 | Service maintenance | Service building operating costs | M |
| ECO-017 | Building maintenance | Zone buildings have base maintenance | S |

### Phase 5: Credit Advances (3 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-018 | Credit advance (bond) data structures | Bonds with principal, interest, term | M |
| ECO-019 | Credit advance issuance | Take loan, add to treasury, create bond | M |
| ECO-020 | Credit advance repayment | Per-phase interest + principal payments | M |

### Phase 6: Integration and Testing (2 items)

| ID | Item | Description | Estimate |
|----|------|-------------|----------|
| ECO-021 | Network sync for treasury state | Sync balance, income, expenses to clients | M |
| ECO-022 | Budget window data provider | Data structure for UI budget display | S |

### Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 7 | ECO-001, ECO-002, ECO-003, ECO-005, ECO-007, ECO-014, ECO-017, ECO-022 |
| M | 13 | ECO-004, ECO-006, ECO-008, ECO-009, ECO-010, ECO-012, ECO-013, ECO-015, ECO-016, ECO-018, ECO-019, ECO-020, ECO-021 |
| L | 1 | ECO-011 |
| **Total** | **22** | |

---

## 2. Treasury Formulas

### 2.1 Core Treasury Equation

```
treasury_balance_change = total_income - total_expenses

total_income = zone_tribute + other_income
total_expenses = infrastructure_maintenance + service_maintenance + building_maintenance + bond_payments
```

### 2.2 Tribute Collection Formula

Each taxable building generates tribute based on multiple factors:

```cpp
int64_t calculate_building_tribute(
    EntityID building,
    const TaxableComponent& taxable,
    const BuildingOccupancyComponent& occupancy,
    uint8_t sector_value,
    float tribute_rate
) {
    // Base tribute value (depends on building type and level)
    int64_t base_value = taxable.base_tribute_value;

    // Occupancy factor: 0.0 at empty, 1.0 at full capacity
    float occupancy_factor = static_cast<float>(occupancy.current_occupancy)
                           / static_cast<float>(occupancy.capacity);

    // Sector value factor: 0.5x at value 0, 2.0x at value 255
    // value 128 (neutral) = 1.0x
    float value_factor = 0.5f + (sector_value / 255.0f) * 1.5f;

    // Apply tribute rate (0-20%)
    float rate_factor = tribute_rate / 100.0f;

    // Final tribute
    return static_cast<int64_t>(base_value * occupancy_factor * value_factor * rate_factor);
}
```

### 2.3 Base Tribute Values by Zone Type

| Zone Type | Density | Base Tribute/Phase | Notes |
|-----------|---------|-------------------|-------|
| Habitation | Low | 50 credits | Residential income tax |
| Habitation | High | 200 credits | Higher density = more taxpayers |
| Exchange | Low | 100 credits | Commercial sales tax |
| Exchange | High | 400 credits | More business activity |
| Fabrication | Low | 75 credits | Industrial output tax |
| Fabrication | High | 300 credits | Larger facilities |

**Total Revenue Example (1000 beings, balanced zones):**
- ~200 habitation buildings at avg 75 credits = 15,000/phase
- ~100 exchange buildings at avg 150 credits = 15,000/phase
- ~50 fabrication buildings at avg 100 credits = 5,000/phase
- **Total: ~35,000 credits/phase** at 7% default rate

### 2.4 Maintenance Cost Formula

Maintenance is simpler - flat cost per entity type:

```cpp
int64_t calculate_maintenance_cost(EntityID entity) {
    // Infrastructure
    if (has_component<RoadComponent>(entity)) {
        return ROAD_MAINTENANCE_PER_TILE;  // 5 credits/tile/phase
    }
    if (has_component<EnergyConduitComponent>(entity)) {
        return CONDUIT_MAINTENANCE_PER_TILE;  // 2 credits/tile/phase
    }
    if (has_component<FluidConduitComponent>(entity)) {
        return PIPE_MAINTENANCE_PER_TILE;  // 3 credits/tile/phase
    }

    // Service buildings
    if (has_component<ServiceProviderComponent>(entity)) {
        return get_service_maintenance(entity);  // 100-300 credits/phase
    }

    // Energy producers
    if (has_component<EnergyProducerComponent>(entity)) {
        return get_energy_nexus_maintenance(entity);  // 50-500 credits/phase
    }

    // Zone buildings (minimal maintenance)
    if (has_component<ZoneBuildingComponent>(entity)) {
        return ZONE_BUILDING_BASE_MAINTENANCE;  // 5 credits/phase
    }

    return 0;
}
```

### 2.5 Maintenance Cost Table

| Entity Type | Maintenance/Phase | Notes |
|-------------|-------------------|-------|
| Pathway (road) tile | 5 credits | Traffic wear |
| Energy conduit tile | 2 credits | Low maintenance |
| Fluid conduit tile | 3 credits | Pipe maintenance |
| Rail track tile | 8 credits | Higher maintenance |
| Enforcer post | 100 credits | Staffing costs |
| Hazard response post | 120 credits | Equipment upkeep |
| Medical nexus | 300 credits | Expensive to operate |
| Learning center | 200 credits | Teacher salaries |
| Carbon nexus | 200 credits | Fuel and repairs |
| Petrochemical nexus | 150 credits | Moderate upkeep |
| Nuclear nexus | 400 credits | High safety costs |
| Fluid extractor | 75 credits | Pump maintenance |
| Zone building (each) | 5 credits | Basic upkeep |

**Total Expenses Example (medium colony):**
- 500 road tiles = 2,500/phase
- 200 conduit tiles = 400/phase
- 200 pipe tiles = 600/phase
- 5 service buildings = 1,000/phase
- 3 energy nexuses = 500/phase
- 200 zone buildings = 1,000/phase
- **Total: ~6,000 credits/phase**

### 2.6 Net Budget Calculation

```
Net Budget = Total Income - Total Expenses
           = 35,000 - 6,000
           = +29,000 credits/phase (healthy surplus)
```

At 7% default tribute rate with balanced development, a medium colony runs a healthy surplus.

---

## 3. Budget Cycle Design

### 3.1 Timing Options Considered

#### Option A: Per-Tick Processing (REJECTED)

```
Each tick: collect small income, pay small expenses
Pro: Smooth treasury changes
Con: Excessive computation, hard to display meaningful numbers
```

#### Option B: Per-Phase Processing (RECOMMENDED)

```
Each phase (~500 ticks / 25 seconds):
  - Collect all tribute
  - Pay all maintenance
  - Process bond payments
  - Update treasury balance
Pro: Meaningful budget cycles, classic city builder feel
Con: Large balance swings
```

#### Option C: Per-Cycle (Year) Processing (REJECTED)

```
Once per cycle (~2000 ticks / 100 seconds):
  - Process all finances
Pro: Simple
Con: Too long between updates, player loses financial feedback
```

### 3.2 Recommendation: Per-Phase Budget Cycle

Budget processing occurs at **phase transitions** (12 times per cycle):

```cpp
void EconomySystem::tick(float delta_time) {
    const uint8_t current_phase = simulation_time_->get_current_phase();

    for (PlayerID player = 1; player <= num_players_; ++player) {
        auto& treasury = get_treasury(player);

        // Check for phase transition
        if (current_phase != treasury.last_processed_phase) {
            process_budget_cycle(player);
            treasury.last_processed_phase = current_phase;
        }
    }

    // Continuous: process any immediate transactions (building purchases, etc.)
    process_pending_transactions();
}

void EconomySystem::process_budget_cycle(PlayerID player) {
    auto& treasury = get_treasury(player);

    // Phase 1: Calculate income
    int64_t phase_income = calculate_total_income(player);

    // Phase 2: Calculate expenses
    int64_t phase_expenses = calculate_total_expenses(player);

    // Phase 3: Process bond payments
    int64_t bond_payments = process_bond_payments(player);
    phase_expenses += bond_payments;

    // Phase 4: Update treasury
    treasury.balance += phase_income - phase_expenses;

    // Phase 5: Update history
    treasury.last_income = phase_income;
    treasury.last_expenses = phase_expenses;

    // Phase 6: Check for deficit/bankruptcy
    if (treasury.balance < 0) {
        handle_deficit(player);
    }

    // Phase 7: Emit event
    emit(BudgetCycleCompletedEvent{player, phase_income, phase_expenses, treasury.balance});
}
```

### 3.3 Budget Timing Summary

| Event | Frequency | Purpose |
|-------|-----------|---------|
| Tribute collection | Per phase | Main income source |
| Maintenance payment | Per phase | Operating costs |
| Bond payment | Per phase | Debt service |
| Construction cost | Immediate | When building placed |
| Demolition refund | Immediate | When building demolished |
| Tile purchase | Immediate | When tile purchased from Game Master |

### 3.4 Deficit Handling

When treasury goes negative:

```cpp
void EconomySystem::handle_deficit(PlayerID player) {
    auto& treasury = get_treasury(player);

    // Warning at -5000 credits
    if (treasury.balance < -5000 && !treasury.deficit_warning_sent) {
        emit(DeficitWarningEvent{player, treasury.balance});
        treasury.deficit_warning_sent = true;
    }

    // Forced bond at -10000 credits (emergency loan)
    if (treasury.balance < -10000 && !treasury.emergency_bond_active) {
        issue_emergency_bond(player, 25000);  // Auto-loan to cover deficit
        emit(EmergencyBondIssuedEvent{player, 25000});
    }

    // Bankruptcy at -50000 credits (game mechanic TBD)
    if (treasury.balance < -50000) {
        emit(BankruptcyEvent{player});
        // @game-designer: What happens at bankruptcy?
        // Options: forced restart, Game Master takeover, reduced services
    }
}
```

---

## 4. Data Structures

### 4.1 TreasuryComponent

Per-player financial state:

```cpp
struct TreasuryComponent {
    // Current state
    int64_t balance;                      // 8 bytes: Current treasury balance (can be negative)

    // Last cycle values (for UI display)
    int64_t last_income;                  // 8 bytes: Total income last phase
    int64_t last_expenses;                // 8 bytes: Total expenses last phase

    // Income breakdown
    int64_t habitation_tribute;           // 8 bytes: From residential zones
    int64_t exchange_tribute;             // 8 bytes: From commercial zones
    int64_t fabrication_tribute;          // 8 bytes: From industrial zones
    int64_t other_income;                 // 8 bytes: Special/external income

    // Expense breakdown
    int64_t infrastructure_maintenance;   // 8 bytes: Roads, conduits, pipes
    int64_t service_maintenance;          // 8 bytes: Service buildings
    int64_t energy_maintenance;           // 8 bytes: Power plants
    int64_t bond_payments;                // 8 bytes: Debt service
    int64_t other_expenses;               // 8 bytes: Ordinances, etc.

    // Tribute rates (0-20 representing 0-20%)
    uint8_t habitation_tribute_rate;      // 1 byte
    uint8_t exchange_tribute_rate;        // 1 byte
    uint8_t fabrication_tribute_rate;     // 1 byte

    // State tracking
    uint8_t last_processed_phase;         // 1 byte
    bool deficit_warning_sent;            // 1 byte
    bool emergency_bond_active;           // 1 byte

    uint8_t padding[2];                   // 2 bytes: Alignment

    // Total: 90 bytes
};
```

### 4.2 TaxableComponent

Attached to zone buildings that generate tribute:

```cpp
struct TaxableComponent {
    int64_t base_tribute_value;           // 8 bytes: Base tribute before modifiers
    ZoneBuildingType zone_type;           // 1 byte: For rate lookup
    uint8_t density_level;                // 1 byte: 0=low, 1=high
    uint8_t padding[6];                   // 6 bytes: Alignment

    // Total: 16 bytes
};
```

### 4.3 MaintenanceCostComponent

Attached to entities with maintenance costs:

```cpp
struct MaintenanceCostComponent {
    int32_t base_cost;                    // 4 bytes: Base maintenance per phase
    float cost_multiplier;                // 4 bytes: Modifier (age, damage, etc.)

    // Total: 8 bytes
};
```

### 4.4 CreditAdvanceComponent

Tracks active bonds/loans:

```cpp
struct CreditAdvanceComponent {
    int64_t principal;                    // 8 bytes: Original loan amount
    int64_t remaining_principal;          // 8 bytes: Amount still owed
    int32_t interest_rate_basis_points;   // 4 bytes: Interest rate * 100 (e.g., 750 = 7.5%)
    int32_t term_phases;                  // 4 bytes: Total phases for repayment
    int32_t phases_remaining;             // 4 bytes: Phases left
    bool is_emergency_bond;               // 1 byte: True if forced loan
    uint8_t padding[3];                   // 3 bytes: Alignment

    // Total: 32 bytes
};
```

### 4.5 BudgetCategory Enum

```cpp
enum class BudgetCategory : uint8_t {
    // Income categories
    HabitationTribute = 0,
    ExchangeTribute = 1,
    FabricationTribute = 2,
    OtherIncome = 3,

    // Expense categories
    InfrastructureMaintenance = 10,
    ServiceMaintenance = 11,
    EnergyMaintenance = 12,
    BondPayments = 13,
    OrdinanceCosts = 14,
    OtherExpenses = 15
};
```

### 4.6 Data Structure Memory Budget

| Structure | Per-Player | 4 Players |
|-----------|------------|-----------|
| TreasuryComponent | 90 bytes | 360 bytes |
| CreditAdvanceComponent (max 5 bonds) | 160 bytes | 640 bytes |
| **Total per-player** | ~250 bytes | ~1 KB |

Plus per-entity components:
- TaxableComponent: 16 bytes * ~500 buildings = 8 KB
- MaintenanceCostComponent: 8 bytes * ~1000 entities = 8 KB

**Total Epic 11 Memory: ~17 KB** (negligible)

---

## 5. Credit Advance System

### 5.1 Bond Types

| Bond Type | Amount | Interest | Term | Availability |
|-----------|--------|----------|------|--------------|
| Small Advance | 5,000 | 5% | 12 phases | Always |
| Standard Bond | 25,000 | 7.5% | 24 phases | Balance > -5000 |
| Large Bond | 100,000 | 10% | 48 phases | Population > 5000 |
| Emergency Loan | 25,000 | 15% | 12 phases | Auto at deficit |

### 5.2 Bond Mechanics

```cpp
bool EconomySystem::issue_bond(PlayerID player, BondType type) {
    const BondConfig& config = get_bond_config(type);
    auto& treasury = get_treasury(player);

    // Validation
    if (treasury.balance < config.min_balance_requirement) {
        return false;  // Can't take more debt
    }
    if (get_total_debt(player) >= get_max_debt(player)) {
        return false;  // Debt ceiling reached
    }

    // Create bond entity
    EntityID bond = registry_.create();
    registry_.emplace<CreditAdvanceComponent>(bond,
        config.principal,
        config.principal,
        config.interest_rate_basis_points,
        config.term_phases,
        config.term_phases,
        false
    );
    registry_.emplace<OwnershipComponent>(bond, player);

    // Add to treasury
    treasury.balance += config.principal;

    emit(BondIssuedEvent{player, bond, config.principal});
    return true;
}
```

### 5.3 Bond Payment Calculation

```cpp
int64_t calculate_bond_payment(const CreditAdvanceComponent& bond) {
    // Simple amortization: equal payments each phase
    // Payment = Principal / Term + Interest on Remaining

    int64_t principal_payment = bond.principal / bond.term_phases;

    int64_t interest_payment = (bond.remaining_principal * bond.interest_rate_basis_points)
                              / (10000 * 12);  // Convert basis points to per-phase

    return principal_payment + interest_payment;
}
```

### 5.4 Example Bond Payment Schedule (25,000 at 7.5%, 24 phases)

| Phase | Principal Payment | Interest | Total Payment | Remaining |
|-------|-------------------|----------|---------------|-----------|
| 1 | 1,042 | 156 | 1,198 | 23,958 |
| 2 | 1,042 | 150 | 1,192 | 22,916 |
| ... | ... | ... | ... | ... |
| 24 | 1,042 | 7 | 1,049 | 0 |
| **Total** | 25,000 | 1,950 | 26,950 | - |

---

## 6. Ordinances

### 6.1 Ordinance System Overview

Ordinances are colony-wide policies that cost credits but provide benefits.

**Deferred to Epic 11 Phase 2:** Full ordinance system with many options.
**MVP:** Implement framework with 2-3 sample ordinances.

### 6.2 Sample Ordinances

| Ordinance | Monthly Cost | Effect |
|-----------|--------------|--------|
| Enhanced Patrol | 1,000/phase | -10% disorder generation |
| Industrial Scrubbers | 2,000/phase | -15% industrial contamination |
| Free Transit | 5,000/phase | +10 transport accessibility bonus |

### 6.3 Ordinance Data Structure

```cpp
enum class OrdinanceType : uint8_t {
    EnhancedPatrol = 0,
    IndustrialScrubbers = 1,
    FreeTransit = 2,
    // ... more ordinances
};

struct OrdinanceComponent {
    OrdinanceType type;                   // 1 byte
    bool is_active;                       // 1 byte
    int32_t cost_per_phase;               // 4 bytes
    float effect_multiplier;              // 4 bytes
    uint8_t padding[6];                   // 6 bytes

    // Total: 16 bytes
};
```

### 6.4 Ordinance Framework Work Items

**Deferred to separate tickets after MVP:**
- ECO-030: Ordinance toggle UI integration
- ECO-031: Ordinance effect system hooks
- ECO-032: Full ordinance library (10+ ordinances)

---

## 7. Questions for Other Agents

### @systems-architect

1. **Tick Priority Confirmation:** EconomySystem at tick_priority 60 (after ServicesSystem 55, before DisorderSystem 70). This allows:
   - Services to calculate coverage first
   - Economy to process before disorder uses funding effectiveness
   - Does this align with your system ordering?

2. **IEconomyQueryable Ownership:** The interface is currently defined in interfaces.yaml stubs section. Should Epic 11 move it to a proper section, or keep it in stubs with "replaced by EconomySystem" note?

3. **Construction Cost Deduction:** When a player places a building:
   - Option A: Deduct cost immediately during BuildingSystem.tick()
   - Option B: Deduct via EconomySystem transaction queue
   Which pattern aligns better with our architecture?

### @game-designer

4. **Starting Treasury:** The stub returns 20,000 credits. Is this the canonical starting balance? Should it vary by difficulty or map size?

5. **Default Tribute Rates:** Stub returns 7% for all zones. Should each zone type have a different default?
   - Habitation: 7%?
   - Exchange: 8%?
   - Fabrication: 6%?

6. **Tribute Rate Impact on Demand:** How severely should high tribute reduce zone demand? Proposed formula:
   ```
   demand_modifier = 1.0 - (tribute_rate - 7) * 0.05
   // At 7%: 1.0x (neutral)
   // At 10%: 0.85x
   // At 15%: 0.6x
   // At 20%: 0.35x (severe penalty)
   ```
   Is this curve appropriate?

7. **Bankruptcy Consequences:** What happens when a player hits -50,000 credits?
   - Option A: Forced colony restart (tiles become ghost town)
   - Option B: Game Master takes over services (reduced quality)
   - Option C: Unable to build, forced austerity until balanced
   - Option D: Nothing (sandbox game, no fail state)

8. **Ordinance Priority:** Should ordinances be in Epic 11 MVP, or deferred to a polish pass? I've included framework only.

9. **Bond Limits:** Should there be a maximum number of concurrent bonds? Proposed: Max 5 bonds per player.

### @services-engineer

10. **Service Funding Integration:** Epic 9 mentions funding_level affecting service effectiveness. How should EconomySystem communicate funding to ServicesSystem?
    - Option A: Direct funding_level field in ServiceProviderComponent
    - Option B: Query IEconomyQueryable.get_department_funding(ServiceType)
    Which pattern did you implement?

11. **Service Maintenance Costs:** Should maintenance costs scale with funding level, or be fixed regardless of funding? (e.g., 50% funding = 50% cost, or full cost either way?)

### @population-engineer

12. **Population Effect on Tribute:** My formula uses building occupancy. Does PopulationSystem update BuildingOccupancyComponent.current_occupancy reliably each tick?

13. **Tribute Impact on Migration:** High tribute reduces attractiveness. Should I emit a TributeChangedEvent that MigrationFactorComponent listens to, or should PopulationSystem query tribute rates directly?

---

## 8. Testing Strategy

### 8.1 Unit Tests

#### Treasury Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_starting_balance | New player starts with 20,000 credits | P0 |
| test_income_calculation | Tribute calculated correctly from buildings | P0 |
| test_expense_calculation | Maintenance summed correctly | P0 |
| test_budget_cycle | Balance updates at phase transition | P0 |
| test_deficit_warning | Warning emitted at -5,000 | P0 |
| test_emergency_bond | Auto-loan at -10,000 | P0 |

#### Tribute Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_tribute_base_values | Each zone type has correct base | P0 |
| test_tribute_rate_application | Rate correctly applied | P0 |
| test_tribute_occupancy_factor | Empty buildings pay less | P0 |
| test_tribute_sector_value_factor | High value = more tribute | P0 |
| test_tribute_rate_change | Changing rate takes effect next phase | P0 |

#### Bond Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_bond_issuance | Principal added to treasury | P0 |
| test_bond_payment | Correct payment deducted each phase | P0 |
| test_bond_completion | Bond removed after term | P0 |
| test_bond_interest | Interest calculated correctly | P0 |
| test_max_bonds | Cannot exceed bond limit | P1 |

### 8.2 Integration Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_building_construction_cost | Cost deducted when building placed | P0 |
| test_demand_tribute_interaction | High tribute reduces demand | P0 |
| test_service_funding | Service effectiveness scales with funding | P1 |
| test_full_budget_cycle | Income - expenses = balance change | P0 |

### 8.3 Edge Cases

| Test | Description | Priority |
|------|-------------|----------|
| test_zero_population | No tribute with no buildings | P0 |
| test_all_buildings_empty | Zero occupancy = zero tribute | P0 |
| test_max_tribute_rate | 20% rate applied correctly | P0 |
| test_zero_tribute_rate | 0% rate = no tribute income | P0 |
| test_negative_balance | Treasury can go negative | P0 |
| test_very_large_city | 100,000+ beings, no overflow | P1 |

### 8.4 Performance Tests

| Test | Description | Target |
|------|-------------|--------|
| test_tribute_calculation_speed | Calculate tribute for 1000 buildings | <1ms |
| test_maintenance_calculation_speed | Calculate maintenance for 2000 entities | <1ms |
| test_budget_cycle_speed | Full phase processing | <5ms |

---

## Appendix A: IEconomyQueryable Interface (Replacing Stub)

```cpp
// Replaces Epic 10 stub with real implementation
class IEconomyQueryable {
public:
    virtual ~IEconomyQueryable() = default;

    // Tribute rate queries (used by DemandSystem)
    virtual int get_tribute_rate(ZoneBuildingType zone_type, PlayerID owner) const = 0;
    virtual int get_average_tribute_rate(PlayerID owner) const = 0;

    // Treasury queries
    virtual int64_t get_treasury_balance(PlayerID owner) const = 0;
    virtual int64_t get_last_income(PlayerID owner) const = 0;
    virtual int64_t get_last_expenses(PlayerID owner) const = 0;

    // Debt queries
    virtual int64_t get_total_debt(PlayerID owner) const = 0;
    virtual int get_active_bond_count(PlayerID owner) const = 0;

    // Department funding (for ServicesSystem)
    virtual float get_department_funding(ServiceType service_type, PlayerID owner) const = 0;
};
```

**Stub Behavior (Epic 10):**
- get_tribute_rate(): Returns 7
- get_average_tribute_rate(): Returns 7
- get_treasury_balance(): Returns 20000

**Real Implementation (Epic 11):**
- All methods return actual calculated values

---

## Appendix B: Canonical Terminology Reference

| Human Term | Canonical (Alien) Term |
|------------|------------------------|
| Money | Credits |
| Dollar | Credit |
| Budget | Colony treasury |
| Taxes | Tribute |
| Tax rate | Tribute rate |
| Income | Revenue |
| Expenses | Expenditure |
| Debt | Deficit |
| Loan | Credit advance |
| Bond | Treasury bond |

---

## Appendix C: Budget UI Data Structure

For the budget window display:

```cpp
struct BudgetWindowData {
    // Treasury summary
    int64_t current_balance;
    int64_t projected_balance;  // Balance after next phase

    // Income breakdown
    int64_t habitation_tribute;
    int64_t exchange_tribute;
    int64_t fabrication_tribute;
    int64_t other_income;
    int64_t total_income;

    // Expense breakdown
    int64_t infrastructure_maintenance;
    int64_t service_maintenance;
    int64_t energy_maintenance;
    int64_t bond_payments;
    int64_t ordinance_costs;
    int64_t total_expenses;

    // Net
    int64_t net_budget;  // total_income - total_expenses

    // Tribute rates (for sliders)
    uint8_t habitation_rate;
    uint8_t exchange_rate;
    uint8_t fabrication_rate;

    // Debt summary
    int64_t total_debt;
    int active_bonds;

    // Historical data (for graphs)
    std::array<int64_t, 12> income_history;   // Last 12 phases
    std::array<int64_t, 12> expense_history;  // Last 12 phases
};
```

---

## Appendix D: Tick Execution Order Context

```
Priority  System                 Notes
-------   ------                 -----
   10     EnergySystem           Power coverage
   20     FluidSystem            Water coverage
   30     ZoneSystem             Zone state
   40     BuildingSystem         Building state
   45     TransportSystem        Traffic
   47     RailSystem             Rail
   50     PopulationSystem       Demographics, employment
   52     DemandSystem           RCI demand (queries tribute rates)
   55     ServicesSystem         Service coverage (queries funding)
   60     EconomySystem          <-- EPIC 11 (treasury, tribute, expenses)
   70     DisorderSystem         Crime
   80     ContaminationSystem    Pollution
   85     LandValueSystem        Sector value
```

EconomySystem at 60 ensures:
- DemandSystem (52) can query tribute rates for demand calculation
- ServicesSystem (55) can query department funding for effectiveness
- Both run before EconomySystem processes the phase budget

---

**End of Economy Engineer Analysis: Epic 11 - Financial System**
