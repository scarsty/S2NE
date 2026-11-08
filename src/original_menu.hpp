#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
namespace s2ne {
class OriginalMenu {
    std::vector<uint8_t> bin, pal;
    size_t strings{}, index{}, glyphs{};
    unsigned count{};
    unsigned word(size_t p) const { return bin.at(p)|(unsigned(bin.at(p+1))<<8); }
    unsigned dword(size_t p) const { return word(p)|(word(p+2)<<16); }
    size_t entry(unsigned id) const {
        size_t p=808+5*8+(id-1)*8;
        unsigned value=pal.at(p)|(unsigned(pal.at(p+1))<<8)|(unsigned(pal.at(p+2))<<16)|(unsigned(pal.at(p+3))<<24);
        return value-11-(id-1);
    }
public:
    int selected=0;
    int activated=-1;
    static constexpr std::array<unsigned,5> ids{212,213,214,123,131};
    static constexpr std::array<unsigned,5> targets{0x40E53C,0x4371E6,0x40EBF3,0x434103,0x43183B};
    explicit OriginalMenu(const std::filesystem::path& root){
        auto read=[](auto p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing menu resources");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        bin=read(root/"SUPERSP.BIN");pal=read(root/"SUPERSP.PAL");
        strings=entry(5);count=word(strings);index=entry(11);glyphs=entry(13);
        for(unsigned id:ids){if(id>=count)throw std::runtime_error("Missing original menu string");}
    }
    void move(int delta){selected=std::clamp(selected+delta,0,4);activated=-1;}
    int hit(float x,float y) const {return x>=266 && x<=386 && y>=283 && y<433?int(y-283)/30:-1;}
    bool mouse(float x,float y,bool click){int row=hit(x,y);if(row<0)return false;bool same=row==selected;selected=row;activated=-1;return click&&same;}
    unsigned activate(){activated=selected;return targets.at(selected);}
    void draw_text(std::vector<uint8_t>& image, unsigned id, int x, int y, unsigned color) const {
        if(id>=count)throw std::runtime_error("Invalid original string ID");
        size_t text=strings+2+count*8+dword(strings+2+id*8);
        std::vector<uint8_t> bytes;while(bin.at(text))bytes.push_back(bin.at(text++));
        draw_big5(image,bytes,x,y,color);
    }
    void draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
                   int x, int y, unsigned color) const {
        for(size_t p=0;p<text.size();) {
            unsigned lead=text.at(p++),code=0;
            if(lead<=0x80) {
                size_t bits=entry(16)+std::max(30u,lead)*48;
                for(int gy=0;gy<24;++gy)for(int gx=0;gx<16;++gx){
                    if(!(bin.at(bits+gy*2+gx/8)&(128>>(gx%8))))continue;
                    if(x+gx<0||x+gx>=640||y+gy<0||y+gy>=480)continue;
                    size_t dst=(size_t(y+gy)*640+x+gx)*4;
                    for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(unsigned(pal.at(color*3+c))*255/63);
                    image.at(dst+3)=255;
                }
                x+=16;continue;
            }
            unsigned trail=text.at(p++);
            if(lead==0xC6 && trail>=0xA1) code=0x8001+trail-0xA1;
            else {
                unsigned base=0,first=0;
                if(lead>=0xA1&&lead<=0xA3){base=0x8400;first=0xA1;}
                else if(lead>=0xA4&&lead<=0xC6){base=0x8800;first=0xA4;}
                else if(lead>=0xC7&&lead<=0xC8){base=0x805F;first=0xC7;}
                else if(lead>=0xC9&&lead<=0xF9){base=0x9D19;first=0xC9;}
                else throw std::runtime_error("Invalid original Big5 lead");
                if(!((trail>=0x40&&trail<=0x7E)||(trail>=0xA1&&trail<=0xFE)))throw std::runtime_error("Invalid original Big5 trail");
                code=base+(lead-first)*157+trail-(trail<=0x7E?0x40:0x62);
            }
            unsigned glyph=word(index+(code-0x8000)*2);
            if(!glyph)throw std::runtime_error("Missing original dialogue glyph");
            size_t bits=glyphs+(glyph-1)*72;
            for(int gy=0;gy<24;++gy)for(int gx=0;gx<24;++gx){
                if(!(bin.at(bits+gy*3+gx/8)&(128>>(gx%8))))continue;
                if(x+gx<0||x+gx>=640||y+gy<0||y+gy>=480)continue;
                size_t dst=(size_t(y+gy)*640+x+gx)*4;
                for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(std::min(255u,unsigned(pal.at(color*3+c))*255/63));
                image.at(dst+3)=255;
            }
            x+=24;
        }
    }
    void draw(std::vector<uint8_t>& image) const {
        for(unsigned row=0;row<5;++row){
            size_t text=strings+2+count*8+dword(strings+2+ids[row]*8);
            int x=278,y=286+int(row)*30;
            unsigned color=int(row)==selected?2:15;
            for(unsigned letter=0;letter<4;++letter){
                unsigned lead=bin.at(text++),trail=bin.at(text++);
                if(lead<0xA4||lead>0xC6)throw std::runtime_error("Unsupported menu Big5 range");
                unsigned code=0x8800+(lead-0xA4)*157+(trail<=0x7E?trail-0x40:trail-0x62);
                unsigned glyph=word(index+(code-0x8000)*2);
                if(!glyph)throw std::runtime_error("Missing menu glyph");
                size_t bits=glyphs+(glyph-1)*72;
                for(int gy=0;gy<24;++gy)for(int gx=0;gx<24;++gx){
                    if(!(bin.at(bits+gy*3+gx/8)&(128>>(gx%8))))continue;
                    size_t dst=(size_t(y+gy)*640+x+gx)*4;
                    for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(std::min(255u,unsigned(pal.at(color*3+c))*255/63));
                    image.at(dst+3)=255;
                }x+=24;
            }
        }
    }
};
}
