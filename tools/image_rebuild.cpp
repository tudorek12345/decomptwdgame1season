// Binary-backed reconstruction experiment. This tool is C++; the generated game
// still contains original machine code. It is NOT a source-rebuilt Telltale engine.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stdexcept>
std::string hash(const std::vector<unsigned char>& b) {
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE h=nullptr;DWORD size=0,written=0;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&written,0);
    std::vector<unsigned char> object(size);unsigned char digest[32];
    if(BCryptCreateHash(alg,&h,object.data(),size,nullptr,0,0)<0 || BCryptHashData(h,const_cast<PUCHAR>(b.data()),static_cast<ULONG>(b.size()),0)<0 || BCryptFinishHash(h,digest,32,0)<0)throw std::runtime_error("SHA256 failed");
    BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(alg,0);
    std::ostringstream s;for(auto v:digest)s<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(v);return s.str();
}
int wmain(int argc,wchar_t** argv) {
 try {
    if(argc!=3)throw std::runtime_error("Usage: twd_image_rebuild analysis.exe new-output.exe");
    std::filesystem::path input=argv[1],output=argv[2];
    if(std::filesystem::exists(output))throw std::runtime_error("Output already exists; use a fresh output path");
    std::ifstream in(input,std::ios::binary);if(!in)throw std::runtime_error("Input unavailable");
    std::vector<unsigned char> b((std::istreambuf_iterator<char>(in)),{});
    if(hash(b)!="39df65b5e03e68703e04da90d516fbc15392706fbc9a4580f2aaf44ef78c33d9")throw std::runtime_error("Unsupported analysis image hash");
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b.data());
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(b.data()+dos->e_lfanew);
    auto* sections=IMAGE_FIRST_SECTION(nt);
    auto offset=[&](DWORD rva)->size_t{
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;i++)if(rva>=sections[i].VirtualAddress&&rva<sections[i].VirtualAddress+sections[i].SizeOfRawData)return sections[i].PointerToRawData+rva-sections[i].VirtualAddress;
        throw std::runtime_error("RVA outside file-backed sections");
    };
    constexpr unsigned char entry[]={0xe8,0x9e,0xea,0,0,0xe9,0x89,0xfe,0xff,0xff};
    if(std::memcmp(b.data()+offset(0x4d9026),entry,sizeof(entry)))throw std::runtime_error("Startup evidence does not match");
    nt->OptionalHeader.AddressOfEntryPoint=0x4d9026;
    nt->OptionalHeader.CheckSum=0;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]={0,0};
    // Same-length replacement isolates Documents and HKCU namespaces without
    // moving data or modifying existing save files. Length literals remain valid.
    const std::string before="Telltale Games",after="TWD-Rebuild-01";
    static_assert(sizeof("Telltale Games")==sizeof("TWD-Rebuild-01"));
    unsigned changes=0;
    for(size_t i=0;i+before.size()<=b.size();i++)if(std::memcmp(b.data()+i,before.data(),before.size())==0){std::memcpy(b.data()+i,after.data(),after.size());changes++;i+=before.size()-1;}
    if(!changes)throw std::runtime_error("Save namespace anchor missing");
    std::filesystem::create_directories(output.parent_path());
    std::ofstream out(output,std::ios::binary);out.write(reinterpret_cast<const char*>(b.data()),b.size());if(!out)throw std::runtime_error("Output write failed");
    std::cout<<"{\"sha256\":\""<<hash(b)<<"\",\"entry_rva\":\"0x4d9026\",\"namespace_replacements\":"<<changes<<",\"contains_original_machine_code\":true,\"source_engine_rebuilt\":false}\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
