# Fluid Overlay UI Requirements

**Ticket:** 6-042
**Status:** Draft
**Related:** 6-019 (Fluid Distribution), 6-022 (Pool Events), 5-031 (Energy Overlay Pattern)

## Overview

This document specifies the visual requirements for the fluid infrastructure overlay. The overlay displays coverage zones, structure states, conduit flow, reservoir levels, and deficit/collapse warnings. All data is queried from FluidSystem at render time.

## Color Palette

The fluid overlay uses a **blue/cyan palette** to distinguish it from the energy system's warm amber tones.

| Element | Color | Hex | Notes |
|---------|-------|-----|-------|
| Coverage zone (healthy) | Light blue | `#4FC3F7` | Translucent fill over covered tiles |
| Coverage zone (marginal) | Cyan | `#00BCD4` | Slightly different hue to indicate caution |
| Coverage zone (deficit) | Dark blue | `#1565C0` | Muted to indicate degraded service |
| Coverage zone (collapse) | Gray-blue | `#78909C` | Desaturated to indicate failure |
| Uncovered area | No overlay | -- | Grid tiles without coverage have no fluid overlay |

## Structure Visual States

### Hydrated Structures (has_fluid = true)

- **Indicator:** Blue glow effect around the building sprite
- **Color:** `#42A5F5` (medium blue, 50% opacity pulse)
- **Animation:** Subtle pulse at 1Hz (gentle breathing effect)
- **Query:** `FluidComponent.has_fluid == true`

### Dehydrated Structures (has_fluid = false)

- **Indicator:** Gray/dull desaturation filter on building sprite
- **Color:** Apply grayscale filter with `#9E9E9E` tint
- **Animation:** None (static desaturated state)
- **Query:** `FluidComponent.has_fluid == false`
- **Additional:** Small red droplet icon above building if in coverage but no fluid (deficit/collapse)

### Out of Coverage

- **Indicator:** No fluid overlay or indicator
- **Query:** `get_coverage_at(x, y) == 0` for the building's position

## Conduit Visual States

Conduits are rendered as thin lines or pipe segments connecting infrastructure.

### Active Flow (is_active = true)

- **Color:** Bright blue `#2196F3`
- **Animation:** Flowing pulse animation along the conduit direction
- **Pulse speed:** 2 tiles/second
- **Pulse width:** 1 tile
- **Query:** `FluidConduitComponent.is_active == true`

### Inactive (is_connected = true, is_active = false)

- **Color:** Dim blue `#90CAF9` (40% opacity)
- **Animation:** None (static dim)
- **Meaning:** Connected to network but no fluid flowing (no generation)
- **Query:** `FluidConduitComponent.is_connected == true && is_active == false`

### Disconnected (is_connected = false)

- **Color:** Gray `#BDBDBD`
- **Animation:** None (static gray)
- **Meaning:** Not reachable from any extractor/reservoir via BFS
- **Query:** `FluidConduitComponent.is_connected == false`

## Reservoir Fill Level Display

Each reservoir renders a fill gauge visualization.

### Bar Gauge

- **Position:** Floating above the reservoir building sprite
- **Size:** 24x6 pixels (at 1x zoom)
- **Background:** Dark blue `#0D47A1` (empty portion)
- **Fill color:** Gradient from `#1E88E5` (low) to `#42A5F5` (high)
- **Border:** 1px `#1565C0`
- **Fill level:** `current_level / capacity` as percentage

### Numeric Display (info panel)

When the player selects a reservoir, the info panel shows:
- Current level: `FluidReservoirComponent.current_level`
- Capacity: `FluidReservoirComponent.capacity`
- Fill percentage: `(current_level / capacity) * 100%`
- Fill rate: `FluidReservoirComponent.fill_rate` fluid/tick
- Drain rate: `FluidReservoirComponent.drain_rate` fluid/tick
- Status: "Filling" / "Draining" / "Full" / "Empty" / "Idle"

### Fill Level Thresholds

| Level | Gauge Color | Status |
|-------|------------|--------|
| 75-100% | `#42A5F5` | Full/Healthy |
| 50-74% | `#1E88E5` | Adequate |
| 25-49% | `#FF9800` | Low (orange warning) |
| 0-24% | `#F44336` | Critical (red warning) |

## Deficit/Collapse Warnings

### Deficit Warning

- **Trigger:** `FluidDeficitBeganEvent` emitted
- **Visual:** Orange warning banner at top of screen: "Fluid deficit -- reservoirs draining"
- **Icon:** Orange water droplet with exclamation mark
- **Color:** `#FF9800` (amber/orange)
- **Duration:** Persistent while pool state == Deficit
- **Dismiss:** Auto-clears when `FluidDeficitEndedEvent` emitted

### Collapse Warning

- **Trigger:** `FluidCollapseBeganEvent` emitted
- **Visual:** Red alert banner at top of screen: "Fluid collapse -- all consumers dehydrated"
- **Icon:** Red water droplet with X mark
- **Color:** `#F44336` (red)
- **Animation:** Flashing border at 2Hz
- **Duration:** Persistent while pool state == Collapse
- **Sound:** Alarm tone (distinct from energy collapse alarm)
- **Dismiss:** Auto-clears when `FluidCollapseEndedEvent` emitted

### Warning Priority

When both energy and fluid warnings are active simultaneously:
1. Energy collapse (highest priority, left position)
2. Fluid collapse
3. Energy deficit
4. Fluid deficit (lowest priority, rightmost position)

## Pool Status Display

The fluid status panel (accessible from the infrastructure toolbar) shows:

| Field | Data Source | Format |
|-------|-----------|--------|
| Generation | `pool.total_generated` | "100 fluid/tick" |
| Consumption | `pool.total_consumed` | "80 fluid/tick" |
| Surplus | `pool.surplus` | "+20 fluid/tick" or "-30 fluid/tick" |
| Reservoir buffer | `pool.total_reservoir_stored` / `pool.total_reservoir_capacity` | "500/1000 fluid" |
| Extractors | `pool.extractor_count` | "3 active" |
| Reservoirs | `pool.reservoir_count` | "2 active" |
| Consumers | `pool.consumer_count` | "15 in coverage" |
| Status | `pool.state` | "Healthy" / "Marginal" / "Deficit" / "Collapse" |

### Status Color Coding

| State | Status Text Color | Background |
|-------|------------------|------------|
| Healthy | `#4CAF50` (green) | None |
| Marginal | `#FF9800` (amber) | None |
| Deficit | `#FF5722` (deep orange) | Light orange tint |
| Collapse | `#F44336` (red) | Light red tint, flashing |

## Data Queries from FluidSystem

The overlay renderer queries the following APIs each frame:

### Per-tile coverage

```cpp
// For each visible tile in the viewport:
uint8_t owner = fluid_system.get_coverage_at(x, y);
// owner == 0: uncovered, no overlay
// owner > 0: covered, render coverage zone
```

### Pool status

```cpp
// For status panel and warning banners:
const PerPlayerFluidPool& pool = fluid_system.get_pool(owner);
// Read: total_generated, total_consumed, surplus, state,
//        total_reservoir_stored, total_reservoir_capacity,
//        extractor_count, reservoir_count, consumer_count
```

### Conduit rendering

```cpp
// For each conduit entity:
FluidConduitComponent conduit = registry.get<FluidConduitComponent>(entity);
// conduit.is_active: render flowing pulse
// conduit.is_connected && !conduit.is_active: render dim
// !conduit.is_connected: render gray/disconnected
```

### Reservoir gauge

```cpp
// For each reservoir entity:
FluidReservoirComponent res = registry.get<FluidReservoirComponent>(entity);
// res.current_level: current fill
// res.capacity: max capacity
// Render bar gauge at (current_level / capacity) fill percentage
```

### Structure fluid state

```cpp
// For each building entity with FluidComponent:
FluidComponent fc = registry.get<FluidComponent>(entity);
// fc.has_fluid: render blue glow (true) or gray desaturation (false)
// fc.fluid_received: tooltip data
// fc.fluid_required: tooltip data
```

## Performance Requirements

- **Coverage overlay:** Must not drop framerate below 30fps on a 256x256 map
  - Only render coverage for tiles within the camera viewport (culling)
  - Use a single draw call with instanced rendering for coverage tiles
  - Coverage data is O(1) lookup per tile via `get_coverage_at(x, y)`

- **Conduit animation:** Pulse animation uses a shader uniform (time) rather than per-frame texture swaps
  - Single shader pass for all active conduits
  - GPU-side animation, no CPU per-conduit work

- **Reservoir gauge:** Only render for reservoirs within the viewport
  - Maximum 4 players x ~10 reservoirs = ~40 gauges maximum
  - Trivial rendering cost

- **Warning banners:** Fixed-position UI elements, negligible cost
  - State change events drive show/hide, no per-frame polling

## Interaction with Energy Overlay

When both energy and fluid overlays are active:
- Energy uses warm colors (amber, orange, yellow)
- Fluid uses cool colors (blue, cyan)
- The player can toggle each overlay independently
- When both are active, overlays blend additively (warm + cool = white/neutral in overlap zones)
- Coverage zones from different systems may overlap -- each renders independently
