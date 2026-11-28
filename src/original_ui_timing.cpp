#include "original_ui_timing.h"

namespace s2ne
{
auto OriginalEdgeScrollTiming::due(uint64_t now, bool active, unsigned delay_ms) -> bool
{
    if (!active)
    {
        active_ = false;
        return false;
    }
    if (!active_ || now < last_ || now - last_ >= delay_ms)
    {
        active_ = true;
        last_ = now;
        return true;
    }
    return false;
}

auto OriginalBannerPresentation::reset() -> void
{
    pose_ = elapsed_ = 0;
    presented_ = false;
}

auto OriginalBannerPresentation::pose() const -> unsigned { return pose_; }

auto OriginalBannerPresentation::presented() -> void { presented_ = true; }

auto OriginalBannerPresentation::tick(unsigned milliseconds) -> bool
{
    elapsed_ = std::min(10u, elapsed_ + std::min(milliseconds, 10u));
    if (!presented_)
    {
        return false;
    }
    if (pose_ == 8)
    {
        return true;
    }
    if (elapsed_ < 10)
    {
        return false;
    }
    ++pose_;
    elapsed_ = 0;
    presented_ = false;
    return false;
}

auto original_ui_frame_delay(uint64_t started, uint64_t now) -> uint64_t
{
    return now >= started && now - started < 10 ? 10 - (now - started) : 0;
}
}    //namespace s2ne
