#pragma once
#include "original_events.hpp"
#include "original_campaign.hpp"
#include "original_menu.hpp"
#include "original_disc.hpp"
#include "flic.hpp"
#include <optional>
namespace s2ne {
// 41E9CF resource order and timed presentation; no save flag is manufactured.
class OriginalEnding {
    OriginalMenu font_;
    std::vector<uint8_t> text_,pal_,cel_,prg_,background_,intro_;
    void class_image(std::vector<uint8_t>& image,unsigned appearance)const{
        auto group=[&](unsigned id){return size_t(dword(pal_,808+8*(64+id-1))-10-(id-1));};
        size_t index=group(1),frames=group(2);if(!appearance||appearance>word(prg_,index))throw std::runtime_error("Invalid ending class appearance");
        unsigned first=word(prg_,index+2+(appearance-1)*8+2);size_t table=frames+2+first*7;
        size_t p=frames+dword(prg_,table),end=p+word(prg_,table+4);unsigned w=word(prg_,p)+1,h=word(prg_,p+2)+1;p+=4;size_t n=0;
        while(p<end){unsigned op=prg_.at(p++),count=op?op&127:1,c=0;if(op&&!(op&128))c=prg_.at(p++);while(count--){unsigned ink=op&128?prg_.at(p++):c;if(n>=size_t(w)*h)throw std::runtime_error("Invalid ending class image");int x=225-int(w/2)+int(n%w),y=180-int(h)+int(n/w);++n;if(ink&&x>=0&&x<640&&y>=0&&y<480){size_t q=(size_t(y)*640+x)*4;for(unsigned k=0;k<3;++k)image[q+k]=uint8_t(unsigned(pal_.at(ink*3+k))*255/63);image[q+3]=255;}}}
        if(n!=size_t(w)*h)throw std::runtime_error("Truncated ending class image");
    }
    OriginalEvents events_;
    std::vector<unsigned> roles_;
    size_t page_{};unsigned variant_{};
    std::vector<std::vector<uint8_t>> illustrations_;
    std::vector<unsigned> illustration_pages_;
    unsigned scroll_{},elapsed_{},movie_frames_{};
    std::optional<Flic> movie_;
    std::filesystem::path root_;
    OriginalDisc disc_;
    std::vector<uint8_t> movie_image_;
    struct Page {int role{-1};int illustration{-1};unsigned movie{},text{};bool relationship{},summary{};};
    std::vector<Page> sequence_;
    void enter(){elapsed_=0;movie_.reset();movie_frames_=0;movie_image_.clear();if(done())return;const auto& p=sequence_[page_];if(p.illustration>=0)scroll_=unsigned(illustrations_.at(p.illustration).size()/(640*4))-480;if(p.movie){movie_.emplace(Flic::from_disc(root_,20+p.movie,disc_));movie_->delay=std::max(10u,(movie_->delay*10/48)*10);}}
    bool scrolling()const{return std::find(illustration_pages_.begin(),illustration_pages_.end(),unsigned(page_))!=illustration_pages_.end();}
    static unsigned word(const auto& b,size_t p){if(p>=b.size()||b.size()-p<2)throw std::runtime_error("Ending word offset="+std::to_string(p)+" size="+std::to_string(b.size()));return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static std::vector<uint8_t> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing native ending resource");return {std::istreambuf_iterator<char>(f),{}};}
    std::vector<uint8_t> string(size_t p)const{std::vector<uint8_t> result;while(text_.at(p))result.push_back(text_.at(p++));return result;}
    std::vector<uint8_t> pcx(const std::filesystem::path& root,unsigned id,OriginalDisc disc){
        auto b=OriginalDiscResources(root).dat_entry(disc,id);if(b.size()<897||b[0]!=10||b[2]!=1||b[3]!=8||b[65]!=1||word(b,8)!=639||word(b,10)<479)throw std::runtime_error("Unsupported native ending PCX");
        size_t colors=b.size()-768;if(b.at(colors-1)!=12)throw std::runtime_error("Missing ending palette");
                unsigned stride=word(b,66),height=word(b,10)+1;if(stride<640)throw std::runtime_error("Invalid ending PCX stride");size_t p=128,n=0;std::vector<uint8_t> out(size_t(640)*height*4);
                while(n<size_t(stride)*height){unsigned c=b.at(p++),count=1;if((c&192)==192){count=c&63;c=b.at(p++);}if(!count||count>size_t(stride)*height-n||p>=colors)throw std::runtime_error("Invalid ending PCX run");while(count--){size_t x=n%stride,y=n/stride;++n;if(x>=640)continue;size_t q=(y*640+x)*4;for(unsigned k=0;k<3;++k)out[q+k]=b.at(colors+c*3+k);out[q+3]=255;}}
        return out;
    }
    void paragraph(std::vector<uint8_t>& image,const std::vector<uint8_t>& bytes,int x,int y,int width)const{
        font_.draw_original_wrapped(image,bytes,x,y,175,20,unsigned(width/28),28);
    }
    void role_text(std::vector<uint8_t>& image,const std::vector<uint8_t>& text)const{
        size_t p=0;for(unsigned row=0;row<30&&p<text.size();++row){int compensation=0;for(unsigned cell=0,units=1;units<50&&p<text.size();++cell){unsigned c=text[p++];if(!c||c==10)return;if(c=='*'){if(!cell)--cell;else break;continue;}std::vector<uint8_t> glyph{uint8_t(std::max(c,30u))};if(c>128){glyph.push_back(text.at(p++));units+=2;}else ++units;int x=120+int(cell)*16-compensation; font_.draw_small(image,glyph,x,230+int(row)*20,175,true);if(c<=128)compensation+=8;}}
    }
public:
    OriginalEnding(const std::filesystem::path& root,const OriginalEvents& events,OriginalDisc disc):font_(root),events_(events),root_(root),disc_((events.stage==61||events.stage==131)?OriginalDisc::two:disc){
        disc=disc_;
        if((events.stage!=61&&events.stage!=131)||OriginalCampaign(root).selected_route(events)!=0||!events.flags[12])throw std::runtime_error("Native terminal outcome not authorized");
        pal_=read(root/"SUPERSP.PAL");cel_=read(root/"SUPERSP.CEL");prg_=read(root/"SUPERSP.PRG");auto bin=read(root/"SUPERSP.BIN");size_t at=848+16;size_t offset=dword(pal_,at)-13,size=dword(pal_,at+4)-13;if(offset>bin.size()||size>bin.size()-offset)throw std::runtime_error("Invalid BIN3 ending text");text_.assign(bin.begin()+offset,bin.begin()+offset+size);
        for(unsigned i=0;i<8;++i)if(events.flags[71+i]){variant_=i;break;}
        constexpr std::array<unsigned,13> order{11,1,22,2,3,4,14,15,18,32,16,17,42};
        auto append=[&](unsigned id){if(std::any_of(events.units.begin(),events.units.end(),[&](auto& u){return u.id==id&&!u.hostile();}))roles_.push_back(id);};
        for(auto id:order)append(id);for(unsigned id=1;id<=36;++id)if(std::find(order.begin(),order.end(),id)==order.end())append(id);
        background_=pcx(root,26,disc);intro_=pcx(root,40,disc);
        sequence_.push_back(Page{.text=0});sequence_.push_back(Page{.movie=4});
        sequence_.push_back(Page{.text=1});sequence_.push_back(Page{.movie=5});
        for(size_t r=0;r<roles_.size();++r){auto it=std::find(order.begin(),order.end(),roles_[r]);if(it!=order.end()){
            illustration_pages_.push_back(unsigned(sequence_.size()));
            illustrations_.push_back(pcx(root,27+unsigned(it-order.begin()),disc));
            sequence_.push_back(Page{.illustration=int(illustrations_.size()-1)});
        }sequence_.push_back(Page{.role=int(r)});sequence_.push_back(Page{.role=int(r),.relationship=true});}
        sequence_.push_back(Page{.summary=true});sequence_.push_back(Page{.text=2});sequence_.push_back(Page{.movie=3});enter();
    }
    size_t page()const{return page_;}size_t pages()const{return sequence_.size();}unsigned variant()const{return variant_;}bool done()const{return page_>=pages();}
    bool automatic()const{return !done()&&(movie_.has_value()||scrolling());}
    unsigned movie_frames()const{return movie_frames_;}
    std::string phase_description()const{if(done())return "returned";const auto& p=sequence_[page_];if(p.movie)return "movie"+std::to_string(p.movie)+" frames="+std::to_string(movie_?movie_->frames:0)+" delay="+std::to_string(movie_?movie_->delay:0);if(p.illustration>=0)return "41F011 scroll rows="+std::to_string(illustrations_[p.illustration].size()/(640*4))+" step=3/10ms";if(p.summary)return "41EDD4 campaign statistics";if(p.role>=0)return "41F167 role="+std::to_string(roles_[p.role])+(p.relationship?" relationships":" class/stat/text");return "BIN3 text="+std::to_string(p.text);}
    void tick(unsigned milliseconds){
        if(!automatic())return;elapsed_+=milliseconds;
        if(movie_){while(elapsed_>=movie_->delay){elapsed_-=movie_->delay;if(!movie_->next(movie_image_)){++page_;enter();break;}++movie_frames_;}}
        else{while(scroll_&&elapsed_>=10){elapsed_-=10;scroll_=scroll_>3?scroll_-3:0;}if(!scroll_&&elapsed_>=500){++page_;enter();}}
    }
    void advance(){if(!done()&&!automatic()){++page_;enter();}}
    std::vector<uint8_t> draw()const{
        auto image=background_;if(done())return image;const auto& phase=sequence_[page_];
        if(phase.movie)return movie_image_.empty()?std::vector<uint8_t>(640*480*4):movie_image_;
        if(phase.illustration>=0){const auto& source=illustrations_[phase.illustration];std::copy_n(source.begin()+size_t(scroll_)*640*4,image.size(),image.begin());return image;}
        unsigned count=word(text_,0);size_t records=4,variants=records+count*6;
        if(phase.role<0&&!phase.summary){image=phase.text<2?intro_:background_;paragraph(image,string(dword(text_,records+phase.text*6)),phase.text<2?25:60,phase.text==1?120:50,590);return image;}
        if(phase.summary){paragraph(image,font_.text_bytes(258),70,50,300);
            if(!events_.ending_statistics)throw std::runtime_error("Native ending campaign statistics unavailable");
            unsigned total=0;for(auto id:roles_){auto it=std::find_if(events_.units.begin(),events_.units.end(),[&](const auto& u){return !u.hostile()&&u.id==id;});total+=it->ending_kills;}
            const std::array<int,6> y{50,78,106,130,158,186};for(size_t i=0;i<6;++i){auto number=std::to_string(i<5?(*events_.ending_statistics)[i]:int(total));font_.draw_original_large(image,{number.begin(),number.end()},400,y[i],175);}return image;}
        auto id=roles_.at(phase.role);const auto& unit=*std::find_if(events_.units.begin(),events_.units.end(),[&](auto& u){return u.id==id&&!u.hostile();});
        if(!phase.relationship){
            class_image(image,unit.appearance);
            font_.draw_original_large(image,unit.name,367,42,175);
            auto title=unit.profession;auto level=" "+std::to_string(unit.level)+" ";title.insert(title.end(),level.begin(),level.end());auto suffix=font_.text_bytes(113);title.insert(title.end(),suffix.begin(),suffix.end());font_.draw_original_large(image,title,367,72,175);
            auto number=[&](unsigned label,int value,int y){auto text=font_.text_bytes(label);auto n=std::to_string(value);text.insert(text.end(),n.begin(),n.end());font_.draw_original_large(image,text,367,y,175);};
            if(!unit.ending_combat_value)throw std::runtime_error("Native ending cached combat statistic unavailable");number(28,*unit.ending_combat_value,102);number(256,int(unit.ending_kills),132);number(257,int(unit.ending_losses),162);
            size_t list=dword(text_,variants+variant_*8);unsigned n=word(text_,variants+variant_*8+4);
            for(unsigned i=0;i<n;++i){size_t p=list+i*8;if(word(text_,p)==id){role_text(image,string(dword(text_,p+2)));break;}}
            if(unit.portrait){size_t g=dword(pal_,824)-58,p=g+dword(cel_,g+2+(unit.portrait*2-2)*8);unsigned type=cel_.at(p++),w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
                for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;){unsigned op=type?0x81:cel_.at(p++),n=op?op&127:1,c=0;if(!n||n>w-x)throw std::runtime_error("Invalid ending portrait");if(op&&!(op&128))c=cel_.at(p++);while(n--){unsigned ink=op&128?cel_.at(p++):c;int dx=53+int(x++),dy=76+int(y);if(ink&&dx>=0&&dy>=0&&dx<640&&dy<480){size_t q=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image[q+k]=uint8_t(unsigned(pal_.at(ink*3+k))*255/63);}}}
            }
        }else{
            auto title=unit.name;title.push_back(' ');auto label=font_.text_bytes(261);title.insert(title.end(),label.begin(),label.end());font_.draw_original_large(image,title,34,220,175);
            unsigned row=0;for(const auto& other:events_.units){if(other.hostile()||other.id==id||other.id<1||other.id>40)continue;int x=row%2?320:34,y=250+int(row/2)*26;auto name=other.name;std::string value=":"+std::to_string(unit.relationships[other.id-1]);name.insert(name.end(),value.begin(),value.end());font_.draw_original_large(image,name,x,y,175);if(++row>=16)break;}
        }
        return image;
    }
};
}
