# Energy Overlay UI Requirements

**Ticket:** 5-040
**Epic:** 5 - Energy System
**Status:** Spec

## Overview

The energy overlay is a toggleable map overlay that visualizes the state of the energy
distribution network for the active player. It shows coverage zones, powered/unpowered
structures, conduit glow states, deficit/collapse warnings, and pool status readout.

## Visual Elements

### 1. Coverage Zones Colored by Owner

Each tile covered by a player's energy network is tinted with that player's color.
Overlapping ownership is not possible (one owner per tile), so colors are exclusive.

- **Player 0:** Blue tint
- **Player 1:** Red tint
- **Player 2:** Green tint
- **Player 3:** Yellow tint
- **Uncovered tiles:** No tint (base terrain visible)

Coverage data is read from the CoverageGrid via `get_coverage_at(x, y)` which returns
the overseer_id (1-4) or 0 for uncovered tiles.

### 2. Powered vs Unpowered Structure Indicators

Structures within coverage are marked as powered or unpowered:

- **Powered:** Green checkmark icon or green highlight on the structure sprite
- **Unpowered:** Red X icon or red highlight on the structure sprite
- **Outside coverage:** Dimmed/grayed out (no coverage zone color underneath)

Data source: `EnergyComponent.is_powered` on each entity.

### 3. Conduit Glow States

Conduits display a visual glow based on their operational state:

| State        | Visual                              | Data Source                           |
|------------- |-------------------------------------|---------------------------------------|
| Active       | Bright glow (pulsing animation)     | `EnergyConduitComponent.is_active == true`  |
| Inactive     | Dim glow (static)                   | `is_active == false && is_connected == true` |
| Disconnected | No glow, dashed outline             | `is_connected == false`               |

Active means the conduit is both connected to the network and the player has nonzero
energy generation. Inactive means connected but no generation. Disconnected means
the conduit is isolated from all nexuses.

### 4. Deficit/Collapse Warnings

When the player's energy pool enters deficit or collapse state, visual warnings appear:

- **Deficit (EnergyPoolState::Deficit):**
  - Amber warning banner at top of screen: "ENERGY DEFICIT - [X] units short"
  - Pulsing amber border around the energy overlay panel
  - Affected unpowered structures flash amber

- **Collapse (EnergyPoolState::Collapse):**
  - Red alert banner at top of screen: "GRID COLLAPSE - Critical energy failure"
  - Pulsing red border around the energy overlay panel
  - All unpowered structures flash red
  - Audible alarm (AudioSystem integration)

Warning data is driven by `get_pool_state(owner)` returning `Deficit` or `Collapse`.

### 5. Pool Status Readout

A persistent panel (bottom-right corner when overlay is active) displays:

| Field           | Value                         | Source                              |
|-----------------|-------------------------------|-------------------------------------|
| Total Generated | "1,250 / tick"                | `get_pool(owner).total_generated`   |
| Total Consumed  | "980 / tick"                  | `get_pool(owner).total_consumed`    |
| Surplus/Deficit | "+270" or "-150"              | `get_pool(owner).surplus`           |
| Pool State      | "Healthy" / "Marginal" / etc. | `get_pool_state(owner)`             |
| Nexus Count     | "5 nexuses"                   | `get_pool(owner).nexus_count`       |
| Consumer Count  | "42 structures"               | `get_pool(owner).consumer_count`    |

Color coding for surplus line:
- Green: surplus > 0
- Yellow: surplus == 0 or Marginal state
- Red: surplus < 0

## Data Queries from EnergySystem

The overlay renderer requires the following queries from EnergySystem:

### Per-Tile Queries (called per visible tile)

| Query                              | Return Type | Description                                  |
|------------------------------------|-------------|----------------------------------------------|
| `get_coverage_at(x, y)`           | `uint8_t`   | Overseer ID (1-4) or 0, for overlay coloring |
| `is_in_coverage(x, y, owner)`     | `bool`      | Whether tile is in coverage for a player     |

### Per-Player Queries (called once per frame)

| Query                    | Return Type              | Description                       |
|--------------------------|--------------------------|-----------------------------------|
| `get_pool(owner)`        | `PerPlayerEnergyPool&`   | Full pool data for status readout |
| `get_pool_state(owner)`  | `EnergyPoolState`        | Pool health for warning states    |

### Per-Entity Queries (called per visible entity)

| Query                                | Return Type | Description                         |
|--------------------------------------|-------------|-------------------------------------|
| `EnergyComponent.is_powered`         | `bool`      | Structure powered indicator         |
| `EnergyConduitComponent.is_active`   | `bool`      | Conduit glow: active vs inactive    |
| `EnergyConduitComponent.is_connected`| `bool`      | Conduit glow: connected vs isolated |

## Performance Requirements

- **Per-tile queries:** `get_coverage_at()` and `is_in_coverage()` are O(1) array lookups
  in CoverageGrid. These are called for every visible tile each frame when the overlay
  is active.
- **Target:** Overlay queries for a full viewport of visible tiles (typically 40x30 = 1,200
  tiles on screen) must complete within 1ms to avoid framerate drops.
- **Per-player queries:** Called once per frame. Negligible cost (direct struct field access).
- **Per-entity queries:** ECS component lookups via EnTT registry. Expected O(1) per entity.
  With ~200 visible entities, total cost should be under 0.5ms.
- **Total overlay budget:** Under 2ms per frame to maintain 60fps without impact.

## Toggle Behavior

- The energy overlay is toggled via a keybind (default: `E` key) or toolbar button.
- When toggled off, no per-tile queries are made (zero overhead).
- When toggled on, the overlay renders on top of the base terrain/structure layer.
- Only one utility overlay can be active at a time (energy, water, etc.).

## Integration Notes

- The overlay renderer reads data from EnergySystem but does not modify it.
- All data access is read-only and thread-safe for single-threaded rendering.
- Coverage grid data is updated once per simulation tick, not per render frame.
- The overlay should interpolate or cache coverage data between ticks for smooth visuals.
