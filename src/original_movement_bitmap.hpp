#pragma once
#include "resource_result.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace s2ne::original_movement
{
class SharedEffectClock
{
    unsigned phase_{ };

public:
    unsigned phase() const { return phase_; }
    // 45CA1D draws the current global slot BEFORE testing the timer.
    // Commit only after the corresponding frame is actually presented.
    void presented(bool camera, uint64_t now_ms, uint64_t& actor_stamp_ms)
    {
        const uint64_t now = now_ms / 10, stamp = actor_stamp_ms / 10;
        if (camera || (now >= stamp ? now - stamp : stamp - now) >= 19)
        {
            phase_ ^= 1;
            if (!camera) { actor_stamp_ms = now_ms; }
        }
    }
};
inline Result<std::vector<uint8_t>> try_shadow(std::span<const uint8_t> background, std::span<const uint8_t> foot, std::span<const uint8_t> mapping)
{
    if (background.size() != 2048 || foot.size() != 2048 || mapping.size() != 256)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("448B99 requires indexed64x32 background/foot and256 palette map"));
    }
    std::vector<uint8_t> output(2048);
    for (size_t i = 0; i < output.size(); ++i)
    {
        output[i] = foot[i] ? mapping[background[i]] : 0;
    }
    return Result<std::vector<uint8_t>>::success(std::move(output));
}
inline Result<bool> try_erase(std::span<uint8_t> destination, unsigned width, unsigned height, std::span<const uint8_t> mask, unsigned mask_width, unsigned mask_height, int left, int top)
{
    if (!width || !height || !mask_width || !mask_height || size_t(width) * height != destination.size() || size_t(mask_width) * mask_height != mask.size())
    {
        return Result<bool>::failure(make_error("423D76 bitmap extent mismatch"));
    }
    for (unsigned y = 0; y < mask_height; ++y)
    {
        for (unsigned x = 0; x < mask_width; ++x)
        {
            int dx = left + int(x), dy = top + int(y);
            if (mask[size_t(y) * mask_width + x] && dx >= 0 && dy >= 0 && dx < int(width) && dy < int(height))
            {
                destination[size_t(dy) * width + unsigned(dx)] = 0;
            }
        }
    }
    return Result<bool>::success(true);
}
}    //namespace s2ne::original_movement
