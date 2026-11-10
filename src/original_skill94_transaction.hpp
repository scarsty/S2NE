#pragma once
#include "original_skill94_resolution.hpp"
#include "original_death_dispatch.hpp"
namespace s2ne {
// Single94 staging boundary. Unknown scripted death dispatch is rejected, never
// replaced by physical-combat settlement or a generic dead flag.
class OriginalSkill94Transaction {
    OriginalSkill94Formula formula_;
    OriginalSkill94Resolution resolution_;
    std::array<int,256> config_{};
public:
    struct Context {
        OriginalSkill94Resolution::HitInput hit;
        OriginalSkill94Resolution::DamageContext damage;
        int elevation{};uint32_t seconds{};
        unsigned payment_state{};
        bool original_second_partition{},target_runtime_known{},synchronous_death_ready{};
    };
    enum class Failure { none, gate, unknown_runtime, scripted_death, resurrection_consumption, award };
    struct Result {
        bool applied{},hit{},revived{},reverse_heal{};
        Failure failure{Failure::gate};
        int chance{},roll{},damage{},healed{},raw_xp{},proficiency{},hp_snapshot{};
        bool killed{};unsigned consumed_item{};
    };
    static bool target_supported(const OriginalEvents& events,const OriginalEvents::Unit& target){
        return !(target.status_flags&0x50)&&OriginalDeathDispatch::continuation(events,target.id).supported&&
            (target.action_status_runtime.has_value()||target.status39.has_value());
    }
    using Award=OriginalSkills::AwardAdapter;
    explicit OriginalSkill94Transaction(const std::filesystem::path& root):formula_(root),resolution_(root){
        auto read=[](const auto& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing94 transaction config");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto b=read(root/"SUPERSP.BIN"),p=read(root/"SUPERSP.PAL");auto w=[](const auto& b,size_t i){return b.at(i)|(unsigned(b.at(i+1))<<8);};
        size_t c=w(p,920)+(size_t(w(p,922))<<16)-20;c+=2+4*w(b,c);c+=2+w(b,c);c+=2;
        for(unsigned i=0;i<256;++i)config_[i]=int(int8_t(b.at(c+i)));
        if(config_[0xE1]<=0)throw std::runtime_error("Invalid94 reward divisor");
    }
    // The caller must pass a requested original94 record and the actual map
    // triples. The function stages unit/RNG copies; adapters see staged data only.
    Result execute(OriginalSkills& menu,OriginalEvents::Unit& actor,OriginalEvents::Unit& target,const OriginalEvents& resources,
        original_combat::Random& random,const Context& ctx,const Award& award,
        std::vector<uint8_t>* army_inventory=nullptr,const OriginalPromotionTransaction* equipment=nullptr)const{
        Result out;
        if(menu.page()!=OriginalSkills::Page::requested||!menu.request())return out;
        const auto request=*menu.request();const auto record=menu.selected();
        if(request.skill_id!=94||record.id!=94||record.group!=9||record.bit!=0||record.effect()!=1||record.raw[20]!=44||record.word(13)!=5||record.raw[32]!=0||record.raw[33]!=194||
            request.unit_id!=actor.id||request.appearance!=actor.appearance||request.group!=9||request.bit!=0||!request.second||!ctx.original_second_partition||!award||
            !(actor.learned[9]&1)||!actor.controllable()||!(actor.status_flags&0x80)||(actor.status_flags&0x20)||actor.hp<=0||target.hp<=0||actor.hostile()==target.hostile()||&actor==&target||!(target.status_flags&0x80)||(target.status_flags&0x20)||
            ctx.damage.difficulty<1||ctx.damage.difficulty>5||!ctx.damage.friendly_army||ctx.damage.conditions_mode||actor.st<5||actor.action_points<config_[0xEF]+((actor.status34&12)?config_[0xF3]:0))return out;
        int dx=2*(target.x-actor.x)+int(target.y&1)-int(actor.y&1),dy=target.y-actor.y;
        unsigned status3d=actor.action_status_runtime?actor.action_status_runtime->at(0x3D):actor.status3d;
        if(std::max(std::abs(dx),std::abs(dy))>3||(status3d&0x40))return out;
        if(!ctx.target_runtime_known||!actor.ending_combat_value){out.failure=Failure::unknown_runtime;return out;}
        if(ctx.hit.actor_proficiency!=actor.learned_proficiency[9]||ctx.hit.target_proficiency!=target.learned_proficiency[9]||ctx.hit.actor_fencing!=actor.fencing||ctx.hit.target_fencing!=target.fencing||ctx.hit.target_tactics!=target.tactics)return out;
        if(!target_supported(resources,target)){out.failure=Failure::scripted_death;return out;}
        auto a=actor,t=target;auto rng=random;
        auto inventory=army_inventory?*army_inventory:std::vector<uint8_t>{};
        if(army_inventory&&(!equipment||inventory.size()!=equipment->item_count()))return out;
        auto packet=resolution_.resolve(ctx.hit,formula_.raw_damage(a,t),t.hp,ctx.elevation,ctx.damage,rng,ctx.seconds);
        out.chance=packet.chance;out.roll=packet.roll;out.hit=packet.hit;out.damage=packet.hit?packet.damage:0;
        // 468BAC always captures+132, even on MISS; effect1 has no drain branch.
        out.hp_snapshot=t.hp;t.hp-=out.damage;
        if(!t.hp){
            if(resources.equipment_special(t,25)){
                out.hp_snapshot=0;t.hp=t.max_hp;out.revived=true;
            }else if(resources.equipment_special(t,28)){
                if(!army_inventory||!equipment){out.failure=Failure::resurrection_consumption;return out;}
                auto slot=equipment->first_consumable_special(t,28);
                if(!slot){out.failure=Failure::resurrection_consumption;return out;}
                out.hp_snapshot=0;t.hp=t.max_hp;out.revived=true;out.consumed_item=t.equipment[*slot];
                OriginalPromotionTransaction::State state{t,inventory,OriginalRandom({rng.seed_state(),rng.draw_count()})};
                equipment->remove_equipment(state,*slot,true);t=std::move(state.unit);inventory=std::move(state.inventory);
            }else{
                // Only the no-death-event ordinary corpse branch is authorized.
                // Scripted removal/transformation and summoned-role slots need
                // the full41C02A/4531AB dispatcher, not a generic physical callback.
                auto dispatch=OriginalDeathDispatch::inspect(resources,t.id);
                if(!army_inventory||!equipment||(!dispatch.settled()&&!ctx.synchronous_death_ready)||(t.status_flags&0x50)){out.failure=Failure::scripted_death;return out;}
                out.killed=true;t.ending_losses=uint16_t(t.ending_losses+1);t.status_flags|=0x20;
                a.ending_kills=uint16_t(a.ending_kills+1);
                // 468FDF mode0/friendly army collects initial_status WORDs, not
                // equipped items.458B03 merges the byte buffer with cap99.
                std::vector<uint8_t> loot(inventory.size());
                for(auto id:t.initial_status)if(id){if(id>loot.size()){out.failure=Failure::scripted_death;return out;}++loot[id-1];}
                for(size_t i=0;i<inventory.size();++i)if(loot[i])inventory[i]=uint8_t(std::min(99u,unsigned(inventory[i])+unsigned(loot[i])));
            }
        }else if(resources.equipment_special(t,29)&&rng.percent(ctx.seconds)<30){
            // Original only draws this extra roll for living targets, including MISS.
            out.reverse_heal=true;out.hp_snapshot=t.hp;
            out.healed=std::min(out.damage*2,t.max_hp-t.hp);t.hp+=out.healed;
        }
        // 469101 mode1 uses the last468BAC+132 HP snapshot, other stats live.
        t.ending_combat_value=out.hp_snapshot;
        if(!out.killed){
            int av=resources.combat_value(a,1),tv=resources.combat_value(t,1);
            out.raw_xp=std::max(1,out.damage*config_[0xE6]*tv/(tv+av)/config_[0xE1]);
        }
        // 468FDF passes arg4=0 to469101. With HP0,46913D increments
        // the XP accumulator once and returns BEFORE value/kill-bonus arithmetic.
        if(out.killed)out.raw_xp=1;
        // 466082: successful targets + kill-count delta; resurrection is not a kill.
        out.proficiency=(6+int(out.damage!=0)+int(out.killed)-int(ctx.damage.difficulty))*10/3+10;
        if(!award(a,rng,out.raw_xp,out.proficiency,9)){out.failure=Failure::award;return out;}
        a.action_points-=config_[0xEF]+((a.status34&8)?config_[0xF3]:0);a.status34|=8;
        a.carry_points=std::min(a.carry_points,a.action_points);
        if(ctx.payment_state!=2)a.st-=5;
        actor=std::move(a);target=std::move(t);random=rng;if(army_inventory)*army_inventory=std::move(inventory);menu.cancel();out.applied=true;out.failure=Failure::none;return out;
    }
};
}
