# Energy Balance Design Document

**Ticket:** 5-045
**Status:** Complete
**Canon Version:** 0.13.0
**Related:** 5-023 (Nexus Type Definitions), 5-037 (Structure Energy Requirements), 5-038 (Priority Assignments), 5-039 (Balance Thresholds)

---

## Table of Contents

1. [Nexus Type Comparison](#1-nexus-type-comparison)
2. [Structure Consumption Reference](#2-structure-consumption-reference)
3. [Game Scenarios](#3-game-scenarios)
4. [Crisis Experience](#4-crisis-experience)
5. [Recovery Flow](#5-recovery-flow)
6. [Pool State Thresholds](#6-pool-state-thresholds)
7. [Multiplayer Dynamics](#7-multiplayer-dynamics)
8. [Playtest Feedback Integration Plan](#8-playtest-feedback-integration-plan)

---

## 1. Nexus Type Comparison

The 6 MVP nexus types (per CCR-004) are divided into two categories: **dirty nexuses** that produce contamination, and **clean nexuses** that produce no contamination but have lower or variable output.

### Dirty Nexuses

| Attribute | Carbon | Petrochemical | Gaseous |
|-----------|--------|---------------|---------|
| **Output** | 100 | 150 | 120 |
| **Build Cost** | 5,000 | 8,000 | 10,000 |
| **Maintenance** | 50 | 75 | 60 |
| **Contamination** | 200 | 120 | 40 |
| **Coverage Radius** | 8 | 8 | 8 |
| **Aging Floor** | 60% | 65% | 70% |
| **Variable Output** | No | No | No |

**Design Intent:** Dirty nexuses offer reliable, high output at the cost of contamination. Carbon is the cheapest entry point. Petrochemical provides the best raw output among dirty types. Gaseous is the "cleanest dirty option" with only 40 contamination, making it a natural mid-game transition choice.

### Clean Nexuses

| Attribute | Nuclear | Wind | Solar |
|-----------|---------|------|-------|
| **Output** | 400 | 30 (variable) | 50 (variable) |
| **Build Cost** | 50,000 | 3,000 | 6,000 |
| **Maintenance** | 200 | 20 | 30 |
| **Contamination** | 0 | 0 | 0 |
| **Coverage Radius** | 10 | 4 | 6 |
| **Aging Floor** | 75% | 80% | 85% |
| **Variable Output** | No | Yes | Yes |

**Design Intent:** Nuclear is the endgame powerhouse -- massive output, zero contamination, but extremely expensive. Wind and Solar are cheap and clean but unreliable and low-output, requiring many installations. The variable output (weather/day-night cycle) creates interesting risk-reward decisions.

### Full Comparison Table

| Type | Output | Build Cost | Maintenance | Contamination | Coverage Radius | Aging Floor |
|------|--------|-----------|-------------|---------------|----------------|-------------|
| Carbon | 100 | 5,000 | 50 | 200 | 8 | 60% |
| Petrochemical | 150 | 8,000 | 75 | 120 | 8 | 65% |
| Gaseous | 120 | 10,000 | 60 | 40 | 8 | 70% |
| Nuclear | 400 | 50,000 | 200 | 0 | 10 | 75% |
| Wind | 30 (variable) | 3,000 | 20 | 0 | 4 | 80% |
| Solar | 50 (variable) | 6,000 | 30 | 0 | 6 | 85% |

### Key Trade-offs

- **Cost vs. Contamination:** Cheap dirty nexuses pollute the colony. Clean alternatives cost more or produce less.
- **Output vs. Reliability:** Wind and Solar are variable. Overseers relying on renewables must overbuild to handle low-output periods.
- **Coverage Radius:** Nuclear has the largest radius (10), reducing conduit requirements. Wind has the smallest (4), requiring dense conduit networks.
- **Aging Floor:** Clean nexuses degrade less over time. Solar retains 85% of its output even at maximum age, while Carbon drops to 60%.

---

## 2. Structure Consumption Reference

Energy consumption values assigned per structure type. These values populate `EnergyComponent.energy_required` when a structure is created from its template.

### Zone Structures

| Structure | Density | Energy Required | Priority |
|-----------|---------|----------------|----------|
| Habitation | Low | 5 | 4 (Low) |
| Habitation | High | 20 | 4 (Low) |
| Exchange | Low | 10 | 3 (Normal) |
| Exchange | High | 40 | 3 (Normal) |
| Fabrication | Low | 15 | 3 (Normal) |
| Fabrication | High | 60 | 3 (Normal) |

### Service Buildings

| Structure | Energy Required | Priority |
|-----------|----------------|----------|
| Medical nexus | 50 | 1 (Critical) |
| Command nexus | 40 | 1 (Critical) |
| Enforcer post | 30 | 2 (Important) |
| Hazard response post | 30 | 2 (Important) |
| Education facility | 25 | 3 (Normal) |
| Recreation facility | 20 | 3 (Normal) |

### Infrastructure

| Structure | Energy Required | Priority |
|-----------|----------------|----------|
| Energy nexuses | 0 (producers) | N/A |
| Energy conduits | 0 (infrastructure) | N/A |

**Note:** Energy nexuses produce energy; they do not consume it. Conduits extend coverage; they have no energy requirement.

### Energy-per-Unit Ratios

For quick balancing reference, here are the energy units each nexus type can support at full output:

| Nexus Type | Output | Low-Density Hab (5 ea.) | High-Density Hab (20 ea.) | Low-Density Fab (15 ea.) |
|------------|--------|------------------------|--------------------------|--------------------------|
| Carbon | 100 | 20 | 5 | 6 |
| Petrochemical | 150 | 30 | 7 | 10 |
| Gaseous | 120 | 24 | 6 | 8 |
| Nuclear | 400 | 80 | 20 | 26 |
| Wind | 30 | 6 | 1 | 2 |
| Solar | 50 | 10 | 2 | 3 |

---

## 3. Game Scenarios

### Early Game

**Context:** Player has just started their colony with minimal funds and few structures.

**Typical Setup:**
- 1 Carbon nexus (100 output, 5,000 credits)
- 5-10 low-density habitation zones (25-50 energy required)
- 1-2 low-density exchange zones (10-20 energy required)
- No conduits needed yet (nexus coverage radius of 8 covers initial area)

**Energy Balance:**
- Generation: 100
- Consumption: 35-70
- Surplus: 30-65 (Healthy state)

**Player Experience:** Energy is not yet a concern. The single Carbon nexus provides comfortable headroom. The player focuses on zoning and placement fundamentals. The nexus's contamination (200 units) is noticeable but manageable with few habitations.

**Design Goal:** Energy should feel like a resource that "just works" in the early game. The first Carbon nexus is cheap enough to build early and powerful enough to cover initial growth without stress.

### Mid Game

**Context:** Colony has grown. Multiple zone types, first service buildings, increased density.

**Typical Setup:**
- 2-3 mixed nexuses (Carbon + Petrochemical, or transitioning to Gaseous)
- 30-50 structures across all zone types
- First conduit networks extending coverage to distant zones
- Service buildings beginning to appear

**Energy Balance Example:**
- Generation: 100 (Carbon) + 150 (Petrochemical) + 120 (Gaseous) = 370
- Consumption: ~300 (mix of low/high density across zone types + services)
- Surplus: 70 (Marginal to Healthy)

**Player Experience:** Energy becomes a management consideration. The player must plan conduit networks to reach distant zones. Aging nexuses begin to show reduced output, requiring decisions about replacement or supplementing. Contamination from dirty nexuses starts affecting habitability near nexus sites.

**Key Decisions:**
- When to place new nexuses vs. extend conduits
- Whether to invest in cleaner energy (Gaseous) to reduce contamination
- Where to place conduits for optimal coverage
- Whether to start investing in Wind/Solar farms

### Late Game

**Context:** Large colony with high-density zones, multiple nexus types, complex conduit networks.

**Typical Setup:**
- 1-2 Nuclear nexuses for high-density core areas
- Wind/Solar farms in peripheral areas for clean energy
- Remaining Gaseous nexuses for reliable mid-range power
- Extensive conduit network spanning the map
- Service buildings at all priority levels

**Energy Balance Example:**
- Generation: 400 (Nuclear) + 400 (Nuclear) + 120 (Gaseous) + 150 (10x Wind + 5x Solar) = 1,070
- Consumption: ~900 (high-density zones + full service coverage)
- Surplus: 170 (Healthy with moderate buffer)

**Player Experience:** Energy is a strategic resource requiring ongoing management. Nexus aging means the player must periodically build new nexuses or accept reduced output. The transition from dirty to clean energy is a major economic decision. Nuclear nexuses are game-changing but require significant capital investment.

**Key Decisions:**
- Nuclear investment timing (50,000 credits is significant)
- Clean energy transition path (contamination vs. cost vs. reliability)
- Conduit network optimization for coverage efficiency
- Managing aging nexuses (replace vs. supplement)
- Balancing energy surplus buffer against over-investment

---

## 4. Crisis Experience

### Deficit State

**Trigger:** `surplus < 0` (consumption exceeds generation)

**What the Player Sees:**
- Warning notification: "Energy Deficit -- rationing in effect"
- Priority-based rationing activates:
  1. Critical structures (medical, command) remain powered
  2. Important structures (enforcer, hazard) powered next
  3. Normal structures (exchange, fabrication) may lose power
  4. Low-priority structures (habitation) cut first
- Unpowered habitations lose their glow effect (bioluminescent theme)
- Pool status HUD shows deficit amount in red

**Player Impact:**
- Unpowered habitation zones reduce habitability and happiness
- Unpowered exchange/fabrication zones reduce economic output
- Unpowered service buildings lose effectiveness
- Population may begin to leave if deficit persists

**Design Intent:** Deficit should feel urgent but not catastrophic. The priority system ensures critical services stay online, giving the player time to react. Habitations going dark is the most visible signal that something is wrong.

### Collapse State

**Trigger:** `surplus <= -collapse_threshold` (deficit exceeds 50% of total consumption)

**What the Player Sees:**
- Urgent notification: "Grid Collapse -- critical energy failure"
- Mass outages across all priority levels
- Only the most critical structures may remain powered
- Conduits dim across the network (visual: the colony "goes dark")
- Pool status HUD flashes collapse warning

**Player Impact:**
- Nearly all structures lose power
- Service buildings offline means increased disorder, fire risk, health problems
- Economic output drops dramatically
- Population exodus accelerates
- Recovery becomes the top priority

**Design Intent:** Collapse is a crisis moment that demands immediate player attention. The visual of the colony going dark is dramatic and reinforces the bioluminescent energy theme. The player must take decisive action to recover.

---

## 5. Recovery Flow

### From Deficit

**Approach 1: Build More Nexuses**
1. Identify cheapest available nexus type the player can afford
2. Place nexus in coverage range of existing network (or extend with conduits)
3. New nexus immediately contributes to pool on next tick
4. Once surplus >= 0, rationing ends and all structures in coverage are powered

**Approach 2: Reduce Consumption**
1. Demolish or zone-down low-priority structures
2. Temporarily disable (deconstruct) non-essential service buildings
3. Focus on powering the highest-value zones

**Approach 3: Optimize Coverage**
1. Remove conduits that extend to low-value areas
2. Concentrate coverage on high-priority zones
3. Consumers outside coverage no longer draw from the pool

### From Collapse

Recovery from collapse follows the same approaches as deficit but with greater urgency:

1. **Emergency nexus:** A single Carbon nexus (5,000 credits, 100 output) can immediately lift a small colony out of collapse
2. **Triage:** Focus conduit network on critical and important structures only
3. **Staged recovery:** Gradually re-extend coverage as generation capacity is rebuilt

**Grace Period (CCR-009):** When power is restored to a structure, there is a 100-tick (5-second) grace period before the building state updates. This prevents flickering during recovery and gives the player visual feedback that recovery is in progress.

### Recovery Time Expectations

| Colony Size | Deficit Recovery | Collapse Recovery |
|-------------|-----------------|-------------------|
| Small (< 50 structures) | 1 Carbon nexus | 1-2 Carbon nexuses |
| Medium (50-200 structures) | 1-2 nexuses or optimization | 2-3 nexuses + triage |
| Large (200+ structures) | Multiple nexuses + restructuring | Major rebuilding effort |

---

## 6. Pool State Thresholds

### Threshold Definitions

| State | Condition | Threshold Value |
|-------|-----------|----------------|
| **Healthy** | `surplus >= buffer_threshold` | `buffer_threshold = total_generated * 0.10` |
| **Marginal** | `0 <= surplus < buffer_threshold` | Between 0 and 10% of generation |
| **Deficit** | `-collapse_threshold < surplus < 0` | Negative surplus, not yet at 50% of consumption |
| **Collapse** | `surplus <= -collapse_threshold` | `collapse_threshold = total_consumed * 0.50` |

### Buffer Threshold: 10% of Generated

**Rationale:**
- A 10% buffer means a city generating 1,000 units needs at least 100 units of headroom to be Healthy
- Losing a single small nexus (50-200 units) can push from Healthy to Marginal, serving as early warning
- Low enough that players do not need to massively overbuild energy infrastructure
- Creates a narrow but meaningful Marginal band

### Collapse Threshold: 50% of Consumed

**Rationale:**
- The deficit must equal half of total consumption before collapse triggers
- Prevents brief or minor deficits from immediately causing grid collapse
- A city consuming 1,000 units only collapses if generation drops below 500
- Scales with consumption: larger cities have proportionally larger thresholds
- Gives players a window to respond to Deficit before Collapse

### Edge Cases

| Scenario | Result |
|----------|--------|
| Zero generation, zero consumption | Healthy (surplus = 0, buffer = 0, 0 >= 0) |
| Zero generation, nonzero consumption | Collapse (deficit always exceeds 50% threshold) |
| Exactly at buffer threshold | Healthy (uses >= comparison) |
| Exactly at collapse threshold | Collapse (uses <= comparison) |

### State Transition Diagram

```
                    surplus increases
     Collapse  ----------------------->  Deficit
        ^                                  ^  |
        |  surplus decreases               |  | surplus increases
        |                                  |  v
     Deficit  <-----------------------  Marginal
                    surplus decreases      ^  |
                                           |  | surplus increases
                                           |  v
     Deficit  <-----------------------  Healthy
                    surplus decreases
```

Events are emitted at each transition:
- `EnergyDeficitBeganEvent`: Entering Deficit from Healthy or Marginal
- `EnergyDeficitEndedEvent`: Leaving Deficit to Healthy or Marginal
- `GridCollapseBeganEvent`: Entering Collapse
- `GridCollapseEndedEvent`: Leaving Collapse

---

## 7. Multiplayer Dynamics

### Cross-Overseer Visibility

All players see all energy states for all overseers (CCR-008). This enables strategic awareness:
- A rival's colony going dark signals vulnerability
- Players can observe rival energy infrastructure investments
- "Schadenfreude" factor: watching a rival's grid collapse is part of the multiplayer experience

### Isolated Energy Pools

Energy does NOT flow between overseers. Each overseer maintains their own:
- Energy pool (generation, consumption, surplus)
- Coverage grid (conduits only connect within ownership)
- Rationing decisions (independent priority ordering)

### Competitive Implications

- Players who over-invest in clean energy gain contamination advantages
- Players who under-invest in energy risk deficit during growth phases
- Nuclear nexus investment is a visible signal of late-game economic strength
- Rival energy infrastructure can be observed to gauge their development stage

---

## 8. Playtest Feedback Integration Plan

This document is a living reference that should be updated based on playtesting sessions. The following areas are flagged for validation:

### Balance Validation Checklist

- [ ] Carbon nexus (100 output) comfortably powers early game (< 20 low-density habitations)
- [ ] Players naturally transition from dirty to clean energy as colony grows
- [ ] Deficit state triggers warning behavior without causing panic
- [ ] Collapse state feels dramatic and motivates immediate recovery action
- [ ] Recovery from deficit is achievable within 1-2 minutes of gameplay
- [ ] Nuclear cost (50,000) feels like a significant but achievable late-game investment
- [ ] Wind/Solar variable output creates interesting decisions without frustration
- [ ] Conduit placement feels rewarding ("painting glowing lines")
- [ ] Nexus aging creates ongoing management without harsh punishment
- [ ] 10% buffer threshold feels right for small cities (< 500 total consumption)
- [ ] 10% buffer threshold feels right for large cities (> 5,000 total consumption)
- [ ] 50% collapse threshold gives adequate response time

### Tuning Parameters

All values in this document are configurable and should be adjusted based on playtest data:

| Parameter | Current Value | Notes |
|-----------|--------------|-------|
| Nexus outputs | See comparison table | May need adjustment per type |
| Structure consumption | See consumption table | Density scaling may need tuning |
| Buffer threshold | 10% | Consider 5-15% range |
| Collapse threshold | 50% | Consider 30-70% range |
| Grace period | 100 ticks (5 sec) | May need to be longer for player comfort |
| Aging decay rate | Configurable per type | Affects long-term energy management feel |
| Conduit cost | 10 credits | Should feel cheap to encourage network building |
| Wind/Solar weather factor | 0.75 average (MVP stub) | Final implementation needs realistic variation |
