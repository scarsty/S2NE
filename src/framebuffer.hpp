#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace s2ne
{
inline int framebuffer_width(std::span<const uint8_t> pixels)
{
    constexpr size_t row_unit = 480u * 4;
    if (pixels.empty() || pixels.size() % row_unit || pixels.size() / row_unit > size_t(std::numeric_limits<int>::max() / 4)) { return 0; }
    return int(pixels.size() / row_unit);
}
}    //namespace s2ne
