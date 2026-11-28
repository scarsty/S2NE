#pragma once
#include "original_campaign.h"
#include "original_progression.h"
#include "original_save_bridge.h"

namespace s2ne
{
class OriginalCampaignPersistence
{
    std::filesystem::path root_;
    OriginalProgression progression_;
    OriginalPromotion promotion_;
    static unsigned get(std::span<const uint8_t> b, size_t p, unsigned width = 2);
    static void put(std::span<uint8_t> b, size_t p, unsigned v, unsigned width = 2);

public:
    static Result<bool> try_collect_physical_loot(OriginalSave::State& raw, const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target);
    struct RewardState
    {
        OriginalEvents events;
        OriginalSave::State raw;
    };
    OriginalCampaignPersistence() = default;
    static Result<OriginalCampaignPersistence> create(const std::filesystem::path& root);
    explicit OriginalCampaignPersistence(const std::filesystem::path& root);
    // Caller supplies copies and publishes this result together with the staged RNG.
    Result<RewardState> try_settle(OriginalEvents events, const OriginalCampaign::Settlement& plan,
        const OriginalSave::State& baseline, unsigned turn, OriginalRandom& random, uint32_t seconds) const;
    RewardState settle(OriginalEvents events, const OriginalCampaign::Settlement& plan, const OriginalSave::State& baseline, unsigned turn, OriginalRandom& random, uint32_t seconds) const;
    // 462224 assigns living persistent roles to generic turn1 controllable slots,
    // in persistent roster order, NOT by matching next-stage role IDs.
    Result<OriginalEvents> try_deploy_next(const OriginalEvents& previous, const OriginalSave::State& raw) const;
    OriginalEvents deploy_next(const OriginalEvents& previous, const OriginalSave::State& raw) const;
    OriginalEvents deploy_stage2(const OriginalEvents& previous, const OriginalSave::State& raw) const;
    Result<OriginalSave::State> try_rebase_stage2(const OriginalEvents& next, const OriginalSave::State& settled,
        const OriginalSave::State& stage2_baseline) const;
    OriginalSave::State rebase_stage2(const OriginalEvents& next, const OriginalSave::State& settled, const OriginalSave::State& baseline) const;
};
}    //namespace s2ne
