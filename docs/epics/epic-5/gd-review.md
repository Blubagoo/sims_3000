# Epic 5: Energy Infrastructure - Game Designer Review

**Reviewer:** Game Designer Agent
**Date:** 2026-02-07
**Canon Version:** 0.17.0
**Documents Reviewed:** tickets.md, energy-balance-design.md, energy-balance.md, energy-overlay-ui-requirements.md, energy-system-technical.md, game-designer-analysis.md, EnergySystem.h, EnergyEnums.h, EnergyEvents.h, NexusTypeConfig.h, EnergyPriorities.h

---

## Summary Verdict

Epic 5 is a strong implementation of the first major tension system in ZergCity. The design does the most important thing right: it creates a system where the player's colony visually communicates its health state. When energy flows, the bioluminescent colony glows with life; when it fails, the colony goes dark. This is deeply satisfying and thematically perfect for an alien colony builder. The pool model (per-overseer, no physical flow simulation) is the correct choice -- it keeps the system understandable for players while remaining computationally cheap for multiplayer. The 4-tier priority rationing, asymptotic aging, and binary power model together create a system with real strategic depth.

However, there are areas where the design risks falling short of its potential. The binary power model (CCR-002) may prove too harsh during marginal deficits. The gap between Wind/Solar output and dirty nexus output is large enough that renewable energy feels like a vanity project rather than a real strategic path. And the system currently lacks enough "knobs" for the player to turn during a crisis -- the recovery options are essentially "build more nexuses" or "demolish things," with little in between. These are solvable problems, and the overall architecture is sound enough to support iteration. The epic earns a confident recommendation to proceed, with the specific tuning recommendations below.

---

## Player Experience Assessment

### Decision-Making Quality

The energy system creates three distinct layers of meaningful decisions:

**Layer 1: Infrastructure Investment (When and What to Build)**
The nexus type selection offers genuine trade-offs. A Carbon nexus at 5,000 credits and 100 output is the obvious early choice, but it comes with 200 contamination. A Nuclear nexus at 400 output and 0 contamination costs 50,000 credits -- ten times the price of a Carbon nexus for four times the output. This creates a real decision about colony philosophy: grow fast and dirty, or invest in clean infrastructure at the cost of slower expansion. This is good design.

**Layer 2: Spatial Planning (Where to Build)**
Conduit placement as coverage extension creates a genuine spatial puzzle. The coverage radius differences between nexus types (Wind at 4, Nuclear at 10) mean that the choice of nexus type affects how much conduit infrastructure is needed. Wind farms require dense conduit networks; a Nuclear nexus covers a large area with minimal conduit investment. This coupling between nexus type choice and conduit cost is an elegant interaction.

**Layer 3: Crisis Management (What to Do When Things Go Wrong)**
The priority-based rationing system creates a narrative during crises: medical and command structures stay lit while habitations go dark. This tells a story about survival priorities without requiring any player input. The player's role during a crisis is to decide how to recover, not to micromanage which buildings get power.

### Tension Arc

The emotional arc described in the planning analysis (prosperity -> capacity tension -> crisis -> recovery) is well-supported by the implementation. The four pool states (Healthy, Marginal, Deficit, Collapse) map cleanly to distinct player emotional states:

- **Healthy:** Quiet pride. The colony glows.
- **Marginal:** Mild anxiety. "I should probably build another nexus soon."
- **Deficit:** Active tension. The colony is visibly struggling. Rationing creates visible dark spots.
- **Collapse:** Crisis mode. The colony is dying. Immediate action required.

The transition from Marginal to Deficit is the most critical design moment. This is where the player either catches the problem or lets it spiral. The 10% buffer threshold for Marginal is narrow enough that a single nexus aging event or a burst of zone growth can push the player from Healthy to Marginal, which is exactly the right level of pressure.

### Binary Power Model Assessment (CCR-002)

The binary power model (fully powered or completely unpowered, no partial power) is the right call for MVP. It keeps the system simple, deterministic, and visually clear. A partially powered building would require additional visual states, a more complex rationing algorithm, and would muddy the player's ability to read the colony state at a glance.

That said, the binary model creates a specific risk during tight deficits: imagine a player with 99 units of generation and a single high-density fabrication structure requiring 60 units plus a medical nexus requiring 50 units. Total demand is 110. The rationing algorithm powers the medical nexus (50, priority 1) but cannot power the fabrication structure (60 remaining, only 49 available). Both would benefit from partial power, but the binary model means one gets everything and one gets nothing. This is acceptable for MVP but should be monitored in playtesting for frustration.

### Asymptotic Aging Assessment (CCR-006)

Asymptotic aging is an excellent design choice. It creates long-term planning pressure without hard failure states. A Carbon nexus that degrades to 60% of its original output is still producing 60 energy units -- not worthless, but noticeably less. The player must decide: supplement with a new nexus, or replace the old one entirely? This is exactly the kind of ongoing management decision that keeps a city builder engaging in the late game.

The aging floor values create an interesting secondary consideration: clean nexuses degrade less (Solar retains 85%) while dirty nexuses degrade more (Carbon drops to 60%). This subtly rewards clean energy investment in the long run. A Solar nexus producing 50 * 0.85 = 42.5 effective output over time is proportionally more efficient than a Carbon nexus at 100 * 0.60 = 60 when you consider the contamination difference. This is a well-designed emergent incentive.

The decay rate of 0.0001f means it takes thousands of ticks (at 20 ticks/second, roughly several real-time minutes) before aging becomes noticeable. This is appropriate -- aging should be a slow-burn concern, not an immediate pressure.

---

## Fun Analysis

### Moments of Satisfaction

1. **First Nexus Placement:** Watching unpowered structures come alive with bioluminescent glow for the first time is a defining moment. The contrast between dark structures and illuminated ones makes this feel magical. This is the "hook" moment.

2. **Conduit Network Expansion:** Extending coverage to a distant zone and watching it light up creates a "painting with light" feeling. The coverage preview (CCR-010) enhances this by letting the player see exactly which tiles will gain coverage before committing.

3. **Nuclear Nexus Investment:** At 50,000 credits, placing a Nuclear nexus is a major commitment. When it comes online and covers a huge area at 400 output with zero contamination, the player should feel a rush of accomplishment. This is a milestone moment.

4. **Crisis Recovery:** Watching the colony re-illuminate after a deficit is deeply satisfying. The priority-based recovery order (medical first, then safety, then economy, then habitation) creates a natural "wave of light" that tells a story.

5. **Clean Energy Transition:** Decommissioning the last dirty nexus and running entirely on Nuclear/Wind/Solar is an endgame pride moment. The colony becomes visually cleaner (no contamination) and the player has "solved" energy.

### Moments of Tension

1. **Marginal State Warning:** The narrow band between Healthy and Deficit creates a knife-edge feeling. One more zone designation could tip the balance. This is good tension.

2. **Grid Collapse:** The colony going dark is dramatic and memorable. The visual contrast with other players' colonies in multiplayer adds social stakes.

3. **Nexus Aging Notifications:** Every 10% efficiency drop is a reminder that infrastructure needs attention. This creates gentle but persistent pressure.

### What Could Be Better

1. **"Nothing Is Happening" Problem:** When energy is Healthy (which should be most of the time), there is no active engagement with the energy system. The planning analysis correctly identified this risk. The current design has no answer for it beyond visual ambient feedback (glowing structures). Consider: efficiency monitoring minigame, weather forecasts affecting Wind/Solar, or periodic maintenance events.

2. **Wind/Solar Feel Underwhelming:** Wind at 30 output (variable) and Solar at 50 output (variable) are dramatically weaker than dirty alternatives. A single Carbon nexus (100 output, 5,000 credits) outproduces three Wind nexuses (90 output, 9,000 credits) at less cost and with more reliability. The "farm" fantasy (many small clean generators) requires disproportionate investment for equivalent output. The variable output stub (0.75 average) further reduces effective output to 22.5 for Wind and 37.5 for Solar.

3. **Limited Crisis Tools:** During a deficit, the player's options are: build a new nexus, demolish buildings, or reorganize conduit coverage. There is no "emergency power" option, no ability to temporarily overclock a nexus at a cost, no way to selectively prioritize specific buildings. The system is reactive but lacks player-driven mitigation tools.

4. **Conduit Tedium Risk:** The planning analysis flagged this. With a coverage radius of 2-3 for basic conduits, extending coverage across a large map requires many individual placements. An auto-extend or "drag to place" tool is not mentioned in the tickets. This could become busywork.

### Depth vs. Complexity Verdict

The system has genuine **depth**: the interaction between nexus type choice, contamination, aging, coverage radius, and cost creates a strategic landscape where different overseers can pursue meaningfully different energy strategies. The complexity is proportional to the depth -- each system element creates interesting interactions rather than just adding parameters to manage. The 4-tier priority system is lean enough that players can understand it intuitively (hospitals stay on, homes go dark first) without needing to study a manual.

---

## Multiplayer Dynamics

### Per-Player Isolated Pools

The isolated pool model is the correct choice for the game's social/casual vibe. Each overseer is responsible for their own energy infrastructure, eliminating griefing through energy denial and ensuring that a less experienced player's mistakes do not cascade into other players' colonies. This aligns with Core Principle #4 (Endless Sandbox) -- players should be able to build at their own pace without being punished by others.

### Competitive Dynamics

The "schadenfreude" effect (CCR-008: all players see all energy states) is the most interesting multiplayer aspect. When a rival's colony goes dark, every other player can see it happening. This creates natural competitive pressure to maintain infrastructure without requiring any explicit competitive mechanics. The visual nature of energy state (glowing vs. dark) means this information is ambient, not something players need to actively check.

The decision to sync pool state for all players (16 bytes per player per change) is lightweight and effective. Players can gauge a rival's development stage by observing their energy infrastructure: a colony with visible Nuclear nexuses signals late-game economic strength.

### What Is Missing

1. **Cross-Boundary Contamination:** The planning analysis discussed contamination crossing overseer boundaries but the implementation tickets (5-016) explicitly enforce that coverage does NOT cross boundaries. Contamination spread across boundaries is deferred to Epic 10. When it arrives, it will add a valuable social tension layer (dirty nexuses near borders affecting neighbors). This is a good deferral -- energy should work before social conflict layers are added.

2. **Energy Trading:** The planning analysis mentions energy credit trading via a bank/market mechanism, but this is not part of Epic 5 and has no tickets. This is fine for MVP. Energy should be self-contained before trading is introduced.

3. **Observable Strategy Differentiation:** Currently, other players can see that your colony is glowing (or not), but the specific energy strategy (dirty vs. clean, concentrated vs. distributed) is not easily readable at a distance. Consider: different glow colors for different energy mixes, or contamination clouds visible to neighboring overseers.

### Snowball and Catch-Up Concerns

Energy does not directly create snowball dynamics because each player's pool is independent. However, a player who falls into Collapse loses economic output, making it harder to afford new nexuses, which prolongs the Collapse. This is a soft snowball effect. The mitigation is that a single Carbon nexus (5,000 credits) provides enough output to recover a small colony. The question is whether a player in Collapse can afford 5,000 credits. This needs playtesting.

---

## Alien Theme Consistency

### Terminology Audit

The implementation correctly uses canonical alien terminology throughout:

- **Nexus** (not plant/station): Used consistently in NexusType, NexusTypeConfig, all events, all documentation.
- **Energy** (not power/electricity): EnergySystem, EnergyComponent, energy_required, energy_received.
- **Overseer** (not player): Used in documentation and design docs. Code uses PlayerID which is acceptable for internal code.
- **Colony** (not city): Used in all player-facing documentation.
- **Grid Collapse** (not blackout): GridCollapseBeganEvent, GridCollapseEndedEvent.
- **Energy Conduit** (not power line): EnergyConduitComponent, ConduitPlacedEvent.
- **Energy Matrix** (not power grid): Referenced in documentation.
- **Contamination** (not pollution): contamination_output, ContaminationSourceData.
- **Beings** (not citizens): Used in design documents referencing population impact.
- **Harmony** (not happiness): Referenced in crisis impact descriptions.

**Issue Found:** The planning analysis document (`game-designer-analysis.md`) uses "Coal/Plasma Nexus" and "Oil/Petroleum Nexus" in section headings. These are human-equivalent terms. The canonical terms are "Carbon Nexus" and "Petrochemical Nexus" as defined in the tickets and implementation. The planning document was written before the terminology was finalized and should be updated for consistency, but this does not affect the implementation.

**Issue Found:** The `nexus_type_to_string()` function in `EnergyEnums.h` returns raw type names ("Carbon", "Petrochemical") without the "Nexus" suffix. For player-facing display, these should be rendered as "Carbon Nexus", "Petrochemical Nexus", etc. This is a UI concern for Epic 12 rather than an Epic 5 bug.

### Thematic Fit

The bioluminescent visual concept is perfectly aligned with the energy system. Energy literally powers the glow of the colony. This is the strongest thematic element in the entire project -- the alien aesthetic and the game mechanic are the same thing. When energy fails, the alien colony loses its bioluminescence and becomes a dark husk. When energy returns, light returns. This is simple, visceral, and thematically resonant.

The nexus types feel sufficiently alien. "Carbon Nexus" and "Gaseous Nexus" evoke industrial alien technology without being too Earth-centric. "Nuclear Nexus" is universal enough to work in any sci-fi context. Wind and Solar are physics-based and therefore theme-neutral.

**Suggestion:** The deferred nexus types (Hydro, Geothermal, Fusion, MicrowaveReceiver) have even stronger alien potential. "Microwave Receiver" in particular suggests orbital infrastructure that could be a powerful late-game fantasy. Fusion could be reimagined as "Void Nexus" or "Quantum Nexus" for stronger alien flavor.

---

## Balance Assessment

### Nexus Type Differentiation

The 6 MVP nexus types occupy distinct positions in the cost/output/contamination space:

| Type | Credits/Energy Unit | Contamination/Energy Unit | Coverage Radius | Aging Floor |
|------|-------------------|--------------------------|----------------|-------------|
| Carbon | 50 | 2.00 | 8 | 60% |
| Petrochemical | 53 | 0.80 | 8 | 65% |
| Gaseous | 83 | 0.33 | 8 | 70% |
| Nuclear | 125 | 0.00 | 10 | 75% |
| Wind | 100 | 0.00 | 4 | 80% |
| Solar | 120 | 0.00 | 6 | 85% |

**Key Observations:**

1. **Carbon is strictly dominated by Petrochemical** in credits-per-unit efficiency (50 vs. 53 is negligible) while having drastically worse contamination (2.0 vs. 0.8). The only advantage Carbon has is lower absolute cost (5,000 vs. 8,000), making it the "can barely afford anything" choice. This is acceptable for an early-game starter option, but consider making the cost difference more pronounced (Carbon at 3,000?) to make it a clearer "budget starter" choice.

2. **Wind and Solar are not competitive.** Wind at 100 credits/unit and Solar at 120 credits/unit are more expensive per unit of output than Carbon (50) or Petrochemical (53), while also being variable and having smaller coverage radii. Their only advantage is zero contamination and better aging floors. The contamination advantage is meaningful, but the cost-per-unit gap is too large. To make Wind/Solar feel like real alternatives, either increase their output (Wind to 50, Solar to 75) or decrease their cost (Wind to 2,000, Solar to 4,000).

3. **Nuclear is correctly positioned** as the expensive endgame option. At 125 credits/unit with zero contamination and the largest coverage radius, it rewards long-term investment. The 50,000 build cost is a significant gate.

4. **Gaseous is the overlooked gem.** At only 0.33 contamination per energy unit, it is by far the cleanest dirty option. Its cost-per-unit (83) is moderate. This is the natural mid-game choice, but there is no design language calling attention to it as a transition option. Consider: Gaseous should be presented as the "upgrade path" from Carbon/Petrochemical in tutorial or advisor messaging.

### Threshold Analysis

**Buffer Threshold (10%):**
For a typical mid-game colony generating 370 units, the buffer threshold is 37 units. This means the player enters Marginal state with a surplus below 37. A single low-density habitation zone (5 units) brings them 5 units closer to Marginal. This creates meaningful awareness that each new zone matters. The 10% value is appropriate.

**Collapse Threshold (50%):**
For a colony consuming 300 units, Collapse triggers at a deficit of 150 (generation below 150). This means the colony must lose more than half its generation before Collapse. This is a generous threshold -- it gives players a wide Deficit band to operate in before the catastrophic state. I would argue this is correct for a casual/social game. If the game were competitive, a lower threshold (30-40%) would be more punishing and create sharper pressure.

**Edge Case Concern:** The "zero generation, zero consumption" edge case resolves to Healthy. This is correct but could create confusion for a player who has not yet built any energy infrastructure. Their colony is "Healthy" with zero energy. This is technically accurate (no demand, no deficit) but could benefit from a distinct "No Energy Infrastructure" state or display for clarity.

### Priority System Assessment

The 4-tier priority system (Critical, Important, Normal, Low) provides enough granularity for the current structure types. With only 6 zone types and 6 service building types in play, 4 tiers are sufficient. The mapping is intuitive:

- Medical/Command as Critical (tier 1) -- survival needs
- Enforcer/Hazard Response as Important (tier 2) -- safety needs
- Exchange/Fabrication/Education/Recreation as Normal (tier 3) -- economic needs
- Habitation as Low (tier 4) -- beings can survive short outages

**One concern:** Habitation being the lowest priority means that during any deficit, the most visible part of the colony (where beings live) goes dark first. This is thematically appropriate (triage logic) but could feel counterintuitive to players who expect homes to be prioritized. The design doc correctly identifies this as creating the most visible signal of energy problems, which is the right design intent. However, the rationale should be communicated to the player through an advisor message or tooltip explaining that habitation structures are designed to tolerate temporary power loss.

### Grace Period Assessment (CCR-009)

100 ticks at 20 ticks/second = 5 seconds of real time before an unpowered structure transitions to a degraded state. This is long enough to prevent flickering during recovery but short enough that a persistent deficit has visible consequences. Five seconds is a reasonable default.

**Risk:** During oscillating deficits (supply barely below demand, nexus aging ticking, structures popping in and out of power), the 5-second grace period could create a confusing "strobe" effect where buildings cycle between powered and degraded states. Consider: a hysteresis mechanism where the grace period is longer for subsequent power losses (e.g., 100 ticks for first loss, 200 ticks for second loss within a cycle).

---

## Pacing Review

### Tick Order

EnergySystem at priority 10, before ZoneSystem (30) and BuildingSystem (40), is the correct ordering. Energy state must be settled before zones can check if their structures are powered, and before buildings can update their development state based on power availability. The downstream systems read energy state as a settled input rather than a concurrent variable. This eliminates race conditions and ensures deterministic behavior.

### Crisis Pacing

The current design creates crises through two mechanisms:

1. **Growth-driven deficit:** The player zones faster than they build nexuses. This is a gradual creep from Healthy through Marginal to Deficit, giving the player clear warning signs along the way. Pacing: slow, player-controllable.

2. **Aging-driven deficit:** Nexuses gradually lose output as they age. A colony that was Healthy six months ago may drift into Marginal as aging erodes its surplus. Pacing: very slow, background pressure.

Both mechanisms produce crises at an appropriate pace. The player always has warning and always has time to react. There is no "surprise collapse" mechanic (no sudden nexus failures in MVP), which is correct for a casual game.

**Concern:** There is no fast crisis mechanism. If all crises are slow and gradual, the player never experiences a dramatic emergency. The planning analysis mentions "core breach" for Nuclear nexuses as a future disaster (Epic 13), but in MVP, energy crises will always be gentle slopes rather than cliffs. This may make the energy system feel low-stakes once the player understands the patterns. Consider: a "nexus emergency shutdown" event (rare, not tied to neglect) that takes a nexus offline instantly, creating a sudden deficit and a memorable moment.

### Conduit Placement Pacing

Conduit placement is a deliberate, methodical activity. The player plans their network, places conduits one at a time, and watches coverage expand. With the coverage preview (CCR-010), each placement is informed and satisfying. This activity should feel like "drawing" the colony's circulatory system.

The risk is that large-scale conduit placement becomes tedious. For a 256x256 map with distant zones, the player may need to place dozens of conduits in a straight line. Without a drag-to-place tool, this becomes repetitive clicking. This is a UI concern for Epic 12 but should be planned for now.

---

## Recommendations

### High Priority (Should Address Before or During Implementation)

1. **Rebalance Wind/Solar output or cost.** Wind at 30 output for 3,000 credits and Solar at 50 output for 6,000 credits are not competitive with dirty alternatives. Recommend: Wind to 45 output or 2,000 cost; Solar to 70 output or 4,000 cost. The goal is making a Wind/Solar-only strategy viable (expensive but not absurd) rather than always requiring dirty nexuses.

2. **Add "No Energy Infrastructure" display state.** A colony with zero generation and zero consumption should show a distinct UI state ("No Energy Matrix") rather than "Healthy." This prevents player confusion during the tutorial phase.

3. **Plan for conduit drag-to-place tool.** Even if implementation is in Epic 12, the EnergySystem API should support batch conduit placement efficiently. Ticket 5-027 should note this as a future requirement.

4. **Add hysteresis to grace period.** The 100-tick grace period should include a cooldown -- once a building has been through a power loss cycle, the grace period for the next loss should be extended (e.g., 200 ticks). This prevents visual flickering during oscillating deficits.

### Medium Priority (Should Address During Balancing)

5. **Reduce Carbon build cost to 3,000-4,000.** Carbon should feel like the "desperate starter" option. At 5,000 it is cheap but not dramatically cheaper than Petrochemical (8,000). A wider gap emphasizes the "budget vs. quality" decision.

6. **Add an emergency power mechanic.** During Deficit, allow the overseer to temporarily overclock a nexus (e.g., +50% output for 200 ticks at the cost of accelerated aging and increased contamination). This gives players an active tool during crises rather than just "build more nexuses."

7. **Create an advisor message for habitation priority.** When habitations lose power, a tooltip or notification should explain: "Habitation structures are deprioritized during energy rationing. Beings can tolerate short outages. Critical services remain powered." This prevents the player from feeling like the system is broken.

8. **Consider weather variation for Wind/Solar.** The 0.75 average stub is fine for MVP, but the eventual variable output system should create cycles of abundance and scarcity that make Wind/Solar interesting to play around. Good variation: 0.5 to 1.0. Bad variation: 0.0 to 1.0 (too punishing).

### Low Priority (Future Enhancement)

9. **Add energy efficiency display to overlay.** When the energy overlay is active, show nexus efficiency percentages over each nexus structure. This gives the player ambient awareness of aging without requiring them to click on each nexus.

10. **Consider terrain-based Solar bonus.** Wind gets +20% on Ridges. Solar currently has no terrain bonus. Consider: Solar gets +15% on Substrate (flat open ground) or a penalty in Biolume Groves (shaded). This creates spatial differentiation between Wind and Solar farm placement.

11. **Rename deferred nexus types for stronger alien flavor.** "Fusion" could be "Void Nexus." "MicrowaveReceiver" could be "Orbital Nexus" or "Beacon Nexus." These names better serve the alien theme.

---

## Risks & Concerns

### Risk 1: Wind/Solar Never Adopted
**Severity:** Medium
**Description:** The current balance makes Wind/Solar strictly worse than dirty alternatives in cost-per-unit terms. Players who min-max will never build them. Players who try them will feel punished.
**Impact:** The clean-vs-dirty strategic axis -- one of the most interesting design elements -- becomes theoretical rather than practical. The late-game clean energy transition becomes "build Nuclear" rather than a diverse clean portfolio.
**Mitigation:** Rebalance Wind/Solar per recommendation #1. Add late-game efficiency bonuses for clean energy. Make contamination penalties from Epic 10 severe enough to incentivize transition.

### Risk 2: Conduit Placement Tedium
**Severity:** Medium
**Description:** Placing conduits one tile at a time across a large map is repetitive. Coverage radius of 2-3 for basic conduits means a 20-tile gap requires 7-10 individual placements.
**Impact:** Players may avoid extending coverage to distant areas, limiting strategic options. Infrastructure placement becomes a chore rather than a satisfying activity.
**Mitigation:** Plan for drag-to-place tool in Epic 12. Consider increasing default conduit coverage radius to 4-5. Consider an "auto-connect" feature that pathfinds between two points.

### Risk 3: "Nothing Is Happening" During Healthy State
**Severity:** Low-Medium
**Description:** When energy is working well (most of the time), the energy system provides no active engagement. The player has no reason to interact with it until something goes wrong.
**Impact:** Energy feels like a "set and forget" system rather than an ongoing management concern. Players may be surprised by crises they were not monitoring.
**Mitigation:** Add efficiency monitoring to the overlay. Consider periodic minor events (weather changes affecting Wind/Solar, nexus maintenance prompts). Add energy history graphs to the UI.

### Risk 4: Collapse Recovery Spiral
**Severity:** Low
**Description:** A player in Collapse loses economic output, making it harder to afford recovery infrastructure, potentially creating a death spiral. This is mitigated by the low cost of Carbon nexuses (5,000 credits) but could trap a player who has overextended.
**Impact:** In multiplayer, one player falling permanently behind is not fun for anyone.
**Mitigation:** The 5,000-credit Carbon nexus is cheap enough that recovery should always be possible unless the player has literally zero credits. Consider: emergency credit advance (loan) system in Epic 11. Consider: a guaranteed minimum income that prevents total bankruptcy.

### Risk 5: Multiplayer Energy Invisible
**Severity:** Low
**Description:** While all players can see all energy states (CCR-008), the pool state sync is numeric data. In practice, players will only notice a rival's energy crisis if they happen to be looking at that colony when it goes dark. There is no proactive notification that a rival is struggling.
**Impact:** The "schadenfreude" effect requires the player to be paying attention to rivals, which may not happen in focused gameplay.
**Mitigation:** Consider: a notification when a rival enters Deficit or Collapse ("Overseer [name]'s energy matrix is faltering"). This ensures the social dynamic is visible without requiring constant monitoring.

### Risk 6: Deterministic Rationing Feels Unfair
**Severity:** Low
**Description:** EntityID-based tie-breaking means that during rationing within the same priority tier, the building with the lower entity ID always gets power. This is deterministic (correct for multiplayer sync) but means the same buildings always go dark first -- typically the ones built later (higher entity IDs).
**Impact:** Players may notice that their newest buildings are always the first to lose power within a tier. This could feel arbitrary.
**Mitigation:** This is acceptable for MVP. If it becomes a player complaint, consider: round-robin tie-breaking that rotates which buildings within a tier get power during extended deficits.

---

*Review prepared by Game Designer Agent. All recommendations are suggestions for balancing iteration; the core architecture is sound and approved for implementation.*
