#pragma once
#include "original_events.hpp"
#include <optional>
namespace s2ne {
// 4211A3 conditions0..6. False predicates remain pending, not consumed.
inline std::optional<bool> original_role_predicate(const OriginalEvents& events,size_t event){
    if(events.type(event)!=11)return std::nullopt;
    int condition=events.condition(event),id=events.event_word(event,5);
    auto allied=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return !u.hostile()&&int(u.id)==id;});
    auto any=allied;if(any==events.units.end())any=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return u.hostile()&&int(u.id)==id;});
    if(condition==0)return any!=events.units.end()&&(any->status_flags&0x20)!=0;
    if(condition==1)return any!=events.units.end()&&(any->status_flags&0x20)==0;
    // 440D4C(mode0) accepts any matching identity except 40h without80h;
    // it does not test HP, death20h, appearance, or alternate-form10h.
    if(condition==2){for(const auto& u:events.units)if(int(u.id)==id&&(!(u.status_flags&0x40)||(u.status_flags&0x80)))return true;return false;}
    if(condition==3)return allied==events.units.end();
    if(condition==4)return allied!=events.units.end()&&(allied->status_flags&0x40)&&!(allied->status_flags&0x80);
    if(condition==5)return allied!=events.units.end()&&(allied->status_flags&0x80)!=0;
    if(condition==6)return allied!=events.units.end()&&!(allied->status_flags&0x40)&&(allied->status_flags&0x80);
    return std::nullopt;
}
}
