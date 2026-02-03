# UI Developer Analysis: Epic 12 - Information & UI Systems

**Author:** UI Developer Agent
**Date:** 2026-01-29
**Canon Version:** 0.12.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 12 implements the **UISystem** - the visual interface layer for ZergCity. This system owns all UI rendering, state management, toolbars, panels, dialogs, overlays, and the minimap. Critically, UISystem does NOT own input handling - it receives events from InputSystem and provides UI events to other systems.

Key architectural characteristics:
1. **Two UI modes:** Classic (SimCity 2000-style) and Holographic (sci-fi floating panels) - sharing underlying logic
2. **Skinning system:** Both modes use identical data/keybindings with different visual presentation
3. **2D overlay on 3D:** UI renders as 2D layer composited on top of SDL_GPU 3D scene
4. **Query-based data:** UISystem queries other systems via interfaces (IStatQueryable, IGridOverlay, etc.)
5. **No input ownership:** InputSystem provides events; UISystem consumes and routes them

The UISystem is primarily a **presentation layer** - it displays data from simulation systems but does not modify game state directly. User actions flow through InputSystem to the appropriate game systems.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Architecture Design](#2-architecture-design)
3. [Technical Work Items](#3-technical-work-items)
4. [Rendering Approach](#4-rendering-approach)
5. [Input Handling](#5-input-handling)
6. [Questions for Other Agents](#6-questions-for-other-agents)
7. [Performance Considerations](#7-performance-considerations)
8. [Testing Strategy](#8-testing-strategy)

---

## 1. System Boundaries

### 1.1 UISystem (No tick priority - Render-phase only)

**Owns:**
- All UI rendering (toolbars, panels, dialogs, HUD)
- UI state management (which panels open, tool selected, etc.)
- Toolbar layout and interaction
- Info panels (selected entity details, statistics)
- Data overlays (disorder, contamination, energy, fluid coverage)
- Minimap rendering
- Query tool display
- Budget window
- Statistics and graphs windows
- News/notification display

**Does NOT Own:**
- Input handling (InputSystem provides events)
- Game state (queries from other systems)
- Tool execution (sends commands to game systems)
- 3D scene rendering (RenderingSystem owns)
- Camera control (CameraSystem owns)

**Provides:**
- UI events (button clicked, tool selected, panel opened)
- Current tool state (for cursor display)
- UI hit testing (is mouse over UI?)

**Depends On:**
- InputSystem (for UI interaction events)
- RenderingSystem (for 3D-to-screen coordinate conversion)
- All simulation systems (for display data via interfaces)
- AssetManager (for UI textures, fonts)

### 1.2 Canonical Interfaces Used

| Interface | Purpose | Source Epic |
|-----------|---------|-------------|
| IStatQueryable | Query statistics for display (population, economy, etc.) | Epic 10 |
| IGridOverlay | Get overlay data for visualization (disorder, contamination, land value) | Epic 10 |
| IEconomyQueryable | Budget data, treasury balance, income/expense breakdown | Epic 11 |
| IServiceQueryable | Service coverage for overlay display | Epic 9 |
| ITerrainQueryable | Terrain info for query tool | Epic 3 |
| IEnergyProvider | Power coverage overlay | Epic 5 |
| IFluidProvider | Fluid coverage overlay | Epic 6 |
| ITransportProvider | Traffic/pathway overlay | Epic 7 |
| ISimulationTime | Current cycle/phase for display | Epic 10 |

### 1.3 Boundary Summary Table

| Concern | UISystem | InputSystem | RenderingSystem | Game Systems |
|---------|----------|-------------|-----------------|--------------|
| UI rendering | **Owns** | - | - | - |
| UI state | **Owns** | - | - | - |
| Mouse/keyboard input | - | **Owns** | - | - |
| UI event routing | Receives | **Provides** | - | - |
| 3D scene rendering | - | - | **Owns** | - |
| Simulation data | Queries | - | - | **Owns** |
| Tool execution | Requests | Routes | - | **Handles** |

---

## 2. Architecture Design

### 2.1 Immediate Mode vs Retained Mode Decision

**Recommendation: Retained Mode with Immediate-Mode Patterns**

| Approach | Pros | Cons |
|----------|------|------|
| Immediate Mode (Dear ImGui style) | Simple, stateless, easy iteration | Harder to style, less control over rendering |
| Retained Mode (Qt/WxWidgets style) | Full styling control, complex layouts | More state management, heavier |
| **Hybrid** | Balance of flexibility and control | Moderate complexity |

**Proposed Approach:**
- **Retained mode for structure:** Widget tree built once, updated when needed
- **Immediate mode for data binding:** Data pulled fresh each frame via interfaces
- **Custom rendering:** Not using Dear ImGui - custom SDL_GPU 2D rendering for full style control

This approach allows the two visual modes (Classic/Holographic) to share widget logic while having completely different rendering paths.

### 2.2 Widget Architecture

```
+------------------+
|   UIManager      |  - Owns UI state, widget tree, mode switching
+------------------+
         |
         v
+------------------+
|   UIRenderer     |  - Abstract renderer interface
+------------------+
    /          \
   v            v
+---------+  +------------+
| Classic |  | Holographic |
| Renderer|  | Renderer    |
+---------+  +------------+
```

**Widget Hierarchy:**
```
UIRoot
  +-- Toolbar (command_array)
  |     +-- ZoneToolGroup
  |     +-- InfrastructureToolGroup
  |     +-- ServiceToolGroup
  |     +-- LandscapeToolGroup
  |     +-- QueryTool
  +-- InfoPanel (data_readout)
  |     +-- EntityDetails
  |     +-- TileInfo
  +-- Minimap (sector_scan)
  +-- StatusBar
  |     +-- PopulationDisplay
  |     +-- TreasuryDisplay
  |     +-- DateDisplay
  +-- OverlaySelector
  +-- NotificationArea
  +-- DialogStack (modal dialogs)
```

### 2.3 Skinning System

Both UI modes share:
- Same keybindings
- Same widget hierarchy
- Same data sources
- Same interaction logic

Modes differ in:
- Visual rendering (colors, transparency, effects)
- Layout positioning (docked vs floating)
- Animation/transition effects
- Decorative elements

**Skin Definition Structure:**
```cpp
struct UISkin {
    std::string skin_id;           // "classic" or "holographic"

    // Panel styling
    Color panel_background;
    Color panel_border;
    float panel_opacity;           // 1.0 for classic, 0.7 for holographic
    float border_glow_intensity;   // 0.0 for classic, 1.0 for holographic

    // Button styling
    Color button_normal;
    Color button_hover;
    Color button_pressed;
    Color button_disabled;

    // Typography
    std::string font_path;
    uint32_t font_size_header;
    uint32_t font_size_body;
    uint32_t font_size_small;

    // Effects
    bool use_scanlines;            // false for classic, true for holographic
    bool use_hologram_flicker;     // false for classic, true for holographic
    bool use_rounded_corners;      // true for both
    float corner_radius;           // 4px classic, 8px holographic
};
```

### 2.4 UI Modes: Classic vs Holographic

**Classic Mode (legacy_interface):**
- Opaque panels with alien-themed borders
- Toolbar docked at top
- Info panel docked at bottom
- Minimap in corner (fixed position)
- Traditional grid-based button layout
- Bioluminescent accent colors on dark backgrounds

**Holographic Mode (holo_interface):**
- Translucent floating panels (70% opacity)
- Radial/contextual menus
- Heads-up display overlays
- Minimap as holographic projection
- Hologram-style readouts
- Scan-line and subtle flicker effects
- Panels can be repositioned

---

## 3. Technical Work Items

### Phase 1: UI Framework Foundation (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-001 | UIManager class skeleton | M | Core UI state management, widget tree root, mode switching |
| E12-002 | Widget base class | M | Base class for all UI elements: position, size, visibility, children, hit testing |
| E12-003 | UIRenderer interface | S | Abstract rendering interface for skin system |
| E12-004 | Classic renderer implementation | M | Opaque panel rendering, traditional styling |
| E12-005 | Holographic renderer implementation | L | Translucent panels, glow effects, scan-lines, flicker |
| E12-006 | UISkin data structure and loading | S | Skin definitions for both modes, hot-reloadable |

### Phase 2: Core UI Elements (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-007 | Button widget | M | Clickable button with states (normal, hover, pressed, disabled) |
| E12-008 | Panel widget | M | Container with background, border, title bar, close button |
| E12-009 | Label widget | S | Text display with alignment and wrapping |
| E12-010 | Icon widget | S | Image display for tool icons, status indicators |
| E12-011 | ProgressBar widget | S | Horizontal bar for RCI demand, resource levels |

### Phase 3: Toolbar & Tool System (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-012 | Toolbar panel implementation | M | Top toolbar (command_array) with tool groups |
| E12-013 | ToolGroup container | M | Expandable groups for zone, infrastructure, service, landscape tools |
| E12-014 | Tool selection state machine | M | Active tool tracking, cursor display, tool switching |
| E12-015 | Tool-specific cursors | S | Custom cursors for each tool type (zone, bulldoze, query, etc.) |

### Phase 4: Info Panel & Query Tool (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-016 | Info panel implementation | M | Bottom panel (data_readout) for selected entity details |
| E12-017 | Query tool implementation | M | Click-to-query any tile, display building/terrain/value info |
| E12-018 | Entity detail display | M | Building name, type, status, power/water state, disorder, value |
| E12-019 | Tile info display | S | Terrain type, elevation, zone type, overlay values |

### Phase 5: Overlays (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-020 | Overlay rendering system | L | 2D semi-transparent grid overlay on 3D scene (via IGridOverlay) |
| E12-021 | Overlay color schemes | M | Heat map, green-red, purple-yellow gradients per overlay type |
| E12-022 | Overlay selector UI | S | Toggle buttons for disorder, contamination, land value, power, water, traffic |
| E12-023 | Coverage overlay implementation | M | Power/water/service coverage visualization |

### Phase 6: Minimap (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-024 | Minimap rendering | L | Simplified top-down view of entire map (sector_scan) |
| E12-025 | Minimap click navigation | S | Click minimap to move camera |
| E12-026 | Minimap viewport indicator | S | Show current camera view area on minimap |

### Phase 7: Statistics & Budget Windows (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-027 | Budget window | M | Treasury, income/expense breakdown, tribute rates, funding sliders |
| E12-028 | Statistics window | M | Population, demand bars, key metrics display |
| E12-029 | Graph widget | L | Line graphs for historical data (population, disorder, etc.) |

### Phase 8: Notifications & Multiplayer (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-030 | Notification system | M | Toast notifications for events (milestones, disasters, etc.) |
| E12-031 | Player list panel (multiplayer) | S | Show connected players, colors, activity indicators |
| E12-032 | Chat window (multiplayer) | M | In-game text chat display and input |

### Phase 9: Input Integration (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-033 | UI hit testing | M | Determine if mouse is over UI (block world interaction) |
| E12-034 | UI event consumption | S | Mark events as consumed to prevent propagation to game |
| E12-035 | Keyboard shortcuts | M | Hotkeys for tools, panels, overlays, mode toggle |

### Phase 10: Testing (2 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E12-036 | UI unit tests | L | Test widget hierarchy, hit testing, state management |
| E12-037 | Visual regression tests | L | Screenshot comparison for both UI modes |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 8 | E12-003, E12-006, E12-009, E12-010, E12-011, E12-015, E12-019, E12-022, E12-025, E12-026, E12-031, E12-034 |
| M | 17 | E12-001, E12-002, E12-004, E12-007, E12-008, E12-012, E12-013, E12-014, E12-016, E12-017, E12-018, E12-021, E12-023, E12-027, E12-028, E12-030, E12-032, E12-033, E12-035 |
| L | 5 | E12-005, E12-020, E12-024, E12-029, E12-036, E12-037 |
| **Total** | **30** | |

---

## 4. Rendering Approach

### 4.1 2D UI on 3D Scene Compositing

UISystem renders as a 2D overlay composited on top of the 3D scene rendered by RenderingSystem.

**Render Order:**
```
1. RenderingSystem: 3D scene (terrain, buildings, entities)
2. RenderingSystem: 3D overlays (selection highlight, placement preview)
3. UISystem: 2D data overlays (disorder heat map, etc.)
4. UISystem: 2D UI elements (panels, toolbar, minimap)
5. UISystem: Cursor
```

**Compositing Strategy:**
```
+------------------------------------------+
|          SDL_GPU Render Target           |
|  +------------------------------------+  |
|  |      3D Scene (RenderingSystem)    |  |
|  +------------------------------------+  |
|                    |                     |
|                    v                     |
|  +------------------------------------+  |
|  |    Data Overlay (UISystem)         |  |
|  |    (semi-transparent grid shader)  |  |
|  +------------------------------------+  |
|                    |                     |
|                    v                     |
|  +------------------------------------+  |
|  |    UI Panels (UISystem)            |  |
|  |    (2D quad rendering)             |  |
|  +------------------------------------+  |
+------------------------------------------+
```

### 4.2 Overlay Rendering Technical Approach

Data overlays (disorder, contamination, land value, coverage) need to render as semi-transparent colored grids aligned with the 3D terrain.

**Option A: 3D Grid Mesh (Recommended)**
- Generate a flat grid mesh at terrain height
- Apply overlay texture with per-tile colors
- Blend additively with scene
- Pros: Follows terrain elevation, correct perspective
- Cons: Need to regenerate mesh when camera changes significantly

**Option B: Screen-Space Post-Process**
- Render terrain depth to buffer
- In post-process, project overlay grid based on depth
- Pros: No mesh generation
- Cons: Complex shader, potential artifacts at terrain edges

**Recommendation:** Option A - 3D grid mesh aligned with terrain chunks. RenderingSystem already has terrain chunk system; overlay can reuse chunk coordinates.

### 4.3 Minimap Rendering

**Option A: Render-to-Texture**
- Render simplified scene from top-down camera to small texture (256x256)
- Display texture in minimap panel
- Update periodically (every N frames or on change)
- Pros: Accurate representation
- Cons: Extra render passes, GPU cost

**Option B: CPU-Generated Image**
- Query terrain/zone/building data directly
- Generate 2D pixel buffer on CPU
- Upload as texture
- Pros: No extra 3D rendering
- Cons: Manual color mapping, less visual fidelity

**Recommendation:** Option B for initial implementation - simpler, less GPU overhead. Option A can be added later for polish.

**Minimap Color Scheme:**
| Element | Color |
|---------|-------|
| Substrate (ground) | Dark gray (#404040) |
| Water | Dark blue (#1a4d7a) |
| Habitation zones | Green (#00aa00) |
| Exchange zones | Blue (#0066cc) |
| Fabrication zones | Yellow (#cccc00) |
| Pathways | White (#cccccc) |
| Player ownership | Border in player color |

### 4.4 Font Rendering

Use SDL_ttf or stb_truetype for font rendering to textures.

**Font Requirements:**
- Header font: 24px for panel titles
- Body font: 16px for general text
- Small font: 12px for tooltips, fine print
- Support for alien glyph decorations (optional Unicode symbols)

**Glyph Caching:**
- Pre-render common characters to texture atlas
- Dynamic rendering for uncommon glyphs
- Separate atlas per font size

---

## 5. Input Handling

### 5.1 Input Flow

```
+-------------+     +------------+     +-------------+
| SDL Events  | --> | InputSystem| --> | UISystem    |
| (Raw)       |     | (Processed)|     | (UI Events) |
+-------------+     +------------+     +------+------+
                                              |
                         UI consumes? --------+
                              |               |
                         Yes  |          No   |
                              v               v
                    +------------+     +--------------+
                    | UI Action  |     | Game Systems |
                    | (Internal) |     | (World Cmds) |
                    +------------+     +--------------+
```

### 5.2 UI Hit Testing

UISystem maintains a spatial structure for hit testing:

```cpp
class UIHitTester {
public:
    // Returns the topmost widget under the point
    Widget* hit_test(int screen_x, int screen_y);

    // Returns true if point is over any UI element
    bool is_over_ui(int screen_x, int screen_y);

    // Register widget for hit testing
    void register_widget(Widget* widget, const Rect& bounds);

    // Unregister widget
    void unregister_widget(Widget* widget);
};
```

**Hit Test Priority:**
1. Modal dialogs (block all)
2. Floating panels (holographic mode)
3. Docked panels (classic mode)
4. Toolbar
5. Minimap
6. HUD elements

### 5.3 Event Consumption

When UISystem handles an input event, it marks the event as consumed to prevent it from propagating to game systems:

```cpp
struct UIInputEvent {
    InputEventType type;
    int mouse_x, mouse_y;
    int key_code;
    bool consumed = false;
};

// In UISystem::handle_input()
if (is_over_ui(event.mouse_x, event.mouse_y)) {
    // Process UI interaction
    handle_ui_click(event);
    event.consumed = true;
}
```

### 5.4 Keyboard Shortcuts

| Key | Action | Notes |
|-----|--------|-------|
| 1-9 | Select tool | Map to toolbar slots |
| Q | Query tool | Quick access |
| B | Bulldoze tool | Quick access |
| P | Pause/unpause | Simulation control |
| Escape | Cancel/close | Deselect tool, close panel |
| Tab | Toggle UI mode | Classic <-> Holographic |
| M | Toggle minimap | Show/hide |
| O | Cycle overlays | Next overlay type |
| F1-F5 | Quick overlays | Power, water, traffic, disorder, value |

---

## 6. Questions for Other Agents

### @systems-architect

1. **UI and Simulation Thread Separation:** Is UISystem expected to run on a separate thread from simulation? If so, how should we handle data queries (snapshot vs live access)?

2. **Overlay Data Access Pattern:** IGridOverlay provides raw pointer to grid data. Is this safe to access from render thread while simulation thread may be writing? Need thread-safety clarification.

3. **Event System Integration:** How should UISystem emit events (tool selected, button clicked)? Direct callbacks, event queue, or observer pattern?

4. **World Coordinate Conversion:** UISystem needs screen-to-world conversion for tool placement. RenderingSystem owns this. Should UISystem query RenderingSystem directly, or should there be a shared CoordinateConverter service?

### @game-designer

5. **Overlay Toggle Persistence:** Should enabled overlays persist across sessions (saved in settings), or reset to "none" on game load?

6. **Classic Mode Layout Specifics:** Can you provide a mockup or detailed description of the Classic mode toolbar layout? Number of tool groups, icons per group, arrangement?

7. **Holographic Mode Radial Menus:** Where should radial menus appear? On right-click at cursor position? On designated hotkey? How many items per radial?

8. **Notification Priority and Duration:**
   - How long should notifications stay visible? (5 seconds? Until dismissed?)
   - Priority levels for notifications? (Critical: disasters, Important: milestones, Info: general)
   - Maximum simultaneous notifications?

9. **News/Newspaper System:** Is the newspaper system (random articles, opinion polls) in scope for Epic 12, or deferred to Polish (Epic 17)?

### @graphics-engineer

10. **Overlay Shader Requirements:** For data overlay rendering, what shader capabilities are available? Specifically:
    - Additive blending support
    - Per-instance vertex coloring
    - Texture array support for gradient LUTs

11. **2D Rendering Integration:** How should UISystem submit 2D render commands? Direct SDL_GPU calls, or through a RenderingSystem 2D API?

12. **Depth Buffer Access:** For overlays that follow terrain, does UISystem have access to depth buffer, or should overlays be rendered by RenderingSystem on request?

---

## 7. Performance Considerations

### 7.1 Overlay Rendering Performance

**Concern:** Data overlays are full-map grid renders (up to 512x512 = 262,144 tiles).

**Mitigations:**
- **Chunked rendering:** Only render visible chunks (reuse terrain chunk system)
- **LOD for distant tiles:** Reduce overlay resolution at distance
- **Update throttling:** Overlay data from IGridOverlay changes slowly; cache and update every N frames
- **GPU instancing:** Use instanced rendering for grid quads

**Target:** Overlay rendering should add < 2ms to frame time.

### 7.2 Minimap Update Frequency

**Concern:** Regenerating minimap texture every frame is expensive.

**Mitigations:**
- **Dirty tracking:** Only update minimap when relevant data changes (zones, buildings, ownership)
- **Incremental updates:** Update only changed regions
- **Async generation:** Generate minimap on separate thread, display previous until ready

**Target:** Minimap update should not cause frame hitches; prefer slightly stale data over stuttering.

### 7.3 Font Rendering

**Concern:** Text rendering can be expensive if done per-frame without caching.

**Mitigations:**
- **Glyph atlas:** Pre-render all needed glyphs to texture atlas at startup
- **Text caching:** Cache rendered text strings that don't change frequently
- **Batching:** Batch all text rendering into single draw call per font size

### 7.4 Widget Tree Updates

**Concern:** Deep widget trees with many elements can be slow to traverse.

**Mitigations:**
- **Dirty flags:** Only re-layout widgets that changed
- **Visibility culling:** Skip hidden widgets entirely
- **Flat hit-test structure:** Maintain separate flat list for hit testing, not tree traversal

---

## 8. Testing Strategy

### 8.1 Unit Tests

| Test Category | Coverage |
|---------------|----------|
| Widget hierarchy | Parent-child relationships, add/remove |
| Hit testing | Point-in-rect, overlapping widgets, z-order |
| State management | Tool selection, panel open/close, mode toggle |
| Layout calculation | Widget positioning, sizing, constraints |
| Data binding | IStatQueryable queries, value display |

### 8.2 Integration Tests

| Test Category | Coverage |
|---------------|----------|
| Input routing | UI events consumed correctly, world events pass through |
| Overlay data flow | IGridOverlay -> overlay render -> correct colors |
| Budget window | IEconomyQueryable -> budget display accuracy |
| Query tool | Click tile -> correct entity/terrain info displayed |
| Multiplayer | Player list updates, chat message display |

### 8.3 Visual Regression Tests

- Screenshot comparison for both Classic and Holographic modes
- Test each panel/dialog in isolation
- Test overlay rendering at different zoom levels
- Test font rendering at different sizes

### 8.4 Performance Tests

- Measure frame time with all overlays enabled
- Measure minimap update time for different map sizes
- Stress test with many notifications
- Profile widget tree traversal with complex UIs

---

## Appendix A: Widget Base Class

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
    virtual void on_key_down(int key);
    virtual void on_key_up(int key);

protected:
    bool hovered = false;
    bool pressed = false;
};
```

---

## Appendix B: UIRenderer Interface

```cpp
class UIRenderer {
public:
    virtual ~UIRenderer() = default;

    // Panel rendering
    virtual void draw_panel(const Rect& bounds, const std::string& title, bool closable) = 0;
    virtual void draw_panel_background(const Rect& bounds) = 0;
    virtual void draw_panel_border(const Rect& bounds) = 0;

    // Button rendering
    virtual void draw_button(const Rect& bounds, const std::string& text, ButtonState state) = 0;
    virtual void draw_icon_button(const Rect& bounds, TextureHandle icon, ButtonState state) = 0;

    // Text rendering
    virtual void draw_text(const std::string& text, int x, int y, FontSize size, Color color) = 0;
    virtual void draw_text_centered(const std::string& text, const Rect& bounds, FontSize size, Color color) = 0;

    // Primitives
    virtual void draw_rect(const Rect& bounds, Color fill, Color border) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2, Color color) = 0;
    virtual void draw_texture(TextureHandle texture, const Rect& dest) = 0;

    // Progress bars
    virtual void draw_progress_bar(const Rect& bounds, float progress, Color fill_color) = 0;
    virtual void draw_demand_bar(const Rect& bounds, int value, int min_val, int max_val) = 0;

    // Special effects (holographic mode)
    virtual void begin_glow_effect(float intensity) = 0;
    virtual void end_glow_effect() = 0;
    virtual void draw_scanlines(const Rect& bounds, float opacity) = 0;

    // State
    virtual void set_clip_rect(const Rect& bounds) = 0;
    virtual void clear_clip_rect() = 0;
    virtual void set_opacity(float opacity) = 0;
};
```

---

## Appendix C: Overlay Color Schemes

Per canon (interfaces.yaml), overlays use specific color schemes:

| Overlay | Color Scheme | Low Value Color | High Value Color |
|---------|--------------|-----------------|------------------|
| Land Value | heat_map | Blue (#0000ff) | Red (#ff0000) |
| Disorder | green_red | Green (#00ff00) | Red (#ff0000) |
| Contamination | purple_yellow | Purple (#800080) | Yellow (#ffff00) |
| Energy Coverage | coverage | Dark (#333333) | Cyan (#00ffff) |
| Fluid Coverage | coverage | Dark (#333333) | Blue (#0066ff) |
| Traffic | heat_map | Green (#00ff00) | Red (#ff0000) |
| Service Coverage | coverage | Dark (#333333) | Green (#00ff00) |

**Gradient Implementation:**
```cpp
Color get_overlay_color(OverlayColorScheme scheme, uint8_t value) {
    float t = value / 255.0f;

    switch (scheme) {
        case HEAT_MAP:
            // Blue -> Cyan -> Green -> Yellow -> Red
            return interpolate_heatmap(t);
        case GREEN_RED:
            // Green -> Yellow -> Red
            return Color::lerp(Color::GREEN, Color::RED, t);
        case PURPLE_YELLOW:
            // Purple -> Blue -> Cyan -> Green -> Yellow
            return interpolate_purple_yellow(t);
        case COVERAGE:
            // Dark -> Accent color at 50% opacity
            Color accent = get_coverage_accent_color();
            return Color::lerp(Color::DARK_GRAY, accent, t);
    }
}
```

---

**End of UI Developer Analysis: Epic 12 - Information & UI Systems**
