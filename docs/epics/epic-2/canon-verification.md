# Canon Verification: Epic 2

**Verification Date:** 2026-01-27
**Canon Version:** 0.3.0
**Verified By:** Systems Architect Agent

---

## Summary

- **Total tickets:** 26
- **Canon-compliant:** 19
- **Minor issues:** 5
- **Conflicts found:** 2

---

## Verification Details

### Compliant Tickets

The following tickets fully comply with canon:

| Ticket | Title | Verification Notes |
|--------|-------|-------------------|
| 2-001 | SDL3 Renderer Initialization | Correctly uses SDL3 per tech stack; shares renderer with UISystem per systems.yaml |
| 2-003 | RenderComponent and SpriteComponent Definitions | Follows ECS component patterns; components are pure data, snake_case fields |
| 2-004 | Core Sprite Rendering | Correctly queries ECS; uses AssetManager from Epic 0 |
| 2-005 | Render Layer System | Layer separation matches systems.yaml boundaries |
| 2-007 | CameraComponent and Camera State | Correctly identifies camera as client-authoritative per multiplayer patterns |
| 2-008 | Sprite Batching | Performance optimization consistent with architecture |
| 2-009 | Camera Pan Controls | Client-side updates per-frame correctly; respects multiplayer architecture |
| 2-010 | Camera Zoom Controls | Range 0.25x-2.0x aligns with game design; client-authoritative |
| 2-011 | Camera Viewport Bounds | Correctly bounds to map; default 128x128 matches expected map sizes |
| 2-012 | Frustum Culling | Correctly references IGridQueryable from interfaces.yaml |
| 2-013 | Tile Picking | Correctly handles view mode awareness for underground entities |
| 2-014 | Render State Interpolation | 20Hz tick rate matches canon (50ms = 20 ticks/sec from interfaces.yaml) |
| 2-015 | Camera Interpolation | Client-only animation correctly identified |
| 2-017 | Texture Atlas Support | Integrates with Epic 0 AssetManager per file_organization pattern |
| 2-018 | AnimationSystem Integration | Correctly creates separate system; follows "one thing well" principle |
| 2-019 | View Mode System | Layer filtering approach consistent with RenderingSystem ownership |
| 2-020 | Multiplayer Cursor Rendering | Correctly handles network message integration with Epic 1 |
| 2-021 | Selection Highlight Rendering | Uses faction colors per multiplayer.ownership patterns |
| 2-024 | Parallax Background Layers | Non-ECS background assets match canon guidance |

### Minor Issues

Issues that require small adjustments but don't violate core canon principles:

---

#### Minor Issue 1: Ticket 2-016 - Terminology

**Ticket:** 2-016 Debug Rendering Framework

**Issue:** Uses "zone colors" terminology. Canon terminology.yaml specifies zones should use alien terms:
- residential -> habitation
- commercial -> exchange
- industrial -> fabrication

**Canon Reference:** terminology.yaml, zones section (lines 51-68)

**Current Text:**
> "Zone coloring: tints zones by type (H/E/F) when enabled"

**Suggested Fix:** The abbreviated form "(H/E/F)" is actually canon-compliant since it uses:
- H = Habitation (not R = Residential)
- E = Exchange (not C = Commercial)
- F = Fabrication (not I = Industrial)

However, the full phrase should read:
> "Zone coloring: tints zones by type (Habitation/Exchange/Fabrication)"

**Severity:** Low - Abbreviations are correct; just ensure full names match canon when displayed.

---

#### Minor Issue 2: Ticket 2-022 - Time Unit Terminology

**Ticket:** 2-022 Day/Night Cycle Rendering

**Issue:** Uses "minutes" as time unit. Canon terminology.yaml defines simulation time units:
- year -> cycle
- month -> phase
- day -> rotation

**Canon Reference:** terminology.yaml, simulation section (lines 181-184)

**Current Text:**
> "Cycle duration: 20-30 minutes (configurable in settings)"

**Suggested Fix:** This is acceptable as-is because "minutes" refers to real-world time, not in-game time. The distinction should be made clearer:
> "Cycle duration: 20-30 real-world minutes (configurable in settings)"

**Severity:** Low - Just clarify real-world vs. in-game time.

---

#### Minor Issue 3: Ticket 2-006 - Depth Formula Inconsistency

**Ticket:** 2-006 Z-Ordering and Depth Sorting

**Issue:** Acceptance criteria specifies depth formula as:
> `depth = grid_x + grid_y + elevation * weight`

But the Systems Architect note says:
> "Depth = gx + gy; elevation adds small offset (0.001 * elevation)"

These formulas are slightly inconsistent. The 0.001 multiplier seems too small for 32 elevation levels.

**Canon Reference:** patterns.yaml grid section (lines 412-446) specifies:
- 32 elevation levels (0-31)
- 8 pixels per elevation level

**Suggested Fix:** Clarify the formula. With elevation_step of 8 pixels, a reasonable weight might be:
> `depth = grid_x + grid_y + elevation * 0.1`

This ensures elevation 31 adds 3.1 to depth, which properly orders buildings at different heights without overwhelming the grid position.

**Severity:** Medium - Formula needs to be specified precisely for correct rendering.

---

#### Minor Issue 4: Ticket 2-023 - Terminology

**Ticket:** 2-023 Construction Visual ("Materializing" Effect)

**Issue:** Uses term "structures" which should be "structures" per buildings terminology.

**Canon Reference:** terminology.yaml, buildings section (line 210):
> building: structure

**Current Text:**
> "Effect works for all building sizes"

**Suggested Fix:** Ticket correctly uses "structures" in the title. The acceptance criterion should consistently use "structure":
> "Effect works for all structure sizes"

**Severity:** Low - Minor terminology consistency.

---

#### Minor Issue 5: Ticket 2-025 - Entity Count Clarification

**Ticket:** 2-025 Render Performance Testing and Optimization

**Issue:** Agent notes mention 25,000-35,000 total entities but acceptance criteria says 50,000.

**Current Text:**
> "Test with 50,000 total entities, 2000 visible: stable performance"

vs. Agent Notes:
> "Q033/Q034 budgets: 100-500 draw calls; 25,000-35,000 total entities target"

**Suggested Fix:** Align numbers. 50,000 is a stretch goal; primary target should be 25,000-35,000 per agent notes.

**Severity:** Low - Clarify which is primary vs. stretch goal.

---

### Conflicts

Issues that directly conflict with canon and require resolution.

---

#### Conflict 1: Ticket 2-002 - Tile Dimensions

**Issue:** Ticket specifies incorrect tile dimensions.

**Ticket Text (2-002):**
> "Projection constants defined: TILE_WIDTH=64, TILE_HEIGHT=32 (screen diamond)"

**Canon Reference:** patterns.yaml, grid.tile_sizes (lines 432-437):
```yaml
tile_sizes:
  base_width: 64        # pixels
  base_height: 64       # pixels
  shape: square         # Square tiles (not diamond)
  elevation_step: 8     # pixels per elevation level
  elevation_levels: 32  # Total elevation levels (0-31)
```

**Analysis:**
Canon explicitly states tiles are **64x64 square tiles**, not 64x32 diamond tiles. The ticket's 2:1 dimetric projection is a rendering transformation of square tiles into diamond screen space, but the source tiles themselves are 64x64 squares.

Additionally, the ticket overview states:
> "64x32 pixel diamond tiles (2:1 dimetric projection)"

This conflicts with canon's explicit "shape: square" specification.

**Impact:**
- Art pipeline would produce wrong aspect ratio
- Coordinate math would be incorrect
- All subsequent rendering would be misaligned

**Resolution Options:**

**A) Modify ticket to comply with canon:**
Correct the terminology:
- Source tiles: 64x64 pixels (square)
- Screen projection: 2:1 dimetric creates 64x32 diamond appearance
- TILE_WIDTH constant remains 64
- TILE_HEIGHT constant should remain 64 (source), with TILE_SCREEN_HEIGHT = 32 for projection

**B) Propose canon update (NOT RECOMMENDED):**
Canon explicitly chose square tiles with dimetric projection. Changing to true diamond tiles would require significant art pipeline changes.

**Recommendation:** Option A - Update ticket to clarify:
- World tiles are 64x64 squares
- Screen projection renders them as 64x32 diamonds
- Rename constants: TILE_SIZE=64, TILE_SCREEN_HEIGHT=32

---

#### Conflict 2: Epic 2 Overview - Tile Dimensions (Same Issue)

**Issue:** The tickets.md overview section has the same error.

**Ticket Text (Overview):**
> "the canonical 64x32 pixel diamond tiles"

**Canon Reference:** patterns.yaml specifies 64x64 square tiles.

**Resolution:** Same as Conflict 1 - update to clarify the distinction between source tiles (64x64 square) and their dimetric screen projection (64x32 diamond).

---

## System Boundary Verification

Per systems.yaml, Epic 2 defines:

| System | Canon Ownership | Ticket Compliance |
|--------|-----------------|-------------------|
| RenderingSystem | SDL3 renderer, sprites, projection, Z-ordering, layers | COMPLIANT - Tickets 2-001 through 2-006, 2-008, etc. correctly scope to these responsibilities |
| RenderingSystem | Does NOT own: Camera position, UI rendering | COMPLIANT - Camera delegated to CameraSystem; UI reserved for Epic 12 |
| CameraSystem | Camera position, zoom, pan, viewport bounds | COMPLIANT - Tickets 2-007 through 2-011, 2-015 correctly scope |
| CameraSystem | Does NOT own: Rendering, input handling | COMPLIANT - Defers to RenderingSystem and InputSystem |

All tickets respect the canon system boundaries from systems.yaml (lines 108-146).

---

## Interface Verification

Per interfaces.yaml, Epic 2 should implement/use:

| Interface | Verification |
|-----------|--------------|
| IGridQueryable | Ticket 2-012 correctly references for spatial queries |
| ISerializable | Not directly relevant to rendering (network layer) |
| Coordinate data contracts (GridPosition, GridRect) | Ticket 2-002 aligns with int32_t grid positions |

Rendering systems correctly do NOT implement simulation interfaces (ISimulatable) as they are client-side.

---

## Multiplayer Verification

| Requirement | Ticket | Status |
|-------------|--------|--------|
| Camera is client-authoritative | 2-007 | COMPLIANT |
| Simulation interpolation from server ticks | 2-014 | COMPLIANT (20Hz tick rate correct) |
| Other player cursors visible | 2-020 | COMPLIANT |
| Day/night synchronized from server | 2-022 | COMPLIANT |
| No client-side simulation | All | COMPLIANT - All rendering is display-only |

---

## Terminology Verification (Spot Check)

| Term Used | Canon Term | Status |
|-----------|------------|--------|
| Habitation (H) | habitation | COMPLIANT |
| Exchange (E) | exchange | COMPLIANT |
| Fabrication (F) | fabrication | COMPLIANT |
| Beings | beings | COMPLIANT (2-018 agent notes) |
| Energy conduits | energy_conduit | COMPLIANT (2-022) |
| Faction color | faction_color | COMPLIANT (2-020, 2-021) |

---

## Recommendations

### Immediate Actions (Before Implementation)

1. **Fix tile dimension conflict in Overview and Ticket 2-002:**
   - Change "64x32 pixel diamond tiles" to "64x64 square tiles with 2:1 dimetric projection (64x32 screen diamonds)"
   - Update projection constants documentation to clarify source vs. screen coordinates

2. **Clarify depth formula in Ticket 2-006:**
   - Specify exact formula with elevation weight
   - Ensure 32 elevation levels are properly ordered

### Optional Improvements

3. **Standardize time terminology in 2-022:**
   - Distinguish "real-world minutes" from "in-game rotations"

4. **Align performance targets in 2-025:**
   - Primary: 25,000-35,000 entities
   - Stretch: 50,000 entities

5. **Consistent terminology sweep:**
   - Ensure all UI-visible text uses canon alien terms
   - "Zone" abbreviations are fine; full names must be Habitation/Exchange/Fabrication

### Architecture Notes

The Epic 2 tickets correctly:
- Separate RenderingSystem and CameraSystem responsibilities
- Keep camera client-authoritative while simulation is server-authoritative
- Use interpolation to bridge 20Hz server ticks to 60fps client rendering
- Integrate with Epic 0 (AssetManager, InputSystem) and Epic 1 (NetworkManager) appropriately

No systemic architectural issues found.

---

## Verification Checklist

| Category | Status |
|----------|--------|
| System boundaries respected | PASS |
| Required interfaces implemented | PASS |
| ECS patterns followed | PASS |
| Terminology correct | PASS (minor fixes noted) |
| Multiplayer accounted for | PASS |
| **Rendering specs correct** | **FAIL** - Tile dimensions conflict |
| Dependencies properly declared | PASS |
| No scope creep into other systems | PASS |

---

## Conclusion

Epic 2 tickets are **mostly canon-compliant** with strong adherence to system boundaries, ECS patterns, and multiplayer architecture.

**Critical fix required:** The tile dimension specification (64x32 diamond vs. 64x64 square) must be corrected before implementation to align with canon. This is a documentation issue, not an architectural flaw - the intended behavior (dimetric projection) is correct, but the terminology conflates source tile dimensions with their screen projection.

After addressing the tile dimension conflict, Epic 2 is ready for implementation.

---

## Revision Verification (2026-01-28)

### Trigger
Canon update v0.4.0 → v0.5.0 (includes v0.4.1 free camera change)

### Note on Previous Verification
The original verification above was performed against canon v0.3.0 for the 2D version of Epic 2 (26 tickets). Epic 2 was subsequently replanned for 3D (v0.4.0, 45 tickets) and has now been revised for v0.5.0 (50 tickets). The conflicts and issues from the original 2D verification (tile dimensions, sprite-based concerns) are moot — the epic is now fully 3D-based.

### Verified Tickets (Modified)

| Ticket | Status | Canon Compliant | Notes |
|--------|--------|-----------------|-------|
| 2-005 | MODIFIED | Yes | Emissive term, bioluminescent palette, world-space light align with patterns.yaml art_direction section; RenderingSystem owns shader pipeline per systems.yaml |
| 2-006 | MODIFIED | Yes | Perspective projection support aligns with free camera (systems.yaml CameraSystem); edge detection remains within RenderingSystem boundary |
| 2-007 | MODIFIED | Yes | Pipeline validation for perspective is infrastructure; MRT consideration for emissive aligns with art_direction.shading |
| 2-008 | MODIFIED | Yes | Expanded config params (bloom, emissive, ambient) align with patterns.yaml art_direction; RenderingSystem owns shader config |
| 2-009 | MODIFIED | Yes | Emissive extraction from glTF aligns with patterns.yaml art_pipeline.model_format (.glb); RenderingSystem owns model rendering |
| 2-010 | MODIFIED | Yes | Emissive material data aligns with art_direction; GPUMesh within RenderingSystem boundary |
| 2-012 | MODIFIED | Yes | Per-instance emissive aligns with art_direction.shading; 512x512 budget note aligns with map_configuration |
| 2-017 | MODIFIED | Yes | Free camera mode enum, variable pitch/yaw, presets align with systems.yaml CameraSystem ("free orbit/pan/zoom/tilt, isometric preset snap views") |
| 2-018 | MODIFIED | Yes | Dark clear color aligns with art_direction.palette.base_tones; bloom in pipeline aligns with art_direction.shading |
| 2-019 | MODIFIED | Yes | Mandatory bloom aligns with patterns.yaml art_direction.shading ("bloom post-processing for glow bleed") |
| 2-020 | MODIFIED | Yes | Variable orientation aligns with CameraSystem "free orbit/pan/zoom/tilt" per systems.yaml |
| 2-021 | MODIFIED | Yes | Perspective projection per accepted decision (epic-2-projection-type.md); CameraSystem owns projection matrices |
| 2-023 | MODIFIED | Yes | Camera-relative pan and orbit input align with CameraSystem "free orbit/pan/zoom/tilt" per systems.yaml |
| 2-024 | MODIFIED | Yes | Map-size-aware zoom aligns with map_configuration (128/256/512) |
| 2-025 | MODIFIED | Yes | Configurable map size bounds align with patterns.yaml map_configuration; CameraSystem owns viewport bounds |
| 2-026 | MODIFIED | Yes | Mandatory culling aligns with patterns.yaml map_configuration ("Larger maps require distance-based LOD and frustum culling") |
| 2-027 | MODIFIED | Yes | Preset snap transitions support "isometric preset snap views" per systems.yaml CameraSystem |
| 2-029 | MODIFIED | Yes | Perspective ray casting aligns with projection type decision; coordinate conversion within CameraSystem boundary |
| 2-030 | MODIFIED | Yes | All-angle picking required for free camera; within CameraSystem boundary |
| 2-031 | MODIFIED | Yes | Emissive fields align with art_direction; RenderComponent within RenderingSystem boundary |
| 2-032 | MODIFIED | Yes | Quaternion rotation for free camera viewing; TransformComponent correctly separated from PositionComponent per patterns.yaml |
| 2-037 | MODIFIED | Yes | Expanded emissive scope aligns with art_direction.shading ("emissive materials for bioluminescent glow"); 10 terrain types per terrain_types |
| 2-038 | MODIFIED | Yes | Mandatory bloom aligns with art_direction.shading ("bloom post-processing for glow bleed"); RenderingSystem owns post-processing |
| 2-039 | MODIFIED | Yes | Free camera shadow adaptation required; RenderingSystem owns shadow rendering |
| 2-040 | MODIFIED | Yes | All-angle grid required for free camera; debug tools within RenderingSystem boundary |

### Verified Tickets (New)

| Ticket | Status | Canon Compliant | Notes |
|--------|--------|-----------------|-------|
| 2-046 | NEW | Yes | Orbit/tilt controls implement CameraSystem "free orbit/pan/zoom/tilt" per systems.yaml |
| 2-047 | NEW | Yes | Preset snap implements CameraSystem "isometric preset snap views" per systems.yaml |
| 2-048 | NEW | Yes | Mode management required to support both camera modes; CameraSystem boundary respected |
| 2-049 | NEW | Yes | LOD framework implements patterns.yaml map_configuration requirement ("Larger maps require distance-based LOD") |
| 2-050 | NEW | Yes | Spatial partitioning implements patterns.yaml map_configuration requirement ("frustum culling") for 512x512 maps |

### New Conflicts

None. All 25 modified tickets and 5 new tickets comply with canon v0.5.0.

### Updated System Boundary Verification

| Canon Responsibility | Original Coverage | Revision Impact |
|---------------------|-------------------|-----------------|
| RenderingSystem: SDL_GPU/3D renderer | 2-001 through 2-004 | Unchanged |
| RenderingSystem: toon/cell shader pipeline | 2-005, 2-006, 2-007, 2-008 | 2-005/2-006/2-007/2-008 expanded with emissive, perspective, bloom — still within RenderingSystem boundary |
| RenderingSystem: 3D model rendering | 2-009 through 2-013 | 2-009/2-010/2-012 expanded with emissive data — still within boundary |
| RenderingSystem: depth buffer, render layers | 2-014 through 2-019, 2-034 through 2-036 | 2-018/2-019 expanded with bloom — still within boundary |
| CameraSystem: 3D camera position | 2-017, 2-020 through 2-027 | 2-017 through 2-027 significantly expanded for free camera — still within CameraSystem boundary |
| CameraSystem: free orbit/pan/zoom/tilt | NEW (v0.5.0) | 2-046, 2-047, 2-048 implement this new canon responsibility |
| CameraSystem: isometric preset snap views | NEW (v0.5.0) | 2-047, 2-048 implement this new canon responsibility |
| CameraSystem: view/projection matrices | 2-020, 2-021, 2-022 | 2-021 changed to perspective — still within CameraSystem boundary |

### Previous Issues Status

The 2 conflicts and 5 minor issues from the original v0.3.0 verification are **moot** — they referenced the 2D sprite-based version of Epic 2 (tile dimensions, dimetric projection, sprite components). Epic 2 was completely replanned for 3D rendering. These issues no longer apply.

### Decision Records

| Decision | Date | Status | Referenced By |
|----------|------|--------|--------------|
| epic-2-projection-type.md | 2026-01-28 | Accepted | 2-021, 2-024, 2-029, 2-030 |
