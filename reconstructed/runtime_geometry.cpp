#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "geometry.hpp"
#include "telemetry.hpp"
// This exported source implementation replaces one original routine in the
// isolated hybrid image. All remaining engine machine code is still original.
extern "C" twd::Vec3* __cdecl twd_closest_point_on_segment(twd::Vec3* out,const twd::Vec3* a,const twd::Vec3* b,const twd::Vec3* p) {
    twd_count(0);
    static volatile LONG calls=0;
    LONG count=InterlockedIncrement(&calls);
    if(count==1) {
        HANDLE f=CreateFileW(L"reconstructed-geometry-calls.log",FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(f!=INVALID_HANDLE_VALUE){const char line[]="Reconstructed C++ geometry called by the hybrid engine.\r\n";DWORD written;WriteFile(f,line,sizeof(line)-1,&written,nullptr);CloseHandle(f);}
    }
    return twd::closest_point_on_segment(out,a,b,p);
}
