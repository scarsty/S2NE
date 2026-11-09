#pragma once
#include "original_menu.hpp"
#include "title_image.hpp"
namespace s2ne {
class NewGameSetup {
    std::vector<uint8_t> dat,pal,backdrop_;
    size_t group{};
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    void sprite(std::vector<uint8_t>& image,unsigned id,int left,int top)const {
        size_t offset=group+dword(dat,group+2+(id-1)*8), size=dword(dat,group+6+(id-1)*8);
        if(offset>dat.size()||size>dat.size()-offset||size<5)throw std::runtime_error("Invalid setup sprite");
        int w=word(dat,offset+1)+1,h=word(dat,offset+3)+1;
        size_t p=offset+5,end=offset+size;
        if(dat.at(offset)!=0)throw std::runtime_error("Unsupported setup sprite encoding");
        for(int y=0;y<h;++y){int x=0;while(x<w){if(p>=end)throw std::runtime_error("Truncated setup sprite");unsigned code=dat.at(p++),n=code&127;
            if(!code){++x;continue;}if(!n||n>unsigned(w-x))throw std::runtime_error("Invalid setup run");
            unsigned repeat=0;if(!(code&128)){if(p>=end)throw std::runtime_error("Truncated setup repeat");repeat=dat.at(p++);}
            while(n--){if((code&128)&&p>=end)throw std::runtime_error("Truncated setup literal");unsigned color=(code&128)?dat.at(p++):repeat;
                int dx=left+x++,dy=top+y;if(dx<0||dx>=640||dy<0||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;
                // Only zero repeat runs are transparent; literal runs copy palette index zero.
                if(!(code&128)&&!color)continue;
                for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(std::min(255u,unsigned(pal.at(color*3+c))*255/63));image.at(dst+3)=255;}
        }}
    }
public:
    int difficulty=2;
    unsigned confirmed=0;
    explicit NewGameSetup(const std::filesystem::path& root){
        auto read=[](auto p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing setup resource");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        dat=read(root/"cd"/"windows-disc1"/"SUPERSP"/"SUPERSP.DAT");pal=read(root/"SUPERSP.PAL");
        // 425FE5 forces the global transparent/black entry to zero.
        pal.at(0)=pal.at(1)=pal.at(2)=0;
        group=dword(pal,808+8*(5+18+6))-14;
        backdrop_=load_title(root,20,false);
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
        // 0x40E31E restores DAT 20, not the bright DAT 19 used by the title menu.
        auto image=backdrop_;
        sprite(image,4,173,223);
        for(int x: {9,169,329,489})sprite(image,3,x,453);
        font.draw_original_large(image,font.text_bytes(215),295,237,115);
        font.draw_original_large(image,font.text_bytes(221),319,410,107);
        for(unsigned i=0;i<5;++i){
            bool selected=i==unsigned(difficulty);
            font.draw_original_large(image,font.text_bytes(216+i),280,267+int(i)*28,selected?60:52,selected?std::optional<unsigned>(108):std::nullopt);
        }
        // sub_40E53C, loc_40E691 / loc_40E796; original small-font footer.
        static constexpr std::array<const char*,5> rate{"5%","25%","50%","80%","100%"};
        static constexpr std::array<const char*,5> modifier{"-20%","-10%","0%","+10%","+20%"};
        static constexpr std::array<const char*,5> reward{"300%","200%","100%","100%","100%"};
        for(unsigned i=0;i<4;++i)font.draw_small(image,font.text_bytes(222+i),18+int(i)*160,457,113,true);
        auto ascii=[&](const char* text,int x){std::vector<uint8_t> bytes;while(*text)bytes.push_back(uint8_t(*text++));font.draw_small(image,bytes,x,457,113,true);};
        ascii(rate.at(difficulty),110);
        font.draw_small(image,font.text_bytes(difficulty==0?229:230),270,457,113,true);
        ascii(modifier.at(difficulty),430);ascii(reward.at(difficulty),590);
        // sub_475970 fills the original 18x25 selection swatch, not a substitute cursor.
        for(int y=266+difficulty*28;y<=290+difficulty*28;++y)for(int x=249;x<=266;++x){
            size_t p=(size_t(y)*640+x)*4;
            for(unsigned c=0;c<3;++c)image[p+c]=uint8_t(unsigned(pal.at(60*3+c))*255/63);
            image[p+3]=255;
        }
        // 40E8C3..40E8F2 presents the whole indexed frame with byte_503DB8.
        // Undo the font/sprite renderer's 6-bit expansion before applying that DAC.
        for(size_t p=0;p<image.size();p+=4)for(unsigned c=0;c<3;++c)
            image[p+c]=opening_palette_channel(uint8_t((unsigned(image[p+c])*63+254)/255));
        return image;
    }
};
}
