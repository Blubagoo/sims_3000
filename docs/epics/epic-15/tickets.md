# Epic 15: Audio System - Ticket Breakdown

**Generated:** 2026-02-01
**Canon Version:** 0.15.0
**Status:** READY FOR VERIFICATION
**Total Tickets:** 52

---

## Overview

Epic 15 implements the AudioSystem - a client-only core system providing music playback, sound effects, ambient audio, and volume mixing. The system is event-driven, responding to game events to automatically trigger appropriate sounds.

### Key Characteristics
- **System Type:** Core System (not ECS - doesn't participate in simulation tick)
- **Authority:** Client-authoritative (no network sync)
- **Integration:** Event-driven via EventBus subscriptions
- **Audio Backend:** SDL3 with SDL_mixer for format support

### Design Decisions
- Sound variants: Tiered (3-4 for frequent, 2-3 for medium, 1 for rare)
- Positional audio: Zoom-aware, 50 tile max default
- Music: Streaming with 3-second crossfade
- Pause behavior: Fade ambient to 30-40%

---

## Group A: Components & Data Structures (8 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-A01 | AudioChannel enum | S | Define audio channels: Master, Music, SFX, Ambient, UI | None |
| 15-A02 | VolumeSettings struct | S | Volume levels per channel (0.0-1.0) with defaults | 15-A01 |
| 15-A03 | SoundEffectType enum | S | All sound types: construction (0-99), infrastructure (100-199), UI (200-299), simulation (300-399), ambient (400-499) | None |
| 15-A04 | SoundEffectAsset struct | S | Asset definition with file_path, base_volume, pitch_variation, distance params, max_instances | 15-A03 |
| 15-A05 | MusicTrack struct | S | Track definition with file_path, title, duration, volume_multiplier | None |
| 15-A06 | Playlist struct | S | Track list with shuffle, repeat, current_index state | 15-A05 |
| 15-A07 | ListenerState struct | S | Camera position/forward/zoom for positional audio calculation | None |
| 15-A08 | SoundPriority enum | S | Priority levels: Critical, High, Normal, Low | None |

---

## Group B: Interfaces (3 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-B01 | IAudioProvider interface | M | Core interface: play_sound(), play_sound_at(), set_volume(), play_music(), pause_music() | 15-A01, 15-A03 |
| 15-B02 | StubAudioProvider | S | Stub implementation for testing (no-op) | 15-B01 |
| 15-B03 | IAudioProvider integration | M | Register AudioSystem as IAudioProvider, make available to other systems | 15-B01 |

---

## Group C: Core System (7 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-C01 | AudioSystem class skeleton | M | Core class with initialize(), shutdown(), update() lifecycle | 15-B01 |
| 15-C02 | SDL3 audio device initialization | M | Open audio device, configure 48kHz stereo, handle errors | 15-C01 |
| 15-C03 | SDL_mixer integration | M | Initialize SDL_mixer for OGG/MP3/FLAC format support | 15-C02 |
| 15-C04 | Audio asset loading | M | Load SFX via AssetManager, cache in memory (~20MB budget) | 15-C01 |
| 15-C05 | Audio config file format | M | JSON/YAML defining sound-to-event mappings, per-sound settings | 15-A04 |
| 15-C06 | Audio shutdown and cleanup | S | Proper resource cleanup, stream closing, device closure | 15-C02 |
| 15-C07 | AudioSystem main loop integration | M | Call update() from main loop (not simulation tick) | 15-C01 |

---

## Group D: Sound Effect System (10 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-D01 | SFXPlayer class | L | Sound effect playback with channel pooling, active sound tracking | 15-C02 |
| 15-D02 | Sound priority system | M | Priority-based channel allocation, drop low-priority when full | 15-A08, 15-D01 |
| 15-D03 | Instance limiting | S | Limit max simultaneous instances per sound type | 15-D01 |
| 15-D04 | Pitch variation | S | Random pitch variation within configured range for variety | 15-D01 |
| 15-D05 | Sound variant selection | M | Multiple sounds per type, random selection for variation | 15-A04, 15-D01 |
| 15-D06 | Positional audio calculation | M | Distance attenuation with zoom-aware scaling | 15-A07, 15-D01 |
| 15-D07 | Stereo panning | M | Left/right pan based on sound position relative to camera | 15-D06 |
| 15-D08 | CameraSystem listener integration | M | Query CameraSystem for ListenerState (position, zoom) | 15-A07, 15-D06 |
| 15-D09 | Sound spam prevention | M | Aggregate rapid placements, cooldown per sound type | 15-D03 |
| 15-D10 | Density-based sound layers | M | Layer additional sounds for high-density building completion | 15-D05 |

---

## Group E: Music System (9 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-E01 | MusicPlayer class | L | Music playback with state machine (stopped/playing/paused/crossfading) | 15-C02 |
| 15-E02 | Music streaming | M | Stream large files with ~128KB buffer, not preload | 15-E01 |
| 15-E03 | Crossfade implementation | M | 3-second crossfade between tracks (canon requirement) | 15-E01 |
| 15-E04 | Playlist management | M | Play, pause, skip, shuffle, repeat controls | 15-A06, 15-E01 |
| 15-E05 | User music directory scanning | M | Scan user folder for MP3/OGG/FLAC, add to playlist | 15-E04 |
| 15-E06 | Default music fallback | S | Provide minimal default ambient tracks if no user music | 15-E04 |
| 15-E07 | Music controls API | M | Expose play/pause/skip/shuffle through IAudioProvider | 15-B01, 15-E04 |
| 15-E08 | Dynamic music state transitions | M | Layer tension during disasters, transition to recovery | 15-E03 |
| 15-E09 | Pause behavior for music | S | Reduce to 50% volume during game pause | 15-E01 |

---

## Group F: Ambient Audio System (6 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-F01 | AmbientPlayer class | M | Layered ambient sound system with multiple loops | 15-C02 |
| 15-F02 | Colony activity hum | M | Population-scaled ambient layer (canon: scales with population) | 15-F01 |
| 15-F03 | Environmental layers | M | Wind, bioluminescent pulses as looping layers | 15-F01 |
| 15-F04 | Ambient volume fading | S | Smooth transitions when layers activate/deactivate | 15-F01 |
| 15-F05 | Pause behavior for ambient | S | Fade to 30-40% volume during game pause | 15-F01 |
| 15-F06 | Zoom-based ambient mixing | M | More local sounds zoomed in, colony-wide blend zoomed out | 15-F01, 15-D08 |

---

## Group G: Volume Mixing (5 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-G01 | VolumeManager class | M | Per-channel volume control with master multiplier | 15-A02 |
| 15-G02 | Music ducking | M | Reduce music during important sounds (milestones, disasters) | 15-G01 |
| 15-G03 | Ducking recovery | S | Smooth fade back to normal after duck triggers end | 15-G02 |
| 15-G04 | Volume settings serialization | S | Save/load volume preferences (for PersistenceSystem) | 15-A02 |
| 15-G05 | Mute/unmute functionality | S | Quick mute toggles per channel | 15-G01 |

---

## Group H: Event Integration (9 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-H01 | EventBus subscription setup | M | Subscribe to all relevant game events during initialization | 15-C01 |
| 15-H02 | Construction event sounds | M | ZoneDesignated, BuildingMaterializing, BuildingComplete, Demolition | 15-H01, 15-D01 |
| 15-H03 | Infrastructure event sounds | M | PathwayPlaced, ConduitPlaced, PipePlaced, Connected | 15-H01, 15-D01 |
| 15-H04 | UI event sounds | M | ButtonClick, ToolSelected, MenuOpen/Close, AlertPulse | 15-H01, 15-D01 |
| 15-H05 | Disaster event sounds | M | DisasterWarning, DisasterActive, DisasterEnded with severity scaling | 15-H01, 15-D01 |
| 15-H06 | Progression event sounds | M | MilestoneAchieved (tiered length), PopulationThreshold | 15-H01, 15-D01 |
| 15-H07 | Other player milestone sounds | M | Subtle chime for rival achievements, configurable | 15-H06 |
| 15-H08 | Population change listener | M | Update colony hum based on PopulationChangedEvent | 15-F02 |
| 15-H09 | Sound-to-event mapping config | M | Data-driven config file for event-to-sound mappings | 15-C05, 15-H01 |

---

## Group I: Settings & UI Integration (4 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-I01 | Audio settings data model | S | Settings struct with all volume levels, mute states | 15-A02 |
| 15-I02 | Audio settings UI hooks | M | Expose volume controls for settings UI (Epic 12) | 15-G01 |
| 15-I03 | Audio device selection | M | Choose output device from available (optional feature) | 15-C02 |
| 15-I04 | Accessibility options | M | Mono audio, reduced audio mode, volume presets | 15-G01 |

---

## Group J: Testing (4 tickets)

| ID | Title | Size | Description | Dependencies |
|----|-------|------|-------------|--------------|
| 15-J01 | Unit tests for volume mixing | M | Test VolumeManager calculations, ducking | 15-G01 |
| 15-J02 | Unit tests for positional audio | M | Test distance attenuation, zoom scaling, panning | 15-D06 |
| 15-J03 | Integration test for events | L | Test event-to-sound mapping with mock EventBus | 15-H01 |
| 15-J04 | Audio playback stress test | M | Test max simultaneous sounds, priority system | 15-D02 |

---

## Dependency Graph

```
A01─┬──►B01──►B03
A03─┘    │
         ▼
    ┌──►C01──►C02──►C03──►C04
    │    │           │
    │    ▼           ▼
    │  C07         D01──►D02──►D03
    │              │     │
    │              │     ▼
    │              │   D04,D05
    │              ▼
    │            D06──►D07
    │              │
    │              ▼
    │            D08
    │
    └──►E01──►E02──►E03──►E04
         │           │
         ▼           ▼
       E08,E09    E05,E07

F01──►F02──►F06
  │
  ▼
F03,F04,F05

G01──►G02──►G03
  │
  ▼
G04,G05

H01──►H02,H03,H04,H05,H06,H07,H08,H09
```

---

## Size Summary

| Size | Count | Tickets |
|------|-------|---------|
| S | 17 | 15-A01 through 15-A08, 15-B02, 15-C06, 15-D03, 15-D04, 15-E06, 15-E09, 15-F04, 15-F05, 15-G03, 15-G04, 15-G05, 15-I01 |
| M | 31 | 15-B01, 15-B03, 15-C01 through 15-C05, 15-C07, 15-D02, 15-D05 through 15-D10, 15-E02 through 15-E05, 15-E07, 15-E08, 15-F01 through 15-F03, 15-F06, 15-G01, 15-G02, 15-H01 through 15-H09, 15-I02 through 15-I04, 15-J01, 15-J02, 15-J04 |
| L | 4 | 15-D01, 15-E01, 15-F01, 15-J03 |
| **Total** | **52** | |

---

## Canon Updates Required

Based on the discussion, the following canon updates are needed:

1. **systems.yaml**: Change AudioSystem from `type: ecs_system` to `type: core`
2. **interfaces.yaml**: Add IAudioProvider interface definition
3. **patterns.yaml**: Add audio_patterns section with format recommendations

---

## Implementation Order Recommendation

### Phase 1: Foundation
15-A01 through 15-A08, 15-B01, 15-B02, 15-C01 through 15-C07

### Phase 2: Sound Effects
15-D01 through 15-D10

### Phase 3: Music
15-E01 through 15-E09

### Phase 4: Ambient
15-F01 through 15-F06

### Phase 5: Mixing
15-G01 through 15-G05

### Phase 6: Event Integration
15-H01 through 15-H09

### Phase 7: Settings & Polish
15-I01 through 15-I04, 15-J01 through 15-J04

---

## Notes

- AudioSystem is client-only; no network considerations
- All SFX preloaded (~20MB), music streamed (~256KB buffer)
- Supports user music directory for player-curated playlists
- Event-driven architecture minimizes coupling with game systems
- Alien sound design: crystalline, organic-electronic, bioluminescent aesthetic
