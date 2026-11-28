#pragma once
#include "damage_formula.h"
#include "events.h"
namespace s2ne
{
// Verified arithmetic subchain only; intentionally has no state/payment API.
class Skill94Formula
{
    int numerator_{}, denominator_{};

public:
    static int hit_curve(int difference);
    // 46D874 effect1: both proficiency reads use the actor, not the target.
    static Result<int> try_base_hit(unsigned actor_proficiency, int actor_sp, int target_sp, int power = 50);
    static int base_hit(unsigned actor_proficiency, int actor_sp, int target_sp, int power = 50);
    // 4704AC, called by 46F89E case44. This is BEFORE 46E2D6 modifiers.
    static Result<int> try_raw_formula44(int actor_fencing, unsigned proficiency, int target_fencing, int target_defense, int numerator, int denominator, int power = 50);
    static int raw_formula44(int actor_fencing, unsigned proficiency, int target_fencing, int target_defense, int numerator, int denominator, int power = 50);
    Skill94Formula() = default;
    static Result<Skill94Formula> create(const std::filesystem::path& root);
    explicit Skill94Formula(const std::filesystem::path& root);
    Result<int> try_raw_damage(const Events::Unit& actor, const Events::Unit& target) const;
    int raw_damage(const Events::Unit& actor, const Events::Unit& target) const;
    int numerator() const;
    int denominator() const;
};
}    //namespace s2ne
