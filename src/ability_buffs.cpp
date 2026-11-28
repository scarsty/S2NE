#include "ability_buffs.h"

namespace s2ne
{
auto AbilityBuffs::apply(Events::Unit& target, unsigned effect, int power, int duration) -> Result<bool>
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
    put(0x138, target.tactics);
    bool changed = false;
    if (effect == 63 || effect == 84 || effect == 86)
    {
        unsigned mask = 0x2000;
        if (effect == 63) { mask = 0x800; }
        else if (effect == 84) { mask = 0x1000; }
        auto v = s2ne::try_apply_stat_status(r, mask,
            duration, effect == 63 ? target.attack / 2 : power, effect == 63 ? target.defense / 2 : power);
        if (!v) { return v; }
        changed = v.value();
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
}    //namespace s2ne
