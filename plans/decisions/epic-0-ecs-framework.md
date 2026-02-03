# Decision: ECS Framework Selection

**Date:** 2026-01-25
**Status:** accepted
**Epic:** 0
**Affects Tickets:** 0-004, 0-005, 0-023, 0-024, 0-027, 0-028, 0-029

## Context

Epic 0 requires an ECS framework for entity management, component storage, and system execution. This is a foundational decision that affects all subsequent epics.

## Options Considered

### 1. EnTT
- Header-only C++17 library
- Mature, widely used in game development
- Fast sparse-set based storage
- Excellent documentation
- Active maintenance

### 2. flecs
- C-based with C++ wrapper
- Built-in query system
- Good for games
- Larger API surface

### 3. Custom Implementation
- Full control over design
- Learning opportunity
- Significant development effort
- Risk of bugs and performance issues

## Decision

**EnTT** - Use EnTT as the ECS framework.

## Rationale

1. **Maturity:** Battle-tested in production games
2. **Integration:** Header-only, easy to add to build
3. **Performance:** Sparse-set storage is cache-friendly
4. **Compatibility:** C++17 aligns with our stack
5. **Documentation:** Well-documented with examples
6. **Risk reduction:** Proven solution vs custom implementation

## Consequences

- Add EnTT to vcpkg.json dependencies
- All components must be trivially copyable (already a canon requirement)
- Systems will use EnTT's view/group query patterns
- Entity IDs will use EnTT's entity type (compatible with our uint32_t EntityID)

## References

- EnTT GitHub: https://github.com/skypjack/entt
- EnTT Wiki: https://github.com/skypjack/entt/wiki
