# Game Designer Analysis: Epic 10 - Simulation Core

**Author:** Game Designer Agent
**Date:** 2026-01-29
**Canon Version:** 0.9.0
**Status:** Planning Phase

---

## Core Fantasy

**"I am the Overseer of a living, breathing alien colony."**

Epic 10 is the heartbeat of ZergCity. These systems transform a static collection of buildings into a dynamic organism that responds to player decisions. The core fantasy is not just "I built stuff" but "I nurture and guide a living colony."

### What Players Should Feel

1. **Vitality** - The colony pulses with life; beings move, numbers change, problems emerge and resolve
2. **Consequence** - My decisions ripple through the colony in visible, understandable ways
3. **Rhythm** - The colony has a daily/seasonal pulse that creates natural gameplay rhythms
4. **Emergence** - Unexpected interactions between systems create "stories" unique to each colony

### The Sandbox Promise

This is an **endless sandbox**, not a survival game. These simulation systems should create:
- Interesting problems to solve, not existential threats
- Goals to pursue, not failure states to avoid
- Texture and variety, not stress and punishment

A colony with high disorder and contamination is an **interesting challenge**, not a death spiral.

---

## System Experience Goals

### 1. SimulationCore (Tick Scheduling & Time)

**Player Experience:** "Time flows naturally; I control the pace of my world."

| Aspect | Goal |
|--------|------|
| Speed Control | Players should feel in control, not rushed. Pause at any time, speed up for waiting |
| Time Progression | Cycles/phases should feel meaningful - not just numbers ticking |
| Feedback Timing | Systems should update at a pace players can follow |

**Emotional Notes:**
- **Fast forward** should feel exciting - watching your colony bloom in accelerated time
- **Pause** should feel safe - I can take my time making decisions
- **Normal speed** should feel "alive" - constant subtle activity

**Sandbox Alignment:** Time is a tool, not a pressure. No urgency-based failure states.

---

### 2. PopulationSystem (Beings)

**Player Experience:** "My beings are thriving because I've created a good home for them."

| Metric | Player Perception |
|--------|-------------------|
| Population Count | Pride metric - "Look how many beings choose to live here!" |
| Birth Rate | Colony vitality indicator |
| Death Rate | Longevity feedback (health, services) |
| Migration | The ultimate report card - are beings choosing your colony? |
| Employment | Economic health indicator |

**Emotional Arc:**
1. **Early game:** Small population, every new being feels significant
2. **Mid game:** Growth momentum, watching numbers climb is satisfying
3. **Late game:** Population as status symbol, density challenges emerge

**Key Feel Requirement:** Population changes should feel **responsive but not volatile**. A single bad decision shouldn't cause mass exodus. Beings should have "inertia" - they don't flee at the first sign of trouble.

**Migration Philosophy:**
- Beings migrate **in** when your colony is desirable (high harmony, good services, available habitation)
- Beings migrate **out** when conditions deteriorate significantly over time
- Migration should be a slow pressure, not a sudden punishment
- **Sandbox rule:** You can never "lose all your beings" - there's always a minimum population that stays

---

### 3. DemandSystem (Zone Pressure - RCI)

**Player Experience:** "I understand what my colony needs, and I can provide it."

**Visual Concept:** Classic RCI bars showing demand for Habitation (R), Exchange (C), Fabrication (I)

| Demand State | Player Feeling |
|--------------|----------------|
| High Demand | Opportunity - "I should zone more of this!" |
| Balanced | Satisfaction - "My colony is well-planned" |
| Oversupply | Signal to rebalance - not punishment |

**Feedback Loop Quality:**
- Demand should be **legible** - players should understand WHY demand is high/low
- Demand factors should be **actionable** - "High Fabrication demand because population wants jobs"
- Response should be **timely** - zone the land, watch demand respond within a few cycles

**Anti-Frustration:**
- Demand imbalances should not cause immediate problems
- Oversupply gradually corrects (buildings downgrade) rather than crashing
- Demand indicators should guide, not dictate

---

### 4. LandValueSystem (Sector Value)

**Player Experience:** "I'm creating desirable neighborhoods - beings WANT to live here."

**What Affects Sector Value (Player Mental Model):**

| Positive | Negative |
|----------|----------|
| Water proximity | Contamination nearby |
| Terrain beauty (prisma_fields, spore_flats) | Disorder in area |
| Low disorder | Traffic congestion |
| Low contamination | Fabrication zones nearby |
| Good transport access | Distance from services |
| Service coverage | Abandoned/derelict structures |

**Why This Matters to Players:**
- High sector value = better buildings develop
- Better buildings = higher tribute (tax revenue)
- Higher tribute = more credits for services/infrastructure
- More services = happier beings = more migration
- **This is the core positive feedback loop of city builders**

**Visual Feedback:**
- Clear overlay showing sector value heat map
- Obvious visual difference between high/low value areas
- Building quality should visually reflect sector value

---

### 5. DisorderSystem (Crime)

**Player Experience:** "I maintain order in my colony through thoughtful enforcement."

**Disorder Philosophy for Sandbox:**
Disorder is an **interesting problem to solve**, not a failure state. High disorder makes certain gameplay choices more challenging but never causes game-ending cascades.

| Disorder Level | Player Experience |
|----------------|-------------------|
| Low (0-20%) | Peaceful colony, easy management |
| Moderate (20-50%) | Active challenge, enforcers working, visible but manageable |
| High (50-80%) | Serious problem requiring attention, clear feedback, buildings affected |
| Critical (80%+) | Major project to address, but NOT a death spiral |

**What Causes Disorder:**
- High density with low sector value
- Unemployment
- Inadequate enforcer coverage
- Abandoned/derelict buildings (attracts disorder)
- Contaminated areas

**How Players Address It:**
- Build enforcer posts (direct solution)
- Improve sector value (addresses root cause)
- Reduce unemployment (addresses root cause)
- Clear abandoned buildings (removes hotspots)

**Disorder Spread:**
- Should be visible but slow - players have time to respond
- Enforcers create "safe zones" that resist spread
- Disorder should not "jump" to well-maintained areas

**Sandbox Rule:** Even at maximum disorder, buildings still function (reduced efficiency). Disorder is a drain on harmony and sector value, not a building destroyer.

---

### 6. ContaminationSystem (Pollution)

**Player Experience:** "I balance progress with environmental responsibility."

**Contamination Philosophy:**
Contamination creates interesting **trade-offs**, not punishment. Industrial fabrication creates contamination but also creates jobs and revenue. The player decides how to balance this.

| Contamination Level | Effects |
|---------------------|---------|
| Low | Aesthetic - some visual haze, minimal impact |
| Moderate | Sector value reduction, reduced longevity in area |
| High | Significant habitability impact, beings avoid area |
| Severe | Area essentially uninhabitable for habitation zones |

**Contamination Sources (clear to players):**
- Fabrication zones (primary source)
- Certain energy nexuses (carbon, petrochemical, gaseous)
- Heavy traffic pathways
- Blight_mires terrain (natural source)

**Contamination Solutions:**
- Spatial planning - put fabrication away from habitation
- Green zones (parks) - absorb contamination
- Technology upgrades - cleaner fabrication buildings
- Natural decay - contamination slowly dissipates over time

**Contamination Spread:**
- Wind direction concept? (adds strategic depth)
- Gradual spread, not instant
- Clear visual feedback (haze, color shift in affected tiles)

**Sandbox Rule:** Contamination doesn't destroy buildings. Even heavily contaminated areas can be remediated over time. No permanent damage.

---

## Feedback Loops

### The Core Positive Loop (Growth Spiral)

```
Build Infrastructure → Attract Beings → Generate Tribute → Build More Infrastructure
        ↑                                                           ↓
        └───────────── Higher Sector Value ←────────────────────────┘
```

This is the fundamental city builder satisfaction loop. It should feel **rewarding** and **clear**.

### The Harmony Loop

```
Good Services → High Harmony → High Migration In → Population Growth
      ↑                                                      ↓
      └────────── More Tribute Revenue ←────────────────────┘
```

Players who invest in their beings' well-being are rewarded with growth.

### The Disorder/Contamination Pressure Loops

These are **balancing loops** that create texture, not death spirals:

**Disorder Loop:**
```
High Density + Low Value → Disorder → Reduced Sector Value → Less Development
                              ↓
                        Enforcement → Order → Value Recovery
```

**Contamination Loop:**
```
Fabrication → Contamination → Reduced Habitability → Beings Avoid Area
                    ↓
          Natural Decay + Green Zones → Remediation
```

### Emotional Impact of Loops

| Loop | Emotion |
|------|---------|
| Growth Spiral | Pride, satisfaction, momentum |
| Harmony Loop | Nurturing, caretaking |
| Disorder Pressure | Vigilance, problem-solving |
| Contamination Trade-off | Strategic thinking, planning |

**Critical Design Rule:** Negative loops should NEVER outpace player ability to respond. The sandbox promise means players always have time and tools to address problems.

---

## Emotional Arc

### Session Arc (Single Play Session)

1. **Opening (0-10 min):** Survey colony state, identify priorities
2. **Active Play (10-45 min):** Build, expand, solve problems, watch results
3. **Satisfaction Peak:** Milestone reached, problem solved, beautiful moment
4. **Natural Pause:** Good stopping point, colony stable

### Colony Lifecycle Arc

1. **Founding:** Excitement, possibility, small decisions feel big
2. **Growth:** Momentum, watching numbers climb, infrastructure challenges
3. **Maturity:** Optimization, prestige buildings, aesthetic refinement
4. **Legacy:** Pride in creation, showing off to friends, "immortalize" option

### Emotional Beats Within Simulation

| Event | Emotion |
|-------|---------|
| Population milestone | Pride, achievement |
| First disorder outbreak | Concern, then satisfaction (solving it) |
| Beautiful high-value district | Aesthetic pleasure |
| Colony pulse visible (beings moving) | Life, vitality |
| Solving contamination problem | Competence, clean-up satisfaction |

---

## Balancing Framework

### Initial Tuning Philosophy

These are **starting points**, not final values. Tuning should be data-driven and feel-tested.

### Population Growth Rates

| Factor | Suggested Range | Notes |
|--------|-----------------|-------|
| Base birth rate | 0.5-1.0% per cycle | Slow but visible growth |
| Base death rate | 0.3-0.5% per cycle | Slightly below birth = natural growth |
| Migration rate (max) | 2-5% per cycle | Can swing both ways based on conditions |
| Migration threshold | 30% harmony difference | Significant gap needed to trigger migration |

**Key Principle:** Growth should feel "earned" but not blocked. A well-run colony grows steadily. A neglected colony stagnates but doesn't collapse.

### Disorder Tuning

| Parameter | Suggested Value | Rationale |
|-----------|-----------------|-----------|
| Base disorder generation | 0.1% per tick | Constant low pressure |
| Density multiplier | Up to 3x at max density | Dense areas need more enforcers |
| Enforcer effectiveness radius | 8-12 tiles | Meaningful coverage, not overwhelming |
| Disorder decay rate | 5% per tick when covered | Quick recovery in enforced areas |
| Max disorder impact | -30% sector value | Significant but not devastating |

### Contamination Tuning

| Parameter | Suggested Value | Rationale |
|-----------|-----------------|-----------|
| Fabrication contamination | 1-3 units per tick | Clear source |
| Energy nexus (dirty) | 2-5 units per tick | Trade-off for cheap power |
| Natural decay rate | 1% per tick | Slow self-healing |
| Spread rate | 0.5% per tick to adjacent | Gradual, not instant |
| Max habitability impact | -50% capacity | Severe but recoverable |

### Sector Value Tuning

| Factor | Suggested Impact | Notes |
|--------|------------------|-------|
| Water proximity | +20-30% | Major positive |
| Prisma_fields proximity | +15-20% | Alien terrain bonus |
| Low disorder bonus | +10-20% | Safe neighborhoods |
| Contamination penalty | -10-40% | Scales with severity |
| Service coverage | +5-15% | Per service type |

### Migration Sensitivity

**Beings should be "sticky" but not deaf to conditions.**

- Small harmony changes: No migration response
- 10-20% harmony difference: Slight migration adjustment
- 30%+ harmony difference: Noticeable migration
- 50%+ harmony difference: Significant migration (but never total exodus)

---

## Visual/Audio Feedback

### Visual Feedback Recommendations

#### Population & Colony Vitality
| System | Visual Feedback |
|--------|-----------------|
| Population | Cosmetic beings visible on pathways (scales with population) |
| Employment | Beings moving toward fabrication/exchange zones during work phases |
| Migration | Moving convoys arriving/departing at colony edges |
| Growth | Subtle "shimmer" or glow on buildings when population increases |

#### Disorder
| State | Visual |
|-------|--------|
| Low disorder | Normal appearance |
| Moderate | Subtle visual noise/static on affected buildings |
| High | Visible "disruption" patterns, darker lighting |
| Enforcer activity | Visible enforcer beings patrolling, calming glow |

#### Contamination
| Level | Visual |
|-------|--------|
| Low | Slight atmospheric haze |
| Moderate | Yellow-green tint, visible particles |
| High | Dense haze, sickly glow, affected vegetation wilts |
| Source | Clear emanation from fabrication/energy nexuses |

#### Sector Value
| Value | Visual |
|-------|--------|
| Low | Dimmer lighting, less bioluminescence |
| Medium | Normal appearance |
| High | Brighter bioluminescence, premium building variants |
| Very High | Prestige glow, visible wealth indicators |

### Audio Feedback Recommendations

| Event/State | Audio Concept |
|-------------|---------------|
| Population milestone | Achievement chime, celebratory tone |
| Colony activity | Ambient hum that scales with population (alive-feeling) |
| Disorder rising | Subtle dissonant undertone |
| Disorder addressed | Resolution tone, harmony restored |
| Contamination | Industrial drone in affected areas |
| High sector value | Pleasant ambient tones |
| Migration arrival | Welcoming sound, arrival fanfare |

### UI Feedback

| Information | UI Approach |
|-------------|-------------|
| Population | Clear number display, trend arrow |
| Demand (RCI) | Classic bars with labeled factors |
| Disorder | Overlay toggle, heat map |
| Contamination | Overlay toggle, visual spread map |
| Sector Value | Overlay toggle, gradient visualization |

---

## Questions for Other Agents

### @systems-architect: Circular Dependencies

The canon notes that LandValueSystem uses "PREVIOUS tick's disorder/contamination to avoid circular dependency."

**Questions:**
1. Does this one-tick delay create any noticeable gameplay issues?
2. Should players perceive this as "instant" or is a visible delay acceptable?
3. How do we handle the first tick where there's no "previous" data?

### @systems-architect: Population Granularity

**Questions:**
1. Are beings tracked as individual entities or as aggregate numbers?
2. If aggregate, how do we generate the cosmetic beings for visual feedback?
3. What's the performance impact of showing cosmetic beings at high populations?

### @systems-architect: Contamination Spread Model

**Questions:**
1. Is wind direction a viable mechanic for contamination spread?
2. What's the grid resolution for contamination tracking?
3. How does terrain (ridges, flow_channels) affect spread?

### @systems-architect: Demand Factor Legibility

**Questions:**
1. Can we expose the demand calculation factors to the UI for player understanding?
2. What data structure represents "why is habitation demand high"?

### @systems-architect: Service Integration

Epic 9 (Services) provides service buildings that Epic 10 consumes.

**Questions:**
1. What interface does Epic 10 use to query service coverage?
2. How do we handle the case where Epic 10 runs without Epic 9 (stub/default)?

### @systems-architect: Tick Priority and Causality

**Questions:**
1. The tick priorities (Population: 50, Disorder: 70, Contamination: 80) suggest order of evaluation. Does this create any player-perceivable causality issues?
2. Should disorder/contamination changes from THIS tick affect population THIS tick or NEXT tick?

---

## Sandbox Considerations

### Core Sandbox Principles for Epic 10

1. **No Fail States:** No combination of system states should end the game or make recovery impossible
2. **Slow Pressure:** Negative effects build gradually, giving players time to respond
3. **Always Recoverable:** Any problem can be addressed with enough time and resources
4. **Interesting Problems:** Challenges should be puzzles to solve, not punishments to endure

### Specific Sandbox Rules

#### Population
- Minimum population floor: Colony always has at least X beings (prevents "empty city")
- Migration OUT is capped: Never lose more than Y% per cycle
- Death rate ceiling: Longevity can drop but never causes population crash

#### Disorder
- Disorder doesn't destroy buildings
- Disorder doesn't cause being deaths (disorder affects harmony, not survival)
- Maximum disorder impact is sector value reduction, not infrastructure damage
- Enforcers always have SOME effect (never 0% effectiveness)

#### Contamination
- Contamination doesn't permanently damage terrain (except maybe extreme edge cases)
- Contamination naturally decays (world heals itself over time)
- Contamination affects habitability, not building health
- Fabrication zones are immune to their own contamination effects

### The "Neglected Colony" Experience

What happens if a player completely ignores their colony?

**Sandbox Answer:** It stagnates but doesn't die.
- Population stabilizes at a minimum
- Buildings downgrade but don't disappear
- Disorder rises but caps out
- Contamination spreads but has a natural maximum
- The colony becomes "dormant" but can always be revived

### Multiplayer Sandbox Considerations

- One player's neglected colony shouldn't harm other players
- Disorder/contamination shouldn't "invade" other players' territory
- A trailing player should always be able to catch up given time

---

## Alien Theme Integration

### Terminology Application

| System | Human Term | Alien Term (Use This) |
|--------|------------|----------------------|
| Population | Citizens | Beings |
| Population | Population | Colony population |
| Time | Year | Cycle |
| Time | Month | Phase |
| Time | Day | Rotation |
| Metrics | Happiness | Harmony |
| Metrics | Crime | Disorder |
| Metrics | Crime rate | Disorder index |
| Metrics | Pollution | Contamination |
| Metrics | Land value | Sector value |
| Services | Police | Enforcers |
| Zones | Residential | Habitation |
| Zones | Commercial | Exchange |
| Zones | Industrial | Fabrication |

### Thematic Flavor for Simulation

#### Population Narrative
Beings are not just "people" - they're members of an alien collective. Consider:
- Do they reproduce differently? (Spawning pools? Hive births?)
- Migration could be "drift" or "swarm movement"
- Employment might be "assignment to function"

#### Disorder Narrative
Disorder isn't "crime" in the human sense. For aliens:
- Could be "disharmony" or "frequency disruption"
- Enforcers might be "harmonizers" or "resonance keepers"
- The visual could be "discordant energy patterns" rather than crime activity

#### Contamination Narrative
Contamination fits alien theming well:
- Could be "essence corruption" or "bio-static interference"
- Natural contamination sources (blight_mires) are ancient/primordial
- Clean energy is "pure resonance" vs. dirty energy creating "harmonic waste"

#### Sector Value Narrative
Property value becomes:
- "Harmony resonance" of an area
- "Desirability quotient"
- Higher value = stronger "life pulse"

### Visual Theme Integration

All simulation feedback should reinforce the bioluminescent alien aesthetic:
- Healthy colony: Strong, vibrant bioluminescence
- High harmony: Synchronized pulsing of lights
- Disorder: Erratic, discordant light patterns
- Contamination: Sickly coloring, dimmed or corrupted glow
- High sector value: Premium glow intensity and colors

---

## Summary

Epic 10 is the soul of ZergCity. These systems transform static infrastructure into a living world. The key design principles are:

1. **Sandbox First:** Create interesting challenges, not fail states
2. **Legible Feedback:** Players should understand cause and effect
3. **Emotional Resonance:** Systems should create feelings, not just numbers
4. **Alien Authenticity:** Terminology and theming should be consistent
5. **Responsive but Stable:** Changes should feel meaningful but not volatile

The ultimate test: Does this make players feel like benevolent overseers nurturing a thriving alien colony? If yes, we've succeeded.

---

## Next Steps

1. @systems-architect: Please provide technical analysis addressing the questions above
2. Balancing framework values need playtesting to validate
3. Visual/audio feedback needs concept work and prototyping
4. Consider creating a "simulation feel" test level for isolated system testing
