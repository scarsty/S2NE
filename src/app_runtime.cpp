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
#include "original_presentation.hpp"
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
#include "original_spirit_sight.hpp"
#include "original_teleport_smoke.hpp"
#include "prologue.hpp"
#include "runtime_diagnostics.hpp"
#include "startup_trace.hpp"
#include "stdio_protocol.hpp"
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
    s2ne::StartupTrace startup;
    startup.mark("entry");
    s2ne::configure_runtime_diagnostics();
    s2ne::AppSdlSession sdl;
    auto& window = sdl.window;
    auto& renderer = sdl.renderer;
    auto& texture = sdl.texture;
    {
        const s2ne::AppOptions options(argc, argv);
        std::optional<s2ne::StdioProtocol> stdio;
        struct OutputRestore
        {
            std::streambuf* previous{};
            ~OutputRestore()
            {
                if (previous) { std::cout.rdbuf(previous); }
            }
        } outputRestore;
        if (options.stdio)
        {
            stdio.emplace();
            outputRestore.previous = std::cout.rdbuf(std::cerr.rdbuf());
        }
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
            bool preserve{};
            ~SaveTestCleanup()
            {
                if (root && !preserve)
                {
                    std::error_code ignored;
                    std::filesystem::remove_all(*root, ignored);
                }
            }
        } saveTestCleanup{ isolatedSaveRoot };
        if (options.stdio)
        {
            if (!std::filesystem::create_directory(options.stdioSaveRoot)) { throw std::runtime_error("stdio save-root must be a new directory"); }
            isolatedSaveRoot = options.stdioSaveRoot;
            saveTestCleanup.preserve = true;
        }
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
            startup.mark(id == 21 ? "flic21" : "flic22");
        }
        auto titleResult = s2ne::try_load_title(root);
        if (!titleResult)
        {
            s2ne::report_error(titleResult.error());
            return EXIT_FAILURE;
        }
        auto title = std::move(titleResult).value();
        startup.mark("title");
        auto menuResult = s2ne::OriginalMenu::create(root);
        if (!menuResult)
        {
            s2ne::report_error(menuResult.error());
            return EXIT_FAILURE;
        }
        auto menu = std::move(menuResult).value();
        startup.mark("menu");
        auto setupResult = s2ne::NewGameSetup::create(root);
        if (!setupResult)
        {
            s2ne::report_error(setupResult.error());
            return EXIT_FAILURE;
        }
        auto setup = std::move(setupResult).value();
        startup.mark("setup");
        auto setupCheck = setup.draw(title, menu);
        auto menuCheck = title;
        menu.draw(menuCheck);
        startup.mark("trial-draw");
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
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            s2ne::fatal(SDL_GetError());
        }
        s2ne::OpeningAudio audio;
        startup.mark("sdl-init");
        bool audioAvailable = false;
        if (!SDL_CreateWindowAndRenderer("Northern Envoy - original intro and menu", 960, 720,
                SDL_WINDOW_RESIZABLE, &window, &renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480);
        startup.mark("window-renderer");
        if (!texture)
        {
            s2ne::fatal(SDL_GetError());
        }
        auto presentation = s2ne::configure_original_presentation(renderer, texture);
        if (!presentation)
        {
            s2ne::fatal(presentation.error());
        }
        // Present the original opening frame before preparing battle-only data or audio.
        std::vector<uint8_t> firstFrame;
        if (!clips.front().next(firstFrame) || !SDL_UpdateTexture(texture, nullptr, firstFrame.data(), 640 * 4) || !SDL_RenderClear(renderer) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
        {
            s2ne::fatal("Cannot present original opening frame: " + std::string(SDL_GetError()));
        }
        startup.mark("opening-first-present");
        startup.mark("opening-frame-hash=" + std::to_string(s2ne::resource_hash(firstFrame)));
        SDL_PumpEvents();
        startup.mark("opening-events-pumped");
        auto stdioLogicalFrame = firstFrame;
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            s2ne::fatal(SDL_GetError());
        }
        startup.mark("audio-init");
        auto audioStart = audio.start(root / "MP3" / "SUPERSP_A02.mp3");
        audioAvailable = bool(audioStart);
        startup.mark("audio-start");
        if (!audioStart)
        {
            s2ne::report_error(audioStart.error());
            std::cerr << "Audio unavailable; continuing without sound\n";
        }
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
        size_t usableItemSelection{};
        bool itemMenu = false, itemConfirmation = false;
        std::optional<s2ne::OriginalSkills> skillMenu;
        std::optional<size_t> skillTarget;
        std::optional<size_t> pendingSkillTarget;
        std::optional<s2ne::Battle::Point> sleepCenter;
        std::optional<size_t> transferSource;
        std::optional<s2ne::Battle::Point> transferDestination;
        bool pendingSkillVisualStarted = false;
        std::optional<std::vector<uint8_t>> spiritSightImage;
        std::vector<uint8_t> spiritSightIndices;
        std::optional<unsigned> screenshotSequence;
        std::function<void()> spiritSightCommit;
        bool skillCastVerified = false;
        s2ne::OriginalSkills skillRules(root);
        startup.mark("skill-rules");
        s2ne::OriginalSkill94Transaction sword94(root);
        auto drain = s2ne::OriginalDrain::create(root);
        if (!drain) { s2ne::fatal(drain.error()); }
        startup.mark("sword94");
        bool autosaveBlocked = false;
        s2ne::Prologue* connectedCampaign = nullptr;
        s2ne::OriginalInitialization initialization(root);
        startup.mark("initialization");
        s2ne::AppCampaignAdapter campaignAdapter(root, prologue, rawSave);
        startup.mark("campaign-adapter");
        s2ne::OriginalPromotionTransaction promotionTransaction(root);
        startup.mark("promotion-transaction");
        s2ne::OriginalPromotion promotionRules(root);
        startup.mark("promotion-rules");
        std::optional<s2ne::OriginalMap> promotionMap;
        std::optional<s2ne::OriginalConfirmation> overwriteConfirmation;
        std::ifstream itemFile(root / "ITEMSP.BIN", std::ios::binary);
        std::vector<uint8_t> itemBytes{ std::istreambuf_iterator<char>(itemFile), {} };
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
            auto created = s2ne::Prologue::create(root, restored.events.difficulty);
            if (!created)
            {
                s2ne::report_error(created.error());
                return EXIT_FAILURE;
            }
            prologue = std::move(created).value();
            auto domain = s2ne::try_app_domain_for_save(restored.raw);
            if (!domain)
            {
                s2ne::report_error(domain.error());
                return EXIT_FAILURE;
            }
            prologue->set_resource_domain(domain.value());
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
            auto created = s2ne::Prologue::create(root, restored.events.difficulty);
            if (!created)
            {
                s2ne::report_error(created.error());
                return EXIT_FAILURE;
            }
            prologue = std::move(created).value();
            auto domain = s2ne::try_app_domain_for_save(restored.raw);
            if (!domain)
            {
                s2ne::report_error(domain.error());
                return EXIT_FAILURE;
            }
            prologue->set_resource_domain(domain.value());
            const bool controlledHostileStatus = !options.skillControl.empty() && (NativeAbilityBuffs::find(options.inputSkill) || NativeTimedBuffs::find(options.inputSkill) || options.inputSkill == 8 || options.inputSkill == 12 || options.inputSkill == 35 || options.inputSkill == 39 || options.inputSkill == 48 || options.inputSkill == 56 || options.inputSkill == 60 || options.inputSkill == 17 || options.inputSkill == 25 || options.inputSkill == 52 || options.inputSkill == 61 || options.inputSkill == 13 || options.inputSkill == 15 || options.inputSkill == 53 || options.inputSkill == 34 || options.inputSkill == 38 || options.inputSkill == 54 || options.inputSkill == 96 || NativeTimedBuffs::find(options.inputSkill));
            if (controlledHostileStatus)
            {
                auto actor = std::find_if(restored.events.units.begin(), restored.events.units.end(), [](const auto& u)
                    {
                        return u.id == 1;
                    });
                if (actor == restored.events.units.end()) { fatal("Controlled053 source actor missing"); }
                unsigned int group;
                if (NativeAbilityBuffs::find(options.inputSkill)) { group = NativeAbilityBuffs::find(options.inputSkill)->group; }
                else if (NativeTimedBuffs::find(options.inputSkill)) { group = NativeTimedBuffs::find(options.inputSkill)->group; }
                else if (options.inputSkill == 8) { group = 0; }
                else if (options.inputSkill == 12) { group = 1; }
                else if (options.inputSkill == 35 || options.inputSkill == 39) { group = 3; }
                else if (options.inputSkill == 48) { group = 4; }
                else if (options.inputSkill == 56 || options.inputSkill == 60) { group = 5; }
                else if (options.inputSkill == 17) { group = 1; }
                else if (options.inputSkill == 25) { group = 2; }
                else if (options.inputSkill == 52) { group = 4; }
                else if (options.inputSkill == 61) { group = 5; }
                else if (options.inputSkill == 96) { group = 9; }
                else if (options.inputSkill == 53 || options.inputSkill == 54) { group = 5; }
                else if (options.inputSkill == 34 || options.inputSkill == 38) { group = 3; }
                else
                {
                    group = 1;
                }

                unsigned int bit;
                if (NativeAbilityBuffs::find(options.inputSkill)) { bit = NativeAbilityBuffs::find(options.inputSkill)->bit; }
                else if (NativeTimedBuffs::find(options.inputSkill)) { bit = NativeTimedBuffs::find(options.inputSkill)->bit; }
                else if (options.inputSkill == 8 || options.inputSkill == 60) { bit = 7; }
                else if (options.inputSkill == 12) { bit = 1; }
                else if (options.inputSkill == 35) { bit = 2; }
                else if (options.inputSkill == 39) { bit = 6; }
                else if (options.inputSkill == 48) { bit = 5; }
                else if (options.inputSkill == 56) { bit = 3; }
                else if (options.inputSkill == 17) { bit = 6; }
                else if (options.inputSkill == 25) { bit = 4; }
                else if (options.inputSkill == 52) { bit = 9; }
                else if (options.inputSkill == 61) { bit = 8; }
                else if (options.inputSkill == 96) { bit = 2; }
                else if (options.inputSkill == 34 || options.inputSkill == 54) { bit = 1; }
                else if (options.inputSkill == 38) { bit = 5; }
                else if (options.inputSkill == 53) { bit = 0; }
                else if (options.inputSkill == 13) { bit = 2; }
                else
                {
                    bit = 4;
                }

                actor->learned[group] |= uint16_t(1u << bit);
                if (options.inputSkill == 76 || options.inputSkill == 78 || options.inputSkill == 79)
                {
                    auto partner = std::find_if(restored.events.units.begin(), restored.events.units.end(), [&](const auto& u)
                        {
                            return u.id != actor->id && u.controllable() && u.present(restored.turn);
                        });
                    if (partner == restored.events.units.end()) { fatal("Controlled cooperative source partner missing"); }
                    partner->x = actor->x + (actor->y & 1);
                    partner->y = actor->y - 1;
                    partner->learned[group] |= uint16_t(1u << bit);
                    partner->mp = 1000;
                    partner->action_points = partner->carry_points = 110;
                    partner->action_state = 0;
                    partner->status34 = 0;
                    std::cout << "CONTROLLED cooperative explicit adjacent partner=" << partner->id << " learned/coordinates/MP/AP injected; NOT natural\n";
                }
                std::cout << "CONTROLLED hostile status explicit learned bit injection on actor1; NOT natural learning; source SAVE read-only\n";
            }
            prologue->restore_battle(std::move(restored.events), restored.turn, !controlledHostileStatus);
            if (controlledHostileStatus) { std::cout << "CONTROLLED hostile status battle-only restore: story-stage continuation suppressed; NOT natural campaign entry\n"; }
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
            auto created = s2ne::Prologue::create(root, restored.events.difficulty);
            if (!created)
            {
                s2ne::report_error(created.error());
                return EXIT_FAILURE;
            }
            prologue = std::move(created).value();
            auto domain = s2ne::try_app_domain_for_save(restored.raw);
            if (!domain)
            {
                s2ne::report_error(domain.error());
                return EXIT_FAILURE;
            }
            prologue->set_resource_domain(domain.value());
            prologue->restore_battle(std::move(restored.events), restored.turn);
            rawSave = std::move(restored.raw);
            paused = true;
        }
        auto settingsState = s2ne::OriginalSettings::load_state(root);
        menu.small_font = settingsState.small_font != 0;
        auto showSettings = [&]
        {
            auto rendered = settings->try_draw(s2ne::load_title(root, 20, false));
            if (!rendered)
            {
                s2ne::report_error(rendered.error());
                settings.reset();
                return;
            }
            auto image = std::move(rendered).value();
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4))
            {
                s2ne::fatal(SDL_GetError());
            }
        };
        auto beginSettings = [&]
        {
            auto created = s2ne::OriginalSettings::create(root, 0, settingsState);
            if (!created)
            {
                s2ne::report_error(created.error());
                return;
            }
            settings.emplace(std::move(*created.value()));
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
            if (innMenu)
            {
                innMenu->tick(s2ne::OriginalShopMenu::wall_clock_cs());
            }
            if (shopMenu && rawSave)
            {
                shopMenu->tick(s2ne::OriginalShopMenu::wall_clock_cs(), *rawSave);
                if (auto point = shopMenu->take_cursor())
                {
                    float x{}, y{};
                    if (SDL_RenderCoordinatesToWindow(renderer, float(point->first), float(point->second), &x, &y))
                    {
                        SDL_WarpMouseInWindow(window, x, y);
                    }
                }
            }
            std::vector<uint8_t> image;
            if (innMenu && rawSave) { image = innMenu->draw(*rawSave); }
            else if (churchMenu && rawSave) { image = churchMenu->draw(*rawSave); }
            else if (shopMenu && rawSave) { image = shopMenu->draw(*rawSave); }
            else
            {
                image = prologue->draw(menu);
                if (spiritSightImage) { image = *spiritSightImage; }
            }

            if (options.skillInputSmoke)
            {
                if (options.inputSkill == 47 && prologue->invisibility_presentation_pending())
                {
                    if (prologue->original_save_boundary_ready()) { s2ne::fatal("4748F3 pending viewport did not lock save boundary"); }
                    ++smokeDriver.state.invisibilityPresentations;
                }
                smokeDriver.state.skillInputImageHash = 1469598103934665603ull;
                for (auto c : image)
                {
                    smokeDriver.state.skillInputImageHash ^= c;
                    smokeDriver.state.skillInputImageHash *= 1099511628211ull;
                }
                int visualFrame;
                if ((options.inputSkill >= 64 && options.inputSkill <= 74) || NativeDamageRecords::ordinary_sword(options.inputSkill) || options.inputSkill == 29 || options.inputSkill == 18 || options.inputSkill == 40 || options.inputSkill == 45 || options.inputSkill == 50 || options.inputSkill == 51) { visualFrame = prologue->skill88_visual_frame(); }
                else if (options.skill94Input) { visualFrame = prologue->skill94_visual_frame(); }
                else if ((options.skill88Input || options.inputSkill == 1 || (NativeAbilityBuffs::find(options.inputSkill) || NativeTimedBuffs::find(options.inputSkill) || (options.inputSkill >= 2 && options.inputSkill <= 10) || options.inputSkill == 8 || options.inputSkill == 12 || options.inputSkill == 35 || options.inputSkill == 39 || options.inputSkill == 48 || options.inputSkill == 56 || options.inputSkill == 60 || options.inputSkill == 17 || options.inputSkill == 25 || options.inputSkill == 52 || options.inputSkill == 61 || options.inputSkill == 13 || options.inputSkill == 15 || options.inputSkill == 53 || options.inputSkill == 34 || options.inputSkill == 38 || options.inputSkill == 54 || options.inputSkill == 96 || NativeTimedBuffs::find(options.inputSkill)))) { visualFrame = prologue->skill88_visual_frame(); }
                else
                {
                    visualFrame = prologue->skill32_visual_frame();
                }
                int frame = (visualFrame);
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
                auto rendered = battleMenu->try_draw(image);
                if (rendered)
                {
                    image = std::move(rendered).value();
                }
                else
                {
                    s2ne::report_error(rendered.error());
                    battleMenu.reset();
                }
            }
            if (battleReports)
            {
                auto rendered = battleReports->try_draw();
                if (rendered)
                {
                    image = std::move(rendered).value();
                }
                else
                {
                    s2ne::report_error(rendered.error());
                    battleReports.reset();
                }
            }
            if (promotionMenu)
            {
                SDL_ShowCursor();
                auto rendered = promotionMenu->try_draw(image);
                if (rendered)
                {
                    image = std::move(rendered).value();
                }
                else
                {
                    s2ne::report_error(rendered.error());
                    promotionMenu.reset();
                }
            }
            if (skillMenu && !pendingSkillTarget)
            {
                if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && prologue->battle_selected() >= 0)
                {
                    auto actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                    auto camera = prologue->camera();
                    if (skillMenu->transfer51_supported() && transferSource)
                    {
                        const auto cells = prologue->battle().spell_transfer_destinations(actor, prologue->events.units.at(*transferSource), prologue->events.units);
                        for (const auto cell : cells)
                        {
                            const auto point = prologue->screen_cell(cell);
                            if (point.x >= 0 && point.x < 640 && point.y >= 0 && point.y < 480)
                            {
                                const size_t p = (size_t(point.y) * 640 + point.x) * 4;
                                image[p] = 255;
                                image[p + 1] = 80;
                                image[p + 2] = 80;
                            }
                        }
                    }
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
                auto rendered = saveMenu->try_draw(image, menu);
                if (!rendered)
                {
                    s2ne::report_error(rendered.error());
                    saveMenu.reset();
                }
            }
            if (overwriteConfirmation)
            {
                overwriteConfirmation->draw(image, menu);
            }
            if (stdio) { stdioLogicalFrame = image; }
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
                    if (!promotionMap)
                    {
                        auto created = s2ne::OriginalMap::create(root);
                        if (!created)
                        {
                            s2ne::report_error(created.error());
                            return false;
                        }
                        promotionMap.emplace(std::move(created).value());
                    }
                    return campaignAdapter.commit_promotion(selected, request, saveItems, promotionTransaction, *promotionMap);
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
            auto created = s2ne::Prologue::create(root, setup.confirmed);
            if (!created)
            {
                s2ne::report_error(created.error());
                return;
            }
            prologue = std::move(created).value();
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
        startup.mark("event-loop");
        bool firstPresent = true;
        bool firstEvent = true;
        bool firstInput = true;
        std::optional<s2ne::StdioProtocol::Json> stdioPending;
        uint64_t stdioReadyAt{};
        if (stdio && !options.stdioInitialSave.empty())
        {
            auto initial = s2ne::OriginalSave::load(options.stdioInitialSave, saveItems);
            auto restored = s2ne::OriginalSaveBridge::restore(root, initial);
            prologue = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
            prologue->restore_battle(std::move(restored.events), restored.turn);
            rawSave = std::move(initial);
            s2ne::OriginalSaveStore::write(*isolatedSaveRoot, 1, *rawSave, saveItems);
            inMenu = inSetup = false;
            showPrologue();
        }
        auto stdioObserve = [&]()
        {
            using Json = s2ne::StdioProtocol::Json;
            Json state{ { "scene", "opening" }, { "modal", "none" }, { "units", Json::array() }, { "commands", Json::array() }, { "skills", Json::array() } };
            if (inMenu) { state["scene"] = "title"; }
            if (inSetup) { state["scene"] = "setup"; }
            if (prologue)
            {
                state["scene"] = "prologue";
                if (prologue->in_battle()) { state["scene"] = "battle"; }
                state["stage"] = prologue->events.stage;
                state["turn"] = prologue->battle_turn();
                state["selected"] = prologue->battle_selected();
                state["input_locked"] = !prologue->original_save_boundary_ready() || prologue->computer_actions() || prologue->campaign_modal();
                state["blocker"] = prologue->campaign_blocker();
                if (prologue->campaign_modal()) { state["modal"] = "campaign"; }
                if (prologue->in_battle()) { state["battle_state"] = int(prologue->battle_state()); }
                for (size_t i = 0; i < prologue->events.units.size(); ++i)
                {
                    const auto& u = prologue->events.units[i];
                    auto point = prologue->battle().screen_center({ u.x, u.y }, prologue->camera().first, prologue->camera().second);
                    state["units"].push_back({ { "index", i }, { "id", u.id }, { "hostile", u.hostile() }, { "x", u.x }, { "y", u.y }, { "screen_x", point.x }, { "screen_y", point.y }, { "hp", u.hp }, { "max_hp", u.max_hp }, { "mp", u.mp }, { "st", u.st }, { "ap", u.action_points }, { "level", u.level }, { "xp", u.experience }, { "xp_threshold", u.experience_threshold }, { "learned", u.learned }, { "proficiency", u.learned_proficiency } });
                }
                const int selected = prologue->battle_selected();
                if (prologue->in_battle() && selected >= 0)
                {
                    for (unsigned command = 0; command < 6; ++command)
                    {
                        state["commands"].push_back({ { "id", command }, { "slot", prologue->battle().command_slot(prologue->events.units, int(command)) }, { "enabled", prologue->battle().command_available(prologue->events.units, int(command)) } });
                    }
                }
                if (skillMenu && selected >= 0)
                {
                    const auto& unit = prologue->events.units.at(size_t(selected));
                    for (const auto& group : skillMenu->groups())
                    {
                        for (const auto& record : group.records)
                        {
                            auto failure = skillMenu->resource_failure(unit, record);
                            static constexpr const char* failures[]{ "none", "mp", "st", "action_points", "hp", "partner_unresolved", "world_unresolved", "unsupported_execution", "stale_unit" };
                            state["skills"].push_back({ { "id", record.id }, { "group", record.group }, { "bit", record.bit }, { "resource_failure", failures[int(failure)] }, { "cost", record.word(13) }, { "effect", record.effect() } });
                        }
                    }
                    state["selected_skill"] = skillMenu->selected().id;
                    state["skill_page"] = int(skillMenu->page());
                    auto probe = *skillMenu;
                    state["selected_skill_failure"] = int(probe.select(unit));
                    state["modal"] = "skills";
                }
            }
            if (battleMenu) { state["modal"] = "battle_menu"; }
            if (promotionMenu) { state["modal"] = "promotion"; }
            if (saveMenu) { state["modal"] = "save"; }
            if (settings) { state["modal"] = "settings"; }
            if (spiritSightImage) { state["modal"] = "spirit_sight"; }
            return state;
        };
        while (running)
        {
            uint64_t nowUi = SDL_GetTicks();
            if (stdio && !stdioPending)
            {
                stdio->poll();
                if (auto request = stdio->next())
                {
                    auto response = s2ne::StdioProtocol::Json{ { "ok", true } };
                    try
                    {
                        if (request->contains("id")) { response["id"] = request->at("id"); }
                        if (request->contains("protocol_error")) { throw std::runtime_error(request->at("protocol_error").get<std::string>()); }
                        const auto command = request->at("cmd").get<std::string>();
                        if (command == "observe") { response["state"] = stdioObserve(); }
                        else if (command == "quit") { running = false; }
                        else if (command == "wait")
                        {
                            if (!request->at("ms").is_number_integer()) { throw std::runtime_error("ms must be an integer"); }
                            const int ms = request->at("ms").get<int>();
                            if (ms < 0 || ms > 10000) { throw std::runtime_error("wait ms must be 0..10000"); }
                            stdioReadyAt = nowUi + unsigned(ms);
                            stdioPending = response;
                        }
                        else if (command == "key")
                        {
                            const auto key = SDL_GetKeyFromName(request->at("key").get<std::string>().c_str());
                            if (key == SDLK_UNKNOWN) { throw std::runtime_error("unknown key name"); }
                            SDL_Event event{};
                            event.type = SDL_EVENT_KEY_DOWN;
                            event.key.windowID = SDL_GetWindowID(window);
                            event.key.key = key;
                            event.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
                            if (!SDL_PushEvent(&event)) { throw std::runtime_error(SDL_GetError()); }
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "mouse")
                        {
                            if (!request->at("x").is_number_integer() || !request->at("y").is_number_integer()) { throw std::runtime_error("mouse coordinates must be integers"); }
                            const int x = request->at("x").get<int>(), y = request->at("y").get<int>();
                            if (x < 0 || x >= 640 || y < 0 || y >= 480) { throw std::runtime_error("mouse outside logical 640x480 frame"); }
                            const auto button = request->value("button", std::string("left"));
                            if (button != "left" && button != "right" && button != "move") { throw std::runtime_error("button must be left/right/move"); }
                            float wx{}, wy{};
                            if (!SDL_RenderCoordinatesToWindow(renderer, float(x), float(y), &wx, &wy)) { throw std::runtime_error(SDL_GetError()); }
                            SDL_Event event{};
                            event.type = SDL_EVENT_MOUSE_MOTION;
                            event.motion.windowID = SDL_GetWindowID(window);
                            event.motion.x = wx;
                            event.motion.y = wy;
                            SDL_PushEvent(&event);
                            if (button != "move")
                            {
                                event = {};
                                event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                                event.button.windowID = SDL_GetWindowID(window);
                                event.button.x = wx;
                                event.button.y = wy;
                                event.button.button = SDL_BUTTON_LEFT;
                                if (button == "right") { event.button.button = SDL_BUTTON_RIGHT; }
                                event.button.down = true;
                                SDL_PushEvent(&event);
                                event.type = SDL_EVENT_MOUSE_BUTTON_UP;
                                event.button.down = false;
                                SDL_PushEvent(&event);
                            }
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "screenshot")
                        {
                            auto* surface = SDL_CreateSurfaceFrom(640, 480, SDL_PIXELFORMAT_RGBA32, stdioLogicalFrame.data(), 640 * 4);
                            if (!surface) { throw std::runtime_error(SDL_GetError()); }
                            const auto path = *isolatedSaveRoot / "frame.bmp";
                            const bool saved = SDL_SaveBMP(surface, path.string().c_str());
                            SDL_DestroySurface(surface);
                            if (!saved) { throw std::runtime_error(SDL_GetError()); }
                            response["path"] = path.string();
                        }
                        else
                        {
                            throw std::runtime_error("unknown command");
                        }
                    } catch (const std::exception& error)
                    {
                        response["ok"] = false;
                        response["error"] = error.what();
                    }
                    if (!stdioPending) { stdio->respond(response); }
                }
                else if (stdio->eof()) { running = false; }
            }
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
                if (!spiritSightImage && !battleMenu && !battleReports && !promotionMenu && !itemMenu && (!skillMenu || pendingSkillTarget))
                {
                    unsigned int tickMilliseconds;
                    if (prologue->ending_active()) { tickMilliseconds = unsigned(nowUi - lastUiTick); }
                    else if (options.clearSmoke) { tickMilliseconds = 120u; }
                    else if (options.feedbackSmoke) { tickMilliseconds = std::max(30u, unsigned(nowUi - lastUiTick)); }
                    else
                    {
                        tickMilliseconds = unsigned(nowUi - lastUiTick);
                    }
                    prologue->tick(tickMilliseconds);
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
                    static size_t endingObserved = ~size_t{};
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
                    bool visualStarted;
                    if (skillMenu->selected().id == 94) { visualStarted = prologue->begin_skill94_visual(unsigned(prologue->battle_selected()), unsigned(*pendingSkillTarget)); }
                    else if (skillMenu->summon_supported() || skillMenu->transfer51_supported() || skillMenu->selected().id == 88 || skillMenu->selected().id == 1 || skillMenu->status_area_supported())
                    {
                        unsigned visual_target = unsigned(prologue->battle_selected());
                        if (skillMenu->selected().id != 88) { visual_target = unsigned(*pendingSkillTarget); }
                        unsigned status_skill = 0;
                        if (skillMenu->selected().id >= 4 && skillMenu->status_area_supported()) { status_skill = skillMenu->selected().id; }
                        if (skillMenu->transfer51_supported())
                        {
                            status_skill = 51;
                            visual_target = unsigned(prologue->battle_selected());
                            prologue->set_spell_transfer_visual_source(unsigned(*pendingSkillTarget));
                        }
                        if (skillMenu->summon_supported())
                        {
                            status_skill = skillMenu->selected().id;
                            visual_target = unsigned(prologue->battle_selected());
                        }
                        const unsigned performer = unsigned(prologue->battle_selected());
                        auto partner = skillMenu->cooperative_partner(prologue->events.units.at(performer), prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                            {
                                return prologue->battle().cell({ x, y }).elevation;
                            });
                        std::optional<unsigned> partnerVisual;
                        if (partner) { partnerVisual = unsigned(*partner); }
                        std::vector<unsigned> targets;
                        if (status_skill == 78 || (NativeDamageRecords::find(status_skill) && NativeDamageRecords::find(status_skill)->dispatch == 9))
                        {
                            const auto& actor = prologue->events.units.at(performer);
                            const auto& center = prologue->events.units.at(*pendingSkillTarget);
                            for (auto i : skillMenu->collect_targets(actor, center, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                     {
                                         return prologue->battle().skill_area_cell({ x, y });
                                     }))
                            {
                                targets.push_back(unsigned(i));
                            }
                        }
                        visualStarted = prologue->begin_skill88_visual(visual_target, skillMenu->selected().id == 1, skillMenu->selected().id == 2, sleepCenter, skillMenu->selected().id == 3, status_skill, (partner || skillMenu->fissure29_supported() || skillMenu->drain_supported() || NativeDamageRecords::find(status_skill)) ? performer : visual_target, partnerVisual, std::move(targets));
                    }
                    else
                    {
                        visualStarted = prologue->begin_skill32_visual(unsigned(prologue->battle_selected()), unsigned(*pendingSkillTarget), skillMenu->selected().id == 33);
                    }

                    pendingSkillVisualStarted = visualStarted;
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
                                if (options.skillControl == "rollback") { return false; }
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
                                sword.damage.attack_mode = s2ne::Battle::attack_mode({ actor.x, actor.y }, { victim.x, victim.y }, victim.direction, prologue->events.difficulty == 1);
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
                                    swordFeedbackPending = prologue->battle().begin_skill94_feedback(size_t(selected), target, victim.hp, outcome.damage, [&, selected, target, finalActor, finalVictim, inventory, seed, calls, outcome]()
                                        {
                                            auto publish = [&, selected, target, finalActor, finalVictim, inventory, seed, calls]()
                                            {
                                                finalActor->animation_action = 0;
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
                                                if (!prologue->begin_synchronous_death(finalVictim->id, std::move(death), finalVictim.get()))
                                                {
                                                    s2ne::fatal("Original94 synchronous death rejected before settlement");
                                                }
                                            }
                                            else if (outcome.revived || outcome.reverse_heal)
                                            {
                                                const int hp = outcome.revived ? 0 : outcome.hp_snapshot;
                                                const int healing = outcome.revived ? finalVictim->max_hp : outcome.healed;
                                                if (!prologue->battle().begin_healing_feedback(size_t(selected), target, hp, healing, std::move(publish)))
                                                {
                                                    s2ne::fatal("Original94 secondary468BAC feedback rejected before settlement");
                                                }
                                                std::cout << "Original94 secondary468BAC revived=" << outcome.revived << " reverse=" << outcome.reverse_heal << " consumed=" << outcome.consumed_item << " healing=" << healing << '\n';
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
                            if (!skillMenu->pipeline_supported() && !skillMenu->summon_supported()) { return false; }
                            auto a = prologue->events.units.at(size_t(selected));
                            auto center = prologue->events.units.at(target);
                            if (sleepCenter)
                            {
                                center.x = sleepCenter->x;
                                center.y = sleepCenter->y;
                            }
                            context.status3c = a.action_status_runtime ? uint32_t(a.action_status_runtime->at(0x3C)) | uint32_t(a.action_status_runtime->at(0x3D)) << 8 | uint32_t(a.action_status_runtime->at(0x3E)) << 16 | uint32_t(a.action_status_runtime->at(0x3F)) << 24 : uint32_t(a.status3c) | uint32_t(a.status3d) << 8 | uint32_t(a.status3e) << 16 | uint32_t(a.status3f) << 24;
                            auto menu = *skillMenu;
                            menu.set_area_camera(prologue->camera());
                            menu.set_area_elevation([&](int x, int y)
                                {
                                    return prologue->battle().cell({ x, y }).elevation;
                                });
                            menu.set_area_valid([&](int x, int y)
                                {
                                    return prologue->battle().skill_area_cell({ x, y });
                                });
                            auto stream = rng;
                            const auto handler = menu.handler(menu.summon_supported());
                            const bool healing = handler == s2ne::OriginalSkills::Handler::healing465C14;
                            const bool status = handler == s2ne::OriginalSkills::Handler::status4668F0;
                            auto indices = menu.collect_targets(a, center, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                {
                                    return prologue->battle().skill_area_cell({ x, y });
                                });
                            if (handler == s2ne::OriginalSkills::Handler::army4666C5) { indices.clear(); }
                            if (handler == s2ne::OriginalSkills::Handler::summon465E04) { indices.clear(); }
                            if (handler == s2ne::OriginalSkills::Handler::transfer4665D8) { indices = { target }; }
                            std::vector<s2ne::OriginalEvents::Unit> victims;
                            for (auto i : indices) { victims.push_back(prologue->events.units[i]); }
                            auto partnerIndex = menu.cooperative_partner(a, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                {
                                    return prologue->battle().cell({ x, y }).elevation;
                                });
                            std::optional<s2ne::OriginalEvents::Unit> partner;
                            if (partnerIndex) { partner = prologue->events.units[*partnerIndex]; }
                            if (menu.damage_supported())
                            {
                                if (NativeDamageRecords::ordinary_sword(options.inputSkill) && !options.skillControl.empty())
                                {
                                    smokeDriver.state.controlledAreaTargets = indices;
                                    for (const auto& victim : victims) { std::cout << "CONTROLLED viewport target=" << victim.id << " hp=" << victim.hp << " status39=" << unsigned(victim.action_status_runtime->at(0x39)) << '\n'; }
                                }
                                std::vector<s2ne::OriginalSkill94Transaction::Context> contexts;
                                for (const auto& victim : victims)
                                {
                                    s2ne::OriginalSkill94Transaction::Context damage;
                                    damage.original_second_partition = skillMenu->request()->second;
                                    damage.target_runtime_known = victim.action_status_runtime.has_value();
                                    damage.synchronous_death_ready = s2ne::OriginalDeathDispatch::continuation(prologue->events, victim.id).supported;
                                    damage.seconds = seconds;
                                    damage.payment_state = context.skill_payment_state;
                                    auto actorTerrain = prologue->battle().original_42c88f_bytes(prologue->events, a);
                                    auto targetTerrain = prologue->battle().original_42c88f_bytes(prologue->events, victim);
                                    const auto group = menu.selected().group;
                                    damage.hit = { a.learned_proficiency[group], victim.learned_proficiency[group], a.fencing, victim.fencing, victim.tactics, int(int8_t(actorTerrain[1])), int(int8_t(targetTerrain[2])), a.st, victim.st };
                                    damage.elevation = s2ne::original_combat::terrain_height_difference(prologue->battle().cell({ a.x, a.y }).elevation, prologue->battle().cell({ victim.x, victim.y }).elevation, prologue->events.difficulty == 1);
                                    const auto& runtime = victim.action_status_runtime;
                                    damage.damage.difficulty = context.difficulty;
                                    damage.damage.attack_mode = s2ne::Battle::attack_mode({ a.x, a.y }, { victim.x, victim.y }, victim.direction, context.difficulty == 1);
                                    damage.damage.target_status39 = runtime ? runtime->at(0x39) : victim.status39.value_or(0);
                                    damage.damage.target_status3c = runtime ? runtime->at(0x3C) : victim.status3c;
                                    damage.damage.target_status3d = runtime ? runtime->at(0x3D) : victim.status3d;
                                    damage.damage.target_status3f = runtime ? runtime->at(0x3F) : victim.status3f;
                                    damage.damage.target_special13 = prologue->events.equipment_special(victim, 13);
                                    auto masks = prologue->events.skill_class_masks(victim);
                                    damage.damage.profession_resistance = masks[0];
                                    damage.damage.profession_weakness = masks[1];
                                    contexts.push_back(damage);
                                }
                                auto inventory = rawSave->groups[0].inventory;
                                auto packet = sword94.execute_area(menu, a, victims, center, prologue->events, stream, contexts, award, inventory, promotionTransaction, partner ? &*partner : nullptr);
                                if (!packet)
                                {
                                    s2ne::report_error(packet.error());
                                    return false;
                                }
                                auto presentation = std::make_shared<std::vector<s2ne::Prologue::DamagePresentation>>();
                                for (size_t n = 0; n < indices.size(); ++n)
                                {
                                    const auto& outcome = packet.value().targets[n];
                                    presentation->push_back({ unsigned(indices[n]), prologue->events.units[indices[n]].hp, outcome.damage, outcome.revived ? victims[n].max_hp : outcome.healed, outcome.hp_snapshot, outcome.killed, outcome.revived, outcome.reverse_heal, victims[n] });
                                }
                                auto seed = stream.seed_state(), calls = stream.draw_count();
                                swordFeedbackPending = prologue->begin_damage_area_feedback(unsigned(selected), presentation, 0,
                                    [&, selected, a = std::move(a), victims = std::move(victims), inventory = std::move(inventory), indices, seed, calls, partnerIndex, partner = std::move(partner)]() mutable
                                    {
                                        a.animation_action = 0;
                                        prologue->events.units.at(size_t(selected)) = std::move(a);
                                        if (partnerIndex && partner)
                                        {
                                            partner->animation_action = 0;
                                            prologue->events.units.at(*partnerIndex) = std::move(*partner);
                                        }
                                        for (size_t n = 0; n < indices.size(); ++n)
                                        {
                                            prologue->events.units[indices[n]] = std::move(victims[n]);
                                            const auto& unit = prologue->events.units[indices[n]];
                                            if (unit.status_flags & 0x20) { prologue->events.release_summon_slot(unit); }
                                        }
                                        rawSave->groups[0].inventory = std::move(inventory);
                                        prologue->events.rebuild_army_modes(0);
                                        prologue->battle().set_hostile_resources(prologue->events);
                                        prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                            {
                                                live.restore_stream(seed, calls);
                                                return true;
                                            });
                                        prologue->battle().commit_external_action();
                                        skillCastVerified = true;
                                        *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                        std::cout << "Original466082 area damage published once targets=" << indices.size() << '\n';
                                    });
                                if (!swordFeedbackPending) { s2ne::fatal("466082 feedback staging rejected"); }
                                return false;
                            }
                            auto drainInventory = rawSave->groups[0].inventory;
                            std::optional<s2ne::OriginalSave::State> summonedSave;
                            std::optional<s2ne::OriginalEvents> summonedEvents;
                            unsigned summonedIndex = 0;
                            s2ne::OriginalSkills::SummonAdapter summonAdapter;
                            if (handler == s2ne::OriginalSkills::Handler::summon465E04)
                            {
                                summonAdapter = [&](const auto& caster, unsigned slot, auto& stagedRandom)
                                {
                                    if (!prologue->events.army_status) { return false; }
                                    const unsigned army = caster.hostile() ? 1 : 0;
                                    const auto& slots = (*prologue->events.army_status)[army];
                                    const unsigned appearance = slots[slot * 3 + 1] | unsigned(slots[slot * 3 + 2]) << 8;
                                    const auto destination = prologue->battle().spell_summon_destination(caster, appearance, prologue->events.units);
                                    if (!destination) { return false; }
                                    auto before = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                    if (!before)
                                    {
                                        s2ne::report_error(before.error());
                                        return false;
                                    }
                                    s2ne::OriginalInitialization initialization(root);
                                    s2ne::OriginalPromotion promotion(root);
                                    s2ne::OriginalRandom growth({ stagedRandom.seed_state(), stagedRandom.draw_count() });
                                    unsigned control = caster.behavior == 0 ? 0 : 1;
                                    if (caster.action_status_runtime) { control = unsigned(caster.action_status_runtime->at(8)) | unsigned(caster.action_status_runtime->at(9)) << 8; }
                                    // MOV [caster+128h],SAR16 reads signed WORD12Ah (SP), not ST128h.
                                    auto created = initialization.try_summon_role(before.value(), prologue->events, army, slot, s2ne::OriginalSkills::summon_level(caster), destination->x, destination->y, control ? 1 : 0, growth, promotion, [seconds]
                                        {
                                            return seconds;
                                        });
                                    if (!created)
                                    {
                                        s2ne::report_error(created.error());
                                        return false;
                                    }
                                    auto restored = s2ne::OriginalSaveBridge::try_restore(root, created.value());
                                    if (!restored)
                                    {
                                        s2ne::report_error(restored.error());
                                        return false;
                                    }
                                    auto& units = restored.value().events.units;
                                    auto found = std::find_if(units.begin(), units.end(), [&](const auto& unit)
                                        {
                                            return unit.hostile() == caster.hostile() && unit.appearance == appearance && (unit.status_flags & 0x10);
                                        });
                                    if (found == units.end()) { return false; }
                                    summonedIndex = unsigned(found - units.begin());
                                    summonedSave = std::move(created).value();
                                    summonedEvents = std::move(restored).value().events;
                                    stagedRandom.restore_stream(growth.state().seed, growth.state().calls);
                                    return true;
                                };
                            }
                            auto packet = menu.execute_pipeline(a, victims, center, &prologue->events, &promotionTransaction, stream, context, award, partner ? &*partner : nullptr,
                                &drain.value(), &drainInventory, summonAdapter);
                            if (!packet)
                            {
                                s2ne::report_error(packet.error());
                                return false;
                            }
                            result = packet.value();
                            if (handler == s2ne::OriginalSkills::Handler::summon465E04)
                            {
                                if (!summonedSave || !summonedEvents) { return false; }
                                const auto seed = stream.seed_state(), calls = stream.draw_count();
                                auto staged = prologue->events;
                                staged.units = std::move(summonedEvents->units);
                                staged.army_status = summonedEvents->army_status;
                                auto caster = std::find_if(staged.units.begin(), staged.units.end(), [&](const auto& unit)
                                    {
                                        return unit.id == a.id && unit.appearance == a.appearance && unit.hostile() == a.hostile();
                                    });
                                if (caster == staged.units.end()) { return false; }
                                a.animation_action = 0;
                                *caster = std::move(a);
                                staged.units.at(summonedIndex).action_state = 2;
                                if (staged.units.at(summonedIndex).action_status_runtime) { staged.units.at(summonedIndex).action_status_runtime->at(0x2D) = 2; }
                                staged.rebuild_army_modes(0);
                                auto captured = s2ne::OriginalSaveBridge::try_capture(staged, prologue->battle_turn(), prologue->camera(), &*summonedSave);
                                if (!captured)
                                {
                                    s2ne::report_error(captured.error());
                                    return false;
                                }
                                auto presentationRoster = staged.units;
                                presentationRoster.at(summonedIndex).action_state = 0;
                                presentationRoster.at(summonedIndex).status_flags |= 8;
                                if (options.skillInputSmoke && options.skillControl == "summonfail") { presentationRoster.at(summonedIndex).animation_action = 8; }
                                auto started = prologue->begin_spell_summon(std::move(presentationRoster), summonedIndex,
                                    [&, staged = std::move(staged), saved = std::move(captured).value(), seed, calls, pendingMpGrowth]() mutable
                                    {
                                        prologue->events = std::move(staged);
                                        *rawSave = std::move(saved);
                                        smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                                        prologue->battle().set_hostile_resources(prologue->events);
                                        prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                            {
                                                live.restore_stream(seed, calls);
                                                return true;
                                            });
                                        prologue->battle().commit_external_action();
                                        skillCastVerified = true;
                                        std::cout << "Original465E04 summon roster/payment published once\n";
                                    });
                                if (!started)
                                {
                                    s2ne::report_error(started.error());
                                    return false;
                                }
                                swordFeedbackPending = true;
                                return false;
                            }
                            if (handler == s2ne::OriginalSkills::Handler::transfer4665D8)
                            {
                                if (!transferSource || !transferDestination || indices.size() != 1 || indices[0] != *transferSource) { return false; }
                                const auto destinations = prologue->battle().spell_transfer_destinations(prologue->events.units.at(size_t(selected)), prologue->events.units.at(*transferSource), prologue->events.units);
                                if (std::find(destinations.begin(), destinations.end(), *transferDestination) == destinations.end()) { return false; }
                                victims[0].x = transferDestination->x;
                                victims[0].y = transferDestination->y;
                                const auto seed = stream.seed_state(), calls = stream.draw_count();
                                const auto destination = *transferDestination;
                                const auto source = *transferSource;
                                if (!prologue->begin_spell_transfer(unsigned(source), destination,
                                        [&, selected, a = std::move(a), victims = std::move(victims), source, seed, calls, pendingMpGrowth]() mutable
                                        {
                                            smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                                            a.animation_action = 0;
                                            if (source == size_t(selected))
                                            {
                                                a.x = victims[0].x;
                                                a.y = victims[0].y;
                                                victims[0] = a;
                                            }
                                            prologue->events.units.at(size_t(selected)) = std::move(a);
                                            prologue->events.units.at(source) = std::move(victims[0]);
                                            prologue->events.rebuild_army_modes(0);
                                            prologue->battle().set_hostile_resources(prologue->events);
                                            prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                                {
                                                    live.restore_stream(seed, calls);
                                                    return true;
                                                });
                                            prologue->battle().commit_external_action();
                                            skillCastVerified = true;
                                            *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                            std::cout << "Original45DB10 transfer arrival published once\n";
                                        }))
                                {
                                    return false;
                                }
                                swordFeedbackPending = true;
                                return false;
                            }
                            if (handler == s2ne::OriginalSkills::Handler::drain467625 || handler == s2ne::OriginalSkills::Handler::fissure467625)
                            {
                                if (options.skillInputSmoke && options.skillControl == "drainfail" && !indices.empty())
                                {
                                    auto invalid = prologue->events.units;
                                    invalid.at(indices.back()).animation_action = 8;
                                    s2ne::OriginalMap probe(root);
                                    probe.load(prologue->events.map_id);
                                    const auto camera = prologue->camera();
                                    probe.camera_x = camera.first;
                                    probe.camera_y = camera.second;
                                    auto image = probe.draw();
                                    auto prepared = probe.try_draw_units(image, invalid, prologue->battle_turn());
                                    if (prepared) { s2ne::fatal("Drain late renderer fixture unexpectedly valid"); }
                                    s2ne::report_error(prepared.error());
                                    return false;
                                }
                                auto presentation = std::make_shared<std::vector<s2ne::Prologue::DamagePresentation>>();
                                for (size_t n = 0; n < indices.size(); ++n)
                                {
                                    const auto& outcome = result.drain_targets.at(n);
                                    std::cout << "Original467625 staged damage=" << outcome.damage << " revived=" << outcome.revived << " reverse=" << outcome.reverse << " item=" << outcome.consumed_item << '\n';
                                    presentation->push_back({ unsigned(indices[n]), prologue->events.units[indices[n]].hp, outcome.damage,
                                        outcome.healing, outcome.snapshot, outcome.killed, outcome.revived, outcome.reverse, victims[n],
                                        skillMenu->selected().effect(), outcome.caster_before, a.max_hp, outcome.reversed_damage, outcome.hit,
                                        handler == s2ne::OriginalSkills::Handler::fissure467625 && outcome.hit });
                                }
                                const auto seed = stream.seed_state(), calls = stream.draw_count();
                                swordFeedbackPending = prologue->begin_damage_area_feedback(unsigned(selected), presentation, 0,
                                    [&, selected, a = std::move(a), victims = std::move(victims), inventory = std::move(drainInventory), indices, seed, calls, pendingMpGrowth]() mutable
                                    {
                                        smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                                        a.animation_action = 0;
                                        prologue->events.units.at(size_t(selected)) = std::move(a);
                                        for (size_t n = 0; n < indices.size(); ++n)
                                        {
                                            prologue->events.units.at(indices[n]) = std::move(victims[n]);
                                            const auto& unit = prologue->events.units.at(indices[n]);
                                            if (unit.status_flags & 0x20) { prologue->events.release_summon_slot(unit); }
                                        }
                                        rawSave->groups[0].inventory = std::move(inventory);
                                        prologue->events.rebuild_army_modes(0);
                                        prologue->battle().set_hostile_resources(prologue->events);
                                        prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                            {
                                                live.restore_stream(seed, calls);
                                                return true;
                                            });
                                        prologue->battle().commit_external_action();
                                        skillCastVerified = true;
                                        *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                        std::cout << "Original467625 drain feedback/payment/reward published once\n";
                                    });
                                if (!swordFeedbackPending) { s2ne::fatal("467625 drain feedback staging rejected"); }
                                return false;
                            }
                            const bool revealArmy = result.reveal_army;
                            if (skillMenu->spirit45_supported() && result.hit)
                            {
                                const auto& viewed = prologue->events.units.at(target);
                                const auto camera = prologue->camera();
                                s2ne::OriginalMenu sightFont(root);
                                auto panelTarget = viewed;
                                if (options.skillInputSmoke && options.skillControl == "panelfail") { panelTarget.appearance = 0; }
                                auto background = prologue->prepare_spirit_sight_background();
                                if (!background)
                                {
                                    s2ne::report_error(background.error());
                                    return false;
                                }
                                auto indices = std::move(background.value().second);
                                auto panel = s2ne::OriginalSpiritSight(root).draw(std::move(background.value().first), sightFont, prologue->events, panelTarget,
                                    viewed.x * 64 + (viewed.y & 1) * 32 - camera.first, &indices);
                                if (!panel)
                                {
                                    s2ne::report_error(panel.error());
                                    return false;
                                }
                                spiritSightImage = std::move(panel).value();
                                spiritSightIndices = std::move(indices);
                                prologue->spirit_sight_pending = true;
                                smokeDriver.state.spiritSightActive = true;
                                const auto seed = stream.seed_state(), calls = stream.draw_count();
                                spiritSightCommit = [&, selected, a = std::move(a), seed, calls, pendingMpGrowth]() mutable
                                {
                                    prologue->events.units.at(size_t(selected)) = std::move(a);
                                    smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                                    prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                        {
                                            live.restore_stream(seed, calls);
                                            return true;
                                        });
                                    prologue->battle().commit_external_action();
                                    skillCastVerified = true;
                                    *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                };
                                swordFeedbackPending = true;
                                showPrologue();
                                return false;
                            }
                            std::optional<s2ne::OriginalMap> preparedInvisibility;
                            if (revealArmy)
                            {
                                auto prepared = prologue->prepare_army_reveal_world(a.hostile(), options.skillInputSmoke && options.skillControl == "scratchfail");
                                if (!prepared)
                                {
                                    s2ne::report_error(prepared.error());
                                    return false;
                                }
                                preparedInvisibility = std::move(prepared).value();
                            }
                            if (skillMenu->invisibility47_supported())
                            {
                                auto staged = prologue->events.units;
                                for (size_t n = 0; n < indices.size(); ++n) { staged[indices[n]] = victims[n]; }
                                if (options.skillInputSmoke && options.skillControl == "scratchfail" && !indices.empty()) { staged[indices.back()].animation_action = 8; }
                                auto prepared = prologue->prepare_invisibility_scratch(staged, indices);
                                if (!prepared)
                                {
                                    s2ne::report_error(prepared.error());
                                    return false;
                                }
                                preparedInvisibility = std::move(prepared).value();
                            }
                            std::cout << "AREA production skill=" << skillMenu->selected().id << " center=" << center.x << "," << center.y << " targets=" << indices.size() << " stagedRNG=" << stream.draw_count() - rng.draw_count() << " unpublished=1\n";
                            for (size_t n = 0; n < indices.size(); ++n) { std::cout << "AREA target=" << victims[n].id << " xy=" << victims[n].x << "," << victims[n].y << " feedback=" << int(result.target_hits[n]) << " status3e=" << unsigned(victims[n].status3e) << " status3f=" << unsigned(victims[n].status3f) << '\n'; }
                            std::vector<std::pair<size_t, bool>> feedback;
                            for (size_t i = 0; i < indices.size(); ++i) { feedback.emplace_back(indices[i], result.target_hits[i]); }
                            if (revealArmy && feedback.empty()) { feedback.emplace_back(size_t(selected), false); }
                            auto seed = stream.seed_state(), calls = stream.draw_count();
                            swordFeedbackPending = prologue->battle().begin_status_area_feedback(size_t(selected), std::move(feedback), prologue->events.units, [&, selected, a = std::move(a), victims = std::move(victims), indices, seed, calls, pendingMpGrowth, partnerIndex, partner = std::move(partner), preparedInvisibility = std::move(preparedInvisibility), revealArmy]() mutable
                                {
                                    smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                                    prologue->events.units.at(size_t(selected)) = std::move(a);
                                    if (partnerIndex && partner) { prologue->events.units.at(*partnerIndex) = std::move(*partner); }
                                    for (size_t i = 0; i < indices.size(); ++i) { prologue->events.units.at(indices[i]) = std::move(victims[i]); }
                                    prologue->events.rebuild_army_modes(0);
                                    prologue->battle().set_hostile_resources(prologue->events);
                                    if (preparedInvisibility) { prologue->commit_invisibility_scratch(std::move(*preparedInvisibility)); }
                                    if (revealArmy)
                                    {
                                        if (!prologue->events.army_modes) { prologue->events.army_modes = std::array<uint32_t, 2>{}; }
                                        (*prologue->events.army_modes)[prologue->events.units.at(size_t(selected)).hostile() ? 1 : 0] |= 1;
                                        prologue->battle().set_hostile_resources(prologue->events);
                                    }
                                    prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                        {
                                            live.restore_stream(seed, calls);
                                            return true;
                                        });
                                    prologue->battle().commit_external_action();
                                    skillCastVerified = true;
                                    *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                    std::cout << "Original area feedback complete; all statuses/timers/payment/reward/RNG published once targets=" << indices.size() << '\n';
                                },
                                healing || status || revealArmy, result.target_amounts, healing);
                            return false;
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
                    sleepCenter.reset();
                    skillTarget.reset();
                    transferSource.reset();
                    transferDestination.reset();
                    showPrologue();
                }
                if (!spiritSightImage && rawSave && prologue->in_battle() && !prologue->battle().animating_movement() && !prologue->battle().combat_feedback_active())
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
                float x{}, y{}, rx{}, ry{};
                SDL_GetMouseState(&x, &y);
                bool edgeActive = !spiritSightImage && !battleMenu && !battleReports && !promotionMenu && !itemMenu && !saveMenu && !overwriteConfirmation && !exitConfirmation && !options.battleSmoke && !options.skillInputSmoke && prologue->in_battle() && !prologue->battle().animating_movement() && (prologue->battle_state() == s2ne::Battle::State::idle || prologue->battle_state() == s2ne::Battle::State::movement || prologue->battle_state() == s2ne::Battle::State::attack_target) && SDL_GetMouseFocus() == window && SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry) && rx >= 0 && rx < 640 && ry >= 0 && ry < 480 && (rx < 10 || rx > 630 || ry < 10 || ry > 470);
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
                if (spiritSightImage && event.type == SDL_EVENT_KEY_DOWN && (event.key.mod & SDL_KMOD_SHIFT)) { continue; }
                if (spiritSightImage && event.type == SDL_EVENT_KEY_DOWN && (event.key.mod & SDL_KMOD_ALT) && event.key.key == SDLK_C)
                {
                    if (!event.key.repeat)
                    {
                        auto saved = s2ne::OriginalSpiritSight(root).save_screenshot(isolatedSaveRoot ? *isolatedSaveRoot : root, *spiritSightImage, screenshotSequence, spiritSightIndices);
                        if (!saved) { s2ne::report_error(saved.error()); }
                    }
                    continue;
                }
                if (spiritSightImage && ((event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                {
                    spiritSightImage.reset();
                    spiritSightIndices.clear();
                    prologue->spirit_sight_pending = false;
                    smokeDriver.state.spiritSightActive = false;
                    auto commit = std::move(spiritSightCommit);
                    if (commit) { commit(); }
                    showPrologue();
                    continue;
                }
                if (spiritSightImage && event.type != SDL_EVENT_QUIT) { continue; }
                if (firstInput && (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                {
                    startup.mark("first-input-dispatched");
                    firstInput = false;
                }
                if (firstEvent)
                {
                    startup.mark("first-event");
                    firstEvent = false;
                }
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
                    if (event.type == SDL_EVENT_KEY_DOWN && (!event.key.repeat || innMenu->world_active()))
                    {
                        unsigned code = 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: code = 1; break;
                        case SDLK_UP: code = 0x48; break;
                        case SDLK_DOWN: code = 0x50; break;
                        case SDLK_LEFT: code = 0x4B; break;
                        case SDLK_RIGHT: code = 0x4D; break;
                        case SDLK_HOME: code = 0x47; break;
                        case SDLK_END: code = 0x4F; break;
                        case SDLK_PAGEUP: code = 0x49; break;
                        case SDLK_PAGEDOWN: code = 0x51; break;
                        case SDLK_TAB: code = 0x0F; break;
                        case SDLK_RETURN: code = 0x1C; break;
                        case SDLK_SPACE: code = 0x39; break;
                        }
                        bool enteringWorld = !innMenu->world_active();
                        innMenu->scan(code, *rawSave);
                        if (enteringWorld && innMenu->world_active())
                        {
                            innMenu->enable_world_presentation();
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{}, ry{};
                        if (SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry))
                        {
                            bool enteringWorld = !innMenu->world_active();
                            innMenu->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT, *rawSave);
                            if (enteringWorld && innMenu->world_active())
                            {
                                innMenu->enable_world_presentation();
                            }
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
                    if (innMenu->title_requested())
                    {
                        innMenu.reset();
                        prologue.reset();
                        rawSave.reset();
                        connectedCampaign = nullptr;
                        autosaveBlocked = false;
                        inSetup = false;
                        showMenu();
                        continue;
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
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{}, ry{};
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
                if (prologue && (prologue->spell_summon_active() || prologue->teleport_active() || prologue->evacuation_active()))
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
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x, y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y, rx{}, ry{};
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
                        float rx{}, ry{};
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
                            if (skillMenu->transfer51_supported() && transferSource)
                            {
                                transferSource.reset();
                                transferDestination.reset();
                                showPrologue();
                                continue;
                            }
                            skillMenu.reset();
                            transferSource.reset();
                            transferDestination.reset();
                            sleepCenter.reset();
                            skillTarget.reset();
                            showPrologue();
                            continue;
                        }
                        if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                        {
                            int scan;
                            if (event.key.key == SDLK_LEFT) { scan = 0x4B; }
                            else if (event.key.key == SDLK_RIGHT) { scan = 0x4D; }
                            else if (event.key.key == SDLK_UP) { scan = 0x48; }
                            else if (event.key.key == SDLK_DOWN) { scan = 0x50; }
                            else
                            {
                                scan = 0;
                            }

                            if (scan)
                            {
                                skillMenu->navigate(scan);
                                sleepCenter.reset();
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
                                if ((skillMenu->healing1_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i])) || skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i]))
                                {
                                    skillTarget = i;
                                    sleepCenter.reset();
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
                            if (skillMenu->transfer51_supported() && transferSource)
                            {
                                transferSource.reset();
                                transferDestination.reset();
                                showPrologue();
                                continue;
                            }
                            skillMenu.reset();
                            transferSource.reset();
                            transferDestination.reset();
                            sleepCenter.reset();
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
                            if (skillMenu->transfer51_supported() && transferSource)
                            {
                                const auto destinations = prologue->battle().spell_transfer_destinations(prologue->events.units.at(size_t(selected)), prologue->events.units.at(*transferSource), prologue->events.units);
                                if (std::find(destinations.begin(), destinations.end(), cell) != destinations.end())
                                {
                                    transferDestination = cell;
                                    skillTarget = transferSource;
                                    confirm = click && event.button.button == SDL_BUTTON_LEFT;
                                }
                            }
                            else if (skillMenu->status_area_supported())
                            {
                                auto center = prologue->events.units.at(size_t(selected));
                                center.x = cell.x;
                                center.y = cell.y;
                                if (skillMenu->sleep2_center_in_range(prologue->events.units.at(size_t(selected)), center))
                                {
                                    for (size_t i = 0; i < prologue->events.units.size(); ++i)
                                    {
                                        const auto& u = prologue->events.units[i];
                                        if (skillMenu->status_area_target(center, u) && u.present(prologue->battle_turn()) && skillMenu->sleep2_area(center, u) && prologue->battle().skill_area_cell({ u.x, u.y }) && prologue->battle().target_visible(u, std::span(&center, 1), 0))
                                        {
                                            sleepCenter = cell;
                                            skillTarget = i;
                                            confirm = click && event.button.button == SDL_BUTTON_LEFT;
                                            break;
                                        }
                                    }
                                }
                            }
                            else if (target >= 0 && ((skillMenu->selected().id == 88 && target == selected) || skillMenu->healing1_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[size_t(target)]) || skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[size_t(target)])))
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
                            const auto& actor = prologue->events.units.at(size_t(selected));
                            auto partner = skillMenu->cooperative_partner(actor, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                {
                                    return prologue->battle().cell({ x, y }).elevation;
                                });
                            const s2ne::OriginalEvents::Unit* partnerUnit = nullptr;
                            if (partner) { partnerUnit = &prologue->events.units[*partner]; }
                            bool summonReady = false;
                            if (skillMenu->summon_supported() && prologue->events.army_status)
                            {
                                const auto& slots = (*prologue->events.army_status)[actor.hostile() ? 1 : 0];
                                summonReady = true;
                                for (unsigned i = 0; i < 11; ++i)
                                {
                                    if (slots[i * 3] == 1) { summonReady = false; }
                                }
                            }
                            auto failure = skillMenu->select(actor, partnerUnit, summonReady);
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
                            else if (!skillMenu->summon_supported() && !skillMenu->transfer51_supported() && !skillMenu->hostile_supported() && !skillMenu->sword94_supported() && !skillMenu->curse33_supported() && !skillMenu->status_area_supported() && !skillMenu->effect_supported(skillMenu->selected()))
                            {
                                std::cout << "Original selected spell executor unavailable id=" << skillMenu->selected().id << '\n';
                                skillMenu->resume();
                            }
                            else if (skillMenu->summon_supported() || skillMenu->selected().id == 88)
                            {
                                skillTarget = size_t(selected);
                            }
                            else
                            {
                                skillTarget.reset();
                                for (size_t i = 0; i < prologue->events.units.size(); ++i)
                                {
                                    if ((skillMenu->healing1_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i])) || skillMenu->hostile_in_range(prologue->events.units.at(size_t(selected)), prologue->events.units[i]))
                                    {
                                        skillTarget = i;
                                        break;
                                    }
                                }
                            }
                        }
                        else if (skillTarget)
                        {
                            if (skillMenu->transfer51_supported())
                            {
                                if (!transferSource)
                                {
                                    transferSource = skillTarget;
                                    transferDestination.reset();
                                    showPrologue();
                                    continue;
                                }
                                if (!transferDestination) { continue; }
                            }
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
                            if (skillMenu->status_area_supported() && !sleepCenter) { sleepCenter = skillMenu->selected().raw[18] == 7 ? s2ne::Battle::Point{ caster.x, caster.y } : s2ne::Battle::Point{ target.x, target.y }; }
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
                        int mouseButtons;
                        if (event.button.button == SDL_BUTTON_LEFT) { mouseButtons = 1; }
                        else if (event.button.button == SDL_BUTTON_RIGHT) { mouseButtons = 2; }
                        else
                        {
                            mouseButtons = 0;
                        }

                        input.buttons = mouseButtons;
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
                        int mouseButtons;
                        if (down)
                        {
                            if (event.button.button == SDL_BUTTON_LEFT) { mouseButtons = 1; }
                            else if (event.button.button == SDL_BUTTON_RIGHT) { mouseButtons = 2; }
                            else
                            {
                                mouseButtons = 0;
                            }
                        }
                        else
                        {
                            mouseButtons = 0;
                        }

                        input.buttons = mouseButtons;
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
                            auto created = s2ne::Prologue::create(root, restored.events.difficulty);
                            if (!created)
                            {
                                s2ne::report_error(created.error());
                                saveMenu->complete(false);
                                continue;
                            }
                            auto loaded = std::move(created).value();
                            auto domain = s2ne::try_app_domain_for_save(restored.raw);
                            if (!domain)
                            {
                                s2ne::report_error(domain.error());
                                saveMenu->complete(false);
                                continue;
                            }
                            loaded->set_resource_domain(domain.value());
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
                        int mouseButtons;
                        if (down)
                        {
                            if (event.button.button == SDL_BUTTON_LEFT) { mouseButtons = 1; }
                            else if (event.button.button == SDL_BUTTON_RIGHT) { mouseButtons = 2; }
                            else
                            {
                                mouseButtons = 0;
                            }
                        }
                        else
                        {
                            mouseButtons = 0;
                        }

                        input.buttons = mouseButtons;
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
                        int mouseButtons;
                        if (down)
                        {
                            if (event.button.button == SDL_BUTTON_LEFT) { mouseButtons = 1; }
                            else if (event.button.button == SDL_BUTTON_RIGHT) { mouseButtons = 2; }
                            else
                            {
                                mouseButtons = 0;
                            }
                        }
                        else
                        {
                            mouseButtons = 0;
                        }

                        input.buttons = mouseButtons;
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
                        int mouseButtons;
                        if (down)
                        {
                            if (event.button.button == SDL_BUTTON_LEFT) { mouseButtons = 1; }
                            else if (event.button.button == SDL_BUTTON_RIGHT) { mouseButtons = 2; }
                            else
                            {
                                mouseButtons = 0;
                            }
                        }
                        else
                        {
                            mouseButtons = 0;
                        }

                        input.buttons = mouseButtons;
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
                        s2ne::OriginalBattleReports::Page page;
                        if (request.action == s2ne::OriginalBattleMenu::Action::conditions) { page = s2ne::OriginalBattleReports::Page::conditions; }
                        else if (request.action == s2ne::OriginalBattleMenu::Action::inspect_map) { page = s2ne::OriginalBattleReports::Page::map; }
                        else
                        {
                            page = s2ne::OriginalBattleReports::Page::roster;
                        }

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
                        if ((options.skill94Input || NativeDamageRecords::ordinary_sword(options.inputSkill)) && click)
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
                        innMenu->enable_presentation_timing();
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
                    auto transitioned = initialization.try_transition_stage(root, *rawSave, next);
                    if (!transitioned)
                    {
                        s2ne::report_error(transitioned.error());
                        return EXIT_FAILURE;
                    }
                    auto staged = std::move(transitioned).value();
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
                auto initialized = initialization.try_fresh(root, prologue->events.stage, prologue->events.difficulty, prologue->events.flags, s2ne::OriginalInitialization::setup_from_settings(settingsState, 0));
                if (!initialized)
                {
                    s2ne::report_error(initialized.error());
                    return EXIT_FAILURE;
                }
                auto staged = std::move(initialized).value();
                auto restoredResult = s2ne::OriginalSaveBridge::try_restore(root, staged);
                if (!restoredResult)
                {
                    s2ne::report_error(restoredResult.error());
                    return EXIT_FAILURE;
                }
                auto restored = std::move(restoredResult).value();
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
                            return skillRules.executable_available(unit, {}, true);
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
                s2ne::OriginalSkills::Context context{};
                context.second = prologue->battle_requested_command() == 7;
                prologue->clear_battle_requested_command();
                skillMenu.emplace(root);
                skillMenu->set_area_camera(prologue->camera());
                skillMenu->set_area_elevation([&](int x, int y)
                    {
                        return prologue->battle().cell({ x, y }).elevation;
                    });
                skillMenu->set_area_valid([&](int x, int y)
                    {
                        return prologue->battle().skill_area_cell({ x, y });
                    });
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
                auto rendered = saveMenu->try_draw(image, menu);
                if (!rendered)
                {
                    s2ne::report_error(rendered.error());
                    saveMenu.reset();
                }
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
            if (firstPresent)
            {
                startup.mark("first-present");
                firstPresent = false;
            }
            if (innMenu && innMenu->world_active())
            {
                innMenu->frame_presented();
                showPrologue();
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
                    int visualFrame;
                    if ((options.inputSkill >= 64 && options.inputSkill <= 74) || NativeDamageRecords::ordinary_sword(options.inputSkill) || options.inputSkill == 29 || options.inputSkill == 18 || options.inputSkill == 40 || options.inputSkill == 45 || options.inputSkill == 50 || options.inputSkill == 51) { visualFrame = prologue->skill88_visual_frame(); }
                    else if (options.skill94Input) { visualFrame = prologue->skill94_visual_frame(); }
                    else if ((options.skill88Input || options.inputSkill == 1 || (NativeAbilityBuffs::find(options.inputSkill) || NativeTimedBuffs::find(options.inputSkill) || (options.inputSkill >= 2 && options.inputSkill <= 10) || options.inputSkill == 8 || options.inputSkill == 12 || options.inputSkill == 35 || options.inputSkill == 39 || options.inputSkill == 48 || options.inputSkill == 56 || options.inputSkill == 60 || options.inputSkill == 17 || options.inputSkill == 25 || options.inputSkill == 52 || options.inputSkill == 61 || options.inputSkill == 13 || options.inputSkill == 15 || options.inputSkill == 53 || options.inputSkill == 34 || options.inputSkill == 38 || options.inputSkill == 54 || options.inputSkill == 96 || NativeTimedBuffs::find(options.inputSkill)))) { visualFrame = prologue->skill88_visual_frame(); }
                    else
                    {
                        visualFrame = prologue->skill32_visual_frame();
                    }
                    int frame = (visualFrame);
                    if (frame >= 0 && frame != previous)
                    {
                        ++smokeDriver.state.skillInputFrames;
                        int visualResource;
                        if (options.skill94Input) { visualResource = 85; }
                        else if (options.skill88Input) { visualResource = 79; }
                        else if (options.inputSkill == 29 || options.inputSkill == 18 || options.inputSkill == 40) { visualResource = int(options.inputSkill); }
                        else if (options.inputSkill == 45) { visualResource = 45; }
                        else if (options.inputSkill == 50) { visualResource = 51; }
                        else if (options.inputSkill == 51) { visualResource = 52; }
                        else if (options.inputSkill >= 64 && options.inputSkill <= 74) { visualResource = 65; }
                        else if ((NativeAbilityBuffs::find(options.inputSkill) || NativeTimedBuffs::find(options.inputSkill) || (options.inputSkill >= 2 && options.inputSkill <= 10) || options.inputSkill == 8 || options.inputSkill == 12 || options.inputSkill == 35 || options.inputSkill == 39 || options.inputSkill == 48 || options.inputSkill == 56 || options.inputSkill == 60 || options.inputSkill == 17 || options.inputSkill == 25 || options.inputSkill == 52 || options.inputSkill == 61 || options.inputSkill == 13 || options.inputSkill == 15 || options.inputSkill == 53 || options.inputSkill == 34 || options.inputSkill == 38 || options.inputSkill == 54 || options.inputSkill == 96 || NativeTimedBuffs::find(options.inputSkill))) { visualResource = 3; }
                        else if (options.inputSkill == 1) { visualResource = 2; }
                        else if (options.inputSkill == 33) { visualResource = 34; }
                        else
                        {
                            visualResource = 33;
                        }
                        std::cout << "SDL SPELLSP" << (visualResource)
                                  << " frame=" << frame << " ticks=" << SDL_GetTicks() << '\n';
                    }
                    previous = frame;
                    smokeDriver.state.skillSequence << presented << ',' << SDL_GetTicks() << ',' << smokeDriver.state.skillInputPhase << ',' << prologue->battle().animating_movement() << ',' << frame << ',' << smokeDriver.state.skillInputImageHash << '\n';
                }
                prologue->frame_presented();
            }
            ++presented;
            if (stdioPending && SDL_GetTicks() >= stdioReadyAt)
            {
                (*stdioPending)["state"] = stdioObserve();
                stdio->respond(*stdioPending);
                stdioPending.reset();
            }
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
