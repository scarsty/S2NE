#pragma once
#include "campaign.h"
#include "progression.h"
#include "save_bridge.h"

namespace s2ne
{
class CampaignPersistence
{
    std::filesystem::path root_;
    Progression progression_;
    Promotion promotion_;
    static unsigned get(std::span<const uint8_t> b, size_t p, unsigned width = 2);
    static void put(std::span<uint8_t> b, size_t p, unsigned v, unsigned width = 2);

public:
    static Result<bool> try_collect_physical_loot(Save::State& raw, const Events::Unit& actor, const Events::Unit& target);
    struct RewardState
    {
        Events events;
        Save::State raw;
    };
    CampaignPersistence() = default;
    static Result<CampaignPersistence> create(const std::filesystem::path& root);
    explicit CampaignPersistence(const std::filesystem::path& root);
    // Caller supplies copies and publishes this result together with the staged RNG.
    Result<RewardState> try_settle(Events events, const Campaign::Settlement& plan,
        const Save::State& baseline, unsigned turn, RandomStream& random, uint32_t seconds) const;
    RewardState settle(Events events, const Campaign::Settlement& plan, const Save::State& baseline, unsigned turn, RandomStream& random, uint32_t seconds) const;
    // 462224 assigns living persistent roles to generic turn1 controllable slots,
    // in persistent roster order, NOT by matching next-stage role IDs.
    Result<Events> try_deploy_next(const Events& previous, const Save::State& raw) const;
    Events deploy_next(const Events& previous, const Save::State& raw) const;
    Events deploy_stage2(const Events& previous, const Save::State& raw) const;
    Result<Save::State> try_rebase_stage2(const Events& next, const Save::State& settled,
        const Save::State& stage2_baseline) const;
    Save::State rebase_stage2(const Events& next, const Save::State& settled, const Save::State& baseline) const;
};
}    //namespace s2ne
