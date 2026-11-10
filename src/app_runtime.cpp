#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "app_runtime.hpp"
#include "app_campaign.hpp"
#include "app_options.hpp"
#include "app_sdl.hpp"
#include "app_smoke.hpp"
#include "flic.hpp"
#include "new_game.hpp"
#include "opening_audio.hpp"
#include "original_battle_menu.hpp"
#include "original_battle_reports.hpp"
#include "original_campaign_persistence.hpp"
#include "original_church.hpp"
#include "original_confirmation.hpp"
#include "original_evacuation_smoke.hpp"
#include "original_initialization.hpp"
#include "original_inn.hpp"
#include "original_menu.hpp"
#include "original_promotion_transaction.hpp"
#include "original_save_bridge.hpp"
#include "original_save_menu.hpp"
#include "original_save_store.hpp"
#include "original_settings.hpp"
#include "original_shop_menu.hpp"
#include "original_shop_smoke.hpp"
#include "original_skill94_transaction.hpp"
#include "original_skills.hpp"
#include "original_smoke_exit.hpp"
#include "original_teleport_smoke.hpp"
#include "prologue.hpp"
#include "runtime_diagnostics.hpp"
#include "title_image.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <windows.h>

int s2ne::run_application(int argc, char** argv)
{
    s2ne::configure_runtime_diagnostics();
    s2ne::AppSdlSession sdl;
    auto& window = sdl.window;
    auto& renderer = sdl.renderer;
    auto& texture = sdl.texture;
    {
        const s2ne::AppOptions options(argc, argv);
        s2ne::AppSmokeDriver smokeDriver(options);
        if (options.strengthenedFlow)
        {
            std::cout << "NON-ORIGINAL strengthened flow acceptance: allied HP/attack/defense boosted, MP capacity floored400 and currentMP replenished (NON-ORIGINAL); not balance or legitimate clear evidence\n";
        }
        if (options.campaignContinue)
        {
            std::cout.setf(std::ios::unitbuf);
        }
        std::optional<std::filesystem::path> isolatedSaveRoot;
        struct SaveTestCleanup
        {
            const std::optional<std::filesystem::path>& root;
            ~SaveTestCleanup()
            {
                if (root)
                {
                    std::error_code ignored;
                    std::filesystem::remove_all(*root, ignored);
                }
            }
        } saveTestCleanup{ isolatedSaveRoot };
        if (options.option)
        {
            auto directory = std::filesystem::current_path() / "S2NE" / "out" / "isolated-saves";
            std::filesystem::create_directories(directory);
            auto candidate = directory / (L"s2ne-options.smoke-save-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
            if (!std::filesystem::create_directory(candidate))
            {
                s2ne::fatal("Smoke save directory already exists; refusing to reuse it");
            }
            isolatedSaveRoot = std::move(candidate);
        }
        auto root = options.resource_root(argc, argv);
        std::vector<s2ne::Flic> clips;
        for (unsigned id : { 21u, 22u })
        {
            auto clip = s2ne::Flic::try_from_disc(root, id);
            if (!clip)
            {
                s2ne::report_error(clip.error());
                return EXIT_FAILURE;
            }
            clips.push_back(std::move(clip).value());
        }
        auto titleResult = s2ne::try_load_title(root);
        if (!titleResult)
        {
            s2ne::report_error(titleResult.error());
            return EXIT_FAILURE;
        }
        auto title = std::move(titleResult).value();
        auto menuResult = s2ne::OriginalMenu::create(root);
        if (!menuResult)
        {
            s2ne::report_error(menuResult.error());
            return EXIT_FAILURE;
        }
        auto menu = std::move(menuResult).value();
        auto setupResult = s2ne::NewGameSetup::create(root);
        if (!setupResult)
        {
            s2ne::report_error(setupResult.error());
            return EXIT_FAILURE;
        }
        auto setup = std::move(setupResult).value();
        auto setupCheck = setup.draw(title, menu);
        auto menuCheck = title;
        menu.draw(menuCheck);
        if (options.verify)
        {
            std::cout << "Decoded original title PCX: 640x480\n";
            for (auto& clip : clips)
            {
                std::vector<uint8_t> output;
                unsigned frames = 0;
                while (clip.next(output))
                {
                    ++frames;
                }
                if (frames != clip.frames)
                {
                    s2ne::fatal("Incomplete intro decoding");
                }
                std::cout << "Decoded original intro: " << clip.width << 'x' << clip.height << ", " << frames << " frames, delay " << clip.delay << " ms\n";
            }
            return 0;
        }
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO))
        {
            s2ne::fatal(SDL_GetError());
        }
        s2ne::OpeningAudio audio;
        bool audioAvailable = false;
        auto audioStart = audio.start(root / "MP3" / "SUPERSP_A02.mp3");
        audioAvailable = bool(audioStart);
        if (!audioStart)
        {
            s2ne::report_error(audioStart.error());
            std::cerr << "Audio unavailable; continuing without sound\n";
        }
        if (!SDL_CreateWindowAndRenderer("Northern Envoy - original intro and menu", 960, 720,
                SDL_WINDOW_RESIZABLE, &window, &renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
        SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480);
        if (!texture)
        {
            s2ne::fatal(SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        if (options.teleportSmoke)
        {
            s2ne::original_teleport_smoke(root, renderer, texture);
            audio.stop();

            return 0;
        }
        if (options.evacuationSmoke)
        {
            s2ne::original_evacuation_smoke(root, renderer, texture);
            audio.stop();

            return 0;
        }
        if (options.shopSmoke)
        {
            s2ne::original_shop_smoke(root, renderer, texture);
            audio.stop();

            return 0;
        }
        std::vector<uint8_t> rgba(640 * 480 * 4);
        size_t clip = 0;
        bool running = true, paused = false, inMenu = false, inSetup = false;
        auto showMenu = [&]
        {
            SDL_ShowCursor();
            inMenu = true;
            paused = true;
            auto image = title;
            menu.draw(image);
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        auto showSetup = [&]
        {
            inSetup = true;
            auto image = setup.draw(title, menu);
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        std::unique_ptr<s2ne::Prologue> prologue;
        std::optional<s2ne::OriginalSettings> settings;
        std::optional<s2ne::OriginalBattleMenu> battleMenu;
        std::optional<s2ne::OriginalSaveMenu> saveMenu;
        std::optional<s2ne::OriginalBattleReports> battleReports;
        std::optional<s2ne::OriginalSave::State> rawSave;
        std::optional<s2ne::OriginalShopMenu> shopMenu;
        std::optional<s2ne::OriginalChurch> churchMenu;
        std::optional<s2ne::OriginalInn> innMenu;
        std::optional<s2ne::OriginalPromotion> promotionMenu;
        std::vector<unsigned> usableItems;
        size_t usableItemSelection{ };
        bool itemMenu = false, itemConfirmation = false;
        std::optional<s2ne::OriginalSkills> skillMenu;
        std::optional<size_t> skillTarget;
        std::optional<size_t> pendingSkillTarget;
        bool pendingSkillVisualStarted = false;
        bool skillCastVerified = false;
        s2ne::OriginalSkills skillRules(root);
        s2ne::OriginalSkill94Transaction sword94(root);
        bool autosaveBlocked = false;
        s2ne::Prologue* connectedCampaign = nullptr;
        s2ne::OriginalInitialization initialization(root);
        s2ne::AppCampaignAdapter campaignAdapter(root, prologue, rawSave);
        s2ne::OriginalPromotionTransaction promotionTransaction(root);
        s2ne::OriginalPromotion promotionRules(root);
        s2ne::OriginalMap promotionMap(root);
        std::optional<s2ne::OriginalConfirmation> overwriteConfirmation;
        std::ifstream itemFile(root / "ITEMSP.BIN", std::ios::binary);
        std::vector<uint8_t> itemBytes{ std::istreambuf_iterator<char>(itemFile), { } };
        const size_t saveItems = s2ne::OriginalSave::item_count(itemBytes);
        if (options.campaignContinue && argc > 3)
        {
            auto state = s2ne::OriginalSave::load(std::filesystem::u8path(argv[3]), saveItems);
            auto restoreResult = s2ne::OriginalSaveBridge::try_restore(root, state);
            if (!restoreResult)
            {
                s2ne::report_error(restoreResult.error());
                return EXIT_FAILURE;
            }
            auto restored = std::move(restoreResult).value();
            prologue = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
            prologue->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
            prologue->restore_battle(std::move(restored.events), restored.turn);
            rawSave = std::move(restored.raw);
            paused = true;
            if ((prologue->events.stage == 61 || prologue->events.stage == 131) && prologue->events.flags[12] && s2ne::OriginalCampaign(root).selected_route(prologue->events) == 0 && std::any_of(prologue->events.flags.begin() + 71, prologue->events.flags.end(), [](auto f)
                    {
                        return f != 0;
                    }))
            {
                prologue->restore_terminal_ending();
            }
            s2ne::OriginalSaveStore::write(*isolatedSaveRoot, 52, *rawSave, saveItems);
        }
        if (options.skillInputSmoke)
        {
            if (argc < 4)
            {
                s2ne::fatal("Skill32 input options.smoke requires an unchanged learned save");
            }
            auto state = s2ne::OriginalSave::load(std::filesystem::u8path(argv[3]), saveItems);
            auto restoreResult = s2ne::OriginalSaveBridge::try_restore(root, state);
            if (!restoreResult)
            {
                s2ne::report_error(restoreResult.error());
                return EXIT_FAILURE;
            }
            auto restored = std::move(restoreResult).value();
            prologue = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
            prologue->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
            prologue->restore_battle(std::move(restored.events), restored.turn);
            rawSave = std::move(restored.raw);
            paused = true;
            std::filesystem::create_directories(root / "S2NE" / "out" / options.inputFolder);
            smokeDriver.state.skillSequence.open(root / "S2NE" / "out" / options.inputFolder / "sequence.csv");
            smokeDriver.state.skillSequence << "present,ticks,phase,preparation,spell_frame,hash\n";
            std::cout.setf(std::ios::unitbuf);
            for (unsigned i = 0; i < prologue->events.units.size(); ++i)
            {
                const auto& u = prologue->events.units[i];
                if (u.controllable() && (u.learned[2] & 0x800))
                {
                    std::cout << "Learned candidate index=" << i << " id=" << u.id << " xy=" << u.x << "," << u.y << " MP=" << u.mp << " AP=" << u.action_points << " HP=" << u.hp << " action=" << unsigned(u.action_state) << "\n";
                }
            }
        }
        if (options.promotionSmoke)
        {
            if (argc < 4)
            {
                s2ne::fatal("Promotion options.smoke requires resource root and earned original save path");
            }
            auto state = s2ne::OriginalSave::load(std::filesystem::u8path(argv[3]), saveItems);
            auto restoreResult = s2ne::OriginalSaveBridge::try_restore(root, state);
            if (!restoreResult)
            {
                s2ne::report_error(restoreResult.error());
                return EXIT_FAILURE;
            }
            auto restored = std::move(restoreResult).value();
            prologue = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
            prologue->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
            prologue->restore_battle(std::move(restored.events), restored.turn);
            rawSave = std::move(restored.raw);
            paused = true;
        }
        auto settingsState = s2ne::OriginalSettings::load_state(root);
        menu.small_font = settingsState.small_font != 0;
        auto showSettings = [&]
        {
            auto image = settings->draw(s2ne::load_title(root, 20, false));
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        auto beginSettings = [&]
        {
            settings.emplace(root, 0, settingsState);
            showSettings();
        };
        std::optional<s2ne::OriginalConfirmation> exitConfirmation;
        auto showExitConfirmation = [&]
        {
            auto image = s2ne::load_title(root, 20, false);
            exitConfirmation->draw(image, menu);
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        auto beginExitConfirmation = [&]
        {
            exitConfirmation.emplace(root, menu, 51, 200, 403, true);
            showExitConfirmation();
        };
        bool prologueRedraw = false;
        s2ne::Prologue* presentedPrologue = nullptr;
        auto showPrologue = [&]
        {
            prologueRedraw = true;
        };
        auto renderPrologue = [&]
        {
            prologueRedraw = false;
            if (prologue->drawing_original_mouse() || prologue->computer_actions())
            {
                SDL_HideCursor();
            }
            else
            {
                SDL_ShowCursor();
            }
            if (shopMenu && rawSave)
            {
                shopMenu->tick(s2ne::OriginalShopMenu::wall_clock_cs(), *rawSave);
                if (auto point = shopMenu->take_cursor())
                {
                    float x{ }, y{ };
                    if (SDL_RenderCoordinatesToWindow(renderer, float(point->first), float(point->second), &x, &y))
                    {
                        SDL_WarpMouseInWindow(window, x, y);
                    }
                }
            }
            auto image = innMenu && rawSave ? innMenu->draw(*rawSave) : churchMenu && rawSave ? churchMenu->draw(*rawSave) :
                shopMenu && rawSave                                                           ? shopMenu->draw(*rawSave) :
                                                                                                prologue->draw(menu);
            if (options.skillInputSmoke)
            {
                smokeDriver.state.skillInputImageHash = 1469598103934665603ull;
                for (auto c : image)
                {
                    smokeDriver.state.skillInputImageHash ^= c;
                    smokeDriver.state.skillInputImageHash *= 1099511628211ull;
                }
                int frame = (options.skill94Input ? prologue->skill94_visual_frame() : options.skill88Input ? prologue->skill88_visual_frame() :
                                                                                                              prologue->skill32_visual_frame());
                static int saved = -1;
                if (frame >= 0 && frame != saved)
                {
                    auto* surface = SDL_CreateSurfaceFrom(640, 480, SDL_PIXELFORMAT_RGBA32, image.data(), 640 * 4);
                    if (!surface)
                    {
                        s2ne::fatal(SDL_GetError());
                    }
                    auto file = "S2NE\\out\\" + options.inputFolder + "\\frame-" + std::to_string(frame) + ".bmp";
                    bool ok = SDL_SaveBMP(surface, file.c_str());
                    SDL_DestroySurface(surface);
                    if (!ok)
                    {
                        s2ne::fatal(SDL_GetError());
                    }
                    saved = frame;
                }
            }
            if (!battleMenu && !battleReports && !promotionMenu && !saveMenu && !overwriteConfirmation)
            {
                presentedPrologue = prologue.get();
            }
            if (options.feedbackSmoke && prologue->reaction_draws() > smokeDriver.state.checkedReactionDraws)
            {
                smokeDriver.state.checkedReactionDraws = prologue->reaction_draws();
                const auto& bitmap = prologue->last_reaction_bitmap();
                if (bitmap.empty())
                {
                    s2ne::fatal("Production reaction draw has no indexed bitmap");
                }
                if (bitmap != prologue->expected_reaction_bitmap(prologue->last_reaction_target(), prologue->last_reaction_left()))
                {
                    s2ne::fatal("SDL reaction bitmap disagrees with indexed compositor");
                }
                smokeDriver.state.feedbackReaction = true;
            }
            if (battleMenu)
            {
                SDL_ShowCursor();
                image = battleMenu->draw(image);
            }
            if (battleReports)
            {
                image = battleReports->draw();
            }
            if (promotionMenu)
            {
                SDL_ShowCursor();
                image = promotionMenu->draw(image);
            }
            if (skillMenu && !pendingSkillTarget)
            {
                if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && prologue->battle_selected() >= 0)
                {
                    auto actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                    auto camera = prologue->camera();
                    for (int dy = -4; dy <= 4; ++dy)
                    {
                        for (int dx = -4; dx <= 4; ++dx)
                        {
                            auto marker = actor;
                            marker.x += dx;
                            marker.y += dy;
                            marker.behavior = actor.hostile() ? 0 : 2;
                            marker.hp = 1;
                            marker.status_flags = 0x80;
                            if (!skillMenu->hostile_in_range(actor, marker))
                            {
                                continue;
                            }
                            auto point = prologue->screen_cell({ marker.x, marker.y });
                            if (point.x >= 0 && point.x < 640 && point.y >= 0 && point.y < 380)
                            {
                                auto p = (size_t(point.y) * 640 + point.x) * 4;
                                image[p] = 242;
                                image[p + 1] = 220;
                                image[p + 2] = 64;
                            }
                        }
                    }
                    if (skillTarget)
                    {
                        auto point = prologue->screen_cell({ prologue->events.units.at(*skillTarget).x, prologue->events.units.at(*skillTarget).y });
                        for (int d = -6; d <= 6; ++d)
                        {
                            for (auto xy : { std::pair{ point.x + d, point.y }, std::pair{ point.x, point.y + d } })
                            {
                                if (xy.first >= 0 && xy.first < 640 && xy.second >= 0 && xy.second < 380)
                                {
                                    auto p = (size_t(xy.second) * 640 + xy.first) * 4;
                                    image[p] = 255;
                                    image[p + 1] = 80;
                                    image[p + 2] = 80;
                                }
                            }
                        }
                    }
                }
                skillMenu->draw(image);
            }
            if (itemMenu && !usableItems.empty())
            {
                auto name = promotionTransaction.item_name(usableItems.at(usableItemSelection));
                menu.draw_original_large(image, name, 148, 398, 168);
                static const std::vector<uint8_t> useLabel{ 0xA8, 0xCF, 0xA5, 0xCE };
                menu.draw_original_large(image, useLabel, 148, 438, itemConfirmation ? 242 : 168);
            }
            if (saveMenu)
            {
                saveMenu->draw(image, menu);
            }
            if (overwriteConfirmation)
            {
                overwriteConfirmation->draw(image, menu);
            }
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        auto openSelectedDetail = [&]
        {
            int selected = prologue->battle_selected();
            if (selected < 0 || size_t(selected) >= prologue->events.units.size())
            {
                return;
            }
            const auto& unit = prologue->events.units[size_t(selected)];
            std::vector<s2ne::OriginalBattleReports::Entry> entries{ { unit, unit.hostile(), unit.npc_class() ? 1 : 0, false } };
            battleReports.emplace(root, s2ne::OriginalBattleReports::Page::roster, prologue->events, std::move(entries), prologue->draw(menu), nullptr, settingsState.small_font != 0);
            battleReports->detail();
            battleReports->close_to_battle();
            SDL_ShowCursor();
            showPrologue();
        };
        auto openSelectedPromotion = [&]
        {
            if (!rawSave || !prologue->in_battle() || prologue->computer_actions() || prologue->battle().combat_feedback_active() || prologue->battle().animating_movement())
            {
                return;
            }
            int selected = prologue->battle_selected();
            if (selected < 0 || size_t(selected) >= rawSave->groups[0].units.size() || size_t(selected) >= prologue->events.units.size())
            {
                return;
            }
            if (rawSave->groups[0].inventory.size() != saveItems)
            {
                return;
            }
            const auto& unit = prologue->events.units[size_t(selected)];
            promotionMenu.emplace(root);
            int gameMode = int(rawSave->globals_4f7cd0[0]);
            bool opened = promotionMenu->open(unit, unit.experience_threshold, [&, selected](const auto& request)
                {
                    return campaignAdapter.commit_promotion(selected, request, saveItems, promotionTransaction, promotionMap);
                },
                true, gameMode);
            if (!opened)
            {
                promotionMenu.reset();
            }
            else
            {
                showPrologue();
            }
        };
        auto startGame = [&]
        {
            setup.confirm();
            rawSave.reset();
            autosaveBlocked = false;
            connectedCampaign = nullptr;
            prologue = std::make_unique<s2ne::Prologue>(root, setup.confirmed);
            prologue->set_small_font(settingsState.small_font != 0);
            prologue->enable_text_animation(unsigned(settingsState.message_delay));
            inMenu = false;
            inSetup = false;
            paused = true;
            showPrologue();
            std::cout << "Original prologue stage=" << prologue->events.stage << ", difficulty=" << setup.confirmed << '\n';
        };
        uint64_t next = SDL_GetTicks();
        const uint64_t started = next;
        unsigned presented = 0;
        uint64_t lastUiTick = SDL_GetTicks();
        s2ne::OriginalEdgeScrollTiming edgeScrollTiming;
        while (running)
        {
            uint64_t nowUi = SDL_GetTicks();
            presentedPrologue = nullptr;
            if (prologue && connectedCampaign != prologue.get())
            {
                campaignAdapter.connect();
                connectedCampaign = prologue.get();
            }
            if (prologue)
            {
                if (options.campaignContinue && prologue->battle_turn() != smokeDriver.state.continuousDiagnosticTurn)
                {
                    smokeDriver.state.continuousDiagnosticTurn = prologue->battle_turn();
                    unsigned enemies = 0, allies = 0;
                    for (const auto& u : prologue->events.units)
                    {
                        if (u.present(smokeDriver.state.continuousDiagnosticTurn) && (u.status_flags & 0x80) && !(u.status_flags & 0x20))
                        {
                            if (u.hostile())
                            {
                                ++enemies;
                            }
                            else if (u.controllable())
                            {
                                ++allies;
                            }
                        }
                    }
                    std::cout << "Continuous turn stage=" << prologue->events.stage << " turn=" << smokeDriver.state.continuousDiagnosticTurn << " allies=" << allies << " enemies=" << enemies << '\n';
                    if (prologue->events.stage == 12)
                    {
                        for (const auto& u : prologue->events.units)
                        {
                            if ((u.status_flags & 0x80) && !(u.status_flags & 0x20) && u.hp > 0)
                            {
                                std::cout << " live=" << u.id << " side=" << u.behavior << " hp=" << u.hp << " xy=" << u.x << "," << u.y << " action=" << unsigned(u.action_state) << '\n';
                            }
                        }
                    }
                }
                if (options.campaignSmoke && options.alternateSmoke && prologue->battle_turn() != smokeDriver.state.diagnosticTurn)
                {
                    smokeDriver.state.diagnosticTurn = prologue->battle_turn();
                    std::cout << "Natalia easy auto diagnostic turn=" << smokeDriver.state.diagnosticTurn << " difficulty=" << prologue->events.difficulty << '\n';
                    for (const auto& u : prologue->events.units)
                    {
                        if (!u.hostile())
                        {
                            std::cout << " ally=" << u.id << " behavior=" << u.behavior << " hp=" << u.hp << " ap=" << u.action_points << " action=" << unsigned(u.action_state) << " x=" << u.x << " y=" << u.y << '\n';
                        }
                    }
                }
                if (prologue->computer_actions() && options.battleSmoke)
                {
                    ++smokeDriver.state.hostileActionFrames;
                }
                if (options.autoSmoke && prologue->automatic_actions())
                {
                    ++smokeDriver.state.autoFriendlyFrames;
                }
                if (options.autoSmoke && prologue->hostile_actions())
                {
                    ++smokeDriver.state.autoEnemyFrames;
                }
                if (prologue->battle_intro() && options.battleSmoke && smokeDriver.state.battleSmokeStartTurn)
                {
                    if (prologue->hostile_banner())
                    {
                        ++smokeDriver.state.hostileBannerFrames;
                    }
                    else
                    {
                        ++smokeDriver.state.playerBannerFrames;
                    }
                }
                prologue->set_auto_selection(settingsState.status_window == 1);
                prologue->set_mask_surfaces(settingsState.status_window == 1);
                prologue->set_checker_mask(settingsState.flag_4f7d0a == 1);
                if (options.strengthenedFlow)
                {
                    for (auto& unit : prologue->events.units)
                    {
                        if ((unit.controllable() || unit.behavior == 1) && (unit.status_flags & 0x80) && !(unit.status_flags & 0x20) && unit.hp > 0)
                        {
                            unit.max_hp = 30000;
                            unit.hp = 30000;
                            unit.attack = 30000;
                            unit.defense = 30000;
                            s2ne::smoke_exit::strengthen_mp(unit, true);
                        }
                    }
                }
                if (!battleMenu && !battleReports && !promotionMenu && !itemMenu && (!skillMenu || pendingSkillTarget))
                {
                    prologue->tick(prologue->ending_active() ? unsigned(nowUi - lastUiTick) : options.clearSmoke ? 120u :
                            options.feedbackSmoke                                                                ? std::max(30u, unsigned(nowUi - lastUiTick)) :
                                                                                                                   unsigned(nowUi - lastUiTick));
                }
                if (prologue->ending_active())
                {
                    static bool nataliaTerminalSaved = false;
                    if (options.nataliaStrengthened && !nataliaTerminalSaved && prologue->ending_page() == 0)
                    {
                        nataliaTerminalSaved = true;
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-natalia-terminal.sav");
                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                        std::ofstream out(path, std::ios::binary);
                        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                    }
                    if (!prologue->events.ending_statistics)
                    {
                        auto captured = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto restored = s2ne::OriginalSaveBridge::restore(root, captured);
                        prologue->events = std::move(restored.events);
                        prologue->restore_terminal_ending();
                    }
                    static size_t endingObserved = ~size_t{ };
                    if (endingObserved != prologue->ending_page())
                    {
                        endingObserved = prologue->ending_page();
                        std::cout << "Native ending presentation phase=" << endingObserved + 1 << "/" << prologue->ending_pages() << " " << prologue->ending_description() << '\n';
                    }
                    showPrologue();
                    if (prologue->ending_done())
                    {
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + (options.nataliaStrengthened ? "-natalia-ending-viewed.sav" : "-white-ending-viewed.sav"));
                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                        std::ofstream output(path, std::ios::binary);
                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                        output.close();
                        auto loaded = s2ne::OriginalSave::load(path, saveItems);
                        if (s2ne::OriginalSave::encode(loaded, saveItems) != bytes)
                        {
                            s2ne::fatal("Ending persistent result changed on reload");
                        }
                        std::cout << "Native41E9CF presentation returned after movie3;40D7CB returns to WinMain428F2E cleanup/exit. Native flags42=" << unsigned(prologue->events.flags[41]) << " 43=" << unsigned(prologue->events.flags[42]) << " 73=" << unsigned(prologue->events.flags[72]) << " 75=" << unsigned(prologue->events.flags[74]) << "; no invented completion flag; checkpoint=" << path.string() << '\n';
                        if (options.clearSmoke)
                        {
                            smokeDriver.state.clearVerified = true;
                        }
                        running = false;
                    }
                }
                if (skillMenu && pendingSkillTarget && !prologue->battle().animating_movement() && !pendingSkillVisualStarted)
                {
                    pendingSkillVisualStarted = skillMenu->selected().id == 94 ? prologue->begin_skill94_visual(unsigned(prologue->battle_selected()), unsigned(*pendingSkillTarget)) : skillMenu->selected().id == 88 ? prologue->begin_skill88_visual(unsigned(prologue->battle_selected())) :
                                                                                                                                                                                                                         prologue->begin_skill32_visual(unsigned(prologue->battle_selected()), unsigned(*pendingSkillTarget));
                    if (pendingSkillVisualStarted)
                    {
                        nowUi = SDL_GetTicks();
                    }
                    if (!pendingSkillVisualStarted)
                    {
                        pendingSkillTarget.reset();
                        skillMenu.reset();
                        skillTarget.reset();
                        std::cout << "Original spell32 visual rejected; no effect/payment committed\n";
                    }
                }
                if (skillMenu && pendingSkillTarget && pendingSkillVisualStarted && !prologue->skill_visual_active())
                {
                    pendingSkillVisualStarted = false;
                    int selected = prologue->battle_selected();
                    auto target = *pendingSkillTarget;
                    pendingSkillTarget.reset();
                    s2ne::OriginalSkills::HostileResult result;
                    int pendingMpGrowth = 0;
                    bool swordFeedbackPending = false;
                    bool applied = prologue->battle().mutate_combat_random([&](auto& rng, uint32_t seconds)
                        {
                            s2ne::OriginalSkills::ExecutionContext context;
                            context.difficulty = prologue->events.difficulty;
                            context.seconds = seconds;
                            context.skill_payment_state = skillMenu->initial_payment_state(skillMenu->selected().id);
                            auto award = [&](auto& unit, auto& stream, int xp, int proficiency, unsigned group)
                            {
                                // 4659BD skips capped proficiency, not the remaining XP/payment transaction.
                                promotionTransaction.award_proficiency(unit, 6 + group, unsigned(proficiency));
                                s2ne::OriginalRandom native({ stream.seed_state(), stream.draw_count() });
                                s2ne::OriginalProgression progression(root);
                                int mpBeforeGrowth = unit.mp;
                                if (!progression.award_native(unit, native, xp, promotionRules, [seconds]
                                                    {
                                                        return seconds;
                                                    })
                                        .applied)
                                {
                                    return false;
                                }
                                if (options.skillInputSmoke)
                                {
                                    pendingMpGrowth += unit.mp - mpBeforeGrowth;
                                }
                                auto state = native.state();
                                stream.restore_stream(state.seed, state.calls);
                                return true;
                            };
                            if (skillMenu->selected().id == 88)
                            {
                                auto& actor = prologue->events.units.at(size_t(selected));
                                context.status3c = actor.action_status_runtime ? (uint32_t(actor.action_status_runtime->at(0x3C)) | uint32_t(actor.action_status_runtime->at(0x3D)) << 8 | uint32_t(actor.action_status_runtime->at(0x3E)) << 16 | uint32_t(actor.action_status_runtime->at(0x3F)) << 24) : uint32_t(actor.status3c) | uint32_t(actor.status3d) << 8 | uint32_t(actor.status3e) << 16;
                                std::cout << "88 context difficulty=" << context.difficulty << " runtime3c=" << context.status3c << " cached3c=" << unsigned(actor.status3c) << " entry=" << skillRules.entry_available(actor, s2ne::OriginalSkills::Context{ .second = true }) << " xp=" << actor.experience << " threshold=" << actor.experience_threshold << '\n';
                                auto heal = skillMenu->execute(actor, actor, prologue->events, rng, context, award);
                                result.applied = heal.applied;
                                result.failure = heal.failure;
                                std::cout << "Original88 self heal=" << heal.healing << " xp=" << heal.experience_award << " proficiency=" << heal.proficiency_award << '\n';
                                return heal.applied;
                            }
                            if (skillMenu->selected().id == 94)
                            {
                                auto& actor = prologue->events.units.at(size_t(selected));
                                auto& victim = prologue->events.units.at(target);
                                if (!rawSave || !s2ne::OriginalSkill94Transaction::target_supported(prologue->events, victim))
                                {
                                    return false;
                                }
                                s2ne::OriginalSkill94Transaction::Context sword;
                                sword.original_second_partition = true;
                                sword.target_runtime_known = true;
                                sword.synchronous_death_ready = s2ne::OriginalDeathDispatch::continuation(prologue->events, victim.id).supported;
                                sword.seconds = seconds;
                                sword.payment_state = context.skill_payment_state;
                                auto a = prologue->battle().original_42c88f_bytes(prologue->events, actor), b = prologue->battle().original_42c88f_bytes(prologue->events, victim);
                                sword.hit = { actor.learned_proficiency[9], victim.learned_proficiency[9], actor.fencing, victim.fencing, victim.tactics, int(int8_t(a[1])), int(int8_t(b[2])) };
                                sword.elevation = s2ne::original_combat::terrain_height_difference(prologue->battle().cell({ actor.x, actor.y }).elevation, prologue->battle().cell({ victim.x, victim.y }).elevation, prologue->events.difficulty == 1);
                                auto status = [&](unsigned offset, unsigned cached)
                                {
                                    return victim.action_status_runtime ? unsigned(victim.action_status_runtime->at(offset)) : cached;
                                };
                                sword.damage.difficulty = prologue->events.difficulty;
                                sword.damage.friendly_army = true;
                                sword.damage.target_status39 = status(0x39, victim.status39.value_or(0));
                                sword.damage.target_status3c = status(0x3C, victim.status3c);
                                sword.damage.target_status3d = status(0x3D, victim.status3d);
                                sword.damage.target_status3f = status(0x3F, victim.status3f);
                                sword.damage.target_special13 = prologue->events.equipment_special(victim, 13);
                                auto masks = prologue->events.skill_class_masks(victim);
                                sword.damage.profession_resistance = masks[0];
                                sword.damage.profession_weakness = masks[1];
                                auto stagedActor = actor, stagedVictim = victim;
                                auto stagedInventory = rawSave->groups[0].inventory;
                                auto stagedMenu = *skillMenu;
                                auto stagedRandom = rng;
                                auto outcome = sword94.execute(stagedMenu, stagedActor, stagedVictim, prologue->events, stagedRandom, sword, award, &stagedInventory, &promotionTransaction);
                                if (outcome.applied)
                                {
                                    // The accepted transaction remains unpublished through468BAC's
                                    // numbers. Do not invoke physical settlement/reward a second time.
                                    auto finalActor = std::make_shared<s2ne::OriginalEvents::Unit>(std::move(stagedActor));
                                    auto finalVictim = std::make_shared<s2ne::OriginalEvents::Unit>(std::move(stagedVictim));
                                    auto inventory = std::make_shared<std::vector<uint8_t>>(std::move(stagedInventory));
                                    auto seed = stagedRandom.seed_state(), calls = stagedRandom.draw_count();
                                    swordFeedbackPending = prologue->battle().begin_skill94_feedback(size_t(selected), target, victim.hp, outcome.damage, [&, selected, target, finalActor, finalVictim, inventory, seed, calls]()
                                        {
                                            auto publish = [&, selected, target, finalActor, finalVictim, inventory, seed, calls]()
                                            {
                                                prologue->events.units.at(size_t(selected)) = std::move(*finalActor);
                                                prologue->events.units.at(target) = std::move(*finalVictim);
                                                rawSave->groups[0].inventory = std::move(*inventory);
                                                prologue->battle().mutate_combat_random([&](auto& stream, uint32_t)
                                                    {
                                                        stream.restore_stream(seed, calls);
                                                        return true;
                                                    });
                                                prologue->battle().commit_external_action();
                                                skillCastVerified = true;
                                                *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                                std::cout << "Original94 feedback/death complete; staged HP/reward/ST/AP/inventory/RNG published once\n";
                                            };
                                            if (finalVictim->hp == 0)
                                            {
                                                auto death = [&, target, publish = std::move(publish)]() mutable
                                                {
                                                    if (!prologue->begin_skill94_death(unsigned(target), std::move(publish)))
                                                    {
                                                        s2ne::fatal("Original94 death presentation rejected before settlement");
                                                    }
                                                };
                                                if (!prologue->begin_synchronous_death(finalVictim->id, std::move(death)))
                                                {
                                                    s2ne::fatal("Original94 synchronous death rejected before settlement");
                                                }
                                            }
                                            else
                                            {
                                                publish();
                                            }
                                        });
                                    if (!swordFeedbackPending)
                                    {
                                        s2ne::fatal("94 feedback staging rejected");
                                    }
                                }
                                result.applied = outcome.applied;
                                result.hit = outcome.hit;
                                result.chance = outcome.chance;
                                result.roll = outcome.roll;
                                if (swordFeedbackPending)
                                {
                                    std::cout << "Original94 accepted and suspended for468BAC numbers; no live payment yet\n";
                                }
                                std::cout << "Original94 damage=" << outcome.damage << " xp=" << outcome.raw_xp << " proficiency=" << outcome.proficiency << " runtime39=" << sword.damage.target_status39 << " masks=" << masks[0] << ',' << masks[1] << " failure=" << int(outcome.failure) << '\n';
                                return false;
                            }
                            result = skillMenu->execute_hostile(prologue->events.units.at(size_t(selected)), prologue->events.units.at(target), rng, context, promotionTransaction, award);
                            return result.applied;
                        });
                    std::cout << "Original skill" << skillMenu->selected().id << " target=" << prologue->events.units.at(target).id << " applied=" << applied << " hit=" << result.hit << " immune=" << result.immune << " chance=" << result.chance << " roll=" << result.roll << " removed=" << result.removed << " failure=" << int(result.failure) << '\n';
                    if (applied)
                    {
                        smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                        prologue->battle().commit_external_action();
                        skillCastVerified = true;
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        if (options.campaignContinue && options.strengthenedFlow)
                        {
                            auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-hostile32.sav");
                            auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                            std::ofstream output(path, std::ios::binary);
                            output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                            output.close();
                            auto loaded = s2ne::OriginalSave::load(path, saveItems);
                            auto restored = s2ne::OriginalSaveBridge::restore(root, loaded);
                            if (restored.events.units.at(target).equipment != prologue->events.units.at(target).equipment)
                            {
                                s2ne::fatal("Hostile32 equipment save mismatch");
                            }
                            std::cout << "SDL hostile32 normal input cast save/reload verified checkpoint=" << path.string() << " (not ending)\n";
                        }
                        *rawSave = std::move(snapshot);
                    }
                    skillMenu.reset();
                    skillTarget.reset();
                    showPrologue();
                }
                if (rawSave && prologue->in_battle() && !prologue->battle().animating_movement() && !prologue->battle().combat_feedback_active())
                {
                    for (size_t i = 0; i < prologue->events.units.size(); ++i)
                    {
                        if (auto id = prologue->pickup_at_unit(i, rawSave->groups[0].inventory, rawSave->groups[0].metadata[4]))
                        {
                            prologue->battle().refresh_command_availability(prologue->events.units);
                            std::cout << "Original category12 pickup item " << id << " by unit " << prologue->events.units[i].id << '\n';
                        }
                    }
                }
                float x{ }, y{ }, rx{ }, ry{ };
                SDL_GetMouseState(&x, &y);
                bool edgeActive = !battleMenu && !battleReports && !promotionMenu && !itemMenu && !saveMenu && !overwriteConfirmation && !exitConfirmation && !options.battleSmoke && !options.skillInputSmoke && prologue->in_battle() && !prologue->battle().animating_movement() && (prologue->battle_state() == s2ne::Battle::State::idle || prologue->battle_state() == s2ne::Battle::State::movement || prologue->battle_state() == s2ne::Battle::State::attack_target) && SDL_GetMouseFocus() == window && SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry) && rx >= 0 && rx < 640 && ry >= 0 && ry < 480 && (rx < 10 || rx > 630 || ry < 10 || ry > 470);
                if (edgeScrollTiming.due(nowUi, edgeActive, unsigned(std::max(0, settingsState.screen_delay)) * 10u))
                {
                    prologue->edge_scroll(rx, ry);
                }
                showPrologue();
                if (options.feedbackSmoke)
                {
                    if (auto feedback = prologue->combat_feedback(); feedback && feedback->active())
                    {
                        smokeDriver.state.feedbackFlash |= feedback->silhouette();
                        smokeDriver.state.feedbackNumbers |= feedback->text_visible() && !feedback->miss;
                        smokeDriver.state.feedbackHp |= feedback->hp_after < feedback->hp_before;
                    }
                }
            }
            lastUiTick = nowUi;
            if (smokeDriver.before_events({ root, prologue, rawSave, isolatedSaveRoot, saveItems, window, renderer, audio, audioAvailable, inMenu, inSetup, running, presented, started, menu, setup, battleMenu, battleReports, skillMenu, skillTarget, itemMenu, saveMenu, settingsState, skillRules }))
            {
                continue;
            }
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (prologue && prologue->ending_active())
                {
                    if (event.type == SDL_EVENT_QUIT)
                    {
                        running = false;
                        continue;
                    }
                    if ((event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) || (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                    {
                        auto page = prologue->ending_page();
                        prologue->advance();
                        if (page != prologue->ending_page())
                        {
                            std::cout << "Native ending normal input completed phase=" << page + 1 << "/" << prologue->ending_pages() << '\n';
                        }
                        showPrologue();
                    }
                    continue;
                }
                if (event.type == SDL_EVENT_QUIT)
                {
                    if (prologue && !prologue->original_save_boundary_ready())
                    {
                        continue;
                    }
                    running = false;
                }
                if (innMenu && rawSave)
                {
                    settingsState.difficulty = prologue->events.difficulty;
                    innMenu->prepare_settings(settingsState);
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned code = 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: code = 1; break;
                        case SDLK_UP: code = 0x48; break;
                        case SDLK_DOWN: code = 0x50; break;
                        case SDLK_LEFT: code = 0x4B; break;
                        case SDLK_RIGHT: code = 0x4D; break;
                        case SDLK_TAB: code = 0x0F; break;
                        case SDLK_RETURN: code = 0x1C; break;
                        case SDLK_SPACE: code = 0x39; break;
                        }
                        innMenu->scan(code, *rawSave);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{ }, ry{ };
                        if (SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry))
                        {
                            innMenu->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT, *rawSave);
                        }
                    }
                    if (innMenu->take_equipment_changed())
                    {
                        s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                    }
                    if (auto loaded = innMenu->take_loaded())
                    {
                        auto restored = s2ne::OriginalSaveBridge::restore(root, *loaded);
                        prologue->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
                        prologue->restore_battle(std::move(restored.events), restored.turn);
                        rawSave = std::move(restored.raw);
                        connectedCampaign = nullptr;
                        autosaveBlocked = false;
                        innMenu.reset();
                        showPrologue();
                        continue;
                    }
                    if (auto changed = innMenu->take_settings())
                    {
                        settingsState = *changed;
                        prologue->events.difficulty = settingsState.difficulty;
                        rawSave->globals_4f7cd0[28] = settingsState.difficulty;
                        menu.small_font = settingsState.small_font != 0;
                        prologue->set_small_font(settingsState.small_font != 0);
                    }
                    if (!innMenu->blocker().empty())
                    {
                        std::cout << "Original inn unavailable: " << innMenu->blocker() << '\n';
                    }
                    if (innMenu->closed())
                    {
                        innMenu.reset();
                        prologue->set_interbattle_adapter([](auto request)
                            {
                                return request.option == 1 ? s2ne::OriginalInterbattle::Result::returned : s2ne::OriginalInterbattle::Result::unsupported;
                            });
                        prologue->campaign_scan(0);
                        prologue->set_interbattle_adapter([](auto)
                            {
                                return s2ne::OriginalInterbattle::Result::unsupported;
                            });
                    }
                    showPrologue();
                    continue;
                }
                if (churchMenu && rawSave)
                {
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned code = 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: code = 1; break;
                        case SDLK_LEFT: code = 0x4B; break;
                        case SDLK_RIGHT: code = 0x4D; break;
                        case SDLK_TAB: code = 0x0F; break;
                        case SDLK_UP: code = 0x48; break;
                        case SDLK_DOWN: code = 0x50; break;
                        case SDLK_RETURN: code = 0x1C; break;
                        case SDLK_SPACE: code = 0x39; break;
                        }
                        if (churchMenu->scan(code, *rawSave))
                        {
                            s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{ }, ry{ };
                        if (SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry) && churchMenu->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT, *rawSave))
                        {
                            s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                        }
                    }
                    if (!churchMenu->blocker().empty())
                    {
                        std::cout << "Original church unavailable: " << churchMenu->blocker() << '\n';
                    }
                    if (churchMenu->page() == s2ne::OriginalChurch::Page::closed)
                    {
                        churchMenu.reset();
                        prologue->set_interbattle_adapter([](auto request)
                            {
                                return request.option == 3 ? s2ne::OriginalInterbattle::Result::returned : s2ne::OriginalInterbattle::Result::unsupported;
                            });
                        prologue->campaign_scan(0);
                        prologue->set_interbattle_adapter([](auto)
                            {
                                return s2ne::OriginalInterbattle::Result::unsupported;
                            });
                    }
                    showPrologue();
                    continue;
                }
                if (prologue && (prologue->teleport_active() || prologue->evacuation_active()))
                {
                    showPrologue();
                    continue;
                }
                if (shopMenu && rawSave)
                {
                    shopMenu->clock(s2ne::OriginalShopMenu::wall_clock_cs());
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                    {
                        shopMenu->release_scroll();
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned code = 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: code = 1; break;
                        case SDLK_UP: code = 0x48; break;
                        case SDLK_DOWN: code = 0x50; break;
                        case SDLK_LEFT: code = 0x4B; break;
                        case SDLK_RIGHT: code = 0x4D; break;
                        case SDLK_TAB: code = 0x0F; break;
                        case SDLK_RETURN: code = 0x1C; break;
                        case SDLK_SPACE: code = 0x39; break;
                        }
                        if (shopMenu->scan(code, *rawSave))
                        {
                            s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{ }, ry{ };
                        if (SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry) && shopMenu->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT, *rawSave))
                        {
                            s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                        }
                    }
                    if (shopMenu->page() == s2ne::OriginalShopMenu::Page::closed)
                    {
                        shopMenu.reset();
                        prologue->set_interbattle_adapter([](auto request)
                            {
                                return request.option == 2 ? s2ne::OriginalInterbattle::Result::returned : s2ne::OriginalInterbattle::Result::unsupported;
                            });
                        prologue->campaign_scan(0);
                        prologue->set_interbattle_adapter([](auto)
                            {
                                return s2ne::OriginalInterbattle::Result::unsupported;
                            });
                    }
                    showPrologue();
                    continue;
                }
                if (prologue && prologue->campaign_modal())
                {
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned scan = 0;
                        switch (event.key.key)
                        {
                        case SDLK_UP: scan = 0x48; break;
                        case SDLK_DOWN: scan = 0x50; break;
                        case SDLK_LEFT: scan = 0x4B; break;
                        case SDLK_RIGHT: scan = 0x4D; break;
                        case SDLK_TAB: scan = 0x0F; break;
                        case SDLK_ESCAPE: scan = 1; break;
                        case SDLK_RETURN: scan = 0x1C; break;
                        case SDLK_SPACE: scan = 0x39; break;
                        }
                        if (prologue->interbattle_active())
                        {
                            prologue->campaign_scan(scan);
                        }
                        else if (scan == 0x1C || scan == 0x39)
                        {
                            prologue->advance();
                        }
                        else if (event.key.key == SDLK_LALT || event.key.key == SDLK_RALT)
                        {
                            prologue->reveal_text();
                        }
                    }
                    else if (prologue->interbattle_active() && (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                        float y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                        float rx{ }, ry{ };
                        if (SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry))
                        {
                            prologue->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT && !prologue->interbattle_active())
                    {
                        prologue->advance();
                    }
                    showPrologue();
                    continue;
                }
                if (skillMenu && prologue && rawSave)
                {
                    if (pendingSkillTarget)
                    {
                        continue;
                    }
                    int selected = prologue->battle_selected();
                    bool confirm = false;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        if (event.key.key == SDLK_ESCAPE)
                        {
                            skillMenu.reset();
                            skillTarget.reset();
                            showPrologue();
                            continue;
                        }
                        if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                        {
                            unsigned scan = event.key.key == SDLK_LEFT ? 0x4B : event.key.key == SDLK_RIGHT ? 0x4D :
                                event.key.key == SDLK_UP                                                    ? 0x48 :
                                event.key.key == SDLK_DOWN                                                  ? 0x50 :
                                                                                                              0;
                            if (scan)
                            {
                                skillMenu->navigate(scan);
                            }
                        }
                        if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                        {
                            confirm = true;
                        }
                        if (event.key.key == SDLK_TAB && skillMenu->selected().id != 88 && skillMenu->page() == s2ne::OriginalSkills::Page::requested)
                        {
                            for (size_t n = 1; n <= prologue->events.units.size(); ++n)
                            {
                                size_t i = (skillTarget.value_or(0) + n) % prologue->events.units.size();
                                if (skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i]))
                                {
                                    skillTarget = i;
                                    break;
                                }
                            }
                        }
                    }
                    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool click = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        if (click && event.button.button == SDL_BUTTON_RIGHT)
                        {
                            skillMenu.reset();
                            skillTarget.reset();
                            showPrologue();
                            continue;
                        }
                        int x = int(click ? event.button.x : event.motion.x), y = int(click ? event.button.y : event.motion.y);
                        if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                        {
                            confirm = skillMenu->mouse(x, y, click && event.button.button == SDL_BUTTON_LEFT);
                        }
                        else
                        {
                            auto camera = prologue->camera();
                            auto cell = prologue->battle().pick_surface(float(x), float(y), camera.first, camera.second);
                            int target = prologue->battle().occupant(prologue->events.units, cell);
                            if (target >= 0 && ((skillMenu->selected().id == 88 && target == selected) || skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[size_t(target)])))
                            {
                                skillTarget = size_t(target);
                                confirm = click && event.button.button == SDL_BUTTON_LEFT;
                            }
                        }
                    }
                    if (confirm && selected >= 0)
                    {
                        if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                        {
                            auto failure = skillMenu->select(prologue->events.units.at(size_t(selected)));
                            if (failure != s2ne::OriginalSkills::Failure::none)
                            {
                                std::cout << "Original spell selection rejected reason=" << int(failure) << '\n';
                                if (options.campaignContinue && options.strengthenedFlow)
                                {
                                    auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                    auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-cast-blocked.sav");
                                    auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                                    std::ofstream output(path, std::ios::binary);
                                    output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                                    output.close();
                                    const auto& actor = prologue->events.units.at(size_t(selected));
                                    s2ne::fatal("Final quest normal-input cast blocked: reason=" + std::to_string(int(failure)) + " MP=" + std::to_string(actor.mp) + " ST=" + std::to_string(actor.st) + " checkpoint=" + path.string());
                                }
                            }
                            else if (!skillMenu->hostile_supported() && !skillMenu->sword94_supported() && !skillMenu->effect_supported(skillMenu->selected()))
                            {
                                std::cout << "Original selected spell executor unavailable id=" << skillMenu->selected().id << '\n';
                                skillMenu->resume();
                            }
                            else if (skillMenu->selected().id == 88)
                            {
                                skillTarget = size_t(selected);
                            }
                            else
                            {
                                skillTarget.reset();
                                for (size_t i = 0; i < prologue->events.units.size(); ++i)
                                {
                                    if (skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i]))
                                    {
                                        skillTarget = i;
                                        break;
                                    }
                                }
                            }
                        }
                        else if (skillTarget)
                        {
                            const auto& caster = prologue->events.units.at(size_t(selected));
                            const auto& target = prologue->events.units.at(*skillTarget);
                            if (skillMenu->selected().id == 94 && !s2ne::OriginalSkill94Transaction::target_supported(prologue->events, target))
                            {
                                std::cout << "Original94 unsupported death/runtime target rejected before preparation/payment\n";
                                continue;
                            }
                            if (!prologue->battle().target_visible(target, std::span(&caster, 1), 0))
                            {
                                std::cout << "Original spell target hidden\n";
                                continue;
                            }
                            prologue->battle().set_hostile_resources(prologue->events);
                            if (!prologue->battle().present_skill32_preparation(prologue->events.units, size_t(selected), *skillTarget, (skillMenu->selected().id == 88 || skillMenu->selected().id == 94)))
                            {
                                std::cout << "Original46C078 spell32 preparation unavailable\n";
                                continue;
                            }
                            pendingSkillTarget = skillTarget;
                            skillTarget.reset();
                        }
                    }
                    showPrologue();
                    continue;
                }
                if (itemMenu && prologue && rawSave)
                {
                    bool confirm = false;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        if (event.key.key == SDLK_ESCAPE)
                        {
                            itemMenu = false;
                            itemConfirmation = false;
                        }
                        else if (event.key.key == SDLK_UP || event.key.key == SDLK_LEFT)
                        {
                            usableItemSelection = (usableItemSelection + usableItems.size() - 1) % usableItems.size();
                            itemConfirmation = false;
                        }
                        else if (event.key.key == SDLK_DOWN || event.key.key == SDLK_RIGHT)
                        {
                            usableItemSelection = (usableItemSelection + 1) % usableItems.size();
                            itemConfirmation = false;
                        }
                        else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                        {
                            confirm = true;
                        }
                    }
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        if (event.button.button == SDL_BUTTON_RIGHT)
                        {
                            itemMenu = false;
                            itemConfirmation = false;
                        }
                        else if (event.button.button == SDL_BUTTON_LEFT && event.button.x >= 148 && event.button.x < 500 && event.button.y >= 394 && event.button.y < 470)
                        {
                            confirm = true;
                        }
                    }
                    if (confirm)
                    {
                        if (!itemConfirmation)
                        {
                            itemConfirmation = true;
                        }
                        else
                        {
                            int selected = prologue->battle_selected();
                            if (selected >= 0 && prologue->in_battle())
                            {
                                auto& unit = prologue->events.units.at(size_t(selected));
                                s2ne::OriginalPromotionTransaction::State candidate{ unit, rawSave->groups[0].inventory, s2ne::OriginalRandom({ 1, 0 }) };
                                if (promotionTransaction.use_learning_item(candidate, usableItems.at(usableItemSelection)))
                                {
                                    unit = std::move(candidate.unit);
                                    rawSave->groups[0].inventory = std::move(candidate.inventory);
                                    prologue->battle().commit_external_action();
                                    std::cout << "Original USE learning item " << usableItems.at(usableItemSelection) << " committed for unit " << unit.id << '\n';
                                    if (options.campaignContinue && options.strengthenedFlow && (prologue->events.stage == 61 || prologue->events.stage == 131) && (unit.learned[2] & 0x800))
                                    {
                                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-mineral-learned.sav");
                                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                                        std::ofstream output(path, std::ios::binary);
                                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                                        output.close();
                                        if (!output)
                                        {
                                            s2ne::fatal("Cannot persist learned mineral checkpoint");
                                        }
                                        auto loaded = s2ne::OriginalSave::load(path, saveItems);
                                        if (s2ne::OriginalSave::encode(loaded, saveItems) != bytes)
                                        {
                                            s2ne::fatal("Mineral learned checkpoint roundtrip failed");
                                        }
                                        auto restored = s2ne::OriginalSaveBridge::restore(root, loaded);
                                        if (!(restored.events.units.at(size_t(selected)).learned[2] & 0x800))
                                        {
                                            s2ne::fatal("Learned spell lost after original restore");
                                        }
                                        std::cout << "SDL final quest partial: original pickup301 USE65 learned32 persisted and reloaded at turn=" << prologue->battle_turn() << " checkpoint=" << path.string() << "; continuing normal-input hostile32 quest, not an ending\n";
                                        smokeDriver.state.mineralQuestVerified = true;
                                    }
                                }
                            }
                            itemMenu = false;
                            itemConfirmation = false;
                        }
                    }
                    showPrologue();
                    continue;
                }
                if (settings)
                {
                    s2ne::OriginalSettings::Event input;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        input.type = 1;
                        input.ascii = event.key.key < 128 ? unsigned(event.key.key) : 0;
                        if (event.key.key == SDLK_ESCAPE)
                        {
                            input.scan = 1;
                        }
                        else if (event.key.key == SDLK_O)
                        {
                            input.scan = 0x18;
                        }
                        else if (event.key.key == SDLK_C)
                        {
                            input.scan = 0x2e;
                        }
                        if (event.key.mod & SDL_KMOD_LCTRL)
                        {
                            input.modifiers |= 0x20;
                        }
                        if (event.key.mod & SDL_KMOD_RCTRL)
                        {
                            input.modifiers |= 0x40;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        input.type = 0x20;
                        input.x = int(event.button.x);
                        input.y = int(event.button.y);
                        input.buttons = event.button.button == SDL_BUTTON_LEFT ? 1 : event.button.button == SDL_BUTTON_RIGHT ? 2 :
                                                                                                                               0;
                    }
                    auto result = settings->event(input);
                    if (result.closed)
                    {
                        settingsState = settings->state();
                        menu.small_font = settingsState.small_font != 0;
                        if (result.save_config)
                        {
                            std::map<std::string, std::string> replacements;
                            std::istringstream updated(settings->config_text());
                            std::string line;
                            while (std::getline(updated, line))
                            {
                                auto eq = line.find('=');
                                if (eq != std::string::npos && line.substr(0, eq) != "Version")
                                {
                                    replacements[line.substr(0, eq)] = line;
                                }
                            }
                            std::ifstream existing(root / "SUPERSP.CFG", std::ios::binary);
                            std::string merged;
                            while (std::getline(existing, line))
                            {
                                if (!line.empty() && line.back() == '\r')
                                {
                                    line.pop_back();
                                }
                                auto key = line.substr(0, line.find('='));
                                auto replacement = replacements.find(key);
                                if (replacement != replacements.end())
                                {
                                    line = replacement->second;
                                    replacements.erase(replacement);
                                }
                                merged += line + "\r\n";
                            }
                            existing.close();
                            for (const auto& [key, value] : replacements)
                            {
                                merged += value + "\r\n";
                            }
                            std::ofstream config(root / "SUPERSP.CFG", std::ios::binary);
                            if (!config)
                            {
                                s2ne::fatal("Cannot save original settings");
                            }
                            config << merged;
                            if (!config)
                            {
                                s2ne::fatal("Original settings write failed");
                            }
                        }
                        settings.reset();
                        showMenu();
                    }
                    else
                    {
                        showSettings();
                    }
                    continue;
                }
                if (exitConfirmation)
                {
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        if (event.key.key == SDLK_ESCAPE)
                        {
                            exitConfirmation->cancel();
                        }
                        else if (event.key.key == SDLK_LEFT || event.key.key == SDLK_RIGHT || event.key.key == SDLK_TAB)
                        {
                            exitConfirmation->toggle();
                        }
                        else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                        {
                            exitConfirmation->confirm();
                        }
                    }
                    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        if (down && event.button.button == SDL_BUTTON_RIGHT)
                        {
                            exitConfirmation->cancel();
                        }
                        else
                        {
                            exitConfirmation->mouse(down ? event.button.x : event.motion.x, down ? event.button.y : event.motion.y, down && event.button.button == SDL_BUTTON_LEFT);
                        }
                    }
                    if (auto result = exitConfirmation->result(); result)
                    {
                        if (*result)
                        {
                            running = false;
                        }
                        exitConfirmation.reset();
                        showMenu();
                    }
                    else
                    {
                        showExitConfirmation();
                    }
                    continue;
                }
                if (saveMenu && !battleMenu)
                {
                    s2ne::OriginalSaveMenu::Event input;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        input.type = 1;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: input.scan = 1; break;
                        case SDLK_UP: input.scan = 0x48; break;
                        case SDLK_DOWN: input.scan = 0x50; break;
                        case SDLK_LEFT: input.scan = 0x4b; break;
                        case SDLK_RIGHT: input.scan = 0x4d; break;
                        case SDLK_RETURN: input.scan = 0x1c; break;
                        case SDLK_SPACE: input.scan = 0x39; break;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        input.type = down ? 0x20 : 8;
                        input.x = int(down ? event.button.x : event.motion.x);
                        input.y = int(down ? event.button.y : event.motion.y);
                        input.buttons = down ? (event.button.button == SDL_BUTTON_LEFT ? 1 : event.button.button == SDL_BUTTON_RIGHT ? 2 :
                                                                                                                                       0) :
                                               0;
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN)
                    {
                        input.ascii = 13;
                    }
                    auto result = saveMenu->event(input);
                    if (result.action == s2ne::OriginalSaveMenu::Action::load)
                    {
                        auto loadResult = s2ne::OriginalSave::try_load(s2ne::OriginalSave::slot_path(root, result.slot), saveItems);
                        if (loadResult)
                        {
                            auto state = std::move(loadResult).value();
                            auto restoreResult = s2ne::OriginalSaveBridge::try_restore(root, state);
                            if (!restoreResult)
                            {
                                s2ne::report_error(restoreResult.error());
                                saveMenu->complete(false);
                                continue;
                            }
                            auto restored = std::move(restoreResult).value();
                            auto loaded = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
                            loaded->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
                            loaded->restore_battle(std::move(restored.events), restored.turn);
                            loaded->set_small_font(settingsState.small_font != 0);
                            prologue = std::move(loaded);
                            rawSave = std::move(restored.raw);
                            connectedCampaign = nullptr;
                            autosaveBlocked = false;
                            saveMenu->complete(true);
                            inMenu = false;
                            inSetup = false;
                            showPrologue();
                        }
                        else
                        {
                            saveMenu->complete(false);
                            s2ne::report_error(loadResult.error());
                        }
                    }
                    if (saveMenu->closed())
                    {
                        saveMenu.reset();
                        if (!prologue)
                        {
                            showMenu();
                        }
                    }
                    continue;
                }
                if (battleReports && !battleMenu)
                {
                    s2ne::OriginalSettings::Event input;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        input.type = 1;
                        input.ascii = event.key.key < 128 ? unsigned(event.key.key) : 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: input.scan = 1; break;
                        case SDLK_UP: input.scan = 0x48; break;
                        case SDLK_DOWN: input.scan = 0x50; break;
                        case SDLK_LEFT: input.scan = 0x4b; break;
                        case SDLK_RIGHT: input.scan = 0x4d; break;
                        case SDLK_RETURN: input.scan = 0x1c; break;
                        case SDLK_SPACE: input.scan = 0x39; break;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        input.type = down ? 0x20 : 8;
                        input.x = int(down ? event.button.x : event.motion.x);
                        input.y = int(down ? event.button.y : event.motion.y);
                        input.buttons = down ? (event.button.button == SDL_BUTTON_LEFT ? 1 : event.button.button == SDL_BUTTON_RIGHT ? 2 :
                                                                                                                                       0) :
                                               0;
                    }
                    battleReports->event(input);
                    if (battleReports->page() == s2ne::OriginalBattleReports::Page::closed)
                    {
                        battleReports.reset();
                    }
                    showPrologue();
                    continue;
                }
                if (promotionMenu)
                {
                    s2ne::OriginalSettings::Event input;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        input.type = 1;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: input.scan = 1; break;
                        case SDLK_UP: input.scan = 0x48; break;
                        case SDLK_DOWN: input.scan = 0x50; break;
                        case SDLK_LEFT: input.scan = 0x4B; break;
                        case SDLK_RIGHT: input.scan = 0x4D; break;
                        case SDLK_TAB: input.scan = 0x0F; break;
                        case SDLK_RETURN: input.scan = 0x1C; break;
                        case SDLK_SPACE: input.scan = 0x39; break;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        input.type = down ? 0x20 : 8;
                        input.x = int(down ? event.button.x : event.motion.x);
                        input.y = int(down ? event.button.y : event.motion.y);
                        input.buttons = down ? (event.button.button == SDL_BUTTON_LEFT ? 1 : event.button.button == SDL_BUTTON_RIGHT ? 2 :
                                                                                                                                       0) :
                                               0;
                    }
                    promotionMenu->event(input);
                    if (promotionMenu->page() == s2ne::OriginalPromotion::Page::closed)
                    {
                        promotionMenu.reset();
                    }
                    showPrologue();
                    continue;
                }
                if (battleMenu)
                {
                    s2ne::OriginalSettings::Event input;
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        input.type = 1;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: input.scan = 1; break;
                        case SDLK_UP: input.scan = 0x48; break;
                        case SDLK_DOWN: input.scan = 0x50; break;
                        case SDLK_LEFT: input.scan = 0x4b; break;
                        case SDLK_RIGHT: input.scan = 0x4d; break;
                        case SDLK_RETURN: input.scan = 0x1c; break;
                        case SDLK_SPACE: input.scan = 0x39; break;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        input.type = down ? 0x20 : 8;
                        input.x = int(down ? event.button.x : event.motion.x);
                        input.y = int(down ? event.button.y : event.motion.y);
                        input.buttons = down ? (event.button.button == SDL_BUTTON_LEFT ? 1 : event.button.button == SDL_BUTTON_RIGHT ? 2 :
                                                                                                                                       0) :
                                               0;
                    }
                    if (saveMenu)
                    {
                        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN)
                        {
                            input.ascii = 13;
                        }
                        s2ne::OriginalSaveMenu::Result result;
                        if (overwriteConfirmation)
                        {
                            if (input.scan == 1 || (input.buttons & 2))
                            {
                                overwriteConfirmation->cancel();
                            }
                            else if (input.scan == 0x4b || input.scan == 0x4d)
                            {
                                overwriteConfirmation->toggle();
                            }
                            else if (input.scan == 0x1c || input.scan == 0x39)
                            {
                                overwriteConfirmation->confirm();
                            }
                            else if (input.type == 8 || input.type == 0x20)
                            {
                                overwriteConfirmation->mouse(float(input.x), float(input.y), (input.buttons & 1) != 0);
                            }
                            if (auto yes = overwriteConfirmation->result())
                            {
                                result = saveMenu->confirm_overwrite(*yes);
                                overwriteConfirmation.reset();
                            }
                        }
                        else
                        {
                            result = saveMenu->event({ input.type, input.x, input.y, input.buttons, input.ascii, input.scan });
                        }
                        if (result.action == s2ne::OriginalSaveMenu::Action::confirm_overwrite)
                        {
                            overwriteConfirmation.emplace(root, menu, 253, 176, 76 + 30 * saveMenu->row(), true);
                        }
                        if (result.action == s2ne::OriginalSaveMenu::Action::load || result.action == s2ne::OriginalSaveMenu::Action::save)
                        {
                            auto saveResult = [&]() -> s2ne::Result<bool>
                            {
                                if (!prologue->original_save_boundary_ready())
                                {
                                    return s2ne::Result<bool>::failure(s2ne::make_error("Original synchronous settlement in progress"));
                                }
                                auto path = s2ne::OriginalSave::slot_path(root, result.slot);
                                if (result.action == s2ne::OriginalSaveMenu::Action::load)
                                {
                                    auto loaded = s2ne::OriginalSave::try_load(path, saveItems);
                                    if (!loaded)
                                    {
                                        return s2ne::Result<bool>::failure(loaded.error());
                                    }
                                    auto state = std::move(loaded).value();
                                    auto restoreResult = s2ne::OriginalSaveBridge::try_restore(root, state);
                                    if (!restoreResult)
                                    {
                                        return s2ne::Result<bool>::failure(restoreResult.error());
                                    }
                                    auto restored = std::move(restoreResult).value();
                                    prologue->set_resource_domain(s2ne::app_domain_for_save(restored.raw));
                                    prologue->restore_battle(std::move(restored.events), restored.turn);
                                    rawSave = std::move(restored.raw);
                                    connectedCampaign = nullptr;
                                    autosaveBlocked = false;
                                }
                                else
                                {
                                    auto captured = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), rawSave ? &*rawSave : nullptr);
                                    if (!captured)
                                    {
                                        return s2ne::Result<bool>::failure(captured.error());
                                    }
                                    auto state = std::move(captured).value();
                                    auto written = s2ne::OriginalSaveStore::try_write(isolatedSaveRoot ? *isolatedSaveRoot : root, result.slot, state, saveItems);
                                    if (!written)
                                    {
                                        return written;
                                    }
                                    rawSave = std::move(state);
                                }
                                return s2ne::Result<bool>::success(true);
                            }();
                            saveMenu->complete(bool(saveResult));
                            if (!saveResult)
                            {
                                s2ne::report_error(saveResult.error());
                            }
                        }
                        if (saveMenu->closed())
                        {
                            int code = saveMenu->return_code();
                            saveMenu.reset();
                            battleMenu->complete_operation(code);
                        }
                    }
                    else if (battleReports)
                    {
                        battleReports->event(input);
                        if (battleReports->page() == s2ne::OriginalBattleReports::Page::closed)
                        {
                            if (auto location = battleReports->location())
                            {
                                auto point = prologue->screen_cell({ location->first, location->second });
                                prologue->pan(point.x - 320, point.y - 240);
                            }
                            int code = battleReports->return_code();
                            battleReports.reset();
                            battleMenu->complete_operation(code);
                        }
                    }
                    else
                    {
                        battleMenu->event(input);
                    }
                    auto request = battleMenu->request();
                    if (!battleReports && (request.action == s2ne::OriginalBattleMenu::Action::conditions || request.action == s2ne::OriginalBattleMenu::Action::inspect_characters || request.action == s2ne::OriginalBattleMenu::Action::inspect_map))
                    {
                        std::vector<s2ne::OriginalBattleReports::Entry> entries;
                        for (const auto& unit : prologue->events.units)
                        {
                            if (unit.present(prologue->battle_turn()))
                            {
                                entries.push_back({ unit, unit.hostile(), unit.npc_class() ? 1 : 0, false });
                            }
                        }
                        auto page = request.action == s2ne::OriginalBattleMenu::Action::conditions ? s2ne::OriginalBattleReports::Page::conditions : request.action == s2ne::OriginalBattleMenu::Action::inspect_map ? s2ne::OriginalBattleReports::Page::map :
                                                                                                                                                                                                                       s2ne::OriginalBattleReports::Page::roster;
                        s2ne::OriginalMap overview(root);
                        overview.set_dynamic_object_chains(prologue->events.dynamic_object_chains);
                        overview.set_event_flags(prologue->events.flags);
                        overview.load(prologue->events.map_id);
                        battleReports.emplace(root, page, prologue->events, std::move(entries), prologue->draw(menu), &overview, settingsState.small_font != 0);
                    }
                    if (!saveMenu && (request.action == s2ne::OriginalBattleMenu::Action::load || request.action == s2ne::OriginalBattleMenu::Action::save)) { saveMenu.emplace(root, request.action == s2ne::OriginalBattleMenu::Action::save ? 1 : 0, s2ne::OriginalSaveMenu::scan(root)); }
                    if (auto code = battleMenu->return_code())
                    {
                        settingsState = battleMenu->settings_state();
                        prologue->set_small_font(settingsState.small_font != 0);
                        battleMenu.reset();
                        if (*code == 3)
                        {
                            prologue->end_battle_turn();
                        }
                        else if (*code == 6)
                        {
                            prologue->start_auto_battle();
                        }
                        else if (*code == 11)
                        {
                            running = false;
                        }
                    }
                    showPrologue();
                    continue;
                }
                if (prologue)
                {
                    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool click = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT;
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                        float y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                        if (prologue->in_battle() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT && prologue->battle_state() == s2ne::Battle::State::idle)
                        {
                            auto slots = s2ne::OriginalSaveMenu::scan(root);
                            s2ne::OriginalBattleMenu::Context context;
                            context.saves_present = std::any_of(slots.begin(), slots.end(), [](const auto& slot)
                                {
                                    return slot.occupied();
                                });
                            battleMenu.emplace(root, context, settingsState);
                            showPrologue();
                            continue;
                        }
                        if (options.skill94Input && click)
                        {
                            auto [cx, cy] = prologue->camera();
                            auto p = prologue->battle().pick_surface(x, y, cx, cy);
                            std::cout << "94 mouse delivered=" << x << ',' << y << " picked=" << p.x << ',' << p.y << " camera=" << cx << ',' << cy << " occupant=" << prologue->battle().occupant(prologue->events.units, p) << '\n';
                        }
                        if (prologue->choosing() || prologue->in_battle())
                        {
                            prologue->mouse(x, y, click);
                        }
                        else if (click)
                        {
                            prologue->advance();
                        }
                        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT)
                        {
                            prologue->reveal_text();
                        }
                        if (prologue->in_battle() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT)
                        {
                            prologue->cancel_battle_selection();
                        }
                        showPrologue();
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        switch (event.key.key)
                        {
                        case SDLK_UP: prologue->move(-1); break;
                        case SDLK_DOWN: prologue->move(1); break;
                        case SDLK_LEFT:
                            if (prologue->in_battle() && (prologue->battle_state() == s2ne::Battle::State::command || prologue->battle_state() == s2ne::Battle::State::attack_menu || prologue->battle_state() == s2ne::Battle::State::finish_confirm))
                            {
                                prologue->battle_cycle(-1);
                            }
                            else
                            {
                                prologue->pan(-32, 0);
                            }
                            break;
                        case SDLK_RIGHT:
                            if (prologue->in_battle() && (prologue->battle_state() == s2ne::Battle::State::command || prologue->battle_state() == s2ne::Battle::State::attack_menu || prologue->battle_state() == s2ne::Battle::State::finish_confirm))
                            {
                                prologue->battle_cycle(1);
                            }
                            else
                            {
                                prologue->pan(32, 0);
                            }
                            break;
                        case SDLK_ESCAPE: prologue->cancel_battle_selection(); break;
                        case SDLK_RETURN:
                            if (prologue->in_battle())
                            {
                                prologue->battle_confirm();
                            }
                            else
                            {
                                prologue->advance();
                            }
                            break;
                        case SDLK_SPACE:
                            if (prologue->in_battle())
                            {
                                prologue->battle_confirm();
                            }
                            else
                            {
                                prologue->advance();
                            }
                            break;
                        case SDLK_LALT:
                        case SDLK_RALT: prologue->reveal_text(); break;
                        case SDLK_TAB:
                            if (prologue->in_battle())
                            {
                                prologue->battle_cycle_target();
                            }
                            break;
                        case SDLK_HOME: prologue->finish_facing(3); break;
                        case SDLK_PAGEUP: prologue->finish_facing(2); break;
                        case SDLK_END: prologue->finish_facing(0); break;
                        case SDLK_PAGEDOWN: prologue->finish_facing(1); break;
                        case SDLK_F10:
                            if (prologue->in_battle())
                            {
                                prologue->end_battle_turn();
                            }
                            break;
                        default: break;
                        }
                        showPrologue();
                    }
                    if (prologue->in_battle())
                    {
                        SDL_SetWindowTitle(window, ("Battle subset - " + prologue->battle_status()).c_str());
                    }
                    continue;
                }
                if (inSetup && (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                {
                    SDL_ConvertEventToRenderCoordinates(renderer, &event);
                    bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                    float x = down ? event.button.x : event.motion.x, y = down ? event.button.y : event.motion.y;
                    if (down && event.button.button == SDL_BUTTON_RIGHT)
                    {
                        inSetup = false;
                        showMenu();
                        continue;
                    }
                    bool confirm = setup.mouse(x, y, down && event.button.button == SDL_BUTTON_LEFT);
                    if (confirm)
                    {
                        startGame();
                    }
                    else if (down)
                    {
                        showSetup();
                    }
                    continue;
                }
                if (inMenu && !inSetup && (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                {
                    SDL_ConvertEventToRenderCoordinates(renderer, &event);
                    bool click = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT;
                    float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                    float y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                    if (menu.mouse(x, y, click))
                    {
                        auto target = menu.activate();
                        if (menu.selected == 0)
                        {
                            showSetup();
                            continue;
                        }
                        if (menu.selected == 1)
                        {
                            saveMenu.emplace(root, 0, s2ne::OriginalSaveMenu::scan(root));
                            continue;
                        }
                        if (menu.selected == 4)
                        {
                            beginExitConfirmation();
                            continue;
                        }
                        if (menu.selected == 3)
                        {
                            beginSettings();
                            continue;
                        }
                        std::cout << "Selected original menu row " << menu.selected << ", handler 0x" << std::hex << target << std::dec << " (not implemented)\n";
                    }
                    showMenu();
                }
                if (!inMenu && !prologue && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT)
                {
                    showMenu();
                    continue;
                }
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                {
                    if (inSetup)
                    {
                        if (event.key.key == SDLK_ESCAPE)
                        {
                            inSetup = false;
                            showMenu();
                        }
                        else if (event.key.key == SDLK_UP)
                        {
                            setup.move(-1);
                            showSetup();
                        }
                        else if (event.key.key == SDLK_DOWN)
                        {
                            setup.move(1);
                            showSetup();
                        }
                        else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { startGame(); }
                        continue;
                    }
                    if (event.key.key == SDLK_ESCAPE && !inMenu)
                    {
                        showMenu();
                    }
                    if (inMenu && event.key.key == SDLK_UP)
                    {
                        menu.move(-1);
                        showMenu();
                    }
                    if (inMenu && event.key.key == SDLK_DOWN)
                    {
                        menu.move(1);
                        showMenu();
                    }
                    if (inMenu && (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE))
                    {
                        auto target = menu.activate();
                        if (menu.selected == 0)
                        {
                            showSetup();
                            continue;
                        }
                        if (menu.selected == 1)
                        {
                            saveMenu.emplace(root, 0, s2ne::OriginalSaveMenu::scan(root));
                            continue;
                        }
                        if (menu.selected == 4)
                        {
                            beginExitConfirmation();
                            continue;
                        }
                        if (menu.selected == 3)
                        {
                            beginSettings();
                            continue;
                        }
                        std::cout << "Selected original menu row " << menu.selected << ", handler 0x" << std::hex << target << std::dec << " (not implemented)\n";
                        SDL_SetWindowTitle(window, "Original menu selected - handler not implemented (see console)");
                    }
                    if (event.key.key == SDLK_SPACE && !inMenu)
                    {
                        paused = !paused;
                    }
                }
            }
            auto& a = clips[clip];
            if (!paused && SDL_GetTicks() >= next)
            {
                if (!a.next(rgba))
                {
                    if (clip + 1 < clips.size())
                    {
                        ++clip;
                    }
                    else
                    {
                        showMenu();
                    }
                }
                else
                {
                    if (!SDL_UpdateTexture(texture, nullptr, rgba.data(), 640 * 4))
                    {
                        s2ne::fatal(SDL_GetError());
                    }
                    next = SDL_GetTicks() + a.delay;
                }
            }
            if (prologue && rawSave && prologue->pending_story_route())
            {
                auto captured = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                auto next = campaignAdapter.deploy_next(captured);
                auto staged = initialization.transition_stage(root, captured, next);
                auto readGlobal = [](const auto& bytes, size_t at)
                {
                    uint32_t v = 0;
                    for (unsigned i = 0; i < 4; ++i)
                    {
                        v |= uint32_t(bytes.at(at + i)) << (8 * i);
                    }
                    return int32_t(v);
                };
                auto disc = s2ne::resource_domain_for_campaign(readGlobal(staged.stage_4fb478, 12), readGlobal(staged.stage_4fb478, 8), readGlobal(staged.globals_4f7cd0, 16));
                staged.stage_4fb478[16] = uint8_t(disc);
                for (size_t i = 17; i < 20; ++i)
                {
                    staged.stage_4fb478[i] = 0;
                }
                auto restored = s2ne::OriginalSaveBridge::restore(root, staged);
                prologue->set_resource_domain(disc);
                prologue->clear_autosave_requests();
                auto publication = prologue->try_resume_campaign_story_route(std::move(restored.events));
                if (!publication)
                {
                    s2ne::report_error(publication.error());
                    return EXIT_FAILURE;
                }
                using std::swap;
                swap(*rawSave, staged);
                prologue->mark_campaign_arrivals_inserted(1);
                autosaveBlocked = false;
                showPrologue();
            }
            if (prologue && rawSave && prologue->interbattle_active() && !shopMenu && !churchMenu && !innMenu)
            {
                if (auto request = prologue->interbattle_request(); request)
                {
                    if (request->option == 1)
                    {
                        innMenu.emplace(root, s2ne::app_domain_for_save(*rawSave));
                    }
                    else if (request->option == 2)
                    {
                        shopMenu.emplace(root, s2ne::app_domain_for_save(*rawSave));
                        shopMenu->enable_presentation_timing();
                    }
                    else if (request->option == 3)
                    {
                        churchMenu.emplace(root, s2ne::app_domain_for_save(*rawSave));
                        churchMenu->set_donation([&](auto& save)
                            {
                                if (s2ne::OriginalChurch::supported(save))
                                {
                                    return s2ne::OriginalChurch::donate(save);
                                }
                                return prologue->interbattle_random([&](auto& rng, uint32_t seconds)
                                    {
                                        auto stagedSave = save;
                                        auto stagedEvents = prologue->events;
                                        if (!s2ne::OriginalChurch::donate_native(root, stagedSave, rng, seconds))
                                        {
                                            return false;
                                        }
                                        s2ne::OriginalChurch::sync_roster(stagedSave, stagedEvents);
                                        using std::swap;
                                        swap(save, stagedSave);
                                        swap(prologue->events, stagedEvents);
                                        return true;
                                    });
                            });
                    }
                    showPrologue();
                }
            }
            if (prologue && rawSave && prologue->interbattle_active())
            {
                if (auto request = prologue->interbattle_request(); request && request->option == 4)
                {
                    if ((prologue->events.stage == 61 || prologue->events.stage == 131))
                    {
                        auto target = prologue->campaign_next_stage();
                        std::cout << "Original61 post-victory route=" << target << " mode=" << (target ? s2ne::OriginalCampaign(root).stage(target).mode : 0) << " flags42=" << unsigned(prologue->events.flags[41]) << " flags43=" << unsigned(prologue->events.flags[42]) << " flags73=" << unsigned(prologue->events.flags[72]) << " flags75=" << unsigned(prologue->events.flags[74]) << '\n';
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-boss-outcome.sav");
                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                        std::ofstream output(path, std::ios::binary);
                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                        output.close();
                        std::cout << "Original boss-outcome checkpoint=" << path.string() << '\n';
                    }
                    auto next = campaignAdapter.deploy_next(*rawSave);
                    auto staged = initialization.transition_stage(root, *rawSave, next);
                    auto readGlobal = [](const auto& bytes, size_t at)
                    {
                        uint32_t v = 0;
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            v |= uint32_t(bytes.at(at + i)) << (8 * i);
                        }
                        return int32_t(v);
                    };
                    auto disc = s2ne::resource_domain_for_campaign(readGlobal(staged.stage_4fb478, 12), readGlobal(staged.stage_4fb478, 8), readGlobal(staged.globals_4f7cd0, 16));
                    staged.stage_4fb478[16] = uint8_t(disc);
                    for (size_t i = 17; i < 20; ++i)
                    {
                        staged.stage_4fb478[i] = 0;
                    }
                    auto restored = s2ne::OriginalSaveBridge::restore(root, staged);
                    prologue->set_interbattle_adapter([](auto request)
                        {
                            return request.option == 4 ? s2ne::OriginalInterbattle::Result::enter_battle : s2ne::OriginalInterbattle::Result::unsupported;
                        });
                    prologue->campaign_scan(0);
                    prologue->set_interbattle_adapter([](auto)
                        {
                            return s2ne::OriginalInterbattle::Result::unsupported;
                        });
                    prologue->set_resource_domain(disc);
                    prologue->clear_autosave_requests();
                    auto publication = prologue->try_resume_campaign_stage(std::move(restored.events));
                    if (!publication)
                    {
                        s2ne::report_error(publication.error());
                        return EXIT_FAILURE;
                    }
                    using std::swap;
                    swap(*rawSave, staged);
                    prologue->mark_campaign_arrivals_inserted(1);
                    autosaveBlocked = false;
                    showPrologue();
                }
            }
            if (prologue && !rawSave && !prologue->autosave_requests().empty() && prologue->events.stage != 140)
            {
                auto staged = initialization.fresh(root, prologue->events.stage, prologue->events.difficulty, prologue->events.flags, s2ne::OriginalInitialization::setup_from_settings(settingsState, 0));
                auto restored = s2ne::OriginalSaveBridge::restore(root, staged);
                auto publication = prologue->try_normalize_campaign_roster(std::move(restored.events), 1);
                if (!publication)
                {
                    s2ne::report_error(publication.error());
                    return EXIT_FAILURE;
                }
                rawSave = std::move(staged);
                autosaveBlocked = false;
            }
            if (prologue && !autosaveBlocked && !prologue->autosave_requests().empty())
            {
                bool allSaved = true;
                for (const auto& request : prologue->autosave_requests())
                {
                    if (!prologue->original_save_boundary_ready())
                    {
                        allSaved = false;
                        break;
                    }
                    if (!rawSave)
                    {
                        std::cerr << "Original autosave " << request.slot << " blocked: no complete original baseline\n";
                        allSaved = false;
                        break;
                    }
                    auto captured = s2ne::OriginalSaveBridge::try_capture(prologue->events, std::max(1u, prologue->battle_turn()), prologue->camera(), &*rawSave);
                    if (!captured)
                    {
                        s2ne::report_error(captured.error());
                        allSaved = false;
                        break;
                    }
                    auto staged = std::move(captured).value();
                    auto written = s2ne::OriginalSaveStore::try_write(isolatedSaveRoot ? *isolatedSaveRoot : root, request.slot, staged, saveItems);
                    if (!written)
                    {
                        s2ne::report_error(written.error());
                        allSaved = false;
                        break;
                    }
                    rawSave = std::move(staged);
                }
                if (allSaved)
                {
                    prologue->clear_autosave_requests();
                }
                else
                {
                    autosaveBlocked = true;
                }
            }
            if (prologue && prologue->in_battle() && !promotionMenu)
            {
                prologue->battle().set_command_request_availability([&](const auto& unit, int command)
                    {
                        if (!rawSave || rawSave->groups[0].inventory.size() != saveItems)
                        {
                            return false;
                        }
                        if (command == 7)
                        {
                            s2ne::OriginalSkills::Context context;
                            context.second = true;
                            return skillRules.executable_available(unit, context, true);
                        }
                        if (command == 6)
                        {
                            return (unit.learned[2] & 0x800) && skillRules.entry_available(unit, { });
                        }
                        if (command == 2)
                        {
                            for (unsigned id = 1; id <= saveItems; ++id)
                            {
                                if (rawSave->groups[0].inventory[id - 1] && promotionTransaction.learning_item(id) && promotionTransaction.usable_item(unit, id))
                                {
                                    return true;
                                }
                            }
                            return false;
                        }
                        if (command != 4)
                        {
                            return false;
                        }
                        auto found = std::find_if(rawSave->groups[0].units.begin(), rawSave->groups[0].units.end(), [&](const auto& r)
                            {
                                return unsigned(r[2]) + (unsigned(r[3]) << 8) == unit.id && unsigned(r[4]) + (unsigned(r[5]) << 8) == unit.appearance;
                            });
                        if (found == rawSave->groups[0].units.end())
                        {
                            return false;
                        }
                        auto probe = unit;
                        return promotionRules.eligible(probe, probe.experience_threshold, int(rawSave->globals_4f7cd0[0])) && !promotionRules.candidates(probe).empty();
                    });
            }
            if (prologue && rawSave && !battleMenu && !battleReports && !promotionMenu && !itemMenu && !skillMenu && (prologue->battle_requested_command() == 6 || prologue->battle_requested_command() == 7))
            {
                s2ne::OriginalSkills::Context context{ };
                context.second = prologue->battle_requested_command() == 7;
                prologue->clear_battle_requested_command();
                skillMenu.emplace(root);
                skillMenu->open(prologue->events.units.at(size_t(prologue->battle_selected())), context);
                if (skillMenu->page() == s2ne::OriginalSkills::Page::closed)
                {
                    skillMenu.reset();
                }
                showPrologue();
            }
            if (prologue && rawSave && !battleMenu && !battleReports && !promotionMenu && !itemMenu && prologue->battle_requested_command() == 2)
            {
                prologue->clear_battle_requested_command();
                usableItems.clear();
                usableItemSelection = 0;
                itemConfirmation = false;
                int selected = prologue->battle_selected();
                if (selected >= 0)
                {
                    const auto& unit = prologue->events.units.at(size_t(selected));
                    for (unsigned id = 1; id <= saveItems; ++id)
                    {
                        if (rawSave->groups[0].inventory[id - 1] && promotionTransaction.learning_item(id) && promotionTransaction.usable_item(unit, id))
                        {
                            usableItems.push_back(id);
                        }
                    }
                }
                itemMenu = !usableItems.empty();
                showPrologue();
            }
            if (prologue && !battleMenu && !battleReports && !promotionMenu && prologue->battle_requested_command() == 4)
            {
                prologue->clear_battle_requested_command();
                openSelectedPromotion();
            }
            if (prologue && !battleMenu && !battleReports && !promotionMenu && prologue->battle_requested_command() == 3)
            {
                prologue->clear_battle_requested_command();
                openSelectedDetail();
            }
            if (smokeDriver.after_events({ root, prologue, rawSave, isolatedSaveRoot, saveItems, window, renderer, audio, audioAvailable, inMenu, inSetup, running, presented, started, menu, setup, battleMenu, battleReports, skillMenu, skillTarget, itemMenu, saveMenu, settingsState, skillRules }, nowUi, promotionMenu, promotionRules, skillCastVerified, pendingSkillTarget, prologueRedraw))
            {
                continue;
            }
            if (prologue && prologueRedraw)
            {
                renderPrologue();
            }
            else if (!prologue && saveMenu)
            {
                auto image = title;
                menu.draw(image);
                saveMenu->draw(image, menu);
                if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
                {
                    s2ne::fatal(SDL_GetError());
                }
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            if (!SDL_RenderPresent(renderer))
            {
                s2ne::fatal(SDL_GetError());
            }
            if (shopMenu && shopMenu->transitioning())
            {
                shopMenu->frame_presented();
                showPrologue();
            }
            if (options.skillScheduleStress && presentedPrologue && prologue->skill_visual_active())
            {
                constexpr unsigned delays[]{ 0, 1, 49, 50, 51, 250, 0, 1000 };
                SDL_Delay(delays[presented % 8]);
            }
            if (presentedPrologue && presentedPrologue == prologue.get())
            {
                if (options.skillInputSmoke)
                {
                    static int previous = -1;
                    int frame = (options.skill94Input ? prologue->skill94_visual_frame() : options.skill88Input ? prologue->skill88_visual_frame() :
                                                                                                                  prologue->skill32_visual_frame());
                    if (frame >= 0 && frame != previous)
                    {
                        ++smokeDriver.state.skillInputFrames;
                        std::cout << "SDL SPELLSP" << (options.skill94Input ? 85 : options.skill88Input ? 79 :
                                                                                                          33)
                                  << " frame=" << frame << " ticks=" << SDL_GetTicks() << '\n';
                    }
                    previous = frame;
                    smokeDriver.state.skillSequence << presented << ',' << SDL_GetTicks() << ',' << smokeDriver.state.skillInputPhase << ',' << prologue->battle().animating_movement() << ',' << frame << ',' << smokeDriver.state.skillInputImageHash << '\n';
                }
                prologue->frame_presented();
            }
            ++presented;
            if (auto delay = s2ne::original_ui_frame_delay(nowUi, SDL_GetTicks()))
            {
                SDL_Delay(delay);
            }
        }
        if (options.skillInputSmoke && !smokeDriver.state.skillInputVerified)
        {
            s2ne::fatal("Normal-input skill32 acceptance interrupted");
        }
        if (options.promotionSmoke && !smokeDriver.state.promotionVerified)
        {
            s2ne::fatal("Earned promotion validation interrupted");
        }
        if (options.clearSmoke && !smokeDriver.state.clearVerified && !smokeDriver.state.mineralQuestVerified)
        {
            s2ne::fatal("First-battle validation interrupted before original victory report");
        }
        if (options.feedbackSmoke && (!smokeDriver.state.feedbackFlash || !smokeDriver.state.feedbackNumbers || !smokeDriver.state.feedbackHp || !smokeDriver.state.feedbackReaction))
        {
            s2ne::fatal("Feedback validation interrupted before HP, flash, numbers and original indexed reaction were observed");
        }
        std::cout << "Presented " << presented << " frames\n";
        audio.stop();

        return 0;
    }
}
