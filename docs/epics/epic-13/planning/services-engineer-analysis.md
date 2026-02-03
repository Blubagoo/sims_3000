# Services Engineer Analysis: Epic 13 - Disasters

**Author:** Services Engineer Agent
**Date:** 2026-02-01
**Canon Version:** 0.13.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 13 introduces disasters that challenge the player's colony infrastructure. From a services perspective, this epic extends the role of service buildings from passive coverage providers (Epic 9) to active emergency responders. The key integration points are:

1. **Hazard response posts** respond to fires (conflagrations)
2. **Enforcer posts** respond to civil unrest (riots)
3. **Medical nexuses** may handle disaster injuries (optional)

The primary technical challenge is implementing the IEmergencyResponder interface from canon, which requires:
- Dispatch mechanics (who responds, how fast)
- Response effectiveness calculation
- Capacity management (multiple simultaneous disasters)
- Fallback behavior when no responders exist

This analysis focuses on the **service system integration** with DisasterSystem, not the disaster mechanics themselves.

---

## Table of Contents

1. [Emergency Response Mechanics](#1-emergency-response-mechanics)
2. [Service Integration Points](#2-service-integration-points)
3. [Response Capacity and Prioritization](#3-response-capacity-and-prioritization)
4. [Medical Integration](#4-medical-integration)
5. [Fallback Behavior](#5-fallback-behavior)
6. [Technical Work Items](#6-technical-work-items)
7. [Questions for Other Agents](#7-questions-for-other-agents)
8. [Testing Strategy](#8-testing-strategy)

---

## 1. Emergency Response Mechanics

### 1.1 How Hazard Response Posts Respond to Fires

Per canon, hazard response posts provide fire protection within their coverage radius (Epic 9). For disasters, they must also **actively respond** to fire events.

**Proposed Dispatch Model:**

```
Fire Event at position (X, Y)
    |
    v
Query hazard_response coverage at (X, Y)
    |
    +-- Coverage >= THRESHOLD (e.g., 50%) --> Automatic suppression bonus
    |
    +-- Coverage < THRESHOLD --> Check for dispatchable units
            |
            v
        Find nearest active hazard_response_post
            |
            +-- None found --> Fire spreads at natural rate
            |
            +-- Found --> Dispatch response
                    |
                    v
                Calculate response time (distance-based)
                    |
                    v
                Apply suppression after response time elapses
```

**Response Mechanics:**

| Factor | Effect |
|--------|--------|
| Coverage at fire location | Immediate suppression bonus (if covered) |
| Distance to nearest post | Affects response time |
| Post funding level | Affects suppression effectiveness |
| Post activity state | Unpowered posts cannot respond |

**Suppression Calculation:**

```cpp
// Fire in coverage zone - passive protection
if (hazard_coverage >= COVERAGE_THRESHOLD) {
    fire_spread_rate *= (1.0f - hazard_coverage * 0.7f);  // Up to 70% slower spread
    fire_damage_rate *= (1.0f - hazard_coverage * 0.5f);  // Up to 50% less damage
}

// Active dispatch - direct suppression
if (responder_dispatched) {
    // After response_time elapses
    fire_intensity -= suppression_power * response_effectiveness;
}
```

### 1.2 How Enforcer Posts Respond to Civil Unrest

Enforcer posts normally suppress disorder passively (Epic 9). During civil unrest disasters, they actively respond to contain riots.

**Proposed Dispatch Model:**

```
Civil Unrest Event at position (X, Y)
    |
    v
Query enforcer coverage at (X, Y)
    |
    +-- Coverage >= THRESHOLD --> Automatic containment bonus
    |
    +-- Coverage < THRESHOLD --> Check for dispatchable units
            |
            v
        Find nearest active enforcer_post
            |
            +-- None found --> Riot spreads, damage increases
            |
            +-- Found --> Dispatch response
                    |
                    v
                Calculate response time (distance-based)
                    |
                    v
                Apply containment after response time elapses
```

**Containment Mechanics:**

| Factor | Effect |
|--------|--------|
| Coverage at riot location | Immediate containment bonus |
| Distance to nearest post | Affects response time |
| Post funding level | Affects containment effectiveness |
| Riot intensity | Higher intensity = harder to contain |

### 1.3 Dispatch Mechanics

**Coverage Radius vs. Response Radius**

The existing coverage radius (Epic 9) represents the area where a service building provides **passive protection**. For disaster response, we need a separate **response radius** that may be larger.

| Service Type | Coverage Radius | Response Radius | Notes |
|--------------|-----------------|-----------------|-------|
| Hazard Post | 10 tiles | 15 tiles | Can respond beyond coverage |
| Hazard Station | 15 tiles | 25 tiles | |
| Hazard Nexus | 20 tiles | 35 tiles | |
| Enforcer Post | 8 tiles | 12 tiles | |
| Enforcer Station | 12 tiles | 20 tiles | |
| Enforcer Nexus | 16 tiles | 28 tiles | |

**Response Time Calculation:**

```cpp
float calculate_response_time(GridPosition from, GridPosition to, ServiceTier tier) {
    float distance = manhattan_distance(from, to);

    // Base response speeds (tiles per tick)
    constexpr float RESPONSE_SPEEDS[] = {
        1.0f,  // Post tier
        1.5f,  // Station tier
        2.0f   // Nexus tier
    };

    float speed = RESPONSE_SPEEDS[static_cast<int>(tier)];
    return distance / speed;  // Ticks until arrival
}
```

**Travel Time Modifiers:**

| Factor | Modifier |
|--------|----------|
| Congestion on path | +50% response time at high congestion |
| Funding level < 50% | +25% response time |
| Multiple simultaneous events | +10% per active dispatch |

### 1.4 Effectiveness Calculation

Response effectiveness determines how quickly responders suppress fires or contain riots.

```cpp
float calculate_response_effectiveness(
    EntityID responder,
    DisasterType disaster_type,
    float distance
) {
    auto& service = registry_.get<ServiceProviderComponent>(responder);

    // Base effectiveness from funding
    float base = service.funding_effectiveness;  // 0.0-1.0 from Epic 9

    // Distance penalty (farther = less effective)
    float distance_factor = 1.0f - (distance / MAX_RESPONSE_RADIUS) * 0.3f;

    // Tier bonus
    float tier_bonus = 1.0f + (service.tier - 1) * 0.15f;  // 1x, 1.15x, 1.3x

    return base * distance_factor * tier_bonus;
}
```

---

## 2. Service Integration Points

### 2.1 How DisasterSystem Queries ServicesSystem

The DisasterSystem needs to query service buildings for emergency response. This requires extending the IServiceQueryable interface or adding a new IEmergencyResponder interface.

**Option A: Extend IServiceQueryable (RECOMMENDED)**

Add methods to the existing interface:

```cpp
class IServiceQueryable {
public:
    // Existing methods from Epic 9
    virtual float get_coverage(ServiceType type, PlayerID owner) const = 0;
    virtual float get_coverage_at(ServiceType type, GridPosition pos) const = 0;
    virtual float get_effectiveness(ServiceType type, PlayerID owner) const = 0;

    // NEW: Emergency response methods for Epic 13
    virtual bool can_respond_to(DisasterType type, GridPosition pos, PlayerID owner) const = 0;
    virtual std::optional<EntityID> find_nearest_responder(
        DisasterType type, GridPosition pos, PlayerID owner) const = 0;
    virtual bool dispatch_to(EntityID responder, GridPosition target) = 0;
    virtual float get_response_effectiveness(EntityID responder) const = 0;
    virtual uint32_t get_response_time_ticks(EntityID responder, GridPosition target) const = 0;
};
```

**Option B: Separate IEmergencyResponder Interface**

Per canon interfaces.yaml, IEmergencyResponder is defined as a building-level interface. ServicesSystem could aggregate these:

```cpp
// Per-building interface (from canon)
class IEmergencyResponder {
public:
    virtual bool can_respond_to(DisasterType type) const = 0;
    virtual bool dispatch_to(GridPosition pos) = 0;
    virtual float get_response_effectiveness() const = 0;
};

// ServicesSystem aggregates responders
class IEmergencyResponseProvider {
public:
    virtual std::vector<EntityID> get_available_responders(
        DisasterType type, PlayerID owner) const = 0;
    virtual bool request_dispatch(EntityID responder, GridPosition target) = 0;
    virtual void release_responder(EntityID responder) = 0;
};
```

**Recommendation:** Implement IEmergencyResponder at the building level (per canon), and add IEmergencyResponseProvider to ServicesSystem for aggregation. This maintains the clean separation from Epic 9.

### 2.2 Data Flow: ServicesSystem to DisasterSystem

```
DisasterSystem                         ServicesSystem
     |                                      |
     |-- Query: can_respond_to(fire, pos) ->|
     |<-- Response: true/false -------------|
     |                                      |
     |-- Query: find_nearest_responder() -->|
     |<-- Response: EntityID or nullopt ----|
     |                                      |
     |-- Command: dispatch_to(id, target) ->|
     |<-- Response: success/failure --------|
     |                                      |
     |-- Query: get_response_time_ticks() ->|
     |<-- Response: ticks until arrival ----|
     |                                      |
     |     (after response_time ticks)      |
     |                                      |
     |-- Query: get_response_effectiveness->|
     |<-- Response: 0.0-1.0 ----------------|
     |                                      |
     |-- Apply suppression to disaster      |
     |                                      |
     |-- Command: release_responder(id) --->|
```

### 2.3 Real-Time vs. Batched Response Dispatch

**Real-Time Dispatch (RECOMMENDED):**

Dispatch decisions are made immediately when disasters occur or spread:

```cpp
void DisasterSystem::on_fire_spread(GridPosition new_fire_pos, PlayerID owner) {
    // Immediately check for and dispatch responders
    if (auto responder = services_.find_nearest_responder(
            DisasterType::Fire, new_fire_pos, owner)) {
        services_.dispatch_to(*responder, new_fire_pos);
        track_active_response(*responder, new_fire_pos);
    }
}
```

**Pros:**
- Immediate response feels realistic
- Player sees direct cause-and-effect
- Simpler state management

**Cons:**
- More queries per tick during active disasters
- Potential for dispatch thrashing (reassigning responders)

**Batched Dispatch (Alternative):**

Collect all dispatch requests and process once per tick:

```cpp
void DisasterSystem::tick(float delta_time) {
    // Phase 1: Collect all fire locations needing response
    std::vector<DispatchRequest> requests;
    for (auto& fire : active_fires_) {
        if (!has_active_response(fire.position)) {
            requests.push_back({DisasterType::Fire, fire.position, fire.owner});
        }
    }

    // Phase 2: Batch dispatch
    services_.process_dispatch_requests(requests);
}
```

**Recommendation:** Real-time dispatch for responsiveness. Add rate-limiting to prevent excessive dispatch changes.

---

## 3. Response Capacity and Prioritization

### 3.1 Can Service Buildings Be Overwhelmed?

Yes. A single hazard response post should not be able to respond to unlimited fires simultaneously.

**Proposed Capacity Model:**

| Building Tier | Simultaneous Responses |
|---------------|------------------------|
| Post | 1 |
| Station | 3 |
| Nexus | 5 |

```cpp
struct EmergencyResponseState {
    uint8_t max_concurrent_responses;  // From tier
    std::vector<GridPosition> active_responses;  // Current dispatch targets

    bool can_accept_dispatch() const {
        return active_responses.size() < max_concurrent_responses;
    }

    void on_response_complete(GridPosition target) {
        active_responses.erase(
            std::remove(active_responses.begin(), active_responses.end(), target),
            active_responses.end());
    }
};
```

### 3.2 Multiple Simultaneous Disasters

When multiple disasters occur simultaneously:

1. **Same type (multiple fires):** Responders distributed across fire locations
2. **Different types (fire + riot):** Each responder type handles its specialty
3. **Overwhelming events:** Some disasters go unresponded, spread faster

**Prioritization Algorithm:**

```cpp
void ServicesSystem::prioritize_dispatch_requests(
    std::vector<DispatchRequest>& requests,
    PlayerID owner
) {
    // Priority factors:
    // 1. Severity (higher damage potential = higher priority)
    // 2. Proximity to high-value buildings
    // 3. Spread potential
    // 4. Time since disaster started (older = higher priority)

    std::sort(requests.begin(), requests.end(),
        [this, owner](const auto& a, const auto& b) {
            float priority_a = calculate_priority(a, owner);
            float priority_b = calculate_priority(b, owner);
            return priority_a > priority_b;
        });
}

float ServicesSystem::calculate_priority(
    const DispatchRequest& request,
    PlayerID owner
) {
    float priority = 0.0f;

    // Severity weight
    priority += request.severity * 0.4f;

    // Proximity to important buildings (command nexus, nexuses)
    priority += calculate_infrastructure_threat(request.position, owner) * 0.3f;

    // Spread potential (adjacent flammable buildings)
    priority += calculate_spread_threat(request) * 0.2f;

    // Age factor (avoid letting disasters fester)
    priority += std::min(1.0f, request.age_ticks / 100.0f) * 0.1f;

    return priority;
}
```

### 3.3 Response Cooldown and Fatigue

After a responder completes a dispatch, they should have a brief cooldown before being available again:

```cpp
constexpr uint32_t RESPONSE_COOLDOWN_TICKS = 10;  // ~500ms game time

void ServicesSystem::on_response_complete(EntityID responder, GridPosition target) {
    auto& state = response_states_[responder];
    state.on_response_complete(target);
    state.cooldown_until_tick = current_tick_ + RESPONSE_COOLDOWN_TICKS;
}

bool EmergencyResponseState::is_available() const {
    return can_accept_dispatch() && current_tick_ >= cooldown_until_tick;
}
```

---

## 4. Medical Integration

### 4.1 Should Injuries Be Tracked?

Disasters could cause injuries that affect population health. This integrates medical nexuses into disaster recovery.

**Option A: Population Health Penalty (RECOMMENDED)**

Simple approach: disasters reduce colony health index, which medical coverage counteracts.

```cpp
void DisasterSystem::apply_disaster_health_impact(
    DisasterType type,
    uint32_t affected_population,
    PlayerID owner
) {
    // Disasters cause health index drop
    float health_penalty = affected_population * HEALTH_PENALTY_PER_BEING[type];

    // Medical coverage mitigates
    float medical_coverage = services_.get_coverage(ServiceType::Medical, owner);
    health_penalty *= (1.0f - medical_coverage * 0.5f);  // Up to 50% mitigation

    population_system_.apply_health_penalty(owner, health_penalty);
}
```

**Option B: Individual Injury Tracking (COMPLEX)**

Track injured beings that need medical treatment:

```cpp
struct DisasterInjury {
    uint32_t injured_count;
    DisasterType cause;
    uint32_t tick_occurred;
    float severity;  // Affects recovery time
};

// Medical nexuses "treat" injuries over time
void MedicalSystem::process_injuries(PlayerID owner) {
    auto& injuries = active_injuries_[owner];
    float treatment_rate = get_treatment_capacity(owner);

    for (auto& injury : injuries) {
        injury.injured_count -= treatment_rate * delta_time;
        if (injury.injured_count <= 0) {
            // Injury resolved
        }
    }
}
```

**Recommendation:** Option A (population health penalty) for MVP. Keep it simple and leverage existing Epic 9 medical coverage mechanics.

### 4.2 Medical Nexus Role in Disaster Recovery

Even without explicit injury tracking, medical coverage affects disaster recovery:

1. **During disaster:** Medical coverage reduces death rate from disaster damage
2. **After disaster:** Medical coverage speeds population recovery
3. **Long-term:** Better medical coverage = faster return to normal health index

```cpp
float DisasterSystem::calculate_death_rate_modifier(
    DisasterType type,
    GridPosition pos,
    PlayerID owner
) {
    float base_death_rate = DISASTER_BASE_DEATH_RATES[type];

    // Medical coverage reduces deaths
    float medical_coverage = services_.get_coverage(ServiceType::Medical, owner);
    float medical_modifier = 1.0f - (medical_coverage * 0.4f);  // Up to 40% reduction

    return base_death_rate * medical_modifier;
}
```

---

## 5. Fallback Behavior

### 5.1 What Happens With No Hazard Response Posts?

If a player has no hazard response buildings, fires spread at their natural rate without suppression.

**SimCity 2000 Reference:**
- "Bucket brigade" - nearby beings attempt amateur firefighting (very slow)
- "National Guard" - after severe disasters, external help arrives

**Alien-Themed Fallbacks:**

| Fallback | Trigger | Effect |
|----------|---------|--------|
| Spontaneous Suppression | Fire intensity > 200, no response | 5% chance per tick of spontaneous extinguish (terrain feature) |
| Civilian Response | Fire in habitation zone, coverage = 0 | 10% natural suppression from inhabitants (bucket brigade equivalent) |
| Overmind Intervention | Colony population < 100 AND major disaster | External suppression assist (new player protection) |

**Implementation:**

```cpp
void DisasterSystem::apply_fallback_suppression(
    Fire& fire,
    PlayerID owner
) {
    float hazard_coverage = services_.get_coverage_at(
        ServiceType::HazardResponse, fire.position);

    if (hazard_coverage < 0.01f) {  // Essentially no coverage
        // Fallback 1: Spontaneous suppression (alien terrain feature)
        if (fire.intensity > 200) {
            if (random_.chance(0.05f)) {
                fire.intensity *= 0.9f;  // Terrain absorbs some fire
            }
        }

        // Fallback 2: Civilian response in habitation zones
        if (is_habitation_zone(fire.position)) {
            float civilian_suppression = get_occupancy_at(fire.position) * 0.001f;
            fire.intensity -= civilian_suppression;
        }

        // Fallback 3: New player protection
        auto& pop = population_.get(owner);
        if (pop.total_beings < 100 && fire.intensity > 150) {
            fire.intensity *= 0.95f;  // Overmind assistance
            emit_event(OvermindAssistanceEvent{owner, fire.position});
        }
    }
}
```

### 5.2 What Happens With No Enforcer Posts?

Riots spread faster and cause more damage without enforcer response.

**Alien-Themed Fallbacks:**

| Fallback | Trigger | Effect |
|----------|---------|--------|
| Collective Calm | Harmony > 70, riot < 100 intensity | 10% chance riot dissipates naturally |
| Hive Mind Pacification | Riot in exchange zone | 15% natural containment (beings prioritize commerce) |
| Overmind Intervention | New player protection | External containment assist |

### 5.3 Advisor Warnings

When disasters occur without adequate service coverage, the game should warn players:

```cpp
void DisasterSystem::check_service_warnings(
    DisasterType type,
    GridPosition pos,
    PlayerID owner
) {
    ServiceType required = get_required_service(type);
    float coverage = services_.get_coverage_at(required, pos);

    if (coverage < 0.1f) {
        AdvisorWarning warning;
        warning.type = AdvisorWarningType::NoDisasterCoverage;
        warning.message = format_no_coverage_message(type, required);
        warning.suggested_action = SuggestedAction::BuildServiceBuilding;
        warning.suggested_building = get_service_building_type(required);

        ui_system_.show_advisor_warning(owner, warning);
    }
}
```

---

## 6. Technical Work Items

### Phase 1: IEmergencyResponder Implementation

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-001 | EmergencyResponseState component | Per-building response capacity tracking | S |
| DIS-SVC-002 | IEmergencyResponder interface | Building-level response interface (canon) | M |
| DIS-SVC-003 | Hazard response IEmergencyResponder | Implement for hazard response buildings | M |
| DIS-SVC-004 | Enforcer IEmergencyResponder | Implement for enforcer buildings | M |
| DIS-SVC-005 | Response radius configuration | Separate from coverage radius | S |

### Phase 2: Dispatch System

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-010 | IEmergencyResponseProvider interface | ServicesSystem aggregation interface | M |
| DIS-SVC-011 | find_nearest_responder() implementation | Spatial query for available responders | M |
| DIS-SVC-012 | dispatch_to() implementation | Assign responder to disaster location | M |
| DIS-SVC-013 | Response time calculation | Distance and modifier-based timing | S |
| DIS-SVC-014 | Response effectiveness calculation | Funding, tier, distance factors | S |
| DIS-SVC-015 | release_responder() implementation | Free responder after disaster resolved | S |

### Phase 3: Capacity Management

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-020 | Concurrent response tracking | Track active dispatches per building | M |
| DIS-SVC-021 | Response cooldown system | Cooldown between responses | S |
| DIS-SVC-022 | Dispatch prioritization algorithm | Priority when resources are scarce | L |
| DIS-SVC-023 | Response capacity exhaustion events | Emit when all responders busy | S |

### Phase 4: DisasterSystem Integration

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-030 | Fire suppression from active response | Apply suppression when responder arrives | M |
| DIS-SVC-031 | Riot containment from active response | Apply containment when responder arrives | M |
| DIS-SVC-032 | Passive coverage integration | Coverage-based mitigation (existing) | S |
| DIS-SVC-033 | Medical health impact integration | Medical coverage affects disaster deaths | M |

### Phase 5: Fallback Behavior

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-040 | Fallback suppression system | Spontaneous/civilian suppression | M |
| DIS-SVC-041 | New player protection (Overmind assist) | External help for small colonies | S |
| DIS-SVC-042 | Advisor warning integration | Warn when no coverage for disaster | S |

### Phase 6: Testing

| ID | Item | Description | Size |
|----|------|-------------|------|
| DIS-SVC-050 | Unit tests: dispatch mechanics | Test find/dispatch/release cycle | L |
| DIS-SVC-051 | Unit tests: capacity management | Test concurrent response limits | M |
| DIS-SVC-052 | Integration tests: fire response | End-to-end fire suppression | L |
| DIS-SVC-053 | Integration tests: riot response | End-to-end riot containment | L |

### Work Item Summary

| Size | Count |
|------|-------|
| S | 9 |
| M | 12 |
| L | 4 |
| **Total** | **25** |

---

## 7. Questions for Other Agents

### @systems-architect

1. **Interface ownership:** Should IEmergencyResponseProvider be a new interface owned by ServicesSystem, or should DisasterSystem directly query the ECS registry for IEmergencyResponder components? I recommend ServicesSystem owns the aggregation for encapsulation.

2. **Tick ordering:** DisasterSystem tick_priority is not yet defined in canon. Should it run before or after ServicesSystem (55)? I recommend after (e.g., 90) so coverage data is available when processing disasters.

3. **Response state persistence:** Should EmergencyResponseState be a component or internal ServicesSystem state? If component, it syncs over network automatically; if internal, we need explicit sync.

4. **Active dispatch tracking:** Where should the list of active dispatches live? ServicesSystem (tracks responder assignments) or DisasterSystem (tracks disaster-to-responder mapping)?

### @game-designer

5. **Response radius values:** Should response radius be significantly larger than coverage radius? My proposal is 1.5x coverage radius. Does this feel right for gameplay?

6. **Capacity per tier:** Proposed 1/3/5 concurrent responses for post/station/nexus. Does this provide good differentiation between tiers?

7. **Response time feel:** Is distance-based response time (1-2 tiles per tick) the right feel? Should there be a minimum response time regardless of distance?

8. **Fallback intensity:** How strong should fallback suppression be? Strong enough to eventually extinguish fires, or just slow the spread? I lean toward "eventually extinguish" for sandbox feel.

9. **Medical involvement:** Should medical nexuses provide explicit disaster injury treatment, or is the health index penalty approach sufficient for MVP?

10. **Overmind assistance:** Is new-player protection during disasters appropriate, or does it undermine the challenge? What population threshold (100, 500, 1000)?

### @disaster-engineer (if exists)

11. **Suppression formula:** How should active response suppression interact with existing fire mechanics? Additive, multiplicative, or separate phase?

12. **Dispatch triggers:** Should dispatches happen on disaster start, disaster spread, or both?

13. **Response completion:** When is a response "complete"? When fire at target is extinguished? When intensity drops below threshold?

---

## 8. Testing Strategy

### 8.1 Unit Tests

#### Dispatch Mechanics

| Test | Description | Priority |
|------|-------------|----------|
| test_find_nearest_responder_single | One hazard post, one fire, finds it | P0 |
| test_find_nearest_responder_multiple | Multiple posts, finds closest | P0 |
| test_find_nearest_responder_none | No posts, returns nullopt | P0 |
| test_find_nearest_responder_all_busy | All posts at capacity, returns nullopt | P0 |
| test_dispatch_success | Dispatch to available responder succeeds | P0 |
| test_dispatch_failure_capacity | Dispatch to full responder fails | P0 |
| test_dispatch_failure_unpowered | Dispatch to unpowered building fails | P0 |
| test_release_responder | Release frees capacity | P0 |

#### Response Time

| Test | Description | Priority |
|------|-------------|----------|
| test_response_time_adjacent | Adjacent tile = minimal time | P0 |
| test_response_time_distance | Time scales with distance | P0 |
| test_response_time_tier_bonus | Higher tier = faster response | P0 |
| test_response_time_congestion | Congestion increases time | P1 |

#### Capacity Management

| Test | Description | Priority |
|------|-------------|----------|
| test_concurrent_response_limit_post | Post limited to 1 response | P0 |
| test_concurrent_response_limit_station | Station limited to 3 responses | P0 |
| test_concurrent_response_limit_nexus | Nexus limited to 5 responses | P0 |
| test_cooldown_prevents_immediate_reuse | Cooldown enforced after completion | P0 |

### 8.2 Integration Tests

| Test | Description | Priority |
|------|-------------|----------|
| test_fire_with_hazard_coverage | Fire in coverage zone suppressed faster | P0 |
| test_fire_without_coverage | Fire spreads at natural rate | P0 |
| test_fire_dispatch_and_suppress | Full dispatch cycle with suppression | P0 |
| test_multiple_fires_prioritization | Scarce resources go to highest priority | P1 |
| test_riot_with_enforcer_coverage | Riot contained in coverage zone | P0 |
| test_riot_dispatch_and_contain | Full dispatch cycle with containment | P0 |
| test_disaster_medical_death_reduction | Medical coverage reduces disaster deaths | P1 |
| test_fallback_suppression_no_coverage | Fallback kicks in with zero coverage | P1 |

### 8.3 Performance Tests

| Test | Target | Priority |
|------|--------|----------|
| test_dispatch_query_performance | < 1ms for find_nearest_responder | P1 |
| test_simultaneous_disasters | Handle 10+ active disasters smoothly | P1 |
| test_responder_iteration | Iterate all responders < 0.5ms | P1 |

---

## Appendix A: Disaster Type to Service Type Mapping

| Disaster Type | Responding Service | Fallback Available |
|---------------|-------------------|-------------------|
| Fire (conflagration) | hazard_response | Yes (civilian, terrain) |
| Civil Unrest (riot) | enforcer | Yes (hive mind, commerce) |
| Seismic Event | None (infrastructure damage) | No |
| Inundation (flood) | None (drainage, not service) | No |
| Vortex Storm | None (shelter in place) | No |
| Core Breach | hazard_response (contamination) | No |
| Titan Emergence | enforcer (containment) | No |

---

## Appendix B: IEmergencyResponder Interface (From Canon)

Per interfaces.yaml:

```cpp
class IEmergencyResponder {
public:
    // Whether this responder handles this disaster type
    virtual bool can_respond_to(DisasterType disaster_type) const = 0;

    // Send responders to position. Returns false if unavailable.
    virtual bool dispatch_to(GridPosition position) = 0;

    // How effective this responder is (based on funding)
    virtual float get_response_effectiveness() const = 0;
};

// Implemented by:
// - Hazard response post (fires)
// - Enforcer post (riots/civil unrest)
```

---

## Appendix C: Response State Data Structures

```cpp
// Attached to service buildings that can respond
struct EmergencyResponseStateComponent {
    uint8_t max_concurrent_responses;     // 1/3/5 based on tier
    uint8_t current_response_count;       // Active dispatches
    uint16_t response_radius;             // Tiles (larger than coverage)
    uint32_t cooldown_until_tick;         // When available after completion
    // Active targets stored separately for memory efficiency
};

// Tracking active responses (internal to ServicesSystem)
struct ActiveResponse {
    EntityID responder;
    GridPosition target;
    uint32_t dispatch_tick;
    uint32_t estimated_arrival_tick;
    bool has_arrived;
};
```

---

## Appendix D: Canonical Terminology Reference

| Human Term | Canonical (Alien) Term |
|------------|------------------------|
| Fire department | Hazard response |
| Fire station | Hazard response post |
| Firefighter | Hazard responder |
| Police | Enforcers |
| Police station | Enforcer post |
| Hospital | Medical nexus |
| Fire | Conflagration |
| Riot | Civil unrest |
| Earthquake | Seismic event |
| Flood | Inundation |
| Tornado | Vortex storm |
| Nuclear meltdown | Core breach |
| Monster attack | Titan emergence |

---

**End of Services Engineer Analysis: Epic 13 - Disasters**
