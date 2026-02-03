# Epic 4: Zoning & Building System -- Systems Architect Analysis

**Author:** Systems Architect Agent
**Date:** 2026-01-28
**Canon Version:** 0.6.0
**Status:** ANALYSIS COMPLETE -- Ready for ticket breakdown

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Data Flow](#3-data-flow)
4. [Key Work Items](#4-key-work-items)
5. [Interface Contracts](#5-interface-contracts)
6. [Multiplayer Implications](#6-multiplayer-implications)
7. [Cross-Epic Dependencies](#7-cross-epic-dependencies)
8. [Building Template System](#8-building-template-system)
9. [Questions for Other Agents](#9-questions-for-other-agents)
10. [Risks & Concerns](#10-risks--concerns)

---

## 1. System Boundaries

### 1.1 ZoneSystem

**Owns:**

- Zone designation onto tiles (habitation, exchange, fabrication)
- Zone density levels per tile (low, high)
- Basic zone demand calculation (growth pressure per zone type)
- Zone removal (redesignation / undesignation)
- Zone placement validation (terrain buildability, ownership, not already zoned)
- Zone grid storage (which tiles are zoned and how)

**Does NOT Own:**

- Buildings placed within zones (BuildingSystem owns)
- Zone rendering / overlay visualization (RenderingSystem owns, Epic 12 provides UI overlay)
- Full demand simulation with population/economy feedback loops (DemandSystem in Epic 10 owns)
- Terrain data or buildability logic (TerrainSystem owns, ZoneSystem queries ITerrainQueryable)
- Land ownership validation beyond query (OwnershipComponent, cross-cutting concern)

**Provides:**

- `IZoneQueryable` -- zone type and density at any grid position
- Zone demand values (basic R/C/I pressure -- a simplified version that BuildingSystem consumes before the full DemandSystem exists in Epic 10)
- `ZoneDesignatedEvent` / `ZoneRemovedEvent` for BuildingSystem and other consumers
- Batch zone queries for area operations (e.g., "count of habitation tiles owned by player X")

**Depends On:**

- `ITerrainQueryable` (Epic 3) -- is_buildable() check before allowing zone placement
- `OwnershipComponent` (cross-cutting) -- only tile owner can zone

### 1.2 BuildingSystem

**Owns:**

- Building spawning logic (when zones meet conditions, spawn a building)
- Building state machine (materializing -> active -> abandoned/derelict; upgrades/downgrades)
- Building variety and template selection (which template to use for a given zone tile)
- Building upgrades (density increase triggers model swap) and downgrades (conditions deteriorate)
- Demolition of buildings (player-initiated deconstruction)
- Building footprint management (multi-tile buildings occupy a GridRect)
- Construction progress tracking (ticks remaining for materializing state)
- Building health (for disaster damage, Epic 13)

**Does NOT Own:**

- Zone designation (ZoneSystem owns -- BuildingSystem reacts to zone state)
- Energy/power state (EnergySystem owns in Epic 5; BuildingSystem queries stub)
- Fluid/water state (FluidSystem owns in Epic 6; BuildingSystem queries stub)
- Road proximity calculation (TransportSystem owns in Epic 7; BuildingSystem queries stub)
- Building rendering / 3D models (RenderingSystem owns)
- Population or employment (PopulationSystem in Epic 10 owns)
- Tax or maintenance costs (EconomySystem in Epic 11 owns)
- Land value calculation (LandValueSystem in Epic 10 owns)

**Provides:**

- `IBuildingQueryable` -- building type, state, template ID, density at any position
- `IBuildable` implementation -- construction cost, time, footprint, placement validation
- `IDemolishable` implementation -- demolition cost, cleanup
- `BuildingConstructedEvent` / `BuildingDemolishedEvent` / `BuildingStateChangedEvent`
- Building entity creation (with BuildingComponent, ConstructionComponent, PositionComponent, OwnershipComponent)
- Contamination source data for fabrication buildings (IContaminationSource implementation)

**Depends On:**

- ZoneSystem (zone state triggers building spawning)
- `ITerrainQueryable` (Epic 3) -- terrain type affects build cost, elevation for placement
- `IEnergyConsumer` stub (Epic 5 forward dependency) -- buildings need power to remain active
- `IFluidConsumer` stub (Epic 6 forward dependency) -- buildings need water to develop
- `ITransportConnectable` stub (Epic 7 forward dependency) -- 3-tile road proximity rule
- `OwnershipComponent` (cross-cutting) -- buildings inherit tile owner

### 1.3 Boundary Between ZoneSystem and BuildingSystem

This is a critical architectural boundary. The separation follows this principle:

| Concern | Owner | Rationale |
|---------|-------|-----------|
| "What type of development should happen here?" | ZoneSystem | Player intent (zone designation) |
| "Does a building actually appear here?" | BuildingSystem | Simulation logic (conditions met) |
| "What does the zone demand look like?" | ZoneSystem (basic), DemandSystem (Epic 10, full) | Zone pressure drives spawning |
| "Which template was selected?" | BuildingSystem | Building variety logic |
| "Is this tile buildable terrain?" | TerrainSystem (queried by ZoneSystem) | Terrain is the foundation |
| "Is this tile zoned?" | ZoneSystem (queried by BuildingSystem) | Zone is a precondition for building |

**The handoff:** ZoneSystem designates tiles. BuildingSystem polls zoned tiles each tick and, when conditions are met (zone exists, no building yet, demand is positive, utilities are connected or stubbed as available), it spawns a building entity. ZoneSystem does NOT call BuildingSystem directly -- the relationship is data-driven through component queries and events.

---

## 2. Data Model

### 2.1 ZoneComponent

```cpp
struct ZoneComponent {
    ZoneType zone_type = ZoneType::None;    // habitation, exchange, fabrication, none
    ZoneDensity density = ZoneDensity::Low;  // low, high
    uint8_t demand_weight = 0;               // local demand factor (0-255)
    // Note: zone_type is per-tile. A zone is NOT a region entity --
    // each tile independently stores its zone designation.
};

enum class ZoneType : uint8_t {
    None = 0,
    Habitation = 1,    // residential
    Exchange = 2,      // commercial
    Fabrication = 3    // industrial
};

enum class ZoneDensity : uint8_t {
    Low = 0,
    High = 1
};
```

**Storage approach:** ZoneComponent is attached to tile entities (or stored in a dense zone grid, analogous to TerrainGrid). Since zones are per-tile and potentially cover large areas, a dense grid may be more efficient than per-entity ECS for queries like "count all habitation tiles." However, unlike terrain (which covers every tile), zones are sparse (most tiles are unzoned). **Recommendation: Use standard ECS registration.** Entities with ZoneComponent are sparse -- only zoned tiles have them. A spatial index provides fast positional lookups.

**Alternative under consideration:** If zone coverage exceeds ~50% of tiles in typical late-game scenarios, a dense grid would be warranted (same rationale as TerrainGrid). But zones are typically 20-40% of map area, so ECS is appropriate.

### 2.2 BuildingComponent

```cpp
struct BuildingComponent {
    BuildingType building_type = BuildingType::Zone; // zone, service, infrastructure, landmark
    ZoneType zone_type = ZoneType::None;             // which zone spawned this (None for non-zone buildings)
    ZoneDensity density = ZoneDensity::Low;          // low or high density
    uint16_t template_id = 0;                        // index into building template pool
    BuildingState state = BuildingState::Materializing;
    uint16_t health = 100;                           // current health (for disasters, Epic 13)
    uint16_t max_health = 100;                       // max health
    uint8_t level = 1;                               // current building level (1 = base)
    uint8_t max_level = 1;                           // max level for this template
    bool has_power = false;                          // cached from EnergySystem (or stub)
    bool has_fluid = false;                          // cached from FluidSystem (or stub)
    bool is_road_accessible = false;                 // cached from TransportSystem (or stub)
    uint8_t abandonment_ticks = 0;                   // ticks without utilities before abandonment
};

enum class BuildingState : uint8_t {
    Materializing = 0,   // under construction
    Active = 1,          // functioning normally
    Degraded = 2,        // missing utilities, will abandon if not restored
    Abandoned = 3,       // derelict -- no longer functioning
    Demolishing = 4      // being torn down
};

enum class BuildingType : uint8_t {
    Zone = 0,            // spawned by zone system (habitation, exchange, fabrication)
    Service = 1,         // service buildings (Epic 9)
    Infrastructure = 2,  // power plants, water pumps, etc.
    Landmark = 3         // special reward buildings (Epic 14)
};
```

### 2.3 ConstructionComponent

```cpp
struct ConstructionComponent {
    uint32_t ticks_remaining = 0;    // ticks until construction complete
    uint32_t total_ticks = 0;        // total construction time (for progress bar)
    float progress = 0.0f;           // derived: 1.0 - (ticks_remaining / total_ticks)
    // Note: ConstructionComponent is removed when construction completes.
    // It is a transient component.
};
```

**Lifecycle:** ConstructionComponent is added when a building entity is created and removed when `ticks_remaining` reaches 0. BuildingState transitions from `Materializing` to `Active` at the same time.

### 2.4 Building Template Data Structure

```cpp
struct BuildingTemplate {
    uint16_t template_id;                      // unique ID
    ZoneType zone_type;                        // habitation, exchange, fabrication
    ZoneDensity density;                       // low, high
    ModelSource model_source;                  // procedural or asset
    std::string model_path;                    // path to .glb (if asset)
    GridRect footprint;                        // tiles occupied (most zone buildings are 1x1)
    uint32_t construction_cost;                // credits to build
    uint32_t construction_ticks;               // ticks to materialize
    uint32_t min_land_value;                   // minimum sector value to appear
    uint32_t max_land_value;                   // maximum sector value (0 = no cap)
    uint8_t max_level;                         // max upgrade level
    uint32_t energy_required;                  // energy per tick when active
    uint32_t fluid_required;                   // fluid per tick when active
    uint32_t contamination_output;             // contamination per tick (fabrication only)
    uint32_t population_capacity;              // beings this building holds (habitation)
    uint32_t job_capacity;                     // jobs provided (exchange, fabrication)
    uint16_t selection_weight;                 // relative selection probability
    ProceduralVariation variation;             // allowed randomization
};

struct ProceduralVariation {
    bool allow_rotation = true;                // random Y-axis rotation (0, 90, 180, 270)
    float scale_min = 0.95f;                   // minimum scale factor
    float scale_max = 1.05f;                   // maximum scale factor
    uint8_t color_accent_variants = 4;         // number of color accent options
};

enum class ModelSource : uint8_t {
    Procedural = 0,
    Asset = 1
};
```

### 2.5 Zone Grid Storage Approach

**Decision: Standard ECS with spatial index.**

Zone data is stored as ZoneComponent attached to entities that also have PositionComponent and OwnershipComponent. A spatial hash/grid index (from the cross-cutting SpatialIndex utility) allows O(1) lookup of "what zone is at position (x, y)?"

Rationale:
- Zones are sparse (20-40% of tiles in a mature colony)
- Zone operations are infrequent (player designates, not per-tick)
- BuildingSystem needs to iterate "all zoned tiles without buildings" -- component query is natural for this
- Unlike terrain (every tile has data), zones are the exception, not the rule

If profiling shows zone queries are a bottleneck (unlikely given zone operations are player-driven, not simulation-driven), we can add a dense zone overlay grid later without changing the interface.

---

## 3. Data Flow

### 3.1 Data Flow Diagram

```
                              EPIC 3
                         +---------------+
                         | TerrainSystem |
                         | (ITerrainQry) |
                         +-------+-------+
                                 |
                    is_buildable(), get_elevation(),
                    get_build_cost_modifier()
                                 |
                                 v
+----------------+     +-----------------+     +-------------------+
|   Player Input | --> |   ZoneSystem    | --> |  BuildingSystem   |
| (zone/demolish)|     |                 |     |                   |
+----------------+     | - validates     |     | - polls zones     |
                       | - designates    |     | - spawns buildings|
                       | - calculates    |     | - manages states  |
                       |   basic demand  |     | - handles demo    |
                       +---------+-------+     +---------+---------+
                                 |                       |
                    ZoneDesignatedEvent         BuildingConstructedEvent
                    ZoneRemovedEvent            BuildingDemolishedEvent
                    IZoneQueryable              BuildingStateChangedEvent
                    zone demand values          IBuildingQueryable
                                 |                       |
                    +------------+------------+          |
                    |            |            |          |
                    v            v            v          v
              [Epic 10]    [Epic 10]    [Epic 12]  [Epic 5,6,7,9,10,11,13]
             DemandSys   PopulationSys   UISys    (downstream consumers)


FORWARD DEPENDENCY STUBS (Epic 4 defines, later epics implement):
    +-------------------+
    | IEnergyConsumer   |  <-- BuildingSystem queries; EnergySystem (Epic 5) answers
    | (stub: always ON) |      Stub always returns has_power = true
    +-------------------+

    +-------------------+
    | IFluidConsumer    |  <-- BuildingSystem queries; FluidSystem (Epic 6) answers
    | (stub: always ON) |      Stub always returns has_fluid = true
    +-------------------+

    +-------------------+
    | ITransportConnect |  <-- BuildingSystem queries; TransportSystem (Epic 7) answers
    | (stub: always OK) |      Stub always returns is_road_accessible = true
    +-------------------+
```

### 3.2 Data Producers and Consumers

#### ZoneSystem Produces:

| Data | Consumer(s) | Frequency |
|------|-------------|-----------|
| ZoneComponent on entities | BuildingSystem (polls for spawning) | On zone placement |
| ZoneDesignatedEvent | BuildingSystem, UISystem, DemandSystem | On zone action |
| ZoneRemovedEvent | BuildingSystem (removes building?), UISystem | On unzone action |
| IZoneQueryable (zone at position) | BuildingSystem, UISystem, DemandSystem, LandValueSystem | Per query |
| Basic demand values (R/C/I) | BuildingSystem (spawn gating) | Per tick |
| Zone count stats (per type, per player) | UISystem, DemandSystem | Per tick or on change |

#### ZoneSystem Consumes:

| Data | Source | Frequency |
|------|--------|-----------|
| is_buildable(x, y) | ITerrainQueryable (Epic 3) | On zone placement |
| get_build_cost_modifier(x, y) | ITerrainQueryable (Epic 3) | On zone placement |
| OwnershipComponent | Cross-cutting | On zone placement |
| Player input (zone tool) | InputSystem / NetworkManager | On player action |

#### BuildingSystem Produces:

| Data | Consumer(s) | Frequency |
|------|-------------|-----------|
| BuildingComponent on entities | PopulationSystem, EconomySystem, DisorderSystem, ContaminationSystem, ServicesSystem, UISystem | Persistent |
| BuildingConstructedEvent | UISystem, AudioSystem, EconomySystem | On construction complete |
| BuildingDemolishedEvent | UISystem, AudioSystem, EconomySystem | On demolition |
| BuildingStateChangedEvent | UISystem, EnergySystem, FluidSystem | On state transition |
| IBuildingQueryable (building at position) | All downstream systems | Per query |
| IContaminationSource (fabrication) | ContaminationSystem (Epic 10) | Per tick |
| IBuildable / IDemolishable | UISystem (cost preview), EconomySystem | On query |

#### BuildingSystem Consumes:

| Data | Source | Frequency |
|------|--------|-----------|
| ZoneComponent (zoned tiles) | ZoneSystem | Per tick (polling) |
| ZoneDesignatedEvent | ZoneSystem | On zone action |
| ITerrainQueryable | TerrainSystem (Epic 3) | On building spawn |
| has_power (stub: true) | IEnergyConsumer stub (Epic 5 forward dep) | Per tick |
| has_fluid (stub: true) | IFluidConsumer stub (Epic 6 forward dep) | Per tick |
| is_road_accessible (stub: true) | ITransportConnectable stub (Epic 7 forward dep) | Per tick |
| land_value (stub: 50) | LandValueSystem stub (Epic 10 forward dep) | Per tick (template selection) |
| Player input (demolish tool) | InputSystem / NetworkManager | On player action |
| OwnershipComponent | Cross-cutting | On spawn / demolition |

### 3.3 Forward-Dependency Stubs Needed

Epic 4 depends on systems that do not exist yet. We must define stub interfaces that return safe defaults, to be replaced by real implementations in later epics.

| Stub | Real System (Epic) | Default Behavior | Why Needed |
|------|--------------------|------------------|------------|
| `IEnergyQueryStub` | EnergySystem (Epic 5) | `has_power() -> true` | Buildings need power to stay active; without stub, all buildings abandon |
| `IFluidQueryStub` | FluidSystem (Epic 6) | `has_fluid() -> true` | Buildings need water to develop; without stub, no development |
| `ITransportQueryStub` | TransportSystem (Epic 7) | `is_road_accessible() -> true` | 3-tile road proximity rule; without stub, no buildings can spawn |
| `ILandValueQueryStub` | LandValueSystem (Epic 10) | `get_land_value() -> 50` | Template selection based on sector value; stub provides mid-range value |
| `IDemandQueryStub` | DemandSystem (Epic 10) | `get_demand(type) -> 100` | Full demand calculation depends on population/economy; stub provides positive demand |

**Stub pattern:**

```cpp
// Interface
class IEnergyQuery {
public:
    virtual ~IEnergyQuery() = default;
    virtual bool has_power(EntityID building) const = 0;
    virtual uint32_t get_energy_available(PlayerID player) const = 0;
};

// Stub (used until Epic 5 provides real implementation)
class EnergyQueryStub : public IEnergyQuery {
public:
    bool has_power(EntityID) const override { return true; }
    uint32_t get_energy_available(PlayerID) const override { return UINT32_MAX; }
};
```

This pattern allows seamless replacement: when Epic 5 is implemented, we swap the stub for the real EnergySystem without changing BuildingSystem code.

---

## 4. Key Work Items

### 4.1 ZoneSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| Z-01 | ZoneType and ZoneDensity enums | S | Define ZoneType (None/Habitation/Exchange/Fabrication) and ZoneDensity (Low/High) enums. Use canonical alien terminology in documentation. |
| Z-02 | ZoneComponent definition | S | 3-byte component: zone_type (u8), density (u8), demand_weight (u8). Trivially copyable, serializable. |
| Z-03 | Zone placement validation | M | Validate zone placement: terrain is_buildable, tile owned by requesting player, tile not already zoned, tile has no existing building. Uses ITerrainQueryable and OwnershipComponent. |
| Z-04 | Zone designation action | M | Apply zone designation to tile: create entity with ZoneComponent + PositionComponent + OwnershipComponent (inherited from tile owner). Emit ZoneDesignatedEvent. Server-authoritative. |
| Z-05 | Zone removal action | M | Remove zone from tile: remove ZoneComponent. If building exists on tile, either demolish or leave as-is (design decision needed). Emit ZoneRemovedEvent. Server-authoritative. |
| Z-06 | Zone area operations (drag-zone) | M | Support rectangular area zone designation (drag tool). Iterate all tiles in rect, validate each, zone valid ones. Single network message for area. |
| Z-07 | IZoneQueryable interface | M | Define and implement zone query interface: get_zone_type(x,y), get_zone_density(x,y), is_zoned(x,y), get_zone_count(player, type), get_zoned_tiles_without_buildings(player). |
| Z-08 | Basic demand calculation | L | Calculate basic zone demand (growth pressure). Simple formula: demand = base_demand - existing_zone_count * saturation_factor. Per zone type. Per player. This is a simplified version; Epic 10 DemandSystem replaces it with full population/economy feedback. |
| Z-09 | Zone density upgrade trigger | M | When conditions are met (land value exceeds threshold, services present -- stubbed for now), upgrade zone density from Low to High. Emits ZoneDensityChangedEvent. This triggers BuildingSystem to potentially upgrade/replace the building. |
| Z-10 | Zone network messages | M | ZoneDesignateRequest, ZoneRemoveRequest, ZoneBatchDesignateRequest messages. Server validates, applies, broadcasts. ISerializable implementation. |
| Z-11 | ZoneComponent serialization | S | ISerializable for ZoneComponent. Fixed-size, little-endian. Version field for forward compatibility. |

### 4.2 BuildingSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| B-01 | BuildingComponent definition | S | Component as specified in Data Model section. Trivially copyable, serializable. |
| B-02 | ConstructionComponent definition | S | Transient component tracking construction progress. Removed on completion. |
| B-03 | BuildingTemplate data structure | M | Template struct with all fields. Static template registry loaded from data file or hardcoded initial set. |
| B-04 | Building template pool and registry | L | Template pool organized by zone_type + density. Selection API: get_templates(zone_type, density, land_value) returns weighted candidate list. Initial template set: 3-5 per zone type per density = 18-30 templates minimum. |
| B-05 | Building template selection algorithm | L | Select template from pool: filter by zone_type + density + land_value range, apply selection weights, avoid adjacent duplicate check, apply procedural variation (rotation, scale, color accent). Uses seeded PRNG for deterministic selection (seed = hash(tile_x, tile_y, simulation_tick)). |
| B-06 | Building spawn logic | XL | Core spawning system. Each tick, iterate zoned tiles without buildings. Check conditions: demand > 0, utilities connected (stub: true), road accessible (stub: true). If conditions met, select template, create building entity with BuildingComponent + ConstructionComponent + PositionComponent + OwnershipComponent. Stagger spawning: max N buildings spawned per tick to avoid simulation spikes. |
| B-07 | Building state machine | L | State transitions: Materializing -> Active (construction complete), Active -> Degraded (missing utilities for N ticks), Degraded -> Abandoned (still missing after M ticks), Abandoned -> Active (utilities restored within grace period), Any -> Demolishing (player action). Each state affects visual appearance via RenderingSystem. |
| B-08 | Construction progress system | M | Each tick, decrement ConstructionComponent.ticks_remaining. When 0: remove ConstructionComponent, set BuildingState to Active, emit BuildingConstructedEvent. Update progress float for UI. |
| B-09 | Building demolition | M | Player-initiated demolition. Validate: player owns tile, building exists. Set state to Demolishing, after demolition_ticks remove building entity. Emit BuildingDemolishedEvent. Free the zone tile for new development. Cost from IDemolishable. |
| B-10 | Building upgrade/downgrade | L | When zone density increases (Low -> High), building may upgrade: select new template at higher density, enter Materializing state for upgrade construction. When conditions deteriorate (land value drops, demand drops), building may downgrade: select lower-density template. Upgrade/downgrade creates a visual transition. |
| B-11 | IBuildable interface implementation | M | Implement IBuildable for buildings: get_construction_cost(), get_construction_time(), get_footprint(), can_build_at(). Uses template data. Validates terrain and ownership. |
| B-12 | IDemolishable interface implementation | S | Implement IDemolishable: get_demolition_cost(), on_demolished(). Demolition cost is typically 0 for zone buildings, non-zero for infrastructure. |
| B-13 | IBuildingQueryable interface | M | Define and implement: get_building_at(x,y), get_building_state(entity), get_building_type(entity), get_buildings_in_rect(rect), get_building_count(player, zone_type), is_position_occupied(x,y). |
| B-14 | IContaminationSource for fabrication | S | Fabrication buildings implement IContaminationSource: get_contamination_output() returns template's contamination value. get_contamination_type() returns ContaminationType::Industrial. |
| B-15 | Forward dependency stub interfaces | M | Define IEnergyQuery, IFluidQuery, ITransportQuery, ILandValueQuery, IDemandQuery stubs. All return "everything is fine" defaults. Register as injectable dependencies so real implementations can replace them. |
| B-16 | Building network messages | M | BuildingDemolishRequest message. Building spawning is server-side only (no client request needed -- server decides when conditions are met). BuildingConstructedEvent, BuildingDemolishedEvent, BuildingStateChangedEvent sync. |
| B-17 | BuildingComponent serialization | S | ISerializable for BuildingComponent and ConstructionComponent. Fixed-size, little-endian, versioned. |
| B-18 | Multi-tile building footprint | M | Support buildings with footprint > 1x1 (e.g., high-density buildings at 2x2). Primary tile owns the entity. Adjacent tiles reference the primary tile's entity. Footprint validation ensures no overlap. |
| B-19 | Building abandonment and decay | M | When utilities are missing for configurable ticks, building transitions to Degraded, then Abandoned. Abandoned buildings stop contributing to population/economy. Visual decay (RenderingSystem reflects state). Ties into ghost town process for player abandonment. |
| B-20 | ISimulatable implementation for both systems | M | ZoneSystem.tick() at priority 30, BuildingSystem.tick() at priority 40. Ensure correct execution order: zones process before buildings each tick. |

### 4.3 Shared / Infrastructure Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| S-01 | Event definitions | S | ZoneDesignatedEvent, ZoneRemovedEvent, ZoneDensityChangedEvent, BuildingConstructedEvent, BuildingDemolishedEvent, BuildingStateChangedEvent. All follow canon event pattern. |
| S-02 | Spatial index integration | M | Ensure ZoneSystem and BuildingSystem entities are registered in SpatialIndex for O(1) positional lookups. Zone and building queries use spatial index. |
| S-03 | Building template data file format | M | Define the file format for building template definitions (JSON, YAML, or binary). Initial set of templates for MVP. Template loading at game startup. |
| S-04 | Integration test: zone-to-building pipeline | L | End-to-end test: zone a tile, verify zone entity created, wait for building spawn conditions, verify building entity created with correct template, verify construction progress, verify state transition to Active. |

---

## 5. Interface Contracts

### 5.1 Interfaces Epic 4 Implements

#### IBuildable (defined in interfaces.yaml)

```cpp
class IBuildable {
public:
    virtual uint32_t get_construction_cost() const = 0;
    virtual uint32_t get_construction_time() const = 0;  // ticks
    virtual GridRect get_footprint() const = 0;
    virtual bool can_build_at(GridPosition pos, const ITerrainQueryable& terrain) const = 0;
};
```

BuildingSystem implements this for all buildings. Data sourced from BuildingTemplate.

#### IDemolishable (defined in interfaces.yaml)

```cpp
class IDemolishable {
public:
    virtual uint32_t get_demolition_cost() const = 0;
    virtual void on_demolished() = 0;
};
```

BuildingSystem implements this. on_demolished() cleans up building entity and frees the zone tile.

#### IContaminationSource (defined in interfaces.yaml)

```cpp
class IContaminationSource {
public:
    virtual uint32_t get_contamination_output() const = 0;
    virtual ContaminationType get_contamination_type() const = 0;
};
```

Fabrication buildings implement this. Contamination output from template data.

#### IEnergyConsumer (defined in interfaces.yaml, components added in Epic 4)

Buildings get EnergyComponent attached when created. EnergySystem (Epic 5) reads this. In Epic 4, the component exists but is serviced by the stub.

```cpp
// Component added to building entities in Epic 4:
struct EnergyComponent {
    uint32_t energy_required = 0;    // from template
    uint32_t energy_received = 0;    // set by EnergySystem (or stub)
    bool is_powered = true;          // stub default: true
    uint8_t energy_priority = 3;     // 1=critical, 3=normal
};
```

#### IFluidConsumer (defined in interfaces.yaml, components added in Epic 4)

```cpp
struct FluidComponent {
    uint32_t fluid_required = 0;     // from template
    uint32_t fluid_received = 0;     // set by FluidSystem (or stub)
    bool has_fluid = true;           // stub default: true
};
```

#### ITransportConnectable (defined in interfaces.yaml, evaluated in Epic 4 via stub)

BuildingSystem checks road proximity. In Epic 4, the stub always returns true. Epic 7 provides real implementation that queries road tile proximity.

### 5.2 New Interfaces Epic 4 Must Define

#### IZoneQueryable (new -- Epic 4 defines)

```cpp
class IZoneQueryable {
public:
    virtual ~IZoneQueryable() = default;
    virtual ZoneType get_zone_type(int32_t x, int32_t y) const = 0;
    virtual ZoneDensity get_zone_density(int32_t x, int32_t y) const = 0;
    virtual bool is_zoned(int32_t x, int32_t y) const = 0;
    virtual uint32_t get_zone_count(PlayerID player, ZoneType type) const = 0;
    virtual std::vector<GridPosition> get_zoned_tiles_without_buildings(
        PlayerID player, ZoneType type) const = 0;
    virtual float get_demand(ZoneType type, PlayerID player) const = 0;
};
```

**Consumers:** BuildingSystem (Epic 4), DemandSystem (Epic 10), UISystem (Epic 12), PortSystem (Epic 8).

#### IBuildingQueryable (new -- Epic 4 defines)

```cpp
class IBuildingQueryable {
public:
    virtual ~IBuildingQueryable() = default;
    virtual std::optional<EntityID> get_building_at(int32_t x, int32_t y) const = 0;
    virtual BuildingState get_building_state(EntityID entity) const = 0;
    virtual BuildingType get_building_type(EntityID entity) const = 0;
    virtual ZoneType get_building_zone_type(EntityID entity) const = 0;
    virtual uint16_t get_template_id(EntityID entity) const = 0;
    virtual uint32_t get_building_count(PlayerID player, ZoneType type) const = 0;
    virtual uint32_t get_building_count_by_state(PlayerID player, BuildingState state) const = 0;
    virtual std::vector<EntityID> get_buildings_in_rect(GridRect rect) const = 0;
    virtual bool is_position_occupied(int32_t x, int32_t y) const = 0;
};
```

**Consumers:** PopulationSystem (Epic 10), EconomySystem (Epic 11), DisorderSystem (Epic 10), ContaminationSystem (Epic 10), ServicesSystem (Epic 9), EnergySystem (Epic 5), FluidSystem (Epic 6), DisasterSystem (Epic 13), UISystem (Epic 12).

#### IBuildingTemplateQuery (new -- Epic 4 defines)

```cpp
class IBuildingTemplateQuery {
public:
    virtual ~IBuildingTemplateQuery() = default;
    virtual const BuildingTemplate& get_template(uint16_t template_id) const = 0;
    virtual std::vector<const BuildingTemplate*> get_templates_for_zone(
        ZoneType type, ZoneDensity density) const = 0;
    virtual uint32_t get_energy_required(uint16_t template_id) const = 0;
    virtual uint32_t get_fluid_required(uint16_t template_id) const = 0;
    virtual uint32_t get_population_capacity(uint16_t template_id) const = 0;
    virtual uint32_t get_job_capacity(uint16_t template_id) const = 0;
};
```

**Consumers:** PopulationSystem (Epic 10), EconomySystem (Epic 11), UISystem (Epic 12), EnergySystem (Epic 5), FluidSystem (Epic 6).

### 5.3 Forward Dependency Query Interfaces (stubs in Epic 4)

These interfaces are defined in Epic 4 with stub implementations. Later epics provide real implementations.

```cpp
// Epic 5 will implement:
class IEnergyQuery {
public:
    virtual ~IEnergyQuery() = default;
    virtual bool has_power(EntityID building) const = 0;
    virtual uint32_t get_energy_surplus(PlayerID player) const = 0;
};

// Epic 6 will implement:
class IFluidQuery {
public:
    virtual ~IFluidQuery() = default;
    virtual bool has_fluid(EntityID building) const = 0;
    virtual uint32_t get_fluid_surplus(PlayerID player) const = 0;
};

// Epic 7 will implement:
class ITransportQuery {
public:
    virtual ~ITransportQuery() = default;
    virtual bool is_road_accessible(EntityID building) const = 0;
    virtual uint32_t get_nearest_road_distance(int32_t x, int32_t y) const = 0;
};

// Epic 10 will implement:
class ILandValueQuery {
public:
    virtual ~ILandValueQuery() = default;
    virtual uint32_t get_land_value(int32_t x, int32_t y) const = 0;
};

class IDemandQuery {
public:
    virtual ~IDemandQuery() = default;
    virtual float get_demand(ZoneType type, PlayerID player) const = 0;
    virtual float get_demand_cap(ZoneType type, PlayerID player) const = 0;
};
```

---

## 6. Multiplayer Implications

### 6.1 Authority Model

| Action | Authority | Validation |
|--------|-----------|------------|
| Zone designation | Server | Terrain buildable, tile owned by player, not already zoned |
| Zone removal | Server | Tile zoned, tile owned by player |
| Building spawn | Server only | No client request -- server decides autonomously based on conditions |
| Building demolition | Server | Building exists, tile owned by player |
| Building state change | Server only | Utility state changes trigger state machine |
| Template selection | Server only | Seeded PRNG ensures determinism |
| Demand calculation | Server only | All players see same demand values |

### 6.2 Sync Requirements

| Data | Sync Method | Frequency | Direction |
|------|------------|-----------|-----------|
| ZoneComponent creation | On-change delta | When player zones | Server -> All clients |
| ZoneComponent removal | On-change delta | When player unzones | Server -> All clients |
| BuildingComponent creation | On-change delta | When building spawns | Server -> All clients |
| BuildingComponent state change | On-change delta | On state transition | Server -> All clients |
| BuildingComponent removal | On-change delta | On demolition complete | Server -> All clients |
| ConstructionComponent | Every tick (if visible) | During construction | Server -> All clients |
| Demand values | Every N ticks | Periodic (every 20 ticks = 1s) | Server -> All clients |
| Zone designation request | On player action | On zone tool use | Client -> Server |
| Demolition request | On player action | On demolish tool use | Client -> Server |

### 6.3 Ownership Validation

- **Zone placement:** Server checks `OwnershipComponent.owner == requesting_player` for the target tile. Rejects if tile is owned by Game Master (must purchase first) or another player.
- **Zone removal:** Same ownership check. Player can only unzone their own tiles.
- **Demolition:** Server checks building entity's `OwnershipComponent.owner == requesting_player`.
- **Building spawn:** Server automatically sets `OwnershipComponent.owner` to match the zone tile's owner. No player request involved.

### 6.4 Concurrent Request Handling

**Scenario: Two players zone adjacent tiles simultaneously**
- No conflict. Each player zones their own tiles. Requests processed in server receive order. Both succeed independently.

**Scenario: Player A zones, Player B demolishes nearby building in same tick**
- No conflict. Different tiles, different owners. Both processed independently.

**Scenario: Player zones a tile they just sold (race condition)**
- Server validates ownership at processing time. If tile already transferred, zone request fails. Client gets rejection response.

**Scenario: Player disconnects during multi-tile zone operation**
- Server processes the ZoneBatchDesignateRequest as a single atomic operation. If it arrives complete, it is fully processed. If connection drops mid-transmission, the message is incomplete and discarded -- no partial application.

**Scenario: Building spawn on a tile that was just unzoned**
- Server processes zone removal first (player action), then building tick. Building tick sees no zone, does not spawn. No race condition because simulation tick is single-threaded and zone removal happens within the same tick or before the next building tick.

### 6.5 Visibility

- All zone and building data is visible to all players. No fog of war in this game.
- All players see all buildings materializing, becoming active, degrading, abandoning.
- Demand values are per-player (each player has their own R/C/I demand based on their colony state), but all are computed server-side.

### 6.6 Latency Handling

- No client-side prediction for zoning or building. Client sends request, waits for server confirmation.
- At 20 tick/s with typical 50-100ms latency, zone placement confirmation is 1-3 frames behind input. Acceptable for a city builder (not an FPS).
- For area zone operations (drag-zone), client shows a preview overlay immediately but does not apply until server confirms. Server may reject some tiles in the area (e.g., unbuildable terrain in the middle of the drag rectangle). Client updates to match server result.

---

## 7. Cross-Epic Dependencies

### 7.1 What Epic 4 Needs from Epic 3

| Epic 3 Provides | Epic 4 Consumer | How Used |
|----------------|----------------|----------|
| `ITerrainQueryable.is_buildable(x, y)` | ZoneSystem (zone placement validation) | Before allowing zone designation |
| `ITerrainQueryable.get_terrain_type(x, y)` | ZoneSystem (zone cost modifiers) | Volcanic rock increases zone cost |
| `ITerrainQueryable.get_build_cost_modifier(x, y)` | BuildingSystem (construction cost) | Template cost * terrain modifier |
| `ITerrainQueryable.get_elevation(x, y)` | BuildingSystem (3D placement) | Building world Y position |
| `ITerrainQueryable.get_value_bonus(x, y)` | BuildingSystem (template selection, until LandValueSystem exists) | Crystal field bonus affects which templates can spawn |
| `ITerrainQueryable.get_map_width/height()` | ZoneSystem (bounds checking) | Zone area operations |
| TerrainModifiedEvent | ZoneSystem (zone invalidation if terrain changes under zone) | Rare: terraform removes zone buildability |

**Critical requirement:** ITerrainQueryable must be stable and implemented before Epic 4 zone placement can function. This is the #1 hard dependency.

### 7.2 What Epics 5/6/7 Need from Epic 4

#### Epic 5 (EnergySystem) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Query which buildings exist and need power |
| `BuildingComponent.energy_required` (from template) | Determine power consumption per building |
| `EnergyComponent` on building entities | EnergySystem writes `is_powered` and `energy_received` |
| `BuildingConstructedEvent` | New consumer added to energy grid |
| `BuildingDemolishedEvent` | Consumer removed from energy grid |

#### Epic 6 (FluidSystem) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Query which buildings need water |
| `BuildingComponent.fluid_required` (from template) | Determine water consumption per building |
| `FluidComponent` on building entities | FluidSystem writes `has_fluid` and `fluid_received` |
| `BuildingConstructedEvent` | New consumer added to fluid network |
| `BuildingDemolishedEvent` | Consumer removed from fluid network |

#### Epic 7 (TransportSystem) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Query buildings for traffic generation |
| Building positions | Calculate road proximity and traffic contribution |
| `BuildingConstructedEvent` | New traffic generator on the network |

### 7.3 What Epics 9/10/11/13 Need from Epic 4

#### Epic 9 (ServicesSystem) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Service buildings are buildings (placed through BuildingSystem) |
| `BuildingComponent` entity creation pattern | Services uses same building infrastructure |
| `IBuildable` / `IDemolishable` | Service building construction/demolition |

#### Epic 10 (Simulation) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | PopulationSystem: count habitation buildings for housing capacity |
| `IBuildingQueryable` | DisorderSystem: buildings generate disorder |
| `IBuildingQueryable` | ContaminationSystem: fabrication buildings generate contamination |
| `IZoneQueryable` | DemandSystem: zone counts for demand calculation |
| `IContaminationSource` (fabrication) | ContaminationSystem: pollution sources |
| Building template data (population_capacity, job_capacity) | PopulationSystem: employment and housing |
| Zone demand values | DemandSystem replaces Epic 4's basic demand with full calculation |

#### Epic 11 (Financial) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Taxable building counts, building values |
| Building template data (construction_cost) | Budget: building costs affect treasury |
| `BuildingConstructedEvent` | Deduct construction cost from treasury |
| `BuildingDemolishedEvent` | Possible demolition cost |

#### Epic 13 (Disasters) needs:

| Epic 4 Provides | How Used |
|----------------|----------|
| `IBuildingQueryable` | Find buildings to damage |
| `BuildingComponent.health` / `max_health` | Apply damage, track destruction |
| `IDamageable` (implied -- buildings are damageable) | Disaster damage application |
| Building demolition/destruction | Buildings destroyed by disaster |

### 7.4 Dependency Summary Diagram

```
EPIC 3 (Terrain)
    |
    | ITerrainQueryable
    v
EPIC 4 (Zoning & Building)  <-- THIS EPIC
    |
    |-- IZoneQueryable, IBuildingQueryable, events, components
    |
    +---> EPIC 5 (Energy)      -- reads buildings, writes power state
    +---> EPIC 6 (Fluid)       -- reads buildings, writes fluid state
    +---> EPIC 7 (Transport)   -- reads building positions for traffic
    +---> EPIC 9 (Services)    -- service buildings ARE buildings
    +---> EPIC 10 (Simulation) -- population, demand, disorder, contamination
    +---> EPIC 11 (Financial)  -- taxes, costs
    +---> EPIC 13 (Disasters)  -- building damage/destruction
```

Epic 4 is the most-depended-upon epic after Epic 3. Seven downstream epics consume its data. Interface stability is critical.

---

## 8. Building Template System

### 8.1 Template Pool Organization

Templates are organized in a two-level hierarchy:

```
Template Pool
  |-- Habitation
  |     |-- Low Density: [template_001, template_002, template_003, template_004, template_005]
  |     '-- High Density: [template_006, template_007, template_008, template_009, template_010]
  |-- Exchange
  |     |-- Low Density: [template_011, template_012, template_013, template_014, template_015]
  |     '-- High Density: [template_016, template_017, template_018, template_019, template_020]
  '-- Fabrication
        |-- Low Density: [template_021, template_022, template_023, template_024, template_025]
        '-- High Density: [template_026, template_027, template_028, template_029, template_030]
```

**Minimum viable pool:** 5 templates per (zone_type, density) pair = 30 templates total. This provides enough variety to avoid obvious repetition on a 256x256 map.

### 8.2 Selection Algorithm

```
function select_template(tile_x, tile_y, zone_type, density, land_value, sim_tick):
    // 1. Get candidate pool
    candidates = template_registry.get_templates(zone_type, density)

    // 2. Filter by land value
    candidates = candidates.filter(t => t.min_land_value <= land_value
                                     && (t.max_land_value == 0 || t.max_land_value >= land_value))

    // 3. Check adjacent buildings to avoid duplicates
    adjacent_templates = get_adjacent_building_templates(tile_x, tile_y)
    preferred = candidates.filter(t => t.template_id NOT IN adjacent_templates)
    if preferred is not empty:
        candidates = preferred
    // If all candidates match adjacent, allow duplicates (small pool edge case)

    // 4. Weighted random selection
    seed = hash(tile_x, tile_y, sim_tick)  // deterministic per-tile per-tick
    rng = SeededPRNG(seed)
    selected = weighted_random(candidates, t => t.selection_weight, rng)

    // 5. Apply procedural variation
    variation = generate_variation(selected.variation, rng)
    // variation includes: rotation (0/90/180/270), scale factor, color accent index

    return (selected, variation)
```

### 8.3 Density-to-Template Mapping

| Zone Type | Low Density | High Density |
|-----------|-------------|--------------|
| Habitation | Small dwellings (1x1), hab pods, single-unit shelters | Hab towers (1x1 or 2x2), multi-unit hab blocks, stacked pods |
| Exchange | Small trade posts (1x1), vendor stalls, exchange kiosks | Exchange towers (1x1 or 2x2), trade centers, market complexes |
| Fabrication | Small fabricators (1x1), workshop sheds, processing units | Large fabricators (2x2), factory complexes, production towers |

Low-density buildings are always 1x1 footprint. High-density buildings may be 1x1 or 2x2 (defined per template).

### 8.4 Land Value Ranges

| Category | Land Value Range | Template Tier |
|----------|-----------------|---------------|
| Poor | 0 - 30 | Basic/rundown templates |
| Moderate | 31 - 60 | Standard templates |
| Wealthy | 61 - 100 | Premium/ornate templates |

Templates have `min_land_value` and `max_land_value`. Some templates span multiple ranges. Wealthy areas get visually distinct, more impressive buildings.

### 8.5 Procedural Variation Parameters

Each template defines what variations are allowed:

| Parameter | Range | Purpose |
|-----------|-------|---------|
| Y-axis rotation | 0, 90, 180, 270 degrees | Avoid grid alignment monotony |
| Scale | 0.95x - 1.05x | Slight size variation |
| Color accent | 4 variants per template | Bioluminescent glow color shift (same hue family, different intensity/saturation) |
| Height variation | 0.9x - 1.1x (Y-axis only) | Slight height differences in same template |

Variations are deterministic from tile coordinate hash, so all clients produce identical visual results.

### 8.6 Template Data Loading

For MVP, templates can be defined in a static C++ array or loaded from a JSON/YAML data file at startup. The data file approach is preferred for iteration speed:

```json
{
  "templates": [
    {
      "id": 1,
      "zone_type": "habitation",
      "density": "low",
      "model_source": "procedural",
      "footprint": { "w": 1, "h": 1 },
      "construction_cost": 100,
      "construction_ticks": 40,
      "min_land_value": 0,
      "max_land_value": 0,
      "energy_required": 5,
      "fluid_required": 3,
      "contamination_output": 0,
      "population_capacity": 10,
      "job_capacity": 0,
      "selection_weight": 100,
      "variation": {
        "allow_rotation": true,
        "scale_min": 0.95,
        "scale_max": 1.05,
        "color_accent_variants": 4
      }
    }
  ]
}
```

### 8.7 Construction Animation Integration

Per canon (patterns.yaml, art_pipeline section), each building template includes a materializing animation:

- Buildings play construction animation when spawned (state = Materializing)
- Animation is part of the template definition
- RenderingSystem reads ConstructionComponent.progress and renders the appropriate animation frame
- On completion, the building transitions to its active/idle visual state

For procedural buildings, the materializing animation could be a generic "grow from ground" effect: scale Y from 0 to 1 over construction duration, with bioluminescent glow intensifying as construction progresses. This is a RenderingSystem concern, but BuildingSystem provides the progress data.

---

## 9. Questions for Other Agents

### @game-designer

1. **Zone removal with existing building:** When a player unzones a tile that has an active building on it, should the building be demolished immediately, should it remain but stop receiving zone benefits (and eventually abandon), or should unzoning be blocked while a building exists? This is a significant gameplay decision that affects player agency vs. city stability.

2. **Density upgrade trigger:** What conditions should cause a zone tile to upgrade from Low to High density? Options: (a) land value exceeds threshold, (b) time since zone designation + positive demand, (c) player explicitly upgrades via tool, (d) automatic when services are available. Canon says density levels exist but does not specify the upgrade mechanic.

3. **Building spawn rate:** How many buildings should spawn per tick across the entire map? Spawning too fast feels unrealistic; too slow feels unresponsive. SimCity 2000 spawns buildings gradually with visible construction. Should we target N buildings per simulation second (e.g., max 2-3 new constructions per second per player)?

4. **Demand formula:** The basic demand calculation needs design parameters. What should the baseline demand be for each zone type at game start? What is the saturation curve -- linear, logarithmic, stepped? Should demand ever go negative (causing building abandonment)?

5. **Abandoned building behavior:** How long should a building remain in Degraded state before transitioning to Abandoned? Should abandoned buildings eventually self-demolish (clearing the tile for new development), or should they persist until the player manually demolishes them?

6. **Building level/upgrade visual:** When a building upgrades from level 1 to level 2 (or low density to high density), should it visually transform in-place (construction animation again), or should it demolish and rebuild? The in-place transform is smoother but requires more template/animation work.

### @graphics-engineer

7. **Building model requirements for Epic 4:** What is the minimum 3D model infrastructure needed from Epic 2 for buildings to be visible? Specifically: (a) Does the building toon shader variant need to exist, or can we reuse the terrain shader initially? (b) What vertex format should building models use? (c) Can procedural buildings use simple box geometry for MVP, with proper models added later?

8. **Construction animation rendering:** How should ConstructionComponent.progress drive the visual materializing effect? Options: (a) Y-scale lerp (building "grows" from ground), (b) alpha fade-in with glow, (c) particle effect overlay, (d) wireframe-to-solid transition. What is achievable within Epic 2's rendering pipeline?

9. **Building state visual feedback:** Active buildings should glow (bioluminescent), degraded should flicker, abandoned should have no glow. Is this achievable through per-entity emissive uniform, or does it require a building-specific shader variant?

### @orchestrator

10. **Epic 4 / Epic 5 parallelism:** Can Epic 5 (Energy) begin implementation before Epic 4 is complete, given that Epic 4 defines the interfaces Epic 5 depends on? The stub interfaces (work item B-15) could be delivered early as a standalone ticket to unblock Epic 5 planning.

---

## 10. Risks & Concerns

### 10.1 Architectural Risks

**RISK: Interface Stability (HIGH)**
Epic 4 defines IZoneQueryable and IBuildingQueryable, which are consumed by 7+ downstream epics. Any breaking change to these interfaces after Epic 5+ development begins would require coordinated changes across multiple systems. **Mitigation:** Lock these interfaces during Epic 4 planning. Design with extensibility (virtual methods allow additions without breaking existing callers). Include forward-looking fields even if unused initially.

**RISK: Forward Dependency Coupling (MEDIUM)**
BuildingSystem's state machine depends on utility availability (power, water, road access). With stubs returning "always available," the state machine's Degraded/Abandoned paths cannot be tested until Epics 5/6/7 are implemented. **Mitigation:** (a) Stubs should have a debug toggle to simulate utility failure, allowing state machine testing. (b) Unit tests should directly inject false values. (c) Accept that integration testing of degradation flows is deferred to Epic 5+.

**RISK: Stub-to-Real Transition (MEDIUM)**
When EnergySystem (Epic 5) replaces the energy stub, all existing buildings suddenly experience real power evaluation. If a player's colony has no power plants, every building immediately enters Degraded state. **Mitigation:** (a) Epic 5 must include a migration path (e.g., grace period on first power evaluation), (b) Epic 4's stub interface should use dependency injection (service locator or similar) so the swap is seamless, (c) Document the transition risk clearly for Epic 5 planning.

### 10.2 Performance Concerns

**CONCERN: Building Spawn Tick Cost (MEDIUM)**
BuildingSystem.tick() must iterate all zoned tiles without buildings, evaluate conditions, and potentially spawn buildings. On a 256x256 map with aggressive zoning, this could be 10,000+ zoned tiles. **Mitigation:** (a) Maintain a dirty list of "zoned tiles needing evaluation" rather than scanning all tiles each tick. (b) Cap spawns per tick (max 3-5 per player per tick). (c) Use spatial index for zone queries, not full iteration.

**CONCERN: Template Selection with Adjacent Check (LOW)**
Checking adjacent buildings to avoid duplicates requires 4-8 spatial queries per spawn. At 3-5 spawns per tick, this is 12-40 queries per tick -- negligible.

**CONCERN: Multi-Tile Building Footprint Overlap (LOW)**
2x2 footprint buildings require checking 4 tiles for availability. This is straightforward but must handle race conditions in the same tick (two buildings trying to claim overlapping 2x2 areas). **Mitigation:** Process spawns sequentially within a tick, marking tiles as claimed immediately.

### 10.3 Design Ambiguities

**AMBIGUITY: Zone Storage Model**
I recommend standard ECS (entity per zoned tile) over dense grid for zones. But this means ZoneSystem queries like "all habitation tiles for player X" require iterating all zone entities with a filter. If this is too slow, we may need a supplementary index. **Decision needed:** Accept ECS with spatial index, or use dense grid? The ECS approach is more consistent with canon patterns. Dense grid exception applies to terrain because terrain covers every tile; zones do not.

**AMBIGUITY: Demand Calculation Ownership**
Canon defines basic demand in ZoneSystem (Epic 4) AND full demand in DemandSystem (Epic 10). The question is: does ZoneSystem's basic demand get replaced entirely by DemandSystem, or do they coexist? **Recommendation:** ZoneSystem provides a simple demand calculation used only until Epic 10 is implemented. DemandSystem then takes over completely, and ZoneSystem's demand code is deprecated. The IZoneQueryable.get_demand() method should delegate to IDemandQuery if available, falling back to the basic calculation.

**AMBIGUITY: Building Upgrade Mechanics**
Canon mentions building upgrades/downgrades but does not specify the trigger conditions or visual behavior. This needs Game Designer input before ticket breakdown (see question #2 and #6 above).

**AMBIGUITY: Zone Removal with Existing Buildings**
Canon does not specify behavior when a player removes a zone designation from a tile that has a building. This needs Game Designer input (see question #1 above).

### 10.4 Technical Debt Considerations

**DEBT: Basic Demand Calculation**
The basic demand in ZoneSystem is intentionally simplified and will be replaced by Epic 10. We should mark this code clearly as temporary with a migration path documented.

**DEBT: Stub Interfaces**
Five stub interfaces are introduced in Epic 4. Each must be tracked and replaced in its respective epic. A checklist of stub replacements should be maintained in cross-epic tracking.

**DEBT: Procedural Building Visuals**
For MVP, procedural buildings may be simple box geometry with bioluminescent tinting. Proper 3D models (even procedural ones) require more art pipeline work. This is acceptable for Epic 4 but must be improved before public release.

### 10.5 Sequencing Risks

**RISK: Epic 3 Interface Readiness**
Epic 4 cannot begin zone placement validation without ITerrainQueryable from Epic 3 (ticket 3-014). If Epic 3 is delayed, Epic 4's data model and building logic (work items Z-01 through Z-02, B-01 through B-05) can proceed, but zone placement (Z-03, Z-04) is blocked.

**RISK: Epic 2 Rendering for Building Visibility**
Buildings need to be visible to test the full zone-to-building pipeline. If Epic 2's 3D model rendering is not ready, Epic 4 can be developed and tested with console output / debug overlay only, but visual validation is deferred. The terrain rendering patterns from Epic 3 (ITerrainRenderData, chunk dirty tracking) should inform how BuildingSystem exposes data to RenderingSystem.

---

## Appendix A: Simulation Tick Order (Epic 4 Context)

From interfaces.yaml ISimulatable priorities:

```
Priority 5:  TerrainSystem        -- terrain state available
Priority 10: EnergySystem         -- power state calculated (stub in Epic 4)
Priority 20: FluidSystem          -- fluid state calculated (stub in Epic 4)
Priority 30: ZoneSystem           -- zone state updated, demand calculated
Priority 40: BuildingSystem       -- buildings spawned/updated based on zone + utility state
Priority 50: PopulationSystem     -- population updated based on building state
...
```

This ordering ensures:
1. Terrain is settled before zones evaluate buildability
2. Utilities are calculated before buildings check powered/watered status
3. Zones are updated before buildings spawn
4. Buildings are settled before population counts housing

---

## Appendix B: Entity Composition Examples

**Zoned tile (no building yet):**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 75, elevation: 3 }
    OwnershipComponent { owner: PLAYER_1, state: Owned }
    ZoneComponent { zone_type: Habitation, density: Low, demand_weight: 128 }
}
```

**Building under construction:**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 75, elevation: 3 }
    OwnershipComponent { owner: PLAYER_1, state: Owned }
    ZoneComponent { zone_type: Habitation, density: Low, demand_weight: 128 }
    BuildingComponent { building_type: Zone, zone_type: Habitation, density: Low,
                        template_id: 3, state: Materializing, health: 100, ... }
    ConstructionComponent { ticks_remaining: 30, total_ticks: 40, progress: 0.25 }
    EnergyComponent { energy_required: 5, is_powered: true }
    FluidComponent { fluid_required: 3, has_fluid: true }
}
```

**Active building:**
```
Entity {
    PositionComponent { grid_x: 50, grid_y: 75, elevation: 3 }
    OwnershipComponent { owner: PLAYER_1, state: Owned }
    ZoneComponent { zone_type: Habitation, density: Low, demand_weight: 128 }
    BuildingComponent { building_type: Zone, zone_type: Habitation, density: Low,
                        template_id: 3, state: Active, health: 100, ... }
    EnergyComponent { energy_required: 5, is_powered: true }
    FluidComponent { fluid_required: 3, has_fluid: true }
}
```
Note: ConstructionComponent is removed after construction completes.

**Fabrication building (with contamination):**
```
Entity {
    PositionComponent { grid_x: 120, grid_y: 90, elevation: 5 }
    OwnershipComponent { owner: PLAYER_2, state: Owned }
    ZoneComponent { zone_type: Fabrication, density: High, demand_weight: 200 }
    BuildingComponent { building_type: Zone, zone_type: Fabrication, density: High,
                        template_id: 27, state: Active, health: 100, ... }
    EnergyComponent { energy_required: 15, is_powered: true }
    FluidComponent { fluid_required: 8, has_fluid: true }
    // ContaminationSource data derived from template (contamination_output: 20)
}
```

---

## Appendix C: Work Item Size Summary

| Size | Count | Items |
|------|-------|-------|
| S | 8 | Z-01, Z-02, Z-11, B-01, B-02, B-12, B-14, B-17 |
| M | 17 | Z-03, Z-04, Z-05, Z-06, Z-07, Z-09, Z-10, B-03, B-08, B-09, B-11, B-13, B-15, B-16, B-18, B-19, S-02 |
| L | 6 | Z-08, B-04, B-05, B-07, B-10, S-04 |
| XL | 1 | B-06 |
| **Total** | **32** | Plus S-01 (S), S-03 (M), B-20 (M) = **35 work items** |

**Estimated effort distribution:**
- ZoneSystem: 11 items (S:2, M:7, L:1, XL:0)
- BuildingSystem: 20 items (S:4, M:9, L:4, XL:1)
- Shared: 4 items (S:1, M:2, L:1)

BuildingSystem is approximately twice the scope of ZoneSystem, which aligns with its greater complexity (state machine, templates, spawning logic, multiple interface implementations).

---

## Appendix D: Forward Dependency Stub Replacement Checklist

This checklist tracks all stubs introduced in Epic 4 and which epic replaces them.

| Stub | Introduced | Replaced By | Notes |
|------|-----------|-------------|-------|
| IEnergyQuery (always powered) | Epic 4, B-15 | Epic 5 (EnergySystem) | Must handle transition: existing buildings may lose power |
| IFluidQuery (always has fluid) | Epic 4, B-15 | Epic 6 (FluidSystem) | Must handle transition: existing buildings may lose water |
| ITransportQuery (always accessible) | Epic 4, B-15 | Epic 7 (TransportSystem) | 3-tile rule enforcement may strand existing buildings |
| ILandValueQuery (value = 50) | Epic 4, B-15 | Epic 10 (LandValueSystem) | Template selection becomes land-value-responsive |
| IDemandQuery (demand = 100) | Epic 4, B-15 / Z-08 | Epic 10 (DemandSystem) | Full R/C/I demand with population/economy feedback |

**Each stub replacement epic must:**
1. Implement the real interface
2. Register it to replace the stub via dependency injection
3. Handle the transition for existing game state (buildings that existed under stub assumptions)
4. Test the integration with BuildingSystem's state machine
