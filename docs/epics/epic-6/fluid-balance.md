# Fluid Balance: Extractor & Reservoir Stats

**Ticket:** 6-040
**Status:** Draft
**Related:** 6-023 (ExtractorConfig), 6-024 (ReservoirConfig), 6-027 (Extractor Placement), 6-028 (Reservoir Placement)

## Overview

This document records the balance values and design rationale for the fluid (water) infrastructure system. All values are defined as named constants in `FluidExtractorConfig.h` and `FluidReservoirConfig.h`.

## Extractor Balance

| Stat | Value | Constant |
|------|-------|----------|
| Base output | 100 fluid/tick | `EXTRACTOR_DEFAULT_BASE_OUTPUT` |
| Build cost | 3000 credits | `EXTRACTOR_DEFAULT_BUILD_COST` |
| Maintenance cost | 30 credits/cycle | `EXTRACTOR_DEFAULT_MAINTENANCE_COST` |
| Energy required | 20 units | `EXTRACTOR_DEFAULT_ENERGY_REQUIRED` |
| Energy priority | 2 (Important) | `EXTRACTOR_DEFAULT_ENERGY_PRIORITY` |
| Max placement distance | 8 tiles | `EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE` |
| Max operational distance | 5 tiles | `EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE` |
| Coverage radius | 8 tiles | `EXTRACTOR_DEFAULT_COVERAGE_RADIUS` |

### Output Rationale: 100 fluid/tick

An extractor producing 100 fluid per tick can supply approximately **20 low-density habitations** at 5 fluid units each (`FLUID_REQ_HABITATION_LOW = 5`). This creates a ratio where a single extractor supports a small neighborhood, encouraging players to build extractors proportionally to residential growth.

At high density (10 units each), a single extractor supports ~10 habitations. Exchange zones (3 low / 8 high) and fabrication zones (8 low / 15 high) consume more, requiring dedicated extractors for industrial districts.

### Build Cost Rationale: 3000 credits

Extractors cost **3000 credits**, which is higher than a reservoir (2000) but comparable to energy nexuses. This reflects:
- Extractors are active producers requiring mechanical infrastructure
- The cost should be significant enough to prevent careless over-building
- ROI is reasonable: a single extractor serves ~20 habitations, which collectively generate tax revenue exceeding the build cost within a reasonable number of ticks

### Energy Cost Rationale: 20 units

Extractors consume **20 energy units**, which ensures they need dedicated power infrastructure. For context:
- A Carbon nexus produces 200 energy
- 20 units = 10% of a Carbon nexus's output
- A player can power ~10 extractors from a single Carbon nexus
- This creates an intentional coupling between the energy and fluid systems: you cannot build water infrastructure without first establishing power

Energy priority is set to **Important (2)** per CCR-008, meaning extractors are prioritized over normal consumers (zone buildings at priority 3) but below critical infrastructure (priority 1) during rationing.

## Reservoir Balance

| Stat | Value | Constant |
|------|-------|----------|
| Capacity | 1000 fluid | `RESERVOIR_DEFAULT_CAPACITY` |
| Fill rate | 50 fluid/tick | `RESERVOIR_DEFAULT_FILL_RATE` |
| Drain rate | 100 fluid/tick | `RESERVOIR_DEFAULT_DRAIN_RATE` |
| Build cost | 2000 credits | `RESERVOIR_DEFAULT_BUILD_COST` |
| Maintenance cost | 20 credits/cycle | `RESERVOIR_DEFAULT_MAINTENANCE_COST` |
| Coverage radius | 6 tiles | `RESERVOIR_DEFAULT_COVERAGE_RADIUS` |
| Requires energy | No (passive) | `RESERVOIR_DEFAULT_REQUIRES_ENERGY` |

### Capacity Rationale: 1000 fluid

At a consumption rate of 100 fluid/tick (one extractor's full output), a reservoir provides approximately **10 ticks of buffer**. This serves as emergency storage that can absorb brief supply disruptions without immediate fluid deficit.

For a neighborhood consuming 50 fluid/tick, the buffer extends to 20 ticks, giving the player meaningful time to respond to infrastructure failures.

### Asymmetric Rates Rationale (CCR-005)

The fill and drain rates are deliberately asymmetric:
- **Fill rate: 50 fluid/tick** -- 20 ticks to fill from empty
- **Drain rate: 100 fluid/tick** -- 10 ticks to drain from full

This asymmetry (drain 2x faster than fill) serves critical balance goals:
1. **Prevents storage-as-substitute**: Players cannot rely on reservoirs instead of building extractors. Reservoirs drain much faster than they fill, so they are a buffer, not a replacement for production.
2. **Creates urgency**: When fluid supply is disrupted, reserves deplete quickly, forcing players to address the root cause rather than waiting for reservoirs to refill.
3. **Rewards over-production**: Players who build excess extraction capacity see their reservoirs stay full, creating a visible indicator of a healthy fluid network.

### Build Cost Rationale: 2000 credits

Reservoirs cost **2000 credits**, cheaper than extractors (3000) because:
- They are passive storage (no energy cost)
- They do not produce fluid, only buffer it
- Players should be encouraged to build reservoirs as network buffers without feeling penalized

### No Energy Requirement

Reservoirs are **passive storage** and do not require energy. This design choice:
- Simplifies reservoir placement (no dependency on energy coverage)
- Allows reservoirs to function during energy deficits, providing critical fluid buffer
- Differentiates reservoirs from extractors as a distinct infrastructure type

## Coverage Comparison

| Structure | Coverage Radius | Rationale |
|-----------|----------------|-----------|
| Extractor | 8 tiles | Larger radius because extractors are the primary distribution source |
| Reservoir | 6 tiles | Smaller radius because reservoirs supplement, not replace, extractors |

The extractor's larger coverage radius (8 vs 6) means players can space extractors further apart while maintaining coverage. Reservoirs fill gaps and extend coverage but should not replace extractors as the primary coverage providers.

## Water Proximity Efficiency Curve

Extractors have a distance-dependent efficiency multiplier that simulates diminishing returns as the extractor is placed farther from water:

| Distance (tiles) | Efficiency | Rationale |
|-------------------|-----------|-----------|
| 0 | 100% | Directly on water source -- maximum extraction |
| 1-2 | 90% | Very close, minimal efficiency loss |
| 3-4 | 70% | Close, noticeable efficiency drop |
| 5-6 | 50% | Moderate distance, significant loss |
| 7-8 | 30% | Far, barely operational |
| 9+ | 0% | Too far, cannot extract at all |

### Curve Design Rationale

The curve uses a **tiered step function** rather than a continuous formula for several reasons:
- **Clarity**: Players can easily understand discrete tiers (green/yellow/red zones on the UI)
- **Predictability**: No fractional surprises; efficiency is deterministic within each tier
- **Meaningful choices**: The sharp drops at tier boundaries (90 to 70, 70 to 50) create clear tradeoffs for placement decisions

The maximum placement distance (8 tiles) combined with low efficiency (30%) at that distance creates a soft boundary -- technically placeable but clearly suboptimal. This encourages but does not force placement near water.

The maximum operational distance (5 tiles, 50% efficiency) marks the boundary where the extractor drops to half capacity, serving as a practical guideline for "good" placement.

## Comparison with Energy System

| Property | Energy (Nexus) | Fluid (Extractor) |
|----------|---------------|-------------------|
| Base output | 50-400 (varies by type) | 100 (fixed) |
| Build cost | 1000-6000 (varies by type) | 3000 (fixed) |
| Coverage radius | 5-8 (varies by type) | 8 (fixed) |
| Energy required | N/A (producer) | 20 units |
| Proximity requirement | None (except Hydro stub) | Water distance <= 8 tiles |
| Distance efficiency | None | 30%-100% based on water distance |
| Aging/degradation | Yes (type-specific decay) | Not yet implemented |
| Storage equivalent | None | Reservoir (1000 capacity) |

Key differences:
- Fluid extractors have a **proximity constraint** that energy nexuses lack (except stubbed Hydro/Geothermal). This adds spatial strategy to fluid infrastructure placement.
- Fluid has a **storage buffer** (reservoirs) that energy does not. This creates a different failure dynamic -- fluid shortages can be temporarily absorbed.
- Energy nexuses have **type diversity** (6 types with different stats), while fluid extractors are currently **uniform**. Future expansion may add specialized extractor types.
- Energy has **aging/degradation** that reduces output over time. Fluid extractors do not age (yet), keeping the system simpler for initial implementation.

## Playtest Notes

<!-- Placeholder for playtest observations. Update after playtesting sessions. -->

- [ ] Verify 100 output per extractor feels right for early-game city growth
- [ ] Verify 1000 reservoir capacity provides meaningful buffer without making extractors feel unnecessary
- [ ] Verify asymmetric fill/drain rates create appropriate urgency during supply disruptions
- [ ] Test scenario: expanding residential zones with insufficient extractor capacity
- [ ] Test scenario: losing an extractor while at full reservoir capacity -- how long until deficit?
- [ ] Verify 20 energy cost creates a meaningful power dependency without being punitive
- [ ] Test scenario: energy deficit cutting off fluid extractors -- cascading failure dynamics
- [ ] Consider whether extractor types (shallow/deep/artesian) should be added for variety
- [ ] Consider whether reservoir capacity should scale with build cost tiers
