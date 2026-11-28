#include "campaign_persistence.h"

namespace s2ne
{
auto CampaignPersistence::get(std::span<const uint8_t> b, size_t p, unsigned width) -> unsigned
{
    unsigned v = 0;
    for (unsigned i = 0; i < width; ++i)
    {
        v |= unsigned(b[p + i]) << (8 * i);
    }
    return v;
}

auto CampaignPersistence::put(std::span<uint8_t> b, size_t p, unsigned v, unsigned width) -> void
{
    for (unsigned i = 0; i < width; ++i)
    {
        b[p + i] = uint8_t(v >> (8 * i));
    }
}

auto CampaignPersistence::try_collect_physical_loot(Save::State& raw, const Events::Unit& actor, const Events::Unit& target) -> Result<bool>
{
    if (!(target.status_flags & 0x20) || actor.persistent_role6 == 1) { return Result<bool>::success(true); }
    auto staged = raw.groups[0].inventory;
    std::vector<uint8_t> counts(staged.size());
    for (unsigned id : target.initial_status)
    {
        if (!id) { continue; }
        if (id > counts.size()) { return Result<bool>::failure(make_error("Original physical loot item index")); }
        ++counts[id - 1];
    }
    for (size_t item = 0; item < counts.size(); ++item)
    {
        if (counts[item]) { staged[item] = uint8_t(std::min(99u, unsigned(staged[item]) + unsigned(counts[item]))); }
    }
    raw.groups[0].inventory.swap(staged);
    return Result<bool>::success(true);
}

auto CampaignPersistence::create(const std::filesystem::path& root) -> Result<CampaignPersistence>
{
    CampaignPersistence result;
    auto progression = Progression::create(root);
    if (!progression)
    {
        return Result<CampaignPersistence>::failure(progression.error());
    }
    auto promotion = Promotion::create(root);
    if (!promotion)
    {
        return Result<CampaignPersistence>::failure(promotion.error());
    }
    result.root_ = root;
    result.progression_ = std::move(progression).value();
    result.promotion_ = std::move(promotion).value();
    return Result<CampaignPersistence>::success(std::move(result));
}

CampaignPersistence::CampaignPersistence(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto CampaignPersistence::try_settle(Events events, const Campaign::Settlement& plan,
    const Save::State& baseline, unsigned turn, RandomStream& random, uint32_t seconds) const -> Result<RewardState>
{
    if (!baseline.battle_context())
    {
        return Result<RewardState>::failure(make_error("Campaign rewards already settled in interbattle scene"));
    }
    auto campaignResult = Campaign::create(root_);
    if (!campaignResult)
    {
        return Result<RewardState>::failure(campaignResult.error());
    }
    auto campaign = std::move(campaignResult).value();
    auto source = campaign.stage(events.stage);
    if (events.stage != plan.stage || plan.next != campaign.selected_route(events))
    {
        return Result<RewardState>::failure(make_error("Campaign settlement does not match original stage route"));
    }
    auto captured = SaveBridge::try_capture(events, turn, { 32, 16 }, &baseline);
    if (!captured)
    {
        return Result<RewardState>::failure(captured.error());
    }
    auto raw = std::move(captured).value();
    auto& group = raw.groups[0];
    if (group.inventory.size() != 302)
    {
        return Result<RewardState>::failure(make_error("Campaign requires complete original302-item inventory"));
    }
    for (const auto& reward : plan.rewards)
    {
        if (reward.type == 1)
        {
            for (int id : reward.values)
            {
                size_t found = group.units.size();
                for (size_t i = 0; i < group.units.size(); ++i)
                {
                    if (get(group.units[i], 2) == unsigned(id) && !(group.units[i][0x33] & 0x20))
                    {
                        found = i;
                        break;
                    }
                }
                // 45CC8F(mode1) returns no role when dead/missing: no fabricated replacement.
                if (found == group.units.size())
                {
                    continue;
                }
                auto& u = events.units.at(found);
                u.behavior = 0;
                u.first_turn = 1;
                u.last_turn = 0;
                u.status_flags &= ~0x40u;
                put(group.units[found], 8, 0);
                put(group.units[found], 0xA, 1);
                put(group.units[found], 0xC, 0);
                group.units[found][0x33] &= 0xBF;
            }
        }
        else if (reward.type == 3)
        {
            if (reward.values.empty())
            {
                return Result<RewardState>::failure(make_error("Missing original money reward"));
            }
            put(group.metadata, 0, get(group.metadata, 0, 4) + unsigned(reward.values.front()), 4);
        }
        else if (reward.type == 4)
        {
            for (int item : reward.values)
            {
                if (item < 1 || item > 302)
                {
                    return Result<RewardState>::failure(make_error("Invalid original reward item"));
                }
                ++group.inventory[size_t(item - 1)];
            }
        }
        else
        {
            return Result<RewardState>::failure(make_error("Unimplemented original campaign reward opcode"));
        }
    }
    auto staged_random = random;
    for (size_t i = 0; i < group.units.size(); ++i)
    {
        auto& u = events.units.at(i);
        if (!Campaign::living(u))
        {
            continue;
        }
        u.experience = uint16_t(u.experience + 30u);
        if (!progression_.award_native(u, staged_random, 0, promotion_, [=]
                             {
                                 return seconds;
                             })
                .applied)
        {
            return Result<RewardState>::failure(make_error("Original victory level growth refused"));
        }
        for (size_t j = 0; j < group.units.size(); ++j)
        {
            const auto& other = events.units.at(j);
            if (i == j || !Campaign::living(other) || other.id < 1 || other.id > 40)
            {
                continue;
            }
            size_t p = 0x142 + other.id - 1;
            int value = u.relationships[other.id - 1] + events.victory_relationship_increment();
            u.relationships[other.id - 1] = std::clamp(value, -128, 127);
            group.units[i][p] = uint8_t(u.relationships[other.id - 1]);
        }
    }
    auto final = SaveBridge::try_capture(events, turn, { 32, 16 }, &raw);
    if (!final)
    {
        return Result<RewardState>::failure(final.error());
    }
    raw = std::move(final).value();
    if (is_custom_stage(events.stage))
    {
        events.script_route = int32_t(plan.next);
        put(raw.globals_4f7cd0, 16, plan.next, 4);
    }
    if (plan.next && campaign.stage(plan.next).mode != 3)
    {
        put(raw.globals_4f7cd0, 4, 1, 4);
        raw.event_states.clear();
        auto hub = SaveBridge::try_restore(root_, raw);
        if (!hub) { return Result<RewardState>::failure(hub.error()); }
        auto restored = std::move(hub).value();
        events = std::move(restored.events);
        raw = std::move(restored.raw);
    }
    random = std::move(staged_random);
    return Result<RewardState>::success({ std::move(events), std::move(raw) });
}

auto CampaignPersistence::settle(Events events, const Campaign::Settlement& plan, const Save::State& baseline, unsigned turn, RandomStream& random, uint32_t seconds) const -> RewardState
{
    auto result = try_settle(std::move(events), plan, baseline, turn, random, seconds);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto CampaignPersistence::try_deploy_next(const Events& previous, const Save::State& raw) const -> Result<Events>
{
    if (get(raw.globals_4f7cd0, 8, 4) != previous.stage)
    {
        return Result<Events>::failure(make_error("Stale original deployment stage"));
    }
    auto campaignResult = Campaign::create(root_);
    if (!campaignResult)
    {
        return Result<Events>::failure(campaignResult.error());
    }
    auto campaign = std::move(campaignResult).value();
    unsigned target = campaign.selected_route(previous);
    if (campaign.stage(previous.stage).mode == 3)
    {
        auto sourceResult = Events::create(root_, previous.difficulty);
        if (!sourceResult)
        {
            return Result<Events>::failure(sourceResult.error());
        }
        auto source = std::move(sourceResult).value();
        auto sourceLoaded = source.try_load_stage(previous.stage);
        if (!sourceLoaded)
        {
            return Result<Events>::failure(sourceLoaded.error());
        }
        target = Campaign::completed_story_route(source, previous, campaign.stage(previous.stage).next);
        if (!target)
        {
            return Result<Events>::failure(make_error("Original story stage has no completed route event"));
        }
    }
    if (!target || (campaign.stage(target).mode != 1 && campaign.stage(target).mode != 3))
    {
        return Result<Events>::failure(make_error("Original intermediate campaign stage requires dedicated dispatcher: source=" + std::to_string(previous.stage) + " target=" + std::to_string(target) + " mode=" + std::to_string(target ? campaign.stage(target).mode : 0)));
    }
    Events next = previous;
    next.clear_stage_flags();
    auto loaded = next.try_load_stage(target);
    if (!loaded)
    {
        return Result<Events>::failure(loaded.error());
    }
    if (campaign.stage(target).mode == 3)
    {
        next.units = previous.units;
        return Result<Events>::success(std::move(next));
    }
    auto slots = next.deployment_slots();
    if (raw.groups[0].units.size() > previous.units.size())
    {
        return Result<Events>::failure(make_error("Missing original persistent role roster"));
    }
    std::vector<Events::Unit> roster;
    size_t slot = 0;
    for (size_t i = 0; i < raw.groups[0].units.size(); ++i)
    {
        auto u = previous.units[i];
        if (get(raw.groups[0].units[i], 2) != u.id)
        {
            return Result<Events>::failure(make_error("Stale campaign role identity"));
        }
        // 44A4A9 ->45DDA6 resets current pools/temporary statuses before462224.
        bool removed = get(raw.groups[0].units[i], 8) == 1 && ((u.status_flags & 0x20) || !(u.status_flags & 0x40));
        // 44A4A9会保留离队成员的持久化记录，但0x20/0x40成员不进入下一关的初始部署槽。
        bool deployable = !removed && Campaign::living(u) && !(u.status_flags & 0x70);
        // 剧情关后的下一场战斗可能没有部署槽；原程序仍保留持久队伍的登场位。
        const bool implicit_deployment = slots.empty() && deployable;
        u.status_flags &= 0x73;
        u.status34 = 0;
        u.status3c = 0;
        u.action_state = 0;
        u.hp = u.max_hp;
        u.mp = u.max_mp;
        u.st = u.max_st;
        u.sp = u.max_sp;
        u.max_action_points = u.action_points = std::min(u.base_action_points, 110);
        u.carry_points = 0;
        u.attack = u.attack_limit;
        u.defense = u.defense_limit;
        u.tactics = u.base_tactics;
        if (deployable && (implicit_deployment || slot < slots.size()))
        {
            u.status_flags |= 0x80;
            if (!implicit_deployment)
            {
                const auto& p = slots[slot++];
                u.x = p.x;
                u.y = p.y;
                u.direction = p.direction & 3;
                u.animation_action = p.direction >> 2;
            }
            u.first_turn = 1;
            u.last_turn = 0;
            u.action_state = 0;
            u.movement_pose = false;
            u.movement_frame = 0;
        }
        else
        {
            u.status_flags &= ~0x80u;
        }
        auto resources = next.try_restore_unit_resources(u);
        if (!resources)
        {
            return Result<Events>::failure(resources.error());
        }
        roster.push_back(std::move(u));
    }
    bool hostile = false;
    for (auto& u : next.units)
    {
        hostile = hostile || u.behavior == 2 || u.behavior == 3 || u.behavior == 8;
        bool arrives = u.last_turn == 0 ? u.first_turn == 1 : u.first_turn <= 1 && u.last_turn >= 1;
        if (hostile && arrives)
        {
            roster.push_back(std::move(u));
        }
    }
    next.units = std::move(roster);
    return Result<Events>::success(std::move(next));
}

auto CampaignPersistence::deploy_next(const Events& previous, const Save::State& raw) const -> Events
{
    auto result = try_deploy_next(previous, raw);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto CampaignPersistence::deploy_stage2(const Events& previous, const Save::State& raw) const -> Events { return deploy_next(previous, raw); }

auto CampaignPersistence::try_rebase_stage2(const Events& next, const Save::State& settled,
    const Save::State& stage2_baseline) const -> Result<Save::State>
{
    if (next.stage != 2 || get(settled.globals_4f7cd0, 8, 4) != 1 || get(stage2_baseline.globals_4f7cd0, 8, 4) != 2)
    {
        return Result<Save::State>::failure(make_error("Original stage2 full-state initialization required"));
    }
    auto staged = stage2_baseline;
    staged.groups[0] = settled.groups[0];
    // 44A4A9 clears only the first byte of each of the11 group status triples.
    for (size_t i = 0; i < 11; ++i)
    {
        staged.groups[0].status[i * 3] = 0;
    }
    for (auto& r : staged.groups[0].units) { std::fill(r.begin() + 0x34, r.begin() + 0x40, uint8_t(0)); }
    staged.flags_4f7d10 = next.flags;
    staged.event_states = next.save_event_states();
    return SaveBridge::try_capture(next, 1, { 32, 16 }, &staged);
}

auto CampaignPersistence::rebase_stage2(const Events& next, const Save::State& settled, const Save::State& baseline) const -> Save::State
{
    auto result = try_rebase_stage2(next, settled, baseline);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}
}    //namespace s2ne
