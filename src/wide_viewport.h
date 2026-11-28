#pragma once
#include "framebuffer.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne
{
struct WideViewport
{
    static inline int& width = screen_width;
    static inline int& height = screen_height;
    static constexpr int legacy_width = 640;
    static constexpr size_t legacy_bytes = 640u * 480 * 4;
    static int margin();
    static size_t bytes();
    static std::vector<uint8_t> render(const std::function<std::vector<uint8_t>(int, int)>& tile);

    static std::vector<uint8_t> compose(std::span<const uint8_t> center, std::span<const uint8_t> left, std::span<const uint8_t> right);
};
}    //namespace s2ne
