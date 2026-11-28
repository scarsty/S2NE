#include "original_random.h"

namespace s2ne
{
OriginalRandom::OriginalRandom(State state) : state_(state) {}

auto OriginalRandom::state() const -> State { return state_; }

auto OriginalRandom::restore(State state) -> void { state_ = state; }

auto OriginalRandom::srand(uint32_t seed) -> void { state_.seed = seed; }

auto OriginalRandom::rand() -> unsigned
{
    state_.seed = state_.seed * uint32_t(0x41C64E6D) + uint32_t(0x3039);
    return (state_.seed >> 16) & 0x7FFF;
}

auto OriginalRandom::try_bounded(int32_t bound, const TimeSource& time) -> Result<unsigned>
{
    if (!bound)
    {
        return Result<unsigned>::success(0);
    }
    State next = state_;
    ++next.calls;
    if (std::bit_cast<int32_t>(next.calls) >= 200)
    {
        if (!time)
        {
            return Result<unsigned>::failure(make_error("Original RNG reseed requires explicit time source"));
        }
        next.seed = time();
        next.calls = 0;
    }
    next.seed = next.seed * uint32_t(0x41C64E6D) + uint32_t(0x3039);
    unsigned value = ((next.seed >> 16) & 0x7FFF) % uint32_t(bound < 0 ? -int64_t(bound) : int64_t(bound));
    state_ = next;
    return Result<unsigned>::success(value);
}

auto OriginalRandom::bounded(int32_t bound, const TimeSource& time) -> unsigned
{
    auto result = try_bounded(bound, time);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}
}    //namespace s2ne
