#pragma once
#include <SDL3/SDL.h>
namespace s2ne
{
class AppSdlSession
{
public:
    SDL_Window* window{ };
    SDL_Renderer* renderer{ };
    SDL_Texture* texture{ };
    AppSdlSession() = default;
    AppSdlSession(const AppSdlSession&) = delete;
    AppSdlSession& operator=(const AppSdlSession&) = delete;
    ~AppSdlSession();
};
}    //namespace s2ne
