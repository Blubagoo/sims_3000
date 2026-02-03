# Revision Report: Epic 2 — 2026-01-28

## Trigger
**Document:** Canon update v0.4.0 → v0.5.0 (includes v0.4.1)
**Type:** canon-update
**Summary:** Free camera with isometric presets (v0.4.1), bioluminescent art direction, configurable map sizes, expanded terrain types, building template system, audio direction, dual UI modes (v0.5.0)

## Impact Summary

| Agent | Impact Level | Key Finding |
|-------|-------------|-------------|
| systems-architect | HIGH | Camera model change from fixed isometric to free camera invalidates core CameraSystem design; bioluminescent art direction elevates emissive/bloom to core; configurable map sizes require spatial partitioning and LOD |
| graphics-engineer | HIGH | Projection type invalidated (orthographic → perspective); toon shader needs emissive integration; bloom elevated from optional to mandatory; 512x512 maps stress draw call budget; shadow mapping needs dynamic frustum |
| game-designer | HIGH | REQ-10 ("No Rotation Is a Feature") invalidated; 25 experience requirements modified for free camera and bioluminescent palette; 10 new concerns including orbit feel, tilt limits, dark environment readability |

## Ticket Changes

### Modified Tickets (25)

| Ticket | What Changed | Why |
|--------|-------------|-----|
| 2-005 | Added emissive term, world-space light, ambient term, bioluminescent palette tuning | Canon v0.5.0 art direction requires bioluminescent toon shader |
| 2-006 | Added perspective projection support, normal-based edges as primary, depth linearization | Free camera uses perspective; depth non-linear in perspective |
| 2-007 | Added perspective validation, MRT consideration for emissive channel | Perspective projection + bloom may need separate emissive target |
| 2-008 | Added bloom/emissive/ambient params, per-terrain emissive color presets | Bloom and emissive are now core configurable features |
| 2-009 | Added emissive texture/factor extraction from glTF material | Emissive materials are core; models carry emission data |
| 2-010 | Added emissive texture reference and color/factor to Material struct | GPU-side materials must store emissive data |
| 2-012 | Added per-instance emissive intensity/color, 512x512 budget note, chunked instancing | Powered/unpowered glow state is per-instance; large maps stress instancing |
| 2-017 | MAJOR: Free camera state, mode enum, variable pitch (15°-80°), yaw (0°-360°), 6 preset definitions, transition state | Canon v0.4.1 changed camera from fixed isometric to free camera with presets |
| 2-018 | Changed clear color to dark base ({0.02, 0.02, 0.05}), bloom as required pipeline stage | Dark bioluminescent environment; bloom is mandatory |
| 2-019 | Formalized bloom as mandatory pass, documented pass order | Bloom required for art direction, not deferrable |
| 2-020 | Variable pitch/yaw from camera state, tested at all angles, gimbal lock avoidance | Free camera means view matrix inputs are dynamic |
| 2-021 | MAJOR: Orthographic → Perspective (~35° FOV), all criteria rewritten | Free camera requires perspective; decision record created |
| 2-023 | Camera-relative pan direction, right mouse drag, edge scrolling | Free camera rotation means pan must follow view direction |
| 2-024 | Perspective zoom (distance-based), map-size-aware range | Perspective zoom differs from orthographic; 512x512 needs wider range |
| 2-025 | Frustum trapezoid footprint, configurable map size (128/256/512) | Perspective frustum is trapezoidal; map size is variable |
| 2-026 | Elevated to requirement, mandatory spatial partitioning, all camera angles | Canon requires culling + LOD for 512x512 maps |
| 2-027 | Added preset snap transitions (0.3-0.5s ease-in-out), interpolate all camera params | Mode switching requires smooth animated angle transitions |
| 2-029 | Perspective ray casting (divergent rays), numerical stability for near-horizontal pitch | Perspective rays diverge from camera; stability at extreme angles |
| 2-030 | All camera angles including extreme tilt, numerical stability guard | Free camera means pick ray at any angle, including near-parallel to ground |
| 2-031 | Added emissive_intensity (float) and emissive_color (Vec3) per instance | Per-entity bioluminescent glow state (powered/unpowered) |
| 2-032 | Quaternion required for rotation (removed Euler alternative) | Free camera views models from all angles; quaternion avoids gimbal lock |
| 2-037 | MAJOR: Expanded from energy conduit glow to full bioluminescent system (10 terrain types, multi-color palette, per-instance control, glow hierarchy, 0.5s transitions) | Canon v0.5.0 makes bioluminescence the core visual identity |
| 2-038 | Elevated from optional to mandatory, cannot be fully disabled, quality tiers, conservative threshold for dark environment | Bloom required for bioluminescent glow bleed per canon |
| 2-039 | Shadow frustum adapts to free camera, texel snapping for stability, world-space fixed light, dark base tuning | Free camera means shadow map frustum changes every frame |
| 2-040 | Grid readable at all camera angles, fading at extreme tilt, configurable map sizes | Free camera means grid viewed from arbitrary angles |

### New Tickets (5)

| Ticket | Description | Why |
|--------|-------------|-----|
| 2-046 | Free Camera Orbit and Tilt Controls | Canon v0.4.1 adds free camera with orbit/tilt; implements CameraSystem "free orbit/pan/zoom/tilt" |
| 2-047 | Isometric Preset Snap System | Canon v0.4.1 specifies isometric preset snap views (N/E/S/W) with smooth transitions |
| 2-048 | Camera Mode Management State Machine | Free camera requires mode management (Free/Preset/Animating) for clean transitions |
| 2-049 | LOD System (Basic Framework) | Canon v0.5.0 requires LOD for 512x512 maps ("Larger maps require distance-based LOD") |
| 2-050 | Spatial Partitioning for Large Maps | Canon v0.5.0 requires frustum culling infrastructure for 512x512 maps |

### Removed Tickets

None.

### Unchanged Tickets (20)

2-001, 2-002, 2-003, 2-004, 2-011, 2-013, 2-014, 2-015, 2-016, 2-022, 2-028, 2-033, 2-034, 2-035, 2-036, 2-041, 2-042, 2-043, 2-044, 2-045

## Discussion Summary

Ten cross-agent questions were raised and resolved:

1. **Projection Type (ALL agents):** "Orthographic, perspective, or hybrid for free camera?"
   - Resolution: Perspective always with ~35° FOV. Decision record created at plans/decisions/epic-2-projection-type.md.

2. **Camera Input Bindings (SA → GD):** "Which inputs for orbit, pan, preset snap?"
   - Resolution: Middle mouse = orbit, right mouse = pan, Q/E = 90° preset snap, WASD/edge scroll = pan, scroll wheel = zoom.

3. **Glow State Transitions (SA → GD):** "Instant, fade, or flicker for power loss?"
   - Resolution: Fade over ~0.5s. Emissive intensity is an interpolated float.

4. **LOD Scope (GE → SA):** "LOD in Epic 2 or defer?"
   - Resolution: Basic LOD framework in Epic 2 (ticket 2-049). Full LOD content in later epics. 512x512 is Epic 2 deliverable.

5. **Camera Pitch Limits (GE → GD):** "What pitch range?"
   - Resolution: 15°–80°. Default preset ~35.264°.

6. **Camera Mode Transitions (GE → SA, GD → SA):** "Smooth or snap?"
   - Resolution: Smooth 0.3–0.5s ease-in-out for preset transitions. Preset-to-free = instant unlock on orbit input.

7. **Camera Speed + Map Size (GD → SA):** "Auto or manual?"
   - Resolution: Auto-scales with map diagonal + zoom level.

8. **Minimum Ambient Light (GD → GE):** "How to keep dark world readable?"
   - Resolution: Ambient shader term (~0.05–0.1) + terrain emissive materials for baseline readability.

9. **Bloom Threshold (GD → GE):** "Dynamic or fixed?"
   - Resolution: Fixed conservative threshold. Quality tiers via resolution (1/4, 1/8).

10. **Terrain Emissive Variants (GD → GE):** "Separate materials or parameterized?"
    - Resolution: Parameterized — one material with per-instance emissive color/intensity from TerrainVisualConfig.

No formal discussion document created — all questions immediately resolvable by agent convergence.

## Canon Verification

- Tickets verified: 30 (25 MODIFIED + 5 NEW)
- Conflicts found: 0
- Resolutions: N/A — all modifications and additions comply with canon v0.5.0
- Previous issues (v0.3.0 verification): 2 conflicts + 5 minor issues from 2D plan are moot (epic replanned for 3D)

## Decisions Made

| Decision | File | Summary |
|----------|------|---------|
| Projection Type | plans/decisions/epic-2-projection-type.md | Perspective with ~35° FOV (replaces orthographic) |

## Cascading Impact

The Epic 2 revision has potential cascading impact on other epics:

| Epic | Impact | Reason |
|------|--------|--------|
| Epic 3 (Terrain) | MEDIUM | 10 terrain types with distinct glow properties require per-terrain emissive config; terrain rendering uses Epic 2's instancing with per-instance emissive data |
| Epic 4 (Buildings) | MEDIUM | Building models must be 360°-ready (no "ugly back"); powered/unpowered glow state uses Epic 2's per-instance emissive; building templates must work with free camera |
| Epic 5 (Beings/Units) | LOW | Being models viewed from all angles; uses same transform interpolation and rendering pipeline |
| Epic 12 (UI) | LOW-MEDIUM | Dual UI modes (Classic + Sci-fi Holographic) may need SDL_GPU rendering for holographic effects; dark base environment affects UI readability |
| Epic 15 (Audio) | NONE | Audio is independent of rendering changes |

Note: These impacts originate from the canon v0.5.0 changes themselves, not from Epic 2's revision. They should be addressed when those epics are planned or revised.

## Scope Change Summary

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Total tickets | 45 | 50 | +5 |
| Camera tickets (Group E) | 11 | 14 | +3 |
| New group (L: Performance) | — | 2 | +2 |
| Modified tickets | — | 25 | — |
| Unchanged tickets | — | 20 | — |
| Decision records | 0 | 1 | +1 |
