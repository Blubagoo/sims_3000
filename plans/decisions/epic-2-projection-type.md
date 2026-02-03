# Decision: Projection Type for Free Camera

**Date:** 2026-01-28
**Status:** accepted
**Epic:** 2
**Tickets:** 2-021, 2-024, 2-029, 2-030
**Triggered By:** Canon update v0.4.0 → v0.5.0 (v0.4.1 free camera)

## Context

Epic 2 originally planned orthographic projection for a fixed isometric camera (canon v0.4.0). Canon v0.4.1 changed the camera model to free camera (orbit/pan/zoom/tilt) with isometric preset snap views. The original orthographic-only decision was predicated on a fixed viewing angle and must be revisited.

## Canon Conflict

**Original Decision (v0.4.0):** Orthographic projection for "true isometric feel" — parallel lines stay parallel, no foreshortening.

**New Canon (v0.4.1):** Free camera with full orbit/pan/zoom/tilt. Orthographic projection feels unnatural when orbiting freely (objects maintain constant screen size regardless of distance, disorienting during rotation).

## Options

### A) Orthographic Always
- Pros: Grid clarity, parallel lines, classic isometric feel at presets
- Cons: Feels unnatural during free orbit/tilt, disorienting at non-standard angles, objects don't recede with distance

### B) Hybrid (Orthographic for Presets, Perspective for Free)
- Pros: Best of both worlds aesthetically
- Cons: Projection switch creates visual discontinuity, picking/edge detection behavior changes between modes, significantly more complex

### C) Perspective Always with Narrow FOV (~35°)
- Pros: Natural free camera feel, single projection type eliminates discontinuities in picking/edge detection/shadows, narrow FOV approximates orthographic at preset angles, subtle depth cues enhance 3D feel
- Cons: Slight foreshortening visible (grid lines not perfectly parallel), not "true" isometric

## Decision

**Option C: Perspective projection always, with narrow FOV (~35°).**

### Rationale

1. **All three agents independently converged on this recommendation** — Systems Architect, Graphics Engineer, and Game Designer all favor perspective with narrow FOV.
2. **Single projection type eliminates discontinuities** — picking, edge detection, shadow mapping, and bloom all work consistently regardless of camera mode.
3. **Narrow FOV approximates orthographic** — at 35° FOV, foreshortening is minimal. At isometric preset angles, the visual result is very close to orthographic.
4. **Natural free camera feel** — perspective projection is what players expect from free camera orbit/tilt.
5. **Simpler implementation** — one code path instead of two.

### Specification

| Parameter | Value |
|-----------|-------|
| Projection type | Perspective |
| Field of view | ~35° vertical (tunable via ToonShaderConfig) |
| Near plane | 0.1 |
| Far plane | 1000.0 |
| Aspect ratio | Window width / height |

## Consequences

- Canon files to update: None required — canon specifies "free camera" but does not mandate a projection type
- Ticket 2-021 changes from "Orthographic projection" to "Perspective projection with narrow FOV"
- Ticket 2-024 zoom uses camera distance (perspective), not view volume width (orthographic)
- Ticket 2-029 ray casting uses divergent rays (perspective), not parallel rays (orthographic)
- Ticket 2-030 tile picking works naturally with perspective
- Ticket 2-006 edge detection uses normal-based edges primarily (depth non-linear in perspective)
- Other epics affected: None — projection type is internal to Epic 2
