# Systems Architect Analysis: Epic 11 - Financial System

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.11.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 11 implements the **EconomySystem** - the financial backbone of Sims 3000. This epic replaces the `IEconomyQueryable` stub from Epic 10 with a real implementation, enabling dynamic tribute (tax) rates, treasury management, expenditure tracking, and budget allocation.

Key architectural characteristics:
1. **Stub replacement:** Epic 10's IEconomyQueryable stub (returning 7% tribute, 20000 credits) becomes a real system
2. **Per-player treasury:** Each overseer has their own colony_treasury - no shared funds
3. **Bidirectional integration:** Services (Epic 9) query funding levels; Economy queries service costs
4. **Tick priority 60:** Runs after ServicesSystem (55), before LandValueSystem (85)

The EconomySystem is primarily a **data aggregation system** - it collects income from taxable buildings (PopulationSystem), calculates expenditure from services (ServicesSystem), and maintains the treasury balance. The core simulation (disorder, contamination, demand) remains in Epic 10.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Flow Analysis](#2-data-flow-analysis)
3. [Key Work Items](#3-key-work-items)
4. [Questions for Other Agents](#4-questions-for-other-agents)
5. [Integration Points](#5-integration-points)
6. [Risks and Concerns](#6-risks-and-concerns)
7. [Multiplayer Considerations](#7-multiplayer-considerations)

---

## 1. System Boundaries

### 1.1 EconomySystem (tick_priority: 60)

**Owns:**
- Colony treasury (per-player credits balance)
- Tribute (tax) collection calculation
- Expense/expenditure calculation
- Budget allocation (funding levels per service type)
- Credit advances (treasury bonds) - borrowing mechanism
- Ordinances (optional policy toggles affecting economy)

**Does NOT Own:**
- Individual building costs (defined in BuildingComponent data)
- What generates income (PopulationSystem tracks taxable population; BuildingSystem tracks buildings)
- Service effectiveness calculation (ServicesSystem owns; Economy only provides funding levels)
- Demand effects from tribute rates (DemandSystem queries tribute rates and calculates effects)

**Provides (IEconomyQueryable - replacing stub):**
- `get_tribute_rate(zone_type, owner)` - Current tribute rate for zone type (0-20%)
- `get_average_tribute_rate(owner)` - Average tribute rate across zones
- `get_treasury_balance(owner)` - Current credits balance
- `get_income_breakdown(owner)` - Revenue by category
- `get_expense_breakdown(owner)` - Expenditure by category
- `get_funding_level(service_type, owner)` - Budget allocation for services (0-150%)
- `can_afford(amount, owner)` - Check if treasury has sufficient credits
- `deduct_credits(amount, owner)` - Deduct from treasury (for construction costs)

**Depends On:**
- BuildingSystem (taxable buildings, construction costs)
- PopulationSystem (population for tribute calculation)
- ServicesSystem (service costs, building maintenance)

### 1.2 Boundary Summary Table

| Concern | EconomySystem | PopulationSystem | ServicesSystem | DemandSystem |
|---------|---------------|------------------|----------------|--------------|
| Treasury balance | **Owns** | - | - | - |
| Tribute rates | **Owns** | - | - | Queries |
| Tribute collection | **Owns** | Provides taxable pop | - | - |
| Service funding | **Owns** | - | Queries | - |
| Service costs | Queries | - | **Owns** | - |
| Demand effects | - | - | - | **Owns** |
| Building maintenance | Queries | - | Provides | - |

---

## 2. Data Flow Analysis

### 2.1 Tick Order Context

```
Priority 50: PopulationSystem      -- population calculated, provides taxable beings
Priority 52: DemandSystem          -- queries tribute rates (uses previous tick's rates)
Priority 55: ServicesSystem        -- queries funding levels, provides service costs
Priority 60: EconomySystem         -- EPIC 11: calculates income/expenses, updates treasury
Priority 85: LandValueSystem       -- may query economy for future features
Priority 70: DisorderSystem        -- no direct economy dependency
Priority 80: ContaminationSystem   -- no direct economy dependency
```

### 2.2 Income Calculation Flow

```
                    REVENUE SOURCES
                    ===============

PopulationSystem (50)
    |
    | get_taxable_population(zone_type, owner)
    | get_building_count(zone_type, owner)
    v
+--------------------------------------------------+
|                 EconomySystem (60)               |
|--------------------------------------------------|
| For each zone type (habitation, exchange, fab):  |
|   base_income = population * value_per_being     |
|   tribute = base_income * tribute_rate           |
|   total_revenue += tribute                       |
|                                                   |
| Other income:                                     |
|   + Trade income (future: PortSystem)            |
|   + Credit advance (bond) proceeds               |
|   + One-time revenue (crystal clearing, etc.)    |
+--------------------------------------------------+
    |
    v
colony_treasury += total_revenue
```

### 2.3 Expense Calculation Flow

```
                    EXPENDITURE SOURCES
                    ====================

ServicesSystem (55)
    |
    | get_service_maintenance_cost(service_type, owner)
    | get_total_service_buildings(owner)
    v
+--------------------------------------------------+
|                 EconomySystem (60)               |
|--------------------------------------------------|
| Service expenditure:                              |
|   For each service type:                         |
|     base_cost = building_count * maintenance     |
|     funded_cost = base_cost * funding_level      |
|     service_expenses += funded_cost              |
|                                                   |
| Infrastructure maintenance:                       |
|   + Pathway maintenance (per tile of road)       |
|   + Energy conduit maintenance                   |
|   + Fluid conduit maintenance                    |
|                                                   |
| Credit advance interest:                          |
|   + bond_principal * interest_rate               |
|                                                   |
| Ordinance costs:                                  |
|   + Active ordinance upkeep                      |
+--------------------------------------------------+
    |
    v
colony_treasury -= total_expenditure
```

### 2.4 Budget Allocation Cycle

```
+------------------------+
|     UI Budget Panel    |
| (Set funding levels)   |
+------------------------+
           |
           | Player sets funding 0-150%
           v
+------------------------+
|    EconomySystem       |
| (Stores funding_level) |
+------------------------+
           |
           | get_funding_level(service_type)
           v
+------------------------+
|   ServicesSystem       |
| (Applies to coverage)  |
+------------------------+
           |
           | Coverage affects simulation
           v
+------------------------+
| Disorder, Population,  |
| Land Value systems     |
+------------------------+
```

### 2.5 Cross-System Query Pattern

```
+------------------+         +-------------------+         +--------------------+
| PopulationSystem |         | ServicesSystem    |         | BuildingSystem     |
|------------------|         |-------------------|         |--------------------|
| - Taxable pop    |         | - Service costs   |         | - Building count   |
| - By zone type   |         | - By service type |         | - Maintenance data |
+--------+---------+         +---------+---------+         +----------+---------+
         |                             |                              |
         |   IPopulationQueryable      |   IServiceCostProvider       |   IBuildingQueryable
         v                             v                              v
+--------+-----------------------------+------------------------------+---------+
|                                                                               |
|                        EconomySystem (Epic 11)                                |
|                                                                               |
|  +-----------------+  +------------------+  +-------------------+             |
|  | TributeCalc     |  | ExpenseCalc      |  | TreasuryManager   |             |
|  |-----------------|  |------------------|  |-------------------|             |
|  | - Per zone type |  | - Service costs  |  | - Balance track   |             |
|  | - Rate config   |  | - Infra maint    |  | - Credit advances |             |
|  +-----------------+  +------------------+  +-------------------+             |
|                                                                               |
+-------------------------------------------------------------------------------+
         |
         | IEconomyQueryable (replacing stub)
         v
+------------------+
| DemandSystem     |
| ServicesSystem   |
| UISystem (Ep 12) |
+------------------+
```

---

## 3. Key Work Items

### Phase 1: Core Infrastructure (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-001 | TaxableComponent definition | S | Component for buildings that generate tribute: zone_type, base_value, tribute_modifier |
| E11-002 | MaintenanceCostComponent definition | S | Component for buildings with upkeep: maintenance_cost, maintenance_interval |
| E11-003 | TreasuryState data structure | S | Per-player treasury: balance, income_history, expense_history |
| E11-004 | EconomySystem class skeleton | M | ISimulatable at tick_priority 60, owns TreasuryState per player |
| E11-005 | IEconomyQueryable real implementation | M | Replace Epic 10 stub with real methods |

### Phase 2: Tribute Collection (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-006 | Tribute rate configuration | S | Per-zone tribute rates (0-20%), default 7%, stored per player |
| E11-007 | Taxable population aggregation | M | Query PopulationSystem for taxable beings by zone type |
| E11-008 | Tribute calculation per zone | M | Calculate income: population * value * rate |
| E11-009 | Revenue breakdown tracking | M | Track income by category for UI display |

### Phase 3: Expenditure Calculation (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-010 | Service cost aggregation | M | Query ServicesSystem for total maintenance costs |
| E11-011 | Infrastructure maintenance calculation | M | Calculate pathway, conduit, pipe upkeep |
| E11-012 | Funding level application | M | Funding levels (0-150%) affect service costs |
| E11-013 | Expense breakdown tracking | M | Track expenses by category for UI display |

### Phase 4: Budget Allocation (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-014 | Funding level storage | S | Store funding_level per service type per player (0-150%) |
| E11-015 | Budget allocation commands | M | Commands to adjust funding levels |
| E11-016 | ServicesSystem funding integration | M | ServicesSystem queries get_funding_level() for effectiveness |
| E11-017 | Budget deficit handling | M | What happens when treasury goes negative (grace period, penalties) |

### Phase 5: Credit Advances / Treasury Bonds (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-018 | Credit advance data structure | S | Bond: principal, interest_rate, remaining_balance, term_ticks |
| E11-019 | Bond issuance and repayment | M | Take out bonds, automatic interest payments |
| E11-020 | Bond limit calculation | M | Max bonds based on city value/population |

### Phase 6: Integration (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-021 | DemandSystem tribute rate integration | M | Update Epic 10's DemandSystem to use real tribute rates |
| E11-022 | Construction cost deduction | M | BuildingSystem calls deduct_credits() on construction |
| E11-023 | IStatQueryable implementation | S | Economy stats for UI (balance, income, expenses, debt) |

### Phase 7: Testing (2 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E11-024 | Unit tests for tribute calculation | L | Test tribute rates, population aggregation, income calc |
| E11-025 | Integration test with Epic 9/10 | L | End-to-end: services funded, demand affected by rates |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 6 | E11-001, E11-002, E11-003, E11-006, E11-014, E11-018, E11-023 |
| M | 15 | E11-004, E11-005, E11-007, E11-008, E11-009, E11-010, E11-011, E11-012, E11-013, E11-015, E11-016, E11-017, E11-019, E11-020, E11-021, E11-022 |
| L | 2 | E11-024, E11-025 |
| **Total** | **23** | |

---

## 4. Questions for Other Agents

### @game-designer

1. **Tribute rate range and defaults:** Canon specifies 0-20% with 7% default. Is this per-zone or global? SimCity 2000 had separate R/C/I rates. Proposed: Per-zone rates (habitation, exchange, fabrication) each 0-20%, default 7%.

2. **Credit advance (bond) mechanics:** How many bonds can a player have? SimCity 2000 allowed up to 9 bonds at different rates. Proposed: Max bonds = population/10000, capped at 10.

3. **Treasury deficit behavior:** What happens when credits go negative?
   - (A) Immediate service shutdown
   - (B) Grace period (N cycles) before effects
   - (C) Forced bond issuance
   - Proposed: (B) 10-cycle grace period, then services at 0% funding.

4. **Ordinances scope:** Which ordinances should Epic 11 include? Or should ordinances be a separate epic? Classic examples:
   - Legalize gambling (+revenue, +disorder)
   - Free clinics (+medical coverage, +expense)
   - Recycling program (-contamination, +expense)

5. **Income per being value:** What's the base credit value per taxable being? This affects the entire economy scale. Proposed: 10 credits/being/cycle base, modified by zone type and sector value.

### @economy-engineer

6. **Tribute calculation frequency:** Should tribute be collected every tick or at phase (month) boundaries? Every tick is smoother but may be computationally heavier.

7. **Funding level granularity:** Should funding be 0-100% in 10% increments, or allow any value 0-150%? SimCity 2000 used 0-100% in 10% steps with overfunding possible.

8. **Infrastructure maintenance formula:** Should maintenance scale with:
   - (A) Total tiles of infrastructure
   - (B) Total buildings connected
   - (C) Flat per-entity cost
   - Proposed: (A) per-tile with decay multiplier for old infrastructure.

9. **Bond interest rates:** Should interest rates be:
   - (A) Fixed (e.g., 5% per cycle)
   - (B) Variable based on city rating
   - (C) Increasing with number of existing bonds
   - Proposed: (C) - first bond 4%, each additional +1% up to 10%.

### @services-engineer

10. **Funding level effect curve:** How does funding level affect service effectiveness?
    - 0% funding: Building inactive?
    - 50% funding: 50% effectiveness or less?
    - 100% funding: Full effectiveness
    - 150% funding: 110% effectiveness? 125%?

    Proposed curve for consistency with Epic 9:
    ```
    funding 0-49%:  effectiveness = funding * 0.8
    funding 50-100%: effectiveness = 0.4 + (funding-50) * 0.012
    funding 101-150%: effectiveness = 1.0 + (funding-100) * 0.005
    ```

---

## 5. Integration Points

### 5.1 Epic 10 Integration (CRITICAL - Stub Replacement)

Epic 10 created an **IEconomyQueryable stub** that returns default values:

```cpp
// Epic 10 stub behavior (to be replaced)
get_tribute_rate(zone_type, owner) -> 7      // 7% default
get_average_tribute_rate(owner) -> 7         // 7% default
get_treasury_balance(owner) -> 20000         // Starting credits
```

**Epic 11 must:**
1. Implement the REAL IEconomyQueryable with actual treasury tracking
2. Ensure DemandSystem uses real tribute rates for demand calculation
3. Provide funding levels to ServicesSystem
4. Track actual income/expense and update treasury balance

### 5.2 Epic 9 Integration (Bidirectional)

**EconomySystem -> ServicesSystem:**
- `get_funding_level(service_type, owner)` - ServicesSystem queries this to modify coverage effectiveness

**ServicesSystem -> EconomySystem:**
- `get_service_maintenance_cost(service_type, owner)` - Economy queries this for expense calculation

### 5.3 Epic 4/5/6/7 Integration (Maintenance Costs)

Infrastructure from earlier epics has maintenance costs:

| Epic | Infrastructure | Proposed Maintenance |
|------|----------------|---------------------|
| Epic 5 | Energy conduits | 0.1 credits/tile/cycle |
| Epic 6 | Fluid conduits | 0.15 credits/tile/cycle |
| Epic 7 | Pathways | 0.2 credits/tile/cycle |
| Epic 7 | Rail lines | 0.5 credits/tile/cycle |

### 5.4 Tick Order Verification

```
Priority 50: PopulationSystem      -- provides taxable population
Priority 52: DemandSystem          -- uses PREVIOUS tick's tribute rates (acceptable lag)
Priority 55: ServicesSystem        -- uses PREVIOUS tick's funding (acceptable lag)
Priority 60: EconomySystem         -- calculates income/expense, updates treasury
                                   -- DemandSystem will get new rates NEXT tick
                                   -- ServicesSystem will get new funding NEXT tick
```

The one-tick lag for tribute rates and funding levels is acceptable - these values change slowly (player adjustments) and the simulation remains stable.

---

## 6. Risks and Concerns

### 6.1 Architectural Risks

**RISK: Economy Scale Balance (HIGH)**
Getting the economy balance right is critical. If income is too high, treasury accumulates infinitely. If expenses are too high, players can never build.

**Mitigation:**
- Use SimCity 2000 ratios as baseline
- Make all values data-driven (configurable)
- Extensive playtesting needed
- Consider auto-balancing based on population tier

**RISK: Circular Dependency with Services (MEDIUM)**
ServicesSystem queries funding levels. EconomySystem queries service costs. Both run in same tick.

**Mitigation:**
- ServicesSystem (55) runs BEFORE EconomySystem (60)
- Services use PREVIOUS tick's funding levels
- Economy calculates costs based on current service building count
- No true circular dependency - just one-tick lag on funding changes

**RISK: Treasury Overflow/Underflow (LOW)**
Very large or very small treasury values could cause issues.

**Mitigation:**
- Use int64_t for Credits (already in canon)
- Clamp treasury at reasonable bounds (e.g., -1B to +1T)
- Log warnings at extreme values

### 6.2 Design Ambiguities

**AMBIGUITY: Tribute Collection Granularity**
Options for when tribute is collected:
- (A) Every tick (20x/second) - smoothest but expensive
- (B) Every phase (12x/cycle) - traditional
- (C) Every cycle (1x/year) - simplest

**Recommendation:** Option (B) - per-phase collection matches SimCity feel and is computationally reasonable.

**AMBIGUITY: Deficit Handling**
What happens when treasury goes negative? Options:
- (A) Nothing special - just negative balance
- (B) Forced service cuts
- (C) Automatic bond issuance
- (D) Game over

**Recommendation:** Combination - negative balance allowed but services auto-reduce to 50% funding, automatic bond offer, no game over.

### 6.3 Technical Debt

**DEBT: Hardcoded Economy Values**
Initial implementation will have hardcoded:
- Tribute calculation rates
- Maintenance costs
- Bond interest rates
- Funding effect curves

**Remediation:** Data-driven economy config file in Epic 16/17 polish phase.

---

## 7. Multiplayer Considerations

### 7.1 Per-Player Treasury

Each player has completely separate economy:

| Data | Scope | Notes |
|------|-------|-------|
| Treasury balance | **Per-player** | No shared funds |
| Tribute rates | **Per-player** | Each sets own rates |
| Funding levels | **Per-player** | Each funds own services |
| Credit advances | **Per-player** | Each has own bond debt |
| Income/Expense | **Per-player** | Calculated from own buildings |

### 7.2 No Inter-Player Economy (Current Scope)

Epic 11 does NOT include:
- Trading credits between players
- Shared infrastructure costs
- Economic treaties
- Resource trading

These could be future features but are out of scope for Epic 11.

### 7.3 Authority Model

| Action | Authority | Notes |
|--------|-----------|-------|
| Tribute rate changes | Server | Player requests, server validates |
| Funding level changes | Server | Player requests, server validates |
| Treasury balance | Server | Server-authoritative calculation |
| Bond issuance | Server | Server validates limits |
| Construction cost deduction | Server | Server processes after build validation |

### 7.4 Network Sync

| Data | Sync Method | Frequency |
|------|-------------|-----------|
| Treasury balance | Delta | Every tick (small data: int64) |
| Tribute rates | On-change | Only when player adjusts |
| Funding levels | On-change | Only when player adjusts |
| Active bonds | On-change | Only on bond events |
| Income/Expense breakdown | On-request | When budget window opens |

### 7.5 Late Join

When a new player joins:
- Start with 20000 credits (starting_credits constant)
- No existing bonds
- Default tribute rates (7% all zones)
- Default funding levels (100% all services)
- Must purchase tiles and build from scratch

### 7.6 Ghost Town Economy

When a player abandons (triggers ghost town):
- Treasury is lost (not transferred)
- Bonds are forgiven (no debt transfer)
- Buildings stop generating tribute (no owner)
- Maintenance stops (decay instead)

---

## Appendix A: Component Definitions

### TaxableComponent

```cpp
struct TaxableComponent {
    ZoneBuildingType zone_type;     // habitation, exchange, fabrication
    uint32_t base_value = 100;      // Base tribute value per being/unit
    float tribute_modifier = 1.0f;  // Modified by land value, services
};
// Size: 9 bytes (padded to 12)
```

### MaintenanceCostComponent

```cpp
struct MaintenanceCostComponent {
    uint32_t maintenance_cost = 0;      // Credits per maintenance interval
    uint16_t maintenance_interval = 1;  // Phases between payments (1 = every phase)
    uint16_t phases_since_payment = 0;  // Counter
};
// Size: 8 bytes
```

### TreasuryState (Per-Player)

```cpp
struct TreasuryState {
    int64_t balance = 20000;            // Current credits
    int64_t last_income = 0;            // Income from last cycle
    int64_t last_expense = 0;           // Expense from last cycle
    int64_t total_debt = 0;             // Sum of all bond principals

    // Per-zone tribute rates (0-20, representing 0-20%)
    uint8_t tribute_rate_habitation = 7;
    uint8_t tribute_rate_exchange = 7;
    uint8_t tribute_rate_fabrication = 7;

    // Per-service funding levels (0-150, representing 0-150%)
    uint8_t funding_enforcer = 100;
    uint8_t funding_hazard_response = 100;
    uint8_t funding_medical = 100;
    uint8_t funding_education = 100;

    // Bonds
    std::vector<CreditAdvance> active_bonds;
};
// Note: std::vector makes this non-trivially copyable - serialize carefully
```

### CreditAdvance (Bond)

```cpp
struct CreditAdvance {
    int64_t principal;              // Original amount borrowed
    int64_t remaining_balance;      // Amount still owed
    uint8_t interest_rate;          // Annual rate (e.g., 5 = 5%)
    uint32_t term_cycles;           // Total term in cycles
    uint32_t cycles_remaining;      // Cycles until maturity
    bool is_active = true;          // False when fully repaid
};
// Size: ~32 bytes
```

---

## Appendix B: Economy Formulas (Proposed)

### Tribute Calculation

```
// Per zone type, per phase
taxable_population = PopulationSystem.get_population(zone_type, owner)
base_value_per_being = zone_base_values[zone_type]  // e.g., habitation=8, exchange=12, fabrication=15
sector_value_modifier = avg_land_value / 128.0      // 0.0-2.0 range
tribute_rate = tribute_rates[zone_type] / 100.0     // 0.0-0.2

raw_income = taxable_population * base_value_per_being * sector_value_modifier
zone_income = raw_income * tribute_rate

total_income = sum(zone_income for each zone_type)
```

### Expense Calculation

```
// Service maintenance
service_expense = 0
for each service_type:
    building_count = ServicesSystem.get_building_count(service_type, owner)
    base_maintenance = service_maintenance_costs[service_type]
    funding_level = funding_levels[service_type] / 100.0
    service_expense += building_count * base_maintenance * funding_level

// Infrastructure maintenance
infra_expense = 0
infra_expense += TransportSystem.get_pathway_tile_count(owner) * PATHWAY_MAINTENANCE
infra_expense += EnergySystem.get_conduit_tile_count(owner) * CONDUIT_MAINTENANCE
infra_expense += FluidSystem.get_pipe_tile_count(owner) * PIPE_MAINTENANCE

// Bond interest
bond_interest = 0
for each bond in active_bonds:
    bond_interest += bond.remaining_balance * (bond.interest_rate / 100.0 / 12.0)  // Monthly

total_expense = service_expense + infra_expense + bond_interest
```

### Treasury Update

```
// Per phase
net_change = total_income - total_expense
treasury_balance += net_change

// Check deficit
if treasury_balance < 0:
    if deficit_grace_cycles <= 0:
        auto_reduce_funding_to_50()
        offer_emergency_bond()
    else:
        deficit_grace_cycles -= 1
```

### Demand Effect from Tribute

```
// In DemandSystem
tribute_rate = EconomySystem.get_tribute_rate(zone_type, owner)
baseline_rate = 7  // 7% is neutral

rate_deviation = tribute_rate - baseline_rate
// +1% rate above 7% = -10 demand points
// -1% rate below 7% = +5 demand points (smaller bonus for low taxes)

if rate_deviation > 0:
    demand_modifier = -10 * rate_deviation
else:
    demand_modifier = -5 * rate_deviation  // Negative * negative = positive

zone_demand += demand_modifier
```

---

## Appendix C: Interface Definition

### IEconomyQueryable (Full Interface)

```cpp
class IEconomyQueryable {
public:
    // Tribute rates (replacing stub)
    virtual int get_tribute_rate(ZoneBuildingType zone_type, PlayerID owner) const = 0;
    virtual int get_average_tribute_rate(PlayerID owner) const = 0;

    // Treasury
    virtual int64_t get_treasury_balance(PlayerID owner) const = 0;
    virtual bool can_afford(int64_t amount, PlayerID owner) const = 0;
    virtual bool deduct_credits(int64_t amount, PlayerID owner) = 0;
    virtual void add_credits(int64_t amount, PlayerID owner) = 0;

    // Budget allocation (new for Epic 11)
    virtual int get_funding_level(ServiceType service_type, PlayerID owner) const = 0;
    virtual void set_funding_level(ServiceType service_type, int level, PlayerID owner) = 0;

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
    virtual void issue_bond(int64_t amount, PlayerID owner) = 0;
};
```

---

**End of Systems Architect Analysis: Epic 11 - Financial System**
