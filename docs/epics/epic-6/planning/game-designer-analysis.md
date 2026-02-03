# Game Designer Analysis: Epic 6 - Fluid Infrastructure (FluidSystem)

**Author:** Game Designer Agent
**Date:** 2026-01-28
**Canon Version:** 0.7.0
**Reference:** Epic 5 (EnergySystem) as structural template

---

## Experience Goals

### What Should Managing Fluid Feel Like?

**Core Fantasy: The Lifegiver**

Where energy makes your colony *function*, fluid makes it *alive*. Managing fluid should feel like nurturing an ecosystem. Energy is industrial and mechanical; fluid is organic and vital. The overseer isn't just powering machines -- they're sustaining life itself.

**Experiential Pillars:**

1. **Location Matters (Geographic Strategy)**
   - Unlike energy nexuses that can be placed almost anywhere, fluid extractors MUST be near water bodies
   - This creates natural competition for prime waterfront locations
   - Players should feel the weight of geography -- "I need to expand toward that still_basin"
   - Water scarcity creates genuine strategic tension, not just resource math

2. **Buffer Comfort (Reservoirs as Safety Nets)**
   - Fluid reservoirs (towers) store fluid, providing a buffer against temporary shortages
   - This should feel like having "savings" -- a comforting cushion
   - Watching reservoir levels rise should feel like growing security
   - Seeing them drain during extractor problems should create gentle anxiety, not panic

3. **Flow and Life (Visual Feedback)**
   - Fluid conduits should feel like veins carrying life through your colony
   - Active conduits pulse with soft blue/white bioluminescence
   - Buildings with fluid glow with a healthy, hydrated look
   - Dehydrated structures look parched, dimmed, stressed

4. **Interdependence with Energy**
   - Fluid extractors REQUIRE energy to operate
   - This creates a beautiful dependency chain: energy powers extractors, extractors provide fluid
   - Losing power means losing water (eventually, as reservoirs drain)
   - This layered dependency creates interesting crisis cascades

---

## Emotional Arc

### The Fluid Journey: From Plenty to Scarcity to Recovery

**Phase 1: Abundant Beginnings**
- Early game near water: fluid feels plentiful
- Single extractor easily serves early habitation
- Player develops false sense of security ("water is easy")
- Emotion: **Confidence, maybe complacency**

**Phase 2: Growing Pains**
- Colony expands, demand increases
- First fluid shortages appear in distant districts
- "Why isn't that building developing?" -- discovery that fluid matters
- Emotion: **Surprise, mild concern**

**Phase 3: Infrastructure Investment**
- Player builds conduit network to extend coverage
- Places reservoirs strategically for buffer zones
- Feels like "solving" fluid -- system mastery moment
- Emotion: **Satisfaction, pride in network design**

**Phase 4: Crisis Event (Scarcity)**
- Multiple pressures combine: rapid growth, extractor aging, energy problems
- Reservoir levels drop visibly
- Buildings begin dehydration cascade
- Emotion: **Tension, urgency, problem-solving mode**

**Phase 5: Recovery and Adaptation**
- Player adds extractors, extends network, manages priorities
- Reservoir levels stabilize and recover
- Buildings rehydrate and resume development
- Emotion: **Relief, accomplishment, lessons learned**

**Phase 6: Mastery**
- Player maintains healthy fluid surplus
- Reservoirs stay comfortably full
- Expansion planned with water access in mind
- Emotion: **Competence, strategic confidence**

---

## Fluid Infrastructure Fantasy

### How Each Infrastructure Type Should Feel

**Fluid Extractor (pump) -- "The Wellspring"**

- **Placement Feel:** Deliberate, geographic. Player scouts waterfront locations
- **Visual Fantasy:** Mechanical roots reaching into water, drawing up glowing fluid
- **Audio:** Soft rhythmic pumping sound, water flow
- **Strategic Role:** The production source. Must be protected (they need energy!)
- **Placement Satisfaction:** "I secured a good water spot" -- territorial pride
- **Alien Term:** fluid_extractor (not water pump)

**Fluid Reservoir (tower) -- "The Cistern"**

- **Placement Feel:** Protective, strategic. "Where do I need my safety buffer?"
- **Visual Fantasy:** Tall structure with visible fluid level, gentle glow from stored fluid
- **Audio:** Gentle sloshing, filling/draining sounds
- **Strategic Role:** Buffer against disruption. Peace of mind made visible
- **Placement Satisfaction:** "My district is protected" -- security through preparation
- **Alien Term:** fluid_reservoir (not water tower)

**Fluid Conduit (pipe) -- "The Veins"**

- **Placement Feel:** Connective, network-building. Drawing circulation paths
- **Visual Fantasy:** Underground pipes with surface indicators, soft blue glow pulses
- **Audio:** Subtle fluid flow when active, silence when dry
- **Strategic Role:** Extend coverage zones, connect extractors to consumers
- **Placement Satisfaction:** "My network reaches everywhere" -- completionist joy
- **Alien Term:** fluid_conduit (not water pipe)

---

## Fluid Shortage Experience

### How Fluid Shortage Differs From Energy Shortage

**Energy Shortage (Epic 5 Reference):**
- Buildings go dark (lose glow)
- Functions stop immediately
- Priority rationing keeps critical structures running
- Visual: Dead, dark, industrial failure
- Feel: The lights went out. Emergency mode.

**Fluid Shortage (Epic 6 - Different!):**

**Key Difference 1: Habitability Impact**
- Fluid shortage affects HABITABILITY, not just function
- Beings cannot live in dehydrated habitation zones
- Population starts leaving dehydrated areas
- This is more personal -- you're not just breaking machines, you're driving away inhabitants

**Key Difference 2: Gradual Decline**
- Reservoirs provide buffer time (grace period measured in reservoir capacity)
- Shortage is a slow drain, not an instant cutoff
- Player has warning time to react
- Feel: Growing concern rather than sudden crisis

**Key Difference 3: Health Effects**
- Prolonged fluid shortage reduces being vitality (health metric)
- Medical nexuses become more critical
- This creates secondary cascade effects
- Feel: The colony is suffering, not just malfunctioning

**Key Difference 4: Development Blocking**
- Buildings cannot DEVELOP without fluid (per canon: required_for_development: true)
- Zones stay stagnant even if powered
- Player sees demand but no growth
- Feel: Frustrating stagnation rather than active failure

**Visual Language for Fluid Shortage:**
- Structures lose their "hydrated" glow (different from energy's "powered" glow)
- Parched/cracked visual treatment (not dark, but dried)
- Blue/white bioluminescence fades to dull gray-blue
- Beings show stressed animations, slower movement

**Shortage Severity States:**

| State | Reservoir Level | Effect | Visual |
|-------|-----------------|--------|--------|
| Healthy | >50% | All systems normal | Full blue glow |
| Marginal | 25-50% | Warning, no immediate effect | Dimmer glow, warning icon |
| Deficit | 1-25% | Development halted, habitability dropping | Flickering glow |
| Collapse | 0% | Population fleeing, vitality dropping | Gray, parched look |

---

## Visual Integration

### Bioluminescent Fluid Effects

**The Fluid Color Palette (Distinct from Energy):**

| System | Base Glow Color | Active Color | Warning Color | Critical Color |
|--------|-----------------|--------------|---------------|----------------|
| Energy | Amber/Orange | Bright Green | Yellow | Red |
| Fluid | Soft Blue | Bright Cyan/White | Teal | Gray-Blue |

**Fluid Visual Language:**

1. **Fluid Extractors**
   - Base: Deep blue glow at water intake point
   - Active: Pulsing cyan/white as fluid is drawn
   - Connecting: Visible flow lines to conduit network
   - Offline: Dark blue, no pulse (but not black like unpowered energy)

2. **Fluid Reservoirs**
   - Empty: Structural frame visible, minimal glow
   - Filling: Rising blue glow from base to top
   - Full: Radiant blue-white beacon effect
   - Draining: Falling glow level, increasing pulse rate (urgency)
   - Visible Level: Player can gauge reservoir state at a glance

3. **Fluid Conduits**
   - Inactive: Dim blue trace line (infrastructure visible but not flowing)
   - Active: Soft blue pulse traveling along conduit path
   - High Flow: Brighter pulse, faster travel
   - Disconnected: No glow, just structural trace

4. **Hydrated Structures**
   - Healthy Habitation: Soft blue window glows, "life" feeling
   - Healthy Exchange: Blue accent lighting, active signage
   - Healthy Fabrication: Blue coolant glow effects
   - Dehydrated: All blue elements fade to gray, parched texture overlay

5. **Water Bodies (Source)**
   - Still Basin: Gentle bioluminescent particles floating
   - Flow Channel: Streaming light effects following current
   - Deep Void: Distant mysterious glow at edges

**Contrast with Energy:**
- Energy = warm colors (amber, green, yellow-red spectrum) = industrial/power
- Fluid = cool colors (blue, cyan, white) = life/water/organic
- Combined health = both glow systems active = vibrant, living colony

---

## Multiplayer Dynamics

### Water Scarcity as Competition and Cooperation

**Competitive Dynamics:**

1. **Waterfront Rush**
   - Prime water access is limited and location-dependent
   - Early chartering of waterfront sectors becomes strategically critical
   - "Water rights" create natural territorial competition
   - Player who secures water early has development advantage

2. **Water Denial (Soft Griefing Considerations)**
   - Per canon: Fluid conduits do NOT connect across overseer boundaries
   - Players cannot directly cut off rivals' water
   - BUT: chartering all waterfront tiles denies water access
   - Design consideration: Ensure multiple water access points on map generation

3. **Visible Rival Suffering**
   - All players see all fluid states (per CCR-008 pattern from Epic 5)
   - Watching rival's reservoirs drain creates "schadenfreude" moment
   - BUT also creates empathy -- "that could be me"
   - Adds stakes to the experience

4. **Resource Pressure Competition**
   - Large maps may have limited water bodies
   - Multiple players competing for same water sources (via tile chartering)
   - Creates natural conflict zones around valuable geography

**Cooperative Dynamics:**

1. **Water Trading**
   - Per canon: Players can trade resources via bank/trading system
   - Fluid becomes a tradeable commodity
   - Water-rich player can sell to water-poor player
   - Creates economic interdependence

2. **Shared Water Bodies**
   - Multiple players can build extractors on same large water body
   - No direct conflict (per-tile ownership) but shared resource
   - Large water bodies feel like "commons" -- shared natural resource

3. **Emergency Aid**
   - Trading fluid during rival's crisis creates relationship moments
   - "You helped me when my colony was dying of thirst"
   - Builds social bonds in endless sandbox context

4. **Infrastructure Adjacency**
   - While conduits don't connect, players may build parallel infrastructure
   - Visual: Neighboring conduit networks create interesting visual patterns
   - No mechanical benefit, but aesthetic cooperation

**Multiplayer Questions for Systems Architect:**

- Q1: Should water body capacity be shared between extractors of different players, or infinite per water body?
- Q2: Can fluid trades happen instantly, or is there a delivery mechanism?
- Q3: Should map generation ensure each player has reachable water access at spawn?

---

## Terminology Alignment

### Canonical Alien Terms (from terminology.yaml)

| Human Concept | Canonical Term | Usage Context |
|---------------|----------------|---------------|
| Water | Fluid | All references to the resource |
| Water system | Fluid network | The overall infrastructure |
| Water pump | Fluid extractor | Production structure |
| Water tower | Fluid reservoir | Storage structure |
| Pipes | Fluid conduits | Distribution infrastructure |
| Water pressure | Fluid pressure | Internal metric (if used) |
| Dehydrated | Fluid-deprived | Building state |
| Water shortage | Fluid deficit | Crisis state |
| Water coverage | Fluid coverage | Zone metric |

**UI Text Examples:**

- "Fluid extractor placed" (not "water pump built")
- "Fluid reservoir at 75% capacity" (not "water tower")
- "District has no fluid coverage" (not "no water")
- "Structure requires fluid to develop" (not "needs water")
- "Fluid deficit detected in outer district" (not "water shortage")
- "Beings leaving due to fluid deprivation" (not "water crisis")

**Avoid These Terms:**
- Water (use: fluid)
- Pump (use: extractor)
- Tower (use: reservoir)
- Pipe/pipeline (use: conduit)
- Hydration (use: fluid state or similar)
- Irrigation (fabrication-specific, discuss if needed)

---

## Questions for Other Agents

### For Systems Architect

1. **Reservoir Buffering Mechanics:**
   - How many ticks of buffer should a full reservoir provide?
   - Should reservoir capacity scale with structure level/upgrade?
   - Is fluid stored per-reservoir or pooled like energy?

2. **Extractor-Water Proximity:**
   - What is the maximum distance from water for extractor placement?
   - Can extractors be placed ON water tiles, or only adjacent?
   - Does water body type (still_basin vs flow_channel) affect output?

3. **Fluid-Energy Interdependence:**
   - Should unpowered extractors still consume from reservoir buffer?
   - When power returns, does extraction resume immediately or require restart?
   - Is there a "startup" delay after power restoration?

4. **Coverage Model Differences:**
   - Does fluid use identical pool model to energy (per CCR-001)?
   - Any reason for fluid conduit coverage to work differently?
   - Should conduit coverage radius differ from energy conduits?

5. **Priority System:**
   - Does fluid need priority levels like energy (CCR-003)?
   - If yes, same 4 levels or different priorities for fluid?
   - Rationale: fluid affects habitability, energy affects function -- different stakes?

### For Services Engineer

1. **Performance Considerations:**
   - Can FluidSystem share CoverageGrid pattern with EnergySystem?
   - Should fluid and energy coverage be stored in same grid or separate?
   - Reservoir level animation -- per-tick update or event-driven?

2. **Reservoir Mechanics:**
   - Reservoir fill rate vs drain rate -- symmetric or asymmetric?
   - Multiple reservoirs in same coverage zone -- how do they interact?
   - Can reservoirs transfer fluid to each other?

3. **Water Body System:**
   - Does Epic 3 TerrainSystem provide water_body_id and water_distance?
   - What interface do we need for extractor placement validation?
   - Is water body capacity finite or infinite?

---

## Design Recommendations

### Priority Recommendations

**P0 - Critical (Must Have for MVP):**

1. **Fluid Pool Model** - Per-overseer fluid pools, mirroring energy pattern
2. **Extractor Placement Validation** - Enforce water proximity requirement
3. **Reservoir Buffer Mechanic** - Stored fluid provides shortage protection
4. **Coverage Zone System** - Conduits extend fluid coverage, shared pattern with energy
5. **Building Fluid Requirement** - Structures need fluid for development (per canon)
6. **Basic Visual Feedback** - Hydrated vs dehydrated building states

**P1 - Important (Should Have):**

1. **Reservoir Level Display** - Visible fill level on reservoir structures
2. **Fluid Overlay** - UI overlay showing fluid coverage zones
3. **Shortage Notifications** - Events for deficit/collapse states
4. **Conduit Glow Animation** - Active vs inactive visual distinction
5. **Population Response** - Beings leave fluid-deprived habitation

**P2 - Enhancement (Nice to Have):**

1. **Water Body Visualization** - Bioluminescent water effects
2. **Flow Animation** - Visible fluid pulse along conduit network
3. **Reservoir Fill Animation** - Smooth level transitions
4. **Drought Events** - Disaster system integration (Epic 8)
5. **Fluid Trading UI** - Player-to-player fluid exchange interface

### Specific Design Decisions

**Decision 1: NO Fluid Priority System**

Unlike energy (which has 4 priority levels), fluid should NOT have priority-based rationing:

- **Rationale:** Fluid affects habitability uniformly. You can't say "these beings get water and these don't."
- **Instead:** All consumers in coverage share proportionally if deficit occurs
- **Experience:** Collective suffering rather than selective protection
- **Exception:** Consider if fluid extractors themselves need priority (they need power)

**Decision 2: Reservoir Buffer is Per-Reservoir, Not Pooled**

Unlike energy (pure pool model), reservoirs should store actual fluid:

- **Rationale:** Visual satisfaction of seeing levels rise/fall
- **Mechanic:** Reservoirs hold X units, release into pool when pool deficit
- **Experience:** Building reservoirs feels like building actual storage
- **Question:** Does this complicate the pool model too much? Need Systems Architect input.

**Decision 3: Slower Failure Cascade Than Energy**

Fluid shortage should unfold more slowly than energy shortage:

- **Energy:** Immediate impact on is_powered when deficit
- **Fluid:** Reservoir buffer provides grace period measured in game-cycles
- **Rationale:** Water shortage is a "slow death" -- matches real-world feel
- **Experience:** Player has time to react, doesn't feel like whiplash

**Decision 4: Water Body Capacity is Effectively Infinite**

Don't simulate water body depletion:

- **Rationale:** Complexity without meaningful gameplay
- **Instead:** Extractors have fixed output based on type/age
- **Exception:** Could add water_body_capacity as future drought mechanic
- **Multiplayer:** Multiple extractors on same water body don't compete

**Decision 5: Clear Visual Distinction from Energy**

Ensure players can instantly distinguish fluid state from energy state:

- **Energy overlay:** Warm colors (amber/green)
- **Fluid overlay:** Cool colors (blue/cyan)
- **Combined display:** Should be possible to show both simultaneously
- **Building glow:** Two distinct glow channels (energy glow + fluid glow)

### Experience Guidelines for Implementation

1. **First Fluid Crisis Should Be Educational, Not Punishing**
   - Clear notification: "Buildings require fluid to develop"
   - Obvious visual: Parched building appearance
   - Easy solution: Build extractor near water
   - Learning moment, not failure state

2. **Reservoirs Should Feel Like Insurance**
   - Visible level indicator at all times
   - "Full reservoir" should feel like accomplishment
   - Draining reservoir should create gentle urgency
   - Never feel like reservoirs are pointless

3. **Conduit Placement Should Feel Like Network Design**
   - Preview showing coverage delta (like energy per CCR-010)
   - Satisfaction in "completing" a network
   - Visual reward when conduits activate

4. **Water Location Should Drive Expansion Strategy**
   - Map generation ensures water access for all players
   - Water bodies should be visible and desirable on minimap
   - "Toward the water" should be a natural expansion direction

---

## Summary

Epic 6 introduces the second major infrastructure system, but with crucial experiential differences from energy:

1. **Geographic constraint** (must be near water) creates strategic depth
2. **Buffer mechanic** (reservoirs) provides time-based tension
3. **Habitability impact** (population leaves) makes fluid personally affecting
4. **Slower cascade** (reservoir drain) gives players time to react
5. **Cool color palette** (blue/cyan) visually distinguishes from energy

The goal is for fluid management to feel like **nurturing life** rather than **powering machines**. Players should care about their beings' fluid access the way they care about living things, not industrial processes.

---

**Document Status:** Ready for Systems Architect and Services Engineer review
**Next Steps:**
1. Systems Architect validates technical feasibility
2. Services Engineer reviews performance implications
3. Resolve open questions via agent discussion
4. Proceed to ticket creation
