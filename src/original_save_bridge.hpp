#pragma once
#include "original_events.hpp"
#include "original_save.hpp"
#include <optional>
#include <utility>

namespace s2ne {
class OriginalSaveBridge {
    friend class OriginalInitialization;
    using Raw=OriginalSave::UnitRecord;
    static uint32_t get(std::span<const uint8_t> b,size_t p,unsigned n=2){
        uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(b[p+i])<<(8*i);return v;
    }
    static int signed_word(const Raw& r,size_t p){return int(int16_t(get(r,p)));}
    static void put(std::span<uint8_t> b,size_t p,uint32_t v,unsigned n=2){for(unsigned i=0;i<n;++i)b[p+i]=uint8_t(v>>(8*i));}
    [[noreturn]] static void unsupported(const char* message){throw std::runtime_error(message);}
    static void validate(const OriginalSave::State& raw){
        if(!raw.battle_context())unsupported("Save bridge supports battle context only");
        if(get(raw.globals_4f7cd0,0,4)!=0)unsupported("Unsupported original game mode");
        if(raw.globals_4f7cd0[28]<1||raw.globals_4f7cd0[28]>5)unsupported("Invalid saved difficulty");
        if(get(raw.globals_4f7cd0,12,4)<1||get(raw.globals_4f7cd0,12,4)>0x7FFFFFFF)unsupported("Invalid saved turn");
        for(const auto& group:raw.groups)if(group.units.size()>OriginalSave::maximum_units)
            unsupported("Original saved group exceeds runtime capacity");
    }
    static void decode_unit(const Raw& r,OriginalEvents::Unit& u){
        u.persistent_role6=signed_word(r,6);for(size_t i=0;i<u.relationships.size();++i)u.relationships[i]=int(int8_t(r[0x142+i]));
        u.id=get(r,2);u.appearance=get(r,4);u.x=int(int32_t(get(r,0x23,4)));u.y=int(int32_t(get(r,0x27,4)));
        u.name.clear();for(size_t p=0x10;p<0x23&&r[p];++p)u.name.push_back(r[p]);
        u.direction=r[0x2B]&3;u.animation_action=r[0x2B]>>2;u.action_state=r[0x2D];u.allegiance_tag=r[0x2E];u.runtime_role_selector=r[0x2F];
        // Saved records already belong to the live army; source arrival selectors
        // are not serialized at +A/+C (those are separate original role words).
        u.first_turn=1;u.last_turn=0;
        u.set_flags30(get(r,0x30,4));u.status34=r[0x34];u.status3c=r[0x3C];
        if(get(r,0x38,4)&0x400u){u.action_status_runtime.emplace();std::copy_n(r.begin(),0x13A,u.action_status_runtime->begin());}
        u.level=signed_word(r,0xB5);u.max_hp=signed_word(r,0xB7);u.max_mp=signed_word(r,0xB9);
        u.max_st=signed_word(r,0xBB);u.max_sp=signed_word(r,0xBD);u.base_action_points=signed_word(r,0xBF);
        u.strikeback=signed_word(r,0xC1);u.attack_limit=signed_word(r,0xC3);u.defense_limit=signed_word(r,0xC5);
        u.fencing=signed_word(r,0xC7);u.base_tactics=signed_word(r,0xC9);u.tactics=signed_word(r,0x138);
        u.experience_threshold=get(r,0x111);u.experience=get(r,0x113);
        u.abilities=get(r,0x115,4);u.kind_attribute=r[0x123];
        u.hp=signed_word(r,0x124);u.mp=signed_word(r,0x126);u.st=signed_word(r,0x128);u.sp=signed_word(r,0x12A);
        u.max_action_points=signed_word(r,0x12C);u.action_points=signed_word(r,0x12E);u.carry_points=signed_word(r,0x130);
        u.attack=signed_word(r,0x134);u.defense=signed_word(r,0x136);
        for(size_t i=0;i<6;++i){u.equipment[i]=get(r,0xCB+2*i);u.weapon_proficiency[i]=get(r,0xD7+2*i);}
        for(size_t i=0;i<11;++i){u.learned[i]=get(r,0xFB+2*i);u.learned_proficiency[i]=get(r,0xE3+2*i);}
        u.special_proficiency=get(r,0xF9);
        for(size_t i=0;i<4;++i){u.initial_status[i]=get(r,0x11B+2*i);u.equipment_stat_deltas[i]=signed_word(r,0x13A+2*i);}
        u.movement_pose=false;u.movement_frame=0;
    }
    static void patch_unit(Raw& r,const OriginalEvents::Unit& u){
        auto w=[&](size_t p,int value){if(value< -32768||value>32767)unsupported("Unit stat exceeds original signed WORD");put(r,p,uint16_t(value));};
        auto uw=[&](size_t p,unsigned value){if(value>65535)unsupported("Unit value exceeds original WORD");put(r,p,value);};
        if(u.movement_pose||u.movement_frame)unsupported("Cannot save during a native movement animation");
        if(u.name.size()>18||u.direction>3||u.animation_action>63||u.status_flags>255||u.allegiance_tag>255||u.kind_attribute>255)
            unsupported("Unit field exceeds original save representation");
        std::vector<uint8_t> old;for(size_t p=0x10;p<0x23&&r[p];++p)old.push_back(r[p]);
        if(old!=u.name){std::copy(u.name.begin(),u.name.end(),r.begin()+0x10);r[0x10+u.name.size()]=0;}
        put(r,0x23,uint32_t(u.x),4);put(r,0x27,uint32_t(u.y),4);r[0x2B]=uint8_t(u.direction|(u.animation_action<<2));
        if(u.action_status_runtime)std::copy(u.action_status_runtime->begin()+0x38,u.action_status_runtime->begin()+0xA0,r.begin()+0x38);
        r[0x2D]=u.action_state;r[0x2E]=uint8_t(u.allegiance_tag);put(r,0x30,u.flags30(),4);r[0x34]=u.status34;r[0x3C]=u.status3c;
        if(u.runtime_role_selector>255)unsupported("Original role selector exceeds BYTE");r[0x2F]=uint8_t(u.runtime_role_selector);
        w(6,u.persistent_role6);
        for(size_t i=0;i<u.relationships.size();++i){if(u.relationships[i]< -128||u.relationships[i]>127)unsupported("Original relationship exceeds signed BYTE");r[0x142+i]=uint8_t(u.relationships[i]);}
        // +A/+C preserve original source20/22 role words, not arrival51/52.
        w(0xB5,u.level);w(0xB7,u.max_hp);w(0xB9,u.max_mp);w(0xBB,u.max_st);w(0xBD,u.max_sp);
        w(0xBF,u.base_action_points);w(0xC1,u.strikeback);w(0xC3,u.attack_limit);w(0xC5,u.defense_limit);
        w(0xC7,u.fencing);w(0xC9,u.base_tactics);w(0x138,u.tactics);uw(0x111,u.experience_threshold);uw(0x113,u.experience);put(r,0x115,u.abilities,4);r[0x123]=uint8_t(u.kind_attribute);
        w(0x124,u.hp);w(0x126,u.mp);w(0x128,u.st);w(0x12A,u.sp);w(0x12C,u.max_action_points);w(0x12E,u.action_points);
        w(0x130,u.carry_points);w(0x134,u.attack);w(0x136,u.defense);
        for(size_t i=0;i<6;++i){uw(0xCB+2*i,u.equipment[i]);uw(0xD7+2*i,u.weapon_proficiency[i]);}
        for(size_t i=0;i<11;++i){uw(0xFB+2*i,u.learned[i]);uw(0xE3+2*i,u.learned_proficiency[i]);}
        uw(0xF9,u.special_proficiency);
        for(size_t i=0;i<4;++i){uw(0x11B+2*i,u.initial_status[i]);w(0x13A+2*i,u.equipment_stat_deltas[i]);}
    }
public:
    struct Restored { OriginalEvents events;unsigned turn;std::pair<int,int> camera;OriginalSave::State raw; };
    // Camera is not in sub_4362B4's serialized fields. Original map reload resets it.
    static constexpr std::pair<int,int> restored_camera{32,16};
    static Restored restore(const std::filesystem::path& root,const OriginalSave::State& raw){
        validate(raw);OriginalEvents events(root,raw.globals_4f7cd0[28]);
        events.load_stage(get(raw.globals_4f7cd0,8,4));
        events.units.clear();
        for(unsigned group=0;group<2;++group)for(const auto& record:raw.groups[group].units){
            OriginalEvents::Unit u{};decode_unit(record,u);
            // +8 is the original AI/control class; group supplies allegiance.
            // Preserve the other role words in raw state instead of inventing stage behaviors.
            int ai=signed_word(record,8);
            if(ai<0||ai>1)unsupported("Unsupported original saved control class");
            u.behavior=group==1?2:ai?1:0;
            events.restore_unit_resources(u);events.units.push_back(std::move(u));
        }
        events.restore_event_states(raw.event_states);events.flags=raw.flags_4f7d10;
        events.campaign_currencies=std::array<int32_t,2>{int32_t(get(raw.stage_4fb478,0,4)),int32_t(get(raw.stage_4fb478,4,4))};
        return {std::move(events),get(raw.globals_4f7cd0,12,4),restored_camera,raw};
    }
    static OriginalSave::State capture_promotion(const OriginalEvents& events,unsigned turn,std::pair<int,int> camera,
            const OriginalSave::State& baseline,size_t selected,unsigned source_kind,
            std::span<const uint8_t> inventory,uint8_t army_metadata){
        validate(baseline);
        if(selected>=baseline.groups[0].units.size()||selected>=events.units.size())unsupported("Promotion requires original allied unit identity");
        const auto& old=baseline.groups[0].units[selected];const auto& unit=events.units[selected];
        if(get(old,2)!=unit.id||get(old,4)!=source_kind||unit.appearance==source_kind||unit.hostile())unsupported("Stale original promotion identity");
        if(inventory.size()!=baseline.groups[0].inventory.size())unsupported("Promotion requires complete original inventory");
        auto staged=baseline;put(staged.groups[0].units[selected],4,unit.appearance);
        staged.groups[0].inventory.assign(inventory.begin(),inventory.end());staged.groups[0].metadata[4]=army_metadata;
        return capture(events,turn,camera,&staged);
    }
    static OriginalSave::State capture(const OriginalEvents& events,unsigned turn,std::pair<int,int> camera,
                                      const OriginalSave::State* baseline=nullptr){
        if(!baseline)unsupported("Fresh original save requires complete original raw state; unknown globals/inventory cannot be invented");
        validate(*baseline);
        (void)camera; // Original intentionally reconstructs the camera instead of serializing it.
        if(!turn||turn>0x7FFFFFFF)unsupported("Invalid original turn");
        if(events.stage!=get(baseline->globals_4f7cd0,8,4)||events.difficulty!=baseline->globals_4f7cd0[28])
            unsupported("Stage/difficulty changed without original full-state transition");
        auto result=*baseline;size_t index=0;
        for(unsigned group=0;group<2;++group)for(auto& record:result.groups[group].units){
            if(index>=events.units.size())unsupported("Native unit set no longer matches raw save");
            const auto& u=events.units[index++];
            if(u.id!=get(record,2)||u.appearance!=get(record,4)||(group==1)!=u.hostile())
                unsupported("Native unit identity/order/group changed");
            patch_unit(record,u);
        }
        if(index!=events.units.size())unsupported("Native unit set no longer matches raw save");
        if(events.campaign_currencies)for(size_t i=0;i<2;++i)put(result.stage_4fb478,i*4,uint32_t((*events.campaign_currencies)[i]),4);
        result.flags_4f7d10=events.flags;result.event_states=events.save_event_states();
        if(result.event_states.size()!=baseline->event_states.size())unsupported("Stage event set changed");
        put(result.globals_4f7cd0,12,turn,4);return result;
    }
};
}
