# Graphics Engineer Answers: Epic 2 Discussion

**Date:** 2026-01-27

---

## Answers to Systems Architect Questions

### SA-Q1: Toon Shader Technique

**Recommendation: Option C - Combination with edge detection**

**Rationale:**

For ZergCity's alien aesthetic, a combination approach provides the best results:

1. **Discrete lighting bands (stepped diffuse):** Use 3-4 quantized lighting levels via dot product thresholds in the fragment shader. This creates the classic toon look with crisp light/shadow transitions.

2. **Screen-space edge detection (post-process):** Use Sobel/Roberts cross operator on the depth and normal buffers to detect silhouettes and material boundaries.

**Technical Implementation:**
```hlsl
// Fragment shader - stepped lighting
float NdotL = dot(normal, lightDir);
float toonShade = floor(NdotL * 4.0) / 4.0; // 4 bands
float3 finalColor = baseColor * (ambientStrength + toonShade * lightStrength);

// Post-process edge detection on depth/normal
float depthEdge = sobelDepth(depthBuffer, uv);
float normalEdge = sobelNormal(normalBuffer, uv);
float edge = max(depthEdge, normalEdge);
finalColor = lerp(finalColor, outlineColor, edge * outlineStrength);
```

**Why not toon ramp textures:** Ramp textures add texture sampling overhead and require artist iteration. Math-based banding is faster, more controllable, and easier to tweak at runtime.

**Performance:** The post-process edge pass adds ~0.5-1ms at 1080p. The stepped lighting is essentially free (simple math in existing fragment shader).

---

### SA-Q2: Projection Type

**Recommendation: Orthographic projection**

**Rationale:**

1. **Classic isometric feel:** Orthographic gives the true dimetric/isometric look that matches SimCity 2000 inspiration. No foreshortening means tiles remain consistent size regardless of screen position.

2. **Grid alignment:** With orthographic, grid lines remain perfectly parallel. This is crucial for tile-based building placement - the player can easily see tile boundaries.

3. **Zoom behavior:** Orthographic zoom is intuitive - everything scales uniformly. Perspective zoom changes the viewing angle feel.

4. **Simpler math:** Screen-to-world picking is simpler with orthographic (no perspective divide needed for ray casting).

5. **Alien aesthetic:** The slightly "flattened" look of orthographic reinforces the stylized, non-realistic aesthetic.

**Camera setup:**
```cpp
// Orthographic with isometric-style view
float orthoWidth = viewWidth / pixelsPerUnit;
float orthoHeight = viewHeight / pixelsPerUnit;
mat4 projection = ortho(-orthoWidth/2, orthoWidth/2, -orthoHeight/2, orthoHeight/2, nearPlane, farPlane);

// Fixed isometric rotation (35.264 pitch, 45 yaw)
mat4 view = lookAt(cameraPos, cameraTarget, vec3(0,1,0));
```

---

### SA-Q3: SDL_GPU Shader Format

**Recommendation: Multiple backends with SDL_GPU's built-in shader cross-compilation**

**Rationale:**

SDL_GPU (introduced in SDL3) has native shader cross-compilation support:

1. **Primary source format: HLSL** - Write shaders once in HLSL (most widely documented, good tooling with DXC).

2. **SDL_GPU handles translation:** SDL_GPU can consume SPIR-V and translate at load time, or we can pre-compile to multiple backends.

3. **Build-time compilation pipeline:**
   - Source: HLSL (.hlsl files)
   - DXC compiler: HLSL -> SPIR-V
   - SPIRV-Cross: SPIR-V -> Metal Shading Language (for macOS)
   - SPIRV-Cross: SPIR-V -> GLSL (for OpenGL fallback)
   - Direct: HLSL -> DXIL (for D3D12)

4. **Runtime loading:** SDL_GPU's `SDL_CreateGPUShader()` accepts the appropriate format for the current backend.

**Build configuration:**
```cmake
# Shader compilation in CMake
add_custom_command(
    OUTPUT ${SHADER_SPIRV}
    COMMAND dxc -spirv -T vs_6_0 -E VSMain ${SHADER_HLSL} -Fo ${SHADER_SPIRV}
    DEPENDS ${SHADER_HLSL}
)
```

**Backend selection at runtime:**
```cpp
SDL_GPUDevice* device = SDL_CreateGPUDevice(
    SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL,
    true, // debug mode
    NULL  // let SDL pick best backend
);
```

---

### SA-Q4: Model Instancing Strategy

**Recommendation: GPU instancing with instance buffers**

**Expected draw call budget: 500-1000 draw calls per frame**

**Rationale:**

1. **GPU instancing is ideal for city builders:** Many repeated objects (trees, buildings of same type, road segments). One draw call can render 100+ identical meshes.

2. **Instance buffer contents:**
   - World transform (mat4 or position + rotation + scale)
   - Per-instance color tint
   - Per-instance flags (selected, powered, etc.)

3. **Batching strategy:**
   - Group instances by (mesh, material) pairs
   - Each unique (mesh, material) = one draw call
   - Sort by material to minimize state changes

**Implementation:**
```cpp
struct InstanceData {
    mat4 worldMatrix;
    vec4 tintColor;
    uint32_t flags; // bit 0: selected, bit 1: powered, etc.
};

// Per-frame update
void RenderSystem::prepareInstances() {
    for (auto& [meshMat, instances] : instanceGroups) {
        updateInstanceBuffer(meshMat.instanceBuffer, instances);
    }
}

// Single draw call for all instances of a mesh/material
SDL_DrawGPUIndexedPrimitivesIndirect(cmdBuffer, indexBuffer, instanceBuffer, instanceCount);
```

**Draw call budget breakdown:**
- Terrain chunks (8x8 tile batches): ~64 draws for 64x64 visible area
- Buildings (by type): ~50-100 types, instanced
- Infrastructure: ~30 draw calls
- Vegetation/decoration: ~20 draw calls (heavily instanced)
- Effects/overlays: ~50 draw calls
- UI: ~100 draw calls (separate pass)

**Total: 300-400 typical, 1000 worst case**

**Frustum culling:** Apply CPU-side frustum culling before building instance buffers. With orthographic projection, frustum is a simple box test.

---

### SA-Q5: Outline Technique

**Recommendation: Screen-space edge detection (post-process)**

**Rationale:**

1. **Consistent line width:** Screen-space produces perfectly consistent pixel-width outlines regardless of distance or mesh complexity. Inverted hull produces varying thickness.

2. **Works with all geometry:** No mesh preparation needed. Inverted hull requires closed meshes with no holes; edge extrusion requires geometry shader support.

3. **One pass, all outlines:** Single post-process pass outlines everything. Inverted hull requires an extra draw call per outlined object.

4. **Configurable:** Easy to adjust line width, color, and threshold at runtime via uniforms.

5. **SDL_GPU compatibility:** Post-process is standard render-to-texture + fullscreen quad. Geometry shaders have inconsistent support across backends.

**Technical implementation:**

```hlsl
// Requires depth buffer and optional normal buffer as inputs
// Render scene to color + depth + normal targets first

// Edge detection post-process
float3 PostProcessEdges(float2 uv) {
    float3 sceneColor = colorTexture.Sample(sampler, uv).rgb;

    // Sobel on depth
    float depth = depthTexture.Sample(sampler, uv).r;
    float depthEdge = sobelMagnitude(depthTexture, uv);

    // Sobel on normals (catches inner edges)
    float3 normal = normalTexture.Sample(sampler, uv).rgb;
    float normalEdge = sobelMagnitudeRGB(normalTexture, uv);

    // Combine edges
    float edge = saturate(depthEdge * depthSensitivity + normalEdge * normalSensitivity);
    edge = step(edgeThreshold, edge); // Hard edge

    return lerp(sceneColor, outlineColor, edge);
}
```

**Render target setup:**
1. Main pass: Color (RGBA8), Depth (D24), Normal (RGB8 or RGB10A2)
2. Post-process pass: Fullscreen quad with edge shader
3. Optional: Separate edge buffer for selective outlining (selected objects only)

**Performance:** ~0.5-1ms at 1080p, scales linearly with resolution. Can be optimized with half-resolution edge detection if needed.

---

## Answers to Game Designer Questions

### GD-Q6: Toon Shader Technique Details

**SDL_GPU Fragment Shader Support:**

SDL_GPU fully supports custom fragment shaders. We can implement any toon shading technique.

**Recommended technique breakdown:**

1. **Stepped/quantized diffuse lighting:**
   - Calculate N dot L (normal dot light direction)
   - Quantize to 3-4 discrete bands using floor() or step()
   - This creates the hard light/shadow transitions

2. **Screen-space edge detection (Sobel):**
   - Render scene to depth buffer and normal buffer
   - Post-process pass: Run Sobel operator on both
   - Depth edges catch silhouettes
   - Normal edges catch inner details and material boundaries
   - Combine into final outline

3. **NOT using inverted hull:**
   - Inverted hull (rendering scaled-up back-faces) requires mesh preprocessing
   - Produces inconsistent line widths
   - Doubles draw calls for outlined objects
   - Does not work well with complex/non-manifold meshes

**Shader structure:**
```
Pass 1: Scene render
  - Vertex: Transform, output position + normal + worldPos
  - Fragment: Stepped diffuse lighting, output to color/depth/normal targets

Pass 2: Edge post-process
  - Vertex: Fullscreen quad
  - Fragment: Sobel on depth/normal, composite outlines onto scene

Pass 3 (optional): Bloom for emissives
```

---

### GD-Q7: glTF Material Specification for Toon Shading

**Recommendation: Use glTF PBR base with custom interpretation**

glTF materials are PBR (Physically Based Rendering) by default, but we reinterpret them for toon:

**Material mapping:**
| glTF Property | Toon Interpretation |
|---------------|---------------------|
| baseColorTexture | Diffuse color (sampled directly) |
| baseColorFactor | Color tint multiplier |
| metallicFactor | Ignored (or use for specular highlight threshold) |
| roughnessFactor | Ignored |
| emissiveTexture | Glow map (used for bloom pass) |
| emissiveFactor | Glow intensity |
| normalTexture | Used for edge detection, not lighting |

**Custom extensions approach:**
We can define a simple JSON extension in glTF for toon-specific properties:

```json
"materials": [{
    "pbrMetallicRoughness": {
        "baseColorTexture": { "index": 0 },
        "baseColorFactor": [0.8, 0.2, 0.3, 1.0]
    },
    "emissiveTexture": { "index": 1 },
    "emissiveFactor": [1.0, 0.5, 0.0],
    "extensions": {
        "ZERGCITY_toon": {
            "shadowColor": [0.1, 0.05, 0.15],
            "highlightThreshold": 0.7,
            "outlineWidth": 1.5
        }
    }
}]
```

**UV2 for emission maps:** glTF supports multiple UV sets. If artists need separate UVs for emission, they can use `TEXCOORD_1` and reference it in the emissive texture.

**Practical recommendation:** Start simple - just use baseColor and emissive from standard glTF. Add custom extension only if artists need per-material toon tweaks.

---

### GD-Q8: Performance Budget for Toon Shader

**Target: 16.67ms frame time (60 FPS) with 8ms rendering budget**

**Breakdown:**

| Pass | Budget | Notes |
|------|--------|-------|
| Scene geometry | 4-5ms | Instanced rendering, ~500 draw calls |
| Edge post-process | 0.5-1ms | Fullscreen Sobel |
| Bloom (optional) | 0.5-1ms | Downsample + blur + composite |
| UI overlay | 1ms | 2D rendering |
| **Total rendering** | **7-8ms** | Leaves headroom |

**Draw call budget with outlines:**

The screen-space edge approach does NOT add per-object draw calls. Total draw calls remain:
- 500-1000 scene draws (instanced)
- 3-5 post-process draws (edge, bloom, composite)
- ~100 UI draws

**Outline rendering cost comparison:**
| Technique | Extra draws | Cost |
|-----------|-------------|------|
| Screen-space edge | +1 fullscreen | +0.5-1ms fixed |
| Inverted hull | +1 per outlined mesh | +2-4ms variable |
| Geometry shader | +0 | +1-2ms variable |

**Screen-space wins** because cost is fixed regardless of scene complexity.

**Scaling strategy:**
- Low-end: Disable bloom, reduce edge detection resolution
- Mid-range: Full toon with bloom
- High-end: Higher resolution edge detection, MSAA

---

### GD-Q9: Transparency Handling in 3D

**This is a significant challenge. Recommended approach: Multi-pass with sorted transparent queue.**

**Rendering order:**
1. **Opaque pass:** All solid geometry, depth write ON
2. **Edge post-process:** On opaque geometry only (important!)
3. **Transparent pass:** Sorted back-to-front, depth write OFF, depth test ON

**Specific cases:**

**Construction ghost preview:**
- Render at 30-50% alpha
- Use additive or alpha blending
- Drawn AFTER opaques, no outline (ghosts don't need edges)
- Depth test ON (occluded by terrain), depth write OFF

**Underground view:**
- Fade surface objects to transparency (animate alpha from 1 to 0.2)
- Underground objects rendered opaque
- Or: Use stencil buffer to mask surface, render underground separately

**Selection overlays:**
- Additive color overlay (adds highlight color to existing)
- Or: Fresnel rim shader effect (highlights edges without transparency)
- Can be done in main pass with per-instance flag

**Z-fighting prevention:**
- Small depth bias for overlays
- Explicit sorting for known overlap cases

**Code structure:**
```cpp
void RenderSystem::render() {
    // 1. Opaque geometry (depth write ON)
    renderOpaquePass();

    // 2. Edge detection (reads depth/normal from step 1)
    renderEdgePostProcess();

    // 3. Sort transparent objects back-to-front
    sortTransparents(cameraPosition);

    // 4. Transparent geometry (depth write OFF, depth test ON)
    renderTransparentPass();

    // 5. Final compositing + UI
    renderComposite();
}
```

---

### GD-Q10: Emissive Materials / Bloom Support

**SDL_GPU fully supports bloom via post-processing.**

**Bloom implementation:**

1. **Emissive threshold pass:**
   - During scene render, output emissive to separate render target
   - Or: Post-process threshold pass extracts bright pixels (>1.0 HDR)

2. **Blur passes (separable Gaussian):**
   - Downsample emissive to 1/2, 1/4, 1/8 resolution
   - Horizontal blur pass
   - Vertical blur pass
   - Creates soft glow

3. **Composite:**
   - Add blurred glow back to scene (additive blend)
   - Adjust intensity with uniform

**Shader example:**
```hlsl
// Threshold pass
float4 ExtractBright(float2 uv) {
    float3 color = sceneTexture.Sample(sampler, uv).rgb;
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    return float4(color * step(bloomThreshold, brightness), 1.0);
}

// Blur pass (one direction)
float4 BlurPass(float2 uv, float2 direction) {
    float4 result = 0;
    float weights[5] = {0.227, 0.194, 0.121, 0.054, 0.016};
    for (int i = -4; i <= 4; i++) {
        float2 offset = direction * i * texelSize;
        result += texture.Sample(sampler, uv + offset) * weights[abs(i)];
    }
    return result;
}
```

**Performance:** ~0.5ms for 3-level bloom pyramid at 1080p.

**"Fake" alternative:** If bloom is too expensive, we can use additive geometry:
- Render emissive objects twice: once solid, once as enlarged additive quad
- Cheaper but less smooth

**Recommended for ZergCity:**
- Implement true bloom (it's not expensive)
- Use for: energy conduits, powered building indicators, alien bioluminescence
- Keep bloom subtle to maintain toon aesthetic (not HDR-realistic)

---

## Summary of Recommendations

| Question | Recommendation |
|----------|----------------|
| SA-Q1: Toon technique | Stepped lighting + screen-space edges |
| SA-Q2: Projection | Orthographic |
| SA-Q3: Shader format | HLSL source, multi-backend compilation |
| SA-Q4: Instancing | GPU instancing, 500-1000 draw budget |
| SA-Q5: Outline technique | Screen-space Sobel edge detection |
| GD-Q6: Technique details | See SA-Q1, no inverted hull |
| GD-Q7: glTF materials | Standard PBR reinterpreted, optional custom extension |
| GD-Q8: Performance | 8ms render budget, screen-space edges are cheap |
| GD-Q9: Transparency | Multi-pass sorted, edges on opaques only |
| GD-Q10: Bloom | True post-process bloom, ~0.5ms cost |
