# Epic 9: City Services -- Systems Architect Review

**Reviewer:** Systems Architect (SA)
**Date:** 2026-02-08
**Epic:** 9 -- City Services
**Verdict:** APPROVED WITH FINDINGS
**Confidence:** 8/10

---

## Summary

Epic 9 implements the City Services subsystem: four service types (Enforcer, HazardResponse, Medical, Education), a coverage grid for radius-based services, global aggregation for capacity-based services, a funding modifier, an overlay visualization system, and integration contracts for Epic 10 consumers. The architecture is well-structured, follows established project patterns (dense_grid_exception, ISimulatable, forward dependency interfaces), and provides clean separation between spatial coverage and global capacity models.

The implementation is solid and ready for integration, but contains several data inconsistencies between redundant configuration sources and a handful of design improvements that should be addressed before Epic 10 work begins.

---

## Files Reviewed

### Headers (16 files)
1. `include/sims3000/services/ServiceTypes.h`
2. `include/sims3000/services/ServiceProviderComponent.h`
3. `include/sims3000/services/ServiceSerialization.h`
4. `include/sims3000/services/ServiceConfigs.h`
5. `include/sims3000/services/ServicesSystem.h`
6. `include/sims3000/services/ServiceCoverageGrid.h`
7. `include/sims3000/services/ServiceEvents.h`
8. `include/sims3000/services/CoverageCalculation.h`
9. `include/sims3000/services/GlobalServiceAggregation.h`
10. `include/sims3000/services/FundingModifier.h`
11. `include/sims3000/services/ServiceStatistics.h`
12. `include/sims3000/services/IGridOverlay.h`
13. `include/sims3000/services/ServiceCoverageOverlay.h`
14. `include/sims3000/services/DisorderSuppression.h`
15. `include/sims3000/services/LongevityBonus.h`
16. `include/sims3000/services/EducationBonus.h`

### Sources (6 files)
17. `src/services/ServiceSerialization.cpp`
18. `src/services/ServicesSystem.cpp`
19. `src/services/ServiceCoverageGrid.cpp`
20. `src/services/CoverageCalculation.cpp`
21. `src/services/GlobalServiceAggregation.cpp`
22. `src/services/ServiceCoverageOverlay.cpp`

### Modified integration files (6 files)
23. `include/sims3000/building/ForwardDependencyInterfaces.h`
24. `include/sims3000/building/ForwardDependencyStubs.h`
25. `include/sims3000/building/BuildingSystem.h`
26. `src/building/BuildingSystem.cpp`
27. `CMakeLists.txt`
28. `tests/CMakeLists.txt`

---

## Findings

### HIGH Severity

#### Finding 1: Duplicate and contradictory configuration data between `ServiceTypes.h` and `ServiceConfigs.h`

**Files:**
- `include/sims3000/services/ServiceTypes.h` (lines 108-141, `get_service_config()`)
- `include/sims3000/services/ServiceConfigs.h` (lines 90-226, `SERVICE_CONFIGS[]`)

**Description:** Two independent, authoritative sources of configuration data exist for the same service type+tier combinations, and they contradict each other in several places:

| Service | Tier | `get_service_config()` radius | `SERVICE_CONFIGS[]` radius | `get_service_config()` capacity | `SERVICE_CONFIGS[]` capacity |
|---------|------|------|------|------|------|
| HazardResponse | Post | 8 | 10 | 0 | 0 |
| HazardResponse | Station | 12 | 15 | 0 | 0 |
| HazardResponse | Nexus | 16 | 20 | 0 | 0 |
| Medical | Post | 8 | 0 | 100 | 500 |
| Medical | Station | 12 | 0 | 500 | 2000 |
| Medical | Nexus | 16 | 0 | 2000 | 5000 |
| Education | Post | 8 | 0 | 200 | 300 |
| Education | Station | 12 | 0 | 1000 | 1200 |
| Education | Nexus | 16 | 0 | 5000 | 3000 |

The `ServiceConfigs.h` data is clearly the more considered and authoritative source (Medical/Education have radius=0 for global effect, HazardResponse has differentiated radii, and the data model includes names and power requirements). The `get_service_config()` in `ServiceTypes.h` appears to be an earlier placeholder that was never reconciled.

**Impact:** Any code that calls `get_service_config()` will use wrong radius and capacity values. `CoverageCalculation.cpp` line 55 calls `get_service_config()` to look up the radius for each building. If Medical buildings are passed to the radius calculation function, they would get radius 8/12/16 instead of 0 (global), meaning they would be treated as radius-based rather than global.

**Recommendation:** Remove `get_service_config()` from `ServiceTypes.h` entirely. Replace any callers with lookups into `get_service_building_config()` from `ServiceConfigs.h`. `ServiceTypes.h` should only contain enums, validation, and string conversion. This is the single-source-of-truth principle.

---

#### Finding 2: Duplicate constants across multiple headers (ODR risk)

**Files:**
- `include/sims3000/services/ServiceConfigs.h` (lines 62-65): `MEDICAL_BASE_LONGEVITY = 60`, `MEDICAL_MAX_LONGEVITY_BONUS = 40`
- `include/sims3000/services/LongevityBonus.h` (lines 29-32): `MEDICAL_BASE_LONGEVITY = 60`, `MEDICAL_MAX_LONGEVITY_BONUS = 40`
- `include/sims3000/services/ServiceTypes.h` (line 87): `ENFORCER_SUPPRESSION_MULTIPLIER = 0.7f`
- `include/sims3000/services/DisorderSuppression.h` (line 28): `ENFORCER_SUPPRESSION_FACTOR = 0.7f`
- `include/sims3000/services/ServiceConfigs.h` (line 74): `EDUCATION_KNOWLEDGE_BONUS = 0.1f`
- `include/sims3000/services/EducationBonus.h` (line 29): `EDUCATION_LAND_VALUE_BONUS = 0.10f`

**Description:** The same gameplay constants are defined in multiple headers under slightly different names. While `constexpr` in header files is safe from ODR violations (each TU gets its own copy), having two independently-named constants for the same concept (`ENFORCER_SUPPRESSION_MULTIPLIER` vs `ENFORCER_SUPPRESSION_FACTOR`) is a maintenance hazard. If one is changed but not the other, subtle gameplay bugs will occur.

`MEDICAL_BASE_LONGEVITY` and `MEDICAL_MAX_LONGEVITY_BONUS` are identically named and defined in two separate headers. If both are included in the same TU, this will cause a compilation error (redefinition).

**Recommendation:** Define each constant in exactly one header. `ServiceConfigs.h` should be the single source for all gameplay tuning constants. `LongevityBonus.h`, `DisorderSuppression.h`, and `EducationBonus.h` should `#include <sims3000/services/ServiceConfigs.h>` and reference the constants from there. Remove the duplicates.

---

### MEDIUM Severity

#### Finding 3: `IServiceQueryable::get_coverage_at()` is missing `player_id` parameter

**File:** `include/sims3000/building/ForwardDependencyInterfaces.h` (lines 341-347)

**Description:** The `get_coverage_at()` method signature is:
```cpp
virtual float get_coverage_at(uint8_t service_type, int32_t x, int32_t y) const = 0;
```

Unlike `get_coverage()` and `get_effectiveness()` which both take `player_id`, `get_coverage_at()` does not. However, the coverage grids are per-player (indexed as `m_coverage_grids[SERVICE_TYPE][PLAYER_ID]`). Without a `player_id` parameter, the implementation cannot determine which player's grid to query.

**Impact:** When Epic 10 systems call `get_coverage_at()`, there is no way to specify which player's coverage to check. The implementation will need to either assume player 0 or iterate all players, neither of which is correct for a multiplayer game.

**Recommendation:** Add `uint8_t player_id` parameter to `get_coverage_at()`:
```cpp
virtual float get_coverage_at(uint8_t service_type, int32_t x, int32_t y, uint8_t player_id) const = 0;
```
Update `StubServiceQueryable` accordingly.

---

#### Finding 4: `on_building_constructed()` marks all service types dirty instead of the specific type

**File:** `src/services/ServicesSystem.cpp` (lines 146-158)

**Description:** When a building is constructed, `on_building_constructed()` calls `mark_all_dirty(owner_id)` instead of `mark_dirty(type, owner_id)`. The code comments acknowledge this: "We mark all types because we don't know the service type from entity_id alone."

However, the `ServiceBuildingPlacedEvent` in `ServiceEvents.h` already carries the `service_type` field. The handler method signature should accept `ServiceType` so that only the relevant grid is recalculated.

**Impact:** Every building construction triggers recalculation of all 4 coverage grids for that player, when only 1 needs recalculation. On a 256x256 map with frequent construction, this is 4x wasted work per construction event.

**Recommendation:** Change event handler signatures to accept `ServiceType`:
```cpp
void on_building_constructed(uint32_t entity_id, uint8_t owner_id, ServiceType type);
```
Then call `mark_dirty(type, owner_id)` instead of `mark_all_dirty(owner_id)`. Same for `on_building_deconstructed` and `on_building_power_changed`.

---

#### Finding 5: `ServiceProviderComponent::tier` stores raw `uint8_t` instead of `ServiceTier` enum

**File:** `include/sims3000/services/ServiceProviderComponent.h` (line 37)

**Description:** The `tier` field is declared as `uint8_t` with default value `1`. All other code paths use `ServiceTier` enum, requiring constant casting. The `ServiceBuildingData` struct in `CoverageCalculation.h` also uses `uint8_t tier` (line 41). This forces callers to perform `static_cast<ServiceTier>(building.tier)` at usage sites (e.g., `CoverageCalculation.cpp` line 54).

**Recommendation:** Use `ServiceTier` as the type for `tier` in both `ServiceProviderComponent` and `ServiceBuildingData`. The serialization layer already handles the uint8_t conversion. Since `ServiceTier` is backed by `uint8_t`, `sizeof` and trivial-copy static assertions are unaffected.

---

#### Finding 6: `set_service_provider()` stores the pointer but never propagates to subsystems

**File:** `src/building/BuildingSystem.cpp` (lines 78-80)

**Description:** `BuildingSystem::set_service_provider()` stores the `IServiceQueryable*` in `m_service` but does not propagate it to any subsystem. Looking at the other provider setters (lines 55-92), the same pattern applies to all of them -- the comments in `set_energy_provider()` (lines 57-63) explicitly acknowledge this limitation.

However, for the service provider specifically, no subsystem within `BuildingSystem` currently consumes it. This is acceptable for now but the stored pointer `m_service` is essentially dead code until a subsystem is wired to use it.

**Recommendation:** Document in the header that `m_service` is reserved for future Epic 10 integration. Consider adding a `TODO` comment noting when/how it will be consumed, so it does not appear to be a forgotten wiring step.

---

### LOW Severity

#### Finding 7: Custom `strcmp` implementation in `ServiceTypes.h` instead of using `<cstring>`

**File:** `include/sims3000/services/ServiceTypes.h` (lines 173-188)

**Description:** `service_type_from_string()` and `service_tier_from_string()` both define an inline lambda `str_eq` that reimplements `strcmp`. The comment says "no `<cstring>` dependency in header" but this is an unnecessary restriction -- other headers in the project include standard library headers freely.

**Recommendation:** Include `<cstring>` and use `std::strcmp()` or keep the lambda but define it once as a namespace-scope utility rather than duplicating it in two functions.

---

#### Finding 8: `ServiceCoverageGrid::clear()` uses `std::fill` while `energy::CoverageGrid` uses `clear_all()`

**File:** `src/services/ServiceCoverageGrid.cpp` (line 42-44)

**Description:** The naming convention differs from the established pattern in `energy::CoverageGrid` (method `clear_all()`) and `fluid::FluidCoverageGrid` (method `clear_all()`). The service grid uses `clear()` instead. While functionally equivalent, the inconsistent naming breaks the pattern established by earlier epics.

**Recommendation:** Rename `ServiceCoverageGrid::clear()` to `clear_all()` for consistency with the `energy::CoverageGrid` and `fluid::FluidCoverageGrid` patterns.

---

#### Finding 9: `ServiceCoverageOverlay` stores a raw `const char*` name that must outlive the object

**File:** `include/sims3000/services/ServiceCoverageOverlay.h` (line 85)

**Description:** The constructor accepts `const char* name` and stores it as a raw pointer. The documentation says "must outlive this object" which is a valid contract, but fragile. If an overlay is constructed with a dynamically-allocated string, the caller must ensure lifetime management. In practice, all current uses pass string literals, so this is safe today.

**Recommendation:** Acceptable as-is given current usage with literals. If dynamic names are ever needed, switch to `std::string`.

---

#### Finding 10: Event structs have non-trivial constructors, breaking potential aggregate initialization

**File:** `include/sims3000/services/ServiceEvents.h` (all three event structs)

**Description:** Each event struct defines both a default constructor and a parameterized constructor. This prevents C++17 aggregate initialization. While this is not a bug, it means callers must use the parameterized constructor rather than the more concise brace initialization:
```cpp
// Cannot do this because of user-declared constructors:
ServiceBuildingPlacedEvent evt{entity, owner, type, tier, x, y};
// Must do this instead:
ServiceBuildingPlacedEvent evt(entity, owner, type, tier, x, y);
```

**Recommendation:** Consider removing the user-declared constructors and relying on aggregate initialization, consistent with how `ServiceBuildingData` is used. If default construction is needed, use `= default` or in-class member initializers.

---

#### Finding 11: `ServiceStatisticsManager` duplicates `SERVICE_TYPE_COUNT` and `MAX_PLAYERS` constants

**File:** `include/sims3000/services/ServiceStatistics.h` (lines 53-56)

**Description:** `ServiceStatisticsManager` defines its own `SERVICE_TYPE_COUNT = 4` and `MAX_PLAYERS = 4` rather than referencing the canonical constants from `ServiceTypes.h` and `ServicesSystem.h`. If either canonical value changes, these copies will silently diverge.

**Recommendation:** Include `ServiceTypes.h` and reference `services::SERVICE_TYPE_COUNT` directly. For `MAX_PLAYERS`, reference the canonical constant from `ServicesSystem.h` or define it once in a shared location.

---

#### Finding 12: `CoverageCalculation.cpp` calls `get_service_config()` from `ServiceTypes.h` instead of `ServiceConfigs.h`

**File:** `src/services/CoverageCalculation.cpp` (line 55)

**Description:** The coverage calculation function uses `get_service_config()` from `ServiceTypes.h` (which has the wrong radius values per Finding 1) rather than `get_service_building_config()` from `ServiceConfigs.h`. This means the calculation uses incorrect configuration data for HazardResponse, Medical, and Education services.

**Recommendation:** Switch to using `get_service_building_config()` from `ServiceConfigs.h`. This will resolve the radius discrepancy and ensure the coverage calculation uses the authoritative config data.

---

## Architecture Assessment

### Strengths
1. **Pattern compliance:** `ServiceCoverageGrid` correctly follows the `dense_grid_exception` pattern established by `energy::CoverageGrid` and `fluid::FluidCoverageGrid` -- row-major layout, bounds checking, 1 byte per cell, identical memory budget documentation.
2. **Clean ISimulatable implementation:** `ServicesSystem` properly implements `ISimulatable` at priority 55, with correct lifecycle management (init/cleanup/tick).
3. **Lazy allocation:** Coverage grids are only allocated on first dirty recalculation (E9-011), which is memory-efficient for unused service types.
4. **Dirty flag optimization:** The per-type-per-player dirty flag system avoids unnecessary recalculation, a good O(1) check before O(n) grid work.
5. **Forward dependency integration:** `IServiceQueryable` follows the exact same pattern as all other interfaces in `ForwardDependencyInterfaces.h`. The stub implementation is correctly conservative (returns 0.0f).
6. **Header-only effect calculators:** `DisorderSuppression.h`, `LongevityBonus.h`, and `EducationBonus.h` are clean, self-contained, testable units with well-defined contracts.
7. **Comprehensive test coverage:** 17 test targets cover types, components, serialization, systems, grids, calculations, overlays, events, and integration contracts.
8. **Serialization with versioning:** Field-by-field serialization with a version byte for forward compatibility.

### Performance
- **Coverage calculation:** O(B * R^2) where B = number of buildings and R = max radius (16). For a worst case of 100 buildings with radius 16, this is ~100 * 33^2 = ~109,000 iterations, well within budget.
- **Grid memory:** 4 types * 4 players * 256KB (512x512) = 4MB maximum if all grids are allocated. With lazy allocation, typical usage will be much lower.
- **No O(n^2) algorithms detected.** The `on_building_deconstructed` entity removal uses swap-and-pop (O(n) scan, O(1) removal), which is appropriate for the expected entity count.

### Thread Safety
- No thread safety issues detected. `ServicesSystem` is designed to run on the simulation thread only, consistent with the tick-based architecture. The coverage grids are not accessed concurrently -- they are written during `recalculate_if_dirty()` and read by overlay/query systems after the tick completes.

### Error Handling
- All grid operations have bounds checking with safe defaults (return 0/0.0f for out-of-bounds).
- Serialization validates version and buffer size before reading.
- Dirty flag operations validate player_id and type indices.
- `ServiceStatisticsManager` validates all input indices.

---

## Verdict Rationale

**APPROVED WITH FINDINGS** because:
- The architecture is sound and follows all established project patterns.
- The HIGH findings (1, 2) are data consistency issues that will cause incorrect behavior if not fixed before Epic 10 integration, but they do not represent structural problems.
- The MEDIUM findings (3-6) are design improvements that will smooth Epic 10 integration.
- All LOW findings are minor naming/style issues.
- The system is well-tested with 17 test targets.
- No blocking structural issues exist.

**Action required before Epic 10 begins:**
1. Resolve Finding 1 (remove `get_service_config()`, consolidate to `ServiceConfigs.h`)
2. Resolve Finding 2 (deduplicate constants)
3. Resolve Finding 3 (add `player_id` to `get_coverage_at()`)
