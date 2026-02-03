# Game Designer Analysis: Epic 16 - Persistence

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.16.0
**Status:** Initial Analysis

---

## Executive Summary

Persistence in ZergCity serves a fundamentally different purpose than in single-player games. In a multiplayer endless sandbox, saving isn't about capturing a "moment in time" to reload later - it's about **world continuity** and **player reconnection**. The colony lives on the server; persistence ensures that world survives restarts, crashes, and the passage of real time.

The "Immortalize" feature from Core Principle #4 transforms persistence from a technical necessity into a creative feature - allowing players to freeze their achievements as permanent monuments across the ZergCity multiverse.

This epic must deliver: **seamless reconnection**, **resilient world state**, **meaningful save metadata**, and the **immortalize celebration feature**.

---

## 1. Player Experience Goals

### 1.1 Core Experience Pillars

| Pillar | Description | Why It Matters |
|--------|-------------|----------------|
| **Invisible Persistence** | Players should never think about saving | Endless sandbox = always saved |
| **Confident Reconnection** | "My colony is exactly as I left it" | Trust in the system |
| **World History** | See how the colony evolved over time | Emotional connection to progress |
| **Immortalize Moments** | Freeze peak achievements permanently | Celebrate without ending |
| **Graceful Recovery** | Corruption never loses everything | Protect player investment |

### 1.2 Player Mindset by Context

**During Normal Play:**
- "I never think about saving - it just happens"
- "I can close the game anytime without worry"
- "The server can restart and I lose nothing"

**When Reconnecting:**
- "My colony continued while I was away - interesting!"
- "Everything is exactly where I left my cursor"
- "My friends' colonies are here too"

**When Something Goes Wrong:**
- "Okay, we lost a few cycles - that's fine"
- "The backup saved our progress from yesterday"
- "My immortalized snapshot is still safe"

**When Immortalizing:**
- "I want to capture this moment forever"
- "This is my colony at its peak - before the disaster"
- "Other players can visit my frozen monument"

---

## 2. Auto-Save Design

### 2.1 Continuous vs. Interval Saving

**ZergCity uses CONTINUOUS database persistence, not periodic saves.**

This is a fundamental architectural difference from traditional games:

| Traditional Save | ZergCity Persistence |
|------------------|---------------------|
| Player initiates save | System continuously writes to DB |
| Save file = full snapshot | DB = live game state |
| "Did I save?" anxiety | Never think about it |
| Lost progress on crash | Minimal loss (last tick) |

### 2.2 Why Continuous Persistence?

1. **Multiplayer Reality:** With 2-4 players, you can't ask everyone to "wait while I save"
2. **Server Restarts:** Server can restart for updates; all state is in DB
3. **Endless Sandbox:** No natural "save points" in infinite play
4. **Reconnection Model:** Players disconnect/reconnect; state must persist

### 2.3 What Gets Saved When

| Data Type | Persistence Timing | Rationale |
|-----------|-------------------|-----------|
| Entity component changes | Every tick (20/sec) | Authoritative state |
| Player actions (build, zone) | Immediate | Confirm before response |
| Economic transactions | Immediate | Credits must be accurate |
| Milestone achievements | Immediate | Never lose achievements |
| Dense grid data | Every N ticks (5-10) | Bulk efficiency |
| Statistics/history | Every cycle (game month) | Aggregate data |

### 2.4 Checkpoint System (Backup Snapshots)

**In addition to continuous writes, periodic full snapshots:**

| Checkpoint Type | Frequency | Retention | Purpose |
|----------------|-----------|-----------|---------|
| **Rolling Checkpoint** | Every 10 cycles | Keep last 3 | Quick recovery |
| **Daily Backup** | Real-world day | Keep 7 days | Disaster recovery |
| **Milestone Snapshot** | On milestone achieve | Permanent | Historical reference |
| **Manual Snapshot** | Player-triggered | 5 per player | "Save state" for experimentation |

### 2.5 Player-Facing Checkpoint Experience

**No "Save" Button in Normal Play**

Instead, the UI shows:
- Status indicator: "Colony State: Synchronized" (always visible, subtle)
- Last checkpoint: "Last backup: 3 cycles ago" (in settings/info panel)
- Manual snapshot: "Create Snapshot" button (limited uses, explained below)

**Manual Snapshots - Limited Experimentation**

Players get 5 manual snapshot slots for experimentation:
- "I want to try demolishing this district"
- "Let me save before I take out this credit advance"
- Creates a full state capture they can restore to

**Restore Experience:**
1. Player opens Colony Archives panel
2. Sees list of snapshots with metadata
3. Selects snapshot to preview (shows population, credits, cycle)
4. Confirms restore: "Restore to Cycle 247? Current progress will be lost."
5. Server restores state, all clients reconnect to new state

---

## 3. Save Slot UX

### 3.1 World-Centric, Not Slot-Centric

**Traditional games:** "Slot 1, Slot 2, Slot 3..."
**ZergCity:** "Worlds are containers; snapshots are within worlds"

```
Colony Archives
================
[*] World: "Nexus Prime" (Active - 4 players)
    Created: 2026-01-15
    Last Activity: 2 minutes ago
    Map Size: 256x256
    Total Population: 147,832
    [Snapshots: 3 of 5 used]

[ ] World: "Crystal Shores" (Offline)
    Created: 2026-01-08
    Last Activity: 3 days ago
    Map Size: 128x128
    Total Population: 23,491
    [Snapshots: 1 of 5 used]

[ ] World: "Test World" (Offline)
    Created: Yesterday
    Map Size: 512x512
    Total Population: 0
    [Snapshots: 0 of 5 used]

[+ Create New World]
```

### 3.2 World Naming Conventions

**Auto-Generated Names (Player Can Edit):**
- Algorithm: `[Terrain Feature] + [Celestial/Abstract]`
- Examples: "Crystal Expanse", "Spore Nexus", "Void Basin", "Ember Fields"
- Uses seed-based generation for consistent suggestions

**Naming Rules:**
- Max 32 characters
- Alphanumeric + spaces + basic punctuation
- No profanity filter (private servers, trust players)
- Names must be unique per player's world list

### 3.3 Multiplayer World Display

**When browsing worlds:**
```
World: "Nexus Prime"
========================================
Created by: OverSeer_Alpha (Host)
Players:
  - OverSeer_Alpha (2 days active)
  - ZergMaster99 (Online now)
  - ColonyBuilder3 (Last seen: 1 hour ago)
  - [Slot Available]

Your Colony: "Northern Reach"
  Population: 34,521
  Cycle: 892
  Last Played: 2 hours ago
```

### 3.4 Joining Existing Worlds

**First-Time Join Experience:**
1. Browse available worlds (server browser or invite link)
2. Preview world: map size, current players, world age
3. Accept invite or request to join
4. Spawn at emergence_site (spawn point)
5. Begin with starting credits, no tiles owned

**Returning Player Experience:**
1. Select world from "My Worlds" list
2. See colony preview (minimap, stats)
3. Click "Reconnect"
4. Load directly into colony view (camera at last position)
5. Notification: "Welcome back! 47 cycles have passed."

---

## 4. World Information Display

### 4.1 World Summary Card

**What players see when browsing worlds:**

```
+------------------------------------------+
|  [World Thumbnail - isometric preview]   |
|                                          |
|  NEXUS PRIME                             |
|  Map: Standard Region (256x256)          |
|  Age: 1,247 cycles                       |
|  Population: 147,832 beings              |
|  Overseers: 3 of 4                       |
|                                          |
|  [Last played: 2 hours ago]              |
+------------------------------------------+
```

### 4.2 Detailed World Info Panel

**Accessible from world selection or in-game:**

| Section | Information | Purpose |
|---------|-------------|---------|
| **Overview** | Name, creation date, map size, seed | Identity |
| **Population** | Total, per-player breakdown, growth trend | Social context |
| **Economy** | Total credits in circulation, trade volume | Economic health |
| **Infrastructure** | Total pathways, energy/fluid coverage % | Development level |
| **Achievements** | Milestones reached (aggregate), monuments built | Progress markers |
| **History** | Major events log (disasters, transcendence) | Narrative |

### 4.3 Your Colony Info Card

**Personal statistics within a world:**

```
YOUR DOMAIN: Northern Reach
================================
Population: 34,521 beings
  - Trend: +12% last 100 cycles
Tiles Owned: 847 sectors
Treasury: 234,891 credits

Milestones:
  [x] Colony Emergence (2K)
  [x] Colony Establishment (10K)
  [x] Colony Identity (30K)
  [ ] Colony Security (60K) - 58% progress

Active Edicts: 2 of 3 slots
  - Welcoming Presence
  - Trade Magnate

Arcologies: 0
  - Unlock at: 120K beings
```

### 4.4 World History Log

**Chronicle of major events:**

```
WORLD HISTORY: Nexus Prime
===========================
Cycle 1,203: OverSeer_Alpha achieved Transcendence
Cycle 1,187: Seismic Event struck Western Ridge
Cycle 1,102: ZergMaster99 reached Colony Wonder (90K)
Cycle 892: ColonyBuilder3 joined the world
Cycle 756: First Arcology constructed
Cycle 501: OverSeer_Alpha reached Colony Identity (30K)
Cycle 247: World created by OverSeer_Alpha
...
[Show More]
```

---

## 5. Immortalize Feature

### 5.1 Core Concept

**"Immortalize" freezes a colony's peak moment as a permanent, visitable monument.**

This is NOT a save file - it's a **frozen snapshot that becomes a location in the ZergCity multiverse**.

**Why Immortalize Exists:**
1. Core Principle #4: "Optional 'immortalize' feature to freeze a city as a monument"
2. Endless sandbox has no natural endings - immortalize provides closure without ending
3. Celebrates achievement without removing the colony from active play
4. Creates shareable content for the community

### 5.2 When to Immortalize

**Player Motivations:**

| Scenario | Player Thinking |
|----------|-----------------|
| **Peak Achievement** | "My colony just hit Transcendence - capture this forever" |
| **Before Disaster** | "I know a vortex_storm is coming - freeze this moment" |
| **Moving On** | "I'm starting fresh, but I want this preserved" |
| **Sharing** | "Friends should be able to visit what I built" |
| **Time Capsule** | "In a cycle, I'll compare then vs now" |

### 5.3 Immortalize vs. Snapshot

| Feature | Manual Snapshot | Immortalize |
|---------|-----------------|-------------|
| **Purpose** | Experimentation backup | Permanent monument |
| **Limit** | 5 per player | Unlimited (costs credits) |
| **Restoration** | Yes - replaces current state | No - read-only |
| **Visibility** | Private | Optional public gallery |
| **Persistence** | Deleted with world | Survives world deletion |
| **Interactivity** | None (data only) | Visitable as read-only world |

### 5.4 Immortalize Experience Flow

**Step 1: Initiate Immortalize**
- Player opens Colony Archives
- Selects "Immortalize Colony"
- Cost displayed: 50,000 credits (significant but affordable at milestone stage)

**Step 2: Capture Configuration**
```
IMMORTALIZE YOUR COLONY
========================
Colony Name: Northern Reach
Current Cycle: 892
Population: 34,521 beings

What will be captured:
  - All structures and terrain
  - All statistics and history
  - Current camera position (as default view)
  - Your colony only (not other players')

Visibility:
  ( ) Private - Only you can visit
  (*) Friends - Invited overseers can visit
  ( ) Public - Listed in Monument Gallery

Confirm Cost: 50,000 credits
[Cancel]  [Immortalize]
```

**Step 3: Capture Animation**
- Screen briefly shimmers with ethereal effect
- "Capturing moment in time..."
- Progress bar (2-3 seconds)
- Triumphant tone on completion
- "Your colony has been immortalized!"

**Step 4: Access Immortalized Colonies**
- Listed in new "Monuments" section of Colony Archives
- Each shows thumbnail, stats, creation date
- Click to "Visit" - loads as read-only world

### 5.5 Visiting Immortalized Colonies

**The Visitor Experience:**

1. **Select Monument** from gallery or direct link
2. **Loading:** "Entering Monument: Northern Reach - Cycle 892"
3. **Camera:** Opens at captured camera position
4. **Mode:** Read-only exploration
   - Camera works normally (pan, zoom, rotate)
   - Query tool works (inspect any structure)
   - No building, zoning, or modification
   - UI shows "MONUMENT MODE" indicator
5. **Information Overlay:**
   - Original overseer name
   - Capture date and cycle
   - Population at capture
   - Milestones achieved
6. **Exit:** "Leave Monument" returns to main menu

### 5.6 Monument Gallery (Public Immortalized Worlds)

**Community Feature - Optional Public Sharing:**

```
MONUMENT GALLERY
================
[Search by name, population, features...]

Featured Monuments:
+------------------------------------------+
|  "The Spire of Eternity"                |
|  by OverSeer_Prime                       |
|  Captured at: 1.2M beings               |
|  Features: Transcendence Monument        |
|  Visits: 2,847                          |
|  [Visit]                                |
+------------------------------------------+
|  "Crystal Paradise"                      |
|  by ZergArchitect                        |
|  Captured at: 450K beings               |
|  Features: 12 Arcologies                |
|  Visits: 1,203                          |
|  [Visit]                                |
+------------------------------------------+

[Browse All] [Most Visited] [Newest] [My Monuments]
```

### 5.7 Immortalize as Closure Mechanism

**Psychological Design:**

In an endless sandbox, some players want closure. Immortalize provides:
- A "finished" feeling without ending the world
- Permission to move on ("this is preserved")
- Legacy creation ("others can see what I built")
- Emotional capstone for long play sessions

**Important:** Immortalizing does NOT end the active colony. The live world continues; the monument is a frozen copy.

---

## 6. Error Handling UX

### 6.1 Design Philosophy

**"Players should never feel afraid of data loss."**

Errors are inevitable. The system must:
1. Prevent catastrophic loss (multiple redundancy)
2. Communicate clearly what happened
3. Offer recovery options
4. Never blame the player

### 6.2 Connection Loss Scenarios

| Scenario | Player Experience | System Response |
|----------|-------------------|-----------------|
| **Brief Disconnect** | Screen dims briefly, "Reconnecting..." | Auto-reconnect within 30 sec |
| **Server Restart** | "Server restarting for update. Please wait." | Hold on title screen, auto-rejoin |
| **Extended Outage** | "Server unavailable. Your colony is safe." | Show last known state, retry periodically |
| **Client Crash** | Relaunches normally | Server has authoritative state |

### 6.3 Corruption Recovery

**Corruption Tiers:**

| Tier | What Happened | Recovery |
|------|---------------|----------|
| **Minor** | Single component corrupted | Auto-repair from redundant data |
| **Moderate** | Entity state inconsistent | Rollback to last checkpoint (minutes) |
| **Major** | Database segment corrupt | Rollback to daily backup (hours) |
| **Catastrophic** | Full database corruption | Rollback to oldest backup, notify players |

### 6.4 Recovery Communication

**Example: Moderate Corruption Recovery**

```
COLONY RESTORATION NOTICE
==========================
We detected an issue with the world state and
performed an automatic recovery.

World: Nexus Prime
Restored to: Cycle 1,234 (12 minutes ago)

What this means:
  - Any actions in the last 12 minutes may need to be repeated
  - Your milestones and achievements are safe
  - Other players' progress is similarly affected

This is rare and we apologize for the inconvenience.
Your colony is now safe and stable.

[Continue]
```

### 6.5 Player-Initiated Recovery

**Colony Archives > Recovery Options:**

```
RECOVERY OPTIONS
================
Current State: Cycle 1,247

Available Recovery Points:
  1. [Rolling Checkpoint] Cycle 1,237 - 10 minutes ago
  2. [Rolling Checkpoint] Cycle 1,227 - 20 minutes ago
  3. [Rolling Checkpoint] Cycle 1,217 - 30 minutes ago
  4. [Daily Backup] Cycle 1,103 - Yesterday 3:00 PM
  5. [Milestone] Cycle 892 - Colony Identity achieved

Warning: Recovery affects ALL players in this world.
Host approval required for recovery beyond 30 minutes.

[Cancel] [Recover to Selected Point]
```

---

## 7. Questions for Other Agents

### 7.1 For Systems Architect

1. **Continuous Persistence Implementation:**
   - What database technology supports 20 writes/second with transactional integrity?
   - How are component changes batched for efficient DB writes?
   - Is there a write-ahead log for crash recovery?

2. **Checkpoint Storage:**
   - How large is a full world snapshot for each map size?
   - What compression is used for checkpoint files?
   - Where are checkpoints stored (local, cloud, both)?

3. **Immortalize Storage:**
   - Are immortalized worlds stored in the same DB or separate?
   - How is the Monument Gallery indexed and searched?
   - What's the storage cost per immortalized colony?

4. **Multiplayer State Synchronization:**
   - When server restores from backup, how are all clients synchronized?
   - Is there a "state hash" for quick corruption detection?
   - How is recovery coordinated across connected clients?

### 7.2 For UI Developer

5. **Colony Archives Interface:**
   - What's the visual design for the world browser?
   - How is Monument Gallery integrated into the UI flow?
   - What animations accompany the Immortalize capture?

6. **Status Indicators:**
   - How is "Colony State: Synchronized" displayed without being intrusive?
   - What visual treatment for Monument Mode (read-only viewing)?

### 7.3 For QA Engineer

7. **Corruption Testing:**
   - How do we simulate various corruption scenarios?
   - What's the test matrix for recovery paths?
   - How do we validate checkpoint integrity?

8. **Edge Cases:**
   - What happens if Immortalize is triggered during disconnect?
   - How do we handle recovery during an active disaster event?

---

## 8. Recommendations

### 8.1 MVP Features (Must Have)

| Feature | Priority | Rationale |
|---------|----------|-----------|
| Continuous database persistence | P0 | Core architecture |
| Rolling checkpoints | P0 | Quick recovery |
| Basic world browser | P0 | World selection UX |
| Connection loss handling | P0 | Player trust |
| World info display | P1 | Context for players |

### 8.2 Full Release Features

| Feature | Priority | Rationale |
|---------|----------|-----------|
| Manual snapshots (5 slots) | P1 | Experimentation support |
| World history log | P1 | Narrative engagement |
| Daily backups | P1 | Disaster recovery |
| Player-initiated recovery | P2 | Player agency |

### 8.3 Post-Release / Deferred

| Feature | Priority | Rationale |
|---------|----------|-----------|
| Immortalize feature | P2 | Celebration feature |
| Monument Gallery | P3 | Community feature |
| Public monument browsing | P3 | Social discovery |
| Monument statistics/visits | P3 | Engagement metrics |

### 8.4 Key Design Decisions

1. **No explicit "Save" button** - Continuous persistence means always saved
2. **World-centric organization** - Players think in worlds, not slots
3. **Immortalize costs credits** - Prevents frivolous use, feels significant
4. **Monuments are read-only** - Preserves the "frozen in time" concept
5. **Recovery is host-gated** - Prevents griefing via rollbacks

### 8.5 Canon Update Proposals

#### Terminology Additions

```yaml
persistence:
  save_game: chronicle_capture          # Manual snapshot
  load_game: chronicle_restore          # Restore from snapshot
  auto_save: continuous_sync            # Background persistence
  save_slot: world_chronicle            # World-level container
  checkpoint: restoration_point         # Recovery checkpoint
  backup: archive_copy                  # Full backup

immortalize:
  immortalize: crystallize              # Freeze as monument
  monument: eternal_colony              # Immortalized colony
  monument_gallery: hall_of_eternity    # Public gallery
  visit_monument: witness_eternity      # View immortalized world

recovery:
  corruption: data_anomaly              # Corruption event
  recovery: chronicle_restoration       # Recovery process
  rollback: temporal_reversion          # State rollback
```

#### UI Terminology

```yaml
ui:
  colony_archives: colony_chronicles    # Main persistence UI
  world_browser: realm_navigator        # World selection interface
  snapshot: moment_capture              # Manual save point
  restore: chronicle_recall             # Restore operation
```

---

## 9. Summary

Epic 16 transforms save/load from a mechanical feature into an experience that supports ZergCity's endless sandbox philosophy:

**Continuous Persistence** eliminates save anxiety - the world is always preserved.

**World Organization** treats multiplayer worlds as living entities, not save files.

**Immortalize** provides emotional closure without endings, letting players freeze peak moments as visitable monuments.

**Graceful Recovery** protects player investment through layered redundancy.

The system should be invisible during normal play, reassuring during disruption, and celebratory during immortalization. Players should trust that their colonies are safe, their progress is permanent, and their achievements can last forever.

---

*This analysis focuses on player experience and emotional design. Technical implementation details require Systems Architect input.*
