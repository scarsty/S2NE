#pragma once
#include "native_effect55.hpp"
namespace s2ne
{
// ID002: 466544 ->467625 ->467FDE, not an inferred effect-name mapping.
struct NativeSleep2
{
    using Packet = NativeEffect55::Packet;
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        original_combat::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode)
    {
        if (difficulty < 1 || difficulty > 5 || config[11] + 3 <= 0)
        {
            return Result<Packet>::failure(make_error("Sleep2 runtime/config unavailable"));
        }
        Packet out;
        // WORD25=2; the opposing army (DA=1) adds one at46767E.
        out.duration = 2;
        int chance = 0;
        // 46D50F: record29 bit2 excludes runtime role1 before any power RNG.
        if (target.runtime_role_selector != 1)
        {
            chance = original_status_hit_chance(actor.st - target.st - 100, random, config, difficulty, seconds, conditions_mode);
        }
        out.chance = chance;
        out.roll = random.percent(seconds);
        out.hit = out.roll <= chance;
        if (!out.hit) { return Result<Packet>::success(out); }
        auto& r = ensure_original_status_runtime(target);
        // 4685CD wakes first, even an immune target; stale timer is retained.
        r[0x3E] &= uint8_t(~2u);
        out.immune = (r[0x3E] & 0x10) || (target.abilities & 0x4000);
        if (!out.immune)
        {
            auto applied = original_combat::try_apply_timed_status(r, 0x20000, out.duration);
            if (!applied) { return Result<Packet>::failure(applied.error()); }
            out.changed = applied.value();
        }
        target.status3e = r[0x3E];
        return Result<Packet>::success(out);
    }
};
}    //namespace s2ne
