# Systems Architect Analysis: Epic 3 — Terrain System

**Epic:** Terrain System
**Analyst:** Systems Architect
**Date:** 2026-01-28
**Canon Version Referenced:** 0.5.0
**Phase:** 1 (Foundation)

---

## Overview

Epic 3 establishes the world foundation for ZergCity. The TerrainSystem owns all tile data (terrain type, elevation), procedural generation, water body placement, vegetation placement, terrain modification, and sea level. It has **zero dependencies** on other epics at the system level (it depends on no other ECS systems), but it critically depends on Epic 2's rendering infrastructure to be visible, and its data is consumed by nearly every gameplay system in later phases.

This is the most-depended-upon ECS system in the entire project. Six systems directly depend on TerrainSystem: ZoneSystem, EnergySystem, FluidSystem, TransportSystem, PortSystem, and LandValueSystem. Indirectly, every system that queries terrain type, elevation, buildability, or water proximity flows through this system. Getting the data model and interface contracts right here is essential.

**Key architectural concern:** TerrainSystem is the first system that must produce data consumed by both the server (simulation) and the client (rendering). This means we must establish the pattern for how ECS component data flows from authoritative game state to the rendering pipeline, including the emissive/glow properties per terrain type defined in canon v0.5.0.

---

## System Boundaries

### What TerrainSystem Owns

| Responsibility | Details |
|---------------|---------|
| **Tile data** | Terrain type (10 types) and elevation (0-31) for every tile on the map |
| **Procedural generation** | Seeded, deterministic map generation that scales to 128/256/512 |
| **Water body placement** | Ocean (map edges), rivers (flow channels), lakes (inland basins) |
| **Vegetation placement** | Biolume groves (forests) and alien biome distribution |
| **Terrain modification** | Clearing (purge_terrain), leveling (grade_terrain), cost calculation |
| **Sea level** | Global sea level value that determines water/land boundaries |
| **Terrain queries** | Type, elevation, slope, buildability, water proximity at any coordinate |
| **Terrain gameplay effects** | Value bonuses, contamination, build cost modifiers per terrain type |

### What TerrainSystem Does NOT Own

| Responsibility | Owner | Interaction Pattern |
|---------------|-------|-------------------|
| Terrain rendering | RenderingSystem (Epic 2) | TerrainSystem provides data; RenderingSystem reads TerrainComponent + RenderComponent to produce visuals |
| What can be built where | ZoneSystem (Epic 4) | ZoneSystem calls TerrainSystem queries to validate placement |
| Terrain mesh generation | RenderingSystem or terrain-specific render code | TerrainSystem owns the data; mesh construction from that data is a rendering concern |
| Camera interaction with terrain | CameraSystem (Epic 2) | Ray casting against terrain heightmap is a camera/rendering concern |
| Player ownership of tiles | OwnershipComponent (cross-cutting) | TerrainSystem does not track who owns what |

### Boundary Clarity Issues

**Terrain mesh generation** is the most ambiguous boundary. Canon says RenderingSystem "owns terrain rendering," but terrain mesh generation sits between data and rendering. My recommendation:

- **TerrainSystem** owns the heightmap data (elevation per tile) and terrain type data
- **A terrain render subsystem** (within RenderingSystem or as a dedicated helper) converts heightmap data to renderable geometry (vertex buffers, instance data)
- This subsystem reads TerrainComponent data and produces GPU-ready representation
- TerrainSystem fires a `TerrainModifiedEvent` when tiles change; the render subsystem rebuilds affected chunks

This follows the ECS pattern: TerrainSystem writes components (data), RenderingSystem reads components (rendering).

**Terrain heightmap for ray casting** is another boundary question. Epic 2 ticket 2-030 (Tile Picking) specifies "accounts for elevation (terrain height)." The CameraSystem needs to intersect rays with the terrain surface, not just a flat plane. TerrainSystem must expose a fast `get_elevation_at(grid_x, grid_y)` query that the ray caster can use without going through a full ECS query.

---

## Data Model

### TerrainComponent Structure

```cpp
// Terrain type enum - 10 types per canon
enum class TerrainType : uint8_t {
    // Classic
    Substrate = 0,      // flat_ground - standard buildable
    Ridge = 1,          // hills - elevated, buildable
    DeepVoid = 2,       // ocean - map-edge water, not buildable
    FlowChannel = 3,    // river - flowing water, not buildable
    StillBasin = 4,     // lake - inland water, not buildable
    BiolumeGrove = 5,   // forest - clearable vegetation

    // Alien
    PrismaFields = 6,   // crystal_fields - bonus value, clearable
    SporeFlats = 7,     // spore_plains - harmony boost
    BlightMires = 8,    // toxic_marshes - contamination, not buildable
    EmberCrust = 9,     // volcanic_rock - increased build cost, geothermal bonus

    COUNT = 10
};

// Per-tile terrain data
struct TerrainComponent {
    TerrainType terrain_type = TerrainType::Substrate;
    uint8_t elevation = 0;           // 0-31 (32 levels)
    uint8_t moisture = 0;            // 0-255, affects vegetation density
    uint8_t flags = 0;               // bit flags (see below)
};

// TerrainComponent::flags bit definitions
// bit 0: is_cleared       - vegetation/crystals have been removed
// bit 1: is_underwater    - tile is below sea level
// bit 2: is_coastal       - tile adjacent to water
// bit 3: is_slope         - tile has significant elevation difference to neighbors
// bit 4-7: reserved
```

**Size:** 4 bytes per tile. This is critical for performance.

### Memory Layout: Dense Grid vs Per-Entity

**Decision: Dense 2D grid array, NOT per-entity ECS storage.**

Rationale:
- Terrain covers every tile on the map. There are no "missing" tiles.
- A 512x512 map = 262,144 tiles. Storing these as individual entities with ECS overhead is wasteful.
- Spatial queries (elevation at position, neighbors, slope) require O(1) random access by grid coordinate.
- Cache-friendly linear traversal is essential for generation and simulation.
- This matches how terrain works in virtually all tile-based games.

```cpp
// TerrainGrid - dense 2D storage
struct TerrainGrid {
    uint32_t width;                        // 128, 256, or 512
    uint32_t height;                       // Same as width (square maps)
    std::vector<TerrainComponent> tiles;   // width * height elements
    uint8_t sea_level = 8;                 // Global sea level (elevation units)

    // O(1) access
    TerrainComponent& at(uint32_t x, uint32_t y) {
        return tiles[y * width + x];
    }

    const TerrainComponent& at(uint32_t x, uint32_t y) const {
        return tiles[y * width + x];
    }

    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < (int32_t)width &&
               y >= 0 && y < (int32_t)height;
    }
};
```

**Memory budget:**
- 128x128: 16,384 tiles * 4 bytes = 64 KB
- 256x256: 65,536 tiles * 4 bytes = 256 KB
- 512x512: 262,144 tiles * 4 bytes = 1 MB

All sizes fit comfortably in L2 cache for linear traversal. This is excellent.

### Derived Data (Computed, Not Stored)

Some terrain properties are computed on demand from neighbor relationships rather than stored per-tile:

```cpp
// Slope between two adjacent tiles
uint8_t get_slope(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) const;

// Average elevation in a radius (for smoothing queries)
float get_average_elevation(uint32_t x, uint32_t y, uint32_t radius) const;

// Water proximity (distance to nearest water tile)
uint32_t get_water_distance(uint32_t x, uint32_t y) const;

// Is tile buildable (considers type, cleared state, underwater)
bool is_buildable(uint32_t x, uint32_t y) const;
```

`get_water_distance` is expensive to compute per-query for all 262K tiles. **Pre-compute a distance field** during generation and after water body changes. Store as a separate `uint8_t` grid (capped at 255 tiles). Cost: additional 1 MB for 512x512. This is a critical optimization since FluidSystem, LandValueSystem, and PortSystem all need water proximity.

### Terrain Gameplay Effect Data (Static Table)

```cpp
struct TerrainTypeInfo {
    TerrainType type;
    bool buildable;              // Can structures be placed (after clearing if needed)
    bool clearable;              // Can be cleared (forests, crystals)
    bool generates_contamination; // Toxic marshes
    int32_t clear_cost;          // Credits to clear
    int32_t clear_revenue;       // Credits gained from clearing (crystals)
    int32_t build_cost_modifier; // Percentage modifier (e.g., +50% for volcanic)
    float value_bonus;           // Sector value modifier
    float harmony_bonus;         // Harmony (happiness) modifier for nearby habitation

    // Bioluminescent rendering properties (consumed by RenderingSystem)
    Vec3 emissive_color;         // Per-type glow color
    float emissive_intensity;    // Base glow intensity (0.0-1.0)
};

// Static lookup table - 10 entries
static const TerrainTypeInfo TERRAIN_INFO[TerrainType::COUNT] = { /* ... */ };
```

This table is the single source of truth for all terrain type properties. Both TerrainSystem (gameplay) and RenderingSystem (visuals) reference it.

---

## Data Flow

### TerrainSystem to RenderingSystem

This is the most critical data flow in Epic 3. The terrain must be visible.

**Pattern: Component-Based Data Flow (Read-Only Consumer)**

```
TerrainSystem (Epic 3)          RenderingSystem (Epic 2)
  |                               |
  | Writes:                       | Reads:
  |  TerrainGrid                  |  TerrainGrid (read-only ref)
  |  (type, elevation per tile)   |  TerrainTypeInfo (emissive data)
  |                               |
  | Events:                       | Listens:
  |  TerrainModifiedEvent         |  TerrainModifiedEvent
  |  (which tiles changed)        |  (rebuilds affected GPU chunks)
  |                               |
  v                               v
```

**Terrain rendering data flow detail:**

1. TerrainSystem generates terrain during map creation (or loads from save)
2. TerrainSystem stores data in TerrainGrid (dense array)
3. RenderingSystem (or a terrain render subsystem) reads TerrainGrid to build:
   - Terrain mesh geometry (vertices from heightmap)
   - Per-instance data for instanced rendering:
     - Position (from grid coordinates)
     - Terrain type (determines which model/texture)
     - Elevation (determines Y position in 3D)
     - Emissive color and intensity (from TerrainTypeInfo lookup)
4. When terrain is modified (clearing, leveling), TerrainSystem fires `TerrainModifiedEvent` with the affected tile range
5. The terrain render subsystem rebuilds only the affected chunk(s)

**Per-instance emissive data** is specified by Epic 2 ticket 2-012 (GPU Instancing) and 2-037 (Emissive Material Support). Each terrain tile instance carries:
- `emissive_intensity` (float): base intensity from TerrainTypeInfo, potentially modulated by time/animation
- `emissive_color` (Vec3): from TerrainTypeInfo per-type glow color

The 10 terrain emissive presets (from ticket 2-008 ToonShaderConfig):

| Terrain Type | Canon Visual | Emissive Color | Intensity |
|-------------|-------------|---------------|-----------|
| Substrate | Dark soil, subtle bioluminescent moss | Dim teal | 0.05 |
| Ridge | Layered rock, glowing vein patterns | Soft cyan veins | 0.15 |
| DeepVoid | Dark water, bioluminescent particles | Deep blue-white | 0.10 |
| FlowChannel | Flowing water, particles | Soft blue-white | 0.12 |
| StillBasin | Still water, subtle glow | Soft blue-white | 0.08 |
| BiolumeGrove | Glowing alien trees/fungi | Teal-green | 0.25 |
| PrismaFields | Bright crystal spires | Magenta-cyan, strong | 0.60 |
| SporeFlats | Pulsing spore clouds | Green-teal, pulsing | 0.30 |
| BlightMires | Sickly glow, bubbling | Yellow-green | 0.20 |
| EmberCrust | Orange/red glow cracks | Orange-red | 0.35 |

These values will be refined by the Game Designer but the data pipeline must support them.

### TerrainSystem to ZoneSystem (Epic 4)

```
TerrainSystem.is_buildable(x, y) -> bool
TerrainSystem.get_terrain_type(x, y) -> TerrainType
TerrainSystem.get_elevation(x, y) -> uint8_t
TerrainSystem.get_slope(x1, y1, x2, y2) -> uint8_t
TerrainSystem.get_build_cost_modifier(x, y) -> int32_t
```

ZoneSystem calls these during zone placement validation. A tile is buildable if:
- `terrain_type` is buildable (see TerrainTypeInfo)
- OR `terrain_type` is clearable AND tile is already cleared
- AND tile is not underwater (elevation >= sea_level for non-water types)

### TerrainSystem to EnergySystem (Epic 5)

```
TerrainSystem.get_terrain_type(x, y) -> TerrainType
// EmberCrust provides geothermal energy bonus
// Placement validation for hydro plants near water
TerrainSystem.get_water_distance(x, y) -> uint32_t
```

### TerrainSystem to FluidSystem (Epic 6)

```
TerrainSystem.get_water_distance(x, y) -> uint32_t
// Pumps near water have 3x efficiency
// Critical: water distance must be pre-computed for performance
```

### TerrainSystem to TransportSystem (Epic 7)

```
TerrainSystem.get_elevation(x, y) -> uint8_t
TerrainSystem.get_terrain_type(x, y) -> TerrainType
TerrainSystem.is_buildable(x, y) -> bool
// Roads require buildable terrain
// Elevation differences affect road connectivity and cost
// Tunnels through terrain (future)
```

### TerrainSystem to PortSystem (Epic 8)

```
TerrainSystem.get_terrain_type(x, y) -> TerrainType
TerrainSystem.get_water_distance(x, y) -> uint32_t
// Aqua ports require adjacent water (DeepVoid)
// Aero ports require flat terrain
TerrainSystem.get_average_elevation(x, y, radius) -> float
```

### TerrainSystem to LandValueSystem (Epic 10)

```
TerrainSystem.get_water_distance(x, y) -> uint32_t
// Water proximity boosts sector value
TerrainSystem.get_terrain_type(x, y) -> TerrainType
// PrismaFields boost value, BlightMires reduce value
TerrainSystem.get_value_bonus(x, y) -> float
```

### TerrainSystem to ContaminationSystem (Epic 10)

```
TerrainSystem.get_terrain_type(x, y) -> TerrainType
// BlightMires generate contamination
TerrainSystem.get_contamination_output(x, y) -> uint32_t
```

---

## Key Work Items

### Terrain Data Infrastructure

- [ ] **TERRAIN-01: TerrainGrid data structure** Define TerrainComponent (4 bytes), TerrainGrid dense array, TerrainType enum (10 types), and in-bounds checking. Configurable map size (128/256/512). This is the foundational data structure everything else builds on.

- [ ] **TERRAIN-02: TerrainTypeInfo static table** Define gameplay properties per terrain type: buildable, clearable, costs, value modifiers, contamination, harmony. Also includes emissive rendering properties (color, intensity) from canon art direction. Single source of truth for terrain type behavior.

- [ ] **TERRAIN-03: Water distance field** Pre-computed uint8_t grid storing distance to nearest water tile (capped at 255). Computed during generation and updated on water body changes. Critical performance optimization for FluidSystem, LandValueSystem, PortSystem queries.

- [ ] **TERRAIN-04: TerrainModifiedEvent** Event structure carrying the affected tile range (GridRect) and modification type. Fired when tiles are cleared, leveled, or otherwise modified. Consumed by rendering subsystem for chunk rebuild and by other systems that cache terrain-derived data.

### Procedural Generation

- [ ] **TERRAIN-05: Seeded RNG and noise infrastructure** Deterministic random number generator seeded from a uint64 map seed. Simplex/Perlin noise implementation (or library integration). Fractal Brownian motion (fBm) for natural-looking terrain. All generation must be fully deterministic for multiplayer.

- [ ] **TERRAIN-06: Elevation generation** Generate elevation heightmap using multi-octave noise. Create ridges (hills/mountains), valleys, and flat areas (substrate). Configurable parameters: roughness, amplitude, feature scale. Output: elevation values 0-31 for every tile. Must scale naturally to all three map sizes.

- [ ] **TERRAIN-07: Water body generation** Place DeepVoid (ocean) along map edges. Generate FlowChannels (rivers) from high elevation to ocean/map-edge using gradient descent. Place StillBasins (lakes) in terrain depressions. Apply sea level: tiles below sea_level adjacent to water become water. Update water distance field after placement.

- [ ] **TERRAIN-08: Alien biome distribution** Place BiolumeGroves (forests), PrismaFields (crystals), SporeFlats (spores), BlightMires (toxic), and EmberCrust (volcanic) using noise-based distribution. Biomes should form coherent clusters, not random scatter. Respect constraints: BlightMires near water, EmberCrust on ridges, PrismaFields in clusters, SporeFlats on open substrate.

- [ ] **TERRAIN-09: Generation parameter scaling** Ensure generation parameters scale properly across 128/256/512 map sizes. Feature density should remain perceptually similar regardless of map size. A 512x512 map should not just be a zoomed-in 256x256 — it should have proportionally more features.

- [ ] **TERRAIN-10: Map validation pass** Post-generation validation: ensure minimum buildable area percentage, ensure water bodies are connected where they should be, ensure no isolated single-tile terrain anomalies, ensure at least one river exists, ensure coastline is continuous.

### Terrain Queries (Interface Implementation)

- [ ] **TERRAIN-11: Core terrain queries** Implement the query API that all downstream systems depend on: `get_terrain_type(x, y)`, `get_elevation(x, y)`, `is_buildable(x, y)`, `get_build_cost_modifier(x, y)`, `get_water_distance(x, y)`, `get_slope(x1, y1, x2, y2)`, `get_value_bonus(x, y)`, `get_average_elevation(x, y, radius)`. All queries must be O(1) or pre-computed.

- [ ] **TERRAIN-12: Terrain batch queries** Batch query support for systems that need data for many tiles: `get_tiles_in_rect(GridRect)`, `get_buildable_tiles_in_rect(GridRect)`, `count_terrain_type_in_rect(GridRect, TerrainType)`. Used by ZoneSystem for area operations and by simulation overlay generation.

### Terrain Modification

- [ ] **TERRAIN-13: Clear terrain (purge_terrain)** Clear vegetation (BiolumeGrove) and crystals (PrismaFields) from a tile. Costs credits (from TerrainTypeInfo). PrismaFields clearing yields one-time credit revenue. Sets is_cleared flag. Fires TerrainModifiedEvent. Requires authority validation (server-side in multiplayer).

- [ ] **TERRAIN-14: Level terrain (grade_terrain)** Raise or lower tile elevation. Cost scales with elevation change amount. Cannot modify water tiles. Cannot raise above max elevation (31). Fires TerrainModifiedEvent. Adjacent tiles may need slope recalculation.

- [ ] **TERRAIN-15: Terrain modification authority** All terrain modifications are server-authoritative. Client sends modification request (InputMessage). Server validates: player owns the tile (OwnershipComponent check), player can afford the cost (EconomySystem check in future), terrain type allows modification. Server applies change and syncs.

### Rendering Integration

- [ ] **TERRAIN-16: Terrain render data provider** Expose terrain data in a format efficient for RenderingSystem consumption. Provide per-tile data needed for instanced rendering: position, terrain type, elevation, emissive properties. Consider chunked access pattern (16x16 or 32x32 chunks matching Epic 2's spatial partitioning cell size).

- [ ] **TERRAIN-17: Terrain chunk dirty tracking** Track which chunks of the terrain grid have been modified since last render update. When TerrainModifiedEvent fires, mark affected chunks dirty. RenderingSystem can query dirty chunks and rebuild only those. Chunking aligns with Epic 2 spatial partitioning (2-050).

### Serialization

- [ ] **TERRAIN-18: Terrain serialization** Serialize TerrainGrid for network sync (full snapshot on join) and save/load (Epic 16). Implement ISerializable: fixed-size types, little-endian, versioned. For network: delta compression is possible (terrain rarely changes after generation) but full snapshot is simpler initially. For 512x512: 1MB uncompressed, compresses well due to spatial coherence.

---

## Multiplayer Implications

### Deterministic Generation

**This is the single most critical multiplayer requirement for Epic 3.**

All clients must generate identical terrain from the same seed. If terrain differs between server and clients, every subsequent system breaks (buildings placed on wrong terrain, water in wrong places, etc.).

Requirements:
1. **Seeded PRNG:** Use a deterministic PRNG (e.g., xoshiro256**) seeded with a uint64 map_seed. No use of `std::rand()` or platform-specific random.
2. **Deterministic noise:** Noise functions (Simplex/Perlin) must produce identical results across all platforms (Windows, macOS, Linux). Use integer-based or well-specified floating-point operations. Avoid relying on floating-point operation order that may differ across compilers.
3. **Fixed generation order:** The order in which tiles are generated must be specified and invariant. Row-major order, top-to-bottom, left-to-right.
4. **No threading in generation:** Procedural generation must be single-threaded to ensure deterministic order of RNG calls. (Generation happens once at game start; performance is acceptable.)
5. **Platform-invariant math:** Avoid `sin()`, `cos()`, or other transcendental functions in generation that may differ across platforms. Use lookup tables or integer approximations if needed.

**Verification strategy:** Generate terrain on two different platforms with the same seed; binary-compare the resulting TerrainGrid arrays. They must be identical.

### Terrain Modification Sync

Terrain modifications (clearing, leveling) are rare events (compared to 20Hz simulation ticks). They are handled as on-change sync, not every-tick:

1. Client sends `TerrainModifyRequest` message (position, modification type)
2. Server validates (ownership, cost, terrain type allows it)
3. Server applies modification to authoritative TerrainGrid
4. Server broadcasts `TerrainModifiedEvent` to all clients
5. All clients update their local TerrainGrid
6. All clients rebuild affected render chunks

**Conflict resolution:** Server is authoritative. If two players try to modify adjacent tiles simultaneously, server processes in order received. No conflict possible since tiles are independently owned (only owner can modify).

### Authority Model

| Data | Authority | Sync |
|------|-----------|------|
| TerrainGrid (initial) | Server generates from seed | Full snapshot on join; clients can also generate locally from seed for instant load |
| TerrainGrid (modifications) | Server | On-change broadcast |
| Sea level | Server | On-change (rarely changes) |
| Water distance field | Derived | Each client computes locally from TerrainGrid |
| Terrain type info table | Static | Compiled into binary, no sync needed |

**Optimization for join:** When a new player joins, the server can send just the map seed + list of modifications since generation. The client generates terrain locally from seed, then applies modifications. This is much smaller than sending the full 1MB grid. However, this only works if generation is truly deterministic (see above).

---

## Cross-Epic Dependencies

### What Epic 3 Needs From Epic 2

| Epic 2 Deliverable | Why Epic 3 Needs It | Ticket(s) |
|--------------------|--------------------|-----------|
| SDL_GPU device and pipeline | Terrain rendering | 2-001 through 2-007 |
| GPU instancing | Render 262K terrain tiles efficiently | 2-012 |
| Per-instance emissive data | Bioluminescent terrain glow | 2-012, 2-037 |
| Toon shader with emissive term | Terrain visual identity | 2-005, 2-037 |
| Bloom post-process | Terrain glow bleeds | 2-038 |
| Frustum culling | Only render visible terrain | 2-026 |
| Spatial partitioning | Efficiently cull 512x512 terrain | 2-050 |
| LOD framework | Distant terrain simplification | 2-049 |
| Ray casting against heightmap | Tile picking with elevation | 2-029, 2-030 |
| Render layers | Terrain renders on TERRAIN layer | 2-034 |
| TransformComponent and Position-to-Transform sync | Place terrain entities in 3D | 2-032, 2-033 |
| Configurable map sizes | 128/256/512 support | 2-025 |

**Hard requirement:** Epic 2's instancing system (2-012) must support at least 262,144 instances for 512x512 maps. The chunked instancing approach mentioned in 2-012 is essential. Epic 3 should align its chunk size with Epic 2's spatial partitioning cell size (16x16 or 32x32 from 2-050).

**Terrain heightmap for ray casting:** Epic 2 ticket 2-030 (Tile Picking) says "accounts for elevation." This means the ray caster needs access to terrain elevation data. Epic 3 must expose `get_elevation(x, y)` before Epic 2 can fully implement tile picking with elevation support. This creates a soft circular dependency: Epic 2 needs terrain data for picking, Epic 3 needs Epic 2 for rendering. Resolution: Epic 2 implements ray casting against a flat plane initially; Epic 3 provides the heightmap query; then ray casting is updated to use actual elevation.

### What Later Epics Need From Epic 3

| Consumer Epic | What They Need | Priority |
|--------------|---------------|----------|
| Epic 4 (Zoning) | `is_buildable()`, `get_terrain_type()`, `get_build_cost_modifier()` | Critical |
| Epic 5 (Energy) | `get_terrain_type()` for geothermal, `get_water_distance()` for hydro | High |
| Epic 6 (Water) | `get_water_distance()` for pump efficiency | Critical |
| Epic 7 (Transport) | `is_buildable()`, `get_elevation()` for road grading | High |
| Epic 8 (Ports) | `get_terrain_type()` for aqua port placement, `get_water_distance()` | Medium |
| Epic 10 (Simulation) | `get_value_bonus()` for land value, `get_terrain_type()` for contamination sources | High |
| Epic 12 (UI) | Terrain overlay data, minimap generation | Medium |
| Epic 13 (Disasters) | Elevation for flood simulation, terrain type for fire spread | Medium |

**Interface stability is paramount.** The query API designed in TERRAIN-11 will be called by at least 6 major systems. Changes to this API after those systems are built would be extremely costly. Lock it down during Epic 3 planning.

### Coordination With Epic 0 and Epic 1

- **Epic 0 (Foundation):** TerrainGrid is not an ECS entity-per-tile design, so it does not use the ECS registry directly for tile storage. However, TerrainSystem itself is an ECS system that is registered with the system executor. TerrainSystem may create entities for special terrain features (e.g., a forest entity for rendering a 3D tree model), which would use the ECS registry.
- **Epic 1 (Multiplayer):** TerrainSystem implements ISerializable for network sync. SyncSystem handles the transport. The deterministic generation requirement is the key constraint from Epic 1.

---

## Interface Contracts

### ITerrainQueryable (New Interface)

This interface should be added to `interfaces.yaml`. It is the primary way all downstream systems interact with terrain.

```cpp
class ITerrainQueryable {
public:
    // Core tile queries
    virtual TerrainType get_terrain_type(int32_t x, int32_t y) const = 0;
    virtual uint8_t get_elevation(int32_t x, int32_t y) const = 0;
    virtual bool is_buildable(int32_t x, int32_t y) const = 0;

    // Derived queries
    virtual uint8_t get_slope(int32_t x1, int32_t y1,
                              int32_t x2, int32_t y2) const = 0;
    virtual uint32_t get_water_distance(int32_t x, int32_t y) const = 0;
    virtual float get_average_elevation(int32_t x, int32_t y,
                                        uint32_t radius) const = 0;

    // Gameplay effect queries
    virtual float get_value_bonus(int32_t x, int32_t y) const = 0;
    virtual float get_harmony_bonus(int32_t x, int32_t y) const = 0;
    virtual int32_t get_build_cost_modifier(int32_t x, int32_t y) const = 0;
    virtual uint32_t get_contamination_output(int32_t x, int32_t y) const = 0;

    // Map metadata
    virtual uint32_t get_map_width() const = 0;
    virtual uint32_t get_map_height() const = 0;
    virtual uint8_t get_sea_level() const = 0;

    // Batch queries
    virtual void get_tiles_in_rect(GridRect rect,
                                   std::vector<TerrainComponent>& out) const = 0;
    virtual uint32_t count_terrain_type_in_rect(GridRect rect,
                                                TerrainType type) const = 0;
};
```

### ITerrainModifier (New Interface)

Separate interface for modification operations (only available server-side in multiplayer):

```cpp
class ITerrainModifier {
public:
    // Modification operations
    virtual bool clear_terrain(int32_t x, int32_t y, PlayerID player) = 0;
    virtual bool level_terrain(int32_t x, int32_t y, uint8_t target_elevation,
                               PlayerID player) = 0;
    virtual bool set_sea_level(uint8_t new_level) = 0;

    // Cost queries (can be called client-side for UI preview)
    virtual int64_t get_clear_cost(int32_t x, int32_t y) const = 0;
    virtual int64_t get_level_cost(int32_t x, int32_t y,
                                   uint8_t target_elevation) const = 0;
};
```

### ITerrainRenderData (New Interface)

Interface for rendering subsystem to efficiently read terrain data:

```cpp
class ITerrainRenderData {
public:
    // Raw grid access for mesh/instance generation
    virtual const TerrainGrid& get_grid() const = 0;

    // Terrain type info for visual properties
    virtual const TerrainTypeInfo& get_type_info(TerrainType type) const = 0;

    // Dirty chunk tracking
    virtual bool is_chunk_dirty(uint32_t chunk_x, uint32_t chunk_y) const = 0;
    virtual void clear_chunk_dirty(uint32_t chunk_x, uint32_t chunk_y) = 0;
    virtual uint32_t get_chunk_size() const = 0;  // 16 or 32
};
```

### IContaminationSource (Existing Interface)

TerrainSystem partially implements this for BlightMires (toxic marshes). Each BlightMires tile produces contamination per tick. However, terrain tiles are not individual entities, so this is exposed through a query rather than a per-entity component:

```cpp
// ContaminationSystem calls this during its tick
std::vector<ContaminationSourceData> get_terrain_contamination_sources() const;

struct ContaminationSourceData {
    GridPosition position;
    uint32_t contamination_per_tick;
    ContaminationType type;  // terrain
};
```

---

## Performance Considerations

### Memory Budget

| Data Structure | 128x128 | 256x256 | 512x512 |
|---------------|---------|---------|---------|
| TerrainGrid (4B/tile) | 64 KB | 256 KB | 1 MB |
| Water distance field (1B/tile) | 16 KB | 64 KB | 256 KB |
| Dirty chunk flags | ~1 KB | ~1 KB | ~4 KB |
| **Total CPU memory** | **~81 KB** | **~321 KB** | **~1.26 MB** |

This is negligible. The real memory concern is GPU-side (instance buffers for rendering), which is Epic 2's domain.

### Spatial Query Performance

All core queries must be O(1):
- `get_terrain_type(x, y)`: Direct array index. ~1ns.
- `get_elevation(x, y)`: Direct array index. ~1ns.
- `is_buildable(x, y)`: Array index + table lookup + flag check. ~2ns.
- `get_water_distance(x, y)`: Pre-computed distance field array index. ~1ns.
- `get_slope(x1, y1, x2, y2)`: Two array reads + subtraction. ~2ns.

Batch queries iterate a rectangular sub-region. For `get_tiles_in_rect(100x100)`:
- 10,000 array reads. At ~1ns each: ~10us. Acceptable.

### Cache-Friendly Layout

TerrainGrid stores tiles in row-major order (x varies fastest within a row). This means:
- Horizontal neighbor access is cache-friendly (adjacent in memory)
- Vertical neighbor access crosses rows (stride = width * 4 bytes)
- For 512x512: stride = 2048 bytes. Still within L1 cache line for small jumps.

Procedural generation iterates row-by-row (row-major), which is cache-optimal.

### Generation Performance

Procedural generation is a one-time cost at game start. Target budget:

| Map Size | Target Time | Notes |
|----------|------------|-------|
| 128x128 | <100ms | Instant feel |
| 256x256 | <500ms | Brief pause, loading screen |
| 512x512 | <2s | Loading screen with progress |

Primary cost: noise evaluation (multiple octaves per tile for elevation, moisture, biome selection). For 512x512 with 6 octaves: ~1.5M noise evaluations. At ~50ns each: ~75ms. Well within budget.

Water distance field BFS: O(width * height) = O(262K) for 512x512. ~1-5ms. Trivial.

### Rendering Performance (Epic 2 Constraints)

For rendering, the key constraint is draw call count and instance buffer size.

**Naive approach:** One instanced draw call per terrain type visible = up to 10 draw calls for terrain. This is excellent.

**Chunked approach:** Aligned with spatial partitioning (32x32 chunks). For 512x512: 16*16 = 256 chunks. Only visible chunks are submitted. At average ~50% visibility: ~128 chunks. With 10 terrain types per chunk: up to 1,280 draw calls just for terrain. This exceeds the 500-1000 budget.

**Recommended approach:** Hybrid. Build one large instance buffer per terrain type (not per chunk). Frustum cull at the chunk level to select which tile positions to include. Rebuild the instance buffer each frame with only visible tiles. Cost: iterate visible chunks, append tile positions. For 50% visibility of 512x512 with ~10 types: ~131K tile-type pairs to iterate. At ~10ns each: ~1.3ms. Acceptable but tight. Profile and optimize.

**Alternative:** Pre-build instance buffers per chunk. When a chunk is visible, bind its instance buffer and draw. 10 types * ~128 visible chunks = 1,280 draw calls. This is over budget. Reduce by merging similar terrain types into a single draw call with per-instance type index, then branch in shader. This brings it back to ~128 draw calls (one per visible chunk).

The terrain rendering strategy must be designed in coordination with the Graphics Engineer during implementation. Epic 3 provides the data; Epic 2 determines the GPU strategy.

---

## Questions for Other Agents

### @graphics-engineer

- **Q1:** What chunk size should terrain rendering use? This needs to align with Epic 2's spatial partitioning cell size (ticket 2-050 says 16x16 or 32x32). I recommend 32x32 to reduce draw call count while keeping instance buffer updates fast. Does this align with your GPU instancing strategy?

- **Q2:** Should terrain tiles be rendered as individual instanced quads/models, or should we generate a continuous terrain mesh (heightmap mesh) per chunk? A heightmap mesh is more efficient (single draw call per chunk regardless of terrain type count) but harder to support per-tile terrain type variation. A hybrid approach (heightmap mesh + per-tile type coloring via texture array) may be optimal.

- **Q3:** For water rendering, should water tiles be part of the terrain mesh (with a water material), or a separate translucent water plane rendered at sea level? Separate water plane enables reflection/refraction effects and animated water, but is more complex.

- **Q4:** The terrain emissive intensity values I proposed are initial estimates. How does the bloom post-process (ticket 2-038) interact with these? Should the emissive values be tuned relative to bloom threshold? For example, PrismaFields at 0.60 intensity — will that cause excessive bloom bleed?

### @game-designer

- **Q5:** What percentage of the map should be buildable versus unbuildable terrain? For a 256x256 map (65K tiles), if 40% is water/toxic/ocean, that leaves ~39K buildable tiles. Is this enough for 2-3 players? What about 128x128 (16K tiles, ~9.6K buildable)?

- **Q6:** The canon says "terrain editor mode (pre-city building)" — is this a separate mode where players shape terrain before the simulation begins? Or is terrain modification available during gameplay? The Epic 3 scope says "terrain modification" is owned but does not specify when.

- **Q7:** Should the procedural generator produce maps that favor some terrain types over others? For example, PrismaFields and SporeFlats are positive effects — should they be rarer to make them strategically valuable? BlightMires are negative — should they be common enough to be a real constraint?

- **Q8:** For multiplayer, should the map seed be visible to players? If players can share seeds, they can replay maps. Is map replayability desirable? This affects whether we expose the seed in UI.

- **Q9:** Sea level adjustment — the scope says TerrainSystem owns "sea level." Can players change sea level during gameplay? That would be a dramatic terrain modifier. Should this be a disaster event, a game-master tool, or not adjustable?

- **Q10:** How should terrain transitions look? When substrate is adjacent to PrismaFields, is the boundary sharp (per-tile) or gradual (blending)? Sharp is simpler; gradual looks better but adds rendering complexity.

---

## Risks & Concerns

### Risk 1: Deterministic Cross-Platform Generation (HIGH)

**Severity:** High
**Description:** Procedural generation must produce identical terrain on all platforms. Floating-point behavior varies across compilers (MSVC, GCC, Clang) and instruction sets (SSE, AVX, ARM NEON). Even with the same seed, different platforms may produce different noise values if we use `float` math naively.
**Mitigation:**
- Use integer-based noise (or fixed-point) for generation
- Compile with strict floating-point semantics (`/fp:strict` on MSVC, `-ffp-contract=off` on GCC/Clang)
- Write cross-platform verification tests (same seed -> same output binary)
- Have a "golden output" reference that CI tests against
- If floating-point determinism proves impossible, generate terrain server-side only and send full TerrainGrid to clients (1MB, compressed)

### Risk 2: Terrain Rendering Performance at 512x512 (MEDIUM-HIGH)

**Severity:** Medium-High
**Description:** 262,144 tiles is a lot of instances. Even with frustum culling, a zoomed-out view of a 512x512 map could show 50,000+ tiles. If each tile is a separate instance, the instance buffer update and draw call count may exceed budget.
**Mitigation:**
- Design chunk-based rendering from the start (not as an optimization later)
- Align chunk size with Epic 2 spatial partitioning
- Consider heightmap mesh approach (one mesh per chunk) instead of per-tile instances
- LOD framework (Epic 2 ticket 2-049): distant chunks can use simplified terrain
- Profile early with 512x512 placeholder data

### Risk 3: Terrain-Rendering Integration Complexity (MEDIUM)

**Severity:** Medium
**Description:** The boundary between TerrainSystem (data) and RenderingSystem (visuals) requires careful coordination. Terrain is the first system where we must establish the data-to-GPU pipeline. Getting this pattern wrong will affect every subsequent system (buildings, infrastructure, etc.).
**Mitigation:**
- Define the ITerrainRenderData interface clearly before implementation
- Prototype the terrain rendering pipeline early (even with flat colored quads)
- Use the TerrainModifiedEvent pattern consistently
- Document the data flow pattern for reuse by later epics

### Risk 4: Water Body Generation Quality (MEDIUM)

**Severity:** Medium
**Description:** Procedurally generating convincing rivers, coastlines, and lakes is notoriously difficult. Rivers must flow downhill, coastlines must be continuous, lakes must fill depressions. Bad water generation makes the map look artificial.
**Mitigation:**
- Use well-known algorithms: hydraulic erosion for rivers, depression filling for lakes
- Coastline via elevation threshold (sea level) rather than explicit generation
- Allow manual seed rejection ("generate new map" button)
- Post-generation validation (TERRAIN-10) catches pathological cases
- Reference existing open-source terrain generators for algorithms

### Risk 5: Terrain Modification Sync Edge Cases (LOW-MEDIUM)

**Severity:** Low-Medium
**Description:** In multiplayer, two players modifying adjacent tiles simultaneously could cause visual glitches (chunk rebuild order) or gameplay inconsistencies (one player's clear operation changes the buildability check for another player's zone placement).
**Mitigation:**
- Server processes modifications sequentially (no parallel terrain mods)
- Server broadcasts modification results in order
- Chunk rebuild is idempotent (rebuilding the same chunk twice is harmless)
- Ownership model prevents direct conflicts (you can only modify your tiles)

### Risk 6: Late Discovery of Missing Query Requirements (LOW)

**Severity:** Low
**Description:** Later epics (Phase 2-4) may need terrain queries we did not anticipate in Epic 3. Adding new queries is easy if the data exists, but if a later system needs data we did not store (e.g., soil fertility, wind exposure), we may need to modify TerrainComponent.
**Mitigation:**
- Reserve bits in TerrainComponent flags field (4 reserved bits)
- Keep TerrainComponent extensible (add fields without breaking serialization by versioning)
- Review query requirements with each Phase 2 epic during their planning
- The `moisture` field is somewhat forward-looking (not immediately used)
