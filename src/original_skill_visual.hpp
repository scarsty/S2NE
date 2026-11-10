#pragma once
#include "original_palette.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace s2ne {
// Bounded 463D6D/463EA6 reader: spell32 selects CEL1 SPELLSP entry33.
class OriginalSkill32Visual {
public:
    struct Frame { unsigned width{},height{};std::vector<uint8_t> pixels; };
private:
    std::vector<Frame> frames_;
    unsigned duration_{};
    size_t frame_{};
    uint64_t elapsed_{};
    bool presented_{};
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static std::vector<uint8_t> read(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing original skill visual resource");return {std::istreambuf_iterator<char>(f),{}};}
public:
    static Frame decode(const std::vector<uint8_t>& bytes,unsigned expected_width=63,unsigned expected_height=272){
        if(bytes.size()<4)throw std::runtime_error("Truncated original skill frame");
        int w=int(int16_t(word(bytes,0)))+1,h=int(int16_t(word(bytes,2)))+1;
        if(w!=int(expected_width)||h!=int(expected_height))throw std::runtime_error("Unsupported original spell33 frame extent");
        Frame result{unsigned(w),unsigned(h),{}};size_t size=size_t(w)*h;result.pixels.reserve(size);
        for(size_t p=4;p<bytes.size();){unsigned code=bytes.at(p++),n=code?code&127:1;
            if(n>size-result.pixels.size())throw std::runtime_error("Original skill run overflow");
            if(!code){result.pixels.push_back(0);continue;}
            unsigned source=code&128?n:1;if(source>bytes.size()-p)throw std::runtime_error("Truncated original skill run");
            if(code&128){while(n--)result.pixels.push_back(bytes.at(p++));}
            else{auto color=bytes.at(p++);result.pixels.insert(result.pixels.end(),n,color);}
        }
        if(result.pixels.size()!=size)throw std::runtime_error("Incomplete original skill frame");
        return result;
    }
    static std::vector<Frame> parse(const std::vector<uint8_t>& entry){
        // No extrapolation to palette-bearing, ordered/repeating or other anchor modes.
        const std::array<unsigned,8> header{23,0,0,63,272,0,0,5};
        if(entry.size()<16)throw std::runtime_error("Truncated original SPELLSP header");
        for(unsigned i=0;i<header.size();++i)if(word(entry,i*2)!=header[i])throw std::runtime_error("Unsupported original SPELLSP layout");
        const size_t data=16+23*8;if(entry.size()<data)throw std::runtime_error("Truncated original SPELLSP directory");
        std::vector<Frame> frames;size_t previous=0;
        for(unsigned i=0;i<23;++i){size_t offset=dword(entry,16+i*8),length=dword(entry,20+i*8);
            if(offset!=previous||offset>entry.size()-data||length>entry.size()-data-offset)throw std::runtime_error("Invalid original SPELLSP frame directory");
            frames.push_back(decode({entry.begin()+data+offset,entry.begin()+data+offset+length}));previous=offset+length;
        }
        if(previous!=entry.size()-data)throw std::runtime_error("Unexpected original SPELLSP trailing data");
        return frames;
    }
    explicit OriginalSkill32Visual(const std::filesystem::path& root){
        auto pal=read(root/"SUPERSP.PAL"),bin=read(root/"SUPERSP.BIN"),cel=read(root/"SUPERSP.CEL");
        size_t table=dword(pal,920)-20;unsigned count=word(bin,table);
        if(count!=115||bin.at(table+2+31)!=32||bin.at(table+2+count+31)!=0||bin.at(table+2+count*2+31)!=140||bin.at(table+2+count*3+31)!=45)throw std::runtime_error("Unsupported original spell32 BIN10 dispatch");
        size_t base=dword(pal,808)-56;
        if(word(cel,base)<33)throw std::runtime_error("Missing original SPELLSP entry33");
        size_t offset=dword(cel,base+2+32*8),length=dword(cel,base+6+32*8);
        if(base>cel.size()||offset>cel.size()-base||length>cel.size()-base-offset)throw std::runtime_error("Invalid original SPELLSP archive extent");
        frames_=parse({cel.begin()+base+offset,cel.begin()+base+offset+length});duration_=5*10;
    }
    const std::vector<Frame>& frames()const{return frames_;}
    const Frame& current()const{return frames_.at(frame_);}
    size_t frame()const{return frame_;}
    bool done()const{return frame_==frames_.size();}
    unsigned duration_ms()const{return duration_;}
    void presented(){presented_=true;}
    void tick(unsigned milliseconds){if(done())return;elapsed_+=milliseconds;if(presented_&&elapsed_>=duration_){++frame_;elapsed_=0;presented_=false;}}
    static void tint(std::vector<uint8_t>& indexed,const OriginalPalette& palette){
        std::array<uint8_t,256> colors{};for(unsigned i=1;i<256;++i)colors[i]=palette.blend_index(i,140,45);
        for(auto& c:indexed)if(c)c=colors[c];
    }
};
}
