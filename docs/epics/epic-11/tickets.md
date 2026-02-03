# Epic 11: Financial System - Tickets

**Epic:** 11 - Financial System
**Created:** 2026-01-29
**Canon Version:** 0.13.0
**Total Tickets:** 24 (target: under 30)

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.11.0 | Initial ticket creation |
| 2026-01-29 | canon-verification (v0.11.0 → v0.13.0) | No changes required |
| 2026-01-29 | canon-enforcement | Renamed TaxableComponent → TributableComponent per terminology.yaml |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. IEconomyQueryable interface (ticket E11-005) matches interfaces.yaml. IncomeBreakdown/ExpenseBreakdown/CreditAdvance data contracts align. Tick priority 60 confirmed in systems.yaml.

---

## Ticket Summary Table

| ID | Title | Size | Phase | Blocked By |
|----|-------|------|-------|------------|
| E11-001 | Core Components (TributableComponent, MaintenanceCostComponent) | S | 1 | - |
| E11-002 | TreasuryState Data Structure | S | 1 | - |
| E11-003 | CreditAdvance (Bond) Data Structure | S | 1 | - |
| E11-004 | EconomySystem Skeleton (ISimulatable, priority 60) | M | 1 | E11-001, E11-002 |
| E11-005 | IEconomyQueryable Real Implementation | M | 1 | E11-004 |
| E11-006 | Per-Zone Tribute Rate Configuration | S | 2 | E11-004 |
| E11-007 | Tribute Calculation Engine | L | 2 | E11-006 |
| E11-008 | Revenue Breakdown Tracking | M | 2 | E11-007 |
| E11-009 | Infrastructure Maintenance Calculation | M | 3 | E11-004 |
| E11-010 | Service Maintenance Calculation | M | 3 | E11-004 |
| E11-011 | Expense Breakdown Tracking | M | 3 | E11-009, E11-010 |
| E11-012 | Budget Cycle Processing (Per-Phase) | M | 3 | E11-007, E11-011 |
| E11-013 | Funding Level Storage and API | M | 4 | E11-005 |
| E11-014 | ServicesSystem Funding Integration | M | 4 | E11-013 |
| E11-015 | Deficit Warning and Handling | M | 4 | E11-012 |
| E11-016 | Bond Issuance System | M | 5 | E11-003, E11-015 |
| E11-017 | Bond Repayment Processing | M | 5 | E11-016 |
| E11-018 | Emergency Bond Auto-Issuance | S | 5 | E11-016, E11-015 |
| E11-019 | DemandSystem Tribute Rate Integration | M | 6 | E11-006 |
| E11-020 | Construction Cost Deduction | M | 6 | E11-005 |
| E11-021 | Ordinance Framework and Sample Ordinances | M | 6 | E11-012 |
| E11-022 | Network Sync for Treasury State | M | 7 | E11-012 |
| E11-023 | Unit Tests for Tribute and Maintenance | L | 7 | E11-007, E11-009 |
| E11-024 | Integration Tests with Epic 9/10 | L | 7 | E11-014, E11-019 |

---

## Size Distribution

| Size | Count | Tickets |
|------|-------|---------|
| S | 5 | E11-001, E11-002, E11-003, E11-006, E11-018 |
| M | 15 | E11-004, E11-005, E11-008, E11-009, E11-010, E11-011, E11-012, E11-013, E11-014, E11-015, E11-016, E11-017, E11-019, E11-020, E11-021, E11-022 |
| L | 2 | E11-007, E11-023, E11-024 |

**Total: 24 tickets**

---

## Critical Path

```
Phase 1 (Foundation):
  E11-001 -> E11-004 -> E11-005
             E11-002 ->|
             E11-003

Phase 2 (Tribute):
  E11-006 -> E11-007 -> E11-008

Phase 3 (Expenses + Cycle):
  E11-009 -> E11-011 -> E11-012
  E11-010 ->|

Phase 4 (Funding + Deficit):
  E11-013 -> E11-014
  E11-015

Phase 5 (Bonds):
  E11-016 -> E11-017
           -> E11-018

Phase 6 (Integration):
  E11-019, E11-020, E11-021

Phase 7 (Testing + Sync):
  E11-022, E11-023, E11-024
```

**Critical Path:** E11-001 -> E11-004 -> E11-005 -> E11-006 -> E11-007 -> E11-012 -> E11-015 -> E11-016 -> E11-024

---

## Phase 1: Core Infrastructure

### E11-001: Core Components (TributableComponent, MaintenanceCostComponent)

**Size:** S (Small)
**Blocked By:** None

**Description:**
Define the two core ECS components that tag entities for economy calculations.

**Deliverables:**
1. `TributableComponent` - Attached to zone buildings that generate tribute
2. `MaintenanceCostComponent` - Attached to entities with upkeep costs

**TributableComponent Definition:**
```cpp
struct TributableComponent {
    ZoneBuildingType zone_type;     // habitation, exchange, fabrication
    uint32_t base_value = 100;      // Base tribute value per phase
    uint8_t density_level = 0;      // 0=low, 1=high
    float tribute_modifier = 1.0f;  // Modified by sector value, services
};
// Size: ~12 bytes
```

**MaintenanceCostComponent Definition:**
```cpp
struct MaintenanceCostComponent {
    int32_t base_cost = 0;          // Base maintenance per phase
    float cost_multiplier = 1.0f;   // Modifier (age, damage)
};
// Size: 8 bytes
```

**Acceptance Criteria:**
- [ ] Components defined in appropriate header file
- [ ] Components are trivially copyable
- [ ] Components follow canon naming conventions
- [ ] ZoneBuildingType enum includes habitation, exchange, fabrication

---

### E11-002: TreasuryState Data Structure

**Size:** S (Small)
**Blocked By:** None

**Description:**
Define the per-player treasury state data structure that tracks all financial information.

**TreasuryState Definition:**
```cpp
struct TreasuryState {
    // Current state
    int64_t balance = 20000;            // Starting credits (canonical)

    // Last cycle values (for UI)
    int64_t last_income = 0;
    int64_t last_expense = 0;

    // Income breakdown
    int64_t habitation_tribute = 0;
    int64_t exchange_tribute = 0;
    int64_t fabrication_tribute = 0;
    int64_t other_income = 0;

    // Expense breakdown
    int64_t infrastructure_maintenance = 0;
    int64_t service_maintenance = 0;
    int64_t energy_maintenance = 0;
    int64_t bond_payments = 0;
    int64_t ordinance_costs = 0;

    // Per-zone tribute rates (0-20%)
    uint8_t tribute_rate_habitation = 7;
    uint8_t tribute_rate_exchange = 7;
    uint8_t tribute_rate_fabrication = 7;

    // Per-service funding levels (0-150%)
    uint8_t funding_enforcer = 100;
    uint8_t funding_hazard_response = 100;
    uint8_t funding_medical = 100;
    uint8_t funding_education = 100;

    // State tracking
    uint8_t last_processed_phase = 0;
    bool deficit_warning_sent = false;
    bool emergency_bond_active = false;

    // Bonds (see E11-003)
    std::vector<CreditAdvance> active_bonds;
};
```

**Acceptance Criteria:**
- [ ] TreasuryState struct defined
- [ ] Balance starts at 20000 credits (canonical)
- [ ] All tribute rates default to 7
- [ ] All funding levels default to 100
- [ ] Uses canonical terminology (tribute, not tax)

---

### E11-003: CreditAdvance (Bond) Data Structure

**Size:** S (Small)
**Blocked By:** None

**Description:**
Define the credit advance (bond) data structure for tracking debt.

**CreditAdvance Definition:**
```cpp
struct CreditAdvance {
    int64_t principal;              // Original amount borrowed
    int64_t remaining_principal;    // Amount still owed
    uint16_t interest_rate_basis_points; // e.g., 750 = 7.5%
    uint16_t term_phases;           // Total phases for repayment
    uint16_t phases_remaining;      // Phases until maturity
    bool is_emergency = false;      // True if forced emergency bond
};
// Size: ~24 bytes
```

**Bond Types (Constants):**
```cpp
// Bond configurations
constexpr BondConfig BOND_SMALL     = {5000,   500,  12, false}; // 5K at 5%
constexpr BondConfig BOND_STANDARD  = {25000,  750,  24, false}; // 25K at 7.5%
constexpr BondConfig BOND_LARGE     = {100000, 1000, 48, false}; // 100K at 10%
constexpr BondConfig BOND_EMERGENCY = {25000,  1500, 12, true};  // 25K at 15%

constexpr int MAX_BONDS_PER_PLAYER = 5;
```

**Acceptance Criteria:**
- [ ] CreditAdvance struct defined
- [ ] BondConfig type for bond templates
- [ ] 4 bond types defined with canonical values
- [ ] MAX_BONDS_PER_PLAYER = 5

---

### E11-004: EconomySystem Skeleton (ISimulatable, priority 60)

**Size:** M (Medium)
**Blocked By:** E11-001, E11-002

**Description:**
Create the EconomySystem class skeleton implementing ISimulatable at tick_priority 60.

**Class Structure:**
```cpp
class EconomySystem : public ISimulatable {
public:
    EconomySystem(Registry& registry);

    // ISimulatable
    void tick(float delta_time) override;
    int get_priority() const override { return 60; }

    // Treasury access (per player)
    TreasuryState& get_treasury(PlayerID player);
    const TreasuryState& get_treasury(PlayerID player) const;

private:
    Registry& registry_;
    std::array<TreasuryState, MAX_PLAYERS> treasuries_;

    void process_budget_cycle(PlayerID player);
    void process_pending_transactions();
};
```

**Acceptance Criteria:**
- [ ] EconomySystem implements ISimulatable
- [ ] tick_priority returns 60
- [ ] Per-player treasury storage (array indexed by PlayerID)
- [ ] Basic tick() structure with phase transition detection
- [ ] Constructor initializes all treasuries to starting state

---

### E11-005: IEconomyQueryable Real Implementation

**Size:** M (Medium)
**Blocked By:** E11-004

**Description:**
Implement the full IEconomyQueryable interface, replacing the Epic 10 stub.

**Interface Methods:**
```cpp
class IEconomyQueryable {
public:
    virtual ~IEconomyQueryable() = default;

    // Tribute rates (replaces stub returning 7)
    virtual int get_tribute_rate(ZoneBuildingType zone_type, PlayerID owner) const = 0;
    virtual int get_average_tribute_rate(PlayerID owner) const = 0;

    // Treasury (replaces stub returning 20000)
    virtual int64_t get_treasury_balance(PlayerID owner) const = 0;
    virtual bool can_afford(int64_t amount, PlayerID owner) const = 0;
    virtual bool deduct_credits(int64_t amount, PlayerID owner) = 0;
    virtual void add_credits(int64_t amount, PlayerID owner) = 0;

    // Budget allocation (new for Epic 11)
    virtual int get_funding_level(ServiceType service_type, PlayerID owner) const = 0;
    virtual void set_funding_level(ServiceType type, int level, PlayerID owner) = 0;

    // Statistics
    virtual int64_t get_last_income(PlayerID owner) const = 0;
    virtual int64_t get_last_expense(PlayerID owner) const = 0;
    virtual IncomeBreakdown get_income_breakdown(PlayerID owner) const = 0;
    virtual ExpenseBreakdown get_expense_breakdown(PlayerID owner) const = 0;

    // Credit advances
    virtual int64_t get_total_debt(PlayerID owner) const = 0;
    virtual int get_bond_count(PlayerID owner) const = 0;
    virtual int get_max_bonds(PlayerID owner) const = 0;
    virtual bool can_issue_bond(PlayerID owner) const = 0;
};
```

**Acceptance Criteria:**
- [ ] EconomySystem implements IEconomyQueryable
- [ ] All stub methods now return real values
- [ ] Funding level methods implemented
- [ ] Statistics methods implemented
- [ ] Bond query methods implemented
- [ ] Epic 10 StubEconomyProvider can be removed

---

## Phase 2: Tribute Collection

### E11-006: Per-Zone Tribute Rate Configuration

**Size:** S (Small)
**Blocked By:** E11-004

**Description:**
Implement tribute rate storage and modification for each zone type.

**API:**
```cpp
// In EconomySystem
void set_tribute_rate(ZoneBuildingType zone_type, uint8_t rate, PlayerID owner);
uint8_t get_tribute_rate(ZoneBuildingType zone_type, PlayerID owner) const;
uint8_t get_average_tribute_rate(PlayerID owner) const;
```

**Constraints:**
- Rate range: 0-20%
- Default: 7% for all zones
- Changes take effect next phase

**Acceptance Criteria:**
- [ ] Tribute rates stored per zone type per player
- [ ] Rate clamped to 0-20 range
- [ ] get_average_tribute_rate calculates weighted average
- [ ] Emits TributeRateChangedEvent on modification

---

### E11-007: Tribute Calculation Engine

**Size:** L (Large)
**Blocked By:** E11-006

**Description:**
Implement the complete tribute calculation system that computes income from taxable buildings.

**Formula:**
```cpp
int64_t calculate_building_tribute(
    EntityID building,
    const TributableComponent& taxable,
    const BuildingOccupancyComponent& occupancy,
    uint8_t sector_value,
    float tribute_rate
) {
    // Base tribute (from TributableComponent)
    int64_t base = taxable.base_value;

    // Occupancy factor: 0.0-1.0
    float occupancy_factor = (float)occupancy.current / (float)occupancy.capacity;

    // Sector value factor: 0.5x at 0, 1.0x at 128, 2.0x at 255
    float value_factor = 0.5f + (sector_value / 255.0f) * 1.5f;

    // Tribute rate: 0.0-0.2
    float rate_factor = tribute_rate / 100.0f;

    return (int64_t)(base * occupancy_factor * value_factor * rate_factor);
}
```

**Base Tribute Values:**
| Zone Type | Low Density | High Density |
|-----------|-------------|--------------|
| Habitation | 50 | 200 |
| Exchange | 100 | 400 |
| Fabrication | 75 | 300 |

**Acceptance Criteria:**
- [ ] Iterates all entities with TributableComponent
- [ ] Queries BuildingOccupancyComponent for occupancy
- [ ] Queries LandValueSystem for sector value
- [ ] Applies tribute rate from TreasuryState
- [ ] Sums to total income per zone type
- [ ] Performance: <1ms for 1000 buildings

---

### E11-008: Revenue Breakdown Tracking

**Size:** M (Medium)
**Blocked By:** E11-007

**Description:**
Track income by category for UI display and historical analysis.

**IncomeBreakdown Struct:**
```cpp
struct IncomeBreakdown {
    int64_t habitation_tribute;
    int64_t exchange_tribute;
    int64_t fabrication_tribute;
    int64_t other_income;
    int64_t total;
};
```

**History Tracking:**
```cpp
std::array<int64_t, 12> income_history;  // Last 12 phases
```

**Acceptance Criteria:**
- [ ] IncomeBreakdown returned by get_income_breakdown()
- [ ] Last 12 phases of income history stored
- [ ] TreasuryState updated after each budget cycle
- [ ] Total income calculated correctly

---

## Phase 3: Expenditure Calculation

### E11-009: Infrastructure Maintenance Calculation

**Size:** M (Medium)
**Blocked By:** E11-004

**Description:**
Calculate maintenance costs for pathways, conduits, and pipes.

**Maintenance Rates (per tile per phase):**
| Infrastructure Type | Cost |
|--------------------|------|
| Pathway | 5 credits |
| Energy Conduit | 2 credits |
| Fluid Conduit | 3 credits |
| Rail Track | 8 credits |

**Implementation:**
```cpp
int64_t calculate_infrastructure_maintenance(PlayerID owner) {
    int64_t total = 0;

    // Query all entities with MaintenanceCostComponent owned by player
    auto view = registry_.view<MaintenanceCostComponent, OwnershipComponent>();
    for (auto entity : view) {
        auto& maint = view.get<MaintenanceCostComponent>(entity);
        auto& ownership = view.get<OwnershipComponent>(entity);
        if (ownership.owner == owner) {
            total += maint.base_cost * maint.cost_multiplier;
        }
    }
    return total;
}
```

**Acceptance Criteria:**
- [ ] MaintenanceCostComponent added to infrastructure entities
- [ ] Query all owned entities with maintenance
- [ ] Sum costs correctly
- [ ] Performance: <1ms for 2000 entities

---

### E11-010: Service Maintenance Calculation

**Size:** M (Medium)
**Blocked By:** E11-004

**Description:**
Calculate maintenance costs for service buildings, scaled by funding level.

**Service Building Costs (per phase at 100% funding):**
| Service Type | Cost |
|--------------|------|
| Enforcer Post | 100 credits |
| Hazard Response Post | 120 credits |
| Medical Nexus | 300 credits |
| Learning Center | 200 credits |

**Funding Scaling:**
```cpp
actual_cost = base_cost * (funding_level / 100.0f)
// 50% funding = 50% cost
// 125% funding = 125% cost
```

**Acceptance Criteria:**
- [ ] Query ServicesSystem for service building costs
- [ ] Apply funding level to scale costs
- [ ] Sum costs by service type
- [ ] Track expenses per category

---

### E11-011: Expense Breakdown Tracking

**Size:** M (Medium)
**Blocked By:** E11-009, E11-010

**Description:**
Track expenses by category for UI display and historical analysis.

**ExpenseBreakdown Struct:**
```cpp
struct ExpenseBreakdown {
    int64_t infrastructure_maintenance;
    int64_t service_maintenance;
    int64_t energy_maintenance;
    int64_t bond_payments;
    int64_t ordinance_costs;
    int64_t total;
};
```

**History Tracking:**
```cpp
std::array<int64_t, 12> expense_history;  // Last 12 phases
```

**Acceptance Criteria:**
- [ ] ExpenseBreakdown returned by get_expense_breakdown()
- [ ] Last 12 phases of expense history stored
- [ ] Categories match TreasuryState fields

---

### E11-012: Budget Cycle Processing (Per-Phase)

**Size:** M (Medium)
**Blocked By:** E11-007, E11-011

**Description:**
Implement the main budget cycle that runs at each phase transition.

**Process Flow:**
```cpp
void EconomySystem::process_budget_cycle(PlayerID player) {
    auto& treasury = get_treasury(player);

    // 1. Calculate income
    int64_t income = calculate_total_income(player);

    // 2. Calculate expenses
    int64_t expenses = calculate_total_expenses(player);

    // 3. Process bond payments
    int64_t bond_payments = process_bond_payments(player);
    expenses += bond_payments;

    // 4. Update treasury
    treasury.balance += income - expenses;
    treasury.last_income = income;
    treasury.last_expense = expenses;

    // 5. Check deficit
    if (treasury.balance < 0) {
        handle_deficit(player);
    }

    // 6. Emit event
    emit(BudgetCycleCompletedEvent{player, income, expenses, treasury.balance});
}
```

**Phase Detection:**
```cpp
void EconomySystem::tick(float delta_time) {
    uint8_t current_phase = simulation_time_->get_current_phase();

    for (PlayerID p = 1; p <= num_players_; ++p) {
        auto& treasury = get_treasury(p);
        if (current_phase != treasury.last_processed_phase) {
            process_budget_cycle(p);
            treasury.last_processed_phase = current_phase;
        }
    }
}
```

**Acceptance Criteria:**
- [ ] Budget cycle runs at phase transitions (12x per cycle)
- [ ] Income and expenses calculated correctly
- [ ] Treasury balance updated
- [ ] History updated
- [ ] BudgetCycleCompletedEvent emitted
- [ ] Deficit handling triggered when balance < 0

---

## Phase 4: Budget Allocation

### E11-013: Funding Level Storage and API

**Size:** M (Medium)
**Blocked By:** E11-005

**Description:**
Implement funding level storage and modification API.

**API:**
```cpp
// In EconomySystem / IEconomyQueryable
int get_funding_level(ServiceType service_type, PlayerID owner) const;
void set_funding_level(ServiceType service_type, int level, PlayerID owner);
```

**Constraints:**
- Range: 0-150%
- Default: 100%
- Stored as uint8_t in TreasuryState

**Funding Effect Curve:**
```cpp
float calculate_effectiveness(int funding_level) {
    if (funding_level <= 0) return 0.0f;
    if (funding_level <= 25) return 0.40f * (funding_level / 25.0f);
    if (funding_level <= 50) return 0.40f + 0.25f * ((funding_level - 25) / 25.0f);
    if (funding_level <= 75) return 0.65f + 0.20f * ((funding_level - 50) / 25.0f);
    if (funding_level <= 100) return 0.85f + 0.15f * ((funding_level - 75) / 25.0f);
    // Diminishing returns above 100%
    return 1.0f + 0.10f * std::min((funding_level - 100) / 50.0f, 1.0f);
}
```

**Acceptance Criteria:**
- [ ] Funding levels stored per service type per player
- [ ] Level clamped to 0-150 range
- [ ] calculate_effectiveness function implemented
- [ ] Emits FundingLevelChangedEvent on modification

---

### E11-014: ServicesSystem Funding Integration

**Size:** M (Medium)
**Blocked By:** E11-013

**Description:**
Integrate funding levels with ServicesSystem so service effectiveness scales with funding.

**Integration Point:**
```cpp
// In ServicesSystem
float ServicesSystem::get_effectiveness(ServiceType type, PlayerID owner) const {
    int funding = economy_->get_funding_level(type, owner);
    float base_effectiveness = calculate_base_effectiveness(type, owner);
    float funding_factor = economy_->calculate_effectiveness_factor(funding);
    return base_effectiveness * funding_factor;
}
```

**Acceptance Criteria:**
- [ ] ServicesSystem queries IEconomyQueryable for funding
- [ ] Service effectiveness scales with funding
- [ ] 0% funding = services inactive
- [ ] 100% funding = full effectiveness
- [ ] 150% funding = ~110% effectiveness

---

### E11-015: Deficit Warning and Handling

**Size:** M (Medium)
**Blocked By:** E11-012

**Description:**
Implement deficit detection, warnings, and handling.

**Thresholds:**
| Balance | Action |
|---------|--------|
| < -5,000 | Emit DeficitWarningEvent |
| < -10,000 | Offer emergency bond (auto-issue if enabled) |

**Implementation:**
```cpp
void EconomySystem::handle_deficit(PlayerID player) {
    auto& treasury = get_treasury(player);

    if (treasury.balance < -5000 && !treasury.deficit_warning_sent) {
        emit(DeficitWarningEvent{player, treasury.balance});
        treasury.deficit_warning_sent = true;
    }

    if (treasury.balance < -10000 && !treasury.emergency_bond_active) {
        emit(EmergencyBondOfferEvent{player, BOND_EMERGENCY.principal});
        // Auto-issue if player has enabled auto-bonds
    }
}
```

**Acceptance Criteria:**
- [ ] Warning at -5,000 credits
- [ ] Emergency bond offered at -10,000
- [ ] Warning only sent once per deficit period
- [ ] Reset warning flag when balance > 0
- [ ] Events emitted for UI notification

---

## Phase 5: Credit Advances

### E11-016: Bond Issuance System

**Size:** M (Medium)
**Blocked By:** E11-003, E11-015

**Description:**
Implement the system for issuing credit advances (bonds).

**API:**
```cpp
bool issue_bond(PlayerID player, BondType type);
bool can_issue_bond(PlayerID player) const;
int get_bond_count(PlayerID player) const;
int get_max_bonds(PlayerID player) const { return MAX_BONDS_PER_PLAYER; }
```

**Validation:**
- Player must have fewer than MAX_BONDS_PER_PLAYER (5)
- Large bond requires population > 5000
- Cannot issue bond if already at max debt

**Implementation:**
```cpp
bool EconomySystem::issue_bond(PlayerID player, BondType type) {
    if (!can_issue_bond(player)) return false;

    auto& treasury = get_treasury(player);
    const BondConfig& config = get_bond_config(type);

    CreditAdvance bond;
    bond.principal = config.principal;
    bond.remaining_principal = config.principal;
    bond.interest_rate_basis_points = config.interest_rate;
    bond.term_phases = config.term_phases;
    bond.phases_remaining = config.term_phases;
    bond.is_emergency = config.is_emergency;

    treasury.active_bonds.push_back(bond);
    treasury.balance += config.principal;

    emit(BondIssuedEvent{player, config.principal, config.interest_rate});
    return true;
}
```

**Acceptance Criteria:**
- [ ] 4 bond types available (Small, Standard, Large, Emergency)
- [ ] Max 5 bonds per player
- [ ] Principal added to treasury on issuance
- [ ] BondIssuedEvent emitted
- [ ] Validation prevents invalid issuance

---

### E11-017: Bond Repayment Processing

**Size:** M (Medium)
**Blocked By:** E11-016

**Description:**
Implement automatic bond payment processing each phase.

**Payment Calculation:**
```cpp
int64_t calculate_bond_payment(const CreditAdvance& bond) {
    // Principal payment: equal installments
    int64_t principal_payment = bond.principal / bond.term_phases;

    // Interest payment: on remaining principal
    int64_t interest_payment = (bond.remaining_principal *
        bond.interest_rate_basis_points) / (10000 * 12);  // Per-phase

    return principal_payment + interest_payment;
}
```

**Processing:**
```cpp
int64_t process_bond_payments(PlayerID player) {
    auto& treasury = get_treasury(player);
    int64_t total_payment = 0;

    for (auto& bond : treasury.active_bonds) {
        int64_t payment = calculate_bond_payment(bond);
        bond.remaining_principal -= (bond.principal / bond.term_phases);
        bond.phases_remaining--;
        total_payment += payment;
    }

    // Remove matured bonds
    treasury.active_bonds.erase(
        std::remove_if(treasury.active_bonds.begin(), treasury.active_bonds.end(),
            [](const CreditAdvance& b) { return b.phases_remaining == 0; }),
        treasury.active_bonds.end()
    );

    return total_payment;
}
```

**Acceptance Criteria:**
- [ ] Bond payments processed each phase
- [ ] Principal and interest calculated correctly
- [ ] Remaining principal decreases
- [ ] Matured bonds removed automatically
- [ ] BondPaidOffEvent emitted when bond completes

---

### E11-018: Emergency Bond Auto-Issuance

**Size:** S (Small)
**Blocked By:** E11-016, E11-015

**Description:**
Implement automatic emergency bond issuance when deficit threshold is reached.

**Behavior:**
- Triggered when balance < -10,000
- Automatically issues emergency bond (25K at 15%)
- Only if player has not disabled auto-bonds
- Marked as is_emergency = true

**Acceptance Criteria:**
- [ ] Emergency bond auto-issued at -10,000
- [ ] Player setting to disable auto-bonds
- [ ] Emergency bonds marked with is_emergency flag
- [ ] EmergencyBondIssuedEvent emitted

---

## Phase 6: Integration

### E11-019: DemandSystem Tribute Rate Integration

**Size:** M (Medium)
**Blocked By:** E11-006

**Description:**
Update DemandSystem to use real tribute rates instead of stub values.

**Demand Modifier Calculation:**
```cpp
int calculate_tribute_demand_modifier(int tribute_rate) {
    // Tiered system per Game Designer
    if (tribute_rate <= 3) return 15;       // +15% demand
    if (tribute_rate <= 7) return 0;        // Neutral
    if (tribute_rate <= 12) return -((tribute_rate - 7) * 4);  // -4% to -20%
    if (tribute_rate <= 16) return -20 - ((tribute_rate - 12) * 5);  // -25% to -40%
    return -40 - ((tribute_rate - 16) * 5); // -45% to -60%
}
```

**Integration:**
```cpp
// In DemandSystem
int64_t DemandSystem::calculate_zone_demand(ZoneBuildingType zone, PlayerID owner) {
    int tribute_rate = economy_->get_tribute_rate(zone, owner);
    int tribute_modifier = calculate_tribute_demand_modifier(tribute_rate);

    // Apply modifier to base demand calculation
    return base_demand * (100 + tribute_modifier) / 100;
}
```

**Acceptance Criteria:**
- [ ] DemandSystem queries IEconomyQueryable instead of stub
- [ ] Tribute rates affect demand per tiered formula
- [ ] High tribute reduces demand
- [ ] Low tribute (0-3%) boosts demand

---

### E11-020: Construction Cost Deduction

**Size:** M (Medium)
**Blocked By:** E11-005

**Description:**
Integrate construction cost deduction with BuildingSystem.

**Integration Point:**
```cpp
// In BuildingSystem - during construction validation
bool BuildingSystem::can_construct(EntityID building, PlayerID owner) {
    int64_t cost = get_construction_cost(building);

    if (!economy_->can_afford(cost, owner)) {
        return false;  // Insufficient funds
    }

    return true;
}

void BuildingSystem::construct(EntityID building, PlayerID owner) {
    int64_t cost = get_construction_cost(building);
    economy_->deduct_credits(cost, owner);

    // Proceed with construction...
}
```

**Acceptance Criteria:**
- [ ] BuildingSystem queries can_afford before construction
- [ ] Credits deducted via deduct_credits on construction start
- [ ] InsufficientFundsEvent emitted if cannot afford
- [ ] Works for zone buildings, infrastructure, service buildings

---

### E11-021: Ordinance Framework and Sample Ordinances

**Size:** M (Medium)
**Blocked By:** E11-012

**Description:**
Implement ordinance framework with 2-3 sample ordinances.

**OrdinanceComponent:**
```cpp
enum class OrdinanceType : uint8_t {
    EnhancedPatrol = 0,
    IndustrialScrubbers = 1,
    FreeTransit = 2
};

struct Ordinance {
    OrdinanceType type;
    bool is_active = false;
    int32_t cost_per_phase;
    float effect_multiplier;
};
```

**Sample Ordinances:**
| Ordinance | Cost/Phase | Effect |
|-----------|------------|--------|
| Enhanced Patrol | 1,000 | -10% disorder generation |
| Industrial Scrubbers | 2,000 | -15% industrial contamination |
| Free Transit | 5,000 | +10 transport accessibility |

**API:**
```cpp
void enable_ordinance(OrdinanceType type, PlayerID owner);
void disable_ordinance(OrdinanceType type, PlayerID owner);
bool is_ordinance_active(OrdinanceType type, PlayerID owner) const;
int64_t get_ordinance_costs(PlayerID owner) const;
```

**Acceptance Criteria:**
- [ ] OrdinanceType enum with 3 sample ordinances
- [ ] Enable/disable API
- [ ] Costs deducted each phase
- [ ] Effects applied (hooks for DisorderSystem, ContaminationSystem, TransportSystem)
- [ ] OrdinanceChangedEvent emitted

---

## Phase 7: Testing and Sync

### E11-022: Network Sync for Treasury State

**Size:** M (Medium)
**Blocked By:** E11-012

**Description:**
Implement network synchronization for treasury state.

**Sync Strategy:**
| Data | Sync Method | Frequency |
|------|-------------|-----------|
| balance | Delta | Every phase |
| tribute_rates | On-change | When player adjusts |
| funding_levels | On-change | When player adjusts |
| active_bonds | On-change | On bond events |
| breakdowns | On-request | When budget window opens |

**Message Types:**
- TreasuryUpdateMessage (balance, last_income, last_expense)
- TributeRateChangeMessage (zone, rate)
- FundingLevelChangeMessage (service, level)
- BondStateMessage (bond list)

**Acceptance Criteria:**
- [ ] Treasury balance synced each phase
- [ ] Rate/funding changes synced on modification
- [ ] Bond state synced on events
- [ ] Breakdowns provided on UI request
- [ ] Server-authoritative for all financial data

---

### E11-023: Unit Tests for Tribute and Maintenance

**Size:** L (Large)
**Blocked By:** E11-007, E11-009

**Description:**
Comprehensive unit tests for tribute calculation and maintenance costs.

**Test Categories:**

**Treasury Tests:**
- test_starting_balance - New player starts with 20,000
- test_income_calculation - Tribute summed correctly
- test_expense_calculation - Maintenance summed correctly
- test_budget_cycle - Balance updates at phase transition

**Tribute Tests:**
- test_tribute_base_values - Each zone type has correct base
- test_tribute_rate_application - Rate applied correctly
- test_tribute_occupancy_factor - Empty buildings pay less
- test_tribute_sector_value_factor - High value = more tribute
- test_tribute_rate_change - New rate takes effect next phase

**Maintenance Tests:**
- test_infrastructure_maintenance - Per-tile costs correct
- test_service_maintenance_scaling - Funding scales cost
- test_energy_maintenance - Power plant upkeep

**Bond Tests:**
- test_bond_issuance - Principal added to treasury
- test_bond_payment - Correct payment deducted
- test_bond_completion - Bond removed after term
- test_bond_interest - Interest calculated correctly
- test_max_bonds - Cannot exceed limit

**Edge Cases:**
- test_zero_population - No tribute with no buildings
- test_all_buildings_empty - Zero occupancy = zero tribute
- test_max_tribute_rate - 20% rate works correctly
- test_zero_tribute_rate - 0% rate = no tribute
- test_negative_balance - Treasury can go negative

**Acceptance Criteria:**
- [ ] All test categories implemented
- [ ] Tests cover happy path and edge cases
- [ ] Performance test: <1ms for 1000 buildings
- [ ] All tests pass

---

### E11-024: Integration Tests with Epic 9/10

**Size:** L (Large)
**Blocked By:** E11-014, E11-019

**Description:**
End-to-end integration tests with ServicesSystem and DemandSystem.

**Integration Test Scenarios:**

**Services Integration:**
- test_service_funding_effectiveness - Funding affects service coverage
- test_service_cost_scaling - Higher funding = higher cost
- test_funding_default - New game has 100% funding

**Demand Integration:**
- test_demand_tribute_interaction - High tribute reduces demand
- test_demand_low_tribute_bonus - 0-3% tribute boosts demand
- test_demand_neutral_at_7_percent - 7% is neutral

**Full Budget Cycle:**
- test_full_cycle_surplus - Income > expense = surplus
- test_full_cycle_deficit - Income < expense = deficit
- test_deficit_warning_triggers - Warning at -5K
- test_emergency_bond_triggers - Bond offered at -10K

**Construction Integration:**
- test_construction_deducts_cost - Building costs deducted
- test_cannot_afford_blocks_construction - No build without funds

**Acceptance Criteria:**
- [ ] All integration scenarios tested
- [ ] Tests run against real Epic 9/10 systems
- [ ] Budget cycle produces expected results
- [ ] All tests pass

---

## Dependency Graph

```
E11-001 ─────┐
E11-002 ─────┼──> E11-004 ──> E11-005 ──> E11-013 ──> E11-014
E11-003 ─────┘         │           │
                       │           └──> E11-020
                       │
                       └──> E11-006 ──> E11-007 ──> E11-008
                              │              │
                              │              └──> E11-012 ──> E11-015 ──> E11-016 ──> E11-017
                              │                        │            │           │
                              │                        │            └──> E11-018
                              │                        │
                              │                        └──> E11-021
                              │
                              └──> E11-019

E11-009 ──> E11-011 ──> E11-012
E11-010 ──┘

E11-012 ──> E11-022
E11-007 ──> E11-023
E11-009 ──┘
E11-014 ──> E11-024
E11-019 ──┘
```

---

## Notes

1. **Canon Compliance:** All terminology uses alien terms (tribute, credits, etc.) per terminology.yaml
2. **Stub Replacement:** E11-005 replaces Epic 10's IEconomyQueryable stub
3. **Tick Priority:** EconomySystem at 60 (after ServicesSystem 55, before DisorderSystem 70)
4. **Ordinances:** Framework only in Epic 11 MVP; full library deferred to polish phase
5. **Sandbox Safety:** No bankruptcy or game over - deficit is manageable state
