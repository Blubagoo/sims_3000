# Services Engineer Analysis: Epic 6

**Epic:** Water Infrastructure (FluidSystem)
**Agent:** Services Engineer
**Date:** 2026-01-28
**Canon Version:** 0.7.0

---

## Executive Summary

Epic 6 implements water infrastructure following the pool model pattern established in Epic 5. FluidSystem shares architectural patterns with EnergySystem but introduces key variations: water proximity requirements for extractors, power dependency for operation, and reservoir buffering for supply smoothing. This analysis focuses on coverage system design, pool calculation with reservoirs, component structures, and integration with terrain and energy systems.

---

## Coverage System Design

### FluidCoverageGrid

Like Epic 5's CoverageGrid, FluidCoverageGrid uses dense 2D array storage per the `dense_grid_exception` pattern from canon. The grid stores coverage owner per cell for O(1) queries.

**Data Structure:**
```cpp
class FluidCoverageGrid {
private:
    std::vector<uint8_t> coverage_;  // 0=uncovered, 1-4=owner PlayerID
    uint32_t width_;
    uint32_t height_;

public:
    // O(1) queries
    bool is_in_coverage(int32_t x, int32_t y, PlayerID owner) const;
    PlayerID get_coverage_owner(int32_t x, int32_t y) const;

    // Mutations
    void set(int32_t x, int32_t y, PlayerID owner);
    void clear(int32_t x, int32_t y);
    void clear_all_for_owner(PlayerID owner);
};
```

**Memory Budget:**
| Map Size | Grid Cells | Memory |
|----------|-----------|--------|
| 128x128 | 16,384 | 16 KB |
| 256x256 | 65,536 | 64 KB |
| 512x512 | 262,144 | 256 KB |

### BFS Flood-Fill Algorithm

Coverage propagates from fluid extractors and reservoirs through connected fluid conduits. The algorithm is identical to Epic 5's energy coverage with one critical difference: extractors must have power AND water proximity to contribute to coverage seeding.

**Algorithm Steps:**
1. Clear coverage for owner
2. Seed from all OPERATIONAL extractors and reservoirs owned by player
   - Extractor is operational if: `is_powered(entity) == true AND water_distance <= MAX_PUMP_DISTANCE`
3. BFS through 4-directional neighbors
4. If neighbor has fluid conduit owned by same player, mark conduit's radius, add to frontier
5. Continue until frontier empty

**Operational Check (Extractors Only):**
```cpp
bool is_extractor_operational(EntityID extractor) const {
    const auto& producer = registry.get<FluidProducerComponent>(extractor);
    const auto& pos = registry.get<PositionComponent>(extractor);

    // Must have power
    if (!energy_provider_->is_powered(extractor)) {
        return false;
    }

    // Must be within water proximity
    uint8_t water_dist = terrain_queryable_->get_water_distance(pos.grid_x, pos.grid_y);
    return water_dist <= producer.max_water_distance;
}
```

**Ownership Boundary Enforcement:**
Per canon `patterns.yaml`, fluid conduits do NOT connect across overseer boundaries. Coverage only extends to tiles owned by same player or GAME_MASTER (unclaimed).

---

## Pool Calculation with Reservoirs

### Pool Model Overview

FluidSystem uses the same pool model as EnergySystem:
- Each overseer has a fluid pool
- Extractors add to pool (generation)
- Buildings draw from pool (consumption)
- Reservoirs provide buffering (unique to fluid)

### Pool Calculation Formula

```
available_fluid = current_generation + reservoir_stored
total_consumed = SUM(consumer.fluid_required for consumers in coverage)
surplus = available_fluid - total_consumed
```

**Key Difference from Energy:** Reservoirs contribute stored fluid to the available pool, not just generation. This smooths supply fluctuations.

### PerPlayerFluidPool Structure

```cpp
struct PerPlayerFluidPool {
    PlayerID owner;                    // Overseer who owns this pool
    uint32_t total_generated;          // Sum of operational extractor outputs
    uint32_t total_reservoir_stored;   // Sum of all reservoir current levels
    uint32_t total_reservoir_capacity; // Sum of all reservoir max capacities
    uint32_t available;                // total_generated + total_reservoir_stored
    uint32_t total_consumed;           // Sum of consumer fluid_required in coverage
    int32_t surplus;                   // available - total_consumed (can be negative)
    uint32_t extractor_count;          // Number of operational extractors
    uint32_t reservoir_count;          // Number of reservoirs
    uint32_t consumer_count;           // Number of consumers in coverage
    FluidPoolState state;              // Healthy/Marginal/Deficit/Collapse
    FluidPoolState previous_state;     // For transition detection
};
// Size: ~48 bytes
```

### Pool State Thresholds

| State | Condition |
|-------|-----------|
| Healthy | surplus >= buffer_threshold (10% of available) |
| Marginal | 0 <= surplus < buffer_threshold |
| Deficit | -collapse_threshold < surplus < 0 |
| Collapse | surplus <= -collapse_threshold (50% of total_consumed) |

---

## Component Designs

### FluidComponent (Consumer)

Attached to all structures that consume fluid (zone structures, some service structures).

```cpp
struct FluidComponent {
    uint32_t fluid_required;   // Fluid units needed per tick, from template
    uint32_t fluid_received;   // Fluid units actually received this tick
    bool has_fluid;            // true if fluid_received >= fluid_required
    uint8_t padding[3];        // Alignment to 12 bytes
};
static_assert(sizeof(FluidComponent) == 12, "FluidComponent must be 12 bytes");
```

**Size:** 12 bytes (matches EnergyComponent for consistency)

**Notes:**
- No priority field (unlike energy) - fluid distribution is all-or-nothing based on pool state
- Fluid is binary: structure has water or doesn't
- No rationing during deficit - buildings simply go without water

### FluidProducerComponent (Extractor)

Attached to fluid extractors that pump water.

```cpp
struct FluidProducerComponent {
    uint32_t base_output;       // Maximum output at optimal conditions
    uint32_t current_output;    // Actual output = base_output * water_factor
    uint8_t max_water_distance; // Maximum tiles from water source (typically 3-5)
    uint8_t current_water_dist; // Actual distance to water (from terrain query)
    bool is_operational;        // true if powered AND within water proximity
    uint8_t producer_type;      // ExtractorType enum (for future expansion)
    // Power dependency handled via IEnergyProvider query, not stored here
};
static_assert(sizeof(FluidProducerComponent) == 12, "FluidProducerComponent must be 12 bytes");
```

**Size:** 12 bytes

**Key Differences from EnergyProducerComponent:**
- No aging/efficiency - extractors maintain output indefinitely (water doesn't degrade pumps)
- No contamination output - fluid infrastructure is clean
- Water proximity factor affects output based on distance

**Water Proximity Factor:**
```cpp
float get_water_factor(uint8_t water_distance, uint8_t max_distance) {
    if (water_distance > max_distance) return 0.0f;
    if (water_distance == 0) return 1.0f;  // Adjacent to water
    // Linear falloff from 1.0 at distance 0 to 0.5 at max_distance
    return 1.0f - (0.5f * water_distance / max_distance);
}
```

### FluidReservoirComponent (Storage Buffer)

Attached to fluid reservoirs (water towers) that store fluid buffer.

```cpp
struct FluidReservoirComponent {
    uint32_t capacity;          // Maximum storage
    uint32_t current_level;     // Current stored amount
    uint16_t fill_rate;         // Units per tick that can fill
    uint16_t drain_rate;        // Units per tick that can drain
    bool is_active;             // true if connected to network
    uint8_t reservoir_type;     // ReservoirType enum (for future expansion)
    uint8_t padding[2];         // Alignment
};
static_assert(sizeof(FluidReservoirComponent) == 16, "FluidReservoirComponent must be 16 bytes");
```

**Size:** 16 bytes

**Reservoir Behavior:**
- When pool has surplus: reservoirs fill (capped by fill_rate)
- When pool has deficit: reservoirs drain to meet demand (capped by drain_rate)
- Reservoirs smooth supply fluctuations (unlike energy which has instant-on nexuses)

### FluidConduitComponent

Attached to fluid conduit entities (pipes) that extend coverage zones.

```cpp
struct FluidConduitComponent {
    uint8_t coverage_radius;   // Tiles of coverage this conduit adds (typically 2-3)
    bool is_connected;         // true if connected to fluid network via BFS
    bool is_active;            // true if carrying fluid (for rendering flow)
    uint8_t conduit_level;     // 1=basic pipe, 2=upgraded (future expansion)
};
static_assert(sizeof(FluidConduitComponent) == 4, "FluidConduitComponent must be 4 bytes");
```

**Size:** 4 bytes (identical to EnergyConduitComponent)

---

## Water Proximity Validation

### ITerrainQueryable Integration

FluidSystem depends on Epic 3's ITerrainQueryable interface for water proximity checks.

**Key Method:**
```cpp
// From interfaces.yaml
uint8_t get_water_distance(int32_t x, int32_t y);
// Returns Manhattan distance to nearest water tile (capped at 255)
```

### Extractor Placement Validation

```cpp
bool validate_extractor_placement(int32_t x, int32_t y, PlayerID owner) {
    // 1. Bounds check
    if (!is_valid_position(x, y)) return false;

    // 2. Ownership check
    if (get_tile_owner(x, y) != owner && get_tile_owner(x, y) != GAME_MASTER) {
        return false;
    }

    // 3. Terrain buildable check
    if (!terrain_queryable_->is_buildable(x, y)) return false;

    // 4. No existing structure check
    if (building_grid_->has_building(x, y)) return false;

    // 5. Water proximity check (UNIQUE TO FLUID)
    uint8_t water_dist = terrain_queryable_->get_water_distance(x, y);
    if (water_dist > MAX_EXTRACTOR_PLACEMENT_DISTANCE) {
        return false;  // Too far from water - cannot place here
    }

    return true;
}
```

**Placement Distance vs Operational Distance:**
- MAX_EXTRACTOR_PLACEMENT_DISTANCE: 8 tiles (can place extractor)
- MAX_PUMP_DISTANCE (in FluidProducerComponent): 5 tiles (extractor is operational)
- Extractors placed 6-8 tiles from water can be placed but start non-operational

This creates interesting gameplay: players can place extractors speculatively before building to water, or place them at the edge of range accepting reduced efficiency.

### Water Proximity Preview

During extractor placement, show water distance and expected efficiency:
```cpp
struct ExtractorPlacementPreview {
    bool can_place;           // Within placement distance
    bool will_be_operational; // Within operational distance
    uint8_t water_distance;   // Actual distance
    float expected_efficiency; // Based on water factor
};
```

---

## Power Dependency

### IEnergyProvider Integration

FluidSystem depends on Epic 5's IEnergyProvider interface for power state queries.

**Key Method:**
```cpp
// From interfaces.yaml
bool is_powered(EntityID entity);
// Whether entity is currently receiving power
```

### Extractor Power Requirements

Fluid extractors are energy consumers. They require power to pump water.

**Component Setup:**
When a fluid extractor is placed, it gets both:
- `FluidProducerComponent` (produces fluid)
- `EnergyComponent` (consumes energy)

**Tick Order Dependency:**
- EnergySystem runs at tick_priority: 10
- FluidSystem runs at tick_priority: 20 (AFTER energy)
- This ensures power state is resolved before fluid calculations

**Operational Check Flow:**
```cpp
// In FluidSystem::tick()
for (auto& extractor : player_extractors_[owner]) {
    auto& producer = registry.get<FluidProducerComponent>(extractor);

    // Check power first (from previous system's tick)
    bool has_power = energy_provider_->is_powered(extractor);

    // Then check water proximity
    auto& pos = registry.get<PositionComponent>(extractor);
    uint8_t water_dist = terrain_queryable_->get_water_distance(pos.grid_x, pos.grid_y);
    bool in_water_range = water_dist <= producer.max_water_distance;

    // Operational only if BOTH conditions met
    producer.is_operational = has_power && in_water_range;

    if (producer.is_operational) {
        float water_factor = get_water_factor(water_dist, producer.max_water_distance);
        producer.current_output = static_cast<uint32_t>(producer.base_output * water_factor);
    } else {
        producer.current_output = 0;
    }
}
```

### Power Loss Cascade

When an extractor loses power:
1. EnergySystem marks extractor as unpowered
2. FluidSystem (next tick) sees is_powered = false
3. Extractor becomes non-operational
4. Pool generation drops
5. If pool goes to deficit, buildings lose water
6. Buildings degrade (affects population, services)

This creates cascading failure scenarios - power loss can cause water crisis.

---

## Reservoir Buffering

### Buffering Logic

Reservoirs smooth supply fluctuations by storing excess during surplus and releasing during deficit.

**Tick Phases:**
1. Calculate generation (from operational extractors)
2. Calculate consumption (from consumers in coverage)
3. Determine raw surplus/deficit
4. Apply reservoir buffering
5. Calculate final pool state
6. Distribute fluid to consumers

### Reservoir Fill/Drain Algorithm

```cpp
void apply_reservoir_buffering(PlayerID owner) {
    auto& pool = player_pools_[owner];

    // Raw calculation (before reservoir)
    int32_t raw_surplus = pool.total_generated - pool.total_consumed;

    if (raw_surplus > 0) {
        // SURPLUS: Fill reservoirs
        uint32_t excess = static_cast<uint32_t>(raw_surplus);
        for (auto& reservoir_id : player_reservoirs_[owner]) {
            auto& res = registry.get<FluidReservoirComponent>(reservoir_id);
            uint32_t space = res.capacity - res.current_level;
            uint32_t fill_amount = std::min({excess, space, static_cast<uint32_t>(res.fill_rate)});
            res.current_level += fill_amount;
            excess -= fill_amount;
            if (excess == 0) break;
        }
    } else if (raw_surplus < 0) {
        // DEFICIT: Drain reservoirs
        uint32_t needed = static_cast<uint32_t>(-raw_surplus);
        for (auto& reservoir_id : player_reservoirs_[owner]) {
            auto& res = registry.get<FluidReservoirComponent>(reservoir_id);
            uint32_t drain_amount = std::min({needed, res.current_level, static_cast<uint32_t>(res.drain_rate)});
            res.current_level -= drain_amount;
            needed -= drain_amount;
            if (needed == 0) break;
        }
        // Adjust surplus based on what reservoirs provided
        pool.surplus = raw_surplus + (static_cast<int32_t>(-raw_surplus) - needed);
    }

    // Update total stored
    pool.total_reservoir_stored = calculate_total_stored(owner);
}
```

### Reservoir Strategic Value

1. **Smoothing Fluctuations:** Single extractor going offline doesn't immediately cause crisis
2. **Build-Ahead Buffer:** Build reservoirs before expanding to avoid growing pains
3. **Recovery Aid:** Reservoirs help recover from power outages faster
4. **Peak Demand:** Handle temporary demand spikes without building more extractors

### Reservoir vs Energy Comparison

| Aspect | Energy (Epic 5) | Fluid (Epic 6) |
|--------|----------------|----------------|
| Storage | None (instant on/off) | Reservoirs store buffer |
| Response Time | Immediate | Buffered (gradual) |
| Production Stability | Nexus aging | Constant output |
| Deficit Handling | Priority rationing | All-or-nothing + reservoir drain |
| Crisis Recovery | Build more nexuses | Reservoirs buy time |

---

## Performance Targets

### Tick Budget Allocation

FluidSystem tick_priority: 20 (after EnergySystem at 10, before ZoneSystem at 30)

| Phase | Operation | Target Budget |
|-------|-----------|---------------|
| 1 | Coverage recalculation (if dirty) | <10ms |
| 2 | Extractor operational checks | <0.5ms |
| 3 | Pool calculation | <0.5ms |
| 4 | Reservoir buffering | <0.2ms |
| 5 | Fluid distribution | <0.5ms |
| 6 | Event emission | <0.2ms |
| **Total** | Full tick | **<2ms** for 256x256 |

### Scalability Targets

| Metric | Target |
|--------|--------|
| Coverage recalc (512x512, 5000 conduits) | <10ms |
| Pool calculation (10,000 consumers) | <1ms |
| Full tick (256x256 map) | <2ms |
| Full tick (512x512 map) | <5ms |

### Memory Budget

| Data Structure | Size (256x256) | Size (512x512) |
|----------------|----------------|----------------|
| FluidCoverageGrid | 64 KB | 256 KB |
| Per-player pools (4 players) | ~200 bytes | ~200 bytes |
| Extractor tracking | ~1 KB | ~4 KB |
| Reservoir tracking | ~1 KB | ~4 KB |
| Consumer tracking | ~50 KB | ~200 KB |
| **Total** | ~120 KB | ~470 KB |

---

## Questions for Other Agents

### For Systems Architect

1. **IFluidProvider Interface:** Should we define an IFluidProvider interface (like IEnergyProvider) for other systems to query fluid state? BuildingSystem needs to know has_fluid for development decisions.

2. **Tick Priority Confirmation:** FluidSystem at priority 20 (after Energy at 10) - is this the right ordering? Need to ensure power state is resolved before fluid checks.

3. **Event Bus vs Direct Query:** Should reservoir state changes emit events, or should interested systems query reservoir levels directly?

4. **Coverage Grid Canon Update:** Need to add FluidCoverageGrid to `dense_grid_exception.applies_to` in patterns.yaml (like we did for CoverageGrid in Epic 5).

### For Game Designer

1. **Reservoir Sizing:** What should reservoir capacities be relative to consumption? e.g., "one reservoir holds enough for 50 buildings for 100 ticks"

2. **Water Distance Balance:**
   - Placement distance: 8 tiles (can place)
   - Operational distance: 5 tiles (produces at >50% efficiency)
   - Adjacent bonus: Full efficiency only at 0-1 tiles from water?

3. **No Rationing Decision:** Confirm that fluid has no priority rationing (unlike energy). During deficit, ALL buildings lose water equally?

4. **Power Loss Cascade:** Is it intentional that power loss -> extractor offline -> potential water crisis? This creates interesting cascade scenarios but might be frustrating.

5. **Extractor Types:** MVP has one extractor type. Should we reserve space for future types (deep well, desalination, etc.)?

---

## Work Items

### Infrastructure (Foundational)

1. Define FluidPoolState enum (Healthy, Marginal, Deficit, Collapse)
2. Define FluidComponent struct (12 bytes, consumer)
3. Define FluidProducerComponent struct (12 bytes, extractor)
4. Define FluidReservoirComponent struct (16 bytes, storage)
5. Define FluidConduitComponent struct (4 bytes, pipe)
6. Define PerPlayerFluidPool struct (~48 bytes, aggregated state)
7. Implement FluidCoverageGrid class (dense 2D array)
8. Define fluid event types (FluidStateChangedEvent, DeficitBeganEvent, etc.)

### Core System

9. Implement FluidSystem class skeleton (ISimulatable, priority 20)
10. Implement IFluidProvider interface (if approved by Systems Architect)
11. Integrate IEnergyProvider for extractor power checks
12. Integrate ITerrainQueryable for water proximity checks

### Coverage System

13. Implement BFS flood-fill for fluid coverage (from operational extractors/reservoirs)
14. Implement coverage dirty flag tracking
15. Implement ownership boundary enforcement
16. Implement coverage queries (is_in_coverage, get_coverage_at)

### Pool Calculation

17. Implement extractor registration and operational state tracking
18. Implement consumer registration and aggregation
19. Implement reservoir registration and level tracking
20. Implement pool calculation (generation + stored = available, consumption, surplus)
21. Implement pool state machine (Healthy/Marginal/Deficit/Collapse)
22. Implement reservoir fill/drain logic

### Distribution

23. Implement fluid distribution (consumers in coverage get fluid if pool healthy)
24. Implement FluidStateChangedEvent emission
25. Implement pool state transition events

### Extractor Mechanics

26. Implement water proximity validation using ITerrainQueryable.get_water_distance
27. Implement water factor calculation (efficiency based on distance)
28. Implement extractor placement validation (bounds, ownership, terrain, proximity)

### Conduit Mechanics

29. Implement fluid conduit placement and validation
30. Implement conduit connection detection (via BFS)
31. Implement conduit removal and coverage update
32. Implement conduit active state for rendering (flow visualization)
33. Implement conduit placement preview (coverage delta)

### Integration

34. Implement BuildingConstructedEvent handler (register consumer/producer)
35. Implement BuildingDeconstructedEvent handler (unregister)
36. Network serialization for FluidComponent
37. Network serialization for pool state
38. Integration with BuildingSystem (replace stub if exists)

### Content

39. Define fluid requirements per structure template
40. Define extractor base stats (output, placement distance, operational distance)
41. Define reservoir stats (capacity, fill/drain rates)
42. Define conduit stats (coverage radius, cost)
43. Balance deficit/collapse thresholds

### Testing

44. Unit tests for all components
45. Integration test suite (full pipeline)
46. Multiplayer sync verification tests
47. Performance benchmark suite

### Documentation

48. FluidSystem technical documentation
49. Fluid balance design document

---

## Appendix: Component Size Summary

| Component | Size | Notes |
|-----------|------|-------|
| FluidComponent | 12 bytes | Matches EnergyComponent |
| FluidProducerComponent | 12 bytes | No aging, no contamination |
| FluidReservoirComponent | 16 bytes | Storage buffer |
| FluidConduitComponent | 4 bytes | Matches EnergyConduitComponent |
| PerPlayerFluidPool | ~48 bytes | Per-overseer aggregate |
| FluidCoverageGrid | 1 byte/cell | Dense 2D array |

---

## Appendix: Key Differences from Epic 5 (EnergySystem)

| Aspect | EnergySystem (Epic 5) | FluidSystem (Epic 6) |
|--------|----------------------|---------------------|
| Producers | Energy nexuses | Fluid extractors |
| Storage | None | Reservoirs |
| Placement Constraint | Terrain only | Water proximity required |
| Power Dependency | Self-powered | Requires external power |
| Aging/Degradation | Yes (asymptotic) | No |
| Contamination Output | Some nexus types | None |
| Priority Rationing | Yes (4 levels) | No (all-or-nothing) |
| Deficit Handling | Priority-based cutoff | Reservoir drain, then cutoff |
| ISimulatable Priority | 10 | 20 (runs after energy) |

---

**Document Status:** Ready for Discussion
**Next Steps:** Review with Systems Architect and Game Designer before ticket creation
