#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne {
enum class OriginalDisc : unsigned { one=0, two=1 };

constexpr OriginalDisc original_disc_for_campaign(int route,int progress,int stage_global) {
    // 416333: 4FB484, 4FB480 and 4F7CE0 are distinct globals.
    return (route==1&&progress>=7)||(route==0&&(stage_global==60||stage_global==61))
        ?OriginalDisc::two:OriginalDisc::one;
}

class OriginalDiscResources {
    std::filesystem::path root_;
    std::vector<uint8_t> pal_;
    struct Entry { uint32_t offset,size; };
    static uint32_t dword(std::span<const uint8_t> bytes,size_t at) {
        if(at>bytes.size()||bytes.size()-at<4)throw std::runtime_error("Truncated original disc index");
        return uint32_t(bytes[at])|(uint32_t(bytes[at+1])<<8)|(uint32_t(bytes[at+2])<<16)|(uint32_t(bytes[at+3])<<24);
    }
    Entry entry(OriginalDisc disc,unsigned id)const {
        constexpr std::array<unsigned,5> biases{56,11,8,10,20};
        size_t table=808;
        unsigned selected=disc==OriginalDisc::one?2:4,count=0;
        for(unsigned i=0;i<=selected;++i){
            auto encoded=dword(pal_,768+i*4);
            if(encoded<biases[i])throw std::runtime_error("Invalid original disc index count");
            count=encoded-biases[i];
            if(i!=selected)table+=size_t(count)*8;
        }
        if(!id||id>count)throw std::runtime_error("Invalid original disc resource ID");
        size_t index=id-1,at=table+index*8;
        uint32_t bias=biases[selected]+uint32_t(index),offset=dword(pal_,at),size=dword(pal_,at+4);
        if(offset<bias||size<bias)throw std::runtime_error("Invalid original disc entry");
        return {offset-bias,size-bias};
    }
    bool compatible(const std::filesystem::path& path,OriginalDisc disc)const {
        std::ifstream file(path,std::ios::binary);
        if(!file)return false;
        std::array<uint8_t,28> header{};
        if(!file.read(reinterpret_cast<char*>(header.data()),header.size()))return false;
        constexpr std::array<uint8_t,8> signature{'S','u','p','e','r','S','p','_'};
        if(!std::equal(signature.begin(),signature.end(),header.begin()+8))return false;
        file.seekg(0,std::ios::end);auto length=file.tellg();
        if(length<28||uint64_t(length)!=uint64_t(dword(header,0))+28)return false;
        unsigned encoded=dword(pal_,disc==OriginalDisc::one?776:784),bias=disc==OriginalDisc::one?8:20;
        if(encoded<bias)return false;
        uint64_t end=0;
        for(unsigned id=1;id<=encoded-bias;++id){auto e=entry(disc,id);
            if(e.offset>uint64_t(length)||e.size>uint64_t(length)-e.offset)return false;
            end=std::max(end,uint64_t(e.offset)+e.size);
        }
        return end==uint64_t(length);
    }
public:
    explicit OriginalDiscResources(const std::filesystem::path& root):root_(root) {
        std::ifstream file(root_/"SUPERSP.PAL",std::ios::binary);
        if(!file)throw std::runtime_error("Missing original disc index SUPERSP.PAL");
        pal_={std::istreambuf_iterator<char>(file),{}};
    }
    std::filesystem::path dat_path(OriginalDisc disc)const {
        const char* windows=disc==OriginalDisc::one?"windows-disc1":"windows-disc2";
        const char* extracted=disc==OriginalDisc::one?"disc1":"disc2";
        for(const char* folder:{windows,extracted}){
            auto path=root_/"cd"/folder/"SUPERSP"/"SUPERSP.DAT";
            if(compatible(path,disc))return path;
        }
        throw std::runtime_error(disc==OriginalDisc::one?"Missing compatible locally extracted CD1 SUPERSP.DAT":"Missing compatible locally extracted CD2 SUPERSP.DAT");
    }
    std::vector<uint8_t> dat_entry(OriginalDisc disc,unsigned id)const {
        auto e=entry(disc,id);
        std::ifstream file(dat_path(disc),std::ios::binary);
        file.seekg(e.offset);
        std::vector<uint8_t> bytes(e.size);
        if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))throw std::runtime_error("Truncated original disc resource");
        return bytes;
    }
};
}
