# Graphics Engineer Impact Analysis: Epic 2

**Epic:** Core Rendering Engine (3D)
**Analyst:** Graphics Engineer
**Date:** 2026-01-28
**Canon Change:** v0.4.0 -> v0.5.0 (includes v0.4.1 camera changes)

---

## Trigger
Canon update v0.4.0 -> v0.5.0

## Overall Impact: HIGH

## Impact Summary
The canon update introduces two sweeping changes to the rendering pipeline: (1) the camera model changed from fixed isometric (orthographic-only, locked angles) to a free camera with full orbit/pan/zoom/tilt plus isometric preset snap views, which invalidates the fixed-angle assumptions baked into camera math, projection, shaders, edge detection, and shadow mapping; (2) the bioluminescent art direction with emissive materials, per-terrain-type glow properties, bloom post-processing, and configurable map sizes (up to 512x512) significantly expands the shader system, render pass structure, and performance optimization requirements.

---

## Item-by-Item Assessment

### UNAFFECTED

- **Item: RENDER-3D-01 (GPU Device Creation)** -- Reason: Device initialization is independent of camera model, art direction, and map size. No change to backend detection, debug layers, or device creation logic.

- **Item: RENDER-3D-02 (Window Integration and Swap Chain)** -- Reason: Swap chain management, vsync, resize handling, and fullscreen toggle are unaffected by camera or art direction changes.

- **Item: RENDER-3D-03 (Resource Management Infrastructure)** -- Reason: Buffer pools, texture loading, sampler creation, and frame resource management are GPU infrastructure that does not depend on camera angles or art style specifics.

- **Item: RENDER-3D-04 (Shader Compilation Pipeline)** -- Reason: The compilation workflow (HLSL -> SPIR-V/DXIL, hot-reload, fallback strategy) is independent of what the shaders compute. More shaders may be needed, but the pipeline itself is unchanged.

- **Item: RENDER-3D-08 (glTF Model Loading)** -- Reason: cgltf integration and model parsing are format-level concerns. Emissive materials are already part of glTF PBR spec and extractable through the same path. No structural change needed.

- **Item: RENDER-3D-10 (Render Command Recording)** -- Reason: The draw call recording pattern (bind vertex/index buffers, upload uniforms, issue draw) is invariant to camera or art changes.

- **Item: RENDER-3D-15 (Depth Buffer Setup)** -- Reason: Depth texture creation, format selection, and resize handling are unaffected. Depth buffer works identically for perspective and orthographic projections.

- **Item: RENDER-3D-16 (Depth State Configuration)** -- Reason: LESS compare, depth write enable/disable for opaque/transparent passes remain the same regardless of projection type.

- **Item: RENDER-3D-17 (Transparent Object Handling)** -- Reason: Opaque-first then back-to-front transparent rendering order is projection-independent.

- **Item: RENDER-3D-20 (Layer Management / RenderLayer enum)** -- Reason: Semantic render layers are defined by content type, not camera model or art style.

- **Item: RENDER-3D-21 (Underground View Toggle)** -- Reason: Layer visibility toggling is unaffected.

- **Item: RENDER-3D-22 (Data Overlays)** -- Reason: Overlay rendering as transparent layers is independent of these changes.

- **Item: RENDER-3D-23 (AssetManager Integration)** -- Reason: Asset handle pattern, caching, and async loading are unchanged.

- **Item: RENDER-3D-24 (ECS Component Integration)** -- Reason: ModelComponent and TransformComponent structures are projection/camera-independent. (EmissiveComponent is a NEW concern addressed below.)

- **Item: RENDER-3D-25 (Interpolation)** -- Reason: Transform interpolation between ticks is independent of camera or art direction.

- **Item: RENDER-3D-13 (Placeholder Model)** -- Reason: Magenta placeholder cube is unaffected.

- **Item: Risk 1 (SDL_GPU Learning Curve)** -- Reason: Still applies, severity unchanged.

- **Item: Risk 2 (Shader Cross-Compilation)** -- Reason: Still applies, no change to build complexity.

- **Item: Risk 3 (glTF Library Integration)** -- Reason: Still applies, no change.

- **Item: Risk 6 (UI Integration)** -- Reason: SDL_Renderer for 2D UI coexistence is unchanged.

---

### MODIFIED

- **Item: RENDER-3D-05 (Toon Shader Implementation / Fragment Shader)**
  - Previous: Fragment shader with 3-4 discrete lighting bands, shadow color shift toward purple, single directional light. Band values are neutral grayscale multiplied against texture color. No emissive term.
  - Revised: Fragment shader must now include: (a) emissive material term added to final output, unaffected by lighting bands; (b) shadow color shift toward purple/teal is now canon-required, not merely aesthetic preference; (c) band colors should be tuned for the bioluminescent palette (dark base tones in shadow, not generic gray); (d) shader must produce correct results from ANY view angle (free camera means normals are evaluated from arbitrary directions, not just the fixed isometric angle -- this was already mathematically correct but must be explicitly validated); (e) output must feed into bloom pass (bright emissive pixels extracted by threshold).
  - Reason: Bioluminescent art direction makes emissive materials a core shader feature. Free camera means the shader is viewed from all angles, so fixed-angle visual assumptions (like "shadow always falls this way") are invalid.

- **Item: RENDER-3D-06 (Screen-Space Edge Detection / Sobel Post-Process)**
  - Previous: Sobel edge detection on depth/normal buffers, designed for orthographic projection where depth values are linearly distributed.
  - Revised: Must work correctly with BOTH perspective and orthographic projections. In perspective projection, depth values are non-linear (clustered near the near plane), so depth-based Sobel thresholds need to account for this. Normal-based edge detection is projection-independent and should be preferred as the primary edge signal. Depth edges may need linearization before Sobel in perspective mode, or use an adaptive threshold based on pixel depth. Edge thickness must remain consistent in screen-space regardless of projection type.
  - Reason: Free camera likely uses perspective projection (or hybrid), where depth distribution differs from orthographic. Edge detection must not produce artifacts or inconsistent outlines when switching between free camera and isometric presets.

- **Item: RENDER-3D-07 (Graphics Pipeline Creation)**
  - Previous: Single pipeline configuration for fixed isometric toon rendering.
  - Revised: Pipeline state itself (vertex layout, depth state, blend state, cull mode) is largely unchanged, but must be validated with perspective projection. Additionally, the emissive material path may require a variant pipeline or additional render target attachment (for emissive channel output to feed bloom). If bloom uses a bright-pass extraction from the main color buffer, no pipeline change is needed; if it uses a separate emissive render target (MRT), the pipeline must be configured for multiple render targets.
  - Reason: Bloom implementation strategy affects pipeline configuration. Perspective projection needs validation for cull mode and depth precision.

- **Item: RENDER-3D-09 (GPU Mesh Representation)**
  - Previous: GPUMesh with vertex buffer, index buffer, diffuse texture, AABB.
  - Revised: GPUMesh must now also store emissive texture reference (or emissive color if no texture). Material struct needs an emission field. glTF already supports emissive in its material model, so this is an extraction + storage concern.
  - Reason: Emissive materials are core to bioluminescent art direction. Every mesh needs to carry its emission data for the toon shader.

- **Item: RENDER-3D-11 (Instance Rendering / Instancing)**
  - Previous: Instance buffer with per-instance transforms and tint colors. Budget of 500-1000 draw calls.
  - Revised: Instance buffer should also carry per-instance emissive intensity/color (e.g., powered vs unpowered state changes emission). For 512x512 maps (262K tiles), the instancing budget and strategy must be reassessed. At maximum zoom-out on a large map, potentially tens of thousands of terrain tiles are visible. Instancing batches must be larger, and the 500-1000 draw call budget should be treated as a hard ceiling that drives aggressive batching. Consider chunked instancing (group tiles into 16x16 or 32x32 chunks with one instanced draw per chunk per model type).
  - Reason: Large map support (512x512) and per-instance emissive state both expand instancing requirements.

- **Item: RENDER-3D-12 (Camera Matrix Calculations / IsometricCamera)**
  - Previous: Fixed isometric camera: 35.264 pitch, 45 yaw, calculated from spherical coords. Single getViewMatrix() and getProjectionMatrix() that produce orthographic projection.
  - Revised: Camera struct must be generalized to a FreeCamera with variable pitch, yaw, and position. The isometric presets (N/E/S/W at 45-degree yaw increments, ~35.264 pitch) become parameterized snap targets, not the only mode. getViewMatrix() is already generic (lookAt from spherical coords works for any angles). getProjectionMatrix() must support BOTH orthographic (for isometric presets) and perspective (for free camera) -- or perspective always. This is a design decision that needs resolution. The entire IsometricCamera struct should be renamed/refactored to a more general Camera struct with a mode enum (Free, Preset_N, Preset_E, Preset_S, Preset_W).
  - Reason: Canon v0.4.1 replaces fixed isometric with free camera + presets. The camera is no longer constrained to two fixed angles.

- **Item: RENDER-3D-13 (Orthographic vs Perspective Decision)**
  - Previous: Recommended orthographic for authenticity to SimCity 2000 aesthetic.
  - Revised: This decision must be revisited. Options: (a) Perspective always -- simplest, works for free camera, isometric presets just set specific angles but still use perspective (slight foreshortening visible); (b) Orthographic for presets, perspective for free camera -- switching projection mid-play is jarring unless smoothly interpolated; (c) Perspective with very low FOV to approximate orthographic -- compromise approach. Recommendation shifts to **(a) perspective always** with a narrow FOV (~30-40 degrees) to minimize foreshortening while supporting free camera naturally. Orthographic breaks down for free camera (tilting to near-horizontal produces unintuitive results with ortho).
  - Reason: Free camera with full tilt range makes pure orthographic impractical. Perspective projection is the natural fit for orbit/tilt interaction.

- **Item: RENDER-3D-14 (Screen-to-World Ray Casting)**
  - Previous: Ray casting designed for orthographic projection (parallel rays, constant ray direction).
  - Revised: Must support perspective projection where rays diverge from camera position. The existing code already uses inverse view-projection matrix for unprojection, which is projection-agnostic. However, the comment "For tile picking with orthographic camera" and the assumption of parallel rays must be updated. With perspective, ray direction varies per pixel. The intersection math (ray-plane) is the same, but the ray construction differs. Need to ensure the inverse VP approach handles both projection types correctly (it does mathematically, but tests must cover both).
  - Reason: Free camera uses perspective projection; ray casting must produce correct results for divergent rays.

- **Item: RENDER-3D-18 (Main Render Pass Structure)**
  - Previous: Single render pass: clear, bind toon pipeline, render terrain/buildings/effects, end.
  - Revised: Render pass structure must now accommodate bloom post-processing as a REQUIRED pass (not optional/future). The pass order becomes: (1) main scene pass (opaques with toon shader + emissive output), (2) edge detection post-process, (3) transparent pass, (4) bloom extraction + blur + composite, (5) UI overlay. If bloom uses MRT (separate emissive target), the main scene pass needs an additional color attachment. Clear color should be dark (matching bioluminescent dark base environment, e.g., deep blue-black instead of the current sky blue {0.2, 0.3, 0.4, 1.0}).
  - Reason: Bloom is now canon-required, not a future enhancement. Clear color should match bioluminescent art direction.

- **Item: RENDER-3D-19 (Multi-Pass Rendering)**
  - Previous: Listed as "future" -- shadow map pass, SSAO, post-processing, UI.
  - Revised: Post-processing (bloom) is no longer future -- it is required for Epic 2. Shadow mapping must also handle variable camera angles (shadow map frustum can no longer be tightly fit to a fixed isometric view frustum -- it must adapt to the free camera's current view). The pass order should be formalized: (1) shadow map, (2) main scene (toon + emissive), (3) edge detection, (4) transparents, (5) bloom, (6) UI.
  - Reason: Bloom and emissive are now core requirements, not future enhancements.

- **Item: CAMERA-3D-01 (Camera State)**
  - Previous: Fixed pitch (35.264), fixed yaw (45), only distance and target are variable.
  - Revised: Pitch and yaw are now fully variable in free camera mode. Camera state needs: (a) mode enum (Free, Preset_N, Preset_E, Preset_S, Preset_W); (b) current pitch/yaw as float variables with min/max limits (e.g., pitch clamped to 10-89 degrees to prevent degenerate views); (c) preset snap targets as named configurations; (d) smooth interpolation when snapping to a preset; (e) tilt controls (pitch adjustment).
  - Reason: Canon v0.4.1 introduces free camera with full orbit/pan/zoom/tilt.

- **Item: CAMERA-3D-02 (Pan Controls)**
  - Previous: WASD/arrows move target in world XZ plane. Middle-mouse drag for pan.
  - Revised: Pan direction must now be relative to current camera yaw (not world-aligned). When camera is rotated, "W" should move the target "forward" relative to the camera's facing, not always world-north. Middle-mouse drag panning must also be camera-relative. This is a standard free-camera pattern but was not needed with fixed yaw.
  - Reason: Free camera rotation means world-aligned pan controls would feel wrong at non-45-degree yaw angles.

- **Item: CAMERA-3D-04 (Viewport Bounds)**
  - Previous: Calculate visible world rectangle from orthographic frustum (simple axis-aligned rectangle).
  - Revised: With perspective projection and variable camera angles, the visible region is a general trapezoid (frustum footprint), not a rectangle. getVisibleTileRange() must return a conservative bounding rectangle of the frustum footprint on the ground plane, or use the full frustum planes for culling. The "clamp target to map bounds" logic must account for perspective frustum (which sees more of the ground plane than ortho at the same distance).
  - Reason: Perspective frustum has a trapezoidal ground footprint, not rectangular.

- **Item: CAMERA-3D-05 (Camera Rotation)**
  - Previous: Marked as "Optional / Future" -- 90-degree yaw snaps.
  - Revised: No longer optional. Full orbit rotation is a core feature (free camera). Isometric preset snaps (45, 135, 225, 315) are also core. This work item is now mandatory, not optional.
  - Reason: Canon v0.4.1 makes camera rotation a core feature, not a future enhancement.

- **Item: RENDER-3D-37 (Emissive Material Support)**
  - Previous: Emission texture or color in material, fragment shader adds emission to output, energy conduits glow teal.
  - Revised: Scope significantly expanded. Emissive materials are now core to the entire visual identity, not just energy conduits. EVERY terrain type has distinct glow properties (crystal fields = magenta/cyan, spore plains = green/teal, toxic marshes = yellow-green, volcanic rock = orange/red, water = soft blue glow, forest = teal/green, flat ground = subtle moss glow, hills = glowing vein patterns). Buildings glow when active/powered and lose glow when unpowered/abandoned. Emissive intensity should be per-instance (driven by game state: powered, watered, active). The emissive term must be configurable per material AND per instance.
  - Reason: Bioluminescent art direction makes emissive materials central to the entire game's visual identity, not a limited feature for energy conduits alone.

- **Item: RENDER-3D-38 (Bloom Post-Process)**
  - Previous: Bloom for emissive materials, gaussian blur, additive blend, 0.5ms budget.
  - Revised: Bloom is now a REQUIRED post-process for the core art style (not just a nice-to-have). The bloom must handle the full range of emissive colors from the bioluminescent palette (cyan, green, amber, magenta, soft white/blue). Bloom intensity may need to be color-channel-aware (e.g., avoid oversaturating magenta). The 0.5ms budget may be tight given 512x512 maps with many emissive sources. Consider downscaled bloom (1/4 resolution blur) to meet the budget. Bloom should be configurable (quality tiers for different hardware) but NEVER fully disabled -- low-quality mode should still have minimal bloom for art consistency.
  - Reason: Bloom is now foundational to the art direction, not optional. 10 terrain types with glow + all powered buildings + conduits = massive emissive surface area.

- **Item: RENDER-3D-39 (Shadow Rendering)**
  - Previous: Basic shadow mapping from directional light, designed for fixed isometric angle.
  - Revised: Shadow map frustum must now adapt to the free camera's current view. With a fixed camera, the shadow map frustum could be tightly computed for the known view angle. With free camera, shadow map must use cascaded shadow maps (CSM) or a dynamically-fitted frustum that changes every frame based on the camera's view frustum. Shadow mapping at extreme tilt angles (near-horizontal) requires careful near/far fitting to avoid shadow acne and Peter Panning. Shadow style must still match toon aesthetic (hard edges).
  - Reason: Variable camera angles mean the shadow map cannot be pre-optimized for a single viewpoint.

- **Item: Risk 4 (Performance with Many Models)**
  - Previous: Severity Medium -- thousands of buildings at large cities.
  - Revised: Severity upgraded to **HIGH**. 512x512 maps have 262K tiles. Even with frustum culling, a zoomed-out view of a large map could have 10K+ visible terrain tiles plus buildings. LOD is now explicitly required by canon for large maps. The draw call budget (500-1000) will be heavily challenged. Instancing alone may not suffice -- LOD (reducing triangle count at distance) is critical. Consider terrain chunking (merge tiles into larger meshes for distant chunks). Emissive rendering adds cost (bloom pass scales with emissive pixel count).
  - Reason: Configurable map sizes up to 512x512 dramatically increase worst-case entity counts.

- **Item: Risk 5 (Orthographic Picking Precision)**
  - Previous: Severity Low -- orthographic picking can be tricky with stacked objects.
  - Revised: With perspective projection, picking is actually MORE intuitive (single ray from camera through pixel). However, the projection type decision (perspective vs ortho vs hybrid) introduces a new concern: if we switch projection between free and preset modes, picking behavior would change discontinuously. Recommend: use a single projection type (perspective) for consistent picking behavior.
  - Reason: Projection type decision affects picking behavior consistency.

---

### INVALIDATED

- **Item: Fixed isometric camera pitch/yaw as constants (35.264/45 hardcoded)**
  - Reason: Canon v0.4.1 makes pitch and yaw fully variable in free camera mode. These values are now preset parameters, not system constants. Code referencing `float pitch = 35.264f` and `float yaw = 45.0f` as fixed fields must be refactored to variable fields with preset snap targets.
  - Recommendation: Replace with a camera mode system. Preset values (35.264 pitch, 45/135/225/315 yaw) are stored as named presets. Camera state holds current pitch/yaw as mutable floats.

- **Item: Orthographic-only projection recommendation**
  - Reason: Orthographic projection does not work well for a free camera with full tilt. At near-horizontal tilt, orthographic produces unintuitive scaling (distant objects same size as near objects when looking near-horizontally). The recommendation for orthographic-only is invalidated.
  - Recommendation: Replace with perspective projection (narrow FOV ~30-40 degrees) or a smooth blend that approaches orthographic at preset angles. Decision needs resolution with Systems Architect.

- **Item: RENDER-3D-19 labeled as "Future"**
  - Reason: Multi-pass rendering (bloom post-process specifically) is no longer future. It is required for the bioluminescent art direction in Epic 2.
  - Recommendation: Remove "Future" designation. Bloom is part of the Epic 2 deliverable, not a later enhancement.

- **Item: CAMERA-3D-05 labeled as "Optional"**
  - Reason: Camera rotation is now a core feature (free camera with full orbit), not optional. The "future" annotation is invalidated.
  - Recommendation: Remove "Optional" designation. Camera orbit/rotation is a core Epic 2 deliverable.

---

### NEW CONCERNS

- **Concern: Projection Type Decision (Perspective vs Orthographic vs Hybrid)**
  - Reason: The switch from fixed isometric to free camera creates a fundamental projection question. Pure orthographic fails for free camera. Pure perspective loses the classic isometric feel at preset angles. A hybrid (switch projection based on mode) creates discontinuities. This must be decided before camera/shader/picking work can proceed.
  - Recommendation: Decide projection strategy. My recommendation is perspective with a configurable narrow FOV (30-40 degrees). At isometric preset angles with narrow FOV, the result is visually close to orthographic. In free camera mode, perspective feels natural. Single projection type eliminates discontinuities in picking, edge detection, and shadow mapping. This needs a formal decision record.

- **Concern: Per-Terrain-Type Emissive Properties**
  - Reason: Canon defines 10 terrain types, each with DISTINCT glow colors and behaviors (crystal fields = magenta/cyan spires with strong emission, spore plains = pulsing green/teal, toxic marshes = sickly yellow-green, volcanic rock = orange/red glow cracks, water = soft blue bioluminescent particles, forest = teal/green, flat ground = subtle moss, hills = glowing vein patterns). The toon shader and terrain rendering system must support per-terrain-type emissive parameters. This was not in the original analysis.
  - Recommendation: Create a TerrainVisualConfig data structure mapping terrain type to emissive color, emissive intensity, emissive pattern (static, pulse, flicker), and glow radius (for bloom). This config drives the per-instance emissive data uploaded to the GPU. May need a new ticket for terrain visual configuration.

- **Concern: LOD System for Large Maps (512x512)**
  - Reason: Canon explicitly requires LOD and frustum culling for 512x512 maps. The original analysis mentioned LOD as a "future" mitigation for Risk 4, but it is now canon-required. LOD for terrain (lower-poly chunks at distance) and buildings (simplified meshes at distance, potentially billboard imposters at extreme distance) must be part of Epic 2 scope.
  - Recommendation: Add LOD tickets. At minimum: (a) terrain LOD via chunk merging (distant 16x16 tile groups merge into single low-poly mesh), (b) building LOD with 2-3 levels (full model -> simplified -> billboard/imposter), (c) LOD selection based on camera distance. This is non-trivial and adds significant scope to Epic 2.

- **Concern: Draw Call Budget Viability at 512x512**
  - Reason: 512x512 = 262K tiles. At moderate zoom, thousands of tiles are visible. Even with aggressive instancing (one draw per model type per chunk), the 500-1000 draw call budget may be exceeded. Consider: 10 terrain types x 32 chunks visible = 320 terrain draws + building variety + effects + overlays. Feasible but tight. At high zoom (many tiles visible), frustum culling reduces count, but LOD must kick in or the triangle budget (100K) will be blown.
  - Recommendation: Formalize a performance budget table for each map size tier. For 512x512, accept that LOD aggressively reduces quality at distance. Consider compute-shader-based indirect draw for terrain to minimize CPU-side draw call overhead.

- **Concern: Bloom Performance on Large Emissive Surfaces**
  - Reason: In a bioluminescent world, a large fraction of the screen may contain emissive pixels (terrain glow, building glow, conduit glow). Traditional bloom (extract bright pixels, blur, composite) can be expensive when the bright-pass yields a large percentage of pixels. The 0.5ms bloom budget assumed sparse emissive sources.
  - Recommendation: Use downscaled bloom (extract and blur at 1/4 resolution, 1/8 for additional blur passes). This amortizes the cost over fewer pixels. Bloom quality tiers: High (1/2 res), Medium (1/4 res), Low (1/8 res). Test with worst-case scenario (zoomed-out crystal field terrain = massive emissive area).

- **Concern: Camera Mode Transition Visual Continuity**
  - Reason: Switching between free camera and isometric presets must be smooth. If the projection type changes (e.g., wider FOV in free mode, narrower at presets), the visual "snap" must be animated. If pitch/yaw snap to preset angles, the transition must interpolate smoothly.
  - Recommendation: All camera transitions (mode switch, preset snap) must use smooth interpolation (slerp for rotation, lerp for FOV/distance). Duration configurable (default ~0.3-0.5 seconds). Canon says "snap views" but this should be a quick smooth animation, not a hard cut, for visual comfort.

- **Concern: Shadow Map Stability with Free Camera**
  - Reason: With a fixed camera, shadow map stability (avoiding shimmering/swimming) is straightforward because the view doesn't change. With a free camera, the shadow map frustum changes every frame, causing shadow edge swimming (texel alignment issues). This is a well-known problem requiring shadow map stabilization techniques.
  - Recommendation: Implement shadow map texel snapping (round shadow map movement to texel boundaries) to prevent sub-texel shadow swimming. This is standard practice for any camera that moves freely.

- **Concern: Toon Shader Band Tuning for Dark Palette**
  - Reason: The original toon shader bands used generic grayscale values (1.0, 0.7, 0.4, 0.2). With the bioluminescent palette (dark base tones), the deep shadow band at 0.2 multiplied against already-dark base colors will produce near-black pixels, potentially losing detail. Band values need retuning for the dark palette.
  - Recommendation: Tune band values for the bioluminescent palette. Consider: (a) raising the deep shadow floor (0.2 -> 0.3); (b) shifting shadow color toward purple/teal (already partially specified, now canon-required); (c) adding a subtle ambient emissive term to prevent pure-black shadows in the dark world. ToonShaderConfig (ticket 2-008) must expose these as tunable parameters.

---

## Questions for Other Agents

### @systems-architect:

- **Q-NEW-01:** Projection type decision: Should we use perspective projection always (my recommendation), orthographic always, or a hybrid that switches based on camera mode? This must be decided before camera math, shader validation, edge detection, and picking work can proceed. I recommend perspective with narrow FOV (30-40 degrees) for reasons outlined in the INVALIDATED and NEW CONCERNS sections.

- **Q-NEW-02:** LOD strategy: Canon requires LOD for 512x512 maps. What LOD approach should we use for terrain and buildings? My recommendation is chunked terrain with distance-based polygon reduction and 2-3 LOD levels for buildings. This adds significant scope -- should LOD be part of Epic 2, or should we scope Epic 2 to 256x256 max and defer LOD to a follow-up?

- **Q-NEW-03:** Camera mode transitions: Should free-to-preset and preset-to-free transitions interpolate smoothly (my recommendation) or hard-snap? If interpolating, what duration?

### @game-designer:

- **Q-NEW-04:** Perspective vs orthographic feel: With perspective projection at a narrow FOV (~35 degrees), the view at isometric preset angles looks very similar to orthographic but with subtle depth cues. Is this acceptable, or is true orthographic (perfectly parallel lines) required for the intended aesthetic?

- **Q-NEW-05:** Free camera pitch limits: What are the acceptable pitch angle limits? I recommend clamping to 10-85 degrees (10 = near-horizontal, 85 = near-top-down). Near-horizontal views expose the low-poly nature of terrain from the side. Near-top-down views lose the isometric feel. What range matches the design vision?

---

## Affected Tickets

### Group B: Toon Shader System

- **2-005: MODIFY** -- Toon shader must add emissive material support (emissive color/texture added to final output, unaffected by lighting bands). Shadow color purple/teal shift is now canon-required. Band values must be tunable for bioluminescent dark palette. Shader must be validated from arbitrary view angles (free camera).

- **2-006: MODIFY** -- Sobel edge detection must work with perspective projection (non-linear depth). Prefer normal-based edges as primary signal. Add depth linearization or adaptive threshold for perspective depth edges. Validate at multiple camera angles.

- **2-007: MODIFY** -- Pipeline may need additional render target attachment if bloom uses MRT for emissive channel. Validate pipeline with perspective projection.

- **2-008: MODIFY** -- ToonShaderConfig must include emissive parameters (intensity, color) and bloom parameters (threshold, intensity). Add terrain glow config entries. Add per-terrain-type emissive color presets.

### Group C: 3D Model Pipeline

- **2-009: MODIFY** -- glTF loader must extract emissive texture and emissive factor from glTF material (already in spec, but extraction must be implemented, not skipped).

- **2-010: MODIFY** -- GPUMesh/Material struct must store emissive texture reference and emissive color/factor. Material definition expanded.

- **2-012: MODIFY** -- Instance buffer must include per-instance emissive intensity (for powered/unpowered state). Instance data layout changes. Budget reassessment for 512x512 maps. Consider chunked instancing.

### Group D: Depth Buffer and Render Pass

- **2-018: MODIFY** -- Clear color must change from sky blue to dark bioluminescent base (deep blue-black). Render pass must accommodate bloom as a required post-process step, not a future addition.

- **2-019: MODIFY** -- Complete frame flow must include bloom as a mandatory pass. SDL_Renderer coexistence unchanged but bloom is inserted before UI pass. Pass order formalized: scene -> edges -> transparents -> bloom -> UI.

### Group E: Camera System

- **2-017: MODIFY** -- Camera state must support free camera (variable pitch/yaw) plus preset modes. Mode enum required. Pitch/yaw are mutable with limits, not fixed constants. Animation state must handle mode transitions.

- **2-020: MODIFY** -- View matrix calculation unchanged in math, but must be validated with arbitrary pitch/yaw values. Edge cases: near-horizontal pitch, gimbal lock avoidance.

- **2-021: MODIFY** -- Projection matrix must support perspective (likely primary) in addition to or instead of orthographic. This is a significant change from "orthographic only." Pending projection type decision.

- **2-022: MODIFY** -- VP integration must handle dynamic projection changes (if hybrid approach chosen). Aspect ratio handling unchanged.

- **2-023: MODIFY** -- Pan controls must be camera-relative (direction adjusted by current yaw), not world-axis-aligned.

- **2-024: MODIFY** -- Zoom-toward-cursor with perspective projection uses a different focal adjustment than orthographic. Math must be updated.

- **2-025: MODIFY** -- Visible tile range calculation must use perspective frustum footprint (trapezoid, not rectangle). Conservative bounding rect of frustum-ground intersection.

- **2-026: MODIFY** -- Frustum culling planes change every frame with free camera (they already would with pan/zoom, but now also with rotation/tilt).

### Group F: Coordinate Conversion

- **2-029: MODIFY** -- Ray casting must handle perspective projection (divergent rays from camera, not parallel rays). Math already correct via inverse VP approach, but must be tested.

- **2-030: MODIFY** -- Tile picking must work correctly at all camera angles, not just the fixed isometric angle. Edge cases at steep angles.

### Group I: Visual Effects

- **2-037: MODIFY** -- Emissive material scope vastly expanded. All 10 terrain types have distinct glow properties. Buildings glow when powered, lose glow when unpowered. Per-instance emissive intensity required. Acceptance criteria must be expanded significantly.

- **2-038: MODIFY** -- Bloom is now mandatory, not optional. Must handle bioluminescent color range. Performance budget at risk with large emissive areas. Quality tiers required. 0.5ms budget needs validation against worst-case (crystal field zoomed out).

- **2-039: MODIFY** -- Shadow mapping must adapt to free camera (dynamic shadow frustum). Shadow stabilization (texel snapping) required. Cascaded shadow maps may be needed for large camera distance ranges.

### Group J: Debug Tools

- **2-040: MODIFY** -- Debug grid must render correctly from any camera angle (free camera), not just isometric.

### NEW Tickets Needed

- **NEW: Terrain Visual Configuration System** -- Data-driven system mapping each of the 10 terrain types to emissive color, emissive intensity, glow pattern (static/pulse/flicker), and bloom contribution. Loaded from configuration, not hardcoded. Feeds into per-instance emissive data in instance buffers.

- **NEW: LOD System (Terrain)** -- Distance-based LOD for terrain meshes on large maps. Chunk terrain into groups (16x16 or 32x32), generate simplified meshes for distant chunks. Required for 512x512 map performance. Canon-required.

- **NEW: LOD System (Buildings)** -- 2-3 LOD levels for building models. Full detail at close range, simplified at medium distance, billboard/imposter at far distance. Required for 512x512 map performance. Canon-required.

- **NEW: Camera Mode System** -- Free camera mode implementation with full orbit/pan/zoom/tilt. Isometric preset snap system with smooth transitions. Mode toggle between free and preset. Pitch/yaw limits and input mapping. (This may be absorbed into modifications to existing camera tickets 2-017 and CAMERA-3D-05, but the scope is large enough to warrant explicit tracking.)

- **NEW: Projection Type Decision Record** -- Formal decision on perspective vs orthographic vs hybrid projection. Must be resolved before camera, shader, edge detection, and picking work proceeds. Blocks multiple tickets.
