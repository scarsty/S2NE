#pragma once
#include "original_combat.hpp"
#include "original_events.hpp"
namespace s2ne
{
inline auto& ensure_original_status_runtime(OriginalEvents::Unit& target)
{
    if (!target.action_status_runtime)
    {
        target.action_status_runtime.emplace();
        auto& status = *target.action_status_runtime;
        status[0x39] = target.status39.value_or(0);
        status[0x3C] = target.status3c;
        status[0x3D] = target.status3d;
        status[0x3E] = target.status3e;
        status[0x3F] = target.status3f;
    }
    return *target.action_status_runtime;
}
inline int original_status_hit_chance(int d, original_combat::Random& random,
    std::span<const int, 256> config, unsigned difficulty, uint32_t seconds, bool conditions_mode)
{
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
    return std::clamp(hit, 1, 99);
}
// 4654A6 -> 466544 -> 467625: skill33 is curse, not effect1 or healing.
struct NativeEffect55
{
    struct Packet
    {
        int chance{}, roll{}, duration{};
        bool hit{}, changed{}, immune{};
    };
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        original_combat::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode)
    {
        if (difficulty < 1 || difficulty > 5 || config[11] + 3 <= 0)
        {
            return Result<Packet>::failure(make_error("Curse33 runtime/config unavailable"));
        }
        auto& r = ensure_original_status_runtime(target);
        Packet out;
        // 470505 reads WORD25 (210), not WORD23 (55): signed ST difference /10.
        out.duration = std::max(1, (actor.st - target.st) / 10);
        // 46D874 ->46DBA9, 407E2D, 46D719/46D848. Formula power is WORD21 (zero).
        out.chance = original_status_hit_chance(actor.st - target.st - 100, random, config, difficulty, seconds, conditions_mode);
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
