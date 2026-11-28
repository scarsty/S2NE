#include "wide_viewport.h"

namespace s2ne
{
auto WideViewport::margin() -> int { return ui_offset_x(); }

auto WideViewport::bytes() -> size_t { return size_t(width) * height * 4; }

auto WideViewport::render(const std::function<std::vector<uint8_t>(int, int)>& tile) -> std::vector<uint8_t>
{
    std::vector<uint8_t> output(bytes());
    for (int top = 0; top < height; top += 480)
    {
        for (int left = 0; left < width; left += 640)
        {
            auto pixels = tile(left - ui_offset_x(), top - ui_offset_y());
            for (int y = 0; y < std::min(480, height - top); ++y)
            {
                std::copy_n(pixels.begin() + size_t(y) * 640 * 4, std::min(640, width - left) * 4, output.begin() + (size_t(top + y) * width + left) * 4);
            }
        }
    }
    return output;
}

auto WideViewport::compose(std::span<const uint8_t> center, std::span<const uint8_t> left, std::span<const uint8_t> right) -> std::vector<uint8_t>
{
    if (center.size() != legacy_bytes || left.size() != legacy_bytes || right.size() != legacy_bytes)
    {
        throw std::invalid_argument("Invalid wide viewport tile extent");
    }
    std::vector<uint8_t> output(bytes());
    const int right_margin = width - margin() - legacy_width;
    if (margin() > legacy_width || right_margin > legacy_width) { throw std::invalid_argument("Wide viewport side exceeds original tile"); }
    for (int y = 0; y < 480; ++y)
    {
        const size_t source = size_t(y) * legacy_width * 4, destination = size_t(y + ui_offset_y()) * width * 4;
        std::copy_n(left.begin() + source, margin() * 4, output.begin() + destination);
        std::copy_n(center.begin() + source, legacy_width * 4, output.begin() + destination + margin() * 4);
        std::copy_n(right.begin() + source + (legacy_width - right_margin) * 4, right_margin * 4, output.begin() + destination + (margin() + legacy_width) * 4);
    }
    return output;
}
}    //namespace s2ne
