#pragma once
#include "native_effect55.hpp"
namespace s2ne
{
// 466544 -> 467625 -> 46803D: bounded zero-power hostile timed statuses.
struct NativeHostileStatus
{
    using Packet = NativeEffect55::Packet;
    static Result<Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        original_combat::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions_mode, unsigned effect, unsigned exclusions, int duration, int power = 0, int hit_base = 0, bool movement_excluded = false, bool bypass_immunity = false, bool preserve_sleep = false)
    {
        int statusMask;
        if (effect == 69) { statusMask = 0x40000; }
        else if (effect == 67) { statusMask = 0x80000; }
        else if (effect == 8) { statusMask = 0x1000; }
        else if (effect == 61) { statusMask = 0x20000; }
        else if (effect == 55) { statusMask = 0x10000; }
        else if (effect == 43) { statusMask = 0x8000; }
        else if (effect == 17) { statusMask = 0x4000; }
        else if (effect == 13) { statusMask = 0x2000; }
        else
        {
            statusMask = 0;
        }

        const uint32_t mask = statusMask;
        const bool stat = effect == 16 || effect == 46 || effect == 15 || effect == 5 || effect == 40 || effect == 58;
        const bool special = effect == 56 || effect == 47;
        if ((!mask && !stat && !special) || difficulty < 1 || difficulty > 5 || config[11] + 3 <= 0)
        {
            return Result<Packet>::failure(make_error("Hostile timed status runtime/config unavailable"));
        }
        Packet out;
        out.duration = duration;
        const bool excluded = movement_excluded || (target.runtime_role_selector == 1 && (exclusions & 2)) || (target.runtime_role_selector == 2 && (exclusions & 0x20)) || (effect == 16 && (target.abilities & 0x5000));
        if (!excluded)
        {
            const int d = actor.st - target.st + hit_base - 100;
            int chance;
            if (d > 42) { chance = 99; }
            else if (d >= 10) { chance = (d - 10) / 4 + 90; }
            else if (d > 0) { chance = d + 80; }
            else if (d == 0) { chance = 80; }
            else if (d > -40) { chance = d / 2 + 80; }
            else if (d > -80) { chance = d + 100; }
            else if (d > -120) { chance = (d + 80) / 2 + 21; }
            else
            {
                chance = 1;
            }

            chance = chance * (config[10] + random.bounded(config[11] + 3, seconds)) / 10;
            if (!conditions_mode) { chance = chance * (100 + config[0xF3 + difficulty]) / 100; }
            out.chance = std::clamp(chance, 1, 99);
        }
        if (effect == 56)
        {
            const unsigned status = target.action_status_runtime ? target.action_status_runtime->at(0x3E) : target.status3e;
            out.immune = (status & 0x10) || (target.abilities & 0x4000);
            if (!out.immune)
            {
                for (unsigned i = 0; i < 5; ++i)
                {
                    const int roll = random.percent(seconds);
                    out.roll = roll;
                    if (roll > out.chance) { continue; }
                    if (!target.action_status_runtime) { return Result<Packet>::failure(make_error("Composite status requires runtime")); }
                    auto& r = *target.action_status_runtime;
                    auto applied = Result<bool>::success(false);
                    if (i == 0) { applied = original_combat::try_apply_stat_status(r, 0x400, duration, -target.attack / 5, -target.defense / 5); }
                    else
                    {
                        unsigned status_mask = 0x1000;
                        if (i == 1) { status_mask = 0x10000; }
                        else if (i == 2) { status_mask = 0x4000; }
                        else if (i == 3) { status_mask = 0x8000; }
                        applied = original_combat::try_apply_timed_status(r, status_mask, duration);
                    }

                    if (!applied) { return Result<Packet>::failure(applied.error()); }
                    out.changed |= applied.value();
                }
            }
            out.hit = out.changed;
            if (!out.hit) { return Result<Packet>::success(out); }
        }
        else
        {
            out.roll = random.percent(seconds);
            out.hit = out.roll <= out.chance;
        }
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
        if (!preserve_sleep) { r[0x3E] &= uint8_t(~2u); }
        out.immune = !bypass_immunity && ((r[0x3E] & 0x10) || (target.abilities & 0x4000));
        if (!out.immune)
        {
            auto word = [&](size_t p)
            {
                return int(int16_t(unsigned(r[p]) | (unsigned(r[p + 1]) << 8)));
            };
            auto put = [&](size_t p, int v)
            {
                r[p] = uint8_t(v);
                r[p + 1] = uint8_t(unsigned(v) >> 8);
            };
            if (effect == 56 || effect == 47) { }
            else if (effect == 58)
            {
                out.changed = !(r[0x38] & 8);
                if (out.changed)
                {
                    const int delta = -word(0xC1) * power / 100;
                    r[0x38] |= 8;
                    put(0x52, delta);
                    put(0x134, word(0x134) + delta);
                }
                if (out.changed || (int8_t(r[0x51]) != -1 && int8_t(r[0x51]) < duration)) { r[0x51] = uint8_t(duration); }
            }
            else if (effect == 40)
            {
                auto applied = original_combat::try_apply_stat134_status(r, power, duration);
                if (!applied) { return Result<Packet>::failure(applied.error()); }
                out.changed = applied.value();
            }
            else if (effect == 16 || effect == 5)
            {
                auto applied = original_combat::try_apply_stat_status(r, effect == 5 ? 0x200 : 0x400, duration, -word(0x134) / (effect == 5 ? 2 : 5), -word(0x136) / (effect == 5 ? 2 : 5));
                if (!applied) { return Result<Packet>::failure(applied.error()); }
                out.changed = applied.value();
            }
            else if (effect == 46 || effect == 15)
            {
                const unsigned byte = effect == 46 ? 0x38 : 0x39, bit = effect == 46 ? 0x80 : 0x40;
                const unsigned timer = effect == 46 ? 0x5D : 0x7C;
                out.changed = !(r[byte] & bit);
                if (out.changed)
                {
                    r[byte] |= uint8_t(bit);
                    if (effect == 46)
                    {
                        power = std::min(power, word(0x12C));
                        put(0x5E, -power);
                        put(0x12C, word(0x12C) - power);
                        put(0x12E, std::max(0, word(0x12E) - power));
                        put(0x130, std::min(word(0x130), word(0x12E)));
                    }
                    else
                    {
                        put(0x7D, -power);
                        put(0x12C, word(0x12C) - power);
                        put(0x7F, -word(0x134) / 10);
                        put(0x134, word(0x134) + word(0x7F));
                        put(0x81, -word(0x136) / 10);
                        put(0x136, word(0x136) + word(0x81));
                    }
                }
                if (out.changed || (int8_t(r[timer]) != -1 && int8_t(r[timer]) < duration)) { r[timer] = uint8_t(duration); }
            }
            else
            {
                auto applied = original_combat::try_apply_timed_status(r, mask, duration);
                if (!applied) { return Result<Packet>::failure(applied.error()); }
                out.changed = applied.value();
            }
            if (stat || effect == 56)
            {
                target.attack = word(0x134);
                target.defense = word(0x136);
                target.max_action_points = word(0x12C);
                target.action_points = word(0x12E);
                target.carry_points = word(0x130);
                target.status39 = r[0x39];
            }
        }
        target.status3c = r[0x3C];
        target.status3d = r[0x3D];
        target.status3e = r[0x3E];
        target.status3f = r[0x3F];
        return Result<Packet>::success(out);
    }
};
}    //namespace s2ne
