# Systems Architect Analysis: Epic 17 - Polish & Alien Theming

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.17.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 17 is unique among all epics: **it does not introduce new systems**. Instead, it focuses on polishing and optimizing the 16 previously implemented epics into a cohesive, performant, and thematically consistent experience.

From a Systems Architect perspective, Epic 17 addresses five critical areas:

1. **Performance Optimization** - Rendering, network, memory, and CPU hotspots
2. **Technical Debt Resolution** - Code cleanup, pattern consistency, TODO resolution
3. **Integration Polish** - Cross-system edge cases, event flow, state synchronization
4. **Network Optimization** - Bandwidth reduction, latency handling, prediction tuning
5. **Testing Infrastructure** - Coverage gaps, performance benchmarks, stress testing

Additionally, this epic enforces **Core Principle #5: Alien Theme Consistency** by ensuring all user-facing text uses terminology from `terminology.yaml`.

**Key Insight:** This epic's scope is primarily **maintenance and refinement**, not feature development. Success is measured by absence of bugs, consistent framerates, low network bandwidth, and immersive alien theming.

---

## Table of Contents

1. [Performance Optimization](#1-performance-optimization)
2. [Technical Debt Resolution](#2-technical-debt-resolution)
3. [Integration Polish](#3-integration-polish)
4. [Network Optimization](#4-network-optimization)
5. [Testing Infrastructure](#5-testing-infrastructure)
6. [Alien Terminology Audit](#6-alien-terminology-audit)
7. [Key Work Items](#7-key-work-items)
8. [Questions for Other Agents](#8-questions-for-other-agents)
9. [Risks and Concerns](#9-risks-and-concerns)
10. [Dependencies](#10-dependencies)
11. [Performance Benchmarks](#11-performance-benchmarks)
12. [Canon Compliance Checklist](#12-canon-compliance-checklist)

---

## 1. Performance Optimization

### 1.1 Rendering Performance

After 16 epics, the rendering pipeline handles:
- Terrain mesh (chunked, 32x32 tiles per chunk)
- Building models (potentially thousands)
- Infrastructure (pathways, conduits, pipes)
- Vegetation instances
- UI overlays (crime, pollution, power, etc.)
- Effects (fire, bioluminescence, construction animations)

**Optimization Targets:**

| Area | Current State | Optimization Strategy |
|------|--------------|----------------------|
| LOD System | Not implemented | Distance-based model swapping (3 LOD levels) |
| Frustum Culling | Basic | Hierarchical culling with spatial hash |
| Instanced Rendering | Partial | Full instancing for buildings, vegetation, infrastructure |
| Overlay Rendering | Per-tile | Texture-based overlay with GPU upload |
| Draw Call Batching | Per-model | Material batching, merge static geometry |
| Terrain Mesh | Full rebuild on change | Incremental chunk updates only |

**LOD Distance Thresholds (512x512 map):**

```cpp
struct LODConfig {
    float lod0_distance = 50.0f;   // Full detail
    float lod1_distance = 150.0f;  // Medium detail
    float lod2_distance = 300.0f;  // Low detail (billboards for buildings)
    float cull_distance = 500.0f;  // Don't render
};
```

**Instanced Rendering Candidates:**

| Asset Type | Instance Count (Large Map) | Memory Savings |
|------------|---------------------------|----------------|
| Zone buildings | 5,000-20,000 | 80% reduction |
| Pathway tiles | 10,000-50,000 | 90% reduction |
| Vegetation | 20,000-100,000 | 95% reduction |
| Conduit segments | 5,000-20,000 | 85% reduction |

### 1.2 ECS Query Optimization

System tick profiles indicate query-heavy systems need optimization:

**Hot Systems (by tick time):**

| System | Tick Priority | Query Pattern | Optimization |
|--------|---------------|---------------|--------------|
| ContaminationSystem | 80 | All fabrication + traffic | Spatial partitioning |
| DisorderSystem | 70 | All buildings + population | Cached query results |
| LandValueSystem | 85 | Reads all grids | Grid sampling (1:4 ratio) |
| PopulationSystem | 50 | All habitation buildings | Dirty flag tracking |
| TransportSystem | 45 | All pathway entities | Incremental connectivity |

**Query Optimization Strategies:**

```cpp
// Before: Query all entities every tick
void ContaminationSystem::tick(float dt) {
    for (auto entity : registry.view<FabricationComponent>()) {
        // Process every fabrication building every tick
    }
}

// After: Spatial partitioning + dirty tracking
void ContaminationSystem::tick(float dt) {
    // Only process dirty chunks
    for (ChunkID chunk : dirty_contamination_chunks_) {
        process_chunk_contamination(chunk);
    }
    dirty_contamination_chunks_.clear();
}
```

### 1.3 Dense Grid Memory Optimization

Current dense grid allocation (from patterns.yaml):

| Grid | Bytes/Tile | 512x512 Map | Optimization |
|------|------------|-------------|--------------|
| TerrainGrid | 4 | 1 MB | None (required) |
| BuildingGrid | 4 | 1 MB | Sparse for empty tiles |
| EnergyCoverageGrid | 1 | 256 KB | None |
| FluidCoverageGrid | 1 | 256 KB | None |
| PathwayGrid | 4 | 1 MB | Sparse for empty tiles |
| ProximityCache | 1 | 256 KB | Lazy computation |
| DisorderGrid | 1 | 256 KB | None (double-buffered) |
| ContaminationGrid | 2 | 512 KB | None (double-buffered) |
| LandValueGrid | 2 | 512 KB | None |
| ServiceCoverageGrid | 1/type | 256 KB x 4 | Combine into single grid |
| ConflagrationGrid | 3 | 768 KB | Sparse (only during disasters) |

**Optimization Opportunities:**

1. **Sparse Grids for Pathways/Buildings:** Most tiles are empty. Use sparse storage with default-zero lookup.

```cpp
// Before: Dense array
EntityID pathway_grid[512 * 512];  // 1 MB always allocated

// After: Sparse map with dense chunks
class SparseGrid<EntityID> {
    std::unordered_map<ChunkID, std::array<EntityID, CHUNK_SIZE>> chunks_;
    EntityID get(int x, int y) const {
        auto it = chunks_.find(to_chunk(x, y));
        return it != chunks_.end() ? it->second[local_index(x, y)] : INVALID_ENTITY;
    }
};
```

2. **ConflagrationGrid Lazy Allocation:** Only allocate during disasters.

3. **ServiceCoverageGrid Consolidation:** Use bit flags instead of separate grids per service type.

**Memory Budget (512x512 map):**

| Category | Current | Optimized | Savings |
|----------|---------|-----------|---------|
| Dense Grids | ~6 MB | ~3 MB | 50% |
| Entity Components | ~10 MB | ~8 MB | 20% |
| Render State | ~50 MB | ~30 MB | 40% |
| **Total Runtime** | ~66 MB | ~41 MB | **38%** |

### 1.4 Tick Rate Stability

Canon specifies 20 ticks/second (50ms per tick). Ensure stability:

**Tick Budget Analysis:**

| System Category | Budget | Typical | Peak |
|-----------------|--------|---------|------|
| Terrain | 1 ms | 0.2 ms | 0.5 ms |
| Energy + Fluid | 3 ms | 2 ms | 4 ms |
| Zones + Buildings | 5 ms | 3 ms | 8 ms |
| Transport | 5 ms | 4 ms | 10 ms |
| Population | 3 ms | 2 ms | 5 ms |
| Demand + Economy | 3 ms | 1.5 ms | 3 ms |
| Services | 3 ms | 2 ms | 4 ms |
| Disorder + Contamination | 5 ms | 3 ms | 8 ms |
| Disasters | 2 ms | 0.5 ms | 15 ms* |
| Land Value | 5 ms | 3 ms | 6 ms |
| Progression | 1 ms | 0.2 ms | 0.5 ms |
| Sync | 5 ms | 3 ms | 8 ms |
| **Total** | **50 ms** | **24 ms** | **72 ms*** |

*Peak during active disaster - acceptable occasional spike.

**Mitigation for Peak Overruns:**

```cpp
// Spread expensive calculations across multiple ticks
class LandValueSystem : public ISimulatable {
    uint32_t current_row_ = 0;
    static constexpr uint32_t ROWS_PER_TICK = 32;  // Process 32 rows/tick

    void tick(float dt) override {
        uint32_t end_row = std::min(current_row_ + ROWS_PER_TICK, map_height_);
        for (uint32_t y = current_row_; y < end_row; ++y) {
            calculate_row_land_values(y);
        }
        current_row_ = (end_row >= map_height_) ? 0 : end_row;
    }
};
```

---

## 2. Technical Debt Resolution

### 2.1 Code Pattern Consistency

After 16 epics implemented by various agent profiles, ensure consistent patterns:

**Pattern Audit Checklist:**

| Pattern | Standard | Common Violations |
|---------|----------|-------------------|
| Component naming | `{Concept}Component` | Missing suffix, wrong casing |
| System naming | `{Concept}System` | Inconsistent with canon |
| Interface naming | `I{Concept}` | Missing prefix |
| Event naming | `{Subject}{Action}Event` | Wrong order |
| Snake_case fields | `energy_required` | camelCase leakage |
| RAII for resources | Constructor/destructor | Manual new/delete |
| Optional for nullable | `std::optional<T>` | Raw pointers |
| Entity references | `EntityID` | Raw pointers to components |

**Refactoring Work Items:**

| Category | Estimated Count | Effort |
|----------|-----------------|--------|
| Naming inconsistencies | 30-50 | Low |
| Missing interfaces | 5-10 | Medium |
| Pattern violations | 20-30 | Medium |
| Memory management | 5-10 | High |

### 2.2 TODO/FIXME Resolution

Scan codebase for unresolved markers:

```bash
# Expected categories:
# TODO: Feature incomplete
# FIXME: Known bug
# HACK: Temporary workaround
# OPTIMIZE: Performance improvement needed
```

**Resolution Strategy:**

| Priority | Criteria | Action |
|----------|----------|--------|
| Critical | Affects correctness | Fix immediately |
| High | Affects performance | Fix in Epic 17 |
| Medium | Code quality | Fix if time permits |
| Low | Nice-to-have | Document for future |

### 2.3 Dead Code Removal

After 16 epics, accumulated dead code includes:
- Stub implementations replaced by real systems
- Debug-only code left in release paths
- Unused component fields
- Orphaned event types

**Audit Targets:**

| Category | Detection Method | Action |
|----------|------------------|--------|
| Unused functions | Static analysis | Remove |
| Stub implementations | Grep for "Stub" prefix | Remove/archive |
| Debug code | #ifdef DEBUG audit | Verify guards |
| Unused components | ECS query coverage | Remove |

### 2.4 Error Handling Consistency

From patterns.yaml:
- Use return values or std::optional for expected failures
- Use exceptions only for programmer errors (bugs)
- Log errors with context (what, where, why)

**Audit Areas:**

| System | Error Pattern | Expected Behavior |
|--------|---------------|-------------------|
| NetworkManager | Connection failures | Reconnect with backoff |
| PersistenceSystem | Save/load failures | User notification + fallback |
| AssetManager | Missing assets | Fallback + warning |
| ECS Registry | Invalid entity | Assert in debug, silent in release |

---

## 3. Integration Polish

### 3.1 Cross-System Event Flow Issues

Known integration pain points after 16 epics:

**Event Ordering Issues:**

| Event Chain | Issue | Resolution |
|-------------|-------|------------|
| Building -> Energy -> Zone | Zone checks energy before Energy processes | Use previous tick energy state |
| Disaster -> Building -> Economy | Damage applied before economy calculates | Event queue with priorities |
| Population -> Demand -> Building | Circular dependency | Double-buffered demand values |
| Service -> Disorder -> LandValue | Circular dependency | Already resolved via double-buffering |

**Proposed Event Processing Order:**

```cpp
// Per-tick event processing
void SimulationCore::process_tick_events() {
    // Phase 1: Infrastructure events (network changes, construction complete)
    process_events<InfrastructureEvents>();

    // Phase 2: State change events (energy state, fluid state)
    process_events<StateChangeEvents>();

    // Phase 3: Simulation events (population, economy)
    process_events<SimulationEvents>();

    // Phase 4: UI/notification events (milestones, alerts)
    process_events<NotificationEvents>();
}
```

### 3.2 State Synchronization Edge Cases

**Multiplayer Sync Edge Cases:**

| Scenario | Issue | Resolution |
|----------|-------|------------|
| Player join mid-disaster | Partial fire grid sync | Full ConflagrationGrid snapshot |
| Player disconnect during transaction | Incomplete tile purchase | Transaction rollback on disconnect |
| Late join after long session | Large delta sync | Switch to full snapshot if delta > threshold |
| Simultaneous builds on adjacent tiles | Race condition | Server queues with ownership lock |
| Load save while clients connected | Client state invalidation | Full resync after load |

**Sync Priority Table:**

| Component Category | Sync Priority | Compression |
|--------------------|---------------|-------------|
| Position changes | High | Delta encoded |
| Building state | High | Full state |
| Dense grid changes | Medium | Region-based delta |
| Population stats | Low | Every 5 ticks |
| Land value changes | Low | Every 10 ticks |

### 3.3 Interface Mismatches

After implementing all interfaces, verify contracts:

| Interface | Provider | Consumers | Verification Needed |
|-----------|----------|-----------|---------------------|
| IEnergyProvider | EnergySystem | BuildingSystem, UISystem | Return type consistency |
| IFluidProvider | FluidSystem | BuildingSystem, UISystem | Return type consistency |
| ITransportProvider | TransportSystem | BuildingSystem, UISystem | Method signatures |
| ITerrainQueryable | TerrainSystem | All systems | Thread safety for render |
| IDisasterProvider | DisasterSystem | UISystem, RenderingSystem | Active disaster lifecycle |
| IProgressionProvider | ProgressionSystem | UISystem, BuildingSystem | Milestone permanence |
| IPersistenceProvider | PersistenceSystem | Application | Error handling paths |
| IAudioProvider | AudioSystem | All systems | Null check for server |

---

## 4. Network Optimization

### 4.1 Bandwidth Analysis

Current estimated bandwidth (4 players, 512x512 map):

| Message Type | Frequency | Size | Bandwidth |
|--------------|-----------|------|-----------|
| State delta | 20/sec | 2-10 KB | 40-200 KB/s |
| Input messages | 30/sec | 50 B | 1.5 KB/s |
| Chat messages | 0.1/sec | 200 B | 0.02 KB/s |
| Full snapshot | On connect | 500 KB | One-time |
| Cursor sync | 10/sec | 20 B | 0.2 KB/s |

**Target Bandwidth:** <100 KB/s per client sustained

### 4.2 Delta Compression Improvements

**Current Delta Encoding:**
- Per-component dirty flag
- Full component serialization when dirty

**Improved Delta Encoding:**

```cpp
// Field-level delta for frequently changing components
struct PositionComponentDelta {
    uint8_t changed_fields;  // Bit flags
    // Only include fields with corresponding bit set
    std::optional<int32_t> grid_x;
    std::optional<int32_t> grid_y;
    std::optional<int32_t> elevation;
};

// For dense grids: region-based delta
struct GridRegionDelta {
    uint16_t region_x, region_y;  // 16x16 region coordinates
    uint16_t region_size;         // Usually 16x16
    std::vector<uint8_t> data;    // LZ4 compressed region
};
```

**Expected Bandwidth Reduction:**

| Optimization | Savings |
|--------------|---------|
| Field-level delta | 30-40% |
| Grid region delta | 50-60% |
| LZ4 compression | 70-80% |
| Adaptive send rate | 20-30% |
| **Combined** | **~85%** |

### 4.3 Message Batching

**Current:** Each state change = separate message
**Optimized:** Batch changes within tick

```cpp
class SyncSystem {
    MessageBatch current_batch_;

    void on_component_changed(ComponentChange change) {
        current_batch_.add(change);
    }

    void tick_end() {
        if (current_batch_.size() > 0) {
            network_.send(current_batch_.compress());
            current_batch_.clear();
        }
    }
};
```

### 4.4 Prediction and Interpolation Tuning

Canon specifies "No client-side prediction (simplicity over responsiveness)."

However, for visual smoothness:

**Interpolation Settings:**

| Entity Type | Interpolation | Smoothing Factor |
|-------------|---------------|------------------|
| Buildings | None (static) | N/A |
| Construction progress | Linear | 0.5 |
| Cosmetic beings | Linear | 0.8 |
| Cosmetic vehicles | Linear | 0.9 |
| Camera following | Ease-out | 0.7 |
| UI values | Animated | 0.3 |

**Interpolation Implementation:**

```cpp
Vec3 interpolate_position(
    const PositionComponent& prev,
    const PositionComponent& curr,
    float alpha,
    float smoothing
) {
    // Apply smoothing to alpha
    float smooth_alpha = 1.0f - std::pow(1.0f - alpha, smoothing * 3.0f);
    return Vec3::lerp(to_world(prev), to_world(curr), smooth_alpha);
}
```

---

## 5. Testing Infrastructure

### 5.1 Integration Test Coverage Gaps

After 16 epics, verify cross-system integration:

| Test Category | Coverage | Gap Areas |
|---------------|----------|-----------|
| Energy <-> Building | Good | Brownout cascade |
| Fluid <-> Building | Good | Reservoir buffer edge cases |
| Transport <-> Building | Medium | Multi-player pathway merge |
| Disaster <-> Services | Low | Emergency response effectiveness |
| Progression <-> Economy | Low | Edict cost calculations |
| Save <-> Load | Medium | Migration across all versions |
| Audio <-> Events | Low | All event types trigger sounds |

**Required Integration Tests:**

| Test | Systems Involved | Priority |
|------|------------------|----------|
| Full simulation cycle | All | Critical |
| Multiplayer sync accuracy | Sync, Network, All | Critical |
| Disaster damage + recovery | Disaster, Building, Service | High |
| Progression milestone chain | Progression, Population, Economy | High |
| Save/load round-trip | Persistence, All | High |
| Theme terminology consistency | UI, All | Medium |

### 5.2 Performance Benchmarks

**Benchmark Suite Requirements:**

| Benchmark | Target | Measurement |
|-----------|--------|-------------|
| Tick time (empty map) | <5 ms | 99th percentile |
| Tick time (full 512x512) | <45 ms | 99th percentile |
| Frame time (60 FPS) | <16.67 ms | 99th percentile |
| Memory (512x512 map) | <100 MB | Peak usage |
| Network bandwidth | <100 KB/s | Per client |
| Save time (512x512) | <2 sec | 95th percentile |
| Load time (512x512) | <3 sec | 95th percentile |
| Asset load time | <5 sec | Cold start |

**Automated Benchmark Framework:**

```cpp
class BenchmarkSuite {
public:
    void run_all() {
        benchmark("tick_empty", [this]() { run_tick_benchmark(MapSize::Empty); });
        benchmark("tick_small", [this]() { run_tick_benchmark(MapSize::Small); });
        benchmark("tick_medium", [this]() { run_tick_benchmark(MapSize::Medium); });
        benchmark("tick_large", [this]() { run_tick_benchmark(MapSize::Large); });
        benchmark("frame_render", [this]() { run_render_benchmark(); });
        benchmark("network_sync", [this]() { run_sync_benchmark(); });
        benchmark("save_load", [this]() { run_persistence_benchmark(); });
    }

    void report_results(const std::string& output_path);
};
```

### 5.3 Stress Testing

**Stress Test Scenarios:**

| Scenario | Configuration | Success Criteria |
|----------|---------------|------------------|
| Maximum entities | 50,000 buildings | 60 FPS, <50ms tick |
| Maximum players | 4 players, full activity | <200 KB/s total |
| Rapid input | 100 actions/second | No dropped inputs |
| Long session | 8 hours continuous | No memory growth |
| Network packet loss | 10% packet loss | Graceful degradation |
| Server restart | Mid-session restart | Clean reconnection |
| Disaster cascade | 5 simultaneous disasters | No crash, <100ms tick |

### 5.4 Test Data Generation

**Procedural Test World Generation:**

```cpp
class TestWorldGenerator {
public:
    void generate(const TestWorldConfig& config) {
        // Generate terrain
        generate_terrain(config.map_size, config.terrain_seed);

        // Place infrastructure
        place_pathways(config.pathway_density);
        place_conduits(config.conduit_density);

        // Zone and develop
        zone_areas(config.zone_distribution);
        develop_buildings(config.building_density);

        // Populate
        set_population(config.population);

        // Age the world
        simulate_ticks(config.simulation_ticks);
    }
};
```

---

## 6. Alien Terminology Audit

### 6.1 Terminology Mapping Completeness

From `terminology.yaml`, verify all user-facing strings use canonical terms:

**Critical Term Categories:**

| Category | Human Term | Alien Term | Usage Count* |
|----------|------------|------------|--------------|
| World | city | colony | ~50 |
| World | citizen | being | ~30 |
| World | mayor | overseer | ~10 |
| Zones | residential | habitation | ~40 |
| Zones | commercial | exchange | ~40 |
| Zones | industrial | fabrication | ~40 |
| Infrastructure | power | energy | ~100 |
| Infrastructure | water | fluid | ~80 |
| Infrastructure | road | pathway | ~60 |
| Services | police | enforcers | ~20 |
| Services | fire_department | hazard_response | ~20 |
| Economy | money | credits | ~50 |
| Economy | taxes | tribute | ~30 |
| Simulation | happiness | harmony | ~25 |
| Simulation | crime | disorder | ~25 |
| Simulation | pollution | contamination | ~25 |
| Time | year | cycle | ~40 |
| Time | month | phase | ~20 |
| UI | classic_mode | legacy_interface | ~5 |
| UI | holographic_mode | holo_interface | ~5 |

*Estimated usage across UI, tooltips, notifications, and logs.

### 6.2 String Externalization

All user-facing strings should be externalized for:
1. Terminology consistency
2. Future localization
3. Easy audit and update

**String Table Format:**

```yaml
# strings/en/ui.yaml
panel_titles:
  budget: "Colony Treasury"
  population: "Colony Population"
  zones: "Designation Panel"

button_labels:
  zone_habitation: "Designate Habitation"
  zone_exchange: "Designate Exchange"
  zone_fabrication: "Designate Fabrication"

notifications:
  building_complete: "Structure materialized"
  energy_deficit: "Energy deficit detected"
  disaster_warning: "Catastrophe warning: {disaster_type}"
```

### 6.3 Terminology Validation Tool

**Automated Terminology Checker:**

```cpp
class TerminologyValidator {
public:
    void validate_string(const std::string& str, const std::string& context) {
        for (const auto& [human_term, alien_term] : forbidden_terms_) {
            if (str.find(human_term) != std::string::npos) {
                report_violation(human_term, alien_term, context);
            }
        }
    }

    void scan_ui_strings();
    void scan_log_messages();
    void scan_tooltip_text();
    void scan_notification_text();

    void generate_report(const std::string& output_path);

private:
    std::map<std::string, std::string> forbidden_terms_ = {
        {"city", "colony"},
        {"citizen", "being"},
        {"residential", "habitation"},
        {"commercial", "exchange"},
        {"industrial", "fabrication"},
        // ... all terms from terminology.yaml
    };
};
```

---

## 7. Key Work Items

### Phase 1: Performance Optimization (12 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-001 | LOD system implementation | L | 3-level LOD for buildings and vegetation |
| E17-002 | Hierarchical frustum culling | M | Spatial hash-based culling |
| E17-003 | Instanced rendering for buildings | L | GPU instancing for zone buildings |
| E17-004 | Instanced rendering for vegetation | M | GPU instancing for all vegetation |
| E17-005 | Instanced rendering for infrastructure | M | GPU instancing for pathways/conduits |
| E17-006 | Overlay texture rendering | M | GPU-based overlay instead of per-tile |
| E17-007 | Draw call batching | M | Material-based batching |
| E17-008 | Incremental terrain mesh updates | M | Dirty chunk tracking |
| E17-009 | ECS query optimization | L | Spatial partitioning for hot systems |
| E17-010 | Tick distribution for expensive systems | M | Spread calculations across ticks |
| E17-011 | Sparse grid conversion | M | PathwayGrid, BuildingGrid sparse storage |
| E17-012 | ConflagrationGrid lazy allocation | S | Allocate only during disasters |

### Phase 2: Technical Debt Resolution (10 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-013 | Code pattern audit | M | Scan for naming/pattern violations |
| E17-014 | Pattern violation fixes | M | Fix identified violations |
| E17-015 | TODO/FIXME audit | S | Catalog all markers |
| E17-016 | Critical TODO resolution | M | Fix critical TODOs |
| E17-017 | High-priority TODO resolution | M | Fix high-priority TODOs |
| E17-018 | Dead code removal | M | Remove stubs and unused code |
| E17-019 | Error handling audit | M | Verify consistent error patterns |
| E17-020 | Error handling fixes | M | Fix inconsistent error handling |
| E17-021 | Memory leak audit | M | Verify RAII compliance |
| E17-022 | Memory leak fixes | M | Fix identified leaks |

### Phase 3: Integration Polish (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-023 | Event ordering audit | M | Document all event chains |
| E17-024 | Event ordering fixes | M | Fix identified ordering issues |
| E17-025 | Multiplayer sync edge case audit | M | Test all edge case scenarios |
| E17-026 | Multiplayer sync fixes | L | Fix identified sync issues |
| E17-027 | Interface contract verification | M | Verify all interfaces match contracts |
| E17-028 | Interface fixes | M | Fix interface mismatches |
| E17-029 | Cross-system state consistency | M | Verify state coherence |
| E17-030 | State consistency fixes | M | Fix identified inconsistencies |

### Phase 4: Network Optimization (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-031 | Field-level delta encoding | L | Component field-level deltas |
| E17-032 | Grid region delta encoding | M | Region-based grid sync |
| E17-033 | Message batching | M | Batch messages per tick |
| E17-034 | Adaptive send rate | M | Reduce rate for stable state |
| E17-035 | Interpolation tuning | M | Smooth visual interpolation |
| E17-036 | Bandwidth monitoring | M | Real-time bandwidth display |
| E17-037 | Network stress testing | M | Test under packet loss |
| E17-038 | Network optimization validation | M | Verify bandwidth targets met |

### Phase 5: Testing Infrastructure (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-039 | Integration test suite | L | Cross-system integration tests |
| E17-040 | Performance benchmark suite | L | Automated performance benchmarks |
| E17-041 | Stress test framework | M | Automated stress testing |
| E17-042 | Test world generator | M | Procedural test data |
| E17-043 | CI benchmark integration | M | Run benchmarks in CI |
| E17-044 | Performance regression detection | M | Alert on performance regression |
| E17-045 | Memory profiling integration | M | Automated memory audits |
| E17-046 | Coverage gap filling | L | Tests for identified gaps |

### Phase 6: Alien Terminology (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E17-047 | String externalization | L | Extract all UI strings |
| E17-048 | Terminology validation tool | M | Automated term checking |
| E17-049 | UI string audit | M | Verify all UI uses alien terms |
| E17-050 | Log/debug string audit | S | Verify logs use alien terms |
| E17-051 | Notification string audit | S | Verify notifications use alien terms |
| E17-052 | Final terminology verification | S | Full scan with validation tool |

---

## Work Item Summary

| Size | Count |
|------|-------|
| S | 5 |
| M | 33 |
| L | 8 |
| **Total** | **52** |

---

## 8. Questions for Other Agents

### @game-designer

1. **LOD visual impact:** At what zoom distance should buildings transition to simplified models? Should the transition be noticeable, or prioritize smoothness?

2. **Performance vs visual quality trade-off:** If we must choose between 60 FPS and maximum visual detail on large maps, which is the priority?

3. **Terminology strictness:** Should debug/developer-facing strings also use alien terminology, or only player-facing content?

4. **Loading screen content:** What should players see during the initial asset load? Tips? Alien lore? Colony statistics?

5. **Performance indicators:** Should players see FPS/ping in the UI? If so, how prominent?

6. **Terminology exceptions:** Are there any terms that should remain human (e.g., "Tutorial", "Settings", "Quit")?

### @qa-engineer

7. **Test coverage targets:** What percentage of code coverage should we target for integration tests?

8. **Performance benchmark baselines:** Should benchmarks compare against previous release, or fixed targets?

9. **Stress test duration:** How long should continuous stress tests run? 1 hour? 8 hours? 24 hours?

10. **Regression criteria:** What performance regression percentage should trigger a build failure?

11. **Platform testing matrix:** Should performance benchmarks run on multiple hardware configurations?

12. **Terminology test automation:** Should CI fail on terminology violations, or just warn?

13. **Multiplayer test configuration:** How many concurrent sessions should the stress test support?

14. **Save compatibility testing:** How many prior versions should we maintain save compatibility with?

---

## 9. Risks and Concerns

### 9.1 Technical Risks

**RISK: Performance Targets Unachievable (MEDIUM)**

512x512 maps with maximum entities may not hit 60 FPS on target hardware.

**Mitigation:**
- Implement configurable quality presets
- Automatic quality scaling based on FPS
- Warn users about hardware requirements for large maps

---

**RISK: Network Optimization Breaks Sync Accuracy (MEDIUM)**

Aggressive delta compression could introduce sync drift.

**Mitigation:**
- Periodic full state hash comparison
- Automatic resync on drift detection
- Comprehensive multiplayer testing

---

**RISK: Terminology Changes Break Existing Content (LOW)**

Renaming strings could break string references.

**Mitigation:**
- String IDs, not raw strings in code
- Automated string validation in CI
- String change audit before release

---

**RISK: Regression During Polish (MEDIUM)**

Optimization changes could introduce bugs.

**Mitigation:**
- Comprehensive test coverage before optimization
- Benchmark and test after each change
- Feature flags for risky optimizations

---

### 9.2 Scope Risks

**RISK: Polish Scope Creep (HIGH)**

"Polish" is inherently open-ended. Every system could be improved.

**Mitigation:**
- Fixed work item list (52 items)
- Prioritized by impact
- Time-boxed per category
- Clear "done" criteria per item

---

**RISK: Testing Reveals Major Issues (MEDIUM)**

Integration testing may uncover fundamental problems requiring significant rework.

**Mitigation:**
- Prioritize integration testing early in epic
- Reserve buffer time for fixes
- Accept some issues may carry to post-release patches

---

## 10. Dependencies

### 10.1 What Epic 17 Needs from All Previous Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 0 | All core systems | Base for optimization |
| Epic 1 | Network infrastructure | Network optimization |
| Epic 2 | Rendering pipeline | LOD, instancing, culling |
| Epic 3 | Terrain system | Grid optimization |
| Epic 4 | Zone/Building systems | Instance rendering |
| Epic 5 | Energy system | Integration testing |
| Epic 6 | Fluid system | Integration testing |
| Epic 7 | Transport system | Pathway instancing |
| Epic 8 | Port system | Integration testing |
| Epic 9 | Services system | Integration testing |
| Epic 10 | Simulation core | Tick optimization |
| Epic 11 | Economy system | Integration testing |
| Epic 12 | UI system | Terminology audit |
| Epic 13 | Disaster system | Stress testing |
| Epic 14 | Progression system | Integration testing |
| Epic 15 | Audio system | Event integration |
| Epic 16 | Persistence system | Save/load testing |

### 10.2 What Epic 17 Provides

Epic 17 is the final epic. It provides:
- Optimized, stable game
- Verified cross-system integration
- Consistent alien theming
- Performance benchmarks and baselines
- Comprehensive test suite

---

## 11. Performance Benchmarks

### 11.1 Target Hardware Specifications

**Minimum Spec:**
- CPU: 4-core @ 2.5 GHz
- RAM: 8 GB
- GPU: Integrated graphics (Intel UHD 630 equivalent)
- Network: 10 Mbps

**Recommended Spec:**
- CPU: 6-core @ 3.0 GHz
- RAM: 16 GB
- GPU: Discrete mid-range (GTX 1060 equivalent)
- Network: 50 Mbps

### 11.2 Performance Targets by Hardware

| Metric | Minimum Spec | Recommended Spec |
|--------|--------------|------------------|
| Small map FPS | 60 | 60 |
| Medium map FPS | 45 | 60 |
| Large map FPS | 30 | 60 |
| Tick stability | 95% | 99% |
| Memory usage | 2 GB | 4 GB |
| Network bandwidth | 150 KB/s | 100 KB/s |

### 11.3 Benchmark Automation

```yaml
# ci/benchmarks.yaml
benchmarks:
  tick_performance:
    script: run_tick_benchmark
    targets:
      empty_map: 5ms
      small_map: 15ms
      medium_map: 30ms
      large_map: 45ms
    failure_threshold: 1.2x  # 20% regression fails

  render_performance:
    script: run_render_benchmark
    targets:
      frame_time_p99: 16.67ms
    failure_threshold: 1.1x

  network_performance:
    script: run_network_benchmark
    targets:
      bandwidth_per_client: 100KB/s
    failure_threshold: 1.5x
```

---

## 12. Canon Compliance Checklist

### 12.1 Core Principles Verification

| Principle | Verification Method | Status |
|-----------|---------------------|--------|
| 1. Multiplayer is Foundational | All systems sync correctly | To verify |
| 2. ECS Everywhere | No logic in components | To audit |
| 3. Dedicated Server Model | Server/client separation clean | To verify |
| 4. Endless Sandbox | No win conditions exist | To verify |
| 5. Alien Theme Consistency | Terminology tool passes | To verify |
| 6. Tile-Based Ownership | Ownership verified in all systems | To verify |

### 12.2 Pattern Compliance

| Pattern | Standard | Compliance |
|---------|----------|------------|
| Component naming | `{Concept}Component` | To audit |
| System naming | `{Concept}System` | To audit |
| Interface naming | `I{Concept}` | To audit |
| Event naming | `{Subject}{Action}Event` | To audit |
| Dense grid exception | Documented grids only | To verify |
| Double-buffering | Disorder, Contamination grids | To verify |

### 12.3 Interface Contracts

| Interface | Implemented By | Contract Verified |
|-----------|----------------|-------------------|
| ISimulatable | All simulation systems | To verify |
| ISimulationTime | SimulationCore | To verify |
| IEnergyProvider | EnergySystem | To verify |
| IFluidProvider | FluidSystem | To verify |
| ITransportProvider | TransportSystem | To verify |
| ITerrainQueryable | TerrainSystem | To verify |
| ITerrainRenderData | TerrainSystem | To verify |
| ITerrainModifier | TerrainSystem | To verify |
| IServiceProvider | ServicesSystem | To verify |
| IDisasterProvider | DisasterSystem | To verify |
| IEmergencyResponseProvider | ServicesSystem | To verify |
| IProgressionProvider | ProgressionSystem | To verify |
| IUnlockPrerequisite | ProgressionSystem | To verify |
| IDemandProvider | DemandSystem | To verify |
| IGridOverlay | Multiple systems | To verify |
| IAudioProvider | AudioSystem | To verify |
| IPersistenceProvider | PersistenceSystem | To verify |
| ISettingsProvider | SettingsManager | To verify |
| ISerializable | All synced components | To verify |
| IRenderStateProvider | SyncSystem | To verify |

---

## Summary

Epic 17 (Polish & Alien Theming) is a **maintenance and refinement epic** that ensures the 16 previously implemented epics work together as a cohesive, performant, and thematically consistent game.

**Five Focus Areas:**

1. **Performance Optimization:** LOD, instancing, culling, tick distribution, sparse grids
2. **Technical Debt:** Pattern consistency, TODO resolution, dead code removal
3. **Integration Polish:** Event ordering, sync edge cases, interface verification
4. **Network Optimization:** Delta compression, batching, interpolation tuning
5. **Testing Infrastructure:** Integration tests, benchmarks, stress tests

**Alien Theming:**

Core Principle #5 requires all user-facing content to use alien terminology. This epic implements:
- String externalization
- Automated terminology validation
- Comprehensive string audits

**Work Items:** 52 items covering all focus areas

**Key Success Criteria:**
- 60 FPS on recommended hardware for all map sizes
- <100 KB/s network bandwidth per client
- <50ms tick time for 512x512 maps
- Zero terminology violations in player-facing content
- All integration tests passing
- All interfaces verified against canon contracts

---

**End of Systems Architect Analysis: Epic 17 - Polish & Alien Theming**
