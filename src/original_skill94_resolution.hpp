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
        unsigned damage_mask{ 64 }, original_group{ 10 }, formula_flags{ 194 };
        bool bypass_modifiers{ }, target_magic_guard{ }, target_spell_armor{ }, actor_spell_boost{ }, target_equipment11{ };
        unsigned terrain_flags{ }, target_movement_class{ }, target_immunity2f{ };
        unsigned attack_mode{ 1 };
        bool target_direction_guard{ };
        unsigned effect{ 1 };
    };
    OriginalSkill94Resolution() = default;
    static Result<OriginalSkill94Resolution> create(const std::filesystem::path& root)
    {
        auto formula = OriginalSkill94Formula::create(root);
        if (!formula)
        {
            return Result<OriginalSkill94Resolution>::failure(formula.error());
        }
        auto resources = read_combat_resources(root);
        if (!resources)
        {
            return Result<OriginalSkill94Resolution>::failure(resources.error());
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
        if (ctx.difficulty < 1 || ctx.difficulty > 5 || ctx.profession_resistance > 65535 || ctx.profession_weakness > 65535 || ctx.target_movement_class > 5 || ctx.target_immunity2f > 255)
        {
            return Result<int>::failure(make_error("Invalid sword94 modifier context"));
        }
        int damage = raw;
        // 46E13D: effect41 goes directly through 407EE5, not 46E2D6.
        if (ctx.effect == 41)
        {
            const int factor = config_[4] + (randomized ? rng.bounded(config_[5] + 3, seconds) : (config_[5] + 3) / 2);
            damage = damage * factor / 10;
            if (ctx.friendly_army && !ctx.conditions_mode)
            {
                damage = damage * (100 + config_[0xF3 + ctx.difficulty]) / 100;
            }
            return Result<int>::success(std::max(1, damage));
        }
        if (ctx.bypass_modifiers) { return Result<int>::success((ctx.target_status3d & 8) ? 0 : raw); }
        if ((ctx.formula_flags & 0x70) && ctx.difficulty != 1 && !ctx.target_direction_guard)
        {
            int index = 2;
            if (ctx.formula_flags & 0x10) { index = 0; }
            else if (ctx.formula_flags & 0x20) { index = 1; }
            if (ctx.attack_mode == 2) { damage += damage * (20 + index * 5) / 100; }
            else if (ctx.attack_mode == 3) { damage += damage * (40 + index * 5) / 100; }
        }
        if (ctx.original_group == 5 && (ctx.target_status3d & 1)) { damage /= 2; }
        if (ctx.profession_resistance & ctx.damage_mask)
        {
            damage /= 10;
        }
        if (ctx.profession_weakness & ctx.damage_mask)
        {
            damage *= 3;
        }
        // 46E521..46E5DC: independent tests, in this order, before ability reductions.
        if (ctx.target_movement_class == 0 && (ctx.terrain_flags & 4)) { damage += damage / 5; }
        if ((ctx.target_movement_class == 3 || ctx.target_movement_class == 4) && (ctx.terrain_flags & 8)) { damage += damage / 5; }
        if ((ctx.target_movement_class == 3 || ctx.target_movement_class == 4) && (ctx.terrain_flags & 16)) { damage /= 10; }
        if (ctx.target_magic_guard && (ctx.formula_flags & 1)) { damage /= 2; }
        if (ctx.actor_spell_boost && (ctx.formula_flags & 1)) { damage = damage * 125 / 100; }
        if (ctx.target_spell_armor && (ctx.original_group == 2 || ctx.original_group == 4)) { damage /= 2; }
        if (ctx.target_equipment11 && ctx.original_group >= 3 && ctx.original_group <= 6) { damage /= 2; }
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
        int actor_st{ }, target_st{ };
    };
    // 42C88F ->42C8D1 returns byte[3]: cost, hit%, defense%, not facing.
    static Result<int> try_dd11(const HitInput& in, int power = 50)
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
        return Result<int>::success(OriginalSkill94Formula::hit_curve(difference) * power / 100);
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
    // 018/040: 466544 ->467625 uses the generic ST branch46DBA9,
    // not effect1's proficiency-weighted46D999 or471F51 zero promotion.
    Result<Resolution> try_resolve_drain(int actor_st, int target_st, int raw, int target_hp,
        const DamageContext& ctx, original_combat::Random& random, uint32_t seconds, int hit_power) const
    {
        if ((ctx.effect != 41 && ctx.effect != 42) || target_hp <= 0 || target_hp > 32767
            || actor_st < -32768 || actor_st > 32767 || target_st < -32768 || target_st > 32767
            || ctx.difficulty < 1 || ctx.difficulty > 5 || config_[11] + 3 <= 0)
        {
            return Result<Resolution>::failure(make_error("Invalid467625 drain arithmetic context"));
        }
        auto staged = random;
        const bool excluded = ((ctx.terrain_flags & 1) && ctx.target_movement_class != 0)
            || ((ctx.terrain_flags & 2) && ctx.target_immunity2f == 1)
            || ((ctx.terrain_flags & 32) && ctx.target_immunity2f == 2);
        int chance = 0, damage = 0;
        if (!excluded)
        {
            chance = OriginalSkill94Formula::hit_curve(actor_st - target_st + hit_power - 100);
            chance = chance * (config_[10] + staged.bounded(config_[11] + 3, seconds)) / 10;
            if (ctx.friendly_army && !ctx.conditions_mode) { chance = chance * (100 + config_[0xF3 + ctx.difficulty]) / 100; }
            chance = std::clamp(chance, 1, 99);
            auto modified = try_modified_damage(raw, ctx, staged, seconds);
            if (!modified) { return Result<Resolution>::failure(modified.error()); }
            damage = std::clamp(modified.value(), 0, target_hp);
        }
        const int roll = staged.percent(seconds);
        random = staged;
        return Result<Resolution>::success({ chance, damage, roll, roll <= chance });
    }
    // effect1: 4654A6 ->466082 ->46618D ->471F51 ->46D431.
    // 471F51 caps damage to HP, promotes zero to1, THEN draws47328B.
    // Arithmetic packet only: no HP, rewards or payment; RNG commits on success.
    Result<Resolution> try_resolve(const HitInput& in, int raw, int target_hp, int elevation, const DamageContext& ctx, original_combat::Random& rng, uint32_t seconds, int hit_power = 50, bool guaranteed_hit = false, bool elevation_enabled = true, unsigned hit_flags = 2) const
    {
        if (target_hp < 0 || target_hp > 32767 || ctx.difficulty < 1 || ctx.difficulty > 5 || ctx.profession_resistance > 65535 || ctx.profession_weakness > 65535 || ctx.target_movement_class > 5 || ctx.target_immunity2f > 255)
        {
            return Result<Resolution>::failure(make_error("Invalid sword94 resolution context"));
        }
        auto staged = rng;
        auto base = try_dd11(in, hit_power);
        if (!base)
        {
            return Result<Resolution>::failure(base.error());
        }
        auto hit = Result<int>::success(99);
        const bool immune = ((ctx.terrain_flags & 1) && ctx.target_movement_class != 0) || ((ctx.terrain_flags & 2) && ctx.target_immunity2f == 1) || ((ctx.terrain_flags & 32) && ctx.target_immunity2f == 2);
        if (immune) { hit = Result<int>::success(0); }
        else if (!guaranteed_hit)
        {
            if (hit_flags == 2) { hit = try_final_hit_from_dd11(base.value(), elevation_enabled ? elevation : 0, ctx.difficulty, ctx.friendly_army, ctx.conditions_mode, staged, seconds); }
            else if (hit_flags == 1 || hit_flags == 3 || hit_flags == 5)
            {
                const int proficiency = std::max(10u, in.actor_proficiency / 10);
                const int difference = ((proficiency + 500) * std::clamp(in.actor_st, 10, 99) - (proficiency + 500) * std::clamp(in.target_st, 10, 99)) / 500;
                int curve;
                if (difference > 42) { curve = 99; }
                else if (difference >= 10) { curve = (difference - 10) / 4 + 90; }
                else if (difference > 0) { curve = difference + 80; }
                else if (difference == 0) { curve = 80; }
                else if (difference > -40) { curve = difference / 2 + 80; }
                else if (difference > -80) { curve = difference + 100; }
                else if (difference > -120) { curve = (difference + 80) / 2 + 21; }
                else
                {
                    curve = 1;
                }
                const int factor = config_[6] + staged.bounded(config_[7] + 3, seconds);
                int accumulator = hit_flags == 5 ? hit_power : 0;
                accumulator = accumulator * factor / 10;
                if (ctx.friendly_army && !ctx.conditions_mode) { accumulator = accumulator * (100 + config_[0xF3 + ctx.difficulty]) / 100; }
                accumulator = std::clamp(accumulator, 1, 99);
                int chance = accumulator + curve * hit_power / 100;
                if (hit_flags == 3)
                {
                    // 46D748 randomizes the accumulated ST result before adding DD11.
                    const int second_factor = config_[6] + staged.bounded(config_[7] + 3, seconds);
                    chance = chance * second_factor / 10;
                    if (ctx.friendly_army && !ctx.conditions_mode) { chance = chance * (100 + config_[0xF3 + ctx.difficulty]) / 100; }
                    chance = std::clamp(chance, 1, 99);
                    chance = (chance + base.value()) / 2;
                }
                if (hit_flags == 5) { chance /= 2; }
                if (elevation_enabled && ctx.difficulty != 1) { chance += chance * 20 * std::clamp(elevation, -5, 5) / 100; }
                hit = Result<int>::success(std::clamp(chance, 1, 99));
            }
            else
            {
                return Result<Resolution>::failure(make_error("Unsupported original damage hit source"));
            }
        }
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
        if (damage == 0 && ctx.effect != 41 && ctx.effect != 42)
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
