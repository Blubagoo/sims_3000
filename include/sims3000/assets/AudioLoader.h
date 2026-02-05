/**
 * @file AudioLoader.h
 * @brief Audio asset loading (sound effects and music).
 *
 * Supports WAV and OGG formats.
 * Sound effects are fully loaded into memory.
 * Music may be streamed for large files.
 */

#ifndef SIMS3000_ASSETS_AUDIOLOADER_H
#define SIMS3000_ASSETS_AUDIOLOADER_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace sims3000 {

/**
 * @struct AudioChunk
 * @brief Sound effect data (fully loaded in memory).
 */
struct AudioChunk {
    std::vector<std::uint8_t> data;
    std::uint32_t sampleRate = 44100;
    std::uint16_t channels = 2;
    std::uint16_t bitsPerSample = 16;
    float duration = 0.0f;  // Duration in seconds

    bool isValid() const { return !data.empty(); }
};

/**
 * @struct Music
 * @brief Background music data.
 *
 * May be streamed for large files.
 */
struct Music {
    std::string path;
    std::vector<std::uint8_t> data;  // Empty if streaming
    std::uint32_t sampleRate = 44100;
    std::uint16_t channels = 2;
    float duration = 0.0f;
    bool streaming = false;

    bool isValid() const { return !path.empty(); }
};

using AudioChunkHandle = std::shared_ptr<AudioChunk>;
using MusicHandle = std::shared_ptr<Music>;

/**
 * @class AudioLoader
 * @brief Loads audio files (WAV, OGG).
 */
class AudioLoader {
public:
    AudioLoader() = default;
    ~AudioLoader() = default;

    /**
     * Load a sound effect from file.
     * @param path Path to audio file
     * @return Loaded audio chunk, or null on failure
     */
    AudioChunkHandle loadChunk(const std::string& path);

    /**
     * Load background music from file.
     * @param path Path to audio file
     * @param stream If true, stream from disk instead of loading fully
     * @return Loaded music, or null on failure
     */
    MusicHandle loadMusic(const std::string& path, bool stream = false);

    /**
     * Check if a file format is supported.
     * @param path File path (checks extension)
     * @return true if format is supported
     */
    static bool isSupported(const std::string& path);

private:
    AudioChunkHandle loadWAV(const std::string& path);
    // OGG loading would require stb_vorbis or similar
};

/**
 * @class MusicPlaylist
 * @brief Playlist for background music with crossfade support.
 */
class MusicPlaylist {
public:
    MusicPlaylist() = default;
    ~MusicPlaylist() = default;

    /**
     * Add a track to the playlist.
     * @param music Music to add
     */
    void addTrack(MusicHandle music);

    /**
     * Remove a track from the playlist.
     * @param index Track index
     */
    void removeTrack(std::size_t index);

    /**
     * Clear all tracks.
     */
    void clear();

    /**
     * Get number of tracks.
     */
    std::size_t size() const { return m_tracks.size(); }

    /**
     * Get current track index.
     */
    std::size_t getCurrentIndex() const { return m_currentIndex; }

    /**
     * Get current track.
     */
    MusicHandle getCurrentTrack() const;

    /**
     * Advance to next track.
     * @param loop If true, wrap around to beginning
     */
    void next(bool loop = true);

    /**
     * Go to previous track.
     * @param loop If true, wrap around to end
     */
    void previous(bool loop = true);

    /**
     * Shuffle the playlist.
     */
    void shuffle();

    /**
     * Set crossfade duration in seconds.
     */
    void setCrossfadeDuration(float seconds) { m_crossfadeDuration = seconds; }

    /**
     * Get crossfade duration.
     */
    float getCrossfadeDuration() const { return m_crossfadeDuration; }

private:
    std::vector<MusicHandle> m_tracks;
    std::size_t m_currentIndex = 0;
    float m_crossfadeDuration = 2.0f;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_AUDIOLOADER_H
