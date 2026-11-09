#pragma once
#include "original_combat.hpp"
#include "original_events.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <optional>
#include <string>
#include <queue>
#include <limits>

namespace s2ne {
// Original terrain/action-point arithmetic with basic contact melee; scored AI and abilities remain pending.
class Battle {
public:
    struct Cell { int elevation{},ground{},object{}; };
    struct Point { int x{},y{}; bool operator==(const Point&)const=default; };
private:
    unsigned width_{},height_{};
    std::vector<Cell> cells_;
    std::vector<unsigned> ground_attributes_,object_attributes_;
    std::vector<uint8_t> movement_;
    std::vector<Point> sealed_cells_;
    std::array<int,12> config_{};
    unsigned turn_=1;
    bool hostile_side_=false;
    std::vector<uint8_t> effect_palette_;
    std::array<std::vector<uint8_t>,3> movement_effects_;
    std::array<std::vector<uint8_t>,2> selection_frames_;
    std::vector<uint8_t> ground_cursor_;
    std::array<std::vector<uint8_t>,4> movement_mouse_;
    std::array<Point,4> movement_mouse_sizes_{};
    Point mouse_position_{-1,-1},hovered_cell_{-1,-1};
    Point hovered_{-1,-1};
public:
    // Raw inputs used by sub_476707; offsets refer to its runtime unit record.
    struct MovementContext {
        unsigned extra_flags{},status34{},status118{},status116{};
        bool has_usable_ability{}; // sub_443235(side, unit, 3) == 1
        unsigned movement_class{},sprite_height{};
    };
private:
    std::vector<MovementContext> movement_contexts_;
    std::vector<OriginalEvents::Unit> movement_poses_;
    unsigned movement_elapsed_=0;
    unsigned movement_interval_=80;
    int moving_unit_=-1;
    bool movement_animation_enabled_=false;
    bool original_rules_=false;
    bool mask_surfaces_=false;
    original_combat::Random random_{1};
    std::optional<uint32_t> random_seconds_;
    int selected_=-1;
public:
    enum class State { idle, command, movement, attack_menu, attack_target };
    static constexpr std::array<unsigned,6> command_icons{158,160,162,164,188,166};
    // sub_43AE24 copies these six availability masks before packing the
    // surviving entries into the original 32-pixel command strip.
    static constexpr std::array<uint32_t,6> command_masks{
        0x80000000u, 0x40000000u, 0x20000000u,
        0x10000000u, 0x00010000u, 0x08000000u};
    static constexpr std::array<unsigned,8> attack_icons{168,170,172,174,176,178,180,182};
    static constexpr std::array<uint32_t,8> attack_masks{
        0x04000000u,0x02000000u,0x01000000u,0x00800000u,
        0x00400000u,0x00200000u,0x00100000u,0x00080000u};
private:
    State state_=State::idle;
    int command_=0,target_=-1;
    bool moved_=false,acted_=false;
    OriginalEvents::Unit before_move_{};
    std::vector<int> distances_,previous_;
    std::vector<uint8_t> movement_categories_;
    std::vector<Point> path_;
    std::string message_="Select a unit to open its original command bar";
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    uint32_t random_seconds()const{return random_seconds_.value_or(uint32_t(std::time(nullptr)));}
    int percent(){return random_.percent(random_seconds());}
    int roll(int n){return random_.bounded(n,random_seconds());}
public:
    Battle(unsigned w,unsigned h,std::vector<Cell> cells):width_(w),height_(h),cells_(std::move(cells)){
        if(!w||!h||cells_.size()!=size_t(w)*h)throw std::runtime_error("Invalid battle terrain");
        random_.initialize_with_clock([]{return uint32_t(std::time(nullptr));});
    }
    static Battle load(const std::filesystem::path& root,unsigned map){
        auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing battle terrain archive");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto prg=read(root/"SUPERSP.PRG"),pal=read(root/"SUPERSP.PAL");
        size_t group=dword(pal,808+8*(64+6-1))-10-(6-1);
        if(!map||map>word(prg,group))throw std::runtime_error("Invalid battle map ID");
        size_t m=group+dword(prg,group+2+(map-1)*8);
        unsigned w=word(prg,m),h=word(prg,m+2)*2;
        if(!w||w>256||!h||h>512)throw std::runtime_error("Invalid battle map bounds");
        std::vector<Cell> cells;
        for(size_t i=0;i<size_t(w)*h;++i){size_t p=m+28+i*5;cells.push_back({int(prg.at(p)),int(word(prg,p+1)),int(word(prg,p+3))});}
        Battle result(w,h,std::move(cells));
        size_t special=m+28+size_t(w)*h*5+word(prg,m+12)*10;
        for(unsigned i=0;i<word(prg,m+14);++i){size_t p=special+i*6;if(word(prg,p+4)&0x8000)result.sealed_cells_.push_back({int(int16_t(word(prg,p))),int(int16_t(word(prg,p+2)))});}
        auto dat_path=root/"cd"/"windows-disc1"/"SUPERSP"/"SUPERSP.DAT";
        if(!std::filesystem::exists(dat_path))dat_path=root/"cd"/"disc1"/"SUPERSP"/"SUPERSP.DAT";
        auto dat=read(dat_path);
        auto archive=[&](unsigned id){return size_t(dword(pal,808+8*(23+id-1))-8-(id-1));};
        size_t attr_group=dword(pal,808+8*(64+7-1))-16,attributes=attr_group+6;
        for(unsigned layer=0;layer<3;++layer){
            unsigned count=word(prg,attr_group+layer*2);
            auto& dest=layer==0?result.ground_attributes_:result.object_attributes_;
            if(layer<2)for(unsigned i=0;i<count;++i)dest.push_back(word(prg,attributes+i*2));
            attributes+=count*2;
        }
        size_t movement=archive(6),length=dword(pal,808+8*(23+5)+4)-13;
        result.movement_.assign(dat.begin()+movement,dat.begin()+movement+length);
        // sub_4262EC loads ICONSO from DAT item 5; sub_4785E6 uses
        // pointer slots 35/36/37 = DAT entries 36/37/38 (slot 0 is entry 1).
        size_t effects=archive(5);
        for(unsigned i=0;i<3;++i){
            size_t p=effects+dword(dat,effects+2+(35+i)*8);
            if(word(dat,p)!=63||word(dat,p+2)!=31)throw std::runtime_error("Invalid original movement effect");
            result.movement_effects_[i].assign(dat.begin()+p+4,dat.begin()+p+4+2048);
        }
        for(unsigned i=0;i<4;++i){
            constexpr std::array<unsigned,4> slots{15,17,19,18};
            size_t p=effects+dword(dat,effects+2+slots[i]*8);
            unsigned width=word(dat,p)+1,height=word(dat,p+2)+1;
            if(width>64||height>64)throw std::runtime_error("Invalid original movement mouse sprite");
            result.movement_mouse_sizes_[i]={int(width),int(height)};
            result.movement_mouse_[i].assign(dat.begin()+p+4,dat.begin()+p+4+size_t(width)*height);
        }
        size_t ground_cursor=effects+dword(dat,effects+2+25*8);
        if(word(dat,ground_cursor)!=63||word(dat,ground_cursor+2)!=31)throw std::runtime_error("Invalid original ground cursor");
        result.ground_cursor_.assign(dat.begin()+ground_cursor+4,dat.begin()+ground_cursor+4+64*32);
        // sub_431347 / sub_447AFE compose ICONSO 27 and 28 (0x2244 bytes).
        for(unsigned i=0;i<2;++i){
            size_t cursor=effects+dword(dat,effects+2+(26+i)*8);
            if(word(dat,cursor)!=63||word(dat,cursor+2)!=136)throw std::runtime_error("Invalid original selection frame");
            result.selection_frames_[i].assign(dat.begin()+cursor+4,dat.begin()+cursor+4+64*137);
        }
        result.effect_palette_.assign(pal.begin(),pal.begin()+768);
        result.effect_palette_[0]=result.effect_palette_[1]=result.effect_palette_[2]=0;
        OriginalEvents events(root,1);result.config_=events.battle_config;result.original_rules_=true;
        return result;
    }
    bool valid(Point p)const{return p.x>=0&&p.y>=0&&p.y<int(height_)&&p.x<int(width_)-(p.y&1);}
    Cell cell(Point p)const{return valid(p)?cells_.at(size_t(p.y)*width_+p.x):Cell{};}
    void clear_runtime_object(Point p){if(valid(p))cells_.at(size_t(p.y)*width_+p.x).object=0;}
    // 42ACF4 differs from storage bounds: border and raised top-edge cells are excluded.
    bool target_surface(Point p)const{
        if(p.x<0||p.y<0||(!(p.y&1)&&p.x<1)||p.x>=int(width_)-1||p.y>=int(height_)-1)return false;
        int elevation=cell(p).elevation;
        return p.y-(elevation>0?elevation-1:0)>=1&&std::find(sealed_cells_.begin(),sealed_cells_.end(),p)==sealed_cells_.end();
    }
    struct TerrainEffect { int result{};std::array<uint8_t,3> bytes{1,0,0}; };
    TerrainEffect terrain_effect(const OriginalEvents& resources,const OriginalEvents::Unit& unit,Point p)const{
        TerrainEffect effect;
        if(!valid(p)||!unit.appearance||movement_.empty())return effect;
        auto tile=cell(p);if(!tile.ground)return effect;
        unsigned attr=ground_attributes_.at(size_t(tile.ground-1));if(!attr)return effect;
        int terrain=int(int8_t(attr>>8));unsigned local=attr&255;
        size_t stride=word(movement_,0),kind=unit.appearance-1;
        auto triple=[&](size_t index){size_t offset=6+index*3;return std::array<uint8_t,3>{movement_.at(offset),movement_.at(offset+1),movement_.at(offset+2)};};
        effect.bytes=triple(kind*stride+(local?local-1:0));
        if(terrain==5||effect.bytes[0]==255)return effect;
        bool special=resources.movement_class(unit)!=0;
        int type=unit.movement_type;
        if((type>=1&&type<=5)||type==10){
            if(terrain==0||terrain==1||terrain==2||terrain==4)effect.result=1;
            else if(special)effect.result=2;else return effect;
        }else if(type>=6&&type<=8)effect.result=terrain==2||terrain==3?2:1;
        else if(type==9){if(terrain==1||terrain==2)effect.result=1;else if(special)effect.result=2;else return effect;}
        else return effect;
        if(tile.object){
            unsigned object=object_attributes_.at(size_t(tile.object-1)),category=object>>8,object_local=object&255;
            if(object_local>word(movement_,4)){effect.result=0;return effect;}
            auto extra=triple(kind*stride+word(movement_,2)-1+object_local);
            if(category==1||extra[0]==255){effect.result=0;return effect;}
            if(category==2)effect.result=2;
            else if(category==10){if(!special){effect.result=0;return effect;}effect.result=2;}
            for(size_t i=0;i<3;++i)effect.bytes[i]=uint8_t(effect.bytes[i]+extra[i]);
        }
        return effect;
    }
    bool target_visible(const OriginalEvents::Unit& target,std::span<const OriginalEvents::Unit> opposing,unsigned opposing_mode)const{
        if(!(target.abilities&0x80)&&!(target.status3c&0x10))return true;
        if(opposing_mode&9)return true;
        Point origin{target.x,target.y};int height=cell(origin).elevation;
        const int right=origin.x+(origin.y&1),left=right-1;
        for(Point p:std::array<Point,4>{{{right,origin.y-1},{left,origin.y+1},{left,origin.y-1},{right,origin.y+1}}}){
            auto observer=std::find_if(opposing.begin(),opposing.end(),[&](const auto& unit){
                return (unit.status_flags&0x80)&&(!(unit.status_flags&0x20)||(unit.status_flags&2))&&unit.x==p.x&&unit.y==p.y;
            });
            if(observer==opposing.end())continue;
            int other_height=cell(p).elevation,delta=other_height-height;
            if(other_height>0&&delta>=-3&&delta<=2)return true;
        }
        return false;
    }
    std::vector<Point> action_cells(Point origin,OriginalEvents::ActionRange range)const{
        std::vector<Point> result;range.maximum=std::max(range.minimum,range.maximum);
        if(range.shape==2){
            Point top=origin;
            for(int radius=1;radius<=range.maximum;++radius){
                if(!(top.y&1))--top.x;--top.y;
                if(radius<range.minimum)continue;
                auto append=[&](Point p){if(target_surface(p))result.push_back(p);};
                for(int x=top.x;x<=top.x+radius;++x){append({x,top.y});append({x,top.y+2*radius});}
                for(int y=top.y+2;y<=top.y+2*radius-2;y+=2){append({top.x,y});append({top.x+radius,y});}
            }
        }else{
            int height=cell(origin).elevation;
            for(int direction=0;direction<4;++direction){
                Point p=origin;
                for(int distance=1;distance<=range.maximum;++distance){
                    if(direction==0){if(p.y&1)++p.x;--p.y;}
                    else if(direction==1){if(!(p.y&1))--p.x;++p.y;}
                    else if(direction==2){if(!(p.y&1))--p.x;--p.y;}
                    else{if(p.y&1)++p.x;++p.y;}
                    if(distance<range.minimum)continue;
                    int delta=cell(p).elevation-height;
                    if(!target_surface(p)||delta>2||delta< -3)break;
                    result.push_back(p);
                }
            }
        }
        return result;
    }
    // 408455 compares the attacker's row parity, not the defender's.
    static unsigned attack_mode(Point attacker,Point defender,unsigned defender_direction,bool simple_rules=false){
        if(simple_rules)return 1;
        bool left=(attacker.y&1)?attacker.x<defender.x:attacker.x<=defender.x;
        unsigned rear=left?(attacker.y>defender.y?2u:1u):(attacker.y>defender.y?3u:0u);
        unsigned front=left?(attacker.y>defender.y?0u:3u):(attacker.y>defender.y?1u:2u);
        return defender_direction==rear?3:defender_direction==front?1:2;
    }
    struct ActionTarget { size_t unit_index{};Point position;unsigned direction{}; };
    std::vector<ActionTarget> action_targets(const OriginalEvents& resources,const OriginalEvents::Unit& actor,
        const OriginalEvents::ActionRecord& action,std::span<const OriginalEvents::Unit> candidates,
        std::span<const OriginalEvents::Unit> opposing,unsigned opposing_mode)const{
        std::vector<ActionTarget> result;
        if(!resources.action_equipment_allowed(actor,action))return result;
        for(Point p:action_cells({actor.x,actor.y},resources.action_range(actor,action))){
            for(size_t i=0;i<candidates.size();++i){
                const auto& candidate=candidates[i];
                if(!(candidate.status_flags&0x80)||((candidate.status_flags&0x20)&&!(candidate.status_flags&2))||candidate.x!=p.x||candidate.y!=p.y)continue;
                if(target_visible(candidate,opposing,opposing_mode))result.push_back({i,p,facing({actor.x,actor.y},p,actor.direction)});
                break;
            }
        }
        return result;
    }
    std::vector<unsigned> counter_actions(const OriginalEvents& resources,const OriginalEvents::Unit& defender,
        Point attacker_position,std::span<const OriginalEvents::Unit> attackers,std::span<const OriginalEvents::Unit> defenders,
        unsigned attacker_mode,uint8_t defender_39,uint32_t defender_status3c_dword)const{
        std::vector<unsigned> result;
        if((defender_39&2)||(defender_status3c_dword&0x2B000))return result;
        for(const auto& action:resources.action_records(defender)){
            if(action.fields[0]==11||action.fields[0]==12)continue;
            auto targets=action_targets(resources,defender,action,attackers,defenders,attacker_mode);
            if(std::any_of(targets.begin(),targets.end(),[&](const auto& target){return target.position==attacker_position;}))result.push_back(action.index);
        }
        return result;
    }
    struct AttackAvailability { uint32_t mask{};std::array<int,8> actions{-1,-1,-1,-1,-1,-1,-1,-1}; };
    AttackAvailability attack_availability(const OriginalEvents& resources,const OriginalEvents::Unit& actor,
        std::span<const OriginalEvents::Unit> candidates,std::span<const OriginalEvents::Unit> opposing,unsigned opposing_mode)const{
        AttackAvailability result;
        if(actor.action_points<attack_menu_cost(actor))return result;
        for(const auto& action:resources.action_records(actor)){
            int type=action.fields[0];if(type==11||type==12)continue;
            if(action_targets(resources,actor,action,candidates,opposing,opposing_mode).empty())continue;
            unsigned slot=type==5?1:type>=1&&type<=7?0:type==23?6:type==32?5:type==36?4:7;
            result.mask|=attack_masks[slot];result.actions[slot]=int(action.index);
        }
        if(result.mask)result.mask|=command_masks[0];
        return result;
    }
    static int original_distance(Point from,Point to){
        // 478AF3 scans staggered rings 1..79 and returns 80 when none matches.
        int left=from.x,top=from.y;
        for(int radius=1;radius<80;++radius){
            if(!(top&1))--left;
            --top;
            if((to.y==top||to.y==top+2*radius)&&to.x>=left&&to.x<=left+radius)return radius;
            if((to.x==left||to.x==left+radius)&&to.y>top&&to.y<top+2*radius&&((to.y-top)&1)==0)return radius;
        }
        return 80;
    }
    // sub_448CD6: four diagonal neighbours in the staggered grid.
    static bool adjacent(Point a,Point b){return std::abs(a.y-b.y)==1&&(b.x==a.x||b.x==a.x+((a.y&1)?1:-1));}
    static Point center(Point p){return {p.x*64+(p.y&1)*32+32,p.y*16+16};}
    Point pick(float x,float y,int camera_x,int camera_y)const{
        x+=camera_x;y+=camera_y;
        Point best{-1,-1};float distance=2;
        int row=int(std::floor((y-16)/16));
        for(int yy=row-2;yy<=row+2;++yy){int col=int(std::floor((x-(yy&1)*32)/64));
            for(int xx=col-1;xx<=col+1;++xx){Point p{xx,yy};if(!valid(p))continue;auto c=center(p);float d=std::abs(x-c.x)/32+std::abs(y-c.y)/16;if(d<=1&&d<distance){distance=d;best=p;}}
        }return best;
    }
    int selected()const{return selected_;}
    const std::string& message()const{return message_;}
    State state()const{return state_;}
    int command_index()const{return command_;}
    int target()const{return target_;}
    // This is the six-bit portion of dword_50049C consumed by sub_43AE24.
    // It deliberately exposes availability rather than inventing labels: the
    // renderer uses the same source icon IDs and marks unavailable slots.
    uint32_t command_availability(const std::vector<OriginalEvents::Unit>& units) const {
        if (selected_<0 || size_t(selected_)>=units.size()) return 0;
        const auto& u=units.at(selected_);
        if (!u.present(turn_) || !u.controllable()) return 0;
        uint32_t mask=command_masks[5];
        if ((!acted_||original_rules_) && u.action_points>=(original_rules_?attack_menu_cost(u):attack_cost(u))) mask|=command_masks[0];
        const auto context=size_t(selected_)<movement_contexts_.size()?movement_contexts_[selected_]:MovementContext{};
        bool move_budget=!original_rules_||u.max_action_points>=config_[0]+config_[context.movement_class?1:4];
        if (!moved_ && !(u.status34&1) && move_budget) {
            if(original_rules_)mask|=command_masks[1];
            else for(int cost:distances_)if(cost!=std::numeric_limits<int>::max()&&cost>0){mask|=command_masks[1];break;}
        }
        // The original sets these only after class/status/equipment checks;
        // fail closed until those records are decoded rather than enabling a
        // replacement action.
        return mask;
    }
    void refresh_command_availability(const std::vector<OriginalEvents::Unit>& units){available_mask_=command_availability(units);}
    bool command_available(const std::vector<OriginalEvents::Unit>& units,int index) const {
        return index>=0&&index<6&&(command_availability(units)&command_masks[size_t(index)])!=0;
    }
    int command_slot(const std::vector<OriginalEvents::Unit>& units,int command) const {
        if(command<0||command>=6||!command_available(units,command))return -1;
        int slot=0;for(int i=0;i<command;++i)if(command_available(units,i))++slot;return slot;
    }
    int command_from_slot(const std::vector<OriginalEvents::Unit>& units,int slot) const {
        if(slot<0)return -1;for(int i=0;i<6;++i)if(command_available(units,i)&&slot--==0)return i;return -1;
    }
    int command_count(const std::vector<OriginalEvents::Unit>& units) const {
        int count=0;for(int i=0;i<6;++i)if(command_available(units,i))++count;return count;
    }
    void prepare_commands(const std::vector<OriginalEvents::Unit>& units){
        available_mask_=command_availability(units);
        if(command_<0||command_>=6||!command_available_for_cycle(command_))command_=command_from_slot(units,0);
    }
    const std::vector<int>& route_costs() const{return distances_;}
    const std::vector<Point>& path()const{return path_;}
    // sub_477BEB compares signed remaining AP, not path length or terrain.
    static uint8_t movement_category(int remaining,unsigned flags,int ability_threshold,int attack_threshold){
        if((flags&2)&&remaining>=ability_threshold)return 2;
        if((flags&1)&&remaining>=attack_threshold)return 1;
        return 0;
    }
    void set_movement_contexts(std::vector<MovementContext> contexts){movement_contexts_=std::move(contexts);}
    const std::vector<uint8_t>& movement_categories()const{return movement_categories_;}
    // Optional raw destination-record override, applied after build_routes.
    // Normal operation generates these values in build_routes itself.
    void set_movement_categories(const std::vector<uint8_t>& categories){
        movement_categories_.assign(cells_.size(),0);
        for(size_t i=0;i<movement_categories_.size()&&i<categories.size();++i)
            movement_categories_[i]=categories[i]<=2?categories[i]:0;
    }
    void enable_movement_animation(){movement_animation_enabled_=true;}
    bool animating_movement()const{return moving_unit_>=0;}
    void tick_movement(unsigned milliseconds){
        if(!animating_movement())return;
        movement_elapsed_+=std::min(milliseconds,movement_interval_);
        if(movement_elapsed_>=movement_poses_.size()*movement_interval_){moving_unit_=-1;movement_poses_.clear();movement_elapsed_=0;}
    }
    std::vector<OriginalEvents::Unit> render_units(const std::vector<OriginalEvents::Unit>& units)const{
        auto result=units;
        if(animating_movement()&&size_t(moving_unit_)<result.size())result[moving_unit_]=movement_poses_.at(movement_elapsed_/movement_interval_);
        return result;
    }
    void cancel(){
        if(animating_movement())return;
        path_.clear();target_=-1;
        if(state_==State::movement||state_==State::attack_menu||state_==State::attack_target){state_=State::command;message_="Command cancelled; select another command";return;}
        selected_=-1;state_=State::idle;message_="Select a unit to open its original command bar";
    }
    void cancel(std::vector<OriginalEvents::Unit>& units){
        if(animating_movement())return;
        // 0x439E6D restores the saved position/AP when leaving a moved, uncommitted action.
        if(state_==State::command&&moved_&&!acted_&&selected_>=0){units.at(selected_)=before_move_;moved_=false;build_routes(units);prepare_commands(units);message_="Movement undone";return;}
        cancel();
    }
    static Point surface_center(Point p,Cell c){auto q=center(p);q.y-=std::max(0,c.elevation-1)*16;return q;}
    Point screen_center(Point p,int cx,int cy)const{auto q=surface_center(p,cell(p));return {q.x-cx,q.y-cy};}
    void set_mask_surfaces(bool enabled){mask_surfaces_=enabled;}
    Point pick_surface(float x,float y,int cx,int cy)const{
        if(original_rules_){
            // 42A2BD/42A3D2: integer division truncates toward zero, including negative edges.
            int half=cx/32,camera_row=cy/16;
            int mx=int(x)+((half&1)?32:0),my=int(y)+((camera_row&1)?16:0);
            int left=-((mx+2*my-32)/64)*64-32;
            int right=((2*my-mx+608)/64)*64-608;
            int anchor_x=(-left-right)/2,anchor_y=(right-left)/4;
            int local_x=mx-anchor_x+32,local_y=my-anchor_y;
            int quadrant=2*local_y+local_x<64?(local_x-2*local_y<0?3:1):(local_x-2*local_y<0?2:4);
            int col=(anchor_x-32)/64+half/2;
            if(anchor_x==0&&!(half&1))--col;
            int base_row=anchor_y/16+camera_row-((camera_row&1)?1:0);
            auto height_at=[&](Point p){
                if(!valid(p)||p.y<1||(!(p.y&1)&&(p.x==0||p.x==int(width_)-1)))return 0;
                return cell(p).elevation;
            };
            for(int rise=20;rise>=0;rise-=2){
                Point p{col,base_row+rise};
                if(valid(p)){
                    int h=height_at(p);
                    if(h==rise+1||(h==rise&&quadrant==2))return p;
                    if(h==rise+2&&quadrant==1)return p;
                    if(!mask_surfaces_&&h>=rise+2)return {-1,-1};
                }
                if(!rise)continue;
                p={col-(((base_row+rise-1)&1)?1:0),base_row+rise-1};
                if(valid(p)){
                    int h=height_at(p);
                    if(h==rise&&quadrant==3)return p;
                    if(!mask_surfaces_&&local_x<32&&((h==rise&&local_y>=16)||h>rise))return {-1,-1};
                }
                p={col+(((base_row+rise-1)&1)?0:1),base_row+rise-1};
                if(valid(p)){
                    int h=height_at(p);
                    if(h==rise&&quadrant==4)return p;
                    if(!mask_surfaces_&&local_x>=32&&((h==rise&&local_y>=16)||h>rise))return {-1,-1};
                }
            }
            return {-1,-1};
        }
        Point best{-1,-1};
        // Synthetic test maps use the geometric surface without the original terrain picker.
        for(int row=0;row<int(height_);++row)for(int col=0;col<int(width_)-(row&1);++col){Point p{col,row};if(cell(p).elevation<=0)continue;auto c=screen_center(p,cx,cy);if(std::abs(x-c.x)/32+std::abs(y-c.y)/16<=1)best=p;}
        return best;
    }
    Point command_origin(const std::vector<OriginalEvents::Unit>& units,int cx,int cy)const{
        if(selected_<0)return {-1,-1};auto c=screen_center({units[selected_].x,units[selected_].y},cx,cy);
        int count=state_==State::attack_menu?1:std::max(1,command_count(units));
        const auto context=size_t(selected_)<movement_contexts_.size()?movement_contexts_[selected_]:MovementContext{};
        int y=c.y+16;
        if(context.movement_class>=3&&context.movement_class<=5)y-=64;
        y=std::max(0,y);
        if(y>330)y-=int(context.sprite_height)+72;
        return {std::clamp(c.x-count*16,8,639-count*32),y};
    }
    void build_routes(const std::vector<OriginalEvents::Unit>& units){
        distances_.assign(cells_.size(),std::numeric_limits<int>::max());previous_.assign(cells_.size(),-1);movement_categories_.assign(cells_.size(),0);path_.clear();
        if(selected_<0)return;const auto& u=units[selected_];int start=u.y*int(width_)+u.x;
        using Node=std::pair<int,int>;std::priority_queue<Node,std::vector<Node>,std::greater<Node>> todo;
        distances_[start]=0;todo.push({0,start});int budget=original_rules_?std::min(110,u.action_points-config_[0]):110;
        while(!todo.empty()){auto [cost,index]=todo.top();todo.pop();if(cost!=distances_[index])continue;Point p{index%int(width_),index/int(width_)};
            for(int dy:{-1,1})for(int dx:{0,(p.y&1)?1:-1}){Point q{p.x+dx,p.y+dy};int step=step_cost(u,p,q);if(step<0||occupant(units,q)>=0||cost+step>budget)continue;int next=q.y*int(width_)+q.x;
                if(cost+step<distances_[next]){distances_[next]=cost+step;previous_[next]=index;todo.push({cost+step,next});}}
        }
        if(original_rules_){
            const auto context=size_t(selected_)<movement_contexts_.size()?movement_contexts_[selected_]:MovementContext{};
            unsigned flags=1u|context.extra_flags|(context.has_usable_ability?2u:0u);
            int ability_threshold=config_[7]+((context.status34&0x0c)?config_[11]:0);
            int attack_threshold=config_[6]-((context.status118&0x20)?4:0);
            if((context.status34&2)&&!(context.status116&0x80))attack_threshold+=config_[11];
            for(size_t i=0;i<distances_.size();++i)if(previous_[i]>=0)
                movement_categories_[i]=movement_category(budget-distances_[i],flags,ability_threshold,attack_threshold);
        }
    }
    bool preview(Point p){
        path_.clear();if(!valid(p)||previous_.empty())return false;int index=p.y*int(width_)+p.x;
        if(previous_[index]<0)return false;
        while(previous_[index]>=0){path_.push_back({index%int(width_),index/int(width_)});index=previous_[index];}
        std::reverse(path_.begin(),path_.end());return true;
    }
    int attack_menu_cost(const OriginalEvents::Unit& unit)const{
        // 43B096 reads dwords at EB/F0 then SAR 24: the signed bytes are EE/F3.
        return attack_cost(unit);
    }
    int attack_cost(const OriginalEvents::Unit& unit)const{
        int cost=config_[6]-((unit.abilities&0x20000000u)?4:0);
        if((unit.status34&2)&&!(unit.abilities&0x8000))cost+=config_[11];
        return cost;
    }
    bool can_attack(const std::vector<OriginalEvents::Unit>& units,size_t a,size_t b)const{
        if(a>=units.size()||b>=units.size()||a==b)return false;const auto& u=units[a];const auto& v=units[b];
        return u.present(turn_)&&v.present(turn_)&&u.hostile()!=v.hostile()&&adjacent({u.x,u.y},{v.x,v.y})&&cell({u.x,u.y}).elevation==cell({v.x,v.y}).elevation&&(!original_rules_||u.action_points>=attack_cost(u));
    }
    void cycle_command(int delta){
        if(state_!=State::command)return;
        // The original strip contains only entries whose availability bit is
        // set; keyboard cycling therefore skips disabled commands.
        int next=command_;
        for(int i=0;i<6;++i){next=(next+(delta<0?-1:1)+6)%6;if(command_available_for_cycle(next)){command_=next;return;}}
    }
    bool command_available_for_cycle(int index) const {
        return (command_masks[size_t(index)]&available_mask_)!=0;
    }
private:
    uint32_t available_mask_{};
public:
    void cycle_target(const std::vector<OriginalEvents::Unit>& units){
        if(state_!=State::attack_target||units.empty())return;
        for(size_t offset=1;offset<=units.size();++offset){size_t next=(size_t(std::max(target_,0))+offset)%units.size();if(can_attack(units,selected_,next)){target_=int(next);return;}}
    }
    bool activate(std::vector<OriginalEvents::Unit>& units){
        if(animating_movement())return false;
        if(selected_<0)return false;
        if(state_==State::attack_menu){state_=State::attack_target;target_=-1;
            for(size_t i=0;i<units.size();++i)if(can_attack(units,selected_,i)){target_=int(i);break;}
            if(target_<0){state_=State::command;message_="No supported ordinary melee target";return false;}
            message_="Select target; Enter/Space or left click confirms; Esc cancels";return true;}
        if(state_==State::attack_target){if(target_<0||!attack(units,selected_,target_))return false;acted_=true;state_=State::command;target_=-1;return true;}
        if(state_!=State::command)return false;
        prepare_commands(units);
        if(!command_available_for_cycle(command_)){message_="Command disabled by original availability flags";return false;}
        if(command_==0){state_=State::attack_menu;message_="Attack submenu: ordinary equal-height melee only";return true;}
        if(command_==1){if(moved_){message_="Movement already used; cancel to undo before committing";return false;}state_=State::movement;build_routes(units);message_="Choose a reachable destination; hover previews route, left click moves";return true;}
        if(command_==5){selected_=-1;state_=State::idle;path_.clear();message_="Unit action finished";return true;}
        message_="Original command not implemented; no action performed";return false;
    }
    void hover(const std::vector<OriginalEvents::Unit>& units,Point p){
        hovered_cell_=p;
        hovered_=valid(p)&&occupant(units,p)>=0?p:Point{-1,-1};
        if(state_==State::movement)preview(p);
        if(state_==State::attack_target){int hit=occupant(units,p);if(hit>=0&&can_attack(units,selected_,hit))target_=hit;}
    }
    bool mouse(std::vector<OriginalEvents::Unit>& units,float x,float y,bool pressed,int cx,int cy){
        mouse_position_={int(x),int(y)};
        if(animating_movement())return false;
        if(state_==State::command||state_==State::attack_menu){
            prepare_commands(units);auto o=command_origin(units,cx,cy);int count=state_==State::attack_menu?1:command_count(units);
            if(x>=o.x&&x<o.x+count*32&&y>=o.y&&y<o.y+32){
                if(state_==State::command){int slot=int(x-o.x)/32;int command=command_from_slot(units,slot);if(command<0)return false;command_=command;}
                return pressed?activate(units):false;
            }
            return false;
        }
        auto p=pick_surface(x,y,cx,cy);hover(units,p);return pressed?click(units,p):false;
    }
    int occupant(const std::vector<OriginalEvents::Unit>& units,Point p)const{
        for(size_t i=0;i<units.size();++i)if(units[i].present(turn_)&&units[i].x==p.x&&units[i].y==p.y)return int(i);return -1;
    }
    int step_cost(const OriginalEvents::Unit& u,Point from,Point to)const{
        if(!valid(to)||!adjacent(from,to))return -1;
        auto a=cell(from),b=cell(to);
        if(!original_rules_)return a.elevation>0&&a.elevation==b.elevation&&a.ground>4&&b.ground>4&&!b.object?1:-1;
        if(std::find(sealed_cells_.begin(),sealed_cells_.end(),to)!=sealed_cells_.end())return -1;
        if(to.y<1||to.y>=int(height_)-1||(!(to.y&1)&&(to.x==0||to.x==int(width_)-1)))return -1;
        int climb=b.elevation-a.elevation;
        if(climb>3||climb< -4||to.y-b.elevation<0||b.ground<=0)return -1;
        unsigned attr=ground_attributes_.at(b.ground-1),terrain=attr>>8;
        if(!attr||terrain==5)return -1;
        int type=u.movement_type;
        if((type>=1&&type<=5||type==10)&&terrain!=0&&terrain!=1&&terrain!=2&&terrain!=4)return -1;
        if(type==9&&terrain!=1&&terrain!=2)return -1;
        if(type<1||type>10||type==7||type==8)return -1; // Flying modes require their separate height rules.
        size_t stride=word(movement_,0),kind=u.appearance-1;
        size_t index=kind*stride+((attr&255)?(attr&255)-1:0);
        int cost=int(int8_t(movement_.at(6+index*3)));
        if(cost<0)return -1;
        if(b.object){
            unsigned object=object_attributes_.at(b.object-1),category=object>>8,local=object&255;
            if(category==1||category==10||local>word(movement_,4))return -1;
            index=kind*stride+word(movement_,2)-1+local;
            int extra=int(int8_t(movement_.at(6+index*3)));if(extra<0)return -1;cost+=extra;
        }
        return cost+config_[3]*std::max(0,climb);
    }
    bool can_step(const std::vector<OriginalEvents::Unit>& units,size_t index,Point to)const{
        if(index>=units.size()||!units[index].present(turn_)||!units[index].controllable())return false;
        const auto& u=units[index];int cost=step_cost(u,{u.x,u.y},to);
        return cost>=0&&occupant(units,to)<0&&(!original_rules_||cost<=std::min(110,u.action_points-config_[0]));
    }
    int hovered_unit(const std::vector<OriginalEvents::Unit>& units)const{return valid(hovered_)?occupant(units,hovered_):-1;}
    static unsigned facing(Point from,Point to,unsigned current){
        if(from.x==to.x&&from.y==to.y)return current;
        bool right=(from.y&1)?from.x<to.x:from.x<=to.x;
        return right?(from.y>to.y?2u:1u):(from.y>to.y?3u:0u);
    }
    bool step(std::vector<OriginalEvents::Unit>& units,size_t index,Point to){
        if(!can_step(units,index,to))return false;
        auto& u=units[index];if(original_rules_){u.action_points-=step_cost(u,{u.x,u.y},to);u.max_action_points=u.action_points;u.carry_points=std::min(u.carry_points,u.action_points);}
        u.direction=facing({u.x,u.y},to,u.direction);
        u.x=to.x;u.y=to.y;message_="Moved; action points "+std::to_string(u.action_points)+" (Enter: finish side)";return true;
    }
    // sub_408CB7 / sub_40A622: equal-height base arithmetic only; class/status/skill modifiers remain pending.
    static int ordinary_damage(int attack,int defense,int divisor,int minimum_roll){
        return original_combat::ordinary_damage(attack,defense,divisor,minimum_roll);
    }
    void seed(unsigned value){random_seconds_=value;random_=original_combat::Random(1);random_.initialize(value);}
    unsigned turn()const{return turn_;}
    bool hostile_side()const{return hostile_side_;}
    void begin_hostile_side(std::vector<OriginalEvents::Unit>& units){
        reset_side(units,false);hostile_side_=true;
        selected_=-1;state_=State::idle;path_.clear();target_=-1;
    }
    void reset_side(std::vector<OriginalEvents::Unit>& units,bool hostile){
        for(auto& u:units)if(u.present(turn_)&&u.hostile()==hostile){
            int carry=u.base_action_points>config_[5]?config_[5]:u.action_points;
            if(u.action_points+u.base_action_points>=110)carry=0;
            u.carry_points=carry;u.action_points=u.max_action_points=u.base_action_points+carry;u.status34=0;
        }
    }
    // Basic contact response only; the original scored movement/skill AI is not substituted.
    void end_turn(std::vector<OriginalEvents::Unit>& units){
        if(!hostile_side_)begin_hostile_side(units);
        unsigned responses=0;
        for(size_t a=0;a<units.size();++a){
            if(!units[a].hostile()||!units[a].present(turn_))continue;
            for(size_t b=0;b<units.size();++b)if(units[b].present(turn_)&&!units[b].hostile()&&adjacent({units[a].x,units[a].y},{units[b].x,units[b].y})&&cell({units[a].x,units[a].y}).elevation==cell({units[b].x,units[b].y}).elevation){
                if(strike(units,a,b))++responses;break;
            }
        }
        reset_side(units,true);++turn_;hostile_side_=false;selected_=-1;state_=State::idle;path_.clear();target_=-1;
        message_="Turn "+std::to_string(turn_)+"; enemy contact attacks "+std::to_string(responses)+"; Enter: finish side";
    }
    bool attack(std::vector<OriginalEvents::Unit>& units,size_t a,size_t b){
        if(a>=units.size()||!units[a].controllable())return false;
        return strike(units,a,b);
    }
    bool strike(std::vector<OriginalEvents::Unit>& units,size_t a,size_t b){
        if(a>=units.size()||b>=units.size()||a==b)return false;
        auto& attacker=units[a];auto& defender=units[b];
        if(!attacker.present(turn_)||!defender.present(turn_)||attacker.hostile()==defender.hostile()||!adjacent({attacker.x,attacker.y},{defender.x,defender.y})||cell({attacker.x,attacker.y}).elevation!=cell({defender.x,defender.y}).elevation)return false;
        attacker.direction=facing({attacker.x,attacker.y},{defender.x,defender.y},attacker.direction);
        if(original_rules_){
            int cost=attack_cost(attacker);if(attacker.action_points<cost)return false;
            attacker.action_points=int(int16_t(attacker.action_points-cost));attacker.max_action_points=attacker.action_points;
            attacker.carry_points=std::min(attacker.carry_points,attacker.action_points);attacker.status34|=2;
        }
        int attack_style=attack_mode({attacker.x,attacker.y},{defender.x,defender.y},defender.direction);
        int hit=std::clamp(attacker.tactics-defender.strikeback+(attack_style==3?5:attack_style==2?3:0),1,100)*12/10;
        if(percent()>hit){message_="Ordinary melee: miss";return true;}
        int chance=percent(),jitter=roll(10);
        int divisor=original_combat::damage_divisor(chance,jitter);
        int minimum_roll=(attacker.attack-defender.defense)*100/divisor<3?percent():0;
        int damage=ordinary_damage(attacker.attack+(attack_style==3?10:attack_style==2?5:0),defender.defense,divisor,minimum_roll);
        defender.hp=std::max(0,defender.hp-damage);
        if(defender.hp==0)defender.status34|=0x80;
        message_="Ordinary melee: -"+std::to_string(damage)+" HP; target HP "+std::to_string(defender.hp);
        return true;
    }
    bool click(std::vector<OriginalEvents::Unit>& units,Point p){
        if(animating_movement())return false;
        if(!valid(p))return false;int hit=occupant(units,p);
        if(state_==State::idle){
            if(hit<0||!units[hit].controllable())return false;
            selected_=hit;before_move_=units[hit];moved_=acted_=false;command_=0;state_=State::command;
            build_routes(units);prepare_commands(units);
            message_="Original command bar: available commands only";return true;
        }
        if(state_==State::movement){
            build_routes(units);if(!preview(p)){message_="Destination unreachable or occupied";return false;}
            auto route=path_;auto saved=units[selected_];
            movement_poses_.clear();
            const unsigned movement_class=size_t(selected_)<movement_contexts_.size()?movement_contexts_[selected_].movement_class:0;
            // 452AD5 routes classes 3/4/5 through 452450's 12-centisecond wait.
            movement_interval_=movement_class>=3&&movement_class<=5?120:80;
            for(auto q:route){
                if(!step(units,selected_,q)){units[selected_]=saved;movement_poses_.clear();return false;}
                auto pose=units[selected_];pose.movement_pose=true;
                const auto& previous_pose=movement_poses_.empty()?saved:movement_poses_.back();
                pose.movement_frame=previous_pose.direction!=pose.direction?0:previous_pose.movement_frame+(movement_class==4||movement_class==5?0:1);
                movement_poses_.push_back(std::move(pose));
            }
            if(!movement_poses_.empty())units[selected_].movement_frame=movement_poses_.back().movement_frame;
            // 452450+2D0 skips destination AP writeback for class5.
            if(movement_class==5){
                auto preserve_budget=[&](auto& unit){unit.action_points=saved.action_points;unit.max_action_points=saved.max_action_points;unit.carry_points=saved.carry_points;};
                preserve_budget(units[selected_]);
                for(auto& pose:movement_poses_)preserve_budget(pose);
            }
            if(movement_animation_enabled_&&!movement_poses_.empty()){moving_unit_=selected_;movement_elapsed_=0;}
            moved_=true;state_=State::command;path_.clear();message_="Moved; choose attack or finish, or cancel to undo";return true;
        }
        if(state_==State::attack_target&&hit>=0&&can_attack(units,selected_,hit)){target_=hit;return activate(units);}
        return false;
    }
    bool has_movement_mouse()const{return state_==State::movement&&mouse_position_.x>=0&&!movement_mouse_[0].empty();}
    bool draw_movement_mouse(std::vector<uint8_t>& image)const{
        if(!has_movement_mouse()||effect_palette_.size()<768)return false;
        unsigned sprite=0;
        if(valid(hovered_cell_)){
            size_t cell=size_t(hovered_cell_.y)*width_+hovered_cell_.x;
            if(cell<previous_.size()&&previous_[cell]>=0)sprite=1+movement_categories_[cell];
        }
        const auto& pixels=movement_mouse_[sprite];auto size=movement_mouse_sizes_[sprite];
        if(pixels.empty())return false;
        for(int y=0;y<size.y;++y)for(int x=0;x<size.x;++x){
            unsigned c=pixels[size_t(y)*size.x+x];int dx=mouse_position_.x+x,dy=mouse_position_.y+y;
            if(!c||dx<0||dy<0||dx>=640||dy>=480)continue;
            size_t p=(size_t(dy)*640+dx)*4;
            for(unsigned k=0;k<3;++k)image.at(p+k)=uint8_t(unsigned(effect_palette_[c*3+k])*255/63);
        }
        return true;
    }
    void draw_selection(std::vector<uint8_t>& image,const std::vector<OriginalEvents::Unit>& units,int cx,int cy,int layer=-1)const{
        if(image.size()<640*480*4||effect_palette_.size()<768)return;
        auto blit=[&](const std::vector<uint8_t>& tile,Point cell){
            if(tile.empty())return;auto c=screen_center(cell,cx,cy);int height=int(tile.size()/64);
            // sub_42B0C6 bottom-aligns both 32- and 137-row resources.
            for(int y=0;y<height;++y)for(int x=0;x<64;++x){
                unsigned color=tile[size_t(y)*64+x];int dx=c.x-32+x,dy=c.y+16-height+y;
                if(!color||dx<0||dy<0||dx>=640||dy>=480)continue;
                size_t dst=(size_t(dy)*640+dx)*4;
                for(unsigned k=0;k<3;++k)image[dst+k]=uint8_t(unsigned(effect_palette_[color*3+k])*255/63);
            }
        };
        if(state_==State::movement&&layer!=1){
            for(size_t i=0;i<distances_.size();++i){
                if(previous_[i]<0)continue;
                blit(movement_effects_[movement_categories_[i]],{int(i%width_),int(i/width_)});
            }
        }
        Point cursor=hovered_cell_;
        if(selected_>=0&&size_t(selected_)<units.size())cursor={units[selected_].x,units[selected_].y};
        if(!valid(cursor))return;
        if(selected_<0){
            auto surface=cell(cursor);
            if(surface.elevation<=0||surface.elevation>20||surface.ground==1)return;
            bool simple=std::find(sealed_cells_.begin(),sealed_cells_.end(),cursor)!=sealed_cells_.end();
            if(simple){if(layer!=1)blit(ground_cursor_,cursor);return;}
        }
        // Original back/front cursor masks; scene occlusion belongs to the scene renderer.
        if(layer<0)for(const auto& frame:selection_frames_)blit(frame,cursor);
        else if(layer<2)blit(selection_frames_[size_t(layer)],cursor);
    }
};
}