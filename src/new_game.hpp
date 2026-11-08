#pragma once
#include "original_menu.hpp"
namespace s2ne {
class NewGameSetup {
    std::vector<uint8_t> dat,pal;
    size_t group{};
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    void sprite(std::vector<uint8_t>& image,unsigned id,int left,int top)const {
        size_t offset=group+dword(dat,group+2+(id-1)*8), size=dword(dat,group+6+(id-1)*8);
        if(offset>dat.size()||size>dat.size()-offset||size<5)throw std::runtime_error("Invalid setup sprite");
        int w=word(dat,offset+1)+1,h=word(dat,offset+3)+1;
        size_t p=offset+5,end=offset+size;
        for(int y=0;y<h;++y){int x=0;while(x<w){if(p>=end)throw std::runtime_error("Truncated setup sprite");unsigned code=dat.at(p++),n=code&127;
            if(!code){++x;continue;}if(!n||n>unsigned(w-x))throw std::runtime_error("Invalid setup run");
            unsigned repeat=0;if(!(code&128)){if(p>=end)throw std::runtime_error("Truncated setup repeat");repeat=dat.at(p++);}
            while(n--){if((code&128)&&p>=end)throw std::runtime_error("Truncated setup literal");unsigned color=(code&128)?dat.at(p++):repeat;
                int dx=left+x++,dy=top+y;if(dx<0||dx>=640||dy<0||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;
                for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(std::min(255u,unsigned(pal.at(color*3+c))*255/63));image.at(dst+3)=255;}
        }}
    }
public:
    int difficulty=2;
    unsigned confirmed=0;
    explicit NewGameSetup(const std::filesystem::path& root){
        auto read=[](auto p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing setup resource");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        dat=read(root/"cd"/"windows-disc1"/"SUPERSP"/"SUPERSP.DAT");pal=read(root/"SUPERSP.PAL");
        group=dword(pal,808+8*(5+18+6))-14;
    }
    void move(int d){difficulty=std::clamp(difficulty+d,0,4);}
    bool mouse(float x,float y,bool left_click){
        if(!left_click)return false;
        // 0x40EA75 handles button events only; hovering does not select a difficulty.
        if(x>=248&&x<=437&&y>=265&&y<405){difficulty=int(y-265)/28;return false;}
        if(x>=272&&x<=412&&y>=411&&y<=438){confirm();return true;}
        return false;
    }
    void confirm(){confirmed=unsigned(difficulty+1);}
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background,const OriginalMenu& font)const {
        auto image=background;
        sprite(image,4,173,223);
        for(int x: {9,169,329,489})sprite(image,3,x,453);
        font.draw_text(image,215,295,237,115);
        font.draw_text(image,221,319,410,107);
        for(unsigned i=0;i<5;++i)font.draw_text(image,216+i,280,267+int(i)*28,i==unsigned(difficulty)?60:52);
        return image;
    }
};
}
