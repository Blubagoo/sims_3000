# Fluid Deficit/Collapse Threshold Balance

**Ticket:** 6-041
**Status:** Draft
**Related:** 6-017 (Pool Calculation), 6-018 (Pool State Machine), 5-013 (Energy Pool State Machine)

## Overview

This document records the threshold values used to determine fluid pool health state transitions, compares them with the energy system, and provides rationale for the chosen values. All threshold logic is implemented in `FluidSystem::calculate_pool_state()`.

## Threshold Definitions

### Buffer Threshold (Healthy vs Marginal Boundary)

**Value:** 10% of `available` supply

```
buffer_threshold = available * 0.10
```

- **Healthy:** `surplus >= buffer_threshold` (10% or more headroom)
- **Marginal:** `0 <= surplus < buffer_threshold` (operating at capacity)

The buffer threshold represents the minimum surplus required for the pool to be considered healthy. When surplus drops below 10% of available supply, the system enters Marginal state as an early warning that capacity is nearing its limit.

### Deficit Condition

**Condition:** `surplus < 0 AND total_reservoir_stored > 0`

```
surplus = available - total_consumed
available = total_generated + total_reservoir_stored
```

When consumption exceeds available supply, the pool enters Deficit. In Deficit state, reservoirs are actively draining to cover the shortfall. The pool remains in Deficit (rather than Collapse) as long as reservoir buffer remains.

### Collapse Condition

**Condition:** `surplus < 0 AND total_reservoir_stored == 0`

Collapse occurs when the deficit cannot be buffered because all reservoirs are empty. This is the critical failure state where all consumers lose fluid simultaneously (all-or-nothing distribution per CCR-002).

### Empty Grid (Special Case)

**Condition:** `total_generated == 0 AND total_consumed == 0`

An empty grid with no extractors and no consumers is always Healthy. This prevents false warnings during initial city setup.

## State Transition Summary

```
                    surplus >= 10% available
Healthy  <-----------------------------------------+
   |                                                |
   |  surplus < 10% available but >= 0              |
   v                                                |
Marginal <------------------------------------------+
   |                                                |
   |  surplus < 0                                   |
   v                                                |
Deficit  <---- reservoir_stored > 0 ----+           |
   |                                    |           |
   |  reservoir_stored == 0             |           |
   v                                    |           |
Collapse ---- reservoir filled ---------+           |
         ---- surplus restored ---------------------+
```

## Configurable Values

All threshold values are designed to be tunable for balance testing:

| Parameter | Current Value | Location | Notes |
|-----------|--------------|----------|-------|
| Buffer threshold percent | 10% | `calculate_pool_state()` | Healthy vs Marginal boundary |
| Reservoir capacity | 1000 fluid | `FluidReservoirConfig.h` | Determines buffer duration |
| Reservoir fill rate | 50 fluid/tick | `FluidReservoirConfig.h` | How fast buffer recovers |
| Reservoir drain rate | 100 fluid/tick | `FluidReservoirConfig.h` | How fast buffer depletes |
| Extractor base output | 100 fluid/tick | `FluidExtractorConfig.h` | Baseline generation |

### Tuning Guidelines

- **Increasing buffer threshold** (e.g., 15%): Makes Healthy harder to achieve, more time in Marginal state. Creates earlier warnings but may feel overly cautious.
- **Decreasing buffer threshold** (e.g., 5%): Delays Marginal warnings, less reaction time before Deficit.
- **Increasing reservoir capacity**: Extends Deficit duration before Collapse. More forgiving but may reduce urgency.
- **Increasing drain rate relative to fill rate**: Makes Collapse happen faster once in Deficit. Creates more urgency.

## Comparison with Energy Thresholds

| Property | Energy System | Fluid System |
|----------|--------------|--------------|
| **Buffer threshold** | 10% of `total_generated` | 10% of `available` (generated + stored) |
| **Deficit condition** | `surplus < 0` | `surplus < 0 AND reservoir_stored > 0` |
| **Collapse condition** | `surplus <= -(total_consumed * collapse_threshold)` | `surplus < 0 AND reservoir_stored == 0` |
| **Collapse threshold %** | Configurable (% of consumption) | N/A (binary: reservoirs empty or not) |
| **Buffer mechanism** | Grace period (100 ticks, CCR-006) | Reservoir drain (physical storage) |
| **Recovery mechanism** | Build more nexuses | Build more extractors or refill reservoirs |
| **Distribution during deficit** | Priority-based rationing | All-or-nothing (CCR-002) |

### Key Differences

1. **Buffer threshold base**: Energy uses `total_generated` as the denominator, while fluid uses `available` (which includes reservoir stored). This means fluid's Healthy threshold accounts for stored reserves, making it more generous when reservoirs are full.

2. **Collapse mechanism**: Energy collapse is a graduated threshold (percentage of consumption), while fluid collapse is binary (reservoirs empty = collapse). This reflects the physical nature of water storage -- once the tank is empty, there is no water.

3. **No grace period**: Energy has a 100-tick grace period (CCR-006) before cutting power to consumers. Fluid has NO grace period -- reservoir drain serves this purpose instead. When reservoirs empty, fluid cuts off immediately. This creates a different gameplay dynamic: players see reservoir levels dropping as an explicit warning rather than an invisible countdown.

4. **All-or-nothing vs rationing**: Energy uses priority-based rationing during deficit (critical infrastructure stays powered). Fluid uses all-or-nothing distribution (CCR-002) -- either everyone has water or nobody does. This simplifies the fluid system and creates a more dramatic failure state.

## Playtest Rationale

The 10% buffer threshold was chosen to match the energy system for consistency in player mental models. When players see "Marginal" in both energy and fluid overlays, the meaning is analogous: "you are running near capacity, consider expanding."

The binary collapse condition (reservoirs empty) was chosen because:
- It maps to an intuitive physical reality (no water in the tank)
- It provides a clear, visible warning mechanism (reservoir level dropping)
- It avoids the complexity of percentage-based collapse thresholds
- It makes reservoir construction a meaningful strategic decision

### Playtest Checklist

- [ ] Verify 10% buffer threshold triggers Marginal early enough for player reaction
- [ ] Verify reservoir drain provides adequate warning time before Collapse
- [ ] Test scenario: gradually increasing population with fixed extractor count
- [ ] Test scenario: losing an extractor while reservoirs are at various fill levels
- [ ] Test scenario: simultaneous energy + fluid deficit (cascading failure)
- [ ] Consider whether buffer threshold should differ between energy and fluid
- [ ] Consider whether partial fluid service (rationing) would improve gameplay
