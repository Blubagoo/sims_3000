# Zone Engineer Analysis: EPIC 4 -- Zoning System

**Agent:** Zone Engineer
**Epic:** 4 -- Zoning & Building System (ZoneSystem half)
**Date:** 2026-01-28
**Canon Version:** 0.6.0

---

## Table of Contents

1. [ZoneComponent Design](#1-zonecomponent-design)
2. [Zone Grid Storage](#2-zone-grid-storage)
3. [Zone Placement Logic](#3-zone-placement-logic)
4. [Zone Types & Density](#4-zone-types--density)
5. [Demand System](#5-demand-system)
6. [De-zoning](#6-de-zoning)
7. [Zone Queries API](#7-zone-queries-api)
8. [Forward Dependency Stubs](#8-forward-dependency-stubs)
9. [Multiplayer](#9-multiplayer)
10. [Key Work Items](#10-key-work-items)
11. [Questions for Other Agents](#11-questions-for-other-agents)
12. [Risks & Concerns](#12-risks--concerns)

---

## 1. ZoneComponent Design

### Overview

ZoneComponent is a per-entity ECS component. Unlike TerrainGrid (which uses the dense_grid_exception), zones are sparse -- most tiles on the map will NOT be designated (zoned). Per canon (systems.yaml `epic_4_zoning`), zones are tracked as ECS entities with ZoneComponent, not as a dense grid overlay. This is correct because:

- Zones are sparse: even in a mature colony, most tiles remain undesignated
- Zones are player-owned: each zone entity needs an OwnershipComponent
- Zones participate in the ECS query pipeline (BuildingSystem queries zone entities)
- Zone lifecycle is dynamic: zones are created, upgraded, and destroyed at player request

### Data Structure

```cpp
// Canonical alien terminology:
//   residential = habitation
//   commercial  = exchange
//   industrial  = fabrication

enum class ZoneType : uint8_t {
    Habitation  = 0,  // Residential
    Exchange    = 1,  // Commercial
    Fabrication = 2   // Industrial
};

enum class ZoneDensity : uint8_t {
    LowDensity  = 0,
    HighDensity  = 1
};

enum class ZoneState : uint8_t {
    Designated   = 0,  // Zoned, no building yet
    Occupied     = 1,  // Building present on this zone tile
    Stalled      = 2   // Designated but cannot develop (no road, no power, etc.)
};

struct ZoneComponent {
    ZoneType    zone_type;       // 1 byte
    ZoneDensity density;         // 1 byte
    ZoneState   state;           // 1 byte
    uint8_t     desirability;    // 1 byte -- 0-255, cached attractiveness score
    // Total: 4 bytes
};
```

### Size Considerations

- **4 bytes per zone entity** for the ZoneComponent itself.
- Each zone tile is an entity with additional cross-cutting components:
  - `PositionComponent` (12 bytes: grid_x, grid_y, elevation)
  - `OwnershipComponent` (~10 bytes: owner, state, timestamp)
  - ECS registry overhead (~24 bytes per entity for metadata/indices)
- **Total per zone tile entity: ~50 bytes** including ECS overhead.
- At maximum, a 512x512 map has 262,144 tiles. Realistically, even 30% zoned = ~78,000 zone entities = ~3.8 MB. This is well within budget.

### Design Rationale

The `desirability` field is a cached per-tile attractiveness score (0-255) updated each simulation tick. It aggregates sector value, service coverage, contamination, disorder, and transport access into a single byte that BuildingSystem and DemandSystem can read cheaply. This avoids recalculating desirability every time any system queries a zone.

`ZoneState::Stalled` represents tiles that are designated but cannot develop because prerequisites are not met (no road access, no power coverage, etc.). This state is useful for UI feedback (show stalled zones in a different color overlay) and for demand calculation (stalled zones should not count toward supply).

---

## 2. Zone Grid Storage

### Architecture: Per-Entity ECS with Spatial Index

Per canon, zones use per-entity ECS storage, NOT a dense grid. However, zones need fast spatial lookup ("what zone is at position X,Y?"). This requires a supplementary spatial index.

### Spatial Index Design

```cpp
// ZoneGrid is a lightweight spatial lookup, NOT dense storage.
// Only stores entity IDs at positions where zones exist.
// Backed by a flat array the same dimensions as the terrain grid,
// but storing EntityID (uint32_t) per cell. INVALID (0) means no zone.

class ZoneGrid {
public:
    ZoneGrid(uint32_t width, uint32_t height);

    // Core operations
    bool place_zone(int32_t x, int32_t y, EntityID zone_entity);
    bool remove_zone(int32_t x, int32_t y);
    EntityID get_zone_at(int32_t x, int32_t y) const;
    bool has_zone_at(int32_t x, int32_t y) const;
    bool in_bounds(int32_t x, int32_t y) const;

private:
    uint32_t width_;
    uint32_t height_;
    std::vector<EntityID> grid_;  // width * height, INVALID = no zone
};
```

### Memory Analysis

- `EntityID` = `uint32_t` = 4 bytes per cell
- 512x512 = 262,144 cells * 4 bytes = **1 MB** for the spatial index
- 256x256 = 65,536 cells * 4 bytes = **256 KB**
- 128x128 = 16,384 cells * 4 bytes = **64 KB**

This is acceptable. The spatial index gives O(1) point queries and prevents overlapping zones via a simple non-zero check.

### Relationship to Terrain Grid

The ZoneGrid has identical dimensions to TerrainGrid. Grid coordinates are shared: `ZoneGrid.get_zone_at(x, y)` and `TerrainSystem.get_terrain_type(x, y)` use the same (x, y) coordinate space. Zone placement validation checks terrain via `ITerrainQueryable.is_buildable(x, y)` before allowing zone designation.

### Query Patterns

| Query | Implementation | Complexity |
|-------|---------------|------------|
| Zone at position? | ZoneGrid lookup | O(1) |
| All zones of type X? | ECS component query (iterate ZoneComponent where zone_type == X) | O(n_zones) |
| Zone count by type? | Cached counters, updated on place/remove | O(1) |
| Zones in rect? | Iterate ZoneGrid sub-rect | O(rect_area) |
| Zones near position? | ZoneGrid area scan | O(search_area) |

### Cached Aggregate Counters

Maintaining per-player, per-type, per-density counters avoids iterating all zones for common statistics:

```cpp
struct ZoneCounts {
    uint32_t by_type[3];                    // habitation, exchange, fabrication
    uint32_t by_type_and_density[3][2];     // [type][density]
    uint32_t designated_count;              // zones with no building
    uint32_t occupied_count;                // zones with a building
    uint32_t stalled_count;                 // zones that cannot develop
    uint32_t total;
};

// Per-player ZoneCounts stored in ZoneSystem, updated incrementally on zone add/remove/state change.
```

---

## 3. Zone Placement Logic

### Validation Pipeline

Zone placement follows a strict validation order. All checks must pass before a zone is created:

```
1. BOUNDS CHECK     -- Is (x, y) within map bounds?
2. OWNERSHIP CHECK  -- Does the requesting player own this tile?
3. TERRAIN CHECK    -- Is ITerrainQueryable.is_buildable(x, y) true?
4. OVERLAP CHECK    -- Is ZoneGrid.has_zone_at(x, y) false?
5. ROAD CHECK       -- Is there a road within 3 tiles? (STUB for Epic 7)
6. BUILDING CHECK   -- Is there a non-zone building at this position? (via IGridQueryable)
```

### Validation Details

**Step 1 -- Bounds Check:**
Trivial. Reject coordinates outside `[0, map_width)` x `[0, map_height)`.

**Step 2 -- Ownership Check:**
Query `OwnershipComponent` for the tile at (x, y). The tile must be owned by the requesting player (`owner == player_id` AND `state == OwnershipState::Owned`). Tiles owned by GAME_MASTER (uncharted) or other players cannot be zoned.

**Step 3 -- Terrain Check:**
Call `ITerrainQueryable::is_buildable(x, y)`. This returns false for water tiles, toxic marshes (blight_mires), uncleared vegetation, etc. Volcanic rock (ember_crust) IS buildable but has increased cost -- that cost applies to buildings, not zone designation itself. Zone designation is free (or near-free) in terms of credits.

**Step 4 -- Overlap Check:**
Check `ZoneGrid.has_zone_at(x, y)`. If a zone already exists at this position, placement fails. To change zone type, player must first de-zone (redesignate) then re-zone. Or we can support direct redesignation as a single operation (de-zone + re-zone atomically).

**Step 5 -- Road Proximity Check (STUB):**
Canon requires zones within 3 tiles of a road (pathway) per `ITransportConnectable`. TransportSystem is Epic 7 -- a forward dependency. For Epic 4, we provide a stub interface that always returns true (road accessible). See Section 8 for stub design.

**IMPORTANT design decision:** The 3-tile road proximity rule should gate BUILDING DEVELOPMENT, not zone designation. Players should be able to designate zones before roads are built. The zone overlay should show a "stalled" indicator for zones without road access. This matches SimCity behavior where you zone first, then connect roads, then buildings develop.

**Step 6 -- Building Check:**
Ensure no non-zone building (landmark, service building, energy nexus, etc.) occupies this tile. Zone designation applies to empty tiles.

### Drag-to-Zone Tool Mechanics

Zone designation uses a drag tool (click-and-drag to designate a rectangular area):

```
1. Player selects zone tool (habitation/exchange/fabrication, low/high density)
2. Mouse down at (x1, y1) -- start position
3. Mouse drag to (x2, y2) -- preview rectangle shown as overlay
4. Mouse up -- commit: validate and place zones for all tiles in rectangle
5. Failed tiles silently skipped (water, other players' tiles, existing zones)
6. Successful tiles counted and cost deducted
```

**Multi-Tile Placement Flow:**

```cpp
struct ZonePlacementRequest {
    GridRect     area;        // Rectangle defined by drag
    ZoneType     zone_type;
    ZoneDensity  density;
    PlayerID     player_id;
};

struct ZonePlacementResult {
    uint32_t tiles_designated;   // How many succeeded
    uint32_t tiles_skipped;      // How many failed validation
    Credits  total_cost;         // Total cost deducted
};
```

On the client side, the drag preview shows which tiles will be zoned (green overlay) and which will be skipped (red overlay or no highlight) in real-time as the player drags.

### Zone Designation Cost

Zone designation should have a nominal per-tile cost to prevent spam zoning. Suggested: 1-5 credits per tile depending on density. Low density is cheaper than high density. The cost is per-tile and deducted atomically on commit.

```
Low density designation:  2 credits per tile
High density designation: 5 credits per tile
```

These values are configurable and should be balanced during playtesting.

---

## 4. Zone Types & Density

### Zone Type Definitions

| Canon Name | Human Equivalent | Purpose | Develops Into |
|-----------|-----------------|---------|---------------|
| Habitation | Residential | Living quarters for beings | Dwellings, hab units, towers |
| Exchange | Commercial | Trade and services | Work centers, storage hubs |
| Fabrication | Industrial | Manufacturing and production | Fabricators, storage hubs |

### Density Levels

Canon specifies two density levels: `low_density` and `high_density`.

| Property | Low Density | High Density |
|----------|------------|-------------|
| Building height | 1-3 stories | 4-12+ stories |
| Population capacity | Low (per building) | High (per building) |
| Energy consumption | Low | High |
| Fluid consumption | Low | High |
| Contamination output | Low (fabrication only) | High (fabrication only) |
| Road traffic | Low | High |
| Sector value impact | Slight positive | Neutral to negative |
| Designation cost | 2 credits/tile | 5 credits/tile |

### Density Data

Density is stored in `ZoneComponent.density` and is set at designation time by the player. The player explicitly chooses low or high density when using the zone tool.

**NOTE:** The zone-engineer agent profile mentions "low, medium, high" density but canon `terminology.yaml` defines only `low_density` and `high_density` (no medium). Canon wins. Two density levels.

### Density Upgrade Path

Canon `building_templates.density` uses `"low | high"`. There is no automatic density upgrade. The player explicitly designates a tile as low or high density. To change density, the player must de-zone (destroying any building) and re-zone at the new density.

**Alternative consideration:** We could allow density upgrade without de-zoning if no building exists yet (tile is in `Designated` state). If a building exists, de-zone is required. This is a minor convenience that avoids unnecessary destroy/rebuild for empty zones.

### What Data Density Affects

Density is consumed by:
- **BuildingSystem:** Selects building template from the matching density pool
- **DemandSystem:** Demand is calculated per zone type, with density weighting
- **PopulationSystem:** Population capacity per building scales with density
- **EnergySystem/FluidSystem:** Consumption per building scales with density
- **ContaminationSystem:** Fabrication contamination output scales with density
- **TransportSystem (Epic 7):** Traffic generation per building scales with density
- **LandValueSystem:** High density habitation may reduce nearby sector value

---

## 5. Demand System

### Overview

The demand system calculates zone_pressure (RCI demand) -- how much the colony "wants" more of each zone type. Demand drives building spawning: when demand is positive and zones are available, buildings develop. When demand is negative, buildings may downgrade or become derelict.

Per canon (`systems.yaml`), there is both a basic demand owned by ZoneSystem (priority 30) and a fuller DemandSystem in Epic 10 (priority unspecified, depends on PopulationSystem, EconomySystem, TransportSystem). For Epic 4, we implement the **basic demand calculation** that ZoneSystem owns, with hooks for DemandSystem (Epic 10) to override or refine later.

### Demand Algorithm

Demand is calculated per player, per zone type. Output is a signed value from -100 to +100 representing growth pressure.

```
demand[type] = base_pressure[type]
             + population_factor[type]
             + employment_factor[type]
             + utility_factor[type]
             + tribute_factor[type]
             - supply_saturation[type]
```

#### Factor Breakdown

**Base Pressure (always positive, provides minimum growth):**
- Every colony has a small baseline demand for all zone types
- Habitation base: +10
- Exchange base: +5
- Fabrication base: +5

**Population Factor (Epic 10 -- stub for Epic 4):**
- More beings = more demand for exchange and fabrication
- More unemployed beings = more demand for exchange and fabrication (jobs)
- Overcrowded habitation = more demand for habitation
- **Epic 4 stub:** Fixed +20 for habitation, +10 for exchange, +10 for fabrication

**Employment Factor (Epic 10 -- stub for Epic 4):**
- High employment rate reduces habitation demand (people are content)
- Low employment rate increases exchange/fabrication demand (need jobs)
- **Epic 4 stub:** Fixed 0

**Utility Factor (Epic 5/6 -- stub for Epic 4):**
- Available energy and fluid coverage encourage development
- Lack of utilities suppresses demand
- **Epic 4 stub:** Fixed +10 (assume utilities available)

**Tribute Factor (Epic 11 -- stub for Epic 4):**
- High tribute rates suppress demand (beings leave)
- Low tribute rates boost demand (beings immigrate)
- **Epic 4 stub:** Fixed 0

**Supply Saturation (computed from actual zone counts):**
- As more zones of a type exist relative to demand, pressure decreases
- `supply_saturation[type] = (occupied_zone_count[type] / max(1, target_count[type])) * 100`
- `target_count[type]` derived from colony population and demand factors
- **Epic 4 implementation:** Simple ratio of occupied zones to total designated zones

### Demand Caps

- Hard cap: demand clamped to [-100, +100]
- Soft cap: above +80, demand growth rate halved (diminishing returns)
- Below -50: buildings begin abandonment process (handled by BuildingSystem)

### Demand Update Frequency

Demand is recalculated once per simulation tick (50ms = 20 ticks/sec) as part of ZoneSystem::tick(). This is lightweight: a few additions per zone type per player. No iteration over zone entities needed thanks to cached counters.

### Demand Display Data

For UI consumption (Epic 12):

```cpp
struct ZoneDemandData {
    int8_t habitation_demand;   // -100 to +100
    int8_t exchange_demand;     // -100 to +100
    int8_t fabrication_demand;  // -100 to +100
};

// Per player. Queried via ZoneSystem::get_zone_demand(PlayerID).
```

This directly maps to the classic SimCity RCI demand bar display.

### Demand-to-Building Pipeline

When demand > 0 for a zone type:
1. ZoneSystem reports positive demand
2. BuildingSystem (Epic 4, Building Engineer's scope) queries zones of that type in `Designated` state
3. BuildingSystem checks zone prerequisites (road access, power, water)
4. BuildingSystem selects a building template matching zone type + density
5. Building materializes on the zone tile
6. Zone state transitions from `Designated` to `Occupied`

When demand < 0 for a zone type:
1. ZoneSystem reports negative demand
2. BuildingSystem may transition buildings to `Abandoned` (derelict) state
3. Zone state may revert from `Occupied` to `Designated` if building is demolished

---

## 6. De-zoning

### Rules for Removing Zones

De-zoning (undesignation) removes the zone designation from a tile and returns it to undesignated state.

**Validation for de-zoning:**
1. Tile must have an existing zone (`ZoneGrid.has_zone_at(x, y)`)
2. Requesting player must own the tile (`OwnershipComponent.owner == player_id`)

### What Happens to Buildings on De-zoned Tiles

**If the zone tile has a building (`ZoneState::Occupied`):**
- De-zoning triggers demolition of the building on that tile
- BuildingSystem handles the demolition process (may take time if demolition is multi-tick)
- Building demolition may produce debris that requires clearing
- After building is demolished, zone entity is destroyed

**If the zone tile has no building (`ZoneState::Designated` or `ZoneState::Stalled`):**
- Zone entity is immediately destroyed
- No cost or delay

### Cost

- De-zoning an empty zone: Free (0 credits)
- De-zoning an occupied zone: Standard demolition cost (determined by BuildingSystem based on building type and size)
- No refund of original designation cost

### Restrictions

- Cannot de-zone tiles you do not own
- Cannot de-zone during active disaster on that tile (if DisasterSystem is active, Epic 13)
- De-zoning in bulk (drag-to-dezone) follows same pattern as zone placement: iterate rectangle, skip invalid tiles

### Redesignation (Zone Type Change)

Changing a zone from one type to another (e.g., habitation -> exchange) is implemented as atomic de-zone + re-zone:

1. If building exists: demolish first (cost applies)
2. Remove old zone entity
3. Create new zone entity with new type/density
4. Deduct new designation cost

This could be offered as a single "redesignation" tool action for player convenience.

---

## 7. Zone Queries API

### Interface Definition

ZoneSystem exposes queries through direct method calls (not a formal interface like ITerrainQueryable, since ZoneSystem consumers are well-known: BuildingSystem, DemandSystem, UISystem, EconomySystem).

```cpp
class ZoneSystem : public ISimulatable {
public:
    // === ISimulatable ===
    void tick(float delta_time) override;
    int get_priority() override { return 30; }

    // === Zone Placement ===
    ZonePlacementResult place_zones(const ZonePlacementRequest& request);
    DezoneResult remove_zones(const GridRect& area, PlayerID player_id);
    bool redesignate_zone(int32_t x, int32_t y, ZoneType new_type,
                          ZoneDensity new_density, PlayerID player_id);

    // === Point Queries ===
    EntityID get_zone_entity_at(int32_t x, int32_t y) const;
    bool has_zone_at(int32_t x, int32_t y) const;
    std::optional<ZoneType> get_zone_type_at(int32_t x, int32_t y) const;
    std::optional<ZoneDensity> get_zone_density_at(int32_t x, int32_t y) const;
    std::optional<ZoneState> get_zone_state_at(int32_t x, int32_t y) const;

    // === Area Queries ===
    std::vector<EntityID> get_zones_in_rect(const GridRect& rect) const;
    std::vector<EntityID> get_zones_of_type(ZoneType type, PlayerID player_id) const;
    std::vector<EntityID> get_designated_zones(ZoneType type, PlayerID player_id) const;

    // === Aggregate Queries ===
    const ZoneCounts& get_zone_counts(PlayerID player_id) const;
    uint32_t count_zones_by_type(ZoneType type, PlayerID player_id) const;
    uint32_t count_zones_by_type_and_density(ZoneType type, ZoneDensity density,
                                              PlayerID player_id) const;

    // === Demand Queries ===
    ZoneDemandData get_zone_demand(PlayerID player_id) const;
    int8_t get_demand_for_type(ZoneType type, PlayerID player_id) const;

    // === State Mutation (called by BuildingSystem) ===
    void set_zone_state(int32_t x, int32_t y, ZoneState new_state);
    void update_desirability(int32_t x, int32_t y, uint8_t desirability);

    // === Map Info ===
    uint32_t get_map_width() const;
    uint32_t get_map_height() const;
};
```

### Consumer Usage

| Consumer System | Queries Used |
|----------------|-------------|
| BuildingSystem | `get_designated_zones()`, `get_zone_type_at()`, `get_zone_density_at()`, `set_zone_state()` |
| DemandSystem (Epic 10) | `get_zone_counts()`, `get_zone_demand()` |
| EconomySystem (Epic 11) | `count_zones_by_type()`, `count_zones_by_type_and_density()` |
| UISystem (Epic 12) | `get_zone_type_at()`, `get_zone_demand()`, zone overlay data |
| LandValueSystem (Epic 10) | `get_zone_type_at()`, `get_zone_density_at()` |
| PopulationSystem (Epic 10) | `get_zone_counts()` (habitation supply) |

---

## 8. Forward Dependency Stubs

### Problem

ZoneSystem depends on TransportSystem for road proximity checking, but TransportSystem is Epic 7. Canon states: "Buildings beyond 3 tiles from road cannot develop" (interfaces.yaml `ITransportConnectable`).

### Design Decision: Gate Building Development, Not Zone Designation

After analysis, the road proximity rule should apply to **building development** (BuildingSystem), not zone designation (ZoneSystem). Reasoning:

1. **Player workflow:** Players zone before building roads (SimCity convention)
2. **Visual feedback:** Zone overlay shows planned zones; stalled zones indicate missing roads
3. **Separation of concerns:** ZoneSystem designates; BuildingSystem develops
4. **Stub simplification:** ZoneSystem does not need the road stub at all for placement

However, ZoneSystem still needs road proximity awareness for:
- Setting `ZoneState::Stalled` on zones without road access
- The `desirability` score (lower if no road access)

### Stub Interface

```cpp
// Forward dependency stub for TransportSystem (Epic 7)
// Implemented as a simple interface that always returns "road accessible"
// until Epic 7 replaces it with real road proximity checking.

class ITransportAccessible {
public:
    virtual ~ITransportAccessible() = default;

    // Returns true if position is within max_distance tiles of a road
    virtual bool is_road_accessible(int32_t x, int32_t y,
                                     uint32_t max_distance = 3) const = 0;

    // Returns distance to nearest road (0 = adjacent, UINT32_MAX = no road)
    virtual uint32_t get_nearest_road_distance(int32_t x, int32_t y) const = 0;

    // Returns true if transport system is available (false = stub mode)
    virtual bool is_transport_available() const = 0;
};

// Stub implementation for Epic 4 (before TransportSystem exists)
class TransportStub : public ITransportAccessible {
public:
    bool is_road_accessible(int32_t x, int32_t y,
                             uint32_t max_distance = 3) const override {
        return true;  // Everything accessible until Epic 7
    }

    uint32_t get_nearest_road_distance(int32_t x, int32_t y) const override {
        return 0;  // Pretend road is adjacent
    }

    bool is_transport_available() const override {
        return false;  // Signal that we're in stub mode
    }
};
```

### Integration Point

ZoneSystem holds a pointer to `ITransportAccessible`. During Epic 4, this points to `TransportStub`. When Epic 7 is implemented, TransportSystem implements `ITransportAccessible` and replaces the stub. No ZoneSystem code changes needed.

```cpp
// ZoneSystem constructor
ZoneSystem(ITerrainQueryable* terrain, ITransportAccessible* transport);
```

### Behavior With Stub

When `is_transport_available()` returns false:
- All zones are considered road-accessible
- No zones enter `Stalled` state due to road distance
- Desirability is not penalized for road distance
- BuildingSystem develops buildings freely on designated zones

When TransportSystem replaces the stub (Epic 7):
- Zones beyond 3 tiles from a road enter `Stalled` state
- Desirability is penalized for road distance
- BuildingSystem only develops on road-accessible zones

---

## 9. Multiplayer

### Server-Authoritative Validation Flow

All zone operations are server-authoritative. Clients send requests; server validates, applies, and broadcasts results.

```
CLIENT                           SERVER
  |                                |
  |  ZonePlacementRequest -------> |
  |                                | 1. Validate bounds
  |                                | 2. Validate ownership (per-tile)
  |                                | 3. Validate terrain (is_buildable)
  |                                | 4. Validate no overlap
  |                                | 5. Validate road proximity (stub: pass)
  |                                | 6. Deduct credits (if EconomySystem ready)
  |                                | 7. Create zone entities
  |                                | 8. Update ZoneGrid
  |                                | 9. Update ZoneCounts
  |                                |
  |  <--- ZonePlacementResult ---- |  (success count, cost)
  |  <--- StateUpdateMessage ----- |  (new entities + components)
  |                                |
```

### Network Messages

```cpp
// Client -> Server
struct ZonePlacementRequestMsg {
    MessageType type = MessageType::ZonePlacement;
    GridRect area;              // Rectangle to zone
    ZoneType zone_type;
    ZoneDensity density;
    // PlayerID inferred from connection
};

struct DezoneRequestMsg {
    MessageType type = MessageType::Dezone;
    GridRect area;              // Rectangle to de-zone
};

struct RedesignateRequestMsg {
    MessageType type = MessageType::Redesignate;
    int32_t x, y;              // Single tile
    ZoneType new_type;
    ZoneDensity new_density;
};

// Server -> All Clients (via StateUpdateMessage)
// Zone entity creation/destruction flows through standard ECS sync (Epic 1).
// ZoneComponent values sync as part of normal component delta updates.
```

### Ownership Enforcement

Only the tile owner can zone a tile. This is enforced server-side in step 2 of the validation pipeline. The ownership check is per-tile within the requested rectangle. In a multi-tile drag operation:
- Tiles owned by the requesting player: zoned
- Tiles owned by other players: silently skipped
- Tiles owned by GAME_MASTER: silently skipped (must purchase first)

The result message tells the client how many tiles succeeded vs. were skipped.

### What Syncs

| Data | Sync Method | Frequency |
|------|------------|-----------|
| ZoneComponent values | ECS component delta (Epic 1 SyncSystem) | On change |
| Zone entity create/destroy | ECS entity delta (Epic 1 SyncSystem) | On change |
| ZoneGrid spatial index | NOT synced -- each client maintains locally from entity events | N/A |
| ZoneCounts aggregates | NOT synced -- each client derives locally | N/A |
| ZoneDemandData | Periodic stat sync (every ~10 ticks) | Low frequency |

### Concurrent Request Handling

Multiple players can zone simultaneously because:
- **Tile ownership prevents conflicts:** Player A can only zone their tiles; Player B can only zone their tiles. No overlap possible.
- **Server processes sequentially:** Requests are processed in arrival order within the tick.
- **Atomic per-request:** Each ZonePlacementRequest is processed atomically. Partial failures (some tiles skipped) are reported in the result.

**Edge case -- GAME_MASTER tiles:** Two players cannot both purchase and zone the same GAME_MASTER tile simultaneously. Tile purchase (separate system) is a separate step that must complete before zoning. Purchase is serialized by the server.

### Client-Side Prediction

Per canon: "No client-side prediction (simplicity over responsiveness)." Zone placement is NOT predicted client-side. The player sees the zone overlay after the server confirms. Given the 50ms tick rate and ~100ms typical network latency, the delay is ~150ms -- acceptable for a zone placement action (not time-critical).

The drag preview overlay IS client-side (shows which tiles would be zoned based on local validation), but the actual zone creation waits for server confirmation.

---

## 10. Key Work Items

### Group A: Data Structures and Types

**4-Z01: Zone Type and Component Definitions**
- Type: infrastructure
- Scope: S
- Define `ZoneType`, `ZoneDensity`, `ZoneState` enums
- Define `ZoneComponent` struct (4 bytes, static_assert verified)
- Define `ZoneDemandData` struct
- Define `ZoneCounts` struct
- Define `ZonePlacementRequest`, `ZonePlacementResult`, `DezoneResult` structs
- File: `src/components/ZoneComponent.h`
- Blocked by: None (foundational)
- Blocks: 4-Z02, 4-Z03, 4-Z04, 4-Z05, 4-Z06, 4-Z07

**4-Z02: ZoneGrid Spatial Index**
- Type: infrastructure
- Scope: M
- Implement `ZoneGrid` class with EntityID-per-cell storage
- `place_zone()`, `remove_zone()`, `get_zone_at()`, `has_zone_at()`
- Bounds checking, row-major layout
- Support all map sizes (128/256/512)
- File: `src/simulation/zones/ZoneGrid.h`, `src/simulation/zones/ZoneGrid.cpp`
- Blocked by: 4-Z01
- Blocks: 4-Z04, 4-Z05, 4-Z06, 4-Z10

### Group B: Zone Placement and Validation

**4-Z03: ITransportAccessible Stub Interface**
- Type: infrastructure
- Scope: S
- Define `ITransportAccessible` abstract interface
- Implement `TransportStub` (always returns accessible)
- File: `src/simulation/zones/ITransportAccessible.h`
- Blocked by: None
- Blocks: 4-Z04

**4-Z04: Zone Placement Validation Pipeline**
- Type: feature
- Scope: L
- Implement full validation pipeline: bounds, ownership, terrain, overlap, road (stub), building check
- Validates single tile and rectangular area (multi-tile)
- Returns per-tile validation result for drag preview
- File: `src/simulation/zones/ZoneSystem.cpp` (placement methods)
- Blocked by: 4-Z01, 4-Z02, 4-Z03, Epic 3 ITerrainQueryable
- Blocks: 4-Z05, 4-Z10

**4-Z05: Zone Placement Execution**
- Type: feature
- Scope: M
- Create zone entities with ZoneComponent, PositionComponent, OwnershipComponent
- Update ZoneGrid spatial index
- Update ZoneCounts aggregates
- Emit ZoneDesignatedEvent
- Handle multi-tile placement (iterate rectangle, skip failures)
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z04
- Blocks: 4-Z10

### Group C: De-zoning and Redesignation

**4-Z06: De-zoning Implementation**
- Type: feature
- Scope: M
- Validate de-zone request (zone exists, player owns tile)
- For occupied zones: trigger building demolition (integration point with BuildingSystem)
- Destroy zone entity, update ZoneGrid, update ZoneCounts
- Emit ZoneUndesignatedEvent
- Support rectangular de-zone (drag-to-dezone)
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z01, 4-Z02
- Blocks: 4-Z07

**4-Z07: Redesignation (Zone Type Change)**
- Type: feature
- Scope: S
- Atomic de-zone + re-zone for single tile
- Shortcut: if zone is Designated (no building), skip demolition
- Validate new type/density, deduct cost difference
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z06
- Blocks: None

### Group D: Demand System

**4-Z08: Basic Demand Calculation**
- Type: feature
- Scope: L
- Implement per-player, per-zone-type demand calculation
- Base pressure + stub factors for population, employment, utilities, tribute
- Supply saturation from zone counts
- Demand clamped to [-100, +100]
- Update once per tick in ZoneSystem::tick()
- File: `src/simulation/zones/ZoneDemand.h`, `src/simulation/zones/ZoneDemand.cpp`
- Blocked by: 4-Z01, 4-Z02
- Blocks: 4-Z09

**4-Z09: Demand Factor Extension Points**
- Type: infrastructure
- Scope: S
- Define demand factor interface for future systems to inject factors
- PopulationSystem, EconomySystem, TransportSystem will provide factors in their epics
- DemandSystem (Epic 10) will aggregate all factors
- File: `src/simulation/zones/ZoneDemand.h`
- Blocked by: 4-Z08
- Blocks: None

### Group E: Desirability and Zone State

**4-Z10: Zone Desirability Calculation**
- Type: feature
- Scope: M
- Calculate per-zone-tile desirability score (0-255)
- Factors: terrain value bonus, road proximity (stub: max), contamination penalty (stub: 0), service coverage (stub: neutral)
- Cache result in ZoneComponent.desirability
- Update periodically (every N ticks, not every tick -- e.g., every 10 ticks)
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z04, 4-Z05, 4-Z02
- Blocks: None

**4-Z11: Zone State Management**
- Type: feature
- Scope: M
- Manage ZoneState transitions: Designated -> Occupied (when building placed), Occupied -> Designated (when building demolished), Designated -> Stalled (when road access lost -- requires Epic 7), Stalled -> Designated (when road access restored)
- BuildingSystem calls `set_zone_state()` on build/demolish
- Stalled state driven by road proximity check
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z01, 4-Z05
- Blocks: None

### Group F: ZoneSystem Core

**4-Z12: ZoneSystem Class Skeleton and ISimulatable**
- Type: infrastructure
- Scope: M
- ZoneSystem class definition implementing ISimulatable (priority 30)
- Constructor takes ITerrainQueryable*, ITransportAccessible*
- tick() method orchestrates demand update and desirability refresh
- Query method declarations (all from Section 7)
- File: `src/simulation/zones/ZoneSystem.h`, `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z01
- Blocks: 4-Z04, 4-Z05, 4-Z06, 4-Z08, 4-Z10, 4-Z11

### Group G: Network and Events

**4-Z13: Zone Network Messages**
- Type: feature
- Scope: M
- Define ZonePlacementRequestMsg, DezoneRequestMsg, RedesignateRequestMsg
- Implement ISerializable for all messages
- Server-side message handler dispatches to ZoneSystem methods
- Result messages back to requesting client
- File: `src/network/messages/ZoneMessages.h`
- Blocked by: 4-Z05, 4-Z06, Epic 1 ISerializable
- Blocks: 4-Z14

**4-Z14: Zone Server-Authoritative Validation Pipeline**
- Type: feature
- Scope: M
- Server receives zone messages, validates, applies, broadcasts
- Ownership check per-tile for multi-tile requests
- Concurrent request handling (serialized per tick)
- State delta broadcast to all clients via SyncSystem
- File: `src/simulation/zones/ZoneSystem.cpp` (server-side handler)
- Blocked by: 4-Z13
- Blocks: None

### Group H: Events and Integration

**4-Z15: Zone Events**
- Type: infrastructure
- Scope: S
- Define event structs:
  - `ZoneDesignatedEvent { EntityID zone, GridPosition pos, ZoneType type, ZoneDensity density, PlayerID owner }`
  - `ZoneUndesignatedEvent { EntityID zone, GridPosition pos, ZoneType type, PlayerID owner }`
  - `ZoneStateChangedEvent { EntityID zone, GridPosition pos, ZoneState old_state, ZoneState new_state }`
  - `ZoneDemandChangedEvent { PlayerID player, ZoneDemandData demand }`
- File: `src/simulation/zones/ZoneEvents.h`
- Blocked by: 4-Z01
- Blocks: 4-Z05, 4-Z06, 4-Z11

**4-Z16: Zone Serialization**
- Type: feature
- Scope: M
- ISerializable for ZoneComponent
- ZoneGrid serialization for save/load (EntityID array + metadata)
- ZoneCounts serialization
- Integration with Epic 1 SyncSystem for component sync
- File: `src/simulation/zones/ZoneSystem.cpp`
- Blocked by: 4-Z01, 4-Z02, Epic 1 ISerializable
- Blocks: None

### Summary Table

| ID | Name | Scope | Group | Blocks |
|----|------|-------|-------|--------|
| 4-Z01 | Zone Type and Component Definitions | S | A | Z02,Z03,Z04,Z05,Z06,Z07,Z08,Z11,Z12,Z15 |
| 4-Z02 | ZoneGrid Spatial Index | M | A | Z04,Z05,Z06,Z10 |
| 4-Z03 | ITransportAccessible Stub | S | B | Z04 |
| 4-Z04 | Zone Placement Validation | L | B | Z05,Z10 |
| 4-Z05 | Zone Placement Execution | M | B | Z10,Z13 |
| 4-Z06 | De-zoning Implementation | M | C | Z07 |
| 4-Z07 | Redesignation | S | C | -- |
| 4-Z08 | Basic Demand Calculation | L | D | Z09 |
| 4-Z09 | Demand Factor Extension Points | S | D | -- |
| 4-Z10 | Zone Desirability Calculation | M | E | -- |
| 4-Z11 | Zone State Management | M | E | -- |
| 4-Z12 | ZoneSystem Class Skeleton | M | F | Z04,Z05,Z06,Z08,Z10,Z11 |
| 4-Z13 | Zone Network Messages | M | G | Z14 |
| 4-Z14 | Zone Server-Authoritative Validation | M | G | -- |
| 4-Z15 | Zone Events | S | H | Z05,Z06,Z11 |
| 4-Z16 | Zone Serialization | M | H | -- |

### Critical Path

```
4-Z01 (S) -> 4-Z12 (M) -> 4-Z02 (M) -> 4-Z04 (L) -> 4-Z05 (M) -> 4-Z13 (M) -> 4-Z14 (M)
                                            ^
                                            |
                              4-Z03 (S) ----+
                              (parallel)

Parallel track: 4-Z01 -> 4-Z08 (L) -> 4-Z09 (S)
Parallel track: 4-Z01 -> 4-Z15 (S)
```

**Minimum path to "zones placeable on screen":**
1. 4-Z01 -- type definitions
2. 4-Z12 -- system skeleton
3. 4-Z02 -- spatial index
4. 4-Z03 -- transport stub
5. 4-Z04 -- placement validation
6. 4-Z05 -- placement execution
7. 4-Z15 -- events (parallel with 4-Z05)

---

## 11. Questions for Other Agents

### @systems-architect

1. **Zone entity lifecycle and ECS registry:** When a zone is de-zoned, should the entity be destroyed immediately or recycled? Is there an entity pool pattern we should follow to avoid fragmentation in the ECS registry?

2. **ZoneGrid vs. IGridQueryable:** The ZoneGrid spatial index is functionally similar to IGridQueryable from interfaces.yaml. Should ZoneSystem implement IGridQueryable, or is ZoneGrid a private implementation detail with ZoneSystem exposing purpose-specific methods?

3. **Cross-player tile queries:** When rendering zone overlays, the client needs to know all zones on the map (not just the local player's). Should zone entities for all players be synced to all clients, or only the local player's zones? Canon says all component changes sync -- confirming this means all zones are visible to all clients.

4. **Demand calculation ownership:** Canon systems.yaml has DemandSystem in Epic 10 as a separate system. But ZoneSystem (Epic 4) "owns zone demand calculation (basic)." How should we handle the handoff? Should ZoneSystem compute demand and DemandSystem override it later? Or should we design demand as a pluggable provider from the start?

### @building-engineer

5. **Zone-to-building interface:** When BuildingSystem places a building on a zone, it needs to call `ZoneSystem::set_zone_state(x, y, ZoneState::Occupied)`. Conversely, when a building is demolished, it calls `set_zone_state(x, y, ZoneState::Designated)`. Does this bidirectional dependency work for you, or should we use events instead?

6. **De-zone with building:** When a player de-zones an occupied tile, should ZoneSystem directly call BuildingSystem to demolish, or emit a `DemolitionRequestEvent` that BuildingSystem handles? The event approach is cleaner for decoupling.

7. **Building template density field:** Canon `building_templates` has `density: "low | high"`. Confirming that BuildingSystem selects templates based on `ZoneComponent.density`. Is there any scenario where a building density differs from the zone density?

### @game-designer

8. **Zone designation cost:** I proposed 2 credits/tile for low density, 5 credits/tile for high density. Is this the right ballpark? Should designation be free to reduce friction in early game?

9. **Redesignation workflow:** Should players be able to change zone type without de-zoning first (if the tile is empty/Designated state)? This is more convenient but might confuse the UI.

10. **Stalled zones display:** How should stalled zones (designated but no road access) appear in the UI? Different overlay color? Flashing? Icon? This affects what data ZoneSystem needs to expose.

11. **Density upgrade without de-zone:** Should players be able to upgrade an empty zone's density (low -> high) without de-zoning and re-zoning? What about downgrade?

### @infrastructure-engineer

12. **Road proximity rule detail:** The canon says "3 tiles" for road proximity. Is this Manhattan distance, Euclidean distance, or Chebyshev distance? Manhattan distance of 3 means a diamond shape; Chebyshev distance of 3 means a 7x7 square. The implementation is trivially different but the gameplay effect matters.

13. **ITransportAccessible interface:** I defined a stub interface for road proximity checking (Section 8). Does this interface shape work for TransportSystem's eventual implementation? Should I add more methods?

### @graphics-engineer

14. **Zone overlay rendering:** ZoneSystem provides zone type at each position. How does the rendering system want this data -- per-entity query, or a dense grid of zone types for overlay rendering? The ZoneGrid spatial index stores EntityIDs; rendering might prefer a color-coded type grid.

15. **Stalled zone visual:** Should stalled zones (no road access) have a distinct visual treatment in the zone overlay? If so, ZoneSystem needs to expose the stalled state per tile.

---

## 12. Risks & Concerns

### Performance Risks

**Risk: Zone entity count on large maps**
- Severity: LOW
- A fully-zoned 512x512 map would have 262,144 zone entities -- unlikely but theoretically possible.
- At ~50 bytes per entity, this is ~13 MB. Acceptable.
- ECS iteration over all zone entities for type queries: O(n_zones). With 50K+ zones, this could take microseconds to low milliseconds.
- **Mitigation:** Cached ZoneCounts avoid iterating for aggregate queries. ECS archetype storage keeps ZoneComponent tightly packed for fast iteration.

**Risk: ZoneGrid memory for spatial index**
- Severity: VERY LOW
- 1 MB for 512x512. Trivial.

**Risk: Desirability recalculation cost**
- Severity: LOW
- Recalculating desirability for all zone tiles every N ticks.
- With 50K zones at ~10 operations per zone = 500K operations every 10 ticks = 50K operations per tick.
- At ~10ns per operation = 0.5ms per tick. Acceptable at 50ms tick budget.
- **Mitigation:** Update desirability every 10 ticks (500ms) instead of every tick (50ms). Desirability changes slowly.

### Design Risks

**Risk: Demand calculation complexity**
- Severity: MEDIUM
- The basic demand calculation in Epic 4 is straightforward (stub factors).
- When DemandSystem (Epic 10) takes over, integrating real PopulationSystem, EconomySystem, and TransportSystem factors could make demand unstable or oscillate.
- **Mitigation:** Design demand with damping (smoothing). Use running average rather than instantaneous calculation. Expose demand factor breakdown for debugging.

**Risk: Demand-building feedback loop**
- Severity: MEDIUM
- Positive demand -> buildings spawn -> population grows -> more demand -> more buildings. This can spiral.
- Negative demand -> buildings abandon -> population drops -> less demand -> more abandonment. This can crash.
- **Mitigation:** Damping, hard caps on demand change rate per tick, hysteresis (different thresholds for growth vs. decline).

### Forward Dependency Risks

**Risk: TransportSystem stub (Epic 7) makes zones too easy**
- Severity: LOW-MEDIUM
- Without road proximity requirements, all zones develop freely. Players building during Epic 4 testing will not learn the road-first workflow.
- When Epic 7 arrives, existing cities may have zones far from roads that suddenly become stalled.
- **Mitigation:** When Epic 7 activates, existing zones should get a grace period before stalling. Or Epic 7 retroactively validates existing zones and marks stalled ones. Document this as a known transition issue.

**Risk: EnergySystem/FluidSystem stubs (Epics 5/6)**
- Severity: LOW
- Buildings require power and water to develop. Without these systems, demand and building development need stubs.
- **Mitigation:** Same pattern as transport stub: stub interfaces that return "utilities available." BuildingSystem handles the utility integration, not ZoneSystem.

### Multiplayer Risks

**Risk: Zone sync volume**
- Severity: LOW
- Zone creation produces entity + component deltas via SyncSystem.
- A large drag-zone operation (e.g., 100 tiles) creates 100 entities in one tick.
- At ~50 bytes per entity sync = 5 KB per 100-tile operation. Trivial network cost.
- **Mitigation:** None needed. SyncSystem handles batching.

**Risk: Concurrent zone + purchase race condition**
- Severity: LOW
- Player A purchases tile from GAME_MASTER. Player B tries to zone that tile (which they do not own). Server validates ownership and rejects Player B.
- Edge case: Player A purchases tile AND zones it in the same tick. Purchase must complete before zone validation runs.
- **Mitigation:** Server processes purchase messages before zone messages within a tick. Or: zone validation reads current ownership state (which includes just-completed purchases if processed first).

---

## Appendix A: Terminology Quick Reference

| Human Term | Canon Term | Used In |
|-----------|-----------|---------|
| Residential | Habitation | Zone type |
| Commercial | Exchange | Zone type |
| Industrial | Fabrication | Zone type |
| Zoned | Designated | Zone state |
| Unzoned | Undesignated | Zone state |
| Rezoning | Redesignation | Zone action |
| RCI Demand | Zone Pressure | Demand metric |
| Land value | Sector value | Desirability factor |
| Road | Pathway | Transport dependency |
| Building | Structure | What develops on zones |
| Citizen | Being | Population unit |
| Tile | Sector | Grid unit (player-facing) |

## Appendix B: File Locations

```
src/
  components/
    ZoneComponent.h              # ZoneComponent, ZoneType, ZoneDensity, ZoneState enums
  simulation/
    zones/
      ZoneSystem.h               # ZoneSystem class declaration
      ZoneSystem.cpp             # ZoneSystem implementation
      ZoneGrid.h                 # ZoneGrid spatial index
      ZoneGrid.cpp               # ZoneGrid implementation
      ZoneDemand.h               # Demand calculation types and interface
      ZoneDemand.cpp             # Demand calculation implementation
      ZoneEvents.h               # Zone event structs
      ITransportAccessible.h     # Stub interface for road proximity
  network/
    messages/
      ZoneMessages.h             # Network message definitions for zone operations
```

## Appendix C: Dependency Map

```
Epic 3 (TerrainSystem)
  ITerrainQueryable.is_buildable() -----> 4-Z04 (placement validation)

Epic 1 (Multiplayer)
  ISerializable -----> 4-Z13 (network messages)
  SyncSystem -----> 4-Z14 (state broadcast)

Epic 7 (TransportSystem) [FORWARD DEPENDENCY]
  ITransportAccessible -----> 4-Z03 (stub), 4-Z04 (validation)
                               Replaced by real implementation in Epic 7

Epic 4 BuildingSystem (Building Engineer's scope)
  <---- reads: get_designated_zones(), get_zone_type_at(), get_zone_density_at()
  ----> writes: set_zone_state()

Epic 10 DemandSystem
  <---- reads: get_zone_counts(), get_zone_demand()
  ----> may override demand calculation
```

---

*End of Zone Engineer Analysis*
