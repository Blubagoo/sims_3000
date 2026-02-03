# Services Engineer Answers: Epic 13 Planning Questions

**Author:** Services Engineer Agent
**Date:** 2026-02-01
**Status:** ANSWERS PROVIDED

---

## Questions from @systems-architect

### Question 9: Hazard response dispatch algorithm

**Question:** Should hazard responders prioritize:
- (A) Closest fire to responder
- (B) Most intense fire in range
- (C) Fire nearest to highest-value building
- (D) Player-directed (click to dispatch)

**Answer: Hybrid approach - (B) primary with (A) as tiebreaker, optional (D) override**

**Rationale:**

1. **Primary: Most intense fire (B)** - Intense fires spread faster and deal more damage per tick. Attacking the highest-intensity fire first prevents exponential spread. This is the most effective use of response resources from a simulation standpoint.

2. **Tiebreaker: Closest fire (A)** - When multiple fires have similar intensity (within 10% of each other), dispatch to the closest one. This minimizes response time and creates a natural "work outward" pattern that players can understand.

3. **Optional override: Player-directed (D)** - Allow players to manually dispatch a responder to a specific fire location by clicking the hazard response post and then the target fire. This gives agency during tense moments but should NOT be required for effective response.

**Implementation:**

```cpp
GridPosition find_dispatch_target(
    EntityID responder,
    const std::vector<FireInfo>& active_fires
) {
    // Filter fires within response radius
    auto in_range = filter_by_range(active_fires, responder.position, response_radius);

    if (in_range.empty()) return INVALID_POSITION;

    // Sort by intensity (descending), then distance (ascending) as tiebreaker
    std::sort(in_range.begin(), in_range.end(), [&](const auto& a, const auto& b) {
        // If intensities are within 10%, use distance as tiebreaker
        if (std::abs(a.intensity - b.intensity) < a.intensity * 0.1f) {
            return distance(responder.position, a.pos) < distance(responder.position, b.pos);
        }
        return a.intensity > b.intensity;
    });

    return in_range[0].pos;
}
```

**Why NOT purely closest (A):** A small nearby fire might be safely contained while a larger fire farther away spreads out of control. Pure distance-based dispatch can lead to suboptimal outcomes.

**Why NOT purely player-directed (D):** Requiring micromanagement during disasters is anti-fun and doesn't scale. Automatic dispatch should handle 90% of cases well; manual override is for edge cases and player preference.

---

### Question 10: Response capacity limits

**Question:** Can a single hazard response post fight multiple fires simultaneously? Or does it dispatch "units" with limited capacity?

**Answer: Limited concurrent responses per building, NOT dispatched "units"**

**Rationale:**

The capacity model should be **building-based**, not unit-based. This keeps the system simple while still creating meaningful constraints:

| Building Tier | Concurrent Responses | Response Radius |
|---------------|---------------------|-----------------|
| Hazard Post (small) | 1 | 15 tiles |
| Hazard Station (medium) | 3 | 25 tiles |
| Hazard Nexus (large) | 5 | 35 tiles |

**Why building-based, not unit-based:**

1. **Simplicity:** No need to track individual "fire trucks" or "firefighters" as entities. The building itself has capacity.

2. **Visual clarity:** Players can see a hazard post is "busy" (visual indicator) without tracking multiple vehicle positions.

3. **Balance lever:** We can tune capacity per tier without adding entity spawning complexity.

4. **SimCity precedent:** Classic SimCity used radius-based coverage without explicit unit dispatch. This hybrid adds response mechanics without full unit simulation.

**Mechanics:**

- When a hazard post is assigned to a fire location, one of its "response slots" is occupied.
- The response slot remains occupied until: (a) the fire at that location is extinguished, OR (b) the fire spreads beyond the post's effective range.
- If all slots are occupied, the post cannot accept new dispatch requests.
- Multiple posts can respond to the same fire (their effects stack with diminishing returns).

**Implementation:**

```cpp
struct EmergencyResponseCapacity {
    uint8_t max_concurrent;      // 1, 3, or 5 based on tier
    uint8_t current_count;       // Active dispatches
    std::vector<GridPosition> active_targets;

    bool can_accept_dispatch() const {
        return current_count < max_concurrent;
    }

    void on_dispatch(GridPosition target) {
        active_targets.push_back(target);
        current_count++;
    }

    void on_fire_extinguished(GridPosition target) {
        std::erase(active_targets, target);
        current_count--;
    }
};
```

**Alternative considered (unit-based):** We could spawn "hazard response vehicle" entities that travel to fires. This would be more visually engaging but adds significant complexity (pathfinding, entity lifecycle, network sync). Recommend deferring this to a future "visual polish" epic if desired.

---

### Question 11: Enforcer riot suppression

**Question:** How does enforcer coverage suppress riots? Instant suppression in coverage area, or gradual reduction?

**Answer: Gradual reduction with coverage-based rate**

**Rationale:**

Instant suppression would be anti-climactic and unrealistic. Gradual reduction creates:
1. **Tension:** Will the enforcers contain it in time?
2. **Meaningful coverage differentiation:** Higher coverage = faster suppression
3. **Player agency:** Opportunity to adjust funding or dispatch reinforcements

**Mechanics:**

```
Riot Suppression Formula:
    base_suppression_rate = 5 per tick (at 100% coverage, 100% funding)
    actual_rate = base_suppression_rate * enforcer_coverage * funding_effectiveness

    Each tick:
        riot_intensity -= actual_rate
        if riot_intensity <= 0:
            riot_extinguished()
```

**Coverage Effects:**

| Coverage Level | Suppression Rate | Spread Prevention |
|---------------|------------------|-------------------|
| 0% | 0 (riot grows) | None |
| 25% | 1.25/tick | 50% spread chance reduction |
| 50% | 2.5/tick | 75% spread chance reduction |
| 75% | 3.75/tick | 90% spread chance reduction |
| 100% | 5/tick | 99% spread chance reduction |

**Key Interactions:**

1. **Coverage prevents spread:** Even before suppression completes, high enforcer coverage prevents the riot from spreading to adjacent tiles. This is the primary value of coverage.

2. **Funding affects speed:** Lower funding (50%) means 50% suppression rate. This creates budget tension: underfunded enforcers work, just slower.

3. **Overwhelm threshold:** If riot intensity exceeds a threshold (e.g., 200), even full coverage can only slow it, not suppress. This requires multiple enforcer posts or higher-tier buildings.

**Implementation:**

```cpp
void DisasterSystem::process_riot_suppression(RiotComponent& riot, GridPosition pos, PlayerID owner) {
    float enforcer_coverage = services_.get_coverage_at(ServiceType::Enforcer, pos);
    float funding_effectiveness = services_.get_effectiveness(ServiceType::Enforcer, owner);

    float suppression_rate = BASE_SUPPRESSION_RATE * enforcer_coverage * funding_effectiveness;

    // Overwhelm check: very intense riots are harder to suppress
    if (riot.intensity > OVERWHELM_THRESHOLD) {
        suppression_rate *= 0.5f;  // Half effective against intense riots
    }

    riot.intensity = std::max(0.0f, riot.intensity - suppression_rate);

    // Coverage also prevents spread
    float spread_prevention = enforcer_coverage * 0.9f;  // Up to 90% reduction
    riot.spread_chance *= (1.0f - spread_prevention);
}
```

---

### Question 12: Service building disaster vulnerability

**Question:** Should hazard response posts and enforcer posts be protected from disasters (lower damage) or equally vulnerable?

**Answer: Protected, with moderate damage reduction (40% resistance)**

**Rationale:**

Service buildings should be more resilient than typical buildings for several reasons:

1. **Gameplay necessity:** If hazard response posts burn down during a fire, players lose their ability to fight the fire. This creates a frustrating death spiral.

2. **Thematic justification:** Emergency services are built to higher standards and staffed during crises. They're designed to survive what they respond to.

3. **Historical precedent:** Real fire stations have fire-resistant construction. SimCity always treated them as more durable.

4. **Still vulnerable:** They should NOT be immune. A severe disaster can still damage or destroy them, creating meaningful risk. 40% resistance means they still take 60% of normal damage.

**Proposed Resistance Values:**

| Service Building | Fire Resist | Seismic Resist | Storm Resist | Explosion Resist |
|-----------------|-------------|----------------|--------------|------------------|
| Hazard Post | 60% | 50% | 40% | 40% |
| Hazard Station | 70% | 55% | 45% | 50% |
| Hazard Nexus | 80% | 60% | 50% | 60% |
| Enforcer Post | 50% | 50% | 40% | 40% |
| Enforcer Station | 60% | 55% | 45% | 50% |
| Enforcer Nexus | 70% | 60% | 50% | 60% |
| Medical Nexus | 50% | 60% | 50% | 40% |
| Learning Center | 40% | 50% | 40% | 30% |

**Key Design Points:**

1. **Hazard response has highest fire resistance:** They're specifically designed to resist what they fight (80% at nexus tier).

2. **Higher tiers are more resilient:** Investing in better infrastructure pays off in durability.

3. **No building is immune:** Even the Hazard Nexus with 80% fire resistance still takes 20% damage. A sufficiently severe conflagration can destroy it.

4. **Different disaster types have different resistances:** Buildings aren't universally hardened. A medical nexus might survive an earthquake but not a fire as well.

**Implementation:**

```cpp
struct ServiceBuildingResistance {
    // Resistance values by service tier (post=0, station=1, nexus=2)
    static constexpr float HAZARD_FIRE_RESIST[3] = {0.60f, 0.70f, 0.80f};
    static constexpr float HAZARD_SEISMIC_RESIST[3] = {0.50f, 0.55f, 0.60f};
    static constexpr float ENFORCER_FIRE_RESIST[3] = {0.50f, 0.60f, 0.70f};
    // ... etc
};

// Applied when DamageableComponent is created for service buildings
void BuildingSystem::create_service_building(EntityID entity, ServiceType type, uint8_t tier) {
    auto& damageable = registry_.emplace<DamageableComponent>(entity);

    // Higher base health for service buildings
    damageable.max_health = BASE_HEALTH * (1.0f + tier * 0.25f);
    damageable.health = damageable.max_health;

    // Apply service-specific resistances
    damageable.resistances = get_service_resistances(type, tier);
}
```

**Alternative considered (equal vulnerability):** This would be more "realistic" but creates bad gameplay. The first fire could destroy your only fire station, leaving you helpless. This punishes players for disasters they couldn't predict or prevent.

---

## Summary

| Question | Answer | Key Rationale |
|----------|--------|---------------|
| Q9: Dispatch algorithm | Intensity-first, distance tiebreaker, optional manual override | Prevents exponential spread; gives player agency |
| Q10: Capacity limits | Building-based (1/3/5 concurrent), not unit-based | Simple yet meaningful; no entity spawning complexity |
| Q11: Riot suppression | Gradual reduction based on coverage and funding | Creates tension; differentiates coverage levels |
| Q12: Disaster vulnerability | Protected (40% average resistance) | Prevents death spirals; thematically justified |

---

**End of Services Engineer Answers**
