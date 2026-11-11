#pragma once
#include "runtime_error.hpp"
#include <SDL3/SDL.h>

namespace s2ne
{
// Filtering is applied only when presenting the unchanged 640x480 framebuffer.
inline Result<bool> configure_original_presentation(SDL_Renderer* renderer, SDL_Texture* texture, SDL_ScaleMode scale_mode = SDL_SCALEMODE_LINEAR)
{
    if (!renderer || !texture)
    {
        return Result<bool>::failure(make_error("Original presentation requires renderer and texture"));
    }
    if (!SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX) || !SDL_SetTextureScaleMode(texture, scale_mode) || !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE))
    {
        return Result<bool>::failure(make_error(SDL_GetError()));
    }
    return Result<bool>::success(true);
}
}    //namespace s2ne
