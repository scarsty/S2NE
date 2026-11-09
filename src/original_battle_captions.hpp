#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"

namespace s2ne {
// 43AE24 packs 4EAC38/4EAC70 alongside the enabled CEL entries.
// 43A62D paints the selected entry's caption, not overlapping captions for every icon.
struct OriginalBattleCaptions {
    static constexpr std::array<unsigned,6> command_ids{54,55,56,57,198,58};
    static constexpr std::array<unsigned,8> attack_ids{60,61,62,63,64,65,66,67};
    static std::vector<uint8_t> command(const OriginalMenu& font,int index,bool exhausted=false){
        return font.text_bytes(index==5&&exhausted?59:command_ids.at(index));
    }
    static std::vector<uint8_t> attack(const OriginalMenu& font,int index,const OriginalEvents::Unit& unit,
                                     const std::optional<OriginalEvents::ActionRecord>& action,
                                     const std::vector<uint8_t>& items){
        // 43B135 uses weapon+2, otherwise action+1; 43B1D1 uses action+1.
        if(action&&(index==0||index==7)){
            std::vector<uint8_t> text;
            if(index==0&&unit.equipment[0]){
                size_t p=2+size_t(unit.equipment[0]-1)*38+2;
                for(unsigned i=0;i<12&&items.at(p+i);++i)text.push_back(items.at(p+i));
            }else for(unsigned i=1;i<11&&action->fields[i];++i)text.push_back(uint8_t(action->fields[i]));
            if(!text.empty())return text;
        }
        return font.text_bytes(attack_ids.at(index));
    }
    static int left(const std::vector<uint8_t>& text,int icon_x){
        int x=icon_x+16-int(text.size())*4;
        if(x<0)return 0;
        return std::min(x,639-int(text.size())*8);
    }
    static void draw(const OriginalMenu& font,std::vector<uint8_t>& image,const std::vector<uint8_t>& text,int icon_x,int icon_y){
        // 43A866/43AA0E -> 422402(ink=88h, shadow=9Ah, style=1).
        int x=left(text,icon_x),y=icon_y+40;
        font.draw_small(image,text,x+1,y+1,154,true);
        font.draw_small(image,text,x,y,136,true);
    }
};
}
