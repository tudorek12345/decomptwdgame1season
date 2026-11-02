// Public contract regressions. No captured machine code or game resources.
// The stronger original/replacement differential harnesses require local inputs.
#include "geometry.hpp"
#include "hash.hpp"
#include "symbol.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>

int main() {
    unsigned checks=0,failures=0;
    auto check=[&](bool value,const char* name) {
        ++checks;
        if(!value){++failures;std::cerr<<"Failed: "<<name<<'\n';}
    };
    check(twd::crc32(0,"123456789",9)==0xcbf43926U,"CRC32 check vector");
    check(twd::crc64_string(0,"123456789")==0x6c40df5f0b497347ULL,"CRC64 check vector");
    check(twd::crc32(0x12345678U,nullptr,123)==0,"CRC32 null convention");
    check(twd::crc32(0x12345678U,"",0)==0x12345678U,"CRC32 empty seed");
    check(twd::hash_words(0x12345678U,nullptr,0)==0x12345678U,"Empty word buffer");
    const char* name="adv_streetoutskirts.scene";
    auto id=twd::crc64_string(0,name);
    check(twd::crc64_string_ascii_fold(0,"ADV_STREETOUTSKIRTS.SCENE")==id,"ASCII fold");
    check(twd::crc64_string_ascii_fold(twd::crc64_string_ascii_fold(0,"ADV_"),"StreetOutskirts.scene")==id,"Streaming seed");
    const char high[]={char(0xc0),0};const char different[]={char(0xe0),0};
    check(twd::crc64_string_ascii_fold(0,high)==twd::crc64_string(0,high),"High byte remains unchanged");
    check(twd::crc64_string_ascii_fold(0,high)!=twd::crc64_string_ascii_fold(0,different),"No Unicode folding");
    twd::Symbol s{};twd::symbol_from_text(&s,"ADV_STREETOUTSKIRTS.SCENE");
    check((std::uint64_t(s.high)<<32|s.low)==id,"Symbol text layout");
    twd::symbol_from_text(&s,"ADV_");twd::symbol_append(&s,"streetOutskirts.scene");
    check((std::uint64_t(s.high)<<32|s.low)==id,"Symbol append");
    check(twd::symbol_copy(&s,&s)==&s && (std::uint64_t(s.high)<<32|s.low)==id,"Symbol self-copy");
    twd::String32 inline_string{{'A','B','C',0},3,15};twd::symbol_from_string32(&s,&inline_string);
    check((std::uint64_t(s.high)<<32|s.low)==twd::crc64_string(0,"abc"),"String32 inline layout");
    twd::symbol_zero(&s);check(s.low==0 && s.high==0,"Symbol zero");
    twd::Vec3 a{0,0,0},b{2,0,0},p{1,3,4},out{};
    check(twd::closest_point_on_segment(&out,&a,&b,&p)==&out && out.x==1 && out.y==0 && out.z==0,"Segment projection");
    p={-2,1,1};twd::closest_point_on_segment(&out,&a,&b,&p);check(out.x==0 && out.y==0 && out.z==0,"Segment start clamp");
    p={5,1,1};twd::closest_point_on_segment(&p,&a,&b,&p);check(p.x==2 && p.y==0 && p.z==0,"Segment end clamp with alias");
    twd::closest_point_on_segment(&out,&a,&a,&p);check(std::isnan(out.x)&&std::isnan(out.y)&&std::isnan(out.z),"Degenerate segment NaNs");
    std::cout<<"{\"checks\":"<<checks<<",\"failed\":"<<failures<<"}\n";
    return failures?1:0;
}
