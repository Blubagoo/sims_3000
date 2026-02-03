# Decision: Snapshot Generation Strategy

**Date:** 2026-01-27
**Status:** accepted
**Epic:** 1
**Affects Tickets:** 1-014

## Context

Epic 1 requires generating full state snapshots for reconnecting players. These snapshots can be 1-5MB for mature worlds. Generating them synchronously would block the simulation tick and cause stuttering for all connected players.

## Options Considered

### 1. Async with Copy-on-Write (COW)
- When snapshot requested, mark current state as "snapshot base"
- Background thread serializes the snapshot
- Components modified during serialization are copied first (copy-on-write)
- Non-blocking, simulation continues normally

### 2. Blocking Snapshot
- Pause simulation, generate snapshot, resume
- Simple and guarantees consistency
- Causes gameplay stutters for all players

### 3. Incremental Snapshots
- Spread snapshot work over multiple ticks
- Build up state progressively
- Complex tracking, longer reconnection time

## Decision

**Async with Copy-on-Write** - Generate snapshots asynchronously using a copy-on-write pattern.

## Rationale

1. **Non-blocking:** Simulation continues at full tick rate during snapshot generation
2. **Consistency:** COW ensures snapshot represents a consistent point-in-time state
3. **Standard approach:** This is how most game servers handle large state snapshots
4. **Scalability:** Works regardless of world size

## Implementation Details

```cpp
// When snapshot requested:
// 1. Acquire snapshot lock (brief)
// 2. Mark all components as "snapshot generation in progress"
// 3. Release lock, start background serialization

// During snapshot generation:
// - Simulation continues normally
// - Any component modification triggers copy-on-write:
//   - Copy old value to snapshot buffer
//   - Then apply modification to live state
// - Background thread reads from snapshot buffer where COW occurred,
//   live state where it hasn't

// When snapshot complete:
// - Clear snapshot-in-progress flags
// - Send snapshot to reconnecting client
```

## Consequences

- Add COW flag to component modification paths
- Background thread for snapshot serialization
- Small memory overhead during snapshot generation (COW copies)
- Ticket 1-014 acceptance criteria updated to reflect async approach

## References

- Standard game server pattern
- Similar to database MVCC (multi-version concurrency control)
