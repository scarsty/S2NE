#pragma once
#include "original_campaign_persistence.h"
#include "original_initialization.h"
#include "original_promotion_transaction.h"
#include "original_save_bridge.h"
#include "prologue.h"
namespace s2ne
{
Result<ResourceDomain> try_app_domain_for_save(const OriginalSave::State& state);
ResourceDomain app_domain_for_save(const OriginalSave::State& state);
ResourceDomain update_app_domain_for_stage(OriginalSave::State& state);
class AppCampaignAdapter
{
public:
    AppCampaignAdapter(const std::filesystem::path& root, std::unique_ptr<Prologue>& prologue, std::optional<OriginalSave::State>& rawSave);
    void connect();
    bool commit_promotion(int selected, const OriginalPromotion::Request& request, size_t saveItems, const OriginalPromotionTransaction& promotionTransaction, const OriginalMap& promotionMap);
    OriginalEvents deploy_next(const OriginalSave::State& captured) const;

private:
    const std::filesystem::path& root;
    std::unique_ptr<Prologue>& prologue;
    std::optional<OriginalSave::State>& rawSave;
    OriginalInitialization initialization;
    OriginalCampaignPersistence campaignPersistence;
    std::optional<OriginalSave::State> physicalLoot;
};
}    //namespace s2ne
