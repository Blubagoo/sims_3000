# Systems Architect Analysis: Epic 15 - Audio System

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**Canon Version:** 0.15.0
**Status:** ANALYSIS COMPLETE - Ready for cross-agent discussion

---

## Executive Summary

Epic 15 implements the **AudioSystem** - the audio infrastructure that provides music playback, sound effects, ambient audio, and volume mixing for the game. This is a **client-authoritative** system with no server synchronization.

Key architectural characteristics:
1. **Client-only system:** Audio state is purely local, never synced over network
2. **Event-driven:** AudioSystem subscribes to game events for automatic sound triggering
3. **SDL3 Audio Integration:** Uses SDL3's audio subsystem for playback
4. **Core System vs ECS System:** While systems.yaml lists AudioSystem as ecs_system, recommend Core System since it doesn't participate in the simulation tick

The AudioSystem is primarily a **service provider** - other systems emit events or call methods to request sounds, and AudioSystem handles mixing, playback, and volume management.

---

## Table of Contents

1. [System Boundaries](#1-system-boundaries)
2. [System Architecture](#2-system-architecture)
3. [Data Model](#3-data-model)
4. [Music System](#4-music-system)
5. [Sound Effect System](#5-sound-effect-system)
6. [Ambient Audio System](#6-ambient-audio-system)
7. [Positional Audio](#7-positional-audio)
8. [Volume Mixing](#8-volume-mixing)
9. [Event Integration](#9-event-integration)
10. [SDL3 Audio Integration](#10-sdl3-audio-integration)
11. [Key Work Items](#11-key-work-items)
12. [Questions for Other Agents](#12-questions-for-other-agents)
13. [Risks and Concerns](#13-risks-and-concerns)
14. [Dependencies](#14-dependencies)
15. [Proposed Interfaces](#15-proposed-interfaces)
16. [Canon Update Proposals](#16-canon-update-proposals)

---

## 1. System Boundaries

### 1.1 AudioSystem

**Type:** Core System (NOT ECS - doesn't participate in simulation tick)

**Owns:**
- Music playback (playlist, crossfade, looping)
- Sound effect playback (triggered by events)
- Ambient audio (colony activity hum, environmental)
- Volume mixing (per-channel volume control)
- Audio asset management (loading, caching, streaming)
- SDL3 audio device management

**Does NOT Own:**
- When to play sounds (other systems trigger via events)
- Sound asset files (AssetManager owns file loading)
- Audio settings persistence (PersistenceSystem owns)
- What sound is appropriate for an action (event mapping defined in config)

**Provides:**
- `IAudioProvider` interface for sound playback requests
- Play sound API (positional and non-positional)
- Volume controls (per-channel)
- Music control (play, pause, skip, shuffle)

**Depends On:**
- AssetManager (audio file loading)
- CameraSystem (listener position for positional audio)
- EventBus (subscribes to game events for automatic sound triggering)

### 1.2 Boundary Summary Table

| Concern | AudioSystem | AssetManager | EventBus | Systems |
|---------|-------------|--------------|----------|---------|
| Audio file loading | - | **Owns** | - | - |
| Sound playback | **Owns** | - | - | - |
| Sound triggering | Provides API | - | Delivers | Emit events |
| Volume settings | **Owns** | - | - | - |
| Music selection | **Owns** | - | - | - |
| Listener position | Queries | - | - | CameraSystem |

---

## 2. System Architecture

### 2.1 Core vs ECS System Decision

**Recommendation: Core System, NOT ECS System**

Rationale:
- AudioSystem does NOT participate in the simulation tick
- Audio state is not synchronized across network (client-authoritative)
- No AudioComponent on entities (we don't track "which entities make sound")
- Purely service-oriented: provides playback on request

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│                                                              │
│  ┌─────────────────┐   Events    ┌──────────────────────┐   │
│  │   Game Systems   │ ─────────> │    AudioSystem       │   │
│  │                  │            │    (Core System)     │   │
│  │  - BuildingSystem│            │                      │   │
│  │  - DisasterSystem│            │  ┌────────────────┐  │   │
│  │  - UISystem      │            │  │ MusicPlayer    │  │   │
│  │  - etc.          │            │  │ SFXPlayer      │  │   │
│  └─────────────────┘            │  │ AmbientPlayer  │  │   │
│                                  │  │ VolumeManager  │  │   │
│  ┌─────────────────┐            │  └────────────────┘  │   │
│  │   CameraSystem   │────────────│                      │   │
│  │   (listener pos) │            └──────────────────────┘   │
│  └─────────────────┘                       │                │
│                                            │                │
│  ┌─────────────────┐            ┌──────────▼─────────┐     │
│  │  AssetManager   │ ──────────>│   SDL3 Audio       │     │
│  │  (audio files)   │            │   Device           │     │
│  └─────────────────┘            └────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Component Architecture

AudioSystem doesn't use ECS components for audio entities. Instead:

```cpp
class AudioSystem {
private:
    // Music playback
    MusicPlayer music_player_;

    // Sound effect playback (pooled channels)
    SFXPlayer sfx_player_;

    // Ambient audio (looping layers)
    AmbientPlayer ambient_player_;

    // Volume control
    VolumeManager volume_manager_;

    // SDL3 audio device
    SDL_AudioDeviceID audio_device_;

    // Event subscriptions
    EventSubscriptions event_subs_;

public:
    // IAudioProvider interface
    void play_sound(SoundEffectType type);
    void play_sound_at(SoundEffectType type, Vec3 position);
    void set_volume(AudioChannel channel, float volume);
    void play_music();
    void pause_music();
    void stop_music();
    void skip_track();

    // Update (called from main loop, not simulation tick)
    void update(float delta_time);
};
```

---

## 3. Data Model

### 3.1 Audio Channel Configuration

```cpp
enum class AudioChannel : uint8_t {
    Master = 0,     // Controls all audio
    Music = 1,      // Background music
    SFX = 2,        // Sound effects
    Ambient = 3,    // Colony ambient sounds
    UI = 4          // UI sounds (clicks, notifications)
};

struct VolumeSettings {
    float master = 1.0f;    // 0.0 - 1.0
    float music = 0.7f;     // 0.0 - 1.0
    float sfx = 1.0f;       // 0.0 - 1.0
    float ambient = 0.5f;   // 0.0 - 1.0
    float ui = 1.0f;        // 0.0 - 1.0
};
```

### 3.2 Sound Effect Types

```cpp
enum class SoundEffectType : uint16_t {
    // Construction (0-99)
    ZoneDesignated = 0,
    BuildingMaterializing = 1,
    BuildingComplete = 2,
    Demolition = 3,

    // Infrastructure (100-199)
    PathwayPlaced = 100,
    ConduitPlaced = 101,
    PipePlaced = 102,
    InfrastructureConnected = 103,

    // UI (200-299)
    ButtonClick = 200,
    ToolSelected = 201,
    MenuOpen = 202,
    MenuClose = 203,
    AlertPulse = 204,
    NotificationDismiss = 205,

    // Simulation (300-399)
    DisasterWarning = 300,
    DisasterActive = 301,
    DisasterEnded = 302,
    MilestoneAchieved = 303,
    PopulationThreshold = 304,

    // Ambient (400-499)
    ColonyHum = 400,
    WindEnvironmental = 401,
    BioluminescentPulse = 402
};
```

### 3.3 Music Track

```cpp
struct MusicTrack {
    std::string file_path;          // Path to audio file
    std::string title;              // Display name (optional)
    float duration_seconds;         // Track length
    float volume_multiplier = 1.0f; // Per-track volume adjustment
};

struct Playlist {
    std::vector<MusicTrack> tracks;
    bool shuffle = false;
    bool repeat = true;
    uint32_t current_track_index = 0;
};
```

### 3.4 Sound Effect Asset

```cpp
struct SoundEffectAsset {
    SoundEffectType type;
    std::string file_path;
    float base_volume = 1.0f;
    float min_pitch = 0.95f;        // Pitch variation
    float max_pitch = 1.05f;
    float max_distance = 100.0f;    // For positional audio (in tiles)
    float reference_distance = 10.0f;
    bool is_looping = false;
    uint32_t max_instances = 5;     // Max simultaneous plays
};
```

---

## 4. Music System

### 4.1 MusicPlayer Design

```cpp
class MusicPlayer {
private:
    Playlist current_playlist_;
    SDL_AudioStream* music_stream_;

    float crossfade_duration_ = 3.0f;  // Seconds (canon: crossfade between tracks)
    float current_crossfade_ = 0.0f;
    bool is_crossfading_ = false;

    MusicState state_ = MusicState::Stopped;
    float current_position_ = 0.0f;

public:
    void set_playlist(const Playlist& playlist);
    void play();
    void pause();
    void stop();
    void skip();
    void toggle_shuffle();

    void update(float delta_time);

    // Queries
    MusicState get_state() const;
    const MusicTrack* get_current_track() const;
    float get_position() const;
    float get_duration() const;
};

enum class MusicState : uint8_t {
    Stopped,
    Playing,
    Paused,
    Crossfading
};
```

### 4.2 Crossfade Implementation

```cpp
void MusicPlayer::start_crossfade_to_next() {
    if (current_playlist_.tracks.empty()) return;

    is_crossfading_ = true;
    current_crossfade_ = 0.0f;

    // Prepare next track
    uint32_t next_index = get_next_track_index();
    load_track(current_playlist_.tracks[next_index]);
}

void MusicPlayer::update_crossfade(float delta_time) {
    if (!is_crossfading_) return;

    current_crossfade_ += delta_time;
    float t = current_crossfade_ / crossfade_duration_;

    if (t >= 1.0f) {
        // Crossfade complete
        complete_track_transition();
        is_crossfading_ = false;
    } else {
        // Blend volumes
        float outgoing_volume = 1.0f - t;
        float incoming_volume = t;

        set_track_volume(current_track_, outgoing_volume);
        set_track_volume(next_track_, incoming_volume);
    }
}
```

### 4.3 User Music Directory

Canon specifies "user-curated ambient tracks". Support loading from user directory:

```cpp
void AudioSystem::scan_user_music_directory() {
    // Platform-specific paths
    #ifdef _WIN32
    std::string music_path = get_user_data_path() + "/music/";
    #else
    std::string music_path = get_user_data_path() + "/music/";
    #endif

    // Supported formats: MP3, OGG, WAV, FLAC
    std::vector<std::string> supported = {".mp3", ".ogg", ".wav", ".flac"};

    for (const auto& entry : std::filesystem::directory_iterator(music_path)) {
        if (is_supported_format(entry.path().extension())) {
            add_to_playlist(entry.path().string());
        }
    }
}
```

---

## 5. Sound Effect System

### 5.1 SFXPlayer Design

```cpp
class SFXPlayer {
private:
    std::unordered_map<SoundEffectType, SoundEffectAsset> assets_;
    std::vector<ActiveSound> active_sounds_;

    static constexpr uint32_t MAX_ACTIVE_SOUNDS = 32;

    struct ActiveSound {
        SoundEffectType type;
        SDL_AudioStream* stream;
        float volume;
        Vec3 position;
        bool is_positional;
        float time_remaining;
    };

public:
    void load_sound(SoundEffectType type, const SoundEffectAsset& asset);

    void play(SoundEffectType type);
    void play_at(SoundEffectType type, Vec3 position);
    void stop_all(SoundEffectType type);

    void update(float delta_time, Vec3 listener_position);

private:
    float calculate_positional_volume(Vec3 sound_pos, Vec3 listener_pos, const SoundEffectAsset& asset);
    float apply_pitch_variation(const SoundEffectAsset& asset);
};
```

### 5.2 Sound Priority System

When MAX_ACTIVE_SOUNDS reached, prioritize:

```cpp
enum class SoundPriority : uint8_t {
    Critical = 0,   // Always play (disasters, milestones)
    High = 1,       // UI sounds, building complete
    Normal = 2,     // Construction sounds
    Low = 3         // Ambient variations
};

const std::map<SoundEffectType, SoundPriority> SOUND_PRIORITIES = {
    {SoundEffectType::DisasterWarning, SoundPriority::Critical},
    {SoundEffectType::MilestoneAchieved, SoundPriority::Critical},
    {SoundEffectType::AlertPulse, SoundPriority::High},
    {SoundEffectType::BuildingComplete, SoundPriority::High},
    {SoundEffectType::ButtonClick, SoundPriority::High},
    {SoundEffectType::ZoneDesignated, SoundPriority::Normal},
    {SoundEffectType::PathwayPlaced, SoundPriority::Normal},
    {SoundEffectType::BioluminescentPulse, SoundPriority::Low}
};
```

### 5.3 Instance Limiting

Prevent sound spam (e.g., rapid-clicking zone tool):

```cpp
bool SFXPlayer::can_play(SoundEffectType type) {
    const auto& asset = assets_[type];

    uint32_t current_instances = std::count_if(
        active_sounds_.begin(), active_sounds_.end(),
        [type](const ActiveSound& s) { return s.type == type; }
    );

    return current_instances < asset.max_instances;
}
```

---

## 6. Ambient Audio System

### 6.1 AmbientPlayer Design

```cpp
class AmbientPlayer {
private:
    // Layered ambient sounds
    struct AmbientLayer {
        SoundEffectType type;
        SDL_AudioStream* stream;
        float target_volume;
        float current_volume;
        float fade_speed;
        bool is_looping;
    };

    std::vector<AmbientLayer> layers_;

    // Colony activity hum scaling
    float colony_activity_level_ = 0.0f;  // 0.0 - 1.0 based on population

public:
    void set_layer_active(SoundEffectType type, bool active);
    void set_layer_volume(SoundEffectType type, float volume);

    void set_colony_activity_level(float level);

    void update(float delta_time);
};
```

### 6.2 Colony Activity Hum

Canon specifies "Colony activity hum (scales with population)":

```cpp
void AmbientPlayer::update_colony_hum(uint32_t population) {
    // Scale from 0 (no population) to 1.0 (100K+ beings)
    float activity = std::min(1.0f, population / 100000.0f);

    // Apply non-linear curve for better feel
    activity = std::sqrt(activity);  // Faster initial ramp

    set_colony_activity_level(activity);

    // Adjust hum volume and intensity
    float hum_volume = 0.2f + (activity * 0.3f);  // 0.2 - 0.5 range
    set_layer_volume(SoundEffectType::ColonyHum, hum_volume);
}
```

### 6.3 Environmental Layers

```cpp
void AmbientPlayer::setup_environment_layers() {
    // Base environmental sounds
    add_layer(SoundEffectType::WindEnvironmental, {
        .is_looping = true,
        .base_volume = 0.3f
    });

    // Bioluminescent pulses (periodic)
    add_layer(SoundEffectType::BioluminescentPulse, {
        .is_looping = true,
        .base_volume = 0.2f
    });

    // Colony activity hum
    add_layer(SoundEffectType::ColonyHum, {
        .is_looping = true,
        .base_volume = 0.0f  // Scales with population
    });
}
```

---

## 7. Positional Audio

### 7.1 3D Sound Design

The game uses 2.5D isometric view, so positional audio is simplified:

```cpp
struct ListenerState {
    Vec3 position;      // Camera target position
    Vec3 forward;       // Camera forward direction
    float zoom_level;   // Current zoom (affects attenuation)
};

float SFXPlayer::calculate_positional_volume(
    Vec3 sound_pos,
    const ListenerState& listener,
    const SoundEffectAsset& asset
) {
    // Distance in tile units
    float distance = Vec3::distance(sound_pos, listener.position);

    // Adjust for zoom (zoomed out = larger audible area)
    float effective_distance = distance / listener.zoom_level;

    // Linear falloff with reference distance
    if (effective_distance <= asset.reference_distance) {
        return 1.0f;
    }

    float attenuation = asset.reference_distance / effective_distance;
    attenuation = std::max(0.0f, attenuation);

    // Cut off at max distance
    if (effective_distance >= asset.max_distance) {
        return 0.0f;
    }

    return attenuation;
}
```

### 7.2 Pan Calculation

For stereo panning based on screen position:

```cpp
float SFXPlayer::calculate_pan(Vec3 sound_pos, const ListenerState& listener) {
    // Calculate relative position to listener
    Vec3 relative = sound_pos - listener.position;

    // Project onto horizontal plane (ignore Y)
    Vec2 horizontal(relative.x, relative.z);

    // Calculate angle
    float angle = std::atan2(horizontal.x, horizontal.y);

    // Convert to pan (-1.0 = left, 1.0 = right)
    float pan = std::sin(angle);

    // Reduce pan intensity for distant sounds
    float distance = horizontal.length();
    float pan_falloff = std::min(1.0f, asset.reference_distance / distance);

    return pan * pan_falloff;
}
```

---

## 8. Volume Mixing

### 8.1 VolumeManager Design

```cpp
class VolumeManager {
private:
    VolumeSettings settings_;

    // Ducking state (lower music during important sounds)
    float music_duck_level_ = 1.0f;
    float duck_recovery_speed_ = 2.0f;  // Seconds to recover

public:
    void set_volume(AudioChannel channel, float volume);
    float get_volume(AudioChannel channel) const;

    float get_effective_volume(AudioChannel channel) const;

    // Ducking
    void duck_music(float level, float duration);
    void update(float delta_time);

    // Persistence
    VolumeSettings get_settings() const;
    void set_settings(const VolumeSettings& settings);
};

float VolumeManager::get_effective_volume(AudioChannel channel) const {
    float channel_volume = get_channel_volume(channel);
    float master = settings_.master;

    if (channel == AudioChannel::Music) {
        return channel_volume * master * music_duck_level_;
    }

    return channel_volume * master;
}
```

### 8.2 Music Ducking

Duck music during important sounds (milestones, disasters):

```cpp
void AudioSystem::on_milestone_achieved(const MilestoneAchievedEvent& event) {
    // Duck music to 30% for 3 seconds
    volume_manager_.duck_music(0.3f, 3.0f);

    // Play milestone fanfare
    sfx_player_.play(SoundEffectType::MilestoneAchieved);
}

void AudioSystem::on_disaster_active(const DisasterActiveEvent& event) {
    // Duck music to 50% during disaster
    volume_manager_.duck_music(0.5f, 0.0f);  // duration=0 means until recovered

    // Play disaster sound
    sfx_player_.play_at(SoundEffectType::DisasterActive, event.epicenter);
}
```

---

## 9. Event Integration

### 9.1 Event Subscriptions

AudioSystem subscribes to game events for automatic sound triggering:

```cpp
void AudioSystem::subscribe_to_events() {
    // Construction events (Epic 4)
    EventBus::subscribe<ZoneDesignatedEvent>([this](const auto& e) {
        sfx_player_.play_at(SoundEffectType::ZoneDesignated, e.position);
    });

    EventBus::subscribe<BuildingMaterializingEvent>([this](const auto& e) {
        sfx_player_.play_at(SoundEffectType::BuildingMaterializing, e.position);
    });

    EventBus::subscribe<BuildingCompleteEvent>([this](const auto& e) {
        sfx_player_.play_at(SoundEffectType::BuildingComplete, e.position);
    });

    // Infrastructure events (Epic 5, 6, 7)
    EventBus::subscribe<InfrastructurePlacedEvent>([this](const auto& e) {
        SoundEffectType type = get_infrastructure_sound(e.infrastructure_type);
        sfx_player_.play_at(type, e.position);
    });

    // Disaster events (Epic 13)
    EventBus::subscribe<DisasterWarningEvent>([this](const auto& e) {
        volume_manager_.duck_music(0.6f, 5.0f);
        sfx_player_.play(SoundEffectType::DisasterWarning);
    });

    EventBus::subscribe<DisasterActiveEvent>([this](const auto& e) {
        sfx_player_.play_at(SoundEffectType::DisasterActive, e.epicenter);
    });

    // Progression events (Epic 14)
    EventBus::subscribe<MilestoneAchievedEvent>([this](const auto& e) {
        volume_manager_.duck_music(0.3f, 3.0f);
        sfx_player_.play(SoundEffectType::MilestoneAchieved);
    });

    // UI events (Epic 12)
    EventBus::subscribe<UIToolSelectedEvent>([this](const auto& e) {
        sfx_player_.play(SoundEffectType::ToolSelected);
    });

    EventBus::subscribe<UIAlertPulseEvent>([this](const auto& e) {
        sfx_player_.play(SoundEffectType::AlertPulse);
    });
}
```

### 9.2 Population Updates

For colony activity hum scaling:

```cpp
void AudioSystem::on_population_changed(const PopulationChangedEvent& event) {
    ambient_player_.update_colony_hum(event.new_population);
}
```

---

## 10. SDL3 Audio Integration

### 10.1 Audio Device Setup

```cpp
bool AudioSystem::initialize() {
    // Initialize SDL audio subsystem
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        log_error("Failed to initialize SDL audio: {}", SDL_GetError());
        return false;
    }

    // Get default audio device
    SDL_AudioSpec desired_spec = {
        .format = SDL_AUDIO_F32,
        .channels = 2,          // Stereo
        .freq = 48000           // 48kHz
    };

    audio_device_ = SDL_OpenAudioDevice(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &desired_spec
    );

    if (audio_device_ == 0) {
        log_error("Failed to open audio device: {}", SDL_GetError());
        return false;
    }

    // Resume playback
    SDL_ResumeAudioDevice(audio_device_);

    return true;
}

void AudioSystem::shutdown() {
    if (audio_device_ != 0) {
        SDL_CloseAudioDevice(audio_device_);
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
```

### 10.2 Audio Streaming

SDL3 uses audio streams for mixing:

```cpp
SDL_AudioStream* AudioSystem::create_stream() {
    SDL_AudioSpec src_spec = {
        .format = SDL_AUDIO_F32,
        .channels = 2,
        .freq = 48000
    };

    return SDL_CreateAudioStream(&src_spec, &src_spec);
}

void AudioSystem::play_on_stream(SDL_AudioStream* stream, const AudioData& data) {
    SDL_PutAudioStreamData(stream, data.samples, data.size);
    SDL_BindAudioStream(audio_device_, stream);
}
```

### 10.3 Supported Formats

SDL3 with SDL_mixer or native loading:

- **WAV:** Native SDL3 support
- **OGG/Vorbis:** Recommended for SFX (good compression, no licensing)
- **MP3:** Supported for music (common user format)
- **FLAC:** Supported for music (lossless)

---

## 11. Key Work Items

### Phase 1: Core Infrastructure (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-001 | AudioChannel enum and VolumeSettings | S | Define channel types and settings struct |
| E15-002 | SoundEffectType enum | S | Define all sound effect types from canon |
| E15-003 | SDL3 audio device initialization | M | Initialize and configure audio device |
| E15-004 | VolumeManager implementation | M | Channel volumes and music ducking |
| E15-005 | AudioSystem class skeleton | M | Core class with lifecycle methods |
| E15-006 | IAudioProvider interface | M | Define interface for sound requests |
| E15-007 | Audio asset loading integration | M | Load sounds via AssetManager |
| E15-008 | Audio configuration file format | S | Define JSON/YAML for sound mappings |

### Phase 2: Sound Effects (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-009 | SFXPlayer implementation | L | Sound effect playback with pooling |
| E15-010 | Sound priority system | M | Priority-based channel allocation |
| E15-011 | Instance limiting | S | Prevent sound spam |
| E15-012 | Pitch variation | S | Add randomness to sounds |
| E15-013 | Positional audio calculation | M | Distance/pan for 3D sounds |
| E15-014 | CameraSystem listener integration | M | Get listener position from camera |

### Phase 3: Music System (6 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-015 | MusicPlayer implementation | L | Playlist playback with state machine |
| E15-016 | Crossfade implementation | M | 3-second crossfade between tracks |
| E15-017 | User music directory scanning | M | Load from user music folder |
| E15-018 | Shuffle and repeat modes | S | Playlist order options |
| E15-019 | Track streaming | M | Stream large audio files |
| E15-020 | Music controls (UI integration) | M | Play/pause/skip interface |

### Phase 4: Ambient Audio (5 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-021 | AmbientPlayer implementation | M | Layered ambient sound system |
| E15-022 | Colony activity hum | M | Population-scaled ambient layer |
| E15-023 | Environmental layers | M | Wind, bioluminescent pulses |
| E15-024 | Ambient volume fading | S | Smooth layer transitions |
| E15-025 | Pause behavior | S | Handle ambient during game pause |

### Phase 5: Event Integration (8 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-026 | Construction event sounds | M | Zone, building, demolition sounds |
| E15-027 | Infrastructure event sounds | M | Pathway, conduit, pipe sounds |
| E15-028 | UI event sounds | M | Clicks, tools, menus, alerts |
| E15-029 | Disaster event sounds | M | Warning, active, ended sounds |
| E15-030 | Progression event sounds | M | Milestones, population thresholds |
| E15-031 | Event subscription setup | M | Subscribe to all game events |
| E15-032 | Sound-to-event mapping config | M | Data-driven event mapping |
| E15-033 | Music ducking triggers | S | Duck on important events |

### Phase 6: Settings & Persistence (4 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-034 | Volume settings serialization | S | Save/load volume preferences |
| E15-035 | Audio settings UI integration | M | Volume sliders in settings |
| E15-036 | Mute/unmute functionality | S | Quick mute toggles |
| E15-037 | Audio device selection | M | Choose output device (optional) |

### Phase 7: Testing (3 items)

| ID | Work Item | Size | Description |
|----|-----------|------|-------------|
| E15-038 | Unit tests for volume mixing | M | Test volume calculations |
| E15-039 | Integration test for events | M | Test event-to-sound mapping |
| E15-040 | Audio playback stress test | M | Test max simultaneous sounds |

---

## Work Item Summary

| Size | Count | Items |
|------|-------|-------|
| S | 9 | E15-001, E15-002, E15-008, E15-011, E15-012, E15-018, E15-024, E15-025, E15-033, E15-034, E15-036 |
| M | 24 | E15-003 through E15-007, E15-010, E15-013 through E15-014, E15-016 through E15-017, E15-019 through E15-023, E15-026 through E15-032, E15-035, E15-037 through E15-040 |
| L | 2 | E15-009, E15-015 |
| **Total** | **40** | |

---

## 12. Questions for Other Agents

### @game-designer

1. **Sound effect variety:** Should there be multiple sound variants per action (e.g., 3-5 building complete sounds that play randomly)? This adds variety but increases asset requirements.

2. **Disaster audio intensity:** Should disaster sounds scale with severity? E.g., conflagration intensity affects sound intensity?

3. **Milestone fanfare duration:** How long should milestone achievement sounds be? Short confirmation (1-2 sec) or celebratory fanfare (5-10 sec)?

4. **Other player notification sounds:** When another player achieves a milestone, should there be audio feedback? If so, what level - same as own, reduced, or subtle chime?

5. **Ambient during pause:** Should ambient audio continue during pause, or fade out?

6. **Disaster-specific sounds:** Should each disaster type have unique sounds (conflagration = crackling, seismic = rumbling, inundation = rushing water)?

7. **Building type sounds:** Should different building densities have different completion sounds?

8. **Music mood:** Should music change based on game state (e.g., disaster music during catastrophe)?

---

## 13. Risks and Concerns

### 13.1 Technical Risks

**RISK: Audio Memory Usage (LOW)**
Loading all sounds at startup may use significant memory.

**Mitigation:**
- Stream music files, only load SFX into memory
- Implement sound pooling with LRU eviction for large sound sets
- Use compressed formats (OGG) for SFX

**RISK: SDL3 Audio API Stability (LOW)**
SDL3 is relatively new; API may evolve.

**Mitigation:**
- Abstract SDL3 calls behind AudioSystem interface
- Track SDL3 release notes for breaking changes
- Use stable subset of API

**RISK: Audio Synchronization (MEDIUM)**
Many simultaneous sounds may cause audio glitches.

**Mitigation:**
- Limit concurrent sounds (MAX_ACTIVE_SOUNDS = 32)
- Priority system drops low-priority sounds
- Use audio streaming with adequate buffering

### 13.2 Design Ambiguities

**AMBIGUITY: Music Source**
Canon says "user-curated ambient tracks" - does this mean:
A) Users must provide their own music (no bundled music)?
B) We provide default music + users can add their own?

**Recommendation:** Provide minimal default ambient tracks + user music folder support.

**AMBIGUITY: Positional Audio Extent**
How far should positional audio travel? Zoomed-out view covers large area.

**Recommendation:** Scale audible range with zoom level. Zoomed out = larger audible area.

---

## 14. Dependencies

### 14.1 What Epic 15 Needs from Earlier Epics

| From Epic | What | How Used |
|-----------|------|----------|
| Epic 0 | AssetManager | Audio file loading |
| Epic 0 | Application | Main loop update call |
| Epic 2 | CameraSystem | Listener position for positional audio |
| Epic 4 | Building events | Construction sounds |
| Epic 5-7 | Infrastructure events | Infrastructure sounds |
| Epic 10 | Population data | Colony hum scaling |
| Epic 12 | UI events | UI sounds |
| Epic 13 | Disaster events | Disaster sounds |
| Epic 14 | Milestone events | Progression sounds |

### 14.2 What Later Epics Need from Epic 15

| Epic | What They Need | How Provided |
|------|----------------|--------------|
| Epic 16 (Persistence) | Volume settings | VolumeSettings serialization |
| Epic 17 (Polish) | Audio polish | Fine-tuning hooks |

### 14.3 Stub Requirements

If implementing Epic 15 before Epic 13 (Disasters):

```cpp
// AudioSystem can still setup subscriptions
// They just won't fire until disaster events exist
EventBus::subscribe<DisasterWarningEvent>([this](const auto& e) {
    // Will be called when Epic 13 emits this event
});
```

No stubs required - event-driven architecture handles missing emitters gracefully.

---

## 15. Proposed Interfaces

### 15.1 IAudioProvider Interface

```cpp
class IAudioProvider {
public:
    virtual ~IAudioProvider() = default;

    // === Sound Effect Playback ===

    // Play non-positional sound
    virtual void play_sound(SoundEffectType type) = 0;

    // Play sound at world position
    virtual void play_sound_at(SoundEffectType type, Vec3 position) = 0;

    // Stop all instances of a sound type
    virtual void stop_sound(SoundEffectType type) = 0;

    // === Volume Control ===

    // Set channel volume (0.0 - 1.0)
    virtual void set_volume(AudioChannel channel, float volume) = 0;

    // Get channel volume
    virtual float get_volume(AudioChannel channel) const = 0;

    // === Music Control ===

    virtual void play_music() = 0;
    virtual void pause_music() = 0;
    virtual void stop_music() = 0;
    virtual void skip_track() = 0;

    virtual bool is_music_playing() const = 0;

    // === Queries ===

    virtual bool is_sound_playing(SoundEffectType type) const = 0;
    virtual uint32_t get_active_sound_count() const = 0;
};
```

---

## 16. Canon Update Proposals

### 16.1 systems.yaml Update

Change AudioSystem from `ecs_system` to `core`:

```yaml
phase_5:
  epic_15_audio:
    name: "Audio System"
    systems:
      AudioSystem:
        type: core  # Changed from ecs_system - doesn't participate in tick
        owns:
          - Music playback (playlist, crossfade, looping)
          - Sound effect playback (positional and non-positional)
          - Ambient audio (colony hum, environmental layers)
          - Volume mixing (per-channel control)
          - Audio asset management
          - SDL3 audio device
        does_not_own:
          - When to play sounds (event-driven)
          - Audio file loading (AssetManager owns)
          - Settings persistence (PersistenceSystem owns)
        provides:
          - "IAudioProvider: sound and music playback API"
          - Volume controls
          - Music playlist management
        depends_on:
          - AssetManager (audio files)
          - CameraSystem (listener position)
        multiplayer:
          authority: client  # Audio is purely client-side
          per_player: true
```

### 16.2 interfaces.yaml Addition

```yaml
audio:
  IAudioProvider:
    description: "Provides audio playback services for sound effects and music"
    purpose: "Central interface for all sound requests"

    methods:
      - name: play_sound
        params:
          - name: type
            type: SoundEffectType
        returns: void
        description: "Play non-positional sound effect"

      - name: play_sound_at
        params:
          - name: type
            type: SoundEffectType
          - name: position
            type: Vec3
        returns: void
        description: "Play positional sound effect"

      - name: set_volume
        params:
          - name: channel
            type: AudioChannel
          - name: volume
            type: float
        returns: void
        description: "Set channel volume (0.0 - 1.0)"

      - name: play_music
        params: []
        returns: void
        description: "Start/resume music playback"

      - name: pause_music
        params: []
        returns: void
        description: "Pause music playback"

    implemented_by:
      - AudioSystem (Epic 15)

    notes:
      - "Client-authoritative - audio state is never synchronized"
      - "Event-driven - systems emit events, AudioSystem reacts"
```

---

## Summary

Epic 15 (Audio System) is a **client-only core system** that:

1. Provides **music playback** with playlist, crossfade, and user music support
2. Implements **sound effects** with positional audio and priority system
3. Creates **ambient audio** layers that scale with colony activity
4. Integrates via **event subscriptions** for automatic sound triggering
5. Uses **SDL3 audio API** for cross-platform playback

The epic is scoped to **40 work items** covering SDL3 integration, music system, sound effects, ambient audio, event integration, and settings.

**Key Characteristics:**
- Client-authoritative (no network sync)
- Event-driven (reacts to game events)
- Core System (not ECS simulation participant)
- Data-driven (sound mappings in config files)

**Critical Integration Points:**
- AssetManager for audio file loading
- CameraSystem for listener position
- EventBus for game event subscriptions
- PersistenceSystem for volume settings

---

**End of Systems Architect Analysis: Epic 15 - Audio System**
