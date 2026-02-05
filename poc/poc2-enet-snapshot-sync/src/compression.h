#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace Compression {

// Compress data using LZ4. Returns compressed data with a 4-byte uncompressed size prefix.
std::vector<uint8_t> compress(const uint8_t* data, size_t size);

// Decompress LZ4 data. Expects 4-byte uncompressed size prefix.
std::vector<uint8_t> decompress(const uint8_t* data, size_t size);

} // namespace Compression
