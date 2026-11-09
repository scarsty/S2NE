#pragma once
#include "original_save_bridge.hpp"
#include "original_campaign.hpp"
#include "original_promotion_transaction.hpp"

namespace s2ne {
// Original mode0 initialization and runtime arrivals, with caller-owned opening/settings state.
class OriginalInitialization {
    std::vector<uint8_t> bin_,pal_,items_,prg_;
    static std::vector<uint8_t> read(const std::filesystem::path& p){
        std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing original initialization resource");
        return {std::istreambuf_iterator<char>(f),{}};
    }
    static unsigned get(const std::vector<uint8_t>& b,size_t p,unsigned n=2){
        unsigned v=0;for(unsigned i=0;i<n;++i)v|=unsigned(b.at(p+i))<<(8*i);return v;
    }
    size_t entry(unsigned id)const{return get(pal_,848+(id-1)*8,4)-11-(id-1);}
    std::vector<uint8_t> text(unsigned id)const{
        size_t p=entry(5);unsigned count=get(bin_,p);if(id>=count)throw std::runtime_error("Invalid original save label string");
        p+=2+count*8+get(bin_,p+2+id*8,4);std::vector<uint8_t> result;while(bin_.at(p))result.push_back(bin_.at(p++));return result;
    }
    void describe(OriginalSave::State& raw,const OriginalEvents& events)const{
        auto label=text(144),turn=text(1);std::vector<uint8_t> bytes=label;bytes.push_back(' ');
        auto number=std::to_string(OriginalSaveBridge::get(raw.stage_4fb478,8,4));bytes.insert(bytes.end(),number.begin(),number.end());bytes.push_back('.');
        if(events.stage_name.size()<=40)bytes.insert(bytes.end(),events.stage_name.begin(),events.stage_name.end());
        while(bytes.size()<25)bytes.push_back(' ');
        bool substituted=false;for(size_t i=0;i<turn.size();++i){        if(turn[i]=='%'&&i+2<turn.size()&&turn[i+1]=='2'&&turn[i+2]=='d'){
                    auto n=std::to_string(OriginalSaveBridge::get(raw.globals_4f7cd0,12,4));if(n.size()<2)bytes.push_back(' ');bytes.insert(bytes.end(),n.begin(),n.end());i+=2;substituted=true;
        }else bytes.push_back(turn[i]);}
        if(!substituted||bytes.size()>=raw.description.size())throw std::runtime_error("Original save label exceeds50 bytes");
        raw.description.fill(0);std::copy(bytes.begin(),bytes.end(),raw.description.begin());
        constexpr char attribution[]="The SuperSp Game V1.3d Copyright (c) 1997 by U&J TECHNOLOGY CO., LTD.";
        raw.attribution.fill(0);std::copy_n(attribution,sizeof(attribution),raw.attribution.begin());
    }
public:
    struct Source {
        std::array<uint8_t,101> bytes{};
        unsigned group{};
        unsigned id()const{return bytes[0]|(unsigned(bytes[1])<<8);}
        int behavior()const{return int(int8_t(bytes[49]));}
        int first_turn()const{return int(int8_t(bytes[51]));}
        int last_turn()const{return int(int8_t(bytes[52]));}
        bool arrives(unsigned turn)const{
            if(!turn||turn>0x7FFFFFFF)throw std::runtime_error("Invalid original arrival turn");
            return last_turn()==0?first_turn()==int(turn):first_turn()<=int(turn)&&last_turn()>=int(turn);
        }
        int control_class()const{int b=behavior();return b==0||b==5||b==6||b==7?0:1;}
        int role6()const{return int(int8_t(bytes[21]));}
        int role_a()const{return int(int8_t(bytes[20]));}
        int role_c()const{return int(int8_t(bytes[22]));}
    };
    struct Opening {unsigned primary_stage,alternate_stage,opening_stage;int currency_a,currency_b;};
    struct PlacementMap {
        unsigned width{},height{};
        std::vector<uint8_t> elevations;
        std::vector<std::pair<int,int>> sealed;
        bool accepts(int x,int y)const{
            if(x<0||y<0||(!(y&1)&&x<1)||x>=int(width)-1||y>=int(height)-1)return false;
            int elevation=elevations.at(size_t(y)*width+unsigned(x));
            return y-(elevation>0?elevation-1:0)>=1&&std::find(sealed.begin(),sealed.end(),std::pair{x,y})==sealed.end();
        }
    };
    PlacementMap placement_map(const std::filesystem::path& root,unsigned map)const{
        auto prg=read(root/"SUPERSP.PRG");size_t table=get(pal_,808+8*(64+6-1),4)-15;
        if(!map||map>get(prg,table))throw std::runtime_error("Invalid original placement map");
        size_t m=table+get(prg,table+2+(map-1)*8,4);PlacementMap result;
        result.width=get(prg,m);result.height=get(prg,m+2)*2;
        if(!result.width||!result.height)throw std::runtime_error("Invalid original placement dimensions");
        for(size_t i=0;i<size_t(result.width)*result.height;++i)result.elevations.push_back(prg.at(m+28+i*5));
        size_t special=m+28+result.elevations.size()*5+get(prg,m+12)*10;
        for(unsigned i=0;i<get(prg,m+14);++i){size_t p=special+i*6;if(get(prg,p+4)&0x8000)result.sealed.emplace_back(int(int16_t(get(prg,p))),int(int16_t(get(prg,p+2))));}
        return result;
    }
    static std::pair<int,int> place(int x,int y,const PlacementMap& map,std::span<const OriginalSave::UnitRecord> first,std::span<const OriginalSave::UnitRecord> second){
        auto occupied=[&](int cx,int cy){for(auto army:{first,second})for(const auto& r:army){unsigned status=r[0x33];
            if(!(status&0x80)||((status&0x20)&&!(status&2)))continue;
            if(int(int32_t(OriginalSaveBridge::get(r,0x23,4)))==cx&&int(int32_t(OriginalSaveBridge::get(r,0x27,4)))==cy)return true;
        }return false;};
        if(!occupied(x,y))return {x,y};
        auto accepts=[&](int cx,int cy){return map.accepts(cx,cy)&&!occupied(cx,cy);};
        int left=x,top=y;
        for(int radius=1;radius<=10;++radius){if(!(top&1))--left;--top;
            for(int cx=left;cx<=left+radius;++cx){if(accepts(cx,top))return {cx,top};if(accepts(cx,top+2*radius))return {cx,top+2*radius};}
            for(int cy=top+2;cy<=top+2*radius-2;cy+=2){if(accepts(left,cy))return {left,cy};if(accepts(left+radius,cy))return {left+radius,cy};}
        }return {x,y};
    }
    explicit OriginalInitialization(const std::filesystem::path& root):bin_(read(root/"SUPERSP.BIN")),pal_(read(root/"SUPERSP.PAL")),items_(read(root/"ITEMSP.BIN")),prg_(read(root/"SUPERSP.PRG")){
        OriginalSave::item_count(items_);
    }
    Opening opening()const{
        size_t p=entry(10);p+=2+4*get(bin_,p);p+=2+get(bin_,p);p+=2;
        return {bin_.at(p+0xFE),bin_.at(p+0xFF),bin_.at(p+0x100),int(int8_t(bin_.at(p+0xF9))),int(int8_t(bin_.at(p+0xFA)))};
    }
    // 44367C/444276: inventory memset, status memset then 44382B's eleven triples.
    OriginalSave::Group empty_group()const{
        OriginalSave::Group g;g.inventory.resize(OriginalSave::item_count(items_));
        for(unsigned i=0;i<11;++i)g.status[1+i*3]=uint8_t(113+i);
        return g;
    }
    std::vector<Source> sources(unsigned stage)const{
        size_t table=entry(1);if(!stage||stage>get(bin_,table))throw std::runtime_error("Invalid original initialization stage");
        size_t h=table+2+(stage-1)*30,data=table+get(bin_,h,4),p=data+get(bin_,h+26,4);
        size_t end=data+get(bin_,h+4,4),count=bin_.at(h+8);
        if(end>bin_.size()||p>end||count*101>end-p)throw std::runtime_error("Truncated original initialization roster");
        std::vector<Source> out;bool enemy=false;
        for(size_t i=0;i<count;++i){Source s;std::copy_n(bin_.begin()+p+i*101,101,s.bytes.begin());
            enemy=enemy||s.behavior()==2||s.behavior()==3||s.behavior()==8;s.group=enemy?1:0;out.push_back(s);
        }return out;
    }
    OriginalSave::UnitRecord new_role(const Source& source,const OriginalEvents::Unit& numeric,unsigned index,const PlacementMap& map,
                                      std::span<const OriginalSave::UnitRecord> first,std::span<const OriginalSave::UnitRecord> second)const{
        if(!index||index>50||numeric.id!=source.id()||numeric.appearance!=(unsigned(source.bytes[16])|(unsigned(source.bytes[17])<<8)))throw std::runtime_error("Original source/runtime role mismatch");
        OriginalSave::UnitRecord raw{}; // 443C3E memset362, before4443B3's assignments.
        auto put=[&](size_t p,unsigned v,unsigned n=2){OriginalSaveBridge::put(raw,p,v,n);};
        put(0,index);put(2,numeric.id);put(4,numeric.appearance);put(6,uint16_t(source.role6()));put(8,source.control_class());
        put(0xA,uint16_t(source.role_a()));put(0xC,uint16_t(source.role_c()));
        raw[0x2F]=source.behavior()==3||source.behavior()==9?1:source.behavior()==8||source.behavior()==10?2:0;
        put(0x119,unsigned(source.bytes[53])|(unsigned(source.bytes[54])<<8));
        OriginalSaveBridge::patch_unit(raw,numeric);
        int x=int(int16_t(unsigned(source.bytes[23])|(unsigned(source.bytes[24])<<8))),y=int(int16_t(unsigned(source.bytes[25])|(unsigned(source.bytes[26])<<8)));
        auto position=place(x,y,map,first,second);put(0x23,uint32_t(position.first),4);put(0x27,uint32_t(position.second),4);
        size_t index_table=get(pal_,808+8*64,4)-10;unsigned count=get(prg_,index_table);
        if(!numeric.appearance||numeric.appearance>count)throw std::runtime_error("Invalid original class frame selector");
        size_t sequences=index_table+2+count*8+(numeric.appearance-1)*200;
        unsigned selector=uint8_t(uint32_t(position.second)>>24);size_t offset=0;
        for(unsigned i=0;i<selector;++i){while(offset<200&&prg_.at(sequences+offset))++offset;if(++offset>=200)throw std::runtime_error("Original frame selector out of range");}
        raw[0x2C]=uint8_t(offset);
        unsigned first_frame=get(prg_,index_table+2+(numeric.appearance-1)*8+2);
        unsigned frame=prg_.at(sequences+offset);if(!frame)throw std::runtime_error("Empty original initial class frame");
        size_t frame_table=get(pal_,808+8*65,4)-11;
        raw[0xAC]=prg_.at(frame_table+2+(first_frame+frame-1)*7+6);
        return raw;
    }
    // 427645 inserts only this turn's arrivals. The caller supplies unchanged source
    // numeric expansion, and publishes the returned copy together with live caches.
    OriginalSave::State insert_arrivals(const OriginalSave::State& before,const OriginalEvents& expanded,unsigned turn,const PlacementMap& map,bool enemies_only=false,bool transition_allies=false)const{
        if(OriginalSaveBridge::get(before.globals_4f7cd0,8,4)!=expanded.stage)throw std::runtime_error("Arrival stage mismatch");
        auto source=sources(expanded.stage);if(source.size()!=expanded.units.size())throw std::runtime_error("Arrival requires complete source expansion");
        auto staged=before;
        for(unsigned g=enemies_only?1:0;g<(transition_allies?1u:2u);++g)for(size_t i=0;i<source.size();++i){const auto& s=source[i];if(s.group!=g||!s.arrives(turn)||(transition_allies&&g==0&&s.behavior()==0))continue;
            auto& army=staged.groups[g];auto existing=army.units.end();
            if(s.id())existing=std::find_if(army.units.begin(),army.units.end(),[&](const auto& r){return OriginalSaveBridge::get(r,2)==s.id();});
            if(existing!=army.units.end()){
                if(!((*existing)[0x33]&0x20)){
                    int x=int(int16_t(unsigned(s.bytes[23])|(unsigned(s.bytes[24])<<8))),y=int(int16_t(unsigned(s.bytes[25])|(unsigned(s.bytes[26])<<8)));
                    OriginalSaveBridge::put(*existing,0x23,uint32_t(x),4);OriginalSaveBridge::put(*existing,0x27,uint32_t(y),4);
                    auto position=place(x,y,map,staged.groups[0].units,staged.groups[1].units);
                    OriginalSaveBridge::put(*existing,0x23,uint32_t(position.first),4);OriginalSaveBridge::put(*existing,0x27,uint32_t(position.second),4);
                    (*existing)[0x33]|=0x80;(*existing)[0x2B]=s.bytes[50];if(s.behavior()==7)(*existing)[0x33]&=0xBF;
                }continue;
            }
            if(army.units.size()>=50)throw std::runtime_error("Original arrival army capacity exceeded");
            auto raw=new_role(s,expanded.units[i],unsigned(army.units.size()+1),map,staged.groups[0].units,staged.groups[1].units);
            grant_source_consumables(army,s);army.units.push_back(raw);
        }
        return staged;
    }
    // 42012F condition0: create/reuse matching stage SOURCE roles, not transfer
    // an arbitrary live enemy or grant a replacement with invented statistics.
    OriginalSave::State insert_scripted_roles(const OriginalSave::State& before,const OriginalEvents& expanded,size_t event,const PlacementMap& map)const{
        if(expanded.type(event)==12&&expanded.condition(event)==0)return reset_scripted_role(before,expanded,event);
        if(expanded.type(event)==12&&(expanded.condition(event)==22||expanded.condition(event)==1))return mutate_scripted_control(before,expanded,event);
        if(expanded.type(event)!=6||expanded.condition(event)!=0)throw std::runtime_error("Unsupported original scripted role operation stage "+std::to_string(expanded.stage)+" event "+std::to_string(event)+" opcode "+std::to_string(expanded.type(event))+" condition "+std::to_string(expanded.condition(event)));
        if(OriginalSaveBridge::get(before.globals_4f7cd0,8,4)!=expanded.stage)throw std::runtime_error("Scripted role stage mismatch");
        auto source=sources(expanded.stage);if(source.size()!=expanded.units.size())throw std::runtime_error("Scripted roles require complete source expansion");
        auto staged=before;
        for(size_t i=0;i<source.size();++i){const auto& s=source[i];bool match=false;
            for(unsigned offset:{5u,7u,9u})if(expanded.event_word(event,offset)>0&&s.id()==unsigned(expanded.event_word(event,offset)))match=true;
            if(!match)continue;
            auto& army=staged.groups[s.group];
            auto existing=std::find_if(army.units.begin(),army.units.end(),[&](const auto& r){return OriginalSaveBridge::get(r,2)==s.id();});
            if(existing!=army.units.end()){
                if((*existing)[0x33]&0x20)continue;
                int x=int(int16_t(unsigned(s.bytes[23])|(unsigned(s.bytes[24])<<8))),y=int(int16_t(unsigned(s.bytes[25])|(unsigned(s.bytes[26])<<8)));
                auto position=place(x,y,map,staged.groups[0].units,staged.groups[1].units);
                OriginalSaveBridge::put(*existing,0x23,uint32_t(position.first),4);OriginalSaveBridge::put(*existing,0x27,uint32_t(position.second),4);(*existing)[0x33]|=0x80;
            }else{
                if(army.units.size()>=50)throw std::runtime_error("Original scripted army capacity exceeded");
                auto role=new_role(s,expanded.units[i],unsigned(army.units.size()+1),map,staged.groups[0].units,staged.groups[1].units);
                grant_source_consumables(army,s);army.units.push_back(role);
            }
        }
        return staged;
    }
    // 461535 changes control words in-place, without army transfer or activation.
    // 461236 condition1 withdraws the role (40h on,80h off), not reverse control.
    OriginalSave::State mutate_scripted_control(const OriginalSave::State& before,const OriginalEvents& current,size_t event)const{
        int condition=current.condition(event);
        if(current.type(event)!=12||(condition!=22&&condition!=1)||OriginalSaveBridge::get(before.globals_4f7cd0,8,4)!=current.stage)throw std::runtime_error("Invalid original control mutation");
        auto staged=before;unsigned id=unsigned(current.event_word(event,5));
        for(auto& group:staged.groups)for(auto& role:group.units)if(OriginalSaveBridge::get(role,2)==id){
            if(condition>=10&&((role[0x33]&0x20)||!(role[0x33]&0x80)))return staged;
            if(condition==22){OriginalSaveBridge::put(role,8,0);OriginalSaveBridge::put(role,10,1);OriginalSaveBridge::put(role,12,0);role[0x33]&=0xBF;}
            else role[0x33]=uint8_t((role[0x33]|0x40)&0x7F);
            return staged;
        }
        throw std::runtime_error("Original461110 control mutation role missing");
    }
    // 461215 removes identity from its fixed army slot, not from the vector.
    OriginalSave::State reset_scripted_role(const OriginalSave::State& before,const OriginalEvents& current,size_t event)const{
        if(current.type(event)!=12||current.condition(event)!=0||OriginalSaveBridge::get(before.globals_4f7cd0,8,4)!=current.stage)throw std::runtime_error("Invalid original role reset transaction");
        auto staged=before;unsigned id=unsigned(current.event_word(event,5));
        for(auto& group:staged.groups)for(auto& role:group.units)if(OriginalSaveBridge::get(role,2)==id){
            OriginalSaveBridge::put(role,0x30,0,4);OriginalSaveBridge::put(role,8,1);OriginalSaveBridge::put(role,2,0);return staged;
        }
        throw std::runtime_error("Original461215 role missing");
    }
    struct Setup {
        // Original4FB488 is DISC.ID selection (0 disc1/1 disc2), NOT renderer backend.
        uint32_t display_backend;
        uint32_t screen_delay{},speed{10},music{100},effects{100},speech{100};
        std::array<uint8_t,11> switches{1,0,0,1,1,1,1,0,0,0,0};
    };
    template<class Settings> static Setup setup_from_settings(const Settings& state,uint32_t disc_selection){
        if(disc_selection>1)throw std::runtime_error("Original first-stage disc selection must be0/1");
        Setup setup{disc_selection};setup.screen_delay=uint32_t(state.screen_delay);setup.speed=uint32_t(state.message_delay);
        setup.music=uint32_t(state.midi_volume);setup.effects=uint32_t(state.cdda_volume);setup.speech=uint32_t(state.effect_volume);
        setup.switches={state.masking,state.flag_4f7d05,state.animation,state.status_window,state.cdda,state.flag_4f7d09,state.flag_4f7d0a,0,0,0,0};
        return setup;
    }
    // Boundary: mode0 first battle after427870 arrivals, before first turn dialogue.
    // The supplied flags are the actual completed opening140 snapshot, not a choice guess.
    OriginalSave::State fresh(const std::filesystem::path& root,unsigned stage,unsigned difficulty,
                             const std::array<uint8_t,80>& opening_flags,const Setup& setup)const{
        auto config=opening();if(stage!=(opening_flags[40]?config.alternate_stage:config.primary_stage))throw std::runtime_error("Fresh stage does not match original opening flag40");
        OriginalEvents expanded(root,difficulty);expanded.flags=opening_flags;expanded.clear_stage_flags();expanded.load_stage(stage);
        OriginalSave::State raw;raw.flags_4f7d10=expanded.flags;raw.event_states=expanded.save_event_states();
        auto put=[](auto& b,size_t p,uint32_t v,unsigned n=4){OriginalSaveBridge::put(b,p,v,n);};
        put(raw.settings_4f7cf0,0,setup.screen_delay);put(raw.settings_4f7cf0,4,setup.speed);put(raw.settings_4f7cf0,8,setup.music);put(raw.settings_4f7cf0,12,setup.effects);put(raw.settings_4f7cf0,16,setup.speech);
        std::copy(setup.switches.begin(),setup.switches.end(),raw.settings_4f7cf0.begin()+20);
        size_t table=entry(1),h=table+2+(stage-1)*30,data=table+get(bin_,h,4);
        put(raw.globals_4f7cd0,8,stage);put(raw.globals_4f7cd0,12,1);put(raw.globals_4f7cd0,16,bin_.at(h+12));put(raw.globals_4f7cd0,20,bin_.at(h+16));
        put(raw.globals_4f7cd0,24,stage,2);raw.globals_4f7cd0[28]=uint8_t(difficulty);
        put(raw.settings_4f7cf0,27,bin_.at(h+10));
        put(raw.stage_4fb478,0,uint32_t(config.currency_a));put(raw.stage_4fb478,4,uint32_t(config.currency_b));
        put(raw.stage_4fb478,8,1);put(raw.stage_4fb478,12,opening_flags[40]?1:0);put(raw.stage_4fb478,16,setup.display_backend);
        unsigned sections=bin_.at(h+25);size_t p=data+sections*4;
        for(unsigned i=0;i<sections;++i){unsigned type=get(bin_,data+i*4),count=get(bin_,data+i*4+2);
            if(type==5){if(count%4||count>80)throw std::runtime_error("Invalid original dynamic chains");put(raw.value_4f705c,0,count/4);
                for(size_t j=0;j<count*2;++j)raw.words_4f6fbc[j]=bin_.at(p+j);break;}
            p+=count*2;
        }
        raw.groups[0]=empty_group();raw.groups[1]=empty_group();
        auto map=placement_map(root,expanded.map_id);raw=insert_arrivals(raw,expanded,1,map);
        describe(raw,expanded);
        return raw;
    }
    OriginalSave::State transition_stage(const std::filesystem::path& root,const OriginalSave::State& settled,const OriginalEvents& deployed)const{
        unsigned previous=OriginalSaveBridge::get(settled.globals_4f7cd0,8,4);size_t table=entry(1);
        if(!previous||previous>get(bin_,table)||!deployed.stage||deployed.stage>get(bin_,table)||deployed.difficulty!=settled.globals_4f7cd0[28]||OriginalSaveBridge::get(settled.globals_4f7cd0,0,4)!=0)throw std::runtime_error("Invalid original campaign transition");
        size_t prior=table+2+(previous-1)*30;
        unsigned direct=bin_.at(prior+12),next=bin_.at(prior+16);
        bool authorized=deployed.stage==direct||deployed.stage==next;
        {
            OriginalEvents original(root,deployed.difficulty);original.load_stage(previous);
            auto completed=OriginalSaveBridge::restore(root,settled).events;
            unsigned selected=OriginalCampaign::completed_story_route(original,completed);
            if(selected)authorized=selected==deployed.stage;
        }
        if(bin_.at(prior+21)==3){
            OriginalEvents original(root,deployed.difficulty);original.load_stage(previous);
            auto completed=OriginalSaveBridge::restore(root,settled).events;
            authorized=OriginalCampaign::completed_story_route(original,completed,direct)==deployed.stage;
        }
        if(!authorized)throw std::runtime_error("Transition not authorized by original stage header/event");
        unsigned target_mode=bin_.at(table+2+(deployed.stage-1)*30+21);
        if(target_mode!=1&&target_mode!=3)throw std::runtime_error("Original intermediary stage requires separate scripted entry");
        OriginalEvents source(root,deployed.difficulty);source.flags=deployed.flags;source.load_stage(deployed.stage);
        auto raw=settled;
        if(target_mode!=3){
            auto& army=raw.groups[0];std::vector<OriginalSave::UnitRecord> retained;
            for(auto role:army.units){unsigned flags=role[0x33];
                if(OriginalSaveBridge::get(role,8)==1&&((flags&0x20)||!(flags&0x40)))continue;
                if(flags&0x10)throw std::runtime_error("Original44A4A9 alternate appearance retention requires army status lookup");
                retained.push_back(role);
            }
            army.units=std::move(retained);
        }
        raw.groups[1]=empty_group();raw.flags_4f7d10=deployed.flags;raw.event_states=deployed.save_event_states();
        size_t h=table+2+(deployed.stage-1)*30,data=table+get(bin_,h,4);
        OriginalSaveBridge::put(raw.globals_4f7cd0,8,deployed.stage,4);OriginalSaveBridge::put(raw.globals_4f7cd0,12,1,4);
        OriginalSaveBridge::put(raw.globals_4f7cd0,16,bin_.at(h+12),4);OriginalSaveBridge::put(raw.globals_4f7cd0,20,bin_.at(h+16),4);OriginalSaveBridge::put(raw.globals_4f7cd0,24,deployed.stage,2);raw.globals_4f7cd0[26]=0;
        unsigned branch=OriginalSaveBridge::get(raw.stage_4fb478,12,4),progress=OriginalSaveBridge::get(raw.stage_4fb478,8,4);
        unsigned disc=(branch==1&&progress>=7)||(branch==0&&(direct==60||direct==61))?1:0;
        OriginalSaveBridge::put(raw.stage_4fb478,16,disc,4);
        OriginalSaveBridge::put(raw.stage_4fb478,8,OriginalSaveBridge::get(raw.stage_4fb478,8,4)+1,4);
        OriginalSaveBridge::put(raw.settings_4f7cf0,27,bin_.at(h+10),4);
        raw.value_4f705c.fill(0);raw.words_4f6fbc.fill(0);
        unsigned sections=bin_.at(h+25);size_t p=data+sections*4;
        for(unsigned i=0;i<sections;++i){unsigned type=get(bin_,data+i*4),count=get(bin_,data+i*4+2);if(type==5){if(count%4||count>80)throw std::runtime_error("Invalid original transition chains");OriginalSaveBridge::put(raw.value_4f705c,0,count/4,4);for(size_t j=0;j<count*2;++j)raw.words_4f6fbc[j]=bin_.at(p+j);break;}p+=count*2;}
        if(target_mode==3){
            for(size_t i=0;i<raw.groups[0].units.size();++i)OriginalSaveBridge::patch_unit(raw.groups[0].units[i],deployed.units.at(i));
            // 40D7D4 ->427870 ->427645 creates this story turn's source actors.
            return insert_arrivals(raw,source,1,placement_map(root,source.map_id));
        }
        if(deployed.units.size()<raw.groups[0].units.size())throw std::runtime_error("Missing persistent deployment roster");
        for(size_t i=0;i<raw.groups[0].units.size();++i){auto& r=raw.groups[0].units[i];
            unsigned original_index=OriginalSaveBridge::get(r,0);if(!original_index||original_index>deployed.units.size())throw std::runtime_error("Missing retained original deployment slot");
                        const auto& u=deployed.units[original_index-1];OriginalSaveBridge::put(r,0,unsigned(i+1));
            if(OriginalSaveBridge::get(r,2)!=u.id||OriginalSaveBridge::get(r,4)!=u.appearance)throw std::runtime_error("Changed original deployment identity");
            std::fill(r.begin()+0x34,r.begin()+0x40,uint8_t(0));std::fill(r.begin()+0x124,r.begin()+0x13A,uint8_t(0));OriginalSaveBridge::patch_unit(r,u);
        }
        // 40FEE9: controllable slots were consumed by462224; nonzero-behavior
        // allied source arrivals still pass through443BB1 after enemy creation.
        auto map=placement_map(root,source.map_id);
        auto arrivals=insert_arrivals(raw,source,1,map,true);
        arrivals=insert_arrivals(arrivals,source,1,map,false,true);
        describe(arrivals,deployed);return arrivals;
    }
    OriginalSave::State transition_stage2(const std::filesystem::path& root,const OriginalSave::State& settled,const OriginalEvents& deployed)const{
        if(OriginalSaveBridge::get(settled.globals_4f7cd0,8,4)!=1||deployed.stage!=2)throw std::runtime_error("Stage2 compatibility entry requires1->2");
        return transition_stage(root,settled,deployed);
    }
    // Call only when 4443B3 creates a NEW role, never for a 444068 reused role.
    void grant_source_consumables(OriginalSave::Group& group,const Source& source)const{
        if(group.inventory.size()!=OriginalSave::item_count(items_))throw std::runtime_error("Complete original inventory required");
        auto staged=group.inventory;
        for(unsigned i=0;i<6;++i){unsigned item=source.bytes[63+i*2]|(unsigned(source.bytes[64+i*2])<<8);if(!item)continue;
            if(item>staged.size())throw std::runtime_error("Invalid original source item");
            int type=int(int8_t(items_.at(2+(item-1)*38+14)));
            if(type<1||type>12)throw std::runtime_error("Invalid original source item type");
            if(type==12)++staged[item-1];
        }group.inventory.swap(staged);
    }
};
}
