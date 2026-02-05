/**
 * @file Serialization.h
 * @brief Binary serialization interfaces for networking and persistence.
 */

#ifndef SIMS3000_CORE_SERIALIZATION_H
#define SIMS3000_CORE_SERIALIZATION_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

namespace sims3000 {

/**
 * @class WriteBuffer
 * @brief Binary write buffer for serialization.
 */
class WriteBuffer {
public:
    WriteBuffer() { m_data.reserve(1024); }

    void writeU8(std::uint8_t v) { m_data.push_back(v); }
    void writeU16(std::uint16_t v) { write(&v, sizeof(v)); }
    void writeU32(std::uint32_t v) { write(&v, sizeof(v)); }
    void writeU64(std::uint64_t v) { write(&v, sizeof(v)); }
    void writeI8(std::int8_t v) { m_data.push_back(static_cast<std::uint8_t>(v)); }
    void writeI16(std::int16_t v) { write(&v, sizeof(v)); }
    void writeI32(std::int32_t v) { write(&v, sizeof(v)); }
    void writeI64(std::int64_t v) { write(&v, sizeof(v)); }
    void writeF32(float v) { write(&v, sizeof(v)); }
    void writeF64(double v) { write(&v, sizeof(v)); }

    void writeString(const std::string& s) {
        writeU32(static_cast<std::uint32_t>(s.size()));
        write(s.data(), s.size());
    }

    void write(const void* data, std::size_t size) {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        m_data.insert(m_data.end(), bytes, bytes + size);
    }

    const std::uint8_t* data() const { return m_data.data(); }
    std::size_t size() const { return m_data.size(); }
    void clear() { m_data.clear(); }

private:
    std::vector<std::uint8_t> m_data;
};

/**
 * @class ReadBuffer
 * @brief Binary read buffer for deserialization.
 */
class ReadBuffer {
public:
    ReadBuffer(const std::uint8_t* data, std::size_t size)
        : m_data(data), m_size(size), m_pos(0) {}

    std::uint8_t readU8() { return read<std::uint8_t>(); }
    std::uint16_t readU16() { return read<std::uint16_t>(); }
    std::uint32_t readU32() { return read<std::uint32_t>(); }
    std::uint64_t readU64() { return read<std::uint64_t>(); }
    std::int8_t readI8() { return static_cast<std::int8_t>(readU8()); }
    std::int16_t readI16() { return read<std::int16_t>(); }
    std::int32_t readI32() { return read<std::int32_t>(); }
    std::int64_t readI64() { return read<std::int64_t>(); }
    float readF32() { return read<float>(); }
    double readF64() { return read<double>(); }

    std::string readString() {
        std::uint32_t len = readU32();
        if (m_pos + len > m_size) return "";
        std::string s(reinterpret_cast<const char*>(m_data + m_pos), len);
        m_pos += len;
        return s;
    }

    bool readBytes(void* out, std::size_t size) {
        if (m_pos + size > m_size) return false;
        std::memcpy(out, m_data + m_pos, size);
        m_pos += size;
        return true;
    }

    std::size_t remaining() const { return m_size - m_pos; }
    bool hasMore() const { return m_pos < m_size; }
    std::size_t position() const { return m_pos; }

private:
    template<typename T>
    T read() {
        T v{};
        if (m_pos + sizeof(T) <= m_size) {
            std::memcpy(&v, m_data + m_pos, sizeof(T));
            m_pos += sizeof(T);
        }
        return v;
    }

    const std::uint8_t* m_data;
    std::size_t m_size;
    std::size_t m_pos;
};

/**
 * @interface ISerializable
 * @brief Interface for serializable types.
 */
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void serialize(WriteBuffer& buffer) const = 0;
    virtual void deserialize(ReadBuffer& buffer) = 0;
};

/**
 * @enum SyncPolicy
 * @brief Component synchronization policy for networking.
 */
enum class SyncPolicy {
    None,           // Never synced (client-only visuals)
    ServerAuth,     // Server authoritative, full sync
    Predicted,      // Client predicted, server validated
    Interpolated,   // Interpolated on client between server states
    EventDriven     // Only synced on change via events
};

} // namespace sims3000

#endif // SIMS3000_CORE_SERIALIZATION_H
