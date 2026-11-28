#pragma once
#include "campaign_persistence.h"
#include "initialization.h"
#include "prologue.h"
#include "promotion_transaction.h"
#include "save_bridge.h"
namespace s2ne
{
Result<ResourceDomain> try_app_domain_for_save(const Save::State& state);
ResourceDomain app_domain_for_save(const Save::State& state);
ResourceDomain update_app_domain_for_stage(Save::State& state);
class AppCampaignAdapter
{
public:
    AppCampaignAdapter(const std::filesystem::path& root, std::unique_ptr<Prologue>& prologue, std::optional<Save::State>& rawSave);
    void connect();
    bool commit_promotion(int selected, const Promotion::Request& request, size_t saveItems, const PromotionTransaction& promotionTransaction, const Map& promotionMap);
    Events deploy_next(const Save::State& captured) const;

private:
    const std::filesystem::path& root;
    std::unique_ptr<Prologue>& prologue;
    std::optional<Save::State>& rawSave;
    Initialization initialization;
    CampaignPersistence campaignPersistence;
    std::optional<Save::State> physicalLoot;
};
}    //namespace s2ne
