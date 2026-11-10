#pragma once
#include "original_skills.hpp"
namespace s2ne {
// Verified arithmetic subchain only; intentionally has no state/payment API.
class OriginalSkill94Formula {
    int numerator_{},denominator_{};
public:
    static int hit_curve(int difference){
        if(difference>42)return 99;
        if(difference>=10)return (difference-10)/4+90;
        if(difference>0)return difference+80;
        if(difference==0)return 80;
        if(difference> -40)return difference/2+80;
        if(difference> -80)return difference+100;
        if(difference> -120)return (difference+80)/2+21;
        return 1;
    }
    // 46D874 effect1: both proficiency reads use the actor, not the target.
    static int base_hit(unsigned actor_proficiency,int actor_sp,int target_sp,int power=50){
        if(actor_proficiency>65535||actor_sp< -32768||actor_sp>32767||target_sp< -32768||target_sp>32767||power!=50)throw std::invalid_argument("Unsupported sword94 hit input");
        int proficiency=std::max(10,int(actor_proficiency/10));
        int difference=((proficiency+500)*std::clamp(actor_sp,10,99)-(proficiency+500)*std::clamp(target_sp,10,99))/500;
        return hit_curve(difference)*power/100;
    }
    // 4704AC, called by 46F89E case44. This is BEFORE 46E2D6 modifiers.
    static int raw_formula44(int actor_fencing,unsigned proficiency,int target_fencing,int target_defense,int numerator,int denominator,int power=50){
        if(proficiency>65535||denominator<=0||power!=50)throw std::invalid_argument("Unsupported sword94 formula input");
        return int((int64_t(proficiency/10)+1000)*std::max(1,actor_fencing)/1000-int64_t(std::max(1,target_fencing))*numerator/denominator-target_defense+power);
    }
    explicit OriginalSkill94Formula(const std::filesystem::path& root){
        auto read=[](const auto& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing sword94 formula resource");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};auto bin=read(root/"SUPERSP.BIN"),pal=read(root/"SUPERSP.PAL");auto word=[](const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);};size_t t=word(pal,920)+(size_t(word(pal,922))<<16)-20;size_t config=t+2+4*word(bin,t);config+=2+word(bin,config);config+=2;
        numerator_=int(int8_t(bin.at(config+0x98)));denominator_=int(int8_t(bin.at(config+0x99)));if(denominator_<=0)throw std::runtime_error("Invalid sword94 divisor");
        OriginalSkills skills(root);OriginalEvents::Unit unit;unit.id=1;unit.appearance=1;unit.action_points=110;unit.learned[9]=1;OriginalSkills::Context ctx;ctx.second=true;skills.open(unit,ctx);const auto& r=skills.selected();if(r.id!=94||r.effect()!=1||r.raw[20]!=44||r.word(21)!=50||r.raw[0]!=10||r.raw[27]!=64||r.raw[28]!=0||r.raw[29]!=0||r.raw[31]!=2||r.raw[32]!=0||r.raw[33]!=194)throw std::runtime_error("Unsupported sword94 formula record");
    }
    int raw_damage(const OriginalEvents::Unit& actor,const OriginalEvents::Unit& target)const{return raw_formula44(actor.fencing,actor.learned_proficiency[9],target.fencing,target.defense,numerator_,denominator_);}
    int numerator()const{return numerator_;}int denominator()const{return denominator_;}
};
}
