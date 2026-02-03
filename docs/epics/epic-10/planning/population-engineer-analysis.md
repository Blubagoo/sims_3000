# Population Engineer Analysis: Epic 10 - Simulation Core

**Author:** Population Engineer Agent
**Date:** 2026-01-29
**Canon Version:** 0.9.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Table of Contents

1. [Population Model Overview](#1-population-model-overview)
2. [Component Designs](#2-component-designs)
3. [Population Growth Algorithm](#3-population-growth-algorithm)
4. [Employment System](#4-employment-system)
5. [Demand Calculation (RCI)](#5-demand-calculation-rci)
6. [Migration Model](#6-migration-model)
7. [Life Expectancy](#7-life-expectancy)
8. [Key Work Items](#8-key-work-items)
9. [Questions for Other Agents](#9-questions-for-other-agents)
10. [Performance Considerations](#10-performance-considerations)
11. [Multiplayer Considerations](#11-multiplayer-considerations)

---

## 1. Population Model Overview

### 1.1 Core Philosophy

The population simulation in ZergCity uses an **aggregate demographic model**, not individual being simulation. We track population counts, rates, and distributions rather than simulating individual beings. This approach provides:

- **Performance:** Supports large colonies (100,000+ beings) without per-entity overhead
- **Network efficiency:** Only aggregate statistics need synchronization
- **SimCity 2000 authenticity:** Classic city builders use aggregate models
- **Gameplay clarity:** Players understand rates and percentages, not individual life stories

### 1.2 Population Domains

Population simulation involves three interconnected systems:

```
                    +-------------------+
                    | PopulationSystem  |
                    | (tick_priority:50)|
                    +---------+---------+
                              |
        +---------------------+---------------------+
        |                     |                     |
        v                     v                     v
+---------------+    +------------------+    +--------------+
| Demographics  |    | Employment       |    | Migration    |
| - Birth rate  |    | - Job matching   |    | - In-flow    |
| - Death rate  |    | - Unemployment   |    | - Out-flow   |
| - Life expect.|    | - Labor pool     |    | - Attractors |
+---------------+    +------------------+    +--------------+
        |                     |                     |
        +---------------------+---------------------+
                              |
                              v
                    +-------------------+
                    |   DemandSystem    |
                    | (after Population)|
                    +-------------------+
                              |
                              v
                    +-------------------+
                    | Zone Pressure RCI |
                    +-------------------+
```

### 1.3 Canon Terminology Mapping

| Human Term | Canon Term | Component/Field |
|------------|------------|-----------------|
| citizen | being | N/A (aggregate) |
| population | colony_population | PopulationComponent.total_beings |
| resident | inhabitant | PopulationComponent.total_beings |
| worker | laborer | EmploymentComponent.employed_laborers |
| residential | habitation | ZoneBuildingType::Habitation |
| commercial | exchange | ZoneBuildingType::Exchange |
| industrial | fabrication | ZoneBuildingType::Fabrication |
| happiness | harmony | PopulationComponent.harmony_index |
| crime | disorder | Input from DisorderSystem |
| pollution | contamination | Input from ContaminationSystem |

### 1.4 Simulation Rate Context

- **Tick rate:** 20 ticks/second (50ms per tick)
- **PopulationSystem tick_priority:** 50 (after BuildingSystem 40, TransportSystem 45, RailSystem 47)
- **Demographic cycles:** Some calculations (birth/death) run every 100 ticks (5 seconds game time)
- **Migration calculations:** Run every 20 ticks (1 second game time)
- **Employment matching:** Run every tick for responsiveness

---

## 2. Component Designs

### 2.1 PopulationComponent

Attached to each player entity (not per-being). Stores aggregate population statistics.

```cpp
struct PopulationComponent {
    // === Core Counts ===
    uint32_t total_beings = 0;           // Total population
    uint32_t max_capacity = 0;           // Sum of all habitation building capacity

    // === Age Distribution (simplified) ===
    // Instead of tracking individual ages, we track cohort percentages
    uint8_t youth_percent = 25;          // 0-17 equivalent (non-working)
    uint8_t adult_percent = 60;          // 18-64 equivalent (working age)
    uint8_t elder_percent = 15;          // 65+ equivalent (retired)

    // === Demographics per 1000 beings ===
    uint16_t birth_rate_per_1000 = 15;   // Births per 1000 beings per cycle
    uint16_t death_rate_per_1000 = 8;    // Deaths per 1000 beings per cycle

    // === Derived Statistics ===
    int32_t natural_growth = 0;          // births - deaths (last cycle)
    int32_t net_migration = 0;           // in - out (last cycle)
    int32_t growth_rate = 0;             // natural + migration (last cycle)

    // === Quality of Life ===
    uint8_t harmony_index = 50;          // 0-100, affects migration
    uint8_t health_index = 50;           // 0-100, affects death rate
    uint8_t education_index = 50;        // 0-100, affects demand/land value

    // === Timing ===
    uint32_t last_demographic_tick = 0;  // When birth/death last calculated
    uint32_t last_migration_tick = 0;    // When migration last calculated

    // === Historical (for UI graphs) ===
    // Ring buffer of last 12 cycles (1 cycle = ~500 ticks = 25 seconds)
    static constexpr size_t HISTORY_SIZE = 12;
    std::array<uint32_t, HISTORY_SIZE> population_history;
    uint8_t history_index = 0;
};
```

**Size Analysis:**
- Core counts: 8 bytes
- Age distribution: 3 bytes
- Rates: 4 bytes
- Derived: 12 bytes
- Quality: 3 bytes
- Timing: 8 bytes
- History: 48 bytes + 1 byte
- **Total: ~87 bytes** per player

With 4 players maximum, total PopulationComponent memory is negligible (~350 bytes).

### 2.2 EmploymentComponent

Attached to each player entity. Tracks employment status for the colony.

```cpp
struct EmploymentComponent {
    // === Labor Pool ===
    uint32_t working_age_beings = 0;     // adult_percent * total_beings
    uint32_t labor_force = 0;            // working_age * labor_participation_rate
    uint32_t employed_laborers = 0;      // Currently employed
    uint32_t unemployed = 0;             // labor_force - employed

    // === Job Market ===
    uint32_t total_jobs = 0;             // Sum of all exchange/fabrication capacity
    uint32_t exchange_jobs = 0;          // Jobs in exchange buildings
    uint32_t fabrication_jobs = 0;       // Jobs in fabrication buildings

    // === Rates ===
    uint8_t unemployment_rate = 0;       // 0-100, (unemployed / labor_force) * 100
    uint8_t labor_participation = 65;    // 0-100, what % of working age seek employment

    // === Distribution ===
    uint32_t exchange_employed = 0;      // Laborers in exchange sector
    uint32_t fabrication_employed = 0;   // Laborers in fabrication sector

    // === Commute (simplified) ===
    // We don't track individual commutes; this is aggregate
    uint8_t avg_commute_satisfaction = 50; // 0-100, affects harmony
};
```

**Size Analysis:** ~41 bytes per player

### 2.3 BuildingOccupancyComponent

Attached to individual habitation/exchange/fabrication buildings. Tracks occupancy.

```cpp
struct BuildingOccupancyComponent {
    // === Capacity ===
    uint16_t capacity = 0;               // Max beings (hab) or laborers (exch/fab)
    uint16_t current_occupancy = 0;      // Current filled count

    // === Occupancy State ===
    enum class OccupancyState : uint8_t {
        Empty = 0,           // No occupants
        UnderOccupied = 1,   // < 50% capacity
        NormalOccupied = 2,  // 50-90% capacity
        FullyOccupied = 3,   // > 90% capacity
        Overcrowded = 4      // > 100% (temporary overflow)
    };
    OccupancyState state = OccupancyState::Empty;

    // === Building Type Context ===
    // Not stored here - derived from BuildingComponent.zone_type
    // Habitation: occupancy = inhabitants
    // Exchange: occupancy = laborers (employees)
    // Fabrication: occupancy = laborers (employees)

    // === Timing ===
    uint32_t occupancy_changed_tick = 0; // For rate-limiting occupancy changes
};
```

**Size Analysis:** 9 bytes per building entity

With 10,000 buildings on a large map: ~90 KB

### 2.4 MigrationFactorComponent

Per-player component that aggregates factors affecting migration attractiveness.

```cpp
struct MigrationFactorComponent {
    // === Positive Attractors (0-100 each) ===
    uint8_t job_availability = 50;       // Low unemployment = attractive
    uint8_t housing_availability = 50;   // Low occupancy rate = attractive
    uint8_t sector_value_avg = 50;       // Average land value
    uint8_t service_coverage = 50;       // Medical/education/enforcer coverage
    uint8_t harmony_level = 50;          // Overall harmony

    // === Negative Repulsors (0-100 each) ===
    uint8_t disorder_level = 0;          // High disorder = repels
    uint8_t contamination_level = 0;     // High contamination = repels
    uint8_t tribute_burden = 50;         // High taxes = repels
    uint8_t congestion_level = 0;        // Traffic congestion = repels

    // === Computed Scores ===
    int16_t net_attraction = 0;          // Weighted sum: positive - negative
    int16_t migration_pressure = 0;      // How many beings want to move in/out
};
```

**Size Analysis:** 12 bytes per player

---

## 3. Population Growth Algorithm

### 3.1 Growth Components

Population growth is the sum of two factors:

```
growth = natural_growth + net_migration

where:
  natural_growth = births - deaths
  net_migration = migration_in - migration_out
```

### 3.2 Birth Rate Calculation

Births occur based on population size, modified by quality-of-life factors.

```cpp
// Run every DEMOGRAPHIC_CYCLE_TICKS (100 ticks = 5 seconds)
void calculate_births(PopulationComponent& pop, const MigrationFactorComponent& factors) {
    // Base birth rate: 15 per 1000 per cycle
    const float BASE_BIRTH_RATE = 0.015f;

    // Modifiers
    float harmony_modifier = lerp(0.5f, 1.5f, factors.harmony_level / 100.0f);
    float health_modifier = lerp(0.7f, 1.2f, pop.health_index / 100.0f);
    float housing_modifier = lerp(0.3f, 1.0f, housing_availability_ratio());

    // Don't birth if no housing capacity
    if (pop.total_beings >= pop.max_capacity) {
        housing_modifier = 0.1f;  // Minimal births when overcrowded
    }

    float effective_rate = BASE_BIRTH_RATE * harmony_modifier * health_modifier * housing_modifier;

    // Calculate births this cycle
    uint32_t births = static_cast<uint32_t>(pop.total_beings * effective_rate);

    // Minimum 1 birth if population > 0 and housing available
    if (pop.total_beings > 0 && pop.total_beings < pop.max_capacity && births == 0) {
        births = 1;
    }

    pop.birth_rate_per_1000 = static_cast<uint16_t>(effective_rate * 1000);
    return births;
}
```

### 3.3 Death Rate Calculation

Deaths occur based on population size, age distribution, and health factors.

```cpp
void calculate_deaths(PopulationComponent& pop, const MigrationFactorComponent& factors) {
    // Base death rate: 8 per 1000 per cycle
    const float BASE_DEATH_RATE = 0.008f;

    // Modifiers
    float health_modifier = lerp(1.5f, 0.5f, pop.health_index / 100.0f); // Low health = more deaths
    float contamination_modifier = lerp(1.0f, 2.0f, factors.contamination_level / 100.0f);
    float service_modifier = lerp(1.3f, 0.7f, factors.service_coverage / 100.0f);

    // Age distribution affects death rate (more elders = more deaths)
    float age_modifier = 0.5f + (pop.elder_percent / 100.0f) * 1.5f;

    float effective_rate = BASE_DEATH_RATE * health_modifier * contamination_modifier
                         * service_modifier * age_modifier;

    uint32_t deaths = static_cast<uint32_t>(pop.total_beings * effective_rate);

    // Cap deaths at 5% of population per cycle to prevent sudden collapse
    deaths = std::min(deaths, pop.total_beings / 20);

    pop.death_rate_per_1000 = static_cast<uint16_t>(effective_rate * 1000);
    return deaths;
}
```

### 3.4 Natural Growth

```cpp
void apply_natural_growth(PopulationComponent& pop) {
    uint32_t births = calculate_births(pop, factors);
    uint32_t deaths = calculate_deaths(pop, factors);

    // Apply changes
    pop.total_beings += births;
    pop.total_beings = std::max(0u, pop.total_beings - deaths);

    // Track for statistics
    pop.natural_growth = static_cast<int32_t>(births) - static_cast<int32_t>(deaths);

    // Update age distribution (simplified: births are youth, deaths remove elders)
    update_age_distribution(pop, births, deaths);
}
```

### 3.5 Age Distribution Evolution

Rather than tracking individual ages, we evolve cohort percentages over time.

```cpp
void update_age_distribution(PopulationComponent& pop, uint32_t births, uint32_t deaths) {
    // Births add to youth cohort
    // Deaths primarily remove from elder cohort
    // Each cycle, some youth "age" into adults, some adults into elders

    const float AGING_RATE = 0.02f;  // 2% of each cohort ages per cycle

    // Age youth into adults
    float youth_aging = pop.youth_percent * AGING_RATE;
    // Age adults into elders
    float adult_aging = pop.adult_percent * AGING_RATE;

    // Apply aging
    pop.youth_percent -= youth_aging;
    pop.adult_percent += youth_aging - adult_aging;
    pop.elder_percent += adult_aging;

    // Add births (all births are youth)
    float birth_fraction = static_cast<float>(births) / std::max(1u, pop.total_beings);
    pop.youth_percent += birth_fraction * 100.0f;

    // Remove deaths (weighted toward elders: 70% elder, 20% adult, 10% youth)
    float death_fraction = static_cast<float>(deaths) / std::max(1u, pop.total_beings);
    pop.elder_percent -= death_fraction * 100.0f * 0.7f;
    pop.adult_percent -= death_fraction * 100.0f * 0.2f;
    pop.youth_percent -= death_fraction * 100.0f * 0.1f;

    // Normalize to 100%
    float total = pop.youth_percent + pop.adult_percent + pop.elder_percent;
    if (total > 0) {
        pop.youth_percent = (pop.youth_percent / total) * 100.0f;
        pop.adult_percent = (pop.adult_percent / total) * 100.0f;
        pop.elder_percent = (pop.elder_percent / total) * 100.0f;
    }
}
```

---

## 4. Employment System

### 4.1 Employment Model Overview

Employment matches the labor pool (working-age beings) to available jobs (exchange + fabrication capacity).

```
Labor Pool = Working Age Beings * Labor Participation Rate
Jobs = Exchange Capacity + Fabrication Capacity
Employment = min(Labor Pool, Jobs)
Unemployment = Labor Pool - Employment
```

### 4.2 Labor Pool Calculation

```cpp
void calculate_labor_pool(PopulationComponent& pop, EmploymentComponent& emp) {
    // Working age beings = adults
    emp.working_age_beings = static_cast<uint32_t>(
        pop.total_beings * (pop.adult_percent / 100.0f)
    );

    // Labor participation rate: what % of working-age beings seek employment
    // Affected by:
    // - Harmony (high harmony = more want to work)
    // - Education (high education = more skilled laborers)
    // - Service coverage (childcare/eldercare enables more to work)
    float base_participation = 0.65f;  // 65% base

    float harmony_factor = lerp(0.9f, 1.1f, pop.harmony_index / 100.0f);
    float education_factor = lerp(0.95f, 1.05f, pop.education_index / 100.0f);

    emp.labor_participation = static_cast<uint8_t>(
        std::clamp(base_participation * harmony_factor * education_factor * 100.0f, 0.0f, 100.0f)
    );

    emp.labor_force = static_cast<uint32_t>(
        emp.working_age_beings * (emp.labor_participation / 100.0f)
    );
}
```

### 4.3 Job Availability Calculation

Jobs are aggregated from building capacities.

```cpp
void calculate_job_market(EmploymentComponent& emp, IBuildingQueryable& buildings, PlayerID owner) {
    // Query total capacity from exchange and fabrication buildings
    emp.exchange_jobs = buildings.get_total_capacity(ZoneBuildingType::Exchange, owner);
    emp.fabrication_jobs = buildings.get_total_capacity(ZoneBuildingType::Fabrication, owner);

    emp.total_jobs = emp.exchange_jobs + emp.fabrication_jobs;
}
```

### 4.4 Employment Matching Algorithm

The matching algorithm distributes laborers to jobs based on availability and preferences.

```cpp
void match_employment(EmploymentComponent& emp) {
    // Total employment is capped by min(labor_force, total_jobs)
    uint32_t max_employment = std::min(emp.labor_force, emp.total_jobs);

    // Distribute between exchange and fabrication based on job availability
    // Simple proportional distribution
    if (emp.total_jobs > 0) {
        float exchange_ratio = static_cast<float>(emp.exchange_jobs) / emp.total_jobs;
        float fabrication_ratio = static_cast<float>(emp.fabrication_jobs) / emp.total_jobs;

        emp.exchange_employed = static_cast<uint32_t>(max_employment * exchange_ratio);
        emp.fabrication_employed = static_cast<uint32_t>(max_employment * fabrication_ratio);

        // Ensure totals match
        emp.employed_laborers = emp.exchange_employed + emp.fabrication_employed;
    } else {
        emp.exchange_employed = 0;
        emp.fabrication_employed = 0;
        emp.employed_laborers = 0;
    }

    // Calculate unemployment
    emp.unemployed = emp.labor_force - emp.employed_laborers;
    emp.unemployment_rate = emp.labor_force > 0
        ? static_cast<uint8_t>((emp.unemployed * 100) / emp.labor_force)
        : 0;
}
```

### 4.5 Building Occupancy Distribution

After aggregate employment is calculated, distribute laborers to individual buildings.

```cpp
void distribute_building_occupancy(
    EmploymentComponent& emp,
    Registry& registry,
    PlayerID owner
) {
    // === Distribute inhabitants to habitation buildings ===
    uint32_t inhabitants_remaining = pop.total_beings;
    auto habitation_view = registry.view<BuildingComponent, BuildingOccupancyComponent, OwnershipComponent>();

    for (auto [entity, building, occupancy, ownership] : habitation_view.each()) {
        if (ownership.owner != owner) continue;
        if (building.zone_type != ZoneBuildingType::Habitation) continue;
        if (building.state != BuildingState::Active) continue;

        // Fill buildings proportionally
        uint32_t fill = std::min(inhabitants_remaining, static_cast<uint32_t>(occupancy.capacity));
        occupancy.current_occupancy = fill;
        inhabitants_remaining -= fill;

        update_occupancy_state(occupancy);
    }

    // === Distribute laborers to exchange buildings ===
    uint32_t exchange_remaining = emp.exchange_employed;
    for (auto [entity, building, occupancy, ownership] : view.each()) {
        if (ownership.owner != owner) continue;
        if (building.zone_type != ZoneBuildingType::Exchange) continue;
        if (building.state != BuildingState::Active) continue;

        uint32_t fill = std::min(exchange_remaining, static_cast<uint32_t>(occupancy.capacity));
        occupancy.current_occupancy = fill;
        exchange_remaining -= fill;

        update_occupancy_state(occupancy);
    }

    // === Distribute laborers to fabrication buildings ===
    // (similar pattern for fabrication)
}

void update_occupancy_state(BuildingOccupancyComponent& occ) {
    float ratio = occ.capacity > 0
        ? static_cast<float>(occ.current_occupancy) / occ.capacity
        : 0.0f;

    if (ratio == 0.0f) occ.state = OccupancyState::Empty;
    else if (ratio < 0.5f) occ.state = OccupancyState::UnderOccupied;
    else if (ratio < 0.9f) occ.state = OccupancyState::NormalOccupied;
    else if (ratio <= 1.0f) occ.state = OccupancyState::FullyOccupied;
    else occ.state = OccupancyState::Overcrowded;
}
```

### 4.6 Unemployment Effects

High unemployment has negative effects:

| Unemployment Rate | Effect |
|-------------------|--------|
| 0-5% | Optimal - labor shortage may develop |
| 5-10% | Normal - healthy labor market |
| 10-15% | Elevated - minor harmony penalty |
| 15-25% | High - significant harmony penalty, increased disorder |
| 25%+ | Critical - major harmony drop, disorder spike, out-migration |

```cpp
void apply_unemployment_effects(PopulationComponent& pop, EmploymentComponent& emp, DisorderComponent& disorder) {
    // Harmony penalty from unemployment
    if (emp.unemployment_rate > 10) {
        int penalty = (emp.unemployment_rate - 10) / 2;  // -1 harmony per 2% over 10%
        pop.harmony_index = std::max(0, pop.harmony_index - penalty);
    }

    // Disorder increase from unemployment
    if (emp.unemployment_rate > 15) {
        int disorder_increase = (emp.unemployment_rate - 15);  // +1 disorder per 1% over 15%
        disorder.base_disorder_rate += disorder_increase;
    }
}
```

---

## 5. Demand Calculation (RCI)

### 5.1 Demand System Overview

DemandSystem calculates "zone pressure" - how much the colony wants to grow in each zone type. This drives BuildingSystem spawning and upgrade decisions.

Per canon `systems.yaml`:
```yaml
DemandSystem:
  owns:
    - RCI (zone pressure) calculation
    - Demand factors aggregation
    - Demand cap calculation
  depends_on:
    - PopulationSystem
    - EconomySystem (tax effects)
    - TransportSystem (accessibility)
```

### 5.2 Demand Component

```cpp
struct DemandComponent {
    // === Raw Demand Values (-100 to +100) ===
    // Positive = growth pressure, Negative = contraction pressure
    int8_t habitation_demand = 0;     // R demand
    int8_t exchange_demand = 0;       // C demand
    int8_t fabrication_demand = 0;    // I demand

    // === Demand Factors (for UI breakdown) ===
    struct DemandFactors {
        int8_t population_factor = 0;      // Population growth pressure
        int8_t employment_factor = 0;      // Job availability
        int8_t services_factor = 0;        // Service coverage
        int8_t tribute_factor = 0;         // Tax effects
        int8_t transport_factor = 0;       // Road accessibility
        int8_t contamination_factor = 0;   // Pollution effects
    };

    DemandFactors habitation_factors;
    DemandFactors exchange_factors;
    DemandFactors fabrication_factors;

    // === Caps ===
    uint32_t habitation_cap = 0;      // Max population from current infrastructure
    uint32_t exchange_cap = 0;        // Max exchange jobs
    uint32_t fabrication_cap = 0;     // Max fabrication jobs
};
```

### 5.3 Habitation Demand Formula

Habitation demand is driven by employment availability and quality of life.

```cpp
int8_t calculate_habitation_demand(
    const PopulationComponent& pop,
    const EmploymentComponent& emp,
    const MigrationFactorComponent& factors,
    const EconomyQueryable& economy
) {
    int total = 0;

    // === Population Factor ===
    // If housing occupancy is high, demand is high
    float occupancy_ratio = pop.max_capacity > 0
        ? static_cast<float>(pop.total_beings) / pop.max_capacity
        : 1.0f;

    if (occupancy_ratio > 0.9f) {
        total += 30;  // High demand when near capacity
    } else if (occupancy_ratio > 0.7f) {
        total += 15;
    } else if (occupancy_ratio < 0.5f) {
        total -= 10;  // Oversupply
    }

    // === Employment Factor ===
    // Jobs available = people want to move in
    if (emp.total_jobs > emp.labor_force) {
        int surplus_percent = ((emp.total_jobs - emp.labor_force) * 100) / std::max(1u, emp.labor_force);
        total += std::min(20, surplus_percent / 2);  // Cap at +20
    } else if (emp.unemployment_rate > 15) {
        total -= (emp.unemployment_rate - 15);  // Negative if high unemployment
    }

    // === Services Factor ===
    total += (factors.service_coverage - 50) / 5;  // +/- 10 based on services

    // === Tribute Factor ===
    // High residential tribute rate reduces demand
    int tribute_rate = economy.get_tribute_rate(ZoneBuildingType::Habitation);
    if (tribute_rate > 10) {
        total -= (tribute_rate - 10) / 2;
    } else if (tribute_rate < 7) {
        total += (7 - tribute_rate);
    }

    // === Contamination Factor ===
    total -= factors.contamination_level / 5;

    return std::clamp(total, -100, 100);
}
```

### 5.4 Exchange Demand Formula

Exchange demand is driven by population (customers) and disposable income.

```cpp
int8_t calculate_exchange_demand(
    const PopulationComponent& pop,
    const EmploymentComponent& emp,
    const MigrationFactorComponent& factors,
    const ITransportProvider& transport
) {
    int total = 0;

    // === Population Factor ===
    // More beings = more customers for exchange
    // Target ratio: 1 exchange job per 3 beings
    const float TARGET_RATIO = 0.33f;
    float current_ratio = pop.total_beings > 0
        ? static_cast<float>(emp.exchange_jobs) / pop.total_beings
        : 0.0f;

    if (current_ratio < TARGET_RATIO * 0.8f) {
        total += 25;  // Under-served, high demand
    } else if (current_ratio < TARGET_RATIO) {
        total += 10;
    } else if (current_ratio > TARGET_RATIO * 1.5f) {
        total -= 15;  // Over-served
    }

    // === Employment Factor ===
    // Low unemployment = people have money to spend
    if (emp.unemployment_rate < 5) {
        total += 15;
    } else if (emp.unemployment_rate > 15) {
        total -= 10;
    }

    // === Transport Factor ===
    // Exchange needs good road access for customers
    // (Note: uses average congestion across exchange zones)
    float avg_congestion = transport.get_average_congestion_for_zone(ZoneBuildingType::Exchange);
    if (avg_congestion > 0.7f) {
        total -= 15;  // Customers avoid gridlocked areas
    }

    // === Tribute Factor ===
    // High commercial tribute reduces demand
    int tribute_rate = economy.get_tribute_rate(ZoneBuildingType::Exchange);
    total -= std::max(0, tribute_rate - 8);

    return std::clamp(total, -100, 100);
}
```

### 5.5 Fabrication Demand Formula

Fabrication demand is driven by external trade, labor availability, and infrastructure.

```cpp
int8_t calculate_fabrication_demand(
    const PopulationComponent& pop,
    const EmploymentComponent& emp,
    const MigrationFactorComponent& factors,
    const ITransportProvider& transport
) {
    int total = 0;

    // === Labor Factor ===
    // Fabrication needs laborers
    // Target ratio: 1 fabrication job per 5 beings (laborers work in fabrication)
    const float TARGET_RATIO = 0.20f;
    float current_ratio = pop.total_beings > 0
        ? static_cast<float>(emp.fabrication_jobs) / pop.total_beings
        : 0.0f;

    if (current_ratio < TARGET_RATIO * 0.7f) {
        total += 20;  // Under-developed industry
    } else if (current_ratio > TARGET_RATIO * 1.5f) {
        total -= 10;  // Over-industrialized
    }

    // === Employment Factor ===
    // High unemployment = available labor force
    if (emp.unemployment_rate > 10) {
        total += 10;  // Labor surplus encourages industry
    }

    // === Transport Factor ===
    // Fabrication needs freight access (road connectivity)
    if (!transport.has_map_edge_connectivity()) {
        total -= 20;  // Can't ship goods without external connection
    }

    // === External Demand (placeholder for Epic 8 Ports) ===
    // Future: port connectivity increases demand
    total += 5;  // Base external demand

    // === Contamination tolerance ===
    // Fabrication is less deterred by contamination than habitation
    // (Industrial areas accept pollution)

    return std::clamp(total, -100, 100);
}
```

### 5.6 Demand Caps

Demand is capped by infrastructure capacity.

```cpp
void calculate_demand_caps(DemandComponent& demand, const PopulationComponent& pop,
                           const EmploymentComponent& emp, const MigrationFactorComponent& factors) {
    // Habitation cap: based on energy, fluid, and service coverage
    // Each limiting factor reduces the cap
    float energy_factor = factors.service_coverage / 100.0f;  // Placeholder
    float fluid_factor = 1.0f;  // Placeholder until FluidSystem integration

    demand.habitation_cap = static_cast<uint32_t>(
        pop.max_capacity * energy_factor * fluid_factor
    );

    // Exchange/Fabrication caps: based on transport and labor
    float transport_factor = 1.0f - (factors.congestion_level / 200.0f);  // Congestion halves capacity

    demand.exchange_cap = static_cast<uint32_t>(emp.exchange_jobs * transport_factor);
    demand.fabrication_cap = static_cast<uint32_t>(emp.fabrication_jobs * transport_factor);
}
```

### 5.7 IDemandProvider Interface

Provides demand data to BuildingSystem (replaces Epic 4 stub).

```cpp
class IDemandProvider {
public:
    virtual ~IDemandProvider() = default;

    // Get demand value for a zone type (-100 to +100)
    virtual int8_t get_demand(ZoneBuildingType type, PlayerID owner) const = 0;

    // Get demand cap for a zone type
    virtual uint32_t get_demand_cap(ZoneBuildingType type, PlayerID owner) const = 0;

    // Get detailed demand factors (for UI)
    virtual DemandComponent::DemandFactors get_demand_factors(
        ZoneBuildingType type, PlayerID owner) const = 0;

    // Check if demand is positive (shortcut for BuildingSystem)
    virtual bool has_positive_demand(ZoneBuildingType type, PlayerID owner) const = 0;
};
```

---

## 6. Migration Model

### 6.1 Migration Overview

Migration determines how beings move in and out of the colony. It's the primary growth driver for early-game colonies.

```
net_migration = migration_in - migration_out

migration_in = base_rate * attractiveness_factor * available_housing
migration_out = population * (1 - attractiveness_factor) * desperation_factor
```

### 6.2 Attractiveness Calculation

```cpp
void calculate_attractiveness(MigrationFactorComponent& factors,
                               const PopulationComponent& pop,
                               const EmploymentComponent& emp,
                               const DisorderComponent& disorder,
                               const ContaminationComponent& contamination,
                               const EconomyQueryable& economy) {
    // === Positive Factors ===
    // Job availability (low unemployment = attractive)
    factors.job_availability = 100 - std::min(100, emp.unemployment_rate * 2);

    // Housing availability (low occupancy = attractive)
    float occupancy_ratio = pop.max_capacity > 0
        ? static_cast<float>(pop.total_beings) / pop.max_capacity
        : 1.0f;
    factors.housing_availability = static_cast<uint8_t>((1.0f - occupancy_ratio) * 100);

    // Services (from ServicesSystem query)
    factors.service_coverage = query_service_coverage(owner);

    // Harmony
    factors.harmony_level = pop.harmony_index;

    // === Negative Factors ===
    factors.disorder_level = disorder.average_disorder_index;
    factors.contamination_level = contamination.average_contamination_index;
    factors.tribute_burden = economy.get_average_tribute_rate();
    factors.congestion_level = query_average_congestion(owner);

    // === Compute Net Attraction ===
    int positive = (factors.job_availability + factors.housing_availability +
                    factors.service_coverage + factors.harmony_level +
                    factors.sector_value_avg) / 5;

    int negative = (factors.disorder_level + factors.contamination_level +
                    factors.tribute_burden + factors.congestion_level) / 4;

    factors.net_attraction = positive - negative;
}
```

### 6.3 Migration In Calculation

```cpp
uint32_t calculate_migration_in(const PopulationComponent& pop,
                                 const MigrationFactorComponent& factors) {
    // Base migration rate: 50 beings per cycle at neutral attractiveness
    const uint32_t BASE_MIGRATION = 50;

    // Scale by attractiveness (-100 to +100 -> 0x to 2x)
    float attraction_multiplier = 1.0f + (factors.net_attraction / 100.0f);
    attraction_multiplier = std::clamp(attraction_multiplier, 0.0f, 2.0f);

    // Scale by available housing
    uint32_t available_housing = pop.max_capacity > pop.total_beings
        ? pop.max_capacity - pop.total_beings
        : 0;

    // Migration is capped by available housing
    uint32_t potential_migrants = static_cast<uint32_t>(BASE_MIGRATION * attraction_multiplier);
    uint32_t actual_migrants = std::min(potential_migrants, available_housing);

    // Colony size scaling (larger colonies attract more migrants)
    float size_factor = std::min(2.0f, 1.0f + (pop.total_beings / 10000.0f));
    actual_migrants = static_cast<uint32_t>(actual_migrants * size_factor);

    return actual_migrants;
}
```

### 6.4 Migration Out Calculation

```cpp
uint32_t calculate_migration_out(const PopulationComponent& pop,
                                  const MigrationFactorComponent& factors) {
    // Base out-migration: 0.5% of population per cycle
    const float BASE_OUT_RATE = 0.005f;

    // Desperation factors increase out-migration
    float desperation = 0.0f;

    // High disorder drives people away
    if (factors.disorder_level > 50) {
        desperation += (factors.disorder_level - 50) / 100.0f;
    }

    // High contamination drives people away
    if (factors.contamination_level > 50) {
        desperation += (factors.contamination_level - 50) / 100.0f;
    }

    // High unemployment drives people away
    if (factors.job_availability < 30) {
        desperation += (30 - factors.job_availability) / 100.0f;
    }

    // Low harmony drives people away
    if (factors.harmony_level < 30) {
        desperation += (30 - factors.harmony_level) / 100.0f;
    }

    // Effective out-rate
    float effective_rate = BASE_OUT_RATE * (1.0f + desperation);
    effective_rate = std::min(effective_rate, 0.05f);  // Cap at 5% per cycle

    return static_cast<uint32_t>(pop.total_beings * effective_rate);
}
```

### 6.5 Migration Application

```cpp
void apply_migration(PopulationComponent& pop, const MigrationFactorComponent& factors) {
    uint32_t in_migration = calculate_migration_in(pop, factors);
    uint32_t out_migration = calculate_migration_out(pop, factors);

    // Apply changes
    pop.total_beings += in_migration;
    pop.total_beings = std::max(0u, pop.total_beings - out_migration);

    // Track statistics
    pop.net_migration = static_cast<int32_t>(in_migration) - static_cast<int32_t>(out_migration);

    // Update growth rate
    pop.growth_rate = pop.natural_growth + pop.net_migration;
}
```

---

## 7. Life Expectancy

### 7.1 Life Expectancy Model

Life expectancy is a derived statistic showing average expected lifespan. It's not used directly in simulation but is important for:
- UI statistics display
- Player feedback on health policy
- IStatQueryable interface compliance

### 7.2 Life Expectancy Calculation

```cpp
float calculate_life_expectancy(const PopulationComponent& pop,
                                 const MigrationFactorComponent& factors) {
    // Base life expectancy: 75 cycles (alien equivalent of years)
    const float BASE_LIFE_EXPECTANCY = 75.0f;

    // === Health Services Factor ===
    // Medical coverage improves life expectancy
    float health_factor = lerp(0.7f, 1.3f, pop.health_index / 100.0f);

    // === Contamination Factor ===
    // Contamination reduces life expectancy
    float contamination_factor = lerp(1.0f, 0.6f, factors.contamination_level / 100.0f);

    // === Disorder Factor ===
    // High disorder slightly reduces life expectancy (violence, stress)
    float disorder_factor = lerp(1.0f, 0.9f, factors.disorder_level / 100.0f);

    // === Education Factor ===
    // Higher education correlates with longer life
    float education_factor = lerp(0.95f, 1.1f, pop.education_index / 100.0f);

    // === Harmony Factor ===
    // Happier beings live longer
    float harmony_factor = lerp(0.9f, 1.1f, factors.harmony_level / 100.0f);

    float life_expectancy = BASE_LIFE_EXPECTANCY
                          * health_factor
                          * contamination_factor
                          * disorder_factor
                          * education_factor
                          * harmony_factor;

    return std::clamp(life_expectancy, 30.0f, 120.0f);  // Reasonable bounds
}
```

### 7.3 Health Index Calculation

Health index is affected by medical service coverage and contamination.

```cpp
void update_health_index(PopulationComponent& pop,
                          const IServiceProvider& services,
                          const ContaminationComponent& contamination,
                          PlayerID owner) {
    // Base health: 50
    float health = 50.0f;

    // Medical service coverage (from Epic 9)
    float medical_coverage = services.get_coverage(ServiceType::Medical, owner);
    health += (medical_coverage - 50) * 0.5f;  // +/- 25 from services

    // Contamination penalty
    health -= contamination.average_contamination_index * 0.3f;  // Up to -30

    // Fluid availability bonus (clean water = healthier)
    // Placeholder until FluidSystem integration
    float fluid_coverage = 1.0f;  // Assume full coverage for now
    health += (fluid_coverage - 0.5f) * 20.0f;  // +/- 10 from water

    pop.health_index = static_cast<uint8_t>(std::clamp(health, 0.0f, 100.0f));
}
```

### 7.4 IStatQueryable Implementation

```cpp
class PopulationSystem : public IStatQueryable {
public:
    float get_stat_value(StatID stat_id) const override {
        switch (stat_id) {
            case StatID::Population:
                return static_cast<float>(pop_.total_beings);
            case StatID::GrowthRate:
                return static_cast<float>(pop_.growth_rate);
            case StatID::BirthRate:
                return pop_.birth_rate_per_1000 / 1000.0f;
            case StatID::DeathRate:
                return pop_.death_rate_per_1000 / 1000.0f;
            case StatID::LifeExpectancy:
                return calculate_life_expectancy(pop_, factors_);
            case StatID::UnemploymentRate:
                return emp_.unemployment_rate / 100.0f;
            case StatID::HarmonyIndex:
                return pop_.harmony_index / 100.0f;
            default:
                return 0.0f;
        }
    }

    std::vector<float> get_stat_history(StatID stat_id, uint32_t periods) const override {
        // Return from ring buffer
        // Implementation depends on which stats we track historically
    }
};
```

---

## 8. Key Work Items

### Group A: Component Infrastructure

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P01 | PopulationComponent definition | S | Define component per Section 2.1 |
| 10-P02 | EmploymentComponent definition | S | Define component per Section 2.2 |
| 10-P03 | BuildingOccupancyComponent definition | S | Define component per Section 2.3 |
| 10-P04 | MigrationFactorComponent definition | S | Define component per Section 2.4 |
| 10-P05 | DemandComponent definition | S | Define component per Section 5.2 |
| 10-P06 | Population events | S | PopulationMilestoneEvent, EmploymentChangedEvent |

### Group B: Population Dynamics

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P07 | Birth rate calculation | M | Implement Section 3.2 |
| 10-P08 | Death rate calculation | M | Implement Section 3.3 |
| 10-P09 | Natural growth system | M | Combine birth/death, update totals |
| 10-P10 | Age distribution evolution | M | Implement Section 3.5 |

### Group C: Employment System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P11 | Labor pool calculation | M | Implement Section 4.2 |
| 10-P12 | Job market aggregation | M | Query buildings for capacity |
| 10-P13 | Employment matching algorithm | L | Implement Section 4.4 |
| 10-P14 | Building occupancy distribution | L | Distribute to individual buildings |
| 10-P15 | Unemployment effects | M | Harmony/disorder penalties |

### Group D: Demand System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P16 | IDemandProvider interface | S | Define interface per Section 5.7 |
| 10-P17 | Habitation demand formula | M | Implement Section 5.3 |
| 10-P18 | Exchange demand formula | M | Implement Section 5.4 |
| 10-P19 | Fabrication demand formula | M | Implement Section 5.5 |
| 10-P20 | Demand cap calculation | M | Implement Section 5.6 |
| 10-P21 | DemandSystem integration | L | Wire to BuildingSystem |

### Group E: Migration System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P22 | Attractiveness calculation | M | Implement Section 6.2 |
| 10-P23 | Migration in calculation | M | Implement Section 6.3 |
| 10-P24 | Migration out calculation | M | Implement Section 6.4 |
| 10-P25 | Migration application | M | Apply changes, update stats |

### Group F: Life Expectancy & Stats

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P26 | Life expectancy calculation | M | Implement Section 7.2 |
| 10-P27 | Health index system | M | Implement Section 7.3 |
| 10-P28 | IStatQueryable implementation | M | Stats API for UI |
| 10-P29 | Historical data ring buffer | M | Population history tracking |

### Group G: System Integration

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P30 | PopulationSystem class (ISimulatable) | L | Main system, priority 50 |
| 10-P31 | DemandSystem class | L | Main demand system |
| 10-P32 | BuildingSystem integration | L | Replace stub IDemandProvider |
| 10-P33 | Component serialization | M | Network sync for components |
| 10-P34 | Network messages | M | Population stat sync |

### Group H: Testing

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 10-P35 | Unit tests: demographics | L | Birth/death/growth tests |
| 10-P36 | Unit tests: employment | L | Job matching tests |
| 10-P37 | Unit tests: demand | L | RCI formula tests |
| 10-P38 | Integration test: full cycle | L | End-to-end population sim |

### Work Item Summary

| Size | Count |
|------|-------|
| S | 7 |
| M | 19 |
| L | 12 |
| **Total** | **38** |

### Critical Path

```
10-P01 (PopulationComponent) ----+
10-P02 (EmploymentComponent) ----|---> 10-P07/08/09 (Demographics)
10-P04 (MigrationFactors) -------+            |
                                              v
                                  10-P22/23/24/25 (Migration)
                                              |
10-P03 (BuildingOccupancy) ------+            v
10-P11/12 (Labor/Jobs) ----------+---> 10-P13/14 (Employment)
                                              |
                                              v
10-P05 (DemandComponent) --------+---> 10-P17/18/19 (Demand Formulas)
10-P16 (IDemandProvider) --------+            |
                                              v
                                      10-P30 (PopulationSystem)
                                      10-P31 (DemandSystem)
                                              |
                                              v
                                      10-P32 (Integration)
```

---

## 9. Questions for Other Agents

### @systems-architect

1. **Tick ordering between PopulationSystem and DemandSystem:** Canon shows PopulationSystem at priority 50, but DemandSystem has no explicit priority. Should DemandSystem run immediately after PopulationSystem (priority 51) or at the end of the simulation tick?

2. **Circular dependency concern:** PopulationSystem depends on BuildingSystem for capacity queries. BuildingSystem depends on DemandSystem for spawning decisions. DemandSystem depends on PopulationSystem for demand calculation. How do we handle this circular dependency? Current proposal: use previous tick's values.

3. **Per-player vs shared simulation:** Should PopulationSystem maintain completely separate state per player, or is there any shared population pool? Current assumption: fully separate per-player populations.

### @building-engineer

4. **Building occupancy update frequency:** When a structure transitions from Active to Abandoned, should PopulationSystem immediately evict occupants, or should eviction happen gradually over several ticks?

5. **Capacity notification:** Does BuildingSystem emit events when total capacity changes (building constructed/demolished), or should PopulationSystem query capacity every tick?

6. **BuildingOccupancyComponent ownership:** Should this component be attached by BuildingSystem (Epic 4) or PopulationSystem (Epic 10)? It straddles both domains.

### @economy-engineer

7. **Tribute rate query interface:** What interface does PopulationSystem use to query current tribute rates per zone type? Need an IEconomyQueryable or similar.

8. **Population-based revenue:** Does EconomySystem calculate tribute revenue, or does it query PopulationSystem for employed laborers and apply rates itself?

### @services-engineer

9. **Service coverage query:** What interface provides service coverage values (medical, education, enforcer)? Need an IServiceQueryable for PopulationSystem migration/health calculations.

10. **Education effects:** How does education service coverage translate to the education_index used in demand and longevity calculations?

### @game-designer

11. **Birth/death rate tuning:** Proposed base rates are 15 births / 8 deaths per 1000 per cycle. Is this the right pace for gameplay? Should growth be faster (more dynamic) or slower (more deliberate)?

12. **Unemployment thresholds:** Proposed thresholds: 10% = elevated, 15% = high, 25% = critical. Do these align with intended gameplay experience?

13. **Population milestones:** What population values trigger milestone events? Canon mentions 2000, 10000, 30000, 60000, 90000, 120000 for reward buildings. Should PopulationSystem emit events at these thresholds?

14. **Starting population:** What is the starting population for a new player? Zero (must attract migrants), or a seed population (e.g., 100 beings)?

### @infrastructure-engineer

15. **Commute satisfaction:** The EmploymentComponent has avg_commute_satisfaction. How is this calculated? Does it use TransportSystem connectivity/congestion data?

---

## 10. Performance Considerations

### 10.1 Aggregate vs Per-Entity Approach

**Key Decision: Aggregate population model**

We do NOT create entities for individual beings because:
- 100,000 beings as entities = massive memory and iteration cost
- Individual being AI/pathfinding is computationally infeasible
- Network sync would be overwhelming
- No gameplay benefit (players manage aggregate statistics, not individuals)

**Aggregate approach:**
- PopulationComponent per player: O(1) memory per player
- EmploymentComponent per player: O(1) memory per player
- BuildingOccupancyComponent per building: O(buildings) ~10,000 max

### 10.2 Building Iteration Performance

PopulationSystem must iterate buildings for:
- Capacity aggregation
- Occupancy distribution
- State checks (only active buildings count)

**Optimization strategies:**

1. **Pre-filtered views:** Maintain cached views for active buildings by zone type
2. **Dirty flags:** Only recalculate capacity when buildings change (use BuildingConstructedEvent, BuildingDemolishedEvent)
3. **Incremental updates:** Don't re-scan all buildings every tick; maintain running totals updated by events

```cpp
// Event-driven capacity tracking
void on_building_constructed(const BuildingConstructedEvent& event) {
    capacity_cache_[event.owner][event.zone_type] += event.capacity;
}

void on_building_demolished(const BuildingDemolishedEvent& event) {
    capacity_cache_[event.owner][event.zone_type] -= event.capacity;
}
```

### 10.3 Calculation Frequency

Not all calculations need to run every tick:

| Calculation | Frequency | Rationale |
|-------------|-----------|-----------|
| Employment matching | Every tick (50ms) | Responsive to building changes |
| Demand calculation | Every 5 ticks (250ms) | Demand changes slowly |
| Migration | Every 20 ticks (1s) | Migration is gradual |
| Birth/death | Every 100 ticks (5s) | Demographics change slowly |
| Life expectancy | Every 200 ticks (10s) | Derived stat, UI-only |
| Age distribution | Every 100 ticks (5s) | Tied to demographics |

### 10.4 Memory Budget

| Data | Size per player | 4 players |
|------|-----------------|-----------|
| PopulationComponent | ~90 bytes | 360 bytes |
| EmploymentComponent | ~45 bytes | 180 bytes |
| MigrationFactorComponent | ~12 bytes | 48 bytes |
| DemandComponent | ~40 bytes | 160 bytes |
| **Total per player** | ~187 bytes | **~750 bytes** |

Plus BuildingOccupancyComponent:
| Data | Size per building | 10,000 buildings |
|------|-------------------|------------------|
| BuildingOccupancyComponent | 9 bytes | 90 KB |

**Total memory footprint:** ~100 KB - negligible.

### 10.5 Tick Budget

Target: PopulationSystem tick < 2ms

Breakdown estimate:
- Capacity aggregation (cached): ~0.1ms
- Employment matching: ~0.3ms
- Building occupancy distribution (10,000 buildings): ~1.0ms
- Migration factors: ~0.1ms
- Demand calculation: ~0.2ms
- Stats update: ~0.1ms

**Total: ~1.8ms** - within budget

### 10.6 Profiling Points

Add profiling markers at:
- `PopulationSystem::tick()` entry/exit
- `match_employment()` start/end
- `distribute_building_occupancy()` start/end
- `calculate_demand()` start/end

---

## 11. Multiplayer Considerations

### 11.1 Per-Player Population

Each player has completely independent population state:
- Own PopulationComponent
- Own EmploymentComponent
- Own MigrationFactorComponent
- Own DemandComponent

**No shared population pool.** Players compete for migrants but don't share beings.

### 11.2 Server Authority

| Data | Authority | Notes |
|------|-----------|-------|
| Population counts | Server | Authoritative, synced to clients |
| Birth/death events | Server | Calculated server-side only |
| Migration | Server | Server determines who moves where |
| Employment matching | Server | Server distributes laborers |
| Demand values | Server | Server calculates, synced to clients |
| Building occupancy | Server | Server sets occupancy values |

Clients receive:
- Updated PopulationComponent values
- Updated EmploymentComponent values
- Updated DemandComponent values
- Periodic building occupancy deltas

### 11.3 Sync Frequency

| Data | Sync Frequency | Notes |
|------|----------------|-------|
| Population totals | Every 20 ticks (1s) | Major stat |
| Employment stats | Every 20 ticks (1s) | Major stat |
| Demand values | Every 20 ticks (1s) | Affects UI demand bars |
| Building occupancy | On change + every 100 ticks | Per-building detail |
| Migration factors | Every 100 ticks (5s) | Background stat |

### 11.4 Component Serialization

```cpp
// PopulationComponent serialization (subset for network)
struct PopulationNetworkData {
    uint32_t total_beings;
    int32_t growth_rate;
    uint8_t harmony_index;
    uint8_t health_index;
    uint16_t birth_rate_per_1000;
    uint16_t death_rate_per_1000;
};
// Size: 14 bytes per player

// EmploymentNetworkData
struct EmploymentNetworkData {
    uint32_t employed_laborers;
    uint32_t unemployed;
    uint8_t unemployment_rate;
};
// Size: 9 bytes per player

// DemandNetworkData
struct DemandNetworkData {
    int8_t habitation_demand;
    int8_t exchange_demand;
    int8_t fabrication_demand;
};
// Size: 3 bytes per player
```

**Total per-player sync:** ~26 bytes per sync interval

With 4 players syncing every second: 104 bytes/second - negligible bandwidth.

### 11.5 Migration Competition

When multiple players have positive net_attraction, migrants are distributed based on relative attractiveness:

```cpp
void distribute_global_migrants(uint32_t total_migrants,
                                 std::vector<PlayerID> players,
                                 std::map<PlayerID, MigrationFactorComponent>& factors) {
    // Calculate total attraction
    int total_attraction = 0;
    for (auto player : players) {
        total_attraction += std::max(0, factors[player].net_attraction);
    }

    if (total_attraction == 0) return;  // No one is attractive

    // Distribute proportionally
    for (auto player : players) {
        int attraction = std::max(0, factors[player].net_attraction);
        float share = static_cast<float>(attraction) / total_attraction;
        uint32_t migrants = static_cast<uint32_t>(total_migrants * share);

        // Apply to player (capped by their housing availability)
        apply_migration_in(player, migrants);
    }
}
```

### 11.6 Late Join

When a new player joins mid-game:
- Start with 0 population (or configurable seed)
- Migration attractiveness calculated normally
- Will attract migrants if colony has jobs/housing and good conditions
- No instant population injection

### 11.7 Player Disconnect / Abandonment

When a player disconnects or abandons (ghost town process):
- Population enters "stasis" - no births, minimal deaths
- Buildings transition to Abandoned state (handled by BuildingSystem)
- As buildings become Derelict, capacity decreases
- Population gradually dies off or migrates out
- Eventually population reaches 0

This is handled by the ghost town process in AbandonmentSystem, not PopulationSystem directly.

---

## Appendix A: Formula Summary

### Birth Rate
```
effective_birth_rate = BASE_BIRTH_RATE (0.015)
                     * harmony_modifier (0.5-1.5)
                     * health_modifier (0.7-1.2)
                     * housing_modifier (0.1-1.0)
```

### Death Rate
```
effective_death_rate = BASE_DEATH_RATE (0.008)
                     * health_modifier (0.5-1.5)
                     * contamination_modifier (1.0-2.0)
                     * service_modifier (0.7-1.3)
                     * age_modifier (0.5-2.0)
```

### Life Expectancy
```
life_expectancy = BASE (75)
                * health_factor (0.7-1.3)
                * contamination_factor (0.6-1.0)
                * disorder_factor (0.9-1.0)
                * education_factor (0.95-1.1)
                * harmony_factor (0.9-1.1)
```

### Migration In
```
migration_in = BASE_MIGRATION (50)
             * attraction_multiplier (0.0-2.0)
             * size_factor (1.0-2.0)
             capped by available_housing
```

### Migration Out
```
migration_out = population
              * effective_out_rate (0.005 base, up to 0.05)
              * desperation_factor (0.0+)
```

### Unemployment Rate
```
unemployment_rate = (labor_force - employed) / labor_force * 100
```

### Habitation Demand
```
demand = occupancy_factor (-10 to +30)
       + employment_factor (-unemployment to +20)
       + services_factor (-10 to +10)
       + tribute_factor (-rate to +5)
       - contamination_factor (0 to -20)
```

---

## Appendix B: Stat IDs for IStatQueryable

```cpp
enum class StatID : uint16_t {
    // Population stats
    Population = 100,
    GrowthRate = 101,
    BirthRate = 102,
    DeathRate = 103,
    NetMigration = 104,
    LifeExpectancy = 105,
    HarmonyIndex = 106,
    HealthIndex = 107,
    EducationIndex = 108,

    // Employment stats
    LaborForce = 200,
    EmployedLaborers = 201,
    Unemployed = 202,
    UnemploymentRate = 203,
    TotalJobs = 204,
    ExchangeJobs = 205,
    FabricationJobs = 206,

    // Demand stats
    HabitationDemand = 300,
    ExchangeDemand = 301,
    FabricationDemand = 302,

    // Capacity stats
    HousingCapacity = 400,
    HousingOccupancy = 401,
};
```

---

## Appendix C: Event Definitions

```cpp
struct PopulationMilestoneEvent {
    PlayerID player;
    uint32_t milestone;  // Population threshold reached
    uint32_t current_population;
};

struct EmploymentChangedEvent {
    PlayerID player;
    uint32_t previous_employed;
    uint32_t current_employed;
    int32_t change;
};

struct DemandChangedEvent {
    PlayerID player;
    ZoneBuildingType zone_type;
    int8_t previous_demand;
    int8_t current_demand;
};

struct MigrationEvent {
    PlayerID player;
    int32_t net_migration;
    uint32_t migration_in;
    uint32_t migration_out;
};
```

---

*End of Population Engineer Analysis*
