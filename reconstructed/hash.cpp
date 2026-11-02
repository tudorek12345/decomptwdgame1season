#include "hash.hpp"
#include <cstring>

namespace twd {
namespace {
constexpr auto make_crc32_table() {
    std::array<std::uint32_t,256> result{};
    for (unsigned i=0;i<256;++i) {
        std::uint32_t c=i;
        for (unsigned bit=0;bit<8;++bit) c=(c>>1)^((c&1)?0xedb88320U:0);
        result[i]=c;
    }
    return result;
}
constexpr auto make_crc64_table() {
    std::array<std::uint64_t,256> result{};
    for (unsigned i=0;i<256;++i) {
        std::uint64_t c=std::uint64_t(i)<<56;
        for (unsigned bit=0;bit<8;++bit) c=(c<<1)^((c>>63)?0x42f0e1eba9ea3693ULL:0);
        result[i]=c;
    }
    return result;
}
constexpr auto table32=make_crc32_table();
constexpr auto table64=make_crc64_table();
}
const std::array<std::uint32_t,256>& crc32_table() noexcept { return table32; }
const std::array<std::uint64_t,256>& crc64_table() noexcept { return table64; }
std::uint32_t crc32(std::uint32_t seed,const void* bytes,std::uint32_t length) noexcept {
    if (!bytes) return 0; // Original returns zero for null, even for nonzero seed/length.
    auto p=static_cast<const unsigned char*>(bytes);
    auto c=~seed;
    while (length--) c=(c>>8)^table32[(c^*p++)&255];
    return ~c;
}
std::uint32_t hash_words(std::uint32_t seed,const void* words,std::uint32_t count) noexcept {
    // Count is in little-endian 32-bit words, not bytes. Unaligned reads are valid.
    auto p=static_cast<const unsigned char*>(words);
    while (count--) {
        std::uint32_t word=std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
            (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
        seed=(seed<<8)^table32[(word+seed)>>24]^word;
        p+=4;
    }
    return seed;
}
std::uint64_t crc64_string(std::uint64_t seed,const char* text) noexcept {
    auto p=reinterpret_cast<const unsigned char*>(text);
    for (auto c=*p;c;c=*++p) seed=(seed<<8)^table64[(seed>>56)^c];
    return seed;
}
std::uint64_t crc64_string_ascii_fold(std::uint64_t seed,const char* text) noexcept {
    auto p=reinterpret_cast<const unsigned char*>(text);
    for (auto c=*p;c;c=*++p) {
        if (c>='A' && c<='Z') c=static_cast<unsigned char>(c+('a'-'A'));
        seed=(seed<<8)^table64[(seed>>56)^c];
    }
    return seed;
}
}
