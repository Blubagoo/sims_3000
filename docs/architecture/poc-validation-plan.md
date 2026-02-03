# POC Validation Plan: Closing the Uncertainty Gap

**Created:** 2026-02-01
**Current Confidence:** 6.5/10
**Target Confidence:** 8.5/10
**ClickUp Tickets:** POC-1 through POC-4

---

## Executive Summary

Before committing to 900+ tickets across 18 epics, we need to validate 4 high-risk technical assumptions. Each POC has clear quantifiable benchmarks and explicit success/failure criteria.

**If all POCs succeed:** Proceed with Epic 0 implementation, confidence rises to 8.5/10.

**If any POC fails:** Evaluate fallback options documented in each POC before proceeding.

---

## The Four POCs

| POC | Risk Area | Affects | Priority |
|-----|-----------|---------|----------|
| **POC-1** | SDL_GPU Toon Shading | Epic 2 (Rendering) + all visual systems | P1 - Critical |
| **POC-2** | ENet Multiplayer Sync | Epic 1 (Networking) + all systems | P1 - Critical |
| **POC-3** | Dense Grid Performance | Architectural foundation, 10+ systems | P2 - High |
| **POC-4** | ECS Simulation Loop | Epic 10 (Simulation) + all gameplay | P2 - High |

---

## POC Execution Strategy

### Parallel Execution

All POCs are **independent** and can be developed in parallel:

```
POC-1 (Rendering)     ████████████████░░░░  [Independent]
POC-2 (Networking)    ████████████████░░░░  [Independent]
POC-3 (Grids)         ████████████████░░░░  [Independent]
POC-4 (ECS)           ████████████████░░░░  [Independent]
```

### Demo Output

Each POC produces a **working demo** that:
1. Runs standalone (no game code dependency)
2. Demonstrates the core technical capability
3. Outputs benchmark measurements to console/file
4. Can be re-run to validate after changes

### Success Threshold

A POC **succeeds** if:
- All quantifiable benchmarks meet targets (not failure thresholds)
- All success criteria checkboxes are complete
- The technique is documented and reproducible

A POC **fails** if:
- Any metric hits the failure threshold
- A fundamental blocker is discovered
- The approach requires unacceptable workarounds

---

## Benchmark Summary

### POC-1: SDL_GPU Toon Shading

| Metric | Target | Failure |
|--------|--------|---------|
| Frame time @ 1000 instances | ≤16.67ms | >33.33ms |
| Draw calls @ 1000 instances | ≤10 | >100 |
| GPU memory @ 1000 instances | <256 MB | >512 MB |
| Outline quality | Clean, consistent | Artifacts |

### POC-2: ENet Multiplayer Sync

| Metric | Target | Failure |
|--------|--------|---------|
| Bandwidth/client @ 50K entities | ≤100 KB/s | >200 KB/s |
| Snapshot processing time | ≤5ms | >15ms |
| Late-join transfer time | ≤1s | >5s |
| Desync rate (5% packet loss) | 0 | Any |

### POC-3: Dense Grid Performance

| Metric | Target | Failure |
|--------|--------|---------|
| Full iteration (512×512) | ≤0.5ms | >2ms |
| 3×3 neighbor ops | ≤2ms | >10ms |
| LZ4 serialization (all grids) | ≤10ms | >30ms |
| Memory (all grids) | ≤12 MB | >20 MB |

### POC-4: ECS Simulation Loop

| Metric | Target | Failure |
|--------|--------|---------|
| Total tick time @ 50K entities | ≤25ms | >50ms |
| Event dispatch (1000/tick) | ≤1ms | >5ms |
| Query time (3 components) | ≤1ms | >5ms |
| Memory per entity | ≤64 bytes | >128 bytes |

---

## Fallback Options

### If POC-1 Fails (SDL_GPU)

| Option | Tradeoff |
|--------|----------|
| Raw OpenGL 4.x | More code, but proven |
| Raw Vulkan | More code, maximum control |
| bgfx/sokol_gfx | Different abstraction, less SDL integration |
| Simplify visuals | No outlines, basic shading |

### If POC-2 Fails (Networking)

| Option | Tradeoff |
|--------|----------|
| Reduce sync rate | 10Hz feels less responsive |
| Interest management | Complex, but reduces bandwidth |
| GameNetworkingSockets | Different API, Valve's library |
| Reduce map size | 256×256 max instead of 512×512 |

### If POC-3 Fails (Grids)

| Option | Tradeoff |
|--------|----------|
| Sparse grids | More complex code, less cache-friendly |
| Reduce map size | 256×256 max |
| Hierarchical grids | Quadtree, more complex |
| GPU grid operations | Requires compute shaders |

### If POC-4 Fails (ECS)

| Option | Tradeoff |
|--------|----------|
| Different ECS (flecs) | Learning curve, migration |
| Simplify simulation | Fewer interacting systems |
| Batch updates | Less granular, but faster |
| Hybrid architecture | Some systems outside ECS |

---

## Post-POC Decision Matrix

| POC-1 | POC-2 | POC-3 | POC-4 | Decision |
|-------|-------|-------|-------|----------|
| ✅ | ✅ | ✅ | ✅ | **Proceed with Epic 0** (confidence: 8.5/10) |
| ✅ | ✅ | ✅ | ❌ | Evaluate ECS alternatives, then proceed |
| ✅ | ✅ | ❌ | ✅ | Reduce map size or implement sparse grids |
| ✅ | ❌ | ✅ | ✅ | Re-architect networking (biggest change) |
| ❌ | ✅ | ✅ | ✅ | Switch rendering approach (significant work) |
| ❌ | ❌ | * | * | **Major re-architecture required** |

---

## Definition of Done (All POCs)

- [ ] POC-1 demo runs and meets benchmarks
- [ ] POC-2 demo runs and meets benchmarks
- [ ] POC-3 demo runs and meets benchmarks
- [ ] POC-4 demo runs and meets benchmarks
- [ ] All benchmark data recorded in `/docs/architecture/poc-results/`
- [ ] Any failures documented with fallback decision
- [ ] Confidence assessment updated
- [ ] Go/no-go decision documented

---

## Project Confidence Update

After POC validation:

| Scenario | New Confidence |
|----------|----------------|
| All POCs pass | **8.5/10** |
| 3/4 POCs pass (with fallback) | **7.5/10** |
| 2/4 POCs pass | **6.0/10** - Re-evaluate scope |
| <2 POCs pass | **4.0/10** - Major re-architecture |

---

## Next Steps After POC Completion

1. **Update confidence assessment** with actual data
2. **Document learnings** in architecture decisions
3. **Revise tickets** if POC revealed new work
4. **Begin Epic 0** with validated technical foundation
5. **Carry POC code forward** as reference implementations

---

*This plan ensures we validate high-risk assumptions before investing in 900+ tickets of implementation work.*
