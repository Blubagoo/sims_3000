# Building Engineer Analysis -- Epic 4: Zoning & Building System

**Agent:** Building Engineer
**Epic:** 4 -- Zoning & Building System (BuildingSystem half)
**Canon Version:** 0.6.0
**Date:** 2026-01-28

---

## Table of Contents

1. [BuildingComponent Design](#1-buildingcomponent-design)
2. [ConstructionComponent Design](#2-constructioncomponent-design)
3. [Building State Machine](#3-building-state-machine)
4. [Template System Implementation](#4-template-system-implementation)
5. [Building Spawning Logic](#5-building-spawning-logic)
6. [Building Upgrades/Downgrades](#6-building-upgradesdowngrades)
7. [Demolition System](#7-demolition-system)
8. [Forward Dependency Stubs](#8-forward-dependency-stubs)
9. [Building Queries API](#9-building-queries-api)
10. [Multiplayer](#10-multiplayer)
11. [Key Work Items](#11-key-work-items)
12. [Questions for Other Agents](#12-questions-for-other-agents)
13. [Risks & Concerns](#13-risks--concerns)

---

## 1. BuildingComponent Design

### Purpose

BuildingComponent is the primary data component attached to every structure entity. It holds all non-transient data about a structure -- its identity, type, current state, capacity, and physical attributes. Per canon, components are pure data with no logic.

### Struct Definition

```cpp
enum class BuildingState : uint8_t {
    Materializing = 0,  // Under construction (canon: "materializing")
    Active        = 1,  // Operational, serving its function
    Abandoned     = 2,  // Lost utilities/conditions, heading toward derelict
    Derelict      = 3,  // Canon: "abandoned" => "derelict" -- fully non-functional
    Deconstructed = 4   // Canon: "demolished" => "deconstructed" -- debris remains
};

enum class ZoneBuildingType : uint8_t {
    Habitation  = 0,  // Canon: residential => habitation
    Exchange    = 1,  // Canon: commercial => exchange
    Fabrication = 2   // Canon: industrial => fabrication
};

enum class DensityLevel : uint8_t {
    Low  = 0,
    High = 1
};

struct BuildingComponent {
    // Identity
    uint32_t template_id       = 0;       // Reference to BuildingTemplate
    ZoneBuildingType zone_type  = ZoneBuildingType::Habitation;
    DensityLevel density        = DensityLevel::Low;

    // State
    BuildingState state         = BuildingState::Materializing;
    uint8_t level               = 1;      // Current development level (1 = base, max TBD per density)
    uint8_t health              = 100;    // 0-255, for IDamageable (Epic 13)

    // Capacity
    uint16_t capacity           = 0;      // Inhabitants (habitation) or laborers (exchange/fabrication)
    uint16_t current_occupancy  = 0;      // Filled by PopulationSystem (Epic 10)

    // Physical
    uint8_t footprint_w         = 1;      // Width in tiles (1 for low density, 1-2+ for high density)
    uint8_t footprint_h         = 1;      // Height in tiles

    // Timing
    uint32_t state_changed_tick = 0;      // Tick when state last changed
    uint16_t abandon_timer      = 0;      // Ticks remaining before abandoned->derelict transition

    // Variation (for rendering)
    uint8_t rotation            = 0;      // 0-3 (0/90/180/270 degrees)
    uint8_t color_accent_index  = 0;      // Index into template color accent palette
};
```

### Size Analysis

- Total struct size: 28 bytes (packed)
- Practical aligned size: 32 bytes (with padding for alignment)
- At 10,000 structures on a 512x512 map: ~320 KB
- This is well within memory budget and trivially copyable for network serialization.

### Design Rationale

- **template_id as uint32_t**: Allows up to ~4 billion templates. In practice, we will have dozens to low hundreds. Using uint32_t keeps serialization simple and avoids string hashing at runtime.
- **level (uint8_t)**: Development level within a density tier. Low density: levels 1-3. High density: levels 1-5. Level drives model variant and capacity multiplier.
- **health (uint8_t)**: 0-255 range. Used by DisasterSystem (Epic 13) via IDamageable. 0 = destroyed.
- **capacity/current_occupancy as uint16_t**: Supports buildings up to 65,535 occupants. Largest high-density habitation tower might house ~5,000 beings.
- **abandon_timer**: Counts down when structure loses essential services. Prevents flickering between active/abandoned on brief power outages.
- **rotation and color_accent_index**: Procedural variation stored per-structure for deterministic rendering across clients.

### Canon Compliance

- Uses canon terminology: `Materializing`, `Derelict`, `Deconstructed` for states
- Zone types use canon names: `Habitation`, `Exchange`, `Fabrication`
- Pure data struct, no methods (ECS rule)
- Trivially copyable for ISerializable
- No pointers -- template_id is a lookup key, not a pointer

---

## 2. ConstructionComponent Design

### Purpose

ConstructionComponent is a temporary component attached to structure entities while they are in the `Materializing` state. It tracks construction progress, timing, and animation state. Removed when construction completes.

### Struct Definition

```cpp
enum class ConstructionPhase : uint8_t {
    Foundation   = 0,  // Initial phase -- ground preparation
    Framework    = 1,  // Structural skeleton emerges
    Exterior     = 2,  // Shell completion
    Finalization = 3   // Final touches, glow activation
};

struct ConstructionComponent {
    // Progress
    uint16_t ticks_total       = 0;   // Total ticks to complete
    uint16_t ticks_elapsed     = 0;   // Ticks completed so far

    // Animation
    ConstructionPhase phase    = ConstructionPhase::Foundation;
    uint8_t phase_progress     = 0;   // 0-255 within current phase (for smooth interpolation)

    // Cost tracking
    uint32_t construction_cost = 0;   // Total cost (debited at start)

    // State
    bool is_paused             = false; // Paused due to lack of resources or owner action
};
```

### Size Analysis

- Total: 12 bytes (packed), 12 bytes aligned
- Only exists on structures currently materializing, so count is typically low (dozens at most)

### Progress Mechanics

- **Ticks total** depends on template: low-density dwelling = ~40 ticks (2 seconds at 20 ticks/sec). High-density tower = ~200 ticks (10 seconds).
- **Phase transitions** are derived from progress percentage:
  - Foundation: 0-25%
  - Framework: 25-50%
  - Exterior: 50-75%
  - Finalization: 75-100%
- **phase_progress (0-255)** within each phase provides smooth interpolation for the materializing animation. RenderingSystem reads this to lerp the visual effect.

### Pause Behavior

- If `is_paused` is true, `ticks_elapsed` does not increment.
- Pause triggers: structure is in a zone that lost road access mid-construction, or player-initiated pause (future feature).
- NOTE: For initial implementation, we do NOT pause for missing energy/fluid during construction. Utilities affect the Active state, not Materializing.

### Lifecycle

1. ConstructionComponent is created when a structure entity is spawned.
2. BuildingSystem increments `ticks_elapsed` each simulation tick.
3. When `ticks_elapsed >= ticks_total`, BuildingSystem:
   - Removes ConstructionComponent from the entity.
   - Transitions BuildingComponent.state from `Materializing` to `Active`.
   - Emits `BuildingConstructedEvent`.

---

## 3. Building State Machine

### States and Transitions

```
                    +-----------------+
                    |  Materializing  |
                    | (constructing)  |
                    +--------+--------+
                             |
                   construction_complete
                             |
                             v
                    +--------+--------+
           +------>|     Active      |<------+
           |       | (operational)   |       |
           |       +--------+--------+       |
           |                |                |
     conditions_restored    |        conditions_restored
           |         lost_essentials         |
           |                |                |
           |                v                |
           |       +--------+--------+       |
           +-------+   Abandoned     +-------+
                   | (non-functional)|
                   +--------+--------+
                            |
                    abandon_timer_expired
                            |
                            v
                   +--------+--------+
                   |    Derelict     |
                   | (beyond saving) |
                   +--------+--------+
                            |
                   derelict_timer_expired
                     OR player_demolish
                            |
                            v
                   +--------+--------+
                   |  Deconstructed  |
                   |   (debris)      |
                   +-----------------+
                            |
                     debris_cleared
                            |
                            v
                      [Entity removed]
```

### Transition Triggers

#### Materializing --> Active
- **Trigger:** `ConstructionComponent.ticks_elapsed >= ConstructionComponent.ticks_total`
- **Action:** Remove ConstructionComponent. Set BuildingComponent.state = Active. Emit `BuildingConstructedEvent`. Begin consuming energy/fluid if available.

#### Active --> Abandoned
- **Trigger:** Structure loses essential service AND `abandon_grace_ticks` has elapsed.
- **Essential services** (checked via forward dependency stubs initially):
  - `is_powered() == false` for configurable duration (e.g., 100 ticks / 5 seconds)
  - `has_water() == false` for configurable duration
  - `is_road_accessible() == false` (immediate -- no grace period for road loss)
- **Grace period rationale:** Prevents flickering during brief outages. A brownout lasting 2 seconds should not trigger abandonment.
- **Action:** Set BuildingComponent.state = Abandoned. Set `abandon_timer` to configurable value (e.g., 200 ticks / 10 seconds). Emit `BuildingAbandonedEvent`. Structure stops producing tax revenue, stops generating traffic, occupants begin leaving (PopulationSystem concern).

#### Abandoned --> Active
- **Trigger:** All essential services restored while structure is still in Abandoned state (before timer expires).
- **Action:** Set BuildingComponent.state = Active. Reset `abandon_timer`. Emit `BuildingRestoredEvent`. Occupants return over time (PopulationSystem concern).

#### Abandoned --> Derelict
- **Trigger:** `abandon_timer` reaches zero without services being restored.
- **Action:** Set BuildingComponent.state = Derelict. Structure is now beyond recovery through normal means. Visual decay intensifies. Emits `BuildingDerelictEvent`.
- **NOTE:** Derelict structures cannot transition back to Active. They must be deconstructed and the zone will spawn a new structure if conditions are met.

#### Derelict --> Deconstructed
- **Trigger:** Timer expiration (configurable, e.g., 500 ticks / 25 seconds) OR player-initiated demolition.
- **Action:** Replace structure entity with a debris entity. Set BuildingComponent.state = Deconstructed. Emit `BuildingDeconstructedEvent`. Tile is now occupied by debris.

#### Deconstructed --> Entity Removed
- **Trigger:** Debris clears automatically after a short delay (e.g., 60 ticks / 3 seconds) OR player pays to clear debris immediately.
- **Action:** Remove entity from ECS registry. Free the tile(s) for new development. Emit `DebrisClearedEvent`.

### State Duration Summary

| Transition | Duration | Notes |
|-----------|----------|-------|
| Materializing | 40-200 ticks (2-10s) | Template-dependent |
| Active -> Abandoned grace | 100 ticks (5s) | Utility loss grace period |
| Abandoned -> Derelict | 200 ticks (10s) | Can be reversed |
| Derelict -> Deconstructed | 500 ticks (25s) | Auto-demolish |
| Deconstructed (debris) -> Cleared | 60 ticks (3s) | Auto-clear |

All durations are configurable constants, not hardcoded.

---

## 4. Template System Implementation

### Template Data Structure

```cpp
struct BuildingTemplate {
    // Identity
    uint32_t template_id;                  // Unique ID (hash of template name)
    std::string name;                      // Human-readable name (debug only)

    // Classification
    ZoneBuildingType zone_type;            // Habitation, Exchange, Fabrication
    DensityLevel density;                  // Low, High

    // Model
    enum class ModelSource : uint8_t {
        Procedural = 0,
        Asset      = 1
    };
    ModelSource model_source;
    std::string model_path;               // Path to .glb if Asset source
    // Procedural params (if Procedural source):
    uint8_t procedural_floors;            // Number of floors for procedural model
    uint8_t procedural_style;             // Style variant index

    // Physical
    uint8_t footprint_w;                  // Width in tiles
    uint8_t footprint_h;                  // Height in tiles

    // Requirements
    uint32_t min_land_value;              // Minimum sector value to spawn
    uint8_t min_level;                    // Minimum building level to use this template

    // Capacity
    uint16_t base_capacity;               // Inhabitants or laborers at level 1

    // Construction
    uint16_t construction_ticks;          // Time to build
    uint32_t construction_cost;           // Credits to construct

    // Animation
    // (construction_animation is defined per-template as canon requires)
    uint8_t construction_animation_id;    // Index into animation table

    // Utility requirements
    uint16_t energy_required;             // Energy units per tick when active
    uint16_t fluid_required;              // Fluid units per tick when active

    // Contamination (fabrication only)
    uint16_t contamination_output;        // Contamination units per tick (0 for hab/exchange)

    // Variation
    uint8_t color_accent_count;           // Number of accent color variants
};
```

### Template Loading

Templates are loaded from a data file at startup. The format will be a simple binary or structured text format (decision deferred to Data Engineer). The loading flow is:

1. **At startup:** `BuildingTemplateRegistry::load(path)` reads all template definitions.
2. **Registry stores:** `std::unordered_map<uint32_t, BuildingTemplate>` keyed by `template_id`.
3. **Pool indexes:** `std::unordered_map<TemplatePoolKey, std::vector<uint32_t>>` where `TemplatePoolKey` is `{zone_type, density}`. Each pool contains template_ids valid for that zone+density combination.

```cpp
struct TemplatePoolKey {
    ZoneBuildingType zone_type;
    DensityLevel density;

    bool operator==(const TemplatePoolKey& other) const {
        return zone_type == other.zone_type && density == other.density;
    }
};
```

### Template Pool Sizes (Target)

| Zone Type | Low Density | High Density |
|-----------|------------|--------------|
| Habitation | 8-12 templates | 6-10 templates |
| Exchange | 6-8 templates | 4-6 templates |
| Fabrication | 4-6 templates | 3-5 templates |

Total: ~30-47 templates. This provides sufficient variety without excessive asset cost.

### Selection Algorithm

When BuildingSystem needs to spawn a structure, template selection proceeds:

```
function select_template(zone_type, density, land_value, neighbor_templates):
    1. Get template pool for (zone_type, density)
    2. Filter: remove templates where min_land_value > current land_value
    3. Filter: remove templates where min_level > 1 (level 1 structures only at spawn)
    4. Weight remaining templates:
       a. Base weight: 1.0 for all candidates
       b. Land value bonus: +0.5 if land_value > template.min_land_value * 1.5
       c. Duplicate penalty: -0.7 if template_id matches any of the 4 nearest neighbors
       d. Clamp weight to minimum 0.1 (never fully eliminate a template)
    5. Weighted random selection using server RNG
    6. Return selected template_id
```

**Neighbor duplicate avoidance:** We check the 4 orthogonally adjacent tiles (not diagonal) for existing structures. If a neighbor uses the same template_id, that template gets a penalty. This prevents rows of identical structures.

### Procedural Variation Parameters

Each spawned structure gets random variation applied at creation time, stored in BuildingComponent:

- **rotation:** Random from {0, 1, 2, 3} (0/90/180/270 degrees). All 4 rotations equally weighted.
- **color_accent_index:** Random from {0, ..., template.color_accent_count - 1}. Drives minor color differences (door color, trim color, glow accent hue).
- **Scale variation is NOT applied.** Structures must fit their tile footprint exactly. Scale variation would cause overlap or gaps.

Variation is determined server-side using the seeded RNG, ensuring all clients see the same structure appearance.

---

## 5. Building Spawning Logic

### Spawn Conditions

A structure spawns in a zone tile when ALL of the following are true:

1. **Zone is designated:** Tile has a ZoneComponent with valid zone_type.
2. **Zone has demand:** DemandSystem reports positive growth_pressure for this zone_type. (Initially, a stub that always returns positive demand.)
3. **Tile is empty:** No existing structure occupies this tile.
4. **Terrain is buildable:** `ITerrainQueryable::is_buildable(x, y)` returns true.
5. **Road access:** `is_road_accessible(x, y)` returns true (within 3 tiles of a pathway). Stub returns true initially.
6. **Utilities available:** `is_powered()` AND `has_water()` return true for the owner. Stubs return true initially.
7. **Tile is owned:** OwnershipComponent.owner is a valid player (not GAME_MASTER). Structures do not spawn on unchartered sectors.

### Spawn Rate and Timing

- **Scan frequency:** BuildingSystem scans for spawnable tiles every N ticks (configurable, default: 20 ticks = 1 second).
- **Spawns per scan:** Maximum M structures per player per scan (configurable, default: 3). This prevents an explosion of simultaneous constructions.
- **Priority ordering:** Tiles closest to existing road network are spawned first. This creates organic growth outward from roads.
- **Randomized offset:** Each player's scan offset is staggered by `player_id * 5` ticks to avoid all players spawning in the same tick, distributing server load.

### Spawn Flow

```
BuildingSystem::tick(delta_time):
    if (current_tick % SPAWN_SCAN_INTERVAL != 0) return;

    for each player:
        spawned_count = 0
        for each zone_tile owned by player (shuffled):
            if spawned_count >= MAX_SPAWNS_PER_SCAN: break
            if can_spawn_building(tile):
                template = select_template(tile.zone_type, tile.density, land_value, neighbors)
                entity = spawn_building(tile, template)
                spawned_count++
```

### Multi-Tile Footprints

High-density structures may occupy more than one tile (e.g., 2x2 for a large tower).

**Placement rules for multi-tile footprints:**

1. The "anchor" tile is the top-left tile of the footprint.
2. ALL tiles in the footprint must be:
   - Same zone_type and density
   - Same owner
   - Empty (no existing structures)
   - Buildable terrain
3. Only the anchor tile stores the PositionComponent for the entity.
4. All occupied tiles are marked in a spatial index (BuildingGrid) pointing to the same entity ID.
5. Road access is checked from any tile in the footprint (at least one tile must be within 3 tiles of a pathway).

**BuildingGrid for occupancy tracking:**

```cpp
class BuildingGrid {
    // Dense 2D grid storing entity ID at each tile
    // INVALID_ENTITY (0) means tile is empty
    std::vector<EntityID> grid;  // size = map_width * map_height
    uint32_t width;
    uint32_t height;

public:
    EntityID get_building_at(int32_t x, int32_t y) const;
    void set_building_at(int32_t x, int32_t y, EntityID entity);
    void clear_building_at(int32_t x, int32_t y);
    bool is_tile_occupied(int32_t x, int32_t y) const;
    bool is_footprint_available(int32_t x, int32_t y, uint8_t w, uint8_t h) const;
};
```

Memory: 4 bytes per tile (EntityID = uint32_t). 512x512 = 1 MB. Acceptable.

The BuildingGrid follows the same dense grid exception pattern established by TerrainGrid in Epic 3. Rationale: every tile potentially has a structure, spatial lookups must be O(1), and per-entity overhead is prohibitive at scale.

---

## 6. Building Upgrades/Downgrades

### Upgrade Triggers

A structure upgrades (increases its `level`) when:

1. **Structure is Active** (not Materializing, Abandoned, Derelict, or Deconstructed).
2. **All utilities connected:** `is_powered()` AND `has_water()` AND `is_road_accessible()`.
3. **Land value threshold met:** Current sector value at the tile meets or exceeds the `min_land_value` for the next-level template.
4. **Time at current level:** Structure has been Active at current level for at least `UPGRADE_COOLDOWN` ticks (configurable, default: 200 ticks / 10 seconds).
5. **Demand exists:** Positive growth_pressure for this zone_type.

### Upgrade Mechanics

- **Level increase:** `BuildingComponent.level++`
- **Template change:** The structure may switch to a different template appropriate for the new level. Template selection uses the same algorithm as spawning, but filtered by `min_level <= new_level`.
- **Capacity increase:** Capacity scales with level. Formula: `template.base_capacity * level_multiplier[level]`. Suggested multipliers: [1.0, 1.5, 2.0, 2.5, 3.0].
- **Visual change:** Yes -- the model changes to reflect the new level. For low-density habitation, a level 1 dwelling might become a level 2 larger dwelling. For high-density, levels correspond to taller towers.
- **Upgrade animation:** The structure plays a brief materializing animation (faster than initial construction, ~20 ticks / 1 second) during which it transitions between models. A `ConstructionComponent` is temporarily attached with `ticks_total` set to the upgrade duration.
- **During upgrade:** The structure remains in Active state but renders the upgrade animation. Occupants stay.

### Downgrade Triggers

A structure downgrades (decreases its `level`) when:

1. **Land value drops** below the `min_land_value` threshold for the current level.
2. **Demand drops** significantly for the zone_type (DemandSystem reports negative growth_pressure sustained for `DOWNGRADE_DELAY` ticks).
3. **Service coverage drops** (services like education, medical -- future Epic 9 concern).

### Downgrade Mechanics

- **Level decrease:** `BuildingComponent.level--` (minimum 1).
- **Template change:** May revert to a lower-level template.
- **Capacity decrease:** Follows the same formula in reverse. Excess occupants are displaced (PopulationSystem handles migration).
- **Visual change:** Yes -- model downgrades.
- **Downgrade animation:** Brief animation (same as upgrade, ~20 ticks).
- **No cost:** Downgrades do not cost the player credits.

### Density Change (Zone Redesignation)

If the ZoneSystem changes a tile's density (e.g., low to high density):
- Existing structures in that zone are NOT automatically demolished.
- Over time, as structures are abandoned or deconstructed, new structures spawn at the new density level.
- The structure's density field does NOT change dynamically. A low-density structure on a now-high-density zone remains low-density until naturally replaced.

---

## 7. Demolition System

### Player-Initiated Demolition

Players can demolish any structure they own using the demolition tool.

### Demolition Flow

```
1. Player selects demolition tool and clicks a structure.
2. Client sends DemolishRequest(entity_id, player_id) to server.
3. Server validates:
   a. Entity exists and has BuildingComponent
   b. OwnershipComponent.owner == requesting player_id
   c. Structure is not already Deconstructed
4. Server calculates demolition cost:
   a. Base cost: template.construction_cost * DEMOLITION_COST_RATIO (e.g., 0.25 = 25% of build cost)
   b. State modifier: Active = 1.0x, Materializing = 0.5x (partial refund), Abandoned = 0.1x, Derelict = 0.0x (free)
   c. Total = base_cost * state_modifier
5. Server debits credits from player (if insufficient, reject with error).
6. Server transitions structure to Deconstructed:
   a. Set BuildingComponent.state = Deconstructed
   b. Remove ConstructionComponent if present
   c. Clear occupancy from BuildingGrid for all footprint tiles
   d. Create debris entity at same position
   e. Emit BuildingDeconstructedEvent
7. Debris entity auto-clears after DEBRIS_CLEAR_TICKS (60 ticks / 3s).
8. After debris clears, tile is free for new development.
```

### Cost Calculation Summary

| Structure State | Cost Multiplier | Rationale |
|----------------|-----------------|-----------|
| Materializing | 0.5x base | Partial refund for aborting construction |
| Active | 1.0x base | Full demolition cost |
| Abandoned | 0.1x base | Mostly symbolic cost |
| Derelict | 0.0x (free) | Clearing derelict structures is free |

### Demolition While Zone is Designated

- **Yes, you can demolish while zone is still designated.** Demolishing a structure does not remove the zone designation.
- After debris clears, the zone tile is empty and eligible for new structure spawning (assuming demand and conditions are met).
- This allows players to demolish undesirable structures and let new ones grow, or to clear derelict structures.

### Debris Entity

```cpp
struct DebrisComponent {
    uint32_t original_template_id;  // For visual reference (what kind of debris)
    uint16_t clear_timer;           // Ticks until auto-clear
    uint8_t footprint_w;            // Original footprint
    uint8_t footprint_h;
};
```

- Debris occupies the same tiles as the original structure.
- Debris blocks new construction until cleared.
- Debris is purely visual + occupancy -- no gameplay effect.
- Auto-clears after timer. Player can pay to clear immediately (cost: nominal, e.g., 10 credits).

---

## 8. Forward Dependency Stubs

### Problem

BuildingSystem depends on EnergySystem (Epic 5), FluidSystem (Epic 6), and TransportSystem (Epic 7). These systems do not exist yet during Epic 4 implementation.

### Stub Interface Pattern

We define abstract interfaces that BuildingSystem programs against. Stub implementations return permissive defaults. When the real systems are implemented, they provide concrete implementations that replace the stubs.

```cpp
// ============================================================
// IUtilityProvider -- Stub interface for forward dependencies
// ============================================================

class IEnergyProvider {
public:
    virtual ~IEnergyProvider() = default;
    virtual bool is_powered(EntityID entity) const = 0;
    virtual bool is_powered_at(int32_t x, int32_t y, PlayerID owner) const = 0;
};

class IFluidProvider {
public:
    virtual ~IFluidProvider() = default;
    virtual bool has_water(EntityID entity) const = 0;
    virtual bool has_water_at(int32_t x, int32_t y, PlayerID owner) const = 0;
};

class ITransportProvider {
public:
    virtual ~ITransportProvider() = default;
    virtual bool is_road_accessible(int32_t x, int32_t y) const = 0;
    virtual uint32_t get_nearest_road_distance(int32_t x, int32_t y) const = 0;
};
```

### Stub Implementations

```cpp
class StubEnergyProvider : public IEnergyProvider {
public:
    bool is_powered(EntityID) const override { return true; }
    bool is_powered_at(int32_t, int32_t, PlayerID) const override { return true; }
};

class StubFluidProvider : public IFluidProvider {
public:
    bool has_water(EntityID) const override { return true; }
    bool has_water_at(int32_t, int32_t, PlayerID) const override { return true; }
};

class StubTransportProvider : public ITransportProvider {
public:
    bool is_road_accessible(int32_t, int32_t) const override { return true; }
    uint32_t get_nearest_road_distance(int32_t, int32_t) const override { return 0; }
};
```

### Swap Strategy

BuildingSystem holds interface pointers, not concrete types:

```cpp
class BuildingSystem : public ISimulatable {
private:
    IEnergyProvider*    energy_provider_;    // Stub or real
    IFluidProvider*     fluid_provider_;     // Stub or real
    ITransportProvider* transport_provider_; // Stub or real

public:
    void set_energy_provider(IEnergyProvider* provider);
    void set_fluid_provider(IFluidProvider* provider);
    void set_transport_provider(ITransportProvider* provider);
};
```

At initialization:
- **Epic 4:** All three are stub implementations.
- **Epic 5:** `set_energy_provider(&real_energy_system)` -- energy is live, others are stubs.
- **Epic 6:** `set_fluid_provider(&real_fluid_system)` -- energy and fluid live, transport stub.
- **Epic 7:** `set_transport_provider(&real_transport_system)` -- all live.

This follows dependency injection and requires zero changes to BuildingSystem when real systems come online. The interfaces are defined in a shared header (e.g., `src/systems/interfaces/IUtilityProvider.h`).

### IDemandProvider Stub

Similarly, DemandSystem (Epic 10) does not exist yet:

```cpp
class IDemandProvider {
public:
    virtual ~IDemandProvider() = default;
    virtual float get_demand(ZoneBuildingType type) const = 0;
};

class StubDemandProvider : public IDemandProvider {
public:
    float get_demand(ZoneBuildingType) const override { return 1.0f; } // Always positive demand
};
```

### ILandValueProvider Stub

```cpp
class ILandValueProvider {
public:
    virtual ~ILandValueProvider() = default;
    virtual uint32_t get_land_value(int32_t x, int32_t y) const = 0;
};

class StubLandValueProvider : public ILandValueProvider {
public:
    uint32_t get_land_value(int32_t, int32_t) const override { return 50; } // Medium value
};
```

---

## 9. Building Queries API

### IBuildingQueryable Interface

Other systems need to query structure data. BuildingSystem exposes an interface:

```cpp
class IBuildingQueryable {
public:
    virtual ~IBuildingQueryable() = default;

    // === Spatial Queries ===

    // Get the building entity at a specific tile position.
    // Returns INVALID_ENTITY if no building at that tile.
    virtual EntityID get_building_at(int32_t x, int32_t y) const = 0;

    // Check if a tile is occupied by a building (including multi-tile footprints).
    virtual bool is_tile_occupied(int32_t x, int32_t y) const = 0;

    // Check if a footprint area is available for construction.
    virtual bool is_footprint_available(int32_t x, int32_t y,
                                        uint8_t w, uint8_t h) const = 0;

    // Get all building entities within a rectangular area.
    virtual std::vector<EntityID> get_buildings_in_rect(
        int32_t x, int32_t y, int32_t w, int32_t h) const = 0;

    // === Ownership Queries ===

    // Get all buildings owned by a specific player.
    virtual std::vector<EntityID> get_buildings_by_owner(PlayerID owner) const = 0;

    // Get count of buildings by owner (cheaper than getting full list).
    virtual uint32_t get_building_count_by_owner(PlayerID owner) const = 0;

    // === State Queries ===

    // Get the state of a specific building entity.
    virtual BuildingState get_building_state(EntityID entity) const = 0;

    // Get all buildings in a specific state.
    virtual std::vector<EntityID> get_buildings_by_state(BuildingState state) const = 0;

    // === Type Queries ===

    // Get count of buildings by zone type (across all players).
    virtual uint32_t get_building_count_by_type(ZoneBuildingType type) const = 0;

    // Get count of buildings by zone type for a specific player.
    virtual uint32_t get_building_count_by_type_and_owner(
        ZoneBuildingType type, PlayerID owner) const = 0;

    // === Capacity Queries ===

    // Get total capacity across all buildings of a type for a player.
    virtual uint32_t get_total_capacity(ZoneBuildingType type, PlayerID owner) const = 0;

    // Get total current occupancy across all buildings of a type for a player.
    virtual uint32_t get_total_occupancy(ZoneBuildingType type, PlayerID owner) const = 0;

    // === Statistics ===

    // Get building stats for a player (total count, active count, abandoned count, etc.)
    struct BuildingStats {
        uint32_t total;
        uint32_t materializing;
        uint32_t active;
        uint32_t abandoned;
        uint32_t derelict;
        uint32_t deconstructed;
    };
    virtual BuildingStats get_building_stats(PlayerID owner) const = 0;
};
```

### Query Consumers

| Consumer System | Queries Used | Purpose |
|----------------|-------------|---------|
| PopulationSystem (Epic 10) | get_total_capacity, get_buildings_by_owner, get_building_state | Assign beings to habitation structures |
| EconomySystem (Epic 11) | get_buildings_by_owner, get_building_count_by_type_and_owner | Calculate tribute (tax) revenue |
| DemandSystem (Epic 10) | get_total_capacity, get_total_occupancy | Calculate zone pressure |
| ContaminationSystem (Epic 10) | get_buildings_in_rect (fabrication type) | Fabrication contamination sources |
| DisorderSystem (Epic 10) | get_buildings_in_rect | Disorder generation per area |
| ServicesSystem (Epic 9) | get_buildings_in_rect | Service coverage targets |
| UISystem (Epic 12) | get_building_at, get_building_stats | Query tool display |
| DisasterSystem (Epic 13) | get_buildings_in_rect | Damage targets |

### Performance Considerations

- **get_building_at()**: O(1) via BuildingGrid dense array.
- **get_buildings_by_owner()**: Maintain per-player entity lists, updated on create/destroy. O(n) where n = buildings for that player.
- **get_building_count_by_type()**: Maintain counters, updated on create/destroy/state-change. O(1).
- **get_buildings_in_rect()**: Iterate BuildingGrid for the rect. O(w*h) but grid access is cache-friendly.

---

## 10. Multiplayer

### Server-Authoritative Spawning

All structure spawning decisions are made server-side. The client never spawns structures locally.

**Flow:**
1. Server's BuildingSystem runs on each simulation tick.
2. When spawn conditions are met, server creates the entity with all components.
3. Server's SyncSystem detects the new entity and sends a state update to all clients.
4. Clients create the entity in their local ECS registry and begin rendering.

### Building Ownership

- **Rule:** Structures inherit ownership from the zone tile they are built on.
- Stored in `OwnershipComponent.owner` on the structure entity.
- Owner is set at structure creation and does NOT change.
- If a player abandons their colony (ghost town process), their structures go through the Abandoned -> Derelict -> Deconstructed pipeline as managed by the AbandonmentSystem (SimulationCore).

### What Syncs

| Data | Sync Mode | Notes |
|------|-----------|-------|
| BuildingComponent (full) | On creation + on change | State transitions, level changes |
| ConstructionComponent | On creation + every N ticks | Progress updates for animation |
| PositionComponent | On creation only | Structures do not move |
| OwnershipComponent | On creation only | Ownership does not change |
| EnergyComponent | On change | When power state changes |
| FluidComponent | On change | When water state changes |
| DebrisComponent | On creation + on removal | Short-lived entities |

### Construction Progress Sync

- ConstructionComponent.ticks_elapsed is NOT synced every tick (too chatty).
- Server sends progress updates every 10 ticks (0.5 seconds).
- Clients interpolate between received progress values for smooth animation.
- On construction complete, server sends the state transition (Materializing -> Active) plus component removal.

### Concurrent Considerations

**Scenario: Two players zone adjacent tiles simultaneously.**
- Server processes zone requests sequentially.
- BuildingSystem scans players in order (staggered by player_id offset).
- No race condition: each tile has exactly one owner, and BuildingGrid prevents double-occupation.

**Scenario: Player demolishes while BuildingSystem is about to upgrade.**
- Demolish request arrives at server as a network message.
- Server processes messages before simulation tick.
- If demolish processed first, the structure is Deconstructed when BuildingSystem runs -- it skips upgrade checks for non-Active structures.

**Scenario: Two clients see different construction progress.**
- Client A might be slightly ahead of Client B due to network latency.
- This is purely visual -- server state is authoritative.
- When construction completes, all clients receive the same `BuildingConstructedEvent` and converge.

### Network Messages

```
Client -> Server:
  DemolishRequestMessage  { entity_id, player_id }
  ClearDebrisMessage      { entity_id, player_id }

Server -> Client:
  BuildingSpawnedMessage  { entity_id, position, template_id, owner, rotation, color_accent }
  BuildingStateChanged    { entity_id, new_state }
  BuildingUpgraded        { entity_id, new_level, new_template_id }
  ConstructionProgress    { entity_id, ticks_elapsed, ticks_total }
  BuildingDemolished      { entity_id }
  DebrisCleared           { entity_id, position }
```

---

## 11. Key Work Items

### Group A: Data Infrastructure

**4-B01: BuildingComponent and Enums**
- Type: infrastructure
- Scope: S
- Define BuildingState, ZoneBuildingType, DensityLevel enums.
- Define BuildingComponent struct per Section 1.
- static_assert on struct size.
- Unit tests for enum values and component initialization.

**4-B02: ConstructionComponent**
- Type: infrastructure
- Scope: S
- Define ConstructionPhase enum and ConstructionComponent struct per Section 2.
- Unit tests for progress calculation and phase transitions.

**4-B03: DebrisComponent**
- Type: infrastructure
- Scope: S
- Define DebrisComponent struct per Section 7.
- Unit tests for timer behavior.

**4-B04: BuildingGrid Dense Array**
- Type: infrastructure
- Scope: M
- Implement BuildingGrid class per Section 5.
- Dense 2D array of EntityID, O(1) lookups.
- Footprint availability checking.
- Memory validation at 512x512 = 1 MB.
- Unit tests for set/get/clear, multi-tile footprints, bounds checking.

**4-B05: Building Events**
- Type: infrastructure
- Scope: S
- Define all building-related events:
  - BuildingConstructedEvent { entity_id, owner, zone_type, position, template_id }
  - BuildingAbandonedEvent { entity_id, owner, position }
  - BuildingRestoredEvent { entity_id, owner, position }
  - BuildingDerelictEvent { entity_id, owner, position }
  - BuildingDeconstructedEvent { entity_id, owner, position, was_player_initiated }
  - DebrisClearedEvent { entity_id, position }
  - BuildingUpgradedEvent { entity_id, old_level, new_level }
  - BuildingDowngradedEvent { entity_id, old_level, new_level }

### Group B: Forward Dependency Interfaces

**4-B06: IEnergyProvider, IFluidProvider, ITransportProvider Interfaces**
- Type: infrastructure
- Scope: S
- Define abstract interfaces per Section 8.
- Place in `src/systems/interfaces/`.
- No dependencies on unimplemented systems.

**4-B07: Stub Implementations**
- Type: infrastructure
- Scope: S
- Implement StubEnergyProvider, StubFluidProvider, StubTransportProvider, StubDemandProvider, StubLandValueProvider per Section 8.
- Unit tests verifying stubs return expected defaults.

### Group C: Template System

**4-B08: BuildingTemplate Data Structure**
- Type: infrastructure
- Scope: M
- Define BuildingTemplate struct, TemplatePoolKey, BuildingTemplateRegistry per Section 4.
- Implement template loading from data file (initially hardcoded set for development).
- Pool indexing by zone_type + density.
- Unit tests for registry lookup, pool filtering.

**4-B09: Template Selection Algorithm**
- Type: feature
- Scope: M
- Implement weighted random selection per Section 4.
- Land value filtering.
- Neighbor duplicate avoidance.
- Procedural variation assignment (rotation, color accent).
- Unit tests for selection distribution, edge cases (empty pool, all filtered out).

**4-B10: Initial Template Definitions**
- Type: content
- Scope: M
- Define 30-47 initial building templates with placeholder values.
- Cover all 6 zone_type+density combinations.
- Assign construction times, costs, capacities, land value thresholds.
- This is a data-authoring task, not code.

### Group D: Building Spawning

**4-B11: Building Spawn Conditions Check**
- Type: feature
- Scope: M
- Implement `can_spawn_building(tile)` function per Section 5.
- Checks: zone designated, demand positive, tile empty, terrain buildable, road accessible, utilities available, tile owned.
- Uses stub interfaces for forward dependencies.
- Unit tests for each condition individually and combined.

**4-B12: Building Entity Creation**
- Type: feature
- Scope: M
- Implement `spawn_building(tile, template)` function.
- Creates entity with: PositionComponent, BuildingComponent, ConstructionComponent, EnergyComponent, FluidComponent, OwnershipComponent.
- Registers in BuildingGrid (including multi-tile footprints).
- Emits no event yet (event emitted on construction completion).
- Unit tests for entity creation, component initialization, grid registration.

**4-B13: Building Spawning Loop**
- Type: feature
- Scope: L
- Implement the per-tick spawning scan in BuildingSystem::tick() per Section 5.
- Player-staggered scanning.
- Max spawns per scan per player.
- Priority ordering (closest to road first, using stub).
- Integration test: zones with positive demand spawn structures over time.

### Group E: Construction System

**4-B14: Construction Progress System**
- Type: feature
- Scope: M
- Each tick, increment ticks_elapsed for all entities with ConstructionComponent.
- Update ConstructionPhase based on progress percentage.
- Handle is_paused flag.
- Unit tests for progress increments, phase transitions, paused behavior.

**4-B15: Construction Completion Handler**
- Type: feature
- Scope: M
- When ticks_elapsed >= ticks_total:
  - Remove ConstructionComponent.
  - Set BuildingComponent.state = Active.
  - Emit BuildingConstructedEvent.
- Unit tests for completion trigger, component removal, event emission.

### Group F: Building State Machine

**4-B16: State Transition System**
- Type: feature
- Scope: L
- Implement all state transitions per Section 3.
- Active -> Abandoned (with grace period).
- Abandoned -> Active (restoration).
- Abandoned -> Derelict (timer expiry).
- Derelict -> Deconstructed (timer expiry).
- Deconstructed -> Entity removal (debris clear).
- Per-tick state evaluation for all Active and Abandoned structures.
- Integration tests for full lifecycle: spawn -> construct -> active -> abandon -> derelict -> deconstructed -> cleared.

**4-B17: Abandon Grace Period Logic**
- Type: feature
- Scope: S
- Track per-structure utility loss duration.
- Only transition Active -> Abandoned after grace period.
- Reset grace counter when utilities restored.
- Unit tests for grace period behavior with intermittent outages.

### Group G: Upgrades and Downgrades

**4-B18: Building Upgrade System**
- Type: feature
- Scope: L
- Implement upgrade conditions check per Section 6.
- Level increase logic.
- Template re-selection for new level.
- Capacity recalculation.
- Upgrade animation via temporary ConstructionComponent.
- Emit BuildingUpgradedEvent.
- Unit tests for upgrade conditions, level cap, capacity scaling.

**4-B19: Building Downgrade System**
- Type: feature
- Scope: M
- Implement downgrade triggers per Section 6.
- Level decrease logic.
- Template re-selection for lower level.
- Capacity recalculation.
- Emit BuildingDowngradedEvent.
- Unit tests for downgrade triggers, minimum level enforcement.

### Group H: Demolition

**4-B20: Player Demolition Handler**
- Type: feature
- Scope: M
- Handle DemolishRequest messages.
- Validate ownership and entity state.
- Calculate demolition cost per Section 7.
- Create debris entity.
- Clear BuildingGrid entries.
- Emit BuildingDeconstructedEvent.
- Unit tests for cost calculation, ownership validation, grid cleanup.

**4-B21: Debris System**
- Type: feature
- Scope: S
- Auto-decrement DebrisComponent.clear_timer each tick.
- On timer expiry: remove entity, emit DebrisClearedEvent.
- Handle player-initiated immediate clear (ClearDebrisMessage).
- Unit tests for auto-clear timing, manual clear.

### Group I: Queries

**4-B22: IBuildingQueryable Interface and Implementation**
- Type: infrastructure
- Scope: L
- Define IBuildingQueryable per Section 9.
- Implement all query methods in BuildingSystem.
- Maintain per-player entity lists and type counters for O(1) aggregate queries.
- Unit tests for all query methods with various building configurations.

### Group J: BuildingSystem Integration

**4-B23: BuildingSystem Class (ISimulatable)**
- Type: feature
- Scope: L
- Main BuildingSystem class implementing ISimulatable (priority: 40).
- Wire together: spawning loop, construction progress, state machine, upgrades, downgrades, demolition, debris.
- Dependency injection for forward dependency providers.
- Integration tests for full system behavior.

**4-B24: Building Network Messages**
- Type: feature
- Scope: M
- Define all client->server and server->client messages per Section 10.
- Implement ISerializable for all message types.
- Integration with SyncSystem (Epic 1) for entity state delta sync.
- Unit tests for serialization round-trips.

**4-B25: BuildingComponent Serialization**
- Type: feature
- Scope: S
- Implement ISerializable for BuildingComponent, ConstructionComponent, DebrisComponent.
- Fixed-size types, little-endian, versioned.
- Round-trip unit tests.

### Task Summary

| Group | Tickets | Description |
|-------|---------|-------------|
| A: Data Infrastructure | 4-B01 to 4-B05 (5) | Components, grid, events |
| B: Forward Dependencies | 4-B06 to 4-B07 (2) | Stub interfaces |
| C: Template System | 4-B08 to 4-B10 (3) | Templates, selection, content |
| D: Building Spawning | 4-B11 to 4-B13 (3) | Spawn logic |
| E: Construction | 4-B14 to 4-B15 (2) | Progress and completion |
| F: State Machine | 4-B16 to 4-B17 (2) | Transitions and grace periods |
| G: Upgrades/Downgrades | 4-B18 to 4-B19 (2) | Level changes |
| H: Demolition | 4-B20 to 4-B21 (2) | Player demolition, debris |
| I: Queries | 4-B22 (1) | Query API |
| J: Integration | 4-B23 to 4-B25 (3) | System wiring, network, serialization |
| **Total** | **25** | |

### Size Distribution

| Size | Count |
|------|-------|
| S | 8 |
| M | 10 |
| L | 7 |

### Critical Path

```
4-B01 (BuildingComponent) ----+
4-B02 (ConstructionComponent) |---> 4-B08 (Template struct) --> 4-B09 (Selection) --> 4-B10 (Content)
4-B03 (DebrisComponent) ------+                                      |
4-B04 (BuildingGrid) -----+                                         |
4-B05 (Events) -----------|---> 4-B11 (Spawn conditions) ------+    |
4-B06 (Interfaces) -------+    4-B12 (Entity creation) --------+--> 4-B13 (Spawn loop)
4-B07 (Stubs) ------------+                                         |
                                                                     v
                                                4-B14 (Construction progress) --> 4-B15 (Completion)
                                                                     |
                                                                     v
                                           4-B16 (State machine) --> 4-B17 (Grace period)
                                                     |
                                     +---------------+---------------+
                                     |               |               |
                                     v               v               v
                               4-B18 (Upgrade)  4-B20 (Demolish) 4-B22 (Queries)
                               4-B19 (Downgrade) 4-B21 (Debris)
                                     |               |               |
                                     +-------+-------+-------+------+
                                             |
                                             v
                                   4-B23 (BuildingSystem integration)
                                   4-B24 (Network messages)
                                   4-B25 (Serialization)
```

Minimum path to "structures appear on screen":
1. 4-B01, 4-B02, 4-B04, 4-B05, 4-B06, 4-B07 (all S-M, parallelizable)
2. 4-B08, 4-B09 (template system)
3. 4-B11, 4-B12, 4-B13 (spawning)
4. 4-B14, 4-B15 (construction)
5. 4-B23 (integration)

---

## 12. Questions for Other Agents

**@zone-engineer:** How does ZoneSystem communicate zone designation to BuildingSystem? Does BuildingSystem query ZoneComponent on tiles directly, or does ZoneSystem emit events when zones are designated? Need to know whether to poll or subscribe.

**@zone-engineer:** When a zone is redesignated (e.g., habitation -> exchange), what happens to existing structures? Does ZoneSystem emit a RedesignationEvent? Should BuildingSystem immediately demolish mismatched structures or let them decay naturally?

**@zone-engineer:** Does ZoneSystem handle density levels per-tile or per-zone-area? Can adjacent tiles within the same zone have different densities? This affects how BuildingSystem scans for eligible tiles.

**@systems-architect:** BuildingGrid follows the same dense-grid-exception pattern as TerrainGrid. Should this be formally added to canon patterns.yaml? The rationale is identical: per-tile occupancy data, every tile potentially occupied, O(1) spatial lookups required.

**@systems-architect:** The forward dependency stub pattern (Section 8) uses raw interface pointers with explicit setter injection. Is this consistent with how other systems handle forward dependencies, or should we use a service locator / registry pattern instead?

**@systems-architect:** ConstructionComponent is a temporary component removed upon completion. Does the ECS framework efficiently handle frequent add/remove of components on existing entities? Or should we consider a flag-based approach within BuildingComponent instead?

**@graphics-engineer:** What rendering data does RenderingSystem need for structure materialization animation? Does it read ConstructionComponent.phase and phase_progress directly, or do we need a separate RenderingHint component?

**@graphics-engineer:** Construction animation is described as "materializing" in canon. What does this look like visually? A fade-in? A bottom-to-top emergence? Particle effects? This affects what data we expose from ConstructionComponent.

**@graphics-engineer:** For structure level upgrades (Section 6), the model changes during a brief animation. Does RenderingSystem need to interpolate between two different 3D models, or is a simple swap with a flash effect sufficient?

**@game-designer:** What are the actual capacity values for each zone type and density? Canon does not specify numerical values. Suggested: low-density habitation dwelling = 4-8 beings, high-density habitation tower = 50-200 beings. Need concrete numbers for template definitions.

**@game-designer:** Should structures have a happiness/harmony bonus or penalty based on their surroundings (e.g., fabrication structures near habitation)? This affects whether BuildingComponent needs additional fields.

**@game-designer:** Upgrade cooldown of 10 seconds (200 ticks) -- is this the right pace? Should structures upgrade faster or slower? The pace affects how quickly a colony develops visually.

**@infrastructure-engineer:** The 3-tile road proximity rule (ITransportConnectable) -- is distance measured Manhattan, Euclidean, or Chebyshev? Does it measure from the nearest edge of a multi-tile footprint or from the anchor tile?

**@population-engineer:** When a structure is abandoned, do occupants leave immediately or gradually? Does BuildingSystem need to notify PopulationSystem before transitioning to Abandoned state so it can begin migration?

**@economy-engineer:** Does demolition cost come from the player's treasury (EconomySystem), or does BuildingSystem handle credits directly? We need to know who validates "can afford to demolish."

---

## 13. Risks & Concerns

### R1: Performance with Many Buildings (HIGH)

**Risk:** On a 512x512 map with 4 players, there could be tens of thousands of structures. Per-tick state evaluation for all Active and Abandoned structures could become expensive.

**Mitigation:**
- Use scan intervals (not every tick) for non-critical evaluations (spawning, upgrades).
- State machine evaluation for Abandoned structures uses a timer, not per-tick condition re-checking.
- Maintain per-state entity lists to avoid iterating all structures when only a subset matters.
- Profile early and establish performance budgets: BuildingSystem tick should complete in <2ms.

### R2: Template Variety (MEDIUM)

**Risk:** With only 30-47 templates, players may notice repetitive structures quickly. Procedural variation (rotation, color accent) may not be sufficient.

**Mitigation:**
- Ensure each template is visually distinct (not just color variants of the same shape).
- Use procedural parameters (floor count, facade style) for additional variation within templates.
- Plan for template expansion in future epics -- the system is designed for easy template addition.
- Consider seasonal/event templates later.

### R3: Forward Dependency Complexity (MEDIUM)

**Risk:** Stubs that always return true create a significantly different gameplay experience than real systems. Structures never abandon during Epic 4 testing. When real systems come online, the abandonment cascade may be surprising and require significant tuning.

**Mitigation:**
- Create a "strict stubs" mode where stubs return false for configurable percentages of queries. This simulates partial utility coverage for testing.
- Write integration tests that exercise the full state machine with both permissive and restrictive stubs.
- Plan explicit integration testing milestones for Epics 5, 6, 7 against BuildingSystem.

### R4: Construction Animation Synchronization (MEDIUM)

**Risk:** Construction progress is synced every 10 ticks (0.5s). If animation interpolation is poor, different clients may show structures at visibly different construction stages. The materializing effect may stutter or jump.

**Mitigation:**
- Client-side interpolation of progress between server updates.
- Animation should be smooth and forgiving -- a materializing effect (fade/emerge) hides small timing discrepancies better than a frame-by-frame animation.
- On construction completion, all clients receive the same event and converge.

### R5: Multi-Tile Footprint Edge Cases (MEDIUM)

**Risk:** Multi-tile structures create complexity: partial overlaps with zone boundaries, tiles at map edges, one tile losing road access while another has it, ownership boundaries within a footprint.

**Mitigation:**
- Strict validation: ALL tiles in footprint must satisfy ALL conditions. No partial placement.
- Multi-tile structures are limited to high-density templates and capped at 2x2 initially.
- BuildingGrid handles multi-tile registration atomically.

### R6: Spawn Rate Tuning (LOW)

**Risk:** Spawn rate too fast = city fills instantly (no sense of growth). Too slow = players wait around watching empty zones. Getting the pacing right is crucial for game feel.

**Mitigation:**
- All spawn rate parameters are configurable constants, not hardcoded.
- Spawn rate scales with demand (higher demand = faster spawning).
- Implement a debug UI to adjust spawn rate in real-time during development.
- Expect this to require significant playtesting and tuning.

### R7: Ghost Town Integration (LOW)

**Risk:** The ghost town abandonment process (canon Section: multiplayer.ownership.ghost_town_process) triggers mass building state transitions when a player leaves. If a player with 1,000 structures abandons, all 1,000 simultaneously enter the Abandoned pipeline, potentially causing a spike in processing.

**Mitigation:**
- Stagger ghost town transitions: not all structures transition in the same tick. Spread over multiple scan cycles.
- The AbandonmentSystem (SimulationCore) should signal BuildingSystem gradually, not all-at-once.

### R8: Serialization Versioning (LOW)

**Risk:** BuildingComponent struct may evolve as we add features (Epic 9 services, Epic 13 damage). Network serialization format must be forward/backward compatible.

**Mitigation:**
- Version field in serialization format.
- New fields get default values when deserializing old formats.
- Plan field layout with padding/reserved bytes for anticipated additions (health for damage, service flags).

---

## File Locations (Proposed)

```
src/
  components/
    BuildingComponent.h          # BuildingComponent, ConstructionComponent, DebrisComponent
    BuildingTypes.h              # Enums: BuildingState, ZoneBuildingType, DensityLevel, ConstructionPhase
  systems/
    buildings/
      BuildingSystem.h           # BuildingSystem class declaration
      BuildingSystem.cpp         # Main tick logic, spawning, state machine
      BuildingGrid.h             # Dense occupancy grid
      BuildingGrid.cpp
      BuildingTemplateRegistry.h # Template loading and selection
      BuildingTemplateRegistry.cpp
      DemolitionHandler.h        # Demolition logic
      DemolitionHandler.cpp
    interfaces/
      IUtilityProvider.h         # IEnergyProvider, IFluidProvider, ITransportProvider
      IDemandProvider.h          # IDemandProvider
      ILandValueProvider.h       # ILandValueProvider
      IBuildingQueryable.h       # Building query interface
      StubProviders.h            # All stub implementations
      StubProviders.cpp
  events/
    BuildingEvents.h             # All building-related events
  network/
    messages/
      BuildingMessages.h         # Network message definitions
      BuildingMessages.cpp       # Serialization
```

This layout follows the canon file organization pattern: components in `src/components/`, systems in `src/systems/`, and the building-engineer-specific files grouped under `src/systems/buildings/`.

---

*End of Building Engineer Analysis*
