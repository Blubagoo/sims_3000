# Game Designer Answers: Epic 14 Systems Architect Questions

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Status:** Answers for Implementation

---

## Q1: Edict Passive Bonuses

**Question:** What specific passive bonuses should each Edict provide?

### Design Decision

I approve your proposed edict values with minor adjustments. Here are the final specifications:

| Edict | Effect | Modifier Value | Implementation Notes |
|-------|--------|----------------|---------------------|
| **ColonyHarmony** | +harmony baseline | 1.10 (multiply by 1.10) | Applied in PopulationSystem harmony calculation |
| **GrowthIncentive** | +migration in-rate | 1.15 (multiply by 1.15) | Applied in PopulationSystem migration logic |
| **TributeForgiveness** | -tribute collected, +demand | 0.90 tribute, 1.05 demand | Two multipliers in EconomySystem |
| **TradeAgreement** | +port income | 1.20 (multiply by 1.20) | Applied in PortSystem income calculation |
| **EnforcerMandate** | -disorder generation | 0.90 (multiply by 0.90) | Applied in DisorderSystem generation |
| **HazardPreparedness** | -disaster damage | 0.80 (multiply by 0.80) | Applied in DisasterSystem damage calculation |
| **GreenInitiative** | -contamination generation | 0.85 (multiply by 0.85) | Applied in ContaminationSystem generation |

### Edict Slot Progression

**Decision:** Use milestone-based slot progression, not population-based increments.

| Milestone Achieved | Edict Slots Available |
|-------------------|----------------------|
| 10K (Command Nexus built) | 1 slot |
| 30K (Monument unlocked) | 2 slots |
| 60K (Defense unlocked) | 3 slots |
| 90K (Special Landmark unlocked) | 4 slots |

**Max edicts: 4** (not 5 as proposed - keeps the system simpler and makes each slot feel more valuable)

### Additional Edict Rules

1. **Cooldown:** 10-cycle cooldown when changing edicts (prevents micro-optimization)
2. **Command Nexus Required:** Edicts only function while at least one Command Nexus exists
3. **Single Instance:** Same edict cannot be selected twice (no stacking)
4. **Multiplayer:** Each player has independent edict selection

### Rationale

- Effects are intentionally mild (10-20% range) to keep them optional
- No edict should feel mandatory for progression
- Effects should be noticeable but not game-changing
- Multiplier approach makes implementation straightforward

---

## Q2: Arcology Visual Styles

**Question:** Should arcologies have different visual styles and gameplay effects?

### Design Decision

**Yes - both visual styles AND gameplay effects.** Arcologies should feel like meaningful choices, not just capacity numbers.

### Final Arcology Specifications

| Arcology | Capacity | Build Cost | Maintenance | Visual Theme | Gameplay Effect |
|----------|----------|------------|-------------|--------------|-----------------|
| **Plymouth** | 65,000 | 200,000 cr | 800 cr/cycle | Classic dome, balanced aesthetics | Baseline - no special effect |
| **Forest** | 55,000 | 180,000 cr | 700 cr/cycle | Organic, bioluminescent vegetation | +15% harmony in 15-tile radius |
| **Darco** | 75,000 | 220,000 cr | 900 cr/cycle | Industrial, angular, fabrication-focused | +10% fabrication jobs in 10-tile radius |
| **Launch** | 50,000 | 250,000 cr | 600 cr/cycle | Sleek, vertical, space-age | -40% energy consumption (internal generation) |

### Implementation Parameters

```cpp
struct ArcologyTemplate {
    ArcologyType type;
    uint32_t base_population_capacity;
    uint32_t build_cost;
    uint32_t maintenance_per_cycle;

    // Gameplay effects
    float harmony_radius_bonus;      // Forest: 0.15, others: 0.0
    uint8_t harmony_effect_radius;   // Forest: 15, others: 0
    float fabrication_job_bonus;     // Darco: 0.10, others: 0.0
    uint8_t fabrication_effect_radius; // Darco: 10, others: 0
    float energy_consumption_mult;   // Launch: 0.6, others: 1.0
};
```

### Arcology Effect Integration

| Arcology | Target System | Query Pattern |
|----------|---------------|---------------|
| Forest | PopulationSystem | `get_arcology_harmony_bonus(tile_x, tile_y)` |
| Darco | EconomySystem | `get_arcology_fabrication_bonus(tile_x, tile_y)` |
| Launch | EnergySystem | `get_arcology_energy_multiplier(building_id)` |

### Visual Style Notes

- **Plymouth:** SimCity 2000 reference - rounded dome, neutral colors, blue accent lighting
- **Forest:** Grown from the ground, organic curves, green bioluminescence, visible vegetation
- **Darco:** Sharp angles, industrial orange/amber lighting, visible smoke/steam effects
- **Launch:** Streamlined vertical design, white/silver with thruster glow effects at base

### Unlock Progression

**All arcology types unlock simultaneously at 120K population.** No additional unlocks at 150K.

**Rationale for single unlock:**
- Avoids "waiting for the good arcology" frustration
- Player choice becomes about playstyle, not availability
- Simplifies progression system

### Rationale for Gameplay Effects

- Effects are localized (radius-based) to encourage strategic placement
- Effects are thematic (Forest = harmony, Darco = industry, Launch = energy)
- Plymouth serves as the "safe default" choice
- Different arcologies suit different colony designs

---

## Q3: Transcendence Monument Mechanics

**Question:** How should the Transcendence Monument work exactly?

### Design Decision

**Unlock Threshold:** 150,000 total population (simplified from arcology capacity requirement)

**Rationale for population-based unlock:**
- Arcology capacity tracking adds complexity
- 150K population is already a significant achievement
- Matches the milestone progression pattern
- More accessible than "500K arcology capacity" (which would require 8+ fully-populated arcologies)

### Transcendence Monument Specifications

| Attribute | Value | Notes |
|-----------|-------|-------|
| **Unlock** | 150,000 beings total | Same as Transcendence_150K milestone |
| **Build Cost** | 500,000 credits | Very high - ultimate prestige building |
| **Footprint** | 4x4 tiles | Largest single structure |
| **Maintenance** | 0 credits | Free to maintain once built |
| **Limit** | One per player | Cannot spam prestige buildings |

### Functional Effects

| Effect | Value | Implementation |
|--------|-------|----------------|
| Harmony boost | +25% in 30-tile radius | PopulationSystem queries for nearby monuments |
| Catastrophe immunity | No damage in 20-tile radius | DisasterSystem checks monument proximity |
| Contamination immunity | Cannot generate in 25-tile radius | ContaminationSystem checks monument proximity |
| Sector value boost | +50 to tiles in 15-tile radius | LandValueSystem queries |

### Implementation Parameters

```cpp
struct TranscendenceMonumentEffect {
    uint8_t harmony_radius = 30;
    float harmony_bonus = 1.25f;

    uint8_t catastrophe_immunity_radius = 20;

    uint8_t contamination_immunity_radius = 25;

    uint8_t sector_value_radius = 15;
    int16_t sector_value_bonus = 50;
};
```

### Special Event on Construction

**Yes - trigger a colony-wide celebration event:**

1. **Visual:** 15-second spectacular light show (aurora, particle effects, reality shimmer)
2. **Audio:** Grand orchestral achievement fanfare
3. **Server Broadcast:** All players receive notification: "Overseer [Name] has achieved Transcendence!"
4. **Persistent Effect:** Permanent aurora effect in sky above monument (visible server-wide)

### Repeatability Decision

**No - one Transcendence Monument per player maximum.**

**Rationale:**
- Maintains the "ultimate achievement" feeling
- Prevents late-game monument spam
- One permanent marker is more meaningful than many
- Simplifies implementation (no scaling thresholds)

If extreme late-game content is needed in future, create new achievement types rather than repeatable monuments.

---

## Q4: Defense Installation Specifics

**Question:** How does the Defense Installation provide disaster mitigation?

### Design Decision

**Single Defense Installation per player with significant localized protection.**

### Defense Installation Specifications

| Attribute | Value | Notes |
|-----------|-------|-------|
| **Coverage Radius** | 25 tiles | Covers a significant portion of colony |
| **Damage Reduction** | -30% disaster damage | Applied to all structures in radius |
| **Stacking** | No stacking | Multiple installations don't combine |
| **Limit** | One per player | Prestige building, not infrastructure |
| **Footprint** | 3x3 tiles | Medium-large building |
| **Cost** | 100,000 credits | Significant investment |
| **Maintenance** | 500 credits/cycle | Moderate upkeep |

### Disaster Types Affected

| Disaster Type | Mitigation Effect |
|---------------|-------------------|
| Fire catastrophe | -30% spread rate, -30% structure damage |
| Seismic event | -30% structural damage |
| Storm/Vortex | -30% structural damage |
| Contamination surge | -30% contamination level increase |
| All other types | -30% damage/effect |

### Implementation Pattern

```cpp
// In DisasterSystem when calculating damage
float get_disaster_damage(uint16_t tile_x, uint16_t tile_y, float base_damage) {
    float damage = base_damage;

    // Check for Defense Installation coverage
    if (progression_->is_in_defense_coverage(owner_at(tile_x, tile_y), tile_x, tile_y)) {
        damage *= 0.70f;  // 30% reduction
    }

    // Check for HazardPreparedness edict (stacks multiplicatively)
    if (progression_->is_edict_active(owner, EdictType::HazardPreparedness)) {
        damage *= 0.80f;  // Additional 20% reduction
    }

    return damage;
}
```

### Variant System Decision

**No variants for Defense Installation.**

**Rationale:**
- Simplifies implementation
- One-per-player limit makes variants less meaningful
- Visual customization can be achieved through other means
- Keeps the "defense" fantasy clear (not specialized per disaster type)

### Additional Benefits

| Benefit | Value | Target System |
|---------|-------|---------------|
| Response time bonus | +20% service speed in radius | ServiceSystem |
| Emergency vehicle priority | -30% response delay | HazardResponseService |
| Visual presence | Patrol animations, warning lights | Visual only |

### Rationale

- 25-tile radius provides meaningful coverage without trivializing disasters
- 30% reduction is significant but doesn't make disasters ignorable
- Single installation per player maintains prestige value
- Edict stacking (HazardPreparedness) allows players to build a "disaster-resistant" colony identity

---

## Q5: Monument Customization

**Question:** What customization options should the Monument (30K reward) offer?

### Design Decision

**Limited but meaningful customization focused on visual identity.**

### Customization Options

#### Style Variants (4 options)

| Style | Visual Description | Thematic Feel |
|-------|-------------------|---------------|
| **Obelisk** | Tall crystalline spire, sharp edges | Achievement, ambition |
| **Spiral** | Organic double-helix tower, flowing | Growth, evolution |
| **Bloom** | Flower-like structure, petals open | Life, creativity |
| **Nexus** | Geometric floating elements, abstract | Mystery, technology |

#### Color Themes (6 options)

| Theme | Primary Color | Accent | Glow |
|-------|---------------|--------|------|
| **Aurora** | Teal | Pink | Multi-shift |
| **Solar** | Gold | Orange | Warm pulse |
| **Void** | Deep purple | Silver | Cool pulse |
| **Life** | Green | Blue | Organic pulse |
| **Fire** | Amber | Red | Flickering |
| **Crystal** | White | Cyan | Steady glow |

#### Custom Name

- **Character Limit:** 32 characters
- **Display:** Shown when monument is queried by any player
- **Default:** "Monument of [PlayerName]"
- **Profanity Filter:** Yes, basic filter applied

### Implementation Data

```cpp
struct MonumentCustomization {
    uint8_t style_variant;    // 0-3 (Obelisk, Spiral, Bloom, Nexus)
    uint8_t color_theme;      // 0-5 (Aurora, Solar, Void, Life, Fire, Crystal)
    char custom_name[33];     // 32 chars + null terminator
};
```

### Customization Experience

1. **On Construction:** Full customization UI opens (modal)
2. **Preview:** Player can preview all style/color combinations before confirming
3. **Finalization:** Once placed, style and color are permanent
4. **Name Change:** Name can be changed at any time (via query tool on monument)

### No Scale Customization

**Decision:** Fixed 2x2 footprint, fixed height range per style.

**Rationale:**
- Scale affects gameplay (sector value radius, visibility)
- Variable scale complicates collision and rendering
- Players express through style/color, not size

### Rationale for Limited Options

- 4 styles x 6 colors = 24 unique combinations (enough variety)
- Named monuments create personal connection
- Style choice reflects player identity
- Color choice harmonizes with colony aesthetic

---

## Q6: Overseer's Sanctum and Special Landmark Effects

**Question:** Do these buildings have gameplay effects or are they purely prestige?

### Design Decision

**Both have mild gameplay effects in addition to prestige value.**

### Overseer's Sanctum (2K unlock)

| Effect Type | Value | Implementation |
|-------------|-------|----------------|
| **Harmony bonus** | +5% colony-wide | Global multiplier in PopulationSystem |
| **Sector value boost** | +15 to adjacent tiles (8 tiles) | LandValueSystem queries |
| **Visual prestige** | Unique "overseer lives here" marker | Visual only |

**Rationale for mild effects:**
- First reward building - should feel meaningful but not OP
- Colony-wide harmony bonus rewards reaching 2K milestone
- Sector value boost encourages thoughtful placement

### Special Landmark / Resonance Spire (90K unlock)

| Effect Type | Value | Implementation |
|-------------|-------|----------------|
| **Harmony bonus** | +15% in 20-tile radius | Radius query in PopulationSystem |
| **Sector value boost** | +30 to tiles in 15-tile radius | LandValueSystem queries |
| **Exchange demand** | +20% in 25-tile radius | EconomySystem queries |
| **Contamination reduction** | -10% generation in 15-tile radius | ContaminationSystem queries |
| **Server visibility** | Visible beacon from any player's view | Visual/network |

**Rationale for stronger effects:**
- 90K is a major achievement - reward should feel significant
- Radius-based effects encourage central/strategic placement
- Multiple effect types make it feel like a "wonder"

### Implementation Parameters

```cpp
// Overseer's Sanctum
struct SanctumEffect {
    float colony_harmony_bonus = 1.05f;  // +5%
    int16_t adjacent_sector_value = 15;
};

// Resonance Spire
struct SpireEffect {
    uint8_t harmony_radius = 20;
    float harmony_bonus = 1.15f;  // +15%

    uint8_t sector_value_radius = 15;
    int16_t sector_value_bonus = 30;

    uint8_t exchange_radius = 25;
    float exchange_demand_bonus = 1.20f;  // +20%

    uint8_t contamination_radius = 15;
    float contamination_reduction = 0.90f;  // -10%
};
```

### Effect Priority Pattern

For both buildings, effects should:
1. Apply passively each tick (no player action required)
2. Be additive with other bonuses (not multiplicative chains)
3. Require the building to be powered/functional
4. Cease immediately if building is destroyed

### Query Interface Addition

```cpp
// Addition to IProgressionProvider
virtual float get_sanctum_harmony_bonus(PlayerID player) const = 0;
virtual float get_spire_harmony_bonus(uint16_t tile_x, uint16_t tile_y) const = 0;
virtual float get_spire_exchange_bonus(uint16_t tile_x, uint16_t tile_y) const = 0;
virtual float get_spire_contamination_mult(uint16_t tile_x, uint16_t tile_y) const = 0;
```

### Rationale

**Why not purely prestige?**
- Reward buildings should reward - pure cosmetics feel hollow
- Effects encourage thoughtful placement decisions
- Mild effects maintain "optional" nature
- Creates synergies with colony layout

**Why not stronger effects?**
- Reward buildings shouldn't become required infrastructure
- Core Principle: "A player can reach 150K without building ANY reward structures"
- Effects are "nice to have" bonuses, not mandatory mechanics

---

## Summary Table

| Question | Key Decision | Primary Value |
|----------|--------------|---------------|
| Q1: Edicts | 7 edicts, 4 max slots, 10-20% effects | Multiplier values defined |
| Q2: Arcologies | 4 types with distinct gameplay effects | Capacity + unique bonuses |
| Q3: Transcendence | 150K unlock, 500K cost, one per player | 4x4 with powerful aura |
| Q4: Defense | 25-tile radius, -30% damage, one per player | Universal disaster mitigation |
| Q5: Monument | 4 styles, 6 colors, custom name | Fixed footprint, permanent style |
| Q6: Sanctum/Spire | Both have gameplay effects | +5% and +15% harmony respectively |

---

**End of Game Designer Answers**

*These answers provide actionable specifications for implementation. If any values need adjustment during testing, please coordinate before finalizing.*
