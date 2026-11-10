#include "app_smoke.hpp"
#include "runtime_error.hpp"
#include <windows.h>
namespace s2ne
{
static std::string campaign_name_utf8(const s2ne::OriginalCampaign::Stage& stage)
{
    if (stage.name.empty())
    {
        return "（原资源未命名）";
    }
    const auto* text = reinterpret_cast<const char*>(stage.name.data());
    int count = MultiByteToWideChar(950, 0, text, int(stage.name.size()), nullptr, 0);
    if (!count)
    {
        s2ne::fatal("Cannot decode original campaign name");
    }
    std::wstring wide(size_t(count), L'\0');
    MultiByteToWideChar(950, 0, text, int(stage.name.size()), wide.data(), count);
    int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, nullptr, 0, nullptr, nullptr);
    if (!bytes)
    {
        s2ne::fatal("Cannot encode original campaign name");
    }
    std::string result(size_t(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, result.data(), bytes, nullptr, nullptr);
    return result;
}

bool AppSmokeDriver::before_events(AppSmokeContext context)
{
    const auto& verify = options.verify;
    const auto& teleportSmoke = options.teleportSmoke;
    const auto& evacuationSmoke = options.evacuationSmoke;
    const auto& shopSmoke = options.shopSmoke;
    const auto& openingRightSmoke = options.openingRightSmoke;
    const auto& smoke = options.smoke;
    const auto& newGameSmoke = options.newGameSmoke;
    const auto& battleSmoke = options.battleSmoke;
    const auto& alternateSmoke = options.alternateSmoke;
    const auto& autoSmoke = options.autoSmoke;
    const auto& detailSmoke = options.detailSmoke;
    const auto& feedbackSmoke = options.feedbackSmoke;
    const auto& nataliaContinue = options.nataliaContinue;
    const auto& nataliaStrengthened = options.nataliaStrengthened;
    const auto& strengthenedFlow = options.strengthenedFlow;
    const auto& campaignContinue = options.campaignContinue;
    const auto& campaignSmoke = options.campaignSmoke;
    const auto& clearSmoke = options.clearSmoke;
    const auto& loadSmoke = options.loadSmoke;
    const auto& skill88Input = options.skill88Input;
    const auto& skill94Input = options.skill94Input;
    const auto& skillInputSmoke = options.skillInputSmoke;
    const auto& skill88Positive = options.skill88Positive;
    const auto& inputSkill = options.inputSkill;
    const auto& inputFolder = options.inputFolder;
    const auto& skillScheduleStress = options.skillScheduleStress;
    const auto& promotionSmoke = options.promotionSmoke;
    const auto& option = options.option;
    auto& feedbackFlash = state.feedbackFlash;
    auto& feedbackNumbers = state.feedbackNumbers;
    auto& feedbackHp = state.feedbackHp;
    auto& feedbackReaction = state.feedbackReaction;
    auto& checkedReactionDraws = state.checkedReactionDraws;
    auto& feedbackRounds = state.feedbackRounds;
    auto& campaignCheckpoints = state.campaignCheckpoints;
    auto& lastCampaignStage = state.lastCampaignStage;
    auto& observedCampaignStage = state.observedCampaignStage;
    auto& campaignStageStarted = state.campaignStageStarted;
    auto& exitMoveTurn = state.exitMoveTurn;
    auto& smokeExitStage = state.smokeExitStage;
    auto& smokeExitCells = state.smokeExitCells;
    auto& clearVerified = state.clearVerified;
    auto& mineralQuestVerified = state.mineralQuestVerified;
    auto& diagnosticTurn = state.diagnosticTurn;
    auto& continuousDiagnosticTurn = state.continuousDiagnosticTurn;
    auto& contactStep = state.contactStep;
    auto& skillInputPhase = state.skillInputPhase;
    auto& skillInputCaster = state.skillInputCaster;
    auto& skillInputTarget = state.skillInputTarget;
    auto& skillInputFrames = state.skillInputFrames;
    auto& skillInputNext = state.skillInputNext;
    auto& skillInputImageHash = state.skillInputImageHash;
    auto& skillInputVictim = state.skillInputVictim;
    auto& skillInputCommitted = state.skillInputCommitted;
    auto& skillInputBefore = state.skillInputBefore;
    auto& skillInputVerified = state.skillInputVerified;
    auto& skillInputMpGrowth = state.skillInputMpGrowth;
    auto& skillCancelBaseline = state.skillCancelBaseline;
    auto& skillSequence = state.skillSequence;
    auto& promotionSmokePhase = state.promotionSmokePhase;
    auto& promotionSmokeKind = state.promotionSmokeKind;
    auto& promotionSmokeTarget = state.promotionSmokeTarget;
    auto& promotionVerified = state.promotionVerified;
    auto& smokePhase = state.smokePhase;
    auto& battleClickPending = state.battleClickPending;
    auto& battleSmokeStartTurn = state.battleSmokeStartTurn;
    auto& hostileBannerFrames = state.hostileBannerFrames;
    auto& playerBannerFrames = state.playerBannerFrames;
    auto& battleSmokeRounds = state.battleSmokeRounds;
    auto& battleSmokeDestination = state.battleSmokeDestination;
    auto& battleSmokeUnit = state.battleSmokeUnit;
    auto& battleSmokeEnemyOrigins = state.battleSmokeEnemyOrigins;
    auto& hostileActionFrames = state.hostileActionFrames;
    auto& detailSmokePhase = state.detailSmokePhase;
    auto& autoMenuOpened = state.autoMenuOpened;
    auto& autoFriendlyFrames = state.autoFriendlyFrames;
    auto& autoEnemyFrames = state.autoEnemyFrames;
    auto& root = context.root;
    auto& prologue = context.prologue;
    auto& rawSave = context.rawSave;
    auto& isolatedSaveRoot = context.isolatedSaveRoot;
    auto& saveItems = context.saveItems;
    auto& window = context.window;
    auto& renderer = context.renderer;
    auto& audio = context.audio;
    auto& audioAvailable = context.audioAvailable;
    auto& inMenu = context.inMenu;
    auto& inSetup = context.inSetup;
    auto& running = context.running;
    auto& presented = context.presented;
    auto& started = context.started;
    auto& menu = context.menu;
    auto& setup = context.setup;
    auto& battleMenu = context.battleMenu;
    auto& battleReports = context.battleReports;
    auto& skillMenu = context.skillMenu;
    auto& skillTarget = context.skillTarget;
    auto& itemMenu = context.itemMenu;
    auto& saveMenu = context.saveMenu;
    auto& settingsState = context.settingsState;
    auto& skillRules = context.skillRules;
    if (smoke && smokePhase == 0 && SDL_GetTicks() - started > 300)
    {
        SDL_Event e{ };
        if (openingRightSmoke)
        {
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.button = SDL_BUTTON_RIGHT;
        }
        else
        {
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = SDLK_ESCAPE;
        }
        SDL_PushEvent(&e);
        ++smokePhase;
    }
    if (smoke && smokePhase == 1 && SDL_GetTicks() - started > 600)
    {
        SDL_Event e{ };
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = SDLK_DOWN;
        SDL_PushEvent(&e);
        e.key.key = SDLK_RETURN;
        SDL_PushEvent(&e);
        ++smokePhase;
    }
    if (smoke && SDL_GetTicks() - started > 1200)
    {
        if ((audioAvailable && !audio.progressing()) || !inMenu || menu.selected != 1 || menu.activated != 1)
        {
            s2ne::fatal("Audio/menu smoke test failed");
        }
        std::cout << (audioAvailable ? "Audio playback position advanced\n" : "Silent mode verified\n");
        running = false;
    }
    if (battleSmoke && SDL_GetTicks() - started > 100 + smokePhase * 100)
    {
        SDL_Event e{ };
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = smokePhase == 0 ? SDLK_ESCAPE : SDLK_RETURN;
        if (inSetup && smokePhase == 2)
        {
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.windowID = SDL_GetWindowID(window);
            e.button.button = SDL_BUTTON_LEFT;
            SDL_RenderCoordinatesToWindow(renderer, 300, 280, &e.button.x, &e.button.y);
        }
        if (inSetup && smokePhase == 3)
        {
            if (setup.difficulty != 0 || !inSetup)
            {
                s2ne::fatal("Difficulty page did not retain easiest mouse selection");
            }
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.windowID = SDL_GetWindowID(window);
            e.button.button = SDL_BUTTON_LEFT;
            SDL_RenderCoordinatesToWindow(renderer, 300, 425, &e.button.x, &e.button.y);
        }
        if (prologue && prologue->choosing() && ((alternateSmoke && prologue->events.stage == 140) || (campaignContinue && prologue->events.stage == 10)) && prologue->selection() == 0)
        {
            e.key.key = SDLK_DOWN;
        }
        if (campaignContinue && prologue && prologue->events.stage != observedCampaignStage)
        {
            observedCampaignStage = prologue->events.stage;
            campaignStageStarted = SDL_GetTicks();
            std::cout << "Continuous campaign pending stage=" << prologue->events.stage << " name=" << campaign_name_utf8(s2ne::OriginalCampaign(root).stage(prologue->events.stage)) << " battle=" << prologue->in_battle() << " dialogue=" << prologue->campaign_dialogue() << '\n';
        }
        if (campaignContinue && prologue && !prologue->campaign_blocker().empty())
        {
            if (rawSave && nataliaStrengthened)
            {
                auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage" + std::to_string(prologue->events.stage) + "-blocked.sav");
                auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                std::ofstream output(path, std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                output.close();
                if (!output || s2ne::OriginalSave::encode(s2ne::OriginalSave::load(path, saveItems), saveItems) != bytes)
                {
                    s2ne::fatal("Natalia blocker checkpoint persistence failed");
                }
                std::cout << "Natalia blocker checkpoint=" << path.string() << '\n';
            }
            if (rawSave && (prologue->events.stage == 61 || prologue->events.stage == 131))
            {
                auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-terminal-pending.sav");
                auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                std::ofstream output(path, std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                output.close();
                std::cout << "Native terminal checkpoint=" << path.string() << " flag42=" << unsigned(prologue->events.flags[41]) << " flag43=" << unsigned(prologue->events.flags[42]) << " flag73=" << unsigned(prologue->events.flags[72]) << " flag75=" << unsigned(prologue->events.flags[74]) << '\n';
            }
            s2ne::fatal("Continuous campaign blocked at stage=" + std::to_string(prologue->events.stage) + ": " + prologue->campaign_blocker());
        }
        if (campaignContinue && prologue && prologue->ending_active())
        {
            e.key.key = SDLK_RETURN;
            SDL_PushEvent(&e);
        }
        if (campaignContinue && strengthenedFlow && skillMenu)
        {
            e.key.key = SDLK_RETURN;
            if (skillMenu->page() == s2ne::OriginalSkills::Page::list && skillMenu->selected().id != 32)
            {
                bool groupHas = false;
                for (auto& r : skillMenu->groups().at(skillMenu->selected_group()).records)
                {
                    groupHas |= r.id == 32;
                }
                e.key.key = groupHas ? SDLK_DOWN : SDLK_RIGHT;
            }
            else if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && skillTarget && prologue->events.units.at(*skillTarget).id != 20)
            {
                e.key.key = SDLK_TAB;
            }
            SDL_PushEvent(&e);
        }
        if (campaignContinue && prologue && prologue->in_battle() && prologue->events.stage != lastCampaignStage)
        {
            if (!rawSave || !prologue->autosave_requests().empty())
            {
                s2ne::fatal("Continuous battle entered before autosave committed");
            }
            auto saved = s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot, 52), saveItems);
            auto restored = s2ne::OriginalSaveBridge::restore(root, saved);
            if (restored.events.stage != prologue->events.stage)
            {
                s2ne::fatal("Continuous autosave stage mismatch");
            }
            lastCampaignStage = prologue->events.stage;
            ++campaignCheckpoints;
            std::cout << "Continuous campaign checkpoint=" << campaignCheckpoints << " stage=" << lastCampaignStage << " original autosave reload verified\n";
            auto directory = root / "S2NE" / "out" / (strengthenedFlow ? "strengthened-flow-acceptance" : "campaign-acceptance");
            std::filesystem::create_directories(directory);
            auto path = directory / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage" + std::to_string(lastCampaignStage) + "-start.sav");
            auto bytes = s2ne::OriginalSave::encode(saved, saveItems);
            std::ofstream output(path, std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
            if (!output)
            {
                s2ne::fatal("Cannot persist continuous campaign checkpoint");
            }
            output.close();
            auto reloaded = s2ne::OriginalSave::load(path, saveItems);
            if (s2ne::OriginalSave::encode(reloaded, saveItems) != bytes)
            {
                s2ne::fatal("Continuous checkpoint changed on reload");
            }
            auto savedGlobal = [&](size_t at)
            {
                uint32_t value = 0;
                for (unsigned i = 0; i < 4; ++i)
                {
                    value |= uint32_t(saved.stage_4fb478.at(at + i)) << (8 * i);
                }
                return value;
            };
            auto disc = app_domain_for_save(saved);
            auto discBytes = s2ne::OriginalDiscResources(root).dat_entry(disc, 5);
            std::cout << "Continuous native checkpoint path=" << path.string() << " route=" << savedGlobal(12) << " progress=" << savedGlobal(8) << " resource-domain=" << unsigned(disc) << " DAT5bytes=" << discBytes.size() << '\n';
            if (nataliaStrengthened && !nataliaContinue && disc == s2ne::OriginalDisc::two)
            {
                std::cout << "Natalia bounded batch reached first secondary-domain battle; not full route or ending evidence\n";
                clearVerified = true;
                running = false;
            }
        }
        if (options.freshSaveSmoke && prologue && prologue->in_battle() && !prologue->campaign_modal())
        {
            if (!rawSave || !isolatedSaveRoot || !prologue->autosave_requests().empty())
            {
                s2ne::fatal("Fresh normal battle entered without initialized autosave");
            }
            auto captured = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
            if (!captured)
            {
                s2ne::fatal(captured.error());
            }
            auto written = s2ne::OriginalSaveStore::try_write(*isolatedSaveRoot, 52, captured.value(), saveItems);
            if (!written)
            {
                s2ne::fatal(written.error());
            }
            auto loaded = s2ne::OriginalSave::try_load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot, 52), saveItems);
            if (!loaded || s2ne::OriginalSave::encode(loaded.value(), saveItems) != s2ne::OriginalSave::encode(captured.value(), saveItems))
            {
                s2ne::fatal("Fresh normal game save byte roundtrip failed");
            }
            auto restored = s2ne::OriginalSaveBridge::try_restore(root, loaded.value());
            if (!restored || restored.value().events.stage != prologue->events.stage)
            {
                s2ne::fatal("Fresh normal game restore failed");
            }
            std::cout << "SDL normal title/setup/opening -> fresh stage=" << prologue->events.stage << " name=" << campaign_name_utf8(s2ne::OriginalCampaign(root).stage(prologue->events.stage)) << " original initialization/save/reload verified; no source SAVE or injected flags/items/stats\n";
            running = false;
            return true;
        }
        if (campaignSmoke && !campaignContinue && prologue && prologue->events.stage == (alternateSmoke ? 72u : 2u) && prologue->in_battle())
        {
            if (!rawSave || !prologue->autosave_requests().empty())
            {
                s2ne::fatal("Stage2 entered before original autosaves committed");
            }
            auto saved = s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot, 52), saveItems);
            auto restored = s2ne::OriginalSaveBridge::restore(root, saved);
            auto recaptured = s2ne::OriginalSaveBridge::try_capture(restored.events, restored.turn, restored.camera, &restored.raw);
            if (!recaptured || s2ne::OriginalSave::encode(recaptured.value(), saveItems) != s2ne::OriginalSave::encode(saved, saveItems))
            {
                s2ne::fatal("Next-battle complete persistent state did not roundtrip");
            }
            if (restored.events.stage != (alternateSmoke ? 72u : 2u))
            {
                s2ne::fatal("Next-battle autosave restored the wrong stage");
            }
            std::cout << "SDL campaign victory -> rewards -> hub -> next battle stage=" << restored.events.stage << " name=" << campaign_name_utf8(s2ne::OriginalCampaign(root).stage(restored.events.stage)) << " -> original autosave complete byte reload verified\n";
            clearVerified = true;
            running = false;
        }
        else if (campaignSmoke && prologue && prologue->interbattle_active())
        {
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.button = SDL_BUTTON_LEFT;
            SDL_RenderCoordinatesToWindow(renderer, 100, 450, &e.button.x, &e.button.y);
            SDL_PushEvent(&e);
        }
        else if (clearSmoke && prologue && prologue->campaign_reward_report())
        {
            std::cout << "SDL original first-battle scripts and victory report verified at turn=" << prologue->battle_turn() << "\n";
            if (prologue->battle_turn() > 99)
            {
                s2ne::fatal("Victory report exceeded original turn deadline");
            }
            if (campaignSmoke)
            {
                e.key.key = SDLK_RETURN;
                SDL_PushEvent(&e);
            }
            else
            {
                clearVerified = true;
                running = false;
            }
        }
        else if (clearSmoke && prologue && prologue->campaign_defeated())
        {
            std::cerr << "Campaign defeat stage=" << prologue->events.stage << " turn=" << prologue->battle_turn() << " difficulty=" << prologue->events.difficulty << '\n';
            for (const auto& u : prologue->events.units)
            {
                std::cerr << " unit=" << u.id << " behavior=" << u.behavior << " hp=" << u.hp << " status=" << unsigned(u.status_flags) << " x=" << u.x << " y=" << u.y << '\n';
            }
            s2ne::fatal("Production first-battle smoke reached original defeat");
        }
        else if (campaignContinue && strengthenedFlow && prologue && rawSave && (prologue->events.stage == 61 || prologue->events.stage == 131) && prologue->in_battle() && rawSave->groups[0].inventory.at(300) > 0 && std::any_of(prologue->events.units.begin(), prologue->events.units.end(), [&](const auto& u)
                     {
                         return u.id == s2ne::smoke_exit::protagonist(prologue->events) && u.controllable() && !(u.learned[2] & 0x800);
                     }))
        {
            auto& battle = prologue->battle();
            if (!battle.animating_movement() && !battle.combat_feedback_active())
            {
                if (itemMenu)
                {
                    SDL_Event key{ };
                    key.type = SDL_EVENT_KEY_DOWN;
                    key.key.key = SDLK_RETURN;
                    SDL_PushEvent(&key);
                }
                else if (battle.state() == s2ne::Battle::State::idle)
                {
                    auto hero = std::find_if(prologue->events.units.begin(), prologue->events.units.end(), [&](const auto& u)
                        {
                            return u.id == s2ne::smoke_exit::protagonist(prologue->events) && u.controllable();
                        });
                    auto p = prologue->screen_cell({ hero->x, hero->y });
                    prologue->pan(p.x - 320, p.y - 240);
                    p = prologue->screen_cell({ hero->x, hero->y });
                    prologue->mouse(float(p.x), float(p.y), true);
                }
                else if (battle.state() == s2ne::Battle::State::command)
                {
                    battle.prepare_commands(prologue->events.units);
                    SDL_Event key{ };
                    key.type = SDL_EVENT_KEY_DOWN;
                    key.key.key = battle.command_index() == 2 ? SDLK_RETURN : SDLK_RIGHT;
                    SDL_PushEvent(&key);
                }
            }
        }
        else if (clearSmoke && prologue && prologue->in_battle() && !itemMenu)
        {
            unsigned friendly = 0, hostile = 0;
            for (const auto& unit : prologue->events.units)
            {
                if (unit.present(prologue->battle_turn()) && (unit.status_flags & 0x80) && !(unit.status_flags & 0x20))
                {
                    if (unit.hostile())
                    {
                        ++hostile;
                    }
                    else if (unit.controllable())
                    {
                        ++friendly;
                    }
                }
            }
            bool livingAlliedNpc = std::any_of(prologue->events.units.begin(), prologue->events.units.end(), [&](const auto& unit)
                {
                    return !unit.hostile() && unit.behavior == 1 && unit.present(prologue->battle_turn()) && (unit.status_flags & 0x80) && !(unit.status_flags & 0x20);
                });
            if ((!hostile && !campaignContinue) || (!friendly && !(campaignContinue && livingAlliedNpc)))
            {
                std::cout << "SDL first battle outcome: turn=" << prologue->battle_turn() << " friendly=" << friendly << " hostile=" << hostile << "\n";
                for (const auto& unit : prologue->events.units)
                {
                    if (unit.controllable())
                    {
                        std::cout << " actor=" << unit.id << " hp=" << unit.hp << " level=" << unit.level << " xp=" << unit.experience << "\n";
                    }
                }
                if (!friendly)
                {
                    s2ne::fatal("First battle production automatic actions lost; not a verified victory");
                }
                s2ne::fatal("Enemy elimination reached; original victory settlement must also complete");
            }
            if (prologue->battle_turn() > 100)
            {
                s2ne::fatal("First battle production automatic actions failed to clear within100 turns");
            }
            if (!battleSmokeStartTurn)
            {
                battleSmokeStartTurn = prologue->battle_turn();
            }
            if (campaignContinue && (!smokeExitStage || smokeExitStage->id != prologue->events.stage))
            {
                smokeExitStage = s2ne::OriginalCampaign(root).stage(prologue->events.stage);
                smokeExitCells.clear();
                exitMoveTurn = 0;
            }
            auto revealInput = campaignContinue && strengthenedFlow && prologue->battle_turn() > 1 ? s2ne::smoke_exit::reveal_input(prologue->battle(), prologue->events) : std::nullopt;
            auto mineralInput = campaignContinue && strengthenedFlow ? s2ne::smoke_exit::campaign_mineral_input(prologue->battle(), prologue->events) : std::nullopt;
            if (campaignContinue && strengthenedFlow && (prologue->events.stage == 61 || prologue->events.stage == 131) && exitMoveTurn != prologue->battle_turn())
            {
                for (const auto& boss : prologue->events.units)
                {
                    if (boss.id == 20)
                    {
                        std::cout << "Quest boss id=" << boss.id << " behavior=" << boss.behavior << " hp=" << boss.hp << " status=" << boss.status_flags << " xy=" << boss.x << ',' << boss.y << " accessory=" << boss.equipment[5] << " questRoute=" << bool(mineralInput) << '\n';
                    }
                }
            }
            auto padInput = mineralInput ? mineralInput : campaignContinue && strengthenedFlow ? s2ne::smoke_exit::campaign_pad_input(prologue->battle(), prologue->events) :
                                                                                                 std::nullopt;
            if (padInput)
            {
                revealInput = padInput;
            }
            bool captureInput = campaignContinue && strengthenedFlow && hostile == 0 && s2ne::smoke_exit::capture_ready(prologue->events);
            bool unresolvedFinalQuest = campaignContinue && strengthenedFlow && (prologue->events.stage == 61 || prologue->events.stage == 131) && std::any_of(prologue->events.units.begin(), prologue->events.units.end(), [](const auto& u)
                                            {
                                                return u.id == 20 && u.hostile() && (u.status_flags & 0x80) && !(u.status_flags & 0x20) && std::find(u.equipment.begin(), u.equipment.end(), 237u) != u.equipment.end();
                                            });
            if (unresolvedFinalQuest && (!mineralInput || (!nataliaContinue && prologue->battle_turn() > 30)))
            {
                auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-quest-route-blocked.sav");
                auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                std::ofstream output(path, std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                output.close();
                s2ne::fatal("Final quest approach requires next legal route; refusing blind auto while boss237 remains; checkpoint=" + path.string());
            }
            if (skillMenu) { }
            else if (campaignContinue && (s2ne::smoke_exit::ready(*smokeExitStage, prologue->events, hostile) || revealInput || captureInput))
            {
                auto& battle = prologue->battle();
                if (!battle.animating_movement() && !battle.combat_feedback_active())
                {
                    auto actor = std::find_if(prologue->events.units.begin(), prologue->events.units.end(), [&](const auto& u)
                        {
                            return u.id == (revealInput ? revealInput->actor : s2ne::smoke_exit::protagonist(prologue->events)) && !u.hostile() && u.present() && (u.status_flags & 0x80) && !(u.status_flags & 0x20);
                        });
                    if (actor == prologue->events.units.end())
                    {
                        s2ne::fatal("Exit protagonist unavailable");
                    }
                    using Point = s2ne::Battle::Point;
                    Point origin{ actor->x, actor->y };
                    if (!revealInput && !captureInput && smokeExitCells.empty())
                    {
                        smokeExitCells = s2ne::smoke_exit::cells(root, prologue->events);
                    }
                    auto exits = revealInput ? std::vector<std::pair<int, int>>{ revealInput->destination } : smokeExitCells;
                    if (captureInput)
                    {
                        exits.clear();
                        s2ne::OriginalMap original(root);
                        original.load(prologue->events.map_id);
                        for (unsigned y = 0; y < original.height * 2; ++y)
                        {
                            for (unsigned x = 0; x < original.width - (y & 1); ++x)
                            {
                                if (battle.object_category({ int(x), int(y) }) == 4)
                                {
                                    exits.emplace_back(int(x), int(y));
                                }
                            }
                        }
                    }
                    std::cout << "Exit input turn=" << prologue->battle_turn() << " hero=" << origin.x << "," << origin.y << " ap=" << actor->action_points << " state=" << int(prologue->battle_state()) << " selected=" << prologue->battle_selected() << " committedTurn=" << exitMoveTurn << '\n';
                    std::vector<Point> queue{ origin };
                    std::map<std::pair<int, int>, Point> previous;
                    previous[{ origin.x, origin.y }] = origin;
                    std::optional<Point> destination;
                    for (size_t i = 0; i < queue.size() && !destination; ++i)
                    {
                        auto p = queue[i];
                        if (std::find(exits.begin(), exits.end(), std::pair{ p.x, p.y }) != exits.end() && (p == origin || battle.occupant(prologue->events.units, p) < 0))
                        {
                            destination = p;
                            break;
                        }
                        for (int dy : { -1, 1 })
                        {
                            for (int dx : { 0, (p.y & 1) ? 1 : -1 })
                            {
                                Point q{ p.x + dx, p.y + dy };
                                if (previous.contains({ q.x, q.y }) || battle.step_cost(*actor, p, q) < 0 || !battle.player_route_passable(prologue->events.units, *actor, q))
                                {
                                    continue;
                                }
                                previous[{ q.x, q.y }] = p;
                                queue.push_back(q);
                            }
                        }
                    }
                    if (!destination)
                    {
                        auto checkpoint = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto directory = root / "S2NE" / "out" / (strengthenedFlow ? "strengthened-flow-acceptance" : "campaign-acceptance");
                        std::filesystem::create_directories(directory);
                        auto path = directory / ("stage" + std::to_string(prologue->events.stage) + "-route-blocked-" + std::to_string(GetCurrentProcessId()) + ".sav");
                        auto bytes = s2ne::OriginalSave::encode(checkpoint, saveItems);
                        std::ofstream output(path, std::ios::binary);
                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                        if (!output)
                        {
                            s2ne::fatal("Cannot persist actual blocked-route checkpoint");
                        }
                        if (nataliaContinue && hostile > 0)
                        {
                            std::cout << "Temporary occupied objective route stage=" << prologue->events.stage << " turn=" << prologue->battle_turn() << " hostiles=" << hostile << "; normal automatic combat then retry; checkpoint=" << path.string() << '\n';
                            output.close();
                            prologue->cancel_battle_selection();
                            prologue->start_auto_battle();
                            return true;
                        }
                        s2ne::fatal("No legal route to original stage" + std::to_string(prologue->events.stage) + " exit; genuine checkpoint=" + path.string() + " origin=" + std::to_string(origin.x) + "," + std::to_string(origin.y));
                    }
                    if (padInput && *destination == origin)
                    {
                        if (mineralInput && (actor->learned[2] & 0x800) && battle.object_category(origin) != 35 && std::any_of(prologue->events.units.begin(), prologue->events.units.end(), [&](const auto& u)
                                {
                                    return u.id == 20 && u.hostile() && std::max(std::abs(2 * (u.x - actor->x) + int(u.y & 1) - int(actor->y & 1)), std::abs(u.y - actor->y)) <= 4;
                                }))
                        {
                            bool amuletRemaining = std::any_of(prologue->events.units.begin(), prologue->events.units.end(), [](const auto& u)
                                {
                                    return u.id == 20 && u.hostile() && std::find(u.equipment.begin(), u.equipment.end(), 237u) != u.equipment.end();
                                });
                            if (!amuletRemaining)
                            {
                                if (battle.state() == s2ne::Battle::State::movement)
                                {
                                    prologue->cancel_battle_selection();
                                }
                                else if (battle.state() == s2ne::Battle::State::idle)
                                {
                                    auto p = prologue->screen_cell(origin);
                                    auto camera = prologue->camera();
                                    for (int dy = -32; dy <= 32 && battle.state() == s2ne::Battle::State::idle; ++dy)
                                    {
                                        for (int dx = -32; dx <= 32; ++dx)
                                        {
                                            if (battle.pick_surface(float(p.x + dx), float(p.y + dy), camera.first, camera.second) == origin)
                                            {
                                                prologue->mouse(float(p.x + dx), float(p.y + dy), true);
                                                break;
                                            }
                                        }
                                    }
                                }
                                else if (battle.state() == s2ne::Battle::State::command)
                                {
                                    for (unsigned i = 0; i < 6 && prologue->battle_command_index() != 0; ++i)
                                    {
                                        prologue->battle_cycle(1);
                                    }
                                    if (prologue->battle_command_index() == 0)
                                    {
                                        prologue->battle_confirm();
                                    }
                                    else
                                    {
                                        prologue->end_battle_turn();
                                    }
                                }
                                else if (battle.state() == s2ne::Battle::State::attack_menu)
                                {
                                    for (unsigned i = 0; i < 8 && battle.attack_index() != 0; ++i)
                                    {
                                        prologue->battle_cycle(1);
                                    }
                                    prologue->battle_confirm();
                                }
                                else if (battle.state() == s2ne::Battle::State::attack_target)
                                {
                                    for (unsigned i = 0; i < prologue->events.units.size() && battle.target() >= 0 && prologue->events.units.at(size_t(battle.target())).id != 20; ++i)
                                    {
                                        prologue->battle_cycle(1);
                                    }
                                    prologue->battle_confirm();
                                }
                            }
                            else if (battle.state() == s2ne::Battle::State::movement) { prologue->cancel_battle_selection(); }
                            else if (battle.state() == s2ne::Battle::State::idle)
                            {
                                auto p = prologue->screen_cell(origin);
                                prologue->pan(p.x - 320, p.y - 240);
                                p = prologue->screen_cell(origin);
                                auto camera = prologue->camera();
                                bool clicked = false;
                                for (int dy = -32; dy <= 32 && !clicked; ++dy)
                                {
                                    for (int dx = -32; dx <= 32; ++dx)
                                    {
                                        if (battle.pick_surface(float(p.x + dx), float(p.y + dy), camera.first, camera.second) == origin)
                                        {
                                            prologue->mouse(float(p.x + dx), float(p.y + dy), true);
                                            clicked = true;
                                            break;
                                        }
                                    }
                                }
                                if (!clicked)
                                {
                                    s2ne::fatal("Spell caster has no original selection pixel");
                                }
                            }
                            else if (battle.state() == s2ne::Battle::State::command)
                            {
                                if (!skillRules.entry_available(*actor, { })) { prologue->end_battle_turn(); }
                                else
                                {
                                    for (unsigned i = 0; i < 6 && prologue->battle_command_index() != 0; ++i)
                                    {
                                        prologue->battle_cycle(1);
                                    }
                                    if (prologue->battle_command_index() != 0)
                                    {
                                        s2ne::fatal("Magic attack entry unavailable despite skill AP eligibility");
                                    }
                                    prologue->battle_confirm();
                                }
                            }
                            else if (battle.state() == s2ne::Battle::State::attack_menu)
                            {
                                if (!battle.attack_command_available(prologue->events.units, 2)) { prologue->end_battle_turn(); }
                                else
                                {
                                    for (unsigned i = 0; i < 8 && battle.attack_index() != int((skill88Input || skill94Input) ? 3 : 2); ++i)
                                    {
                                        prologue->battle_cycle(1);
                                    }
                                    prologue->battle_confirm();
                                }
                            }
                        }
                        else
                        {
                            prologue->end_battle_turn();
                        }
                    }
                    if (*destination != origin)
                    {
                        if (exitMoveTurn == prologue->battle_turn()) { prologue->end_battle_turn(); }
                        else if (prologue->battle_state() == s2ne::Battle::State::idle)
                        {
                            auto p = prologue->screen_cell(origin);
                            prologue->pan(p.x - 320, p.y - 240);
                            p = prologue->screen_cell(origin);
                            auto camera = prologue->camera();
                            std::optional<Point> click;
                            for (int dy = -32; dy <= 32 && !click; ++dy)
                            {
                                for (int dx = -32; dx <= 32; ++dx)
                                {
                                    Point pixel{ p.x + dx, p.y + dy };
                                    if (pixel.x < 0 || pixel.x >= 640 || pixel.y < 0 || pixel.y >= 480)
                                    {
                                        continue;
                                    }
                                    if (battle.pick_surface(float(pixel.x), float(pixel.y), camera.first, camera.second) == origin)
                                    {
                                        click = pixel;
                                        break;
                                    }
                                }
                            }
                            if (!click)
                            {
                                s2ne::fatal("Exit protagonist has no verified selection pixel");
                            }
                            prologue->mouse(float(click->x), float(click->y), true);
                            if (prologue->battle_selected() != int(actor - prologue->events.units.begin()))
                            {
                                s2ne::fatal("Exit click selected wrong actor");
                            }
                        }
                        else if (prologue->battle_state() == s2ne::Battle::State::command)
                        {
                            for (unsigned i = 0; i < 6 && prologue->battle_command_index() != 1; ++i)
                            {
                                prologue->battle_cycle(1);
                            }
                            prologue->battle_confirm();
                        }
                        else if (prologue->battle_state() == s2ne::Battle::State::movement)
                        {
                            std::vector<Point> path;
                            Point cursor = *destination;
                            while (cursor != origin)
                            {
                                path.push_back(cursor);
                                cursor = previous.at({ cursor.x, cursor.y });
                            }
                            std::optional<Point> reachable;
                            for (auto point : path)
                            {
                                if (battle.preview(point))
                                {
                                    reachable = point;
                                    break;
                                }
                            }
                            if (!reachable)
                            {
                                prologue->end_battle_turn();
                            }
                            else
                            {
                                auto p = prologue->screen_cell(*reachable);
                                prologue->pan(p.x - 320, p.y - 240);
                                p = prologue->screen_cell(*reachable);
                                auto camera = prologue->camera();
                                std::optional<Point> click;
                                for (int radius = 0; radius <= 64 && !click; ++radius)
                                {
                                    for (int dy = -radius; dy <= radius && !click; ++dy)
                                    {
                                        for (int dx = -radius; dx <= radius; ++dx)
                                        {
                                            if (radius && std::abs(dx) != radius && std::abs(dy) != radius)
                                            {
                                                continue;
                                            }
                                            Point pixel{ p.x + dx, p.y + dy };
                                            if (pixel.x < 0 || pixel.x >= 640 || pixel.y < 0 || pixel.y >= 480)
                                            {
                                                continue;
                                            }
                                            if (battle.pick_surface(float(pixel.x), float(pixel.y), camera.first, camera.second) == *reachable)
                                            {
                                                click = pixel;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (!click && revealInput)
                                {
                                    for (auto point : path)
                                    {
                                        if (!battle.preview(point))
                                        {
                                            continue;
                                        }
                                        auto center = prologue->screen_cell(point);
                                        prologue->pan(center.x - 320, center.y - 240);
                                        center = prologue->screen_cell(point);
                                        camera = prologue->camera();
                                        for (int dy = -64; dy <= 64 && !click; ++dy)
                                        {
                                            for (int dx = -64; dx <= 64; ++dx)
                                            {
                                                Point pixel{ center.x + dx, center.y + dy };
                                                if (pixel.x < 0 || pixel.x >= 640 || pixel.y < 0 || pixel.y >= 480)
                                                {
                                                    continue;
                                                }
                                                if (battle.pick_surface(float(pixel.x), float(pixel.y), camera.first, camera.second) == point)
                                                {
                                                    click = pixel;
                                                    reachable = point;
                                                    break;
                                                }
                                            }
                                        }
                                        if (click)
                                        {
                                            break;
                                        }
                                    }
                                }
                                if (!click)
                                {
                                    s2ne::fatal("Original exit destination has no verified visible click pixel");
                                }
                                prologue->mouse(float(click->x), float(click->y), true);
                                if (!battle.animating_movement() && Point{ actor->x, actor->y } != *reachable)
                                {
                                    s2ne::fatal("Verified exit click did not start movement");
                                }
                                exitMoveTurn = prologue->battle_turn();
                            }
                        }
                    }
                }
            }
            else
            {
                prologue->start_auto_battle();
            }
        }
        else if (detailSmoke && prologue && prologue->in_battle())
        {
            if (detailSmokePhase == 0)
            {
                auto unit = std::find_if(prologue->events.units.begin(), prologue->events.units.end(), [](const auto& candidate)
                    {
                        return candidate.controllable() && candidate.present();
                    });
                if (unit == prologue->events.units.end())
                {
                    s2ne::fatal("Missing detail smoke unit");
                }
                auto point = prologue->screen_cell({ unit->x, unit->y });
                prologue->pan(point.x - 320, point.y - 240);
                point = prologue->screen_cell({ unit->x, unit->y });
                prologue->mouse(float(point.x), float(point.y), true);
                detailSmokePhase = 1;
            }
            else if (detailSmokePhase == 1)
            {
                if (prologue->battle_state() != s2ne::Battle::State::command)
                {
                    s2ne::fatal("Detail smoke selection failed");
                }
                for (unsigned i = 0; i < 6 && prologue->battle_command_index() != 3; ++i)
                {
                    prologue->battle_cycle(1);
                }
                SDL_Event key{ };
                key.type = SDL_EVENT_KEY_DOWN;
                key.key.key = SDLK_RETURN;
                SDL_PushEvent(&key);
                detailSmokePhase = 2;
            }
            else if (detailSmokePhase == 2)
            {
                if (!battleReports || battleReports->page() != s2ne::OriginalBattleReports::Page::detail)
                {
                    s2ne::fatal("Detail command did not open original report");
                }
                SDL_Event key{ };
                key.type = SDL_EVENT_KEY_DOWN;
                key.key.key = SDLK_ESCAPE;
                SDL_PushEvent(&key);
                detailSmokePhase = 3;
            }
            else
            {
                if (battleReports || prologue->battle_state() != s2ne::Battle::State::command)
                {
                    s2ne::fatal("Detail report did not return to selected command");
                }
                std::cout << "SDL selected command3 -> original detail modal -> selected command return verified\n";
                running = false;
            }
        }
        else if (autoSmoke && prologue && prologue->in_battle())
        {
            if (!battleSmokeStartTurn)
            {
                battleSmokeStartTurn = prologue->battle_turn();
                auto pushRight = [&]
                {
                    SDL_Event click{ };
                    click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    click.button.button = SDL_BUTTON_RIGHT;
                    SDL_RenderCoordinatesToWindow(renderer, 320, 240, &click.button.x, &click.button.y);
                    SDL_PushEvent(&click);
                };
                pushRight();
            }
            else if (!autoMenuOpened && !battleMenu)
            {
                SDL_Event click{ };
                click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                click.button.windowID = SDL_GetWindowID(window);
                click.button.button = SDL_BUTTON_RIGHT;
                SDL_RenderCoordinatesToWindow(renderer, 320, 240, &click.button.x, &click.button.y);
                SDL_PushEvent(&click);
            }
            else if (battleMenu)
            {
                autoMenuOpened = true;
                SDL_Event key{ };
                key.type = SDL_EVENT_KEY_DOWN;
                key.key.key = SDLK_DOWN;
                if (battleMenu->page() == s2ne::OriginalBattleMenu::Page::root && battleMenu->selected() < 3)
                {
                    SDL_PushEvent(&key);
                }
                else
                {
                    key.key.key = SDLK_RETURN;
                    SDL_PushEvent(&key);
                }
            }
            else if (prologue->battle_turn() > battleSmokeStartTurn)
            {
                if (!autoMenuOpened || !autoFriendlyFrames || !autoEnemyFrames)
                {
                    s2ne::fatal("Automatic menu completed without both friendly and hostile action frames");
                }
                if (feedbackSmoke && (!feedbackFlash || !feedbackNumbers || !feedbackHp || !feedbackReaction))
                {
                    if (++feedbackRounds >= 15)
                    {
                        s2ne::fatal("No complete rendered flash/number/HP feedback within 15 original turns");
                    }
                    battleSmokeStartTurn = 0;
                    autoMenuOpened = false;
                }
                else
                {
                    std::cout << "SDL battlefield automatic combat menu -> friendly actions -> hostile actions -> player control verified\n";
                    if (feedbackSmoke)
                    {
                        std::cout << "Rendered original indexed reaction, hit silhouette, rising damage numbers and stepped HP reduction verified\n";
                    }
                    running = false;
                }
            }
        }
        else if (prologue && prologue->in_battle())
        {
            if (prologue->events.stage != (alternateSmoke ? 71u : 1u))
            {
                s2ne::fatal("Wrong campaign battle stage");
            }
            auto pushClick = [&](s2ne::Battle::Point point)
            {
                SDL_Event click{ };
                click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                click.button.windowID = SDL_GetWindowID(window);
                click.button.button = SDL_BUTTON_LEFT;
                SDL_RenderCoordinatesToWindow(renderer, float(point.x), float(point.y), &click.button.x, &click.button.y);
                SDL_PushEvent(&click);
            };
            if (battleClickPending)
            {
                if (prologue->battle_state() == s2ne::Battle::State::command && !battleSmokeStartTurn)
                {
                    auto origin = prologue->battle_command_origin();
                    pushClick({ origin.x + 48, origin.y + 16 });
                    battleSmokeStartTurn = prologue->battle_turn();
                }
                else if (prologue->battle_state() == s2ne::Battle::State::movement)
                {
                    pushClick(prologue->screen_cell(battleSmokeDestination));
                }
                else if (prologue->battle_state() == s2ne::Battle::State::command && prologue->battle_turn() == battleSmokeStartTurn)
                {
                    const auto& unit = prologue->events.units.at(battleSmokeUnit);
                    if (unit.x != battleSmokeDestination.x || unit.y != battleSmokeDestination.y)
                    {
                        s2ne::fatal("SDL routed movement failed");
                    }
                    SDL_Event finish{ };
                    finish.type = SDL_EVENT_KEY_DOWN;
                    finish.key.key = SDLK_F10;
                    SDL_PushEvent(&finish);
                }
                else if (prologue->battle_state() == s2ne::Battle::State::idle || (prologue->battle_state() == s2ne::Battle::State::command && prologue->battle_turn() > battleSmokeStartTurn))
                {
                    if (prologue->battle_turn() != battleSmokeStartTurn + 1 || hostileBannerFrames < 8 || playerBannerFrames < 8)
                    {
                        s2ne::fatal("SDL side banners missing or side finish failed");
                    }
                    if (++battleSmokeRounds == 2)
                    {
                        bool enemy_moved = false;
                        for (size_t i = 0; i < prologue->events.units.size(); ++i)
                        {
                            const auto& unit = prologue->events.units[i];
                            if (unit.hostile() && std::pair{ unit.x, unit.y } != battleSmokeEnemyOrigins.at(i))
                            {
                                enemy_moved = true;
                            }
                        }
                        if (!enemy_moved || hostileActionFrames < 2)
                        {
                            std::cerr << "Battle smoke movement diagnostic: turn=" << prologue->battle_turn() << " action_frames=" << hostileActionFrames << '\n';
                            for (size_t i = 0; i < prologue->events.units.size(); ++i)
                            {
                                const auto& u = prologue->events.units[i];
                                if (u.hostile())
                                {
                                    std::cerr << " enemy[" << i << "] behavior=" << u.behavior << " hp=" << u.hp << " origin=" << battleSmokeEnemyOrigins.at(i).first << ',' << battleSmokeEnemyOrigins.at(i).second << " now=" << u.x << ',' << u.y << " action=" << unsigned(u.action_state) << '\n';
                                }
                            }
                            s2ne::fatal("Enemy turn did not visibly execute movement");
                        }
                        std::cout << "SDL campaign battle " << prologue->events.stage << ": player movement, visible enemy movement and two complete side-banner cycles verified (" << hostileActionFrames << " hostile frames)\n";
                        running = false;
                    }
                    else
                    {
                        battleSmokeStartTurn = prologue->battle_turn();
                        hostileBannerFrames = playerBannerFrames = 0;
                        SDL_Event finish{ };
                        finish.type = SDL_EVENT_KEY_DOWN;
                        finish.key.key = SDLK_F10;
                        SDL_PushEvent(&finish);
                    }
                }
                else
                {
                    s2ne::fatal("Unexpected SDL battle state");
                }
            }
            else
            {
                battleSmokeEnemyOrigins.clear();
                for (const auto& unit : prologue->events.units)
                {
                    battleSmokeEnemyOrigins.emplace_back(unit.x, unit.y);
                }
                s2ne::OriginalBattleMenu originalMenu(root, { }, settingsState);
                originalMenu.activate();
                if (originalMenu.request().action != s2ne::OriginalBattleMenu::Action::conditions)
                {
                    s2ne::fatal("Battlefield conditions menu request missing");
                }
                originalMenu.complete_operation();
                originalMenu.move(1);
                originalMenu.activate();
                if (originalMenu.page() != s2ne::OriginalBattleMenu::Page::overview)
                {
                    s2ne::fatal("Battlefield overview submenu missing");
                }
                originalMenu.cancel();
                originalMenu.cancel();
                if (!originalMenu.return_code() || *originalMenu.return_code() != 0)
                {
                    s2ne::fatal("Battlefield menu return path failed");
                }
                if (settingsState.status_window == 1)
                {
                    auto first = std::find_if(prologue->events.units.begin(), prologue->events.units.end(), [](const auto& unit)
                        {
                            return unit.controllable() && (unit.status_flags & 0x80) && !(unit.status_flags & 0x20);
                        });
                    if (first == prologue->events.units.end() || prologue->battle_selected() != int(first - prologue->events.units.begin()) || prologue->battle_state() != s2ne::Battle::State::command)
                    {
                        s2ne::fatal("Original opening ordered selection missing");
                    }
                }
                prologue->cancel_battle_selection();
                prologue->pan(320, 160);
                auto before = prologue->screen_cell({ 0, 0 });
                prologue->edge_scroll(0, 0);
                prologue->edge_scroll(0, 0);
                auto after = prologue->screen_cell({ 0, 0 });
                if (after.x - before.x != 64 || after.y - before.y != 64)
                {
                    s2ne::fatal("Stationary edge cursor failed to keep scrolling");
                }
                prologue->edge_scroll(639, 479);
                prologue->edge_scroll(639, 479);
                auto restored = prologue->screen_cell({ 0, 0 });
                if (restored.x != before.x || restored.y != before.y)
                {
                    s2ne::fatal("Reverse edge cursor failed to restore camera");
                }
                auto terrain = s2ne::Battle::load(root, prologue->events.map_id);
                bool found = false;
                for (size_t i = 0; i < prologue->events.units.size() && !found; ++i)
                {
                    const auto& u = prologue->events.units[i];
                    for (int dy : { -1, 1 })
                    {
                        for (int dx : { 0, (u.y & 1) ? 1 : -1 })
                        {
                            s2ne::Battle::Point to{ u.x + dx, u.y + dy };
                            auto fromScreen = prologue->screen_cell({ u.x, u.y }), toScreen = prologue->screen_cell(to);
                            if (found || !terrain.can_step(prologue->events.units, i, to) || fromScreen.x < 0 || fromScreen.x >= 640 || fromScreen.y < 0 || fromScreen.y >= 480 || toScreen.x < 0 || toScreen.x >= 640 || toScreen.y < 0 || toScreen.y >= 480)
                            {
                                continue;
                            }
                            battleSmokeUnit = i;
                            battleSmokeDestination = to;
                            pushClick(fromScreen);
                            found = battleClickPending = true;
                        }
                    }
                }
                if (!found)
                {
                    for (size_t i = 0; i < prologue->events.units.size() && !found; ++i)
                    {
                        const auto& u = prologue->events.units[i];
                        for (int dy : { -1, 1 })
                        {
                            for (int dx : { 0, (u.y & 1) ? 1 : -1 })
                            {
                                s2ne::Battle::Point to{ u.x + dx, u.y + dy };
                                if (found || !terrain.can_step(prologue->events.units, i, to))
                                {
                                    continue;
                                }
                                auto position = prologue->screen_cell({ u.x, u.y });
                                prologue->pan(position.x - 320, position.y - 240);
                                battleSmokeUnit = i;
                                battleSmokeDestination = to;
                                pushClick(prologue->screen_cell({ u.x, u.y }));
                                found = battleClickPending = true;
                            }
                        }
                    }
                    if (!found)
                    {
                        s2ne::fatal("SDL battle smoke has no movable unit");
                    }
                }
            }
        }
        else if (!prologue || (!prologue->computer_actions() && !prologue->battle_intro()))
        {
            SDL_PushEvent(&e);
        }
        if (!prologue || !prologue->computer_actions())
        {
            if (++smokePhase > 1000 && !clearSmoke && (!prologue || !prologue->in_battle()))
            {
                s2ne::fatal("SDL battle entry timed out");
            }
        }
        if (SDL_GetTicks() - (campaignContinue && campaignStageStarted ? campaignStageStarted : started) > (clearSmoke ? 600000u : 120000u))
        {
            s2ne::fatal("SDL battle action cycle timed out: stage=" + std::to_string(prologue ? prologue->events.stage : 0) + " blocker=" + (prologue ? prologue->campaign_blocker() : "") + " dialogue=" + std::to_string(prologue && prologue->campaign_dialogue()) + " choosing=" + std::to_string(prologue && prologue->choosing()) + " storyRoute=" + std::to_string(prologue ? prologue->pending_story_route() : 0) + " turn=" + std::to_string(prologue ? prologue->battle_turn() : 0) + " start=" + std::to_string(battleSmokeStartTurn) + " feedbackRounds=" + std::to_string(feedbackRounds) + " flash=" + std::to_string(feedbackFlash) + " numbers=" + std::to_string(feedbackNumbers) + " hp=" + std::to_string(feedbackHp));
        }
    }
    if (newGameSmoke && SDL_GetTicks() - started > 100 + smokePhase * 150 && (!prologue || prologue->line_index() == 0))
    {
        SDL_Event e{ };
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = prologue || inSetup ? SDLK_RETURN : inMenu ? (menu.selected ? SDLK_UP : SDLK_RETURN) :
                                                                 SDLK_ESCAPE;
        SDL_PushEvent(&e);
        ++smokePhase;
    }
    if (newGameSmoke && prologue && prologue->line_index() == 1)
    {
        if (!prologue || prologue->line_index() != 1 || inSetup || inMenu)
        {
            s2ne::fatal("New-game dialogue smoke test failed");
        }
        std::cout << "SDL new game -> original stage 140 -> second dialogue line verified\n";
        running = false;
    }
    if (loadSmoke && SDL_GetTicks() - started > 100 + smokePhase * 150 && !prologue)
    {
        SDL_Event e{ };
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = saveMenu ? SDLK_RETURN : inMenu ? (menu.selected == 1 ? SDLK_RETURN : SDLK_DOWN) :
                                                      SDLK_ESCAPE;
        SDL_PushEvent(&e);
        ++smokePhase;
    }
    if (loadSmoke && prologue && presented > 10)
    {
        if (!prologue->in_battle() || !rawSave || !isolatedSaveRoot)
        {
            s2ne::fatal("Title save load did not restore battle");
        }
        auto captured = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
        s2ne::OriginalSaveStore::write(*isolatedSaveRoot, 52, captured, saveItems);
        auto reloaded = s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot, 52), saveItems);
        if (s2ne::OriginalSave::encode(captured, saveItems) != s2ne::OriginalSave::encode(reloaded, saveItems))
        {
            s2ne::fatal("Isolated original save changed on reload");
        }
        std::cout << "Original save restored through title menu, stage " << prologue->events.stage << ", turn " << prologue->battle_turn() << "; isolated slot52 write/reload verified\n";
        running = false;
    }
    if (loadSmoke && SDL_GetTicks() - started > 15000)
    {
        s2ne::fatal("Original save smoke timed out: phase=" + std::to_string(smokePhase) + " menu=" + std::to_string(inMenu) + " row=" + std::to_string(menu.selected) + " save=" + std::to_string(bool(saveMenu)));
    }
    if (newGameSmoke && SDL_GetTicks() - started > 15000)
    {
        s2ne::fatal("New-game dialogue smoke timed out");
    }
    return false;
}
bool AppSmokeDriver::after_events(AppSmokeContext context, uint64_t nowUi, std::optional<OriginalPromotion>& promotionMenu, OriginalPromotion& promotionRules, bool skillCastVerified, const std::optional<size_t>& pendingSkillTarget, bool& prologueRedraw)
{
    const auto& skill88Input = options.skill88Input;
    const auto& skill94Input = options.skill94Input;
    const auto& skillInputSmoke = options.skillInputSmoke;
    const auto& skill88Positive = options.skill88Positive;
    const auto& inputSkill = options.inputSkill;
    const auto& inputFolder = options.inputFolder;
    const auto& promotionSmoke = options.promotionSmoke;
    auto& contactStep = state.contactStep;
    auto& skillInputPhase = state.skillInputPhase;
    auto& skillInputCaster = state.skillInputCaster;
    auto& skillInputTarget = state.skillInputTarget;
    auto& skillInputFrames = state.skillInputFrames;
    auto& skillInputNext = state.skillInputNext;
    auto& skillInputVictim = state.skillInputVictim;
    auto& skillInputCommitted = state.skillInputCommitted;
    auto& skillInputBefore = state.skillInputBefore;
    auto& skillInputVerified = state.skillInputVerified;
    auto& skillInputMpGrowth = state.skillInputMpGrowth;
    auto& skillCancelBaseline = state.skillCancelBaseline;
    auto& promotionSmokePhase = state.promotionSmokePhase;
    auto& promotionSmokeKind = state.promotionSmokeKind;
    auto& promotionSmokeTarget = state.promotionSmokeTarget;
    auto& promotionVerified = state.promotionVerified;
    auto& root = context.root;
    auto& prologue = context.prologue;
    auto& rawSave = context.rawSave;
    auto& isolatedSaveRoot = context.isolatedSaveRoot;
    auto& saveItems = context.saveItems;
    auto& window = context.window;
    auto& renderer = context.renderer;
    auto& running = context.running;
    auto& started = context.started;
    auto& skillMenu = context.skillMenu;
    auto& skillTarget = context.skillTarget;
    auto& skillRules = context.skillRules;
    if (skillInputSmoke && prologue && nowUi >= skillInputNext)
    {
        skillInputNext = nowUi + 80;
        if (nowUi - started > (skill88Positive ? 180000u : 60000u))
        {
            s2ne::fatal("Skill input timeout phase=" + std::to_string(skillInputPhase) + " state=" + std::to_string(int(prologue->battle_state())) + " turn=" + std::to_string(prologue->battle_turn()) + " status=" + prologue->battle_status() + " camera=" + std::to_string(prologue->camera().first) + "," + std::to_string(prologue->camera().second));
        }
        auto key = [&](SDL_Keycode code)
        {
            SDL_Event e{ };
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = code;
            SDL_PushEvent(&e);
        };
        auto snapshot = [&](const char* name)
        {
            auto state = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
            auto bytes = s2ne::OriginalSave::encode(state, saveItems);
            auto path = root / "S2NE" / "out" / inputFolder / name;
            std::ofstream f(path, std::ios::binary);
            f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            f.close();
            if (s2ne::OriginalSave::encode(s2ne::OriginalSave::load(path, saveItems), saveItems) != bytes)
            {
                s2ne::fatal("Skill input evidence save roundtrip failed");
            }
            return bytes;
        };
        if (skillInputPhase == 0)
        {
            bool found = false;
            for (unsigned i = 0; i < prologue->events.units.size() && !found; ++i)
            {
                const auto& u = prologue->events.units[i];
                s2ne::OriginalSkills::Context inputContext;
                inputContext.second = skill88Input || skill94Input;
                if (!u.controllable() || !(u.status_flags & 0x80) || (u.status_flags & 0x20) || !(u.learned[skill94Input ? 9 : skill88Input ? 8 :
                                                                                                                                              2]
                        & (skill94Input ? 1 : skill88Input ? 32 :
                                                             0x800))
                    || !skillRules.entry_available(u, inputContext))
                {
                    continue;
                }
                s2ne::OriginalSkills probe(root);
                probe.open(u, inputContext);
                for (unsigned n = 0; n < 150 && probe.selected().id != inputSkill; ++n)
                {
                    bool has = false;
                    for (auto& r : probe.groups()[probe.selected_group()].records)
                    {
                        has |= r.id == inputSkill;
                    }
                    probe.navigate(has ? 0x50 : 0x4D);
                }
                if (probe.selected().id != inputSkill || probe.select(u) != s2ne::OriginalSkills::Failure::none)
                {
                    continue;
                }
                if (skill88Input)
                {
                    skillInputCaster = i;
                    skillInputTarget = i;
                    found = true;
                    break;
                }
                for (unsigned t = 0; t < prologue->events.units.size(); ++t)
                {
                    if (probe.hostile_in_range(u, prologue->events.units[t]) && prologue->battle().target_visible(prologue->events.units[t], std::span(&u, 1), 0))
                    {
                        skillInputCaster = i;
                        skillInputTarget = t;
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                if (skill94Input)
                {
                    s2ne::fatal("No active learned94 caster with naturally supported in-range target; original reserve flags retained");
                }
                if (skill88Input)
                {
                    if (prologue->campaign_dialogue())
                    {
                        key(SDLK_RETURN);
                    }
                    else if (prologue->in_battle() && !prologue->battle().animating_movement())
                    {
                        key(SDLK_F10);
                    }
                }
                else
                {
                    s2ne::fatal("Learned save has no naturally deployed in-range skill32 pair; no units relocated");
                }
            }
            if (found)
            {
                const auto& u = prologue->events.units[skillInputCaster];
                auto point = prologue->screen_cell({ u.x, u.y });
                if (skill94Input)
                {
                    auto [cx, cy] = prologue->camera();
                    bool picked = false;
                    for (int dy = -32; dy <= 32 && !picked; ++dy)
                    {
                        for (int dx = -32; dx <= 32; ++dx)
                        {
                            if (prologue->battle().pick_surface(float(point.x + dx), float(point.y + dy), cx, cy) == s2ne::Battle::Point{ u.x, u.y } && prologue->battle().pick_surface(float(point.x + dx + 2), float(point.y + dy + 2), cx, cy) == s2ne::Battle::Point{ u.x, u.y } && prologue->battle().pick_surface(float(point.x + dx - 2), float(point.y + dy - 2), cx, cy) == s2ne::Battle::Point{ u.x, u.y })
                            {
                                point.x += dx;
                                point.y += dy;
                                picked = true;
                                break;
                            }
                        }
                    }
                }
                if (prologue->battle_state() != s2ne::Battle::State::idle) { key(SDLK_ESCAPE); }
                else if (point.x < 32 || point.x > 608 || point.y < 32 || point.y > 370) { key(point.x < 32 ? SDLK_LEFT : point.x > 608 ? SDLK_RIGHT :
                        point.y < 32                                                                                                    ? SDLK_UP :
                                                                                                                                          SDLK_DOWN); }
                else
                {
                    SDL_Event e{ };
                    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    e.button.windowID = SDL_GetWindowID(window);
                    e.button.button = SDL_BUTTON_LEFT;
                    SDL_RenderCoordinatesToWindow(renderer, float(point.x), float(point.y), &e.button.x, &e.button.y);
                    SDL_PushEvent(&e);
                    skillInputPhase = skill88Positive && u.hp == u.max_hp ? 20 : 1;
                    std::cout << "INPUT select caster=" << u.id << " target=" << prologue->events.units[skillInputTarget].id << " action=" << unsigned(u.action_state) << " click=" << point.x << ',' << point.y << " xy=" << u.x << ',' << u.y << '\n';
                }
            }
        }
        else if (skillInputPhase == 20)
        {
            if (prologue->battle().animating_movement()) { }
            else if (prologue->events.units[skillInputCaster].x == 16 && prologue->events.units[skillInputCaster].y == 41)
            {
                if (contactStep >= 5)
                {
                    snapshot("contact-blocked.sav");
                    s2ne::fatal("Bounded88 two additional adjacent AI turns produced no injury");
                }
                contactStep = std::max(3u, contactStep) + 1;
                key(SDLK_F10);
                skillInputPhase = 0;
            }
            else if (prologue->battle_state() == s2ne::Battle::State::command)
            {
                if (prologue->battle_command_index() != 1)
                {
                    key(SDLK_RIGHT);
                }
                else
                {
                    key(SDLK_RETURN);
                }
            }
            else if (prologue->battle_state() == s2ne::Battle::State::movement)
            {
                constexpr std::array<std::pair<int, int>, 3> waypoints{ { { 23, 30 }, { 20, 35 }, { 16, 41 } } };
                if (contactStep >= waypoints.size())
                {
                    snapshot("contact-blocked.sav");
                    s2ne::fatal("Bounded88 contact route exhausted without injury");
                }
                auto [x, y] = waypoints[contactStep];
                auto p = prologue->screen_cell({ x, y });
                SDL_Event e{ };
                e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                e.button.windowID = SDL_GetWindowID(window);
                e.button.button = SDL_BUTTON_LEFT;
                SDL_RenderCoordinatesToWindow(renderer, float(p.x), float(p.y), &e.button.x, &e.button.y);
                SDL_PushEvent(&e);
                skillInputPhase = 21;
                skillInputNext = nowUi + 1000;
                std::cout << "CONTACT input waypoint=" << x << "," << y << " turn=" << prologue->battle_turn() << std::endl;
            }
        }
        else if (skillInputPhase == 21 && !prologue->battle().animating_movement())
        {
            const auto& u = prologue->events.units[skillInputCaster];
            constexpr std::array<std::pair<int, int>, 3> waypoints{ { { 23, 30 }, { 20, 35 }, { 16, 41 } } };
            if (std::pair(u.x, u.y) != waypoints[contactStep])
            {
                snapshot("contact-blocked.sav");
                s2ne::fatal("Bounded88 original movement rejected waypoint; no relocation fallback");
            }
            ++contactStep;
            std::cout << "CONTACT moved actor=" << u.id << " xy=" << u.x << "," << u.y << " HP=" << u.hp << " ST=" << u.st << " AP=" << u.action_points << std::endl;
            key(SDLK_F10);
            skillInputPhase = 0;
        }
        else if (skillInputPhase == 1 || skillInputPhase == 6)
        {
            if (prologue->battle_state() == s2ne::Battle::State::command)
            {
                if (prologue->battle_command_index() != 0)
                {
                    key(SDLK_RIGHT);
                }
                else
                {
                    key(SDLK_RETURN);
                }
            }
            else if (prologue->battle_state() == s2ne::Battle::State::attack_menu)
            {
                if (skill94Input)
                {
                    std::cout << "94 submenu selected=" << prologue->events.units.at(size_t(prologue->battle_selected())).id << " index=" << prologue->battle().attack_index() << " secondAvailable=" << skillRules.executable_available(prologue->events.units.at(size_t(prologue->battle_selected())), s2ne::OriginalSkills::Context{ .second = true }, true) << '\n';
                }
                if (prologue->battle().attack_index() != int((skill88Input || skill94Input) ? 3 : 2))
                {
                    key(SDLK_RIGHT);
                }
                else
                {
                    key(SDLK_RETURN);
                    ++skillInputPhase;
                }
            }
        }
        else if (skillInputPhase == 2 || skillInputPhase == 7)
        {
            if (skillMenu)
            {
                if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                {
                    if (skillMenu->selected().id != inputSkill)
                    {
                        bool has = false;
                        for (auto& r : skillMenu->groups()[skillMenu->selected_group()].records)
                        {
                            has |= r.id == inputSkill;
                        }
                        key(has ? SDLK_DOWN : SDLK_RIGHT);
                    }
                    else
                    {
                        key(SDLK_RETURN);
                    }
                }
                else if (skillTarget && *skillTarget == skillInputTarget) { ++skillInputPhase; }
                else
                {
                    key(SDLK_TAB);
                }
            }
        }
        else if (skillInputPhase == 3)
        {
            skillCancelBaseline = snapshot("cancel-before.sav");
            key(SDLK_ESCAPE);
            ++skillInputPhase;
        }
        else if (skillInputPhase == 4 && !skillMenu)
        {
            if (snapshot("cancel-after.sav") != skillCancelBaseline)
            {
                s2ne::fatal("Target cancel mutated original save");
            }
            std::cout << "PASS normal target cancel no payment or RNG change\n";
            skillInputPhase = 6;
        }
        else if (skillInputPhase == 8)
        {
            skillInputBefore = prologue->events.units[skillInputCaster];
            skillInputVictim = prologue->events.units[skillInputTarget];
            snapshot("cast-before.sav");
            key(SDLK_RETURN);
            ++skillInputPhase;
        }
        else if (skillInputPhase == 9 && !skillMenu && !pendingSkillTarget)
        {
            if (prologue->battle().combat_feedback_active())
            {
                return true;
            }
            if (!skillCastVerified)
            {
                s2ne::fatal("Normal skill cast did not commit");
            }
            const auto& u = prologue->events.units[skillInputCaster];
            if (((skill88Input || skill94Input) ? u.st != skillInputBefore->st + (u.max_st - skillInputBefore->max_st) - (skill94Input ? 5 : 25) : u.mp != skillInputBefore->mp + skillInputMpGrowth - 40) || u.action_points >= skillInputBefore->action_points || skillInputFrames != (skill94Input ? 15 : skill88Input ? 35 :
                                                                                                                                                                                                                                                                                                                            23))
            {
                s2ne::fatal("Normal skill32 payment/frame count mismatch mp=" + std::to_string(u.mp) + " before=" + std::to_string(skillInputBefore->mp) + " ap=" + std::to_string(u.action_points) + " beforeAP=" + std::to_string(skillInputBefore->action_points) + " frames=" + std::to_string(skillInputFrames));
            }
            if (skill88Positive && (u.hp <= skillInputBefore->hp || u.experience <= skillInputBefore->experience))
            {
                s2ne::fatal("88 positive heal/XP not obtained; no retry or RNG filtering");
            }
            skillInputCommitted = snapshot("cast-after.sav");
            std::cout << "PASS INPUT complete" << inputSkill << " ST=" << skillInputBefore->st << "->" << u.st << " HP=" << skillInputBefore->hp << "->" << u.hp << " MP=" << skillInputBefore->mp << "->" << u.mp << " nativeMPGrowth=" << skillInputMpGrowth << " paidMP=" << (skillInputBefore->mp + skillInputMpGrowth - u.mp) << " AP=" << skillInputBefore->action_points << "->" << u.action_points << " proficiency=" << skillInputBefore->learned_proficiency[skill94Input ? 9 : skill88Input ? 8 :
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         2]
                      << "->" << u.learned_proficiency[skill94Input ? 9 : skill88Input ? 8 :
                                                                                         2]
                      << " XP=" << skillInputBefore->experience << "->" << u.experience << " level=" << skillInputBefore->level << "->" << u.level << " SDL frames=" << skillInputFrames << '\n';
            skillInputPhase = 10;
            skillInputNext = nowUi + 500;
        }
        else if (skillInputPhase == 10)
        {
            if (snapshot("settled-after.sav") != skillInputCommitted)
            {
                s2ne::fatal("Post-cast idle input loop repeated payment or mutation");
            }
            std::cout << "PASS restored idle loop500ms no duplicate MP/AP/proficiency/XP payment; Windows SDL driver=" << SDL_GetCurrentVideoDriver() << '\n';
            skillInputVerified = true;
            running = false;
        }
        prologueRedraw = true;
    }
    if (promotionSmoke && prologue)
    {
        if (prologue->campaign_defeated() || prologue->campaign_reward_report())
        {
            s2ne::fatal("Earned save reached campaign outcome before promotion: defeat=" + std::to_string(prologue->campaign_defeated()) + " victory=" + std::to_string(prologue->campaign_reward_report()));
        }
        if (SDL_GetTicks() - started > 30000)
        {
            s2ne::fatal("Earned promotion SDL flow timed out: phase=" + std::to_string(promotionSmokePhase) + " selected=" + std::to_string(prologue->battle_selected()) + " state=" + std::to_string(int(prologue->battle_state())) + " blocker=" + prologue->campaign_blocker() + " dialogue=" + std::to_string(prologue->campaign_dialogue()) + " modal=" + std::to_string(prologue->campaign_modal()) + " defeat=" + std::to_string(prologue->campaign_defeated()) + " reward=" + std::to_string(prologue->campaign_reward_report()));
        }
        auto pushPromotionClick = [&](float x, float y)
        {
            SDL_Event click{ };
            click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            click.button.windowID = SDL_GetWindowID(window);
            click.button.button = SDL_BUTTON_LEFT;
            SDL_RenderCoordinatesToWindow(renderer, x, y, &click.button.x, &click.button.y);
            SDL_PushEvent(&click);
        };
        if (promotionSmokePhase == 0 && prologue->campaign_dialogue())
        {
            prologue->reveal_text();
            prologue->advance();
        }
        else if (promotionSmokePhase == 0 && prologue->in_battle())
        {
            auto found = std::find_if(prologue->events.units.begin(), prologue->events.units.end(), [&](auto u)
                {
                    return u.controllable() && u.present(prologue->battle_turn()) && promotionRules.eligible(u, u.experience_threshold) && !promotionRules.candidates(u).empty();
                });
            if (found == prologue->events.units.end())
            {
                s2ne::fatal("Earned save contains no eligible deployed promotion candidate");
            }
            promotionSmokeKind = found->appearance;
            auto point = prologue->screen_cell({ found->x, found->y });
            prologue->pan(point.x - 320, point.y - 240);
            point = prologue->screen_cell({ found->x, found->y });
            prologue->mouse(float(point.x), float(point.y), true);
            if (prologue->battle_selected() < 0)
            {
                s2ne::fatal("Earned candidate could not be selected at original projected cell");
            }
            ++promotionSmokePhase;
        }
        else if (promotionSmokePhase == 1 && prologue->battle_state() == s2ne::Battle::State::command)
        {
            if (!prologue->battle().command_available_for_cycle(4))
            {
                s2ne::fatal("Earned promotion command disabled");
            }
            if (prologue->battle().command_index() != 4)
            {
                SDL_Event key{ };
                key.type = SDL_EVENT_KEY_DOWN;
                key.key.key = SDLK_RIGHT;
                SDL_PushEvent(&key);
            }
            else
            {
                SDL_Event key{ };
                key.type = SDL_EVENT_KEY_DOWN;
                key.key.key = SDLK_RETURN;
                SDL_PushEvent(&key);
                ++promotionSmokePhase;
            }
        }
        else if (promotionSmokePhase == 2 && promotionMenu)
        {
            promotionSmokeTarget = promotionMenu->selected().kind;
            SDL_Event key{ };
            key.type = SDL_EVENT_KEY_DOWN;
            key.key.key = SDLK_RETURN;
            SDL_PushEvent(&key);
            ++promotionSmokePhase;
        }
        else if (promotionSmokePhase == 3 && promotionMenu && promotionMenu->page() == s2ne::OriginalPromotion::Page::confirmation)
        {
            pushPromotionClick(100, 430);
            ++promotionSmokePhase;
        }
        else if (promotionSmokePhase == 4 && !promotionMenu)
        {
            int selected = prologue->battle_selected();
            if (selected < 0 || prologue->events.units.at(size_t(selected)).appearance != promotionSmokeTarget || promotionSmokeKind == promotionSmokeTarget)
            {
                s2ne::fatal("SDL promotion did not commit the selected class");
            }
            auto captured = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
            s2ne::OriginalSaveStore::write(*isolatedSaveRoot, 1, captured, saveItems);
            auto loaded = s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot, 1), saveItems);
            if (s2ne::OriginalSave::encode(captured, saveItems) != s2ne::OriginalSave::encode(loaded, saveItems))
            {
                s2ne::fatal("Promoted original save did not roundtrip");
            }
            auto restored = s2ne::OriginalSaveBridge::restore(root, loaded);
            if (restored.events.units.at(size_t(selected)).appearance != promotionSmokeTarget || restored.events.units.at(size_t(selected)).level != prologue->events.units.at(size_t(selected)).level)
            {
                s2ne::fatal("Promoted original class did not survive runtime restore");
            }
            std::cout << "SDL earned save -> command4 -> original promotion selection/confirmation -> class " << promotionSmokeKind << " to " << promotionSmokeTarget << " -> save reload verified\n";
            promotionVerified = true;
            running = false;
        }
        prologueRedraw = true;
    }
    return false;
}
}    //namespace s2ne
