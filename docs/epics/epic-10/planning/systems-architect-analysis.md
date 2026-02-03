# Systems Architect Analysis: Epic 10 - Simulation Core

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.9.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 10 is the **simulation heartbeat** of Sims 3000. It contains six interconnected systems that calculate the fundamental city metrics: population growth, zone demand (RCI), land value, disorder (crime), and contamination (pollution). These systems form circular data dependencies that require careful tick ordering and the use of previous-tick values to break cycles.

Key architectural challenges:
1. **Circular dependencies:** LandValue <-> Disorder <-> Contamination form a value triangle where each affects the others
2. **Per-player vs global state:** Population and demand are per-player; disorder/contamination spread across ownership boundaries
3. **Dense grid storage:** Contamination and land value grids are candidates for the dense_grid_exception pattern
4. **Ghost town integration:** SimulationCore manages the AbandonmentSystem for player departure decay

This epic depends on Epics 4-7 for building, energy, fluid, and transport data. It provides critical data to Epic 9 (services effectiveness) and Epic 11 (tax base from population).

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Tick Ordering Analysis](#2-tick-ordering-analysis)
3. [Circular Dependency Resolution](#3-circular-dependency-resolution)
4. [Data Flow Diagram](#4-data-flow-diagram)
5. [Dense Grid Candidates](#5-dense-grid-candidates)
6. [Multiplayer Implications](#6-multiplayer-implications)
7. [Key Work Items](#7-key-work-items)
8. [Questions for Other Agents](#8-questions-for-other-agents)
9. [Risks and Concerns](#9-risks-and-concerns)
10. [Interface Recommendations](#10-interface-recommendations)
11. [Dependencies](#11-dependencies)

---

## 1. System Boundaries

### 1.1 SimulationCore (type: core)

**Owns:**
- Tick scheduling and dispatch
- System execution order enforcement
- Simulation speed control (pause, 1x, 2x, 3x)
- Time progression (cycles, phases)
- AbandonmentSystem (ghost town decay process)

**Does NOT Own:**
- Individual simulation calculations (domain systems own)
- UI speed controls (UISystem owns)
- Network sync timing (SyncSystem owns)

**Provides:**
- Current simulation time (tick count, cycle, phase)
- Tick events (TickStartEvent, TickCompleteEvent)
- Speed state queries

**Depends On:**
- All ISimulatable systems (to orchestrate)

### 1.2 PopulationSystem (tick_priority: 50)

**Owns:**
- Population count per player
- Birth/death rate calculation
- Migration in/out calculation
- Life expectancy calculation (affected by medical coverage, contamination)
- Employment tracking (employed vs unemployed counts)
- Population capacity per building (from BuildingComponent data)

**Does NOT Own:**
- Building placement (BuildingSystem owns)
- Medical service coverage (ServicesSystem owns in Epic 9)
- Visual representation of beings (RenderingSystem owns)

**Provides:**
- IStatQueryable: population stats per player
- Total population per player
- Population breakdown (employed, unemployed, by age if modeled)
- Population growth rate
- Life expectancy stats

**Depends On:**
- BuildingSystem (habitation buildings provide capacity)
- EnergySystem (unpowered buildings reduce habitability)
- FluidSystem (unwatered buildings reduce habitability)
- DisorderSystem (high disorder reduces migration in)
- ContaminationSystem (high contamination reduces life expectancy)

### 1.3 DemandSystem (tick_priority: not yet defined - recommend 52)

**Owns:**
- RCI (zone pressure) calculation
- Demand factors aggregation
- Demand cap calculation (max demand based on city size)

**Does NOT Own:**
- Zone placement (ZoneSystem owns)
- Building spawning (BuildingSystem owns)
- Tax rate effects on demand (EconomySystem owns, DemandSystem queries)

**Provides:**
- Current demand values (R, C, I as -100 to +100 or similar scale)
- Demand factors breakdown (for UI display)

**Depends On:**
- PopulationSystem (population drives all demand)
- EconomySystem (tax rates affect demand - stub until Epic 11)
- TransportSystem (accessibility affects demand)

### 1.4 LandValueSystem (tick_priority: not yet defined - recommend 65)

**Owns:**
- Sector value calculation per tile
- Value factor aggregation (positive and negative modifiers)

**Does NOT Own:**
- What affects value (other systems report via interfaces)
- Building upgrades from high value (BuildingSystem owns)

**Provides:**
- Land value per tile (0-255 scale)
- Value overlay data for UI
- IStatQueryable: average land value stats

**Depends On:**
- TerrainSystem (water proximity bonus, terrain type effects)
- ContaminationSystem (**PREVIOUS TICK** data to avoid circular dependency)
- DisorderSystem (**PREVIOUS TICK** data to avoid circular dependency)
- TransportSystem (accessibility bonus, congestion penalty)
- ServicesSystem (Epic 9: education/medical boost land value)

**Critical Note:** Uses previous tick's disorder and contamination values.

### 1.5 DisorderSystem (tick_priority: 70)

**Owns:**
- Disorder (crime) generation per tile
- Disorder spread calculation (from high-disorder tiles to neighbors)
- Enforcer effectiveness calculation (how much enforcers reduce disorder)

**Does NOT Own:**
- Enforcer building placement (ServicesSystem via BuildingSystem)
- Disorder-triggered events (DisasterSystem owns riots)

**Provides:**
- Disorder rate per tile (0-255 scale)
- Disorder overlay data for UI
- Total disorder stats (IStatQueryable)

**Depends On:**
- BuildingSystem (buildings generate base disorder)
- PopulationSystem (population density affects disorder generation)
- ServicesSystem (Epic 9: enforcer coverage reduces disorder)

**Critical Note:** Reads **PREVIOUS TICK's** land value to avoid circular dependency. Low land value increases disorder generation.

**Components:**
- DisorderComponent (attached to tiles or as dense grid)

### 1.6 ContaminationSystem (tick_priority: 80)

**Owns:**
- Contamination (pollution) generation per source
- Contamination spread (diffusion from high to low areas)
- Contamination decay over time (natural cleanup)

**Does NOT Own:**
- What produces contamination (other systems report via IContaminationSource)
- Contamination cleanup buildings (future feature)

**Provides:**
- Contamination level per tile (0-255 scale)
- Contamination overlay data for UI
- Total contamination stats (IStatQueryable)
- Contamination type breakdown (industrial, traffic, energy, terrain)

**Depends On:**
- BuildingSystem (fabrication buildings implement IContaminationSource)
- TransportSystem (traffic pollution via ITransportProvider.get_traffic_volume_at)
- EnergySystem (polluting nexuses implement IContaminationSource)
- TerrainSystem (blight_mires/toxic_marshes generate terrain contamination)

**Components:**
- ContaminationComponent (attached to sources, or dense grid for tile values)

### 1.7 Boundary Summary Table

| Concern | SimulationCore | PopulationSys | DemandSys | LandValueSys | DisorderSys | ContaminationSys |
|---------|----------------|---------------|-----------|--------------|-------------|------------------|
| Tick scheduling | **Owns** | - | - | - | - | - |
| Simulation speed | **Owns** | - | - | - | - | - |
| Ghost town decay | **Owns** | - | - | - | - | - |
| Population count | - | **Owns** | - | - | - | - |
| Birth/death rates | - | **Owns** | - | - | - | - |
| Employment | - | **Owns** | - | - | - | - |
| RCI demand | - | - | **Owns** | - | - | - |
| Tile value | - | - | - | **Owns** | - | - |
| Crime generation | - | - | - | - | **Owns** | - |
| Crime spread | - | - | - | - | **Owns** | - |
| Pollution generation | - | - | - | - | - | **Owns** |
| Pollution spread | - | - | - | - | - | **Owns** |

---

## 2. Tick Ordering Analysis

### 2.1 Current Tick Order (from interfaces.yaml)

```
Priority 5:  TerrainSystem
Priority 10: EnergySystem
Priority 20: FluidSystem
Priority 30: ZoneSystem
Priority 40: BuildingSystem
Priority 45: TransportSystem
Priority 47: RailSystem
Priority 50: PopulationSystem      <-- Epic 10
Priority 60: EconomySystem         <-- Epic 11 (stub until then)
Priority 70: DisorderSystem        <-- Epic 10
Priority 80: ContaminationSystem   <-- Epic 10
```

### 2.2 Missing Priorities for Epic 10 Systems

The following systems need tick priorities assigned:

| System | Recommended Priority | Rationale |
|--------|---------------------|-----------|
| DemandSystem | 52 | After PopulationSystem (needs population), before EconomySystem |
| LandValueSystem | 65 | After EconomySystem (tax effects), before DisorderSystem (disorder uses previous land value, but land value should compute first to have fresh data for next tick) |

### 2.3 Proposed Complete Tick Order

```
Priority 5:  TerrainSystem         -- terrain data available
Priority 10: EnergySystem          -- power state calculated
Priority 20: FluidSystem           -- fluid state calculated
Priority 30: ZoneSystem            -- zones updated
Priority 40: BuildingSystem        -- buildings updated, capacity known
Priority 45: TransportSystem       -- traffic aggregated, congestion calculated
Priority 47: RailSystem            -- transit coverage applied
Priority 50: PopulationSystem      -- population updated based on building capacity
Priority 52: DemandSystem          -- RCI demand calculated from population  <-- NEW
Priority 60: EconomySystem         -- taxes collected, expenses paid
Priority 65: LandValueSystem       -- land value calculated (uses prev disorder/contamination) <-- NEW
Priority 70: DisorderSystem        -- disorder calculated (uses prev land value)
Priority 80: ContaminationSystem   -- contamination calculated (last, uses traffic data)
```

### 2.4 Tick Priority Rationale

**PopulationSystem at 50:**
- Needs BuildingSystem (40) for habitation capacity
- Needs EnergySystem (10) and FluidSystem (20) for habitability state
- Provides data to DemandSystem

**DemandSystem at 52:**
- Needs PopulationSystem (50) for population counts
- Needs EconomySystem (60) for tax rate effects... but EconomySystem comes after
- **Design Decision:** DemandSystem uses previous tick's tax rates (acceptable lag)

**LandValueSystem at 65:**
- Needs TerrainSystem (5) for terrain bonuses
- Needs TransportSystem (45) for accessibility
- Uses **previous tick's** disorder (70) and contamination (80)
- Provides data for next tick's disorder calculation

**DisorderSystem at 70:**
- Needs BuildingSystem (40) for disorder sources
- Needs PopulationSystem (50) for population density
- Uses **previous tick's** land value (65)
- Provides data for next tick's land value calculation

**ContaminationSystem at 80:**
- Needs BuildingSystem (40) for fabrication pollution
- Needs TransportSystem (45) for traffic pollution
- Needs EnergySystem (10) for power plant pollution
- Provides data for next tick's land value calculation
- Runs last so other systems can query current traffic/building state

---

## 3. Circular Dependency Resolution

### 3.1 The Value-Disorder-Contamination Triangle

These three systems have circular dependencies:

```
LandValueSystem
     ^    \
    /      \
   /        v
DisorderSys <-- ContaminationSys
```

- **LandValue affects Disorder:** Low land value areas have higher crime generation
- **Disorder affects LandValue:** High crime reduces land value
- **Contamination affects LandValue:** High pollution reduces land value
- **LandValue affects Contamination:** (Indirect) Low value -> more fabrication -> more pollution

### 3.2 Canon-Specified Resolution

From systems.yaml:
- LandValueSystem: "Uses PREVIOUS tick's disorder/contamination to avoid circular dependency"
- DisorderSystem: "Reads previous tick's land value to avoid circular dependency"

### 3.3 Implementation Pattern

```cpp
class LandValueSystem : public ISimulatable {
private:
    // Double-buffered value grids
    LandValueGrid current_values;
    LandValueGrid previous_values;

public:
    void tick(float delta_time) override {
        // Swap buffers at tick start
        std::swap(current_values, previous_values);

        // Calculate new values using previous disorder/contamination
        for (each tile) {
            uint8_t disorder = disorder_system.get_disorder_at_previous_tick(x, y);
            uint8_t contamination = contamination_system.get_contamination_at_previous_tick(x, y);
            current_values[x][y] = calculate_land_value(terrain, disorder, contamination);
        }
    }

    // For current tick queries (after this system ticks)
    uint8_t get_land_value_at(int x, int y) const { return current_values[x][y]; }

    // For next tick's disorder calculation (previous tick's values)
    uint8_t get_land_value_at_previous_tick(int x, int y) const { return previous_values[x][y]; }
};
```

### 3.4 Double-Buffering Requirements

| System | Needs Double Buffer | Why |
|--------|---------------------|-----|
| LandValueSystem | **YES** | DisorderSystem needs previous tick's values |
| DisorderSystem | **YES** | LandValueSystem needs previous tick's values |
| ContaminationSystem | **YES** | LandValueSystem needs previous tick's values |
| PopulationSystem | NO | No circular dependencies |
| DemandSystem | NO | No circular dependencies |

### 3.5 Interface Methods for Previous Tick Access

Each double-buffered system needs two query methods:

```cpp
// Current tick value (for UI, current-tick calculations)
uint8_t get_disorder_at(int x, int y) const;

// Previous tick value (for systems that need to break circular dependency)
uint8_t get_disorder_at_previous_tick(int x, int y) const;
```

---

## 4. Data Flow Diagram

### 4.1 Tick Data Flow

```
                    TICK N DATA FLOW
                    ================

TerrainSystem(5)
      |
      v
EnergySystem(10) --> EnergyPoolState, CoverageZone
      |
      v
FluidSystem(20) --> FluidPoolState, CoverageZone
      |
      v
ZoneSystem(30)
      |
      v
BuildingSystem(40) --> BuildingCapacity, TrafficContribution
      |                     IContaminationSource
      v
TransportSystem(45) --> Congestion, TrafficVolume, Accessibility
      |
      v
RailSystem(47) --> TransitCoverage
      |
      v
PopulationSystem(50) --> TotalPopulation, EmploymentStats
      |                        |
      v                        v
DemandSystem(52)        EconomySystem(60) [stub until Epic 11]
      |                        |
      v                        v
  RCI Demand             TaxRates (previous tick)
      |
      v
LandValueSystem(65)
      |
      +-- Reads: Terrain bonuses
      +-- Reads: Transport accessibility, congestion
      +-- Reads: Disorder (PREVIOUS TICK)
      +-- Reads: Contamination (PREVIOUS TICK)
      |
      v
DisorderSystem(70)
      |
      +-- Reads: Building disorder sources
      +-- Reads: Population density
      +-- Reads: Land value (PREVIOUS TICK)
      +-- Reads: Enforcer coverage (ServicesSystem, Epic 9)
      |
      v
ContaminationSystem(80)
      |
      +-- Reads: Fabrication buildings (IContaminationSource)
      +-- Reads: Traffic volume (TransportSystem)
      +-- Reads: Polluting energy nexuses (IContaminationSource)
      +-- Reads: Terrain (blight_mires)
      |
      v
      |
   [Values stored for NEXT TICK]
```

### 4.2 Cross-System Query Pattern

```
+------------------+         +-------------------+         +--------------------+
| BuildingSystem   |         | TransportSystem   |         | TerrainSystem      |
|------------------|         |-------------------|         |--------------------|
| - Building count |         | - Traffic volume  |         | - Water distance   |
| - Capacity       |         | - Congestion      |         | - Terrain type     |
| - Contamination  |         | - Accessibility   |         | - Elevation        |
|   sources        |         |                   |         |                    |
+--------+---------+         +---------+---------+         +----------+---------+
         |                             |                              |
         |   IContaminationSource      |   ITransportProvider         |   ITerrainQueryable
         v                             v                              v
+--------+-----------------------------+------------------------------+---------+
|                                                                               |
|                          Epic 10 Simulation Systems                           |
|                                                                               |
|  +----------------+  +-------------+  +--------------+  +------------------+  |
|  | PopulationSys  |  | DemandSys   |  | LandValueSys |  | DisorderSys      |  |
|  |----------------|  |-------------|  |--------------|  |------------------|  |
|  | - Query Bldgs  |  | - Query Pop |  | - Query All  |  | - Query Bldgs    |  |
|  | - Query Energy |  | - Query Tax |  | - Double buf |  | - Query Pop      |  |
|  | - Query Fluid  |  |             |  |              |  | - Double buf     |  |
|  +----------------+  +-------------+  +--------------+  +------------------+  |
|                                                                               |
|  +------------------+                                                         |
|  | ContaminationSys |                                                         |
|  |------------------|                                                         |
|  | - Query Sources  |                                                         |
|  | - Query Traffic  |                                                         |
|  | - Double buf     |                                                         |
|  +------------------+                                                         |
|                                                                               |
+-------------------------------------------------------------------------------+
         |
         | IStatQueryable
         v
+------------------+
| UISystem (Ep 12) |
| - Population     |
| - RCI Demand     |
| - Land Value     |
| - Disorder       |
| - Contamination  |
+------------------+
```

---

## 5. Dense Grid Candidates

### 5.1 Current Dense Grid Exception Pattern

From patterns.yaml:
```yaml
dense_grid_exception:
  applies_to:
    - "TerrainGrid (Epic 3): 4 bytes/tile"
    - "BuildingGrid (Epic 4): 4 bytes/tile EntityID array"
    - "EnergyCoverageGrid (Epic 5): 1 byte/tile"
    - "FluidCoverageGrid (Epic 6): 1 byte/tile"
    - "PathwayGrid (Epic 7): 4 bytes/tile EntityID array"
    - "ProximityCache (Epic 7): 1 byte/tile distance"
    - "Future: contamination grid, land value grid"
```

### 5.2 Epic 10 Dense Grid Proposals

#### 5.2.1 ContaminationGrid

**Recommendation:** Dense 2D grid, 2 bytes/tile (double-buffered = 4 bytes/tile total)

```cpp
struct ContaminationTileData {
    uint8_t contamination_level;    // 0-255 total contamination
    uint8_t contamination_type : 4; // Primary type (industrial, traffic, energy, terrain)
    uint8_t decay_rate : 4;         // How fast this tile cleans up (0-15)
};
// Total: 2 bytes/tile, double-buffered = 4 bytes/tile
```

**Justification:**
1. Contamination covers every tile (not sparse)
2. Spread algorithm needs neighbor access (cache-friendly traversal)
3. Overlay rendering needs per-tile data
4. Per-entity ECS overhead prohibitive (24+ bytes vs 2 bytes)

**Memory Budget:**
| Map Size | Single Buffer | Double Buffer |
|----------|---------------|---------------|
| 128x128 | 32 KB | 64 KB |
| 256x256 | 128 KB | 256 KB |
| 512x512 | 512 KB | 1 MB |

#### 5.2.2 DisorderGrid

**Recommendation:** Dense 2D grid, 1 byte/tile (double-buffered = 2 bytes/tile total)

```cpp
struct DisorderTileData {
    uint8_t disorder_level; // 0-255 disorder/crime rate
};
// Total: 1 byte/tile, double-buffered = 2 bytes/tile
```

**Justification:**
1. Disorder spreads from tile to tile (dense access pattern)
2. Overlay rendering needs per-tile data
3. Enforcer coverage calculation iterates over tiles in radius

**Memory Budget:**
| Map Size | Single Buffer | Double Buffer |
|----------|---------------|---------------|
| 128x128 | 16 KB | 32 KB |
| 256x256 | 64 KB | 128 KB |
| 512x512 | 256 KB | 512 KB |

#### 5.2.3 LandValueGrid

**Recommendation:** Dense 2D grid, 1 byte/tile (double-buffered = 2 bytes/tile total)

```cpp
struct LandValueTileData {
    uint8_t land_value; // 0-255 sector value
};
// Total: 1 byte/tile, double-buffered = 2 bytes/tile
```

**Justification:**
1. Land value affects every tile
2. Building desirability calculation needs neighbor values
3. Overlay rendering needs per-tile data

**Memory Budget:**
| Map Size | Single Buffer | Double Buffer |
|----------|---------------|---------------|
| 128x128 | 16 KB | 32 KB |
| 256x256 | 64 KB | 128 KB |
| 512x512 | 256 KB | 512 KB |

### 5.3 Total Memory Budget (Epic 10 Grids)

| Grid | Bytes/Tile | 256x256 | 512x512 |
|------|------------|---------|---------|
| ContaminationGrid (x2) | 4 | 256 KB | 1 MB |
| DisorderGrid (x2) | 2 | 128 KB | 512 KB |
| LandValueGrid (x2) | 2 | 128 KB | 512 KB |
| **Epic 10 Total** | 8 | 512 KB | 2 MB |

Combined with existing grids (from Epics 3-7): ~5 MB total at 512x512. Acceptable.

### 5.4 Canon Update Required

Add to patterns.yaml dense_grid_exception.applies_to:
```yaml
- "ContaminationGrid (Epic 10): 2 bytes/tile (double-buffered) for contamination level and type"
- "DisorderGrid (Epic 10): 1 byte/tile (double-buffered) for disorder level"
- "LandValueGrid (Epic 10): 1 byte/tile (double-buffered) for land value"
```

---

## 6. Multiplayer Implications

### 6.1 Per-Player vs Global State

| Data | Scope | Rationale |
|------|-------|-----------|
| Population count | **Per-player** | Each overseer has their own population |
| Employment stats | **Per-player** | Jobs in your zones for your beings |
| RCI Demand | **Per-player** | Each overseer has own zone pressure |
| Land value | **Global (per-tile)** | Tile value is same regardless of viewer |
| Disorder | **Global (per-tile)** | Crime spreads across ownership boundaries |
| Contamination | **Global (per-tile)** | Pollution spreads across ownership boundaries |

### 6.2 Cross-Ownership Spread Effects

Disorder and contamination can spread across player boundaries:

```
Player 1's high-crime area:      Player 2's low-crime area:
+---+---+---+                    +---+---+---+
| 80| 90| 70|                    | 20| 15| 10|
+---+---+---+                    +---+---+---+
| 85|[95]| 65|  <-- Spreads -->  | 25|[30]| 12|
+---+---+---+                    +---+---+---+
| 75| 85| 60|                    | 18| 14| 8 |
+---+---+---+                    +---+---+---+
```

This creates competitive dynamics:
- Player A's fabrication zone can pollute Player B's habitation zone
- Player A's neglected area can spread crime to Player B
- Players may build "buffer zones" or complain to host

### 6.3 Authority Model

| Action | Authority | Notes |
|--------|-----------|-------|
| Population calculation | Server | Per-player authoritative |
| Demand calculation | Server | Per-player authoritative |
| Land value calculation | Server | Global grid, server computes |
| Disorder calculation | Server | Global grid, server computes |
| Contamination calculation | Server | Global grid, server computes |
| Simulation speed | Server (host) | All players see same speed |

### 6.4 Sync Requirements

| Data | Sync Method | Frequency |
|------|-------------|-----------|
| Population per player | Delta | Every tick (small data) |
| RCI demand per player | Delta | Every tick (small data) |
| LandValueGrid changes | Delta/RLE | Every 10-20 ticks (compressed) |
| DisorderGrid changes | Delta/RLE | Every 10-20 ticks (compressed) |
| ContaminationGrid changes | Delta/RLE | Every 10-20 ticks (compressed) |

**Grid sync optimization:** Grids change slowly. Use run-length encoding or delta compression. Only sync changed regions.

### 6.5 Ghost Town Process (AbandonmentSystem)

Per canon patterns.yaml, SimulationCore owns the ghost town decay process:

```
Active (PlayerID) -> Abandoned (50 cycles) -> GhostTown (100 cycles) -> Cleared (GAME_MASTER)
```

Implementation in SimulationCore:
1. Detect player abandonment (explicit restart or prolonged inactivity)
2. Mark player's tiles as OwnershipState::Abandoned
3. Each tick, progress decay timer
4. Buildings stop functioning, decay visually
5. After 50 cycles, transition to GhostTown state
6. Buildings crumble, infrastructure degrades
7. After 100 cycles, tiles cleared and return to GAME_MASTER
8. Available for purchase by other players

### 6.6 Late Join Population

When a new player joins:
- Start with 0 population
- Must build habitation zones to attract beings
- Beings migrate from "external" (not from other players' colonies)
- Migration rate affected by global colony attractiveness

---

## 7. Key Work Items

### 7.1 SimulationCore Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| SC-01 | SimulationCore base class | M | Tick scheduling, system registration, execution order |
| SC-02 | Simulation speed control | S | Pause, 1x, 2x, 3x speed multipliers |
| SC-03 | Time progression tracking | S | Tick count, cycle counter, phase tracking |
| SC-04 | TickStartEvent / TickCompleteEvent | S | Event definitions for tick lifecycle |
| SC-05 | AbandonmentSystem integration | L | Ghost town decay process implementation |
| SC-06 | Simulation state serialization | M | Save/load simulation time state |

### 7.2 PopulationSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| POP-01 | PopulationComponent definition | S | Per-building population capacity and current |
| POP-02 | EmploymentComponent definition | S | Jobs provided/filled per building |
| POP-03 | Population aggregation per player | M | Sum population across all buildings |
| POP-04 | Birth rate calculation | M | Based on habitability, services |
| POP-05 | Death rate calculation | M | Based on life expectancy, contamination |
| POP-06 | Migration calculation | L | In/out based on jobs, housing, disorder |
| POP-07 | Life expectancy calculation | M | Based on medical coverage, contamination |
| POP-08 | Employment tracking | M | Match population to jobs |
| POP-09 | IStatQueryable implementation | M | Population stats for UI |
| POP-10 | PopulationSystem tick (priority 50) | M | ISimulatable implementation |
| POP-11 | Network messages | S | Population sync protocol |

### 7.3 DemandSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| DEM-01 | Demand calculation algorithm | L | RCI formula based on SC2000 |
| DEM-02 | Demand factors aggregation | M | Collect factors from multiple sources |
| DEM-03 | Demand cap calculation | M | Max demand based on city size |
| DEM-04 | Tax rate effect on demand | S | Query EconomySystem (stub) |
| DEM-05 | Transport accessibility effect | S | Query TransportSystem |
| DEM-06 | DemandSystem tick (priority 52) | S | ISimulatable implementation |
| DEM-07 | Demand factors UI data | M | Breakdown for UI display |

### 7.4 LandValueSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| LV-01 | LandValueGrid implementation | M | Dense 1-byte grid with double buffer |
| LV-02 | Terrain value factors | M | Water proximity, terrain type bonuses |
| LV-03 | Disorder value penalty | M | Read previous tick's disorder |
| LV-04 | Contamination value penalty | M | Read previous tick's contamination |
| LV-05 | Transport accessibility bonus | M | Query TransportSystem |
| LV-06 | Services coverage bonus | S | Stub for Epic 9 integration |
| LV-07 | LandValueSystem tick (priority 65) | M | ISimulatable implementation |
| LV-08 | Previous tick query interface | S | get_land_value_at_previous_tick() |
| LV-09 | IStatQueryable implementation | S | Land value stats |
| LV-10 | Land value overlay data | S | For UI rendering |

### 7.5 DisorderSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| DIS-01 | DisorderGrid implementation | M | Dense 1-byte grid with double buffer |
| DIS-02 | DisorderComponent definition | S | For special disorder sources |
| DIS-03 | Base disorder generation | M | Per-building type disorder rates |
| DIS-04 | Population density effect | M | More population = more disorder |
| DIS-05 | Land value effect | M | Low value = more disorder (prev tick) |
| DIS-06 | Disorder spread algorithm | L | Diffusion to neighbors |
| DIS-07 | Enforcer coverage reduction | M | Stub for Epic 9 integration |
| DIS-08 | DisorderSystem tick (priority 70) | M | ISimulatable implementation |
| DIS-09 | Previous tick query interface | S | get_disorder_at_previous_tick() |
| DIS-10 | IStatQueryable implementation | S | Disorder stats |
| DIS-11 | Disorder overlay data | S | For UI rendering |

### 7.6 ContaminationSystem Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| CON-01 | ContaminationGrid implementation | M | Dense 2-byte grid with double buffer |
| CON-02 | ContaminationComponent definition | S | For tile-level contamination if needed |
| CON-03 | IContaminationSource aggregation | M | Query all contamination sources |
| CON-04 | Industrial pollution calculation | M | From fabrication buildings |
| CON-05 | Traffic pollution calculation | M | From TransportSystem traffic data |
| CON-06 | Energy pollution calculation | M | From polluting nexuses |
| CON-07 | Terrain pollution calculation | S | From blight_mires terrain |
| CON-08 | Contamination spread algorithm | L | Diffusion to neighbors |
| CON-09 | Contamination decay algorithm | M | Natural cleanup over time |
| CON-10 | ContaminationSystem tick (priority 80) | M | ISimulatable implementation |
| CON-11 | Previous tick query interface | S | get_contamination_at_previous_tick() |
| CON-12 | IStatQueryable implementation | S | Contamination stats |
| CON-13 | Contamination overlay data | S | For UI rendering |
| CON-14 | Contamination type breakdown | S | Industrial/traffic/energy/terrain |

### 7.7 Integration Work Items

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| INT-01 | IContaminationSource interface | S | Define interface for pollution sources |
| INT-02 | BuildingSystem IContaminationSource | M | Fabrication buildings implement interface |
| INT-03 | EnergySystem IContaminationSource | M | Polluting nexuses implement interface |
| INT-04 | ServicesSystem stubs | S | Stub interfaces for Epic 9 |
| INT-05 | EconomySystem stubs | S | Stub interfaces for Epic 11 |
| INT-06 | Integration tests | L | End-to-end simulation tests |

### 7.8 Work Item Size Summary

| Size | Count | Items |
|------|-------|-------|
| S | 20 | SC-02, SC-03, SC-04, POP-01, POP-02, POP-11, DEM-04, DEM-05, DEM-06, LV-06, LV-08, LV-09, LV-10, DIS-02, DIS-09, DIS-10, DIS-11, CON-02, CON-07, CON-11, CON-12, CON-13, CON-14, INT-01, INT-04, INT-05 |
| M | 28 | SC-01, SC-06, POP-03, POP-04, POP-05, POP-07, POP-08, POP-09, POP-10, DEM-02, DEM-03, DEM-07, LV-01, LV-02, LV-03, LV-04, LV-05, LV-07, DIS-01, DIS-03, DIS-04, DIS-05, DIS-07, DIS-08, CON-01, CON-03, CON-04, CON-05, CON-06, CON-09, CON-10, INT-02, INT-03 |
| L | 6 | SC-05, POP-06, DEM-01, DIS-06, CON-08, INT-06 |
| **Total** | **~54** | |

---

## 8. Questions for Other Agents

### @game-designer

1. **Population growth rate:** What should the base birth/death rates be? SimCity 2000 had relatively slow population changes. Should we match that or make it faster for more dynamic gameplay?

2. **Disorder threshold for migration:** At what disorder level should migration start being negatively affected? Proposed: 50/255 = minor effect, 150/255 = significant effect, 200/255 = major exodus.

3. **Cross-player contamination disputes:** When Player A's fabrication zone pollutes Player B's habitation zone, should there be any game mechanic to address this (fines, treaties, barriers)? Or is this just competitive multiplayer dynamics?

4. **Demand cap formula:** What determines the maximum possible demand? SimCity 2000 used city size and services. Proposed formula: `demand_cap = base_cap * (1 + log10(population/1000))`. Is this the right feel?

5. **Land value range for building upgrades:** At what land value thresholds should buildings consider upgrading vs downgrading? This affects the density progression.

6. **Life expectancy effects:** Should contamination reduce life expectancy directly (beings die faster) or indirectly (beings migrate away)? Or both?

### @population-engineer

7. **Employment matching algorithm:** Should we track which specific buildings provide jobs for which beings, or use aggregate matching (total jobs vs total population)?

8. **Migration source:** When beings migrate in, where do they come from? Should we model "external world" as infinite source, or use a supply curve that diminishes as your city grows?

9. **Commute patterns:** Should we model commute distance/time as a factor in employment? This could interact with TransportSystem congestion.

### @services-engineer

10. **Enforcer coverage algorithm:** How should enforcer coverage reduce disorder? Linear reduction in radius? Exponential falloff? What's the base effectiveness at maximum funding?

11. **Medical coverage algorithm:** How does medical coverage affect life expectancy? Global modifier or per-tile based on coverage?

### @economy-engineer

12. **Tax rate demand effects:** What's the sensitivity of demand to tax rates? SimCity 2000 had significant effects at extreme rates. Proposed: +/- 10% demand per 1% tax rate deviation from 7% baseline.

---

## 9. Risks and Concerns

### 9.1 Architectural Risks

**RISK: Circular Dependency Performance (HIGH)**
Double-buffering three grids means 6 grids in memory plus buffer swaps every tick. Buffer swap is O(1) pointer swap, but memory pressure increases.

**Mitigation:**
- Buffer swap is cheap (pointer swap, not data copy)
- Total grid memory is <5MB at largest map size
- Profile early to confirm

**RISK: Cross-Ownership Spread Griefing (HIGH)**
Players can intentionally create high-disorder or high-contamination zones near rivals to damage their land value and population.

**Mitigation:**
- This is intended competitive gameplay
- Document as expected behavior
- Consider optional "environmental treaty" mode in future
- Barriers (walls, parks) could reduce spread

**RISK: Ghost Town Process Complexity (MEDIUM)**
AbandonmentSystem interacts with many systems (buildings, population, economy). Abandoned tiles need special handling everywhere.

**Mitigation:**
- Clear OwnershipState enum with well-defined transitions
- Buildings check ownership state before processing
- Document all system interactions with abandoned state

### 9.2 Design Ambiguities

**AMBIGUITY: Demand Formula Complexity**
SimCity 2000's demand formula was complex and not fully documented. We need to decide:
- (A) Reverse-engineer SC2000 formula (complex, may have quirks)
- (B) Design simplified formula (easier to balance)
- (C) Hybrid (start simple, add complexity if needed)

**Recommendation:** Option (C). Start with simplified formula, iterate based on playtesting.

**AMBIGUITY: Spread Algorithm Parameters**
Disorder and contamination spread algorithms have tunable parameters:
- Spread rate (how much transfers to neighbors per tick)
- Decay rate (how fast contamination naturally cleans up)
- Barrier effects (do buildings block spread?)

**Recommendation:** Make all parameters data-driven for tuning. Document baseline values from SC2000 if known.

**AMBIGUITY: Population Granularity**
Options for tracking population:
- (A) Aggregate only (total population number)
- (B) Per-building (population in each habitation building)
- (C) Individual beings (actual entities per being)

**Recommendation:** Option (B). Per-building tracking enables:
- Building-level overcrowding
- Per-building energy/fluid effects
- Visible population distribution
- No individual being simulation (too expensive)

### 9.3 Technical Debt

**DEBT: Hardcoded Simulation Parameters**
Initial implementation will have hardcoded rates for:
- Birth/death rates
- Migration rates
- Disorder/contamination spread rates
- Land value factor weights

**Remediation:** Data-driven configuration file for all simulation parameters in Epic 16 or 17.

**DEBT: No Service Effectiveness Integration**
Epic 10 runs before Epic 9 (Services). Service effectiveness effects on disorder/contamination will be stubbed.

**Remediation:** Epic 9 implements ServicesSystem, Epic 10 stub replaced.

---

## 10. Interface Recommendations

### 10.1 IContaminationSource (Already in Canon)

From interfaces.yaml - already defined, but implementations needed:

```cpp
class IContaminationSource {
public:
    virtual uint32_t get_contamination_output() const = 0;
    virtual ContaminationType get_contamination_type() const = 0;
};
```

**Implementors to add:**
- Fabrication buildings (ContaminationType::Industrial)
- Carbon/Petrochemical/Gaseous nexuses (ContaminationType::Energy)
- High-traffic road segments (ContaminationType::Traffic)
- Blight_mires terrain (ContaminationType::Terrain)

### 10.2 IStatQueryable (Already in Canon)

From interfaces.yaml - already defined, implementations needed:

```cpp
class IStatQueryable {
public:
    virtual float get_stat_value(StatID stat_id) const = 0;
    virtual std::vector<float> get_stat_history(StatID stat_id, uint32_t periods) const = 0;
};
```

**Implementors:**
- PopulationSystem (total_population, birth_rate, death_rate, migration_rate, life_expectancy, unemployment_rate)
- DemandSystem (r_demand, c_demand, i_demand)
- LandValueSystem (average_land_value, min_land_value, max_land_value)
- DisorderSystem (total_disorder, average_disorder, max_disorder_tile)
- ContaminationSystem (total_contamination, average_contamination, by_type)

### 10.3 New Interface: ISimulationTime

**Recommendation:** Add interface for querying simulation time.

```cpp
class ISimulationTime {
public:
    virtual uint64_t get_current_tick() const = 0;
    virtual uint32_t get_current_cycle() const = 0;    // ~1 year
    virtual uint8_t get_current_phase() const = 0;     // Season/quarter
    virtual float get_simulation_speed() const = 0;    // 0=paused, 1=normal, 2=fast, 3=fastest
    virtual bool is_paused() const = 0;
};
```

**Implemented by:** SimulationCore

### 10.4 New Interface: IGridOverlay

**Recommendation:** Add interface for systems that provide overlay data.

```cpp
class IGridOverlay {
public:
    virtual const uint8_t* get_overlay_data() const = 0;  // Raw grid data
    virtual uint32_t get_overlay_width() const = 0;
    virtual uint32_t get_overlay_height() const = 0;
    virtual uint8_t get_value_at(int32_t x, int32_t y) const = 0;
    virtual OverlayColorScheme get_color_scheme() const = 0;  // For UI rendering
};
```

**Implemented by:** LandValueSystem, DisorderSystem, ContaminationSystem

### 10.5 Canon Updates Required

1. **interfaces.yaml:** Add ISimulationTime interface
2. **interfaces.yaml:** Add IGridOverlay interface
3. **interfaces.yaml:** Add DemandSystem tick_priority: 52
4. **interfaces.yaml:** Add LandValueSystem tick_priority: 65
5. **patterns.yaml:** Add ContaminationGrid, DisorderGrid, LandValueGrid to dense_grid_exception
6. **systems.yaml:** Add DemandSystem and LandValueSystem tick priorities

---

## 11. Dependencies

### 11.1 What Epic 10 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 3 | ITerrainQueryable | Water distance for land value, terrain type effects |
| Epic 3 | TerrainType (blight_mires) | Terrain contamination source |
| Epic 4 | BuildingSystem | Building population capacity, disorder sources |
| Epic 4 | BuildingComponent | Building type for contamination/disorder rates |
| Epic 5 | IEnergyProvider | Habitability check for population |
| Epic 5 | EnergyProducerComponent | Polluting nexus identification |
| Epic 6 | IFluidProvider | Habitability check for population |
| Epic 7 | ITransportProvider | Traffic volume for pollution, accessibility for demand |

### 11.2 What Later Epics Need from Epic 10

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 9 (Services) | Disorder/contamination queries | get_disorder_at(), get_contamination_at() |
| Epic 9 (Services) | Service effectiveness integration | Stub interfaces for enforcer/medical coverage |
| Epic 11 (Economy) | Tax base from population | PopulationSystem.get_total_population() |
| Epic 11 (Economy) | Land value for property taxes | LandValueSystem queries |
| Epic 12 (UI) | All overlay data | IGridOverlay implementations |
| Epic 12 (UI) | All statistics | IStatQueryable implementations |
| Epic 14 (Progression) | Population milestones | PopulationSystem.get_total_population() |

### 11.3 Forward Dependency Stubs

Epic 10 needs stubs for systems that don't exist yet:

| Stub | For Epic | Interface | Stub Behavior |
|------|----------|-----------|---------------|
| ServicesStub | Epic 9 | IServiceCoverage | Returns 0% coverage everywhere |
| EconomyStub | Epic 11 | ITaxRateProvider | Returns 7% default tax rate |

These stubs should be replaceable via dependency injection when the real systems are implemented.

---

## Appendix A: Component Definitions

### PopulationComponent

```cpp
struct PopulationComponent {
    uint32_t population_capacity = 0;   // Max beings this building can house
    uint32_t current_population = 0;    // Current beings in this building
    float habitability = 1.0f;          // 0.0-1.0 based on power/water/disorder
    float desirability = 0.5f;          // 0.0-1.0 for migration preference
};
```

### EmploymentComponent

```cpp
struct EmploymentComponent {
    uint32_t jobs_provided = 0;         // Jobs this building provides
    uint32_t jobs_filled = 0;           // Currently filled jobs
    uint8_t job_skill_level = 1;        // Skill level required (1-5)
};
```

### DisorderComponent

```cpp
// Optional: For special disorder sources beyond base building rates
struct DisorderComponent {
    uint32_t base_disorder_output = 0;  // Base disorder per tick
    float disorder_multiplier = 1.0f;   // Modified by conditions
};
```

### ContaminationComponent

```cpp
// For contamination sources not covered by IContaminationSource
struct ContaminationComponent {
    uint32_t contamination_output = 0;
    ContaminationType contamination_type = ContaminationType::Industrial;
    float output_multiplier = 1.0f;     // Modified by efficiency
};
```

---

## Appendix B: Simulation Formulas (Proposed)

### Population Growth

```
birth_rate = BASE_BIRTH_RATE * habitability_factor * services_factor
death_rate = BASE_DEATH_RATE / life_expectancy_factor
migration_in = BASE_MIGRATION * job_availability * housing_availability * (1 - disorder_factor)
migration_out = BASE_MIGRATION * disorder_factor * contamination_factor

net_population_change = birth_rate - death_rate + migration_in - migration_out
```

### RCI Demand

```
residential_demand = (jobs_available - population) * DEMAND_FACTOR - (tax_penalty)
commercial_demand = (population - commercial_capacity) * DEMAND_FACTOR - (tax_penalty)
industrial_demand = (commercial_capacity - industrial_capacity) * DEMAND_FACTOR - (tax_penalty)

demand = clamp(raw_demand, -100, demand_cap)
```

### Land Value

```
base_value = terrain_value + water_proximity_bonus
positive_factors = park_proximity + service_coverage + low_density_bonus
negative_factors = disorder_penalty + contamination_penalty + traffic_noise_penalty

land_value = clamp(base_value + positive_factors - negative_factors, 0, 255)
```

### Disorder Spread

```
new_disorder[x][y] = current_disorder[x][y] * RETENTION_FACTOR
                   + sum(neighbor_disorder) * SPREAD_FACTOR
                   - enforcer_coverage * ENFORCEMENT_FACTOR
                   + population_density * GENERATION_FACTOR

new_disorder = clamp(new_disorder, 0, 255)
```

### Contamination Spread

```
new_contamination[x][y] = current_contamination[x][y] * DECAY_FACTOR
                        + sum(source_outputs in radius) * GENERATION_FACTOR
                        + sum(neighbor_contamination) * SPREAD_FACTOR
                        - natural_decay * CLEANUP_FACTOR

new_contamination = clamp(new_contamination, 0, 255)
```

---

## Appendix C: Tick Order Quick Reference

```
Priority 5:  TerrainSystem         -- terrain data
Priority 10: EnergySystem          -- power state
Priority 20: FluidSystem           -- fluid state
Priority 30: ZoneSystem            -- zones
Priority 40: BuildingSystem        -- buildings, capacity
Priority 45: TransportSystem       -- traffic, congestion
Priority 47: RailSystem            -- transit coverage
Priority 50: PopulationSystem      -- population (Epic 10)
Priority 52: DemandSystem          -- RCI demand (Epic 10)  <-- NEW
Priority 60: EconomySystem         -- taxes (Epic 11)
Priority 65: LandValueSystem       -- land value (Epic 10)  <-- NEW
Priority 70: DisorderSystem        -- disorder (Epic 10)
Priority 80: ContaminationSystem   -- contamination (Epic 10)
```

---

**End of Systems Architect Analysis: Epic 10 - Simulation Core**
