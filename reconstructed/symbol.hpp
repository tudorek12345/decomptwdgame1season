#pragma once
#include <cstdint>
#include <cstddef>

namespace twd {
// Binary layout recovered from stores/loads at RVAs 0x57ea0..0x57fb4.
struct Symbol { std::uint32_t low,high; };
static_assert(sizeof(Symbol)==8);
struct String32 {
    // Inline char[16] when capacity<16; otherwise first four bytes hold a pointer.
    unsigned char storage[16];
    std::uint32_t length,capacity;
};
static_assert(sizeof(String32)==24 && offsetof(String32,capacity)==20);
struct SymbolCompareContext32 { std::uint8_t equal,padding[3]; std::uint32_t other; };
Symbol* symbol_zero(Symbol* self) noexcept;
Symbol* symbol_from_text(Symbol* self,const char* text) noexcept;
Symbol* symbol_copy(Symbol* self,const Symbol* other) noexcept;
void symbol_from_value(Symbol* self,std::uint64_t value) noexcept;
void symbol_append(Symbol* self,const char* text) noexcept;
std::uint64_t symbol_identity(std::uint64_t value) noexcept;
Symbol* symbol_from_string32(Symbol* self,const String32* text) noexcept;
void symbol_assign_string32(Symbol* self,const String32* text) noexcept;
int symbol_compare32(const Symbol* self,SymbolCompareContext32* context) noexcept;
}
