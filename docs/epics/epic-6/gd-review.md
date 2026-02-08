# Epic 6: Water Infrastructure (FluidSystem) - Game Designer Review

**Reviewer:** Game Designer Agent
**Date:** 2026-02-08
**Canon Version:** 0.18.0
**Documents Reviewed:** fluid-balance.md, fluid-threshold-balance.md, fluid-overlay-ui-requirements.md, no-rationing-design-note.md, FluidSystem.h, FluidEnums.h, FluidEvents.h, FluidComponent.h, FluidProducerComponent.h, FluidReservoirComponent.h, FluidExtractorConfig.h, FluidReservoirConfig.h, FluidRequirements.h, PerPlayerFluidPool.h

---

## Summary Verdict

Epic 6 delivers a fluid infrastructure system that meaningfully differentiates itself from the energy system while sharing enough structural DNA to keep the player's mental model coherent. The two most important design decisions -- all-or-nothing distribution (CCR-002) and reservoir buffering as an explicit physical mechanic -- work together to create a system that feels fundamentally different from energy despite using the same pool-based architecture. Energy is about triage and graceful degradation. Fluid is about abundance or catastrophe. This contrast is excellent and will make managing both systems simultaneously feel like managing two genuinely different problems rather than managing the same problem twice with a different color palette.

The water proximity curve for extractor placement is the standout design element. It gives fluid a spatial dimension that energy lacks: where you build matters as much as what you build. The tiered step function is player-readable and creates clear placement incentives without requiring optimization math. The extractor-reservoir pairing is well-balanced: extractors produce, reservoirs buffer, and neither can substitute for the other. The asymmetric fill/drain rates (50/100) are an elegant mechanism that prevents reservoir-hoarding strategies while making reservoirs feel valuable during genuine emergencies.

However, the system has a significant structural weakness: it is too uniform. There is exactly one extractor type at one price with one output. The energy system offers six nexus types with different cost/output/contamination profiles, creating a rich strategic landscape. The fluid system has a single extractor and a single reservoir. This means that after the player learns "build extractors near water and reservoirs for buffer," there is no further strategic depth to discover. The proximity curve adds spatial decision-making, but the infrastructure itself lacks variety. Additionally, the all-or-nothing distribution, while thematically justified, creates a binary outcome that may feel frustrating to players who lose their entire fluid network over a small deficit. The reservoir buffer mitigates this, but the cliff-edge from "everyone has water" to "nobody has water" is steep.

The epic earns a recommendation to proceed. The architecture is sound, the cross-system interaction with energy is well-designed, and the spatial gameplay from water proximity is genuinely interesting. The uniformity problem is addressable through future extractor types without requiring architectural changes. The all-or-nothing risk is mitigated by reservoirs and can be tuned through capacity and rate adjustments if playtesting reveals frustration.

---

## Player Experience Assessment

### Decision-Making Quality

The fluid system creates two primary layers of meaningful decisions, with a third emerging from cross-system interaction:

**Layer 1: Spatial Planning (Where to Build Extractors)**
This is the strongest layer and the one that most differentiates fluid from energy. The water proximity curve creates a genuine spatial puzzle:
- At distance 0: 100% efficiency (100 fluid/tick from a single extractor)
- At distance 3-4: 70% efficiency (70 fluid/tick)
- At distance 7-8: 30% efficiency (30 fluid/tick)

The player must decide: place three extractors at 30% efficiency near existing infrastructure, or extend conduits to place one extractor at 100% efficiency near water? Three distant extractors cost 9,000 credits for 90 fluid/tick. One optimal extractor costs 3,000 credits for 100 fluid/tick plus conduit costs. The math is clear, but the tradeoff with conduit placement effort and coverage reach makes this a genuine decision. This is good design.

**Layer 2: Buffer Strategy (How Many Reservoirs to Build)**
The asymmetric fill/drain rates create a decision about insurance investment. A reservoir costs 2,000 credits and provides 10 ticks of emergency buffer at full consumption (1,000 capacity / 100 drain rate). Two reservoirs double the buffer to 20 ticks. The question for the player is: how much am I willing to spend on insurance versus production? This is a secondary decision that adds texture without dominating gameplay.

**Layer 3: Cross-System Energy Dependency**
Extractors require 20 energy units to operate. A Carbon nexus at 200 output can power 10 extractors. This means the player cannot expand fluid infrastructure without first establishing energy infrastructure. This creates a natural build order (energy first, then fluid) and a cascading failure risk (energy deficit kills extractors, which causes fluid deficit). The dependency is well-calibrated: 20 energy per extractor is significant enough to require planning but not so expensive that fluid infrastructure becomes parasitic on the energy budget.

### Tension Arc

The fluid system creates a different emotional arc than energy, driven by the reservoir mechanic:

- **Healthy:** Quiet confidence. Reservoirs are full, extractors are running. Fluid "just works." The visual cue is blue-glowing structures with full reservoir gauges.
- **Marginal:** Mild concern. Surplus is thin. The player notices the reservoir gauges are not refilling as quickly. Each new habitation zone noticeably tightens the margin.
- **Deficit:** Active urgency. Reservoirs are visibly draining. The player can see the countdown happening in real-time as gauge levels drop. This is the strongest moment in the fluid system -- the visual drama of watching reservoirs empty creates more visceral tension than the energy system's abstract "surplus below threshold" warning.
- **Collapse:** Catastrophe. All structures simultaneously lose fluid. The entire colony goes gray/desaturated. Unlike energy's graduated rationing (where critical buildings stay lit), fluid collapse is absolute. Everyone loses water at once. This is dramatic and memorable.

The key difference from energy's arc is the **reservoir drain as visible countdown**. In the energy system, the transition from Deficit to Collapse is governed by an invisible percentage threshold. In the fluid system, the player can literally watch the blue bars on their reservoirs dropping toward empty. This is superior player feedback because it converts an abstract number into a visual, spatial, time-pressure experience. The player does not need to check a status panel; the crisis is visible on the map.

### All-or-Nothing Distribution Assessment (CCR-002)

The all-or-nothing decision is bold and thematically coherent. The design rationale -- "you can't say these beings get water and these don't" -- is the right framing for a habitability resource. Water is not electricity; you cannot meaningfully "ration" it in a way that feels fair. A hospital with water and homes without feels logical for power. A hospital with water and homes without feels cruel.

The gameplay consequence is a system with higher stakes and lower complexity than energy:
- **Higher stakes:** When fluid fails, it fails everywhere simultaneously. There is no "some buildings are still working" partial failure. This makes fluid crises feel more urgent than energy crises.
- **Lower complexity:** No priority tiers, no rationing logic, no "which buildings go dark first" calculation. The player either has enough water or does not. This is simpler to understand but leaves fewer levers to pull during a crisis.

**Risk:** The cliff-edge between "everything works" and "nothing works" could feel unfair. Consider a scenario: a player with 95 fluid generation and 100 fluid consumption. Their surplus is -5. Without reservoirs, all 100 units of consumption are cut -- every building loses water simultaneously over a deficit of 5 fluid units. In the energy system, the same scenario would result in only the lowest-priority buildings losing power, preserving most services. The player feels punished disproportionately to the size of the deficit.

**Mitigation:** The reservoir system exists precisely to soften this cliff. With even a single reservoir (1,000 capacity, draining at 100/tick), the player gets 10 ticks of buffer before collapse -- enough time to notice and react. The design is betting that players will always build at least one reservoir. If they do not, the cliff-edge is fully exposed. This should be addressed through tutorial/advisor messaging: "Build a reservoir to protect against fluid disruptions" should be a first-time advisor prompt.

### Reservoir Buffering Assessment

The asymmetric fill/drain rates (50 fill, 100 drain) are the most elegant mechanical design in Epic 6. The 2:1 ratio creates three important properties:

1. **Reservoirs cannot substitute for extractors.** A reservoir drains in 10 ticks but takes 20 ticks to refill. A player who relies on reservoirs instead of extractors will see them empty twice as fast as they fill. After 2-3 drain/refill cycles, the player will have internalized that reservoirs are insurance, not production.

2. **Over-production is visually rewarded.** A player with excess extraction sees reservoirs sitting at full capacity -- visible proof that their infrastructure is healthy. The blue gauge bars at 100% are a small pride moment every time the player glances at them.

3. **Recovery requires patience.** After a deficit event, reservoirs refill slowly (50/tick). The player must wait for the "all clear" even after building enough extractors to cover demand. This post-crisis recovery period creates a lingering tension: "Am I safe yet?" The answer is not just "do I have enough extractors?" but "have my reservoirs had time to refill?" This is a subtle but excellent design element.

**One concern:** The 50 fill rate means a fully drained reservoir takes 20 ticks to refill. At 20 ticks/second, that is 1 second of real time. This is quite fast -- the "patience" mentioned above is barely perceptible at normal game speed. If the game runs at a slower tick rate or if the player is in a time-acceleration mode, the refill time may need to be longer to create real tension. Consider: if the 20-tick refill feels too fast during playtesting, reduce the fill rate to 25 (40 ticks = 2 seconds) or make fill rate dependent on surplus magnitude (faster refill with more excess extraction).

---

## Fun Analysis

### Moments of Satisfaction

1. **First Extractor Near Water:** Placing an extractor at distance 0 from a water source and seeing 100% efficiency displayed creates an immediate "I did this right" feeling. The placement preview showing the green efficiency zone around water sources makes this satisfying before the player even commits.

2. **Coverage Network Completion:** Extending fluid coverage to reach a distant residential district and watching all those buildings turn from un-covered to hydrated (blue glow) is the "painting with water" equivalent of energy's "painting with light." The cool blue palette provides visual contrast with energy's warm amber.

3. **Reservoir Gauge at 100%:** Full reservoir bars are a quiet pride indicator. They tell the player "you have built enough -- your colony is resilient." This is an understated but important satisfaction signal in a management game.

4. **Surviving a Crisis via Reservoir Buffer:** A player who builds reservoirs and then loses an extractor will watch the reservoirs drain while they frantically build a replacement. If they succeed before collapse, the "save" feels earned. The visible drain countdown creates a mini-narrative: race against the dropping gauge.

5. **Cross-System Recovery:** After an energy deficit cuts off extractors, restoring energy and watching both systems recover simultaneously is a cascade of satisfaction: energy comes back, extractors power up, fluid starts flowing, reservoirs refill, buildings re-hydrate. This multi-system recovery sequence is one of the strongest potential moments in the game.

### Moments of Tension

1. **Reservoir Drain Countdown:** Watching reservoir gauges drop during a deficit is the most visceral tension moment. The transition from orange (25-49%) to red (0-24%) creates escalating visual urgency.

2. **Cascading Failure:** An energy deficit that shuts down extractors creates a two-system crisis. The player must fix energy first (because extractors need power), then wait for fluid to recover. The dependency chain creates genuine strategic pressure: "Which problem do I solve first?"

3. **Expansion Overreach:** Building a new residential district without pre-building extractors creates a "did I just overcommit?" moment. The 10% buffer threshold means even modest expansion can push the pool from Healthy to Marginal.

### What Could Be Better

1. **"One Extractor, One Choice" Problem:** The single extractor type means every placement decision is purely spatial (where to put it, how close to water). There is no "which type to build" decision. Compare with the energy system's six nexus types, each with distinct cost/output/contamination tradeoffs. The fluid system needs at least 2-3 extractor types to create the same decision richness. Suggestions:
   - **Surface Extractor:** Cheap (2,000 credits), low output (60 fluid/tick), must be adjacent to water (distance 0-2). The budget option.
   - **Deep Extractor:** Standard (3,000 credits), current stats (100 fluid/tick), distance 0-8. The default.
   - **Artesian Extractor:** Expensive (6,000 credits), high output (180 fluid/tick), no distance constraint (taps deep aquifer). The premium option with no spatial limitation.

2. **Reservoir Homogeneity:** Similarly, one reservoir type limits buffer strategy decisions. Consider:
   - **Cistern:** Cheap (1,000 credits), small capacity (500 fluid), fast fill (40/tick). Emergency patch.
   - **Reservoir:** Standard (2,000 credits), current stats. The default.
   - **Treatment Facility:** Expensive (5,000 credits), large capacity (3,000 fluid), but requires energy (20 units). The premium buffer with a dependency.

3. **No Contamination Equivalent:** Energy has contamination as a secondary cost for dirty nexuses. Fluid extractors have no secondary consequence -- no "polluted water" or "resource depletion" mechanic. This means fluid infrastructure has no long-term cost beyond credits. Adding a "water table depletion" mechanic for over-extraction (similar to energy aging) would create a long-term management dimension. This can be deferred but should be planned.

4. **No Weather/Seasonal Variation:** Energy has variable output for Wind/Solar. Fluid has no variability at all -- extractors produce a fixed amount based on distance. Consider: drought events that reduce water availability, forcing players to rely more heavily on reservoirs. This would make the reservoir system even more strategically important.

5. **Limited Multiplayer Differentiation:** The energy system has visible strategy differentiation (dirty nexus clouds, different glow colors). Fluid infrastructure looks the same regardless of strategy because there is only one strategy: "build extractors near water." When multiplayer becomes relevant, rival colonies' fluid strategies will be indistinguishable.

### Depth vs. Complexity Verdict

The fluid system has **moderate depth** with **low complexity**. The water proximity curve provides the primary strategic dimension, and reservoir management provides a secondary buffer dimension. Cross-system energy dependency adds a tertiary planning dimension. These three layers interact in meaningful ways (proximity constrains placement, energy constrains operation, reservoirs constrain crisis tolerance). However, the lack of infrastructure type variety means the strategic landscape is narrower than the energy system's. The fluid system is approachable -- a new player can understand "build extractors near water, build reservoirs for safety" within minutes -- but it may not sustain long-term engagement on its own. The energy-fluid interaction is the primary source of ongoing complexity.

---

## Multiplayer Dynamics

### Per-Player Isolated Pools

The isolated pool model carries over correctly from energy. Each overseer manages their own fluid infrastructure independently. This is consistent with the game's social/casual design philosophy and prevents fluid-based griefing.

### Cascading Failure Visibility

The most interesting multiplayer moment for fluid is the cascading failure: when a rival's energy goes down, observers can watch their fluid system follow shortly after. The sequence -- energy collapse warning, then fluid deficit warning, then fluid collapse warning -- tells a dramatic story visible to all players. The warning priority system (energy collapse > fluid collapse > energy deficit > fluid deficit) correctly orders these banners for maximum narrative clarity.

### What Is Missing

1. **Water Source Competition:** The current design treats water sources as infinite and non-competitive. Multiple players can extract from the same water body without interference. In future epics, shared water sources that deplete under heavy extraction would create a rich multiplayer tension: "My rival's five extractors on the lake are drawing down the supply for my two extractors." This would be the fluid equivalent of energy's cross-boundary contamination.

2. **Fluid Trading:** Analogous to energy trading (deferred from Epic 5), fluid trading is absent and appropriately deferred. A future "aqueduct" or "fluid pipeline" system that crosses overseer boundaries would add a cooperative dimension.

3. **Observable Fluid Strategy:** As noted above, all fluid infrastructure looks identical because there is only one extractor type. Rivals cannot read each other's fluid strategy from visual inspection alone. When extractor types are added, different types should have visually distinct appearances.

---

## Alien Theme Consistency

### Terminology Audit

The implementation correctly uses canonical terminology:

- **Fluid** (not water): Used consistently across all files -- FluidSystem, FluidComponent, fluid_required, fluid_received. Excellent discipline.
- **Extractor** (not pump): FluidExtractorConfig, ExtractorPlacedEvent, register_extractor. Consistent throughout.
- **Reservoir** (not tank/tower): FluidReservoirComponent, ReservoirPlacedEvent. Consistent.
- **Conduit** (not pipe): FluidConduitComponent, FluidConduitPlacedEvent. Mirrors energy conduit terminology.
- **Overseer** (not player): Referenced in documentation. Code uses owner/player_id internally, which is acceptable.
- **Colony** (not city): Used in documentation.

**No Issues Found.** The terminology is clean and consistent. The choice to call this "fluid" rather than "water" leaves room for alien worlds where the resource might not literally be H2O. This is good forward-thinking thematic design.

### Thematic Fit

The fluid system's visual language (blue/cyan palette) provides clean contrast with the energy system (amber/warm). The "hydrated" building glow (blue pulse) versus "dehydrated" desaturation creates a visual metaphor that works for both Earth-like water and alien fluids. The reservoir gauge bars provide an intuitive "tank level" readout that transcends cultural context.

The all-or-nothing distribution has strong thematic resonance for an alien colony: the colony's circulatory system either works or it does not. There is no partial circulation. This feels appropriately biological -- more like a living system than a utility grid.

**Suggestion:** When extractor types are added, consider names that reinforce the alien theme:
- "Surface Extractor" -> "Membrane Extractor" (biological extraction from surface)
- "Deep Extractor" -> "Root Extractor" (tapping deep into the substrate)
- "Artesian Extractor" -> "Aquifer Tap" or "Substrate Bore" (deep geological access)

---

## Balance Assessment

### Extractor Economics

At 3,000 credits and 100 fluid/tick base output, a single extractor supports approximately 20 low-density habitations (5 fluid each) or 10 high-density habitations (20 fluid each, per the note that FLUID_REQ_HABITATION_HIGH should logically be the high-density value, though the code shows 20). The credits-per-fluid-unit ratio is 30 credits per fluid unit of output (3,000 / 100). This is reasonable and comparable to the energy system's Carbon nexus (5,000 / 200 = 25 credits per energy unit, adjusting for the fact that a single Carbon nexus was noted in Epic 5 docs as 100 output, giving 50 credits/unit).

**Concern:** The fluid requirement values mirror energy values exactly (per CCR-007). This means a low-density habitation needs 5 fluid AND 5 energy. At first glance, this is elegant -- the player learns one set of numbers. But it creates a subtle balance issue: fluid extractors produce 100/tick while a Carbon nexus produces 200/tick (based on Epic 5 balance tables). One nexus covers twice as many consumers as one extractor. This means the player needs roughly twice as many extractors as nexuses for the same population. Combined with the water proximity constraint (extractors must be near water), fluid infrastructure requires more planning and more physical structures than energy. This is intentional and correct -- fluid should feel like a more hands-on system -- but it should be validated through playtesting to ensure it does not cross the line from "engaging" to "tedious."

### Water Proximity Curve Assessment

The tiered step function is well-designed for player readability:

| Distance | Efficiency | Effective Output | Credits/Effective Unit |
|----------|-----------|-----------------|----------------------|
| 0 | 100% | 100 | 30 |
| 1-2 | 90% | 90 | 33 |
| 3-4 | 70% | 70 | 43 |
| 5-6 | 50% | 50 | 60 |
| 7-8 | 30% | 30 | 100 |

The curve creates a 3.3x cost difference between optimal (distance 0) and worst-case (distance 7-8) placement. This is steep enough to strongly incentivize water-adjacent placement without absolutely forbidding distance placement. A player who places an extractor at distance 7-8 is paying 100 credits per effective fluid unit -- twice the cost per unit of a Carbon nexus. This makes distant extractors genuinely bad choices, which is the correct signal.

The sharp drops at tier boundaries (90% to 70% at distance 3, 70% to 50% at distance 5) create clear "green/yellow/red" zones that the overlay can communicate effectively. The player does not need to do math; they just need to see whether they are in the green, yellow, or red zone.

**One issue:** The maximum placement distance (8 tiles) allows placement at 30% efficiency. This is a trap choice -- the player can place an extractor there, but it produces so little fluid that it is nearly worthless. Consider whether the UI should display a stronger warning at distance 7-8 (e.g., "Minimal efficiency -- consider relocating") or whether placement should be soft-blocked beyond distance 5 (the "maximum operational distance") with an override option.

### Reservoir Balance Assessment

At 2,000 credits and 1,000 capacity, a reservoir provides 10 ticks of full-consumption buffer. For a small colony consuming 50 fluid/tick, this extends to 20 ticks. The cost is cheap enough (2/3 of an extractor) that players should be willing to build at least one. The lack of energy requirement makes reservoir placement flexible -- they can be placed anywhere without concern for energy coverage.

**The cost ratio is well-calibrated:** An extractor at 3,000 produces 100 fluid/tick indefinitely. A reservoir at 2,000 buffers 1,000 fluid total. After 10 ticks of full drain, the reservoir is empty and produces nothing. This makes the extractor clearly superior for sustained production and the reservoir clearly a buffer-only investment. No player will confuse the two roles.

**Coverage radius concern:** The reservoir's 6-tile coverage radius is smaller than the extractor's 8-tile radius. This means a reservoir placed to fill a coverage gap may not fully cover the same area. A player might need to place both a reservoir AND a conduit to achieve the same coverage that a single extractor provides. This creates a non-obvious cost: the reservoir's true cost is 2,000 credits plus conduit costs to match coverage. This is a potential friction point but not a design flaw -- it correctly positions reservoirs as supplements, not replacements.

### Fluid Requirements Analysis

The consumption values scale predictably:

| Consumer Type | Low | High | Ratio |
|--------------|-----|------|-------|
| Habitation | 5 | 20 | 4x |
| Exchange | 10 | 40 | 4x |
| Fabrication | 15 | 60 | 4x |
| Service (S/M/L) | 20 | 35/50 | - |

The 4x ratio between low and high density is consistent across all zone types, which is learnable. The absolute values mean:
- One extractor at 100% efficiency serves: 20 low-hab, 10 low-exchange, or 6.7 low-fabrication zones
- A mixed neighborhood of 10 low-hab + 3 low-exchange + 2 low-fabrication = 50 + 30 + 30 = 110 fluid/tick = just over one extractor

This means a "starter neighborhood" needs approximately one extractor, which is an intuitive ratio. A high-density neighborhood of the same composition = 200 + 120 + 120 = 440 fluid/tick = nearly five extractors. The density upgrade creating a 4x consumption increase per building creates a strong incentive to expand extraction before upgrading density. This is good pacing -- it gates density upgrades behind infrastructure investment.

### Threshold Analysis

**Buffer Threshold (10%):**
Identical to the energy system, maintaining player mental model consistency. For a colony generating 100 fluid/tick with 1,000 reservoir stored (available = 1,100), the buffer threshold is 110. This means Marginal triggers when surplus drops below 110. Given that a single low-density habitation costs 5 fluid, the player can add about 22 habitations before transitioning from Healthy to Marginal. This feels right -- a small expansion cushion before warning.

**However**, the fluid system calculates available as `total_generated + total_reservoir_stored`. For a colony with 100 generated and 1,000 stored, available is 1,100 and the buffer threshold is 110. This means a colony with full reservoirs has a much more generous Healthy threshold than one with empty reservoirs. This is intentional (full reservoirs = healthier system) but creates a subtle behavior: after a deficit event drains reservoirs, the system may remain in Marginal for a long time while reservoirs slowly refill (because available recovers slowly, making the buffer threshold creep back up). This is actually good gameplay -- it creates a lingering "recovery" period where the player knows things are not yet fully stable.

**Collapse condition (reservoirs empty):**
The binary collapse condition (reservoirs empty = collapse, any reservoir stored = deficit) is simpler than energy's percentage-based threshold. This is correct for a system with physical storage -- the mechanic maps to an intuitive physical reality. A player who sees an empty reservoir gauge understands they are in collapse. No explanation needed.

---

## State Machine Assessment

The four-state machine (Healthy/Marginal/Deficit/Collapse) correctly mirrors the energy system's states while using fluid-specific transition conditions:

**State transitions are well-defined:**
- Healthy -> Marginal: surplus drops below 10% of available. Clear trigger.
- Marginal -> Deficit: surplus goes negative, but reservoirs still have fluid. The "we're running on reserves" state.
- Deficit -> Collapse: reservoirs empty out completely. The "there is no water" state.
- Collapse -> Deficit: reservoirs receive any fluid (e.g., player builds a new extractor). Recovery begins.
- Deficit -> Marginal/Healthy: surplus recovers above zero. Crisis resolved.

The state machine is simple, deterministic, and maps to intuitive physical conditions. No concerns here.

**Edge case handling:**
- Zero generation, zero consumption = Healthy. Correct -- no infrastructure means no problem.
- Generation > 0 but no consumers = Healthy. Correct -- the player is building proactively.
- Reservoir full but generation = 0 (all extractors lost) = available is reservoir stored only, surplus may remain positive temporarily, then transitions to Deficit as reservoir drains. This creates the intended "draining countdown" dynamic.

---

## Player Feedback Assessment

### Event Coverage

The event system provides comprehensive coverage for UI feedback:

| Event | Trigger | UI Response |
|-------|---------|-------------|
| FluidStateChangedEvent | Entity gains/loses fluid | Blue glow on/off, tooltip update |
| FluidDeficitBeganEvent | Pool enters Deficit | Orange warning banner |
| FluidDeficitEndedEvent | Pool exits Deficit | Clear orange banner |
| FluidCollapseBeganEvent | Pool enters Collapse | Red alert banner + alarm |
| FluidCollapseEndedEvent | Pool exits Collapse | Clear red banner |
| ReservoirLevelChangedEvent | Reservoir fill changes | Gauge bar update |
| ExtractorPlacedEvent | Extractor built | Visual appears, overlay update |
| ReservoirPlacedEvent | Reservoir built | Visual appears, gauge appears |
| FluidConduitPlacedEvent | Conduit built | Conduit visual, connectivity update |

This is thorough. Every significant state change has a corresponding event for the UI to consume. The ReservoirLevelChangedEvent in particular enables the "draining countdown" visual that makes fluid crises so dramatic.

### Missing Events/Feedback

1. **No "Marginal" transition event.** The system emits events for Deficit and Collapse transitions but not for the Healthy-to-Marginal transition. Marginal is the early warning state -- arguably the most important moment for player feedback. The player needs to know "you are running thin on fluid" before things get bad. **Recommendation:** Add `FluidMarginalBeganEvent` and `FluidMarginalEndedEvent` to match the deficit/collapse event pattern.

2. **No extractor efficiency change event.** When an extractor's effective output changes (e.g., due to losing power, or hypothetical future water table depletion), there is no event. The UI must poll FluidProducerComponent each frame to detect changes. For MVP this is fine (extractors either produce or do not, based on power), but as the system becomes more complex, an `ExtractorOutputChangedEvent` would be valuable.

3. **No aggregate surplus change event.** The pool status panel shows surplus, but there is no event when surplus crosses notable thresholds (e.g., drops below 50% of generation). The UI must poll the pool each frame. This is acceptable for MVP.

4. **No "first extractor placed" tutorial trigger.** The first time a player places an extractor should trigger an advisor message about reservoirs. The current event system supports this (ExtractorPlacedEvent can be counted), but the advisor logic does not appear to be implemented yet. This is an Epic 12 concern.

---

## Energy Dependency Assessment

The extractor's 20-energy-unit requirement at priority 2 (Important) creates a well-calibrated cross-system interaction:

**Priority 2 (Important) is the correct choice.** During energy rationing, extractors are prioritized above zone buildings (priority 3/Normal) but below life support (priority 1/Critical). This means:
- During mild energy deficits, zone buildings lose power before extractors do. Fluid infrastructure stays up while non-critical buildings go dark. This is the correct triage order.
- During severe energy deficits, extractors lose power too, triggering a cascading fluid crisis. This creates the dramatic two-system failure scenario.

**The 20-unit cost is proportional.** A Carbon nexus at 200 output powers 10 extractors. Each extractor produces 100 fluid. So one Carbon nexus indirectly supports 1,000 fluid/tick of extraction -- enough for a substantial city. The energy cost is significant enough to require planning (the player cannot ignore it) but not so expensive that fluid infrastructure becomes a major drain on energy capacity.

**Cascading failure dynamics are the most interesting cross-system interaction:**
1. Energy generation drops (nexus aging, nexus destroyed, demand growth)
2. Energy rationing kicks in, eventually reaching priority 2
3. Extractors lose power, fluid generation drops to zero
4. Fluid pool enters Deficit, reservoirs start draining
5. Reservoirs empty, fluid pool enters Collapse
6. All buildings lose fluid -- habitation, fabrication, services

This cascade creates a story: the player's colony is dying because they neglected energy maintenance. The fix is not "build more extractors" (as a player might assume from the fluid deficit warning), it is "fix the energy system first." This diagnostic challenge -- understanding root causes across systems -- is excellent strategic depth for experienced players. For new players, an advisor message should connect the dots: "Extractors require energy to operate. Restore energy to resume fluid extraction."

---

## Difficulty Curve Assessment

### New Player Approachability

The fluid system is approachable for new players:
- **Single extractor type** means no "which one do I build?" paralysis
- **Single reservoir type** means "build this for backup" is the only buffer decision
- **Water proximity** is visually communicated through the overlay (green/yellow/red zones)
- **All-or-nothing** distribution means no rationing rules to learn

A new player can grasp the system in three steps: (1) build extractor near water, (2) build reservoir for backup, (3) extend conduits to reach your buildings. This is simpler than the energy system's six nexus types with contamination considerations.

### Experienced Player Depth

This is where the system falls short. After learning the three-step basics, experienced players have limited strategic exploration available:
- **No extractor type choice** means no cost/output/contamination optimization
- **No aging** means no maintenance management
- **No variability** means no adaptation to changing conditions
- **No contamination** means no long-term consequence management

The water proximity curve adds spatial depth, but once a player has learned to place extractors near water, the optimization is complete. Compare with energy, where an experienced player is continuously balancing six nexus types, managing aging, planning contamination reduction, and transitioning from dirty to clean energy.

**This is the primary design gap in Epic 6.** The system needs more "knobs" for experienced players to turn. Extractor types, aging/depletion, and variability events are the three most impactful additions, and all can be layered on top of the existing architecture.

---

## Recommendations

### High Priority (Should Address Before or During Implementation)

1. **Add Marginal transition events.** `FluidMarginalBeganEvent` and `FluidMarginalEndedEvent` should be added to enable the UI to warn players when fluid surplus is thinning. The Marginal state is the early warning window; without a transition event, the UI must poll the pool state every frame to detect it. The energy system benefits from its graduated rationing being visible; the fluid system needs the Marginal warning to compensate for the absence of visible rationing.

2. **Add advisor prompt for "build a reservoir."** The first time a player places an extractor, an advisor should prompt: "Build a reservoir near your extractor to protect against fluid supply disruptions." This mitigates the all-or-nothing cliff-edge risk by ensuring players learn about reservoirs early. Without this, a player who skips reservoirs will experience the full binary failure with no buffer.

3. **Plan extractor type variants for future epic.** The single-type extractor creates a uniformity problem that limits strategic depth. At minimum, plan for 2-3 extractor types (surface/deep/artesian or equivalent alien names) with distinct cost/output/proximity profiles. The architecture supports this -- `FluidExtractorConfig` already has all necessary fields, and adding a type enum to `FluidProducerComponent.producer_type` would be straightforward.

4. **Add "minimal efficiency" warning at distance 7-8.** When a player is previewing extractor placement at distance 7-8 (30% efficiency), the UI should display a clear warning: "Minimal extraction efficiency at this distance. Consider placing closer to a fluid source." This prevents the trap choice of placing a nearly useless extractor at maximum distance.

### Medium Priority (Should Address During Balancing)

5. **Consider reducing coverage radius gap.** The extractor at 8 tiles and reservoir at 6 tiles creates a 2-tile coverage gap that makes reservoirs less useful as coverage extenders. Consider equalizing at 7 tiles each, or increasing reservoir coverage to 7, to make reservoir placement more flexible. The gameplay distinction (extractor = production, reservoir = buffer) should be driven by their functional difference, not by coverage radius.

6. **Add a "reservoir recommended" indicator.** When a player has extractors but no reservoirs, the fluid status panel should display a persistent suggestion: "No reservoirs built -- fluid supply has no emergency buffer." This is a soft advisory rather than a warning, appearing only in the status panel and not as a banner or alert.

7. **Consider drain rate reduction.** The 100 drain rate empties a full reservoir in 10 ticks (0.5 seconds at 20 ticks/second). This is very fast -- a player might not even notice the reservoir draining before collapse. If playtesting shows that the drain happens too quickly for players to react, reduce the drain rate to 50 (matching the fill rate) for a symmetrical 20-tick drain time, or to 75 for a 13-tick drain. The asymmetry is still valuable for preventing reservoir-as-substitute, but the ratio could be 1.5:1 rather than 2:1.

8. **Plan for water table depletion mechanic.** Analogous to energy's nexus aging, a slow "water table depletion" mechanic would reduce extractor output over time for extractors placed near heavily tapped water sources. This would create long-term infrastructure management decisions: relocate extractors to fresh water sources, or build deeper (more expensive) extractors. This can be deferred to a future epic but should be planned in the FluidProducerComponent (add reserved fields for depletion_rate and current_depletion).

### Low Priority (Future Enhancement)

9. **Add extractor efficiency display to overlay.** When the fluid overlay is active, show efficiency percentages over each extractor. This gives the player ambient awareness of which extractors are running at optimal efficiency and which are suboptimal.

10. **Consider seasonal/drought events.** Periodic reductions in water availability would exercise the reservoir buffer system and create planning incentives to over-build reservoirs. A drought that reduces all extractor output by 30% for 200 ticks would test the player's buffer strategy without being catastrophic.

11. **Add a "fluid network health" visualization.** Beyond per-structure blue glow, consider a "circulatory system" visualization that shows fluid flow through conduits as animated blue pulses. The overlay UI requirements document describes this but it deserves emphasis: the flowing pulse animation on active conduits is a key emotional element. When fluid is flowing, the colony should feel alive with circulating blue light.

12. **Add reservoir type variants.** As with extractors, reservoir homogeneity limits strategic depth. A small/medium/large reservoir tier with different capacity/cost tradeoffs would give players more buffer strategy options. A treatment facility that requires energy but provides larger capacity would create an interesting dependency decision.

---

## Risks & Concerns

### Risk 1: All-or-Nothing Frustration
**Severity:** Medium
**Description:** The binary distribution model means a deficit of 1 fluid unit causes all consumers to lose fluid simultaneously. Players accustomed to energy's graduated rationing may find this disproportionately punishing.
**Impact:** Player frustration during tight-margin scenarios. "I was only 1 unit short and everything died" feels unfair even if the reservoir buffer was supposed to prevent it.
**Mitigation:** Reservoir buffer absorbs small deficits. Advisor messaging teaches reservoir importance. If playtesting confirms frustration, consider a "micro-rationing" variant where consumers within coverage are randomly served (each consumer has a probability of receiving fluid proportional to surplus/demand ratio). This preserves the "no priority" principle while softening the cliff.

### Risk 2: Extractor Uniformity
**Severity:** Medium
**Description:** One extractor type with one cost and one output creates a repetitive build experience. Every extractor placement feels the same except for the proximity question.
**Impact:** The fluid system feels thin compared to the energy system. Experienced players find no strategic depth beyond "build near water." The energy-vs-fluid comparison highlights how much richer the energy system is.
**Mitigation:** Plan extractor types for a future epic. The architecture supports it without changes.

### Risk 3: Reservoir Drain Speed
**Severity:** Low-Medium
**Description:** At 100 drain/tick and 20 ticks/second, a full reservoir empties in 0.5 seconds of real time. Players may not notice the drain happening before collapse occurs.
**Impact:** The "visible countdown" narrative fails if the countdown is too fast to see. The reservoir gauge transitions from full to empty before the player can react.
**Mitigation:** Reduce drain rate if playtesting confirms. The configurable value in `FluidReservoirConfig.h` makes this a one-line change. Multiple reservoirs extend the buffer proportionally: 3 reservoirs = 1.5 seconds, which is more noticeable.

### Risk 4: Cascading Failure Confusion
**Severity:** Low-Medium
**Description:** When energy deficit causes fluid deficit (because extractors lose power), the player sees both an energy warning and a fluid warning. They might try to fix the fluid problem directly (build more extractors) rather than fixing the root cause (energy deficit). New extractors would also fail because they also need power.
**Impact:** Player frustration from "I built more extractors but fluid is still failing." The root cause is not obvious from the fluid system's feedback alone.
**Mitigation:** Add a context-aware advisor message: when fluid deficit co-occurs with energy deficit and extractors are unpowered, display "Extractors require energy. Restore energy supply to resume fluid extraction." This connects the diagnostic dots for the player.

### Risk 5: Coverage Tedium
**Severity:** Low
**Description:** Same concern as energy: extending coverage across a large map requires many conduit placements. Fluid conduits at 3-tile coverage radius require even more placements than energy conduits to cover the same area.
**Impact:** Conduit placement becomes busywork rather than strategic activity.
**Mitigation:** Plan for drag-to-place conduit tool in Epic 12. Consider increasing conduit coverage radius to 4. Consider shared conduit infrastructure between energy and fluid systems (one conduit carries both, reducing placement count).

### Risk 6: "Nothing Is Happening" During Healthy State
**Severity:** Low
**Description:** Identical to the energy system concern. When fluid is working well, there is no active engagement with the system. Reservoirs sit at full, extractors hum along, and the player has no reason to interact.
**Impact:** Fluid becomes "set and forget" infrastructure. Players are surprised by crises they were not monitoring.
**Mitigation:** Reservoir level monitoring provides some ambient engagement (watching gauges is satisfying). Future drought events would create periodic re-engagement. Water table depletion would create long-term monitoring needs.

---

## Follow-Up Tickets (EPIC FF)

The following items should be tracked for future implementation:

1. **FF-FLUID-001: Add FluidMarginalBeganEvent / FluidMarginalEndedEvent** -- Pool state transition events for the Marginal state, enabling UI early warnings.
2. **FF-FLUID-002: Advisor prompt - "Build a reservoir"** -- First-extractor tutorial trigger suggesting reservoir construction.
3. **FF-FLUID-003: Extractor type variants (Surface/Deep/Artesian)** -- 2-3 extractor types with distinct cost/output/proximity profiles.
4. **FF-FLUID-004: Reservoir type variants (Cistern/Reservoir/Treatment)** -- 2-3 reservoir types with distinct capacity/cost/energy profiles.
5. **FF-FLUID-005: Water table depletion mechanic** -- Long-term extractor output reduction analogous to energy nexus aging.
6. **FF-FLUID-006: Drought events** -- Periodic water availability reduction exercising reservoir buffer strategy.
7. **FF-FLUID-007: Cascading failure advisor message** -- Context-aware message linking energy deficit to fluid failure through unpowered extractors.
8. **FF-FLUID-008: Distance 7-8 placement warning** -- UI warning for minimal-efficiency extractor placement.
9. **FF-FLUID-009: Conduit drag-to-place tool** -- Batch conduit placement for large-scale network construction (shared with energy system).
10. **FF-FLUID-010: Overlay efficiency display** -- Per-extractor efficiency percentage shown on fluid overlay.
11. **FF-FLUID-011: Shared conduit infrastructure evaluation** -- Design investigation into whether energy and fluid conduits should share physical placement.

---

## Final Verdict

**APPROVED WITH NOTES**

Epic 6 delivers a mechanically sound fluid infrastructure system that differentiates itself from energy through the all-or-nothing distribution model, reservoir buffering as explicit physical storage, and water proximity-based extractor efficiency. The cross-system energy dependency creates genuine strategic depth through cascading failure dynamics. The architecture is clean, the component design is compact and serialization-ready, and the event system provides comprehensive UI feedback (with the notable exception of Marginal state transitions, which should be added).

The primary concern is extractor and reservoir uniformity. The energy system's strategic richness comes from its six nexus types creating a complex decision landscape. The fluid system's single extractor type creates a simpler, shallower experience. This is acceptable for an initial implementation -- the system works and is fun -- but should be addressed in a near-term follow-up epic before the two systems are seen side-by-side in the final product. A player who spends thirty minutes optimizing their energy grid across six nexus types and then spends five minutes placing identical extractors near water will feel the imbalance.

The system is approved to proceed with the notes above incorporated into planning for the next balancing and feature-expansion passes.

---

*Review prepared by Game Designer Agent. All recommendations are suggestions for balancing iteration; the core architecture is sound and approved for implementation.*
