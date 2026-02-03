# Game Designer Analysis: Epic 9 - City Services

**Author:** Game Designer Agent
**Date:** 2026-01-29
**Canon Version:** 0.10.0
**Status:** Planning Phase

---

## Player Experience Goals

### Core Fantasy

**"I protect and nurture my colony through thoughtful investment in services."**

City services represent the player's commitment to their beings' well-being. Unlike infrastructure (which enables growth), services represent quality of life choices. They answer the question: "What kind of colony do I want to build?"

### What Services Should Feel Like

| Service Type | Player Feeling |
|--------------|----------------|
| Enforcer Posts | "I maintain order and safety in my colony" |
| Hazard Response Posts | "I protect my colony from calamity" |
| Medical Nexuses | "I care for my beings' longevity" |
| Learning Centers / Archives | "I invest in my colony's future potential" |

### Emotional Beats

1. **Placement Satisfaction** - Seeing coverage radii light up feels like extending protection
2. **Problem Prevention** - Services working means NOT seeing bad things happen (subtle reward)
3. **Crisis Response** - When disaster strikes, watching hazard responders deploy creates dramatic moments
4. **Long-term Pride** - High longevity and knowledge quotient stats show colony maturity

### The Sandbox Service Philosophy

In an **endless sandbox**, services are tools for expression, not survival requirements:
- A colony with minimal services is a valid playstyle (lean and scrappy)
- A colony with maximum services is equally valid (nurturing overseer)
- Services create texture and options, not mandatory checkboxes
- No service should be strictly required to prevent game-ending spirals

---

## Interesting Decisions

### Decision 1: Coverage vs. Global Services

**The Split:**
- **Area-of-Effect (AoE)** services: Enforcer posts, Hazard response posts
- **Global-Effect** services: Medical nexuses, Learning centers, Archives

**Why This Creates Decisions:**

For **AoE services**, players face spatial puzzles:
- Where to place for optimal coverage?
- Do I cover high-value areas first or problem areas?
- How much overlap is acceptable vs. wasteful?
- Can I cover my neighbor's border problems?

For **Global services**, players face investment timing:
- When is my colony "mature enough" to justify a medical nexus?
- Do I invest in longevity (medical) or potential (education)?
- One large nexus vs. multiple smaller posts?

### Decision 2: Funding Trade-offs

**The Core Loop:**
```
Service Quality = Building Tier + Funding Level
```

Players must balance:
- **High funding** = maximum effectiveness, but drains treasury
- **Low funding** = reduced effectiveness, but saves credits for expansion
- **Variable funding** = adjust per-service based on priorities

**Design Intent:**
Funding should feel like a dial, not a switch. The difference between 80% and 100% funding should be noticeable but not catastrophic. Players who carefully tune funding should feel rewarded.

**Funding Tiers (Proposed):**

| Funding Level | Effectiveness | Player Perception |
|---------------|---------------|-------------------|
| 0% (Defunded) | 0% | Closed/inactive (visible) |
| 25% | 40% | Struggling, visible problems |
| 50% | 65% | Adequate, some gaps |
| 75% | 85% | Good coverage, minor issues |
| 100% | 100% | Full effectiveness |
| 125% | 110% | Overtime/premium (diminishing returns) |

**Note:** These feed into Epic 11 (Financial System) - stubbed for now with 100% effectiveness.

### Decision 3: Service Building Tiers

**Progression within Each Service Type:**

| Service | Tier 1 (Small) | Tier 2 (Standard) | Tier 3 (Advanced) |
|---------|----------------|-------------------|-------------------|
| Enforcers | Enforcer Post | Enforcer Station | Enforcer Nexus |
| Hazard Response | Response Post | Response Station | Response Nexus |
| Medical | Medical Post | Medical Center | Medical Nexus |
| Education | Learning Center | Archive | Knowledge Nexus |

**Trade-offs:**
- **Small buildings:** Cheap, fast to build, limited radius/capacity
- **Standard buildings:** Balanced cost/effectiveness
- **Advanced buildings:** Expensive, large footprint, excellent coverage

**Design Intent:** Players start with small buildings, upgrade or supplement as colony grows. Late-game optimization might involve replacing multiple small buildings with fewer large ones.

### Decision 4: Service Placement vs. Zone Development

**The Spatial Puzzle:**
Service buildings compete for land with zones:
- An enforcer post on a prime corner means no exchange building there
- Medical nexuses are large (2x2 or 3x3) - significant land commitment
- Players must plan districts that include service buildings

**Design Intent:** Service placement should feel like city planning, not just "drop it anywhere." Good overseers integrate services into their urban design.

---

## Feedback Mechanisms

### How Players Know Services Are Working

#### 1. Enforcer Posts (Disorder Reduction)

**Active Feedback:**
| Signal | What It Means |
|--------|---------------|
| Coverage overlay (cyan/blue glow) | Area under protection |
| Visible enforcer patrols | Service is active and funded |
| Disorder overlay comparison | Lower values in covered areas |
| Harmony stat improvement | Colony-wide benefit visible |

**Passive Feedback (Absence of Problems):**
- No disorder "eruptions" in covered areas
- Buildings maintain value/don't degrade
- No civil unrest events trigger

**Visual Design:**
- Enforcer posts should have a distinct "protective" visual - perhaps a calming blue pulse
- Enforcer beings visible patrolling the coverage area (cosmetic)
- Discordant "static" visuals appear in uncovered high-disorder areas

#### 2. Hazard Response Posts (Fire Protection)

**Active Feedback:**
| Signal | What It Means |
|--------|---------------|
| Coverage overlay (orange/amber glow) | Fire protection zone |
| Response time indicator | How fast help arrives |
| Protection level display | Current effectiveness |

**Reactive Feedback (During Conflagrations):**
- Visible hazard responders deploying to fires
- Fire containment/spread visualization
- "Saved" buildings flash green after extinguishment
- Post-disaster report showing buildings saved vs. lost

**Visual Design:**
- Hazard response posts should have an alert, ready appearance
- Warning lights active when conflagration detected in range
- Responder beings visible rushing to emergencies

#### 3. Medical Nexuses (Longevity - Global)

**Active Feedback:**
| Signal | What It Means |
|--------|---------------|
| Longevity stat in colony panel | Direct metric display |
| Population age distribution | More "elder" beings if longevity high |
| Health indicator on beings (cosmetic) | Visible vitality |

**Subtle Feedback:**
- Death rate trends downward over time
- "Beings saved by medical care" notification (rare, positive)
- Colony population stability (fewer fluctuations)

**Visual Design:**
- Medical nexuses should radiate a healing/calming glow
- No coverage overlay (global effect) - but show "medical access" indicator for buildings
- Healthy colony has more vibrant bioluminescence overall

#### 4. Learning Centers / Archives (Knowledge Quotient - Global)

**Active Feedback:**
| Signal | What It Means |
|--------|---------------|
| Knowledge quotient stat | Direct metric display |
| Education level breakdown | Low/medium/high educated population |
| Technology unlock progress | Education enables future tech (Epic 14) |

**Long-term Feedback:**
- Higher-tier buildings develop (educated workforce)
- Sector value bonus in educated colonies
- "Innovation" events (positive random events)

**Visual Design:**
- Learning centers should have an "active learning" visual - perhaps scrolling data patterns
- Archives should feel ancient and wise - glowing repository aesthetic
- Knowledge nexus should be visually impressive - a monument to learning

### Feedback Timing

| Service | Feedback Speed | Player Expectation |
|---------|----------------|-------------------|
| Enforcers | Fast (1-5 cycles) | "I built it, disorder should drop soon" |
| Hazard Response | Reactive (immediate when needed) | "When fire happens, they respond" |
| Medical | Slow (10-20 cycles) | "Long-term investment in longevity" |
| Education | Very Slow (20-50 cycles) | "Generational improvement" |

**Design Intent:** Fast feedback for safety services (players need to see results). Slow feedback for development services (rewards patience and planning).

---

## Questions for Other Agents

### @systems-architect: Coverage Calculation

1. **Coverage radius values:** What are the proposed tile radii for each service tier?
   - Enforcer post: 8 tiles? 12 tiles? 16 tiles?
   - Does coverage fall off with distance or is it binary (covered/not covered)?

2. **Coverage overlap behavior:** When two enforcer posts overlap:
   - Do effects stack? (Probably not - diminishing returns?)
   - Does the stronger coverage win?
   - Is there a "redundancy bonus" for reliability?

3. **Global service scaling:** For medical and education:
   - One nexus serves entire colony regardless of size?
   - Or effectiveness dilutes with population (need more buildings)?
   - Proposed: Effectiveness per capita - need to scale with population

### @systems-architect: Integration with Epic 10

4. **Disorder reduction formula:** How does enforcer coverage translate to disorder reduction?
   - Linear reduction in coverage zone?
   - Percentage reduction of base disorder?
   - What's the baseline (no enforcers) vs. maximum (full coverage)?

5. **Longevity calculation:** How does medical coverage affect life expectancy?
   - Base longevity without medical?
   - Maximum bonus from full medical coverage?
   - Does contamination interact with medical effectiveness?

6. **Service coverage data structure:** How does Epic 9 provide coverage data to Epic 10?
   - Per-tile coverage values?
   - Service effectiveness queries?
   - What interface does ServicesSystem implement?

### @systems-architect: Tick Ordering

7. **When does ServicesSystem tick?**
   - Before DisorderSystem (so disorder can query coverage)?
   - What priority number?

### @services-engineer: Service Building Specifics

8. **Footprint sizes:** What are the tile sizes for each service tier?
   - Small (1x1), Standard (2x2), Advanced (3x3)?
   - Any terrain requirements (flat land, road access)?

9. **Energy requirements:** Do services require energy to function?
   - What happens during grid collapse? Services offline?
   - Priority tier for service buildings (probably High/Critical)?

10. **Build costs:** Rough credit costs for each tier?
    - These feed into the funding trade-off decisions

---

## Balancing Considerations

### Enforcer Post Balancing

**Goal:** Enforcers should feel necessary but not mandatory.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Coverage radius (post) | 8 tiles | Small, local coverage |
| Coverage radius (station) | 12 tiles | Good neighborhood coverage |
| Coverage radius (nexus) | 16 tiles | District-level coverage |
| Max disorder reduction | 70% | Can't eliminate disorder entirely |
| Funding sensitivity | Medium | 50% funding = 65% effectiveness |

**Failure Mode (No Enforcers):**
- Disorder rises gradually
- Sector value decreases
- Migration out increases
- NOT instant collapse

**Balancing Insight:** Low-disorder terrain types (spore_flats?) and small colonies should be viable without enforcers. Large, dense colonies need enforcers to maintain order.

### Hazard Response Balancing

**Goal:** Hazard response should prevent catastrophic loss, not eliminate all fire.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Coverage radius (post) | 10 tiles | Larger than enforcers (fires spread) |
| Coverage radius (station) | 15 tiles | Good area coverage |
| Coverage radius (nexus) | 20 tiles | Major district coverage |
| Fire suppression speed | 3x faster in coverage | Significant but not instant |
| Funding sensitivity | High | Underfunded = slow response |

**Failure Mode (No Hazard Response):**
- Fires spread further before burning out
- More buildings damaged/destroyed
- Recovery takes longer
- NOT guaranteed total destruction

**Balancing Insight:** Fires should be manageable without hazard response (they eventually burn out), but damage is significantly higher. Hazard response is "insurance" against catastrophic loss.

### Medical Nexus Balancing

**Goal:** Medical services create long-term colony health improvement.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Base longevity (no medical) | 60 cycles | Beings have natural lifespan |
| Max longevity bonus | +40 cycles (100 total) | Significant improvement |
| Coverage scaling | Per capita (1 nexus per 10k beings?) | Scales with colony size |
| Funding sensitivity | Low | Medical is more about access than quality |

**Failure Mode (No Medical):**
- Higher death rate
- Lower population stability
- Slower population growth
- NOT instant population crash

**Balancing Insight:** Small colonies can thrive without medical. Large colonies need medical to maintain population. Medical is an investment in stability, not a survival requirement.

### Education Balancing

**Goal:** Education creates long-term potential and unlocks higher-tier development.

| Parameter | Proposed Value | Rationale |
|-----------|----------------|-----------|
| Base knowledge quotient | 50% | Uneducated but functional |
| Max knowledge quotient | 100% | Fully educated colony |
| Effect on building tier | +1 max tier at 75% education | High education enables high-density development |
| Time to see effects | 20-50 cycles | Generational improvement |
| Funding sensitivity | Medium | Quality education needs funding |

**Failure Mode (No Education):**
- Limited to low/medium tier buildings
- Lower sector value cap
- Slower technology unlocks (Epic 14)
- NOT a growth blocker

**Balancing Insight:** Education is an optional investment for players who want maximum colony potential. Uneducated colonies are viable but have a lower ceiling.

### Cross-Service Interactions

**Synergies:**
- High education + medical = maximum longevity (educated beings live longer?)
- Enforcer coverage + hazard response = full safety (multiplicative sector value bonus?)

**Anti-Synergies:**
- None planned - services should feel complementary, not competing

---

## Alien Theme Notes

### Terminology Reminders

| Human Term | Alien Term (Use This) | Context |
|------------|----------------------|---------|
| Police | Enforcers | Safety service |
| Police station | Enforcer post / station / nexus | Building tiers |
| Fire department | Hazard response | Safety service |
| Fire station | Hazard response post / station / nexus | Building tiers |
| Hospital | Medical nexus | Health service |
| Clinic | Medical post | Small health building |
| Health | Vitality | Being attribute |
| Life expectancy | Longevity | Colony metric |
| School | Learning center | Basic education |
| Library | Archive | Knowledge repository |
| Education | Knowledge quotient | Colony metric |
| Crime | Disorder | Negative metric |
| Crime rate | Disorder index | Metric display |

### Visual Theme Integration

**Enforcer Aesthetics:**
- Enforcers are not "police" in a human sense - they're harmony maintainers
- Visual: Calm blue/teal glow, resonance patterns
- Enforcer beings might be specialized alien forms (larger? different coloring?)
- Posts should feel like "calm zones" radiating order

**Hazard Response Aesthetics:**
- Not fire trucks - think rapid response organisms
- Visual: Alert amber/orange ready-state, shifts to active red during response
- Response beings might be specialized for danger (bio-suits? natural fire resistance?)
- Posts should feel prepared, vigilant, ready to deploy

**Medical Aesthetics:**
- Not hospitals in human sense - healing chambers, vitality nexuses
- Visual: Soft green/cyan healing glow, organic curves
- Medical beings are healers/caretakers
- Nexuses should feel nurturing, life-giving

**Education Aesthetics:**
- Not classrooms - knowledge transfer chambers, data archives
- Visual: Flowing data patterns, ancient wisdom + modern technology blend
- Learning centers are active, dynamic
- Archives are serene, contemplative
- Knowledge nexuses are monuments to accumulated wisdom

### Alien Narrative Hooks

**Why Do Aliens Need Enforcers?**
Even alien civilizations have discord. Enforcers maintain "resonance" - the harmonic frequency of a healthy colony. Disorder isn't crime in the human sense - it's dissonance, frequency disruption, beings out of sync with the collective.

**Why Do Aliens Need Hazard Response?**
Conflagrations threaten the organic structures of the colony. Hazard responders are specialized beings who can suppress thermal chaos and protect the living infrastructure.

**Why Do Aliens Need Medical?**
Beings have finite lifespans even in an advanced civilization. Medical nexuses extend that span, maintaining vitality and allowing beings to contribute longer to the colony.

**Why Do Aliens Need Education?**
Knowledge quotient represents the colony's accumulated wisdom and technical capability. Higher education enables more sophisticated development and unlocks advanced technologies.

---

## Summary

Epic 9 introduces **services as investment choices**, not mandatory requirements. The key design principles are:

1. **Two Service Categories:** AoE (spatial puzzle) vs. Global (investment timing)
2. **Funding as a Dial:** Quality scales with funding - meaningful trade-offs
3. **Clear Feedback:** Players know services are working through overlays, stats, and visible activity
4. **Sandbox Compatible:** No service is strictly required - all playstyles valid
5. **Alien Authenticity:** Services fit the bioluminescent alien theme

**Core Player Experience:** "I thoughtfully invest in my colony's well-being, making trade-offs between protection, health, and education. My service choices shape what kind of overseer I am."

---

## Next Steps

1. @systems-architect: Please provide technical analysis addressing the coverage and integration questions
2. @services-engineer: Please provide building specifications (costs, footprints, requirements)
3. Balancing values need validation against Epic 10 simulation formulas
4. Visual concepts needed for each service type's aesthetic

