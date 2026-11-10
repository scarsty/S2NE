#pragma once
#include <bit>
#include <cstdint>
#include <functional>
#include "runtime_error.hpp"

namespace s2ne {
// Single original CRT thread stream plus the shared 506140 wrapper counter.
// Time is provided explicitly: speculative copies must not read wall-clock time.
class OriginalRandom {
public:
    struct State {
        uint32_t seed=1;
        uint32_t calls=0;
        bool operator==(const State&)const=default;
    };
    using TimeSource=std::function<uint32_t()>;
private:
    State state_;
public:
    explicit OriginalRandom(State state):state_(state){}
    State state()const{return state_;}
    void restore(State state){state_=state;}
    void srand(uint32_t seed){state_.seed=seed;}
    unsigned rand(){
        state_.seed=state_.seed*uint32_t(0x41C64E6D)+uint32_t(0x3039);
        return (state_.seed>>16)&0x7FFF;
    }
    // 4732ED: signed IDIV bound, zero returns without counter/seed/time changes.
    Result<unsigned> try_bounded(int32_t bound,const TimeSource& time={}){
        if(!bound)return Result<unsigned>::success(0);
        State next=state_;++next.calls;
        if(std::bit_cast<int32_t>(next.calls)>=200){
            if(!time)return Result<unsigned>::failure(make_error("Original RNG reseed requires explicit time source"));
            next.seed=time();next.calls=0;
        }
        next.seed=next.seed*uint32_t(0x41C64E6D)+uint32_t(0x3039);
        unsigned value=((next.seed>>16)&0x7FFF)%uint32_t(bound<0?-int64_t(bound):int64_t(bound));
        state_=next;return Result<unsigned>::success(value);
    }
    unsigned bounded(int32_t bound,const TimeSource& time={}){auto result=try_bounded(bound,time);if(!result)fatal(result.error());return result.value();}

};
}
