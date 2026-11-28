#pragma once
#include "hostile_status.h"
namespace s2ne
{
// 466D3A, 4670BF and 46847F: cleansing is not a hostile timed-status application.
struct SpecialStatus
{
    static Result<HostileStatus::Packet> resolve(Events::Unit& target, const Events::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions, unsigned id, unsigned effect, unsigned exclusions, int duration);
};
}    //namespace s2ne
