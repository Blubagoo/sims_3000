# Game Designer Analysis: Epic 11 - Financial System

**Author:** Game Designer Agent
**Date:** 2026-01-29
**Canon Version:** 0.11.0
**Status:** Planning Phase

---

## Player Experience Goals

### Core Fantasy

**"I am the economic steward of my colony, making wise investments that help my beings prosper."**

The financial system is where player decisions translate into colony growth. Unlike the survival-focused economies of competitive games, ZergCity's economy should feel like a **resource allocation puzzle** where there's no wrong answer - just different playstyles and trade-offs.

### What the Economy Should Feel Like

| Aspect | Player Feeling |
|--------|----------------|
| Colony Treasury | "My colony's wealth reflects my decisions" |
| Tribute (Taxes) | "I balance growth needs against being income" |
| Expenditure | "My investments in services and infrastructure pay off" |
| Credit Advances (Bonds) | "I can take calculated risks for faster growth" |
| Ordinances | "I can shape my colony's character through policy" |

### Emotional Beats

1. **Treasury Satisfaction** - Watching credits accumulate feels rewarding, not hoarded
2. **Investment Pride** - Spending on services/infrastructure feels productive, not costly
3. **Risk/Reward Tension** - Taking credit advances creates exciting "will it pay off?" moments
4. **Balance Achievement** - Ending a cycle in the positive feels like skilled management
5. **Recovery Triumph** - Climbing out of deficit feels earned and satisfying

### The Sandbox Financial Philosophy

In an **endless sandbox**, the economy creates texture, not pressure:

- Deficit is a state to manage, NOT a failure condition
- There is no bankruptcy, no game over, no forced restart
- Credit advances (bonds) allow deficit spending without punishment loops
- A colony can always recover - it just takes time and attention
- Money enables expression ("What do I want to build?") not survival ("Can I afford to exist?")

**Key Principle:** The economy should make players feel like overseers with OPTIONS, not survivors scrambling to pay bills.

---

## Interesting Decisions (That Don't Create Fail States)

### Decision 1: Tribute Rate Balancing

**The Trade-off:**
```
Higher Tribute Rate = More Revenue BUT Lower Demand (slower growth)
Lower Tribute Rate = Less Revenue BUT Higher Demand (faster growth)
```

**Why This Creates Decisions:**
- Early game: Lower tribute to encourage growth
- Mid game: Balanced tribute for sustainable services
- Late game: Can afford higher tribute because infrastructure is complete
- Emergency: Temporary rate hikes to fund crisis response

**Sandbox Safety:**
- Maximum tribute rate (20%) never generates negative demand
- Minimum rate (0%) is viable for wealthy players who want maximum growth
- Changes take effect gradually (beings don't flee instantly from high tribute)

**Design Intent:**
Tribute should feel like a dial between "investing in growth" and "funding services." Both extremes are valid playstyles.

**Proposed Tribute Tiers:**

| Tribute Rate | Effect on Demand | Player Perception |
|--------------|------------------|-------------------|
| 0-3% | +15% demand bonus | "Growth mode - building fast" |
| 4-7% | Neutral | "Balanced - sustainable" |
| 8-12% | -10 to -20% demand | "Revenue mode - funding services" |
| 13-16% | -25 to -40% demand | "High tribute - mature colony" |
| 17-20% | -50 to -60% demand | "Maximum extraction - no new growth" |

### Decision 2: Service Funding Allocation

**The Core Loop:**
```
Service Effectiveness = Base Effectiveness * Funding Level
More Funding = Better Services BUT Higher Expenditure
Less Funding = Weaker Services BUT More Credits for Other Uses
```

**Why This Creates Decisions:**
- Which services matter most right now?
- Can I underfund enforcers in low-disorder areas?
- Do I over-fund medical to boost longevity stats?
- How lean can I run during expansion phases?

**Sandbox Safety:**
- Services at 0% funding are inactive but NOT destroyed
- Services at 25% funding still provide some benefit
- Defunded services can be reactivated instantly
- No "death spiral" from service cuts - effects are gradual

**Funding Impact Table:**

| Funding Level | Service Effectiveness | Expenditure | Player Scenario |
|---------------|----------------------|-------------|-----------------|
| 0% (Defunded) | 0% | 0% of base | Crisis cost-cutting |
| 25% | 40% | 25% of base | Lean times |
| 50% | 65% | 50% of base | Budget-conscious |
| 75% | 85% | 75% of base | Normal operations |
| 100% | 100% | 100% of base | Full service |
| 125% | 110% | 125% of base | Premium/overtime |

**Design Intent:**
Players should feel like they're making meaningful choices, not just setting everything to 100%.

### Decision 3: Credit Advances (Bonds)

**The Trade-off:**
```
Taking a Credit Advance = Immediate Credits NOW
BUT Interest Payments LATER + Reduced Credit Limit
```

**Why This Creates Decisions:**
- Do I accelerate infrastructure now or grow organically?
- Is this opportunity worth the future cost?
- How much debt is comfortable for my playstyle?
- Can I pay off existing advances before taking new ones?

**Sandbox Safety (CRITICAL):**

| Risk | Mitigation |
|------|------------|
| Can't pay interest | Interest accrues but doesn't compound infinitely - caps at 2x principal |
| Max debt reached | Can still operate, just can't take more advances |
| Negative treasury | Services still run (at reduced effectiveness), buildings don't disappear |
| "Trapped" by debt | Credit advances can be restructured (extend term, reduce payments) |

**Proposed Credit Advance Structure:**

| Advance Tier | Principal | Interest Rate | Term (Cycles) | Monthly Payment |
|--------------|-----------|---------------|---------------|-----------------|
| Minor | 5,000 cr | 5% | 10 cycles | ~525 cr/cycle |
| Standard | 20,000 cr | 6% | 20 cycles | ~1,100 cr/cycle |
| Major | 50,000 cr | 7% | 30 cycles | ~1,850 cr/cycle |
| Emergency | 100,000 cr | 8% | 50 cycles | ~2,200 cr/cycle |

**Design Intent:**
Credit advances should feel like a strategic tool, not a trap. Players who use them wisely should feel clever; players who avoid them should feel virtuous.

### Decision 4: Ordinances (Special Policies)

**The Trade-off:**
```
Enabling an Ordinance = Specific Benefit
BUT Ongoing Credit Cost AND/OR Trade-offs
```

**Why This Creates Decisions:**
- Which ordinances fit my colony vision?
- Is the benefit worth the cost?
- Do I have room in my budget?
- How do ordinances interact with each other?

**Proposed Ordinance Categories:**

| Category | Example Ordinance | Effect | Cost/Trade-off |
|----------|-------------------|--------|----------------|
| Economic | "Trade Incentives" | +15% exchange zone demand | 200 cr/cycle |
| Safety | "Enhanced Enforcement" | +20% enforcer effectiveness | 300 cr/cycle |
| Environmental | "Contamination Scrubbers" | -25% contamination spread | 400 cr/cycle |
| Growth | "Settler Subsidies" | +25% migration in | 500 cr/cycle + -5% tribute revenue |
| Austerity | "Streamlined Services" | -15% service costs | -10% service effectiveness |

**Sandbox Safety:**
- Ordinances can be enacted/revoked at any time
- No permanent commitments
- Effects are beneficial, not mandatory
- Multiple ordinances can be active simultaneously

**Design Intent:**
Ordinances let players express their vision: "I run a trade colony" vs. "I prioritize safety" vs. "I grow fast."

---

## Feedback Mechanisms

### How Players Know Their Financial Health

#### 1. Treasury Display (Always Visible)

**Primary Indicators:**

| Signal | What It Shows |
|--------|---------------|
| Treasury Balance | Current credits (prominent number) |
| Trend Arrow | Up/down/stable indicator |
| Balance Color | Green (healthy), Yellow (tight), Orange (deficit) |
| Income/Expense Summary | Net change per cycle (simplified) |

**Visual Design:**
- Treasury should be visible at all times (status bar)
- Not alarming - deficit is orange, not red/flashing
- Trend matters more than absolute number for casual monitoring

#### 2. Budget Panel (Detailed View)

**Revenue Breakdown:**

| Source | Information Shown |
|--------|-------------------|
| Habitation Tribute | Amount + rate + building count |
| Exchange Tribute | Amount + rate + building count |
| Fabrication Tribute | Amount + rate + building count |
| Other Income | Trade deals, ordinance bonuses |
| **Total Revenue** | Sum with comparison to last cycle |

**Expenditure Breakdown:**

| Category | Information Shown |
|----------|-------------------|
| Services | Per-service cost + funding level |
| Infrastructure | Maintenance costs |
| Credit Advance Payments | Interest + principal breakdown |
| Ordinance Costs | Per-ordinance listing |
| **Total Expenditure** | Sum with comparison to last cycle |

**Visual Design:**
- Classic SimCity 2000-style budget window
- Sliders for tribute rates and service funding
- Clear income vs. expense visualization
- Projection: "At current rates, in 10 cycles you'll have..."

#### 3. Economic Health Indicators

| Indicator | What It Measures | Player Understanding |
|-----------|------------------|----------------------|
| Net Cash Flow | Revenue - Expenditure | "Am I making or losing credits?" |
| Debt Ratio | Total debt / Annual revenue | "How leveraged am I?" |
| Reserve Cycles | Treasury / Monthly expenditure | "How long can I sustain a crisis?" |
| Growth Potential | Available budget for new spending | "Can I afford to expand?" |

#### 4. Economic Events and Notifications

**Positive Events:**

| Event | Notification | Feeling |
|-------|--------------|---------|
| Treasury milestone (10K, 50K, 100K) | "Treasury reaches 50,000 credits!" | Achievement |
| Cycle surplus | "Surplus of 2,500 credits this cycle" | Satisfaction |
| Credit advance paid off | "Minor credit advance fully repaid!" | Accomplishment |
| High-value development | "Premium exchange district generates bonus revenue" | Pride |

**Informational Events (Not Alarming):**

| Event | Notification | Feeling |
|-------|--------------|---------|
| Entering deficit | "Treasury entering deficit - 5,200 cr shortfall" | Attention needed |
| Low treasury warning | "Reserve funds below 2 cycles of expenses" | Planning prompt |
| Interest payment due | "Credit advance payment: 1,100 cr" | Reminder |
| Service underfunded | "Enforcer funding at 50% - reduced effectiveness" | Trade-off acknowledged |

**Design Intent:**
Notifications should inform, not panic. Deficit is a state to manage, not an emergency.

### Feedback Timing

| Economic Event | Feedback Speed | Player Expectation |
|----------------|----------------|-------------------|
| Tribute rate change | Immediate (next cycle) | "I should see the effect soon" |
| Service funding change | Immediate | "Services respond right away" |
| Credit advance | Instant credits | "Money now, payments later" |
| Ordinance effect | 1-2 cycles | "Policy takes time to implement" |
| Debt payoff | Immediate relief | "No more payments!" |

---

## Questions for Other Agents

### @systems-architect: EconomySystem Integration

1. **Tick Priority:** Epic 9 ServicesSystem is priority 55. Where should EconomySystem run?
   - Proposed: Priority 60 (after services calculate costs, before LandValueSystem)
   - EconomySystem needs to know service costs to calculate expenditure

2. **Per-Player Treasury:** Each player has their own treasury. How is this stored?
   - ECS component on a "player" entity?
   - Or dedicated EconomySystem member data per PlayerID?

3. **IEconomyQueryable Interface:** Epic 10 defines a stub interface. What methods does the real EconomySystem need to expose?
   - `get_tribute_rate(zone_type, owner)` - confirmed
   - `get_treasury_balance(owner)` - confirmed
   - What else? `can_afford(amount, owner)`? `get_debt_ratio(owner)`?

4. **Credit Advance Tracking:** How do we track multiple outstanding credit advances?
   - Component per advance? Vector in EconomySystem?
   - Need: principal, interest_rate, remaining_term, payment_per_cycle

### @systems-architect: Service Cost Integration

5. **Service Funding Data Flow:**
   - Does EconomySystem tell ServicesSystem how much each service is funded?
   - Or does ServicesSystem query EconomySystem for available budget?
   - Proposed: EconomySystem sets funding levels, ServicesSystem queries them

6. **Infrastructure Maintenance:**
   - How do we calculate maintenance for roads, conduits, pipes?
   - Query TransportSystem, EnergySystem, FluidSystem for infrastructure counts?
   - Or register a MaintenanceCostComponent on each infrastructure entity?

### @economy-engineer: Financial Formulas

7. **Tribute Calculation Formula:**
   - Is tribute calculated per-building or per-zone?
   - Proposed: Per-building based on sector value
   - `tribute = base_tribute * sector_value_multiplier * tribute_rate`

8. **Interest Compounding:**
   - Simple interest or compound?
   - Proposed: Simple interest (principal stays fixed, interest accrues per cycle)
   - Sandbox safety: Total owed caps at 2x principal

9. **Deficit Consequences:**
   - What happens when treasury is negative?
   - Proposed: Service effectiveness scales down proportionally
   - At -10,000 cr, services run at 80% even with 100% funding
   - At -50,000 cr, services run at 50%
   - Services never drop below 25% regardless of deficit

### @economy-engineer: Ordinance Design

10. **Ordinance Implementation:**
    - Are ordinances ECS components on the player entity?
    - Or flags/enums in EconomySystem?
    - Need to track: enabled, cost_per_cycle, effect_parameters

11. **Ordinance Limits:**
    - Can players enable unlimited ordinances (if they can afford it)?
    - Or cap at e.g., 5 active ordinances?
    - Proposed: No hard cap, budget is the natural limit

---

## Balancing Considerations

### Tribute Rate Balancing

**Goal:** Tribute should feel meaningful but never punitive.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Default tribute rate | 7% | Neutral starting point |
| Minimum tribute rate | 0% | Allows "no tribute" playstyle |
| Maximum tribute rate | 20% | High but not exploitative |
| Demand impact per 1% | -3% to -5% | Noticeable but not dramatic |
| Rate change smoothing | 2 cycles | Prevents instant demand swings |

**Balance Philosophy:**
A colony at 20% tribute should still function - just with slower growth. A colony at 0% tribute should struggle with service funding but thrive on demand.

### Credit Advance Balancing

**Goal:** Credit advances should be useful tools, not traps.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Interest rates | 5-8% | Low enough to be attractive |
| Maximum debt ratio | 200% of annual revenue | Generous ceiling |
| Minimum payment | 2% of principal/cycle | Manageable even in crisis |
| Missed payment penalty | None (interest accrues) | Sandbox-safe |
| Maximum total owed | 2x principal | Prevents infinite debt spiral |

**Balance Philosophy:**
Taking credit advances should feel like a calculated business decision. Players who over-leverage should feel the squeeze but never be locked out of play.

### Service Funding Balancing

**Goal:** Funding should create meaningful choices without punishing budget-conscious players.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Base service cost | Varies by service | Medical > Enforcers > Hazard Response > Education |
| Cost scaling | Linear with colony size | 10K beings = 10x base cost |
| Funding floor | 0% | Can fully defund |
| Funding ceiling | 125% | Diminishing returns above 100% |
| Defund recovery time | Instant | No penalty for reactivating |

**Balance Philosophy:**
A lean colony should work. A rich colony should work better. Neither should fail.

### Ordinance Balancing

**Goal:** Ordinances should feel like meaningful policy choices.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Base ordinance cost | 100-500 cr/cycle | Meaningful but not crippling |
| Effect magnitude | 10-25% modifier | Noticeable impact |
| Number limit | None (budget-limited) | Player agency |
| Enact/revoke delay | 1 cycle | Prevents micro-optimization |

**Balance Philosophy:**
Ordinances should define your colony's character. A trade-focused colony, an eco-colony, and a growth colony should all feel different.

### Starting Economy

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Starting treasury | 20,000 cr | Enough for initial infrastructure |
| Starting debt | 0 | Fresh start |
| Initial credit limit | 50,000 cr | Room to grow |
| First cycle costs | ~500 cr | Low burn rate at start |

---

## Alien Theme Notes

### Terminology Reminders

| Human Term | Alien Term (Use This) | Context |
|------------|----------------------|---------|
| Money | Credits | Currency |
| Dollar(s) | Credit(s) | Unit |
| Budget | Colony treasury | Overall financial state |
| Taxes | Tribute | Revenue from zones |
| Tax rate | Tribute rate | Percentage extracted |
| Income | Revenue | Money coming in |
| Expenses | Expenditure | Money going out |
| Debt | Deficit | Negative balance state |
| Loan | Credit advance | Borrowed money |
| Bond | Treasury bond | Financial instrument |
| Interest | Advance cost | Price of borrowing |

### Visual Theme Integration

**Colony Treasury Aesthetics:**
- Credits are not physical coins - think energy units, quantum tokens
- Treasury visualized as a "resource pool" or "energy reserve"
- Bioluminescent glow intensity could reflect treasury health
- Rich colonies have vibrant glow; struggling colonies are dimmer

**Budget Panel Aesthetics:**
- Classic layout with alien terminology
- Sliders have bioluminescent accents
- Positive numbers in cyan/green glow
- Negative numbers in amber (not red - not alarming)
- Charts and graphs with flowing data visualization

**Credit Advance Aesthetics:**
- Not "banks" - think "resource arbitrage" or "temporal exchange"
- Taking an advance could be visualized as "borrowing from future energy"
- Debt displayed as a "deferred obligation" meter
- Paying off advances shows energy returning to balance

**Ordinance Aesthetics:**
- Ordinances are "colony protocols" or "administrative directives"
- Active ordinances displayed as glowing policy icons
- Each ordinance has a unique visual symbol reflecting its effect
- Colony aesthetic subtly shifts based on active policies

### Alien Narrative Hooks

**Why Do Aliens Have an Economy?**
Even advanced alien civilizations require resource allocation. Credits represent accumulated productive capacity - the colony's stored ability to BUILD and MAINTAIN. Tribute is not taxation in the human sense - it's the colony's share of beings' productive output, willingly contributed for collective benefit.

**Why Credit Advances?**
The colony can "borrow" resources from projected future production. This temporal exchange allows overseers to accelerate development, but future cycles must repay the borrowed potential. It's not debt in the human sense - it's a resource time-shift.

**Why Ordinances?**
Ordinances are collective agreements that redirect colony focus. When beings agree to "Trade Incentives," they collectively prioritize exchange productivity. When "Enhanced Enforcement" is active, more beings volunteer for peacekeeper duty. Ordinances reflect the colony's social choices, not imposed laws.

---

## Multiplayer Economy Considerations

### Per-Player Treasury

Each player manages their own treasury. There is NO shared economy.

| Aspect | Design |
|--------|--------|
| Treasury | Completely separate per player |
| Tribute | Collected only from your zones |
| Services | Funded only by you, serve only your zones |
| Credit Advances | Personal obligation, not shared |
| Ordinances | Apply only to your territory |

### Cross-Player Economic Interactions

| Interaction | Design |
|-------------|--------|
| Trade deals | Not in MVP (future feature) |
| Shared infrastructure costs | Not in MVP |
| Economic comparisons | Visible - see rivals' treasury (optional) |
| Economic victory | Not applicable (sandbox) |

**Design Intent:**
Players should focus on their own economy without worrying about rivals' financial state. Economic interactions are a future feature, not MVP.

### Economic Catch-Up

Since this is a sandbox without victory conditions:

- Trailing players are NOT punished economically
- New/late-joining players start with standard treasury
- No "economic snowball" mechanics
- Smaller colonies are proportionally cheaper to run

---

## Summary

Epic 11 introduces the **financial system as an expression tool**, not a survival mechanic. The key design principles are:

1. **Sandbox Safety:** No bankruptcy, no game over, always recoverable
2. **Meaningful Choices:** Tribute rates, funding levels, credit advances, ordinances
3. **Clear Feedback:** Players understand their financial health at a glance
4. **Alien Authenticity:** Credits, tribute, and advances fit the theme
5. **Multiplayer Isolation:** Each player manages their own economy

**Core Player Experience:** "I thoughtfully manage my colony's resources, making trade-offs between growth, services, and investment. My financial decisions shape what kind of overseer I am - lean and aggressive, generous and nurturing, or balanced and sustainable. I never feel trapped or punished by the economy."

---

## Next Steps

1. @systems-architect: Please provide technical analysis on EconomySystem integration, tick ordering, and data structures
2. @economy-engineer: Please provide detailed formulas for tribute calculation, interest computation, and service cost scaling
3. Validate balancing values against Epic 9 (Services) and Epic 10 (Simulation) dependencies
4. Define the specific ordinances available at launch
5. Design the budget UI panel for both Classic and Holographic modes
