# Systems Architect Impact Analysis: Epic 2

**Epic:** Core Rendering Engine (3D)
**Analyst:** Systems Architect
**Date:** 2026-01-28
**Trigger:** Canon update v0.4.0 -> v0.5.0

---

## Trigger
Canon update v0.4.0 -> v0.5.0 (includes v0.4.1 camera change and v0.5.0 art direction/map/terrain/building/UI changes)

## Overall Impact: HIGH

## Impact Summary

The camera model change from fixed isometric to free camera with isometric presets is a **fundamental architectural change** that invalidates the core CameraSystem design assumptions (fixed angles, locked rotation, single projection mode). Additionally, bioluminescent art direction makes emissive materials and bloom post-processing core requirements rather than optional features, and configurable map sizes (up to 512x512) elevate frustum culling and LOD from nice-to-have to mandatory for large maps. Together these changes affect the majority of Epic 2's camera subsystem (Group E) and have significant knock-on effects on rendering, components, and coordinate conversion.

---

## Item-by-Item Assessment

### UNAFFECTED

- **Item: RENDER-01 / SDL_GPU Initialization (2-001)** -- Reason: GPU device creation is independent of camera model, art direction, and map size. No change needed.

- **Item: RENDER-02 / 3D Render Pipeline Setup (2-002, 2-007, 2-014, 2-015)** -- Reason: Pipeline configuration (depth buffer, blend states, rasterizer state, swap chain) is not affected by camera model or art direction changes. Depth buffer, vertex input layout, and cull mode remain the same.

- **Item: RENDER-04 / 3D Model Rendering Core (2-009, 2-010, 2-011)** -- Reason: glTF loading, GPU mesh creation, and render command recording are model-pipeline concerns unaffected by camera or art direction changes.

- **Item: GPU Instancing (2-012)** -- Reason: Instancing mechanics (instance buffers, per-instance transforms) are unchanged. The number of instances may increase with larger maps, but the instancing architecture itself is the same.

- **Item: Placeholder Model (2-013)** -- Reason: Magenta placeholder cube is unaffected by any of these changes.

- **Item: Depth Buffer Setup and Depth State Configuration (2-014, 2-015)** -- Reason: Depth format, depth test, and depth write configuration are projection-independent.

- **Item: Transparent Object Handling (2-016)** -- Reason: Transparency sorting and depth write rules are unchanged by camera model or art direction.

- **Item: Main Render Pass Structure (2-018)** -- Reason: The overall pass structure (acquire, clear, render layers, present) remains valid. Bloom and edge detection are already accounted for.

- **Item: World-to-Screen Transformation (2-028)** -- Reason: The MVP transformation pipeline is mathematically the same regardless of projection type. The function signature and approach (model * view * projection * viewport) does not change.

- **Item: Position-to-Transform Synchronization (2-033)** -- Reason: Grid-to-world coordinate mapping (grid_x -> X, grid_y -> Z, elevation -> Y) is not affected by camera or art direction changes.

- **Item: Render Layer Definitions (2-034)** -- Reason: Semantic render layers (Terrain, Buildings, Effects, etc.) are unchanged by these canon updates.

- **Item: Layer Visibility Toggle (2-035)** -- Reason: Show/hide/ghost layer mechanism is camera-independent.

- **Item: Underground View Mode (2-036)** -- Reason: ViewMode state machine (Surface, Underground, Cutaway) is unaffected.

- **Item: Wireframe Mode (2-041)** -- Reason: Debug wireframe toggle is unaffected.

- **Item: Render Statistics Display (2-042)** -- Reason: FPS, frame time, draw calls, triangle count display is unaffected.

- **Item: Bounding Box Visualization (2-043)** -- Reason: Debug AABB wireframe visualization is unaffected.

- **Item: Transform Interpolation (2-044)** -- Reason: Lerp/slerp interpolation between ticks is independent of camera model. Rotation interpolation (slerp) was already planned.

- **Item: Other Player Cursor Rendering (2-045)** -- Reason: Cursor rendering at world positions is camera-independent (uses standard MVP pipeline).

- **Item: Risk 1: SDL_GPU Maturity** -- Reason: SDL_GPU maturity concerns are unchanged.

- **Item: Risk 3: 3D Model Pipeline** -- Reason: glTF pipeline risks are unchanged by camera or art direction.

- **Item: Risk 7: Interpolation in 3D** -- Reason: Interpolation complexity is the same regardless of camera model.

- **Item: System Interaction Map (overall data flow)** -- Reason: The high-level flow (InputSystem -> CameraSystem -> RenderingSystem -> Screen) remains the same. CameraSystem still provides matrices to RenderingSystem.

- **Item: Dependencies (Epic 0, Epic 1, Epic 3)** -- Reason: Inter-epic dependency structure is unchanged. Epic 2 still depends on Epic 0 for window/assets/input and still blocks Epic 3 for terrain rendering.

---

### MODIFIED

- **Item: CameraSystem Scope and Responsibilities (CAMERA-01 / 2-017)**
  - Previous: Fixed isometric camera state -- pitch locked at 26.57 degrees (later 35.264 in analysis), yaw locked at 45 degrees. Camera only stores focus point and distance. Rotation explicitly deferred.
  - Revised: CameraSystem must manage two modes: free camera and isometric presets. Free camera state requires: pitch (tilt), yaw (orbit), distance (zoom), focus point (pan). Isometric preset mode provides snap views at 4 cardinal directions (N/E/S/W at 45-degree yaw increments, ~35.264-degree pitch). System must store current mode, allow toggling between modes, and handle smooth transitions between free and preset states.
  - Reason: Canon v0.4.1 fundamentally changed the camera from fixed-angle to full free camera with preset snap views. Camera rotation is now core functionality, not deferred.

- **Item: View Matrix Calculation (CAMERA-02 / 2-020)**
  - Previous: View matrix calculated from fixed pitch/yaw angles. Only focus point and distance vary.
  - Revised: View matrix must be calculated from variable pitch, yaw, focus point, and distance. In free mode, pitch and yaw are user-controlled. In preset mode, pitch and yaw snap to predefined values. The lookAt calculation itself is the same, but the inputs are now dynamic.
  - Reason: Camera orientation is no longer constant; view matrix must respond to orbit/tilt inputs.

- **Item: Projection Matrix Calculation (CAMERA-03 / 2-021)**
  - Previous: Orthographic projection only, decided by consensus for "true isometric feel."
  - Revised: The projection type needs re-evaluation. Options: (A) Keep orthographic for both free and preset modes -- simpler, preserves grid clarity, but free camera with orthographic can feel unnatural at extreme angles. (B) Perspective for free camera, orthographic for presets -- more natural free camera but requires switching projection on mode change. (C) Perspective only -- most natural for free camera, loses "true isometric" parallel lines in preset mode. Recommendation: perspective projection for both modes with a relatively narrow FOV (~30-40 degrees) that approximates isometric feel while supporting free camera naturally. This is a new decision point.
  - Reason: Free camera with full tilt/orbit makes pure orthographic less natural. The consensus decision for orthographic was predicated on fixed isometric angle.

- **Item: Zoom Controls (CAMERA-04 / 2-024)**
  - Previous: Zoom via camera distance only, zoom centered on cursor position.
  - Revised: Zoom controls remain the same conceptually (adjust distance from focus point, center on cursor). However, zoom behavior interacts with projection choice -- in orthographic, zoom adjusts view volume width; in perspective, zoom adjusts camera distance. The zoom limits may need adjustment for larger map sizes (512x512 requires zooming out much further to see significant portions of the map).
  - Reason: Configurable map sizes (128-512) mean zoom range and defaults need to be map-size-aware.

- **Item: Pan Controls (CAMERA-05 / 2-023)**
  - Previous: WASD/arrows and middle mouse drag move focus point along the ground plane.
  - Revised: Pan controls must now account for camera orientation. When the camera is rotated via orbit, "forward" pan direction must follow the camera's viewing direction projected onto the ground plane, not world-axis-aligned. Middle mouse drag panning must also be camera-relative. Canon specifies: pan via WASD/edge scroll/right mouse (not middle mouse as previously planned -- middle mouse is now orbit).
  - Reason: Free camera means pan direction is relative to current view orientation. Also, input binding has changed: middle mouse is orbit, right mouse is pan per canon.

- **Item: Viewport Bounds and Map Clamping (CAMERA-06 / 2-025)**
  - Previous: Visible world rectangle calculated from orthographic frustum with fixed orientation.
  - Revised: Visible bounds must be calculated from a potentially rotated and tilted frustum. The visible region is no longer a simple axis-aligned rectangle -- it becomes a frustum-projected polygon on the ground plane. Map clamping must prevent the focus point from moving too far outside the map at any camera angle. Configurable map sizes (128/256/512) mean the bounds constraints must be parameterized by map dimensions, not hardcoded.
  - Reason: Camera rotation means the visible footprint is orientation-dependent. Configurable map sizes require parameterized bounds.

- **Item: Frustum Culling (RENDER-08 / 2-026)**
  - Previous: Frustum culling listed as optimization, described as extracting planes from VP matrix.
  - Revised: Frustum culling is now **mandatory** for large maps (512x512 = 262,144 tiles). The frustum extraction math is the same, but spatial partitioning (octree or grid-based) is now a requirement, not optional. Culling must handle the full range of camera orientations, not just one fixed view direction. The ticket should be elevated in priority for large map support.
  - Reason: Canon v0.5.0 specifies 512x512 maps "require LOD and frustum culling." This is no longer a nice-to-have.

- **Item: Camera Animation (CAMERA-07 / 2-027)**
  - Previous: Animate to target position, camera shake. Focus point animation only.
  - Revised: Animation must now also support orbit/tilt transitions. When snapping to an isometric preset, the camera should smoothly animate pitch and yaw to the target preset angles. The "animate to" feature must interpolate all camera parameters (focus point, distance, pitch, yaw), not just position.
  - Reason: Mode switching (free to preset, preset to preset) requires animated angle transitions for a polished feel.

- **Item: Screen-to-World Ray Casting (COORD-02 / 2-029)**
  - Previous: Ray casting from fixed orthographic projection. With orthographic, the ray direction is constant (parallel to view direction) and only the ray origin varies.
  - Revised: If perspective projection is used (recommended for free camera), ray direction varies per pixel (divergent rays from camera position through screen point). The ray casting math changes from orthographic (parallel rays) to perspective (divergent rays). The existing code snippet in the analysis already handles perspective divide, but the orthographic special case (where ray direction is constant) would no longer apply.
  - Reason: Projection type change from orthographic to perspective (or hybrid) changes ray casting math.

- **Item: Tile Picking (COORD-03 / 2-030)**
  - Previous: Ray cast against ground plane from fixed angle. At fixed isometric angle, picking is geometrically stable.
  - Revised: Tile picking must work correctly at all camera angles, including extreme tilt and orbit positions. At near-horizontal tilt angles, ray-ground intersection becomes numerically unstable (ray nearly parallel to ground). The picking system should handle edge cases: very low pitch angles, picking at extreme zoom, and picking with perspective distortion.
  - Reason: Free camera means the pick ray can approach the ground plane at any angle, including near-parallel.

- **Item: TransformComponent Definition (RCOMP-02 / 2-032)**
  - Previous: Rotation stored as quaternion but noted "or Euler if simpler for fixed-angle." Since camera was fixed, rotation was largely cosmetic.
  - Revised: Rotation is now fully relevant -- models will be viewed from all angles. Quaternion rotation is the correct choice (not Euler). All models must be properly textured on all visible sides. The dirty flag and cached matrix computation remain the same, but rotation will change more frequently for entities that animate or face the camera.
  - Reason: Free camera means every model is viewed from arbitrary angles; rotation in TransformComponent is no longer decorative.

- **Item: RenderComponent Definition (RCOMP-01 / 2-031)**
  - Previous: Model, material, layer, visibility, tint, scale.
  - Revised: RenderComponent should include an `emissive_intensity` or `glow_color` field to support the bioluminescent art direction at the per-entity level (e.g., powered vs unpowered glow state). Alternatively, this can live in the material system, but the RenderComponent needs a way to toggle emissive state per-instance (powered buildings glow, unpowered buildings do not).
  - Reason: Canon v0.5.0 specifies that structures glow when active (powered + watered) and lose glow when unpowered. This requires per-instance emissive control, not just per-material.

- **Item: Toon Shader Implementation (RENDER-03 / 2-005)**
  - Previous: Stepped lighting bands, purple shadow shift, single directional light.
  - Revised: Toon shader must now explicitly support the bioluminescent color palette. Shadow color shifts toward deep blue/purple (per art direction: "deep blues, dark greens, dark purples"). Shader must interact correctly with emissive materials -- emissive output should not be affected by toon lighting bands. The shader should support a `glow_color` uniform or texture channel for per-instance bioluminescent effects.
  - Reason: Canon v0.5.0 establishes bioluminescent art direction as core visual identity. Toon shader must be designed with this palette in mind from the start.

- **Item: ToonShaderConfig Runtime Resource (2-008)**
  - Previous: Band thresholds, shadow color, edge thickness.
  - Revised: ToonShaderConfig should also include: bloom intensity/threshold, emissive multiplier, and potentially a "glow palette" for the bioluminescent accent colors. These parameters are part of the visual identity and should be tunable at runtime alongside the toon bands.
  - Reason: Bloom and emissive are now core visual features, not optional post-processing. Their parameters should be part of the unified shader config.

- **Item: Emissive Material Support (2-037)**
  - Previous: Listed as "Visual Effects (Foundation)" -- optional/nice-to-have.
  - Revised: Emissive materials are now a **core requirement**, not a visual effect enhancement. Every powered structure should have emissive output. Terrain features (crystal fields, spore plains, volcanic rock) have distinct glow properties. Emissive support must be in the initial toon shader implementation (2-005), not deferred to a later group.
  - Reason: Canon v0.5.0 makes bioluminescence the defining art characteristic. "Emissive materials for bioluminescent glow" is listed as a core shading feature.

- **Item: Bloom Post-Process (2-038)**
  - Previous: Listed as "Visual Effects (Foundation)" -- dependent on emissive, later in pipeline.
  - Revised: Bloom is now a **required post-process** for the bioluminescent art direction, not optional. "Bloom post-processing for glow bleed" is listed as a core shading feature in canon. Bloom should be included in the critical path and its 0.5ms budget is already specified in the ticket summary. It should move from Group I (Visual Effects) to the main render pipeline (Group D).
  - Reason: Canon v0.5.0 specifies bloom as a core visual feature for glow bleed.

- **Item: Complete Render Frame Flow (2-019)**
  - Previous: Edge post-process -> transparents -> bloom -> UI. Bloom was already listed here.
  - Revised: Bloom is already in the frame flow but was dependent on emissive (2-037) being implemented first. Since emissive is now core, the dependency chain tightens: emissive must be in the initial toon shader, and bloom must be in the initial frame flow. No structural change, but priority elevation is needed.
  - Reason: Emissive and bloom are no longer deferrable; they must be in the initial rendering pipeline.

- **Item: Risk 2: Toon Shader Complexity**
  - Previous: Severity Medium. Concerned about achieving the right "cell-shaded alien" look.
  - Revised: Severity **Medium-High**. The bioluminescent palette adds constraints: dark base tones must work with glow accents, emissive materials must interact correctly with toon bands, and bloom must not wash out the toon shading style. The shader must balance three competing concerns: toon bands, emissive glow, and bloom bleed. More iteration will be required.
  - Reason: Additional visual constraints from bioluminescent art direction increase shader complexity.

- **Item: Risk 4: Performance with 3D Rendering**
  - Previous: Severity Medium. Target: 100k triangles, <200 draw calls, 60 FPS.
  - Revised: Severity **Medium-High**. Large maps (512x512 = 262,144 tiles) with 10 terrain types having distinct visuals and glow properties significantly increase the rendering load. Bloom post-process adds GPU cost. Emissive materials with per-instance glow states add uniform complexity. The draw call budget should be revisited -- the tickets already specify 500-1000 draw calls which is more generous than the original 200, but 512x512 maps may still stress this. LOD is mandatory for large maps, not optional.
  - Reason: Configurable map sizes (up to 512x512) and mandatory bloom/emissive increase performance pressure.

- **Item: Risk 5: Camera Math Complexity**
  - Previous: Severity Low-Medium. Concerned about 3D camera math bugs.
  - Revised: Severity **Medium**. Free camera with full orbit/tilt adds gimbal lock risk (mitigated by quaternions), numerical instability at extreme pitch angles (near-vertical or near-horizontal), and more complex frustum extraction. The interaction between two camera modes (free vs preset) adds state management complexity and potential for bugs during transitions.
  - Reason: Free camera is significantly more complex than fixed-angle camera. Mode switching adds state machine complexity.

- **Item: Camera as Singleton Manager (Architectural Recommendation 5)**
  - Previous: CameraSystem with fixed pitch/yaw, only focus_point and distance as state.
  - Revised: CameraSystem must store: focus_point, distance, pitch, yaw, current_mode (free/preset), target preset index (for snap transitions), animation state for all parameters. The interface expands: `get_camera_mode()`, `set_preset(PresetDirection)`, `toggle_mode()`, `orbit(float delta_yaw)`, `tilt(float delta_pitch)`. Design still supports multiple cameras later.
  - Reason: Free camera requires substantially more state and a richer public interface.

- **Item: Debug Grid Overlay (2-040)**
  - Previous: Grid lines on terrain plane.
  - Revised: Grid overlay must remain readable at all camera angles, not just the fixed isometric view. At low tilt angles, grid lines become visually compressed and hard to read. The grid may need to fade or change rendering at extreme angles. Grid should also handle configurable map sizes (128/256/512) without becoming too dense or too sparse.
  - Reason: Free camera means grid is viewed from arbitrary angles; must remain useful.

- **Item: Shadow Rendering (2-039)**
  - Previous: Basic shadow mapping from directional light. "Shadows help ground structures."
  - Revised: Shadow direction must work with the bioluminescent art direction. In a dark-base-tone environment, shadows are less visible against already-dark surfaces. Shadow color/intensity may need adjustment. The light direction should still be configurable but needs to consider that the primary visual contrast comes from glow, not from lit-vs-shadow. Shadow mapping viewpoint must also account for free camera (shadow frustum fitting changes with camera orientation).
  - Reason: Bioluminescent palette (dark base tones) changes how shadows read visually. Free camera affects shadow map frustum fitting.

- **Item: Data Flow Per Frame (Camera Update section)**
  - Previous: CameraSystem reads pan keys and zoom wheel, updates focus point and distance only.
  - Revised: CameraSystem reads orbit input (middle mouse/Q/E), tilt input (vertical angle control), pan input (WASD/right mouse/edge scroll), zoom input (scroll wheel). Updates pitch, yaw, focus point, and distance. Checks current mode (free vs preset) and applies constraints accordingly. Calculates view and projection matrices from full orientation state.
  - Reason: Expanded input set and mode management for free camera.

- **Item: Camera Configuration code sample (3D Coordinate System section)**
  - Previous: `CAMERA_PITCH = 35.264`, `CAMERA_YAW = 45.0` as constants. `calculate_camera_position()` using fixed angles.
  - Revised: Pitch and yaw are now variables, not constants. The preset angles become named presets (e.g., `PRESET_N = {pitch: 35.264, yaw: 45}`, `PRESET_E = {pitch: 35.264, yaw: 135}`, etc.). `calculate_camera_position()` uses current pitch/yaw from camera state. The code sample is architecturally invalidated in its current form but the math is reusable.
  - Reason: Fixed-angle constants replaced by variable camera orientation with preset snappoints.

- **Item: Projection Matrix code sample**
  - Previous: Orthographic projection code sample using `ortho()`.
  - Revised: If perspective projection is adopted (recommended), this changes to `perspective(fov, aspect, near, far)`. If orthographic is kept, the code is still valid. This is a pending decision point.
  - Reason: Projection type is now a decision point due to free camera support.

---

### INVALIDATED

- **Item: Risk 6: Isometric vs True 3D Expectations**
  - Reason: This risk was about players expecting fixed isometric behavior and resisting scope creep toward free camera. Canon v0.4.1 explicitly embraces free camera as a core feature. The risk as stated ("Lock camera rotation to fixed isometric angle initially... Resist feature creep") directly contradicts canon. The concern about building detail ("Design buildings with front-facing detail, don't need all 4 sides") is also invalidated -- buildings must now be fully detailed from all angles.
  - Recommendation: **Replace** with a new risk about camera mode UX (see NEW CONCERNS below).

- **Item: Q7 (for Game Designer): "Which isometric angle feels appropriate?"**
  - Reason: Canon v0.4.1 establishes free camera with isometric presets at ~35.264-degree pitch with 45-degree yaw increments. The angle question is resolved by canon.
  - Recommendation: **Remove** -- answered by canon.

- **Item: Q8 (for Game Designer): "Under what conditions would we add camera rotation?"**
  - Reason: Canon v0.4.1 adds camera rotation as core functionality from day one. This question is fully answered.
  - Recommendation: **Remove** -- answered by canon.

- **Item: Open Question 1: "Orthographic vs Perspective"**
  - Reason: This was decided as orthographic by consensus for fixed isometric. With free camera, this decision needs to be **reopened**. The original rationale (grid clarity, no foreshortening, classic feel) was predicated on a fixed camera angle. Free camera with orthographic can feel disorienting at non-standard angles.
  - Recommendation: **Replace** with a new decision: Projection strategy for free camera (see NEW CONCERNS).

- **Item: Ticket note "Camera rotation (Q/E 90-degree snaps) deferred to later phase but models should be 4-sided ready"**
  - Reason: Camera rotation is no longer deferred. It is core Epic 2 functionality. Models must be 4-sided (all angles visible) from the start.
  - Recommendation: **Remove** the deferral note. Orbit controls are now in-scope for Epic 2.

---

### NEW CONCERNS

- **Concern: Camera Mode State Machine and UX**
  - Reason: Canon introduces two camera modes (free vs isometric presets) with toggling between them. This requires a state machine: what happens when a player is in free camera at an unusual angle and presses a preset snap key? Does it animate smoothly to the preset? Does it snap instantly? What if they press orbit during a snap animation? These UX transitions are new complexity not in the original analysis.
  - Recommendation: Add a new ticket (or expand 2-017) for camera mode management state machine. Define transitions: free-to-preset (smooth snap), preset-to-free (instant unlock), preset-to-preset (smooth rotate), interruption behavior (any input cancels snap animation). Input bindings per canon: orbit via middle mouse/Q/E, pan via WASD/right mouse/edge scroll, zoom via scroll wheel, preset snaps via hotkeys.

- **Concern: Projection Type Decision Reopened**
  - Reason: The consensus decision for orthographic projection was predicated on a fixed isometric camera. Free camera with full orbit and tilt makes orthographic feel unnatural (objects maintain constant screen size regardless of distance, which is disorienting when rotating freely). This decision must be revisited.
  - Recommendation: Create a decision record evaluating three options: (A) orthographic for both modes, (B) perspective for free / orthographic for preset, (C) perspective for both modes with narrow FOV. My recommendation as Systems Architect is option C (perspective with narrow FOV ~30-40 degrees) for simplicity (one projection type) and natural free-camera feel, with preset angles tuned to approximate isometric appearance.

- **Concern: Model/Asset All-Angle Readiness**
  - Reason: With free camera, all models are viewable from any angle. The original analysis noted buildings could have "front-facing detail" since the camera was fixed. This is no longer valid. All building models, terrain features, and entities must be fully textured and detailed on all sides. This affects the art pipeline (hand-modeled and procedural) and increases asset production scope.
  - Recommendation: Update model requirements documentation to specify full 360-degree detail. Communicate to art pipeline that single-angle optimization is not possible. This primarily affects Epic 4+ (building assets) but Epic 2 should establish the expectation in its model loading and placeholder systems.

- **Concern: Per-Instance Emissive State for Bioluminescence**
  - Reason: Canon v0.5.0 specifies that structures glow when active and lose glow when unpowered/abandoned. This requires per-instance emissive control -- the same building model must render with or without glow depending on its game state (powered, watered, abandoned). This is not a material-level toggle; it is an instance-level property that must be uploaded as part of the instance data (or as a uniform per draw call).
  - Recommendation: Add emissive intensity/color to the instance buffer data (alongside transform and tint). Or add an `emissive_state` field to RenderComponent. The toon shader must read this per-instance data. This should be part of ticket 2-005 (Toon Shader) and 2-012 (Instancing).

- **Concern: Configurable Map Size Impact on Spatial Partitioning**
  - Reason: 512x512 maps have 262,144 tiles. With buildings and terrain features, entity counts could reach 300k+. Naive ECS queries for renderable entities will be too slow. The frustum culling ticket (2-026) mentions spatial partitioning as optional ("octree or grid"). For 512x512 maps, spatial partitioning is mandatory.
  - Recommendation: Elevate spatial partitioning from optional to required in ticket 2-026. Recommend a 2D grid-based spatial hash (cells of 16x16 or 32x32 tiles) rather than octree, since the game world is predominantly planar. This is simpler and more cache-friendly for city-builder workloads.

- **Concern: LOD System Required for Large Maps**
  - Reason: Canon v0.5.0 explicitly states large maps (512x512) "require LOD and frustum culling." The original analysis mentioned LOD as "future" in RENDER-06 (Model Asset Binding). This is no longer future -- it is a requirement for large maps. A basic LOD system (at minimum 2 levels: full detail and simplified) must be designed as part of Epic 2.
  - Recommendation: Add a new ticket or expand 2-026 to include basic LOD switching based on camera distance. At minimum: full-detail models within N units of camera, simplified models (or billboards) beyond N units, and culled beyond frustum. LOD distances should be configurable and map-size-aware.

- **Concern: Terrain Type Visual Diversity (10 types with glow properties)**
  - Reason: Canon v0.5.0 expands terrain from 4 to 10 types, each with "distinct visuals and glow properties." Crystal fields have "strong glow emission," spore plains have "pulsing glow," toxic marshes have "sickly yellow-green glow," volcanic rock has "orange/red glow cracks." This means the terrain rendering system must support per-tile-type emissive properties, which interacts with the instancing system (terrain tiles are instanced, but now need per-type emissive parameters).
  - Recommendation: This is primarily an Epic 3 (Terrain) concern, but Epic 2 must ensure the rendering infrastructure supports it. The instancing system (2-012) should support per-instance emissive color/intensity in the instance buffer. The material system must accommodate 10+ terrain material variations efficiently (texture atlas or material array).

- **Concern: Dual UI Mode Rendering Implications**
  - Reason: Canon v0.5.0 introduces two UI modes (Classic and Sci-fi Holographic). The holographic mode features "translucent panels with glowing borders," "scan-line and flicker effects," and "hologram-style readouts." These may require post-process effects or special rendering support beyond what SDL_Renderer provides for 2D UI.
  - Recommendation: This is primarily an Epic 12 (UI) concern, but Epic 2 should note the interface point. The current plan has UI rendered via SDL_Renderer (2D) on top of the 3D scene. Holographic UI effects (translucency, glow borders, scan lines) may need to be rendered via SDL_GPU instead of SDL_Renderer, or may require an additional compositing pass. Flag this for Epic 12 planning.

---

## Questions for Other Agents

### @game-designer

- **Q-NEW-1: Camera Input Bindings Confirmation.** Canon specifies orbit via middle mouse/Q/E, pan via WASD/right mouse/edge scroll, zoom via scroll wheel. Please confirm: (a) Is middle mouse drag = orbit (continuous rotation), and Q/E = 90-degree snap rotate (preset)? Or are Q/E also continuous orbit? (b) What key/button switches between free and preset mode? (c) Are there preset snap hotkeys beyond Q/E (e.g., numpad 1-4 for N/E/S/W)?

- **Q-NEW-2: Projection Preference for Free Camera.** With free camera, orthographic projection can feel unnatural during orbit/tilt. Do you have a preference between: (a) orthographic (classic isometric feel, but odd when orbiting freely), (b) perspective with narrow FOV (natural free camera, slight foreshortening), or (c) hybrid (orthographic in preset mode, perspective in free mode)?

- **Q-NEW-3: Bioluminescent Glow State Transitions.** When a building loses power and its glow goes out, should this be: (a) instant on/off, (b) fade out over ~1 second, or (c) flicker and fade? This affects whether emissive state is binary or animated.

---

## Affected Tickets

### Group E: Camera System (Major Rework)

- **2-017: MODIFY** -- CameraState must include pitch, yaw as variables (not constants), camera mode enum (Free/Preset), active preset index, mode transition state. Remove "fixed pitch: 26.57 degrees" and "fixed yaw: 45 degrees." Add orbit/tilt state. Add mode toggle API.

- **2-020: MODIFY** -- View matrix calculation must use variable pitch/yaw from camera state instead of constants. Math is the same, inputs change.

- **2-021: MODIFY** -- Projection matrix needs re-evaluation. If perspective is chosen, change from `ortho()` to `perspective()`. Remove "parallel lines stay parallel" acceptance criterion. Add "projection type configurable" criterion.

- **2-022: MODIFY** -- VP integration is unchanged structurally but must handle projection type changes if hybrid projection is adopted.

- **2-023: MODIFY** -- Pan controls must be camera-orientation-relative, not world-axis-aligned. Input binding changes: right mouse for pan (not middle mouse). Add edge-scroll pan.

- **2-024: MODIFY** -- Zoom range should be map-size-aware. Zoom behavior depends on projection type.

- **2-025: MODIFY** -- Viewport bounds calculation must handle arbitrary camera orientations. Map boundary clamping must be parameterized by configurable map size (128/256/512).

- **2-026: MODIFY** -- Elevate frustum culling from optimization to requirement. Add mandatory spatial partitioning for large maps. Culling must work at all camera angles.

- **2-027: MODIFY** -- Camera animation must interpolate pitch/yaw for preset snap transitions, not just focus point position.

### Group B: Toon Shader (Moderate Rework)

- **2-005: MODIFY** -- Toon shader must integrate emissive material support from day one (not deferred to 2-037). Add per-instance emissive color/intensity uniform. Shadow colors must use bioluminescent palette (deep blues/purples, not just generic darker).

- **2-008: MODIFY** -- ToonShaderConfig should include bloom threshold, bloom intensity, and emissive multiplier alongside existing toon parameters.

### Group C: Model Pipeline (Minor Adjustments)

- **2-012: MODIFY** -- Instance buffer should include per-instance emissive color/intensity for bioluminescent glow state.

### Group F: Coordinate Conversion (Moderate Rework)

- **2-029: MODIFY** -- Ray casting must handle perspective projection (divergent rays) if projection type changes from orthographic. Must handle all camera angles gracefully.

- **2-030: MODIFY** -- Tile picking must be robust at all camera tilt angles, including near-horizontal. Add numerical stability guards.

### Group G: Components (Minor Adjustments)

- **2-031: MODIFY** -- RenderComponent should include emissive state (color/intensity) for per-instance bioluminescent control.

- **2-032: MODIFY** -- TransformComponent rotation should use quaternion (not "or Euler if simpler"). Remove the Euler option; quaternion is required for free camera.

### Group I: Visual Effects (Priority Elevation)

- **2-037: MODIFY** -- Elevate from "Visual Effects Foundation" to core toon shader feature. Move dependency: should be part of 2-005, not a separate later ticket. Or keep separate but move to Group B and make it a dependency of 2-005 rather than the reverse.

- **2-038: MODIFY** -- Elevate bloom from optional post-process to required rendering feature. Move from Group I to Group D (Depth and Render Pass). Ensure it is in the critical path.

### Group J: Debug (Minor Adjustments)

- **2-040: MODIFY** -- Debug grid overlay must remain readable at all camera angles. Consider fading or adjusting density based on camera tilt.

### New Tickets Needed

- **NEW: Camera Mode Management** -- State machine for free vs preset camera modes. Define transitions, input bindings per mode, smooth snap animation between presets, interruption behavior. Could be folded into expanded 2-017 or a new ticket 2-046.

- **NEW: LOD System (Basic)** -- Required for 512x512 maps per canon. At minimum 2 LOD levels with distance-based switching. Configurable LOD distances. Map-size-aware defaults. Could be ticket 2-047.

- **NEW: Spatial Partitioning for Culling** -- Grid-based spatial hash (16x16 or 32x32 cells) for efficient frustum culling on large maps. Required for 512x512. Could be folded into expanded 2-026 or a new ticket 2-048.

- **NEW: Projection Type Decision** -- Decision record needed: orthographic vs perspective vs hybrid for free camera. Affects 2-021, 2-024, 2-029, 2-030. Should be resolved before camera implementation begins.

---

## Internal Dependencies Update

The dependency graph from the original analysis is structurally the same but the **critical path changes**:

Previous critical path ended at camera basics (fixed state, fixed matrices).
New critical path must include: camera mode management, orbit/tilt controls, and emissive/bloom (now core).

```
Revised Critical Path:
1. RENDER-01 (SDL_GPU Device)
2. RENDER-02 (Pipeline Setup)
3. RENDER-03 (Toon Shader) -- now includes emissive support
4. RENDER-04 (Model Rendering)
5. CAMERA-01 (Camera State) -- now includes mode management
6. CAMERA-02 (View Matrix) -- now with variable orientation
7. CAMERA-03 (Projection) -- pending projection type decision
8. RENDER-07 (Depth Buffer)
9. RENDER-09 (MVP Integration)
10. CAMERA-04, 05 (Zoom, Pan) -- now camera-relative
11. NEW: Camera Orbit/Tilt Controls
12. NEW: Camera Mode Transitions
13. RENDER-08 (Culling) -- now mandatory with spatial partitioning
14. 2-037/2-038 (Emissive + Bloom) -- now on critical path
15. COORD-03 (Picking)
```
