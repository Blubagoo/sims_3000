# Epic 6: Water Infrastructure (FluidSystem) -- Systems Architect Review

**Reviewer:** Systems Architect Agent
**Date:** 2026-02-08
**Epic:** 6 -- Water Infrastructure / FluidSystem
**Canon Version:** 0.17.0
**Files Reviewed:** 24 (15 headers, 5 source files, 4 test files)

---

## Summary Verdict

**APPROVED WITH FINDINGS**

Epic 6 delivers a well-structured fluid infrastructure system that closely mirrors the established EnergySystem (Epic 5) patterns while introducing justified domain-specific deviations: reservoir buffering with asymmetric fill/drain rates, all-or-nothing distribution (CCR-002, no priority rationing), water proximity efficiency curves for extractors, and energy dependency for extractor operation. The implementation correctly inherits `building::IFluidProvider`, follows the dense grid exception for `FluidCoverageGrid`, and cleanly factors BFS, placement validation, and serialization into separate compilation units. The 10-phase tick pipeline is well-ordered, deterministic, and consistent with the energy system's approach.

The review identifies several findings: an O(N*M) position-to-entity reverse lookup performance concern (inherited from the same pattern flagged in Epic 5), a `PerPlayerFluidPool::clear()` method that resets `previous_state` which may break state transition detection, the `pack_position` / `pack_pos` function duplication across FluidSystem and FluidCoverageBFS, and the need for `FluidProducerComponent` / `FluidConduitComponent` serialization before full state snapshot support. None of these block continued development.

---

## 1. Architecture Pattern Consistency

### Structural Alignment with EnergySystem

The FluidSystem consciously mirrors EnergySystem's architecture, which is the correct decision for a parallel infrastructure system. The mapping is:

| EnergySystem Concept | FluidSystem Equivalent | Status |
|---|---|---|
| `EnergySystem` class | `FluidSystem` class | Implemented |
| `EnergyComponent` (16 bytes) | `FluidComponent` (12 bytes) | Implemented, smaller (no priority field) |
| `EnergyProducerComponent` (24 bytes) | `FluidProducerComponent` (12 bytes) | Implemented, smaller (no aging) |
| `EnergyConduitComponent` (4 bytes) | `FluidConduitComponent` (4 bytes) | Identical structure |
| N/A | `FluidReservoirComponent` (16 bytes) | New: fluid-specific storage |
| `PerPlayerEnergyPool` (24 bytes) | `PerPlayerFluidPool` (40 bytes) | Larger: reservoir tracking fields |
| `CoverageGrid` | `FluidCoverageGrid` | Separate per CCR-009 |
| `EnergySerialization` | `FluidSerialization` | Parallel implementation |
| BFS inline in EnergySystem | `FluidCoverageBFS` separate file | Better: factored out |
| Validation inline in EnergySystem | `FluidPlacementValidation` separate file | Better: factored out |

### Justified Deviations

1. **No priority field on FluidComponent (CCR-002):** EnergyComponent has a `priority` field for 4-tier rationing. FluidComponent omits this because fluid uses all-or-nothing distribution. This is a correct domain-driven difference, well-documented with CCR references.

2. **No aging on FluidProducerComponent:** Energy nexuses have `ticks_since_built`, `age_factor`, `efficiency` fields. Fluid extractors do not age -- their efficiency is driven by water proximity distance instead. This simplification is appropriate: extractors are simpler facilities than nexuses.

3. **FluidReservoirComponent is new:** Energy has no buffering mechanism. Fluid introduces reservoir entities with asymmetric fill/drain rates (CCR-005: fill=50, drain=100). This is a well-justified gameplay difference -- reservoirs provide the "grace period" that energy implements via a built-in timer.

4. **BFS factored into separate compilation unit:** EnergySystem has BFS inlined in the .cpp. FluidSystem extracts BFS into `FluidCoverageBFS.h/.cpp` with a `BFSContext` struct. This is an improvement -- it enables parallel development (ticket 6-009 skeleton vs 6-010 BFS) and cleaner testing.

5. **Placement validation factored out:** Similarly, `FluidPlacementValidation.h/.cpp` is a dedicated compilation unit. EnergySystem keeps validation methods on the main class. The fluid approach is cleaner for large codebases.

---

## 2. Component Design Review

### FluidComponent (12 bytes)

```
fluid_required:  uint32_t  (4 bytes)
fluid_received:  uint32_t  (4 bytes)
has_fluid:       bool      (1 byte)
_padding:        uint8_t[3](3 bytes)
Total: 12 bytes -- verified by static_assert
```

Clean POD layout. The explicit `_padding` and `static_assert` for size and trivial copyability are correct practices. Smaller than EnergyComponent (16 bytes) by omitting the `priority` and `_padding2` fields, which is appropriate given the no-rationing design.

### FluidProducerComponent (12 bytes)

```
base_output:            uint32_t (4 bytes)
current_output:         uint32_t (4 bytes)
max_water_distance:     uint8_t  (1 byte)
current_water_distance: uint8_t  (1 byte)
is_operational:         bool     (1 byte)
producer_type:          uint8_t  (1 byte)
Total: 12 bytes -- verified by static_assert
```

Natural alignment, no wasted padding. The `producer_type` field using `uint8_t` instead of the `FluidProducerType` enum directly is acceptable for size control, though it requires manual casting at usage sites.

### FluidReservoirComponent (16 bytes)

```
capacity:       uint32_t  (4 bytes)
current_level:  uint32_t  (4 bytes)
fill_rate:      uint16_t  (2 bytes)
drain_rate:     uint16_t  (2 bytes)
is_active:      bool      (1 byte)
reservoir_type: uint8_t   (1 byte)
_padding:       uint8_t[2](2 bytes)
Total: 16 bytes -- verified by static_assert
```

Good use of `uint16_t` for rates to save space (max 65,535 units/tick is more than sufficient). The asymmetric default rates (fill=50, drain=100) per CCR-005 are correctly documented and implemented.

### FluidConduitComponent (4 bytes)

Identical structure to `EnergyConduitComponent`. This is intentional and correct -- conduits serve the same role in both systems.

**Finding F6-CD-01 (Minor):** The default `coverage_radius` in `FluidConduitComponent` is 2, but `place_conduit()` in FluidSystem overrides it to 3. This mismatch between the component default and the placement-time value could cause confusion if components are created outside `place_conduit()`. The default in the struct should match the intended placement value, or the placement should not override it.

### PerPlayerFluidPool (40 bytes)

Well-structured with all necessary aggregate fields. The `previous_state` field for transition detection is a clean approach.

**Finding F6-CD-02 (Medium):** The `clear()` method resets `previous_state` to `Healthy`:
```cpp
void clear() {
    // ...
    state = FluidPoolState::Healthy;
    previous_state = FluidPoolState::Healthy;
    // ...
}
```
If `clear()` is called at the start of each tick (as the doc comment suggests), it would wipe the previous state before `detect_pool_state_transitions()` can compare it. However, reviewing the actual tick pipeline, `clear()` is NOT called in `tick()` -- the pool fields are updated incrementally by `update_extractor_outputs()`, `update_reservoir_totals()`, and the consumption aggregation phase. The `calculate_pool()` method correctly saves `pool.previous_state = pool.state` before recalculating. So the `clear()` method is a setup-time utility, not a per-tick operation, and the code is correct in practice. The doc comment on `clear()` saying "Called at the start of each tick" is misleading and should be updated.

---

## 3. Separation of Concerns

### BFS (FluidCoverageBFS)

The extraction of BFS into a standalone module with `BFSContext` is well-designed:

- `BFSContext` aggregates all dependencies without exposing FluidSystem internals
- `recalculate_coverage()` is a free function, not a method on FluidSystem
- `mark_coverage_radius()` and `can_extend_coverage_to()` are reusable helpers
- `pack_pos()`, `unpack_x()`, `unpack_y()` are provided as inline helpers

**Finding F6-SC-01 (Low):** Both `FluidCoverageBFS.h` and `FluidSystem.h` define position packing functions with slightly different names: `pack_pos()` / `unpack_x()` / `unpack_y()` in BFS vs `pack_position()` / `unpack_x()` / `unpack_y()` as static methods on FluidSystem. The implementations are identical. Consider consolidating to a single set of functions (the BFS header's inline versions) to avoid duplication.

### Placement Validation (FluidPlacementValidation)

Clean separation with dedicated result types (`ExtractorPlacementResult`, `ReservoirPlacementResult`). The `calculate_water_factor()` function is appropriately placed here since it is a stateless calculation used by both validation and runtime output computation.

**Finding F6-SC-02 (Info):** There is a forward declaration workaround in `FluidSystem.cpp` line 32:
```cpp
namespace sims3000 { namespace fluid { float calculate_water_factor(uint8_t distance); } }
```
This is documented as necessary because `FluidPlacementValidation.h` forward-declares `entt::registry` as a class, which conflicts with EnTT's type alias when included after `FluidSystem.h`. This is a pragmatic workaround but creates a fragile include-order dependency. A cleaner fix would be to have `FluidPlacementValidation.h` include `<entt/entity/fwd.hpp>` (which it already does) and ensure the forward declaration uses the correct EnTT forward-declaration machinery.

### Serialization (FluidSerialization)

Proper separation. Three serialization formats:
1. Full `FluidComponent` (13 bytes: 1 version + 12 memcpy) -- correct for initial sync
2. Bit-packed fluid states (4 + ceil(N/8) bytes) -- efficient for per-tick delta
3. `FluidPoolSyncMessage` (22 bytes) -- compact pool state snapshot

The little-endian helpers are clean and correctly implemented. Version byte provides forward compatibility.

**Finding F6-SR-01 (Info):** Similar to Epic 5's finding E5-MP-04, `FluidProducerComponent` and `FluidConduitComponent` lack serialization functions. These will be needed for full state snapshots and reconnection support.

---

## 4. Thread Safety

FluidSystem is designed for single-threaded, server-authoritative operation. All state is mutable through non-const methods and there are no mutexes, atomics, or other synchronization primitives. This is correct for the current architecture where `tick()` is called sequentially by the simulation loop.

No shared state concerns exist within the current single-threaded model. The `entt::registry*` is a non-owning pointer set once during initialization, and all registry access happens within the tick pipeline.

If future parallelism is introduced (e.g., per-player parallel ticks), the per-player arrays (`m_pools[MAX_PLAYERS]`, `m_extractor_ids[MAX_PLAYERS]`, etc.) provide a natural partition boundary. However, the shared `m_coverage_grid` would need partitioning or locking since multiple players' coverage can overlap on the same grid.

---

## 5. Memory Layout

### Dense Grid (FluidCoverageGrid)

Correctly implements the canonical dense grid exception pattern:
- Row-major `std::vector<uint8_t>` storage
- 1 byte per cell, O(1) coordinate lookup
- Memory budget: 16KB (128x128), 64KB (256x256), 256KB (512x512)
- Separate from EnergyCoverageGrid per CCR-009

The `clear_all_for_owner()` method does a linear scan of the entire grid (O(W*H)). This was flagged as E5-PA-01 in the energy review. The same concern applies here. For a 512x512 grid, this is 262,144 byte comparisons -- cache-friendly and fast, but worth monitoring.

### Hash Map Usage

The system uses `std::unordered_map<uint64_t, uint32_t>` for spatial lookups (extractor, reservoir, conduit, and consumer positions per player). This provides O(1) average-case position-to-entity lookup.

Memory overhead per entry: approximately 48-64 bytes (key + value + hash table overhead). For 5,000 conduits + 100 extractors + 50 reservoirs + 10,000 consumers per player:
- Spatial maps: ~15,150 entries * ~56 bytes = ~830 KB per player, ~3.3 MB for 4 players
- Plus entity ID vectors: ~15,150 entries * 4 bytes = ~60 KB per player

Total estimated memory: ~64KB grid + ~3.5MB hash maps = ~3.6MB. This is within acceptable bounds but higher than necessary -- the hash maps could be replaced with a flat grid-indexed structure for position lookups if memory pressure becomes a concern.

### Event Buffers

12 separate event type vectors. Events are cleared each tick and bounded by the number of entities that can change state in a single tick. No unbounded growth risk.

**Finding F6-ML-01 (Low):** The `m_prev_has_fluid[MAX_PLAYERS]` array uses `std::unordered_map<uint32_t, bool>` for snapshot storage. This is rebuilt every tick in `snapshot_fluid_states()`. For 10,000 consumers, this allocates and deallocates ~10,000 hash map entries per tick. A `std::vector<std::pair<uint32_t, bool>>` with reserve, or a flat array indexed by a consumer index, would be more cache-friendly and avoid per-tick allocation churn.

---

## 6. API Design

### Public Interface

The FluidSystem API is clean and consistent:
- Registration methods follow the `register_X()` / `unregister_X()` pattern
- Position registration follows `register_X_position()` pattern
- Placement follows `place_X()` returning entity ID or `INVALID_ENTITY_ID`
- Queries follow `get_X()` / `is_X()` pattern
- Event accessors return `const std::vector<T>&`
- Building integration via `on_building_constructed()` / `on_building_deconstructed()`

### IFluidProvider Implementation

`FluidSystem` correctly inherits `building::IFluidProvider` and implements both `has_fluid()` and `has_fluid_at()`. The implementations are defensive: null-registry and invalid-entity checks return safe defaults.

The `has_fluid_at()` method correctly converts player_id (0-based) to overseer_id (1-based) for grid lookup, matching the energy system's convention.

### Registration Symmetry

All registration methods have corresponding unregistration methods. Bounds checking on `owner >= MAX_PLAYERS` is consistent across all registration methods. Coverage dirty flags are correctly set when infrastructure entities are registered or unregistered.

**Finding F6-API-01 (Minor):** `unregister_consumer()` does not set `m_coverage_dirty` because consumer changes do not affect the BFS coverage network (only the consumption aggregation). This is correct behavior but may be surprising to callers who expect symmetry with `unregister_extractor()` which does set the dirty flag. A brief comment explaining this would be helpful.

---

## 7. Error Handling

### Null Registry Protection

All methods that access `m_registry` check for nullptr first:
- `has_fluid()` returns `false`
- `update_extractor_outputs()` returns early
- `update_reservoir_totals()` returns early
- `distribute_fluid()` returns early
- Placement methods return `INVALID_ENTITY_ID`

### Bounds Checking

All public methods validate `owner >= MAX_PLAYERS` and return safe defaults:
- `get_pool()` returns `m_pools[0]` as fallback (documented)
- `get_pool_state()` returns `FluidPoolState::Healthy`
- Registration methods silently return
- Count accessors return 0

Grid coordinates are validated against `m_map_width` / `m_map_height` in all placement and query methods. The `FluidCoverageGrid` additionally validates bounds internally, providing defense-in-depth.

### Building Event Handlers

`on_building_constructed()` and `on_building_deconstructed()` correctly validate:
- `owner >= MAX_PLAYERS`
- Null registry
- Negative grid coordinates (`grid_x < 0 || grid_y < 0`)
- Out-of-bounds coordinates
- Entity validity

This addresses the finding E5-IC-01 from the Epic 5 review -- the fluid system includes the defensive coordinate validation that was recommended for the energy system.

---

## 8. Serialization Assessment

### FluidComponent Serialization

The 13-byte format (1 version + 12 memcpy) is correct for trivially copyable components. Version checking on deserialization provides forward compatibility. Error handling throws `std::runtime_error` for buffer underflow and version mismatch.

**Finding F6-SR-02 (Medium):** The `serialize_fluid_component()` function uses `memcpy` for the 12-byte component body, which assumes the same struct layout on serializer and deserializer. This is safe within a single platform but could be problematic for cross-platform serialization if struct padding differs. Since the components have `static_assert` for exact size and are trivially copyable, this risk is low but should be documented. The `FluidPoolSyncMessage` serialization correctly uses field-by-field little-endian encoding, avoiding this issue.

### Bit-Packed Fluid States

The 1-bit-per-entity packing is efficient: 10,000 entities compress to 1,254 bytes (4 count + 1,250 packed). Correct LSB-first bit ordering. Proper count validation on deserialization prevents buffer overread.

### FluidPoolSyncMessage

22 bytes is compact and sufficient:
```
owner:              1 byte
state:              1 byte
total_generated:    4 bytes LE
total_consumed:     4 bytes LE
surplus:            4 bytes LE (signed)
reservoir_stored:   4 bytes LE
reservoir_capacity: 4 bytes LE
```

Field-by-field serialization with little-endian encoding is the correct approach for network messages. The fixed size (`FLUID_POOL_SYNC_MESSAGE_SIZE = 22`) is declared as a constant.

---

## 9. Test Coverage Assessment

### test_fluid_system.cpp (42 tests)

Comprehensive skeleton tests covering:
- Construction with various map sizes (5 tests)
- Priority value verification
- Registry wiring (2 tests)
- Pool query defaults (3 tests)
- `has_fluid` edge cases (4 tests)
- `has_fluid_at` edge cases (2 tests)
- Extractor placement (5 tests including out-of-bounds and invalid owner)
- Conduit placement (3 tests)
- Conduit removal (5 tests including error cases)
- Coverage dirty flags (4 tests)
- Registration/unregistration (5 tests)
- Per-player isolation (1 test)
- Reservoir placement (2 tests)
- Tick lifecycle (2 tests)
- Energy provider wiring (1 test)

Good edge case coverage for error paths (null registry, invalid entity, wrong component, out-of-bounds owner).

### test_fluid_integration.cpp (14 tests)

End-to-end pipeline tests:
- Extractor near/far water generation
- Unpowered extractor
- Consumer in/outside coverage
- Deficit all-or-nothing distribution
- Reservoir fill/drain
- Proportional drain across reservoirs
- Conduit coverage extension and contraction
- Ownership boundary stub
- State change event emission
- Performance with 1000 consumers

Good coverage of the full tick pipeline. The performance test is informational (no strict timing assertion) which is appropriate for a CI environment.

### test_fluid_distribution.cpp (12 tests)

Focused distribution tests:
- Surplus distribution
- Deficit distribution
- Outside-coverage rejection
- No-rationing verification
- Reservoir buffering integration
- Edge cases (no consumers, no registry, invalid owner)
- Mixed coverage states
- Surplus == 0 boundary
- Multi-player isolation

The multi-player isolation test is particularly valuable, verifying that player 0's surplus does not affect player 1's deficit.

### test_pool_state_machine.cpp (13 tests)

State machine and reservoir tests:
- Healthy -> Marginal transition
- Marginal -> Deficit transition
- Collapse when reservoirs empty
- Fill during surplus (rate-limited)
- Drain during deficit
- Proportional drain
- Asymmetric rates verification
- Deficit -> Healthy recovery
- Transition event emission (deficit began/ended, collapse began/ended)
- Previous state tracking

**Finding F6-TC-01 (Minor):** No tests exist for the serialization module (`FluidSerialization`). Round-trip tests for `serialize_fluid_component` / `deserialize_fluid_component`, `pack_fluid_states` / `unpack_fluid_states`, and `serialize_pool_sync` / `deserialize_pool_sync` should be added to verify correctness of the bit packing and little-endian encoding.

**Finding F6-TC-02 (Low):** No tests exercise `FluidPlacementValidation` directly (the standalone `validate_extractor_placement()` and `validate_reservoir_placement()` functions). The integration tests exercise placement through `FluidSystem::place_extractor()` which does not delegate to the standalone validation functions. Direct unit tests for `calculate_water_factor()` and the validation functions with mock terrain would improve coverage.

---

## 10. Performance Analysis

### Tick Pipeline (10 Phases)

```
Phase 0: clear_transition_events()         -- O(1) amortized (vector clear)
Phase 1: (reserved)
Phase 2: update_extractor_outputs()        -- O(E) extractors per player
Phase 3: update_reservoir_totals()         -- O(R) reservoirs per player
Phase 4: recalculate_coverage() if dirty   -- O(E+R+C) BFS traversal
Phase 5: aggregate_consumption()           -- O(N) consumers per player
Phase 6: calculate_pool + buffering        -- O(R) reservoirs per player
Phase 7a: snapshot_fluid_states()          -- O(N) consumers per player
Phase 7b: distribute_fluid()              -- O(N) consumers per player
Phase 8: update_conduit_active_states()    -- O(C) conduits per player
Phase 9: emit_state_change_events()        -- O(N) consumers per player
```

Overall per-player complexity: O(E + R + C + N) where E=extractors, R=reservoirs, C=conduits, N=consumers. This is optimal -- each entity is visited a constant number of times.

### Critical Performance Finding

**Finding F6-PA-01 (High):** The same O(N*M) reverse lookup pattern flagged in Epic 5 (E5-PA-02) exists in FluidSystem at two locations:

1. `update_extractor_outputs()` (FluidSystem.cpp lines 911-924):
```cpp
for (uint32_t eid : m_extractor_ids[owner]) {    // N extractors
    // ...
    for (const auto& pos_pair : m_extractor_positions[owner]) {  // M positions
        if (pos_pair.second == eid) {  // linear scan for entity
```

2. `distribute_fluid()` and `aggregate_consumption()` (FluidSystem.cpp lines 126-137, 1314-1323):
```cpp
for (uint32_t eid : m_consumer_ids[owner]) {     // N consumers
    // ...
    for (const auto& pos_pair : m_consumer_positions[owner]) {  // M positions
        if (pos_pair.second == eid) {  // linear scan for entity
```

For a player with 10,000 consumers, this is 100,000,000 iterations per tick in Phase 5 alone. This will exceed the tick budget on large maps.

**Recommended fix:** Add reverse lookup maps (`std::unordered_map<uint32_t, uint64_t>`) mapping entity_id to packed position. This converts O(N*M) to O(N) with O(1) per lookup.

### BFS Performance

The BFS in `FluidCoverageBFS.cpp` uses:
- `std::queue<uint64_t>` for frontier
- `std::unordered_set<uint64_t>` for visited tracking
- 4-directional neighbor traversal

For the default conduit radius of 2, each conduit marks a 5x5 = 25 cell area. For 5,000 conduits: ~125,000 cell writes. The BFS itself is O(conduits), not O(grid cells). Performance target (<10ms for 512x512 with 5,000 conduits) should be achievable.

**Finding F6-PA-02 (Low):** The BFS `visited` set uses `std::unordered_set<uint64_t>` with per-lookup allocation overhead. For high-conduit-count scenarios, a flat boolean grid (`std::vector<bool>` sized to map dimensions) would be faster and more cache-friendly, at the cost of W*H bits of memory.

### Coverage Grid Operations

`clear_all_for_owner()` iterates the full grid (up to 262K cells for 512x512). Called once per dirty player per tick. Sequential scan is cache-friendly. Acceptable for up to 4 players dirty simultaneously (~1M comparisons).

`get_coverage_count()` also iterates the full grid. Called from `FluidSystem::get_coverage_count()` accessor, which does not appear to be called in the tick pipeline. No concern.

---

## 11. Technical Debt Inventory

### Priority 1 (Should fix before large-map testing)

| ID | Description | Location | Ticket |
|---|---|---|---|
| F6-PA-01 | O(N*M) reverse position lookup in update_extractor_outputs(), aggregate_consumption(), and distribute_fluid() | FluidSystem.cpp:911-924, 126-137, 1314-1323 | Needs new ticket |

### Priority 2 (Should fix before multiplayer integration)

| ID | Description | Location | Ticket |
|---|---|---|---|
| F6-SR-01 | FluidProducerComponent and FluidConduitComponent lack serialization | FluidSerialization.h | Needs new ticket |
| F6-SR-02 | memcpy serialization for FluidComponent assumes same struct layout cross-platform | FluidSerialization.cpp:60 | Document or convert to field-by-field |
| F6-TC-01 | No serialization round-trip tests | tests/fluid/ | Needs new test file |

### Priority 3 (Cleanup / documentation)

| ID | Description | Location | Ticket |
|---|---|---|---|
| F6-CD-01 | FluidConduitComponent default coverage_radius (2) != place_conduit override (3) | FluidConduitComponent.h:41, FluidSystem.cpp:468 | Sync values |
| F6-CD-02 | PerPlayerFluidPool::clear() doc comment is misleading | PerPlayerFluidPool.h:75 | Doc fix |
| F6-SC-01 | Duplicate pack_pos/pack_position functions | FluidCoverageBFS.h, FluidSystem.h | Consolidate |
| F6-SC-02 | Forward declaration workaround for calculate_water_factor | FluidSystem.cpp:32 | Clean up include chain |
| F6-TC-02 | No direct unit tests for FluidPlacementValidation functions | tests/fluid/ | Add test file |
| F6-API-01 | unregister_consumer() not setting coverage_dirty lacks explanatory comment | FluidSystem.cpp:276-285 | Add comment |
| F6-ML-01 | m_prev_has_fluid uses per-tick hash map rebuild | FluidSystem.h:834 | Consider vector-based snapshot |

### Stubs Requiring Future Activation

| Stub | Description | Activation Epic |
|---|---|---|
| Ownership validation | `can_extend_coverage_to()` always returns true | Territory system epic |
| Existing structure check | No check for existing structure at placement position | Epic 4 BuildingGrid integration |
| Cost deduction | `place_conduit()` comment: "Cost deduction stubbed: not yet deducted" | Epic 11 (EconomySystem) |
| Coverage ownership boundary | `can_extend_coverage_to()` stub in FluidCoverageBFS.cpp | Territory system epic |
| Extractor terrain validation | `validate_extractor_placement()` always passes structure check | Future validation epic |

---

## 12. Cross-Epic Dependency Map

```
Epic 3 (Terrain)
  |
  +--> [ITerrainQueryable] --> FluidSystem (placement validation, water distance)

Epic 4 (Zoning & Building)
  |
  +--> [building::IFluidProvider] <-- FluidSystem (inherits)
  |
  +--> BuildingConstructedEvent --> FluidSystem.on_building_constructed()
  +--> BuildingDeconstructedEvent --> FluidSystem.on_building_deconstructed()

Epic 5 (Energy)
  |
  +--> [building::IEnergyProvider] --> FluidSystem.set_energy_provider()
  |        (extractors depend on power to operate)
  |
  +--> [EnergyComponent] --> FluidSystem.place_extractor()
  |        (attaches EnergyComponent to extractor entities)

Epic 6 (Water) -- THIS EPIC
  |
  +--> [FluidStateChangedEvent] --> Epic 4 BuildingSystem (fluid state affects building)
  |
  +--> [FluidPoolState queries] --> Epic 12 UISystem (fluid overlay, status display)

Epic 7 (Transport)
  No direct dependency with FluidSystem.

Epic 10 (Simulation)
  |
  +--> PopulationSystem reads pool state (affects habitability)

Epic 11 (Financial)
  |
  +--> EconomySystem will provide ICreditProvider for cost deduction
```

The energy-fluid dependency is the most notable cross-epic coupling: FluidSystem imports `EnergyComponent` and `building::IEnergyProvider` to make extractors energy-dependent. This is architecturally sound -- the dependency flows one direction (fluid depends on energy), not bidirectionally.

---

## 13. Multiplayer Architecture

### Server Authority

FluidSystem maintains per-player isolation through:
- `m_pools[MAX_PLAYERS]` separate pool arrays
- `m_extractor_ids[MAX_PLAYERS]` separate entity lists
- `m_reservoir_ids[MAX_PLAYERS]` separate entity lists
- `m_consumer_ids[MAX_PLAYERS]` separate entity lists
- `m_coverage_dirty[MAX_PLAYERS]` separate dirty flags
- All spatial lookup maps indexed by player

No client-side state or prediction. Server-authoritative design is correct.

### Determinism

**Finding F6-MP-01 (Medium):** `calculate_pool_state()` uses floating-point arithmetic for threshold comparison:
```cpp
uint32_t buffer_threshold = static_cast<uint32_t>(
    static_cast<float>(pool.available) * 0.10f);
```
This inherits the same concern as E5-MP-02 from the energy review. For identical integer inputs, this should produce identical results on the same platform, but cross-platform determinism is not guaranteed. Consider using integer-only arithmetic: `buffer_threshold = pool.available / 10`.

**Finding F6-MP-02 (Low):** `calculate_water_factor()` returns hardcoded float constants (1.0f, 0.9f, 0.7f, etc.), and `update_extractor_outputs()` uses float multiplication:
```cpp
float output = static_cast<float>(prod->base_output) * water_factor * (powered ? 1.0f : 0.0f);
prod->current_output = static_cast<uint32_t>(output);
```
The water factor values are exact in IEEE 754, and the multiplication with small integers should be deterministic across platforms. However, documenting this assumption would be prudent.

---

## 14. Risk Register

| Risk | Severity | Probability | Mitigation |
|---|---|---|---|
| O(N*M) position lookup causes tick budget overrun with many consumers | High | High | Add reverse lookup map (entity_id -> packed position) |
| Serialization memcpy assumes identical struct layout cross-platform | Medium | Low | Convert to field-by-field LE for cross-platform; current platform-only use is safe |
| Pool state float threshold gives inconsistent results cross-platform | Medium | Low | Switch to integer-only threshold (available / 10) |
| Stubs forgotten and shipped without real validation | Medium | Medium | Track stub activation in per-epic checklist |
| FluidConduitComponent default radius != placement radius causes bugs | Low | Medium | Sync the default value to 3 |
| Hash map per-tick rebuild for state snapshots causes allocation churn | Low | Low | Replace with vector-based snapshot; profile first |
| BFS visited set allocation overhead on large conduit networks | Low | Low | Profile; replace with flat grid if needed |
| Event buffers grow during rapid state oscillation | Low | Low | Events cleared each tick; bounded by entity count |

---

## 15. Recommendations

### Must Address (before next milestone)

1. **Fix O(N*M) reverse position lookup** (F6-PA-01): Add `std::unordered_map<uint32_t, uint64_t> m_entity_to_position[MAX_PLAYERS]` for each entity type (extractor, consumer). This converts three O(N*M) operations to O(N) and is critical for large-map playability. This is the same recommendation made in the Epic 5 review (E5-PA-02) and should be addressed in both systems.

### Should Address (before multiplayer testing)

2. **Add serialization tests** (F6-TC-01): Round-trip tests for all three serialization formats. The bit-packing logic in particular is error-prone and benefits from exhaustive testing (boundary cases at count=0, count=1, count=7, count=8, count=9).

3. **Sync conduit coverage_radius default** (F6-CD-01): Either change `FluidConduitComponent::coverage_radius` default to 3, or change `place_conduit()` to not override the default. Consistency prevents subtle bugs when conduits are created outside the placement path.

### Consider (for future robustness)

4. **Add FluidPlacementValidation unit tests** (F6-TC-02): Direct tests for `calculate_water_factor()` at all distance breakpoints and for `validate_extractor_placement()` / `validate_reservoir_placement()` with mock terrain.

5. **Consolidate position packing utilities** (F6-SC-01): Move `pack_pos` / `unpack_x` / `unpack_y` to a shared utility header and use them from both FluidSystem and FluidCoverageBFS.

6. **Replace float threshold with integer** (F6-MP-01): Change `buffer_threshold = static_cast<uint32_t>(float(available) * 0.10f)` to `buffer_threshold = available / 10` for deterministic cross-platform behavior.

---

*Review generated by Systems Architect Agent. Findings should be triaged by the project lead and tracked as tickets where appropriate.*
