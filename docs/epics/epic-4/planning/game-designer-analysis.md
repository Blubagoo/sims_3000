# Epic 4: Zoning & Building System -- Game Designer Analysis

**Author:** Game Designer Agent
**Date:** 2026-01-28
**Epic:** 4 -- Zoning & Building System
**Canon Version:** 0.6.0

---

## Table of Contents

1. [Experience Goals](#1-experience-goals)
2. [Zone Types Deep Dive](#2-zone-types-deep-dive)
3. [Zone Density System](#3-zone-density-system)
4. [Demand System (Zone Pressure)](#4-demand-system-zone-pressure)
5. [Building Lifecycle](#5-building-lifecycle)
6. [Building Variety & Templates](#6-building-variety--templates)
7. [Multiplayer Dynamics](#7-multiplayer-dynamics)
8. [Terrain Interaction](#8-terrain-interaction)
9. [Pacing & Progression](#9-pacing--progression)
10. [Key Work Items](#10-key-work-items)
11. [Questions for Other Agents](#11-questions-for-other-agents)
12. [Risks & Concerns](#12-risks--concerns)

---

## 1. Experience Goals

### Why This Epic Matters

Epic 4 is the most important epic in the entire project from a player experience perspective. Epics 0 through 3 built the stage -- the engine, the network, the renderer, the terrain. Epic 4 is where the player steps onto that stage and says "I am the Overseer." This is the moment the software stops being a tech demo and becomes a *game*.

The core loop of every city builder lives here: designate zones, watch structures materialize, respond to what happens, expand. If this loop does not feel satisfying, nothing built on top of it will save the experience.

### Emotional Arc of the Core Loop

The fundamental cycle should produce this emotional sequence, repeating hundreds of times per session:

1. **Anticipation** -- "I have a plan. I am going to designate habitation here, exchange there." The player feels agency and intent.
2. **Commitment** -- "I am placing the designation." The click is meaningful because it costs credits, claims sectors, and sets expectations.
3. **Wonder** -- "Something is materializing." Structures begin to appear autonomously. The colony is alive. The player did not place a building -- they created the *conditions* for life to emerge.
4. **Satisfaction** -- "It worked. Beings are inhabiting. The exchange district is thriving." The plan paid off. The colony grows.
5. **Tension** -- "That sector is stalling. That district is declining. Growth pressure is shifting." New problems emerge from success, pulling the player back to step 1.

This is the SimCity magic: the player is a gardener, not an architect. They do not place individual structures. They cultivate conditions, and the colony responds. The gap between player action (designating a zone) and system response (structures materializing, beings arriving, the economy shifting) is where engagement lives.

### Core Experience Pillars Applied to Epic 4

| Pillar | How Epic 4 Serves It |
|--------|---------------------|
| **Building** | Designating zones and watching structures materialize IS building. This is the primary expression of the pillar. |
| **Managing** | Balancing habitation/exchange/fabrication demand. Deciding density. Responding to stalling zones. |
| **Competing** | Comparing colony growth rates with rivals. Racing for prime sectors. Seeing rival structures appear on the shared map. |
| **Expressing** | Zone layout as creative expression. Each colony's shape and composition is unique. The player's "signature" emerges through how they zone. |

### The "Feel" We Are After

**The colony should feel alive and responsive, not mechanical.** When the player designates a zone, the structures that appear should feel like an organic response from the colony -- beings choosing to settle, merchants choosing to trade, fabricators choosing to produce. The player set the stage; the colony performed.

**Growth should feel earned, not automatic.** Structures should only materialize when conditions are genuinely right: pathway access, energy, fluid, demand. When a zone thrives, the player should feel "I created those conditions." When a zone stalls, the player should feel "Something is missing -- what do I need to provide?"

**Decline should feel like a signal, not a punishment.** Derelict structures are information: "This area needs attention." They should create urgency without creating frustration. The player should always feel they can fix it.

**The alien theme should enhance, not replace, the city builder fantasy.** Beings inhabiting a luminous dwelling on a bioluminescent world is still "people moving into homes." The alien layer adds wonder and novelty on top of familiar, intuitive mechanics.

---

## 2. Zone Types Deep Dive

### 2.1 Habitation Zones

**Player Fantasy/Role:** "I am providing shelter for my colony. I am creating places where beings can live, grow, and thrive."

Habitation is the most emotionally resonant zone type. It represents *life* -- beings choosing to call this colony home. When habitation zones fill up, the player feels like a successful leader. When they sit empty or decline, it feels personal -- "Why don't beings want to live here?"

**Visual Identity (Bioluminescent Art Direction):**

- **Base Tone:** Deep blue-green organic forms. Habitation structures should feel grown rather than built -- rounded edges, asymmetric profiles, living architecture.
- **Glow Accents:** Soft cyan and teal windows/apertures. Active dwellings pulse gently with inner light, as if the beings inside are generating warmth. The glow should be warm and inviting.
- **Low Density:** Small, individual dwelling pods. Organic shapes like rounded domes, shell-like structures, or clustered coral forms. 1x1 tile footprint. Modest glow -- a few lit apertures.
- **High Density:** Towering hab-spires. Vertical organic columns with many apertures, like crystalline hive towers or luminous coral stacks. 1x1 tile footprint but significantly taller. Intense, multi-colored glow from dozens of apertures. Visible activity (cosmetic beings near entrances at ground level).
- **Derelict State:** Glow fades to near-dark. Structure surfaces crack, showing dead organic material underneath. Color shifts from healthy blue-green to grey-brown. Apertures go dark one by one.
- **Theme Distinction:** Unlike human houses and apartments (boxes with windows), these structures are clearly alien: organic, asymmetric, bioluminescent, alive-looking when active.

**Growth Progression:**

1. **Empty Designation** -- Zone color overlay on terrain (soft cyan tint). Terrain is prepared.
2. **Materializing (Low Density)** -- Small dwelling pod rises from the ground, bioluminescent tendrils forming the structure. Construction animation takes a few seconds of real time.
3. **Active (Low Density)** -- Dwelling glows softly. Cosmetic beings occasionally visible near pathways.
4. **Density Upgrade** -- If conditions support high density, the small dwelling dissolves (deconstruction animation) and a hab-spire materializes in its place. This should feel like a natural metamorphosis, not demolition-and-rebuild.
5. **Active (High Density)** -- Hab-spire pulses with life. Many lit apertures. More cosmetic beings in the area. Clearly a thriving hub of activity.

**Feedback Signals:**

| Signal | Meaning | Visual |
|--------|---------|--------|
| Strong glow, many lit apertures | Thriving, at capacity | Bright cyan/teal |
| Moderate glow | Functional but not full | Normal lighting |
| Flickering glow | Energy deficit or low fluid | Intermittent light |
| Fading glow | Declining -- beings leaving | Lights going dark progressively |
| No glow, cracked surfaces | Derelict -- abandoned | Grey-brown, dark |

### 2.2 Exchange Zones

**Player Fantasy/Role:** "I am creating a marketplace. I am building the economy that sustains my colony."

Exchange zones are about prosperity and activity. They represent commerce, trade, social gathering -- the economic lifeblood of the colony. When exchange zones thrive, the colony feels prosperous. When they fail, the colony feels stagnant.

**Visual Identity (Bioluminescent Art Direction):**

- **Base Tone:** Amber, warm gold, and orange-tinged structures. Exchange zones should feel energetic and active, contrasting with habitation's cool tones.
- **Glow Accents:** Bright amber and warm orange lights. Active exchange structures project light outward rather than containing it inward -- these are places of display and attraction. Signage-like glow patterns on surfaces.
- **Low Density:** Small market pods, trading kiosks, open-air exchange platforms. Flat or slightly elevated structures with prominent display surfaces. Warm inviting glow. 1x1 tile footprint.
- **High Density:** Exchange towers and trade spires. Vertical structures with dramatic exterior lighting -- think bioluminescent billboards and holographic signage as organic glow patterns on the building surface. Busier, more vibrant, taller. 1x1 tile footprint but taller silhouette.
- **Derelict State:** Amber glow dies to dull brown. Display surfaces go blank/dark. Structure sags or tilts slightly. Feels economically dead.
- **Theme Distinction:** Alien marketplaces are not strip malls. They are organic trade-hubs with bioluminescent advertising, pulsing with economic activity visible from a distance.

**Growth Progression:**

1. **Empty Designation** -- Zone color overlay (soft amber tint).
2. **Materializing (Low Density)** -- Small exchange pod forms. Structure unfurls rather than rises -- opening outward like a flower, reflecting the outward-facing nature of commerce.
3. **Active (Low Density)** -- Warm amber glow. Occasional cosmetic beings entering/exiting.
4. **Density Upgrade** -- Pod dissolves, exchange tower materializes with ascending formation animation.
5. **Active (High Density)** -- Brilliant amber glow patterns across the facade. Prominent from a distance. Cosmetic activity at ground level.

**Feedback Signals:**

| Signal | Meaning | Visual |
|--------|---------|--------|
| Bright amber patterns, active facades | Thriving commerce | Vibrant warm glow |
| Moderate glow | Functional exchange | Normal amber |
| Dim, static glow | Low demand, few customers | Fading amber |
| Dark, sagging structure | Derelict -- no commerce | Grey-brown, dark |

### 2.3 Fabrication Zones

**Player Fantasy/Role:** "I am building the productive engine of my colony. I am creating the industrial base that produces goods and employs laborers."

Fabrication zones are about power, production, and pragmatism. They are essential but come with tradeoffs (contamination). The player places them knowing they are necessary but must manage their side effects. This creates interesting spatial decisions: fabrication zones need to exist, but where?

**Visual Identity (Bioluminescent Art Direction):**

- **Base Tone:** Deep purple and magenta structures with harsh, bright accent lighting. Fabrication should feel powerful and slightly ominous -- efficient but not cozy.
- **Glow Accents:** Bright magenta and hot pink emissions. Unlike habitation's soft inner glow or exchange's outward display glow, fabrication glow is *exhaust* -- it pulses from vents, chimneys, and processing apertures. It is industrial luminescence.
- **Low Density:** Compact fabricator pods. Blocky or angular shapes (contrasting with habitation's organic curves). Visible processing vents with colored emission. 1x1 tile footprint.
- **High Density:** Fabrication complexes. Tall, angular structures with multiple emission points. Visible activity -- exhaust plumes of luminescent vapor, pulsing conduits along the exterior. More imposing and less inviting. 1x1 tile footprint, tall and broad silhouette.
- **Derelict State:** Emission glow dies. Structures darken to deep purple-grey. Vents go cold and still. May leak residual contamination glow (sickly yellow-green) as decay sets in.
- **Theme Distinction:** Alien factories are not smokestacks. They are processing organisms or crystalline refineries. The "pollution" they produce should look alien too -- luminescent vapor, not grey smoke.

**Growth Progression:**

1. **Empty Designation** -- Zone color overlay (soft magenta tint).
2. **Materializing (Low Density)** -- Fabricator pod assembles from the ground up, angular formation with visible internal machinery activating.
3. **Active (Low Density)** -- Magenta emission from processing vents. Rhythmic pulsing indicates active production.
4. **Density Upgrade** -- Pod deconstructs, fabrication complex materializes with a more dramatic, industrial formation sequence.
5. **Active (High Density)** -- Multiple emission points. Luminescent exhaust plumes. Clearly a major production facility.

**Feedback Signals:**

| Signal | Meaning | Visual |
|--------|---------|--------|
| Strong magenta emissions, rhythmic pulsing | Full production | Bright magenta |
| Moderate emissions | Partial production | Normal magenta |
| Weak, irregular emissions | Declining output | Dim, stuttering |
| No emissions, dark vents | Derelict -- shut down | Purple-grey, still |
| Yellow-green leak glow | Contamination source | Warning color |

### 2.4 Zone Color Language Summary

A critical design point: the three zone types must be instantly distinguishable at a glance, even zoomed out. The bioluminescent art direction gives us a powerful tool here -- *color*.

| Zone | Base Tone | Glow Accent | Overlay Tint | Emotional Register |
|------|-----------|-------------|--------------|-------------------|
| Habitation | Blue-green | Soft cyan/teal | Cyan | Warm, living, inviting |
| Exchange | Amber/gold | Bright amber/orange | Amber | Active, prosperous, busy |
| Fabrication | Deep purple | Hot magenta/pink | Magenta | Powerful, industrial, intense |

This color language should be consistent across all contexts: zone overlays, structure glow, minimap (sector scan) representation, UI icons, and demand indicators.

---

## 3. Zone Density System

### Two Density Tiers: Low and High

Canon defines two density levels: low and high. This is the right call for several reasons:

- **Simplicity:** Two tiers keeps the decision space manageable. Players ask "low or high?" not "which of five density levels?"
- **Visual Clarity:** Low structures vs. tall structures is an instantly readable visual difference.
- **Meaningful Choice:** Low density is cheaper and faster but caps growth. High density enables more beings/commerce/production per sector but has prerequisites.

### When Should Players Use Each Density?

**Low Density Habitation ("Dwellings")**
- **When:** Early colony, expansion frontiers, low-value sectors, areas near fabrication zones (beings tolerate contamination less in cramped high-density towers).
- **Player Thinking:** "I need to house beings cheaply and quickly. I do not need maximum capacity here."
- **Characteristics:** Low cost, fast materialization, low population per sector, low infrastructure demand (less energy, less fluid per sector). Modest sector value contribution.

**High Density Habitation ("Hab-Spires")**
- **When:** Core colony sectors, high sector value areas, near exchange zones, when growth pressure demands more capacity.
- **Player Thinking:** "This is prime real estate. I want maximum beings per sector to drive my economy."
- **Characteristics:** Higher cost, slower materialization, high population per sector, high infrastructure demand. Significant sector value contribution. Requires more energy, more fluid, better pathway access.

**Low Density Exchange ("Market Pods")**
- **When:** Small neighborhoods, areas near habitation that need basic services/goods, early-game economy.
- **Player Thinking:** "This district needs somewhere for beings to trade. Nothing fancy."
- **Characteristics:** Small economic output, low traffic generation, modest but quick.

**High Density Exchange ("Exchange Towers")**
- **When:** Core sector districts, high sector value areas, near major pathway junctions, when the colony economy demands more commercial capacity.
- **Player Thinking:** "I need a major commercial hub. This is the heart of my colony's economy."
- **Characteristics:** High economic output, significant traffic generation, attracts beings from wider area. Needs strong pathway connectivity and high sector value.

**Low Density Fabrication ("Fabricator Pods")**
- **When:** Early industry, distant sectors where contamination is acceptable, areas with limited infrastructure.
- **Player Thinking:** "I need production capacity. This area is expendable for contamination."
- **Characteristics:** Moderate contamination output, moderate employment, cheap.

**High Density Fabrication ("Fabrication Complexes")**
- **When:** Dedicated industrial districts, areas far from habitation, when employment demand is high.
- **Player Thinking:** "I need serious production. I have planned for the contamination."
- **Characteristics:** High contamination output, high employment, high traffic. Needs strong pathway access. Significantly impacts nearby sector value (negatively via contamination, positively via employment).

### Density Progression -- How It Happens

Density is designated by the player, not automatically upgraded. This is a critical design decision.

**The player chooses the density level when placing a zone.** A sector designated as low-density habitation will only ever spawn low-density dwellings. To get high-density hab-spires, the player must either:

1. Designate new sectors as high-density habitation, OR
2. Re-designate existing low-density sectors as high-density (redesignation)

**Why player-chosen, not automatic?** Because automatic density upgrades remove player agency. The player should decide "this district is ready for high density" based on their understanding of infrastructure capacity, sector value, and demand. Making this decision automatic turns an interesting strategic choice into a passive event.

**Redesignation Process:**
- Player selects the redesignation tool and paints over existing low-density zones.
- Existing low-density structures enter a "metamorphosis" phase: the old structure dissolves and a new high-density structure materializes in its place.
- Redesignation has a credit cost (reflecting the disruption and infrastructure upgrades needed).
- During metamorphosis, the sector temporarily produces no population/commerce/production. This creates a meaningful trade-off: short-term loss for long-term gain.

### Density Effects on Surroundings

| Density Level | Population/Capacity | Infrastructure Demand | Contamination | Pathway Traffic | Visual Height |
|---------------|--------------------|-----------------------|---------------|-----------------|---------------|
| Low | Low | Low | Low (fabrication only) | Low | Short (1-2 stories) |
| High | High (3-5x low) | High | High (fabrication only) | High | Tall (4-8 stories) |

High density zones create more demand on surrounding infrastructure, which creates cascading decisions: upgrading a habitation district to high density means more beings, which means more pathway traffic, which means more exchange demand, which means the player needs to plan for that growth.

---

## 4. Demand System (Zone Pressure)

### Overview

The demand system (canonically "zone pressure" or "growth pressure") is the invisible engine that drives the entire zoning gameplay loop. It answers the question every player constantly asks: "What should I build next?"

In canonical terms, the three pressure types are:
- **Habitation Pressure** (H) -- How much beings want to move into the colony
- **Exchange Pressure** (E) -- How much commercial activity wants to establish
- **Fabrication Pressure** (F) -- How much production capacity wants to exist

### What Drives Demand for Each Zone Type?

**Habitation Pressure (H) -- "Do beings want to live here?"**

Positive factors (increase H pressure):
- Available employment (exchange and fabrication jobs without enough inhabitants)
- Low tribute rates (tax)
- High colony harmony (happiness)
- Good services (enforcers, medical, learning centers nearby)
- Low contamination
- Available infrastructure (energy, fluid, pathways)

Negative factors (decrease H pressure):
- High unemployment (too many beings, not enough jobs)
- High tribute rates
- Low harmony
- High contamination
- Missing infrastructure (no energy, no fluid)
- High disorder (crime)

**Exchange Pressure (E) -- "Does commerce want to establish here?"**

Positive factors:
- Large colony population (more beings = more demand for exchange)
- High sector value in available zones
- Good pathway connectivity (customers need access)
- Proximity to habitation zones
- Port access (later epics -- external trade)

Negative factors:
- Low population (not enough customers)
- Poor pathway access
- High tribute rates
- Low sector value
- High contamination in exchange zones

**Fabrication Pressure (F) -- "Does industry want to operate here?"**

Positive factors:
- High exchange zone demand (commerce needs goods)
- Available labor force (population with adequate employment openings)
- Low tribute rates
- Port access (later epics -- external markets)

Negative factors:
- Low demand from exchange zones
- High tribute rates
- No available labor
- Very high contamination (even fabrication has limits)

### The Feedback Loops

This is where the demand system creates emergent gameplay:

**The Growth Spiral (positive feedback):**
1. Player places habitation zones. Beings arrive.
2. Beings create exchange pressure ("We need somewhere to trade!").
3. Player places exchange zones. Commerce establishes.
4. Exchange zones create fabrication pressure ("We need goods to sell!").
5. Player places fabrication zones. Production begins.
6. Fabrication zones create employment, which creates habitation pressure ("More beings could find jobs here!").
7. Return to step 1.

**The Stagnation Trap (negative feedback -- intentional):**
1. Player over-zones one type without supporting infrastructure.
2. Structures materialize but stall (no energy, no fluid, no pathway access).
3. Demand pressure decreases for that zone type ("Conditions are not met").
4. New structures stop materializing. Existing ones may decline.
5. Player must fix infrastructure to restart growth.

**The Balance Challenge (the interesting part):**
- Too much habitation without exchange = unemployment, being unrest, exodus.
- Too much exchange without habitation = empty shops, commercial decline.
- Too much fabrication without exchange = overproduction, industrial decline.
- The player must constantly balance all three, creating the ongoing management tension that defines the genre.

### Communicating Demand to the Player

**Primary: The Growth Pressure Indicator (Demand Bars)**

A visible UI element (in both legacy and holo interface modes) showing three vertical bars:

| Bar | Color | Label (Canonical) |
|-----|-------|------------------|
| H | Cyan/teal | Habitation Pressure |
| E | Amber/orange | Exchange Pressure |
| F | Magenta/pink | Fabrication Pressure |

Bars extend upward for positive pressure (the colony wants more of this zone type) and downward for negative pressure (the colony has too much of this type). The further a bar extends, the more strongly the colony is "asking" for that zone type.

**Secondary: Zone Overlay Visual Feedback**

When a zone overlay is active, sectors with unmet demand could pulse more strongly, and sectors with excess could dim. This gives spatial information: "Where is the demand strongest?"

**Tertiary: Notification System**

At threshold levels, the game should notify the player:
- "Habitation pressure is critical -- beings are seeking dwellings."
- "Exchange zones are oversaturated -- merchants are leaving."
- "Fabrication laborers are idle -- insufficient exchange demand."

### What Creates Interesting Demand Tension?

The best moments in city builders come from demand *conflict* -- situations where you cannot satisfy all pressures simultaneously:

1. **Spatial Tension:** You have a prime high-value sector. Habitation pressure and exchange pressure are both high. Do you zone it for habitation (more beings) or exchange (more commerce)? You cannot do both on the same sector.

2. **Infrastructure Tension:** You have enough energy for 50 more structures. Habitation, exchange, and fabrication all have high pressure. Where do you allocate the energy coverage? Expanding conduits in one direction means another zone waits.

3. **Contamination Tension:** Fabrication pressure is high, but the only available sectors are near habitation. Do you accept the contamination penalty to meet fabrication demand, or do you let fabrication pressure build while you charter distant sectors?

4. **Multiplayer Tension:** A rival is expanding aggressively. You need to charter sectors before they do, but growth pressure suggests you should consolidate existing zones first. Rush or optimize?

---

## 5. Building Lifecycle

### 5.1 Materializing (Construction)

**What Happens:** When conditions are met (zone designated, demand present, infrastructure connected, pathway within 3 sectors), the BuildingSystem selects a template from the appropriate pool and begins materialization.

**What It Looks Like:**

The materialization process should be one of the most visually delightful moments in the game. This is not human construction (scaffolding, cranes). This is alien:

- **Phase 1: Emergence (0-25%)** -- A glow point appears at the center of the sector. Bioluminescent tendrils/strands begin extending upward and outward from the ground, forming a translucent wireframe of the structure-to-be. Color matches the zone type (cyan for habitation, amber for exchange, magenta for fabrication).
- **Phase 2: Formation (25-75%)** -- The wireframe fills in with opaque material, growing from the base upward. The structure "crystallizes" or "solidifies" section by section. Construction glow is brighter than the final active glow -- it is visually attention-grabbing.
- **Phase 3: Activation (75-100%)** -- The structure is fully formed. The intense construction glow settles down to the normal active glow level. A brief "completion pulse" radiates outward -- a momentary brightening that signals "this structure is now active."

**What It Feels Like:** Wonder. The player placed the zone, and now the colony is responding. Watching structures materialize should feel like watching a garden bloom -- organic, alive, beautiful. This is a reward for good planning.

**Duration:** Materialization should take long enough to be noticed (3-8 seconds of real time at normal speed) but not so long that it becomes tedious. The player should be able to glance at a newly zoned area and see structures in various stages of materializing, creating a sense of bustling growth.

**Sound Design Note:** The materialization process should have a satisfying crystalline/organic growth sound effect that is subtly different for each zone type. Players should be able to *hear* their colony growing, even when zoomed away from the construction site.

### 5.2 Active / Growing State

**What Happens:** The structure is complete and functioning. Beings inhabit dwellings, merchants operate exchange pods, fabricators produce goods. The structure contributes to colony population, economy, and employment.

**What It Looks Like:**
- Normal glow level for the zone type.
- Cosmetic activity: beings near habitation entrances, activity at exchange structures, emission from fabrication vents.
- Over time, active structures in high-value areas could develop subtle visual enhancements: slightly brighter glow, additional decorative elements (small garden-like bioluminescent growths at the base, light particles).

**Upgrade Path:**

Active structures do not individually "upgrade." Instead, density is a zone-level property. If the player redesignates a low-density zone to high-density, existing structures undergo metamorphosis (see Section 3). However, within a given density level, structures at different sector value tiers may use different templates:

- **Low sector value:** Basic templates. Simple shapes, modest glow.
- **Medium sector value:** Improved templates. More detail, additional glow elements.
- **High sector value:** Premium templates. Most detailed models, richest glow effects, distinct architectural character.

When sector value changes around an existing structure (due to new services, contamination changes, etc.), the structure *does not* automatically swap templates. Template selection happens only at materialization time. This means older structures in improving districts look "older" than newer ones -- a natural visual history of the colony's development that creates authentic character.

### 5.3 Decline

**What Causes Decline?**

Structures decline when the conditions that supported them deteriorate:

- **Energy loss:** Grid collapse or energy deficit. Without energy, structures cannot function.
- **Fluid loss:** Without fluid, habitation and exchange structures stall. Beings need fluid.
- **Pathway disconnection:** If pathways decay or are demolished, structures beyond 3 sectors lose access and begin declining.
- **Demand collapse:** If zone pressure turns strongly negative, excess structures begin declining. Beings leave because there are no jobs; merchants leave because there are no customers.
- **Contamination:** Rising contamination drives beings away from habitation zones and reduces exchange zone viability.
- **High disorder:** Unchecked disorder causes beings and merchants to flee.
- **Service withdrawal:** Loss of enforcer, medical, or hazard response coverage reduces sector value and can trigger decline.

**What It Looks Like:**

Decline is a gradual visual degradation, not a sudden change:

1. **Early Decline:** Glow begins to flicker or dim. Apertures/windows go dark intermittently. The structure still looks intact but is clearly "struggling."
2. **Mid Decline:** Glow is significantly reduced. Some structural elements appear to wilt or sag (organic architecture drooping). Color shifts away from healthy toward grey undertones.
3. **Late Decline:** Glow is nearly gone. Structure shows visible damage -- cracks, missing sections, darkened surfaces. This is the last stage before abandonment.

**How It Feels:**

Decline should create *urgency without panic*. The gradual visual degradation gives the player time to diagnose and respond. They see a section of their colony dimming and think "What is going wrong there?" -- not "Everything is ruined!" The decline gradient from "struggling" to "derelict" should take many simulation cycles, giving the player a comfortable window to act.

**Critical Design Rule:** Decline is always the player's "fault" in the sense that it is always caused by conditions the player could address. This maintains player agency. Structures never decline randomly or capriciously.

### 5.4 Abandonment (Derelict State)

**What Happens:** If decline conditions persist long enough without correction, the structure enters the derelict state. Beings leave. Commerce ceases. Production stops. The structure is dead -- it contributes nothing to the colony.

**What It Looks Like:**

- **All glow is extinguished.** This is the most impactful visual signal. In a bioluminescent world, a dark structure is deeply striking -- a void in the colony's luminous fabric.
- **Color shifts to grey-brown.** The vibrant zone-specific colors are gone.
- **Structural decay:** Cracks, listing angles, partially collapsed sections. The organic architecture is dying.
- **Contamination:** Derelict fabrication structures may leak residual contamination (yellow-green glow from cracks), creating ongoing problems even when inactive.

**Emotional Impact:**

Derelict structures should evoke a specific emotion: **quiet melancholy mixed with determination.** The player sees the darkness in their colony and feels "I need to fix this" -- not "I give up." The visual contrast between a glowing, thriving district and a dark, derelict one is powerful. It should make the player *want* to restore the area or clear it and start fresh.

**Recovery:** Derelict structures can recover if conditions improve. If energy, fluid, and pathway access are restored, and if demand is positive, derelict structures will slowly re-activate: glow returns, surfaces repair, the structure comes back to life. This recovery process should be as visually satisfying as initial materialization -- the colony reclaiming what was lost.

**Design Note:** Derelict structures should persist indefinitely. They do not auto-demolish. The player must choose to deconstruct them (costing credits) or wait for recovery. This forces an active decision rather than letting problems silently disappear.

### 5.5 Demolition (Deconstruction)

**What Happens:** The player uses the deconstruction tool to remove a structure. The structure is destroyed, the sector returns to its designated zone state (empty, ready for new structures), and the terrain beneath is cleared.

**What It Looks Like:**

The deconstruction animation should be a satisfying *reverse materialization*:

- Structure glows bright briefly (a "farewell pulse").
- Structure dissolves from top to bottom, bioluminescent particles dispersing into the air.
- Ground-level debris briefly visible, then fades.
- Sector returns to the zone overlay color, ready for new development.

**Cost:** Deconstruction has a credit cost. This is small for low-density structures and moderate for high-density structures. The cost prevents frivolous demolition spam while not being so expensive that players avoid necessary clearing.

**What Remains:** After deconstruction, the sector retains its zone designation. It is immediately available for new structures to materialize (if demand exists). The player does not need to re-designate the zone.

**Player Choice:** Deconstruction is always the player's choice. No system auto-demolishes player structures (except the ghost town process for abandoned colonies, which is a separate canon mechanic). The player is always in control of their colony's physical form.

---

## 6. Building Variety & Templates

### Template Counts Per Zone Type and Density

Visual variety is essential to prevent the colony from looking like a grid of identical boxes. The template system (defined in `patterns.yaml`) provides variety through template pools, parameter variation, and selection rules.

**Recommended Initial Template Counts:**

| Zone Type | Density | Template Count | Rationale |
|-----------|---------|---------------|-----------|
| Habitation | Low | 5-6 | Most numerous zone type; variety prevents monotony |
| Habitation | High | 4-5 | Fewer but more detailed; each should be distinct |
| Exchange | Low | 4-5 | Second most common; needs visual identity |
| Exchange | High | 3-4 | Distinctive commercial towers |
| Fabrication | Low | 3-4 | Less visually prominent; functional aesthetic |
| Fabrication | High | 3-4 | Large industrial structures; each should be imposing |

**Total: 23-28 unique building templates at launch.** This is the minimum for a visually interesting colony. More can be added post-launch, and the template system is designed for extension.

### Variation Parameters

Beyond template selection, each placed structure has variation parameters that create uniqueness:

| Parameter | Range | Effect |
|-----------|-------|--------|
| **Rotation** | 0, 90, 180, 270 degrees | Same template, different facing. Prevents grid-alignment monotony. |
| **Color Accent Shift** | +/- 10-15% hue shift on glow accents | Two "same" dwellings glow slightly different shades of cyan. Organic variety. |
| **Scale Variation** | +/- 5-10% on height | Slight height differences create an organic skyline, not a flat wall. |
| **Glow Intensity** | +/- 10% | Subtle brightness variation between structures of the same type. |
| **Detail Seed** | Random uint32 | Controls which optional detail elements appear (antenna-like growths, base vegetation, surface patterns). |

These parameters are set at materialization time using the server's seeded RNG (ensuring multiplayer consistency). They do not change over the structure's lifetime.

### Sector Value and Template Selection

Sector value (land value) influences *which* templates are eligible to spawn:

- **Low Sector Value (0-30):** Only basic templates available. Simple, modest structures.
- **Medium Sector Value (31-65):** Basic and improved templates available. More detailed, larger structures possible.
- **High Sector Value (66-100):** All templates available, including premium ones. The most architecturally impressive structures only appear in high-value sectors.

Each template has a `min_land_value` field. Selection picks randomly from all eligible templates for the zone type, density, and sector value. Adjacent structures should avoid duplicates (the system checks neighbors and re-rolls if the same template appears next to itself).

### Should Players Control Building Appearance?

**No.** This is a deliberate design decision.

The player controls *zone type* and *density*. The colony autonomously determines which structures appear. This is core to the SimCity fantasy: you are an overseer, not an architect. You create conditions; the colony responds.

Player control over individual structure appearance would:
1. Slow down the core loop (micromanagement instead of macro-management).
2. Undermine the "living colony" feeling.
3. Create analysis paralysis in a game about flow.

The player's creative expression comes from *zoning layout*, not individual structure placement. Two overseers given the same map and conditions will create very different colonies through different zoning patterns, pathway layouts, and district plans. That is the expression vector.

**One possible exception (future feature, not Epic 4):** A "style preference" setting per district that biases template selection without controlling it directly. "I want this habitation district to favor taller structures" or "I want this exchange district to be organic rather than angular." This is a nice-to-have for a later polish pass.

---

## 7. Multiplayer Dynamics

### Can You See Other Overseers' Zones?

**Yes, fully.** All players share the same map and can see everything. Zone overlays, structures, infrastructure -- it is all visible. This is fundamental to the competitive and social experience.

Seeing other overseers' colonies creates:
- **Inspiration:** "Their habitation district looks amazing. I want to build something like that."
- **Competition:** "They are growing faster than me. I need to step it up."
- **Strategic Information:** "They are zoning fabrication near my border. That contamination could affect my habitation zones."
- **Social Engagement:** "Look at their colony -- they zoned exchange right next to blight mires! What were they thinking?"

Each overseer's zones and structures should be subtly tinted or marked with their faction color so ownership is always clear. This could be:
- A faint faction-color border around owned sectors.
- A subtle faction-color tinge to structure glow (Player 1's habitation glows slightly more blue-cyan, Player 2's slightly more green-cyan, etc.).
- Faction color on pathways and infrastructure within owned sectors.

### Zone Competition Near Borders

Border dynamics create some of the most interesting multiplayer moments:

**Chartering Race:** When two overseers want the same uncharted sectors, it becomes a race. Who charters first? This creates urgency and excitement. Do you save credits for optimal sectors or rush to charter before your rival?

**Contamination Spillover:** Fabrication zones generate contamination that spreads. If one overseer places fabrication near the border, the contamination can affect a rival's habitation zones. This is intentional -- it creates spatial politics.

**Pathway Connectivity:** Pathways connect across ownership boundaries (canon: `connects_across_ownership: true` for roads). Two overseers' pathway networks merge when they meet. This creates implicit cooperation -- better pathway connectivity benefits both colonies. But it also means another overseer's flow blockage (congestion) can affect your pathways.

**Sector Value Influence:** High-quality districts near the border can boost or reduce neighboring overseers' sector values. A beautiful habitation district near the border raises sector value for adjacent sectors -- even rival-owned ones. A fabrication district lowers it. This creates interesting neighbor dynamics.

### Does One Overseer's Zone Affect Another's Demand?

**In the current canon model, each overseer has independent demand calculations.** Each overseer has their own habitation/exchange/fabrication pressure based on their own colony's conditions.

However, there are indirect effects:
- **Shared map resources:** Overseers compete for high-value terrain (crystal fields, water-adjacent sectors). Sectors charted by one overseer are unavailable to others.
- **Infrastructure competition:** Limited prime locations for pathways and transit corridors.
- **Late-game ports (Epic 8):** Port zones create demand bonuses that could influence map-wide dynamics.

**Design Question:** Should there be any cross-overseer demand interaction? For example, should a rival's successful exchange district reduce your own exchange pressure (competition for the same "market")? This would create deeper multiplayer dynamics but also more complexity. **Recommendation:** Start with independent demand and evaluate during playtesting. Cross-overseer demand interaction is a tuning knob, not a structural decision.

### Structure Comparison and Social Dynamics

In an endless sandbox with no victory conditions (canon: "No win/lose states"), the social dynamics around structures become the primary competitive element:

- **Colony Size Comparison:** Whose colony is bigger? More populated? More structures? These are the bragging rights in an endless sandbox.
- **Visual Aesthetics:** Whose colony looks better? In a bioluminescent world, a well-planned colony with glowing districts and clean infrastructure is visually impressive. Social comparison drives effort.
- **Growth Rate:** Even without a win condition, overseers will naturally compare who is growing faster. The growth pressure indicator serves as an implicit scoreboard.
- **Resilience:** When disasters arrive (Epic 13), whose colony survives better? Preparation becomes a source of pride.

**Social Features to Support (not all in Epic 4, but design for them):**
- Colony statistics visible to all overseers (population, structure count, etc.).
- The ability to view another overseer's colony closely (camera can move over anyone's territory).
- Chat/notification when an overseer achieves a growth milestone.

---

## 8. Terrain Interaction

### Buildable Terrain Types

Based on canon terrain types, here is the buildability matrix for zoning:

| Terrain Type | Canonical Name | Buildable? | Zone Placement? | Notes |
|-------------|----------------|------------|-----------------|-------|
| Flat Ground | Substrate | Yes | Direct | Standard buildable terrain. No extra cost. |
| Hills | Ridges | Yes | Direct | Increased structure materialization cost based on elevation delta. |
| Ocean | Deep Void | No | No | Not buildable. Map edge water. |
| River | Flow Channel | No | No | Not buildable. Required for fluid extractors nearby. |
| Lake | Still Basin | No | No | Not buildable. Boosts sector value for nearby zones. |
| Forest | Biolume Grove | Yes | After Purge | Must purge (clear) vegetation first. Purge costs credits. Boosts nearby sector value if preserved. |
| Crystal Fields | Prisma Fields | Yes | After Purge | Must purge first. Yields one-time credit bonus. Provides sector value bonus to nearby zones while intact. |
| Spore Plains | Spore Flats | Yes | After Purge | Must purge first. Boosts harmony for nearby habitation while intact. |
| Toxic Marshes | Blight Mires | No | No | Not buildable. Generates contamination. Must be avoided or (late-game) reclaimed via terraform. |
| Volcanic Rock | Ember Crust | Yes | Direct | Increased materialization cost. Natural barrier. Geothermal energy bonus for energy nexuses. |

### Terrain and Zone Placement Interaction

**Automatic Purge on Zone Designation:** When a player designates a zone on forest, crystal, or spore terrain, the system should automatically initiate the purge process (with the cost deducted from the overseer's credits). The player does not need to separately purge and then zone -- that would be tedious busywork. The zone designation tool should show the total cost (zone designation cost + purge cost if applicable).

**Alternative: Preserve-or-Purge Decision.** A more interesting design would give the player a choice when zoning over natural terrain: "This sector contains a biolume grove. Purge for building? (Cost: X credits. Note: Nearby sectors will lose sector value bonus.)" This makes the player consider whether the natural terrain's bonus is more valuable than the buildable space. For crystal fields, the decision is even more interesting: purge for a one-time credit payout plus buildable space, or preserve for ongoing sector value bonus to neighbors.

**Recommendation:** Implement the preserve-or-purge choice. It creates meaningful decisions and reinforces the alien theme (the overseers are interacting with a living alien world, not just bulldozing terrain).

### Elevation Effects

- **Flat Terrain (elevation 0-3):** No additional cost. Standard materialization.
- **Low Hills (elevation 4-10):** Slight cost increase (10-25%). Structures adapt their base to the slope (visual tilting or foundation extension).
- **Medium Hills (elevation 11-20):** Moderate cost increase (25-50%). More dramatic visual adaptation.
- **High Hills (elevation 21-31):** Significant cost increase (50-100%). Structures must be graded (terrain flattened) before placement, adding additional cost and time. However, high-elevation structures may have sector value bonuses (scenic views).

**Grading Integration:** If the player zones on significantly elevated terrain, the system should automatically grade (flatten) the sector before materialization begins. The grading cost is included in the total zone designation cost shown to the player.

### Water Proximity Bonuses

Water proximity should significantly affect zone performance:

- **Habitation near water (within 5 sectors of a flow channel, still basin, or deep void edge):** Sector value bonus. Beings prefer waterfront living. Visual enhancement: structures near water may have water-facing apertures or glow patterns oriented toward the water.
- **Exchange near water:** Sector value bonus. Waterfront commerce is premium.
- **Fabrication near water:** No sector value bonus (beings do not care about waterfront factories). However, fluid extractors near water are required for the fluid system (Epic 6), so fabrication near water has practical infrastructure value.

### Alien Terrain Special Effects

The alien terrain types create unique interactions with zones:

- **Crystal Fields (Prisma Fields) -- Preserved:** Adjacent zones get +15-20% sector value bonus. High-density exchange zones near crystal fields become premium commercial districts. Visually, crystal glow bleeds into nearby structure glow for a beautiful mixed-light effect.
- **Spore Plains (Spore Flats) -- Preserved:** Adjacent habitation zones get a harmony (happiness) bonus. Beings enjoy living near the spore plains. This creates a reason to preserve natural terrain rather than paving everything.
- **Toxic Marshes (Blight Mires) -- Unavoidable:** Contamination radiates from blight mires. Zones within the contamination radius suffer. Fabrication zones tolerate it best. Habitation zones suffer most. This creates natural "bad neighborhoods" that the overseer must plan around.
- **Volcanic Rock (Ember Crust):** Increased build costs but unique visual character (orange-red glow cracks in the terrain give the district an dramatic look). Energy nexuses on volcanic rock get a geothermal efficiency bonus, making it ideal for energy infrastructure placement.

---

## 9. Pacing & Progression

### Early Colony (First 10-15 Minutes of a Session)

**Player State:** Excitement, fresh start, full of plans.

**What Happens:**
1. Player spawns at their emergence site. They have starting credits and a small amount of pre-charted sectors.
2. **First Decision: Where to zone?** The player looks at their terrain and makes their first layout plan. This should feel exciting, not overwhelming.
3. **Habitation First.** The natural first move is habitation. The player needs beings before anything else. Zone 6-10 sectors of low-density habitation near a pathway.
4. **Pathway Foundation.** Place the core pathway network. This is Epic 7's domain, but it directly gates structure development in Epic 4 (3-sector pathway proximity rule). The player needs to lay pathways before zones can develop.
5. **First Structures Materialize.** The player watches their first dwellings appear. This is the "wow" moment. The colony is alive.
6. **Exchange Follows.** As beings arrive, exchange pressure builds. The player zones some exchange sectors. Market pods materialize.
7. **Fabrication Last.** Fabrication pressure builds as exchange grows. The player zones fabrication sectors, ideally downwind/downslope from habitation.

**Design Note:** The early colony should feel *fast* and *responsive*. Structures should materialize quickly at low density. The player should see results within seconds of placing zones. Slow early development kills the excitement of a new session.

**Pacing Knob:** Materialization speed should be tuned so that in the first 5 minutes, a player can have ~15-20 structures materialized. This creates a sense of rapid growth that hooks the player.

### Mid Colony (15-45 Minutes)

**Player State:** Engaged, managing multiple concerns, starting to feel the tension.

**What Happens:**
1. **Infrastructure Expansion.** The player needs energy and fluid (Epics 5, 6) to sustain growth. Without these, zones stall. The player is now managing multiple systems.
2. **Density Decisions.** Core districts may be ready for high-density redesignation. Does the player upgrade existing districts or expand outward? This is the first major strategic fork.
3. **Demand Balancing.** The growth pressure bars show imbalances. The player is now actively reading and responding to demand. "Exchange pressure is high -- I need more market pods."
4. **Spatial Planning.** The easy sectors are taken. The player starts making harder choices about terrain: do they purge the prisma fields for more space, or preserve them for sector value? Do they build near the blight mires?
5. **Multiplayer Awareness.** Rival overseers are growing too. The player starts paying attention to border dynamics, chartering competition, and comparative growth.

**Pacing Knob:** Mid-colony should feel like "busy management." The player should have 3-5 things they want to do and need to prioritize. There should always be a next action available but never a sense of idle waiting.

### Late Colony (45+ Minutes)

**Player State:** Pride in what they have built. Optimization mindset. Looking for refinement opportunities.

**What Happens:**
1. **Optimization.** The player starts redesignating zones, improving district layouts, optimizing pathway networks. This is where the "tweaker" personality thrives.
2. **Sector Reshuffling.** Early-game fabrication zones near the colony center might need to move to accommodate high-density habitation expansion. The player demolishes and re-zones sections of their colony. This is expensive but satisfying.
3. **Specialization.** Districts develop distinct characters: the waterfront habitation spires, the core sector exchange district, the distant fabrication corridor. The colony has an identity.
4. **Mega-Structures (Future Epics).** Arcologies and landmarks (Epic 14) provide late-game goals. The zoning/building system lays the foundation for these by establishing the population and economic base required.
5. **Endless Play.** Since there are no victory conditions, the late colony is about creative satisfaction and social engagement. The player builds because building is fun, not because they are chasing a win state.

**Pacing Knob:** Late colony should feel relaxed and creative. The urgency of early/mid colony gives way to refinement and expression. The player should never feel "done" -- there is always a district to improve, a layout to optimize, a rival to outpace.

### Session-to-Session Pull

Because ZergCity uses a persistent server with database-backed state (canon), the colony persists between sessions. This creates the "check on my colony" pull:

- "I left my colony growing last night. Have structures materialized in those new zones I designated?"
- "My rival was catching up. Have they expanded while I was away?"
- "I re-designated that district before logging off. Has the metamorphosis completed?"

The zoning/building system provides the primary reason to return to the game between sessions.

---

## 10. Key Work Items

### Design-Side Work Items

These are deliverables the Game Designer needs to produce or collaborate on for Epic 4:

| # | Work Item | Description | Dependencies |
|---|-----------|-------------|--------------|
| D-1 | Zone Color Specification | Exact color values for all zone overlays, glow accents, and visual states in the bioluminescent palette. Provide hex/RGB values for each zone type at each state (empty, materializing, active, declining, derelict). | Art direction review |
| D-2 | Materialization Animation Specification | Timing, visual phases, and feel targets for structure materialization. Define the 3-phase emergence/formation/activation process per zone type. Provide reference imagery or mood boards. | Graphics Engineer consultation |
| D-3 | Demand Tuning Parameters | Initial values for all demand factors (weights for each positive/negative factor per zone type). These are starting points for playtesting. | Systems Architect consultation |
| D-4 | Building Template Visual Briefs | For each of the 23-28 templates, a brief describing the visual identity, silhouette, glow character, and mood. These guide 3D model creation. | Art pipeline setup |
| D-5 | Decline/Dereliction Visual Stages | Detailed spec for the 3-stage decline process and derelict state. What changes at each stage? How long between stages? | Graphics Engineer consultation |
| D-6 | Zone Overlay Design | How zone designation overlays appear on terrain. Transparency, color, pattern (solid tint vs. hatch vs. border). Must work on all terrain types. | UI design review |
| D-7 | Growth Pressure UI Mockup | Layout and behavior of the demand bar indicator for both legacy and holo interface modes. How bars scale, colors, labels, and threshold notifications. | UI Developer consultation |
| D-8 | Multiplayer Visual Differentiation | How each overseer's structures and zones are visually distinguishable. Faction color integration with zone colors. Must not create visual noise. | Multiplayer design review |
| D-9 | Sound Design Direction for Structures | What materialization, activation, decline, and deconstruction should sound like for each zone type. Mood descriptions and audio reference examples. | Audio Engineer consultation |
| D-10 | Preserve-or-Purge Decision Flow | UX flow for the terrain clearing decision when zoning over natural terrain. Dialog design, cost display, information shown. | UI Developer consultation |

---

## 11. Questions for Other Agents

### For Systems Architect

@systems-architect: The BuildingSystem depends on EnergySystem, FluidSystem, and TransportSystem (per `systems.yaml`). In Epic 4's initial implementation, these systems do not yet exist (they are Epics 5, 6, and 7). How should the BuildingSystem handle the absence of these dependencies? Should structures materialize and activate without energy/fluid/pathway checks initially, with those gates added when the dependency epics are implemented? Or should we stub the interfaces now so structures behave correctly from the start (always returning "has power = true, has fluid = true, has pathway = true")?

@systems-architect: What is the expected data flow for demand calculation? Canon shows that ZoneSystem owns "zone demand calculation (basic)" but the DemandSystem in Epic 10 owns "RCI (zone pressure) calculation." Are these the same system split across epics, or does ZoneSystem have a simplified demand model that DemandSystem later replaces? What should Epic 4 implement for demand?

@systems-architect: The template selection process needs server-authoritative random values (seeded RNG per canon). How should the BuildingSystem access the seeded RNG? Is there a central RNG service, or does each system maintain its own seeded generator? Multiplayer consistency requires all clients to see the same template selections.

@systems-architect: Structure materialization animations are client-side visual effects, but the server must communicate "structure X began materializing at tick T with template Y." Should the BuildingConstructedEvent include the template ID and variation parameters (rotation, color shift, scale), or should clients derive these from the same seeded RNG?

@systems-architect: How should the redesignation (density upgrade) process work at the ECS level? Does the existing structure entity get destroyed and a new one created? Or does the existing entity's components get updated in place? The former is cleaner but requires more network traffic. The latter is more efficient but more complex.

### For Graphics Engineer

@graphics-engineer: The materialization animation described in this analysis (3-phase: emergence, formation, activation) requires shader support for partial transparency and glow interpolation. Is this feasible with the current toon shader pipeline? What are the rendering costs of having multiple structures in various materialization stages simultaneously?

@graphics-engineer: Structure glow intensity needs to vary based on game state (materializing, active, declining, derelict). Is this a per-instance shader uniform, or do we need separate materials/shaders for each state? What is the most performant approach for hundreds of structures with individually varying glow?

@graphics-engineer: The variation parameters (rotation, scale variation, color accent shift) need to be applied per instance. Does the current rendering pipeline support per-instance transformation and material parameter overrides efficiently?

### For Zone Engineer

@zone-engineer: What data structure should the ZoneComponent use? My recommendation based on game design needs: zone_type (habitation/exchange/fabrication), density (low/high), and designation_state (designated/materializing/occupied/derelict). Is this sufficient, or does the system need additional fields?

@zone-engineer: The 3-sector pathway proximity rule is critical for the player experience (it prevents zones from developing without pathway access). How should this be validated? Should ZoneSystem check proximity at designation time (preventing designation without nearby pathways) or should BuildingSystem check proximity at materialization time (allowing designation anywhere but only materializing structures with access)?

**Recommendation:** Allow designation anywhere but gate materialization on pathway proximity. This lets the player "plan ahead" by designating zones before building pathways to them, which feels much better than being blocked from designating.

### For Building Engineer

@building-engineer: How many simulation ticks should materialization take for each density level? Game design wants 3-8 seconds of real time at normal speed (1x). At 20 ticks/second, that is 60-160 ticks. Should this be configurable per template?

@building-engineer: The adjacent-duplicate-avoidance rule (same template should not appear next to itself) needs a spatial query. How should this be implemented? Simple Manhattan-distance check to all neighbors within 1 sector, or a more sophisticated approach?

@building-engineer: How should the "metamorphosis" process (low to high density redesignation) be represented in the ECS? My design intent is: old structure enters a "deconstructing" state (reverse materialization animation), then a new high-density structure enters "materializing" state. During the transition, the sector contributes no population/commerce/production. Is this two sequential entity lifecycle events, or one entity with complex state transitions?

### For UI Developer

@ui-developer: The growth pressure indicator (demand bars) is one of the most critical UI elements in the game. It needs to be visible at all times without being obtrusive. In the legacy interface, it should sit in the status bar area. In the holo interface, it could be a floating holographic readout. What are the constraints on always-visible UI elements in the current layout plans?

@ui-developer: Zone designation needs clear cost feedback before the player commits. When the player hovers over terrain with a zone tool selected, they should see: zone type, density, credit cost (including purge cost if applicable), and any terrain modifiers. What is the best UX pattern for this real-time cost preview?

---

## 12. Risks & Concerns

### Player Experience Risks

#### Risk 1: "Nothing Is Happening" Syndrome
**Description:** The player zones sectors, but structures do not materialize because prerequisites are not met (no energy, no fluid, no pathway). The player does not understand why.
**Severity:** Critical -- this kills the core loop.
**Mitigation:** Clear visual feedback on why zones are not developing. When a zone is designated but prerequisites are missing, show specific icons on the zone overlay: a darkened energy symbol, a missing fluid symbol, a disconnected pathway symbol. The player should never wonder "why isn't anything happening?" -- the answer should be visually obvious.

#### Risk 2: Demand System Opacity
**Description:** The player does not understand why demand bars are moving. The relationship between zone pressure and colony conditions is too opaque.
**Severity:** High -- demand is the game's steering mechanism.
**Mitigation:** Hovering over a demand bar should show a tooltip with the top 3 contributing factors: "Exchange Pressure: +45 (high population), +12 (low tribute rate), -20 (poor pathway access) = +37." The player should always be able to trace demand to actionable causes.

#### Risk 3: Density Trap
**Description:** Players redesignate everything to high density as soon as possible because "high density = more = better." Infrastructure collapses because high-density zones demand more energy, fluid, and pathway capacity than the player has.
**Severity:** Medium -- classic city builder trap, but needs mitigation.
**Mitigation:** Make the prerequisites for high-density development clear and visible. High-density zones that lack infrastructure should stall or decline *quickly*, teaching the player that density without infrastructure is counterproductive. The redesignation tool should warn: "High-density habitation requires X energy, Y fluid per sector. Your current surplus is Z."

#### Risk 4: Contamination Frustration
**Description:** New players do not understand contamination and place fabrication zones next to habitation. Habitation declines. Player blames the game.
**Severity:** Medium -- common new-player mistake in city builders.
**Mitigation:** When designating fabrication zones near habitation, show a warning: "Fabrication zones generate contamination that may affect nearby habitation zones." Include a contamination preview overlay showing the expected contamination radius. Educated players can still do it intentionally; new players are warned.

#### Risk 5: Multiplayer Border Griefing
**Description:** A player intentionally places fabrication zones at their border to contaminate a rival's habitation district.
**Severity:** Medium -- this is actually an interesting strategy, but needs limits.
**Analysis:** This is a feature, not a bug, as long as:
1. Contamination spread has a finite and predictable radius.
2. The contaminating player also suffers (their own nearby sectors lose value).
3. The victim has counterplay: place their own habitation away from the border, or invest in contamination mitigation (future epic feature).
**Recommendation:** Allow it but make the contamination radius and effects clearly visible to all overseers. Transparency prevents griefing from feeling unfair.

#### Risk 6: Early Game Without Infrastructure Dependencies
**Description:** If Epic 4 ships before Epics 5 (energy), 6 (fluid), and 7 (transport), structures will materialize without any infrastructure gates. When those epics are added, existing structures may retroactively fail, creating a jarring experience.
**Severity:** High -- architectural risk with player experience consequences.
**Mitigation:** Stub the dependency interfaces (see question to Systems Architect). Structures should behave as if fully powered, watered, and connected in Epic 4. When dependency epics ship, they gate *new* development, and existing structures are grandfathered for a grace period.

### Fun Killers to Avoid

1. **Waiting without information.** If structures take time to materialize, the player needs to know progress is happening. A visual progress indicator (the 3-phase materialization animation) is essential. Never leave the player staring at an empty zone with no feedback.

2. **Punishment without agency.** Every negative outcome (decline, abandonment, contamination) must have a player-addressable cause. "Your habitation is declining because contamination is high" is useful. "Your habitation is declining" with no explanation is a fun killer.

3. **Micro-management tedium.** The player should manage at the district/zone level, not the individual structure level. If the player feels they need to babysit each structure, the abstraction level is wrong. Zones should be "paint and observe," not "place and manage."

4. **Snowballing in multiplayer.** If a leading overseer's growth advantage becomes insurmountable, trailing overseers will disengage. The endless sandbox model (no victory conditions) mitigates this somewhat, but the game should still offer catch-up opportunities: unclaimed prime sectors, abandoned rival territories becoming available, etc.

5. **Analysis paralysis.** Too many zone types, density levels, or demand factors can overwhelm. The current design (3 zone types, 2 density levels) is deliberately simple. Resist the urge to add complexity for its own sake.

### Complexity Traps to Avoid

1. **Per-structure simulation.** Simulating each structure individually (tracking individual being assignments, per-structure economy, etc.) would be technically expensive and not perceptibly different to the player than aggregate simulation. Structures should contribute to aggregate stats (population, employment, tax revenue) without individual tracking.

2. **Realistic demand modeling.** The demand system should be intuitive, not realistic. Real urban economics is chaotic and often counter-intuitive. Game demand should follow clear cause-and-effect: more beings = more exchange demand. Players should be able to predict outcomes.

3. **Too many building templates too early.** Start with 23-28 templates (as recommended). More can always be added. Building 100 templates before testing visual variety is wasted effort.

4. **Over-engineering the redesignation system.** Keep density changes simple: old structure dissolves, new structure materializes. Do not try to simulate a continuous density gradient or partial upgrades.

---

## Appendix: Canonical Term Reference

For all agents working on Epic 4, here is the terminology mapping for all concepts in this document:

| This Document Uses | Canonical Alien Term | Human Equivalent |
|-------------------|---------------------|------------------|
| Colony | Colony | City |
| Being(s) | Being(s) | Citizen(s) |
| Overseer | Overseer | Player / Mayor |
| Habitation | Habitation | Residential |
| Exchange | Exchange | Commercial |
| Fabrication | Fabrication | Industrial |
| Sector | Sector | Tile / Plot |
| Sector Value | Sector Value | Land Value |
| Structure | Structure | Building |
| Dwelling | Dwelling | House |
| Hab-Spire | Hab-Spire | Apartment Tower |
| Market Pod | Market Pod | Shop |
| Exchange Tower | Exchange Tower | Office Building |
| Fabricator Pod | Fabricator Pod | Factory |
| Fabrication Complex | Fabrication Complex | Industrial Plant |
| Growth Pressure | Growth Pressure | RCI Demand |
| Designation | Designation | Zoning |
| Redesignation | Redesignation | Rezoning |
| Materializing | Materializing | Under Construction |
| Derelict | Derelict | Abandoned |
| Deconstructed | Deconstructed | Demolished |
| Pathway | Pathway | Road |
| Energy | Energy | Electricity |
| Fluid | Fluid | Water |
| Harmony | Harmony | Happiness |
| Disorder | Disorder | Crime |
| Contamination | Contamination | Pollution |
| Credits | Credits | Money |
| Tribute | Tribute | Tax |
| Cycle | Cycle | Year |
| Phase | Phase | Month |
| Chartering | Chartering | Purchasing (land) |
| Purge | Purge | Clear (terrain) |
| Substrate | Substrate | Flat Ground |
| Ridges | Ridges | Hills |
| Biolume Grove | Biolume Grove | Forest |
| Prisma Fields | Prisma Fields | Crystal Fields |
| Spore Flats | Spore Flats | Spore Plains |
| Blight Mires | Blight Mires | Toxic Marshes |
| Ember Crust | Ember Crust | Volcanic Rock |

---

*End of Game Designer Analysis for Epic 4: Zoning & Building System*
