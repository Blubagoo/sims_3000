#include "compression.h"
#include <lz4.h>
#include <cstring>
#include <stdexcept>

namespace Compression {

std::vector<uint8_t> compress(const uint8_t* data, size_t size) {
    if (size == 0) return {};

    int max_compressed = LZ4_compressBound(static_cast<int>(size));
    // 4 bytes for uncompressed size prefix + compressed data
    std::vector<uint8_t> result(4 + max_compressed);

    // Write uncompressed size as little-endian u32
    uint32_t orig_size = static_cast<uint32_t>(size);
    std::memcpy(result.data(), &orig_size, 4);

    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(result.data() + 4),
        static_cast<int>(size),
        max_compressed
    );

    if (compressed_size <= 0) {
        throw std::runtime_error("LZ4 compression failed");
    }

    result.resize(4 + compressed_size);
    return result;
}

std::vector<uint8_t> decompress(const uint8_t* data, size_t size) {
    if (size < 4) {
        throw std::runtime_error("Compressed data too small");
    }

    uint32_t orig_size;
    std::memcpy(&orig_size, data, 4);

    if (orig_size > 64 * 1024 * 1024) { // 64MB safety limit
        throw std::runtime_error("Decompressed size exceeds safety limit");
    }

    std::vector<uint8_t> result(orig_size);

    int decompressed = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data + 4),
        reinterpret_cast<char*>(result.data()),
        static_cast<int>(size - 4),
        static_cast<int>(orig_size)
    );

    if (decompressed < 0 || static_cast<uint32_t>(decompressed) != orig_size) {
        throw std::runtime_error("LZ4 decompression failed");
    }

    return result;
}

} // namespace Compression
