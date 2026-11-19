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
#include "original_ani_audio.hpp"
#include "original_battle_menu.hpp"
#include "original_battle_reports.hpp"
#include "original_campaign_persistence.hpp"
#include "original_church.hpp"
#include "original_confirmation.hpp"
#include "original_deployment.hpp"
#include "original_evacuation_smoke.hpp"
#include "original_growth_panel.hpp"
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
#include "original_spell_summon_candidate.hpp"
#include "original_spirit_sight.hpp"
#include "original_teleport_smoke.hpp"
#include "prologue.hpp"
#include "runtime_diagnostics.hpp"
#include "simple_cc.hpp"
#include "startup_trace.hpp"
#include "stdio_events.hpp"
#include "stdio_protocol.hpp"
#include "title_image.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_messagebox.h>
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
    SDL_Texture* ownedPresentationTexture{};
    struct PresentationTextureRestore
    {
        SDL_Texture*& current;
        SDL_Texture*& owned;
        ~PresentationTextureRestore() { current = owned; }
    } presentationTextureRestore{ texture, ownedPresentationTexture };
    struct TextureCleanup
    {
        ~TextureCleanup() { s2ne::texture_resources.clear(); }
    } textureCleanup;
    {
        const s2ne::AppOptions options(argc, argv);
        s2ne::texture_resources.configure(std::filesystem::current_path());
        const bool diagnostics = options.stdio || options.option;
        s2ne::configure_terminal_output(diagnostics);
        uint64_t headlessTicks = 1;
        auto gameTicks = [&]() -> uint64_t
        {
            if (options.headlessSpeed) { return headlessTicks; }
            return SDL_GetTicks();
        };
        std::optional<s2ne::StdioProtocol> stdio;
        if (options.stdio) { stdio.emplace(); }
        s2ne::AppSmokeDriver smokeDriver(options);
        if (options.strengthenedFlow)
        {
            std::print("NON-ORIGINAL strengthened flow acceptance: allied HP/attack/defense boosted, MP capacity floored400 and currentMP replenished (NON-ORIGINAL); not balance or legitimate clear evidence\n");
        }
        if (options.campaignContinue)
        {
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
        if (options.verify)
        {
            std::print("Decoded original title PCX: 640x480\n");
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
                std::print("Decoded original intro: {}x{}, {} frames, delay {} ms\n", clip.width, clip.height, frames, clip.delay);
            }
            return 0;
        }
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            s2ne::fatal(SDL_GetError());
        }
        if (options.headlessSpeed && (!SDL_GetCurrentVideoDriver() || std::string_view(SDL_GetCurrentVideoDriver()) != "dummy"))
        {
            s2ne::fatal("--headless-speed requires SDL dummy video");
        }
        s2ne::OpeningAudio audio;
        audio.set_diagnostics(diagnostics);
        startup.mark("sdl-init");
        bool audioAvailable = false;
        if (!SDL_CreateWindowAndRenderer("Northern Envoy - original intro and menu", 960, 720,
                SDL_WINDOW_RESIZABLE, &window, &renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480);
        ownedPresentationTexture = texture;
        s2ne::texture_resources.bind(renderer);
        auto setupCheck = setup.draw(title, menu);
        auto menuCheck = title;
        menu.draw(menuCheck);
        startup.mark("trial-draw");
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
        if (options.headlessSpeed && (!SDL_GetCurrentAudioDriver() || std::string_view(SDL_GetCurrentAudioDriver()) != "dummy"))
        {
            s2ne::fatal("--headless-speed requires SDL dummy audio");
        }
        auto audioStart = audio.start({});
        audioAvailable = bool(audioStart);
        if (audioAvailable)
        {
            auto stopped = audio.stop_music();
            if (!stopped) { s2ne::report_error(stopped.error()); }
        }
        startup.mark("audio-start");
        if (!audioStart)
        {
            s2ne::report_error(audioStart.error());
            std::print(stderr, "Audio unavailable; continuing without sound\n");
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
        std::optional<s2ne::OriginalAniAudio> aniAudio;
        unsigned aniFrame = 1;
        auto stopEffects = [&]() -> s2ne::Result<bool>
        {
            if (!audio.effects()) { return s2ne::Result<bool>::success(true); }
            return audio.effects()->stop();
        };
        auto playAni = [&](unsigned id, unsigned channel) -> s2ne::Result<unsigned>
        {
            if (!audio.effects()) { return s2ne::Result<unsigned>::failure(s2ne::make_error("ANI effects unavailable")); }
            auto result = audio.effects()->play(id, channel);
            if (diagnostics && result && !channel) { std::print(stderr, "Original ANI effect id={} channel={}\n", id, result.value()); }
            return result;
        };
        auto showMenu = [&]
        {
            if (aniAudio)
            {
                auto stopped = aniAudio->finish(stopEffects);
                if (!stopped) { s2ne::report_error(stopped.error()); }
            }
            SDL_ShowCursor();
            inMenu = true;
            paused = true;
            auto image = title;
            menu.draw(image);
            if (stdio) { stdioLogicalFrame = image; }
            if (s2ne::texture_resources.active())
            {
                auto uploaded = s2ne::texture_resources.upload_frame(image);
                if (!uploaded) { s2ne::fatal(uploaded.error()); }
                texture = uploaded.value();
            }
            else if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
        };
        auto showSetup = [&]
        {
            inSetup = true;
            auto image = setup.draw(title, menu);
            if (stdio) { stdioLogicalFrame = image; }
            if (s2ne::texture_resources.active())
            {
                auto uploaded = s2ne::texture_resources.upload_frame(image);
                if (!uploaded) { s2ne::fatal(uploaded.error()); }
                texture = uploaded.value();
            }
            else if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
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
        std::optional<s2ne::OriginalDeployment> deployment;
        std::optional<s2ne::OriginalPromotion> promotionMenu;
        std::optional<s2ne::OriginalArmyEquipmentMenu> battleItems;
        s2ne::OriginalItemUse itemUse(root);
        std::optional<std::pair<size_t, unsigned>> itemTargetRequest;
        std::optional<unsigned> itemSpellSource;
        unsigned itemNoticeMilliseconds{};
        std::vector<std::vector<uint8_t>> itemLearningNotices;
        uint64_t itemLearningSince{};
        std::function<void()> itemLearningPublish;
        s2ne::OriginalShopMenu itemNoticeGraphics(root);
        s2ne::OriginalMenu itemNoticeFont(root);
        auto publishItemSpell = [&]()
        {
            if (!itemSpellSource || !rawSave) { return; }
            const unsigned id = *itemSpellSource;
            if (itemUse.targeted(id)) { --rawSave->groups[0].inventory.at(id - 1); }
            itemSpellSource.reset();
            battleItems.reset();
        };
        std::vector<unsigned> usableItems;
        size_t usableItemSelection{};
        bool itemMenu = false, itemConfirmation = false;
        std::optional<s2ne::OriginalSkills> skillMenu;
        std::optional<size_t> skillTarget;
        struct SkillGrowthPause
        {
            s2ne::OriginalProgression::PendingGrowth pending;
            uint32_t seconds;
        };
        struct SkillGrowthAward
        {
            s2ne::OriginalEvents::Unit unit;
            s2ne::OriginalRandom::State random;
        };
        std::vector<SkillGrowthAward> skillGrowthAwards;
        std::vector<SkillGrowthPause> skillGrowthNotices;
        std::optional<uint32_t> skillGrowthSeconds;
        std::optional<s2ne::OriginalSpellSummonCandidate> skillGrowthSummon;
        std::optional<size_t> pendingSkillTarget;
        std::optional<s2ne::Battle::Point> sleepCenter;
        std::optional<size_t> transferSource;
        std::optional<s2ne::Battle::Point> transferDestination;
        bool pendingSkillVisualStarted = false;
        bool aiSkillActive = false;
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
            const bool finalBattle = (prologue->events.stage == 61 || prologue->events.stage == 131) && prologue->events.flags[12];
            const bool nataliaDeparture = prologue->events.stage == 130 && prologue->events.flags[13] && prologue->events.flags[3] && prologue->events.type(28) == 0 && prologue->events.type(31) == 0;
            if ((finalBattle || nataliaDeparture) && s2ne::OriginalCampaign(root).selected_route(prologue->events) == 0 && std::any_of(prologue->events.flags.begin() + 71, prologue->events.flags.end(), [](auto f)
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
                    std::print("CONTROLLED cooperative explicit adjacent partner={} learned/coordinates/MP/AP injected; NOT natural\n", partner->id);
                }
                std::print("CONTROLLED hostile status explicit learned bit injection on actor1; NOT natural learning; source SAVE read-only\n");
            }
            prologue->restore_battle(std::move(restored.events), restored.turn, !controlledHostileStatus);
            if (controlledHostileStatus) { std::print("CONTROLLED hostile status battle-only restore: story-stage continuation suppressed; NOT natural campaign entry\n"); }
            rawSave = std::move(restored.raw);
            paused = true;
            std::filesystem::create_directories(root / "S2NE" / "out" / options.inputFolder);
            smokeDriver.state.skillSequence.open(root / "S2NE" / "out" / options.inputFolder / "sequence.csv");
            smokeDriver.state.skillSequence << "present,ticks,phase,preparation,spell_frame,hash\n";
            for (unsigned i = 0; i < prologue->events.units.size(); ++i)
            {
                const auto& u = prologue->events.units[i];
                if (u.controllable() && (u.learned[2] & 0x800))
                {
                    std::print("Learned candidate index={} id={} xy={},{} MP={} AP={} HP={} action={}\n", i, u.id, u.x, u.y, u.mp, u.action_points, u.hp, unsigned(u.action_state));
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
        const bool simplifiedChinese = settingsState.simplified_chinese != 0;
        if (std::filesystem::exists(root / "chinese.ttf"))
        {
            auto font = s2ne::texture_resources.initialize_font(root / "chinese.ttf");
            if (!font) { s2ne::fatal(font.error()); }
            auto smallFont = s2ne::texture_resources.initialize_small_font(root / "chinese.ttf");
            if (!smallFont) { s2ne::fatal(smallFont.error()); }
        }
        menu.set_simplified_chinese(simplifiedChinese, root / "cc");
        auto playMusic = [&](unsigned id) -> s2ne::Result<bool>
        {
            auto domain = s2ne::ResourceDomain::primary;
            if (prologue) { domain = prologue->music_domain(); }
            auto current = settingsState;
            if (settings) { current = settings->state(); }
            if (battleMenu) { current = battleMenu->settings_state(); }
            return audio.play_music(root, id, domain, !current.cdda);
        };
        auto playMidi = [&](unsigned id) -> s2ne::Result<bool>
        {
            auto domain = s2ne::ResourceDomain::primary;
            if (prologue) { domain = prologue->music_domain(); }
            return audio.play_music(root, id, domain, true);
        };
        auto initialMusicVolume = audio.music_volume(settingsState.cdda_volume);
        if (!initialMusicVolume) { s2ne::report_error(initialMusicVolume.error()); }
        if (audioAvailable)
        {
            auto started = audio.start_effects(root);
            if (!started) { s2ne::report_error(started.error()); }
            else if (!paused && options.stdioInitialSave.empty())
            {
                auto loaded = s2ne::OriginalAniAudio::create(root);
                if (!loaded) { s2ne::report_error(loaded.error()); }
                else
                {
                    aniAudio = std::move(loaded).value();
                    aniAudio->play_music = playMusic;
                    aniAudio->play_midi = playMidi;
                    auto begun = aniAudio->begin(1, 0x400);
                    if (!begun) { s2ne::report_error(begun.error()); }
                    auto volume = audio.effects()->volume(settingsState.effect_volume);
                    if (!volume) { s2ne::report_error(volume.error()); }
                    auto advanced = aniAudio->advance(1, playAni, stopEffects);
                    if (!advanced) { s2ne::report_error(advanced.error()); }
                }
            }
        }
        auto playEffect = [&](unsigned id)
        {
            if (!audio.effects()) { return; }
            int gain = settingsState.effect_volume;
            if (settings) { gain = settings->state().effect_volume; }
            if (battleMenu) { gain = battleMenu->settings_state().effect_volume; }
            if (audio.is_muted()) { gain = 0; }
            auto volume = audio.effects()->volume(gain);
            if (!volume)
            {
                s2ne::report_error(volume.error());
                return;
            }
            auto played = audio.effects()->play(id);
            if (!played) { s2ne::report_error(played.error()); }
            else
            {
                if (diagnostics) { std::print(stderr, "Original effect id={} channel={}\n", id, played.value()); }
            }
        };
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
            if (stdio) { stdioLogicalFrame = image; }
            if (s2ne::texture_resources.active())
            {
                auto uploaded = s2ne::texture_resources.upload_frame(image);
                if (!uploaded) { s2ne::fatal(uploaded.error()); }
                texture = uploaded.value();
            }
            else if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
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
        auto confirmExit = [&]()
        {
            const SDL_MessageBoxButtonData buttons[] = {
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel" }
            };
            const SDL_MessageBoxData data = {
                SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT,
                window,
                "S2NE",
                "Exit the game?",
                2,
                buttons,
                nullptr
            };
            int button = 0;
            if (!SDL_ShowMessageBox(&data, &button))
            {
                s2ne::report_error(s2ne::make_error(SDL_GetError()));
                return false;
            }
            return button == 0;
        };
        bool suppressNextCloseEvent = false;
        auto requestExit = [&](bool fromWindowClose)
        {
            auto stopped = stopEffects();
            if (!stopped) { s2ne::report_error(stopped.error()); }
            if (confirmExit()) { running = false; }
            else if (fromWindowClose) { suppressNextCloseEvent = true; }
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
                innMenu->tick(options.headlessSpeed ? unsigned(gameTicks() / 10) : s2ne::OriginalShopMenu::wall_clock_cs());
            }
            if (shopMenu && rawSave)
            {
                shopMenu->tick(options.headlessSpeed ? unsigned(gameTicks() / 10) : s2ne::OriginalShopMenu::wall_clock_cs(), *rawSave);
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
            if (deployment)
            {
                image = deployment->draw();
                deployment->frame_presented();
            }
            else if (innMenu && rawSave) { image = innMenu->draw(*rawSave); }
            else if (churchMenu && rawSave) { image = churchMenu->draw(*rawSave); }
            else if (shopMenu && rawSave) { image = shopMenu->draw(*rawSave); }
            else if (battleItems && !itemTargetRequest && !itemSpellSource && rawSave) { image = battleItems->draw(*rawSave); }
            else
            {
                image = prologue->draw(menu, [&](auto& ground)
                    {
                        if (!skillMenu || pendingSkillTarget || skillMenu->page() != s2ne::OriginalSkills::Page::requested || prologue->battle_selected() < 0) { return; }
                        auto actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                        auto camera = prologue->camera();
                        prologue->battle().draw_skill_cells(ground, camera.first, camera.second, 0, [&](auto cell)
                            {
                                return skillMenu->selection_cell_in_range(actor, cell.x, cell.y);
                            },
                            [&](auto& pixels, auto cell)
                            {
                                prologue->mask_ground_selection(pixels, cell);
                            });
                        if (sleepCenter && skillMenu->status_area_supported())
                        {
                            auto center = actor;
                            center.x = sleepCenter->x;
                            center.y = sleepCenter->y;
                            prologue->battle().draw_skill_cells(ground, camera.first, camera.second, 1, [&](auto cell)
                                {
                                    auto target = actor;
                                    target.x = cell.x;
                                    target.y = cell.y;
                                    return skillMenu->sleep2_area(center, target);
                                },
                                [&](auto& pixels, auto cell)
                                {
                                    prologue->mask_ground_selection(pixels, cell);
                                });
                        }
                    });
                if (spiritSightImage) { image = *spiritSightImage; }
            }

            // 覆盖层接管画面前，清除战场状态栏、命令提示及上一覆盖层留下的字体命令。
            // 原程序先切换显示页的底图，再由该页重新提交自己的文字；不能让
            // 逻辑帧已清除但 renderer 仍保留的文字跨页复用。
            if (battleMenu || battleReports || promotionMenu || skillMenu || battleItems || itemMenu || saveMenu || overwriteConfirmation)
            {
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
                    prologue->set_battle_menu_overlay(false);
                }
            }
            if (battleReports)
            {
                s2ne::texture_resources.set_elapsed(gameTicks());
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
            if (!itemLearningNotices.empty())
            {
                const int x = 320 - itemNoticeGraphics.panel_width(208) / 2;
                const int y = 240 - itemNoticeGraphics.panel_height(208) / 2;
                itemNoticeGraphics.panel(image, 208, x, y);
                const auto& text = itemLearningNotices.front();
                itemNoticeFont.draw_original_large(image, text, 320 - (int(text.size()) - 2) * 6 - 16, 228, 42);
                if (!itemLearningSince) { itemLearningSince = gameTicks(); }
            }
            if (itemNoticeMilliseconds)
            {
                itemNoticeGraphics.shared_panel(image, 0x4F72C8, 176, 80);
                const auto text = itemNoticeFont.text_bytes(73);
                const unsigned panel = s2ne::OriginalShopMenu::shared_sprite(0x4F72C8);
                itemNoticeFont.draw_original_large(image, text, 176 + itemNoticeGraphics.panel_width(panel) / 2 - int(text.size()) * 6,
                    80 + itemNoticeGraphics.panel_height(panel) / 2 - 12, 42);
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
            if (prologue->experience_notice() > 0)
            {
                auto notice = prologue->draw_experience_notice(menu, image);
                if (!notice)
                {
                    prologue->reject_growth();
                    s2ne::report_error(notice.error());
                    return;
                }
            }
            else if (const auto* learnedName = prologue->learning_notice())
            {
                const auto& unit = prologue->reward_growth().unit();
                auto text = unit.name;
                text.push_back(' ');
                auto label = menu.text_bytes(3);
                text.insert(text.end(), label.begin(), label.end());
                text.push_back(' ');
                text.insert(text.end(), learnedName->begin(), learnedName->end());
                // 原458E61以CEL208显示姓名／原学习标签／技能名，等待473420(250)。
                auto cel = s2ne::read_shared_resource(root / "SUPERSP.CEL");
                auto pal = s2ne::read_resource(root / "SUPERSP.PAL");
                if (!cel || !pal)
                {
                    prologue->reject_growth();
                    if (!cel) { s2ne::report_error(cel.error()); }
                    else
                    {
                        s2ne::report_error(pal.error());
                    }
                    return;
                }
                auto panel = s2ne::draw_original_cel(image, cel.value(), pal.value(), 208, 128, 180, s2ne::OriginalZeroRun::transparent);
                if (panel) { panel = menu.try_draw_original_large(image, text, 144, 198, 42); }
                if (!panel)
                {
                    prologue->reject_growth();
                    s2ne::report_error(panel.error());
                    return;
                }
            }
            if (auto step = prologue->growth_step())
            {
                auto panel = s2ne::OriginalGrowthPanel::draw(root, std::move(image), *step);
                if (!panel)
                {
                    prologue->reject_growth();
                    s2ne::report_error(panel.error());
                    return;
                }
                image = std::move(panel).value();
            }
            if (stdio) { stdioLogicalFrame = image; }
            if (s2ne::texture_resources.active())
            {
                auto uploaded = s2ne::texture_resources.upload_frame(image);
                if (!uploaded) { s2ne::fatal(uploaded.error()); }
                texture = uploaded.value();
            }
            else if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
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
            auto report_background = prologue->draw(menu);
            battleReports.emplace(root, s2ne::OriginalBattleReports::Page::roster, prologue->events, std::move(entries), std::move(report_background), nullptr, settingsState.small_font != 0);
            battleReports->detail();
            battleReports->close_to_battle();
            SDL_ShowCursor();
            showPrologue();
        };
        auto openSelectedPromotion = [&](int hubSelected = -1)
        {
            const bool hub = prologue->interbattle_active() && !churchMenu && !shopMenu && !innMenu;
            if (!rawSave || (!prologue->in_battle() && !hub) || prologue->computer_actions() || prologue->battle().combat_feedback_active() || prologue->battle().animating_movement())
            {
                return;
            }
            int selected = hub ? hubSelected : prologue->battle_selected();
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
                promotionMenu->set_preview_source([&](unsigned target) -> s2ne::Result<s2ne::OriginalPromotion::Preview>
                    {
                        std::optional<s2ne::OriginalPromotion::Preview> preview;
                        std::optional<s2ne::Error> error;
                        const bool applied = prologue->interbattle_random([&](s2ne::OriginalRandom& random, uint32_t seconds)
                            {
                                auto result = promotionMenu->try_preview(target, random, [seconds]
                                    {
                                        return seconds;
                                    });
                                if (!result)
                                {
                                    error = result.error();
                                    return false;
                                }
                                preview = result.value();
                                return true;
                            });
                        if (error) { return s2ne::Result<s2ne::OriginalPromotion::Preview>::failure(*error); }
                        if (!applied || !preview) { return s2ne::Result<s2ne::OriginalPromotion::Preview>::failure(s2ne::make_error("Promotion preview random stream unavailable")); }
                        return s2ne::Result<s2ne::OriginalPromotion::Preview>::success(*preview);
                    });
                playEffect(123);
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
            std::print("Original prologue stage={}, difficulty={}\n", prologue->events.stage, setup.confirmed);
        };
        uint64_t next = gameTicks() + s2ne::original_ani_frame_ms(0xAF12, clips.front().delay);
        const uint64_t started = gameTicks();
        unsigned presented = 0;
        uint64_t lastUiTick = gameTicks();
        s2ne::OriginalEdgeScrollTiming edgeScrollTiming;
        startup.mark("event-loop");
        bool firstPresent = true;
        bool firstEvent = true;
        bool firstInput = true;
        std::optional<s2ne::StdioProtocol::Json> stdioPending;
        uint64_t stdioReadyAt{};
        bool stdioScreenshotPending = false;
        unsigned stdioRemainingFrames{};
        std::deque<s2ne::StdioProtocol::Json> stdioSteps;
        std::optional<s2ne::StdioProtocol::Json> stdioTransaction;
        s2ne::StdioProtocol::Progress stdioProgress;
        std::vector<SDL_Event> stdioInputEvents;
        if (stdio && !options.stdioInitialSave.empty())
        {
            auto initial = s2ne::OriginalSave::load(options.stdioInitialSave, saveItems);
            auto restored = s2ne::OriginalSaveBridge::restore(root, initial);
            prologue = std::make_unique<s2ne::Prologue>(root, restored.events.difficulty);
            if (s2ne::OriginalCampaign(root).stage(restored.events.stage).mode == 3)
            {
                prologue->restore_campaign_story(std::move(restored.events));
            }
            else
            {
                prologue->restore_battle(std::move(restored.events), restored.turn, true, !restored.raw.battle_context());
            }
            rawSave = std::move(restored.raw);
            s2ne::OriginalSaveStore::write(*isolatedSaveRoot, 1, *rawSave, saveItems);
            inMenu = inSetup = false;
            showPrologue();
        }
        auto confirmSpiritSight = [&]()
        {
            if (!spiritSightImage || !prologue) { return false; }
            spiritSightImage.reset();
            spiritSightIndices.clear();
            prologue->spirit_sight_pending = false;
            smokeDriver.state.spiritSightActive = false;
            auto commit = std::move(spiritSightCommit);
            if (commit) { commit(); }
            showPrologue();
            return true;
        };
        auto stdioObserve = [&]()
        {
            using Json = s2ne::StdioProtocol::Json;
            Json state{ { "scene", "opening" }, { "modal", "none" }, { "units", Json::array() }, { "commands", Json::array() }, { "skills", Json::array() } };
            state["music_id"] = audio.music_id();
            state["music_position"] = audio.music_position();
            state["mixed_samples"] = audio.mixed_samples();
            if (battleItems) { state["item_id"] = battleItems->selected_item(); }
            if (inMenu) { state["scene"] = "title"; }
            if (inSetup) { state["scene"] = "setup"; }
            if (inSetup) { state["difficulty"] = setup.difficulty + 1; }
            if (prologue)
            {
                state["scene"] = "prologue";
                state["text_revealing"] = prologue->text_revealing();
                state["prologue_phase"] = prologue->diagnostic_phase();
                state["story_event"] = prologue->diagnostic_event();
                state["story_line"] = prologue->diagnostic_line();
                if (prologue->diagnostic_phase() == 4) { state["briefing"] = { { "encoding", "Big5" }, { "text_bytes", prologue->events.briefing } }; }
                if (prologue->choosing() || prologue->diagnostic_phase() == 0 || prologue->diagnostic_phase() == 2 || prologue->diagnostic_phase() == 3 || prologue->diagnostic_phase() == 16)
                {
                    const auto event = prologue->diagnostic_event();
                    const auto line = prologue->diagnostic_line();
                    if (event < prologue->events.event_count() && line < prologue->events.line_count(event))
                    {
                        const auto dialogue = prologue->events.line(event, line);
                        state["dialogue"] = { { "encoding", "Big5" }, { "speaker", dialogue.speaker }, { "name_bytes", dialogue.name }, { "text_bytes", dialogue.text }, { "page", prologue->page_index() } };
                    }
                }
                state["deployment_requested"] = prologue->deployment_requested();
                state["dialogue_scrolling"] = prologue->dialogue_scrolling();
                state["choosing"] = prologue->choosing();
                if (prologue->choosing())
                {
                    state["choices"] = Json::array();
                    const auto event = prologue->diagnostic_event();
                    for (size_t i = 1; i < prologue->events.line_count(event); ++i)
                    {
                        state["choices"].push_back({ { "index", i - 1 }, { "text_bytes", prologue->events.line(event, i).text } });
                    }
                }
                state["battle_intro"] = prologue->battle_intro();
                state["showing_objectives"] = prologue->showing_objectives();
                state["reward_report"] = prologue->campaign_reward_report();
                if (prologue->ending_active())
                {
                    state["ending"] = { { "page", prologue->ending_page() }, { "pages", prologue->ending_pages() }, { "done", prologue->ending_done() }, { "description", prologue->ending_description() } };
                }
                if (!prologue->events.victory_objective.empty() || !prologue->events.defeat_objective.empty())
                {
                    state["objectives"] = { { "encoding", "Big5" }, { "victory_bytes", prologue->events.victory_objective }, { "defeat_bytes", prologue->events.defeat_objective } };
                }
                if (prologue->temporary_text_active()) { state["modal"] = "temporary_text"; }
                if (prologue->in_battle()) { state["scene"] = "battle"; }
                state["stage"] = prologue->events.stage;
                state["stage_name_bytes"] = prologue->events.stage_name;
                state["turn"] = prologue->battle_turn();
                state["exit_cells"] = nlohmann::json::array();
                for (const auto& [x, y] : prologue->visible_exit_cells()) { state["exit_cells"].push_back({ { "x", x }, { "y", y } }); }
                state["capture_cells"] = nlohmann::json::array();
                if (prologue->has_battle())
                {
                    for (const auto& cell : prologue->battle().object_cells(4))
                    {
                        state["capture_cells"].push_back({ { "x", cell.x }, { "y", cell.y }, { "object_category", 4 } });
                    }
                }
                state["selected"] = prologue->battle_selected();
                state["camera_x"] = prologue->camera().first;
                state["camera_y"] = prologue->camera().second;
                state["computer_actions"] = prologue->computer_actions();
                state["player_selection_context"] = prologue->events.army_modes ? unsigned((*prologue->events.army_modes)[0]) : 0u;
                if (prologue->has_battle())
                {
                    state["battle_state"] = int(prologue->battle_state());
                    state["movement_active"] = prologue->battle().animating_movement();
                    state["ai_skill_pending"] = prologue->battle().ai_skill_pending();
                    state["combat_feedback_active"] = prologue->battle().combat_feedback_active();
                }
                state["input_locked"] = bool(itemLearningPublish) || battleItems.has_value() || itemTargetRequest.has_value() || !prologue->original_save_boundary_ready() || prologue->computer_actions() || prologue->campaign_modal();
                if (battleItems) { state["modal"] = "items"; }
                if (itemTargetRequest)
                {
                    state["modal"] = "item_target";
                    state["item_id"] = itemTargetRequest->second;
                }
                state["blocker"] = prologue->campaign_blocker();
                if (deployment)
                {
                    state["modal"] = "deployment";
                    state["input_locked"] = true;
                    state["deployment_position"] = deployment->positioning();
                    state["deployment_capacity"] = deployment->capacity();
                    state["deployment_assignments"] = deployment->assignments();
                    state["deployment_facing"] = deployment->facing();
                    state["deployment_roles"] = deployment->roles();
                    state["deployment_confirmation"] = deployment->confirmation_pending();
                    state["deployment_slots"] = Json::array();
                    for (const auto& slot : deployment->slots())
                    {
                        auto point = deployment->cell_screen(slot.x, slot.y);
                        state["deployment_slots"].push_back({ { "x", slot.x }, { "y", slot.y }, { "screen_x", point.first + 32 }, { "screen_y", point.second + 16 }, { "fixed", slot.fixed } });
                    }
                }
                state["campaign_dialogue"] = prologue->campaign_dialogue();
                state["campaign_defeated"] = prologue->campaign_defeated();
                state["campaign_rewards"] = prologue->campaign_reward_report();
                state["interbattle"] = prologue->interbattle_active();
                state["interbattle_services_allowed"] = prologue->interbattle_services_allowed();
                state["interbattle_available_options"] = nlohmann::json::array();
                if (prologue->interbattle_services_allowed()) { state["interbattle_available_options"] = { 0, 1, 2, 3 }; }
                if (prologue->campaign_modal()) { state["modal"] = "campaign"; }
                if (innMenu)
                {
                    state["modal"] = "inn";
                    state["inn_saves"] = innMenu->saves_active();
                    state["inn_equipment"] = innMenu->equipment_active();
                    state["inn_world"] = innMenu->world_active();
                    state["inn_settings"] = innMenu->settings_active();
                    state["inn_roster"] = innMenu->roster_active();
                    state["blocker"] = innMenu->blocker();
                }
                if (churchMenu)
                {
                    state["modal"] = "church";
                    state["church_page"] = int(churchMenu->page());
                    if (rawSave)
                    {
                        state["money"] = s2ne::OriginalShopMenu::money(*rawSave);
                        state["revival_recipients"] = churchMenu->revival_recipients();
                        if (churchMenu->page() == s2ne::OriginalChurch::Page::recipients || churchMenu->page() == s2ne::OriginalChurch::Page::resurrection_confirm)
                        {
                            state["revival_selected"] = churchMenu->revival_selected();
                            state["revival_cost"] = churchMenu->revival_price(*rawSave);
                        }
                    }
                }
                if (shopMenu)
                {
                    state["modal"] = "shop";
                    state["shop_page"] = int(shopMenu->page());
                    state["shop_selected"] = shopMenu->selected();
                    state["shop_transitioning"] = shopMenu->transitioning();
                    state["shop_items"] = Json::array();
                    if (rawSave)
                    {
                        state["money"] = s2ne::OriginalShopMenu::money(*rawSave);
                        for (const auto id : shopMenu->listed_items()) { state["shop_items"].push_back({ { "id", id }, { "price", shopMenu->listed_price(*rawSave, id) }, { "owned", rawSave->groups[0].inventory.at(id - 1) } }); }
                    }
                }
                if (const auto* name = prologue->learning_notice())
                {
                    state["modal"] = "learning";
                    state["learning"] = { { "id", prologue->reward_growth().unit().id }, { "name_bytes", *name } };
                }
                if (prologue->experience_notice() > 0)
                {
                    state["modal"] = "experience";
                    state["experience_award"] = prologue->experience_notice();
                }
                if (auto step = prologue->growth_step())
                {
                    state["modal"] = "growth";
                    state["growth"] = { { "id", step->before.id }, { "level", step->before.level }, { "xp", step->before.experience }, { "increments", step->increments } };
                }
                if (prologue->in_battle())
                {
                    state["battle_state"] = int(prologue->battle_state());
                    state["command_index"] = prologue->battle().command_index();
                    state["attack_index"] = prologue->battle().attack_index();
                    state["target"] = prologue->battle().target();
                    if (prologue->battle_state() == s2ne::Battle::State::movement)
                    {
                        state["movement_cells"] = Json::array();
                        state["reachable_cells"] = Json::array();
                        auto probe = prologue->battle();
                        const auto camera = prologue->camera();
                        for (int y = 0; y < 100; ++y)
                        {
                            for (int x = 0; x < 100; ++x)
                            {
                                if (!probe.preview({ x, y })) { continue; }
                                state["reachable_cells"].push_back({ { "x", x }, { "y", y } });
                                auto point = probe.screen_center({ x, y }, camera.first, camera.second);
                                if (point.x < 0 || point.x >= 640 || point.y < 0 || point.y >= 480) { continue; }
                                if (probe.pick_surface(float(point.x), float(point.y), camera.first, camera.second) != s2ne::Battle::Point{ x, y }) { continue; }
                                state["movement_cells"].push_back({ { "x", x }, { "y", y }, { "input_x", point.x }, { "input_y", point.y } });
                            }
                        }
                    }
                    state["attacks"] = Json::array();
                    state["attack_targets"] = Json::array();
                    state["attack_previews"] = Json::array();
                    if (prologue->battle_state() == s2ne::Battle::State::attack_target)
                    {
                        auto probe = prologue->battle();
                        for (size_t i = 0; i < prologue->events.units.size(); ++i)
                        {
                            if (probe.select_attack_target(prologue->events.units, i))
                            {
                                state["attack_targets"].push_back(i);
                                if (auto action = prologue->battle().executable_action(prologue->events.units, size_t(prologue->battle_selected()), i, prologue->battle().attack_index()))
                                {
                                    const auto& actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                                    const int damage = prologue->battle().physical_action_preview(prologue->events, actor, prologue->events.units[i], *action);
                                    state["attack_previews"].push_back({ { "index", i }, { "damage", damage } });
                                }
                            }
                        }
                    }
                    for (int i = 0; i < 8; ++i)
                    {
                        state["attacks"].push_back({ { "id", i }, { "enabled", prologue->battle().attack_command_available(prologue->events.units, i) } });
                    }
                    if (battleMenu)
                    {
                        state["battle_menu_page"] = int(battleMenu->page());
                        state["battle_menu_selected"] = battleMenu->selected();
                        state["battle_menu_items"] = Json::array();
                        const auto page = battleMenu->page();
                        std::span<const unsigned> ids = s2ne::OriginalBattleMenu::overview_ids;
                        if (page == s2ne::OriginalBattleMenu::Page::root) { ids = s2ne::OriginalBattleMenu::root_ids; }
                        else if (page == s2ne::OriginalBattleMenu::Page::system) { ids = s2ne::OriginalBattleMenu::system_ids; }
                        for (size_t i = 0; i < battleMenu->rows(); ++i)
                        {
                            const auto bytes = s2ne::OriginalMenu(root).text_bytes(ids[i]);
                            state["battle_menu_items"].push_back({ { "index", i }, { "id", ids[i] }, { "name_bytes", bytes }, { "enabled", true } });
                        }
                    }
                }
                for (size_t i = 0; i < prologue->events.units.size(); ++i)
                {
                    const auto& u = prologue->events.units[i];
                    s2ne::Battle::Point point{};
                    if (prologue->in_battle()) { point = prologue->battle().screen_center({ u.x, u.y }, prologue->camera().first, prologue->camera().second); }
                    state["units"].push_back({ { "index", i }, { "id", u.id }, { "hostile", u.hostile() }, { "x", u.x }, { "y", u.y }, { "screen_x", point.x }, { "screen_y", point.y }, { "hp", u.hp }, { "max_hp", u.max_hp }, { "mp", u.mp }, { "st", u.st }, { "ap", u.action_points }, { "level", u.level }, { "xp", u.experience }, { "xp_threshold", u.experience_threshold }, { "abilities", u.abilities }, { "learned", u.learned }, { "proficiency", u.learned_proficiency } });
                    state["units"].back()["controllable"] = u.controllable();
                    bool selectable = false;
                    if (prologue->in_battle() && prologue->battle_state() == s2ne::Battle::State::idle && !prologue->computer_actions())
                    {
                        auto probe = prologue->battle();
                        auto units = prologue->events.units;
                        const unsigned context = prologue->events.army_modes ? unsigned((*prologue->events.army_modes)[0]) : 0u;
                        selectable = probe.select_unit(units, i, context);
                    }
                    state["units"].back()["selectable"] = selectable;
                    state["units"].back()["attack"] = u.attack;
                    state["units"].back()["defense"] = u.defense;
                    state["units"].back()["max_mp"] = u.max_mp;
                    state["units"].back()["max_st"] = u.max_st;
                    state["units"].back()["name_bytes"] = u.name;
                    state["units"].back()["class_id"] = u.appearance;
                    if (u.appearance)
                    {
                        state["units"].back()["class_name_bytes"] = promotionRules.candidate(u.appearance).name;
                        auto promotionProbe = u;
                        state["units"].back()["promotion_eligible"] = rawSave && u.controllable() && promotionRules.eligible(promotionProbe, u.experience_threshold, int(rawSave->globals_4f7cd0[0])) && !promotionRules.candidates(promotionProbe).empty();
                    }
                    auto unlearned = u;
                    unlearned.learned.fill(0);
                    state["units"].back()["known_skills"] = Json::array();
                    for (const auto& skill : skillRules.newly_learned_records(unlearned, u))
                    {
                        state["units"].back()["known_skills"].push_back({ { "id", skill.id }, { "name_bytes", skill.name() } });
                    }
                    state["units"].back()["present"] = u.present(prologue->battle_turn());
                    state["units"].back()["action_state"] = u.action_state;
                    state["units"].back()["status_flags"] = u.status_flags;
                    state["units"].back()["behavior"] = u.behavior;
                    state["units"].back()["runtime_role_selector"] = u.runtime_role_selector;
                    state["units"].back()["runtime_flags30"] = u.runtime_flags30;
                    state["units"].back()["equipment"] = u.equipment;
                    if (prologue->in_battle())
                    {
                    }
                    if (prologue->in_battle()) { state["units"].back()["occupant"] = prologue->battle().occupant(prologue->events.units, { u.x, u.y }); }
                    auto inputPoint = s2ne::Battle::center({ u.x, u.y });
                    state["units"].back()["input_x"] = inputPoint.x - prologue->camera().first;
                    state["units"].back()["input_y"] = inputPoint.y - prologue->camera().second;
                    state["units"].back()["input_hittable"] = false;
                    if (prologue->in_battle())
                    {
                        for (int dy = -24; dy <= 24 && !state["units"].back()["input_hittable"].get<bool>(); dy += 4)
                        {
                            for (int dx = -28; dx <= 28; dx += 4)
                            {
                                const int x = point.x + dx, y = point.y + dy;
                                if (x < 0 || x >= 640 || y < 0 || y >= 480) { continue; }
                                auto picked = prologue->battle().pick_surface(float(x), float(y), prologue->camera().first, prologue->camera().second);
                                if (picked.x == u.x && picked.y == u.y)
                                {
                                    state["units"].back()["input_x"] = x;
                                    state["units"].back()["input_y"] = y;
                                    state["units"].back()["input_hittable"] = true;
                                    break;
                                }
                            }
                            state["units"].back()["height"] = prologue->battle().cell({ u.x, u.y }).elevation;
                        }
                    }
                }
                state["item_cells"] = Json::array();
                if (rawSave) { state["inventory"] = rawSave->groups[0].inventory; }
                if (prologue->in_battle())
                {
                    for (const auto point : prologue->battle().object_cells(12))
                    {
                        const unsigned marker = prologue->item_marker(point.x, point.y);
                        Json item{ { "x", point.x }, { "y", point.y }, { "marker", marker }, { "locked", bool(marker & 0x100) } };
                        const unsigned chain = marker & 255;
                        if (chain && chain <= prologue->events.dynamic_object_chains.size()) { item["item_candidates"] = prologue->events.dynamic_object_chains[chain - 1]; }
                        state["item_cells"].push_back(std::move(item));
                    }
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
                            if (failure == s2ne::OriginalSkills::Failure::none)
                            {
                                auto eligibility = *skillMenu;
                                eligibility.select_record(record.id);
                                auto partner = eligibility.cooperative_partner(unit, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                    {
                                        return prologue->battle().cell({ x, y }).elevation;
                                    });
                                failure = eligibility.select(unit, partner ? &prologue->events.units[*partner] : nullptr, eligibility.ai_summon_available(unit, prologue->events));
                            }
                            static constexpr const char* failures[]{ "none", "mp", "st", "action_points", "hp", "partner_unresolved", "world_unresolved", "unsupported_execution", "stale_unit" };
                            state["skills"].push_back({ { "index", state["skills"].size() }, { "id", record.id }, { "name_bytes", record.name() }, { "group", record.group }, { "bit", record.bit }, { "resource_failure", failures[int(failure)] }, { "cost", record.word(13) }, { "effect", record.effect() } });
                        }
                    }
                    state["selected_skill"] = skillMenu->selected().id;
                    state["skill_page"] = int(skillMenu->page());
                    state["skill_area_supported"] = skillMenu->status_area_supported();
                    state["skill_targets"] = Json::array();
                    if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && prologue->battle_selected() >= 0)
                    {
                        const auto& actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                        for (size_t i = 0; i < prologue->events.units.size(); ++i)
                        {
                            const auto& target = prologue->events.units[i];
                            if (target.present(prologue->battle_turn()) && prologue->battle().target_visible(target, std::span(&actor, 1), 0) && (skillMenu->healing1_in_range(actor, target) || skillMenu->hostile_in_range(actor, target))) { state["skill_targets"].push_back(i); }
                        }
                    }
                    auto probe = *skillMenu;
                    auto partner = skillMenu->cooperative_partner(unit, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                        {
                            return prologue->battle().cell({ x, y }).elevation;
                        });
                    const s2ne::OriginalEvents::Unit* partnerUnit = nullptr;
                    if (partner) { partnerUnit = &prologue->events.units[*partner]; }
                    bool summonReady = false;
                    if (skillMenu->summon_supported() && prologue->events.army_status)
                    {
                        const auto& slots = (*prologue->events.army_status)[unit.hostile() ? 1 : 0];
                        summonReady = true;
                        for (unsigned i = 0; i < 11; ++i)
                        {
                            if (slots[i * 3] == 1) { summonReady = false; }
                        }
                    }
                    static constexpr const char* failures[]{ "none", "mp", "st", "action_points", "hp", "partner_unresolved", "world_unresolved", "unsupported_execution", "stale_unit" };
                    auto selectionFailure = probe.resource_failure(unit, probe.selected());
                    if (probe.page() == s2ne::OriginalSkills::Page::list) { selectionFailure = probe.select(unit, partnerUnit, summonReady); }
                    state["selected_skill_failure"] = failures[int(selectionFailure)];
                    state["skill_target"] = skillTarget ? Json(*skillTarget) : Json(nullptr);
                    if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && prologue->battle_selected() >= 0)
                    {
                        const auto& caster = prologue->events.units.at(size_t(prologue->battle_selected()));
                        state["skill_cells"] = Json::array();
                        auto camera = prologue->camera();
                        for (int y = 0; y < 100; ++y)
                        {
                            for (int x = 0; x < 100; ++x)
                            {
                                if (!prologue->battle().skill_area_cell({ x, y }) || !skillMenu->selection_cell_in_range(caster, x, y)) { continue; }
                                auto p = s2ne::Battle::center({ x, y });
                                state["skill_cells"].push_back({ { "x", x }, { "y", y }, { "input_x", p.x - camera.first }, { "input_y", p.y - camera.second } });
                                if (skillMenu->status_area_supported() && !skillMenu->directional_skill())
                                {
                                    auto center = caster;
                                    center.x = x;
                                    center.y = y;
                                    auto& entry = state["skill_cells"].back();
                                    entry["targets"] = Json::array();
                                    for (size_t i = 0; i < prologue->events.units.size(); ++i)
                                    {
                                        const auto& target = prologue->events.units[i];
                                        if (target.present(prologue->battle_turn()) && skillMenu->status_area_target(caster, target) && skillMenu->sleep2_area(center, target) && prologue->battle().skill_area_cell({ target.x, target.y }) && prologue->battle().target_visible(target, std::span(&center, 1), 0)) { entry["targets"].push_back(i); }
                                    }
                                }
                            }
                        }
                    }
                    if (!prologue->growth_active()) { state["modal"] = "skills"; }
                }
            }
            if (battleMenu)
            {
                state["modal"] = "battle_menu";
                state["battle_menu_page"] = int(battleMenu->page());
                state["battle_menu_selected"] = battleMenu->selected();
            }
            if (promotionMenu)
            {
                state["modal"] = "promotion";
                state["promotion_unit"] = promotionMenu->unit().id;
                state["promotion_selected"] = promotionMenu->selected_index();
                state["promotion_page"] = int(promotionMenu->page());
                state["promotion_candidates"] = Json::array();
                for (const auto& candidate : promotionMenu->choices())
                {
                    state["promotion_candidates"].push_back({ { "kind", candidate.kind }, { "name_bytes", candidate.name }, { "ability_mask", candidate.ability_mask }, { "granted_ability", promotionRules.innate_ability(candidate.kind) }, { "training_mask", candidate.training_mask } });
                }
            }
            if (saveMenu)
            {
                state["modal"] = "save";
                state["save_slot"] = saveMenu->selected_slot();
            }
            if (settings) { state["modal"] = "settings"; }
            if (spiritSightImage) { state["modal"] = "spirit_sight"; }
            if (battleReports)
            {
                state["modal"] = "battle_reports";
                state["reports_page"] = int(battleReports->page());
            }
            if (itemMenu)
            {
                state["modal"] = "item_use";
                state["item_confirmation"] = itemConfirmation;
                state["item_selected"] = usableItemSelection;
                state["usable_items"] = usableItems;
            }
            if (overwriteConfirmation) { state["modal"] = "overwrite_confirmation"; }
            if (itemLearningPublish) { state["modal"] = "item_learning_animation"; }
            if (prologue && prologue->growth_active())
            {
                state["modal"] = "growth";
                if (prologue->learning_notice()) { state["modal"] = "learning"; }
                if (prologue->experience_notice() > 0) { state["modal"] = "experience"; }
            }
            const bool automatic = state.value("computer_actions", false) || state.value("movement_active", false)
                || state.value("combat_feedback_active", false) || state.value("battle_intro", false)
                || state.value("dialogue_scrolling", false) || bool(itemLearningPublish) || pendingSkillTarget.has_value() || (shopMenu && shopMenu->transitioning())
                || (prologue && prologue->diagnostic_phase() >= 11 && prologue->diagnostic_phase() <= 14);
            state["interaction"] = { { "mode", automatic ? "advancing" : "input" },
                { "context", state["modal"] }, { "state_contract_supported", true } };
            if (prologue && prologue->growth_active()) { state["interaction"]["mode"] = "input"; }
            if (prologue && prologue->campaign_dialogue() && !prologue->dialogue_scrolling()) { state["interaction"]["mode"] = "input"; }
            if (prologue && prologue->choosing()) { state["interaction"]["mode"] = "input"; }
            if (!state.value("blocker", std::string{}).empty()) { state["interaction"]["mode"] = "blocked"; }
            state["response_required"] = state["interaction"]["mode"] == "input";
            auto menuItems = [&](const auto& ids, int first = 0)
            {
                Json entries = Json::array();
                for (const auto id : ids)
                {
                    entries.push_back({ { "index", first++ }, { "id", id }, { "name_bytes", s2ne::OriginalMenu(root).text_bytes(id) } });
                }
                return entries;
            };
            if (prologue && prologue->interbattle_active())
            {
                state["interbattle_items"] = menuItems(s2ne::OriginalInterbattle::labels);
                for (size_t i = 0; i < state["interbattle_items"].size(); ++i)
                {
                    state["interbattle_items"][i]["enabled"] = prologue->interbattle_services_allowed() || i == 3;
                    state["interbattle_items"][i]["address"] = s2ne::OriginalInterbattle::targets[i];
                }
            }
            if (innMenu) { state["inn_items"] = menuItems(std::array<unsigned, 10>{ 193, 194, 128, 129, 195, 192, 80, 131, 123, 190 }, 1); }
            if (churchMenu && churchMenu->page() == s2ne::OriginalChurch::Page::menu) { state["church_items"] = menuItems(std::array<unsigned, 3>{ 196, 197, 198 }, 1); }
            if (shopMenu)
            {
                if (shopMenu->page() == s2ne::OriginalShopMenu::Page::shop) { state["shop_options"] = menuItems(std::array<unsigned, 4>{ 93, 94, 95, 199 }); }
                s2ne::OriginalShop names(root);
                for (auto& item : state["shop_items"]) { item["name_bytes"] = names.name(item.at("id").get<unsigned>()); }
            }
            if (deployment) { state["deployment_items"] = menuItems(std::array<unsigned, 5>{ 187, 188, 189, 190, 191 }); }
            if (settings || (innMenu && innMenu->settings_active()))
            {
                state["settings_items"] = menuItems(std::array<unsigned, 12>{ 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143 });
            }
            if (battleMenu && battleMenu->page() == s2ne::OriginalBattleMenu::Page::confirmation)
            {
                state["confirmation"] = { { "id", battleMenu->confirmation_id() }, { "text_bytes", s2ne::OriginalMenu(root).text_bytes(battleMenu->confirmation_id()) } };
            }
            if (battleMenu && battleMenu->page() == s2ne::OriginalBattleMenu::Page::root)
            {
                static constexpr std::array<int, 6> returns{ 0, 0, 0, 6, 3, 15 };
                for (auto& item : state["battle_menu_items"])
                {
                    const auto index = item.at("index").get<size_t>();
                    item["return_code"] = index < 3 ? Json(nullptr) : Json(returns.at(index));
                    item["return_code_kind"] = index < 3 ? "operation" : "confirmed";
                }
            }
            for (const char* list : { "commands", "attacks", "promotion_candidates", "shop_items", "usable_items" })
            {
                if (!state.contains(list) || !state[list].is_array()) { continue; }
                size_t index = 0;
                for (auto& item : state[list])
                {
                    if (item.is_object())
                    {
                        if (!item.contains("index")) { item["index"] = index; }
                        if (!item.contains("name_bytes")) { item["name_bytes"] = Json::array(); }
                    }
                    if (state.contains("usable_items"))
                    {
                        s2ne::OriginalShop names(root);
                        state["usable_item_options"] = Json::array();
                        for (const auto& item : state["usable_items"])
                        {
                            if (!item.is_number_unsigned() && !item.is_number_integer()) { continue; }
                            const auto id = item.get<unsigned>();
                            state["usable_item_options"].push_back({ { "index", state["usable_item_options"].size() }, { "id", id }, { "name_bytes", names.name(id) } });
                        }
                    }
                    ++index;
                }
            }
            for (const char* list : { "attack_targets", "skill_targets", "revival_recipients" })
            {
                if (!state.contains(list)) { continue; }
                auto& entries = state[std::string(list) + "_items"] = Json::array();
                for (const auto& index : state[list])
                {
                    const auto unit = index.get<size_t>();
                    if (unit >= state["units"].size()) { continue; }
                    entries.push_back({ { "index", unit }, { "id", state["units"][unit]["id"] }, { "name_bytes", state["units"][unit]["name_bytes"] } });
                }
            }
            state["semantic_actions"] = Json::array();
            auto semanticAction = [&](const char* command, std::optional<unsigned> textId = {})
            {
                Json entry{ { "index", state["semantic_actions"].size() }, { "command", command }, { "name_bytes", Json::array() } };
                if (textId)
                {
                    entry["id"] = *textId;
                    entry["name_bytes"] = s2ne::OriginalMenu(root).text_bytes(*textId);
                }
                state["semantic_actions"].push_back(std::move(entry));
            };
            for (const char* command : { "observe", "await_state", "transaction", "wait", "quit", "release_control", "screenshot" }) { semanticAction(command); }
            if (prologue)
            {
                if (prologue->in_battle())
                {
                    for (const char* command : { "select_unit", "command_select", "attack_select", "target_select", "target_options", "move_select", "skill_select", "skill_area_select", "skill_cancel", "battle_confirm", "battle_cancel", "item_select", "item_target_select", "item_confirm", "item_cancel", "open_system_menu", "menu_select", "promotion_open", "promotion_choose", "promotion_train", "promotion_confirm", "promotion_cancel", "save_checkpoint" }) { semanticAction(command); }
                    semanticAction("end_turn", 125);
                }
                if (prologue->growth_active()) { semanticAction("growth_confirm"); }
                if (prologue->campaign_reward_report()) { semanticAction("rewards_confirm"); }
                if (prologue->choosing()) { semanticAction("story_choose"); }
                if (prologue->campaign_dialogue())
                {
                    semanticAction("dialogue_advance");
                    semanticAction("dialogue_reveal");
                }
                if (prologue->interbattle_active()) { semanticAction("interbattle_select"); }
                if (prologue->deployment_requested())
                {
                    for (const char* command : { "deployment_auto", "deployment_option", "deployment_role", "deployment_cell", "deployment_confirm" }) { semanticAction(command); }
                }
            }
            if (shopMenu)
            {
                for (const char* command : { "shop_category", "shop_item", "shop_commit", "shop_back" }) { semanticAction(command); }
            }
            if (churchMenu)
            {
                for (const char* command : { "church_revival", "church_recipient", "church_confirm", "church_leave" }) { semanticAction(command); }
            }
            if (innMenu)
            {
                for (const char* command : { "inn_auto_equipment", "inn_auto_confirm", "inn_leave" }) { semanticAction(command); }
            }
            return state;
        };
        auto stdioRespond = [&](s2ne::StdioProtocol::Json response)
        {
            if (stdioTransaction)
            {
                auto& transaction = *stdioTransaction;
                transaction["completed_steps"] = transaction.at("completed_steps").get<unsigned>() + unsigned(response.value("ok", false));
                if (response.value("ok", false) && !stdioSteps.empty()) { return; }
                transaction["ok"] = response.value("ok", false);
                transaction["state"] = response["state"];
                transaction["progress"] = response["progress"];
                if (!response.value("ok", false))
                {
                    transaction["error"] = response.value("error", std::string("transaction step failed"));
                    transaction["failed_step"] = transaction["completed_steps"];
                }
                stdioSteps.clear();
                s2ne::StdioProtocol::verify_response(transaction);
                stdio->respond(transaction);
                stdioTransaction.reset();
                return;
            }
            stdio->respond(response);
        };
        s2ne::Prologue* musicPrologue = nullptr;
        unsigned musicStage = 0;
        unsigned musicRestoreGeneration = 0;
        unsigned musicScene = 0;
        bool musicCd = settingsState.cdda != 0;
        std::optional<size_t> semanticItemTarget;
        auto useOriginalItemTarget = [&](size_t actor, unsigned id, size_t target)
        {
            auto snapshot = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
            if (!snapshot)
            {
                s2ne::report_error(snapshot.error());
                return;
            }
            if (!itemUse.target_allowed(snapshot.value(), actor, target, id))
            {
                itemNoticeMilliseconds = 1;
                return;
            }
            prologue->battle().mutate_combat_random([&](auto& random, uint32_t seconds)
                {
                    const auto beforeRandom = random;
                    auto next = itemUse.try_prepare(snapshot.value(), actor, id, random, seconds, target);
                    if (!next)
                    {
                        s2ne::report_error(next.error());
                        return false;
                    }
                    auto restored = s2ne::OriginalSaveBridge::try_restore(root, next.value());
                    if (!restored)
                    {
                        s2ne::report_error(restored.error());
                        return false;
                    }
                    const auto seed = random.seed_state(), calls = random.draw_count();
                    const int amount = itemUse.feedback_amount(snapshot.value(), next.value(), target, id, beforeRandom, seconds);
                    const int hp = prologue->events.units[target].hp;
                    std::vector<std::vector<uint8_t>> learningNotices;
                    for (const auto& record : skillRules.newly_learned_records(prologue->events.units[target], restored.value().events.units[target]))
                    {
                        auto text = prologue->events.units[target].name;
                        text.push_back(' ');
                        const auto learned = itemNoticeFont.text_bytes(3);
                        text.insert(text.end(), learned.begin(), learned.end());
                        text.push_back(' ');
                        const auto name = record.name();
                        text.insert(text.end(), name.begin(), name.end());
                        learningNotices.push_back(std::move(text));
                    }
                    auto publish = [&, saved = std::move(next).value(), events = std::move(restored.value().events), seed, calls, id, target]() mutable
                    {
                        using std::swap;
                        swap(*rawSave, saved);
                        swap(prologue->events, events);
                        prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                            {
                                live.restore_stream(seed, calls);
                                return true;
                            });
                        prologue->battle().commit_external_action();
                        std::print("Original45024C item={} target={} effect/inventory/RNG published once\n", id, target);
                    };
                    auto afterPose = [&, actor, target, hp, amount, learningNotices = std::move(learningNotices), publish = std::move(publish)]() mutable
                    {
                        if (!learningNotices.empty())
                        {
                            itemLearningNotices = std::move(learningNotices);
                            itemLearningSince = 0;
                            itemLearningPublish = std::move(publish);
                            return;
                        }
                        if (!amount)
                        {
                            publish();
                            return;
                        }
                        if (!prologue->battle().begin_healing_feedback(actor, target, hp, amount, std::move(publish), false)) { s2ne::report_error(s2ne::make_error("451201 item number presentation failed; transaction unpublished")); }
                    };
                    auto presentation = prologue->begin_item_presentation(actor, target, itemUse.targeted(id), std::move(afterPose));
                    if (!presentation)
                    {
                        s2ne::report_error(presentation.error());
                        return false;
                    }
                    itemTargetRequest.reset();
                    battleItems.reset();
                    return false;
                });
        };
        while (running)
        {
            if (semanticItemTarget && itemTargetRequest)
            {
                const auto target = *semanticItemTarget;
                semanticItemTarget.reset();
                const auto [actor, id] = *itemTargetRequest;
                useOriginalItemTarget(actor, id, target);
            }
            if (prologue && prologue->defeat_finished())
            {
                prologue.reset();
                rawSave.reset();
                connectedCampaign = nullptr;
                autosaveBlocked = false;
                inSetup = false;
                showMenu();
            }
            if (options.headlessSpeed) { headlessTicks += 16; }
            uint64_t nowUi = gameTicks();
            static uint64_t reportFrame = 0;
            if (battleReports && nowUi / 100 != reportFrame)
            {
                reportFrame = nowUi / 100;
                showPrologue();
            }
            if (promotionMenu && promotionMenu->tick(nowUi)) { showPrologue(); }
            auto musicSettings = settingsState;
            if (settings) { musicSettings = settings->state(); }
            if (battleMenu) { musicSettings = battleMenu->settings_state(); }
            auto musicVolume = audio.music_volumes(musicSettings.midi_volume, musicSettings.cdda_volume);
            if (!musicVolume) { s2ne::report_error(musicVolume.error()); }
            if (musicCd != (musicSettings.cdda != 0))
            {
                musicCd = musicSettings.cdda != 0;
                unsigned id = audio.music_id();
                auto stopped = audio.stop_music();
                if (!stopped) { s2ne::report_error(stopped.error()); }
                else if (id)
                {
                    auto played = playMusic(id);
                    if (!played) { s2ne::report_error(played.error()); }
                }
            }
            unsigned scene = 0, sceneTrack = 0;
            if (inMenu || inSetup)
            {
                scene = 1;
                sceneTrack = 25;
            }
            if (prologue && prologue->interbattle_active())
            {
                scene = 2;
                sceneTrack = 4;
            }
            if (churchMenu)
            {
                scene = 3;
                sceneTrack = 37;
            }
            if (innMenu && innMenu->world_active())
            {
                // 原410EEE进入414D5B世界地图时切36，返回后恢复原曲4。
                scene = 5;
                sceneTrack = 36;
            }
            if (promotionMenu)
            {
                scene = 4;
                sceneTrack = 0;
            }
            if (deployment)
            {
                scene = 6;
                sceneTrack = 25;
            }
            if (scene != musicScene)
            {
                unsigned previousScene = musicScene;
                musicScene = scene;
                if (scene || previousScene)
                {
                    if (!scene && prologue) { sceneTrack = prologue->events.stage_music; }
                    auto played = playMusic(sceneTrack);
                    if (!played) { s2ne::report_error(played.error()); }
                }
            }
            if (prologue)
            {
                if (!scene && (musicPrologue != prologue.get() || musicStage != prologue->events.stage || musicRestoreGeneration != prologue->music_restore_generation()))
                {
                    musicPrologue = prologue.get();
                    musicStage = prologue->events.stage;
                    musicRestoreGeneration = prologue->music_restore_generation();
                    // 原4293AF场景构建先停曲，即便新旧关卡曲号相同也不是同曲续播。
                    auto played = audio.stop_music();
                    if (played) { played = playMusic(prologue->events.stage_music); }
                    if (!played) { s2ne::report_error(played.error()); }
                }
                prologue->set_music_audio(playMusic, playMidi);
                prologue->observe_music(audio.music_id());
            }
            else
            {
                musicPrologue = nullptr;
                musicStage = 0;
            }
            if (prologue && prologue->ending_active() && audio.effects())
            {
                prologue->set_ending_effect_audio(playAni, stopEffects);
            }
            if (audio.effects())
            {
                int gain = settingsState.effect_volume;
                if (settings) { gain = settings->state().effect_volume; }
                if (battleMenu) { gain = battleMenu->settings_state().effect_volume; }
                if (audio.is_muted()) { gain = 0; }
                auto volume = audio.effects()->volume(gain);
                if (!volume) { s2ne::report_error(volume.error()); }
            }
            if (prologue)
            {
                prologue->set_skill_effect_audio(playEffect);
                prologue->set_script_effect_audio(playAni, stopEffects);
                if (prologue->has_battle())
                {
                    prologue->battle().set_effect_audio([&](unsigned id, unsigned channel)
                        {
                            if (!audio.effects()) { return 0u; }
                            auto result = audio.effects()->play(id, channel);
                            if (!result)
                            {
                                s2ne::report_error(result.error());
                                return 0u;
                            }
                            if (diagnostics) { std::print(stderr, "Original action effect id={} channel={}\n", id, result.value()); }
                            return result.value();
                        });
                }
            }
            if (stdio && !stdioPending)
            {
                stdio->poll();
                std::optional<s2ne::StdioProtocol::Json> nextRequest;
                if (!stdioSteps.empty())
                {
                    nextRequest = std::move(stdioSteps.front());
                    stdioSteps.pop_front();
                }
                else
                {
                    nextRequest = stdio->next();
                }
                if (auto request = std::move(nextRequest))
                {
                    auto response = s2ne::StdioProtocol::Json{ { "ok", true } };
                    auto queue_stdio_key = [&](SDL_Keycode key)
                    {
                        SDL_Event event{};
                        event.type = SDL_EVENT_KEY_DOWN;
                        event.key.windowID = SDL_GetWindowID(window);
                        event.key.key = key;
                        event.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
                        if (request->contains("modifiers"))
                        {
                            const auto& modifiers = request->at("modifiers");
                            if (!modifiers.is_array()) { throw std::runtime_error("modifiers must be an array of SDL modifier key names"); }
                            for (const auto& modifier : modifiers)
                            {
                                if (!modifier.is_string()) { throw std::runtime_error("modifier must be a string"); }
                                const auto modifierKey = SDL_GetKeyFromName(modifier.get<std::string>().c_str());
                                SDL_Keymod mask = SDL_KMOD_NONE;
                                switch (modifierKey)
                                {
                                case SDLK_LSHIFT: mask = SDL_KMOD_LSHIFT; break;
                                case SDLK_RSHIFT: mask = SDL_KMOD_RSHIFT; break;
                                case SDLK_LCTRL: mask = SDL_KMOD_LCTRL; break;
                                case SDLK_RCTRL: mask = SDL_KMOD_RCTRL; break;
                                case SDLK_LALT: mask = SDL_KMOD_LALT; break;
                                case SDLK_RALT: mask = SDL_KMOD_RALT; break;
                                case SDLK_LGUI: mask = SDL_KMOD_LGUI; break;
                                case SDLK_RGUI: mask = SDL_KMOD_RGUI; break;
                                default: throw std::runtime_error("unsupported modifier key");
                                }
                                event.key.mod |= mask;
                            }
                        }
                        stdioInputEvents.push_back(event);
                        event.type = SDL_EVENT_KEY_UP;
                        stdioInputEvents.push_back(event);
                    };
                    try
                    {
                        if (request->contains("id")) { response["id"] = request->at("id"); }
                        if (request->contains("protocol_error")) { throw std::runtime_error(request->at("protocol_error").get<std::string>()); }
                        const auto command = request->at("cmd").get<std::string>();
                        if (request->contains("expect_after"))
                        {
                            if (!request->at("expect_after").is_object()) { throw std::runtime_error("expect_after must be an object"); }
                            response["expect_after"] = request->at("expect_after");
                        }
                        if (request->contains("expect_before"))
                        {
                            if (auto error = s2ne::StdioProtocol::mismatch(stdioObserve(), request->at("expect_before"))) { throw std::runtime_error(*error); }
                        }
                        if (command == "transaction")
                        {
                            if (stdioTransaction) { throw std::runtime_error("nested transaction is not allowed"); }
                            const auto& steps = request->at("steps");
                            if (!steps.is_array() || steps.empty() || steps.size() > 128) { throw std::runtime_error("steps must contain 1..128 commands"); }
                            for (const auto& step : steps)
                            {
                                if (!step.is_object() || !step.contains("cmd") || !step.at("cmd").is_string()) { throw std::runtime_error("transaction step requires cmd"); }
                                const auto name = step.at("cmd").get<std::string>();
                                if (name == "transaction" || name == "quit") { throw std::runtime_error("transaction cannot contain transaction or quit"); }
                                if (!step.contains("expect_after") || !step.at("expect_after").is_object() || step.at("expect_after").empty()) { throw std::runtime_error("every transaction step requires nonempty expect_after"); }
                            }
                            for (const auto& step : steps) { stdioSteps.push_back(step); }
                            response["completed_steps"] = 0;
                            stdioTransaction = response;
                            continue;
                        }
                        else if (command == "new_game")
                        {
                            if (!inMenu || inSetup || prologue || saveMenu || settings) { throw std::runtime_error("new game requires original title menu"); }
                            while (menu.selected != 0) { menu.move(-1); }
                            menu.activate();
                            showSetup();
                        }
                        else if (command == "setup_difficulty")
                        {
                            if (!inSetup || !request->contains("index") || !request->at("index").is_number_integer()) { throw std::runtime_error("difficulty selection requires original setup and integer index"); }
                            const int index = request->at("index").get<int>();
                            if (index < 0 || index > 4) { throw std::runtime_error("difficulty index must be 0..4"); }
                            if (!prologue || !prologue->interbattle_services_allowed()) { throw std::runtime_error("original town has no village services; automatic departure pending"); }
                            setup.move(index - setup.difficulty);
                            showSetup();
                        }
                        else if (command == "setup_confirm")
                        {
                            if (!inSetup) { throw std::runtime_error("setup confirmation unavailable"); }
                            startGame();
                        }
                        else if (command == "observe") { response["state"] = stdioObserve(); }
                        else if (command == "observe_movement_graph")
                        {
                            if (!prologue || !prologue->in_battle()) { throw std::runtime_error("movement graph requires battle"); }
                            const auto actor = request->value("index", 0u);
                            if (actor >= prologue->events.units.size()) { throw std::runtime_error("invalid graph actor"); }
                            response["state"] = stdioObserve();
                            response["state"]["movement_graph"] = prologue->battle().observed_movement_edges(prologue->events.units, actor, request->value("occupied", true));
                            const auto& unit = prologue->events.units.at(actor);
                            response["state"]["movement_budget"] = std::max(0, std::min(110, unit.max_action_points - prologue->events.battle_config[0]));
                            response["state"]["maximum_action_range"] = prologue->events.maximum_action_range(unit);
                        }
                        else if (command == "await_state")
                        {
                            if (!request->contains("expect_after") || request->at("expect_after").empty()) { throw std::runtime_error("await_state requires nonempty expect_after"); }
                            if (!request->contains("max_frames") || !request->at("max_frames").is_number_integer()) { throw std::runtime_error("await_state requires integer max_frames"); }
                            const int frames = request->at("max_frames").get<int>();
                            if (frames < 1 || frames > 100000) { throw std::runtime_error("max_frames must be 1..100000"); }
                            stdioRemainingFrames = unsigned(frames);
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "quit") { running = false; }
                        else if (command == "release_control")
                        {
                            if (options.headlessSpeed) { throw std::runtime_error("release_control requires visible runtime"); }
                        }
                        else if (command == "shop_category" || command == "shop_item" || command == "shop_commit" || command == "shop_back")
                        {
                            if (!shopMenu || !rawSave || !prologue) { throw std::runtime_error("shop unavailable"); }
                            bool accepted = false;
                            if (command == "shop_back") { accepted = shopMenu->return_parent(); }
                            else if (command == "shop_commit")
                            {
                                accepted = shopMenu->commit_quantity(*rawSave);
                                if (accepted) { s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events); }
                            }
                            else
                            {
                                if (!request->contains("index") || !request->at("index").is_number_integer() || request->at("index").get<int>() < 0) { throw std::runtime_error("shop selection requires nonnegative integer index"); }
                                const auto index = request->at("index").get<unsigned>();
                                if (command == "shop_category") { accepted = shopMenu->select_category(index, *rawSave); }
                                else
                                {
                                    accepted = shopMenu->select_item(index, *rawSave);
                                }
                            }
                            if (!accepted) { throw std::runtime_error("shop operation rejected by original conditions"); }
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
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "church_revival" || command == "church_recipient" || command == "church_confirm" || command == "church_leave")
                        {
                            using Page = s2ne::OriginalChurch::Page;
                            if (!churchMenu || !rawSave || !prologue) { throw std::runtime_error("original church unavailable"); }
                            if (command == "church_revival")
                            {
                                if (churchMenu->page() != Page::menu) { throw std::runtime_error("church root required"); }
                                while (churchMenu->selected() != 2) { churchMenu->scan(0x4D, *rawSave); }
                                churchMenu->scan(0x1C, *rawSave);
                            }
                            else if (command == "church_recipient")
                            {
                                if (churchMenu->page() != Page::recipients) { throw std::runtime_error("revival recipients required"); }
                                const auto index = request->at("index").get<size_t>();
                                const auto& candidates = churchMenu->revival_recipients();
                                if (std::find(candidates.begin(), candidates.end(), index) == candidates.end()) { throw std::runtime_error("ineligible revival recipient"); }
                                for (size_t i = 0; i < candidates.size(); ++i) { churchMenu->scan(0x48, *rawSave); }
                                while (churchMenu->revival_selected() != index) { churchMenu->scan(0x50, *rawSave); }
                                if (churchMenu->revival_price(*rawSave) > s2ne::OriginalShopMenu::money(*rawSave)) { throw std::runtime_error("insufficient money for original revival fee"); }
                                churchMenu->scan(0x1C, *rawSave);
                            }
                            else if (command == "church_confirm")
                            {
                                if (churchMenu->page() != Page::resurrection_confirm) { throw std::runtime_error("revival confirmation required"); }
                                if (!churchMenu->scan(0x1C, *rawSave)) { throw std::runtime_error("original revival transaction refused"); }
                                s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events);
                            }
                            else
                            {
                                if (churchMenu->page() == Page::recipients || churchMenu->page() == Page::message) { churchMenu->scan(1, *rawSave); }
                                if (churchMenu->page() != Page::menu) { throw std::runtime_error("church root required before leaving"); }
                                churchMenu->scan(1, *rawSave);
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
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "inn_auto_equipment" || command == "inn_auto_confirm")
                        {
                            if (!innMenu || !rawSave || !prologue) { throw std::runtime_error("inn unavailable"); }
                            bool accepted = false;
                            if (command == "inn_auto_equipment") { accepted = innMenu->select_auto_equipment(); }
                            else
                            {
                                accepted = innMenu->confirm_auto_equipment(*rawSave);
                            }
                            if (!accepted) { throw std::runtime_error("original inn auto equipment rejected"); }
                            if (innMenu->take_equipment_changed()) { s2ne::OriginalChurch::sync_roster(*rawSave, prologue->events); }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "inn_leave")
                        {
                            if (!innMenu || !prologue || !innMenu->leave_root()) { throw std::runtime_error("inn root cannot be left in current state"); }
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
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "item_target_select")
                        {
                            if (!prologue || !rawSave || !itemTargetRequest || semanticItemTarget || prologue->item_presentation_) { throw std::runtime_error("original item target unavailable"); }
                            const auto [actor, id] = *itemTargetRequest;
                            const int target = request->value("index", -1);
                            if (target < 0 || size_t(target) >= prologue->events.units.size()) { throw std::runtime_error("invalid original item target index"); }
                            auto snapshot = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                            if (!snapshot || !itemUse.target_allowed(snapshot.value(), actor, size_t(target), id)) { throw std::runtime_error("original item target rejected"); }
                            semanticItemTarget = size_t(target);
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "item_select" || command == "item_confirm" || command == "item_cancel")
                        {
                            if (!battleItems || itemTargetRequest || itemSpellSource || !rawSave) { throw std::runtime_error("original item menu unavailable"); }
                            if (command == "item_select")
                            {
                                if (!battleItems->select_item(request->value("item_id", 0u))) { throw std::runtime_error("item unavailable in original choices"); }
                            }
                            else if (command == "item_confirm")
                            {
                                if (!battleItems->confirm_use(*rawSave)) { throw std::runtime_error("original item confirmation unavailable"); }
                                if (itemTargetRequest && !itemUse.targeted(itemTargetRequest->second)) { semanticItemTarget = itemTargetRequest->first; }
                            }
                            else
                            {
                                battleItems->scan(1, *rawSave);
                            }
                            if (battleItems && battleItems->closed() && !itemTargetRequest && !itemSpellSource) { battleItems.reset(); }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "save_checkpoint")
                        {
                            const bool reward_boundary = prologue && prologue->campaign_reward_report() && !prologue->growth_active() && !prologue->computer_actions();
                            const bool hub_boundary = prologue && prologue->interbattle_active() && !churchMenu && !shopMenu && !innMenu;
                            if (!isolatedSaveRoot || !rawSave || !prologue || ((!prologue->in_battle() || !prologue->original_save_boundary_ready()) && !reward_boundary && !hub_boundary) || prologue->growth_active() || skillMenu || battleMenu || prologue->computer_actions()) { throw std::runtime_error("isolated checkpoint unavailable at current original boundary"); }
                            if (!request->contains("slot") || !request->at("slot").is_number_integer()) { throw std::runtime_error("save_checkpoint requires integer slot"); }
                            const int slot = request->at("slot").get<int>();
                            if (slot < 1 || slot > 50) { throw std::runtime_error("checkpoint slot must be 1..50"); }
                            auto captured = hub_boundary ? s2ne::Result<s2ne::OriginalSave::State>::success(*rawSave) : s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                            if (!captured) { throw std::runtime_error(captured.error().message); }
                            auto written = s2ne::OriginalSaveStore::try_write(*isolatedSaveRoot, unsigned(slot), captured.value(), saveItems);
                            if (!written) { throw std::runtime_error(written.error().message); }
                            response["saved_slot"] = slot;
                            response["state"] = stdioObserve();
                        }
                        else if (command == "rewards_confirm")
                        {
                            if (!prologue || !prologue->campaign_reward_report() || prologue->growth_active()) { throw std::runtime_error("reward report unavailable"); }
                            prologue->advance();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "interbattle_select")
                        {
                            const auto state = stdioObserve();
                            if (!prologue || !prologue->interbattle_active() || state.at("modal") != "campaign") { throw std::runtime_error("interbattle root unavailable"); }
                            if (!request->contains("index") || !request->at("index").is_number_integer()) { throw std::runtime_error("interbattle_select requires integer index"); }
                            const int index = request->at("index").get<int>();
                            if (index < 0 || index >= 4 || !prologue->select_interbattle_option(unsigned(index))) { throw std::runtime_error("interbattle selection rejected"); }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "deployment_auto" || command == "deployment_option" || command == "deployment_role" || command == "deployment_cell" || command == "deployment_confirm")
                        {
                            if (!deployment) { throw std::runtime_error("deployment unavailable"); }
                            bool accepted = false;
                            if (command == "deployment_auto")
                            {
                                accepted = deployment->select_option(0);
                            }
                            else if (command == "deployment_confirm")
                            {
                                if (!request->contains("yes") || !request->at("yes").is_boolean()) { throw std::runtime_error("deployment_confirm requires boolean yes"); }
                                accepted = deployment->respond_confirmation(request->at("yes").get<bool>());
                            }
                            else if (command == "deployment_cell")
                            {
                                if (!request->contains("x") || !request->contains("y") || !request->at("x").is_number_integer() || !request->at("y").is_number_integer()) { throw std::runtime_error("deployment_cell requires integer x and y"); }
                                accepted = deployment->select_cell({ request->at("x").get<int>(), request->at("y").get<int>() });
                            }
                            else
                            {
                                if (!request->contains("index") || !request->at("index").is_number_integer() || request->at("index").get<int>() < 0) { throw std::runtime_error("deployment selection requires nonnegative integer index"); }
                                const auto index = request->at("index").get<size_t>();
                                if (command == "deployment_option") { accepted = deployment->select_option(index); }
                                else
                                {
                                    accepted = deployment->select_role(index);
                                }
                            }
                            if (!accepted) { throw std::runtime_error("deployment operation rejected by current original state"); }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "assist_experience")
                        {
                            const bool hub = prologue && prologue->interbattle_active() && !churchMenu && !shopMenu && !innMenu;
                            if (!options.headlessSpeed || !prologue || (!prologue->in_battle() && !hub) || prologue->growth_active() || !prologue->original_save_boundary_ready() || prologue->computer_actions() || skillMenu || battleMenu || promotionMenu)
                            {
                                throw std::runtime_error("experience assistance requires an idle headless test battle");
                            }
                            if (!request->contains("index") || !request->at("index").is_number_integer() || !request->contains("amount") || !request->at("amount").is_number_integer()) { throw std::runtime_error("assist_experience requires integer index and amount"); }
                            const int index = request->at("index").get<int>();
                            const int amount = request->at("amount").get<int>();
                            if (index < 0 || index >= int(prologue->events.units.size()) || amount < 1 || amount > 999) { throw std::runtime_error("experience assistance index or amount outside range"); }
                            auto unit = prologue->events.units.at(size_t(index));
                            if (!unit.controllable() || unit.hostile() || !unit.present(prologue->battle_turn()) || unit.hp <= 0 || (unit.status_flags & 0x20) || unit.level >= 99) { throw std::runtime_error("experience assistance requires a living player character below level 99"); }
                            const auto beforeExperience = unit.experience;
                            unit.experience = std::min(999u, unit.experience + unsigned(amount));
                            const bool started = prologue->battle().mutate_random([&](s2ne::OriginalRandom& random, uint32_t seconds)
                                {
                                    return prologue->begin_reward_growth(s2ne::OriginalProgression::PendingGrowth(unit, random), seconds, [&, index, hub](bool commit)
                                        {
                                            if (!commit) { return; }
                                            const auto completed = prologue->reward_growth();
                                            prologue->events.units.at(size_t(index)) = completed.unit();
                                            prologue->battle().mutate_random([&](s2ne::OriginalRandom& current, uint32_t)
                                                {
                                                    current.restore(completed.random().state());
                                                    return true;
                                                });
                                            if (hub)
                                            {
                                                auto captured = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                                if (!captured) { s2ne::fatal(captured.error()); }
                                                *rawSave = std::move(captured).value();
                                            }
                                        });
                                });
                            if (!started) { throw std::runtime_error("experience assistance growth could not start"); }
                            response["assisted"] = true;
                            response["experience_added"] = unit.experience - beforeExperience;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "spirit_sight_confirm")
                        {
                            if (!confirmSpiritSight()) { throw std::runtime_error("no spirit sight awaiting confirmation"); }
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "growth_confirm")
                        {
                            if (!prologue || !prologue->growth_active()) { throw std::runtime_error("no growth awaiting confirmation"); }
                            prologue->confirm_growth();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "battle_confirm" || command == "battle_cancel")
                        {
                            const auto state = stdioObserve();
                            if (!prologue || !prologue->in_battle() || state.at("modal") != "none" || state.at("interaction").at("mode") != "input")
                            {
                                throw std::runtime_error("battle confirmation unavailable in current state");
                            }
                            if (prologue->battle_state() == s2ne::Battle::State::idle) { throw std::runtime_error("no battle selection to confirm or cancel"); }
                            if (command == "battle_cancel") { prologue->cancel_battle_selection(); }
                            else
                            {
                                prologue->battle_confirm();
                            }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "objectives_confirm")
                        {
                            if (!prologue || !prologue->showing_objectives()) { throw std::runtime_error("objectives are not awaiting confirmation"); }
                            prologue->advance();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "briefing_advance")
                        {
                            if (!prologue || prologue->diagnostic_phase() != 4 || deployment) { throw std::runtime_error("briefing unavailable"); }
                            prologue->advance();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "story_choose")
                        {
                            if (!prologue || !prologue->choosing() || prologue->dialogue_scrolling()) { throw std::runtime_error("original story choice is still revealing"); }
                            const int index = request->at("index").get<int>();
                            const int count = int(prologue->events.line_count(prologue->diagnostic_event())) - 1;
                            if (index < 0 || index >= count) { throw std::runtime_error("invalid original story choice index"); }
                            prologue->move(-count);
                            prologue->move(index);
                            prologue->advance();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "ending_advance")
                        {
                            if (!prologue || !prologue->ending_active() || prologue->ending_done()) { throw std::runtime_error("ending presentation unavailable"); }
                            prologue->advance();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "dialogue_advance" || command == "dialogue_reveal")
                        {
                            const auto state = stdioObserve();
                            const auto modal = state.at("modal").get<std::string>();
                            if (!prologue || prologue->in_battle() || prologue->choosing() || prologue->dialogue_scrolling() || !state.contains("dialogue") || (modal != "none" && modal != "campaign") || state.at("interaction").at("mode") != "input")
                            {
                                throw std::runtime_error("dialogue operation unavailable in current state");
                            }
                            if (command == "dialogue_reveal") { prologue->reveal_text(); }
                            else
                            {
                                prologue->advance();
                            }
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "promotion_open")
                        {
                            const bool hub = prologue && prologue->interbattle_active() && !churchMenu && !shopMenu && !innMenu;
                            if (!prologue || skillMenu || battleMenu || promotionMenu || (!hub && prologue->battle_state() != s2ne::Battle::State::command)) { throw std::runtime_error("promotion requires selected player command state or idle interbattle roster"); }
                            openSelectedPromotion(hub ? request->value("index", -1) : -1);
                            if (!promotionMenu) { throw std::runtime_error("original promotion conditions not met"); }
                            response["semantic"] = command;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "promotion_choose" || command == "promotion_train" || command == "promotion_confirm" || command == "promotion_cancel")
                        {
                            if (!promotionMenu) { throw std::runtime_error("promotion menu is not open"); }
                            if (command == "promotion_choose")
                            {
                                int index = request->value("index", -1);
                                if (promotionMenu->page() != s2ne::OriginalPromotion::Page::selection || index < 0 || size_t(index) >= promotionMenu->choices().size()) { throw std::runtime_error("invalid promotion candidate"); }
                                promotionMenu->choose(size_t(index));
                                promotionMenu->inspect();
                            }
                            else if (command == "promotion_train")
                            {
                                if (!promotionMenu->train(request->value("group", -1))) { throw std::runtime_error("original promotion training group rejected"); }
                            }
                            else if (command == "promotion_confirm")
                            {
                                if (!promotionMenu->confirm()) { throw std::runtime_error("original promotion commit rejected"); }
                            }
                            else
                            {
                                promotionMenu->cancel();
                            }
                            if (promotionMenu->page() == s2ne::OriginalPromotion::Page::closed) { promotionMenu.reset(); }
                            response["semantic"] = command;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "end_turn")
                        {
                            if (!prologue || !prologue->in_battle() || prologue->computer_actions() || battleMenu || prologue->battle_state() != s2ne::Battle::State::idle) { throw std::runtime_error("end turn requires idle player battle"); }
                            prologue->end_battle_turn();
                            response["semantic"] = "end_turn";
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "open_system_menu")
                        {
                            if (!prologue || !prologue->in_battle() || battleMenu || prologue->computer_actions())
                            {
                                throw std::runtime_error("system menu unavailable in current state");
                            }
                            float wx{}, wy{};
                            if (!SDL_RenderCoordinatesToWindow(renderer, 320.0f, 240.0f, &wx, &wy))
                            {
                                throw std::runtime_error(SDL_GetError());
                            }
                            SDL_Event event{};
                            event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                            event.button.windowID = SDL_GetWindowID(window);
                            event.button.x = wx;
                            event.button.y = wy;
                            event.button.button = SDL_BUTTON_RIGHT;
                            event.button.down = true;
                            stdioInputEvents.push_back(event);
                            event.type = SDL_EVENT_MOUSE_BUTTON_UP;
                            event.button.down = false;
                            stdioInputEvents.push_back(event);
                            response["semantic"] = "open_system_menu";
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "menu_select")
                        {
                            if (!battleMenu || (battleMenu->page() != s2ne::OriginalBattleMenu::Page::root && battleMenu->page() != s2ne::OriginalBattleMenu::Page::system))
                            {
                                throw std::runtime_error("battle root menu is not open");
                            }
                            if (!request->contains("index") || !request->at("index").is_number_integer())
                            {
                                throw std::runtime_error("menu_select requires integer index");
                            }
                            const int requested = request->at("index").get<int>();
                            const int item_count = battleMenu->rows();
                            if (requested < 0 || requested >= item_count)
                            {
                                throw std::runtime_error("battle menu index outside current page rows");
                            }
                            const int current = int(battleMenu->selected());
                            const int delta = requested - current;
                            if (battleMenu->page() == s2ne::OriginalBattleMenu::Page::root && requested == 5 && item_count < 6)
                            {
                                throw std::runtime_error("automatic battle is unavailable in current original menu");
                            }
                            battleMenu->move(delta);
                            battleMenu->activate();
                            response["semantic"] = "menu_select";
                            response["index"] = requested;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "target_select")
                        {
                            if (prologue && prologue->battle_state() == s2ne::Battle::State::attack_target)
                            {
                                if (!request->contains("index") || !request->at("index").is_number_integer()) { throw std::runtime_error("target_select requires integer unit index"); }
                                const int target = request->at("index").get<int>();
                                if (target < 0 || !prologue->battle().select_attack_target(prologue->events.units, size_t(target)) || !prologue->battle().activate(prologue->events.units)) { throw std::runtime_error("target is not selectable by original attack rules"); }
                                response["semantic"] = "target_select";
                                response["index"] = target;
                                stdioPending = response;
                                stdioReadyAt = nowUi;
                                continue;
                            }
                            if (!skillMenu || skillMenu->page() != s2ne::OriginalSkills::Page::requested)
                            {
                                throw std::runtime_error("skill target selection is unavailable");
                            }
                            if (!request->contains("index") || !request->at("index").is_number_integer())
                            {
                                throw std::runtime_error("target_select requires integer unit index");
                            }
                            const int target = request->at("index").get<int>();
                            if (target < 0 || target >= int(prologue->events.units.size()))
                            {
                                throw std::runtime_error("target index outside current roster");
                            }
                            const int actor = prologue->battle_selected();
                            if (actor < 0 || !prologue->events.units.at(size_t(actor)).present(prologue->battle_turn()))
                            {
                                throw std::runtime_error("skill actor is not selectable");
                            }
                            const auto& unit = prologue->events.units.at(size_t(target));
                            const auto& caster = prologue->events.units.at(size_t(actor));
                            if (!unit.present(prologue->battle_turn()) || (!skillMenu->healing1_in_range(caster, unit) && !skillMenu->hostile_in_range(caster, unit)))
                            {
                                throw std::runtime_error("target is not selectable by original skill rules");
                            }
                            if (skillMenu->transfer51_supported()) { throw std::runtime_error("transfer requires separate source and destination selection"); }
                            if (skillMenu->selected().id == 94 && !s2ne::OriginalSkill94Transaction::target_supported(prologue->events, unit)) { throw std::runtime_error("original skill94 target runtime unsupported"); }
                            if (!prologue->battle().target_visible(unit, std::span(&caster, 1), 0)) { throw std::runtime_error("original skill target is hidden"); }
                            prologue->battle().set_hostile_resources(prologue->events);
                            if (!prologue->battle().present_skill32_preparation(prologue->events.units, size_t(actor), size_t(target), skillMenu->selected().id == 88 || skillMenu->selected().id == 94)) { throw std::runtime_error("original skill preparation rejected"); }
                            if (skillMenu->status_area_supported() && !sleepCenter) { sleepCenter = skillMenu->selected().raw[18] == 7 ? s2ne::Battle::Point{ caster.x, caster.y } : s2ne::Battle::Point{ unit.x, unit.y }; }
                            pendingSkillTarget = size_t(target);
                            skillTarget.reset();
                            response["semantic"] = "target_select";
                            response["index"] = target;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "select_unit")
                        {
                            if (!prologue || !prologue->in_battle() || prologue->computer_actions() || prologue->battle_state() != s2ne::Battle::State::idle || skillMenu || battleMenu)
                            {
                                throw std::runtime_error("unit selection unavailable in current state");
                            }
                            if (!request->contains("index") || !request->at("index").is_number_integer())
                            {
                                throw std::runtime_error("select_unit requires integer index");
                            }
                            const int index = request->at("index").get<int>();
                            if (index < 0 || index >= int(prologue->events.units.size()))
                            {
                                throw std::runtime_error("unit index outside current roster");
                            }
                            const unsigned context = prologue->events.army_modes ? unsigned((*prologue->events.army_modes)[0]) : 0u;
                            if (!prologue->battle().select_unit(prologue->events.units, size_t(index), context))
                            {
                                const auto& unit = prologue->events.units.at(size_t(index));
                                std::string detail = "unit is not eligible under original selection rules: id=" + std::to_string(unit.id)
                                    + " action_state=" + std::to_string(unit.action_state) + " flags=" + std::to_string(unit.status_flags)
                                    + " context=" + std::to_string(context) + " x=" + std::to_string(unit.x) + " y=" + std::to_string(unit.y);
                                if (unit.action_status_runtime)
                                {
                                    for (unsigned offset : { 0x38u, 0x39u, 0x3Cu, 0x3Du, 0x3Eu, 0x3Fu })
                                    {
                                        detail += " runtime[" + std::to_string(offset) + "]=" + std::to_string(unit.action_status_runtime->at(offset));
                                    }
                                }
                                throw std::runtime_error(detail);
                            }
                            response["semantic"] = "select_unit";
                            response["index"] = index;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "target_options")
                        {
                            if (!skillMenu || skillMenu->page() != s2ne::OriginalSkills::Page::requested || prologue->battle_selected() < 0)
                            {
                                throw std::runtime_error("target options unavailable in current state");
                            }
                            const auto& actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                            response["semantic"] = "target_options";
                            response["targets"] = s2ne::StdioProtocol::Json::array();
                            for (size_t i = 0; i < prologue->events.units.size(); ++i)
                            {
                                const auto& target = prologue->events.units[i];
                                const bool valid = target.present(prologue->battle_turn()) && (skillMenu->healing1_in_range(actor, target) || skillMenu->hostile_in_range(actor, target));
                                if (!valid) { continue; }
                                response["targets"].push_back({ { "index", i }, { "x", target.x }, { "y", target.y }, { "hostile", target.hostile() } });
                            }
                        }
                        else if (command == "skill_cancel")
                        {
                            if (!skillMenu || pendingSkillTarget || itemSpellSource) { throw std::runtime_error("skill cancellation unavailable"); }
                            if (skillMenu->page() == s2ne::OriginalSkills::Page::requested) { skillMenu->resume(); }
                            else
                            {
                                skillMenu.reset();
                            }
                            sleepCenter.reset();
                            skillTarget.reset();
                            transferSource.reset();
                            transferDestination.reset();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "skill_area_select")
                        {
                            if (!prologue || !skillMenu || skillMenu->page() != s2ne::OriginalSkills::Page::requested || !skillMenu->status_area_supported() || skillMenu->directional_skill() || prologue->battle_selected() < 0 || pendingSkillTarget) { throw std::runtime_error("area skill selection unavailable"); }
                            if (!request->contains("x") || !request->contains("y") || !request->at("x").is_number_integer() || !request->at("y").is_number_integer()) { throw std::runtime_error("skill_area_select requires integer x and y"); }
                            const s2ne::Battle::Point cell{ request->at("x").get<int>(), request->at("y").get<int>() };
                            const auto actor = size_t(prologue->battle_selected());
                            const auto& caster = prologue->events.units.at(actor);
                            auto center = caster;
                            center.x = cell.x;
                            center.y = cell.y;
                            if (!prologue->battle().skill_area_cell(cell) || !skillMenu->sleep2_center_in_range(caster, center)) { throw std::runtime_error("area center outside original skill range"); }
                            std::optional<size_t> target;
                            for (size_t i = 0; i < prologue->events.units.size(); ++i)
                            {
                                const auto& unit = prologue->events.units[i];
                                if (skillMenu->status_area_target(caster, unit) && unit.present(prologue->battle_turn()) && skillMenu->sleep2_area(center, unit) && prologue->battle().skill_area_cell({ unit.x, unit.y }) && prologue->battle().target_visible(unit, std::span(&center, 1), 0))
                                {
                                    target = i;
                                    break;
                                }
                            }
                            if (!target) { throw std::runtime_error("area contains no original eligible target"); }
                            prologue->battle().set_hostile_resources(prologue->events);
                            if (!prologue->battle().present_skill32_preparation(prologue->events.units, actor, *target, false)) { throw std::runtime_error("original area preparation rejected"); }
                            sleepCenter = cell;
                            pendingSkillTarget = target;
                            skillTarget.reset();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "command_select")
                        {
                            if (!prologue || prologue->battle_state() != s2ne::Battle::State::command || prologue->battle_selected() < 0 || skillMenu)
                            {
                                throw std::runtime_error("command selection unavailable in current state");
                            }
                            if (!request->contains("index") || !request->at("index").is_number_integer()) { throw std::runtime_error("command_select requires integer index"); }
                            const int index = request->at("index").get<int>();
                            if (index < 0 || index >= 6 || !prologue->battle().command_available(prologue->events.units, index)) { throw std::runtime_error("command is disabled by original rules"); }
                            while (prologue->battle().command_index() != index) { prologue->battle().cycle_command(index > prologue->battle().command_index() ? 1 : -1); }
                            if (!prologue->battle().activate(prologue->events.units)) { throw std::runtime_error("original command activation rejected"); }
                            response["semantic"] = "command_select";
                            response["index"] = index;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "attack_select")
                        {
                            if (!prologue || prologue->battle_state() != s2ne::Battle::State::attack_menu || skillMenu || battleMenu || prologue->computer_actions()) { throw std::runtime_error("attack menu unavailable"); }
                            if (!request->contains("index") || !request->at("index").is_number_integer()) { throw std::runtime_error("attack_select requires integer index"); }
                            const int index = request->at("index").get<int>();
                            if (index < 0 || index >= 8 || !prologue->battle().attack_command_available(prologue->events.units, index)) { throw std::runtime_error("attack command disabled by original rules"); }
                            for (unsigned step = 0; step < 8 && prologue->battle().attack_index() != index; ++step) { prologue->battle_cycle(1); }
                            if (prologue->battle().attack_index() != index) { throw std::runtime_error("attack menu selection could not reach requested command"); }
                            prologue->battle_confirm();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "skill_select")
                        {
                            if (!skillMenu || skillMenu->page() != s2ne::OriginalSkills::Page::list || !prologue || prologue->battle_selected() < 0) { throw std::runtime_error("skill list unavailable"); }
                            if (!request->contains("skill_id") || !request->at("skill_id").is_number_integer()) { throw std::runtime_error("skill_select requires integer skill_id"); }
                            const int id = request->at("skill_id").get<int>();
                            auto candidate = *skillMenu;
                            if (id <= 0 || !candidate.select_record(unsigned(id))) { throw std::runtime_error("skill is not in the current original list"); }
                            const auto& actor = prologue->events.units.at(size_t(prologue->battle_selected()));
                            const auto partner = candidate.cooperative_partner(actor, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                                {
                                    return prologue->battle().cell({ x, y }).elevation;
                                });
                            const auto failure = candidate.select(actor, partner ? &prologue->events.units[*partner] : nullptr, candidate.ai_summon_available(actor, prologue->events));
                            if (failure != s2ne::OriginalSkills::Failure::none) { throw std::runtime_error("original skill selection rejected: " + std::to_string(int(failure))); }
                            *skillMenu = std::move(candidate);
                            sleepCenter.reset();
                            skillTarget.reset();
                            showPrologue();
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "move_select")
                        {
                            if (!prologue || prologue->battle_state() != s2ne::Battle::State::movement || prologue->battle_selected() < 0)
                            {
                                throw std::runtime_error("movement selection unavailable in current state");
                            }
                            if (!request->contains("x") || !request->contains("y") || !request->at("x").is_number_integer() || !request->at("y").is_number_integer())
                            {
                                throw std::runtime_error("move_select requires integer x and y");
                            }
                            const s2ne::Battle::Point cell{ request->at("x").get<int>(), request->at("y").get<int>() };
                            if (!prologue->battle().click(prologue->events.units, cell))
                            {
                                throw std::runtime_error("destination rejected by original movement rules");
                            }
                            response["semantic"] = "move_select";
                            response["x"] = cell.x;
                            response["y"] = cell.y;
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
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
                            queue_stdio_key(key);
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
                            SDL_WarpMouseInWindow(window, wx, wy);
                            SDL_Event event{};
                            event.type = SDL_EVENT_MOUSE_MOTION;
                            event.motion.windowID = SDL_GetWindowID(window);
                            event.motion.x = wx;
                            event.motion.y = wy;
                            stdioInputEvents.push_back(event);
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
                                stdioInputEvents.push_back(event);
                                event.type = SDL_EVENT_MOUSE_BUTTON_UP;
                                event.button.down = false;
                                stdioInputEvents.push_back(event);
                            }
                            stdioPending = response;
                            stdioReadyAt = nowUi;
                        }
                        else if (command == "screenshot")
                        {
                            stdioScreenshotPending = true;
                            stdioPending = response;
                            stdioReadyAt = std::numeric_limits<uint64_t>::max();
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
                    if (!stdioPending)
                    {
                        if (!response.contains("state")) { response["state"] = stdioObserve(); }
                        response["progress"] = stdioProgress.sample(response["state"], SDL_GetTicks());
                        s2ne::StdioProtocol::verify_response(response);
                        stdioRespond(response);
                        if (request->value("cmd", std::string{}) == "release_control" && response.value("ok", false)) { stdio.reset(); }
                    }
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
                    std::print("Continuous turn stage={} turn={} allies={} enemies={}\n", prologue->events.stage, smokeDriver.state.continuousDiagnosticTurn, allies, enemies);
                    if (prologue->events.stage == 12)
                    {
                        for (const auto& u : prologue->events.units)
                        {
                            if ((u.status_flags & 0x80) && !(u.status_flags & 0x20) && u.hp > 0)
                            {
                                std::print(" live={} side={} hp={} xy={},{} action={}\n", u.id, u.behavior, u.hp, u.x, u.y, unsigned(u.action_state));
                            }
                        }
                    }
                }
                if (options.campaignSmoke && options.alternateSmoke && prologue->battle_turn() != smokeDriver.state.diagnosticTurn)
                {
                    smokeDriver.state.diagnosticTurn = prologue->battle_turn();
                    std::print("Natalia easy auto diagnostic turn={} difficulty={}\n", smokeDriver.state.diagnosticTurn, prologue->events.difficulty);
                    for (const auto& u : prologue->events.units)
                    {
                        if (!u.hostile())
                        {
                            std::print(" ally={} behavior={} hp={} ap={} action={} x={} y={}\n", u.id, u.behavior, u.hp, u.action_points, unsigned(u.action_state), u.x, u.y);
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
                if (itemLearningPublish && itemLearningSince && gameTicks() - itemLearningSince >= 2500)
                {
                    itemLearningNotices.erase(itemLearningNotices.begin());
                    itemLearningSince = 0;
                    if (itemLearningNotices.empty())
                    {
                        auto publish = std::move(itemLearningPublish);
                        itemLearningPublish = {};
                        publish();
                    }
                    showPrologue();
                }
                if (skillCastVerified && !skillGrowthNotices.empty() && !prologue->growth_active() && !prologue->battle().combat_feedback_active() && !prologue->synchronous_death_active())
                {
                    auto notice = std::move(skillGrowthNotices.front());
                    skillGrowthNotices.erase(skillGrowthNotices.begin());
                    std::print("Original skill reward presentation after effect feedback/death settlement\n");
                    if (!prologue->begin_reward_growth(std::move(notice.pending), notice.seconds, [](bool committed)
                            {
                                if (!committed) { s2ne::report_error(s2ne::make_error("Skill reward presentation failed after settlement")); }
                            }))
                    {
                        s2ne::fatal("Skill reward presentation already active");
                    }
                }
                if (!deployment && !itemLearningPublish && (!battleItems || itemSpellSource) && !spiritSightImage && !battleMenu && !battleReports && !promotionMenu && !itemMenu && (!skillMenu || pendingSkillTarget || prologue->growth_active()))
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
                        std::print("Native ending presentation phase={}/{} {}\n", endingObserved + 1, prologue->ending_pages(), prologue->ending_description());
                    }
                    showPrologue();
                    if (prologue->ending_done())
                    {
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + (options.nataliaStrengthened ? "-natalia-ending-viewed.sav" : "-white-ending-viewed.sav"));
                        std::filesystem::create_directories(path.parent_path());
                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                        std::ofstream output(path, std::ios::binary);
                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                        output.close();
                        if (!output) { s2ne::fatal("Cannot persist native ending result: " + path.string()); }
                        auto loaded = s2ne::OriginalSave::load(path, saveItems);
                        if (s2ne::OriginalSave::encode(loaded, saveItems) != bytes)
                        {
                            s2ne::fatal("Ending persistent result changed on reload");
                        }
                        std::print("Native41E9CF presentation returned after movie3;40D7CB returns to WinMain428F2E cleanup/exit. Native flags42={} 43={} 73={} 75={}; no invented completion flag; checkpoint={}\n", unsigned(prologue->events.flags[41]), unsigned(prologue->events.flags[42]), unsigned(prologue->events.flags[72]), unsigned(prologue->events.flags[74]), path.string());
                        if (options.clearSmoke)
                        {
                            smokeDriver.state.clearVerified = true;
                        }
                        running = false;
                    }
                }
                if (prologue->has_battle() && prologue->campaign_blocker().empty() && aiSkillActive && !skillMenu && skillGrowthNotices.empty() && !prologue->growth_active() && !prologue->skill_visual_active() && !prologue->battle().combat_feedback_active() && !prologue->synchronous_death_active())
                {
                    prologue->battle().finish_ai_skill();
                    aiSkillActive = false;
                }
                if (prologue->has_battle() && prologue->campaign_blocker().empty() && !skillMenu && prologue->battle().ai_skill_ready() && !aiSkillActive)
                {
                    const auto proposal = *prologue->battle().ai_skill_request();
                    const auto actorIndex = prologue->battle().ai_skill_actor().value_or(size_t(prologue->battle_selected()));
                    const auto& actor = prologue->events.units.at(actorIndex);
                    s2ne::OriginalSkills::Context context{};
                    context.second = proposal.second;
                    context.computer = true;
                    context.army_da_nonzero = prologue->events.army_modes && (*prologue->events.army_modes)[actor.hostile() ? 1 : 0] != 0;
                    context.suppress_first = prologue->events.flags[8] != 0;
                    context.suppress_second = prologue->events.flags[9] != 0;
                    skillMenu = skillRules;
                    skillMenu->set_ai_direction(proposal.direction);
                    skillMenu->open(actor, context);
                    const bool found = skillMenu->select_record(proposal.id);
                    std::optional<size_t> partner;
                    if (found)
                    {
                        partner = skillMenu->cooperative_partner(actor, prologue->events.units, prologue->battle_turn(), [&](int x, int y)
                            {
                                return prologue->battle().cell({ x, y }).elevation;
                            });
                    }
                    const auto selectionFailure = found ? skillMenu->select(actor, partner ? &prologue->events.units[*partner] : nullptr, skillMenu->ai_summon_available(actor, prologue->events)) : s2ne::OriginalSkills::Failure::world_unresolved;
                    if (selectionFailure != s2ne::OriginalSkills::Failure::none)
                    {
                        s2ne::report_error(s2ne::make_error("470EF6 selected AI skill failed live eligibility: skill=" + std::to_string(proposal.id)
                            + " failure=" + std::to_string(int(selectionFailure)) + " ap=" + std::to_string(actor.action_points)
                            + " mp=" + std::to_string(actor.mp) + " st=" + std::to_string(actor.st)));
                        prologue->battle().fail_ai_skill();
                        skillMenu.reset();
                    }
                    else
                    {
                        if (!prologue->battle().present_skill32_preparation(prologue->events.units, actorIndex, proposal.target, proposal.second))
                        {
                            s2ne::report_error(s2ne::make_error("46C078 AI skill preparation unavailable: skill=" + std::to_string(proposal.id)));
                            prologue->battle().fail_ai_skill();
                            skillMenu.reset();
                            continue;
                        }
                        aiSkillActive = true;
                        prologue->battle().begin_ai_skill();
                        pendingSkillTarget = proposal.target;
                        if (skillMenu->status_area_supported()) { sleepCenter = s2ne::Battle::Point{ proposal.center_x, proposal.center_y }; }
                        pendingSkillVisualStarted = false;
                    }
                }
                if (prologue->campaign_blocker().empty() && skillMenu && pendingSkillTarget && !prologue->battle().animating_movement() && !pendingSkillVisualStarted)
                {
                    const auto& target = prologue->events.units.at(*pendingSkillTarget);
                    if (!prologue->prepare_skill_camera(target.x, target.y))
                    {
                        showPrologue();
                    }
                    else
                    {
                        skillCastVerified = false;
                        skillGrowthNotices.clear();
                        const auto visualActorIndex = prologue->battle().ai_skill_actor().value_or(size_t(prologue->battle_selected()));
                        const auto& visualActor = prologue->events.units.at(visualActorIndex);
                        std::print("Original AI visual context actor_index={} actor_id={} target_index={} target_id={} selected_skill={}\n", visualActorIndex, visualActor.id, *pendingSkillTarget, target.id, skillMenu->selected().id);
                        bool visualStarted;
                        std::string visualError;
                        const char* visualDispatch = "skill32";
                        if (skillMenu->selected().id == 94) { visualStarted = prologue->begin_skill94_visual(unsigned(visualActorIndex), unsigned(*pendingSkillTarget)); }
                        else if (skillMenu->summon_supported() || skillMenu->transfer51_supported() || skillMenu->selected().id == 88 || skillMenu->selected().id == 1 || skillMenu->status_area_supported())
                        {
                            visualDispatch = "skill88";
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
                            const unsigned performer = unsigned(visualActorIndex);
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
                            visualStarted = prologue->begin_skill88_visual(visual_target, skillMenu->selected().id == 1, skillMenu->selected().id == 2, sleepCenter, skillMenu->selected().id == 3, status_skill, (partner || skillMenu->fissure29_supported() || skillMenu->drain_supported() || NativeDamageRecords::find(status_skill)) ? performer : visual_target, partnerVisual, std::move(targets), &visualError);
                        }
                        else
                        {
                            visualStarted = prologue->begin_skill32_visual(unsigned(visualActorIndex), unsigned(*pendingSkillTarget), skillMenu->selected().id == 33, &visualError);
                        }

                        pendingSkillVisualStarted = visualStarted;
                        if (pendingSkillVisualStarted)
                        {
                            nowUi = gameTicks();
                        }
                        if (!pendingSkillVisualStarted)
                        {
                            const auto failedActorIndex = prologue->battle_selected();
                            const auto failedActor = prologue->events.units.at(size_t(failedActorIndex));
                            const auto failedTarget = prologue->events.units.at(*pendingSkillTarget);
                            const auto failureContext = " actor_index=" + std::to_string(failedActorIndex) + " actor_id=" + std::to_string(failedActor.id)
                                + " target_index=" + std::to_string(*pendingSkillTarget) + " target_id=" + std::to_string(failedTarget.id)
                                + " selected_skill=" + std::to_string(skillMenu->selected().id);
                            if (aiSkillActive)
                            {
                                prologue->battle().fail_ai_skill();
                                aiSkillActive = false;
                                pendingSkillTarget.reset();
                                skillMenu.reset();
                            }
                            pendingSkillTarget.reset();
                            skillMenu.reset();
                            skillTarget.reset();
                            if (itemSpellSource && battleItems)
                            {
                                itemSpellSource.reset();
                                battleItems->return_from_target(*rawSave);
                            }
                            if (visualError.empty()) { visualError = "visual dispatch returned false without an error"; }
                            std::print(stderr, "Original {} visual rejected; no effect/payment committed reason={}{}\n", visualDispatch, visualError, failureContext);
                            if (aiSkillActive && visualDispatch != std::string("skill32"))
                            {
                                prologue->set_campaign_blocker("Original " + std::string(visualDispatch) + " visual transaction rejected; reload required: " + visualError);
                                aiSkillActive = false;
                            }
                            if (!visualError.empty() && prologue->campaign_blocker().empty())
                            {
                                prologue->set_campaign_blocker("Original " + std::string(visualDispatch) + " visual transaction rejected; reload required: " + visualError);
                                aiSkillActive = false;
                            }
                        }
                    }
                }
                if (skillMenu && pendingSkillTarget && pendingSkillVisualStarted && !prologue->skill_visual_active() && !prologue->growth_active())
                {
                    pendingSkillVisualStarted = false;
                    int selected = prologue->battle_selected();
                    auto target = *pendingSkillTarget;
                    pendingSkillTarget.reset();
                    s2ne::OriginalSkills::HostileResult result;
                    int pendingMpGrowth = 0;
                    bool swordFeedbackPending = false;
                    size_t growthAwardCursor = 0;
                    bool applied = false;
                    bool growthSuspended = false;
                    try
                    {
                        applied = prologue->battle().mutate_combat_random([&](auto& rng, uint32_t seconds)
                            {
                                if (!skillGrowthSeconds) { skillGrowthSeconds = seconds; }
                                seconds = *skillGrowthSeconds;
                                s2ne::OriginalSkills::ExecutionContext context;
                                context.difficulty = prologue->events.difficulty;
                                context.friendly_army = !prologue->events.units.at(size_t(selected)).hostile();
                                context.seconds = seconds;
                                context.skill_payment_state = skillMenu->initial_payment_state(skillMenu->selected().id);
                                auto award = [&](auto& unit, auto& stream, int xp, int proficiency, unsigned group)
                                {
                                    if (options.skillControl == "rollback") { return false; }
                                    if (growthAwardCursor < skillGrowthAwards.size())
                                    {
                                        const auto& saved = skillGrowthAwards[growthAwardCursor++];
                                        if (options.skillInputSmoke) { pendingMpGrowth += saved.unit.mp - unit.mp; }
                                        unit = saved.unit;
                                        stream.restore_stream(saved.random.seed, saved.random.calls);
                                        return true;
                                    }
                                    // 4659BD skips capped proficiency, not the remaining XP/payment transaction.
                                    const auto beforeLearning = unit;
                                    // 原4659BD电脑施法跳熟练；458B03军DA==1仍累积经验并调用459309。
                                    if (!aiSkillActive && !skillMenu->item_source()) { promotionTransaction.award_proficiency(unit, 6 + group, unsigned(proficiency)); }
                                    auto learnedName = promotionTransaction.newly_learned_name(beforeLearning, unit, group);
                                    s2ne::OriginalRandom native({ stream.seed_state(), stream.draw_count() });
                                    s2ne::OriginalProgression progression(root);
                                    int mpBeforeGrowth = unit.mp;
                                    const bool hostileReward = unit.hostile() && prologue->events.army_modes && (*prologue->events.army_modes)[1] == 1;
                                    if (unit.hostile() && !hostileReward)
                                    {
                                        skillGrowthAwards.push_back({ unit, native.state() });
                                        ++growthAwardCursor;
                                        return true;
                                    }
                                    auto pending = progression.stage(unit, native, xp, hostileReward, prologue->events.difficulty, true);
                                    if (!learnedName.empty() && unit.controllable() && !(unit.status_flags & 0x20) && (unit.status_flags & 0x80))
                                    {
                                        pending.set_learning_name(std::move(learnedName));
                                    }
                                    if (pending.experience_notice() > 0 || !pending.learning_name().empty() || (pending.unit().experience >= pending.unit().experience_threshold && pending.unit().level < 99))
                                    {
                                        skillGrowthNotices.push_back({ std::move(pending), seconds });
                                    }
                                    if (!progression.award_native(unit, native, xp, promotionRules, [seconds]
                                                        {
                                                            return seconds;
                                                        },
                                                        0, hostileReward, prologue->events.difficulty)
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
                                    skillGrowthAwards.push_back({ unit, state });
                                    ++growthAwardCursor;
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
                                    sword.player_controlled = prologue->battle().player_controlled(actor, size_t(selected));
                                    sword.original_second_partition = true;
                                    sword.target_runtime_known = true;
                                    sword.synchronous_death_ready = prologue->synchronous_death_supported(victim.id);
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
                                                    publishItemSpell();
                                                    prologue->battle().commit_external_action();
                                                    skillCastVerified = true;
                                                    *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                                    std::print("Original94 feedback/death complete; staged HP/reward/ST/AP/inventory/RNG published once\n");
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
                                                    std::print("Original94 secondary468BAC revived={} reverse={} consumed={} healing={}\n", outcome.revived, outcome.reverse_heal, outcome.consumed_item, healing);
                                                }
                                                else
                                                {
                                                    publish();
                                                }
                                            },
                                            1, 0, 0, false, outcome.hit && outcome.damage != 0);
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
                                        std::print("Original94 accepted and suspended for468BAC numbers; no live payment yet\n");
                                    }
                                    std::print("Original94 damage={} xp={} proficiency={} runtime39={} masks={},{} failure={}\n", outcome.damage, outcome.raw_xp, outcome.proficiency, sword.damage.target_status39, masks[0], masks[1], int(outcome.failure));
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
                                        for (const auto& victim : victims) { std::print("CONTROLLED viewport target={} hp={} status39={}\n", victim.id, victim.hp, unsigned(victim.action_status_runtime->at(0x39))); }
                                    }
                                    std::vector<s2ne::OriginalSkill94Transaction::Context> contexts;
                                    for (const auto& victim : victims)
                                    {
                                        s2ne::OriginalSkill94Transaction::Context damage;
                                        damage.computer = aiSkillActive;
                                        damage.player_controlled = prologue->battle().player_controlled(a, size_t(selected));
                                        damage.damage.friendly_army = !a.hostile();
                                        damage.original_second_partition = skillMenu->request()->second;
                                        damage.target_runtime_known = victim.action_status_runtime.has_value();
                                        damage.synchronous_death_ready = prologue->synchronous_death_supported(victim.id);
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
                                    auto inventory = rawSave->groups[a.hostile() ? 1 : 0].inventory;
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
                                        presentation->push_back({ unsigned(indices[n]), prologue->events.units[indices[n]].hp, outcome.damage, outcome.revived ? victims[n].max_hp : outcome.healed, outcome.hp_snapshot, outcome.killed, outcome.revived, outcome.reverse_heal, victims[n], 1, 0, 0, false, outcome.hit && outcome.damage != 0 });
                                    }
                                    auto seed = stream.seed_state(), calls = stream.draw_count();
                                    swordFeedbackPending = prologue->begin_damage_area_feedback(unsigned(selected), presentation, 0,
                                        [&, selected, a = std::move(a), victims = std::move(victims), inventory = std::move(inventory), indices, seed, calls, partnerIndex, partner = std::move(partner)]() mutable
                                        {
                                            a.animation_action = 0;
                                            auto& liveActor = prologue->events.units.at(size_t(selected));
                                            if (liveActor.id == a.id && liveActor.appearance == a.appearance) { liveActor = std::move(a); }
                                            if (partnerIndex && partner)
                                            {
                                                partner->animation_action = 0;
                                                auto& livePartner = prologue->events.units.at(*partnerIndex);
                                                if (livePartner.id == partner->id && livePartner.appearance == partner->appearance) { livePartner = std::move(*partner); }
                                            }
                                            for (size_t n = 0; n < indices.size(); ++n)
                                            {
                                                auto& liveVictim = prologue->events.units[indices[n]];
                                                // 同步死亡脚本461215已重置的槽位不能被施法前快照复活。
                                                if (liveVictim.id == victims[n].id && liveVictim.appearance == victims[n].appearance) { liveVictim = std::move(victims[n]); }
                                                const auto& unit = prologue->events.units[indices[n]];
                                                if (unit.status_flags & 0x20) { prologue->events.release_summon_slot(unit); }
                                            }
                                            rawSave->groups[prologue->events.units.at(size_t(selected)).hostile() ? 1 : 0].inventory = std::move(inventory);
                                            prologue->events.rebuild_army_modes(0);
                                            prologue->battle().set_hostile_resources(prologue->events);
                                            prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                                {
                                                    live.restore_stream(seed, calls);
                                                    return true;
                                                });
                                            publishItemSpell();
                                            prologue->battle().commit_external_action();
                                            skillCastVerified = true;
                                            *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                            std::print("Original466082 area damage published once targets={}\n", indices.size());
                                        });
                                    if (!swordFeedbackPending) { s2ne::fatal("466082 feedback staging rejected"); }
                                    return false;
                                }
                                auto drainInventory = rawSave->groups[a.hostile() ? 1 : 0].inventory;
                                std::optional<s2ne::OriginalSave::State> summonedSave;
                                std::optional<s2ne::OriginalEvents> summonedEvents;
                                unsigned summonedIndex = 0;
                                s2ne::OriginalSkills::SummonAdapter summonAdapter;
                                if (handler == s2ne::OriginalSkills::Handler::summon465E04)
                                {
                                    summonAdapter = [&](const auto& caster, unsigned slot, auto& stagedRandom)
                                    {
                                        if (skillGrowthSummon)
                                        {
                                            skillGrowthSummon->resume(summonedSave, summonedEvents, summonedIndex, stagedRandom);
                                            return true;
                                        }
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
                                        skillGrowthSummon = s2ne::OriginalSpellSummonCandidate{ *summonedSave, *summonedEvents, summonedIndex, growth.state().seed, growth.state().calls };
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
                                            publishItemSpell();
                                            prologue->battle().commit_external_action();
                                            skillCastVerified = true;
                                            std::print("Original465E04 summon roster/payment published once\n");
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
                                                publishItemSpell();
                                                prologue->battle().commit_external_action();
                                                skillCastVerified = true;
                                                *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                                std::print("Original45DB10 transfer arrival published once\n");
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
                                        std::print("Original467625 staged damage={} revived={} reverse={} item={}\n", outcome.damage, outcome.revived, outcome.reverse, outcome.consumed_item);
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
                                            rawSave->groups[prologue->events.units.at(size_t(selected)).hostile() ? 1 : 0].inventory = std::move(inventory);
                                            prologue->events.rebuild_army_modes(0);
                                            prologue->battle().set_hostile_resources(prologue->events);
                                            prologue->battle().mutate_combat_random([&](auto& live, uint32_t)
                                                {
                                                    live.restore_stream(seed, calls);
                                                    return true;
                                                });
                                            publishItemSpell();
                                            prologue->battle().commit_external_action();
                                            skillCastVerified = true;
                                            *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                            std::print("Original467625 drain feedback/payment/reward published once\n");
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
                                        publishItemSpell();
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
                                std::print("AREA production skill={} center={},{} targets={} stagedRNG={} unpublished=1\n", skillMenu->selected().id, center.x, center.y, indices.size(), stream.draw_count() - rng.draw_count());
                                for (size_t n = 0; n < indices.size(); ++n) { std::print("AREA target={} xy={},{} feedback={} status3e={} status3f={}\n", victims[n].id, victims[n].x, victims[n].y, int(result.target_hits[n]), unsigned(victims[n].status3e), unsigned(victims[n].status3f)); }
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
                                        publishItemSpell();
                                        prologue->battle().commit_external_action();
                                        skillCastVerified = true;
                                        *rawSave = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                                        std::print("Original area feedback complete; all statuses/timers/payment/reward/RNG published once targets={}\n", indices.size());
                                    },
                                    healing || status || revealArmy, result.target_amounts, healing);
                                return false;
                            });
                    } catch (SkillGrowthPause& pause)
                    {
                        growthSuspended = true;
                        if (!prologue->begin_reward_growth(std::move(pause.pending), pause.seconds, [&, target](bool commit)
                                {
                                    if (!commit)
                                    {
                                        skillGrowthAwards.clear();
                                        skillGrowthSeconds.reset();
                                        skillGrowthSummon.reset();
                                        if (!applied && !swordFeedbackPending) { skillGrowthNotices.clear(); }
                                        pendingSkillTarget.reset();
                                        if (aiSkillActive) { prologue->battle().fail_ai_skill(); }
                                        skillMenu.reset();
                                        return;
                                    }
                                    const auto& completed = prologue->reward_growth();
                                    skillGrowthAwards.push_back({ completed.unit(), completed.random().state() });
                                    pendingSkillTarget = target;
                                    pendingSkillVisualStarted = true;
                                }))
                        {
                            s2ne::fatal("Reward growth continuation already active");
                        }
                    }
                    if (growthSuspended)
                    {
                        showPrologue();
                        continue;
                    }
                    skillGrowthAwards.clear();
                    skillGrowthSeconds.reset();
                    skillGrowthSummon.reset();
                    std::print("Original skill{} target={} applied={} hit={} immune={} chance={} roll={} removed={} failure={}\n", skillMenu->selected().id, prologue->events.units.at(target).id, applied, result.hit, result.immune, result.chance, result.roll, result.removed, int(result.failure));
                    if (applied)
                    {
                        smokeDriver.state.skillInputMpGrowth += pendingMpGrowth;
                        publishItemSpell();
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
                            std::print("SDL hostile32 normal input cast save/reload verified checkpoint={} (not ending)\n", path.string());
                        }
                        *rawSave = std::move(snapshot);
                    }
                    if (aiSkillActive && !applied && !swordFeedbackPending)
                    {
                        s2ne::report_error(s2ne::make_error("AI cast transaction rejected; controller remains locked"));
                        prologue->set_campaign_blocker("AI cast transaction rejected; no effect/payment committed; reload required");
                        prologue->battle().fail_ai_skill();
                    }
                    if (itemSpellSource && !applied && !swordFeedbackPending && !prologue->skill_visual_active())
                    {
                        itemSpellSource.reset();
                        if (battleItems) { battleItems->return_from_target(*rawSave); }
                    }
                    skillMenu.reset();
                    sleepCenter.reset();
                    skillTarget.reset();
                    transferSource.reset();
                    transferDestination.reset();
                    // 原技能反馈完成后同一帧即可回到自动战斗；仅在所有展示/成长/反馈事务均已结束时清理请求。
                    if (aiSkillActive && skillGrowthNotices.empty() && !prologue->growth_active() && !prologue->skill_visual_active()
                        && !prologue->battle().combat_feedback_active() && !prologue->synchronous_death_active())
                    {
                        prologue->battle().finish_ai_skill();
                        aiSkillActive = false;
                    }
                    showPrologue();
                }
                if (!spiritSightImage && rawSave && prologue->in_battle() && !prologue->battle().animating_movement() && !prologue->battle().combat_feedback_active())
                {
                    for (size_t i = 0; i < prologue->events.units.size(); ++i)
                    {
                        if (auto id = prologue->pickup_at_unit(i, rawSave->groups[0].inventory, rawSave->groups[0].metadata[4]))
                        {
                            prologue->battle().refresh_command_availability(prologue->events.units);
                            std::print("Original category12 pickup item {} by unit {}\n", id, prologue->events.units[i].id);
                        }
                    }
                }
                float x{}, y{}, rx{}, ry{};
                SDL_GetMouseState(&x, &y);
                bool edgeActive = !spiritSightImage && !battleMenu && !battleReports && !promotionMenu && !itemMenu && !saveMenu && !overwriteConfirmation && !options.battleSmoke && !options.skillInputSmoke && prologue->in_battle() && !prologue->battle().animating_movement() && (prologue->battle_state() == s2ne::Battle::State::idle || prologue->battle_state() == s2ne::Battle::State::movement || prologue->battle_state() == s2ne::Battle::State::attack_target) && SDL_GetMouseFocus() == window && SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry) && rx >= 0 && rx < 640 && ry >= 0 && ry < 480 && (rx < 10 || rx > 630 || ry < 10 || ry > 470);
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
            std::deque<SDL_Event> orderedEvents;
            if (stdio)
            {
                std::vector<SDL_Event> ordinary;
                while (SDL_PollEvent(&event)) { ordinary.push_back(event); }
                orderedEvents = s2ne::stdio_event_order(stdioInputEvents, ordinary);
                stdioInputEvents.clear();
            }
            auto nextEvent = [&]()
            {
                if (!stdio) { return SDL_PollEvent(&event); }
                if (orderedEvents.empty()) { return false; }
                event = orderedEvents.front();
                orderedEvents.pop_front();
                return true;
            };
            while (nextEvent())
            {
                if (stdio && (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED))
                {
                    running = false;
                    break;
                }
                if (!stdio && (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED))
                {
                    if (suppressNextCloseEvent)
                    {
                        suppressNextCloseEvent = false;
                        continue;
                    }
                    requestExit(true);
                    if (!running)
                    {
                        break;
                    }
                    continue;
                }
                if (prologue && prologue->growth_active() && event.type != SDL_EVENT_QUIT)
                {
                    if ((event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { prologue->confirm_growth(); }
                    showPrologue();
                    continue;
                }
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
                    confirmSpiritSight();
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
                if (prologue && prologue->temporary_text_active())
                {
                    if (event.type == SDL_EVENT_QUIT) { running = false; }
                    else if ((event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) || (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT)) { prologue->cancel_temporary_text(); }
                    continue;
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
                            std::print("Native ending normal input completed phase={}/{}\n", page + 1, prologue->ending_pages());
                        }
                        showPrologue();
                    }
                    continue;
                }
                if (prologue && prologue->item_presentation_) { continue; }
                if (itemLearningPublish) { continue; }
                if (deployment)
                {
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned code = 0;
                        switch (event.key.key)
                        {
                        case SDLK_ESCAPE: code = 1; break;
                        case SDLK_LEFT: code = 0x4B; break;
                        case SDLK_RIGHT: code = 0x4D; break;
                        case SDLK_UP: code = 0x48; break;
                        case SDLK_DOWN: code = 0x50; break;
                        case SDLK_TAB: code = 0x0F; break;
                        case SDLK_RETURN: code = 0x1C; break;
                        case SDLK_SPACE: code = 0x39; break;
                        }
                        deployment->scan(code);
                    }
                    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                        float y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                        float rx{}, ry{};
                        SDL_RenderCoordinatesFromWindow(renderer, x, y, &rx, &ry);
                        deployment->mouse(int(rx), int(ry), event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT);
                    }
                    showPrologue();
                    continue;
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
                        prologue->restore_battle(std::move(restored.events), restored.turn, true, !restored.raw.battle_context());
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
                        std::print(stderr, "Original inn unavailable: {}\n", innMenu->blocker());
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
                if (battleItems && !itemTargetRequest && !itemSpellSource && prologue && rawSave)
                {
                    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                    {
                        unsigned scan = 0;
                        if (event.key.key == SDLK_ESCAPE) { scan = 1; }
                        else if (event.key.key == SDLK_RETURN) { scan = 0x1C; }
                        else if (event.key.key == SDLK_SPACE) { scan = 0x39; }
                        else if (event.key.key == SDLK_LEFT) { scan = 0x4B; }
                        else if (event.key.key == SDLK_RIGHT) { scan = 0x4D; }
                        battleItems->scan(scan, *rawSave);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_ConvertEventToRenderCoordinates(renderer, &event);
                        bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                        battleItems->mouse(int(down ? event.button.x : event.motion.x), int(down ? event.button.y : event.motion.y),
                            down && event.button.button == SDL_BUTTON_LEFT, down && event.button.button == SDL_BUTTON_RIGHT, *rawSave);
                    }
                    if (battleItems->closed() && !itemTargetRequest && !itemSpellSource) { battleItems.reset(); }
                    showPrologue();
                    continue;
                }
                if (itemTargetRequest && prologue && rawSave)
                {
                    if (itemNoticeMilliseconds)
                    {
                        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                        {
                            itemNoticeMilliseconds = 0;
                            showPrologue();
                        }
                        continue;
                    }
                    bool cancel = event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE;
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || !itemUse.targeted(itemTargetRequest->second))
                    {
                        const bool selfUse = !itemUse.targeted(itemTargetRequest->second);
                        cancel = !selfUse && event.button.button == SDL_BUTTON_RIGHT;
                        if (selfUse || event.button.button == SDL_BUTTON_LEFT)
                        {
                            if (!selfUse) { SDL_ConvertEventToRenderCoordinates(renderer, &event); }
                            auto [actor, id] = *itemTargetRequest;
                            auto snapshot = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                            if (!snapshot) { s2ne::report_error(snapshot.error()); }
                            else
                            {
                                const auto camera = prologue->camera();
                                const auto cell = prologue->battle().pick_surface(event.button.x, event.button.y, camera.first, camera.second);
                                if (!selfUse && !prologue->battle().skill_area_cell(cell))
                                {
                                    showPrologue();
                                    continue;
                                }
                                for (size_t target = 0; target < prologue->events.units.size(); ++target)
                                {
                                    const auto& unit = prologue->events.units[target];
                                    if (selfUse ? target != actor : unit.x != cell.x || unit.y != cell.y) { continue; }
                                    if (!(unit.status_flags & 0x80) || ((unit.status_flags & 0x20) && !(unit.status_flags & 2))) { continue; }
                                    if (!itemUse.target_allowed(snapshot.value(), actor, target, id))
                                    {
                                        itemNoticeMilliseconds = 1;
                                        break;
                                    }
                                    useOriginalItemTarget(actor, id, target);
                                    break;
                                }
                            }
                        }
                    }
                    if (cancel)
                    {
                        itemTargetRequest.reset();
                        if (battleItems) { battleItems->return_from_target(*rawSave); }
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
                        std::print(stderr, "Original church unavailable: {}\n", churchMenu->blocker());
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
                    shopMenu->clock(options.headlessSpeed ? unsigned(gameTicks() / 10) : s2ne::OriginalShopMenu::wall_clock_cs());
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
                            if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && !itemSpellSource)
                            {
                                skillMenu->resume();
                                sleepCenter.reset();
                                skillTarget.reset();
                                showPrologue();
                                continue;
                            }
                            skillMenu.reset();
                            transferSource.reset();
                            transferDestination.reset();
                            sleepCenter.reset();
                            skillTarget.reset();
                            if (itemSpellSource && battleItems)
                            {
                                itemSpellSource.reset();
                                battleItems->return_from_target(*rawSave);
                            }
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
                                skillMenu->navigate(scan, (event.key.mod & SDL_KMOD_SHIFT) != 0);
                                playEffect(100);
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
                            if (skillMenu->page() == s2ne::OriginalSkills::Page::requested && !itemSpellSource)
                            {
                                skillMenu->resume();
                                sleepCenter.reset();
                                skillTarget.reset();
                                showPrologue();
                                continue;
                            }
                            skillMenu.reset();
                            transferSource.reset();
                            transferDestination.reset();
                            sleepCenter.reset();
                            skillTarget.reset();
                            if (itemSpellSource && battleItems)
                            {
                                itemSpellSource.reset();
                                battleItems->return_from_target(*rawSave);
                            }
                            showPrologue();
                            continue;
                        }
                        int x = int(click ? event.button.x : event.motion.x), y = int(click ? event.button.y : event.motion.y);
                        if (skillMenu->page() == s2ne::OriginalSkills::Page::list)
                        {
                            const auto oldGroup = skillMenu->selected_group();
                            const auto oldSkill = skillMenu->selected_skill();
                            confirm = skillMenu->mouse(x, y, click && event.button.button == SDL_BUTTON_LEFT);
                            if (oldGroup != skillMenu->selected_group() || oldSkill != skillMenu->selected_skill() || confirm) { playEffect(100); }
                        }
                        else
                        {
                            auto camera = prologue->camera();
                            auto cell = prologue->battle().pick_surface(float(x), float(y), camera.first, camera.second);
                            int target = prologue->battle().occupant(prologue->events.units, cell);
                            skillTarget.reset();
                            sleepCenter.reset();
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
                            else if (skillMenu->status_area_supported() && !skillMenu->directional_skill())
                            {
                                auto center = prologue->events.units.at(size_t(selected));
                                center.x = cell.x;
                                center.y = cell.y;
                                if (skillMenu->sleep2_center_in_range(prologue->events.units.at(size_t(selected)), center))
                                {
                                    sleepCenter = cell;
                                    for (size_t i = 0; i < prologue->events.units.size(); ++i)
                                    {
                                        const auto& u = prologue->events.units[i];
                                        if (skillMenu->status_area_target(prologue->events.units.at(size_t(selected)), u) && u.present(prologue->battle_turn()) && skillMenu->sleep2_area(center, u) && prologue->battle().skill_area_cell({ u.x, u.y }) && prologue->battle().target_visible(u, std::span(&center, 1), 0))
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
                                std::print(stderr, "Original spell selection rejected reason={}\n", int(failure));
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
                                std::print(stderr, "Original selected spell executor unavailable id={}\n", skillMenu->selected().id);
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
                                std::print("Original94 unsupported death/runtime target rejected before preparation/payment\n");
                                continue;
                            }
                            if (!prologue->battle().target_visible(target, std::span(&caster, 1), 0))
                            {
                                std::print("Original spell target hidden\n");
                                continue;
                            }
                            prologue->battle().set_hostile_resources(prologue->events);
                            if (!prologue->battle().present_skill32_preparation(prologue->events.units, size_t(selected), *skillTarget, (skillMenu->selected().id == 88 || skillMenu->selected().id == 94)))
                            {
                                std::print("Original46C078 spell32 preparation unavailable\n");
                                continue;
                            }
                            if (skillMenu->status_area_supported() && !skillMenu->directional_skill() && !sleepCenter) { sleepCenter = skillMenu->selected().raw[18] == 7 ? s2ne::Battle::Point{ caster.x, caster.y } : s2ne::Battle::Point{ target.x, target.y }; }
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
                                    std::print("Original USE learning item {} committed for unit {}\n", usableItems.at(usableItemSelection), unit.id);
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
                                        std::print("SDL final quest partial: original pickup301 USE65 learned32 persisted and reloaded at turn={} checkpoint={}; continuing normal-input hostile32 quest, not an ending\n", prologue->battle_turn(), path.string());
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
                        case SDLK_PAGEUP: input.scan = 0x49; break;
                        case SDLK_PAGEDOWN: input.scan = 0x51; break;
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
                        auto loadResult = s2ne::OriginalSave::try_load(s2ne::OriginalSave::slot_path(isolatedSaveRoot ? *isolatedSaveRoot : root, result.slot), saveItems);
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
                            auto stoppedMusic = audio.stop_music();
                            if (!stoppedMusic)
                            {
                                s2ne::report_error(stoppedMusic.error());
                                saveMenu->complete(false);
                                continue;
                            }
                            loaded->restore_battle(std::move(restored.events), restored.turn, true, !restored.raw.battle_context());
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
                        auto stopped = stopEffects();
                        if (!stopped) { s2ne::report_error(stopped.error()); }
                        promotionMenu.reset();
                    }
                    showPrologue();
                    continue;
                }
                if (battleMenu)
                {
                    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                    }
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
                        case SDLK_PAGEUP: input.scan = 0x49; break;
                        case SDLK_PAGEDOWN: input.scan = 0x51; break;
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
                            else if (input.scan == 0x4b || input.scan == 0x4d || input.scan == 0x0f)
                            {
                                if (input.scan == 0x0f) { playEffect(100); }
                                overwriteConfirmation->toggle();
                            }
                            else if (input.scan == 0x1c || input.scan == 0x39)
                            {
                                if (input.scan == 0x39) { playEffect(100); }
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
                            auto stopped = stopEffects();
                            if (!stopped) { s2ne::report_error(stopped.error()); }
                        }
                        if (result.action == s2ne::OriginalSaveMenu::Action::load || result.action == s2ne::OriginalSaveMenu::Action::save)
                        {
                            auto saveResult = [&]() -> s2ne::Result<bool>
                            {
                                if (!prologue->original_save_boundary_ready())
                                {
                                    return s2ne::Result<bool>::failure(s2ne::make_error("Original synchronous settlement in progress"));
                                }
                                auto path = s2ne::OriginalSave::slot_path(isolatedSaveRoot ? *isolatedSaveRoot : root, result.slot);
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
                                    prologue->restore_battle(std::move(restored.events), restored.turn, true, !restored.raw.battle_context());
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
                        const auto oldPage = battleMenu->page();
                        const auto oldSelection = battleMenu->selected();
                        battleMenu->event(input);
                        if ((oldPage == s2ne::OriginalBattleMenu::Page::root || oldPage == s2ne::OriginalBattleMenu::Page::overview || oldPage == s2ne::OriginalBattleMenu::Page::system) && battleMenu->page() == oldPage && battleMenu->selected() != oldSelection) { playEffect(100); }
                        if (oldPage != s2ne::OriginalBattleMenu::Page::confirmation && battleMenu->page() == s2ne::OriginalBattleMenu::Page::confirmation)
                        {
                            auto stopped = stopEffects();
                            if (!stopped) { s2ne::report_error(stopped.error()); }
                        }
                        if (oldPage == s2ne::OriginalBattleMenu::Page::confirmation && input.type == 1 && (input.scan == 0x39 || input.scan == 0x0f)) { playEffect(100); }
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
                        auto report_background = prologue->draw(menu);
                        battleReports.emplace(root, page, prologue->events, std::move(entries), std::move(report_background), &overview, settingsState.small_font != 0);
                    }
                    if (!saveMenu && (request.action == s2ne::OriginalBattleMenu::Action::load || request.action == s2ne::OriginalBattleMenu::Action::save))
                    {
                        saveMenu.emplace(root, request.action == s2ne::OriginalBattleMenu::Action::save ? 1 : 0, s2ne::OriginalSaveMenu::scan(isolatedSaveRoot ? *isolatedSaveRoot : root));
                    }
                    if (auto code = battleMenu->return_code())
                    {
                        settingsState = battleMenu->settings_state();
                        prologue->set_small_font(settingsState.small_font != 0);
                        battleMenu.reset();
                        prologue->set_battle_menu_overlay(false);
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
                        if (prologue->in_battle() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT && !prologue->computer_actions() && prologue->battle_state() == s2ne::Battle::State::idle)
                        {
                            auto stopped = stopEffects();    // 原431DE8入口停止效果音，并非所有模态窗口都停止。
                            if (!stopped) { s2ne::report_error(stopped.error()); }
                            auto slots = s2ne::OriginalSaveMenu::scan(isolatedSaveRoot ? *isolatedSaveRoot : root);
                            s2ne::OriginalBattleMenu::Context context;
                            context.saves_present = std::any_of(slots.begin(), slots.end(), [](const auto& slot)
                                {
                                    return slot.occupied();
                                });
                            battleMenu.emplace(root, context, settingsState);
                            prologue->set_battle_menu_overlay(true);
                            showPrologue();
                            continue;
                        }
                        if ((options.skill94Input || NativeDamageRecords::ordinary_sword(options.inputSkill)) && click)
                        {
                            auto [cx, cy] = prologue->camera();
                            auto p = prologue->battle().pick_surface(x, y, cx, cy);
                            std::print("94 mouse delivered={},{} picked={},{} camera={},{} occupant={}\n", x, y, p.x, p.y, cx, cy, prologue->battle().occupant(prologue->events.units, p));
                        }
                        if (prologue->choosing() || prologue->in_battle())
                        {
                            const bool selecting = prologue->in_battle() && prologue->battle_state() == s2ne::Battle::State::idle;
                            prologue->mouse(x, y, click);
                            if (selecting && click && prologue->battle_state() == s2ne::Battle::State::command) { playEffect(126); }
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
                            saveMenu.emplace(root, 0, s2ne::OriginalSaveMenu::scan(isolatedSaveRoot ? *isolatedSaveRoot : root));
                            continue;
                        }
                        if (menu.selected == 4)
                        {
                            requestExit(false);
                            if (!running) { break; }
                            continue;
                        }
                        if (menu.selected == 3)
                        {
                            beginSettings();
                            continue;
                        }
                        std::print("Selected original menu row {}, handler 0x{:x} (not implemented)\n", menu.selected, target);
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
                            saveMenu.emplace(root, 0, s2ne::OriginalSaveMenu::scan(isolatedSaveRoot ? *isolatedSaveRoot : root));
                            continue;
                        }
                        if (menu.selected == 4)
                        {
                            requestExit(false);
                            if (!running) { break; }
                            continue;
                        }
                        if (menu.selected == 3)
                        {
                            beginSettings();
                            continue;
                        }
                        std::print("Selected original menu row {}, handler 0x{:x} (not implemented)\n", menu.selected, target);
                        SDL_SetWindowTitle(window, "Original menu selected - handler not implemented (see console)");
                    }
                    if (event.key.key == SDLK_SPACE && !inMenu)
                    {
                        paused = !paused;
                    }
                }
            }
            auto& a = clips[clip];
            if (!paused && gameTicks() >= next)
            {
                const auto frameStarted = gameTicks();
                if (!a.next(rgba))
                {
                    if (clip + 1 < clips.size())
                    {
                        if (aniAudio)
                        {
                            auto stopped = aniAudio->finish(stopEffects);
                            if (!stopped) { s2ne::report_error(stopped.error()); }
                            auto begun = aniAudio->begin(unsigned(clip + 2), 0x400);
                            if (!begun) { s2ne::report_error(begun.error()); }
                        }
                        aniFrame = 0;
                        ++clip;
                    }
                    else
                    {
                        showMenu();
                    }
                }
                else
                {
                    if (aniAudio)
                    {
                        auto advanced = aniAudio->advance(++aniFrame, playAni, stopEffects);
                        if (!advanced) { s2ne::report_error(advanced.error()); }
                    }
                    if (stdio) { stdioLogicalFrame = rgba; }
                    if (s2ne::texture_resources.active())
                    {
                        auto uploaded = s2ne::texture_resources.upload_frame(rgba);
                        if (!uploaded) { s2ne::fatal(uploaded.error()); }
                        texture = uploaded.value();
                    }
                    else if (!SDL_UpdateTexture(texture, nullptr, rgba.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
                    next = frameStarted + s2ne::original_ani_frame_ms(0xAF12, a.delay);
                }
            }
            if (!paused && aniAudio)
            {
                auto polled = aniAudio->poll(playAni);
                if (!polled) { s2ne::report_error(polled.error()); }
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
                        innMenu->set_save_root(isolatedSaveRoot ? *isolatedSaveRoot : root);
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
                        churchMenu->set_promotion_stop([&]
                            {
                                auto stopped = stopEffects();
                                if (!stopped) { s2ne::report_error(stopped.error()); }
                            });
                        churchMenu->set_promotion([&](auto& save, size_t index, const auto& request)
                            {
                                if (request.battlefield || index >= save.groups[0].units.size()) { return false; }
                                return prologue->interbattle_random([&](auto& rng, uint32_t seconds)
                                    {
                                        auto candidateRandom = rng;
                                        auto captured = s2ne::OriginalChurch::try_promote(root, save, index, request, candidateRandom, seconds);
                                        if (!captured)
                                        {
                                            s2ne::report_error(captured.error());
                                            return false;
                                        }
                                        auto validated = s2ne::OriginalSaveBridge::try_restore(root, captured.value());
                                        if (!validated)
                                        {
                                            s2ne::report_error(validated.error());
                                            return false;
                                        }
                                        auto encoded = s2ne::OriginalSave::try_encode(captured.value(), saveItems);
                                        if (!encoded)
                                        {
                                            s2ne::report_error(encoded.error());
                                            return false;
                                        }
                                        using std::swap;
                                        swap(save, captured.value());
                                        swap(prologue->events, validated.value().events);
                                        rng = candidateRandom;
                                        return true;
                                    });
                            });
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
                        std::print("Original61 post-victory route={} mode={} flags42={} flags43={} flags73={} flags75={}\n", target, (target ? s2ne::OriginalCampaign(root).stage(target).mode : 0), unsigned(prologue->events.flags[41]), unsigned(prologue->events.flags[42]), unsigned(prologue->events.flags[72]), unsigned(prologue->events.flags[74]));
                        auto snapshot = s2ne::OriginalSaveBridge::capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                        auto path = root / "S2NE" / "out" / "strengthened-flow-acceptance" / ("run-" + std::to_string(GetCurrentProcessId()) + "-stage61-boss-outcome.sav");
                        auto bytes = s2ne::OriginalSave::encode(snapshot, saveItems);
                        std::ofstream output(path, std::ios::binary);
                        output.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                        output.close();
                        std::print("Original boss-outcome checkpoint={}\n", path.string());
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
            if (prologue && !rawSave && (prologue->deployment_requested() || !prologue->autosave_requests().empty()) && prologue->events.stage != 140)
            {
                if (diagnostics) { std::print(stderr, "Campaign deployment init begin stage={}\n", prologue->events.stage); }
                auto initialized = initialization.try_fresh(root, prologue->events.stage, prologue->events.difficulty, prologue->events.flags, s2ne::OriginalInitialization::setup_from_settings(settingsState, 0));
                if (!initialized)
                {
                    s2ne::report_error(initialized.error());
                    return EXIT_FAILURE;
                }
                auto staged = std::move(initialized).value();
                if (diagnostics) { std::print(stderr, "Campaign deployment init fresh complete stage={}\n", prologue->events.stage); }
                auto restoredResult = s2ne::OriginalSaveBridge::try_restore(root, staged);
                if (!restoredResult)
                {
                    s2ne::report_error(restoredResult.error());
                    return EXIT_FAILURE;
                }
                auto restored = std::move(restoredResult).value();
                if (diagnostics) { std::print(stderr, "Campaign deployment restore complete stage={}\n", prologue->events.stage); }
                auto publication = prologue->try_normalize_campaign_roster(std::move(restored.events), 1);
                if (!publication)
                {
                    s2ne::report_error(publication.error());
                    return EXIT_FAILURE;
                }
                rawSave = std::move(staged);
                autosaveBlocked = false;
                if (diagnostics) { std::print(stderr, "Campaign deployment normalization complete stage={}\n", prologue->events.stage); }
            }
            if (prologue && rawSave && prologue->deployment_requested() && !deployment)
            {
                if (diagnostics) { std::print(stderr, "Campaign deployment object begin stage={}\n", prologue->events.stage); }
                auto captured = s2ne::OriginalSaveBridge::try_capture(prologue->events, 1, prologue->camera(), &*rawSave);
                if (!captured)
                {
                    s2ne::report_error(captured.error());
                    return EXIT_FAILURE;
                }
                deployment.emplace(root, prologue->events, std::move(captured).value(), s2ne::app_domain_for_save(*rawSave));
                if (diagnostics) { std::print(stderr, "Campaign deployment object complete stage={}\n", prologue->events.stage); }
                showPrologue();
            }
            if (deployment && deployment->done())
            {
                auto committed = deployment->prepare_commit();
                if (!committed)
                {
                    s2ne::report_error(committed.error());
                    return EXIT_FAILURE;
                }
                auto restored = s2ne::OriginalSaveBridge::try_restore(root, committed.value());
                if (!restored)
                {
                    s2ne::report_error(restored.error());
                    return EXIT_FAILURE;
                }
                auto stopped = audio.stop_music();
                if (!stopped)
                {
                    s2ne::report_error(stopped.error());
                    return EXIT_FAILURE;
                }
                *rawSave = std::move(committed).value();
                deployment.reset();
                prologue->commit_deployment(std::move(restored).value().events);
                showPrologue();
            }
            if (prologue && !deployment && !autosaveBlocked && !prologue->autosave_requests().empty())
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
                        std::print(stderr, "Original autosave {} blocked: no complete original baseline\n", request.slot);
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
                prologue->battle().set_ai_skills(skillRules);
                prologue->battle().set_ai_damage(sword94);
                prologue->battle().set_ai_skill_camera(prologue->camera());
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
                            const int selected = prologue->battle_selected();
                            if (selected < 0 || size_t(selected) >= rawSave->groups[0].units.size()) { return false; }
                            return !itemUse.choices(*rawSave, size_t(selected)).empty();
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
                const int actor = prologue->battle_selected();
                auto current = s2ne::OriginalSaveBridge::try_capture(prologue->events, prologue->battle_turn(), prologue->camera(), &*rawSave);
                if (!current) { s2ne::report_error(current.error()); }
                else if (actor >= 0)
                {
                    auto page = s2ne::OriginalArmyEquipmentMenu::create(root, current.value(), true, s2ne::app_domain_for_save(*rawSave));
                    if (!page) { s2ne::report_error(page.error()); }
                    else
                    {
                        battleItems = std::move(page).value();
                        battleItems->set_use_adapter(size_t(actor), [&](size_t role, unsigned id)
                            {
                                if (itemUse.spell(id))
                                {
                                    skillMenu = skillRules;
                                    if (!skillMenu->open_item(prologue->events.units.at(role), itemUse.spell(id)))
                                    {
                                        skillMenu.reset();
                                        return s2ne::Result<bool>::failure(s2ne::make_error("46527D item spell cooperative or invalid source rejected"));
                                    }
                                    const auto& actorUnit = prologue->events.units.at(role);
                                    if (skillMenu->select(actorUnit, nullptr, skillMenu->ai_summon_available(actorUnit, prologue->events)) != s2ne::OriginalSkills::Failure::none)
                                    {
                                        skillMenu.reset();
                                        return s2ne::Result<bool>::failure(s2ne::make_error("46527D item spell world condition rejected"));
                                    }
                                    itemSpellSource = id;
                                    return s2ne::Result<bool>::success(true);
                                }
                                itemTargetRequest = { role, id };
                                return s2ne::Result<bool>::success(true);
                            });
                        battleItems->refresh_items(current.value());
                    }
                }
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
                if (stdio) { stdioLogicalFrame = image; }
                if (s2ne::texture_resources.active())
                {
                    auto uploaded = s2ne::texture_resources.upload_frame(image);
                    if (!uploaded) { s2ne::fatal(uploaded.error()); }
                    texture = uploaded.value();
                }
                else if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) { s2ne::fatal(SDL_GetError()); }
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            if (s2ne::texture_resources.active())
            {
            }
            if (!SDL_RenderPresent(renderer))
            {
                s2ne::fatal(SDL_GetError());
            }
            if (stdioScreenshotPending && stdioPending)
            {
                std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_RenderReadPixels(renderer, nullptr), SDL_DestroySurface);
                if (!surface)
                {
                    (*stdioPending)["ok"] = false;
                    (*stdioPending)["error"] = SDL_GetError();
                }
                else
                {
                    const auto path = *isolatedSaveRoot / "frame.bmp";
                    if (!SDL_SaveBMP(surface.get(), path.string().c_str()))
                    {
                        (*stdioPending)["ok"] = false;
                        (*stdioPending)["error"] = SDL_GetError();
                    }
                    else
                    {
                        (*stdioPending)["path"] = path.string();
                    }
                }
                stdioScreenshotPending = false;
                stdioReadyAt = gameTicks();
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
                        std::print("SDL SPELLSP{} frame={} ticks={}\n", (visualResource), frame, gameTicks());
                    }
                    previous = frame;
                    smokeDriver.state.skillSequence << presented << ',' << gameTicks() << ',' << smokeDriver.state.skillInputPhase << ',' << prologue->battle().animating_movement() << ',' << frame << ',' << smokeDriver.state.skillInputImageHash << '\n';
                }
                prologue->frame_presented();
            }
            ++presented;
            if (stdioPending && gameTicks() >= stdioReadyAt)
            {
                (*stdioPending)["state"] = stdioObserve();
                if (stdioRemainingFrames)
                {
                    --stdioRemainingFrames;
                    const auto& observed = (*stdioPending)["state"];
                    if (observed.value("response_required", false) && (observed.contains("dialogue") || observed.value("choosing", false) || observed.value("modal", std::string{}) == "experience" || observed.value("modal", std::string{}) == "growth" || observed.value("modal", std::string{}) == "learning")) { stdioRemainingFrames = 0; }
                    if (!s2ne::StdioProtocol::mismatch((*stdioPending)["state"], (*stdioPending)["expect_after"])) { stdioRemainingFrames = 0; }
                }
                if (!stdioRemainingFrames)
                {
                    (*stdioPending)["progress"] = stdioProgress.sample((*stdioPending)["state"], SDL_GetTicks());
                    s2ne::StdioProtocol::verify_response(*stdioPending);
                    stdioRespond(*stdioPending);
                    stdioPending.reset();
                }
            }
            if (auto delay = options.headlessSpeed ? 0u : s2ne::original_ui_frame_delay(nowUi, gameTicks()))
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
        std::print("Presented {} frames\n", presented);
        audio.stop();

        return 0;
    }
}

#include "terminal_output.hpp"