#pragma once
#include "original_campaign.hpp"
#include "original_save_bridge.hpp"
#include "original_progression.hpp"

namespace s2ne {
class OriginalCampaignPersistence {
    std::filesystem::path root_;
    OriginalProgression progression_;
    OriginalPromotion promotion_;
    static unsigned get(std::span<const uint8_t> b,size_t p,unsigned width=2){unsigned v=0;for(unsigned i=0;i<width;++i)v|=unsigned(b[p+i])<<(8*i);return v;}
    static void put(std::span<uint8_t> b,size_t p,unsigned v,unsigned width=2){for(unsigned i=0;i<width;++i)b[p+i]=uint8_t(v>>(8*i));}
public:
    struct RewardState { OriginalEvents events;OriginalSave::State raw; };
    explicit OriginalCampaignPersistence(const std::filesystem::path& root):root_(root),progression_(root),promotion_(root){}
    // Caller supplies copies and publishes this result together with the staged RNG.
    RewardState settle(OriginalEvents events,const OriginalCampaign::Settlement& plan,
                       const OriginalSave::State& baseline,unsigned turn,OriginalRandom& random,uint32_t seconds)const{
        OriginalCampaign campaign(root_);auto source=campaign.stage(events.stage);
        if(events.stage!=plan.stage||plan.next!=campaign.selected_route(events))throw std::runtime_error("Campaign settlement does not match original stage route");
        auto raw=OriginalSaveBridge::capture(events,turn,{32,16},&baseline);
        auto& group=raw.groups[0];
        if(group.inventory.size()!=302)throw std::runtime_error("Campaign requires complete original302-item inventory");
        for(const auto& reward:plan.rewards){
            if(reward.type==1){for(int id:reward.values){
                size_t found=group.units.size();for(size_t i=0;i<group.units.size();++i)if(get(group.units[i],2)==unsigned(id)&&!(group.units[i][0x33]&0x20)){found=i;break;}
                // 45CC8F(mode1) returns no role when dead/missing: no fabricated replacement.
                if(found==group.units.size())continue;
                auto& u=events.units.at(found);u.behavior=0;u.first_turn=1;u.last_turn=0;u.status_flags&=~0x40u;
                put(group.units[found],8,0);put(group.units[found],0xA,1);put(group.units[found],0xC,0);group.units[found][0x33]&=0xBF;
            }}else if(reward.type==3){if(reward.values.empty())throw std::runtime_error("Missing original money reward");
                put(group.metadata,0,get(group.metadata,0,4)+unsigned(reward.values.front()),4);
            }else if(reward.type==4){for(int item:reward.values){if(item<1||item>302)throw std::runtime_error("Invalid original reward item");++group.inventory[size_t(item-1)];}}
            else throw std::runtime_error("Unimplemented original campaign reward opcode");
        }
        auto staged_random=random;
        for(size_t i=0;i<group.units.size();++i){auto& u=events.units.at(i);if(!OriginalCampaign::living(u))continue;
            u.experience=uint16_t(u.experience+30u);
            if(!progression_.award_native(u,staged_random,0,promotion_,[=]{return seconds;}).applied)throw std::runtime_error("Original victory level growth refused");
            for(size_t j=0;j<group.units.size();++j){const auto& other=events.units.at(j);
                if(i==j||!OriginalCampaign::living(other)||other.id<1||other.id>40)continue;
                size_t p=0x142+other.id-1;
                int value=u.relationships[other.id-1]+events.victory_relationship_increment();
                u.relationships[other.id-1]=std::clamp(value,-128,127);
                group.units[i][p]=uint8_t(u.relationships[other.id-1]);
            }
        }
        raw=OriginalSaveBridge::capture(events,turn,{32,16},&raw);
        random=std::move(staged_random);return {std::move(events),std::move(raw)};
    }
    // 462224 assigns living persistent roles to generic turn1 controllable slots,
    // in persistent roster order, NOT by matching next-stage role IDs.
    OriginalEvents deploy_next(const OriginalEvents& previous,const OriginalSave::State& raw)const{
        if(get(raw.globals_4f7cd0,8,4)!=previous.stage)throw std::runtime_error("Stale original deployment stage");
        OriginalCampaign campaign(root_);unsigned target=campaign.selected_route(previous);
        if(campaign.stage(previous.stage).mode==3){
            OriginalEvents source(root_,previous.difficulty);source.load_stage(previous.stage);
            target=OriginalCampaign::completed_story_route(source,previous,campaign.stage(previous.stage).next);
            if(!target)throw std::runtime_error("Original story stage has no completed route event");
        }
        if(!target||(campaign.stage(target).mode!=1&&campaign.stage(target).mode!=3))throw std::runtime_error("Original intermediate campaign stage requires dedicated dispatcher");
        OriginalEvents next=previous;next.clear_stage_flags();next.load_stage(target);
        if(campaign.stage(target).mode==3){next.units=previous.units;return next;}
        auto slots=next.deployment_slots();
        if(raw.groups[0].units.size()>previous.units.size())throw std::runtime_error("Missing original persistent role roster");
        std::vector<OriginalEvents::Unit> roster;
        size_t slot=0;
        for(size_t i=0;i<raw.groups[0].units.size();++i){auto u=previous.units[i];
            if(get(raw.groups[0].units[i],2)!=u.id)throw std::runtime_error("Stale campaign role identity");
            // 44A4A9 ->45DDA6 resets current pools/temporary statuses before462224.
            bool removed= get(raw.groups[0].units[i],8)==1&&((u.status_flags&0x20)||!(u.status_flags&0x40));
            bool deployable=!removed&&OriginalCampaign::living(u)&&!(u.status_flags&0x50);
            u.status_flags&=0x73;u.status34=0;u.status3c=0;u.action_state=0;
            u.hp=u.max_hp;u.mp=u.max_mp;u.st=u.max_st;u.sp=u.max_sp;
            u.max_action_points=u.action_points=std::min(u.base_action_points,110);u.carry_points=0;
            u.attack=u.attack_limit;u.defense=u.defense_limit;u.tactics=u.base_tactics;
            if(deployable&&slot<slots.size()){
                u.status_flags|=0x80;
                const auto& p=slots[slot++];u.x=p.x;u.y=p.y;u.direction=p.direction&3;u.animation_action=p.direction>>2;
                u.first_turn=1;u.last_turn=0;u.action_state=0;u.movement_pose=false;u.movement_frame=0;
            }else u.status_flags&=~0x80u;
            next.restore_unit_resources(u);roster.push_back(std::move(u));
        }
        bool hostile=false;
        for(auto& u:next.units){hostile=hostile||u.behavior==2||u.behavior==3||u.behavior==8;
            bool arrives=u.last_turn==0?u.first_turn==1:u.first_turn<=1&&u.last_turn>=1;
            if(hostile&&arrives)roster.push_back(std::move(u));
        }
        next.units=std::move(roster);return next;
    }
    OriginalEvents deploy_stage2(const OriginalEvents& previous,const OriginalSave::State& raw)const{return deploy_next(previous,raw);}
    OriginalSave::State rebase_stage2(const OriginalEvents& next,const OriginalSave::State& settled,
                                    const OriginalSave::State& stage2_baseline)const{
        if(next.stage!=2||get(settled.globals_4f7cd0,8,4)!=1||get(stage2_baseline.globals_4f7cd0,8,4)!=2)
            throw std::runtime_error("Original stage2 full-state initialization required");
        auto staged=stage2_baseline;staged.groups[0]=settled.groups[0];
        // 44A4A9 clears only the first byte of each of the11 group status triples.
        for(size_t i=0;i<11;++i)staged.groups[0].status[i*3]=0;
        for(auto& r:staged.groups[0].units){std::fill(r.begin()+0x34,r.begin()+0x40,uint8_t(0));}
        staged.flags_4f7d10=next.flags;staged.event_states=next.save_event_states();
        return OriginalSaveBridge::capture(next,1,{32,16},&staged);
    }
};
}
