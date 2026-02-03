# Epic 2 Cross-Agent Discussion (3D)

**Epic:** Core Rendering Engine (3D)
**Created:** 2026-01-27
**Status:** In Progress

---

## Questions From Systems Architect

### For Graphics Engineer

**SA-Q1:** For the toon shader, what technique should we use?
- A) Discrete lighting bands (dot product thresholds)
- B) Toon ramp texture lookup
- C) Combination with edge detection

What provides the best balance of alien aesthetic and performance?

**SA-Q2:** Should we use orthographic or perspective projection?
- Orthographic: True isometric, no foreshortening, classic feel
- Perspective (narrow FOV): Slight depth perception, more modern

**SA-Q3:** For SDL_GPU shader compilation, what format should we target?
- SPIR-V for Vulkan backend?
- HLSL for D3D12 backend?
- Multiple backends with shader cross-compilation?

**SA-Q4:** Model instancing strategy for large tile counts:
- GPU instancing (one draw call, instance buffer)
- Batch geometry into mega-meshes per chunk
- Standard draw calls with frustum culling

What's the expected draw call budget?

**SA-Q5:** For the outline/silhouette effect in toon shading, which technique?
- Inverted hull (scale + backface render)
- Screen-space edge detection (post-process)
- Geometry shader edge extrusion

### For Game Designer

**SA-Q6:** With true 3D models, should buildings have idle animations?
- Animated smoke from power plants
- Glowing energy effects on conduits
- Activity indicators (beings moving inside?)

**SA-Q7:** For the isometric camera angle, classic games used different ratios:
- SimCity 2000: 2:1 diamond (26.57 degrees)
- Standard isometric: 30 degrees
- True isometric: 35.264 degrees

Which angle feels most appropriate for ZergCity's alien aesthetic?

**SA-Q8:** Camera rotation - the decision says "none initially, can add later." Under what conditions would we add rotation? This affects how we model buildings (all 4 sides textured, or just front-facing?).

**SA-Q9:** Day/night cycle - does it affect the toon shader? Should lighting direction change, or just color temperature/palette shift?

**SA-Q10:** For zoom levels, what's the minimum detail the player should see:
- At max zoom in: Individual being faces? Texture details?
- At max zoom out: Entire colony (128x128 tiles)? Multiple colonies?

---

## Questions From Graphics Engineer

### For Systems Architect

**GE-Q1:** Should TransformComponent replace PositionComponent, or should we have both (PositionComponent for grid logic, TransformComponent for rendering)?

**GE-Q2:** For glTF loading, should this be part of Epic 0 AssetManager, or a separate system in Epic 2?

**GE-Q3:** How should we handle the UI layer? Continue with SDL_Renderer for 2D UI overlaid on 3D scene, or move to GPU-based UI?

**GE-Q4:** What's the coordinate mapping? Proposed: grid_x -> world X, grid_y -> world Z (depth), elevation -> world Y (up).

**GE-Q5:** For multiplayer, cursors need to render in world space. Should cursor position be a 3D world point or screen-space with ray-cast on hover?

### For Game Designer

**GE-Q6:** Orthographic vs perspective projection? Orthographic gives true isometric, perspective gives slight depth. Which matches the vision?

**GE-Q7:** Should camera rotation (90-degree snaps) be in initial release, or deferred?

**GE-Q8:** Day/night cycle - with real-time shading, we can do actual lighting changes. Desired? Or stick with palette shift approach?

**GE-Q9:** Building construction animation - with 3D, we could animate the model (rise from ground, assembly). Or keep the "materializing" ghost effect?

**GE-Q10:** Shadows - real-time shadows are possible and add depth. Worth the performance cost?

---

## Questions From Game Designer

### For Systems Architect

**GD-Q1:** For the toon shader, should shader parameters (number of color bands, edge line width, shadow color) be configurable at runtime or baked into the shader? What is the interface between shader configuration and game systems?

**GD-Q2:** How does underground view work in 3D? Is it camera position change (move camera down), transparency on surface objects, or culling/layer system? What is the state machine?

**GD-Q3:** For model LOD (Level of Detail), is this managed per-entity via a LODComponent, or is it a global RenderingSystem policy based on camera distance? Who decides which LOD to display?

**GD-Q4:** 3D model animation (ambient motion, construction effects) - does this live in AnimationSystem or is it shader-driven? What is the boundary between skeletal/transform animation and visual effects?

**GD-Q5:** For the fixed isometric camera, should camera parameters (pitch angle, rotation) be truly constant or configurable-but-locked? Thinking about future rotation feature.

### For Graphics Engineer

**GD-Q6:** What toon shader technique will we use? Sobel edge detection + stepped lighting? Inverted hull for outlines? What does SDL_GPU support for custom fragment shaders?

**GD-Q7:** For model format (glTF), how do we specify toon shader materials? Custom material extensions? Texture-based ramps? UV2 for emission maps?

**GD-Q8:** What is the expected performance budget for toon shader? How many draw calls can we afford with outline rendering (often requires extra passes)?

**GD-Q9:** How do we handle transparency in 3D with toon shading? Construction ghost previews, underground view, selection overlays - these all need alpha and can break depth sorting.

**GD-Q10:** For emissive materials (energy conduits, powered indicators), does SDL_GPU support bloom/glow post-processing? Or do we fake it with additive geometry?

---

## Answers

### Systems Architect Answers

*(To be filled by Systems Architect agent)*

### Graphics Engineer Answers

*(To be filled by Graphics Engineer agent)*

### Game Designer Answers

*(To be filled by Game Designer agent)*

---

## Consensus Decisions

**Date:** 2026-01-27

### Technical Architecture

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Toon Shader Technique** | Stepped lighting + screen-space Sobel edges | Best balance of alien aesthetic and performance |
| **Projection Type** | Orthographic | Grid clarity, genre expectation, simpler math |
| **Shader Format** | HLSL source → SPIR-V/DXIL cross-compilation | SDL_GPU abstracts backend at runtime |
| **Instancing Strategy** | GPU instancing with instance buffers | 500-1000 draw call budget |
| **Outline Technique** | Screen-space edge detection (post-process) | Fixed 0.5-1ms cost, works with all geometry |

### Component Architecture

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **PositionComponent vs TransformComponent** | Keep both (separate concerns) | PositionComponent for game logic, TransformComponent for rendering |
| **glTF Loading** | Epic 0 AssetManager | Single asset loading source, follows existing pattern |
| **UI Layer** | SDL_Renderer for 2D UI | Keep separate from 3D pipeline, simpler |
| **Coordinate Mapping** | grid_x→X, grid_y→Z, elevation→Y | Standard 3D conventions confirmed |
| **Cursor Representation** | 3D world point with ray-cast | Shared reality in multiplayer |

### Camera & View

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Camera Angle** | **26.57° (SimCity 2000 2:1 diamond)** | Nostalgia, readability, we differentiate through art style |
| **Camera Rotation** | Defer to later phase | Model 4 sides from start, add rotation when tall buildings cause occlusion |
| **Camera Parameters** | Configurable-but-locked | Stored as values (not hardcoded) for future rotation |
| **Underground View** | Camera + layer-based culling | ViewMode state machine (Surface/Underground/Cutaway) |
| **LOD Management** | RenderingSystem policy | Based on camera distance, no LODComponent needed |

### Visual Style

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Day/Night Cycle** | Palette shift (not light direction) | Consistent shadows, glowing night colonies |
| **Building Animations** | Yes, purposeful | Pulsing nexuses, traveling conduit light, mechanical motion |
| **Construction Style** | Materializing ghost effect | Fits alien theme, readable progress |
| **Real-Time Shadows** | Yes, with quality tiers | Essential for depth in orthographic view |
| **Bloom/Emissive** | True post-process bloom (~0.5ms) | Energy conduits, powered indicators glow |

### Zoom & Detail

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Max Zoom In** | Structure detail, NO being faces | Colony focus, beings are ambient silhouettes |
| **Max Zoom Out** | 128-256 tiles visible | See domain + neighboring colonies |

### Animation Boundary

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **AnimationSystem** | Skeletal/transform animation (geometry changes) | Defer skeletal to later epic |
| **Shader Effects** | UV scrolling, emission pulsing, color shifts | Appearance changes without geometry |

### Shader Configuration

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **ToonShaderConfig** | Runtime configurable singleton resource | Tuning, day/night, accessibility |

### Transparency Handling

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Render Order** | Opaques → Edge post-process → Sorted transparents | Edges on opaques only |

---

## Key Metric Targets

| Metric | Target |
|--------|--------|
| Frame Rate | 60 FPS |
| Render Budget | 8ms |
| Draw Calls | 500-1000 |
| Visible Triangles | <100k |
| Edge Post-Process | 0.5-1ms |
| Bloom Post-Process | 0.5ms |
