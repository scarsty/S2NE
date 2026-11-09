#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "opening_audio.hpp"
#include "flic.hpp"
#include "title_image.hpp"
#include "original_menu.hpp"
#include "new_game.hpp"
#include "prologue.hpp"
#include "original_battle_menu.hpp"
#include "original_battle_reports.hpp"
#include "original_save_menu.hpp"
#include "original_save_bridge.hpp"
#include "original_promotion_transaction.hpp"
#include "original_save_store.hpp"
#include "original_initialization.hpp"
#include "original_campaign_persistence.hpp"
#include "original_confirmation.hpp"
#include "original_settings.hpp"
#include <memory>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    try {
        bool verify = argc > 1 && std::string(argv[1]) == "--verify";
        bool smoke = argc > 1 && std::string(argv[1]) == "--smoke";
        bool newGameSmoke = argc > 1 && std::string(argv[1]) == "--smoke-new-game";
        bool battleSmoke = argc > 1 && (std::string(argv[1]) == "--smoke-battle" || std::string(argv[1]) == "--smoke-battle-alternate");
        bool alternateSmoke = battleSmoke && std::string(argv[1]) == "--smoke-battle-alternate";
        bool autoSmoke=argc>1&&(std::string(argv[1])=="--smoke-auto"||std::string(argv[1])=="--smoke-auto-alternate");
        if(autoSmoke&&std::string(argv[1])=="--smoke-auto-alternate")alternateSmoke=true;
        bool detailSmoke=argc>1&&std::string(argv[1])=="--smoke-detail";
        bool feedbackSmoke=argc>1&&std::string(argv[1])=="--smoke-feedback";
        bool feedbackFlash=false,feedbackNumbers=false,feedbackHp=false,feedbackReaction=false;
        uint64_t checkedReactionDraws=0;
        unsigned feedbackRounds=0;
        bool campaignContinue=argc>1&&std::string(argv[1])=="--smoke-campaign-continue";
        if(campaignContinue)std::cout.setf(std::ios::unitbuf);
        unsigned campaignCheckpoints=0,lastCampaignStage=0,observedCampaignStage=0;
        Uint64 campaignStageStarted=0;
        unsigned exitMoveTurn=0;
        bool campaignSmoke=campaignContinue||(argc>1&&(std::string(argv[1])=="--smoke-campaign"||std::string(argv[1])=="--smoke-campaign-alternate"));
        if(campaignSmoke&&std::string(argv[1])=="--smoke-campaign-alternate")alternateSmoke=true;
        bool clearSmoke=campaignSmoke||(argc>1&&std::string(argv[1])=="--smoke-clear");
        bool clearVerified=false;
        unsigned diagnosticTurn=0;
        autoSmoke=autoSmoke||feedbackSmoke;
        battleSmoke=battleSmoke||autoSmoke||detailSmoke||clearSmoke;
        bool loadSmoke=argc>1&&std::string(argv[1])=="--smoke-load";
        bool promotionSmoke=argc>1&&std::string(argv[1])=="--smoke-promotion";
        unsigned promotionSmokePhase=0,promotionSmokeKind=0,promotionSmokeTarget=0;
        bool promotionVerified=false;
        bool option = verify || smoke || newGameSmoke || battleSmoke || loadSmoke || promotionSmoke;
        std::optional<std::filesystem::path> isolatedSaveRoot;
        struct SaveTestCleanup{
            const std::optional<std::filesystem::path>& root;
            ~SaveTestCleanup(){if(root){std::error_code ignored;std::filesystem::remove_all(*root,ignored);}}
        } saveTestCleanup{isolatedSaveRoot};
        if(option){
            wchar_t directory[MAX_PATH]{};
            if(!GetTempPathW(MAX_PATH,directory))throw std::runtime_error("Cannot isolate smoke-test saves");
            auto candidate=std::filesystem::path(directory)/(L"s2ne-smoke-save-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
            if(!std::filesystem::create_directory(candidate))throw std::runtime_error("Smoke save directory already exists; refusing to reuse it");
            isolatedSaveRoot=std::move(candidate);
        }
        auto root = argc > (option ? 2 : 1) ? std::filesystem::u8path(argv[option ? 2 : 1]) : std::filesystem::current_path();
        if (argc <= (option ? 2 : 1) && !std::filesystem::exists(root / "SUPERSP.PAL")) {
            auto candidate = std::filesystem::u8path(SDL_GetBasePath());
            while (!candidate.empty()) {
                if (std::filesystem::exists(candidate / "SUPERSP.PAL")) { root = candidate; break; }
                auto parent = candidate.parent_path();
                if (parent == candidate) break;
                candidate = parent;
            }
        }
        std::vector<s2ne::Flic> clips;
        clips.push_back(s2ne::Flic::from_disc(root, 21));
        clips.push_back(s2ne::Flic::from_disc(root, 22));
        auto title = s2ne::load_title(root);
        s2ne::OriginalMenu menu(root);
        s2ne::NewGameSetup setup(root);
        auto setupCheck = setup.draw(title, menu);
        auto menuCheck = title; menu.draw(menuCheck);
        if (verify) {
            std::cout << "Decoded original title PCX: 640x480\n";
            for (auto& clip : clips) {
                std::vector<uint8_t> output;
                unsigned frames = 0;
                while (clip.next(output)) ++frames;
                if (frames != clip.frames) throw std::runtime_error("Incomplete intro decoding");
                std::cout << "Decoded original intro: " << clip.width << 'x' << clip.height << ", " << frames << " frames, delay " << clip.delay << " ms\n";
            }
            return 0;
        }
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) throw std::runtime_error(SDL_GetError());
        s2ne::OpeningAudio audio;
        bool audioAvailable = false;
        try { audio.start(root / "MP3" / "SUPERSP_A02.mp3"); audioAvailable = true; }
        catch (const std::exception& ex) { audio.stop(); std::cerr << "Audio unavailable; continuing without sound: " << ex.what() << '\n'; }
        if (!SDL_CreateWindowAndRenderer("Northern Envoy - original intro and menu", 960, 720,
                                        SDL_WINDOW_RESIZABLE, &window, &renderer)) throw std::runtime_error(SDL_GetError());
        SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480);
        if (!texture) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        std::vector<uint8_t> rgba(640 * 480 * 4);
        size_t clip = 0;
        bool running = true, paused = false, inMenu = false, inSetup = false;
        auto showMenu = [&] { SDL_ShowCursor(); inMenu = true; paused = true; auto image = title; menu.draw(image); if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4)) throw std::runtime_error(SDL_GetError()); };
        auto showSetup = [&] { inSetup = true; auto image=setup.draw(title,menu); if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError()); };
        std::unique_ptr<s2ne::Prologue> prologue;
        std::optional<s2ne::OriginalSettings> settings;
        std::optional<s2ne::OriginalBattleMenu> battleMenu;
        std::optional<s2ne::OriginalSaveMenu> saveMenu;
        std::optional<s2ne::OriginalBattleReports> battleReports;
        std::optional<s2ne::OriginalSave::State> rawSave;
        std::optional<s2ne::OriginalPromotion> promotionMenu;
        bool autosaveBlocked=false;
        auto discForSave=[](const s2ne::OriginalSave::State& state){
            auto value=[](const auto& bytes,size_t at){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bytes.at(at+i))<<(8*i);return int32_t(v);};
            auto selected=value(state.stage_4fb478,16);
            if(selected<0||selected>1)throw std::runtime_error("Invalid saved original disc selector");
            return static_cast<s2ne::OriginalDisc>(selected);
        };
        s2ne::Prologue* connectedCampaign=nullptr;
        s2ne::OriginalInitialization initialization(root);
        s2ne::OriginalCampaignPersistence campaignPersistence(root);
        s2ne::OriginalPromotionTransaction promotionTransaction(root);
        s2ne::OriginalPromotion promotionRules(root);
        s2ne::OriginalMap promotionMap(root);
        std::optional<s2ne::OriginalConfirmation> overwriteConfirmation;
        std::ifstream itemFile(root/"ITEMSP.BIN",std::ios::binary);
        std::vector<uint8_t> itemBytes{std::istreambuf_iterator<char>(itemFile),{}};
        const size_t saveItems=s2ne::OriginalSave::item_count(itemBytes);
        if(promotionSmoke){
            if(argc<4)throw std::runtime_error("Promotion smoke requires resource root and earned original save path");
            auto state=s2ne::OriginalSave::load(std::filesystem::u8path(argv[3]),saveItems);
            auto restored=s2ne::OriginalSaveBridge::restore(root,state);
            prologue=std::make_unique<s2ne::Prologue>(root,restored.events.difficulty);
            prologue->set_disc(discForSave(restored.raw));prologue->restore_battle(std::move(restored.events),restored.turn);rawSave=std::move(restored.raw);paused=true;
        }
        auto settingsState=s2ne::OriginalSettings::load_state(root);
        menu.small_font=settingsState.small_font!=0;
        auto showSettings=[&]{auto image=settings->draw(s2ne::load_title(root,20,false));if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());};
        auto beginSettings=[&]{settings.emplace(root,0,settingsState);showSettings();};
        std::optional<s2ne::OriginalConfirmation> exitConfirmation;
        auto showExitConfirmation=[&]{auto image=s2ne::load_title(root,20,false);exitConfirmation->draw(image,menu);if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());};
        auto beginExitConfirmation=[&]{exitConfirmation.emplace(root,menu,51,200,403,true);showExitConfirmation();};
        bool prologueRedraw=false;
        auto showPrologue = [&] { prologueRedraw=true; };
        auto renderPrologue = [&] {
            prologueRedraw=false;
            if(prologue->drawing_original_mouse()||prologue->computer_actions())SDL_HideCursor();else SDL_ShowCursor();
            auto image=prologue->draw(menu);
            if(feedbackSmoke&&prologue->reaction_draws()>checkedReactionDraws){
                checkedReactionDraws=prologue->reaction_draws();
                const auto& bitmap=prologue->last_reaction_bitmap();
                if(bitmap.empty())throw std::runtime_error("Production reaction draw has no indexed bitmap");
                if(bitmap!=prologue->expected_reaction_bitmap(prologue->last_reaction_target(),prologue->last_reaction_left()))throw std::runtime_error("SDL reaction bitmap disagrees with indexed compositor");
                feedbackReaction=true;
            }
            if(battleMenu){SDL_ShowCursor();image=battleMenu->draw(image);}
            if(battleReports)image=battleReports->draw();
            if(promotionMenu){SDL_ShowCursor();image=promotionMenu->draw(image);}
            if(saveMenu)saveMenu->draw(image,menu);
            if(overwriteConfirmation)overwriteConfirmation->draw(image,menu);
            if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());
        };
        auto openSelectedDetail=[&]{
            int selected=prologue->battle_selected();
            if(selected<0||size_t(selected)>=prologue->events.units.size())return;
            const auto& unit=prologue->events.units[size_t(selected)];
            std::vector<s2ne::OriginalBattleReports::Entry> entries{{unit,unit.hostile(),unit.npc_class()?1:0,false}};
            battleReports.emplace(root,s2ne::OriginalBattleReports::Page::roster,prologue->events,std::move(entries),prologue->draw(menu),nullptr,settingsState.small_font!=0);
            battleReports->detail();battleReports->close_to_battle();SDL_ShowCursor();showPrologue();
        };
        auto openSelectedPromotion=[&]{
            if(!rawSave||!prologue->in_battle()||prologue->computer_actions()||prologue->battle().combat_feedback_active()||prologue->battle().animating_movement())return;
            int selected=prologue->battle_selected();
            if(selected<0||size_t(selected)>=rawSave->groups[0].units.size()||size_t(selected)>=prologue->events.units.size())return;
            if(rawSave->groups[0].inventory.size()!=saveItems)return;
            const auto& unit=prologue->events.units[size_t(selected)];
            promotionMenu.emplace(root);
            int gameMode=int(rawSave->globals_4f7cd0[0]);
            bool opened=promotionMenu->open(unit,unit.experience_threshold,[&,selected](const auto& request){
                try{
                    if(!rawSave||!prologue->in_battle()||prologue->battle_selected()!=selected||prologue->battle_state()!=s2ne::Battle::State::command||prologue->computer_actions()||prologue->battle().combat_feedback_active()||prologue->battle().animating_movement())return false;
                    auto stagedEvents=prologue->events;auto stagedBattle=prologue->battle();auto stagedSave=*rawSave;
                    bool committed=stagedBattle.mutate_random([&](s2ne::OriginalRandom& random,uint32_t seconds){
                        s2ne::OriginalPromotionTransaction::State before{stagedEvents.units.at(size_t(selected)),stagedSave.groups[0].inventory,random};
                        uint8_t metadata=stagedSave.groups[0].metadata[4];
                        s2ne::OriginalPromotionTransaction::Context context;
                        context.game_mode=int(stagedSave.globals_4f7cd0[0]);
                        context.rebuild=[&](auto& next){stagedEvents.restore_unit_resources(next.unit);
                            for(unsigned direction=0;direction<4;++direction)if(!promotionMap.unit_height(next.unit.appearance,direction,next.unit.animation_action))return false;
                            return true;};
                        context.recalculate_army=[&](auto& next){
                            stagedEvents.units.at(size_t(selected))=next.unit;std::vector<int> roles;
                            for(const auto& record:stagedSave.groups[0].units)roles.push_back(int(int16_t(unsigned(record[8])|(unsigned(record[9])<<8))));
                            auto allies=std::span<const s2ne::OriginalEvents::Unit>(stagedEvents.units).first(roles.size());
                            metadata=promotionTransaction.aggregate_army_metadata(metadata,allies,roles);return true;};
                        auto next=promotionTransaction.prepare(before,request,context,[seconds]{return seconds;});if(!next)return false;
                        stagedSave=s2ne::OriginalSaveBridge::capture_promotion(stagedEvents,prologue->battle_turn(),prologue->camera(),stagedSave,size_t(selected),request.source_kind,next->inventory,metadata);
                        s2ne::OriginalSave::encode(stagedSave,saveItems);
                        stagedBattle.set_hostile_resources(stagedEvents);
                        std::vector<s2ne::Battle::MovementContext> contexts(stagedEvents.units.size());
                        for(size_t i=0;i<contexts.size();++i){const auto& u=stagedEvents.units[i];auto& c=contexts[i];
                            c.status34=u.status34;c.status118=uint8_t(u.abilities>>24);c.status116=uint8_t(u.abilities>>8);
                            c.has_usable_ability=stagedEvents.has_learned_ability(u);c.movement_class=stagedEvents.movement_class(u);
                            c.sprite_height=promotionMap.unit_height(u.appearance,u.direction,u.animation_action);}
                        stagedBattle.set_movement_contexts(std::move(contexts));random=next->random;return true;
                    });
                    if(!committed)return false;
                    static_assert(std::is_nothrow_swappable_v<s2ne::OriginalEvents>&&std::is_nothrow_swappable_v<s2ne::Battle>&&std::is_nothrow_swappable_v<s2ne::OriginalSave::State>);
                    using std::swap;swap(prologue->events,stagedEvents);swap(prologue->battle(),stagedBattle);swap(*rawSave,stagedSave);
                    return true;
                }catch(const std::exception& ex){std::cerr<<"Promotion rejected: "<<ex.what()<<'\n';return false;}
            },true,gameMode);
            if(!opened)promotionMenu.reset();else showPrologue();
        };
        auto startGame = [&] {
            setup.confirm();
            rawSave.reset();autosaveBlocked=false;connectedCampaign=nullptr;
            prologue=std::make_unique<s2ne::Prologue>(root,setup.confirmed);
            prologue->set_small_font(settingsState.small_font!=0);
            prologue->enable_text_animation(unsigned(settingsState.message_delay));
            inMenu=false;inSetup=false;paused=true;
            showPrologue();
            std::cout<<"Original prologue stage="<<prologue->events.stage<<", difficulty="<<setup.confirmed<<'\n';
        };
        uint64_t next = SDL_GetTicks();
        const uint64_t started = next;
        unsigned presented = 0;
        unsigned smokePhase = 0;
        bool battleClickPending=false;
        unsigned battleSmokeStartTurn=0;
        unsigned hostileBannerFrames=0,playerBannerFrames=0,battleSmokeRounds=0;
        s2ne::Battle::Point battleSmokeDestination{};
        size_t battleSmokeUnit=0;
        std::vector<std::pair<int,int>> battleSmokeEnemyOrigins;
        unsigned hostileActionFrames=0;
        unsigned detailSmokePhase=0;
        bool autoMenuOpened=false;
        unsigned autoFriendlyFrames=0,autoEnemyFrames=0;
        uint64_t lastUiTick=SDL_GetTicks();
        while (running) {
            uint64_t nowUi=SDL_GetTicks();
            if(prologue){
                if(campaignSmoke&&alternateSmoke&&prologue->battle_turn()!=diagnosticTurn){
                    diagnosticTurn=prologue->battle_turn();
                    std::cout<<"Natalia easy auto diagnostic turn="<<diagnosticTurn<<" difficulty="<<prologue->events.difficulty<<'\n';
                    for(const auto& u:prologue->events.units)if(!u.hostile())std::cout<<" ally="<<u.id<<" behavior="<<u.behavior<<" hp="<<u.hp<<" ap="<<u.action_points<<" action="<<unsigned(u.action_state)<<" x="<<u.x<<" y="<<u.y<<'\n';
                }
                if(prologue->computer_actions()&&battleSmoke)++hostileActionFrames;
                if(autoSmoke&&prologue->automatic_actions())++autoFriendlyFrames;
                if(autoSmoke&&prologue->hostile_actions())++autoEnemyFrames;
                if(prologue->battle_intro()&&battleSmoke&&battleSmokeStartTurn){if(prologue->hostile_banner())++hostileBannerFrames;else ++playerBannerFrames;}
                prologue->set_mask_surfaces(settingsState.status_window==1);
                prologue->set_checker_mask(settingsState.flag_4f7d0a==1);
                if(!battleMenu&&!battleReports&&!promotionMenu)prologue->tick(clearSmoke?120u:feedbackSmoke?std::max(30u,unsigned(nowUi-lastUiTick)):unsigned(nowUi-lastUiTick));
                if(!battleMenu&&!battleReports&&!promotionMenu&&!battleSmoke&&SDL_GetMouseFocus()==window){
                    float x{},y{},rx{},ry{};SDL_GetMouseState(&x,&y);
                    if(SDL_RenderCoordinatesFromWindow(renderer,x,y,&rx,&ry))prologue->edge_scroll(rx,ry);
                }
                showPrologue();
                if(feedbackSmoke)if(auto feedback=prologue->combat_feedback();feedback&&feedback->active()){
                    feedbackFlash|=feedback->silhouette();feedbackNumbers|=feedback->text_visible()&&!feedback->miss;
                    feedbackHp|=feedback->hp_after<feedback->hp_before;
                }
            }
            lastUiTick=nowUi;
            if (smoke && smokePhase == 0 && SDL_GetTicks() - started > 300) {
                SDL_Event e{}; e.type = SDL_EVENT_KEY_DOWN; e.key.key = SDLK_ESCAPE; SDL_PushEvent(&e); ++smokePhase;
            }
            if (smoke && smokePhase == 1 && SDL_GetTicks() - started > 600) {
                SDL_Event e{}; e.type = SDL_EVENT_KEY_DOWN; e.key.key = SDLK_DOWN; SDL_PushEvent(&e);
                e.key.key = SDLK_RETURN; SDL_PushEvent(&e); ++smokePhase;
            }
            if (smoke && SDL_GetTicks() - started > 1200) {
                if ((audioAvailable && !audio.progressing()) || !inMenu || menu.selected != 1 || menu.activated != 1) throw std::runtime_error("Audio/menu smoke test failed");
                std::cout << (audioAvailable ? "Audio playback position advanced\n" : "Silent mode verified\n");
                running = false;
            }
            if(battleSmoke && SDL_GetTicks()-started > 100+smokePhase*100){
                SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=smokePhase==0?SDLK_ESCAPE:SDLK_RETURN;
                if(inSetup&&smokePhase==2){e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.windowID=SDL_GetWindowID(window);e.button.button=SDL_BUTTON_LEFT;SDL_RenderCoordinatesToWindow(renderer,300,280,&e.button.x,&e.button.y);}
                if(inSetup&&smokePhase==3){
                    if(setup.difficulty!=0||!inSetup)throw std::runtime_error("Difficulty page did not retain easiest mouse selection");
                    e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.windowID=SDL_GetWindowID(window);e.button.button=SDL_BUTTON_LEFT;SDL_RenderCoordinatesToWindow(renderer,300,425,&e.button.x,&e.button.y);
                }
                if(prologue&&prologue->choosing()&&((alternateSmoke&&prologue->events.stage==140)||(campaignContinue&&prologue->events.stage==10))&&prologue->selection()==0)e.key.key=SDLK_DOWN;
                if(campaignContinue&&prologue&&prologue->events.stage!=observedCampaignStage){
                    observedCampaignStage=prologue->events.stage;campaignStageStarted=SDL_GetTicks();
                    std::cout<<"Continuous campaign pending stage="<<prologue->events.stage<<" battle="<<prologue->in_battle()<<" dialogue="<<prologue->campaign_dialogue()<<'\n';
                }
                if(campaignContinue&&prologue&&!prologue->campaign_blocker().empty())throw std::runtime_error("Continuous campaign blocked at stage="+std::to_string(prologue->events.stage)+": "+prologue->campaign_blocker());
                if(campaignContinue&&prologue&&prologue->in_battle()&&prologue->events.stage!=lastCampaignStage){
                    if(!rawSave||!prologue->autosave_requests().empty())throw std::runtime_error("Continuous battle entered before autosave committed");
                    auto saved=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot,52),saveItems);
                    auto restored=s2ne::OriginalSaveBridge::restore(root,saved);
                    if(restored.events.stage!=prologue->events.stage)throw std::runtime_error("Continuous autosave stage mismatch");
                    lastCampaignStage=prologue->events.stage;++campaignCheckpoints;
                    std::cout<<"Continuous campaign checkpoint="<<campaignCheckpoints<<" stage="<<lastCampaignStage<<" original autosave reload verified\n";
                }
                if(campaignSmoke&&!campaignContinue&&prologue&&prologue->events.stage==(alternateSmoke?72u:2u)&&prologue->in_battle()){
                    if(!rawSave||!prologue->autosave_requests().empty())throw std::runtime_error("Stage2 entered before original autosaves committed");
                    auto saved=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot,52),saveItems);
                    auto restored=s2ne::OriginalSaveBridge::restore(root,saved);
                    if(restored.events.stage!=(alternateSmoke?72u:2u))throw std::runtime_error("Next-battle autosave restored the wrong stage");
                    std::cout<<"SDL campaign victory -> rewards -> hub -> next battle -> original autosave reload verified\n";
                    clearVerified=true;running=false;
                }else if(campaignSmoke&&prologue&&prologue->interbattle_active()){
                    e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;
                    SDL_RenderCoordinatesToWindow(renderer,100,450,&e.button.x,&e.button.y);SDL_PushEvent(&e);
                }else if(clearSmoke&&prologue&&prologue->campaign_reward_report()){ 
                    std::cout<<"SDL original first-battle scripts and victory report verified at turn="<<prologue->battle_turn()<<"\n";
                    if(prologue->battle_turn()>99)throw std::runtime_error("Victory report exceeded original turn deadline");
                    if(campaignSmoke){e.key.key=SDLK_RETURN;SDL_PushEvent(&e);}
                    else {clearVerified=true;running=false;}
                }else if(clearSmoke&&prologue&&prologue->campaign_defeated()){ 
                    std::cerr<<"Campaign defeat stage="<<prologue->events.stage<<" turn="<<prologue->battle_turn()<<" difficulty="<<prologue->events.difficulty<<'\n';
                    for(const auto& u:prologue->events.units)std::cerr<<" unit="<<u.id<<" behavior="<<u.behavior<<" hp="<<u.hp<<" status="<<unsigned(u.status_flags)<<" x="<<u.x<<" y="<<u.y<<'\n';
                    throw std::runtime_error("Production first-battle smoke reached original defeat");
                }else if(clearSmoke&&prologue&&prologue->in_battle()){ 
                    unsigned friendly=0,hostile=0;
                    for(const auto& unit:prologue->events.units)if(unit.present(prologue->battle_turn())){if(unit.hostile())++hostile;else if(unit.controllable())++friendly;}
                    if((!hostile&&!campaignContinue)||!friendly){
                        std::cout<<"SDL first battle outcome: turn="<<prologue->battle_turn()<<" friendly="<<friendly<<" hostile="<<hostile<<"\n";
                        for(const auto& unit:prologue->events.units)if(unit.controllable())std::cout<<" actor="<<unit.id<<" hp="<<unit.hp<<" level="<<unit.level<<" xp="<<unit.experience<<"\n";
                        if(!friendly)throw std::runtime_error("First battle production automatic actions lost; not a verified victory");
                        throw std::runtime_error("Enemy elimination reached; original victory settlement must also complete");
                    }
                    if(prologue->battle_turn()>100)throw std::runtime_error("First battle production automatic actions failed to clear within100 turns");
                    if(!battleSmokeStartTurn)battleSmokeStartTurn=prologue->battle_turn();
                    if(campaignContinue&&prologue->events.stage==10&&!hostile){
                        auto& battle=prologue->battle();
                        if(!battle.animating_movement()&&!battle.combat_feedback_active()){
                            auto actor=std::find_if(prologue->events.units.begin(),prologue->events.units.end(),[](const auto& u){return u.id==11&&!u.hostile()&&u.present();});
                            if(actor==prologue->events.units.end())throw std::runtime_error("Exit protagonist unavailable");
                            using Point=s2ne::Battle::Point;
                            Point origin{actor->x,actor->y};
                            std::vector<Point> queue{origin};std::map<std::pair<int,int>,Point> previous;previous[{origin.x,origin.y}]=origin;
                            std::optional<Point> destination;
                            for(size_t i=0;i<queue.size()&&!destination;++i){auto p=queue[i];
                                if((p.x==21&&(p.y==4||p.y==6))||(p.x==20&&p.y==5)){destination=p;break;}
                                for(int dy:{-1,1})for(int dx:{0,(p.y&1)?1:-1}){Point q{p.x+dx,p.y+dy};
                                    if(previous.contains({q.x,q.y})||battle.step_cost(*actor,p,q)<0||battle.occupant(prologue->events.units,q)>=0)continue;
                                    previous[{q.x,q.y}]=p;queue.push_back(q);
                                }
                            }
                            if(!destination)throw std::runtime_error("No legal route to original stage10 exit");
                            if(*destination!=origin){
                                if(exitMoveTurn==prologue->battle_turn()){prologue->end_battle_turn();}
                                else if(prologue->battle_state()==s2ne::Battle::State::idle){auto p=prologue->screen_cell(origin);prologue->pan(p.x-320,p.y-240);p=prologue->screen_cell(origin);prologue->mouse(float(p.x),float(p.y),true);}
                                else if(prologue->battle_state()==s2ne::Battle::State::command){
                                    for(unsigned i=0;i<6&&prologue->battle_command_index()!=1;++i)prologue->battle_cycle(1);
                                    prologue->battle_confirm();
                                }else if(prologue->battle_state()==s2ne::Battle::State::movement){
                                    Point next=*destination;while(previous.at({next.x,next.y})!=origin)next=previous.at({next.x,next.y});
                                    if(!battle.can_step(prologue->events.units,size_t(actor-prologue->events.units.begin()),next))prologue->end_battle_turn();
                                    else{auto p=prologue->screen_cell(next);prologue->pan(p.x-320,p.y-240);p=prologue->screen_cell(next);prologue->mouse(float(p.x),float(p.y),true);exitMoveTurn=prologue->battle_turn();}
                                }
                            }
                        }
                    }else prologue->start_auto_battle();
                }else if(detailSmoke&&prologue&&prologue->in_battle()){
                    if(detailSmokePhase==0){
                        auto unit=std::find_if(prologue->events.units.begin(),prologue->events.units.end(),[](const auto& candidate){return candidate.controllable()&&candidate.present();});
                        if(unit==prologue->events.units.end())throw std::runtime_error("Missing detail smoke unit");
                        auto point=prologue->screen_cell({unit->x,unit->y});prologue->pan(point.x-320,point.y-240);point=prologue->screen_cell({unit->x,unit->y});
                        prologue->mouse(float(point.x),float(point.y),true);detailSmokePhase=1;
                    }else if(detailSmokePhase==1){
                        if(prologue->battle_state()!=s2ne::Battle::State::command)throw std::runtime_error("Detail smoke selection failed");
                        for(unsigned i=0;i<6&&prologue->battle_command_index()!=3;++i)prologue->battle_cycle(1);
                        SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_RETURN;SDL_PushEvent(&key);detailSmokePhase=2;
                    }else if(detailSmokePhase==2){
                        if(!battleReports||battleReports->page()!=s2ne::OriginalBattleReports::Page::detail)throw std::runtime_error("Detail command did not open original report");
                        SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_ESCAPE;SDL_PushEvent(&key);detailSmokePhase=3;
                    }else{
                        if(battleReports||prologue->battle_state()!=s2ne::Battle::State::command)throw std::runtime_error("Detail report did not return to selected command");
                        std::cout<<"SDL selected command3 -> original detail modal -> selected command return verified\n";running=false;
                    }
                }else if(autoSmoke&&prologue&&prologue->in_battle()){
                    if(!battleSmokeStartTurn){
                        battleSmokeStartTurn=prologue->battle_turn();
                        auto pushRight=[&]{SDL_Event click{};click.type=SDL_EVENT_MOUSE_BUTTON_DOWN;click.button.button=SDL_BUTTON_RIGHT;SDL_RenderCoordinatesToWindow(renderer,320,240,&click.button.x,&click.button.y);SDL_PushEvent(&click);};
                        pushRight();autoMenuOpened=true;
                    }else if(battleMenu){
                        SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_DOWN;
                        if(battleMenu->page()==s2ne::OriginalBattleMenu::Page::root&&battleMenu->selected()<3)SDL_PushEvent(&key);else{key.key.key=SDLK_RETURN;SDL_PushEvent(&key);}
                    }else if(prologue->battle_turn()>battleSmokeStartTurn){
                        if(!autoMenuOpened||!autoFriendlyFrames||!autoEnemyFrames)throw std::runtime_error("Automatic menu completed without both friendly and hostile action frames");
                        if(feedbackSmoke&&(!feedbackFlash||!feedbackNumbers||!feedbackHp||!feedbackReaction)){ 
                            if(++feedbackRounds>=15)throw std::runtime_error("No complete rendered flash/number/HP feedback within 15 original turns");
                            battleSmokeStartTurn=0;
                        }else{
                            std::cout<<"SDL battlefield automatic combat menu -> friendly actions -> hostile actions -> player control verified\n";
                            if(feedbackSmoke)std::cout<<"Rendered original indexed reaction, hit silhouette, rising damage numbers and stepped HP reduction verified\n";
                            running=false;
                        }
                    }
                }else if(prologue&&prologue->in_battle()){
                    if(prologue->events.stage!=(alternateSmoke?71u:1u))throw std::runtime_error("Wrong campaign battle stage");
                    auto pushClick=[&](s2ne::Battle::Point point){SDL_Event click{};click.type=SDL_EVENT_MOUSE_BUTTON_DOWN;click.button.windowID=SDL_GetWindowID(window);click.button.button=SDL_BUTTON_LEFT;SDL_RenderCoordinatesToWindow(renderer,float(point.x),float(point.y),&click.button.x,&click.button.y);SDL_PushEvent(&click);};
                    if(battleClickPending){
                        if(prologue->battle_state()==s2ne::Battle::State::command && !battleSmokeStartTurn){
                            auto origin=prologue->battle_command_origin();pushClick({origin.x+48,origin.y+16});
                            battleSmokeStartTurn=prologue->battle_turn();
                        }else if(prologue->battle_state()==s2ne::Battle::State::movement){
                            pushClick(prologue->screen_cell(battleSmokeDestination));
                        }else if(prologue->battle_state()==s2ne::Battle::State::command){
                            const auto& unit=prologue->events.units.at(battleSmokeUnit);
                            if(unit.x!=battleSmokeDestination.x||unit.y!=battleSmokeDestination.y)throw std::runtime_error("SDL routed movement failed");
                            SDL_Event finish{};finish.type=SDL_EVENT_KEY_DOWN;finish.key.key=SDLK_F10;SDL_PushEvent(&finish);
                        }else if(prologue->battle_state()==s2ne::Battle::State::idle){
                            if(prologue->battle_turn()!=battleSmokeStartTurn+1||hostileBannerFrames<8||playerBannerFrames<8)throw std::runtime_error("SDL side banners missing or side finish failed");
                            if(++battleSmokeRounds==2){
                                bool enemy_moved=false;
                                for(size_t i=0;i<prologue->events.units.size();++i){const auto& unit=prologue->events.units[i];if(unit.hostile()&&std::pair{unit.x,unit.y}!=battleSmokeEnemyOrigins.at(i))enemy_moved=true;}
                                if(!enemy_moved||hostileActionFrames<2)throw std::runtime_error("Enemy turn did not visibly execute movement");
                                std::cout<<"SDL campaign battle "<<prologue->events.stage<<": player movement, visible enemy movement and two complete side-banner cycles verified ("<<hostileActionFrames<<" hostile frames)\n";running=false;
                            }else{
                                battleSmokeStartTurn=prologue->battle_turn();hostileBannerFrames=playerBannerFrames=0;
                                SDL_Event finish{};finish.type=SDL_EVENT_KEY_DOWN;finish.key.key=SDLK_F10;SDL_PushEvent(&finish);
                            }
                        }else throw std::runtime_error("Unexpected SDL battle state");
                    }else{
                        battleSmokeEnemyOrigins.clear();
                        for(const auto& unit:prologue->events.units)battleSmokeEnemyOrigins.emplace_back(unit.x,unit.y);
                        s2ne::OriginalBattleMenu originalMenu(root,{},settingsState);
                        originalMenu.activate();
                        if(originalMenu.request().action!=s2ne::OriginalBattleMenu::Action::conditions)throw std::runtime_error("Battlefield conditions menu request missing");
                        originalMenu.complete_operation();originalMenu.move(1);originalMenu.activate();
                        if(originalMenu.page()!=s2ne::OriginalBattleMenu::Page::overview)throw std::runtime_error("Battlefield overview submenu missing");
                        originalMenu.cancel();originalMenu.cancel();
                        if(!originalMenu.return_code()||*originalMenu.return_code()!=0)throw std::runtime_error("Battlefield menu return path failed");
                        prologue->pan(320,160);
                        auto before=prologue->screen_cell({0,0});
                        prologue->edge_scroll(0,0);prologue->edge_scroll(0,0);
                        auto after=prologue->screen_cell({0,0});
                        if(after.x-before.x!=64||after.y-before.y!=64)throw std::runtime_error("Stationary edge cursor failed to keep scrolling");
                        prologue->edge_scroll(639,479);prologue->edge_scroll(639,479);
                        auto restored=prologue->screen_cell({0,0});
                        if(restored.x!=before.x||restored.y!=before.y)throw std::runtime_error("Reverse edge cursor failed to restore camera");
                        auto terrain=s2ne::Battle::load(root,prologue->events.map_id);bool found=false;
                        for(size_t i=0;i<prologue->events.units.size()&&!found;++i){const auto& u=prologue->events.units[i];
                            for(int dy:{-1,1})for(int dx:{0,(u.y&1)?1:-1}){s2ne::Battle::Point to{u.x+dx,u.y+dy};
                                auto fromScreen=prologue->screen_cell({u.x,u.y}),toScreen=prologue->screen_cell(to);
                                if(found||!terrain.can_step(prologue->events.units,i,to)||fromScreen.x<0||fromScreen.x>=640||fromScreen.y<0||fromScreen.y>=480||toScreen.x<0||toScreen.x>=640||toScreen.y<0||toScreen.y>=480)continue;
                                battleSmokeUnit=i;battleSmokeDestination=to;pushClick(fromScreen);found=battleClickPending=true;
                            }
                        }
                        if(!found){
                            for(size_t i=0;i<prologue->events.units.size()&&!found;++i){const auto& u=prologue->events.units[i];
                                for(int dy:{-1,1})for(int dx:{0,(u.y&1)?1:-1}){s2ne::Battle::Point to{u.x+dx,u.y+dy};if(found||!terrain.can_step(prologue->events.units,i,to))continue;
                                    auto position=prologue->screen_cell({u.x,u.y});prologue->pan(position.x-320,position.y-240);battleSmokeUnit=i;battleSmokeDestination=to;pushClick(prologue->screen_cell({u.x,u.y}));found=battleClickPending=true;
                                }
                            }
                            if(!found)throw std::runtime_error("SDL battle smoke has no movable unit");
                        }
                    }
                }else if(!prologue||(!prologue->computer_actions()&&!prologue->battle_intro()))SDL_PushEvent(&e);
                if(!prologue||!prologue->computer_actions()){
                    if(++smokePhase>1000&&!clearSmoke)throw std::runtime_error("SDL battle entry timed out");
                }
                if(SDL_GetTicks()-(campaignContinue&&campaignStageStarted?campaignStageStarted:started)>(clearSmoke?600000u:120000u))throw std::runtime_error("SDL battle action cycle timed out: stage="+std::to_string(prologue?prologue->events.stage:0)+" blocker="+(prologue?prologue->campaign_blocker():"")+" dialogue="+std::to_string(prologue&&prologue->campaign_dialogue())+" choosing="+std::to_string(prologue&&prologue->choosing())+" storyRoute="+std::to_string(prologue?prologue->pending_story_route():0)+" turn="+std::to_string(prologue?prologue->battle_turn():0)+" start="+std::to_string(battleSmokeStartTurn)+" feedbackRounds="+std::to_string(feedbackRounds)+" flash="+std::to_string(feedbackFlash)+" numbers="+std::to_string(feedbackNumbers)+" hp="+std::to_string(feedbackHp));
            }
            if(newGameSmoke && SDL_GetTicks()-started > 100+smokePhase*150 && (!prologue||prologue->line_index()==0)){
                SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;
                e.key.key=prologue||inSetup?SDLK_RETURN:inMenu?(menu.selected?SDLK_UP:SDLK_RETURN):SDLK_ESCAPE;
                SDL_PushEvent(&e);++smokePhase;
            }
            if(newGameSmoke && prologue && prologue->line_index()==1){
                if(!prologue||prologue->line_index()!=1||inSetup||inMenu)throw std::runtime_error("New-game dialogue smoke test failed");
                std::cout<<"SDL new game -> original stage 140 -> second dialogue line verified\n";
                running=false;
            }
            if(loadSmoke&&SDL_GetTicks()-started>100+smokePhase*150&&!prologue){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=saveMenu?SDLK_RETURN:inMenu?(menu.selected==1?SDLK_RETURN:SDLK_DOWN):SDLK_ESCAPE;SDL_PushEvent(&e);++smokePhase;}
            if(loadSmoke&&prologue&&presented>10){
                if(!prologue->in_battle()||!rawSave||!isolatedSaveRoot)throw std::runtime_error("Title save load did not restore battle");
                auto captured=s2ne::OriginalSaveBridge::capture(prologue->events,prologue->battle_turn(),prologue->camera(),&*rawSave);
                s2ne::OriginalSaveStore::write(*isolatedSaveRoot,52,captured,saveItems);
                auto reloaded=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot,52),saveItems);
                if(s2ne::OriginalSave::encode(captured,saveItems)!=s2ne::OriginalSave::encode(reloaded,saveItems))throw std::runtime_error("Isolated original save changed on reload");
                std::cout<<"Original save restored through title menu, stage "<<prologue->events.stage<<", turn "<<prologue->battle_turn()<<"; isolated slot52 write/reload verified\n";
                running=false;
            }
            if(loadSmoke&&SDL_GetTicks()-started>15000)throw std::runtime_error("Original save smoke timed out: phase="+std::to_string(smokePhase)+" menu="+std::to_string(inMenu)+" row="+std::to_string(menu.selected)+" save="+std::to_string(bool(saveMenu)));
            if(newGameSmoke && SDL_GetTicks()-started>15000)throw std::runtime_error("New-game dialogue smoke timed out");
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) running = false;
                if(prologue&&prologue->campaign_modal()){
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                        unsigned scan=0;
                        switch(event.key.key){case SDLK_UP:scan=0x48;break;case SDLK_DOWN:scan=0x50;break;case SDLK_LEFT:scan=0x4B;break;case SDLK_RIGHT:scan=0x4D;break;case SDLK_TAB:scan=0x0F;break;case SDLK_ESCAPE:scan=1;break;case SDLK_RETURN:scan=0x1C;break;case SDLK_SPACE:scan=0x39;break;}
                        if(prologue->interbattle_active())prologue->campaign_scan(scan);
                        else if(scan==0x1C||scan==0x39)prologue->advance();
                        else if(event.key.key==SDLK_LALT||event.key.key==SDLK_RALT)prologue->reveal_text();
                    }
                    else if(prologue->interbattle_active()&&(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN)){
                        float x=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.x:event.button.x;
                        float y=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.y:event.button.y;
                        float rx{},ry{};
                        if(SDL_RenderCoordinatesFromWindow(renderer,x,y,&rx,&ry))prologue->mouse(int(rx),int(ry),event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT);
                    }
                    else if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT&&!prologue->interbattle_active())prologue->advance();
                    showPrologue();continue;
                }
                if(settings){
                    s2ne::OriginalSettings::Event input;
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){input.type=1;input.ascii=event.key.key<128?unsigned(event.key.key):0;
                        if(event.key.key==SDLK_ESCAPE)input.scan=1;else if(event.key.key==SDLK_O)input.scan=0x18;else if(event.key.key==SDLK_C)input.scan=0x2e;
                        if(event.key.mod&SDL_KMOD_LCTRL)input.modifiers|=0x20;if(event.key.mod&SDL_KMOD_RCTRL)input.modifiers|=0x40;
                    }else if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_ConvertEventToRenderCoordinates(renderer,&event);input.type=0x20;input.x=int(event.button.x);input.y=int(event.button.y);input.buttons=event.button.button==SDL_BUTTON_LEFT?1:event.button.button==SDL_BUTTON_RIGHT?2:0;}
                    auto result=settings->event(input);
                    if(result.closed){settingsState=settings->state();menu.small_font=settingsState.small_font!=0;if(result.save_config){std::map<std::string,std::string> replacements;std::istringstream updated(settings->config_text());std::string line;while(std::getline(updated,line)){auto eq=line.find('=');if(eq!=std::string::npos&&line.substr(0,eq)!="Version")replacements[line.substr(0,eq)]=line;}
                        std::ifstream existing(root/"SUPERSP.CFG",std::ios::binary);std::string merged;while(std::getline(existing,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto key=line.substr(0,line.find('='));auto replacement=replacements.find(key);if(replacement!=replacements.end()){line=replacement->second;replacements.erase(replacement);}merged+=line+"\r\n";}existing.close();for(const auto& [key,value]:replacements)merged+=value+"\r\n";
                        std::ofstream config(root/"SUPERSP.CFG",std::ios::binary);if(!config)throw std::runtime_error("Cannot save original settings");config<<merged;if(!config)throw std::runtime_error("Original settings write failed");}settings.reset();showMenu();}
                    else showSettings();
                    continue;
                }
                if(exitConfirmation){
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                        if(event.key.key==SDLK_ESCAPE)exitConfirmation->cancel();
                        else if(event.key.key==SDLK_LEFT||event.key.key==SDLK_RIGHT||event.key.key==SDLK_TAB)exitConfirmation->toggle();
                        else if(event.key.key==SDLK_RETURN||event.key.key==SDLK_SPACE)exitConfirmation->confirm();
                    }
                    if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
                        SDL_ConvertEventToRenderCoordinates(renderer,&event);
                        bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;
                        if(down&&event.button.button==SDL_BUTTON_RIGHT)exitConfirmation->cancel();
                        else exitConfirmation->mouse(down?event.button.x:event.motion.x,down?event.button.y:event.motion.y,down&&event.button.button==SDL_BUTTON_LEFT);
                    }
                    if(auto result=exitConfirmation->result();result){if(*result)running=false;exitConfirmation.reset();showMenu();}
                    else showExitConfirmation();
                    continue;
                }
                if(saveMenu&&!battleMenu){
                    s2ne::OriginalSaveMenu::Event input;
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){input.type=1;switch(event.key.key){case SDLK_ESCAPE:input.scan=1;break;case SDLK_UP:input.scan=0x48;break;case SDLK_DOWN:input.scan=0x50;break;case SDLK_LEFT:input.scan=0x4b;break;case SDLK_RIGHT:input.scan=0x4d;break;case SDLK_RETURN:input.scan=0x1c;break;case SDLK_SPACE:input.scan=0x39;break;}}
                    else if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_ConvertEventToRenderCoordinates(renderer,&event);bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;input.type=down?0x20:8;input.x=int(down?event.button.x:event.motion.x);input.y=int(down?event.button.y:event.motion.y);input.buttons=down?(event.button.button==SDL_BUTTON_LEFT?1:event.button.button==SDL_BUTTON_RIGHT?2:0):0;}
                    if(event.type==SDL_EVENT_KEY_DOWN&&event.key.key==SDLK_RETURN)input.ascii=13;
                    auto result=saveMenu->event(input);
                    if(result.action==s2ne::OriginalSaveMenu::Action::load){try{auto state=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(root,result.slot),saveItems);auto restored=s2ne::OriginalSaveBridge::restore(root,state);auto loaded=std::make_unique<s2ne::Prologue>(root,restored.events.difficulty);loaded->set_disc(discForSave(restored.raw));loaded->restore_battle(std::move(restored.events),restored.turn);loaded->set_small_font(settingsState.small_font!=0);prologue=std::move(loaded);rawSave=std::move(restored.raw);connectedCampaign=nullptr;autosaveBlocked=false;saveMenu->complete(true);inMenu=false;inSetup=false;showPrologue();}catch(const std::exception& ex){saveMenu->complete(false);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Original load",ex.what(),window);}}
                    if(saveMenu->closed()){saveMenu.reset();if(!prologue)showMenu();}
                    continue;
                }
                if(battleReports&&!battleMenu){
                    s2ne::OriginalSettings::Event input;
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){input.type=1;input.ascii=event.key.key<128?unsigned(event.key.key):0;
                        switch(event.key.key){case SDLK_ESCAPE:input.scan=1;break;case SDLK_UP:input.scan=0x48;break;case SDLK_DOWN:input.scan=0x50;break;case SDLK_LEFT:input.scan=0x4b;break;case SDLK_RIGHT:input.scan=0x4d;break;case SDLK_RETURN:input.scan=0x1c;break;case SDLK_SPACE:input.scan=0x39;break;}}
                    else if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_ConvertEventToRenderCoordinates(renderer,&event);bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;input.type=down?0x20:8;input.x=int(down?event.button.x:event.motion.x);input.y=int(down?event.button.y:event.motion.y);input.buttons=down?(event.button.button==SDL_BUTTON_LEFT?1:event.button.button==SDL_BUTTON_RIGHT?2:0):0;}
                    battleReports->event(input);
                    if(battleReports->page()==s2ne::OriginalBattleReports::Page::closed)battleReports.reset();
                    showPrologue();continue;
                }
                if(promotionMenu){
                    s2ne::OriginalSettings::Event input;
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){input.type=1;
                        switch(event.key.key){case SDLK_ESCAPE:input.scan=1;break;case SDLK_UP:input.scan=0x48;break;case SDLK_DOWN:input.scan=0x50;break;case SDLK_LEFT:input.scan=0x4B;break;case SDLK_RIGHT:input.scan=0x4D;break;case SDLK_TAB:input.scan=0x0F;break;case SDLK_RETURN:input.scan=0x1C;break;case SDLK_SPACE:input.scan=0x39;break;}}
                    else if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_ConvertEventToRenderCoordinates(renderer,&event);bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;
                        input.type=down?0x20:8;input.x=int(down?event.button.x:event.motion.x);input.y=int(down?event.button.y:event.motion.y);input.buttons=down?(event.button.button==SDL_BUTTON_LEFT?1:event.button.button==SDL_BUTTON_RIGHT?2:0):0;}
                    promotionMenu->event(input);
                    if(promotionMenu->page()==s2ne::OriginalPromotion::Page::closed)promotionMenu.reset();
                    showPrologue();continue;
                }
                if(battleMenu){
                    s2ne::OriginalSettings::Event input;
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){input.type=1;
                        switch(event.key.key){case SDLK_ESCAPE:input.scan=1;break;case SDLK_UP:input.scan=0x48;break;case SDLK_DOWN:input.scan=0x50;break;case SDLK_LEFT:input.scan=0x4b;break;case SDLK_RIGHT:input.scan=0x4d;break;case SDLK_RETURN:input.scan=0x1c;break;case SDLK_SPACE:input.scan=0x39;break;}}
                    else if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_ConvertEventToRenderCoordinates(renderer,&event);bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;input.type=down?0x20:8;input.x=int(down?event.button.x:event.motion.x);input.y=int(down?event.button.y:event.motion.y);input.buttons=down?(event.button.button==SDL_BUTTON_LEFT?1:event.button.button==SDL_BUTTON_RIGHT?2:0):0;}
                    if(saveMenu){
                                            if(event.type==SDL_EVENT_KEY_DOWN&&event.key.key==SDLK_RETURN)input.ascii=13;
                                            s2ne::OriginalSaveMenu::Result result;
                        if(overwriteConfirmation){
                            if(input.scan==1||(input.buttons&2))overwriteConfirmation->cancel();else if(input.scan==0x4b||input.scan==0x4d)overwriteConfirmation->toggle();else if(input.scan==0x1c||input.scan==0x39)overwriteConfirmation->confirm();else if(input.type==8||input.type==0x20)overwriteConfirmation->mouse(float(input.x),float(input.y),(input.buttons&1)!=0);
                            if(auto yes=overwriteConfirmation->result()){result=saveMenu->confirm_overwrite(*yes);overwriteConfirmation.reset();}
                        }else result=saveMenu->event({input.type,input.x,input.y,input.buttons,input.ascii,input.scan});
                        if(result.action==s2ne::OriginalSaveMenu::Action::confirm_overwrite)overwriteConfirmation.emplace(root,menu,253,176,76+30*saveMenu->row(),true);
                        if(result.action==s2ne::OriginalSaveMenu::Action::load||result.action==s2ne::OriginalSaveMenu::Action::save){
                            try{
                                auto path=s2ne::OriginalSave::slot_path(root,result.slot);
                                if(result.action==s2ne::OriginalSaveMenu::Action::load){auto state=s2ne::OriginalSave::load(path,saveItems);auto restored=s2ne::OriginalSaveBridge::restore(root,state);prologue->set_disc(discForSave(restored.raw));prologue->restore_battle(std::move(restored.events),restored.turn);rawSave=std::move(restored.raw);connectedCampaign=nullptr;autosaveBlocked=false;}
                                else{auto state=s2ne::OriginalSaveBridge::capture(prologue->events,prologue->battle_turn(),prologue->camera(),rawSave?&*rawSave:nullptr);s2ne::OriginalSaveStore::write(isolatedSaveRoot?*isolatedSaveRoot:root,result.slot,state,saveItems);rawSave=std::move(state);}
                                saveMenu->complete(true);
                            }catch(const std::exception& ex){saveMenu->complete(false);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Original save/load",ex.what(),window);}
                        }
                        if(saveMenu->closed()){int code=saveMenu->return_code();saveMenu.reset();battleMenu->complete_operation(code);}
                    }
                    else if(battleReports){battleReports->event(input);if(battleReports->page()==s2ne::OriginalBattleReports::Page::closed){if(auto location=battleReports->location()){auto point=prologue->screen_cell({location->first,location->second});prologue->pan(point.x-320,point.y-240);}int code=battleReports->return_code();battleReports.reset();battleMenu->complete_operation(code);}}
                    else battleMenu->event(input);
                    auto request=battleMenu->request();
                    if(!battleReports&&(request.action==s2ne::OriginalBattleMenu::Action::conditions||request.action==s2ne::OriginalBattleMenu::Action::inspect_characters||request.action==s2ne::OriginalBattleMenu::Action::inspect_map)){
                        std::vector<s2ne::OriginalBattleReports::Entry> entries;
                        for(const auto& unit:prologue->events.units)if(unit.present(prologue->battle_turn()))entries.push_back({unit,unit.hostile(),unit.npc_class()?1:0,false});
                        auto page=request.action==s2ne::OriginalBattleMenu::Action::conditions?s2ne::OriginalBattleReports::Page::conditions:request.action==s2ne::OriginalBattleMenu::Action::inspect_map?s2ne::OriginalBattleReports::Page::map:s2ne::OriginalBattleReports::Page::roster;
                        s2ne::OriginalMap overview(root);overview.set_dynamic_object_chains(prologue->events.dynamic_object_chains);overview.set_event_flags(prologue->events.flags);overview.load(prologue->events.map_id);
                        battleReports.emplace(root,page,prologue->events,std::move(entries),prologue->draw(menu),&overview,settingsState.small_font!=0);
                    }
                    if(!saveMenu&&(request.action==s2ne::OriginalBattleMenu::Action::load||request.action==s2ne::OriginalBattleMenu::Action::save)){saveMenu.emplace(root,request.action==s2ne::OriginalBattleMenu::Action::save?1:0,s2ne::OriginalSaveMenu::scan(root));}
                    if(auto code=battleMenu->return_code()){settingsState=battleMenu->settings_state();prologue->set_small_font(settingsState.small_font!=0);battleMenu.reset();if(*code==3)prologue->end_battle_turn();else if(*code==6)prologue->start_auto_battle();else if(*code==11)running=false;}
                    showPrologue();continue;
                }
                if(prologue){
                    if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
                        SDL_ConvertEventToRenderCoordinates(renderer,&event);
                        bool click=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT;
                        float x=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.x:event.button.x;
                        float y=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.y:event.button.y;
                        if(prologue->in_battle()&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT&&prologue->battle_state()==s2ne::Battle::State::idle){
                            auto slots=s2ne::OriginalSaveMenu::scan(root);s2ne::OriginalBattleMenu::Context context;context.saves_present=std::any_of(slots.begin(),slots.end(),[](const auto& slot){return slot.occupied();});
                            battleMenu.emplace(root,context,settingsState);showPrologue();continue;
                        }
                        if(prologue->choosing()||prologue->in_battle())prologue->mouse(x,y,click);
                        else if(click)prologue->advance();
                        else if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT)prologue->reveal_text();
                        if(prologue->in_battle()&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT)prologue->cancel_battle_selection();
                        showPrologue();
                    }
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                        switch(event.key.key){
                        case SDLK_UP:prologue->move(-1);break;
                        case SDLK_DOWN:prologue->move(1);break;
                        case SDLK_LEFT:if(prologue->in_battle()&&(prologue->battle_state()==s2ne::Battle::State::command||prologue->battle_state()==s2ne::Battle::State::finish_confirm))prologue->battle_cycle(-1);else prologue->pan(-32,0);break;
                        case SDLK_RIGHT:if(prologue->in_battle()&&(prologue->battle_state()==s2ne::Battle::State::command||prologue->battle_state()==s2ne::Battle::State::finish_confirm))prologue->battle_cycle(1);else prologue->pan(32,0);break;
                        case SDLK_ESCAPE:prologue->cancel_battle_selection();break;
                        case SDLK_RETURN:if(prologue->in_battle())prologue->battle_confirm();else prologue->advance();break;
                        case SDLK_SPACE:if(prologue->in_battle())prologue->battle_confirm();else prologue->advance();break;
                        case SDLK_LALT:case SDLK_RALT:prologue->reveal_text();break;
                        case SDLK_TAB:if(prologue->in_battle())prologue->battle_cycle_target();break;
                        case SDLK_HOME:prologue->finish_facing(3);break;
                        case SDLK_PAGEUP:prologue->finish_facing(2);break;
                        case SDLK_END:prologue->finish_facing(0);break;
                        case SDLK_PAGEDOWN:prologue->finish_facing(1);break;
                        case SDLK_F10:if(prologue->in_battle())prologue->end_battle_turn();break;
                        default:break;
                        }
                        showPrologue();
                    }
                    if(prologue->in_battle())SDL_SetWindowTitle(window,("Battle subset - "+prologue->battle_status()).c_str());
                    continue;
                }
                if(inSetup && (event.type==SDL_EVENT_MOUSE_MOTION || event.type==SDL_EVENT_MOUSE_BUTTON_DOWN)){
                    SDL_ConvertEventToRenderCoordinates(renderer,&event);
                    bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;
                    float x=down?event.button.x:event.motion.x,y=down?event.button.y:event.motion.y;
                    if(down&&event.button.button==SDL_BUTTON_RIGHT){inSetup=false;showMenu();continue;}
                    bool confirm=setup.mouse(x,y,down&&event.button.button==SDL_BUTTON_LEFT);
                    if(confirm)startGame();else if(down)showSetup();
                    continue;
                }
                if (inMenu && !inSetup && (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)) {
                    SDL_ConvertEventToRenderCoordinates(renderer, &event);
                    bool click = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT;
                    float x = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                    float y = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                    if (menu.mouse(x,y,click)) { auto target=menu.activate(); if(menu.selected==0){showSetup();continue;} if(menu.selected==1){saveMenu.emplace(root,0,s2ne::OriginalSaveMenu::scan(root));continue;} if(menu.selected==4){beginExitConfirmation();continue;} if(menu.selected==3){beginSettings();continue;} std::cout << "Selected original menu row " << menu.selected << ", handler 0x" << std::hex << target << std::dec << " (not implemented)\n"; }
                    showMenu();
                }
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                    if(inSetup){
                        if(event.key.key==SDLK_ESCAPE){inSetup=false;showMenu();}
                        else if(event.key.key==SDLK_UP){setup.move(-1);showSetup();}
                        else if(event.key.key==SDLK_DOWN){setup.move(1);showSetup();}
                        else if(event.key.key==SDLK_RETURN||event.key.key==SDLK_SPACE){startGame();}
                        continue;
                    }
                    if (event.key.key == SDLK_ESCAPE && !inMenu) showMenu();
                    if (inMenu && event.key.key == SDLK_UP) { menu.move(-1); showMenu(); }
                    if (inMenu && event.key.key == SDLK_DOWN) { menu.move(1); showMenu(); }
                    if (inMenu && (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)) {
                        auto target = menu.activate();
                        if(menu.selected==0){showSetup();continue;}
                        if(menu.selected==1){saveMenu.emplace(root,0,s2ne::OriginalSaveMenu::scan(root));continue;}
                        if(menu.selected==4){beginExitConfirmation();continue;}
                        if(menu.selected==3){beginSettings();continue;}
                        std::cout << "Selected original menu row " << menu.selected << ", handler 0x" << std::hex << target << std::dec << " (not implemented)\n";
                        SDL_SetWindowTitle(window, "Original menu selected - handler not implemented (see console)");
                    }
                    if (event.key.key == SDLK_SPACE && !inMenu) paused = !paused;
                }
            }
            auto& a = clips[clip];
            if (!paused && SDL_GetTicks() >= next) {
                if (!a.next(rgba)) {
                    if (clip + 1 < clips.size()) ++clip;
                    else {
                        showMenu();
                    }
                } else {
                    if (!SDL_UpdateTexture(texture, nullptr, rgba.data(), 640 * 4)) throw std::runtime_error(SDL_GetError());
                    next = SDL_GetTicks() + a.delay;
                }
            }
            if(prologue&&connectedCampaign!=prologue.get()){
                prologue->set_campaign_arrival_adapter([&](const auto& current,unsigned turn)->std::optional<s2ne::Prologue::ArrivalTransaction>{
                    if(!rawSave)return std::nullopt;
                    auto captured=s2ne::OriginalSaveBridge::capture(current,turn,prologue->camera(),&*rawSave);
                    s2ne::OriginalEvents expanded(root,current.difficulty);expanded.load_stage(current.stage);
                    auto inserted=initialization.insert_arrivals(captured,expanded,turn,initialization.placement_map(root,expanded.map_id));
                    auto arrivals=s2ne::OriginalSaveBridge::restore(root,inserted);
                    auto pending=std::make_shared<s2ne::OriginalSave::State>(std::move(inserted));
                    return s2ne::Prologue::ArrivalTransaction{std::move(arrivals.events),[&,pending]()noexcept{using std::swap;swap(*rawSave,*pending);}};
                },rawSave?prologue->battle_turn():1);
                prologue->set_campaign_scripted_roles_adapter([&](const auto& current,unsigned eventIndex)->std::optional<s2ne::Prologue::ArrivalTransaction>{
                    if(!rawSave)return std::nullopt;
                    auto captured=s2ne::OriginalSaveBridge::capture(current,prologue->battle_turn(),prologue->camera(),&*rawSave);
                    s2ne::OriginalEvents expanded(root,current.difficulty);expanded.load_stage(current.stage);
                    auto inserted=initialization.insert_scripted_roles(captured,expanded,eventIndex,initialization.placement_map(root,expanded.map_id));
                    auto restored=s2ne::OriginalSaveBridge::restore(root,inserted);
                    auto pending=std::make_shared<s2ne::OriginalSave::State>(std::move(inserted));
                    return s2ne::Prologue::ArrivalTransaction{std::move(restored.events),[&,pending]()noexcept{using std::swap;swap(*rawSave,*pending);}};
                });
                prologue->set_campaign_reward_adapter([&](auto& staged,const auto& plan){
                    if(!rawSave)return false;
                    return prologue->battle().mutate_random([&](auto& rng,uint32_t seconds){
                        auto result=campaignPersistence.settle(staged,plan,*rawSave,prologue->battle_turn(),rng,seconds);
                        using std::swap;swap(staged,result.events);swap(*rawSave,result.raw);return true;
                    });
                });
                prologue->set_interbattle_adapter([](auto){return s2ne::OriginalInterbattle::Result::unsupported;});
                connectedCampaign=prologue.get();
            }
            if(prologue&&rawSave&&prologue->pending_story_route()){
                auto captured=s2ne::OriginalSaveBridge::capture(prologue->events,prologue->battle_turn(),prologue->camera(),&*rawSave);
                auto next=campaignPersistence.deploy_next(prologue->events,captured);
                auto staged=initialization.transition_stage(root,captured,next);
                auto readGlobal=[](const auto& bytes,size_t at){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bytes.at(at+i))<<(8*i);return int32_t(v);};
                auto disc=s2ne::original_disc_for_campaign(readGlobal(staged.stage_4fb478,12),readGlobal(staged.stage_4fb478,8),readGlobal(staged.globals_4f7cd0,16));
                staged.stage_4fb478[16]=uint8_t(disc);for(size_t i=17;i<20;++i)staged.stage_4fb478[i]=0;
                s2ne::OriginalDiscResources(root).dat_path(disc);
                auto restored=s2ne::OriginalSaveBridge::restore(root,staged);
                prologue->set_disc(disc);prologue->clear_autosave_requests();
                prologue->resume_campaign_story_route(std::move(restored.events));
                using std::swap;swap(*rawSave,staged);prologue->mark_campaign_arrivals_inserted(1);autosaveBlocked=false;showPrologue();
            }
            if(prologue&&rawSave&&prologue->interbattle_active())if(auto request=prologue->interbattle_request();request&&request->option==4){
                auto next=campaignPersistence.deploy_next(prologue->events,*rawSave);
                auto staged=initialization.transition_stage(root,*rawSave,next);
                auto readGlobal=[](const auto& bytes,size_t at){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bytes.at(at+i))<<(8*i);return int32_t(v);};
                auto disc=s2ne::original_disc_for_campaign(readGlobal(staged.stage_4fb478,12),readGlobal(staged.stage_4fb478,8),readGlobal(staged.globals_4f7cd0,16));staged.stage_4fb478[16]=uint8_t(disc);
                for(size_t i=17;i<20;++i)staged.stage_4fb478[i]=0;
                s2ne::OriginalDiscResources(root).dat_path(disc);
                auto restored=s2ne::OriginalSaveBridge::restore(root,staged);
                prologue->set_interbattle_adapter([](auto request){return request.option==4?s2ne::OriginalInterbattle::Result::enter_battle:s2ne::OriginalInterbattle::Result::unsupported;});
                prologue->campaign_scan(0);
                prologue->set_interbattle_adapter([](auto){return s2ne::OriginalInterbattle::Result::unsupported;});
                prologue->set_disc(disc);
                prologue->clear_autosave_requests();prologue->resume_campaign_stage(std::move(restored.events));
                using std::swap;swap(*rawSave,staged);prologue->mark_campaign_arrivals_inserted(1);autosaveBlocked=false;showPrologue();
            }
            if(prologue&&!rawSave&&!prologue->autosave_requests().empty()&&prologue->events.stage!=140){
                auto staged=initialization.fresh(root,prologue->events.stage,prologue->events.difficulty,prologue->events.flags,s2ne::OriginalInitialization::setup_from_settings(settingsState,0));
                auto restored=s2ne::OriginalSaveBridge::restore(root,staged);
                prologue->normalize_campaign_roster(std::move(restored.events),1);
                rawSave=std::move(staged);autosaveBlocked=false;
            }
            if(prologue&&!autosaveBlocked&&!prologue->autosave_requests().empty()){ 
                bool allSaved=true;
                for(const auto& request:prologue->autosave_requests()){
                    if(!rawSave){std::cerr<<"Original autosave "<<request.slot<<" blocked: no complete original baseline\n";allSaved=false;break;}
                    try{
                        auto staged=s2ne::OriginalSaveBridge::capture(prologue->events,std::max(1u,prologue->battle_turn()),prologue->camera(),&*rawSave);
                        s2ne::OriginalSaveStore::write(isolatedSaveRoot?*isolatedSaveRoot:root,request.slot,staged,saveItems);rawSave=std::move(staged);
                    }catch(const std::exception& ex){std::cerr<<"Original autosave "<<request.slot<<" rejected: "<<ex.what()<<'\n';allSaved=false;break;}
                }
                if(allSaved)prologue->clear_autosave_requests();else autosaveBlocked=true;
            }
            if(prologue&&prologue->in_battle()&&!promotionMenu){
                prologue->battle().set_command_request_availability([&](const auto& unit,int command){
                    if(command!=4||!rawSave||rawSave->groups[0].inventory.size()!=saveItems)return false;
                    auto found=std::find_if(rawSave->groups[0].units.begin(),rawSave->groups[0].units.end(),[&](const auto& r){return unsigned(r[2])+(unsigned(r[3])<<8)==unit.id&&unsigned(r[4])+(unsigned(r[5])<<8)==unit.appearance;});
                    if(found==rawSave->groups[0].units.end())return false;
                    auto probe=unit;
                    return promotionRules.eligible(probe,probe.experience_threshold,int(rawSave->globals_4f7cd0[0]))&&!promotionRules.candidates(probe).empty();
                });
            }
            if(prologue&&!battleMenu&&!battleReports&&!promotionMenu&&prologue->battle_requested_command()==4){
                prologue->clear_battle_requested_command();openSelectedPromotion();
            }
            if(prologue&&!battleMenu&&!battleReports&&!promotionMenu&&prologue->battle_requested_command()==3){
                prologue->clear_battle_requested_command();openSelectedDetail();
            }
            if(promotionSmoke&&prologue){
                if(prologue->campaign_defeated()||prologue->campaign_reward_report())throw std::runtime_error("Earned save reached campaign outcome before promotion: defeat="+std::to_string(prologue->campaign_defeated())+" victory="+std::to_string(prologue->campaign_reward_report()));
                if(SDL_GetTicks()-started>30000)throw std::runtime_error("Earned promotion SDL flow timed out: phase="+std::to_string(promotionSmokePhase)+" selected="+std::to_string(prologue->battle_selected())+" state="+std::to_string(int(prologue->battle_state()))+" blocker="+prologue->campaign_blocker()+" dialogue="+std::to_string(prologue->campaign_dialogue())+" modal="+std::to_string(prologue->campaign_modal())+" defeat="+std::to_string(prologue->campaign_defeated())+" reward="+std::to_string(prologue->campaign_reward_report()));
                auto pushPromotionClick=[&](float x,float y){SDL_Event click{};click.type=SDL_EVENT_MOUSE_BUTTON_DOWN;click.button.windowID=SDL_GetWindowID(window);click.button.button=SDL_BUTTON_LEFT;SDL_RenderCoordinatesToWindow(renderer,x,y,&click.button.x,&click.button.y);SDL_PushEvent(&click);};
                if(promotionSmokePhase==0&&prologue->campaign_dialogue()){
                    prologue->reveal_text();prologue->advance();
                }else if(promotionSmokePhase==0&&prologue->in_battle()){ 
                    auto found=std::find_if(prologue->events.units.begin(),prologue->events.units.end(),[&](auto u){return u.controllable()&&u.present(prologue->battle_turn())&&promotionRules.eligible(u,u.experience_threshold)&&!promotionRules.candidates(u).empty();});
                    if(found==prologue->events.units.end())throw std::runtime_error("Earned save contains no eligible deployed promotion candidate");
                    promotionSmokeKind=found->appearance;
                    auto point=prologue->screen_cell({found->x,found->y});prologue->pan(point.x-320,point.y-240);point=prologue->screen_cell({found->x,found->y});
                    prologue->mouse(float(point.x),float(point.y),true);
                    if(prologue->battle_selected()<0)throw std::runtime_error("Earned candidate could not be selected at original projected cell");
                    ++promotionSmokePhase;
                }else if(promotionSmokePhase==1&&prologue->battle_state()==s2ne::Battle::State::command){
                    if(!prologue->battle().command_available_for_cycle(4))throw std::runtime_error("Earned promotion command disabled");
                    if(prologue->battle().command_index()!=4){SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_RIGHT;SDL_PushEvent(&key);}
                    else{SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_RETURN;SDL_PushEvent(&key);++promotionSmokePhase;}
                }else if(promotionSmokePhase==2&&promotionMenu){
                    promotionSmokeTarget=promotionMenu->selected().kind;SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_RETURN;SDL_PushEvent(&key);++promotionSmokePhase;
                }else if(promotionSmokePhase==3&&promotionMenu&&promotionMenu->page()==s2ne::OriginalPromotion::Page::confirmation){pushPromotionClick(100,430);++promotionSmokePhase;
                }else if(promotionSmokePhase==4&&!promotionMenu){
                    int selected=prologue->battle_selected();
                    if(selected<0||prologue->events.units.at(size_t(selected)).appearance!=promotionSmokeTarget||promotionSmokeKind==promotionSmokeTarget)throw std::runtime_error("SDL promotion did not commit the selected class");
                    auto captured=s2ne::OriginalSaveBridge::capture(prologue->events,prologue->battle_turn(),prologue->camera(),&*rawSave);
                    s2ne::OriginalSaveStore::write(*isolatedSaveRoot,1,captured,saveItems);
                    auto loaded=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(*isolatedSaveRoot,1),saveItems);
                    if(s2ne::OriginalSave::encode(captured,saveItems)!=s2ne::OriginalSave::encode(loaded,saveItems))throw std::runtime_error("Promoted original save did not roundtrip");
                    auto restored=s2ne::OriginalSaveBridge::restore(root,loaded);
                    if(restored.events.units.at(size_t(selected)).appearance!=promotionSmokeTarget||restored.events.units.at(size_t(selected)).level!=prologue->events.units.at(size_t(selected)).level)throw std::runtime_error("Promoted original class did not survive runtime restore");
                    std::cout<<"SDL earned save -> command4 -> original promotion selection/confirmation -> class "<<promotionSmokeKind<<" to "<<promotionSmokeTarget<<" -> save reload verified\n";
                    promotionVerified=true;running=false;
                }
                showPrologue();
            }
            if(prologue&&prologueRedraw)renderPrologue();
            else if(!prologue&&saveMenu){auto image=title;menu.draw(image);saveMenu->draw(image,menu);if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());}
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            if (!SDL_RenderPresent(renderer)) throw std::runtime_error(SDL_GetError());
                        ++presented;
                        SDL_Delay(10);
        }
        if(promotionSmoke&&!promotionVerified)throw std::runtime_error("Earned promotion validation interrupted");
        if(clearSmoke&&!clearVerified)throw std::runtime_error("First-battle validation interrupted before original victory report");
        if(feedbackSmoke&&(!feedbackFlash||!feedbackNumbers||!feedbackHp||!feedbackReaction))throw std::runtime_error("Feedback validation interrupted before HP, flash, numbers and original indexed reaction were observed");
        std::cout << "Presented " << presented << " frames\n";
        audio.stop();
        SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
        return 1;
    }
}
