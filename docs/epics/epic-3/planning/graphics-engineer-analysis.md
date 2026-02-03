# Graphics Engineer Analysis: Epic 3 -- Terrain System

**Epic:** 3 -- Terrain System
**Analyst:** Graphics Engineer
**Date:** 2026-01-28
**Canon Version Referenced:** 0.5.0

---

## Overview

Epic 3 is the first system that consumes Epic 2's rendering pipeline at scale. The terrain is the visual foundation of every frame -- it covers the entire visible area and must look correct, perform well, and integrate seamlessly with the toon shader + bloom pipeline established in Epic 2. Every other visual system (buildings, roads, effects) sits on top of terrain.

This analysis focuses exclusively on the rendering and GPU aspects of terrain. TerrainSystem owns tile data (type, elevation) and procedural generation. RenderingSystem owns terrain rendering -- meaning this epic's visual output depends on data contracts between the two.

**Key constraints from Epic 2 that terrain must respect:**
- Perspective projection (~35 deg FOV) with free camera
- Toon shader: 3-4 lighting bands, world-space directional light, ambient term, emissive term
- Mandatory bloom post-process for all emissive sources
- Per-instance emissive_intensity and emissive_color
- Instanced rendering with chunked instancing for 512x512 maps
- 500-1000 draw call budget total (terrain + buildings + everything else)
- <100k visible triangles budget
- <8ms total render budget at 60fps
- Dark clear color {0.02, 0.02, 0.05} -- terrain IS the lit surface
- Normal-based edge detection (primary) for outlines
- LOD framework (2+ levels) and spatial partitioning (16x16 or 32x32 cell grid)

---

## Terrain Mesh Generation

### Heightmap to Mesh Strategy

The terrain is a tile-based heightmap with 32 elevation levels. Each tile occupies 1x1 world units on the XZ plane. Elevation maps to world Y via `elevation * ELEVATION_HEIGHT` (0.25 per level from Epic 2 ticket 2-033, yielding 0-7.75 world units total height range).

**Approach: Chunked Heightmap Mesh**

Rather than rendering individual tile quads (which would require 262,144 draw calls for a 512x512 map), terrain is generated as contiguous mesh chunks.

```
Chunk Layout:
- Chunk size: 32x32 tiles (1,024 tiles per chunk)
- 128x128 map = 4x4 = 16 chunks
- 256x256 map = 8x8 = 64 chunks
- 512x512 map = 16x16 = 256 chunks
```

Each chunk is a single mesh with shared vertices at tile corners. This allows:
- One draw call per visible chunk (not per tile)
- Smooth normals across tile boundaries within a chunk
- Efficient GPU vertex buffer usage
- Natural alignment with Epic 2's spatial partitioning (32x32 cell grid)

**Vertex Format:**

```cpp
struct TerrainVertex {
    float3 position;      // World-space position (x, y=elevation, z)
    float3 normal;        // Calculated from adjacent elevations
    float2 texcoord;      // UV for terrain texture/atlas lookup
    float2 tile_coord;    // Integer tile coordinate (for terrain type lookup)
    uint32_t terrain_type; // Packed terrain type ID (0-9)
};
```

The `terrain_type` per-vertex allows the fragment shader to select the correct visual treatment. At tile boundaries where types differ, this creates hard edges -- which is desirable for toon shading (no blending between terrain types).

**Vertex count per chunk:**
- 32x32 tiles = 33x33 vertices = 1,089 vertices (shared corners)
- With additional vertices for cliff faces: up to ~2,500 per chunk worst-case
- 256 chunks max (512x512) = ~640k total vertices stored, but only visible chunks rendered

**Index count per chunk:**
- 32x32 tiles x 2 triangles x 3 indices = 6,144 indices for flat terrain
- Additional triangles for cliff faces

### Normal Calculation

Normals are computed from the heightmap using central differences:

```
normal.x = height(x-1, z) - height(x+1, z)
normal.z = height(x, z-1) - height(x, z+1)
normal.y = 2.0 * ELEVATION_HEIGHT
normalize(normal)
```

This produces smooth shading across elevation gradients. For the toon shader, these normals drive the stepped lighting bands -- shallow slopes get full light, steep faces shift toward shadow. This is essential for terrain readability.

### Cliff Face Generation

When adjacent tiles differ by more than a configurable threshold (recommended: 2+ elevation levels), generate vertical cliff face geometry:

```
If abs(elevation[x][z] - elevation[x+1][z]) > CLIFF_THRESHOLD:
    Insert vertical quad(s) between the two elevation levels
    Normal = horizontal (pointing away from cliff face)
```

Cliff faces render as hard vertical surfaces with their own normals, producing strong shadow bands in the toon shader. This makes elevation changes visually prominent.

### Mesh Rebuild Strategy

Terrain mesh chunks must be rebuilt when:
- Terrain modification occurs (clearing, leveling) -- rebuild affected chunk only
- Sea level changes -- rebuild all water-adjacent chunks
- Initial generation -- build all chunks

Rebuild is per-chunk, not whole-map. A dirty flag per chunk tracks which need regeneration. GPU buffer upload uses `SDL_UploadToGPUBuffer` with staged transfer.

---

## Terrain Visual Pipeline

### Integration with Epic 2 Toon Shader

Terrain uses the same toon shader pipeline as all other geometry (ticket 2-005), but with terrain-specific uniforms and material properties.

**Render pass order (from ticket 2-019):**
1. Opaque geometry (terrain is FIRST in this pass)
2. Edge detection post-process
3. Sorted transparents (water surface is here)
4. Bloom post-process
5. UI overlay

Terrain renders as opaque geometry in the terrain render layer (RenderLayer::TERRAIN = 1). Water surfaces render in the water layer (RenderLayer::WATER = 2) as semi-transparent with special treatment.

**Terrain Shader Variant:**

The toon shader (ticket 2-005) defines the lighting model. Terrain extends this with a terrain-specific fragment shader that:

1. Reads `terrain_type` from vertex data
2. Looks up base color from a terrain color palette (uniform buffer or small texture)
3. Looks up emissive color and intensity from TerrainVisualConfig (ticket 2-008)
4. Applies toon lighting bands using the shared lighting model
5. Adds emissive term (unaffected by lighting)
6. Outputs to both color target and emissive target (for bloom extraction)

```hlsl
// Terrain fragment shader additions to base toon shader
cbuffer TerrainVisuals : register(b3) {
    float4 base_colors[10];      // Per terrain type
    float4 emissive_colors[10];  // Per terrain type (rgb = color, a = intensity)
    float  glow_time;            // For animated glow (pulse)
    float  sea_level;            // For water edge detection
};

// In fragment shader:
uint type_id = input.terrain_type;
float3 base_color = base_colors[type_id].rgb;
float3 emissive = emissive_colors[type_id].rgb * emissive_colors[type_id].a;

// Apply toon lighting to base color
float3 lit_color = base_color * toonFactor * lightColor + ambientColor * base_color;

// Add emissive (unaffected by lighting)
float3 final_color = lit_color + emissive;
```

### MRT (Multiple Render Targets) for Bloom

Per ticket 2-007, MRT is considered for separating emissive channel. If MRT is used, the terrain shader writes:
- RT0: Final lit color (color target)
- RT1: Emissive color only (bloom extraction target)

If MRT is not used, bloom extraction uses a brightness threshold pass on the final image. Either approach works, but MRT gives cleaner bloom extraction for terrain (which has many low-intensity emissive surfaces).

---

## Per-Terrain-Type Visuals

Each of the 10 terrain types needs a distinct visual identity under the toon shader. The key differentiators are: base color, emissive color, emissive behavior, and surface detail.

### Classic Terrain Types

| Type | Base Color (Dark Tone) | Visual Approach |
|------|----------------------|-----------------|
| **flat_ground** (substrate) | Dark grey-brown (#1A1A14) | Subtle bioluminescent moss -- very faint teal glow in crevices. Mostly serves as neutral backdrop. Minimal emissive. |
| **hills** (ridges) | Dark slate-blue (#1C1E2A) | Layered rock appearance. Glowing vein patterns running along slope contours. Emissive concentrated in crevices using normal-based masking (areas where normal.y is low = crevice = glow). |
| **water_ocean** (deep_void) | Deep dark blue (#050A14) | See Water Rendering section. Separate mesh/shader. |
| **water_river** (flow_channel) | Dark blue-teal (#081418) | See Water Rendering section. Animated flow. |
| **water_lake** (still_basin) | Dark blue (#0A1020) | See Water Rendering section. Calm surface. |
| **forest** (biolume_grove) | Dark green (#0A1A0A) | See Vegetation Rendering section. Glowing tree/fungi models. |

### Alien Terrain Types

| Type | Base Color (Dark Tone) | Visual Approach |
|------|----------------------|-----------------|
| **crystal_fields** (prisma_fields) | Dark purple (#1A0A2A) | Bright magenta/cyan crystal spires as instanced models on terrain surface. Terrain base is dark, crystals provide strong point-source glow. Highest terrain emissive intensity. |
| **spore_plains** (spore_flats) | Dark teal-green (#0A1814) | Pulsing green/teal glow on terrain surface. Animated emissive intensity (sine wave pulse, 2-4 second period). Spore particle effect layer on top. |
| **toxic_marshes** (blight_mires) | Dark olive-yellow (#14140A) | Sickly yellow-green glow. Bubbling surface animation via vertex displacement or texture scrolling. Volumetric fog effect as transparent layer. |
| **volcanic_rock** (ember_crust) | Very dark grey (#0F0A0A) | Orange/red glow in cracks. Normal-based crack detection: where surface normal deviates from vertical, glow increases (cracks in the rock). High contrast dark obsidian vs bright orange. |

### Terrain Type Distinction Strategy

With only toon shading (no PBR textures), terrain types are distinguished by:

1. **Base color** -- Each type has a unique dark hue
2. **Emissive color** -- Unique glow hue per type (cyan, green, magenta, orange, yellow-green, teal, etc.)
3. **Emissive behavior** -- Static, pulsing, or reactive
4. **Surface detail** -- Instanced decoration models (crystals, trees, spore clouds)
5. **Normal pattern** -- How the toon shader bands interact with surface geometry (flat vs hilly)

**No individual textures per terrain type.** Instead, use a small color palette lookup (uniform buffer with 10 entries). This is far cheaper than texture atlases and fits the toon aesthetic (flat color regions with glow accents). If more visual detail is needed later, a single terrain detail texture atlas can be added.

---

## Bioluminescent Properties

### Per-Type Emissive Specification

From patterns.yaml art_direction and terrain_types, refined for the toon shader pipeline:

| Terrain Type | Emissive Color (RGB) | Intensity | Behavior | Bloom Contribution |
|-------------|---------------------|-----------|----------|-------------------|
| flat_ground | Teal (#00443A) | 0.05 | Static | Minimal -- barely visible bloom |
| hills | Cyan-blue (#004466) | 0.10 | Static | Low -- visible in crevices only |
| water_ocean | Soft blue-white (#3366AA) | 0.15 | Slow pulse (6s period) | Medium -- shore glow |
| water_river | Teal (#00D4AA) | 0.20 | Flow animation (scrolling) | Medium |
| water_lake | Blue-white (#4488CC) | 0.15 | Gentle pulse (8s period) | Medium |
| forest | Green-teal (#00AA66) | 0.25 | Subtle pulse (4s period) | Medium -- canopy glow |
| crystal_fields | Magenta/cyan (#FF44FF / #00FFDD) | 0.60 | Shimmer (random flicker) | High -- strong bloom |
| spore_plains | Green (#44FF88) | 0.35 | Rhythmic pulse (3s period) | Medium-high |
| toxic_marshes | Yellow-green (#88CC00) | 0.30 | Irregular bubble pulse | Medium-high -- eerie |
| volcanic_rock | Orange-red (#FF4400) | 0.40 | Slow throb (5s period) | High -- crack glow |

### Glow Behavior Implementation

Animated glow behaviors are implemented in the terrain fragment shader using a time uniform:

```hlsl
float calculate_glow_intensity(uint terrain_type, float base_intensity, float time) {
    switch (terrain_type) {
        case FLAT_GROUND:  return base_intensity; // Static
        case HILLS:        return base_intensity; // Static
        case WATER_OCEAN:  return base_intensity * (0.8 + 0.2 * sin(time * 1.047)); // 6s
        case WATER_RIVER:  return base_intensity; // Flow handled by UV scroll
        case WATER_LAKE:   return base_intensity * (0.85 + 0.15 * sin(time * 0.785)); // 8s
        case FOREST:       return base_intensity * (0.9 + 0.1 * sin(time * 1.571)); // 4s
        case CRYSTAL:      return base_intensity * (0.7 + 0.3 * frac(sin(dot(input.tile_coord, float2(12.9898, 78.233))) * 43758.5453 + time * 3.0)); // shimmer
        case SPORE_PLAINS: return base_intensity * (0.7 + 0.3 * sin(time * 2.094)); // 3s
        case TOXIC_MARSH:  return base_intensity * (0.6 + 0.4 * abs(sin(time * 1.5 + sin(time * 0.7) * 2.0))); // irregular
        case VOLCANIC:     return base_intensity * (0.8 + 0.2 * sin(time * 1.257)); // 5s
    }
}
```

### Glow Intensity Hierarchy

From ticket 2-037, the hierarchy is: player structures > terrain features > background.

Terrain emissive intensities are calibrated to sit below building glow (buildings at 0.5-1.0 when powered). The strongest terrain glow is crystal_fields at 0.60, still below a powered building. This ensures buildings "pop" on top of terrain.

### Transition Behavior

When terrain is modified (cleared, leveled), the emissive properties transition smoothly over ~0.5s (per ticket 2-037). This is handled by interpolating the per-instance emissive_intensity in the instance buffer. The TerrainSystem signals a terrain change event; the RenderingSystem interpolates the visual transition.

---

## Water Rendering

Water (ocean, river, lake) requires special rendering treatment because it is:
- Semi-transparent (you should see the dark depth beneath)
- Animated (rivers flow, lakes pulse)
- A source of bioluminescent glow (shore glow, particle effects)
- Rendered in the WATER layer (after terrain, before buildings)

### Recommended Approach: Stylized Toon Water

Given the toon aesthetic, do NOT attempt realistic water rendering (screen-space reflections, refraction, caustics). Instead:

**Water Surface Mesh:**
- Separate mesh from terrain (not part of terrain chunks)
- Flat plane at sea_level elevation (or per-water-body elevation for lakes)
- Rendered as semi-transparent with alpha ~0.7-0.8
- Depth test ON, depth write OFF (so terrain beneath is visible where alpha < 1.0)

**Water Visual Effect:**
1. **Base color:** Very dark blue/teal (barely visible without glow)
2. **Scrolling UV animation:** Slow scroll for rivers (direction-aware), gentle distortion for ocean/lakes
3. **Emissive shoreline:** Where water meets land, increase emissive intensity. Detect via distance from water edge (encoded per-vertex or computed in shader). Creates the bioluminescent shore glow effect.
4. **Surface pattern:** Simple sine-wave normal perturbation for subtle wave highlights in toon shader
5. **No reflections:** Not needed for toon style, expensive, and doesn't match the aesthetic
6. **No refraction:** Not needed -- dark water obscures depth naturally

**Shoreline Glow:**

The most visually important water feature is the emissive shoreline. Where water tiles are adjacent to land tiles, a glow fringe should appear. Implementation options:

- **Option A (recommended): Per-vertex shoreline attribute.** When generating water mesh, vertices adjacent to land tiles get a `shore_factor` (0.0-1.0). The shader uses this to boost emissive intensity at shores.
- **Option B: Screen-space shore detection.** Depth difference between water and terrain in depth buffer. More expensive, harder to tune.

**River Flow:**
- UV coordinates scroll in the river flow direction
- Flow direction stored per-tile by TerrainSystem
- Shader reads flow_direction uniform and scrolls UVs accordingly

**Separate Draw Calls for Water Types:**
- Ocean: 1 draw call (single large mesh at map edges)
- Rivers: 1 draw call per river segment chunk (or batched)
- Lakes: 1 draw call per lake body
- Total water draw calls: estimated 5-15 depending on map

### Water Particles (Deferred)

Bioluminescent particles floating above water surfaces would enhance the visual. This is a particle effect system feature, likely deferred to a later polish epic. The terrain system should provide water surface positions for future particle emitters.

---

## Vegetation Rendering

Forest (biolume_grove) tiles need 3D vegetation on the terrain surface. This is a key visual differentiator -- forest tiles should look dramatically different from flat ground.

### Recommended Approach: Instanced Low-Poly Models

**NOT billboard sprites.** With free camera (all viewing angles), billboards would be obvious. Instead:

1. **Low-poly tree/fungi models:** 3-5 model variants, hand-modeled as glTF assets
   - Low vertex count: 50-200 triangles per model
   - Alien design: bioluminescent mushroom trees, glowing tendrils, crystal-tipped flora
   - Each model has emissive material regions (glowing caps, vein patterns)

2. **Instanced placement:** Each forest tile places 2-4 vegetation instances
   - Position: random jitter within tile bounds (seeded by tile coordinate for determinism)
   - Rotation: random Y-axis rotation (seeded)
   - Scale: slight random variation (0.8-1.2x)
   - Per-instance emissive_color and emissive_intensity from TerrainVisualConfig

3. **Render as instances:** All vegetation of the same model variant batched into one instanced draw call
   - 5 model variants x 1 draw call each = 5 draw calls for ALL forest on screen
   - Instance buffer contains: model matrix, emissive_color, emissive_intensity

**Vegetation Triangle Budget:**
- Average 150 triangles per tree, 3 per tile
- 128x128 map with ~15% forest = ~2,500 tiles = ~1.1M total triangles stored
- But only visible forest rendered (frustum culled)
- At typical zoom, ~50-200 forest tiles visible = 22k-90k vegetation triangles
- This fits within the <100k visible triangle budget alongside terrain

### Crystal Fields, Spore Plains

Crystal fields and spore plains use the same instanced model approach:

- **Crystal fields:** 2-3 crystal spire model variants. Higher poly (~200-300 triangles) for the distinct crystal shape. Strong emissive. 1-3 per tile.
- **Spore plains:** Terrain surface glow (shader-based) plus optional small spore emitter models. Low poly (~30-50 triangles). Dense placement (4-6 per tile) since they are small.
- **Toxic marshes:** No vegetation models. Visual effect is shader-based (terrain color, glow, optional fog layer).
- **Volcanic rock:** No vegetation. Emissive cracks via normal-based shader effect.

### LOD for Vegetation

At far distances (high zoom out), individual vegetation models are too small to see. LOD strategy:

- **LOD 0 (close):** Full vegetation models rendered
- **LOD 1 (far):** Vegetation models removed. Forest tiles use boosted terrain emissive to maintain color impression at distance. Essentially "colored glow dots" on the terrain surface.

This is a natural LOD transition because the glow is the most visible feature at distance anyway.

---

## Key Work Items

- [ ] Item 1: **Terrain Chunk Mesh Generator** -- Generate 32x32 tile terrain mesh chunks from heightmap data. Includes vertex position, normal, UV, tile_coord, and terrain_type per vertex. Handles elevation deltas and cliff face generation. Dirty flag per chunk for incremental rebuild.
- [ ] Item 2: **Terrain Fragment Shader Variant** -- Extend the base toon shader (ticket 2-005) with terrain-specific uniforms: per-type base color palette, per-type emissive color/intensity, animated glow behaviors (time-based), and normal-based crevice glow for hills/volcanic.
- [ ] Item 3: **TerrainVisualConfig Integration** -- Connect TerrainVisualConfig (ticket 2-008 defines per-terrain emissive presets) to the terrain shader uniform buffer. 10 terrain type entries with base_color, emissive_color, emissive_intensity, glow_behavior_type.
- [ ] Item 4: **Water Surface Mesh and Shader** -- Separate water mesh generation for ocean/river/lake. Semi-transparent rendering (alpha 0.7-0.8, depth test on, depth write off). Scrolling UV animation for rivers, sine-wave normal perturbation for surface highlights, per-vertex shoreline glow factor.
- [ ] Item 5: **Vegetation Instance System** -- Load 3-5 low-poly glTF vegetation models. Generate per-tile instance data (position jitter, rotation, scale, emissive) for forest tiles. Render all instances of each model variant in a single instanced draw call. Support crystal_fields and spore_plains similarly.
- [ ] Item 6: **Terrain LOD Integration** -- Integrate terrain chunks with Epic 2's LOD framework (ticket 2-049). LOD 0: full mesh + vegetation. LOD 1: simplified mesh (fewer subdivisions per chunk) + no vegetation (boosted emissive instead). LOD transition distances configurable per map size.
- [ ] Item 7: **Terrain Chunk Culling Integration** -- Register terrain chunks with Epic 2's spatial partitioning system (ticket 2-050). Each 32x32 chunk maps naturally to one spatial cell. Frustum culling skips entire chunks. Provide terrain AABB per chunk (includes max elevation).
- [ ] Item 8: **Cliff Face and Slope Rendering** -- Generate cliff geometry for steep elevation transitions. Configure cliff threshold. Cliff normals oriented horizontally for distinct shadow bands. Visual distinction between gentle slopes and sheer cliffs.
- [ ] Item 9: **Terrain Modification Visual Update** -- When TerrainSystem modifies terrain (clear, level, type change), rebuild affected chunk mesh. Interpolate emissive transitions over 0.5s. Update vegetation instances for affected tiles. Handle sea level changes across all water-adjacent chunks.
- [ ] Item 10: **Terrain Edge Detection Tuning** -- Ensure Epic 2's edge detection post-process (ticket 2-006) produces correct outlines on terrain. Normal-based edges should highlight: terrain type boundaries, cliff edges, water shorelines. Depth-based edges should not produce artifacts at gentle slopes. May require terrain-specific edge detection threshold tuning.
- [ ] Item 11: **Water Flow Direction Data** -- Define data contract for river flow direction per tile. TerrainSystem provides flow direction; rendering uses it for UV scroll direction. Flow direction encoded as 2D vector or 8-direction enum.
- [ ] Item 12: **Terrain Debug Visualization** -- Extend debug grid overlay (ticket 2-040) with terrain-specific debug modes: elevation heatmap, terrain type colormap, chunk boundary visualization, LOD level visualization, normals visualization.

---

## LOD & Culling Integration

### Chunk-Based Culling

Terrain chunks (32x32 tiles) align with Epic 2's spatial partitioning cells. Each chunk has an AABB:
- Min: (chunk_x * 32, 0, chunk_z * 32)
- Max: (chunk_x * 32 + 32, max_elevation_in_chunk * 0.25, chunk_z * 32 + 32)

Frustum culling (ticket 2-026) tests chunk AABBs against camera frustum. At typical zoom levels:

| Map Size | Total Chunks | Visible Chunks (typical) | Culled % |
|----------|-------------|-------------------------|----------|
| 128x128 | 16 | 8-16 | 0-50% |
| 256x256 | 64 | 12-30 | 53-81% |
| 512x512 | 256 | 15-40 | 84-94% |

Larger maps benefit most from culling -- on a 512x512 map, only ~6-15% of terrain chunks are visible at any given time.

### Terrain LOD Levels

**LOD 0 (Full Detail):** Default terrain mesh. 32x32 tiles = 33x33 = 1,089 vertices per chunk. All cliff faces generated. Vegetation models rendered.

**LOD 1 (Reduced Detail):** Every other vertex dropped. Effective 16x16 grid per chunk = 17x17 = 289 vertices. No cliff faces (smooth interpolation). No vegetation models -- terrain emissive boosted to compensate. ~4x fewer triangles.

**LOD 2 (Minimal, for 512x512 far distance):** 8x8 grid per chunk = 81 vertices. Flat shaded. Terrain appears as colored glow patches at extreme distance.

**LOD distance thresholds (configurable):**
- LOD 0 to LOD 1: camera distance > 80 units (~5 chunk widths)
- LOD 1 to LOD 2: camera distance > 160 units (~10 chunk widths)

These thresholds should be tuned per map size. On 128x128 maps, LOD 2 may never trigger.

### Vegetation LOD

Vegetation instances only render at LOD 0. At LOD 1+, the vegetation is represented by boosted terrain emissive on forest/crystal tiles. This is a dramatic simplification but at LOD 1 distances individual trees are sub-pixel anyway.

---

## Performance Budget

### Draw Call Budget (Terrain's Share)

Epic 2 sets a total budget of 500-1000 draw calls. Terrain should consume no more than 20-30% of this budget, leaving the majority for buildings (which have far more variety).

| Component | Draw Calls | Notes |
|-----------|-----------|-------|
| Terrain chunks (visible) | 15-40 | One per visible chunk |
| Water surfaces | 5-15 | Ocean + rivers + lakes |
| Vegetation instances | 5-15 | One per model variant (instanced) |
| **Terrain Total** | **25-70** | Well within 30% of 500-1000 budget |

### Triangle Budget (Terrain's Share)

| Component | Triangles (Typical View) | Notes |
|-----------|------------------------|-------|
| Terrain mesh (LOD 0) | 20k-40k | ~2,000 triangles per visible chunk |
| Terrain mesh (LOD 1) | 2k-8k | Distant chunks |
| Water surfaces | 1k-5k | Simple planes |
| Vegetation (LOD 0 chunks) | 15k-45k | 150 tri avg x 3 per tile x visible tiles |
| **Terrain Total** | **38k-98k** | At the high end, pushes against 100k budget |

At maximum zoom-out on a 512x512 map, LOD reduces triangle count substantially. The 100k budget is achievable with proper LOD.

**Risk:** At close zoom with dense forest, vegetation triangles could exceed budget. Mitigation: reduce vegetation density at close zoom if needed (counterintuitive but close-up individual trees take more screen space and need fewer total to fill view).

### GPU Memory Budget (Terrain's Share)

From Epic 2's 512MB minimum / 1GB recommended GPU memory budget:

| Resource | Memory | Notes |
|----------|--------|-------|
| Terrain vertex buffers (all chunks) | 8-32 MB | ~2.5k vertices x 40 bytes x 16-256 chunks |
| Terrain index buffers | 2-8 MB | ~6k indices x 4 bytes x 16-256 chunks |
| Water meshes | 1-4 MB | Simpler geometry |
| Vegetation model buffers | 2-5 MB | 5 models, low-poly, shared across instances |
| Vegetation instance buffers | 4-16 MB | Per-tile instance data for forest/crystal tiles |
| Terrain uniform buffers | <1 MB | Color palette, emissive config |
| **Terrain Total** | **17-66 MB** | ~3-13% of min spec 512MB |

This is well within budget. Terrain is geometry-heavy but texture-light (no large texture atlases needed thanks to the toon color palette approach).

### Per-Map-Size Summary

| Map Size | Chunks | Max Draw Calls | Max Triangles | GPU Memory |
|----------|--------|---------------|---------------|------------|
| 128x128 | 16 | ~30 | ~40k | ~17 MB |
| 256x256 | 64 | ~50 | ~65k | ~35 MB |
| 512x512 | 256 | ~70 | ~98k | ~66 MB |

---

## Elevation Rendering

### 32-Level Elevation System

32 levels at 0.25 world units per level = 7.75 world units total height range. With 1.0 unit tile size, maximum slope is ~7.75:1 over a single tile boundary (if adjacent tiles differ by 31 levels). In practice, procedural generation should produce smooth terrain with rare extreme cliffs.

### Slope Visualization

The toon shader naturally visualizes slopes through its lighting bands:
- **Flat terrain** (normal pointing up): Receives full light from the world-space sun direction. Appears in the brightest toon band.
- **Gentle slopes** (normal tilted slightly): Still bright, possibly dropping one band.
- **Steep slopes** (normal tilted significantly): Drop into shadow bands. The steeper the slope, the darker the toon shading.
- **Cliff faces** (normal pointing sideways): Appear in deep shadow or shadow band, creating strong visual contrast with flat terrain above and below.

This is a natural consequence of the toon shader and requires no special effort -- elevation is automatically communicated through lighting.

### Cliff Face Treatment

Cliff faces (vertical or near-vertical surfaces from large elevation differences) receive special treatment:

1. **Separate geometry** with horizontal normals
2. **Emissive crevice glow** on volcanic_rock and hills terrain types, driven by normal deviation:
   ```hlsl
   float crevice_factor = 1.0 - abs(normal.y); // 0 at top, 1 at vertical
   float glow = emissive_intensity * crevice_factor;
   ```
3. **Edge detection emphasis** -- cliff edges produce strong normal discontinuities, which the edge detection post-process (ticket 2-006) renders as bold outlines
4. **Elevation labels** -- at close zoom, elevation levels could be subtly indicated by alternating slight color shifts every 8 levels (visual "strata" bands)

### Building Placement on Slopes

TerrainSystem determines buildability (not RenderingSystem), but the visual consequence of placing buildings on sloped terrain needs handling:
- Buildings snap to the highest elevation corner of their footprint
- Terrain beneath buildings is visually flattened (the building model sits on a platform)
- This is a visual-only adjustment -- the TerrainSystem still stores the original elevation data

This is likely an Epic 4 (Zoning) concern for implementation, but terrain rendering must support the concept of "terrain modification for building placement."

---

## Questions for Other Agents

### @systems-architect:

- **Q1: Terrain data contract.** What is the exact data structure TerrainSystem provides to RenderingSystem? I propose: `TerrainTileData { uint8_t terrain_type; uint8_t elevation; uint8_t flags; }` per tile, accessible via `get_tile(x, z)`. What flags are needed? (water flow direction, cleared state, buildable state?)

- **Q2: Chunk dirty notification.** How does TerrainSystem notify RenderingSystem that a chunk needs visual rebuild? Event-based (`TerrainModifiedEvent` with GridRect of affected tiles) or dirty flag polling? Event is preferred for efficiency.

- **Q3: Vegetation placement data.** Does TerrainSystem provide exact vegetation instance positions, or just "this tile is forest"? I recommend TerrainSystem provides only tile type, and RenderingSystem deterministically generates instance placement using a seeded PRNG (seed = tile coordinate hash). This avoids syncing per-tree positions over the network.

- **Q4: Water body identification.** Does TerrainSystem identify discrete water bodies (lake_1, river_segment_3) or just tile types? For rendering, I need to know which water tiles form a contiguous body (for single-mesh generation) and river flow direction per tile.

- **Q5: Memory layout.** For 512x512 maps (262,144 tiles), the terrain data array is ~768KB at 3 bytes per tile. Should this be row-major or chunk-major in memory? Chunk-major is better for rendering (locality when rebuilding one chunk), but row-major may be simpler for game logic.

### @game-designer:

- **Q6: Water transparency.** Should water be transparent enough to see terrain beneath (dark depth), or opaque with surface-only glow? I recommend semi-transparent (alpha 0.7-0.8) for the bioluminescent effect -- the dark depth beneath makes the surface glow more dramatic.

- **Q7: Crystal field density.** How dense are crystal formations? 1-3 large crystal spires per tile, or 5-10 small ones? This affects triangle budget significantly. I recommend 1-3 large for visual impact and performance.

- **Q8: Terrain type transition boundaries.** Should there be any visual blending at boundaries between terrain types (e.g., grass fading to sand), or hard boundaries? Hard boundaries fit the toon aesthetic better and are much simpler to implement. The edge detection post-process will draw outlines at type boundaries.

- **Q9: Sea level visual.** Should there be a visible "waterline" effect where water meets land? I recommend yes -- a glowing shoreline fringe (emissive boost at water-land boundary) is one of the strongest bioluminescent visual opportunities.

- **Q10: Toxic marsh visual priority.** The toxic_marshes description mentions "bubbling surface" and "dark fog." Should the fog be a transparent overlay (screen-space or volumetric), or just terrain color with glow? Volumetric fog is expensive. I recommend terrain-level glow plus optional transparent fog billboards as a later polish item.

---

## Risks & Concerns

### Risk 1: Terrain Triangle Budget at Close Zoom with Dense Vegetation
**Severity:** Medium
**Description:** At close zoom on a forest-heavy area, vegetation instances could push past the 100k triangle budget. A 256x256 map with 20% forest has ~13,000 forest tiles; even with culling, 200+ forest tiles visible at close zoom means 90k+ vegetation triangles alone.
**Mitigation:** LOD vegetation aggressively. Close-zoom LOD could reduce tree poly count. Add a configurable vegetation density cap. Monitor with render statistics (ticket 2-042).

### Risk 2: Terrain Mesh Rebuild Performance on Modification
**Severity:** Medium
**Description:** When players clear or level terrain, the affected chunk mesh must be rebuilt and re-uploaded to GPU. For a 32x32 chunk, this is ~5k vertices + index buffer. If multiple chunks are modified in quick succession (e.g., drag-leveling terrain), this could stall the GPU.
**Mitigation:** Rebuild at most 1 chunk per frame. Queue dirty chunks and process them over multiple frames. Use staged GPU buffer upload to avoid stalls.

### Risk 3: Water Rendering Depth Sorting with Transparents
**Severity:** Low-Medium
**Description:** Water surfaces are semi-transparent and rendered in the WATER layer between terrain and buildings. If buildings are partially submerged or at water's edge, depth sorting between water surface and building base may produce artifacts.
**Mitigation:** Water depth test ON but depth write OFF ensures buildings behind water are occluded correctly but buildings at water level render on top. If needed, a separate water depth pass can handle edge cases.

### Risk 4: Per-Terrain-Type Shader Branching
**Severity:** Low
**Description:** The terrain fragment shader switches on terrain_type (10 branches) for glow behavior. GPU shader branching can be expensive if fragments within a warp/wavefront take different branches.
**Mitigation:** Terrain types are spatially coherent -- most fragments in a draw call share the same type. The branch divergence should be low. If profiling shows issues, split terrain rendering by type (one draw call per type per chunk), but this is unlikely to be needed.

### Risk 5: Chunk Seams at LOD Boundaries
**Severity:** Medium
**Description:** Adjacent terrain chunks at different LOD levels may have mismatched edge vertices, causing visible seams (gaps or T-junctions).
**Mitigation:** Use "skirt" geometry -- extend each chunk's edge vertices downward by 0.5 units to hide gaps. Alternatively, use LOD transition strips (stitching rows between LOD levels). Skirt geometry is simpler and sufficient for toon rendering where small gaps would be hidden by the dark background.

### Risk 6: Edge Detection on Terrain Type Boundaries
**Severity:** Low
**Description:** The normal-based edge detection (ticket 2-006) may not produce outlines at terrain type boundaries where the surface is geometrically continuous but visually different (e.g., flat_ground adjacent to spore_plains at the same elevation). Only color differs, not normals or depth.
**Mitigation:** Accept this -- toon shading with different base colors already provides visual separation. If outlines at type boundaries are desired, add a per-fragment "material ID" to the G-buffer and detect edges on material ID changes. This is a minor enhancement, not a blocker.

### Risk 7: 512x512 Map GPU Memory on Minimum Spec
**Severity:** Low
**Description:** A 512x512 map uses ~66MB for terrain alone (geometry + instances). On the minimum spec (512MB total), this is 13% of budget. With buildings, textures, and render targets, total could approach limits.
**Mitigation:** Terrain memory is well-bounded and predictable. 66MB is acceptable. The bigger concern is instance buffers for fully built 512x512 maps (buildings), which is an Epic 4+ problem.
