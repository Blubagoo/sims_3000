# Game Designer Analysis: Epic 8 - Ports & External Connections

**Author:** Game Designer Agent
**Date:** 2026-01-29
**Canon Version:** 0.13.0
**Epic:** 8 - Ports & External Connections
**Reference:** Epic 7 (TransportSystem) as integration point, Epic 4 (ZoneSystem) for zoning patterns

---

## Experience Goals

### What Should Ports Feel Like?

**Core Fantasy: The Gateway to Beyond**

Your colony isn't alone in the void. Aero ports and aqua ports are your gateways to the wider universe -- they represent connection to civilizations beyond your map's edge. While pathways connect you to neighbors you can see, ports connect you to a vast economy you can only imagine.

**Experiential Pillars:**

1. **Opening the World**
   - Ports transform your colony from isolated settlement to connected hub
   - The first port feels like "putting your colony on the map"
   - External connections create opportunity (demand boosts, trade income)
   - "We're part of something bigger now"

2. **Infrastructure as Statement**
   - Ports are LARGE -- they're visible from anywhere on the map
   - An aero port runway cuts across your terrain like a scar of progress
   - An aqua port's docks reach into the water like fingers grasping for trade
   - These aren't hidden infrastructure -- they're landmarks

3. **Zone Mastery Culmination**
   - Ports are ZONED, not placed as buildings -- requires space planning mastery
   - Aero port requires long runway zones -- finding that space is a challenge
   - Aqua port requires waterfront zones -- coastline becomes precious
   - "I planned for this from the beginning" vs. "I wish I'd left space"

4. **Economic Engine**
   - Ports boost zone_pressure (demand) -- aero for exchange, aqua for fabrication
   - External trade brings credits without local taxation burden
   - High-reward but high-investment infrastructure
   - "My port paid for itself"

**The Port Fantasy vs. Other Infrastructure:**

| System | Feel | Player Role |
|--------|------|-------------|
| Energy | Industrial power | Power plant operator |
| Fluid | Life sustenance | Resource manager |
| Pathways | Local connectivity | Network architect |
| **Ports** | Global connectivity | **Trade mogul** |

Ports add a new dimension: connection to the OFF-MAP world. Everything else is about your colony internally. Ports are about your colony's place in the universe.

---

## Player Decisions

### What Meaningful Choices Do Ports Create?

#### Decision 1: When to Invest

**The Timing Question:**
Ports are expensive, require significant land, and take time to develop. When is the right time?

**Trade-offs:**
- **Early port** = sacrifice prime land but gain early demand boost
- **Late port** = land already claimed, may need to demolish for runway space
- **No port** = viable strategy for smaller colonies, rely on local economy

**Player Experience:**
- "Should I reserve waterfront for aqua port or develop habitation there?"
- "My colony is big enough for an aero port now, but I'd need to demolish that exchange district..."
- "I'll focus on local growth instead of ports this game"

---

#### Decision 2: Port Type Selection

**Aero Port vs. Aqua Port:**
Different boost targets, different terrain requirements, different feels.

| Port Type | Demand Boost | Terrain Requirement | Feel |
|-----------|--------------|---------------------|------|
| Aero Port | Exchange (commercial) | Large flat zone for runway | High-tech, prestigious |
| Aqua Port | Fabrication (industrial) | Waterfront access | Trade-focused, gritty |

**Trade-offs:**
- **Aero port** boosts exchange demand -- good for wealthy, service-oriented colonies
- **Aqua port** boosts fabrication demand -- good for industrial, manufacturing colonies
- **Both** possible but requires massive investment and space

**Player Experience:**
- "My colony is exchange-heavy, I need an aero port"
- "All this waterfront would be perfect for aqua port docks"
- "I'm going to build both -- become a true trade hub"

---

#### Decision 3: Location Selection

**Where to Place the Port:**

**Aero Port Location:**
- Needs LONG FLAT ZONE for runway (6+ tiles minimum?)
- Ideally away from high-density habitation (noise/contamination?)
- But needs pathway access for workers and travelers
- Creates exclusion zone around runway (no tall structures?)

**Aqua Port Location:**
- Must be adjacent to water (deep_void, still_basin, or flow_channel)
- Consumes valuable waterfront real estate
- Competes with habitation zones wanting water view (sector_value bonus)
- Dock extends into water -- visual landmark

**Player Experience:**
- "I need to find space for a runway without demolishing too much"
- "That coastline is prime real estate... but perfect for aqua port"
- "I'll put the aero port on the edge of my territory"

---

#### Decision 4: External Connection Strategy

**Neighbor Connections (Map Edge):**
These are connections to AI or player neighbors at the map edge.

**Connection Types:**
- **Pathway connections** -- extend pathways to map edge for trade/migration
- **Rail connections** -- extend rail_lines for high-capacity external link
- **Energy connections** -- sell/buy energy from neighbors (optional)

**Trade-offs:**
- **More connections** = more trade income, more migration, more exposure
- **Fewer connections** = isolated, self-sufficient, protected

**Player Experience:**
- "Should I connect my pathways to the edge for migration?"
- "That neighbor is offering good trade deals -- I should connect"
- "I'll stay isolated and not deal with external problems"

---

#### Decision 5: Trade Deal Management

**If Trade Mechanics Are Deep:**
Active management of import/export deals with external entities.

**Potential Decisions:**
- Accept trade deal that brings credits but increases flow (pathway load)
- Export resources (energy surplus?) for credits
- Import resources at premium during shortage
- Negotiate with AI neighbors for favorable terms

**If Trade Mechanics Are Simple:**
Passive income based on port capacity and external connection count.

**Design Question:** How deep should trade be? (See Questions for Other Agents)

---

## Aero Port vs. Aqua Port

### How Do These Feel Different?

#### Aero Port (Airport) Fantasy

**The Vision:**
Your colony has achieved flight. Vessels descend from the sky, carrying visitors and goods from distant worlds. The runway stretches across your terrain -- a bold statement of technological achievement.

**Emotional Tone:**
- Prestigious and high-tech
- "We've arrived as a civilization"
- Clean, modern, future-focused
- Exchange (commercial) association -- business travelers, tourism

**Visual Identity:**
- LONG runway cutting across terrain
- Terminal building with distinctive architecture
- Vessels (alien aircraft) taking off and landing
- Bright beacon lights, landing path indicators
- Cyan/white glow theme (high-tech)

**Sound Identity:**
- Vessel approach sound (ascending whine)
- Landing/takeoff sound events
- Terminal ambient bustle

**Gameplay Feel:**
- Major land commitment (runway length)
- Placement puzzle: finding flat terrain for runway
- Exclusion zone prevents tall structures nearby
- Boosts exchange demand ("business travelers want shops and services")

**Player Statements:**
- "My aero port is the jewel of my colony"
- "I had to demolish a whole district for that runway"
- "Exchange is booming since the aero port opened"

---

#### Aqua Port (Seaport) Fantasy

**The Vision:**
Ships arrive from across the waters, laden with raw materials and manufactured goods. Cranes move cargo, docks bustle with activity. Your colony has become a trade hub.

**Emotional Tone:**
- Industrial and practical
- "We're open for business"
- Gritty, working-class, trade-focused
- Fabrication (industrial) association -- raw materials, exports

**Visual Identity:**
- Docks extending into water
- Cranes and cargo handling equipment
- Ships (alien vessels) arriving and departing
- Warehouses and storage along waterfront
- Amber/orange glow theme (industrial warmth)

**Sound Identity:**
- Ship horn sounds (arrival/departure)
- Crane operation ambient
- Waterfront industrial bustle

**Gameplay Feel:**
- Waterfront commitment (precious coastline)
- Placement puzzle: coastline access vs. habitation
- Dock structures extend into water visually
- Boosts fabrication demand ("raw materials need factories")

**Player Statements:**
- "My aqua port handles all the trade for this region"
- "I gave up waterfront habitation for those docks"
- "Fabrication is booming since the aqua port opened"

---

### Side-by-Side Comparison

| Aspect | Aero Port | Aqua Port |
|--------|-----------|-----------|
| Terrain Need | Large flat zone | Waterfront access |
| Demand Boost | Exchange | Fabrication |
| Visual Theme | High-tech, clean | Industrial, gritty |
| Color Scheme | Cyan/white | Amber/orange |
| Sound Theme | Vessel engines | Ship horns, cranes |
| Player Role | Prestige builder | Trade mogul |
| Challenge | Finding runway space | Sacrificing coastline |
| Placement Puzzle | Long flat area | Water adjacency |

**Design Principle:** These should feel like distinctly different infrastructure with different gameplay implications. Not just "port A" and "port B" but genuinely different experiences.

---

## Neighbor Connections Experience

### What's the Fantasy Here?

**The Question:** Are neighbors AI-controlled settlements or other players?

#### If AI Neighbors (Single-Player / Solo Sandbox)

**Fantasy:** Your colony exists in a populated universe with other civilizations nearby.

**Experience:**
- Edge connections lead to "off-map" AI settlements
- AI settlements have personalities (trade-focused, isolationist, etc.)
- Trade deals negotiated with AI
- Migration flows based on relative colony quality
- "My neighbor is thriving, beings are migrating there"

**Interaction Types:**
- **Trade agreements** -- passive income, resource exchange
- **Migration effects** -- colony quality affects population flow
- **Diplomacy light** -- friendly vs. hostile AI neighbors
- **Shared disasters?** -- problems that cross boundaries

---

#### If Real Player Neighbors (Multiplayer)

**Fantasy:** Your colony is part of a shared world with friends.

**Experience:**
- Edge connections lead to other player colonies
- Real player decisions affect your colony
- Trade/migration between actual players
- "My friend's colony is connected to mine -- we share beings"

**Current Canon Note:**
Per canon, this is a multiplayer sandbox with 2-4 players on a shared map. So "neighbors" in the traditional off-map sense may be AI, while on-map connections are real players via pathways.

**Clarification Needed:** Are "external connections" at map edge connecting to:
- AI settlements off-map (persistent trade partners)?
- Other game instances (cross-server play)?
- Simply representing "the wider universe" abstractly?

---

### External Connections at Map Edge

**The Mechanic:**
Players can extend pathways/rail to the map edge to create "external connections."

**Experience Goals:**

1. **Symbolic Gateway**
   - The connection to map edge represents link to outside world
   - Visual indicator at map edge (gateway structure?)
   - "This pathway leads to... somewhere else"

2. **Trade Income Source**
   - External connections generate passive trade income
   - More connections = more trade
   - Encourages building to map edges

3. **Migration Channel**
   - Beings can migrate in/out through external connections
   - Colony quality affects migration direction
   - High harmony = immigration; low harmony = emigration

4. **Strategic Positioning**
   - Controlling map edge positions = trade advantage
   - Competition for edge access in multiplayer?
   - "Gateway" territories become valuable

---

### Migration Effects

**How Should External Migration Feel?**

**Immigration (Beings Moving In):**
- Positive indicator: your colony is attractive
- Boosts colony_population without waiting for births
- Comes through external connections (ports, edge pathways)
- "Beings are arriving from beyond!"

**Emigration (Beings Moving Out):**
- Warning indicator: your colony has problems
- Reduces colony_population gradually
- Happens when harmony is low, contamination is high, etc.
- "Beings are leaving for better opportunities..."

**Player Experience:**
- Immigration feels rewarding ("My colony is desirable")
- Emigration feels like wake-up call ("I need to fix things")
- External connections amplify both effects

---

## Multiplayer Dynamics

### How Do Ports Affect Player Interactions?

#### Scenario 1: Competition for Waterfront

**The Dynamic:**
Aqua ports require waterfront access. If multiple players share a coastline, there's competition for aqua port zones.

**Experience:**
- Early game: "I should claim that coastline before they do"
- Mid game: "They got the best aqua port location, I'll need to find another spot"
- Late game: "I control the aqua port, they have to come to me for fabrication boost"

**Design Consideration:**
Waterfront is already valuable (sector_value bonus). Adding aqua port value increases competition.

---

#### Scenario 2: Shared Aero Port?

**The Question:**
Can/should players share a single aero port? Or must each build their own?

**If Shared:**
- Cooperative investment opportunity
- "Let's build one aero port together and share the benefits"
- Runway crosses ownership boundaries?
- Shared costs, shared demand boost

**If Separate:**
- Each player needs own aero port
- Competition for limited flat terrain
- Duplication of infrastructure

**Design Consideration:** Shared ports could create interesting cooperation moments but add complexity. Separate ports are simpler but may not fit on small maps.

---

#### Scenario 3: Inter-Player Trade

**The Mechanic:**
Per epic-plan.md: "Multiplayer: Inter-player trade/connections"

**Potential Implementation:**
- Players can create trade deals with each other
- Exchange credits for resources (energy, materials?)
- Ports facilitate higher-volume trade
- "I'll sell you my excess energy for credits"

**Experience:**
- Creates economic interdependence
- Negotiation moments in gameplay
- Can help struggling players (or exploit them?)
- "I need energy -- what's your price?"

**Design Consideration:** Trade adds depth but also complexity and griefing potential. (See Risks section)

---

#### Scenario 4: External Connection Competition

**The Dynamic:**
If external connections (to AI/off-map) provide benefits, players may compete for map edge positions.

**Experience:**
- "I'm going to build to the map edge first for that trade income"
- Strategic territory planning includes edge access
- "Gateway" tiles become valuable

**Design Consideration:** Map edge value adds another dimension to territory competition.

---

### Multiplayer Trade Mechanics

**The Deep Question:** How deep should inter-player trade be?

**Option A: No Direct Trade**
- Players have separate economies
- Ports affect only own demand/income
- Simpler, less interaction

**Option B: Passive Trade**
- Connected players automatically share some economic benefits
- Ports near ownership boundaries create "trade zones"
- No active management required

**Option C: Active Trade**
- Players negotiate trade deals
- Buy/sell resources (energy, credits, materials?)
- Ports increase trade capacity
- Active management, more depth

**Recommendation:** Start with Option B (passive trade), consider Option C for later epic. Active trade adds significant complexity and potential for griefing.

---

## Questions for Other Agents

### @systems-architect

1. **Port Zone Implementation:**
   - Are aero_port and aqua_port new zone types in ZoneSystem, or a building type in BuildingSystem?
   - How do multi-tile port zones work? (Runway needs linear zone, dock needs waterfront zone)
   - Is there a minimum zone size for ports to function?

2. **External Connection Architecture:**
   - How do map-edge connections work technically?
   - Are external neighbors AI entities with state, or abstract trade sources?
   - How is external trade income calculated?

3. **Runway Exclusion Zones:**
   - Do aero ports create height restrictions for nearby structures?
   - How is this enforced in BuildingSystem?
   - What's the exclusion zone radius?

4. **Cross-Ownership Ports:**
   - Can a port zone span multiple player territories?
   - If yes, how is ownership and benefit sharing handled?
   - If no, what prevents edge-case placement?

5. **Port Capacity Model:**
   - Do ports have capacity limits (like energy pools)?
   - Does capacity affect demand boost amount?
   - Can ports become "congested"?

### @economy-engineer

1. **Demand Boost Calculation:**
   - How much does an aero port boost exchange demand numerically?
   - How much does an aqua port boost fabrication demand?
   - Is boost proportional to port size/capacity?

2. **Trade Income Model:**
   - How is external trade income calculated?
   - Does it depend on port capacity, connection count, or both?
   - Is trade income passive or activity-based?

3. **Port Operating Costs:**
   - What are ongoing maintenance costs for ports?
   - Do ports require energy? Fluid? Both?
   - Is there a break-even point for port investment?

4. **Inter-Player Trade Economics:**
   - How would trade deals between players work economically?
   - What can be traded? (Credits, resources, population?)
   - How do we prevent economic griefing?

### @infrastructure-engineer

1. **Runway Requirements:**
   - What's the minimum runway length for aero port function?
   - Do runways need to be perfectly straight?
   - Can runways cross pathways?

2. **Dock Requirements:**
   - What's the minimum dock size for aqua port function?
   - How do docks connect to water technically?
   - Do different water types (deep_void vs. still_basin) matter?

3. **Port Connections to Transport:**
   - How do ports connect to the pathway network?
   - Do port zones need internal pathway access?
   - Do ports generate flow on connected pathways?

4. **External Connection Points:**
   - How are map-edge connection points implemented?
   - Can any edge tile be a connection, or specific points?
   - Visual representation of edge connections?

---

## Risks & Concerns

### Risk 1: Ports as "Win Button"

**The Concern:**
If ports provide significant demand boosts, the first player to build a port could snowball advantage.

**Mitigation Ideas:**
- Port benefits scale with existing development (empty colony gets less benefit)
- High cost/space requirement prevents early rush
- Multiple port types prevent single-port dominance
- Demand boost has diminishing returns

---

### Risk 2: Waterfront Monopoly

**The Concern:**
In multiplayer, one player could claim all coastline, denying others aqua port access.

**Mitigation Ideas:**
- Multiple coastlines on most maps
- Alternative fabrication boost sources (without aqua port)
- Aqua port benefits accessible via trade with port-owning player
- Map generation ensures coast access for all spawn points

---

### Risk 3: Trade Griefing

**The Concern:**
Inter-player trade could enable economic griefing (predatory deals, market manipulation).

**Mitigation Ideas:**
- Trade deals are optional (no forced participation)
- Price limits or "fair market value" guidance
- Reputation/relationship effects for bad deals
- Option B (passive trade) avoids this entirely

---

### Risk 4: Port Space Regret

**The Concern:**
Players who develop their colony without reserving port space may feel punished later.

**Mitigation Ideas:**
- Clear early-game education about port space needs
- Port zones can overlap some zone types (exchange zones become terminal?)
- Smaller "regional port" option with reduced benefits
- Demolition + rebuild is valid late-game strategy

---

### Risk 5: External Connections Feel Abstract

**The Concern:**
If external connections just generate passive income without visible impact, they feel meaningless.

**Mitigation Ideas:**
- Visible ships/vessels arriving at ports
- Migration visuals (beings arriving through edge connections)
- News events about external trade ("Trade vessel arrived with goods!")
- External events affect your colony (off-map disasters reduce trade?)

---

### Risk 6: Multiplayer Balance with Asymmetric Port Access

**The Concern:**
Random map generation might give some players better port locations than others.

**Mitigation Ideas:**
- Map generation guarantees each player access to water and flat land
- Port benefits can be gained through connection to other players' ports
- Multiple valid strategies (not everyone needs own ports)
- Trade deals allow non-port players to benefit from ports

---

## Theme & Terminology

### Canonical Terms (from terminology.yaml)

**Always Use:**

| Human Concept | Canonical Term | Context |
|---------------|----------------|---------|
| Airport | Aero_port | Air transport facility |
| Seaport | Aqua_port | Water transport facility |
| Commercial | Exchange | Zone type boosted by aero_port |
| Industrial | Fabrication | Zone type boosted by aqua_port |
| Road | Pathway | Surface transport connections |
| Traffic | Flow | Movement metric |
| Population | Colony_population | Beings in your colony |
| Money | Credits | Currency |
| Neighbor | Adjacent_colony | External connection target |

**UI Text Examples:**
- "Aero port operational" (not "airport open")
- "Exchange zone_pressure increased by aero port" (not "commercial demand")
- "Aqua port receiving trade vessels" (not "seaport ships")
- "External connection to adjacent_colony established" (not "neighbor connected")

---

### New Terminology Proposals

**For Canon Update Consideration:**

| Concept | Proposed Term | Rationale |
|---------|---------------|-----------|
| Airplane | Aero_vessel | Alien sky transport |
| Ship | Aqua_vessel | Alien water transport |
| Runway | Launch_corridor | Alien take-off/landing zone |
| Dock | Mooring_platform | Alien ship berth |
| Trade deal | Exchange_pact | Alien trade agreement |
| Immigration | Influx | Beings arriving |
| Emigration | Exodus | Beings departing |
| Map edge | Boundary_threshold | Edge of known territory |
| External trade | Beyond_commerce | Trade with off-map entities |

**Visual Terminology:**
- Aero_vessels should look alien (not Earth airplanes)
- Aqua_vessels should look alien (not Earth ships)
- Port architecture should match bioluminescent aesthetic

---

### Thematic Consistency Notes

**Ports as Alien Infrastructure:**
These aren't human airports and seaports with alien paintjobs. They should feel fundamentally alien:

- **Aero ports:** Maybe vessels land vertically? Organic landing pads? Bioluminescent runway lights?
- **Aqua ports:** Docks could be living structures that grow? Cranes could be tentacle-like?
- **External beings:** Arrivals from off-map should look different (varied alien species?)

**Tone:**
Ports represent your colony's connection to the broader alien civilization. They should feel expansive and mysterious -- there's a whole universe out there.

---

## Pacing Recommendations

### When Do Players Typically Build Ports?

**Game Phase Analysis:**

| Phase | Port Relevance | Player Focus |
|-------|----------------|--------------|
| Early (Cycles 0-50) | None | Basic infrastructure, first zones |
| Mid-Early (Cycles 50-150) | Planning | Reserve space, consider locations |
| Mid (Cycles 150-300) | First Port | Major investment decision |
| Mid-Late (Cycles 300-500) | Expansion | Second port type, external connections |
| Late (Cycles 500+) | Optimization | Maximize port efficiency, trade mastery |

**Recommendation:** Ports are **mid-game infrastructure**. They require:
- Established economy to afford construction
- Developed zones to benefit from demand boost
- Enough colony size to justify external connections

**First Port Milestone:**
- Should feel like a major achievement
- "My colony is ready for external trade"
- Distinct milestone moment (celebration?)

---

### Ports in Session Pacing

**Within a Play Session:**

- Ports are long-term investments, not moment-to-moment decisions
- Once built, ports provide passive benefits
- Occasional trade deal decisions (if active trade implemented)
- Port watching can be relaxing (vessels arriving/departing)

**Emotional Beat:**
Ports provide a "plateau" in gameplay -- after the stress of building the port, enjoy the steady benefits.

---

## Summary

Epic 8 introduces **ports and external connections** -- your colony's gateway to the universe beyond the map edge.

**Key Experience Goals:**

1. **Opening the World** -- Ports connect your isolated colony to something bigger
2. **Infrastructure as Statement** -- Ports are landmarks, not hidden infrastructure
3. **Zone Mastery Culmination** -- Zoning strategy pays off (or creates regret)
4. **Economic Engine** -- Ports drive demand and generate trade income

**Core Distinctions:**

| Port Type | Demand Boost | Terrain | Feel |
|-----------|--------------|---------|------|
| Aero_port | Exchange | Flat runway | High-tech, prestigious |
| Aqua_port | Fabrication | Waterfront | Industrial, trade-focused |

**Multiplayer Dynamics:**
- Competition for prime port locations (waterfront, flat terrain)
- Potential for shared ports or inter-player trade
- External connections add strategic dimension to territory planning

**Design Principles:**

1. Ports should feel like **major achievements**, not incremental upgrades
2. Port placement is a **strategic puzzle** with lasting consequences
3. Aero and aqua ports should feel **distinctly different**, not interchangeable
4. External connections should feel **meaningful**, not abstract passive income
5. Multiplayer trade should **encourage cooperation** without enabling griefing

**The Feeling We Want:**

Building your first port should feel like your colony has "arrived" -- you're not just surviving, you're thriving and connected to the wider universe. The aero port is a prestige landmark that says "we're advanced." The aqua port is a trade hub that says "we're open for business." External connections represent mysterious opportunity and risk from beyond your borders.

The player should feel like a **trade mogul** and **gateway builder** -- someone who doesn't just build a colony, but connects it to something greater.

---

**Document Status:** Ready for Systems Architect and Economy Engineer review

**Next Steps:**
1. Systems Architect clarifies port zone implementation and external connection architecture
2. Economy Engineer defines demand boost values and trade income model
3. Resolve open questions via agent discussion
4. Determine scope for multiplayer trade features
5. Proceed to ticket creation
