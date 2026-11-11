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
            std::optional<s2ne::smoke_exit::RevealInput> padInput;
            if (mineralInput) { padInput = mineralInput; }
            else if (campaignContinue && strengthenedFlow) { padInput = s2ne::smoke_exit::campaign_pad_input(prologue->battle(), prologue->events); }
            else
            {
                padInput = std::nullopt;
            }

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
                                    for (unsigned i = 0; i < 8 && battle.attack_index() != int((skill88Input || skill94Input || inputSkill == 96 || inputSkill == 86 || inputSkill == 92 || (NativeAbilityBuffs::find(inputSkill) && NativeAbilityBuffs::find(inputSkill)->group == 8)) ? 3 : 2); ++i)
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
        if (prologue || inSetup) { e.key.key = SDLK_RETURN; }
        else
        {
            if (inMenu)
            {
                if (menu.selected) { e.key.key = SDLK_UP; }
                else
                {
                    e.key.key = SDLK_RETURN;
                }
            }
            else
            {
                e.key.key = SDLK_ESCAPE;
            }
        }
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
        if (saveMenu) { e.key.key = SDLK_RETURN; }
        else
        {
            if (inMenu)
            {
                if (menu.selected == 1) { e.key.key = SDLK_RETURN; }
                else
                {
                    e.key.key = SDLK_DOWN;
                }
            }
            else
            {
                e.key.key = SDLK_ESCAPE;
            }
        }
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
        if (nowUi - started > ((skill88Positive || !options.skillControl.empty()) ? 240000u : 60000u))
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
            auto events = prologue->events;
            if (skill94Input && skillInputPhase == 9 && !skillCastVerified && skillInputBefore)
            {
                events.units[skillInputCaster].action_state = skillInputBefore->action_state;
                events.units[skillInputCaster].direction = skillInputBefore->direction;
                events.units[skillInputCaster].animation_action = skillInputBefore->animation_action;
            }
            auto captured = s2ne::OriginalSaveBridge::capture(events, prologue->battle_turn(), prologue->camera(), &*rawSave);
            auto bytes = s2ne::OriginalSave::encode(captured, saveItems);
            auto path = root / "S2NE" / "out" / inputFolder / name;
            std::ofstream f(path, std::ios::binary);
            f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            f.close();
            auto decoded = s2ne::OriginalSave::load(path, saveItems);
            if (s2ne::OriginalSave::encode(decoded, saveItems) != bytes)
            {
                s2ne::fatal("Skill input evidence save roundtrip failed");
            }
            if ((NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61) && (std::string_view(name) == "cast-after.sav" || std::string_view(name) == "settled-after.sav"))
            {
                const auto restored = s2ne::OriginalSaveBridge::restore(root, decoded);
                for (auto index : state.controlledAreaTargets)
                {
                    const auto& live = events.units[index];
                    const auto& loaded = restored.events.units[index];
                    if (loaded.attack != live.attack || loaded.defense != live.defense || loaded.max_action_points != live.max_action_points || loaded.action_points != live.action_points || loaded.carry_points != live.carry_points || !std::equal(live.action_status_runtime->begin() + 0x38, live.action_status_runtime->begin() + 0xA0, loaded.action_status_runtime->begin() + 0x38)) { fatal("Stat production capture/decode/restore mismatch"); }
                }
                std::cout << "PASS production stat save/decode/restore all targets and timers\n";
            }
            if (skill94Input && (std::string_view(name) == "cast-after.sav" || std::string_view(name) == "settled-after.sav"))
            {
                auto restored = s2ne::OriginalSaveBridge::restore(root, decoded);
                for (auto index : { skillInputCaster, skillInputTarget })
                {
                    const auto& live = events.units[index];
                    const auto& loaded = restored.events.units[index];
                    if (loaded.hp != live.hp || loaded.st != live.st || loaded.equipment != live.equipment || loaded.ending_kills != live.ending_kills || loaded.ending_losses != live.ending_losses || loaded.action_points != live.action_points || loaded.status_flags != live.status_flags) { fatal("094 persisted death/revival/payment restore mismatch"); }
                }
            }
            return bytes;
        };
        if ((NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96) && prologue->campaign_modal())
        {
            key(SDLK_RETURN);
            return true;
        }
        if (skillInputPhase == 0)
        {
            bool found = false;
            for (unsigned i = 0; i < prologue->events.units.size() && !found; ++i)
            {
                const auto& u = prologue->events.units[i];
                if ((NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96) && !options.skillControl.empty() && u.id != 1) { continue; }
                s2ne::OriginalSkills::Context inputContext;
                inputContext.second = (NativeAbilityBuffs::find(inputSkill) && NativeAbilityBuffs::find(inputSkill)->group == 8) || (NativeTimedBuffs::find(inputSkill) && NativeTimedBuffs::find(inputSkill)->group == 8) || skill88Input || skill94Input || inputSkill == 96;
                unsigned int group;
                if (NativeAbilityBuffs::find(inputSkill)) { group = NativeAbilityBuffs::find(inputSkill)->group; }
                else if (NativeTimedBuffs::find(inputSkill)) { group = NativeTimedBuffs::find(inputSkill)->group; }
                else if (inputSkill == 8) { group = 0; }
                else if (inputSkill == 12) { group = 1; }
                else if (inputSkill == 35 || inputSkill == 39) { group = 3; }
                else if (inputSkill == 48) { group = 4; }
                else if (inputSkill == 56 || inputSkill == 60) { group = 5; }
                else if (inputSkill == 17) { group = 1; }
                else if (inputSkill == 25) { group = 2; }
                else if (inputSkill == 52) { group = 4; }
                else if (inputSkill == 61) { group = 5; }
                else if (inputSkill == 96) { group = 9; }
                else if (inputSkill == 34 || inputSkill == 38) { group = 3; }
                else if (inputSkill == 54) { group = 5; }
                else if (inputSkill == 13 || inputSkill == 15) { group = 1; }
                else if (inputSkill == 53) { group = 5; }
                else if ((inputSkill >= 1 && inputSkill <= 10)) { group = 0; }
                else if (inputSkill == 33) { group = 3; }
                else if (skill94Input) { group = 9; }
                else if (skill88Input) { group = 8; }
                else
                {
                    group = 2;
                }

                unsigned bit;

                if (NativeAbilityBuffs::find(inputSkill)) { bit = (1u << NativeAbilityBuffs::find(inputSkill)->bit); }
                else if (NativeTimedBuffs::find(inputSkill)) { bit = (1u << NativeTimedBuffs::find(inputSkill)->bit); }
                else if (inputSkill == 8 || inputSkill == 60) { bit = 128; }
                else if (inputSkill == 12) { bit = 2; }
                else if (inputSkill == 35) { bit = 4; }
                else if (inputSkill == 39) { bit = 64; }
                else if (inputSkill == 48) { bit = 32; }
                else if (inputSkill == 56) { bit = 8; }
                else if (inputSkill == 17) { bit = 64; }
                else if (inputSkill == 25) { bit = 16; }
                else if (inputSkill == 52) { bit = 512; }
                else if (inputSkill == 61) { bit = 256; }
                else if (inputSkill == 96) { bit = 4; }
                else if (inputSkill == 34 || inputSkill == 54) { bit = 2; }
                else if (inputSkill == 38) { bit = 32; }
                else if (inputSkill == 13) { bit = 4; }
                else if (inputSkill == 15) { bit = 16; }
                else if (inputSkill == 53) { bit = 1; }
                else if (inputSkill == 6 || inputSkill == 9 || inputSkill == 10) { bit = 1u << (inputSkill - 1); }
                else if (inputSkill == 4) { bit = 8; }
                else if (inputSkill == 5) { bit = 16; }
                else if (inputSkill == 3) { bit = 4; }
                else if (inputSkill == 2) { bit = 2; }
                else if (inputSkill == 1 || inputSkill == 33 || skill94Input) { bit = 1; }
                else if (skill88Input) { bit = 32; }
                else
                {
                    bit = 0x800;
                }

                if (!u.controllable() || !(u.status_flags & 0x80) || (u.status_flags & 0x20) || !(u.learned[group] & bit) || !skillRules.entry_available(u, inputContext)) { continue; }
                s2ne::OriginalSkills probe(root);
                probe.set_area_camera({ u.x * 64 - 320, u.y * 16 - 160 });
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
                    if ((inputSkill != 8 && inputSkill != 56 && inputSkill != 46 || i != t) && (inputSkill != 35 && inputSkill != 39 && inputSkill != 48 || prologue->events.units[t].hostile()) && (!skill94Input || (options.skillControl != "script82" && options.skillControl != "scriptfail82") || prologue->events.units[t].id == 147) && ((inputSkill == 1 && i != t && probe.healing1_in_range(u, prologue->events.units[t]) && prologue->events.units[t].hp < prologue->events.units[t].max_hp) || probe.hostile_in_range(u, prologue->events.units[t])) && prologue->battle().target_visible(prologue->events.units[t], std::span(&u, 1), 0))
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
                if (skill94Input && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& actor = prologue->events.units[skillInputCaster];
                    auto& victim = prologue->events.units[skillInputTarget];
                    actor.st = 1000;
                    actor.learned_proficiency[9] = 60000;
                    victim.learned_proficiency[9] = 0;
                    victim.fencing = victim.tactics = 0;
                    victim.hp = 1;
                    victim.max_hp = 5000;
                    victim.equipment = { };
                    victim.initial_status = { };
                    victim.status39 = 0;
                    victim.status3c = victim.status3d = victim.status3f = 0;
                    victim.action_status_runtime.reset();
                    if (options.skillControl == "reverse29") { victim.hp = 2500; }
                    int special;
                    if (options.skillControl == "revive25") { special = 25; }
                    else if (options.skillControl == "consume28") { special = 28; }
                    else if (options.skillControl == "reverse29") { special = 29; }
                    else
                    {
                        special = 0;
                    }

                    std::ifstream itemFile(root / "ITEMSP.BIN", std::ios::binary);
                    std::vector<uint8_t> items(std::istreambuf_iterator<char>(itemFile), { });
                    if (items.size() < 40) { fatal("Controlled094 original item table unavailable"); }
                    if (special)
                    {
                        for (unsigned i = 0; i < (items.size() - 2) / 38; ++i)
                        {
                            if (items[2 + i * 38 + 15] == 0 && items[2 + i * 38 + 25] == special)
                            {
                                victim.equipment[0] = i + 1;
                                break;
                            }
                        }
                        if (!victim.equipment[0]) { fatal("Controlled094 original special item missing"); }
                    }
                    else
                    {
                        victim.initial_status[0] = 1;
                    }
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.initialize(12);
                            rng.restore_stream(14, 0);
                            return true;
                        });
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED094 branch=" << options.skillControl << " HP/ST/proficiency/defense/equipment/status/RNG fixture; learning/coordinates/source SAVE unchanged; target=" << victim.id << '\n';
                }
                if (NativeAbilityBuffs::find(inputSkill) && !state.controlledAreaReady)
                {
                    auto& a = prologue->events.units[skillInputCaster];
                    a.abilities |= 0x20;
                    a.st = 200;
                    state.controlledAreaTargets = { skillInputTarget };
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.restore_stream(1, 0);
                            state.controlledSeed = rng.seed_state();
                            state.controlledCalls = rng.draw_count();
                            return false;
                        });
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED ability family learned/ST/halfMP/RNG fixture; source SAVE read-only; NOT natural\n";
                }
                if (const auto* buff = NativeTimedBuffs::find(inputSkill); buff && !state.controlledAreaReady)
                {
                    auto& a = prologue->events.units[skillInputCaster];
                    a.abilities |= 0x20;
                    a.st = 200;
                    state.controlledAreaTargets = { skillInputTarget };
                    auto& t = prologue->events.units[skillInputTarget];
                    auto& r = *t.action_status_runtime;
                    r[0x3C] &= uint8_t(~buff->mask);
                    r[0x3D] &= uint8_t(~(buff->mask >> 8));
                    t.status3c = r[0x3C];
                    t.status3d = r[0x3D];
                    if (inputSkill == 46)
                    {
                        r[0x3D] |= 1;
                        r[0x8D] = 255;
                        t.status3d = r[0x3D];
                    }
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.restore_stream(1, 0);
                            state.controlledSeed = rng.seed_state();
                            state.controlledCalls = rng.draw_count();
                            return false;
                        });
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED timed buff learned/ST/halfMP/status/RNG fixture; original positions/source SAVE read-only; NOT natural\n";
                }
                if ((inputSkill == 6 || inputSkill == 9 || inputSkill == 10) && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& units = prologue->events.units;
                    OriginalSkills probe(root);
                    probe.open(units[skillInputCaster], { });
                    while (probe.selected().id != inputSkill) { probe.navigate(0x50); }
                    const auto center = units[skillInputTarget];
                    for (size_t i = 0; i < units.size(); ++i)
                    {
                        auto& v = units[i];
                        if (!v.present(prologue->battle_turn()) || !probe.status_area_target(center, v) || !probe.sleep2_area(center, v)) { continue; }
                        if (inputSkill != 10)
                        {
                            v.hp = std::max(1, v.max_hp / 2);
                            v.action_status_runtime->at(0x3E) &= uint8_t(~1u);
                            v.status3e = v.action_status_runtime->at(0x3E);
                        }
                        else
                        {
                            v.action_status_runtime->at(0x3C) &= uint8_t(~0x40u);
                            v.action_status_runtime->at(0x8B) = 0;
                            v.status3c = v.action_status_runtime->at(0x3C);
                        }
                        state.controlledAreaTargets.push_back(i);
                    }
                    if (options.skillControl == "branches")
                    {
                        units[skillInputCaster].abilities |= 0x20;
                        if (inputSkill == 10)
                        {
                            auto& v = units[skillInputTarget];
                            v.status3c |= 0x40;
                            v.action_status_runtime->at(0x3C) |= 0x40;
                            v.action_status_runtime->at(0x8B) = 255;
                        }
                        else
                        {
                            if (state.controlledAreaTargets.size() < 4) { fatal("Family mixed full/blocked/healed needs four targets"); }
                            auto& full = units[state.controlledAreaTargets[0]];
                            full.hp = full.max_hp;
                            auto& blocked = units[state.controlledAreaTargets[1]];
                            blocked.status3e |= 1;
                            blocked.action_status_runtime->at(0x3E) |= 1;
                        }
                        std::cout << "CONTROLLED branches: halfMP ability; full/blocked plus positive targets, or010 permanent timer255; NOT natural branch\n";
                    }
                    if (inputSkill != 10 && state.controlledAreaTargets.size() < 2) { fatal("Restoration production requires multiple original allied targets"); }
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.initialize(12);
                            rng.seed(1);
                            return true;
                        });
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED support HP/status/RNG fixture; original learning/coordinates/source SAVE unchanged, NOT natural success targets=" << state.controlledAreaTargets.size() << '\n';
                }
                if ((inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96) && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& units = prologue->events.units;
                    auto& a = units[skillInputCaster];
                    a.st = 1000;
                    a.abilities |= 0x20;
                    auto center = units[skillInputTarget];
                    OriginalSkills probe(root);
                    probe.set_area_camera({ a.x * 64 - 320, a.y * 16 - 160 });
                    probe.open(a, OriginalSkills::Context{ .second = inputSkill == 96 });
                    for (unsigned n = 0; n < 150 && probe.selected().id != inputSkill; ++n)
                    {
                        bool has = false;
                        for (const auto& r : probe.groups()[probe.selected_group()].records) { has |= r.id == inputSkill; }
                        probe.navigate(has ? 0x50 : 0x4D);
                    }
                    state.controlledAreaTargets = { skillInputTarget };
                    {
                        std::vector<Battle::Point> cells;
                        for (int y = center.y - 2; y <= center.y + 2; ++y)
                        {
                            for (int x = center.x - 2; x <= center.x + 2; ++x)
                            {
                                auto c = center;
                                c.x = x;
                                c.y = y;
                                if (((inputSkill != 53 && inputSkill != 25 && inputSkill != 61 && inputSkill != 48) || probe.sleep2_area(inputSkill == 61 ? a : center, c)) && prologue->battle().skill_area_cell({ x, y }) && std::none_of(units.begin(), units.end(), [&](const auto& u)
                                        {
                                            return u.present(prologue->battle_turn()) && u.x == x && u.y == y;
                                        }))
                                {
                                    cells.push_back({ x, y });
                                }
                            }
                        }
                        for (size_t n = 0; n < units.size() && state.controlledAreaTargets.size() < 3; ++n)
                        {
                            if (n == skillInputTarget || n == skillInputCaster || (inputSkill == 48 && state.controlledAreaTargets.size() == 1 ? units[n].hostile() : !units[n].hostile()) || cells.empty()) { continue; }
                            auto pos = cells.back();
                            cells.pop_back();
                            auto& u = units[n];
                            u.x = pos.x;
                            u.y = pos.y;
                            u.hp = std::max(1, u.hp);
                            u.first_turn = 1;
                            u.last_turn = 0;
                            u.status_flags = uint8_t((u.status_flags | 0x80) & ~0x20);
                            state.controlledAreaTargets.push_back(n);
                        }
                        if (state.controlledAreaTargets.size() != 3) { fatal("053 mixed needs three enemy slots"); }
                    }
                    for (size_t n = 0; n < state.controlledAreaTargets.size(); ++n)
                    {
                        auto& u = units[state.controlledAreaTargets[n]];
                        u.first_turn = 1;
                        u.last_turn = 0;
                        u.status_flags = uint8_t((u.status_flags | 0x80) & ~0x20);
                        u.st = 1;
                        u.runtime_role_selector = n == 2 ? 1 : 0;
                        u.abilities = n == 1 ? 0x4000 : 0;
                        u.status3d = 0;
                        u.status3e = 2;
                        if (!u.action_status_runtime) { fatal("Hostile status fixture missing runtime"); }
                        auto& r = *u.action_status_runtime;
                        r[0x3D] = 0;
                        r[0x3E] = 2;
                        r[0x3C] = r[0x3F] = 0;
                        r[0x3D] = 0;
                        u.status3c = u.status3d = u.status3f = 0;
                        r[0x91] = r[0x92] = r[0x93] = r[0x94] = r[0x97] = r[0x98] = 0;
                        if (NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61)
                        {
                            u.equipment = { };
                            u.movement_type = 0;
                            u.status39 = 0;
                            u.attack = 203;
                            u.defense = 157;
                            u.base_action_points = u.max_action_points = 100;
                            u.action_points = 80;
                            u.carry_points = 70;
                            r[0x38] = r[0x39] = r[0x3A] = r[0x3B] = 0;
                            r[0x5D] = r[0x68] = r[0x7C] = 0;
                            auto put = [&](size_t p, int v)
                            {
                                r[p] = uint8_t(v);
                                r[p + 1] = uint8_t(unsigned(v) >> 8);
                            };
                            put(0xBF, 100);
                            put(0xC1, 211);
                            if (inputSkill == 8 || inputSkill == 48 || inputSkill == 56)
                            {
                                r[0x38] = 0x80;
                                r[0x39] = 4;
                                r[0x3D] = 0x80;
                                r[0x3E] = 0;
                                r[0x5D] = 3;
                                r[0x68] = 5;
                                r[0x94] = 3;
                            }
                            put(0xC3, 203);
                            put(0xC5, 157);
                            put(0x134, 203);
                            put(0x136, 157);
                            put(0x12C, 100);
                            put(0x12E, 80);
                            put(0x130, 70);
                            if (n == 2 && (inputSkill == 25 || inputSkill == 61)) { u.movement_type = 7; }
                        }
                    }
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.seed(1);
                            state.controlledSeed = rng.seed_state();
                            state.controlledCalls = rng.draw_count();
                            return true;
                        });
                    if (inputSkill == 8 || inputSkill == 56)
                    {
                        auto& u = units[skillInputTarget];
                        u.status39 = u.action_status_runtime->at(0x39);
                        u.status3c = u.action_status_runtime->at(0x3C);
                        u.status3d = u.action_status_runtime->at(0x3D);
                        u.status3e = u.action_status_runtime->at(0x3E);
                        u.status3f = u.action_status_runtime->at(0x3F);
                    }
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED hostile timed status ST/abilities/status/roles/RNG/deployment; two existing enemies relocated (053 inside area, 013/015 outside single tile); learned bit explicitly injected on actor1; source SAVE untouched; NOT natural\n";
                }
                if (inputSkill == 2 && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& units = prologue->events.units;
                    auto& actor = units[skillInputCaster];
                    auto center = units[skillInputTarget];
                    actor.st = 1000;
                    state.controlledAreaTargets = { skillInputTarget };
                    std::vector<Battle::Point> cells;
                    OriginalSkills probe(root);
                    probe.open(actor, { });
                    for (unsigned n = 0; n < 150 && probe.selected().id != 2; ++n)
                    {
                        bool has = false;
                        for (const auto& r : probe.groups()[probe.selected_group()].records) { has |= r.id == 2; }
                        probe.navigate(has ? 0x50 : 0x4D);
                    }
                    for (int y = center.y - 2; y <= center.y + 2; ++y)
                    {
                        for (int x = center.x - 2; x <= center.x + 2; ++x)
                        {
                            auto candidate = center;
                            candidate.x = x;
                            candidate.y = y;
                            if (!probe.sleep2_area(center, candidate) || !prologue->battle().skill_area_cell({ x, y }) || (x == center.x && y == center.y)) { continue; }
                            if (std::none_of(units.begin(), units.end(), [&](const auto& v)
                                    {
                                        return v.present(prologue->battle_turn()) && v.x == x && v.y == y;
                                    }))
                            {
                                cells.push_back({ x, y });
                            }
                        }
                    }
                    std::cout << "CONTROLLED probe=" << probe.selected().id << " center=" << center.x << "," << center.y << " emptyValid=" << cells.size() << "\n";
                    for (size_t n = 0; n < units.size() && state.controlledAreaTargets.size() < 3; ++n)
                    {
                        if (n == skillInputTarget || n == skillInputCaster || (inputSkill == 48 && state.controlledAreaTargets.size() == 1 ? units[n].hostile() : !units[n].hostile()) || cells.empty()) { continue; }
                        auto pos = cells.back();
                        cells.pop_back();
                        units[n].hp = std::max(1, units[n].hp);
                        units[n].first_turn = 1;
                        units[n].last_turn = 0;
                        units[n].x = pos.x;
                        units[n].y = pos.y;
                        units[n].status_flags = uint8_t((units[n].status_flags | 0x80) & ~0x20);
                        state.controlledAreaTargets.push_back(n);
                    }
                    if (state.controlledAreaTargets.size() != 3) { fatal("Controlled002 needs three existing enemies and empty valid area cells"); }
                    for (size_t n = 0; n < 3; ++n)
                    {
                        auto& v = units[state.controlledAreaTargets[n]];
                        v.st = 1;
                        v.runtime_role_selector = n == 2 ? 1 : 0;
                        v.abilities = n == 1 ? 0x4000 : 0;
                        v.status3e = 0;
                        if (!v.action_status_runtime) { fatal("Controlled002 missing runtime"); }
                        v.action_status_runtime->at(0x3E) = 0;
                        v.action_status_runtime->at(0x96) = 0;
                    }
                    prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                        {
                            rng.seed(1);
                            state.controlledSeed = rng.seed_state();
                            state.controlledCalls = rng.draw_count();
                            return true;
                        });
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED002 explicit runtime fixture: three original enemy slots activated/relocated, HP/ST/role/immunity/seed injected; NOT natural source; empty area cells retained\n";
                }
                if ((inputSkill == 4 || inputSkill == 5) && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& units = prologue->events.units;
                    const auto center = units[skillInputTarget];
                    unsigned n = 0;
                    OriginalSkills probe(root);
                    probe.open(units[skillInputCaster], { });
                    while (probe.selected().id != inputSkill) { probe.navigate(0x50); }
                    for (auto& v : units)
                    {
                        if (!v.present(prologue->battle_turn()) || !probe.status_area_target(center, v) || !probe.sleep2_area(center, v)) { continue; }
                        auto& r = *v.action_status_runtime;
                        if (inputSkill == 4)
                        {
                            r[0x38] &= uint8_t(~0x20u);
                            r[0x57] = 0;
                            if (n == 1 || n == 2)
                            {
                                r[0x38] |= 0x20;
                                r[0x57] = n == 1 ? 1 : 255;
                            }
                        }
                        else
                        {
                            r[0x3C] &= uint8_t(~0x20u);
                            r[0x8A] = 0;
                            v.status3c = r[0x3C];
                        }
                        state.controlledAreaTargets.push_back(size_t(&v - units.data()));
                        ++n;
                    }
                    if (inputSkill == 4 && n < 3) { fatal("004 mixed production requires three original allied targets"); }
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED004/005 explicit status/timer fixture only; original learned/positions retained, NOT natural success, targets=" << n << '\n';
                }
                const auto& u = prologue->events.units[skillInputCaster];
                auto point = prologue->screen_cell({ u.x, u.y });
                if (NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || skill94Input || inputSkill == 32 || inputSkill == 33 || inputSkill == 1 || ((inputSkill >= 2 && inputSkill <= 10) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96))
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
                if ((NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || skill94Input || inputSkill == 32 || inputSkill == 33 || inputSkill == 1 || ((inputSkill >= 2 && inputSkill <= 10) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96)) && skillInputPhase == 0 && (point.x < 32 || point.x > 608 || point.y < 32 || point.y > 370))
                {
                    if (prologue->battle_state() != s2ne::Battle::State::idle)
                    {
                        key(SDLK_ESCAPE);
                        return true;
                    }
                    SDL_Event e{ };
                    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    e.button.button = SDL_BUTTON_RIGHT;
                    SDL_RenderCoordinatesToWindow(renderer, 320, 240, &e.button.x, &e.button.y);
                    SDL_PushEvent(&e);
                    skillInputPhase = 30;
                }
                else if (prologue->battle_state() != s2ne::Battle::State::idle) { key(SDLK_ESCAPE); }
                else if (point.x < 32 || point.x > 608 || point.y < 32 || point.y > 370)
                {
                    SDL_Keycode keyCode{ };
                    if (point.x < 32) { keyCode = SDLK_LEFT; }
                    else
                    {
                        if (point.x > 608) { keyCode = SDLK_RIGHT; }
                        else
                        {
                            if (point.y < 32) { keyCode = SDLK_UP; }
                            else
                            {
                                keyCode = SDLK_DOWN;
                            }
                        }
                    }
                    key(keyCode);
                }
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
        else if (skillInputPhase >= 30 && skillInputPhase <= 33)
        {
            if (skillInputPhase == 30 && !context.battleMenu)
            {
                SDL_Event retry{ };
                retry.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                retry.button.windowID = SDL_GetWindowID(window);
                retry.button.button = SDL_BUTTON_RIGHT;
                SDL_RenderCoordinatesToWindow(renderer, 320, 240, &retry.button.x, &retry.button.y);
                SDL_PushEvent(&retry);
                return true;
            }
            if ((skillInputPhase <= 32 && !context.battleMenu) || (skillInputPhase == 32 && !context.battleReports) || (skillInputPhase == 33 && (context.battleMenu || context.battleReports)))
            {
                s2ne::fatal("Skill production roster/menu transition failed phase=" + std::to_string(skillInputPhase) + " battle=" + std::to_string(int(prologue->battle_state())) + " menu=" + std::to_string(bool(context.battleMenu)));
            }
            SDL_Event e{ };
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.windowID = SDL_GetWindowID(window);
            e.button.button = SDL_BUTTON_LEFT;
            std::cout << "INPUT33 phase=" << skillInputPhase << " menu=" << bool(context.battleMenu) << " roster=" << bool(context.battleReports) << " camera=" << prologue->camera().first << ',' << prologue->camera().second << '\n';
            float x = 320, y = 220;
            if (skillInputPhase == 32)
            {
                x = 560;
                y = 400;
            }
            if (skillInputPhase == 33)
            {
                if ((inputSkill == 4 || inputSkill == 5) && !options.skillControl.empty() && !state.controlledAreaReady)
                {
                    auto& units = prologue->events.units;
                    const auto center = units[skillInputTarget];
                    unsigned n = 0;
                    OriginalSkills probe(root);
                    probe.open(units[skillInputCaster], { });
                    while (probe.selected().id != inputSkill) { probe.navigate(0x50); }
                    for (auto& v : units)
                    {
                        if (!v.present(prologue->battle_turn()) || !probe.status_area_target(center, v) || !probe.sleep2_area(center, v)) { continue; }
                        auto& r = *v.action_status_runtime;
                        if (inputSkill == 4)
                        {
                            r[0x38] &= uint8_t(~0x20u);
                            r[0x57] = 0;
                            if (n == 1 || n == 2)
                            {
                                r[0x38] |= 0x20;
                                r[0x57] = n == 1 ? 1 : 255;
                            }
                        }
                        else
                        {
                            r[0x3C] &= uint8_t(~0x20u);
                            r[0x8A] = 0;
                            v.status3c = r[0x3C];
                        }
                        state.controlledAreaTargets.push_back(size_t(&v - units.data()));
                        ++n;
                    }
                    if (inputSkill == 4 && n < 3) { fatal("004 mixed production requires three original allied targets"); }
                    state.controlledAreaReady = true;
                    std::cout << "CONTROLLED004/005 explicit status/timer fixture only; original learned/positions retained, NOT natural success, targets=" << n << '\n';
                }
                const auto& u = prologue->events.units[skillInputCaster];
                auto point = prologue->screen_cell({ u.x, u.y });
                x = float(point.x);
                y = float(point.y);
                auto [cx, cy] = prologue->camera();
                for (int dy = -32; dy <= 32; ++dy)
                {
                    bool picked = false;
                    for (int dx = -32; dx <= 32; ++dx)
                    {
                        if (prologue->battle().pick_surface(float(point.x + dx), float(point.y + dy), cx, cy) == s2ne::Battle::Point{ u.x, u.y })
                        {
                            x = float(point.x + dx);
                            y = float(point.y + dy);
                            picked = true;
                            break;
                        }
                    }
                    if (picked) { break; }
                }
                std::cout << "INPUT33 roster locate then select id=" << u.id << " xy=" << u.x << ',' << u.y << '\n';
            }
            SDL_RenderCoordinatesToWindow(renderer, x, y, &e.button.x, &e.button.y);
            SDL_Event motion{ };
            motion.type = SDL_EVENT_MOUSE_MOTION;
            motion.motion.windowID = SDL_GetWindowID(window);
            motion.motion.x = e.button.x;
            motion.motion.y = e.button.y;
            SDL_PushEvent(&motion);
            SDL_PushEvent(&e);
            skillInputPhase = skillInputPhase == 33 ? 1 : skillInputPhase + 1;
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
                if (prologue->battle().attack_index() != int((skill88Input || skill94Input || inputSkill == 96 || inputSkill == 86 || inputSkill == 92 || (NativeAbilityBuffs::find(inputSkill) && NativeAbilityBuffs::find(inputSkill)->group == 8)) ? 3 : 2))
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
            if (skill94Input && !options.skillControl.empty())
            {
                prologue->battle().mutate_combat_random([&](auto& rng, uint32_t seconds)
                    {
                        rng.initialize(12);
                        for (unsigned seed = 0; seed < 1000; ++seed)
                        {
                            auto probe = rng;
                            probe.restore_stream(seed, 0);
                            probe.bounded(10, seconds);
                            probe.bounded(10, seconds);
                            if (probe.percent(seconds) <= 30 && probe.percent(seconds) < 30)
                            {
                                rng.restore_stream(seed, 0);
                                std::cout << "CONTROLLED094 fixed preflight RNG seed=" << seed << " original percent table12, no live retry\n";
                                return true;
                            }
                        }
                        fatal("Controlled094 deterministic seed unavailable");
                    });
            }
            skillInputBefore = prologue->events.units[skillInputCaster];
            skillInputVictim = prologue->events.units[skillInputTarget];
            skillInputCommitted = snapshot("cast-before.sav");
            prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                {
                    state.controlledSeed = rng.seed_state();
                    state.controlledCalls = rng.draw_count();
                    return false;
                });
            key(SDLK_RETURN);
            ++skillInputPhase;
        }
        else if (skillInputPhase == 9 && !skillMenu && !pendingSkillTarget)
        {
            if (skill94Input && prologue->synchronous_death_active())
            {
                if (skillCastVerified || prologue->original_save_boundary_ready() || !prologue->battle().combat_feedback_active()) { fatal("094 script prematurely published/unlocked"); }
                if (!state.scriptDeathObserved)
                {
                    state.scriptDeathObserved = true;
                    std::cout << "CONTROLLED094 production lethal->original synchronous script stage=" << prologue->events.stage << " role=" << prologue->events.units[skillInputTarget].id << " flag13=" << unsigned(prologue->events.flags[12]) << " save/input locked; no payment\n";
                }
                if (options.skillControl == "scriptfail82" && !state.scriptFailureInjected)
                {
                    // A runtime-invalid speaker exercises the real dialogue Result
                    // failure, after preflight and damage, not an event-complete shortcut.
                    prologue->events.units.clear();
                    state.scriptFailureInjected = true;
                    std::cout << "CONTROLLED094 injecting runtime missing speaker after synchronous entry for script Result rollback\n";
                }
                key(SDLK_RETURN);
                return true;
            }
            if (options.skillControl == "scriptfail82" && state.scriptFailureInjected)
            {
                if (skillCastVerified || !prologue->original_save_boundary_ready() || snapshot("script-rollback-after.sav") != skillInputCommitted) { fatal("094 script failure leaked state/payment/input lock"); }
                prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                    {
                        if (rng.seed_state() != state.controlledSeed || rng.draw_count() != state.controlledCalls) { fatal("094 script failure changed RNG"); }
                        return false;
                    });
                std::cout << "PASS CONTROLLED094 production script failure restores units/events/flags/currency/inventory/payment/reward/RNG and input/save boundary; no return/payment callback\n";
                state.skillInputVerified = true;
                running = false;
                return true;
            }
            if (prologue->battle().combat_feedback_active())
            {
                if (((NativeAbilityBuffs::find(inputSkill) || NativeTimedBuffs::find(inputSkill) || (inputSkill >= 2 && inputSkill <= 10) || inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96) || (skill94Input && !prologue->synchronous_death_active())) && snapshot("feedback-unpublished.sav") != skillInputCommitted) { fatal("Area feedback published before all targets presented"); }
                return true;
            }
            if (options.skillControl == "rollback")
            {
                if (skillCastVerified || snapshot("rollback-after.sav") != skillInputCommitted) { fatal("Controlled002 rejected award published state"); }
                prologue->battle().mutate_combat_random([&](auto& rng, uint32_t)
                    {
                        if (rng.seed_state() != state.controlledSeed || rng.draw_count() != state.controlledCalls) { fatal("Controlled002 rejected award changed RNG"); }
                        return false;
                    });
                std::cout << "PASS CONTROLLED production area rejected award: all targets/state/payment/reward/RNG rolled back\n";
                state.skillInputVerified = true;
                running = false;
                return true;
            }
            if (!skillCastVerified)
            {
                s2ne::fatal("Normal skill cast did not commit");
            }
            const auto& u = prologue->events.units[skillInputCaster];
            bool payment_mismatch = false;
            int expected_mp_cost = 0;
            if (!NativeAbilityBuffs::find(inputSkill) && !NativeTimedBuffs::find(inputSkill))
            {
                if (skill88Input || skill94Input)
                {
                    int st_cost = 25;
                    if (skill94Input) { st_cost = 5; }
                    payment_mismatch = u.st != skillInputBefore->st + (u.max_st - skillInputBefore->max_st) - st_cost;
                }
                else
                {
                    if (inputSkill == 6 || inputSkill == 9 || inputSkill == 10)
                    {
                        if ((skillInputBefore->abilities & 0x20))
                        {
                            if (inputSkill == 6) { expected_mp_cost = 16; }
                            else if (inputSkill == 9) { expected_mp_cost = 20; }
                            else
                            {
                                expected_mp_cost = 27;
                            }
                        }
                        else if (inputSkill == 6) { expected_mp_cost = 32; }
                        else if (inputSkill == 9) { expected_mp_cost = 40; }
                        else
                        {
                            expected_mp_cost = 55;
                        }
                    }
                    else if (inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60 || inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61 || inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96)
                    {
                        if (inputSkill == 96) { expected_mp_cost = 0; }
                        else if ((skillInputBefore->abilities & 0x20))
                        {
                            if (inputSkill == 8 || inputSkill == 35 || inputSkill == 56) { expected_mp_cost = 10; }
                            else if (inputSkill == 12) { expected_mp_cost = 6; }
                            else if (inputSkill == 39) { expected_mp_cost = 12; }
                            else if (inputSkill == 48 || inputSkill == 60) { expected_mp_cost = 15; }
                            else if (inputSkill == 17 || inputSkill == 25) { expected_mp_cost = 12; }
                            else if (inputSkill == 52) { expected_mp_cost = 22; }
                            else if (inputSkill == 61) { expected_mp_cost = 20; }
                            else if (inputSkill == 34) { expected_mp_cost = 6; }
                            else if (inputSkill == 38 || inputSkill == 54) { expected_mp_cost = 10; }
                            else if (inputSkill == 15) { expected_mp_cost = 15; }
                            else
                            {
                                expected_mp_cost = 7;
                            }
                        }
                        else if (inputSkill == 8 || inputSkill == 35 || inputSkill == 56) { expected_mp_cost = 20; }
                        else if (inputSkill == 12) { expected_mp_cost = 13; }
                        else if (inputSkill == 39) { expected_mp_cost = 25; }
                        else if (inputSkill == 48 || inputSkill == 60) { expected_mp_cost = 30; }
                        else if (inputSkill == 17 || inputSkill == 25) { expected_mp_cost = 25; }
                        else if (inputSkill == 52) { expected_mp_cost = 45; }
                        else if (inputSkill == 61) { expected_mp_cost = 40; }
                        else if (inputSkill == 34) { expected_mp_cost = 13; }
                        else if (inputSkill == 38 || inputSkill == 54) { expected_mp_cost = 20; }
                        else if (inputSkill == 15) { expected_mp_cost = 30; }
                        else
                        {
                            expected_mp_cost = 15;
                        }
                    }
                    else if (inputSkill == 4)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 10; }
                        else
                        {
                            expected_mp_cost = 20;
                        }
                    }
                    else if (inputSkill == 5)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 15; }
                        else
                        {
                            expected_mp_cost = 30;
                        }
                    }
                    else if (inputSkill == 33)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 3; }
                        else
                        {
                            expected_mp_cost = 7;
                        }
                    }
                    else if (inputSkill == 3)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 6; }
                        else
                        {
                            expected_mp_cost = 12;
                        }
                    }
                    else if (inputSkill == 2)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 9; }
                        else
                        {
                            expected_mp_cost = 18;
                        }
                    }
                    else if (inputSkill == 1)
                    {
                        if ((skillInputBefore->abilities & 0x20)) { expected_mp_cost = 4; }
                        else
                        {
                            expected_mp_cost = 8;
                        }
                    }
                    else
                    {
                        expected_mp_cost = 40;
                    }

                    payment_mismatch = u.mp != skillInputBefore->mp + skillInputMpGrowth - expected_mp_cost;
                }
                int expectedFrames;
                if (inputSkill == 8) { expectedFrames = 37; }
                else if (inputSkill == 12) { expectedFrames = 32; }
                else if (inputSkill == 35) { expectedFrames = 22; }
                else if (inputSkill == 39) { expectedFrames = 30; }
                else if (inputSkill == 48) { expectedFrames = 36; }
                else if (inputSkill == 56) { expectedFrames = 34; }
                else if (inputSkill == 60) { expectedFrames = 33; }
                else if (inputSkill == 17) { expectedFrames = 31; }
                else if (inputSkill == 25) { expectedFrames = 28; }
                else if (inputSkill == 52) { expectedFrames = 63; }
                else if (inputSkill == 61) { expectedFrames = 32; }
                else if (inputSkill == 34) { expectedFrames = 36; }
                else if (inputSkill == 38 || inputSkill == 96) { expectedFrames = 30; }
                else if (inputSkill == 54) { expectedFrames = 26; }
                else if (inputSkill == 13) { expectedFrames = 25; }
                else if (inputSkill == 15) { expectedFrames = 38; }
                else if (inputSkill == 53) { expectedFrames = 27; }
                else if (inputSkill == 6) { expectedFrames = 38; }
                else if (inputSkill == 9) { expectedFrames = 33; }
                else if (inputSkill == 10) { expectedFrames = 59; }
                else if (inputSkill == 4) { expectedFrames = 24; }
                else if (inputSkill == 5) { expectedFrames = 22; }
                else if (inputSkill == 3) { expectedFrames = 27; }
                else if (inputSkill == 2) { expectedFrames = 40; }
                else if (inputSkill == 1) { expectedFrames = 26; }
                else if (inputSkill == 33) { expectedFrames = 21; }
                else if (skill94Input) { expectedFrames = 15; }
                else if (skill88Input) { expectedFrames = 35; }
                else
                {
                    expectedFrames = 23;
                }

                payment_mismatch = payment_mismatch || (inputSkill == 96 && u.st != skillInputBefore->st - 15) || u.action_points >= skillInputBefore->action_points || skillInputFrames != expectedFrames;
            }
            if (payment_mismatch)
            {
                s2ne::fatal("Normal skill32 payment/frame count mismatch mp=" + std::to_string(u.mp) + " before=" + std::to_string(skillInputBefore->mp) + " ap=" + std::to_string(u.action_points) + " beforeAP=" + std::to_string(skillInputBefore->action_points) + " frames=" + std::to_string(skillInputFrames));
            }
            if (skill88Positive && (u.hp <= skillInputBefore->hp || u.experience <= skillInputBefore->experience))
            {
                s2ne::fatal("88 positive heal/XP not obtained; no retry or RNG filtering");
            }
            if (inputSkill == 1 && prologue->events.units[skillInputTarget].hp <= skillInputVictim->hp) { s2ne::fatal("Healing1 produced no positive target healing"); }
            if (skill94Input && !options.skillControl.empty())
            {
                const auto& v = prologue->events.units[skillInputTarget];
                if (options.skillControl == "kill" && (v.hp != 0 || !(v.status_flags & 0x20) || u.ending_kills != uint16_t(skillInputBefore->ending_kills + 1) || v.ending_losses != uint16_t(skillInputVictim->ending_losses + 1) || rawSave->groups[0].inventory[0] == 0)) { fatal("Controlled094 lethal/loot branch not reached"); }
                if ((options.skillControl == "revive25" || options.skillControl == "consume28") && (v.hp != v.max_hp || (v.status_flags & 0x20) || (options.skillControl == "consume28" ? v.equipment[0] != 0 : v.equipment[0] != skillInputVictim->equipment[0]))) { fatal("Controlled094 revival branch not reached"); }
                if (options.skillControl == "reverse29" && v.hp <= skillInputVictim->hp) { fatal("Controlled094 reversal not reached"); }
                std::cout << "PASS CONTROLLED094 " << options.skillControl << " targetHP=" << skillInputVictim->hp << "->" << v.hp << " kills=" << u.ending_kills << " losses=" << v.ending_losses << " equipment=" << v.equipment[0] << " inventory0=" << unsigned(rawSave->groups[0].inventory[0]) << '\n';
            }
            if (inputSkill == 6 || inputSkill == 9 || inputSkill == 10)
            {
                if (inputSkill == 10)
                {
                    const auto& v = prologue->events.units[skillInputTarget];
                    if (!(v.status3c & 0x40) || v.action_status_runtime->at(0x8B) != (options.skillControl == "branches" ? 255 : 2) || v.hp != skillInputVictim->hp) { fatal("010 native timed protection mismatch"); }
                }
                else
                {
                    if (options.skillControl == "branches")
                    {
                        const auto& full = prologue->events.units[state.controlledAreaTargets[0]];
                        const auto& blocked = prologue->events.units[state.controlledAreaTargets[1]];
                        if (full.hp != full.max_hp || blocked.hp != std::max(1, blocked.max_hp / 2)) { fatal("Family full/blocked production branch mutated HP"); }
                        std::cout << "PASS production fullHP skipped feedback/blocked zero feedback with positive neighbours; halfMP\n";
                    }
                    unsigned healed = 0;
                    for (auto i : state.controlledAreaTargets)
                    {
                        if (prologue->events.units[i].hp > prologue->events.units[i].max_hp / 2) { ++healed; }
                    }
                    if (healed < 2) { fatal("Production multi-target healing not reached"); }
                    std::cout << "PASS production multi-target healing changed=" << healed << '\n';
                }
            }
            if (inputSkill == 13 || inputSkill == 15 || inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96)
            {
                const unsigned byte = inputSkill == 53 || inputSkill == 34 || inputSkill == 38 || inputSkill == 54 || inputSkill == 96 ? 0x3D : 0x3E;
                unsigned bit;

                if (inputSkill == 34 || inputSkill == 96) { bit = 128; }
                else if (inputSkill == 38) { bit = 64; }
                else if (inputSkill == 54) { bit = 32; }
                else if (inputSkill == 13) { bit = 4; }
                else if (inputSkill == 15) { bit = 8; }
                else
                {
                    bit = 16;
                }

                unsigned timer;

                if (inputSkill == 34 || inputSkill == 96) { timer = 0x94; }
                else if (inputSkill == 38) { timer = 0x93; }
                else if (inputSkill == 54) { timer = 0x92; }
                else if (inputSkill == 13) { timer = 0x97; }
                else if (inputSkill == 15) { timer = 0x98; }
                else
                {
                    timer = 0x91;
                }

                if (state.controlledAreaReady)
                {
                    for (size_t n = 0; n < (inputSkill == 53 ? state.controlledAreaTargets.size() : 1); ++n)
                    {
                        const auto& v = prologue->events.units[state.controlledAreaTargets[n]];
                        const auto& r = *v.action_status_runtime;
                        int expectedDuration = 0;
                        if (n == 0)
                        {
                            if (inputSkill == 53 || inputSkill == 34 || inputSkill == 96) { expectedDuration = 3; }
                            else
                            {
                                expectedDuration = 4;
                            }
                        }
                        if (bool(r[byte] & bit) != (n == 0) || r[timer] != expectedDuration || bool(r[0x3E] & 2) != (n == 2)) { fatal("Hostile status mixed hit/immune/role miss/wake/timer mismatch"); }
                    }
                    unsigned int group;
                    if (NativeAbilityBuffs::find(inputSkill)) { group = NativeAbilityBuffs::find(inputSkill)->group; }
                    else if (NativeTimedBuffs::find(inputSkill)) { group = NativeTimedBuffs::find(inputSkill)->group; }
                    else if (inputSkill == 8) { group = 0; }
                    else if (inputSkill == 12) { group = 1; }
                    else if (inputSkill == 35 || inputSkill == 39) { group = 3; }
                    else if (inputSkill == 48) { group = 4; }
                    else if (inputSkill == 56 || inputSkill == 60) { group = 5; }
                    else if (inputSkill == 17) { group = 1; }
                    else if (inputSkill == 25) { group = 2; }
                    else if (inputSkill == 52) { group = 4; }
                    else if (inputSkill == 61) { group = 5; }
                    else if (inputSkill == 96) { group = 9; }
                    else if (inputSkill == 34 || inputSkill == 38) { group = 3; }
                    else if (inputSkill == 54) { group = 5; }
                    else if (inputSkill == 13 || inputSkill == 15) { group = 1; }
                    else if (inputSkill == 53) { group = 5; }
                    else if ((inputSkill >= 1 && inputSkill <= 10)) { group = 0; }
                    else if (inputSkill == 33) { group = 3; }
                    else if (skill94Input) { group = 9; }
                    else if (skill88Input) { group = 8; }
                    else
                    {
                        group = 2;
                    }

                    int cost;

                    if (inputSkill == 34) { cost = 13; }
                    else if (inputSkill == 38 || inputSkill == 54) { cost = 20; }
                    else if (inputSkill == 15) { cost = 30; }
                    else
                    {
                        cost = 15;
                    }

                    const int proficiency = (cost + 6 - int(prologue->events.difficulty)) * 5 / 2 + 10;
                    if (u.action_points != skillInputBefore->action_points - 45 || u.learned_proficiency[group] != skillInputBefore->learned_proficiency[group] + proficiency || u.experience != skillInputBefore->experience + 6) { fatal("Hostile native once-only reward/AP oracle mismatch"); }
                    std::cout << "PASS CONTROLLED hostile native status/timer and wake; 053 mixed new/immune/role MISS; no damage; halfMP once\n";
                }
                if (prologue->events.units[skillInputTarget].hp != skillInputVictim->hp) { fatal("Zero-power hostile status changed HP"); }
            }
            if (inputSkill == 17 || inputSkill == 25 || inputSkill == 52 || inputSkill == 61)
            {
                const auto& v = prologue->events.units[skillInputTarget];
                const auto& r = *v.action_status_runtime;
                unsigned timer;

                if (inputSkill == 17) { timer = 0x68; }
                else if (inputSkill == 52) { timer = 0x7C; }
                else
                {
                    timer = 0x5D;
                }

                int power;

                if (inputSkill == 52) { power = 40; }
                else if (inputSkill == 25) { power = 10; }
                else
                {
                    power = 12;
                }

                int expectedDuration;
                if (inputSkill == 17) { expectedDuration = 5; }
                else
                {
                    if (inputSkill == 52) { expectedDuration = 4; }
                    else
                    {
                        expectedDuration = 3;
                    }
                }
                int expectedAttack = 203;
                int expectedDefense = 157;
                if (inputSkill == 17)
                {
                    expectedAttack = 163;
                    expectedDefense = 126;
                }
                else if (inputSkill == 52)
                {
                    expectedAttack = 183;
                    expectedDefense = 142;
                }
                if (v.hp != skillInputVictim->hp || r[timer] != expectedDuration || v.attack != expectedAttack || v.defense != expectedDefense || v.max_action_points != (inputSkill == 17 ? 100 : 100 - power) || v.action_points != (inputSkill == 25 || inputSkill == 61 ? 80 - power : 80)) { fatal("Stat family independent native values/timer mismatch"); }
                unsigned int group;
                if (NativeAbilityBuffs::find(inputSkill)) { group = NativeAbilityBuffs::find(inputSkill)->group; }
                else if (NativeTimedBuffs::find(inputSkill)) { group = NativeTimedBuffs::find(inputSkill)->group; }
                else if (inputSkill == 8) { group = 0; }
                else if (inputSkill == 12) { group = 1; }
                else if (inputSkill == 35 || inputSkill == 39) { group = 3; }
                else if (inputSkill == 48) { group = 4; }
                else if (inputSkill == 56 || inputSkill == 60) { group = 5; }
                else if (inputSkill == 17) { group = 1; }
                else if (inputSkill == 25) { group = 2; }
                else if (inputSkill == 52) { group = 4; }
                else if (inputSkill == 61) { group = 5; }
                else if (inputSkill == 96) { group = 9; }
                else if (inputSkill == 34 || inputSkill == 38) { group = 3; }
                else if (inputSkill == 54) { group = 5; }
                else if (inputSkill == 13 || inputSkill == 15) { group = 1; }
                else if (inputSkill == 53) { group = 5; }
                else if ((inputSkill >= 1 && inputSkill <= 10)) { group = 0; }
                else if (inputSkill == 33) { group = 3; }
                else if (skill94Input) { group = 9; }
                else if (skill88Input) { group = 8; }
                else
                {
                    group = 2;
                }

                int cost;

                if (inputSkill == 52) { cost = 45; }
                else if (inputSkill == 61) { cost = 40; }
                else
                {
                    cost = 25;
                }

                if (u.action_points != skillInputBefore->action_points - 45 || u.learned_proficiency[group] != skillInputBefore->learned_proficiency[group] + (cost + 6 - int(prologue->events.difficulty)) * 5 / 2 + 10) { fatal("Stat family once reward/payment mismatch"); }
                for (size_t n = 1; n < state.controlledAreaTargets.size(); ++n)
                {
                    const auto& other = prologue->events.units[state.controlledAreaTargets[n]];
                    if (other.attack != 203 || other.defense != 157 || other.max_action_points != (inputSkill == 25 && n == 2 ? 90 : 100)) { fatal("Stat mixed immune/excluded neighbour mutated"); }
                }
                std::cout << "PASS CONTROLLED stat family exact attack/defense/AP current/limit/carry reduction and timer; mixed immunity/exclusion; unique payment\n";
            }
            if (inputSkill == 8 || inputSkill == 12 || inputSkill == 35 || inputSkill == 39 || inputSkill == 48 || inputSkill == 56 || inputSkill == 60)
            {
                const auto& v = prologue->events.units[skillInputTarget];
                const auto& r = *v.action_status_runtime;
                if (v.hp != skillInputVictim->hp || (inputSkill == 35 && !(r[0x38] & 2)) || (inputSkill == 39 && (v.attack != 161 || r[0x51] != 3)) || (inputSkill == 60 && (v.attack != 102 || v.defense != 79 || r[0x63] != 0)) || (inputSkill == 12 && !(r[0x39] & 4)) || (inputSkill == 8 && (r[0x39] & 4)) || (inputSkill == 48 && !(r[0x38] & 0x80)) || (inputSkill == 56 && (r[0x38] || r[0x39] || r[0x3C] || r[0x3D] || r[0x3E] || r[0x3F]))) { fatal("Special family independent production status oracle"); }
                unsigned int group;
                if (NativeAbilityBuffs::find(inputSkill)) { group = NativeAbilityBuffs::find(inputSkill)->group; }
                else if (NativeTimedBuffs::find(inputSkill)) { group = NativeTimedBuffs::find(inputSkill)->group; }
                else if (inputSkill == 8) { group = 0; }
                else if (inputSkill == 12) { group = 1; }
                else if (inputSkill == 35 || inputSkill == 39) { group = 3; }
                else if (inputSkill == 48) { group = 4; }
                else if (inputSkill == 56 || inputSkill == 60) { group = 5; }
                else if (inputSkill == 17) { group = 1; }
                else if (inputSkill == 25) { group = 2; }
                else if (inputSkill == 52) { group = 4; }
                else if (inputSkill == 61) { group = 5; }
                else if (inputSkill == 96) { group = 9; }
                else if (inputSkill == 34 || inputSkill == 38) { group = 3; }
                else if (inputSkill == 54) { group = 5; }
                else if (inputSkill == 13 || inputSkill == 15) { group = 1; }
                else if (inputSkill == 53) { group = 5; }
                else if ((inputSkill >= 1 && inputSkill <= 10)) { group = 0; }
                else if (inputSkill == 33) { group = 3; }
                else if (skill94Input) { group = 9; }
                else if (skill88Input) { group = 8; }
                else
                {
                    group = 2;
                }

                int cost;

                if (inputSkill == 12) { cost = 13; }
                else if (inputSkill == 39) { cost = 25; }
                else if (inputSkill == 48 || inputSkill == 60) { cost = 30; }
                else
                {
                    cost = 20;
                }

                if (u.action_points != skillInputBefore->action_points - 45 || u.experience != skillInputBefore->experience + 6 || u.learned_proficiency[group] != skillInputBefore->learned_proficiency[group] + (cost + 6 - int(prologue->events.difficulty)) * 5 / 2 + 10) { fatal("Special family duplicate payment/reward"); }
                if (inputSkill == 48)
                {
                    const auto& friendTarget = prologue->events.units[state.controlledAreaTargets[1]];
                    const auto& enemyTarget = prologue->events.units[state.controlledAreaTargets[2]];
                    if (friendTarget.hostile() || !enemyTarget.hostile() || (friendTarget.action_status_runtime->at(0x39) & 4) || (friendTarget.action_status_runtime->at(0x3D) & 0x80) || !(enemyTarget.action_status_runtime->at(0x39) & 4)) { fatal("048 production mixed friendly cure/enemy selective dispel mismatch"); }
                    std::cout << "PASS CONTROLLED048 same transaction friendly cure/enemy selective dispel/ability immunity bypass; save all targets\n";
                }
                std::cout << "PASS CONTROLLED special/clear native status oracle ID=" << inputSkill << "; no HP damage; single transaction payment\n";
            }
            if (inputSkill == 33)
            {
                const auto& victim = prologue->events.units[skillInputTarget];
                if (victim.hp != skillInputVictim->hp || u.learned != skillInputBefore->learned || u.learned_proficiency[3] <= skillInputBefore->learned_proficiency[3])
                {
                    s2ne::fatal("Curse33 HP/learning/proficiency invariant failed");
                }
                std::cout << "PASS INPUT33 natural learned[3]=" << u.learned[3] << " proficiency3=" << skillInputBefore->learned_proficiency[3] << "->" << u.learned_proficiency[3] << " targetHP=" << victim.hp << " status3e=" << unsigned(victim.status3e) << '\n';
            }
            if (inputSkill == 3)
            {
                const auto& v = prologue->events.units[skillInputTarget];
                if (!(v.status3f & 4) || !v.action_status_runtime || v.action_status_runtime->at(0x9F) < 4 || v.hp != skillInputVictim->hp) { fatal("Blessing3 production status/timer/HP mismatch"); }
                std::cout << "PASS INPUT003 original friendly nine-cell state4000000/timer9F/unchangedHP\n";
            }
            if (state.controlledAreaReady && inputSkill == 2)
            {
                if (u.action_points != skillInputBefore->action_points - 45 || u.experience != skillInputBefore->experience + 6 || u.learned_proficiency[0] != skillInputBefore->learned_proficiency[0] + 57) { fatal("Controlled002 once-only AP/XP/proficiency mismatch"); }
                for (size_t n = 0; n < 3; ++n)
                {
                    const auto& v = prologue->events.units[state.controlledAreaTargets[n]];
                    if (bool(v.status3e & 2) != (n == 0) || v.action_status_runtime->at(0x96) != (n == 0 ? 2 : 0)) { fatal("Controlled002 mixed HIT/immune/MISS status mismatch"); }
                }
                std::cout << "PASS CONTROLLED002 production nine-cell mixed HIT/ability immunity/role MISS/empty cells; once-only payment and serial feedback committed\n";
            }
            if (inputSkill == 4 || inputSkill == 5)
            {
                if (u.action_points != skillInputBefore->action_points - 45 || u.experience != skillInputBefore->experience + 6 || u.learned_proficiency[0] != skillInputBefore->learned_proficiency[0] + (inputSkill == 4 ? 62 : 87)) { fatal("004/005 once payment/reward mismatch"); }
                const auto& v = prologue->events.units[skillInputTarget];
                if (!v.action_status_runtime || v.hp != skillInputVictim->hp) { fatal("004/005 HP/runtime mismatch"); }
                if (inputSkill == 5 && (!(v.status3c & 0x20) || v.action_status_runtime->at(0x8A) < 2)) { fatal("005 mask20/timer8A mismatch"); }
                if (inputSkill == 4 && state.controlledAreaReady)
                {
                    unsigned n = 0;
                    for (auto i : state.controlledAreaTargets)
                    {
                        const auto& r = *prologue->events.units[i].action_status_runtime;
                        if (!(r[0x38] & 0x20) || r[0x57] != (n == 2 ? 255 : 3)) { fatal("004 production mixed new/extend/permanent mismatch"); }
                        ++n;
                    }
                }
                std::cout << "PASS INPUT004/005 exact status/timer and once-only MP/AP/XP/proficiency; HP unchanged\n";
            }
            if (const auto* ability = NativeAbilityBuffs::find(inputSkill))
            {
                const int cost = ability->tail[0];
                const unsigned flags = ability->tail[20];
                if (u.mp != skillInputBefore->mp + skillInputMpGrowth - ((flags & 1) ? cost / 2 : 0) || u.st != skillInputBefore->st - ((flags & 2) ? cost : 0) || u.hp != skillInputBefore->hp - ((flags & 4) ? cost : 0) || u.action_points != skillInputBefore->action_points - 45 || u.learned_proficiency[ability->group] <= skillInputBefore->learned_proficiency[ability->group] || skillInputFrames != (ability->header[6] ? ability->header[6] : ability->header[0])) { fatal("Ability production payment/reward/original CEL mismatch"); }
                const auto& target = prologue->events.units[skillInputTarget];
                const auto& raw = *target.action_status_runtime;
                const unsigned effect = ability->tail[6];
                unsigned timer;

                if (effect == 2) { timer = 0x54; }
                else if (effect == 6) { timer = 0x4E; }
                else if (effect == 10) { timer = 0x5A; }
                else if (effect == 12) { timer = 0x60; }
                else if (effect == 63) { timer = 0x6D; }
                else if (effect == 84) { timer = 0x72; }
                else
                {
                    timer = 0x77;
                }

                unsigned mask;

                if (effect == 2) { mask = 0x10; }
                else if (effect == 6) { mask = 4; }
                else if (effect == 10) { mask = 0x40; }
                else if (effect == 12) { mask = 0x100; }
                else if (effect == 63) { mask = 0x800; }
                else if (effect == 84) { mask = 0x1000; }
                else
                {
                    mask = 0x2000;
                }

                const unsigned active = unsigned(raw[0x38]) | (unsigned(raw[0x39]) << 8);
                auto signedWord = [&](size_t p)
                {
                    return int(int16_t(unsigned(raw[p]) | (unsigned(raw[p + 1]) << 8)));
                };
                if (!(active & mask) || !raw[timer]) { fatal("Ability production independent mask/timer missing"); }
                if (effect == 2 && target.defense != skillInputVictim->defense + signedWord(timer + 1)) { fatal("Ability production defense delta mismatch"); }
                if (effect == 6 && target.attack != skillInputVictim->attack + signedWord(timer + 1)) { fatal("Ability production attack delta mismatch"); }
                if (effect == 12 && target.tactics != skillInputVictim->tactics + 30) { fatal("Ability production tactics delta mismatch"); }
                if ((effect == 63 || effect == 84 || effect == 86) && (target.attack != skillInputVictim->attack + signedWord(timer + 1) || target.defense != skillInputVictim->defense + signedWord(timer + 3))) { fatal("Ability production paired delta mismatch"); }
                if (effect == 10)
                {
                    const auto& beforeRaw = *skillInputVictim->action_status_runtime;
                    const bool existing = (beforeRaw[0x38] & 0x40) != 0;
                    const int expected = skillInputVictim->max_action_points + (existing ? 0 : signedWord(timer + 1));
                    if (target.max_action_points != expected || (!existing && skillInputVictim->max_action_points < 110 && target.max_action_points > 110))
                    {
                        std::cerr << "Ability AP before=" << skillInputVictim->max_action_points << " after=" << target.max_action_points << " delta=" << signedWord(timer + 1) << " existing=" << existing << " expected=" << expected << '\n';
                        fatal("Ability production AP delta/cap mismatch");
                    }
                }
                std::cout << "PASS CONTROLLED ability family ID=" << inputSkill << " original CEL, stat mask/delta/timer and unique MP/ST/HP/AP/reward transaction\n";
            }
            if (const auto* buff = NativeTimedBuffs::find(inputSkill))
            {
                const auto& v = prologue->events.units[skillInputTarget];
                const auto& r = *v.action_status_runtime;
                unsigned slot = 0;
                while (!(buff->mask & (1u << slot))) { ++slot; }
                const int duration = inputSkill == 58 ? std::max(1, skillInputBefore->st / 25) : buff->tail[12];
                const unsigned frames = buff->header[6] ? buff->header[6] : buff->header[0];
                const unsigned flags = unsigned(r[0x3C]) | (unsigned(r[0x3D]) << 8);
                const int cost = buff->tail[0];
                if (!(flags & buff->mask) || r[0x85 + slot] != (inputSkill == 46 ? 255 : duration) || skillInputFrames != frames || u.action_points != skillInputBefore->action_points - 45 || u.mp != skillInputBefore->mp + skillInputMpGrowth - (buff->group == 8 ? 0 : cost / 2) || (buff->group == 8 && u.st != skillInputBefore->st - cost) || u.learned_proficiency[buff->group] <= skillInputBefore->learned_proficiency[buff->group] || v.hp != skillInputVictim->hp - (inputSkill == 92 && skillInputTarget == skillInputCaster ? cost : 0)) { fatal("Timed buff production oracle/payment/CEL mismatch"); }
                std::cout << "PASS CONTROLLED timed buff ID=" << inputSkill << " mask=" << buff->mask << " timer=" << unsigned(r[0x85 + slot]) << " original branch, unique payment/reward, no HP damage\n";
            }
            skillInputCommitted = snapshot("cast-after.sav");
            unsigned int abilityGroup;
            if (NativeTimedBuffs::find(inputSkill)) { abilityGroup = NativeTimedBuffs::find(inputSkill)->group; }
            else if (inputSkill == 8) { abilityGroup = 0; }
            else if (inputSkill == 12) { abilityGroup = 1; }
            else if (inputSkill == 35 || inputSkill == 39) { abilityGroup = 3; }
            else if (inputSkill == 48) { abilityGroup = 4; }
            else if (inputSkill == 56 || inputSkill == 60) { abilityGroup = 5; }
            else if (inputSkill == 17) { abilityGroup = 1; }
            else if (inputSkill == 25) { abilityGroup = 2; }
            else if (inputSkill == 52) { abilityGroup = 4; }
            else if (inputSkill == 61) { abilityGroup = 5; }
            else if (inputSkill == 96) { abilityGroup = 9; }
            else if (inputSkill == 34 || inputSkill == 38) { abilityGroup = 3; }
            else if (inputSkill == 54) { abilityGroup = 5; }
            else if (inputSkill == 13 || inputSkill == 15) { abilityGroup = 1; }
            else if (inputSkill == 53) { abilityGroup = 5; }
            else if (skill94Input) { abilityGroup = 9; }
            else if (skill88Input) { abilityGroup = 8; }
            else if ((inputSkill >= 1 && inputSkill <= 10)) { abilityGroup = 0; }
            else if (inputSkill == 33) { abilityGroup = 3; }
            else
            {
                abilityGroup = 2;
            }

            std::cout << "Skill input committed ID=" << inputSkill
                      << " proficiency=" << skillInputBefore->learned_proficiency[abilityGroup] << "->" << u.learned_proficiency[abilityGroup]
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
