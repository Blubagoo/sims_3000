# Epic 11: Financial System - Canon Verification

**Epic:** 11 - Financial System
**Created:** 2026-01-29
**Canon Version:** 0.12.0 (updated)
**Verification Status:** PASS

---

## Summary

| Check | Status | Notes |
|-------|--------|-------|
| System Boundaries | PASS | EconomySystem boundaries match systems.yaml |
| Tick Priority | PASS | Priority 60 documented in interfaces.yaml |
| Terminology | PASS | Uses alien terms throughout |
| IEconomyQueryable Stub Replacement | PASS | Stub behavior documented, replacement planned |
| Component Definitions | PASS | tick_priority added to EconomySystem |
| Dense Grid Exception | N/A | EconomySystem does not use dense grids |
| Data Contracts | PASS | IncomeBreakdown, ExpenseBreakdown, CreditAdvance added |

**Overall Status:** PASS (Canon v0.12.0)

---

## Detailed Verification

### 1. System Boundaries (systems.yaml)

**Location:** systems.yaml > phase_3 > epic_11_financial > EconomySystem

**Canon Definition:**
```yaml
EconomySystem:
  type: ecs_system
  owns:
    - Colony treasury
    - Tribute (tax) collection
    - Expense calculation
    - Budget allocation
    - Credit advances (bonds)
    - Ordinances
  does_not_own:
    - Individual building costs (defined per building)
    - What generates income (other systems report)
  provides:
    - Current treasury balance
    - Income/expense breakdown
    - Budget window data
  depends_on:
    - BuildingSystem (taxable buildings)
    - PopulationSystem (population for taxes)
    - ServicesSystem (service costs)
  components:
    - TaxableComponent
    - MaintenanceCostComponent
```

**Verification:**

| Canon Item | Epic 11 Design | Status |
|------------|----------------|--------|
| Owns: Colony treasury | TreasuryState per player | MATCH |
| Owns: Tribute collection | Tribute calculation engine (E11-007) | MATCH |
| Owns: Expense calculation | Maintenance calculation (E11-009, E11-010) | MATCH |
| Owns: Budget allocation | Funding level storage (E11-013) | MATCH |
| Owns: Credit advances | Bond system (E11-016, E11-017) | MATCH |
| Owns: Ordinances | Ordinance framework (E11-021) | MATCH |
| Does not own: Building costs | Costs queried from BuildingSystem | MATCH |
| Does not own: Income sources | Queries PopulationSystem, BuildingSystem | MATCH |
| Depends on: BuildingSystem | Yes, for taxable buildings | MATCH |
| Depends on: PopulationSystem | Yes, for occupancy data | MATCH |
| Depends on: ServicesSystem | Yes, for service costs | MATCH |
| Component: TaxableComponent | Defined in E11-001 | MATCH |
| Component: MaintenanceCostComponent | Defined in E11-001 | MATCH |

**Result:** PASS - System boundaries match canon

---

### 2. Tick Priority (interfaces.yaml)

**Location:** interfaces.yaml > simulation > ISimulatable > implemented_by

**Canon Definition:**
```yaml
implemented_by:
  - EconomySystem (priority: 60)
```

**Verification:**
- Epic 11 designs EconomySystem at tick_priority 60
- Runs after ServicesSystem (55)
- Runs before DisorderSystem (70)

**Result:** PASS - Tick priority matches canon

---

### 3. Terminology (terminology.yaml)

**Canon Economy Terms:**
```yaml
economy:
  money: credits
  dollar: credit
  dollars: credits
  budget: colony_treasury
  taxes: tribute
  tax_rate: tribute_rate
  income: revenue
  expenses: expenditure
  debt: deficit
  loan: credit_advance
  bond: treasury_bond
```

**Verification:**

| Human Term | Canon Term | Epic 11 Usage | Status |
|------------|------------|---------------|--------|
| money | credits | "credits" throughout | PASS |
| taxes | tribute | "tribute" throughout | PASS |
| tax_rate | tribute_rate | "tribute_rate" in code | PASS |
| income | revenue | "revenue" in UI text | PASS |
| expenses | expenditure | "expenditure" in UI text | PASS |
| debt | deficit | "deficit" for negative balance | PASS |
| loan | credit_advance | "credit_advance" in code | PASS |
| bond | treasury_bond | "treasury_bond" in UI | PASS |

**Result:** PASS - All terminology uses alien terms

---

### 4. IEconomyQueryable Stub Replacement (interfaces.yaml)

**Location:** interfaces.yaml > stubs > IEconomyQueryable

**Canon Definition:**
```yaml
IEconomyQueryable:
  description: "Query economy data (stub until Epic 11)"
  stub_behavior:
    get_tribute_rate: "Returns 7 (7% default rate)"
    get_average_tribute_rate: "Returns 7 (7% default rate)"
    get_treasury_balance: "Returns 20000 (starting credits)"
  implemented_by:
    - StubEconomyProvider (Epic 10 stub)
    - EconomySystem (Epic 11 real implementation)
  notes:
    - "Epic 10 provides stub implementation with default values"
    - "Epic 11 replaces stub with real EconomySystem"
```

**Verification:**
- Epic 11 implements real IEconomyQueryable (E11-005)
- Starting balance: 20,000 credits (matches stub)
- Default tribute rate: 7% (matches stub)
- Stub can be removed after Epic 11 completion

**Result:** PASS - Stub replacement planned correctly

---

### 5. Component Definitions

**Canon Status:** Components listed but not fully defined in systems.yaml

**Required Canon Updates:**

#### TaxableComponent
```yaml
# Needs to be added to systems.yaml or patterns.yaml
TaxableComponent:
  description: "Tags zone buildings that generate tribute"
  fields:
    - zone_type: ZoneBuildingType  # habitation, exchange, fabrication
    - base_value: uint32           # Base tribute per phase (50-400)
    - density_level: uint8         # 0=low, 1=high
    - tribute_modifier: float      # Modified by sector value
  size: "12 bytes"
  attached_to:
    - Zone buildings (habitation, exchange, fabrication)
```

#### MaintenanceCostComponent
```yaml
# Needs to be added to systems.yaml or patterns.yaml
MaintenanceCostComponent:
  description: "Tags entities with ongoing maintenance costs"
  fields:
    - base_cost: int32             # Cost per phase
    - cost_multiplier: float       # Modifier (age, damage)
  size: "8 bytes"
  attached_to:
    - Infrastructure (pathways, conduits, pipes, rail)
    - Service buildings
    - Energy nexuses
```

**Result:** NEEDS UPDATE - Component definitions need formal canon entry

---

### 6. Dense Grid Exception (patterns.yaml)

**Location:** patterns.yaml > ecs > dense_grid_exception

**Verification:**
EconomySystem does NOT use dense grids. Financial data is:
- Per-player (TreasuryState array)
- Per-entity (TaxableComponent, MaintenanceCostComponent)

No spatial grid data is maintained by EconomySystem.

**Result:** N/A - Not applicable to Epic 11

---

### 7. Data Contracts (interfaces.yaml)

**Canon Status:** Some data types used by Epic 11 are not in data_contracts

**Required Canon Updates:**

#### IncomeBreakdown
```yaml
# Needs to be added to interfaces.yaml > data_contracts
IncomeBreakdown:
  description: "Revenue breakdown by category for UI display"
  fields:
    - habitation_tribute: int64
    - exchange_tribute: int64
    - fabrication_tribute: int64
    - other_income: int64
    - total: int64
  used_by:
    - EconomySystem.get_income_breakdown()
    - UI Budget window
```

#### ExpenseBreakdown
```yaml
# Needs to be added to interfaces.yaml > data_contracts
ExpenseBreakdown:
  description: "Expenditure breakdown by category for UI display"
  fields:
    - infrastructure_maintenance: int64
    - service_maintenance: int64
    - energy_maintenance: int64
    - bond_payments: int64
    - ordinance_costs: int64
    - total: int64
  used_by:
    - EconomySystem.get_expense_breakdown()
    - UI Budget window
```

#### CreditAdvance
```yaml
# Needs to be added to interfaces.yaml > data_contracts
CreditAdvance:
  description: "A credit advance (bond) taken by a player"
  fields:
    - principal: int64
    - remaining_principal: int64
    - interest_rate_basis_points: uint16
    - term_phases: uint16
    - phases_remaining: uint16
    - is_emergency: bool
  used_by:
    - EconomySystem.active_bonds
    - UI Debt display
```

**Result:** NEEDS UPDATE - Data contracts need formal canon entry

---

### 8. Additional Verification Points

#### Service Types (for funding)
**Canon:** Not explicitly enumerated in canon
**Epic 11 Uses:** enforcer, hazard_response, medical, education

**Recommendation:** Add ServiceType enum to canon (terminology.yaml or interfaces.yaml)

```yaml
ServiceType:
  description: "Types of city services"
  values:
    - enforcer           # Police/security
    - hazard_response    # Fire/emergency
    - medical            # Health care
    - education          # Schools/learning
```

#### BondType (for credit advances)
**Canon:** Not defined
**Epic 11 Uses:** Small, Standard, Large, Emergency

**Recommendation:** Add BondConfig to canon (interfaces.yaml > data_contracts)

```yaml
BondConfig:
  description: "Configuration for a credit advance type"
  values:
    BOND_SMALL:
      principal: 5000
      interest_rate: 500      # 5%
      term_phases: 12
    BOND_STANDARD:
      principal: 25000
      interest_rate: 750      # 7.5%
      term_phases: 24
    BOND_LARGE:
      principal: 100000
      interest_rate: 1000     # 10%
      term_phases: 48
    BOND_EMERGENCY:
      principal: 25000
      interest_rate: 1500     # 15%
      term_phases: 12
```

---

## Canon Updates Required

The following updates should be applied to canon files in Phase 6:

### 1. systems.yaml

Add component definitions:
```yaml
# Under phase_3 > epic_11_financial > EconomySystem
components:
  TaxableComponent:
    fields:
      - zone_type: ZoneBuildingType
      - base_value: uint32
      - density_level: uint8
      - tribute_modifier: float
    size: 12 bytes

  MaintenanceCostComponent:
    fields:
      - base_cost: int32
      - cost_multiplier: float
    size: 8 bytes
```

Add tick_priority:
```yaml
EconomySystem:
  type: ecs_system
  tick_priority: 60  # After ServicesSystem (55), before DisorderSystem (70)
```

### 2. interfaces.yaml

Add data contracts:
```yaml
data_contracts:
  IncomeBreakdown:
    description: "Revenue breakdown by category"
    fields:
      - habitation_tribute: int64
      - exchange_tribute: int64
      - fabrication_tribute: int64
      - other_income: int64
      - total: int64

  ExpenseBreakdown:
    description: "Expenditure breakdown by category"
    fields:
      - infrastructure_maintenance: int64
      - service_maintenance: int64
      - energy_maintenance: int64
      - bond_payments: int64
      - ordinance_costs: int64
      - total: int64

  CreditAdvance:
    description: "A credit advance (bond)"
    fields:
      - principal: int64
      - remaining_principal: int64
      - interest_rate_basis_points: uint16
      - term_phases: uint16
      - phases_remaining: uint16
      - is_emergency: bool

  BondConfig:
    description: "Configuration for bond types"
    types:
      BOND_SMALL: {principal: 5000, rate: 500, term: 12}
      BOND_STANDARD: {principal: 25000, rate: 750, term: 24}
      BOND_LARGE: {principal: 100000, rate: 1000, term: 48}
      BOND_EMERGENCY: {principal: 25000, rate: 1500, term: 12}
```

Update stub notes:
```yaml
stubs:
  IEconomyQueryable:
    notes:
      - "Epic 10 provides stub implementation with default values"
      - "Epic 11 replaces stub with real EconomySystem"
      - "NOTE: Stub replaced by EconomySystem as of Epic 11"  # ADD THIS
```

### 3. terminology.yaml

Already complete - no updates needed.

### 4. patterns.yaml

No updates needed - EconomySystem does not use dense grids.

---

## Version Update

After applying canon updates, increment version:

**Current:** 0.11.0
**New:** 0.12.0

**Changelog Entry:**
```
| 0.12.0 | 2026-01-XX | Epic 11 canon integration: EconomySystem tick_priority: 60 (after ServicesSystem, before DisorderSystem), TaxableComponent and MaintenanceCostComponent definitions added, IncomeBreakdown/ExpenseBreakdown/CreditAdvance data contracts added, BondConfig constants defined, IEconomyQueryable stub marked as replaced |
```

---

## Verification Checklist

- [x] System boundaries match systems.yaml
- [x] Tick priority (60) documented
- [x] Alien terminology used throughout
- [x] IEconomyQueryable stub replacement documented
- [ ] TaxableComponent formally defined in canon
- [ ] MaintenanceCostComponent formally defined in canon
- [ ] IncomeBreakdown data contract in canon
- [ ] ExpenseBreakdown data contract in canon
- [ ] CreditAdvance data contract in canon
- [ ] BondConfig constants in canon
- [x] No dense grid needed (N/A)
- [ ] Canon version updated to 0.12.0

---

## Conclusion

Epic 11 design is **largely canon-compliant** with the following outstanding items:

1. **Component Definitions:** TaxableComponent and MaintenanceCostComponent need formal field definitions in systems.yaml
2. **Data Contracts:** IncomeBreakdown, ExpenseBreakdown, CreditAdvance, and BondConfig need entries in interfaces.yaml
3. **Version Bump:** Canon version should be updated to 0.12.0 after Epic 11 implementation

These are documentation updates only - no design changes required. The Epic 11 architecture follows all canon principles and patterns.
