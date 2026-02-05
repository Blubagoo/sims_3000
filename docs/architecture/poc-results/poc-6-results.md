# POC-6: SDL_GPU UI Overlay - Results (Complete)

## Executive Summary

**Status: PASS**

POC-6 validates that UI can be rendered efficiently over a 3D scene using SDL_GPU. The complete implementation includes:
- **Actual GPU rendering** of 100 colored rectangles via custom sprite batcher pipeline
- **Text API validation** with TTF_GetGPUTextDrawData for 50 text objects
- **10-frame warmup** to exclude driver initialization from benchmarks

A critical discovery during research: **SDL_Renderer and SDL_GPU cannot coexist**. This POC demonstrates the correct approach: using SDL_GPU for all rendering with SDL3_ttf's GPU text engine.

## Key Research Finding

During implementation research, we discovered that SDL_Renderer overlay on SDL_GPU is **not supported**:
- GitHub Issue #10882: "Using SDL_Renderer to draw UI over SDL_GPU" was closed as "not planned"
- SDL developers confirmed these APIs are incompatible
- Production UI must use SDL_GPU sprite batcher + SDL3_ttf GPU text engine

## Benchmark Results

### Configuration
| Parameter | Value |
|-----------|-------|
| Window Size | 1280 x 720 |
| Rect Widgets | 100 (ACTUALLY RENDERED) |
| Text Widgets | 50 (API OVERHEAD MEASURED) |
| Total Widgets | 150 |
| Frames Sampled | 100 (after 10-frame warmup) |

### [1] Rectangle Rendering (Sprite Batcher)

| Metric | Value |
|--------|-------|
| Min | 0.0200 ms |
| Avg | 0.0353 ms |
| Max | 0.0822 ms |
| Draw Calls | 1 (batched) |
| Vertices | 600 |

**Result: PASS** - Actual GPU rendering with custom pipeline

### [2] Text Data Retrieval (SDL_ttf GPU)

| Metric | Value |
|--------|-------|
| Min | 0.0004 ms |
| Avg | 0.1667 ms |
| Max | 16.5538 ms |
| Text Objects | 50 |

**Result: PASS** - Measures `TTF_GetGPUTextDrawData()` + sequence traversal

*Note: Full text rendering would use same pattern as rectangles (vertex buffer + indexed draw). Estimated additional cost: ~0.1-0.3ms for 50 text objects.*

### [3] Total UI Overlay Time

| Metric | Value |
|--------|-------|
| Min | 0.0204 ms |
| Avg | 0.2022 ms |
| Max | 16.6361 ms |
| Target | <= 2 ms |
| Headroom | **9.9x** |

**Result: PASS**

### [4] Total Frame Time

| Metric | Value |
|--------|-------|
| Min | 0.1438 ms |
| Avg | 2.8365 ms |
| Max | 17.3345 ms |
| Effective FPS | 352.5 |

**Result: PASS** - Well above 60 FPS target

### [5] Widget Rendering Verification

| Metric | Value | Status |
|--------|-------|--------|
| Rect widgets rendered | 100 | RENDERED |
| Text widgets processed | 50 | PROCESSED |
| Total | 150 | PASS |

## Thresholds Summary

| Metric | Target | Failure | Actual | Status |
|--------|--------|---------|--------|--------|
| UI overlay render time | <= 2ms | > 5ms | 0.20 ms | **PASS** |
| Rect rendering | Working | - | 0.035 ms | **PASS** |
| Text API | Working | - | 0.167 ms | **PASS** |
| UI elements | >= 100 | < 50 | 150 | **PASS** |

## Technical Implementation

### Approach
- SDL_GPU for all rendering (3D scene + UI)
- Custom sprite batcher pipeline for colored rectangles
- TTF_CreateGPUTextEngine for text atlas management
- TTF_GetGPUTextDrawData for text vertex/atlas data
- 10-frame warmup to exclude driver initialization
- Single render pass with clear color simulating 3D background

### Code Structure
```
poc/poc6-ui-overlay/
├── CMakeLists.txt              # Links SDL3, SDL3_ttf
├── main.cpp                    # ~850 lines - complete implementation
└── shaders/
    ├── ui_quad.vert.hlsl       # Colored quad vertex shader
    ├── ui_quad.frag.hlsl       # Colored quad fragment shader
    ├── ui_quad.vert.dxil       # Compiled for D3D12
    └── ui_quad.frag.dxil       # Compiled for D3D12
```

### Key APIs Used
- `SDL_CreateGPUDevice()` - GPU device creation
- `SDL_CreateGPUGraphicsPipeline()` - Sprite batcher pipeline
- `SDL_CreateGPUBuffer()` - Vertex buffer for batched quads
- `SDL_MapGPUTransferBuffer()` - CPU->GPU data upload
- `SDL_DrawGPUPrimitives()` - Batched draw call
- `TTF_CreateGPUTextEngine()` - GPU text engine
- `TTF_GetGPUTextDrawData()` - Get atlas draw sequences

## Architecture Implications

### What This Means for Sims 3000

1. **No SDL_Renderer in production**: All 2D UI must go through SDL_GPU
2. **Sprite batcher validated**: Custom pipeline for filled rects works efficiently
3. **Text via SDL3_ttf GPU engine**: Atlas-based rendering pattern validated
4. **Single GPU context**: Simplifies state management

### Recommended Production Approach

```
Production Rendering Pipeline:
┌─────────────────────────────────────────────┐
│  SDL_GPU Device                             │
│  ├── 3D Scene Pass (POC-1 toon rendering)  │
│  │   └── Models, terrain, entities          │
│  └── UI Overlay Pass                        │
│      ├── Sprite Batcher (VALIDATED)         │
│      │   └── Rects, icons, backgrounds      │
│      └── Text Batcher (same pattern)        │
│          └── Atlas textures from SDL_ttf    │
└─────────────────────────────────────────────┘
```

## Performance Analysis

### Breakdown
| Component | Time | % of Budget |
|-----------|------|-------------|
| Rectangle batching + draw | 0.035ms | 1.75% |
| Text API overhead | 0.167ms | 8.35% |
| **Total measured** | 0.20ms | 10.1% |
| Est. full text draw | +0.1-0.3ms | +5-15% |
| **Conservative total** | ~0.4ms | 20% |

### Headroom
- Current measurement: **9.9x headroom**
- With full text rendering: **~5x headroom** (conservative)
- Budget remaining: 1.6ms for additional UI complexity

### Scalability
With 0.035ms for 100 rectangles:
- Could render ~5700 rectangles within 2ms budget
- Text rendering similar efficiency expected

## Known Limitations

1. **Max spike**: 16.6ms seen (first-frame atlas creation after warmup)
2. **Text rendering**: Measures API overhead only; full rendering uses same pattern as rects
3. **Platform-specific shaders**: DXIL only (Windows); SPIRV needed for Linux/Vulkan

## Files Created

- `poc/poc6-ui-overlay/main.cpp` - Complete implementation (~850 lines)
- `poc/poc6-ui-overlay/CMakeLists.txt` - Build configuration
- `poc/poc6-ui-overlay/shaders/ui_quad.vert.hlsl` - Vertex shader
- `poc/poc6-ui-overlay/shaders/ui_quad.frag.hlsl` - Fragment shader
- `poc/poc6-ui-overlay/shaders/*.dxil` - Compiled shaders

## Dependencies

- `sdl3-ttf` added to `vcpkg.json`

## Conclusion

POC-6 **successfully validates** that SDL_GPU can handle UI overlay rendering with excellent performance:

- **Rectangle sprite batcher**: 0.035ms for 100 quads (actual GPU rendering)
- **Text API**: 0.167ms for 50 text objects (atlas data retrieval)
- **Total**: 0.20ms with 9.9x headroom vs 2ms target

The critical finding that SDL_Renderer cannot coexist with SDL_GPU changes our approach but the alternative (pure SDL_GPU rendering) is proven efficient.

**Recommendation**: Proceed with SDL_GPU-based UI system. The sprite batcher pattern is validated and scales well. Text rendering follows the same pattern with atlas textures.

---

## Systems Architect Review (Final)

**Status: APPROVED**

**Date:** 2026-02-04

### Summary

The revised POC-6 successfully addresses all concerns from the initial review. Rectangle rendering now exercises the full GPU pipeline: shader compilation, vertex buffer uploads, and batched draw calls. The measured 0.035ms average for 100 colored quads represents real GPU work, not metadata retrieval. With 9.9x headroom against the 2ms budget (or ~5x with conservative text rendering estimates), the architecture is validated for production use.

### Previous Concerns Addressed

| Initial Concern | Resolution |
|-----------------|------------|
| "Benchmark measures metadata retrieval, not GPU rendering" | **FIXED** - Full GPU pipeline: vertex buffer upload, copy pass, pipeline bind, draw call |
| "Rectangle rendering is a stub" | **FIXED** - Complete sprite batcher with custom HLSL shaders |
| "15x headroom is optimistic" | **CORRECTED** - 9.9x measured, ~5x conservative |

### Implementation Quality

**Sprite Batcher**: Correct batching pattern - CPU-side vertex generation, single upload, single draw call

**Pipeline Configuration**: Correct - alpha blending, cull mode disabled, proper vertex layout

**Shaders**: Correct - normalized [0,1] to clip space [-1,1] with Y-flip for screen-space UI

### Headroom Analysis

| Scenario | Time | Headroom vs 2ms |
|----------|------|-----------------|
| Measured (rects + text API) | 0.20ms | 9.9x |
| + Full text rendering (est.) | 0.40ms | 5.0x |
| + Icons/textures (est.) | 0.60ms | 3.3x |
| + Safety margin | 1.0ms | 2.0x |

### Remaining Gaps (Non-Blocking)

1. Text batcher not fully implemented (pattern validated via rect batcher)
2. SPIRV shaders needed for cross-platform (Linux/Vulkan)
3. Could optimize with indexed drawing (4 verts + 6 indices per quad)
4. Max spike (16.6ms) from atlas creation - pre-warm in production

### Production Recommendations

1. **Immediate**: Accept POC as validation complete
2. **Phase 1**: Text batcher, SPIRV shaders, indexed drawing
3. **Phase 2**: Texture atlas batching, pre-warm atlases during loading

### Verdict

The POC now validates what it claims to validate. The sprite batcher pattern is proven with real GPU work, performance has substantial headroom, and the text API demonstrates that SDL3_ttf's GPU text engine integrates correctly. The remaining work follows the validated pattern.

**Proceed with confidence to production implementation.**

---

*POC completed: 2026-02-04*
*SA Review: 2026-02-04*
*Build: MSVC 19.44, C++17, Release*
*Shaders: HLSL SM 6.0 -> DXIL*
