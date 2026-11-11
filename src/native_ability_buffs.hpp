#pragma once
#include "original_events.hpp"
namespace s2ne
{
struct NativeAbilityBuffs
{
    struct Oracle
    {
        unsigned id, group, bit, visual, color, tint;
        std::array<uint8_t, 24> tail;
        std::array<unsigned, 8> header;
    };
    static constexpr std::array<Oracle, 12> records{ { { 22, 2, 1, 22, 108, 35, { 10, 0, 10, 0, 4, 1, 2, 13, 15, 0, 100, 0, 4, 0, 0, 0, 0, 0, 1, 0, 1, 51, 15, 0 }, { 24, 0, 0, 147, 242, 4, 0, 4 } },
        { 23, 2, 2, 23, 139, 35, { 10, 0, 20, 0, 4, 1, 6, 14, 15, 0, 100, 0, 4, 0, 0, 0, 0, 0, 1, 0, 1, 48, 18, 0 }, { 24, 0, 0, 103, 242, 4, 0, 2 } },
        { 28, 2, 7, 28, 140, 45, { 32, 0, 160, 0, 3, 2, 10, 31, 10, 0, 100, 0, 3, 0, 0, 0, 0, 0, 1, 0, 1, 56, 9, 0 }, { 41, 100, 160, 200, 259, 0, 0, 5 } },
        { 42, 3, 9, 42, 233, 20, { 40, 0, 24, 1, 0, 1, 63, 25, 40, 0, 100, 0, 3, 0, 0, 0, 0, 0, 0, 0, 5, 32, 120, 0 }, { 22, 0, 0, 127, 230, 11, 36, 4 } },
        { 47, 4, 4, 47, 0, 0, { 25, 0, 56, 0, 3, 1, 33, 0, 0, 0, 100, 0, 3, 0, 0, 0, 0, 0, 1, 0, 1, 45, 25, 0 }, { 7, 81, 196, 163, 207, 0, 0, 3 } },
        { 57, 5, 4, 59, 233, 20, { 25, 0, 56, 0, 4, 1, 12, 0, 30, 0, 100, 0, 130, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 }, { 30, 0, 0, 100, 116, 0, 0, 5 } },
        { 76, 7, 1, 68, 233, 40, { 15, 0, 10, 0, 3, 2, 88, 0, 0, 0, 100, 0, 5, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 }, { 61, 65535, 65535, 640, 480, 10, 0, 5 } },
        { 78, 7, 3, 110, 233, 65, { 30, 0, 48, 0, 0, 7, 90, 0, 231, 3, 100, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 }, { 37, 0, 0, 80, 264, 24, 0, 5 } },
        { 79, 7, 4, 71, 222, 60, { 35, 0, 72, 0, 4, 1, 91, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 }, { 41, 65535, 65535, 640, 480, 3, 47, 5 } },
        { 84, 8, 1, 74, 233, 45, { 15, 0, 10, 0, 0, 1, 84, 0, 20, 0, 100, 0, 4, 0, 0, 0, 0, 0, 0, 0, 66, 46, 20, 0 }, { 31, 123, 182, 250, 250, 0, 0, 5 } },
        { 89, 8, 6, 79, 233, 50, { 15, 0, 128, 0, 0, 1, 10, 31, 10, 0, 100, 0, 4, 0, 0, 0, 0, 0, 0, 0, 66, 52, 14, 0 }, { 31, 141, 226, 281, 302, 5, 0, 5 } },
        { 91, 8, 8, 81, 233, 55, { 35, 0, 230, 0, 0, 1, 86, 0, 45, 0, 100, 0, 4, 0, 0, 0, 0, 0, 0, 0, 66, 43, 30, 0 }, { 24, 141, 175, 281, 285, 10, 0, 5 } } } };
    static constexpr const Oracle* find(unsigned id)
    {
        for (const auto& r : records)
        {
            if (r.id == id) { return &r; }
        }
        return nullptr;
    }
    static Result<bool> apply(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor, unsigned effect, int power, int duration, std::span<const int, 256> config)
    {
        // Only the native 4668F0 increment / 442E96 contracts. Invisibility,
        // cooperative effects and reactivation require their own scene adapters.
        if (effect != 2 && effect != 6 && effect != 10 && effect != 12 && effect != 63 && effect != 84 && effect != 86)
        {
            return Result<bool>::failure(make_error("Unsupported native stat effect contract"));
        }
        if (!target.action_status_runtime) { return Result<bool>::failure(make_error("Ability status requires runtime")); }
        auto& r = *target.action_status_runtime;
        auto word = [&](size_t p)
        {
            return int(int16_t(unsigned(r[p]) | (unsigned(r[p + 1]) << 8)));
        };
        auto put = [&](size_t p, int v)
        {
            r[p] = uint8_t(v);
            r[p + 1] = uint8_t(unsigned(v) >> 8);
        };
        put(0x134, target.attack);
        put(0x136, target.defense);
        put(0x12A, target.sp);
        put(0x12C, target.max_action_points);
        put(0x12E, target.action_points);
        put(0x130, target.carry_points);
        bool changed = false;
        if (effect == 33)
        {
            auto v = original_combat::try_apply_timed_status(r, 0x10, duration);
            if (!v) { return v; }
            changed = v.value();
        }
        else if (effect == 63 || effect == 84 || effect == 86)
        {
            unsigned mask = 0x2000;
            if (effect == 63) { mask = 0x800; }
            else if (effect == 84) { mask = 0x1000; }
            auto v = original_combat::try_apply_stat_status(r, mask,
                duration, effect == 63 ? target.attack / 2 : power, effect == 63 ? target.defense / 2 : power);
            if (!v) { return v; }
            changed = v.value();
        }
        else if (effect == 91)
        {
            if (target.id != actor.id || target.appearance != actor.appearance)
            {
                if (target.action_state == 2 && word(8) == 1)
                {
                    r[0x2D] = 0;
                    std::fill(r.begin() + 0x34, r.begin() + 0x38, 0);
                    int carry = target.action_points > config[0xED] ? config[0xF0] : target.action_points;
                    if (target.sp + target.action_points >= 110) { carry = 0; }
                    target.action_state = 0;
                    target.status34 = 0;
                    target.carry_points = carry;
                    target.action_points = int16_t(target.max_action_points + carry);
                    put(0x12E, target.action_points);
                    put(0x130, carry);
                    changed = true;
                }
            }
        }
        else
        {
            unsigned mask = 0x100, slot = 8;
            size_t destination = 0x138;
            switch (effect)
            {
            case 2:
                mask = 0x10;
                slot = 4;
                destination = 0x136;
                break;
            case 6:
                mask = 4;
                slot = 2;
                destination = 0x134;
                break;
            case 10:
                mask = 0x40;
                slot = 6;
                destination = 0x12C;
                break;
            default: break;
            }
            if (effect == 10) { power = std::min(power, 110 - target.max_action_points); }
            if (effect != 10 || power > 0)
            {
                changed = !(r[0x38 + slot / 8] & (mask >> (slot / 8 * 8)));
                if (changed)
                {
                    r[0x38 + slot / 8] |= uint8_t(mask >> (slot / 8 * 8));
                    put(destination, word(destination) + power);
                    put(0x49 + slot * 3, power);
                    if (effect == 10) { put(0x12E, word(0x12E) + power); }
                }
                auto& timer = r[0x48 + slot * 3];
                if (changed || (int8_t(timer) != -1 && int8_t(timer) < duration)) { timer = uint8_t(duration); }
            }
            if (effect == 88)
            {
                for (unsigned bit : { 4u, 0x200u })
                {
                    auto v = original_combat::try_apply_timed_status(r, bit, duration);
                    if (!v) { return v; }
                    changed |= v.value();
                }
            }
        }
        target.attack = word(0x134);
        target.defense = word(0x136);
        target.sp = word(0x12A);
        target.max_action_points = word(0x12C);
        target.action_points = word(0x12E);
        target.carry_points = word(0x130);
        target.tactics = word(0x138);
        target.status39 = r[0x39];
        target.status3c = r[0x3C];
        target.status3d = r[0x3D];
        target.status3e = r[0x3E];
        target.status3f = r[0x3F];
        return Result<bool>::success(changed);
    }
};
}    //namespace s2ne
