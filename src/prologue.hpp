#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_map.hpp"
#include "battle.hpp"
#include "original_palette.hpp"
#include "original_unit_status.hpp"
#include "original_battle_captions.hpp"
#include "original_confirmation.hpp"
#include "original_ui_timing.hpp"
#include "original_progression.hpp"
#include "original_campaign.hpp"
#include "original_script_predicates.hpp"
#include "original_interbattle.hpp"
#include "original_scripted_movement.hpp"
#include <deque>
#include <map>
#include <optional>

namespace s2ne {
class Prologue {
    std::vector<uint8_t> cel_,pal_,caption_items_;
    mutable std::optional<OriginalPalette> palette_effects_;
    OriginalUnitStatus unit_status_;
    OriginalMap map_;
    std::filesystem::path root_;
    OriginalDisc disc_{OriginalDisc::one};
    std::optional<Battle> battle_;
    void connect_combat_progression(){
        battle_->set_death_settlement([this](auto& units,size_t actor,size_t target){queue_original_death(units,actor,target);});
        battle_->set_physical_settlement([this](auto& units,const Battle::PhysicalSettlement& result){
            OriginalProgression progression(root_);OriginalPromotion promotion(root_);
            if(result.actor>=units.size())throw std::runtime_error("Combat XP actor out of range");
            bool applied=battle_->mutate_random([&](OriginalRandom& random,uint32_t seconds){
                return progression.award_native(units[result.actor],random,result.raw_xp,promotion,[seconds]{return seconds;}).applied;
            });
            if(!applied&&!units[result.actor].hostile())throw std::runtime_error("Original combat progression rejected");
        });
    }
    std::optional<OriginalConfirmation> finish_confirmation_;
    std::optional<unsigned> finish_direction_;
    enum class Phase { dialogue, choice, branch, flag_dialogue, briefing, battle_intro, objectives, battle, hostile_actions, automatic_actions, npc_actions, story_animation, story_movement, story_disappearance, story_wait, story_ready, campaign_dialogue, campaign_rewards, campaign_defeat, interbattle, campaign_blocked };
    std::optional<OriginalCampaign> campaign_;
    std::optional<OriginalInterbattle> interbattle_;
    std::deque<size_t> script_queue_;
    unsigned arrivals_turn_{};
public:
    struct ArrivalTransaction {
        OriginalEvents events;
        // Must be noexcept: all fallible resource preparation precedes publication.
        std::function<void()> commit;
    };
private:
    std::function<std::optional<ArrivalTransaction>(const OriginalEvents&,unsigned)> campaign_arrival_adapter_;
    std::function<std::optional<ArrivalTransaction>(const OriginalEvents&,size_t)> campaign_scripted_roles_adapter_;
    Phase script_return_{Phase::battle};
    std::optional<OriginalCampaign::Decision> campaign_decision_;
    std::vector<OriginalCampaign::SaveRequest> autosave_requests_;
    std::string campaign_blocker_;
    std::function<bool(OriginalEvents&,const OriginalCampaign::Settlement&)> campaign_reward_adapter_;
    std::function<OriginalInterbattle::Result(OriginalInterbattle::Request)> interbattle_adapter_;
    bool campaign_settled_{};
    unsigned intro_elapsed_=0;
    bool presentation_driven_=false;
    OriginalBannerPresentation banner_presentation_;
    unsigned animation_elapsed_=0;
    std::optional<std::pair<int,int>> dialogue_camera_target_;
    std::pair<int,int> dialogue_camera_subject_{};
    unsigned dialogue_visible_steps_=0;
    bool first_banner_=true;
    mutable int status_panel_side_=1;
    // Nine original poses, including the settled pose, must reach the renderer.
    // 4389A9 passes edx=1 to 473111: one centisecond minimum per presented pose.
    static constexpr unsigned banner_pose_duration_=10;
    static constexpr unsigned banner_duration_=9*banner_pose_duration_;
    struct Banner { int y=0,turn_x=0,number_x=620,strip=1;bool done=false; };
    static Banner banner(unsigned frame){
        Banner b;
        while(frame--&&!b.done){
            bool y_done=b.y==159,x_done=b.turn_x==249;
            if(y_done&&x_done){b.done=true;break;}
            b.y=std::min(159,b.y<100?b.y+24:b.y<140?b.y+16:b.y<150?b.y+8:159);
            b.turn_x=std::min(249,b.turn_x<200?b.turn_x+40:b.turn_x<230?b.turn_x+28:249);
            b.number_x=std::max(358,b.number_x>439?b.number_x-40:b.number_x>386?b.number_x-28:358);
            b.strip=std::min(320,b.strip<260?b.strip+60:b.strip+30);
        }
        return b;
    }
    Phase phase_=Phase::dialogue;
    unsigned chosen_=0;
    unsigned story_route_{};
    struct StoryPose {size_t unit{};unsigned frames{},action{};uint8_t action_state{};};
    std::vector<StoryPose> story_poses_;
    unsigned story_pose_elapsed_{};
    unsigned story_wait_ms_{};
    size_t story_wait_event_{};
    bool story_turning_{};
    std::optional<size_t> story_animation_event_;
    Phase story_movement_return_{Phase::story_ready};
    std::optional<original_scripted_movement::Group> story_group_;
    std::vector<size_t> story_group_units_;
    std::vector<size_t> story_hidden_units_;
    std::optional<original_scripted_movement::Track> story_movement_;
    size_t story_movement_event_{},story_movement_unit_{};
    bool story_movement_follow_{};
    size_t story_disappearance_event_{},story_disappearance_unit_{};
    unsigned story_disappearance_slot_{};
    void next_story_disappearance(){
        while(story_disappearance_slot_<3){
            int id=events.event_word(story_disappearance_event_,5+2*story_disappearance_slot_++);if(id<=0)continue;
            auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==id&&!u.hostile();});
            if(role==events.units.end())role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==id;});
            if(role==events.units.end())continue;
            if(!map_.begin_disappearance(events.units,unsigned(role-events.units.begin()),1,animation_elapsed_)){
                campaign_blocker_="Original455048 disappearance unavailable for role "+std::to_string(id);phase_=Phase::campaign_blocked;return;
            }
            story_disappearance_unit_=size_t(role-events.units.begin());map_.center(role->x,role->y);phase_=Phase::story_disappearance;return;
        }
        event_=story_disappearance_event_;line_=0;
        if(events.line_count(event_)){phase_=Phase::flag_dialogue;prepare();return;}
        events.complete(event_);dispatch_story();
    }
    bool begin_story_animation(size_t event){
        if(!campaign_)campaign_.emplace(root_);
        const auto group=OriginalCampaign::story_animation_group(events,event);
        std::vector<StoryPose> poses;bool turning=false;
        for(const auto& item:group.roles){
            auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return u.id==item.id&&u.hostile()==group.hostile;});
            if(role==events.units.end())continue;
            if(role->status_flags&0x10){campaign_blocker_="Original alternate appearance animation requires runtime sequence table 440677";phase_=Phase::campaign_blocked;return true;}
            unsigned facing=unsigned(item.direction)&3,sequence=unsigned(item.action)+facing;
            poses.push_back({size_t(role-events.units.begin()),campaign_->animation_sequence_length(role->appearance,sequence),unsigned(item.action)/4,role->action_state});
            turning=turning||role->direction!=facing;
        }
        for(size_t i=group.begin;i<=group.end;++i)if(events.type(i)==15&&events.event_word(i,3)==events.event_word(event,3))events.complete(i);
        for(const auto& item:group.roles){auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return u.id==item.id&&u.hostile()==group.hostile;});if(role==events.units.end())continue;
            role->direction=unsigned(item.direction)&3;role->animation_action=turning?0:unsigned(item.action)/4;role->movement_pose=true;role->movement_frame=0;role->action_state=1;
        }
        story_poses_=std::move(poses);story_pose_elapsed_=0;story_turning_=turning;phase_=Phase::story_animation;return true;
    }
    bool campaign_choice_{};
    std::optional<Phase> scripted_battle_return_;
    Phase campaign_choice_return_{Phase::battle};
    bool story_stage()const{return events.stage!=140&&OriginalCampaign(root_).stage(events.stage).mode==3;}
    void dispatch_story(bool starting_battle_script=false){
        if(scripted_battle_return_&&!starting_battle_script){phase_=*scripted_battle_return_;scripted_battle_return_.reset();dispatch_campaign();return;}
        for(size_t i=0;i<events.event_count();++i){int flag=events.event_word(i,3);if(!events.type(i)||flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            if(auto predicate=original_role_predicate(events,i)){
                if(!*predicate)continue;
                event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            if(events.type(i)==15&&(events.condition(i)==0||events.condition(i)==3)){begin_story_animation(i);return;}
            if(events.type(i)==16&&(events.condition(i)==0||events.condition(i)==4)){
                auto decoded=OriginalCampaign::story_movement_group(events,i);
                std::vector<std::pair<size_t,Battle::Point>> roles;
                for(const auto& item:decoded.roles){auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return u.id==item.id&&u.hostile()==decoded.hostile;});
                    if(role!=events.units.end())roles.push_back({size_t(role-events.units.begin()),{item.x,item.y}});
                }
                original_scripted_movement::Group group;
                if(!group.begin(*battle_,events,roles,decoded.camera)){campaign_blocker_="Original461808 parallel movement unavailable";phase_=Phase::campaign_blocked;return;}
                story_group_units_.clear();for(auto item:roles)story_group_units_.push_back(item.first);
                if((decoded.camera==1||decoded.camera==3)&&!roles.empty()){const auto& role=events.units[roles.front().first];map_.center(role.x,role.y);}
                for(size_t member=i;member<=decoded.end;++member)if(events.type(member)==16&&events.event_word(member,3)==events.event_word(i,3))events.complete(member);
                story_movement_return_=story_stage()?Phase::story_ready:phase_;
                group.apply(events.units);story_group_=std::move(group);phase_=Phase::story_movement;return;
            }
            if(events.type(i)==7&&(events.condition(i)==0||events.condition(i)==1)){
                auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==events.event_word(i,5)&&!u.hostile();});
                if(role==events.units.end())role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==events.event_word(i,5);});
                original_scripted_movement::Track track;
                if(role==events.units.end()||!track.begin(*battle_,*role,{events.event_word(i,7),events.event_word(i,9)},events.movement_class(*role))){
                    campaign_blocker_="Original45E107 scripted movement unavailable at event "+std::to_string(i);phase_=Phase::campaign_blocked;return;
                }
                story_movement_event_=i;story_movement_unit_=size_t(role-events.units.begin());story_movement_follow_=events.condition(i)==1;
                track.apply(*role);if(story_movement_follow_)map_.center(role->x,role->y);
                story_movement_=std::move(track);phase_=Phase::story_movement;return;
            }
            if(scripted_battle_return_&&events.type(i)==6&&events.condition(i)==0){
                phase_=*scripted_battle_return_;scripted_battle_return_.reset();dispatch_campaign();return;
            }
            if(events.type(i)==6&&events.condition(i)==1){
                story_disappearance_event_=i;story_disappearance_slot_=0;next_story_disappearance();return;
            }
            if(events.type(i)==12&&events.condition(i)==1){
                auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==events.event_word(i,5);});
                if(role==events.units.end()){campaign_blocker_="Original story missing role for461236 event "+std::to_string(i);phase_=Phase::campaign_blocked;return;}
                role->status_flags=(role->status_flags|0x40u)&~0x80u;
                event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            if(events.type(i)==8&&events.condition(i)==8){
                event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            if(events.type(i)==12&&(events.condition(i)==16||events.condition(i)==10)){
                auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==events.event_word(i,5);});
                if(role==events.units.end()){campaign_blocker_="Original story missing role for461110 event "+std::to_string(i);phase_=Phase::campaign_blocked;return;}
                // 461149 returns success for condition>=10 on dead/inactive roles,
                // before mutation. 41BA1E still presents lines, then 41BA56 consumes it.
                if(!OriginalCampaign::living(*role)){
                    event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                    events.complete(i);dispatch_story();return;
                }
                if(events.condition(i)==16){
                    role->direction=unsigned(events.event_word(i,7))&3;role->movement_frame=0;
                    event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                    events.complete(i);dispatch_story();return;
                }
                if(!campaign_)campaign_.emplace(root_);
                if(role->status_flags&0x10){campaign_blocker_="Original alternate appearance single animation440677 unavailable";phase_=Phase::campaign_blocked;return;}
                unsigned action=unsigned(events.event_word(i,7)),facing=unsigned(events.event_word(i,9))&3;
                unsigned frames=campaign_->animation_sequence_length(role->appearance,action*4+facing);
                story_poses_={{size_t(role-events.units.begin()),frames,action,role->action_state}};
                story_turning_=role->direction!=facing;story_pose_elapsed_=0;story_animation_event_=i;
                role->direction=facing;role->animation_action=story_turning_?0:action;role->movement_pose=true;role->movement_frame=0;
                phase_=Phase::story_animation;return;
            }
            if(events.type(i)==8&&events.condition(i)==7){
                story_wait_event_=i;story_wait_ms_=unsigned(uint16_t(events.event_word(i,5)))*10;phase_=Phase::story_wait;return;
            }
            if(events.type(i)==9&&events.condition(i)==0){
                event_=i;line_=0;if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            if(events.type(i)==8&&events.condition(i)==0){
                map_.center(events.event_word(i,5),events.event_word(i,7));event_=i;line_=0;
                if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            if(events.type(i)==8&&events.condition(i)==3){
                auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==events.event_word(i,5);});
                if(role==events.units.end())break;
                map_.center(role->x,role->y);event_=i;line_=0;
                if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
                events.complete(i);dispatch_story();return;
            }
            break;
        }
        for(size_t i=0;i<events.event_count();++i)if(events.flag_trigger(i)&&events.condition(i)==0){event_=i;line_=0;phase_=Phase::choice;selection_=0;prepare();return;}
        for(size_t i=0;i<events.event_count();++i)if(events.branch_trigger(i)){
            event_=i;events.consume_branch(i);line_=0;
            if(events.line_count(i)){phase_=Phase::branch;prepare();return;}
            events.complete(i);dispatch_story();return;
        }
        for(size_t i=0;i<events.event_count();++i){int flag=events.event_word(i,3);
            if(events.type(i)!=9||flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            if(events.condition(i)!=6)continue;
            event_=i;line_=0;
            if(events.line_count(i)){phase_=Phase::flag_dialogue;prepare();return;}
            story_route_=unsigned(events.event_word(i,5));events.complete(i);phase_=Phase::story_ready;return;
        }
        for(size_t i=0;i<events.event_count();++i){int flag=events.event_word(i,3);
            if(!events.type(i)||flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            if(events.type(i)==15&&(events.condition(i)==0||events.condition(i)==3)){begin_story_animation(i);return;}
            if(auto predicate=original_role_predicate(events,i);predicate&&!*predicate)continue;
            campaign_blocker_="Original story event "+std::to_string(i)+" opcode "+std::to_string(events.type(i))+" condition "+std::to_string(events.condition(i))+" requires native executor";
            if(events.type(i)==15&&(events.condition(i)==0||events.condition(i)==3)){
                const auto group=OriginalCampaign::story_animation_group(events,i);
                campaign_blocker_+=" 421B6F->454A96 grouped animation roles="+std::to_string(group.roles.size());
            }
            phase_=Phase::campaign_blocked;return;
        }
        OriginalEvents source(root_,events.difficulty);source.load_stage(events.stage);
        story_route_=OriginalCampaign::completed_story_route(source,events,OriginalCampaign(root_).stage(events.stage).next);
        if(story_route_){phase_=Phase::story_ready;return;}
        throw std::runtime_error("Original story stage has no supported continuation");
    }
    int selection_=0;
    std::vector<uint8_t> briefing_page_;
    size_t briefing_offset_=0;
    size_t event_=0,line_=0,page_=0;
    struct Glyph { std::vector<uint8_t> bytes; unsigned color; int x{},y{}; unsigned delay{}; };
    std::vector<std::vector<Glyph>> pages_;
    bool text_animation_=false,small_font_=false;
    unsigned message_delay_=10,text_elapsed_=0;
    size_t revealed_glyphs_=0;
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    size_t group(unsigned id) const {return dword(pal_,808+(id-1)*8)-56-(id-1);}
    size_t sprite_offset(unsigned archive,unsigned index)const {
        size_t g=group(archive);
        if(index>=word(cel_,g))throw std::runtime_error("Invalid dialogue sprite index");
        return g+dword(cel_,g+2+index*8);
    }
    void sprite_shadow(std::vector<uint8_t>& image,size_t p,int left,int top)const {
        int width=int(word(cel_,p))+1,height=int(word(cel_,p+2))+1-10;
        if(width<=0||width>630||height<=0||height>468)return;
        // 424D47: two eight-pixel table6 strips, not a black rectangle.
        if(left<8){if(top+height+18>=479)return;palette_effects_->transform_rect(image,0,top+8+height,width,8,6);}
        else {
            palette_effects_->transform_rect(image,left-8,top+8,8,height,6);
            palette_effects_->transform_rect(image,left-8,top+8+height,width,8,6);
        }
    }
    void sprite(std::vector<uint8_t>& image,size_t p,bool typed,int left,int top,bool opaque_zero=false)const {
        unsigned type=typed?cel_.at(p++):0;
        unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        if(w>640||h>480)throw std::runtime_error("Invalid dialogue sprite dimensions");
        if(type==1){
            for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned c=cel_.at(p++);int dx=left+int(x),dy=top+int(y);if(!c||dx<0||dx>=640||dy<0||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);image.at(dst+3)=255;}
            return;
        }
        if(type!=0)throw std::runtime_error("Unsupported original portrait encoding");
        for(unsigned y=0;y<h;++y){unsigned x=0;while(x<w){unsigned code=cel_.at(p++),n=code&127;
            if(!code){
                int dx=left+int(x++),dy=top+int(y);
                if(opaque_zero&&dx>=0&&dx<640&&dy>=0&&dy<480){size_t dst=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(k))*255/63);image.at(dst+3)=255;}
                continue;
            }
            if(!n||n>w-x)throw std::runtime_error("Invalid dialogue sprite run at "+std::to_string(p-1)+" width="+std::to_string(w)+" x="+std::to_string(x)+" run="+std::to_string(n));
            unsigned repeat=0;if(!(code&128))repeat=cel_.at(p++);
            while(n--){unsigned c=(code&128)?cel_.at(p++):repeat;int dx=left+int(x++),dy=top+int(y);
                if((!c&&!opaque_zero&&!(code&128))||dx<0||dx>=640||dy<0||dy>=480)continue;
                size_t dst=(size_t(dy)*640+dx)*4;
                for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);
                image.at(dst+3)=255;
            }
        }}
    }
    void prepare() {
        map_.set_event_flags(events.flags);
        pages_={{}};page_=0;revealed_glyphs_=0;text_elapsed_=0;
        auto dialogue=events.line(event_,line_);
        auto speaker=std::find_if(events.units.begin(),events.units.end(),[&](const auto& unit){return unit.id==unsigned(dialogue.speaker);});
        dialogue_camera_target_.reset();dialogue_visible_steps_=0;
        if(speaker!=events.units.end()){
            auto position=speaker;
            if((speaker->status_flags&0x20)||!(speaker->status_flags&0x80))position=std::find_if(events.units.begin(),events.units.end(),[&](const auto& unit){return unit.hostile()==speaker->hostile();});
            if(position!=events.units.end()&&!map_.dialogue_visible(position->x,position->y)){
                dialogue_camera_subject_={position->x,position->y};
                dialogue_camera_target_=map_.dialogue_camera(position->x,position->y);
            }
        }
        auto text=dialogue.text;
        unsigned color=42,delay=message_delay_,wait_delay=message_delay_;int text_x=0,text_y=0;
        auto hex=[&](size_t p){auto digit=[](unsigned c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid original text control");};return digit(text.at(p))*16+digit(text.at(p+1));};
        for(size_t p=0;p<text.size();){
            if(text[p]=='@'&&p+1<text.size()&&text[p+1]=='@'){
                p+=2;
                while(p<text.size() && !(text[p]=='@'&&p+1<text.size()&&text[p+1]=='@')){
                    if(text[p]=='W'){color=hex(p+1);p+=6;}
                    else if(text[p]=='D'){delay=hex(p+1);p+=3;}
                    else if(text[p]=='R'&&p+1<text.size()){if(text[p+1]=='W')color=42;else if(text[p+1]=='D')delay=message_delay_;p+=2;}
                    else throw std::runtime_error("Unsupported original text control");
                }
                if(p+1>=text.size())throw std::runtime_error("Unterminated original text control");
                p+=2;continue;
            }
            unsigned lead=text[p++];
            // 423C6E leaves '*' undrawn; 41D230 checks pagination after each character.
            if(lead=='*')pages_.back().push_back({{},color,text_x,text_y,wait_delay});
            else {
                if(lead<=0x80){pages_.back().push_back({{uint8_t(std::max(lead,30u))},color,text_x,text_y,wait_delay});                text_x+=small_font_?8:16;}
                                else {pages_.back().push_back({{uint8_t(lead),text.at(p++)},color,text_x,text_y,wait_delay});text_x+=small_font_?16:24;}
            }
            wait_delay=delay;
            if(p<text.size()){
                if(text_y>=149&&text_x>=432){pages_.push_back({});text_x=text_y=0;}
                else if(text_x>=432||(lead=='*'&&text_x>0)){text_x=0;text_y+=25;}
            }
        }
    }
    void enter_stage_battle(){
        events.adjust_battle_entry_stats();
        battle_.emplace(Battle::load(root_,events.map_id,disc_));battle_->enable_movement_animation();connect_combat_progression();
        map_.set_dynamic_object_chains({});map_.set_event_flags(events.flags);
        map_.set_dynamic_object_chains(events.dynamic_object_chains);map_.load(events.map_id);
        for(unsigned y=0;y<map_.height*2;++y)for(unsigned x=0;x<map_.width-(y&1);++x)
            if(!map_.object_at(int(x),int(y)))battle_->clear_runtime_object({int(x),int(y)});
        if(auto request=OriginalCampaign::save_request(OriginalCampaign::SavePoint::before_battle,0,1))autosave_requests_.push_back(*request);
        phase_=Phase::battle_intro;intro_elapsed_=0;banner_presentation_.reset();
    }
    void prepare_briefing(){
        briefing_page_.clear();
        size_t end=std::min(events.briefing.size(),briefing_offset_+400);
        while(briefing_offset_<end){unsigned c=events.briefing.at(briefing_offset_++);briefing_page_.push_back(uint8_t(c));if(c>0x80)briefing_page_.push_back(events.briefing.at(briefing_offset_++));}
    }
    bool start_campaign_script(){
        if(script_queue_.empty())return false;
        script_return_=phase_;event_=script_queue_.front();script_queue_.pop_front();line_=0;
        if(!events.line_count(event_)){events.complete(event_);return start_campaign_script();}
        phase_=Phase::campaign_dialogue;prepare();return true;
    }
    bool prepare_campaign_transaction_pose(){
        if(!battle_||battle_->animating_movement()||battle_->combat_feedback_active())return false;
        // Script callbacks capture raw state before the controller's next idle-pose tick.
        for(auto& unit:events.units){unit.animation_action=0;unit.movement_frame=0;unit.movement_pose=false;}
        return true;
    }
    bool dispatch_original_flag_operations(){
        for(size_t i=0;i<events.event_count();++i){unsigned type=events.type(i);if(type!=6&&type!=7&&type!=8&&type!=11&&type!=12&&type!=14)continue;
            int flag=events.event_word(i,3);if(flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            int condition=events.event_word(i,1),subject=events.event_word(i,5);
            if(auto predicate=original_role_predicate(events,i)){
                if(!*predicate)continue;
                if(events.line_count(i)){script_queue_.push_back(i);return start_campaign_script();}
                events.complete(i);return dispatch_original_flag_operations();
            }
            auto found=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return int(u.id)==subject&&OriginalCampaign::living(u);});
            if(type==12&&(condition==22||condition==1)){
                if(!prepare_campaign_transaction_pose())return false;
                if(!campaign_scripted_roles_adapter_){campaign_blocker_="Original461110 control transaction unavailable";phase_=Phase::campaign_blocked;return true;}
                auto transaction=campaign_scripted_roles_adapter_(events,i);
                if(!transaction||transaction->events.stage!=events.stage){campaign_blocker_="Original461110 control transaction rejected";phase_=Phase::campaign_blocked;return true;}
                auto prepared=*battle_;prepared.set_hostile_resources(transaction->events);
                if(transaction->commit)transaction->commit();
                using std::swap;swap(events,transaction->events);swap(*battle_,prepared);
            }else if(type==12&&condition==0){
                if(!prepare_campaign_transaction_pose())return false;
                if(!campaign_scripted_roles_adapter_){campaign_blocker_="Original461215 role reset transaction unavailable";phase_=Phase::campaign_blocked;return true;}
                auto transaction=campaign_scripted_roles_adapter_(events,i);
                if(!transaction||transaction->events.stage!=events.stage){campaign_blocker_="Original461215 role reset transaction rejected";phase_=Phase::campaign_blocked;return true;}
                auto prepared=*battle_;prepared.set_hostile_resources(transaction->events);
                if(transaction->commit)transaction->commit();
                using std::swap;swap(events,transaction->events);swap(*battle_,prepared);
            }else if(type==6&&condition==0){
                if(!prepare_campaign_transaction_pose())return false;
                if(!campaign_scripted_roles_adapter_){campaign_blocker_="Original opcode6 source-role transaction unavailable 42012F";phase_=Phase::campaign_blocked;return true;}
                auto transaction=campaign_scripted_roles_adapter_(events,i);
                if(!transaction||transaction->events.stage!=events.stage){campaign_blocker_="Original opcode6 source-role transaction rejected";phase_=Phase::campaign_blocked;return true;}
                auto prepared=*battle_;prepared.set_hostile_resources(transaction->events);
                if(transaction->commit)transaction->commit();
                using std::swap;swap(events,transaction->events);swap(*battle_,prepared);
            }else if(type==11&&(condition==11||condition==12)){
                if(!events.campaign_currencies){campaign_blocker_="Original campaign currency baseline unavailable";phase_=Phase::campaign_blocked;return true;}
                auto& value=events.campaign_currencies->at(size_t(condition-11));
                value=int32_t(uint32_t(value)+uint32_t(subject));
            }else if(type==14){
                if(condition==0&&found!=events.units.end()){
                    int other=events.event_word(i,7);if(other<1||other>40)continue;
                    auto& value=found->relationships[size_t(other-1)];value=std::clamp(value+events.event_word(i,9),-128,127);
                }else if(condition==1&&subject>=1&&subject<=40){
                    for(auto& u:events.units)if(!u.hostile()&&u.persistent_role6==0&&!(u.status_flags&0x40)){
                        auto& value=u.relationships[size_t(subject-1)];value=std::clamp(value+events.event_word(i,7),-128,127);
                    }
                }else continue;
            }else if(found==events.units.end())continue;
            else if(type==8&&condition==3){map_.center(found->x,found->y);}
            else if(type==12&&condition==16){found->direction=unsigned(events.event_word(i,7))&3;}
            else if(type==7&&condition==0){
                Battle::Point destination{events.event_word(i,7),events.event_word(i,9)};
                Battle::Point origin{found->x,found->y};
                std::vector<Battle::Point> search{origin};std::map<std::pair<int,int>,size_t> visited;visited[{origin.x,origin.y}]=0;
                bool reached=origin==destination;
                for(size_t at=0;at<search.size()&&!reached;++at){auto p=search[at];
                    for(int dy:{-1,1})for(int dx:{0,(p.y&1)?1:-1}){Battle::Point q{p.x+dx,p.y+dy};
                        if(visited.contains({q.x,q.y})||battle_->step_cost(*found,p,q)<0||battle_->occupant(events.units,q)>=0)continue;
                        visited[{q.x,q.y}]=at;search.push_back(q);if(q==destination){reached=true;break;}
                    }
                }
                if(!reached){campaign_blocker_="Original scripted movement destination unreachable";phase_=Phase::campaign_blocked;return true;}
                found->x=destination.x;found->y=destination.y;found->action_state=0;found->animation_action=0;
            }else continue;
            if(events.line_count(i)){script_queue_.push_back(i);return start_campaign_script();}
            events.complete(i);return dispatch_original_flag_operations();
        }
        return false;
    }
    bool dispatch_campaign(){
        if(!battle_||battle_->animating_movement()||battle_->combat_feedback_active())return false;
        if(campaign_arrival_adapter_&&arrivals_turn_!=battle_->turn()){
            if(!prepare_campaign_transaction_pose())return false;
            auto transaction=campaign_arrival_adapter_(events,battle_->turn());
            if(!transaction){campaign_blocker_="Original arrival transaction unavailable";phase_=Phase::campaign_blocked;return true;}
            if(transaction->events.stage!=events.stage)throw std::runtime_error("Arrival changed campaign stage");
            auto staged_battle=*battle_;staged_battle.set_hostile_resources(transaction->events);
            if(transaction->commit)transaction->commit();
            using std::swap;swap(events,transaction->events);swap(*battle_,staged_battle);
            arrivals_turn_=battle_->turn();
        }
        if(!script_queue_.empty())return start_campaign_script();
        for(size_t i=0;i<events.event_count();++i)if(events.flag_trigger(i)&&events.condition(i)==0){
            campaign_choice_=true;campaign_choice_return_=phase_;event_=i;line_=0;selection_=0;phase_=Phase::choice;prepare();return true;
        }
        for(size_t i=0;i<events.event_count();++i)if(events.turn_trigger(i,battle_->turn())){script_queue_.push_back(i);}
        if(!script_queue_.empty())return start_campaign_script();
        for(size_t i=0;i<events.event_count();++i){int flag=events.event_word(i,3);
            if(events.type(i)==16&&(events.condition(i)==0||events.condition(i)==4)&&flag>=1&&flag<=80&&events.flags[size_t(flag-1)]){dispatch_story();return true;}
        }
        battle_->settle_object_flags(events);
        if(dispatch_original_flag_operations())return true;
        for(size_t i=0;i<events.event_count();++i){int flag=events.event_word(i,3);
            if(flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            bool visible_script=(events.type(i)==12&&(events.condition(i)==10||events.condition(i)==16||events.condition(i)==1))||
                (events.type(i)==8&&events.condition(i)==7);
            if(!visible_script)continue;
            scripted_battle_return_=phase_;
            // Enter the shared original executor; its completion returns here.
            dispatch_story(true);return true;
        }

        for(size_t i=0;i<events.event_count();++i){
            if(events.type(i)!=9)continue;int flag=events.event_word(i,3),condition=events.event_word(i,1);
            if(flag<1||flag>80||!events.flags[size_t(flag-1)])continue;
            bool ready=condition==0||condition==6;std::vector<int> operands;
            if(condition==1||condition==2){ready=true;for(unsigned off:{5u,7u,9u}){int f=events.event_word(i,off);if(f>=1&&f<=80){operands.push_back(f);if(!events.flags[size_t(f-1)])ready=false;}}if(operands.empty())ready=false;}
            if(!ready)continue;
            if(condition==2)for(int f:operands)events.flags[size_t(f-1)]=0;
            if(!events.line_count(i)){events.complete(i);return dispatch_campaign();}
            script_queue_.push_back(i);return start_campaign_script();
        }
        if(dispatch_original_flag_operations())return true;
        if(campaign_decision_)return false;
        if(finish_confirmation_)return false;
        if(!campaign_)campaign_.emplace(root_);
        std::vector<OriginalEvents::Unit> allies,enemies;
        bool enemy=false;for(const auto& u:events.units){if(u.behavior==2||u.behavior==3||u.behavior==8)enemy=true;if(u.first_turn>int(battle_->turn()))continue;(enemy?enemies:allies).push_back(u);}
        OriginalCampaign::Runtime runtime;runtime.turn=battle_->turn();runtime.forced_victory=events.flags[3]!=0;runtime.forced_defeat=events.flags[4]!=0;
                runtime.special_condition=[&](int condition){return condition==31&&events.flags[0]&&campaign_->protagonist_at_exit(events);};
                auto decision=OriginalCampaign::evaluate(campaign_->stage(events.stage),allies,enemies,runtime);
        if(decision.outcome==OriginalCampaign::Outcome::pending)return false;
        // The visible track is drained, but its controller may retain a last
        // movement frame. Settle presentation without scheduling another action.
        for(auto& unit:events.units){unit.animation_action=0;unit.movement_frame=0;unit.movement_pose=false;}
        // 41BC44 dispatches only type2 records matching the victory group;
        // unselected choices and dormant flag operations remain unconsumed.
        campaign_decision_=decision;
        if(decision.outcome==OriginalCampaign::Outcome::defeat){phase_=Phase::campaign_defeat;return true;}
        for(size_t i:OriginalCampaign::victory_events(events,decision.group))script_queue_.push_back(i);
        if(!script_queue_.empty()){
            if(start_campaign_script())script_return_=Phase::campaign_rewards;
            else{phase_=Phase::campaign_rewards;dispatch_original_flag_operations();}
        }else{phase_=Phase::campaign_rewards;dispatch_original_flag_operations();}
        return true;
    }
public:
    OriginalEvents events;
    Battle& battle(){if(!battle_)throw std::runtime_error("No active battle");return *battle_;}
    const Battle& battle()const{if(!battle_)throw std::runtime_error("No active battle");return *battle_;}
    void reconnect_battle_callbacks(){connect_combat_progression();}
    void queue_original_death(std::vector<OriginalEvents::Unit>& units,size_t actor,size_t target){
        if(target>=units.size()||units[target].hp>0)throw std::runtime_error("Invalid original death notification");
        for(unsigned condition:{3u,2u})for(size_t i=0;i<events.event_count();++i){
            if(events.type(i)!=3||events.condition(i)!=condition||events.event_word(i,3)!=int(units[target].id))continue;
            if(condition==3&&(actor>=units.size()||events.event_word(i,5)!=int(units[actor].id)))continue;
            if(std::find(script_queue_.begin(),script_queue_.end(),i)==script_queue_.end())script_queue_.push_back(i);
            if(condition==2){unsigned f=events.completion_flag(i);if(f>=1&&f<=80)events.flags[f-1]=1;}
        }
        units[target].status_flags|=0x20;
    }
    bool campaign_modal()const{return phase_==Phase::campaign_dialogue||phase_==Phase::campaign_rewards||phase_==Phase::campaign_defeat||phase_==Phase::interbattle||phase_==Phase::campaign_blocked;}
    bool campaign_dialogue()const{return phase_==Phase::campaign_dialogue;}
    bool campaign_defeated()const{return phase_==Phase::campaign_defeat;}
    bool campaign_reward_report()const{return phase_==Phase::campaign_rewards;}
    unsigned last_reaction_target()const{return map_.last_reaction_target();}
    bool last_reaction_left()const{return map_.last_reaction_left();}
    uint64_t reaction_draws()const{return map_.reaction_draws();}
    const std::vector<uint8_t>& last_reaction_bitmap()const{return map_.last_reaction_bitmap();}
    std::vector<uint8_t> expected_reaction_bitmap(unsigned id,bool left)const{return map_.expected_reaction_bitmap(id,left);}
    bool interbattle_active()const{return phase_==Phase::interbattle;}
    const std::string& campaign_blocker()const{return campaign_blocker_;}
    const auto& autosave_requests()const{return autosave_requests_;}
    void clear_autosave_requests(){autosave_requests_.clear();}
    void set_campaign_reward_adapter(std::function<bool(OriginalEvents&,const OriginalCampaign::Settlement&)> callback){campaign_reward_adapter_=std::move(callback);}
    void set_campaign_arrival_adapter(std::function<std::optional<ArrivalTransaction>(const OriginalEvents&,unsigned)> callback,unsigned already_inserted_turn=0){campaign_arrival_adapter_=std::move(callback);arrivals_turn_=already_inserted_turn;}
    void mark_campaign_arrivals_inserted(unsigned turn){arrivals_turn_=turn;}
    void normalize_campaign_roster(OriginalEvents restored,unsigned already_inserted_turn=1){
        if(restored.stage!=events.stage||restored.difficulty!=events.difficulty||
           (battle_&&(battle_->animating_movement()||battle_->combat_feedback_active()))||!script_queue_.empty())
            throw std::runtime_error("Campaign roster normalization is not idle");
        auto staged_battle=battle_;if(staged_battle)staged_battle->set_hostile_resources(restored);
        using std::swap;swap(events,restored);swap(battle_,staged_battle);arrivals_turn_=already_inserted_turn;
    }
    void set_campaign_scripted_roles_adapter(std::function<std::optional<ArrivalTransaction>(const OriginalEvents&,size_t)> callback){campaign_scripted_roles_adapter_=std::move(callback);}
    void set_disc(OriginalDisc disc){map_.set_disc(disc);disc_=disc;}
    void set_interbattle_adapter(std::function<OriginalInterbattle::Result(OriginalInterbattle::Request)> callback){interbattle_adapter_=std::move(callback);}
    void campaign_scan(unsigned scan){if(interbattle_&&interbattle_active()){interbattle_->scan(scan);if(interbattle_adapter_)interbattle_->dispatch(interbattle_adapter_);}}
    std::optional<OriginalInterbattle::Request> interbattle_request()const{return interbattle_?interbattle_->request():std::nullopt;}
    unsigned pending_story_route()const{return phase_==Phase::story_ready?story_route_:0;}
    unsigned campaign_next_stage()const{return pending_story_route()?pending_story_route():campaign_?campaign_->selected_route(events):0;}
    void restore_campaign_entry(OriginalEvents restored){
        restore_battle(std::move(restored),1);phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();
    }
    void restore_campaign_story(OriginalEvents restored){
        if(OriginalCampaign(root_).stage(restored.stage).mode!=3)throw std::runtime_error("Not an original story stage");
        restore_campaign_entry(std::move(restored));
    }
    void resume_campaign_story_route(OriginalEvents restored){
        if(!pending_story_route()||restored.stage!=pending_story_route())throw std::runtime_error("Original story route not authorized");
        restore_battle(std::move(restored),1);phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();
        if(auto request=OriginalCampaign::save_request(OriginalCampaign::SavePoint::before_stage_story,0,1))autosave_requests_.push_back(*request);
    }
    void resume_campaign_stage(OriginalEvents restored){
        // Caller must merge persistent original role/global state before publication.
        // Reloading fresh stage units alone would silently discard level/equipment.
        if(!interbattle_active()||!interbattle_||!interbattle_->closed()||restored.stage!=campaign_next_stage())throw std::runtime_error("Campaign continuation not authorized");
        restore_battle(std::move(restored),1);phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();
        if(auto request=OriginalCampaign::save_request(OriginalCampaign::SavePoint::before_stage_story,0,1))autosave_requests_.push_back(*request);
    }
    bool finished=false;
    explicit Prologue(const std::filesystem::path& root,unsigned difficulty):unit_status_(root),map_(root),root_(root),events(root,difficulty){
        auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing dialogue graphics");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        cel_=read(root/"SUPERSP.CEL");pal_=read(root/"SUPERSP.PAL");pal_.at(0)=pal_.at(1)=pal_.at(2)=0;
        caption_items_=read(root/"ITEMSP.BIN");
        palette_effects_.emplace(pal_);
        // sub_4262EC builds pointers by adding each directory length to the previous pointer.
        // Its merged slots address the following CEL payload, rather than directory slot n-1.
        for(auto [id,width,height]:std::array<std::array<unsigned,3>,4>{{{149,203,49},{150,202,49},{152,325,1},{153,326,1}}}){
            size_t p=sprite_offset(2,id-142);
            if(word(cel_,p)+1!=width||word(cel_,p+2)+1!=height)throw std::runtime_error("Merged CEL index mismatch");
        }
        while(event_<events.event_count()&&!events.turn_trigger(event_,1))++event_;
        if(event_==events.event_count())throw std::runtime_error("Missing original turn-one event");
        map_.set_dynamic_object_chains(events.dynamic_object_chains);
        map_.load(events.map_id);
        map_.center(events.units.front().x,events.units.front().y);
        prepare();
    }
    bool choosing()const{return phase_==Phase::choice;}
    bool in_battle()const{return phase_==Phase::battle;}
    bool hostile_actions()const{return phase_==Phase::hostile_actions;}
    bool automatic_actions()const{return phase_==Phase::automatic_actions;}
    bool npc_actions()const{return phase_==Phase::npc_actions;}
    bool computer_actions()const{return hostile_actions()||automatic_actions()||npc_actions();}
    const original_combat::Feedback* combat_feedback()const{return battle_?&battle_->combat_feedback():nullptr;}
    bool battle_intro()const{return phase_==Phase::battle_intro;}
    void enable_presentation_timing(){presentation_driven_=true;}
    void frame_presented(){if(battle_intro())banner_presentation_.presented();}
    bool showing_objectives()const{return phase_==Phase::objectives;}
    void set_small_font(bool enabled){if(small_font_!=enabled){small_font_=enabled;if(phase_==Phase::dialogue||phase_==Phase::choice||phase_==Phase::branch||phase_==Phase::flag_dialogue)prepare();}}
    void enable_text_animation(unsigned delay){
        if(text_animation_)return;
        text_animation_=true;message_delay_=delay;prepare();
    }
    bool text_revealing()const{return text_animation_&&(phase_==Phase::campaign_dialogue||phase_==Phase::dialogue||phase_==Phase::branch||phase_==Phase::flag_dialogue)&&!pages_.empty()&&revealed_glyphs_<pages_.at(page_).size();}
    size_t revealed_glyphs()const{return revealed_glyphs_;}
    void reveal_text(){if(!dialogue_scrolling()&&text_revealing())revealed_glyphs_=pages_.at(page_).size();}
    void tick(unsigned milliseconds){
        map_.set_event_flags(events.flags);
        animation_elapsed_+=milliseconds;
        if(phase_==Phase::story_movement){
            if(story_group_){
                story_group_->tick(*battle_,milliseconds);story_group_->apply(events.units);
                if(story_group_->active())return;
                if(!story_group_->draw_completed())for(size_t unit:story_group_units_)story_hidden_units_.push_back(unit);
                story_group_.reset();story_group_units_.clear();
                if(story_movement_return_==Phase::story_ready)dispatch_story();
                else{phase_=story_movement_return_;dispatch_campaign();}return;
            }
            story_movement_->tick(*battle_,milliseconds);auto& role=events.units.at(story_movement_unit_);story_movement_->apply(role);
            if(story_movement_follow_)map_.center(role.x,role.y);
            if(story_movement_->active())return;
            story_movement_.reset();event_=story_movement_event_;line_=0;
            if(events.line_count(event_)){phase_=Phase::flag_dialogue;prepare();return;}
            events.complete(event_);dispatch_story();return;
        }
        if(phase_==Phase::story_disappearance){
            map_.tick_disappearance(milliseconds);
            if(!map_.disappearance_finished())return;
            events.units.at(story_disappearance_unit_).status_flags&=~0x80u;
            next_story_disappearance();return;
        }
        if(phase_==Phase::story_wait){
            story_wait_ms_-=std::min(story_wait_ms_,milliseconds);if(story_wait_ms_)return;
            events.complete(story_wait_event_);dispatch_story();return;
        }
        if(phase_==Phase::story_animation){
            story_pose_elapsed_+=milliseconds;
            if(story_turning_){if(story_pose_elapsed_<50)return;story_turning_=false;story_pose_elapsed_=0;for(const auto& pose:story_poses_){auto& unit=events.units.at(pose.unit);unit.animation_action=pose.action;unit.movement_frame=0;}return;}
            if(story_pose_elapsed_<120)return;
            story_pose_elapsed_=0;bool active=false;
            for(const auto& pose:story_poses_){auto& unit=events.units.at(pose.unit);
                if(unit.movement_frame+1<pose.frames){++unit.movement_frame;active=true;}
            }
            if(active)return;
            for(const auto& pose:story_poses_){auto& unit=events.units.at(pose.unit);unit.action_state=pose.action_state;unit.animation_action=0;unit.movement_pose=false;unit.movement_frame=0;}
            story_poses_.clear();
            if(story_animation_event_){size_t event=*story_animation_event_;story_animation_event_.reset();event_=event;line_=0;
                if(events.line_count(event)){phase_=Phase::flag_dialogue;prepare();return;}events.complete(event);
            }
            dispatch_story();return;
        }
        if(!dialogue_camera_target_&&text_revealing()){
            text_elapsed_+=milliseconds;
            // 41D01C resets its timer after every presented glyph; long frames do not catch up.
            if(text_elapsed_>=pages_.at(page_).at(revealed_glyphs_).delay*10){++revealed_glyphs_;text_elapsed_=0;}
        }
        if(dialogue_camera_target_){
            auto [x,y]=*dialogue_camera_target_;
            map_.camera_x+=std::clamp(x-map_.camera_x,-32,32);map_.camera_y+=std::clamp(y-map_.camera_y,-16,16);
            // 42B461 counts successful visibility checks; failures do not reset the count.
            if(map_.dialogue_visible(dialogue_camera_subject_.first,dialogue_camera_subject_.second,0))++dialogue_visible_steps_;
            if(dialogue_visible_steps_==4||(map_.camera_x==x&&map_.camera_y==y))dialogue_camera_target_.reset();
        }
        if(computer_actions()){
            const bool automatic=automatic_actions();
            const int previous_actor=battle_->moving_unit();
            const bool done=npc_actions()?battle_->tick_npc_actions(events.units,milliseconds):automatic?battle_->tick_automatic_actions(events.units,milliseconds):battle_->tick_hostile_actions(events.units,milliseconds);
            const int actor=battle_->moving_unit()>=0?battle_->moving_unit():previous_actor;
            if(actor>=0){
                auto poses=battle_->render_units(events.units);
                const auto& pose=poses.at(size_t(actor));
                map_.movement_camera(pose.x,pose.y);
            }
            if(dispatch_campaign())return;
            if(done){
                if(automatic){phase_=Phase::battle;end_battle_turn();}
                else if(npc_actions()){
                    battle_->begin_hostile_side(events.units);phase_=Phase::battle_intro;intro_elapsed_=0;banner_presentation_.reset();
                }
                else{phase_=Phase::battle_intro;intro_elapsed_=0;banner_presentation_.reset();}
            }
            return;
        }
        if(in_battle()){
            const bool was_moving=battle_->animating_movement();
            battle_->tick_movement(milliseconds);
            battle_->tick_combat_feedback(events.units,milliseconds);
            if(dispatch_campaign())return;
            if(battle_->state()==Battle::State::idle&&std::none_of(events.units.begin(),events.units.end(),[&](const auto& unit){return unit.controllable()&&unit.present(battle_->turn())&&!(unit.status_flags&0x20)&&(unit.status_flags&0x80)&&unit.action_state==0;}))end_battle_turn();
            if(was_moving){
                auto poses=battle_->render_units(events.units);
                const auto& pose=poses.at(size_t(battle_->selected()));
                map_.movement_camera(pose.x,pose.y);
            }
        }
        if(!battle_intro())return;
        if(presentation_driven_){
            if(!banner_presentation_.tick(milliseconds))return;
        }else{
            // Headless simulations retain their explicit logical clock.
            intro_elapsed_+=std::min(milliseconds,banner_pose_duration_);
            if(intro_elapsed_<banner_duration_)return;
        }
        if(!battle_->hostile_side()&&dispatch_campaign())return;
        // 40D7D4: side 0 -> first-turn objectives -> player; side 1 -> AI;
        // increment the round only after AI, then show the next side-0 banner.
        if(battle_->hostile_side()){
            sync_movement_contexts();
            battle_->set_hostile_resources(events);
            battle_->begin_hostile_actions(events.units);phase_=Phase::hostile_actions;intro_elapsed_=0;
        }else if(first_banner_){first_banner_=false;phase_=Phase::objectives;}
        else{phase_=Phase::battle;select_ordered_player(true);}
    }
    int selection()const{return selection_;}
    void move(int delta){if(dialogue_scrolling()||(in_battle()&&battle_->animating_movement()))return;if(choosing())selection_=std::clamp(selection_+delta,0,int(events.line_count(event_))-2);else if(in_battle())map_.pan(0,delta*16);}
    void pan(int dx,int dy){if(in_battle()&&!battle_->animating_movement())map_.pan(dx,dy);}
    void edge_scroll(float x,float y){
        if(in_battle()&&!battle_->animating_movement()&&(battle_->state()==Battle::State::idle||battle_->state()==Battle::State::movement||battle_->state()==Battle::State::attack_target)&&map_.edge_scroll(x,y))mouse(x,y,false);
    }
    void set_auto_selection(bool enabled){if(battle_)battle_->set_auto_selection(enabled);}
    void select_ordered_player(bool restart){
        sync_movement_contexts();
        if(battle_->select_next_player_unit(events.units,restart)){
            const auto& unit=events.units.at(size_t(battle_->selected()));map_.center(unit.x,unit.y);
        }
    }
    void set_mask_surfaces(bool enabled){if(battle_)battle_->set_mask_surfaces(enabled);}
    void set_checker_mask(bool enabled){map_.set_checker_mask(enabled);}
    void sync_movement_contexts(){
        std::vector<Battle::MovementContext> contexts(events.units.size());
        for(size_t i=0;i<contexts.size();++i){contexts[i].status34=events.units[i].status34;contexts[i].status118=uint8_t(events.units[i].abilities>>24);contexts[i].status116=uint8_t(events.units[i].abilities>>8);contexts[i].has_usable_ability=events.has_learned_ability(events.units[i]);contexts[i].movement_class=events.movement_class(events.units[i]);contexts[i].sprite_height=map_.unit_height(events.units[i].appearance,events.units[i].direction,events.units[i].animation_action);}
        battle_->set_movement_contexts(std::move(contexts));
    }
    void mouse(float x,float y,bool click){
        if(interbattle_active()){interbattle_->mouse(int(x),int(y),click);if(interbattle_adapter_)interbattle_->dispatch(interbattle_adapter_);return;}
        if(campaign_modal()){if(click)advance();return;}
        if(dialogue_scrolling())return;
        if(in_battle()){
            if(finish_confirmation_){
                finish_confirmation_->mouse(x,y,click);
                if(finish_confirmation_->result()){resolve_finish();return;}
                const auto& unit=events.units.at(size_t(battle_->selected()));auto [sx,sy]=map_.screen_position(unit.x,unit.y);
                int left=sx-12,top=sy-12;
                if(click&&x>=left-4&&x<=left+96&&y>=top-4&&y<=top+60){
                    if(x<=left+48&&y<=top+28)finish_facing(3);
                    else if(y<=top+28)finish_facing(2);
                    else if(x<=left+44)finish_facing(0);
                    else if(x<=left+92)finish_facing(1);
                }
                finish_confirmation_->mouse(x,y,click);resolve_finish();return;
            }
            sync_movement_contexts();auto before=battle_->state();
            battle_->mouse(events.units,x,y,click,map_.camera_x,map_.camera_y);
            begin_finish_confirmation();
            if(!battle_->animating_movement()&&battle_->state()==Battle::State::command&&before!=Battle::State::command&&battle_->selected()>=0){
                const auto& unit=events.units.at(battle_->selected());map_.movement_camera(unit.x,unit.y);
            }
            return;
        }
        if(choosing()&&x>=149&&y>=209&&y<261){int row=int(y-209)/26;if(row!=selection_)selection_=row;else if(click)advance();}
    }
    void begin_finish_confirmation(){
        if(!in_battle()||battle_->state()!=Battle::State::finish_confirm||finish_confirmation_)return;
        const auto& unit=events.units.at(size_t(battle_->selected()));auto [sx,sy]=map_.screen_position(unit.x,unit.y);
        size_t panel=sprite_offset(2,197-142);
        int panel_height=int(word(cel_,panel+2)),left=320-int(word(cel_,panel))/2,top=sy+50;
        if(top+panel_height>=479){
            top=sy-(int(map_.unit_height(unit.appearance,unit.direction,unit.animation_action))-1)-panel_height+32;
            auto movement_class=events.movement_class(unit);
            if(movement_class==3||movement_class==4||movement_class==5)top-=64;
            top=std::max(0,top);
        }
        // 45369A saves direction BEFORE 454159; cancel restores that baseline.
        finish_direction_=unit.direction;
        battle_->orient_finish_threat(events,events.units);
        finish_confirmation_.emplace(root_,OriginalMenu(root_),6,left,top,true);
    }
    void resolve_finish(){if(finish_confirmation_&&finish_confirmation_->result()){
        bool confirmed=*finish_confirmation_->result();
        if(!confirmed&&finish_direction_&&battle_->selected()>=0)events.units.at(size_t(battle_->selected())).direction=*finish_direction_;
        battle_->finish_action(events.units,confirmed);finish_confirmation_.reset();finish_direction_.reset();
        if(confirmed)select_ordered_player(false);
    }}
    void finish_facing(unsigned direction){if(in_battle()&&finish_confirmation_&&battle_->selected()>=0){
        auto& unit=events.units.at(size_t(battle_->selected()));
        if(!finish_direction_)finish_direction_=unit.direction;
        if(unit.direction!=(direction&3)){unit.direction=direction&3;unit.movement_frame=0;}
    }}
    void cancel_battle_selection(){if(in_battle()){if(finish_confirmation_){finish_confirmation_->cancel();resolve_finish();}else battle_->cancel(events.units);}}
    void battle_confirm(){if(in_battle()){if(finish_confirmation_){finish_confirmation_->confirm();resolve_finish();}else{sync_movement_contexts();battle_->activate(events.units);begin_finish_confirmation();}}}
    void battle_cycle(int delta){if(in_battle()&&!battle_->animating_movement()){if(finish_confirmation_)finish_confirmation_->toggle();else battle_->cycle_command(delta);}}
    void battle_cycle_target(){if(in_battle())battle_->cycle_target(events.units);}
    Battle::State battle_state()const{return battle_?battle_->state():Battle::State::idle;}
    std::pair<int,int> camera()const{return {map_.camera_x,map_.camera_y};}
    void restore_battle(OriginalEvents restored,unsigned turn){
        story_movement_.reset();story_group_.reset();story_group_units_.clear();story_hidden_units_.clear();
        campaign_choice_=false;story_route_=0;arrivals_turn_=0;script_queue_.clear();campaign_decision_.reset();campaign_settled_=false;campaign_blocker_.clear();interbattle_.reset();
        events=std::move(restored);battle_.emplace(Battle::load(root_,events.map_id,disc_));battle_->enable_movement_animation();battle_->restore_turn(turn);connect_combat_progression();
        // Flag changes reload the old map; do not give it the new map's chains.
        map_.set_dynamic_object_chains({});map_.set_event_flags(events.flags);
        map_.set_dynamic_object_chains(events.dynamic_object_chains);map_.load(events.map_id);
        for(unsigned y=0;y<map_.height*2;++y)for(unsigned x=0;x<map_.width-(y&1);++x)if(!map_.object_at(int(x),int(y)))battle_->clear_runtime_object({int(x),int(y)});
        finish_confirmation_.reset();finish_direction_.reset();dialogue_camera_target_.reset();phase_=Phase::battle;first_banner_=false;finished=true;
    }
    void show_objectives(){if(in_battle())phase_=Phase::objectives;}
    bool drawing_original_mouse()const{return in_battle()&&battle_->has_movement_mouse();}
    Battle::Point battle_command_origin()const{return battle_->command_origin(events.units,map_.camera_x,map_.camera_y);}
    void start_auto_battle(){if(in_battle()&&!battle_->animating_movement()&&!battle_->combat_feedback_active()){ 
        finish_confirmation_.reset();finish_direction_.reset();sync_movement_contexts();battle_->set_hostile_resources(events);
        battle_->begin_automatic_actions(events.units);phase_=Phase::automatic_actions;
    }}
    void end_battle_turn(){if(in_battle()&&!battle_->animating_movement()&&!battle_->combat_feedback_active()){ 
        finish_confirmation_.reset();finish_direction_.reset();sync_movement_contexts();battle_->set_hostile_resources(events);
        // 437B4A: finish allied NPC actions before entering the enemy side.
        battle_->begin_npc_actions(events.units);phase_=Phase::npc_actions;
    }}
    bool hostile_banner()const{return battle_intro()&&battle_->hostile_side();}
    std::string battle_status()const{return battle_?battle_->message():std::string{};}
    int battle_selected()const{return battle_?battle_->selected():-1;}
    int battle_command_index()const{return battle_?battle_->command_index():-1;}
    int battle_requested_command()const{return battle_?battle_->requested_command():-1;}
    void clear_battle_requested_command(){if(battle_)battle_->clear_requested_command();}
    unsigned battle_turn()const{return battle_?battle_->turn():0;}
    Battle::Point screen_cell(Battle::Point p)const{return battle_->screen_center(p,map_.camera_x,map_.camera_y);}
    size_t line_index()const{return line_;}
    size_t page_index()const{return page_;}
    bool dialogue_scrolling()const{return dialogue_camera_target_.has_value();}
    void advance(){
        if(phase_==Phase::campaign_rewards){
            if(campaign_settled_)return;
            auto stage=campaign_->stage(events.stage);
            size_t count=0;while(count<events.units.size()&&events.units[count].behavior!=2&&events.units[count].behavior!=3&&events.units[count].behavior!=8)++count;
            auto settlement=OriginalCampaign::plan(stage,*campaign_decision_,std::span<const OriginalEvents::Unit>(events.units.data(),count));
            settlement.next=campaign_->selected_route(events);
            auto staged=events;
            OriginalInterbattle staged_hub(root_,disc_);
            if(!campaign_reward_adapter_||!campaign_reward_adapter_(staged,settlement)){
                campaign_blocker_="Original reward inventory/role baseline unavailable";return;
            }
            events=std::move(staged);campaign_settled_=true;
            interbattle_=std::move(staged_hub);phase_=Phase::interbattle;campaign_blocker_.clear();return;
        }
        if(phase_==Phase::campaign_defeat||phase_==Phase::campaign_blocked||phase_==Phase::story_ready||interbattle_active())return;
        if(in_battle()||computer_actions()||battle_intro()||phase_==Phase::story_animation||phase_==Phase::story_movement||phase_==Phase::story_disappearance||phase_==Phase::story_wait||dialogue_scrolling())return;
        if(showing_objectives()){dialogue_camera_target_.reset();phase_=Phase::battle;finished=true;select_ordered_player(true);return;}
        if(choosing()){
            chosen_=unsigned(selection_+1);events.choose(event_,chosen_);
            if(story_stage()){dispatch_story();return;}
            if(campaign_choice_){
                campaign_choice_=false;phase_=campaign_choice_return_;
                for(size_t i=0;i<events.event_count();++i)if(events.branch_trigger(i)){events.consume_branch(i);script_queue_.push_back(i);}
                if(!script_queue_.empty())start_campaign_script();else dispatch_campaign();
                return;
            }
            for(size_t i=0;i<events.event_count();++i)if(events.branch_trigger(i)){event_=i;events.consume_branch(i);line_=0;phase_=Phase::branch;prepare();return;}
            throw std::runtime_error("Missing original choice branch");
        }
        if(phase_==Phase::briefing){
            if(briefing_offset_<events.briefing.size()){prepare_briefing();return;}
            phase_=Phase::dialogue;event_=0;while(event_<events.event_count()&&!events.turn_trigger(event_,1))++event_;line_=0;
            if(event_==events.event_count()){
                if(story_stage())dispatch_story();else enter_stage_battle();
            }else if(!events.line_count(event_)){
                events.complete(event_);if(story_stage())dispatch_story();else enter_stage_battle();
            }else prepare();
            return;
        }
        if(text_revealing()){revealed_glyphs_=pages_.at(page_).size();return;}
        if(page_+1<pages_.size()){++page_;revealed_glyphs_=0;text_elapsed_=0;return;}
        if(++line_<events.line_count(event_)){prepare();return;}
        events.complete(event_);
        if(scripted_battle_return_&&phase_==Phase::flag_dialogue){dispatch_story();return;}
        if(story_stage()&&(phase_==Phase::dialogue||phase_==Phase::branch||phase_==Phase::flag_dialogue)){
            if(phase_==Phase::flag_dialogue&&events.condition(event_)==6){story_route_=unsigned(events.event_word(event_,5));phase_=Phase::story_ready;}
            else dispatch_story();
            return;
        }
        if(phase_==Phase::campaign_dialogue){
            phase_=script_return_;
            if(!script_queue_.empty()){auto resume=script_return_;start_campaign_script();script_return_=resume;}
            else if(phase_==Phase::campaign_rewards)dispatch_campaign();
            else dispatch_campaign();
            return;
        }
        if(events.stage!=140){enter_stage_battle();return;}
        if(phase_==Phase::dialogue){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_trigger(i)&&events.condition(i)==0){event_=i;line_=0;phase_=Phase::choice;selection_=0;prepare();return;}
        }else if(phase_==Phase::branch){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_dialogue_trigger(i)){event_=i;line_=0;phase_=Phase::flag_dialogue;prepare();return;}
        }else if(phase_==Phase::flag_dialogue){
            events.clear_stage_flags();
            events.load_stage(chosen_==1?events.first_stage:events.alternate_stage);
            if(auto request=OriginalCampaign::save_request(OriginalCampaign::SavePoint::before_stage_story,0,1))autosave_requests_.push_back(*request);
            map_.set_dynamic_object_chains(events.dynamic_object_chains);
            map_.set_event_flags(events.flags);
            map_.load(events.map_id);map_.center(events.units.front().x,events.units.front().y);
            phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();return;
        }
        throw std::runtime_error("Unsupported original prologue event transition");
    }
    std::vector<uint8_t> draw(const OriginalMenu& font)const {
        if(interbattle_active())return interbattle_->draw();
        auto image=map_.draw();
        auto story_units=events.units;
        for(size_t index:story_hidden_units_)if(index<story_units.size())story_units[index].status_flags&=~0x80u;
        if(story_group_&&!story_group_->draw_completed())for(size_t i=0;i<story_group_units_.size();++i)
            if(!story_group_->tracks()[i].active())story_units[story_group_units_[i]].status_flags&=~0x80u;
        if(phase_==Phase::campaign_rewards){campaign_->render_rewards(image,campaign_->stage(events.stage),events.units);return image;}
        if(phase_==Phase::story_animation||phase_==Phase::story_movement||phase_==Phase::story_disappearance||phase_==Phase::story_wait){map_.draw_units(image,story_units,1,animation_elapsed_);return image;}
        if(phase_==Phase::campaign_defeat||phase_==Phase::campaign_blocked||phase_==Phase::story_ready)return image;
        if(battle_intro()||showing_objectives()){
            if(showing_objectives()){
                std::vector<OriginalEvents::Unit> visible;for(const auto& unit:events.units)if(unit.present(battle_->turn()))visible.push_back(unit);map_.draw_units(image,visible,battle_->turn(),animation_elapsed_);
            }
            if(battle_intro()){
                // sub_438719: one 50% edge, 176 rows at 60%, then the other edge.
                palette_effects_->transform(image,121,1,6);
                palette_effects_->transform(image,122,176,5);
                palette_effects_->transform(image,298,1,6);
                auto b=banner(presentation_driven_?banner_presentation_.pose():intro_elapsed_/banner_pose_duration_);
                sprite(image,sprite_offset(2,(battle_->hostile_side()?149:150)-142),false,208,b.y);
                font.draw_banner_ascii(image,{'T','U','R','N'},b.turn_x,250);
                auto turn=std::to_string(battle_->turn());
                font.draw_banner_ascii(image,std::vector<uint8_t>(turn.begin(),turn.end()),b.number_x,250);
                sprite(image,sprite_offset(2,152-142),false,b.strip-320,234,true);
                sprite(image,sprite_offset(2,153-142),false,639-b.strip,234,true);
            }else{
                size_t panel=sprite_offset(2,207-142);int left=320-int((word(cel_,panel)+1)/2),top=240-int((word(cel_,panel+2)+1)/2);
                sprite(image,panel,false,left,top);
                auto heading=font.text_bytes(47);
                font.draw_original_large(image,heading,left+320-int(heading.size())*6,top+17,57);
                font.draw_original_large(image,font.text_bytes(48),left+90,top+57,54);
                font.draw_original_large(image,font.text_bytes(49),left+90,top+108,119,42);
                font.draw_original_large(image,font.text_bytes(50),left+90,top+208,87,42);
                auto name=events.stage_name;name.insert(name.begin(),'.');auto number=std::to_string(events.stage);name.insert(name.begin(),number.begin(),number.end());
                font.draw_original_large(image,name,left+213,top+57,54);
                font.draw_original_wrapped(image,events.victory_objective,left+213,top+108,119,4,11,28,42);
                font.draw_original_wrapped(image,events.defeat_objective,left+213,top+208,87,4,11,28,42);
                unsigned npc=0,friendly=0,enemy=0;
                for(const auto& unit:events.units){
                    if(!unit.present(battle_->turn())||(unit.status_flags&0x20)||!(unit.status_flags&0x80))continue;
                    if(unit.hostile())++enemy;else if(unit.npc_class())++npc;else ++friendly;
                }
                auto outlined=[&](const std::vector<uint8_t>& bytes,int x,unsigned ink){
                    for(auto offset:std::array<std::pair<int,int>,4>{{{-1,0},{1,0},{0,-1},{0,1}}})
                        font.draw_original_large(image,bytes,x+offset.first,top+330+offset.second,43);
                    font.draw_original_large(image,bytes,x,top+330,ink);
                };
                outlined(font.text_bytes(19),left+86,40);outlined({'N','P','C'},left+264,38);outlined(font.text_bytes(20),left+436,60);
                auto count=[&](unsigned value,int x){auto s=std::to_string(value);font.draw_original_large(image,{s.begin(),s.end()},left+x,top+330,185);};
                count(friendly,144);count(npc,320);count(enemy,494);
            }
            palette_effects_->cycle(image,animation_elapsed_);
            return image;
        }
        if(in_battle()||computer_actions()){
            std::vector<OriginalEvents::Unit> visible;
            auto rendered_units=battle_->render_units(events.units);
            for(const auto& unit:rendered_units)if(unit.present(battle_->turn()))visible.push_back(unit);
            if(in_battle())battle_->draw_selection(image,rendered_units,map_.camera_x,map_.camera_y,0);
            std::optional<unsigned> reaction;
            const auto& feedback=battle_->combat_feedback();
            if(feedback.phase==original_combat::Feedback::Phase::windup&&!feedback.miss&&feedback.damage>0&&
               feedback.target<rendered_units.size()&&rendered_units[feedback.target].movement_pose)
                reaction=unsigned(feedback.target);
            map_.draw_units(image,rendered_units,battle_->turn(),animation_elapsed_,std::nullopt,reaction);
            if(computer_actions())battle_->draw_projectile(image,map_.camera_x,map_.camera_y);
            battle_->render_combat_feedback(events.units,[&](const auto& unit){auto [x,y]=map_.screen_position(unit.x,unit.y);return Battle::Point{x,y};},
                [&](auto unit,auto,unsigned palette){auto flash_units=rendered_units;unit.hp=std::max(1,unit.hp);flash_units.at(feedback.target)=unit;map_.draw_units(image,flash_units,battle_->turn(),animation_elapsed_,palette,std::nullopt,0,unsigned(feedback.target));},
                [&](const std::string& text,int x,int y,unsigned ink,unsigned shadow){font.draw_original_large(image,std::vector<uint8_t>(text.begin(),text.end()),x,y,ink,shadow);});
            if(in_battle())battle_->draw_selection(image,rendered_units,map_.camera_x,map_.camera_y,1);
            if(in_battle()&&!battle_->animating_movement()&&(battle_->state()==Battle::State::command||battle_->state()==Battle::State::attack_menu)){
                // sub_43AA52 uses CEL 143 (4F729C) at the opposite edge of the selected unit.
                const auto& unit=events.units.at(size_t(battle_->selected()));
                auto position=map_.screen_position(unit.x,unit.y);
                if(position.second<184&&status_panel_side_==1)status_panel_side_=2;
                else if(position.second>272&&status_panel_side_==2)status_panel_side_=1;
                int panel_y=status_panel_side_==2?363:0;
                sprite(image,sprite_offset(2,143-142),false,0,panel_y);
                unit_status_.draw(font,image,unit,panel_y);
                auto origin=battle_command_origin();bool submenu=battle_->state()==Battle::State::attack_menu;
                if(submenu){
                    int slot=0;
                    for(int attack=0;attack<8;++attack)if(battle_->attack_command_available(events.units,attack)){
                        sprite_shadow(image,sprite_offset(2,Battle::attack_icons[attack]-142),origin.x+slot*32,origin.y);++slot;
                    }
                    slot=0;
                    for(int attack=0;attack<8;++attack)if(battle_->attack_command_available(events.units,attack)){
                        unsigned id=Battle::attack_icons[attack]+(attack==battle_->attack_index()?1u:0u);
                        sprite(image,sprite_offset(2,id-142),false,origin.x+slot*32,origin.y,true);
                        if(attack==battle_->attack_index()){
                            std::optional<OriginalEvents::ActionRecord> action;
                            for(size_t target=0;target<events.units.size()&&!action;++target)
                                action=battle_->executable_action(events.units,size_t(battle_->selected()),target,attack);
                            OriginalBattleCaptions::draw(font,image,OriginalBattleCaptions::attack(font,attack,unit,action,caption_items_),origin.x+slot*32,origin.y);
                        }
                        ++slot;
                    }
                }else{
                    int shadow_slot=0;
                    for(int command=0;command<6;++command)if(battle_->command_available(events.units,command)){
                        sprite_shadow(image,sprite_offset(2,Battle::command_icons[command]-142),origin.x+shadow_slot*32,origin.y);++shadow_slot;
                    }
                    int slot=0;
                    for(int command=0;command<6;++command)if(battle_->command_available(events.units,command)){
                        unsigned id=Battle::command_icons[command];
                        // sub_43AE24 packs enabled entries; sub_43AA52 uses
                        // 32-pixel slots and the selected CEL variant (+1).
                        int variant=(command==battle_->command_index())?1:0;
                        sprite(image,sprite_offset(2,id-142+variant),false,origin.x+slot*32,origin.y,true);
                        if(variant)OriginalBattleCaptions::draw(font,image,OriginalBattleCaptions::command(font,command,unit.action_points<events.battle_config[8]),origin.x+slot*32,origin.y);
                        ++slot;
                    }
                }
            }
            if(in_battle()&&!battle_->animating_movement()&&(battle_->state()==Battle::State::idle||battle_->state()==Battle::State::movement)){
                int status_unit=battle_->hovered_unit(events.units);
                if(status_unit<0&&battle_->state()==Battle::State::movement)status_unit=battle_->selected();
                if(status_unit>=0){
                    const auto& unit=events.units.at(size_t(status_unit));
                    int panel_y=status_panel_side_==2?363:0;
                    sprite(image,sprite_offset(2,143-142),false,0,panel_y);
                    unit_status_.draw(font,image,unit,panel_y,unit.hostile());
                }
            }
            if(in_battle()&&battle_->state()==Battle::State::finish_confirm&&finish_confirmation_){
                const auto& unit=events.units.at(size_t(battle_->selected()));auto [sx,sy]=map_.screen_position(unit.x,unit.y);
                battle_->draw_finish_directions(image,sx-12,sy-12,unit.direction);
                finish_confirmation_->draw(image,font);
            }
            if(in_battle())battle_->draw_movement_mouse(image);
            palette_effects_->cycle(image,animation_elapsed_);
            return image;
        }
        map_.draw_units(image,story_units,phase_==Phase::campaign_dialogue?battle_->turn():1,animation_elapsed_);
        if(phase_==Phase::briefing){
            std::fill(image.begin(),image.end(),0);for(size_t i=3;i<image.size();i+=4)image[i]=255;
            size_t p=0;int x=58,y=150;
            while(p<briefing_page_.size()){
                unsigned lead=briefing_page_[p++];if(lead<=0x80)continue;
                font.draw_big5(image,{uint8_t(lead),briefing_page_.at(p++)},x,y,56);x+=24;
                if(x>=538){x=58;y+=25;}
            }return image;
        }
        if(dialogue_scrolling()){
            palette_effects_->cycle(image,animation_elapsed_);
            return image;
        }
        auto line=events.line(event_,std::min(line_,size_t(events.line_count(event_)-1)));
        int frame_y=125;
        if(!choosing()){
            auto speaker=std::find_if(events.units.begin(),events.units.end(),[&](const auto& unit){return unit.id==unsigned(line.speaker);});
            if(speaker!=events.units.end()&&!(speaker->status_flags&0x20)&&(speaker->status_flags&0x80)){
                auto [screen_x,screen_y]=map_.screen_position(speaker->x,speaker->y);
                bool above=screen_y>310;
                frame_y=above?std::max(32,screen_y-326):std::min(screen_y-2,253);
                sprite(image,sprite_offset(2,(above?237:238)-142),false,std::clamp(screen_x+(above?12:33),0,591),above?frame_y+215:frame_y-32);
            }
        }
        sprite(image,sprite_offset(2,235-142),false,6,frame_y,true);
        size_t portrait=sprite_offset(3,unsigned(line.speaker-1)*2);
        int portrait_width=int(word(cel_,portrait+1))+1;
        sprite(image,portrait,true,80-portrait_width/2,frame_y+29);
        font.draw_original_large(image,line.name,80-int(line.name.size())*6,frame_y+168,54);
        if(choosing()){
            // 41D426 draws the prompt via 423AC5, not the dialogue typewriter.
            font.draw_original_wrapped(image,line.text,149,150,42,2,16,28);
            for(size_t i=1;i<events.line_count(event_);++i)
                font.draw_original_large(image,events.line(event_,i).text,149,209+int(i-1)*26,int(i-1)==selection_?60:42);
        }else{
            const auto& glyphs=pages_.at(page_);
            const size_t visible=text_animation_?std::min(revealed_glyphs_,glyphs.size()):glyphs.size();
            for(size_t i=0;i<visible;++i){
                const auto& glyph=glyphs[i];
                if(glyph.bytes.empty())continue;
                bool ascii=glyph.bytes.front()<=128;
                // 423C6E calls 4235AE directly, without 4239AD's ASCII x+2.
                if(small_font_)font.draw_small(image,glyph.bytes,149+glyph.x,frame_y+33+glyph.y,glyph.color,true);
                else font.draw_big5(image,glyph.bytes,150+glyph.x,frame_y+29+glyph.y+(ascii?1:0),glyph.color);
            }
            if(page_+1<pages_.size()&&!text_revealing())
                sprite(image,sprite_offset(2,236-142),false,586,frame_y+183);
        }
        palette_effects_->cycle(image,animation_elapsed_);
        return image;
    }
};
}





