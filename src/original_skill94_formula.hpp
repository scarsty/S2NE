#pragma once
#include "native_damage_formula.hpp"
#include "original_events.hpp"
namespace s2ne
{
// Verified arithmetic subchain only; intentionally has no state/payment API.
class OriginalSkill94Formula
{
    int numerator_{}, denominator_{};

public:
    static int hit_curve(int difference)
    {
        if (difference > 42)
        {
            return 99;
        }
        if (difference >= 10)
        {
            return (difference - 10) / 4 + 90;
        }
        if (difference > 0)
        {
            return difference + 80;
        }
        if (difference == 0)
        {
            return 80;
        }
        if (difference > -40)
        {
            return difference / 2 + 80;
        }
        if (difference > -80)
        {
            return difference + 100;
        }
        if (difference > -120)
        {
            return (difference + 80) / 2 + 21;
        }
        return 1;
    }
    // 46D874 effect1: both proficiency reads use the actor, not the target.
    static Result<int> try_base_hit(unsigned actor_proficiency, int actor_sp, int target_sp, int power = 50)
    {
        if (actor_proficiency > 65535 || actor_sp < -32768 || actor_sp > 32767 || target_sp < -32768 || target_sp > 32767 || power != 50)
        {
            return Result<int>::failure(make_error("Unsupported sword94 hit input"));
        }
        int proficiency = std::max(10, int(actor_proficiency / 10));
        int difference = ((proficiency + 500) * std::clamp(actor_sp, 10, 99) - (proficiency + 500) * std::clamp(target_sp, 10, 99)) / 500;
        return Result<int>::success(hit_curve(difference) * power / 100);
    }
    static int base_hit(unsigned actor_proficiency, int actor_sp, int target_sp, int power = 50)
    {
        auto result = try_base_hit(actor_proficiency, actor_sp, target_sp, power);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    // 4704AC, called by 46F89E case44. This is BEFORE 46E2D6 modifiers.
    static Result<int> try_raw_formula44(int actor_fencing, unsigned proficiency, int target_fencing, int target_defense, int numerator, int denominator, int power = 50)
    {
        if (proficiency > 65535 || denominator <= 0 || power != 50)
        {
            return Result<int>::failure(make_error("Unsupported sword94 formula input"));
        }
        std::array<int, 256> config{};
        config[0x98] = numerator;
        config[0x99] = denominator;
        NativeDamageFormula::Input input;
        input.actor_value = std::max(1, actor_fencing);
        input.target_value = std::max(1, target_fencing);
        input.proficiency = int(proficiency);
        input.target_defense = target_defense;
        return NativeDamageFormula::evaluate(44, power, input, config);
    }
    static int raw_formula44(int actor_fencing, unsigned proficiency, int target_fencing, int target_defense, int numerator, int denominator, int power = 50)
    {
        auto result = try_raw_formula44(actor_fencing, proficiency, target_fencing, target_defense, numerator, denominator, power);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    OriginalSkill94Formula() = default;
    static Result<OriginalSkill94Formula> create(const std::filesystem::path& root)
    {
        auto resources = read_combat_resources(root);
        if (!resources)
        {
            return Result<OriginalSkill94Formula>::failure(resources.error());
        }
        auto& data = resources.value();
        OriginalSkill94Formula result;
        result.numerator_ = int(int8_t(data.bin[data.config + 0x98]));
        result.denominator_ = int(int8_t(data.bin[data.config + 0x99]));
        if (result.denominator_ <= 0)
        {
            return Result<OriginalSkill94Formula>::failure(make_error("Invalid sword94 divisor"));
        }
        ResourceReader palette(data.pal), bin(data.bin);
        const unsigned address = palette.number(856, 4);
        if (!palette || address < 12 || !bin.extent(address - 12, 9))
        {
            return Result<OriginalSkill94Formula>::failure(make_error("Missing sword94 formula header"));
        }
        const size_t header = address - 12;
        const unsigned groups = data.bin[header + 1], records = data.bin[header + 7];
        const size_t start = header + 9 + size_t(groups) * 2;
        if (groups < 10 || groups > 11 || records < 94 || !bin.extent(header + 9, size_t(groups) * 2 + size_t(records) * 37)
            || data.bin[header + 9 + 9 * 2] != 93 || !data.bin[header + 10 + 9 * 2])
        {
            return Result<OriginalSkill94Formula>::failure(make_error("Missing sword94 formula record"));
        }
        const size_t r = start + 93 * 37;
        if (data.bin[r + 19] != 1 || data.bin[r + 20] != 44 || bin.number(r + 21) != 50 || data.bin[r] != 10 || data.bin[r + 27] != 64 || data.bin[r + 28] != 0 || data.bin[r + 29] != 0 || data.bin[r + 31] != 2 || data.bin[r + 32] != 0 || data.bin[r + 33] != 194)
        {
            return Result<OriginalSkill94Formula>::failure(make_error("Unsupported sword94 formula record"));
        }
        return Result<OriginalSkill94Formula>::success(std::move(result));
    }
    explicit OriginalSkill94Formula(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    Result<int> try_raw_damage(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target) const { return try_raw_formula44(actor.fencing, actor.learned_proficiency[9], target.fencing, target.defense, numerator_, denominator_); }
    int raw_damage(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target) const
    {
        auto result = try_raw_damage(actor, target);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    int numerator() const { return numerator_; }
    int denominator() const { return denominator_; }
};
}    //namespace s2ne
