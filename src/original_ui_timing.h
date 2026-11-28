#pragma once
#include <algorithm>
#include <cstdint>
namespace s2ne
{
class OriginalEdgeScrollTiming
{
    bool active_ = false;
    uint64_t last_ = 0;

public:
    bool due(uint64_t now, bool active, unsigned delay_ms);
};
class OriginalBannerPresentation
{
    unsigned pose_ = 0, elapsed_ = 0;
    bool presented_ = false;

public:
    void reset();
    unsigned pose() const;
    void presented();
    bool tick(unsigned milliseconds);
};
uint64_t original_ui_frame_delay(uint64_t started, uint64_t now);
}    //namespace s2ne
