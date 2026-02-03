# Epic 12: Information & UI Systems - Tickets

**Epic:** 12 - Information & UI Systems
**Created:** 2026-01-29
**Canon Version:** 0.13.0
**Total Tickets:** 27 (target: 25-30)

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.12.0 | Initial ticket creation |
| 2026-01-29 | canon-verification (v0.12.0 → v0.13.0) | UI events aligned |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. UI event types from interfaces.yaml:
> - E12-011 (Tool State Machine) should emit UIToolSelectedEvent on tool change
> - E12-022 already emits UITributeRateChangedEvent/UIFundingChangedEvent ✓
> - Build/zone placement flows should emit UIBuildRequestEvent/UIZoneRequestEvent
> - E12-016 (Scan Layer Toggle) should emit UIOverlayToggledEvent
>
> IGridOverlay integration (E12-017) matches interfaces.yaml. Terminology per terminology.yaml (probe_function, alert_pulse, comm_panel, etc.).

---

## Ticket Summary Table

| ID | Title | Size | Phase | Blocked By |
|----|-------|------|-------|------------|
| E12-001 | Widget Base Class and Hierarchy | M | 1 | - |
| E12-002 | UIManager Core and State Management | M | 1 | E12-001 |
| E12-003 | UIRenderer Interface and Skinning System | M | 1 | - |
| E12-004 | Classic Renderer Implementation | M | 1 | E12-003 |
| E12-005 | Holographic Renderer Implementation | L | 1 | E12-003 |
| E12-006 | Core Widget Library (Button, Label, Panel, Icon) | M | 2 | E12-001 |
| E12-007 | ProgressBar and Demand Meter Widgets | S | 2 | E12-006 |
| E12-008 | Input Routing and Hit Testing | M | 2 | E12-002 |
| E12-009 | Command Array - Classic Mode Layout | M | 3 | E12-006 |
| E12-010 | Command Array - Holographic Radial Menu | M | 3 | E12-006 |
| E12-011 | Tool State Machine and Cursor Display | M | 3 | E12-008 |
| E12-012 | Data Readout Panel (Query Tool Display) | M | 4 | E12-006 |
| E12-013 | Probe Function (Query Tool) Implementation | M | 4 | E12-012 |
| E12-014 | Colony Status Bar (Population, Treasury, Date) | M | 4 | E12-006 |
| E12-015 | Zone Pressure Indicator (RCI Demand) | S | 4 | E12-007 |
| E12-016 | Scan Layer Manager and Toggle System | M | 5 | E12-002 |
| E12-017 | Scan Layer Rendering (IGridOverlay Integration) | L | 5 | E12-016 |
| E12-018 | Scan Layer Color Schemes | S | 5 | E12-017 |
| E12-019 | Sector Scan Rendering (Minimap) | L | 6 | E12-002 |
| E12-020 | Sector Scan Click Navigation | S | 6 | E12-019 |
| E12-021 | Colony Treasury Panel (Budget Window) | L | 7 | E12-006 |
| E12-022 | Tribute Rate and Funding Sliders | M | 7 | E12-021 |
| E12-023 | Alert Pulse Notification System | M | 8 | E12-006 |
| E12-024 | Keyboard Shortcut System | M | 8 | E12-008 |
| E12-025 | Terminology Lookup and Application | S | 8 | - |
| E12-026 | Unit Tests for Widget System | L | 9 | E12-008 |
| E12-027 | Integration Tests with Simulation Systems | L | 9 | E12-017, E12-021 |

---

## Size Distribution

| Size | Count | Tickets |
|------|-------|---------|
| S | 5 | E12-007, E12-015, E12-018, E12-020, E12-025 |
| M | 16 | E12-001, E12-002, E12-003, E12-004, E12-006, E12-008, E12-009, E12-010, E12-011, E12-012, E12-013, E12-014, E12-016, E12-022, E12-023, E12-024 |
| L | 6 | E12-005, E12-017, E12-019, E12-021, E12-026, E12-027 |

**Total: 27 tickets**

---

## Critical Path

```
Phase 1 (Foundation):
  E12-001 -> E12-002 -> E12-008
  E12-003 -> E12-004
           -> E12-005

Phase 2 (Core Widgets):
  E12-006 -> E12-007

Phase 3 (Command Array):
  E12-009 (Classic)
  E12-010 (Holographic)
  E12-011 (Tool State)

Phase 4 (Info Panels):
  E12-012 -> E12-013
  E12-014
  E12-015

Phase 5 (Scan Layers):
  E12-016 -> E12-017 -> E12-018

Phase 6 (Sector Scan):
  E12-019 -> E12-020

Phase 7 (Budget):
  E12-021 -> E12-022

Phase 8 (Polish):
  E12-023, E12-024, E12-025

Phase 9 (Testing):
  E12-026, E12-027
```

**Critical Path:** E12-001 -> E12-002 -> E12-008 -> E12-006 -> E12-012 -> E12-013 -> E12-027

---

## Phase 1: UI Framework Foundation

### E12-001: Widget Base Class and Hierarchy

**Size:** M (Medium)
**Blocked By:** None

**Description:**
Define the base Widget class that all UI elements inherit from. Includes position, size, visibility, parent-child hierarchy, and hit testing.

**Widget Base Class:**
```cpp
class Widget {
public:
    virtual ~Widget() = default;

    // Lifecycle
    virtual void update(float delta_time);
    virtual void render(UIRenderer* renderer);

    // Layout
    Rect bounds;                    // Position and size in parent space
    Rect screen_bounds;             // Position and size in screen space
    bool visible = true;
    bool enabled = true;

    // Hierarchy
    Widget* parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;

    void add_child(std::unique_ptr<Widget> child);
    void remove_child(Widget* child);

    // Hit testing
    virtual bool hit_test(int x, int y);

    // Events
    virtual void on_mouse_enter();
    virtual void on_mouse_leave();
    virtual void on_mouse_down(int button, int x, int y);
    virtual void on_mouse_up(int button, int x, int y);

protected:
    bool hovered = false;
    bool pressed = false;
};
```

**Acceptance Criteria:**
- [ ] Widget base class with all lifecycle methods
- [ ] Parent-child hierarchy management
- [ ] Bounds calculation (local to screen space)
- [ ] Basic hit testing implementation
- [ ] Mouse event virtual methods

---

### E12-002: UIManager Core and State Management

**Size:** M (Medium)
**Blocked By:** E12-001

**Description:**
Create the UIManager class that owns the widget tree, manages UI state, and handles mode switching between Legacy and Holo interfaces.

**UIState Structure:**
```cpp
struct UIState {
    // Mode
    UIMode current_mode = UIMode::Legacy;  // legacy_interface or holo_interface

    // Tools
    ToolType current_tool = ToolType::Select;
    ZoneType zone_brush_type = ZoneType::None;
    InfraType infra_brush_type = InfraType::None;

    // Panels
    bool budget_panel_open = false;
    std::optional<EntityID> selected_entity;
    std::optional<GridPosition> query_position;

    // Overlays
    OverlayType current_overlay = OverlayType::None;
    float overlay_opacity = 0.7f;

    // Notifications
    std::deque<AlertPulse> active_alerts;
};
```

**Acceptance Criteria:**
- [ ] UIManager class with widget tree root
- [ ] UIState struct for all UI state tracking
- [ ] Mode switching between Legacy and Holo
- [ ] Tool selection state management
- [ ] Panel open/close tracking
- [ ] Overlay toggle state

---

### E12-003: UIRenderer Interface and Skinning System

**Size:** M (Medium)
**Blocked By:** None

**Description:**
Define the abstract UIRenderer interface and UISkin data structure for the skinning system.

**UIRenderer Interface:**
```cpp
class UIRenderer {
public:
    virtual ~UIRenderer() = default;

    // Panel rendering
    virtual void draw_panel(const Rect& bounds, const std::string& title, bool closable) = 0;
    virtual void draw_panel_background(const Rect& bounds) = 0;

    // Button rendering
    virtual void draw_button(const Rect& bounds, const std::string& text, ButtonState state) = 0;
    virtual void draw_icon_button(const Rect& bounds, TextureHandle icon, ButtonState state) = 0;

    // Text rendering
    virtual void draw_text(const std::string& text, int x, int y, FontSize size, Color color) = 0;

    // Primitives
    virtual void draw_rect(const Rect& bounds, Color fill, Color border) = 0;
    virtual void draw_progress_bar(const Rect& bounds, float progress, Color fill_color) = 0;

    // Effects (holographic mode)
    virtual void draw_scanlines(const Rect& bounds, float opacity) = 0;
    virtual void begin_glow_effect(float intensity) = 0;
    virtual void end_glow_effect() = 0;
};
```

**UISkin Structure:**
```cpp
struct UISkin {
    std::string skin_id;           // "legacy" or "holo"
    Color panel_background;
    Color panel_border;
    float panel_opacity;           // 1.0 for legacy, 0.7 for holo
    float border_glow_intensity;   // 0.0 for legacy, 1.0 for holo
    Color button_normal;
    Color button_hover;
    Color button_pressed;
    bool use_scanlines;
    bool use_hologram_flicker;
};
```

**Acceptance Criteria:**
- [ ] UIRenderer abstract interface defined
- [ ] UISkin data structure for visual properties
- [ ] Skin loading from configuration
- [ ] Runtime skin switching support

---

### E12-004: Classic Renderer Implementation

**Size:** M (Medium)
**Blocked By:** E12-003

**Description:**
Implement the Classic (Legacy Interface) renderer with opaque panels, traditional button styling, and bioluminescent accents.

**Visual Style:**
- Dark backgrounds (#0a0a12)
- Opaque panels with alien-themed borders
- Cyan/teal highlights for interactive elements
- No transparency effects
- No scan-line effects

**Acceptance Criteria:**
- [ ] ClassicRenderer implements UIRenderer
- [ ] Opaque panel rendering with borders
- [ ] Traditional button states (normal, hover, pressed, disabled)
- [ ] Bioluminescent color palette applied
- [ ] Text rendering with standard fonts

---

### E12-005: Holographic Renderer Implementation

**Size:** L (Large)
**Blocked By:** E12-003

**Description:**
Implement the Holographic (Holo Interface) renderer with translucent panels, glow effects, scan-lines, and subtle flicker.

**Visual Style:**
- Translucent panels (70% opacity)
- Glow effects on borders and active elements
- Scan-line overlay effect
- Subtle 2% opacity flicker (randomized)
- Hologram-style color shifts

**Acceptance Criteria:**
- [ ] HolographicRenderer implements UIRenderer
- [ ] Translucent panel rendering with glow borders
- [ ] Scan-line overlay effect (shader-based)
- [ ] Hologram flicker effect (2% opacity variation)
- [ ] Glow effects for active elements
- [ ] Radial gradient backgrounds

---

## Phase 2: Core UI Elements

### E12-006: Core Widget Library (Button, Label, Panel, Icon)

**Size:** M (Medium)
**Blocked By:** E12-001

**Description:**
Implement the core widget types used throughout the UI: Button, Label, Panel, and Icon widgets.

**Widget Types:**
```cpp
class ButtonWidget : public Widget {
    std::string text;
    TextureHandle icon;
    ButtonState state;
    std::function<void()> on_click;
};

class LabelWidget : public Widget {
    std::string text;
    FontSize font_size;
    TextAlignment alignment;
    Color text_color;
};

class PanelWidget : public Widget {
    std::string title;
    bool closable;
    bool draggable;  // Only for holo mode
};

class IconWidget : public Widget {
    TextureHandle texture;
    Color tint;
};
```

**Acceptance Criteria:**
- [ ] ButtonWidget with click callback
- [ ] LabelWidget with alignment options
- [ ] PanelWidget with title bar and optional close button
- [ ] IconWidget for tool icons and status indicators
- [ ] All widgets render correctly in both modes

---

### E12-007: ProgressBar and Demand Meter Widgets

**Size:** S (Small)
**Blocked By:** E12-006

**Description:**
Implement ProgressBar widget for resource levels and specialized ZonePressureBar for RCI demand display.

**Widget Types:**
```cpp
class ProgressBarWidget : public Widget {
    float value;      // 0.0 - 1.0
    float max_value;
    Color fill_color;
    bool show_label;
};

class ZonePressureWidget : public Widget {
    int8_t habitation_demand;   // -100 to +100
    int8_t exchange_demand;     // -100 to +100
    int8_t fabrication_demand;  // -100 to +100
};
```

**Acceptance Criteria:**
- [ ] ProgressBarWidget with smooth animation
- [ ] ZonePressureWidget showing R/C/I bars
- [ ] Color coding (green positive, red negative)
- [ ] Animated value changes (lerp over 200ms)

---

### E12-008: Input Routing and Hit Testing

**Size:** M (Medium)
**Blocked By:** E12-002

**Description:**
Implement input routing from InputSystem to UISystem, including hit testing to determine if input is over UI or should pass through to game world.

**Hit Test Priority:**
1. Modal dialogs (block all)
2. Floating panels (holo mode)
3. Docked panels (legacy mode)
4. Command array
5. Sector scan
6. Status bar

**Implementation:**
```cpp
class UIHitTester {
public:
    Widget* hit_test(int screen_x, int screen_y);
    bool is_over_ui(int screen_x, int screen_y);
};

// Event consumption
struct UIInputEvent {
    InputEventType type;
    int mouse_x, mouse_y;
    int key_code;
    bool consumed = false;
};
```

**Acceptance Criteria:**
- [ ] Hit testing respects widget z-order
- [ ] Modal dialogs block all input
- [ ] Events marked as consumed prevent propagation
- [ ] Mouse enter/leave events generated correctly
- [ ] Integration with InputSystem

---

## Phase 3: Command Array (Toolbar)

### E12-009: Command Array - Classic Mode Layout

**Size:** M (Medium)
**Blocked By:** E12-006

**Description:**
Implement the Classic mode command array (toolbar) with top-docked horizontal layout.

**Layout:**
```
+------------------------------------------------------------------+
| BUILD: [H][E][F] [Pathway][Energy][Fluid] [Structures v]          |
| MODIFY: [Bulldoze] [Purge] [Grade]                               |
| INSPECT: [Probe] [Scan Layers v]                                 |
| VIEW: [Pause][|>][>>][>>>] [Preset N][E][S][W]                   |
+------------------------------------------------------------------+
```

**Tool Groups:**
- BUILD: Zone designators (H/E/F), Pathway, Energy Conduit, Fluid Conduit, Structures dropdown
- MODIFY: Bulldoze, Purge terrain, Grade terrain
- INSPECT: Probe function (query tool), Scan layer selector
- VIEW: Speed controls, Camera presets

**Acceptance Criteria:**
- [ ] Horizontal toolbar at screen top
- [ ] Tool groups visually separated
- [ ] Dropdown menus for structures and scan layers
- [ ] Speed control buttons with visual state
- [ ] Keyboard shortcuts displayed on hover

---

### E12-010: Command Array - Holographic Radial Menu

**Size:** M (Medium)
**Blocked By:** E12-006

**Description:**
Implement the Holographic mode radial menu that appears on right-click.

**Radial Menu Design:**
- Inner ring: 4 categories (Build, Modify, Inspect, View)
- Hover category to expand sub-ring with specific tools
- 8 items max per ring
- ESC or click-away to close

**Acceptance Criteria:**
- [ ] Right-click opens radial at cursor position
- [ ] Inner ring with 4 main categories
- [ ] Category hover expands sub-ring
- [ ] Smooth animation for ring expansion
- [ ] Visual feedback for selection
- [ ] Close on ESC or click outside

---

### E12-011: Tool State Machine and Cursor Display

**Size:** M (Medium)
**Blocked By:** E12-008

**Description:**
Implement the tool state machine that tracks the current tool and manages cursor display.

**Tool Types:**
```cpp
enum class ToolType {
    Select,
    ZoneHabitation,
    ZoneExchange,
    ZoneFabrication,
    Pathway,
    EnergyConduit,
    FluidConduit,
    Structure,
    Bulldoze,
    Purge,
    Grade,
    Probe
};
```

**Cursor Modes:**
- Default: Arrow
- Zone tools: Zone-type colored brush indicator
- Infrastructure: Line preview
- Bulldoze: Red X cursor
- Probe: Magnifying glass / analyzer icon

**Acceptance Criteria:**
- [ ] Tool state machine with all tool types
- [ ] Custom cursor per tool type
- [ ] Placement preview (ghost) for buildables
- [ ] Valid/invalid placement indicator (green/red)
- [ ] Tool switching via keyboard shortcuts

---

## Phase 4: Information Panels

### E12-012: Data Readout Panel (Query Tool Display)

**Size:** M (Medium)
**Blocked By:** E12-006

**Description:**
Implement the data readout panel that shows information about the selected tile/structure.

**Panel Content:**
```
+---------------------------+
| DATA READOUT              |
+---------------------------+
| Structure: Hab-Cluster B7 |
| Type: High-Density Hab    |
| Status: Active            |
|                           |
| [v] Details               |
|   Sector Value: 78/100    |
|   Disorder Index: 12/100  |
|   Contamination: 3/100    |
|   Energy: Connected       |
|   Fluid: Connected        |
|   Pathway Distance: 1     |
+---------------------------+
```

**Acceptance Criteria:**
- [ ] Panel shows selected entity info
- [ ] Summary section always visible
- [ ] Expandable details section
- [ ] Status icons for energy/fluid/pathway
- [ ] Updates when selection changes
- [ ] Uses alien terminology

---

### E12-013: Probe Function (Query Tool) Implementation

**Size:** M (Medium)
**Blocked By:** E12-012

**Description:**
Implement the Probe function that queries all systems for data at a clicked tile.

**Query Aggregation:**
```cpp
struct TileQueryResult {
    GridPosition position;

    // Terrain
    TerrainType terrain_type;
    uint8_t elevation;

    // Building (if present)
    std::optional<BuildingInfo> structure;

    // Zone (if present)
    std::optional<ZoneType> zone;

    // Utilities
    bool has_energy;
    bool has_fluid;
    uint8_t pathway_distance;

    // Simulation values
    uint8_t disorder_level;
    uint8_t contamination_level;
    uint8_t sector_value;

    // Ownership
    PlayerID owner;
};
```

**Acceptance Criteria:**
- [ ] Click tile to query
- [ ] Aggregates data from all systems
- [ ] Queries TerrainSystem, BuildingSystem, EnergySystem, FluidSystem, etc.
- [ ] Populates TileQueryResult struct
- [ ] Sends result to data readout panel
- [ ] Performance: <5ms for full query

---

### E12-014: Colony Status Bar (Population, Treasury, Date)

**Size:** M (Medium)
**Blocked By:** E12-006

**Description:**
Implement the always-visible status bar showing colony population, treasury balance, and current cycle/phase.

**Status Bar Layout:**
```
+---------------------------------------------------------------+
| Pop: 12,450 beings | Treasury: 45,230 cr | Cycle 5, Phase 3   |
+---------------------------------------------------------------+
```

**Data Sources:**
- Population: IStatQueryable from PopulationSystem
- Treasury: IEconomyQueryable from EconomySystem
- Date: ISimulationTime from SimulationCore

**Acceptance Criteria:**
- [ ] Fixed position status bar (top for Legacy, floating for Holo)
- [ ] Population display with thousands separator
- [ ] Treasury balance with credit symbol
- [ ] Cycle and phase display
- [ ] Updates every tick
- [ ] Uses alien terminology

---

### E12-015: Zone Pressure Indicator (RCI Demand)

**Size:** S (Small)
**Blocked By:** E12-007

**Description:**
Implement the zone pressure (RCI demand) indicator showing current demand for each zone type.

**Display:**
```
+-------------------+
| ZONE PRESSURE     |
| H: ████░░░░ +45   |
| E: ██░░░░░░ +15   |
| F: ░░░░░░░░ -10   |
+-------------------+
```

**Acceptance Criteria:**
- [ ] Three horizontal bars for H/E/F
- [ ] Bars show demand -100 to +100
- [ ] Green for positive, red for negative
- [ ] Animated bar movement
- [ ] Numeric value displayed

---

## Phase 5: Scan Layers (Overlays)

### E12-016: Scan Layer Manager and Toggle System

**Size:** M (Medium)
**Blocked By:** E12-002

**Description:**
Implement the scan layer manager that tracks which overlay is active and handles toggle logic.

**Overlay Types:**
```cpp
enum class OverlayType {
    None,
    Disorder,        // DisorderSystem IGridOverlay
    Contamination,   // ContaminationSystem IGridOverlay
    SectorValue,     // LandValueSystem IGridOverlay
    EnergyCoverage,  // EnergySystem coverage
    FluidCoverage,   // FluidSystem coverage
    ServiceCoverage, // ServicesSystem coverage
    Traffic          // TransportSystem congestion
};
```

**Toggle Behavior:**
- TAB cycles: None -> Energy -> Fluid -> Disorder -> Contamination -> Value -> None
- Shift+1-7 for direct selection
- Only one overlay active at a time (MVP)

**Acceptance Criteria:**
- [ ] OverlayType enum with all types
- [ ] Overlay state in UIState
- [ ] Toggle cycling with TAB
- [ ] Direct selection with Shift+number
- [ ] Overlay fade transition (250ms)

---

### E12-017: Scan Layer Rendering (IGridOverlay Integration)

**Size:** L (Large)
**Blocked By:** E12-016

**Description:**
Implement the scan layer rendering system that visualizes IGridOverlay data as semi-transparent colored tiles.

**Rendering Approach:**
1. Query IGridOverlay from appropriate system
2. Generate overlay texture (256x256 or 512x512 depending on map size)
3. Render as 3D grid mesh aligned with terrain chunks
4. Apply color scheme to map values to colors
5. Additive blend with scene

**Performance Considerations:**
- Chunked rendering (32x32 tiles per chunk)
- Texture updated at tick boundaries, not every frame
- LOD for distant tiles (optional)

**Acceptance Criteria:**
- [ ] Query IGridOverlay from simulation systems
- [ ] Generate colored overlay texture
- [ ] Render aligned with terrain mesh
- [ ] Semi-transparent blending
- [ ] Chunked updates for performance
- [ ] <2ms frame time overhead

---

### E12-018: Scan Layer Color Schemes

**Size:** S (Small)
**Blocked By:** E12-017

**Description:**
Implement the color scheme mapping for each overlay type.

**Color Schemes:**
| Overlay | Scheme | Low Color | High Color |
|---------|--------|-----------|------------|
| Sector Value | heat_map | Blue | Red |
| Disorder | green_red | Green | Red |
| Contamination | purple_yellow | Purple | Yellow |
| Energy Coverage | coverage | Dark Gray | Cyan |
| Fluid Coverage | coverage | Dark Gray | Blue |
| Traffic | heat_map | Green | Red |
| Service Coverage | coverage | Dark Gray | Green |

**Acceptance Criteria:**
- [ ] Color scheme enum matching IGridOverlay
- [ ] Gradient interpolation for heat_map/green_red
- [ ] Binary/stepped display for coverage
- [ ] Legend widget showing color scale

---

## Phase 6: Sector Scan (Minimap)

### E12-019: Sector Scan Rendering (Minimap)

**Size:** L (Large)
**Blocked By:** E12-002

**Description:**
Implement the sector scan (minimap) that shows a zoomed-out view of the entire map.

**Rendering Approach:**
- CPU-generated image (Option B from analysis)
- Query terrain/zone/building data directly
- Generate 2D pixel buffer
- Upload as texture
- Update on significant changes (zone placed, building demolished)

**Color Scheme:**
| Element | Color |
|---------|-------|
| Substrate (ground) | Dark gray (#404040) |
| Deep void (water) | Dark blue (#1a4d7a) |
| Habitation zones | Green (#00aa00) |
| Exchange zones | Blue (#0066cc) |
| Fabrication zones | Yellow (#cccc00) |
| Pathways | White (#cccccc) |
| Player ownership | Border in player color |

**Acceptance Criteria:**
- [ ] Minimap panel in corner (Legacy) or floating (Holo)
- [ ] Shows terrain types with correct colors
- [ ] Shows zone designations
- [ ] Shows player ownership via colored borders
- [ ] Viewport indicator (current camera view)
- [ ] Updates incrementally on changes

---

### E12-020: Sector Scan Click Navigation

**Size:** S (Small)
**Blocked By:** E12-019

**Description:**
Implement click-to-navigate functionality on the sector scan.

**Behavior:**
- Click on minimap to move camera to that world position
- Camera smoothly pans to new location
- Works in both UI modes

**Acceptance Criteria:**
- [ ] Click minimap to navigate
- [ ] Converts minimap coords to world coords
- [ ] Smooth camera pan transition (500ms)
- [ ] Works in Legacy and Holo modes

---

## Phase 7: Budget Window

### E12-021: Colony Treasury Panel (Budget Window)

**Size:** L (Large)
**Blocked By:** E12-006

**Description:**
Implement the colony treasury panel (budget window) as a tabbed modal dialog.

**Tabs:**
1. **Revenue:** Tribute breakdown by zone type
2. **Expenditure:** Maintenance costs by category
3. **Services:** Funding sliders for each service type
4. **Credit Advances:** Active bonds, issue new bonds

**Data Sources:**
- IEconomyQueryable.get_income_breakdown()
- IEconomyQueryable.get_expense_breakdown()
- IEconomyQueryable.get_funding_level()
- IEconomyQueryable.get_bond_count()

**Acceptance Criteria:**
- [ ] Modal dialog with 4 tabs
- [ ] Revenue tab with per-zone breakdown
- [ ] Expenditure tab with per-category breakdown
- [ ] Services tab with funding sliders
- [ ] Credit Advances tab with bond management
- [ ] Queries EconomySystem for real data
- [ ] Updates in real-time while open

---

### E12-022: Tribute Rate and Funding Sliders

**Size:** M (Medium)
**Blocked By:** E12-021

**Description:**
Implement the slider widgets for tribute rates (0-20%) and service funding levels (0-150%).

**Slider Widget:**
```cpp
class SliderWidget : public Widget {
    float value;
    float min_value;
    float max_value;
    float step;
    std::function<void(float)> on_change;
};
```

**Tribute Rate Sliders:**
- Habitation tribute: 0-20%
- Exchange tribute: 0-20%
- Fabrication tribute: 0-20%

**Funding Sliders:**
- Enforcer funding: 0-150%
- Hazard Response funding: 0-150%
- Medical funding: 0-150%
- Education funding: 0-150%

**Acceptance Criteria:**
- [ ] SliderWidget with drag interaction
- [ ] Shows current value numerically
- [ ] Emits events on change (UITributeRateChangedEvent, UIFundingChangedEvent)
- [ ] Visual feedback for slider position
- [ ] Snapping to valid increments

---

## Phase 8: Polish Features

### E12-023: Alert Pulse Notification System

**Size:** M (Medium)
**Blocked By:** E12-006

**Description:**
Implement the alert pulse (notification) system for game events.

**Priority Levels:**
| Priority | Color | Duration | Audio |
|----------|-------|----------|-------|
| Critical | Red pulse | 8 seconds | Alert chime |
| Warning | Amber glow | 5 seconds | Soft tone |
| Info | Cyan flash | 3 seconds | None |

**Notification Queue:**
- Max 4 visible at once
- FIFO queue (newest pushes oldest)
- Click to dismiss
- Click to focus (for location-based alerts)

**Acceptance Criteria:**
- [ ] Three priority levels with distinct styling
- [ ] Notification panel with queue
- [ ] Auto-dismiss after duration
- [ ] Click to dismiss or focus
- [ ] Audio cues for critical/warning (optional)
- [ ] Fade-out animation

---

### E12-024: Keyboard Shortcut System

**Size:** M (Medium)
**Blocked By:** E12-008

**Description:**
Implement the keyboard shortcut system for tools, panels, and overlays.

**Default Shortcuts:**
| Key | Action |
|-----|--------|
| 1-3 | Zone types (H/E/F) |
| R | Pathway tool |
| P | Energy conduit tool |
| W | Fluid conduit tool |
| B | Bulldoze tool |
| Q | Probe function (query) |
| TAB | Cycle overlays |
| ESC | Cancel/close |
| Space | Pause/resume |
| +/- | Speed up/down |
| F1 | Toggle UI mode |

**Acceptance Criteria:**
- [ ] Shortcut mapping system
- [ ] Default shortcuts implemented
- [ ] Shortcuts work regardless of UI mode
- [ ] ESC cancels current tool or closes panel
- [ ] Space toggles pause

---

### E12-025: Terminology Lookup and Application

**Size:** S (Small)
**Blocked By:** None

**Description:**
Implement the terminology lookup system that translates human terms to alien equivalents.

**Terminology Mapping:**
```cpp
class TerminologyLookup {
public:
    // Load from terminology.yaml
    void load(const std::string& path);

    // Get alien term
    std::string get(const std::string& human_term);

    // Format with terms (e.g., "Population: {count} beings")
    std::string format(const std::string& template_str, ...);
};
```

**Key Mappings:**
- minimap -> sector_scan
- toolbar -> command_array
- info_panel -> data_readout
- overlay -> scan_layer
- residential -> habitation
- commercial -> exchange
- industrial -> fabrication

**Acceptance Criteria:**
- [ ] Load terminology from terminology.yaml
- [ ] Lookup API for all UI text
- [ ] Applied to all player-facing strings
- [ ] Format helper for dynamic text

---

## Phase 9: Testing

### E12-026: Unit Tests for Widget System

**Size:** L (Large)
**Blocked By:** E12-008

**Description:**
Comprehensive unit tests for the widget system.

**Test Categories:**

**Widget Hierarchy Tests:**
- test_add_child - Adding child to parent
- test_remove_child - Removing child from parent
- test_bounds_calculation - Local to screen space conversion
- test_visibility_inheritance - Hidden parent hides children

**Hit Testing Tests:**
- test_point_in_rect - Basic hit testing
- test_nested_widgets - Hit testing with hierarchy
- test_z_order - Correct widget selected on overlap
- test_modal_blocking - Modal dialogs block input

**State Management Tests:**
- test_tool_selection - Tool state machine
- test_panel_open_close - Panel state tracking
- test_overlay_toggle - Overlay cycling

**Acceptance Criteria:**
- [ ] All test categories implemented
- [ ] Widget hierarchy tests pass
- [ ] Hit testing tests pass
- [ ] State management tests pass
- [ ] >80% code coverage for widget system

---

### E12-027: Integration Tests with Simulation Systems

**Size:** L (Large)
**Blocked By:** E12-017, E12-021

**Description:**
End-to-end integration tests with simulation systems.

**Integration Test Scenarios:**

**Overlay Integration:**
- test_disorder_overlay - DisorderSystem IGridOverlay renders correctly
- test_contamination_overlay - ContaminationSystem IGridOverlay renders correctly
- test_sector_value_overlay - LandValueSystem IGridOverlay renders correctly

**Query Tool Integration:**
- test_query_tile - Probe function returns correct data
- test_query_building - Building info displayed correctly
- test_query_updates - Data readout updates on selection change

**Budget Integration:**
- test_treasury_display - Treasury balance from EconomySystem
- test_tribute_slider - Tribute rate changes sent to EconomySystem
- test_funding_slider - Funding changes sent to EconomySystem

**Status Bar Integration:**
- test_population_display - PopulationSystem stats displayed
- test_date_display - SimulationTime displayed correctly
- test_demand_display - DemandSystem values displayed

**Acceptance Criteria:**
- [ ] All integration scenarios tested
- [ ] Tests run against real simulation systems (or mocks)
- [ ] Overlay rendering produces expected output
- [ ] Query tool returns correct aggregate data
- [ ] Budget window displays accurate financial data

---

## Dependency Graph

```
E12-001 -> E12-002 -> E12-008 -> E12-011
    |          |          |
    |          |          +-> E12-024
    |          |
    |          +-> E12-016 -> E12-017 -> E12-018
    |          |
    |          +-> E12-019 -> E12-020
    |
    +-> E12-006 -> E12-007 -> E12-015
           |
           +-> E12-009
           +-> E12-010
           +-> E12-012 -> E12-013
           +-> E12-014
           +-> E12-021 -> E12-022
           +-> E12-023

E12-003 -> E12-004
       +-> E12-005

E12-025 (no dependencies)

E12-008 -> E12-026
E12-017, E12-021 -> E12-027
```

---

## Notes

1. **Canon Compliance:** All terminology uses alien terms per terminology.yaml (sector_scan, command_array, data_readout, etc.)
2. **No Tick Priority:** UISystem renders between simulation ticks, not during simulation execution
3. **Dual UI Modes:** Legacy Interface and Holo Interface share widget logic, differ only in rendering
4. **IGridOverlay:** Used for all overlay visualization - systems implement interface, UISystem consumes
5. **Thread Safety:** Overlay data uses double-buffering for safe read access from render thread
6. **Newspaper Deferred:** Chronicle/newspaper system deferred to Epic 17 (Polish)
