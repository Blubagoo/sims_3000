# Game Designer Answers: Epic 16 Discussion

**Author:** Game Designer Agent
**Date:** 2026-02-01
**In Response To:** Systems Architect Questions (SA-1 through SA-6)

---

## SA-1: Auto-save frequency

**Question:** "Auto-save frequency: What's the appropriate interval for auto-saves? 5 minutes? 10 minutes? Should it be player-configurable?"

**Answer:**

Given my established position on continuous persistence, this question reframes slightly. ZergCity doesn't use traditional interval-based auto-saves - instead, we have **continuous database persistence** (every tick, 20/sec for entity changes) combined with **periodic checkpoint snapshots** for disaster recovery.

For the checkpoint system specifically, I recommend **every 10 cycles** (roughly 5-10 minutes of real time depending on game speed) for rolling checkpoints, with 3 retained. This balances recovery granularity against storage overhead. The interval should NOT be player-configurable for the core system - this is server infrastructure, not a gameplay preference. However, players can create **manual snapshots** (5 slots) whenever they want to capture a specific moment for experimentation purposes.

The cozy sandbox philosophy means players should never think about saving. Making auto-save intervals configurable would introduce decision fatigue and anxiety ("did I set it right?"). The system should just work invisibly.

---

## SA-2: Additional auto-save trigger events

**Question:** "Besides disasters and milestones, are there other events that should trigger auto-save? (e.g., first building placed, reaching population threshold)"

**Answer:**

Yes, several events warrant immediate checkpoint capture beyond disasters and milestones:

**High-Priority Triggers:**
- **First structure placed** - Protects the vulnerable early-game investment
- **Population thresholds** (every 10K beings) - Natural progress markers between formal milestones
- **Arcology construction complete** - Major resource investment deserves protection
- **Player joining/leaving world** - Captures state at session boundaries
- **Large economic transaction** (e.g., credit advance repayment, major purchase > 50K credits)

**Context-Aware Triggers:**
- **Before server restart** (planned maintenance) - Obvious protection
- **Idle detection** (no player input for 5+ minutes) - Capture before potential disconnect

I'd recommend against triggering on every small action - that defeats the purpose of batched efficiency. The philosophy is: protect meaningful progress moments, not every micro-action. Players should feel confident that "if something important happened, it's safe."

---

## SA-3: Save file browser statistics

**Question:** "What stats should be shown in the save file browser? Population, treasury, map size, play time, player names?"

**Answer:**

The world browser should show information that helps players make decisions about which world to join or resume. Here's my recommended hierarchy:

**Primary Display (Always Visible in Card):**
- World name and thumbnail preview
- Map size (e.g., "Standard Region 256x256")
- Total population (the core progress metric)
- Active players count (e.g., "3 of 4 Overseers")
- Last activity timestamp ("2 hours ago" / "3 days ago")

**Secondary Display (Expanded View):**
- World age in cycles
- Player names with their individual colony populations
- Creation date
- Your personal colony stats (population, treasury, tiles owned)
- Milestones achieved (aggregate and personal)

**What to Omit:**
- Total play time (endless sandbox makes this potentially anxiety-inducing)
- Detailed treasury amounts for other players (privacy, reduces comparison)
- Technical data (seed, file size, etc.)

The cozy multiplayer vibe means emphasizing collaboration and shared progress over competitive metrics. Show enough to orient players without creating FOMO or performance pressure.

---

## SA-4: Manual save naming conventions

**Question:** "Should manual saves have auto-generated names (timestamp-based) or always require user input? Should the default name be [World Name] - [Date]?"

**Answer:**

Manual snapshots should **auto-generate names with easy editing**. The default format I recommend:

**Format:** `[World Name] - Cycle [Number]`
**Example:** `Nexus Prime - Cycle 892`

This is more meaningful than timestamps because:
1. Cycles are the in-world time unit players track
2. Players don't think in real-world dates when playing
3. It connects the snapshot to game state, not wall-clock time

**UX Flow:**
1. Player clicks "Create Snapshot"
2. Name field pre-populated with default
3. Cursor positioned at end for easy addition ("Nexus Prime - Cycle 892 - before risky demolition")
4. Optional: one-click to confirm with default

For the **Immortalize** feature specifically, I'd prompt for a custom monument name since these are meant to be permanent and shareable. The default there could be just the colony name, encouraging players to give it a meaningful title.

---

## SA-5: Multiplayer load confirmation

**Question:** "When loading a save in an active multiplayer session, should there be a vote/confirmation from connected players, or is host authority sufficient?"

**Answer:**

This requires a nuanced approach based on rollback severity:

**Minor Rollbacks (< 30 minutes / 3 checkpoints):**
- Host authority is sufficient
- Other players receive notification: "Host restored to Cycle 1,234. Progress from the last 15 minutes has been reverted."
- No vote required - this is routine recovery

**Major Rollbacks (> 30 minutes):**
- Require confirmation from connected players (simple accept/decline)
- Majority vote (2 of 3, or 3 of 4) OR unanimous for rollbacks > 24 hours
- Declining players can disconnect and keep their local session active until resolution
- Timeout: 2 minutes to respond, then host decision stands

**Rationale:**
The cozy sandbox vibe requires that no single player can grief others by erasing hours of work. But the host needs authority for legitimate recovery (corruption, accidental disaster). The time threshold balances autonomy with protection.

**Edge Case:** If only the host is online, they have full authority regardless of rollback size - there's no one else to consult.

---

## SA-6: Share world / export feature

**Question:** "Should there be a 'share world' feature that exports a save file with certain data stripped (e.g., player sessions)? For sharing worlds on forums/Discord?"

**Answer:**

Yes, but I'd position this as **World Template Export** rather than "share save." The distinction matters for player expectations.

**What Gets Exported:**
- Full terrain and map seed
- All structures and infrastructure (buildings, pathways, zones)
- Named landmarks and custom designations
- Basic statistics (population at export, cycle count)

**What Gets Stripped:**
- Player identities and session data
- Treasury amounts (start fresh economically)
- Achievement/milestone progress (let new players earn their own)
- Disaster history and event logs
- Any private colony names

**Use Cases:**
1. **Showcase:** "Here's my beautiful city layout - try playing it yourself"
2. **Challenge:** "Can you manage this sprawling metropolis?"
3. **Learning:** "Study how I organized my industrial district"
4. **Templates:** "Here's a great starter layout for beginners"

**Format Consideration:**
Export should produce a single portable file (.zcworld?) that can be imported as a new world. It's essentially a "blank slate with infrastructure" - all the buildings are there, but it's a fresh start economically and progression-wise.

This aligns with the cozy sandbox philosophy: share creativity, not competition. Players can appreciate others' work and build on it without comparing stats.

---

*End of Game Designer responses. Ready for synthesis phase.*
