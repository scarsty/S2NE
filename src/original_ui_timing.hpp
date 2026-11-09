#pragma once
#include <algorithm>
#include <cstdint>
namespace s2ne {
class OriginalEdgeScrollTiming {
    bool active_=false;
    uint64_t last_=0;
public:
    bool due(uint64_t now,bool active,unsigned delay_ms){
        if(!active){active_=false;return false;}
        if(!active_||now<last_||now-last_>=delay_ms){active_=true;last_=now;return true;}
        return false;
    }
};
class OriginalBannerPresentation {
    unsigned pose_=0,elapsed_=0;
    bool presented_=false;
public:
    void reset(){pose_=elapsed_=0;presented_=false;}
    unsigned pose()const{return pose_;}
    void presented(){presented_=true;}
    bool tick(unsigned milliseconds){
        elapsed_=std::min(10u,elapsed_+std::min(milliseconds,10u));
        if(!presented_)return false;
        if(pose_==8)return true;
        if(elapsed_<10)return false;
        ++pose_;elapsed_=0;presented_=false;return false;
    }
};
inline uint64_t original_ui_frame_delay(uint64_t started,uint64_t now){
    return now>=started&&now-started<10?10-(now-started):0;
}
}
