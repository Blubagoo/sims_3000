# Systems Architect Analysis: Epic 6 - Water Infrastructure (FluidSystem)

**Epic:** 6 - Water Infrastructure
**System:** FluidSystem
**Canon Version:** 0.7.0
**Analysis Date:** 2026-01-28
**Agent:** Systems Architect

---

## Executive Summary

Epic 6 implements the fluid (water) infrastructure system, closely mirroring Epic 5's pool model pattern. FluidSystem is the second resource system, creating the complete "power + water = development" formula that gates building progression. Key differences from EnergySystem: fluid extractors require water proximity (TerrainSystem dependency) and power (EnergySystem dependency), fluid reservoirs provide storage buffers, and there is no aging/efficiency degradation or contamination from fluid infrastructure.

---

## System Boundaries

### FluidSystem Owns

| Concern | Ownership | Notes |
|---------|-----------|-------|
| Fluid extractor (pump) simulation | Full | Extraction rate based on water proximity |
| Fluid reservoir (tower) storage | Full | Buffers against demand spikes |
| Fluid pool calculation (per player) | Full | Generated - consumed = surplus |
| Fluid coverage zone calculation | Full | BFS from reservoirs/extractors through conduits |
| Fluid distribution to buildings | Full | Powered = in coverage AND pool surplus |
| FluidComponent state | Full | Per-consumer fluid state |
| FluidProducerComponent state | Full | Per-producer output/storage |
| FluidConduitComponent state | Full | Per-conduit connectivity |

### FluidSystem Does NOT Own

| Concern | Owner | Notes |
|---------|-------|-------|
| Building fluid consumption amounts | BuildingSystem (Epic 4) | Defined per building template |
| Fluid UI/overlay | UISystem (Epic 12) | EnergySystem pattern |
| Fluid costs | EconomySystem (Epic 11) | Build/maintenance costs |
| Water tile data | TerrainSystem (Epic 3) | Provides water proximity queries |
| Power state for extractors | EnergySystem (Epic 5) | Extractors require power |
| Fluid conduit rendering | RenderingSystem (Epic 2) | Just provides state for glow |
| Building state transitions | BuildingSystem (Epic 4) | Responds to fluid state changes |

---

## Data Model

### FluidComponent (12 bytes)

```cpp
struct FluidComponent {
    uint32_t fluid_required = 0;     // Units needed per tick (from template)
    uint32_t fluid_received = 0;     // Units actually received this tick
    bool     is_hydrated = false;    // True if fluid_received >= fluid_required
    uint8_t  priority = 3;           // Rationing priority 1-4 (mirrors energy)
    uint8_t  grid_id = 0;            // Future: isolated network support
    uint8_t  padding = 0;            // Alignment
};
// static_assert(sizeof(FluidComponent) == 12, "FluidComponent size mismatch");
```

**Notes:**
- Mirrors EnergyComponent structure exactly for consistency
- `is_hydrated` is the fluid equivalent of `is_powered`
- Same priority levels: 1=Critical, 2=Important, 3=Normal, 4=Low

### FluidProducerComponent (20 bytes)

```cpp
enum class FluidProducerType : uint8_t {
    Extractor = 0,   // Pumps water from water sources
    Reservoir = 1,   // Stores water for buffering
};

struct FluidProducerComponent {
    uint32_t base_output = 0;        // Max output at full capacity
    uint32_t current_output = 0;     // Actual output this tick
    uint32_t reservoir_capacity = 0; // Max storage (0 for extractors)
    uint32_t reservoir_level = 0;    // Current stored fluid
    FluidProducerType producer_type = FluidProducerType::Extractor;
    bool     is_online = true;       // Operational status
    bool     has_power = false;      // Powered by EnergySystem (extractors only)
    uint8_t  water_distance = 255;   // Distance to water (extractors only)
};
// static_assert(sizeof(FluidProducerComponent) == 20, "FluidProducerComponent size mismatch");
```

**Key Differences from EnergyProducerComponent (24 bytes):**
- No `efficiency` field (no weather/time factors)
- No `age_factor` field (no aging mechanics per task scope)
- No `ticks_since_built` (no aging tracking)
- No `contamination_output` (fluid infrastructure doesn't pollute)
- Added `reservoir_capacity` and `reservoir_level` for storage buffers
- Added `producer_type` to distinguish extractors from reservoirs
- Added `has_power` dependency tracking
- Added `water_distance` for extraction efficiency

### FluidConduitComponent (4 bytes)

```cpp
struct FluidConduitComponent {
    uint8_t coverage_radius = 2;  // Tiles of coverage (default 2-3)
    bool    is_connected = false; // Reachable from producer via BFS
    bool    is_active = false;    // Has fluid flowing (for glow rendering)
    uint8_t conduit_level = 1;    // 1=basic, 2=upgraded (future)
};
// static_assert(sizeof(FluidConduitComponent) == 4, "FluidConduitComponent size mismatch");
```

**Notes:**
- Identical to EnergyConduitComponent (good pattern reuse)

### PerPlayerFluidPool (28 bytes)

```cpp
enum class FluidPoolState : uint8_t {
    Healthy = 0,   // Surplus >= buffer threshold
    Marginal = 1,  // 0 <= surplus < buffer threshold
    Deficit = 2,   // Negative surplus, rationing active
    Collapse = 3,  // Severe deficit, most structures dehydrated
};

struct PerPlayerFluidPool {
    PlayerID        owner;                           // Pool owner
    uint32_t        total_generated = 0;             // Sum of extractor outputs
    uint32_t        total_consumed = 0;              // Sum of consumer requirements in coverage
    int32_t         surplus = 0;                     // generated - consumed
    uint32_t        total_reservoir_capacity = 0;    // Sum of all reservoir capacities
    uint32_t        total_reservoir_level = 0;       // Sum of all current storage
    uint32_t        extractor_count = 0;             // Active extractors
    uint32_t        reservoir_count = 0;             // Active reservoirs
    uint32_t        consumer_count = 0;              // Consumers in coverage
    FluidPoolState  state = FluidPoolState::Healthy;
    FluidPoolState  previous_state = FluidPoolState::Healthy;
};
```

**Key Difference from PerPlayerEnergyPool:**
- Added reservoir tracking fields for storage buffer mechanic
- Reservoirs can absorb demand spikes (strategic depth)

### Component Memory Summary

| Component | Size | Entities at 256x256 | Total Memory |
|-----------|------|---------------------|--------------|
| FluidComponent | 12 bytes | ~20,000 buildings | ~240 KB |
| FluidProducerComponent | 20 bytes | ~500 producers | ~10 KB |
| FluidConduitComponent | 4 bytes | ~5,000 conduits | ~20 KB |
| FluidCoverageGrid | 1 byte/cell | 65,536 cells | 64 KB |
| **Total** | - | - | **~334 KB** |

---

## Pool Model (Fluid)

FluidSystem follows the same pool model as EnergySystem per canon `patterns.yaml`:

### Generation Phase

```
For each overseer:
    total_generated = 0
    for each extractor in owner's network:
        if extractor.is_online AND extractor.has_power:
            output = calculate_extraction_output(extractor.water_distance)
            total_generated += output
            extractor.current_output = output
        else:
            extractor.current_output = 0

    for each reservoir in owner's network:
        if reservoir.is_online:
            # Reservoirs distribute stored fluid when demand exceeds generation
            available_from_storage = min(reservoir.reservoir_level, deficit_if_any)
            total_generated += available_from_storage
```

### Consumption Phase

```
For each overseer:
    total_consumed = 0
    for each consumer in coverage zone:
        total_consumed += consumer.fluid_required

    surplus = total_generated - total_consumed
```

### Reservoir Buffer Mechanic (New)

Reservoirs provide strategic depth beyond simple generation:

1. **Filling:** When `surplus > 0`, excess flows into reservoirs (up to capacity)
2. **Draining:** When `surplus < 0`, reservoirs release stored fluid to meet demand
3. **Buffer Time:** Full reservoir provides X ticks of buffer during generation loss

```cpp
// Simplified reservoir update logic
void update_reservoirs(PlayerID owner) {
    auto& pool = pools_[owner];

    if (pool.surplus > 0) {
        // Fill reservoirs with excess
        uint32_t excess = static_cast<uint32_t>(pool.surplus);
        for (auto& reservoir : reservoirs_[owner]) {
            uint32_t space = reservoir.reservoir_capacity - reservoir.reservoir_level;
            uint32_t fill = std::min(excess, space);
            reservoir.reservoir_level += fill;
            excess -= fill;
            if (excess == 0) break;
        }
    } else if (pool.surplus < 0) {
        // Drain reservoirs to cover deficit
        uint32_t deficit = static_cast<uint32_t>(-pool.surplus);
        for (auto& reservoir : reservoirs_[owner]) {
            uint32_t drain = std::min(deficit, reservoir.reservoir_level);
            reservoir.reservoir_level -= drain;
            deficit -= drain;
            pool.surplus += drain; // Reduce deficit
            if (deficit == 0) break;
        }
    }
}
```

### Pool State Determination

```
if surplus >= buffer_threshold (10% of total_generated + reservoir_level):
    state = Healthy
else if surplus >= 0:
    state = Marginal
else if surplus > -collapse_threshold:
    state = Deficit (rationing active)
else:
    state = Collapse (severe crisis)
```

---

## Coverage Algorithm

FluidSystem uses BFS flood-fill identical to EnergySystem (Epic 5 ticket 5-014):

### Algorithm

```
recalculate_coverage(PlayerID owner):
    1. Clear all coverage for owner
    2. Collect all fluid producers (extractors + reservoirs) owned by player
    3. For each producer:
        - If producer.is_online AND (producer has_power OR is reservoir):
            - Mark coverage_radius around producer
            - Add producer position to BFS frontier
    4. While frontier not empty:
        - Pop position from frontier
        - For each 4-directional neighbor:
            - If neighbor has FluidConduitComponent owned by same player:
            - If not already visited:
                - Mark conduit's coverage_radius
                - Set conduit.is_connected = true
                - Add to frontier
    5. Mark all unvisited conduits as is_connected = false
```

### Ownership Boundary Enforcement

Per canon `patterns.yaml`, fluid conduits do NOT connect across overseer boundaries:

```cpp
bool can_extend_coverage_to(int32_t x, int32_t y, PlayerID owner) {
    auto tile_owner = get_tile_owner(x, y);
    return tile_owner == owner || tile_owner == GAME_MASTER;
}
```

### Coverage Grid (Dense 2D Array)

```cpp
class FluidCoverageGrid {
    uint8_t* grid_;  // 0=uncovered, 1-4=owner_id
    int width_, height_;

public:
    bool is_in_coverage(int x, int y, PlayerID owner) const {
        return grid_[y * width_ + x] == owner;
    }

    PlayerID get_coverage_owner(int x, int y) const {
        return grid_[y * width_ + x];
    }

    void clear_for_owner(PlayerID owner);
    void mark(int x, int y, PlayerID owner);
};
```

**Memory:** 1 byte per cell (64KB for 256x256, 256KB for 512x512)

---

## Cross-System Integration

### TerrainSystem (Epic 3) - Water Proximity

**Interface Used:** `ITerrainQueryable`

```cpp
// FluidSystem queries water distance for extraction efficiency
uint8_t water_distance = terrain_->get_water_distance(extractor_x, extractor_y);

// Extraction efficiency based on distance
float efficiency = calculate_extraction_efficiency(water_distance);
// water_distance = 0 (adjacent): 100% efficiency
// water_distance = 1-3: 80% efficiency
// water_distance = 4-6: 60% efficiency
// water_distance >= 7: 40% efficiency (minimum viable)
// water_distance >= 10: Cannot place extractor
```

**Integration Points:**
- Extractor placement validation: `get_water_distance(x, y) <= MAX_EXTRACTOR_DISTANCE`
- Extractor output scaling: `base_output * water_efficiency_factor`
- Water body queries for UI overlay

### EnergySystem (Epic 5) - Extractor Power Dependency

**Interface Used:** `IEnergyProvider`

```cpp
// FluidSystem checks if extractor is powered each tick
bool extractor_powered = energy_provider_->is_powered(extractor_entity);

// Extractor only produces if powered
if (extractor.is_online && extractor_powered) {
    extractor.has_power = true;
    extractor.current_output = calculate_output(...);
} else {
    extractor.has_power = false;
    extractor.current_output = 0;
}
```

**Critical Dependency:**
- FluidSystem MUST run AFTER EnergySystem (tick priority 20 > 10)
- Ensures `is_powered` reflects current tick state
- Reservoirs do NOT require power (passive storage)

### BuildingSystem (Epic 4) - Consumer Registration

**Events Consumed:**
- `BuildingConstructedEvent`: Register fluid consumers/producers
- `BuildingDeconstructedEvent`: Unregister consumers/producers

**Interface Provided:** `IFluidProvider`

```cpp
// Proposed IFluidProvider interface (mirrors IEnergyProvider)
class IFluidProvider {
public:
    virtual bool is_hydrated(EntityID entity) const = 0;
    virtual bool is_hydrated_at(int32_t x, int32_t y, PlayerID owner) const = 0;
    virtual FluidPoolState get_pool_state(PlayerID owner) const = 0;
    virtual const PerPlayerFluidPool& get_pool(PlayerID owner) const = 0;
};
```

**Integration Pattern:**
- Epic 4 defines IFluidProvider stub (like IEnergyProvider stub in 4-019)
- Epic 6 provides real implementation
- BuildingSystem queries `is_hydrated()` for building state transitions

---

## Multiplayer

### Server Authority

| Aspect | Authority | Notes |
|--------|-----------|-------|
| Pool calculation | Server | All resource math on server |
| Coverage calculation | Server | BFS runs server-side only |
| Distribution (rationing) | Server | Deterministic order for consistency |
| Producer/consumer state | Server | is_hydrated set by server |
| Reservoir levels | Server | Storage state authoritative |

### Sync Strategy

**Every Tick Sync:**
- `FluidComponent.is_hydrated` (1 bit per entity, packed)
- Changed component values only (delta compression)

**On Change Sync:**
- `FluidPoolSyncMessage` when pool values change:
  ```cpp
  struct FluidPoolSyncMessage {
      PlayerID owner;
      uint32_t total_generated;
      uint32_t total_consumed;
      int32_t  surplus;
      uint32_t reservoir_level; // Aggregate
      FluidPoolState state;
  }; // ~18 bytes
  ```

- `FluidStateChangedEvent` when structure hydration changes
- Conduit placement/removal events

**Client-Side Reconstruction:**
- Coverage grid reconstructed client-side from conduit positions
- Client runs same BFS algorithm for local overlay rendering
- Server is authoritative; client display matches server state

### Per-Player Pools

Each overseer has their own independent fluid pool:
- Extractors only feed owner's pool
- Conduits only extend owner's coverage
- No fluid trading between players (infrastructure separation)
- All players can SEE all pool states (strategic visibility)

---

## Interface Contracts

### IFluidProvider (New - mirrors IEnergyProvider)

```yaml
IFluidProvider:
  description: "Provides fluid state queries for other systems"
  purpose: "Allows BuildingSystem and other consumers to query hydration state"

  methods:
    - name: is_hydrated
      params:
        - name: entity
          type: EntityID
      returns: bool
      description: "Whether entity is currently receiving fluid"

    - name: is_hydrated_at
      params:
        - name: x
          type: int32_t
        - name: y
          type: int32_t
        - name: owner
          type: PlayerID
      returns: bool
      description: "Whether position has fluid coverage and pool surplus"

    - name: get_pool_state
      params:
        - name: owner
          type: PlayerID
      returns: FluidPoolState
      description: "Current pool state (Healthy/Marginal/Deficit/Collapse)"

    - name: get_pool
      params:
        - name: owner
          type: PlayerID
      returns: const PerPlayerFluidPool&
      description: "Full pool data for UI display"

  implemented_by:
    - FluidSystem (Epic 6)

  notes:
    - "Replaces Epic 4 stub (should be ticket 4-0XX for fluid stub)"
    - "is_hydrated_at combines coverage check + pool surplus check"
```

### IFluidConsumer (Existing in Canon)

From `interfaces.yaml`:
- `get_fluid_required()` -> uint32
- `on_fluid_state_changed(bool has_fluid)` -> void
- Implemented by: habitation, exchange, some fabrication buildings

### IFluidProducer (Existing in Canon)

From `interfaces.yaml`:
- `get_fluid_output()` -> uint32
- `get_reservoir_level()` -> uint32
- Implemented by: fluid extractors, fluid reservoirs

---

## Tick Ordering

Per `interfaces.yaml` ISimulatable:

| Priority | System | Notes |
|----------|--------|-------|
| 5 | TerrainSystem | Terrain queries available |
| **10** | **EnergySystem** | Power state settled |
| **20** | **FluidSystem** | Can query is_powered for extractors |
| 30 | ZoneSystem | Zone designation |
| 40 | BuildingSystem | Can query is_hydrated for state |
| 50 | PopulationSystem | Habitability checks |
| 60 | EconomySystem | Revenue/costs |
| 70 | DisorderSystem | Crime simulation |
| 80 | ContaminationSystem | Pollution simulation |

**Critical:** FluidSystem at priority 20 runs AFTER EnergySystem (10) to ensure extractor power state is current.

### FluidSystem Tick Phases

```
tick(float delta_time):
    Phase 1: Coverage recalculation (if dirty)
        - recalculate_coverage() for dirty overseers

    Phase 2: Producer update
        - Update extractor power state from IEnergyProvider
        - Calculate extractor output based on power + water proximity
        - Reservoirs: no update needed (passive storage)

    Phase 3: Pool calculation
        - Sum generation from all online producers
        - Sum consumption from covered consumers
        - Calculate surplus

    Phase 4: Reservoir management
        - If surplus > 0: fill reservoirs
        - If surplus < 0: drain reservoirs

    Phase 5: Pool state determination
        - Calculate pool state from adjusted surplus
        - Detect state transitions, emit events

    Phase 6: Distribution
        - If surplus >= 0: all covered consumers hydrated
        - If surplus < 0: priority-based rationing

    Phase 7: State change events
        - Emit FluidStateChangedEvent for hydration transitions

    Phase 8: Conduit active state
        - Update is_active for rendering glow
```

---

## Questions for Other Agents

### For Game Designer

1. **Fluid Requirements per Building Type:** What fluid_required values for each zone type and density? Should mirror energy pattern? (Suggested: identical or proportional to energy requirements)

2. **Reservoir Storage Capacity:** What capacity values for fluid reservoir tiers? How many ticks of buffer should a full reservoir provide?

3. **Extraction Efficiency Curve:** What efficiency multiplier at each water distance tier? Suggested:
   - 0 tiles: 100%
   - 1-3 tiles: 80%
   - 4-6 tiles: 60%
   - 7-9 tiles: 40%
   - 10+ tiles: Cannot place

4. **Priority Mapping:** Should fluid priority map exactly to energy priority, or does fluid have different critical consumers? (Medical still highest, habitation still lowest?)

5. **Grace Period:** Should buildings have the same 100-tick grace period before "dehydrated" state as they do for "unpowered"?

6. **Reservoir Strategic Intent:** Are reservoirs intended as:
   - Emergency buffer (survive extractor loss)
   - Demand spike absorption (prevent rationing flickers)
   - Strategic resource stockpiling
   - All of the above?

### For Services Engineer

1. **Epic 4 IFluidProvider Stub:** Does Epic 4 have a stub similar to IEnergyProvider (ticket 4-019)? If not, should we create one during Epic 4 or define it in Epic 6?

2. **Network Message Optimization:** Can FluidPoolSyncMessage share encoding with EnergyPoolSyncMessage, or should they be distinct message types?

3. **Coverage Grid Memory:** Should FluidCoverageGrid be a separate allocation or could we pack energy and fluid coverage into a single 2-byte-per-cell grid?

4. **Reservoir Drain Order:** When multiple reservoirs exist, what drain order? FIFO (oldest first), proportional, or configurable?

5. **Performance Validation:** Epic 5 targets <10ms coverage recalc, <1ms pool calc, <2ms full tick. Should Epic 6 have identical targets?

---

## Work Items

### Group A: Data Types and Enumerations

1. **6-001: Fluid Type Enumerations** [S, P0]
   - FluidProducerType enum (Extractor, Reservoir)
   - FluidPoolState enum (Healthy, Marginal, Deficit, Collapse)
   - Unit tests for enum ranges

2. **6-002: FluidComponent Structure** [S, P0]
   - 12-byte struct with static_assert
   - Fields: fluid_required, fluid_received, is_hydrated, priority, grid_id, padding
   - Unit tests for size and initialization

3. **6-003: FluidProducerComponent Structure** [S, P0]
   - 20-byte struct with static_assert
   - Fields: base_output, current_output, reservoir_capacity, reservoir_level, producer_type, is_online, has_power, water_distance
   - Unit tests for size and initialization

4. **6-004: FluidConduitComponent Structure** [S, P0]
   - 4-byte struct with static_assert (mirrors EnergyConduitComponent)
   - Fields: coverage_radius, is_connected, is_active, conduit_level

5. **6-005: PerPlayerFluidPool Structure** [S, P0]
   - Aggregated pool data per overseer
   - Includes reservoir tracking fields
   - Unit tests for surplus calculation

6. **6-006: Fluid Event Definitions** [S, P0]
   - FluidStateChangedEvent, FluidDeficitBeganEvent, FluidDeficitEndedEvent
   - FluidCollapseBeganEvent, FluidCollapseEndedEvent
   - ConduitPlacedEvent, ConduitRemovedEvent (may share with energy?)
   - ExtractorPlacedEvent, ExtractorRemovedEvent, ReservoirPlacedEvent, ReservoirRemovedEvent

### Group B: Coverage System

7. **6-007: FluidCoverageGrid Dense 2D Array** [M, P0]
   - 1 byte per cell, same pattern as Epic 5
   - O(1) queries for is_in_coverage, get_coverage_owner
   - Memory: 64KB (256x256) to 256KB (512x512)

8. **6-008: FluidSystem Class Skeleton and ISimulatable** [M, P0]
   - ISimulatable at priority 20
   - Constructor takes ITerrainQueryable* and IEnergyProvider*
   - Internal FluidCoverageGrid, per-player pools, dirty flags
   - All query method declarations

9. **6-009: IFluidProvider Interface Implementation** [M, P0]
   - is_hydrated(EntityID), is_hydrated_at(x, y, owner), get_pool_state(owner), get_pool(owner)
   - Replaces Epic 4 stub (or establishes pattern)
   - Integration tests with Epic 4's BuildingSystem

10. **6-010: Coverage Zone BFS Flood-Fill Algorithm** [L, P0]
    - Same algorithm as Epic 5 ticket 5-014
    - Seed from extractors (if powered) and reservoirs (always)
    - Extend through same-owner conduits
    - Performance target: <10ms for 512x512 with 5,000 conduits

11. **6-011: Coverage Dirty Flag Tracking** [M, P1]
    - Dirty when conduit/producer placed or removed
    - Only recalculate if dirty

12. **6-012: Ownership Boundary Enforcement in Coverage** [M, P0]
    - Coverage does NOT cross overseer boundaries
    - Same-owner OR GAME_MASTER tiles only

13. **6-013: Coverage Queries** [S, P0]
    - is_in_coverage(x, y, owner), get_coverage_at(x, y), get_coverage_count(owner)
    - O(1) via FluidCoverageGrid

### Group C: Pool Calculation and Distribution

14. **6-014: Extractor Registration and Output Calculation** [M, P0]
    - Register/unregister extractors per overseer
    - Output = base_output * water_efficiency * (has_power ? 1 : 0)
    - Query IEnergyProvider for power state

15. **6-015: Reservoir Registration and Storage Management** [M, P0]
    - Register/unregister reservoirs per overseer
    - Fill when surplus > 0, drain when surplus < 0
    - Track total_reservoir_capacity and total_reservoir_level per pool

16. **6-016: Consumer Registration and Requirement Aggregation** [M, P0]
    - Same pattern as Epic 5 ticket 5-011
    - Only consumers in coverage contribute to total_consumed

17. **6-017: Pool Calculation** [M, P0]
    - total_generated = extractors + reservoir drain
    - total_consumed = sum of covered consumer requirements
    - surplus = generated - consumed (adjusted by reservoir)

18. **6-018: Pool State Machine** [M, P0]
    - Same thresholds as EnergySystem (configurable)
    - Transition detection and event emission

19. **6-019: Fluid Distribution (Healthy/Marginal Pools)** [M, P0]
    - All covered consumers hydrated when surplus >= 0
    - is_hydrated = true, fluid_received = fluid_required

20. **6-020: Priority-Based Rationing During Deficit** [L, P0]
    - Same pattern as Epic 5 ticket 5-019
    - Sort by priority then EntityID, distribute until exhausted

21. **6-021: FluidStateChangedEvent Emission** [S, P1]
    - Emit when is_hydrated transitions

22. **6-022: Pool State Transition Event Emission** [S, P1]
    - Emit deficit/collapse began/ended events

### Group D: Producer Mechanics

23. **6-023: Fluid Extractor Type Definition and Stats** [M, P0]
    - Base output values, build costs, water distance requirements
    - No contamination (clean infrastructure)
    - Coverage radius

24. **6-024: Fluid Reservoir Type Definition and Stats** [M, P0]
    - Capacity values, build costs
    - No power requirement (passive)
    - Coverage radius

25. **6-025: Water Proximity Extraction Efficiency** [M, P0]
    - Query ITerrainQueryable.get_water_distance()
    - Efficiency curve based on distance
    - Extractor placement validation: max distance check

26. **6-026: Extractor Power Dependency** [M, P0]
    - Query IEnergyProvider.is_powered(extractor_entity)
    - current_output = 0 if not powered
    - has_power flag for UI feedback

27. **6-027: Extractor Placement Validation** [M, P0]
    - Water proximity check (get_water_distance <= MAX)
    - Terrain buildable check
    - Ownership check
    - No existing structure check

28. **6-028: Reservoir Placement Validation** [M, P0]
    - Standard placement checks (no water proximity required)
    - Reservoirs can be placed anywhere buildable

### Group E: Conduit Mechanics

29. **6-029: Fluid Conduit Placement and Validation** [M, P0]
    - Same pattern as Epic 5 ticket 5-027
    - Emit ConduitPlacedEvent, set dirty flag

30. **6-030: Conduit Connection Detection** [M, P0]
    - Set is_connected during BFS traversal
    - Isolated conduits remain is_connected = false

31. **6-031: Conduit Removal and Coverage Update** [M, P0]
    - Emit ConduitRemovedEvent, set dirty flag
    - No refund

32. **6-032: Conduit Active State for Rendering** [S, P1]
    - is_active = is_connected AND pool.total_generated > 0

33. **6-033: Conduit Placement Preview (Coverage Delta)** [S, P1]
    - Show new coverage tiles during placement
    - Same pattern as Epic 5 ticket 5-031

### Group F: Integration

34. **6-034: BuildingConstructedEvent Handler** [M, P0]
    - Register consumers (FluidComponent) and producers
    - Set coverage dirty for producer placements

35. **6-035: BuildingDeconstructedEvent Handler** [M, P0]
    - Unregister consumers and producers
    - Set coverage dirty for producer removals

36. **6-036: Network Serialization for FluidComponent** [M, P1]
    - is_hydrated synced every tick
    - Same pattern as Epic 5 ticket 5-034

37. **6-037: Network Serialization for Pool State** [S, P1]
    - FluidPoolSyncMessage on change
    - All players see all pool states

38. **6-038: Integration with Epic 4 BuildingSystem** [L, P0]
    - Replace IFluidProvider stub with real implementation
    - BuildingSystem queries is_hydrated() for state transitions
    - Grace period (100 ticks) before state change

### Group G: Content and Design

39. **6-039: Define Fluid Requirements Per Structure Template** [M, P0]
    - Values for each zone type and density
    - Service building requirements
    - Document in template system

40. **6-040: Define Priority Assignments Per Structure Type** [S, P0]
    - Same mapping as energy priorities (or document differences)

41. **6-041: Balance Deficit/Collapse Thresholds** [S, P2]
    - Configurable values for pool state calculation
    - Playtest documentation

42. **6-042: Fluid Overlay UI Requirements** [S, P2]
    - Document for Epic 12 implementation
    - Coverage zones, hydrated indicators, pool status

### Group H: Testing and Validation

43. **6-043: FluidSystem Integration Test Suite** [L, P1]
    - Full pipeline tests (extractor -> coverage -> consumer -> hydration)
    - Reservoir buffer tests
    - Power dependency tests
    - Ownership boundary tests

44. **6-044: Multiplayer Sync Verification Tests** [M, P1]
    - is_hydrated state matches server/clients
    - Deterministic rationing order
    - Pool state transitions sync

45. **6-045: Performance Benchmark Suite** [M, P1]
    - Coverage recalc <10ms at 512x512
    - Pool calculation <1ms at 10,000 consumers
    - Full tick <2ms at 256x256

### Group I: Documentation

46. **6-046: FluidSystem Technical Documentation** [M, P2]
    - Architecture overview
    - Component documentation
    - Pool model with reservoir buffers
    - Coverage algorithm
    - Interface documentation

47. **6-047: Fluid Balance Design Document** [S, P2]
    - Extractor/reservoir comparison
    - Structure consumption reference
    - Expected gameplay scenarios

---

## Ticket Summary

| Category | Count |
|----------|-------|
| **By Type** | |
| Infrastructure | 8 |
| Feature | 28 |
| Content | 2 |
| Design | 2 |
| Testing | 3 |
| Documentation | 2 |
| **By Priority** | |
| P0 (critical path) | 30 |
| P1 (important) | 11 |
| P2 (enhancement) | 6 |
| **Total** | **47** |

---

## Canon Update Flags

The following canon updates should be proposed:

| File | Change | Rationale |
|------|--------|-----------|
| interfaces.yaml | Add IFluidProvider interface | Mirror IEnergyProvider pattern |
| systems.yaml | Add FluidSystem tick_priority: 20 | Document tick ordering |
| patterns.yaml | Add FluidCoverageGrid to dense_grid_exception | Follows same pattern as CoverageGrid |
| terminology.yaml | Confirm fluid_extractor, fluid_reservoir terms | Verify canonical naming |

---

## Risk Assessment

### Low Risk
- Pattern heavily reuses Epic 5 architecture (proven design)
- No aging/efficiency degradation (simpler than energy)
- No contamination from fluid infrastructure (fewer integration points)

### Medium Risk
- Dual dependency (TerrainSystem + EnergySystem) creates integration complexity
- Reservoir buffer mechanic is new (not in Epic 5), needs design validation
- Tick ordering critical: FluidSystem MUST run after EnergySystem

### Mitigations
- IEnergyProvider interface provides clean power queries
- ITerrainQueryable provides clean water proximity queries
- Reservoir buffer mechanic has clear fallback (disable if complex)
- Integration tests verify tick ordering

---

**Document Status:** Ready for Game Designer and Services Engineer Review

**Next Steps:**
1. Game Designer answers design questions (fluid requirements, reservoir mechanics)
2. Services Engineer confirms Epic 4 stub pattern for IFluidProvider
3. Discussion document created for unresolved questions
4. Ticket refinement based on answers
