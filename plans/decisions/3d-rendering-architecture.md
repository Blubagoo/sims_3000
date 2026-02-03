# Decision: 3D Rendering Architecture

**Date:** 2026-01-28
**Status:** accepted (revised)
**Affects:** Epic 0 (AssetManager), Epic 2 (Rendering), Canon files

## Context

The original plan called for 2D dimetric (isometric-style) rendering with pre-rendered sprites. After consideration, 3D rendering is more practical for modern development:

- Depth sorting handled by GPU (depth buffer)
- Zoom is trivial (camera movement, no resolution issues)
- Lighting and shadows are built-in
- Cell-shading achieved via shaders, not pre-rendered
- Asset creation simpler (one model vs multiple sprite angles)
- Full camera freedom is natural in a 3D world

## Options Considered

### 1. 2D Dimetric with Sprites (Original Plan)
- Pre-rendered 2D sprites
- Manual depth sorting
- SDL_Renderer
- Classic SimCity 2000 style

### 2. 3D with Fixed Isometric Camera
- 3D models on a grid
- GPU depth buffer handles sorting
- SDL3 + SDL_GPU (or OpenGL)
- Cell-shaded via toon shaders
- Locked camera angle, no free rotation

### 3. Full 3D with Free Camera + Isometric Presets (Selected)
- Complete camera freedom (orbit, pan, zoom, tilt)
- Isometric preset views at fixed rotations (N/S/E/W snap)
- Player chooses between free camera and preset views
- Modern city builder style (Cities Skylines, Tropico)

## Decision

**3D rendering with full camera controls and isometric preset views** using SDL3 + SDL_GPU.

Players get free camera orbit/pan/zoom/tilt by default, plus snap-to isometric preset rotations for a classic city builder feel.

## Rationale

1. **Full 3D world deserves full camera:** Locking the camera in a 3D world is an artificial limitation
2. **Isometric presets for familiarity:** Snap-to rotations (N/E/S/W) give the classic SimCity feel when wanted
3. **Player choice:** Free camera for exploration, presets for efficient building
4. **GPU handles depth sorting:** No sprite-based constraints to worry about
5. **Better zoom:** No sprite resolution limits
6. **Easier assets:** One 3D model vs multiple sprite angles
7. **Modern tooling:** 3D modeling tools are mature
8. **Cell-shading:** Shaders achieve the look dynamically

## Technical Approach

### Rendering Stack
- SDL3 for window/input/audio
- SDL_GPU for 3D rendering (or OpenGL as fallback)
- Custom toon/cell-shading shader

### Camera
- **Free camera mode:** Full orbit, pan, zoom, tilt
  - Orbit: Middle mouse drag or Q/E keys
  - Pan: WASD / edge scroll / right mouse drag
  - Zoom: Scroll wheel (continuous)
  - Tilt: Vertical angle adjustment
- **Isometric preset mode:** Snap-to fixed rotations
  - 4 cardinal snap angles (N/E/S/W at 45° increments)
  - Fixed isometric pitch (~35.264° from horizontal)
  - Quick-rotate hotkeys to snap between presets
- Players can toggle between free and preset modes
- Default: Isometric preset (familiar city builder feel)

### Grid System
- Still tile-based (grid coordinates)
- 3D models placed on grid cells
- Elevation via actual 3D positioning

### Asset Pipeline
- 3D models in glTF format (industry standard, compact)
- Textures as PNG
- Model loading library needed (e.g., cgltf, tinygltf)

## Consequences

### Canon Updates Required
- `patterns.yaml`: Update rendering section for 3D
- `systems.yaml`: Update RenderingSystem description

### Epic 0 Impact
- AssetManager needs 3D model loading support
- Add glTF loading library to dependencies

### Epic 2 Impact
- Complete re-plan for 3D rendering
- New tickets for: 3D renderer, shaders, model rendering, camera system
- Camera system needs both free orbit and isometric preset modes
- Camera input handling (mouse + keyboard controls)

### Asset Pipeline Impact
- 3D models instead of 2D sprites
- Toon shader instead of pre-rendered cell-shading

## References

- SDL_GPU documentation: https://wiki.libsdl.org/SDL3/CategoryGPU
- glTF format: https://www.khronos.org/gltf/
- Isometric preset math: Standard 35.264° pitch, 45° yaw increments
