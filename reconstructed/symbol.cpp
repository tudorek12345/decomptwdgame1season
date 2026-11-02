#include "symbol.hpp"
#include "hash.hpp"
#include <cstring>

namespace twd {
namespace {
std::uint64_t value(const Symbol* s) noexcept { return s->low|(std::uint64_t(s->high)<<32); }
const char* characters(const String32* s) noexcept {
    if (s->capacity<16) return reinterpret_cast<const char*>(s->storage);
    std::uint32_t address;
    std::memcpy(&address,s->storage,sizeof(address));
    return reinterpret_cast<const char*>(std::uintptr_t(address));
}
}
Symbol* symbol_zero(Symbol* self) noexcept { self->low=0;self->high=0;return self; }
Symbol* symbol_from_text(Symbol* self,const char* text) noexcept {
    symbol_from_value(self,crc64_string_ascii_fold(0,text));return self;
}
Symbol* symbol_copy(Symbol* self,const Symbol* other) noexcept {
    // Preserve two ordered 32-bit accesses, including exact self-aliasing.
    self->low=other->low;self->high=other->high;return self;
}
void symbol_from_value(Symbol* self,std::uint64_t v) noexcept {
    self->low=static_cast<std::uint32_t>(v);self->high=static_cast<std::uint32_t>(v>>32);
}
void symbol_append(Symbol* self,const char* text) noexcept {
    symbol_from_value(self,crc64_string_ascii_fold(value(self),text));
}
std::uint64_t symbol_identity(std::uint64_t v) noexcept { return v; }
Symbol* symbol_from_string32(Symbol* self,const String32* text) noexcept {
    return symbol_from_text(self,characters(text));
}
void symbol_assign_string32(Symbol* self,const String32* text) noexcept {
    symbol_from_text(self,characters(text));
}
int symbol_compare32(const Symbol* self,SymbolCompareContext32* context) noexcept {
    auto other=reinterpret_cast<const Symbol*>(std::uintptr_t(context->other));
    context->equal=static_cast<std::uint8_t>(self->low==other->low && self->high==other->high);
    return 1;
}
}
