# Epic 8: Ports & External Connections - Game Design Review

**Reviewer:** Game Designer Agent
**Date:** 2026-02-08
**Canon Version:** 0.13.0
**Epic:** 8 - Ports & External Connections
**Files Reviewed:** 28 port headers, tickets.md, port-system-spec.md, canon files

---

## Verdict: APPROVED WITH NOTES

The port and external connections system establishes a compelling gameplay pillar that city builders need: the feeling that your colony exists in a larger world. The two-port-type design, NPC neighbor diplomacy, inter-player trade, and contamination tradeoffs create a layered decision space. The technical foundation is clean -- 16-byte packed components, clear IPortProvider interface, well-defined tick phases, and thorough config externalization. However, several balance issues, a significant gap in the demand bonus interaction model, and a lack of player feedback mechanisms need to be addressed before this system delivers a satisfying gameplay loop.

**Confidence Score: 7/10**

The architectural and technical decisions are sound. Confidence is moderated by balance values that will require playtesting to validate, a few design gaps that could produce confusing player experiences, and one structural issue with how diminishing returns interact with the +30 demand bonus cap.

---

## Strengths

### 1. Two Port Types With Distinct Strategic Roles

The Aero/Aqua distinction is not cosmetic -- each port type boosts a different zone demand (Exchange vs. Fabrication), has different terrain prerequisites (flat runway vs. water adjacency), and produces different contamination types (noise vs. industrial). This forces genuine strategic decisions: where you place your port determines what your economy optimizes for. A colony with a natural harbor has a different growth trajectory than one with flat interior terrain. This is the kind of terrain-driven strategy that made SimCity maps feel unique.

### 2. Diminishing Returns Encourage Diversification (E8-035)

The 100%/50%/25%/12.5% diminishing returns curve for same-type ports is well-calibrated. It makes the second port of the same type a reasonable investment (50% is still meaningful), but the third port starts to feel wasteful. This naturally pushes players toward building one of each type rather than stacking, which creates more interesting city layouts and broader economic profiles. The header-only inline implementation is clean and efficient.

### 3. NPC Neighbor Relationship System Creates a Diplomacy Mini-Game (E8-034)

The six-tier relationship system (Hostile through Allied) with trade-gated tier unlocks gives players a long-term investment arc with their neighbors. Starting at Neutral (Basic deals only) and working toward Allied (Premium deals) requires sustained positive trade activity. This creates a satisfying progression that rewards consistent engagement rather than one-time actions. The -100 to +100 range with well-spaced thresholds provides granular feedback.

### 4. Contamination Creates Genuine Tradeoffs (E8-033)

Ports are not free bonuses. Aero ports generate noise in a 10-tile radius and aqua ports generate industrial contamination in an 8-tile radius, both reducing sector_value. The linear falloff with Manhattan distance is clean and understandable. This creates the classic city builder tension: ports boost your economy but degrade nearby land value. Players must decide between placing ports near their economic zones (for local demand bonuses) or far away (to avoid contamination). The fact that local demand bonus radii (20 and 25 tiles) are larger than contamination radii (10 and 8 tiles) means there exists a sweet spot -- close enough for the bonus, far enough from the noise.

### 5. Server-Authoritative Trade Offers Are Correctly Designed (E8-025)

The TradeOfferManager with server-authoritative create/accept/reject lifecycle, 500-tick expiry, and duplicate-offer prevention is solid multiplayer architecture. No client can forge a trade agreement. The offer expiration prevents stale offers from cluttering the system. The validation checks (different players, valid IDs, no self-trade) cover the obvious abuse cases.

### 6. Clean Config Externalization Across the Board

Every balance-relevant value is a named constexpr: capacity formulas, demand bonus thresholds, contamination radii, diminishing returns multipliers, trade deal costs, migration factors. The spec documents 40+ config constants. This is essential for iteration and shows mature engineering discipline.

---

## Concerns

### Concern 1: Demand Bonus Cap and Diminishing Returns Create a Confusing Interaction (HIGH)

The global demand bonus is capped at +30 (MAX_TOTAL_DEMAND_BONUS). Diminishing returns reduce each subsequent port's contribution. Trade deals also add demand bonuses (+5/+10/+15 per deal tier). Local demand bonuses stack on top of global bonuses. The combined bonus (global + local) is also capped at +30.

The problem: there are too many bonus sources feeding into one cap, and the player has no way to understand which sources are being truncated. Consider a player with:
- One Large aero port: +15 global bonus
- One Premium NPC trade deal: +15 demand bonus
- That alone hits the +30 cap

Any additional ports, trade deals, or local bonuses are completely wasted. The player built a second aero port (which the diminishing returns system already penalizes) only to discover that it contributes zero effective bonus because the cap was already reached through trade deals.

The issue is not the cap itself (caps are necessary), but that the player cannot predict or diagnose why building more infrastructure yields no additional demand.

**Recommendation:** Either (a) separate the cap into distinct buckets (port cap +30, trade deal cap +15, local cap +10) so sources do not cannibalize each other, or (b) expose a "demand bonus breakdown" diagnostic showing how much of the cap is consumed by each source and how much is being wasted.

### Concern 2: Trade Income Formula Produces Very Low Revenue (MODERATE)

Running the numbers for a fully operational Large aero port with a Premium trade deal:

```
capacity = 2500 (Large aero, max)
utilization = 0.9 (Large tier estimate)
income_rate = 0.8 (aero)
trade_multiplier = 1.2 (Premium deal)
external_demand_factor = 1.0

income = 2500 * 0.9 * 0.8 * 1.2 * 1.0 = 2,160 credits/phase
```

Meanwhile, the Premium trade deal costs 5,000 credits/cycle. A Large aero port at maximum capacity with the best possible trade deal generates less income than the deal itself costs. The player is paying 5,000 credits per cycle for a deal that adds only the difference between the 1.2x and 1.0x multiplier:

```
income_without_deal = 2500 * 0.9 * 0.8 * 1.0 * 1.0 = 1,800 credits/phase
income_with_deal = 2,160 credits/phase
net deal benefit (income only) = 360 credits/phase
deal cost = 5,000 credits/cycle
```

Even if "phase" and "cycle" are equivalent, the Premium deal costs 5,000 and generates only 360 extra income. The player is paying 5,000 for a +360 income boost plus a +15 demand bonus. The demand bonus is the real value, but the income component is misleading -- it looks like a trade income system but the income is negligible.

**Recommendation:** Either increase income rates significantly (3-5x current values), reduce deal costs, or reframe trade deals explicitly as "demand investment" rather than "trade income." If trade deals are meant to be purchased primarily for the demand bonus, the income multiplier tier column in E8-022 is confusing because it suggests income is the primary benefit.

### Concern 3: Aqua Port Capacity Is Dramatically Higher Than Aero With No Compensating Cost (MODERATE)

Aqua ports have a max capacity of 5,000 (vs. 2,500 for aero), a higher base capacity per tile (15 vs. 10), and benefit from both dock and rail multipliers that can stack to very high values. A well-placed aqua port with 8 docks and rail connection:

```
base = 80 tiles * 15 = 1200
dock_bonus = 1.0 + (8 * 0.2) = 2.6
water_access = 1.0
rail_bonus = 1.5
total = 1200 * 2.6 * 1.0 * 1.5 = 4,680 (capped at 5,000)
```

An equivalent-size aero port:

```
base = 80 tiles * 10 = 800
runway_bonus = 1.5
access_bonus = 1.0
total = 800 * 1.5 * 1.0 = 1,200
```

The aqua port produces 3.9x the capacity of an equivalent aero port. This is partially offset by the fact that aqua ports require water-adjacent terrain (which is scarcer), but the magnitude of the difference means aqua ports are strictly superior as trade income generators. Aero ports need a compensating advantage to be strategically comparable.

**Recommendation:** Either give aero ports a higher income rate (1.2 vs. 0.6, so income per unit favors aero) or provide aero ports with a unique benefit that aqua ports lack (such as boosting migration capacity, or a larger local demand bonus radius). Currently, aero income is 0.8 vs. aqua 0.6, which narrows the gap but does not close it -- a Large aqua port at 0.6/unit still earns more than a Large aero port at 0.8/unit because the capacity difference is so large.

### Concern 4: Migration System Lacks Player Agency (MODERATE)

The migration formula (E8-024) is entirely passive. Immigration and emigration happen based on harmony_factor, disorder_index, and connection capacity. The player influences migration indirectly (by building connections and maintaining harmony) but has no direct migration tools: no immigration policy, no border control, no recruitment campaigns.

In contrast, SimCity games gave players direct control over zoning to attract specific populations. The migration system as designed feels like a background effect rather than a gameplay lever.

**Recommendation:** Add at least one active migration mechanic -- perhaps a "recruitment bonus" that can be purchased through the trade deal system, or a migration-focused port upgrade that increases max_immigration. The player should have at least one button they can press that says "I want more immigrants."

### Concern 5: No Feedback When Ports Become Non-Operational (MODERATE)

A port becomes non-operational when any of four conditions fails: zone validation, infrastructure, pathway connectivity, or capacity > 0. The PortOperationalEvent fires on state change, but there is no mechanism to tell the player *why* the port went down. Did the pathway get demolished? Did the dock count drop below minimum? Did terrain change?

A Large port going offline could cost thousands of credits per cycle in lost trade income. The player needs an immediate, clear explanation.

**Recommendation:** Add a PortDiagnostic struct that returns which of the four operational conditions are failing, queryable by the UI. Similar to F7-GD-005 (congestion diagnostic) from the Epic 7 review.

### Concern 6: Trade Deal Duration Is Very Long and Not Clearly Communicated (LOW)

Default durations are 500/1,000/1,500 cycles. Without knowing the tick rate, a player cannot evaluate whether a 1,500-cycle Premium deal is a 5-minute commitment or a 2-hour one. The tickets do not specify how deal duration maps to real-time or in-game time. Additionally, the only escape from a bad deal is to wait for expiration or get downgraded due to payment failure -- there is no voluntary cancellation mechanic for NPC deals (though E8-025/E8-026 define TradeCancelRequest for inter-player trade).

**Recommendation:** Add a cancel_trade_deal function for NPC deals (with a penalty, such as relationship point loss). Also document the expected real-time equivalent of deal durations so designers can evaluate whether the time commitment is appropriate.

### Concern 7: Local Demand Bonuses Do Not Reference Diminishing Returns (LOW)

calculate_local_demand_bonus in DemandBonus.h stacks local bonuses from all nearby ports without applying diminishing returns. The DiminishingReturns.h only applies to calculate_global_demand_bonus_with_diminishing. This means a player who builds three aero ports within 20 tiles of a habitation zone gets 3x the +5% local bonus (=+15%) with no diminishing returns, even though the global bonus from those same three ports is heavily diminished.

This is not necessarily wrong (local proximity bonuses rewarding dense clustering has design merit), but it creates an inconsistency that could confuse players: "Why does my third port barely help global demand but fully help local demand?"

**Recommendation:** Document this as an intentional design decision, or apply a softer diminishing curve to local bonuses (e.g., 100%/75%/50% instead of 100%/50%/25%).

### Concern 8: PortContamination Intensity Is Unspecified (LOW)

The PortContaminationSource struct has an `intensity` field (0-255) but no ticket, spec, or config constant defines what the default intensity values should be. Without defined intensity, the contamination system is technically complete but has no actual gameplay effect until someone picks a number.

**Recommendation:** Define default intensity constants: AERO_NOISE_INTENSITY and AQUA_INDUSTRIAL_INTENSITY (suggested: 120 and 100 respectively, producing meaningful but not overwhelming sector_value reduction). Add these to the config constants table in the spec.

---

## Balance Analysis

### Demand Bonus Economy

The demand bonus system has three layers: global (+5/+10/+15 per port, capped at +30), local (+5% aero/habitation, +10% aqua/exchange within radius), and trade deal bonuses (+5/+10/+15 per deal tier). With diminishing returns on multiple same-type ports, the practical maximum for a player with one Large aero port and one Large aqua port is:

- Aero (Exchange): +15 global + up to +15 trade deal = +30 (cap)
- Aqua (Fabrication): +15 global + up to +15 trade deal = +30 (cap)
- Local bonuses: effectively wasted once global cap is hit

This means the optimal strategy is: one Large port of each type plus a Premium trade deal with each neighbor. Beyond that, additional ports contribute only trade income (which is low, per Concern 2) and local bonuses. The system reaches saturation relatively quickly for experienced players.

### Trade Income vs. Trade Cost

| Configuration | Income/Phase | Deal Cost/Cycle | Net |
|--------------|-------------|----------------|-----|
| Small Aero, No Deal | 200 * 0.5 * 0.8 * 1.0 = 80 | 0 | +80 |
| Large Aero, Basic Deal | 2500 * 0.9 * 0.8 * 0.8 = 1,440 | 1,000 | +440 |
| Large Aero, Premium Deal | 2500 * 0.9 * 0.8 * 1.2 = 2,160 | 5,000 | -2,840 |
| Large Aqua, Premium Deal | 5000 * 0.9 * 0.6 * 1.2 = 3,240 | 5,000 | -1,760 |

Premium trade deals are income-negative in all configurations. They are purchased purely for the +15 demand bonus. This should either be rebalanced or explicitly communicated as "demand investment" rather than "trade deal."

### Contamination vs. Demand Bonus Radius

The contamination-free zone where local demand bonuses still apply:

| Port Type | Contamination Radius | Local Bonus Radius | Safe Bonus Zone |
|-----------|---------------------|-------------------|-----------------|
| Aero | 10 tiles | 20 tiles (habitation) | 10-20 tiles from port |
| Aqua | 8 tiles | 25 tiles (exchange) | 8-25 tiles from port |

This is well-designed. Players can place habitation 11+ tiles from an aero port and get the demand bonus without contamination. Aqua ports have an even larger safe zone. This rewards thoughtful city layout without making contamination feel punitive.

---

## Player Experience Flow

### Early Game (No Ports)
The player builds pathways to map edges, which creates ExternalConnectionComponents. NPC neighbors are generated. The player has basic trade capacity from pathway connections and modest migration. Ports are not yet available as a zoning option (or the player lacks resources).

### Mid Game (First Port)
The player zones their first port. An aero port requires flat terrain and a runway; an aqua port requires waterfront and docks. The port becomes operational, providing a global demand bonus to Exchange or Fabrication zones. The player sees increased growth in the boosted zone type. This is the "aha" moment where ports connect to the core growth loop.

### Late Game (Trade Network)
The player negotiates trade deals with NPC neighbors, invests in port upgrades, and potentially establishes inter-player trade with other human players. The relationship system rewards sustained trading. The player balances port placement against contamination, manages trade deal costs, and optimizes their port portfolio.

### Missing Experience: The "Why Should I Build a Port?" Moment
The current design assumes the player understands that ports boost zone demand. But the connection between "zone this area as aero_port" and "my exchange zones grow faster" is indirect. There is no tutorial ticket, no tooltip specification, and no visual indicator showing the demand bonus flowing from port to zones. The player could build a port and never realize it is helping.

---

## Multiplayer Dynamics

### Inter-Player Trade Is Symmetric and Fair
Both players in a trade agreement receive identical benefits (+3/+6/+10 demand, +5%/+10%/+15% income). This prevents exploitative asymmetric deals and encourages cooperation. The server-authoritative model prevents cheating.

### Trade Offer Spam Prevention
The duplicate-offer check in TradeOfferManager prevents a player from flooding another player with repeated offers. The 500-tick expiry cleans up ignored offers. This is adequate for MVP.

### Missing: Trade Disruption Mechanics
There is no way for a third player to interfere with a trade route between two other players. In multiplayer city builders, the ability to disrupt trade (by cutting pathway connections, for example) creates interesting geopolitical dynamics. This is not necessarily needed for MVP but would add strategic depth.

---

## Follow-Up Tickets

### F8-GD-001: Separate Demand Bonus Caps by Source
**Priority:** P0 | **Size:** M
Split MAX_TOTAL_DEMAND_BONUS into separate caps: PORT_DEMAND_BONUS_CAP (+20) and TRADE_DEAL_DEMAND_BONUS_CAP (+15). This prevents trade deals from consuming the entire bonus budget and making additional ports worthless. Add a demand bonus breakdown to IPortProvider so the UI can show per-source contributions. This addresses Concern 1.

### F8-GD-002: Rebalance Trade Income Rates
**Priority:** P1 | **Size:** S
Increase AERO_INCOME_PER_UNIT to 2.0 and AQUA_INCOME_PER_UNIT to 1.5 (approximately 2.5x current values). This makes Premium trade deals roughly income-neutral for Large ports, so the demand bonus is a genuine bonus rather than the only reason to sign the deal. Alternatively, reduce Premium deal cost to 2,000 credits/cycle. Playtest both approaches. This addresses Concern 2.

### F8-GD-003: Aero Port Unique Advantage -- Migration Boost
**Priority:** P1 | **Size:** M
Give aero ports a unique migration capacity bonus: each operational aero port adds +25 to max_immigration per cycle (stacking with diminishing returns). This differentiates aero ports from aqua ports beyond just which zone type they boost. Aqua ports are the capacity/income champions; aero ports are the population growth champions. This addresses Concern 3.

### F8-GD-004: Active Migration Control Mechanic
**Priority:** P1 | **Size:** M
Add a "Recruitment Drive" action purchasable through the port UI. Costs 2,000 credits and doubles immigration rate for 100 cycles. Requires at least one operational aero port. Limited to one active recruitment drive at a time. This gives the player a direct lever for population management and addresses Concern 4.

### F8-GD-005: Port Operational Diagnostic Query
**Priority:** P1 | **Size:** S
Add a PortDiagnostic struct returned by a new IPortProvider method: `get_port_diagnostic(EntityID port)`. Returns which of the four operational conditions are passing/failing (zone_valid, infrastructure_met, pathway_connected, capacity_nonzero) with a human-readable reason string for each failing condition. This addresses Concern 5.

### F8-GD-006: NPC Trade Deal Voluntary Cancellation
**Priority:** P1 | **Size:** S
Add cancel_trade_deal(TradeAgreementComponent&) function for NPC deals. Cancellation immediately ends the deal and applies a relationship penalty of -10 points. This allows players to exit bad deals without waiting for expiration, with a meaningful consequence. This addresses Concern 6.

### F8-GD-007: Define Default Contamination Intensity Values
**Priority:** P0 | **Size:** S
Add constants: AERO_NOISE_INTENSITY = 120, AQUA_INDUSTRIAL_INTENSITY = 100. Document these in the config table in port-system-spec.md. Without these values, the contamination system has no gameplay effect. This addresses Concern 8.

### F8-GD-008: Demand Bonus Visual Overlay Specification
**Priority:** P1 | **Size:** S
Define a "port demand influence" overlay that renders the global and local demand bonus coverage as a colored gradient on affected tiles. Green for positive bonus, with intensity proportional to bonus magnitude. Show the demand bonus cap usage in a corner HUD element. This addresses the "Why should I build a port?" experience gap.

### F8-GD-009: Document Local Bonus Diminishing Returns Policy
**Priority:** P2 | **Size:** S
Either apply a softer diminishing returns curve to local demand bonuses (100%/75%/50% for ports 1/2/3) or explicitly document in the spec that local bonuses intentionally do not diminish. Add a comment to DemandBonus.h explaining the design rationale. This addresses Concern 7.

### F8-GD-010: Port Placement Preview Overlay
**Priority:** P2 | **Size:** M
When the player is placing a port zone, show a preview overlay indicating: (a) the contamination radius in red, (b) the local demand bonus radius in green, and (c) the "safe bonus zone" (bonus radius minus contamination radius) highlighted. This helps players make informed placement decisions and leverages the well-designed radius differential.

### F8-GD-011: Trade Deal Cost-Benefit Tooltip
**Priority:** P2 | **Size:** S
When the player is considering a trade deal in the negotiation UI, show a projected cost-benefit analysis: estimated income change, demand bonus change, and net credits per cycle. This helps players understand the trade deal value proposition, especially given that Premium deals are currently income-negative.

### F8-GD-012: Port Achievement Milestones
**Priority:** P2 | **Size:** M
Define port-related milestones for the ProgressionSystem: "First Contact" (build first external connection), "Open for Business" (first operational port), "Trade Baron" (sign 3 active trade deals), "Allied Nations" (reach Allied relationship with any neighbor), "Port Authority" (have both an aero port and an aqua port operational simultaneously).

---

## Summary Table

| ID | Title | Priority | Concern |
|----|-------|----------|---------|
| F8-GD-001 | Separate Demand Bonus Caps | P0 | #1 - Confusing cap interaction |
| F8-GD-002 | Rebalance Trade Income Rates | P1 | #2 - Low revenue |
| F8-GD-003 | Aero Port Migration Boost | P1 | #3 - Aqua dominance |
| F8-GD-004 | Active Migration Control | P1 | #4 - No player agency |
| F8-GD-005 | Port Operational Diagnostic | P1 | #5 - No failure feedback |
| F8-GD-006 | NPC Deal Cancellation | P1 | #6 - No exit from bad deals |
| F8-GD-007 | Default Contamination Intensity | P0 | #8 - Unspecified values |
| F8-GD-008 | Demand Bonus Overlay | P1 | Player experience gap |
| F8-GD-009 | Local Bonus Diminishing Policy | P2 | #7 - Inconsistency |
| F8-GD-010 | Port Placement Preview | P2 | Player experience |
| F8-GD-011 | Trade Deal Cost-Benefit Tooltip | P2 | Player experience |
| F8-GD-012 | Port Milestones | P2 | Progression |

---

## Final Notes

The port system has a strong structural foundation and makes good macro-level design decisions: two distinct port types with terrain-driven placement, contamination tradeoffs, diminishing returns for diversification, and a relationship-gated NPC diplomacy layer. The code is clean, well-documented, and consistently uses canonical alien terminology throughout (aero_port, aqua_port, pathway, fabrication, exchange, habitation, harmony, disorder). No terminology violations were found.

The two P0 follow-up tickets (F8-GD-001 and F8-GD-007) should be addressed before implementation begins. The demand bonus cap interaction (Concern 1) is a structural issue that will be difficult to fix retroactively once the DemandSystem integration (E8-018) is implemented. The missing contamination intensity values (Concern 8) are a trivial fix but block the contamination system from having any effect.

The trade income balance (Concern 2) is the most significant gameplay concern. Premium trade deals costing more than they generate in income creates a counterintuitive player experience. Players expect "trade deal" to mean "I make money from trading." The current numbers mean trade deals are demand-bonus purchases with a trade-income facade. This can be fixed with number tuning, but the design intent should be clarified: are trade deals income generators or demand investments?

**Is this fun?** The bones are good. Port placement is a genuine spatial puzzle (terrain requirements, contamination vs. bonus radii, connectivity). The NPC relationship ladder gives long-term goals. The two port types create strategic variety. What is missing is feedback -- the player needs to see, feel, and understand the economic impact of their port decisions in real time. The overlay and tooltip tickets (F8-GD-008, F8-GD-010, F8-GD-011) will close that gap. With the balance adjustments and feedback mechanisms addressed, this system will deliver the "connected colony in a wider world" fantasy that makes city builders feel alive.
