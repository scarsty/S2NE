#pragma once
#include "original_confirmation.hpp"
#include "original_settings.hpp"
#include <memory>

namespace s2ne {
// UI/state only. World mutation and the modal operations below belong to the host.
class OriginalBattleMenu {
public:
    enum class Page { root, overview, system, confirmation, settings, operation, closed };
    enum class Action { none, conditions, inspect_characters, inspect_map, load, save };
    struct Context {
        unsigned byte_4f6f4d=0;
        int dword_4f7cd0=0;
        bool saves_present=false;
    };
    struct Request { Action action=Action::none; unsigned address=0; int eax=0,edx=0,ebx=0; unsigned eax_sprite=0; };
    struct Result { bool changed=false; unsigned consumed_mask=0; bool rebuild_mask_surfaces=false,debug_scene_requested=false; };
    static constexpr unsigned entry_address=0x431DE8;
    static constexpr std::array<unsigned,6> root_ids{121,122,123,124,125,126};
    static constexpr std::array<unsigned,2> overview_ids{156,157};
    static constexpr std::array<unsigned,4> system_ids{128,129,130,131};
private:
    std::filesystem::path root_;
    OriginalMenu font_;
    std::vector<uint8_t> pal_,cel_;
    Context context_;
    OriginalSettings::State settings_state_;
    Page page_=Page::root, confirmation_parent_=Page::root, operation_parent_=Page::root;
    int selected_=0,root_selected_=0,system_selected_=0;
    int left_=0,top_=0,confirmation_code_=0;
    unsigned confirmation_id_=0;
    Request request_;
    std::optional<int> return_code_;
    std::unique_ptr<OriginalConfirmation> confirmation_;
    std::unique_ptr<OriginalSettings> settings_;
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    static std::vector<uint8_t> read(const std::filesystem::path& p){
        std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing battlefield menu resource");
        return {std::istreambuf_iterator<char>(f),{}};
    }
    size_t sprite_offset(unsigned id)const {
        size_t g=dword(pal_,816)-57;return g+dword(cel_,g+2+(id-142)*8);
    }
    void sprite(std::vector<uint8_t>& image,unsigned id,int left,int top,bool transparent)const {
        size_t p=sprite_offset(id);unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;){
            unsigned op=cel_.at(p++),n=op?op&127:1,value=0;
            if(!n||n>w-x)throw std::runtime_error("Invalid battlefield menu sprite run");
            if(op&&!(op&128))value=cel_.at(p++);
            while(n--){unsigned c=op&128?cel_.at(p++):value;int dx=left+int(x++),dy=top+int(y);
                if((transparent&&!(op&128)&&!c)||dx<0||dx>=640||dy<0||dy>=480)continue;
                size_t q=(size_t(dy)*640+dx)*4;
                for(unsigned k=0;k<3;++k)image.at(q+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);
                image.at(q+3)=255;
            }
        }
    }
    void close(int code){page_=Page::closed;return_code_=code;request_={};}
    void operation(Action action,unsigned address,int eax=0,int edx=0,int ebx=0){
        operation_parent_=page_;request_={action,address,eax,edx,ebx};page_=Page::operation;
    }
    void ask(unsigned id,int code,int x,int y,bool forced){
        confirmation_parent_=page_;confirmation_id_=id;confirmation_code_=code;
        if(!settings_state_.confirm&&!forced){close(code);return;}
        confirmation_=std::make_unique<OriginalConfirmation>(root_,font_,id,x,y,true);page_=Page::confirmation;
    }
    void finish_confirmation(){
        if(!confirmation_->result())return;
        bool yes=*confirmation_->result();confirmation_.reset();
        if(yes)close(confirmation_code_);else page_=confirmation_parent_;
    }
    unsigned label_id(int row,Page page)const {
        if(page==Page::root)return root_ids.at(row);
        if(page==Page::overview)return overview_ids.at(row);
        return system_ids.at(row);
    }
    void draw_page(std::vector<uint8_t>& image,Page page,int selection)const {
        if(page==Page::root){
            sprite(image,194,left_,top_,true);
            if(context_.byte_4f6f4d==2)sprite(image,196,left_+145,top_+293,true);
            for(int row=0;row<(context_.byte_4f6f4d==2?6:5);++row){
                auto bytes=font_.text_bytes(root_ids[row]);
                font_.draw_original_large(image,bytes,320-int(bytes.size())*6,top_+140+row*31,row==selection?119:233);
            }
        }else{
            OriginalPalette(pal_).transform(image,0,480,6);
            sprite(image,page==Page::overview?201:200,submenu_left(),submenu_top(),true);
            unsigned title=page==Page::overview?122:127;
            auto bytes=font_.text_bytes(title);
            int x=page==Page::overview?320-int(bytes.size())*6:submenu_left()+36;
            font_.draw_original_large(image,bytes,x,submenu_top()+46,75);
            int count=page==Page::overview?2:4;
            for(int row=0;row<count;++row)font_.draw_original_large(image,font_.text_bytes(label_id(row,page)),submenu_left()+12,submenu_top()+77+31*row,row==selection?119:233);
        }
    }
public:
    OriginalBattleMenu(const std::filesystem::path& root,Context context,OriginalSettings::State state)
        :root_(root),font_(root),pal_(read(root/"SUPERSP.PAL")),cel_(read(root/"SUPERSP.CEL")),context_(context),settings_state_(state){
        pal_.at(0)=pal_.at(1)=pal_.at(2)=0;font_.small_font=state.small_font!=0;
        size_t p=sprite_offset(194);left_=320-int(word(cel_,p)+1)/2;top_=240-int(word(cel_,p+2)+1)/2;
    }
    Page page()const{return page_;}
    int selected()const{return selected_;}
    int rows()const{return page_==Page::root?(context_.byte_4f6f4d==2?6:5):page_==Page::overview?2:page_==Page::system?4:0;}
    int left()const{return left_;} int top()const{return top_;}
    int submenu_left()const{return left_+135;} int submenu_top()const{return top_+94;}
    const Request& request()const{return request_;}
    std::optional<int> return_code()const{return return_code_;}
    const OriginalSettings::State& settings_state()const{return settings_state_;}
    unsigned confirmation_id()const{return confirmation_id_;}
    void saves_present(bool present){context_.saves_present=present;}
    // Host return values are original callee results, not booleans except inspect_characters.
    void complete_operation(int code=0){
        if(page_!=Page::operation)throw std::logic_error("No battlefield menu operation pending");
        Action action=request_.action;request_={};page_=operation_parent_;
        if(action==Action::load&&code!=0)close(code);
        else if(action==Action::save&&code==9)close(code);
        else if(action==Action::inspect_characters&&code==1)close(1);
    }
    int hit(int x,int y)const {
        if(page_!=Page::root&&page_!=Page::overview&&page_!=Page::system)return -1;
        int x0=page_==Page::root?left_+145:submenu_left()+7;
        int x1=page_==Page::root?left_+245:submenu_left()+111;
        int y0=page_==Page::root?top_+139:submenu_top()+78;
        if(x<x0||x>x1)return -1;
        for(int row=0;row<rows();++row)if(y>=y0+31*row&&y<=y0+31*row+26)return row;
        return -1;
    }
    void move(int delta){int count=rows();if(count)selected_=((selected_+delta)%count+count)%count;}
    void cancel(){
        if(page_==Page::root)close(0);
        else if(page_==Page::overview||page_==Page::system){page_=Page::root;selected_=root_selected_;}
        else if(page_==Page::confirmation){confirmation_->cancel();finish_confirmation();}
    }
    void activate(){
        if(page_==Page::root){
            root_selected_=selected_;
            switch(selected_){
            case 0:if(context_.dword_4f7cd0!=1)operation(Action::conditions,0x43260D);break;
            case 1:page_=Page::overview;selected_=0;break;
            case 2:page_=Page::system;selected_=0;break;
            case 3:ask(45,6,212,top_+259,false);break;
            case 4:ask(44,3,212,top_+290,false);break;
            case 5:ask(46,15,212,top_+321,false);break;
            }
        }else if(page_==Page::overview){
            if(selected_==0)operation(Action::inspect_characters,0x43471C,0);
            else operation(Action::inspect_map,0x4352CE);
        }else if(page_==Page::system){
            system_selected_=selected_;
            switch(selected_){
            case 0:if(context_.saves_present){operation(Action::load,0x4354C0,0,0,0);request_.eax_sprite=204;}break;
            case 1:operation(Action::save,0x4354C0,0,1,0);request_.eax_sprite=204;break;
            case 2:settings_=std::make_unique<OriginalSettings>(root_,1,settings_state_);page_=Page::settings;break;
            case 3:ask(51,11,submenu_left()-44,submenu_top()+172,true);break;
            }
        }else if(page_==Page::confirmation){confirmation_->confirm();finish_confirmation();}
    }
    Result event(const OriginalSettings::Event& e){
        if(page_==Page::settings){auto r=settings_->event(e);settings_state_=settings_->state();font_.small_font=settings_state_.small_font!=0;
            if(r.closed){settings_.reset();page_=Page::system;selected_=system_selected_;}return {r.changed||r.closed,r.consumed_mask,r.rebuild_mask_surfaces,r.debug_scene_requested};}
        if(page_==Page::operation||page_==Page::closed)return {};
        if(page_==Page::confirmation){
            if(e.type==1||e.type==2){if(e.scan==1)cancel();else if(e.scan==0x4B||e.scan==0x4D)confirmation_->toggle();else if(e.scan==0x1C||e.scan==0x39)activate();return {true,4};}
            if(e.type==8||e.type==0x20){if(e.buttons&2)cancel();else{confirmation_->mouse(float(e.x),float(e.y),(e.buttons&1)!=0);finish_confirmation();}return {true,e.buttons?0x10u:0u};}
            return {};
        }
        if(e.type==1||e.type==2){
            if(e.scan==1){cancel();return {true,4};}
            if(e.scan==0x48||e.scan==0x50){move(e.scan==0x48?-1:1);return {true,0};}
            if(e.scan==0x1C||e.scan==0x39){activate();return {true,0x1F};}
        }else if(e.type==8||e.type==0x20){
            int row=hit(e.x,e.y);bool changed=row>=0&&row!=selected_;
            bool two_click=page_==Page::overview;
            if(row>=0)selected_=row;
            if((e.buttons&1)&&row>=0&&(!two_click||!changed)){activate();return {true,0x1F};}
            if(e.buttons&2){cancel();return {true,0x10};}
            return {changed,0};
        }
        return {};
    }
    std::vector<uint8_t> draw(const std::vector<uint8_t>& battlefield)const {
        if(battlefield.size()!=640*480*4)throw std::runtime_error("Battlefield menu requires 640x480 RGBA");
        if(page_==Page::closed)return battlefield;
        auto image=battlefield;draw_page(image,Page::root,page_==Page::root?selected_:root_selected_);
        Page visible=page_==Page::confirmation?confirmation_parent_:page_==Page::operation?operation_parent_:page_;
        if(visible==Page::overview||visible==Page::system||visible==Page::settings)draw_page(image,visible==Page::settings?Page::system:visible,visible==Page::settings?system_selected_:selected_);
        if(page_==Page::settings)return settings_->draw(image);
        if(page_==Page::confirmation)confirmation_->draw(image,font_);
        return image;
    }
};
}
