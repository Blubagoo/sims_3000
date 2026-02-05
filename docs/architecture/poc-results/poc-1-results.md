# POC-1 Results: SDL_GPU Toon Shading

**Date:** 2026-02-04
**Status:** APPROVED
**Verdict:** Systems Architect APPROVE with conditions

---

## Summary

POC-1 validates that SDL_GPU with instanced toon rendering can handle the performance requirements of a city builder. The POC renders 10,000 building instances across 41 different models with two-pass toon shading (outline + cel shading) at 359 FPS on an RTX 4080, with massive headroom on all metrics.

---

## Benchmark Results

### Primary Test: 10,000 Instances (41 Models, RTX 4080)

| Metric | Target | Measured | Result |
|--------|--------|----------|--------|
| Frame time | ≤16.67ms | **2.78ms avg** (0.27ms min, 5.20ms max) | **PASS** (6x headroom) |
| FPS | ≥60 | **359.1** | **PASS** |
| Draw calls | unique_models × 2 | **82** (41 × 2 passes) | **PASS** |
| GPU memory | <256 MB | **7.18 MB** | **PASS** (35x headroom) |
| Outline quality | Clean | Clean (inverted hull, no artifacts) | **PASS** |

### Stress Test: 100,000 Instances (41 Models, RTX 4080)

| Metric | Measured |
|--------|----------|
| Frame time | 4.20ms avg (0.27ms min, 8.48ms max) |
| FPS | 238.3 |
| Draw calls | 82 (unchanged - instancing scales) |
| GPU memory | 20.89 MB |
| Instances | 99,856 |

### Mid-Range GPU Extrapolation (GTX 1660 / RTX 3060, ~3-4x slower)

| Scenario | Estimated Frame Time | Viable? |
|----------|---------------------|---------|
| 10,000 instances (no culling) | ~11ms | Yes - comfortable headroom |
| 100,000 instances (no culling) | ~17ms avg, ~34ms worst | Borderline |
| 2,000-5,000 visible (with culling) | ~3-6ms | Yes - plenty of headroom |

---

## What Was Implemented

1. **SDL_GPU device initialization** and window management (`gpu_device.cpp`)
2. **DXC pre-compilation** of HLSL shaders to DXIL at build time (`CMakeLists.txt`)
3. **Two-pass toon rendering pipeline** (`toon_pipeline.cpp`)
   - Pass 1: Outline via inverted hull (front-face culling, vertex extrusion along normals)
   - Pass 2: Toon shading with 3-band stepped lighting (back-face culling)
4. **Instanced rendering** via GPU storage buffers with `SV_InstanceID` (`instance_buffer.cpp`)
5. **Multi-model support**: 41 Kenney commercial building GLBs loaded and randomly assigned to instances
6. **Instance sorting by model index** for batched draws with `firstInstance` offset
7. **glTF/GLB model loading** with cgltf - all primitives from all meshes (`model_loader.cpp`)
8. **Per-instance color variation** via deterministic position-based hashing
9. **Isometric orthographic camera** (`camera.cpp`)
10. **Benchmark system** with frame timing, draw call counting, GPU memory tracking (`benchmark.cpp`)

---

## Architecture Assessment (Systems Architect)

### Strengths

- Clean separation of concerns across all modules
- Instance sorting by model index is the correct production pattern
- Storage buffer instancing via `SV_InstanceID` is the modern approach
- Single render pass for both pipeline binds avoids unnecessary target switches
- Draw calls scale with model variety (not instance count) - correct for a city builder
- No fundamental architectural changes needed for production integration

### Draw Call Criterion Revision

The original target of "≤10 draw calls" was for a single-model scenario. With multi-model support, the correct criterion is:

**Draw calls = unique_models × 2** (one per model per pass)

At 41 models this is 82 draw calls. This is architecturally correct - you cannot instance across different vertex/index buffer bindings without indirect draws or mega-buffer approaches. The draw call count scales linearly with model variety, not instance count.

---

## Conditions for Production Integration

1. **Add SPIRV compilation** to CMakeLists.txt for Vulkan/Linux support (shader loader already supports it)
2. **Fix `int*` cast** at `toon_pipeline.cpp:369` - use proper intermediate variable
3. **Normal matrix approximation** assumes uniform scaling - document this limitation

---

## What Is Missing (Expected for POC)

These are not in scope for the POC but required for production:

- Frustum culling
- LOD system
- Texture/UV support (currently color-only toon shading)
- Dynamic instance buffer updates for moving entities
- MSAA / post-process anti-aliasing
- Proper GPU memory query API

---

## Key Technical Decisions Validated

| Decision | Validated? |
|----------|-----------|
| SDL_GPU as rendering abstraction | Yes - clean API, good performance |
| HLSL + DXC for shaders | Yes - reliable compilation, SM 6.0 support |
| Storage buffer instancing | Yes - scales to 100k+ without additional draw calls |
| Inverted hull outlines | Yes - clean results, minimal cost |
| cgltf for model loading | Yes - handles multi-primitive glTF correctly |

---

## Assets Used

- **Kenney City Kit Commercial** (CC0 license) - 41 GLB models
  - 14 standard buildings (building-a through building-n)
  - 5 skyscrapers (building-skyscraper-a through building-skyscraper-e)
  - 6 detail props (awnings, overhangs, parasols)
  - 14 low-detail buildings + 2 wide variants
- Vertex counts range from 76 (detail props) to 8,485 (building-j)

---

## Test Hardware

- **GPU:** ASUS ROG Strix RTX 4080
- **Platform:** Windows (D3D12 backend)
- **Resolution:** 1280×720

---

## Files

```
poc/poc1-toon-rendering/
├── CMakeLists.txt
├── main.cpp
├── src/
│   ├── gpu_device.h/cpp
│   ├── shader_loader.h/cpp
│   ├── model_loader.h/cpp
│   ├── gpu_mesh.h/cpp
│   ├── instance_buffer.h/cpp
│   ├── camera.h/cpp
│   ├── toon_pipeline.h/cpp
│   ├── scene.h/cpp
│   └── benchmark.h/cpp
└── shaders/
    ├── toon.vert.hlsl
    ├── toon.frag.hlsl
    ├── outline.vert.hlsl
    └── outline.frag.hlsl
```
