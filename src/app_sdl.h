#pragma once
#include <SDL3/SDL.h>
namespace s2ne
{
enum class MenuKeys
{
    navigation,
    tabbed_navigation,
    paged_navigation,
    full_navigation
};
constexpr unsigned original_menu_scan(SDL_Keycode key, MenuKeys keys)
{
    switch (key)
    {
    case SDLK_ESCAPE: return 1;
    case SDLK_UP: return 0x48;
    case SDLK_DOWN: return 0x50;
    case SDLK_LEFT: return 0x4B;
    case SDLK_RIGHT: return 0x4D;
    case SDLK_RETURN: return 0x1C;
    case SDLK_SPACE: return 0x39;
    case SDLK_TAB:
        if (keys == MenuKeys::tabbed_navigation || keys == MenuKeys::full_navigation) { return 0x0F; }
        break;
    case SDLK_PAGEUP:
        if (keys == MenuKeys::paged_navigation || keys == MenuKeys::full_navigation) { return 0x49; }
        break;
    case SDLK_PAGEDOWN:
        if (keys == MenuKeys::paged_navigation || keys == MenuKeys::full_navigation) { return 0x51; }
        break;
    case SDLK_HOME:
        if (keys == MenuKeys::full_navigation) { return 0x47; }
        break;
    case SDLK_END:
        if (keys == MenuKeys::full_navigation) { return 0x4F; }
        break;
    }
    return 0;
}
class AppSdlSession
{
public:
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
    AppSdlSession() = default;
    AppSdlSession(const AppSdlSession&) = delete;
    AppSdlSession& operator=(const AppSdlSession&) = delete;
    ~AppSdlSession();
};
}    //namespace s2ne
