#pragma once
#include "native_hostile_status.h"
namespace s2ne
{
// 466D3A, 4670BF and 46847F: cleansing is not a hostile timed-status application.
struct NativeSpecialStatus
{
    static Result<NativeHostileStatus::Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions, unsigned id, unsigned effect, unsigned exclusions, int duration);
};
}    //namespace s2ne
