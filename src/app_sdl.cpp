#include "app_sdl.h"
namespace s2ne
{
AppSdlSession::~AppSdlSession()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
}    //namespace s2ne
