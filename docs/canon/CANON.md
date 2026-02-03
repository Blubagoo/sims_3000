# ZergCity Project Canon

**This is the source of truth.** All plans, designs, and implementations must conform to this document. If something conflicts with canon, canon wins unless explicitly updated.

Last Updated: 2026-02-01
Canon Version: 0.17.0

---

## How to Use This Document

1. **Before planning:** Read relevant canon sections
2. **While planning:** Conform to canon terminology, patterns, interfaces
3. **If conflict:** Either follow canon OR propose a canon update
4. **Canon updates:** Require explicit decision, documented in changelog

**Agents:** You MUST read this file and relevant YAML files before any planning or implementation work.

---

## Quick Reference

| File | Contents |
|------|----------|
| [terminology.yaml](./terminology.yaml) | What we call things (alien theme) |
| [patterns.yaml](./patterns.yaml) | How we build things (ECS, multiplayer) |
| [interfaces.yaml](./interfaces.yaml) | Contracts between systems |
| [systems.yaml](./systems.yaml) | What each system owns |

---

## Core Principles

These are non-negotiable architectural decisions.

### 1. Multiplayer is Foundational
Multiplayer is not a feature bolted on later. It's a fundamental architecture constraint that affects every system. All designs must account for:
- Dedicated server architecture
- State synchronization
- Player ownership
- Network latency
- Database-backed persistence

### 2. ECS Everywhere
All game state lives in components. All logic lives in systems. Entities are just IDs.
- Components: Pure data, no methods
- Systems: Pure logic, no state
- Entities: Just IDs with component composition
- **Exception:** High-density spatial grid data (terrain, contamination grids) may use dense array storage instead of per-entity ECS registration when data covers every cell and spatial locality is critical. See `patterns.yaml` `dense_grid_exception`.

### 3. Dedicated Server Model
Server is a separate executable, not player-hosted.
- Server: Runs simulation, manages DB persistence
- Clients: Connect to server, render and send input
- Database: Game state constantly saved, survives server restart
- Reconnection: Server down = clients disconnect, reconnect when back

### 4. Endless Sandbox
This is NOT a competitive game with victory conditions.
- No win/lose states
- Endless play - build, prosper, hang out with friends
- Social/casual vibe
- Optional "immortalize" feature to freeze a city as a monument

### 5. Alien Theme Consistency
This is an alien colony builder, not a human city builder. All terminology, UI text, and naming must reflect the alien theme. See [terminology.yaml](./terminology.yaml).

### 6. Tile-Based Ownership
Land ownership is tile-by-tile, not assigned territories.
- Players purchase individual tiles
- Only tile owner can build/modify on that tile
- Tiles can be sold or abandoned
- Abandoned colonies become ghost towns, land returns to market

---

## Project Identity

| Attribute | Value |
|-----------|-------|
| Project Name | ZergCity |
| Internal Codename | Sims 3000 |
| Genre | Multiplayer colony builder sandbox |
| Inspiration | SimCity 2000 |
| Theme | Alien civilization |
| Player Count | 2-4 players (expandable) |
| Game Mode | Endless sandbox (no victory conditions) |
| Vibe | Social, casual, creative |

---

## Tech Stack (Locked)

| Layer | Technology |
|-------|------------|
| Language | C++ |
| Graphics/Input/Audio | SDL3 |
| 3D Rendering | SDL_GPU (toon shading) |
| Art Style | Bioluminescent toon-shaded 3D |
| Art Pipeline | Hybrid (procedural + hand-modeled) |
| Model Format | glTF 2.0 (.glb) |
| Architecture | Custom engine (no external engine) |
| Pattern | ECS (Entity-Component-System) |
| Networking | Client-Server (host-based) |
| Map Size | Configurable per-game (128/256/512) |
| UI | Two modes: Classic + Sci-fi Holographic |

---

## Visual Identity

| Aspect | Detail |
|--------|--------|
| Art Style | 3D toon/cell-shaded — clean outlines, hard shadow edges |
| Color Palette | Bioluminescent — dark base (deep blues/greens/purples) with glowing accents (cyan, green, amber, magenta) |
| Inspiration | Subnautica/Avatar bioluminescence + SimCity gameplay |
| Models | Hybrid pipeline: procedural for zone buildings, hand-modeled for landmarks |
| Terrain | Expanded alien set: ground, hills, water, forest + crystal fields, spore plains, toxic marshes, volcanic rock |
| Audio | User-sourced ambient music + engine SFX for game actions |
| UI Modes | Classic (SimCity 2000 layout) and Sci-fi Holographic (translucent/glowing panels) — toggleable |

See [patterns.yaml](./patterns.yaml) for full art direction, terrain types, building templates, and UI design specs.

---

## Epic Overview

| Phase | Epics | Focus |
|-------|-------|-------|
| Phase 1 | 0, 1, 2, 3 | Foundation (SDL3, Multiplayer, Rendering, Terrain) |
| Phase 2 | 4, 5, 6, 7, 10 | Core Gameplay (Zoning, Power, Water, Transport, Simulation) |
| Phase 3 | 9, 11, 12 | Services & UI (Services, Financial, UI) |
| Phase 4 | 8, 13, 14 | Advanced (Ports, Disasters, Progression) |
| Phase 5 | 15, 16, 17 | Polish (Audio, Save/Load, Theming) |

**Note:** EPIC 6 (Water) moved to Phase 2 - buildings require water to develop.

See [systems.yaml](./systems.yaml) for detailed system boundaries and dependencies.

---

## Canon Change Process

To update canon:

1. **Identify conflict** - What canon rule doesn't fit?
2. **Propose change** - What should canon say instead?
3. **Rationale** - Why is this better?
4. **Impact assessment** - What else changes if we update this?
5. **Decision** - Accept or reject
6. **Update** - Modify canon files, increment version, add changelog entry

---

## Canon Change Log

| Version | Date | Change |
|---------|------|--------|
| 0.1.0 | 2025-01-25 | Initial canon established from epic plan |
| 0.2.0 | 2025-01-25 | Major updates: dedicated server model, sandbox mode, tile ownership, resource pools, phase reordering |
| 0.3.0 | 2026-01-25 | Game Master ownership model, ghost town decay mechanics, tile sizes (64x64), cell-shaded art style |
| 0.4.0 | 2026-01-27 | Switch to 3D rendering: SDL_GPU for 3D graphics, toon shaders, 3D models instead of sprites, isometric-style fixed camera |
| 0.4.1 | 2026-01-28 | Full camera controls: free camera (orbit/pan/zoom/tilt) with isometric preset snap views, replacing fixed isometric-only camera |
| 0.5.0 | 2026-01-28 | Major visual/design canon: bioluminescent art direction, hybrid 3D model pipeline, configurable map sizes, expanded alien terrain types, building template system, audio direction, dual UI modes (Classic + Sci-fi Holographic) |
| 0.6.0 | 2026-01-28 | Epic 3 canon integration: dense grid ECS exception, vegetation ownership split (TerrainSystem designates, RenderingSystem generates visuals), terrain interfaces (ITerrainQueryable, ITerrainRenderData, ITerrainModifier), terrain contamination source, missing terminology (terraform, spawn_point), SporeFlats clearable clarification |
| 0.7.0 | 2026-01-28 | Epic 5 canon integration: IEnergyProvider interface added, IEnergyConsumer 4 priority levels (was 3), IEnergyProducer/IContaminationSource alien terminology (carbon/petrochemical/gaseous), CoverageGrid + BuildingGrid added to dense_grid_exception, EnergySystem tick_priority: 10 |
| 0.8.0 | 2026-01-28 | Epic 6 canon integration: IFluidProvider interface added (mirrors IEnergyProvider pattern), FluidCoverageGrid added to dense_grid_exception, FluidSystem tick_priority: 20 (after EnergySystem, before ZoneSystem) |
| 0.9.0 | 2026-01-29 | Epic 7 canon integration: ITransportProvider interface added (network model, cross-ownership connectivity), TransportSystem tick_priority: 45, RailSystem tick_priority: 47, PathwayGrid + ProximityCache added to dense_grid_exception, TerminalComponent added to RailSystem |
| 0.10.0 | 2026-01-29 | Epic 10 canon integration: ISimulationTime interface added, IGridOverlay interface for overlay rendering, IDemandProvider interface for zone demand queries, IServiceQueryable/IEconomyQueryable stub interfaces for forward compatibility, DemandSystem tick_priority: 52, LandValueSystem tick_priority: 85, DisorderGrid + ContaminationGrid + LandValueGrid added to dense_grid_exception (with double-buffering pattern for circular dependency resolution) |
| 0.11.0 | 2026-01-29 | Epic 9 canon integration: ServicesSystem tick_priority: 55 (after DemandSystem, before EconomySystem), ServiceCoverageGrid added to dense_grid_exception, ServicesSystem replaces Epic 10 IServiceQueryable stub |
| 0.12.0 | 2026-01-29 | Epic 11 canon integration: EconomySystem tick_priority: 60, replaces Epic 10 IEconomyQueryable stub, added IncomeBreakdown/ExpenseBreakdown/CreditAdvance data contracts |
| 0.13.0 | 2026-01-29 | Epic 12 canon integration: UI event types added (UIToolSelectedEvent, UIBuildRequestEvent, UIZoneRequestEvent, UITributeRateChangedEvent, UIFundingChangedEvent), UI terminology additions (probe_function, alert_pulse, comm_panel, colony_treasury_panel, zone_pressure_indicator), UI animation specifications and notification priorities added |
| 0.14.0 | 2026-02-01 | Epic 13 canon integration: IDisasterProvider and IEmergencyResponseProvider interfaces added, DisasterSystem tick_priority: 75, ConflagrationGrid added to dense_grid_exception (3 bytes/tile for fire spread), DisasterSystem components expanded (DisasterComponent, RecoveryZoneComponent, DebrisComponent, EmergencyResponseStateComponent), ISimulatable.implemented_by updated with DisasterSystem |
| 0.15.0 | 2026-02-01 | Epic 14 canon integration: IProgressionProvider and IUnlockPrerequisite interfaces added, ProgressionSystem tick_priority: 90, progression_components and transcendence_area_effects patterns added, edict/transcendence/arcology/milestone terminology added, ProgressionSystem components defined (MilestoneComponent, EdictComponent, ArcologyComponent, RewardBuildingComponent, TranscendenceAuraComponent), "Victory conditions" and "Technology timeline" removed from ProgressionSystem.owns per Core Principle #4 (Endless Sandbox) |
| 0.16.0 | 2026-02-01 | Epic 15 canon integration: AudioSystem changed from ecs_system to core (client-only, doesn't participate in simulation tick), IAudioProvider interface added with play_sound/play_sound_at/volume control/music control methods, audio_patterns section expanded with format recommendations (OGG Vorbis for SFX, streaming for music), positional audio specs (50 tile max, zoom-aware), variant tiers, pause behavior, priority levels, 3-second crossfade requirement |
| 0.17.0 | 2026-02-01 | Epic 16 canon integration: PersistenceSystem expanded with checkpoint system, World Template export, Immortalize feature, Monument Gallery; SettingsManager added for client settings; IPersistenceProvider and ISettingsProvider interfaces added; persistence patterns section added (save file format, component versioning, checkpoint system, multiplayer rollback authority, world template export, immortalize feature, client settings, corruption detection); data contracts added (SaveResult, LoadResult, CompatibilityResult, SaveSlotMetadata, AutoSaveConfig, ClientSettings) |
