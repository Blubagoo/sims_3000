# Systems Architect Answers: Epic 14 Questions

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**In Response To:** Game Designer Analysis Section 7
**Status:** Complete

---

## Overview

This document provides technical answers to the Game Designer's questions from the Epic 14 analysis. Each answer includes the technical approach, implementation details, and relevant performance or synchronization considerations.

---

## Question 1: Milestone Notification Delivery

> How are milestone notifications sent to all connected clients? Is there a MilestoneEvent that triggers UI and audio on all clients? What's the latency tolerance for synchronized celebrations?

### Technical Approach

Milestone notifications use a **server-authoritative broadcast pattern**. When ProgressionSystem detects a milestone crossing, it emits a `MilestoneAchievedEvent` that the network layer broadcasts to all connected clients.

### Implementation Details

```cpp
// Event definition
struct MilestoneAchievedEvent {
    PlayerID player;              // Who achieved the milestone
    MilestoneType milestone;      // Which milestone (enum value)
    uint32_t population;          // Population at time of achievement
    uint32_t server_tick;         // Authoritative tick for sync
    std::string player_name;      // Display name for notification text
};

// In ProgressionSystem::tick()
void ProgressionSystem::check_milestone_thresholds(PlayerID player, uint32_t population) {
    auto& milestone_comp = get_milestone_component(player);

    for (auto threshold : milestone_thresholds) {
        if (population >= threshold.population &&
            !milestone_comp.has_milestone(threshold.type)) {

            milestone_comp.set_milestone(threshold.type);
            milestone_comp.last_milestone_tick = current_tick_;

            // Emit event - NetworkSystem subscribes and broadcasts
            event_bus_.emit(MilestoneAchievedEvent{
                .player = player,
                .milestone = threshold.type,
                .population = population,
                .server_tick = current_tick_,
                .player_name = get_player_name(player)
            });
        }
    }
}
```

**Network Flow:**
1. Server: ProgressionSystem emits `MilestoneAchievedEvent`
2. Server: NetworkSystem subscribes to event, serializes to `MilestoneNotificationMessage`
3. Server: Broadcasts message to ALL connected clients (including the achieving player)
4. Client: NetworkSystem receives message, reconstructs event
5. Client: UISystem subscribes to event, triggers celebration sequence
6. Client: AudioSystem subscribes to event, plays milestone-specific audio

### Latency Tolerance

**Recommended Tolerance:** 100-300ms for celebration synchronization.

**Rationale:**
- Milestone celebrations are non-gameplay-affecting (purely visual/audio)
- Players don't interact with each other during celebrations
- Exact frame-level sync is unnecessary
- Network jitter within 300ms is imperceptible for celebration timing

**Implementation:**
- The `server_tick` field allows clients to interpolate/schedule the celebration
- Clients can delay local celebration start to align with network round-trip estimate
- For simplicity, immediate local playback on receipt is acceptable

**Multi-client Experience:**
- All clients receive the same notification content
- Each client plays their local celebration effects
- The achieving player sees full celebration; others see scaled-down notification
- No strict sync required between clients viewing the celebration

---

## Question 2: Population Counting for Milestones

> Does arcology population count toward milestone thresholds? Example: Player at 115K with arcologies housing 10K - are they at 125K for milestone purposes? Proposed: Yes, all beings count regardless of structure type.

### Technical Approach

**Confirmed: YES, arcology population counts toward milestones.**

ProgressionSystem queries `PopulationSystem::get_total_population(PlayerID)` which returns the **total being count across all habitation types**, including arcologies.

### Implementation Details

PopulationSystem already tracks total population as an aggregate:

```cpp
// PopulationSystem provides this (Epic 10)
uint32_t PopulationSystem::get_total_population(PlayerID player) const {
    uint32_t total = 0;

    // Iterate all habitation entities owned by player
    for (auto [entity, hab, owner] : habitation_view.each()) {
        if (owner.player_id == player) {
            total += hab.current_population;
        }
    }

    // Arcologies are just another habitation type
    // ArcologyComponent entities also have HabitationComponent
    // They're included in the above iteration

    return total;
}
```

**Key Design Points:**
- Arcologies ARE habitation buildings (they have `HabitationComponent`)
- `ArcologyComponent` is an ADDITIONAL component marking special properties
- No special handling needed in ProgressionSystem - it just asks for total population
- This aligns with the Game Designer's proposal

**Example Scenario:**
- Player has 115,000 beings in regular habitation
- Player builds Plymouth Arcology (65,000 capacity), fills to 10,000
- Total population: 125,000
- Milestone check: 125,000 >= 120,000 (Arcology_120K) - **ACHIEVED**

### Performance Consideration

Population aggregation is O(n) where n = habitation entity count. This is already computed by PopulationSystem each tick for other purposes (tribute calculation, demand evaluation). ProgressionSystem simply reads the cached value - no additional computation.

---

## Question 3: Milestone State Persistence

> Are achieved milestones stored in DB per player? If player's population drops below threshold, do they lose milestone? (Proposed: No, once achieved always achieved)

### Technical Approach

**Confirmed: Milestones are PERMANENT once achieved.**

Milestones are stored as a bitmask in `MilestoneComponent`, which is attached to the player entity and persisted via the normal entity serialization system.

### Implementation Details

```cpp
struct MilestoneComponent {
    // Bitmask: bit N set = milestone N achieved (PERMANENT)
    uint8_t achieved_milestones = 0;

    // Tracking field - only ever increases
    uint32_t highest_population_ever = 0;

    // ... other fields ...
};

// In ProgressionSystem::check_milestone_thresholds()
void check_milestone_thresholds(PlayerID player, uint32_t population) {
    auto& comp = get_milestone_component(player);

    // Update peak population (monotonically increasing)
    comp.highest_population_ever = std::max(comp.highest_population_ever, population);

    // Check against thresholds - bits are ONLY EVER SET, never cleared
    for (auto threshold : thresholds) {
        if (population >= threshold.population &&
            !comp.has_milestone(threshold.type)) {
            comp.set_milestone(threshold.type);  // Sets bit, never clears
            // ... emit event ...
        }
    }

    // NOTE: No code path clears milestone bits
}
```

**Persistence:**
- `MilestoneComponent` implements `ISerializable` (defined in Epic 1)
- Saved to game state on save/checkpoint
- Loaded on session restore
- Multiplayer: Server is authoritative; client receives sync on join

**Scenario:**
1. Player reaches 125,000 population - achieves Arcology_120K milestone
2. Catastrophe kills 50,000 beings - population drops to 75,000
3. Milestone status: **Still achieved** (bit remains set)
4. Player still has access to arcology construction
5. `highest_population_ever` remains 125,000 (historical record)

### Design Rationale

Permanent milestones support the Game Designer's emotional journey design:
- Achievements feel meaningful (can't be "lost")
- Population volatility doesn't punish exploration/risk-taking
- Aligns with Core Principle #4 (Endless Sandbox) - no fail states

---

## Question 4: Edict Stacking with Multiple Command Nexuses

> Can a player build multiple Command Nexuses? Do edicts stack if same edict selected? (Proposed: No, one instance per edict) Do edict slot counts stack? (Proposed: No, slots based on milestones not buildings)

### Technical Approach

**Confirmed: Players can build only ONE Command Nexus.**

Command Nexus is a "unique" reward building - only one instance per player.

### Implementation Details

```cpp
// BuildingTemplate for Command Nexus
BuildingTemplate command_nexus_template {
    .id = BUILDING_COMMAND_NEXUS,
    .name = "Command Nexus",
    .required_unlock = RewardBuildingType::CommandNexus,
    .is_unique_per_player = true,  // KEY: Only one allowed
    // ... other fields ...
};

// BuildingSystem checks uniqueness
bool BuildingSystem::can_build_template(PlayerID player, uint32_t template_id) {
    const auto& tmpl = template_registry_.get(template_id);

    // Unique building check
    if (tmpl.is_unique_per_player) {
        if (player_has_building_of_type(player, template_id)) {
            return false;  // Already owns one
        }
    }

    // ... other checks ...
    return true;
}
```

**Edict Slots:**
Slots are tracked in `EdictComponent` and scale with milestones, NOT buildings:

```cpp
// In ProgressionSystem, after milestone achievement
void update_edict_slots(PlayerID player) {
    auto& edict = get_edict_component(player);

    // Slot progression based on milestones
    uint8_t slots = 0;
    if (has_milestone(player, Nexus_10K))    slots = 1;
    if (has_milestone(player, Monument_30K)) slots = 2;
    if (has_milestone(player, Defense_60K))  slots = 3;
    if (has_milestone(player, Landmark_90K)) slots = 4;

    edict.max_active_edicts = slots;
}
```

**Edict Stacking:**
Even in a hypothetical multi-Nexus scenario (not allowed, but for clarity):
- Each edict can only be active ONCE (bitmask representation)
- `active_edicts` is a bitmask - setting the same bit twice has no effect
- No mechanism exists to "double activate" an edict

**Summary:**
| Question | Answer |
|----------|--------|
| Multiple Command Nexuses? | No - unique building |
| Edict stacking? | N/A (only one instance possible per bitmask) |
| Slot stacking from buildings? | No - slots from milestones only |

---

## Question 5: Edict Data Storage

> EdictComponent on player entity? Or dedicated system storage? How are edict effects applied to simulation systems?

### Technical Approach

**Confirmed: EdictComponent is attached to the player entity.**

This follows the ECS pattern where player-specific state lives on the player entity. ProgressionSystem manages the component; consuming systems query it.

### Implementation Details

**Storage:**
```cpp
// EdictComponent attached to player entity
struct EdictComponent {
    uint8_t active_edicts = 0;      // Bitmask of active edicts
    uint8_t max_active_edicts = 0;  // Slots available (from milestones)
    uint32_t last_change_tick = 0;  // For cooldown enforcement
};

// Registry structure
entt::entity player_entity = player_registry.get_entity(player_id);
player_registry.emplace<EdictComponent>(player_entity);
```

**Effect Application Pattern:**

Consuming systems query `IProgressionProvider` during their tick:

```cpp
// Example: DisorderSystem applying EnforcerMandate edict
void DisorderSystem::tick(float delta) {
    for (auto [entity, disorder, owner] : disorder_view.each()) {
        float base_disorder = calculate_base_disorder(entity);

        // Apply edict modifier if active
        if (progression_->is_edict_active(owner.player_id, EdictType::EnforcerMandate)) {
            float modifier = progression_->get_edict_modifier(EdictType::EnforcerMandate);
            base_disorder *= modifier;  // modifier = 0.9 for -10%
        }

        disorder.current_level = base_disorder;
    }
}
```

**Modifier Definition:**
```cpp
// In ProgressionSystem or EdictData configuration
float ProgressionSystem::get_edict_modifier(EdictType edict) const {
    static const std::unordered_map<EdictType, float> modifiers = {
        {EdictType::ColonyHarmony,      1.10f},  // +10% harmony
        {EdictType::GrowthIncentive,    1.15f},  // +15% migration
        {EdictType::TributeForgiveness, 0.90f},  // -10% tribute
        {EdictType::TradeAgreement,     1.20f},  // +20% port income
        {EdictType::EnforcerMandate,    0.90f},  // -10% disorder
        {EdictType::HazardPreparedness, 0.80f},  // -20% damage
        {EdictType::GreenInitiative,    0.85f},  // -15% contamination
    };
    return modifiers.at(edict);
}
```

### Performance Consideration

- Edict queries are O(1) bitmask checks
- Called once per system tick, not per-entity
- Total overhead: ~7 boolean checks per tick across all systems (negligible)

---

## Question 6: Edict Multiplayer Sync

> How quickly do edict changes propagate to clients? Can players see each other's edicts? (Proposed: Yes, via query tool on Command Nexus)

### Technical Approach

**Propagation:** Immediate (within next network frame, typically 50-100ms).
**Visibility:** Yes, via inspection/query tool.

### Implementation Details

**Edict Change Flow:**
```
1. Client: Player clicks to activate "EnforcerMandate" edict
2. Client: Sends EdictChangeRequest message to server

   struct EdictChangeRequest {
       PlayerID player;
       EdictType edict;
       bool activate;  // true = enable, false = disable
   };

3. Server: ProgressionSystem validates request
   - Has Command Nexus built?
   - Has available slot (if activating)?
   - Not in cooldown period?
   - Is this player's request (anti-cheat)?

4. Server: If valid, updates EdictComponent
5. Server: Broadcasts EdictChangedEvent to all clients

   struct EdictChangedEvent {
       PlayerID player;
       EdictType edict;
       bool is_now_active;
   };

6. All Clients: Update local EdictComponent cache
7. Simulation: Next tick uses new edict state
```

**Propagation Timing:**
- Server processing: <1ms
- Network broadcast: 20-80ms typical (depends on latency)
- Client receives: Next network frame
- Simulation effect: Next simulation tick after client receipt

**Total latency:** 50-150ms for effect to be visible in other clients' simulations.

**Visibility to Other Players:**
```cpp
// When player inspects another player's Command Nexus building
struct QueryCommandNexusResponse {
    PlayerID owner;
    std::vector<EdictType> active_edicts;  // What edicts are active
    uint8_t slots_used;
    uint8_t slots_available;
};

// UI shows: "Overseer [Name]'s Edicts: EnforcerMandate, GreenInitiative"
```

**Design Note:** This visibility supports the social dynamics the Game Designer described - players can see how others have shaped their colony's character through policy choices.

---

## Question 7: Arcology Capacity Calculation

> How is total arcology capacity tracked for Transcendence threshold? Is this a running sum, or calculated on demand? ArcologyCapacityComponent on player entity?

### Technical Approach

**Recommended: Calculated on demand with caching.**

Arcology capacity is queried infrequently (only when checking Transcendence unlock), so on-demand calculation is efficient. However, we cache the result for UI display purposes.

### Implementation Details

```cpp
// Query method in ProgressionSystem
uint32_t ProgressionSystem::get_total_arcology_capacity(PlayerID player) const {
    // Check cache validity
    if (arcology_cache_[player].valid &&
        arcology_cache_[player].last_update == current_tick_) {
        return arcology_cache_[player].total_capacity;
    }

    // Calculate on demand
    uint32_t total = 0;
    for (auto [entity, arcology, owner] : arcology_view_.each()) {
        if (owner.player_id == player) {
            total += arcology.base_population_capacity;
        }
    }

    // Cache result
    arcology_cache_[player] = {
        .total_capacity = total,
        .last_update = current_tick_,
        .valid = true
    };

    return total;
}

// Cache invalidation on arcology construction/destruction
void ProgressionSystem::on_arcology_built(ArcologyBuiltEvent& event) {
    arcology_cache_[event.player].valid = false;
}

void ProgressionSystem::on_arcology_destroyed(ArcologyDestroyedEvent& event) {
    arcology_cache_[event.player].valid = false;
}
```

**Transcendence Threshold Check:**
```cpp
// Per Game Designer: 500,000 arcology capacity required
constexpr uint32_t TRANSCENDENCE_THRESHOLD = 500'000;

bool ProgressionSystem::can_build_transcendence_monument(PlayerID player) const {
    if (!has_milestone(player, MilestoneType::Arcology_120K)) {
        return false;  // Must have arcology unlock first
    }

    uint32_t capacity = get_total_arcology_capacity(player);
    return capacity >= TRANSCENDENCE_THRESHOLD;
}
```

**Why Not a Dedicated Component?**
- Arcology count is small (max ~16 per player)
- Query is infrequent (UI display, Transcendence check)
- Caching handles performance concerns
- Avoids duplicate state that could desync

### Performance Consideration

- Iteration over arcologies: O(n) where n = total arcologies (max ~64 for 4 players)
- With caching: O(1) for repeated queries in same tick
- Cache invalidation is event-driven, not polled
- Total overhead: negligible

---

## Question 8: Arcology Self-Containment

> How do arcologies reduce infrastructure strain? Internal energy/fluid generation, or reduced consumption? Proposed: Reduced consumption (50% of normal per capita)

### Technical Approach

**Confirmed: Reduced consumption model (50% of normal per capita).**

Arcologies have internal systems that satisfy part of their residents' needs, reducing demand on external infrastructure.

### Implementation Details

```cpp
struct ArcologyComponent {
    ArcologyType type;
    uint32_t base_population_capacity;
    uint32_t current_population;

    // Self-containment factors (0.0 to 1.0)
    // 0.5 = 50% reduction in external demand
    float energy_self_sufficiency = 0.5f;
    float fluid_self_sufficiency = 0.5f;

    // Derived: External demand multiplier
    float get_external_energy_demand_multiplier() const {
        return 1.0f - energy_self_sufficiency;  // 0.5
    }
    float get_external_fluid_demand_multiplier() const {
        return 1.0f - fluid_self_sufficiency;   // 0.5
    }
};
```

**Integration with EnergySystem:**
```cpp
// EnergySystem calculates demand per habitation
float EnergySystem::calculate_habitation_demand(entt::entity entity) const {
    auto& hab = registry_.get<HabitationComponent>(entity);
    float base_demand = hab.current_population * ENERGY_PER_BEING;

    // Check if this is an arcology
    if (registry_.has<ArcologyComponent>(entity)) {
        auto& arc = registry_.get<ArcologyComponent>(entity);
        base_demand *= arc.get_external_energy_demand_multiplier();
    }

    return base_demand;
}
```

**Same pattern for FluidSystem.**

**Per-Type Variation (per Game Designer's arcology types):**

| Arcology Type | Energy Self-Sufficiency | Fluid Self-Sufficiency | Notes |
|---------------|------------------------|------------------------|-------|
| Genesis | 50% | 60% | Bio-processes recycle water |
| Nexus | 40% | 50% | More tech, more energy need |
| Harmony | 55% | 55% | Balanced sustainability |
| Ascension | 60% | 50% | Advanced tech, efficient |

**Alternative Considered (Internal Generation):**
We considered having arcologies generate their own energy/fluid, but this creates complexity:
- Would need to model internal power plants
- Partial coverage scenarios are confusing
- Doesn't integrate cleanly with grid-based distribution

**Reduced consumption is simpler and achieves the same gameplay goal:** arcologies are efficient, high-density housing that doesn't overwhelm infrastructure.

---

## Question 9: Monument Unlock Check

> When is arcology capacity checked against threshold? Is it continuous, or triggered by arcology construction? Event-driven: "Check threshold on arcology construction complete"?

### Technical Approach

**Recommended: Event-driven check on arcology construction/destruction.**

This is more efficient than continuous polling and provides immediate feedback to players.

### Implementation Details

```cpp
// ProgressionSystem subscribes to arcology events
void ProgressionSystem::initialize() {
    event_bus_.subscribe<ArcologyConstructionCompleteEvent>(
        [this](auto& event) { on_arcology_built(event); }
    );
    event_bus_.subscribe<ArcologyDestroyedEvent>(
        [this](auto& event) { on_arcology_destroyed(event); }
    );
}

void ProgressionSystem::on_arcology_built(ArcologyConstructionCompleteEvent& event) {
    PlayerID player = event.owner;

    // Invalidate cache
    arcology_cache_[player].valid = false;

    // Check Transcendence unlock
    check_transcendence_unlock(player);
}

void ProgressionSystem::check_transcendence_unlock(PlayerID player) {
    // Already unlocked?
    if (has_transcendence_unlock(player)) {
        return;
    }

    uint32_t capacity = get_total_arcology_capacity(player);

    if (capacity >= TRANSCENDENCE_THRESHOLD) {
        // Unlock Transcendence Monument construction
        set_transcendence_unlock(player, true);

        // Notify player
        event_bus_.emit(TranscendenceUnlockedEvent{
            .player = player,
            .total_arcology_capacity = capacity
        });
    }
}
```

**UI Feedback:**
- After each arcology construction, UI shows progress: "Arcology Capacity: 325,000 / 500,000"
- When threshold is reached: "Transcendence Unlocked! You may now construct the Transcendence Monument."

**Edge Case - Arcology Destruction:**
If an arcology is destroyed (catastrophe, player demolition), capacity decreases. However:
- **Transcendence unlock is PERMANENT** once achieved (like milestones)
- Player can still build Transcendence Monument even if capacity drops below threshold
- This mirrors the milestone permanence design

```cpp
void ProgressionSystem::on_arcology_destroyed(ArcologyDestroyedEvent& event) {
    // Invalidate cache (for UI display)
    arcology_cache_[event.owner].valid = false;

    // NOTE: Do NOT revoke Transcendence unlock
    // Once unlocked, always unlocked
}
```

---

## Question 10: Monument Effects Radius

> How are area effects (harmony boost, contamination immunity) implemented? TranscendenceAuraComponent with radius queries? Performance implications of 50-tile radius effects?

### Technical Approach

**Recommended: Pre-computed affected tile set with dirty flag invalidation.**

A 50-tile radius is large (~7,850 tiles in a circle). Per-tick spatial queries would be expensive. Instead, we pre-compute the affected tile set when the monument is placed and cache it.

### Implementation Details

```cpp
// Component attached to Transcendence Monument entity
struct TranscendenceAuraComponent {
    // Pre-computed affected tiles (indexed by effect type)
    std::vector<TileCoord> harmony_tiles;       // 50-tile radius
    std::vector<TileCoord> contamination_tiles; // 30-tile radius
    std::vector<TileCoord> sector_value_tiles;  // 20-tile radius
    std::vector<TileCoord> catastrophe_tiles;   // 25-tile radius

    // Monument position (for invalidation check)
    TileCoord position;

    // Dirty flag (monuments don't move, so this is rarely true)
    bool needs_recompute = false;
};

// Pre-computation on monument placement
void ProgressionSystem::on_transcendence_monument_placed(MonumentPlacedEvent& event) {
    auto& aura = registry_.emplace<TranscendenceAuraComponent>(event.entity);
    aura.position = event.position;

    // Compute affected tiles for each radius
    aura.harmony_tiles = compute_tiles_in_radius(event.position, 50);
    aura.contamination_tiles = compute_tiles_in_radius(event.position, 30);
    aura.sector_value_tiles = compute_tiles_in_radius(event.position, 20);
    aura.catastrophe_tiles = compute_tiles_in_radius(event.position, 25);
}

std::vector<TileCoord> compute_tiles_in_radius(TileCoord center, int radius) {
    std::vector<TileCoord> tiles;
    tiles.reserve(M_PI * radius * radius);  // Approximate circle area

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                TileCoord tile{center.x + dx, center.y + dy};
                if (is_valid_tile(tile)) {
                    tiles.push_back(tile);
                }
            }
        }
    }
    return tiles;
}
```

**Effect Application:**

Consuming systems iterate over pre-computed tile lists:

```cpp
// ContaminationSystem - immunity within 30 tiles
void ContaminationSystem::tick(float delta) {
    // Get all transcendence monuments
    for (auto [entity, aura] : monument_view.each()) {
        // Clear contamination in affected tiles
        for (const auto& tile : aura.contamination_tiles) {
            contamination_grid_.set(tile, 0.0f);  // Forced to zero
        }
    }

    // Normal contamination processing for other tiles...
}

// HarmonySystem - +25% in 50 tiles
void HarmonySystem::apply_monument_bonus(TileCoord tile) {
    for (auto [entity, aura] : monument_view.each()) {
        // Use spatial hash or binary search for O(log n) lookup
        if (std::binary_search(aura.harmony_tiles.begin(),
                               aura.harmony_tiles.end(), tile)) {
            return 1.25f;  // 25% bonus
        }
    }
    return 1.0f;
}
```

### Performance Analysis

**Memory Cost:**
- 50-tile radius circle: ~7,850 tiles
- Each TileCoord: 8 bytes (2x int32)
- Per monument: ~63 KB for harmony tiles
- Total for all radii: ~150 KB per monument
- Acceptable for rare end-game buildings

**Runtime Cost:**
- Pre-computation: O(r^2) one-time cost (~31ms for r=50)
- Effect application: O(n) where n = affected tiles
- For contamination immunity: Iterate 2,827 tiles (30-radius) once per tick
- With SIMD optimization: ~0.1ms per monument

**Optimization: Spatial Hashing**
For O(1) "is this tile affected?" queries:

```cpp
struct TranscendenceAuraComponent {
    // Hash set for fast lookup
    std::unordered_set<TileCoord, TileCoordHash> harmony_set;
    std::unordered_set<TileCoord, TileCoordHash> contamination_set;
    // ...
};

// O(1) lookup
bool is_in_harmony_radius(const TranscendenceAuraComponent& aura, TileCoord tile) {
    return aura.harmony_set.contains(tile);
}
```

**Multiple Monuments:**
- Players can build multiple Transcendence Monuments (per Game Designer)
- Effects do NOT stack (harmony is capped, contamination is binary)
- Implementation: Union of affected tile sets, deduplicated

```cpp
// For multiple monuments, effects cap (don't stack)
float get_harmony_multiplier(TileCoord tile) {
    float bonus = 1.0f;
    for (auto& aura : all_monument_auras) {
        if (aura.harmony_set.contains(tile)) {
            bonus = std::max(bonus, 1.25f);  // Cap at 25%, don't stack
            break;  // Early exit since it's capped
        }
    }
    return bonus;
}
```

### Summary

| Concern | Mitigation |
|---------|------------|
| Large radius (50 tiles) | Pre-computed tile set, cached |
| Per-tick query cost | Spatial hash set for O(1) lookup |
| Memory usage | ~150 KB per monument (acceptable) |
| Multiple monuments | Effects cap, don't stack |
| Monument destruction | Remove component, systems skip |

---

## Summary Table

| # | Question Topic | Key Answer |
|---|---------------|------------|
| 1 | Milestone Notification | Server broadcasts `MilestoneAchievedEvent` to all clients; 100-300ms latency tolerance |
| 2 | Population Counting | Yes, arcology population counts (all beings count regardless of structure) |
| 3 | Milestone Persistence | Permanent once achieved; stored in `MilestoneComponent` bitmask |
| 4 | Multiple Command Nexuses | No - unique building; slots from milestones, not buildings |
| 5 | Edict Storage | `EdictComponent` on player entity; systems query via `IProgressionProvider` |
| 6 | Edict Sync | Immediate propagation (50-150ms); visible via query tool |
| 7 | Arcology Capacity | Calculated on demand with caching; event-driven cache invalidation |
| 8 | Arcology Self-Containment | Reduced consumption (50% of normal per capita) |
| 9 | Monument Unlock Check | Event-driven on arcology construction; unlock is permanent |
| 10 | Monument Effects Radius | Pre-computed tile sets; spatial hash for O(1) lookup; effects cap (don't stack) |

---

**End of Systems Architect Answers**
