#pragma once
#include "original_shop_menu.hpp"
#include "original_map.hpp"
namespace s2ne {
// 44D675's217-panel: ten original rows, selected role portrait and native standing bitmap.
class OriginalRolePanel {
    OriginalMenu font_;
    OriginalShopMenu graphics_;
    mutable OriginalMap map_;
    std::vector<std::vector<uint8_t>> skill_names_;
public:
    explicit OriginalRolePanel(const std::filesystem::path& root):font_(root),graphics_(root),map_(root){
        std::ifstream f(root/"SUPERSP.BIN",std::ios::binary),pf(root/"SUPERSP.PAL",std::ios::binary);std::vector<uint8_t> b{std::istreambuf_iterator<char>(f),{}},pal{std::istreambuf_iterator<char>(pf),{}};size_t base=0;for(unsigned i=0;i<4;++i)base|=size_t(pal.at(856+i))<<(8*i);base-=12;b.erase(b.begin(),b.begin()+base);auto word=[&](size_t p){return unsigned(b.at(p))|(unsigned(b.at(p+1))<<8);};unsigned count=b.at(1),all=count+b.at(8);size_t offsets=9+2*count+37*b.at(7),strings=offsets+2*all;
        for(unsigned i=0;i<count;++i){size_t p=strings+(i?word(offsets+2*i):0),end=p;while(b.at(end))++end;skill_names_.emplace_back(b.begin()+p,b.begin()+end);}
    }
    int top()const{return 480-graphics_.panel_height(217);}
    void draw(std::vector<uint8_t>& image,const OriginalEvents& events,std::span<const size_t> roles,size_t first,size_t row)const{
        int y=top();graphics_.panel(image,217,0,y);if(roles.empty())return;
        const auto& u=events.units.at(roles[first+row]);
        graphics_.role_portrait(image,u.portrait,27,y+30);
        auto units=std::vector<OriginalEvents::Unit>{u};units[0].hp=std::max(1,u.hp);units[0].status_flags|=0x80;units[0].status_flags&=~0x20u;units[0].animation_action=0;units[0].action_state=0;units[0].x=1;units[0].y=1;units[0].first_turn=1;units[0].last_turn=0;
        map_.load(events.map_id);auto scratch=map_.draw();map_.draw_units(scratch,units,1,0);
        auto bounds=map_.unit_bitmap_bounds(0);const auto& pixels=map_.unit_source_bitmap(0);graphics_.role_bitmap(image,pixels,bounds[2],bounds[3],545-bounds[2]/2,y+139-bounds[3]);
        auto text=[&](const auto& value,int x,int yy,unsigned color){font_.draw_original_large(image,value,x,y+yy,color);};
        text(u.name,72-int(u.name.size())*6,165,170);text(u.profession,72-int(u.profession.size())*6,195,170);
        std::array<int,10> values{(u.status_flags&0x20)?0:u.hp,u.mp,u.st,u.action_points,u.attack,u.defense,u.strikeback,u.sp,u.tactics,u.fencing};
        std::array<std::vector<uint8_t>,10> labels{{{'H','P'},{'M','P'},{'S','T'},{'A','P'},font_.text_bytes(7),font_.text_bytes(8),font_.text_bytes(10),font_.text_bytes(11),font_.text_bytes(9),font_.text_bytes(12)}};
        for(unsigned i=0;i<10;++i){font_.draw_small(image,labels[i],137,y+20+20*int(i),56,true);auto n=std::to_string(values[i]);font_.draw_small(image,{n.begin(),n.end()},175,y+20+20*int(i),105,true);}
        for(size_t i=0;i<10&&first+i<roles.size();++i){const auto& role=events.units.at(roles[first+i]);font_.draw_small(image,role.name,256,y+24+20*int(i),i==row?56:177,true);font_.draw_small(image,role.profession,349,y+24+20*int(i),i==row?56:140,true);}
        int sx=465,sy=145;for(size_t i=0;i<u.learned.size()&&i<skill_names_.size();++i)if(u.learned[i]){font_.draw_small(image,skill_names_[i],sx,y+sy,56,true);sy+=20;if(sy>205){sy=145;sx+=51;}}
    }
};
}
