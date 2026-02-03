# Revision Report: Epic 0 — 2026-01-28

## Trigger
**Document:** Canon update v0.3.0 → v0.5.0
**Type:** canon-update
**Summary:** Major visual/design canon: bioluminescent art direction, hybrid 3D model pipeline, configurable map sizes, expanded alien terrain types, building template system, audio direction, dual UI modes (Classic + Sci-fi Holographic)

## Impact Summary

| Agent | Impact Level | Key Finding |
|-------|-------------|-------------|
| systems-architect | LOW | Map configuration is the only structurally new concept; all other changes are content/visual and hit later epics |
| game-designer | LOW | Theme seeds (palette, audio, UI modes) are now locked in canon; minimal Epic 0 ticket changes needed |

## Ticket Changes

### Modified Tickets

| Ticket | What Changed | Why |
|--------|-------------|-----|
| 0-008 | Added canonical asset directory paths (models/, textures/, audio/) | Canon v0.5.0 defines file organization: sprites/ → models/ + textures/ |
| 0-012 | Added music streaming consideration and playlist/crossfade | Canon v0.5.0 audio direction specifies user-sourced ambient music with playlist support |
| 0-014 | Added map_size server parameter + UI mode client preference | Canon v0.5.0 introduces configurable map sizes and dual UI modes |
| 0-020 | Added UI_MODE_TOGGLE input action | Canon v0.5.0 dual UI modes require a toggle hotkey |
| 0-022 | Added --map-size startup flag + status display | Canon v0.5.0 map size is set at game creation by server host |
| 0-033 | Updated to test 3D model and canonical directory paths | Canon v0.5.0 art pipeline uses .glb models from assets/models/ |
| 0-034 | Added canonical directory paths and hybrid pipeline context | Canon v0.5.0 defines assets/models/ and assets/textures/ paths |

### Removed Tickets

None.

### New Tickets

None.

### Unchanged Tickets

0-001, 0-002, 0-003, 0-004, 0-005, 0-006, 0-007, 0-009, 0-010, 0-011, 0-013, 0-015, 0-016, 0-017, 0-018, 0-019, 0-021, 0-023, 0-024, 0-025, 0-026, 0-027, 0-028, 0-029, 0-030, 0-031, 0-032

## Discussion Summary

One question raised by Game Designer: "Should UI mode preference (Classic vs. Holographic) be persisted via Configuration System or transient session state?"

Resolution: Persisted via Configuration System as a client preference — same pattern as key bindings (ticket 0-019). Added to ticket 0-014 acceptance criteria.

No formal discussion document created (single question, immediately resolved).

## Canon Verification

- Tickets verified: 7 (all MODIFIED tickets)
- Conflicts found: 0
- Resolutions: N/A — all modifications comply with canon v0.5.0

## Decisions Made

No new decisions required. All modifications are straightforward canon alignment.

## Cascading Impact

No cascading impact detected for other epics from Epic 0's revision.

Note: The canon v0.5.0 changes (terrain types, building templates, UI modes, art direction) will have **significant impact on Epics 2-5 and 12** when those epics are planned or revised. However, those impacts originate from the canon update itself, not from Epic 0's revision.
