# POC Asset Requirements: 3D Models for Rendering Validation

**Created:** 2026-02-02
**Purpose:** Define asset requirements for POC-1 (SDL_GPU Toon Shading) testing
**Source:** Unreal Engine Marketplace (free assets)
**Format:** Export to glTF 2.0 (.glb)

---

## Overview

POC-1 validates our toon shading pipeline with these key tests:
- **1000+ instanced buildings** at 60 FPS
- **Outline rendering** with clean silhouettes
- **Draw call batching** (target: 10 or fewer draw calls)
- **Visual variety** through model/color variation

We need enough model variety to test instancing with different meshes while keeping asset acquisition practical. Free Unreal Marketplace assets will be exported to glTF 2.0 for testing.

---

## 1. Building Models Needed for POC-1

### 1.1 Habitation Structures (Residential Zone)

Where beings dwell. Three density tiers with distinct visual profiles.

#### Low-Density Habitation (Single Dwellings)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Low-density habitation / dwelling |
| Footprint | 1x1 to 2x2 tiles |
| Height | 0.5 - 1.5 units (single story equivalent) |
| Polygon Target | 200-500 triangles |
| Variants Needed | 2-3 models |
| Key Features | Simple silhouette, flat or domed roof, single entrance |
| Look For | Small houses, pods, capsules, sci-fi huts |

#### Medium-Density Habitation (Hab Blocks)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Medium-density habitation / hab unit |
| Footprint | 2x2 to 3x3 tiles |
| Height | 2-4 units (3-6 story equivalent) |
| Polygon Target | 500-1000 triangles |
| Variants Needed | 2-3 models |
| Key Features | Stacked appearance, multiple windows/openings, balconies |
| Look For | Apartment blocks, modular buildings, sci-fi complexes |

#### High-Density Habitation (Towers)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | High-density habitation / tower |
| Footprint | 2x2 to 4x4 tiles |
| Height | 6-12 units (15-30 story equivalent) |
| Polygon Target | 800-1500 triangles |
| Variants Needed | 2-3 models |
| Key Features | Vertical emphasis, distinctive top silhouette, tiered sections |
| Look For | Skyscrapers, high-rise towers, futuristic spires |

---

### 1.2 Exchange Structures (Commercial Zone)

Where beings trade and conduct business.

#### Low-Density Exchange (Small Shops)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Low-density exchange / work center |
| Footprint | 1x1 to 2x2 tiles |
| Height | 1-2 units |
| Polygon Target | 300-600 triangles |
| Variants Needed | 2 models |
| Key Features | Storefront appearance, signage area, awning/canopy |
| Look For | Small shops, kiosks, market stalls (sci-fi style) |

#### High-Density Exchange (Trade Centers)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | High-density exchange / trade hub |
| Footprint | 3x3 to 4x4 tiles |
| Height | 4-8 units |
| Polygon Target | 800-1500 triangles |
| Variants Needed | 2 models |
| Key Features | Wide base, glassy appearance, distinctive roof structure |
| Look For | Office buildings, commercial centers, corporate towers |

---

### 1.3 Fabrication Structures (Industrial Zone)

Where beings produce goods and process materials.

#### Low-Density Fabrication (Small Fabricators)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Low-density fabrication / fabricator |
| Footprint | 2x2 to 3x3 tiles |
| Height | 1-2 units |
| Polygon Target | 400-800 triangles |
| Variants Needed | 2 models |
| Key Features | Flat roof, smoke stacks/vents, loading bays |
| Look For | Warehouses, small factories, processing plants |

#### High-Density Fabrication (Heavy Fabricators)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | High-density fabrication / heavy fabricator |
| Footprint | 4x4 to 6x6 tiles |
| Height | 2-4 units |
| Polygon Target | 800-1500 triangles |
| Variants Needed | 1-2 models |
| Key Features | Complex silhouette, multiple stacks, industrial machinery |
| Look For | Factories, refineries, processing facilities |

---

### 1.4 Energy Nexus (Power Plants)

Generates energy for the colony.

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Energy nexus |
| Footprint | 4x4 to 6x6 tiles |
| Height | 2-5 units |
| Polygon Target | 1000-2000 triangles |
| Variants Needed | 1-2 models |
| Key Features | Prominent energy source visual (reactor dome, solar arrays, turbines), cooling towers |
| Look For | Power plants, reactors, generators (sci-fi style) |

---

### 1.5 Service Buildings

Essential colony services.

#### Enforcer Post (Police Station)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Enforcer post |
| Footprint | 2x2 to 3x3 tiles |
| Height | 1-2 units |
| Polygon Target | 500-800 triangles |
| Variants Needed | 1 model |
| Key Features | Authoritative appearance, communication array, vehicle bay |
| Look For | Security buildings, police stations, military outposts |

#### Hazard Response Post (Fire Station)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Hazard response post |
| Footprint | 2x2 to 3x3 tiles |
| Height | 1-2 units |
| Polygon Target | 500-800 triangles |
| Variants Needed | 1 model |
| Key Features | Large bay doors, equipment visible, tower element |
| Look For | Fire stations, emergency response buildings |

#### Medical Nexus (Hospital)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Medical nexus |
| Footprint | 3x3 to 4x4 tiles |
| Height | 3-6 units |
| Polygon Target | 800-1200 triangles |
| Variants Needed | 1 model |
| Key Features | Clinical appearance, cross/medical symbol area, multiple wings |
| Look For | Hospitals, medical centers, clinics |

---

### 1.6 Infrastructure Elements

Pathways and utility conduits.

#### Pathway Tiles (Roads)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Pathway |
| Footprint | 1x1 tile |
| Height | 0.05 units (nearly flat) |
| Polygon Target | 10-50 triangles |
| Variants Needed | Straight, curve, T-junction, crossroads |
| Key Features | Defined edges, lane markings area, modular connection points |
| Look For | Road tiles, path segments, modular road kits |

#### Energy Conduits (Power Lines)

| Attribute | Specification |
|-----------|---------------|
| ZergCity Term | Energy conduit |
| Footprint | 1x1 tile (per pylon) |
| Height | 1-2 units |
| Polygon Target | 50-150 triangles |
| Variants Needed | Pylon + line segment |
| Key Features | Vertical pylon, horizontal cable/beam |
| Look For | Power pylons, transmission towers, energy pylons |

---

## 2. Unreal Marketplace Search Terms

### 2.1 Recommended Search Queries

**Primary Style Keywords:**
- `low poly buildings`
- `stylized buildings`
- `sci-fi buildings low poly`
- `cartoon buildings`
- `modular buildings stylized`
- `futuristic city low poly`
- `alien buildings`

**Specific Building Types:**
- `low poly houses`
- `stylized skyscrapers`
- `sci-fi industrial`
- `modular factory`
- `stylized power plant`

**Infrastructure:**
- `low poly roads`
- `modular road kit`
- `stylized city props`

### 2.2 Known Free Asset Packs (As of 2025)

Note: Availability changes monthly. Check current "Free for the Month" section.

| Pack Name | Contents | Suitability |
|-----------|----------|-------------|
| **City Sample Buildings** | Various city buildings | Medium (may be too realistic) |
| **Low Poly Town** | Small houses, shops | Good for low-density |
| **Stylized Modular Pack** | Modular building pieces | Excellent if available |
| **Sci-Fi Props** | Industrial elements | Good for fabrication zone |

### 2.3 What to Look For in Thumbnails

**Good Signs:**
- Clean, simple silhouettes
- Flat shaded or minimal texture detail
- Visible geometric facets (low-poly look)
- Bright, saturated colors (indicates stylized)
- Modular pieces shown
- Triangle count listed under 2000

**Warning Signs:**
- Photorealistic textures
- Complex surface detail (normal maps heavy)
- "AAA quality" or "realistic" in description
- No triangle count listed (often high-poly)
- PBR material showcase images

### 2.4 What to Avoid

| Avoid | Reason |
|-------|--------|
| Realistic/AAA packs | Wrong art style, high poly count |
| Megascans content | Photogrammetry, wrong aesthetic |
| Foliage-heavy packs | Not needed for POC-1 |
| Interior-focused packs | We only need exteriors |
| Very old packs (pre-2020) | May have compatibility issues |
| Packs without polygon counts | Risk of hidden complexity |

---

## 3. Model Specifications for Export

### 3.1 File Format

| Specification | Value |
|---------------|-------|
| Format | glTF 2.0 binary (.glb) |
| Coordinate System | Y-up, right-handed |
| Scale | 1 unit = 1 meter (default glTF) |
| Compression | Draco mesh compression optional |

### 3.2 Polygon Limits

| Building Category | Max Triangles |
|-------------------|---------------|
| Small buildings (1x1, 2x2) | 500 |
| Medium buildings (3x3, 4x4) | 1,000 |
| Large buildings (5x5+) | 2,000 |
| Infrastructure pieces | 200 |

**Total POC Budget:** All loaded models combined should fit under 100,000 triangles.

### 3.3 Texture Requirements

For toon shading POC, textures are **optional but helpful**:

| Texture Type | Needed? | Notes |
|--------------|---------|-------|
| Albedo/Diffuse | Optional | Can use solid colors instead |
| Normal Map | No | Toon shading uses hard edges |
| Roughness/Metallic | No | Not used in toon pipeline |
| Emissive | Optional | Useful for glow testing |
| AO | No | Toon shading has stylized shadows |

**If textures are included:**
- Max resolution: 512x512 (1024x1024 for large buildings)
- Format: PNG or JPEG
- Single atlas per building preferred

### 3.4 UV Mapping Needs

- **Required:** Valid UV coordinates (for potential future texturing)
- **Preferred:** Non-overlapping UVs
- **Acceptable:** Overlapping UVs if model uses solid colors

### 3.5 Origin Point / Pivot Placement

| Placement | Convention |
|-----------|------------|
| Horizontal | Center of building footprint |
| Vertical | Bottom of building (ground level) |
| Rotation | Front facing +Y (or consistent across all models) |

**Example:** A 2x2 tile building should have origin at (1.0, 0.0, 1.0) relative to its corner, sitting on the XZ plane.

### 3.6 Scale Conventions

| Reference | Size |
|-----------|------|
| 1 grid tile | 1.0 x 1.0 units |
| 1 story height | ~0.4 units |
| Small dwelling | ~0.6-1.0 units tall |
| Medium building | ~1.5-3.0 units tall |
| Tall tower | ~4.0-10.0 units tall |

**Rescaling:** Models will likely need rescaling during export. Document original scale and applied multiplier.

---

## 4. Minimum Asset Set for POC

### 4.1 Essential Models (Must Have)

These are required to validate POC-1 success criteria:

| Category | Count | Purpose |
|----------|-------|---------|
| Low-density habitation | 2 | Test basic instancing |
| Medium-density habitation | 2 | Test mid-complexity models |
| High-density habitation | 2 | Test tall building rendering |
| Low-density exchange | 1 | Zone variety |
| Low-density fabrication | 1 | Zone variety |

**Total Essential:** 8 unique models

### 4.2 Nice-to-Have Models

Additional variety if time permits:

| Category | Count | Purpose |
|----------|-------|---------|
| High-density exchange | 1-2 | More visual variety |
| High-density fabrication | 1 | Industrial zone testing |
| Energy nexus | 1 | Landmark building test |
| Service buildings | 1-2 | Special building category |
| Pathway tiles | 4 | Infrastructure rendering |
| Energy conduits | 2 | Infrastructure rendering |

**Total Nice-to-Have:** 8-12 additional models

### 4.3 Placeholder Strategy

For POC-1, **primitives are acceptable placeholders**:

| Placeholder | Use For |
|-------------|---------|
| Colored cubes | Any building without sourced model |
| Flat quads | Pathway tiles |
| Cylinders | Energy conduit pylons |
| Scaled cubes with color coding | Zone type differentiation |

**Color Coding Convention:**
- Habitation: Blue tones
- Exchange: Green tones
- Fabrication: Orange tones
- Infrastructure: Gray

### 4.4 POC Success Without Full Asset Set

POC-1 can succeed with:

| Scenario | Assessment |
|----------|------------|
| 8 unique models + primitives | **Full validation possible** |
| 4 unique models + primitives | **Adequate** - tests instancing with less variety |
| Primitives only | **Minimal** - validates pipeline, not visual quality |

The key benchmarks (frame time, draw calls, outline quality) can be measured with any geometry. More variety tests the visual appeal and instancing efficiency with mixed meshes.

---

## 5. Export Workflow Summary

### From Unreal to glTF

1. **Download** free asset pack from Marketplace
2. **Import** into Unreal project
3. **Evaluate** polygon count and style fit
4. **Export** using glTF Exporter plugin or FBX intermediate
5. **Convert** FBX to glTF using Blender if needed
6. **Verify** origin point and scale
7. **Rename** following convention: `{zone}_{density}_{variant}.glb`

### File Naming Convention

```
habitation_low_01.glb
habitation_low_02.glb
habitation_medium_01.glb
habitation_high_01.glb
exchange_low_01.glb
fabrication_low_01.glb
infrastructure_pathway_straight.glb
infrastructure_conduit_pylon.glb
```

### Validation Checklist

- [ ] Model loads in glTF viewer (e.g., https://gltf-viewer.donmccurdy.com/)
- [ ] Origin point at base center
- [ ] Scale matches grid convention (1 tile = 1 unit)
- [ ] Triangle count within limits
- [ ] No missing faces or inverted normals
- [ ] File size reasonable (<1MB per model)

---

## 6. Asset Tracking

| Model | Source Pack | Triangles | Footprint | Status |
|-------|-------------|-----------|-----------|--------|
| habitation_low_01 | | | | Pending |
| habitation_low_02 | | | | Pending |
| habitation_medium_01 | | | | Pending |
| habitation_medium_02 | | | | Pending |
| habitation_high_01 | | | | Pending |
| habitation_high_02 | | | | Pending |
| exchange_low_01 | | | | Pending |
| fabrication_low_01 | | | | Pending |

---

*This document supports POC-1 validation. Update the Asset Tracking table as models are acquired and exported.*
