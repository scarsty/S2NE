#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "opening_audio.hpp"
#include "flic.hpp"
#include "title_image.hpp"
#include "original_menu.hpp"
#include "new_game.hpp"
#include "prologue.hpp"
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
        bool option = verify || smoke || newGameSmoke || battleSmoke;
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
        auto settingsState=s2ne::OriginalSettings::load_state(root);
        menu.small_font=settingsState.small_font!=0;
        auto showSettings=[&]{auto image=settings->draw(s2ne::load_title(root,20,false));if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());};
        auto beginSettings=[&]{settings.emplace(root,0,settingsState);showSettings();};
        std::optional<s2ne::OriginalConfirmation> exitConfirmation;
        auto showExitConfirmation=[&]{auto image=s2ne::load_title(root,20,false);exitConfirmation->draw(image,menu);if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());};
        auto beginExitConfirmation=[&]{exitConfirmation.emplace(root,menu,51,200,403,true);showExitConfirmation();};
        auto showPrologue = [&] {
            if(prologue->drawing_original_mouse())SDL_HideCursor();else SDL_ShowCursor();
            auto image=prologue->draw(menu);
            if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4))throw std::runtime_error(SDL_GetError());
        };
        auto startGame = [&] {
            setup.confirm();
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
                prologue->tick(unsigned(nowUi-lastUiTick));
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
                if(prologue){
                    if(event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
                        SDL_ConvertEventToRenderCoordinates(renderer,&event);
                        bool click=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT;
                        float x=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.x:event.button.x;
                        float y=event.type==SDL_EVENT_MOUSE_MOTION?event.motion.y:event.button.y;
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
                        case SDLK_LEFT:if(prologue->in_battle()&&prologue->battle_state()==s2ne::Battle::State::command)prologue->battle_cycle(-1);else prologue->pan(-32,0);break;
                        case SDLK_RIGHT:if(prologue->in_battle()&&prologue->battle_state()==s2ne::Battle::State::command)prologue->battle_cycle(1);else prologue->pan(32,0);break;
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
                    if (menu.mouse(x,y,click)) { auto target=menu.activate(); if(menu.selected==0){showSetup();continue;} if(menu.selected==4){beginExitConfirmation();continue;} if(menu.selected==3){beginSettings();continue;} std::cout << "Selected original menu row " << menu.selected << ", handler 0x" << std::hex << target << std::dec << " (not implemented)\n"; }
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
