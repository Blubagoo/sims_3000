# Decision: Epic 4 Canon Updates

**Date:** 2026-01-28
**Status:** accepted
**Epic:** 4
**Tickets:** 4-007, 4-034, 4-035, 4-036, 4-037, 4-048

## Context

During Epic 4 planning verification, the Systems Architect identified that several aspects of the Epic 4 design are more thorough than current canon documentation. These updates ensure downstream epics (5, 6, 7, 9, 10, 11, 13) can rely on stable interface definitions and accurate dependency graphs.

## Updates Required

### 1. Add BuildingGrid to dense_grid_exception (patterns.yaml)

**Current Canon:** `dense_grid_exception.applies_to` lists only TerrainGrid.

**Update:** Add BuildingGrid with same rationale:
```yaml
applies_to:
  - "TerrainGrid (Epic 3): 4 bytes/tile dense 2D array for terrain height, type, properties"
  - "BuildingGrid (Epic 4): 4 bytes/tile dense EntityID array for O(1) spatial occupancy lookups"
```

**Rationale:** Same pattern as TerrainGrid. Per-tile occupancy data, O(1) lookups required for spawn validation and query APIs.

---

### 2. Add New Query Interfaces (interfaces.yaml)

Add three new interfaces under new sections:

**a) IZoneQueryable (under `zoning:` section)**
```yaml
zoning:
  IZoneQueryable:
    description: "Read-only queries for zone data"
    purpose: "Allow BuildingSystem and downstream systems to query zone state without direct ZoneSystem coupling"
    methods:
      - name: get_zone_type
        params:
          - {name: x, type: int32_t}
          - {name: y, type: int32_t}
        returns: "std::optional<ZoneType>"
        description: "Get zone type at position, empty if not zoned"
      - name: get_zone_density
        params:
          - {name: x, type: int32_t}
          - {name: y, type: int32_t}
        returns: "std::optional<ZoneDensity>"
        description: "Get zone density at position, empty if not zoned"
      - name: is_zoned
        params:
          - {name: x, type: int32_t}
          - {name: y, type: int32_t}
        returns: bool
        description: "Check if position is zoned"
      - name: get_zone_count
        params:
          - {name: player_id, type: PlayerID}
          - {name: type, type: ZoneType}
        returns: uint32_t
        description: "Get count of zones of type for player"
      - name: get_demand
        params:
          - {name: type, type: ZoneType}
          - {name: player_id, type: PlayerID}
        returns: float
        description: "Get demand (growth pressure) for zone type [-100 to +100]"
    implemented_by:
      - ZoneSystem
```

**b) IBuildingQueryable (under `buildings:` section)**
```yaml
buildings:
  IBuildingQueryable:
    description: "Read-only queries for building data"
    purpose: "Allow downstream systems to query building state, capacity, occupancy"
    methods:
      - name: get_building_at
        params:
          - {name: x, type: int32_t}
          - {name: y, type: int32_t}
        returns: EntityID
        description: "Get building entity at position, INVALID_ENTITY if none"
      - name: is_tile_occupied
        params:
          - {name: x, type: int32_t}
          - {name: y, type: int32_t}
        returns: bool
        description: "Check if tile is occupied by a building"
      - name: get_building_state
        params:
          - {name: entity, type: EntityID}
        returns: BuildingState
        description: "Get current state of a building"
      - name: get_total_capacity
        params:
          - {name: type, type: ZoneBuildingType}
          - {name: owner, type: PlayerID}
        returns: uint32_t
        description: "Total capacity across all buildings of type for player"
      - name: get_total_occupancy
        params:
          - {name: type, type: ZoneBuildingType}
          - {name: owner, type: PlayerID}
        returns: uint32_t
        description: "Current total occupancy for type"
    implemented_by:
      - BuildingSystem

  IBuildingTemplateQuery:
    description: "Read-only access to building template data"
    purpose: "Allow EnergySystem, FluidSystem, PopulationSystem to query template requirements"
    methods:
      - name: get_template
        params:
          - {name: template_id, type: uint32_t}
        returns: "const BuildingTemplate&"
        description: "Get template by ID"
      - name: get_energy_required
        params:
          - {name: template_id, type: uint32_t}
        returns: uint16_t
        description: "Energy units required per tick"
      - name: get_fluid_required
        params:
          - {name: template_id, type: uint32_t}
        returns: uint16_t
        description: "Fluid units required per tick"
    implemented_by:
      - BuildingTemplateRegistry
```

---

### 3. Update BuildingSystem depends_on (systems.yaml)

**Current Canon:**
```yaml
depends_on:
  - ZoneSystem (triggers spawning)
  - EnergySystem (affects building state)
  - FluidSystem (affects building state)
  - TransportSystem (road proximity rule)
```

**Update to:**
```yaml
depends_on:
  - ZoneSystem (triggers spawning, zone type/density queries)
  - EnergySystem (affects building state -- stub in Epic 4)
  - FluidSystem (affects building state -- stub in Epic 4)
  - TransportSystem (pathway proximity rule -- stub in Epic 4)
  - LandValueSystem (template selection filtering -- stub in Epic 4)
  - DemandSystem (spawn gating -- basic impl in Epic 4, full in Epic 10)
  - EconomySystem (construction/demolition costs -- stub in Epic 4)
```

**Rationale:** More accurate dependency graph. Notes which dependencies are stubs vs. basic implementations.

---

### 4. Add Forward Dependency Stub Pattern (patterns.yaml)

Add new section documenting the stub pattern:

```yaml
forward_dependencies:
  overview: |
    When an epic depends on systems from later epics, use forward dependency
    interfaces with stub implementations. This allows development to proceed
    without blocking on downstream epics.

  stub_pattern:
    description: "Abstract interface + stub implementation + setter injection"
    rules:
      - "Define abstract interface for the dependency (e.g., IEnergyProvider)"
      - "Create stub implementation returning permissive defaults"
      - "Use setter injection to swap stub for real implementation"
      - "Include debug toggle for testing restrictive scenarios"

    example: |
      // Interface
      class IEnergyProvider {
      public:
          virtual bool is_powered(EntityID entity) const = 0;
      };

      // Stub (permissive default)
      class StubEnergyProvider : public IEnergyProvider {
      public:
          bool is_powered(EntityID) const override { return true; }
      };

      // Consumer uses setter injection
      class BuildingSystem {
          IEnergyProvider* energy_provider_;
      public:
          void set_energy_provider(IEnergyProvider* p) { energy_provider_ = p; }
      };

  applies_to:
    - "Epic 4: IEnergyProvider, IFluidProvider, ITransportProvider, ILandValueProvider, IDemandProvider, ICreditProvider"
    - "Future epics may add additional forward dependency stubs"
```

---

### 5. Add Building State Terminology (terminology.yaml)

Add building state lifecycle terms:

```yaml
building_states:
  construction: materializing
  operational: active
  utilities_lost: abandoned
  terminal_decay: derelict
  demolished: deconstructed
```

---

## Decision

All updates ACCEPTED. These are documentation updates that make canon more complete and accurate. The ticket specifications are authoritative -- canon is catching up.

## Consequences

- Canon files to update: patterns.yaml, interfaces.yaml, systems.yaml, terminology.yaml
- Other epics affected: None (these updates document what Epic 4 tickets already specify)
- Interface lock: IZoneQueryable, IBuildingQueryable, IBuildingTemplateQuery signatures are now locked for downstream epics

## Implementation

Apply these updates using `/update-canon` skill after this decision is recorded.
