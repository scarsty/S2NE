#include "app_campaign.hpp"
#include <iostream>
namespace s2ne
{
Result<ResourceDomain> try_app_domain_for_save(const OriginalSave::State& state)
{
    auto value = [](const auto& bytes, size_t at)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(bytes.at(at + i)) << (8 * i);
        }
        return int32_t(v);
    };
    auto selected = value(state.stage_4fb478, 16);
    if (selected < 0 || selected > 1)
    {
        return Result<ResourceDomain>::failure(make_error("Invalid saved logical resource domain"));
    }
    return Result<ResourceDomain>::success(static_cast<ResourceDomain>(selected));
}

ResourceDomain app_domain_for_save(const OriginalSave::State& state) { return try_app_domain_for_save(state).value(); }

OriginalEvents AppCampaignAdapter::deploy_next(const OriginalSave::State& captured) const
{
    return campaignPersistence.deploy_next(prologue->events, captured);
}

void AppCampaignAdapter::connect()
{
    prologue->enable_presentation_timing();
    prologue->set_physical_loot_commit([this](const auto& units, const auto& awards) -> Result<bool>
        {
            if (!rawSave) { return Result<bool>::failure(make_error("Original physical loot requires save baseline")); }
            auto staged = *rawSave;
            auto equipment = OriginalPromotionTransaction::create(root);
            if (!equipment) { return Result<bool>::failure(equipment.error()); }
            for (size_t group = 0; group < 2; ++group)
            {
                std::vector<OriginalEvents::Unit> members;
                std::vector<int> roles;
                for (const auto& unit : units)
                {
                    if (size_t(unit.hostile()) != group) { continue; }
                    members.push_back(unit);
                    roles.push_back(int(unit.runtime_role_selector));
                }
                auto metadata = equipment.value().try_aggregate_army_metadata(staged.groups[group].metadata[4], members, roles);
                if (!metadata) { return Result<bool>::failure(metadata.error()); }
                staged.groups[group].metadata[4] = metadata.value();
                if (prologue->events.army_status) { staged.groups[group].status = (*prologue->events.army_status)[group]; }
            }
            for (const auto& award : awards)
            {
                if (!award.killed || !(units[award.target].status_flags & 0x20)) { continue; }
                auto collected = OriginalCampaignPersistence::try_collect_physical_loot(staged, units[award.actor], units[award.target]);
                if (!collected) { return collected; }
            }
            using std::swap;
            swap(*rawSave, staged);
            return Result<bool>::success(true);
        });
    prologue->set_campaign_arrival_adapter([this](const auto& current, unsigned turn) -> std::optional<s2ne::Prologue::ArrivalTransaction>
        {
            if (!rawSave)
            {
                return std::nullopt;
            }
            auto captureResult = OriginalSaveBridge::try_capture(current, turn, prologue->camera(), &*rawSave);
            if (!captureResult)
            {
                report_error(captureResult.error());
                return std::nullopt;
            }
            auto captured = std::move(captureResult).value();
            s2ne::OriginalEvents expanded(root, current.difficulty);
            expanded.load_stage(current.stage);
            auto mapResult = initialization.try_placement_map(root, expanded.map_id);
            if (!mapResult)
            {
                report_error(mapResult.error());
                return std::nullopt;
            }
            auto insertResult = initialization.try_insert_arrivals(captured, expanded, turn, mapResult.value());
            if (!insertResult)
            {
                report_error(insertResult.error());
                return std::nullopt;
            }
            auto inserted = std::move(insertResult).value();
            auto restoreResult = OriginalSaveBridge::try_restore(root, inserted);
            if (!restoreResult)
            {
                report_error(restoreResult.error());
                return std::nullopt;
            }
            auto arrivals = std::move(restoreResult).value();
            auto pending = std::make_shared<s2ne::OriginalSave::State>(std::move(inserted));
            return s2ne::Prologue::ArrivalTransaction{ std::move(arrivals.events), [this, pending]() noexcept
                {
                    using std::swap;
                    swap(*rawSave, *pending);
                } };
        },
        rawSave ? prologue->battle_turn() : 1);
    prologue->set_campaign_scripted_roles_adapter([this](const auto& current, unsigned eventIndex) -> std::optional<s2ne::Prologue::ArrivalTransaction>
        {
            if (!rawSave)
            {
                return std::nullopt;
            }
            auto captureResult = OriginalSaveBridge::try_capture(current, prologue->battle_turn(), prologue->camera(), &*rawSave);
            if (!captureResult)
            {
                report_error(captureResult.error());
                return std::nullopt;
            }
            auto captured = std::move(captureResult).value();
            s2ne::OriginalEvents expanded(root, current.difficulty);
            expanded.load_stage(current.stage);
            auto mapResult = initialization.try_placement_map(root, expanded.map_id);
            if (!mapResult)
            {
                report_error(mapResult.error());
                return std::nullopt;
            }
            auto insertResult = initialization.try_insert_scripted_roles(captured, expanded, eventIndex, mapResult.value());
            if (!insertResult)
            {
                report_error(insertResult.error());
                return std::nullopt;
            }
            auto inserted = std::move(insertResult).value();
            auto restoreResult = OriginalSaveBridge::try_restore(root, inserted);
            if (!restoreResult)
            {
                report_error(restoreResult.error());
                return std::nullopt;
            }
            auto restored = std::move(restoreResult).value();
            auto pending = std::make_shared<s2ne::OriginalSave::State>(std::move(inserted));
            return s2ne::Prologue::ArrivalTransaction{ std::move(restored.events), [this, pending]() noexcept
                {
                    using std::swap;
                    swap(*rawSave, *pending);
                } };
        });
    prologue->set_campaign_reward_adapter([this](auto& staged, const auto& plan)
        {
            if (!rawSave)
            {
                return false;
            }
            return prologue->battle().mutate_random([this, &staged, &plan](auto& rng, uint32_t seconds)
                {
                    auto settled = campaignPersistence.try_settle(staged, plan, *rawSave, prologue->battle_turn(), rng, seconds);
                    if (!settled)
                    {
                        report_error(settled.error());
                        return false;
                    }
                    auto result = std::move(settled).value();
                    using std::swap;
                    swap(staged, result.events);
                    swap(*rawSave, result.raw);
                    return true;
                });
        });
    prologue->set_interbattle_adapter([](auto)
        {
            return s2ne::OriginalInterbattle::Result::unsupported;
        });
}
bool AppCampaignAdapter::commit_promotion(int selected, const OriginalPromotion::Request& request, size_t saveItems, const OriginalPromotionTransaction& promotionTransaction, const OriginalMap& promotionMap)
{
    {
        if (selected < 0 || size_t(selected) >= prologue->events.units.size())
        {
            return false;
        }
        if (!rawSave || !prologue->in_battle() || prologue->battle_selected() != selected || prologue->battle_state() != s2ne::Battle::State::command || prologue->computer_actions() || prologue->battle().combat_feedback_active() || prologue->battle().animating_movement())
        {
            return false;
        }
        auto stagedEvents = prologue->events;
        auto stagedBattle = prologue->battle();
        auto stagedSave = *rawSave;
        bool committed = stagedBattle.mutate_random([&](s2ne::OriginalRandom& random, uint32_t seconds)
            {
                s2ne::OriginalPromotionTransaction::State before{ stagedEvents.units.at(size_t(selected)), stagedSave.groups[0].inventory, random };
                uint8_t metadata = stagedSave.groups[0].metadata[4];
                s2ne::OriginalPromotionTransaction::Context context;
                context.game_mode = int(stagedSave.globals_4f7cd0[0]);
                context.rebuild = [&](auto& next)
                {
                    auto restored = stagedEvents.try_restore_unit_resources(next.unit);
                    if (!restored)
                    {
                        report_error(restored.error());
                        return false;
                    }
                    for (unsigned direction = 0; direction < 4; ++direction)
                    {
                        if (!promotionMap.unit_height(next.unit.appearance, direction, next.unit.animation_action))
                        {
                            return false;
                        }
                    }
                    return true;
                };
                context.recalculate_army = [&](auto& next)
                {
                    stagedEvents.units.at(size_t(selected)) = next.unit;
                    std::vector<int> roles;
                    for (const auto& record : stagedSave.groups[0].units)
                    {
                        roles.push_back(int(int16_t(unsigned(record[8]) | (unsigned(record[9]) << 8))));
                    }
                    if (roles.size() > stagedEvents.units.size())
                    {
                        return false;
                    }
                    auto allies = std::span<const s2ne::OriginalEvents::Unit>(stagedEvents.units).first(roles.size());
                    auto aggregate = promotionTransaction.try_aggregate_army_metadata(metadata, allies, roles);
                    if (!aggregate)
                    {
                        report_error(aggregate.error());
                        return false;
                    }
                    metadata = aggregate.value();
                    return true;
                };
                auto next = promotionTransaction.prepare(before, request, context, [seconds]
                    {
                        return seconds;
                    });
                if (!next)
                {
                    return false;
                }
                auto captured = OriginalSaveBridge::try_capture_promotion(stagedEvents, prologue->battle_turn(), prologue->camera(), stagedSave, size_t(selected), request.source_kind, next->inventory, metadata);
                if (!captured)
                {
                    report_error(captured.error());
                    return false;
                }
                stagedSave = std::move(captured).value();
                auto encoded = OriginalSave::try_encode(stagedSave, saveItems);
                if (!encoded)
                {
                    report_error(encoded.error());
                    return false;
                }
                stagedBattle.set_hostile_resources(stagedEvents);
                std::vector<s2ne::Battle::MovementContext> contexts(stagedEvents.units.size());
                for (size_t i = 0; i < contexts.size(); ++i)
                {
                    const auto& u = stagedEvents.units[i];
                    auto& c = contexts[i];
                    c.status34 = u.status34;
                    c.status118 = uint8_t(u.abilities >> 24);
                    c.status116 = uint8_t(u.abilities >> 8);
                    c.has_usable_ability = stagedEvents.has_learned_ability(u);
                    c.movement_class = stagedEvents.movement_class(u);
                    c.sprite_height = promotionMap.unit_height(u.appearance, u.direction, u.animation_action);
                }
                stagedBattle.set_movement_contexts(std::move(contexts));
                random = next->random;
                return true;
            });
        if (!committed)
        {
            return false;
        }
        static_assert(std::is_nothrow_swappable_v<s2ne::OriginalEvents> && std::is_nothrow_swappable_v<s2ne::Battle> && std::is_nothrow_swappable_v<s2ne::OriginalSave::State>);
        using std::swap;
        swap(prologue->events, stagedEvents);
        swap(prologue->battle(), stagedBattle);
        swap(*rawSave, stagedSave);
        return true;
    }
}
}    //namespace s2ne
