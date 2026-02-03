# Epic 10: Simulation Core - Tickets

**Epic:** 10 - Simulation Core
**Date:** 2026-01-29
**Canon Version:** 0.13.0
**Discussion:** [epic-10-planning.discussion.yaml](../../discussions/epic-10-planning.discussion.yaml)

---

## Revision History

| Date | Canon Change | Summary |
|------|-------------|---------|
| 2026-01-29 | v0.9.0 | Initial ticket creation |
| 2026-01-29 | canon-verification (v0.9.0 → v0.13.0) | No changes required |
| 2026-01-29 | canon-enforcement | Fixed "Crime simulation" → "Disorder simulation" |

> **Verification Note (2026-01-29):** Verified against canon v0.13.0. ISimulationTime, IDemandProvider, IGridOverlay interfaces match interfaces.yaml. Double-buffered grids (DisorderGrid, ContaminationGrid, LandValueGrid) in dense_grid_exception pattern. Tick priorities confirmed in systems.yaml.

---

## Epic Overview

Epic 10 implements the simulation heartbeat of ZergCity - the systems that transform static buildings into a living, breathing colony. This epic contains:

- **SimulationCore**: Tick scheduling, time progression, simulation speed control
- **PopulationSystem**: Demographics, employment, migration
- **DemandSystem**: RCI zone pressure calculation
- **LandValueSystem**: Sector value per tile
- **DisorderSystem**: Disorder simulation and spread
- **ContaminationSystem**: Pollution simulation and spread

Key architectural patterns:
- Double-buffered grids for circular dependency resolution
- Aggregate population model (not individual being entities)
- Per-player population/demand; global disorder/contamination grids
- Dense grid exception pattern for tile-level data

---

## Ticket Summary

| ID | Title | Size | Priority | Dependencies |
|----|-------|------|----------|--------------|
| **Phase 1: Core Infrastructure** |
| E10-001 | SimulationCore tick scheduling | M | P0 | - |
| E10-002 | Simulation speed control | S | P0 | E10-001 |
| E10-003 | Time progression tracking | S | P0 | E10-001 |
| E10-004 | ISimulationTime interface | S | P0 | E10-003 |
| E10-005 | TickStartEvent and TickCompleteEvent | S | P0 | E10-001 |
| **Phase 2: Population System** |
| E10-010 | PopulationComponent definition | S | P0 | - |
| E10-011 | EmploymentComponent definition | S | P0 | - |
| E10-012 | BuildingOccupancyComponent definition | S | P0 | - |
| E10-013 | MigrationFactorComponent definition | S | P0 | - |
| E10-014 | PopulationSystem class skeleton | M | P0 | E10-010, E10-011 |
| E10-015 | Birth rate calculation | M | P0 | E10-014 |
| E10-016 | Death rate calculation | M | P0 | E10-014 |
| E10-017 | Natural growth application | M | P0 | E10-015, E10-016 |
| E10-018 | Age distribution evolution | M | P1 | E10-017 |
| E10-019 | Labor pool calculation | M | P0 | E10-014, E10-011 |
| E10-020 | Job market aggregation | M | P0 | E10-019 |
| E10-021 | Employment matching algorithm | L | P0 | E10-020 |
| E10-022 | Building occupancy distribution | L | P1 | E10-021, E10-012 |
| E10-023 | Unemployment effects on harmony | M | P1 | E10-021 |
| E10-024 | Attractiveness calculation | M | P0 | E10-013 |
| E10-025 | Migration in calculation | M | P0 | E10-024 |
| E10-026 | Migration out calculation | M | P0 | E10-024 |
| E10-027 | Migration application | M | P0 | E10-025, E10-026 |
| E10-028 | Life expectancy calculation | M | P1 | E10-014 |
| E10-029 | Health index system | M | P1 | E10-028 |
| E10-030 | PopulationSystem IStatQueryable | M | P1 | E10-014 |
| E10-031 | Population milestone events | S | P1 | E10-014 |
| E10-032 | Population network sync | M | P1 | E10-014 |
| **Phase 3: Demand System** |
| E10-040 | DemandComponent definition | S | P0 | - |
| E10-041 | IDemandProvider interface | S | P0 | E10-040 |
| E10-042 | DemandSystem class skeleton | M | P0 | E10-041 |
| E10-043 | Habitation demand formula | M | P0 | E10-042 |
| E10-044 | Exchange demand formula | M | P0 | E10-042 |
| E10-045 | Fabrication demand formula | M | P0 | E10-042 |
| E10-046 | Demand cap calculation | M | P0 | E10-042 |
| E10-047 | Demand factors for UI | M | P1 | E10-043, E10-044, E10-045 |
| E10-048 | DemandSystem IStatQueryable | S | P1 | E10-042 |
| E10-049 | BuildingSystem integration (replace stub) | L | P0 | E10-041 |
| **Phase 4: Dense Grids** |
| E10-060 | DisorderGrid with double-buffering | L | P0 | - |
| E10-061 | ContaminationGrid with double-buffering | L | P0 | - |
| E10-062 | LandValueGrid implementation | M | P0 | - |
| E10-063 | Grid buffer swap mechanism | M | P0 | E10-060, E10-061 |
| E10-064 | IGridOverlay interface | S | P0 | - |
| **Phase 5: Disorder System** |
| E10-070 | DisorderComponent definition | S | P0 | - |
| E10-071 | ContaminationType enum | S | P0 | - |
| E10-072 | DisorderSystem class skeleton | M | P0 | E10-060, E10-070 |
| E10-073 | Disorder generation from buildings | M | P0 | E10-072 |
| E10-074 | Land value effect on disorder (prev tick) | M | P0 | E10-073, E10-062 |
| E10-075 | Disorder spread algorithm | L | P0 | E10-060, E10-073 |
| E10-076 | Enforcer suppression integration | M | P1 | E10-072 |
| E10-077 | DisorderSystem IStatQueryable | S | P1 | E10-072 |
| E10-078 | Disorder overlay data generation | S | P1 | E10-064, E10-060 |
| E10-079 | Disorder events | S | P1 | E10-072 |
| **Phase 6: Contamination System** |
| E10-080 | ContaminationComponent definition | S | P0 | E10-071 |
| E10-081 | ContaminationSystem class skeleton | M | P0 | E10-061, E10-080 |
| E10-082 | IContaminationSource query aggregation | M | P0 | E10-081 |
| E10-083 | Industrial contamination generation | M | P0 | E10-082 |
| E10-084 | Energy contamination generation | M | P0 | E10-082 |
| E10-085 | Traffic contamination generation | M | P0 | E10-082 |
| E10-086 | Terrain contamination (blight_mires) | S | P0 | E10-081 |
| E10-087 | Contamination spread algorithm | L | P0 | E10-061, E10-083 |
| E10-088 | Contamination decay algorithm | M | P0 | E10-087 |
| E10-089 | ContaminationSystem IStatQueryable | S | P1 | E10-081 |
| E10-090 | Contamination overlay data generation | S | P1 | E10-064, E10-061 |
| E10-091 | Contamination events | S | P1 | E10-081 |
| **Phase 7: Land Value System** |
| E10-100 | LandValueSystem class skeleton | M | P0 | E10-062 |
| E10-101 | Terrain value factors | M | P0 | E10-100 |
| E10-102 | Road accessibility bonus | M | P0 | E10-100 |
| E10-103 | Disorder penalty (prev tick read) | M | P0 | E10-100, E10-063 |
| E10-104 | Contamination penalty (prev tick read) | M | P0 | E10-100, E10-063 |
| E10-105 | Full value recalculation | L | P0 | E10-101, E10-102, E10-103, E10-104 |
| E10-106 | LandValueSystem IStatQueryable | S | P1 | E10-100 |
| E10-107 | Land value overlay data generation | S | P1 | E10-064, E10-062 |
| **Phase 8: Ghost Town & Integration** |
| E10-110 | AbandonmentSystem integration | L | P1 | E10-001 |
| E10-111 | Simulation state serialization | M | P1 | E10-001 |
| E10-112 | Service coverage stubs (IServiceQueryable) | S | P0 | - |
| E10-113 | Economy stubs (IEconomyQueryable) | S | P0 | - |
| E10-114 | Epic 5 EnergySystem IContaminationSource | M | P1 | E10-082 |
| E10-115 | Epic 7 TransportSystem traffic pollution | M | P1 | E10-082 |
| **Phase 9: Testing** |
| E10-120 | Unit tests: demographics | L | P1 | E10-017, E10-027 |
| E10-121 | Unit tests: employment | L | P1 | E10-021 |
| E10-122 | Unit tests: demand formulas | L | P1 | E10-043, E10-044, E10-045 |
| E10-123 | Unit tests: disorder spread | L | P1 | E10-075 |
| E10-124 | Unit tests: contamination spread | L | P1 | E10-087 |
| E10-125 | Unit tests: land value calculation | L | P1 | E10-105 |
| E10-126 | Integration test: full simulation cycle | L | P1 | All P0 tickets |

---

## Detailed Tickets

---

### E10-001: SimulationCore tick scheduling

**Size:** M | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement the SimulationCore class that orchestrates tick scheduling for all ISimulatable systems. This is the heartbeat of the simulation.

**Acceptance Criteria:**
- [ ] SimulationCore class registers ISimulatable systems
- [ ] Systems execute in tick_priority order each frame
- [ ] tick() called with delta_time on each registered system
- [ ] Systems can be added/removed at runtime
- [ ] Frame-rate independent: accumulator pattern for consistent tick rate (20 ticks/second)

**Technical Notes:**
- Use accumulator pattern: accumulate delta_time, tick when >= 50ms
- Priority queue or sorted vector for tick order
- SimulationCore is a core system, not an ECS system itself

**Canon Reference:** systems.yaml - SimulationCore

---

### E10-002: Simulation speed control

**Size:** S | **Priority:** P0 | **Dependencies:** E10-001

**Description:**
Implement simulation speed multipliers: pause (0x), normal (1x), fast (2x), fastest (3x).

**Acceptance Criteria:**
- [ ] set_speed(SimulationSpeed) method
- [ ] get_speed() returns current speed
- [ ] Speed multiplier affects tick accumulator
- [ ] Pause completely stops ticks (accumulator doesn't accumulate)
- [ ] Speed change takes effect immediately

**Technical Notes:**
- enum SimulationSpeed { Paused, Normal, Fast, Fastest }
- Multipliers: 0, 1, 2, 3 (or configurable)
- Host player controls speed in multiplayer

**Canon Reference:** systems.yaml - SimulationCore.owns: "Simulation speed control"

---

### E10-003: Time progression tracking

**Size:** S | **Priority:** P0 | **Dependencies:** E10-001

**Description:**
Track simulation time: tick count, cycle (year equivalent), and phase (season equivalent).

**Acceptance Criteria:**
- [ ] get_current_tick() returns total ticks since game start
- [ ] get_current_cycle() returns current cycle number
- [ ] get_current_phase() returns current phase (0-3 for 4 seasons)
- [ ] Time progresses automatically with ticks
- [ ] Configurable ticks_per_phase and phases_per_cycle

**Technical Notes:**
- ~500 ticks per phase, ~2000 ticks per cycle (configurable)
- Cycle/phase are gameplay time units, not wall clock

**Canon Reference:** terminology.yaml - cycle, phase, rotation

---

### E10-004: ISimulationTime interface

**Size:** S | **Priority:** P0 | **Dependencies:** E10-003

**Description:**
Define ISimulationTime interface for systems to query simulation time without coupling to SimulationCore.

**Acceptance Criteria:**
- [ ] ISimulationTime interface defined in interfaces
- [ ] get_current_tick(), get_current_cycle(), get_current_phase()
- [ ] get_simulation_speed(), is_paused()
- [ ] SimulationCore implements ISimulationTime
- [ ] Add to canon interfaces.yaml

**Technical Notes:**
```cpp
class ISimulationTime {
public:
    virtual uint64_t get_current_tick() const = 0;
    virtual uint32_t get_current_cycle() const = 0;
    virtual uint8_t get_current_phase() const = 0;
    virtual float get_simulation_speed() const = 0;
    virtual bool is_paused() const = 0;
};
```

**Canon Reference:** NEW - add to interfaces.yaml

---

### E10-005: TickStartEvent and TickCompleteEvent

**Size:** S | **Priority:** P0 | **Dependencies:** E10-001

**Description:**
Define and emit tick lifecycle events for systems that need to synchronize with tick boundaries.

**Acceptance Criteria:**
- [ ] TickStartEvent emitted before any system ticks
- [ ] TickCompleteEvent emitted after all systems tick
- [ ] Events include tick number and delta_time
- [ ] Events dispatched via event system

**Technical Notes:**
```cpp
struct TickStartEvent {
    uint64_t tick_number;
    float delta_time;
};
struct TickCompleteEvent {
    uint64_t tick_number;
    float delta_time;
};
```

---

### E10-010: PopulationComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define PopulationComponent struct for per-player aggregate population statistics.

**Acceptance Criteria:**
- [ ] Component stores: total_beings, max_capacity
- [ ] Age distribution: youth_percent, adult_percent, elder_percent
- [ ] Rates: birth_rate_per_1000, death_rate_per_1000
- [ ] Derived: natural_growth, net_migration, growth_rate
- [ ] Quality indices: harmony_index, health_index, education_index
- [ ] Historical ring buffer for population_history (12 entries)
- [ ] Component size ~90 bytes

**Technical Notes:**
See population-engineer-analysis.md Section 2.1 for full struct definition.
Attached to player entities, not individual beings.

**Canon Reference:** systems.yaml - PopulationSystem.components

---

### E10-011: EmploymentComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define EmploymentComponent struct for per-player employment statistics.

**Acceptance Criteria:**
- [ ] Labor pool: working_age_beings, labor_force, employed_laborers, unemployed
- [ ] Job market: total_jobs, exchange_jobs, fabrication_jobs
- [ ] Rates: unemployment_rate, labor_participation
- [ ] Distribution: exchange_employed, fabrication_employed
- [ ] avg_commute_satisfaction field
- [ ] Component size ~45 bytes

**Technical Notes:**
See population-engineer-analysis.md Section 2.2 for full struct definition.

**Canon Reference:** systems.yaml - PopulationSystem.components

---

### E10-012: BuildingOccupancyComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define BuildingOccupancyComponent for individual building occupancy tracking.

**Acceptance Criteria:**
- [ ] capacity, current_occupancy fields (uint16_t)
- [ ] OccupancyState enum: Empty, UnderOccupied, NormalOccupied, FullyOccupied, Overcrowded
- [ ] state field tracking current occupancy state
- [ ] occupancy_changed_tick for rate-limiting
- [ ] Component size 9 bytes

**Technical Notes:**
- Attached to habitation, exchange, and fabrication buildings
- BuildingSystem creates component; PopulationSystem updates values

**Canon Reference:** Discussion CCR-4

---

### E10-013: MigrationFactorComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define MigrationFactorComponent for per-player migration attractiveness factors.

**Acceptance Criteria:**
- [ ] Positive factors: job_availability, housing_availability, sector_value_avg, service_coverage, harmony_level
- [ ] Negative factors: disorder_level, contamination_level, tribute_burden, congestion_level
- [ ] Computed: net_attraction, migration_pressure
- [ ] Component size ~12 bytes

**Technical Notes:**
See population-engineer-analysis.md Section 2.4 for full struct definition.

---

### E10-014: PopulationSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E10-010, E10-011

**Description:**
Implement PopulationSystem class skeleton with ISimulatable interface at tick_priority 50.

**Acceptance Criteria:**
- [ ] PopulationSystem implements ISimulatable
- [ ] tick_priority = 50
- [ ] tick() method structure with calculation phases
- [ ] Per-player population tracking
- [ ] Calculation frequency gating (demographics every 100 ticks, migration every 20 ticks)

**Technical Notes:**
```cpp
class PopulationSystem : public ISimulatable {
public:
    uint8_t get_tick_priority() const override { return 50; }
    void tick(float delta_time) override;
};
```

**Canon Reference:** systems.yaml - PopulationSystem.tick_priority: 50

---

### E10-015: Birth rate calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-014

**Description:**
Implement birth rate calculation based on population, harmony, health, and housing availability.

**Acceptance Criteria:**
- [ ] BASE_BIRTH_RATE = 0.015 (15 per 1000 per cycle)
- [ ] Modifiers: harmony (0.5-1.5x), health (0.7-1.2x), housing (0.1-1.0x)
- [ ] Overcrowded colonies have minimal births
- [ ] Minimum 1 birth if population > 0 and housing available
- [ ] birth_rate_per_1000 stored in PopulationComponent

**Technical Notes:**
See population-engineer-analysis.md Section 3.2 for formula.
Run every DEMOGRAPHIC_CYCLE_TICKS (100 ticks).

---

### E10-016: Death rate calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-014

**Description:**
Implement death rate calculation based on health, contamination, services, and age distribution.

**Acceptance Criteria:**
- [ ] BASE_DEATH_RATE = 0.008 (8 per 1000 per cycle)
- [ ] Modifiers: health (0.5-1.5x), contamination (1.0-2.0x), services (0.7-1.3x), age (0.5-2.0x)
- [ ] Deaths capped at 5% of population per cycle (sandbox rule)
- [ ] death_rate_per_1000 stored in PopulationComponent

**Technical Notes:**
See population-engineer-analysis.md Section 3.3 for formula.

---

### E10-017: Natural growth application

**Size:** M | **Priority:** P0 | **Dependencies:** E10-015, E10-016

**Description:**
Apply natural growth (births - deaths) to population.

**Acceptance Criteria:**
- [ ] Calculate births and deaths
- [ ] Apply to total_beings
- [ ] Track natural_growth statistic
- [ ] Trigger age distribution update
- [ ] Never let population go negative

**Technical Notes:**
See population-engineer-analysis.md Section 3.4.

---

### E10-018: Age distribution evolution

**Size:** M | **Priority:** P1 | **Dependencies:** E10-017

**Description:**
Evolve age cohort percentages (youth/adult/elder) over time.

**Acceptance Criteria:**
- [ ] AGING_RATE = 0.02 (2% of each cohort ages per cycle)
- [ ] Births add to youth cohort
- [ ] Deaths weighted: 70% elder, 20% adult, 10% youth
- [ ] Normalize percentages to 100%
- [ ] Age distribution affects labor force calculation

**Technical Notes:**
See population-engineer-analysis.md Section 3.5 for algorithm.

---

### E10-019: Labor pool calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-014, E10-011

**Description:**
Calculate labor pool from working-age beings and labor participation rate.

**Acceptance Criteria:**
- [ ] working_age_beings = total_beings * adult_percent
- [ ] Labor participation base 65%, modified by harmony and education
- [ ] labor_force = working_age_beings * labor_participation

**Technical Notes:**
See population-engineer-analysis.md Section 4.2.

---

### E10-020: Job market aggregation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-019

**Description:**
Aggregate total jobs from exchange and fabrication building capacities.

**Acceptance Criteria:**
- [ ] Query BuildingSystem for building capacities by zone type and owner
- [ ] exchange_jobs = sum of exchange building capacities
- [ ] fabrication_jobs = sum of fabrication building capacities
- [ ] total_jobs = exchange_jobs + fabrication_jobs
- [ ] Event-driven caching for performance

**Technical Notes:**
Use BuildingConstructedEvent/BuildingDemolishedEvent for incremental updates.

---

### E10-021: Employment matching algorithm

**Size:** L | **Priority:** P0 | **Dependencies:** E10-020

**Description:**
Match labor force to available jobs using aggregate matching algorithm.

**Acceptance Criteria:**
- [ ] max_employment = min(labor_force, total_jobs)
- [ ] Proportional distribution between exchange and fabrication
- [ ] Calculate employed_laborers, unemployed
- [ ] Calculate unemployment_rate (0-100)
- [ ] Run every tick for responsiveness

**Technical Notes:**
See population-engineer-analysis.md Section 4.4 for algorithm.

---

### E10-022: Building occupancy distribution

**Size:** L | **Priority:** P1 | **Dependencies:** E10-021, E10-012

**Description:**
Distribute population and laborers to individual buildings after aggregate calculation.

**Acceptance Criteria:**
- [ ] Distribute inhabitants to habitation buildings (fill proportionally)
- [ ] Distribute laborers to exchange buildings
- [ ] Distribute laborers to fabrication buildings
- [ ] Update BuildingOccupancyComponent.current_occupancy
- [ ] Update OccupancyState based on fill ratio

**Technical Notes:**
See population-engineer-analysis.md Section 4.5 for algorithm.
Only update active buildings (not abandoned).

---

### E10-023: Unemployment effects on harmony

**Size:** M | **Priority:** P1 | **Dependencies:** E10-021

**Description:**
Apply harmony penalties and disorder increases from high unemployment.

**Acceptance Criteria:**
- [ ] 10-15% unemployment: minor harmony penalty
- [ ] 15-25% unemployment: significant harmony penalty, increased disorder
- [ ] 25%+ unemployment: major harmony drop, disorder spike
- [ ] Effects applied gradually (not instant)

**Technical Notes:**
See population-engineer-analysis.md Section 4.6 for thresholds.

---

### E10-024: Attractiveness calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-013

**Description:**
Calculate colony attractiveness factors for migration decisions.

**Acceptance Criteria:**
- [ ] Aggregate positive factors: job availability, housing, services, harmony, sector value
- [ ] Aggregate negative factors: disorder, contamination, tribute, congestion
- [ ] Calculate net_attraction score (-100 to +100)
- [ ] Update MigrationFactorComponent

**Technical Notes:**
See population-engineer-analysis.md Section 6.2 for algorithm.

---

### E10-025: Migration in calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-024

**Description:**
Calculate migration inflow based on attractiveness and available housing.

**Acceptance Criteria:**
- [ ] BASE_MIGRATION = 50 beings per cycle at neutral attractiveness
- [ ] Scale by attraction_multiplier (0.0-2.0)
- [ ] Scale by colony size (larger colonies attract more)
- [ ] Cap by available_housing
- [ ] Multiplayer: distribute global migrants by relative attractiveness

**Technical Notes:**
See population-engineer-analysis.md Section 6.3 for algorithm.

---

### E10-026: Migration out calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-024

**Description:**
Calculate migration outflow based on desperation factors.

**Acceptance Criteria:**
- [ ] BASE_OUT_RATE = 0.5% of population per cycle
- [ ] Desperation factors: disorder > 50, contamination > 50, job availability < 30, harmony < 30
- [ ] Cap at 5% per cycle (sandbox rule)
- [ ] Never cause total exodus

**Technical Notes:**
See population-engineer-analysis.md Section 6.4 for algorithm.

---

### E10-027: Migration application

**Size:** M | **Priority:** P0 | **Dependencies:** E10-025, E10-026

**Description:**
Apply net migration to population.

**Acceptance Criteria:**
- [ ] Apply migration_in and migration_out
- [ ] Track net_migration statistic
- [ ] Update growth_rate (natural + migration)
- [ ] Emit MigrationEvent

**Technical Notes:**
Run every 20 ticks (1 second game time).

---

### E10-028: Life expectancy calculation

**Size:** M | **Priority:** P1 | **Dependencies:** E10-014

**Description:**
Calculate life expectancy as a derived statistic for UI display.

**Acceptance Criteria:**
- [ ] BASE_LIFE_EXPECTANCY = 75 cycles
- [ ] Modifiers: health (0.7-1.3x), contamination (0.6-1.0x), disorder (0.9-1.0x), education (0.95-1.1x), harmony (0.9-1.1x)
- [ ] Clamp to range 30-120 cycles
- [ ] Exposed via IStatQueryable

**Technical Notes:**
See population-engineer-analysis.md Section 7.2 for formula.

---

### E10-029: Health index system

**Size:** M | **Priority:** P1 | **Dependencies:** E10-028

**Description:**
Calculate and update health index based on medical coverage and contamination.

**Acceptance Criteria:**
- [ ] Base health: 50
- [ ] Medical coverage modifies +/- 25
- [ ] Contamination modifies up to -30
- [ ] Fluid availability modifies +/- 10
- [ ] Store in PopulationComponent.health_index

**Technical Notes:**
See population-engineer-analysis.md Section 7.3 for formula.
Uses IServiceQueryable stub until Epic 9.

---

### E10-030: PopulationSystem IStatQueryable

**Size:** M | **Priority:** P1 | **Dependencies:** E10-014

**Description:**
Implement IStatQueryable interface for population statistics.

**Acceptance Criteria:**
- [ ] get_stat_value(StatID) for: Population, GrowthRate, BirthRate, DeathRate, LifeExpectancy, UnemploymentRate, HarmonyIndex
- [ ] get_stat_history(StatID, periods) for historical data
- [ ] StatID enum defined (100-199 for population, 200-299 for employment)

**Technical Notes:**
See population-engineer-analysis.md Appendix B for StatID definitions.

---

### E10-031: Population milestone events

**Size:** S | **Priority:** P1 | **Dependencies:** E10-014

**Description:**
Emit events when population reaches milestone thresholds.

**Acceptance Criteria:**
- [ ] Milestones: 100, 500, 1000, 2000, 5000, 10000, 30000, 60000, 90000, 120000
- [ ] PopulationMilestoneEvent with player, milestone, current_population
- [ ] Only emit once per milestone (track achieved milestones)

**Technical Notes:**
```cpp
struct PopulationMilestoneEvent {
    PlayerID player;
    uint32_t milestone;
    uint32_t current_population;
};
```

---

### E10-032: Population network sync

**Size:** M | **Priority:** P1 | **Dependencies:** E10-014

**Description:**
Implement network synchronization for population statistics.

**Acceptance Criteria:**
- [ ] PopulationNetworkData struct (~14 bytes)
- [ ] EmploymentNetworkData struct (~9 bytes)
- [ ] Sync every 20 ticks (1 second)
- [ ] Server authoritative for all population data

**Technical Notes:**
See population-engineer-analysis.md Section 11.4 for serialization format.

---

### E10-040: DemandComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define DemandComponent struct for per-player RCI demand values.

**Acceptance Criteria:**
- [ ] Raw demand values: habitation_demand, exchange_demand, fabrication_demand (int8_t, -100 to +100)
- [ ] DemandFactors struct for UI breakdown
- [ ] Demand caps: habitation_cap, exchange_cap, fabrication_cap
- [ ] Component size ~40 bytes

**Technical Notes:**
See population-engineer-analysis.md Section 5.2 for struct definition.

---

### E10-041: IDemandProvider interface

**Size:** S | **Priority:** P0 | **Dependencies:** E10-040

**Description:**
Define IDemandProvider interface for demand queries.

**Acceptance Criteria:**
- [ ] get_demand(ZoneBuildingType, PlayerID) -> int8_t
- [ ] get_demand_cap(ZoneBuildingType, PlayerID) -> uint32_t
- [ ] get_demand_factors(ZoneBuildingType, PlayerID) -> DemandFactors
- [ ] has_positive_demand(ZoneBuildingType, PlayerID) -> bool

**Technical Notes:**
```cpp
class IDemandProvider {
public:
    virtual int8_t get_demand(ZoneBuildingType type, PlayerID owner) const = 0;
    virtual uint32_t get_demand_cap(ZoneBuildingType type, PlayerID owner) const = 0;
    virtual DemandComponent::DemandFactors get_demand_factors(ZoneBuildingType type, PlayerID owner) const = 0;
    virtual bool has_positive_demand(ZoneBuildingType type, PlayerID owner) const = 0;
};
```

---

### E10-042: DemandSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E10-041

**Description:**
Implement DemandSystem class skeleton with ISimulatable interface at tick_priority 52.

**Acceptance Criteria:**
- [ ] DemandSystem implements ISimulatable
- [ ] tick_priority = 52
- [ ] Implements IDemandProvider
- [ ] tick() calculates demand for all zone types per player
- [ ] Run every 5 ticks (250ms) for responsiveness

**Canon Reference:** Discussion CCR-3 - DemandSystem tick_priority 52

---

### E10-043: Habitation demand formula

**Size:** M | **Priority:** P0 | **Dependencies:** E10-042

**Description:**
Implement habitation demand calculation based on occupancy, employment, services, and tribute.

**Acceptance Criteria:**
- [ ] Population factor: +30 when > 90% occupancy, -10 when < 50%
- [ ] Employment factor: positive when jobs > labor force
- [ ] Services factor: +/- 10 based on coverage
- [ ] Tribute factor: penalty when > 10%, bonus when < 7%
- [ ] Contamination factor: penalty scaled by level
- [ ] Clamp to -100 to +100

**Technical Notes:**
See population-engineer-analysis.md Section 5.3 for formula.

---

### E10-044: Exchange demand formula

**Size:** M | **Priority:** P0 | **Dependencies:** E10-042

**Description:**
Implement exchange demand calculation based on population, employment, and transport.

**Acceptance Criteria:**
- [ ] Target ratio: 1 exchange job per 3 beings
- [ ] Under-served = high demand, over-served = negative
- [ ] Unemployment affects spending power
- [ ] High congestion reduces demand
- [ ] Tribute rate penalty

**Technical Notes:**
See population-engineer-analysis.md Section 5.4 for formula.

---

### E10-045: Fabrication demand formula

**Size:** M | **Priority:** P0 | **Dependencies:** E10-042

**Description:**
Implement fabrication demand calculation based on labor, transport, and external trade.

**Acceptance Criteria:**
- [ ] Target ratio: 1 fabrication job per 5 beings
- [ ] Labor surplus encourages industry
- [ ] Requires external connectivity (map edge roads)
- [ ] Base external demand of +5
- [ ] Fabrication tolerant of contamination

**Technical Notes:**
See population-engineer-analysis.md Section 5.5 for formula.

---

### E10-046: Demand cap calculation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-042

**Description:**
Calculate demand caps based on infrastructure capacity.

**Acceptance Criteria:**
- [ ] Habitation cap: max_capacity * energy_factor * fluid_factor
- [ ] Exchange cap: exchange_jobs * transport_factor
- [ ] Fabrication cap: fabrication_jobs * transport_factor
- [ ] Congestion reduces caps

**Technical Notes:**
See population-engineer-analysis.md Section 5.6 for formula.

---

### E10-047: Demand factors for UI

**Size:** M | **Priority:** P1 | **Dependencies:** E10-043, E10-044, E10-045

**Description:**
Track and expose demand factor breakdown for UI display.

**Acceptance Criteria:**
- [ ] DemandFactors populated during calculation
- [ ] Each factor tracked separately (population, employment, services, tribute, transport, contamination)
- [ ] Accessible via get_demand_factors()
- [ ] UI can display "Why is demand high/low"

---

### E10-048: DemandSystem IStatQueryable

**Size:** S | **Priority:** P1 | **Dependencies:** E10-042

**Description:**
Implement IStatQueryable interface for demand statistics.

**Acceptance Criteria:**
- [ ] StatIDs 300-302: HabitationDemand, ExchangeDemand, FabricationDemand
- [ ] get_stat_value returns demand value (-100 to +100)
- [ ] Optional history tracking

---

### E10-049: BuildingSystem integration (replace stub)

**Size:** L | **Priority:** P0 | **Dependencies:** E10-041

**Description:**
Integrate DemandSystem with BuildingSystem, replacing the demand stub from Epic 4.

**Acceptance Criteria:**
- [ ] BuildingSystem receives IDemandProvider reference
- [ ] Building spawning checks has_positive_demand()
- [ ] Building upgrade/downgrade considers demand
- [ ] Remove stub demand provider from BuildingSystem

**Technical Notes:**
Epic 4 BuildingSystem has stub IDemandProvider. Replace with real DemandSystem.

---

### E10-060: DisorderGrid with double-buffering

**Size:** L | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement DisorderGrid class with double-buffered storage for circular dependency resolution.

**Acceptance Criteria:**
- [ ] DisorderCell struct: level (uint8_t)
- [ ] grid_ and previous_grid_ vectors
- [ ] get_level(x, y), get_level_previous_tick(x, y)
- [ ] add_disorder(x, y, amount), apply_suppression(x, y, amount)
- [ ] swap_buffers() for tick boundary
- [ ] Cached aggregate stats: total_disorder, high_disorder_tiles
- [ ] Memory: 1 byte/cell * 2 buffers

**Technical Notes:**
See economy-engineer-analysis.md Section 3.2 for design.

**Canon Reference:** Discussion CCR-1, CCR-2

---

### E10-061: ContaminationGrid with double-buffering

**Size:** L | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement ContaminationGrid class with double-buffered storage.

**Acceptance Criteria:**
- [ ] ContaminationCell struct: level (uint8_t), dominant_type (uint8_t)
- [ ] grid_ and previous_grid_ vectors
- [ ] get_level(x, y), get_level_previous_tick(x, y)
- [ ] get_dominant_type(x, y)
- [ ] add_contamination(x, y, amount, type), apply_decay(x, y, amount)
- [ ] swap_buffers() for tick boundary
- [ ] Memory: 2 bytes/cell * 2 buffers

**Technical Notes:**
See economy-engineer-analysis.md Section 3.1 for design.

---

### E10-062: LandValueGrid implementation

**Size:** M | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement LandValueGrid class for sector value storage.

**Acceptance Criteria:**
- [ ] LandValueCell struct: total_value (uint8_t), terrain_bonus (uint8_t)
- [ ] get_value(x, y), get_terrain_bonus(x, y)
- [ ] recalculate() method for full grid recalculation
- [ ] Memory: 2 bytes/cell (no double-buffer needed - reads from other grids' previous buffers)

**Technical Notes:**
See economy-engineer-analysis.md Section 3.3 for design.
LandValueSystem reads from DisorderGrid/ContaminationGrid previous tick.

---

### E10-063: Grid buffer swap mechanism

**Size:** M | **Priority:** P0 | **Dependencies:** E10-060, E10-061

**Description:**
Implement buffer swap mechanism called at tick start for double-buffered grids.

**Acceptance Criteria:**
- [ ] swap_buffers() swaps current and previous pointers (O(1))
- [ ] Called at start of each system's tick() method
- [ ] After swap: writes go to current, dependent systems read previous
- [ ] Document swap sequence in code comments

**Technical Notes:**
```cpp
void swap_buffers() {
    std::swap(grid_, previous_grid_);
}
```

**Canon Reference:** Discussion CCR-1

---

### E10-064: IGridOverlay interface

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define IGridOverlay interface for systems that provide overlay data for UI rendering.

**Acceptance Criteria:**
- [ ] get_overlay_data() returns raw grid pointer
- [ ] get_overlay_width(), get_overlay_height()
- [ ] get_value_at(x, y)
- [ ] get_color_scheme() for UI rendering hints
- [ ] Add to canon interfaces.yaml

**Technical Notes:**
```cpp
class IGridOverlay {
public:
    virtual const uint8_t* get_overlay_data() const = 0;
    virtual uint32_t get_overlay_width() const = 0;
    virtual uint32_t get_overlay_height() const = 0;
    virtual uint8_t get_value_at(int32_t x, int32_t y) const = 0;
    virtual OverlayColorScheme get_color_scheme() const = 0;
};
```

---

### E10-070: DisorderComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define DisorderComponent for buildings that generate or suppress disorder.

**Acceptance Criteria:**
- [ ] base_disorder_generation, current_disorder_generation (uint16_t)
- [ ] suppression_power, suppression_radius (for enforcers)
- [ ] local_disorder_level (cached from grid)
- [ ] is_disorder_source, is_enforcer flags
- [ ] Component size 12 bytes

**Technical Notes:**
See economy-engineer-analysis.md Section 2.1 for struct definition.

---

### E10-071: ContaminationType enum

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Define ContaminationType enum for categorizing contamination sources.

**Acceptance Criteria:**
- [ ] ContaminationType: Industrial, Traffic, Energy, Terrain
- [ ] Used by ContaminationComponent and ContaminationGrid

**Technical Notes:**
```cpp
enum class ContaminationType : uint8_t {
    Industrial = 0,
    Traffic = 1,
    Energy = 2,
    Terrain = 3
};
```

---

### E10-072: DisorderSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E10-060, E10-070

**Description:**
Implement DisorderSystem class skeleton with ISimulatable interface at tick_priority 70.

**Acceptance Criteria:**
- [ ] DisorderSystem implements ISimulatable
- [ ] tick_priority = 70
- [ ] tick() calls: swap_buffers(), generate(), apply_spread(), apply_suppression(), update_stats()
- [ ] Owns DisorderGrid

**Canon Reference:** interfaces.yaml - DisorderSystem.tick_priority: 70

---

### E10-073: Disorder generation from buildings

**Size:** M | **Priority:** P0 | **Dependencies:** E10-072

**Description:**
Generate disorder at building locations based on building type and occupancy.

**Acceptance Criteria:**
- [ ] DisorderGenerationConfig per zone type (base_generation, population_multiplier, land_value_modifier)
- [ ] Low-density habitation: 2 base
- [ ] High-density habitation: 5 base
- [ ] Exchange: 3-6 base
- [ ] Fabrication: 1-2 base
- [ ] Scale by occupancy and land value

**Technical Notes:**
See economy-engineer-analysis.md Section 5.1 for config table.

---

### E10-074: Land value effect on disorder (prev tick)

**Size:** M | **Priority:** P0 | **Dependencies:** E10-073, E10-062

**Description:**
Apply land value modifier to disorder generation using previous tick's land value.

**Acceptance Criteria:**
- [ ] Read land_value_grid.get_value(x, y) - previous tick data
- [ ] Low land value (0) = +100% disorder generation
- [ ] High land value (255) = +0% additional disorder
- [ ] Uses double-buffer pattern correctly

**Canon Reference:** Discussion CCR-1

---

### E10-075: Disorder spread algorithm

**Size:** L | **Priority:** P0 | **Dependencies:** E10-060, E10-073

**Description:**
Implement disorder spread to adjacent tiles.

**Acceptance Criteria:**
- [ ] SPREAD_THRESHOLD = 64 (only spread if level > threshold)
- [ ] Spread to 4-neighbors
- [ ] spread_amount = (level - threshold) / 8
- [ ] Water terrain blocks spread
- [ ] Source loses more than neighbors gain (diffusion)
- [ ] Use delta buffer to avoid order-dependent results

**Technical Notes:**
See economy-engineer-analysis.md Section 5.2 for algorithm.

---

### E10-076: Enforcer suppression integration

**Size:** M | **Priority:** P1 | **Dependencies:** E10-072

**Description:**
Apply disorder suppression from enforcer posts (stub until Epic 9).

**Acceptance Criteria:**
- [ ] Query enforcer posts via IServiceQueryable.get_enforcers_in_region()
- [ ] For each enforcer: apply suppression in radius with linear falloff
- [ ] Standard enforcer: power 80, radius 8
- [ ] Stub returns no enforcers until Epic 9

**Technical Notes:**
See economy-engineer-analysis.md Section 5.3 for algorithm.

---

### E10-077: DisorderSystem IStatQueryable

**Size:** S | **Priority:** P1 | **Dependencies:** E10-072

**Description:**
Implement IStatQueryable interface for disorder statistics.

**Acceptance Criteria:**
- [ ] get_stat_value: total_disorder, average_disorder, high_disorder_tiles
- [ ] get_disorder_at(x, y) direct query

---

### E10-078: Disorder overlay data generation

**Size:** S | **Priority:** P1 | **Dependencies:** E10-064, E10-060

**Description:**
Implement IGridOverlay for disorder visualization.

**Acceptance Criteria:**
- [ ] DisorderSystem implements IGridOverlay
- [ ] get_overlay_data() returns grid data
- [ ] Color scheme: green (low) to red (high)

---

### E10-079: Disorder events

**Size:** S | **Priority:** P1 | **Dependencies:** E10-072

**Description:**
Define and emit disorder-related events.

**Acceptance Criteria:**
- [ ] DisorderCrisisBeganEvent when tiles > 200 exceed threshold
- [ ] DisorderCrisisEndedEvent when crisis resolved
- [ ] Events include player and affected tile count

---

### E10-080: ContaminationComponent definition

**Size:** S | **Priority:** P0 | **Dependencies:** E10-071

**Description:**
Define ContaminationComponent for contamination sources.

**Acceptance Criteria:**
- [ ] base_contamination_output, current_contamination_output (uint32_t)
- [ ] spread_radius, spread_decay_rate
- [ ] contamination_type (ContaminationType)
- [ ] local_contamination_level (cached)
- [ ] is_active_source flag
- [ ] Component size 16 bytes

**Technical Notes:**
See economy-engineer-analysis.md Section 2.2 for struct definition.

---

### E10-081: ContaminationSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E10-061, E10-080

**Description:**
Implement ContaminationSystem class skeleton with ISimulatable interface at tick_priority 80.

**Acceptance Criteria:**
- [ ] ContaminationSystem implements ISimulatable
- [ ] tick_priority = 80
- [ ] tick() calls: swap_buffers(), generate(), apply_spread(), apply_decay(), update_stats()
- [ ] Owns ContaminationGrid

**Canon Reference:** interfaces.yaml - ContaminationSystem.tick_priority: 80

---

### E10-082: IContaminationSource query aggregation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-081

**Description:**
Query all IContaminationSource implementers for contamination output.

**Acceptance Criteria:**
- [ ] Query BuildingSystem for buildings with ContaminationComponent
- [ ] Query EnergySystem for polluting nexuses
- [ ] Aggregate by position and type
- [ ] Inactive buildings (unpowered, abandoned) don't pollute

**Canon Reference:** interfaces.yaml - IContaminationSource

---

### E10-083: Industrial contamination generation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-082

**Description:**
Generate contamination from fabrication buildings.

**Acceptance Criteria:**
- [ ] Fabrication buildings produce 50-200 contamination/tick based on level
- [ ] Scales with building activity/occupancy
- [ ] ContaminationType::Industrial
- [ ] Apply to grid at building position

---

### E10-084: Energy contamination generation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-082

**Description:**
Generate contamination from polluting energy nexuses.

**Acceptance Criteria:**
- [ ] Carbon nexus: 200/tick
- [ ] Petrochemical nexus: 120/tick
- [ ] Gaseous nexus: 40/tick
- [ ] ContaminationType::Energy
- [ ] Only when nexus is active and producing

---

### E10-085: Traffic contamination generation

**Size:** M | **Priority:** P0 | **Dependencies:** E10-082

**Description:**
Generate contamination from high-traffic pathways.

**Acceptance Criteria:**
- [ ] Query TransportSystem.get_traffic_contamination_sources()
- [ ] High congestion = 5-50 contamination/tick
- [ ] ContaminationType::Traffic
- [ ] Applied to road/pathway tiles

---

### E10-086: Terrain contamination (blight_mires)

**Size:** S | **Priority:** P0 | **Dependencies:** E10-081

**Description:**
Generate constant contamination from toxic terrain.

**Acceptance Criteria:**
- [ ] blight_mires (toxic_marshes) terrain generates 30/tick
- [ ] ContaminationType::Terrain
- [ ] Constant regardless of buildings

---

### E10-087: Contamination spread algorithm

**Size:** L | **Priority:** P0 | **Dependencies:** E10-061, E10-083

**Description:**
Implement contamination spread to adjacent tiles.

**Acceptance Criteria:**
- [ ] SPREAD_THRESHOLD = 32
- [ ] Spread to 8-neighbors (including diagonal)
- [ ] Diagonal spread is weaker (divide by 16 vs 8)
- [ ] Use delta buffer to avoid order-dependent results

**Technical Notes:**
See economy-engineer-analysis.md Section 6.3 for algorithm.

---

### E10-088: Contamination decay algorithm

**Size:** M | **Priority:** P0 | **Dependencies:** E10-087

**Description:**
Implement natural contamination decay over time.

**Acceptance Criteria:**
- [ ] BASE_DECAY_RATE = 2/tick
- [ ] Water proximity (dist <= 2) adds +3 decay (natural filtration)
- [ ] Forest/SporePlains terrain adds +3 decay (bioremediation)
- [ ] Decay applied after spread

**Technical Notes:**
See economy-engineer-analysis.md Section 6.4 for algorithm.

---

### E10-089: ContaminationSystem IStatQueryable

**Size:** S | **Priority:** P1 | **Dependencies:** E10-081

**Description:**
Implement IStatQueryable interface for contamination statistics.

**Acceptance Criteria:**
- [ ] get_stat_value: total_contamination, average_contamination, toxic_tiles
- [ ] get_contamination_at(x, y) direct query
- [ ] Breakdown by contamination type

---

### E10-090: Contamination overlay data generation

**Size:** S | **Priority:** P1 | **Dependencies:** E10-064, E10-061

**Description:**
Implement IGridOverlay for contamination visualization.

**Acceptance Criteria:**
- [ ] ContaminationSystem implements IGridOverlay
- [ ] get_overlay_data() returns grid data
- [ ] Color scheme: clear (low) to yellow-green (high)

---

### E10-091: Contamination events

**Size:** S | **Priority:** P1 | **Dependencies:** E10-081

**Description:**
Define and emit contamination-related events.

**Acceptance Criteria:**
- [ ] ContaminationCrisisBeganEvent when tiles > 200 exceed threshold
- [ ] ContaminationCrisisEndedEvent when crisis resolved
- [ ] Events include player and affected tile count

---

### E10-100: LandValueSystem class skeleton

**Size:** M | **Priority:** P0 | **Dependencies:** E10-062

**Description:**
Implement LandValueSystem class skeleton with ISimulatable interface at tick_priority 85.

**Acceptance Criteria:**
- [ ] LandValueSystem implements ISimulatable
- [ ] tick_priority = 85 (after Disorder 70 and Contamination 80)
- [ ] tick() calls recalculate() using previous tick data from other grids
- [ ] Owns LandValueGrid

**Canon Reference:** Discussion CCR-3 - LandValueSystem tick_priority 85

---

### E10-101: Terrain value factors

**Size:** M | **Priority:** P0 | **Dependencies:** E10-100

**Description:**
Calculate terrain-based bonuses and penalties for land value.

**Acceptance Criteria:**
- [ ] Water proximity: +30 (adjacent), +20 (1 tile), +10 (2 tiles)
- [ ] Crystal fields (prisma_fields): +25
- [ ] Spore plains: +15
- [ ] Forest (biolume grove): +10
- [ ] Toxic marshes (blight_mires): -30
- [ ] Store terrain_bonus in LandValueCell

**Technical Notes:**
See economy-engineer-analysis.md Section 4.1 for values.

---

### E10-102: Road accessibility bonus

**Size:** M | **Priority:** P0 | **Dependencies:** E10-100

**Description:**
Calculate road accessibility bonus for land value.

**Acceptance Criteria:**
- [ ] Query TransportSystem.get_nearest_road_distance(x, y)
- [ ] On road: +20
- [ ] 1 tile away: +15
- [ ] 2 tiles away: +10
- [ ] 3 tiles away: +5
- [ ] Beyond 3: +0

---

### E10-103: Disorder penalty (prev tick read)

**Size:** M | **Priority:** P0 | **Dependencies:** E10-100, E10-063

**Description:**
Apply disorder penalty to land value using previous tick's disorder data.

**Acceptance Criteria:**
- [ ] Read disorder_grid.get_level_previous_tick(x, y)
- [ ] Scale disorder (0-255) to penalty (0-40)
- [ ] penalty = (disorder * 40) / 255
- [ ] Uses double-buffer pattern correctly

**Canon Reference:** Discussion CCR-1

---

### E10-104: Contamination penalty (prev tick read)

**Size:** M | **Priority:** P0 | **Dependencies:** E10-100, E10-063

**Description:**
Apply contamination penalty to land value using previous tick's contamination data.

**Acceptance Criteria:**
- [ ] Read contamination_grid.get_level_previous_tick(x, y)
- [ ] Scale contamination (0-255) to penalty (0-50)
- [ ] penalty = (contamination * 50) / 255
- [ ] Uses double-buffer pattern correctly

---

### E10-105: Full value recalculation

**Size:** L | **Priority:** P0 | **Dependencies:** E10-101, E10-102, E10-103, E10-104

**Description:**
Perform full land value recalculation each tick.

**Acceptance Criteria:**
- [ ] Base value: 128 (neutral)
- [ ] Add terrain bonuses
- [ ] Add road accessibility bonus
- [ ] Subtract disorder penalty
- [ ] Subtract contamination penalty
- [ ] Clamp to 0-255
- [ ] Update entire grid each tick

**Optimization Notes:**
- Consider dirty region tracking for partial updates
- Profile to ensure < 8ms at 512x512

---

### E10-106: LandValueSystem IStatQueryable

**Size:** S | **Priority:** P1 | **Dependencies:** E10-100

**Description:**
Implement IStatQueryable interface for land value statistics.

**Acceptance Criteria:**
- [ ] get_stat_value: average_land_value, high_value_tiles, low_value_tiles
- [ ] get_sector_value_at(x, y) direct query

---

### E10-107: Land value overlay data generation

**Size:** S | **Priority:** P1 | **Dependencies:** E10-064, E10-062

**Description:**
Implement IGridOverlay for land value visualization.

**Acceptance Criteria:**
- [ ] LandValueSystem implements IGridOverlay
- [ ] get_overlay_data() returns grid data
- [ ] Color scheme: red (low) to green (high)

---

### E10-110: AbandonmentSystem integration

**Size:** L | **Priority:** P1 | **Dependencies:** E10-001

**Description:**
Implement ghost town decay process within SimulationCore.

**Acceptance Criteria:**
- [ ] Detect player abandonment (disconnect or explicit)
- [ ] OwnershipState transitions: Active -> Abandoned (50 cycles) -> GhostTown (100 cycles) -> Cleared (GAME_MASTER)
- [ ] Buildings stop functioning when abandoned
- [ ] Population gradually dies off or migrates
- [ ] Tiles return to GAME_MASTER ownership

**Technical Notes:**
See systems-architect-analysis.md Section 6.5 for process.

**Canon Reference:** patterns.yaml - ghost_town process

---

### E10-111: Simulation state serialization

**Size:** M | **Priority:** P1 | **Dependencies:** E10-001

**Description:**
Implement serialization for simulation state (save/load).

**Acceptance Criteria:**
- [ ] Serialize: tick count, cycle, phase, speed
- [ ] Serialize grid data (disorder, contamination, land value)
- [ ] Serialize per-player components (population, employment, demand)
- [ ] Deserialize and restore state

---

### E10-112: Service coverage stubs (IServiceQueryable)

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement stub IServiceQueryable interface until Epic 9.

**Acceptance Criteria:**
- [ ] IServiceQueryable interface defined
- [ ] get_coverage(ServiceType, PlayerID) returns 50 (neutral)
- [ ] get_coverage_at(ServiceType, GridPosition) returns 50
- [ ] get_enforcers_in_region() returns empty list
- [ ] Add to canon interfaces.yaml

---

### E10-113: Economy stubs (IEconomyQueryable)

**Size:** S | **Priority:** P0 | **Dependencies:** None

**Description:**
Implement stub IEconomyQueryable interface until Epic 11.

**Acceptance Criteria:**
- [ ] IEconomyQueryable interface defined
- [ ] get_tribute_rate(ZoneBuildingType) returns 7 (default)
- [ ] get_average_tribute_rate() returns 7
- [ ] Add to canon interfaces.yaml

---

### E10-114: Epic 5 EnergySystem IContaminationSource

**Size:** M | **Priority:** P1 | **Dependencies:** E10-082

**Description:**
Implement IContaminationSource in EnergySystem for polluting nexuses.

**Acceptance Criteria:**
- [ ] EnergyProducerComponent tracks contamination_output
- [ ] Carbon nexus: 200, Petrochemical: 120, Gaseous: 40
- [ ] Clean energy sources: 0
- [ ] ContaminationSystem queries via interface

---

### E10-115: Epic 7 TransportSystem traffic pollution

**Size:** M | **Priority:** P1 | **Dependencies:** E10-082

**Description:**
Implement traffic contamination output in TransportSystem.

**Acceptance Criteria:**
- [ ] get_traffic_contamination_sources() method
- [ ] Returns list of {position, contamination_output}
- [ ] High-congestion tiles produce 5-50 contamination
- [ ] Scales with traffic volume

---

### E10-120: Unit tests: demographics

**Size:** L | **Priority:** P1 | **Dependencies:** E10-017, E10-027

**Description:**
Comprehensive unit tests for demographic calculations.

**Acceptance Criteria:**
- [ ] Test birth rate modifiers
- [ ] Test death rate modifiers
- [ ] Test natural growth edge cases (0 population, max capacity)
- [ ] Test migration in/out calculations
- [ ] Test age distribution evolution
- [ ] Test sandbox rules (caps, minimums)

---

### E10-121: Unit tests: employment

**Size:** L | **Priority:** P1 | **Dependencies:** E10-021

**Description:**
Comprehensive unit tests for employment system.

**Acceptance Criteria:**
- [ ] Test labor pool calculation
- [ ] Test job market aggregation
- [ ] Test employment matching (various scenarios)
- [ ] Test unemployment rate calculation
- [ ] Test building occupancy distribution
- [ ] Test unemployment effects

---

### E10-122: Unit tests: demand formulas

**Size:** L | **Priority:** P1 | **Dependencies:** E10-043, E10-044, E10-045

**Description:**
Comprehensive unit tests for demand calculations.

**Acceptance Criteria:**
- [ ] Test habitation demand formula (all factors)
- [ ] Test exchange demand formula
- [ ] Test fabrication demand formula
- [ ] Test demand caps
- [ ] Test edge cases (-100 to +100 bounds)
- [ ] Test demand factor breakdown

---

### E10-123: Unit tests: disorder spread

**Size:** L | **Priority:** P1 | **Dependencies:** E10-075

**Description:**
Comprehensive unit tests for disorder spread algorithm.

**Acceptance Criteria:**
- [ ] Test spread threshold behavior
- [ ] Test 4-neighbor spread pattern
- [ ] Test water blocking spread
- [ ] Test suppression application
- [ ] Test double-buffer correctness
- [ ] Test aggregate stat accuracy

---

### E10-124: Unit tests: contamination spread

**Size:** L | **Priority:** P1 | **Dependencies:** E10-087

**Description:**
Comprehensive unit tests for contamination spread algorithm.

**Acceptance Criteria:**
- [ ] Test spread threshold behavior
- [ ] Test 8-neighbor spread pattern
- [ ] Test diagonal spread reduction
- [ ] Test decay rates
- [ ] Test terrain decay bonuses
- [ ] Test double-buffer correctness

---

### E10-125: Unit tests: land value calculation

**Size:** L | **Priority:** P1 | **Dependencies:** E10-105

**Description:**
Comprehensive unit tests for land value calculation.

**Acceptance Criteria:**
- [ ] Test terrain bonuses (each type)
- [ ] Test road accessibility bonus
- [ ] Test disorder penalty
- [ ] Test contamination penalty
- [ ] Test previous-tick data access
- [ ] Test value clamping

---

### E10-126: Integration test: full simulation cycle

**Size:** L | **Priority:** P1 | **Dependencies:** All P0 tickets

**Description:**
End-to-end integration test for full simulation cycle.

**Acceptance Criteria:**
- [ ] Create test scenario with buildings, zones, population
- [ ] Run simulation for 1000 ticks
- [ ] Verify all systems interact correctly
- [ ] Verify circular dependencies resolve without oscillation
- [ ] Verify population grows with good conditions
- [ ] Verify disorder/contamination spread and decay
- [ ] Verify land value responds to changes
- [ ] Verify demand reflects population state

---

## Size Summary

| Size | Count | Percentage |
|------|-------|------------|
| S | 27 | 35% |
| M | 38 | 49% |
| L | 13 | 16% |
| **Total** | **78** | 100% |

## Priority Summary

| Priority | Count | Notes |
|----------|-------|-------|
| P0 | 49 | Core functionality required for epic completion |
| P1 | 29 | Important but can be deferred to polish phase |
| P2 | 0 | - |

## Critical Path

```
E10-001 (SimulationCore)
    |
    +-- E10-002, E10-003, E10-004, E10-005
    |
E10-060, E10-061, E10-062 (Dense Grids)
    |
    +-- E10-063 (Buffer swap)
    |
E10-010, E10-011, E10-012, E10-013 (Components)
    |
    +-- E10-014 (PopulationSystem)
    |       |
    |       +-- E10-015, E10-016 -> E10-017 (Demographics)
    |       |
    |       +-- E10-019, E10-020 -> E10-021 (Employment)
    |       |
    |       +-- E10-024, E10-025, E10-026 -> E10-027 (Migration)
    |
E10-040, E10-041
    |
    +-- E10-042 (DemandSystem)
    |       |
    |       +-- E10-043, E10-044, E10-045, E10-046 (Formulas)
    |       |
    |       +-- E10-049 (BuildingSystem integration)
    |
E10-070, E10-071, E10-080 (Components)
    |
    +-- E10-072 (DisorderSystem)
    |       |
    |       +-- E10-073, E10-074 -> E10-075 (Generation/Spread)
    |
    +-- E10-081 (ContaminationSystem)
    |       |
    |       +-- E10-082, E10-083-086 -> E10-087, E10-088 (Generation/Spread/Decay)
    |
E10-100 (LandValueSystem)
    |
    +-- E10-101, E10-102, E10-103, E10-104 -> E10-105 (Value calculation)
```

---

## Canon Updates Required

After Epic 10 completion, the following canon updates are needed:

### interfaces.yaml
- Add DemandSystem tick_priority: 52
- Add LandValueSystem tick_priority: 85
- Add ISimulationTime interface
- Add IGridOverlay interface
- Add IServiceQueryable interface (stub)
- Add IEconomyQueryable interface (stub)

### patterns.yaml
- Add ContaminationGrid to dense_grid_exception.applies_to
- Add DisorderGrid to dense_grid_exception.applies_to
- Add LandValueGrid to dense_grid_exception.applies_to

### systems.yaml
- Add DemandSystem tick_priority: 52
- Add LandValueSystem tick_priority: 85
- Add ISimulatable priorities for all Epic 10 systems

---

*End of Epic 10 Tickets*
