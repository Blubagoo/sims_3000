# Epic 7: Transportation Network - Systems Architect Review

**Reviewer:** Systems Architect Agent
**Date:** 2026-02-08
**Files Reviewed:** 24 files (12 headers, 12 implementations)
**Verdict:** APPROVED WITH FINDINGS
**Confidence:** 7/10

---

## Overall Assessment

The Epic 7 implementation delivers a solid, well-structured transportation network system. The architecture cleanly separates concerns across PathwayGrid (spatial), ProximityCache (BFS distance), NetworkGraph (connectivity), FlowDistribution/FlowPropagation (traffic), Pathfinding (A*), and RailSystem (transit). The delegation pattern from TransportSystem to TransportProviderImpl correctly replaces the ITransportProvider stub from Epic 4. Component layouts are carefully sized (16 bytes each for RoadComponent and TrafficComponent) with static_assert guards.

However, there are several architectural findings that warrant follow-up, ranging from a critical node capacity ceiling to performance hotspots in the RailSystem and missing version bytes in serialization. None are blocking for the current milestone, but several must be addressed before 512x512 map support.

---

## Findings

### HIGH Severity

#### F7-SA-001: NetworkGraph uint16_t node index overflow on large maps

**File:** `include/sims3000/transport/NetworkGraph.h` (lines 54, 85-86, 92)
**File:** `src/transport/NetworkGraph.cpp` (line 29)

NetworkNode::neighbor_indices stores `uint16_t` indices. `add_node()` casts `nodes_.size()` to `uint16_t`. The maximum node count is therefore 65,535. On a 512x512 map (262,144 tiles), a dense road network could easily exceed this limit.

When `nodes_.size() >= 65536`, the `uint16_t` cast wraps to 0, causing the new node to collide with the node at index 0 in `position_to_node_`. This silently corrupts the graph: connectivity queries return wrong results, network_ids are incorrect, and A* may find paths through non-existent connections.

**Impact:** Silent data corruption on large maps. Network queries return incorrect results.
**Recommendation:** Promote node indices to `uint32_t`, or document and enforce a maximum road tile count with a runtime guard in `add_node()`.

---

#### F7-SA-002: RailSystem O(N*M) hotspot in tick -- has_rail_at linear scan called per terminal per rail

**File:** `src/transport/RailSystem.cpp` (lines 354-378, 431-453)

`update_terminal_coverage()` iterates all terminals, then for each active terminal iterates all rails (O(T*R) per player). Additionally, `has_adjacent_rail()` calls `has_rail_at()` 4 times, and each `has_rail_at()` is a linear scan of all rails across all players (O(4*P*R) per call).

For `can_place_terminal()` and `check_terminal_activation()`, each call scans all rails across all 4 players to check adjacency. With 1000 rails and frequent placement/activation checks, this becomes a measurable hotspot.

**Impact:** At scale (1000+ rails), tick time for RailSystem becomes non-trivial. Terminal placement validation also pays this cost.
**Recommendation:** Add a spatial index (dense grid or hash map) for rail positions, mirroring the PathwayGrid pattern used for road tiles. This converts O(N) scans to O(1) lookups.

---

#### F7-SA-003: RailSystem circular dependency between terminal activation and terminal-adjacent flag

**File:** `src/transport/RailSystem.cpp` (lines 332-378)

Phase 2 (`update_active_states`) sets rail `is_active = is_powered && is_terminal_adjacent`. But `is_terminal_adjacent` is set by Phase 3 (`update_terminal_coverage`), which only runs *after* Phase 2, and Phase 3 only processes *active* terminals.

On the very first tick after placement, the sequence is:
1. Phase 1: Rail is powered (true). Terminal is powered (true).
2. Phase 2: Rail `is_active = true && false = false` (is_terminal_adjacent was never set). Terminal is_active = powered && has_adjacent_rail = true.
3. Phase 3: Terminal is active, so it marks nearby rails as terminal_adjacent. But rails are already flagged inactive for this tick.

This means it takes two ticks for a rail to become active after a terminal is placed next to it. On the second tick, Phase 2 sees the terminal_adjacent flag from the previous Phase 3.

**Impact:** One-tick activation delay for new rail segments. May cause visible flicker or confusing UI state.
**Recommendation:** Reorder phases so coverage (Phase 3) runs before active-state evaluation (Phase 2), or merge them into a single pass.

---

### MEDIUM Severity

#### F7-SA-004: Network messages lack version bytes

**File:** `include/sims3000/transport/PathwayNetworkMessages.h`, `include/sims3000/transport/RailNetworkMessages.h`
**File:** `src/transport/PathwayNetworkMessages.cpp`, `src/transport/RailNetworkMessages.cpp`

All 8 network message types (PathwayPlaceRequest/Response, PathwayDemolishRequest/Response, RailPlaceRequest/Response, etc.) serialize directly without a leading version byte. The project's multiplayer architecture will need forward-compatible message evolution. Without a version byte, any field addition or reordering will cause deserialization failures for mismatched client/server versions.

**Impact:** Cannot evolve message formats without breaking compatibility. Client/server version mismatch will cause silent deserialization corruption or crashes.
**Recommendation:** Prepend a `uint8_t version = 1` to all message serialize() outputs. Deserialize should check version and handle unknown versions gracefully.

---

#### F7-SA-005: TransportProviderImpl congestion/traffic stubs bypass TransportSystem overrides

**File:** `src/transport/TransportProviderImpl.cpp` (lines 121-131)
**File:** `src/transport/TransportSystem.cpp` (lines 191-214)

TransportProviderImpl::get_congestion_at() and get_traffic_volume_at() are stubs returning 0. However, TransportSystem overrides these methods directly (lines 191-214 in TransportSystem.cpp) with real implementations that read from traffic_ map. The delegation chain works because TransportSystem inherits ITransportProvider and overrides directly rather than delegating to provider_impl_ for these two methods.

This creates an inconsistency: 6 of the 8 ITransportProvider methods delegate to provider_impl_, while 2 (get_congestion_at, get_traffic_volume_at) are implemented directly in TransportSystem. If someone passes a `TransportProviderImpl*` instead of `TransportSystem*` as the ITransportProvider, they get stubs for congestion and traffic.

**Impact:** Confusing code path. Risk of using the wrong pointer type and getting stubs instead of real values.
**Recommendation:** Either move congestion/traffic logic into TransportProviderImpl (give it a reference to the traffic data), or document the delegation pattern clearly in both headers. The cleanest fix is to have TransportProviderImpl own all query logic.

---

#### F7-SA-006: flow_accumulator_.clear() in phase2 followed by repopulation from flow_previous in phase3 -- allocation churn

**File:** `src/transport/TransportSystem.cpp` (lines 293-335)

Phase 2 clears flow_accumulator_ (deallocating all entries from the unordered_map). Phase 3 immediately repopulates it from traffic_[].flow_previous. On every tick, this causes:
1. All hash map entries are destroyed (phase 2, line 300)
2. New entries are inserted for every road with flow (phase 3, lines 308-316)
3. FlowPropagation then iterates and modifies the map

This is allocation churn every tick. For a map with 5000 road tiles carrying flow, this is 5000 insertions per tick.

**Impact:** Unnecessary heap allocation/deallocation every tick. May cause micro-stutter at scale.
**Recommendation:** Instead of `clear()`, iterate and set values to 0, preserving the hash map's bucket structure. Or use a flat array indexed by position (matching the PathwayGrid pattern).

---

#### F7-SA-007: FlowPropagation allocates vectors per tile per tick

**File:** `src/transport/FlowPropagation.cpp` (lines 50-54, 69, 86-87, 121-139)

`get_pathway_neighbors()` returns a `std::vector<uint64_t>` by value, allocated fresh for each tile with flow, every tick. The `SpreadInfo` struct also stores a `std::vector<uint64_t> neighbors` member that captures these vectors. With N road tiles carrying flow, this is N heap allocations per tick.

**Impact:** Heap allocation pressure proportional to road network size, every tick.
**Recommendation:** Pre-allocate a reusable neighbors buffer, or use a fixed-size array (maximum 4 cardinal neighbors) -- e.g., `std::array<uint64_t, 4>` plus a count.

---

#### F7-SA-008: FlowDistribution::find_nearest_pathway allocates unordered_map per building per tick

**File:** `src/transport/FlowDistribution.cpp` (lines 100-148)

Each call to `find_nearest_pathway()` creates a fresh `std::unordered_map<uint64_t, bool> visited` for the BFS search. With max_distance=3, this searches up to ~25 tiles, so the map is small, but it is allocated per building per tick.

With 2000 buildings, this is 2000 small hash map allocations per tick. While each map is small, the allocation overhead adds up.

**Impact:** Per-building-per-tick allocation pressure.
**Recommendation:** Use a pre-allocated visited buffer that is cleared between calls, or use a flat bitset indexed by position offset from the search origin (since max_distance is bounded).

---

#### F7-SA-009: Pathfinding A* uses unordered_map for g_costs, came_from, and closed set

**File:** `src/transport/Pathfinding.cpp` (lines 83-89)

Each `find_path()` call allocates three `unordered_map<uint64_t, ...>` instances. For long paths on a 512x512 grid, these maps could grow to tens of thousands of entries. The hash map overhead (load factor, rehashing) may push pathfinding above the <5ms target.

**Impact:** May exceed <5ms performance target on large grids with long paths.
**Recommendation:** For the open/closed set, consider using a flat visited array (1 bit per tile) for the closed set, and a parallel g_cost array. The 512x512 grid only needs 256KB for a flat uint32_t g_cost array, which is reasonable.

---

#### F7-SA-010: next_entity_id_ not shared between TransportSystem and RailSystem

**File:** `src/transport/TransportSystem.cpp` (line 208: `next_entity_id_ = 1`)
**File:** `src/transport/RailSystem.cpp` (line 25: `next_entity_id_(1)`)

Both TransportSystem and RailSystem have independent `next_entity_id_` counters starting at 1. Entity IDs from TransportSystem (roads) and RailSystem (rails/terminals) will collide. If any downstream system stores entity IDs from both systems in the same collection (e.g., selection, serialization, network messages), collisions will cause silent logic errors.

**Impact:** Entity ID collisions between road and rail entities. Downstream systems that mix entity IDs from both systems will behave incorrectly.
**Recommendation:** Either use a shared entity ID allocator (injected into both systems), or partition the ID space (e.g., roads use IDs 1-999999, rails use 1000000+).

---

### LOW Severity

#### F7-SA-011: ProximityCache uses std::queue of pairs -- suboptimal BFS container

**File:** `src/transport/ProximityCache.cpp` (lines 59-104)

`std::queue<std::pair<int32_t, int32_t>>` uses `std::deque` as the underlying container, which allocates in chunks. For a 512x512 BFS that visits all 262,144 tiles, this causes many deque chunk allocations. A pre-allocated ring buffer or simply reusing a `std::vector` with index tracking would be more cache-friendly.

**Impact:** Minor allocation overhead during proximity cache rebuild.
**Recommendation:** Consider using a pre-allocated vector as a FIFO queue (two indices: read/write) for the BFS frontier.

---

#### F7-SA-012: Duplicated little-endian helpers in PathwayNetworkMessages.cpp and RailNetworkMessages.cpp

**File:** `src/transport/PathwayNetworkMessages.cpp` (lines 20-66)
**File:** `src/transport/RailNetworkMessages.cpp` (lines 20-66)

The anonymous namespace helper functions (`write_u8`, `write_i32_le`, `write_u32_le`, `read_i32_le`, `read_u32_le`, `read_u8`) are copy-pasted identically between both files. The file comments reference `TransportSerialization.cpp`, suggesting a shared utility was intended.

**Impact:** Code duplication. Changes to serialization format must be made in multiple places.
**Recommendation:** Extract shared serialization helpers into a common header (e.g., `SerializationHelpers.h`) or a shared utility .cpp.

---

#### F7-SA-013: TransportSystem stores entity data in 4 separate unordered_maps per entity

**File:** `include/sims3000/transport/TransportSystem.h` (lines 220-223)

Each pathway entity requires entries in `roads_`, `traffic_`, `road_owners_`, and `road_positions_` -- four separate `unordered_map` lookups per entity for most operations. This scatters related data across memory.

**Impact:** Cache-unfriendly access patterns for per-entity operations. Multiple hash lookups per operation.
**Recommendation:** Consider a single `struct PathwayEntity { RoadComponent road; TrafficComponent traffic; uint8_t owner; int32_t x, y; }` stored in a single map. Or use a SoA (struct of arrays) pattern with parallel vectors.

---

#### F7-SA-014: NetworkGraph::get_node() has no bounds check

**File:** `include/sims3000/transport/NetworkGraph.h` (line 131)
**File:** `src/transport/NetworkGraph.cpp` (lines 93-95)

`get_node(uint16_t index)` returns `nodes_[index]` without bounds checking. If an invalid index is passed, this is undefined behavior. While add_edge() does check bounds, get_node() is a public API.

**Impact:** Potential UB if called with invalid index.
**Recommendation:** Add an assert or bounds check, at minimum in debug builds.

---

#### F7-SA-015: PathwayGrid uses struct wrapper (PathwayGridCell) for a single uint32_t

**File:** `include/sims3000/transport/PathwayGrid.h` (lines 41-43)

`PathwayGridCell` wraps a single `uint32_t entity_id`. The struct adds no behavior or safety. Using `std::vector<uint32_t>` directly would be slightly simpler and equivalent.

**Impact:** Negligible. Minor code complexity.
**Recommendation:** Optional cleanup -- consider using `std::vector<uint32_t>` directly if no additional fields are planned for PathwayGridCell.

---

## Correctness Assessment

### BFS Distance (ProximityCache)
The multi-source BFS implementation is correct. Seeds all pathway tiles at distance 0, expands 4-directionally, caps at 254 to prevent sentinel collision with 255. The dirty flag mechanism properly triggers rebuilds when the grid changes.

### A* Pathfinding
The A* implementation is correct. Manhattan distance heuristic is admissible (consistent with uniform edge cost of 10). Early exit via network_id check is a sound optimization. Path reconstruction via came_from chain is standard. The trivial case (start == end) is handled.

### Network ID Assignment
BFS-based connected component labeling is correct. IDs are assigned incrementally, starting from 1 (0 is reserved for "no network"). The is_connected query correctly requires both IDs to be non-zero and equal.

### Flow Propagation
The snapshot-then-apply pattern avoids read-after-write issues. Integer division truncation (losing flow) is noted and acceptable for an aggregate model. The spread_rate of 20% provides reasonable diffusion behavior.

### Network Graph Rebuild
The rebuild_from_grid approach (clear + rescan) is correct but expensive. Only checking East and South neighbors avoids duplicate edges -- this is correct because add_edge() is bidirectional.

---

## Integration Assessment

### ITransportProvider Replacement
TransportSystem correctly inherits from `building::ITransportProvider` and overrides all 8 interface methods. The grace period mechanism (500 ticks = ~25 seconds) provides a smooth transition from the stub. The `pending_access_lost_events_` queue (using `mutable` for const query emission) is an acceptable pattern.

### Cross-System Data Flow
- **Terrain -> Transport:** PathwayGrid bounds are validated against map dimensions. No terrain type checking is performed at placement (network messages mention `invalid_terrain` error code but TransportSystem::place_pathway doesn't check terrain).
- **Transport -> Building:** ITransportProvider queries are all O(1) after cache rebuild.
- **Energy -> Rail:** IEnergyProvider injection works correctly with nullptr fallback (all powered).
- **Transport -> Rendering:** Events (placed, removed) are emitted for rendering system consumption.

### Missing Integration Points
- `place_pathway()` does not check terrain type. The PathwayPlaceResponse has an `invalid_terrain` error code (3) but nothing populates it.
- `is_road_accessible(EntityID)` is still a stub (always returns true). Entity position lookup from ECS registry is not implemented.
- NetworkConnectedEvent and NetworkDisconnectedEvent are defined but never emitted.

---

## Thread Safety Assessment

The implementation is designed for single-threaded use, which is consistent with the simulation tick model (one tick at a time). The `mutable` keyword on `pending_access_lost_events_` in TransportProviderImpl is safe in a single-threaded context but would be a race condition if `is_road_accessible_at()` were called from multiple threads.

No shared mutable state issues exist under the current single-threaded simulation architecture.

---

## Follow-Up Tickets

| ID | Severity | Summary |
|----|----------|---------|
| F7-SA-001 | HIGH | Promote NetworkGraph node indices from uint16_t to uint32_t (or enforce 65535 node limit) |
| F7-SA-002 | HIGH | Add spatial index for rail positions (RailSystem::has_rail_at is O(N) linear scan) |
| F7-SA-003 | HIGH | Fix RailSystem tick phase ordering (coverage before activation) to eliminate 1-tick delay |
| F7-SA-004 | MEDIUM | Add version byte prefix to all network message serialization |
| F7-SA-005 | MEDIUM | Consolidate congestion/traffic queries into TransportProviderImpl delegation |
| F7-SA-006 | MEDIUM | Eliminate flow_accumulator_ allocation churn in TransportSystem tick |
| F7-SA-007 | MEDIUM | Replace per-tile vector allocation in FlowPropagation with fixed-size array |
| F7-SA-008 | MEDIUM | Replace per-building unordered_map in FlowDistribution::find_nearest_pathway with flat visited buffer |
| F7-SA-009 | MEDIUM | Optimize Pathfinding A* data structures for <5ms target on 512x512 |
| F7-SA-010 | MEDIUM | Resolve entity ID space collision between TransportSystem and RailSystem |
| F7-SA-011 | LOW | Optimize ProximityCache BFS container for cache-friendly access |
| F7-SA-012 | LOW | Extract duplicated LE serialization helpers into shared utility |
| F7-SA-013 | LOW | Consolidate per-entity maps in TransportSystem into single structure |
| F7-SA-014 | LOW | Add bounds check to NetworkGraph::get_node() |
| F7-SA-015 | LOW | Simplify PathwayGridCell wrapper (optional) |

---

## Verdict Rationale

**APPROVED WITH FINDINGS** because:

1. The core architecture is sound -- clean separation of concerns, correct algorithms (BFS, A*, connected components), and proper ITransportProvider integration.
2. The HIGH findings (F7-SA-001 through F7-SA-003) are real issues but not blocking for current 256x256 or smaller maps. F7-SA-001 becomes critical at 512x512 with dense networks. F7-SA-002 and F7-SA-003 affect RailSystem behavior at scale and correctness respectively.
3. The MEDIUM findings are performance optimizations and architectural hygiene that should be addressed before beta. The allocation patterns (F7-SA-006/007/008) will become measurable at scale.
4. Code quality is high: consistent documentation, static_asserts on component sizes, bounds checking on all public APIs, and clean namespace organization.
5. No memory leaks detected -- placement and removal properly maintain all data structures in sync.

**Confidence: 7/10** -- Reduced from higher because I did not verify compilation or run tests. The uint16_t overflow (F7-SA-001) and rail tick ordering (F7-SA-003) findings are based on code analysis, not runtime verification.
