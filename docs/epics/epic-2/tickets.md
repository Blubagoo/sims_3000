# Epic 2: Core Rendering Engine (3D) - Tickets

**Epic Goal:** Establish 3D visual foundation with SDL_GPU, toon shading, bioluminescent art direction, and free camera with isometric presets
**Created:** 2026-01-27
**Canon Version:** 0.13.0

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-27 | v0.4.0 | Initial ticket creation |
| 2026-01-28 | v0.4.0 → v0.5.0 | Camera changed from fixed isometric to free camera with presets; projection changed from orthographic to perspective (~35° FOV); bioluminescent art direction (dark base + glow accents); configurable map sizes (128/256/512); 10 terrain types with distinct glow properties; bloom elevated to mandatory; emissive system expanded; 5 new tickets (2-046 through 2-050) |
| 2026-01-29 | canon-verification (v0.5.0 → v0.13.0) | No changes required |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. No changes required. IGridOverlay (v0.10.0) is implemented by simulation systems (Epic 10) and consumed by UI (Epic 12); Epic 2 correctly defines DataOverlay layer for this integration. UI animation specs (v0.13.0) are handled by SDL_Renderer in Epic 12, separate from 3D pipeline.

---

## Summary

Epic 2 establishes the complete 3D visual foundation for ZergCity. This epic implements:

- **SDL_GPU Integration:** Modern cross-platform GPU abstraction for 3D rendering
- **Toon Shader Pipeline:** Stepped lighting with screen-space edge detection for the distinctive alien aesthetic
- **Bioluminescent Art Direction:** Dark base environment with multi-color emissive glow accents
- **Free Camera with Isometric Presets:** Perspective projection with orbit/tilt free camera and N/E/S/W snap presets
- **3D Model Pipeline:** glTF model loading and GPU mesh representation
- **Depth Buffer:** Automatic occlusion handling (major simplification from 2D)
- **Camera Controls:** Pan, zoom, orbit, tilt, preset snap, and viewport bounds with smooth interpolation
- **Render Layers:** Semantic layer system for terrain, buildings, effects, and overlays

**Key Technical Decisions (from Consensus):**
- Perspective projection (~35° FOV) for natural free camera
- Free camera with four isometric preset snap views (N/E/S/W)
- HLSL source compiled to SPIR-V/DXIL via cross-compilation
- GPU instancing with 500-1000 draw call budget
- Screen-space Sobel edge detection for outlines
- Separate PositionComponent (game logic) and TransformComponent (rendering)
- SDL_Renderer for 2D UI overlay (separate from 3D pipeline)
- Bloom is mandatory for bioluminescent art direction
- Configurable map sizes (128/256/512) with spatial partitioning for large maps

**Target Metrics:**
- 60 FPS at <8ms render budget
- 500-1000 draw calls
- <100k visible triangles
- 0.5-1ms edge post-process
- 0.5ms bloom post-process
- <512MB GPU memory (VRAM) for minimum spec hardware
- <1GB GPU memory for recommended spec hardware

**GPU Memory Budget Breakdown:**
| Resource Type | Min Spec | Recommended |
|---------------|----------|-------------|
| Model buffers (geometry) | 64MB | 128MB |
| Textures (diffuse, normal) | 256MB | 512MB |
| Render targets (color, depth, post-process) | 64MB | 128MB |
| Shader programs | 16MB | 32MB |
| Instance buffers | 32MB | 64MB |
| Headroom | 80MB | 136MB |
| **Total** | **512MB** | **1GB** |

**Minimum GPU Spec:** Intel UHD 620 (shared memory) or equivalent discrete with 1GB VRAM

---

## Ticket List

### Group A: GPU Foundation

#### 2-001: SDL_GPU Device Creation
**Assigned:** Graphics Engineer
**Depends On:** Epic 0 (Application/Window)
**Blocks:** 2-002, 2-003, 2-004

**Description:**
Initialize SDL_GPU device with appropriate shader format support. Detect available backends (D3D12, Vulkan, Metal) and configure debug layers for development builds. Handle device creation failure gracefully with clear error messages.

**Acceptance Criteria:**
- [ ] SDL_GPUDevice created successfully on Windows, macOS, and Linux
- [ ] Shader format support detected (SPIR-V, DXIL)
- [ ] Debug layers enabled in development builds
- [ ] Selected backend and capabilities logged at startup
- [ ] Graceful fallback/error message if device creation fails
- [ ] Device wrapper class encapsulates SDL_GPU specifics

---

#### 2-002: Window Integration and Swap Chain
**Assigned:** Graphics Engineer
**Depends On:** 2-001
**Blocks:** 2-018

**Description:**
Claim window for GPU device and configure swap chain for presentation. Handle window resize events (recreate swap chain) and support fullscreen toggle.

**Acceptance Criteria:**
- [ ] Window claimed for GPU device via SDL_ClaimWindowForGPUDevice
- [ ] Swap chain configured with vsync option
- [ ] Window resize triggers swap chain recreation
- [ ] Fullscreen toggle works without crash
- [ ] Present mode configurable (immediate/vsync/mailbox)

---

#### 2-003: GPU Resource Management Infrastructure
**Assigned:** Graphics Engineer
**Depends On:** 2-001
**Blocks:** 2-009, 2-010

**Description:**
Create infrastructure for managing GPU resources: buffer pools for uniform data, texture loading from PNG, sampler creation, and frame resource management for double/triple buffering.

**Acceptance Criteria:**
- [ ] Uniform buffer pool with efficient allocation
- [ ] PNG texture loading via stb_image
- [ ] Sampler creation (linear and nearest filtering)
- [ ] Double-buffering frame resources to avoid GPU stalls
- [ ] Resource destruction tracked and cleaned up properly
- [ ] No GPU memory leaks after extended play

---

#### 2-004: Shader Compilation Pipeline
**Assigned:** Graphics Engineer
**Depends On:** 2-001
**Blocks:** 2-005, 2-007

**Description:**
Establish shader compilation workflow using HLSL as source, cross-compiled to SPIR-V and DXIL. Support shader hot-reload for development iteration. Provide clear error reporting for shader compilation failures.

**Shader Fallback Strategy:**
Runtime shader loading may fail due to corrupted cache, driver incompatibility, or missing files. The system must handle this gracefully:

1. **Embedded Fallback Shaders:**
   - Compile minimal solid-color shaders into the executable
   - These are the "cannot fail" baseline (magenta solid color)
   - Used when all other shader loading fails

2. **Loading Priority:**
   - Try: User shader cache (if hot-reload enabled)
   - Then: Pre-compiled shaders from assets folder
   - Then: Embedded fallback shaders
   - Log warning when falling back

3. **Cache Corruption Handling:**
   - Validate shader cache on load (version check, size check)
   - If validation fails, delete cache and use asset shaders
   - Never crash due to shader issues

**Acceptance Criteria:**
- [ ] HLSL shaders compile to SPIR-V (Vulkan/Metal)
- [ ] HLSL shaders compile to DXIL (D3D12)
- [ ] Offline compilation integrated into build system
- [ ] Shader hot-reload works in development builds
- [ ] Compilation errors include file, line, and clear message
- [ ] Pre-compiled shaders stored in assets folder
- [ ] Embedded fallback shaders compiled into executable
- [ ] Graceful fallback when shader loading fails at runtime
- [ ] Cache validation prevents corrupted shader crashes
- [ ] Warning logged when using fallback shaders

---

### Group B: Toon Shader System

#### 2-005: Toon Shader Implementation (Stepped Lighting)
**Assigned:** Graphics Engineer
**Depends On:** 2-004
**Blocks:** 2-006, 2-007

**Description:**
Implement the core toon shader with stepped/banded lighting. Vertex shader performs standard MVP transformation. Fragment shader quantizes lighting into discrete bands (3-4 bands: lit, mid, shadow, deep shadow). Shadow colors shift toward purple/teal per the alien aesthetic. Fragment shader includes an emissive term that is added to final output unaffected by lighting bands. World-space directional light (fixed alien sun direction, not camera-relative). Ambient term ensures minimum readability in dark bioluminescent environment.

**Acceptance Criteria:**
- [ ] Vertex shader: position, normal, UV passthrough with MVP transform
- [ ] Fragment shader: 3-4 discrete lighting bands
- [ ] Shadow color shifts toward deep purple (#2A1B3D) and teal as canon-required
- [ ] Single directional light (sun) in world-space (not camera-relative)
- [ ] Ambient color contribution configurable (~0.05-0.1 for minimum readability)
- [ ] Emissive term in fragment shader, added to output unaffected by lighting bands
- [ ] Bioluminescent palette tuning: dark base tones with glow accents
- [ ] No specular highlights (or minimal for metal/glass)
- [ ] Toon effect reads clearly at all zoom levels

> **Revised 2026-01-28:** Added emissive term in fragment shader, world-space directional light (not camera-relative), ambient term for minimum readability (~0.05-0.1), shadow colors canon-required deep purple/teal shift, bioluminescent palette tuning for dark base tones (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-006: Screen-Space Edge Detection (Post-Process)
**Assigned:** Graphics Engineer
**Depends On:** 2-005, 2-018
**Blocks:** 2-019

**Description:**
Implement screen-space Sobel edge detection as post-process for cartoon outlines. This gives fixed 0.5-1ms cost regardless of geometry complexity and works with all model types. Must work correctly with perspective projection (non-linear depth buffer). Normal-based edges should be the primary detection signal, with depth used as a secondary signal after linearization for perspective mode.

**Acceptance Criteria:**
- [ ] Sobel edge detection on depth and/or normal buffers
- [ ] Normal-based edges as primary signal
- [ ] Depth linearization for perspective projection (non-linear depth)
- [ ] Outline color configurable (default: dark purple)
- [ ] Outline thickness configurable (screen-space pixels)
- [ ] Edges render only on opaque geometry (before transparents)
- [ ] Performance: <1ms at 1080p
- [ ] Outlines remain readable at all zoom levels
- [ ] Validated at multiple camera angles (preset and free camera)

> **Revised 2026-01-28:** Added perspective projection support (non-linear depth handling), normal-based edges as primary signal, depth linearization, validation at multiple camera angles (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-007: Graphics Pipeline Creation
**Assigned:** Graphics Engineer
**Depends On:** 2-004, 2-005
**Blocks:** 2-010, 2-018

**Description:**
Create SDL_GPUGraphicsPipeline for toon rendering. Configure vertex input layout, enable depth test/write, set blend states, and cull back faces. Validate with perspective projection. Consider MRT (multiple render targets) for emissive channel if bloom uses a separate render target.

**Acceptance Criteria:**
- [ ] Vertex input layout matches model data (position, normal, UV)
- [ ] Depth test enabled (LESS compare)
- [ ] Depth write enabled for opaque pass
- [ ] Back-face culling enabled
- [ ] Blend state configured for opaque and transparent modes
- [ ] Pipeline validated with perspective projection
- [ ] MRT consideration documented for emissive/bloom separate target
- [ ] Pipeline creation does not fail on any target platform

> **Revised 2026-01-28:** Added perspective projection validation and MRT consideration for emissive channel (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-008: ToonShaderConfig Runtime Resource
**Assigned:** Graphics Engineer
**Depends On:** 2-005
**Blocks:** 2-019

**Description:**
Create a runtime-configurable singleton resource for toon shader parameters. Allows tuning band thresholds, colors, edge thickness, and shadow color without recompiling shaders. Supports day/night palette shifts and accessibility options. Includes bioluminescent rendering parameters for bloom and emissive control.

**Acceptance Criteria:**
- [ ] ToonShaderConfig struct with all tunable parameters
- [ ] Band count and thresholds adjustable
- [ ] Shadow color (purple shift amount) adjustable
- [ ] Edge line width adjustable
- [ ] Bloom threshold parameter
- [ ] Bloom intensity parameter
- [ ] Emissive multiplier parameter
- [ ] Per-terrain-type emissive color presets (10 terrain types)
- [ ] Ambient light level parameter (~0.05-0.1)
- [ ] Changes take effect immediately (no restart)
- [ ] Default values match Game Designer specifications

> **Revised 2026-01-28:** Added bloom threshold, bloom intensity, emissive multiplier, per-terrain-type emissive color presets, and ambient light level parameters for bioluminescent art direction (trigger: canon-update v0.4.0→v0.5.0)

---

### Group C: 3D Model Pipeline

#### 2-009: glTF Model Loading
**Assigned:** Graphics Engineer
**Depends On:** 2-003, Epic 0 (AssetManager)
**Blocks:** 2-010, 2-011

**Description:**
Integrate cgltf library for glTF 2.0 model loading. Parse binary (.glb) and JSON (.gltf) formats. Extract mesh data (positions, normals, UVs, indices), material data (base color texture, emissive texture, emissive factor), and handle multiple meshes per model. Register loader with Epic 0 AssetManager.

**Acceptance Criteria:**
- [ ] cgltf integrated as single-header library
- [ ] Load .glb binary format
- [ ] Load .gltf JSON format with external buffers
- [ ] Extract vertex positions, normals, UVs, indices
- [ ] Extract diffuse/albedo texture references
- [ ] Extract emissive texture and emissive factor from glTF material
- [ ] Handle models with multiple meshes/primitives
- [ ] Integrated with AssetManager for caching and reference counting
- [ ] Clear error on malformed or missing models

> **Revised 2026-01-28:** Added extraction of emissive texture and emissive factor from glTF material (already in glTF 2.0 spec) for bioluminescent rendering (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-010: GPU Mesh Representation
**Assigned:** Graphics Engineer
**Depends On:** 2-003, 2-009
**Blocks:** 2-011, 2-012

**Description:**
Create GPU-side mesh representation from loaded glTF data. Create vertex and index buffers, store bounding box for culling, and manage texture references. Material struct must include emissive texture reference and emissive color/factor for bioluminescent rendering.

**Acceptance Criteria:**
- [ ] GPUMesh struct with vertex buffer, index buffer, index count
- [ ] Diffuse texture reference per mesh
- [ ] Emissive texture reference per mesh
- [ ] Emissive color/factor in Material struct
- [ ] AABB bounding box calculated and stored
- [ ] Buffers created once per unique model (not per instance)
- [ ] Proper cleanup on model unload
- [ ] ModelAsset aggregates multiple GPUMesh for multi-mesh models

> **Revised 2026-01-28:** Added emissive texture reference and emissive color/factor to Material struct for bioluminescent rendering (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-011: Render Command Recording
**Assigned:** Graphics Engineer
**Depends On:** 2-010
**Blocks:** 2-018

**Description:**
Implement render command recording for drawing 3D models. Bind vertex/index buffers, upload model matrix uniform, and issue draw calls.

**Acceptance Criteria:**
- [ ] Function to record draw commands for a GPUMesh
- [ ] Model matrix uploaded to uniform buffer per draw
- [ ] Texture binding before draw call
- [ ] Draw indexed primitives with correct index count
- [ ] Support for multiple meshes per model

---

#### 2-012: GPU Instancing for Repeated Models
**Assigned:** Graphics Engineer
**Depends On:** 2-010
**Blocks:** 2-018

**Description:**
Implement GPU instancing for rendering many instances of the same model with a single draw call. Critical for terrain tiles and common buildings. Instance buffer contains per-instance transforms, tint colors, and emissive intensity/color for powered/unpowered state. Budget reassessment needed for 512x512 maps. Consider chunked instancing to manage large instance counts.

**Acceptance Criteria:**
- [ ] Instance buffer with per-instance model matrices
- [ ] Per-instance tint color support
- [ ] Per-instance emissive intensity/color for powered/unpowered state
- [ ] Single draw call renders N instances
- [ ] Instancing used for terrain tiles automatically
- [ ] Instancing used for buildings with same model
- [ ] Draw call count reduced by 10x+ for repeated geometry
- [ ] Performance budget validated for 512x512 maps (up to 262k tiles)
- [ ] Chunked instancing considered for large instance counts

> **Revised 2026-01-28:** Added per-instance emissive intensity/color for powered/unpowered bioluminescent state. Budget reassessment note for 512x512 maps. Consider chunked instancing for large maps (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-013: Placeholder Model for Missing Assets
**Assigned:** Graphics Engineer
**Depends On:** 2-009, 2-010
**Blocks:** None

**Description:**
Create a magenta placeholder cube/model that renders when requested model is not found. Ensures missing assets are visible during development rather than invisible or crashing.

**Acceptance Criteria:**
- [ ] Magenta cube model created at startup
- [ ] Renders when AssetManager returns missing model
- [ ] Does not crash game
- [ ] Warning logged when placeholder used
- [ ] Easily identifiable in game view (bright magenta)

---

### Group D: Depth Buffer and Render Pass

#### 2-014: Depth Buffer Setup
**Assigned:** Graphics Engineer
**Depends On:** 2-002
**Blocks:** 2-015, 2-018

**Description:**
Create depth texture for automatic occlusion handling. Configure depth format (D32_FLOAT or D24_UNORM_S8_UINT) and ensure proper recreation on window resize.

**Acceptance Criteria:**
- [ ] Depth texture created at window resolution
- [ ] D32_FLOAT format (or D24 with stencil if needed)
- [ ] Depth texture recreated on window resize
- [ ] Depth cleared to 1.0 at frame start
- [ ] No depth fighting/z-fighting visible in normal use

---

#### 2-015: Depth State Configuration
**Assigned:** Graphics Engineer
**Depends On:** 2-014
**Blocks:** 2-007

**Description:**
Configure depth state in graphics pipeline: enable depth test with LESS comparison, enable depth write for opaque objects.

**Acceptance Criteria:**
- [ ] Depth test enabled
- [ ] Depth compare operation: LESS
- [ ] Depth write enabled for opaque pass
- [ ] Depth write disabled for transparent pass (separate state)
- [ ] Near objects correctly occlude far objects

---

#### 2-016: Transparent Object Handling
**Assigned:** Graphics Engineer
**Depends On:** 2-015, 2-018
**Blocks:** 2-019

**Description:**
Handle transparent objects (construction ghosts, selection effects, underground view) correctly with depth buffer. Render opaques first with depth write, then transparents back-to-front without depth write.

**Acceptance Criteria:**
- [ ] Transparent pass renders after opaque pass
- [ ] Transparents sorted back-to-front
- [ ] Depth test enabled but depth write disabled for transparents
- [ ] Construction preview ghosts render correctly
- [ ] Selection overlays render correctly
- [ ] No depth sorting artifacts for common cases

---

### Group E: Camera System

#### 2-017: Camera State and Configuration
**Assigned:** Systems Architect
**Depends On:** None
**Blocks:** 2-020, 2-021, 2-022, 2-023, 2-046, 2-047, 2-048

**Description:**
Define camera state structure for free camera with isometric presets. CameraState stores focus_point, distance, pitch (variable, clamped 15-80 degrees), yaw (variable), camera_mode enum, and transition_state for smooth mode switching. Camera supports four isometric preset views and full free orbit/tilt.

**Camera Mode Enum:**
- `Free` — full orbit/pan/zoom/tilt
- `Preset_N` — yaw 45°, pitch ~35.264°
- `Preset_E` — yaw 135°, pitch ~35.264°
- `Preset_S` — yaw 225°, pitch ~35.264°
- `Preset_W` — yaw 315°, pitch ~35.264°
- `Animating` — transitioning between modes/presets

**Preset Definitions:**
| Preset | Yaw | Pitch |
|--------|-----|-------|
| North | 45° | ~35.264° |
| East | 135° | ~35.264° |
| South | 225° | ~35.264° |
| West | 315° | ~35.264° |

**Acceptance Criteria:**
- [ ] CameraState struct with focus_point, distance, pitch, yaw
- [ ] Pitch variable and clamped: 15°-80°
- [ ] Yaw variable: 0°-360° (wrapping)
- [ ] camera_mode enum: Free, Preset_N, Preset_E, Preset_S, Preset_W, Animating
- [ ] transition_state for smooth animated transitions between modes
- [ ] Zoom controls distance (min 5 units, max 100 units)
- [ ] Camera parameters stored in config (not magic numbers)
- [ ] Default mode: Preset_N (yaw 45°, pitch ~35.264°)
- [ ] Animation state for smooth transitions

> **Revised 2026-01-28:** MAJOR REWORK — replaced fixed isometric parameters with free camera state. Added camera_mode enum (Free, Preset_N/E/S/W, Animating), variable pitch (15°-80°) and yaw, preset definitions at cardinal directions, transition_state. Removed fixed pitch 26.57° and fixed yaw 45°. Default mode is Preset_N (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-018: Main Render Pass Structure
**Assigned:** Graphics Engineer
**Depends On:** 2-002, 2-007, 2-011, 2-014
**Blocks:** 2-006, 2-019

**Description:**
Implement the main render pass that brings everything together: acquire swap chain, clear buffers, render layers in order, present frame. Integrate with camera matrices. Bloom is a required pass in the pipeline (not a future addition).

**Acceptance Criteria:**
- [ ] Acquire command buffer each frame
- [ ] Acquire swap chain texture
- [ ] Clear color buffer to dark bioluminescent base (deep blue-black, e.g., {0.02, 0.02, 0.05, 1.0})
- [ ] Clear depth buffer (1.0)
- [ ] Begin render pass with color and depth targets
- [ ] Bind camera uniforms (view/projection matrices)
- [ ] Render terrain layer
- [ ] Render buildings layer
- [ ] Render effects layer
- [ ] Bloom pass integrated as required pipeline stage
- [ ] End render pass
- [ ] Submit command buffer
- [ ] Present frame

> **Revised 2026-01-28:** Changed clear color from sky color to dark bioluminescent base (deep blue-black {0.02, 0.02, 0.05, 1.0}). Bloom is now a required pass in the pipeline, not a future addition (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-019: Complete Render Frame Flow
**Assigned:** Graphics Engineer
**Depends On:** 2-006, 2-016, 2-018
**Blocks:** 2-028

**Description:**
Implement complete per-frame render flow including edge post-process and mandatory bloom. Pass order: scene (opaques) -> edge detection -> sorted transparents -> bloom -> UI overlay. Bloom is mandatory and not deferrable — required for bioluminescent art direction.

**SDL_Renderer + SDL_GPU Coexistence:**
SDL3 supports using both SDL_GPU (3D) and SDL_Renderer (2D) on the same window. The integration approach:

1. **Render Target Strategy:**
   - SDL_GPU renders 3D scene to the swap chain texture
   - After SDL_GPU submit, SDL_Renderer draws 2D UI on top
   - Both target the same window, no intermediate texture needed

2. **Frame Structure:**
   ```
   Begin Frame:
     SDL_AcquireGPUCommandBuffer()
     SDL_WaitAndAcquireGPUSwapchainTexture()

   3D Pass (SDL_GPU):
     BeginRenderPass -> opaques -> edges -> transparents -> bloom -> EndRenderPass
     SDL_SubmitGPUCommandBuffer()

   2D Pass (SDL_Renderer):
     SDL_RenderClear() with no-op (preserve 3D)
     Draw UI elements via SDL_Renderer
     SDL_RenderPresent()
   ```

3. **Key Constraints:**
   - SDL_GPU command buffer MUST be submitted before SDL_Renderer draws
   - Do NOT clear in SDL_Renderer (would erase 3D scene)
   - SDL_Renderer uses software or GPU-accelerated 2D, not SDL_GPU

4. **Alternative (if coexistence is problematic):**
   - Render 3D to offscreen texture via SDL_GPU
   - Composite final frame with SDL_Renderer (blit 3D + draw UI)
   - Slightly higher overhead but more robust

**Acceptance Criteria:**
- [ ] Opaque geometry rendered first
- [ ] Edge detection post-process applied to opaques
- [ ] Transparent geometry rendered in sorted order
- [ ] Bloom post-process applied (mandatory, 0.5ms budget)
- [ ] Bloom is NOT deferrable — required for bioluminescent art direction
- [ ] SDL_GPU submitted before SDL_Renderer draws
- [ ] UI overlay rendered last via SDL_Renderer
- [ ] 3D scene not erased by UI pass
- [ ] Total render time <8ms at target spec
- [ ] Document integration point for Epic 12 (UISystem)

> **Revised 2026-01-28:** Formalized bloom as mandatory pass (not deferrable). Pass order: scene → edges → transparents → bloom → UI. Required for bioluminescent art direction (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-020: View Matrix Calculation
**Assigned:** Systems Architect
**Depends On:** 2-017
**Blocks:** 2-022, 2-024

**Description:**
Calculate view matrix from camera state. Camera position derived from focus point + distance + variable pitch/yaw angles. Use standard lookAt function. Must produce correct results at all pitch/yaw combinations, including isometric presets and arbitrary free camera angles.

**Acceptance Criteria:**
- [ ] Camera position calculated from spherical coordinates (variable pitch/yaw)
- [ ] View matrix calculated via lookAt(position, target, up)
- [ ] Up vector is (0, 1, 0)
- [ ] View matrix correct at all pitch/yaw angles
- [ ] View matrix tested at isometric presets AND arbitrary free camera angles
- [ ] View matrix updated when focus point, distance, pitch, or yaw changes
- [ ] Edge case handled: near-horizontal pitch (gimbal lock avoidance)

> **Revised 2026-01-28:** Replaced references to fixed pitch/yaw with variable values from camera state. Added testing at both preset and free camera angles. Added gimbal lock avoidance for near-horizontal pitch (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-021: Projection Matrix Calculation (Perspective)
**Assigned:** Systems Architect
**Depends On:** 2-017
**Blocks:** 2-022, 2-024

**Description:**
Calculate perspective projection matrix with ~35° vertical FOV for natural free camera feel. FOV is configurable with 35° as default. Near/far planes configured appropriately. Reference: plans/decisions/epic-2-projection-type.md.

**Acceptance Criteria:**
- [ ] Perspective projection (not orthographic)
- [ ] Vertical FOV ~35° (configurable, default 35°)
- [ ] Aspect ratio maintained from window dimensions
- [ ] Near plane: 0.1, Far plane: 1000.0
- [ ] Perspective divide correct
- [ ] Minimal foreshortening at preset angles (~35.264° pitch)
- [ ] FOV configurable via ToonShaderConfig or camera config

> **Revised 2026-01-28:** MAJOR CHANGE — replaced orthographic projection with perspective projection (~35° FOV). Removed "parallel lines stay parallel" and "grid cells appear as 2:1 diamonds." Added configurable FOV, perspective divide, minimal foreshortening at presets. Reference: plans/decisions/epic-2-projection-type.md (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-022: View-Projection Matrix Integration
**Assigned:** Systems Architect
**Depends On:** 2-020, 2-021
**Blocks:** 2-018, 2-024

**Description:**
Combine view and projection matrices and upload to GPU as uniform. Handle aspect ratio changes on window resize.

**Acceptance Criteria:**
- [ ] Combined viewProjection matrix calculated
- [ ] Matrix uploaded to camera uniform buffer
- [ ] Aspect ratio updated on window resize
- [ ] Projection recalculated on resize
- [ ] No visual distortion after resize

---

#### 2-023: Pan Controls (Focus Point Movement)
**Assigned:** Systems Architect
**Depends On:** 2-017, Epic 0 (InputSystem)
**Blocks:** 2-025

**Description:**
Implement camera panning via keyboard (WASD/arrows), right mouse drag, and edge-of-screen scrolling. Pan direction is camera-orientation-relative: when camera is rotated, WASD moves relative to current view direction projected onto the ground plane. Pan speed scales with zoom level. Smooth movement with momentum (ease-out on stop).

**Acceptance Criteria:**
- [ ] WASD and arrow keys pan the camera
- [ ] Right mouse button drag pans the camera (not middle mouse — middle mouse is orbit)
- [ ] Pan direction is camera-orientation-relative (projected onto ground plane)
- [ ] Pan speed scales with zoom (faster when zoomed out)
- [ ] Smooth momentum on key release (ease-out)
- [ ] Pan feels like "flying over" the colony
- [ ] Edge-of-screen scrolling (toggleable)

> **Revised 2026-01-28:** Pan direction changed to camera-orientation-relative (not world-axis-aligned). Right mouse drag for pan (middle mouse reserved for orbit). Added edge-of-screen scrolling (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-024: Zoom Controls (Camera Distance)
**Assigned:** Systems Architect
**Depends On:** 2-020, 2-021, Epic 0 (InputSystem)
**Blocks:** 2-025

**Description:**
Implement zoom via mouse wheel adjusting camera distance (perspective mode — adjusts distance, not view volume width). Zoom toward cursor position (adjust focus point to keep cursor world-point stable). Zoom range should be map-size-aware (wider range for 512x512 maps). Smooth interpolation between zoom levels. Zoom center-on-cursor math updated for perspective projection.

**Acceptance Criteria:**
- [ ] Mouse wheel adjusts camera distance
- [ ] Zoom range: 5 units (close) to 100 units (far), map-size-aware
- [ ] Zoom centers on cursor position (non-negotiable)
- [ ] Zoom center-on-cursor math correct for perspective projection
- [ ] Smooth zoom interpolation
- [ ] Zoom speed feels natural (perceptual consistency)
- [ ] Zoom limits with soft boundaries (decelerate at edges)
- [ ] Wider zoom range available for larger map sizes (512x512)

> **Revised 2026-01-28:** Zoom works with perspective projection (adjusts camera distance, not view volume width). Zoom range is map-size-aware. Center-on-cursor math updated for perspective (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-025: Viewport Bounds and Map Clamping
**Assigned:** Systems Architect
**Depends On:** 2-023, 2-024, 2-046
**Blocks:** 2-026

**Description:**
Calculate visible world area from camera frustum. With perspective projection, the visible area is a frustum footprint (trapezoid), not an axis-aligned rectangle. Clamp focus point to map boundaries with gentle deceleration (not hard stop). Map boundary clamping parameterized by configurable map size (128/256/512). Provide visible tile range for culling queries.

**Acceptance Criteria:**
- [ ] Visible world bounds calculated from frustum (trapezoid for perspective)
- [ ] Focus point clamped to map boundaries
- [ ] Map boundary size configurable (128/256/512, default 256x256 medium)
- [ ] Soft boundaries: decelerate near edge, not hard stop
- [ ] getVisibleTileRange() returns GridRect for culling
- [ ] Bounds update when zoom or camera angle changes
- [ ] Colony feels at "edge of territory" not invisible wall

> **Revised 2026-01-28:** Visible area is frustum footprint (trapezoid for perspective), not axis-aligned rectangle. Map boundary clamping parameterized by configurable map size (128/256/512). Changed default from 128x128 to configurable (default 256x256 medium) (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-026: Frustum Culling
**Assigned:** Systems Architect
**Depends On:** 2-025, 2-022
**Blocks:** 2-049, 2-050

**Description:**
Extract frustum planes from view-projection matrix. Cull entities with bounding box/sphere outside frustum. Mandatory spatial partitioning (2D grid hash, 16x16 or 32x32 cells) for 512x512 map performance. Culling must work correctly at all camera angles. This is a REQUIREMENT, not just an optimization.

**Acceptance Criteria:**
- [ ] Frustum planes extracted from VP matrix
- [ ] Entities outside frustum not submitted for rendering
- [ ] Culling uses entity bounding box (from model)
- [ ] Mandatory spatial partitioning for large maps (see 2-050)
- [ ] Culling works correctly at all camera angles (preset and free)
- [ ] Significant draw call reduction when partially zoomed in
- [ ] No visible popping (conservative culling)
- [ ] Culling contributes to staying under draw call budget
- [ ] Performance critical for 512x512 maps

> **Revised 2026-01-28:** Elevated from optimization to REQUIREMENT. Added mandatory spatial partitioning (2D grid hash) for 512x512 maps. Culling must work at all camera angles. Performance critical for large maps (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-027: Camera Animation
**Assigned:** Systems Architect
**Depends On:** 2-017
**Blocks:** 2-047

**Description:**
Implement smooth camera transitions: animate to target position ("go to" feature), camera shake for disasters, and preset snap transitions. Preset snap animates pitch/yaw/distance to target preset angles (0.3-0.5s ease-in-out). Interpolate all camera params (focus, distance, pitch, yaw). Snap animation interrupted by player input.

**Acceptance Criteria:**
- [ ] animateTo(targetPosition, duration) function
- [ ] Easing functions: linear, ease-in-out
- [ ] Preset snap transitions: smooth animation of pitch/yaw/distance (0.3-0.5s)
- [ ] Interpolate all camera params (focus, distance, pitch, yaw)
- [ ] Animation interrupted by player pan/zoom/orbit input
- [ ] Camera shake with intensity and duration
- [ ] Smooth blend from current state to target

> **Revised 2026-01-28:** Added preset snap transitions (0.3-0.5s ease-in-out), interpolation of all camera params (focus, distance, pitch, yaw), snap animation interrupted by player input (trigger: canon-update v0.4.0→v0.5.0)

---

### Group F: Coordinate Conversion

#### 2-028: World-to-Screen Transformation
**Assigned:** Systems Architect
**Depends On:** 2-019, 2-022
**Blocks:** None

**Description:**
Transform world position to screen coordinates: apply model matrix, view matrix, projection matrix, perspective divide, and viewport transform.

**Acceptance Criteria:**
- [ ] Function: worldToScreen(Vec3 worldPos) -> Vec2 screenPos
- [ ] Accounts for viewport offset and size
- [ ] Returns valid screen coordinates for visible objects
- [ ] Used for UI elements anchored to world positions
- [ ] Handles off-screen positions gracefully

---

#### 2-029: Screen-to-World Ray Casting
**Assigned:** Systems Architect
**Depends On:** 2-022
**Blocks:** 2-030

**Description:**
Unproject screen coordinates to world ray using perspective projection. Rays diverge from camera position through screen point (not parallel rays as in orthographic). Intersect ray with ground plane (y=0 or terrain height). Return world position and hit information. Must handle all camera angles correctly.

**Acceptance Criteria:**
- [ ] Function: screenToWorldRay(Vec2 screenPos) -> Ray
- [ ] Ray constructed from inverse view-projection matrix
- [ ] Divergent rays from camera position through screen point (perspective)
- [ ] Function: rayGroundIntersection(Ray, height) -> Vec3
- [ ] Works correctly with perspective projection at all camera angles
- [ ] Numerical stability for near-horizontal pitch (near-parallel ray-ground intersection)
- [ ] Handles cases where ray is parallel to ground

> **Revised 2026-01-28:** Updated ray casting for perspective projection (divergent rays, not parallel). Removed orthographic-specific notes. Added numerical stability for near-horizontal pitch. Must handle all camera angles (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-030: Tile Picking (3D Ray Cast)
**Assigned:** Systems Architect
**Depends On:** 2-029
**Blocks:** None

**Description:**
Cast ray from camera through cursor position, intersect with terrain/ground plane, return grid tile coordinates. Used for all mouse-based interaction. Must work correctly at all camera angles including extreme tilt.

**Acceptance Criteria:**
- [ ] Function: pickTile(Vec2 screenPos) -> GridPosition
- [ ] Returns correct tile at all zoom levels
- [ ] Accounts for elevation (terrain height)
- [ ] Cursor position maps to expected tile
- [ ] Numerical stability guard for near-parallel ray-ground intersection
- [ ] Tested at preset angles AND arbitrary free camera angles
- [ ] Future: extend to pick buildings by bounding box

> **Revised 2026-01-28:** Must work correctly at all camera angles including extreme tilt. Added numerical stability guard for near-parallel ray-ground intersection. Test at preset AND free camera angles (trigger: canon-update v0.4.0→v0.5.0)

---

### Group G: Components

#### 2-031: RenderComponent Definition
**Assigned:** Systems Architect
**Depends On:** None
**Blocks:** 2-033

**Description:**
Define RenderComponent (or ModelComponent) for entities that should be rendered. References model asset, material, render layer, visibility flag, tint color, and per-instance bioluminescent glow state.

**Acceptance Criteria:**
- [ ] AssetHandle<Model> model reference
- [ ] Material/texture reference
- [ ] RenderLayer assignment (enum)
- [ ] Visibility flag (bool)
- [ ] Tint color (Vec4, default white)
- [ ] Scale factor for size variety
- [ ] emissive_intensity (float, 0.0-1.0) for bioluminescent glow state
- [ ] emissive_color (Vec3) for per-instance glow color
- [ ] Powered buildings: emissive_intensity > 0; unpowered = 0

> **Revised 2026-01-28:** Added emissive_intensity (float, 0.0-1.0) and emissive_color (Vec3) for per-instance bioluminescent glow state. Powered buildings glow; unpowered do not (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-032: TransformComponent Definition
**Assigned:** Systems Architect
**Depends On:** None
**Blocks:** 2-033

**Description:**
Define TransformComponent for 3D rendering transforms. Contains position (Vec3), rotation (quaternion — required for free camera where models are viewed from all angles), scale (Vec3), and cached model matrix.

**Acceptance Criteria:**
- [ ] Position: Vec3 (world space, floats)
- [ ] Rotation: Quaternion (REQUIRED — no Euler alternative for free camera)
- [ ] Scale: Vec3 (default 1,1,1)
- [ ] Cached model matrix (Mat4)
- [ ] Dirty flag for matrix recomputation
- [ ] Separate from PositionComponent (grid-based game logic)

> **Revised 2026-01-28:** Removed "or Euler if simpler for fixed-angle" option. Quaternion is REQUIRED for free camera (models viewed from all angles) (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-033: Position-to-Transform Synchronization
**Assigned:** Systems Architect
**Depends On:** 2-031, 2-032
**Blocks:** None

**Description:**
System to synchronize PositionComponent (grid coordinates for game logic) to TransformComponent (float coordinates for rendering). Handles coordinate mapping: grid_x -> world X, grid_y -> world Z, elevation -> world Y.

**Acceptance Criteria:**
- [ ] Automatic sync when PositionComponent changes
- [ ] Coordinate mapping: grid_x -> X, grid_y -> Z, elevation -> Y
- [ ] Elevation step configurable (default: 0.25 per level)
- [ ] Grid unit size configurable (default: 1.0)
- [ ] TransformComponent model matrix recalculated on change

---

### Group H: Render Layers

#### 2-034: Render Layer Definitions
**Assigned:** Systems Architect
**Depends On:** None
**Blocks:** 2-035

**Description:**
Define semantic render layers for ordering rendering passes. Layers: Underground, Terrain, Water, Roads, Buildings, Units, Effects, DataOverlay, UIWorld.

**Acceptance Criteria:**
- [ ] RenderLayer enum with defined values
- [ ] Layers rendered in defined order
- [ ] All renderable entities assigned a layer
- [ ] Layer affects draw order within render pass

---

#### 2-035: Layer Visibility Toggle
**Assigned:** Systems Architect
**Depends On:** 2-034
**Blocks:** 2-036

**Description:**
Support toggling visibility of render layers for debug and gameplay features (underground view). Each layer can be shown, hidden, or ghosted (transparent).

**Acceptance Criteria:**
- [ ] setLayerVisibility(layer, state) function
- [ ] States: Visible, Hidden, Ghost (transparent)
- [ ] Hidden layers skip rendering entirely
- [ ] Ghost layers render at reduced alpha
- [ ] Used for underground view implementation

---

#### 2-036: Underground View Mode
**Assigned:** Systems Architect
**Depends On:** 2-035, 2-016
**Blocks:** None

**Description:**
Implement underground view mode using layer visibility. ViewMode state machine: Surface, Underground, Cutaway. Underground shows pipes/subterra with surface ghosted.

**Acceptance Criteria:**
- [ ] ViewMode enum: Surface, Underground, Cutaway
- [ ] Surface mode: normal rendering
- [ ] Underground mode: surface ghosted, underground visible
- [ ] Cutaway mode: surface visible with underground also visible
- [ ] Smooth transition between modes
- [ ] Keybind to toggle view mode

---

### Group I: Visual Effects (Foundation)

#### 2-037: Emissive Material Support
**Assigned:** Graphics Engineer
**Depends On:** 2-005
**Blocks:** 2-038

**Description:**
Core rendering feature for bioluminescent art direction. All 10 terrain types have distinct glow properties. All powered buildings glow. Multi-color emissive palette: cyan (#00D4AA), green, amber, magenta. Per-instance emissive control required via emissive_intensity and emissive_color. Glow intensity hierarchy: player structures > terrain features > background. Glow transitions fade over ~0.5s (interpolated float). Emissive color added to final output, not affected by lighting.

**Acceptance Criteria:**
- [ ] Emission texture or color in material definition
- [ ] Fragment shader adds emission to output
- [ ] Emission not affected by lighting bands
- [ ] All 10 terrain types have distinct emissive glow properties
- [ ] Multi-color emissive palette: cyan (#00D4AA), green, amber, magenta
- [ ] Per-instance emissive control (emissive_intensity float, emissive_color Vec3)
- [ ] Glow intensity hierarchy: player structures > terrain features > background
- [ ] Glow transitions fade over ~0.5s (interpolated float)
- [ ] Powered buildings glow; unpowered buildings do not
- [ ] All powered state visible via emission

> **Revised 2026-01-28:** MAJOR EXPANSION — elevated from "Visual Effects Foundation" to core rendering feature. Scope expanded from single teal conduit glow to: all 10 terrain types with distinct glow properties, all powered buildings glow, multi-color emissive palette, per-instance emissive control, glow intensity hierarchy, 0.5s glow transitions (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-038: Bloom Post-Process
**Assigned:** Graphics Engineer
**Depends On:** 2-037, 2-019
**Blocks:** None

**Description:**
MANDATORY bloom post-process for bioluminescent art direction. Bloom cannot be fully disabled (minimum bloom required for art consistency). Must handle full emissive color range (multi-color palette). Quality tiers (1/4 res, 1/8 res) for performance scaling. Tuned for dark environment with conservative threshold to avoid "glow soup." Budget: 0.5ms.

**Acceptance Criteria:**
- [ ] Bright pixel extraction (conservative threshold tuned for dark environment)
- [ ] Gaussian blur on bright pixels
- [ ] Additive blend back to final image
- [ ] Bloom intensity configurable
- [ ] Bloom cannot be fully disabled (minimum for art consistency)
- [ ] Handles full emissive color range (cyan, green, amber, magenta)
- [ ] Quality tiers: 1/4 resolution, 1/8 resolution for performance scaling
- [ ] Performance: <0.5ms at 1080p
- [ ] Conservative threshold to avoid "glow soup" in dark environment
- [ ] All emissive sources have visible glow aura

> **Revised 2026-01-28:** Elevated from optional to MANDATORY. Required for bioluminescent art direction. Bloom cannot be fully disabled. Quality tiers for performance scaling. Conservative threshold for dark environment. Full emissive color range support (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-039: Shadow Rendering (Basic)
**Assigned:** Graphics Engineer
**Depends On:** 2-018
**Blocks:** None

**Description:**
Implement basic shadow mapping for directional light. Light direction is world-space fixed (alien sun), not camera-relative. Shadow frustum must adapt to free camera orientation. Shadow map texel snapping for stability during camera movement. Shadow style should match toon aesthetic (clean edges). Shadow color/intensity tuned for dark base bioluminescent environment.

**Acceptance Criteria:**
- [ ] Shadow map from directional light (world-space fixed alien sun)
- [ ] Buildings cast shadows on terrain
- [ ] Shadow frustum adapts to free camera orientation
- [ ] Shadow map texel snapping for stability during camera movement
- [ ] Shadow edges relatively clean (toon-appropriate)
- [ ] Shadow color/intensity tuned for dark base environment
- [ ] Shadow quality configurable (resolution)
- [ ] Performance: quality tiers for different hardware
- [ ] Can be disabled for low-end systems

> **Revised 2026-01-28:** Shadow frustum adapts to free camera orientation. Shadow map texel snapping for stability. Light direction is world-space fixed (alien sun), not camera-relative. Shadow color/intensity tuned for dark base environment (trigger: canon-update v0.4.0→v0.5.0)

---

### Group J: Debug and Tools

#### 2-040: Debug Grid Overlay
**Assigned:** Graphics Engineer
**Depends On:** 2-018
**Blocks:** None

**Description:**
Render grid overlay on terrain plane for debugging and development. Shows tile boundaries. Grid must remain readable at all camera angles (free camera). Handles configurable map sizes (128/256/512).

**Acceptance Criteria:**
- [ ] Grid lines rendered at tile boundaries
- [ ] Toggle on/off via debug key
- [ ] Grid visible through terrain (or on top)
- [ ] Grid density adjusts with zoom level
- [ ] Different colors for different grid sizes (16x16, 64x64)
- [ ] Grid readable at all camera angles (free camera)
- [ ] Consider fading or density adjustment at extreme tilt angles
- [ ] Grid handles configurable map sizes (128/256/512)

> **Revised 2026-01-28:** Grid must remain readable at all camera angles (free camera). Consider fading/density adjustment at extreme tilt. Grid handles configurable map sizes (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-041: Wireframe Mode
**Assigned:** Graphics Engineer
**Depends On:** 2-007
**Blocks:** None

**Description:**
Toggle wireframe rendering mode for debugging mesh geometry.

**Acceptance Criteria:**
- [ ] Wireframe fill mode in pipeline
- [ ] Toggle via debug key
- [ ] Shows all triangle edges
- [ ] Helps identify mesh issues

---

#### 2-042: Render Statistics Display
**Assigned:** Graphics Engineer
**Depends On:** 2-019
**Blocks:** None

**Description:**
Display render statistics: FPS, frame time, draw calls, triangles rendered, GPU memory usage.

**Acceptance Criteria:**
- [ ] FPS counter
- [ ] Frame time (ms) display
- [ ] Draw call count
- [ ] Visible triangle count
- [ ] Stats toggled via debug key
- [ ] Stats rendered via UI overlay

---

#### 2-043: Bounding Box Visualization
**Assigned:** Graphics Engineer
**Depends On:** 2-010, 2-018
**Blocks:** None

**Description:**
Debug visualization of entity bounding boxes for culling verification.

**Acceptance Criteria:**
- [ ] Render AABB wireframe around entities
- [ ] Toggle via debug key
- [ ] Different colors for culled vs visible
- [ ] Helps verify frustum culling

---

### Group K: Multiplayer Visual Integration

#### 2-044: Transform Interpolation
**Assigned:** Systems Architect
**Depends On:** 2-032, Epic 1 (SyncSystem)
**Blocks:** None

**Description:**
Interpolate TransformComponent between simulation ticks for smooth 60fps rendering from 20Hz simulation. Position uses lerp, rotation uses slerp.

**Acceptance Criteria:**
- [ ] Store previous and current tick transforms
- [ ] Calculate interpolation factor: t = time_since_tick / tick_duration
- [ ] Position: lerp(prev, curr, t)
- [ ] Rotation: slerp(prev, curr, t)
- [ ] Moving entities (beings) interpolate smoothly
- [ ] Static entities (buildings) use current state

---

#### 2-045: Other Player Cursor Rendering
**Assigned:** Systems Architect
**Depends On:** 2-018, 2-028
**Blocks:** None

**Description:**
Render other players' cursors as 3D objects or billboarded sprites at their world position. Faction-colored to show which player.

**Acceptance Criteria:**
- [ ] Cursor position received from server (world coordinates)
- [ ] Render cursor indicator at world position
- [ ] Faction color applied to cursor
- [ ] Cursor visible to all players in area
- [ ] Shows other players' activity and presence

---

### Group E (continued): Free Camera Controls

#### 2-046: Free Camera Orbit and Tilt Controls
**Assigned:** Systems Architect
**Depends On:** 2-017, Epic 0 (InputSystem)
**Blocks:** 2-025, 2-048

**Description:**
Implement orbit (middle mouse drag) and tilt controls for free camera mode. Orbit rotates yaw around focus point. Tilt adjusts pitch. Pitch clamped to 15-80 degrees. Orbit/tilt input automatically enters free camera mode from preset mode (instant unlock, no animation delay).

**Acceptance Criteria:**
- [ ] Middle mouse drag orbits camera (rotates yaw around focus point)
- [ ] Tilt adjusts pitch within 15°-80° range
- [ ] Pitch clamped to 15°-80°
- [ ] Orbit speed comfortable and consistent
- [ ] Entering free mode from preset is instant (no transition animation)
- [ ] Orbit feels like "walking around a diorama"
- [ ] Mouse sensitivity configurable

> **Added 2026-01-28:** New ticket for free camera orbit and tilt controls. Required by canon v0.5.0 free camera system. Middle mouse = orbit, pitch clamped 15°-80°, instant unlock from preset mode (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-047: Isometric Preset Snap System
**Assigned:** Systems Architect
**Depends On:** 2-017, 2-027
**Blocks:** 2-048

**Description:**
Implement four isometric preset snap views (N/E/S/W at 45° yaw increments, ~35.264° pitch). Q key = snap 90° clockwise, E key = snap 90° counterclockwise. Pressing a preset key enters preset mode and animates to target angles. Smooth transition (0.3-0.5s ease-in-out).

**Acceptance Criteria:**
- [ ] Q key rotates to next clockwise cardinal preset (N→E→S→W)
- [ ] E key rotates to next counterclockwise cardinal preset (N→W→S→E)
- [ ] Smooth animated transition to target preset (0.3-0.5s ease-in-out)
- [ ] Camera "settles" into exact preset angle at end of animation
- [ ] Default game start is Preset_N (yaw 45°, pitch ~35.264°)
- [ ] Preset indicators show current cardinal direction
- [ ] Snap animation interrupted by orbit/tilt input (enters free mode)

> **Added 2026-01-28:** New ticket for isometric preset snap system. Q/E for 90° clockwise/counterclockwise snap with smooth animation. Required by canon v0.5.0 camera system (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-048: Camera Mode Management
**Assigned:** Systems Architect
**Depends On:** 2-017, 2-046, 2-047
**Blocks:** None

**Description:**
State machine for camera modes: Preset (locked to isometric preset angle), Free (full orbit/pan/zoom/tilt), Animating (transitioning between modes/presets). Track current mode. Free-to-preset = smooth snap on preset key. Preset-to-free = instant unlock on orbit/tilt input. Provide getCameraMode() query for other systems.

**Acceptance Criteria:**
- [ ] Mode enum: Free, Preset, Animating
- [ ] Automatic mode switching based on input type
- [ ] getCameraMode() API for other systems to query
- [ ] Default mode: Preset on game start
- [ ] Mode transitions never produce jarring visual jump
- [ ] Free-to-preset: smooth animated snap on Q/E input
- [ ] Preset-to-free: instant unlock on orbit/tilt input

> **Added 2026-01-28:** New ticket for camera mode state machine. Manages transitions between Free, Preset, and Animating modes. Required by canon v0.5.0 free camera system (trigger: canon-update v0.4.0→v0.5.0)

---

### Group L: Performance Optimization

#### 2-049: LOD System (Basic Framework)
**Assigned:** Graphics Engineer
**Depends On:** 2-010, 2-026
**Blocks:** None

**Description:**
Basic LOD framework for 512x512 map support. Distance-based LOD level selection (2 levels minimum: full detail and simplified). LOD distances configurable and map-size-aware. Infrastructure for later epics to populate with actual simplified meshes. Required by canon for large maps.

**Acceptance Criteria:**
- [ ] LOD level selection based on camera distance
- [ ] 2+ LOD levels supported per model
- [ ] LOD distances configurable
- [ ] No visible pop-in (smooth crossfade or aggressive distances)
- [ ] Framework extensible for more LOD levels in later epics
- [ ] Tested with 512x512 map entity counts

> **Added 2026-01-28:** New ticket for basic LOD framework. Required for 512x512 map support per canon v0.5.0 configurable map sizes (trigger: canon-update v0.4.0→v0.5.0)

---

#### 2-050: Spatial Partitioning for Large Maps
**Assigned:** Systems Architect
**Depends On:** 2-026
**Blocks:** None

**Description:**
2D grid-based spatial hash (16x16 or 32x32 cell size) for efficient frustum culling on large maps. Entities register in spatial cells based on position. Frustum culling queries only cells that overlap the view frustum. Required for 512x512 map performance.

**Acceptance Criteria:**
- [ ] Spatial hash with configurable cell size (16x16 or 32x32)
- [ ] Entities automatically registered/unregistered on position change
- [ ] Frustum query returns only entities in visible cells
- [ ] Significant performance improvement on 512x512 maps
- [ ] Cell size tuned for cache efficiency

> **Added 2026-01-28:** New ticket for spatial partitioning. Required for 512x512 map frustum culling performance per canon v0.5.0 configurable map sizes (trigger: canon-update v0.4.0→v0.5.0)

---

---

## Dependency Graph

```
Epic 0 (Foundation)
    |
    v
2-001 (GPU Device)
    |
    +---> 2-002 (Window) ---> 2-014 (Depth Buffer) ---> 2-015 (Depth State)
    |         |                                              |
    |         v                                              v
    |     2-018 (Main Render Pass) <-------- 2-007 (Graphics Pipeline)
    |         |                                              ^
    |         v                                              |
    +---> 2-003 (Resources) ---> 2-009 (glTF) ---> 2-010 (GPU Mesh)
    |                                |                  |
    |                                v                  +---> 2-049 (LOD Framework)
    +---> 2-004 (Shader Pipeline) ---> 2-005 (Toon Shader)
              |                             |
              v                             v
         2-007 (Pipeline)             2-006 (Edge Post-Process)
                                           |
                                           v
                                     2-019 (Complete Frame)
                                           |
                                           v
                                     2-037 (Emissive) ---> 2-038 (Bloom)

2-017 (Camera State)
    |
    +---> 2-020 (View Matrix) ----+
    |                             |
    +---> 2-021 (Projection) -----+--> 2-022 (VP Integration)
    |                                       |
    +---> 2-023 (Pan) ----+                 v
    |                      +---> 2-025 (Viewport Bounds)
    +---> 2-024 (Zoom) ---+          ^        |
    |                                |        v
    +---> 2-046 (Orbit/Tilt) -------+  2-026 (Frustum Culling)
    |         |                              |
    +---> 2-027 (Animation)                  +---> 2-050 (Spatial Partitioning)
    |         |
    |         v
    +---> 2-047 (Preset Snap)
    |         |
    v         v
2-048 (Camera Mode Mgmt) <--- 2-046, 2-047

2-022 (VP Integration)
    |
    v
2-029 (Screen-to-World Ray) ---> 2-030 (Tile Picking)
```

## Critical Path

The minimum path to "something visible on screen":

1. **2-001** GPU Device Creation
2. **2-002** Window Integration
3. **2-003** Resource Management
4. **2-004** Shader Compilation
5. **2-005** Toon Shader
6. **2-007** Graphics Pipeline
7. **2-009** glTF Loading
8. **2-010** GPU Mesh
9. **2-011** Render Command Recording
10. **2-014** Depth Buffer
11. **2-017** Camera State
12. **2-020** View Matrix
13. **2-021** Projection Matrix (Perspective)
14. **2-018** Main Render Pass

After this critical path, parallel work can proceed on:
- Camera controls: **2-023** (Pan), **2-024** (Zoom), **2-046** (Orbit/Tilt), **2-047** (Preset Snap)
- Bioluminescent rendering: **2-037** (Emissive Materials), **2-038** (Bloom) — on critical path for art direction
- Post-processing: **2-006** (Edge Detection)
- Camera mode management: **2-048** (Mode State Machine)
- Large map support: **2-049** (LOD), **2-050** (Spatial Partitioning)
- Debug tools (2-040 through 2-043)
- Multiplayer integration (2-044, 2-045)

---

## Ticket Count Summary

| Group | Count | Description |
|-------|-------|-------------|
| A: GPU Foundation | 4 | Device, window, resources, shaders |
| B: Toon Shader | 4 | Stepped lighting, edges, pipeline, config |
| C: Model Pipeline | 5 | glTF, GPU mesh, instancing, placeholder |
| D: Depth & Render | 6 | Depth buffer, transparency, render pass |
| E: Camera System | 14 | State, matrices, controls, culling, animation, orbit, presets, mode mgmt |
| F: Coordinate Conversion | 3 | World-screen transforms, tile picking |
| G: Components | 3 | Render, transform, sync |
| H: Render Layers | 3 | Layer definitions, visibility, underground |
| I: Visual Effects | 3 | Emissive, bloom, shadows |
| J: Debug Tools | 4 | Grid, wireframe, stats, bounds |
| K: Multiplayer | 2 | Interpolation, cursors |
| L: Performance Optimization | 2 | LOD framework, spatial partitioning |

**Total: 50 tickets** (was 45; +3 camera, +2 performance)

---

## Notes

- Epic 0 must complete glTF support (Ticket 0-034) before Epic 2 model loading begins
- Epic 1 (Multiplayer) is soft dependency - rendering works in local/single-player mode without it
- UI rendering (Epic 12) uses SDL_Renderer, separate from the 3D pipeline
- Animation system (skeletal) deferred to later epic per consensus
- Perspective projection (~35° FOV) selected over orthographic — see plans/decisions/epic-2-projection-type.md
- Free camera with isometric presets replaces fixed isometric camera (canon v0.5.0)
- Bioluminescent art direction requires mandatory bloom and per-instance emissive control
- Configurable map sizes (128/256/512) require spatial partitioning and LOD framework for performance

---

## Ticket Classification (Canon v0.4.0 → v0.5.0)

| Ticket | Status | Change Summary |
|--------|--------|---------------|
| 2-001 | UNCHANGED | — |
| 2-002 | UNCHANGED | — |
| 2-003 | UNCHANGED | — |
| 2-004 | UNCHANGED | — |
| 2-005 | MODIFIED | Emissive term, world-space light, ambient term, bioluminescent palette |
| 2-006 | MODIFIED | Perspective projection support, normal-based edges, depth linearization |
| 2-007 | MODIFIED | Perspective validation, MRT consideration for emissive |
| 2-008 | MODIFIED | Bloom/emissive/ambient parameters, per-terrain emissive presets |
| 2-009 | MODIFIED | Emissive texture/factor extraction from glTF |
| 2-010 | MODIFIED | Emissive texture reference and color/factor in Material struct |
| 2-011 | UNCHANGED | — |
| 2-012 | MODIFIED | Per-instance emissive, 512x512 budget, chunked instancing |
| 2-013 | UNCHANGED | — |
| 2-014 | UNCHANGED | — |
| 2-015 | UNCHANGED | — |
| 2-016 | UNCHANGED | — |
| 2-017 | MODIFIED | MAJOR: Free camera state, mode enum, variable pitch/yaw, presets |
| 2-018 | MODIFIED | Dark clear color, bloom required in pipeline |
| 2-019 | MODIFIED | Bloom mandatory, formalized pass order |
| 2-020 | MODIFIED | Variable pitch/yaw, tested at all angles, gimbal lock |
| 2-021 | MODIFIED | MAJOR: Orthographic → Perspective (~35° FOV) |
| 2-022 | UNCHANGED | — |
| 2-023 | MODIFIED | Camera-relative pan, right mouse drag, edge scrolling |
| 2-024 | MODIFIED | Perspective zoom (distance), map-size-aware range |
| 2-025 | MODIFIED | Frustum trapezoid, configurable map size, depends on 2-046 |
| 2-026 | MODIFIED | Elevated to requirement, spatial partitioning, all angles |
| 2-027 | MODIFIED | Preset snap transitions, interpolate all params |
| 2-028 | UNCHANGED | — |
| 2-029 | MODIFIED | Perspective ray casting, numerical stability |
| 2-030 | MODIFIED | All camera angles, numerical stability guard |
| 2-031 | MODIFIED | emissive_intensity, emissive_color for bioluminescent glow |
| 2-032 | MODIFIED | Quaternion required (no Euler alternative) |
| 2-033 | UNCHANGED | — |
| 2-034 | UNCHANGED | — |
| 2-035 | UNCHANGED | — |
| 2-036 | UNCHANGED | — |
| 2-037 | MODIFIED | MAJOR: Expanded to full bioluminescent system, 10 terrain types |
| 2-038 | MODIFIED | Elevated to mandatory, quality tiers, conservative threshold |
| 2-039 | MODIFIED | Free camera shadow frustum, texel snapping, world-space light |
| 2-040 | MODIFIED | All camera angles, configurable map sizes |
| 2-041 | UNCHANGED | — |
| 2-042 | UNCHANGED | — |
| 2-043 | UNCHANGED | — |
| 2-044 | UNCHANGED | — |
| 2-045 | UNCHANGED | — |
| 2-046 | NEW | Free camera orbit and tilt controls |
| 2-047 | NEW | Isometric preset snap system |
| 2-048 | NEW | Camera mode management state machine |
| 2-049 | NEW | LOD system basic framework |
| 2-050 | NEW | Spatial partitioning for large maps |
