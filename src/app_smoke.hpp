#pragma once
#include "app_campaign.hpp"
#include "app_options.hpp"
#include "new_game.hpp"
#include "opening_audio.hpp"
#include "original_battle_menu.hpp"
#include "original_battle_reports.hpp"
#include "original_promotion.hpp"
#include "original_save_menu.hpp"
#include "original_save_store.hpp"
#include "original_settings.hpp"
#include "original_skills.hpp"
#include "original_smoke_exit.hpp"
#include <fstream>
#include <iostream>
namespace s2ne
{
struct AppSmokeContext
{
    const std::filesystem::path& root;
    std::unique_ptr<Prologue>& prologue;
    std::optional<OriginalSave::State>& rawSave;
    const std::optional<std::filesystem::path>& isolatedSaveRoot;
    size_t saveItems;
    SDL_Window* window;
    SDL_Renderer* renderer;
    OpeningAudio& audio;
    bool audioAvailable;
    bool inMenu;
    bool inSetup;
    bool& running;
    unsigned presented;
    uint64_t started;
    OriginalMenu& menu;
    NewGameSetup& setup;
    std::optional<OriginalBattleMenu>& battleMenu;
    std::optional<OriginalBattleReports>& battleReports;
    std::optional<OriginalSkills>& skillMenu;
    std::optional<size_t>& skillTarget;
    bool itemMenu;
    std::optional<OriginalSaveMenu>& saveMenu;
    const OriginalSettings::State& settingsState;
    OriginalSkills& skillRules;
};
struct AppSmokeState
{
    bool feedbackFlash = false, feedbackNumbers = false, feedbackHp = false, feedbackReaction = false;
    uint64_t checkedReactionDraws = 0;
    unsigned feedbackRounds = 0;
    unsigned campaignCheckpoints = 0, lastCampaignStage = 0, observedCampaignStage = 0;
    Uint64 campaignStageStarted = 0;
    unsigned exitMoveTurn = 0;
    std::optional<s2ne::OriginalCampaign::Stage> smokeExitStage;
    std::vector<std::pair<int, int>> smokeExitCells;
    bool clearVerified = false, mineralQuestVerified = false;
    unsigned diagnosticTurn = 0, continuousDiagnosticTurn = 0;
    unsigned contactStep = 0;
    unsigned skillInputPhase = 0, skillInputCaster = 0, skillInputTarget = 0, skillInputFrames = 0;
    uint64_t skillInputNext = 0, skillInputImageHash = 0;
    std::optional<s2ne::OriginalEvents::Unit> skillInputVictim;
    std::vector<uint8_t> skillInputCommitted;
    std::optional<s2ne::OriginalEvents::Unit> skillInputBefore;
    bool skillInputVerified = false;
    bool scriptDeathObserved = false, scriptFailureInjected = false;
    bool controlledAreaReady = false;
    std::vector<size_t> controlledAreaTargets;
    uint64_t controlledSeed = 0, controlledCalls = 0;
    int skillInputMpGrowth = 0;
    std::vector<uint8_t> skillCancelBaseline;
    std::ofstream skillSequence;
    unsigned promotionSmokePhase = 0, promotionSmokeKind = 0, promotionSmokeTarget = 0;
    bool promotionVerified = false;
    unsigned smokePhase = 0;
    bool battleClickPending = false;
    unsigned battleSmokeStartTurn = 0;
    unsigned hostileBannerFrames = 0, playerBannerFrames = 0, battleSmokeRounds = 0;
    s2ne::Battle::Point battleSmokeDestination{ };
    size_t battleSmokeUnit = 0;
    std::vector<std::pair<int, int>> battleSmokeEnemyOrigins;
    unsigned hostileActionFrames = 0;
    unsigned detailSmokePhase = 0;
    bool autoMenuOpened = false;
    unsigned autoFriendlyFrames = 0, autoEnemyFrames = 0;
};
class AppSmokeDriver
{
public:
    explicit AppSmokeDriver(const AppOptions& options) : options(options) { }
    AppSmokeState state;
    // A true result preserves the original driver's continue of the outer app loop.
    bool before_events(AppSmokeContext context);
    bool after_events(AppSmokeContext context, uint64_t nowUi, std::optional<OriginalPromotion>& promotionMenu, OriginalPromotion& promotionRules, bool skillCastVerified, const std::optional<size_t>& pendingSkillTarget, bool& prologueRedraw);

private:
    const AppOptions& options;
};
}    //namespace s2ne
