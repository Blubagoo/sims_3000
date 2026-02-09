# Epic 12: Information & UI Systems - Systems Architect Review

## Summary

Epic 12 delivers a well-structured UI framework spanning 25 headers, 25 source files, and 106 tests (64 unit + 42 integration). The architecture cleanly separates concerns across widget hierarchy, renderer abstraction, state management, and simulation data binding. The code compiles cleanly, is thoroughly documented with Doxygen comments, and follows consistent naming conventions throughout. The system is designed with dual-mode rendering (Legacy/Holo) at its core, simulation data flows through well-defined interfaces (IGridOverlay, IProbeQueryProvider, IMinimapDataProvider), and the overall quality is high.

**Overall assessment: Solid implementation with a small number of architectural issues that should be addressed before integration with the render pipeline.**

## Architecture

### Widget Hierarchy

The widget tree design (`Widget.h`) is clean and well-considered:

- **Ownership model:** `std::unique_ptr<Widget>` children with non-owning `Widget* parent` back-pointer. This is a correct and conventional pattern for UI trees.
- **Screen bounds computation:** Recursive `compute_screen_bounds()` propagates parent offset through the hierarchy. Called once per frame in `UIManager::update()` before any hit testing or rendering.
- **Hit testing:** `find_child_at()` iterates children in reverse order for correct z-order behavior. Skips invisible and disabled children appropriately.
- **Event dispatch:** Virtual no-op methods (`on_mouse_enter`, `on_mouse_leave`, `on_mouse_down`, `on_mouse_up`, `on_mouse_move`) allow leaf widgets to opt-in to events they care about.

The hierarchy depth is reasonable: Widget -> PanelWidget -> BudgetWindow/CommandArray/DataReadoutPanel, with ButtonWidget and LabelWidget as leaf nodes.

### Renderer Abstraction

The `UIRenderer` interface (`UIRenderer.h`) defines a complete pure-virtual drawing API for panels, buttons, text, rects, progress bars, sliders, and icons. Holographic effects (`draw_scanlines`, `begin_glow_effect`, `end_glow_effect`) are virtual with default no-op implementations, so the ClassicRenderer does not need to override them.

Both `ClassicRenderer` and `HolographicRenderer` are currently stubs that count draw calls for testing, which is the correct approach given that SDL_GPU integration happens later. The `DrawStats` diagnostic struct is a thoughtful addition for verifying rendering behavior without a GPU.

### Manager Pattern

`UIManager` serves as the central hub: owns the root widget, tracks UIState (tool, overlay, alerts, panels), and handles mode switching. The state is centralized in `UIState` which enables easy serialization and debugging.

Separate manager classes (`ScanLayerManager`, `ToolStateMachine`, `KeyboardShortcuts`) handle their respective domains without excessive coupling to UIManager. This is clean decomposition.

### Coupling Analysis

The coupling is well-managed with a few exceptions noted in Issues. The core dependency flow is:

```
Widget (base) <- UIRenderer (interface) <- UISkin (data)
     ^                                        ^
     |                                        |
UIManager -----> (all of the above)
     ^
     |
CommandArray, RadialMenu, AlertPulseSystem, etc.
```

Simulation systems are decoupled via interfaces: `IGridOverlay`, `IProbeQueryProvider`, `IMinimapDataProvider`. This is correct -- UI components never directly reference simulation internals.

## API Design

### Interface Quality

Interfaces are consistently designed:
- Non-owning observer pointers for data providers (`IGridOverlay*`, `IMinimapDataProvider*`, `IProbeQueryProvider*`)
- Callback-based communication using `std::function<>` for tool changes, slider adjustments, speed control, and navigation
- Factory functions for common configurations (`create_tribute_slider`, `create_funding_slider`)
- Event structs (`UITributeRateChangedEvent`, `UIFundingChangedEvent`) for game system notification

### Consistency

- All widget constructors use member initializer lists or in-class defaults
- Color constants use `static constexpr` methods (e.g., `HABITATION_COLOR()`) consistently throughout
- Layout constants are `static constexpr float` values in the class definition
- Screen-space coordinates are consistently `float` throughout (no int/float mixing in APIs)
- All headers have include guards and Doxygen documentation

### Extensibility

- New widget types can be added by deriving from `Widget` and overriding `render()` and event methods
- New overlay types can be added to the `OverlayType` enum with a corresponding `IGridOverlay` implementation
- New tools can be added to `ToolType` with cursor mode and display name mappings in `ToolStateMachine`
- The `KeyboardShortcuts` system supports runtime rebinding via `set_binding()`
- `TerminologyLookup` can be extended with additional mappings without code changes (YAML loading is stubbed)

## Performance

### CPU Pixel Buffer Approach

Both `ScanLayerRenderer` and `SectorScan` use CPU-side RGBA8 pixel buffers. This is a reasonable approach for a city builder where overlay data changes at tick rate (20 Hz), not frame rate.

- **ScanLayerRenderer:** Full 256x256 update iterates 65,536 tiles. At ~1 byte read + 4 byte write per tile, this is under 400 KB of memory traffic per tick -- well within the <2ms frame time budget on modern CPUs.
- **SectorScan:** Same approach for the minimap. Regeneration is triggered only when the minimap is invalidated (zone placed, building demolished), not every frame.
- **update_region():** Chunked partial updates are supported for when only a portion of the overlay changes. This is a good optimization for future use.

### Update Frequency

- `UIManager::update()` calls `compute_screen_bounds()` every frame. For the expected widget count (under 100 active widgets), this is negligible.
- Alert timers tick every frame -- a simple float subtraction loop over at most 4 active alerts.
- `ScanLayerManager::update()` handles fade transitions with a single float comparison/addition per frame.

### Memory Usage

- The `OverlayTextureData` pixel buffer for a 256x256 map is 256 KB. For 512x512, it is 1 MB. Both are reasonable.
- `SectorScan` pixel buffer is proportional to map size: up to 1 MB for 512x512.
- `BudgetData` uses `std::vector<>` for line items which is heap-allocated, but budget data is small and updated infrequently.
- `TerminologyLookup` reserves 200 buckets for its hash map. With ~120 actual mappings, this is efficient.

## Thread Safety

All headers explicitly document **"not thread-safe. Call from the main/render thread only."** This is the correct approach for a UI system.

Specific observations:

- **UIManager, UIInputRouter, ToolStateMachine, KeyboardShortcuts, ScanLayerManager, AlertPulseSystem:** All documented as main-thread-only. No shared mutable state.
- **UIInputRouter** stores raw Widget pointers (`m_hovered`, `m_focused`, `m_last_hovered`) across frames. These are safe as long as widget tree mutations (add/remove child) do not happen during input processing -- which is guaranteed by single-threaded design.
- **ScanLayerRenderer:** Writes to the pixel buffer from the main thread, marks dirty, then the render integration layer reads it. The tickets mention double-buffering for safe render-thread access (ticket notes), but the current implementation uses a single buffer with a `dirty` flag. This will need attention during render pipeline integration if the render thread reads the buffer while the main thread writes.
- **ScanLayerColorScheme:** All methods are const or static -- explicitly documented as thread-safe. Good.
- **TerminologyLookup::instance():** Uses C++11 magic statics for thread-safe singleton initialization. Correct.

## Dependency Management

### Include Chains

The include graph is a DAG (directed acyclic graph) with no circular dependencies:

```
Widget.h (root, no UI deps)
  <- UIRenderer.h (forward-declares Rect, Color)
  <- UISkin.h (includes Widget.h for Color)
  <- UIManager.h (includes Widget, UISkin, UIRenderer)
  <- CoreWidgets.h (includes Widget, UIRenderer)
  <- ProgressWidgets.h (includes Widget, UIRenderer)
  <- SliderWidget.h (includes Widget, UIRenderer)
  <- ColonyStatusBar.h (includes Widget, UIRenderer)
  <- SectorScan.h (includes Widget, UIRenderer)
  <- AlertPulseSystem.h (includes Widget, UIRenderer, UIManager)
  <- CommandArray.h (includes CoreWidgets, UIManager)
  <- RadialMenu.h (includes Widget, UIRenderer, UIManager)
  <- ToolStateMachine.h (includes UIManager, Widget)
  <- ScanLayerManager.h (includes UIManager)
  <- ScanLayerRenderer.h (includes ScanLayerManager)
  <- ScanLayerColorScheme.h (includes UIManager, Widget)
  <- DataReadoutPanel.h (includes CoreWidgets)
  <- ProbeFunction.h (includes DataReadoutPanel)
  <- BudgetWindow.h (includes CoreWidgets)
  <- ZonePressurePanel.h (includes CoreWidgets, ProgressWidgets)
  <- KeyboardShortcuts.h (no UI deps - standalone)
  <- TerminologyLookup.h (no deps - standalone)
  <- SectorScanNavigator.h (no deps - standalone)
  <- UIInputRouter.h (includes Widget)
  <- ClassicRenderer.h (includes UIRenderer, UISkin, Widget)
  <- HolographicRenderer.h (includes UIRenderer, UISkin, Widget)
```

No circular dependencies exist. Good.

### Cross-layer Dependencies

- UI -> Core: Only `sims3000/core/types.h` (for `EntityID`, `GridPosition`, `PlayerID`). Minimal and correct.
- UI -> Services: Only `sims3000/services/IGridOverlay.h` via forward declarations in headers and includes in .cpp files. Clean.
- UI -> Input: Only `sims3000/input/InputSystem.h` in `UIInputRouter.cpp`. Properly forward-declared in the header.

### Coupling Concerns

Several headers include `UIManager.h` when they only need the `OverlayType` or `ToolType` enums. These enums are defined inside `UIManager.h`, creating a heavier-than-necessary include dependency. See Issue #1.

## Test Coverage

### Unit Tests (64 tests in `test_ui_widgets.cpp`)

Coverage is thorough across four categories:

1. **Widget Hierarchy (8 tests):** add_child, remove_child, null safety, bounds calculation (nested, deeply nested), visibility inheritance.
2. **Hit Testing (10 tests):** Rect::contains, widget hit_test (visible/invisible/disabled), find_child_at (deepest, z-order, miss, invisible skip, disabled skip).
3. **State Management (24 tests):** UIManager tool selection/state, overlay cycling (full cycle), direct overlay set, budget panel toggle, mode switching, alert push/max visible, root not null. ScanLayerManager cycling (next/previous), set_active, fade progress, on_change callback. ToolStateMachine cursor modes (all 7), cancel, on_change callback, same-tool no-op, is_placement_tool, is_zone_tool, placement validity.
4. **Widget-specific (22 tests):** ButtonWidget click/hover/leave/right-click. ProgressBarWidget value clamping, immediate set, smooth animation. LabelWidget text/alignment/font/color. PanelWidget title/closable/content_bounds/draggable/on_close. IconWidget defaults. ZonePressureWidget demand values. Color::from_rgba8. Widget default state.

### Integration Tests (42 tests in `test_ui_integration.cpp`)

Coverage spans all major integration surfaces:

1. **Overlay Integration (9 tests):** Register/retrieve, None returns null, pixel generation, fade alpha scaling, clear, color scheme mapping for disorder/contamination/sector value, scheme type mapping.
2. **Query Tool Integration (6 tests):** Provider registration, query population, multiple providers, unregister, DataReadoutPanel accepts/clears result.
3. **Budget Integration (8 tests):** BudgetWindow data acceptance, tab switching, callbacks fire, slider value clamping, step snapping, on_change callback, tribute/funding slider factories.
4. **Status Bar Integration (5 tests):** Data storage/retrieval, zero/large population, negative treasury, paused state.
5. **Minimap Integration (7 tests):** Provider set, pixel generation, viewport indicator, navigate callback, SectorScanNavigator interpolation/completion/cancellation.
6. **Alert Integration (7 tests):** Push and count, max visible limit, expired removal, critical/warning duration, focus callback, mixed priority independent expiry.

### Test Quality Assessment

Tests use a simple custom test framework (TEST/RUN_TEST/ASSERT macros) rather than a third-party framework. This keeps external dependencies minimal and is appropriate for the project's build system.

**Strengths:**
- Good edge case coverage (null inputs, boundary values, overflow limits)
- Mock objects for simulation interfaces (MockGridOverlay, MockProbeQueryProvider, MockMinimapDataProvider) properly implement the interfaces
- Tests verify both state transitions and callback invocations
- Alert priority duration tests verify the documented 3s/5s/8s lifetimes

**Gaps:**
- No test for `KeyboardShortcuts.process_key()` or `get_key_name()` (see Issue #5)
- No test for `RadialMenu` hit testing or category/item selection geometry
- No test for `BudgetWindow::format_credits()` output format (thousands separators, "cr" suffix)
- The `budget_callbacks_fire` test invokes callbacks directly rather than through widget interaction, which does not fully test the integration path
- No rendering verification -- tests only check state, not that the correct draw calls are made via the stub renderers

## Issues Found

1. **Heavy include of UIManager.h for enum access.** `ScanLayerManager.h`, `ScanLayerColorScheme.h`, `AlertPulseSystem.h`, `RadialMenu.h`, `CommandArray.h`, and `ToolStateMachine.h` all include `UIManager.h` primarily to access `OverlayType`, `ToolType`, and/or `AlertPriority` enums. `UIManager.h` transitively includes `Widget.h`, `UISkin.h`, and `UIRenderer.h`. This means modifying any core type (e.g., adding a field to `UIState`) forces recompilation of most of the UI layer. The enums should be extracted into a lightweight `UITypes.h` header.

2. **ScanLayerRenderer single-buffer design vs. documented double-buffering.** The ticket notes (line 1134 of tickets.md) state: "Thread Safety: Overlay data uses double-buffering for safe read access from render thread." However, `ScanLayerRenderer` uses a single `OverlayTextureData m_texture` with a `dirty` flag. If the render thread reads the pixel buffer while the main thread writes it during `update_texture()`, a data race will occur. Either implement the documented double-buffering or ensure the texture upload happens synchronously on the main thread.

3. **UIManager alert duration is hardcoded to 3.0f regardless of priority.** In `UIManager::push_alert()` (UIManager.cpp line 153), `alert.time_remaining` is always set to `3.0f`, ignoring the priority level. This conflicts with the ticket specification and `AlertPulseSystem`, which correctly uses `get_priority_duration()` to set 3s/5s/8s based on priority. The `UIManager::push_alert()` method is likely vestigial since `AlertPulseSystem` handles alert lifecycle independently, but the inconsistency should be resolved.

4. **Dangling pointer risk in UIInputRouter.** `UIInputRouter` stores `m_hovered`, `m_focused`, and `m_last_hovered` as raw Widget pointers across frames. If a widget is removed from the tree between frames (e.g., by `Widget::remove_child()`), these pointers become dangling. While the single-threaded design mitigates the immediate risk, there is no invalidation mechanism. A widget removal during dynamic UI changes (closing a panel, for example) could cause a use-after-free on the next `process()` call.

5. **No tests for KeyboardShortcuts.** The `KeyboardShortcuts` class has no unit or integration tests. Its `process_key()` method, default bindings, modifier key handling, rebinding via `set_binding()`, and `get_key_name()` are untested. This is the only untested component among the 25 files.

6. **BudgetWindow "Issue Credit Advance" button is render-only.** In `BudgetWindow::render_bonds_tab()` (BudgetWindow.cpp lines 419-427), the "Issue Credit Advance" button is rendered via `renderer->draw_button()` but there is no corresponding click handler. The button is drawn as a visual element but cannot be interacted with because `BudgetWindow` does not override `on_mouse_down()` or wire up the `m_callbacks.on_issue_bond` callback to click detection.

7. **BudgetWindow tab switching is render-only.** Similarly, the tab buttons in `BudgetWindow::render_tabs()` are drawn but have no click handlers. Tab switching can only occur programmatically via `set_active_tab()`, not through user interaction. The budget window needs mouse event handling to be functional.

8. **ScanLayerManager fade transition defers active_type change.** When `set_active()` is called, the `m_active_type` does not change until the fade transition completes in `update()`. During the fade, `get_active_type()` returns the OLD overlay type while `m_fade_target` holds the new one. This means `get_active_overlay()` returns the wrong overlay during the transition period. The ScanLayerRenderer should use `m_fade_target` (the new overlay) during fade-in, not `m_active_type` (the old one).

9. **Rect and Color types are defined in Widget.h.** These utility types (`Rect`, `Color`) are fundamental data types used by `UIRenderer`, `UISkin`, and all widgets. Defining them in `Widget.h` creates an artificial dependency: files that only need `Rect` or `Color` must include the entire Widget class definition. These should be in their own lightweight header (e.g., `UITypes.h` or `UIGeometry.h`).

10. **AlertPulseSystem::on_mouse_down checks button != 1.** In `AlertPulseSystem.cpp` line 144, the early return condition is `if (button != 1)`. However, `UIInputRouter.cpp` line 58 passes `0` for left mouse button. This means clicking notifications will never trigger the dismiss/focus logic because the button values are inconsistent. The router sends `0` for left button, but the alert system expects `1`.

## Recommendations

1. **Extract enums to UITypes.h.** Create a lightweight `UITypes.h` containing `ToolType`, `OverlayType`, `AlertPriority`, `UIMode`, `InfraType`, `FontSize`, `ButtonState`, `TextureHandle`, and the `Rect`/`Color` structs. This breaks the heavy include chain through `UIManager.h` and significantly reduces recompilation cascade.

2. **Implement double-buffering in ScanLayerRenderer.** Add a second `OverlayTextureData` buffer and swap between them. The main thread writes to the back buffer while the render thread reads the front buffer. Mark the front buffer dirty only after a swap. This matches the documented design and prevents future data races during GPU integration.

3. **Fix the mouse button value inconsistency.** Standardize on either `0` (SDL convention: left=0) or `1` across the entire codebase. The simplest fix is changing `AlertPulseSystem::on_mouse_down()` to check `button != 0`.

4. **Add click handling to BudgetWindow.** Override `on_mouse_down()` in `BudgetWindow` to handle tab button clicks and the "Issue Credit Advance" button. Without this, the budget window is display-only and cannot be interacted with.

5. **Add KeyboardShortcuts tests.** Write unit tests covering `process_key()` with default bindings, modifier key combinations, `set_binding()` override/restore, `get_binding_for_action()` reverse lookup, and `get_key_name()` output.

6. **Add widget invalidation to UIInputRouter.** When a widget is removed from the tree, clear any stored pointers to it in UIInputRouter. This could be done via a notification from `Widget::remove_child()` or by storing widget IDs instead of raw pointers and validating them each frame.

7. **Harmonize alert management.** Either remove `UIManager::push_alert()` and route all alerts through `AlertPulseSystem`, or have `UIManager::push_alert()` delegate to `AlertPulseSystem` with correct priority-based durations. Having two parallel alert mechanisms with different behavior is confusing.

8. **Fix ScanLayerManager fade transition.** During the fade-in, `get_active_overlay()` should return the overlay for `m_fade_target` (the new overlay), not `m_active_type` (the old one). Otherwise, the renderer will display the old overlay fading in rather than the new one.

9. **Consider adding a `TextMeasure` interface.** Several widgets (LabelWidget, BudgetWindow, ColonyStatusBar) need to know text width for alignment, column layout, and centering. Currently they use hardcoded pixel offsets. A `TextMeasure` interface (or method on UIRenderer) would enable proper text layout when real fonts are integrated.

10. **Add rendering verification tests.** Use the stub renderers' `DrawStats` to verify that widgets make the expected draw calls. For example, verify that a visible `ButtonWidget::render()` increments `button_calls` by 1, or that `BudgetWindow::render()` on the Revenue tab makes the expected number of text draw calls.

## Verdict

**CONDITIONAL PASS**

The architecture is sound, the code quality is high, and the test coverage is substantial at 106 tests. The widget hierarchy, renderer abstraction, and simulation data binding patterns are well-designed and will scale to the full game.

However, three issues must be fixed before proceeding:

- **Issue #10 (Critical):** Mouse button value mismatch between `UIInputRouter` (sends 0) and `AlertPulseSystem` (expects 1) means alert click-to-dismiss and click-to-focus are non-functional. This is a straightforward one-line fix.
- **Issue #6 + #7 (Functional):** BudgetWindow tab buttons and "Issue Credit Advance" button have no click handling, making the budget panel non-interactive. This needs `on_mouse_down()` implementation.
- **Issue #3 (Consistency):** UIManager alert duration ignores priority, conflicting with AlertPulseSystem behavior.

These three issues should be resolved before the next epic. The remaining recommendations (enum extraction, double-buffering, widget invalidation, etc.) are improvements that can be addressed during render pipeline integration or as maintenance tasks.
