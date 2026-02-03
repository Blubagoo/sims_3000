# EPIC 5: Energy Infrastructure (EnergySystem) - Game Designer Analysis

**Author:** Game Designer Agent
**Date:** 2026-01-28
**Epic:** 5 - Power Infrastructure
**Status:** Planning

---

## 1. Experience Goals

### What Should Energy Management Feel Like?

Energy management should feel like **maintaining a living organism**. The energy matrix is the lifeblood of your colony - when it flows, everything glows with bioluminescent life. When it falters, your settlement becomes a dark void in an already alien landscape.

#### Core Emotional Beats

**The Hum of Prosperity**
When energy is stable, the player should feel a quiet satisfaction. Structures pulse with soft bioluminescent glow, conduits trace glowing lines across the terrain, and the colony feels *alive*. This is the reward state - everything working in harmony.

**The Tension of Capacity**
As demand approaches supply, players should feel a mounting pressure - not panic, but awareness. "I'm running hot." The energy overlay should show amber warnings before crisis. This creates meaningful decision points: expand nexus capacity, or slow zone development?

**The Crisis of Grid Collapse**
When energy runs out, it shouldn't feel like punishment - it should feel like a *story*. Grid collapse is dramatic, visual, and recoverable. The colony goes dark in waves, bioluminescence fading district by district. This is a moment of crisis that creates memorable player experiences.

**The Satisfaction of Recovery**
Bringing energy back online should feel earned. Watching your colony re-illuminate sector by sector, conduit by conduit, should be as satisfying as the initial build.

#### Design Philosophy

Energy is the **first major tension-creating system** in the game. Unlike peaceful zoning (Epic 4), energy introduces consequences. Structures cannot function without it. This gives player decisions weight.

However, we must balance tension with accessibility:
- **Never punitive** - Grid collapse pauses buildings, doesn't destroy them
- **Always recoverable** - Clear paths back to stability
- **Visually legible** - Players should always understand their energy state at a glance

---

## 2. Emotional Arc

### Energy Infrastructure Through the Game Lifecycle

#### Early Game: Scarcity and Discovery

**Cycles 0-50**

The player's first energy nexus is a pivotal moment. They've zoned their initial habitation areas, perhaps an exchange district - now they need energy to make it all work.

**Experience Goals:**
- First nexus placement should feel significant (tutorial moment)
- Immediate feedback: watch unpowered structures come alive with glow
- Tight energy budget creates meaningful choice ("Do I expand zones or build another nexus?")
- First energy deficit should be a learning experience, not a failure state

**Emotional Tone:** Careful optimism. "I'm building something that lives."

#### Mid Game: Growth vs. Infrastructure

**Cycles 50-200**

The colony is growing. Zone pressure is rising. But every new habitation tower, every exchange hub, draws more energy. The player must balance expansion ambition against infrastructure investment.

**Experience Goals:**
- Energy becomes a strategic resource, not just a checkbox
- Players should feel the tension of "one more district" vs. "one more nexus"
- Conduit placement becomes a spatial puzzle (coverage optimization)
- Plant diversity introduces choice (cheap/dirty vs. expensive/clean)
- First major energy deficit creates a memorable crisis moment

**Emotional Tone:** Strategic tension. "I can grow faster if I take risks."

#### Late Game: Optimization and Legacy

**Cycles 200+**

The colony is established. Energy infrastructure needs maintenance - nexuses age, efficiency drops. Clean energy becomes a prestige goal. The player shifts from survival to optimization.

**Experience Goals:**
- Aging nexuses create long-term planning challenges
- Transition to clean energy feels like a civilizational achievement
- Legacy coal/plasma nexuses can be decommissioned (satisfying removal)
- Excess energy capacity feels luxurious, not wasteful
- Optimized conduit networks become a source of pride

**Emotional Tone:** Pride and refinement. "I've built something that will last."

---

## 3. Energy Nexus Types

### Each Nexus Type's Player Fantasy

#### Coal / Plasma Nexus (Starter)

**Fantasy:** "The Reliable Workhorse"

**Feel:**
- Cheap to build, cheap to operate
- Visually: dark industrial structure with amber/orange glow from furnace
- Steady output, no surprises
- Significant contamination plume (visible, affects neighbors)

**Player Experience:**
- First nexus type available
- Easy decision early game
- Creates future guilt ("I should transition to cleaner energy")
- Contamination creates visible consequence of cheap choices

**Design Notes:**
- High contamination radius
- Reliable 100% efficiency when maintained
- Relatively short lifespan (encourages replacement)
- Distinct industrial audio cue (mechanical hum)

---

#### Oil / Petroleum Nexus (Early-Mid)

**Fantasy:** "The Efficient Compromise"

**Feel:**
- More expensive than coal, but more efficient per credit
- Visually: refinery aesthetic with amber/green glow
- Higher output per footprint
- Moderate contamination (less visible than coal)

**Player Experience:**
- Upgrade path from coal
- "I'm doing better" feeling
- Still dirty, but less guilty
- Good mid-game workhorse

**Design Notes:**
- Moderate contamination radius
- Better credits-per-energy ratio than coal
- Longer lifespan than coal
- Price point encourages mid-game transition

---

#### Gas / Methane Nexus (Mid Game)

**Fantasy:** "The Cleaner Path"

**Feel:**
- Significantly cleaner than coal/oil
- Visually: sleek structure with blue-white flame glow
- Modern, transitional technology
- Minimal visible contamination

**Player Experience:**
- First "clean-ish" option
- Stepping stone to renewables
- Feels like progress
- Price point makes it a conscious investment

**Design Notes:**
- Low contamination (barely visible)
- Higher build cost, lower operating cost
- Good efficiency rating
- Bridge between fossil and renewable

---

#### Nuclear / Fusion Nexus (Mid-Late)

**Fantasy:** "The Ambitious Investment"

**Feel:**
- Massive output, massive investment
- Visually: imposing structure with eerie blue-green glow
- Zero contamination during normal operation
- Rare catastrophic failure potential (core breach)

**Player Experience:**
- Major infrastructure decision
- High risk, high reward tension
- Pride in successful operation
- Fear of meltdown creates ongoing engagement
- Prestige structure (shows colony advancement)

**Design Notes:**
- Zero contamination normally
- Very high output (powers large districts)
- Highest build cost
- **Core breach risk** - rare but catastrophic (see Disasters, Epic 13)
- Long lifespan
- Requires player attention (efficiency monitoring)

**Experience Question:** Should core breach be a random disaster or tied to player neglect? Recommend: primarily neglect-based (aging + poor funding = risk), with rare random events for drama.

---

#### Wind Turbines (Renewable)

**Fantasy:** "The Clean Array"

**Feel:**
- Zero contamination, zero fuel cost
- Visually: tall graceful structures with rotating blades, soft cyan glow
- Variable output based on conditions
- Requires multiple units for significant energy

**Player Experience:**
- Feels virtuous and clean
- Requires spatial planning (multiple turbines needed)
- Weather dependency creates engagement
- Aesthetic appeal (looks beautiful on ridges)

**Design Notes:**
- Zero contamination
- Variable output (50-100% based on conditions)
- Low individual output (need farms)
- Best placed on ridges (elevation bonus?)
- No fuel cost, only maintenance

---

#### Solar Collectors (Renewable)

**Fantasy:** "The Silent Field"

**Feel:**
- Zero contamination, zero fuel cost
- Visually: fields of panels with soft golden/amber glow (catching alien light)
- Day/night cycle dependency (if implemented)
- Large footprint, consistent output

**Player Experience:**
- Set-and-forget reliability
- Spatial tradeoff (uses land that could be zoned)
- Clean conscience
- Pairs well with storage (future tech?)

**Design Notes:**
- Zero contamination
- Moderate output per panel
- Large footprint
- Day/night variation (if day/night exists) OR consistent alien world
- Good sector value (clean, quiet)

---

#### Hydro / Geothermal (Terrain-Dependent)

**Fantasy:** "Harnessing the Planet"

**Feel:**
- Zero contamination, terrain-dependent
- Visually: integrated with environment (dam structures, volcanic vents)
- High output, limited placement
- Permanent infrastructure

**Player Experience:**
- Strategic placement decisions early game
- Reward for terrain awareness
- Pride in utilizing natural features
- Creates iconic landmarks

**Design Notes:**
- Hydro: requires river/flow_channel proximity, high output
- Geothermal: requires ember_crust/volcanic terrain, very high output
- Zero contamination
- High upfront cost, very low operating cost
- Limited placement creates strategic value

---

#### Future Tech (Microwave Receiver, Fusion) (Late Game)

**Fantasy:** "The Civilization Achievement"

**Feel:**
- Massive output, extremely expensive
- Visually: alien megastructures with brilliant multi-color glow
- Zero contamination
- End-game prestige

**Player Experience:**
- Ultimate energy goal
- Civilization milestone moment
- Flex on other overseers
- Frees player from energy concerns entirely

**Design Notes:**
- Unlocked via progression system (Epic 14)
- Extremely high output
- Very high build/maintenance cost
- Zero contamination
- Visual spectacle

---

## 4. Grid Collapse Experience

### What Happens When Energy Runs Out

Energy failure should be **dramatic but not devastating**. It's a crisis to overcome, not a game over.

#### Warning Phase: "Energy Deficit Approaching"

**Triggers:** Energy consumption reaches 80% of generation
**Duration:** Ongoing until resolved or escalation

**Player Experience:**
- Amber warning indicators on energy overlay
- UI notification: "Energy matrix strained"
- Energy nexus structures show stress (warning lights, faster glow pulse)
- Subtle audio cue (rising tension tone)

**What Players Should Feel:** "I need to address this soon, but I have time."

**Visual Feedback:**
- Energy conduits shift from cyan to amber glow
- Energy nexuses show warning indicators
- Overlay highlights structures at risk

---

#### Energy Deficit Phase: "Partial Collapse"

**Triggers:** Energy consumption exceeds generation
**Duration:** Continuous until generation restored

**How It Works (Pool Model):**
1. System calculates energy shortfall
2. Low-priority structures lose energy first
3. High-priority structures (medical nexus, enforcer post) stay powered longer
4. Priority list determines order of darkness

**Priority Tiers:**
| Priority | Structures | Rationale |
|----------|-----------|-----------|
| 1 (Critical) | Medical nexus, command nexus | Life support, essential services |
| 2 (High) | Enforcer post, hazard response | Safety services |
| 3 (Normal) | Exchange, fabrication buildings | Economy |
| 4 (Low) | Habitation buildings | Beings can survive short outages |

**Player Experience:**
- Some structures go dark, others stay lit
- Clear visual pattern (low-priority areas dark first)
- Economy impacts (unpowered buildings don't function)
- Beings become unhappy (harmony drops)

**Visual Feedback:**
- Unpowered structures lose bioluminescent glow
- Dark structures become "voids" against glowing neighbors
- Energy conduits in deficit zones dim or flicker
- Red warning pulses on affected areas

---

#### Grid Collapse Phase: "Total Blackout"

**Triggers:** Energy deficit reaches critical level (all non-critical structures unpowered)
**Duration:** Until significant generation restored

**Player Experience:**
- Nearly all structures dark
- Colony appears "dead"
- Dramatic visual moment
- Clear audio cue (silence, then warning tone)
- All economic activity paused
- Population suffers (harmony plummets)

**Visual Feedback:**
- Entire colony goes dark except command nexus
- Conduits completely dim
- Only ambient terrain bioluminescence remains
- Stark contrast with any powered rival colonies visible

**What Players Should Feel:** "This is a crisis, but I can fix this."

---

#### Recovery Phase: "Bringing the Matrix Online"

**How Recovery Works:**
1. Player builds new nexus OR repairs existing
2. Energy pool increases
3. Structures power on in priority order (reverse of shutdown)
4. Visual "relight" effect cascades through colony

**Player Experience:**
- Satisfying wave of illumination
- Priority structures first, then spreading outward
- Audio cue: rising tone, then "online" confirmation
- Economy resumes
- Harmony begins recovery

**Visual Feedback:**
- Conduits re-illuminate in sequence
- Structures "wake up" with glow animation
- Color shifts back from red/amber to healthy cyan/green
- Progressive relight creates wave effect

**Design Principle:** Recovery should feel like the colony is "waking up." The visual effect of bioluminescence returning should be as satisfying as watching a city light up at night from an airplane.

---

## 5. Energy Conduit Feel

### How Placing Conduits Should Feel

Energy conduits are not just infrastructure - they're **the veins of your colony**, visible and beautiful.

#### Placement Experience

**What It Should Feel Like:**
- Drawing glowing lines across your domain
- Extending your colony's lifeblood
- Strategic puzzle (optimal coverage)
- Visual satisfaction (clean lines, good coverage)

**Feedback During Placement:**
- Preview shows coverage zone expansion
- Highlight tiles that will gain energy coverage
- Clear indication when reaching unpowered zones
- Cost preview per segment

**Feedback After Placement:**
- Immediate glow effect
- Conduit "activates" with pulse animation
- Connected structures illuminate if energy pool allows
- Audio: electrical hum, connection confirmation

#### Coverage Zone Visualization

**Energy Overlay Mode:**
- Powered tiles: bright glow
- In coverage but unpowered (pool deficit): dim glow, amber warning
- No coverage: dark
- Conduit reach: radiating light pattern

**Key Design Rule:** Players should ALWAYS be able to see their coverage at a glance. Energy overlay is essential, not optional.

#### Cost-Benefit Decisions

**Strategic Choices:**
- Conduit cost vs. zone extension
- Direct routes vs. aesthetic placement
- Redundancy (multiple paths) vs. efficiency
- Reaching distant zones vs. developing nearby

**Player Experience:** Conduit placement should feel like puzzle-solving, not busywork. Each placement should involve a meaningful decision about colony layout.

#### Visual Design

**Bioluminescent Art Integration:**
- Conduits are glowing lines (cyan/teal default)
- Color shifts with load (green = healthy, amber = strained, red = deficit)
- Subtle pulse animation (energy flowing)
- Height variation follows terrain
- Connect visually to nexus structures

---

## 6. Multiplayer Dynamics

### Energy in a Multiplayer Context

#### Per-Overseer Energy Pools (No Sharing Infrastructure)

**Design Rationale:**
Energy infrastructure does NOT connect across overseer boundaries. Each overseer maintains their own energy matrix. This creates:
- Clear responsibility (your grid, your problem)
- No griefing via energy denial
- Independent strategic decisions
- Visual territory boundaries (your glow vs. their glow)

**Player Experience:**
- Self-reliance (build your own infrastructure)
- No "energy leech" strategies
- Each colony's glow is distinct
- Competition through comparison, not sabotage

#### Observing Rival Energy States

**Schadenfreude Potential:**
When a rival colony suffers grid collapse, you can SEE it. Their settlement goes dark while yours glows. This creates:
- Natural competitive feedback
- "I'm doing better" satisfaction
- Story moments ("Remember when Player 2's colony went dark for three cycles?")
- Pressure to maintain your own grid

**Visual Feedback:**
- Rival colonies' glow state visible from your domain
- Clear energy status difference
- No hidden information (energy is visible)

#### Energy Trading via Bank

**Mechanism (Abstract, Not Infrastructure):**
- Trade energy credits for credits via bank/market
- Separate from physical infrastructure
- No direct conduit connection between overseers
- Emergency energy purchase option

**Player Experience:**
- Can recover from deficit without building (at cost)
- Market dynamics (supply/demand pricing?)
- Cooperation through trade
- Not mandatory - self-sufficient players can ignore

**Design Note:** Energy trading should be a convenience, not a requirement. Players should be able to succeed without ever trading energy.

#### Contamination Cross-Boundary Effects

**Dirty Energy Affects Neighbors:**
- Contamination from coal/oil nexuses spreads
- Can cross overseer boundaries
- Creates tension between neighbors
- Incentivizes clean energy (or at least placement awareness)

**Player Experience:**
- "Your dirty nexus is polluting my district!"
- Placement strategy (put dirty nexuses away from borders)
- Diplomatic tension point
- Cooperation: "We should all go clean"

**Design Question:** Should contamination from rival nexuses be a minor annoyance or a significant penalty? Recommend: noticeable but not crippling. Creates conversation, not rage-quits.

---

## 7. Bioluminescent Visual Integration

### How Energy Ties Into Art Direction

The bioluminescent art direction and energy system are **perfectly aligned**. Energy literally powers the glow. This creates intuitive visual feedback.

#### Powered Structures: Alive with Light

**Visual Language:**
- Powered structures emit soft bioluminescent glow
- Glow color varies by structure type:
  - Habitation: soft blue-green
  - Exchange: warm amber
  - Fabrication: industrial orange
  - Services: white-cyan
- Active structures have subtle pulse animation
- Energy conduits provide visual connectivity

**Player Experience:**
- Healthy colony = beautiful glowing settlement
- Pride in visual appearance
- Clear feedback on colony state
- Screenshots look amazing

#### Unpowered Structures: Dark Voids

**Visual Language:**
- Unpowered structures lose glow entirely
- Dark silhouettes against glowing neighbors
- Feeling of "dead" zones
- Strong contrast creates visual urgency

**Player Experience:**
- Immediate recognition of problem areas
- Motivation to restore energy
- Emotional response (loss, urgency)
- "My colony is sick"

**Design Principle:** The contrast between powered and unpowered should be STARK. A player should never need the overlay to know something is unpowered - they can see it.

#### Energy Conduits as Glowing Lines

**Visual Language:**
- Conduits are visible glowing lines across terrain
- Cyan/teal base color (matches bioluminescent theme)
- Color shifts with load state
- Subtle energy pulse animation (direction toward consumers)
- Height follows terrain elevation

**Player Experience:**
- Infrastructure becomes decorative
- Conduit networks are visually appealing
- Energy "flow" is readable
- Integrates with alien landscape

#### Nexus Types with Distinct Glow Colors

**Visual Differentiation:**
| Nexus Type | Glow Color | Character |
|------------|------------|-----------|
| Coal/Plasma | Amber/Orange | Industrial, warm, harsh |
| Oil/Petroleum | Amber/Green | Refinery, moderate |
| Gas/Methane | Blue-White | Clean, modern |
| Nuclear/Fusion | Blue-Green | Eerie, powerful |
| Wind | Soft Cyan | Natural, subtle |
| Solar | Golden/Amber | Warm, steady |
| Hydro | Deep Blue | Water-integrated |
| Geothermal | Orange/Red | Volcanic, intense |
| Future Tech | Multi-spectrum | Alien, spectacular |

**Player Experience:**
- Each nexus type visually distinct
- Energy source identifiable at a glance
- Colony energy mix visible in color palette
- Clean vs. dirty immediately readable

---

## 8. Pacing and Progression

### When Does Energy Become Relevant?

#### Immediate Need (Cycle 0+)

**First Structures Require Energy:**
- Zoned buildings cannot develop without energy coverage
- First nexus placement is early tutorial
- Energy is not an optional system
- Clear "before/after" moment when nexus comes online

**Player Experience:**
- "I need to build an energy nexus to grow"
- Clear cause-effect relationship
- Immediate feedback when energy available
- Foundation-level importance established

#### Growing Demand (Cycles 10-50)

**Expansion Creates Pressure:**
- Each new structure increases demand
- Zone development outpaces initial nexus capacity
- Second nexus decision point
- Coverage expansion needed

**Player Experience:**
- "I'm growing, I need more energy"
- Strategic decision: nexus type choice
- Conduit network expansion
- First brush with energy constraints

#### Mid-Game Management (Cycles 50-150)

**Infrastructure Maturity:**
- Multiple nexuses in operation
- Diverse energy mix possible
- Conduit network optimization
- First nexus aging events

**Player Experience:**
- Energy infrastructure requires attention
- Replacement/upgrade decisions
- Clean energy transition opportunity
- Coverage optimization mini-game

#### Late-Game Optimization (Cycles 150+)

**Legacy and Efficiency:**
- Aging plants require replacement decisions
- Clean energy as prestige goal
- Excess capacity as luxury
- Energy matrix "finished" state possible

**Player Experience:**
- Pride in efficient, clean energy matrix
- Decommissioning dirty plants satisfaction
- Optimization for aesthetics and efficiency
- Energy becomes "solved" - focus shifts elsewhere

---

## 9. Key Design Work Items

### Design Deliverables Needed for Epic 5

#### Nexus Type Specifications

**Deliverable:** Detailed spec for each nexus type including:
- Build cost (credits)
- Operating cost (credits/cycle)
- Energy output (units/tick)
- Contamination output (units/tick, radius)
- Footprint (tiles)
- Lifespan (cycles before efficiency degradation)
- Efficiency curve (age vs. output)
- Visual design brief
- Audio design brief

**Priority:** High - Required for implementation

---

#### Energy Deficit Tier Definitions

**Deliverable:** Precise definitions for:
- Warning threshold (% of capacity)
- Deficit tiers and priority order
- Structure priority classifications
- Recovery thresholds
- Timing (how quickly does deficit escalate)

**Priority:** High - Core system behavior

---

#### Visual Feedback Specifications

**Deliverable:** Detailed visual design specs for:
- Powered vs. unpowered structure appearance
- Conduit states (healthy, strained, deficit)
- Overlay layer design
- Warning indicators
- Recovery animation sequence
- Nexus type visual differentiation

**Priority:** High - Critical for player feedback

---

#### Sound Design Recommendations

**Deliverable:** Audio design direction for:
- Nexus ambient sounds (per type)
- Conduit hum (electrical ambiance)
- Warning alerts (approaching deficit, deficit, collapse)
- Recovery audio (systems coming online)
- UI audio (placement, connection)

**Priority:** Medium - Important for feel, not blocking

---

#### Coverage Algorithm Requirements

**Deliverable:** Gameplay requirements for:
- Conduit coverage radius
- Coverage falloff (if any)
- Coverage overlap rules
- Coverage visualization requirements

**Priority:** High - Required for systems architect

---

## 10. Questions for Other Agents

### Cross-Agent Clarifications Needed

**@systems-architect:** Coverage zone calculation approach
- How will conduit coverage be calculated? (radius-based, pathfinding-based, flood fill?)
- What's the performance budget for coverage recalculation on conduit placement?
- Should coverage update immediately or queue for next tick?
- How do we handle coverage for large conduit networks efficiently?

**@systems-architect:** Pool calculation timing
- When in the tick order should energy pool be recalculated?
- Should energy distribution happen before or after building state updates?
- How do we handle the case where a nexus goes offline mid-tick?
- What's the strategy for handling simultaneous producer/consumer changes?

**@systems-architect:** Priority system implementation
- Should priority be a component field or derived from building type?
- How do we handle priority ties during deficit?
- Should players be able to set manual priorities?

**@services-engineer:** Energy priority for service buildings
- What priority tier should each service building type have?
- Should priority be configurable by the player for service buildings?
- How does funding level affect energy priority (if at all)?

**@systems-architect:** Multiplayer energy sync
- How frequently should energy pool state sync?
- Should rival energy state be synced (for visual observation)?
- What happens to energy state on reconnect?

---

## 11. Risks and Concerns

### Player Experience Risks

#### "Nothing Is Happening" Syndrome

**Risk:** When energy is working fine, there's no feedback or engagement.

**Mitigation:**
- Subtle ongoing visual feedback (glow, pulse)
- Nexus efficiency readouts (gives something to monitor)
- Occasional maintenance events (small engagement points)
- Historical tracking (energy graphs, trends)

**Design Principle:** Even stable systems should provide ambient feedback.

---

#### Grid Collapse Frustration

**Risk:** Players feel punished unfairly when grid collapses.

**Mitigation:**
- Clear, escalating warnings
- No permanent loss (buildings pause, don't die)
- Multiple recovery paths (build, buy, trade)
- Tutorial on energy management
- Never surprise collapse (always warning phase first)

**Design Principle:** Grid collapse should feel like a challenge to overcome, not a punishment.

---

#### Conduit Placement Tedium

**Risk:** Extending conduits across the map becomes boring busywork.

**Mitigation:**
- Coverage radius large enough to require less density
- "Auto-extend" tool option for long runs
- Visual satisfaction in placement (glow feedback)
- Strategic placement decisions (not just spam everywhere)
- Upgrade paths (higher coverage conduits later?)

**Design Principle:** Infrastructure should involve decisions, not repetitive clicking.

---

#### Early Game Energy Lock

**Risk:** New players get stuck without understanding energy.

**Mitigation:**
- Clear tutorial/onboarding for first nexus
- UI indicators showing unpowered structures
- Advisor tips when buildings not developing
- Cannot accidentally ignore energy (structures won't work)

**Design Principle:** Energy should be taught, not discovered through failure.

---

#### Clean Energy Too Weak/Strong

**Risk:** Clean energy either trivializes dirty options or is never worth the cost.

**Mitigation:**
- Balance pass during implementation
- Clean energy = higher upfront, lower operating
- Dirty energy = lower upfront, ongoing contamination cost
- Late-game efficiency bonuses for clean
- Prestige incentive (visual, achievement)

**Design Principle:** Both paths should be viable, with different tradeoffs.

---

#### Multiplayer Contamination Grief

**Risk:** Players deliberately place dirty nexuses to contaminate rivals.

**Mitigation:**
- Contamination spread limited (not infinite)
- Contamination primarily affects the source overseer
- Cross-boundary contamination is noticeable but not devastating
- Social enforcement (toxic players get reputation)

**Design Principle:** Contamination creates interesting tension, not griefing tools.

---

## Summary

Energy infrastructure is the **first major tension system** in ZergCity. It transforms the peaceful act of zoning into a living colony that requires care and attention. The bioluminescent art direction and energy mechanics are perfectly aligned - energy literally powers the visual identity of the colony.

**Key Experience Goals:**
1. Energy should feel like the lifeblood of the colony
2. Grid collapse is dramatic but never devastating
3. Recovery is satisfying, not punitive
4. Visual feedback is immediate and intuitive
5. Strategic decisions create meaningful choices

**Design Priorities:**
1. Clear visual feedback (powered vs. unpowered)
2. Escalating warnings before collapse
3. Multiple nexus types for strategic variety
4. Conduit placement as spatial puzzle
5. Clean vs. dirty energy as meaningful progression

The energy matrix should make players feel like they're maintaining something alive - when it thrives, they feel proud; when it struggles, they feel challenged; when it fails, they feel motivated to recover. This is the emotional heart of the energy system.

---

*Document prepared by Game Designer Agent for EPIC 5 planning phase.*
