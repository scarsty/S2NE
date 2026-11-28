#include "presentation.h"

namespace s2ne
{
auto paste_page_center(std::span<uint8_t> wide, std::span<const uint8_t> center) -> void
{
    if (wide.size() != size_t(screen_width) * screen_height * 4 || center.size() != 640u * 480 * 4) { fatal("Invalid full width canvas"); }
    for (int y = 0; y < 480; ++y) { std::copy_n(center.begin() + size_t(y) * 640 * 4, 640 * 4, wide.begin() + (size_t(y + ui_offset_y()) * screen_width + ui_offset_x()) * 4); }
}

FullWidthCanvas::FullWidthCanvas(std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> background) : background_(std::move(background)) {}

auto FullWidthCanvas::empty() const -> bool { return pixels_.empty(); }

auto FullWidthCanvas::set_background(const std::vector<uint8_t>& image) -> void { pixels_ = image; }

auto FullWidthCanvas::background() -> std::vector<uint8_t>& { return pixels_; }

auto FullWidthCanvas::compose(const std::vector<uint8_t>& center) const -> std::vector<uint8_t>
{
    auto wide = pixels_.empty() ? background_(center) : pixels_;
    paste_page_center(wide, center);
    return wide;
}

auto fade_full_width_canvas(std::vector<uint8_t>& image, unsigned intensity, FullWidthCanvas* canvas) -> void
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

auto set_fixed_page_background(std::span<const uint8_t> pixels) -> void
{
    if (pixels.size() != size_t(screen_width) * screen_height * 4) { fatal("Invalid fixed page background"); }
    fixed_page_background.assign(pixels.begin(), pixels.end());
}

auto fixed_page_center() -> std::vector<uint8_t>
{
    if (fixed_page_background.size() != size_t(screen_width) * screen_height * 4) { fatal("Invalid fixed page background"); }
    std::vector<uint8_t> result(640 * 480 * 4);
    for (int y = 0; y < 480; ++y) { std::copy_n(fixed_page_background.begin() + (size_t(y + ui_offset_y()) * screen_width + ui_offset_x()) * 4, 640 * 4, result.begin() + size_t(y) * 640 * 4); }
    return result;
}

auto compose_fixed_page(std::span<const uint8_t> image) -> std::vector<uint8_t>
{
    auto result = fixed_page_background;
    paste_page_center(result, image);
    return result;
}

auto set_full_width_fixed_page(SDL_Renderer* renderer, bool enabled) -> void
{
    if (!SDL_SetBooleanProperty(SDL_GetRendererProperties(renderer), "s2ne.full-width-fixed-page", enabled)) { fatal(SDL_GetError()); }
}

auto full_width_fixed_page(SDL_Renderer* renderer) -> bool
{
    return SDL_GetBooleanProperty(SDL_GetRendererProperties(renderer), "s2ne.full-width-fixed-page", false);
}

auto configure_original_presentation(SDL_Renderer* renderer, SDL_Texture* texture, SDL_ScaleMode scale_mode) -> Result<bool>
{
    if (!renderer || !texture)
    {
        return Result<bool>::failure(make_error("Original presentation requires renderer and texture"));
    }
    if (!SDL_SetRenderLogicalPresentation(renderer, screen_width, screen_height, SDL_LOGICAL_PRESENTATION_LETTERBOX) || !SDL_SetTextureScaleMode(texture, scale_mode) || !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE))
    {
        return Result<bool>::failure(make_error(SDL_GetError()));
    }
    return Result<bool>::success(true);
}

auto legacy_coordinates_from_window(SDL_Renderer* renderer, float x, float y, float* rx, float* ry) -> bool
{
    if (!SDL_RenderCoordinatesFromWindow(renderer, x, y, rx, ry)) { return false; }
    *rx -= ui_offset_x();
    *ry -= ui_offset_y();
    return true;
}

auto legacy_coordinates_to_window(SDL_Renderer* renderer, float x, float y, float* wx, float* wy) -> bool
{
    return SDL_RenderCoordinatesToWindow(renderer, x + ui_offset_x(), y + ui_offset_y(), wx, wy);
}

auto legacy_event_coordinates(SDL_Renderer* renderer, SDL_Event* event) -> bool
{
    if (!SDL_ConvertEventToRenderCoordinates(renderer, event)) { return false; }
    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        event->motion.x -= ui_offset_x();
        event->motion.y -= ui_offset_y();
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        event->button.x -= ui_offset_x();
        event->button.y -= ui_offset_y();
    }
    return true;
}

auto render_logical_frame(SDL_Renderer* renderer, SDL_Texture* texture) -> bool
{
    float width{}, height{};
    if (!SDL_GetTextureSize(texture, &width, &height)) { return false; }
    const SDL_FRect destination{ float(int(screen_width - width) / 2), float(int(screen_height - height) / 2), width, height };
    return SDL_RenderTexture(renderer, texture, nullptr, &destination);
}
}    //namespace s2ne
