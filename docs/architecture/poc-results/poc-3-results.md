# POC-3 Results: Dense Grid Performance at Scale

**Date:** 2026-02-04
**Status:** APPROVED
**Verdict:** Systems Architect APPROVE with conditions

---

## Summary

POC-3 validates that a dense 2D grid (4 bytes/tile, row-major layout) can handle 512×512 tile maps with performance far exceeding targets. Full iteration, neighbor aggregation, LZ4 serialization, and memory footprint all pass with 4-39x headroom on the target thresholds.

---

## Benchmark Results

### Target Size: 512×512 (262,144 tiles)

| Metric | Target | Measured | Headroom | Result |
|--------|--------|----------|----------|--------|
| Full iteration (read+write) | ≤0.5ms | **0.055ms avg** | 9x | **PASS** |
| 3×3 neighbor aggregation | ≤2ms | **0.456ms avg** | 4.4x | **PASS** |
| LZ4 serialize (comp+decomp) | ≤10ms | **0.257ms avg** | 39x | **PASS** |
| Memory (5 grids) | ≤12 MB | **5.00 MB** | 2.4x | **PASS** |

### Scaling Across Grid Sizes

| Grid Size | Tiles | Full Iteration | 3×3 Neighbors | LZ4 Total | Memory (5 grids) |
|-----------|-------|---------------|---------------|-----------|------------------|
| 128×128 | 16,384 | 0.004ms | 0.029ms | 0.010ms | 0.31 MB |
| 256×256 | 65,536 | 0.014ms | 0.112ms | 0.072ms | 1.25 MB |
| 512×512 | 262,144 | 0.055ms | 0.456ms | 0.257ms | 5.00 MB |

Scaling is approximately linear (4x tiles → ~4x time), confirming cache-friendly behavior.

### LZ4 Compression Details (512×512)

| Metric | Value |
|--------|-------|
| Original size | 1,048,576 bytes (1.00 MB) |
| Compressed size | 50,806 bytes (49.6 KB) |
| Compression ratio | 4.8% of original |
| Compress time | 0.237ms avg |
| Decompress time | 0.020ms avg |
| Round-trip verified | Yes |

The 4.8% compression ratio is excellent for network sync. A full 512×512 map compresses to ~50 KB.

---

## Detailed Timing (512×512, 100 iterations)

| Benchmark | Min | Avg | Max |
|-----------|-----|-----|-----|
| Full iteration | 0.053ms | 0.055ms | 0.060ms |
| 3×3 neighbor ops | 0.414ms | 0.456ms | 0.688ms |
| LZ4 compress | 0.226ms | 0.237ms | 0.351ms |
| LZ4 decompress | 0.019ms | 0.020ms | 0.024ms |

Low variance between min/avg/max indicates consistent, cache-friendly performance.

---

## What Was Implemented

1. **TerrainComponent** - 4-byte struct: `{terrain_type, elevation, moisture, flags}` (all uint8)
2. **TerrainGrid** - Dense row-major `std::vector<TerrainComponent>` with O(1) coordinate lookup
3. **Full iteration benchmark** - Read + write pass over all tiles via range-based for loop
4. **Neighbor aggregation benchmark** - 3×3 elevation sum for every interior tile (smoothing pass simulation)
5. **LZ4 serialization benchmark** - Compress + decompress with round-trip verification
6. **Memory measurement** - 5 grids (terrain, elevation overlay, moisture overlay, zone overlay, building flags)
7. **Multi-size testing** - 128×128, 256×256, 512×512 with pass/fail thresholds on 512×512

---

## Architecture Assessment (Systems Architect)

### Strengths

- Implementation perfectly aligns with the Epic 3 terrain system design
- 4-byte struct with `static_assert` enforcement is exactly what was specified
- Row-major layout (`y * width_ + x`) delivers expected cache-friendly traversal
- O(1) coordinate lookup via `at(x, y)` method
- Bounds checking with `in_bounds()` accepts signed integers (handles negative coordinates safely)
- Raw data access for serialization is well-designed
- Range-based for loop support enables idiomatic C++ iteration
- Benchmark methodology is sound (warm-up run, 100 iterations, min/max/avg tracking)
- LZ4 round-trip verification catches serialization bugs
- `volatile sink` prevents compiler from optimizing away benchmark work

### ECS Spirit Compliance

The implementation preserves ECS separation of concerns:

| Concern | Implementation |
|---------|----------------|
| Data | `TerrainComponent` is a pure 4-byte struct with no logic |
| Logic | Benchmark functions simulate system operations |
| Identity | Grid coordinates (x, y) serve as implicit entity IDs |

### Frame Budget Impact

At 60 FPS, frame budget is 16.67ms. POC operations:
- Full iteration: 0.055ms = 0.3% of frame
- 3×3 neighbor ops: 0.456ms = 2.7% of frame
- **Combined terrain ops: ~3% of frame budget**

This leaves ample room for other systems (rendering, AI, networking, UI).

### Multi-Grid Memory Budget

The POC validates 5 simultaneous grids at 5 MB. All planned grid types fit within budget:

| Grid Type | Size/tile | 512×512 Memory |
|-----------|-----------|----------------|
| TerrainGrid | 4 bytes | 1 MB |
| BuildingGrid | 4 bytes | 1 MB |
| EnergyCoverageGrid | 1 byte | 256 KB |
| FluidCoverageGrid | 1 byte | 256 KB |
| PathwayGrid | 4 bytes | 1 MB |
| ProximityCache | 1 byte | 256 KB |
| DisorderGrid (double-buffered) | 1 byte | 512 KB |
| ContaminationGrid (double-buffered) | 2 bytes | 1 MB |
| LandValueGrid | 2 bytes | 512 KB |
| ServiceCoverageGrid (per service) | 1 byte | 256 KB each |
| **Total (conservative)** | | **~7-8 MB** |

---

## Issues Identified

### Minor Issues

| ID | Issue | Resolution |
|----|-------|------------|
| M-001 | Naming: struct is `TerrainComponent` but decision record recommends `TerrainTileData` to avoid ECS confusion | Rename for production |
| M-002 | No bounds checking on `at()` method | Acceptable for performance; document that `at()` is unchecked |
| M-003 | No `TerrainType` enum defined | Fine for POC; add enum for production |

---

## Conditions for Production Integration

1. **Rename `TerrainComponent` to `TerrainTileData`** per decision record to avoid ECS naming confusion
2. **Document the flags bitfield** - define bit meanings: `buildable`, `has_road`, `has_water`, `has_vegetation`, etc.
3. **Add dirty region tracking** for efficient network sync and render updates
4. **Create generic `Grid<T>` template** to reuse for other grid types (BuildingGrid, EnergyCoverageGrid, etc.)

---

## What Is Missing (Expected for POC)

These are not in scope for the POC but required for production:

- TerrainType enum definition
- Flags bitfield documentation
- Multi-grid management class (TerrainManager)
- Dirty tracking for change detection
- Thread safety (double-buffering or row-level locking)
- Iterator adapters (row iterators, region iterators)
- Memory alignment (64-byte cache line alignment)

---

## Key Technical Decisions Validated

| Decision | Validated? |
|----------|-----------|
| Dense grid over per-entity ECS | Yes - 4 bytes/tile, O(1) lookup, excellent cache behavior |
| Row-major layout | Yes - linear scan performance is excellent |
| 512×512 map size | Yes - all operations well within budget |
| LZ4 for network serialization | Yes - 50 KB compressed, sub-millisecond round-trip |
| 5 overlay grids within memory budget | Yes - 5 MB total, well under 12 MB target |
| std::vector storage | Yes - no custom allocator needed at this scale |

---

## Integration Readiness

| Consumer System | Readiness | Notes |
|-----------------|-----------|-------|
| RenderingSystem | Ready | `raw_data()` + `at()` provide needed access |
| NetworkManager | Ready | LZ4 compression validated at 4.8% size |
| TerrainSystem | Ready | Will wrap this grid |
| Other grids | Needs template | Create `Grid<T>` for code reuse |

---

## Implications for Production

- **Map size:** 512×512 is viable with massive headroom. Could support 1024×1024 if needed.
- **Network sync:** Full grid snapshot at 50 KB compressed is small enough for reliable network sync. Seed+modifications strategy is optional but not required for bandwidth reasons.
- **Frame budget:** Terrain operations consume ~3% of frame budget, leaving ample room for other systems.
- **Multiple grids:** 5 simultaneous grids at 5 MB total is conservative. Could support 10+ grids without concern.

---

## Test Hardware

- **CPU:** (Windows 10 build 26200)
- **Compiler:** MSVC 19.44.35222.0
- **Build:** Release mode (optimizations enabled)
- **Standard:** C++17

---

## Files

```
poc/poc3-dense-grid/
├── CMakeLists.txt
├── terrain_grid.h
└── main.cpp
```
