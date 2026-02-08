# Energy Balance: Pool State Thresholds

**Ticket:** 5-039
**Status:** Verified
**Related:** 5-013 (Pool State Machine), 5-012 (Pool Calculation)

## Pool State Thresholds

The energy pool state machine uses two configurable threshold constants (defined in `EnergySystem.h`) to determine transitions between the four pool health states: **Healthy**, **Marginal**, **Deficit**, and **Collapse**.

### buffer_threshold

- **Constant:** `BUFFER_THRESHOLD_PERCENT = 0.10f`
- **Derivation:** `buffer_threshold = total_generated * 0.10`
- **Role:** Determines the boundary between Healthy and Marginal states.
- **Transition rule:**
  - `surplus >= buffer_threshold` => **Healthy**
  - `0 <= surplus < buffer_threshold` => **Marginal**

### collapse_threshold

- **Constant:** `COLLAPSE_THRESHOLD_PERCENT = 0.50f`
- **Derivation:** `collapse_threshold = total_consumed * 0.50`
- **Role:** Determines the boundary between Deficit and Collapse states.
- **Transition rule:**
  - `surplus < 0 AND surplus > -collapse_threshold` => **Deficit**
  - `surplus <= -collapse_threshold` => **Collapse**

### Full State Diagram

```
surplus >= buffer_threshold          => Healthy
0 <= surplus < buffer_threshold      => Marginal
-collapse_threshold < surplus < 0    => Deficit
surplus <= -collapse_threshold       => Collapse
```

## Rationale for Chosen Values

### Buffer Threshold: 10%

A 10% buffer provides a meaningful safety margin without being too forgiving. At this level:

- A city generating 1000 units needs at least 100 units of headroom to remain Healthy.
- Losing a single small nexus (typically 50-200 units) can push a balanced city from Healthy to Marginal, which serves as an early warning.
- The threshold is low enough that players do not need to massively overbuild energy infrastructure just to maintain Healthy status.
- It creates a narrow but meaningful Marginal band where the player knows they are operating close to capacity.

### Collapse Threshold: 50%

A 50% collapse threshold means the deficit must equal half of total consumption before collapse triggers:

- This prevents brief or minor deficits from immediately triggering grid collapse and its associated penalties (widespread outages, potential cascade effects).
- A city consuming 1000 units only collapses if the deficit exceeds 500 units, meaning generation has dropped below 500 (less than half of demand).
- This gives players a window to respond to Deficit state before Collapse occurs.
- The threshold scales with consumption, so larger cities have proportionally larger absolute thresholds, which matches the intuition that larger grids have more inertia.

### Edge Cases

- **Zero generation, zero consumption:** `surplus=0, buffer_threshold=0` => `0 >= 0` => Healthy. An empty city with no energy infrastructure is healthy by default.
- **Zero generation, nonzero consumption:** `surplus < 0, collapse_threshold = consumed * 0.50`. With zero generation, the deficit equals total consumption, which always exceeds the 50% collapse threshold => Collapse.
- **Exact boundary values:** The `>=` and `<=` operators on the boundary values are intentional. Exactly meeting the buffer threshold counts as Healthy; exactly meeting the collapse threshold counts as Collapse.

## Playtest Notes

<!-- Placeholder for playtest observations. Update after playtesting sessions. -->

- [ ] Verify 10% buffer feels right for small cities (< 500 total consumption)
- [ ] Verify 10% buffer feels right for large cities (> 5000 total consumption)
- [ ] Verify 50% collapse threshold gives adequate response time before collapse
- [ ] Test scenario: losing a single large nexus while at Marginal
- [ ] Test scenario: gradual demand growth pushing from Healthy through Marginal to Deficit
- [ ] Consider whether thresholds should vary by game difficulty setting
- [ ] Consider whether thresholds should be displayed in the energy UI overlay
