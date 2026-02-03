# Epic 6: Canon Verification Report

**Verified By:** Systems Architect Agent
**Date:** 2026-01-28
**Canon Version:** 0.7.0
**Status:** PASS_WITH_UPDATES

## Verification Summary

| Category | Status | Notes |
|----------|--------|-------|
| System Boundaries | PASS | FluidSystem owns exactly what canon defines |
| Interfaces | PASS_WITH_UPDATES | IFluidConsumer/IFluidProducer match canon; IFluidProvider needs to be added |
| Component Sizes | PASS | All sizes justified and appropriate |
| Tick Priority | PASS | Priority 20 correctly follows EnergySystem (10) |
| Terminology | PASS | Canonical alien terms used throughout |
| Patterns | PASS_WITH_UPDATES | Pool model matches; FluidCoverageGrid needs dense_grid_exception entry |
| Dependencies | PASS | All cross-epic dependencies correctly identified |

## Detailed Findings

### System Boundaries

**Status: PASS**

The tickets correctly scope FluidSystem to own only what canon defines in `systems.yaml`:

**Canon (systems.yaml epic_6_water):**
- Fluid extractor (pump) simulation
- Fluid reservoir (tower) storage
- Fluid pool calculation (per player)
- Fluid coverage zone calculation
- Fluid distribution to buildings

**Does NOT own (correctly excluded):**
- Building water consumption amounts (buildings define)
- Water UI (UISystem owns)

**Verification:**
- Ticket 6-009 correctly establishes FluidSystem with ISimulatable interface
- Tickets 6-014, 6-015, 6-016 handle extractor, reservoir, and consumer mechanics
- Ticket 6-017, 6-018, 6-019 handle pool calculation and distribution
- Ticket 6-042 correctly defers UI to Epic 12 (documents requirements only)

No boundary violations detected.

### Interfaces

**Status: PASS_WITH_UPDATES**

**IFluidConsumer (canon interfaces.yaml):**

Canon signature:
```yaml
methods:
  - get_fluid_required() -> uint32
  - on_fluid_state_changed(has_fluid: bool) -> void
component_requirements:
  - FluidComponent
  - PositionComponent
```

Ticket verification:
- Ticket 6-002 defines FluidComponent with `fluid_required`, `fluid_received`, `has_fluid` fields - **MATCHES**
- Ticket 6-021 implements state change event emission - **MATCHES**
- No priority field as specified in CCR-002 (differs from IEnergyConsumer) - **CORRECT per design decision**

**IFluidProducer (canon interfaces.yaml):**

Canon signature:
```yaml
methods:
  - get_fluid_output() -> uint32
  - get_reservoir_level() -> uint32
implemented_by:
  - Fluid extractor (pump)
  - Fluid reservoir (tower)
```

Ticket verification:
- Ticket 6-003 (FluidProducerComponent) has `current_output` field - **MATCHES**
- Ticket 6-004 (FluidReservoirComponent) has `current_level` field - **MATCHES**
- Both producer types correctly distinguished via FluidProducerType enum

**IFluidProvider (MISSING from canon):**

Tickets define an IFluidProvider interface (6-038) that mirrors IEnergyProvider:
- `has_fluid(EntityID) -> bool`
- `has_fluid_at(x, y, owner) -> bool`
- `get_pool_state(owner) -> FluidPoolState`
- `get_pool(owner) -> const PerPlayerFluidPool&`

**Canon Update Required:** IFluidProvider interface should be added to interfaces.yaml (flagged in tickets.md Canon Update Flags section).

### Component Sizes

**Status: PASS**

All component sizes are justified:

| Component | Size | Justification |
|-----------|------|---------------|
| FluidComponent | 12 bytes | uint32 required, uint32 received, bool has_fluid, 3 bytes padding |
| FluidProducerComponent | 12 bytes | 2x uint32 output fields, 4 bytes flags/distance (simpler than Energy's 24 bytes - no aging) |
| FluidReservoirComponent | 16 bytes | 2x uint32 capacity/level, 2x uint16 rates, 4 bytes flags (unique to fluid) |
| FluidConduitComponent | 4 bytes | Identical to EnergyConduitComponent - good pattern reuse |

All components use trivially copyable types with static_assert size verification. Memory usage aligns with Epic 5 patterns.

### Tick Priority

**Status: PASS**

**Canon (interfaces.yaml ISimulatable):**
```yaml
implemented_by:
  - EnergySystem (priority: 10)
  - FluidSystem (priority: 20)
  - ZoneSystem (priority: 30)
  - BuildingSystem (priority: 40)
```

**Ticket 6-009 specifies:**
- `get_priority() returns 20`
- Rationale: Fluid runs after energy (extractors need power state resolved), before zones/buildings

This matches canon and the design decision CCR-004 (power dependency requires EnergySystem to complete first).

### Terminology

**Status: PASS**

Cross-reference with terminology.yaml:

| Human Term | Canonical Term | Ticket Usage |
|------------|----------------|--------------|
| water | fluid | All tickets use "fluid" |
| water_pump | fluid_extractor | Tickets use "extractor" |
| water_tower | fluid_reservoir | Tickets use "reservoir" |
| pipes | fluid_conduits | Tickets use "conduit" |
| water_system | fluid_network | Tickets use "fluid network" |
| blackout (equivalent) | (none for fluid) | N/A - fluid has "collapse" state |

**Verified correct usage in tickets:**
- FluidSystem (not WaterSystem)
- FluidComponent (not WaterComponent)
- FluidProducerComponent (not PumpComponent)
- FluidReservoirComponent (not TowerComponent)
- FluidConduitComponent (not PipeComponent)
- Extractor (not pump) in all descriptions
- Reservoir (not tower) in all descriptions

No terminology violations found.

### Patterns

**Status: PASS_WITH_UPDATES**

**Pool Model (patterns.yaml resources.pool_model):**

Canon:
```yaml
water:
  generator: "Fluid extractor (pump) adds to player's fluid pool"
  consumer: "Buildings draw from player's fluid pool"
  infrastructure: "Fluid conduits (pipes) define coverage zone"
  coverage_rule: "Tile must be in coverage zone AND pool must have surplus"
  required_for_development: true
```

Ticket verification:
- Ticket 6-006 (PerPlayerFluidPool) implements pool model correctly
- Ticket 6-017 aggregates generation + storage - consumption = surplus
- Ticket 6-019 applies coverage zone + pool surplus rule for distribution
- Epic goal states "per-overseer fluid pools" - matches canon per-player model

**Coverage Zone Pattern:**
- Ticket 6-010 implements BFS flood-fill from operational producers through conduits
- Ticket 6-012 enforces ownership boundaries (conduits don't connect across overseer boundaries)
- Matches canon `infrastructure_connectivity.pipes.connects_across_ownership: false`

**Dense Grid Exception (patterns.yaml):**

Canon `dense_grid_exception.applies_to`:
- TerrainGrid (Epic 3)
- BuildingGrid (Epic 4)
- CoverageGrid (Epic 5)

Ticket 6-008 creates FluidCoverageGrid:
- 1 byte per cell (same as CoverageGrid)
- Dense 2D array storage
- O(1) lookups
- Meets all conditions for dense_grid_exception

**Canon Update Required:** Add FluidCoverageGrid to dense_grid_exception.applies_to list (flagged in tickets.md).

**Event Pattern:**

All events follow canonical `{Subject}{Action}Event` pattern:
- FluidStateChangedEvent
- FluidDeficitBeganEvent / FluidDeficitEndedEvent
- FluidCollapseBeganEvent / FluidCollapseEndedEvent
- FluidConduitPlacedEvent / FluidConduitRemovedEvent
- ExtractorPlacedEvent / ExtractorRemovedEvent
- ReservoirPlacedEvent / ReservoirRemovedEvent

### Dependencies

**Status: PASS**

**External Dependencies (correctly identified in tickets):**

| Dependency | Epic | Interface | Used By |
|------------|------|-----------|---------|
| ITerrainQueryable | Epic 3 | get_water_distance(), is_buildable() | 6-014 (extractor output), 6-027 (placement validation) |
| IEnergyProvider | Epic 5 | is_powered() | 6-014, 6-026 (extractor power dependency) |
| BuildingConstructedEvent | Epic 4 | Event subscription | 6-034 (consumer/producer registration) |
| BuildingDeconstructedEvent | Epic 4 | Event subscription | 6-035 (unregistration) |
| IFluidProvider stub (4-020) | Epic 4 | Stub replacement | 6-038 (integration) |

**Dependency Graph Verification:**
- TerrainSystem (Epic 3) has no dependencies - correct ordering
- EnergySystem (Epic 5) depends on TerrainSystem - correct
- FluidSystem (Epic 6) depends on TerrainSystem AND EnergySystem - correct
- BuildingSystem (Epic 4) depends on FluidSystem - correct (buildings require fluid to develop)

**Downstream Dependents (correctly identified):**
- Epic 4 BuildingSystem: IFluidProvider.has_fluid() for development gating
- Epic 9 Services: Service structures query fluid state
- Epic 10 PopulationSystem: Fluid affects habitability
- Epic 11 EconomySystem: Extractor/reservoir costs
- Epic 12 UISystem: Fluid overlay display

All dependencies are correctly scoped and interfaces exist or are flagged for creation.

## Canon Update Requirements

The following canon updates are required based on epic planning:

| File | Change | Rationale |
|------|--------|-----------|
| interfaces.yaml | Add IFluidProvider interface | Mirror IEnergyProvider pattern; provides `has_fluid()`, `has_fluid_at()`, `get_pool_state()`, `get_pool()` methods |
| systems.yaml | Document FluidSystem tick_priority: 20 | Currently implied in interfaces.yaml but should be explicit in systems.yaml |
| patterns.yaml | Add FluidCoverageGrid to dense_grid_exception.applies_to | Follows same pattern as CoverageGrid; justified by same conditions |

**Proposed IFluidProvider interface (for interfaces.yaml):**
```yaml
IFluidProvider:
  description: "Provides fluid state queries for other systems"
  purpose: "Allows BuildingSystem and other consumers to query fluid state without direct FluidSystem coupling"

  methods:
    - name: has_fluid
      params:
        - name: entity
          type: EntityID
      returns: bool
      description: "Whether entity is currently receiving fluid"

    - name: has_fluid_at
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
      returns: "const PerPlayerFluidPool&"
      description: "Full pool data for a player"

  implemented_by:
    - FluidSystem (Epic 6)

  notes:
    - "Replaces Epic 4 stub (ticket 4-020)"
    - "has_fluid_at combines coverage check + pool surplus check"
    - "No grace period (CCR-006) - reservoirs serve as buffer"
```

## Conflicts Requiring Decisions

**No conflicts found.**

All design decisions from the discussion document (CCR-001 through CCR-010) are internally consistent and align with canon principles. Key resolved decisions:

1. **CCR-002 (No priority rationing):** Fluid's all-or-nothing distribution differs from energy's 4-level priority system. This is an intentional design choice documented in tickets and does not conflict with canon (IFluidConsumer intentionally lacks priority method).

2. **CCR-006 (No grace period):** Fluid has no built-in delay like energy's 100-tick grace period. Reservoirs serve as the buffer mechanism instead. This is a design variation, not a conflict.

3. **CCR-009 (Separate coverage grids):** FluidCoverageGrid is a separate allocation from EnergyCoverageGrid. Canon supports this via dense_grid_exception pattern.

## Conclusion

Epic 6 tickets are **well-aligned with canon** and ready for implementation after the identified canon updates are applied.

**Strengths:**
- Correct system boundaries - FluidSystem owns exactly what it should
- Proper interface alignment with IFluidConsumer and IFluidProducer
- Canonical terminology used throughout (fluid, extractor, reservoir, conduit)
- Pool model correctly implements per-player pools with coverage zones
- Tick priority correctly positions fluid after energy resolution
- All dependencies properly identified with correct interfaces

**Required Actions:**
1. Apply canon updates via `/update-canon` command:
   - Add IFluidProvider to interfaces.yaml
   - Add FluidCoverageGrid to patterns.yaml dense_grid_exception
   - Optionally document tick_priority: 20 in systems.yaml

2. No decision documents required - all design choices are internally consistent

**Verification Complete.** Epic 6 tickets pass canon verification with minor updates required.
