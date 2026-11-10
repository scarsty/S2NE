#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <span>
#include <vector>
#include <stdexcept>

namespace s2ne::original_combat {
// 453238 consumes indexed AD/B1 buffers, not an RGBA framebuffer.
// The caller retains runtime40's draw-dependent alternating bit.
inline std::vector<uint8_t> reaction_bitmap(unsigned width,unsigned height,
    std::span<const uint8_t> current,std::span<const uint8_t> overlap,
    std::span<const uint8_t> foot_mask,std::span<const uint8_t> shadow,bool left){
    if(!width||height<32||current.size()!=size_t(width)*height||overlap.size()!=current.size()||foot_mask.size()!=64*32||shadow.size()!=256)
        throw std::invalid_argument("Original reaction indexed buffers");
    auto source=std::vector<uint8_t>(current.begin(),current.end());
    // 453334 fills transparent sprite pixels; it never replaces opaque art.
    int foot_left=int(width/2)-32;
    for(int y=0;y<32;++y)for(int x=0;x<64;++x){int dx=foot_left+x;
        if(dx<0||dx>=int(width))continue;
        size_t p=size_t(height-32+unsigned(y))*width+unsigned(dx);
        if(foot_mask[size_t(y)*64+x]&&!source[p])source[p]=foot_mask[size_t(y)*64+x];
    }
    std::vector<uint8_t> result(size_t(128)*height);
    int offset=(128-int(width))/2-4;
    // Original signed division truncates toward zero, unlike arithmetic shift.
    int source_x=offset<0&&left?-offset:0;
    int count=offset<0?int(width)+offset:int(width);
    int destination=offset<0?0:left?offset:128-offset-int(width);
    for(unsigned y=0;y<height;++y)for(int x=0;x<count;++x){int dx=destination+x;
        if(dx<0||dx>=128)continue;
        size_t p=size_t(y)*width+unsigned(source_x+x);
        result[size_t(y)*128+unsigned(dx)]=source[p]==32?shadow[overlap[p]]:source[p];
    }
    return result;
}

// 455048(ebx=0,ecx=0): five original indexed mixes, presented at 20ms.
// Gameplay presence is deliberately caller-owned until finished().
struct Disappearance {
    unsigned target{};unsigned frame{},elapsed{},frame_count{5},frame_interval{20},first_weight{40};bool running{},done{};
    std::vector<uint8_t> source,background;
    void begin(unsigned id,std::span<const uint8_t> a,std::span<const uint8_t> b){
        if(a.empty()||a.size()!=b.size())throw std::invalid_argument("Original disappearance buffers");
        target=id;frame=elapsed=0;running=true;done=false;source.assign(a.begin(),a.end());background.assign(b.begin(),b.end());
    }
    bool active()const{return running;}
    bool finished()const{return done;}
    void death_mode(){frame_count=10;frame_interval=30;first_weight=10;}
    unsigned percentage()const{return first_weight+frame*10;}
    void tick(unsigned ms){if(!running)return;elapsed+=std::min(ms,frame_interval);if(elapsed<frame_interval)return;elapsed=0;
        if(++frame==frame_count){running=false;done=true;}
    }
    template<class Blend>std::vector<uint8_t> pixels(Blend blend)const{
        std::vector<uint8_t> result(source.size());
        for(size_t i=0;i<source.size();++i)if(source[i]&&background[i])result[i]=blend(source[i],background[i],int(percentage()));
        return result;
    }
};

// 452E7F and 43EB67/43EA08. Coordinates are relative to 42C50A's
// projected tile origin, not a fabricated particle/sprite animation.
struct Feedback {
    enum class Phase { idle, windup, flash, numbers };
    Phase phase=Phase::idle;
    size_t attacker{},target{};
    int damage{},remaining{},hp_before{},hp_after{},step{};
    unsigned elapsed{};
    bool miss{};
    bool active()const{return phase!=Phase::idle;}
    bool silhouette()const{return phase==Phase::flash;}
    unsigned silhouette_palette()const{return 0x3c;}
    bool text_visible()const{return phase==Phase::numbers&&step<7;}
    std::string text()const{return miss?"MISS":std::to_string(damage);}
    int text_x()const{return 32-int(text().size())*8;}
    int text_y()const{return -64-step*8;}
    unsigned text_foreground()const{return miss?0x68:0x3c;}
    unsigned text_background()const{return miss?0x24:0x56;}
    unsigned interval()const{return miss?30u:step<3?30u:step<4?20u:step<6?10u:0u;}
    void begin(size_t a,size_t b,int hp,int amount,bool missed,bool windup){
        *this={};attacker=a;target=b;damage=remaining=std::min(hp,std::max(0,amount));
        hp_before=hp_after=hp;miss=missed;phase=windup?Phase::windup:Phase::flash;
    }
    // Called once for each original presented HP step; no RNG here.
    int consume(){
        if(miss)return 0;
        int chunk=std::min(remaining,int(uint32_t(1)<<std::min(step,30)));
        remaining-=chunk;hp_after-=chunk;return chunk;
    }
};
}
