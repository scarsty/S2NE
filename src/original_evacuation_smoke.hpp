#pragma once
#include "runtime_error.hpp"
#include "prologue.hpp"
namespace s2ne {
inline void original_evacuation_smoke(const std::filesystem::path& root,SDL_Renderer* renderer,SDL_Texture* texture){
    OriginalEvents fixture(root,3);fixture.load_stage(42);auto states=fixture.save_event_states();for(size_t i=0;i<states.size();++i)if(i!=14&&i!=17)states[i]=0;fixture.restore_event_states(states);fixture.camera_step_delay=17;
    if(fixture.type(14)!=3||fixture.event_word(14,3)!=39||fixture.type(17)!=9||fixture.completion_flag(17)!=7)s2ne::fatal("Original stage42 evacuation fixture changed");
    for(auto& unit:fixture.units)if(unit.hostile()&&fixture.movement_class(unit)<3){unit.action_state=4;unit.movement_frame=1;break;}
    Prologue game(root,3);game.restore_battle(fixture,1);game.enable_presentation_timing();OriginalMenu font(root);unsigned returns=0,frames=0,removed=0;bool seen=false;
    if(!game.begin_synchronous_death(39,[&]{++returns;}))s2ne::fatal("Evacuation smoke preflight");
    auto start=SDL_GetTicks();while(game.synchronous_death_active()){
        if(SDL_GetTicks()-start>20000||frames>5000)s2ne::fatal("Evacuation SDL timeout");
        SDL_Event injected{};injected.type=SDL_EVENT_KEY_DOWN;injected.key.key=game.evacuation_active()?SDLK_ESCAPE:SDLK_RETURN;SDL_PushEvent(&injected);
        SDL_Event event;while(SDL_PollEvent(&event))if(event.type==SDL_EVENT_KEY_DOWN){if(event.key.key==SDLK_RETURN)game.advance();else game.cancel_battle_selection();}
        if(!game.original_save_boundary_ready()&&game.evacuation_active())seen=true;
        auto image=game.draw(font);if(!SDL_UpdateTexture(texture,nullptr,image.data(),640*4)||!SDL_RenderTexture(renderer,texture,nullptr,nullptr)||!SDL_RenderPresent(renderer))s2ne::fatal(SDL_GetError());
        game.frame_presented();game.tick(100);++frames;
    }
    for(size_t i=0;i<fixture.units.size();++i)if((fixture.units[i].status_flags&0x80)&&!(game.events.units[i].status_flags&0x80))++removed;
    if(!seen||returns!=1||removed<2||game.events.camera_step_delay!=17||!game.original_save_boundary_ready())s2ne::fatal("Evacuation SDL completion invariants");
    std::cout<<"PASS SDL controlled original stage42 records14/17 role39, "<<removed<<" individual enemies, "<<frames<<" acknowledged frames, Escape/save lock, resume once; not natural campaign reachability\n";
}
}
