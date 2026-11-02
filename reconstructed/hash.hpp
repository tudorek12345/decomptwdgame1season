#pragma once
#include <cstdint>
#include <array>

namespace twd {
// Recovered contracts, not original symbol names. See reports/hash-contract.json.
std::uint32_t crc32(std::uint32_t seed, const void* bytes, std::uint32_t length) noexcept;
std::uint32_t hash_words(std::uint32_t seed, const void* words, std::uint32_t count) noexcept;
std::uint64_t crc64_string(std::uint64_t seed, const char* text) noexcept;
std::uint64_t crc64_string_ascii_fold(std::uint64_t seed, const char* text) noexcept;
const std::array<std::uint32_t,256>& crc32_table() noexcept;
const std::array<std::uint64_t,256>& crc64_table() noexcept;
}
