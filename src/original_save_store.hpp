#pragma once
#include "original_save.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace s2ne {
class OriginalSaveStore {
    static void replace(const std::filesystem::path& path,std::span<const uint8_t> bytes){
        auto pending=path;pending+=L".s2ne-tmp";
        HANDLE file=CreateFileW(pending.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create exclusive original-save temporary file");
        bool success=false;
        try{
            size_t offset=0;
            while(offset<bytes.size()){
                DWORD written=0,want=DWORD(std::min<size_t>(bytes.size()-offset,0x40000000));
                if(!WriteFile(file,bytes.data()+offset,want,&written,nullptr)||!written)throw std::runtime_error("Original-save write failed");
                offset+=written;
            }
            if(!FlushFileBuffers(file))throw std::runtime_error("Original-save flush failed");
            CloseHandle(file);file=INVALID_HANDLE_VALUE;
            if(std::filesystem::exists(path)){
                auto backup=path;backup+=L".s2ne-backup";
                success=ReplaceFileW(path.c_str(),pending.c_str(),backup.c_str(),0,nullptr,nullptr)!=0;
            }else success=MoveFileExW(pending.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
            if(!success)throw std::runtime_error("Original-save atomic replacement failed");
        }catch(...){
            if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
            std::error_code ignored;std::filesystem::remove(pending,ignored);throw;
        }
    }
public:
    // 43619F preserves the overwritten original as slot53 before4362B4 writes.
    static void write(const std::filesystem::path& root,unsigned slot,const OriginalSave::State& state,size_t items){
        auto bytes=OriginalSave::encode(state,items);auto path=OriginalSave::slot_path(root,slot);
        std::filesystem::create_directories(path.parent_path());
        auto pending=path;pending+=L".s2ne-tmp";
        if(std::filesystem::exists(pending))throw std::runtime_error("Previous pending save exists; original slots left untouched");
        if(slot!=53&&std::filesystem::exists(path)){
            auto previous=OriginalSave::load(path,items);
            bool described=previous.description.front()!=0;
            if(described){
                std::ifstream input(path,std::ios::binary);std::vector<uint8_t> previous_bytes{std::istreambuf_iterator<char>(input),{}};
                replace(OriginalSave::slot_path(root,53),previous_bytes);
            }
        }
        replace(path,bytes);
    }
};
}
