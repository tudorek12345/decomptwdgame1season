#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include "geometry.hpp"
#include "original_geometry_bytes.hpp"
using Fn=twd::Vec3* (__cdecl*)(twd::Vec3*,const twd::Vec3*,const twd::Vec3*,const twd::Vec3*);
int main(int argc,char** argv) {
    static_assert(sizeof(void*)==4,"Original x86 routine requires a Win32 build");
    unsigned control=0; _controlfp_s(&control,_PC_53,_MCW_PC);
    void* memory=VirtualAlloc(nullptr,sizeof(original_geometry),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!memory)return 2;
    std::memcpy(memory,original_geometry,sizeof(original_geometry));
    DWORD old=0;if(!VirtualProtect(memory,sizeof(original_geometry),PAGE_EXECUTE_READ,&old))return 3;
    FlushInstructionCache(GetCurrentProcess(),memory,sizeof(original_geometry));
    Fn original=reinterpret_cast<Fn>(memory);
    HMODULE library=nullptr;
    Fn replacement=&twd::closest_point_on_segment;
    if(argc==2) {
        library=LoadLibraryA(argv[1]);if(!library)return 4;
        replacement=reinterpret_cast<Fn>(GetProcAddress(library,"twd_closest_point_on_segment"));if(!replacement)return 5;
    }
    size_t cases=0,failed=0,exact=0;double max_absolute_error=0;
    auto check=[&](twd::Vec3 a,twd::Vec3 b,twd::Vec3 p,int alias){
        twd::Vec3 a1=a,b1=b,p1=p,out1{},a2=a,b2=b,p2=p,out2{};
        twd::Vec3* x=alias==1?&a1:alias==2?&b1:alias==3?&p1:&out1;
        twd::Vec3* y=alias==1?&a2:alias==2?&b2:alias==3?&p2:&out2;
        bool ok=original(x,&a1,&b1,&p1)==x;
        ok=(replacement(y,&a2,&b2,&p2)==y)&&ok;
        const float xv[]={x->x,x->y,x->z},yv[]={y->x,y->y,y->z};
        for(int k=0;k<3;k++){
            if(std::isnan(xv[k])&&std::isnan(yv[k]))continue;
            if(xv[k]==yv[k])continue;
            double err=std::abs(double(xv[k])-yv[k]);max_absolute_error=std::max(max_absolute_error,err);
            // Float intermediates and x87/SSE evaluation differ; tolerance is per-coordinate.
            if(!std::isfinite(err)||err>0.0005+0.000002*std::abs(xv[k]))ok=false;
        }
        if(std::memcmp(x,y,sizeof(*x))==0)exact++;
        if(!ok&&failed++<8)std::cerr<<"Mismatch at case "<<cases<<"\n";
        cases++;
    };
    std::mt19937 rng(0x54574431);std::uniform_real_distribution<float> dist(-1024,1024);
    auto vec=[&](){return twd::Vec3{dist(rng),dist(rng),dist(rng)};};
    for(int i=0;i<25000;i++){auto a=vec(),b=vec(),p=vec();for(int alias=0;alias<4;alias++)check(a,b,p,alias);}
    const float nan=std::numeric_limits<float>::quiet_NaN();
    for(int alias=0;alias<4;alias++){
        check({0,0,0},{1,0,0},{-1,3,2},alias);
        check({0,0,0},{1,0,0},{2,3,2},alias);
        check({0,0,0},{1,0,0},{.5f,3,2},alias);
        check({0,0,0},{1,0,0},{0,0,0},alias);
        check({0,0,0},{1,0,0},{1,0,0},alias);
        check({2,3,4},{2,3,4},{9,8,7},alias);
        check({0,0,0},{1,0,0},{nan,1,2},alias);
        check({-0.f,0,0},{0,-0.f,0},{0,0,-0.f},alias);
    }
    VirtualFree(memory,0,MEM_RELEASE);
    if(library)FreeLibrary(library);
    std::cout<<"{\"cases\":"<<cases<<",\"failed\":"<<failed<<",\"bit_exact\":"<<exact<<",\"max_absolute_error\":"<<max_absolute_error<<"}\n";
    return failed?1:0;
}
