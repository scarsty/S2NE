#pragma once
#include "original_events.hpp"
#include <algorithm>
#include <cmath>
#include <random>
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
    bool original_rules_=false;
    std::mt19937 random_{std::random_device{}()};
    int selected_=-1;
public:
    enum class State { idle, command, movement, attack_menu, attack_target };
    static constexpr std::array<unsigned,6> command_icons{158,160,162,164,188,166};
    // sub_43AE24 copies these six availability masks before packing the
    // surviving entries into the original 32-pixel command strip.
    static constexpr std::array<uint32_t,6> command_masks{
        0x80000000u, 0x40000000u, 0x20000000u,
        0x10000000u, 0x01000000u, 0x00080000u};
private:
    State state_=State::idle;
    int command_=0,target_=-1;
    bool moved_=false,acted_=false;
    OriginalEvents::Unit before_move_{};
    std::vector<int> distances_,previous_;
    std::vector<Point> path_;
    std::string message_="Select a unit to open its original command bar";
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    int roll(int n){return int(random_()%unsigned(n));}
public:
    Battle(unsigned w,unsigned h,std::vector<Cell> cells):width_(w),height_(h),cells_(std::move(cells)){
        if(!w||!h||cells_.size()!=size_t(w)*h)throw std::runtime_error("Invalid battle terrain");
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
        OriginalEvents events(root,1);result.config_=events.battle_config;result.original_rules_=true;
        return result;
    }
    bool valid(Point p)const{return p.x>=0&&p.y>=0&&p.y<int(height_)&&p.x<int(width_)-(p.y&1);}
    Cell cell(Point p)const{return valid(p)?cells_.at(size_t(p.y)*width_+p.x):Cell{};}
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
        uint32_t mask=command_masks[5]; // sub_43AE24 always retains finish.
        if (!moved_ && u.action_points>=config_[6]) {
            for(size_t i=0;i<units.size();++i)if(can_attack(units,selected_,i)){mask|=command_masks[0];break;}
        }
        if (!moved_ && !distances_.empty()) {
            for(int cost:distances_)if(cost!=std::numeric_limits<int>::max()&&cost>0){mask|=command_masks[1];break;}
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
    void cancel(){
        path_.clear();target_=-1;
        if(state_==State::movement||state_==State::attack_menu||state_==State::attack_target){state_=State::command;message_="Command cancelled; select another command";return;}
        selected_=-1;state_=State::idle;message_="Select a unit to open its original command bar";
    }
    void cancel(std::vector<OriginalEvents::Unit>& units){
        // 0x439E6D restores the saved position/AP when leaving a moved, uncommitted action.
        if(state_==State::command&&moved_&&!acted_&&selected_>=0){units.at(selected_)=before_move_;moved_=false;message_="Movement undone";return;}
        cancel();
    }
    static Point surface_center(Point p,Cell c){auto q=center(p);q.y-=std::max(0,c.elevation-1)*16;return q;}
    Point screen_center(Point p,int cx,int cy)const{auto q=surface_center(p,cell(p));return {q.x-cx,q.y-cy};}
    Point pick_surface(float x,float y,int cx,int cy)const{
        Point best{-1,-1};
        // 0x42A2BD / 0x42A3D2 resolves the visible elevated surface, not the flat row.
        for(int row=0;row<int(height_);++row)for(int col=0;col<int(width_)-(row&1);++col){Point p{col,row};if(cell(p).elevation<=0)continue;auto c=screen_center(p,cx,cy);if(std::abs(x-c.x)/32+std::abs(y-c.y)/16<=1)best=p;}
        return best;
    }
    Point command_origin(const std::vector<OriginalEvents::Unit>& units,int cx,int cy)const{
        if(selected_<0)return {-1,-1};auto c=screen_center({units[selected_].x,units[selected_].y},cx,cy);
        int count=state_==State::attack_menu?1:6;
        return {std::clamp(c.x-count*16,8,639-count*32),std::clamp(c.y+16,0,330)};
    }
    void build_routes(const std::vector<OriginalEvents::Unit>& units){
        distances_.assign(cells_.size(),std::numeric_limits<int>::max());previous_.assign(cells_.size(),-1);path_.clear();
        if(selected_<0)return;const auto& u=units[selected_];int start=u.y*int(width_)+u.x;
        using Node=std::pair<int,int>;std::priority_queue<Node,std::vector<Node>,std::greater<Node>> todo;
        distances_[start]=0;todo.push({0,start});int budget=original_rules_?std::min(110,u.action_points-config_[0]):110;
        while(!todo.empty()){auto [cost,index]=todo.top();todo.pop();if(cost!=distances_[index])continue;Point p{index%int(width_),index/int(width_)};
            for(int dy:{-1,1})for(int dx:{0,(p.y&1)?1:-1}){Point q{p.x+dx,p.y+dy};int step=step_cost(u,p,q);if(step<0||occupant(units,q)>=0||cost+step>budget)continue;int next=q.y*int(width_)+q.x;
                if(cost+step<distances_[next]){distances_[next]=cost+step;previous_[next]=index;todo.push({cost+step,next});}}
        }
    }
    bool preview(Point p){
        path_.clear();if(!valid(p)||previous_.empty())return false;int index=p.y*int(width_)+p.x;
        if(previous_[index]<0)return false;
        while(previous_[index]>=0){path_.push_back({index%int(width_),index/int(width_)});index=previous_[index];}
        std::reverse(path_.begin(),path_.end());return true;
    }
    bool can_attack(const std::vector<OriginalEvents::Unit>& units,size_t a,size_t b)const{
        if(a>=units.size()||b>=units.size()||a==b)return false;const auto& u=units[a];const auto& v=units[b];
        return u.present(turn_)&&v.present(turn_)&&u.hostile()!=v.hostile()&&adjacent({u.x,u.y},{v.x,v.y})&&cell({u.x,u.y}).elevation==cell({v.x,v.y}).elevation&&(!original_rules_||u.action_points>=config_[6]);
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
        if(state_==State::movement)preview(p);
        if(state_==State::attack_target){int hit=occupant(units,p);if(hit>=0&&can_attack(units,selected_,hit))target_=hit;}
    }
    bool mouse(std::vector<OriginalEvents::Unit>& units,float x,float y,bool pressed,int cx,int cy){
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
    bool step(std::vector<OriginalEvents::Unit>& units,size_t index,Point to){
        if(!can_step(units,index,to))return false;
        auto& u=units[index];if(original_rules_){u.action_points-=step_cost(u,{u.x,u.y},to);u.carry_points=std::min(u.carry_points,u.action_points);}
        u.x=to.x;u.y=to.y;message_="Moved; action points "+std::to_string(u.action_points)+" (Enter: finish side)";return true;
    }
    // sub_408CB7 / sub_40A622: equal-height base arithmetic only; class/status/skill modifiers remain pending.
    static int ordinary_damage(int attack,int defense,int divisor,int minimum_roll){
        if(divisor<=0)throw std::runtime_error("Invalid damage divisor");
        int damage=(attack-defense)*100/divisor;
        if(damage>100){if(damage<=200)damage=100+(damage-100)*90/100;else if(damage<=300)damage=190+(damage-200)*80/100;else if(damage<=400)damage=270+(damage-300)*70/100;else damage=340+(damage-400)*60/100;}
        if(damage<3)damage=minimum_roll<1?5:minimum_roll<6?4:minimum_roll<16?3:minimum_roll<46?2:1;
        return damage;
    }
    void seed(unsigned value){random_.seed(value);}
    unsigned turn()const{return turn_;}
    void reset_side(std::vector<OriginalEvents::Unit>& units,bool hostile){
        for(auto& u:units)if(u.present(turn_)&&u.hostile()==hostile){
            int carry=u.base_action_points>config_[5]?config_[5]:u.action_points;
            if(u.action_points+u.base_action_points>=110)carry=0;
            u.carry_points=carry;u.action_points=u.base_action_points+carry;
        }
    }
    // Basic contact response only; the original scored movement/skill AI is not substituted.
    void end_turn(std::vector<OriginalEvents::Unit>& units){
        reset_side(units,false);
        unsigned responses=0;
        for(size_t a=0;a<units.size();++a){
            if(!units[a].hostile()||!units[a].present(turn_))continue;
            for(size_t b=0;b<units.size();++b)if(units[b].present(turn_)&&!units[b].hostile()&&adjacent({units[a].x,units[a].y},{units[b].x,units[b].y})&&cell({units[a].x,units[a].y}).elevation==cell({units[b].x,units[b].y}).elevation){
                if(strike(units,a,b))++responses;break;
            }
        }
        reset_side(units,true);++turn_;selected_=-1;state_=State::idle;path_.clear();target_=-1;
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
        if(original_rules_){if(attacker.action_points<config_[6])return false;attacker.action_points-=config_[6];attacker.carry_points=std::min(attacker.carry_points,attacker.action_points);}
        int hit=std::clamp(attacker.tactics-defender.strikeback,1,100)*12/10;
        if(roll(100)>hit){message_="Ordinary melee: miss";return true;}
        int chance=roll(100),jitter=roll(10),base=chance<2?(jitter>=5?140:100):chance<10?(jitter>=5?135:105):chance<25?(jitter>=5?130:110):chance<50?(jitter>=5?125:115):120;
        int damage=ordinary_damage(attacker.attack,defender.defense,base+jitter,roll(100));
        defender.hp=std::max(0,defender.hp-damage);
        message_="Ordinary melee: -"+std::to_string(damage)+" HP; target HP "+std::to_string(defender.hp);
        return true;
    }
    bool click(std::vector<OriginalEvents::Unit>& units,Point p){
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
            for(auto q:route)if(!step(units,selected_,q)){units[selected_]=saved;return false;}
            moved_=true;state_=State::command;path_.clear();message_="Moved; choose attack or finish, or cancel to undo";return true;
        }
        if(state_==State::attack_target&&hit>=0&&can_attack(units,selected_,hit)){target_=hit;return activate(units);}
        return false;
    }
    void draw_selection(std::vector<uint8_t>& image,const std::vector<OriginalEvents::Unit>& units,int cx,int cy)const{
        if(selected_<0||size_t(selected_)>=units.size())return;
        auto pixel=[&](int x,int y,unsigned r,unsigned g,unsigned b,unsigned a=255){
            if(x<0||y<0||x>=640||y>=480)return;size_t p=(size_t(y)*640+x)*4;
            image[p]=uint8_t((image[p]*(255-a)+r*a)/255);image[p+1]=uint8_t((image[p+1]*(255-a)+g*a)/255);image[p+2]=uint8_t((image[p+2]*(255-a)+b*a)/255);
        };
        auto diamond=[&](Point point,unsigned r,unsigned g,unsigned b,unsigned a){
            auto c=screen_center(point,cx,cy);
            for(int x=-31;x<=31;++x){int y=15-std::abs(x)/2;for(int sign:{-1,1})pixel(c.x+x,c.y+sign*y,r,g,b,a);}
        };
        // The original movement renderer marks the reachable set, then the
        // current route and cursor cell. Keep these as thin overlays so terrain
        // and elevated sprites remain visible underneath (sub_452069/452B64).
        if(state_==State::movement){
            for(size_t i=0;i<distances_.size();++i)if(previous_[i]>=0)
                diamond({int(i%width_),int(i/width_)},0,120,200,128);
            for(auto p:path_)diamond(p,255,255,255,220);
        }
        diamond({units[selected_].x,units[selected_].y},255,255,0,230);
        if(state_==State::attack_target&&target_>=0)diamond({units[target_].x,units[target_].y},255,40,40,230);
    }
};
}
