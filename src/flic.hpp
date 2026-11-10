#pragma once
#include "original_disc.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <filesystem>
namespace s2ne {
class Flic {
    std::vector<uint8_t> data, pixels;
    std::array<uint8_t, 768> palette{};
    size_t position = 128, first_position_=128, cursor{}, limit{};
    unsigned frame{},chunk_type_{};
    uint8_t byte() { if (cursor >= limit) throw std::runtime_error("Truncated FLIC chunk frame="+std::to_string(frame)+" cursor="+std::to_string(cursor)+" limit="+std::to_string(limit)+" type="+std::to_string(chunk_type_)); return data.at(cursor++); }
    unsigned word() { unsigned n=byte(); return n | (unsigned(byte())<<8); }
    void put(int x,int y,uint8_t value) {
        if(x<0 || y<0 || x>=width || y>=height) throw std::runtime_error("Invalid FLIC pixel position");
        pixels[size_t(y)*width+x]=value;
    }
public:
    int width{}, height{};
    unsigned frames{}, delay{};
    explicit Flic(const std::filesystem::path& path) : Flic(read(path)) {}
    static std::vector<uint8_t> read(const std::filesystem::path& path) {
        std::ifstream in(path,std::ios::binary);
        if(!in) throw std::runtime_error("Missing intro resource");
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(in),{});
    }
    static Flic from_disc(const std::filesystem::path& root, unsigned id,OriginalDisc disc=OriginalDisc::one) {
        if(id<21||id>25) throw std::runtime_error("Invalid original movie resource ID");
        auto bytes=OriginalDiscResources(root).dat_entry(disc,id);
        auto get16=[](const auto& b,size_t p){return unsigned(b.at(p))|(unsigned(b.at(p+1))<<8);};
        auto get32=[&](const auto& b,size_t p){return get16(b,p)|(get16(b,p+2)<<16);};
        if(bytes.size()<128) throw std::runtime_error("Invalid opening archive entry");
        auto put16=[&](size_t p,unsigned value){bytes.at(p)=uint8_t(value);bytes.at(p+1)=uint8_t(value>>8);};
        auto put32=[&](size_t p,unsigned value){put16(p,value);put16(p+2,value>>16);};
        unsigned a=get32(bytes,0),b=get32(bytes,80);put32(0,b);put32(80,a);
        a=get16(bytes,6);b=get16(bytes,14);put16(6,b);put16(14,a);
        put16(4,get16(bytes,4)-0x923);put16(8,get16(bytes,8)-56);
        put16(10,get16(bytes,10)-11);put16(12,get16(bytes,12)-8);
        return Flic(std::move(bytes));
    }
    explicit Flic(std::vector<uint8_t> bytes) : data(std::move(bytes)) {
        cursor=4;limit=data.size();
        if(word()!=0xAF12) throw std::runtime_error("Unsupported FLIC type");
        frames=word();width=word();height=word();
        if(word()!=8 || width!=640 || height<1 || height>480 || !frames) throw std::runtime_error("Unsupported FLIC dimensions");
        cursor=16;delay=word();delay|=word()<<16;
        if(!delay || delay>1000) throw std::runtime_error("Invalid FLIC frame delay");
        cursor=80;unsigned first=word();first|=word()<<16;
        if(first>=128&&first<data.size())position=first;first_position_=position;
        pixels.resize(size_t(width)*height);
    }
    void rewind(){position=first_position_;frame=0;std::fill(pixels.begin(),pixels.end(),0);palette.fill(0);}
    bool next(std::vector<uint8_t>& rgba) {
        if(frame>=frames) return false;
        cursor=position;limit=data.size();
        size_t size=word();size|=size_t(word())<<16;
        if(size<16 || size>data.size()-position || word()!=0xF1FA) throw std::runtime_error("Invalid FLIC frame");
        unsigned chunks=word();size_t end=position+size;cursor=position+16;
        for(unsigned c=0;c<chunks;++c){
            size_t start=cursor;limit=end;
            size_t length=word();length|=size_t(word())<<16;unsigned type=word();
            if(length<6 || length>end-start) throw std::runtime_error("Invalid FLIC chunk length");
            limit=start+length;chunk_type_=type;
            if(type==4 || type==11){
                unsigned packets=word(),index=0;
                for(unsigned p=0;p<packets;++p){index+=byte();unsigned count=byte();if(!count)count=256;
                    if(index+count>256)throw std::runtime_error("Invalid FLIC palette");
                    for(unsigned i=0;i<count*3;++i){unsigned value=byte();palette[index*3+i]=uint8_t(type==11?value*255/63:value);}index+=count;
                }
            } else if(type==15){
                for(int y=0;y<height;++y){byte();int x=0;
                    while(x<width){int n=int8_t(byte());if(!n)throw std::runtime_error("Invalid empty FLIC BRUN run");if(n>=0){auto value=byte();while(n--)put(x++,y,value);}else{while(n++)put(x++,y,byte());}}
                    if(x!=width)throw std::runtime_error("Incomplete FLIC BRUN row");
                }
            } else if(type==7){
                unsigned lines=word();int y=0;
                while(lines){unsigned control=word();
                    if((control&0xC000)==0xC000){y-=int16_t(control);continue;}
                    if(control&0x8000){put(width-1,y,uint8_t(control));continue;}
                    int x=0;
                    for(unsigned p=0;p<control;++p){x+=byte();int n=int8_t(byte());
                        if(n>=0){while(n--){put(x++,y,byte());put(x++,y,byte());}}
                        else{uint8_t a=byte(),b=byte();while(n++){put(x++,y,a);put(x++,y,b);}}
                    }++y;--lines;
                }
            } else if(type==16){if(length!=6+size_t(width)*height&&length!=4+size_t(width)*height)throw std::runtime_error("Invalid native FLIC COPY size");limit=std::min(data.size(),cursor+size_t(width)*height);for(int y=0;y<height;++y)for(int x=0;x<width;++x)put(x,y,byte());}
            else if(type==13){std::fill(pixels.begin(),pixels.end(),0);}
            else if(type!=18)throw std::runtime_error("Unsupported FLIC chunk: "+std::to_string(type));
            cursor=start+length;
        }
        position=end;++frame;
        rgba.assign(640*480*4,0);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){size_t dst=(size_t(y+(480-height)/2)*640+x)*4;unsigned color=pixels[size_t(y)*width+x];
            for(int c=0;c<3;++c)rgba[dst+c]=palette[color*3+c];rgba[dst+3]=255;}
        return true;
    }
};
}
