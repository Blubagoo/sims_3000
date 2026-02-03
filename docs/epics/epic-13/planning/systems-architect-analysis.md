# Systems Architect Analysis: Epic 13 - Disasters

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.13.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 13 implements the **DisasterSystem** - the catastrophe simulation that brings dynamic threats to colonies. This epic introduces random and manual disaster triggering, damage mechanics, emergency response integration, and recovery tracking.

Key architectural characteristics:
1. **Server-authoritative disasters:** All disaster triggering, progression, and damage calculation runs on the server
2. **Cross-system integration:** Heavy interaction with BuildingSystem (damage targets), ServicesSystem (hazard_response), RenderingSystem (visualization)
3. **Dense grid patterns:** Fire spread requires a ConflagrationGrid for efficient spatial simulation
4. **Multiplayer considerations:** Disasters can span ownership boundaries; notification and response coordination needed
5. **Event-driven architecture:** Disasters emit events consumed by multiple systems (UI notifications, audio, rendering)

The DisasterSystem is primarily a **state machine manager** - it spawns disaster entities, progresses them through phases (warning -> active -> recovery), calculates damage, and coordinates cleanup. Actual building destruction is delegated to BuildingSystem; visualization is delegated to RenderingSystem.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Disaster Types and Mechanics](#3-disaster-types-and-mechanics)
4. [Damage System](#4-damage-system)
5. [Fire Spread Mechanics](#5-fire-spread-mechanics)
6. [Emergency Response Integration](#6-emergency-response-integration)
7. [Recovery and Cleanup](#7-recovery-and-cleanup)
8. [Multiplayer Considerations](#8-multiplayer-considerations)
9. [Dense Grid Patterns](#9-dense-grid-patterns)
10. [Tick Priority and Data Flow](#10-tick-priority-and-data-flow)
11. [Key Work Items](#11-key-work-items)
12. [Questions for Other Agents](#12-questions-for-other-agents)
13. [Risks and Concerns](#13-risks-and-concerns)
14. [Dependencies](#14-dependencies)
15. [Integration Points](#15-integration-points)
16. [Proposed Interfaces](#16-proposed-interfaces)
17. [Canon Update Proposals](#17-canon-update-proposals)

---

## 1. System Boundaries

### 1.1 DisasterSystem

**Type:** ECS System

**Owns:**
- Disaster triggering (random via probability system, manual via player/debug command)
- Disaster entity creation and lifecycle
- Disaster progression state machine (warning -> active -> recovery -> complete)
- Damage calculation per tick for active disasters
- Fire spread simulation (ConflagrationGrid)
- Emergency response dispatch coordination
- Recovery tracking (debris cleanup, rebuilding eligibility)
- Disaster statistics and history

**Does NOT Own:**
- Building destruction execution (BuildingSystem handles - receives damage events)
- Infrastructure destruction execution (TransportSystem, EnergySystem, FluidSystem handle)
- Fire spread visualization (RenderingSystem owns - queries ConflagrationGrid)
- Hazard response post placement (ServicesSystem owns)
- Emergency response pathfinding (TransportSystem provides connectivity)
- Disaster audio (AudioSystem handles disaster events)
- Disaster notifications (UISystem handles DisasterEvent)

**Provides:**
- `IDisasterProvider` interface for disaster state queries
- Active disaster information (type, location, severity, phase)
- Damage reports and statistics
- Fire spread state (ConflagrationGrid read access)
- Recovery zone queries

**Depends On:**
- BuildingSystem (damage targets, building queries)
- ServicesSystem (IEmergencyResponder - hazard response dispatch)
- TransportSystem (emergency vehicle pathing, connectivity)
- TerrainSystem (disaster placement validation, terrain modifiers)
- EnergySystem (core breach disasters need power plants)
- RenderingSystem (consumes fire spread data for visualization)

### 1.2 Boundary Summary Table

| Concern | DisasterSystem | BuildingSystem | ServicesSystem | RenderingSystem | UISystem |
|---------|---------------|----------------|----------------|-----------------|----------|
| Disaster triggering | **Owns** | - | - | - | Requests manual |
| Damage calculation | **Owns** | - | - | - | - |
| Building health reduction | Calculates | **Applies** | - | - | - |
| Fire spread simulation | **Owns** | - | - | - | - |
| Fire visualization | - | - | - | **Owns** | - |
| Emergency dispatch | Coordinates | - | **Provides** | - | - |
| Disaster notifications | - | - | - | - | **Owns** |
| Recovery tracking | **Owns** | Clears debris | - | - | Shows status |

---

## 2. Data Model

### 2.1 DisasterComponent

```cpp
struct DisasterComponent {
    DisasterType disaster_type = DisasterType::Conflagration;
    DisasterPhase phase = DisasterPhase::Warning;
    GridPosition epicenter;                    // Center of disaster
    uint32_t radius = 0;                       // Current effect radius
    uint32_t max_radius = 0;                   // Maximum spread radius
    uint32_t severity = 50;                    // 0-100 severity scale
    uint32_t tick_started = 0;                 // When disaster began
    uint32_t tick_active = 0;                  // When warning ended, active began
    uint32_t duration_ticks = 0;               // How long active phase lasts
    uint32_t ticks_remaining = 0;              // Ticks until next phase
    bool is_natural = true;                    // Natural vs player-triggered
    PlayerID affected_players[4] = {};         // Players with affected tiles
    uint8_t affected_player_count = 0;
};

enum class DisasterType : uint8_t {
    Conflagration = 0,     // Fire (canonical: conflagration)
    SeismicEvent = 1,      // Earthquake (canonical: seismic_event)
    Inundation = 2,        // Flood (canonical: inundation)
    VortexStorm = 3,       // Tornado (canonical: vortex_storm)
    MegaStorm = 4,         // Hurricane (canonical: mega_storm)
    CoreBreach = 5,        // Nuclear meltdown (canonical: core_breach)
    CivilUnrest = 6,       // Riot (canonical: civil_unrest)
    TitanEmergence = 7,    // Monster attack (canonical: titan_emergence)
    VoidAnomaly = 8,       // Cosmic event (canonical: void_anomaly)
    CosmicRadiation = 9    // Radiation event (canonical: cosmic_radiation)
};

enum class DisasterPhase : uint8_t {
    Warning = 0,           // Pre-disaster warning period
    Active = 1,            // Disaster in progress, dealing damage
    Recovery = 2,          // Disaster ended, cleanup in progress
    Complete = 3           // Fully resolved, entity can be removed
};
```

### 2.2 DamageableComponent

```cpp
struct DamageableComponent {
    uint32_t health = 100;              // Current health points
    uint32_t max_health = 100;          // Maximum health points
    DamageResistance resistances;       // Per-type resistance (0-100%)
    bool is_destroyed = false;          // Health <= 0
    bool is_repairable = true;          // Can be repaired vs must rebuild
    uint32_t last_damage_tick = 0;      // When last damaged (for decay)
};

struct DamageResistance {
    uint8_t fire = 0;       // 0-100% resistance to conflagration
    uint8_t seismic = 0;    // 0-100% resistance to seismic_event
    uint8_t flood = 0;      // 0-100% resistance to inundation
    uint8_t storm = 0;      // 0-100% resistance to vortex_storm/mega_storm
    uint8_t explosion = 0;  // 0-100% resistance to core_breach
};
```

### 2.3 OnFireComponent

```cpp
struct OnFireComponent {
    uint8_t intensity = 0;              // 0-255 fire intensity
    uint32_t tick_ignited = 0;          // When fire started
    uint32_t ticks_burning = 0;         // How long has burned
    bool is_spreading = true;           // Can spread to neighbors
    bool is_being_extinguished = false; // Hazard response active
    uint8_t extinguish_progress = 0;    // 0-100% extinguished
};
```

### 2.4 RecoveryZoneComponent

```cpp
struct RecoveryZoneComponent {
    DisasterType source_disaster;       // What caused the damage
    uint32_t debris_remaining = 0;      // Debris tiles to clear
    uint32_t buildings_destroyed = 0;   // Count for statistics
    uint32_t tick_disaster_ended = 0;   // When recovery started
    bool auto_rebuild_eligible = true;  // Zone can auto-rebuild
};
```

### 2.5 DisasterConfig

```cpp
struct DisasterConfig {
    DisasterType type;
    uint32_t warning_duration_ticks = 100;    // ~5 seconds warning
    uint32_t min_duration_ticks = 200;        // ~10 seconds minimum
    uint32_t max_duration_ticks = 1000;       // ~50 seconds maximum
    uint32_t base_damage_per_tick = 5;        // Base damage rate
    uint32_t base_radius = 10;                // Starting effect radius
    uint32_t max_radius = 50;                 // Maximum spread
    float spread_rate = 0.1f;                 // Radius increase per tick
    float random_probability = 0.0001f;       // Per-tick spawn chance
    bool can_trigger_manually = true;         // Player can trigger
    bool respects_ownership = false;          // Stops at player boundaries?
};
```

---

## 3. Disaster Types and Mechanics

### 3.1 Conflagration (Fire)

**Canonical Term:** conflagration

**Mechanics:**
- Starts at ignition point, spreads to adjacent tiles
- Spread rate affected by: vegetation, building density, wind direction
- Extinguished by: hazard response posts, reaching water, fuel exhaustion
- Buildings in fire zone take continuous damage until extinguished

**Trigger Conditions:**
- Random: Low probability per tick, higher near fabrication zones
- Manual: Player debug command or "arsonist" event
- Cascade: Core breach causes secondary conflagrations

**Spread Algorithm:**
```
Each tick for each tile on fire:
    if intensity > spread_threshold:
        for each adjacent tile:
            if tile.is_flammable and not tile.on_fire:
                spread_chance = intensity * flammability * (1 - hazard_coverage)
                if random() < spread_chance:
                    ignite(adjacent_tile, intensity * 0.8)

    intensity -= decay_rate
    if hazard_response_active:
        intensity -= extinguish_rate * response_effectiveness

    if intensity <= 0:
        extinguish(tile)
```

**Damage Type:** fire

### 3.2 Seismic Event (Earthquake)

**Canonical Term:** seismic_event

**Mechanics:**
- Instant damage in radius from epicenter
- Damage intensity falls off with distance
- May cause secondary conflagrations (broken energy conduits)
- Destroys infrastructure (pathways, conduits, pipes) preferentially

**Trigger Conditions:**
- Random: Very low probability, not affected by player actions
- Manual: Player debug command

**Damage Pattern:**
```
For each entity in radius:
    distance = manhattan_distance(entity, epicenter)
    falloff = 1.0 - (distance / max_radius)
    damage = base_damage * severity * falloff * (1 - seismic_resistance)
    apply_damage(entity, damage, DamageType::Seismic)

    if random() < secondary_fire_chance * falloff:
        ignite(entity.position)
```

**Damage Type:** seismic

### 3.3 Inundation (Flood)

**Canonical Term:** inundation

**Mechanics:**
- Water rises from water bodies, floods low-elevation tiles
- Duration-based: water level rises, peaks, recedes
- Buildings take damage while submerged
- Halts energy and fluid systems in affected area

**Trigger Conditions:**
- Random: Higher probability during certain game phases (seasons)
- Manual: Player debug command
- Prerequisite: Must have water bodies adjacent to developed area

**Flood Level Calculation:**
```
flood_level = base_level + severity * phase_multiplier

For each tile at elevation <= flood_level:
    if adjacent_to_water_body:
        mark_flooded(tile)
        for each entity at tile:
            apply_damage(entity, flood_damage_per_tick, DamageType::Flood)
            disable_utilities(entity)
```

**Damage Type:** flood

### 3.4 Vortex Storm (Tornado)

**Canonical Term:** vortex_storm

**Mechanics:**
- Mobile disaster: moves across map along path
- Narrow but intense damage corridor
- Random path with general direction
- Short duration, high damage

**Trigger Conditions:**
- Random: Low probability, seasonal modifier
- Manual: Player debug command

**Movement Pattern:**
```
Each tick:
    position += direction * speed
    direction += random_deviation(-10, 10) degrees

    For each entity in path_width of position:
        apply_damage(entity, vortex_damage, DamageType::Storm)
        if random() < debris_throw_chance:
            throw_debris(entity.position, random_direction())

    if position.out_of_bounds or duration_expired:
        end_disaster()
```

**Damage Type:** storm

### 3.5 Mega Storm (Hurricane)

**Canonical Term:** mega_storm

**Mechanics:**
- Large radius, moderate damage over long duration
- Affects entire map or large region
- Reduces energy output (solar, wind affected)
- Can cause secondary inundation near water

**Trigger Conditions:**
- Random: Very low probability, long warning period
- Manual: Player debug command

**Damage Type:** storm

### 3.6 Core Breach (Nuclear Meltdown)

**Canonical Term:** core_breach

**Mechanics:**
- Triggered by nuclear nexus failure
- Explosion damage in immediate radius
- Creates long-term contamination zone
- Contamination persists long after disaster ends

**Trigger Conditions:**
- Random: Probability based on nuclear nexus age and maintenance
- Cascade: Triggered when nuclear nexus takes critical damage
- Manual: Player debug command

**Phases:**
1. Warning: Reactor warnings, evacuation possible
2. Active: Explosion, immediate damage
3. Recovery: Contamination zone, long-term effects

**Damage Type:** explosion (immediate), contamination (ongoing)

### 3.7 Civil Unrest (Riot)

**Canonical Term:** civil_unrest

**Mechanics:**
- Triggered by high disorder index
- Spreads along pathways (not terrain)
- Damages buildings, especially exchange zones
- Suppressed by enforcer coverage

**Trigger Conditions:**
- Random: Probability scales with disorder index
- Threshold: Auto-triggers if disorder > 80%
- Manual: Player debug command

**Spread Pattern:**
```
Each tick:
    For each tile with active unrest:
        for each pathway-connected neighbor:
            if not at_unrest and disorder_at(neighbor) > threshold:
                spread_chance = disorder_at(neighbor) / 100 * (1 - enforcer_coverage)
                if random() < spread_chance:
                    ignite_unrest(neighbor)

        if enforcer_coverage > suppression_threshold:
            suppress_progress += enforcer_effectiveness
            if suppress_progress >= 100:
                end_unrest(tile)
```

**Damage Type:** fire (arson), storm (property damage)

### 3.8 Titan Emergence (Monster Attack)

**Canonical Term:** titan_emergence

**Mechanics:**
- Large mobile entity with pathfinding
- Targets high-value buildings
- Extremely high damage, limited duration
- Can be "redirected" by military installations (future)

**Trigger Conditions:**
- Random: Very rare, late-game only
- Manual: Player debug command
- Milestone: Unlocked after certain population threshold

**Movement Pattern:**
```
Each tick:
    target = find_highest_value_building_in_range(vision_range)
    if target:
        move_toward(target)
        if adjacent_to(target):
            attack(target, titan_damage)
    else:
        wander()

    if rampage_duration_expired:
        retreat_to_edge()
        end_disaster()
```

**Damage Type:** seismic (stomp), fire (breath if applicable)

### 3.9 Void Anomaly (Cosmic Event)

**Canonical Term:** void_anomaly

**Mechanics:**
- Random teleportation of buildings
- Reality distortion effects (visual primarily)
- Moderate damage in affected area
- Short duration, unpredictable effects

**Trigger Conditions:**
- Random: Very rare
- Manual: Player debug command
- Tied to alien theme: related to cosmic radiation levels

**Damage Type:** explosion (reality tear)

### 3.10 Cosmic Radiation (Radiation Event)

**Canonical Term:** cosmic_radiation

**Mechanics:**
- Global event affecting entire map
- Increases contamination everywhere
- Reduces population growth
- Affects being health (reduces life expectancy during event)

**Trigger Conditions:**
- Random: Rare, linked to void_anomaly
- Manual: Player debug command

**Damage Type:** contamination (long-term)

---

## 4. Damage System

### 4.1 IDamageable Interface Implementation

From canon (interfaces.yaml):

```cpp
class IDamageable {
public:
    virtual uint32_t get_health() const = 0;
    virtual uint32_t get_max_health() const = 0;
    virtual void apply_damage(uint32_t amount, DamageType type) = 0;
    virtual bool is_destroyed() const = 0;
};

enum class DamageType : uint8_t {
    Fire = 0,       // From conflagrations
    Seismic = 1,    // From seismic_events
    Flood = 2,      // From inundation
    Storm = 3,      // From vortex_storm, mega_storm
    Explosion = 4   // From core_breach, etc.
};
```

### 4.2 Damage Calculation Flow

```
DisasterSystem.tick():
    For each active disaster:
        affected_tiles = get_affected_tiles(disaster)

        For each tile in affected_tiles:
            entities = get_damageable_entities_at(tile)

            For each entity:
                base_damage = disaster.damage_per_tick
                distance_modifier = calculate_distance_falloff(entity, disaster.epicenter)
                resistance = entity.DamageableComponent.resistances[disaster.damage_type]

                final_damage = base_damage * distance_modifier * (1 - resistance/100)

                emit DamageEvent {
                    target: entity,
                    amount: final_damage,
                    type: disaster.damage_type,
                    source: disaster.entity_id
                }
```

### 4.3 DamageEvent Processing

BuildingSystem (and other systems with damageable entities) listens for DamageEvent:

```cpp
struct DamageEvent {
    EntityID target;
    uint32_t amount;
    DamageType type;
    EntityID source;           // Disaster entity that caused damage
    GridPosition source_pos;   // For directional damage effects
};

// BuildingSystem handles:
void BuildingSystem::on_damage_event(const DamageEvent& event) {
    auto& damageable = registry.get<DamageableComponent>(event.target);

    damageable.health = std::max(0u, damageable.health - event.amount);
    damageable.last_damage_tick = current_tick;

    if (damageable.health == 0 && !damageable.is_destroyed) {
        damageable.is_destroyed = true;
        emit BuildingDestroyedEvent {
            building: event.target,
            cause: event.source,
            damage_type: event.type
        }
    }
}
```

### 4.4 Building Health by Type

| Building Category | Base Health | Fire Resist | Seismic Resist | Notes |
|-------------------|-------------|-------------|----------------|-------|
| Low-density habitation | 100 | 10% | 20% | Wooden structures |
| High-density habitation | 200 | 30% | 40% | Concrete structures |
| Exchange buildings | 150 | 20% | 30% | Mixed materials |
| Fabrication buildings | 250 | 50% | 50% | Industrial materials |
| Energy nexus (carbon) | 300 | 10% | 40% | Vulnerable to fire |
| Energy nexus (nuclear) | 500 | 80% | 60% | Hardened, core breach risk |
| Service buildings | 200 | 40% | 50% | Emergency-hardened |
| Infrastructure (roads) | 50 | 90% | 20% | Asphalt, seismic vulnerable |
| Infrastructure (conduits) | 100 | 70% | 30% | Underground protected |

---

## 5. Fire Spread Mechanics

### 5.1 ConflagrationGrid Design

Fire spread requires efficient spatial tracking. Following the dense_grid_exception pattern:

```cpp
class ConflagrationGrid {
private:
    std::vector<uint8_t> fire_intensity_;  // 0-255 intensity per tile
    std::vector<uint8_t> flammability_;    // 0-255 flammability per tile
    std::vector<uint8_t> fuel_remaining_;  // 0-255 fuel per tile
    uint32_t width_;
    uint32_t height_;

public:
    // Read operations
    uint8_t get_intensity_at(int32_t x, int32_t y) const;
    bool is_on_fire(int32_t x, int32_t y) const { return get_intensity_at(x, y) > 0; }
    uint8_t get_flammability_at(int32_t x, int32_t y) const;

    // Write operations (DisasterSystem only)
    void ignite(int32_t x, int32_t y, uint8_t intensity);
    void extinguish(int32_t x, int32_t y);
    void update_intensity(int32_t x, int32_t y, int8_t delta);

    // Bulk operations
    void spread_tick();  // Process all fire spread for one tick
    void extinguish_in_radius(int32_t x, int32_t y, uint32_t radius, uint8_t strength);

    // Query operations for RenderingSystem
    std::vector<GridPosition> get_all_fire_positions() const;
    std::vector<std::pair<GridPosition, uint8_t>> get_fires_with_intensity() const;
};
```

Memory: 3 bytes per tile (intensity + flammability + fuel)
- 256x256 map: 192 KB
- 512x512 map: 768 KB

### 5.2 Flammability Sources

```cpp
void ConflagrationGrid::calculate_flammability(ITerrainQueryable& terrain, IGridQueryable& spatial) {
    for (int32_t y = 0; y < height_; y++) {
        for (int32_t x = 0; x < width_; x++) {
            uint8_t base = 128;  // Default flammability

            TerrainType type = terrain.get_terrain_type(x, y);
            switch (type) {
                case TerrainType::BiolumGrove:  // Forest
                    base = 220;  // Very flammable
                    break;
                case TerrainType::SporeFlats:   // Spore plains
                    base = 200;  // Flammable
                    break;
                case TerrainType::Water:
                    base = 0;    // Fire barrier
                    break;
                case TerrainType::EmberCrust:   // Volcanic
                    base = 60;   // Fire resistant
                    break;
            }

            // Modify by buildings
            auto entity = spatial.get_entity_at({x, y});
            if (entity) {
                auto* building = registry.try_get<BuildingComponent>(*entity);
                if (building) {
                    // Fabrication increases flammability (industrial materials)
                    // Concrete reduces flammability
                    base = adjust_for_building(base, building->type);
                }
            }

            set_flammability(x, y, base);
        }
    }
}
```

### 5.3 Fire Spread Algorithm

```cpp
void ConflagrationGrid::spread_tick() {
    // Use double-buffer to avoid order-dependent spreading
    auto new_intensity = fire_intensity_;  // Copy current state

    for (int32_t y = 0; y < height_; y++) {
        for (int32_t x = 0; x < width_; x++) {
            uint8_t intensity = fire_intensity_[y * width_ + x];

            if (intensity == 0) continue;

            // Natural decay
            int decay = 2;  // Base decay per tick

            // Spread to neighbors
            if (intensity > SPREAD_THRESHOLD) {
                for (auto [nx, ny] : get_neighbors(x, y)) {
                    if (!is_valid(nx, ny)) continue;
                    if (fire_intensity_[ny * width_ + nx] > 0) continue;  // Already burning

                    uint8_t neighbor_flammability = flammability_[ny * width_ + nx];
                    if (neighbor_flammability == 0) continue;  // Fire barrier

                    float spread_chance = (intensity / 255.0f) * (neighbor_flammability / 255.0f);

                    if (random_float() < spread_chance) {
                        uint8_t new_fire = static_cast<uint8_t>(intensity * 0.7f);
                        new_intensity[ny * width_ + nx] = std::max(
                            new_intensity[ny * width_ + nx],
                            new_fire
                        );
                    }
                }
            }

            // Consume fuel
            uint8_t fuel = fuel_remaining_[y * width_ + x];
            if (fuel > 0) {
                fuel_remaining_[y * width_ + x] = std::max(0, fuel - FUEL_CONSUMPTION);
            } else {
                decay += 5;  // Faster decay without fuel
            }

            // Apply decay
            new_intensity[y * width_ + x] = std::max(0, static_cast<int>(intensity) - decay);
        }
    }

    fire_intensity_ = std::move(new_intensity);
}
```

---

## 6. Emergency Response Integration

### 6.1 IEmergencyResponder Interface (From Canon)

```cpp
class IEmergencyResponder {
public:
    virtual bool can_respond_to(DisasterType disaster_type) const = 0;
    virtual bool dispatch_to(GridPosition position) = 0;
    virtual float get_response_effectiveness() const = 0;
};
```

**Implemented by:**
- Hazard response post (fires/conflagrations)
- Enforcer post (riots/civil_unrest)

### 6.2 Response Dispatch Flow

```
DisasterSystem::dispatch_emergency_response(disaster):
    if disaster.type == Conflagration:
        responder_type = ServiceType::HazardResponse
    elif disaster.type == CivilUnrest:
        responder_type = ServiceType::Enforcer
    else:
        return  // No responder for this disaster type

    affected_tiles = get_affected_tiles(disaster)
    owner = get_primary_affected_owner(affected_tiles)

    responders = services_system.get_responders(responder_type, owner)

    for each responder in responders:
        if responder.can_respond_to(disaster.type):
            # Find highest priority tile in range
            target = find_highest_priority_tile(affected_tiles, responder.position)

            if target and responder.dispatch_to(target):
                create_response_entity(responder, target, disaster)
```

### 6.3 Response Effectiveness

Hazard response effectiveness affects fire extinguishment:

```cpp
void DisasterSystem::apply_hazard_response(GridPosition pos, float effectiveness) {
    // effectiveness = service_coverage_at(pos) * funding_modifier

    uint8_t extinguish_power = static_cast<uint8_t>(effectiveness * 50);
    conflagration_grid_.extinguish_in_radius(pos.x, pos.y, RESPONSE_RADIUS, extinguish_power);
}
```

### 6.4 Response Coverage Integration with Epic 9

```
ServicesSystem (Epic 9) provides:
    get_coverage_at(ServiceType::HazardResponse, position) -> float

DisasterSystem queries:
    For each active conflagration tile:
        coverage = services.get_coverage_at(HazardResponse, tile)
        extinguish_rate = base_extinguish * coverage
        conflagration_grid.update_intensity(tile, -extinguish_rate)
```

---

## 7. Recovery and Cleanup

### 7.1 Recovery Phase Mechanics

When a disaster transitions from Active to Recovery:

```cpp
void DisasterSystem::begin_recovery(EntityID disaster_id) {
    auto& disaster = registry.get<DisasterComponent>(disaster_id);
    disaster.phase = DisasterPhase::Recovery;

    // Survey damage
    GridRect affected_area = get_disaster_bounds(disaster);
    uint32_t buildings_destroyed = 0;
    uint32_t debris_tiles = 0;

    for each tile in affected_area:
        if has_debris(tile):
            debris_tiles++;
        if was_building_destroyed(tile):
            buildings_destroyed++;

    // Create recovery zone entity
    auto recovery = registry.create();
    registry.emplace<RecoveryZoneComponent>(recovery, {
        .source_disaster = disaster.disaster_type,
        .debris_remaining = debris_tiles,
        .buildings_destroyed = buildings_destroyed,
        .tick_disaster_ended = current_tick,
        .auto_rebuild_eligible = true
    });
    registry.emplace<PositionComponent>(recovery, disaster.epicenter);
    registry.emplace<GridRectComponent>(recovery, affected_area);

    emit DisasterRecoveryStartedEvent { disaster_id, recovery };
}
```

### 7.2 Debris Mechanics

Destroyed buildings leave debris that must be cleared:

```cpp
struct DebrisComponent {
    BuildingType original_building;
    uint8_t clear_progress = 0;     // 0-100%
    uint32_t clear_cost = 0;        // Credits to clear
    bool being_cleared = false;
};

// Debris blocks rebuilding
bool BuildingSystem::can_build_at(GridPosition pos) {
    auto entity = spatial.get_entity_at(pos);
    if (entity && registry.has<DebrisComponent>(*entity)) {
        return false;  // Must clear debris first
    }
    return true;
}
```

### 7.3 Auto-Rebuild Eligibility

Zones retain their designation after disaster. If auto_rebuild_eligible:

```cpp
void ZoneSystem::tick() {
    for each zoned_tile with debris_cleared:
        if zone.auto_rebuild_eligible:
            if has_demand(zone.type) and has_utilities(tile):
                queue_for_development(tile)
```

---

## 8. Multiplayer Considerations

### 8.1 Server-Authoritative Disasters

| Aspect | Authority | Notes |
|--------|-----------|-------|
| Disaster triggering | Server | RNG runs on server |
| Disaster type/severity | Server | Determined at spawn |
| Damage calculation | Server | Applied server-side |
| Fire spread | Server | ConflagrationGrid server-side |
| Building destruction | Server | BuildingSystem server-side |
| Response dispatch | Server | ServicesSystem server-side |
| Recovery tracking | Server | RecoveryZone server-side |

### 8.2 Cross-Ownership Disasters

Key question: Do disasters respect player ownership boundaries?

**Proposed Behavior:**

| Disaster Type | Crosses Boundaries? | Rationale |
|---------------|---------------------|-----------|
| Conflagration | YES | Fire spreads physically |
| Seismic Event | YES | Earthquakes don't respect ownership |
| Inundation | YES | Water doesn't respect ownership |
| Vortex Storm | YES | Tornados move freely |
| Mega Storm | YES | Affects entire region |
| Core Breach | YES | Radiation spreads physically |
| Civil Unrest | NO | Respects pathway networks per owner |
| Titan Emergence | YES | Monster moves freely |
| Void Anomaly | YES | Cosmic effects are indiscriminate |
| Cosmic Radiation | YES (Global) | Affects everyone |

### 8.3 Player Notification System

```cpp
struct DisasterNotification {
    DisasterType type;
    DisasterPhase phase;
    GridPosition epicenter;
    uint32_t severity;
    PlayerID primary_affected;      // Player most affected
    std::vector<PlayerID> all_affected;
    bool affects_viewer;            // For recipient
};

// Server sends to each client
void NetworkManager::broadcast_disaster_notification(const DisasterNotification& notif) {
    for each connected_player:
        notif.affects_viewer = is_player_affected(connected_player, notif);
        send_to(connected_player, DisasterNotificationMessage(notif));
}
```

### 8.4 Cooperative Response

When a disaster crosses boundaries:

```cpp
// Each affected player's hazard response can help
void DisasterSystem::dispatch_cross_boundary_response(DisasterComponent& disaster) {
    for each affected_player in disaster.affected_players:
        responders = services_system.get_responders(HazardResponse, affected_player);

        for each responder:
            // Responder can fight fire even on other player's tiles
            // (Cooperative disaster response)
            target = find_closest_fire_tile(responder.position, disaster);
            if target:
                dispatch(responder, target)
}
```

### 8.5 Network Synchronization

Disasters require:
- **Reliable:** Disaster spawn, phase changes, building destruction
- **Unreliable (frequent):** Fire intensity updates (visual only)

```cpp
// Reliable messages
struct DisasterSpawnMessage {
    EntityID disaster_id;
    DisasterComponent data;
};

struct DisasterPhaseChangeMessage {
    EntityID disaster_id;
    DisasterPhase new_phase;
};

struct BuildingDestroyedMessage {
    EntityID building_id;
    EntityID disaster_id;
    DamageType cause;
};

// Unreliable delta updates (fire visualization)
struct FireStateUpdateMessage {
    std::vector<std::pair<GridPosition, uint8_t>> changed_tiles;  // Pos + intensity
};
```

---

## 9. Dense Grid Patterns

### 9.1 ConflagrationGrid (New Dense Grid)

**Justification for dense_grid_exception:**
- Fire can spread to any tile (not sparse entities)
- Spatial locality critical for spread algorithm (neighbor queries)
- Per-entity overhead prohibitive: 3 bytes dense vs 24+ bytes per OnFireComponent entity

**Memory Usage:**

| Map Size | Grid Memory | Notes |
|----------|-------------|-------|
| 128x128 | 48 KB | Small map |
| 256x256 | 192 KB | Medium map |
| 512x512 | 768 KB | Large map |

### 9.2 Integration with Existing Grids

```
TerrainGrid (Epic 3)
    |
    | get_terrain_type() for flammability
    v
ConflagrationGrid (Epic 13)
    |
    | fire intensity for damage
    v
+-- DisasterSystem (damage calculation)
|
+-- RenderingSystem (fire visualization)
|
+-- ServiceCoverageGrid (Epic 9)
        |
        | hazard_response coverage for extinguishment
```

### 9.3 Dirty Region Tracking

Optimize fire spread by tracking modified regions:

```cpp
class ConflagrationGrid {
private:
    std::set<ChunkCoord> dirty_chunks_;  // 32x32 chunks with changes

public:
    void mark_dirty(int32_t x, int32_t y) {
        dirty_chunks_.insert({x / 32, y / 32});
    }

    std::vector<ChunkCoord> get_dirty_chunks() const {
        return {dirty_chunks_.begin(), dirty_chunks_.end()};
    }

    void clear_dirty() {
        dirty_chunks_.clear();
    }
};
```

---

## 10. Tick Priority and Data Flow

### 10.1 Proposed Tick Priority

DisasterSystem should run after services (need coverage) and before building state updates:

```
Priority 55: ServicesSystem        -- hazard_response coverage calculated
Priority 60: EconomySystem         -- maintenance costs
Priority 70: DisorderSystem        -- disorder for civil_unrest triggers
Priority 75: DisasterSystem        -- NEW: disaster progression, damage
Priority 80: ContaminationSystem   -- disaster contamination included
Priority 85: LandValueSystem       -- disaster effects on sector value
```

**Rationale for priority 75:**
- After DisorderSystem (70): Civil unrest triggers based on disorder levels
- After ServicesSystem (55): Hazard response coverage available
- Before ContaminationSystem (80): Core breach contamination included this tick
- Before LandValueSystem (85): Disaster damage affects land value immediately

### 10.2 Data Flow Diagram

```
                    TerrainSystem (5)
                          |
                          | ITerrainQueryable (flammability)
                          v
ServicesSystem (55) --> DisasterSystem (75) <-- DisorderSystem (70)
       |                    |                        |
       | hazard_response    | damage events          | civil_unrest trigger
       v                    v                        |
                    BuildingSystem (40)              |
                    (processes damage)               |
                          |                          |
                          v                          |
              RenderingSystem (render loop)          |
              (visualizes fire/damage)               |
                                                     |
              ContaminationSystem (80) <-------------+
              (receives core_breach contamination)
```

### 10.3 DisasterSystem Tick Logic

```
DisasterSystem::tick():

1. PROCESS RANDOM TRIGGERS
   For each disaster_type with random_probability > 0:
       if random() < probability * time_modifier:
           spawn_disaster(type, random_location())

2. UPDATE ACTIVE DISASTERS
   For each disaster with phase == Active:

       # Progress disaster
       disaster.ticks_remaining--

       # Type-specific logic
       switch disaster.type:
           case Conflagration:
               conflagration_grid.spread_tick()
               apply_fire_damage()
           case SeismicEvent:
               if tick == first_active_tick:
                   apply_instant_damage()
           case VortexStorm:
               move_vortex()
               apply_path_damage()
           # ... etc

       # Check phase transition
       if disaster.ticks_remaining == 0:
           transition_to_recovery(disaster)

3. PROCESS EMERGENCY RESPONSE
   For each disaster requiring response:
       dispatch_emergency_response(disaster)

   Apply hazard response extinguishment:
       For each tile with fire and hazard_coverage > 0:
           extinguish_rate = coverage * effectiveness
           conflagration_grid.reduce_intensity(tile, extinguish_rate)

4. PROCESS RECOVERY ZONES
   For each recovery_zone:
       # Natural debris clearing (slow)
       if tick % DEBRIS_CLEAR_INTERVAL == 0:
           clear_random_debris_tile(zone)

       # Check completion
       if zone.debris_remaining == 0:
           complete_recovery(zone)

5. EMIT EVENTS
   Emit DisasterTickEvent for UI updates
   Emit DamageEvent for each damaged entity
   Emit DisasterPhaseChangeEvent for phase transitions
```

---

## 11. Key Work Items

### Phase 1: Core Infrastructure (7 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-001 | DisasterType enum and DisasterConfig | S | Define all disaster types with configuration |
| E13-002 | DisasterComponent definition | S | Core disaster entity component |
| E13-003 | DamageableComponent definition | S | Health and resistance component for damageable entities |
| E13-004 | OnFireComponent definition | S | Per-tile fire state tracking |
| E13-005 | DamageType enum and DamageEvent | S | Damage system foundation |
| E13-006 | DisasterSystem class skeleton | M | ISimulatable at tick_priority 75 |
| E13-007 | IDisasterProvider interface | M | Query interface for disaster state |

### Phase 2: Damage System (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-008 | IDamageable interface implementation | M | Implement canon interface for buildings |
| E13-009 | DamageableComponent integration | M | Add to buildings, infrastructure entities |
| E13-010 | Damage calculation pipeline | M | Distance falloff, resistance calculation |
| E13-011 | BuildingSystem damage handling | M | Process DamageEvent, handle destruction |
| E13-012 | Infrastructure damage handling | M | Roads, conduits, pipes take damage |

### Phase 3: Fire System (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-013 | ConflagrationGrid implementation | L | Dense grid for fire spread |
| E13-014 | Flammability calculation | M | Terrain and building flammability |
| E13-015 | Fire spread algorithm | L | Neighbor spread with decay |
| E13-016 | Fire ignition and extinguishment | M | Start/stop fire at positions |
| E13-017 | Fire damage application | M | Continuous damage to burning tiles |
| E13-018 | Fire visualization data provider | M | Expose fire state to RenderingSystem |

### Phase 4: Disaster Types (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-019 | Conflagration disaster implementation | M | Fire disaster with spread |
| E13-020 | Seismic event implementation | M | Earthquake with instant damage |
| E13-021 | Inundation implementation | M | Flood based on elevation |
| E13-022 | Vortex storm implementation | M | Mobile tornado |
| E13-023 | Mega storm implementation | M | Large-area storm |
| E13-024 | Core breach implementation | L | Nuclear meltdown with contamination |
| E13-025 | Civil unrest implementation | M | Riot with enforcer suppression |
| E13-026 | Titan emergence implementation | L | Monster attack (late-game) |

### Phase 5: Emergency Response (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-027 | IEmergencyResponder integration | M | Connect to ServicesSystem |
| E13-028 | Hazard response dispatch | M | Fire response coordination |
| E13-029 | Enforcer riot suppression | M | Riot response coordination |
| E13-030 | Response effectiveness calculation | M | Funding affects response |

### Phase 6: Recovery System (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-031 | RecoveryZoneComponent | S | Recovery zone tracking |
| E13-032 | Debris creation and clearing | M | Destroyed buildings leave debris |
| E13-033 | Auto-rebuild integration | M | Zones rebuild after clearing |
| E13-034 | Recovery statistics | S | Track damage and recovery stats |

### Phase 7: Triggering System (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-035 | Random disaster probability | M | Per-tick random triggers |
| E13-036 | Manual disaster triggering | S | Debug/cheat command support |
| E13-037 | Cascade disaster triggers | M | Secondary disasters from primary |
| E13-038 | Disaster warning phase | M | Pre-disaster warning period |

### Phase 8: Multiplayer (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-039 | Cross-boundary disaster handling | M | Disasters spanning ownership |
| E13-040 | Disaster notification messages | M | Network messages for disasters |
| E13-041 | Cooperative response coordination | M | Multi-player response |
| E13-042 | Disaster state synchronization | M | Fire state delta sync |

### Phase 9: Events & Integration (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-043 | DisasterEvent definitions | S | Events for UI/audio integration |
| E13-044 | ContaminationSystem integration | M | Core breach adds contamination |
| E13-045 | UISystem disaster notifications | M | Alert pulse for disasters |
| E13-046 | AudioSystem disaster sounds | S | Disaster audio triggers |

### Phase 10: Testing (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E13-047 | Unit tests for damage system | L | Test damage calculation |
| E13-048 | Unit tests for fire spread | L | Test ConflagrationGrid |
| E13-049 | Integration test with Epic 9 | L | Test emergency response |
| E13-050 | Multiplayer disaster test | L | Test cross-boundary scenarios |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 9 | E13-001 through E13-005, E13-031, E13-034, E13-036, E13-043, E13-046 |
| M | 29 | E13-006 through E13-012, E13-014, E13-016 through E13-023, E13-025, E13-027 through E13-030, E13-032, E13-033, E13-035, E13-037 through E13-042, E13-044, E13-045 |
| L | 8 | E13-013, E13-015, E13-024, E13-026, E13-047 through E13-050 |
| **Total** | **50** | |

---

## 12. Questions for Other Agents

### @game-designer

1. **Disaster frequency balance:** How often should disasters occur? Proposed: ~1 minor disaster per 50 cycles (game years), ~1 major disaster per 200 cycles. Is this appropriate for a "cozy sandbox" game?

2. **Disaster severity scaling:** Should disasters scale with population/city size, or be fixed severity? Large cities could handle disasters better (more services) but also have more to lose.

3. **Player-triggered disasters:** Should players be able to trigger disasters intentionally (not just debug)? Some city builders allow "disaster mode" for fun. Fits our sandbox vibe?

4. **Cross-boundary damage responsibility:** If Player A's fire spreads to Player B's colony, who pays for rebuilding? Should there be compensation mechanics?

5. **Disaster immunity options:** Should players be able to disable disasters entirely in sandbox mode? Or is disaster risk core to the experience?

6. **Titan emergence behavior:** How should the monster behave? Classic SimCity monsters had semi-random paths. Should titans target specific building types (arcologies, command nexus)?

7. **Void anomaly effects:** The cosmic event is very thematic but mechanically vague. What specific effects should void anomalies have? Reality distortion is cool but needs gameplay meaning.

8. **Recovery time balance:** How quickly should zones rebuild after disaster? Immediate (just clear debris) or lengthy recovery period?

### @services-engineer

9. **Hazard response dispatch algorithm:** Should hazard responders prioritize:
   - (A) Closest fire to responder
   - (B) Most intense fire in range
   - (C) Fire nearest to highest-value building
   - (D) Player-directed (click to dispatch)

10. **Response capacity limits:** Can a single hazard response post fight multiple fires simultaneously? Or does it dispatch "units" with limited capacity?

11. **Enforcer riot suppression:** How does enforcer coverage suppress riots? Instant suppression in coverage area, or gradual reduction?

12. **Service building disaster vulnerability:** Should hazard response posts and enforcer posts be protected from disasters (lower damage) or equally vulnerable?

### @graphics-engineer

13. **Fire visualization:** How should fire be rendered?
    - (A) Particle system per burning tile
    - (B) Fire texture overlay on buildings
    - (C) Animated fire sprites
    - (D) Shader-based fire effect

14. **Disaster visual effects:** What visual feedback for each disaster type?
    - Seismic: Screen shake, crack overlays
    - Inundation: Water shader, reflection
    - Vortex: Particle debris, rotation effect
    - Core breach: Radiation glow, Geiger static

15. **Damage state visualization:** Should damaged (but not destroyed) buildings show visual damage? Cracked walls, broken windows, etc.?

### @economy-engineer

16. **Disaster insurance:** Should there be an insurance mechanic to reduce disaster financial impact? Pay premiums, receive payout on damage?

17. **Disaster costs:** Beyond rebuilding, what economic costs?
    - Lost productivity during active disaster
    - Emergency response maintenance increase
    - Temporary population/demand penalties

18. **Core breach contamination costs:** How long should contamination persist? What cleanup cost?

---

## 13. Risks and Concerns

### 13.1 Architectural Risks

**RISK: Fire Spread Performance (MEDIUM)**
Fire spread algorithm iterates all burning tiles, checks neighbors. Could be expensive with large fires.

**Mitigation:**
- Use active tile list (only iterate burning tiles, not entire grid)
- Spatial chunking: process fire per 32x32 chunk
- Cap maximum simultaneous fires
- Consider LOD: distant fires update less frequently

**RISK: Multiplayer Disaster Sync (HIGH)**
Fire state changes frequently. Syncing every tile every tick is bandwidth-expensive.

**Mitigation:**
- Fire visualization is client-predictable (spread algorithm deterministic)
- Only sync ignition events and manual extinguishments
- Periodic full-state reconciliation
- Fire visual state is non-authoritative (cosmetic only)

**RISK: Cascade Disaster Complexity (MEDIUM)**
Disasters triggering other disasters (fire -> explosion -> more fire) could create runaway scenarios.

**Mitigation:**
- Cascade depth limit (max 2 levels)
- Cascade cooldown period
- Secondary disasters are weaker than primary
- Player can disable cascades in settings

**RISK: Cross-Boundary Griefing (HIGH)**
Player A could theoretically set fires that spread to Player B's colony.

**Mitigation:**
- Only natural disasters cross boundaries
- Player-triggered disasters stay within player's territory
- Fire spread probability reduces at ownership boundaries (firebreak effect)
- Consider: disasters pause at boundary, ask Player B for permission to enter

### 13.2 Design Ambiguities

**AMBIGUITY: Disaster Notification Timing**
How much warning before disasters?

**Proposed:**
- Conflagration: No warning (instant ignition)
- Seismic: 5-10 second warning (tremors)
- Inundation: 30 second warning (rising water)
- Vortex: 15 second warning (forming funnel)
- Mega storm: 60 second warning (weather pattern)
- Core breach: 20 second warning (reactor alarm)
- Civil unrest: 10 second warning (crowd forming)
- Titan: 30 second warning (emergence detected)

**AMBIGUITY: Building Repair vs Rebuild**
Can damaged buildings be repaired, or must they be demolished and rebuilt?

**Proposed:**
- Health > 50%: Auto-repair over time (if powered/watered)
- Health 1-50%: Manual repair required (costs credits)
- Health 0: Destroyed, must demolish debris and zone will auto-rebuild

**AMBIGUITY: Disaster-Resistant Buildings**
Should late-game buildings have higher disaster resistance?

**Proposed:**
- Base resistance by building material (density level)
- Research/upgrade could add resistance (future epic)
- Service buildings have innate bonus (emergency-hardened)

### 13.3 Technical Debt

**DEBT: Simple Disaster AI**
Initial implementation uses basic algorithms. Proper pathfinding for mobile disasters (vortex, titan) would require proper A* or similar.

**DEBT: No Disaster Prediction**
No "weather system" predicting storm likelihood. Disasters are purely random.

**DEBT: Hardcoded Probabilities**
Disaster frequencies will be hardcoded. Should eventually be data-driven and tunable.

---

## 14. Dependencies

### 14.1 What Epic 13 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 3 | ITerrainQueryable | Flammability from terrain type |
| Epic 3 | Elevation data | Flood level calculation |
| Epic 4 | BuildingSystem | Damage targets, destruction handling |
| Epic 4 | Building entities | DamageableComponent targets |
| Epic 5 | EnergySystem | Core breach source (nuclear nexus) |
| Epic 5 | Energy infrastructure | Seismic secondary fires |
| Epic 7 | TransportSystem | Riot spread via pathways |
| Epic 7 | Infrastructure entities | Road/conduit damage |
| Epic 9 | ServicesSystem | IEmergencyResponder for response |
| Epic 9 | Service coverage | Hazard response effectiveness |
| Epic 10 | DisorderSystem | Civil unrest trigger threshold |
| Epic 10 | ContaminationSystem | Core breach contamination |
| Epic 12 | UISystem | Disaster notifications |

### 14.2 What Later Epics Need from Epic 13

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 14 (Progression) | Disaster survival milestones | Disaster statistics |
| Epic 15 (Audio) | Disaster audio cues | DisasterEvent emissions |
| Epic 16 (Persistence) | Disaster state saving | Serializable components |

### 14.3 Stub Requirements

If implementing Epic 13 before Epic 9 (ServicesSystem):

```cpp
// Stub IEmergencyResponder
class StubEmergencyResponder : public IEmergencyResponder {
    bool can_respond_to(DisasterType) const override { return false; }
    bool dispatch_to(GridPosition) override { return false; }
    float get_response_effectiveness() const override { return 0.0f; }
};
```

If implementing before full Epic 10:

```cpp
// Stub disorder query
float StubDisorderSystem::get_disorder_at(GridPosition pos) { return 0.0f; }
```

---

## 15. Integration Points

### 15.1 BuildingSystem Integration (Epic 4)

```cpp
// BuildingSystem listens for damage events
EventBus::subscribe<DamageEvent>([this](const DamageEvent& e) {
    if (!registry.has<DamageableComponent>(e.target)) return;

    auto& dmg = registry.get<DamageableComponent>(e.target);
    dmg.health = std::max(0u, dmg.health - e.amount);

    if (dmg.health == 0 && !dmg.is_destroyed) {
        dmg.is_destroyed = true;
        destroy_building(e.target, e.type);
    }
});

void BuildingSystem::destroy_building(EntityID building, DamageType cause) {
    auto pos = registry.get<PositionComponent>(building);

    // Create debris entity
    auto debris = registry.create();
    registry.emplace<PositionComponent>(debris, pos);
    registry.emplace<DebrisComponent>(debris, {
        .original_building = registry.get<BuildingComponent>(building).type,
        .clear_cost = calculate_clear_cost(building)
    });

    // Remove building
    registry.destroy(building);

    emit BuildingDestroyedEvent { building, cause };
}
```

### 15.2 ServicesSystem Integration (Epic 9)

```cpp
// DisasterSystem queries hazard response coverage
void DisasterSystem::apply_hazard_response() {
    auto& conflag_grid = get_conflagration_grid();

    for (auto pos : conflag_grid.get_active_fire_positions()) {
        float coverage = services_system.get_coverage_at(
            ServiceType::HazardResponse, pos
        );

        if (coverage > 0.0f) {
            float extinguish = BASE_EXTINGUISH_RATE * coverage;
            conflag_grid.reduce_intensity(pos.x, pos.y, extinguish);
        }
    }
}

// ServicesSystem provides responders
std::vector<EntityID> ServicesSystem::get_responders(
    ServiceType type, PlayerID owner
) const {
    // Return all service buildings of type owned by player
}
```

### 15.3 ContaminationSystem Integration (Epic 10)

```cpp
// Core breach creates contamination
void DisasterSystem::apply_core_breach_contamination(
    const DisasterComponent& disaster
) {
    GridPosition center = disaster.epicenter;
    uint32_t radius = disaster.radius;
    uint8_t intensity = disaster.severity * 2;  // Strong contamination

    contamination_system.add_contamination_source(
        center, radius, intensity,
        ContaminationType::Energy  // Radioactive contamination
    );
}
```

### 15.4 UISystem Integration (Epic 12)

```cpp
// Disaster events for UI notifications
struct DisasterWarningEvent {
    DisasterType type;
    GridPosition epicenter;
    uint32_t severity;
    uint32_t warning_ticks;
};

struct DisasterActiveEvent {
    EntityID disaster_id;
    DisasterType type;
    GridPosition epicenter;
};

struct DisasterEndedEvent {
    EntityID disaster_id;
    DisasterType type;
    uint32_t buildings_destroyed;
    int64_t estimated_damage;
};

// UISystem displays notifications
void UISystem::on_disaster_warning(const DisasterWarningEvent& e) {
    Notification notif {
        .priority = NotificationPriority::Critical,
        .title = get_disaster_name(e.type) + " Warning!",
        .message = format("Incoming {} detected! Severity: {}",
                          get_disaster_name(e.type), e.severity),
        .position = e.epicenter,
        .duration_ms = e.warning_ticks * 50  // Match warning duration
    };
    show_notification(notif);
}
```

---

## 16. Proposed Interfaces

### 16.1 IDisasterProvider Interface

```cpp
class IDisasterProvider {
public:
    virtual ~IDisasterProvider() = default;

    // === Active Disaster Queries ===

    // Get all currently active disasters
    virtual std::vector<DisasterInfo> get_active_disasters() const = 0;

    // Get disaster affecting a specific position
    virtual std::optional<DisasterInfo> get_disaster_at(GridPosition pos) const = 0;

    // Check if position is in any disaster zone
    virtual bool is_in_disaster_zone(GridPosition pos) const = 0;

    // Get the dominant disaster type at position (for multiple overlapping)
    virtual std::optional<DisasterType> get_disaster_type_at(GridPosition pos) const = 0;

    // === Fire-Specific Queries ===

    // Check if tile is on fire
    virtual bool is_on_fire(int32_t x, int32_t y) const = 0;

    // Get fire intensity (0-255)
    virtual uint8_t get_fire_intensity(int32_t x, int32_t y) const = 0;

    // Get all fire positions (for rendering)
    virtual std::vector<std::pair<GridPosition, uint8_t>> get_all_fires() const = 0;

    // === Recovery Zone Queries ===

    // Get recovery zones
    virtual std::vector<RecoveryZoneInfo> get_recovery_zones() const = 0;

    // Check if position is in recovery
    virtual bool is_in_recovery(GridPosition pos) const = 0;

    // === Statistics ===

    // Get disaster history for player
    virtual DisasterStatistics get_statistics(PlayerID owner) const = 0;

    // Get total damage dealt by disasters
    virtual int64_t get_total_damage(PlayerID owner) const = 0;
};

struct DisasterInfo {
    EntityID entity_id;
    DisasterType type;
    DisasterPhase phase;
    GridPosition epicenter;
    uint32_t radius;
    uint32_t severity;
    uint32_t ticks_remaining;
};

struct RecoveryZoneInfo {
    EntityID entity_id;
    GridRect bounds;
    DisasterType source_disaster;
    uint32_t debris_remaining;
    uint32_t buildings_destroyed;
};

struct DisasterStatistics {
    uint32_t disasters_survived;
    uint32_t buildings_lost;
    uint32_t infrastructure_lost;
    int64_t total_damage_credits;
    DisasterType worst_disaster;
};
```

### 16.2 Disaster Event Definitions

```cpp
// Warning before disaster
struct DisasterWarningEvent {
    DisasterType type;
    GridPosition epicenter;
    uint32_t severity;
    uint32_t warning_ticks;
    std::vector<PlayerID> affected_players;
};

// Disaster becomes active
struct DisasterActiveEvent {
    EntityID disaster_id;
    DisasterType type;
    GridPosition epicenter;
    uint32_t severity;
};

// Disaster phase change
struct DisasterPhaseChangeEvent {
    EntityID disaster_id;
    DisasterPhase old_phase;
    DisasterPhase new_phase;
};

// Disaster fully resolved
struct DisasterEndedEvent {
    EntityID disaster_id;
    DisasterType type;
    uint32_t duration_ticks;
    uint32_t buildings_destroyed;
    uint32_t beings_displaced;
    int64_t total_damage;
};

// Entity took damage
struct DamageEvent {
    EntityID target;
    uint32_t amount;
    DamageType type;
    EntityID source_disaster;
    GridPosition source_position;
};

// Building destroyed by disaster
struct BuildingDestroyedByDisasterEvent {
    EntityID building_id;
    EntityID disaster_id;
    DamageType damage_type;
    PlayerID owner;
    GridPosition position;
};
```

---

## 17. Canon Update Proposals

### 17.1 systems.yaml Addition

```yaml
phase_4:
  epic_13_disasters:
    name: "Disasters"
    systems:
      DisasterSystem:
        type: ecs_system
        tick_priority: 75  # After DisorderSystem (70), before ContaminationSystem (80)
        owns:
          - Disaster triggering (random or manual)
          - Disaster progression state machine
          - Damage calculation
          - Fire spread simulation (ConflagrationGrid)
          - Emergency response dispatch coordination
          - Recovery tracking
        does_not_own:
          - Building destruction (BuildingSystem handles via DamageEvent)
          - Fire spread visualization (RenderingSystem owns)
          - Hazard response placement (ServicesSystem owns)
        provides:
          - "IDisasterProvider: disaster state queries"
          - Active disaster info
          - Damage reports
          - Fire spread state
          - Recovery zone info
        depends_on:
          - BuildingSystem (damage targets)
          - ServicesSystem (emergency response)
          - TerrainSystem (flammability, flood elevation)
          - TransportSystem (riot spread via pathways)
        multiplayer:
          authority: server
          per_player: false  # Disasters are global
        notes:
          - "Uses ConflagrationGrid dense grid for fire spread"
          - "Disasters can cross ownership boundaries"
          - "Damage events processed by target systems"

        components:
          - DisasterComponent
          - DamageableComponent
          - OnFireComponent
          - RecoveryZoneComponent
```

### 17.2 interfaces.yaml Addition

```yaml
disasters:
  IDisasterProvider:
    description: "Provides disaster state queries for other systems"
    purpose: "Allows UISystem, RenderingSystem to query disaster state"

    methods:
      - name: get_active_disasters
        params: []
        returns: "std::vector<DisasterInfo>"
        description: "All currently active disasters"

      - name: is_on_fire
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: bool
        description: "Whether tile is currently on fire"

      - name: get_fire_intensity
        params:
          - name: x
            type: int32_t
          - name: y
            type: int32_t
        returns: uint8_t
        description: "Fire intensity at tile (0-255)"

      - name: get_statistics
        params:
          - name: owner
            type: PlayerID
        returns: DisasterStatistics
        description: "Disaster survival statistics for player"

    implemented_by:
      - DisasterSystem (Epic 13)

    notes:
      - "Fire state used by RenderingSystem for visualization"
      - "Statistics used by ProgressionSystem for milestones"
```

### 17.3 patterns.yaml Addition

Add to `dense_grid_exception.applies_to`:

```yaml
- "ConflagrationGrid (Epic 13): 3 bytes/tile (intensity + flammability + fuel) for fire spread simulation"
```

### 17.4 interfaces.yaml Update for IDamageable/IEmergencyResponder

The interfaces.yaml already defines these correctly. Verify implementation matches:

```yaml
disasters:
  IDamageable:
    # Already defined correctly

  IEmergencyResponder:
    # Already defined correctly
```

---

## Appendix A: Disaster Configuration Reference

```cpp
const std::map<DisasterType, DisasterConfig> DISASTER_CONFIGS = {
    {DisasterType::Conflagration, {
        .warning_duration_ticks = 0,     // No warning
        .min_duration_ticks = 100,       // 5 seconds
        .max_duration_ticks = 2000,      // 100 seconds
        .base_damage_per_tick = 3,
        .base_radius = 1,
        .max_radius = 100,               // Can spread entire map
        .spread_rate = 0.05f,
        .random_probability = 0.0002f,   // ~1 per 100 cycles
        .can_trigger_manually = true,
        .respects_ownership = false
    }},
    {DisasterType::SeismicEvent, {
        .warning_duration_ticks = 100,   // 5 seconds warning
        .min_duration_ticks = 20,        // 1 second active
        .max_duration_ticks = 60,        // 3 seconds max
        .base_damage_per_tick = 50,      // High instant damage
        .base_radius = 20,
        .max_radius = 50,
        .spread_rate = 0.0f,             // Doesn't spread
        .random_probability = 0.00005f,  // ~1 per 400 cycles
        .can_trigger_manually = true,
        .respects_ownership = false
    }},
    // ... etc for each type
};
```

---

## Appendix B: Entity Composition Examples

**Active Conflagration:**
```
Entity {
    DisasterComponent {
        disaster_type: Conflagration,
        phase: Active,
        epicenter: { 100, 100 },
        radius: 15,
        max_radius: 100,
        severity: 60,
        tick_started: 5000,
        tick_active: 5000,
        duration_ticks: 0,  // Duration based on fire spread
        ticks_remaining: 0,
        is_natural: true,
        affected_players: [1, 2],
        affected_player_count: 2
    }
}
```

**Damageable Building:**
```
Entity {
    PositionComponent { grid_x: 100, grid_y: 102 }
    OwnershipComponent { owner: PLAYER_1 }
    BuildingComponent { type: HighDensityHabitation, state: Active }
    DamageableComponent {
        health: 150,
        max_health: 200,
        resistances: { fire: 30, seismic: 40, flood: 10, storm: 20, explosion: 25 },
        is_destroyed: false,
        is_repairable: true,
        last_damage_tick: 5050
    }
}
```

**Building On Fire:**
```
Entity {
    PositionComponent { grid_x: 100, grid_y: 102 }
    OnFireComponent {
        intensity: 180,
        tick_ignited: 5020,
        ticks_burning: 30,
        is_spreading: true,
        is_being_extinguished: true,
        extinguish_progress: 25
    }
}
```

**Recovery Zone:**
```
Entity {
    PositionComponent { grid_x: 90, grid_y: 90 }
    GridRectComponent { x: 90, y: 90, width: 30, height: 30 }
    RecoveryZoneComponent {
        source_disaster: Conflagration,
        debris_remaining: 15,
        buildings_destroyed: 8,
        tick_disaster_ended: 7000,
        auto_rebuild_eligible: true
    }
}
```

---

## Summary

Epic 13 (Disasters) is a **complex cross-cutting epic** that:

1. Introduces a **new ECS system** (DisasterSystem) with tick_priority 75
2. Requires a **dense grid** (ConflagrationGrid) for fire spread simulation
3. Implements the **IDamageable interface** across all buildings and infrastructure
4. Integrates with **ServicesSystem** (Epic 9) for emergency response
5. Creates **multiplayer coordination** challenges for cross-boundary disasters
6. Provides **events and interfaces** for UI, audio, and progression integration

The epic is scoped to **50 work items** covering infrastructure, 10 disaster types, damage systems, fire mechanics, emergency response, recovery, and multiplayer considerations.

**Critical Integration Points:**
- BuildingSystem must handle DamageEvent and building destruction
- ServicesSystem must provide IEmergencyResponder for hazard response
- RenderingSystem must visualize fire state from ConflagrationGrid
- UISystem must display disaster notifications
- ContaminationSystem must receive core breach contamination

**Key Risks:**
- Fire spread performance at scale
- Multiplayer sync for rapidly changing fire state
- Cross-boundary disaster griefing potential

---

**End of Systems Architect Analysis: Epic 13 - Disasters**
