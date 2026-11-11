#pragma once
#include <cstdint>
namespace s2ne
{
// 43A51B tests 19 centiseconds; 43A60B replaces A8 with the current time.
class OriginalIdleClock
{
    uint64_t elapsed_ = 0, stamp_ = 0, phase_ = 0;
    bool presented_ = false;

public:
    void presented() { presented_ = true; }
    void tick(unsigned milliseconds)
    {
        elapsed_ += milliseconds;
        if (presented_ && elapsed_ - stamp_ >= 190)
        {
            ++phase_;
            stamp_ = elapsed_;
            presented_ = false;
        }
    }
    uint64_t pose_time() const { return phase_ * 190; }
};
}    //namespace s2ne
