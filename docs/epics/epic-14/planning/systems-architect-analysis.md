# Systems Architect Analysis: Epic 14 - Progression & Rewards

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.14.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 14 introduces the ProgressionSystem -- a lightweight, late-tick system that tracks population milestones, manages unlock states for reward buildings, and handles the arcology system. This epic respects Canon Principle #4 (Endless Sandbox) by providing **celebratory milestones without win conditions**.

Key architectural insights:

1. **Simple Data Model:** ProgressionSystem owns minimal state (milestone flags, unlock bitmask, arcology counts) and primarily queries PopulationSystem
2. **Per-Player Tracking:** Each player has independent milestone progression in multiplayer
3. **BuildingSystem Extension:** Reward buildings integrate via existing template prerequisite checks
4. **No New Dense Grids:** Unlike most simulation systems, Progression uses entity-based state
5. **Late Tick Priority:** Runs after all simulation systems have updated their state (priority ~90)

This epic has **minimal retroactive impact** on earlier epics -- primarily extending existing interfaces rather than modifying them.

---

## Table of Contents

1. [Critical Design Decisions (Pre-Reconciled)](#1-critical-design-decisions-pre-reconciled)
2. [ProgressionSystem Design](#2-progressionsystem-design)
3. [Component Definitions](#3-component-definitions)
4. [Interface Definitions](#4-interface-definitions)
5. [Integration Points](#5-integration-points)
6. [Retroactive Changes to Earlier Epics](#6-retroactive-changes-to-earlier-epics)
7. [Data Flow Analysis](#7-data-flow-analysis)
8. [Multiplayer Considerations](#8-multiplayer-considerations)
9. [Key Work Items](#9-key-work-items)
10. [Questions for Game Designer](#10-questions-for-game-designer)
11. [Risks and Concerns](#11-risks-and-concerns)
12. [Canon Updates Required](#12-canon-updates-required)

---

## 1. Critical Design Decisions (Pre-Reconciled)

These decisions were made in pre-planning discussion and are **non-negotiable** for this analysis:

### 1.1 NO YEAR-BASED UNLOCKS

**Decision:** Use population thresholds only, not simulation time (cycles/years).

**Rationale:**
- Simplifies ISimulationTime usage (no need for "year X reached" tracking)
- Fits the endless sandbox model better (time-based unlocks feel arbitrary in endless play)
- Population reflects actual colony development, not just waiting

**Implication:** ProgressionSystem does NOT depend on ISimulationTime for unlock logic.

### 1.2 NO WIN CONDITIONS

**Decision:** The "Exodus" feature from SimCity 2000 is REPLACED with "Transcendence Monument."

**Canon Compliance:** Core Principle #4 states: "No win/lose states. Endless play."

**Transcendence Monument:**
- Unlocks at the highest population milestone (150K+ beings)
- A celebratory landmark, NOT a game ending
- Players can build it, admire it, and continue playing
- Multiple players can each build their own Transcendence Monument

### 1.3 POPULATION-ONLY UNLOCK THRESHOLDS

| Population | Unlock |
|------------|--------|
| 2,000 beings | Overseer's Sanctum (prestige building) |
| 10,000 beings | Command Nexus (unlocks Edicts) |
| 30,000 beings | Monument (customizable landmark) |
| 60,000 beings | Defense Installation (disaster mitigation) |
| 90,000 beings | Special Landmark |
| 120,000 beings | Arcologies unlock |
| 150,000+ beings | Additional arcology types + Transcendence Monument |

### 1.4 INDIVIDUAL PROGRESSION

**Decision:** Each player tracks their own milestones in multiplayer.

**Behavior:**
- Player A at 50K population has different unlocks than Player B at 8K
- Achievements are personal
- When ANY player hits a milestone, a shared notification celebrates it
- No shared unlock pool

### 1.5 REWARD BUILDINGS = PRESTIGE + OPTIONAL UTILITY

**Decision:** Reward buildings are NEVER required for core gameplay.

**Design:**
- Reward buildings provide cosmetic prestige and optional bonuses
- A player can reach 150K population without building ANY reward structures
- Edicts (from Command Nexus) are passive bonuses, not required mechanics

---

## 2. ProgressionSystem Design

### 2.1 System Identity

```yaml
ProgressionSystem:
  type: ecs_system
  tick_priority: 90  # After all simulation systems (LandValueSystem is 85)
  owns:
    - Milestone state per player
    - Unlock flags per player
    - Arcology tracking per player
    - Edict state per player
  does_not_own:
    - Population count (PopulationSystem owns)
    - Reward building placement (BuildingSystem owns)
    - Reward building definitions (BuildingTemplateRegistry owns)
  provides:
    - IProgressionProvider: progression state queries for other systems
    - Milestone event emission
  depends_on:
    - PopulationSystem (population queries)
    - BuildingSystem (arcology count queries, prerequisite checking)
    - UISystem (milestone notification display)
```

### 2.2 Tick Priority Rationale

**Priority 90** is chosen because:

1. ProgressionSystem only READS data, never WRITES to other systems' state
2. It must run AFTER PopulationSystem (50) to have accurate population counts
3. It must run AFTER all systems that affect population (energy, fluid, services, etc.)
4. LandValueSystem is at 85, and ContaminationSystem is at 80 -- Progression should run after both
5. It's purely reactive: "Did population cross a threshold this tick?"

### 2.3 System Responsibilities

#### Milestone Detection

Each tick, ProgressionSystem:
1. Queries PopulationSystem for current population per player
2. Compares against milestone thresholds
3. If a new threshold is crossed: sets milestone flag, emits event

#### Unlock State Management

- Maintains a bitmask of unlocked building types per player
- Provides query interface for BuildingSystem prerequisite checks
- Does NOT control what buildings exist -- only what CAN be built

#### Arcology Tracking

- Arcologies are high-capacity special buildings
- Each player has limited arcology slots (suggested: 4 max)
- ProgressionSystem tracks: arcologies built, arcology types unlocked

#### Edict Management

- Edicts are passive colony bonuses unlocked via Command Nexus
- ProgressionSystem tracks which edicts are active per player
- Edicts have simple effects (e.g., -5% disorder, +10% harmony)
- Effects are applied by querying systems (DisorderSystem queries active edicts)

---

## 3. Component Definitions

### 3.1 MilestoneComponent

Attached to each player entity. Tracks milestone state for that player.

```cpp
// Milestone threshold identifiers
enum class MilestoneType : uint8_t {
    Sanctum_2K      = 0,  // 2,000 beings
    Nexus_10K       = 1,  // 10,000 beings
    Monument_30K    = 2,  // 30,000 beings
    Defense_60K     = 3,  // 60,000 beings
    Landmark_90K    = 4,  // 90,000 beings
    Arcology_120K   = 5,  // 120,000 beings
    Transcendence_150K = 6,  // 150,000 beings
    COUNT           = 7
};

struct MilestoneComponent {
    // === Milestone State ===
    // Bitmask: bit N set = milestone N achieved
    uint8_t achieved_milestones = 0;  // Supports up to 8 milestones

    // === Tracking ===
    uint32_t highest_population_ever = 0;  // Peak population reached
    uint32_t current_population_cached = 0; // Cached from PopulationSystem (for UI)

    // === Timing ===
    uint32_t last_milestone_tick = 0;  // When last milestone was achieved

    // === Derived (for quick checks) ===
    uint8_t milestone_count = 0;  // Number of milestones achieved

    // === Helper Methods (non-canon but practical) ===
    bool has_milestone(MilestoneType type) const {
        return (achieved_milestones & (1 << static_cast<uint8_t>(type))) != 0;
    }

    void set_milestone(MilestoneType type) {
        achieved_milestones |= (1 << static_cast<uint8_t>(type));
        milestone_count = __builtin_popcount(achieved_milestones);
    }
};
```

**Size Analysis:** ~14 bytes per player (trivial)

### 3.2 RewardBuildingComponent

Marker component for reward buildings. Attached to specific building entities.

```cpp
enum class RewardBuildingType : uint8_t {
    OverseerSanctum     = 0,  // Prestige building, minor harmony bonus
    CommandNexus        = 1,  // Unlocks Edicts
    Monument            = 2,  // Customizable landmark
    DefenseInstallation = 3,  // Disaster mitigation
    SpecialLandmark     = 4,  // Visual landmark, major prestige
    Arcology            = 5,  // High-capacity habitation
    TranscendenceMonument = 6, // Ultimate celebratory building
    COUNT               = 7
};

struct RewardBuildingComponent {
    RewardBuildingType reward_type;

    // Customization (for Monument, Transcendence Monument)
    uint8_t style_variant = 0;  // Visual variant index

    // Arcology-specific
    uint8_t arcology_tier = 0;  // 0 = not arcology, 1-3 = arcology tiers

    // Effect tracking (some reward buildings have ongoing effects)
    bool effect_active = true;  // False if unpowered/damaged
};
```

**Size Analysis:** ~4 bytes per reward building entity

### 3.3 ArcologyComponent

Specialized component for arcology buildings. Attached alongside BuildingComponent and RewardBuildingComponent.

```cpp
enum class ArcologyType : uint8_t {
    Plymouth       = 0,  // Standard arcology (SimCity 2000 reference)
    Forest         = 1,  // Nature-integrated
    Darco          = 2,  // Industrial focus
    Launch         = 3   // Space-themed (SimCity 2000 "Launch Arco" reference)
};

struct ArcologyComponent {
    ArcologyType arcology_type;

    // Capacity (arcologies are high-capacity habitation)
    uint32_t base_population_capacity = 0;  // e.g., 65,000 for Plymouth
    uint32_t current_population = 0;        // Filled by PopulationSystem

    // Self-contained systems (arcologies have internal utilities)
    uint8_t internal_energy_efficiency = 100;  // 0-100%, reduces external energy need
    uint8_t internal_fluid_efficiency = 100;   // 0-100%, reduces external fluid need

    // Status
    bool is_fully_operational = false;  // All internal systems working
};
```

**Size Analysis:** ~12 bytes per arcology entity

### 3.4 EdictComponent

Attached to player entity. Tracks active edicts for that player.

```cpp
enum class EdictType : uint8_t {
    // Harmony/Population
    ColonyHarmony       = 0,  // +10% harmony
    GrowthIncentive     = 1,  // +15% migration in-rate

    // Economy
    TributeForgiveness  = 2,  // -10% tribute collection but +5% demand
    TradeAgreement      = 3,  // +20% port income (if ports exist)

    // Safety
    EnforcerMandate     = 4,  // -10% disorder
    HazardPreparedness  = 5,  // -20% disaster damage

    // Environment
    GreenInitiative     = 6,  // -15% contamination generation

    COUNT               = 7
};

struct EdictComponent {
    // Bitmask: bit N set = edict N is active
    uint8_t active_edicts = 0;  // Supports up to 8 edicts

    // Max active edicts (scales with Command Nexus level or population)
    uint8_t max_active_edicts = 2;  // Default: 2, can increase

    // Timing for edict changes (cooldown between toggling)
    uint32_t last_edict_change_tick = 0;

    // Helper
    bool is_edict_active(EdictType type) const {
        return (active_edicts & (1 << static_cast<uint8_t>(type))) != 0;
    }

    uint8_t active_count() const {
        return __builtin_popcount(active_edicts);
    }
};
```

**Size Analysis:** ~6 bytes per player

### 3.5 Component Summary

| Component | Attached To | Size | Count |
|-----------|------------|------|-------|
| MilestoneComponent | Player entity | ~14 bytes | 4 (one per player) |
| RewardBuildingComponent | Reward building entities | ~4 bytes | ~20-50 per game |
| ArcologyComponent | Arcology entities | ~12 bytes | ~16 (4 per player max) |
| EdictComponent | Player entity | ~6 bytes | 4 (one per player) |

**Total Memory:** Negligible (<1 KB total)

---

## 4. Interface Definitions

### 4.1 IProgressionProvider

Interface for querying progression state. Implemented by ProgressionSystem.

```cpp
class IProgressionProvider {
public:
    virtual ~IProgressionProvider() = default;

    // === Milestone Queries ===

    // Check if a player has achieved a specific milestone
    virtual bool has_milestone(PlayerID player, MilestoneType milestone) const = 0;

    // Get the highest milestone achieved by a player
    virtual MilestoneType get_highest_milestone(PlayerID player) const = 0;

    // Get the number of milestones achieved
    virtual uint8_t get_milestone_count(PlayerID player) const = 0;

    // Get population required for next milestone (0 if all achieved)
    virtual uint32_t get_next_milestone_population(PlayerID player) const = 0;

    // === Unlock Queries ===

    // Check if a reward building type is unlocked for a player
    virtual bool is_reward_building_unlocked(
        PlayerID player,
        RewardBuildingType type
    ) const = 0;

    // Check if a specific arcology type is unlocked
    virtual bool is_arcology_type_unlocked(
        PlayerID player,
        ArcologyType type
    ) const = 0;

    // Get list of all unlocked reward building types
    virtual std::vector<RewardBuildingType> get_unlocked_rewards(
        PlayerID player
    ) const = 0;

    // === Arcology Queries ===

    // Get number of arcologies built by player
    virtual uint8_t get_arcology_count(PlayerID player) const = 0;

    // Get maximum arcologies allowed for player
    virtual uint8_t get_max_arcologies(PlayerID player) const = 0;

    // Can player build another arcology?
    virtual bool can_build_arcology(PlayerID player) const = 0;

    // === Edict Queries ===

    // Check if a specific edict is active for a player
    virtual bool is_edict_active(PlayerID player, EdictType edict) const = 0;

    // Get all active edicts for a player
    virtual std::vector<EdictType> get_active_edicts(PlayerID player) const = 0;

    // Get modifier value for an edict type (e.g., 0.9 for -10% disorder)
    virtual float get_edict_modifier(EdictType edict) const = 0;

    // === Statistics ===

    // Get player's peak population ever reached
    virtual uint32_t get_peak_population(PlayerID player) const = 0;
};
```

### 4.2 IUnlockPrerequisite

Interface that BuildingSystem uses to check if a building can be constructed.

```cpp
class IUnlockPrerequisite {
public:
    virtual ~IUnlockPrerequisite() = default;

    // Check if a building template's prerequisites are met
    // Returns true if player can build this template
    virtual bool meets_prerequisites(
        PlayerID player,
        uint32_t template_id
    ) const = 0;

    // Get reason why prerequisites are not met (for UI)
    virtual std::string get_prerequisite_failure_reason(
        PlayerID player,
        uint32_t template_id
    ) const = 0;
};
```

**Note:** This interface is implemented by ProgressionSystem but is designed for BuildingSystem to consume. BuildingSystem already uses the forward dependency pattern (Section 8 of Building Engineer Analysis), so this fits naturally.

---

## 5. Integration Points

### 5.1 PopulationSystem Integration

**Dependency Direction:** ProgressionSystem reads from PopulationSystem (no circular dependency).

**Query Pattern:**
```cpp
// In ProgressionSystem::tick()
for (auto [player_entity, milestone] : player_view.each()) {
    PlayerID player = get_player_id(player_entity);
    uint32_t population = population_system_->get_total_population(player);

    milestone.current_population_cached = population;
    milestone.highest_population_ever = std::max(
        milestone.highest_population_ever,
        population
    );

    check_milestone_thresholds(player, population);
}
```

**Interface Used:** ProgressionSystem queries `PopulationSystem::get_total_population(PlayerID)` which already exists per Epic 10 analysis.

**New Interface Needed:** None. PopulationSystem already provides population queries.

### 5.2 BuildingSystem Integration

**Dependency Direction:** BuildingSystem queries ProgressionSystem for prerequisites.

**Integration Pattern:**
```cpp
// BuildingSystem uses IUnlockPrerequisite (implemented by ProgressionSystem)
class BuildingSystem {
private:
    IUnlockPrerequisite* unlock_checker_;  // Set via dependency injection

public:
    bool can_build_template(PlayerID player, uint32_t template_id) {
        // Existing checks...
        if (!terrain_->is_buildable(x, y)) return false;
        if (!energy_->is_powered_at(x, y, player)) return false;
        // ... etc ...

        // NEW: Progression prerequisite check
        if (unlock_checker_ && !unlock_checker_->meets_prerequisites(player, template_id)) {
            return false;
        }

        return true;
    }
};
```

**Template Modification:**
```cpp
struct BuildingTemplate {
    // ... existing fields ...

    // NEW: Progression prerequisite
    std::optional<RewardBuildingType> required_unlock = std::nullopt;
    // If set, this template requires the specified reward unlock
};
```

**Integration Summary:**
1. BuildingTemplateRegistry marks reward building templates with `required_unlock`
2. BuildingSystem calls `IUnlockPrerequisite::meets_prerequisites()` before spawning
3. ProgressionSystem implements the check by looking at the template's `required_unlock` and the player's milestone state

### 5.3 UISystem Integration

**Dependency Direction:** UISystem queries ProgressionSystem for display.

**Data Provided:**
- Current milestone count and details per player
- Progress toward next milestone (percentage)
- Unlocked reward buildings list
- Active edicts list

**Events Consumed:**
- `MilestoneAchievedEvent` triggers a notification

### 5.4 EconomySystem Integration

**Dependency Direction:** EconomySystem may query ProgressionSystem for edict effects.

**Pattern:**
```cpp
// In EconomySystem when calculating tribute collection
if (progression_->is_edict_active(player, EdictType::TributeForgiveness)) {
    tribute_multiplier *= progression_->get_edict_modifier(EdictType::TributeForgiveness);
    // TributeForgiveness returns 0.9 (10% reduction)
}
```

### 5.5 DisorderSystem Integration

**Dependency Direction:** DisorderSystem may query ProgressionSystem for edict effects.

**Pattern:**
```cpp
// In DisorderSystem when calculating disorder generation
if (progression_->is_edict_active(player, EdictType::EnforcerMandate)) {
    disorder_multiplier *= progression_->get_edict_modifier(EdictType::EnforcerMandate);
    // EnforcerMandate returns 0.9 (10% reduction)
}
```

### 5.6 DisasterSystem Integration

**Dependency Direction:** DisasterSystem queries ProgressionSystem for disaster mitigation.

**Pattern:**
```cpp
// Defense Installation provides localized disaster mitigation
// DisasterSystem checks for nearby Defense Installations when calculating damage

// Also check HazardPreparedness edict for global mitigation
if (progression_->is_edict_active(player, EdictType::HazardPreparedness)) {
    damage_multiplier *= progression_->get_edict_modifier(EdictType::HazardPreparedness);
    // HazardPreparedness returns 0.8 (20% reduction)
}
```

---

## 6. Retroactive Changes to Earlier Epics

### 6.1 Epic 4: BuildingSystem

**Changes Required:**

| Change | Type | Impact |
|--------|------|--------|
| Add `IUnlockPrerequisite* unlock_checker_` member | Extension | Low |
| Add `set_unlock_checker(IUnlockPrerequisite*)` setter | Extension | Low |
| Modify `can_build_template()` to call prerequisite check | Modification | Low |
| Add `required_unlock` field to BuildingTemplate | Extension | Low |

**Assessment:** These are minor changes. The forward dependency pattern (IUnlockPrerequisite) follows the same pattern already established for IEnergyProvider, IFluidProvider, etc. No stub implementation needed -- the check simply skips if `unlock_checker_` is null.

### 6.2 Epic 10: PopulationSystem

**Changes Required:**

| Change | Type | Impact |
|--------|------|--------|
| Emit `PopulationMilestoneEvent` at thresholds | Extension | Low |

**Assessment:** Epic 10 analysis (Section 9, Question 13) already anticipated milestone events. The PopulationEngineer asked: "Should PopulationSystem emit events at these thresholds?" The answer is YES.

**Event Definition:**
```cpp
struct PopulationMilestoneEvent {
    PlayerID player;
    uint32_t milestone;  // Population threshold reached
    uint32_t current_population;
};
```

**Implementation Option A:** PopulationSystem emits the event.
**Implementation Option B:** ProgressionSystem detects the crossing and emits the event.

**Recommendation:** Option B (ProgressionSystem emits). This keeps PopulationSystem focused on population mechanics and gives ProgressionSystem ownership of milestone logic. PopulationSystem does NOT need to know about milestone thresholds.

### 6.3 Epic 11: EconomySystem

**Changes Required:**

| Change | Type | Impact |
|--------|------|--------|
| Query ProgressionSystem for edict modifiers | Extension | Low |

**Assessment:** Minor addition. EconomySystem already applies modifiers from various sources. Adding an edict query is trivial.

### 6.4 Canon Updates for Earlier Epic Stubs

**Epic 4 stub pattern:**
The Building Engineer Analysis (Section 8) established the forward dependency stub pattern. For Epic 14, we extend this:

```cpp
// Can be added to StubProviders.h
class StubUnlockPrerequisite : public IUnlockPrerequisite {
public:
    bool meets_prerequisites(PlayerID, uint32_t) const override {
        return true;  // All builds allowed until ProgressionSystem exists
    }

    std::string get_prerequisite_failure_reason(PlayerID, uint32_t) const override {
        return "";  // No failures in stub
    }
};
```

**Integration Timeline:**
- Epic 4: Uses StubUnlockPrerequisite (all builds allowed)
- Epic 14: Replaces stub with ProgressionSystem

---

## 7. Data Flow Analysis

### 7.1 Tick Data Flow

```
                    TICK N DATA FLOW (Progression)
                    ===============================

PopulationSystem(50) --> TotalPopulation per player
      |
      v
[... other systems 52-85 ...]
      |
      v
ProgressionSystem(90)
      |
      +-- Reads: PopulationSystem.get_total_population(player)
      +-- Reads: BuildingSystem.get_arcology_count(player) (if needed)
      |
      +-- Writes: MilestoneComponent.achieved_milestones
      +-- Writes: EdictComponent (on player edict change request)
      |
      +-- Emits: MilestoneAchievedEvent (if threshold crossed)
      |
      v
[Tick complete, events processed]
      |
      v
UISystem (receives MilestoneAchievedEvent, displays notification)
```

### 7.2 Edict Effect Flow

Edicts are passive modifiers. Systems query them as needed:

```
Player activates EdictType::EnforcerMandate
      |
      v
ProgressionSystem sets bit in EdictComponent.active_edicts
      |
      v
[Next tick...]
      |
      v
DisorderSystem::tick()
      |
      +-- Queries: progression_->is_edict_active(player, EnforcerMandate)
      +-- If true: disorder_multiplier *= 0.9
      |
      v
Disorder calculation uses modified multiplier
```

### 7.3 Reward Building Unlock Flow

```
Player reaches 10,000 population
      |
      v
ProgressionSystem detects threshold crossing
      |
      +-- Sets MilestoneComponent bit for Nexus_10K
      +-- Emits MilestoneAchievedEvent(player, 10000, population)
      |
      v
[Player opens build menu]
      |
      v
UISystem queries ProgressionSystem.get_unlocked_rewards(player)
      |
      +-- Returns: [OverseerSanctum, CommandNexus] (unlocked so far)
      |
      v
UI displays Command Nexus as buildable
      |
      v
[Player clicks to build Command Nexus]
      |
      v
BuildingSystem.can_build_template(player, command_nexus_template)
      |
      +-- Calls: unlock_checker_->meets_prerequisites(player, template_id)
      +-- Template has required_unlock = RewardBuildingType::CommandNexus
      +-- ProgressionSystem checks: has_milestone(player, Nexus_10K)
      +-- Returns: true (milestone achieved)
      |
      v
BuildingSystem proceeds with construction
```

---

## 8. Multiplayer Considerations

### 8.1 Per-Player State

All progression state is per-player:

| Data | Scope | Notes |
|------|-------|-------|
| MilestoneComponent | Per-player | Each player has own milestones |
| EdictComponent | Per-player | Each player has own edicts |
| Arcology count | Per-player | Each player limited to own arcologies |
| Unlock flags | Per-player | Derived from milestones, per-player |

### 8.2 Server Authority

| Action | Authority | Notes |
|--------|-----------|-------|
| Milestone detection | Server | Server runs PopulationSystem, detects crossings |
| Edict activation | Server | Client requests, server validates and applies |
| Unlock state | Server | Derived from server-authoritative population |
| Reward building placement | Server | Normal BuildingSystem authority |

### 8.3 Sync Requirements

| Data | Sync Method | Frequency |
|------|-------------|-----------|
| MilestoneComponent | Delta on change | On milestone achievement only |
| EdictComponent | Delta on change | On edict toggle only |
| MilestoneAchievedEvent | Broadcast | Immediate to all clients |

**Bandwidth Analysis:**
- Milestone achievements are rare (7 total milestones, ~30 bytes per event)
- Edict toggles are infrequent (player choice, ~8 bytes per change)
- Total progression sync: negligible

### 8.4 Shared Celebration

When ANY player achieves a milestone:
1. Server emits `MilestoneAchievedEvent` with player ID
2. ALL clients receive the event
3. UI displays a shared notification: "Overseer [PlayerName] has reached 10,000 beings!"

This creates a social, celebratory moment without affecting gameplay balance (other players don't gain unlocks from another player's progress).

### 8.5 Late Join

When a new player joins mid-game:
- They start with 0 population and no milestones
- They must grow their colony to unlock rewards
- They see notifications when other players hit milestones

---

## 9. Key Work Items

### Group A: Component Infrastructure

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P01 | MilestoneComponent definition | S | Define component per Section 3.1 |
| 14-P02 | RewardBuildingComponent definition | S | Define component per Section 3.2 |
| 14-P03 | ArcologyComponent definition | S | Define component per Section 3.3 |
| 14-P04 | EdictComponent definition | S | Define component per Section 3.4 |
| 14-P05 | Progression enums | S | MilestoneType, RewardBuildingType, ArcologyType, EdictType |

### Group B: Interfaces

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P06 | IProgressionProvider interface | M | Define interface per Section 4.1 |
| 14-P07 | IUnlockPrerequisite interface | S | Define interface per Section 4.2 |
| 14-P08 | StubUnlockPrerequisite | S | Stub implementation for BuildingSystem |

### Group C: Core Progression System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P09 | ProgressionSystem class | L | Main system, ISimulatable priority 90 |
| 14-P10 | Milestone detection logic | M | Check population thresholds, set flags |
| 14-P11 | Unlock state derivation | M | Derive unlocks from milestone flags |
| 14-P12 | MilestoneAchievedEvent | S | Event definition and emission |

### Group D: Edict System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P13 | Edict activation/deactivation | M | UI request handling, validation |
| 14-P14 | Edict effect modifiers | M | Define modifier values per edict type |
| 14-P15 | Edict query integration points | M | Hook into Economy, Disorder, Disaster systems |

### Group E: Arcology System

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P16 | Arcology count tracking | S | Track arcologies built per player |
| 14-P17 | Arcology slot limits | S | Enforce max arcologies per player |
| 14-P18 | Arcology type unlocks | S | Track which arcology types are available |
| 14-P19 | Arcology building templates | M | Define 4 arcology templates |

### Group F: Reward Building Templates

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P20 | Overseer's Sanctum template | S | Prestige building definition |
| 14-P21 | Command Nexus template | M | Edict-enabling building definition |
| 14-P22 | Monument template | M | Customizable landmark definition |
| 14-P23 | Defense Installation template | M | Disaster mitigation building definition |
| 14-P24 | Special Landmark template | S | Visual landmark definition |
| 14-P25 | Transcendence Monument template | M | Ultimate celebratory building definition |

### Group G: BuildingSystem Integration

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P26 | Add IUnlockPrerequisite to BuildingSystem | S | Dependency injection point |
| 14-P27 | Add required_unlock to BuildingTemplate | S | Template field extension |
| 14-P28 | Prerequisite check in can_build_template | S | Integration logic |

### Group H: UI Integration

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P29 | Milestone notification handler | M | Display milestone achievements |
| 14-P30 | Unlocked rewards display | M | Show available reward buildings |
| 14-P31 | Edict management panel | L | UI for activating/deactivating edicts |
| 14-P32 | Progression overview panel | M | Show milestones, progress, peak population |

### Group I: Network & Serialization

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P33 | Component serialization | M | ISerializable for all progression components |
| 14-P34 | Edict request message | S | Client -> Server edict toggle |
| 14-P35 | Milestone broadcast | S | Server -> All clients notification |

### Group J: Testing

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| 14-P36 | Unit tests: milestone detection | M | Test threshold crossing logic |
| 14-P37 | Unit tests: unlock derivation | M | Test unlock flag generation |
| 14-P38 | Unit tests: edict effects | M | Test modifier calculations |
| 14-P39 | Integration test: full progression | L | End-to-end milestone -> unlock -> build |

### Work Item Summary

| Size | Count |
|------|-------|
| S | 16 |
| M | 18 |
| L | 4 |
| **Total** | **38** |

### Critical Path

```
14-P01/02/03/04/05 (Components) ----+
14-P06/07/08 (Interfaces) ---------+--> 14-P09 (ProgressionSystem)
                                            |
                                            v
                              14-P10 (Milestone detection)
                                            |
                                            v
                              14-P11 (Unlock derivation) --> 14-P26/27/28 (BuildingSystem integration)
                                            |
                                            v
                              14-P12 (Events) --> 14-P29 (UI notifications)
                                            |
                              +-------------+-------------+
                              |             |             |
                              v             v             v
                        14-P13/14/15    14-P16/17/18  14-P20-25
                        (Edicts)        (Arcology)    (Templates)
                              |             |             |
                              +-------------+-------------+
                                            |
                                            v
                                  14-P30/31/32 (UI panels)
                                            |
                                            v
                                  14-P33/34/35 (Network)
                                            |
                                            v
                                  14-P36-39 (Testing)
```

---

## 10. Questions for Game Designer

The following questions require Game Designer input before finalizing implementation details:

### Q1: Edict Passive Bonuses

**Question:** What specific passive bonuses should each Edict provide?

**Proposed Values (for validation):**

| Edict | Effect | Magnitude |
|-------|--------|-----------|
| ColonyHarmony | +harmony | +10% |
| GrowthIncentive | +migration in | +15% |
| TributeForgiveness | -tribute collected, +demand | -10% tribute, +5% demand |
| TradeAgreement | +port income | +20% |
| EnforcerMandate | -disorder | -10% |
| HazardPreparedness | -disaster damage | -20% |
| GreenInitiative | -contamination generation | -15% |

**Follow-up:** How many edicts can a player have active simultaneously? Proposed: 2 at Command Nexus unlock, +1 per 50K population above 10K (max 5).

### Q2: Arcology Visual Styles

**Question:** Should arcologies have different visual styles?

**Proposed:**
- **Plymouth:** Classic dome structure, balanced aesthetics
- **Forest:** Organic, bioluminescent vegetation-integrated design
- **Darco:** Industrial, angular, fabrication-focused appearance
- **Launch:** Sleek, vertical, space-age aesthetics

**Follow-up:** Do different arcology types have different gameplay effects, or are they purely cosmetic variants?

**Proposed Gameplay Effects:**

| Arcology | Capacity | Special |
|----------|----------|---------|
| Plymouth | 65,000 | Balanced (baseline) |
| Forest | 55,000 | +20% harmony, -10% capacity |
| Darco | 70,000 | +10% fabrication jobs, +5% contamination |
| Launch | 50,000 | -50% energy cost, -15% capacity |

### Q3: Transcendence Monument Mechanics

**Question:** How should the Transcendence Monument work exactly?

**Proposed:**
- **Unlock:** 150,000+ population
- **Cost:** Very high (proposed: 500,000 credits)
- **Footprint:** 4x4 tiles (largest building in game)
- **Effect:** Purely celebratory -- no gameplay bonus
- **Customization:** Player can choose from several visual styles
- **Limit:** One per player
- **Multiplayer:** Other players see the monument; shared celebration notification when built

**Follow-up:** Should building the Transcendence Monument trigger any special event (fireworks, colony-wide celebration animation)?

### Q4: Defense Installation Specifics

**Question:** How does the Defense Installation provide disaster mitigation?

**Proposed:**
- **Coverage Radius:** 20 tiles
- **Effect:** -30% disaster damage to buildings within radius
- **Stacking:** Multiple installations do NOT stack (prevents exploit)
- **Types Affected:** All disaster types (fire, seismic, storm, etc.)
- **Limit:** One per player (prestige building, not spammable)

### Q5: Monument Customization

**Question:** What customization options should the Monument offer?

**Proposed:**
- **Style:** 4-6 predefined visual styles (alien obelisk, crystal spire, bio-sculpture, etc.)
- **Color:** Accent color selection from bioluminescent palette
- **Scale:** Fixed (no size customization -- would affect footprint)
- **Name:** Player can give it a custom name (displayed in UI query)

### Q6: Overseer's Sanctum and Special Landmark Effects

**Question:** Do these buildings have gameplay effects or are they purely prestige?

**Proposed:**

| Building | Prestige Effect | Gameplay Effect |
|----------|-----------------|-----------------|
| Overseer's Sanctum | High land value boost in radius | +5% harmony colony-wide |
| Special Landmark | Highest land value boost in radius | None (pure prestige) |

---

## 11. Risks and Concerns

### R1: Edict System Complexity (MEDIUM)

**Risk:** Edicts interact with multiple systems (Economy, Disorder, Disaster, Contamination). If poorly integrated, effects may be inconsistent or hard to debug.

**Mitigation:**
- Edict modifiers are simple multipliers (0.85, 0.90, 1.10, etc.)
- Each consuming system queries ProgressionSystem once per tick, applies modifier
- No complex stacking rules -- edicts apply independently
- Comprehensive unit tests for each edict's effect

### R2: Milestone Event Flooding (LOW)

**Risk:** If population oscillates around a threshold, milestone events could spam.

**Mitigation:**
- Milestones are one-time achievements -- once set, the flag stays set even if population drops
- `highest_population_ever` tracks peak, ensuring milestones aren't re-triggered
- Event is only emitted once per milestone, ever

### R3: Arcology Balance (MEDIUM)

**Risk:** Arcologies are very high capacity (65K+ beings). They could trivialize progression if too easy to build.

**Mitigation:**
- Very high cost (proposed: 200,000-500,000 credits per arcology)
- Requires 120K population to unlock (already a significant achievement)
- Limited slots per player (4 max)
- Arcologies still require energy/fluid infrastructure

### R4: Command Nexus as Mandatory (LOW)

**Risk:** If edicts are too powerful, Command Nexus becomes effectively mandatory, violating the "optional utility" principle.

**Mitigation:**
- Edict effects are intentionally mild (10-20% modifiers)
- A colony can thrive without any edicts active
- Command Nexus provides OPTIONAL optimization, not required mechanics

### R5: Late-Game Pacing (MEDIUM)

**Risk:** After achieving all milestones (150K+ population), there's no further progression carrot.

**Mitigation:**
- Canon Principle #4 (Endless Sandbox) explicitly states no win conditions
- Post-milestone gameplay is about refinement, aesthetics, multiplayer interaction
- Future consideration: repeatable challenge achievements (not scope of Epic 14)

---

## 12. Canon Updates Required

### 12.1 systems.yaml Updates

Add ProgressionSystem to Phase 4:

```yaml
epic_14_progression:
  name: "Progression & Rewards"
  systems:
    ProgressionSystem:
      type: ecs_system
      tick_priority: 90  # After LandValueSystem (85)
      owns:
        - Milestone tracking per player
        - Reward building unlock state
        - Arcology system
        - Edict state management
      does_not_own:
        - Reward building placement (BuildingSystem owns)
        - Population count (PopulationSystem owns)
      provides:
        - "IProgressionProvider: progression state queries"
        - Milestone event emission
      depends_on:
        - PopulationSystem (population queries)
        - BuildingSystem (arcology count queries)
      multiplayer:
        authority: server
        per_player: true  # Each player has own progression
      notes:
        - "No year-based unlocks - population thresholds only"
        - "No win conditions - Transcendence Monument is celebratory, not victory"
        - "Edicts are optional passive bonuses"

      components:
        - MilestoneComponent
        - RewardBuildingComponent
        - ArcologyComponent
        - EdictComponent
```

### 12.2 interfaces.yaml Updates

Add IProgressionProvider:

```yaml
progression:
  IProgressionProvider:
    description: "Query progression state for milestone and unlock checks"
    purpose: "Allows UISystem, BuildingSystem, and other consumers to query progression without direct ProgressionSystem coupling"

    methods:
      - name: has_milestone
        params:
          - name: player
            type: PlayerID
          - name: milestone
            type: MilestoneType
        returns: bool
        description: "Whether player has achieved the specified milestone"

      - name: is_reward_building_unlocked
        params:
          - name: player
            type: PlayerID
          - name: type
            type: RewardBuildingType
        returns: bool
        description: "Whether player has unlocked the specified reward building"

      - name: is_edict_active
        params:
          - name: player
            type: PlayerID
          - name: edict
            type: EdictType
        returns: bool
        description: "Whether the specified edict is currently active for player"

      - name: get_edict_modifier
        params:
          - name: edict
            type: EdictType
        returns: float
        description: "Get the modifier value for an edict (e.g., 0.9 for -10%)"

    implemented_by:
      - ProgressionSystem (Epic 14)
```

### 12.3 patterns.yaml Updates

No new dense grids needed. Add note about progression component pattern:

```yaml
progression_components:
  description: "Lightweight per-player tracking for progression state"
  pattern: "Entity-component attached to player entities, not dense grids"
  applies_to:
    - MilestoneComponent (per-player milestone flags)
    - EdictComponent (per-player active edicts)
  rationale: |
    Unlike simulation grids (disorder, contamination), progression state is:
    - Per-player only (4 players max)
    - Rarely changing (milestones are one-time, edicts change infrequently)
    - Not spatially distributed
    Therefore, standard ECS components are appropriate, not dense grids.
```

### 12.4 ISimulatable Priority Update

Add to interfaces.yaml ISimulatable.implemented_by:

```yaml
- ProgressionSystem (priority: 90)
```

---

## Appendix A: Milestone Threshold Quick Reference

| Milestone | Population | Unlocks |
|-----------|------------|---------|
| Sanctum_2K | 2,000 | Overseer's Sanctum |
| Nexus_10K | 10,000 | Command Nexus (enables Edicts) |
| Monument_30K | 30,000 | Monument (customizable landmark) |
| Defense_60K | 60,000 | Defense Installation |
| Landmark_90K | 90,000 | Special Landmark |
| Arcology_120K | 120,000 | Arcologies (Plymouth type) |
| Transcendence_150K | 150,000 | Additional arcology types + Transcendence Monument |

---

## Appendix B: Edict Effect Quick Reference

| Edict | Target System | Effect |
|-------|---------------|--------|
| ColonyHarmony | PopulationSystem | +10% harmony |
| GrowthIncentive | PopulationSystem | +15% migration in |
| TributeForgiveness | EconomySystem | -10% tribute, +5% demand |
| TradeAgreement | PortSystem (Epic 8) | +20% port income |
| EnforcerMandate | DisorderSystem | -10% disorder |
| HazardPreparedness | DisasterSystem | -20% damage |
| GreenInitiative | ContaminationSystem | -15% contamination |

---

## Appendix C: File Organization (Proposed)

```
src/
  components/
    ProgressionComponents.h     # All progression component definitions
    ProgressionTypes.h          # Enums (MilestoneType, EdictType, etc.)
  systems/
    progression/
      ProgressionSystem.h       # Main system declaration
      ProgressionSystem.cpp     # Tick logic, milestone detection
      EdictManager.h            # Edict activation/deactivation
      EdictManager.cpp
      ArcologyTracker.h         # Arcology count/slot management
      ArcologyTracker.cpp
    interfaces/
      IProgressionProvider.h    # Query interface
      IUnlockPrerequisite.h     # BuildingSystem prerequisite interface
  events/
    ProgressionEvents.h         # MilestoneAchievedEvent, EdictChangedEvent
  network/
    messages/
      ProgressionMessages.h     # Edict request, milestone broadcast
      ProgressionMessages.cpp
```

---

**End of Systems Architect Analysis: Epic 14 - Progression & Rewards**
