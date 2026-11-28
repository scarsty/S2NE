#pragma once
#include "native_effect55.h"
namespace s2ne
{
// ID002: 466544 ->467625 ->467FDE, not an inferred effect-name mapping.
struct NativeSleep2
{
    using Packet = NativeEffect55::Packet;
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode);
};
}    //namespace s2ne
