#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_map.hpp"
#include "battle.hpp"
#include <optional>

namespace s2ne {
class Prologue {
    std::vector<uint8_t> cel_,pal_;
    OriginalMap map_;
    std::filesystem::path root_;
    std::optional<Battle> battle_;
    enum class Phase { dialogue, choice, branch, flag_dialogue, briefing, battle_intro, objectives, battle };
    unsigned intro_elapsed_=0;
    bool first_banner_=true;
    // sub_438719 waits one centisecond per step and exits on the settled frame.
    static constexpr unsigned banner_duration_=80;
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
    struct Glyph { std::vector<uint8_t> bytes; unsigned color; };
    std::vector<std::vector<Glyph>> pages_;
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    size_t group(unsigned id) const {return dword(pal_,808+(id-1)*8)-56-(id-1);}
    size_t sprite_offset(unsigned archive,unsigned index)const {
        size_t g=group(archive);
        if(index>=word(cel_,g))throw std::runtime_error("Invalid dialogue sprite index");
        return g+dword(cel_,g+2+index*8);
    }
    void sprite(std::vector<uint8_t>& image,size_t p,bool typed,int left,int top)const {
        unsigned type=typed?cel_.at(p++):0;
        unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        if(w>640||h>480)throw std::runtime_error("Invalid dialogue sprite dimensions");
        if(type==1){
            for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned c=cel_.at(p++);int dx=left+int(x),dy=top+int(y);if(!c||dx<0||dx>=640||dy<0||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);image.at(dst+3)=255;}
            return;
        }
        if(type!=0)throw std::runtime_error("Unsupported original portrait encoding");
        for(unsigned y=0;y<h;++y){unsigned x=0;while(x<w){unsigned code=cel_.at(p++),n=code&127;
            if(!code){++x;continue;}
            if(!n||n>w-x)throw std::runtime_error("Invalid dialogue sprite run at "+std::to_string(p-1)+" width="+std::to_string(w)+" x="+std::to_string(x)+" run="+std::to_string(n));
            unsigned repeat=0;if(!(code&128))repeat=cel_.at(p++);
            while(n--){unsigned c=(code&128)?cel_.at(p++):repeat;int dx=left+int(x++),dy=top+int(y);
                if(!c||dx<0||dx>=640||dy<0||dy>=480)continue;
                size_t dst=(size_t(dy)*640+dx)*4;
                for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);
                image.at(dst+3)=255;
            }
        }}
    }
    void prepare() {
        pages_={{}};page_=0;
        auto text=events.line(event_,line_).text;
        unsigned color=42;size_t column=0,row=0;
        auto hex=[&](size_t p){auto digit=[](unsigned c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid original text control");};return digit(text.at(p))*16+digit(text.at(p+1));};
        for(size_t p=0;p<text.size();){
            if(text[p]=='@'&&p+1<text.size()&&text[p+1]=='@'){
                p+=2;
                while(p<text.size() && !(text[p]=='@'&&p+1<text.size()&&text[p+1]=='@')){
                    if(text[p]=='W'){color=hex(p+1);p+=6;}
                    else if(text[p]=='D')p+=3;
                    else if(text[p]=='R'&&p+1<text.size()){if(text[p+1]=='W')color=42;p+=2;}
                    else throw std::runtime_error("Unsupported original text control");
                }
                if(p+1>=text.size())throw std::runtime_error("Unterminated original text control");
                p+=2;continue;
            }
            if(column>=18){column=0;++row;}
            if(row>=6){pages_.push_back({});column=0;row=0;}
            unsigned lead=text[p++];
            if(lead<=0x80)pages_.back().push_back({{uint8_t(lead)},color});
            else pages_.back().push_back({{uint8_t(lead),text.at(p++)},color});
            ++column;
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
    explicit Prologue(const std::filesystem::path& root,unsigned difficulty):map_(root),root_(root),events(root,difficulty){
        auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing dialogue graphics");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        cel_=read(root/"SUPERSP.CEL");pal_=read(root/"SUPERSP.PAL");
        while(event_<events.event_count()&&!events.turn_trigger(event_,1))++event_;
        if(event_==events.event_count())throw std::runtime_error("Missing original turn-one event");
        map_.load(events.map_id);
        map_.center(events.units.front().x,events.units.front().y);
        prepare();
    }
    bool choosing()const{return phase_==Phase::choice;}
    bool in_battle()const{return phase_==Phase::battle;}
    bool battle_intro()const{return phase_==Phase::battle_intro;}
    bool showing_objectives()const{return phase_==Phase::objectives;}
    void tick(unsigned milliseconds){
        if(!battle_intro())return;
        intro_elapsed_+=milliseconds;
        if(intro_elapsed_<banner_duration_)return;
        // 40D7D4: side 0 -> first-turn objectives -> player; side 1 -> AI;
        // increment the round only after AI, then show the next side-0 banner.
        if(battle_->hostile_side()){
            battle_->end_turn(events.units);intro_elapsed_=0;
        }else if(first_banner_){first_banner_=false;phase_=Phase::objectives;}
        else phase_=Phase::battle;
    }
    int selection()const{return selection_;}
    void move(int delta){if(choosing())selection_=std::clamp(selection_+delta,0,int(events.line_count(event_))-2);else if(in_battle())map_.pan(0,delta*16);}
    void pan(int dx,int dy){if(in_battle())map_.pan(dx,dy);}
    void mouse(float x,float y,bool click){
        if(in_battle()){battle_->mouse(events.units,x,y,click,map_.camera_x,map_.camera_y);return;}
        if(choosing()&&x>=149&&y>=209&&y<261){int row=int(y-209)/26;if(row!=selection_)selection_=row;else if(click)advance();}
    }
    void cancel_battle_selection(){if(in_battle())battle_->cancel(events.units);}
    void battle_confirm(){if(in_battle())battle_->activate(events.units);}
    void battle_cycle(int delta){if(in_battle())battle_->cycle_command(delta);}
    void battle_cycle_target(){if(in_battle())battle_->cycle_target(events.units);}
    Battle::State battle_state()const{return battle_?battle_->state():Battle::State::idle;}
    Battle::Point battle_command_origin()const{return battle_->command_origin(events.units,map_.camera_x,map_.camera_y);}
    void end_battle_turn(){if(in_battle()){battle_->begin_hostile_side(events.units);phase_=Phase::battle_intro;intro_elapsed_=0;}}
    bool hostile_banner()const{return battle_intro()&&battle_->hostile_side();}
    std::string battle_status()const{return battle_?battle_->message():std::string{};}
    int battle_selected()const{return battle_?battle_->selected():-1;}
    unsigned battle_turn()const{return battle_?battle_->turn():0;}
    Battle::Point screen_cell(Battle::Point p)const{return battle_->screen_center(p,map_.camera_x,map_.camera_y);}
    size_t line_index()const{return line_;}
    size_t page_index()const{return page_;}
    void advance(){
        if(in_battle()||battle_intro())return;
        if(showing_objectives()){phase_=Phase::battle;finished=true;return;}
        if(choosing()){
            chosen_=unsigned(selection_+1);events.choose(event_,chosen_);
            for(size_t i=0;i<events.event_count();++i)if(events.branch_trigger(i)){event_=i;events.consume_branch(i);line_=0;phase_=Phase::branch;prepare();return;}
            throw std::runtime_error("Missing original choice branch");
        }
        if(phase_==Phase::briefing){
            if(briefing_offset_<events.briefing.size()){prepare_briefing();return;}
            phase_=Phase::dialogue;event_=0;line_=0;prepare();return;
        }
        if(page_+1<pages_.size()){++page_;return;}
        if(++line_<events.line_count(event_)){prepare();return;}
        events.complete(event_);
        if(events.stage!=140){battle_.emplace(Battle::load(root_,events.map_id));        phase_=Phase::battle_intro;intro_elapsed_=0;return;}
        if(phase_==Phase::dialogue){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_trigger(i)&&events.condition(i)==0){event_=i;line_=0;phase_=Phase::choice;selection_=0;prepare();return;}
        }else if(phase_==Phase::branch){
            for(size_t i=0;i<events.event_count();++i)if(events.flag_dialogue_trigger(i)){event_=i;line_=0;phase_=Phase::flag_dialogue;prepare();return;}
        }else if(phase_==Phase::flag_dialogue){
            events.load_stage(chosen_==1?events.first_stage:events.alternate_stage);
            map_.load(events.map_id);map_.center(events.units.front().x,events.units.front().y);
            phase_=Phase::briefing;briefing_offset_=0;prepare_briefing();return;
        }
        throw std::runtime_error("Unsupported original prologue event transition");
    }
    std::vector<uint8_t> draw(const OriginalMenu& font)const {
        auto image=map_.draw();
        if(battle_intro()||showing_objectives()){
            std::vector<OriginalEvents::Unit> visible;for(const auto& unit:events.units)if(unit.present(battle_->turn()))visible.push_back(unit);map_.draw_units(image,visible,battle_->turn());
            if(battle_intro()){
                auto b=banner(intro_elapsed_/10);
                sprite(image,sprite_offset(2,(battle_->hostile_side()?149:150)-142),false,208,b.y);
                font.draw_big5(image,{'T','U','R','N'},b.turn_x,250,33);
                auto turn=std::to_string(battle_->turn());
                font.draw_big5(image,std::vector<uint8_t>(turn.begin(),turn.end()),b.number_x,250,33);
                sprite(image,sprite_offset(2,152-142),false,b.strip-320,234);
                sprite(image,sprite_offset(2,153-142),false,639-b.strip,234);
            }else{
                size_t panel=sprite_offset(2,207-142);int left=320-int((word(cel_,panel)+1)/2),top=240-int((word(cel_,panel+2)+1)/2);
                sprite(image,panel,false,left,top);
                font.draw_text(image,47,left+272,top+17,57);
                font.draw_text(image,48,left+90,top+57,54);
                font.draw_text(image,49,left+90,top+108,119);
                font.draw_text(image,50,left+90,top+208,87);
                auto name=events.stage_name;name.insert(name.begin(),'.');auto number=std::to_string(events.stage);name.insert(name.begin(),number.begin(),number.end());
                font.draw_big5(image,name,left+213,top+57,54);
                auto text=[&](const std::vector<uint8_t>& bytes,int y,unsigned color){int x=left+213,start=x;for(size_t p=0;p<bytes.size();){unsigned c=bytes[p++];if(c=='\r')continue;if(c=='\n'){x=start;y+=28;continue;}std::vector<uint8_t> glyph{uint8_t(c)};if(c>128)glyph.push_back(bytes.at(p++));int width=c>128?24:16;if(x+width>start+264){x=start;y+=28;}font.draw_big5(image,glyph,x,y,color);x+=width;}};
                text(events.victory_objective,top+108,119);text(events.defeat_objective,top+208,87);
            }
            return image;
        }
        if(in_battle()){
            std::vector<OriginalEvents::Unit> visible;
            for(const auto& unit:events.units)if(unit.present(battle_->turn()))visible.push_back(unit);
            battle_->draw_selection(image,events.units,map_.camera_x,map_.camera_y);
            map_.draw_units(image,visible,battle_->turn());
            if(battle_->state()==Battle::State::command||battle_->state()==Battle::State::attack_menu){
                auto origin=battle_command_origin();bool submenu=battle_->state()==Battle::State::attack_menu;
                if(submenu){
                    sprite(image,sprite_offset(2,168-142+1),false,origin.x,origin.y);
                }else{
                    int slot=0;
                    for(int command=0;command<6;++command)if(battle_->command_available(events.units,command)){
                        unsigned id=Battle::command_icons[command];
                        // sub_43AE24 packs enabled entries; sub_43AA52 uses
                        // 32-pixel slots and the selected CEL variant (+1).
                        int variant=(command==battle_->command_index())?1:0;
                        sprite(image,sprite_offset(2,id-142+variant),false,origin.x+slot*32,origin.y);++slot;
                    }
                }
            }
            return image;
        }
        map_.draw_units(image,events.units);
        if(phase_==Phase::briefing){
            std::fill(image.begin(),image.end(),0);for(size_t i=3;i<image.size();i+=4)image[i]=255;
            size_t p=0;int x=58,y=150;
            while(p<briefing_page_.size()){
                unsigned lead=briefing_page_[p++];if(lead<=0x80)continue;
                font.draw_big5(image,{uint8_t(lead),briefing_page_.at(p++)},x,y,56);x+=24;
                if(x>=538){x=58;y+=25;}
            }return image;
        }
        auto line=events.line(event_,std::min(line_,size_t(events.line_count(event_)-1)));
        sprite(image,sprite_offset(2,235-142),false,6,125);
        sprite(image,sprite_offset(3,unsigned(line.speaker-1)*2),true,30,154);
        font.draw_big5(image,line.name,80-int(line.name.size())*6,293,54);
        const auto& glyphs=pages_.at(page_);
        for(size_t i=0;i<glyphs.size();++i)font.draw_big5(image,glyphs[i].bytes,149+int(i%18)*24,154+int(i/18)*25,glyphs[i].color);
        if(choosing())for(size_t i=1;i<events.line_count(event_);++i)font.draw_big5(image,events.line(event_,i).text,149,209+int(i-1)*26,int(i-1)==selection_?60:42);
        return image;
    }
};
}
