#pragma once
#include "original_combat.hpp"
#include "original_events.hpp"
namespace s2ne
{
// 4654A6 -> 466544 -> 467625: skill33 is curse, not effect1 or healing.
struct NativeEffect55
{
    struct Packet
    {
        int chance{ }, roll{ }, duration{ };
        bool hit{ }, changed{ }, immune{ };
    };
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        original_combat::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode)
    {
        if (difficulty < 1 || difficulty > 5 || config[11] + 3 <= 0)
        {
            return Result<Packet>::failure(make_error("Curse33 runtime/config unavailable"));
        }
        if (!target.action_status_runtime)
        {
            // Stage records have no runtime status/timers; preserve the live cached bytes.
            target.action_status_runtime.emplace();
            auto& status = *target.action_status_runtime;
            status[0x39] = target.status39.value_or(0);
            status[0x3C] = target.status3c;
            status[0x3D] = target.status3d;
            status[0x3E] = target.status3e;
            status[0x3F] = target.status3f;
        }
        auto& r = *target.action_status_runtime;
        Packet out;
        // 470505 reads WORD25 (210), not WORD23 (55): signed ST difference /10.
        out.duration = std::max(1, (actor.st - target.st) / 10);
        // 46D874 ->46DBA9, 407E2D, 46D719/46D848. Formula power is WORD21 (zero).
        int d = actor.st - target.st - 100;
        int hit = d > 42 ? 99 : d >= 10 ? (d - 10) / 4 + 90 :
            d > 0                       ? d + 80 :
            d == 0                      ? 80 :
            d > -40                     ? d / 2 + 80 :
            d > -80                     ? d + 100 :
            d > -120                    ? (d + 80) / 2 + 21 :
                                          1;
        hit = hit * (config[10] + random.bounded(config[11] + 3, seconds)) / 10;
        if (!conditions_mode) { hit = hit * (100 + config[0xF3 + difficulty]) / 100; }
        out.chance = std::clamp(hit, 1, 99);
        out.roll = random.percent(seconds);
        out.hit = out.roll <= out.chance;
        // 467804 clears sleep before the status immunity branch 467A42.
        if (out.hit)
        {
            r[0x3E] &= uint8_t(~2u);
            target.status3e = r[0x3E];
            out.immune = (r[0x3E] & 0x10) || (target.abilities & 0x4000);
            if (!out.immune)
            {
                // 467FC0 ->442CD5(mask10000h), timer85h+bit16 =95h.
                auto applied = original_combat::try_apply_timed_status(r, 0x10000, out.duration);
                if (!applied) { return Result<Packet>::failure(applied.error()); }
                out.changed = applied.value();
                target.status3e = r[0x3E];
            }
        }
        return Result<Packet>::success(out);
    }
};
}    // namespace s2ne
