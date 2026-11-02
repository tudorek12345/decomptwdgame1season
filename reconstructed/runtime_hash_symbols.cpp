#include "telemetry.hpp"
#include "hash.hpp"
#include "symbol.hpp"
extern "C" {
volatile LONG twd_reconstruction_counters[14]={};
std::uint32_t __cdecl twd_crc32(std::uint32_t seed,const void* data,std::uint32_t length) {
    twd_count(1);return twd::crc32(seed,data,length);
}
std::uint32_t __cdecl twd_hash_words(std::uint32_t seed,const void* data,std::uint32_t count) {
    twd_count(2);return twd::hash_words(seed,data,count);
}
std::uint64_t __cdecl twd_crc64_string(std::uint64_t seed,const char* text) {
    twd_count(3);return twd::crc64_string(seed,text);
}
std::uint64_t __cdecl twd_crc64_string_ascii_fold(std::uint64_t seed,const char* text) {
    twd_count(4);return twd::crc64_string_ascii_fold(seed,text);
}
twd::Symbol* __fastcall twd_symbol_zero(twd::Symbol* self,void*) {
    twd_count(5);return twd::symbol_zero(self);
}
twd::Symbol* __fastcall twd_symbol_from_text(twd::Symbol* self,void*,const char* text) {
    twd_count(6);return twd::symbol_from_text(self,text);
}
twd::Symbol* __fastcall twd_symbol_copy(twd::Symbol* self,void*,const twd::Symbol* other) {
    twd_count(7);return twd::symbol_copy(self,other);
}
void __fastcall twd_symbol_from_value(twd::Symbol* self,void*,std::uint64_t value) {
    twd_count(8);twd::symbol_from_value(self,value);
}
void __fastcall twd_symbol_append(twd::Symbol* self,void*,const char* text) {
    twd_count(9);twd::symbol_append(self,text);
}
std::uint64_t __cdecl twd_symbol_identity(std::uint64_t value) {
    twd_count(10);return twd::symbol_identity(value);
}
int __cdecl twd_symbol_compare32(const twd::Symbol* self,void*,void*,twd::SymbolCompareContext32* context) {
    twd_count(11);return twd::symbol_compare32(self,context);
}
twd::Symbol* __fastcall twd_symbol_from_string32(twd::Symbol* self,void*,const twd::String32* text) {
    twd_count(12);return twd::symbol_from_string32(self,text);
}
void __fastcall twd_symbol_assign_string32(twd::Symbol* self,void*,const twd::String32* text) {
    twd_count(13);twd::symbol_assign_string32(self,text);
}
}
