# Systems Architect Analysis: Epic 12 - Information & UI Systems

**Author:** Systems Architect Agent
**Date:** 2026-01-29
**Canon Version:** 0.12.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 12 implements the **UISystem** - the player's window into the colony simulation. This epic is unique among all epics in that it **consumes data from almost every other system** while producing minimal data itself. The UISystem is primarily a read-only presentation layer that queries game state and renders it for player consumption.

Key architectural characteristics:
1. **Universal consumer:** UISystem queries data from ALL simulation systems (Epic 10), services (Epic 9), economy (Epic 11), infrastructure (Epics 5-7), and terrain (Epic 3)
2. **Dual UI modes:** Canon specifies two toggleable UI presentations - Classic (SimCity 2000-style) and Holographic (Sci-fi floating panels)
3. **Client-side only:** UI state is NOT networked - each player has independent UI state
4. **Event-driven outputs:** UISystem produces UI events (button clicked, tool selected) that drive player actions
5. **No tick priority:** UISystem renders between simulation ticks, not during simulation execution

This epic is heavily dependent on the **IGridOverlay** interface (Epic 10) for overlay visualization and **IStatQueryable** (multiple systems) for statistics display. It also relies on the **alien terminology** from terminology.yaml for all player-facing text.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Flow Analysis](#2-data-flow-analysis)
3. [Key Work Items](#3-key-work-items)
4. [Questions for Other Agents](#4-questions-for-other-agents)
5. [Integration Points](#5-integration-points)
6. [UI Architecture Decisions](#6-ui-architecture-decisions)
7. [Risks and Concerns](#7-risks-and-concerns)

---

## 1. System Boundaries

### 1.1 UISystem (type: ecs_system)

**Owns:**
- All UI rendering (panels, buttons, icons, text)
- UI state management (which panels open, current tool, active mode)
- Toolbars (command_array in alien terminology)
- Info panels (data_readout) - selected entity details, statistics
- Dialogs (modal windows for budget, ordinances, etc.)
- Query tool display (click-to-inspect building/tile info)
- Overlays (scan_layer) - crime, pollution, power, water, land value visualization
- Mini-map (sector_scan) - overview map with player territories
- Tool state (current placement tool, selection rectangle)

**Does NOT Own:**
- Game state (queries from simulation systems)
- Input handling (InputSystem provides raw events; UISystem interprets them)
- Actual building placement (sends commands to BuildingSystem)
- Simulation calculations (displays results only)
- Audio feedback (AudioSystem owns, UISystem triggers)

**Provides:**
- UI events (UIButtonClickedEvent, UIToolSelectedEvent, UIOverlayToggledEvent)
- Current tool state (for other systems to know what player is doing)
- Tooltip text queries (for context-sensitive help)

**Depends On:**
- **InputSystem** (Epic 0): Mouse/keyboard events for UI interaction
- **RenderingSystem** (Epic 2): 2D UI rendering on top of 3D scene
- **TerrainSystem** (Epic 3): ITerrainQueryable for tile info display
- **ZoneSystem** (Epic 4): Zone type queries for display
- **BuildingSystem** (Epic 4): Building info for query tool
- **EnergySystem** (Epic 5): IEnergyProvider, IGridOverlay for power overlay
- **FluidSystem** (Epic 6): IFluidProvider, IGridOverlay for water overlay
- **TransportSystem** (Epic 7): ITransportProvider for traffic/road info
- **ServicesSystem** (Epic 9): Service coverage for overlay display
- **SimulationCore** (Epic 10): ISimulationTime for date/speed display
- **PopulationSystem** (Epic 10): IStatQueryable for population stats
- **DemandSystem** (Epic 10): RCI demand meter display
- **LandValueSystem** (Epic 10): IGridOverlay for land value overlay
- **DisorderSystem** (Epic 10): IGridOverlay for disorder overlay
- **ContaminationSystem** (Epic 10): IGridOverlay for contamination overlay
- **EconomySystem** (Epic 11): IEconomyQueryable for budget window

### 1.2 Boundary Summary Table

| Concern | UISystem | InputSystem | RenderingSystem | Simulation Systems |
|---------|----------|-------------|-----------------|-------------------|
| Panel layout | **Owns** | - | - | - |
| Button clicks | **Owns** | Provides raw events | - | - |
| Overlay rendering | **Owns** | - | Provides render API | - |
| Overlay data | Queries | - | - | **Own (IGridOverlay)** |
| Tool selection | **Owns** | - | - | - |
| Build commands | Emits events | - | - | Receives commands |
| Statistics display | **Owns** | - | - | **Own (IStatQueryable)** |

---

## 2. Data Flow Analysis

### 2.1 UI Data Flow Pattern

```
                    INPUT FLOW
                    ==========

InputSystem (Epic 0)
    |
    | Raw mouse/keyboard events
    | (SDL_Event)
    v
+--------------------------------------------------+
|                   UISystem                        |
|--------------------------------------------------|
| 1. Check if input hits UI element                |
|    - Yes: Handle UI interaction                  |
|    - No: Pass through to game world input        |
|                                                   |
| 2. UI interaction processing:                    |
|    - Button click -> emit UIButtonClickedEvent   |
|    - Tool select -> update current_tool state    |
|    - Panel drag -> update panel position         |
|    - Slider change -> emit value change event    |
+--------------------------------------------------+
    |
    | UIToolSelectedEvent, UICommandEvent
    v
BuildingSystem, ZoneSystem, etc.
    (Execute player commands)
```

### 2.2 Display Data Flow

```
                    DISPLAY DATA FLOW
                    ==================

All Simulation Systems (Epics 3-11)
    |
    | IStatQueryable, IGridOverlay, IEnergyProvider,
    | IFluidProvider, ITransportProvider, IEconomyQueryable
    v
+--------------------------------------------------+
|                   UISystem                        |
|--------------------------------------------------|
| Query current game state:                        |
|   - Population stats (PopulationSystem)          |
|   - RCI demand (DemandSystem)                    |
|   - Treasury/budget (EconomySystem)              |
|   - Overlay grids (Disorder/Contamination/Value) |
|   - Selected entity info (BuildingSystem)        |
|   - Simulation time (SimulationCore)             |
|                                                   |
| Transform data for display:                      |
|   - Apply alien terminology (terminology.yaml)   |
|   - Format numbers (1234 -> "1,234")            |
|   - Apply color coding (red=bad, green=good)    |
|   - Scale overlay values to colors              |
+--------------------------------------------------+
    |
    | Render commands
    v
RenderingSystem (Epic 2)
    (2D UI layer on top of 3D scene)
```

### 2.3 Overlay Rendering Flow

```
                    OVERLAY RENDERING
                    ==================

IGridOverlay implementors:
+-------------------+  +-------------------+  +-------------------+
| LandValueSystem   |  | DisorderSystem    |  | ContaminationSys  |
| - get_overlay_data|  | - get_overlay_data|  | - get_overlay_data|
| - get_color_scheme|  | - get_color_scheme|  | - get_color_scheme|
+-------------------+  +-------------------+  +-------------------+
         |                     |                       |
         +---------------------+-----------------------+
                               |
                               v
+----------------------------------------------------------+
|                      UISystem                             |
|----------------------------------------------------------|
| Overlay Manager:                                          |
|   - current_overlay: OverlayType (none, disorder, etc.)  |
|   - overlay_opacity: 0.0 - 1.0                           |
|                                                           |
| Rendering logic:                                          |
|   1. Get raw overlay data (uint8_t* grid)                |
|   2. Get color scheme (heat_map, green_red, etc.)        |
|   3. Map value (0-255) to color using scheme             |
|   4. Render semi-transparent colored tiles over terrain  |
+----------------------------------------------------------+
```

### 2.4 Query Tool Data Flow

```
                    QUERY TOOL (Click-to-Inspect)
                    ==============================

User clicks on tile:
    |
    v
+----------------------------------------------------------+
|                      UISystem                             |
|----------------------------------------------------------|
| 1. Convert screen coords to grid position (CameraSystem) |
| 2. Query all systems for data at position:               |
|                                                           |
|    TerrainSystem.get_terrain_type(x, y)                  |
|    TerrainSystem.get_elevation(x, y)                     |
|    BuildingSystem.get_building_at(x, y)                  |
|    ZoneSystem.get_zone_at(x, y)                          |
|    EnergySystem.is_powered_at(x, y, owner)               |
|    FluidSystem.has_fluid_at(x, y, owner)                 |
|    TransportSystem.get_nearest_road_distance(x, y)       |
|    DisorderSystem.get_disorder_at(x, y)                  |
|    ContaminationSystem.get_contamination_at(x, y)        |
|    LandValueSystem.get_land_value_at(x, y)               |
|    OwnershipComponent.owner (via ECS query)              |
|                                                           |
| 3. Format into data_readout panel display                |
| 4. Apply alien terminology                               |
+----------------------------------------------------------+
```

---

## 3. Key Work Items

### Phase 1: UI Framework Foundation (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-001 | UI rendering layer integration | M | Hook into RenderingSystem for 2D UI on top of 3D scene |
| UI-002 | UI element base classes | M | Panel, Button, Label, Slider, ProgressBar base types |
| UI-003 | UI state management | M | Track open panels, current tool, active mode (classic/holo) |
| UI-004 | Input routing to UI | M | UISystem consumes relevant input events, passes through others |
| UI-005 | UI mode toggle system | S | Switch between Classic and Holographic modes |

### Phase 2: Toolbar and Tools (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-006 | Command array (toolbar) layout - Classic | M | Top toolbar with zone/infrastructure/service tool buttons |
| UI-007 | Command array (toolbar) layout - Holographic | M | Radial/floating tool menu for holo mode |
| UI-008 | Tool state management | M | Current selected tool, tool parameters, cursor mode |
| UI-009 | Zone placement tool UI | M | Zone type selector, drag-to-place preview |
| UI-010 | Infrastructure tool UI | M | Road/conduit/pipe placement preview |

### Phase 3: Information Panels (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-011 | Data readout (info panel) - Classic | M | Bottom panel showing selected entity details |
| UI-012 | Data readout (info panel) - Holographic | M | Floating translucent info panel for holo mode |
| UI-013 | Query tool implementation | M | Click-to-inspect tile/building, aggregate all system queries |
| UI-014 | RCI demand meter display | S | Visual R/C/I demand bars with zone_pressure values |
| UI-015 | Population/date/treasury status bar | M | Always-visible top stats: population, current cycle, credits |
| UI-016 | Simulation speed controls | S | Pause/play/fast/ultra buttons with ISimulationTime display |

### Phase 4: Overlay System (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-017 | Overlay manager | M | Toggle overlays, manage current overlay state |
| UI-018 | Overlay rendering | L | Render IGridOverlay data as semi-transparent colored tiles |
| UI-019 | Overlay toggle buttons | S | Buttons for each overlay type (disorder, contamination, value, power, water) |
| UI-020 | Overlay color scheme application | M | Map uint8_t values to colors using OverlayColorScheme |

### Phase 5: Mini-Map / Sector Scan (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-021 | Sector scan (mini-map) rendering | M | Small overview map showing terrain and buildings |
| UI-022 | Player territory visualization | S | Color-code tiles by owner on mini-map |
| UI-023 | Mini-map click navigation | S | Click mini-map to move camera to location |

### Phase 6: Budget and Economy UI (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-024 | Budget window (colony_treasury panel) | L | Income/expense breakdown, funding sliders, bond management |
| UI-025 | Tribute rate sliders | M | Per-zone tribute rate adjustment (0-20%) |
| UI-026 | Service funding sliders | M | Per-service funding level adjustment (0-150%) |

### Phase 7: Alien Terminology Integration (2 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| UI-027 | Terminology lookup system | M | Load terminology.yaml, provide lookup API for UI text |
| UI-028 | Apply terminology to all UI text | S | Replace human terms with alien equivalents throughout |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 6 | UI-005, UI-014, UI-016, UI-019, UI-022, UI-023, UI-028 |
| M | 18 | UI-001, UI-002, UI-003, UI-004, UI-006, UI-007, UI-008, UI-009, UI-010, UI-011, UI-012, UI-013, UI-015, UI-017, UI-020, UI-021, UI-025, UI-026, UI-027 |
| L | 2 | UI-018, UI-024 |
| **Total** | **26** | |

---

## 4. Questions for Other Agents

### @game-designer

1. **Overlay priority when multiple active:** Can players have multiple overlays active simultaneously (e.g., power + disorder)? Or is it mutually exclusive with a single active overlay?
   - Proposed: Single active overlay at a time, toggle replaces current

2. **Info panel depth:** How much detail should the query tool show? Options:
   - (A) Summary only (zone type, powered, watered)
   - (B) Full stats (exact population, exact disorder value, etc.)
   - (C) Progressive reveal (summary by default, expand for details)
   - Proposed: (C) for best UX

3. **Classic vs Holographic mode differences:** Beyond visual styling, should there be any functional differences between modes? Or purely cosmetic?
   - Proposed: Purely cosmetic - identical functionality

4. **Mini-map interaction level:** Should mini-map allow:
   - (A) View only + click to navigate
   - (B) Full tool use (place buildings via mini-map)
   - (C) View only, no interaction
   - Proposed: (A) - view + navigate

5. **Tooltip content:** What information should tooltips show on hover? Building type? Stats? Cost?
   - Proposed: Building name (alien term), status icons (powered/watered), brief description

6. **UI responsiveness to multiplayer actions:** When another player builds something, should there be any notification in the UI? Toast message? Audio cue?

### @ui-developer

7. **UI framework choice:** Should we use:
   - (A) Custom immediate-mode UI (like Dear ImGui)
   - (B) Custom retained-mode UI (traditional widget tree)
   - (C) Existing library (SDL_ttf + custom layout)
   - Trade-offs needed for dual Classic/Holographic modes

8. **Panel dragging/docking:** Should panels be:
   - (A) Fixed positions per mode
   - (B) Freely draggable with snap
   - (C) Dockable to screen edges
   - Proposed: (A) for Classic, (B) for Holographic

9. **Overlay rendering technique:** Best approach for semi-transparent overlay:
   - (A) Texture atlas with pre-baked gradients
   - (B) Per-tile shader coloring
   - (C) Separate 2D pass over terrain
   - Performance vs quality considerations

10. **Animation requirements:** What UI animations are expected?
    - Panel slide in/out
    - Button hover/click feedback
    - Overlay fade transitions
    - RCI meter movement

---

## 5. Integration Points

### 5.1 Interface Consumption Summary

| Interface | Provider | Epic | Data Used |
|-----------|----------|------|-----------|
| ITerrainQueryable | TerrainSystem | 3 | Tile type, elevation for query tool |
| IEnergyProvider | EnergySystem | 5 | Power status for query tool, power overlay |
| IFluidProvider | FluidSystem | 6 | Water status for query tool, water overlay |
| ITransportProvider | TransportSystem | 7 | Road distance, congestion for query tool |
| IGridOverlay | LandValueSystem | 10 | Land value overlay data |
| IGridOverlay | DisorderSystem | 10 | Disorder overlay data |
| IGridOverlay | ContaminationSystem | 10 | Contamination overlay data |
| IStatQueryable | PopulationSystem | 10 | Population stats for status bar |
| IStatQueryable | DemandSystem | 10 | RCI demand values for meter |
| IStatQueryable | EconomySystem | 11 | Financial stats for budget |
| ISimulationTime | SimulationCore | 10 | Current tick/cycle/phase, speed |
| IEconomyQueryable | EconomySystem | 11 | Treasury, tribute rates, funding levels |

### 5.2 Event Production

UISystem produces events that other systems consume:

| Event | Consumers | Purpose |
|-------|-----------|---------|
| UIToolSelectedEvent | RenderingSystem | Change cursor/preview |
| UIBuildRequestEvent | BuildingSystem | Player wants to build |
| UIZoneRequestEvent | ZoneSystem | Player wants to zone |
| UIDemolishRequestEvent | BuildingSystem | Player wants to demolish |
| UITributeRateChangedEvent | EconomySystem | Player adjusted tax |
| UIFundingChangedEvent | EconomySystem | Player adjusted funding |
| UISpeedChangeRequestEvent | SimulationCore | Player changed speed |
| UIOverlayToggledEvent | (internal) | Track overlay state |

### 5.3 Canon Terminology Integration

UISystem must apply terminology.yaml transformations:

| Internal Term | Display Term (Alien) |
|---------------|---------------------|
| residential | habitation |
| commercial | exchange |
| industrial | fabrication |
| power | energy |
| water | fluid |
| road | pathway |
| crime | disorder |
| pollution | contamination |
| land_value | sector_value |
| year | cycle |
| month | phase |
| taxes | tribute |
| budget | colony_treasury |
| minimap | sector_scan |
| toolbar | command_array |
| info_panel | data_readout |
| overlay | scan_layer |

### 5.4 UI Mode Visual Mapping

| Element | Classic Mode | Holographic Mode |
|---------|--------------|------------------|
| Toolbar | Top horizontal bar | Floating radial menu |
| Info panel | Bottom dock | Floating translucent panel |
| Mini-map | Corner fixed | Holographic projection |
| Buttons | Opaque with borders | Glowing translucent |
| Text | Standard font | Sci-fi/alien glyphs accents |
| Colors | Dark backgrounds | Bioluminescent glows |

---

## 6. UI Architecture Decisions

### 6.1 Rendering Layer Architecture

```
+-------------------------------------------------------+
|                    Screen Output                       |
+-------------------------------------------------------+
        ^                       ^
        |                       |
+-------+-------+       +-------+-------+
|   3D Scene    |       |   2D UI Layer |
| (RenderingSystem)     |  (UISystem)    |
+---------------+       +---------------+
        ^                       ^
        |                       |
   SDL_GPU 3D               SDL_GPU 2D
   Render Pass              Render Pass
```

**Decision:** UI renders as a separate 2D pass after 3D scene, using orthographic projection.

### 6.2 UI State Model

```cpp
struct UIState {
    // Mode
    UIMode current_mode = UIMode::Classic;  // Classic or Holographic

    // Tools
    ToolType current_tool = ToolType::Select;
    ZoneType zone_brush_type = ZoneType::None;
    InfraType infra_brush_type = InfraType::None;

    // Panels
    bool budget_panel_open = false;
    bool ordinance_panel_open = false;
    std::optional<EntityID> selected_entity;
    std::optional<GridPosition> query_position;

    // Overlays
    OverlayType current_overlay = OverlayType::None;
    float overlay_opacity = 0.7f;

    // Drag state
    std::optional<GridPosition> drag_start;
    std::optional<GridPosition> drag_current;
};
```

### 6.3 IGridOverlay Color Scheme Implementation

Per interfaces.yaml, color schemes map 0-255 values to colors:

```cpp
enum class OverlayColorScheme {
    HeatMap,      // Red (high) to blue (low)
    GreenRed,     // Green (good) to red (bad)
    PurpleYellow, // Purple (low) to yellow (high) - contamination
    Coverage      // Binary or stepped coverage visualization
};

Color apply_color_scheme(OverlayColorScheme scheme, uint8_t value) {
    float t = value / 255.0f;
    switch (scheme) {
        case HeatMap:
            return Color::lerp(Color::Blue, Color::Red, t);
        case GreenRed:
            return Color::lerp(Color::Green, Color::Red, t);
        case PurpleYellow:
            return Color::lerp(Color::Purple, Color::Yellow, t);
        case Coverage:
            return (value > 128) ? Color::Green : Color::Gray;
    }
}
```

### 6.4 Query Tool Architecture

```cpp
struct TileQueryResult {
    // Position
    GridPosition position;

    // Terrain
    TerrainType terrain_type;
    uint8_t elevation;

    // Ownership
    PlayerID owner;
    OwnershipState ownership_state;

    // Building (if present)
    std::optional<BuildingInfo> building;

    // Zone (if present)
    std::optional<ZoneType> zone;

    // Utilities
    bool has_power;
    bool has_fluid;
    uint8_t road_distance;

    // Simulation values
    uint8_t disorder_level;
    uint8_t contamination_level;
    uint8_t land_value;
};

TileQueryResult UISystem::query_tile(GridPosition pos) {
    TileQueryResult result;
    result.position = pos;

    // Query each system
    result.terrain_type = terrain_system->get_terrain_type(pos.x, pos.y);
    result.elevation = terrain_system->get_elevation(pos.x, pos.y);
    result.building = building_system->get_building_info_at(pos);
    result.zone = zone_system->get_zone_at(pos);
    result.has_power = energy_system->is_powered_at(pos.x, pos.y, local_player);
    result.has_fluid = fluid_system->has_fluid_at(pos.x, pos.y, local_player);
    result.road_distance = transport_system->get_nearest_road_distance(pos.x, pos.y);
    result.disorder_level = disorder_system->get_disorder_at(pos.x, pos.y);
    result.contamination_level = contamination_system->get_contamination_at(pos.x, pos.y);
    result.land_value = land_value_system->get_land_value_at(pos.x, pos.y);
    // ... ownership from OwnershipComponent

    return result;
}
```

---

## 7. Risks and Concerns

### 7.1 Architectural Risks

**RISK: Performance of Overlay Rendering (MEDIUM)**
Rendering overlay colors for every visible tile could be expensive, especially at maximum zoom-out on large maps.

**Mitigation:**
- Render overlay as texture, update only when data changes
- Use chunked rendering matching terrain chunks (32x32)
- Level-of-detail: coarser overlay at distance
- Profile early with 512x512 maps

**RISK: UI Framework Complexity for Dual Modes (HIGH)**
Maintaining two complete visual styles (Classic and Holographic) doubles the layout/styling work.

**Mitigation:**
- Separate content from presentation
- Use style sheets or skinning system
- Share logic, vary only rendering
- Consider implementing Classic first, Holographic as polish (Epic 17?)

**RISK: Query Tool Performance with Many Systems (LOW)**
Query tool aggregates data from 10+ systems on every click.

**Mitigation:**
- Queries are fast O(1) lookups (ECS queries, grid lookups)
- Happens only on explicit user action (click)
- Cache result until next click

### 7.2 Design Ambiguities

**AMBIGUITY: Budget Window Complexity**
The budget window needs to display:
- Income breakdown by zone type
- Expense breakdown by category
- Funding sliders for 4 service types
- Active bonds list
- Tribute rate sliders for 3 zones

**Question:** Is this a single complex panel or multiple sub-panels?
**Recommendation:** Tabbed interface within single budget panel (Income | Expenses | Services | Bonds)

**AMBIGUITY: Overlay Data Update Frequency**
IGridOverlay provides raw grid data. How often should UI refresh overlay display?
- (A) Every frame (smooth but expensive)
- (B) Every simulation tick (20Hz)
- (C) Only when overlay data changes (event-driven)

**Recommendation:** Option (B) - update overlay texture at tick boundaries, render cached texture every frame.

**AMBIGUITY: Panel State Persistence**
Should UI state (which panels open, positions) persist across sessions?
- (A) Reset to defaults on load
- (B) Save in user preferences (not game save)
- (C) Save with game state

**Recommendation:** Option (B) - UI preferences separate from game state.

### 7.3 Technical Debt

**DEBT: Hardcoded UI Layouts**
Initial implementation will have hardcoded panel sizes and positions for Classic mode.

**Remediation:** Data-driven UI layout system in Epic 17 polish phase, enabling easier Holographic mode implementation.

**DEBT: Single Font/Style**
Initial UI will use one font, one button style.

**Remediation:** Style system for Classic vs Holographic themes in polish phase.

---

## Appendix A: UI Event Definitions

### UIToolSelectedEvent

```cpp
struct UIToolSelectedEvent {
    ToolType tool_type;          // Select, Zone, Road, Demolish, Query, etc.
    std::optional<ZoneType> zone_type;     // If tool is Zone
    std::optional<InfraType> infra_type;   // If tool is Infrastructure
};
```

### UIBuildRequestEvent

```cpp
struct UIBuildRequestEvent {
    GridPosition position;
    BuildingType building_type;  // For direct placement buildings (service, power plant)
    PlayerID requester;
};
```

### UIZoneRequestEvent

```cpp
struct UIZoneRequestEvent {
    GridRect area;               // Drag-selected area
    ZoneType zone_type;
    ZoneDensity density;
    PlayerID requester;
};
```

### UITributeRateChangedEvent

```cpp
struct UITributeRateChangedEvent {
    ZoneBuildingType zone_type;  // habitation, exchange, fabrication
    uint8_t new_rate;            // 0-20
    PlayerID player;
};
```

### UIFundingChangedEvent

```cpp
struct UIFundingChangedEvent {
    ServiceType service_type;    // enforcer, hazard_response, medical, education
    uint8_t new_level;           // 0-150
    PlayerID player;
};
```

---

## Appendix B: Overlay Types and Sources

| Overlay Type | Data Source | Color Scheme | Purpose |
|--------------|-------------|--------------|---------|
| None | - | - | Default: no overlay |
| Disorder | DisorderSystem (IGridOverlay) | GreenRed | Show crime levels |
| Contamination | ContaminationSystem (IGridOverlay) | PurpleYellow | Show pollution levels |
| LandValue | LandValueSystem (IGridOverlay) | HeatMap | Show sector value |
| EnergyCoverage | EnergySystem | Coverage | Show powered areas |
| FluidCoverage | FluidSystem | Coverage | Show watered areas |
| ServiceCoverage | ServicesSystem | Coverage | Show service reach |
| Traffic | TransportSystem | HeatMap | Show congestion |

---

## Appendix C: Terminology Reference (UI-relevant subset)

```yaml
# From terminology.yaml - for UI text display

# Zones
residential: habitation
commercial: exchange
industrial: fabrication

# Infrastructure
power: energy
power_line: energy_conduit
water: fluid
pipes: fluid_conduits
road: pathway
traffic: flow

# Simulation
crime: disorder
pollution: contamination
land_value: sector_value
happiness: harmony

# Time
year: cycle
month: phase

# Economy
taxes: tribute
budget: colony_treasury
loan: credit_advance

# UI Elements
minimap: sector_scan
toolbar: command_array
info_panel: data_readout
overlay: scan_layer
classic_mode: legacy_interface
holographic_mode: holo_interface
```

---

## Appendix D: Mini-Map Rendering

The sector_scan (mini-map) provides a zoomed-out view of the entire map:

```cpp
struct MiniMapRenderer {
    // Render at reduced resolution (e.g., 1 pixel per 4 tiles)
    static constexpr int TILES_PER_PIXEL = 4;

    void render(SDL_Texture* target) {
        for (int y = 0; y < map_height; y += TILES_PER_PIXEL) {
            for (int x = 0; x < map_width; x += TILES_PER_PIXEL) {
                // Determine dominant feature in this area
                Color pixel_color = sample_area(x, y, TILES_PER_PIXEL);
                // Set pixel
            }
        }
    }

    Color sample_area(int x, int y, int size) {
        // Priority: Building > Zone > Terrain
        // Color by ownership for multiplayer
        if (has_building(x, y)) {
            return get_owner_color(get_owner(x, y));
        }
        if (has_zone(x, y)) {
            return get_zone_color(get_zone(x, y));
        }
        return get_terrain_color(get_terrain(x, y));
    }
};
```

---

**End of Systems Architect Analysis: Epic 12 - Information & UI Systems**
