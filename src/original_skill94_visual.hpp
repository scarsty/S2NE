#pragma once
#include "original_skill_visual.hpp"
namespace s2ne {
// Sword94 only: BIN10 visual84 -> CEL1 SPELLSP85, 46B9A9 mode2.
class OriginalSkill94Visual {
    std::vector<OriginalSkill32Visual::Frame> frames_;
    size_t frame_{};uint64_t elapsed_{};bool presented_{};
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static auto read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing sword94 resource");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});}
public:
    static std::vector<OriginalSkill32Visual::Frame> parse(const std::vector<uint8_t>& entry){
        constexpr std::array<unsigned,8> header{15,0,0,107,132,0,0,5};
        for(unsigned i=0;i<8;++i)if(word(entry,i*2)!=header[i])throw std::runtime_error("Unsupported sword94 SPELLSP header");
        constexpr size_t data=16+15*8;size_t previous=0;if(entry.size()<data)throw std::runtime_error("Truncated sword94 directory");std::vector<OriginalSkill32Visual::Frame> result;
        for(unsigned i=0;i<15;++i){size_t off=dword(entry,16+i*8),length=dword(entry,20+i*8);if(off!=previous||off>entry.size()-data||length>entry.size()-data-off)throw std::runtime_error("Invalid sword94 frame extent");result.push_back(OriginalSkill32Visual::decode({entry.begin()+data+off,entry.begin()+data+off+length},107,132));previous=off+length;}
        if(previous!=entry.size()-data)throw std::runtime_error("Unexpected sword94 trailing data");return result;
    }
    explicit OriginalSkill94Visual(const std::filesystem::path& root){auto bin=read(root/"SUPERSP.BIN"),pal=read(root/"SUPERSP.PAL"),cel=read(root/"SUPERSP.CEL");size_t t=dword(pal,920)-20;unsigned n=word(bin,t);if(n!=115||bin.at(t+2+93)!=84||bin.at(t+2+n+93)!=0||bin.at(t+2+n*2+93)!=233||bin.at(t+2+n*3+93)!=35)throw std::runtime_error("Unsupported sword94 BIN10 dispatch");size_t base=dword(pal,808)-56;if(word(cel,base)<85)throw std::runtime_error("Missing SPELLSP85");size_t off=dword(cel,base+2+84*8),length=dword(cel,base+6+84*8);if(base>cel.size()||off>cel.size()-base||length>cel.size()-base-off)throw std::runtime_error("Invalid sword94 archive extent");frames_=parse({cel.begin()+base+off,cel.begin()+base+off+length});}
    const auto& frames()const{return frames_;}const auto& current()const{return frames_.at(frame_);}size_t frame()const{return frame_;}bool done()const{return frame_==frames_.size();}
    void presented(){presented_=true;}void tick(unsigned ms){if(done())return;elapsed_+=ms;if(presented_&&elapsed_>=50){++frame_;elapsed_=0;presented_=false;}}
};
}
