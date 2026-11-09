#pragma once
#include "original_campaign.hpp"
#include "original_map.hpp"
#include "battle.hpp"
#include <utility>
namespace s2ne::smoke_exit {
// Acceptance input policy only; these helpers never alter live campaign state.
inline unsigned protagonist(const OriginalEvents& events){return events.flags[40]?1u:11u;}
inline bool controllable_protagonist(const OriginalEvents& events){
    return std::any_of(events.units.begin(),events.units.end(),[&](const auto& u){
        return u.id==protagonist(events)&&u.behavior==0&&u.present()&&OriginalCampaign::living(u);
    });
}
struct RevealInput {unsigned actor{};std::pair<int,int> destination;};
inline std::optional<RevealInput> reveal_input(const Battle& battle,const OriginalEvents& events){
    std::vector<OriginalEvents::Unit> observers;for(const auto& u:events.units)if(!u.hostile()&&OriginalCampaign::living(u))observers.push_back(u);
    bool visible=false;for(const auto& u:events.units)if(u.hostile()&&OriginalCampaign::living(u)&&battle.target_visible(u,observers,0))visible=true;
    if(visible)return std::nullopt;
    for(const auto& actor:events.units){
        if(actor.behavior!=0||!actor.present()||!OriginalCampaign::living(actor))continue;
        std::vector<Battle::Point> queue{{actor.x,actor.y}};std::map<std::pair<int,int>,bool> seen;seen[{actor.x,actor.y}]=true;
        for(size_t i=0;i<queue.size();++i){auto p=queue[i];auto moved=actor;moved.x=p.x;moved.y=p.y;
            for(const auto& target:events.units)if(target.hostile()&&OriginalCampaign::living(target)&&
                battle.target_visible(target,std::span(&moved,1),0)&&battle.occupant(events.units,p)<0)
                return RevealInput{actor.id,{p.x,p.y}};
            for(int dy:{-1,1})for(int dx:{0,(p.y&1)?1:-1}){Battle::Point q{p.x+dx,p.y+dy};
                if(seen.contains({q.x,q.y})||battle.step_cost(actor,p,q)<0||!battle.player_route_passable(events.units,actor,q))continue;
                seen[{q.x,q.y}]=true;queue.push_back(q);
            }
        }
    }
    return std::nullopt;
}
inline bool capture_ready(const OriginalEvents& events){
    if(events.flags[7]||!controllable_protagonist(events))return false;
    for(size_t i=0;i<events.event_count();++i)if(events.type(i)==9&&events.condition(i)==0&&events.flag(i)==8)return true;
    return false;
}
inline bool ready(const OriginalCampaign::Stage& stage,const OriginalEvents& events,unsigned hostiles){
    bool exit_objective=false;
    for(const auto& group:stage.victory)
        if(group==std::vector<int>{31})exit_objective=true;
    return exit_objective&&events.flags[0]&&controllable_protagonist(events)&&(stage.id!=10||hostiles==0);
}
inline std::vector<std::pair<int,int>> cells(const std::filesystem::path& root,const OriginalEvents& events){
    OriginalCampaign campaign(root);OriginalMap map(root);map.load(events.map_id);
    auto probe=events;unsigned id=protagonist(events);
    auto role=std::find_if(probe.units.begin(),probe.units.end(),[&](const auto& u){return !u.hostile()&&u.id==id&&OriginalCampaign::living(u);});
    if(role==probe.units.end())throw std::runtime_error("Exit protagonist unavailable");
    std::vector<std::pair<int,int>> result;
    for(unsigned y=0;y<map.height*2;++y)for(unsigned x=0;x<map.width-(y&1);++x){
        role->x=int(x);role->y=int(y);
        if(campaign.protagonist_at_exit(probe))result.emplace_back(int(x),int(y));
    }
    return result;
}
}
