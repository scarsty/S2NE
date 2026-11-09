#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_map.hpp"
#include "battle.hpp"
#include "original_palette.hpp"
#include "original_unit_status.hpp"
#include "original_confirmation.hpp"
#include <optional>

namespace s2ne {
class Prologue {
    std::vector<uint8_t> cel_,pal_;
    mutable std::optional<OriginalPalette> palette_effects_;
    OriginalUnitStatus unit_status_;
    OriginalMap map_;
    std::filesystem::path root_;
    std::optional<Battle> battle_;
    mutable std::optional<OriginalConfirmation> finish_confirmation_;
    enum class Phase { dialogue, choice, branch, flag_dialogue, briefing, battle_intro, objectives, battle };
    unsigned intro_elapsed_=0;
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
    void prepare_briefing(){
        briefing_page_.clear();
        size_t end=std::min(events.briefing.size(),briefing_offset_+400);
        while(briefing_offset_<end){unsigned c=events.briefing.at(briefing_offset_++);briefing_page_.push_back(uint8_t(c));if(c>0x80)briefing_page_.push_back(events.briefing.at(briefing_offset_++));}
    }
public:
    OriginalEvents events;
    bool finished=false;
    explicit Prologue(const std::filesystem::path& root,unsigned difficulty):unit_status_(root),map_(root),root_(root),events(root,difficulty){
        auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing dialogue graphics");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        cel_=read(root/"SUPERSP.CEL");pal_=read(root/"SUPERSP.PAL");pal_.at(0)=pal_.at(1)=pal_.at(2)=0;
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
    bool battle_intro()const{return phase_==Phase::battle_intro;}
    bool showing_objectives()const{return phase_==Phase::objectives;}
    void set_small_font(bool enabled){if(small_font_!=enabled){small_font_=enabled;if(phase_==Phase::dialogue||phase_==Phase::choice||phase_==Phase::branch||phase_==Phase::flag_dialogue)prepare();}}
    void enable_text_animation(unsigned delay){
        if(text_animation_)return;
        text_animation_=true;message_delay_=delay;prepare();
    }
    bool text_revealing()const{return text_animation_&&(phase_==Phase::dialogue||phase_==Phase::branch||phase_==Phase::flag_dialogue)&&!pages_.empty()&&revealed_glyphs_<pages_.at(page_).size();}
    size_t revealed_glyphs()const{return revealed_glyphs_;}
    void reveal_text(){if(!dialogue_scrolling()&&text_revealing())revealed_glyphs_=pages_.at(page_).size();}
    void tick(unsigned milliseconds){
        map_.set_event_flags(events.flags);
        animation_elapsed_+=milliseconds;
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
        if(in_battle()){
            const bool was_moving=battle_->animating_movement();
            battle_->tick_movement(milliseconds);
            if(battle_->state()==Battle::State::idle&&std::none_of(events.units.begin(),events.units.end(),[&](const auto& unit){return unit.controllable()&&unit.present(battle_->turn())&&!(unit.status_flags&0x20)&&(unit.status_flags&0x80)&&unit.action_state==0;}))end_battle_turn();
            if(was_moving){
                auto poses=battle_->render_units(events.units);
                const auto& pose=poses.at(size_t(battle_->selected()));
                map_.movement_camera(pose.x,pose.y);
            }
        }
        if(!battle_intro())return;
        // Do not skip poses when a map redraw takes longer than one original tick.
        intro_elapsed_+=std::min(milliseconds,banner_pose_duration_);
        if(intro_elapsed_<banner_duration_)return;
        // 40D7D4: side 0 -> first-turn objectives -> player; side 1 -> AI;
        // increment the round only after AI, then show the next side-0 banner.
        if(battle_->hostile_side()){
            battle_->end_turn(events.units);intro_elapsed_=0;
        }else if(first_banner_){first_banner_=false;phase_=Phase::objectives;}
        else phase_=Phase::battle;
    }
    int selection()const{return selection_;}
    void move(int delta){if(dialogue_scrolling()||(in_battle()&&battle_->animating_movement()))return;if(choosing())selection_=std::clamp(selection_+delta,0,int(events.line_count(event_))-2);else if(in_battle())map_.pan(0,delta*16);}
    void pan(int dx,int dy){if(in_battle()&&!battle_->animating_movement())map_.pan(dx,dy);}
    void edge_scroll(float x,float y){
        if(in_battle()&&!battle_->animating_movement()&&(battle_->state()==Battle::State::idle||battle_->state()==Battle::State::movement||battle_->state()==Battle::State::attack_target)&&map_.edge_scroll(x,y))mouse(x,y,false);
    }
    void set_mask_surfaces(bool enabled){if(battle_)battle_->set_mask_surfaces(enabled);}
    void set_checker_mask(bool enabled){map_.set_checker_mask(enabled);}
    void sync_movement_contexts(){
        std::vector<Battle::MovementContext> contexts(events.units.size());
        for(size_t i=0;i<contexts.size();++i){contexts[i].status34=events.units[i].status34;contexts[i].status118=uint8_t(events.units[i].abilities>>24);contexts[i].status116=uint8_t(events.units[i].abilities>>8);contexts[i].has_usable_ability=events.has_learned_ability(events.units[i]);contexts[i].movement_class=events.movement_class(events.units[i]);contexts[i].sprite_height=map_.unit_height(events.units[i].appearance,events.units[i].direction,events.units[i].animation_action);}
        battle_->set_movement_contexts(std::move(contexts));
    }
    void mouse(float x,float y,bool click){
        if(dialogue_scrolling())return;
        if(in_battle()){
            if(finish_confirmation_){finish_confirmation_->mouse(x,y,click);resolve_finish();return;}
            sync_movement_contexts();auto before=battle_->state();
            battle_->mouse(events.units,x,y,click,map_.camera_x,map_.camera_y);
            if(!battle_->animating_movement()&&battle_->state()==Battle::State::command&&before!=Battle::State::command&&battle_->selected()>=0){
                const auto& unit=events.units.at(battle_->selected());map_.movement_camera(unit.x,unit.y);
            }
            return;
        }
        if(choosing()&&x>=149&&y>=209&&y<261){int row=int(y-209)/26;if(row!=selection_)selection_=row;else if(click)advance();}
    }
    void resolve_finish(){if(finish_confirmation_&&finish_confirmation_->result()){battle_->finish_action(events.units,*finish_confirmation_->result());finish_confirmation_.reset();}}
    void cancel_battle_selection(){if(in_battle()){if(finish_confirmation_){finish_confirmation_->cancel();resolve_finish();}else battle_->cancel(events.units);}}
    void battle_confirm(){if(in_battle()){if(finish_confirmation_){finish_confirmation_->confirm();resolve_finish();}else{sync_movement_contexts();battle_->activate(events.units);}}}
    void battle_cycle(int delta){if(in_battle()&&!battle_->animating_movement()){if(finish_confirmation_)finish_confirmation_->toggle();else battle_->cycle_command(delta);}}
    void battle_cycle_target(){if(in_battle())battle_->cycle_target(events.units);}
    Battle::State battle_state()const{return battle_?battle_->state():Battle::State::idle;}
    std::pair<int,int> camera()const{return {map_.camera_x,map_.camera_y};}
    void restore_battle(OriginalEvents restored,unsigned turn){
        events=std::move(restored);battle_.emplace(Battle::load(root_,events.map_id));battle_->enable_movement_animation();battle_->restore_turn(turn);
        map_.set_dynamic_object_chains(events.dynamic_object_chains);map_.set_event_flags(events.flags);map_.load(events.map_id);
        for(unsigned y=0;y<map_.height*2;++y)for(unsigned x=0;x<map_.width-(y&1);++x)if(!map_.object_at(int(x),int(y)))battle_->clear_runtime_object({int(x),int(y)});
        finish_confirmation_.reset();dialogue_camera_target_.reset();phase_=Phase::battle;first_banner_=false;finished=true;
    }
    void show_objectives(){if(in_battle())phase_=Phase::objectives;}
    bool drawing_original_mouse()const{return in_battle()&&battle_->has_movement_mouse();}
    Battle::Point battle_command_origin()const{return battle_->command_origin(events.units,map_.camera_x,map_.camera_y);}
    void end_battle_turn(){if(in_battle()&&!battle_->animating_movement()){
        battle_->begin_hostile_side(events.units);phase_=Phase::battle_intro;intro_elapsed_=0;
    }}
    bool hostile_banner()const{return battle_intro()&&battle_->hostile_side();}
    std::string battle_status()const{return battle_?battle_->message():std::string{};}
    int battle_selected()const{return battle_?battle_->selected():-1;}
    unsigned battle_turn()const{return battle_?battle_->turn():0;}
    Battle::Point screen_cell(Battle::Point p)const{return battle_->screen_center(p,map_.camera_x,map_.camera_y);}
    size_t line_index()const{return line_;}
    size_t page_index()const{return page_;}
    bool dialogue_scrolling()const{return dialogue_camera_target_.has_value();}
    void advance(){
        if(in_battle()||battle_intro()||dialogue_scrolling())return;
        if(showing_objectives()){dialogue_camera_target_.reset();phase_=Phase::battle;finished=true;return;}
        if(choosing()){
            chosen_=unsigned(selection_+1);events.choose(event_,chosen_);
            for(size_t i=0;i<events.event_count();++i)if(events.branch_trigger(i)){event_=i;events.consume_branch(i);line_=0;phase_=Phase::branch;prepare();return;}
            throw std::runtime_error("Missing original choice branch");
        }
        if(phase_==Phase::briefing){
            if(briefing_offset_<events.briefing.size()){prepare_briefing();return;}
            phase_=Phase::dialogue;event_=0;line_=0;prepare();return;
        }
        if(text_revealing()){revealed_glyphs_=pages_.at(page_).size();return;}
        if(page_+1<pages_.size()){++page_;revealed_glyphs_=0;text_elapsed_=0;return;}
        if(++line_<events.line_count(event_)){prepare();return;}
        events.complete(event_);
        if(events.stage!=140){
            events.adjust_battle_entry_stats();
            battle_.emplace(Battle::load(root_,events.map_id));battle_->enable_movement_animation();
            map_.set_dynamic_object_chains(events.dynamic_object_chains);map_.set_event_flags(events.flags);map_.load(events.map_id);
            for(unsigned y=0;y<map_.height*2;++y)for(unsigned x=0;x<map_.width-(y&1);++x)
                if(!map_.object_at(int(x),int(y)))battle_->clear_runtime_object({int(x),int(y)});
            phase_=Phase::battle_intro;intro_elapsed_=0;return;
        }
        if(phase_==Phase::dialogue){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_trigger(i)&&events.condition(i)==0){event_=i;line_=0;phase_=Phase::choice;selection_=0;prepare();return;}
        }else if(phase_==Phase::branch){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_dialogue_trigger(i)){event_=i;line_=0;phase_=Phase::flag_dialogue;prepare();return;}
        }else if(phase_==Phase::flag_dialogue){
            events.load_stage(chosen_==1?events.first_stage:events.alternate_stage);
            map_.set_dynamic_object_chains(events.dynamic_object_chains);
            map_.set_event_flags(events.flags);
            map_.load(events.map_id);map_.center(events.units.front().x,events.units.front().y);
            phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();return;
        }
        throw std::runtime_error("Unsupported original prologue event transition");
    }
    std::vector<uint8_t> draw(const OriginalMenu& font)const {
        auto image=map_.draw();
        if(battle_intro()||showing_objectives()){
            if(showing_objectives()){
                std::vector<OriginalEvents::Unit> visible;for(const auto& unit:events.units)if(unit.present(battle_->turn()))visible.push_back(unit);map_.draw_units(image,visible,battle_->turn(),animation_elapsed_);
            }
            if(battle_intro()){
                // sub_438719: one 50% edge, 176 rows at 60%, then the other edge.
                palette_effects_->transform(image,121,1,6);
                palette_effects_->transform(image,122,176,5);
                palette_effects_->transform(image,298,1,6);
                auto b=banner(intro_elapsed_/banner_pose_duration_);
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
        if(in_battle()){
            std::vector<OriginalEvents::Unit> visible;
            auto rendered_units=battle_->render_units(events.units);
            for(const auto& unit:rendered_units)if(unit.present(battle_->turn()))visible.push_back(unit);
            battle_->draw_selection(image,rendered_units,map_.camera_x,map_.camera_y,0);
            map_.draw_units(image,visible,battle_->turn(),animation_elapsed_);
            battle_->draw_selection(image,rendered_units,map_.camera_x,map_.camera_y,1);
            if(!battle_->animating_movement()&&(battle_->state()==Battle::State::command||battle_->state()==Battle::State::attack_menu)){
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
                    sprite_shadow(image,sprite_offset(2,168-142),origin.x,origin.y);
                    sprite(image,sprite_offset(2,168-142+1),false,origin.x,origin.y,true);
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
                        sprite(image,sprite_offset(2,id-142+variant),false,origin.x+slot*32,origin.y,true);++slot;
                    }
                }
            }
            if(!battle_->animating_movement()&&(battle_->state()==Battle::State::idle||battle_->state()==Battle::State::movement)){
                int status_unit=battle_->hovered_unit(events.units);
                if(status_unit<0&&battle_->state()==Battle::State::movement)status_unit=battle_->selected();
                if(status_unit>=0){
                    const auto& unit=events.units.at(size_t(status_unit));
                    int panel_y=status_panel_side_==2?363:0;
                    sprite(image,sprite_offset(2,143-142),false,0,panel_y);
                    unit_status_.draw(font,image,unit,panel_y,unit.hostile());
                }
            }
            if(battle_->state()==Battle::State::finish_confirm){
                const auto& unit=events.units.at(size_t(battle_->selected()));auto [sx,sy]=map_.screen_position(unit.x,unit.y);
                int top=sy+50;
                if(top+36>=479)top=std::max(0,sy-int(map_.unit_height(unit.appearance,unit.direction,unit.animation_action))-4);
                if(!finish_confirmation_)finish_confirmation_.emplace(root_,font,6,201,top,true);
                finish_confirmation_->draw(image,font);
            }
            battle_->draw_movement_mouse(image);
            palette_effects_->cycle(image,animation_elapsed_);
            return image;
        }
        map_.draw_units(image,events.units,1,animation_elapsed_);
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
