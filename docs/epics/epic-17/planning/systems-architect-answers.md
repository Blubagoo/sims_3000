# Systems Architect Answers: Epic 17 Planning Questions

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**For:** Game Designer and QA Engineer

---

## Answers to Game Designer Questions

### GD-1: Terminology Enforcement Infrastructure

**Question:** Is there a centralized string table or localization system? How are dynamic strings (formatting, concatenation) handled? Can we add compile-time checks for banned terms?

**Answer:**

**Current State:** The canon specifies externalized strings via YAML files (`terminology.yaml` for mappings, and per-language string tables like `strings/en/ui.yaml` as outlined in my Epic 17 analysis), but implementation is pending Epic 17. Currently, strings are likely embedded in code or component data.

**Recommended Architecture:**

1. **Centralized String Table:**
   ```cpp
   class StringTable {
       std::unordered_map<StringID, std::string> strings_;
   public:
       const std::string& get(StringID id) const;
       std::string format(StringID id, const FormatArgs& args) const;
   };
   ```

2. **Dynamic String Handling:**
   - Use format placeholders: `"Catastrophe warning: {disaster_type}"` rather than concatenation
   - Implement a `format()` method that substitutes placeholders
   - This preserves terminology consistency while allowing dynamic content

3. **Compile-Time Checks:**
   - **Feasible approach:** Create a `TerminologyValidator` tool that runs in CI as a pre-commit hook
   - Scan source files for string literals containing banned terms (from `terminology.yaml`)
   - Cannot truly be compile-time in C++, but can fail the build via static analysis step
   - Consider using `constexpr` string ID enums instead of raw strings:
     ```cpp
     // Instead of: display("Building complete")
     // Use: display(StringID::BUILDING_COMPLETE)
     ```

4. **Implementation Path for Epic 17:**
   - Work item E17-047 (String externalization) creates the StringTable
   - Work item E17-048 (Terminology validation tool) creates the CI checker
   - Add clang-tidy or custom linter rule for string literal scanning

**Actionable Recommendation:** Implement string IDs throughout the codebase (E17-047), then add a terminology linter that runs in CI and fails on violations (E17-048). True compile-time enforcement isn't practical in C++, but CI enforcement is equally effective.

---

### GD-2: Balance Data Externalization

**Question:** Are balance values (costs, rates, thresholds) in config files or hardcoded? Can we hot-reload balance changes without recompiling? Is there a tuning override system for playtesting?

**Answer:**

**Current Canon State:** Canon specifies YAML files for configuration (terminology, patterns, interfaces), but balance values are not explicitly addressed. Building costs, tribute rates, service effectiveness, etc. are likely defined in component defaults or system constants.

**Recommended Architecture:**

1. **Balance Data Files:**
   ```yaml
   # config/balance/buildings.yaml
   habitation:
     low_density:
       construction_cost: 100
       maintenance_cost: 2
       energy_required: 5
       fluid_required: 3
       capacity: 50
   ```

2. **Hot-Reload Support:**
   ```cpp
   class BalanceConfig {
       void load(const std::string& path);
       void reload();  // Re-read without restart

       // Notify systems of changes
       EventBus& events_;  // Emit BalanceChangedEvent
   };
   ```
   - Systems subscribe to `BalanceChangedEvent` and refresh cached values
   - File watcher for automatic reload during development

3. **Tuning Override System:**
   ```yaml
   # config/overrides/playtest_2026_02_01.yaml
   overrides:
     - path: "buildings.habitation.low_density.construction_cost"
       value: 50  # Half price for testing
     - path: "disasters.conflagration.spread_rate"
       value: 0.5  # Slower fire spread
   ```
   - Override files layer on top of base balance
   - Server command to toggle override profiles: `/balance load playtest_aggressive`
   - Overrides apply at runtime without restart

4. **Implementation for Epic 17:**
   - This is primarily a technical debt item, not explicitly in my work items
   - **Recommendation:** Add work items for balance externalization during technical debt phase
   - Priority: Medium - improves iteration speed but not blocking for release

**Actionable Recommendation:** Create a `BalanceConfig` class that loads YAML balance files and supports hot-reload. Add override capability via layered config files. This enables rapid playtesting iteration without recompilation.

---

### GD-3: Accessibility System Support

**Question:** Is there a global settings system that persists colorblind/text scale preferences? Can shaders be swapped at runtime for colorblind modes? How do we handle audio-only mode for screen readers?

**Answer:**

**Current Canon State:**
- `SettingsManager` (Epic 16) handles client settings persistence with `ISettingsProvider` interface
- Settings stored in JSON at platform-specific paths (`%APPDATA%/ZergCity/settings.json`)
- `ClientSettings` includes video, audio, controls, and UI categories

**Architecture Support:**

1. **Global Settings Persistence:**
   ```cpp
   struct ClientSettings {
       VideoSettings video;  // Resolution, fullscreen, quality
       // ADD:
       AccessibilitySettings accessibility {
           ColorblindMode colorblind_mode = ColorblindMode::None;
           float text_scale = 1.0f;
           bool high_contrast = false;
           bool reduce_motion = false;
           bool screen_reader_mode = false;
       };
   };
   ```
   - Already have platform-specific paths in `SettingsManager`
   - Persists automatically via JSON

2. **Runtime Shader Swapping:**
   - SDL_GPU supports runtime shader binding
   - Create colorblind-corrected shader variants:
     ```cpp
     enum class ColorblindMode { None, Protanopia, Deuteranopia, Tritanopia };

     void RenderingSystem::set_colorblind_mode(ColorblindMode mode) {
         // Swap post-process shader or modify color LUT
         post_process_shader_ = colorblind_shaders_[mode];
     }
     ```
   - Recommended approach: Use a color lookup table (LUT) texture that can be swapped at runtime
   - More efficient than per-object shader swapping

3. **Screen Reader / Audio-Only Mode:**
   - **Partial support possible** via text-to-speech integration
   - UI elements would need `aria_label`-style metadata:
     ```cpp
     struct UIElement {
         std::string screen_reader_text;  // Descriptive text for TTS
     };
     ```
   - Emit audio cues for UI focus changes, notifications, alerts
   - **Significant scope:** Would require additional work items beyond current Epic 17 plan

**Implementation for Epic 17:**

| Feature | Difficulty | Work Item |
|---------|------------|-----------|
| Colorblind settings persistence | Low | Add to E17-022 (Error handling/settings) |
| Colorblind shader/LUT swap | Medium | New work item needed |
| Text scaling | Medium | New work item needed |
| High contrast mode | Medium | New work item needed |
| Screen reader mode | High | Out of scope for Epic 17 |

**Actionable Recommendation:** Add accessibility settings to `ClientSettings` (low effort). Implement colorblind LUT swapping and text scaling for Epic 17. Screen reader support is significant scope - recommend deferring to post-release accessibility update.

---

### GD-4: Performance Budget for UI Animations

**Question:** What's the frame time budget for UI animations? Can we add particle effects for celebrations without impacting simulation? Are there rendering passes available for outline/glow effects on focus?

**Answer:**

**Frame Time Budget Analysis:**

| Budget Category | Allocation | Notes |
|-----------------|------------|-------|
| Total Frame (60 FPS) | 16.67 ms | Target for recommended hardware |
| Simulation Tick (20 Hz) | 0-50 ms | Amortized: ~2.5ms average impact per frame |
| Rendering (3D scene) | 8-10 ms | Buildings, terrain, vegetation |
| UI Rendering | 2-3 ms | Panels, overlays, text |
| **UI Animations** | **0.5-1 ms** | Panel slides, fades, number transitions |
| Post-processing | 1-2 ms | Toon outline, bloom, colorblind LUT |
| Frame overhead | 1-2 ms | Vsync, buffer swap, input |

**UI Animation Budget:** ~1ms available for all UI animations combined. Canon already specifies animation durations:
- Panel slide: 200ms ease-out-cubic
- Overlay fade: 250ms linear
- Button hover: 100ms linear
- Notification fade: 300ms ease-out
- Value change: 200ms linear

These are wall-clock durations, not frame time. Per-frame cost should be minimal (alpha blending, position interpolation).

**Particle Effects for Celebrations:**

Yes, can add without impacting simulation because:
1. **Particles are client-only** - never synced, no server cost
2. **Rendered in separate pass** - doesn't affect simulation tick
3. **Budget consideration:** Allocate 1-2ms for celebration particles during events

Recommended implementation:
```cpp
// Client-side only particle system
class ParticleSystem {
    void spawn_celebration(GridPosition pos);  // Fireworks, confetti
    void update(float dt);  // CPU-side particle simulation
    void render();  // GPU instanced rendering
};
```

**Glow/Outline Effects on Focus:**

Current pipeline has:
- Toon outline pass (silhouette edges)
- Bloom post-processing (glow bleed)

For focus highlighting, options:
1. **Emissive boost:** Increase emissive color of focused entity (0 cost - just a uniform change)
2. **Separate outline pass:** Render focused entities with thicker/different color outline (1ms additional)
3. **Selection glow:** Use stencil buffer to mask focused entity, apply blur/glow (1-2ms)

**Actionable Recommendation:** UI animations are well within budget at 1ms. Celebration particles can be added client-side. For focus effects, use emissive boost (free) or stencil-based glow (1-2ms budget from post-processing allocation).

---

### GD-9: Animation Performance

**Question:** What's the budget for UI animations (panel slides, fades)? Can celebration effects be disabled for low-spec machines? How do we handle animation stacking (multiple celebrations)?

**Answer:**

**UI Animation Budget:**

Same as GD-4: ~1ms for all UI animations combined. Panel slides and fades are very cheap operations:
- Panel slide: Transform interpolation (microseconds)
- Fade: Alpha blending (built into GPU pipeline)
- Number animation: Text redraw at 60fps (sub-ms)

The 200-300ms durations in canon are animation lengths, not performance costs.

**Celebration Effects on Low-Spec:**

Yes, should be disableable:
```cpp
struct VideoSettings {
    QualityLevel particle_quality = QualityLevel::High;
    // High: Full celebration effects
    // Medium: Reduced particle count
    // Low: Minimal or disabled

    bool enable_celebration_effects = true;  // Toggle
};
```

Auto-scaling based on framerate:
```cpp
void ParticleSystem::spawn_celebration(GridPosition pos) {
    if (!settings_.enable_celebration_effects) return;

    int particle_count = base_count * quality_multipliers_[settings_.particle_quality];
    // ...
}
```

**Animation Stacking Strategy:**

When multiple celebrations trigger simultaneously (e.g., milestone + arcology complete):

1. **Queue-based approach:**
   ```cpp
   class CelebrationQueue {
       std::queue<CelebrationEvent> pending_;

       void enqueue(CelebrationEvent event) {
           // Deduplicate similar events
           // Limit queue size (max 3 pending)
           pending_.push(event);
       }

       void update() {
           if (current_celebration_.is_complete() && !pending_.empty()) {
               start_celebration(pending_.front());
               pending_.pop();
           }
       }
   };
   ```

2. **Priority-based display:**
   - Critical (milestone, transcendence): Full celebration, queue others
   - High (arcology, disaster survived): Moderate celebration
   - Normal (building complete): Brief flash/sound only if queue is full

3. **Concurrent limit:** Maximum 2 active particle effects at once
   - Prevents frame drops from particle overload
   - Lower priority celebrations show abbreviated version

**Actionable Recommendation:** Budget is not a concern for UI animations. Add quality settings for particles with disable option. Implement celebration queue with priority levels and concurrent limit of 2 active effects.

---

### GD-10: Accessibility Implementation

**Question:** Is there a UI framework feature for focus management? How do we implement high-contrast mode (CSS-like theming)? Can tooltips adapt to text scaling automatically?

**Answer:**

**Focus Management:**

Current UI architecture (from canon UISystem specification) owns all UI rendering and state. For accessibility:

```cpp
class UISystem {
    EntityID focused_element_ = INVALID_ENTITY;

    void set_focus(EntityID element);
    EntityID get_focused() const;
    void move_focus(Direction dir);  // Arrow key navigation

    // Tab order
    std::vector<EntityID> focusable_elements_;
    size_t focus_index_ = 0;
};
```

Focus features needed:
- Visual focus indicator (glow/outline per GD-4)
- Tab order traversal
- Focus announcement for screen readers (if implemented)
- Focus trapping in dialogs

**High-Contrast Mode Implementation:**

CSS-like theming via color palette swapping:

```cpp
struct UIColorPalette {
    Color background;
    Color panel_background;
    Color text_primary;
    Color text_secondary;
    Color accent;
    Color border;
    Color focus_indicator;
};

// Predefined palettes
const UIColorPalette PALETTE_DEFAULT = { /* bioluminescent theme */ };
const UIColorPalette PALETTE_HIGH_CONTRAST = {
    .background = Color::Black,
    .panel_background = Color::Black,
    .text_primary = Color::White,
    .text_secondary = Color::Yellow,
    .accent = Color::Cyan,
    .border = Color::White,
    .focus_indicator = Color::Yellow,
};

class UISystem {
    UIColorPalette active_palette_ = PALETTE_DEFAULT;

    void set_high_contrast(bool enabled) {
        active_palette_ = enabled ? PALETTE_HIGH_CONTRAST : PALETTE_DEFAULT;
        // All UI elements reference active_palette_ colors
    }
};
```

**Tooltip Text Scaling:**

Auto-scaling tooltips:

```cpp
class TooltipComponent {
    std::string text;
    // No fixed dimensions - calculated at render time
};

class UISystem {
    float text_scale_ = 1.0f;  // From AccessibilitySettings

    void render_tooltip(const TooltipComponent& tooltip, Vec2 position) {
        float scaled_font_size = base_font_size_ * text_scale_;
        Vec2 text_size = measure_text(tooltip.text, scaled_font_size);

        // Tooltip box sizes to fit scaled text
        Rect box = { position, text_size + padding * 2 };

        // Clamp to screen bounds
        box = clamp_to_screen(box);

        render_box(box);
        render_text(tooltip.text, box.position + padding, scaled_font_size);
    }
};
```

Key considerations:
- Tooltips should never have fixed pixel sizes
- Calculate dimensions from text measurement at runtime
- Clamp to screen edges (scale-aware)
- Consider multi-line wrapping for very large text scales

**Actionable Recommendation:**
1. Add focus management to UISystem with tab order and visual indicator
2. Implement color palette system with high-contrast variant
3. Make all tooltips dynamically sized based on text scale
4. Store text_scale in ClientSettings.accessibility

---

## Answers to QA Engineer Questions

### QA-1: Known Technical Debt

**Question:** What technical debt from Epics 0-16 should be prioritized for cleanup in Epic 17? Are there any "quick fixes" that need proper solutions?

**Answer:**

**Priority Technical Debt (from my analysis):**

**Critical (Fix in Epic 17):**

| Debt Item | Source Epic | Issue | Proper Solution |
|-----------|-------------|-------|-----------------|
| Stub interfaces | Epics 4, 9, 10, 11 | IServiceQueryable, IEconomyQueryable stubs | Replace with real implementations (done by later epics, verify cleanup) |
| Event ordering | All simulation epics | Circular dependencies (building->energy->zone) | Implement phased event processing (E17-023, E17-024) |
| Dense grid memory | Epic 3, 10, 13 | Always-allocated ConflagrationGrid | Lazy allocation (E17-012) |

**High (Should fix in Epic 17):**

| Debt Item | Source Epic | Issue | Proper Solution |
|-----------|-------------|-------|-----------------|
| Pattern inconsistencies | All epics | Naming violations (camelCase fields, wrong suffixes) | Automated audit + fix (E17-013, E17-014) |
| Missing error handling | All network code | Silent failures | Consistent error pattern audit (E17-019, E17-020) |
| Raw pointers | Unknown | Potential leaks | RAII audit (E17-021, E17-022) |
| Hardcoded balance values | All simulation | Constants in code | Externalize to config (new work item recommended) |

**Medium (Fix if time permits):**

| Debt Item | Source Epic | Issue | Proper Solution |
|-----------|-------------|-------|-----------------|
| TODO/FIXME markers | All epics | Unresolved markers | Audit and prioritize (E17-015, E17-016, E17-017) |
| Dead code | All epics | Unused stubs, debug code | Remove (E17-018) |
| Test coverage gaps | All epics | Integration test gaps | Fill gaps (E17-046) |

**Known "Quick Fixes" Needing Proper Solutions:**

1. **Sync system shortcuts:** Early tick optimization that skips sync for "unchanged" entities may miss edge cases
2. **Disaster cascade:** Fire spread may use simplified propagation that doesn't account for all flammability factors
3. **Service coverage calculation:** May use bounding box instead of actual radius for performance

**Actionable Recommendation:** Prioritize stub interface verification (ensure all stubs replaced), event ordering fix, and pattern consistency audit. These have highest impact on code quality and bug prevention.

---

### QA-2: Performance Hotspots

**Question:** From profiling during development, which systems are the biggest performance concerns at scale? Where should optimization efforts focus?

**Answer:**

**Projected Performance Hotspots (512x512 map, maximum entities):**

| System | Tick Priority | Concern Level | Issue | Mitigation |
|--------|---------------|---------------|-------|------------|
| ContaminationSystem | 80 | **HIGH** | Queries all fabrication + traffic tiles every tick | Spatial partitioning, dirty chunk tracking (E17-009) |
| DisorderSystem | 70 | **HIGH** | Queries all buildings + population density | Cached query results, spatial partitioning |
| LandValueSystem | 85 | **HIGH** | Reads all grids (disorder, contamination, terrain) | Grid sampling (1:4 ratio), spread across ticks (E17-010) |
| TransportSystem | 45 | **MEDIUM** | Pathway connectivity recalculation | Incremental connectivity updates |
| PopulationSystem | 50 | **MEDIUM** | Iterates all habitation buildings | Dirty flag tracking |
| RenderingSystem | N/A | **HIGH** | Draw calls for 50K+ entities | Instancing (E17-003-005), LOD (E17-001), culling (E17-002) |
| SyncSystem | N/A | **MEDIUM** | Delta calculation for large state | Field-level delta (E17-031), batching (E17-033) |

**Optimization Priority Order:**

1. **Rendering (highest visual impact):** LOD + instancing + culling can reduce draw calls from 50K to ~500
2. **ContaminationSystem:** Most expensive per-tick system, spatial partitioning gives 10x speedup
3. **LandValueSystem:** Spread calculation across ticks to avoid spikes
4. **Network sync:** Delta compression reduces bandwidth 80%+

**Profiling Recommendations:**

```cpp
// Add tick profiling infrastructure
class SimulationCore {
    void tick() {
        for (auto& system : systems_) {
            auto start = high_resolution_clock::now();
            system->tick(dt);
            auto duration = high_resolution_clock::now() - start;
            profile_data_[system->name()].record(duration);
        }
    }
};
```

**Actionable Recommendation:** Focus optimization on rendering pipeline first (visual), then ContaminationSystem and LandValueSystem (tick stability). Network optimization is important for multiplayer but less critical than framerate.

---

### QA-3: Memory Ownership Patterns

**Question:** Are there any ambiguous memory ownership patterns that could cause leaks? Any shared_ptr cycles or raw pointers without clear ownership?

**Answer:**

**Canon-Specified Patterns (should be followed):**

From `patterns.yaml`:
- "RAII for all resource ownership"
- "Smart pointers for heap allocations"
- "No raw new/delete outside of low-level allocators"
- "No pointers in components - use entity IDs for references"

**Potential Problem Areas:**

| Area | Risk | Pattern to Verify |
|------|------|-------------------|
| Component references | LOW | Should use EntityID, not pointers |
| System cross-references | MEDIUM | Systems may hold raw pointers to other systems |
| Asset handles | MEDIUM | AssetManager should own assets, handles should not prevent cleanup |
| Event subscriptions | MEDIUM | EventBus listeners could create dangling references |
| Network message buffers | MEDIUM | Serialization buffers should use RAII |

**Potential shared_ptr Cycles:**

| Cycle Risk | Systems Involved | Pattern |
|------------|------------------|---------|
| System-EventBus | System subscribes to events, EventBus holds subscriber list | Use weak_ptr for subscribers |
| Entity-Component | Unlikely - ECS uses value semantics | Verify no shared_ptr in components |
| UI-System | UISystem queries other systems | Should be interface pointers, not owning |

**Audit Checklist for E17-021:**

```cpp
// VERIFY each category:

// 1. No raw new/delete
// Search for: \bnew\b, \bdelete\b (outside allocators)

// 2. No shared_ptr in components
// Search for: shared_ptr.*Component

// 3. EventBus uses weak references
// EventBus::subscribe should store weak_ptr or raw pointer with explicit lifetime

// 4. Asset handles don't prevent cleanup
// AssetHandle should be non-owning or weak_ptr based

// 5. System references are non-owning
// Systems injected via interface should not be stored as shared_ptr
```

**Actionable Recommendation:** Run static analysis for raw new/delete usage. Audit EventBus subscription pattern for weak_ptr usage. Verify AssetHandle lifetime semantics. These are the highest-risk areas for leaks.

---

### QA-4: Thread Safety Concerns

**Question:** Beyond the documented patterns, are there any threading edge cases that weren't fully addressed? Any systems that might have subtle race conditions?

**Answer:**

**Threading Model (from canon):**

- Server runs simulation at 20 ticks/second (single-threaded simulation)
- Client renders at 60 FPS (separate thread from network)
- Network I/O on dedicated threads

**Known Thread Safety Concerns:**

| Concern | Systems | Risk | Mitigation |
|---------|---------|------|------------|
| Render reads during tick | RenderingSystem reads components while SimulationCore writes | **HIGH** | Double-buffer or mutex per-component type |
| Event dispatch during iteration | EventBus dispatch while system modifies entity | MEDIUM | Copy event list or deferred dispatch |
| Asset loading callback | AssetManager async load completes during render | MEDIUM | Thread-safe asset state transitions |
| Network message parsing | Messages arrive during tick processing | LOW | Message queue with mutex |

**Specific Race Condition Risks:**

1. **RenderingSystem accessing ITerrainRenderData:**
   - Canon notes: "Thread-safe for read access during render"
   - **Verify:** TerrainSystem write doesn't overlap with RenderingSystem read
   - Solution: Copy-on-write or read-write lock

2. **IRenderStateProvider interpolation:**
   - SyncSystem updates `previous_position` / `current_position`
   - RenderingSystem reads for interpolation
   - **Risk:** Reading during swap
   - Solution: Atomic swap of position buffers

3. **Dense grid double-buffering:**
   - DisorderGrid, ContaminationGrid use double-buffer
   - `grid_buffer_swap()` at end of tick
   - **Risk:** Swap during render read
   - Solution: Swap only happens on simulation thread, render reads "read buffer"

4. **Event queue during system tick:**
   - System emits event, another system subscribed
   - Immediate dispatch could modify state mid-tick
   - Solution: Queue events, dispatch at tick boundaries

**Audit Focus for E17:**

```cpp
// Thread safety checklist:
// 1. All data read by RenderingSystem has thread-safe access
// 2. EventBus dispatch is deferred (not immediate)
// 3. Asset loading callbacks marshal to main thread
// 4. Dense grid buffer swap is atomic or serialized
// 5. Network message queue protected by mutex
```

**Actionable Recommendation:** Primary concern is render thread reading simulation data. Verify double-buffering is correctly implemented for all render-visible state. Add thread sanitizer (TSan) to CI for automated race detection.

---

### QA-5: Dense Grid Optimization

**Question:** Are the dense grids (especially 512x512 maps) using optimal memory layouts? Any opportunities for cache optimization or SIMD?

**Answer:**

**Current Dense Grid Inventory:**

| Grid | Bytes/Tile | 512x512 Total | Access Pattern |
|------|------------|---------------|----------------|
| TerrainGrid | 4 | 1 MB | Random read, rare write |
| BuildingGrid | 4 | 1 MB | Random read/write |
| EnergyCoverageGrid | 1 | 256 KB | Flood-fill write, random read |
| FluidCoverageGrid | 1 | 256 KB | Flood-fill write, random read |
| PathwayGrid | 4 | 1 MB | Sparse, random access |
| ProximityCache | 1 | 256 KB | BFS write, random read |
| DisorderGrid | 1 x 2 | 512 KB | Full scan, double-buffered |
| ContaminationGrid | 2 x 2 | 1 MB | Neighbor scan, double-buffered |
| LandValueGrid | 2 | 512 KB | Read-mostly |
| ServiceCoverageGrid | 1 x 4 | 1 MB | Radius write, random read |
| ConflagrationGrid | 3 | 768 KB | Neighbor scan during disasters |

**Memory Layout Analysis:**

**Current (likely):** Row-major 2D arrays
```cpp
uint8_t disorder_grid[512][512];  // Row-major
```

**Optimization Opportunities:**

1. **Cache-Friendly Tile Layout:**
   For systems that scan neighbors (contamination spread, fire spread), use Z-order (Morton) or tiled layout:
   ```cpp
   // Instead of: grid[y][x]
   // Use: grid[tile_index(x, y)]
   // Where tiles are 4x4 or 8x8 blocks

   // 8x8 tiles: neighbors within same cache line
   constexpr int TILE_SIZE = 8;
   size_t tile_index(int x, int y) {
       int tile_x = x / TILE_SIZE;
       int tile_y = y / TILE_SIZE;
       int local_x = x % TILE_SIZE;
       int local_y = y % TILE_SIZE;
       return (tile_y * tiles_per_row + tile_x) * TILE_SIZE * TILE_SIZE
              + local_y * TILE_SIZE + local_x;
   }
   ```

2. **SIMD Opportunities:**

   | Operation | SIMD Applicability | Speedup |
   |-----------|-------------------|---------|
   | Contamination decay | **HIGH** - uniform subtract across grid | 8-16x |
   | Disorder spread | **HIGH** - neighbor averaging | 4-8x |
   | Coverage flood fill | MEDIUM - conditional per-tile | 2-4x |
   | Land value sum factors | **HIGH** - sum multiple grids | 4-8x |

   Example SIMD contamination decay:
   ```cpp
   // Process 16 tiles at once with AVX2
   void decay_contamination_simd(uint16_t* grid, int count, uint16_t decay) {
       __m256i decay_vec = _mm256_set1_epi16(decay);
       for (int i = 0; i < count; i += 16) {
           __m256i values = _mm256_loadu_si256((__m256i*)&grid[i]);
           values = _mm256_subs_epu16(values, decay_vec);  // Saturating subtract
           _mm256_storeu_si256((__m256i*)&grid[i], values);
       }
   }
   ```

3. **Sparse Grid Conversion:**
   PathwayGrid and BuildingGrid are mostly empty (sparse). Convert to:
   ```cpp
   class SparseGrid<T> {
       std::unordered_map<uint32_t, T> data_;  // Only non-default entries
       T default_value_;

       T get(int x, int y) const {
           auto it = data_.find(pack(x, y));
           return it != data_.end() ? it->second : default_value_;
       }
   };
   ```
   Memory savings: 50-80% for typical maps

**Actionable Recommendation:**
1. Convert PathwayGrid/BuildingGrid to sparse storage (E17-011)
2. Add SIMD paths for contamination/disorder decay (new optimization work item)
3. Consider tiled memory layout for ContaminationGrid/ConflagrationGrid neighbor scans

---

### QA-6: Network Protocol Stability

**Question:** Is the network message format finalized, or are there pending changes that could affect backward compatibility?

**Answer:**

**Current Protocol State:**

From canon (interfaces.yaml ISerializable):
- Little-endian byte order
- Fixed-size types (uint32_t, not int)
- Version your serialization format

**Message Categories (from patterns.yaml):**

| Direction | Messages |
|-----------|----------|
| Client-to-Server | InputMessage, JoinMessage, ChatMessage, PurchaseTileMessage, TradeMessage |
| Server-to-Client | StateUpdateMessage, EventMessage, FullStateMessage, TradeNotification |

**Stability Assessment:**

| Area | Stability | Notes |
|------|-----------|-------|
| Core message types | **STABLE** | Basic structure unlikely to change |
| Component serialization | **UNSTABLE** | Components evolve, need versioning |
| Dense grid format | **UNSTABLE** | May optimize delta encoding |
| Event payloads | **SEMI-STABLE** | New events may be added |

**Backward Compatibility Strategy:**

1. **Message versioning:**
   ```cpp
   struct MessageHeader {
       uint16_t message_type;
       uint16_t message_version;  // Per-message-type version
       uint32_t payload_size;
   };
   ```

2. **Component versioning (per canon persistence patterns):**
   ```cpp
   // Save files store version with component data
   // On load, apply migration functions: v1->v2->v3->current
   ```

3. **Protocol negotiation:**
   ```cpp
   // On connect, client and server exchange supported versions
   // Use lowest common version or reject if incompatible
   ```

**Pending Changes That May Affect Compatibility:**

| Change | Impact | Mitigation |
|--------|--------|------------|
| Field-level delta (E17-031) | Delta message format changes | Version bump, fallback to full-component delta |
| Grid region delta (E17-032) | Grid sync format changes | Version bump, new message type |
| Message batching (E17-033) | Batch wrapper message | Additive - old clients can receive individual |

**Actionable Recommendation:** Implement protocol version negotiation before Epic 17 optimization changes. Ensure message format changes increment version numbers. Maintain one prior version for short-term compatibility during updates.

---

### QA-7: Save Format Versioning

**Question:** What's the migration testing strategy for save files from development builds? How many historical versions need support?

**Answer:**

**Save Format from Canon (patterns.yaml persistence):**

- Binary format: Header + Table of Contents + Compressed Sections
- Magic: "ZCSV" (ZergCity Save)
- Per-section versioning
- LZ4 compression per-section
- Migration function chain: v1 -> v2 -> v3 -> current

**Migration Testing Strategy:**

1. **Version Matrix Testing:**
   ```
   For each supported save version V:
     Load save_v{V}.zcsave
     Verify migration applies correctly
     Verify all entities/components load
     Run simulation for N ticks
     Verify no crashes or assertion failures
     Re-save and verify round-trip
   ```

2. **Component Migration Test Cases:**
   ```cpp
   // Test fixture per component type
   TEST(ComponentMigration, EnergyComponent_v1_to_v2) {
       auto v1_data = load_raw_component_v1("energy_v1.bin");
       auto v2_component = migrate_energy_v1_to_v2(v1_data);
       EXPECT_EQ(v2_component.energy_required, expected_value);
       EXPECT_EQ(v2_component.new_field, default_value);  // New fields get defaults
   }
   ```

3. **Integration Test Saves:**
   - Create saves at each milestone during development
   - Store in `test/saves/` with version tags
   - CI loads all historical saves and verifies

**Historical Version Support:**

| Version Range | Support Level | Rationale |
|---------------|---------------|-----------|
| Current release (N) | Full | Active players |
| Previous release (N-1) | Full | Recent updates |
| N-2 to N-3 | Migration only | Older saves loadable |
| N-4 and older | Unsupported | Warn on load, provide upgrade path |

Canon notes: "Very old versions may be dropped after major releases"

**Recommended Version Retention:**

- **Development builds:** Support last 5 builds (auto-generated version numbers)
- **Release builds:** Support current + 2 prior releases
- **Migration path:** One-version-at-a-time migration (v1->v2->v3, not v1->v3)

**CI Integration:**

```yaml
# ci/save_compatibility.yaml
test_saves:
  - saves/v0.16.0_small_map.zcsave
  - saves/v0.16.0_large_map.zcsave
  - saves/v0.15.0_migration_test.zcsave
  - saves/v0.14.0_disaster_active.zcsave

actions:
  - load_save
  - verify_entity_count
  - simulate_100_ticks
  - verify_no_errors
  - re_save
  - compare_checksum
```

**Actionable Recommendation:** Create versioned test saves for each epic during development. Run migration tests in CI for all supported versions. Support current + 2 prior releases for production, with clear "upgrade required" messaging for older saves.

---

### QA-8: Platform-Specific Concerns

**Question:** Are there any Windows-specific vs Linux-specific issues that haven't been fully tested?

**Answer:**

**Platform Support Matrix:**

| Platform | Status | Primary Concerns |
|----------|--------|------------------|
| Windows | Primary dev platform | Most tested |
| Linux | Supported | Needs explicit testing |
| macOS | TBD | SDL3 supports, needs build setup |

**Known Platform-Specific Issues:**

**Windows:**
| Issue | Area | Notes |
|-------|------|-------|
| Path separators | File I/O | Use std::filesystem::path, not hardcoded "/" |
| Settings path | Persistence | `%APPDATA%/ZergCity/` - verified in canon |
| Console output | Debug | May need console allocation for logging |
| Thread affinity | Performance | Windows scheduler differs from Linux |

**Linux:**
| Issue | Area | Notes |
|-------|------|-------|
| Case sensitivity | File paths | Asset paths must match case exactly |
| Settings path | Persistence | `~/.config/zergcity/` - verify tilde expansion |
| Wayland vs X11 | SDL3 | SDL3 handles, but window behavior may differ |
| Timer precision | Simulation | Verify high-res timer availability |
| Dynamic linking | Distribution | May need to bundle SDL3 or require system install |

**Untested Areas:**

| Area | Risk | Test Needed |
|------|------|-------------|
| Settings file encoding | MEDIUM | Non-ASCII characters in player names |
| Large file handles | LOW | 512x512 saves may hit file size limits |
| Network byte order | **FIXED** | Canon specifies little-endian, verify on big-endian if ever supported |
| GPU shader compilation | MEDIUM | SDL_GPU shader format across drivers |
| Audio device enumeration | MEDIUM | Device hotplug behavior |

**Testing Recommendations:**

```bash
# Linux-specific CI tests
- Build with GCC and Clang
- Run on Ubuntu LTS and Fedora
- Verify settings path creation
- Test case-sensitive asset loading
- Verify SDL_GPU with Mesa (Intel, AMD) and NVIDIA

# Windows-specific CI tests
- Build with MSVC and MinGW
- Verify %APPDATA% path handling
- Test with Windows Defender real-time scanning (performance)
- Verify DirectX backend for SDL_GPU
```

**Path Handling Audit:**

```cpp
// VERIFY all file paths use std::filesystem
// Search for: fopen\(.*".*[/\\]"
// Replace with: std::filesystem::path based API

// Settings path (from canon)
#ifdef _WIN32
    auto settings_path = std::filesystem::path(getenv("APPDATA")) / "ZergCity";
#elif __APPLE__
    auto settings_path = std::filesystem::path(getenv("HOME")) / "Library/Application Support/ZergCity";
#else
    auto settings_path = std::filesystem::path(getenv("HOME")) / ".config/zergcity";
#endif
```

**Actionable Recommendation:** Add Linux CI build and test pipeline. Audit all file path handling for cross-platform compatibility. Test GPU shader compilation across NVIDIA, AMD, and Intel drivers. Verify settings persistence on both platforms before release.

---

## Summary of Critical Items

**For Game Designer:**
1. String externalization with ID-based lookup (E17-047) plus CI terminology checker (E17-048)
2. Balance data externalization recommended as additional work item
3. Accessibility: colorblind LUT and text scaling feasible; screen reader mode out of scope
4. Performance budgets are adequate for planned UI animations and celebrations

**For QA Engineer:**
1. Technical debt priority: stub verification, event ordering, pattern consistency
2. Performance focus: rendering pipeline, ContaminationSystem, LandValueSystem
3. Memory audit: focus on EventBus subscriptions and asset handle lifetimes
4. Thread safety: verify double-buffering for render-visible state, add TSan to CI
5. Dense grid optimization: sparse conversion for PathwayGrid/BuildingGrid, SIMD for decay operations
6. Network: implement protocol versioning before Epic 17 optimizations
7. Save versioning: support current + 2 releases, CI tests for all supported versions
8. Platform: add Linux CI pipeline, audit file paths for cross-platform compatibility

---

**End of Systems Architect Answers**
