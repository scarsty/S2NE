#pragma once
#include "original_skill_visual.hpp"
namespace s2ne {
class OriginalSkill88Visual {
    std::vector<OriginalSkill32Visual::Frame> frames_;
    std::array<int,256> amounts_{};
    size_t frame_{};uint64_t elapsed_{};bool presented_{};
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static std::vector<uint8_t> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing original88 resource");return {std::istreambuf_iterator<char>(f),{}};}
public:
    explicit OriginalSkill88Visual(const std::filesystem::path& root){
        amounts_.fill(-1);auto pal=read(root/"SUPERSP.PAL"),bin=read(root/"SUPERSP.BIN"),cel=read(root/"SUPERSP.CEL");size_t t=dword(pal,920)-20;unsigned n=word(bin,t);
        if(n!=115||bin.at(t+2+87)!=78||bin.at(t+2+n+87)!=0||bin.at(t+2+n*2+87)!=233||bin.at(t+2+n*3+87)!=50)throw std::runtime_error("Unsupported88 dispatch");
        size_t base=dword(pal,808)-56,offset=dword(cel,base+2+78*8),length=dword(cel,base+6+78*8);
        if(base>cel.size()||offset>cel.size()-base||length>cel.size()-base-offset)throw std::runtime_error("Invalid88 archive extent");
        std::vector<uint8_t> entry(cel.begin()+base+offset,cel.begin()+base+offset+length);constexpr std::array<unsigned,8> header{35,89,124,175,124,6,0,5};
        for(unsigned i=0;i<8;++i)if(word(entry,i*2)!=header[i])throw std::runtime_error("Unsupported88 SPELLSP layout");
        // 463EA6 maps six source indices to their independent blend percentages.
        for(unsigned i=0;i<6;++i)amounts_[entry.at(16+i)]=entry.at(22+i);
        size_t data=28+35*8,previous=0;if(entry.size()<data)throw std::runtime_error("Truncated88 directory");
        for(unsigned i=0;i<35;++i){size_t off=dword(entry,28+i*8),size=dword(entry,32+i*8);if(off!=previous||off>entry.size()-data||size>entry.size()-data-off)throw std::runtime_error("Invalid88 frame extent");frames_.push_back(OriginalSkill32Visual::decode({entry.begin()+data+off,entry.begin()+data+off+size},175,124));previous=off+size;}
        if(previous!=entry.size()-data)throw std::runtime_error("Unexpected88 trailing bytes");
    }
    const auto& frames()const{return frames_;}
    const auto& current()const{return frames_.at(frame_);}
    int amount(unsigned color)const{return amounts_.at(color);}
    size_t frame()const{return frame_;}bool done()const{return frame_==frames_.size();}
    void presented(){presented_=true;}void tick(unsigned ms){if(done())return;elapsed_+=ms;if(presented_&&elapsed_>=50){++frame_;elapsed_=0;presented_=false;}}
};
}
