# Game Designer Analysis: Epic 3 — Terrain System

**Agent:** Game Designer
**Epic:** 3 — Terrain System (Phase 1: Foundation)
**Canon Version:** 0.5.0
**Date:** 2026-01-28

---

## Experience Goals

Terrain is the canvas the overseer inherits. Before a single structure materializes, before a single being arrives, there is the world itself — dark, luminous, alive. The Terrain System must deliver three core emotional beats:

### 1. Wonder at First Sight
When a new region generates and the overseer sees it for the first time, the reaction should be: *"This is beautiful, and I want to explore it."* The bioluminescent art direction does heavy lifting here. The deep void glows faintly along the map edge. Prisma fields pulse with inner light. Flow channels trace bright ribbons across dark substrate. The world is not blank — it is already alive, already interesting, already telling a story. Every map should feel like a place that existed before the overseer arrived.

### 2. Strategic Excitement During Site Selection
The second beat is the mental shift from wonder to calculation: *"Where do I build?"* Terrain should force meaningful choices. Flat substrate near a flow channel is ideal for fluid extraction — but so is the spot your rival is eyeing. Those prisma fields would give a massive sector value bonus, but they are up on a ridge and far from fluid. The blight mires block expansion to the south, but beyond them lies untouched ember crust with geothermal energy potential. Every glance at the map should spark a plan.

### 3. Ownership Through Transformation
Late game, as overseers reshape the world — grading ridges, purging biolume groves, even terraforming ember crust — the terrain becomes *theirs*. The world they inherited has been claimed, reshaped, made into something new. This is the satisfaction of mastery: looking at the colony and knowing every sector was a deliberate choice.

### Emotional Palette
| Moment | Target Emotion |
|--------|---------------|
| First map reveal | Wonder, curiosity |
| Scanning terrain types | Strategic excitement |
| Choosing colony site | Commitment, anticipation |
| Discovering rival chose the same region | Tension, rivalry |
| Purging terrain for building | Calculated sacrifice |
| Viewing the colony from a ridge | Pride, ownership |
| Exploring unexplored corners late game | Discovery, surprise |

---

## Terrain Type Gameplay Analysis

### Classic Terrain

#### 1. Substrate (flat_ground)
**Canonical Term:** Substrate
**Visual:** Neutral gray-green, faint luminous veining
**Gameplay Effect:** Baseline buildable terrain — no bonus, no penalty
**Player Decisions:** "Do I build here because it is easy, or push for more interesting terrain with bonuses?"
**Fun Factor:** Low individual interest, but essential as the "safe" option. Substrate is the blank canvas that makes special terrain feel special. Without plentiful substrate, every placement becomes agonizing — we need it to be the comfortable default.
**Design Note:** Substrate should cover roughly 35-45% of any generated map. It is the "bread" of the terrain sandwich.

#### 2. Ridges (hills)
**Canonical Term:** Ridges
**Visual:** Elevated terrain with crystalline outcroppings that glow faintly; layered rock with glowing vein patterns
**Gameplay Effect:** Buildable but costs increase with elevation changes; slows transport routing
**Player Decisions:**
- "Do I grade this ridge flat (expensive) or build around it?"
- "A ridge-top colony has incredible views and natural defense but terrible pathway access."
- "Ridges between me and my rival create a natural border — do I want that?"
**Fun Factor:** HIGH. Ridges create the most interesting geography. They force creative routing, create natural neighborhoods, and provide dramatic viewpoints. The free camera (15-80 degrees pitch) means players will *actually see* elevation differences — a colony built on a ridge looks dramatically different from one in a valley.
**Design Note:** Ridges should use the full 32-elevation range to create gentle slopes, steep cliffs, and plateaus. Avoid purely random noise — ridges should feel geological, with ridgelines, saddle points, and valleys.

#### 3. Deep Void (water_ocean)
**Canonical Term:** Deep Void
**Visual:** Deep bioluminescent waters, gentle glow from depths; dark water with bioluminescent particles
**Gameplay Effect:** Not buildable; enables aqua port placement; defines map boundary
**Player Decisions:**
- "Coastal sectors are premium — fluid extraction is nearby and aqua ports become possible."
- "But coastal sectors also limit expansion in one direction."
- "In competitive play, controlling coastline = controlling trade."
**Fun Factor:** MEDIUM-HIGH. Deep void is a hard boundary that creates scarcity. Coastal real estate is naturally valuable, creating competition. The bioluminescent glow from the depths should be visually striking — looking out over the deep void from a coastal ridge should be a "postcard moment."
**Design Note:** Not every map needs deep void. Landlocked maps create a very different feel (no aqua ports, rivers become the critical fluid source). Offer both types.

#### 4. Flow Channel (water_river)
**Canonical Term:** Flow Channel
**Visual:** Flowing luminous streams
**Gameplay Effect:** Not buildable; fresh fluid source; boosts sector value along banks
**Player Decisions:**
- "Build along the flow channel for fluid access and sector value, but I lose buildable sectors to the water itself."
- "Bridge the flow channel (pathway over water) or route around it?"
- "Control both banks or let a rival have the far side?"
**Fun Factor:** HIGH. Flow channels are the single most gameplay-defining terrain feature. They provide essential resources (fluid), boost sector value, create natural borders between players, and force interesting pathway routing. A map with a single major flow channel running through the center creates a dramatically different competitive dynamic than one with many small branches.
**Design Note:** Flow channels should flow from elevation to sea level (or to still basins). They should feel purposeful, not random. Branching tributaries and river deltas add visual and strategic complexity.

#### 5. Still Basin (water_lake)
**Canonical Term:** Still Basin
**Visual:** Still waters with reflected glow
**Gameplay Effect:** Not buildable; fresh fluid source; recreation bonus (sector value boost); more compact than deep void
**Player Decisions:**
- "A still basin is like a mini coastline — premium adjacent sectors, but it is a pocket of unbuildable space."
- "Build green zones around the basin for maximum harmony and sector value."
- "In a tight map, a large still basin is both a treasure (fluid, value) and an obstacle (lost buildable sectors)."
**Fun Factor:** MEDIUM-HIGH. Still basins create natural focal points. A colony built around a still basin has a town-square feel — everything radiates outward. The reflected glow from still water should be visually calming and distinct from the animated glow of flow channels.
**Design Note:** Still basins should vary in size (from small ponds to large lakes). Small basins are common; large basins are rare and map-defining.

#### 6. Biolume Grove (forest)
**Canonical Term:** Biolume Grove
**Visual:** Dense bioluminescent canopy, brightest natural terrain; glowing alien trees/fungi, teal and green bioluminescence
**Gameplay Effect:** Clearable for building (costs credits); provides harmony bonus + sector value when preserved
**Player Decisions:**
- "Do I purge this grove to build, or preserve it for the harmony and sector value bonus?"
- "A habitation zone next to a preserved biolume grove gets happier beings."
- "Purging costs credits — is it worth it early when credits are tight?"
- "Late game, do I regret purging groves I can never restore?"
**Fun Factor:** HIGH. Biolume groves create the core environmental tension of the game. They are the most visually striking natural terrain (brightest bioluminescence), so purging them feels like a genuine sacrifice. This is the "deforestation dilemma" reframed through an alien lens. Players who preserve groves build harmonious, beautiful colonies. Players who purge everything build efficient, industrial ones. Both are valid. This is expressive gameplay at its best.
**Design Note:** Groves should NOT be restorable once purged (or at minimum, restoration is extremely expensive and slow). The decision to purge must feel permanent and weighty. Consider a "grove preservation bonus" that scales — preserving many groves should be a viable strategy, not just a missed opportunity.

---

### Alien Terrain

#### 7. Prisma Fields (crystal_fields)
**Canonical Term:** Prisma Fields
**Visual:** Clustered crystal formations that pulse with inner light; bright magenta/cyan crystal spires with strong glow emission
**Gameplay Effect:** Buildable (bonus sector value); clearing yields one-time credits
**Player Decisions:**
- "Do I build ON prisma fields for the sector value bonus, or clear them for quick credits?"
- "Prisma fields near my starting sector are incredibly valuable — but so are the credits from clearing them."
- "Preserving prisma fields adjacent to habitation zones creates luxury districts."
**Fun Factor:** VERY HIGH. Prisma fields are the alien terrain type that will define competitive strategies. They are the most visually distinctive terrain, they provide direct economic benefit (credits OR ongoing value), and their rarity creates competition. When two overseers both want the same prisma field cluster, that is a memorable game moment.
**Design Note:** Prisma fields should be rare and clustered, not scattered. A single large prisma formation is more interesting than many small patches. The pulsing glow should be visible from far away, making them landmarks on the map even before zooming in.

#### 8. Spore Flats (spore_plains)
**Canonical Term:** Spore Flats
**Visual:** Undulating fields of bioluminescent spore pods; pulsing green/teal spore clouds, soft ambient glow
**Gameplay Effect:** Buildable; boosts harmony for nearby habitation zones
**Player Decisions:**
- "Spore flats near habitation zones are free harmony — but they take up buildable space."
- "Do I build ON the spore flats (losing the bonus) or AROUND them?"
- "Spore flats + biolume groves near habitation = very happy beings with minimal service investment."
**Fun Factor:** MEDIUM. Spore flats are the "nice to have" terrain — no hard decisions, just a pleasant bonus. They become more interesting in the mid-game when harmony becomes a bottleneck. Their visual distinctiveness (spore clouds, ambient glow) should make them feel alien and atmospheric.
**Design Note:** Spore flats should be moderately common and form in patches near biolume groves (ecologically connected). The harmony bonus should be significant enough that preserving them is a real strategic consideration, not trivially outweighed by building on them.

#### 9. Blight Mires (toxic_marshes)
**Canonical Term:** Blight Mires
**Visual:** Murky terrain with sickly yellow-green glow; bubbling surface, dark fog
**Gameplay Effect:** Not buildable; generates contamination in radius; reduces sector value nearby
**Player Decisions:**
- "Blight mires are the worst terrain — do I avoid this area entirely?"
- "...but the sectors on the OTHER side of the blight mire might be unclaimed and cheap precisely because no one wants to build near contamination."
- "Can I isolate the contamination with fabrication zones (which already generate contamination) and contain the damage?"
- "Late game, can I terraform blight mires away? Is it worth the cost?"
**Fun Factor:** HIGH (through challenge). Blight mires are the "obstacle" terrain. They are only fun because they create problems to solve. A map without any blight mires is less interesting than one where a key strategic location is partially blocked by contamination. The sickly yellow-green glow should feel *wrong* against the beautiful blue/teal bioluminescence elsewhere — visual dissonance as gameplay signal.
**Design Note:** Blight mires should NEVER be placed where they make the map unplayable (blocking all expansion paths). They should be speed bumps, not brick walls. Small blight mire patches near otherwise-desirable terrain create the best dilemmas. In competitive maps, placing blight mires between player starting areas creates natural buffer zones.

#### 10. Ember Crust (volcanic_rock)
**Canonical Term:** Ember Crust
**Visual:** Dark basalt with veins of molten orange glow; obsidian-like rock with orange/red glow cracks
**Gameplay Effect:** Buildable at increased cost; geothermal energy bonus; natural barrier
**Player Decisions:**
- "Ember crust is expensive to build on, but geothermal energy means cheap energy generation."
- "An energy nexus on ember crust is incredibly efficient — worth the extra building cost?"
- "Ember crust regions are natural fortresses in competitive play — hard to reach, hard to develop, but self-sustaining once built."
- "Late game investment: develop the ember crust after cheaper terrain is claimed?"
**Fun Factor:** HIGH. Ember crust is the "advanced player" terrain. New overseers will avoid it because of the cost. Experienced overseers will recognize the geothermal energy potential and the strategic value of a self-powering district. The orange glow against the blue/teal base palette makes ember crust visually dramatic — a colony built on ember crust looks volcanic and imposing.
**Design Note:** Ember crust should form in geologically plausible patterns — volcanic ridges, basalt plains, caldera rims. It should often be at higher elevation (volcanic mountains). The geothermal energy bonus should be significant enough to offset the building cost over time, making it a long-term investment.

---

## Map Generation Feel

### What Makes a Good Map

A great map tells a story before anyone builds on it. Procedural generation must produce geography that feels *intentional*, not random. Here are the principles:

### 1. Geographic Coherence
Terrain features must relate to each other logically:
- Flow channels flow downhill from ridges to deep void or still basins
- Biolume groves cluster in lowlands and along flow channel banks
- Ember crust forms at high elevation in volcanic ridges
- Blight mires form in lowlands (toxic runoff collects in depressions)
- Prisma fields form on exposed ridgelines and plateaus (crystals grow where there is no canopy)
- Spore flats form in transitional zones between substrate and biolume groves

This coherence is not just visual polish — it creates intuitive terrain reading. Players who understand "crystals grow high, toxins pool low" can read a map's elevation from its terrain types alone.

### 2. Dominant Feature
Every map should have one or two "headline" geographic features:
- A massive flow channel bisecting the map
- A central volcanic ridge (ember crust mountain)
- A large still basin ringed by biolume groves
- A prisma field plateau overlooking the deep void
- A blight mire valley separating two habitable regions

These dominant features give the map identity. Players should be able to describe their map: "We played on the one with the huge river" or "The crystal mountain map."

### 3. Discovery Pockets
Not everything should be visible or obvious from the starting position. The map should reward exploration:
- A hidden still basin behind a ridge
- A prisma field cluster on the far side of a blight mire
- A narrow valley with ideal building conditions between two ridges
- A sheltered cove on the coastline

Free camera (with its 15-80 degree pitch range) amplifies discovery — tilting the camera to peer over a ridge and spotting valuable terrain beyond is a moment of genuine excitement.

### 4. Tension Between Quality and Accessibility
The best terrain should not be the easiest to reach:
- Prisma fields on high ridges (great bonus, hard to connect with pathways)
- Flow channel confluences in lowlands surrounded by blight mires
- Still basins with only narrow substrate access corridors
- Ember crust geothermal sites at the far edge of the map

### 5. Variety Across Games
The generation system should produce maps that feel significantly different from each other. Two 256x256 maps should not feel interchangeable. Biome variation, dominant feature selection, and coastline shape should create replayability.

### Map Archetypes (Suggested Generation Templates)
| Archetype | Description | Competitive Feel |
|-----------|-------------|-----------------|
| River Valley | Central flow channel with habitable banks | Players compete for river access |
| Archipelago | Deep void with scattered land masses | Isolated colonies, aqua port focus |
| Volcanic Highlands | Central ember crust massif, lowlands around edges | Energy-rich center vs. accessible edges |
| Crystal Plateau | Elevated prisma field mesa, substrate lowlands | High ground = high value, low ground = easy building |
| Blighted Divide | Blight mire belt separating two habitable halves | Natural player separation with contested crossing points |
| Lush Basin | Still basin center, biolume groves radiating outward | Harmony paradise, fight for lakefront sectors |

---

## Multiplayer Fairness

### The Core Fairness Problem
In a 2-4 overseer competitive sandbox, terrain distribution directly affects who has the best start. Unlike a board game where pieces are dealt equally, a procedurally generated map is inherently asymmetric. The question is: how much asymmetry is acceptable?

### Fairness Philosophy
**Target: Approximately equal opportunity, not identical starting conditions.**

Perfect symmetry is boring. If all four overseers start in identical terrain, the game becomes purely about optimization speed — no interesting site-selection decisions. Instead, different starting locations should offer *different advantages*:
- One overseer starts near a flow channel (fluid advantage)
- Another starts near prisma fields (sector value advantage)
- A third starts on ember crust (energy advantage)
- The fourth starts in open substrate (flexibility advantage)

The key is that no advantage is strictly dominant over others. Each terrain advantage should be roughly equivalent in overall value.

### Spawn Point Design
**Spawn Placement Rules:**
1. Spawn points must be on or adjacent to substrate (immediate buildable terrain)
2. Spawn points must have a minimum "buildable radius" of N sectors of substrate
3. No spawn point should be within the contamination radius of a blight mire
4. Spawn points should be roughly equidistant from each other
5. Each spawn region should have access to at least one fluid source (flow channel, still basin, or deep void) within a reasonable distance

**Resource Access Scoring:**
Generate a "terrain value score" for each potential spawn region:
- Fluid access: weighted by proximity and source type
- Special terrain: weighted by prisma fields, spore flats, ember crust within range
- Buildable area: total substrate + buildable terrain within expansion radius
- Contamination exposure: penalty for nearby blight mires
- Elevation advantage: bonus for defensible/scenic positions

Balance spawn selection by choosing points with similar terrain value scores. Allow variance within a tolerance band (e.g., all spawn scores within 15% of each other).

### Map Symmetry Approaches
| Players | Recommended Symmetry | Rationale |
|---------|---------------------|-----------|
| 2 | Rotational (180 degrees) | Each player gets "mirror" geography; flow channels/features echo across the center |
| 3 | Radial (120 degrees) | Three-fold symmetry around a central feature |
| 4 | Quad rotational (90 degrees) or 2x2 mirror | Four quadrants with shared central feature |

**Important:** Symmetry should be *approximate*, not pixel-perfect. Identical terrain is boring. The generation should apply symmetry to major features (flow channels, ridges, terrain type distribution) while allowing local variation in details.

### Late-Join Fairness
Since this is an endless sandbox with late join support, new overseers arriving mid-game need viable starting locations:
- Reserve some unclaimed sectors with decent terrain
- Ghost town (remnant) sectors that revert to the World Keeper become available for new overseers
- Late-join overseers could receive a slight credit bonus to offset established player advantages

### Anti-Snowball Terrain Mechanics
- No single terrain bonus should be so powerful that controlling it guarantees dominance
- Blight mires near desirable terrain prevent any player from "having it all"
- Ember crust regions reward long-term investment, giving trailing overseers a late-game option
- Prisma field credits are one-time — early clearing gives quick cash but removes the ongoing value bonus

---

## Terrain Modification

### What Can Overseers Change?

Terrain modification is the overseer asserting control over the world. Each modification type should feel like a meaningful investment with visible, permanent consequences.

#### 1. Purge Terrain (purge_terrain)
**What:** Remove biolume groves, prisma fields, or spore flats to make sectors buildable
**Cost:** Credits (varies by terrain type: groves are cheap, prisma fields are expensive but yield credits back)
**Consequence:** Permanent loss of terrain bonus (harmony, sector value, etc.)
**Feel:** Bittersweet. The beautiful bioluminescent canopy fades as it is cleared. The pulsing crystals go dark. This should NOT feel good — it should feel like a sacrifice made for progress. Audio cue: a fading glow sound, not a triumphant demolition.
**Pacing:** Available immediately; early game purging is common as overseers establish their first sectors.

#### 2. Grade Terrain (grade_terrain / level_terrain)
**What:** Flatten ridges or fill valleys to create level building surfaces
**Cost:** Credits, scales with elevation change (grading 5 levels costs more than grading 1)
**Consequence:** Permanent elevation change; may affect flow channel routing and drainage
**Feel:** Powerful. Reshaping the land itself is an act of mastery. The gradual transformation (not instant — should take multiple ticks) gives a sense of engineering accomplishment.
**Pacing:** Mid-game; overseers grade terrain when they have credits to spare and need more buildable space.

#### 3. Terraform (advanced modification)
**What:** Convert one terrain type to another (e.g., blight mires to substrate, ember crust to substrate)
**Cost:** Very expensive; blight mire reclamation should be a major investment
**Consequence:** Removes both the negative effects (contamination) and any potential benefits (geothermal)
**Feel:** Triumphant. Reclaiming blight mire is a late-game achievement. The sickly yellow-green glow being replaced by clean substrate should feel like a medical recovery — the land is healed.
**Pacing:** Late game only; terraforming should be a luxury, not a routine tool.

#### 4. Water Manipulation (limited)
**What:** Overseers CANNOT fill in or drain water bodies (deep void, flow channels, still basins)
**Rationale:** Water is permanent geography. Allowing players to drain a flow channel would break the map's fundamental structure and other players' fluid extraction infrastructure.
**Exception:** Small puddles or shallow water at single-sector scale could potentially be filled as part of grading.
**Feel:** Water is "bigger than you" — the world has some things the overseer cannot change, and that is appropriate for the theme. These aliens respect the deep void.

### Modification Visibility
All terrain modifications should be visually distinct from natural terrain:
- Graded terrain shows construction marks (flattened, slightly lighter substrate)
- Purged groves leave stumps that fade over time
- Terraformed blight mire has a slightly different color than natural substrate (reclaimed land)

This creates a visual history — experienced players can read the terrain and understand what changes an overseer made.

---

## Elevation Gameplay

### How 32 Levels Create Interesting Building Challenges

Thirty-two elevation levels provide a meaningful vertical dimension to the gameplay without overwhelming granularity.

### Elevation as Cost Modifier
- Building on sloped terrain costs more than flat terrain
- Grading (leveling) costs scale with elevation delta
- Pathways on slopes are more expensive and may reduce transport capacity
- Energy conduits and fluid conduits on slopes cost more to place

### Elevation as Strategic Value
- **High Ground Visibility:** From a high camera angle on a ridge, the overseer can survey more territory. This is not just aesthetic — it makes high-elevation colonies feel commanding.
- **Natural Borders:** Ridges between overseers create natural territory divisions. Neither player needs to "claim" the ridge — it functions as a wall.
- **Drainage:** Contamination could spread downhill more easily, making low-elevation sectors near fabrication zones more polluted. High ground stays cleaner.
- **Fluid Dynamics:** Flow channels flow downhill. Fluid extractors at lower elevations near flow channels are more efficient than pumps at high elevation drawing from a still basin.

### Elevation Bands (Suggested Design)
| Elevation Range | Name | Character |
|----------------|------|-----------|
| 0-3 | Lowlands | Flat, easy to build; near fluid; vulnerable to contamination spread |
| 4-10 | Foothills | Gentle slopes; moderate cost; good all-around |
| 11-20 | Highlands | Significant slopes; expensive to grade; great views |
| 21-27 | Ridgelines | Steep; very expensive; natural barriers; strategic value |
| 28-31 | Peaks | Extreme; nearly unbuildable without major grading; dramatic visual |

### Elevation and the Camera
The free camera with 15-80 degree pitch range means elevation is *felt*, not just seen on a heightmap overlay. Key experiences:
- Looking UP at a ridge from a lowland colony and seeing crystalline outcroppings glow against the dark sky
- Looking DOWN from a ridge-top colony and seeing the bioluminescent landscape spread below
- Orbiting a mountain to find buildable saddle points between peaks
- Tilting the camera low (15 degrees) and seeing the silhouette of terrain against the glow of distant prisma fields

**This is one of the strongest arguments for 3D terrain with a free camera. Elevation is not just a number — it is a visual, spatial experience.**

### Elevation as Multiplayer Mechanic
- Ridge control is strategic: building on the ridge between you and a rival gives visibility into their colony
- Valleys between ridges create "contested corridors" for pathway routing
- A player who controls the high ground can potentially build energy conduits with line-of-sight advantages
- Low-ground players have fluid access advantages; high-ground players have defensive/scenic advantages

---

## Visual Readability

### The Core Challenge
With 10 terrain types, a free camera at any angle, and a bioluminescent art direction, players need to instantly identify what terrain they are looking at. The bioluminescent glow is our primary tool — each terrain type has a distinct glow color and pattern.

### Glow Color Language
| Terrain Type | Primary Glow Color | Secondary | Pattern |
|-------------|-------------------|-----------|---------|
| Substrate | Faint blue-green | - | Subtle veining, barely visible |
| Ridges | Blue-white | Cyan edges | Glowing vein patterns in rock layers |
| Deep Void | Deep blue | Scattered particles | Slow, gentle pulse from depths |
| Flow Channel | Bright teal-cyan | White highlights | Flowing, animated movement |
| Still Basin | Soft cyan | Reflected light | Still, mirror-like with subtle shimmer |
| Biolume Grove | Bright teal + green | Varied warm spots | Dense, overlapping, organic glow |
| Prisma Fields | Magenta + cyan | White sparkle | Sharp, geometric, pulsing |
| Spore Flats | Green-teal | Soft cloud | Rhythmic pulsing, cloud-like diffusion |
| Blight Mires | Sickly yellow-green | Dark fog | Irregular, bubbling, unpleasant |
| Ember Crust | Orange-red | Dark base | Crack-line patterns, volcanic veins |

### Readability Rules
1. **The 2-Second Rule:** A player should be able to identify any terrain type within 2 seconds of looking at it, from any camera angle.
2. **Color Uniqueness:** No two terrain types share the same primary glow color. The palette is designed so that each type occupies a unique hue band.
3. **Pattern Differentiation:** Even for color-impaired players, the glow PATTERN (veining vs. pulsing vs. flowing vs. geometric) provides secondary identification.
4. **Distance Readability:** At maximum zoom-out on a 512x512 map, terrain types should still be distinguishable by color, even if pattern detail is lost to LOD.
5. **Minimap Representation:** The sector scan (minimap) should use the same color language as the terrain glow, providing consistent identification across views.

### Camera Angle Considerations
- **Top-down (80 degrees):** Full terrain color/pattern visibility. Best for strategic overview.
- **Mid-angle (45 degrees):** Standard isometric preset view. Terrain glow is visible on surfaces and edges. Ridge silhouettes are clear.
- **Low angle (15 degrees):** Terrain glow becomes horizon glow — you see the light emission more than the surface pattern. Silhouettes of ridges, groves, and prisma spires become the primary identifier.

### Scan Layers (Overlays)
In addition to natural visual readability, the UI should offer terrain-specific scan layers:
- **Terrain Type Overlay:** False-color overlay showing terrain type categories (buildable in green, unbuildable in red, special in gold)
- **Elevation Overlay:** Height-coded color ramp showing the 32 elevation levels
- **Buildability Overlay:** Simple green/red showing where the overseer CAN build right now (considering terrain type, elevation, ownership)
- **Contamination Overlay:** Shows blight mire contamination radius and fabrication pollution spread

---

## Pacing & Progression

### When Terrain Decisions Matter

Terrain creates a pacing arc that evolves across a play session:

### Early Game: Site Selection (First 5-10 Minutes)
**Terrain Dominance: MAXIMUM**

This is when terrain matters most. The overseer is surveying the map, choosing where to charter their first sectors, and the terrain IS the game. Every decision is terrain-driven:
- Where is the fluid?
- Where is the flat substrate?
- Where are the biolume groves (and should I purge them)?
- Where is my rival settling? Can I cut them off from the flow channel?
- Should I rush to the prisma fields for early credits?

**Feel:** Tense, exciting, exploratory. The free camera gets heavy use as the overseer orbits the map, scanning for the perfect site.

### Early-Mid Game: Expansion (10-30 Minutes)
**Terrain Dominance: HIGH**

The initial colony is established. Now terrain shapes expansion decisions:
- Which direction to grow? Toward the flow channel for fluid, toward the prisma fields for value, away from blight mires to avoid contamination?
- When to start purging groves — now (to expand) or later (to preserve harmony)?
- Should I grade that ridge to connect two halves of my colony, or route pathways around it?

**Feel:** Purposeful, strategic. Each expansion is a terrain negotiation.

### Mid Game: Infrastructure (30-60 Minutes)
**Terrain Dominance: MEDIUM**

Infrastructure (energy, fluid, transport) now competes with terrain for attention. Terrain becomes the context rather than the focus:
- Pathway routing through ridges and around water
- Energy conduit placement considering elevation
- Fluid extractor placement near flow channels and still basins
- Terrain type affecting building costs and effectiveness

**Feel:** Terrain is the map you play on, not the game itself. It shapes decisions but does not dominate them.

### Late Game: Optimization & Terraforming (60+ Minutes)
**Terrain Dominance: LOW-MEDIUM (with late spikes)**

By late game, most initial terrain decisions are made. Terrain re-emerges when:
- The overseer needs more space and considers grading ridges
- Blight mire reclamation becomes affordable
- Ember crust development for geothermal energy becomes strategic
- Competing overseers push into contested terrain between colonies

**Feel:** Mastery, transformation. Terrain modification is a late-game luxury that shows wealth and ambition.

### Pacing Insight
Terrain should NEVER become irrelevant. Even in late game, elevation affects contamination spread, terrain type bonuses affect sector value calculations, and the map's geography shapes transport networks. The terrain is always there, always mattering — just at different intensities throughout the session.

---

## Map Size Experience

### Compact Region (128x128 — 16,384 sectors)
**Recommended Overseers:** 1-2
**Feel:** Intimate, competitive, claustrophobic

- Every sector is precious. There is no "unused" terrain.
- In 2-overseer competitive, the map fills fast. Border disputes happen early.
- Terrain features are few and dramatic — a single flow channel dominates the map.
- Blight mires are threatening because there is nowhere else to go.
- Prisma fields are game-defining because there may only be one cluster.
- Ridges create hard borders in a map that is already tight.
- The overseer can see most of the map from any camera position.

**Best For:** Quick competitive games, learning the mechanics, 1v1 rivalry.
**Risk:** Can feel too small if terrain generation creates large unbuildable areas (deep void, blight mires). Generation must ensure adequate buildable space per overseer.

### Standard Region (256x256 — 65,536 sectors)
**Recommended Overseers:** 2-3
**Feel:** Comfortable, balanced, the "default" experience

- Enough space for colonies to develop independently before borders touch.
- Multiple terrain features create varied geography.
- Flow channels can branch, creating interesting confluences.
- Blight mires are obstacles, not map-enders.
- Overseers have meaningful expansion choices (multiple viable directions).
- Late-game exploration still reveals undeveloped areas.

**Best For:** Standard competitive play, balanced experience, most play sessions.
**Design Note:** This should be the default and the most-tested size. If we only have time to perfect one size, it should be 256x256.

### Expanse Region (512x512 — 262,144 sectors)
**Recommended Overseers:** 2-4
**Feel:** Expansive, epic, exploratory

- Huge amounts of wilderness between colonies. The world feels big and untamed.
- Overseers may not encounter each other for the first 20-30 minutes.
- Multiple dominant geographic features create distinct "regions" on the map.
- Late-join overseers can find good terrain far from established colonies.
- The map itself becomes a character — overseers discuss "the northern crystal plateau" or "the southern blight rift."
- Requires LOD and culling for rendering performance.

**Best For:** Epic multi-session games, 3-4 overseer play, exploration-focused players.
**Risk:** Can feel empty or lonely with fewer than 3 overseers. The sandbox needs enough active overseers to fill the space with interesting colonies. Consider populating empty areas with more dramatic terrain features to compensate.

### Size Selection as Game Design
The overseer (or prime overseer, in multiplayer) chooses the region scale when creating a new game. This is itself a design decision:
- Small = fast, tense, confrontational
- Medium = balanced, versatile
- Large = epic, exploratory, long-term

The UI should communicate this clearly during game creation, setting expectations for the play experience.

---

## Key Work Items

- [ ] **Terrain type data model:** Define TerrainComponent with all 10 terrain types, elevation (0-31), and gameplay effect flags (buildable, contamination source, harmony bonus, etc.)
- [ ] **Procedural generation — geology pass:** Generate elevation map with geologically coherent ridges, valleys, plateaus, and lowlands using noise functions with geological constraints
- [ ] **Procedural generation — hydrology pass:** Place deep void (coastlines), flow channels (rivers following elevation), and still basins (lakes in depressions) using drainage simulation
- [ ] **Procedural generation — biome pass:** Place biolume groves, spore flats, prisma fields, blight mires, and ember crust using elevation-dependent and proximity-dependent rules
- [ ] **Procedural generation — spawn balancing:** Implement spawn point selection with terrain value scoring to ensure multiplayer fairness
- [ ] **Map archetype system:** Implement generation templates (River Valley, Archipelago, Volcanic Highlands, etc.) that bias generation toward distinct map identities
- [ ] **Terrain modification — purge:** Implement grove/prisma/spore clearing with credit cost, visual transition, and permanent terrain change
- [ ] **Terrain modification — grading:** Implement elevation modification with cost scaling, multi-tick duration, and visual construction effect
- [ ] **Terrain modification — terraforming:** Implement blight mire reclamation and ember crust clearing as late-game expensive operations
- [ ] **Terrain query API:** Provide terrain type, elevation, buildability, and gameplay effect queries for other systems (ZoneSystem, EnergySystem, FluidSystem, LandValueSystem)
- [ ] **Glow color specification:** Define exact glow colors, patterns, and animation parameters for each terrain type for the rendering team
- [ ] **Map size scaling rules:** Define how terrain feature density, size, and distribution scale across 128/256/512 region scales
- [ ] **Elevation gameplay tuning:** Define cost curves for building on slopes, grading terrain, and routing infrastructure through elevation changes
- [ ] **Multiplayer symmetry generation:** Implement approximate rotational symmetry for 2/3/4 overseer maps with terrain value balancing
- [ ] **Terrain scan layers:** Define terrain-type, elevation, and buildability overlays for the UI system
- [ ] **Audio cues for terrain:** Define ambient audio per terrain type (crystal hum for prisma fields, bubbling for blight mires, flowing water for channels, wind for ridges)

---

## Questions for Other Agents

### @systems-architect
- How does the TerrainComponent interact with the ECS? Is terrain stored as one entity per sector, or as a dense 2D array that the TerrainSystem manages internally? The latter is more cache-efficient for 512x512 maps (262,144 sectors), but the former is more ECS-idiomatic.
- For terrain modification (grading, purging, terraforming), should these be multi-tick operations with a ConstructionComponent analog (e.g., "TerrainModificationComponent" with progress), or instant operations gated only by cost?
- How does terrain state synchronize in multiplayer? Full terrain snapshot on join + delta updates on modification? Or is terrain considered static after generation and only modifications sync?
- What is the authority model for terrain generation? Server generates terrain from a seed, clients receive the full terrain state on join?

### @graphics-engineer
- Can the toon shader pipeline support per-terrain-type emissive glow with the specified color palette? How many distinct emissive materials can we support without performance issues?
- For terrain mesh generation, are we creating a single heightmap mesh for the entire map with per-vertex terrain type data, or separate mesh chunks? What is the LOD strategy for 512x512 maps?
- How do we handle terrain type boundaries visually? Hard edges between substrate and biolume grove, or blended transitions?
- Can we support animated glow effects (pulsing prisma fields, flowing flow channels, bubbling blight mires) at the terrain shader level without per-tile draw calls?
- What does the "purge terrain" visual transition look like? Fade-out of glow? Particle effect? This needs to feel impactful, not just a texture swap.

---

## Risks & Concerns

### 1. Procedural Generation Quality
**Risk:** Procedural maps that look random and boring, lacking the "designed" feel of hand-crafted terrain.
**Mitigation:** Invest heavily in multi-pass generation (geology -> hydrology -> biome) with ecological rules. Use map archetypes to ensure variety. Extensive playtesting of generated maps.
**Impact if unmitigated:** Players get bored of terrain quickly; maps feel interchangeable; "one more game" appeal is lost.

### 2. Multiplayer Fairness Perception
**Risk:** Even with balanced spawns, players will blame losses on "unfair terrain." This is perception, not necessarily reality.
**Mitigation:** Provide a terrain value overlay that shows relative spawn quality. Let players choose spawn locations (draft-style) rather than random assignment. Communicate that terrain advantages are *different* (not *better*).
**Impact if unmitigated:** Salty players, blame-the-map culture, reduced competitive enjoyment.

### 3. Visual Noise from Bioluminescence
**Risk:** With 10 terrain types all glowing in different colors, the map could look like a chaotic light show rather than a coherent environment.
**Mitigation:** Substrate (the most common terrain) should have VERY subtle glow. Use the dark base environment to anchor the palette. Glow intensity should scale with terrain "importance" (prisma = bright, substrate = barely visible). Test at all zoom levels.
**Impact if unmitigated:** Visual fatigue, difficulty reading terrain, ugly screenshots (bad for marketing/sharing).

### 4. Elevation Complexity
**Risk:** 32 elevation levels might be overwhelming for casual overseers who just want to place structures.
**Mitigation:** Auto-grading for small elevation changes (1-2 levels) as part of building cost. Only present elevation as a manual tool for large changes. Show elevation subtly in the terrain mesh, not as a number overlay.
**Impact if unmitigated:** New players confused by elevation, avoid building on anything non-flat, miss interesting terrain strategies.

### 5. Map Generation Performance
**Risk:** Generating a 512x512 map with multi-pass procedural generation could take significant time, especially on weaker hardware.
**Mitigation:** Generation happens on server only (clients receive completed terrain). Use efficient noise generation. Consider pre-generating and caching map seeds. Show a loading screen with the map "being discovered" as a narrative element.
**Impact if unmitigated:** Long wait times when starting games, frustration in lobby.

### 6. Terrain Modification Balance
**Risk:** If terraforming (especially blight mire reclamation) is too cheap, late-game maps become homogeneous substrate. If too expensive, it is irrelevant.
**Mitigation:** Tuning pass with playtesting. Costs should scale with colony wealth (percentage-based rather than flat cost). Terraforming should always be a significant investment, never trivial.
**Impact if unmitigated:** Either every late-game map looks the same (all terraformed) or the feature is ignored (wasted development effort).

---

## Alien Theme Notes

### Terminology Consistency
All player-facing references must use canonical alien terminology:

| Standard Term | Canonical Term | Context |
|--------------|---------------|---------|
| Flat ground | Substrate | Base terrain type |
| Hills | Ridges | Elevated terrain |
| Ocean | Deep Void | Map-edge water |
| River | Flow Channel | Flowing water |
| Lake | Still Basin | Inland water |
| Forest | Biolume Grove | Alien vegetation |
| Crystal fields | Prisma Fields | Crystal formations |
| Spore plains | Spore Flats | Spore flora |
| Toxic marshes | Blight Mires | Toxic terrain |
| Volcanic rock | Ember Crust | Volcanic terrain |
| Clear terrain | Purge Terrain | Removing vegetation/crystals |
| Level terrain | Grade Terrain | Flattening elevation |
| Tile | Sector | Individual land unit |
| Map size | Region Scale | Map dimensions |
| Small map | Compact Region | 128x128 |
| Medium map | Standard Region | 256x256 |
| Large map | Expanse Region | 512x512 |

### Thematic Flavor
The alien civilization in ZergCity has a deep relationship with their terrain. Consider these flavor elements:

- **Biolume groves** are not just trees — they are part of the planet's living network. Purging them should feel like surgery, not logging.
- **Prisma fields** are ancient geological formations, possibly older than the civilization itself. They pulse with an energy that is not fully understood. Building among them feels like building among ancient monuments.
- **Blight mires** are not just "toxic swamps" — they are terrain that has been corrupted. By what? The lore is deliberately ambiguous. Perhaps previous civilizations, perhaps natural geological processes, perhaps something deeper. The sickly glow feels *wrong* because it IS wrong.
- **Ember crust** is the planet's fire — not dead, but sleeping. Building on it is a statement of confidence (or hubris).
- **The deep void** is called the "void" because these aliens do not think of it as "ocean." It is the boundary of the known, the edge of the world. Staring into the deep void and seeing faint bioluminescence from below should evoke awe and unease.
- **Flow channels** are the planet's arteries. The luminous fluid that flows through them is not just water — it glows because it carries the planet's energy. This is why fluid extraction is so important: overseers are literally tapping the planet's lifeblood.

### Tone
The terrain should communicate that this world is *alive, ancient, and indifferent* to the overseers building on it. It was here before them. It will be here after them. The overseers are guests who have been given permission to build — but the world does not need them. This creates a subtle tension between the player's desire to build and the world's existing beauty, which is the emotional core of the terrain experience.
