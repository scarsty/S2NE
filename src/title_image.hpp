#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stdexcept>
namespace s2ne {
// 472EF1: DAC byte << 2; 47F5D2: unsigned product / 255.
// Fade-in visits 0,4,...,60 (not 63) when ebx=-4.
inline constexpr uint8_t opening_palette_channel(uint8_t dac,unsigned fade=60) {
    return uint8_t(unsigned(uint8_t(unsigned(dac)<<2))*unsigned(uint8_t(fade<<2))/255);
}
inline std::vector<uint8_t> load_title(const std::filesystem::path& root,unsigned resource=19,bool display_palette=true) {
    auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing title resource");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
    auto pal=read(root/"SUPERSP.PAL");
    auto dat=read(root/"cd"/"windows-disc1"/"SUPERSP"/"SUPERSP.DAT");
    auto word=[](const auto& b,size_t p){return unsigned(b.at(p))|(unsigned(b.at(p+1))<<8);};
    auto dword=[&](const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);};
    if(resource!=19&&resource!=20)throw std::runtime_error("Unsupported title resource");
    const size_t entry=808+8*(5+18+resource-1);
    size_t offset=dword(pal,entry)-(resource+7), size=dword(pal,entry+4)-(resource+7);
    if(offset>dat.size()||size>dat.size()-offset||size<897)throw std::runtime_error("Invalid title entry");
    if(dat.at(offset)!=10 || dat.at(offset+2)!=1 || dat.at(offset+3)!=8 || dat.at(offset+65)!=1 || word(dat,offset+8)!=639 || word(dat,offset+10)!=479)throw std::runtime_error("Unsupported title PCX");
    unsigned stride=word(dat,offset+66);
    if(stride<640||stride>4096)throw std::runtime_error("Invalid PCX stride");
    size_t own_colors=offset+size-768, p=offset+128;
    if(dat.at(own_colors-1)!=12)throw std::runtime_error("Missing PCX palette");
    // 40E8DB uses the global SUPERSP.PAL, unlike 40E0EF's DAT19 PCX palette.
    size_t colors=resource==19?own_colors:0;
    const auto& palette=resource==19?dat:pal;
    if(pal.size()<768)throw std::runtime_error("Truncated global palette");
    std::vector<uint8_t> image(640*480*4);
    size_t pixel=0, total=size_t(stride)*480;
    while(pixel<total){
        if(p>=own_colors-1)throw std::runtime_error("Truncated PCX");
        unsigned value=dat.at(p++), count=1;
        if((value&192)==192){count=value&63;if(p>=own_colors-1)throw std::runtime_error("Truncated PCX run");value=dat.at(p++);}
        if(!count||count>total-pixel)throw std::runtime_error("Invalid PCX run");
        while(count--){size_t x=pixel%stride,y=pixel/stride;++pixel;if(x>=640)continue;size_t dest=(y*640+x)*4;
            for(unsigned c=0;c<3;++c){
                const auto dac=resource==19?uint8_t(palette.at(colors+value*3+c)>>2):uint8_t(value==0?0:palette.at(value*3+c));
                image[dest+c]=display_palette?opening_palette_channel(dac):uint8_t(unsigned(dac)*255/63);
            }
            image[dest+3]=255;}
    }
    return image;
}
}
