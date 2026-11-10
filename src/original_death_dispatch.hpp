#pragma once
#include "original_events.hpp"
namespace s2ne {
// 41C02A's ordered type3/condition2 scan. A positive dialogue count
// requires synchronous41CA6E followed by41FF30; a queue is not equivalent.
class OriginalDeathDispatch {
public:
    struct Record { unsigned type{},condition{},role{},lines{}; };
    enum class Kind { none, empty_match, synchronous_script };
    struct Plan { Kind kind{Kind::none};size_t event{};unsigned native_return{};bool settled()const{return kind!=Kind::synchronous_script;} };
    static Plan inspect(std::span<const Record> records,unsigned role){
        for(size_t i=0;i<records.size();++i){const auto& r=records[i];
            if(r.type!=3||r.condition!=2||r.role!=role)continue;
            // MOV [record+10h],SAR16 is a SIGNED line count. First matching
            // empty record exits the whole scan, not just this iteration.
            if(int16_t(r.lines)<=0)return {Kind::empty_match,i,0};
            // Conditions1/3/4 cannot reach41C161 via this dispatcher's filter.
            // The reachable native return is hexadecimal12 (decimal18).
            return {Kind::synchronous_script,i,0x12};
        }
        return {};
    }
    static Plan inspect(const OriginalEvents& events,unsigned role){
        std::vector<Record> records;records.reserve(events.event_count());
        for(size_t i=0;i<events.event_count();++i)records.push_back({events.type(i),events.condition(i),unsigned(uint16_t(events.event_word(i,3))),events.line_count(i)});
        return inspect(records,role);
    }
};
}
