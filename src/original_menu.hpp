#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <optional>
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
    bool small_font=false;
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
    std::vector<uint8_t> text_bytes(unsigned id) const {
        if(id>=count)throw std::runtime_error("Invalid original string ID");
        size_t text=strings+2+count*8+dword(strings+2+id*8);
        std::vector<uint8_t> bytes;while(bin.at(text))bytes.push_back(bin.at(text++));
        return bytes;
    }
    void draw_text(std::vector<uint8_t>& image, unsigned id, int x, int y, unsigned color) const {
        draw_big5(image,text_bytes(id),x,y,color);
    }
    // sub_422402: BIN 12 contains 16x16 Big5 glyphs; BIN 15 contains 8x15 ASCII.
    void draw_small(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y,unsigned color,bool original_padding=false)const {
        for(size_t p=0;p<text.size();){
            unsigned lead=text.at(p++);if(!lead||lead=='\n')break;
            unsigned width=8,height=15;size_t bits=entry(15)+std::max(30u,lead)*15;
            if(lead>128){
                unsigned trail=text.at(p++),base=0,first=0;
                if(lead==0xC6&&trail>=0xA1){base=0x8001;first=lead;trail-=0xA1;}
                else {
                    if(lead>=0xA1&&lead<=0xA3){base=0x8400;first=0xA1;}
                    else if(lead>=0xA4&&lead<=0xC6){base=0x8800;first=0xA4;}
                    else if(lead>=0xC7&&lead<=0xC8){base=0x805F;first=0xC7;}
                    else if(lead>=0xC9&&lead<=0xF9){base=0x9D19;first=0xC9;}
                    else throw std::runtime_error("Invalid small-font Big5 lead");
                    if(!((trail>=0x40&&trail<=0x7E)||(trail>=0xA1&&trail<=0xFE)))throw std::runtime_error("Invalid small-font Big5 trail");
                    trail-=trail<=0x7E?0x40:0x62;
                }
                unsigned glyph=word(index+(base+(lead-first)*157+trail-0x8000)*2);
                if(!glyph)throw std::runtime_error("Missing small-font glyph");
                bits=entry(12)+(glyph-1)*32;width=height=16;
            }
            for(unsigned gy=0;gy<height;++gy)for(unsigned gx=0;gx<width;++gx){
                if(!(bin.at(bits+gy*(width/8)+gx/8)&(128>>(gx%8))))continue;
                int dx=x+int(gx)+(original_padding?1:0),dy=y+int(gy)+(original_padding&&height==15?1:0);if(dx<0||dx>=640||dy<0||dy>=480)continue;
                size_t dst=(size_t(dy)*640+dx)*4;
                for(unsigned c=0;c<3;++c)image.at(dst+c)=uint8_t(std::min(255u,unsigned(pal.at(color*3+c))*255/63));
                image.at(dst+3)=255;
            }
            x+=int(width);
        }
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
    void draw_original_large(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y,unsigned color,std::optional<unsigned> shadow=std::nullopt)const {
        for(size_t p=0;p<text.size();){
            unsigned lead=text.at(p++);if(!lead||lead=='\n')break;
            std::vector<uint8_t> glyph{uint8_t(lead)};
            if(lead>128)glyph.push_back(text.at(p++));
            int ink_x=x+(lead>128?1:3),ink_y=y+(lead>128?0:1);
            if(small_font){
                int gx=x+(lead>128?4:2),gy=y+4;
                if(shadow)draw_small(image,glyph,gx+1,gy+1,*shadow,true);
                draw_small(image,glyph,gx,gy,color,true);
            }else{
                if(shadow)draw_big5(image,glyph,ink_x+1,ink_y+1,*shadow);
                draw_big5(image,glyph,ink_x,ink_y,color);
            }
            x+=lead>128?24:16;
        }
    }
    void draw_original_wrapped(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y,unsigned color,unsigned rows,unsigned columns,int row_height,std::optional<unsigned> shadow=std::nullopt)const {
        // 423AC5 uses 28-pixel cells, compensating eight pixels after each ASCII.
        size_t p=0;
        for(unsigned row=0;row<rows;++row){
            int compensation=0;
            for(int cell=0,units=1;units<int(columns*2);++cell){
                if(p>=text.size()||!text[p]||text[p]=='\n')return;
                unsigned lead=text[p++];
                if(lead=='*'){if(cell==0)--cell;else break;continue;}
                std::vector<uint8_t> glyph{uint8_t(std::max(lead,30u))};
                if(lead>128){glyph.push_back(text.at(p++));units+=2;}
                else ++units;
                int gx=x+cell*(small_font?20:28)-compensation,gy=y+int(row)*row_height;
                if(small_font){
                    if(shadow)draw_small(image,glyph,gx+1,gy+5,*shadow,true);
                    draw_small(image,glyph,gx,gy+4,color,true);
                }else{
                    gx+=1;gy+=lead<=128?1:0;
                    if(shadow)draw_big5(image,glyph,gx+1,gy+1,*shadow);
                    draw_big5(image,glyph,gx,gy,color);
                }
                if(lead<=128)compensation+=8;
            }
        }
    }
    void draw_banner_ascii(std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int x,int y)const {
        // 4239AD -> 4235AE: ASCII x+2, padded glyph (1,1), shadow offset (1,1).
        for(uint8_t letter:text){
            draw_big5(image,{letter},x+4,y+2,36);
            draw_big5(image,{letter},x+3,y+1,33);
            x+=16;
        }
    }
    void draw(std::vector<uint8_t>& image) const {
        // 40DFD6/40E057: both normal and selected rows use style1, edge3.
        for(unsigned row=0;row<5;++row)
            draw_original_large(image,text_bytes(ids[row]),278,286+int(row)*30,int(row)==selected?2:15,3);
    }
};
}
