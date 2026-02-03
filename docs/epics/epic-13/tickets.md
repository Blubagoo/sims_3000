# Epic 13: Disasters - Tickets

Generated: 2026-02-01
Canon Version: 0.13.0
Planning Agents: systems-architect, game-designer, services-engineer

## Overview

Epic 13 implements the **DisasterSystem** - the catastrophe simulation that brings dynamic threats to colonies. This epic introduces random and manual disaster triggering, damage mechanics, emergency response integration, and recovery tracking. The system is server-authoritative with support for disasters spanning ownership boundaries in multiplayer.

The DisasterSystem runs at tick_priority 75, positioned after ServicesSystem (55) for hazard response coverage data and DisorderSystem (70) for civil_unrest triggers, and before ContaminationSystem (80) to emit core_breach contamination in the same tick. The system manages disaster entities through a state machine (Warning -> Active -> Recovery -> Complete) and coordinates with BuildingSystem for damage application via DamageEvent.

Fire mechanics use a dedicated **ConflagrationGrid** (dense grid pattern) for efficient spatial simulation of spreading fires. Per canon's "cozy sandbox" vibe, disaster frequency defaults to rare (~1 minor per 100 cycles, ~1 major per 500 cycles) with full disable options for sandbox play. Disasters create memorable moments and stories rather than constant punishment, following the emotional arc: Anticipation -> Tension -> Relief -> Determination -> Pride.

## Ticket Summary

| Group | Tickets | Focus |
|-------|---------|-------|
| A | 13-001 to 13-008 | Core Infrastructure (enums, components, events, interfaces) |
| B | 13-009 to 13-014 | Damage System (IDamageable, calculation, integration) |
| C | 13-015 to 13-021 | Fire System (ConflagrationGrid, spread, extinguishment) |
| D | 13-022 to 13-031 | Disaster Types (10 disaster implementations) |
| E | 13-032 to 13-037 | Emergency Response Integration (dispatch, effectiveness) |
| F | 13-038 to 13-042 | Recovery System (debris, rebuild, statistics) |
| G | 13-043 to 13-047 | Triggering & Settings (random, manual, sandbox toggles) |
| H | 13-048 to 13-052 | Multiplayer (cross-boundary, sync, notifications) |
| I | 13-053 to 13-055 | UI Integration (notifications, overlays) |
| J | 13-056 to 13-060 | Testing (unit, integration, performance) |

## Tickets

---

### Group A: Core Infrastructure

---

### 13-001: Define DisasterType Enum and DisasterConfig

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Define the DisasterType enum with all canonical disaster types using alien terminology. Create the DisasterConfig struct containing configuration parameters for each disaster type (warning duration, damage, radius, probabilities).

**Acceptance Criteria:**
- [ ] DisasterType enum with 10 types: Conflagration, SeismicEvent, Inundation, VortexStorm, MegaStorm, CoreBreach, CivilUnrest, TitanEmergence, VoidAnomaly, CosmicRadiation
- [ ] DisasterConfig struct with warning_duration_ticks, min/max_duration_ticks, base_damage_per_tick, base_radius, max_radius, spread_rate, random_probability
- [ ] Default configurations per disaster type
- [ ] Canonical terminology from terminology.yaml used throughout

**Dependencies:**
- Depends on: None (foundation)
- Blocks: 13-002, 13-003, 13-022 through 13-031

**Agent Notes:**
- Systems Architect: Ensure config values match proposed behavior (conflagration no warning, seismic 5-10s warning, etc.)
- Game Designer: Default frequencies should match recommended ~1 minor/100 cycles, ~1 major/500 cycles

---

### 13-002: Define DisasterComponent

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Create the core DisasterComponent that tracks active disaster state including type, phase, epicenter, radius, severity, timing, and affected players.

**Acceptance Criteria:**
- [ ] DisasterComponent with: disaster_type, phase, epicenter (GridPosition), radius, max_radius, severity (0-100), tick_started, tick_active, duration_ticks, ticks_remaining, is_natural flag
- [ ] affected_players array (up to 4 players) with affected_player_count
- [ ] Default initialization values appropriate for each field

**Dependencies:**
- Depends on: 13-001 (DisasterType enum)
- Blocks: 13-006, 13-022 through 13-031

**Agent Notes:**
- Systems Architect: Use fixed-size array for affected_players to avoid dynamic allocation
- Game Designer: is_natural flag distinguishes random vs player-triggered disasters

---

### 13-003: Define DisasterPhase Enum and State Machine

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Define the DisasterPhase enum representing disaster lifecycle states and document the state machine transitions.

**Acceptance Criteria:**
- [ ] DisasterPhase enum: Warning, Active, Recovery, Complete
- [ ] State transition documentation: Warning -> Active (warning expires), Active -> Recovery (damage ends), Recovery -> Complete (debris cleared)
- [ ] Helper functions for phase transitions

**Dependencies:**
- Depends on: 13-001
- Blocks: 13-006, 13-038

**Agent Notes:**
- Systems Architect: Each disaster entity progresses through this state machine independently
- Game Designer: Warning phase is crucial for anticipation emotion; Complete phase triggers cleanup

---

### 13-004: Define DamageType Enum

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Create the DamageType enum categorizing different types of damage that disasters can inflict.

**Acceptance Criteria:**
- [ ] DamageType enum: Fire, Seismic, Flood, Storm, Explosion
- [ ] Mapping from DisasterType to DamageType(s) documented
- [ ] Multiple damage types per disaster supported (e.g., CoreBreach = Explosion + contamination)

**Dependencies:**
- Depends on: 13-001
- Blocks: 13-005, 13-009, 13-010

**Agent Notes:**
- Systems Architect: DamageType maps to resistance categories on DamageableComponent
- Game Designer: Keep damage types distinct for meaningful resistance differentiation

---

### 13-005: Define DamageEvent and Disaster Events

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Define all events emitted by DisasterSystem for integration with other systems (BuildingSystem, UISystem, AudioSystem).

**Acceptance Criteria:**
- [ ] DamageEvent: target EntityID, amount, DamageType, source disaster EntityID, source_position
- [ ] DisasterWarningEvent: type, epicenter, severity, warning_ticks, affected_players vector
- [ ] DisasterActiveEvent: disaster_id, type, epicenter, severity
- [ ] DisasterPhaseChangeEvent: disaster_id, old_phase, new_phase
- [ ] DisasterEndedEvent: disaster_id, type, duration_ticks, buildings_destroyed, beings_displaced, total_damage
- [ ] BuildingDestroyedByDisasterEvent: building_id, disaster_id, damage_type, owner, position

**Dependencies:**
- Depends on: 13-001, 13-003, 13-004
- Blocks: 13-011, 13-053, 13-054

**Agent Notes:**
- Systems Architect: DamageEvent is the primary interface between DisasterSystem and BuildingSystem
- Game Designer: Events enable the emotional arc (warning -> active -> ended creates anticipation/relief)

---

### 13-006: Create DisasterSystem Class Skeleton

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Create the DisasterSystem class implementing ISimulatable with tick_priority 75. Set up basic tick structure with placeholders for disaster processing phases.

**Acceptance Criteria:**
- [ ] DisasterSystem class implementing ISimulatable
- [ ] tick_priority = 75 (after ServicesSystem 55, DisorderSystem 70; before ContaminationSystem 80)
- [ ] Tick method structure with phases: process random triggers, update active disasters, process emergency response, process recovery zones, emit events
- [ ] Registry access for disaster entity queries
- [ ] Integration points for IEmergencyResponseProvider (stub initially)

**Dependencies:**
- Depends on: 13-001, 13-002, 13-003, 13-005
- Blocks: 13-015, 13-022 through 13-031, 13-032

**Agent Notes:**
- Systems Architect: tick_priority 75 ensures coverage data and disorder levels are available
- Services Engineer: ServicesSystem integration via IEmergencyResponseProvider interface

---

### 13-007: Define IDisasterProvider Interface

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Create the IDisasterProvider interface allowing other systems to query disaster state (active disasters, fire positions, recovery zones, statistics).

**Acceptance Criteria:**
- [ ] get_active_disasters() -> vector<DisasterInfo>
- [ ] get_disaster_at(GridPosition) -> optional<DisasterInfo>
- [ ] is_in_disaster_zone(GridPosition) -> bool
- [ ] get_disaster_type_at(GridPosition) -> optional<DisasterType>
- [ ] is_on_fire(x, y) -> bool
- [ ] get_fire_intensity(x, y) -> uint8_t
- [ ] get_all_fires() -> vector<pair<GridPosition, uint8_t>>
- [ ] get_recovery_zones() -> vector<RecoveryZoneInfo>
- [ ] is_in_recovery(GridPosition) -> bool
- [ ] get_statistics(PlayerID) -> DisasterStatistics
- [ ] get_total_damage(PlayerID) -> int64_t
- [ ] DisasterInfo, RecoveryZoneInfo, DisasterStatistics structs defined

**Dependencies:**
- Depends on: 13-001, 13-002, 13-003
- Blocks: 13-053, 13-054

**Agent Notes:**
- Systems Architect: Fire queries used by RenderingSystem for visualization
- Game Designer: Statistics feed into progression milestones

---

### 13-008: Define OnFireComponent

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Create the OnFireComponent for tracking per-entity fire state when buildings are actively burning.

**Acceptance Criteria:**
- [ ] OnFireComponent: intensity (0-255), tick_ignited, ticks_burning, is_spreading, is_being_extinguished, extinguish_progress (0-100%)
- [ ] Component is added when entity ignites, removed when extinguished
- [ ] Intensity decays over time without intervention

**Dependencies:**
- Depends on: 13-001
- Blocks: 13-017, 13-018

**Agent Notes:**
- Systems Architect: This component tracks entity-level fire state; ConflagrationGrid tracks tile-level
- Game Designer: extinguish_progress provides visual feedback during suppression

---

### Group B: Damage System

---

### 13-009: Define DamageableComponent

**Priority:** P0
**Size:** S
**System:** DisasterSystem, BuildingSystem

**Description:**
Create the DamageableComponent storing health, max health, damage resistances, and destruction state.

**Acceptance Criteria:**
- [ ] DamageableComponent: health (uint32_t), max_health (uint32_t), is_destroyed, is_repairable, last_damage_tick
- [ ] DamageResistance struct: fire, seismic, flood, storm, explosion (each 0-100% as uint8_t)
- [ ] resistances field on DamageableComponent
- [ ] Default values appropriate (100 health, 0% resistance for most buildings)

**Dependencies:**
- Depends on: 13-004 (DamageType)
- Blocks: 13-010, 13-011, 13-012

**Agent Notes:**
- Systems Architect: Single source of truth for entity health; applies to buildings AND infrastructure
- Services Engineer: Service buildings get 40% average resistance per decision

---

### 13-010: Implement IDamageable Interface

**Priority:** P0
**Size:** M
**System:** DisasterSystem, BuildingSystem

**Description:**
Implement the IDamageable interface from canon (interfaces.yaml) for buildings and infrastructure entities.

**Acceptance Criteria:**
- [ ] IDamageable interface: get_health(), get_max_health(), apply_damage(amount, DamageType), is_destroyed()
- [ ] Interface implemented for building entities
- [ ] Resistance calculation applied in apply_damage()
- [ ] Health clamped to 0 minimum

**Dependencies:**
- Depends on: 13-004, 13-009
- Blocks: 13-011

**Agent Notes:**
- Systems Architect: Per canon interfaces.yaml, this is how damage is applied to entities
- Game Designer: Resistance makes building material choices meaningful

---

### 13-011: Implement Damage Calculation Pipeline

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the damage calculation pipeline including distance falloff, resistance application, and DamageEvent emission.

**Acceptance Criteria:**
- [ ] Distance falloff: damage = base_damage * (1.0 - distance/max_radius)
- [ ] Resistance application: final_damage = calculated_damage * (1 - resistance/100)
- [ ] DamageEvent emitted for each damaged entity per tick
- [ ] Batch processing of damage within disaster radius
- [ ] Spatial queries to find damageable entities in affected area

**Dependencies:**
- Depends on: 13-005, 13-009, 13-010
- Blocks: 13-012, 13-022 through 13-031

**Agent Notes:**
- Systems Architect: Use spatial index for efficient entity queries within radius
- Game Designer: Falloff creates natural "epicenter is worst" behavior

---

### 13-012: BuildingSystem DamageEvent Handling

**Priority:** P0
**Size:** M
**System:** BuildingSystem

**Description:**
Implement BuildingSystem handling of DamageEvent to reduce building health and trigger destruction when health reaches zero.

**Acceptance Criteria:**
- [ ] BuildingSystem subscribes to DamageEvent
- [ ] Health reduced on DamageableComponent
- [ ] last_damage_tick updated on damage
- [ ] Building state transitions based on health thresholds (100-51% Active, 50-26% Damaged, 25-1% Derelict, 0% Destroyed)
- [ ] At health=0: is_destroyed=true, BuildingDestroyedByDisasterEvent emitted
- [ ] Destroyed buildings create debris entities at their position

**Dependencies:**
- Depends on: 13-005, 13-009
- Blocks: 13-038, 13-039

**Agent Notes:**
- Systems Architect: Building states (Active/Damaged/Derelict) for visual feedback
- Game Designer: Visual damage before destruction allows player response

---

### 13-013: Infrastructure DamageEvent Handling

**Priority:** P1
**Size:** M
**System:** TransportSystem, EnergySystem, FluidSystem

**Description:**
Implement damage handling for infrastructure entities (pathways, energy conduits, fluid conduits).

**Acceptance Criteria:**
- [ ] Add DamageableComponent to infrastructure entities
- [ ] Infrastructure-specific health and resistance values (roads: 50hp/90% fire/20% seismic; conduits: 100hp/70% fire/30% seismic)
- [ ] Damaged infrastructure reduces effectiveness (congestion, flow reduction)
- [ ] Destroyed infrastructure disconnects networks
- [ ] DamageEvent handling in relevant systems

**Dependencies:**
- Depends on: 13-005, 13-009
- Blocks: None (enhancement)

**Agent Notes:**
- Systems Architect: Seismic events preferentially damage infrastructure
- Game Designer: Infrastructure damage creates ripple effects (power/water outages)

---

### 13-014: Building Health and Resistance by Type

**Priority:** P1
**Size:** M
**System:** BuildingSystem

**Description:**
Configure default health and resistance values for all building categories based on proposed values.

**Acceptance Criteria:**
- [ ] Low-density habitation: 100hp, 10% fire, 20% seismic
- [ ] High-density habitation: 200hp, 30% fire, 40% seismic
- [ ] Exchange buildings: 150hp, 20% fire, 30% seismic
- [ ] Fabrication buildings: 250hp, 50% fire, 50% seismic
- [ ] Energy nexus (carbon): 300hp, 10% fire, 40% seismic
- [ ] Energy nexus (nuclear): 500hp, 80% fire, 60% seismic
- [ ] Service buildings: 200hp, 40% average resistance (hazard posts higher fire resist per services-engineer)
- [ ] Data-driven configuration for easy tuning

**Dependencies:**
- Depends on: 13-009
- Blocks: None (tuning)

**Agent Notes:**
- Systems Architect: Values from analysis; make data-driven for tuning
- Services Engineer: Service buildings get elevated resistance (40% average) to prevent death spirals

---

### Group C: Fire System

---

### 13-015: ConflagrationGrid Data Structure

**Priority:** P0
**Size:** L
**System:** DisasterSystem

**Description:**
Implement the ConflagrationGrid dense grid for fire spread simulation. Stores fire intensity, flammability, and fuel per tile.

**Acceptance Criteria:**
- [ ] ConflagrationGrid class with fire_intensity_, flammability_, fuel_remaining_ arrays (3 bytes/tile)
- [ ] Constructor takes map dimensions (width, height)
- [ ] Memory: 48KB for 128x128, 192KB for 256x256, 768KB for 512x512
- [ ] Thread-safe read operations for RenderingSystem access
- [ ] Dirty chunk tracking (32x32 chunks) for optimized iteration
- [ ] Dense grid exception pattern followed per canon

**Dependencies:**
- Depends on: 13-006
- Blocks: 13-016, 13-017, 13-018

**Agent Notes:**
- Systems Architect: dense_grid_exception pattern like TerrainGrid, ServiceCoverageGrid
- Game Designer: Efficient fire spread is critical for responsive gameplay

---

### 13-016: ConflagrationGrid Read/Write Operations

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement read and write operations for ConflagrationGrid including ignition, extinguishment, and intensity queries.

**Acceptance Criteria:**
- [ ] get_intensity_at(x, y) -> uint8_t
- [ ] is_on_fire(x, y) -> bool (intensity > 0)
- [ ] get_flammability_at(x, y) -> uint8_t
- [ ] ignite(x, y, intensity) - starts fire at position
- [ ] extinguish(x, y) - removes fire at position
- [ ] update_intensity(x, y, delta) - adjust intensity (+ or -)
- [ ] extinguish_in_radius(x, y, radius, strength) - area extinguishment for responders
- [ ] Bounds checking on all operations

**Dependencies:**
- Depends on: 13-015
- Blocks: 13-017, 13-018, 13-019, 13-022

**Agent Notes:**
- Systems Architect: extinguish_in_radius used by hazard response integration
- Game Designer: ignite() should mark dirty chunks for spread processing

---

### 13-017: Flammability Calculation

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Calculate per-tile flammability based on terrain type and building presence. Updates when terrain or buildings change.

**Acceptance Criteria:**
- [ ] Base flammability by terrain type: BiolumGrove 220, SporeFlats 200, Water 0, EmberCrust 60, default 128
- [ ] Building presence modifies flammability (fabrication increases, concrete decreases)
- [ ] Flammability recalculated on terrain change or building placement/destruction
- [ ] Integration with ITerrainQueryable for terrain type
- [ ] Fuel values set based on flammability (vegetation = high fuel)

**Dependencies:**
- Depends on: 13-015, 13-016
- Blocks: 13-018, 13-022

**Agent Notes:**
- Systems Architect: Query ITerrainQueryable from Epic 3 for terrain types
- Game Designer: Biolum groves (forests) are highly flammable - creates natural fire hazard zones

---

### 13-018: Fire Spread Algorithm

**Priority:** P0
**Size:** L
**System:** DisasterSystem

**Description:**
Implement the fire spread algorithm using double-buffering to avoid order-dependent spreading.

**Acceptance Criteria:**
- [ ] spread_tick() processes all fire spread for one simulation tick
- [ ] Double-buffer pattern: copy intensity, process spread, swap buffers
- [ ] Spread to 4-connected neighbors (not diagonal)
- [ ] Spread chance = (intensity/255) * (neighbor_flammability/255)
- [ ] New fire intensity = source_intensity * 0.7
- [ ] Natural decay: intensity -= 2 per tick base
- [ ] Fuel consumption: fuel_remaining -= FUEL_CONSUMPTION per tick
- [ ] Faster decay when fuel exhausted: intensity -= 5 additional
- [ ] Fire extinguishes when intensity reaches 0
- [ ] Only process dirty chunks for efficiency

**Dependencies:**
- Depends on: 13-015, 13-016, 13-017
- Blocks: 13-019, 13-022

**Agent Notes:**
- Systems Architect: Double-buffer critical for deterministic spread; dirty chunk tracking for performance
- Game Designer: Spread rate creates player response window - not too fast, not too slow

---

### 13-019: Fire Damage Application

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Apply continuous damage to entities in burning tiles based on fire intensity.

**Acceptance Criteria:**
- [ ] Each tick, iterate tiles with fire intensity > 0
- [ ] Query entities at burning tiles
- [ ] Calculate damage based on intensity: damage = (intensity / 255) * base_fire_damage
- [ ] Emit DamageEvent for each entity
- [ ] Add OnFireComponent to burning building entities
- [ ] Update OnFireComponent.ticks_burning

**Dependencies:**
- Depends on: 13-005, 13-008, 13-016, 13-018
- Blocks: 13-022

**Agent Notes:**
- Systems Architect: Fire damage is continuous while tile burns
- Game Designer: Intensity-based damage means severe fires are more dangerous

---

### 13-020: Fire Extinguishment Mechanics

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement fire extinguishment from hazard response coverage and responder dispatch.

**Acceptance Criteria:**
- [ ] Passive suppression from coverage: if hazard_coverage >= threshold, spread_rate *= (1 - coverage * 0.7)
- [ ] Active suppression from dispatch: intensity -= suppression_power * response_effectiveness
- [ ] Update OnFireComponent.is_being_extinguished when responder active
- [ ] Update OnFireComponent.extinguish_progress based on suppression applied
- [ ] Remove OnFireComponent when extinguished
- [ ] Integration with IServiceQueryable for coverage queries

**Dependencies:**
- Depends on: 13-008, 13-016, 13-032
- Blocks: 13-022

**Agent Notes:**
- Systems Architect: Coverage from ServicesSystem Epic 9; dispatch from IEmergencyResponseProvider
- Services Engineer: Passive coverage prevents spread; active dispatch suppresses existing fires

---

### 13-021: Fire Visualization Data Provider

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Expose fire state to RenderingSystem for visualization through IDisasterProvider.

**Acceptance Criteria:**
- [ ] get_all_fire_positions() returns vector of positions with fire
- [ ] get_fires_with_intensity() returns position + intensity pairs
- [ ] Efficient iteration over active fires only (not entire grid)
- [ ] Thread-safe read access for render thread
- [ ] FireStateUpdateMessage struct for network sync

**Dependencies:**
- Depends on: 13-007, 13-015, 13-016
- Blocks: None (render integration)

**Agent Notes:**
- Systems Architect: RenderingSystem visualizes fire; needs efficient query
- Game Designer: Fire visualization is critical for player awareness and spectacle

---

### Group D: Disaster Types

---

### 13-022: Conflagration (Fire) Disaster Implementation

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the conflagration disaster type with spreading fire mechanics.

**Acceptance Criteria:**
- [ ] No warning phase (instant ignition)
- [ ] Creates initial ignition point via ConflagrationGrid.ignite()
- [ ] Duration based on fire spread (ends when all fires extinguished)
- [ ] Severity affects initial intensity and spread rate
- [ ] Triggered by: random chance (higher near fabrication), cascade from other disasters, manual
- [ ] Fire spread handled by existing ConflagrationGrid mechanics

**Dependencies:**
- Depends on: 13-006, 13-015 through 13-020
- Blocks: 13-056

**Agent Notes:**
- Systems Architect: Conflagration is special - duration isn't fixed, it's "until extinguished"
- Game Designer: Core disaster type; most common and interactive

---

### 13-023: Seismic Event (Earthquake) Disaster Implementation

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the seismic event disaster with instant damage in radius and secondary fire chance.

**Acceptance Criteria:**
- [ ] Warning phase: 100 ticks (tremors)
- [ ] Active duration: 20-60 ticks (very short, intense)
- [ ] Instant damage application on Active phase start
- [ ] Damage falloff from epicenter: damage *= (1 - distance/radius)
- [ ] Secondary fire chance: 5% per tile in radius, affected by falloff
- [ ] Infrastructure preferentially damaged (lower seismic resistance)
- [ ] Screen shake event emitted for visual effect

**Dependencies:**
- Depends on: 13-006, 13-011, 13-016 (for secondary fires)
- Blocks: 13-056

**Agent Notes:**
- Systems Architect: Instant damage pattern differs from continuous fire damage
- Game Designer: Secondary fires create cascade potential - exciting but contained

---

### 13-024: Inundation (Flood) Disaster Implementation

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the inundation disaster with water level rise based on elevation.

**Acceptance Criteria:**
- [ ] Warning phase: 600 ticks (30 seconds - rising water visible)
- [ ] Duration-based: water level rises, peaks, recedes over 400-1000 ticks
- [ ] Flood level = base_level + severity * phase_multiplier
- [ ] Tiles at elevation <= flood_level and adjacent to water are flooded
- [ ] Buildings take continuous damage while submerged
- [ ] Energy and fluid systems disabled in flooded area
- [ ] Requires water bodies adjacent to affected area (prerequisite check)

**Dependencies:**
- Depends on: 13-006, 13-011
- Blocks: 13-057

**Agent Notes:**
- Systems Architect: Integration with ITerrainQueryable for elevation data
- Game Designer: Slow warning period allows preparation; affects low-lying areas

---

### 13-025: Vortex Storm (Tornado) Disaster Implementation

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the vortex storm disaster as a mobile path-based disaster.

**Acceptance Criteria:**
- [ ] Warning phase: 300 ticks (forming funnel visible)
- [ ] Mobile disaster: position updates each tick
- [ ] Movement: position += direction * speed; direction += random_deviation(-10, +10 degrees)
- [ ] Narrow damage corridor (path_width tiles)
- [ ] High damage but short duration (300-600 ticks)
- [ ] Debris throw mechanic: chance to damage adjacent tiles
- [ ] Ends when position out of bounds or duration expires

**Dependencies:**
- Depends on: 13-006, 13-011
- Blocks: 13-057

**Agent Notes:**
- Systems Architect: First mobile disaster - needs position tracking per tick
- Game Designer: Semi-random path creates uncertainty; visual spectacle

---

### 13-026: Mega Storm (Hurricane) Disaster Implementation

**Priority:** P2
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the mega storm disaster with large-area moderate damage over long duration.

**Acceptance Criteria:**
- [ ] Warning phase: 1200 ticks (60 seconds - weather pattern forming)
- [ ] Large radius, potentially map-wide
- [ ] Long duration: 600-1200 ticks
- [ ] Moderate damage per tick (lower than vortex)
- [ ] Reduces energy output from solar/wind sources
- [ ] Can trigger secondary inundation near water bodies
- [ ] Storm damage type

**Dependencies:**
- Depends on: 13-006, 13-011, 13-024 (cascade)
- Blocks: 13-057

**Agent Notes:**
- Systems Architect: Integration with EnergySystem for output reduction
- Game Designer: Map-wide effect creates shared experience in multiplayer

---

### 13-027: Core Breach (Nuclear Meltdown) Disaster Implementation

**Priority:** P0
**Size:** L
**System:** DisasterSystem

**Description:**
Implement the core breach disaster triggered by nuclear nexus failure with explosion and long-term contamination.

**Acceptance Criteria:**
- [ ] Warning phase: 400 ticks (reactor alarms, evacuation possible)
- [ ] Triggered when nuclear nexus takes critical damage OR random low probability based on age/maintenance
- [ ] Phase 1 - Explosion: instant explosion damage in immediate radius
- [ ] Phase 2 - Fire: secondary conflagration at epicenter
- [ ] Phase 3 - Contamination: long-term ContaminationType::Energy contamination zone
- [ ] Contamination persists after disaster ends (ContaminationSystem integration)
- [ ] Very high severity cap

**Dependencies:**
- Depends on: 13-006, 13-011, 13-016, ContaminationSystem (Epic 10)
- Blocks: 13-056

**Agent Notes:**
- Systems Architect: Multi-phase disaster; integration with ContaminationSystem for long-term effects
- Game Designer: Consequence of nuclear power choice; dramatic but rare

---

### 13-028: Civil Unrest (Riot) Disaster Implementation

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the civil unrest disaster spreading along pathways when disorder is high.

**Acceptance Criteria:**
- [ ] Warning phase: 200 ticks (crowd forming)
- [ ] Triggered when disorder_index > 80% OR random with probability scaling with disorder
- [ ] Spreads along pathway connections (NOT terrain-based)
- [ ] Respects player ownership boundaries (exception to cross-boundary rule)
- [ ] Damages exchange zones preferentially
- [ ] Suppressed by enforcer coverage (see 13-036)
- [ ] Spread chance affected by local disorder level

**Dependencies:**
- Depends on: 13-006, 13-011, DisorderSystem (Epic 10), TransportSystem (Epic 7)
- Blocks: 13-057

**Agent Notes:**
- Systems Architect: Query DisorderSystem for disorder levels; query TransportSystem for pathway connectivity
- Game Designer: Only disaster that respects boundaries - thematically tied to player's population

---

### 13-029: Titan Emergence (Monster Attack) Disaster Implementation

**Priority:** P2
**Size:** L
**System:** DisasterSystem

**Description:**
Implement the titan emergence disaster as a large mobile entity targeting high-value buildings.

**Acceptance Criteria:**
- [ ] Warning phase: 600 ticks (emergence detected, ground rumbling)
- [ ] Only available late-game (requires population threshold of 500+)
- [ ] Mobile entity with target-seeking behavior
- [ ] Target priority: Energy nexuses > High-density structures > Command nexus
- [ ] Semi-intelligent pathing: move toward target, attack when adjacent
- [ ] Extremely high damage, limited duration (900-1800 ticks)
- [ ] Retreats to emergence point and disappears when duration expires
- [ ] Seismic damage (stomp) + optional fire damage

**Dependencies:**
- Depends on: 13-006, 13-011
- Blocks: 13-058

**Agent Notes:**
- Systems Architect: Most complex mobile disaster; needs target-seeking behavior
- Game Designer: Majestic and terrifying, not malicious - native creature disturbed by colony; targets energy because drawn to energy signatures

---

### 13-030: Void Anomaly (Cosmic Event) Disaster Implementation

**Priority:** P2
**Size:** L
**System:** DisasterSystem

**Description:**
Implement the void anomaly disaster with reality distortion effects unique to alien theme.

**Acceptance Criteria:**
- [ ] No warning (sudden cosmic occurrence)
- [ ] Duration: 400-800 ticks
- [ ] Effect selection (2-3 per occurrence): Spatial displacement (teleport buildings), Temporal stutter (pause zone), Reality tear (damage zone), Energy drain (power loss), Population displacement, Infrastructure scramble
- [ ] Effects selected randomly based on severity
- [ ] Expanding sphere of effect
- [ ] Explosion damage type for direct damage
- [ ] Displaced buildings keep zone designation, utilities auto-reconnect
- [ ] Potential for permanent "void zone" terrain feature (extreme severity)

**Dependencies:**
- Depends on: 13-006, 13-011
- Blocks: 13-058

**Agent Notes:**
- Systems Architect: Most unique disaster; multiple effect types create implementation complexity
- Game Designer: Crown jewel alien disaster - visually spectacular, mechanically unique; not just damage but puzzles

---

### 13-031: Cosmic Radiation Disaster Implementation

**Priority:** P2
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the cosmic radiation disaster as a global event affecting the entire map.

**Acceptance Criteria:**
- [ ] Warning phase: 200 ticks (aurora effects visible)
- [ ] Global effect (entire map, all players)
- [ ] Duration: 300-600 ticks
- [ ] Increases contamination everywhere (ContaminationType::Energy)
- [ ] Reduces population growth during event
- [ ] Affects being health (reduces longevity during event)
- [ ] No direct structural damage
- [ ] Linked to void_anomaly occurrence chance

**Dependencies:**
- Depends on: 13-006, ContaminationSystem (Epic 10), PopulationSystem
- Blocks: 13-058

**Agent Notes:**
- Systems Architect: Global disaster - affects all players simultaneously
- Game Designer: Shared experience in multiplayer; creates "remember when" stories

---

### Group E: Emergency Response Integration

---

### 13-032: IEmergencyResponder Building Interface

**Priority:** P0
**Size:** M
**System:** ServicesSystem

**Description:**
Implement the IEmergencyResponder interface for service buildings that can respond to disasters.

**Acceptance Criteria:**
- [ ] IEmergencyResponder interface from canon: can_respond_to(DisasterType), dispatch_to(GridPosition), get_response_effectiveness()
- [ ] Implemented by hazard response buildings (fires/conflagrations)
- [ ] Implemented by enforcer buildings (civil_unrest)
- [ ] Response effectiveness based on funding_effectiveness from Epic 9

**Dependencies:**
- Depends on: 13-001, ServicesSystem (Epic 9)
- Blocks: 13-033, 13-034

**Agent Notes:**
- Systems Architect: Per-building interface as defined in canon interfaces.yaml
- Services Engineer: Ties into existing funding effectiveness calculation

---

### 13-033: IEmergencyResponseProvider Aggregation Interface

**Priority:** P0
**Size:** M
**System:** ServicesSystem

**Description:**
Create the IEmergencyResponseProvider interface owned by ServicesSystem for aggregating emergency responders.

**Acceptance Criteria:**
- [ ] get_available_responders(DisasterType, PlayerID) -> vector<EntityID>
- [ ] request_dispatch(EntityID, GridPosition) -> bool
- [ ] release_responder(EntityID)
- [ ] get_response_effectiveness(EntityID) -> float
- [ ] get_response_time_ticks(EntityID, GridPosition) -> uint32_t
- [ ] DisasterSystem uses this interface, not direct ECS queries
- [ ] Maintains internal mapping of responder assignments

**Dependencies:**
- Depends on: 13-032
- Blocks: 13-034, 13-035

**Agent Notes:**
- Systems Architect: ServicesSystem owns aggregation for encapsulation and query efficiency
- Services Engineer: Pattern matches how other systems query services

---

### 13-034: Hazard Response Dispatch Algorithm

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the dispatch algorithm for hazard response to fires using intensity-first with distance tiebreaker.

**Acceptance Criteria:**
- [ ] Find fires within response radius of each hazard responder
- [ ] Sort by intensity (descending), then distance (ascending) as tiebreaker
- [ ] Intensity tiebreaker threshold: within 10% considered equal
- [ ] Dispatch to highest priority fire
- [ ] Track active responses to avoid double-dispatch
- [ ] Optional manual override: player can click to dispatch

**Dependencies:**
- Depends on: 13-015, 13-016, 13-033
- Blocks: 13-020

**Agent Notes:**
- Systems Architect: Intensity-first prevents exponential spread
- Services Engineer: Per decision, intensity-first is primary, distance tiebreaker

---

### 13-035: EmergencyResponseStateComponent

**Priority:** P0
**Size:** S
**System:** ServicesSystem

**Description:**
Create the EmergencyResponseStateComponent for tracking response capacity per service building.

**Acceptance Criteria:**
- [ ] max_concurrent_responses: 1 for post, 3 for station, 5 for nexus
- [ ] current_response_count: active dispatches
- [ ] response_radius: 15/25/35 tiles (1.5-2x coverage radius)
- [ ] cooldown_until_tick: when available after completion
- [ ] can_accept_dispatch() helper method
- [ ] Attached to hazard response and enforcer buildings

**Dependencies:**
- Depends on: 13-032
- Blocks: 13-034, 13-036

**Agent Notes:**
- Systems Architect: Component auto-syncs over network; actual target list stored in ServicesSystem
- Services Engineer: 1/3/5 capacity per tier provides meaningful differentiation

---

### 13-036: Enforcer Riot Suppression

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Implement enforcer coverage suppression of civil unrest using gradual reduction.

**Acceptance Criteria:**
- [ ] Query enforcer coverage at riot tiles
- [ ] Suppression rate: base_rate * enforcer_coverage * funding_effectiveness
- [ ] Riot intensity reduced by suppression_rate per tick
- [ ] Coverage also prevents spread: spread_chance *= (1 - coverage * 0.9)
- [ ] Overwhelm threshold: if intensity > 200, suppression effectiveness halved
- [ ] Riot ends when intensity reaches 0

**Dependencies:**
- Depends on: 13-028, 13-033, 13-035
- Blocks: 13-057

**Agent Notes:**
- Systems Architect: Gradual suppression creates tension while resolving riot
- Services Engineer: Coverage prevents spread; dispatch accelerates suppression

---

### 13-037: Response Effectiveness and Time Calculation

**Priority:** P1
**Size:** S
**System:** ServicesSystem

**Description:**
Implement response effectiveness and time calculations for emergency dispatch.

**Acceptance Criteria:**
- [ ] Effectiveness: base * distance_factor * tier_bonus
- [ ] distance_factor = 1.0 - (distance / max_response_radius) * 0.3
- [ ] tier_bonus: 1.0 (post), 1.15 (station), 1.3 (nexus)
- [ ] Response time: max(5, distance / speed_factor)
- [ ] speed_factor: 1.0 (post), 1.5 (station), 2.0 (nexus)
- [ ] Minimum 5 ticks even for adjacent fires

**Dependencies:**
- Depends on: 13-033, 13-035
- Blocks: None (tuning)

**Agent Notes:**
- Systems Architect: Distance and tier affect both effectiveness and speed
- Game Designer: Minimum time creates realistic "gear up and go" feel

---

### Group F: Recovery System

---

### 13-038: RecoveryZoneComponent Definition

**Priority:** P0
**Size:** S
**System:** DisasterSystem

**Description:**
Create the RecoveryZoneComponent for tracking disaster aftermath zones.

**Acceptance Criteria:**
- [ ] RecoveryZoneComponent: source_disaster (DisasterType), debris_remaining (count), buildings_destroyed (count), tick_disaster_ended, auto_rebuild_eligible (bool)
- [ ] GridRectComponent attached for zone bounds
- [ ] Created when disaster transitions to Recovery phase

**Dependencies:**
- Depends on: 13-001, 13-003
- Blocks: 13-039, 13-040

**Agent Notes:**
- Systems Architect: Recovery zone is an entity tracking the aftermath area
- Game Designer: auto_rebuild_eligible allows zones to develop once cleared

---

### 13-039: DebrisComponent and Creation

**Priority:** P0
**Size:** M
**System:** DisasterSystem, BuildingSystem

**Description:**
Implement debris creation when buildings are destroyed by disasters.

**Acceptance Criteria:**
- [ ] DebrisComponent: original_building (BuildingType), clear_progress (0-100%), clear_cost (credits), being_cleared (bool)
- [ ] Debris entity created at destroyed building position
- [ ] Debris blocks rebuilding at that tile
- [ ] clear_cost based on original building tier
- [ ] Debris inherits position from destroyed building

**Dependencies:**
- Depends on: 13-012, 13-038
- Blocks: 13-040

**Agent Notes:**
- Systems Architect: Debris is a separate entity from buildings
- Game Designer: Debris clearance creates recovery gameplay phase

---

### 13-040: Debris Clearance Mechanics

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement debris clearance mechanics including auto-clear and manual clear options.

**Acceptance Criteria:**
- [ ] Natural slow clearance: 1 debris tile cleared per DEBRIS_CLEAR_INTERVAL ticks
- [ ] Manual clearance: player pays credits to speed clearance
- [ ] Manual clearance instant (or very fast)
- [ ] Update DebrisComponent.clear_progress during natural clear
- [ ] Remove debris entity when clear_progress reaches 100%
- [ ] Update RecoveryZoneComponent.debris_remaining count

**Dependencies:**
- Depends on: 13-038, 13-039
- Blocks: 13-041

**Agent Notes:**
- Systems Architect: Auto-clear prevents permanent blocked tiles
- Game Designer: Manual option gives agency; auto-clear prevents frustration

---

### 13-041: Auto-Rebuild Integration

**Priority:** P1
**Size:** M
**System:** ZoneSystem, DisasterSystem

**Description:**
Integrate debris clearance with zone redevelopment for automatic rebuilding.

**Acceptance Criteria:**
- [ ] Zone designation persists under debris (no re-zoning needed)
- [ ] When debris cleared and zone is auto_rebuild_eligible:
- [ ] Check demand for zone type
- [ ] Check utilities available (power, water)
- [ ] Queue tile for normal development process
- [ ] Rebuilding same structure type gets 50% construction time discount

**Dependencies:**
- Depends on: 13-040, ZoneSystem (Epic 4)
- Blocks: None (enhancement)

**Agent Notes:**
- Systems Architect: Zones persist through disasters per game designer
- Game Designer: Recovery faster than initial build - not punishing, satisfying

---

### 13-042: Disaster Statistics Tracking

**Priority:** P1
**Size:** S
**System:** DisasterSystem

**Description:**
Track disaster statistics per player for progression and UI display.

**Acceptance Criteria:**
- [ ] DisasterStatistics struct: disasters_survived, buildings_lost, infrastructure_lost, total_damage_credits, worst_disaster (type)
- [ ] Statistics updated during disaster lifecycle
- [ ] Per-player statistics tracking
- [ ] get_statistics(PlayerID) on IDisasterProvider
- [ ] get_total_damage(PlayerID) returns cumulative credits lost

**Dependencies:**
- Depends on: 13-007
- Blocks: 13-054

**Agent Notes:**
- Systems Architect: Statistics needed for progression milestones
- Game Designer: "Disasters survived" feeds achievement system

---

### Group G: Triggering & Settings

---

### 13-043: Random Disaster Probability System

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the per-tick random disaster triggering system with configurable frequencies.

**Acceptance Criteria:**
- [ ] Per-tick probability check for each disaster type
- [ ] Default frequencies: ~1 minor per 100 cycles, ~1 major per 500 cycles
- [ ] Probability modified by game settings (frequency: off/rare/occasional/frequent)
- [ ] Spatial distribution: random valid location per disaster type
- [ ] Pre-requisite checks (inundation needs water, core_breach needs nuclear nexus)
- [ ] Server-authoritative RNG (not deterministic client replay)

**Dependencies:**
- Depends on: 13-001, 13-006
- Blocks: 13-022 through 13-031

**Agent Notes:**
- Systems Architect: Server-only RNG; clients receive results via sync
- Game Designer: Rare frequencies match "cozy sandbox" vibe

---

### 13-044: Manual Disaster Triggering

**Priority:** P1
**Size:** S
**System:** DisasterSystem

**Description:**
Implement manual disaster triggering for sandbox play and debug purposes.

**Acceptance Criteria:**
- [ ] trigger_disaster(DisasterType, GridPosition, severity) function
- [ ] Multiplayer: self-only (can only trigger in own territory)
- [ ] No statistics penalty for manually triggered disasters
- [ ] All disaster types available once unlocked via progression
- [ ] UI integration via command nexus "Catastrophe Simulation Training"
- [ ] Debug console command for development

**Dependencies:**
- Depends on: 13-006, 13-022 through 13-031
- Blocks: None (enhancement)

**Agent Notes:**
- Systems Architect: Self-only in multiplayer prevents griefing
- Game Designer: "Catastrophe Simulation Training" framing for immersion

---

### 13-045: Cascade Disaster Triggers

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Implement cascade mechanics where one disaster can trigger secondary disasters.

**Acceptance Criteria:**
- [ ] Seismic event -> secondary conflagrations (5% per tile chance)
- [ ] Core breach -> conflagration + contamination
- [ ] Mega storm -> secondary inundation near water
- [ ] Cascade depth limit: max 2 levels
- [ ] Cascade cooldown: no secondary disasters for 50 ticks after cascade
- [ ] Secondary disasters weaker than primary (severity * 0.7)

**Dependencies:**
- Depends on: 13-022, 13-023, 13-024, 13-026, 13-027
- Blocks: None (enhancement)

**Agent Notes:**
- Systems Architect: Depth limit prevents runaway scenarios
- Game Designer: Cascades create drama but must be contained

---

### 13-046: Disaster Warning System

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement the warning phase system giving players time to prepare before disasters become active.

**Acceptance Criteria:**
- [ ] Warning duration per disaster type (from config)
- [ ] DisasterWarningEvent emitted at warning start
- [ ] Visual/audio indicators during warning phase
- [ ] Player can take actions during warning (dispatch responders, evacuate)
- [ ] Warning time bonus setting (0%/50%/100% extra time)
- [ ] No warning for conflagration (instant ignition)

**Dependencies:**
- Depends on: 13-001, 13-003, 13-005, 13-006
- Blocks: 13-053

**Agent Notes:**
- Systems Architect: Warning phase is first state in state machine
- Game Designer: Warning creates anticipation emotion in experience arc

---

### 13-047: Disaster Settings System

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement comprehensive disaster settings for sandbox customization.

**Acceptance Criteria:**
- [ ] enabled: true/false (master toggle, disables ALL random disasters)
- [ ] random_events: true/false (random triggers only; manual still works)
- [ ] manual_trigger: true/false (sandbox trigger option)
- [ ] severity_cap: minor/major/catastrophic/extinction (max severity allowed)
- [ ] frequency: off/rare/occasional/frequent (probability multiplier)
- [ ] warning_time_bonus: 0%/50%/100% (extra warning time)
- [ ] Settings per-game, set at game creation
- [ ] Settings queryable by DisasterSystem tick logic

**Dependencies:**
- Depends on: 13-043, 13-044, 13-046
- Blocks: None (settings)

**Agent Notes:**
- Systems Architect: Settings stored in game state, queried during tick
- Game Designer: Full disable must exist - disaster risk NOT core to game identity

---

### Group H: Multiplayer

---

### 13-048: Cross-Boundary Disaster Handling

**Priority:** P0
**Size:** M
**System:** DisasterSystem

**Description:**
Implement disaster effects crossing player ownership boundaries.

**Acceptance Criteria:**
- [ ] Track affected_players array as disaster spreads
- [ ] Add players to affected list when their tiles become affected
- [ ] Most disaster types cross boundaries (conflagration, seismic, inundation, vortex, mega_storm, core_breach, titan, void_anomaly, cosmic_radiation)
- [ ] Exception: civil_unrest respects boundaries (spreads via pathways)
- [ ] Exception: player-triggered disasters stay in triggering player's territory

**Dependencies:**
- Depends on: 13-006, 13-022 through 13-031
- Blocks: 13-049, 13-051

**Agent Notes:**
- Systems Architect: Check ownership on spread; add player to affected list
- Game Designer: Cross-boundary creates cooperation opportunities; each player pays own damage

---

### 13-049: Disaster Notification Messages

**Priority:** P0
**Size:** M
**System:** DisasterSystem, NetworkSystem

**Description:**
Implement network messages for disaster synchronization.

**Acceptance Criteria:**
- [ ] DisasterSpawnMessage: disaster_id, DisasterComponent data (reliable)
- [ ] DisasterPhaseChangeMessage: disaster_id, new_phase (reliable)
- [ ] BuildingDestroyedMessage: building_id, disaster_id, cause (reliable)
- [ ] DisasterNotification struct: type, phase, epicenter, severity, primary_affected, all_affected, affects_viewer
- [ ] Server broadcasts to all clients; client-specific affects_viewer flag

**Dependencies:**
- Depends on: 13-005, NetworkSystem (Epic 1)
- Blocks: 13-050

**Agent Notes:**
- Systems Architect: Reliable messages for authoritative state changes
- Game Designer: Notifications enable "Overseer X is experiencing..." empathy moments

---

### 13-050: Fire State Synchronization

**Priority:** P1
**Size:** M
**System:** DisasterSystem, NetworkSystem

**Description:**
Implement unreliable delta synchronization for fire visualization state.

**Acceptance Criteria:**
- [ ] FireStateUpdateMessage: vector of (position, intensity) pairs for changed tiles
- [ ] Delta compression: only tiles changed since last sync
- [ ] Unreliable delivery (visualization only, not authoritative)
- [ ] Sync frequency: ~4 Hz (every ~5 ticks)
- [ ] Client reconstructs fire visualization from updates
- [ ] Periodic full-state reconciliation for drift correction

**Dependencies:**
- Depends on: 13-015, 13-016, NetworkSystem (Epic 1)
- Blocks: None (optimization)

**Agent Notes:**
- Systems Architect: Fire visual state is non-authoritative; damage is server-calculated
- Game Designer: Players need to see fire spread in real-time for response

---

### 13-051: Cooperative Response Coordination

**Priority:** P1
**Size:** M
**System:** DisasterSystem

**Description:**
Enable hazard responders from multiple players to fight cross-boundary disasters.

**Acceptance Criteria:**
- [ ] Each affected player's hazard responders can respond to disaster
- [ ] Responders can cross boundaries to fight fires cooperatively
- [ ] Dispatch considers all affected players' responders
- [ ] Response effectiveness aggregated from all contributing responders
- [ ] UI shows which players are contributing to response

**Dependencies:**
- Depends on: 13-033, 13-048
- Blocks: None (enhancement)

**Agent Notes:**
- Systems Architect: Query responders from all affected players, not just disaster owner
- Game Designer: Cooperation over compensation - social fun comes from helping each other

---

### 13-052: Disaster Network Synchronization

**Priority:** P0
**Size:** M
**System:** DisasterSystem, NetworkSystem

**Description:**
Ensure proper authoritative synchronization of disaster state.

**Acceptance Criteria:**
- [ ] Server is authoritative for all disaster logic (triggering, progression, damage)
- [ ] Clients receive disaster state via sync messages, do NOT simulate
- [ ] DisasterComponent syncs when disaster entity created/updated
- [ ] Building destruction syncs via normal building sync mechanism
- [ ] Recovery zone state syncs with RecoveryZoneComponent changes
- [ ] Reconnecting clients receive current disaster state

**Dependencies:**
- Depends on: 13-049, 13-050, SyncSystem (Epic 1)
- Blocks: None (foundation)

**Agent Notes:**
- Systems Architect: Server-authoritative per canon multiplayer pattern
- Game Designer: All players see same disaster state

---

### Group I: UI Integration

---

### 13-053: Disaster Warning Notifications

**Priority:** P0
**Size:** M
**System:** UISystem

**Description:**
Display disaster warning notifications with appropriate priority and timing.

**Acceptance Criteria:**
- [ ] DisasterWarningEvent triggers critical priority notification (alert_pulse)
- [ ] Notification shows disaster type, severity, countdown to active
- [ ] Map marker at disaster epicenter location
- [ ] Audio cue for warning (disaster-type specific)
- [ ] Notification persists for warning_duration, then transitions
- [ ] "Dismiss" option for warning acknowledged

**Dependencies:**
- Depends on: 13-005, 13-007, UISystem (Epic 12)
- Blocks: None (UI)

**Agent Notes:**
- Systems Architect: DisasterWarningEvent consumption by UISystem
- Game Designer: Critical priority ensures player notices; audio creates anticipation

---

### 13-054: Disaster Active/Ended Notifications

**Priority:** P0
**Size:** M
**System:** UISystem

**Description:**
Display notifications for disaster activation and resolution.

**Acceptance Criteria:**
- [ ] DisasterActiveEvent triggers critical notification "X has begun!"
- [ ] Active notification shows ongoing damage indicator
- [ ] DisasterEndedEvent triggers notification with summary (duration, damage, buildings lost)
- [ ] Summary notification can be expanded for details
- [ ] UI shows active disasters in status bar (colony_status)
- [ ] Click-to-center on disaster location

**Dependencies:**
- Depends on: 13-005, 13-007, 13-042, UISystem (Epic 12)
- Blocks: None (UI)

**Agent Notes:**
- Systems Architect: DisasterEndedEvent includes statistics for summary
- Game Designer: Summary creates closure; damage report enables assessment

---

### 13-055: Disaster Overlay Integration

**Priority:** P1
**Size:** M
**System:** UISystem, RenderingSystem

**Description:**
Integrate disaster visualization into the overlay system.

**Acceptance Criteria:**
- [ ] Fire overlay: shows fire intensity as color gradient (green -> yellow -> red)
- [ ] Damage zone overlay: shows affected radius during active disasters
- [ ] Recovery zone overlay: shows debris and clearing progress
- [ ] Overlay toggle in toolbar (scan_layer)
- [ ] Integration with IGridOverlay for consistent rendering
- [ ] Performance: efficient rendering even with many active fires

**Dependencies:**
- Depends on: 13-007, 13-021, RenderingSystem (Epic 2), UISystem (Epic 12)
- Blocks: None (visualization)

**Agent Notes:**
- Systems Architect: IGridOverlay interface from Epic 10 for consistent overlay rendering
- Game Designer: Visual feedback essential for player response and recovery tracking

---

### Group J: Testing

---

### 13-056: Unit Tests - Damage System

**Priority:** P0
**Size:** L
**System:** DisasterSystem

**Description:**
Comprehensive unit tests for the damage calculation system.

**Acceptance Criteria:**
- [ ] test_damage_with_no_resistance: full damage applied
- [ ] test_damage_with_resistance: damage reduced by resistance percentage
- [ ] test_damage_falloff: damage reduces with distance from epicenter
- [ ] test_damage_destroys_building: building destroyed at 0 health
- [ ] test_damage_creates_debris: debris entity created on destruction
- [ ] test_infrastructure_damage: infrastructure entities take damage
- [ ] test_service_building_resistance: service buildings take reduced damage
- [ ] test_damage_event_emission: DamageEvent emitted for each damaged entity
- [ ] All tests pass in isolation without full game setup

**Dependencies:**
- Depends on: 13-009 through 13-014
- Blocks: None (testing)

**Agent Notes:**
- Systems Architect: Unit tests should not require full system startup
- Game Designer: Damage calculation must be predictable and testable

---

### 13-057: Unit Tests - Fire Spread

**Priority:** P0
**Size:** L
**System:** DisasterSystem

**Description:**
Comprehensive unit tests for the ConflagrationGrid and fire spread algorithm.

**Acceptance Criteria:**
- [ ] test_ignite_sets_intensity: ignite() sets fire intensity
- [ ] test_fire_spreads_to_neighbor: fire spreads to adjacent flammable tiles
- [ ] test_fire_no_spread_to_water: water tiles (flammability 0) block spread
- [ ] test_fire_intensity_decay: fire intensity decays naturally
- [ ] test_fire_extinguish_removes: extinguish() removes fire
- [ ] test_area_extinguish: extinguish_in_radius affects multiple tiles
- [ ] test_double_buffer_no_order_dependency: spread result independent of iteration order
- [ ] test_flammability_affects_spread: high flammability increases spread chance
- [ ] test_fuel_exhaustion_accelerates_decay: fire burns out faster without fuel

**Dependencies:**
- Depends on: 13-015 through 13-020
- Blocks: None (testing)

**Agent Notes:**
- Systems Architect: Double-buffer test is critical for determinism
- Game Designer: Spread behavior must be predictable for player response

---

### 13-058: Unit Tests - Disaster Types

**Priority:** P1
**Size:** L
**System:** DisasterSystem

**Description:**
Unit tests for each disaster type implementation.

**Acceptance Criteria:**
- [ ] test_conflagration_creates_fire: conflagration adds fire to ConflagrationGrid
- [ ] test_seismic_instant_damage: seismic applies damage once, not continuously
- [ ] test_seismic_secondary_fire: seismic creates secondary fires
- [ ] test_inundation_flood_level: inundation floods tiles at correct elevation
- [ ] test_vortex_moves: vortex storm position updates each tick
- [ ] test_civil_unrest_pathway_spread: civil unrest spreads via pathways only
- [ ] test_civil_unrest_boundary: civil unrest respects player boundaries
- [ ] test_titan_target_seeking: titan moves toward high-value targets
- [ ] test_void_anomaly_effects: void anomaly applies selected effects

**Dependencies:**
- Depends on: 13-022 through 13-031
- Blocks: None (testing)

**Agent Notes:**
- Systems Architect: Each disaster type has unique mechanics requiring specific tests
- Game Designer: Disaster behavior must match design specifications

---

### 13-059: Integration Tests - Emergency Response

**Priority:** P0
**Size:** L
**System:** DisasterSystem, ServicesSystem

**Description:**
Integration tests for emergency response during disasters.

**Acceptance Criteria:**
- [ ] test_fire_with_hazard_coverage: fire suppressed faster in coverage zone
- [ ] test_fire_without_coverage: fire spreads at natural rate
- [ ] test_hazard_dispatch_and_suppress: full dispatch cycle with fire suppression
- [ ] test_multiple_fires_prioritization: resources go to highest intensity
- [ ] test_responder_capacity_limits: dispatch fails when capacity exceeded
- [ ] test_riot_with_enforcer_coverage: riot contained in coverage zone
- [ ] test_riot_suppression_gradual: riot intensity decreases over time
- [ ] test_fallback_suppression: fallback kicks in with zero coverage
- [ ] test_cooperative_response: multiple players' responders contribute

**Dependencies:**
- Depends on: 13-032 through 13-037, ServicesSystem (Epic 9)
- Blocks: None (testing)

**Agent Notes:**
- Systems Architect: Integration tests require ServicesSystem available
- Services Engineer: Emergency response integration is critical path

---

### 13-060: Multiplayer Disaster Tests

**Priority:** P1
**Size:** L
**System:** DisasterSystem, NetworkSystem

**Description:**
Test disaster behavior in multiplayer scenarios.

**Acceptance Criteria:**
- [ ] test_disaster_crosses_boundary: fire spreads across player boundaries
- [ ] test_affected_players_tracked: affected_players array populated correctly
- [ ] test_civil_unrest_respects_boundary: civil unrest stays in owner territory
- [ ] test_manual_disaster_self_only: manual triggers limited to own territory
- [ ] test_disaster_sync_reliable: disaster spawn/phase syncs to clients
- [ ] test_reconnect_disaster_state: reconnecting client receives active disasters
- [ ] test_global_disaster_all_affected: cosmic radiation affects all players
- [ ] Performance: test_100_fire_tiles_sync: sync overhead acceptable

**Dependencies:**
- Depends on: 13-048 through 13-052, NetworkSystem (Epic 1)
- Blocks: None (testing)

**Agent Notes:**
- Systems Architect: Multiplayer tests require network test harness
- Game Designer: Cross-boundary behavior must work correctly for cooperation

---

## Cross-Epic Dependencies

| Epic | Dependency | Direction |
|------|------------|-----------|
| Epic 3 | ITerrainQueryable for flammability, elevation | Consumes |
| Epic 4 | BuildingSystem DamageEvent handling, building destruction | Bidirectional |
| Epic 5 | EnergySystem for core breach triggers, energy reduction during storms | Consumes |
| Epic 6 | FluidSystem disabled in flooded areas | Consumes |
| Epic 7 | TransportSystem for civil_unrest pathway spread | Consumes |
| Epic 9 | ServicesSystem IEmergencyResponseProvider, coverage queries | Bidirectional |
| Epic 10 | DisorderSystem for civil_unrest triggers, ContaminationSystem for core_breach | Consumes |
| Epic 10 | LandValueSystem disaster effects on sector_value | Provides |
| Epic 12 | UISystem notification display | Provides |
| Epic 14 | ProgressionSystem disaster survival milestones | Provides |
| Epic 15 | AudioSystem disaster audio cues | Provides |
| Epic 16 | Persistence disaster state saving | Provides |

## Canon Updates Required

1. **systems.yaml**: Add DisasterSystem with tick_priority 75, component list, owns/does_not_own boundaries
2. **interfaces.yaml**: Add IDisasterProvider interface definition, IEmergencyResponseProvider interface
3. **patterns.yaml**: Add ConflagrationGrid to dense_grid_exception list (3 bytes/tile for fire spread)
4. **terminology.yaml**: Verify all disaster terms present (already defined: conflagration, seismic_event, inundation, vortex_storm, mega_storm, core_breach, civil_unrest, titan_emergence, void_anomaly, cosmic_radiation)
