# Confidence Assessment: Epic 2 3D Rendering Tickets

**Assessed:** 2026-01-27
**Assessor:** Systems Architect
**Plan:** docs/epics/epic-2/tickets.md

## Overall Confidence: HIGH (Caveats Resolved)

This is a well-structured, comprehensive plan that closely aligns with canon decisions. The plan demonstrates strong understanding of the 3D rendering requirements established in Canon v0.4.0, with clear dependencies and realistic scope. All initial concerns around multiplayer integration, system boundaries, and technical specifications have been addressed.

## Dimension Ratings

| Dimension | Rating | Rationale |
|-----------|--------|-----------|
| Canon Alignment | HIGH | Correctly implements SDL_GPU, toon shading, fixed isometric camera, ECS patterns, and separation of RenderingSystem/CameraSystem per canon. Terminology matches (though uses "ZergCity" correctly). Component patterns follow ECS rules. |
| System Boundaries | HIGH | Clear delineation between RenderingSystem (drawing) and CameraSystem (matrices/controls) matches systems.yaml. TransformComponent vs PositionComponent separation correctly documented. RenderComponent follows component patterns. |
| Multiplayer Readiness | HIGH | Transform interpolation (2-044) and cursor rendering (2-045) address multiplayer basics. IRenderStateProvider and ICursorSync interfaces now defined in interfaces.yaml with stub implementations for local/single-player mode. Epic 1 integration points are clear. 20Hz simulation tick rate matches canon (50ms = 20 ticks/sec). |
| Dependency Clarity | HIGH | Excellent dependency graph and critical path analysis. Clear "Depends On" and "Blocks" for each ticket. Group organization (A-K) makes logical sense. Epic 0 dependencies clearly identified. |
| Implementation Feasibility | HIGH | 45 tickets for a complete 3D rendering engine is realistic. Target metrics (60 FPS, <8ms render, 500-1000 draw calls, <100k triangles) are achievable. HLSL/SPIR-V/DXIL shader pipeline is proven technology. GPU instancing for performance is well-scoped. |
| Risk Coverage | HIGH | Debug tools (Group J) help with development. Placeholder model (2-013) prevents missing asset crashes. Shader fallback strategy (2-004) handles runtime compilation failures with embedded fallback shaders. GPU memory budget established with resource breakdown. |

## Strengths

- **Comprehensive Coverage:** All aspects of a 3D toon-shaded rendering pipeline are addressed - from device initialization through post-processing effects.
- **Clear Critical Path:** The 14-step critical path to "something visible on screen" provides excellent milestone tracking.
- **ECS Compliance:** Components (RenderComponent, TransformComponent) and system organization follow canon ECS patterns exactly.
- **Performance-Aware Design:** GPU instancing (2-012), frustum culling (2-026), draw call budgets, and frame time targets show proper performance consideration.
- **Excellent Dependency Mapping:** The ASCII dependency graph and per-ticket dependency annotations make implementation order unambiguous.
- **Debug Tooling:** Grid overlay, wireframe mode, render stats, and bounding box visualization provide solid debugging infrastructure.
- **Future-Proofing:** Camera parameters stored as config (not hardcoded), layer visibility system supports future underground view, shader configs are runtime-adjustable.

## Key Concerns

- **Epic 1 Soft Dependency Ambiguity:** Tickets 2-044 and 2-045 depend on Epic 1 (SyncSystem), but the plan notes Epic 1 is a "soft dependency." This creates ambiguity about when multiplayer visual features should actually be implemented. If Epic 1 is delayed significantly, these tickets block.

- **TransformComponent Authority:** Canon systems.yaml shows RenderingSystem queries components for "what to render," but doesn't explicitly define who owns TransformComponent. Ticket 2-032 assigns it to Systems Architect. Clarify: is TransformComponent owned by RenderingSystem or a shared cross-cutting concern?

- **Shader Hot-Reload in Release:** Ticket 2-004 mentions shader hot-reload for development but doesn't address handling of shader compilation failures at runtime. What happens if a player has corrupted shader cache?

- **UI Overlay Integration:** Tickets reference "SDL_Renderer for 2D UI overlay" (2-019) but Epic 12 (UI) is Phase 3. The integration point between 3D SDL_GPU pipeline and 2D SDL_Renderer needs more detail to prevent conflicts.

- **No Explicit Memory Budget:** While draw call and triangle budgets are specified, GPU memory usage targets are mentioned only in stats display (2-042) rather than as design constraints.

## Recommendations Before Proceeding

- [x] **Clarify TransformComponent Ownership:** Added to systems.yaml under cross_cutting section. Owned by RenderingSystem, client-side only, derived from PositionComponent.

- [x] **Define Epic 1 Integration Point:** Added IRenderStateProvider and ICursorSync interfaces to interfaces.yaml with stub implementation guidance for local/single-player mode.

- [x] **Document SDL_Renderer + SDL_GPU Coexistence:** Added detailed integration approach to ticket 2-019 including frame structure, constraints, and alternative fallback approach.

- [x] **Add Shader Cache Fallback:** Extended ticket 2-004 with embedded fallback shaders, loading priority, and cache corruption handling.

- [x] **Establish GPU Memory Budget:** Added to Target Metrics: <512MB min spec, <1GB recommended, with resource breakdown by type.

## Proceed?

**YES - ALL CAVEATS RESOLVED**

All recommendations have been addressed:
- `systems.yaml` updated with TransformComponent ownership
- `interfaces.yaml` updated with rendering interfaces for Epic 1 integration
- Ticket 2-019 updated with SDL_Renderer + SDL_GPU coexistence
- Ticket 2-004 updated with shader fallback strategy
- Target Metrics updated with GPU memory budget

The plan is ready for implementation.
