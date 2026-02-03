# Economy Engineer Analysis: Epic 10 - Simulation Core

**Agent:** Economy Engineer
**Epic:** 10 - Simulation Core (Value & Impact Systems)
**Canon Version:** 0.9.0
**Date:** 2026-01-29

---

## Executive Summary

Epic 10 introduces the value simulation and impact systems that form the economic backbone of colony development. This analysis covers DisorderSystem, ContaminationSystem, and LandValueSystem - three interconnected systems that create feedback loops affecting habitability, zone demand, and overall colony prosperity. These systems use dense 2D grids (per the `dense_grid_exception` pattern) for efficient spatial simulation, and employ a **previous-tick dependency resolution** strategy to break circular dependencies between disorder, contamination, and land value.

---

## Table of Contents

1. [Value & Impact Overview](#1-value--impact-overview)
2. [Component Designs](#2-component-designs)
3. [Dense Grid Designs](#3-dense-grid-designs)
4. [Sector Value Calculation](#4-sector-value-calculation)
5. [Disorder System](#5-disorder-system)
6. [Contamination System](#6-contamination-system)
7. [Circular Dependency Resolution](#7-circular-dependency-resolution)
8. [Key Work Items](#8-key-work-items)
9. [Questions for Other Agents](#9-questions-for-other-agents)
10. [Performance Considerations](#10-performance-considerations)
11. [Economic Impact](#11-economic-impact)

---

## 1. Value & Impact Overview

### The Value-Impact Triangle

Disorder, contamination, and sector value form a tightly coupled triangle of influence:

```
                    +----------------+
                    |  Sector Value  |
                    | (LandValueSys) |
                    +-------+--------+
                           /|\
                          / | \
          Affects        /  |  \        Affects
          Disorder      /   |   \      Contamination
                       /    |    \
                      v     |     v
        +-------------+     |     +----------------+
        |  Disorder   |<----+---->| Contamination  |
        | (DisorderSys)           | (ContaminSys)  |
        +-------------+           +----------------+
                \                       /
                 \                     /
                  \   Affects Each   /
                   \    Other       /
                    v              v
              Both reduce Sector Value
              Both affect Population
```

### System Interaction Summary

| System | Reads From | Writes To | Tick Priority |
|--------|-----------|-----------|---------------|
| DisorderSystem | BuildingSystem, PopulationSystem, **LandValueSystem (prev tick)** | DisorderGrid, stats | 70 |
| ContaminationSystem | BuildingSystem, TransportSystem, EnergySystem, TerrainSystem | ContaminationGrid, stats | 80 |
| LandValueSystem | TerrainSystem, **DisorderSystem (prev tick)**, **ContaminationSystem (prev tick)**, TransportSystem | LandValueGrid, stats | (no explicit priority) |

### Why Dense Grids?

Per `patterns.yaml` `dense_grid_exception`, contamination and land value are candidates for dense grid storage because:

1. **Every cell needs a value**: Unlike buildings (sparse), every tile has a disorder rate, contamination level, and sector value
2. **Spatial locality is critical**: Spread algorithms traverse neighbors frequently
3. **Per-entity overhead is prohibitive**: 262,144 tiles on a 512x512 map

---

## 2. Component Designs

### 2.1 DisorderComponent (Optional Entity Attachment)

Attached to buildings that generate or suppress disorder. Most disorder state lives in the dense grid, but buildings need individual tracking for service effectiveness.

```cpp
struct DisorderComponent {
    // Generation
    uint16_t base_disorder_generation = 0;   // Disorder units per tick this building adds
    uint16_t current_disorder_generation = 0; // After service suppression

    // Suppression (for enforcer posts)
    uint16_t suppression_power = 0;          // How much disorder this building suppresses
    uint8_t  suppression_radius = 0;         // Radius of suppression effect

    // State
    uint8_t  local_disorder_level = 0;       // Disorder at this building's tile (0-255)

    // Flags
    bool     is_disorder_source = false;     // True if generates disorder
    bool     is_enforcer = false;            // True if suppresses disorder

    uint8_t  padding[2];
};
static_assert(sizeof(DisorderComponent) == 12, "DisorderComponent must be 12 bytes");
```

**Design Rationale:**

- **12 bytes**: Compact for buildings with disorder interaction (many won't have this component)
- **base vs current**: Allows showing service effectiveness in UI
- **suppression_power/radius**: Enforcer posts (from Epic 9) use this to reduce disorder
- **local_disorder_level**: Cached from grid for fast per-entity queries

### 2.2 ContaminationComponent (Optional Entity Attachment)

Attached to buildings that generate contamination. Implements `IContaminationSource`.

```cpp
enum class ContaminationType : uint8_t {
    Industrial = 0,    // From fabrication zones
    Traffic    = 1,    // From congested pathways (via TransportSystem)
    Energy     = 2,    // From polluting energy nexuses
    Terrain    = 3     // From blight_mires (toxic_marshes)
};

struct ContaminationComponent {
    // Generation
    uint32_t base_contamination_output = 0;     // Contamination units per tick
    uint32_t current_contamination_output = 0;  // After any modifiers

    // Spread characteristics
    uint8_t  spread_radius = 0;                 // How far contamination spreads
    uint8_t  spread_decay_rate = 0;             // How quickly it decays with distance

    // Type
    ContaminationType contamination_type = ContaminationType::Industrial;

    // State
    uint8_t  local_contamination_level = 0;     // Contamination at this tile (0-255)

    // Flags
    bool     is_active_source = false;          // True if currently polluting

    uint8_t  padding[3];
};
static_assert(sizeof(ContaminationComponent) == 16, "ContaminationComponent must be 16 bytes");
```

**Design Rationale:**

- **16 bytes**: Standard cache-friendly size
- **Type enum**: Different contamination types may have different effects in future
- **spread_radius/decay_rate**: Per-source spread characteristics (heavy industry spreads farther)
- **is_active_source**: Inactive buildings (unpowered, abandoned) don't pollute

### 2.3 No LandValueComponent

Land value is purely derived from environmental factors and stored only in the dense grid. Buildings don't need individual land value components - they query the grid when needed.

**Query Pattern:**

```cpp
uint8_t LandValueSystem::get_sector_value_at(int32_t x, int32_t y) const {
    return land_value_grid_.get_value(x, y);
}
```

### 2.4 Component Memory Budget

| Component | Size | Typical Count (256x256) | Memory |
|-----------|------|-------------------------|--------|
| DisorderComponent | 12 bytes | ~5,000 (buildings with disorder) | 60 KB |
| ContaminationComponent | 16 bytes | ~2,000 (polluting buildings) | 32 KB |
| DisorderGrid (dense) | 1 byte/cell | 65,536 cells | 64 KB |
| ContaminationGrid (dense) | 2 bytes/cell | 65,536 cells | 128 KB |
| LandValueGrid (dense) | 2 bytes/cell | 65,536 cells | 128 KB |
| **Total** | | | **~412 KB** |

---

## 3. Dense Grid Designs

### 3.1 ContaminationGrid

Stores contamination level per tile with type information.

```cpp
struct ContaminationCell {
    uint8_t level;              // 0-255 contamination intensity
    uint8_t dominant_type;      // ContaminationType (for overlay coloring)
};

class ContaminationGrid {
private:
    std::vector<ContaminationCell> grid_;  // Dense 2D array
    uint32_t width_;
    uint32_t height_;

    // Previous tick snapshot (for dependency resolution)
    std::vector<ContaminationCell> previous_grid_;

    // Dirty tracking
    std::vector<bool> dirty_cells_;        // Cells modified this tick
    bool any_dirty_ = false;

public:
    // Queries (read from previous tick for dependent systems)
    uint8_t get_level(int32_t x, int32_t y) const;
    uint8_t get_level_previous_tick(int32_t x, int32_t y) const;
    ContaminationType get_dominant_type(int32_t x, int32_t y) const;

    // Modifications (write to current tick)
    void add_contamination(int32_t x, int32_t y, uint8_t amount, ContaminationType type);
    void apply_decay(int32_t x, int32_t y, uint8_t amount);

    // Tick boundary
    void swap_buffers();  // Called at start of tick

    // Aggregate queries
    uint32_t get_total_contamination() const;
    uint32_t get_contamination_in_region(GridRect region) const;

    // Overlay data
    void generate_overlay_data(std::vector<ContaminationOverlayCell>& out) const;
};
```

**Memory:** 2 bytes per cell = 128 KB for 256x256, 512 KB for 512x512

**Double-Buffering:** Previous tick data is preserved for LandValueSystem to read while ContaminationSystem writes current tick data.

### 3.2 DisorderGrid

Stores disorder level per tile.

```cpp
struct DisorderCell {
    uint8_t level;              // 0-255 disorder intensity
};

class DisorderGrid {
private:
    std::vector<DisorderCell> grid_;
    uint32_t width_;
    uint32_t height_;

    // Previous tick snapshot
    std::vector<DisorderCell> previous_grid_;

    // Cached aggregate stats
    uint32_t total_disorder_ = 0;
    uint32_t high_disorder_tiles_ = 0;  // Tiles with level > 128

public:
    // Queries
    uint8_t get_level(int32_t x, int32_t y) const;
    uint8_t get_level_previous_tick(int32_t x, int32_t y) const;

    // Modifications
    void add_disorder(int32_t x, int32_t y, uint8_t amount);
    void apply_suppression(int32_t x, int32_t y, uint8_t amount);
    void apply_spread(int32_t x, int32_t y);

    // Tick boundary
    void swap_buffers();

    // Stats
    uint32_t get_total_disorder() const { return total_disorder_; }
    uint32_t get_high_disorder_tile_count() const { return high_disorder_tiles_; }
    float get_average_disorder() const;

    // Overlay
    void generate_overlay_data(std::vector<DisorderOverlayCell>& out) const;
};
```

**Memory:** 1 byte per cell = 64 KB for 256x256, 256 KB for 512x512

### 3.3 LandValueGrid

Stores sector value per tile with factor breakdown.

```cpp
struct LandValueCell {
    uint8_t total_value;        // 0-255 final sector value
    uint8_t terrain_bonus;      // Contribution from terrain (water, crystals)
};

class LandValueGrid {
private:
    std::vector<LandValueCell> grid_;
    uint32_t width_;
    uint32_t height_;

    // No previous tick needed - we READ from disorder/contamination previous tick

    // Dirty tracking
    bool needs_full_recalc_ = true;

public:
    // Queries
    uint8_t get_value(int32_t x, int32_t y) const;
    uint8_t get_terrain_bonus(int32_t x, int32_t y) const;

    // Full recalculation (called each tick)
    void recalculate(
        const DisorderGrid& disorder_prev,
        const ContaminationGrid& contamination_prev,
        const ITerrainQueryable& terrain,
        const ITransportProvider& transport
    );

    // Stats
    uint8_t get_average_value() const;
    uint32_t get_high_value_tile_count() const;  // Tiles > 200

    // Overlay
    void generate_overlay_data(std::vector<LandValueOverlayCell>& out) const;
};
```

**Memory:** 2 bytes per cell = 128 KB for 256x256, 512 KB for 512x512

---

## 4. Sector Value Calculation

### 4.1 Value Formula

Sector value is calculated from multiple factors, each contributing a bonus or penalty:

```cpp
uint8_t calculate_sector_value(
    int32_t x, int32_t y,
    const DisorderGrid& disorder_prev,
    const ContaminationGrid& contamination_prev,
    const ITerrainQueryable& terrain,
    const ITransportProvider& transport
) {
    // Start at neutral (128)
    int32_t value = 128;

    // === POSITIVE FACTORS ===

    // Water proximity bonus (+0 to +30)
    uint8_t water_dist = terrain.get_water_distance(x, y);
    if (water_dist <= 3) {
        value += 30 - (water_dist * 10);  // Adjacent=+30, 1 away=+20, 2 away=+10, 3 away=+0
    }

    // Crystal fields bonus (+0 to +25)
    if (terrain.get_terrain_type(x, y) == TerrainType::CrystalFields) {
        value += 25;
    }

    // Spore plains bonus (+0 to +15, affects harmony not direct value)
    if (terrain.get_terrain_type(x, y) == TerrainType::SporePlains) {
        value += 15;
    }

    // Biolume grove bonus (+0 to +10)
    if (terrain.get_terrain_type(x, y) == TerrainType::Forest) {
        value += 10;
    }

    // Road accessibility bonus (+0 to +20)
    uint8_t road_dist = transport.get_nearest_road_distance(x, y);
    if (road_dist == 0) {
        value += 20;  // On road
    } else if (road_dist <= 3) {
        value += 20 - (road_dist * 5);  // Decreasing bonus
    }

    // === NEGATIVE FACTORS ===

    // Disorder penalty (-0 to -40)
    // Read from PREVIOUS tick to break circular dependency
    uint8_t disorder = disorder_prev.get_level_previous_tick(x, y);
    value -= (disorder * 40) / 255;  // Scale to -0 to -40

    // Contamination penalty (-0 to -50)
    // Read from PREVIOUS tick to break circular dependency
    uint8_t contamination = contamination_prev.get_level_previous_tick(x, y);
    value -= (contamination * 50) / 255;  // Scale to -0 to -50

    // Toxic marshes penalty (-30)
    if (terrain.get_terrain_type(x, y) == TerrainType::ToxicMarshes) {
        value -= 30;
    }

    // === CLAMP AND RETURN ===
    return static_cast<uint8_t>(std::clamp(value, 0, 255));
}
```

### 4.2 Value Factor Summary

| Factor | Type | Range | Notes |
|--------|------|-------|-------|
| Base Value | Neutral | 128 | Starting point |
| Water Proximity | Bonus | +0 to +30 | Within 3 tiles |
| Crystal Fields | Bonus | +25 | On crystal terrain |
| Spore Plains | Bonus | +15 | On spore terrain |
| Biolume Grove | Bonus | +10 | On forest terrain |
| Road Access | Bonus | +0 to +20 | Within 3 tiles |
| Disorder | Penalty | -0 to -40 | From previous tick |
| Contamination | Penalty | -0 to -50 | From previous tick |
| Toxic Marshes | Penalty | -30 | On toxic terrain |
| **Total Range** | | **0-255** | Clamped |

### 4.3 Value Update Frequency

- **Full recalculation**: Every tick (20 Hz)
- **Why every tick?**: Disorder and contamination change frequently; stale land values cause building spawn issues

**Optimization:** Only recalculate tiles where underlying factors changed. Use dirty region tracking from terrain modifications, disorder/contamination spread.

---

## 5. Disorder System

### 5.1 Disorder Generation

Disorder is generated by buildings based on their type and population density:

```cpp
struct DisorderGenerationConfig {
    ZoneBuildingType zone_type;
    uint8_t base_generation;        // Base disorder per building
    float population_multiplier;    // Multiplier based on occupancy
    float land_value_modifier;      // Lower land value = more disorder
};

const DisorderGenerationConfig DISORDER_CONFIGS[] = {
    // Low-density habitation: low base, moderate population factor
    { ZoneBuildingType::LowDensityHabitation, 2, 0.5f, 0.3f },

    // High-density habitation: higher base due to crowding
    { ZoneBuildingType::HighDensityHabitation, 5, 0.8f, 0.4f },

    // Exchange: moderate (petty crime from commerce)
    { ZoneBuildingType::LowDensityExchange, 3, 0.3f, 0.2f },
    { ZoneBuildingType::HighDensityExchange, 6, 0.5f, 0.3f },

    // Fabrication: lower (workers, not residents)
    { ZoneBuildingType::LowDensityFabrication, 1, 0.2f, 0.1f },
    { ZoneBuildingType::HighDensityFabrication, 2, 0.3f, 0.2f },
};
```

**Generation Formula:**

```cpp
uint16_t calculate_disorder_generation(
    EntityID building,
    const BuildingComponent& bldg,
    uint8_t local_land_value_prev  // From previous tick
) {
    const DisorderGenerationConfig& config = get_config(bldg.zone_type);

    float base = config.base_generation;

    // Scale by occupancy
    float occupancy_ratio = static_cast<float>(bldg.current_occupancy) / bldg.capacity;
    base += base * config.population_multiplier * occupancy_ratio;

    // Scale by land value (lower value = more disorder)
    // land_value 0 -> +100% disorder, land_value 255 -> +0%
    float value_factor = 1.0f + config.land_value_modifier * (255 - local_land_value_prev) / 255.0f;
    base *= value_factor;

    return static_cast<uint16_t>(base);
}
```

### 5.2 Disorder Spread Algorithm

Disorder spreads to adjacent tiles over time:

```cpp
void DisorderSystem::apply_spread() {
    // Create temporary buffer for spread calculations
    std::vector<int32_t> spread_delta(width_ * height_, 0);

    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            uint8_t level = disorder_grid_.get_level(x, y);

            if (level < SPREAD_THRESHOLD) continue;  // Only spread if > threshold (e.g., 64)

            // Spread to 4-neighbors
            static const int dx[] = {0, 0, 1, -1};
            static const int dy[] = {1, -1, 0, 0};

            uint8_t spread_amount = (level - SPREAD_THRESHOLD) / 8;  // 0-24 spread

            for (int i = 0; i < 4; ++i) {
                int32_t nx = x + dx[i];
                int32_t ny = y + dy[i];

                if (!in_bounds(nx, ny)) continue;

                // Spread reduced by distance and terrain
                uint8_t actual_spread = spread_amount;

                // Water blocks spread
                if (terrain_->get_terrain_type(nx, ny) == TerrainType::Water) {
                    actual_spread = 0;
                }

                spread_delta[ny * width_ + nx] += actual_spread;
            }

            // Source loses some disorder (diffusion)
            spread_delta[y * width_ + x] -= spread_amount * 2;  // Lose more than neighbors gain
        }
    }

    // Apply deltas
    for (int32_t i = 0; i < width_ * height_; ++i) {
        int32_t new_level = static_cast<int32_t>(disorder_grid_.get_level_at_index(i)) + spread_delta[i];
        disorder_grid_.set_level_at_index(i, static_cast<uint8_t>(std::clamp(new_level, 0, 255)));
    }
}
```

### 5.3 Enforcer Effectiveness

Enforcer posts (from Epic 9 ServicesSystem) suppress disorder in their radius:

```cpp
void DisorderSystem::apply_enforcer_suppression() {
    // Query all enforcer posts
    for (auto& [entity, disorder, pos, ownership] :
         registry_.view<DisorderComponent, PositionComponent, OwnershipComponent>().each()) {

        if (!disorder.is_enforcer) continue;
        if (disorder.suppression_power == 0) continue;

        // Apply suppression in radius
        for (int32_t dy = -disorder.suppression_radius; dy <= disorder.suppression_radius; ++dy) {
            for (int32_t dx = -disorder.suppression_radius; dx <= disorder.suppression_radius; ++dx) {
                int32_t x = pos.grid_x + dx;
                int32_t y = pos.grid_y + dy;

                if (!in_bounds(x, y)) continue;

                // Distance falloff
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > disorder.suppression_radius) continue;

                float falloff = 1.0f - (dist / disorder.suppression_radius);
                uint8_t suppression = static_cast<uint8_t>(disorder.suppression_power * falloff);

                disorder_grid_.apply_suppression(x, y, suppression);
            }
        }
    }
}
```

**Suppression Formula:**

```cpp
void DisorderGrid::apply_suppression(int32_t x, int32_t y, uint8_t amount) {
    uint8_t current = get_level(x, y);
    uint8_t new_level = (current > amount) ? (current - amount) : 0;
    set_level(x, y, new_level);
}
```

### 5.4 Disorder Effects

| Disorder Level | Description | Effects |
|---------------|-------------|---------|
| 0-50 | Low | No negative effects |
| 51-100 | Moderate | -5 to -15 sector value penalty |
| 101-150 | High | -15 to -30 sector value, migration penalty |
| 151-200 | Severe | -30 to -40 sector value, abandonment risk |
| 201-255 | Crisis | Buildings may become abandoned, civil unrest events |

---

## 6. Contamination System

### 6.1 Contamination Sources

Per `interfaces.yaml` `IContaminationSource`:

| Source Type | contamination_type | Typical Output | Notes |
|-------------|-------------------|----------------|-------|
| Fabrication buildings | Industrial | 50-200/tick | Scales with level |
| Carbon nexus | Energy | 200/tick | High pollution |
| Petrochemical nexus | Energy | 120/tick | Medium pollution |
| Gaseous nexus | Energy | 40/tick | Lower pollution |
| High-traffic pathways | Traffic | 5-50/tick | Based on congestion |
| Toxic marshes terrain | Terrain | 30/tick | Constant |

### 6.2 Contamination Generation

```cpp
void ContaminationSystem::generate_contamination() {
    // Clear generation layer (keep existing spread)
    // contamination_grid_.clear_generation_layer();

    // === Industrial Contamination ===
    for (auto& [entity, contam, pos] :
         registry_.view<ContaminationComponent, PositionComponent>().each()) {

        if (!contam.is_active_source) continue;

        // Add contamination at source tile
        contamination_grid_.add_contamination(
            pos.grid_x, pos.grid_y,
            static_cast<uint8_t>(std::min(255u, contam.current_contamination_output)),
            contam.contamination_type
        );
    }

    // === Traffic Contamination ===
    // Query TransportSystem for high-congestion pathways
    auto traffic_sources = transport_provider_->get_traffic_contamination_sources();
    for (const auto& source : traffic_sources) {
        contamination_grid_.add_contamination(
            source.position.x, source.position.y,
            source.contamination_output,
            ContaminationType::Traffic
        );
    }

    // === Terrain Contamination ===
    // Toxic marshes generate constant contamination
    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            if (terrain_->get_terrain_type(x, y) == TerrainType::ToxicMarshes) {
                contamination_grid_.add_contamination(x, y, 30, ContaminationType::Terrain);
            }
        }
    }
}
```

### 6.3 Contamination Spread Algorithm

Contamination spreads outward from sources and decays over distance:

```cpp
void ContaminationSystem::apply_spread() {
    // Spread is applied per-source during generation
    // This function handles the "ambient spread" from existing contamination

    std::vector<int16_t> spread_delta(width_ * height_, 0);

    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            uint8_t level = contamination_grid_.get_level(x, y);

            if (level < SPREAD_THRESHOLD) continue;  // e.g., 32

            // Spread to 8-neighbors (contamination spreads diagonally too)
            for (int32_t dy = -1; dy <= 1; ++dy) {
                for (int32_t dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;

                    int32_t nx = x + dx;
                    int32_t ny = y + dy;

                    if (!in_bounds(nx, ny)) continue;

                    // Diagonal spread is weaker
                    bool diagonal = (dx != 0 && dy != 0);
                    uint8_t spread = (level - SPREAD_THRESHOLD) / (diagonal ? 16 : 8);

                    // Wind direction could modify spread (future)

                    spread_delta[ny * width_ + nx] += spread;
                }
            }
        }
    }

    // Apply deltas (capped at 255)
    for (int32_t i = 0; i < width_ * height_; ++i) {
        int32_t current = contamination_grid_.get_level_at_index(i);
        int32_t new_level = current + spread_delta[i];
        contamination_grid_.set_level_at_index(i, static_cast<uint8_t>(std::min(255, new_level)));
    }
}
```

### 6.4 Contamination Decay

Contamination decays naturally over time (environmental cleanup):

```cpp
void ContaminationSystem::apply_decay() {
    constexpr uint8_t BASE_DECAY_RATE = 2;  // Units per tick
    constexpr uint8_t TERRAIN_DECAY_BONUS = 3;  // Extra decay near water/vegetation

    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            uint8_t level = contamination_grid_.get_level(x, y);
            if (level == 0) continue;

            uint8_t decay = BASE_DECAY_RATE;

            // Water proximity accelerates decay (natural filtration)
            if (terrain_->get_water_distance(x, y) <= 2) {
                decay += TERRAIN_DECAY_BONUS;
            }

            // Vegetation accelerates decay (bioremediation)
            TerrainType type = terrain_->get_terrain_type(x, y);
            if (type == TerrainType::Forest || type == TerrainType::SporePlains) {
                decay += TERRAIN_DECAY_BONUS;
            }

            // Apply decay
            uint8_t new_level = (level > decay) ? (level - decay) : 0;
            contamination_grid_.set_level(x, y, new_level);
        }
    }
}
```

### 6.5 Contamination Effects

| Contamination Level | Description | Effects |
|--------------------|-------------|---------|
| 0-30 | Clean | No negative effects |
| 31-80 | Light | Minor sector value penalty (-5 to -15) |
| 81-150 | Moderate | Significant penalty (-15 to -35), health impact |
| 151-200 | Heavy | Severe penalty (-35 to -45), longevity reduction |
| 201-255 | Toxic | Maximum penalty (-50), high health impact, zone abandonment |

---

## 7. Circular Dependency Resolution

### 7.1 The Problem

These three systems have circular dependencies:

```
LandValueSystem depends on DisorderSystem (disorder penalty)
LandValueSystem depends on ContaminationSystem (contamination penalty)
DisorderSystem depends on LandValueSystem (low value increases disorder)
```

If all systems read from each other's current tick data, we get either:
1. **Stale data**: Order-dependent results where first system sees old data
2. **Infinite loop**: Systems continuously recalculating based on each other

### 7.2 The Solution: Previous Tick Data

**Canon states:** "Uses PREVIOUS tick's disorder/contamination to avoid circular dependency"

Each grid maintains a **double buffer**:
- `current_grid_`: Data being written this tick
- `previous_grid_`: Data from last tick, read by dependent systems

**Buffer Swap:** At the start of each system's tick, swap buffers:

```cpp
void ContaminationSystem::tick(float delta_time) {
    // Swap buffers - previous becomes current, current becomes write target
    contamination_grid_.swap_buffers();

    // Now:
    // - get_level_previous_tick() returns last tick's finalized data
    // - get_level() / set_level() work on this tick's data

    generate_contamination();
    apply_spread();
    apply_decay();
    update_stats();
}
```

### 7.3 Tick Order with Dependency Resolution

```
Tick N:
  [Previous systems: Energy, Fluid, Zone, Building, Transport, Population, Economy]

  70: DisorderSystem
      - Reads: BuildingSystem (current), PopulationSystem (current)
      - Reads: LandValueGrid.previous_tick (from tick N-1)
      - Writes: DisorderGrid.current_tick
      - swap_buffers() at start

  80: ContaminationSystem
      - Reads: BuildingSystem (current), TransportSystem (current), EnergySystem (current)
      - Writes: ContaminationGrid.current_tick
      - swap_buffers() at start

  ??: LandValueSystem (after Disorder and Contamination)
      - Reads: TerrainSystem (current)
      - Reads: DisorderGrid.previous_tick (from tick N)
      - Reads: ContaminationGrid.previous_tick (from tick N)
      - Reads: TransportSystem (current)
      - Writes: LandValueGrid.current_tick
      - swap_buffers() at start

Tick N+1:
  - DisorderSystem reads LandValueGrid.previous_tick (N's data)
  - LandValueSystem reads DisorderGrid.previous_tick (N's data)
  - One tick delay in feedback loop - stable and deterministic
```

### 7.4 Stability Analysis

With one-tick delay:
- Changes in disorder at tick N affect land value at tick N+1
- Land value changes at tick N+1 affect disorder at tick N+2
- This prevents oscillation and ensures deterministic simulation

**Convergence:** The delay acts as a dampener. Rapid changes smooth out over ~5-10 ticks.

---

## 8. Key Work Items

### Infrastructure Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-001 | ContaminationType enum | S | P0 | None |
| V-002 | DisorderComponent struct (12 bytes) | S | P0 | None |
| V-003 | ContaminationComponent struct (16 bytes) | S | P0 | V-001 |
| V-004 | Value/Impact event definitions | S | P0 | None |

### Dense Grid Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-005 | ContaminationGrid class with double-buffering | L | P0 | V-001 |
| V-006 | DisorderGrid class with double-buffering | L | P0 | None |
| V-007 | LandValueGrid class | M | P0 | None |
| V-008 | Grid buffer swap mechanism | M | P0 | V-005, V-006 |

### Disorder System Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-009 | DisorderSystem class skeleton (ISimulatable, priority 70) | M | P0 | V-006 |
| V-010 | Disorder generation from buildings | M | P0 | V-009 |
| V-011 | Disorder spread algorithm | L | P0 | V-006, V-010 |
| V-012 | Enforcer suppression integration (ServicesSystem hook) | M | P1 | V-009 |
| V-013 | Land value feedback (reads previous tick) | M | P0 | V-007, V-008 |
| V-014 | Disorder overlay data generation | S | P1 | V-006 |
| V-015 | Disorder stats calculation | S | P1 | V-006 |

### Contamination System Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-016 | ContaminationSystem class skeleton (ISimulatable, priority 80) | M | P0 | V-005 |
| V-017 | Industrial contamination generation (BuildingSystem query) | M | P0 | V-016 |
| V-018 | Energy contamination generation (EnergySystem query) | M | P0 | V-016 |
| V-019 | Traffic contamination generation (TransportSystem query) | M | P0 | V-016 |
| V-020 | Terrain contamination (toxic_marshes) | S | P0 | V-016 |
| V-021 | Contamination spread algorithm | L | P0 | V-005, V-017 |
| V-022 | Contamination decay algorithm | M | P0 | V-005, V-017 |
| V-023 | Contamination overlay data generation | S | P1 | V-005 |
| V-024 | Contamination stats calculation | S | P1 | V-005 |

### Land Value System Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-025 | LandValueSystem class skeleton (ISimulatable) | M | P0 | V-007 |
| V-026 | Terrain bonuses (water, crystals, forest, etc.) | M | P0 | V-025 |
| V-027 | Road accessibility bonus (TransportSystem query) | M | P0 | V-025 |
| V-028 | Disorder penalty (previous tick read) | M | P0 | V-025, V-013 |
| V-029 | Contamination penalty (previous tick read) | M | P0 | V-025, V-008 |
| V-030 | Full value recalculation per tick | L | P0 | V-026, V-027, V-028, V-029 |
| V-031 | Value overlay data generation | S | P1 | V-007 |
| V-032 | Value stats calculation | S | P1 | V-007 |

### IContaminationSource Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-033 | IContaminationSource query API | S | P0 | None |
| V-034 | Fabrication building contamination output | M | P0 | V-033 |
| V-035 | Energy nexus contamination output (Epic 5 integration) | S | P0 | V-033 |
| V-036 | Traffic contamination output (Epic 7 integration) | S | P0 | V-033 |

### Network Sync Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-037 | Grid delta sync (changed cells only) | L | P1 | V-005, V-006, V-007 |
| V-038 | Stats sync messages | S | P1 | V-015, V-024, V-032 |
| V-039 | Overlay data for client reconstruction | M | P2 | V-014, V-023, V-031 |

### Content Tasks

| ID | Description | Scope | Priority | Dependencies |
|----|-------------|-------|----------|--------------|
| V-040 | Define disorder generation per building type | M | P0 | @game-designer |
| V-041 | Define contamination output per source type | M | P0 | @game-designer |
| V-042 | Define value factor weights | M | P0 | @game-designer |
| V-043 | Balance decay rates | S | P1 | @game-designer |

### Work Item Summary

| Category | Count | S | M | L |
|----------|-------|---|---|---|
| Infrastructure | 4 | 4 | 0 | 0 |
| Dense Grids | 4 | 0 | 2 | 2 |
| Disorder System | 7 | 2 | 4 | 1 |
| Contamination System | 9 | 3 | 4 | 2 |
| Land Value System | 8 | 2 | 5 | 1 |
| IContaminationSource | 4 | 3 | 1 | 0 |
| Network Sync | 3 | 1 | 1 | 1 |
| Content | 4 | 1 | 3 | 0 |
| **Total** | **43** | **16** | **20** | **7** |

---

## 9. Questions for Other Agents

### @systems-architect

1. **LandValueSystem Tick Priority:** DisorderSystem is 70, ContaminationSystem is 80. Where should LandValueSystem run? I suggest **85** (after both, so it reads their finalized previous-tick data).

2. **Double-Buffer Synchronization:** Should the buffer swap happen at system tick start, or should SimulationCore orchestrate a global swap before all systems tick?

3. **Dense Grid Pattern Canon Update:** Should ContaminationGrid, DisorderGrid, and LandValueGrid be formally added to `patterns.yaml` `dense_grid_exception.applies_to` list?

4. **IContaminationSource Ownership:** Canon says ContaminationSystem queries sources via interface. Should the interface be owned by ContaminationSystem, or should it be a shared interface that EnergySystem, TransportSystem, and BuildingSystem implement?

### @game-designer

5. **Disorder Generation Values:** Proposed base values:
   - Low-density habitation: 2/tick
   - High-density habitation: 5/tick
   - Exchange: 3-6/tick
   - Fabrication: 1-2/tick
   Are these reasonable starting points?

6. **Land Value Factor Weights:** Proposed weights:
   - Water proximity: +30 max
   - Crystal fields: +25
   - Disorder penalty: -40 max
   - Contamination penalty: -50 max
   Is contamination correctly weighted as worse than disorder?

7. **Decay Rates:** Proposed base decay:
   - Disorder: Natural decay of 1/tick (halves in ~128 ticks / 6.4 seconds)
   - Contamination: Natural decay of 2/tick (halves in ~64 ticks / 3.2 seconds)
   Is contamination correctly decaying faster than disorder?

8. **Enforcer Effectiveness:** What suppression power should a standard enforcer post have? Proposed: 100 power with radius 8 tiles.

9. **Contamination Health Effects:** How should contamination affect population longevity? Linear penalty to life expectancy, or threshold-based (only above level 150)?

### @population-engineer

10. **Disorder Migration Impact:** How should disorder affect migration? Proposed: High disorder (>150) reduces immigration by 50%, increases emigration by 25%.

11. **Contamination Health Impact:** How should contamination affect longevity stats? Proposed: Each point of contamination above 80 reduces average life expectancy by 0.1 years.

### @services-engineer

12. **Enforcer Post Integration:** DisorderSystem needs to query enforcer posts from ServicesSystem. What's the preferred query pattern? Should DisorderSystem iterate all ServiceProviderComponents, or should ServicesSystem provide a filtered list?

13. **Hazmat Service (Future):** Should there be a "hazmat response" service that accelerates contamination decay? Not in MVP scope, but affects interface design.

---

## 10. Performance Considerations

### 10.1 Grid Iteration Performance

| Operation | Grid Size | Cell Count | Target Time |
|-----------|-----------|------------|-------------|
| Full grid iteration | 256x256 | 65,536 | <1ms |
| Full grid iteration | 512x512 | 262,144 | <5ms |
| Spread calculation | 256x256 | ~10,000 active | <2ms |
| Spread calculation | 512x512 | ~40,000 active | <10ms |

### 10.2 Optimization Strategies

1. **Sparse Iteration for Spread:** Only iterate cells with level > threshold
2. **SIMD for Grid Operations:** Use vectorized operations for decay, spread deltas
3. **Spatial Locality:** Process grid in cache-friendly row-major order
4. **Dirty Region Tracking:** Only recalculate land value in modified regions

### 10.3 Memory Access Pattern

```cpp
// Good: Sequential access
for (int32_t y = 0; y < height_; ++y) {
    for (int32_t x = 0; x < width_; ++x) {
        // Access grid_[y * width_ + x]
    }
}

// Bad: Random access
for (const auto& building : buildings) {
    // Random grid_[building.y * width_ + building.x] access
}
```

### 10.4 Benchmarking Targets

| System | 256x256 Target | 512x512 Target |
|--------|----------------|----------------|
| DisorderSystem full tick | <2ms | <8ms |
| ContaminationSystem full tick | <3ms | <12ms |
| LandValueSystem full recalc | <2ms | <8ms |
| **Total Value/Impact systems** | **<7ms** | **<28ms** |

---

## 11. Economic Impact

### 11.1 Effect on Zone Demand (Epic 11)

Sector value directly affects zone demand:

```cpp
// In DemandSystem (also Epic 10)
float calculate_zone_demand_modifier(ZoneBuildingType zone_type, int32_t x, int32_t y) {
    uint8_t value = land_value_system_->get_sector_value_at(x, y);

    // High-density zones prefer high value
    if (is_high_density(zone_type)) {
        if (value < 150) return 0.5f;  // Reduced demand
        if (value > 200) return 1.5f;  // Increased demand
    }

    // Low-density zones are more tolerant
    if (is_low_density(zone_type)) {
        if (value < 100) return 0.7f;
        if (value > 180) return 1.2f;
    }

    return 1.0f;  // Neutral
}
```

### 11.2 Effect on Tax Revenue (Epic 11)

Higher sector value = higher tribute (tax) base:

```cpp
// In EconomySystem (Epic 11)
uint32_t calculate_building_tax_base(EntityID building) {
    auto& pos = registry_.get<PositionComponent>(building);
    uint8_t value = land_value_system_->get_sector_value_at(pos.grid_x, pos.grid_y);

    // Tax base scales with sector value
    // value 128 (neutral) = 100% base tax
    // value 255 (max) = 200% base tax
    // value 0 (min) = 50% base tax
    float multiplier = 0.5f + (value / 255.0f) * 1.5f;

    return static_cast<uint32_t>(base_tax * multiplier);
}
```

### 11.3 Effect on Service Costs (Epic 9, Epic 11)

High disorder increases enforcer maintenance costs:

```cpp
// In ServicesSystem (Epic 9)
uint32_t calculate_enforcer_maintenance_multiplier(PlayerID owner) {
    uint32_t avg_disorder = disorder_system_->get_average_disorder_for_owner(owner);

    // High disorder = more expensive to maintain order
    // 0-50 = 1.0x, 50-100 = 1.25x, 100-150 = 1.5x, 150+ = 2.0x
    if (avg_disorder > 150) return 200;
    if (avg_disorder > 100) return 150;
    if (avg_disorder > 50) return 125;
    return 100;
}
```

High contamination increases hazmat/cleanup costs:

```cpp
uint32_t calculate_environmental_cleanup_cost(PlayerID owner) {
    uint32_t total_contamination = contamination_system_->get_total_contamination_for_owner(owner);

    // Per-unit cleanup cost
    return total_contamination * CLEANUP_COST_PER_UNIT;
}
```

### 11.4 Feedback Loops Summary

```
Positive Loop (Prosperity):
  High sector value -> More demand -> More buildings -> More tax revenue
                    -> Better services -> Less disorder -> Higher sector value

Negative Loop (Decline):
  Low sector value -> Less demand -> Abandoned buildings -> Less tax revenue
                   -> Worse services -> More disorder -> Lower sector value

Contamination Loop:
  Fabrication buildings -> Contamination -> Lower sector value near industry
                       -> Lower habitation demand nearby -> Industrial zones expand
                       -> More contamination (containable with zoning)
```

---

## Appendix A: Event Definitions

```cpp
// Disorder events
struct DisorderLevelChangedEvent {
    GridPosition position;
    uint8_t old_level;
    uint8_t new_level;
};

struct DisorderCrisisBeganEvent {
    PlayerID owner;
    uint32_t crisis_tile_count;  // Tiles > 200 disorder
};

struct DisorderCrisisEndedEvent {
    PlayerID owner;
};

// Contamination events
struct ContaminationLevelChangedEvent {
    GridPosition position;
    uint8_t old_level;
    uint8_t new_level;
    ContaminationType dominant_type;
};

struct ContaminationCrisisBeganEvent {
    PlayerID owner;
    uint32_t toxic_tile_count;  // Tiles > 200 contamination
};

struct ContaminationCrisisEndedEvent {
    PlayerID owner;
};

// Land value events
struct SectorValueChangedEvent {
    GridPosition position;
    uint8_t old_value;
    uint8_t new_value;
};

struct HighValueDistrictFormedEvent {
    PlayerID owner;
    GridRect region;
    uint8_t average_value;
};

struct SlumFormedEvent {
    PlayerID owner;
    GridRect region;
    uint8_t average_value;
    uint8_t average_disorder;
};
```

---

## Appendix B: Terminology Reference

| Human Term | Canonical Term | Code Identifier |
|------------|----------------|-----------------|
| Crime | Disorder | disorder, DisorderSystem |
| Crime rate | Disorder index | disorder_level |
| Pollution | Contamination | contamination, ContaminationSystem |
| Land value | Sector value | sector_value, LandValueSystem |
| Property value | Sector value | sector_value |
| Police | Enforcers | is_enforcer |
| Police station | Enforcer post | enforcer_post |
| Industrial pollution | Fabrication contamination | ContaminationType::Industrial |

---

## Appendix C: Dense Grid Memory Budget

| Grid | Cell Size | 128x128 | 256x256 | 512x512 |
|------|-----------|---------|---------|---------|
| DisorderGrid | 1 byte | 16 KB | 64 KB | 256 KB |
| DisorderGrid (prev buffer) | 1 byte | 16 KB | 64 KB | 256 KB |
| ContaminationGrid | 2 bytes | 32 KB | 128 KB | 512 KB |
| ContaminationGrid (prev buffer) | 2 bytes | 32 KB | 128 KB | 512 KB |
| LandValueGrid | 2 bytes | 32 KB | 128 KB | 512 KB |
| **Total Dense Grids** | | **128 KB** | **512 KB** | **2 MB** |

---

**Document Status:** Ready for review by Systems Architect, Game Designer, and Population Engineer

**Next Steps:**
1. @systems-architect to confirm tick priorities and double-buffer synchronization strategy
2. @game-designer to provide balance values for generation, decay, and value factors
3. @population-engineer to confirm disorder/contamination effects on migration and health
4. Finalize IContaminationSource interface integration with Epic 5 and Epic 7 systems
5. Convert work items to formal tickets after questions resolved
