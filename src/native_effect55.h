#pragma once
#include "original_combat.h"
#include "original_events.h"
namespace s2ne
{
std::array<unsigned char, 314>& ensure_original_status_runtime(OriginalEvents::Unit& target);
int original_status_hit_chance(int d, s2ne::Random& random,
    std::span<const int, 256> config, unsigned difficulty, uint32_t seconds, bool conditions_mode);
// 4654A6 -> 466544 -> 467625: skill33 is curse, not effect1 or healing.
struct NativeEffect55
{
    struct Packet
    {
        int chance{}, roll{}, duration{};
        bool hit{}, changed{}, immune{};
    };
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        s2ne::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode);
};
}    // namespace s2ne
