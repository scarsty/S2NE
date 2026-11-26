#pragma once
#include "runtime_error.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <functional>
#include <span>
#include <vector>

namespace s2ne
{
inline std::vector<uint8_t> fixed_page_background;
inline void paste_page_center(std::span<uint8_t> wide, std::span<const uint8_t> center)
{
    if (wide.size() != 768u * 480 * 4 || center.size() != 640u * 480 * 4) { fatal("Invalid full width canvas"); }
    for (int y = 0; y < 480; ++y) { std::copy_n(center.begin() + size_t(y) * 640 * 4, 640 * 4, wide.begin() + (size_t(y) * 768 + 64) * 4); }
}
class FullWidthCanvas
{
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> background_;
    std::vector<uint8_t> pixels_;

public:
    explicit FullWidthCanvas(std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> background) : background_(std::move(background)) {}
    FullWidthCanvas(const FullWidthCanvas&) = delete;
    FullWidthCanvas& operator=(const FullWidthCanvas&) = delete;
    bool empty() const { return pixels_.empty(); }
    void set_background(const std::vector<uint8_t>& image) { pixels_ = image; }
    std::vector<uint8_t>& background() { return pixels_; }
    std::vector<uint8_t> compose(const std::vector<uint8_t>& center) const
    {
        auto wide = pixels_.empty() ? background_(center) : pixels_;
        paste_page_center(wide, center);
        return wide;
    }
    template <class Effect>
    auto transform(std::vector<uint8_t>& center, Effect effect)
    {
        auto wide = compose(center);
        auto result = effect(wide);
        if (!result) { return result; }
        for (int y = 0; y < 480; ++y) { std::copy_n(wide.begin() + (size_t(y) * 768 + 64) * 4, 640 * 4, center.begin() + size_t(y) * 640 * 4); }
        pixels_ = std::move(wide);
        return result;
    }
};
inline void fade_full_width_canvas(std::vector<uint8_t>& image, unsigned intensity, FullWidthCanvas* canvas = nullptr)
{
    auto effect = [intensity](auto& pixels)
    {
        for (size_t p = 0; p < pixels.size(); p += 4)
        {
            for (unsigned c = 0; c < 3; ++c)
            {
                const unsigned dac = (unsigned(pixels[p + c]) * 63 + 254) / 255;
                pixels[p + c] = uint8_t((dac * 4) * intensity / 255);
            }
        }
        return true;
    };
    if (canvas) { canvas->transform(image, effect); }
    else
    {
        effect(image);
    }
}
inline void set_fixed_page_background(std::span<const uint8_t> pixels)
{
    if (pixels.size() != 768u * 480 * 4) { fatal("Invalid fixed page background"); }
    fixed_page_background.assign(pixels.begin(), pixels.end());
}
inline std::vector<uint8_t> fixed_page_center()
{
    if (fixed_page_background.size() != 768u * 480 * 4) { fatal("Invalid fixed page background"); }
    std::vector<uint8_t> result(640 * 480 * 4);
    for (int y = 0; y < 480; ++y) { std::copy_n(fixed_page_background.begin() + (size_t(y) * 768 + 64) * 4, 640 * 4, result.begin() + size_t(y) * 640 * 4); }
    return result;
}
inline std::vector<uint8_t> compose_fixed_page(std::span<const uint8_t> image)
{
    auto result = fixed_page_background;
    paste_page_center(result, image);
    return result;
}
inline void set_full_width_fixed_page(SDL_Renderer* renderer, bool enabled)
{
    if (!SDL_SetBooleanProperty(SDL_GetRendererProperties(renderer), "s2ne.full-width-fixed-page", enabled)) { fatal(SDL_GetError()); }
}
inline bool full_width_fixed_page(SDL_Renderer* renderer)
{
    return SDL_GetBooleanProperty(SDL_GetRendererProperties(renderer), "s2ne.full-width-fixed-page", false);
}
// Filtering does not change the indexed game pixels.
inline Result<bool> configure_original_presentation(SDL_Renderer* renderer, SDL_Texture* texture, SDL_ScaleMode scale_mode = SDL_SCALEMODE_LINEAR)
{
    if (!renderer || !texture)
    {
        return Result<bool>::failure(make_error("Original presentation requires renderer and texture"));
    }
    if (!SDL_SetRenderLogicalPresentation(renderer, 768, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX) || !SDL_SetTextureScaleMode(texture, scale_mode) || !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE))
    {
        return Result<bool>::failure(make_error(SDL_GetError()));
    }
    return Result<bool>::success(true);
}
inline bool legacy_coordinates_from_window(SDL_Renderer* renderer, float x, float y, float* rx, float* ry)
{
    if (!SDL_RenderCoordinatesFromWindow(renderer, x, y, rx, ry)) { return false; }
    *rx -= 64;
    return true;
}
inline bool legacy_coordinates_to_window(SDL_Renderer* renderer, float x, float y, float* wx, float* wy)
{
    return SDL_RenderCoordinatesToWindow(renderer, x + 64, y, wx, wy);
}
inline bool legacy_event_coordinates(SDL_Renderer* renderer, SDL_Event* event)
{
    if (!SDL_ConvertEventToRenderCoordinates(renderer, event)) { return false; }
    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        event->motion.x -= 64;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) { event->button.x -= 64; }
    return true;
}
inline bool render_logical_frame(SDL_Renderer* renderer, SDL_Texture* texture)
{
    float width{}, height{};
    if (!SDL_GetTextureSize(texture, &width, &height)) { return false; }
    const SDL_FRect destination{ (768 - width) / 2, 0, width, height };
    return SDL_RenderTexture(renderer, texture, nullptr, &destination);
}
}    //namespace s2ne
