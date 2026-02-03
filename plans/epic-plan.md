# Sims 3000 - Epic Planning Document

## Overview

**Goal:** Create a **multiplayer** SimCity 2000-inspired city builder with an alien theme.

**Key Requirements:**
- **Multiplayer is MVP** - Built for playing with friends
- Humans → Aliens (citizens, workers, etc.)
- All assets (textures, models, sprites) to be provided by user
- Custom terminology to fit alien civilization theme
- No scenario system (cut to reduce scope)

---

## Complete Epic List

### EPIC 0: Project Foundation
**Description:** Core project setup and architecture foundation.

**Features:**
- SDL3 initialization and window management
- ECS (Entity-Component-System) architecture setup
- Main game loop (update/render cycle)
- Asset loading pipeline (sprites, textures, audio)
- Input handling system
- Configuration/settings system

**Dependencies:** None (starting point)

---

### EPIC 1: Multiplayer Networking (MVP REQUIREMENT)
**Description:** Core multiplayer infrastructure enabling friends to play together. This is foundational and must be architected early.

**Player Count:** 2-4 players
**Mode:** Cooperative sandbox (shared world, individual territories, no victory conditions)

**Features:**
- Network Architecture:
  - Client-Server model (dedicated server)
  - Direct join into live world (no lobby phase)
  - Player connection handling (2-4 players)
- Game State Synchronization:
  - Authoritative server for simulation
  - Delta state updates to clients
  - Latency compensation
- Shared World (Endless Sandbox):
  - All players build on same map
  - Each player owns their tiles/zones/buildings
  - Individual budgets and tribute collection
  - Real-time visibility of other players' actions
  - Synchronized simulation tick
  - Shared infrastructure (pathways connect across boundaries)
  - No victory conditions — build, prosper, hang out with friends
- Player Management:
  - Player identification/colors
  - Player-owned zones clearly marked
  - Tile-based ownership (purchase individual tiles)
- Communication:
  - In-game chat
  - Player notifications
- Connection Handling:
  - Reconnection support with session preservation
  - Graceful disconnect handling
  - Ghost town decay for abandoned colonies

**Dependencies:** EPIC 0

---

### EPIC 2: Core Rendering Engine
**Description:** 3D toon-shaded rendering via SDL_GPU with free camera controls.

**Features:**
- SDL_GPU 3D rendering pipeline
- Toon/cell shader (hard shadow edges, outline pass, emissive glow)
- Bioluminescent art direction (dark base + glowing accents)
- Camera system:
  - Free camera: orbit, pan, zoom, tilt
  - Isometric presets: N/E/S/W snap views at 45° increments
  - Toggle between free and preset modes
- Chunked terrain mesh generation (32x32 tiles per chunk)
- 3D model rendering for buildings and landmarks
- Render layers (terrain, buildings, overlays)
- Multiplayer: Render other players' cursors via ICursorSync

**Dependencies:** EPIC 0, EPIC 1

---

### EPIC 3: Terrain System
**Description:** Dense grid terrain data, procedural generation, and alien terrain types.

**Features:**
- TerrainGrid: dense 4-byte/tile storage (type, elevation, moisture, flags)
- 32 elevation levels (0-31)
- Configurable map sizes: 128x128, 256x256, 512x512
- Procedural terrain generation (scales to map size)
- Classic terrain: substrate, ridges, water (deep_void, flow_channel, still_basin), biolume_grove
- Alien terrain: prisma_fields, spore_flats, blight_mires, ember_crust
- Water body tracking (body IDs, distance field for fluid extractor placement)
- Terrain modification: purge (clear vegetation), grade (flatten), terraform (reshape)
- Interfaces: ITerrainQueryable, ITerrainRenderData, ITerrainModifier

**Dependencies:** EPIC 2

---

### EPIC 4: Zoning & Building System
**Description:** Zone designation, structure materialization, building state machine, and growth pressure (demand).

**Features:**
- Zone types (alien terminology):
  - Habitation (low/high density) — residential
  - Exchange (low/high density) — commercial
  - Fabrication (low/high density) — industrial
- ZoneGrid: dense 1-byte/tile storage for O(1) lookups
- BuildingGrid: dense EntityID array for spatial occupancy
- Structure materialization based on desirability score:
  - Zone density, pathway access, energy coverage, fluid coverage
- Building states: Materializing → Active → Stalled → Derelict
- Template-driven building variety (procedural + hand-modeled)
- 3-tile pathway proximity rule (via ITransportProvider stub)
- Forward dependency stubs: IEnergyProvider, IFluidProvider, ITransportProvider, etc.
- Multiplayer: Server-authoritative zone mutations, per-overseer ownership

**Dependencies:** EPIC 2, EPIC 3

---

### EPIC 5: Power Infrastructure
**Description:** Energy network using pool model. Per-overseer energy pools with coverage zones.

**Features:**
- Pool model (per-overseer energy pools, not physical grid simulation)
- Energy nexus types (9 total):
  - Carbon nexus (polluting, cheap)
  - Petrochemical nexus (moderate pollution)
  - Gaseous nexus (cleaner)
  - Nuclear nexus (high output, meltdown risk)
  - Wind turbines (clean, variable output)
  - Hydro nexus (requires water proximity)
  - Solar collectors (clean, expensive)
  - Fusion reactor (future tech)
  - Microwave receiver (future tech)
- Energy conduits define coverage zones (not transport routes)
- 4-level priority rationing during deficit (critical/important/normal/low)
- 100-tick grace period before brownout effects
- Energy nexus aging and efficiency decay
- CoverageGrid: dense 1-byte/tile for O(1) coverage queries
- Multiplayer: Per-overseer pools, conduits don't cross boundaries

**Dependencies:** EPIC 4

---

### EPIC 6: Water Infrastructure
**Description:** Fluid network using pool model (mirrors energy architecture). Buildings require fluid to develop.

> **Note:** Moved to Phase 2 — buildings cannot develop without fluid coverage.

**Features:**
- Pool model (per-overseer fluid pools, not physical flow simulation)
- Fluid extractors:
  - Must be placed within 8 tiles of water body
  - Efficiency falloff by distance (100% adjacent, 30% at 8 tiles)
  - Require energy to operate (power dependency)
- Fluid reservoirs:
  - Storage buffer (1000 capacity)
  - Asymmetric fill/drain rates (slower fill, faster drain)
  - Provides grace period during extractor outages
- Fluid conduits:
  - Define coverage zones (not transport routes)
  - Same pattern as energy conduits
- No priority rationing (all-or-nothing distribution)
- Fluid affects habitability and sector value
- Multiplayer: Per-overseer fluid pools, conduits don't cross boundaries

**Dependencies:** EPIC 3 (water proximity), EPIC 4 (buildings), EPIC 5 (power for extractors)

---

### EPIC 7: Transportation Network
**Description:** Network-based pathway connectivity with aggregate flow simulation. Uses physical network model (not pool model).

> **Planned:** 48 tickets (22 P0, 18 P1, 8 P2). Canon v0.9.0.

**Features:**
- **TransportSystem** (tick_priority: 45):
  - PathwayGrid: dense 4-byte/tile EntityID spatial queries
  - ProximityCache: 1-byte/tile distance-to-nearest-pathway for 3-tile rule
  - Pathway types: BasicPathway, StandardPathway, TransitCorridor, Bridge, Tunnel
  - NetworkGraph: connected component tracking, O(1) connectivity queries
  - Aggregate flow simulation (diffusion model, not per-vehicle)
  - Congestion calculation with sector_value penalties
  - Pathway decay from traffic wear
  - ITransportProvider interface for BuildingSystem integration
  - Cross-ownership connectivity (pathways connect across player boundaries)
- **RailSystem** (tick_priority: 47):
  - Rail types: SurfaceRail, ElevatedRail, SubterraRail
  - Rail terminals with coverage radius (traffic reduction)
  - Power dependency (rails need energy)
  - Single-depth subterra for MVP
- Key differences from Energy/Fluid:
  - Network model, not pool model
  - Cross-ownership connectivity
  - Flow_blockage is penalty (sector_value), not failure
- Transport pods (buses) deferred to Epic 17+

**Dependencies:** EPIC 3 (terrain), EPIC 4 (buildings), EPIC 5 (power for rail)

---

### EPIC 8: Ports & External Connections
**Description:** Airports, seaports, and neighboring city connections.

**Features:**
- Airports:
  - Zoned (not placed as buildings)
  - Boosts commercial demand
  - Runway requirements
- Seaports:
  - Zoned near water
  - Boosts industrial demand
  - Dock/pier requirements
- Neighbor Connections:
  - Road/rail/power connections at map edge
  - Trade deals
  - Resource sharing
  - Population migration effects
- Multiplayer: Inter-player trade/connections

**Dependencies:** EPIC 4, EPIC 7

---

### EPIC 9: City Services
**Description:** Public buildings and services that affect city quality.

**Features:**
- Area-of-Effect Services:
  - Police Stations (crime reduction radius)
  - Fire Stations (fire protection radius)
- Global-Effect Services:
  - Hospitals (life expectancy)
  - Schools (education quotient)
  - Libraries (education boost)
  - Museums (education/culture)
  - Colleges (higher education)
- Recreation:
  - Small Parks (land value boost)
  - Large Parks (larger land value boost)
  - Marinas (near water)
  - Zoos (culture/recreation)
  - Stadiums (entertainment)
- Other:
  - Prisons (crime management)
  - Emergency dispatch during disasters

**Dependencies:** EPIC 4, EPIC 10 (simulation integration)

---

### EPIC 10: Simulation Core
**Description:** The heart of the game - all simulation calculations. Runs on server, synced to clients.

**Features:**
- RCI Demand Engine:
  - Residential demand factors
  - Commercial demand factors
  - Industrial demand factors
  - Demand bar display
  - Tax rate effects
- Land Value Calculation:
  - Proximity to water/parks/trees
  - Pollution effects (negative)
  - Crime effects (negative)
  - Downtown proximity bonus
  - Water service bonus
- Crime Simulation:
  - Crime generation per building type
  - Police coverage calculation
  - Crime rate display
- Pollution Simulation:
  - Industrial pollution
  - Traffic pollution
  - Power plant pollution
  - Pollution spread/decay
- Population Dynamics:
  - Birth/death rates
  - Migration in/out
  - Population caps
  - Life expectancy calculation
- Employment:
  - Job creation per zone
  - Unemployment calculation
  - Commute patterns
- Multiplayer: Authoritative simulation on host, deterministic updates

**Dependencies:** EPIC 4, EPIC 5, EPIC 6, EPIC 7

---

### EPIC 11: Financial System
**Description:** City budget, taxes, and economic management.

**Features:**
- Tax System:
  - Separate rates for R/C/I zones
  - Tax effects on demand
  - Tax collection cycle
- Budget Management:
  - Department funding sliders
  - Police/Fire/Transit/Health/Education
  - Funding effects on service quality
- City Ordinances:
  - Revenue ordinances (gambling, sales tax)
  - Expense ordinances (free clinics, neighborhood watch)
  - Effect tracking
- Bond System:
  - Issue bonds for emergency funds
  - Interest payments
  - Debt management
- Monthly/Yearly Cycle:
  - Income calculation
  - Expense calculation
  - End-of-year summary
- Multiplayer: Shared vs. individual budgets based on mode

**Dependencies:** EPIC 9, EPIC 10

---

### EPIC 12: Information & UI Systems
**Description:** All user interface elements and information displays.

**Features:**
- Main Build Toolbar:
  - Zone tools
  - Infrastructure tools
  - Service building tools
  - Landscape tools
- Query Tool:
  - Click any tile for info
  - Building name/type
  - Land value
  - Crime/pollution levels
  - Traffic levels (on roads)
  - Power plant output
- Statistics Windows:
  - Population breakdown
  - Industry breakdown
  - Budget window
  - Ordinance window
- Graphs Window:
  - Population over time
  - Crime rate
  - Pollution levels
  - Land value
  - Power usage
  - Unemployment
- Map Overlays:
  - Crime heat map
  - Pollution heat map
  - Land value heat map
  - Power coverage
  - Water coverage
  - Traffic density
- Newspaper System:
  - Random news articles
  - Event reporting (disasters, milestones)
  - Opinion polls
  - Citizen complaints/praise
- Mini-map
- Multiplayer: Player list, chat window, player activity indicators

**Dependencies:** All previous EPICs (displays their data)

---

### EPIC 13: Disasters
**Description:** Natural and man-made disasters that challenge the player.

**Features:**
- Natural Disasters:
  - Earthquake (building damage)
  - Flood (low elevation damage)
  - Hurricane/Typhoon (widespread damage)
  - Tornado (path of destruction)
  - Volcano (if terrain supports)
  - Fire spread
- Man-made Disasters:
  - Riots (crime-related)
  - Nuclear Meltdown (nuclear plants)
  - Chemical Spill (industrial)
  - Microwave Beam Misfire (satellite plant)
- Alien-Themed Disasters:
  - Space anomalies
  - Cosmic radiation events
  - (Replace UFO attacks with something fitting)
- Emergency Response:
  - Dispatch firefighters
  - Dispatch police
  - Dispatch military (if base exists)
  - Bucket brigade (no fire dept fallback)
  - National Guard (no police fallback)
- Disaster Toggle:
  - Enable/disable random disasters
  - Manual disaster trigger (sandbox)
- Damage & Recovery:
  - Building destruction
  - Rubble cleanup
  - Reconstruction
- Multiplayer: Synchronized disaster events, cooperative response

**Dependencies:** EPIC 4, EPIC 9

---

### EPIC 14: Progression & Rewards
**Description:** Population milestones, reward buildings, and end-game content.

**Features:**
- Population Milestones:
  - 2,000 - Leader's Quarters (Mayor's Mansion)
  - 10,000 - Central Command (City Hall)
  - 30,000 - Monument/Statue
  - 60,000 - Military Base (4 types: Air, Ground, Missile, Naval)
  - 90,000 - Special Landmark (Braun Llama Dome equivalent)
  - 120,000 - Arcologies unlock
- Arcology System:
  - 4 arcology types with different capacities
  - Year-based unlocking (2000, 2050, 2100, 2150)
  - High population density structures
  - Construction costs
- The Exodus (End-game):
  - 300+ Launch Arcologies trigger exodus
  - Arcologies "launch" to colonize new worlds
  - Population reduction
  - Cost refund
  - Optional "win condition"
- Technology Timeline:
  - Power plant unlocks over years
  - Building upgrades over time

**Dependencies:** EPIC 4, EPIC 10, EPIC 11

---

### EPIC 15: Audio System
**Description:** Music, sound effects, and ambient audio.

**Features:**
- Background Music:
  - Multiple tracks
  - Context-sensitive (calm/busy city)
- Sound Effects:
  - UI clicks and feedback
  - Construction sounds
  - Demolition sounds
  - Traffic ambient
  - Disaster sounds
- Ambient Audio:
  - City bustle based on population
  - Zone-specific ambiance
  - Weather sounds (optional)
- Volume Controls:
  - Master/Music/SFX sliders
- Multiplayer: Player action sounds, chat notifications

**Dependencies:** EPIC 0

---

### EPIC 16: Save/Load System
**Description:** Game persistence for single-player and multiplayer sessions.

**Features:**
- Save System:
  - Full city state serialization
  - Multiple save slots
  - Auto-save
- Load System:
  - City restoration
  - Version compatibility
- New Game Options:
  - Terrain generation settings
  - Starting funds
  - Difficulty settings
  - Year selection
  - Multiplayer mode selection
- Multiplayer Persistence:
  - Host saves game state
  - Session resume capability
  - Player data preservation

**Dependencies:** All EPICs (must save all state)

---

### EPIC 17: Polish & Alien Theming
**Description:** Final polish, alien terminology, and thematic consistency.

**Features:**
- Alien Terminology:
  - Citizens → Colony Members / Beings
  - Mayor → Overseer / Hive Leader
  - City → Colony / Settlement
  - Residential → Habitation
  - Commercial → Exchange / Trade
  - Industrial → Production / Fabrication
- UI Theming:
  - Alien aesthetic for all windows
  - Custom fonts (if applicable)
  - Color scheme
- Building Descriptions:
  - Alien-flavored text for all structures
- Tutorial/Help:
  - In-game guidance
  - Tooltips
- Performance Optimization
- Bug Fixes
- Balance Tuning
- Multiplayer Polish:
  - Network optimization
  - Lag compensation tuning
  - Reconnection reliability

**Dependencies:** All EPICs complete

---

## Asset Requirements (User-Provided)

The following assets will need to be provided by the user:

### Sprites/Textures
- Terrain tiles (grass, dirt, water, trees, etc.)
- Zone tiles (R/C/I at various densities)
- Building sprites (50-100+ unique buildings)
- Infrastructure sprites (roads, rails, pipes, power lines)
- Service building sprites
- UI elements and icons
- Disaster effects

### Audio
- Background music tracks
- UI sound effects
- Ambient sounds
- Disaster sounds

### Fonts
- Main game font
- UI font

---

## Recommended Development Order

> **Note:** The authoritative phase ordering is in `docs/canon/systems.yaml`. This section reflects that ordering as of canon v0.13.0.

**Phase 1: Core Foundation**
1. **EPIC 0** - Foundation (SDL3, ECS, game loop)
2. **EPIC 1** - Multiplayer Networking (MVP requirement - build early!)
3. **EPIC 2** - Rendering (3D toon-shaded via SDL_GPU)
4. **EPIC 3** - Terrain (map generation, water bodies, vegetation)

**Phase 2: Core Gameplay**
5. **EPIC 4** - Zoning & Building (zone designation, structure materialization)
6. **EPIC 5** - Energy Infrastructure (energy nexuses, conduits, pool model)
7. **EPIC 6** - Water Infrastructure (fluid extractors, reservoirs, pool model) *(moved from Phase 3 — buildings require fluid to develop)*
8. **EPIC 7** - Transportation (pathways, traffic simulation)
9. **EPIC 10** - Simulation Core (population, demand, disorder, contamination)

**Phase 3: Services & UI**
10. **EPIC 9** - City Services (enforcers, hazard response, medical, education)
11. **EPIC 11** - Financial System (tribute, treasury, ordinances)
12. **EPIC 12** - Information & UI (overlays, query tool, statistics)

**Phase 4: Advanced Features**
13. **EPIC 8** - Ports & External Connections (aero port, aqua port)
14. **EPIC 13** - Disasters (catastrophes, emergency response)
15. **EPIC 14** - Progression & Rewards (milestones, arcologies)

**Phase 5: Polish**
16. **EPIC 15** - Audio (ambient music, sound effects)
17. **EPIC 16** - Save/Load (persistence, reconnection)
18. **EPIC 17** - Polish & Alien Theming (terminology, optimization)

---

## Planning Status

| Epic | Tickets Created | Canon Version | Status |
|------|-----------------|---------------|--------|
| Epic 0 | 34 | v0.13.0 | Complete |
| Epic 1 | 22 | v0.13.0 | Complete |
| Epic 2 | 50 | v0.13.0 | Complete |
| Epic 3 | Yes | v0.13.0 | Complete |
| Epic 4 | 48 | v0.13.0 | Complete |
| Epic 5 | Yes | v0.13.0 | Complete |
| Epic 6 | Yes | v0.13.0 | Complete |
| Epic 7 | Yes | v0.13.0 | Complete |
| Epic 8 | 40 | v0.13.0 | Complete |
| Epic 9 | Yes | v0.13.0 | Complete |
| Epic 10 | Yes | v0.13.0 | Complete |
| Epic 11 | Yes | v0.13.0 | Complete |
| Epic 12 | Yes | v0.13.0 | Complete |
| Epic 13-17 | — | — | Pending |

> Last verified: 2026-01-29

---

## Sources

Research based on:
- [SimCity 2000 - Wikipedia](https://en.wikipedia.org/wiki/SimCity_2000)
- [SimCity 2000 - SimCity Fandom Wiki](https://simcity.fandom.com/wiki/SimCity_2000)
- [SimCity 2000 Strategy Guides - GameFAQs](https://gamefaqs.gamespot.com/pc/198648-simcity-2000/faqs)
- [Arcology - SimCity Fandom Wiki](https://simcity.fandom.com/wiki/Arcology)
- [SimCity 2000 Manual - Archive.org](https://archive.org/stream/SimCity.2000-Manual/SimCity.2000-Manual_djvu.txt)
- [StrategyWiki - SimCity 2000 Concepts](https://strategywiki.org/wiki/SimCity_2000/Concepts)
