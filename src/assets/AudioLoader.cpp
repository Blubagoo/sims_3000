/**
 * @file AudioLoader.cpp
 * @brief Audio loading implementation.
 */

#include "sims3000/assets/AudioLoader.h"
#include <SDL3/SDL_log.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstring>

namespace sims3000 {

// WAV file header structures
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];           // "RIFF"
    std::uint32_t fileSize;
    char wave[4];           // "WAVE"
};

struct WAVFmtChunk {
    char fmt[4];            // "fmt "
    std::uint32_t chunkSize;
    std::uint16_t audioFormat;
    std::uint16_t numChannels;
    std::uint32_t sampleRate;
    std::uint32_t byteRate;
    std::uint16_t blockAlign;
    std::uint16_t bitsPerSample;
};

struct WAVDataChunk {
    char data[4];           // "data"
    std::uint32_t dataSize;
};
#pragma pack(pop)

AudioChunkHandle AudioLoader::loadChunk(const std::string& path) {
    // Check extension
    std::string ext;
    auto dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    if (ext == ".wav") {
        return loadWAV(path);
    }

    SDL_Log("AudioLoader: Unsupported format '%s'", ext.c_str());
    return nullptr;
}

AudioChunkHandle AudioLoader::loadWAV(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SDL_Log("AudioLoader: Failed to open '%s'", path.c_str());
        return nullptr;
    }

    // Read RIFF header
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        SDL_Log("AudioLoader: Invalid WAV file '%s'", path.c_str());
        return nullptr;
    }

    // Read fmt chunk
    WAVFmtChunk fmt;
    file.read(reinterpret_cast<char*>(&fmt), sizeof(fmt));

    if (std::strncmp(fmt.fmt, "fmt ", 4) != 0) {
        SDL_Log("AudioLoader: Missing fmt chunk in '%s'", path.c_str());
        return nullptr;
    }

    // Skip any extra format bytes
    if (fmt.chunkSize > 16) {
        file.seekg(fmt.chunkSize - 16, std::ios::cur);
    }

    // Find data chunk
    WAVDataChunk dataChunk;
    while (file.read(reinterpret_cast<char*>(&dataChunk), sizeof(dataChunk))) {
        if (std::strncmp(dataChunk.data, "data", 4) == 0) {
            break;
        }
        // Skip unknown chunk
        file.seekg(dataChunk.dataSize, std::ios::cur);
    }

    if (std::strncmp(dataChunk.data, "data", 4) != 0) {
        SDL_Log("AudioLoader: Missing data chunk in '%s'", path.c_str());
        return nullptr;
    }

    // Read audio data
    auto chunk = std::make_shared<AudioChunk>();
    chunk->data.resize(dataChunk.dataSize);
    file.read(reinterpret_cast<char*>(chunk->data.data()), dataChunk.dataSize);

    chunk->sampleRate = fmt.sampleRate;
    chunk->channels = fmt.numChannels;
    chunk->bitsPerSample = fmt.bitsPerSample;

    // Calculate duration
    std::uint32_t bytesPerSample = fmt.numChannels * (fmt.bitsPerSample / 8);
    std::uint32_t numSamples = dataChunk.dataSize / bytesPerSample;
    chunk->duration = static_cast<float>(numSamples) / static_cast<float>(fmt.sampleRate);

    SDL_Log("AudioLoader: Loaded '%s' (%.2fs, %uHz, %u channels)",
            path.c_str(), chunk->duration, chunk->sampleRate, chunk->channels);

    return chunk;
}

MusicHandle AudioLoader::loadMusic(const std::string& path, bool stream) {
    auto music = std::make_shared<Music>();
    music->path = path;
    music->streaming = stream;

    if (stream) {
        // For streaming, just validate file exists
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            SDL_Log("AudioLoader: Failed to open music '%s'", path.c_str());
            return nullptr;
        }
        SDL_Log("AudioLoader: Music '%s' ready for streaming", path.c_str());
    } else {
        // Load fully into memory
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            SDL_Log("AudioLoader: Failed to open music '%s'", path.c_str());
            return nullptr;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        music->data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(music->data.data()), size);

        SDL_Log("AudioLoader: Loaded music '%s' (%zu bytes)",
                path.c_str(), music->data.size());
    }

    return music;
}

bool AudioLoader::isSupported(const std::string& path) {
    std::string ext;
    auto dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    return ext == ".wav" || ext == ".ogg";
}

// MusicPlaylist implementation

void MusicPlaylist::addTrack(MusicHandle music) {
    if (music) {
        m_tracks.push_back(std::move(music));
    }
}

void MusicPlaylist::removeTrack(std::size_t index) {
    if (index < m_tracks.size()) {
        m_tracks.erase(m_tracks.begin() + static_cast<std::ptrdiff_t>(index));
        if (m_currentIndex >= m_tracks.size() && m_currentIndex > 0) {
            m_currentIndex = m_tracks.size() - 1;
        }
    }
}

void MusicPlaylist::clear() {
    m_tracks.clear();
    m_currentIndex = 0;
}

MusicHandle MusicPlaylist::getCurrentTrack() const {
    if (m_currentIndex < m_tracks.size()) {
        return m_tracks[m_currentIndex];
    }
    return nullptr;
}

void MusicPlaylist::next(bool loop) {
    if (m_tracks.empty()) return;

    m_currentIndex++;
    if (m_currentIndex >= m_tracks.size()) {
        m_currentIndex = loop ? 0 : m_tracks.size() - 1;
    }
}

void MusicPlaylist::previous(bool loop) {
    if (m_tracks.empty()) return;

    if (m_currentIndex == 0) {
        m_currentIndex = loop ? m_tracks.size() - 1 : 0;
    } else {
        m_currentIndex--;
    }
}

void MusicPlaylist::shuffle() {
    if (m_tracks.size() <= 1) return;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_tracks.begin(), m_tracks.end(), g);
    m_currentIndex = 0;
}

} // namespace sims3000
