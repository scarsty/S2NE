#pragma once
#include "effect55.h"
namespace s2ne
{
// ID002: 466544 ->467625 ->467FDE, not an inferred effect-name mapping.
struct Sleep2
{
    using Packet = Effect55::Packet;
    static Result<Packet> resolve(Events::Unit& target, const Events::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode);
};
}    //namespace s2ne
