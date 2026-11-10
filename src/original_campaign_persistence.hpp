#pragma once
#include "original_campaign.hpp"
#include "original_progression.hpp"
#include "original_save_bridge.hpp"

namespace s2ne
{
class OriginalCampaignPersistence
{
    std::filesystem::path root_;
    OriginalProgression progression_;
    OriginalPromotion promotion_;
    static unsigned get(std::span<const uint8_t> b, size_t p, unsigned width = 2)
    {
        unsigned v = 0;
        for (unsigned i = 0; i < width; ++i)
        {
            v |= unsigned(b[p + i]) << (8 * i);
        }
        return v;
    }
    static void put(std::span<uint8_t> b, size_t p, unsigned v, unsigned width = 2)
    {
        for (unsigned i = 0; i < width; ++i)
        {
            b[p + i] = uint8_t(v >> (8 * i));
        }
    }

public:
    struct RewardState
    {
        OriginalEvents events;
        OriginalSave::State raw;
    };
    OriginalCampaignPersistence() = default;
    static Result<OriginalCampaignPersistence> create(const std::filesystem::path& root)
    {
        OriginalCampaignPersistence result;
        auto progression = OriginalProgression::create(root);
        if (!progression)
        {
            return Result<OriginalCampaignPersistence>::failure(progression.error());
        }
        auto promotion = OriginalPromotion::create(root);
        if (!promotion)
        {
            return Result<OriginalCampaignPersistence>::failure(promotion.error());
        }
        result.root_ = root;
        result.progression_ = std::move(progression).value();
        result.promotion_ = std::move(promotion).value();
        return Result<OriginalCampaignPersistence>::success(std::move(result));
    }
    explicit OriginalCampaignPersistence(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    // Caller supplies copies and publishes this result together with the staged RNG.
    Result<RewardState> try_settle(OriginalEvents events, const OriginalCampaign::Settlement& plan,
        const OriginalSave::State& baseline, unsigned turn, OriginalRandom& random, uint32_t seconds) const
    {
        auto campaignResult = OriginalCampaign::create(root_);
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
        auto captured = OriginalSaveBridge::try_capture(events, turn, { 32, 16 }, &baseline);
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
            if (!OriginalCampaign::living(u))
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
                if (i == j || !OriginalCampaign::living(other) || other.id < 1 || other.id > 40)
                {
                    continue;
                }
                size_t p = 0x142 + other.id - 1;
                int value = u.relationships[other.id - 1] + events.victory_relationship_increment();
                u.relationships[other.id - 1] = std::clamp(value, -128, 127);
                group.units[i][p] = uint8_t(u.relationships[other.id - 1]);
            }
        }
        auto final = OriginalSaveBridge::try_capture(events, turn, { 32, 16 }, &raw);
        if (!final)
        {
            return Result<RewardState>::failure(final.error());
        }
        raw = std::move(final).value();
        random = std::move(staged_random);
        return Result<RewardState>::success({ std::move(events), std::move(raw) });
    }
    RewardState settle(OriginalEvents events, const OriginalCampaign::Settlement& plan, const OriginalSave::State& baseline, unsigned turn, OriginalRandom& random, uint32_t seconds) const
    {
        auto result = try_settle(std::move(events), plan, baseline, turn, random, seconds);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    // 462224 assigns living persistent roles to generic turn1 controllable slots,
    // in persistent roster order, NOT by matching next-stage role IDs.
    Result<OriginalEvents> try_deploy_next(const OriginalEvents& previous, const OriginalSave::State& raw) const
    {
        if (get(raw.globals_4f7cd0, 8, 4) != previous.stage)
        {
            return Result<OriginalEvents>::failure(make_error("Stale original deployment stage"));
        }
        auto campaignResult = OriginalCampaign::create(root_);
        if (!campaignResult)
        {
            return Result<OriginalEvents>::failure(campaignResult.error());
        }
        auto campaign = std::move(campaignResult).value();
        unsigned target = campaign.selected_route(previous);
        if (campaign.stage(previous.stage).mode == 3)
        {
            auto sourceResult = OriginalEvents::create(root_, previous.difficulty);
            if (!sourceResult)
            {
                return Result<OriginalEvents>::failure(sourceResult.error());
            }
            auto source = std::move(sourceResult).value();
            auto sourceLoaded = source.try_load_stage(previous.stage);
            if (!sourceLoaded)
            {
                return Result<OriginalEvents>::failure(sourceLoaded.error());
            }
            target = OriginalCampaign::completed_story_route(source, previous, campaign.stage(previous.stage).next);
            if (!target)
            {
                return Result<OriginalEvents>::failure(make_error("Original story stage has no completed route event"));
            }
        }
        if (!target || (campaign.stage(target).mode != 1 && campaign.stage(target).mode != 3))
        {
            return Result<OriginalEvents>::failure(make_error("Original intermediate campaign stage requires dedicated dispatcher"));
        }
        OriginalEvents next = previous;
        next.clear_stage_flags();
        auto loaded = next.try_load_stage(target);
        if (!loaded)
        {
            return Result<OriginalEvents>::failure(loaded.error());
        }
        if (campaign.stage(target).mode == 3)
        {
            next.units = previous.units;
            return Result<OriginalEvents>::success(std::move(next));
        }
        auto slots = next.deployment_slots();
        if (raw.groups[0].units.size() > previous.units.size())
        {
            return Result<OriginalEvents>::failure(make_error("Missing original persistent role roster"));
        }
        std::vector<OriginalEvents::Unit> roster;
        size_t slot = 0;
        for (size_t i = 0; i < raw.groups[0].units.size(); ++i)
        {
            auto u = previous.units[i];
            if (get(raw.groups[0].units[i], 2) != u.id)
            {
                return Result<OriginalEvents>::failure(make_error("Stale campaign role identity"));
            }
            // 44A4A9 ->45DDA6 resets current pools/temporary statuses before462224.
            bool removed = get(raw.groups[0].units[i], 8) == 1 && ((u.status_flags & 0x20) || !(u.status_flags & 0x40));
            bool deployable = !removed && OriginalCampaign::living(u) && !(u.status_flags & 0x50);
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
            if (deployable && slot < slots.size())
            {
                u.status_flags |= 0x80;
                const auto& p = slots[slot++];
                u.x = p.x;
                u.y = p.y;
                u.direction = p.direction & 3;
                u.animation_action = p.direction >> 2;
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
                return Result<OriginalEvents>::failure(resources.error());
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
        return Result<OriginalEvents>::success(std::move(next));
    }
    OriginalEvents deploy_next(const OriginalEvents& previous, const OriginalSave::State& raw) const
    {
        auto result = try_deploy_next(previous, raw);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    OriginalEvents deploy_stage2(const OriginalEvents& previous, const OriginalSave::State& raw) const { return deploy_next(previous, raw); }
    Result<OriginalSave::State> try_rebase_stage2(const OriginalEvents& next, const OriginalSave::State& settled,
        const OriginalSave::State& stage2_baseline) const
    {
        if (next.stage != 2 || get(settled.globals_4f7cd0, 8, 4) != 1 || get(stage2_baseline.globals_4f7cd0, 8, 4) != 2)
        {
            return Result<OriginalSave::State>::failure(make_error("Original stage2 full-state initialization required"));
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
        return OriginalSaveBridge::try_capture(next, 1, { 32, 16 }, &staged);
    }
    OriginalSave::State rebase_stage2(const OriginalEvents& next, const OriginalSave::State& settled, const OriginalSave::State& baseline) const
    {
        auto result = try_rebase_stage2(next, settled, baseline);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
};
}    //namespace s2ne
