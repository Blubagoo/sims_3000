# Game Design Review: Epic 11 - Financial System

**Reviewer:** Game Designer
**Date:** 2026-02-08
**Scope:** All 24 tickets (E11-001 through E11-024), headers, source, and tests
**Canon Version:** 0.13.0
**Overall Grade:** B+

---

## Executive Summary

Epic 11 delivers a structurally sound financial system with clean architecture, correct math, and thorough test coverage. The modular "pure calculation" approach (no ECS coupling in calculation modules) is an excellent engineering choice that also benefits design iteration -- we can re-tune numbers without touching system wiring.

The system faithfully implements the core planning document's vision of "economy as expression tool, not survival mechanic." Tribute rates, funding levels, bonds, and ordinances all provide meaningful player levers. However, the implementation reveals several game-feel gaps that need follow-up work before the economy will feel good in-session: the budget cycle is a stub, several feedback loops are incomplete, and the numeric balance has some concerning edge cases.

**Strengths:**
- Clean separation of calculation from system orchestration
- Comprehensive income/expense breakdown for UI transparency
- Sandbox-safe deficit handling (no bankruptcy, no death spiral)
- Tribute-to-demand feedback loop is well-tiered
- Good test coverage across unit and integration layers

**Concerns:**
- EconomySystem.process_budget_cycle() is still a stub -- the entire system is inert at runtime
- Bond interest math creates surprisingly cheap debt, undermining the risk/reward decision
- Ordinance costs are extremely high relative to early-game income
- No population-scaling on costs means late-game finances are trivially easy
- Missing feedback mechanisms (no trend projection, no milestone events, no "why" explanations)

---

## 1. Player Experience

### 1.1 Is the Financial System Fun and Understandable?

**Verdict: Mostly understandable, not yet fun.**

The data structures tell the right story. TreasuryState (E11-002) exposes a clean income/expense breakdown with per-zone tribute and per-service costs. A UI built on top of this will give players clear answers to "where is my money coming from?" and "where is it going?" This is the foundation of a good budget screen.

The tribute formula (E11-007) is well-designed from a comprehension standpoint:

```
tribute = base_value * occupancy_factor * value_factor * rate_factor * tribute_modifier
```

Each factor is intuitive: more residents pay more, higher land value pays more, higher rate extracts more. Players can reason about each knob independently. The sector_value mapping (0.5x at value 0, up to 2.0x at value 255) creates a clear "invest in land value to increase revenue" incentive.

**Problem:** The fun is currently blocked by the stub budget cycle. EconomySystem::process_budget_cycle() on line 217 of EconomySystem.cpp is empty. Until this is wired, the entire economy is non-functional at runtime. Players will see a static 20,000 credit balance forever.

**Problem:** There is no "projection" system. The IncomeHistory and ExpenseHistory circular buffers (E11-008, E11-011) track the last 12 phases and provide trend analysis, which is good. But there is no facility for "at current rates, you will run out of money in N phases" or "this bond will cost you X total over its lifetime." These projections are essential for players to make informed decisions about rate changes and bond issuance.

### 1.2 Player Agency

**Verdict: Good lever design, some levers feel inert.**

Players have four main financial levers:

| Lever | Range | Impact | Agency Feel |
|-------|-------|--------|-------------|
| Tribute rates (per zone) | 0-20% | Revenue + demand modifier | Strong |
| Service funding (per service) | 0-150% | Cost + effectiveness | Strong |
| Bond issuance | 4 types, max 5 | Immediate cash vs future payments | Moderate |
| Ordinances | 3 types, toggle | Cost + system effects | Weak (too few, too expensive) |

Tribute rates and service funding are the strongest levers. The per-zone tribute granularity (habitation/exchange/fabrication independently) is a good design decision -- it lets players favor growth in one sector while extracting from another.

The funding effectiveness curve (E11-013, FundingLevel.cpp lines 63-93) has excellent diminishing returns:

| Funding | Effectiveness |
|---------|--------------|
| 0% | 0.0 |
| 25% | 0.40 |
| 50% | 0.65 |
| 75% | 0.85 |
| 100% | 1.0 |
| 150% | 1.10 |

This is a well-shaped curve. The 25% -> 40% effectiveness point means "lean times" is still viable. The 150% -> 110% cap means overfunding has diminishing returns. Players who micromanage funding levels get rewarded with cost savings, but the ceiling prevents "just dump everything to max" from being dominant strategy.

**Problem:** Ordinances feel like a missed opportunity. Only 3 types exist, and their costs (1,000/2,000/5,000 per phase) are brutal relative to early-game income. A new city with 20,000 credits and near-zero income cannot meaningfully engage with ordinances. Free Transit at 5,000/phase would drain the starting treasury in 4 phases with zero income. Even Enhanced Patrol at 1,000/phase is heavy when a habitation building at default settings might generate ~8 credits/phase.

---

## 2. Balance Analysis

### 2.1 Starting Economy Math

Starting balance: 20,000 credits. Let us work through the early-game math.

**Typical early-city income (10 habitation buildings, all low-density, 7% tribute, 50% occupied, mid land value):**

```
Per building: base_value(50) * occupancy(0.5) * value_factor(~1.25) * rate(0.07) * modifier(1.0)
            = 50 * 0.5 * 1.25 * 0.07 * 1.0
            = ~2.18 credits/phase (truncated to 2)
Total:        10 buildings * 2 = ~20 credits/phase
```

**Typical early-city expenses (20 pathway tiles, 1 enforcer, 5 energy conduits):**

```
Pathways:   20 * 5 = 100
Enforcer:   1 * 100 = 100
E-Conduits: 5 * 2 = 10
Total:      210 credits/phase
```

**Net: 20 - 210 = -190 credits/phase.**

This is a significant problem. With only 10 habitation buildings at half occupancy, the player is losing ~190 credits per phase. At 200 ticks per budget cycle (10 seconds), the 20,000 starting balance lasts roughly 105 phases, or about 17 minutes of real time. The player will hit the -5,000 deficit warning around minute 19 and emergency bond territory by minute 21.

This is salvageable because building occupancy will increase and more buildings will appear, but the early-game burn rate is aggressive. The problem is that maintenance costs are fixed per-tile while tribute income depends on occupancy and building count -- two things that start very low.

**Recommendation:** Either reduce early-game infrastructure maintenance costs, or provide a "startup subsidy" income bonus for the first N phases.

### 2.2 Bond Interest Is Too Cheap

The bond interest formula (BudgetCycle.cpp line 84):

```
interest_payment = (remaining_principal * interest_rate_basis_points) / (10000 * 12)
```

The division by 12 makes sense if phases are monthly and interest rates are annual. But let us run the numbers for BOND_STANDARD (25,000 at 750bp = 7.5%, 24 phases):

```
Phase 1 interest:  (25000 * 750) / 120000 = 156 credits
Phase 1 principal: 25000 / 24 = 1041 credits
Phase 1 total:     1197 credits
```

Over 24 phases, total interest paid (from estimate_total_interest in BondRepayment.cpp) sums to approximately 1,950 credits on a 25,000 principal. That is an effective total cost of about 7.8% over the life of the bond.

The emergency bond (25,000 at 1500bp = 15%, 12 phases) total interest is approximately 1,218 credits -- only about 4.9% of principal despite the "punishing" 15% rate. The short term and the annual-to-phase division make even the emergency bond surprisingly cheap.

**Design Impact:** Bonds should create meaningful "will it pay off?" tension. At current rates, the answer is almost always "yes, obviously." A 25,000 credit injection that costs only ~1,950 in total interest over 24 phases is essentially free money. Players should never hesitate to max out bonds.

**Recommendation:** Either increase effective interest rates (perhaps remove the /12 divisor and use per-phase rates directly), or increase the spread between bond tiers more dramatically, or add a reputation/creditworthiness system where subsequent bonds get progressively more expensive.

### 2.3 Construction Costs vs. Income

Construction costs (E11-020) are reasonable for the starting balance:

| Item | Cost | % of Starting Balance |
|------|------|-----------------------|
| Pathway tile | 10 | 0.05% |
| Service Post | 500 | 2.5% |
| Service Station | 2,000 | 10% |
| Service Nexus | 5,000 | 25% |
| Zone (high-density) | 500-1,000 | 2.5-5% |

These feel right. Players can build meaningful infrastructure from the start without feeling poor, but large buildings require planning. The Service Nexus at 5,000 is a satisfying mid-game investment target.

**Problem:** There is no cost scaling by city size or era. A Service Nexus costs 5,000 whether the city has 100 or 100,000 population. In late-game, construction becomes trivially cheap relative to income. This removes the construction cost decision entirely for mature cities.

### 2.4 Ordinance Costs vs. Income

| Ordinance | Cost/Phase | Phases to Drain Starting 20K |
|-----------|-----------|------------------------------|
| Enhanced Patrol | 1,000 | 20 phases |
| Industrial Scrubbers | 2,000 | 10 phases |
| Free Transit | 5,000 | 4 phases |

As noted above, these are wildly expensive for early game. A city needs to generate at least 1,000+ credits/phase in net income before Enhanced Patrol is viable. Based on the tribute math, this requires hundreds of occupied buildings -- a mid-to-late game state.

**Recommendation:** Either add population-scaled ordinance costs (e.g., 2 credits per capita instead of flat 1,000), or reduce the flat costs by 5-10x for MVP, or gate ordinances behind population milestones so they only appear when they are affordable.

---

## 3. Feedback Loops

### 3.1 Positive Feedback Loops

**Tribute-Growth Loop (healthy):**
More buildings -> more tribute income -> more credits -> build more buildings -> more tribute.

This is the core positive loop and it works correctly in the code. The tribute calculation properly scales with building count and occupancy.

**Land Value-Tribute Loop (healthy):**
Better services -> higher land value -> higher value_factor (0.5-2.0x) -> more tribute -> fund more services.

The value_factor range of 0.5-2.0x means a high-value district generates 4x the tribute of a low-value one. This is a strong incentive to invest in land value, which is good.

**Service Effectiveness-Value Loop (healthy):**
Higher funding -> better service effectiveness -> better living conditions -> higher occupancy -> more tribute -> can afford more funding.

This loop exists structurally through ServiceFundingIntegration.h. The diminishing returns curve prevents it from becoming runaway.

### 3.2 Negative Feedback Loops

**Tribute-Demand Loop (well-tuned):**
High tribute rates reduce demand via TributeDemandModifier. The tiered system is well-designed:

```
0-3%:   +15 demand (bonus for low tribute)
4-7%:   0 (neutral zone, includes default 7%)
8-12%:  -4 per point above 7 (mild penalty, -4 to -20)
13-16%: -20 base, -5 per point above 12 (moderate, -25 to -40)
17-20%: -40 base, -5 per point above 16 (severe, -45 to -60)
```

This is good tiering. The default 7% sits right at the neutral boundary. Players have a 4-point "safe zone" (4-7%) before penalties kick in. The jump from tier 3 to tier 4 at 13% is a clear warning point. The max penalty of -60 at 20% is harsh but not terminal.

**Concern:** The "neutral zone" of 4-7% is quite wide. With 4 points of free rate increase, there is little reason not to set all rates to 7%. The decision between 4% and 7% has zero mechanical consequence. Consider making the neutral zone narrower (e.g., 6-7%) to force more meaningful trade-offs.

**Deficit Recovery Loop (correct but slow):**
Deficit triggers warning at -5,000, emergency bond at -10,000. Emergency bond injects 25,000 credits, bringing balance to ~15,000. This is a healthy recovery mechanism.

However, the emergency bond flag (emergency_bond_active) only resets when balance >= 0 (DeficitHandling.cpp line 58). If the player's structural deficit continues, they will burn through the 25K emergency bond, go negative again, but NOT receive another emergency bond. This is intentional (one emergency bond at a time) but could feel unfair if the player does not understand why help is not coming a second time.

### 3.3 Missing Feedback Loops

**Infrastructure Decay Loop (absent):**
MaintenanceCostComponent has a cost_multiplier field for "age, damage" but there is no system that increases this multiplier over time. Infrastructure never degrades. This removes an entire layer of maintenance pressure from the late game. Without decay, a mature city eventually has zero meaningful expenses because everything is already built.

**Population-to-Expense Scaling (absent):**
Service costs are flat per building (100/120/300/200 per phase). They do not scale with population served. A medical facility serving 50 beings costs the same as one serving 5,000. This makes late-game finances trivially easy -- income scales with population but expenses do not.

**Economic Event System (absent):**
The planning document describes treasury milestones, surplus celebrations, and informational events. None of these are implemented. There is no event bus, and while event structs exist (BudgetCycleCompletedEvent, DeficitWarningEvent, etc.), the EconomySystem stub does not emit them. Players will receive no economic notifications.

---

## 4. Progression

### 4.1 Early Game (0-5,000 population)

**State:** Players are spending the starting 20,000 credits on initial infrastructure. Income is very low (most buildings are empty or under-constructed). The primary financial concern is "don't run out of money before buildings fill up."

**Assessment:** The early game is too punishing with current numbers. The 20,000 starting balance sounds generous but burns fast at 200+ credits/phase in maintenance with near-zero income. The Small Bond (5,000 credits) is appropriately sized for early-game relief.

**Missing:** There is no "tutorial economy" period. A grace period of reduced maintenance for the first N phases, or a starting income subsidy, would smooth the on-ramp.

### 4.2 Mid Game (5,000-50,000 population)

**State:** Buildings are filling, tribute income is rising, players are making meaningful choices about tribute rates and service funding. Bonds become useful for expansion spurts. First ordinances become affordable.

**Assessment:** This is where the system should shine, and it mostly does. The per-zone tribute granularity allows differentiated strategies. The funding effectiveness curve makes budget allocation interesting. Bonds provide expansion fuel.

**Missing:** No economic milestones or achievements. The planning document describes these but they are not implemented.

### 4.3 Late Game (50,000+ population)

**State:** Income vastly exceeds expenses. Construction costs are negligible. All ordinances are easily affordable. Treasury grows indefinitely.

**Assessment:** The economy becomes irrelevant. With no population-scaled expenses, no infrastructure decay, no escalating service costs, and no new money sinks, late-game players have infinite money and no interesting financial decisions.

**Missing:** Late-game money sinks, escalating complexity (e.g., diminishing returns on city size, mega-projects, inter-city trade), or prestige spending options.

---

## 5. Player Communication

### 5.1 What Players Can Understand

The income/expense breakdown in TreasuryState is excellent for UI:

```
Income:  habitation_tribute, exchange_tribute, fabrication_tribute, other_income
Expense: infrastructure_maintenance, service_maintenance, energy_maintenance,
         bond_payments, ordinance_costs
```

This maps directly to a classic SimCity-style budget panel. Every line item is self-explanatory. The IncomeHistory and ExpenseHistory trend analysis (get_average, get_trend) provide the data for "your income is growing/shrinking" indicators.

### 5.2 What Players Cannot Understand

**Why did my income change?** The system tracks totals but not causes. If tribute income drops, players cannot see whether it was due to: buildings losing occupancy, land value decreasing, or tribute rate being too high. The TributeResult struct stores intermediate factors (occupancy_factor, value_factor, rate_factor) but these are per-building and not persisted or aggregated for UI consumption.

**What will happen if I change this rate?** There is no preview/projection system. Players cannot see "if I raise habitation tribute to 12%, my income will increase by X but demand will decrease by Y." This is critical for informed decision-making.

**Why is my city in deficit?** The deficit warning (DeficitWarningEvent) includes only balance and player_id. It does not explain what changed -- whether expenses spiked or income dropped. A diagnostic breakdown would help enormously.

### 5.3 Events and Warnings

The event system is structurally present but functionally absent:

| Event Struct | Defined | Emitted at Runtime |
|-------------|---------|-------------------|
| BudgetCycleCompletedEvent | Yes (E11-012) | No (stub) |
| DeficitWarningEvent | Yes (E11-015) | No (stub) |
| EmergencyBondOfferEvent | Yes (E11-015) | No (stub) |
| EmergencyBondIssuedEvent | Yes (E11-018) | Only in standalone function |
| BondIssuedEvent | Yes (E11-016) | Only in standalone function |
| BondPaidOffEvent | Yes (E11-017) | Only in standalone function |
| TributeRateChangedEvent | Yes (E11-006) | Only in standalone function |
| FundingLevelChangedEvent | Yes (E11-013) | Only in standalone function |
| OrdinanceChangedEvent | Yes (E11-021) | Not emitted anywhere |
| InsufficientFundsEvent | Yes (E11-020) | Not emitted anywhere |

Many events are defined but never emitted. The standalone calculation functions return event structs, but the EconomySystem does not process or route them. Without an event bus or callback mechanism, the UI layer has no way to receive these notifications.

---

## 6. Integration

### 6.1 Population/Demand Integration

The TributeDemandModifier (E11-019) is well-implemented as a standalone calculation. The get_zone_tribute_modifier function cleanly reads from TreasuryState and returns a demand modifier. However, the actual integration with a DemandSystem is not present in this epic -- only the calculation function exists.

### 6.2 Service Funding Integration

ServiceFundingIntegration (E11-014) provides calculate_funded_effectiveness and calculate_all_funded_effectiveness. These correctly bridge the funding level to service effectiveness via the diminishing returns curve. The integration is clean and testable.

### 6.3 Construction Cost Integration

The ICreditProvider interface in EconomySystem provides deduct_credits and has_credits for the building system. This is a good abstraction. However, deduct_credits (EconomySystem.cpp line 202) always succeeds and allows deficit spending:

```cpp
bool EconomySystem::deduct_credits(std::uint32_t player_id, std::int64_t amount) {
    // Allow deficit: always deduct and return true
    m_treasuries[player_id].balance -= amount;
    return true;
}
```

This is at odds with the ConstructionCost module, where deduct_construction_cost (ConstructionCost.cpp line 25) properly rejects if balance < cost. The two systems have contradictory affordability semantics. ICreditProvider allows unlimited deficit spending; ConstructionCost does not. This needs to be reconciled.

### 6.4 Network Sync

EconomyNetSync (E11-022) is clean and correctly implemented. The TreasurySnapshot at 41 bytes packed is compact. The magic-byte validation pattern is simple and effective. The on-change messages for tribute rates and funding levels are 3 bytes each -- very efficient.

**Note:** The snapshot does not include per-bond detail, only active_bond_count and total_debt. The comment on line 57 acknowledges this: "the client does not reconstruct individual bonds from the snapshot." This is fine for display purposes but means clients cannot show detailed bond information without an additional RPC.

### 6.5 The Stub Problem

The most critical integration issue: EconomySystem::process_budget_cycle() is a stub. This means:

- Treasury balance never changes from ticks
- Income is never calculated
- Expenses are never deducted
- Bonds are never processed
- Deficit handling never triggers
- Events are never emitted

All the calculation modules work correctly in isolation (proven by tests), but the orchestration layer that calls them each tick is unimplemented. The economy is architecturally complete but operationally inert.

---

## 7. Detailed Ticket-Level Notes

### Tickets That Are Well-Executed

- **E11-001 (Components):** Clean, trivially copyable, correct size assertions. Good.
- **E11-002 (TreasuryState):** Comprehensive state with clear field naming. Good.
- **E11-003 (CreditAdvance):** Bond presets are well-defined. BondType enum is clean. Good.
- **E11-006 (TributeRateConfig):** Clamping, get/set, event return. Solid utility module.
- **E11-007 (TributeCalculation):** Formula is correct and well-documented. Pure function design is excellent.
- **E11-008 (RevenueTracking):** Circular buffer history is a smart choice. Trend analysis is useful.
- **E11-011 (ExpenseTracking):** Mirrors IncomeHistory pattern consistently. Good.
- **E11-013 (FundingLevel):** Effectiveness curve is well-shaped with diminishing returns.
- **E11-017 (BondRepayment):** Detailed payment tracking with per-bond breakdowns. Good.
- **E11-022 (EconomyNetSync):** Compact, validated, well-structured network messages.
- **E11-023, E11-024 (Tests):** Comprehensive coverage across units and integration scenarios.

### Tickets That Need Follow-Up

- **E11-004/E11-005 (EconomySystem):** Skeleton only. Budget cycle is a stub. Must be completed.
- **E11-009 (InfrastructureMaintenance):** No age/decay multiplier system exists yet.
- **E11-014 (ServiceFundingIntegration):** Calculation exists but no actual ServicesSystem wiring.
- **E11-015 (DeficitHandling):** Logic is correct but never called from the system tick.
- **E11-019 (TributeDemandModifier):** Calculation exists but no actual DemandSystem wiring.
- **E11-020 (ConstructionCost):** Contradicts ICreditProvider's always-succeed behavior.
- **E11-021 (Ordinances):** Only 3 ordinances, costs are prohibitively high for early/mid game.

---

## 8. Follow-Up Recommendations

### Priority 1: Critical (Blocks Playability)

**GD-FU-001: Wire Budget Cycle into EconomySystem tick()**
- Complete the EconomySystem::process_budget_cycle() stub
- Must call: tribute calculation, maintenance calculation, bond processing, expense/income tracking, deficit handling
- This is the single most important follow-up -- without it, the economy does not function
- Related tickets: E11-004, E11-012

**GD-FU-002: Reconcile ICreditProvider vs. ConstructionCost Affordability**
- deduct_credits always succeeds (allows deficit); deduct_construction_cost checks balance
- Decision needed: should construction be allowed in deficit? If yes, remove the check from ConstructionCost. If no, fix ICreditProvider to reject when insufficient
- Related tickets: E11-005, E11-020

### Priority 2: High (Significant Game Feel Impact)

**GD-FU-003: Rebalance Bond Interest Rates for Meaningful Cost**
- Current effective total interest is 5-8% of principal over the bond life
- Bonds should cost 15-30% of principal to create real risk/reward tension
- Consider per-phase rates instead of annual/12 conversion
- Related tickets: E11-003, E11-012, E11-017

**GD-FU-004: Add Early-Game Income Floor or Maintenance Grace Period**
- Starting economy math shows -190 credits/phase net with basic infrastructure
- Options: startup subsidy (e.g., +500 credits/phase for first 20 phases), reduced maintenance for first N phases, or higher base tribute values
- Related tickets: E11-007, E11-009

**GD-FU-005: Rebalance Ordinance Costs for Accessibility**
- Current costs (1,000-5,000/phase) are unaffordable until late mid-game
- Scale costs with population (e.g., 1-5 credits per capita) or reduce flat costs by 5-10x
- Consider gating ordinance availability behind population milestones
- Related tickets: E11-021

**GD-FU-006: Wire Event Emission into EconomySystem**
- Event structs exist but are never emitted at runtime
- Need event bus or callback system to route: BudgetCycleCompleted, DeficitWarning, BondIssued, BondPaidOff, InsufficientFunds, OrdinanceChanged
- Critical for UI notifications and player communication
- Related tickets: E11-006, E11-012, E11-015, E11-016, E11-020, E11-021

### Priority 3: Medium (Improves Depth and Longevity)

**GD-FU-007: Add Budget Projection System**
- "At current rates, your balance in 10 phases will be X"
- "This bond will cost Y total interest over its lifetime"
- "Raising habitation tribute to 12% would increase income by Z"
- Essential for informed player decisions

**GD-FU-008: Add Population-Scaled Service Costs**
- Current flat costs (100-300/phase per building) do not scale with population served
- Late-game finances become trivially easy
- Consider per-capita cost component or tiered cost based on coverage area population

**GD-FU-009: Implement Infrastructure Decay System**
- MaintenanceCostComponent.cost_multiplier exists but nothing modifies it
- Aging infrastructure should gradually increase maintenance costs
- Creates a "replace vs. maintain" decision in late game
- Provides a natural money sink for wealthy cities

**GD-FU-010: Add Income Cause Diagnostics**
- When income changes, players need to know why
- Aggregate TributeResult intermediate factors (occupancy, value, rate) for UI display
- "Your habitation income dropped because occupancy fell from 80% to 60%"

**GD-FU-011: Narrow Tribute Neutral Zone**
- Rates 4-7% all produce zero demand modifier
- This makes the 4% to 7% range a "free" tax increase with no consequence
- Consider 6-7% neutral, with mild +5 bonus at 4-5%

### Priority 4: Low (Polish and Expansion)

**GD-FU-012: Expand Ordinance Library**
- 3 ordinances is too few for meaningful policy expression
- Target 8-12 ordinances across economic, safety, environmental, and growth categories
- Consider mutually exclusive ordinance pairs for interesting trade-offs

**GD-FU-013: Add Economic Milestone Events**
- Treasury reaches 50K, 100K, 500K
- First budget surplus
- All bonds paid off
- Positive reinforcement for financial management

**GD-FU-014: Add Late-Game Money Sinks**
- Mega-projects, prestige buildings, colony beautification
- Diminishing returns on city size (bureaucracy overhead)
- Inter-city trade preparation

**GD-FU-015: Add Bond Detail to Network Sync**
- Currently only bond_count and total_debt are synced
- Clients cannot show per-bond detail without additional queries
- Consider including the first N bonds in the snapshot or adding a bond detail RPC

---

## 9. Numeric Balance Summary Table

| Parameter | Current Value | Assessment | Recommendation |
|-----------|--------------|------------|----------------|
| Starting balance | 20,000 | Adequate | OK as-is |
| Default tribute rate | 7% | Good (neutral zone) | OK as-is |
| Tribute rate range | 0-20% | Good range | OK as-is |
| Low-density hab base tribute | 50 | Low | Consider 75-100 |
| High-density hab base tribute | 200 | Reasonable | OK as-is |
| Pathway maintenance | 5/tile/phase | Reasonable | OK as-is |
| Rail maintenance | 8/tile/phase | Reasonable | OK as-is |
| Service costs (enforcer) | 100/phase | Reasonable | OK as-is |
| Service costs (medical) | 300/phase | Reasonable | OK as-is |
| Bond interest (/12 annual) | Effective 5-8% total | Too cheap | Increase by 2-3x |
| Emergency bond rate | 15% annual (/12) | Not punishing enough | Increase to 25-30% or use per-phase |
| Ordinance costs | 1,000-5,000/phase flat | Way too expensive early | Scale with population |
| Funding max | 150% | Good | OK as-is |
| Max bonds | 5 | Good | OK as-is |
| Large bond pop req | 5,000 | Good gate | OK as-is |
| Deficit warning | -5,000 | Reasonable | OK as-is |
| Emergency bond trigger | -10,000 | Reasonable | OK as-is |
| Budget cycle frequency | 200 ticks (10s) | Good pacing | OK as-is |
| Demand neutral zone | 4-7% (4 points wide) | Too generous | Narrow to 6-7% |

---

## 10. Grade Justification

### Grade: B+

**What earns the B+:**
- Architecturally clean with proper separation of concerns
- All calculation modules are correct, pure, and well-tested (44 tests across 2 test suites)
- The tribute formula, funding curve, and demand modifier tiering are well-designed
- Sandbox-safe philosophy is properly implemented (no bankruptcy, always recoverable)
- Network sync is efficient and well-structured
- Foundation is solid for a great economy system

**What prevents an A:**
- The economy is operationally inert (process_budget_cycle is a stub)
- Bond interest is too cheap, removing the risk/reward tension
- Ordinance costs are prohibitive for early/mid game
- No population scaling on expenses means late-game is trivially easy
- Event system exists structurally but is not wired
- No projection/preview system for player decision-making
- ICreditProvider / ConstructionCost affordability contradiction

**Path to A:** Complete GD-FU-001 through GD-FU-006 (Priority 1 and 2 items). The calculation layer is already solid -- what is missing is the orchestration, the balance tuning, and the player communication layer.

---

*Review completed 2026-02-08. All file references based on implementation at time of review.*
