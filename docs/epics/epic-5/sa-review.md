# Epic 5: Energy Infrastructure -- Systems Architect Review

**Reviewer:** Systems Architect Agent
**Date:** 2026-02-07
**Epic:** 5 -- Power Infrastructure
**Canon Version:** 0.17.0
**Files Reviewed:** 20 (canon docs, planning docs, headers, implementation)

---

## Summary Verdict

**APPROVED WITH FINDINGS**

Epic 5 delivers a structurally sound energy system that faithfully implements the pool model, BFS coverage algorithm, priority-based rationing, and per-player isolation specified in canon. The implementation correctly inherits `building::IEnergyProvider`, follows the dense grid exception pattern for `CoverageGrid`, and provides clean integration points for downstream systems (BuildingSystem, ContaminationSystem). The 8-phase tick pipeline is well-ordered and deterministic in structure.

However, the review identifies several findings that should be tracked: a performance concern in terrain bonus lookup (O(N*M) linear scan), floating-point determinism risks in aging and pool state calculation, documentation inconsistencies, and a number of intentionally stubbed validation checks that will need activation in later epics. None of these block continued development, but several should be addressed before multiplayer integration testing.

---

## 1. System Boundary Analysis

### Ownership Compliance

EnergySystem's ownership scope aligns well with `systems.yaml`:

| Canon "owns" | Implementation | Status |
|---|---|---|
| Energy nexus simulation | `update_nexus_aging()`, `update_nexus_output()`, `place_nexus()` | Implemented |
| Energy pool calculation | `calculate_pool()`, `calculate_pool_state()` | Implemented |
| Energy coverage zone calculation | `recalculate_coverage()` BFS, `CoverageGrid` | Implemented |
| Energy distribution to buildings | `distribute_energy()`, `apply_rationing()` | Implemented |
| Brownout/blackout detection | Pool state machine (Healthy/Marginal/Deficit/Collapse) | Implemented |
| Plant efficiency and aging | `update_nexus_aging()` with asymptotic decay curve | Implemented |

| Canon "does not own" | Implementation | Status |
|---|---|---|
| Building energy consumption amounts | `energy_required` defined on `EnergyComponent` by consumers | Correct |
| Energy UI | No UI code present | Correct |
| Energy costs | `DEFAULT_CONDUIT_COST` defined but explicitly marked as "not actually deducted yet" | Correct |

### Boundary Violations

No boundary violations detected. The system correctly defers to:
- `ITerrainQueryable` for buildability checks (not owning terrain)
- `EnergyComponent.energy_required` for consumption amounts (not owning building definitions)
- `ICreditProvider` for cost deduction (stubbed with TODO comment)

### Interface Contract Compliance

**IEnergyProvider:** EnergySystem inherits `building::IEnergyProvider` and implements both `is_powered()` and `is_powered_at()`. The canon interface in `interfaces.yaml` also specifies `get_pool_state(owner)` on the IEnergyProvider interface. EnergySystem implements `get_pool_state()` as a public method but it is **not** declared in the `building::IEnergyProvider` abstract class in `ForwardDependencyInterfaces.h`.

**Finding E5-SB-01 (Minor):** `get_pool_state()` is defined in canon `interfaces.yaml` as part of `IEnergyProvider` but is missing from the `building::IEnergyProvider` abstract class. This method exists on `EnergySystem` as a non-virtual public method, so downstream systems can call it directly, but the interface contract is incomplete. This should be added to `ForwardDependencyInterfaces.h` before any system needs to query pool state through the interface pointer.

**IContaminationSource:** The canon specifies `IContaminationSource` as an entity-level interface with `get_contamination_output()` and `get_contamination_type()`. The implementation takes a different approach: `IContaminationSource.h` defines `ContaminationSourceData` as a plain data struct, and `EnergySystem` exposes `get_contamination_sources(owner)` which returns a `vector<ContaminationSourceData>`. This is a pragmatic deviation -- instead of per-entity interface polymorphism, the system provides a batch query. This is architecturally sound for the pool model and avoids unnecessary per-entity virtual dispatch.

**ISimulatable:** Duck-typed rather than inherited, matching the documented pattern for avoiding diamond inheritance. `tick(float)` and `get_priority()` signatures match. Priority is 10, consistent with `systems.yaml`.

---

## 2. Data Flow Assessment

### Tick Pipeline (8 Phases)

The `tick()` method implements a well-structured pipeline:

```
Phase 0: clear_transition_events()
Phase 1: Age all nexuses (update_nexus_aging)
Phase 2: Update nexus outputs (update_all_nexus_outputs)
Phase 3: Coverage BFS recomputation if dirty (recalculate_coverage)
Phase 4: Pool calculation (calculate_pool)
Phase 5: Pool state machine (calculate_pool_state + detect_pool_state_transitions)
Phase 5a: Snapshot power states (snapshot_power_states)
Phase 5b: Energy distribution or rationing (distribute_energy / apply_rationing)
Phase 6: Update conduit active states (update_conduit_active_states)
Phase 7: Emit state change events (emit_state_change_events)
```

**Ordering Correctness:**
- Aging before output calculation: Correct. Age factor must be current before computing output.
- Output before coverage: Correct. Output values are independent of coverage.
- Coverage before pool: Correct. Pool aggregation needs current coverage to determine which consumers are in-range.
- Pool before distribution: Correct. Distribution decisions depend on surplus/deficit.
- Snapshot before distribution: Correct. Previous state captured for delta event emission.
- Events after distribution: Correct. Events reflect the final state.

**Finding E5-DF-01 (Info):** The pipeline phases are numbered with a "5b" split. Consider documenting these as phases 0-7 consistently in code comments (the current numbering in comments uses 0, 1, 2, 3, 4, 5, 5b, 6, 7 which maps to 9 logical steps).

### Data Dependency Chain

```
NexusTypeConfig --> EnergyProducerComponent --> Pool (generation side)
EnergyComponent.energy_required --> aggregate_consumption --> Pool (consumption side)
CoverageGrid --> aggregate_consumption (filters by coverage)
Pool --> Pool State Machine --> Distribution/Rationing --> EnergyComponent.is_powered
```

No circular dependencies detected. The one-way flow from producers through pool to consumers is clean.

---

## 3. Integration Contract Review

### Upstream Dependencies (systems EnergySystem reads from)

| System | Interface | Usage | Status |
|---|---|---|---|
| TerrainSystem | `ITerrainQueryable` | `is_buildable()`, `get_terrain_type()` | Forward-declared, used via pointer |
| BuildingSystem | Events | `on_building_constructed()`, `on_building_deconstructed()` | Event handler methods ready |

**Finding E5-IC-01 (Minor):** `on_building_constructed()` and `on_building_deconstructed()` accept `int32_t grid_x, grid_y` parameters and cast them to `uint32_t` inside the method body. The EnergySystem internally uses `uint32_t` for all coordinates. Negative grid coordinates would silently wrap to large unsigned values. The upstream caller (BuildingSystem) should guarantee non-negative coordinates, but defensive validation at the boundary would be prudent.

### Downstream Dependencies (systems that read from EnergySystem)

| Consumer | Interface | Methods | Status |
|---|---|---|---|
| BuildingSystem | `IEnergyProvider` | `is_powered()`, `is_powered_at()` | Implemented |
| ContaminationSystem | `get_contamination_sources()` | Batch query of active sources | Implemented |
| UISystem (future) | Various getters | Pool state, coverage count, events | Accessors present |

### Event Contracts

10 event structs defined in `EnergyEvents.h`:
- `EnergyStateChangedEvent`: Per-entity power state transitions
- `EnergyDeficitBeganEvent` / `EnergyDeficitEndedEvent`: Pool-level deficit transitions
- `GridCollapseBeganEvent` / `GridCollapseEndedEvent`: Pool-level collapse transitions
- `ConduitPlacedEvent` / `ConduitRemovedEvent`: Infrastructure change notifications
- `NexusPlacedEvent` / `NexusRemovedEvent`: Producer change notifications
- `NexusAgedEvent`: Aging threshold crossed

Events are buffered per-tick and cleared at tick start. This is correct for synchronous within-tick processing.

**Finding E5-IC-02 (Info):** Events are stored in internal vectors and accessed via const-reference getters. There is no event bus integration yet. When the EventBus system is implemented (Epic 10/12), these events will need to be dispatched through it. The current architecture supports this -- the getters can be replaced by EventBus publications without changing the event data structures.

---

## 4. Multiplayer Architecture

### Server Authority

Canon specifies `authority: server` and `per_player: true` for EnergySystem. The implementation correctly:
- Maintains separate pools per player (`m_pools[MAX_PLAYERS]`)
- Maintains separate entity lists per player (`m_nexus_ids[MAX_PLAYERS]`, etc.)
- Maintains separate coverage dirty flags per player
- Uses no client-side state or prediction

### Determinism Analysis

**Finding E5-MP-01 (High):** `update_nexus_aging()` uses `std::exp()` for the asymptotic decay curve:
```cpp
comp.age_factor = floor + (1.0f - floor) * std::exp(-NEXUS_AGING_DECAY_RATE * ticks);
```
`std::exp()` is not guaranteed to produce identical results across different compilers, platforms, or even optimization levels. In a lockstep multiplayer model, divergent floating-point results between server instances (or if computation is ever mirrored) would cause desync. The aging calculation is called every tick for every nexus.

**Recommended mitigation:** Use a fixed-point approximation or a lookup table for the aging curve. Alternatively, document that the server is the single authority and clients never compute aging independently.

**Finding E5-MP-02 (Medium):** `calculate_pool_state()` uses floating-point comparison for state thresholds:
```cpp
float surplus_f = static_cast<float>(pool.surplus);
if (surplus_f >= buffer_threshold) { return Healthy; }
```
While the surplus itself is `int32_t`, the comparison against threshold (which is `float(total_generated) * 0.10f`) introduces floating-point arithmetic. For identical integer inputs, this should produce identical results on the same platform, but cross-platform determinism is not guaranteed.

**Finding E5-MP-03 (Low):** `update_nexus_output()` computes output using float multiplication:
```cpp
float output = static_cast<float>(comp.base_output) * comp.efficiency * comp.age_factor;
```
Then casts back to `uint32_t`. The truncation behavior is well-defined, but the intermediate float value may vary by platform.

### Serialization

`EnergySerialization.h` provides:
- `EnergyComponent` full serialization: 13 bytes (1 version + 12 component via memcpy)
- Compact power-state bit packing: 1 bit per entity
- `EnergyPoolSyncMessage`: 16 bytes fixed-size

All multi-byte fields use little-endian encoding. Version byte is included for forward compatibility.

**Finding E5-MP-04 (Info):** `EnergyProducerComponent` (24 bytes) and `EnergyConduitComponent` (4 bytes) do not have serialization functions defined in `EnergySerialization.h`. These will be needed for full state snapshots and reconnection. The current serialization covers the most frequent sync case (power state changes) efficiently.

---

## 5. Performance Assessment

### Coverage BFS

The BFS algorithm in `recalculate_coverage()` is O(nexuses + conduits), not O(grid cells). This is correct for the stated target of `<10ms for 512x512 with 5,000 conduits`. The BFS uses:
- `std::queue<uint64_t>` for frontier
- `std::unordered_set<uint64_t>` for visited tracking
- 4-directional neighbor traversal

The `mark_coverage_radius()` inner function iterates a square of `(2*radius+1)^2` cells per nexus/conduit. For the default conduit radius of 3, this is 49 cells per conduit. For 5,000 conduits: ~245,000 cell writes. For a 512x512 grid (262,144 cells), this covers approximately the entire grid, which is within budget.

**Finding E5-PA-01 (Medium):** `clear_all_for_owner()` in CoverageGrid iterates the entire grid (up to 262,144 cells for 512x512). This is called once per dirty player per tick from `recalculate_coverage()`. For 4 players all dirty simultaneously, this is ~1M byte comparisons. The operation is cache-friendly (sequential scan) so it should be fast, but it could be optimized with a count-based early exit or a per-owner cell list if profiling shows it as a bottleneck.

**Finding E5-PA-02 (High):** `update_all_nexus_outputs()` performs an O(N*M) terrain bonus lookup:
```cpp
for (uint32_t eid : m_nexus_ids[owner]) {    // N nexuses
    // ...
    for (const auto& pos_pair : m_nexus_positions[owner]) {  // M positions
        if (pos_pair.second == eid) {  // linear scan for match
```
This iterates all nexus positions for every nexus to find its grid coordinates. For a player with 100 nexuses, this is 10,000 iterations per tick. This should be O(N) by maintaining a reverse lookup from entity_id to position, or by iterating `m_nexus_positions` directly (which already contains both position and entity_id).

**Finding E5-PA-03 (Low):** `get_coverage_count()` iterates the entire grid to count cells matching an owner. This is called from `calculate_pool()` indirectly? No, it is only a query accessor. It is not called in the tick pipeline. No concern.

### Pool Calculation

Pool calculation aggregates generation and consumption. `get_total_generation()` iterates `m_nexus_ids` per player (O(nexuses)). `aggregate_consumption()` iterates `m_consumer_positions` per player (O(consumers)). For the stated target of `<1ms for 10,000 consumers`, this should be well within budget since each iteration does O(1) map lookup + O(1) component access.

### Memory Footprint

| Data Structure | Size (256x256 map) |
|---|---|
| CoverageGrid | 64 KB |
| PerPlayerEnergyPool (x4) | 96 bytes |
| m_nexus_ids (vectors) | ~16 bytes per nexus |
| m_consumer_positions (hash maps) | ~48 bytes per consumer |
| m_conduit_positions (hash maps) | ~48 bytes per conduit |
| m_prev_powered (hash maps) | ~48 bytes per consumer |

Total overhead for 10,000 consumers + 5,000 conduits + 100 nexuses: ~64 KB grid + ~720 KB hash maps = ~784 KB. This is reasonable.

---

## 6. Coupling Analysis

### Direct Dependencies

| Dependency | Type | Coupling | Assessment |
|---|---|---|---|
| `entt::registry` | Library | Direct include | Acceptable -- project-wide ECS framework |
| `building::IEnergyProvider` | Interface | Inheritance | Clean -- abstract interface |
| `terrain::ITerrainQueryable` | Interface | Pointer (forward-declared) | Clean -- minimal coupling |
| `NexusTypeConfig` | Data | Direct include | Acceptable -- energy-internal configuration |

### Internal Cohesion

EnergySystem is a large class (1165 lines header, 1573 lines implementation) that manages nexuses, consumers, conduits, coverage, pools, distribution, rationing, events, and placement. This is within acceptable bounds for a top-level system orchestrator in the ECS pattern, but the class could benefit from internal decomposition if it continues to grow.

**Finding E5-CA-01 (Info):** EnergySystem handles 6 distinct responsibilities: entity registration, coverage BFS, pool calculation, distribution/rationing, event emission, and placement validation. While all are energy-domain concerns, the class could be split into subsystem helpers (e.g., `CoverageBFS`, `EnergyDistributor`, `PlacementValidator`) if complexity grows further in later epics. The current size is manageable.

### Cross-Namespace Dependencies

EnergySystem lives in `sims3000::energy` and depends on:
- `sims3000::building::IEnergyProvider` (upstream interface)
- `sims3000::terrain::ITerrainQueryable` (forward-declared)

No reverse dependencies (building depending on energy internals) detected. The dependency graph is acyclic.

---

## 7. Extensibility Review

### Post-MVP Nexus Types

`NexusType` enum defines 10 types with `NEXUS_TYPE_MVP_COUNT = 6`. The `is_mvp_nexus_type()` function and `get_aging_floor()` fallback handle non-MVP types gracefully. `NexusTypeConfig` can be extended with new entries for Hydro (6), Geothermal (7), Fusion (8), and MicrowaveReceiver (9).

**Finding E5-EX-01 (Minor):** Terrain requirement enforcement for post-MVP nexuses is stubbed:
```cpp
// 5. Type-specific terrain requirements
// Hydro and Geothermal are stubbed as always valid for MVP
(void)type;
```
When Hydro (requires water proximity) and Geothermal (requires ember crust) are activated, `validate_nexus_placement()` will need to query terrain type at the placement position. The `m_terrain` pointer and `get_terrain_type()` interface are already available, so the extension point is well-prepared.

### Weather System Integration

Wind and Solar nexuses apply a hardcoded `WEATHER_STUB_FACTOR = 0.75f` for variable output. This is clearly labeled as a stub. Future weather system integration should replace this with a runtime query, likely through an `IWeatherProvider` interface injected similarly to `ITerrainQueryable`.

### Territory/Ownership Integration

`can_extend_coverage_to()` always returns `true` with a detailed comment about future behavior:
```cpp
// Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
// Returns false if tile_owner belongs to a different player
```
The BFS already calls this method at the right point in the traversal. Activating ownership enforcement requires only implementing the body of this method with a territory system query.

### Conduit Levels

`EnergyConduitComponent` includes `conduit_level` (uint8_t), currently always set to 1. This field is present but unused, providing a clean extension point for upgraded conduit types with larger coverage radii.

---

## 8. Technical Debt Inventory

### Priority 1 (Should fix before multiplayer testing)

| ID | Description | Location | Ticket |
|---|---|---|---|
| E5-MP-01 | `std::exp()` in aging may cause cross-platform desync | `EnergySystem.cpp:217` | Needs new ticket |
| E5-PA-02 | O(N*M) terrain bonus lookup in `update_all_nexus_outputs()` | `EnergySystem.cpp:267-278` | Needs new ticket |

### Priority 2 (Should fix before Epic 10 integration)

| ID | Description | Location | Ticket |
|---|---|---|---|
| E5-SB-01 | `get_pool_state()` missing from `IEnergyProvider` abstract class | `ForwardDependencyInterfaces.h` | 4-019 update |
| E5-IC-01 | `int32_t` to `uint32_t` coordinate cast in building handlers | `EnergySystem.cpp:1480-1495` | Defensive check |

### Priority 3 (Documentation / cleanup)

| ID | Description | Location | Ticket |
|---|---|---|---|
| E5-TD-01 | PerPlayerEnergyPool doc comment says "32 bytes" but `static_assert` confirms 24 bytes | `PerPlayerEnergyPool.h:27` | Doc fix |
| E5-TD-02 | EnergyEnums.h comment says "first 6 types (Carbon through Hydro)" but Hydro is index 6, excluded from MVP count | `EnergyEnums.h:24-25` | Doc fix |
| E5-TD-03 | `INVALID_ENTITY_ID = UINT32_MAX` used by `place_nexus()` return, but `place_conduit()` returns same sentinel; both documented as "returns 0 on failure" in header comments | `EnergySystem.h:282, 302` | Doc fix |

### Stubs Requiring Future Activation

| Stub | Description | Activation Epic |
|---|---|---|
| Ownership validation | `validate_nexus_placement()` and `validate_conduit_placement()` ownership checks always pass | Epic with territory system |
| Existing structure check | No check for existing structure at placement position | Epic 4 BuildingGrid integration |
| Terrain requirements | Hydro/Geothermal terrain requirements stubbed | Post-MVP nexus activation |
| Weather factor | `WEATHER_STUB_FACTOR = 0.75f` hardcoded for Wind/Solar | Weather system epic |
| Cost deduction | `DEFAULT_CONDUIT_COST = 10` defined but not deducted | Epic 11 (EconomySystem) |
| Coverage ownership | `can_extend_coverage_to()` always returns true | Territory system epic |

---

## 9. Cross-Epic Dependency Map

```
Epic 3 (Terrain)
  |
  +--> [ITerrainQueryable] --> EnergySystem (placement validation, terrain bonus)

Epic 4 (Zoning & Building)
  |
  +--> [building::IEnergyProvider] <-- EnergySystem (inherits)
  |
  +--> BuildingConstructedEvent --> EnergySystem.on_building_constructed()
  +--> BuildingDeconstructedEvent --> EnergySystem.on_building_deconstructed()

Epic 5 (Energy) -- THIS EPIC
  |
  +--> [IContaminationSource data] --> Epic 10 ContaminationSystem
  |
  +--> [EnergyStateChangedEvent] --> Epic 4 BuildingSystem (power state affects building state)
  |
  +--> [EnergyPoolState queries] --> Epic 12 UISystem (power overlay, status display)

Epic 6 (Water)
  Parallel structure to Epic 5; no direct dependency.
  FluidSystem will mirror IFluidProvider pattern.

Epic 7 (Transport)
  No direct dependency with EnergySystem.

Epic 10 (Simulation)
  |
  +--> ContaminationSystem reads get_contamination_sources()
  |
  +--> PopulationSystem reads pool state (affects habitability)

Epic 11 (Financial)
  |
  +--> EconomySystem will provide ICreditProvider for cost deduction

Epic 12 (UI)
  |
  +--> UISystem reads pool state, coverage, events for display
```

---

## 10. Recommendations

### Must Address (before next milestone)

1. **Fix O(N*M) terrain bonus lookup** (E5-PA-02): Iterate `m_nexus_positions` directly instead of doing a nested loop through positions for each nexus ID. This converts an O(N*M) operation to O(N).

2. **Evaluate `std::exp()` determinism risk** (E5-MP-01): If multiplayer will use lockstep or any form of client-side verification, replace `std::exp()` with a deterministic approximation. If the server is the sole computation authority and clients never compute aging, document this decision explicitly and add a comment to the aging function.

### Should Address (before Epic 10 integration)

3. **Add `get_pool_state()` to `IEnergyProvider`** (E5-SB-01): The canon interface specifies it. Systems querying energy through the interface pointer should be able to access pool state without downcasting.

4. **Fix documentation inconsistencies** (E5-TD-01, E5-TD-02, E5-TD-03): The PerPlayerEnergyPool size comment, NexusType MVP range comment, and place_nexus/place_conduit return value documentation should be corrected to match the actual implementation.

### Consider (for future robustness)

5. **Add defensive coordinate validation** in `on_building_constructed()` / `on_building_deconstructed()` to reject negative `int32_t` grid coordinates before casting to `uint32_t`.

6. **Add serialization for `EnergyProducerComponent` and `EnergyConduitComponent`** (E5-MP-04) before full state snapshot implementation.

7. **Profile `clear_all_for_owner()`** on 512x512 maps to determine if optimization is needed.

---

## 11. Risk Register

| Risk | Severity | Probability | Mitigation |
|---|---|---|---|
| `std::exp()` floating-point divergence causes multiplayer desync | High | Medium | Use fixed-point aging or document server-only authority |
| O(N*M) terrain bonus scales poorly with many nexuses | Medium | Low | Fix to O(N) before large map testing |
| Stubs forgotten and shipped without real validation | Medium | Medium | Track stub activation in a per-epic checklist; CI test for stub markers |
| CoverageGrid full-grid clear on 512x512 causes tick budget overrun | Low | Low | Profile; optimize only if measured as bottleneck |
| Pool state float comparison gives inconsistent results cross-platform | Medium | Low | Switch to integer-only threshold comparison |
| Event buffers grow unbounded during rapid state oscillation | Low | Low | Events cleared each tick; bounded by entity count |

---

*Review generated by Systems Architect Agent. Findings should be triaged by the project lead and tracked as tickets where appropriate.*
