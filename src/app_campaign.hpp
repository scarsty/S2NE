#pragma once
#include "original_campaign_persistence.hpp"
#include "original_initialization.hpp"
#include "original_promotion_transaction.hpp"
#include "original_save_bridge.hpp"
#include "prologue.hpp"
namespace s2ne
{
Result<ResourceDomain> try_app_domain_for_save(const OriginalSave::State& state);
ResourceDomain app_domain_for_save(const OriginalSave::State& state);
class AppCampaignAdapter
{
public:
    AppCampaignAdapter(const std::filesystem::path& root, std::unique_ptr<Prologue>& prologue, std::optional<OriginalSave::State>& rawSave) : root(root), prologue(prologue), rawSave(rawSave), initialization(root), campaignPersistence(root) { }
    void connect();
    bool commit_promotion(int selected, const OriginalPromotion::Request& request, size_t saveItems, const OriginalPromotionTransaction& promotionTransaction, const OriginalMap& promotionMap);
    OriginalEvents deploy_next(const OriginalSave::State& captured) const;

private:
    const std::filesystem::path& root;
    std::unique_ptr<Prologue>& prologue;
    std::optional<OriginalSave::State>& rawSave;
    OriginalInitialization initialization;
    OriginalCampaignPersistence campaignPersistence;
};
}    //namespace s2ne
