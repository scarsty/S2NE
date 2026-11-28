#pragma once
#include "framebuffer.h"
#include "runtime_error.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <functional>
#include <span>
#include <vector>

namespace s2ne
{
inline std::vector<uint8_t> fixed_page_background;
void paste_page_center(std::span<uint8_t> wide, std::span<const uint8_t> center);
class FullWidthCanvas
{
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> background_;
    std::vector<uint8_t> pixels_;

public:
    explicit FullWidthCanvas(std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> background);
    FullWidthCanvas(const FullWidthCanvas&) = delete;
    FullWidthCanvas& operator=(const FullWidthCanvas&) = delete;
    bool empty() const;
    void set_background(const std::vector<uint8_t>& image);
    std::vector<uint8_t>& background();
    std::vector<uint8_t> compose(const std::vector<uint8_t>& center) const;
    template <class Effect>
    auto transform(std::vector<uint8_t>& center, Effect effect)
    {
        auto wide = compose(center);
        auto result = effect(wide);
        if (!result) { return result; }
        for (int y = 0; y < 480; ++y) { std::copy_n(wide.begin() + (size_t(y + ui_offset_y()) * screen_width + ui_offset_x()) * 4, 640 * 4, center.begin() + size_t(y) * 640 * 4); }
        pixels_ = std::move(wide);
        return result;
    }
};
void fade_full_width_canvas(std::vector<uint8_t>& image, unsigned intensity, FullWidthCanvas* canvas = nullptr);
void set_fixed_page_background(std::span<const uint8_t> pixels);
std::vector<uint8_t> fixed_page_center();
std::vector<uint8_t> compose_fixed_page(std::span<const uint8_t> image);
void set_full_width_fixed_page(SDL_Renderer* renderer, bool enabled);
bool full_width_fixed_page(SDL_Renderer* renderer);
// Filtering does not change the indexed game pixels.
Result<bool> configure_original_presentation(SDL_Renderer* renderer, SDL_Texture* texture, SDL_ScaleMode scale_mode = SDL_SCALEMODE_LINEAR);
bool legacy_coordinates_from_window(SDL_Renderer* renderer, float x, float y, float* rx, float* ry);
bool legacy_coordinates_to_window(SDL_Renderer* renderer, float x, float y, float* wx, float* wy);
bool legacy_event_coordinates(SDL_Renderer* renderer, SDL_Event* event);
bool render_logical_frame(SDL_Renderer* renderer, SDL_Texture* texture);
}    //namespace s2ne
