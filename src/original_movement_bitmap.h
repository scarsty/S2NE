#pragma once
#include "resource_result.h"
#include <cstdint>
#include <span>
#include <vector>
namespace s2ne
{
class SharedEffectClock
{
    unsigned phase_{};

public:
    unsigned phase() const;
    // 45CA1D draws the current global slot BEFORE testing the timer.
    // Commit only after the corresponding frame is actually presented.
    void presented(bool camera, uint64_t now_ms, uint64_t& actor_stamp_ms);
};
Result<std::vector<uint8_t>> try_shadow(std::span<const uint8_t> background, std::span<const uint8_t> foot, std::span<const uint8_t> mapping);
Result<bool> try_erase(std::span<uint8_t> destination, unsigned width, unsigned height, std::span<const uint8_t> mask, unsigned mask_width, unsigned mask_height, int left, int top);
}    //namespace s2ne
