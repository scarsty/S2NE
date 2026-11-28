#pragma once
#include "skill94_formula.h"
namespace s2ne
{
// Sword94 arithmetic resolution only. Does not authorize targeting or mutate units.
class Skill94Resolution
{
    std::array<int, 256> config_{};

public:
    struct DamageContext
    {
        unsigned difficulty{ 3 };
        bool friendly_army{ true }, conditions_mode{};
        unsigned target_status3c{}, target_status3d{}, target_status39{}, target_status3f{};
        bool target_special13{};
        unsigned profession_resistance{}, profession_weakness{};
        unsigned damage_mask{ 64 }, original_group{ 10 }, formula_flags{ 194 };
        bool bypass_modifiers{}, target_magic_guard{}, target_spell_armor{}, actor_spell_boost{}, target_equipment11{};
        unsigned terrain_flags{}, target_movement_class{}, target_immunity2f{};
        unsigned attack_mode{ 1 };
        bool target_direction_guard{};
        unsigned effect{ 1 };
    };
    Skill94Resolution() = default;
    static Result<Skill94Resolution> create(const std::filesystem::path& root);
    explicit Skill94Resolution(const std::filesystem::path& root);
    // 94 raw[29]==0: 46E2D6 applies neither terrain nor 440F56 family bonuses.
    Result<int> try_modified_damage(int raw, const DamageContext& ctx, s2ne::Random& rng, uint32_t seconds, bool randomized = true) const;
    int modified_damage(int raw, const DamageContext& ctx, s2ne::Random& rng, uint32_t seconds, bool randomized = true) const;
    struct HitInput
    {
        unsigned actor_proficiency{}, target_proficiency{};
        int actor_fencing{}, target_fencing{}, target_tactics{};
        int actor_terrain_hit{}, target_terrain_defense{};
        int actor_st{}, target_st{};
    };
    // 42C88F ->42C8D1 returns byte[3]: cost, hit%, defense%, not facing.
    static Result<int> try_dd11(const HitInput& in, int power = 50);
    static int dd11(const HitInput& in);
    struct Resolution
    {
        int chance{}, damage{}, roll{};
        bool hit{};
    };
    // 018/040: 466544 ->467625 uses the generic ST branch46DBA9,
    // not effect1's proficiency-weighted46D999 or471F51 zero promotion.
    Result<Resolution> try_resolve_drain(int actor_st, int target_st, int raw, int target_hp,
        const DamageContext& ctx, s2ne::Random& random, uint32_t seconds, int hit_power, bool randomized = true) const;
    // effect1: 4654A6 ->466082 ->46618D ->471F51 ->46D431.
    // 471F51 caps damage to HP, promotes zero to1, THEN draws47328B.
    // Arithmetic packet only: no HP, rewards or payment; RNG commits on success.
    Result<Resolution> try_resolve(const HitInput& in, int raw, int target_hp, int elevation, const DamageContext& ctx, s2ne::Random& rng, uint32_t seconds, int hit_power = 50, bool guaranteed_hit = false, bool elevation_enabled = true, unsigned hit_flags = 2, bool randomized = true) const;
    Resolution resolve(const HitInput& in, int raw, int target_hp, int elevation, const DamageContext& ctx, s2ne::Random& rng, uint32_t seconds) const;
    // 46D496 flags21h bit2 calls46DD11, not46D874: caller must supply its result.
    Result<int> try_final_hit_from_dd11(int dd11, int elevation_difference, unsigned difficulty, bool friendly, bool conditions, s2ne::Random& rng, uint32_t seconds, bool randomized = true) const;
    int final_hit_from_dd11(int dd11, int elevation_difference, unsigned difficulty, bool friendly, bool conditions, s2ne::Random& rng, uint32_t seconds, bool randomized = true) const;
};
}    //namespace s2ne
