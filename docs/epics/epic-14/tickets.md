# Epic 14: Progression & Rewards - Ticket Breakdown

**Canon Version:** 0.14.0
**Date:** 2026-02-01
**Total Tickets:** 52
**Status:** Ready for Implementation

---

## Executive Summary

Epic 14 introduces the ProgressionSystem - a lightweight, late-tick system that tracks population milestones, manages unlock states for reward buildings, handles the arcology system, and provides colony-wide edicts. This epic respects Canon Principle #4 (Endless Sandbox) by providing celebratory milestones without win conditions.

### Key Design Decisions

| Decision | Value | Source |
|----------|-------|--------|
| Unlock basis | Population thresholds only (NO year-based) | Pre-reconciled |
| Win conditions | NONE - Transcendence Monument replaces Exodus | Pre-reconciled |
| Progression scope | Individual per player in multiplayer | Pre-reconciled |
| Reward buildings | Prestige + optional utility | Pre-reconciled |
| ProgressionSystem tick_priority | 90 | Systems Architect |
| Max edict slots | 4 (milestone-based) | Game Designer |
| Transcendence unlock | 150K population | Game Designer |
| Transcendence limit | One per player (not repeatable) | Game Designer |
| Defense Installation | One per player, 25-tile radius, no variants | Game Designer |
| Command Nexus | Unique building (one per player) | Game Designer |
| Arcology population | Counts toward milestones | Systems Architect |
| Milestones | Permanent once achieved | Systems Architect |
| Arcology capacity | Calculated on demand with caching | Systems Architect |

---

## Milestone Thresholds

| Population | Milestone | Unlocks |
|------------|-----------|---------|
| 2,000 beings | Sanctum_2K | Overseer's Sanctum |
| 10,000 beings | Nexus_10K | Command Nexus (enables Edicts), 1 edict slot |
| 30,000 beings | Monument_30K | Monument (Eternity Marker), 2 edict slots |
| 60,000 beings | Defense_60K | Defense Installation (Aegis Complex), 3 edict slots |
| 90,000 beings | Landmark_90K | Special Landmark (Resonance Spire), 4 edict slots |
| 120,000 beings | Arcology_120K | All 4 arcology types |
| 150,000 beings | Transcendence_150K | Transcendence Monument |

---

## Group A: Component Infrastructure

### Ticket 14-A01: Define Progression Enums

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define all enum types used by the progression system in a dedicated header file. This includes MilestoneType, RewardBuildingType, ArcologyType, and EdictType.

**Acceptance Criteria:**
- [ ] MilestoneType enum with 7 values (Sanctum_2K through Transcendence_150K)
- [ ] RewardBuildingType enum with 7 values (OverseerSanctum through TranscendenceMonument)
- [ ] ArcologyType enum with 4 values (Plymouth, Forest, Darco, Launch)
- [ ] EdictType enum with 7 values per game designer specification
- [ ] All enums use uint8_t as underlying type for compact storage
- [ ] COUNT sentinel value for each enum

**Dependencies:**
- Blocked by: None
- Blocks: 14-A02, 14-A03, 14-A04, 14-A05

**Canon References:**
- terminology.yaml: progression section
- terminology: arcology -> arcology (keep as-is)

---

### Ticket 14-A02: Define MilestoneComponent

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define the MilestoneComponent that tracks milestone state per player. This component is attached to player entities and stores achieved milestones as a bitmask.

**Acceptance Criteria:**
- [ ] uint8_t achieved_milestones bitmask field
- [ ] uint32_t highest_population_ever field (monotonically increasing)
- [ ] uint32_t current_population_cached field (for UI)
- [ ] uint32_t last_milestone_tick field
- [ ] uint8_t milestone_count derived field
- [ ] has_milestone(MilestoneType) helper method
- [ ] set_milestone(MilestoneType) helper method (only sets, never clears)
- [ ] Component size approximately 14 bytes

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-C01, 14-D01

**Canon References:**
- patterns.yaml: progression_components (entity-component, not dense grid)

---

### Ticket 14-A03: Define RewardBuildingComponent

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define the RewardBuildingComponent marker component for reward buildings. Attached to specific building entities to identify them as reward structures.

**Acceptance Criteria:**
- [ ] RewardBuildingType reward_type field
- [ ] uint8_t style_variant field (for Monument, Transcendence customization)
- [ ] uint8_t arcology_tier field (0 = not arcology, 1-3 = tiers)
- [ ] bool effect_active field (false if unpowered/damaged)
- [ ] Component size approximately 4 bytes

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-G01, 14-I01

**Canon References:**
- terminology.yaml: buildings section
- terminology: mayors_mansion -> overseers_sanctum

---

### Ticket 14-A04: Define ArcologyComponent

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define the ArcologyComponent for arcology buildings. Attached alongside BuildingComponent and RewardBuildingComponent to mark buildings as arcologies with special properties.

**Acceptance Criteria:**
- [ ] ArcologyType arcology_type field
- [ ] uint32_t base_population_capacity field
- [ ] uint32_t current_population field
- [ ] float energy_self_sufficiency field (default 0.5)
- [ ] float fluid_self_sufficiency field (default 0.5)
- [ ] bool is_fully_operational field
- [ ] get_external_energy_demand_multiplier() helper
- [ ] get_external_fluid_demand_multiplier() helper
- [ ] Component size approximately 20 bytes

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-F01, 14-F02

**Canon References:**
- terminology.yaml: progression -> arcology (keep as-is)

---

### Ticket 14-A05: Define EdictComponent

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define the EdictComponent that tracks active edicts per player. Attached to player entity, not Command Nexus building.

**Acceptance Criteria:**
- [ ] uint8_t active_edicts bitmask field
- [ ] uint8_t max_active_edicts field (scales with milestones: 1/2/3/4)
- [ ] uint32_t last_edict_change_tick field (for cooldown enforcement)
- [ ] is_edict_active(EdictType) helper method
- [ ] active_count() helper method
- [ ] Component size approximately 6 bytes

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-E01, 14-E02

**Canon References:**
- terminology.yaml: (add edict -> edict)

---

### Ticket 14-A06: Define TranscendenceAuraComponent

**Type:** infrastructure
**Priority:** P1 (should have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Define the TranscendenceAuraComponent for Transcendence Monument area effects. Uses pre-computed tile sets for efficient radius queries.

**Acceptance Criteria:**
- [ ] TileCoord position field
- [ ] std::vector<TileCoord> harmony_tiles (30-tile radius)
- [ ] std::vector<TileCoord> contamination_tiles (25-tile radius)
- [ ] std::vector<TileCoord> sector_value_tiles (15-tile radius)
- [ ] std::vector<TileCoord> catastrophe_tiles (20-tile radius)
- [ ] std::unordered_set variants for O(1) lookup
- [ ] bool needs_recompute flag (rarely true since monuments don't move)
- [ ] Pre-compute tiles on monument placement

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-H02, 14-H03

**Canon References:**
- patterns.yaml: pre-computed tile sets with dirty flag invalidation

---

## Group B: Interfaces

### Ticket 14-B01: Define IProgressionProvider Interface

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Define the IProgressionProvider interface for querying progression state. Implemented by ProgressionSystem, consumed by UISystem, BuildingSystem, and effect-consuming systems.

**Acceptance Criteria:**
- [ ] has_milestone(PlayerID, MilestoneType) -> bool
- [ ] get_highest_milestone(PlayerID) -> MilestoneType
- [ ] get_milestone_count(PlayerID) -> uint8_t
- [ ] get_next_milestone_population(PlayerID) -> uint32_t
- [ ] is_reward_building_unlocked(PlayerID, RewardBuildingType) -> bool
- [ ] is_arcology_type_unlocked(PlayerID, ArcologyType) -> bool
- [ ] get_unlocked_rewards(PlayerID) -> std::vector<RewardBuildingType>
- [ ] get_arcology_count(PlayerID) -> uint8_t
- [ ] get_max_arcologies(PlayerID) -> uint8_t
- [ ] can_build_arcology(PlayerID) -> bool
- [ ] is_edict_active(PlayerID, EdictType) -> bool
- [ ] get_active_edicts(PlayerID) -> std::vector<EdictType>
- [ ] get_edict_modifier(EdictType) -> float
- [ ] get_peak_population(PlayerID) -> uint32_t
- [ ] get_total_arcology_capacity(PlayerID) -> uint32_t

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-C01, 14-E03, 14-I02

**Canon References:**
- interfaces.yaml: IProgressionProvider section to be added

---

### Ticket 14-B02: Define IUnlockPrerequisite Interface

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** S

**Description:**
Define the IUnlockPrerequisite interface for BuildingSystem to check if a building can be constructed based on progression state.

**Acceptance Criteria:**
- [ ] meets_prerequisites(PlayerID, uint32_t template_id) -> bool
- [ ] get_prerequisite_failure_reason(PlayerID, uint32_t template_id) -> std::string
- [ ] Interface designed for forward dependency pattern

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-B03, 14-I02

**Canon References:**
- patterns.yaml: forward dependency stub pattern (from Epic 4)

---

### Ticket 14-B03: Implement StubUnlockPrerequisite

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** S

**Description:**
Implement a stub version of IUnlockPrerequisite that allows all builds. Used by BuildingSystem until ProgressionSystem is available.

**Acceptance Criteria:**
- [ ] meets_prerequisites() always returns true
- [ ] get_prerequisite_failure_reason() always returns empty string
- [ ] Follows existing StubProviders.h pattern from Epic 4

**Dependencies:**
- Blocked by: 14-B02
- Blocks: 14-I02

**Canon References:**
- patterns.yaml: stub provider pattern

---

## Group C: Core ProgressionSystem

### Ticket 14-C01: Implement ProgressionSystem Class

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** L

**Description:**
Implement the core ProgressionSystem class that implements ISimulatable with tick_priority 90. The system manages milestone detection, unlock states, and provides IProgressionProvider implementation.

**Acceptance Criteria:**
- [ ] Implements ISimulatable with priority 90
- [ ] Implements IProgressionProvider interface
- [ ] Implements IUnlockPrerequisite interface
- [ ] Initializes MilestoneComponent on player entities
- [ ] Initializes EdictComponent on player entities
- [ ] Queries PopulationSystem for total population per player
- [ ] Runs after all simulation systems (LandValueSystem at 85)
- [ ] Handles player join (initialize components)
- [ ] Handles player leave (cleanup)

**Dependencies:**
- Blocked by: 14-A02, 14-A05, 14-B01, 14-B02
- Blocks: 14-D01, 14-E01, 14-F01

**Canon References:**
- systems.yaml: epic_14_progression entry
- patterns.yaml: ecs_system pattern

---

### Ticket 14-C02: Define ProgressionSystem Events

**Type:** infrastructure
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define all events emitted by ProgressionSystem including milestone achievements, edict changes, and transcendence unlock.

**Acceptance Criteria:**
- [ ] MilestoneAchievedEvent: PlayerID, MilestoneType, uint32_t population, uint32_t server_tick, std::string player_name
- [ ] EdictChangedEvent: PlayerID, EdictType, bool is_now_active
- [ ] TranscendenceUnlockedEvent: PlayerID, uint32_t total_arcology_capacity
- [ ] ArcologyConstructionCompleteEvent: PlayerID owner, entt::entity building
- [ ] All events follow existing event patterns

**Dependencies:**
- Blocked by: 14-A01
- Blocks: 14-D02, 14-E02, 14-K03

**Canon References:**
- terminology.yaml: notification -> alert_pulse

---

## Group D: Milestone System

### Ticket 14-D01: Implement Milestone Detection Logic

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Implement the milestone detection logic that runs each tick. Checks population against thresholds, sets milestone flags, and emits events.

**Acceptance Criteria:**
- [ ] Query PopulationSystem.get_total_population(player) each tick
- [ ] Update current_population_cached in MilestoneComponent
- [ ] Update highest_population_ever (monotonically increasing)
- [ ] Check all 7 milestone thresholds
- [ ] Set milestone bit when threshold crossed (never clear)
- [ ] Emit MilestoneAchievedEvent when milestone newly achieved
- [ ] Update milestone_count derived field
- [ ] Milestones permanent even if population drops

**Dependencies:**
- Blocked by: 14-C01
- Blocks: 14-D02, 14-J01

**Canon References:**
- terminology.yaml: population -> colony_population, milestone -> achievement

---

### Ticket 14-D02: Implement Unlock State Derivation

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Implement the logic that derives unlock states from achieved milestones. Maps milestones to available reward buildings and arcology types.

**Acceptance Criteria:**
- [ ] Sanctum_2K unlocks OverseerSanctum
- [ ] Nexus_10K unlocks CommandNexus
- [ ] Monument_30K unlocks Monument
- [ ] Defense_60K unlocks DefenseInstallation
- [ ] Landmark_90K unlocks SpecialLandmark (Resonance Spire)
- [ ] Arcology_120K unlocks all 4 arcology types simultaneously
- [ ] Transcendence_150K unlocks TranscendenceMonument
- [ ] get_unlocked_rewards() returns correct list based on milestones
- [ ] is_reward_building_unlocked() uses milestone check

**Dependencies:**
- Blocked by: 14-D01, 14-C02
- Blocks: 14-I02, 14-J02

**Canon References:**
- patterns.yaml: unlock state derivation from milestone flags

---

### Ticket 14-D03: Implement Edict Slot Progression

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Implement edict slot progression based on milestones. Slots increase as players achieve population milestones.

**Acceptance Criteria:**
- [ ] Nexus_10K: max_active_edicts = 1
- [ ] Monument_30K: max_active_edicts = 2
- [ ] Defense_60K: max_active_edicts = 3
- [ ] Landmark_90K: max_active_edicts = 4 (MAX)
- [ ] Update EdictComponent.max_active_edicts on milestone achievement
- [ ] No edict slots before Command Nexus is built (even with milestone)

**Dependencies:**
- Blocked by: 14-D01
- Blocks: 14-E01

**Canon References:**
- Game Designer Answers Q1: 4 max slots, not 5

---

## Group E: Edict System

### Ticket 14-E01: Implement Edict Activation/Deactivation

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Implement edict activation and deactivation logic including validation, cooldown enforcement, and state updates.

**Acceptance Criteria:**
- [ ] Validate Command Nexus exists before allowing edict changes
- [ ] Validate slot availability when activating
- [ ] Enforce 10-cycle cooldown between edict changes
- [ ] Update active_edicts bitmask
- [ ] Update last_edict_change_tick
- [ ] Same edict cannot be selected twice (bitmask prevents)
- [ ] Emit EdictChangedEvent on state change
- [ ] Edicts cease if Command Nexus destroyed

**Dependencies:**
- Blocked by: 14-C01, 14-D03
- Blocks: 14-E02, 14-E03

**Canon References:**
- Game Designer Answers Q1: 10-cycle cooldown, Command Nexus required

---

### Ticket 14-E02: Define Edict Effect Modifiers

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Define the effect modifier values for each edict type. Modifiers are simple multipliers applied by consuming systems.

**Acceptance Criteria:**
- [ ] ColonyHarmony: 1.10 (multiply harmony by 1.10)
- [ ] GrowthIncentive: 1.15 (multiply migration rate by 1.15)
- [ ] TributeForgiveness: 0.90 tribute, 1.05 demand (two modifiers)
- [ ] TradeAgreement: 1.20 (multiply port income by 1.20)
- [ ] EnforcerMandate: 0.90 (multiply disorder by 0.90)
- [ ] HazardPreparedness: 0.80 (multiply disaster damage by 0.80)
- [ ] GreenInitiative: 0.85 (multiply contamination by 0.85)
- [ ] get_edict_modifier(EdictType) returns correct value
- [ ] Static lookup table for O(1) access

**Dependencies:**
- Blocked by: 14-E01
- Blocks: 14-E03

**Canon References:**
- Game Designer Answers Q1: specific modifier values

---

### Ticket 14-E03: Implement Edict Query Integration Points

**Type:** feature
**Priority:** P1 (should have)
**System:** Multiple (Population, Economy, Disorder, Disaster, Contamination)
**Size:** M

**Description:**
Add edict query integration points to consuming systems. Each system queries IProgressionProvider for active edict modifiers.

**Acceptance Criteria:**
- [ ] PopulationSystem: Apply ColonyHarmony (+10% harmony)
- [ ] PopulationSystem: Apply GrowthIncentive (+15% migration)
- [ ] EconomySystem: Apply TributeForgiveness (-10% tribute, +5% demand)
- [ ] EconomySystem: Apply TradeAgreement (+20% port income)
- [ ] DisorderSystem: Apply EnforcerMandate (-10% disorder)
- [ ] DisasterSystem: Apply HazardPreparedness (-20% damage)
- [ ] ContaminationSystem: Apply GreenInitiative (-15% contamination)
- [ ] All queries through IProgressionProvider interface
- [ ] Graceful handling if ProgressionSystem unavailable

**Dependencies:**
- Blocked by: 14-B01, 14-E02
- Blocks: 14-L04

**Canon References:**
- patterns.yaml: system query pattern via interfaces

---

## Group F: Arcology System

### Ticket 14-F01: Implement Arcology Count Tracking

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Implement tracking of arcology count per player. Subscribe to arcology construction/destruction events and maintain count.

**Acceptance Criteria:**
- [ ] Subscribe to ArcologyConstructionCompleteEvent
- [ ] Subscribe to ArcologyDestroyedEvent
- [ ] Maintain arcology count per player
- [ ] get_arcology_count(PlayerID) returns correct count
- [ ] Count includes all arcology types

**Dependencies:**
- Blocked by: 14-C01, 14-A04
- Blocks: 14-F02, 14-F03

**Canon References:**
- Systems Architect Analysis: arcology count tracking

---

### Ticket 14-F02: Implement Arcology Slot Limits

**Type:** feature
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Implement enforcement of maximum arcologies per player. Default max is 4 per player.

**Acceptance Criteria:**
- [ ] get_max_arcologies(PlayerID) returns 4 (configurable)
- [ ] can_build_arcology(PlayerID) checks count < max
- [ ] IUnlockPrerequisite integration for arcology templates
- [ ] Clear error message when limit reached

**Dependencies:**
- Blocked by: 14-F01
- Blocks: 14-F04

**Canon References:**
- Systems Architect Analysis: max 4 arcologies per player

---

### Ticket 14-F03: Implement Arcology Capacity Calculation

**Type:** feature
**Priority:** P1 (should have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Implement on-demand calculation of total arcology capacity with caching. Used for Transcendence Monument unlock check.

**Acceptance Criteria:**
- [ ] get_total_arcology_capacity(PlayerID) method
- [ ] Iterate ArcologyComponent entities owned by player
- [ ] Sum base_population_capacity values
- [ ] Cache result with tick-based invalidation
- [ ] Invalidate cache on arcology construction/destruction events
- [ ] O(n) calculation where n = arcology count (max 16)

**Dependencies:**
- Blocked by: 14-F01
- Blocks: 14-H01

**Canon References:**
- Systems Architect Answers Q7: calculated on demand with caching

---

### Ticket 14-F04: Define Arcology Building Templates

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define building templates for all 4 arcology types with their unique properties and gameplay effects.

**Acceptance Criteria:**
- [ ] Plymouth: 65K capacity, 200K cost, 800 cr/cycle maintenance, baseline
- [ ] Forest: 55K capacity, 180K cost, 700 cr/cycle, +15% harmony in 15-tile radius
- [ ] Darco: 75K capacity, 220K cost, 900 cr/cycle, +10% fabrication in 10-tile radius
- [ ] Launch: 50K capacity, 250K cost, 600 cr/cycle, -40% energy consumption
- [ ] All arcologies have 4x4 footprint
- [ ] All unlock at Arcology_120K milestone
- [ ] Self-sufficiency: 50% reduced energy/fluid consumption (except Launch)
- [ ] Required_unlock set to ArcologyType enum

**Dependencies:**
- Blocked by: 14-F02, 14-A04
- Blocks: 14-I01

**Canon References:**
- Game Designer Answers Q2: arcology specifications
- terminology.yaml: arcology -> arcology

---

## Group G: Reward Building Templates

### Ticket 14-G01: Define Overseer's Sanctum Template

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** S

**Description:**
Define the Overseer's Sanctum building template - the first reward building unlocked at 2K population.

**Acceptance Criteria:**
- [ ] Footprint: 2x2 tiles
- [ ] Cost: 10,000 credits
- [ ] Maintenance: 100 credits/cycle
- [ ] Unique per player (is_unique_per_player = true)
- [ ] Required_unlock: OverseerSanctum (Sanctum_2K milestone)
- [ ] Effect: +5% colony-wide harmony
- [ ] Effect: +15 sector_value to adjacent 8 tiles
- [ ] Must have HabitationComponent (it's a dwelling)

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-J02

**Canon References:**
- terminology.yaml: mayors_mansion -> overseers_sanctum
- Game Designer Answers Q6: +5% harmony, +15 sector value

---

### Ticket 14-G02: Define Command Nexus Template

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define the Command Nexus building template - unlocks at 10K and enables the Edict system.

**Acceptance Criteria:**
- [ ] Footprint: 3x3 tiles
- [ ] Cost: 25,000 credits
- [ ] Maintenance: 250 credits/cycle
- [ ] Unique per player (is_unique_per_player = true)
- [ ] Required_unlock: CommandNexus (Nexus_10K milestone)
- [ ] Effect: Enables edict system for player
- [ ] Effect: +10% service effectiveness in 10-tile radius
- [ ] Destruction disables all active edicts

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-E01, 14-J02

**Canon References:**
- terminology.yaml: city_hall -> command_nexus
- Game Designer Analysis: Command Nexus unlocks Edicts

---

### Ticket 14-G03: Define Monument (Eternity Marker) Template

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define the Monument (Eternity Marker) building template - unlocked at 30K with customization options.

**Acceptance Criteria:**
- [ ] Footprint: 2x2 tiles
- [ ] Cost: 50,000 credits
- [ ] Maintenance: 150 credits/cycle
- [ ] Unique per player (is_unique_per_player = true)
- [ ] Required_unlock: Monument (Monument_30K milestone)
- [ ] Effect: +15% exchange demand in 15-tile radius
- [ ] Effect: +25 sector_value to adjacent tiles
- [ ] Customization: 4 style variants (Obelisk, Spiral, Bloom, Nexus)
- [ ] Customization: 6 color themes
- [ ] Customization: 32-character custom name

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-J02, 14-J03

**Canon References:**
- Game Designer Answers Q5: customization options
- terminology.yaml: statue -> monument

---

### Ticket 14-G04: Define Defense Installation (Aegis Complex) Template

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define the Defense Installation (Aegis Complex) building template - unlocked at 60K for disaster mitigation.

**Acceptance Criteria:**
- [ ] Footprint: 3x3 tiles
- [ ] Cost: 100,000 credits
- [ ] Maintenance: 500 credits/cycle
- [ ] Unique per player (is_unique_per_player = true)
- [ ] Required_unlock: DefenseInstallation (Defense_60K milestone)
- [ ] Effect: -30% disaster damage in 25-tile radius
- [ ] Effect: +20% service speed in radius
- [ ] Effect: -30% emergency response delay
- [ ] NO variants (single type only)
- [ ] Applies to all disaster types uniformly

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-J02

**Canon References:**
- Game Designer Answers Q4: 25-tile radius, -30% damage, no variants
- terminology.yaml: military_base -> defense_installation

---

### Ticket 14-G05: Define Resonance Spire (Special Landmark) Template

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define the Resonance Spire building template - unlocked at 90K as the ultimate pre-arcology landmark.

**Acceptance Criteria:**
- [ ] Footprint: 3x3 tiles
- [ ] Cost: 150,000 credits
- [ ] Maintenance: 400 credits/cycle
- [ ] Unique per player (is_unique_per_player = true)
- [ ] Required_unlock: SpecialLandmark (Landmark_90K milestone)
- [ ] Effect: +15% harmony in 20-tile radius
- [ ] Effect: +30 sector_value in 15-tile radius
- [ ] Effect: +20% exchange demand in 25-tile radius
- [ ] Effect: -10% contamination generation in 15-tile radius
- [ ] Visible from anywhere on map (server-wide beacon)

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-J02

**Canon References:**
- Game Designer Answers Q6: +15% harmony, multiple effects
- Game Designer Analysis: Resonance Spire

---

## Group H: Transcendence Monument

### Ticket 14-H01: Implement Transcendence Unlock Check

**Type:** feature
**Priority:** P1 (should have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Implement the check for Transcendence Monument unlock. Based on 150K population milestone (simplified from arcology capacity).

**Acceptance Criteria:**
- [ ] Unlock requires Transcendence_150K milestone (150,000 beings)
- [ ] Check is event-driven (on population milestone achievement)
- [ ] Unlock is PERMANENT once achieved
- [ ] Emit TranscendenceUnlockedEvent when unlocked
- [ ] is_reward_building_unlocked returns true for TranscendenceMonument after unlock

**Dependencies:**
- Blocked by: 14-D01, 14-F03
- Blocks: 14-H02

**Canon References:**
- Game Designer Answers Q3: 150K population unlock (simplified)

---

### Ticket 14-H02: Define Transcendence Monument Template

**Type:** feature
**Priority:** P1 (should have)
**System:** BuildingSystem
**Size:** M

**Description:**
Define the Transcendence Monument building template - the ultimate celebratory structure.

**Acceptance Criteria:**
- [ ] Footprint: 4x4 tiles (largest single structure)
- [ ] Cost: 500,000 credits
- [ ] Maintenance: 0 credits/cycle (free to maintain)
- [ ] ONE per player maximum (is_unique_per_player = true)
- [ ] Required_unlock: TranscendenceMonument (150K milestone)
- [ ] Effect: +25% harmony in 30-tile radius
- [ ] Effect: Catastrophe immunity in 20-tile radius
- [ ] Effect: Contamination immunity in 25-tile radius
- [ ] Effect: +50 sector_value in 15-tile radius
- [ ] NOT repeatable (one per player forever)

**Dependencies:**
- Blocked by: 14-H01, 14-A06
- Blocks: 14-H03

**Canon References:**
- Game Designer Answers Q3: one per player, not repeatable
- terminology.yaml: (add transcendence_monument -> transcendence_marker)

---

### Ticket 14-H03: Implement Transcendence Monument Area Effects

**Type:** feature
**Priority:** P1 (should have)
**System:** Multiple (Population, Disaster, Contamination, LandValue)
**Size:** L

**Description:**
Implement the area effects of the Transcendence Monument using pre-computed tile sets for efficient queries.

**Acceptance Criteria:**
- [ ] Pre-compute affected tile sets on monument placement
- [ ] Harmony: +25% to all tiles in 30-tile radius
- [ ] Catastrophe: Complete immunity (0 damage) in 20-tile radius
- [ ] Contamination: Cannot exist/generate in 25-tile radius
- [ ] Sector value: +50 to tiles in 15-tile radius
- [ ] Use spatial hash sets for O(1) tile lookup
- [ ] Effects do NOT stack if multiple monuments exist (cap, don't multiply)
- [ ] Effects cease immediately on monument destruction

**Dependencies:**
- Blocked by: 14-H02, 14-A06
- Blocks: 14-L05

**Canon References:**
- Systems Architect Answers Q10: pre-computed tile sets, spatial hashing

---

### Ticket 14-H04: Implement Transcendence Construction Celebration

**Type:** feature
**Priority:** P2 (nice to have)
**System:** UISystem
**Size:** M

**Description:**
Implement the special celebration event when a Transcendence Monument is constructed.

**Acceptance Criteria:**
- [ ] 15-second spectacular visual celebration
- [ ] Aurora effects, particle effects, reality shimmer
- [ ] Grand orchestral achievement audio
- [ ] Server-wide broadcast: "Overseer [Name] has achieved Transcendence!"
- [ ] Permanent aurora effect in sky above monument (visible server-wide)
- [ ] All players receive notification regardless of distance

**Dependencies:**
- Blocked by: 14-H02
- Blocks: None

**Canon References:**
- Game Designer Answers Q3: colony-wide celebration event

---

## Group I: BuildingSystem Integration

### Ticket 14-I01: Add required_unlock Field to BuildingTemplate

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** S

**Description:**
Extend BuildingTemplate structure to include progression prerequisite field.

**Acceptance Criteria:**
- [ ] Add std::optional<RewardBuildingType> required_unlock field
- [ ] Field is nullopt for standard buildings (no prerequisite)
- [ ] Field set for reward buildings and arcologies
- [ ] Add bool is_unique_per_player field (default false)
- [ ] Update template serialization to include new fields

**Dependencies:**
- Blocked by: 14-A03
- Blocks: 14-I02

**Canon References:**
- Systems Architect Analysis Section 5.2: template modification

---

### Ticket 14-I02: Implement Prerequisite Check in BuildingSystem

**Type:** feature
**Priority:** P0 (must have)
**System:** BuildingSystem
**Size:** M

**Description:**
Modify BuildingSystem.can_build_template() to include progression prerequisite check via IUnlockPrerequisite interface.

**Acceptance Criteria:**
- [ ] Add IUnlockPrerequisite* unlock_checker_ member
- [ ] Add set_unlock_checker(IUnlockPrerequisite*) setter
- [ ] Modify can_build_template() to call meets_prerequisites()
- [ ] Check is skipped if unlock_checker_ is null (backward compatibility)
- [ ] Check unique building limit (is_unique_per_player)
- [ ] Return false with reason if prerequisite not met
- [ ] Integrate with existing terrain, energy, fluid checks

**Dependencies:**
- Blocked by: 14-I01, 14-B02, 14-B03
- Blocks: 14-L02

**Canon References:**
- Systems Architect Analysis Section 5.2: integration pattern

---

### Ticket 14-I03: Integrate Arcology Self-Sufficiency with EnergySystem

**Type:** feature
**Priority:** P1 (should have)
**System:** EnergySystem
**Size:** S

**Description:**
Modify EnergySystem to apply reduced demand multiplier for arcology buildings based on their self-sufficiency rating.

**Acceptance Criteria:**
- [ ] Check if habitation entity has ArcologyComponent
- [ ] Apply get_external_energy_demand_multiplier() to demand calculation
- [ ] Plymouth/Forest/Darco: 50% reduced demand (0.5 multiplier)
- [ ] Launch: 60% reduced demand (0.4 multiplier = -40% consumption)
- [ ] Normal buildings unaffected (no ArcologyComponent)

**Dependencies:**
- Blocked by: 14-A04
- Blocks: 14-L04

**Canon References:**
- Systems Architect Answers Q8: reduced consumption model (50% of normal)

---

### Ticket 14-I04: Integrate Arcology Self-Sufficiency with FluidSystem

**Type:** feature
**Priority:** P1 (should have)
**System:** FluidSystem
**Size:** S

**Description:**
Modify FluidSystem to apply reduced demand multiplier for arcology buildings based on their self-sufficiency rating.

**Acceptance Criteria:**
- [ ] Check if habitation entity has ArcologyComponent
- [ ] Apply get_external_fluid_demand_multiplier() to demand calculation
- [ ] All arcology types: 50% reduced fluid demand (0.5 multiplier)
- [ ] Normal buildings unaffected (no ArcologyComponent)

**Dependencies:**
- Blocked by: 14-A04
- Blocks: 14-L04

**Canon References:**
- Systems Architect Answers Q8: reduced consumption model

---

## Group J: UI Integration

### Ticket 14-J01: Implement Milestone Notification Handler

**Type:** feature
**Priority:** P0 (must have)
**System:** UISystem
**Size:** M

**Description:**
Implement UI handling for milestone achievement events including visual celebration, audio, and notifications.

**Acceptance Criteria:**
- [ ] Subscribe to MilestoneAchievedEvent
- [ ] Display celebration visual effects (scaled by milestone tier)
- [ ] Play milestone-specific audio cue
- [ ] Show notification: "Your colony has reached X beings!"
- [ ] Show unlock alert: "Y is now available to construct."
- [ ] Handle other players' milestones (reduced notification)
- [ ] Duration scales with milestone tier (3-12 seconds)
- [ ] Non-intrusive design (doesn't block gameplay)

**Dependencies:**
- Blocked by: 14-D01, 14-C02
- Blocks: 14-L03

**Canon References:**
- Game Designer Analysis Section 1.3: celebration design per milestone
- terminology.yaml: notification -> alert_pulse

---

### Ticket 14-J02: Implement Unlocked Rewards Display

**Type:** feature
**Priority:** P0 (must have)
**System:** UISystem
**Size:** M

**Description:**
Implement UI display for available reward buildings in the build menu.

**Acceptance Criteria:**
- [ ] Query IProgressionProvider.get_unlocked_rewards(player)
- [ ] Display unlocked reward buildings in dedicated section of build menu
- [ ] Gray out/hide locked reward buildings
- [ ] Show unlock requirement tooltip for locked buildings
- [ ] Show "NEW" indicator when newly unlocked
- [ ] Unique buildings: show "BUILT" if player already owns one

**Dependencies:**
- Blocked by: 14-D02
- Blocks: 14-L03

**Canon References:**
- Game Designer Analysis Section 2: reward building experience

---

### Ticket 14-J03: Implement Edict Management Panel

**Type:** feature
**Priority:** P0 (must have)
**System:** UISystem
**Size:** L

**Description:**
Implement the UI panel for viewing and managing active edicts. Accessed via Command Nexus building.

**Acceptance Criteria:**
- [ ] Panel accessible when Command Nexus is queried/selected
- [ ] Display all 7 available edicts with descriptions
- [ ] Show which edicts are currently active (max 4)
- [ ] Show available slots count
- [ ] Allow activation/deactivation with confirmation
- [ ] Show cooldown timer if recently changed (10-cycle cooldown)
- [ ] Display effect magnitudes (e.g., "+10% harmony")
- [ ] Gray out unavailable edicts (at slot limit)
- [ ] Query other players' Command Nexus to see their edicts

**Dependencies:**
- Blocked by: 14-E01, 14-G02
- Blocks: 14-L03

**Canon References:**
- Game Designer Analysis Section 5.4: edict selection experience

---

### Ticket 14-J04: Implement Monument Customization UI

**Type:** feature
**Priority:** P1 (should have)
**System:** UISystem
**Size:** M

**Description:**
Implement the customization UI for Monument (Eternity Marker) placement.

**Acceptance Criteria:**
- [ ] Modal opens on Monument construction command
- [ ] Display 4 style variants with visual preview
- [ ] Display 6 color themes with visual preview
- [ ] Text field for custom name (32 character limit)
- [ ] Preview updates in real-time as options change
- [ ] Confirm button finalizes customization
- [ ] Style and color are permanent after placement
- [ ] Name can be changed later via query tool

**Dependencies:**
- Blocked by: 14-G03
- Blocks: None

**Canon References:**
- Game Designer Answers Q5: 4 styles, 6 colors, custom name

---

### Ticket 14-J05: Implement Progression Overview Panel

**Type:** feature
**Priority:** P1 (should have)
**System:** UISystem
**Size:** M

**Description:**
Implement a progression overview panel showing milestones, progress, and statistics.

**Acceptance Criteria:**
- [ ] Display all 7 milestones with achieved/locked status
- [ ] Show current population vs next milestone threshold
- [ ] Progress bar visualization
- [ ] Display peak population ever reached
- [ ] Show arcology count and capacity
- [ ] Show active edicts summary
- [ ] List reward buildings owned
- [ ] Accessible from main menu or status bar

**Dependencies:**
- Blocked by: 14-D02, 14-F01
- Blocks: None

**Canon References:**
- Game Designer Analysis Section 9: player journey

---

## Group K: Network & Serialization

### Ticket 14-K01: Implement Progression Component Serialization

**Type:** feature
**Priority:** P0 (must have)
**System:** NetworkSystem
**Size:** M

**Description:**
Implement ISerializable for all progression components to support save/load and network sync.

**Acceptance Criteria:**
- [ ] MilestoneComponent serialization (serialize/deserialize)
- [ ] EdictComponent serialization (serialize/deserialize)
- [ ] RewardBuildingComponent serialization (serialize/deserialize)
- [ ] ArcologyComponent serialization (serialize/deserialize)
- [ ] TranscendenceAuraComponent serialization (serialize/deserialize)
- [ ] Backward compatibility for future field additions
- [ ] Delta serialization support for network updates

**Dependencies:**
- Blocked by: 14-A02, 14-A03, 14-A04, 14-A05, 14-A06
- Blocks: 14-K02, 14-K03

**Canon References:**
- patterns.yaml: ISerializable pattern from Epic 1

---

### Ticket 14-K02: Implement Edict Change Request Message

**Type:** feature
**Priority:** P0 (must have)
**System:** NetworkSystem
**Size:** S

**Description:**
Implement the client-to-server message for requesting edict activation/deactivation.

**Acceptance Criteria:**
- [ ] EdictChangeRequest message: PlayerID, EdictType, bool activate
- [ ] Client sends on UI interaction
- [ ] Server validates and processes
- [ ] Server rejects if: no Command Nexus, in cooldown, no slots
- [ ] Response indicates success/failure with reason
- [ ] Anti-cheat: validate player owns the request

**Dependencies:**
- Blocked by: 14-K01
- Blocks: 14-L04

**Canon References:**
- Systems Architect Answers Q6: edict change flow

---

### Ticket 14-K03: Implement Milestone Broadcast Message

**Type:** feature
**Priority:** P0 (must have)
**System:** NetworkSystem
**Size:** S

**Description:**
Implement the server-to-all-clients broadcast message for milestone achievements.

**Acceptance Criteria:**
- [ ] MilestoneNotificationMessage: PlayerID, MilestoneType, population, server_tick, player_name
- [ ] Server broadcasts on MilestoneAchievedEvent
- [ ] All connected clients receive regardless of distance
- [ ] Latency tolerance: 100-300ms for celebration sync
- [ ] Include server_tick for interpolation if needed

**Dependencies:**
- Blocked by: 14-K01, 14-C02
- Blocks: 14-L03

**Canon References:**
- Systems Architect Answers Q1: 100-300ms latency tolerance

---

## Group L: Testing

### Ticket 14-L01: Unit Tests - Component Definitions

**Type:** task
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** S

**Description:**
Create unit tests for all progression component definitions.

**Acceptance Criteria:**
- [ ] Test MilestoneComponent bitmask operations
- [ ] Test has_milestone() and set_milestone() methods
- [ ] Test milestone permanence (set but never clear)
- [ ] Test EdictComponent bitmask operations
- [ ] Test ArcologyComponent self-sufficiency calculations
- [ ] Test component sizes match expectations

**Dependencies:**
- Blocked by: 14-A02, 14-A03, 14-A04, 14-A05
- Blocks: None

**Canon References:**
- patterns.yaml: unit test pattern

---

### Ticket 14-L02: Unit Tests - Milestone Detection

**Type:** task
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Create unit tests for milestone detection logic.

**Acceptance Criteria:**
- [ ] Test all 7 milestone thresholds trigger correctly
- [ ] Test milestone event emission
- [ ] Test highest_population_ever tracking
- [ ] Test milestone permanence (population drop doesn't revoke)
- [ ] Test arcology population counts toward milestones
- [ ] Test multiple players track independently

**Dependencies:**
- Blocked by: 14-D01
- Blocks: None

**Canon References:**
- Systems Architect Answers Q2, Q3: counting and permanence

---

### Ticket 14-L03: Unit Tests - Unlock Derivation

**Type:** task
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Create unit tests for unlock state derivation from milestones.

**Acceptance Criteria:**
- [ ] Test each milestone unlocks correct reward building
- [ ] Test Arcology_120K unlocks all 4 arcology types
- [ ] Test get_unlocked_rewards() returns correct list
- [ ] Test is_reward_building_unlocked() accuracy
- [ ] Test IUnlockPrerequisite.meets_prerequisites()

**Dependencies:**
- Blocked by: 14-D02, 14-I02
- Blocks: None

**Canon References:**
- patterns.yaml: unlock derivation tests

---

### Ticket 14-L04: Unit Tests - Edict Effects

**Type:** task
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Create unit tests for edict activation, deactivation, and effect calculations.

**Acceptance Criteria:**
- [ ] Test edict activation with available slot
- [ ] Test edict activation rejected when no slots
- [ ] Test edict activation rejected during cooldown
- [ ] Test all 7 edict modifier values
- [ ] Test edict slot progression (1/2/3/4 based on milestones)
- [ ] Test edicts cease when Command Nexus destroyed
- [ ] Test multiplayer: edicts per-player independent

**Dependencies:**
- Blocked by: 14-E01, 14-E02, 14-E03
- Blocks: None

**Canon References:**
- Game Designer Answers Q1: edict specifications

---

### Ticket 14-L05: Integration Tests - Full Progression Flow

**Type:** task
**Priority:** P0 (must have)
**System:** ProgressionSystem
**Size:** L

**Description:**
Create integration tests for complete progression flows from milestone to building.

**Acceptance Criteria:**
- [ ] Test: 0 -> 2K population -> Sanctum_2K milestone -> Overseer's Sanctum buildable -> Build
- [ ] Test: 10K milestone -> Command Nexus buildable -> Build -> Edicts available
- [ ] Test: Edict activation -> Effect applied in target system
- [ ] Test: 120K milestone -> All arcology types unlockable -> Build arcology
- [ ] Test: 150K milestone -> Transcendence Monument unlockable
- [ ] Test: Full network round-trip for edict changes
- [ ] Test: Full network broadcast for milestones
- [ ] Test: Save/load preserves all progression state

**Dependencies:**
- Blocked by: 14-H03, 14-K03
- Blocks: None

**Canon References:**
- patterns.yaml: integration test pattern

---

### Ticket 14-L06: Unit Tests - Arcology System

**Type:** task
**Priority:** P1 (should have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Create unit tests for arcology tracking, limits, and capacity calculations.

**Acceptance Criteria:**
- [ ] Test arcology count tracking on construction/destruction
- [ ] Test max arcology limit enforcement (4 per player)
- [ ] Test arcology capacity calculation and caching
- [ ] Test cache invalidation on arcology events
- [ ] Test different arcology type capacities
- [ ] Test arcology self-sufficiency multipliers

**Dependencies:**
- Blocked by: 14-F01, 14-F02, 14-F03
- Blocks: None

**Canon References:**
- Systems Architect Answers Q7: caching and calculation

---

### Ticket 14-L07: Unit Tests - Transcendence Monument Effects

**Type:** task
**Priority:** P1 (should have)
**System:** ProgressionSystem
**Size:** M

**Description:**
Create unit tests for Transcendence Monument area effects.

**Acceptance Criteria:**
- [ ] Test pre-computed tile set generation for all radii
- [ ] Test harmony bonus applied to tiles in 30-tile radius
- [ ] Test catastrophe immunity in 20-tile radius
- [ ] Test contamination immunity in 25-tile radius
- [ ] Test sector value bonus in 15-tile radius
- [ ] Test effects do NOT stack with multiple monuments
- [ ] Test effects cease on monument destruction

**Dependencies:**
- Blocked by: 14-H03
- Blocks: None

**Canon References:**
- Systems Architect Answers Q10: effect implementation

---

## Dependency Graph - Critical Path

```
                         14-A01 (Enums)
                              |
        +---------------------+---------------------+
        |           |         |         |           |
    14-A02      14-A03    14-A04    14-A05      14-A06
  (Milestone)  (Reward)  (Arcology) (Edict)    (Aura)
        |           |         |         |           |
        +-----+-----+         |    +----+           |
              |               |    |                |
          14-B01          14-F01   |            14-H02
     (IProgressionProvider)  |     |    (Transcendence Template)
              |               |    |                |
         14-B02           14-F02   |            14-H03
    (IUnlockPrerequisite)    |     |       (Area Effects)
              |               |    |
         14-B03           14-F03   |
        (Stub)               |     |
              |               |    |
              +-------+-------+    |
                      |            |
                  14-C01           |
            (ProgressionSystem)    |
                      |            |
          +-----------+------------+
          |           |            |
      14-D01      14-E01        14-F04
   (Milestone    (Edict      (Arcology
    Detection) Activation)  Templates)
          |           |
      14-D02      14-E02
     (Unlock     (Edict
    Derivation) Modifiers)
          |           |
      14-D03      14-E03
    (Edict      (Query
     Slots)   Integration)
          |
      14-I02
   (BuildingSystem
    Integration)
          |
    +-----+-----+
    |           |
14-G01-05   14-H01
(Reward    (Transcendence
Templates)   Unlock)
    |           |
14-J01-05   14-H02
   (UI)    (Template)
    |           |
14-K01-03   14-H03
(Network)  (Effects)
    |
14-L01-07
(Tests)
```

### Critical Path (Minimum Viable Implementation)

1. **14-A01** Enums
2. **14-A02** MilestoneComponent
3. **14-A05** EdictComponent
4. **14-B01** IProgressionProvider
5. **14-B02** IUnlockPrerequisite
6. **14-C01** ProgressionSystem
7. **14-D01** Milestone Detection
8. **14-D02** Unlock Derivation
9. **14-I02** BuildingSystem Integration
10. **14-G01-02** Core Reward Templates (Sanctum, Nexus)
11. **14-J01-02** Core UI (Notifications, Build Menu)
12. **14-K01** Serialization
13. **14-L01-03** Core Tests

---

## Canon Updates Required

### terminology.yaml Additions

```yaml
progression:
  edict: edict                              # Policy/decree system
  edict_slot: guidance_channel              # Edict capacity slot
  transcendence: transcendence              # Ultimate achievement state
  transcendence_monument: transcendence_marker  # 150K reward building

  # Arcology types (new)
  plymouth_arcology: plymouth_arcology
  forest_arcology: forest_arcology
  darco_arcology: darco_arcology
  launch_arcology: launch_arcology

  # Milestone names
  colony_emergence: colony_emergence        # 2K threshold
  colony_establishment: colony_establishment # 10K threshold
  colony_identity: colony_identity          # 30K threshold
  colony_security: colony_security          # 60K threshold
  colony_wonder: colony_wonder              # 90K threshold
  colony_ascension: colony_ascension        # 120K threshold
  colony_transcendence: colony_transcendence # 150K threshold

buildings:
  eternity_marker: eternity_marker          # Monument (30K)
  aegis_complex: aegis_complex              # Defense Installation (60K)
  resonance_spire: resonance_spire          # Special Landmark (90K)
```

### systems.yaml Additions

```yaml
epic_14_progression:
  name: "Progression & Rewards"
  systems:
    ProgressionSystem:
      type: ecs_system
      tick_priority: 90
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
        - "IUnlockPrerequisite: building prerequisite checks"
        - Milestone event emission
      depends_on:
        - PopulationSystem (population queries)
        - BuildingSystem (arcology count queries)
      multiplayer:
        authority: server
        per_player: true
      components:
        - MilestoneComponent
        - RewardBuildingComponent
        - ArcologyComponent
        - EdictComponent
        - TranscendenceAuraComponent
```

### interfaces.yaml Additions

```yaml
progression:
  IProgressionProvider:
    description: "Query progression state for milestone and unlock checks"
    implemented_by:
      - ProgressionSystem (Epic 14)
    methods:
      - has_milestone
      - get_highest_milestone
      - is_reward_building_unlocked
      - is_edict_active
      - get_edict_modifier
      - get_total_arcology_capacity

  IUnlockPrerequisite:
    description: "Check if building construction prerequisites are met"
    implemented_by:
      - ProgressionSystem (Epic 14)
      - StubUnlockPrerequisite (pre-Epic 14)
    methods:
      - meets_prerequisites
      - get_prerequisite_failure_reason
```

### patterns.yaml Additions

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

transcendence_area_effects:
  description: "Large radius area effects using pre-computed tile sets"
  pattern: "Pre-compute affected tiles on placement, use spatial hash for O(1) lookup"
  applies_to:
    - TranscendenceAuraComponent
  performance:
    computation: "O(r^2) one-time on placement"
    lookup: "O(1) per tile check"
    memory: "~150 KB per monument for all radii"
```

---

## Ticket Summary

| Group | Description | Ticket Count |
|-------|-------------|--------------|
| A | Component Infrastructure | 6 |
| B | Interfaces | 3 |
| C | Core ProgressionSystem | 2 |
| D | Milestone System | 3 |
| E | Edict System | 3 |
| F | Arcology System | 4 |
| G | Reward Building Templates | 5 |
| H | Transcendence Monument | 4 |
| I | BuildingSystem Integration | 4 |
| J | UI Integration | 5 |
| K | Network & Serialization | 3 |
| L | Testing | 7 |
| **Total** | | **52** |

### Size Distribution

| Size | Count | Estimated Effort |
|------|-------|------------------|
| S (Small) | 18 | 1-2 hours each |
| M (Medium) | 27 | 2-4 hours each |
| L (Large) | 7 | 4-8 hours each |

### Priority Distribution

| Priority | Count | Description |
|----------|-------|-------------|
| P0 | 36 | Must have for MVP |
| P1 | 13 | Should have for complete feature |
| P2 | 3 | Nice to have enhancements |

---

**End of Epic 14 Ticket Breakdown**
