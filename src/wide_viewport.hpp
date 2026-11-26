#pragma once
#include <algorithm>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne
{
struct WideViewport
{
    static constexpr int width = 768, height = 480, legacy_width = 640, margin = 64;
    static constexpr size_t legacy_bytes = size_t(legacy_width) * height * 4;
    static constexpr size_t bytes = size_t(width) * height * 4;

    static std::vector<uint8_t> compose(std::span<const uint8_t> center, std::span<const uint8_t> left, std::span<const uint8_t> right)
    {
        if (center.size() != legacy_bytes || left.size() != legacy_bytes || right.size() != legacy_bytes)
        {
            throw std::invalid_argument("Invalid wide viewport tile extent");
        }
        std::vector<uint8_t> output(bytes);
        for (int y = 0; y < height; ++y)
        {
            const size_t source = size_t(y) * legacy_width * 4, destination = size_t(y) * width * 4;
            std::copy_n(left.begin() + source, margin * 4, output.begin() + destination);
            std::copy_n(center.begin() + source, legacy_width * 4, output.begin() + destination + margin * 4);
            std::copy_n(right.begin() + source + (legacy_width - margin) * 4, margin * 4, output.begin() + destination + (width - margin) * 4);
        }
        return output;
    }
};
}    //namespace s2ne
