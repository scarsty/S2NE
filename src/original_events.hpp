#pragma once
#include "original_combat.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne {
class OriginalEvents {
    std::vector<uint8_t> bin_, pal_, kinds_, items_;
    size_t events_{};
    int proficiency_difficulty_scale_{};
    size_t combat_config_{};
    static std::vector<uint8_t> read(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Missing original event resource");
        return {std::istreambuf_iterator<char>(file), {}};
    }
    unsigned word(size_t p) const { return bin_.at(p) | (unsigned(bin_.at(p+1)) << 8); }
    unsigned dword(size_t p) const { return word(p) | (word(p+2) << 16); }
    size_t entry(unsigned id) const {
        size_t p = 848 + (id-1)*8;
        unsigned value = 0;
        for (unsigned i=0;i<4;++i) value |= unsigned(pal_.at(p+i)) << (8*i);
        return value - 11 - (id-1);
    }
    size_t record(size_t event) const {
        if (event >= event_count()) throw std::runtime_error("Invalid original event index");
        return events_ + 4 + event*20;
    }
public:
    struct Line { int speaker; std::vector<uint8_t> name, text; };
    unsigned stage{}, first_stage{}, alternate_stage{};
    std::optional<std::array<int32_t,2>> campaign_currencies;
    struct Unit {
        unsigned id, appearance; int x,y; std::vector<uint8_t> name; int first_turn{},last_turn{};
        int hp{},max_hp{},mp{},st{},sp{},strikeback{},attack{},defense{},fencing{},tactics{},behavior{};
        int action_points{},base_action_points{},carry_points{},movement_type{};
        int base_tactics{}; // Signed runtime +C9h; tactics is the distinct current +138h.
        unsigned experience_threshold{}; // WORD runtime +111h, KINDSP record +0Eh.
        uint32_t runtime_flags30{}; // DWORD +30h; high byte overlaps status_flags (+33h).
        unsigned direction{},animation_action{};
        uint8_t action_state{}; // Original runtime +2Dh: 0 idle, 1 selected, 2 finished, 3 moving, 4 moved.
        bool movement_pose{};
        unsigned movement_frame{};
        int level{1}, max_mp{}, max_st{}, max_sp{}, max_action_points{}, attack_limit{}, defense_limit{}; // max_action_points is runtime +12Ch, not current +12Eh.
        unsigned portrait{}, status_flags{0x80}, experience{};
        std::vector<uint8_t> profession;
        std::array<unsigned,6> equipment{};
        std::array<int,4> equipment_stat_deltas{}; // runtime +13Ah/+13Ch/+13Eh/+140h
        std::array<unsigned,4> initial_status{};
        std::array<unsigned,11> learned{};
        std::array<unsigned,11> learned_proficiency{}; // runtime +E3h; learned bits remain at +FBh
        std::array<unsigned,6> weapon_proficiency{}; // runtime +D7h..+E1h
        unsigned special_proficiency{}; // runtime +F9h
        uint32_t abilities{}; // runtime +115h; +116h = bits 8..15, +118h = bits 24..31
        uint8_t status34{},status3c{}; // runtime state, not fields in the 101-byte stage record
        std::optional<std::array<uint8_t,0x13A>> action_status_runtime;
        unsigned kind_attribute{}, allegiance_tag{};
        unsigned runtime_role_selector{};
        int persistent_role6{};
        std::array<int,40> relationships{};
        // sub_4274B5 partitions stage units at the first behavior 2/3/8; behavior 1 is allied AI.
        uint32_t flags30()const{return (runtime_flags30&0x00FFFFFFu)|((status_flags&255u)<<24);}
        void set_flags30(uint32_t value){runtime_flags30=value;status_flags=value>>24;}
        bool hostile()const{return behavior==2||behavior==3||behavior==8||behavior==9||behavior==10;}
        bool controllable()const{return behavior==0;}
        bool npc_class()const{return behavior!=0&&behavior!=5&&behavior!=6&&behavior!=7;}
        bool present(unsigned turn=1)const{return hp>0&&first_turn<=int(turn)&&(last_turn==0||last_turn>=int(turn));}
    };
    unsigned map_id{};
    std::vector<std::array<unsigned,4>> dynamic_object_chains;
    std::vector<Unit> units;
    std::vector<uint8_t> briefing,stage_name,victory_objective,defeat_objective;
    std::array<int,12> battle_config{};
    unsigned learned_ability_count{},first_ability_count{},second_ability_count{};
    int victory_relationship_increment()const{return int(int8_t(bin_.at(combat_config_+0xFB)));}
    struct DeploymentSlot { int x{},y{};unsigned direction{}; };
    std::vector<DeploymentSlot> deployment_slots()const{
        size_t h=entry(1)+2+(stage-1)*30,data=entry(1)+dword(h),characters=data+dword(h+26);
        std::vector<DeploymentSlot> result;
        for(unsigned i=0;i<bin_.at(h+8);++i){size_t p=characters+i*101;
            int behavior=int(int8_t(bin_.at(p+49)));if(behavior==2||behavior==3||behavior==8)break;
            if(int(int8_t(bin_.at(p+51)))==1&&behavior==0)
                result.push_back({int(int16_t(word(p+23))),int(int16_t(word(p+25))),bin_.at(p+50)});
        }
        return result;
    }
    bool has_learned_ability(const Unit& unit,unsigned mode=3,bool suppress_first=false,bool suppress_second=false)const{
        if(!unit.hostile()){
            if(suppress_first){if(mode==1)return false;if(mode==3)mode=2;}
            if(suppress_second){if(mode==2)return false;if(mode==3)mode=1;}
        }
        unsigned begin=mode==2?first_ability_count:0;
        unsigned end=mode==1?first_ability_count:mode==2?first_ability_count+second_ability_count:learned_ability_count;
        for(size_t i=begin;i<std::min(size_t(end),unit.learned.size());++i)if(unit.learned[i])return true;
        return false;
    }
    bool equipment_special(const Unit& unit,unsigned special)const{
        for(unsigned item:unit.equipment){
            if(!item)continue;
            size_t p=2+size_t(item-1)*38;
            if(items_.at(p+15)==0&&items_.at(p+25)==special)return true;
        }
        return false;
    }
    int action_status_kind(const Unit& unit)const{
        size_t table=unsigned(kinds_.at(18))|(unsigned(kinds_.at(19))<<8)|(unsigned(kinds_.at(20))<<16)|(unsigned(kinds_.at(21))<<24);
        return int(int8_t(kinds_.at(table+size_t(unit.appearance-1)*37+17)));
    }
    struct ActionRecord { unsigned index;std::array<int,23> fields; };
    static original_combat::ActionStatusInput action_status_input(const ActionRecord& action,original_combat::ActionStatusInput target){
        target.effect=action.fields[17];target.chance=action.fields[18];target.duration=action.fields[19];
        return target;
    }
    static original_combat::ActionStatusResult apply_action_record_status(std::span<uint8_t> runtime,const ActionRecord& action,original_combat::ActionStatusInput target){
        return original_combat::apply_action_status(runtime,action_status_input(action,target));
    }
    std::vector<ActionRecord> action_records(const Unit& unit)const{
        auto kw=[&](size_t p){return unsigned(kinds_.at(p))|(unsigned(kinds_.at(p+1))<<8);};
        size_t directory=kw(30)+(size_t(kw(32))<<16);unsigned count=kw(22);
        if(!unit.appearance||unit.appearance>count)throw std::runtime_error("Invalid original action appearance");
        size_t p=directory+size_t(unit.appearance-1)*4,records=directory+size_t(count)*4;
        int first=int(int16_t(kw(p))),length=int(int16_t(kw(p+2)));
        if(first<0||length<0)throw std::runtime_error("Invalid original action directory");
        std::vector<ActionRecord> result;
        for(int i=0;i<length;++i){ActionRecord action{unsigned(first+i),{}};for(size_t n=0;n<action.fields.size();++n)action.fields[n]=int(int8_t(kinds_.at(records+size_t(first+i)*23+n)));result.push_back(action);}
        return result;
    }
    bool action_equipment_allowed(const Unit& unit,const ActionRecord& action)const{
        unsigned weapon=unit.equipment[0];int type=action.fields[0];
        if(weapon==83)return true;
        if(type==1)return weapon==0;
        if(type>=2&&type<=7)return weapon&&int(int8_t(items_.at(2+size_t(weapon-1)*38+14)))==type-1;
        return type>=21;
    }
    int stage_adjustment{};
    void adjust_battle_entry_stats(){
        size_t header=entry(1)+2+(stage-1)*30;
        if(bin_.at(header+21)==3)return;
        unsigned baseline=bin_.at(header+23),divisor=bin_.at(header+24);
        if(!baseline||!divisor)return;
        int sum=0,count=0;bool enemy_context=false;
        for(const auto& unit:units){
            enemy_context=enemy_context||unit.behavior==2||unit.behavior==3||unit.behavior==8;
            if(enemy_context)break;
            if((unit.status_flags&0x80)&&!(unit.status_flags&0x30)&&!unit.npc_class()){sum+=combat_value(unit);++count;}
        }
        int percentage=original_combat::stage_stat_adjustment(count?sum/count:sum,baseline,divisor,int(difficulty));
        if(!percentage)return;
        stage_adjustment=percentage;enemy_context=false;
        for(auto& unit:units){
            enemy_context=enemy_context||unit.behavior==2||unit.behavior==3||unit.behavior==8;
            if(!enemy_context||!(unit.status_flags&0x80)||(unit.status_flags&0x30))continue;
            std::array<uint8_t,0x13A> runtime{};
            auto put=[&](size_t p,int value){runtime[p]=uint8_t(value);runtime[p+1]=uint8_t(unsigned(value)>>8);};
            auto get=[&](size_t p){return int(int16_t(unsigned(runtime[p])|(unsigned(runtime[p+1])<<8)));};
            put(0xB7,unit.max_hp);put(0x124,unit.hp);put(0xB9,unit.max_mp);put(0x126,unit.mp);
            put(0xBB,unit.max_st);put(0x128,unit.st);put(0xC3,unit.attack_limit);put(0x134,unit.attack);
            put(0xC5,unit.defense_limit);put(0x136,unit.defense);put(0xC7,unit.fencing);
            put(0xC9,unit.base_tactics);put(0x138,unit.tactics);
            original_combat::adjust_runtime_stats(runtime,percentage);
            unit.max_hp=get(0xB7);unit.hp=get(0x124);unit.max_mp=get(0xB9);unit.mp=get(0x126);
            unit.max_st=get(0xBB);unit.st=get(0x128);unit.attack_limit=get(0xC3);unit.attack=get(0x134);
            unit.defense_limit=get(0xC5);unit.defense=get(0x136);unit.fencing=get(0xC7);unit.base_tactics=get(0xC9);unit.tactics=get(0x138);
        }
    }
    int combat_value(const Unit& unit,int mode=0)const{
        if(mode<0||mode>2)throw std::invalid_argument("Unsupported original combat value cache mode");
        auto kw=[&](size_t p){return unsigned(kinds_.at(p))|(unsigned(kinds_.at(p+1))<<8);};
        size_t kind=kw(18)+(size_t(kw(20))<<16)+size_t(unit.appearance-1)*37;
        original_combat::CombatValueInput in{};
        in.value18=mode==2?unit.max_hp:unit.hp;
        in.value14=mode==2?unit.max_sp:unit.sp;
        in.value10=mode==2?unit.max_mp:unit.mp;
        in.valueC=mode==2?unit.max_st:unit.st;
        in.runtime134=unit.attack;in.runtime136=unit.defense;in.runtime138=unit.tactics;
        in.runtimeC1=unit.strikeback;in.runtimeBF=unit.base_action_points;in.runtimeC7=unit.fencing;
        for(unsigned i=0;i<11;++i)if(unit.learned[i]){if(i<first_ability_count)++in.learned_first;else ++in.learned_second;}
        in.abilities=unit.abilities;in.equipment_18=equipment_special(unit,0x18);
        in.kind_multiplier=kinds_.at(kind+0x21);
        auto config=[&](size_t p){return int(int8_t(bin_.at(combat_config_+p)));};
        in.configD6=config(0xD6);in.configD7=config(0xD7);in.configD8=config(0xD8);in.configD9=config(0xD9);
        in.configDA=config(0xDA);in.configDB=config(0xDB);in.configDC=config(0xDC);in.configDF=config(0xDF);
        if(mode==1)throw std::invalid_argument("Original runtime132 settlement snapshot not yet available");
        return original_combat::combat_value(in);
    }
    struct ActionRange { int minimum{},maximum{},shape{}; };
    ActionRange action_range(const Unit& unit,const ActionRecord& action)const{
        ActionRange range{action.fields[13],action.fields[14],action.fields[22]};
        int type=action.fields[0];unsigned weapon=unit.equipment[0];
        if(type>=1&&type<=10&&weapon){
            size_t p=2+size_t(weapon-1)*38;
            int encoded=int(int8_t(items_.at(p+16)));
            range.maximum=encoded%10;
            range.minimum=encoded==1?1:encoded<10?2:1;
            int shape=int(int8_t(items_.at(p+30)));
            if(shape>0)range.shape=shape;
        }
        if(range.minimum==0)range.minimum=range.maximum=1;
        return range;
    }
    int maximum_action_range(const Unit& unit)const{
        int maximum=0;
        for(const auto& action:action_records(unit))
            if(action_equipment_allowed(unit,action))maximum=std::max(maximum,action_range(unit,action).maximum);
        return maximum;
    }
    int action_accuracy_bonus(const Unit& unit,const ActionRecord& action)const{
        int bonus=action.fields[12];
        if(action.fields[0]>=21)return bonus;
        unsigned weapon=unit.equipment[0];
        int accuracy=weapon?int(int8_t(items_.at(2+size_t(weapon-1)*38+19))):90;
        return bonus*accuracy/100;
    }
    int action_effect_code(const Unit& unit,const ActionRecord& action)const{
        unsigned weapon=unit.equipment[0];
        if(action.fields[0]>=1&&action.fields[0]<=7&&weapon){
            int count=int(int8_t(items_.at(2+size_t(weapon-1)*38+28)));
            if(count>0)return count;
        }
        return action.fields[20];
    }
    struct ActionEffect { unsigned resource{},mode{};int sound{}; };
    static ActionEffect action_effect(const ActionRecord& action,int code){
        constexpr std::array<unsigned,9> resources{43,59,75,91,107,123,124,125,126};
        if(code<1||code>9)return {};
        int sound=code==5?127:int(int16_t(uint16_t(uint8_t(action.fields[15]))|(uint16_t(uint8_t(action.fields[16]))<<8)));
        return {resources[size_t(code-1)],code>=6&&code<=8?2u:1u,sound};
    }
    // 43E36B chooses original CEL orientation by integer slope, not atan2.
    static unsigned effect_initial_direction(int source_x,int source_y,int target_x,int target_y,unsigned mode){
        if(mode==2)return 0;
        if(source_x==target_x)return source_y>target_y?0:8;
        if(source_y==target_y)return source_x>target_x?12:4;
        int dx=source_x-target_x,dy=source_y-target_y;
        int slope=(dy<0?-dy:dy)*100/(dx<0?-dx:dx);
        if(slope<7)return source_x<target_x?4:12;
        if(slope>=350)return source_y>target_y?0:8;
        unsigned offset=slope<30?3:slope<83?2:1;
        if(source_x<target_x)return source_y>target_y?offset:8-offset;
        return source_y>target_y?16-offset:8+offset;
    }
    struct EffectFlight { int source_x{},source_y{},target_x{},target_y{},x{},y{},slope{};unsigned axis{},direction{}; };
    static EffectFlight effect_flight(int source_x,int source_y,int target_x,int target_y,unsigned mode){
        EffectFlight flight{source_x+32,source_y-32,target_x+32,target_y-32};
        flight.axis=source_x==target_x?1:source_y==target_y?2:3;
        if(flight.axis==3){int dx=source_x-target_x,dy=source_y-target_y;flight.slope=(dy<0?-dy:dy)*100/(dx<0?-dx:dx);}
        unsigned orientation=mode==2?0:effect_initial_direction(source_x,source_y,target_x,target_y,mode)+1;
        if(orientation>9)flight.source_x-=32;
        else if(orientation==9)flight.source_y+=48;
        flight.direction=mode==2?0:orientation-1;
        flight.x=flight.source_x;flight.y=flight.source_y;
        // The original reclassifies after the directional source offset, without a second decrement.
        if(flight.axis==3){
            if(flight.source_x==flight.target_x){flight.axis=1;if(mode==1)flight.direction=flight.source_y>flight.target_y?1:9;}
            else if(flight.source_y==flight.target_y){flight.axis=2;if(mode==1)flight.direction=flight.source_x>flight.target_x?13:5;}
        }
        return flight;
    }
    static bool advance_effect_flight(EffectFlight& f){
        if(f.x==f.target_x&&f.y==f.target_y)return false;
        int dx=f.target_x-f.x,dy=f.target_y-f.y;
        if(f.axis==1){if((dy<0?-dy:dy)<=32){f.x=f.target_x;f.y=f.target_y;}else f.y+=dy<0?-32:32;}
        else if(f.axis==2){if((dx<0?-dx:dx)<=32){f.x=f.target_x;f.y=f.target_y;}else f.x+=dx<0?-32:32;}
        else if(dx*dx+dy*dy<64){f.x=f.target_x;f.y=f.target_y;}
        else if(f.slope>50){f.y+=dy<0?-16:16;f.x=f.source_x+(f.target_x-f.source_x)*(f.y-f.source_y)/(f.target_y-f.source_y);}
        else{f.x+=dx<0?-16:16;f.y=f.source_y+(f.target_y-f.source_y)*(f.x-f.source_x)/(f.target_x-f.source_x);}
        return true;
    }
    int equipment_evasion(const Unit& unit)const{
        int value=0;
        for(unsigned item:unit.equipment)if(item)value+=int(int8_t(items_.at(2+size_t(item-1)*38+23)));
        return value;
    }
    int equipment_accuracy(const Unit& unit)const{
        int value=0;
        for(unsigned item:unit.equipment)if(item)value+=int(int8_t(items_.at(2+size_t(item-1)*38+20)));
        return value;
    }
    int class_matchup(const Unit& attacker,const Unit& defender)const{
        auto kw=[&](size_t p){return unsigned(kinds_.at(p))|(unsigned(kinds_.at(p+1))<<8);};
        size_t records=kw(18)+(size_t(kw(20))<<16),matrix=kw(24)+(size_t(kw(26))<<16);
        unsigned count=kw(28);
        if(!attacker.appearance||!defender.appearance)throw std::runtime_error("Invalid original matchup appearance");
        int a=int(int8_t(kinds_.at(records+size_t(attacker.appearance-1)*37+17)));
        int d=int(int8_t(kinds_.at(records+size_t(defender.appearance-1)*37+17)));
        if(a<1||d<1||unsigned(a)>count||unsigned(d)>count)throw std::runtime_error("Invalid original class matchup index");
        return int(int8_t(kinds_.at(matrix+size_t(a-1)*count+unsigned(d-1))));
    }
    unsigned movement_class(const Unit& unit)const{
        if(unit.movement_type==7||(unit.status3c&2))return 1;
        if(unit.movement_type==8||equipment_special(unit,15))return 2;
        if(unit.movement_type==6)return 3;
        if((unit.status3c&1)||equipment_special(unit,17))return 4;
        if(unit.status_flags&4)return 5;
        return 0;
    }
    unsigned difficulty{};
    std::array<uint8_t,80> flags{};
    explicit OriginalEvents(const std::filesystem::path& root, unsigned level)
        : bin_(read(root/"SUPERSP.BIN")), pal_(read(root/"SUPERSP.PAL")), difficulty(level) {
        if (level < 1 || level > 5) throw std::runtime_error("Invalid new-game difficulty");
        // sub_428658: four count-byte tables, followed by two length-prefixed blocks.
        size_t config = entry(10);
        config += 2 + 4*word(config);
        config += 2 + word(config);
        config += 2;
        combat_config_=config;
        proficiency_difficulty_scale_=int(int8_t(bin_.at(config+0xE0)));
        learned_ability_count=bin_.at(entry(2)+1); // 463914 -> 443235(mode 3), runtime unit +FBh words
        first_ability_count=bin_.at(entry(2)+2);second_ability_count=bin_.at(entry(2)+3);
        if(learned_ability_count>11||first_ability_count+second_ability_count>11)throw std::runtime_error("Invalid original learned ability count");
        kinds_=read(root/"KINDSP.BIN");
        items_=read(root/"ITEMSP.BIN");
        for(size_t i=0;i<battle_config.size();++i)battle_config[i]=int(int8_t(bin_.at(config+0xE8+i)));
        stage = bin_.at(config+0x100);
        first_stage = bin_.at(config+0xFE);
        alternate_stage = bin_.at(config+0xFF);
        load_stage(stage);
    }
    void load_stage(unsigned id) {
        stage_adjustment=0;
        size_t table = entry(1);
        if (!id || id > word(table)) throw std::runtime_error("Invalid original stage ID");
        size_t header = table + 2 + (id-1)*30;
        size_t data = table + dword(header);
        size_t end = data + dword(header+4);
        if (end > bin_.size() || end < data) throw std::runtime_error("Truncated original stage");
        map_id=bin_.at(header+9);
        // 426EFC reads type-5 words from the stage's initial type/count directory.
        dynamic_object_chains.clear();
        unsigned section_count=bin_.at(header+25);size_t section_data=data+section_count*4;
        for(unsigned i=0;i<section_count;++i){
            unsigned type=word(data+i*4),count=word(data+i*4+2);
            if(section_data>end||size_t(count)*2>end-section_data)throw std::runtime_error("Truncated original stage section");
            if(type==5){
                if(count/4>20||count%4)throw std::runtime_error("Invalid original dynamic object chains");
                for(unsigned j=0;j<count;j+=4)dynamic_object_chains.push_back({word(section_data+j*2),word(section_data+(j+1)*2),word(section_data+(j+2)*2),word(section_data+(j+3)*2)});
                break;
            }
            section_data+=count*2;
        }
        // 427016 sets these persistent flags when the stage header enables them.
        for(unsigned i=0;i<3;++i)if(bin_.at(header+18+i))flags[i]=1;
        units.clear();
        size_t characters=data+dword(header+26);
        bool enemy_context=false;
        for(unsigned i=0;i<bin_.at(header+8);++i){
            size_t p=characters+i*101;
            Unit unit{word(p),word(p+16),int(int16_t(word(p+23))),int(int16_t(word(p+25))),{}};
            // sub_4443B3 expands these signed words into runtime +124h..+136h.
            auto stat=[&](size_t offset){return int(int16_t(word(p+offset)));};
            unit.hp=unit.max_hp=stat(27);unit.mp=unit.max_mp=stat(29);
            unit.st=unit.max_st=stat(31);unit.sp=unit.max_sp=stat(33);
            unit.strikeback=stat(35);unit.attack=unit.attack_limit=stat(37);
            unit.defense=unit.defense_limit=stat(39);unit.fencing=stat(41);unit.base_tactics=unit.tactics=stat(43);
            unit.level=stat(18);if(!unit.level)unit.level=1;
            unit.allegiance_tag=bin_.at(p+15)&15;
            unit.persistent_role6=int(int8_t(bin_.at(p+21)));
            for(unsigned j=0;j<4;++j)unit.initial_status[j]=word(p+55+j*2);
            for(unsigned j=0;j<11;++j)unit.learned[j]=word(p+79+j*2);
            // 463914 locates the 37-byte ability records after the two-byte group directory.
            size_t ability_header=entry(2),ability_record=ability_header+9+learned_ability_count*2;
            unsigned ability_index=0;
            for(unsigned group=0;group<learned_ability_count;++group){
                unsigned count=bin_.at(ability_header+10+group*2);
                for(unsigned bit=0;bit<count;++bit,++ability_index){
                    if(!(unit.learned[group]&(1u<<(bit&31))))continue;
                    int threshold=int(int16_t(word(ability_record+size_t(ability_index)*37+15)));
                    if(int(unit.learned_proficiency[group]/10)<threshold)
                        unit.learned_proficiency[group]=unsigned(uint16_t(threshold*10));
                }
            }
            unit.abilities=dword(p+75);
            auto kw=[&](size_t q){return unsigned(kinds_.at(q))|(unsigned(kinds_.at(q+1))<<8);};
            size_t kind=kw(18)+(size_t(kw(20))<<16)+(unit.appearance-1)*37;
            unit.experience_threshold=kw(kind+14);
            unit.movement_type=int(int8_t(kinds_.at(kind+18)));
            unit.kind_attribute=stat(45)!=0?bin_.at(p+45):kinds_.at(kind+26);
            unit.base_action_points=stat(47)>0?stat(47):int(int8_t(kinds_.at(kind+21)));
            unit.action_points=unit.max_action_points=unit.base_action_points;
            for(size_t j=kind;j<kind+13&&kinds_.at(j);++j)unit.profession.push_back(kinds_.at(j));
            unit.portrait=unit.id>0&&unit.id<200?unit.id:kinds_.at(kind+13);
            unit.abilities|=kinds_.at(kind+19)?uint32_t(1) << ((kinds_.at(kind+19)-1)&31):0;
            auto iw=[&](size_t q){return unsigned(items_.at(q))|(unsigned(items_.at(q+1))<<8);};
            // 4447E5: six source items are distributed by item type, not source slot.
            for(unsigned j=0;j<6;++j){
                unsigned item=word(p+63+j*2);if(!item)continue;
                if(item>iw(0))throw std::runtime_error("Invalid original equipment ID");
                int type=int(int8_t(items_.at(2+(item-1)*38+14)));
                if(type>=1&&type<=6)unit.equipment[0]=item;
                else if(type>=7&&type<=11)unit.equipment[type-6]=item;
                else if(type!=12)throw std::runtime_error("Invalid original equipment type");
            }
            // 44CD68/44CDB7: preserve the original asymmetric cap operands.
            auto wrap=[](int v){return int(int16_t(uint16_t(v)));};
            for(unsigned item:unit.equipment){
                if(!item)continue;
                size_t q=2+(item-1)*38;
                int delta=int(int16_t(iw(q+17)));
                if(unit.attack+delta>999)delta=999-unit.attack;
                unit.attack=wrap(unit.attack+delta);unit.attack_limit=wrap(unit.attack_limit+delta);
                unit.equipment_stat_deltas[0]=wrap(unit.equipment_stat_deltas[0]+delta);
                delta=int(int8_t(items_.at(q+21)));
                if(unit.defense+delta>999)delta=999-unit.defense;
                unit.defense=wrap(unit.defense+delta);unit.defense_limit=wrap(unit.defense_limit+delta);
                unit.equipment_stat_deltas[1]=wrap(unit.equipment_stat_deltas[1]+delta);
                delta=int(int8_t(items_.at(q+22)));
                if(unit.sp+delta>999)delta=999-unit.sp;
                unit.sp=wrap(unit.sp+delta);unit.max_sp=wrap(unit.max_sp+delta);
                unit.equipment_stat_deltas[2]=wrap(unit.equipment_stat_deltas[2]+delta);
                delta=int(int8_t(items_.at(q+24)));
                if(unit.base_action_points+delta>110)delta=999-unit.base_action_points;
                unit.action_points=wrap(unit.action_points+delta);
                if(unit.action_points<0)unit.action_points=0;
                unit.max_action_points=wrap(unit.max_action_points+delta);
                if(unit.max_action_points<0)unit.max_action_points=0;
                unit.base_action_points=wrap(unit.base_action_points+delta);
                unit.equipment_stat_deltas[3]=wrap(unit.equipment_stat_deltas[3]+delta);
            }
            unit.behavior=int(int8_t(bin_.at(p+49)));
            unit.runtime_role_selector=unit.behavior==3||unit.behavior==9?1u:unit.behavior==8||unit.behavior==10?2u:0u;
            // 4274B5 partitions contexts at the first behavior 2/3/8, not at hostile().
            enemy_context=enemy_context||unit.behavior==2||unit.behavior==3||unit.behavior==8;
            if(bin_.at(header+21)!=3&&(enemy_context||unit.npc_class())){
                int factor=kinds_.at(kind+20);
                int minimum=proficiency_difficulty_scale_*int(difficulty)*factor/10+stat(18)*factor/20;
                if(minimum<0)minimum=0;else if(minimum>999)minimum=999;
                for(const auto& action:action_records(unit)){
                    int type=action.fields[0];
                    if(type>=2&&type<=7){auto& value=unit.weapon_proficiency[size_t(type-2)];if(value<unsigned(minimum))value=unsigned(minimum);}
                    else if((type<1||type>10)&&type!=11&&type!=12&&unit.special_proficiency<unsigned(minimum))unit.special_proficiency=unsigned(minimum);
                }
            }
            if(unit.behavior==4)unit.status_flags|=0x40;
            if(unit.behavior==7)unit.status_flags&=~0x40u;
            unit.runtime_flags30=unit.flags30();
            unit.direction=bin_.at(p+50)&3;unit.animation_action=bin_.at(p+50)>>2;
            unit.first_turn=int(int8_t(bin_.at(p+51)));unit.last_turn=int(int8_t(bin_.at(p+52)));
            for(size_t j=p+2;j<p+16&&bin_.at(j);++j)unit.name.push_back(bin_.at(j));
            units.push_back(std::move(unit));
        }
        size_t sections = characters + bin_.at(header+8)*101;
        auto section_text=[&](size_t p,unsigned n){std::vector<uint8_t> result;for(unsigned i=0;i<n&&bin_.at(p+i);++i)result.push_back(bin_.at(p+i));return result;};
        stage_name=section_text(sections+14,word(sections));
        size_t victory=sections+14+word(sections)+word(sections+2);
        victory_objective=section_text(victory,word(sections+4));
        size_t defeat=victory+word(sections+4)+2*(word(sections+8)+word(sections+10));
        defeat_objective=section_text(defeat,word(sections+6));
        briefing.clear();
        size_t text=sections+14+word(sections);
        for(size_t i=0;i<word(sections+2)&&bin_.at(text+i);++i)briefing.push_back(bin_.at(text+i));
        events_ = sections + 14;
        for (unsigned i : {0,2,4,6}) events_ += word(sections+i);
        for (unsigned i : {8,10,12}) events_ += 2*word(sections+i);
        if (events_+4 > end || events_+4+event_count()*20 > end || events_+4+word(events_+2) > end)
            throw std::runtime_error("Invalid original stage event table");
        stage = id;
    }
    void restore_unit_resources(Unit& unit) const {
        auto kw=[&](size_t p){return unsigned(kinds_.at(p))|(unsigned(kinds_.at(p+1))<<8);};
        if(!unit.appearance||unit.appearance>kw(22))throw std::runtime_error("Invalid saved unit appearance");
        size_t kind=kw(18)+(size_t(kw(20))<<16)+size_t(unit.appearance-1)*37;
        unit.movement_type=int(int8_t(kinds_.at(kind+18)));
        unit.profession.clear();
        for(size_t p=kind;p<kind+13&&kinds_.at(p);++p)unit.profession.push_back(kinds_.at(p));
        unit.portrait=unit.id>0&&unit.id<200?unit.id:kinds_.at(kind+13);
    }
    size_t event_count() const { return word(events_); }
    std::vector<uint16_t> save_event_states() const {
        std::vector<uint16_t> result;
        for(size_t i=0;i<event_count();++i)result.push_back(uint16_t(int16_t(int8_t(bin_.at(record(i))))));
        return result;
    }
    void restore_event_states(std::span<const uint16_t> states) {
        if(states.size()!=event_count())throw std::runtime_error("Original save event count does not match stage");
        for(auto state:states)if(uint16_t(int16_t(int8_t(state&255)))!=state)
            throw std::runtime_error("Invalid original saved event byte");
        for(size_t i=0;i<states.size();++i)bin_.at(record(i))=uint8_t(states[i]);
    }
    int event_word(size_t event,unsigned offset)const{
        if(offset!=1&&offset!=3&&offset!=5&&offset!=7&&offset!=9)throw std::runtime_error("Invalid original event operand");
        return int(int16_t(word(record(event)+offset)));
    }
    void clear_stage_flags(){std::fill_n(flags.begin(),40,uint8_t(0));}
    unsigned completion_flag(size_t event)const{return bin_.at(record(event)+11);}
    unsigned type(size_t event) const { return bin_.at(record(event)); }
    unsigned turn(size_t event) const { return word(record(event)+3); }
    unsigned flag(size_t event) const { return word(record(event)+3); }
    unsigned condition(size_t event) const { return word(record(event)+1); }
    bool flag_trigger(size_t event)const{unsigned f=flag(event);return type(event)==5&&f>=1&&f<=80&&flags[f-1]!=0;}
    void choose(size_t event,unsigned selected){
        if(!flag_trigger(event)||condition(event)!=0||selected<1||selected>=line_count(event))throw std::runtime_error("Invalid original event choice");
        flags[flag(event)-1]=uint8_t(selected);
        complete(event);
    }
    bool branch_trigger(size_t event)const{unsigned f=flag(event);return type(event)==5&&condition(event)>=1&&condition(event)<=4&&f>=1&&f<=80&&flags[f-1]==condition(event);}
    void consume_branch(size_t event){flags.at(flag(event)-1)=0;}
    bool flag_dialogue_trigger(size_t event)const{unsigned f=flag(event);return type(event)==9&&condition(event)==0&&f>=1&&f<=80&&flags[f-1]>0;}
    unsigned line_count(size_t event) const { return word(record(event)+18); }
    bool turn_trigger(size_t event, unsigned current_turn) const {
        size_t p = record(event);
        return type(event)==1 && word(p+1)==0 && turn(event)==current_turn;
    }
    Line line(size_t event, size_t index) const {
        if (index >= line_count(event)) throw std::runtime_error("Invalid original dialogue line");
        size_t start = events_ + dword(record(event)+12);
        size_t p = start + index*23;
        Line result{int(int16_t(word(p))), {}, {}};
        for (size_t i=p+6;i<p+19 && bin_.at(i);++i) result.name.push_back(bin_.at(i));
        size_t text = start + line_count(event)*23 + word(p+19);
        size_t length = word(p+21);
        if (length==0 || bin_.at(text+length-1)!=0 || text+length>events_+4+word(events_+2))
            throw std::runtime_error("Invalid original dialogue text range");
        result.text.assign(bin_.begin()+text, bin_.begin()+text+length-1);
        return result;
    }
    void complete(size_t event) {
        size_t p=record(event);
        bin_.at(p)=0;
        unsigned flag=bin_.at(p+11);
        if (flag>=1 && flag<=80) flags[flag-1]=1;
    }
};
}
