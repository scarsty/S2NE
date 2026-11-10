#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "runtime_error.hpp"
#include <algorithm>
#include <unordered_map>

namespace s2ne {
class OriginalPalette {
    struct Node { std::array<int,3> rgb{};int color=-1,left=257,right=257; };
    std::array<Node,258> nodes_{};
    std::array<std::array<uint8_t,256>,13> maps_{};
    const std::vector<uint8_t>& palette_;
    std::array<unsigned,3> cycle_offsets_{};
    std::array<uint64_t,3> cycle_times_{};
    int nearest(int red,int green,int blue)const {
        // sub_463165: cyclic G/R/B tree traversal, strict 32-unit bounds and distance < 11907.
        const std::array<int,3> target{green,red,blue};
        int best_distance=11907,best=257;
        std::vector<std::pair<int,int>> stack{{-1,nodes_[0].right}};
        while(!stack.empty()){
            auto [axis,node]=stack.back();stack.pop_back();
            while(node!=257&&best_distance){
                axis=(axis+1)%3;
                const auto& n=nodes_.at(size_t(node));
                if(n.rgb[axis]>target[axis]-32){
                    if(n.rgb[axis]<=target[axis]+32){
                        stack.emplace_back(axis,n.right);
                        if(n.color>=32&&n.color<=254){
                            int distance=0;for(unsigned k=0;k<3;++k){int d=n.rgb[k]-target[k];distance+=d*d;}
                            if(distance<best_distance){best_distance=distance;best=node;}
                        }
                    }
                    node=n.left;
                }else node=n.right;
            }
        }
        return nodes_.at(size_t(best)).color;
    }
public:
    uint8_t blend_index(unsigned from,unsigned to,int amount)const{
        if(from>255||to>255)fatal("Original palette blend index");
        if(amount<=0)return uint8_t(from);
        if(amount>=100)return uint8_t(to);
        int rgb[3];for(unsigned k=0;k<3;++k)rgb[k]=(int(palette_[from*3+k])*(100-amount)+int(palette_[to*3+k])*amount)/100;
        return uint8_t(nearest(rgb[0],rgb[1],rgb[2]));
    }
    explicit OriginalPalette(const std::vector<uint8_t>& palette):palette_(palette){
        if(palette.size()<768)fatal("Missing original palette");
        // sub_46340F / sub_462ECB: center-nearest entry first, descending then ascending insertion.
        int center=0,distance=512;
        for(int c=0;c<256;++c){int d=0;for(int k=0;k<3;++k){int v=int(palette[c*3+k])-31;d+=v*v;}if(d<distance){distance=d;center=c;}}
        nodes_[0].color=-1;nodes_[257].color=-1;
        int next=1;
        auto insert=[&](int color){
            int id=next++;auto& n=nodes_[id];n.rgb={palette[color*3+1],palette[color*3],palette[color*3+2]};n.color=color;
            int parent=0,node=nodes_[0].right,axis=-1;bool right=true;
            while(node!=257){parent=node;axis=(axis+1)%3;right=n.rgb[axis]>=nodes_[node].rgb[axis];node=right?nodes_[node].right:nodes_[node].left;}
            (right?nodes_[parent].right:nodes_[parent].left)=id;
        };
        for(int c=center;c>=0;--c)insert(c);
        for(int c=center+1;c<256;++c)insert(c);
        auto generate=[&](unsigned table,unsigned target,int amount){
            for(unsigned c=0;c<256;++c){
                if(!amount){maps_[table][c]=uint8_t(c);continue;}
                int rgb[3];for(unsigned k=0;k<3;++k)rgb[k]=(int(palette[c*3+k])*(100-amount)+int(palette[target*3+k])*amount)/100;
                int mapped=nearest(rgb[0],rgb[1],rgb[2]);
                if(mapped<0||mapped>255)fatal("Original palette search failed");
                maps_[table][c]=uint8_t(mapped);
            }
        };
        for(unsigned i=0;i<5;++i)generate(i,33,int(i)*10);
        generate(5,32,60);generate(6,32,50);generate(7,234,65);generate(8,32,25);
        generate(9,189,60);generate(10,36,15);generate(11,60,15);generate(12,68,80);
    }
    void cycle(std::vector<uint8_t>& image,uint64_t elapsed_ms){
        // 472D78 table4EAF48: 3..10 at >16cs, 11..18 and 19..31 at >9cs.
        constexpr std::array<unsigned,3> starts{3,11,19},counts{8,8,13},periods{16,9,9};
        for(unsigned i=0;i<3;++i){uint64_t now=elapsed_ms/10;
            if(now<cycle_times_[i]){cycle_times_[i]=now;cycle_offsets_[i]=0;}
            else if(now-cycle_times_[i]>periods[i]){cycle_times_[i]=now;cycle_offsets_[i]=(cycle_offsets_[i]+1)%counts[i];}
        }
        std::array<uint32_t,256> source{};
        for(unsigned c=0;c<256;++c)for(unsigned k=0;k<3;++k)source[c]|=uint32_t(unsigned(palette_[c*3+k])*255/63)<<(k*8);
        std::unordered_map<uint32_t,unsigned> rotations;
        for(unsigned i=0;i<3;++i)for(unsigned c=starts[i];c<starts[i]+counts[i];++c)rotations.emplace(source[c],starts[i]+(c-starts[i]+counts[i]-cycle_offsets_[i])%counts[i]);
        for(size_t p=0;p<image.size();p+=4){uint32_t rgb=image[p]|uint32_t(image[p+1])<<8|uint32_t(image[p+2])<<16;
            auto match=rotations.find(rgb);if(match==rotations.end())continue;
            unsigned color=match->second;
            for(unsigned k=0;k<3;++k)image[p+k]=uint8_t(unsigned(palette_[color*3+k])*255/63);
        }
    }
    uint8_t map(unsigned table,unsigned color)const{return maps_.at(table).at(color);}
    void transform(std::vector<uint8_t>& image,int first_row,int row_count,unsigned table)const{
        transform_rect(image,0,first_row,640,row_count,table);
    }
    Result<bool> try_transform_rect(std::vector<uint8_t>& destination,int left,int top,int width,int height,unsigned table)const{
        if(destination.size()!=640*480*4||table>=maps_.size())return Result<bool>::failure(make_error("Invalid palette effect framebuffer or table"));
        auto image=destination;
        std::array<uint32_t,256> rgba{};
        for(unsigned c=0;c<256;++c){for(unsigned k=0;k<3;++k)rgba[c]|=uint32_t(unsigned(palette_[c*3+k])*255/63)<<(k*8);}
        std::unordered_map<uint32_t,unsigned> indices;
        for(unsigned c=0;c<256;++c)indices.emplace(rgba[c],c);
        for(int y=std::max(0,top);y<std::min(480,top+height);++y)for(int x=std::max(0,left);x<std::min(640,left+width);++x){
            size_t p=(size_t(y)*640+x)*4;uint32_t pixel=image[p]|uint32_t(image[p+1])<<8|uint32_t(image[p+2])<<16;
            auto found=indices.find(pixel);
            if(found==indices.end())return Result<bool>::failure(make_error("Non-indexed pixel in original palette effect"));
            unsigned color=map(table,found->second);
            for(unsigned k=0;k<3;++k)image[p+k]=uint8_t(unsigned(palette_[color*3+k])*255/63);
        }
        destination.swap(image);return Result<bool>::success(true);
    }
    void transform_rect(std::vector<uint8_t>& image,int left,int top,int width,int height,unsigned table)const{auto result=try_transform_rect(image,left,top,width,height,table);if(!result)fatal(result.error());}
};
}
