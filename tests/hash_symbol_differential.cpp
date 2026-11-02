#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include "hash.hpp"
#include "symbol.hpp"
#include "original_hash_symbols.hpp"

using S=twd::Symbol;
using String=twd::String32;
using Context=twd::SymbolCompareContext32;
using Hash32=std::uint32_t(__cdecl*)(std::uint32_t,const void*,std::uint32_t);
using Hash64=std::uint64_t(__cdecl*)(std::uint64_t,const char*);
using Zero=S*(__fastcall*)(S*,void*);
using Text=S*(__fastcall*)(S*,void*,const char*);
using Copy=S*(__fastcall*)(S*,void*,const S*);
using Value=void(__fastcall*)(S*,void*,std::uint64_t);
using Append=void(__fastcall*)(S*,void*,const char*);
using Identity=std::uint64_t(__cdecl*)(std::uint64_t);
using Compare=int(__cdecl*)(const S*,void*,void*,Context*);
using FromString=S*(__fastcall*)(S*,void*,const String*);
using AssignString=void(__fastcall*)(S*,void*,const String*);

S* __fastcall static_zero(S* s,void*) { return twd::symbol_zero(s); }
S* __fastcall static_text(S* s,void*,const char* t) { return twd::symbol_from_text(s,t); }
S* __fastcall static_copy(S* s,void*,const S* t) { return twd::symbol_copy(s,t); }
void __fastcall static_value(S* s,void*,std::uint64_t v) { twd::symbol_from_value(s,v); }
void __fastcall static_append(S* s,void*,const char* t) { twd::symbol_append(s,t); }
int __cdecl static_compare(const S* s,void*,void*,Context* c) { return twd::symbol_compare32(s,c); }
S* __fastcall static_string(S* s,void*,const String* t) { return twd::symbol_from_string32(s,t); }
void __fastcall static_assign(S* s,void*,const String* t) { twd::symbol_assign_string32(s,t); }

struct Functions {
    Hash32 crc32,words;Hash64 string,fold;Zero zero;Text text;Copy copy;
    Value value;Append append;Identity identity;Compare compare;FromString from;AssignString assign;
};
const char* names[]={"crc32","hash_words","crc64_string","crc64_string_ascii_fold","symbol_zero",
    "symbol_from_text","symbol_copy","symbol_from_value","symbol_append","symbol_identity",
    "symbol_compare32","symbol_from_string32","symbol_assign_string32","generated_tables","concurrent_hashes"};
std::uint64_t counts[15]={},failures[15]={};
void check(unsigned slot,bool ok) { ++counts[slot];if (!ok && failures[slot]++<4) std::cerr<<"Mismatch: "<<names[slot]<<" case "<<counts[slot]<<"\n"; }
bool same(const S& a,const S& b) { return std::memcmp(&a,&b,sizeof(a))==0; }

int main(int argc,char** argv) try {
    static_assert(sizeof(void*)==4,"Only execute the x86 oracle in a Win32 harness");
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,oracle_image_size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if (!memory) throw std::runtime_error("VirtualAlloc");
    for (const auto& range:oracle_ranges) std::memcpy(memory+range.rva,range.bytes,range.size);
    for (auto offset:oracle_relocations) {
        std::uint32_t address;std::memcpy(&address,memory+offset,4);
        address+=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(memory))-0x400000;
        std::memcpy(memory+offset,&address,4);
    }
    DWORD old=0;
    if (!VirtualProtect(memory,oracle_image_size,PAGE_EXECUTE_READ,&old)) throw std::runtime_error("VirtualProtect");
    FlushInstructionCache(GetCurrentProcess(),memory,oracle_image_size);
    Functions original{reinterpret_cast<Hash32>(memory+0x1cc380),reinterpret_cast<Hash32>(memory+0x1cc430),
        reinterpret_cast<Hash64>(memory+0x1eecf0),reinterpret_cast<Hash64>(memory+0x1eed40),
        reinterpret_cast<Zero>(memory+0x57ea0),reinterpret_cast<Text>(memory+0x57eb0),
        reinterpret_cast<Copy>(memory+0x57ed0),reinterpret_cast<Value>(memory+0x57ef0),
        reinterpret_cast<Append>(memory+0x57f00),reinterpret_cast<Identity>(memory+0x57f20),
        reinterpret_cast<Compare>(memory+0x57f30),reinterpret_cast<FromString>(memory+0x57f60),
        reinterpret_cast<AssignString>(memory+0x57f90)};
    Functions replacement{twd::crc32,twd::hash_words,twd::crc64_string,twd::crc64_string_ascii_fold,
        static_zero,static_text,static_copy,static_value,static_append,twd::symbol_identity,static_compare,static_string,static_assign};
    HMODULE library=nullptr;
    if (argc==2) {
        library=LoadLibraryA(argv[1]);if (!library) throw std::runtime_error("LoadLibrary");
#define RESOLVE(field,type,name) replacement.field=reinterpret_cast<type>(GetProcAddress(library,"twd_" name));if(!replacement.field)throw std::runtime_error("Missing export " name)
        RESOLVE(crc32,Hash32,"crc32");RESOLVE(words,Hash32,"hash_words");
        RESOLVE(string,Hash64,"crc64_string");RESOLVE(fold,Hash64,"crc64_string_ascii_fold");
        RESOLVE(zero,Zero,"symbol_zero");RESOLVE(text,Text,"symbol_from_text");
        RESOLVE(copy,Copy,"symbol_copy");RESOLVE(value,Value,"symbol_from_value");
        RESOLVE(append,Append,"symbol_append");RESOLVE(identity,Identity,"symbol_identity");
        RESOLVE(compare,Compare,"symbol_compare32");RESOLVE(from,FromString,"symbol_from_string32");
        RESOLVE(assign,AssignString,"symbol_assign_string32");
#undef RESOLVE
    }
    check(13,std::memcmp(twd::crc32_table().data(),memory+0x58fce0,1024)==0);
    check(13,std::memcmp(twd::crc64_table().data(),memory+0x5923f8,2048)==0);
    std::mt19937 rng(0x54574432);
    auto hashes=[&](const unsigned char* p,std::uint32_t length,std::uint64_t seed){
        check(0,original.crc32(static_cast<std::uint32_t>(seed),p,length)==replacement.crc32(static_cast<std::uint32_t>(seed),p,length));
        check(1,original.words(static_cast<std::uint32_t>(seed),p,length/4)==replacement.words(static_cast<std::uint32_t>(seed),p,length/4));
        auto text=reinterpret_cast<const char*>(p);
        check(2,original.string(seed,text)==replacement.string(seed,text));
        check(3,original.fold(seed,text)==replacement.fold(seed,text));
    };
    // Exhaust every byte and every top-byte seed, including high-byte strings.
    for (unsigned byte=0;byte<256;++byte) for(unsigned high=0;high<256;++high) {
        unsigned char text[]={static_cast<unsigned char>(byte),0};
        auto seed=(std::uint64_t(high)<<56)|0x123456789abcdeULL;
        hashes(text,1,seed);
    }
    for (unsigned i=0;i<50000;++i) {
        unsigned length=i<1024?i:rng()%2048;
        // Offset cycles across alignment classes. Byte buffer includes embedded NULs.
        std::vector<unsigned char> bytes(length+8,0);auto p=bytes.data()+(i&3);
        for(unsigned k=0;k<length;++k)p[k]=static_cast<unsigned char>(rng());p[length]=0;
        auto seed=(std::uint64_t(rng())<<32)|rng();hashes(p,length,seed);
        auto text=reinterpret_cast<const char*>(p);
        S a{rng(),rng()},b=a;check(4,original.zero(&a,nullptr)==&a && replacement.zero(&b,nullptr)==&b && same(a,b));
        check(5,original.text(&a,nullptr,text)==&a && replacement.text(&b,nullptr,text)==&b && same(a,b));
        S source{rng(),rng()};
        check(6,original.copy(&a,nullptr,&source)==&a && replacement.copy(&b,nullptr,&source)==&b && same(a,b));
        check(6,original.copy(&a,nullptr,&a)==&a && replacement.copy(&b,nullptr,&b)==&b && same(a,b));
        original.value(&a,nullptr,seed);replacement.value(&b,nullptr,seed);check(7,same(a,b));
        original.append(&a,nullptr,text);replacement.append(&b,nullptr,text);check(8,same(a,b));
        check(9,original.identity(seed)==replacement.identity(seed));
        S other=(i&1)?a:source;
        Context c1{0xff,{0x23,0x45,0x67},static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&other))},c2=c1;
        check(10,original.compare(&a,nullptr,nullptr,&c1)==replacement.compare(&b,nullptr,nullptr,&c2) && std::memcmp(&c1,&c2,sizeof(c1))==0);
        String str{};str.capacity=15;str.length=length<15?length:15;
        std::memcpy(str.storage,p,str.length);str.storage[str.length]=0;
        check(11,original.from(&a,nullptr,&str)==&a && replacement.from(&b,nullptr,&str)==&b && same(a,b));
        original.assign(&a,nullptr,&str);replacement.assign(&b,nullptr,&str);check(12,same(a,b));
        str.capacity=length<16?16:length;str.length=length;
        auto address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));std::memcpy(str.storage,&address,4);
        check(11,original.from(&a,nullptr,&str)==&a && replacement.from(&b,nullptr,&str)==&b && same(a,b));
        original.assign(&a,nullptr,&str);replacement.assign(&b,nullptr,&str);check(12,same(a,b));
    }
    for(auto seed:{0U,1U,0xffffffffU,0x12345678U})for(auto length:{0U,1U,0xffffffffU})
        check(0,original.crc32(seed,nullptr,length)==replacement.crc32(seed,nullptr,length));
    check(1,original.words(0x87654321,nullptr,0)==replacement.words(0x87654321,nullptr,0));
    // Guard page ensures each implementation stops at the observed length/terminator.
    SYSTEM_INFO info{};GetSystemInfo(&info);auto page=info.dwPageSize;
    auto guarded=static_cast<unsigned char*>(VirtualAlloc(nullptr,page*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!guarded || !VirtualProtect(guarded+page,page,PAGE_NOACCESS,&old))throw std::runtime_error("Guard allocation");
    for (unsigned length=0;length<1024;++length) {
        auto p=guarded+page-length-1;for(unsigned j=0;j<length;++j)p[j]=static_cast<unsigned char>(1+j%255);p[length]=0;
        hashes(p,length,0xfedcba9876543210ULL);
    }
    // Tables and wrappers must remain reentrant when invoked by game worker threads.
    std::atomic<unsigned> concurrent_failed{0};std::vector<std::thread> workers;
    for(unsigned thread=0;thread<8;++thread)workers.emplace_back([&,thread]{
        const char* text="adv_streetOutskirts.scene";
        for(unsigned i=0;i<100000;++i){auto seed=(std::uint64_t(thread)<<32)|i;
            if(original.fold(seed,text)!=replacement.fold(seed,text))++concurrent_failed;}
    });
    for(auto& worker:workers)worker.join();counts[14]=800000;failures[14]=concurrent_failed;
    std::uint64_t total=0,failed=0;
    std::cout<<"{\"mode\":\""<<(library?"dll":"static")<<"\",\"functions\":[";
    for(unsigned i=0;i<15;++i){if(i)std::cout<<",";total+=counts[i];failed+=failures[i];
        std::cout<<"{\"name\":\""<<names[i]<<"\",\"cases\":"<<counts[i]<<",\"failed\":"<<failures[i]<<"}";}
    std::cout<<"],\"cases\":"<<total<<",\"failed\":"<<failed<<"}\n";
    VirtualFree(guarded,0,MEM_RELEASE);VirtualFree(memory,0,MEM_RELEASE);if(library)FreeLibrary(library);
    return failed?1:0;
} catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 2;}
