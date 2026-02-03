# Epic 12: Information & UI Systems - Game Designer Analysis

**Agent:** Game Designer
**Epic:** 12 - Information & UI Systems
**Date:** 2026-01-29
**Canon Version:** 0.12.0

---

## Player Experience Goals

### Primary Experience: Informed Empowerment

The UI exists to transform raw simulation data into **actionable insight**. Players should feel:

1. **In Control** - Never wondering "what's happening?" or "why did that fail?"
2. **Informed Without Overwhelm** - Right information at the right time
3. **Efficient** - Fast tool switching, minimal clicks to common actions
4. **Immersed** - UI reinforces alien theme without sacrificing clarity

### Emotional Targets by Context

| Context | Target Emotion | UI Behavior |
|---------|----------------|-------------|
| Building/Zoning | Flow state, creativity | Minimal interruption, responsive tools |
| Crisis (energy collapse, disaster) | Urgency + agency | Prominent warnings, quick-access solutions |
| Browsing stats | Curiosity, satisfaction | Clear trends, discoverable detail |
| Multiplayer awareness | Competition, connection | Visible rival progress, social presence |

### Moment-to-Moment Feel

- **Responsive**: Tool selection and placement should feel instant (< 50ms visual feedback)
- **Discoverable**: Hovering reveals information; clicking drills deeper
- **Consistent**: Same patterns everywhere (hover = preview, click = select/confirm)
- **Rewarding**: Positive feedback for good colony health, subtle but present

---

## Information Hierarchy

### Tier 1: Always Visible (Status Bar / Persistent HUD)

Information that answers "How is my colony doing RIGHT NOW?"

| Element | Alien Term | Purpose | Update Rate |
|---------|------------|---------|-------------|
| Credits | Credits | Financial health | Every phase |
| Population | Colony Population | Growth indicator | Every tick |
| Date | Cycle/Phase | Time progression | Every tick |
| Simulation Speed | Time Flow | Player control feedback | On change |
| Current Tool | Active Implement | What action is queued | On change |

**Design Note:** Keep this to 5-6 items max. Cognitive load increases sharply beyond this.

### Tier 2: On-Demand Summary (Sector Scan / Minimap Area)

Information that answers "Where should I focus attention?"

| Element | Alien Term | Purpose |
|---------|------------|---------|
| Sector Scan (minimap) | Sector Scan | Spatial orientation, quick navigation |
| Zone Pressure Bars | Growth Pressure Indicator | R/C/I demand at a glance |
| Alert Queue | Priority Alerts | Problems needing attention |

**Alert Priority Levels:**
1. **Critical (Red Pulse):** Grid collapse, conflagration, catastrophe in progress
2. **Warning (Amber Glow):** Energy deficit, disorder spike, contamination threshold
3. **Info (Cyan Flash):** Milestone reached, new cycle, structure completed

### Tier 3: Contextual (Data Readout / Info Panel)

Information that answers "What is this specific thing?"

Appears when:
- Player selects a tile/building (Query Tool)
- Player hovers over UI elements
- Player clicks for details on alerts

**Query Tool Data Readout Layers:**

```
STRUCTURE SELECTED:
  Name: "Hab-Cluster Beta-7"
  Type: High Density Habitation
  State: Active (Powered, Watered, Accessible)

TILE DATA:
  Sector Value: 78/100
  Disorder Index: 12/100
  Contamination: 3/100
  Energy Coverage: Yes (Nexus Alpha)
  Fluid Coverage: Yes (Extractor 2)
  Pathway Distance: 1 tile
```

### Tier 4: Deep Dive (Statistics Windows / Graphs)

Information for strategic planning. Player explicitly requests this.

- Population breakdown (by zone type, density)
- Economic breakdown (income vs. expense by category)
- Historical trends (graphs over cycles)
- Service coverage maps
- Ordinance management

**Design Principle:** These windows should be dismissable with a single key (ESC) and remember their last position.

---

## Tool Palette Design

### Command Array Organization

Tools should be grouped by **intent**, not by game system.

**Proposed Groupings:**

```
COMMAND ARRAY (Toolbar)
|
+-- BUILD (Primary Actions)
|   +-- Zone Designator (Habitation, Exchange, Fabrication)
|   +-- Infrastructure (Pathways, Energy Conduits, Fluid Conduits)
|   +-- Structures (Energy Nexus, Fluid Extractor, Service Buildings)
|
+-- MODIFY (Change Existing)
|   +-- Bulldoze / Deconstruct
|   +-- Upgrade (when applicable)
|   +-- Terrain Tools (Purge, Grade)
|
+-- INSPECT (Information)
|   +-- Query Tool (click for details)
|   +-- Scan Layers (overlays toggle)
|
+-- VIEW (Camera/Display)
|   +-- Speed Controls
|   +-- Camera Mode Toggle
|   +-- Overlay Quick-Toggle
```

### Keyboard Shortcuts (Must Be Identical in Both UI Modes)

| Key | Action | Rationale |
|-----|--------|-----------|
| 1-3 | Zone types (H/E/F) | Quick zone switching |
| R | Pathway (Road) | Most common infrastructure |
| P | Energy Conduit (Power) | Second most common |
| W | Fluid Conduit (Water) | Third most common |
| B | Bulldoze | Destructive action, easy reach |
| Q | Query Tool | Information access |
| TAB | Cycle overlays | Fast data switching |
| ESC | Cancel/Close | Universal dismiss |
| Space | Pause/Resume | Time control |
| +/- | Speed up/down | Time control |

### Tool Feedback

**Selection Feedback:**
- Selected tool highlighted in command array
- Cursor changes to reflect active tool
- Ghost preview shows placement validity (green = valid, red = invalid)

**Placement Feedback:**
- Valid: Bioluminescent cyan glow on affected tiles
- Invalid: Pulsing red outline, tooltip explains why
- Cost preview: Credits deduction shown before confirming

---

## Overlay System (Scan Layers)

### Available Scan Layers

| Scan Layer | Source Epic | Color Scheme | Purpose |
|------------|-------------|--------------|---------|
| Energy Coverage | Epic 5 | Blue gradient (bright = covered) | See power reach |
| Fluid Coverage | Epic 6 | Teal gradient (bright = covered) | See water reach |
| Transport Network | Epic 7 | White lines, yellow = congested | See traffic flow |
| Disorder Index | Epic 10 | Red gradient (dark = high crime) | Crime hotspots |
| Contamination Level | Epic 10 | Purple-yellow (yellow = high pollution) | Pollution spread |
| Sector Value | Epic 10 | Green-red (green = high value) | Land desirability |
| Service Coverage | Epic 9 | Separate per service type | See coverage gaps |

### Overlay Interaction Design

**Switching Mechanism:**
1. **Quick Toggle (TAB):** Cycles through most-used overlays: None -> Energy -> Fluid -> Disorder -> Contamination -> Value -> None
2. **Direct Select:** Click scan layer button in command array, or use number keys (Shift+1 through Shift+7)
3. **Multi-Select (Advanced):** Hold overlay button to see dropdown, shift-click to show multiple (e.g., disorder + service coverage)

**Visual Principles:**
- Overlays are **semi-transparent** - terrain and buildings remain visible
- **Legend always visible** when overlay active (corner HUD showing color scale)
- **Hover reveals exact value** at cursor position
- **Bioluminescent aesthetic** - overlays should glow, not just color-tint

**Cognitive Load Management:**
- Max 2 overlays simultaneously
- Clear visual distinction between overlays (different hue families)
- Option to disable overlay animations for performance/clarity

---

## UI Mode Considerations

### Legacy Interface (Classic Mode)

**Ideal For:** New players, fans of SimCity 2000, keyboard-heavy workflows

**Layout Principles:**
- Fixed panel positions (predictable)
- Dense information display (all stats visible)
- Mouse-centric with keyboard shortcuts
- Opaque panels with alien-themed borders (bioluminescent accents on dark backgrounds)

**Risk:** May feel dated. Mitigate with bioluminescent color palette and alien iconography.

### Holo Interface (Holographic Mode)

**Ideal For:** Immersion seekers, players who prefer minimalist HUD, streamers/content creators

**Layout Principles:**
- Floating, context-sensitive panels
- Radial menus for tool selection
- Minimal persistent HUD (information appears on demand)
- Translucent panels with scan-line effects, hologram flicker

**Risk:** Information may be harder to find. Mitigate with consistent reveal patterns (hover = info appears).

### Shared Across Both Modes

- Same keybindings
- Same data displayed (just presented differently)
- Same sound effects for actions
- Toggle via settings OR hotkey (suggest F1)
- Default: Legacy Interface (lower barrier to entry)

---

## Multiplayer UI Considerations

### Player Presence Indicators

| Element | Purpose | Display |
|---------|---------|---------|
| Cursor Icons | See where others are working | Colored cursor per player |
| Activity Feed | Know what others are building | Toast notifications |
| Player List | Quick status overview | Expandable panel |
| Territory Borders | Clear ownership | Color-coded boundary lines |

### Competitive Information

**Visible to All:**
- Other players' population (encourages competition)
- Other players' territory size
- Milestone progress indicators

**Hidden:**
- Treasury balance (strategic information)
- Exact service coverage
- Internal crisis details (unless visible in their territory)

### Social Features

- In-game chat (text, possibly quick-emotes)
- Ping system (click map to highlight location for others)
- Victory condition tracker (when enabled)

---

## Questions for Other Agents

### @systems-architect

1. **Overlay Data Access:** How will UISystem query overlay data from multiple systems (DisorderSystem, ContaminationSystem, etc.)? Is IGridOverlay sufficient, or do we need per-system query methods?

2. **UI State Sync:** In multiplayer, does any UI state need to sync? (E.g., should all players see when host opens budget window, or is UI entirely local?)

3. **Performance Budget:** What's the render budget for UI? Should overlays be rendered to texture and cached, or recomputed each frame?

4. **Event Notifications:** How does UISystem subscribe to game events (building completed, disaster started, milestone reached)? Is there an event bus, or direct observer pattern?

### @ui-developer

1. **Panel Framework:** Do we have a base panel/widget system planned, or will each UI element be custom?

2. **Animation System:** For holographic mode's scan-line and flicker effects, do we need a shader-based UI, or can this be sprite-based?

3. **Localization:** Any plans for multi-language support? This affects text layout significantly.

4. **Accessibility:** Should we support colorblind modes for overlays? High-contrast option for text?

---

## Alien Theme Notes

### Terminology Enforcement

All player-facing text MUST use canonical alien terms:

| Human Term | Alien Term (USE THIS) |
|------------|----------------------|
| Minimap | Sector Scan |
| Toolbar | Command Array |
| Info Panel | Data Readout |
| Overlay | Scan Layer |
| Status Bar | Colony Status |
| Query Tool | Probe Function |
| Building | Structure |
| Road | Pathway |
| Power | Energy |
| Water | Fluid |
| Tax | Tribute |
| Budget | Treasury |
| Crime | Disorder |
| Pollution | Contamination |
| Land Value | Sector Value |
| Happiness | Harmony |

### Visual Theme Guidelines

**Bioluminescent Palette in UI:**
- Dark backgrounds (#0a0a12 base)
- Cyan/teal highlights for interactive elements
- Amber/orange for warnings
- Magenta for special/premium items
- Green for positive states (healthy, profitable)
- Red for negative states (crisis, loss)

**Alien Iconography:**
- Geometric, crystalline shapes
- Organic curves mixed with sharp angles
- Avoid human symbols (no $ sign - use credit symbol)
- Consider alien glyphs as decorative accents (not for critical info)

**Audio Feedback:**
- Soft crystalline chimes for UI interactions
- Deeper tones for warnings
- Satisfying "lock-in" sound for successful placements

---

## Summary Priorities

### P0 (Core Experience - Must Have)

1. Command Array with tool groupings
2. Query Tool (Data Readout) for clicking structures
3. Colony Status bar (credits, population, date, speed)
4. Sector Scan (minimap) with navigation
5. At least 4 scan layers (Energy, Fluid, Disorder, Sector Value)
6. Alert system for critical events

### P1 (Enhanced Experience - Should Have)

1. Holographic mode toggle
2. Statistics windows (population, economy graphs)
3. Full overlay suite (all 7 scan layers)
4. Multiplayer player presence indicators
5. Keyboard shortcut system

### P2 (Polish - Nice to Have)

1. Newspaper/Chronicle system
2. Overlay blending (multiple simultaneous)
3. Advanced statistics (historical comparison)
4. Custom keybinding
5. Accessibility features (colorblind modes)

---

*This analysis establishes player experience goals for Epic 12. Technical implementation details require input from @systems-architect and @ui-developer.*
