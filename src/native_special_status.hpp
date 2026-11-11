#pragma once
#include "native_hostile_status.hpp"
namespace s2ne
{
// 466D3A, 4670BF and 46847F: cleansing is not a hostile timed-status application.
struct NativeSpecialStatus
{
    static Result<NativeHostileStatus::Packet> resolve(OriginalEvents::Unit& target, const OriginalEvents::Unit& actor,
        original_combat::Random& random, std::span<const int, 256> config, unsigned difficulty,
        uint32_t seconds, bool conditions, unsigned id, unsigned effect, unsigned exclusions, int duration)
    {
        if (!target.action_status_runtime) { return Result<NativeHostileStatus::Packet>::failure(make_error("Special status requires runtime")); }
        auto& r = *target.action_status_runtime;
        auto put = [&](size_t p, int v)
        {
            r[p] = uint8_t(v);
            r[p + 1] = uint8_t(unsigned(v) >> 8);
        };
        put(0x134, target.attack);
        put(0x136, target.defense);
        auto result = NativeHostileStatus::resolve(target, actor, random, config, difficulty, seconds, conditions,
            id == 8 || id == 56 ? 47 : effect, exclusions, duration, id == 39 ? 20 : 0, id == 39 ? 20 : 0, false, id == 8 || id == 48 || id == 56, id == 8 || id == 56);
        if (!result) { return result; }
        auto& p = result.value();
        if (p.hit && (id == 8 || id == 48 || id == 56))
        {
            if (id == 56)
            {
                std::fill(r.begin() + 0x38, r.begin() + 0x40, 0);
            }
            else if (id == 8 || actor.hostile() == target.hostile())
            {
                r[0x38] &= 0x74;
                r[0x39] &= 0xBB;
                r[0x3D] &= 0x0F;
                r[0x3E] &= 0xF0;
                r[0x3F] &= 0xFE;
            }
            else
            {
                auto removed = original_combat::try_remove_one_status(r);
                if (!removed) { return Result<NativeHostileStatus::Packet>::failure(removed.error()); }
            }
            auto restored = original_combat::try_recalculate_status_stats(r);
            if (!restored) { return Result<NativeHostileStatus::Packet>::failure(restored.error()); }
            p.changed = true;
        }
        auto word = [&](size_t o)
        {
            return int(int16_t(unsigned(r[o]) | (unsigned(r[o + 1]) << 8)));
        };
        target.attack = word(0x134);
        target.defense = word(0x136);
        target.sp = word(0x12A);
        target.max_action_points = word(0x12C);
        target.status39 = r[0x39];
        target.status3c = r[0x3C];
        target.status3d = r[0x3D];
        target.status3e = r[0x3E];
        target.status3f = r[0x3F];
        return result;
    }
};
}    //namespace s2ne
