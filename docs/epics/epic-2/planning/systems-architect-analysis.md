# Systems Architect Analysis: Epic 2 (3D Re-Plan)

**Epic:** Core Rendering Engine (3D)
**Analyst:** Systems Architect
**Date:** 2026-01-27
**Canon Version Referenced:** 0.4.0
**Decision Reference:** /plans/decisions/3d-rendering-architecture.md

---

## Overview

Epic 2 establishes the 3D visual foundation for ZergCity. This is a **RE-PLAN** based on the decision to switch from 2D sprite rendering to 3D rendering with an isometric-style fixed camera. The decision record (`3d-rendering-architecture.md`) outlines the rationale:

- GPU depth buffer handles sorting automatically (major simplification)
- Zoom is trivial (camera distance, no resolution issues)
- Single 3D model per building instead of multiple sprite angles
- Cell-shading achieved via toon shaders, not pre-rendered
- Future flexibility for camera rotation if desired

This epic introduces two ECS systems:

1. **RenderingSystem** - SDL_GPU/3D renderer management, 3D model rendering, toon/cell shader pipeline, depth buffer management, render layers
2. **CameraSystem** - 3D camera position, fixed isometric angle (35.264 pitch, 45 yaw), zoom via camera distance, view/projection matrices

### Key 3D Specifications (from Canon 0.4.0)

| Specification | Value |
|---------------|-------|
| Graphics API | SDL_GPU (SDL3's 3D rendering API) |
| Shader Style | Toon/cell shading (custom shader) |
| Model Format | glTF (loaded via Epic 0 AssetManager) |
| Depth Sorting | GPU depth buffer (automatic) |
| Camera Angle | Fixed isometric: 35.264 pitch, 45 yaw |
| Camera Rotation | None initially (can add later) |
| Grid Unit | 1x1 in 3D world space |
| Elevation | Actual 3D Z-height (32 levels) |

---

## Key Work Items

### RenderingSystem (ECS System)

- [ ] **RENDER-01: SDL_GPU Initialization**
  - Create SDL_GPUDevice from Application's window (Epic 0)
  - Configure swap chain and present mode
  - Handle device lost/recreated scenarios
  - Select appropriate shader format for platform

- [ ] **RENDER-02: 3D Render Pipeline Setup**
  - Create graphics pipeline with vertex/fragment shaders
  - Configure depth buffer (enable depth test, depth write)
  - Set up blend states for transparency
  - Configure rasterizer state (cull mode, fill mode)

- [ ] **RENDER-03: Toon/Cell Shader Implementation**
  - Vertex shader: standard MVP transformation
  - Fragment shader: discrete lighting bands (toon shading)
  - Outline pass (optional): edge detection or inverted-hull technique
  - Support for albedo texture, toon ramp texture
  - Configurable band count and thresholds

- [ ] **RENDER-04: 3D Model Rendering Core**
  - Load glTF models via AssetManager (Epic 0)
  - Create GPU vertex/index buffers from model data
  - Support for textured models (diffuse/albedo maps)
  - Model instancing for repeated objects (buildings, terrain tiles)
  - Missing models render placeholder cube (magenta)

- [ ] **RENDER-05: Render Layer Architecture (3D)**
  - Define semantic render layers: Terrain, Buildings, Beings, Effects, Overlays, UI
  - Layers rendered in order (terrain first)
  - Layer visibility toggling for debug/underground view
  - Overlay layers use separate render pass (additive/alpha blend)

- [ ] **RENDER-06: Model Asset Binding**
  - RenderComponent references model asset handle
  - Support LOD switching based on camera distance (future)
  - Texture atlas support for building variations
  - Animation skeleton binding (if animated models needed)

- [ ] **RENDER-07: Depth Buffer Management**
  - GPU depth buffer handles object occlusion automatically
  - Configure depth format (D24 or D32)
  - Clear depth buffer at frame start
  - No manual depth sorting needed (major simplification from 2D)

- [ ] **RENDER-08: Frustum Culling (3D)**
  - Extract frustum planes from view-projection matrix
  - Cull objects with bounding sphere/box outside frustum
  - Early-out for entities without renderable components
  - Spatial partitioning for large maps (octree or grid)

- [ ] **RENDER-09: View/Projection Matrix Integration**
  - Receive view matrix from CameraSystem
  - Receive projection matrix from CameraSystem
  - Upload combined MVP matrix to shader as uniform
  - Handle aspect ratio changes on window resize

- [ ] **RENDER-10: Debug Rendering (3D)**
  - Grid overlay on terrain plane
  - Bounding box visualization
  - Coordinate axes at origin
  - Wireframe mode toggle
  - Render statistics (FPS, draw calls, triangles)

### CameraSystem (ECS System)

- [ ] **CAMERA-01: 3D Camera State**
  - Camera position in 3D world space (float x, y, z)
  - Fixed pitch angle: 35.264 degrees (arctan(1/sqrt(2)))
  - Fixed yaw angle: 45 degrees
  - Zoom level controls camera distance from focus point
  - No rotation initially (pitch/yaw locked)

- [ ] **CAMERA-02: View Matrix Calculation**
  - Calculate view matrix from camera position and orientation
  - Camera looks at a "focus point" on the ground plane
  - Focus point moves with pan controls
  - Camera position derived from focus point + distance + angles

- [ ] **CAMERA-03: Projection Matrix Calculation**
  - Orthographic projection for true isometric feel (or)
  - Perspective projection with narrow FOV for pseudo-isometric
  - Handle aspect ratio from window dimensions
  - Near/far plane configuration

- [ ] **CAMERA-04: Zoom Controls (Camera Distance)**
  - Mouse wheel adjusts camera distance from focus point
  - Zoom range: 5 units (close) to 100 units (far) from focus
  - Zoom centered on cursor position (adjust focus point)
  - Smooth zoom interpolation
  - Zoom limits with soft boundaries

- [ ] **CAMERA-05: Pan Controls (Focus Point Movement)**
  - Keyboard panning (WASD/arrows) moves focus point
  - Mouse drag panning (middle button)
  - Edge-of-screen panning (optional, toggleable)
  - Pan speed scales with zoom level
  - Smooth camera movement with momentum

- [ ] **CAMERA-06: Viewport Bounds (World Space)**
  - Calculate visible world rectangle from frustum
  - Clamp focus point to map boundaries
  - Provide visible tile range for culling queries
  - Handle zoom affecting visible area

- [ ] **CAMERA-07: Camera Animation**
  - Smooth transition to target position ("go to" feature)
  - Camera shake for disasters (Epic 13)
  - Easing functions (linear, ease-in-out)
  - Interrupt animation on player input

### Coordinate Conversion (3D)

- [ ] **COORD-01: World-to-Screen Transformation**
  - Apply model matrix (entity position/rotation)
  - Apply view matrix (camera)
  - Apply projection matrix
  - Perspective divide and viewport transform
  - Result: screen pixel coordinates

- [ ] **COORD-02: Screen-to-World Ray Casting**
  - Unproject screen coordinates to world ray
  - Intersect ray with ground plane (y=0 or terrain height)
  - Handle elevation: intersect with terrain heightmap
  - Return world position and hit information

- [ ] **COORD-03: Tile Picking (3D Ray Cast)**
  - Cast ray from camera through cursor screen position
  - Intersect with terrain mesh first
  - Then check building bounding boxes along ray
  - Return closest hit entity (depth tested)

### RenderComponent Definition (3D)

- [ ] **RCOMP-01: RenderComponent Structure**
  - Model asset reference (AssetHandle<Model>)
  - Material/texture reference
  - Render layer assignment
  - Visibility flag
  - Tint/color modulation
  - Scale factor (for variety)

- [ ] **RCOMP-02: TransformComponent**
  - Position (float x, y, z in world space)
  - Rotation (quaternion or euler angles)
  - Scale (vec3)
  - Derived model matrix (cached)

---

## Questions for Other Agents

### @graphics-engineer

- **Q1:** For the toon shader, what technique should we use? Options:
  - A) Discrete lighting bands (dot product thresholds)
  - B) Toon ramp texture lookup
  - C) Combination with edge detection
  What provides the best balance of alien aesthetic and performance?

- **Q2:** Should we use orthographic or perspective projection?
  - Orthographic: True isometric, no foreshortening, classic feel
  - Perspective (narrow FOV): Slight depth perception, more modern
  The decision affects how "flat" vs "deep" the world feels.

- **Q3:** For SDL_GPU shader compilation, what format should we target?
  - SPIR-V for Vulkan backend?
  - HLSL for D3D12 backend?
  - Multiple backends with shader cross-compilation?
  SDL_GPU abstracts this but we need to understand the workflow.

- **Q4:** Model instancing strategy for large tile counts. Should we:
  - Use GPU instancing (one draw call, instance buffer)
  - Batch geometry into mega-meshes per chunk
  - Standard draw calls with frustum culling
  What's the expected draw call budget?

- **Q5:** For the outline/silhouette effect in toon shading, which technique?
  - Inverted hull (scale + backface render)
  - Screen-space edge detection (post-process)
  - Geometry shader edge extrusion
  Each has tradeoffs in quality vs performance.

### @game-designer

- **Q6:** With true 3D models, should buildings have idle animations?
  - Animated smoke from power plants
  - Glowing energy effects on conduits
  - Activity indicators (beings moving inside?)
  This affects model complexity and rendering budget.

- **Q7:** For the isometric camera angle, classic games used exact ratios:
  - SimCity 2000: 2:1 diamond (26.57 degrees)
  - Standard isometric: 30 degrees
  - True isometric: 35.264 degrees
  Which angle feels most appropriate for ZergCity's alien aesthetic?

- **Q8:** Camera rotation - the decision says "none initially, can add later." Under what conditions would we add rotation? This affects how we model buildings (all 4 sides textured, or just front-facing?).

- **Q9:** Day/night cycle - does it affect the toon shader? Should lighting direction change, or just color temperature/palette shift?

- **Q10:** For zoom levels, what's the minimum detail the player should see:
  - At max zoom in: Individual being faces? Texture details?
  - At max zoom out: Entire colony (128x128 tiles)? Multiple colonies?
  This determines LOD requirements.

---

## Risks & Concerns

### Risk 1: SDL_GPU Maturity

**Severity:** Medium-High
**Description:** SDL_GPU is relatively new in SDL3. Documentation may be incomplete, and edge cases may not be well-handled. Vulkan/D3D12 backend differences could cause unexpected issues.
**Mitigation:**
- Start with simple rendering and incrementally add complexity
- Create abstraction layer over SDL_GPU for future backend changes
- Test on multiple GPU vendors early (NVIDIA, AMD, Intel)
- Have fallback plan to raw Vulkan/OpenGL if SDL_GPU proves problematic

### Risk 2: Toon Shader Complexity

**Severity:** Medium
**Description:** Achieving the right "cell-shaded alien" look requires shader iteration. Initial implementations may look wrong (too cartoony, too flat, wrong lighting). This is art-dependent and may require multiple iterations.
**Mitigation:**
- Create shader prototypes early with test models
- Get Game Designer feedback on visual direction before full implementation
- Support shader hot-reloading for rapid iteration
- Plan for multiple shader variations (day/night, contamination effects)

### Risk 3: 3D Model Pipeline

**Severity:** Medium
**Description:** Switching to 3D models requires a new asset pipeline. glTF support must be robust. Model creation/export workflows need to be established. Artists need different tools and skills.
**Mitigation:**
- Epic 0 (Ticket 0-034) establishes glTF loading via cgltf
- Document model requirements: poly budget, UV mapping, texture resolution
- Create test models early to validate pipeline
- Consider placeholder programmer-art models for initial development

### Risk 4: Performance with 3D Rendering

**Severity:** Medium
**Description:** 3D rendering has different performance characteristics than 2D sprites. Triangle count, draw calls, shader complexity all matter. Performance on integrated GPUs may be challenging.
**Mitigation:**
- Target minimum spec: Intel UHD 620 or equivalent
- Budget: 100k triangles visible, <200 draw calls, 60 FPS
- Implement frustum culling aggressively
- Use instancing for repeated geometry (terrain, common buildings)
- LOD system for distant objects (lower poly models)

### Risk 5: Camera Math Complexity

**Severity:** Low-Medium
**Description:** 3D camera math (view/projection matrices, ray casting) is more complex than 2D camera offset. Bugs here cause subtle or dramatic visual issues.
**Mitigation:**
- Use established math libraries (GLM or custom verified implementation)
- Unit tests for matrix operations and ray casting
- Debug visualization (frustum, ray, axes)
- Reference implementations from known-working sources

### Risk 6: Isometric vs True 3D Expectations

**Severity:** Low
**Description:** Players expect "isometric" games to behave a certain way (fixed angles, no rotation). Full 3D capability might tempt scope creep (free camera, etc.). Need to stay disciplined.
**Mitigation:**
- Lock camera rotation to fixed isometric angle initially
- Design buildings with front-facing detail (don't need all 4 sides)
- Document that rotation is "future feature, not MVP"
- Resist feature creep during implementation

### Risk 7: Interpolation in 3D

**Severity:** Medium
**Description:** Interpolating 3D transforms (position + rotation + scale) between simulation ticks is more complex than 2D position lerp. Rotation interpolation requires quaternion slerp.
**Mitigation:**
- Use quaternions for rotation (avoids gimbal lock)
- Implement proper slerp for rotation interpolation
- Position lerp is straightforward (same as 2D)
- Scale interpolation rarely needed (buildings don't resize often)

---

## Dependencies

### Epic 2 Depends On

- **Epic 0 (Foundation)** - Hard dependency
  - Application: Window handle, SDL_GPU device creation
  - AssetManager: 3D model loading (glTF via cgltf - Ticket 0-034)
  - InputSystem: Camera control input
  - ECS Framework: Component queries for rendering
  - PositionComponent: Entity world positions

- **Epic 1 (Multiplayer)** - Soft dependency
  - SyncSystem: State updates to render (can stub for local testing)
  - Without Epic 1, rendering works in local/single-player mode
  - Interpolation relies on sync tick timing

### Epic 2 Blocks

- **Epic 3 (Terrain)** - Hard dependency
  - Terrain meshes need RenderingSystem to display
  - Terrain uses coordinate conversion functions
  - Terrain heightmap affects ray casting

- **Epic 4+ (All Visual Systems)** - Hard dependency
  - Everything visible depends on Epic 2
  - Zoning overlays, buildings, infrastructure, UI

- **Epic 12 (UI)** - Shares rendering resources
  - UI renders on top layer
  - May use same SDL_GPU device

### Internal Dependencies (Within Epic 2)

```
RENDER-01 (SDL_GPU Init) ──► RENDER-02 (Pipeline) ──► RENDER-03 (Shaders)
       │                          │                        │
       ▼                          ▼                        ▼
RENDER-07 (Depth Buffer) ──► RENDER-04 (Model Render) ◄── RENDER-06 (Assets)
                                   │
                                   ▼
                            RENDER-05 (Layers)
                                   │
                                   ▼
                            RENDER-08 (Culling)

CAMERA-01 (State) ──► CAMERA-02 (View Matrix) ──► RENDER-09 (MVP Uniforms)
       │                       │
       │                       ▼
       │              CAMERA-03 (Projection)
       │                       │
       ▼                       ▼
CAMERA-04 (Zoom) ───► CAMERA-06 (Bounds)
       │
       ▼
CAMERA-05 (Pan)

COORD-02 (Ray Cast) ◄── CAMERA-02/03 (Matrices)
       │
       ▼
COORD-03 (Tile Picking)
```

Critical Path:
1. RENDER-01 (SDL_GPU Device) - foundation
2. RENDER-02 (Pipeline Setup) - before any rendering
3. RENDER-03 (Toon Shader) - visual identity
4. RENDER-04 (Model Rendering) - first 3D output
5. CAMERA-01, 02, 03 (Camera basics) - view the scene
6. RENDER-07 (Depth Buffer) - correct occlusion
7. RENDER-09 (MVP Integration) - camera-aware rendering
8. CAMERA-04, 05 (Controls) - navigate the world
9. RENDER-08 (Culling) - performance
10. COORD-03 (Picking) - interaction

---

## System Interaction Map

```
+--------------------------------------------------------------------------------+
|                           CLIENT PROCESS                                        |
+--------------------------------------------------------------------------------+
|                                                                                 |
|  +---------------+     +----------------+     +---------------------------+    |
|  |  Application  |---->|  SDL_Window    |---->|     SDL_GPUDevice         |    |
|  |   (Epic 0)    |     |    Handle      |     |  (Owned by RenderingSystem)|    |
|  +---------------+     +----------------+     +-------------+-------------+    |
|         |                                                   |                   |
|         v                                                   v                   |
|  +---------------+                                +-----------------------+     |
|  |  InputSystem  |-------------------------------->|    CameraSystem       |     |
|  |   (Epic 0)    |   keyboard/mouse events        | (position, matrices)  |     |
|  +---------------+                                +----------+------------+     |
|                                                              |                  |
|                                                              | view/proj        |
|                                                              | matrices         |
|                                                              v                  |
|  +---------------+     +----------------+     +--------------------------+     |
|  |  SyncSystem   |---->|  ECS Registry  |---->|    RenderingSystem       |     |
|  |   (Epic 1)    |     | (Components)   |     | (GPU, shaders, models)   |     |
|  +---------------+     +----------------+     +----------+---------------+     |
|         |                     ^                          |                     |
|   state |                     |                          | GPU commands        |
| updates |                     |                          v                     |
|         |              +---------------+         +-------------------+         |
|         |              | AssetManager  |<--------|   Screen Output   |         |
|         +------------->|   (Epic 0)    |         | (swap chain)      |         |
|                        | (3D models)   |         +-------------------+         |
|                        +---------------+                                       |
|                                                                                 |
+--------------------------------------------------------------------------------+
```

### Data Flow Per Frame

**Camera Update (runs first):**
1. InputSystem provides current input state
2. CameraSystem reads input (pan keys, zoom wheel, mouse position)
3. CameraSystem updates focus point and camera distance
4. CameraSystem calculates view matrix from position/orientation
5. CameraSystem calculates projection matrix from FOV/aspect
6. CameraSystem calculates visible world bounds for culling

**Render (runs after camera):**
1. RenderingSystem begins frame (acquire swap chain image)
2. RenderingSystem clears color and depth buffers
3. RenderingSystem gets view/projection matrices from CameraSystem
4. RenderingSystem queries ECS for entities with RenderComponent
5. Frustum cull: skip entities outside camera frustum
6. For each visible entity:
   a. Get model asset from AssetManager
   b. Calculate model matrix from TransformComponent
   c. Upload MVP matrix uniform
   d. Bind model's vertex/index buffers
   e. Bind material textures
   f. Draw indexed primitives
7. GPU depth buffer handles occlusion automatically
8. Render UI overlay layer
9. Present frame (swap buffers)

**Interpolation (for multiplayer):**
1. SyncSystem stores previous_tick_state and current_tick_state
2. RenderingSystem calculates interpolation factor: t = time_since_tick / tick_duration
3. For interpolatable transforms (moving entities):
   - Position: lerp(prev_pos, curr_pos, t)
   - Rotation: slerp(prev_rot, curr_rot, t)
4. Static entities (buildings) use current state directly

---

## 3D Coordinate System Specification

### World Space (3D)

```
Origin: Ground level at map corner (0, 0, 0)
X-axis: East (right) - increases rightward
Y-axis: Up - increases upward (elevation)
Z-axis: South (into screen) - increases downward (toward viewer in iso view)

Note: This is a LEFT-HANDED coordinate system matching typical game conventions
where positive Z goes "into the screen" from the camera's perspective.
```

### Grid to World Conversion

```cpp
// Grid coordinates (tile-based) to world coordinates (3D)
Vec3 grid_to_world(int grid_x, int grid_y, int elevation) {
    return Vec3(
        grid_x * GRID_UNIT_SIZE,        // X
        elevation * ELEVATION_STEP,      // Y (up)
        grid_y * GRID_UNIT_SIZE          // Z
    );
}

// Constants
constexpr float GRID_UNIT_SIZE = 1.0f;   // 1 unit per grid cell
constexpr float ELEVATION_STEP = 0.25f;   // Height per elevation level
constexpr int MAX_ELEVATION = 31;         // 32 levels (0-31)
```

### Camera Configuration (Isometric)

```cpp
// Isometric camera angles
constexpr float CAMERA_PITCH = 35.264f;  // arctan(1/sqrt(2)) degrees - true isometric
constexpr float CAMERA_YAW = 45.0f;      // 45 degrees from X axis

// Camera calculates position from focus point
Vec3 calculate_camera_position(Vec3 focus_point, float distance) {
    // Convert angles to radians
    float pitch_rad = radians(CAMERA_PITCH);
    float yaw_rad = radians(CAMERA_YAW);

    // Offset from focus point
    Vec3 offset;
    offset.x = distance * cos(pitch_rad) * cos(yaw_rad);
    offset.y = distance * sin(pitch_rad);
    offset.z = distance * cos(pitch_rad) * sin(yaw_rad);

    return focus_point + offset;
}
```

### View Matrix

```cpp
Mat4 calculate_view_matrix(Vec3 camera_pos, Vec3 focus_point) {
    Vec3 up = Vec3(0, 1, 0);
    return lookAt(camera_pos, focus_point, up);
}
```

### Projection Matrix (Orthographic for True Isometric)

```cpp
Mat4 calculate_projection_matrix(float zoom, float aspect_ratio) {
    // Orthographic projection for true isometric
    float half_width = 10.0f * zoom;
    float half_height = half_width / aspect_ratio;

    return ortho(
        -half_width, half_width,
        -half_height, half_height,
        0.1f, 1000.0f  // near/far planes
    );
}
```

### Screen-to-World Ray Casting

```cpp
Ray screen_to_world_ray(int screen_x, int screen_y, Mat4 inv_view_proj, Vec2 screen_size) {
    // Normalize screen coordinates to [-1, 1]
    float nx = (2.0f * screen_x / screen_size.x) - 1.0f;
    float ny = 1.0f - (2.0f * screen_y / screen_size.y);

    // Unproject near and far points
    Vec4 near_point = inv_view_proj * Vec4(nx, ny, 0.0f, 1.0f);
    Vec4 far_point = inv_view_proj * Vec4(nx, ny, 1.0f, 1.0f);

    // Perspective divide
    near_point /= near_point.w;
    far_point /= far_point.w;

    // Create ray
    Ray ray;
    ray.origin = Vec3(near_point);
    ray.direction = normalize(Vec3(far_point) - Vec3(near_point));
    return ray;
}

// Intersect ray with ground plane (Y = height)
Vec3 ray_ground_intersection(Ray ray, float ground_height) {
    float t = (ground_height - ray.origin.y) / ray.direction.y;
    return ray.origin + ray.direction * t;
}
```

---

## Multiplayer Considerations

### Client-Authoritative Systems (No Sync)

Per canon, these are client-authoritative:
- Camera position, zoom, focus point
- Audio/visual effects
- UI state
- Render interpolation state

Each player has their own camera view. No synchronization needed.

### Rendering Synced State

RenderingSystem displays state received from server via SyncSystem:
- PositionComponent / TransformComponent: Where entities are in 3D
- RenderComponent: What model to render
- OwnershipComponent: Who owns what (for faction coloring)
- BuildingComponent: Building type (determines which model)

### Interpolation for Smooth Visuals

The 20 Hz simulation tick rate (50ms) would look choppy at 60fps rendering:

**What to interpolate:**
- Entity positions (for moving entities - beings, vehicles)
- Entity rotations (if rotating)
- Construction progress effects

**What NOT to interpolate:**
- Discrete state changes (zone type, ownership, building type)
- Entity creation/destruction (instant or fade effect)

**Implementation:**
- TransformComponent stores previous and current state
- RenderingSystem calculates t = time_since_tick / tick_duration
- Position: lerp(prev, curr, t)
- Rotation: slerp(prev, curr, t)

### Ghost Town Visual Effects

Per canon, abandoned colonies decay through stages:
1. **Abandoned:** Buildings stop functioning, visual decay begins
2. **GhostTown:** Buildings crumble to ruins
3. **Cleared:** Land returns to natural state

3D rendering supports:
- Material tinting (desaturate, darken via shader uniform)
- Damage state models (alternate LODs or morphs)
- Particle effects for crumbling
- Vegetation overgrowth models

---

## Performance Considerations

### Target Metrics

| Metric | Target | Notes |
|--------|--------|-------|
| Frame Rate | 60 FPS | Minimum on target hardware |
| Frame Time | <16ms | Allows headroom for spikes |
| Draw Calls | <200 | Per frame, with instancing |
| Triangles | <100k | Visible, after culling |
| GPU Memory | <512MB | Textures + buffers |
| Min Spec | Intel UHD 620 | Integrated graphics baseline |

### Optimization Strategies

1. **Frustum Culling:** Skip all objects outside camera view
2. **GPU Instancing:** One draw call for many identical models (terrain tiles)
3. **Level of Detail:** Lower poly models for distant objects
4. **Texture Atlasing:** Reduce texture bind changes
5. **Spatial Partitioning:** Octree or uniform grid for fast culling
6. **State Sorting:** Minimize shader/texture/buffer changes
7. **Early-Z:** Render opaque front-to-back for GPU early-Z rejection

### Memory Considerations

- Model assets loaded via AssetManager (shared, reference-counted)
- GPU buffers created once per unique model (not per instance)
- Instance data in separate buffer (transforms, colors)
- Texture streaming for very large maps (future)

---

## Architectural Recommendations

### 1. Renderer Abstraction Layer

Create a thin abstraction over SDL_GPU:
```cpp
class Renderer {
public:
    void begin_frame();
    void end_frame();

    void set_pipeline(const Pipeline& pipeline);
    void set_vertex_buffer(const Buffer& buffer);
    void set_uniform(const char* name, const void* data, size_t size);
    void draw_indexed(uint32_t index_count);

private:
    SDL_GPUDevice* device;
    SDL_GPUCommandBuffer* cmd_buffer;
};
```

This insulates game code from SDL_GPU specifics and enables future backend changes.

### 2. Render Queue Pattern

Instead of immediate-mode rendering:
```cpp
struct RenderCommand {
    Model* model;
    Mat4 transform;
    Material* material;
    float depth;  // For sorting
};

class RenderQueue {
    void submit(const RenderCommand& cmd);
    void sort();  // By shader, then texture, then depth
    void flush(Renderer& renderer);
};
```

Benefits:
- Decouple "what to render" from "how to render"
- Enable state sorting for performance
- Enable multi-threaded command generation (future)

### 3. Material System

Simple material system for toon shading:
```cpp
struct Material {
    Texture* albedo;
    Texture* toon_ramp;  // Optional: custom lighting gradient
    Vec4 tint_color;
    float outline_thickness;
    uint32_t shader_flags;  // Variations: emissive, transparent, etc.
};
```

### 4. Transform Component Separation

Keep RenderComponent focused on visual data:
```cpp
struct RenderComponent {
    AssetHandle<Model> model;
    AssetHandle<Material> material;
    RenderLayer layer;
    bool visible;
    Vec4 tint;
};

struct TransformComponent {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    Mat4 cached_matrix;  // Updated when dirty
    bool dirty;
};
```

PositionComponent (grid-based) is for gameplay; TransformComponent (float) is for rendering. Systems translate between them.

### 5. Camera as Singleton Manager

For MVP, use a single camera:
```cpp
class CameraSystem {
public:
    void update(float dt, const InputState& input);

    const Mat4& get_view_matrix() const;
    const Mat4& get_projection_matrix() const;
    Frustum get_frustum() const;
    GridRect get_visible_tiles() const;

    void animate_to(Vec3 target, float duration);
    void shake(float intensity, float duration);

private:
    Vec3 focus_point;
    float distance;
    // Fixed: pitch = 35.264, yaw = 45
};
```

Design supports multiple cameras later (minimap, observer mode).

### 6. Shader Management

Centralized shader compilation and caching:
```cpp
class ShaderManager {
public:
    Shader* get_shader(const char* vertex_path, const char* fragment_path);
    void reload_all();  // For hot-reloading during development

private:
    std::map<std::pair<std::string, std::string>, Shader> cache;
};
```

---

## Open Questions Requiring Decisions

1. **Orthographic vs Perspective:** Which projection gives the right isometric feel? Affects visual depth perception.

2. **Outline Technique:** Inverted hull vs post-process edge detection? Affects visual quality and performance.

3. **Model LOD Strategy:** How many LOD levels? At what distances do they switch? Affects asset production.

4. **Shader Hot-Reload:** Should we support runtime shader recompilation for development? Affects iteration speed.

5. **Instancing Granularity:** Instance per-model-type or per-material-type? Affects draw call count.

6. **Texture Resolution:** What's the standard texture size for buildings? 256x256? 512x512? Affects memory and visual quality.

7. **Animation System:** Does RenderingSystem handle skeletal animation, or is that a separate AnimationSystem? Affects system boundaries.

---

## Comparison: 2D vs 3D Approach

| Aspect | Previous 2D Plan | New 3D Plan |
|--------|------------------|-------------|
| Depth Sorting | Manual per-entity sorting | GPU depth buffer (automatic) |
| Zoom | Sprite resolution limits | Camera distance (unlimited) |
| Assets per Building | Multiple sprite angles | Single 3D model |
| Cell-Shading | Pre-rendered sprites | Runtime toon shader |
| Coordinate System | 2D dimetric projection | Standard 3D transforms |
| Elevation | 8px offset per level | Actual 3D Y-height |
| Tile Picking | 2D inverse projection | 3D ray casting |
| Camera Controls | 2D offset/scale | 3D position/matrices |
| Renderer | SDL_Renderer (2D) | SDL_GPU (3D) |
| Performance Model | Sprite count, batch count | Triangle count, draw calls |

### Complexity Trade-offs

**Simplified by 3D:**
- No manual depth sorting
- No sprite angle management
- Elevation is natural 3D
- Zoom is trivial

**Added Complexity:**
- 3D math (matrices, quaternions)
- Shader programming
- 3D asset pipeline
- GPU resource management

**Overall:** The 3D approach trades sprite management complexity for 3D math complexity. The GPU handles sorting automatically, which was a major pain point in 2D. The investment in 3D infrastructure pays off in flexibility and future-proofing.

---

## Summary

Epic 2 provides the 3D visual foundation for ZergCity. Key architectural points:

1. **SDL_GPU for 3D rendering** - Modern API, handles depth automatically
2. **Toon/cell shader** creates the alien aesthetic via runtime shading, not pre-rendered assets
3. **Fixed isometric camera** (35.264 pitch, 45 yaw) provides classic city-builder perspective
4. **glTF models** loaded via Epic 0's AssetManager (Ticket 0-034)
5. **GPU depth buffer eliminates manual sorting** - major simplification from 2D approach
6. **View/projection matrices** from CameraSystem drive all coordinate conversions
7. **Ray casting replaces 2D inverse projection** for tile picking
8. **Camera is client-authoritative** - simple design, no sync needed

Critical path: SDL_GPU init -> Pipeline setup -> Toon shader -> Model rendering -> Camera matrices -> Controls -> Culling -> Picking

Epic 2 unblocks all visual development. Once complete, terrain (Epic 3) and beyond can render 3D models using RenderingSystem's pipeline.
