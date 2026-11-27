#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace s2ne
{
inline int screen_width = 768;
inline int screen_height = 480;
inline int ui_offset_x() { return (screen_width - 640) / 2; }
inline int ui_offset_y() { return (screen_height - 480) / 2; }
inline int framebuffer_height(std::span<const uint8_t> pixels)
{
    if (pixels.size() == size_t(screen_width) * screen_height * 4) { return screen_height; }
    return 480;
}
inline int framebuffer_width(std::span<const uint8_t> pixels)
{
    const size_t row_unit = size_t(framebuffer_height(pixels)) * 4;
    if (pixels.empty() || pixels.size() % row_unit || pixels.size() / row_unit > size_t(std::numeric_limits<int>::max() / 4)) { return 0; }
    return int(pixels.size() / row_unit);
}
}    //namespace s2ne
