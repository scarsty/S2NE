#pragma once
#include "runtime_error.h"
#include <bit>
#include <cstdint>
#include <functional>

namespace s2ne
{
// Single original CRT thread stream plus the shared 506140 wrapper counter.
// Time is provided explicitly: speculative copies must not read wall-clock time.
class OriginalRandom
{
public:
    struct State
    {
        uint32_t seed = 1;
        uint32_t calls = 0;
        bool operator==(const State&) const = default;
    };
    using TimeSource = std::function<uint32_t()>;

private:
    State state_;

public:
    explicit OriginalRandom(State state);
    State state() const;
    void restore(State state);
    void srand(uint32_t seed);
    unsigned rand();
    // 4732ED: signed IDIV bound, zero returns without counter/seed/time changes.
    Result<unsigned> try_bounded(int32_t bound, const TimeSource& time = {});
    unsigned bounded(int32_t bound, const TimeSource& time = {});
};
}    //namespace s2ne
