#pragma once
#include "original_events.hpp"
#include "original_script_predicates.hpp"
#include "original_menu.hpp"
#include <functional>
#include <optional>

namespace s2ne {
// Original 438381: dispatch pending events before calling evaluate(), and never
// evaluate while movement, dialogue, combat feedback or death settlement is active.
class OriginalCampaign {
    std::filesystem::path root_;
    std::vector<uint8_t> bin_, pal_, cel_, items_, prg_;
    OriginalMenu font_;
    static std::vector<uint8_t> read(const std::filesystem::path& p) {
        std::ifstream f(p,std::ios::binary);
        if(!f)throw std::runtime_error("Missing campaign resource");
        return {std::istreambuf_iterator<char>(f),{}};
    }
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    size_t table()const{return dword(pal_,848)-11;}
    size_t header(unsigned stage)const{
        if(!stage||stage>word(bin_,table()))throw std::runtime_error("Invalid campaign stage");
        return table()+2+(stage-1)*30;
    }
    void pixel(std::vector<uint8_t>& image,int x,int y,unsigned color)const{
        if(x<0||y<0||x>=640||y>=480)return;
        size_t p=(size_t(y)*640+x)*4;
        for(unsigned k=0;k<3;++k)image.at(p+k)=uint8_t(unsigned(pal_.at(color*3+k))*255/63);
        image.at(p+3)=255;
    }
    void sprite(std::vector<uint8_t>& image,unsigned id,int left,int top)const{
        size_t g=dword(pal_,816)-57,p=g+dword(cel_,g+2+(id-142)*8);
        unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;){
            unsigned op=cel_.at(p++),n=op?op&127:1,c=0;
            if(!n||n>w-x)throw std::runtime_error("Invalid campaign sprite");
            if(op&&!(op&128))c=cel_.at(p++);
            while(n--){unsigned ink=op&128?cel_.at(p++):c;
                if((op&128)||ink)pixel(image,left+int(x),top+int(y),ink);++x;}
        }
    }
public:
    enum class Outcome { pending, victory, defeat };
    struct Reward { unsigned type{}; std::vector<int> values; };
    struct Stage {
        unsigned id{},next{},mode{};
        std::array<std::vector<int>,2> victory;
        std::vector<int> defeat;
        std::vector<Reward> rewards;
        std::vector<uint8_t> name;
    };
    struct Runtime {
        unsigned turn{1},mode{};
        bool idle{true},forced_victory{},forced_defeat{};
        // Original predicates 1..20 and 31/32 need map-object/withdrawal state.
        std::function<bool(int)> special_condition;
    };
    struct Decision { Outcome outcome{Outcome::pending}; unsigned group{}; };
    struct Settlement {
        unsigned stage{},next{},group{};
        std::vector<Reward> rewards;
        std::vector<size_t> survivor_indices;
    };
    explicit OriginalCampaign(const std::filesystem::path& root)
        :root_(root),bin_(read(root/"SUPERSP.BIN")),pal_(read(root/"SUPERSP.PAL")),cel_(read(root/"SUPERSP.CEL")),items_(read(root/"ITEMSP.BIN")),prg_(read(root/"SUPERSP.PRG")),font_(root){}
    Stage stage(unsigned id)const{
        size_t h=header(id),data=table()+dword(bin_,h),end=data+dword(bin_,h+4);
        if(end>bin_.size()||end<data)throw std::runtime_error("Truncated campaign stage");
        Stage s;s.id=id;s.next=bin_.at(h+12);s.mode=bin_.at(h+21);
        unsigned count=bin_.at(h+25);size_t values=data+count*4;
        if(values>end)throw std::runtime_error("Truncated campaign directory");
        for(unsigned i=0;i<count;++i){
            unsigned type=word(bin_,data+i*4),n=word(bin_,data+i*4+2);
            if(n>(end-values)/2)throw std::runtime_error("Truncated campaign reward");
            if(type>=1&&type<=4){Reward r;r.type=type;
                for(unsigned j=0;j<n;++j)r.values.push_back(int(int16_t(word(bin_,values+j*2))));
                s.rewards.push_back(std::move(r));}
            values+=n*2;
        }
        size_t section=data+dword(bin_,h+26)+bin_.at(h+8)*101;
        if(section>end||end-section<14)throw std::runtime_error("Invalid campaign objectives");
        size_t p=section+14;
        for(unsigned i=0;i<word(bin_,section);++i){uint8_t c=bin_.at(p+i);if(c)s.name.push_back(c);}
        p+=word(bin_,section)+word(bin_,section+2)+word(bin_,section+4);
        for(unsigned k=0;k<2;++k)for(unsigned j=0;j<word(bin_,section+8+k*2);++j){s.victory[k].push_back(int(int16_t(word(bin_,p))));p+=2;}
        p+=word(bin_,section+6);
        for(unsigned j=0;j<word(bin_,section+12);++j){s.defeat.push_back(int(int16_t(word(bin_,p))));p+=2;}
        if(p>end)throw std::runtime_error("Campaign conditions outside stage");
        return s;
    }
    struct StoryAnimationRole { unsigned id{};int direction{},action{};size_t event{}; };
    struct StoryAnimationGroup { bool hostile{};size_t begin{},end{};std::vector<StoryAnimationRole> roles; };
    static StoryAnimationGroup story_animation_group(const OriginalEvents& events,size_t begin){
        unsigned condition=events.condition(begin);
        if(events.type(begin)!=15||(condition!=0&&condition!=3))throw std::runtime_error("Not an original grouped animation event");
        int capacity=events.event_word(begin,5),flag=events.event_word(begin,3);
        if(capacity<=0||flag<1||flag>80||!events.flags[size_t(flag-1)])throw std::runtime_error("Original grouped animation trigger unavailable");
        StoryAnimationGroup group;group.hostile=condition==3;group.begin=begin;group.end=events.event_count();
        for(size_t i=begin+1;i<events.event_count();++i){
            if(events.type(i)!=15||events.event_word(i,3)!=flag)continue;
            if(events.condition(i)==condition+2){group.end=i;break;}
            if(events.condition(i)!=condition+1)continue;
            if(group.roles.size()>=size_t(capacity))throw std::runtime_error("Original grouped animation capacity exceeded");
            int id=events.event_word(i,5);
            if(id>0)group.roles.push_back({unsigned(id),events.event_word(i,7),events.event_word(i,9)*4,i});
        }
        if(group.end==events.event_count())throw std::runtime_error("Original grouped animation terminator missing");
        return group;
    }
    struct StoryMovementRole {unsigned id{};int x{},y{};size_t event{};};
    struct StoryMovementGroup {size_t begin{},end{};bool hostile{};unsigned camera{};std::vector<StoryMovementRole> roles;};
    static StoryMovementGroup story_movement_group(const OriginalEvents& events,size_t event){
        int condition=events.condition(event),flag=events.event_word(event,3),capacity=events.event_word(event,5);
        if(events.type(event)!=16||(condition!=0&&condition!=4)||capacity<=0)throw std::runtime_error("Invalid original grouped movement header");
        StoryMovementGroup group{event,events.event_count(),condition==4,unsigned(events.event_word(event,7)),{}};
        for(size_t i=event+1;i<events.event_count();++i){
            if(events.type(i)!=16||events.event_word(i,3)!=flag)continue;
            if(events.condition(i)==condition+3){group.end=i;break;}
            if(events.condition(i)==condition+2)throw std::runtime_error("Original grouped movement role-relative operand requires461808");
            if(events.condition(i)!=condition+1)continue;
            int id=events.event_word(i,5);if(id<=0)continue;
            if(group.roles.size()>=size_t(capacity))throw std::runtime_error("Original grouped movement capacity exceeded");
            group.roles.push_back({unsigned(id),events.event_word(i,7),events.event_word(i,9),i});
        }
        // 422074 also ends the scan at event_count; a same-flag terminator is optional.
        return group;
    }
    static unsigned completed_story_route(const OriginalEvents& source,const OriginalEvents& completed,unsigned header_next=0){
        if(source.stage!=completed.stage||source.event_count()!=completed.event_count())throw std::runtime_error("Mismatched original story event state");
        unsigned target=0;
        for(size_t i=0;i<source.event_count();++i){
            if(source.type(i)!=9||source.condition(i)!=6||completed.type(i)!=0)continue;
            int flag=source.event_word(i,3),route=source.event_word(i,5);
            if(flag>=1&&flag<=80&&completed.flags[size_t(flag-1)]&&route>0){
                // 41C397/41C3A0 scan records in order; each successful 420DC3
                // assignment replaces 4F7CE0, including later flag-chain overrides.
                target=unsigned(route);
            }
        }
        if(!target&&header_next){
            bool exhausted=true;
            for(size_t i=0;i<source.event_count();++i)if(source.type(i)&&completed.type(i)){
                // Group operands/terminators with no trigger are not standalone pending events.
                if(source.type(i)==16&&(source.condition(i)==3||source.condition(i)==7)&&source.event_word(i,3)==0)continue;
                // 41B9A3 scans false predicates without consuming their records.
                if(auto predicate=original_role_predicate(completed,i);predicate&&!*predicate)continue;
                exhausted=false;break;
            }
            if(exhausted)target=header_next;
        }
        return target;
    }
    unsigned selected_route(const OriginalEvents& completed)const{
        OriginalEvents source(root_,completed.difficulty);source.load_stage(completed.stage);
        unsigned route=completed_story_route(source,completed);
        return route?route:stage(completed.stage).next;
    }
    unsigned animation_sequence_length(unsigned appearance,unsigned sequence)const{
        size_t index=dword(pal_,808+8*64)-10;unsigned count=word(prg_,index);
        if(!appearance||appearance>count||sequence>=30)throw std::runtime_error("Unsupported original grouped appearance sequence");
        size_t cursor=index+2+count*8+(appearance-1)*200,end=cursor+200;
        for(unsigned i=0;i<sequence;++i){while(cursor<end&&prg_.at(cursor))++cursor;if(++cursor>=end)throw std::runtime_error("Missing original grouped sequence");}
        unsigned length=0;while(cursor+length<end&&prg_.at(cursor+length))++length;
        if(cursor+length==end)throw std::runtime_error("Unterminated original grouped sequence");
        return length;
    }
    bool protagonist_at_exit(const OriginalEvents& events)const{
        unsigned id=events.flags[40]?1:11;
        auto role=std::find_if(events.units.begin(),events.units.end(),[&](const auto& u){return !u.hostile()&&u.id==id&&living(u);});
        if(role==events.units.end())return false;
        size_t maps=dword(pal_,808+8*(64+6-1))-15;
        size_t map=maps+dword(prg_,maps+2+(events.map_id-1)*8);
        int width=int(word(prg_,map)),height=int(word(prg_,map+2))*2;
        if(role->x<0||role->y<0||role->x>=width-(role->y&1)||role->y>=height)return false;
        size_t cell=map+28+(size_t(role->y)*width+role->x)*5;unsigned object=word(prg_,cell+3);
        if(!object)return false;
        size_t attrs=dword(pal_,808+8*(64+7-1))-16;
        if(object>word(prg_,attrs+2))throw std::runtime_error("Invalid original exit object");
        size_t objects=attrs+6+word(prg_,attrs)*2;
        return (word(prg_,objects+(object-1)*2)>>8)==6;
    }
    static bool living(const OriginalEvents::Unit& u){return (u.status_flags&0x80)&&!(u.status_flags&0x20);}
    static bool absent(std::span<const OriginalEvents::Unit> roster,int selector){
        if(selector>0){
            for(const auto& u:roster)if(int(u.id)==selector&&(u.status_flags&0x80)&&(u.status_flags&0x20))return true;
            return false;
        }
        for(const auto& u:roster){
            if(selector==-1&&u.behavior!=1)continue;
            if(selector==-2&&u.runtime_role_selector!=1)continue;
            if(selector==-3&&u.runtime_role_selector==1)continue;
            if(living(u))return false;
        }
        return true;
    }
    static Decision evaluate(const Stage& s,std::span<const OriginalEvents::Unit> allies,
                             std::span<const OriginalEvents::Unit> enemies,const Runtime& rt){
        if(!rt.idle)return {};
        // Demo mode uses a different roster-only rule (4383E0/4384FE).
        if(rt.mode==1){if(absent(allies,0))return {Outcome::defeat,0};if(absent(enemies,0))return {Outcome::victory,0};return {};}
        if(rt.turn>99||rt.forced_defeat)return {Outcome::defeat,0};
        for(int c:s.defeat){
            if((c==0&&absent(allies,-1))||(c>=1&&c<=43&&absent(allies,c))||
               (c>100&&c<=190&&rt.turn>=unsigned(c-100)))return {Outcome::defeat,0};
        }
        if(rt.forced_victory)return {Outcome::victory,0};
        for(unsigned k=0;k<2;++k){if(s.victory[k].empty())continue;bool all=true;
            for(int c:s.victory[k]){
                bool satisfied=c==0?absent(enemies,0):c==33?absent(enemies,-2):c==34?absent(enemies,-3):
                    ((c>=1&&c<=20)||c==31||c==32)&&rt.special_condition&&rt.special_condition(c);
                if(!satisfied){all=false;break;}
            }
            if(all)return {Outcome::victory,k+1};
        }
        return {};
    }
    static std::vector<size_t> victory_events(const OriginalEvents& events,unsigned group){
        std::vector<size_t> result;
        for(size_t i=0;i<events.event_count();++i)
            if(events.type(i)==2&&(events.condition(i)==0||events.condition(i)==group))result.push_back(i);
        return result;
    }
    static Settlement plan(const Stage& s,Decision d,std::span<const OriginalEvents::Unit> allies){
        if(d.outcome!=Outcome::victory)throw std::runtime_error("Cannot settle a non-victory");
        Settlement p;p.stage=s.id;p.next=s.next;p.group=d.group;p.rewards=s.rewards;
        for(size_t i=0;i<allies.size();++i)if(living(allies[i]))p.survivor_indices.push_back(i);
        return p;
    }
    enum class SavePoint { before_stage_story, before_battle, rotate_previous };
    struct SaveRequest { SavePoint point; unsigned slot; bool rotate_to_previous; };
    static std::optional<SaveRequest> save_request(SavePoint point,unsigned original_mode,unsigned turn){
        if(point==SavePoint::rotate_previous)return SaveRequest{point,53,false};
        if(original_mode==1||(point==SavePoint::before_stage_story&&turn!=1))return std::nullopt;
        return SaveRequest{point,point==SavePoint::before_stage_story?51u:52u,true};
    }
    // 436138/436170 request only; never write original SAVE fixtures here.
    static std::optional<unsigned> autosave_slot(unsigned original_mode,unsigned turn,bool before_story){
        if(original_mode==1||(before_story&&turn!=1))return std::nullopt;
        return before_story?51u:52u;
    }
    // Type1 recruit, type2 departure, type3 money, type4 item.
    // Adapter stages all original effects on a copy and commits once; failed or
    // unsupported effects cannot leave a partially rewarded live campaign.
    template<class State,class Apply>
    static bool transact(State& live,const Settlement& p,Apply apply){
        State next=live;if(!apply(next,p))return false;live=std::move(next);return true;
    }
    static unsigned victory_experience(unsigned xp){return uint16_t(xp+30u);}
    // Verified 46243A reward panel subset (recruit/money/items); not the 410C86 hub.
    // Call only after reward effects are staged, and await confirm before +30 XP.
    bool render_rewards(std::vector<uint8_t>& image,const Stage& s,
                        std::span<const OriginalEvents::Unit> roster)const{
        if(image.size()!=640u*480*4)throw std::runtime_error("Invalid campaign framebuffer");
        for(const auto& r:s.rewards){
            if(r.type!=1&&r.type!=3&&r.type!=4)return false;
            if(r.type==4)for(int id:r.values)if(id<1||unsigned(id)>word(items_,0))return false;
        }
        for(const auto& r:s.rewards)if(r.type==1)for(int id:r.values){
            if(std::none_of(roster.begin(),roster.end(),[&](const auto& u){return int(u.id)==id;}))return false;
        }
        size_t g=dword(pal_,816)-57,p=g+dword(cel_,g+2+(207-142)*8);
        int left=320-int(word(cel_,p)+1)/2,top=240-int(word(cel_,p+2)+1)/2;
        sprite(image,207,left,top);
        auto draw=[&](unsigned id,int x,int y,unsigned c){font_.draw_original_large(image,font_.text_bytes(id),x,y,c);};
        auto title=font_.text_bytes(34);
        font_.draw_original_large(image,title,left+310-int(title.size())*6,top+17,57);
        draw(48,left+90,top+57,54);font_.draw_original_large(image,s.name,left+213,top+57,54);
        int y=top+108;
        for(const auto& r:s.rewards){
            if(r.type==1){draw(35,left+90,y,119);for(size_t j=0;j<r.values.size();++j){
                auto it=std::find_if(roster.begin(),roster.end(),[&](const auto& u){return int(u.id)==r.values[j];});
                font_.draw_original_large(image,it->name,left+205+int(j%2)*163,y+int(j/2)*25,119);
            }y+=int((r.values.size()+1)/2)*25;}
            if(r.type==3&&!r.values.empty()){
                draw(37,left+90,top+330,119);auto value=std::to_string(r.values[0]);
                font_.draw_original_large(image,{value.begin(),value.end()},left+220,top+330,119);
            }
            if(r.type==4){draw(38,left+90,y,119);for(size_t j=0;j<r.values.size();++j){
                int x=left+198+int(j%2)*168,yy=y+int(j/2)*30;
                sprite(image,unsigned(r.values[j])+492,x,yy);
                size_t name=2+size_t(r.values[j]-1)*38+2;std::vector<uint8_t> text;
                for(size_t k=0;k<12&&items_.at(name+k);++k)text.push_back(items_.at(name+k));
                font_.draw_original_large(image,text,left+236+int(j%2)*168,yy,119);
            }y+=int((r.values.size()+1)/2)*30;}
        }
        return true;
    }
};
}

