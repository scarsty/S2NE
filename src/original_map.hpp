#pragma once
#include "original_events.hpp"
#include <algorithm>
#include <map>

namespace s2ne {
class OriginalMap {
    std::vector<uint8_t> prg_,pal_,pixels_;
    struct Image { unsigned w,h; std::vector<uint8_t> pixels; };
    std::map<size_t,Image> cache_;
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    size_t group(unsigned id)const{return dword(pal_,808+8*(64+id-1))-10-(id-1);}
    size_t item(unsigned archive,unsigned index)const{
        size_t g=group(archive);
        if(index>=word(prg_,g))throw std::runtime_error("Invalid original map sprite index");
        return g+dword(prg_,g+2+index*8);
    }
    const Image& raw(size_t offset){
        if(cache_.contains(offset))return cache_.at(offset);
        Image image{word(prg_,offset)+1,word(prg_,offset+2)+1,{}};
        if(image.w>8192||image.h>8192)throw std::runtime_error("Invalid original map sprite dimensions");
        if(offset+4>prg_.size()||size_t(image.w)*image.h>prg_.size()-offset-4)throw std::runtime_error("Truncated original map sprite");
        image.pixels.assign(prg_.begin()+offset+4,prg_.begin()+offset+4+size_t(image.w)*image.h);
        return cache_.emplace(offset,std::move(image)).first->second;
    }
    void blit(const Image& image,int x,int y,bool transparent){
        for(unsigned row=0;row<image.h;++row)for(unsigned col=0;col<image.w;++col){
            int dx=x+int(col),dy=y+int(row);if(dx<0||dy<0||dx>=int(width*64)||dy>=int(height*32))continue;
            unsigned c=image.pixels.at(size_t(row)*image.w+col);
            if(transparent&&!c)continue;
            pixels_.at(size_t(dy)*width*64+dx)=uint8_t(c);
        }
    }
public:
    unsigned width{},height{},map_id{};
    int camera_x{},camera_y{};
    explicit OriginalMap(const std::filesystem::path& root){
        auto read=[](const auto& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing original map archive");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        prg_=read(root/"SUPERSP.PRG");pal_=read(root/"SUPERSP.PAL");
    }
    void load(unsigned id){
        map_id=id;size_t m=item(6,id-1);
        width=word(prg_,m);height=word(prg_,m+2);
        if(width<10||height<15||width>256||height>256||word(prg_,m+4)!=width*height*2)throw std::runtime_error("Invalid original staggered map geometry");
        pixels_.assign(size_t(width)*64*height*32,0);
        size_t cells=m+28;
        size_t scenery=cells+size_t(width)*height*10;
        size_t lookups=scenery+word(prg_,m+12)*10+word(prg_,m+14)*6;
        std::array<std::vector<unsigned>,3> mapping;
        for(unsigned a=0;a<3;++a){unsigned used=word(prg_,lookups);lookups+=2;unsigned count=word(prg_,group(3+a));for(unsigned i=0;i<count;++i)mapping[a].push_back(word(prg_,lookups+i*2));lookups+=count*2+used*8;}
        auto mapped=[&](unsigned layer,unsigned local)->unsigned{
            if(!local||local>mapping[layer].size()||!mapping[layer][local-1])throw std::runtime_error("Missing original map tile mapping");
            return mapping[layer][local-1]-1;
        };
        for(unsigned y=0;y<height*2;++y){
            for(unsigned i=0;i<word(prg_,m+12);++i){
                size_t p=scenery+i*10;
                int sx=int(int16_t(word(prg_,p))),sy=int(int16_t(word(prg_,p+2)));
                int sw=int(int16_t(word(prg_,p+4))),sh=int(int16_t(word(prg_,p+6)));
                if(sy*16+sh!=int(y*16))continue;
                blit(raw(item(5,word(prg_,p+8)-1)),sx*64+(sy&1)*32,sy*16,true);
            }
            for(unsigned x=0;x<width-(y&1);++x){
                size_t p=cells+(size_t(y)*width+x)*5;
                unsigned elevation=prg_.at(p),ground=word(prg_,p+1),object=word(prg_,p+3);
                if(!elevation)continue;
                const auto& tile=raw(item(3,ground-1));
                unsigned layers=ground<=5?1:elevation;
                // 0x42E89C passes zero to 0x4753A4: diamond padding must preserve earlier rows.
                for(unsigned z=0;z<layers;++z)blit(tile,int(x*64+(y&1)*32),int(y*16)-int(z*16),true);
                if(object){const auto& obj=raw(item(4,object-1));blit(obj,int(x*64+(y&1)*32),int(y*16)-int((layers-1)*16)-64,true);}
            }
        }
        camera_x=32;camera_y=16;
    }
    void draw_units(std::vector<uint8_t>& image,const std::vector<OriginalEvents::Unit>& units)const{
        size_t index=group(1),frames=group(2);unsigned count=word(prg_,index);
        auto sorted=units;std::stable_sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.y<b.y;});
        for(const auto& unit:sorted){
            if(unit.first_turn>1||(unit.last_turn!=0&&unit.last_turn<1)||!unit.appearance||unit.appearance>count)continue;
            size_t metadata=index+2+(unit.appearance-1)*8;
            unsigned first=word(prg_,metadata+2);
            size_t animation=index+2+count*8+(unit.appearance-1)*200;
            unsigned frame=prg_.at(animation);
            size_t table=frames+2+(first+frame-1)*7;
            size_t p=frames+dword(prg_,table);unsigned length=word(prg_,table+4);
            unsigned w=word(prg_,p)+1,h=word(prg_,p+2)+1;
            if(w>512||h>512||length<4)throw std::runtime_error("Invalid original unit sprite");
            std::vector<uint8_t> decoded;decoded.reserve(size_t(w)*h);size_t end=p+length;p+=4;
            while(p<end&&decoded.size()<size_t(w)*h){unsigned code=prg_.at(p++),n=code&127;
                if(!code){decoded.push_back(0);continue;}
                if(!n)throw std::runtime_error("Invalid original unit run");
                if(code&128){while(n--)decoded.push_back(prg_.at(p++));}else{unsigned c=prg_.at(p++);decoded.insert(decoded.end(),n,uint8_t(c));}
            }
            if(decoded.size()!=size_t(w)*h)throw std::runtime_error("Truncated original unit sprite");
            int left=unit.x*64+(unit.y&1)*32+32-int(w/2)-camera_x;
            int top=unit.y*16+32-int(h)-camera_y;
            for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned c=decoded.at(size_t(y)*w+x);int dx=left+int(x),dy=top+int(y);if(!c||dx<0||dy<0||dx>=640||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(c*3+k))*255/63);}
        }
    }
    void pan(int dx,int dy){camera_x=std::clamp(camera_x+dx,0,int(width*64)-640);camera_y=std::clamp(camera_y+dy,0,int(height*32)-480);}
    void center(int x,int y){camera_x=std::clamp(x*64+(y&1)*32-320,0,int(width*64)-640);camera_y=std::clamp(y*16-240,0,int(height*32)-480);}
    std::vector<uint8_t> draw()const{
        std::vector<uint8_t> image(640*480*4);
        for(int y=0;y<480;++y)for(int x=0;x<640;++x){unsigned c=pixels_.at(size_t(camera_y+y)*width*64+camera_x+x);size_t dst=(size_t(y)*640+x)*4;for(unsigned k=0;k<3;++k)image[dst+k]=uint8_t(unsigned(pal_.at(c*3+k))*255/63);image[dst+3]=255;}
        return image;
    }
};
}
