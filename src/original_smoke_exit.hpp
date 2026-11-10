#pragma once
#include "battle.hpp"
#include "original_campaign.hpp"
#include "original_map.hpp"
#include "runtime_error.hpp"
#include <utility>
namespace s2ne::smoke_exit
{
// Acceptance input policy only; these helpers never alter live campaign state.
inline unsigned protagonist(const OriginalEvents& events) { return events.flags[40] ? 1u : 11u; }
inline bool controllable_protagonist(const OriginalEvents& events)
{
    return std::any_of(events.units.begin(), events.units.end(), [&](const auto& u)
        {
            return u.id == protagonist(events) && u.behavior == 0 && u.present() && OriginalCampaign::living(u);
        });
}
inline void strengthen_mp(OriginalEvents::Unit& unit, bool enabled)
{
    if (enabled && (unit.controllable() || unit.behavior == 1) && (unit.status_flags & 0x80) && !(unit.status_flags & 0x20) && unit.hp > 0)
    {
        unit.max_mp = std::max(unit.max_mp, 400);
        unit.mp = unit.max_mp;
    }
}
struct RevealInput
{
    unsigned actor{ };
    std::pair<int, int> destination;
};
inline std::optional<RevealInput> campaign_pad_input(const Battle& battle, const OriginalEvents& events)
{
    for (const auto& actor : events.units)
    {
        if (actor.id != protagonist(events) || actor.behavior != 0 || !OriginalCampaign::living(actor))
        {
            continue;
        }
        std::vector<Battle::Point> queue{ { actor.x, actor.y } };
        std::map<std::pair<int, int>, bool> seen;
        seen[{ actor.x, actor.y }] = true;
        std::optional<Battle::Point> pad;
        bool enemy_accessible = false;
        for (size_t i = 0; i < queue.size(); ++i)
        {
            auto p = queue[i];
            if (!pad && battle.object_category(p) == 35 && battle.occupant(events.units, p) < 0)
            {
                pad = p;
            }
            for (const auto& enemy : events.units)
            {
                if (enemy.hostile() && OriginalCampaign::living(enemy) && battle.adjacent(p, { enemy.x, enemy.y }))
                {
                    enemy_accessible = true;
                }
            }
            for (int dy : { -1, 1 })
            {
                for (int dx : { 0, (p.y & 1) ? 1 : -1 })
                {
                    Battle::Point q{ p.x + dx, p.y + dy };
                    if (seen.contains({ q.x, q.y }) || battle.step_cost(actor, p, q) < 0 || !battle.player_route_passable(events.units, actor, q))
                    {
                        continue;
                    }
                    seen[{ q.x, q.y }] = true;
                    queue.push_back(q);
                }
            }
        }
        if (!enemy_accessible && battle.object_category({ actor.x, actor.y }) == 35)
        {
            return RevealInput{ actor.id, { actor.x, actor.y } };
        }
        if (!enemy_accessible && pad)
        {
            return RevealInput{ actor.id, { pad->x, pad->y } };
        }
    }
    return std::nullopt;
}
inline std::optional<RevealInput> campaign_mineral_input(const Battle& battle, const OriginalEvents& events)
{
    if (events.stage != 61 && events.stage != 131)
    {
        return std::nullopt;
    }
    auto found = std::find_if(events.units.begin(), events.units.end(), [&](const auto& actor)
        {
            return actor.id == protagonist(events) && actor.controllable() && OriginalCampaign::living(actor);
        });
    if (found == events.units.end())
    {
        return std::nullopt;
    }
    const auto& actor = *found;
    auto boss = std::find_if(events.units.begin(), events.units.end(), [](const auto& u)
        {
            return u.id == 20 && u.hostile() && OriginalCampaign::living(u);
        });
    bool learned = (actor.learned[2] & 0x800) != 0;
    if (learned && boss == events.units.end())
    {
        return std::nullopt;
    }
    bool dispelled = learned && std::find(boss->equipment.begin(), boss->equipment.end(), 237u) == boss->equipment.end();
    std::vector<Battle::Point> queue{ { actor.x, actor.y } };
    std::map<std::pair<int, int>, bool> seen;
    seen[{ actor.x, actor.y }] = true;
    std::optional<Battle::Point> pad;
    for (size_t i = 0; i < queue.size(); ++i)
    {
        auto p = queue[i];
        int distance = learned ? std::max(std::abs(2 * (boss->x - p.x) + int(boss->y & 1) - int(p.y & 1)), std::abs(boss->y - p.y)) : 999;
        if (learned && distance <= (dispelled ? 1 : 4) && (p == queue.front() || battle.occupant(events.units, p) < 0))
        {
            return RevealInput{ actor.id, { p.x, p.y } };
        }
        if (!learned && p == Battle::Point{ 3, 65 })
        {
            return RevealInput{ actor.id, { 3, 65 } };
        }
        if (!pad && battle.object_category(p) == 35 && (p == queue.front() || battle.occupant(events.units, p) < 0))
        {
            pad = p;
        }
        for (int dy : { -1, 1 })
        {
            for (int dx : { 0, (p.y & 1) ? 1 : -1 })
            {
                Battle::Point q{ p.x + dx, p.y + dy };
                if (seen.contains({ q.x, q.y }) || battle.step_cost(actor, p, q) < 0 || !battle.player_route_passable(events.units, actor, q))
                {
                    continue;
                }
                seen[{ q.x, q.y }] = true;
                queue.push_back(q);
            }
        }
    }
    if (pad)
    {
        if (learned && *pad == queue.front())
        {
            for (auto p : queue)
            {
                if (p != queue.front() && battle.object_category(p) == 35 && battle.occupant(events.units, p) < 0)
                {
                    pad = p;
                    break;
                }
            }
        }
        return RevealInput{ actor.id, { pad->x, pad->y } };
    }
    if (learned)
    {
        auto best = queue.front();
        auto distance = [&](auto p)
        {
            return std::max(std::abs(2 * (boss->x - p.x) + int(boss->y & 1) - int(p.y & 1)), std::abs(boss->y - p.y));
        };
        for (auto p : queue)
        {
            if ((p == queue.front() || battle.occupant(events.units, p) < 0) && distance(p) < distance(best))
            {
                best = p;
            }
        }
        return RevealInput{ actor.id, { best.x, best.y } };
    }
    return std::nullopt;
}
inline std::optional<RevealInput> reveal_input(const Battle& battle, const OriginalEvents& events)
{
    std::vector<OriginalEvents::Unit> observers;
    for (const auto& u : events.units)
    {
        if (!u.hostile() && OriginalCampaign::living(u))
        {
            observers.push_back(u);
        }
    }
    bool visible = false;
    for (const auto& u : events.units)
    {
        if (u.hostile() && OriginalCampaign::living(u) && battle.target_visible(u, observers, 0))
        {
            visible = true;
        }
    }
    if (visible)
    {
        return std::nullopt;
    }
    for (const auto& actor : events.units)
    {
        if (actor.behavior != 0 || !actor.present() || !OriginalCampaign::living(actor))
        {
            continue;
        }
        std::vector<Battle::Point> queue{ { actor.x, actor.y } };
        std::map<std::pair<int, int>, bool> seen;
        seen[{ actor.x, actor.y }] = true;
        for (size_t i = 0; i < queue.size(); ++i)
        {
            auto p = queue[i];
            auto moved = actor;
            moved.x = p.x;
            moved.y = p.y;
            for (const auto& target : events.units)
            {
                if (target.hostile() && OriginalCampaign::living(target) && battle.target_visible(target, std::span(&moved, 1), 0) && battle.occupant(events.units, p) < 0)
                {
                    return RevealInput{ actor.id, { p.x, p.y } };
                }
            }
            for (int dy : { -1, 1 })
            {
                for (int dx : { 0, (p.y & 1) ? 1 : -1 })
                {
                    Battle::Point q{ p.x + dx, p.y + dy };
                    if (seen.contains({ q.x, q.y }) || battle.step_cost(actor, p, q) < 0 || !battle.player_route_passable(events.units, actor, q))
                    {
                        continue;
                    }
                    seen[{ q.x, q.y }] = true;
                    queue.push_back(q);
                }
            }
        }
    }
    return std::nullopt;
}
inline bool capture_ready(const OriginalEvents& events)
{
    if (events.flags[7] || !controllable_protagonist(events))
    {
        return false;
    }
    for (size_t i = 0; i < events.event_count(); ++i)
    {
        if (events.type(i) == 9 && events.condition(i) == 0 && events.flag(i) == 8)
        {
            return true;
        }
    }
    return false;
}
inline bool ready(const OriginalCampaign::Stage& stage, const OriginalEvents& events, unsigned hostiles)
{
    bool exit_objective = false;
    for (const auto& group : stage.victory)
    {
        if (group == std::vector<int>{ 31 })
        {
            exit_objective = true;
        }
    }
    return exit_objective && events.flags[0] && controllable_protagonist(events) && (stage.id != 10 || hostiles == 0);
}
inline std::vector<std::pair<int, int>> cells(const std::filesystem::path& root, const OriginalEvents& events)
{
    OriginalCampaign campaign(root);
    OriginalMap map(root);
    map.load(events.map_id);
    auto probe = events;
    unsigned id = protagonist(events);
    auto role = std::find_if(probe.units.begin(), probe.units.end(), [&](const auto& u)
        {
            return !u.hostile() && u.id == id && OriginalCampaign::living(u);
        });
    if (role == probe.units.end())
    {
        s2ne::fatal("Exit protagonist unavailable");
    }
    std::vector<std::pair<int, int>> result;
    for (unsigned y = 0; y < map.height * 2; ++y)
    {
        for (unsigned x = 0; x < map.width - (y & 1); ++x)
        {
            role->x = int(x);
            role->y = int(y);
            if (campaign.protagonist_at_exit(probe))
            {
                result.emplace_back(int(x), int(y));
            }
        }
    }
    return result;
}
}    //namespace s2ne::smoke_exit
