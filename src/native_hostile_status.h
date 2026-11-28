#pragma once
#include "native_effect55.h"
namespace s2ne
{
// 466544 -> 467625 -> 46803D: bounded zero-power hostile timed statuses.
struct NativeHostileStatus
{
    using Packet = NativeEffect55::Packet;
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode, unsigned effect, unsigned exclusions, int duration, int power = 0, int hit_base = 0, bool movement_excluded = false, bool bypass_immunity = false, bool preserve_sleep = false);
};
}    //namespace s2ne
