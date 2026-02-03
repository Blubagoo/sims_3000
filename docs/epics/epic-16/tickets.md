# Epic 16: Persistence/Save-Load System - Ticket Breakdown

**Generated:** 2026-02-01
**Canon Version:** 0.16.0
**Status:** READY FOR VERIFICATION
**Total Tickets:** 58

---

## Overview

Epic 16 implements the PersistenceSystem - user-facing save/load features that complement Epic 1's continuous database persistence. This system provides save file management, world export/import, checkpoint management, the Immortalize feature, and client settings persistence.

### Key Characteristics
- **System Type:** Core System (not ECS - doesn't participate in simulation tick)
- **Authority:** Server-authoritative for game state; client-only for settings
- **Relationship to Epic 1:** Augments continuous DB persistence with intentional snapshots
- **Save Format:** Section-based binary with LZ4 compression

### Design Decisions (from Discussion)
- **D-1:** Continuous database persistence (Epic 1) + periodic checkpoints, no traditional interval auto-save
- **D-2:** Rolling checkpoints every 10 cycles, keep last 3
- **D-3:** Event-triggered checkpoints: first structure, population thresholds (10K), arcology complete, player join/leave, major transactions
- **D-4:** Multiplayer rollback authority: host for <30 min, majority vote for >30 min
- **D-5:** World Template Export: terrain + structures, stripped of player data
- **D-6:** Immortalized worlds: separate save files with gallery_index.json metadata
- **D-7:** Save file sizes: 128x128 ~150KB, 256x256 ~400KB, 512x512 ~1MB (LZ4)
- **D-8:** Corruption detection: CRC32 file checksum + lightweight runtime state hash

---

## Group A: Components & Data Structures (9 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-A01 | SaveFileHeader struct | S | Define header with magic 'ZCSV', format version, game version, timestamp, map size, tick, player count, checksum, save name | None |
| 16-A02 | SaveFileBody section format | M | Table of contents with section IDs, offsets, sizes, checksums for random access | 16-A01 |
| 16-A03 | SaveSlotMetadata struct | S | Slot ID, display name, file path, timestamps, map size, file size, preview data (population, buildings, player names) | None |
| 16-A04 | AutoSaveConfig struct | S | Enabled flag, interval cycles (10), max checkpoints (3), event triggers, naming convention | None |
| 16-A05 | SaveResult/LoadResult structs | S | Success flag, error message, file path, size, duration metrics | None |
| 16-A06 | CompatibilityResult struct | S | can_load, needs_migration, reason, save_format_version, game_version | None |
| 16-A07 | GalleryIndexEntry struct | S | Metadata for immortalized worlds: name, player names, population, date, achievements | None |
| 16-A08 | WorldTemplateMetadata struct | S | Stripped export format: map seed, structures, landmarks without player data | None |
| 16-A09 | ClientSettings struct | M | Audio volumes, video settings, controls/keybindings, UI preferences, multiplayer bookmarks | None |

---

## Group B: Interfaces (4 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-B01 | IPersistenceProvider interface | M | save_world(), load_world(), export_world(), import_world(), list_saves(), delete_save(), rename_save(), check_compatibility() | 16-A03, 16-A05, 16-A06 |
| 16-B02 | ISettingsProvider interface | S | get_settings(), set_settings(), reset_to_defaults(), reload() for client settings | 16-A09 |
| 16-B03 | StubPersistenceProvider | S | No-op implementation for testing | 16-B01 |
| 16-B04 | StubSettingsProvider | S | Default-returning stub for testing | 16-B02 |

---

## Group C: Core System (8 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-C01 | PersistenceSystem class skeleton | M | Core class with initialize(), shutdown(), lifecycle methods | 16-B01 |
| 16-C02 | LZ4 compression integration | M | Compress/decompress sections using LZ4 (reuse Epic 1 infrastructure) | 16-C01 |
| 16-C03 | ComponentVersionRegistry | M | Track component versions, register components, detect version mismatches | 16-C01 |
| 16-C04 | Save directory structure | S | Define saves/, gallery/, thumbnails/ layout with saves_index.json | 16-C01 |
| 16-C05 | File I/O abstraction | M | Cross-platform file operations with temp file + atomic rename pattern | 16-C01 |
| 16-C06 | SettingsManager class | M | Implement ISettingsProvider, JSON serialization, platform-specific paths | 16-B02 |
| 16-C07 | Settings file path resolution | S | Windows: %APPDATA%/ZergCity/, macOS: ~/Library/Application Support/, Linux: ~/.config/zergcity/ | 16-C06 |
| 16-C08 | Settings defaults and validation | M | Apply defaults for missing values, validate ranges, handle corrupt settings | 16-C06 |

---

## Group D: Serialization (9 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-D01 | Entity registry serialization | M | Serialize all entity IDs and archetypes to ENTITIES section | 16-C01 |
| 16-D02 | Component section serialization | L | Serialize all component types with version tags via ISerializable | 16-C03 |
| 16-D03 | Dense grid serialization | M | Serialize terrain, coverage, contamination, land value, pathway grids | 16-C02 |
| 16-D04 | Double-buffered grid handling | S | Save read buffer only; initialize both buffers on load | 16-D03 |
| 16-D05 | Player data serialization | M | Player info, ownership state, colony names, treasury | 16-C01 |
| 16-D06 | Game metadata serialization | M | Simulation tick, RNG state, cycle count, game settings | 16-C01 |
| 16-D07 | Preview data generation | M | Extract population, building count, player names for save browser | 16-C01 |
| 16-D08 | Thumbnail capture | M | Capture minimap render as PNG for save preview | 16-D07 |
| 16-D09 | Full save file writer | L | Orchestrate all sections, write header, TOC, compressed sections | 16-D01, 16-D02, 16-D03, 16-D05, 16-D06 |

---

## Group E: Deserialization (7 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-E01 | Save file reader | M | Parse header, validate magic and checksums, read TOC | 16-A01, 16-A02 |
| 16-E02 | Entity registry deserialization | M | Rebuild entity registry, restore entity IDs | 16-E01 |
| 16-E03 | Component deserialization | L | Deserialize all components, apply migrations if needed | 16-E01, 16-C03 |
| 16-E04 | Dense grid deserialization | M | Reconstruct all dense grids from compressed sections | 16-E01 |
| 16-E05 | Player data deserialization | M | Restore player state, ownership, sessions | 16-E01 |
| 16-E06 | Full load orchestration | L | Clear ECS registry, deserialize all sections, rebuild system state | 16-E02, 16-E03, 16-E04, 16-E05 |
| 16-E07 | Corruption detection | M | Validate CRC32 checksums, runtime state hash verification | 16-E01 |

---

## Group F: Version Migration (5 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-F01 | Migration function framework | M | Define migration function signature, registration, chain execution | 16-C03 |
| 16-F02 | Version compatibility checking | M | Compare save version to current, determine if loadable/needs migration | 16-A06 |
| 16-F03 | Migration chain execution | M | Apply v1->v2->v3->current migrations sequentially | 16-F01 |
| 16-F04 | Component migration tests | M | Test migration for each component type, verify data integrity | 16-F01 |
| 16-F05 | Save format upgrade tests | M | Test loading saves from prior format versions | 16-F03 |

---

## Group G: Checkpoint System (7 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-G01 | Rolling checkpoint scheduler | M | Trigger checkpoint every 10 cycles (configurable) | 16-D09 |
| 16-G02 | Checkpoint retention policy | S | Keep last 3 rolling checkpoints, delete oldest | 16-G01 |
| 16-G03 | Event-triggered checkpoints | M | First structure, population thresholds (10K), arcology complete, player join/leave | 16-G01 |
| 16-G04 | Major transaction checkpoint | M | Checkpoint on transactions >50K credits | 16-G01 |
| 16-G05 | Pre-disaster checkpoint | M | Checkpoint when disaster warning triggers | 16-G01 |
| 16-G06 | Idle detection checkpoint | S | Checkpoint after 5 minutes no input before potential disconnect | 16-G01 |
| 16-G07 | Manual snapshot slots | M | 5 user-controlled snapshot slots with UI integration | 16-D09 |

---

## Group H: Save Slot Management (6 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-H01 | Save slot enumeration | M | List available saves with metadata for world browser | 16-A03, 16-C04 |
| 16-H02 | Manual save creation | M | Create save with user-provided name "[World Name] - Cycle [N]" | 16-D09 |
| 16-H03 | Save slot deletion | S | Delete save file, metadata, and thumbnail | 16-H01 |
| 16-H04 | Save slot rename | S | Rename existing save display name | 16-H01 |
| 16-H05 | World Template Export | M | Export terrain + structures stripped of player data as .zcworld | 16-D09, 16-A08 |
| 16-H06 | World Template Import | M | Import .zcworld as new world with fresh economy/progression | 16-E06 |

---

## Group I: Immortalize Feature (6 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-I01 | Immortalize save creation | M | Create monument save file with special metadata, 50K credit cost | 16-D09 |
| 16-I02 | Gallery index management | M | gallery_index.json for immortalized world metadata, searchable | 16-A07 |
| 16-I03 | Monument thumbnail capture | M | Capture at immortalize camera position for gallery preview | 16-D08 |
| 16-I04 | Monument visibility settings | S | Private/Friends/Public visibility flag per monument | 16-I01 |
| 16-I05 | Monument visitor mode | M | Read-only world load for visiting immortalized colonies | 16-E06 |
| 16-I06 | Monument Gallery browser | M | UI for browsing, searching, visiting immortalized worlds | 16-I02 |

---

## Group J: Multiplayer Integration (5 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-J01 | Server-only save authority | M | Enforce host-only save/load, reject client requests | 16-C01 |
| 16-J02 | World reload broadcast | M | WorldReloadingMessage to all clients before load | 16-J01 |
| 16-J03 | Post-load state sync | M | Full state snapshot to all clients after load completes | 16-E06 |
| 16-J04 | Load while clients connected | L | Coordinate pause, clear, load, resume across all clients | 16-J02, 16-J03 |
| 16-J05 | Rollback authority voting | M | Host authority <30 min; majority vote >30 min; unanimous >24 hr | 16-J01 |

---

## Group K: Testing (5 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 16-K01 | Save/load round-trip test | M | Save world, load world, verify state identical | 16-D09, 16-E06 |
| 16-K02 | Large world stress test | M | Test with 512x512 map, thousands of entities, measure performance | 16-K01 |
| 16-K03 | Migration test suite | M | Test loading saves from each prior format version | 16-F05 |
| 16-K04 | Corruption detection test | M | Verify truncated, bit-flipped, wrong-version files rejected | 16-E07 |
| 16-K05 | Multiplayer load test | L | Test load-while-connected scenario with multiple clients | 16-J04 |

---

## Dependency Graph

```
A01──►A02──►D09
A03──►B01──►C01
A04──►G01
A05──►B01
A06──►F02
A07──►I02
A08──►H05
A09──►B02──►C06──►C07──►C08

C01──►C02──►D03
  │     │
  │     ▼
  │   C03──►D02──►E03
  │
  ├──►C04
  ├──►C05
  └──►D01──►D09
           │
           ▼
        E06──►J03

G01──►G02
  ├──►G03,G04,G05,G06
  └──►G07

H01──►H02,H03,H04
H05──►H06

I01──►I02──►I06
  ├──►I03
  └──►I04
       │
       ▼
     I05

J01──►J02──►J04
  │     │
  │     ▼
  │   J03
  └──►J05
```

---

## Size Summary

| Size | Count | Tickets |
|------|-------|---------|
| S | 14 | 16-A01, 16-A03, 16-A04, 16-A05, 16-A06, 16-A07, 16-A08, 16-B02, 16-B03, 16-B04, 16-C04, 16-C07, 16-D04, 16-G02, 16-G06, 16-H03, 16-H04, 16-I04 |
| M | 38 | 16-A02, 16-A09, 16-B01, 16-C01, 16-C02, 16-C03, 16-C05, 16-C06, 16-C08, 16-D01, 16-D03, 16-D05, 16-D06, 16-D07, 16-D08, 16-E01, 16-E02, 16-E04, 16-E05, 16-E07, 16-F01, 16-F02, 16-F03, 16-F04, 16-F05, 16-G01, 16-G03, 16-G04, 16-G05, 16-G07, 16-H01, 16-H02, 16-H05, 16-H06, 16-I01, 16-I02, 16-I03, 16-I05, 16-I06, 16-J01, 16-J02, 16-J03, 16-J05, 16-K01, 16-K02, 16-K03, 16-K04 |
| L | 6 | 16-D02, 16-D09, 16-E03, 16-E06, 16-J04, 16-K05 |
| **Total** | **58** | |

---

## Canon Updates Required

Based on the planning discussion, the following canon updates are needed:

1. **systems.yaml**: Add PersistenceSystem as core system, clarify relationship to Epic 1 database
2. **interfaces.yaml**: Add IPersistenceProvider and ISettingsProvider interfaces
3. **patterns.yaml**: Add save_file_format, component_versioning, client_settings patterns

---

## Implementation Order Recommendation

### Phase 1: Foundation (Weeks 1-2)
16-A01 through 16-A09, 16-B01 through 16-B04, 16-C01 through 16-C08

### Phase 2: Serialization (Week 3)
16-D01 through 16-D09

### Phase 3: Deserialization (Week 4)
16-E01 through 16-E07

### Phase 4: Migration (Week 5)
16-F01 through 16-F05

### Phase 5: Checkpoint System (Week 6)
16-G01 through 16-G07

### Phase 6: Save Management (Week 7)
16-H01 through 16-H06

### Phase 7: Immortalize (Week 8)
16-I01 through 16-I06

### Phase 8: Multiplayer & Testing (Week 9)
16-J01 through 16-J05, 16-K01 through 16-K05

---

## Notes

- Epic 16 complements Epic 1's continuous DB persistence; does NOT replace it
- Save files are portable exports; database is operational crash recovery
- Client settings are JSON for human readability; save files are binary for performance
- Immortalize creates read-only monument copies separate from live worlds
- World Template Export strips player identity for sharing creative builds
- Multiplayer rollback has time-based authority thresholds to prevent griefing
