# Debug Session Summary: Terrain Rendering Issue

**Date:** 2026-02-07
**Updated:** Current session
**Issue:** Terrain rendering showing blank screen - NO FRAGMENTS GENERATED

## Current Status

Fragment shader is NOT executing at all. Added debug code to output bright magenta - still black screen. This confirms the problem is in the vertex stage, not fragment.

## Fixes Applied This Session

### 1. Missing Fragment Sampler Binding (FIXED)
- **Error:** `SDL_GPU_CheckGraphicsBindings: "Missing fragment sampler binding!"`
- **Cause:** terrain.frag.hlsl declares shadow sampler/texture at lines 93-94, but nothing was bound
- **Fix:** Added dummy shadow map texture and sampler creation/binding in `renderTerrain()` around line 1443

### 2. Wrong Resource Declaration (FIXED)
- **Error:** `SDL_GPU_CheckGraphicsBindings: "Missing fragment storage texture binding!"`
- **Cause:** `fragResources.numStorageTextures = 1` was wrong - shadow map is a sampled texture, not storage
- **Fix:** Removed that line at ~1131, keeping only `numSamplers = 1`

### 3. Culling Disabled for Testing
- Line 1199: `SDL_GPU_CULLMODE_NONE` (was `SDL_GPU_CULLMODE_BACK`)

## Current Problem: No Fragments Generated

The fragment shader never runs. This means either:
1. All vertices are transformed outside the clip frustum
2. All triangles are degenerate (zero area)
3. Depth test is failing for all fragments

## Files Modified (uncommitted)

### src/app/Application.cpp
- Line ~1128-1130: Changed `fragResources` - removed `numStorageTextures`
- Line 1199: Culling set to NONE
- Lines ~1443-1475: Added dummy shadow map texture/sampler binding
- Lines ~1517-1519: Added cleanup for dummy shadow resources

### assets/shaders/terrain.frag.hlsl
- Lines 364-366: **DEBUG CODE - REMOVE AFTER FIX**
  ```hlsl
  // DEBUG: Output bright magenta to test if fragment shader runs
  output.color = float4(1.0, 0.0, 1.0, 1.0);
  return output;
  ```

## Next Debugging Steps

1. **Add vertex shader debug** - Output fixed clip positions to test if VS runs
2. **Check terrain vertex data** - Log actual positions from TerrainChunkMeshGenerator
3. **Verify camera math** - Print camera position/target and verify terrain is in view
4. **Test with hardcoded triangle** - Bypass terrain mesh, render simple geometry

## Key Locations

| What | File | Lines |
|------|------|-------|
| Pipeline creation | Application.cpp | 1161-1208 |
| Terrain rendering | Application.cpp | 1285-1520 |
| Camera setup | Application.cpp | 1268-1275, 1302-1314 |
| Vertex shader | terrain.vert.hlsl | all |
| Fragment shader | terrain.frag.hlsl | all |
| Vertex format | TerrainVertex.h | all |

## Working Reference

- Demo cube uses same `mul(viewProjection, float4(position, 1.0))` pattern
- Demo uses same matrix calculation: `projection * view`
- Matrix order is CORRECT per System Architect analysis

## Process Note

Before testing, always kill any running sims_3000.exe:
```powershell
Get-Process sims_3000 -ErrorAction SilentlyContinue | Stop-Process -Force
```
