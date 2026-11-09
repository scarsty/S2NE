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
        bool loadSmoke=argc>1&&std::string(argv[1])=="--smoke-load";
        bool option = verify || smoke || newGameSmoke || battleSmoke || loadSmoke;
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
        std::optional<s2ne::OriginalConfirmation> overwriteConfirmation;
        std::ifstream itemFile(root/"ITEMSP.BIN",std::ios::binary);
        std::vector<uint8_t> itemBytes{std::istreambuf_iterator<char>(itemFile),{}};
        const size_t saveItems=s2ne::OriginalSave::item_count(itemBytes);
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
            if(prologue->drawing_original_mouse())SDL_HideCursor();else SDL_ShowCursor();
            auto image=prologue->draw(menu);
            if(battleMenu){SDL_ShowCursor();image=battleMenu->draw(image);}
            if(battleReports)image=battleReports->draw();
            if(saveMenu)saveMenu->draw(image,menu);
            if(overwriteConfirmation)overwriteConfirmation->draw(image,menu);
            if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());
        };
        auto startGame = [&] {
            setup.confirm();
            rawSave.reset();
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
        uint64_t lastUiTick=SDL_GetTicks();
        while (running) {
            uint64_t nowUi=SDL_GetTicks();
            if(prologue){
                if(prologue->battle_intro()&&battleSmoke&&battleSmokeStartTurn){if(prologue->hostile_banner())++hostileBannerFrames;else ++playerBannerFrames;}
                prologue->set_mask_surfaces(settingsState.status_window==1);
                prologue->set_checker_mask(settingsState.flag_4f7d0a==1);
                if(!battleMenu)prologue->tick(unsigned(nowUi-lastUiTick));
                if(!battleMenu&&!battleSmoke&&SDL_GetMouseFocus()==window){
                    float x{},y{},rx{},ry{};SDL_GetMouseState(&x,&y);
                    if(SDL_RenderCoordinatesFromWindow(renderer,x,y,&rx,&ry))prologue->edge_scroll(rx,ry);
                }
                showPrologue();
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
                if(inSetup&&smokePhase==2){e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=300;e.button.y=300;}
                if(inSetup&&smokePhase==3){
                    if(setup.difficulty!=1||!inSetup)throw std::runtime_error("Difficulty page did not retain mouse selection");
                    e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=300;e.button.y=425;
                }
                if(prologue&&prologue->choosing()&&alternateSmoke&&prologue->selection()==0)e.key.key=SDLK_DOWN;
                if(prologue&&prologue->in_battle()){
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
                                std::cout<<"SDL campaign battle "<<prologue->events.stage<<": command -> movement -> two complete side-banner cycles verified\n";running=false;
                            }else{
                                battleSmokeStartTurn=prologue->battle_turn();hostileBannerFrames=playerBannerFrames=0;
                                SDL_Event finish{};finish.type=SDL_EVENT_KEY_DOWN;finish.key.key=SDLK_F10;SDL_PushEvent(&finish);
                            }
                        }else throw std::runtime_error("Unexpected SDL battle state");
                    }else{
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
                }else SDL_PushEvent(&e);
                if(++smokePhase>100)throw std::runtime_error("SDL battle entry timed out");
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
            if(loadSmoke&&prologue&&presented>10){if(!prologue->in_battle()||!rawSave)throw std::runtime_error("Title save load did not restore battle");std::cout<<"Original save restored through title menu, stage "<<prologue->events.stage<<", turn "<<prologue->battle_turn()<<'\n';running=false;}
            if(loadSmoke&&SDL_GetTicks()-started>15000)throw std::runtime_error("Original save smoke timed out: phase="+std::to_string(smokePhase)+" menu="+std::to_string(inMenu)+" row="+std::to_string(menu.selected)+" save="+std::to_string(bool(saveMenu)));
            if(newGameSmoke && SDL_GetTicks()-started>15000)throw std::runtime_error("New-game dialogue smoke timed out");
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) running = false;
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
                    if(result.action==s2ne::OriginalSaveMenu::Action::load){try{auto state=s2ne::OriginalSave::load(s2ne::OriginalSave::slot_path(root,result.slot),saveItems);auto restored=s2ne::OriginalSaveBridge::restore(root,state);auto loaded=std::make_unique<s2ne::Prologue>(root,restored.events.difficulty);loaded->restore_battle(std::move(restored.events),restored.turn);loaded->set_small_font(settingsState.small_font!=0);prologue=std::move(loaded);rawSave=std::move(restored.raw);saveMenu->complete(true);inMenu=false;inSetup=false;showPrologue();}catch(const std::exception& ex){saveMenu->complete(false);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Original load",ex.what(),window);}}
                    if(saveMenu->closed()){saveMenu.reset();if(!prologue)showMenu();}
                    continue;
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
                                if(result.action==s2ne::OriginalSaveMenu::Action::load){auto state=s2ne::OriginalSave::load(path,saveItems);auto restored=s2ne::OriginalSaveBridge::restore(root,state);prologue->restore_battle(std::move(restored.events),restored.turn);rawSave=std::move(restored.raw);}
                                else{auto state=s2ne::OriginalSaveBridge::capture(prologue->events,prologue->battle_turn(),prologue->camera(),rawSave?&*rawSave:nullptr);auto bytes=s2ne::OriginalSave::encode(state,saveItems);std::filesystem::create_directories(path.parent_path());auto temporary=path;temporary+=L".s2ne-tmp";if(std::filesystem::exists(temporary))throw std::runtime_error("Previous pending save exists; original slot left untouched");{std::ofstream output(temporary,std::ios::binary);if(!output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())||!output.flush()){output.close();std::filesystem::remove(temporary);throw std::runtime_error("Original save write failed; original slot left untouched");}}try{if(std::filesystem::exists(path)){auto backup=path;backup+=L".s2ne-backup";if(!ReplaceFileW(path.c_str(),temporary.c_str(),backup.c_str(),0,nullptr,nullptr))throw std::runtime_error("Original save replacement failed; existing slot retained");}else if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Original save creation failed");}catch(...){std::error_code cleanup;std::filesystem::remove(temporary,cleanup);throw;}rawSave=std::move(state);}
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
                    if(auto code=battleMenu->return_code()){settingsState=battleMenu->settings_state();prologue->set_small_font(settingsState.small_font!=0);battleMenu.reset();if(*code==3)prologue->end_battle_turn();else if(*code==6)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Original auto battle","Original automatic combat controller is not restored yet. The current turn is unchanged.",window);else if(*code==11)running=false;}
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
            if(prologue&&prologueRedraw)renderPrologue();
            else if(!prologue&&saveMenu){auto image=title;menu.draw(image);saveMenu->draw(image,menu);if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());}
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            if (!SDL_RenderPresent(renderer)) throw std::runtime_error(SDL_GetError());
                        ++presented;
                        SDL_Delay(10);
        }
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
