# Revision Report: Epic 1 — 2026-01-28

## Trigger
**Document:** Canon update v0.3.0 → v0.5.0
**Type:** canon-update
**Summary:** Major visual/design canon: bioluminescent art direction, hybrid 3D model pipeline, configurable map sizes, expanded alien terrain types, building template system, audio direction, dual UI modes (Classic + Sci-fi Holographic)

## Impact Summary

| Agent | Impact Level | Key Finding |
|-------|-------------|-------------|
| systems-architect | LOW | Configurable map sizes affect snapshot size estimates and require map_size_tier in ServerStatusMessage; all other changes are visual/content with no networking impact |
| engine-developer | LOW | Bandwidth estimates and snapshot projections need map tier ranges; map_size as server config parameter; no architectural changes needed |
| qa-engineer | LOW | Test parameters need map size tier coverage; snapshot size budgets need revision for 512x512 maps; TestServer needs map size config |

## Ticket Changes

### Modified Tickets

| Ticket | What Changed | Why |
|--------|-------------|-----|
| 1-006 | Added map_size_tier to ServerStatusMessage; updated snapshot size estimates from "1-5MB" to "1-30MB depending on map size tier"; updated QA test matrix to parameterize by map size | Canon v0.5.0 introduces configurable map sizes (128/256/512); clients need map dimensions on connect; snapshot sizes vary by tier |
| 1-008 | Added map_size server configuration parameter (small/medium/large, default medium) | Canon v0.5.0 specifies map size is set at game creation by server host |
| 1-014 | Updated snapshot size estimates to per-map-tier ranges; added large-map test scenario; added bounded delta queue tuning note | Canon v0.5.0 configurable map sizes up to 512x512 (262K tiles) significantly increase upper bound of snapshot sizes |
| 1-019 | TestServer supports configurable map size parameter for parameterized tests | Test infrastructure must validate networking at all map size tiers |
| 1-021 | Added large-map integration test scenario (512x512 with substantial entity count) | Verify snapshot transfer completes successfully at the upper bound of configurable map sizes |

### Removed Tickets

None.

### New Tickets

None.

### Unchanged Tickets

1-001, 1-002, 1-003, 1-004, 1-005, 1-007, 1-009, 1-010, 1-011, 1-012, 1-013, 1-015, 1-016, 1-017, 1-018, 1-020, 1-022

## Discussion Summary

Three cross-agent questions were raised and immediately resolved:

1. **Engine Developer → Systems Architect:** "Should map_size be in ServerStatusMessage or a separate GameInfoMessage?"
   - Resolution: Include in ServerStatusMessage — it's a server-level property communicated during connection. No new message type needed.

2. **QA Engineer → Systems Architect:** "Has the snapshot size ceiling been re-estimated? Does this change the chunking strategy?"
   - Resolution: Snapshot sizes updated to per-tier estimates (1-30MB). LZ4 compression + 64KB chunking remain adequate. More chunks for larger maps but no strategy change.

3. **QA Engineer → Engine Developer:** "How will map_size be communicated to clients?"
   - Resolution: Via ServerStatusMessage (same answer as Q1). Tests should assert on this field.

No formal discussion document created — all questions were immediately resolvable.

## Canon Verification

- Tickets verified: 5 (all MODIFIED tickets)
- Conflicts found: 0
- Resolutions: N/A — all modifications comply with canon v0.5.0

## Decisions Made

No new decisions required. All modifications are straightforward canon alignment.

## Cascading Impact

No cascading impact detected for other epics from Epic 1's revision.

Note: The canon v0.5.0 changes (terrain types, building templates, UI modes, art direction, audio) will have **significant impact on Epics 2-5, 12, and 15** when those epics are planned or revised. However, those impacts originate from the canon update itself, not from Epic 1's revision. Epic 1's changes (map_size in server config and messages) are consumed by later epics that depend on NetworkManager, but they add a parameter — they don't change the networking architecture.
