# Systems Architect Answers: Epic 2 Discussion

**Date:** 2026-01-27

---

## Answers to Graphics Engineer Questions

### GE-Q1: TransformComponent vs PositionComponent

**Decision: Keep both components with distinct purposes.**

**Rationale:**

The canon in `patterns.yaml` defines `PositionComponent` as grid-based coordinates:
```cpp
struct PositionComponent {
    int32_t grid_x = 0;
    int32_t grid_y = 0;
    int32_t elevation = 0;  // 0-31 (actual 3D height levels)
};
```

This component is used by ALL game systems: zoning, building placement, power coverage, water coverage, pathfinding, multiplayer sync. It represents the **logical** position in the game world.

For 3D rendering, introduce a **TransformComponent** that contains derived render data:
```cpp
struct TransformComponent {
    glm::mat4 model_matrix;     // World transform for rendering
    glm::vec3 world_position;   // Derived from PositionComponent
    float scale = 1.0f;         // For construction animations, etc.
    float rotation_y = 0.0f;    // Facing direction (in radians)
};
```

**Separation of concerns:**
- `PositionComponent`: Game logic, network sync, queries, spatial index. **Authoritative.**
- `TransformComponent`: Rendering only, derived from PositionComponent. **Client-side, interpolated.**

**System responsibilities:**
- `RenderingSystem` owns TransformComponent updates (derives from PositionComponent each frame)
- Game systems only read/write PositionComponent
- Network syncs PositionComponent only (TransformComponent is reconstructed client-side)

This prevents coupling between game logic and rendering details.

---

### GE-Q2: glTF Loading Location

**Decision: Epic 0 AssetManager with glTF capability.**

**Rationale:**

Canon `systems.yaml` defines AssetManager in Epic 0:
```yaml
AssetManager:
  owns:
    - Loading sprites, textures, audio
    - Asset caching
    - Async loading queue
    - Asset lifetime management
```

With the 3D transition, this should expand to "Loading models, textures, audio" - glTF is the model format, just as PNG was the sprite format.

**Why Epic 0:**
1. **Single source of truth** for asset loading - avoids duplicate caching logic
2. **Async loading** is already Epic 0's responsibility - glTF models need this
3. **Epic 2 should focus on rendering**, not file I/O and parsing
4. **Future epics** will need model loading (buildings, vehicles, beings) without Epic 2 dependency

**Implementation guidance:**
- Epic 0: Add `AssetManager::load<Model>(path)` with glTF parsing
- Epic 2: RenderingSystem queries loaded models, handles GPU upload and material binding

**glTF library:** Use cgltf (single-header C library) or tinygltf for parsing. Keep the actual GPU resource creation in Epic 2's domain.

---

### GE-Q3: UI Layer Approach

**Decision: SDL_Renderer for 2D UI overlaid on SDL_GPU 3D scene.**

**Rationale:**

Canon `systems.yaml` assigns UI to Epic 12:
```yaml
UISystem:
  owns:
    - All UI rendering
    - Toolbars, panels, dialogs
    - Overlays (crime, pollution, power, etc.)
```

For Epic 2 (rendering foundation), the question is: what infrastructure do we provide?

**Recommendation: Keep UI separate from 3D pipeline.**

1. **SDL3 supports both** - SDL_Renderer (2D) and SDL_GPU (3D) can coexist
2. **UI doesn't need 3D** - Panels, buttons, text are inherently 2D
3. **Simpler pipeline** - 3D scene renders to texture, composited with UI in final pass
4. **UI libraries** - Dear ImGui, Nuklear work better with 2D contexts
5. **Performance** - UI rarely changes, can be cached separately from 3D scene

**Epic 2 responsibility:**
- Provide render target for 3D scene
- Expose screen-space overlay hooks (for selection rectangles, tooltips)
- Leave actual UI rendering to Epic 12

**Long-term option:** If UI needs 3D effects (floating labels, world-space indicators), we can add specific world-space UI rendering later.

---

### GE-Q4: Coordinate Mapping Confirmation

**Decision: Confirmed. grid_x -> world X, grid_y -> world Z, elevation -> world Y.**

**Rationale:**

This mapping aligns with standard 3D conventions and the isometric camera setup:

```
Grid Space          World Space (3D)
-----------         ----------------
grid_x (east)   ->  X axis (right)
grid_y (south)  ->  Z axis (into screen)
elevation (up)  ->  Y axis (up)
```

From canon `patterns.yaml`:
```yaml
coordinate_system:
  x_axis: "East (right)"
  y_axis: "South (down)"
  z_axis: "Elevation (up) - actual 3D height"
```

The conversion function:
```cpp
glm::vec3 gridToWorld(const PositionComponent& pos) {
    return glm::vec3(
        static_cast<float>(pos.grid_x),  // X = grid_x
        static_cast<float>(pos.elevation) * HEIGHT_SCALE,  // Y = elevation
        static_cast<float>(pos.grid_y)   // Z = grid_y
    );
}
```

Where `HEIGHT_SCALE` converts elevation levels (0-31) to visual height. Suggest 0.25 units per level, so max elevation is 8 units high.

**Note:** The isometric camera looks down the Y axis at an angle, making grid_y appear as "depth" on screen.

---

### GE-Q5: Multiplayer Cursor Representation

**Decision: 3D world point with real-time ray-cast.**

**Rationale:**

For multiplayer, other players' cursors need to:
1. Show where they're pointing in the shared world
2. Update frequently (input sync)
3. Render correctly with the 3D scene (behind buildings, on terrain)

**Architecture:**

```cpp
struct CursorComponent {
    PlayerID owner;              // Which player
    glm::vec3 world_position;    // 3D world point (on terrain)
    GridPosition grid_position;  // Snapped grid cell
    bool is_active = true;       // Player currently engaged
};
```

**Flow:**
1. Local client: Mouse screen position -> ray-cast -> hit point on terrain -> CursorComponent
2. Network: Sync CursorComponent world_position to server and other clients
3. Remote clients: Receive world_position, render cursor at that 3D location

**Why 3D world point, not screen-space:**
- Screen-space coordinates are camera-dependent (different zoom/pan per player)
- World point is the shared reality - render it in 3D space
- Allows cursor to appear ON the terrain, behind buildings, etc.

**Sync frequency:** Cursor position changes rapidly. Use unreliable (UDP) messaging, ~10-20 Hz update rate. Not every tick - cursor is visual feedback only.

**Cursor rendering:** Simple 3D billboard or ground-projected circle with player color. Lives in RenderingSystem's "overlay" pass.

---

## Answers to Game Designer Questions

### GD-Q1: Shader Parameter Configuration

**Decision: Configurable at runtime via ShaderConfig component/resource, but with sensible defaults baked in.**

**Rationale:**

The shader needs flexibility for:
- Tuning during development (art direction iteration)
- Potential per-biome or per-era visual changes
- Day/night cycle color shifts
- Accessibility options (edge width for visibility)

**Interface design:**

```cpp
// Global resource (not per-entity)
struct ToonShaderConfig {
    // Lighting bands
    uint32_t light_bands = 3;           // 2-5 bands typical
    float band_thresholds[4] = {0.1, 0.4, 0.7, 0.95};  // Dot product cutoffs

    // Colors (can shift with day/night)
    glm::vec3 shadow_color = {0.2, 0.1, 0.3};    // Alien purple shadows
    glm::vec3 highlight_tint = {1.0, 0.95, 0.8}; // Warm alien sun

    // Outline
    float edge_width = 2.0f;            // Pixels
    glm::vec3 edge_color = {0.1, 0.05, 0.15};

    // Ambient
    float ambient_strength = 0.3f;
};
```

**System boundary:**
- `RenderingSystem` owns shader parameter updates to GPU
- Game systems (day/night, weather, etc.) modify `ToonShaderConfig` resource
- Config is NOT synced over network - it's client-side visual preference / game state driven

**Storage:** This is a singleton resource, not a component on entities. Access via registry as a resource.

---

### GD-Q2: Underground View Mechanism

**Decision: Camera position change + layer-based culling.**

**Rationale:**

Underground view (for subway/subterra in Epic 7) needs to show subsurface infrastructure while hiding surface obstructions.

**State machine:**

```
enum class ViewMode {
    Surface,      // Normal view - render all, cull underground
    Underground,  // Below surface - render underground, cull surface buildings
    Cutaway       // X-ray - render both with transparency on surface (future)
};
```

**Implementation approach:**

1. **Layer system:** Each renderable has a `RenderLayer` flag:
   - `TERRAIN` - Always visible
   - `SURFACE` - Buildings, surface infrastructure
   - `UNDERGROUND` - Pipes, tunnels, subway
   - `UI_OVERLAY` - Selection, cursors

2. **View mode controls culling:**
   - Surface: Render TERRAIN + SURFACE, cull UNDERGROUND
   - Underground: Render TERRAIN + UNDERGROUND, cull SURFACE (or render SURFACE as wireframe/transparent)

3. **Camera adjustment:** In Underground mode, camera may move down slightly to better frame subterranean structures.

**Data flow:**
- InputSystem: Player presses underground toggle
- CameraSystem: Updates `ViewMode` state
- RenderingSystem: Queries `ViewMode`, adjusts layer culling mask

**This is NOT multiplayer synced** - each player can have their own view mode.

---

### GD-Q3: LOD Management Approach

**Decision: RenderingSystem policy based on camera distance, with optional per-model LOD hints.**

**Rationale:**

LOD (Level of Detail) is fundamentally a rendering optimization, not game logic.

**Architecture:**

1. **Model assets include LOD meshes:** Each glTF model can contain multiple LOD levels (LOD0 = high, LOD1 = medium, LOD2 = low)

2. **RenderingSystem decides which LOD:** Based on distance from camera to entity's world position

3. **Global distance thresholds:**
   ```cpp
   struct LODConfig {
       float lod0_max_distance = 20.0f;  // High detail
       float lod1_max_distance = 50.0f;  // Medium detail
       // Beyond LOD1 distance: LOD2 or billboard
   };
   ```

4. **No LODComponent needed:** LOD selection is derived from:
   - Entity's TransformComponent (world position)
   - Camera position
   - Model's available LOD meshes

**Why no per-entity component:**
- LOD is view-dependent - same entity renders at different LODs for different camera positions
- Component would need updating every frame based on camera - wasteful
- Keep LOD as an implementation detail of RenderingSystem

**Exception:** If specific entities need forced LOD (always high detail for "hero" building), add an optional `RenderHintComponent` with `force_lod_level` field.

---

### GD-Q4: Animation System Boundary

**Decision: AnimationSystem for skeletal/transform animation. Shaders for visual effects.**

**Rationale:**

There are two distinct types of "animation":

1. **Transform/Skeletal Animation** (AnimationSystem):
   - Model bones moving (beings walking, cranes swinging)
   - Object transforms (buildings rising during construction)
   - Requires: Animation data, timing, bone matrices
   - Lives in: `AnimationSystem` (new system, Epic 2 or later)

2. **Visual Effects** (Shader-driven):
   - Scrolling UV for conveyor belts, water
   - Pulsing emission for power conduits
   - Outline/rim light flicker
   - Color shifts
   - Requires: Shader uniforms, time value
   - Lives in: `RenderingSystem` shader pipeline

**Boundary:**
- If it changes the **geometry or bone poses** -> AnimationSystem
- If it changes **appearance without geometry change** -> Shader effect

**For Epic 2 specifically:**
- Defer skeletal animation to later epic (beings, vehicles)
- Epic 2 provides: Time uniform to shaders for procedural effects
- Construction animation (building "rising"): Simple Y-offset animation, can be shader-driven (world matrix manipulation) or AnimationSystem

**Data structure for animated entities:**
```cpp
struct AnimationComponent {
    AnimationHandle current_animation;
    float playback_time = 0.0f;
    float playback_speed = 1.0f;
    bool looping = false;
};
```

This lives on entities that have skeletal animations. AnimationSystem updates bone matrices each frame, RenderingSystem uploads them to GPU.

---

### GD-Q5: Camera Parameter Configurability

**Decision: Configurable-but-locked, with rotation as a future unlock.**

**Rationale:**

Canon says "fixed isometric-style camera" but also mentions rotation could be added later. This suggests:

**Current state (Epic 2):**
```cpp
struct CameraConfig {
    // Fixed parameters
    float pitch_angle = 35.264f;  // Degrees from horizontal (true isometric)
    float rotation = 0.0f;        // Locked to 0 (facing south initially)

    // Variable parameters
    float zoom_level = 1.0f;      // Distance multiplier
    glm::vec2 focus_point;        // Grid position camera centers on

    // Constraints
    float min_zoom = 0.5f;
    float max_zoom = 4.0f;
};
```

**Key decisions:**
1. **Pitch angle:** Constant, but defined as a configurable value (not hardcoded magic number). If we want to tune isometric feel, change in one place.

2. **Rotation:** Stored as a value (initially 0), but no UI to change it. When rotation feature is added, enable 90-degree snap controls without architectural changes.

3. **Why this matters for modeling:** Buildings should have all 4 sides textured regardless. Even without rotation, camera angle means players see 2 sides of buildings. And rotation WILL come eventually.

**Future rotation feature conditions:**
- Player demand / usability testing shows it's needed
- Performance budget allows (changing view angle invalidates visibility culling)
- All models have 4-sided textures (enforce this from start)

**Implementation:** CameraSystem reads CameraConfig resource. Rotation changes require re-sorting render order (depth) so they should snap instantly, not animate.

---

## Summary

These answers establish clear system boundaries for Epic 2:

| Concern | Owner | Canon Reference |
|---------|-------|-----------------|
| PositionComponent | Game logic (all systems) | patterns.yaml |
| TransformComponent | RenderingSystem | New for Epic 2 |
| glTF loading | AssetManager (Epic 0) | systems.yaml |
| UI rendering | UISystem (Epic 12) | systems.yaml |
| Shader config | RenderingSystem | New for Epic 2 |
| LOD selection | RenderingSystem | Rendering concern |
| Animation | AnimationSystem | New system |
| Camera state | CameraSystem | systems.yaml |
| Multiplayer cursor | Network + Rendering | patterns.yaml |

All decisions maintain the ECS pattern (components are data, systems are logic) and respect the multiplayer architecture (server-authoritative game state, client-side rendering).
