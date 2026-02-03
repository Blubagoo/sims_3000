# Epic 16: Persistence/Save-Load System - Canon Verification

**Date:** 2026-02-01
**Canon Version:** 0.17.0
**Status:** APPROVED - UPDATES APPLIED

---

## Verification Summary

Epic 16 tickets have been verified against canon. The following canon updates are required before implementation.

---

## Canon Compliance Checklist

### 1. systems.yaml - PersistenceSystem

**Current State:** Basic entry exists but lacks detail
```yaml
epic_16_persistence:
  name: "Save/Load System"
  systems:
    PersistenceSystem:
      type: core
      owns:
        - Save file format
        - State serialization
        - State deserialization
        - Save slot management
        - Auto-save
      does_not_own:
        - Component serialization (components define own format)
      provides:
        - Save/load API
        - Save file list
      depends_on:
        - All systems (must serialize all state)
```

**Required Update:** Expand with full detail from planning
```yaml
epic_16_persistence:
  name: "Save/Load System"
  systems:
    PersistenceSystem:
      type: core
      owns:
        - Save file format specification
        - World state serialization to files
        - World state deserialization from files
        - Save slot management (list, create, delete, rename)
        - Checkpoint scheduling and execution
        - Save file metadata (name, timestamp, preview, version)
        - Component version registry and migration
        - World Template export (terrain + structures only)
        - Immortalize feature (monument save files)
      does_not_own:
        - Component serialization format (components define via ISerializable)
        - Continuous database persistence (Epic 1 owns)
        - Client settings persistence (SettingsManager owns)
        - When to trigger saves (host/admin decides)
      provides:
        - "IPersistenceProvider: save/load API for world state"
        - Save file enumeration with metadata
        - Import/export world API
        - Version compatibility checks
        - Checkpoint management
        - Monument Gallery index
      depends_on:
        - All systems implementing ISerializable (for their components)
        - Epic 1 NetworkManager (for database access)
        - Epic 1 SyncSystem (for full state snapshots)
        - AssetManager (for save file I/O)
        - RenderingSystem (for thumbnail generation)
      multiplayer:
        authority: server  # Only host can save/load game state
        per_player: false  # One world state, not per-player
      notes:
        - "Complements Epic 1 database persistence, does not replace"
        - "Save files are portable exports; database is operational"
        - "Client settings are separate (SettingsManager)"
        - "Checkpoints every 10 cycles, keep last 3"
        - "Event triggers: first structure, 10K population, arcology, player join/leave"

    SettingsManager:
      type: core
      owns:
        - Client settings persistence
        - Settings file format (JSON)
        - Platform-specific settings paths
        - Settings validation and defaults
      does_not_own:
        - Game state persistence (PersistenceSystem owns)
        - Audio playback (AudioSystem owns)
        - Video rendering (RenderingSystem owns)
      provides:
        - "ISettingsProvider: settings read/write API"
        - Settings enumeration
        - Default settings
      depends_on: []
      multiplayer:
        authority: client  # Settings are client-local
        per_player: true
      notes:
        - "JSON format for human readability"
        - "Platform paths: Windows %APPDATA%, macOS ~/Library, Linux ~/.config"
        - "Never synced to server"
```

**Compliance:** REQUIRES UPDATE

---

### 2. interfaces.yaml - IPersistenceProvider

**Current State:** Not defined

**Required Addition:**
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

      - name: auto_save
        params: []
        returns: SaveResult
        description: "Trigger checkpoint save"

      - name: export_world
        params:
          - name: file_path
            type: "const std::string&"
        returns: SaveResult
        description: "Export world to arbitrary file path"

      - name: import_world
        params:
          - name: file_path
            type: "const std::string&"
        returns: LoadResult
        description: "Import world from file path"

      - name: export_template
        params:
          - name: file_path
            type: "const std::string&"
        returns: SaveResult
        description: "Export as World Template (stripped of player data)"

      - name: list_saves
        params: []
        returns: "std::vector<SaveSlotMetadata>"
        description: "Enumerate all save slots with metadata"

      - name: delete_save
        params:
          - name: slot_name
            type: "const std::string&"
        returns: bool
        description: "Delete a save slot"

      - name: rename_save
        params:
          - name: old_name
            type: "const std::string&"
          - name: new_name
            type: "const std::string&"
        returns: bool
        description: "Rename a save slot"

      - name: check_compatibility
        params:
          - name: file_path
            type: "const std::string&"
        returns: CompatibilityResult
        description: "Check if save file can be loaded"

      - name: set_auto_save_config
        params:
          - name: config
            type: "const AutoSaveConfig&"
        returns: void
        description: "Configure checkpoint behavior"

      - name: get_auto_save_config
        params: []
        returns: AutoSaveConfig
        description: "Get current checkpoint configuration"

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

      - name: reset_to_defaults
        params: []
        returns: void
        description: "Reset all settings to defaults"

      - name: reload
        params: []
        returns: void
        description: "Reload settings from disk (discard in-memory changes)"

    implemented_by:
      - SettingsManager (Epic 16)

    notes:
      - "Client-only, never synced to server"
      - "JSON format for human readability"
```

**Compliance:** REQUIRES ADDITION

---

### 3. patterns.yaml - Persistence Patterns

**Current State:** Not defined

**Required Addition:**
```yaml
persistence:
  save_file_format:
    description: "Binary save file format with sections"
    pattern: "Header + Table of Contents + Compressed Sections"
    magic: "ZCSV"  # ZergCity Save
    sections:
      - TERRAIN: "Dense terrain grid data"
      - ENTITIES: "Entity registry (IDs, archetypes)"
      - COMPONENTS: "Per-type component arrays with version tags"
      - DENSE_GRIDS: "Coverage grids, contamination, land value, etc."
      - PLAYERS: "Player info, ownership state"
      - METADATA: "Game settings, RNG state, tick count"
      - PREVIEW: "Stats and thumbnail for UI browser"
    compression: "LZ4 per-section"
    versioning: "Per-section version tags for migration support"
    estimated_sizes:
      small_map: "128x128 ~150KB compressed"
      medium_map: "256x256 ~400KB compressed"
      large_map: "512x512 ~1MB compressed"

  component_versioning:
    description: "Backwards compatibility for component format changes"
    pattern: "Version tag per component type, migration function chain"
    rules:
      - "Every component type has a version number"
      - "Save files store version with component data"
      - "On load, apply migration functions: v1->v2->v3->current"
      - "Migrations are tested in CI for all supported versions"
      - "Very old versions may be dropped after major releases"

  checkpoint_system:
    description: "Automatic rolling snapshots for recovery"
    pattern: "Periodic + event-triggered checkpoints"
    rolling:
      interval_cycles: 10
      retention_count: 3
    event_triggers:
      - "First structure placed"
      - "Population thresholds (every 10K)"
      - "Arcology construction complete"
      - "Player join/leave"
      - "Major transactions (>50K credits)"
      - "Idle detection (5 min no input)"
      - "Pre-disaster (on warning)"
    manual_snapshots:
      slots_per_player: 5
      purpose: "Experimentation backup"

  multiplayer_rollback:
    description: "Authority model for multiplayer save loading"
    pattern: "Time-based authority thresholds"
    rules:
      - "Host authority for rollbacks <30 minutes"
      - "Majority vote required for rollbacks >30 minutes"
      - "Unanimous consent for rollbacks >24 hours"
      - "2-minute response timeout, then host decision stands"
      - "Prevents griefing while allowing legitimate recovery"

  world_template_export:
    description: "Shareable world format stripped of player data"
    pattern: "Full save minus player identity/economy/progression"
    includes:
      - "Terrain and map seed"
      - "All structures and infrastructure"
      - "Named landmarks"
      - "Basic statistics (population at export, cycle count)"
    excludes:
      - "Player identities and sessions"
      - "Treasury amounts"
      - "Achievement/milestone progress"
      - "Disaster history and event logs"
    file_extension: ".zcworld"

  immortalize_feature:
    description: "Freeze colony as permanent monument"
    pattern: "Read-only snapshot with gallery metadata"
    cost: 50000  # Credits
    visibility_options:
      - "Private (owner only)"
      - "Friends (invited)"
      - "Public (Monument Gallery)"
    storage:
      location: "saves/gallery/"
      index_file: "gallery_index.json"
      per_monument: "~150KB-1MB + ~50KB thumbnail"
    visitor_mode: "Read-only exploration, no modifications"

  client_settings:
    description: "Local preferences separate from game state"
    pattern: "JSON file in platform-specific user data directory"
    scope:
      - "Audio volume levels (per channel)"
      - "Video settings (resolution, fullscreen, quality)"
      - "Control keybindings"
      - "UI preferences (mode, scale)"
      - "Multiplayer saved servers"
    paths:
      windows: "%APPDATA%/ZergCity/settings.json"
      macos: "~/Library/Application Support/ZergCity/settings.json"
      linux: "~/.config/zergcity/settings.json"
    notes:
      - "Never synced to server"
      - "Human-readable JSON format"
      - "Versioned for migration"

  corruption_detection:
    description: "Integrity verification for save files"
    pattern: "Multi-level checksums"
    file_level: "CRC32 in header"
    runtime_level: "Lightweight state hash (entity count, population, treasury)"
    recovery: "Hash mismatch triggers full resync from server"
```

**Compliance:** REQUIRES ADDITION

---

### 4. Data Contracts

**Current State:** interfaces.yaml has data_contracts section

**Required Additions to data_contracts:**
```yaml
SaveResult:
  description: "Result of a save operation"
  fields:
    - name: success
      type: bool
    - name: error_message
      type: "std::string"
      description: "Empty on success"
    - name: file_path
      type: "std::string"
      description: "Where save was written"
    - name: file_size_bytes
      type: uint64_t
    - name: save_duration_ms
      type: float
  used_by:
    - PersistenceSystem

LoadResult:
  description: "Result of a load operation"
  fields:
    - name: success
      type: bool
    - name: error_message
      type: "std::string"
    - name: loaded_tick
      type: uint64_t
      description: "Simulation tick from save"
    - name: entity_count
      type: uint32_t
    - name: load_duration_ms
      type: float
  used_by:
    - PersistenceSystem

CompatibilityResult:
  description: "Save file compatibility check result"
  fields:
    - name: can_load
      type: bool
    - name: needs_migration
      type: bool
    - name: reason
      type: "std::string"
      description: "Why it can't load, or migration notes"
    - name: save_format_version
      type: uint32_t
    - name: save_game_version
      type: GameVersion
  used_by:
    - PersistenceSystem

SaveSlotMetadata:
  description: "Metadata for save file browser display"
  fields:
    - name: slot_id
      type: "std::string"
    - name: display_name
      type: "std::string"
    - name: file_path
      type: "std::string"
    - name: save_timestamp
      type: uint64_t
    - name: simulation_tick
      type: uint64_t
    - name: player_count
      type: uint32_t
    - name: map_size
      type: uint32_t
    - name: file_size_bytes
      type: uint64_t
    - name: game_version
      type: uint32_t
    - name: is_autosave
      type: bool
    - name: total_population
      type: uint32_t
    - name: total_buildings
      type: uint32_t
    - name: player_names
      type: "std::vector<std::string>"
    - name: thumbnail_path
      type: "std::optional<std::string>"
  used_by:
    - PersistenceSystem
    - UISystem (world browser)

AutoSaveConfig:
  description: "Configuration for checkpoint system"
  fields:
    - name: enabled
      type: bool
      default: true
    - name: interval_cycles
      type: uint32_t
      default: 10
    - name: max_checkpoints
      type: uint32_t
      default: 3
    - name: save_before_disaster
      type: bool
      default: true
    - name: save_on_milestone
      type: bool
      default: true
  used_by:
    - PersistenceSystem

ClientSettings:
  description: "Client-side user preferences"
  fields:
    - name: audio
      type: VolumeSettings
    - name: video
      type: VideoSettings
    - name: controls
      type: ControlSettings
    - name: ui
      type: UISettings
    - name: multiplayer
      type: MultiplayerSettings
  used_by:
    - SettingsManager
    - UISystem
```

**Compliance:** REQUIRES ADDITION

---

## Dependency Verification

| Dependency | Status | Notes |
|------------|--------|-------|
| Epic 0 (AssetManager) | OK | File I/O available |
| Epic 1 (ISerializable) | OK | Interface defined |
| Epic 1 (SyncSystem) | OK | Full state snapshot available |
| Epic 1 (LZ4 compression) | OK | Decision record exists |
| Epic 2 (RenderingSystem) | OK | For thumbnail generation |
| All systems (ISerializable) | OK | Defined in Epic 1 |

---

## Verification Result

**APPROVED - PENDING UPDATES**

The Epic 16 tickets are aligned with canon principles. The following updates must be applied before implementation begins:

1. **systems.yaml**: Expand PersistenceSystem definition, add SettingsManager
2. **interfaces.yaml**: Add IPersistenceProvider, ISettingsProvider interfaces
3. **patterns.yaml**: Add persistence patterns section
4. **interfaces.yaml (data_contracts)**: Add SaveResult, LoadResult, CompatibilityResult, SaveSlotMetadata, AutoSaveConfig, ClientSettings

---

## Sign-off

- [x] Systems Architect: Approved
- [x] Game Designer: Approved
- [x] Canon updates applied via `/update-canon` (2026-02-01)
