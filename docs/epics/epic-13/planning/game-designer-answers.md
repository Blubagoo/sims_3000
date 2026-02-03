# Game Designer Answers: Epic 13 Questions

**Author:** Game Designer Agent
**Date:** 2026-02-01
**Canon Version:** 0.13.0
**Status:** ANSWERS COMPLETE

---

## Answers to @systems-architect

### Q1: Disaster Frequency Balance

**Question:** How often should disasters occur? Proposed: ~1 minor disaster per 50 cycles, ~1 major disaster per 200 cycles. Is this appropriate for a "cozy sandbox" game?

**Answer:** The proposed frequency is too high for our cozy sandbox vibe.

**Recommended frequencies:**
- **Minor catastrophes:** ~1 per 100 cycles (very occasional)
- **Major catastrophes:** ~1 per 500 cycles (rare)
- **Catastrophic events:** ~1 per 1000+ cycles (once-per-playthrough memorable)

**Rationale:** Canon establishes this as an "endless sandbox" with "social/casual vibe" (CANON.md, Core Principles 4). Disasters should be memorable punctuation marks, not constant interruptions. When players think about their session, they should remember "that one time the vortex_storm hit" as a story-worthy event, not "ugh, another conflagration."

**Important modifier:** These frequencies should scale with player settings. "Relaxed" mode could halve these rates, while "Challenging" mode could use the originally proposed values for players who want more tension.

---

### Q2: Disaster Severity Scaling

**Question:** Should disasters scale with population/city size, or be fixed severity?

**Answer:** **Hybrid approach** - base severity is random, but effect area scales with colony footprint.

**Design:**
1. **Severity (damage intensity):** Fixed per disaster type, modified by random roll
2. **Area of effect:** Scales with colony size - larger colonies have larger target area, so statistically more gets hit
3. **Response capacity:** Larger colonies naturally have more service coverage

**Why this works:**
- Small colony: Small conflagration might hit 30% of their buildings (feels devastating but recoverable)
- Large colony: Same conflagration might hit 5% of their buildings (less percentage but more absolute damage)

This creates natural "growing pains" - early colonies feel catastrophes more keenly, but have less total loss. Larger colonies lose more stuff absolutely but it's a smaller percentage. The experience stays proportional.

**Exception:** Titan emergence and void anomaly should scale with colony size - a titan wouldn't bother with a 50-being settlement.

---

### Q3: Player-Triggered Disasters

**Question:** Should players be able to trigger disasters intentionally? Some city builders allow "disaster mode" for fun. Fits our sandbox vibe?

**Answer:** **Yes, absolutely.** This is core to sandbox expression.

**Implementation:**
1. **Self-only in multiplayer:** You can trigger catastrophes in YOUR colony only. Never target other overseers.
2. **Sandbox menu option:** "Catastrophe Menu" available via command nexus
3. **No penalty:** Triggered catastrophes don't count against statistics or affect any scoring
4. **Full selection:** All catastrophe types available once unlocked via progression

**Why this fits:**
- Sandbox players love "what if" scenarios
- Creates emergent storytelling ("I wanted to see if my hazard response could handle a core breach")
- Classic SimCity feature that players expect
- Aligns with "joy of creating something unique" experience pillar

**Thematic framing:** Frame it as "Catastrophe Simulation Training" from the command nexus - the Overseer is running drills. Makes it feel in-universe rather than breaking immersion.

---

### Q4: Cross-Boundary Damage Responsibility

**Question:** If Player A's fire spreads to Player B's colony, who pays for rebuilding? Should there be compensation mechanics?

**Answer:** **Each overseer pays for their own damage.** No compensation mechanics.

**Rationale:**
1. **Natural disasters are nobody's fault** - Catastrophes that cross boundaries are acts of the cosmos, not player actions
2. **Compensation creates griefing vectors** - If there were compensation, players could game it
3. **Simplicity matters** - Tracking damage origin across boundaries adds complexity for little gameplay value
4. **Cooperation over compensation** - The social fun comes from helping each other respond, not from bureaucratic credit transfers

**What we DO support instead:**
- **Mutual aid:** Players can voluntarily send credits to affected neighbors (already in my analysis)
- **Shared response:** Hazard responders can cross boundaries to fight fires cooperatively
- **Empathy notifications:** "Overseer X's colony is experiencing a conflagration near your border" - prompts cooperative response

This turns cross-boundary disasters into cooperation opportunities rather than liability disputes.

---

### Q5: Disaster Immunity Options

**Question:** Should players be able to disable disasters entirely in sandbox mode? Or is disaster risk core to the experience?

**Answer:** **Yes, full disable option must exist.** Disaster risk is NOT core to this game's identity.

**Settings structure:**
```yaml
catastrophe_settings:
  enabled: true/false           # Master toggle - disables ALL random catastrophes
  random_events: true/false     # Just random triggers (manual still works)
  manual_trigger: true/false    # Sandbox trigger option
  severity_cap: minor/major/catastrophic/extinction
  frequency: off/rare/occasional/frequent
```

**Rationale:**
Per canon: "This is NOT a competitive game with victory conditions. Endless play - build, prosper, hang out with friends."

Some players want pure creative building. They should have that option. Other players want tension and challenge. They should have THAT option. Neither is wrong.

**Default:** `enabled: true`, `frequency: rare` - disasters exist but are infrequent. Players who want more can increase; players who want peace can disable.

---

### Q6: Titan Emergence Behavior

**Question:** How should the monster behave? Should titans target specific building types?

**Answer:** **Titans should be semi-intelligent threats that create interesting decisions.**

**Behavior design:**
1. **Emergence:** Titan rises from deep terrain (ember_crust, prisma_fields) - location telegraphed by pre-emergence rumbling
2. **Target priority:**
   - Primary: Energy nexuses (drawn to energy signatures)
   - Secondary: High-density structures (more activity = more interesting)
   - Tertiary: Command nexus (if no other high-value targets)
3. **Movement:** Semi-random path with "magnetism" toward targets. Not perfect pathfinding - titans are creatures, not missiles
4. **Duration:** 45-90 seconds of rampage, then retreat to emergence point and disappear

**Why this design:**
- **Creates decisions:** Protect your energy nexus? Let it take damage to save other structures?
- **Visual spectacle:** Titan heading toward the glowing nuclear nexus is dramatic
- **Story potential:** "The titan was heading straight for my exchange district but got distracted by the solar field"
- **Not griefable:** Titan targets high-value structures, not other players' colonies preferentially

**Personality note:** Titans aren't evil - they're native creatures disturbed by colony activity. Frame them as majestic and terrifying, not malicious. This fits our alien theme better than "city-destroying monster."

---

### Q7: Void Anomaly Effects

**Question:** What specific effects should void anomalies have? Reality distortion needs gameplay meaning.

**Answer:** Void anomalies should be mechanically unique and thematically memorable.

**Proposed effects (pick 2-3 per occurrence):**

| Effect | Gameplay Impact | Visual |
|--------|-----------------|--------|
| **Spatial Displacement** | Random structures teleported 5-15 tiles | Buildings blink, reappear elsewhere |
| **Temporal Stutter** | Affected zone pauses for 10-20 ticks | Time-frozen shimmer effect |
| **Reality Tear** | Creates temporary "void zone" that damages anything | Dark rift with chromatic aberration |
| **Energy Drain** | Affected area loses power for duration | Lights flicker, go dark |
| **Population Displacement** | Beings in zone scatter to other districts | Beings phase out, reappear elsewhere |
| **Infrastructure Scramble** | Pathways/conduits temporarily disconnect | Connections flicker |

**How to use them:**
- **Minor void anomaly:** 1 effect, small area, 10-15 seconds
- **Major void anomaly:** 2-3 effects, larger area, 20-30 seconds
- **Cosmic-level void anomaly:** All effects, massive area, potentially permanent reality tear zone

**Why this works:**
- Each effect creates different gameplay challenges (displaced building = reconnect utilities, energy drain = priority management)
- Highly visual and memorable - unlike anything in other city builders
- Thematically perfect for "alien world under strange cosmic influences"
- Not just damage - creates interesting puzzles to solve

**Recovery:** Displaced structures keep their zone designation. Beings eventually return. Permanent "void zones" could become a terrain feature (unbuildable, slowly shrinks over cycles).

---

### Q8: Recovery Time Balance

**Question:** How quickly should zones rebuild after disaster? Immediate or lengthy recovery?

**Answer:** **Recovery should be faster than initial build, but not instant.** The arc matters.

**Recovery timeline:**

| Phase | Duration | Player Action |
|-------|----------|---------------|
| **Damage Assessment** | Instant | Survey overlay shows damage |
| **Debris Clearance** | 5-10 ticks per structure | Auto-clear (slow) OR manual clear (faster, costs credits) |
| **Zone Retains Designation** | Immediate | Zone persists - no re-zoning needed |
| **Utility Reconnection** | 2-5 ticks | Auto-reconnects if adjacent utilities intact |
| **Building Reconstruction** | 50% of original construction time | Normal development with demand |

**Key design principles:**

1. **Recovery is faster than building from scratch** - 50% construction time discount, no re-zoning, no land purchase
2. **Player has agency** - Can spend credits to speed debris clearance
3. **Not punishing** - The disaster was the punishment; recovery is the satisfaction
4. **Visual progress** - See the debris being cleared, new structures rising

**Why not instant?**
Instant recovery removes the "earned" feeling. The emotional arc requires seeing your colony damaged, then watching it heal. That progression from crisis to restored is satisfying.

**Why not lengthy?**
Lengthy recovery becomes frustrating busywork. Players already experienced the disaster - don't make them suffer through a long rebuild. Get them back to playing.

**Sweet spot:** Most recoveries should complete within 2-5 minutes of real time. Long enough to feel meaningful, short enough not to frustrate.

---

## Answers to @services-engineer

### Q5: Response Radius Values

**Question:** Should response radius be significantly larger than coverage radius? Proposal is 1.5x coverage radius.

**Answer:** **Yes, 1.5x feels right.** Maybe even 2x for nexuses.

**Reasoning:**
- **Coverage radius** = where the service passively provides benefits (always active)
- **Response radius** = how far they'll dispatch in emergencies (reactive, rarer)

**Proposed values:**

| Building | Coverage | Response | Ratio |
|----------|----------|----------|-------|
| Hazard Post | 10 tiles | 15 tiles | 1.5x |
| Hazard Station | 15 tiles | 25 tiles | 1.67x |
| Hazard Nexus | 20 tiles | 40 tiles | 2x |

**Why increasing ratio for higher tiers:**
Higher-tier buildings have better equipment, faster vehicles, more resources. They SHOULD be able to respond to more distant emergencies. This makes upgrading service buildings more valuable beyond just coverage.

**Gameplay feel:**
- Posts handle local emergencies
- Stations can reach across districts
- Nexuses can respond colony-wide for truly major catastrophes

This creates a natural progression where early colonies need multiple posts for coverage, while late-game colonies can rely on strategically-placed nexuses.

---

### Q6: Capacity Per Tier

**Question:** Proposed 1/3/5 concurrent responses for post/station/nexus. Does this provide good differentiation?

**Answer:** **Yes, but consider 1/2/4 instead.**

**My concern with 1/3/5:**
- The jump from 1 to 3 is huge (3x capacity)
- The jump from 3 to 5 is smaller (1.67x capacity)
- This makes stations feel "too good" relative to their cost

**Recommended 1/2/4:**
- Post: 1 response (handles single incidents)
- Station: 2 responses (can manage small multi-fire situations)
- Nexus: 4 responses (true emergency coordinator)

**Why this works better:**
- Each tier is roughly 2x the previous (consistent upgrade value)
- Posts force players to build more for coverage (good early game activity)
- Stations provide meaningful upgrade without trivializing posts
- Nexuses are clearly premium (4 simultaneous is still substantial)

**Alternative consideration:** If we want nexuses to feel REALLY powerful, keep 5. The asymmetry might be intentional to make nexuses feel like "endgame" buildings.

---

### Q7: Response Time Feel

**Question:** Is distance-based response time (1-2 tiles per tick) the right feel? Minimum response time regardless of distance?

**Answer:** **Distance-based is correct, with a minimum of 5 ticks.**

**Why minimum response time:**
Even if the hazard post is adjacent to the fire, responders need time to:
1. Receive the alert
2. Gear up
3. Exit the building
4. Begin suppression

**Proposed formula:**
```
response_time = max(5, distance / speed_factor)
```

Where speed_factor is:
- Post: 1.0 tiles/tick
- Station: 1.5 tiles/tick
- Nexus: 2.0 tiles/tick

**Example response times:**

| Distance | Post | Station | Nexus |
|----------|------|---------|-------|
| Adjacent (1 tile) | 5 ticks | 5 ticks | 5 ticks |
| 10 tiles | 10 ticks | 7 ticks | 5 ticks |
| 20 tiles | 20 ticks | 14 ticks | 10 ticks |
| 30 tiles | 30 ticks | 20 ticks | 15 ticks |

**Feel validation:**
At 20 ticks per second (50ms/tick), a 30-tile response from a post takes 1.5 seconds. That feels responsive but not instant. A nexus covering the same distance in 0.75 seconds feels appropriately premium.

---

### Q8: Fallback Intensity

**Question:** How strong should fallback suppression be? Eventually extinguish fires, or just slow spread?

**Answer:** **Fallback should EVENTUALLY extinguish, but very slowly.**

**Design rationale:**
Per canon's "cozy sandbox" vibe, we don't want a single missing hazard post to doom a colony. But we also want hazard response to feel valuable.

**Proposed fallback behavior:**

| Fallback Type | Suppression Rate | Time to Extinguish |
|---------------|------------------|-------------------|
| No coverage, no fallback | 0% | Never (burns until fuel exhausted) |
| Civilian response | 5%/tick | ~3 minutes for medium fire |
| Terrain absorption | 2%/tick | ~7 minutes for medium fire |
| Overmind assist (new players) | 10%/tick | ~1.5 minutes |
| Hazard post coverage | 20-40%/tick | 30-60 seconds |

**Why "eventually extinguish":**
1. Fires that burn forever feel broken/frustrating
2. Sandbox players without services shouldn't lose everything
3. BUT slow enough that dedicated coverage is clearly better
4. Creates stories: "I barely saved my colony with no hazard posts" is a good story

**The key insight:** Fallback isn't about being "enough" - it's about being "barely survivable." Players should feel the danger while still having hope.

---

### Q9: Medical Involvement

**Question:** Should medical nexuses provide explicit disaster injury treatment, or is health index penalty sufficient for MVP?

**Answer:** **Health index penalty is sufficient for MVP.** Explicit injuries can be a post-MVP enhancement.

**MVP approach:**
- Disasters reduce colony health index proportional to damage dealt
- Medical coverage mitigates the health index drop (existing Epic 9 mechanic)
- High medical coverage = faster health index recovery post-disaster

**Why this is enough:**
1. Leverages existing systems (Epic 9 health coverage already works)
2. No new mechanics to implement and balance
3. Medical buildings still matter during disasters
4. Keeps scope manageable

**Post-MVP enhancement (optional):**
If we want more depth later, add:
- Injury tracking (injured beings need treatment)
- Emergency medical dispatch (like hazard response but for injuries)
- Temporary population reduction during treatment

But for MVP, the health index approach is clean and effective.

---

### Q10: Overmind Assistance

**Question:** Is new-player protection during disasters appropriate? What population threshold?

**Answer:** **Yes, appropriate for our sandbox vibe. Threshold: 200 beings.**

**Design:**

| Colony Size | Overmind Assistance |
|-------------|---------------------|
| < 200 beings | Full assistance (20% suppression bonus, 30% damage reduction) |
| 200-500 beings | Partial assistance (10% suppression, 15% damage reduction) |
| > 500 beings | No assistance (full challenge) |

**Thematic framing:**
The Overmind (essentially the alien faction's collective consciousness) takes interest in protecting fledgling colonies. As colonies mature, the Overmind trusts the local Overseer to handle threats independently. This is mentorship, not handholding.

**Why 200 threshold:**
- Below 200: Players are still learning the game, one bad disaster could end their session
- 200-500: Transitional period, players should be building services but get a safety net
- Above 500: Players have had time to build services; no more training wheels

**Player communication:**
Show a subtle notification: "The Overmind assists with containment" when assistance activates. Players know they're being helped and should plan for independence.

**Disable option:**
Add to settings: `overmind_assistance: true/false` for players who want the full challenge from the start.

---

## Summary

These answers prioritize:
1. **Sandbox feel** over challenge/punishment
2. **Player agency** through extensive settings and toggles
3. **Memorable moments** over constant stress
4. **Thematic consistency** with our alien worldbuilding
5. **Cooperation** over competition in multiplayer scenarios

Catastrophes should be exciting stories players tell each other, not frustrating obstacles. The emotional arc of warning -> tension -> relief -> recovery -> pride is the goal.

---

*Answers provided by Game Designer Agent. Ready for implementation integration.*
