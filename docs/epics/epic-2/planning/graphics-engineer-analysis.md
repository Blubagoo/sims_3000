# Graphics Engineer Analysis: Epic 2 (3D)

**Epic:** Core Rendering Engine (3D)
**Analyst:** Graphics Engineer
**Date:** 2026-01-27
**Canon Version Referenced:** 0.4.0
**Approach:** 3D rendering with fixed isometric camera (replaces 2D sprites)

---

## Overview

Epic 2 establishes the 3D visual foundation for ZergCity. Per the decision record `/plans/decisions/3d-rendering-architecture.md`, we are using **3D rendering with a fixed isometric-style camera** instead of 2D sprites. This fundamentally changes the rendering architecture:

| Aspect | Old (2D) | New (3D) |
|--------|----------|----------|
| Graphics API | SDL_Renderer | SDL_GPU |
| Assets | 2D sprites, texture atlases | 3D models (glTF) |
| Depth sorting | Manual z-ordering | GPU depth buffer |
| Projection | 2D dimetric math | 3D view/projection matrices |
| Shading | Pre-baked cell-shading | Real-time toon shader |
| Zoom | Sprite scaling | Camera distance |

**Key Systems:**
- **RenderingSystem** - SDL_GPU/3D renderer, model rendering, toon shader pipeline, depth buffer
- **CameraSystem** - 3D camera, fixed isometric angle, zoom via camera distance

---

## Key Specifications from Canon

| Specification | Value | Source |
|---------------|-------|--------|
| Graphics API | SDL_GPU | Canon 0.4.0, patterns.yaml |
| Camera pitch | 35.264 degrees | 3d-rendering-architecture.md |
| Camera yaw | 45 degrees | 3d-rendering-architecture.md |
| Grid unit | 1.0 (3D world units) | patterns.yaml |
| Elevation levels | 32 (0-31) | patterns.yaml |
| Model format | glTF | 3d-rendering-architecture.md |
| Shading style | Toon/cell-shaded | Canon 0.4.0 |
| Simulation tick rate | 20 Hz | interfaces.yaml |
| Target frame rate | 60 fps | Canon |

---

## SDL_GPU Architecture

### What is SDL_GPU?

SDL_GPU is SDL3's new cross-platform GPU abstraction layer. It provides a modern rendering API that abstracts over:
- Direct3D 12 (Windows)
- Metal (macOS/iOS)
- Vulkan (cross-platform)

Unlike SDL_Renderer (which is 2D-focused), SDL_GPU provides full 3D rendering capabilities including:
- Custom shaders (HLSL, MSL, SPIR-V via SDL_shadercross)
- Depth/stencil buffers
- Render targets (framebuffers)
- Compute shaders
- Modern GPU features

### SDL_GPU Key Concepts

```cpp
// Core objects
SDL_GPUDevice*        // GPU device handle (one per application)
SDL_GPUCommandBuffer* // Command recording for submission
SDL_GPURenderPass*    // Encapsulates a render operation
SDL_GPUGraphicsPipeline* // Shader + state configuration
SDL_GPUBuffer*        // Vertex/index/uniform buffers
SDL_GPUTexture*       // Textures and render targets
SDL_GPUSampler*       // Texture sampling configuration
```

### Initialization Pattern

```cpp
// 1. Create GPU device
SDL_GPUDevice* gpu = SDL_CreateGPUDevice(
    SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL,
    true,  // debug mode
    NULL   // prefer any backend
);

// 2. Claim window for presentation
SDL_ClaimWindowForGPUDevice(gpu, window);

// 3. Create resources (textures, buffers, pipelines)
// ...

// 4. Each frame:
SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
// Record commands...
SDL_SubmitGPUCommandBuffer(cmd);
```

---

## Key Work Items

### 1. SDL_GPU Initialization and Device Setup

- [ ] **RENDER-3D-01: GPU Device Creation**
  - Create SDL_GPUDevice with appropriate shader format support
  - Detect available backends (D3D12, Vulkan, Metal)
  - Configure debug layers for development builds
  - Handle device creation failure gracefully
  - Log selected backend and capabilities

- [ ] **RENDER-3D-02: Window Integration**
  - Claim window for GPU device (`SDL_ClaimWindowForGPUDevice`)
  - Configure swap chain (vsync, present mode)
  - Handle window resize (recreate swap chain)
  - Support fullscreen toggle

- [ ] **RENDER-3D-03: Resource Management Infrastructure**
  - GPU buffer pool for uniform data
  - Texture loading from PNG via stb_image
  - Sampler creation (linear/nearest filtering)
  - Frame resource management (double/triple buffering)

### 2. Shader System and Toon Shader

The toon/cell shader is critical for achieving the art style. This requires custom shaders.

- [ ] **RENDER-3D-04: Shader Compilation Pipeline**
  - Use SDL_shadercross or compile shaders offline
  - Support HLSL as source (most documented for SDL_GPU)
  - Generate SPIR-V and DXIL for cross-platform
  - Shader hot-reload for development
  - Error reporting for shader compilation failures

- [ ] **RENDER-3D-05: Toon Shader Implementation**

  **Vertex Shader:**
  ```hlsl
  struct VSInput {
      float3 position : POSITION;
      float3 normal : NORMAL;
      float2 texcoord : TEXCOORD0;
  };

  struct VSOutput {
      float4 position : SV_Position;
      float3 worldNormal : NORMAL;
      float2 texcoord : TEXCOORD0;
      float3 worldPos : TEXCOORD1;
  };

  cbuffer CameraUniforms : register(b0) {
      float4x4 viewProjection;
      float3 cameraPosition;
  };

  cbuffer ModelUniforms : register(b1) {
      float4x4 model;
      float4x4 normalMatrix;
  };

  VSOutput main(VSInput input) {
      VSOutput output;
      float4 worldPos = mul(model, float4(input.position, 1.0));
      output.position = mul(viewProjection, worldPos);
      output.worldNormal = normalize(mul((float3x3)normalMatrix, input.normal));
      output.texcoord = input.texcoord;
      output.worldPos = worldPos.xyz;
      return output;
  }
  ```

  **Fragment Shader (Toon Shading):**
  ```hlsl
  struct PSInput {
      float4 position : SV_Position;
      float3 worldNormal : NORMAL;
      float2 texcoord : TEXCOORD0;
      float3 worldPos : TEXCOORD1;
  };

  cbuffer LightUniforms : register(b2) {
      float3 lightDirection;  // Directional light (sun)
      float3 lightColor;
      float3 ambientColor;
  };

  Texture2D diffuseTexture : register(t0);
  SamplerState linearSampler : register(s0);

  // Toon shading bands (3-4 bands typical)
  float3 toonShade(float NdotL) {
      // Quantize lighting into bands
      if (NdotL > 0.8) return float3(1.0, 1.0, 1.0);      // Full light
      if (NdotL > 0.4) return float3(0.7, 0.7, 0.7);      // Mid light
      if (NdotL > 0.1) return float3(0.4, 0.4, 0.4);      // Shadow
      return float3(0.2, 0.2, 0.2);                        // Deep shadow
  }

  float4 main(PSInput input) : SV_Target {
      float3 normal = normalize(input.worldNormal);
      float NdotL = max(dot(normal, -lightDirection), 0.0);

      float3 toonFactor = toonShade(NdotL);
      float4 texColor = diffuseTexture.Sample(linearSampler, input.texcoord);

      float3 finalColor = texColor.rgb * toonFactor * lightColor + ambientColor * texColor.rgb * 0.3;
      return float4(finalColor, texColor.a);
  }
  ```

- [ ] **RENDER-3D-06: Outline Shader (Optional Enhancement)**
  - Second pass for cartoon outlines
  - Inverted hull technique or edge detection post-process
  - Configurable outline thickness and color

- [ ] **RENDER-3D-07: Graphics Pipeline Creation**
  - Create SDL_GPUGraphicsPipeline for toon rendering
  - Configure vertex input layout (position, normal, UV)
  - Enable depth test/write
  - Configure blend state for transparency
  - Cull back faces

### 3. 3D Model Rendering Pipeline

- [ ] **RENDER-3D-08: glTF Model Loading**
  - Integrate cgltf or tinygltf library
  - Parse glTF 2.0 binary (.glb) and JSON (.gltf)
  - Extract mesh data (positions, normals, UVs, indices)
  - Extract material data (base color texture)
  - Handle multiple meshes per model
  - Integration with Epic 0 AssetManager

- [ ] **RENDER-3D-09: GPU Mesh Representation**
  ```cpp
  struct GPUMesh {
      SDL_GPUBuffer* vertexBuffer;
      SDL_GPUBuffer* indexBuffer;
      uint32_t indexCount;
      SDL_GPUTexture* diffuseTexture;

      // Bounding info for culling
      AABB boundingBox;
  };

  struct ModelAsset {
      std::vector<GPUMesh> meshes;
      AABB bounds;
  };
  ```

- [ ] **RENDER-3D-10: Render Command Recording**
  ```cpp
  void recordModelDraw(
      SDL_GPURenderPass* pass,
      const GPUMesh& mesh,
      const mat4& modelMatrix,
      SDL_GPUBuffer* uniformBuffer
  ) {
      SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding){
          .buffer = mesh.vertexBuffer,
          .offset = 0
      }, 1);

      SDL_BindGPUIndexBuffer(pass, &(SDL_GPUBufferBinding){
          .buffer = mesh.indexBuffer,
          .offset = 0
      }, SDL_GPU_INDEXELEMENTSIZE_32BIT);

      // Upload model matrix to uniform buffer
      // ...

      SDL_DrawGPUIndexedPrimitives(pass, mesh.indexCount, 1, 0, 0, 0);
  }
  ```

- [ ] **RENDER-3D-11: Instance Rendering (Optimization)**
  - Batch multiple instances of same model
  - Instance buffer with per-instance transforms
  - Critical for terrain tiles (many identical meshes)

### 4. View and Projection Matrix Math

The isometric-style camera uses specific angles for the classic look.

- [ ] **RENDER-3D-12: Camera Matrix Calculations**

  **Isometric Camera Parameters:**
  - Pitch: 35.264 degrees (arctan(1/sqrt(2)) - "true" isometric)
  - Yaw: 45 degrees
  - Position: Calculated from target + distance

  ```cpp
  struct IsometricCamera {
      vec3 target;      // Point camera looks at (grid center)
      float distance;   // Distance from target (controls zoom)
      float pitch;      // Fixed: 35.264 degrees
      float yaw;        // Fixed: 45 degrees (or 45, 135, 225, 315 for rotation)

      mat4 getViewMatrix() const {
          // Calculate camera position from spherical coordinates
          float cosPitch = cos(radians(pitch));
          float sinPitch = sin(radians(pitch));
          float cosYaw = cos(radians(yaw));
          float sinYaw = sin(radians(yaw));

          vec3 offset = vec3(
              distance * cosPitch * sinYaw,
              distance * sinPitch,
              distance * cosPitch * cosYaw
          );

          vec3 position = target + offset;
          vec3 up = vec3(0, 1, 0);

          return lookAt(position, target, up);
      }

      mat4 getProjectionMatrix(float aspectRatio) const {
          // Orthographic projection for true isometric
          // Width based on distance for consistent zoom feel
          float width = distance * 0.5f;  // Adjust factor for desired zoom
          float height = width / aspectRatio;

          return ortho(-width, width, -height, height, 0.1f, 1000.0f);
      }
  };
  ```

- [ ] **RENDER-3D-13: Orthographic vs Perspective Decision**

  **Orthographic (Recommended for isometric):**
  - True isometric look (parallel lines stay parallel)
  - Matches classic city builder aesthetic
  - Zoom = change ortho bounds

  **Perspective (Alternative):**
  - Slight depth perspective
  - More "modern" feel
  - Zoom = move camera closer

  Recommend: **Orthographic** for authenticity to SimCity 2000 aesthetic.

- [ ] **RENDER-3D-14: Screen-to-World Ray Casting**
  ```cpp
  // For tile picking with orthographic camera
  Ray screenToWorldRay(vec2 screenPos, const Camera& cam) {
      // Normalize screen coordinates to [-1, 1]
      vec2 ndc = (screenPos / screenSize) * 2.0f - 1.0f;
      ndc.y = -ndc.y;  // Flip Y

      // Unproject using inverse view-projection
      mat4 invVP = inverse(cam.getViewProjection());

      vec4 nearPoint = invVP * vec4(ndc, -1.0f, 1.0f);
      vec4 farPoint = invVP * vec4(ndc, 1.0f, 1.0f);

      nearPoint /= nearPoint.w;
      farPoint /= farPoint.w;

      Ray ray;
      ray.origin = vec3(nearPoint);
      ray.direction = normalize(vec3(farPoint - nearPoint));
      return ray;
  }

  // Intersect ray with ground plane (Y = elevation)
  GridPosition pickTile(Ray ray, int elevation) {
      float groundY = elevation * ELEVATION_HEIGHT;
      float t = (groundY - ray.origin.y) / ray.direction.y;
      vec3 hitPoint = ray.origin + ray.direction * t;

      return GridPosition{
          (int)floor(hitPoint.x),
          (int)floor(hitPoint.z)  // Z is "depth" in our coord system
      };
  }
  ```

### 5. Depth Buffer Configuration

The depth buffer is why we switched to 3D - it handles occlusion automatically.

- [ ] **RENDER-3D-15: Depth Buffer Setup**
  ```cpp
  // Create depth texture
  SDL_GPUTexture* depthTexture = SDL_CreateGPUTexture(gpu, &(SDL_GPUTextureCreateInfo){
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,  // or D24_UNORM_S8_UINT
      .width = windowWidth,
      .height = windowHeight,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
  });
  ```

- [ ] **RENDER-3D-16: Depth State Configuration**
  ```cpp
  // In graphics pipeline creation
  SDL_GPUDepthStencilState depthState = {
      .enable_depth_test = true,
      .enable_depth_write = true,
      .compare_op = SDL_GPU_COMPAREOP_LESS,
      .enable_stencil_test = false
  };
  ```

- [ ] **RENDER-3D-17: Transparent Object Handling**
  - Render opaque objects first with depth write
  - Render transparent objects back-to-front WITHOUT depth write
  - Or use Order-Independent Transparency (OIT) for advanced cases

### 6. Render Pass Structure

- [ ] **RENDER-3D-18: Main Render Pass**
  ```cpp
  void renderFrame() {
      SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
      SDL_GPUTexture* swapchainTexture;
      SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTexture, NULL, NULL);

      if (swapchainTexture) {
          // Begin render pass with color and depth targets
          SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd,
              &(SDL_GPUColorTargetInfo){
                  .texture = swapchainTexture,
                  .load_op = SDL_GPU_LOADOP_CLEAR,
                  .store_op = SDL_GPU_STOREOP_STORE,
                  .clear_color = {0.2f, 0.3f, 0.4f, 1.0f}  // Sky color
              }, 1,
              &(SDL_GPUDepthStencilTargetInfo){
                  .texture = depthTexture,
                  .load_op = SDL_GPU_LOADOP_CLEAR,
                  .store_op = SDL_GPU_STOREOP_DONT_CARE,
                  .clear_depth = 1.0f
              }
          );

          // Bind toon shader pipeline
          SDL_BindGPUGraphicsPipeline(pass, toonPipeline);

          // Upload camera uniforms
          uploadCameraUniforms(cmd);

          // Render terrain
          renderTerrain(pass);

          // Render buildings (sorted by opaque/transparent)
          renderBuildings(pass);

          // Render effects
          renderEffects(pass);

          SDL_EndGPURenderPass(pass);
      }

      SDL_SubmitGPUCommandBuffer(cmd);
  }
  ```

- [ ] **RENDER-3D-19: Multi-Pass Rendering (Future)**
  - Shadow map pass (directional light shadows)
  - SSAO pass (ambient occlusion for depth)
  - Post-processing pass (outline, color grading)
  - UI pass (separate, potentially SDL_Renderer)

### 7. Camera System (3D)

- [ ] **CAMERA-3D-01: Camera State**
  ```cpp
  struct CameraState {
      vec3 target;           // Point camera orbits around
      float distance;        // Distance from target (zoom)
      float pitch = 35.264f; // Fixed isometric angle
      float yaw = 45.0f;     // Fixed (or allow 90-degree snaps)

      // Zoom limits
      float minDistance = 5.0f;   // Close zoom
      float maxDistance = 100.0f; // Far zoom

      // Animation state
      vec3 targetVelocity;
      float distanceVelocity;
  };
  ```

- [ ] **CAMERA-3D-02: Pan Controls**
  - WASD/Arrow keys move `target` in world XZ plane
  - Middle-mouse drag for direct pan
  - Edge-of-screen scrolling (toggleable)
  - Pan speed scales with zoom (distance)

- [ ] **CAMERA-3D-03: Zoom Controls**
  - Mouse wheel adjusts `distance`
  - Zoom toward cursor: adjust `target` to keep cursor world-point stable
  - Smooth interpolation between zoom levels
  - Respect min/max distance limits

- [ ] **CAMERA-3D-04: Viewport Bounds**
  - Calculate visible world rectangle from camera frustum
  - Clamp `target` to keep view within map bounds
  - Provide `getVisibleTileRange()` for frustum culling

- [ ] **CAMERA-3D-05: Camera Rotation (Optional)**
  - Allow 90-degree yaw snaps (Q/E keys)
  - Smooth rotation animation between angles
  - Four view directions: 45, 135, 225, 315 degrees
  - Marked as "future" in decision record

### 8. Render Layers and Overlays

Even with depth buffer, we need conceptual layers for certain features.

- [ ] **RENDER-3D-20: Layer Management**
  ```cpp
  enum class RenderLayer : uint8_t {
      UNDERGROUND = 0,     // Pipes, subterra
      TERRAIN = 1,         // Ground mesh
      WATER = 2,           // Water surfaces
      ROADS = 3,           // Road meshes
      BUILDINGS = 4,       // Structure meshes
      UNITS = 5,           // Beings/vehicles
      EFFECTS = 6,         // Particles, construction
      DATA_OVERLAY = 7,    // Power/crime/etc overlays
      UI_WORLD = 8,        // World-space UI (selection rings)
      // UI_SCREEN handled separately
  };
  ```

- [ ] **RENDER-3D-21: Underground View Toggle**
  - Toggle visibility of underground layer
  - Fade/ghost surface buildings when underground active
  - Use stencil buffer or separate pass

- [ ] **RENDER-3D-22: Data Overlays**
  - Power coverage: Colored tiles/meshes
  - Crime/pollution: Heat map overlay
  - Render as transparent layer over terrain
  - Toggle on/off per overlay type

### 9. Integration Points

- [ ] **RENDER-3D-23: AssetManager Integration**
  - Register glTF model loader with AssetManager
  - Request models by ID, receive GPUMesh handles
  - Handle async loading (render placeholder until ready)
  - Model LOD support (future)

- [ ] **RENDER-3D-24: ECS Component Integration**
  ```cpp
  // 3D-specific render component
  struct ModelComponent {
      AssetHandle<ModelAsset> model;
      vec4 tintColor = vec4(1.0f);
      bool visible = true;
  };

  // Position in 3D world
  struct TransformComponent {
      vec3 position;    // World position (grid units)
      quat rotation;    // Orientation
      vec3 scale;       // Scale factor
  };

  // Existing PositionComponent maps to TransformComponent:
  // grid_x -> position.x
  // grid_y -> position.z
  // elevation -> position.y
  ```

- [ ] **RENDER-3D-25: Interpolation**
  - Interpolate TransformComponent between ticks
  - Smooth motion for animated entities
  - Camera interpolation for smooth pan/zoom

---

## Questions for Other Agents

### @systems-architect:

- **Q001:** Should TransformComponent replace PositionComponent, or should we have both (PositionComponent for grid logic, TransformComponent for rendering)?

- **Q002:** For glTF loading, should this be part of Epic 0 AssetManager, or a separate system in Epic 2?

- **Q003:** How should we handle the UI layer? Continue with SDL_Renderer for 2D UI overlaid on 3D scene, or move to GPU-based UI?

- **Q004:** What's the coordinate mapping? I propose: grid_x -> world X, grid_y -> world Z (depth), elevation -> world Y (up).

- **Q005:** For multiplayer, cursors need to render in world space. Should cursor position be a 3D world point or screen-space with ray-cast on hover?

### @game-designer:

- **Q006:** Orthographic vs perspective projection? Orthographic gives true isometric, perspective gives slight depth. Which matches the vision?

- **Q007:** Should camera rotation (90-degree snaps) be in initial release, or deferred?

- **Q008:** Day/night cycle - with real-time shading, we can do actual lighting changes. Desired? Or stick with palette shift approach?

- **Q009:** Building construction animation - with 3D, we could animate the model (rise from ground, assembly). Or keep the "materializing" ghost effect?

- **Q010:** Shadows - real-time shadows are possible and add depth. Worth the performance cost?

---

## Risks and Concerns

### Risk 1: SDL_GPU Learning Curve
**Severity:** High
**Description:** SDL_GPU is relatively new (SDL3). Documentation is limited compared to raw OpenGL/Vulkan. Team may need ramp-up time.
**Mitigation:**
- Start with SDL_GPU examples from SDL repository
- Have fallback plan to raw OpenGL if SDL_GPU proves problematic
- Build simple test cases before full integration

### Risk 2: Shader Cross-Compilation
**Severity:** Medium
**Description:** SDL_GPU supports multiple shader formats. We need HLSL -> SPIR-V and DXIL. SDL_shadercross exists but adds build complexity.
**Mitigation:**
- Use HLSL as source, compile offline to both formats
- Store pre-compiled shaders in assets
- Hot-reload only in development builds

### Risk 3: glTF Library Integration
**Severity:** Medium
**Description:** Need to integrate cgltf or tinygltf. These are header-only but add another dependency.
**Mitigation:**
- cgltf is single-header, minimal footprint
- tinygltf more feature-complete if needed
- Start with cgltf for simplicity

### Risk 4: Performance with Many Models
**Severity:** Medium
**Description:** Unlike 2D batching, each 3D model may be a separate draw call. Large cities could have thousands of buildings.
**Mitigation:**
- Instance rendering for repeated models (same building type)
- Frustum culling (depth buffer helps but still want to skip invisible)
- LOD system for distant buildings (lower poly)
- Occlusion culling (advanced, future)

### Risk 5: Orthographic Picking Precision
**Severity:** Low
**Description:** Orthographic projection can make depth-based picking tricky since multiple objects project to same screen point.
**Mitigation:**
- Ray-cast through all objects at point
- Return closest to camera (lowest depth)
- Or return all and let game logic decide

### Risk 6: UI Integration
**Severity:** Medium
**Description:** SDL_GPU is 3D-focused. UI rendering (menus, HUD) may need different approach.
**Mitigation:**
- Keep SDL_Renderer for UI (can coexist with SDL_GPU)
- Or use ImGui with SDL_GPU backend
- Or render UI to texture, composite in final pass

---

## Dependencies

### Epic 2 Depends On

- **Epic 0: Application** - Window handle, SDL3 initialization
- **Epic 0: AssetManager** - Model and texture loading (needs glTF support)
- **Epic 0: InputSystem** - Camera control input
- **Epic 0: ECS** - Registry, components

### Epic 2 Blocks

- **Epic 3 (Terrain)** - Terrain meshes need RenderingSystem
- **Epic 4+ (All visual)** - Everything needs 3D rendering
- **Epic 12 (UI)** - UI overlay integration

### Internal Dependencies (Within Epic 2)

```
RENDER-3D-01 (GPU Device)
  --> RENDER-3D-02 (Window)
  --> RENDER-3D-03 (Resources)
  --> RENDER-3D-04 (Shaders)

RENDER-3D-04 (Shaders) --> RENDER-3D-05 (Toon Shader)
                       --> RENDER-3D-07 (Pipeline)

RENDER-3D-08 (glTF Loading) --> RENDER-3D-09 (GPU Mesh)
                            --> RENDER-3D-10 (Draw Recording)

RENDER-3D-12 (Camera Math) --> RENDER-3D-14 (Ray Casting)
                           --> CAMERA-3D-01 (Camera State)

RENDER-3D-15, 16 (Depth Buffer) --> RENDER-3D-18 (Render Pass)

CAMERA-3D-01 (State) --> CAMERA-3D-02 (Pan)
                     --> CAMERA-3D-03 (Zoom)
                     --> CAMERA-3D-04 (Bounds)
```

Critical Path:
1. GPU Device + Window (foundation)
2. Shader compilation (need shaders to render)
3. Toon shader + pipeline (visual output)
4. glTF loading + mesh (content to render)
5. Camera matrices (correct view)
6. Depth buffer (automatic occlusion)
7. Render pass (put it together)
8. Camera controls (user navigation)

---

## Technical Recommendations

### 1. Coordinate System Convention

```
World Space:
  X = East (grid_x direction)
  Y = Up (elevation)
  Z = South (grid_y direction, into screen at default angle)

Grid to World:
  world.x = grid_x * GRID_UNIT_SIZE  (1.0)
  world.y = elevation * ELEVATION_HEIGHT  (e.g., 0.25 per level)
  world.z = grid_y * GRID_UNIT_SIZE  (1.0)
```

### 2. Uniform Buffer Layout

```cpp
// Camera uniforms - updated once per frame
struct CameraUniforms {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 cameraPosition;
    float time;
};

// Model uniforms - per draw call
struct ModelUniforms {
    mat4 model;
    mat4 normalMatrix;  // transpose(inverse(model))
    vec4 tintColor;
};

// Light uniforms - updated when lighting changes
struct LightUniforms {
    vec3 sunDirection;
    float padding1;
    vec3 sunColor;
    float padding2;
    vec3 ambientColor;
    float timeOfDay;
};
```

### 3. Toon Shading Bands

For the cell-shaded look, use 3-4 discrete lighting bands:

```
Band 4: NdotL > 0.7  - Full light (1.0)
Band 3: NdotL > 0.4  - Mid light (0.7)
Band 2: NdotL > 0.1  - Shadow (0.4)
Band 1: NdotL <= 0.1 - Deep shadow (0.2)
```

Optionally add rim lighting for alien glow effect.

### 4. Model Asset Pipeline

```
Source: .blend or .fbx (Blender, 3DS Max, etc.)
  |
  v
Export: .glb (glTF binary)
  |
  v
Load: cgltf parses at runtime
  |
  v
Upload: Create SDL_GPUBuffer for vertices/indices
  |
  v
Cache: Store GPUMesh in AssetManager
```

### 5. Frame Structure

```cpp
void renderFrame(float deltaTime, float interpolationAlpha) {
    // 1. Update camera
    cameraSystem.update(deltaTime);

    // 2. Calculate visible entities
    auto visibleRange = cameraSystem.getVisibleTileRange();
    auto visibleEntities = spatialIndex.query(visibleRange);

    // 3. Sort by layer (terrain, buildings, effects)
    sortByLayer(visibleEntities);

    // 4. Begin GPU frame
    auto cmd = SDL_AcquireGPUCommandBuffer(gpu);

    // 5. Upload uniforms
    uploadCameraUniforms(cmd, cameraSystem);
    uploadLightUniforms(cmd, timeOfDay);

    // 6. Main render pass
    auto pass = beginMainRenderPass(cmd);

    for (auto& entity : visibleEntities) {
        if (hasComponent<ModelComponent>(entity)) {
            // Interpolate transform
            auto transform = interpolateTransform(entity, interpolationAlpha);
            uploadModelUniforms(cmd, transform);

            // Draw model
            auto& model = getComponent<ModelComponent>(entity);
            drawModel(pass, model);
        }
    }

    SDL_EndGPURenderPass(pass);

    // 7. UI pass (separate, potentially SDL_Renderer)
    renderUI();

    // 8. Submit
    SDL_SubmitGPUCommandBuffer(cmd);
}
```

---

## Component Definitions (Proposed for 3D)

### ModelComponent
```cpp
struct ModelComponent {
    AssetHandle<ModelAsset> model;   // glTF model asset
    vec4 tintColor = {1, 1, 1, 1};   // Color modulation
    bool castShadow = true;
    bool receiveShadow = true;
    bool visible = true;
};
```

### TransformComponent
```cpp
struct TransformComponent {
    vec3 position = {0, 0, 0};       // World position
    quat rotation = quat::identity;  // Orientation
    vec3 scale = {1, 1, 1};          // Scale

    mat4 getModelMatrix() const {
        return translate(position) * toMat4(rotation) * glm::scale(scale);
    }
};
```

### RenderLayerComponent
```cpp
struct RenderLayerComponent {
    RenderLayer layer = RenderLayer::BUILDINGS;
    int16_t sublayerOrder = 0;  // Fine-tuning within layer
};
```

---

## File Structure (Proposed for 3D)

```
src/
  rendering/
    GPUDevice.h/.cpp           # SDL_GPU device wrapper
    RenderingSystem.h/.cpp     # Main 3D render system
    ShaderManager.h/.cpp       # Shader loading/compilation
    ToonShader.h/.cpp          # Toon shader specifics
    GPUMesh.h/.cpp             # Mesh GPU representation
    GPUTexture.h/.cpp          # Texture GPU representation
    RenderPass.h/.cpp          # Render pass management
  camera/
    CameraSystem.h/.cpp        # 3D camera management
    IsometricCamera.h/.cpp     # Isometric camera math
    CameraController.h/.cpp    # Input handling for camera
  assets/
    ModelLoader.h/.cpp         # glTF loading (with cgltf)
  components/
    ModelComponent.h
    TransformComponent.h
    RenderLayerComponent.h
  math/
    Ray.h                      # Ray for picking
    Frustum.h                  # Frustum for culling
```

---

## Verification Checklist

Before considering Epic 2 complete:

- [ ] SDL_GPU device initializes on target platforms (Windows, macOS, Linux)
- [ ] Window displays rendered content (not blank/black)
- [ ] Toon shader produces correct cell-shaded appearance
- [ ] 3D models load from glTF files
- [ ] Models render at correct world positions
- [ ] Depth buffer handles occlusion automatically
- [ ] Isometric camera angle matches 35.264 pitch, 45 yaw
- [ ] Camera panning moves view smoothly
- [ ] Camera zoom works via distance change
- [ ] Zoom toward cursor keeps cursor point stable
- [ ] Frustum culling skips off-screen entities
- [ ] Screen-to-world picking returns correct tile
- [ ] 1000+ visible models maintain 60fps
- [ ] Memory usage is stable (no GPU resource leaks)
- [ ] Shader errors are logged clearly
- [ ] Missing models show placeholder (not crash)

---

## Summary

Epic 2's shift to 3D rendering with SDL_GPU represents a significant architectural change that brings major benefits:

**Advantages of 3D approach:**
1. **Automatic depth sorting** via GPU depth buffer
2. **Real-time toon shading** for dynamic lighting
3. **Simpler zoom** - just move camera, no resolution concerns
4. **Easier assets** - one 3D model vs multiple sprite angles
5. **Future flexibility** - shadows, reflections, rotation possible

**Key challenges:**
1. **SDL_GPU learning curve** - newer API, less documentation
2. **Shader development** - need HLSL toon shader
3. **glTF integration** - new asset pipeline
4. **Performance** - instancing critical for many buildings

**Recommended implementation order:**
1. GPU device and window setup
2. Basic shader pipeline (solid color first)
3. Toon shader development
4. glTF model loading
5. Camera system with isometric math
6. Depth buffer and render pass
7. Camera controls
8. Optimization (instancing, culling)

The work builds incrementally toward a complete 3D rendering system that maintains the cell-shaded aesthetic while leveraging GPU capabilities for automatic occlusion and real-time effects.
