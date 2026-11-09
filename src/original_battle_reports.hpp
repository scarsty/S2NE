#pragma once
#include "original_map.hpp"
#include "original_unit_status.hpp"
#include "original_settings.hpp"
#include <functional>

namespace s2ne {
class OriginalBattleReports {
public:
    enum class Page { conditions, roster, map, detail, proficiencies, closed };
    struct Entry { OriginalEvents::Unit unit; bool enemy=false; int runtime_kind=0; bool locate_blocked=false; };
    struct Result { bool changed=false; unsigned consumed_mask=0; };
    static constexpr unsigned conditions_address=0x43260D,roster_address=0x43471C,map_address=0x4352CE;
private:
    OriginalMenu font_;
    OriginalUnitStatus status_;
    std::vector<uint8_t> cel_,pal_,background_,mini_;
    std::vector<Entry> entries_;
    std::vector<uint8_t> stage_name_,victory_,defeat_;
    unsigned stage_=0;
    Page page_;
    bool close_to_battle_=false;
    int first_=0,row_=0,code_=0,mini_w_=0,mini_h_=0;
    std::optional<std::pair<int,int>> locate_;
    unsigned detail_address_=0;
    int friendly_count_=0,npc_count_=0,enemy_count_=0;
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static std::vector<uint8_t> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing battle report resource");return {std::istreambuf_iterator<char>(f),{}};}
    static std::vector<uint8_t> bytes(const std::string& s){return {s.begin(),s.end()};}
    void pixel(std::vector<uint8_t>& image,int x,int y,unsigned c)const {
        if(x<0||x>=640||y<0||y>=480)return;size_t p=(size_t(y)*640+x)*4;
        for(unsigned k=0;k<3;++k)image.at(p+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);image.at(p+3)=255;
    }
    void sprite(std::vector<uint8_t>& image,unsigned id,int left,int top,bool transparent=true)const {
        size_t g=dword(pal_,816)-57,p=g+dword(cel_,g+2+(id-142)*8);unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;){unsigned op=cel_.at(p++),n=op?op&127:1,c=0;
            if(!n||n>w-x)throw std::runtime_error("Invalid battle report sprite");if(op&&!(op&128))c=cel_.at(p++);
            while(n--){unsigned ink=op&128?cel_.at(p++):c;if(!transparent||(op&128)||ink)pixel(image,left+int(x),top+int(y),ink);++x;}}
    }
    void large(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y,unsigned color)const{font_.draw_original_large(image,text,x,y,color);}
    void small(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y,unsigned color)const{font_.draw_small(image,text,x,y,color,true);}
    void roster_row(std::vector<uint8_t>& image,int row,const Entry& entry)const {
        int y=5+row*118;sprite(image,206,5,y);
        std::vector<uint8_t> layer(640*480*4);status_.draw(font_,layer,entry.unit,0,entry.enemy);
        // 4584FF uses the first (large) portrait in the character's pair.
        if(entry.unit.portrait){size_t g=dword(pal_,824)-58,p=g+dword(cel_,g+2+(entry.unit.portrait*2-2)*8);
            unsigned type=cel_.at(p++),w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
            if(type>1||w>640||h>480)throw std::runtime_error("Invalid roster portrait");
            for(unsigned yy=0;yy<h;++yy)for(unsigned x=0;x<w;){unsigned op=type?0x81:cel_.at(p++),n=op?op&127:1,c=0;
                if(!n||n>w-x)throw std::runtime_error("Invalid roster portrait run");if(op&&!(op&128))c=cel_.at(p++);
                while(n--){unsigned ink=op&128?cel_.at(p++):c;if(ink)pixel(image,12+int(x),y+4+int(yy),ink);++x;}}}
        for(int yy=39;yy<111;++yy)for(int x=117;x<511;++x)if(layer[(size_t(yy)*640+x)*4+3])
            for(unsigned k=0;k<4;++k)image[(size_t(y+yy)*640+x+11)*4+k]=layer[(size_t(yy)*640+x)*4+k];
        if(!(entry.unit.status_flags&0x10))large(image,entry.unit.name,121,y+7,entry.enemy?60:167);
        auto title=entry.unit.profession;auto level=bytes(" "+std::to_string(entry.unit.level));title.insert(title.end(),level.begin(),level.end());auto suffix=font_.text_bytes(113);title.insert(title.end(),suffix.begin(),suffix.end());
        large(image,title,507-int(title.size())*12,y+7,179);
    }
    void build_mini(OriginalMap map){
        mini_w_=(int(map.width)*64-64)/3;mini_h_=(int(map.height)*32-32)/3;
        if(mini_w_<=0||mini_h_<=0||mini_w_>640||mini_h_>480)throw std::runtime_error("Original inspection map exceeds screen");
        mini_.resize(size_t(mini_w_)*mini_h_*4);
        int cached_x=-1,cached_y=-1;std::vector<uint8_t> tile;
        for(int y=0;y<mini_h_;++y)for(int x=0;x<mini_w_;++x){int sx=31+x*3,sy=16+y*3;
            int cx=std::min((sx/640)*640,int(map.width)*64-640),cy=std::min((sy/480)*480,int(map.height)*32-480);
            if(cx!=cached_x||cy!=cached_y){map.camera_x=cx;map.camera_y=cy;tile=map.draw();cached_x=cx;cached_y=cy;}
            size_t source=(size_t(sy-cy)*640+sx-cx)*4,dest=(size_t(y)*mini_w_+x)*4;for(unsigned k=0;k<4;++k)mini_[dest+k]=tile[source+k];}
    }
public:
    // Entries must retain the original two army arrays' order; no hostility reclassification.
    OriginalBattleReports(const std::filesystem::path& root,Page page,const OriginalEvents& events,
        std::vector<Entry> entries,const std::vector<uint8_t>& background,const OriginalMap* map=nullptr,bool small_font=false,int dword_4f7cd0=0)
        :font_(root),status_(root),cel_(read(root/"SUPERSP.CEL")),pal_(read(root/"SUPERSP.PAL")),background_(background),
         stage_name_(events.stage_name),victory_(events.victory_objective),defeat_(events.defeat_objective),stage_(events.stage),page_(page){
        if(background.size()!=640*480*4)throw std::runtime_error("Battle report requires640x480 RGBA");
        pal_[0]=pal_[1]=pal_[2]=0;font_.small_font=small_font;
        for(auto& entry:entries){unsigned flags=entry.unit.status_flags;if((flags&0x20)||!(flags&0x80))continue;
            if(entry.enemy)++enemy_count_;else if(entry.runtime_kind==1)++npc_count_;else ++friendly_count_;
            entries_.push_back(std::move(entry));}
        if(page==Page::roster&&entries_.empty())page_=Page::closed;
        if(page==Page::conditions&&dword_4f7cd0==1)page_=Page::closed;
        if(page==Page::map){if(!map)throw std::runtime_error("Map inspection requires OriginalMap");build_mini(*map);}
    }
    Page page()const{return page_;}int first()const{return first_;}int selected_row()const{return row_;}
    size_t size()const{return entries_.size();}int return_code()const{return code_;}
    std::optional<std::pair<int,int>> location()const{return locate_;}
    const Entry& selected()const{return entries_.at(size_t(first_+row_));}
    unsigned detail_address()const{return detail_address_;}
    void close_to_battle(bool enabled=true){close_to_battle_=enabled;}
    void close_detail(){if(page_==Page::proficiencies){page_=Page::detail;detail_address_=0x455C7B;}else if(page_==Page::detail){page_=close_to_battle_?Page::closed:Page::roster;detail_address_=0;}}
    void proficiencies(){if(page_==Page::detail){page_=Page::proficiencies;detail_address_=0x456BC0;}}
    void detail(){if(page_==Page::roster){detail_address_=0x455C7B;page_=Page::detail;}}
    void locate(){if(page_==Page::roster&&!selected().locate_blocked){locate_={{selected().unit.x,selected().unit.y}};code_=1;page_=Page::closed;}}
    void navigate(unsigned scan){
        if(page_!=Page::roster)return;int count=int(entries_.size());
        switch(scan){
        case 0x48:if(row_>0)--row_;else if(first_>0)--first_;break;
        case 0x50:if(first_+row_<count-1){if(row_<3)++row_;else ++first_;}break;
        case 0x49:if(first_>0)first_=std::max(0,first_-4);else row_=0;break;
        case 0x51:if(first_+row_+8<count-1)first_+=4;else if(first_+4<=count-1)first_=count-4;else row_=count-first_-1;break;
        case 0x47:first_=row_=0;break;
        case 0x4F:if(count-4>first_){first_=count-4;row_=0;}break;
        }
    }
    int hit(int x,int y)const {
        if(x>=600||page_!=Page::roster)return -1;
        for(int i=0;i<4&&first_+i<int(entries_.size());++i)if(y>=5+i*118&&y<=122+i*118)return i;
        return -1;
    }
    Result event(const OriginalSettings::Event& e){
        if(page_==Page::closed)return {};
        bool key=e.type==1||e.type==2,mouse=e.type==8||e.type==0x20;
        if(page_==Page::conditions||page_==Page::map){if((key&&e.scan)||(mouse&&e.buttons)){page_=Page::closed;return {true,0x1F};}return {};}
        if(page_==Page::proficiencies){if((key&&e.scan)||(mouse&&e.buttons)){close_detail();return {true,0x1F};}return {};}
        if(page_==Page::detail){if((key&&e.scan==1)||(mouse&&(e.buttons&2))){close_detail();return {true,0x1F};}return {};}
        if(key){if(e.scan==1){page_=Page::closed;return {true,4};}if(e.scan==0x39){detail();return {true,4};}if(e.scan==0x1C){locate();return {true,4};}navigate(e.scan);return {true,0};}
        if(mouse){int row=hit(e.x,e.y);if(row>=0)row_=row;
            if(e.buttons&1){if(e.x>=616&&e.x<=635){if(e.y>=6&&e.y<=29)navigate(0x48);else if(e.y>=452&&e.y<=475)navigate(0x50);else if(e.y>=31&&e.y<=450){int n=int(size());first_=std::max(0,std::min((e.y-31)*n/419,n-4));row_=std::min(e.y>=422?3:0,n-first_-1);}}
                else if(row>=0&&e.y>=9+row*118&&e.y<=119+row*118){if(e.x>=12&&e.x<=104)detail();else if(e.x>=520&&e.x<600)locate();}return {true,0x10};}
            if(e.buttons&2){page_=Page::closed;return {true,0x10};}return {row>=0,0};}
        return {};
    }
    std::vector<uint8_t> draw()const {
        auto image=background_;if(page_==Page::closed)return image;
        if(page_==Page::map){int left=320-mini_w_/2,top=240-mini_h_/2;for(int y=0;y<mini_h_;++y)for(int x=0;x<mini_w_;++x)for(unsigned k=0;k<4;++k)image[(size_t(top+y)*640+left+x)*4+k]=mini_[(size_t(y)*mini_w_+x)*4+k];return image;}
        if(page_==Page::conditions){int left=10,top=52;sprite(image,207,left,top);
            large(image,font_.text_bytes(47),left+320-int(font_.text_bytes(47).size())*6,top+17,57);
            large(image,font_.text_bytes(48),left+90,top+57,54);
            auto title=bytes(std::to_string(stage_)+".");title.insert(title.end(),stage_name_.begin(),stage_name_.end());large(image,title,left+213,top+57,54);
            large(image,font_.text_bytes(49),left+90,top+108,119);large(image,font_.text_bytes(50),left+90,top+208,87);
            font_.draw_original_wrapped(image,victory_,left+213,top+108,119,4,11,28,42);
            font_.draw_original_wrapped(image,defeat_,left+213,top+208,87,4,11,28,42);
            const std::array<int,3> xs{86,264,436},numbers{friendly_count_,npc_count_,enemy_count_};
            large(image,font_.text_bytes(19),left+xs[0],top+330,40);large(image,bytes("NPC"),left+xs[1],top+330,38);large(image,font_.text_bytes(20),left+xs[2],top+330,60);
            for(int i=0;i<3;++i)large(image,bytes(std::to_string(numbers[i])),left+144+176*i,top+330,185);return image;
        }
        sprite(image,205,0,0,false);for(int i=0;i<4&&first_+i<int(size());++i)roster_row(image,i,entries_[first_+i]);
        for(int y=9+row_*118;y<=118+row_*118;++y)for(int x=12;x<=104;++x)if(x==12||x==104||y==9+row_*118||y==118+row_*118)pixel(image,x,y,60);
        int thumb=31;if(first_+row_>0)thumb+=(first_+row_+1)*391/int(size());sprite(image,147,616,thumb,true);
        if(page_==Page::detail||page_==Page::proficiencies){sprite(image,206,5,363);status_.draw(font_,image,selected().unit,363,selected().enemy);}
        if(page_==Page::proficiencies){
            sprite(image,232,0,343);auto title=font_.text_bytes(159);large(image,title,320-int(title.size())*6,351,73);
            large(image,font_.text_bytes(42),135,351,73);large(image,font_.text_bytes(160),454,351,73);
            const auto& u=selected().unit;
            auto value=[&](unsigned n,int x,int y){small(image,bytes(std::to_string(n)),x,y,137);};
            for(unsigned i=0;i<6;++i){int x=i<5?340:490,y=i<5?385+int(i)*16:385;
                small(image,font_.text_bytes(172+i),x,y,233);value(u.weapon_proficiency[i],x+80,y);}
            for(unsigned i=0;i<9;++i){int x=i<5?20:165,y=385+int(i<5?i:i-5)*16;
                small(image,font_.text_bytes(161+i),x,y,233);value(u.learned_proficiency[i]/10,x+96,y);}
            small(image,font_.text_bytes(170),490,433,233);value(u.learned_proficiency[9]/10,570,433);
            small(image,font_.text_bytes(171),490,449,233);value(u.learned_proficiency[10]/10,570,449);
        }
        return image;
    }
};
}
