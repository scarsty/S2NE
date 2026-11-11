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
            int d = actor.st - target.st - 100;
            int hit;
            if (d > 42) { hit = 99; }
            else if (d >= 10) { hit = (d - 10) / 4 + 90; }
            else if (d > 0) { hit = d + 80; }
            else if (d == 0) { hit = 80; }
            else if (d > -40) { hit = d / 2 + 80; }
            else if (d > -80) { hit = d + 100; }
            else if (d > -120) { hit = (d + 80) / 2 + 21; }
            else
            {
                hit = 1;
            }

            hit = hit * (config[10] + random.bounded(config[11] + 3, seconds)) / 10;
            if (!conditions_mode) { hit = hit * (100 + config[0xF3 + difficulty]) / 100; }
            chance = std::clamp(hit, 1, 99);
        }
        out.chance = chance;
        out.roll = random.percent(seconds);
        out.hit = out.roll <= chance;
        if (!out.hit) { return Result<Packet>::success(out); }
        if (!target.action_status_runtime)
        {
            target.action_status_runtime.emplace();
            auto& r = *target.action_status_runtime;
            r[0x39] = target.status39.value_or(0);
            r[0x3C] = target.status3c;
            r[0x3D] = target.status3d;
            r[0x3E] = target.status3e;
            r[0x3F] = target.status3f;
        }
        auto& r = *target.action_status_runtime;
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
