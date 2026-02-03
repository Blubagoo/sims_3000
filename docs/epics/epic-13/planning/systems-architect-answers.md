# Systems Architect Answers: Epic 13 - Disasters

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.13.0
**Status:** ANSWERS PROVIDED

---

## Answers for @game-designer

### Question 1: Catastrophe System Architecture - State Machine

> Should DisasterSystem use a state machine for each active catastrophe? How do we handle multiple simultaneous catastrophes? What's the tick_priority for DisasterSystem relative to BuildingSystem?

**Answer:**

**State Machine: Yes, each active catastrophe should use a state machine.**

Each disaster entity has a `DisasterComponent` with a `DisasterPhase` enum that represents its current state:

```cpp
enum class DisasterPhase : uint8_t {
    Warning = 0,    // Pre-disaster warning period
    Active = 1,     // Disaster in progress, dealing damage
    Recovery = 2,   // Disaster ended, cleanup in progress
    Complete = 3    // Fully resolved, entity can be removed
};
```

State transitions are handled by DisasterSystem during its tick:
- `Warning -> Active`: When warning duration expires
- `Active -> Recovery`: When disaster conditions end (fire extinguished, earthquake stops)
- `Recovery -> Complete`: When debris is cleared and zone is ready for rebuild

**Multiple Simultaneous Catastrophes:**

Multiple disasters are handled naturally through ECS architecture:
1. Each active disaster is a separate entity with a `DisasterComponent`
2. DisasterSystem iterates all disaster entities each tick
3. No global "active disaster" state - the system queries all entities with `DisasterComponent`
4. Fire spread uses a shared `ConflagrationGrid` that handles multiple ignition sources

Interaction rules between simultaneous disasters:
- Fires from different sources merge naturally (grid handles overlapping intensity)
- Different disaster types operate independently (earthquake + fire both deal damage)
- Disaster triggering another disaster (earthquake causing fires) creates new disaster entities

**Tick Priority: 75**

DisasterSystem should run at tick_priority **75**, positioned as follows:

| Priority | System | Rationale |
|----------|--------|-----------|
| 55 | ServicesSystem | Hazard response coverage calculated first |
| 60 | EconomySystem | Maintenance costs available |
| 70 | DisorderSystem | Disorder levels available for civil_unrest triggers |
| **75** | **DisasterSystem** | **Disasters process after disorder, before contamination** |
| 80 | ContaminationSystem | Receives core_breach contamination this tick |
| 85 | LandValueSystem | Disaster effects impact land value immediately |

BuildingSystem runs at tick_priority 40, so DisasterSystem runs significantly later. This is correct because:
1. Buildings must exist and be queryable before damage calculation
2. Building state (active/inactive) affects flammability
3. Damage events are emitted by DisasterSystem and processed by BuildingSystem in the same tick via event queue

---

### Question 2: Cross-System Interactions - Damage and Building State

> How does disaster damage interact with BuildingSystem's state machine (Active -> Damaged -> Derelict -> Debris)? Should there be a DamageableComponent or is damage calculated from existing components? How does conflagration spread interact with ContaminationSystem?

**Answer:**

**Building State Machine Integration:**

Disaster damage integrates with BuildingSystem through health thresholds, not direct state machine manipulation:

```
Building Health Thresholds:
  100-51%: Active (normal operation)
  50-26%:  Damaged (reduced function, visual damage)
  25-1%:   Derelict (non-functional, severe visual damage)
  0%:      Destroyed -> Becomes Debris entity
```

DisasterSystem calculates damage and emits `DamageEvent`. BuildingSystem receives these events and:
1. Reduces health on `DamageableComponent`
2. Updates building state based on health thresholds
3. If health reaches 0: destroys building, creates `DebrisComponent` entity at that position

**DamageableComponent: Yes, create a dedicated component.**

A separate `DamageableComponent` is the correct pattern:

```cpp
struct DamageableComponent {
    uint32_t health = 100;
    uint32_t max_health = 100;
    DamageResistance resistances;  // Per-damage-type resistance (0-100%)
    bool is_destroyed = false;
    bool is_repairable = true;
    uint32_t last_damage_tick = 0;
};
```

This is preferable over calculating from existing components because:
1. Single source of truth for health state
2. Resistance values are entity-specific (service buildings are hardened)
3. Clean separation: BuildingComponent handles building type/tier, DamageableComponent handles damage state
4. Infrastructure (roads, conduits) also needs damage without being "buildings"

**Conflagration and ContaminationSystem:**

Fire spread itself does NOT directly interact with ContaminationSystem. However:

1. **Core Breach** disaster creates both immediate fire AND long-term contamination:
   ```cpp
   void DisasterSystem::apply_core_breach_effects(DisasterComponent& disaster) {
       // Immediate fire at epicenter
       conflagration_grid_.ignite(disaster.epicenter, HIGH_INTENSITY);

       // Long-term contamination via ContaminationSystem
       contamination_system_.add_contamination_source(
           disaster.epicenter,
           disaster.radius * 2,  // Contamination spreads wider than fire
           ContaminationType::Energy  // Radioactive
       );
   }
   ```

2. **Fabrication fires** might reasonably add temporary contamination (industrial pollution released), but this is optional complexity for MVP. Recommend deferring.

3. **Contamination DOES NOT cause fires.** The relationship is one-way: disasters create contamination, not vice versa.

---

### Question 3: Multiplayer Sync

> Catastrophe RNG seeding for deterministic replay across clients? Delta sync for catastrophe progression state? How do we handle catastrophe effects crossing player boundaries?

**Answer:**

**RNG Seeding: Server-authoritative, NOT deterministic client replay.**

Following canon's multiplayer pattern, disaster RNG is **server-only**:

1. Server runs the authoritative simulation including all disaster logic
2. Server's RNG determines when disasters trigger, where they spawn, spread patterns
3. Clients receive disaster state via network sync, do NOT run their own disaster simulation
4. No deterministic replay needed - clients just render what server tells them

```cpp
// Server only:
if (rng_.random_float() < disaster_config.random_probability) {
    GridPosition spawn_pos = find_valid_spawn_location(disaster_type);
    create_disaster(disaster_type, spawn_pos);
    // Sync message automatically sent via SyncSystem
}
```

**Delta Sync for Catastrophe Progression:**

Disaster state uses two sync strategies:

| Data | Sync Type | Frequency | Notes |
|------|-----------|-----------|-------|
| Disaster spawn | Reliable | On event | `DisasterSpawnMessage` with full component |
| Phase changes | Reliable | On event | `DisasterPhaseChangeMessage` |
| Building destruction | Reliable | On event | Part of normal building sync |
| Fire intensity grid | Unreliable | ~4 Hz | Delta-compressed `FireStateUpdateMessage` |

Fire visualization sync uses delta compression:
```cpp
struct FireStateUpdateMessage {
    std::vector<std::pair<GridPosition, uint8_t>> changed_tiles;
    // Only tiles that changed since last sync
};
```

**Cross-Player Boundary Handling:**

Most disasters cross ownership boundaries. The handling:

1. **Detection:** When disaster spreads, check if affected tiles have different owners:
   ```cpp
   for (auto& tile : affected_tiles) {
       PlayerID owner = ownership_system_.get_owner(tile);
       if (!is_player_affected(disaster, owner)) {
           disaster.affected_players[disaster.affected_player_count++] = owner;
           notify_player(owner, DisasterCrossingNotification{...});
       }
   }
   ```

2. **Notification:** All affected players receive disaster notifications even if disaster didn't start in their territory

3. **Response Coordination:** Hazard response from ANY affected player can help suppress:
   ```cpp
   // Each affected player's responders can fight the disaster
   for (PlayerID player : disaster.affected_players) {
       auto responders = services_.get_responders(ServiceType::HazardResponse, player);
       for (auto& responder : responders) {
           if (responder.can_reach(disaster.position)) {
               dispatch_to_disaster(responder, disaster);
           }
       }
   }
   ```

4. **Exceptions:**
   - `civil_unrest` respects pathway networks and typically stays within owner's territory
   - Player-triggered disasters (manual/debug) are constrained to triggering player's territory only

---

### Question 4: Dense Grid for Catastrophe Effects

> Should catastrophe effects (fire spread, damage zones) use a dense grid like other systems? DamageGrid as transient data structure during active catastrophes?

**Answer:**

**ConflagrationGrid: Yes, use a dense grid.**

Fire spread absolutely requires a dense grid, following the `dense_grid_exception` pattern established for TerrainGrid, ServiceCoverageGrid, etc.

**Justification:**
1. Fire can spread to ANY tile (not sparse entities)
2. Spread algorithm requires O(1) neighbor queries for performance
3. Memory efficiency: 3 bytes/tile dense vs 24+ bytes per ECS entity
4. Spatial locality critical for cache-friendly traversal

```cpp
class ConflagrationGrid {
    std::vector<uint8_t> fire_intensity_;   // 0-255 per tile
    std::vector<uint8_t> flammability_;     // 0-255 per tile (from terrain/buildings)
    std::vector<uint8_t> fuel_remaining_;   // 0-255 per tile
    // Total: 3 bytes/tile
};
```

Memory footprint:
- 128x128 map: 48 KB
- 256x256 map: 192 KB
- 512x512 map: 768 KB

**DamageGrid: No, NOT recommended.**

Instant-damage disasters (seismic, storm) should NOT use a dense grid:

1. Damage is instantaneous, not persistent state
2. Sparse iteration (only entities in affected radius) is sufficient
3. `DamageEvent` system handles damage propagation cleanly

Instead, instant damage uses spatial queries:
```cpp
void DisasterSystem::apply_seismic_damage(const DisasterComponent& disaster) {
    auto affected_entities = spatial_index_.query_radius(
        disaster.epicenter, disaster.radius
    );

    for (EntityID entity : affected_entities) {
        float distance = calculate_distance(entity, disaster.epicenter);
        float falloff = 1.0f - (distance / disaster.radius);
        uint32_t damage = disaster.base_damage * disaster.severity * falloff;

        emit(DamageEvent{entity, damage, DamageType::Seismic, disaster.entity_id});
    }
}
```

**Summary:**
- `ConflagrationGrid`: Dense grid, persistent during active fires
- `InundationGrid`: Potentially dense (flood level per tile), if flood mechanics require it
- Damage application: Event-based, NOT grid-based

---

## Answers for @services-engineer

### Question 1: Interface Ownership

> Should IEmergencyResponseProvider be a new interface owned by ServicesSystem, or should DisasterSystem directly query the ECS registry for IEmergencyResponder components?

**Answer:**

**Recommendation: IEmergencyResponseProvider owned by ServicesSystem (Option B from your analysis).**

Rationale:

1. **Encapsulation:** ServicesSystem already owns service building state, coverage calculation, and funding effectiveness. Emergency response is an extension of this responsibility.

2. **Query Efficiency:** ServicesSystem maintains spatial indices for service buildings. Direct ECS registry queries would duplicate this work.

3. **Single Responsibility:** DisasterSystem should focus on disaster logic, not service building discovery.

4. **Consistent Pattern:** Matches how other systems interact:
   - BuildingSystem queries IEnergyProvider (not ECS directly)
   - DisorderSystem queries IServiceQueryable (not ECS directly)

Interface structure:
```cpp
// Per-building interface (from canon)
class IEmergencyResponder {
    virtual bool can_respond_to(DisasterType type) const = 0;
    virtual bool dispatch_to(GridPosition pos) = 0;
    virtual float get_response_effectiveness() const = 0;
};

// Aggregation interface owned by ServicesSystem
class IEmergencyResponseProvider {
    virtual std::vector<EntityID> get_available_responders(
        DisasterType type, PlayerID owner) const = 0;
    virtual bool request_dispatch(EntityID responder, GridPosition target) = 0;
    virtual void release_responder(EntityID responder) = 0;
    virtual float get_response_effectiveness(EntityID responder) const = 0;
    virtual uint32_t get_response_time_ticks(EntityID responder, GridPosition target) const = 0;
};
```

DisasterSystem calls `IEmergencyResponseProvider` methods; it never directly queries the ECS registry for service components.

---

### Question 2: Tick Ordering

> DisasterSystem tick_priority is not yet defined in canon. Should it run before or after ServicesSystem (55)?

**Answer:**

**DisasterSystem tick_priority: 75 (after ServicesSystem at 55)**

Full ordering context:
```
Priority 55: ServicesSystem     - Coverage calculated, responders available
Priority 60: EconomySystem      - Maintenance/funding levels set
Priority 70: DisorderSystem     - Disorder levels available for civil_unrest trigger
Priority 75: DisasterSystem     - Disasters process with all needed data
Priority 80: ContaminationSystem - Receives core_breach contamination
```

**Rationale for running AFTER ServicesSystem:**

1. **Coverage Data Available:** Hazard response coverage must be calculated before DisasterSystem can apply extinguishment effects.

2. **Responder Availability:** ServicesSystem tick determines which responders are active (powered, funded). DisasterSystem needs this information for dispatch.

3. **Single-Tick Response:** Player expects that coverage affects disasters in the same tick. If DisasterSystem ran first, there would be a 1-tick delay.

4. **Event Processing Order:** DamageEvents emitted by DisasterSystem at priority 75 can still be processed by BuildingSystem (40) via the event queue within the same frame.

---

### Question 3: Response State Persistence

> Should EmergencyResponseState be a component or internal ServicesSystem state? If component, it syncs over network automatically; if internal, we need explicit sync.

**Answer:**

**Recommendation: EmergencyResponseState as a Component.**

```cpp
struct EmergencyResponseStateComponent {
    uint8_t max_concurrent_responses;     // From tier (1/3/5)
    uint8_t current_response_count;       // Active dispatches
    uint16_t response_radius;             // Tiles (larger than coverage)
    uint32_t cooldown_until_tick;         // When available after completion
};
```

**Rationale:**

1. **Automatic Network Sync:** Component changes sync via existing SyncSystem infrastructure. No custom network messages needed.

2. **Queryable State:** Other systems (UI showing "responder busy") can query component directly.

3. **Persistence Ready:** When Epic 16 (Persistence) arrives, component automatically gets saved/loaded.

4. **Debugging:** Components are visible in entity inspection tools.

**Implementation Detail:**

The actual list of active dispatch targets should be stored separately in ServicesSystem internal state for memory efficiency:

```cpp
// In ServicesSystem (not synced, derived from events)
std::unordered_map<EntityID, std::vector<GridPosition>> active_responses_;
```

This list is reconstructed on client from dispatch/release events. The component just tracks the count for quick availability checks.

---

### Question 4: Active Dispatch Tracking

> Where should the list of active dispatches live? ServicesSystem (tracks responder assignments) or DisasterSystem (tracks disaster-to-responder mapping)?

**Answer:**

**Recommendation: Both systems maintain their own view, synchronized via events.**

**ServicesSystem tracks: Responder -> Target mapping**
```cpp
// Which positions each responder is currently assigned to
std::unordered_map<EntityID, std::vector<GridPosition>> responder_assignments_;
```

Used for:
- Capacity checks ("can this responder accept another dispatch?")
- Response completion ("responder X finished at position Y")
- UI display ("Hazard Post 1 responding to 2/3 fires")

**DisasterSystem tracks: Disaster/Position -> Responder mapping**
```cpp
// Which responders are assigned to each active disaster element
std::unordered_map<GridPosition, std::vector<ActiveResponse>> position_responses_;

struct ActiveResponse {
    EntityID responder;
    uint32_t dispatch_tick;
    uint32_t estimated_arrival_tick;
    bool has_arrived;
};
```

Used for:
- Suppression calculation ("which responders are fighting this fire?")
- Arrival tracking ("has responder reached the fire yet?")
- Effectiveness aggregation ("total suppression at this tile")

**Synchronization via Events:**

```cpp
// DisasterSystem dispatches
void DisasterSystem::request_response(GridPosition target) {
    if (auto responder = emergency_provider_.find_nearest_responder(target)) {
        emergency_provider_.request_dispatch(*responder, target);
        track_dispatch(*responder, target);  // Local tracking
    }
}

// ServicesSystem notifies completion
void ServicesSystem::on_response_complete(EntityID responder, GridPosition target) {
    // Update internal state
    release_responder_internal(responder, target);

    // Notify DisasterSystem
    emit(ResponseCompletedEvent{responder, target});
}

// DisasterSystem handles completion
void DisasterSystem::on_response_completed(const ResponseCompletedEvent& e) {
    remove_from_tracking(e.responder, e.target);
}
```

This dual-tracking pattern is similar to how BuildingSystem and ZoneSystem both track zone occupancy from different perspectives.

---

## Summary of Key Architectural Decisions

| Question | Decision | Rationale |
|----------|----------|-----------|
| Disaster state machine | Per-entity state machine | Clean ECS pattern, multiple disasters naturally supported |
| Tick priority | 75 | After services (55), before contamination (80) |
| DamageableComponent | Dedicated component | Single source of truth, shared across buildings/infrastructure |
| ConflagrationGrid | Dense grid | Performance-critical spread algorithm |
| DamageGrid | NOT recommended | Instant damage uses event system, not persistent grid |
| IEmergencyResponseProvider | ServicesSystem owns | Encapsulation, query efficiency |
| EmergencyResponseState | Component | Auto-sync, queryable, persistence-ready |
| Dispatch tracking | Both systems | Each maintains own view, synced via events |
| RNG for disasters | Server-only | Client receives state, doesn't simulate |
| Cross-boundary disasters | Allowed with notification | Most disaster types cross boundaries |

---

## Canon Update Recommendations

Based on these answers, I recommend the following canon updates:

### 1. systems.yaml
Add DisasterSystem with tick_priority 75 and component list.

### 2. interfaces.yaml
Add IDisasterProvider and IEmergencyResponseProvider interfaces.

### 3. patterns.yaml
Add ConflagrationGrid to dense_grid_exception list.

### 4. terminology.yaml
Verify all disaster terms are present (most already defined).

---

**End of Systems Architect Answers: Epic 13 - Disasters**
