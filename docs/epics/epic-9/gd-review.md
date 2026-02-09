# Epic 9: City Services - Game Design Review

**Reviewer:** Game Design Review (Automated)
**Date:** 2026-02-08
**Epic:** 9 - City Services
**Files Reviewed:** ServiceTypes.h, ServiceConfigs.h, CoverageCalculation.h/.cpp, GlobalServiceAggregation.h/.cpp, DisorderSuppression.h, LongevityBonus.h, EducationBonus.h, FundingModifier.h, ServiceCoverageOverlay.h

---

## Verdict: APPROVED WITH NOTES

**Confidence:** 7/10

The system is architecturally sound and provides a clean foundation for city services gameplay. The dual-model approach (radius-based vs. global) is a strong design choice. However, there are several balance concerns, a data inconsistency between config sources, and gaps in player-facing feedback that should be addressed before full playtesting.

---

## Strengths

- **Clean dual-model architecture.** Splitting services into radius-based (Enforcer, Hazard) and global/capacity-based (Medical, Education) creates two distinct strategic modes. Radius services demand spatial planning while global services are a population-scaling challenge. This is a proven pattern from classic city builders and works well here.

- **Max-value overlap prevents stacking exploits.** The explicit decision to use `max(existing, new)` rather than additive stacking (E9-022, CoverageCalculation.cpp line 102) eliminates the degenerate "pile buildings on one spot" strategy from day one. This is the correct choice.

- **Linear falloff is readable and intuitive.** The `1.0 - distance/radius` formula produces clean gradients that players can reason about. Manhattan distance over Euclidean is a good performance/readability tradeoff for a tile-based game -- coverage shapes will be diamond-shaped, which is familiar from the genre.

- **Funding curve has diminishing returns.** The 150% funding -> 115% effectiveness cap (FundingModifier.h) means overfunding yields diminishing returns rather than linear scaling. This creates a meaningful budget decision: is that last 15% worth 50% more spending?

- **Dirty flag optimization.** Only recalculating coverage grids when buildings change (E9-011) is the right call for a system that mostly stays static between player actions.

- **Header-only integration modules.** DisorderSuppression.h, LongevityBonus.h, and EducationBonus.h are clean, self-contained formula definitions. Easy to tune, easy to test, easy to understand.

- **Well-defined progression names.** Post -> Station -> Nexus is thematically consistent across all four service types and communicates scale clearly.

---

## Concerns

### HIGH Severity

**H1: Config data inconsistency between ServiceTypes.h and ServiceConfigs.h**

`get_service_config()` in ServiceTypes.h and the `SERVICE_CONFIGS[]` array in ServiceConfigs.h define different values for the same buildings. The coverage calculation code (`CoverageCalculation.cpp` line 55) calls `get_service_config()` from ServiceTypes.h, which means ServiceConfigs.h values are ignored during actual coverage computation.

| Building | ServiceTypes.h (used) | ServiceConfigs.h (declared) | Ticket spec |
|---|---|---|---|
| Hazard Post | radius=8 | radius=10 | radius=10 |
| Hazard Station | radius=12 | radius=15 | radius=15 |
| Hazard Nexus | radius=16 | radius=20 | radius=20 |
| Medical Post | radius=8, cap=100 | radius=0, cap=500 | radius=0, cap=500 |
| Medical Center | radius=12, cap=500 | radius=0, cap=2000 | radius=0, cap=2000 |
| Medical Nexus | radius=16, cap=2000 | radius=0, cap=5000 | radius=0, cap=5000 |
| Education Post | radius=8, cap=200 | radius=0, cap=300 | radius=0, cap=300 |
| Education Archive | radius=12, cap=1000 | radius=0, cap=1200 | radius=0, cap=1200 |
| Education Nexus | radius=16, cap=5000 | radius=0, cap=3000 | radius=0, cap=3000 |

This is not just a data discrepancy -- it means **Medical and Education buildings are being treated as radius-based services in the actual calculation**, contradicting the entire global-service design. The `get_service_config()` function returns non-zero radii for Medical and Education, which means if these configs are ever fed to `calculate_radius_coverage()`, they would generate spatial coverage grids instead of being global. The `ServiceConfigs.h` values and ticket specs are correct; `ServiceTypes.h::get_service_config()` is the one that is wrong.

**H2: Education land value bonus (+10%) is too weak to matter.**

At 100% education coverage, the entire colony gets +10% land value (EducationBonus.h). In classic city builders, education is one of the primary drivers of residential desirability and land value. A 10% colony-wide bonus is unlikely to be perceptible to players or to meaningfully influence placement decisions. Compare to enforcer coverage which provides up to 70% disorder reduction -- education needs a stronger mechanical identity.

**H3: Ownership check is a TODO, not implemented.**

CoverageCalculation.cpp line 65 has a TODO comment: coverage currently treats all tiles as owned by all players. In multiplayer, this means placing a single enforcer post would suppress disorder across all players' territories within range. This must be resolved before multiplayer is viable.

### MODERATE Severity

**M1: Hazard Response has no unique mechanical effect beyond coverage.**

The Hazard system defines `HAZARD_SUPPRESSION_SPEED = 3.0f` in ServiceConfigs.h but there is no code that consumes this constant. Unlike Enforcer (which has DisorderSuppression.h integration) and Medical (LongevityBonus.h), Hazard Response has no integration module. The constant is declared but orphaned. Without a fire/disaster system consuming this coverage, Hazard buildings currently do nothing gameplay-affecting.

**M2: Global services (Medical, Education) have no spatial dimension at all.**

Making Medical and Education purely global means placement location is irrelevant for these buildings -- a Medical Nexus in the corner of the map is as effective as one in the center. This removes spatial strategy for half the service types. Consider adding a secondary radius-based bonus (e.g., "happiness boost near medical buildings") to give placement some meaning even if the primary effect is global.

**M3: Enforcer Nexus coverage area may be too large relative to map size.**

On a 128x128 map, an Enforcer Nexus (radius=16 per ServiceTypes.h, or radius=16 per ServiceConfigs.h) covers a manhattan diamond of approximately 512 tiles. A 128x128 map has 16,384 tiles. Four Nexus buildings could theoretically cover ~2,048 tiles (12.5% of the map). This seems reasonable. However, on a 256x256 map, the same Nexus only covers ~0.8% of tiles, requiring approximately 125 Nexus buildings for full coverage. The radius values may need to scale with map size, or larger maps will feel like an endless grind of service building placement.

**M4: Funding modifier uses uint8_t (max 255) but curve caps at 150%.**

`funding_percent` is `uint8_t` (max 255) but the meaningful range is 0-150 (since everything above 150 maps to 1.15x). The 150% cap is fine for balance, but the type allows values up to 255% funding, which all produce the same 1.15x result. This is not a bug, but a UI/UX concern: ensure the funding slider communicates the cap clearly so players don't waste money overfunding beyond 150%.

**M5: No cost data defined anywhere.**

Service building configs include radius, capacity, footprint, and effectiveness, but no construction cost or maintenance cost. Without cost, there is no economic pressure behind service decisions. This is presumably deferred to the Economy epic (Epic 11), but it means balance analysis is incomplete -- a 3x3 Nexus that costs the same as a 1x1 Post would always be dominant.

### LOW Severity

**L1: Manhattan distance produces diamond-shaped coverage, which may look odd on-screen.**

Manhattan distance creates a rotated-square (diamond) coverage shape rather than a circular one. This is a legitimate design choice for performance, but players may expect circular coverage from genre conventions (SimCity 4, Cities: Skylines). The overlay visualization should make the diamond shape clearly visible so players can plan accordingly.

**L2: Medical capacity values are oddly scaled relative to BEINGS_PER_MEDICAL_UNIT.**

With `BEINGS_PER_MEDICAL_UNIT = 500`, a Medical Post (capacity=500) serves exactly 1 unit of population, a Medical Center (capacity=2000) serves 4 units, and a Medical Nexus (capacity=5000) serves 10 units. But the `calculate_global_service()` function divides total_capacity directly by population (not by beings_per_unit). This means BEINGS_PER_MEDICAL_UNIT is defined but never actually used in the calculation. The constant appears to be vestigial from the ticket spec. Effectiveness is simply `capacity / population`, so a Medical Post with capacity 500 at population 1000 gives 50% effectiveness. This is actually fine mechanically, but the unused constant is confusing.

**L3: No overlay colors defined for Medical and Education.**

ServiceCoverageOverlay.h defines colors for Enforcer (cyan) and Hazard (amber) but not for Medical or Education. Since these are global services without spatial grids, they don't have grid overlays -- but players may still want to visualize "am I covered?" as a global indicator, bar, or percentage display.

---

## Balance Analysis

### Coverage Area by Tier (Manhattan Distance Diamond)

Using ServiceConfigs.h values (the intended values per tickets):

| Service | Tier | Radius | Approx. Tiles Covered | Footprint | Coverage-to-Footprint Ratio |
|---|---|---|---|---|---|
| Enforcer Post | 1 | 8 | ~128 | 1 (1x1) | 128:1 |
| Enforcer Station | 2 | 12 | ~288 | 4 (2x2) | 72:1 |
| Enforcer Nexus | 3 | 16 | ~512 | 9 (3x3) | 57:1 |
| Hazard Post | 1 | 10 | ~200 | 1 (1x1) | 200:1 |
| Hazard Station | 2 | 15 | ~450 | 4 (2x2) | 112:1 |
| Hazard Nexus | 3 | 20 | ~800 | 9 (3x3) | 89:1 |

Manhattan diamond area formula: `2 * r^2` (approximate for integer grid).

**Observation:** Higher tiers have *worse* coverage-to-footprint ratios. A 1x1 Post covers more area per tile of footprint than a 3x3 Nexus. Without cost differentiation, this creates a perverse incentive: spamming Posts is spatially more efficient than building Nexuses. The Nexus must offer something beyond raw coverage to justify its larger footprint -- either significantly lower cost-per-tile-covered, higher peak effectiveness, or secondary bonuses.

**Observation:** Hazard buildings have consistently larger radii than Enforcer buildings (10/15/20 vs 8/12/16). This makes thematic sense -- fires spread and response time matters over distance -- but also means Hazard coverage is strictly easier to achieve than Enforcer coverage. If both services are equally important, the Enforcer player experience will feel more costly/tedious.

### Global Service Capacity Scaling

| Pop. Size | Medical Posts Needed (100%) | Med Centers Needed | Med Nexus Needed |
|---|---|---|---|
| 1,000 | 2 | 1 | 1 |
| 5,000 | 10 | 3 | 1 |
| 10,000 | 20 | 5 | 2 |
| 50,000 | 100 | 25 | 10 |

At city sizes above ~10,000, Medical Posts become impractical (20+ buildings). This naturally pushes players toward higher tiers, which is good progression. The 4x capacity jump from Post to Center and 2.5x from Center to Nexus feel about right.

### Integration Effect Strengths

| Effect | Value at 100% Coverage | Gameplay Impact |
|---|---|---|
| Disorder Suppression | 70% reduction | Very strong -- fundamentally changes disorder dynamics |
| Longevity Bonus | +40 cycles (60->100) | +67% lifespan -- strong and clearly felt |
| Land Value Bonus | +10% | Barely perceptible -- weak |
| Hazard Suppression | 3x speed | Not integrated yet -- no effect |

The disorder and longevity effects are well-calibrated to feel impactful. Education's +10% is the outlier.

### Funding Curve Analysis

| Funding Level | Modifier | Marginal Return per 10% Funding |
|---|---|---|
| 0% | 0.00x | N/A |
| 50% | 0.50x | 0.10x |
| 100% | 1.00x | 0.10x |
| 110% | 1.10x | 0.10x |
| 120% | 1.15x | 0.05x |
| 150% | 1.15x | 0.00x |

The curve is linear up to 115% then flat. The kink at 115% is abrupt. A smoother asymptotic curve (e.g., `1.15 * (1 - e^(-funding/100))`) would feel less "gamey," but the current linear-with-cap approach is simpler and more predictable, which is fine for an initial implementation.

---

## Follow-Up Tickets

### F9-GD-001: Fix ServiceTypes.h config data to match ServiceConfigs.h / ticket specs
**Severity:** HIGH | **Blocks:** Correct gameplay behavior
`get_service_config()` in ServiceTypes.h returns wrong radii and capacities for HazardResponse, Medical, and Education. Medical and Education should have radius=0 (global). Hazard should have radius 10/15/20. Either remove `get_service_config()` entirely and have CoverageCalculation.cpp use `get_service_building_config()` from ServiceConfigs.h, or fix the values in ServiceTypes.h to match.

### F9-GD-002: Implement ownership check in coverage calculation
**Severity:** HIGH | **Blocks:** Multiplayer correctness
Replace the TODO in CoverageCalculation.cpp (line 65) with an actual owner_id check so that radius-based coverage only applies to tiles owned by the building's owner.

### F9-GD-003: Increase education land value bonus or add secondary effect
**Severity:** MODERATE | **Blocks:** Education feeling impactful
Options: (a) Increase the bonus from 10% to 20-25%, (b) Add a per-tile residential desirability bonus near education buildings, (c) Add a secondary effect such as reduced crime or increased population growth rate. Education should feel as impactful as the other three services.

### F9-GD-004: Create HazardResponse integration module
**Severity:** MODERATE | **Blocks:** Hazard buildings having gameplay effect
Create a HazardSuppression.h integration module (analogous to DisorderSuppression.h) that the future DisasterSystem (Epic 13) can consume. At minimum, define the interface contract so the constant `HAZARD_SUPPRESSION_SPEED` is not orphaned.

### F9-GD-005: Add cost and maintenance data to ServiceBuildingConfig
**Severity:** MODERATE | **Blocks:** Economic balance
Add `uint32_t construction_cost` and `uint16_t maintenance_per_tick` fields to ServiceBuildingConfig. Even if the EconomySystem (Epic 11) is not yet implemented, having placeholder cost values enables balance analysis and prevents the current situation where all tiers are economically equivalent.

### F9-GD-006: Ensure Nexus tier has stronger value proposition
**Severity:** MODERATE | **Blocks:** Tier progression feeling meaningful
Since coverage-to-footprint ratio favors Posts, Nexus buildings need a secondary advantage. Options: (a) Higher peak effectiveness (110% base instead of 100%), (b) Bonus aura effect (e.g., Enforcer Nexus reduces disorder in a larger secondary radius at lower strength), (c) Significantly lower cost-per-coverage-tile.

### F9-GD-007: Remove or integrate BEINGS_PER_MEDICAL_UNIT / BEINGS_PER_EDUCATION_UNIT
**Severity:** LOW | **Blocks:** Code clarity
These constants in ServiceConfigs.h are never used in GlobalServiceAggregation.cpp. Either integrate them into the formula (dividing both capacity and population by the constant, which is mathematically equivalent) or remove them to avoid confusion.

### F9-GD-008: Consider map-size-aware radius scaling
**Severity:** LOW | **Blocks:** Large map balance
On 512x512 maps, current radii (8-20 tiles) cover a tiny fraction of the map. Consider a scaling factor or larger Nexus-tier radii for bigger maps. Alternatively, document that large maps are intended to require many more service buildings.

---

## Summary

The Epic 9 implementation establishes a solid and well-organized services framework with good architectural separation between radius-based and global service models. The core algorithms (coverage calculation, falloff, max-overlap, funding modifier) are correct and performant. The three integration effects (disorder, longevity, land value) provide clear mechanical hooks into other game systems.

The critical issue is the config data mismatch (F9-GD-001), which means the system is currently using incorrect values for 9 out of 12 building configurations. Once that is resolved, the remaining concerns are balance-oriented and can be iterated on during playtesting. The education bonus weakness (F9-GD-003) and missing Hazard integration (F9-GD-004) are the most impactful design gaps.

Approved with the expectation that F9-GD-001 and F9-GD-002 are addressed before the next milestone gate.
