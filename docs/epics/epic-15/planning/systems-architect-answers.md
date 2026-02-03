# Systems Architect Answers: Epic 15 - Audio System

**Author:** Systems Architect Agent
**Date:** 2026-02-01
**In Response To:** Game Designer questions (GD-1 through GD-5)
**Status:** COMPLETE

---

## GD-1: AudioSystem as ECS or Core?

**Question:** "AudioSystem as ECS or Core? systems.yaml says ecs_system, but audio doesn't really participate in simulation tick. What's the recommendation?"

**Answer:**

I recommend **Core System**, not ECS System. While systems.yaml currently lists AudioSystem as `ecs_system`, this classification is architecturally incorrect for several reasons:

1. **No Simulation Participation:** AudioSystem does not participate in the server-authoritative simulation tick. Audio is purely client-side feedback - it reacts to game events but doesn't affect game state. ECS systems in our architecture run at the 20Hz simulation tick rate and contribute to deterministic game state that must sync across network.

2. **No Components:** Unlike ECS systems that operate on component queries (e.g., EnergySystem queries EnergyComponent), AudioSystem has no AudioComponent attached to entities. We don't track "which entities make sound" in the ECS sense - instead, we react to events and play sounds through a service interface.

3. **Client Authority:** Per our multiplayer patterns in patterns.yaml, "Audio/visual effects" are explicitly listed as client-authoritative. The server never needs to know what sounds are playing.

The AudioSystem should be implemented as a Core System (like Application, AssetManager, InputSystem) that:
- Is updated from the main loop (render frame timing), not simulation tick
- Provides `IAudioProvider` interface for other systems to request sounds
- Subscribes to EventBus for automatic event-driven sound triggering

**Recommendation:** Update systems.yaml to change AudioSystem from `type: ecs_system` to `type: core`. This aligns with the architectural reality that audio is a service provider, not a simulation participant.

---

## GD-2: Interface for Triggering Sounds

**Question:** "What interface do systems use to trigger sounds? EventBus subscription or direct IAudioProvider calls?"

**Answer:**

**Both approaches are valid and serve different purposes.** The recommended pattern is:

**EventBus Subscription (Primary - Automatic)**
AudioSystem subscribes to game events and automatically plays appropriate sounds. This is the cleanest approach for most game actions:

```cpp
// AudioSystem internally subscribes during initialization
EventBus::subscribe<BuildingCompleteEvent>([this](const auto& e) {
    sfx_player_.play_at(SoundEffectType::BuildingComplete, e.position);
});

EventBus::subscribe<MilestoneAchievedEvent>([this](const auto& e) {
    volume_manager_.duck_music(0.3f, 3.0f);
    sfx_player_.play(SoundEffectType::MilestoneAchieved);
});
```

This approach:
- Decouples game systems from audio implementation details
- Follows the existing event pattern in interfaces.yaml (UIToolSelectedEvent, UIBuildRequestEvent, etc.)
- Allows sound mapping to be data-driven (config file defines which events trigger which sounds)
- Other systems just emit events normally; they don't need to know AudioSystem exists

**Direct IAudioProvider Calls (Secondary - Explicit Control)**
For cases where systems need explicit control over audio, IAudioProvider provides a direct API:

```cpp
// UISystem might call directly for immediate UI feedback
audio_provider_->play_sound(SoundEffectType::ButtonClick);
```

This is appropriate for:
- UI sounds that need immediate, synchronous feedback
- Sounds that don't correspond to a game event (e.g., hover sounds)
- Cases where the caller needs to control sound state (stop, volume)

**Recommendation:** Use EventBus subscription as the primary pattern (90% of cases). Define a sound-to-event mapping in a config file. Use direct IAudioProvider calls only for UI sounds and special cases requiring explicit control.

---

## GD-3: SDL3 Audio Format Support

**Question:** "What audio formats does SDL3 support? What's recommended for SFX vs music?"

**Answer:**

SDL3's audio subsystem provides direct support for **WAV** format natively. For other formats, we'll use SDL_mixer (or a similar library like miniaudio) which extends support to:

- **WAV** - Native SDL3 support, uncompressed
- **OGG/Vorbis** - Open source, good compression, no licensing issues
- **MP3** - Widely supported, good for user-provided music
- **FLAC** - Lossless compression, high quality
- **OPUS** - Modern codec, excellent compression (if using SDL_mixer 2.6+)

**Recommendations by Use Case:**

| Use Case | Format | Rationale |
|----------|--------|-----------|
| **Sound Effects** | **OGG/Vorbis** | Good compression (~10:1), no licensing fees, fast decode, good quality. Most SFX are short and can be fully loaded into memory. |
| **UI Sounds** | **OGG** or **WAV** | Short files; WAV acceptable for tiny sounds (<100KB). OGG for anything larger. |
| **Music (Bundled)** | **OGG/Vorbis** | Streaming-friendly, no licensing concerns, good quality at 128-192kbps. |
| **Music (User-Provided)** | **MP3, OGG, FLAC** | Support common formats users have. MP3 is ubiquitous. FLAC for audiophiles. |

**Technical Notes:**
- SFX should be pre-loaded into memory (small files, fast access)
- Music should be streamed (large files, don't consume memory)
- Sample rate: 48kHz standard for all audio
- Channels: Stereo (2) for music, mono (1) acceptable for SFX
- SDL3 audio API uses `SDL_AudioSpec` with format `SDL_AUDIO_F32` (32-bit float) internally

**Recommendation:** Use OGG/Vorbis as the primary format for all bundled audio. Support MP3/FLAC for user music directory. Avoid proprietary formats.

---

## GD-4: Positional Sound Travel Distance

**Question:** "How far should positional sounds travel? Is there a max range?"

**Answer:**

**Yes, positional sounds should have configurable max range, with sensible defaults.**

For our 2.5D isometric game with free camera and zoom, positional audio needs to account for the variable viewing area. I recommend a **zoom-aware distance model**:

**Default Distance Values (in tile units):**

| Sound Category | Reference Distance | Max Distance | Notes |
|---------------|-------------------|--------------|-------|
| **Building Events** | 10 tiles | 50 tiles | Construction, completion, demolition |
| **Infrastructure** | 8 tiles | 40 tiles | Pathway, conduit, pipe placement |
| **Disasters** | 20 tiles | 100 tiles | Should be heard from far away |
| **Ambient Loops** | 15 tiles | 60 tiles | Colony hum, environmental |
| **UI Sounds** | N/A | N/A | Non-positional (always full volume) |

**Zoom Level Adjustment:**
The effective audible distance should scale with camera zoom level:

```cpp
float effective_distance = actual_distance / camera_zoom_level;
```

When zoomed out (zoom_level > 1.0), sounds travel "further" (more tiles visible, larger audible area). When zoomed in (zoom_level < 1.0), sounds are more localized.

**Attenuation Model:**
Linear falloff from reference distance to max distance:

```cpp
if (distance <= reference_distance) {
    volume = 1.0f;  // Full volume within reference distance
} else if (distance >= max_distance) {
    volume = 0.0f;  // Silent beyond max distance
} else {
    // Linear falloff
    volume = 1.0f - (distance - reference_distance) / (max_distance - reference_distance);
}
```

**Recommendation:** Define distance parameters per-sound in the audio config file (SoundEffectAsset). Defaults: reference_distance=10, max_distance=50. Scale with zoom for consistent feel across camera distances.

---

## GD-5: Music File Streaming vs Preloading

**Question:** "Should large music files stream, or preload everything?"

**Answer:**

**Music should stream. Sound effects should preload.** This is the standard approach for good reasons:

**Music (Stream):**
- Music files are large (typically 3-10 MB per track for compressed formats)
- A playlist of 10 tracks = 30-100 MB if preloaded
- Only one track plays at a time (or two during crossfade)
- Streaming uses minimal memory (~64-128 KB buffer per stream)
- Disk/IO latency is acceptable for music (we can buffer ahead)

**Sound Effects (Preload):**
- SFX files are small (typically 10-500 KB each)
- SFX need instant playback (no latency acceptable for UI clicks, actions)
- Multiple SFX may play simultaneously (up to 32 in our design)
- Total SFX memory budget: ~10-20 MB for all sounds

**Implementation Approach:**

```cpp
// Music: Stream from disk
class MusicPlayer {
    SDL_AudioStream* current_stream_;  // Streams from file
    SDL_AudioStream* crossfade_stream_; // For crossfade overlap
    // Buffer ahead ~2-3 seconds
};

// SFX: Preloaded into memory
class SFXPlayer {
    std::unordered_map<SoundEffectType, AudioBuffer> loaded_sounds_;
    // All SFX loaded at startup or on first use
};
```

**Loading Strategy:**
1. **Startup:** Load all SFX into memory (~10-20 MB)
2. **Music:** Open stream when track starts, close when done
3. **Crossfade:** Maintain two streams briefly during transition
4. **User Music:** Scan directory but don't preload - stream on demand

**Memory Budget:**
- SFX Pool: ~20 MB (all sounds loaded)
- Music Streams: ~256 KB (2 x 128 KB buffers for crossfade)
- Total Audio Memory: ~20-25 MB

**Recommendation:** Stream all music files, preload all sound effects. This gives instant SFX response while keeping memory usage reasonable. The crossfade feature requires two simultaneous streams, which is still lightweight.

---

## Summary

| Question | Answer |
|----------|--------|
| GD-1: ECS or Core? | **Core System** - update systems.yaml |
| GD-2: EventBus or direct? | **Both** - EventBus primary, IAudioProvider for explicit control |
| GD-3: Audio formats? | **OGG/Vorbis** for bundled, support MP3/FLAC for user music |
| GD-4: Positional range? | **50 tiles max** default, zoom-aware scaling |
| GD-5: Stream or preload? | **Stream music, preload SFX** |

These answers align with our existing canon patterns and provide a solid technical foundation for Epic 15 implementation.

---

**End of Systems Architect Answers**
