# Epic 7: Transportation Network - Game Design Review

**Reviewer:** Game Designer Agent
**Date:** 2026-02-08
**Canon Version:** 0.17.0
**Epic:** 7 - Transportation Network
**Files Reviewed:** 35 transport headers, tickets.md, canon files

---

## Verdict: APPROVED WITH NOTES

The transport system is architecturally sound and provides a solid gameplay foundation. The aggregate flow model, cross-ownership connectivity, and rail supplement design are well-considered decisions that serve the player experience. However, several balance concerns and missing gameplay elements need attention before this system will feel genuinely engaging during play.

**Confidence Score: 7/10**

The core mechanics are correctly structured. My confidence is tempered by the fact that several values will only prove out in playtesting, and a few design gaps could create frustrating player experiences if not addressed.

---

## Strengths

### 1. Cross-Ownership Connectivity is the Right Call (CCR-002)
The decision to connect pathways regardless of ownership is excellent for a social sandbox game. It creates organic cooperation: players naturally benefit from building toward each other. This aligns perfectly with Core Principle #4 (Endless Sandbox) and the "Connection" emotion pillar. Players will tell stories about linking their colonies together, and the shared network creates a sense of community without forcing cooperation.

### 2. Aggregate Flow Model Serves the Experience (CCR-006)
Not simulating individual vehicles avoids a massive complexity trap. Players care about whether their pathways are congested, not about tracking individual beings. The aggregate model keeps the mental model simple: "this pathway is busy" is all the player needs to know. This is depth without complication.

### 3. Pathway Decay Creates Meaningful Maintenance Tension
The decay system (health 0-255, five visual states, traffic-accelerated deterioration) creates an engaging maintenance loop. The player must balance growth with upkeep. The five visual states (Pristine/Good/Worn/Poor/Crumbling) give clear feedback at a glance. The traffic multiplier on decay (up to 3x) means busy pathways need more attention, which is intuitive and creates interesting choices about where to invest in TransitCorridors vs. BasicPathways.

### 4. Rail Supplements, Doesn't Replace (CCR-009)
Rail as a traffic reduction tool rather than a replacement transport mode is smart design. It gives players a secondary system to master and creates progression within the transport domain. The 50% traffic reduction at terminals with linear falloff to the coverage edge is a clean, understandable mechanic.

### 5. Clean Separation of Config from Logic
TrafficBalanceConfig, PathwayTypeConfig, DecayConfig, and RailConfig all being separate tunable structs is excellent for iteration. Every value can be adjusted without touching system logic. This is essential for getting balance right.

### 6. Grace Period for Stub Replacement (E7-019)
The 500-tick grace period when replacing the StubTransportProvider is a thoughtful anti-frustration measure. Existing buildings do not instantly die when real transport is activated. This shows consideration for the player experience during what is fundamentally a technical transition.

---

## Concerns

### Concern 1: Traffic Contribution Values May Be Too Flat (MODERATE)

**Current values:**
- Habitation: 2 flow/tick per occupant ratio
- Exchange: 5 flow/tick per occupant ratio
- Fabrication: 3 flow/tick per occupant ratio

The 2.5x ratio between Exchange and Habitation feels too narrow. In SimCity-style games, commercial zones generate significantly more traffic than residential because commercial attracts trips from the entire service area, while residential only generates trips from its own inhabitants. A habitation zone with 100% occupancy at level 3 generates 6 flow. An exchange zone at the same spec generates 15 flow. On a BasicPathway with capacity 100, it would take roughly 7 max-level exchange buildings to reach the "light traffic" threshold (flow 50). That seems too forgiving.

**Recommendation:** Consider widening the spread. Exchange at 8-10 and Fabrication at 5-6 would create more pronounced traffic pressure in mixed-use areas and make pathway placement more strategically interesting.

### Concern 2: Congestion Penalties Are Too Gentle (MODERATE)

**Current sector_value penalties:**
- Light (51-100): -5%
- Moderate (101-150): -10%
- Heavy (151-200): -15%
- Flow blockage (201-255): undefined beyond "severe"

A -15% sector_value penalty for "heavy traffic" is extremely mild. In SimCity 2000, heavily congested areas saw significant land value drops that forced players to act. At -15%, players can largely ignore congestion without meaningful consequence. This undermines the core tension of the transport system: the choice between building more infrastructure vs. accepting congestion.

Additionally, the "flow_blockage" tier (201-255) has no defined penalty percentage. This is the most critical tier and it needs explicit, steep consequences.

**Recommendation:** Consider a steeper curve:
- Light: -5% (keep)
- Moderate: -15%
- Heavy: -30%
- Flow blockage: -50% and additional effects (building degradation, being happiness reduction)

### Concern 3: Missing "StandardPathway" Creates a Progression Gap (HIGH)

The ticket spec (E7-024, E7-040) references a "StandardPathway" type in the edge cost table with cost 10 and 200 capacity, but the PathwayType enum only defines 5 types: BasicPathway, TransitCorridor, Pedestrian, Bridge, and Tunnel. There is no StandardPathway.

This creates a jump from BasicPathway (capacity 100, cost 50 credits) directly to TransitCorridor (capacity 500, cost 200 credits). That is a 5x capacity increase at a 4x cost increase. Players need a mid-tier upgrade option between "basic road" and "highway" to create satisfying progression within the pathway system.

**Recommendation:** Add StandardPathway (capacity 200, cost 100-120 credits, decay 0.75x) as the natural upgrade from BasicPathway. This matches what the edge cost table already anticipates.

### Concern 4: Pedestrian Pathways Lack Gameplay Purpose (LOW)

Pedestrian pathways have capacity 50 (half of BasicPathway), cost 25 credits, and free maintenance. But they serve no unique gameplay function -- they are just cheaper, worse pathways. In a city builder, pedestrian pathways should provide a distinct benefit that regular pathways cannot: improved being harmony, higher sector_value bonus, or green_zone-like desirability effects.

Without a unique benefit, players will never build pedestrian pathways because BasicPathways are strictly better per-credit for actual transport.

**Recommendation:** Give pedestrian pathways a sector_value bonus (similar to green_zones) and/or a harmony boost. They should be a different tool, not a worse version of the same tool.

### Concern 5: One-Way Pathways Have No Gameplay Integration (LOW)

PathwayDirection defines OneWayNorth/South/East/West, but the flow propagation system (FlowPropagation.h) makes no mention of directional constraints. Flow spreads equally to all connected pathway neighbors regardless of direction. The one-way system appears to be defined but not wired into any gameplay mechanic.

**Recommendation:** Either implement one-way flow logic in the diffusion model or defer PathwayDirection to a later epic. Defined-but-not-functional enums create confusion for the implementation team and could mislead players if exposed in the UI.

### Concern 6: No Player Feedback for "Why Is This Congested?" (MODERATE)

The system tracks congestion_level and flow_sources, but there is no mechanism for the player to understand what is causing congestion at a specific location. CongestionCalculator produces a number (0-255) but the player needs to know: "This pathway is congested because 4 exchange buildings and 2 fabrication buildings are feeding into it." The flow_sources field counts sources but does not identify them.

Understanding cause is essential for player agency. Without it, transport management becomes trial-and-error rather than strategic decision-making.

**Recommendation:** Add a diagnostic query that returns contributing buildings and their flow amounts for any pathway tile. This does not need to be in the simulation loop -- it can be a UI-time query.

### Concern 7: Contamination Threshold (128) Is Arbitrary and Undocumented (LOW)

The contamination system kicks in at congestion_level > 128 (50% of max). This is a significant gameplay threshold -- it means pathways at moderate-to-heavy traffic generate environmental contamination. But the player has no way to know this threshold exists until they see contamination spreading.

**Recommendation:** The contamination threshold should be visible in-game (perhaps as a color change on the pathway at congestion 128+) and documented in any "help" or tutorial content. Consider making it configurable via TrafficBalanceConfig.

### Concern 8: Rail System Has No Failure Feedback (LOW)

When a rail terminal loses power, it silently becomes inactive. There is no event emission for rail power loss/restoration. Players managing large rail networks need clear alerts when parts of their transit system go down.

**Recommendation:** Add RailPowerLostEvent and RailPowerRestoredEvent to the event system, parallel to the pathway deterioration events.

---

## Multiplayer Dynamics Analysis

### Shared Infrastructure Feel

The cross-ownership model is strong for cooperative dynamics. However, it creates a potential **free-rider problem**: Player A can build an expensive transit corridor that Player B's buildings use for free. The maintenance cost system (E7-021) correctly charges only the owner, but the benefit accrues to everyone on the network.

This is not necessarily bad for a social sandbox game -- it creates generosity dynamics and shared investment stories. But there should be a way for players to see who is benefiting from their infrastructure, and potentially a soft indicator of "mutual contribution" to the shared network.

### Griefing Potential

A player could theoretically demolish a pathway segment that connects two other players' networks, instantly severing their connectivity. The NetworkDisconnectedEvent exists, but there is no protection against this. In a casual social game, this could be frustrating.

**Recommendation:** Consider a "cannot demolish pathway segments connecting other players' buildings" rule, or at minimum a confirmation dialog and a grace period before buildings on the severed network enter Degraded state.

### Snowball Risk

Transport infrastructure costs are front-loaded (build cost) with ongoing maintenance. Players who expand early get better connectivity, which enables better building access, which generates more revenue, which funds more infrastructure. The decay system provides some money drain, but the fundamental snowball dynamic exists.

This is typical for city builders and not necessarily a problem in a sandbox game, but it means late-joining players will have a significantly harder time establishing connectivity.

---

## Missing Features for Fun Transport Experience

### 1. No Visual Traffic Indicators
The system calculates congestion but there is no spec for how players visually see traffic on pathways. City builders live and die on their data overlays. The PathwayRenderData struct (E7-028) includes congestion_level, but there is no overlay specification for a "traffic density" view.

### 2. No Pathway Upgrade In-Place
Players cannot upgrade a BasicPathway to a TransitCorridor without demolishing and rebuilding. This is busywork. An upgrade-in-place mechanic (pay the cost difference, keep connectivity) would feel significantly better.

### 3. No Traffic Routing Intelligence
The flow diffusion model spreads flow equally to all neighbors. This means flow does not prefer less-congested routes. A highway built specifically to bypass a congested area will not attract traffic toward it -- flow just diffuses uniformly. This is a fundamental limitation of the diffusion model vs. a routing model.

For MVP this is acceptable, but players will notice that building a TransitCorridor does not "pull" traffic away from congested BasicPathways the way they expect from city builder intuition.

### 4. No Public Transit Events or Milestone Integration
There are no milestones or achievements related to transport (e.g., "Connect all colonies," "Reduce congestion below X colony-wide," "Build first rail terminal"). Transport is a rich domain for player accomplishment feedback.

---

## Follow-Up Tickets

### F7-GD-001: Add StandardPathway Type
**Priority:** P0 | **Size:** S
Add PathwayType::StandardPathway (capacity 200, cost 100-120, decay 0.75x) to fill the progression gap between BasicPathway and TransitCorridor. Update PathwayTypeConfig, EdgeCost, and PathwayTypeStats. This addresses Concern 3 and gives players a meaningful mid-tier upgrade.

### F7-GD-002: Steepen Congestion Penalty Curve
**Priority:** P1 | **Size:** S
Revise TrafficBalanceConfig sector_value penalties to create stronger incentive to manage congestion. Proposed values: Light -5%, Moderate -15%, Heavy -30%, Flow Blockage -50%. Define explicit flow_blockage_penalty_pct. Add secondary effects at Flow Blockage tier (being harmony reduction). This addresses Concern 2.

### F7-GD-003: Add Pedestrian Pathway Sector Value Bonus
**Priority:** P1 | **Size:** M
Give Pedestrian pathways a unique gameplay role: sector_value bonus in nearby tiles (similar to green_zone effect). Define bonus radius and percentage. Ensure pathway type config supports a "desirability_bonus" field. This addresses Concern 4.

### F7-GD-004: Implement One-Way Flow Constraints or Defer
**Priority:** P1 | **Size:** M
Either integrate PathwayDirection into FlowPropagation (flow only spreads in the allowed direction for one-way segments) or remove one-way enums until a later epic. Do not ship defined-but-nonfunctional direction modes. This addresses Concern 5.

### F7-GD-005: Add Congestion Diagnostic Query
**Priority:** P1 | **Size:** M
Add a query function to TransportSystem that returns a list of contributing buildings and their flow amounts for any pathway tile. Used by the UI to show "this pathway is congested because of these buildings." This addresses Concern 6.

### F7-GD-006: Add Rail Power State Events
**Priority:** P1 | **Size:** S
Add RailPowerLostEvent and RailPowerRestoredEvent emission when rail segments or terminals change power state. Include entity_id, position, and rail_type in event data. This addresses Concern 8.

### F7-GD-007: Widen Traffic Contribution Spread
**Priority:** P2 | **Size:** S
Adjust TrafficContributionConfig to create more pronounced traffic differentiation between zone types. Proposed: Habitation 2, Exchange 8, Fabrication 5. Playtest to confirm feel. This addresses Concern 1.

### F7-GD-008: Add Pathway Upgrade-In-Place Mechanic
**Priority:** P2 | **Size:** M
Implement an upgrade API that replaces a pathway's type in-place (e.g., BasicPathway to StandardPathway to TransitCorridor) without requiring demolish-and-rebuild. Cost is the difference between build costs. Connectivity is preserved during upgrade. This addresses Missing Feature 2.

### F7-GD-009: Make Contamination Threshold Configurable
**Priority:** P2 | **Size:** S
Move the hardcoded congestion_level > 128 contamination threshold into TrafficBalanceConfig as a configurable field (contamination_threshold). This addresses Concern 7.

### F7-GD-010: Add Transport Milestones
**Priority:** P2 | **Size:** M
Define transport-related milestones for the ProgressionSystem (Epic 14): "First Junction" (place 10 pathways), "Network Architect" (connect to another player's network), "Transit Master" (place first rail terminal), "Flow Master" (maintain colony-wide congestion below Light). This addresses Missing Feature 4.

### F7-GD-011: Anti-Griefing Protection for Shared Pathways
**Priority:** P2 | **Size:** M
Add validation to pathway demolition: if removing a pathway would disconnect buildings belonging to other players, require confirmation and add a grace period (500 ticks) before those buildings enter Degraded state. This addresses the multiplayer griefing concern.

### F7-GD-012: Traffic Density Overlay Specification
**Priority:** P1 | **Size:** S
Define the visual specification for a "traffic density" data overlay that renders congestion_level as a color gradient on pathway tiles (green = free flow, yellow = light, orange = moderate, red = heavy, black = flow blockage). This is essential for player feedback and addresses Missing Feature 1.

---

## Summary Table

| ID | Title | Priority | Concern |
|----|-------|----------|---------|
| F7-GD-001 | Add StandardPathway Type | P0 | #3 - Progression gap |
| F7-GD-002 | Steepen Congestion Penalties | P1 | #2 - Too gentle |
| F7-GD-003 | Pedestrian Pathway Bonus | P1 | #4 - No unique purpose |
| F7-GD-004 | One-Way Flow or Defer | P1 | #5 - Nonfunctional enum |
| F7-GD-005 | Congestion Diagnostic Query | P1 | #6 - No cause feedback |
| F7-GD-006 | Rail Power State Events | P1 | #8 - Silent rail failure |
| F7-GD-007 | Widen Traffic Spread | P2 | #1 - Too flat |
| F7-GD-008 | Pathway Upgrade-In-Place | P2 | Missing feature |
| F7-GD-009 | Configurable Contamination Threshold | P2 | #7 - Hardcoded |
| F7-GD-010 | Transport Milestones | P2 | Missing feature |
| F7-GD-011 | Anti-Griefing for Shared Pathways | P2 | Multiplayer |
| F7-GD-012 | Traffic Density Overlay Spec | P1 | Missing feature |

---

## Final Notes

The transport system has a strong technical foundation and makes the right high-level design decisions (aggregate flow, cross-ownership, rail-as-supplement). The primary risk is that the current balance values are too gentle -- congestion penalties need teeth, and the pathway progression needs a middle tier. With the follow-up tickets addressed, this system should deliver the satisfying infrastructure management loop that city builder players expect.

The alien terminology usage is consistent throughout the codebase (pathway, transit_corridor, flow_blockage, junction, subterra). The code comments and documentation correctly reference canon terms. No terminology violations were found.

**Is this fun?** Not yet, but it has all the right bones. The missing piece is consequence -- players need to *feel* the difference between a well-designed transport network and a neglected one. Steeper penalties, better visual feedback, and the StandardPathway upgrade path will get us there.
