#pragma once
#include "original_skill94_formula.hpp"
namespace s2ne
{
// Sword94 arithmetic resolution only. Does not authorize targeting or mutate units.
class OriginalSkill94Resolution
{
    std::array<int, 256> config_{ };

public:
    struct DamageContext
    {
        unsigned difficulty{ 3 };
        bool friendly_army{ true }, conditions_mode{ };
        unsigned target_status3c{ }, target_status3d{ }, target_status39{ }, target_status3f{ };
        bool target_special13{ };
        unsigned profession_resistance{ }, profession_weakness{ };
    };
    OriginalSkill94Resolution() = default;
    static Result<OriginalSkill94Resolution> create(const std::filesystem::path& root)
    {
        auto resources = read_combat_resources(root);
        if (!resources)
        {
            return Result<OriginalSkill94Resolution>::failure(resources.error());
        }
        auto formula = OriginalSkill94Formula::create(root);
        if (!formula)
        {
            return Result<OriginalSkill94Resolution>::failure(formula.error());
        }
        auto& data = resources.value();
        OriginalSkill94Resolution result;
        for (unsigned i = 0; i < 256; ++i)
        {
            result.config_[i] = int(int8_t(data.bin[data.config + i]));
        }
        if (result.config_[5] + 3 <= 0 || result.config_[7] + 3 <= 0)
        {
            return Result<OriginalSkill94Resolution>::failure(make_error("Invalid sword94 random bound"));
        }
        return Result<OriginalSkill94Resolution>::success(std::move(result));
    }
    explicit OriginalSkill94Resolution(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    // 94 raw[29]==0: 46E2D6 applies neither terrain nor 440F56 family bonuses.
    Result<int> try_modified_damage(int raw, const DamageContext& ctx, original_combat::Random& rng, uint32_t seconds, bool randomized = true) const
    {
        if (ctx.difficulty < 1 || ctx.difficulty > 5 || ctx.profession_resistance > 65535 || ctx.profession_weakness > 65535)
        {
            return Result<int>::failure(make_error("Invalid sword94 modifier context"));
        }
        int damage = raw;
        if (ctx.profession_resistance & 64)
        {
            damage /= 10;
        }
        if (ctx.profession_weakness & 64)
        {
            damage *= 3;
        }
        // 94 group byte is10; only special13/3Fh4 and39h10/20 apply here.
        if ((ctx.target_status3f & 4) || ctx.target_special13)
        {
            damage = damage * 2 / 3;
        }
        if (ctx.target_status39 & 16)
        {
            damage = damage * 80 / 100;
        }
        if (ctx.target_status39 & 32)
        {
            damage = damage * 70 / 100;
        }
        int factor = config_[4] + (randomized ? rng.bounded(config_[5] + 3, seconds) : (config_[5] + 3) / 2);
        damage = damage * factor / 10;
        if (ctx.friendly_army && !ctx.conditions_mode)
        {
            damage = damage * (100 + config_[0xF3 + ctx.difficulty]) / 100;
        }
        damage = std::max(1, damage);
        if (ctx.target_status3c & 64)
        {
            damage = damage <= 180 ? 1 : damage - 180;
        }
        // 46E058 does this after 46E2D6, not before its random draw.
        if (ctx.target_status3d & 8)
        {
            damage = 0;
        }
        return Result<int>::success(damage);
    }
    int modified_damage(int raw, const DamageContext& ctx, original_combat::Random& rng, uint32_t seconds, bool randomized = true) const
    {
        auto result = try_modified_damage(raw, ctx, rng, seconds, randomized);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    struct HitInput
    {
        unsigned actor_proficiency{ }, target_proficiency{ };
        int actor_fencing{ }, target_fencing{ }, target_tactics{ };
        int actor_terrain_hit{ }, target_terrain_defense{ };
    };
    // 42C88F ->42C8D1 returns byte[3]: cost, hit%, defense%, not facing.
    static Result<int> try_dd11(const HitInput& in)
    {
        if (in.actor_fencing < -32768 || in.actor_fencing > 32767 || in.target_fencing < -32768 || in.target_fencing > 32767 || in.target_tactics < -32768 || in.target_tactics > 32767 || in.actor_proficiency > 65535 || in.target_proficiency > 65535 || in.actor_terrain_hit < -128 || in.actor_terrain_hit > 127 || in.target_terrain_defense < -128 || in.target_terrain_defense > 127)
        {
            return Result<int>::failure(make_error("Invalid sword94 terrain/proficiency"));
        }
        int64_t attack = (int64_t(in.actor_proficiency / 10) + 1000) * in.actor_fencing * (in.actor_terrain_hit + 100);
        int64_t defense = (int64_t(in.target_proficiency / 10) + 1000) * ((in.target_tactics + in.target_fencing) / 2) * (in.target_terrain_defense + 100);
        auto native = [](int64_t value)
        {
            return int32_t(uint32_t(value));
        };
        int difference = native(int64_t(native(attack)) - native(defense)) / 1000 / 100;
        return Result<int>::success(OriginalSkill94Formula::hit_curve(difference) * 50 / 100);
    }
    static int dd11(const HitInput& in)
    {
        auto result = try_dd11(in);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    struct Resolution
    {
        int chance{ }, damage{ }, roll{ };
        bool hit{ };
    };
    // effect1: 4654A6 ->466082 ->46618D ->471F51 ->46D431.
    // 471F51 caps damage to HP, promotes zero to1, THEN draws47328B.
    // Arithmetic packet only: no HP, rewards or payment; RNG commits on success.
    Result<Resolution> try_resolve(const HitInput& in, int raw, int target_hp, int elevation, const DamageContext& ctx, original_combat::Random& rng, uint32_t seconds) const
    {
        if (target_hp < 0 || target_hp > 32767 || ctx.difficulty < 1 || ctx.difficulty > 5 || ctx.profession_resistance > 65535 || ctx.profession_weakness > 65535)
        {
            return Result<Resolution>::failure(make_error("Invalid sword94 resolution context"));
        }
        auto staged = rng;
        auto base = try_dd11(in);
        if (!base)
        {
            return Result<Resolution>::failure(base.error());
        }
        auto hit = try_final_hit_from_dd11(base.value(), elevation, ctx.difficulty, ctx.friendly_army, ctx.conditions_mode, staged, seconds);
        if (!hit)
        {
            return Result<Resolution>::failure(hit.error());
        }
        int chance = hit.value();
        int damage = 0;
        if (chance)
        {
            auto value = try_modified_damage(raw, ctx, staged, seconds);
            if (!value)
            {
                return Result<Resolution>::failure(value.error());
            }
            damage = value.value();
        }
        damage = std::min(target_hp, damage);
        if (damage == 0)
        {
            damage = 1;
        }
        int roll = staged.percent(seconds);
        rng = staged;
        return Result<Resolution>::success({ chance, damage, roll, roll <= chance });
    }
    Resolution resolve(const HitInput& in, int raw, int target_hp, int elevation, const DamageContext& ctx, original_combat::Random& rng, uint32_t seconds) const
    {
        auto result = try_resolve(in, raw, target_hp, elevation, ctx, rng, seconds);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    // 46D496 flags21h bit2 calls46DD11, not46D874: caller must supply its result.
    Result<int> try_final_hit_from_dd11(int dd11, int elevation_difference, unsigned difficulty, bool friendly, bool conditions, original_combat::Random& rng, uint32_t seconds, bool randomized = true) const
    {
        if (difficulty < 1 || difficulty > 5 || config_[7] + 3 <= 0)
        {
            return Result<int>::failure(make_error("Invalid sword94 hit context"));
        }
        int discarded = config_[6] + (randomized ? rng.bounded(config_[7] + 3, seconds) : (config_[7] + 3) / 2);
        // Original407CD0 modifies accumulator0; its RNG is consumed even though
        // clamp makes that accumulator1 before the separate DD11 value is added.
        (void)discarded;
        (void)friendly;
        (void)conditions;
        int chance = 1 + dd11;
        if (difficulty != 1)
        {
            chance += chance * 20 * std::clamp(elevation_difference, -5, 5) / 100;
        }
        return Result<int>::success(std::clamp(chance, 1, 99));
    }
    int final_hit_from_dd11(int dd11, int elevation_difference, unsigned difficulty, bool friendly, bool conditions, original_combat::Random& rng, uint32_t seconds, bool randomized = true) const
    {
        auto result = try_final_hit_from_dd11(dd11, elevation_difference, difficulty, friendly, conditions, rng, seconds, randomized);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
};
}    //namespace s2ne
