# Systems Architect Analysis: Epic 16 - Persistence/Save-Load System

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.16.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 16 implements the **PersistenceSystem** - the infrastructure for saving and loading game state. However, this epic exists in a unique context: **ZergCity is a multiplayer-first game with continuous database persistence already established in Epic 1**.

The key insight is that Epic 16's scope is NOT about implementing persistence from scratch. Instead, it addresses:

1. **Server-side save management:** Named save slots, manual saves, auto-save intervals
2. **World export/import:** Exporting database state to portable files for backup/sharing
3. **Client-side persistence:** Settings, preferences, keybindings (distinct from game state)
4. **Component versioning:** Backwards compatibility when game updates
5. **Dense grid serialization:** Efficient handling of terrain, coverage grids, etc.

This epic bridges the gap between Epic 1's "continuous database persistence" (operational) and user-facing "save/load" features (intentional snapshots).

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [Data Model](#2-data-model)
3. [Save Format Design](#3-save-format-design)
4. [Multiplayer Considerations](#4-multiplayer-considerations)
5. [Dense Grid Serialization](#5-dense-grid-serialization)
6. [Versioning Strategy](#6-versioning-strategy)
7. [Client-Side Persistence](#7-client-side-persistence)
8. [Key Work Items](#8-key-work-items)
9. [Questions for Other Agents](#9-questions-for-other-agents)
10. [Risks and Concerns](#10-risks-and-concerns)
11. [Dependencies](#11-dependencies)
12. [Proposed Interfaces](#12-proposed-interfaces)
13. [Canon Update Proposals](#13-canon-update-proposals)

---

## 1. System Boundaries

### 1.1 Relationship to Epic 1 Database Persistence

**Critical clarification:** Epic 1 already established continuous database persistence:

| Aspect | Epic 1 (NetworkManager/SyncSystem) | Epic 16 (PersistenceSystem) |
|--------|------------------------------------|-----------------------------|
| **When** | Continuous, every N ticks | On-demand (manual/auto-save) |
| **Where** | Live database on server | Portable save files |
| **Purpose** | Crash recovery, reconnection | Backup, sharing, archival |
| **Format** | SQLite tables | Compressed binary files |
| **Scope** | Current session state | Named snapshots with metadata |

Epic 16 does NOT replace Epic 1's persistence. It augments it with user-facing save/load features.

### 1.2 PersistenceSystem Boundaries

**Type:** Core System (not ECS - orchestrates serialization, doesn't participate in tick)

**Owns:**
- Save file format specification
- World state serialization to files
- World state deserialization from files
- Save slot management (list, create, delete, rename)
- Auto-save scheduling and execution
- Save file metadata (name, timestamp, preview image, version)
- Component version registry and migration

**Does NOT Own:**
- Component serialization format (components define via ISerializable)
- Database schema (Epic 1 owns)
- Runtime state (ECS owns)
- Client settings format (SettingsManager owns)
- When to trigger saves (host/admin decides)

**Provides:**
- `IPersistenceProvider` interface for save/load operations
- Save file enumeration API
- Import/export world API
- Version compatibility checks

**Depends On:**
- All systems implementing ISerializable (for their components)
- Epic 1 NetworkManager (for database access)
- Epic 1 SyncSystem (for full state snapshots)
- AssetManager (for save file I/O)

### 1.3 Boundary Summary Table

| Concern | PersistenceSystem | NetworkManager (E1) | SyncSystem (E1) | Components |
|---------|-------------------|---------------------|-----------------|------------|
| Continuous DB persistence | - | **Owns** | - | - |
| Save file format | **Owns** | - | - | - |
| Component serialization | Uses | - | Uses | **Define** |
| Save slot management | **Owns** | - | - | - |
| Full state snapshot | Requests | - | **Provides** | - |
| Version migration | **Owns** | - | - | - |

---

## 2. Data Model

### 2.1 Save File Structure

```cpp
struct SaveFileHeader {
    char magic[4] = {'Z', 'C', 'S', 'V'};  // ZergCity Save
    uint32_t format_version = 1;           // Save format version
    uint32_t game_version_major;           // Game version at save time
    uint32_t game_version_minor;
    uint32_t game_version_patch;
    uint64_t save_timestamp;               // Unix timestamp
    uint32_t map_size_x;                   // Map dimensions
    uint32_t map_size_y;
    uint64_t simulation_tick;              // Tick at save time
    uint32_t player_count;                 // Number of players in save
    uint32_t checksum;                     // CRC32 of save data
    char save_name[64];                    // User-provided name
    // Reserved for future expansion
    uint8_t reserved[128];
};

struct SaveFileBody {
    // Section offsets (for random access)
    uint64_t terrain_section_offset;
    uint64_t entity_section_offset;
    uint64_t component_section_offset;
    uint64_t grid_section_offset;
    uint64_t player_section_offset;
    uint64_t metadata_section_offset;

    // Compressed sections follow
    // ...
};
```

### 2.2 Save Slot Metadata

```cpp
struct SaveSlotMetadata {
    std::string slot_id;           // Unique identifier (UUID)
    std::string display_name;      // User-visible name
    std::string file_path;         // Relative path to save file
    uint64_t save_timestamp;       // When saved
    uint64_t simulation_tick;      // Game time at save
    uint32_t player_count;
    uint32_t map_size;             // e.g., 256 for 256x256
    uint64_t file_size_bytes;
    uint32_t game_version;         // Encoded version
    bool is_autosave;

    // Preview data (for UI)
    uint32_t total_population;
    uint32_t total_buildings;
    std::vector<std::string> player_names;

    // Optional: thumbnail image path
    std::optional<std::string> thumbnail_path;
};
```

### 2.3 Auto-Save Configuration

```cpp
struct AutoSaveConfig {
    bool enabled = true;
    uint32_t interval_minutes = 10;        // Save every N minutes
    uint32_t max_autosaves = 3;            // Rolling autosave slots
    bool save_before_disaster = true;      // Save when disaster triggers
    bool save_on_milestone = true;         // Save on milestone achievement
    AutoSaveNaming naming = AutoSaveNaming::Timestamp;
};

enum class AutoSaveNaming {
    Timestamp,     // "autosave_2026-02-01_14-30"
    Sequential,    // "autosave_001", "autosave_002"
    Rolling        // "autosave_1", "autosave_2", "autosave_3" (oldest replaced)
};
```

---

## 3. Save Format Design

### 3.1 Design Goals

1. **Portability:** Save files can be copied between machines
2. **Versioning:** Older saves can be loaded by newer game versions
3. **Efficiency:** Fast save/load for large worlds (512x512 maps)
4. **Integrity:** Detect corruption, prevent partial loads
5. **Extensibility:** New data sections without breaking old saves

### 3.2 Section-Based Format

Save files use a section-based format with a table of contents:

```
┌─────────────────────────────────────────┐
│            HEADER (256 bytes)           │
│  - Magic number, versions, metadata     │
├─────────────────────────────────────────┤
│         TABLE OF CONTENTS               │
│  - Section ID, offset, size, checksum   │
│  - Allows skipping unknown sections     │
├─────────────────────────────────────────┤
│          SECTION: TERRAIN               │
│  - Dense terrain grid data              │
│  - LZ4 compressed                       │
├─────────────────────────────────────────┤
│          SECTION: ENTITIES              │
│  - Entity registry (IDs, archetypes)    │
│  - LZ4 compressed                       │
├─────────────────────────────────────────┤
│          SECTION: COMPONENTS            │
│  - Per-component-type arrays            │
│  - Version tag per component type       │
│  - LZ4 compressed per type              │
├─────────────────────────────────────────┤
│          SECTION: DENSE_GRIDS           │
│  - Coverage grids, contamination, etc.  │
│  - LZ4 compressed                       │
├─────────────────────────────────────────┤
│          SECTION: PLAYERS               │
│  - Player info, ownership state         │
├─────────────────────────────────────────┤
│          SECTION: METADATA              │
│  - Game settings, RNG state, time       │
├─────────────────────────────────────────┤
│          SECTION: PREVIEW (optional)    │
│  - Thumbnail image, stats for UI        │
└─────────────────────────────────────────┘
```

### 3.3 Compression Strategy

Based on Epic 1 decision record `epic-1-lz4-compression.md`:

- **Algorithm:** LZ4 (fast, good ratio for structured data)
- **Granularity:** Per-section compression
- **Benefit:** Can decompress sections independently, skip unneeded sections

```cpp
struct CompressedSection {
    uint32_t section_id;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint32_t checksum;  // Of uncompressed data
    // compressed_data follows
};
```

### 3.4 ISerializable Reuse

Components already implement ISerializable for network sync (Epic 1). Save files reuse this:

```cpp
// From interfaces.yaml
class ISerializable {
public:
    virtual void serialize(WriteBuffer& buffer) const = 0;
    virtual void deserialize(ReadBuffer& buffer) = 0;
    virtual size_t get_serialized_size() const = 0;
};
```

**Key difference from network sync:**
- Network: Only changed components, delta updates
- Save files: ALL components, full state snapshot

Save files add version tags that network messages don't need:

```cpp
void PersistenceSystem::serialize_component_type(
    WriteBuffer& buffer,
    ComponentTypeID type_id,
    const std::vector<EntityID>& entities
) {
    // Write component type metadata
    buffer.write(type_id);
    buffer.write(get_component_version(type_id));  // VERSION TAG
    buffer.write(static_cast<uint32_t>(entities.size()));

    // Write component data
    for (EntityID entity : entities) {
        buffer.write(entity);
        // Component serializes itself via ISerializable
        get_component(entity, type_id).serialize(buffer);
    }
}
```

---

## 4. Multiplayer Considerations

### 4.1 Save Authority

In multiplayer, **only the server can save**:

| Action | Who Can Perform | Implementation |
|--------|-----------------|----------------|
| Manual save | Server host only | Save button in host UI |
| Auto-save | Server automatically | Timer-based, event-based |
| Load save | Server on startup | Command-line or admin panel |
| Export world | Server host only | File browser dialog |

Clients have no direct save/load capability for game state.

### 4.2 Save Slot Management for Multiplayer Worlds

Each server maintains its own save directory:

```
saves/
├── world_alpha/
│   ├── metadata.json          # World info, last played
│   ├── autosave_1.zcsv        # Rolling autosave
│   ├── autosave_2.zcsv
│   ├── manual_save_001.zcsv   # User-created saves
│   └── thumbnails/
│       ├── autosave_1.png
│       └── manual_save_001.png
├── world_beta/
│   └── ...
└── saves_index.json           # Index of all worlds
```

### 4.3 Load Process in Multiplayer Context

Loading a save in multiplayer requires coordination:

```
Server Load Flow:
1. Server operator selects save file (CLI or admin panel)
2. Server broadcasts "world_reloading" to all clients
3. Clients show loading screen
4. Server:
   a. Pauses simulation
   b. Clears ECS registry
   c. Deserializes save file
   d. Rebuilds all system state
   e. Updates database to match save
   f. Resumes simulation
5. Server sends full state snapshot to all clients
6. Clients apply snapshot, resume rendering
```

### 4.4 Client-Side Game State: None

Per canon, clients have NO game state persistence for multiplayer sessions:

- Client connects to server
- Server sends full state
- Client renders
- On disconnect, client forgets game state

This simplifies the architecture significantly. Clients only persist:
- Audio settings (volume levels)
- Video settings (resolution, quality)
- Control settings (keybindings)
- UI preferences (classic vs holographic mode)

---

## 5. Dense Grid Serialization

### 5.1 Dense Grid Types (from patterns.yaml)

The following dense grids need special serialization handling:

| Grid | Size per Tile | Total (256x256) | Notes |
|------|---------------|-----------------|-------|
| TerrainGrid | 4 bytes | 256 KB | terrain_type, elevation, moisture, flags |
| BuildingGrid | 4 bytes | 256 KB | EntityID per tile |
| EnergyCoverageGrid | 1 byte | 64 KB | Coverage zone ID |
| FluidCoverageGrid | 1 byte | 64 KB | Coverage zone ID |
| PathwayGrid | 4 bytes | 256 KB | EntityID per tile |
| ProximityCache | 1 byte | 64 KB | Distance to road |
| DisorderGrid | 1 byte | 64 KB | Double-buffered |
| ContaminationGrid | 2 bytes | 128 KB | Level + type |
| LandValueGrid | 2 bytes | 128 KB | Value + bonus flags |
| ServiceCoverageGrid | 1 byte | 64 KB | Per service type |
| ConflagrationGrid | 3 bytes | 192 KB | Fire spread data |

**Total for 256x256 map:** ~1.5 MB uncompressed for all grids

### 5.2 Grid Serialization Strategy

Dense grids compress extremely well due to spatial coherence:

```cpp
void PersistenceSystem::serialize_dense_grid(
    WriteBuffer& buffer,
    const void* grid_data,
    uint32_t width,
    uint32_t height,
    uint32_t bytes_per_tile,
    DenseGridType grid_type
) {
    // Header
    buffer.write(static_cast<uint8_t>(grid_type));
    buffer.write(width);
    buffer.write(height);
    buffer.write(bytes_per_tile);

    // Raw grid data (will be LZ4 compressed at section level)
    size_t data_size = width * height * bytes_per_tile;
    buffer.write_raw(grid_data, data_size);
}
```

**Compression ratios (typical):**
- TerrainGrid: 10:1 (highly repetitive terrain types)
- CoverageGrids: 15:1 (mostly zeros with clusters)
- ContaminationGrid: 8:1 (gradients, clusters)

Expected compressed size for all grids: ~150-200 KB

### 5.3 Double-Buffered Grids

DisorderGrid and ContaminationGrid use double-buffering (from patterns.yaml). Save files store only the current read buffer:

```cpp
void PersistenceSystem::serialize_double_buffered_grid(
    WriteBuffer& buffer,
    const DoubleBufferedGrid& grid
) {
    // Only save the current state (read buffer)
    // On load, both buffers are initialized to the same state
    serialize_dense_grid(buffer, grid.get_read_buffer(), ...);
}
```

---

## 6. Versioning Strategy

### 6.1 Version Compatibility Goals

| Scenario | Supported? | Notes |
|----------|------------|-------|
| Load older save in newer game | YES | Migration system |
| Load newer save in older game | NO | Reject with message |
| Save format changes | Forward-compatible | New sections ignored by old readers |
| Component format changes | Migration required | Per-component versioning |

### 6.2 Component Version Registry

Each component type has a version number:

```cpp
struct ComponentVersionInfo {
    ComponentTypeID type_id;
    uint32_t current_version;
    std::string type_name;  // For debugging

    // Migration function: old version -> current version
    std::optional<MigrationFunc> migrator;
};

class ComponentVersionRegistry {
private:
    std::unordered_map<ComponentTypeID, ComponentVersionInfo> versions_;

public:
    void register_component(
        ComponentTypeID type_id,
        uint32_t version,
        std::string name,
        std::optional<MigrationFunc> migrator = std::nullopt
    );

    uint32_t get_version(ComponentTypeID type_id) const;

    bool needs_migration(ComponentTypeID type_id, uint32_t file_version) const;

    void migrate(
        ComponentTypeID type_id,
        ReadBuffer& old_data,
        WriteBuffer& new_data,
        uint32_t from_version,
        uint32_t to_version
    );
};
```

### 6.3 Migration Function Example

```cpp
// Example: EnergyComponent version migration
void migrate_energy_component_v1_to_v2(ReadBuffer& old, WriteBuffer& new) {
    // V1 format: energy_required (u32), energy_received (u32), is_powered (bool)
    // V2 format: adds grid_id (u32), priority (u8)

    uint32_t energy_required = old.read<uint32_t>();
    uint32_t energy_received = old.read<uint32_t>();
    bool is_powered = old.read<bool>();

    new.write(energy_required);
    new.write(energy_received);
    new.write(is_powered);
    new.write(uint32_t{0});  // grid_id: default 0
    new.write(uint8_t{3});   // priority: default "normal"
}
```

### 6.4 Upgrade Path

When loading an older save:

```
1. Read save header
2. Check game version compatibility
3. For each component section:
   a. Read component version from file
   b. Compare to current component version
   c. If different, apply migration chain (v1 -> v2 -> v3 -> current)
4. If any migration fails, abort load with error message
5. If all migrations succeed, load proceeds normally
```

### 6.5 Save Format Version vs Game Version

Two separate version tracks:

- **Save format version:** Structure of the save file itself
- **Game version:** Which release created the save

```cpp
bool PersistenceSystem::is_compatible(const SaveFileHeader& header) {
    // Save format must be same or older
    if (header.format_version > CURRENT_SAVE_FORMAT_VERSION) {
        return false;  // Future format, can't read
    }

    // Game version: can load older saves if migrations exist
    // Cannot load saves from future game versions
    GameVersion file_version{
        header.game_version_major,
        header.game_version_minor,
        header.game_version_patch
    };

    return file_version <= CURRENT_GAME_VERSION;
}
```

---

## 7. Client-Side Persistence

### 7.1 Client Settings (Separate from Game State)

Clients persist local preferences via SettingsManager (not PersistenceSystem):

```cpp
struct ClientSettings {
    // Audio (from Epic 15)
    VolumeSettings audio;

    // Video
    struct {
        uint32_t width = 1920;
        uint32_t height = 1080;
        bool fullscreen = false;
        bool vsync = true;
        uint8_t quality_level = 2;  // 0=low, 1=med, 2=high, 3=ultra
    } video;

    // Controls
    struct {
        std::unordered_map<std::string, SDL_Keycode> keybindings;
        float camera_pan_speed = 1.0f;
        float camera_zoom_speed = 1.0f;
        bool edge_scroll_enabled = true;
    } controls;

    // UI
    struct {
        UIMode mode = UIMode::Classic;  // Classic vs Holographic
        float ui_scale = 1.0f;
        bool show_fps = false;
        bool show_grid = false;
    } ui;

    // Multiplayer
    struct {
        std::string last_server_address;
        std::string player_name;
        std::vector<ServerBookmark> saved_servers;
    } multiplayer;
};
```

### 7.2 Client Settings File Format

Use JSON for human readability:

```json
{
    "format_version": 1,
    "audio": {
        "master": 1.0,
        "music": 0.7,
        "sfx": 1.0,
        "ambient": 0.5,
        "ui": 1.0
    },
    "video": {
        "width": 1920,
        "height": 1080,
        "fullscreen": false,
        "vsync": true,
        "quality": 2
    },
    "controls": {
        "keybindings": {
            "camera_north": "W",
            "camera_south": "S",
            "camera_east": "D",
            "camera_west": "A",
            "rotate_cw": "E",
            "rotate_ccw": "Q",
            "zoom_in": "=",
            "zoom_out": "-",
            "pause": "Space"
        }
    },
    "ui": {
        "mode": "classic",
        "scale": 1.0
    },
    "multiplayer": {
        "player_name": "Overseer Alpha",
        "saved_servers": [
            {"name": "My Server", "address": "192.168.1.100:7777"}
        ]
    }
}
```

### 7.3 Client Settings Location

Platform-specific paths:

| Platform | Path |
|----------|------|
| Windows | `%APPDATA%/ZergCity/settings.json` |
| macOS | `~/Library/Application Support/ZergCity/settings.json` |
| Linux | `~/.config/zergcity/settings.json` |

---

## 8. Key Work Items

### Phase 1: Save File Infrastructure (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-001 | SaveFileHeader struct | S | Define header format with magic, versions, metadata |
| E16-002 | Section-based file format | M | Table of contents, section offsets, checksums |
| E16-003 | LZ4 compression integration | M | Compress/decompress sections using LZ4 |
| E16-004 | ComponentVersionRegistry | M | Track component versions, detect mismatches |
| E16-005 | SaveSlotMetadata struct | S | Metadata for UI display |
| E16-006 | IPersistenceProvider interface | M | Define save/load API |
| E16-007 | PersistenceSystem class skeleton | M | Core class with lifecycle methods |
| E16-008 | File I/O abstraction | M | Cross-platform file operations |

### Phase 2: Serialization (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-009 | Entity registry serialization | M | Serialize all entity IDs and archetypes |
| E16-010 | Component section serialization | L | Serialize all component types with versions |
| E16-011 | Dense grid serialization | M | Serialize all dense grids (terrain, coverage, etc.) |
| E16-012 | Double-buffered grid handling | S | Serialize read buffer only |
| E16-013 | Player data serialization | M | Player info, ownership, session state |
| E16-014 | Game metadata serialization | M | Tick, RNG state, settings |
| E16-015 | Preview data generation | M | Stats and thumbnail for UI |
| E16-016 | Full save file writer | L | Orchestrate all sections into complete file |

### Phase 3: Deserialization (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-017 | Save file reader | M | Parse header, validate checksum |
| E16-018 | Entity registry deserialization | M | Rebuild entity registry from file |
| E16-019 | Component deserialization | L | Deserialize all components, apply migrations |
| E16-020 | Dense grid deserialization | M | Reconstruct all dense grids |
| E16-021 | Player data deserialization | M | Restore player state |
| E16-022 | Full load orchestration | L | Coordinate clearing state and loading |

### Phase 4: Version Migration (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-023 | Migration function framework | M | Define migration function signature and registration |
| E16-024 | Version compatibility checking | M | Validate save can be loaded |
| E16-025 | Migration chain execution | M | Apply v1->v2->v3->current migrations |
| E16-026 | Component migration tests | M | Test migration for each component type |
| E16-027 | Save format upgrade tests | M | Test loading older format saves |

### Phase 5: Save Slot Management (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-028 | Save directory structure | S | Define directory layout, index files |
| E16-029 | Save slot enumeration | M | List available saves with metadata |
| E16-030 | Manual save creation | M | Create save with user-provided name |
| E16-031 | Save slot deletion | S | Delete save file and metadata |
| E16-032 | Save slot rename | S | Rename existing save |
| E16-033 | Export world to file | M | Export to user-chosen location |

### Phase 6: Auto-Save (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-034 | Auto-save timer | M | Interval-based auto-save trigger |
| E16-035 | Event-based auto-save | M | Save on milestone, before disaster |
| E16-036 | Rolling auto-save slots | M | Manage N rotating auto-save files |
| E16-037 | Auto-save configuration | S | User settings for auto-save behavior |
| E16-038 | Auto-save during simulation | M | Non-blocking save while game runs |

### Phase 7: Client Settings (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-039 | ClientSettings struct | M | Define all client-side settings |
| E16-040 | Settings JSON serialization | M | Read/write settings.json |
| E16-041 | Settings file path resolution | S | Platform-specific paths |
| E16-042 | Settings defaults and validation | M | Apply defaults, validate ranges |
| E16-043 | Settings UI integration (Epic 12) | M | Connect settings to UI panels |

### Phase 8: Multiplayer Integration (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-044 | Server-only save authority | M | Enforce host-only save/load |
| E16-045 | World reload broadcast | M | Notify clients of impending reload |
| E16-046 | Post-load state sync | M | Send full snapshot after load |
| E16-047 | Load while clients connected | L | Coordinate pause, clear, load, resume |

### Phase 9: Testing (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E16-048 | Save/load round-trip test | M | Save world, load world, verify identical |
| E16-049 | Large world stress test | M | Test with 512x512 map, many entities |
| E16-050 | Migration test suite | M | Test loading saves from each prior version |
| E16-051 | Corruption detection test | M | Verify corrupted files are rejected |

---

## Work Item Summary

| Size | Count |
|------|-------|
| S | 7 |
| M | 35 |
| L | 5 |
| **Total** | **51** |

---

## 9. Questions for Other Agents

### @game-designer

1. **Auto-save frequency:** What's the appropriate interval for auto-saves? 5 minutes? 10 minutes? Should it be player-configurable?

2. **Auto-save on events:** Besides disasters and milestones, are there other events that should trigger auto-save? (e.g., first building placed, reaching population threshold)

3. **Save preview info:** What stats should be shown in the save file browser? Population, treasury, map size, play time, player names?

4. **Save file naming:** Should manual saves have auto-generated names (timestamp-based) or always require user input? Should the default name be "[World Name] - [Date]"?

5. **Load confirmation:** When loading a save in an active multiplayer session, should there be a vote/confirmation from connected players, or is host authority sufficient?

6. **Export for sharing:** Should there be a "share world" feature that exports a save file with certain data stripped (e.g., player sessions)? For sharing worlds on forums/Discord?

### @engine-developer

7. **Save performance:** For large worlds (512x512, thousands of entities), is blocking the main thread during save acceptable? Or should save be async with progress callback?

8. **LZ4 vs alternatives:** Epic 1 decided on LZ4. Should saves use same compression, or consider zstd for better ratio at cost of speed?

9. **Thumbnail generation:** Should save thumbnails be a screenshot of current view, a minimap render, or skip thumbnails entirely?

10. **File handle management:** On Windows, open file handles block deletion. Should saves use temp file + atomic rename pattern?

11. **Database sync on load:** When loading a save file, should we write the loaded state back to the live database immediately, or only update database on next tick?

### @qa-engineer

12. **Compatibility testing matrix:** How many prior versions should we test save compatibility with? Last 3 releases? All releases?

13. **Corruption scenarios:** What corruption scenarios should we test? Truncated file, bit flips, wrong version, invalid checksums?

14. **Multiplayer load testing:** How should we test the "load while clients connected" scenario? Automated or manual?

15. **Performance benchmarks:** What are acceptable save/load times? <1 second for small maps? <5 seconds for large maps?

---

## 10. Risks and Concerns

### 10.1 Technical Risks

**RISK: Save File Size for Large Worlds (MEDIUM)**

512x512 maps with thousands of entities could produce large saves.

**Analysis:**
- Dense grids: ~6 MB uncompressed, ~500 KB compressed
- Entities (10,000): ~400 KB uncompressed, ~50 KB compressed
- Components: ~2 MB uncompressed, ~200 KB compressed
- **Total estimate:** ~1 MB compressed for large world

**Mitigation:** LZ4 compression should keep files manageable. Monitor actual sizes during testing.

---

**RISK: Migration Complexity Over Time (MEDIUM)**

As game evolves, migration chain grows: v1->v2->v3->v4->...

**Mitigation:**
- Drop support for very old versions after major releases
- Test migration chain in CI for every supported version
- Document migration requirements in release notes

---

**RISK: Save During Active Simulation (MEDIUM)**

Saving while simulation runs could capture inconsistent state.

**Mitigation:**
- Pause simulation briefly during save
- Or: Save at tick boundary, use copy-on-write for components being serialized
- Accept brief (50ms) pause as acceptable for save operation

---

**RISK: Client Settings Conflict with Server (LOW)**

Client keybindings might conflict with server-defined actions.

**Mitigation:** Keybindings are client-local and don't affect server. Document any server-side action restrictions.

---

### 10.2 Design Ambiguities

**AMBIGUITY: Relationship to Epic 1 Database**

Epic 1 established continuous database persistence. How does save/load interact?

**Resolution Proposal:**
- Save files are EXPORTS from the database state
- Loading a save file REPLACES database content
- They are complementary, not competing systems

---

**AMBIGUITY: What happens to disconnected players on load?**

If a save is from 2 hours ago and a player who was connected then is not connected now, what happens to their tiles?

**Resolution Proposal:**
- Load restores exact state from save file
- Player sessions are cleared
- When players reconnect, they rejoin their existing tiles
- If player never returns, normal abandonment rules apply (eventually)

---

## 11. Dependencies

### 11.1 What Epic 16 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 0 | AssetManager | File I/O for saves |
| Epic 0 | Application | Save directory paths |
| Epic 1 | ISerializable interface | Component serialization |
| Epic 1 | SyncSystem.get_full_snapshot() | Basis for save data |
| Epic 1 | NetworkManager.get_database() | Sync saves with DB |
| Epic 1 | LZ4 compression | Compress save sections |
| Epic 2 | RenderingSystem | Thumbnail generation |
| Epic 3 | TerrainSystem.get_terrain_grid() | Dense grid access |
| Epic 5 | EnergySystem.get_coverage_grid() | Dense grid access |
| Epic 6 | FluidSystem.get_coverage_grid() | Dense grid access |
| Epic 7 | TransportSystem.get_pathway_grid() | Dense grid access |
| Epic 10 | All simulation grids | Dense grid access |
| Epic 12 | UISystem | Save/load UI panels |
| Epic 15 | AudioSystem.get_settings() | Audio settings persistence |
| All | Components implement ISerializable | Required for serialization |

### 11.2 What Later Epics Need from Epic 16

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 17 (Polish) | Settings persistence | ClientSettings serialization |

### 11.3 Circular Dependency Concern

PersistenceSystem needs to serialize components from ALL systems.
ALL systems need ISerializable (defined in Epic 1).

**Resolution:** No circular dependency exists:
- Epic 1 defines ISerializable interface
- Systems implement ISerializable on their components
- Epic 16 uses ISerializable to serialize everything

---

## 12. Proposed Interfaces

### 12.1 IPersistenceProvider Interface

```cpp
class IPersistenceProvider {
public:
    virtual ~IPersistenceProvider() = default;

    // === Save Operations ===

    // Save current world state to named slot
    virtual SaveResult save_world(
        const std::string& slot_name,
        const std::string& display_name
    ) = 0;

    // Trigger auto-save
    virtual SaveResult auto_save() = 0;

    // Export world to arbitrary file path
    virtual SaveResult export_world(const std::string& file_path) = 0;

    // === Load Operations ===

    // Load world from named slot
    virtual LoadResult load_world(const std::string& slot_name) = 0;

    // Import world from arbitrary file path
    virtual LoadResult import_world(const std::string& file_path) = 0;

    // === Slot Management ===

    // List all save slots with metadata
    virtual std::vector<SaveSlotMetadata> list_saves() const = 0;

    // Delete a save slot
    virtual bool delete_save(const std::string& slot_name) = 0;

    // Rename a save slot
    virtual bool rename_save(
        const std::string& old_name,
        const std::string& new_name
    ) = 0;

    // === Compatibility ===

    // Check if save file can be loaded
    virtual CompatibilityResult check_compatibility(
        const std::string& file_path
    ) const = 0;

    // === Auto-Save Configuration ===

    virtual void set_auto_save_config(const AutoSaveConfig& config) = 0;
    virtual AutoSaveConfig get_auto_save_config() const = 0;
};

struct SaveResult {
    bool success;
    std::string error_message;  // Empty on success
    std::string file_path;      // Where save was written
    uint64_t file_size_bytes;
    float save_duration_ms;
};

struct LoadResult {
    bool success;
    std::string error_message;
    uint64_t loaded_tick;       // Simulation tick from save
    uint32_t entity_count;      // Entities loaded
    float load_duration_ms;
};

struct CompatibilityResult {
    bool can_load;
    bool needs_migration;
    std::string reason;         // Why it can't load, or migration notes
    uint32_t save_format_version;
    GameVersion save_game_version;
};
```

### 12.2 ISettingsProvider Interface

```cpp
class ISettingsProvider {
public:
    virtual ~ISettingsProvider() = default;

    // Get current settings
    virtual const ClientSettings& get_settings() const = 0;

    // Update settings (writes to disk)
    virtual void set_settings(const ClientSettings& settings) = 0;

    // Reset to defaults
    virtual void reset_to_defaults() = 0;

    // Reload from disk (discard in-memory changes)
    virtual void reload() = 0;
};
```

---

## 13. Canon Update Proposals

### 13.1 systems.yaml Clarification

Add clarity about PersistenceSystem vs Epic 1 database:

```yaml
phase_5:
  epic_16_persistence:
    name: "Save/Load System"
    systems:
      PersistenceSystem:
        type: core
        owns:
          - Save file format
          - State serialization to files
          - State deserialization from files
          - Save slot management
          - Auto-save scheduling
          - Component version registry
          - Migration functions
        does_not_own:
          - Component serialization format (components define via ISerializable)
          - Continuous database persistence (Epic 1 owns)
          - Client settings (SettingsManager owns)
        provides:
          - "IPersistenceProvider: save/load API"
          - Save file enumeration
          - Version compatibility checks
        depends_on:
          - All systems (must serialize all state)
          - SyncSystem (for full state snapshots)
          - NetworkManager (for database coordination)
        multiplayer:
          authority: server  # Only host can save/load
          per_player: false  # One world state, not per-player
        notes:
          - "Complements Epic 1 database persistence, does not replace"
          - "Save files are portable exports; database is operational"
          - "Client settings are separate (SettingsManager)"
```

### 13.2 interfaces.yaml Addition

```yaml
persistence:
  IPersistenceProvider:
    description: "Save and load world state to/from files"
    purpose: "User-facing save/load features on top of continuous database"

    methods:
      - name: save_world
        params:
          - name: slot_name
            type: "const std::string&"
          - name: display_name
            type: "const std::string&"
        returns: SaveResult
        description: "Save current world to named slot"

      - name: load_world
        params:
          - name: slot_name
            type: "const std::string&"
        returns: LoadResult
        description: "Load world from named slot"

      - name: list_saves
        params: []
        returns: "std::vector<SaveSlotMetadata>"
        description: "Enumerate all save slots with metadata"

      - name: check_compatibility
        params:
          - name: file_path
            type: "const std::string&"
        returns: CompatibilityResult
        description: "Check if save file can be loaded"

    implemented_by:
      - PersistenceSystem (Epic 16)

    notes:
      - "Server-only in multiplayer"
      - "Clients cannot directly save/load game state"
      - "Uses ISerializable from Epic 1 for component serialization"

  ISettingsProvider:
    description: "Client-side settings persistence"
    purpose: "Save/load audio, video, control, UI preferences"

    methods:
      - name: get_settings
        params: []
        returns: "const ClientSettings&"
        description: "Get current settings"

      - name: set_settings
        params:
          - name: settings
            type: "const ClientSettings&"
        returns: void
        description: "Update and persist settings"

    implemented_by:
      - SettingsManager (Epic 16)

    notes:
      - "Client-only, never synced to server"
      - "JSON format for human readability"
```

### 13.3 patterns.yaml Addition

```yaml
persistence:
  save_file_format:
    description: "Binary save file format with sections"
    pattern: "Header + Table of Contents + Compressed Sections"
    sections:
      - TERRAIN: Dense terrain grid
      - ENTITIES: Entity registry
      - COMPONENTS: Per-type component arrays with version tags
      - DENSE_GRIDS: Coverage grids, contamination, etc.
      - PLAYERS: Player info and ownership
      - METADATA: Game settings, RNG state
      - PREVIEW: Stats and thumbnail for UI
    compression: LZ4
    versioning: "Per-section version tags for migration support"

  component_versioning:
    description: "Backwards compatibility for component format changes"
    pattern: "Version tag per component type, migration function chain"
    rules:
      - "Every component type has a version number"
      - "Save files store version with component data"
      - "On load, apply migration functions: v1->v2->v3->current"
      - "Migrations are tested in CI for all supported versions"
      - "Very old versions may be dropped after major releases"

  client_settings:
    description: "Local preferences separate from game state"
    pattern: "JSON file in platform-specific user data directory"
    scope:
      - Audio volume levels
      - Video settings
      - Control keybindings
      - UI preferences (mode, scale)
      - Multiplayer saved servers
    notes:
      - "Never synced to server"
      - "Human-readable JSON format"
      - "Versioned for migration"
```

---

## Summary

Epic 16 (Persistence/Save-Load System) provides **user-facing save/load features** on top of Epic 1's continuous database persistence:

1. **Save Files:** Portable binary format with LZ4 compression, section-based for extensibility
2. **Save Slots:** Named saves, auto-saves, slot management (list/create/delete/rename)
3. **Version Compatibility:** Component version registry, migration functions, backwards compatibility
4. **Dense Grid Handling:** Efficient serialization of terrain, coverage grids, etc.
5. **Multiplayer:** Server-only authority, coordinate load with connected clients
6. **Client Settings:** Separate JSON-based settings for audio, video, controls, UI

**Key Architectural Insight:**

Epic 1's database persistence is *operational* (crash recovery, reconnection).
Epic 16's save files are *intentional* (user-initiated snapshots, backups, sharing).

They complement each other:
- Database: Always current, always available, automatic
- Save files: Point-in-time, portable, user-controlled

The epic is scoped to **51 work items** covering infrastructure, serialization, deserialization, versioning, slot management, auto-save, client settings, and multiplayer integration.

---

**End of Systems Architect Analysis: Epic 16 - Persistence/Save-Load System**
