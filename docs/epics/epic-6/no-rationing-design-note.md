# Design Note: No Priority-Based Rationing for Fluid (Ticket 6-020)

## What

FluidSystem has **no priority-based rationing**. All consumers are treated equally.
When the fluid pool enters deficit (surplus < 0 after reservoir drain), **all**
consumers lose fluid simultaneously. When surplus >= 0, **all** consumers in
coverage receive their full fluid requirement.

## Why

This was resolved in CCR-002 (Canon Change Request). The game designer's rationale:

> "Fluid affects habitability uniformly. You can't say these beings get water
> and these don't."

Unlike energy, where critical infrastructure (hospitals, emergency services) can
be prioritized over luxury buildings during brownouts, fluid is a fundamental
habitability requirement that applies equally to all structures.

## How

`FluidSystem::distribute_fluid()` implements all-or-nothing distribution:

- **Surplus >= 0**: Every consumer in coverage gets `fluid_received = fluid_required`,
  `has_fluid = true`.
- **Surplus < 0**: Every consumer in coverage gets `fluid_received = 0`,
  `has_fluid = false`.
- **Out of coverage**: Always `fluid_received = 0`, `has_fluid = false`,
  regardless of pool state.

`FluidComponent` has no `priority` field (unlike `EnergyComponent` which has a
`uint8_t priority` field for 4-tier rationing).

## Contrast with Energy

EnergySystem uses 4-tier priority-based rationing during deficit:

| Priority | Tier       | Example Buildings           |
|----------|------------|-----------------------------|
| 1        | Critical   | Command centers, life support |
| 2        | Important  | Extractors, factories        |
| 3        | Normal     | Residential, commercial      |
| 4        | Low        | Decorative, luxury           |

During energy deficit, `EnergySystem::apply_rationing()` sorts consumers by
priority (ascending), then allocates available energy to higher-priority
consumers first. Lower-priority consumers are shed when supply is insufficient.

FluidSystem has **none of this**. There is no sorting, no priority field, no
partial allocation.

## Future

If rationing is needed in a future version, the energy system's pattern in
`EnergySystem::apply_rationing()` (file: `src/energy/EnergySystem.cpp`) can be
adapted:

1. Add a `uint8_t priority` field to `FluidComponent`.
2. Create `FluidSystem::apply_rationing(uint8_t owner)` that sorts consumers
   by priority and allocates fluid to higher-priority consumers first.
3. Call `apply_rationing()` from `distribute_fluid()` when `pool.surplus < 0`.
4. Update `FluidComponent` size assertions (currently 12 bytes) to account
   for the new field.
